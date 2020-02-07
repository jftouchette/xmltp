/******************************************************************
 * ct10init.h
 *
 *
 *  Copyright (c) 1997-2001 Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/include/ct10init.h,v $
 * $Header: /ext_hd/cvs/include/ct10init.h,v 1.4 2006/01/27 19:16:41 toucheje Exp $
 *
 * 1997-08-19,jbl: Initial Creation.
 * 2006jan26,jft:  change #define S_HOSTNAME  150 -- was 12, which is too short with the longer hostnames.
 */



#define CT10INIT_VERSION_NUM	"1.0"
#define CT10INIT_VERSION_ID	(CT10INIT_VERSION_NUM " " __FILE__ ": " \
__DATE__ " - " __TIME__)
#define CT10INIT_MODULE_NAME	(__FILE__)
 
/***************************************************************************
 Command line argument and flag definition
 ***************************************************************************/

#define CT10INIT_ARG_CONFIG_FILE		'F'
#define CT10INIT_ARG_CONFIG_FILE_SECTION	'C'
#define CT10INIT_ARG_SERVER_NAME		'S'
#define CT10INIT_ARG_USER_NAME			'U'
#define CT10INIT_ARG_PASSWORD_FILENAME		'P'
#define CT10INIT_ARG_INIT_STRING		'I'
#define CT10INIT_ARG_APPL_NAME			'A'
#define CT10INIT_ARG_DEBUG_MODE			'D'
#define CT10INIT_ARG_LOG_FILE			'L'


/***************************************************************************
 Config file string
 ***************************************************************************/

#define CT10INIT_CONFIG_STRING_LOG_FILENAME		"LOG_FILENAME"
#define CT10INIT_CONFIG_STRING_EXEC_TIMEOUT		"EXEC_TIMEOUT"
#define CT10INIT_CONFIG_STRING_SERVER_NAME		"SERVER_NAME"
#define CT10INIT_CONFIG_STRING_USER_NAME		"LOGIN_NAME"
#define CT10INIT_CONFIG_STRING_PASSWORD_FILENAME	"PASSWORD_FILENAME"
#define CT10INIT_CONFIG_STRING_INIT_STRING		"INIT_STRING"
#define CT10INIT_CONFIG_STRING_APPL_NAME		"APPL_NAME"

#define CT10INIT_CONFIG_STRING_DEBUG_MODE		"DEBUG_MODE_BIT_MASK"

/******
#define CT10INIT_CONFIG_STRING_DEFAULT_NB_EXTEND	"DEFAULT_NB_EXTEND"
#define CT10INIT_CONFIG_STRING_DYNARRAY_INITIAL_SIZE	"INITIAL_SIZE_"
#define CT10INIT_CONFIG_STRING_DYNARRAY_NB_EXTEND	"NB_EXTEND_"
#define CT10INIT_CONFIG_STRING_DYNARRAY_EXTEND_SIZE	"EXTEND_SIZE_"
*******/



/***************************************************************************
 Main program return code
 ***************************************************************************/

#define CT10INIT_SUCCESS			0
#define CT10INIT_INVALID_ARGUMENT		1
#define CT10INIT_FAIL				2


/***************************************************************************
 Other definitions
 ***************************************************************************/

#define CT10INIT_MODULE				"ct10init"

#define CT10INIT_DEFAULT_APPL_NAME		"isql"

#define S_HOSTNAME				150
#define S_LOGIN_NAME				12



/***************************************************************************
 config_prefix_ind valid value.
 ***************************************************************************/

#define	CT10INIT_CONFIF_PREFIX_IND_NONE         1
#define	CT10INIT_CONFIG_PREFIX_IND_SERVER       2
#define	CT10INIT_CONFIG_PREFIX_IND_APPL_NAME    3




/***************************************************************************
 CT10API standard initialization function.


 Calling function must always include this in the arg_string:
	":F:C:S:U:P:A:D:L:I:?"
 Application specific arguments can be placed at the end of that string.

 Pass p_appl_name as NULL and the function will try ti get in from
 the log file or the command line argument.

 config_prefix_ind:

	CT10INIT_CONFIF_PREFIX_IND_NONE         1
	CT10INIT_CONFIG_PREFIX_IND_SERVER       2
	CT10INIT_CONFIG_PREFIX_IND_APPL_NAME    3

 return code:

	CT10INIT_SUCCESS
	CT10INIT_FAIL
	CT10INIT_INVALID_ARGUMENT

 ***************************************************************************/

int ct10init_standard_ct10api_init(
	void	**ct10_connect,
	char	*arg_string,
	int	ac,
	char	*av[],
	char	*config_section,
	int	config_prefix_ind,
	char	*module_for_log,
	char	*p_appl_name);

char *ct10init_get_option_or_config_value(
	char	option_letter, 
	char	*config_file_string);


int ct10init_get_flag_or_config_value(	char option_letter, 
					char *config_file_string);

/* ------------------ End of ct10init.h ------------------------------------ */

