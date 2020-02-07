/* ucfg.h			(To Read a UNIX style CONFIGURATION file)
 * ------
 *
 *  Copyright (c) 1994-2001 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/include/ucfg.h,v $
 * $Header: /ext_hd/cvs/include/ucfg.h,v 1.10 2004/10/13 18:23:37 blanchjj Exp $
 *
 *
 * Functions prefix:	ucfg_
 * Defines prefix:	UCFG_
 *
 *
 * DESCRIPTION:
 *
 * This file contains defines (constants) for the return values, the error 
 * codes, some data sizes and the prototypes of the functions to open the 
 * config file, read the parameters values and to get the value associated 
 * with the parameters.
 *
 *
 * WARNING:
 *
 * To ensure that the errors in parameter name references are discovered at
 * compile time rather than at run-time, the constant which defines the
 * parameter name should be used instead of the name (string) itself.
 *
 *
 * LIMITATIONS:
 *
 * The design of this module assumes that each executable program that uses
 * it only use ONE config file. (The module only keeps the values for the
 * last config file opened!)
 *
 * The maximum length of a parameter value (string) is 200 bytes
 * (UCFG_MAX_VALUE_LEN).
 *
 * This first version associates only one value to each parameter.
 *
 * The string returned by ucfg_get_param_string() is a pointer to 
 * a static area (where the value has been copied). Consequently, using
 * ucfg_get_param_string(x) twice or more as arguments of a function
 * call will NOT give the expected results.
 *
 * Parameter value should not contain embedded spaces or tabs.
 *
 * ---------------------------------------------------------------------------
 * Versions:
 * 1994Aug01,jft: initial version
 * 1997may29,deb: . added ucfg_string_is_number()
 * 1997aug22,deb: . Added ucfg_open_cfg_at_section_ext()
 *                . Added ucfg_assign_substitution_report_file_handle()
 * 2002nov01,DEV: . #define UCFG_MAX_SECTION_NAME_LEN	  80	-- was 50 before
 *		    #define UCFG_MAX_LINE_LEN		16384	-- was 4096 before
 *		    #define UCFG_MAX_PARAM_NAME_LEN		  80	-- was  50 before
 */

#define _UCFG_INCLUDED_


/* ------------------------------------------------ Public structures: */

struct ucfg_check_param_def_entry {
	char	*param_name;
	int	 param_type;
 } ;


/* ------------------------------------------------ Public functions: */


char *ucfg_version_id();
/*
 * Return version id string of module.
 */



int ucfg_open_cfg_at_section(char *cfg_file_name,
			     char *section_name);
/*
 * DESCRIPTION:
 *	Open file cfg_file_name, locate section section_name, and read the 
 *	parameters values.
 *
 * WARNING:
 *	You can but SHOULD NOT call ucfg_get_param_number() and
 *	ucfg_get_param_string() after this function has returned
 *	an error return value.
 *
 * Return Value:
 *	0				zero (0) indicates success
 *	UCFG_OPEN_FAILED
 *	UCFG_SERVER_NAME_NOTFND
 *	UCFG_BAD_FILE_FORMAT
 *	UCFG_PARAM_VALUE_TRUNCATED
 *	UCFG_PARAM_DUPLICATED		found another value for a parameter
 *					which already has one
 */




int ucfg_open_cfg_at_section_ext(char *cfg_file_name,
				char *section_name,
				int b_allow_duplicate_params,
				int b_destroy_old_params);
/*
 * DESCRIPTION:
 *	Open file cfg_file_name, locate section section_name, and read the 
 *	parameters values.
 *
 * If b_allow_duplicate_params == TRUE
 *     If a parameter is found with the same name as an existing parameter
 *     the new value replace the old one, and the new value is reported on
 *     stderr (unless the file pointer was changed by a call to
 *     ucfg_assign_substitution_report_file_handle()).
 * If b_allow_duplicate_params != TRUE
 *     If a parameter is found with the same name as an existing parameter
 *     the new value is ignored and the parameter name is kept for
 *     reporting by ucfg_err_bad_param_name().
 *
 * If b_destroy_old_params == TRUE
 *     The list of parameter is destroyed before reading the new config
 *     file.
 * If b_destroy_old_params != TRUE
 *     The parameters of the config file are appended to the list of parameters
 *     already read (if a config file was already read, otherwise the parameter
 *     has no effect).
 *
 * WARNING:
 *	You can but SHOULD NOT call ucfg_get_param_number() and
 *	ucfg_get_param_string() after this function has returned
 *	an error return value.
 *
 * Return Value:
 *	0				zero (0) indicates success
 *	UCFG_OPEN_FAILED
 *	UCFG_SERVER_NAME_NOTFND
 *	UCFG_BAD_FILE_FORMAT
 *	UCFG_PARAM_VALUE_TRUNCATED
 *	UCFG_PARAM_DUPLICATED		found another value for a parameter
 *					which already has one (only if
 *					b_allow_duplicate_params != TRUE)
 */






int ucfg_check_all_param_defs(int  nb_params_required,
			      struct ucfg_check_param_def_entry val_tab[]);
/*
 * DESCRIPTION:
 *	Checks that all names in val_tab[] can be found in the config that
 *	was previously opened and loaded with ucfg_open_cfg_at_section().
 *	Checks that the param_val is a numeric string if param_type ==
 *	UCFG_NUMERIC_PARAM.
 *	Also tell if the number of loaded params is > than sz_tab
 *
 * Return Value:
 *	0 or
 *	UCFG_PARAM_MISSING
 *	UCFG_PARAM_HAS_BAD_VAL		should be number, but is not
 *	UCFG_SUPPL_PARAM_FOUND
 */ 





