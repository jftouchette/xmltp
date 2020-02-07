/*  NAME: apl_log1.c
 *
 *
 *  Copyright (c) 1994-2001 Jean-Francois Touchette et Jocelyn Blanchet.
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
 * $Source: /ext_hd/cvs/log_fn/apl_log1.c,v $
 * $Header: /ext_hd/cvs/log_fn/apl_log1.c,v 1.15 2004/10/13 18:23:39 blanchjj Exp $
 *
 * @(#) Fichier:   $Source: /ext_hd/cvs/log_fn/apl_log1.c,v $
 * @(#) Revision:  $Header: /ext_hd/cvs/log_fn/apl_log1.c,v 1.15 2004/10/13 18:23:39 blanchjj Exp $
 *
 *
 *   Generic applications log functions. (Generalization of "apl_log.c")
 *
 *
 *  PURPOSE:
 *	Write application messages into a log file.
 *
 *  WARNING:
 *	See "GLOBAL VARIABLES USED".
 *
 *  FUNCTIONS:
 *	*** See descriptions for each public functions in the "apl_log1.h"
 *	*** include file.
 *
 *
 *
 *  GLOBAL VARIABLES USED:
 *
 *	from "aplinit.c":	verbose
 *
 *
 *	NOTE:	historically from first version of apl_log()
 *
 *
 *
 *  AUTHOR: P.C., Feb 1994
 *
 *  2nd Edition: JF Touchette, 30juin1994: all new alog_xxx() functions.
 *
 * ----------------------------------------------------------------------
 *
 * 30juin1994,jft: generalization of the apl_log.c function
 * 1997jun02,deb: included #include <process.h> in the #ifdef WIN32
 * 2001jan03,deb: . Changed #ifdef _AIX for #if defined(_AIX) || defined(linux)
 * 2001oct31,jbl: Add static s_log_parent_pid (default to true)
 *		  + function alog_set_log_parent_pid_to_false
 * 2004fev15,jft: Changed to #ifndef WIN32 just before #include <sys/time.h> to make porting easier (to OSX)
 */

#define _HPUX_SOURCE			/* no impact on NT port         */

/* #if defined(_AIX) || defined(linux) 
 */
#ifndef WIN32
#include <sys/time.h>	/* for time_t, struct timeval			*/
#else
#include <time.h>	/* for time_t, struct timeval			*/
#endif

#ifdef WIN32
#define	pid_t	int	/* pid_t is not available with WatCom	*/
#include "ntunixfn.h"
#include <process.h>	/* for getpid(), ...				*/
#endif

#include <stdio.h>	/* uses stdout */
#include <unistd.h>

#ifndef _INCLUDE_POSIX_SOURCE
#define _INCLUDE_POSIX_SOURCE
#include <sys/types.h>	/* for pid_t */
#undef  _INCLUDE_POSIX_SOURCE
#else
#include <sys/types.h>	/* for pid_t */
#endif

#if 0
#include <syslog.h>	/* unused - 19aug1994,jft */
#endif
#include <string.h>
#include "intstrtb.h"			/* must be before apl_log1.h */
#include "apl_log1.h"			/* must follow intstrtb.h */




/* -------------------------------------- extern of GLOBALS: */

extern int verbose;	/* usually defined in aplinit.c */


/* -------------------------------------- hidden variables: ---------- */

#define MAX_SRV_NAME	8
static char			 s_srv_name[MAX_SRV_NAME+1]	= "apl_log1";

static char		        *s_p_log_filename	 	= NULL;
static char		        *s_p_p_log_filenames[] = {
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL};
#define	MAX_LOG_FILE_NAMES 	(sizeof(s_p_p_log_filenames) / sizeof(char *))

static int			s_nb_of_log_filenames		= 0;

static struct int_str_tab_entry	*s_p_type_strings_table		= NULL,
				*s_p_func_strings_table		= NULL;

static int			 s_nb_bytes_type_strings_table	= 0,
				 s_nb_bytes_func_strings_table	= 0;


static int s_log_parent_pid = 1;	/* Default is log parent pid for batch log */


/* --------------------------------- Private (hidden) functions: ---------- */

