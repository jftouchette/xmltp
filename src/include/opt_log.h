/***********************************************************************
 * opt_log.h
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
 * $Source: /ext_hd/cvs/include/opt_log.h,v $
 * $Header: /ext_hd/cvs/include/opt_log.h,v 1.3 2004/10/15 20:32:42 jf Exp $
 *
 *
 * Optional Log definitions
 *
 *
 * 1997-05-27,jbl:	Initial creation.
 */


#include "sh_fun_d.h"
#include "apl_log1.h"


/* Any function return code. */

#define OPT_LOG_SUCCESS			0
#define OPT_LOG_FAIL			1
#define OPT_LOG_ERROR_PARAMS		2

/* Calling module may use this for message buffer size. */

#define OPT_LOG_BUFFER_LEN  		4096

#define OPT_LOG_APPL_NAME_SIZE		128
#define OPT_LOG_DEFAULT_NOTIFY_SIZE	128




/*
 **************************************************************************
 This function must be called before any other opt_log functions.

 app_name and default_notify can be passed as NULL. In that case
 app_name will be initialize to "" and default_notify to "nobody".
 **************************************************************************/

int opt_log_init_log(char *logname,char *app_name,char *default_notify);


/*
 **************************************************************************
 Same as "opt_log_init_log" except that it will bind the function table
 that is in the parametre line not the default table.
 **************************************************************************/

int opt_log_init_with_func(char *logname,char *app_name,char *default_notify,
				struct int_str_tab_entry  *p_int_str_tab, int sizeof_table);


/*
 **************************************************************************
 This function mus be use to log anything in the a standard log file.

 module name is normally the name of the source file from where the
 opt_log_aplogmsg function is call.

 err_level can be: ALOG_INFO, ALOG_ERROR, ALOG_WARN.

 msg_no is only required for ERROR and WARNING for info 0 must be given.

 **************************************************************************/

int opt_log_aplogmsg(	char	*module_name,   /* Name of the calling module*/
			int	err_level, 	/* ALOG_INFO, ALOG_WARN, ... */
			int	msg_no,		/* Message number.	     */
			char	*msg_text,	/* message text.             */
			long	trace_bit_mask);/* bit mask needed to log.*/


/*
 **************************************************************************
 Same as "opt_log_aplogmsg" except that will log the real PID and not the
 Parent PID. The "*module_name" parameter has been replace by "func_no", so
 instead of giving the name of the module we have to pass func_no of the
 func table that was bind previously with the function "opt_log_init_with_func"

 **************************************************************************/

int opt_logmsg_with_pid(int	func_no,        /* Number of the calling func. (module) */
			int	err_level, 	/* ALOG_INFO, ALOG_WARN, ... */
			int	msg_no,		/* Message number.	     */
			char	*msg_text,	/* message text.             */
			long	trace_bit_mask);/* bit mask needed to log.*/




/*
 **************************************************************************
 This function must be called in order to initialize the optional log
 bit mask. This bit mask will be compared to the one supply with the
 opt_log_aplogmsg. If any bit matche the message will be log. This is true
 only for INFO message type, ERROR and WARNING are always log.
 **************************************************************************/

int opt_log_set_trace_bit_mask(long trace_bit_mask);




/*
 **************************************************************************
 This function allow changing of default_notify value. 
 **************************************************************************/

int opt_log_set_default_notify(char *default_notify);
