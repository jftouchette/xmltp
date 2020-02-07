/*  NAME: apl_log1.h
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
 * $Source: /ext_hd/cvs/include/apl_log1.h,v $
 * $Header: /ext_hd/cvs/include/apl_log1.h,v 1.6 2004/10/13 18:23:32 blanchjj Exp $
 *
 * General applications log functions.
 * 
 *  CONTENT:
 *	Defines, types (struct) and functions prototypes for all callers
 *	of the apl_log1.c functions.
 *
 *  GLOBAL VARIABLES USED:
 *
 *	from "aplinit.c":	verbose
 *
 *
 * ----------------------------------------------------------------------
 *
 * 30juin1994,jft: first version
 * 1997mar07,jft: + int alog_get_log_timestamp(char *edit_buffer, buffer_sz)
 * 2000jul07,deb: Added the possibility of multiple log files
 * 2001oct31,jbl: Added: alog_set_log_parent_pid_to_false()
 */



#ifndef _APL_LOG1_H_
#define _APL_LOG1_H_


#ifndef _INTSTRTB_H_
#include <intstrtb.h>		/* int to string */
#endif

#ifndef _ALOGELVL_H_DONE_
 #include <alogelvl.h>		/* the ALOG_xxxx defines - error levels */
#endif



/* ------------------------------------------------- PUBLIC FUNCTIONS: */


void alog_set_log_parent_pid_to_false();



void apl_log(int type, int func, int nval, void *pval);
/*
 *
 *	  int type	Type of message: Info, Warning, Error...
 *	  int func	Function indicator (Used to select a label)
 *	  int nval	Numeric value indicating stage or return code
 *	  void *pval	Pointer to a textual message (for now).
 *
 *	  Returns:	None.
 */



int alog_write_to_log(int type, int func, int nval, void *pval);
/*
 *	*** alog_write_to_log() is a superset of apl_log()
 *
 *	  Parameters:	see apl_log()
 *
 *	  Return:	0 or ERR_ALOG_OPEN_LOG_FAILED
 *			or ERR_ALOG_NO_LOG_FILENAME
 *			or ERR_ALOG_NO_TYPE_TABLE 
 *			or ERR_ALOG_NO_FUNC_TABLE
 *
 *	  NOTES:	Even if the return value is not 0, 
 *			alog_write_to_log() tries to write to the log,
 *			or to stdout.
 *
 *			The non-zero return codes happen when one of
 *			the following initialization functions has not
 *			been (properly) used before hand.
 *
 *			For an example, see mfsrvini_init_apl_log()
 *			in "mfsrvini.c"
 *
 */



int alog_aplogmsg_write(int err_level, char *module_name, char *app_name,
			int msg_no,    char *notify_to,	  char *msg_text);
/*
 * Called by:	aplogmsg.c
 */


void alog_set_srv_name(char *srv_name);
/*
 *	  Parameters:
 *		srv_name	name of the server, service or application
 *
 *
 */


char *alog_get_srv_name();
/*
 * 	Return:
 *		srv_name[] previously set with alog_set_srv_name()
 */


char *alog_get_name_of_log_file();
/*
 *	Return:
 *		"Active" log filename
 */




int alog_set_log_filename(char *filename);
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



int alog_bind_message_types_table(struct int_str_tab_entry  *p_int_str_tab,
				  int sizeof_table);
/*
 *	  Parameters:
 *		p_int_str_tab	pointer to an array of struct int_str_tab_entry
 *		sizeof_table	the numbers of bytes (sizeof) the table
 *
 *	  Return:
 *		int		(0) ISTR_STRING_TABLE_IS_OK
 *				or ISTR_ERR_PTR_TO_TABLE_IS_NULL
 *				or ISTR_ERR_TABLE_EMPTY
 *				or ISTR_ERR_NUMBERS_NOT_SORTED
 *				or ISTR_ERR_NULL_STRING_FOUND
 */

int alog_bind_func_strings_table(struct int_str_tab_entry  *p_int_str_tab,
				 int sizeof_table);
/*
 *	  Parameters:
 *		see alog_bind_message_types_table()
 *
 *	  Return:
 *		see alog_bind_message_types_table()
 */



int alog_get_log_timestamp(char *edit_buffer, int buffer_sz);
/*
 * This function format the date and time (including miliseconds)
 * into the buffer *edit_buffer.
 *
 * RETURN:
 *	-1	edit_buffer is a NULL pointer
 *	-2	buffer_sz indicates that the buffer is too small
 *	 0	OK
 */





int alog_activate_log_filename(int index);
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




int alog_set_log_filename_ext(char *p_filename, int index);
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





int alog_get_nb_log_filenames(void);
/*
 * This function returns the number of log filenames currently defined
 */





int alog_get_max_nb_log_filenames(void);
/*
 * This function returns the maximum number of log filenames that can be defined
 */




int alog_set_next_log_filename(char *filename);
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







/* ------------------------------------------------- PUBLIC DEFINES: */


/* for new implementation of "apl_log1.c":
 */
#define ERR_ALOG_OPEN_LOG_FAILED	-2
#define ERR_ALOG_NO_LOG_FILENAME	-3
#define ERR_ALOG_NO_TYPE_TABLE		-4
#define ERR_ALOG_NO_FUNC_TABLE		-5
#define ERR_ALOG_LOG_FILENAME_INDEX	-6




/* --------------------------------------- end of apl_log1.h ------------- */

#endif  /* ifndef _APL_LOG1_H_ */