static int get_log_time(char *edit_buffer, int buffer_sz)
{
#ifdef WIN32
	SYSTEMTIME timeval;
#else
	struct	timeval	 tval_now;	/* time, to the microseconds */
	struct	timezone tz_dummy;	/* passed, not used: env.var TZ is */
	char	timwork[60];		/* Text buffer to write Time Of Day */
#endif

	if (edit_buffer == NULL) {
		return (-1);
	}
	if (buffer_sz < 50) {
		return (-2);
	}

#ifdef WIN32
	GetLocalTime( &timeval );

	sprintf( edit_buffer, "%04d-%02d-%02d %02d:%02d:%02d:%03d",
	    timeval.wYear, timeval.wMonth, timeval.wDay,
		timeval.wHour, timeval.wMinute, timeval.wSecond,
		timeval.wMilliseconds );

#else
	/* Get the time with microseconds resolution:
	 */
	gettimeofday(&tval_now, &tz_dummy );
	
	/* Convert timeval struct values to a string:
	 */
	bzero(timwork, sizeof(timwork) );		/* fill with '\0' */

	strftime(timwork, sizeof(timwork), "%Y-%m-%d %H:%M:%S",
		 localtime( (time_t *) &(tval_now.tv_sec) ) );

	/* Concat miliseconds (usec / 1000) to date-time string
	 * in the edit_buffer:
	 */
	sprintf(edit_buffer, "%.44s:%03d",
			     timwork, ( (tval_now.tv_usec + 499L) / 1000L) );
#endif

	return (0);

} /* end of get_log_time() */




static int alog_write_log_line(FILE *fp,
			       char *p_type, char *p_func, char *sdel,
			       int func, int nval, void *pval)
{
 char	time_string[80] = "[time?]";

 if (fp == NULL) {
 	return (-1);
 }
 get_log_time(time_string, sizeof(time_string) );

 fprintf(fp,"%.40s;%.15s;%.8s;%5d;%4d;%3d;%s%s  %s.\n", 
	 time_string, p_type, s_srv_name, getpid(),
	 func, nval, p_func, sdel, pval);

 fflush(fp);
 
 return (0);
} /* end of alog_write_log_line() */




static int sf_write_batch_log(FILE *fp,
			      char *p_err_level, 
			      char *module_name, char *app_name, int msg_no,
			      char *notify_to,	 char *msg_text,
			      int   use_parent_pid)
{
	pid_t	pid = 0;
	char	time_string[80] = "[time?]";

	if (fp == NULL) {
 		return (-1);
	}
	get_log_time(time_string, sizeof(time_string) );

	if (use_parent_pid) {
		pid = getppid();
	} else {
		pid = getpid();
	}
	fprintf(fp, "%.40s;%.15s;%.60s;%5d;%.20s;%3d;%.32s; %.2000s.\n", 
		 time_string, p_err_level, module_name, pid, 
		 app_name, msg_no, notify_to, msg_text);

	fflush(fp);
 
	return (0);

} /* end of sf_write_log_batch() */




static char *sf_get_message_level_desc(int msg_level)
/* 
 * Try to find the text string that matches with message type (number),
 * if not found, find the default one from the table...
 */
{
	char *p_type = NULL;

	if ((p_type = istb_string_for_num(s_p_type_strings_table, 
		  s_nb_bytes_type_strings_table, msg_level) ) == NULL
	 && (p_type = istb_string_for_num(s_p_type_strings_table, 
	 	 s_nb_bytes_type_strings_table, ISTR_NUM_DEFAULT_VAL) )
	  == NULL) {
		p_type = "?type";
	}
	return (p_type);

} /* end of sf_get_message_level_desc() */



static char *sf_get_function_name(int func_no)
/* 
 * Try to find the text string that matches with func (number),
 * if not found, find the default one from the table...
 */
{
	char	*p_func = NULL;

	if ((p_func = istb_string_for_num(s_p_func_strings_table, 
		s_nb_bytes_func_strings_table, func_no) ) == NULL
	 && (p_func = istb_string_for_num(s_p_func_strings_table, 
		s_nb_bytes_func_strings_table, ISTR_NUM_DEFAULT_VAL) )
	 == NULL) {
		p_func = "?func";
	}
	return (p_func);

} /* end of sf_get_function_name() */





