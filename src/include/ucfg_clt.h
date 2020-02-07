/* ucfg_clt.h
 * ----------
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
 * $Source: /ext_hd/cvs/include/ucfg_clt.h,v $
 * $Header: /ext_hd/cvs/include/ucfg_clt.h,v 1.4 2004/10/13 18:23:38 blanchjj Exp $
 *
 *
 * Functions prefix:	ucfg_clt_
 *
 *
 * DESCRIPTION:
 *
 * This file wraps the calls to the functions defined in ucfg.h.  It adds
 * the capability of searching configuration parameters with a prefix and
 * if it is not found without a prefix.  The constants, return values,
 * error codes, data sizes are the same as that defined in ucfg.h.
 * 
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
 * The string returned by ucfg_clt_get_param_string() is a pointer to 
 * a static area (where the value has been copied). Consequently, using
 * ucfg_clt_get_param_string(x) twice or more as arguments of a function
 * call will NOT give the expected results.
 *
 * Parameter value should not contain embedded spaces or tabs.
 *
 * ---------------------------------------------------------------------------
 * Versions:
 * 1997may27,deb: Initial revision
 * 2001apr13,deb: Added explicit void argument in:
 *                . char *ucfg_clt_version_id( void );
 *                . char *ucfg_clt_low_level_version_id( void );
 *                . char *ucfg_clt_err_bad_param_name( void );
 *                to please compiler
 */

#ifndef UCFG_CLT_H
#define UCFG_CLT_H


#include "ucfg.h"


#define  UCFG_CLT_MAX_PREFIX_LEN		128


/* ------------------------------------------------ Public structures: */



/* ------------------------------------------------ Public functions: */


char *ucfg_clt_version_id( void );
/*
 * Return version id string of module.
 */



char *ucfg_clt_low_level_version_id( void );
/*
 * Return version id string of ucfg.c module
 */



void ucfg_clt_set_prefix( char *prefix, int len_prefix );
/*
 * Sets the prefix used to get parameter info
 */



char *ucfg_clt_get_prefix( void );
/*
 * Returns a pointer to a static containing the prefix
 * used to get parameters
 */



int ucfg_clt_open_cfg_at_section( char *cfg_file_name,
				  char *section_name);
/*
 * DESCRIPTION:
 *	Open file cfg_file_name, locate section section_name, and read the 
 *	parameters values.  The parameters are searched first by appending
 *	the prefix and then, if none is found, without a prefix.
 *
 * WARNING:
 *	You can but SHOULD NOT call ucfg_clt_get_param_number() and
 *	ucfg_clt_get_param_string() after this function has returned
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



int ucfg_clt_check_all_param_defs( int  nb_params_required,
				   struct ucfg_check_param_def_entry val_tab[]);
/*
 * DESCRIPTION:
 *	Checks that all names in val_tab[] can be found in the config that
 *	was previously opened and loaded with ucfg_clt_open_cfg_at_section().
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





char *ucfg_clt_err_bad_param_name( void );
/*
 * DESCRIPTION:
 *	if ucfg_clt_open_cfg_at_section() has returned 
 *	UCFG_PARAM_VALUE_TRUNCATED or UCFG_PARAM_DUPLICATED,
 *	you can use this function to get the name of the problem
 *	parameter.
 *
 * Return Value:
 *	NULL (if there was no error specific to one parameter)
 *	or the name of the problem parameter.
 */



int ucfg_clt_copy_param_values_list_to_array( char *param_name,
					      char *ptr_array[], int max_sz);
/*
 * ************************* TO BE DEVELOPPED ***************************
 */




int ucfg_clt_is_string_in_param_values_list_ext(char *param_name,
						char *a_string,
						int   b_with_pattern);
/*
 * Return:
 *	-1	if param_name not found
 *	 1	if a_string is in the list of values for param param_name
 *	 0	if param_name exist but a_string is not amongst its values
 */




int ucfg_clt_is_string_in_param_values_list_pattern(char *param_name,
						char *a_string);
/*
 * Return 1 if a_string is in the list of values for param param_name
 * else return 0
 */



int ucfg_clt_is_string_in_param_values_list(char *param_name, char *a_string);
/*
 * Return 1 if a_string is in the list of values for param param_name
 * else return 0
 */




char *ucfg_clt_get_param_string_first( char *param_name );
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
 *	return NULL if ucfg_clt_open_cfg_at_section() failed and param was not
 *	initialized or if param_name does not match any of the parameters read
 *	from the config file.
 */



char *ucfg_clt_get_param_string_next( char *param_name );
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
 *	return NULL if ucfg_clt_open_cfg_at_section() failed and param was not
 *	initialized or if param_name does not match any of the parameters read
 *	from the config file or if the list of values is exhausted.
 */

#endif		/* ifdef UCFG_CLT_H */

/* --------------------------- end of ucfg_clt.h ----------------------------- */