char *ucfg_err_bad_param_name();
/*
 * DESCRIPTION:
 *	if ucfg_open_cfg_at_section() has returned 
 *	UCFG_PARAM_VALUE_TRUNCATED or UCFG_PARAM_DUPLICATED,
 *	you can use this function to get the name of the problem
 *	parameter.
 *
 * Return Value:
 *	NULL (if there was no error specific to one parameter)
 *	or the name of the problem parameter.
 */



int ucfg_copy_param_values_list_to_array(char *param_name,
					 char *ptr_array[], int max_sz);
/*
 * Copy each datum pointer of the list to one element of ptr_array[].
 *
 * Return number of values or LKLS_ERR_ARRAY_OVERFLOW if ctr >= max_sz
 * or LKLS_ERR_NULL_LIST_PTR if linked_list == NULL
 * or LKLS_ERR_BAD_SIGNATURE if linked_list is not a good handle
 * (detection is NOT guaranteed).
 */



int ucfg_is_string_in_param_values_list_ext(char *param_name, char *a_string,
					    int   b_with_pattern);
/*
 * Return:
 *	-1	if param_name not found
 *	 1	if a_string is in the list of values for param param_name
 *	 0	if param_name exist but a_string is not amongst its values
 */



int ucfg_is_string_in_param_values_list_pattern(char *param_name,
						char *a_string);
/*
 * Return 1 if a_string is in the list of values for param param_name
 * else return 0
 */



int ucfg_is_string_in_param_values_list(char *param_name, char *a_string);
/*
 * Return 1 if a_string is in the list of values for param param_name
 * else return 0
 */




char *ucfg_get_param_string_first(char *param_name);
/*
 * DESCRIPTION:
 *	returns the FIRST string value associated to the parameter
 *
 * NOTE: 
 *	The string returned is a pointer to a static area (where the value 
 *	has been copied).
 *
 * Return Value:
 *	return the string value associated to the parameter param_name
 *
 *	return NULL if ucfg_open_cfg_at_section() failed and param was not
 *	initialized or if param_name does not match any of the parameters read
 *	from the config file.
 */



char *ucfg_get_param_string_next(char *param_name);
/*
 * DESCRIPTION:
 *	returns the NEXT string value associated to the parameter
 *
 * NOTE: 
 *	The string returned is a pointer to a static area (where the value 
 *	has been copied).
 *
 * Return Value:
 *	return the string value associated to the parameter param_name
 *
 *	return NULL if ucfg_open_cfg_at_section() failed and param was not
 *	initialized or if param_name does not match any of the parameters read
 *	from the config file or if the list of values is exhausted.
 */



int ucfg_nb_params( void );
/*
 * DESCRIPTION:
 *	Returns the number of parameters read from the configuration file.
 */



char *ucfg_remember_bad_param_name( char *id_param, int reset_ind );
/*
 * DESCRIPTION:
 *	Allows a client to set (and clear) the bad parameter returned
 *	by ucfg_err_bad_param_name().
 *
 * NOTE:
 *	This functionality is used by the module ucfg_clt so it can
 *	report the bad parameter
 */



int ucfg_string_is_number( char *value );
/*
 * DESCRIPTION:
 *	Returns 1 if the string pointed by value is a numeric string (digits
 *	and optionally comma or dot), otherwise, returns 0
 *
 * NOTE:
 *	This functionality is used by the module ucfg_clt so it can
 *	apply the same validation of numeric parameters as ucfg
 */



int ucfg_assign_substitution_report_file_handle(FILE *fi);
/*
 * Used to change the file pointer used to report the new values of
 * duplicate parameters (when duplicates are allowed).
 * Default value of file pointer: stderr
 *
 * RETURNS: 0  
 *
 * Note:
 * It is the responsibility of the calling module to open the
 * file, to close it and to assure that the pointer passed is
 * a valid file pointer.  A NULL argument disables the reporting
 * of the duplicate parameters.
 */


/* -------------------------------------------- Public defines:  */

#define UCFG_NUMERIC_PARAM		  1
#define UCFG_STRING_PARAM		  2


#define UCFG_MAX_SECTION_NAME_LEN	  80	/* was 50 before 2002-nov-01*/
#define UCFG_MAX_LINE_LEN		16384	/* was 4096 before 2002-nov-01*/
#define UCFG_MAX_PARAM_NAME_LEN		  80	/* was 50 before 2002-nov-01*/
#define UCFG_MAX_VALUE_LEN		2048


#define	UCFG_OPEN_FAILED		 -1
#define	UCFG_SECTION_NAME_NOTFND	 -2
#define	UCFG_BAD_FILE_FORMAT		 -3
#define	UCFG_PARAM_DUPLICATED		 -8
#define	UCFG_PARAM_VALUE_TRUNCATED	 -9
#define	UCFG_PARAM_MISSING		-10
#define	UCFG_PARAM_HAS_BAD_VAL		-11
#define UCFG_PARAM_NOT_FOUND		-12
#define	UCFG_WRONG_SERV_TYPE		-20
#define	UCFG_SUPPL_PARAM_FOUND		 44
#define UCFG_CONFIG_FILE_NOT_LOADED	-90
#define	UCFG_OUT_OF_MEMORY		-100
#define	UCFG_LINKLIST_ERROR		-151

/* -------------------------------------------- end of ucfg.h ----------- */