static int sf_check_previous_init()
/* 
 * Called by:   alog_write_to_log_ext()
 */
{
	int	rc = 0;

	if (s_p_type_strings_table == NULL) {
		rc = ERR_ALOG_NO_TYPE_TABLE;
	}
	if (s_p_func_strings_table == NULL) {
		rc = ERR_ALOG_NO_FUNC_TABLE;
	}
	if (s_p_log_filename == NULL) {
		rc = ERR_ALOG_NO_LOG_FILENAME;
	}
	return (rc);

} /* end of sf_check_previous_init() */





/*
 * This function is used to define the name of a log file.
 *
 * p_filename:	Name of the log file
 *
 * index:	Index of the log filename must be
 *              0 <= index < s_nb_of_log_filenam <= MAX_LOG_FILE_NAMES
 *
 * Returns:
 * 0:		Success
 *
 * ERR_ALOG_NO_LOG_FILENAME: Either p_filename is NULL or the string
 *              could not be allocated
 *
 * ERR_ALOG_LOG_FILENAME_INDEX: index is out of bound, it does not respect
 *              0 <= index < s_nb_of_log_filename <= MAX_LOG_FILE_NAMES
 *		s_nb_of_log_filename can be retrieved with
 *		alog_get_nb_log_filenames();
 */
static int sf_set_log_filename(char *p_filename, int index) {
	if (NULL == p_filename) {
		return (ERR_ALOG_NO_LOG_FILENAME);
	}

	if ((index < 0) || (index >= MAX_LOG_FILE_NAMES) ||
	    (index > s_nb_of_log_filenames)) {
		return (ERR_ALOG_LOG_FILENAME_INDEX);
	}

	if (s_p_p_log_filenames[index] != NULL) {
		free(s_p_p_log_filenames[index]);
		s_p_p_log_filenames[index] = NULL;
	}

	s_p_p_log_filenames[index] = strdup(p_filename);

	if (NULL == s_p_p_log_filenames[index]) {
		return (ERR_ALOG_NO_LOG_FILENAME);
	}

	if (index == s_nb_of_log_filenames) {
		s_nb_of_log_filenames++;
	}

	return (0);
} /* end of sf_set_log_filename */




/* --------------------------------- Public (visible) functions: ---------- */



/*
 * We dont always want to log the parent pid.
 * this function enable to change the default.
 */

void alog_set_log_parent_pid_to_false()
{

	s_log_parent_pid = 0;

} /* End of alog_set_log_parent_pid_to_false() */



int alog_write_to_log_ext(int err_level, int func, int msg_no, void *pval,
			  char *module_name, char *app_name, char *notify_to)
