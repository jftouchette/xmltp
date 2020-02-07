/**********************************************************************
 * opt_log.c
 *
 *  Copyright (c) 1996-1999 Jocelyn Blanchet
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  (The file "COPYING" or "LICENSE" in a directory above this source file
 *  should contain a copy of the GNU Library General Public License text).
 *  -------------------------------------------------------------------------
 *
 * $Source: /ext_hd/cvs/log_fn/opt_log.c,v $
 * $Header: /ext_hd/cvs/log_fn/opt_log.c,v 1.4 2004/10/13 18:23:39 blanchjj Exp $
 *
 *
 *  Optional log api.
 *
 * CHANGES:
 * --------
 * 1997-05-27,jbl: Initial creation.
 * 1999-12-03,dmi: Add 2 new function: opt_log_init_with_func() and opt_logmsg_with_pid()
 */


#ifdef WIN32
#include "ntunixfn.h"
#endif 

#include <stdio.h>
#include "opt_log.h"

static long opt_log_trace_bit_mask;
static char opt_log_appl_name[OPT_LOG_APPL_NAME_SIZE];
static char opt_log_default_notify[OPT_LOG_DEFAULT_NOTIFY_SIZE];

int verbose = 0;		/* To please apl_log1.c */

/*
 * Log application message type table.
 *
 */

static struct int_str_tab_entry s_type_strings_table[] = {
#include "alogtypt.h"

	{ ISTR_NUM_DEFAULT_VAL,  "?     " }
};


/* This is only to please apl_log1.c 			*/
/* With this logging function will return 0.		*/
/* If not bind will always return -5.			*/

static struct int_str_tab_entry  s_func_strings_table[] = {
	{ 0,  "?     " }
};


/*
 *******************************
 Init log system and bind function name
 *******************************/

int opt_log_init_with_func(char *logname, char *appl_name, char *default_notify,
			struct int_str_tab_entry  *p_int_str_tab, int sizeof_table)
{
int     rc;



/* Parameters validation. */

if (logname == NULL) {
	return OPT_LOG_ERROR_PARAMS;
}

if (appl_name == NULL) {
	strcpy(opt_log_appl_name,"");
}
else {
	strncpy(opt_log_appl_name,appl_name,OPT_LOG_APPL_NAME_SIZE);
}

if (default_notify == NULL) {
	strcpy(opt_log_default_notify,"nobody");
}
else {
	strncpy(opt_log_default_notify,default_notify,
		OPT_LOG_DEFAULT_NOTIFY_SIZE);
}


/* apl_log1 initialisation. */

if ((rc = alog_set_log_filename(logname)) != 0) {
	return OPT_LOG_FAIL;
}

if ((rc = alog_bind_message_types_table(&s_type_strings_table[0],
				    sizeof(s_type_strings_table) )) 
		   != 0) {

	fprintf(stderr,"Log init: Impossible to bind msg type,rc: %d\n",rc);
	return OPT_LOG_FAIL;
}


if ((rc = alog_bind_func_strings_table(p_int_str_tab,
				    sizeof_table) )
		   != 0) {

	fprintf(stderr,"Log init: Impossible to bind fnc type,rc: %d\n",rc);
	return OPT_LOG_FAIL;
}

return OPT_LOG_SUCCESS;
} /* --------------------------- End of opt_log_init_with_func -------------------- */


/*
 *******************************
 Init log system. 
 *******************************/

int opt_log_init_log(char *logname, char *appl_name, char *default_notify)
{
int     rc;

	/* init log et binding with default function name */
	return( opt_log_init_with_func( logname, appl_name, default_notify,
				&s_func_strings_table[0], sizeof( s_func_strings_table ) ) );

} /* --------------------------- End of opt_log_init_log -------------------- */


/*
 ****************************
 Write log message.
 ****************************/

int opt_log_aplogmsg(	char	*module_name,   /* Name of the calling module*/
			int	err_level, 	/* ALOG_INFO, ALOG_WARN, ... */
			int	msg_no,		/* Message number.	     */
			char	*msg_text,	/* message text.             */
			long	trace_bit_mask) /* bit mask needed to log.*/
{
int rc=0;

if ((err_level != ALOG_INFO) || (trace_bit_mask & opt_log_trace_bit_mask)) {
	if ((rc=alog_aplogmsg_write(	err_level, module_name, opt_log_appl_name,
	  			msg_no, opt_log_default_notify, msg_text)) != 0) {

		fprintf(stderr,"Impossible to write to log file, rc: %d\n",rc);
		return OPT_LOG_FAIL;
	}
}
return OPT_LOG_SUCCESS;
} /* ------------------------ End of opt_log_aplogmsg --------------------- */


/*
 ****************************
 Write log message with process id
        (not the parent process id)
 ****************************/

int opt_logmsg_with_pid(int	func_no, 	/* Number of the calling func. (module) */
			int	err_level, 	/* ALOG_INFO, ALOG_WARN, ... */
			int	msg_no,		/* Message number.	     */
			char	*msg_text,	/* message text.             */
			long	trace_bit_mask) /* bit mask needed to log.*/
{
int rc=0;

if ((err_level != ALOG_INFO) || (trace_bit_mask & opt_log_trace_bit_mask)) {
	if ((rc=alog_write_to_log( err_level, func_no, msg_no, msg_text)) != 0) {

		fprintf(stderr,"Impossible to write to log file, rc: %d\n",rc);
		return OPT_LOG_FAIL;
	}
}
return OPT_LOG_SUCCESS;
} /* ------------------------ End of opt_logmsg_with_pid --------------------- */


/* 
 *****************************************************
 Set log bit mask.
 *****************************************************/

int opt_log_set_trace_bit_mask(long trace_bit_mask)
{
	opt_log_trace_bit_mask = trace_bit_mask;
	return OPT_LOG_SUCCESS;

} /* ------------- End of opt_log_set_trace_bit_mask --------------------- */


/* 
 *****************************************************
 Set log default notify.
 *****************************************************/

int opt_log_set_default_notify(char *default_notify) {

	if (default_notify == NULL) {
		strcpy(opt_log_default_notify,"");
	}
	else {
		strncpy(opt_log_default_notify,default_notify,
			OPT_LOG_DEFAULT_NOTIFY_SIZE);
	}

	return OPT_LOG_SUCCESS;

} /* ------------ End of opt_log_set_default_notify ------------ */