/* 
 * Called by:	alog_write_to_log()	---> module_name == NULL, func >= 0
 *		alog_aplogmsg_write()	---> module_name != NULL, func == -1
 */
{
  FILE	 *fplog = NULL;
  char   *sdel;		/* Pointer to label delimiter */
  char	 *p_type;	/* Pointer to text, type of msg */
  char	 *p_func;	/* Pointer to text, function name */
  int	  rc = 0;	/* return code */

  rc = sf_check_previous_init();

  /* Get function name matching func
   * and error level description matching err_level:
   */
  if (module_name == NULL) {
	p_func = sf_get_function_name(func);
  } else {
  	p_func = "nofunct!";
  }
  p_type = sf_get_message_level_desc(err_level);

  /* Prepare sdel delimiter according to pval and _func:
   */
  if (pval == NULL) {
	pval = (void *) "";
	sdel = (char *) "";
  } else {
	if(strlen(p_func) == 0)
		sdel = (char *) "";
	else
		sdel = (char *) ", ";
  }

  if (s_p_log_filename == NULL) {
	fplog = NULL;
  } else {
  	fplog = fopen(s_p_log_filename, "a");		/* Open log file */
	if (fplog == NULL) {
		rc = ERR_ALOG_OPEN_LOG_FAILED;
	}
  }

  /* Display log message on stdout if needed: */

  /*
  ** If the Verbose indicator is greater then one or
  ** we are unable to open the log file, send the log 
  ** message to the standard error device. 
  **
  ** This change will allow applications to determine
  ** at runtime (by setting "verbose") where the log
  ** entry should be sent. This is required in the case
  ** where an error is detected at server "start up"
  ** time and we want to insure the system operator
  ** would see the error, even in the case where 
  ** standard output is redirected to a file.
  **
  */
  if (module_name == NULL) {
	if (verbose > 1 || fplog == NULL) {
		alog_write_log_line(stderr, p_type, p_func, sdel, func, 
					    msg_no, pval);
	} else {
  		if (verbose) { 
    			alog_write_log_line(stdout, p_type, p_func, sdel, func, 
    						    msg_no, pval);
		}
	}
	/* Write message to log file:
	 */
	if (fplog != NULL) {
		rc = alog_write_log_line(fplog, p_type, p_func, sdel, func, 
						msg_no, pval);
	}
  } else {
  	/*
  	 * The logging format when this function is called by a batch 
  	 * application is rather different.
  	 * So, we call a different function to format and write the line
  	 * to the log:
  	 */
	if (verbose || fplog == NULL) {
		sf_write_batch_log(stderr,
				   p_type,				/* error level description */
			 	   module_name, app_name, msg_no,
				   notify_to,	
				   pval,				/* message text */
				   s_log_parent_pid);			
	}
	/* Write message to log file:
	 */
	if (fplog != NULL) {
		sf_write_batch_log(fplog, 
				   p_type,				/* error level description */
			 	   module_name, app_name, msg_no,
				   notify_to,	
				   pval,				/* message text */
				   s_log_parent_pid);
	}
  }

  if (fplog != NULL) {
	fclose(fplog);
  }
  return (rc);

} /* end of alog_write_to_log_ext() */




int alog_write_to_log(int err_level, int func, int msg_no, void *pval)
/*
 * alog_write_to_log() is a superset of apl_log():
 */
{
	return (
		alog_write_to_log_ext(err_level, func, msg_no, pval,
					NULL,	/* module_name == NULL */
					"unknown",
					"nobody")
		);
}



/* apl_log() -- backward compatible function 
 */
void apl_log(int err_level, int func, int msg_no, void *pval)
{
	alog_write_to_log(err_level, func, msg_no, pval);
}



int alog_aplogmsg_write(int err_level, char *module_name, char *app_name,
			int msg_no,    char *notify_to,	  char *msg_text)
/*
 * Called by:	aplogmsg.c
 *
 *
 */
{
	int rc;

	rc = alog_write_to_log_ext(err_level,
				   -1,		/* func_no: not applicable */
				   msg_no, 
				   msg_text,
				   module_name, 
				   app_name,
				   notify_to);

	return (rc);

} /* end of alog_aplogmsg_write() */



void alog_set_srv_name(char *srv_name)
{
	strncpy(s_srv_name, srv_name, MAX_SRV_NAME);

	/* The NUL bytes at [MAX_SRV_NAME] is already initialized as []
	 * s_srv_name is a static.
	 */
}



char *alog_get_srv_name()
{
	return (s_srv_name);
}



/*
 * This function returns the name of the "active" log file name
 */
char *alog_get_name_of_log_file()
{
	return (s_p_log_filename);
}





/*
 * This function sets the log filename used for the next
 * apl_log_xxxx() calls.
 *
 * index: The number of the file name to activate (between 0
 *        and the number of log file names that has been previously
 *        set.
 *
 * return:
 * 0:    success
 *
 * ERR_ALOG_LOG_FILENAME_INDEX: The index specified is out of
 *       bound, either < 0 or the index exceeds or equals the
 *       number of file names defined
 *
 * ERR_ALOG_NO_LOG_FILENAME:  The index specified correspond
 *       to a filename that could not be allocated
 */
int alog_activate_log_filename(int index) {
	char *p = NULL;

	if ((index < 0) || (index >= s_nb_of_log_filenames)) {
		return (ERR_ALOG_LOG_FILENAME_INDEX);
	}

	p = s_p_p_log_filenames[index];

	if (NULL == p) {
		return (ERR_ALOG_NO_LOG_FILENAME);
	}

	s_p_log_filename = p;

	return (0);
}




/*
 * This function is used to set a given log filename
 *
 * p_filename:	Name of the log file
 *
 * index:	Index of the log filename must be
 *              0 <= index < s_nb_of_log_filenam <= MAX_LOG_FILE_NAMES
 *
 * Returns:
 * 0:		Success
 *
 * ERR_ALOG_NO_LOG_FILENAME: Either p_filename is NULL or the string
 *              could not be allocated
 *
 * ERR_ALOG_LOG_FILENAME_INDEX: index is out of bound, it does not respect
 *              0 <= index < s_nb_of_log_filename <= MAX_LOG_FILE_NAMES
 *		s_nb_of_log_filename can be retrieved with
 *		alog_get_nb_log_filenames();
 */
int alog_set_log_filename_ext(char *p_filename, int index) {
	return (sf_set_log_filename(p_filename, index));
}





/*
 * This function returns the number of log filenames currently defined
 */
int alog_get_nb_log_filenames(void) {
	return (s_nb_of_log_filenames);
}





/*
 * This function returns the maximum number of log filenames that can be defined
 */
int alog_get_max_nb_log_filenames(void) {
	return (MAX_LOG_FILE_NAMES);
}




/*
 * This function is used to set a given log filename
 *
 * p_filename:	Name of the log file
 *
 * Returns:
 * 0:		Success
 *
 * ERR_ALOG_NO_LOG_FILENAME: Either p_filename is NULL or the string
 *              could not be allocated
 *
 * ERR_ALOG_LOG_FILENAME_INDEX: Max number of filenames have already
 *              been defined.  This maximum is MAX_LOG_FILE_NAMES
 *		the current number of log fileanmes can be retrieved
 *		with alog_get_nb_log_filenames();
 */
int alog_set_next_log_filename(char *p_filename) {
	return (sf_set_log_filename(p_filename, s_nb_of_log_filenames));
}





/*
 * This function is used to set the first log filename.  It activates
 * this filename.
 *
 * p_filename:	Name of the log file
 *
 * Returns:
 * 0:		Success
 *
 * ERR_ALOG_NO_LOG_FILENAME: Either p_filename is NULL or the string
 *              could not be allocated
 */
int alog_set_log_filename(char *p_filename)
{
	int rc = sf_set_log_filename(p_filename, 0);

	s_p_log_filename = s_p_p_log_filenames[0];

	return (rc);
}





int alog_bind_message_types_table(struct int_str_tab_entry  *p_int_str_tab,
				  int sizeof_table)
{
	if (p_int_str_tab == NULL) {
		return (ISTR_ERR_PTR_TO_TABLE_IS_NULL);
	}
	if (sizeof_table < sizeof(struct int_str_tab_entry)) {
	 	return (ISTR_ERR_TABLE_EMPTY);
	}
	s_p_type_strings_table = p_int_str_tab;
	s_nb_bytes_type_strings_table = sizeof_table;

	return (istb_check_int_strings_table(p_int_str_tab,
				  	     sizeof_table) );

} /* end of alog_bind_message_types_table() */




int alog_bind_func_strings_table(struct int_str_tab_entry  *p_int_str_tab,
				 int sizeof_table)
{
	if (p_int_str_tab == NULL) {
		return (ISTR_ERR_PTR_TO_TABLE_IS_NULL);
	}
	if (sizeof_table < sizeof(struct int_str_tab_entry)) {
	 	return (ISTR_ERR_TABLE_EMPTY);
	}
	s_p_func_strings_table = p_int_str_tab;
	s_nb_bytes_func_strings_table = sizeof_table;

	return (istb_check_int_strings_table(p_int_str_tab,
				  	     sizeof_table) );

} /* end of alog_bind_func_strings_table() */




int alog_get_log_timestamp(char *edit_buffer, int buffer_sz)
/*
 * This function format the date and time (including miliseconds)
 * into the buffer *edit_buffer.
 *
 * RETURN:
 *	-1	edit_buffer is a NULL pointer
 *	-2	buffer_sz indicates that the buffer is too small
 *	 0	OK
 */
{
	return (get_log_time(edit_buffer, buffer_sz) );
}


/* --------------------------------------- end of apl_log1.c ------------- */

