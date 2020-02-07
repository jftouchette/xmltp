/* ucfg_clt.c
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
 * $Source: /ext_hd/cvs/cfg_read/ucfg_clt.c,v $
 * $Header: /ext_hd/cvs/cfg_read/ucfg_clt.c,v 1.4 2004/10/15 20:12:20 jf Exp $
 *
 * Public Functions prefix:	ucfg_clt_
 *
 *
 * DESCRIPTION:
 *
 * This file wraps the calls to the functions defined in ucfg.h.  It adds
 * the capability of searching configuration parameters with a prefix and
 * if it is not found without a prefix.  The constants, return values,
 * error codes, data sizes are the same as that defined in ucfg.h.
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
 * 1997nov14,deb: Added ucfg_clt_is_string_in_param_values_list_ext()
 */

#ifdef WIN32
#include "ntunixfn.h"
#endif

#include <stdio.h>
#include <stdlib.h>	/* mem functions prototypes */
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "linklist.h"
#include "ucfg_clt.h"





/* ------------------------------------------------ Private defines: */


#define VERSION_ID	(__FILE__ ": " __DATE__ " - " __TIME__)


#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif


/* ----------------------------------------------- Private variables: */


/*
 * String length is one more than the maximum prefix length to take into account
 * the NULL character terminating the string.
 */
static char s_prefix[ UCFG_CLT_MAX_PREFIX_LEN + 1 ] = "";




/* ------------------------------------------------ Private functions: */



int sf_does_prefixed_param_exists( char *param_name )
/*
 * DESCRIPTION:
 * 	When calling ucfg_clt_get_param_string_next(), it is necessary to
 *	determine before the call ucfg_get_param_string_next() if the prefixed 
 *	parameter exist or not.  The call to the function 
 *	ucfg_is_string_in_param_values_list_ext() will not disrupt the static
 *	iterator of the link list of the parameter
 *
 * RETURNS:
 *	TRUE:	the prefixed parameter exist
 *	FALSE:	the prefixed parameter does not exist
 *
 * NOTE:
 *	This technique is directly taken from the file oscfgpar.c
 */
{
	char prefixed_param[UCFG_CLT_MAX_PREFIX_LEN+UCFG_MAX_PARAM_NAME_LEN+3];

 	sprintf( prefixed_param, "%.*s.%.*s",
		 UCFG_CLT_MAX_PREFIX_LEN,
		 s_prefix,
		 UCFG_MAX_PARAM_NAME_LEN,
		 param_name );

	/*
	 * ucfg_is_string_in_param_values_list_ext() return (-1) if the
	 * parameter does not exist.
	 *
	 * Otherwise, it returns 1 or 0.
	 *
	 * "\1\1" is a string value that cannot be found. It is short
	 * so that the search in the list of values will be minimal.
	 */
	if ( ucfg_is_string_in_param_values_list_ext(
		prefixed_param, "\1\1", FALSE ) >= 0 ) {

		return ( TRUE );
	}

	return ( FALSE );
} /* end of sf_does_prefixed_param_exists() */



/* ------------------------------------------------ Public functions: */

char *ucfg_clt_version_id( void )
{
	return (VERSION_ID);
} /* end of ucfg_clt_version_id() */



char *ucfg_clt_low_level_version_id( void )
{
	return ( ucfg_version_id() );
} /* end of ucfg_clt_low_level_version_id() */




int ucfg_clt_open_cfg_at_section( char *cfg_file_name,
				  char *section_name )
{
	return ( ucfg_open_cfg_at_section( cfg_file_name, section_name ) );
} /* end of ucfg_clt_open_cfg_at_section() */





int ucfg_clt_check_all_param_defs( int  nb_params_required,
				   struct ucfg_check_param_def_entry val_tab[] )
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
{
 int	i;
 int	rc = 0;
 char	*p_val;
 char	*p_name;

 for ( i = 0; i < nb_params_required; i++ ) {
 	p_name = val_tab[i].param_name;

 	p_val = ucfg_clt_get_param_string_first( p_name );

	if ( p_val == NULL ) {

		/* Clear bad param name that might have been set by
		 * a UCFG_PARAM_HAS_BAD_VAL error:
		 */
		ucfg_remember_bad_param_name( "reset", 1 );

		/* Memorize the missing param name:
		 */
		ucfg_remember_bad_param_name( p_name, 0 );

		return ( UCFG_PARAM_MISSING );	/* return critical error */
	}

	/* If the parameter type is numeric, check if all the values are numeric
	 */
	if ( val_tab[i].param_type == UCFG_NUMERIC_PARAM ) {
		while ( ( p_val != NULL ) && ( rc == 0 ) ) {
			if ( ucfg_string_is_number( p_val ) == 0 ) {
				ucfg_remember_bad_param_name( p_name, 0 );
				rc = UCFG_PARAM_HAS_BAD_VAL;
			}

 			p_val = ucfg_clt_get_param_string_next( p_name );
		}
	}
 } /* end loop - all params in table */

 if (rc != 0)
	return (rc);

 return ( ( ucfg_nb_params() > nb_params_required )
 	 ? UCFG_SUPPL_PARAM_FOUND : 0);

} /* end of ucfg_clt_check_all_param_defs() */





char *ucfg_clt_err_bad_param_name()
/*
 * if ucfg_open_cfg_at_section() has returned UCFG_PARAM_VALUE_TRUNCATED or
 * UCFG_PARAM_DUPLICATED, you can use this function to get the name of the
 * problem parameter.
 *
 * Return NULL (if there was no error specific to one parameter)
 * or the name of the problem parameter.
 */
{
	return ( ucfg_remember_bad_param_name( NULL, 0 ) );

} /* end of ucfg_err_bad_param_name() */




int ucfg_clt_copy_param_values_list_to_array(char *param_name,
					 char *ptr_array[], int max_sz)
/*
 * ************************* TO BE DEVELOPPED ***************************
 */
{
 return ( 0 );
} /* end of ucfg_copy_param_values_list_to_array() */




int ucfg_clt_is_string_in_param_values_list_ext(char *param_name,
						char *a_string,
						int   b_with_pattern)
/*
 * Return:
 *	-1	if param_name not found
 *	 1	if a_string is in the list of values for param param_name
 *	 0	if param_name exist but a_string is not amongst its values
 */
{
 	char prefixed_param[UCFG_CLT_MAX_PREFIX_LEN+UCFG_MAX_PARAM_NAME_LEN+3];

	if ( sf_does_prefixed_param_exists( param_name ) == FALSE ) {
		return ( ucfg_is_string_in_param_values_list_ext(
						param_name,
						a_string,
						b_with_pattern ) );
	}

	sprintf( prefixed_param, "%.*s.%.*s",
		 UCFG_CLT_MAX_PREFIX_LEN,
		 s_prefix,
		 UCFG_MAX_PARAM_NAME_LEN,
		 param_name);

	return ( ucfg_is_string_in_param_values_list_ext(
							prefixed_param,
							a_string,
							b_with_pattern ) );

} /* End of ucfg_clt_is_string_in_param_values_list_ext() */






int ucfg_clt_is_string_in_param_values_list_pattern(char *param_name,
						char *a_string)
/*
 * Return 1 if a_string is in the list of values for param param_name
 * else return 0
 */
{
 	char prefixed_param[UCFG_CLT_MAX_PREFIX_LEN+UCFG_MAX_PARAM_NAME_LEN+3];

	if ( sf_does_prefixed_param_exists( param_name ) == FALSE ) {
		return ( ucfg_is_string_in_param_values_list_pattern(
						param_name,
						a_string ) );
	}

	sprintf( prefixed_param, "%.*s.%.*s",
		 UCFG_CLT_MAX_PREFIX_LEN,
		 s_prefix,
		 UCFG_MAX_PARAM_NAME_LEN,
		 param_name);

	return ( ucfg_is_string_in_param_values_list_pattern(
							prefixed_param,
							a_string ) );

} /* End of ucfg_clt_is_string_in_param_values_list_pattern() */





int ucfg_clt_is_string_in_param_values_list(char *param_name, char *a_string)
/*
 * Return 1 if a_string is in the list of values for param param_name
 * else return 0
 */
{
 	char prefixed_param[UCFG_CLT_MAX_PREFIX_LEN+UCFG_MAX_PARAM_NAME_LEN+3];

	if ( sf_does_prefixed_param_exists( param_name ) == FALSE ) {
		return ( ucfg_is_string_in_param_values_list(
						param_name,
						a_string ) );
	}

	sprintf( prefixed_param, "%.*s.%.*s",
		 UCFG_CLT_MAX_PREFIX_LEN,
		 s_prefix,
		 UCFG_MAX_PARAM_NAME_LEN,
		 param_name);

	return ( ucfg_is_string_in_param_values_list(
						prefixed_param,
						a_string ) );

} /* end of ucfg_clt_is_string_in_param_values_list() */




char *ucfg_clt_get_param_string_first( char *param_name )
/*
 * Returns the FIRST string value associated to the parameter or NULL if not
 * found.
 */
{
	char *p_value = NULL;
 	char prefixed_param[UCFG_CLT_MAX_PREFIX_LEN+UCFG_MAX_PARAM_NAME_LEN+3];

	if ( param_name == NULL ) {
		return ( NULL );
	}
 	
	sprintf( prefixed_param, "%.*s.%.*s",
		 UCFG_CLT_MAX_PREFIX_LEN,
		 s_prefix,
		 UCFG_MAX_PARAM_NAME_LEN,
		 param_name);

	p_value = ucfg_get_param_string_first( prefixed_param );
	if ( p_value == NULL ) {
		p_value = ucfg_get_param_string_first( param_name );
	}

	return ( p_value );
} /* end of ucfg_clt_get_param_string_first() */




char *ucfg_clt_get_param_string_next( char *param_name )
/*
 * Returns the NEXT string value associated to the parameter or NULL if not
 * found or if list of values is exhausted.
 */
{
 	char prefixed_param[UCFG_CLT_MAX_PREFIX_LEN+UCFG_MAX_PARAM_NAME_LEN+3];

	if ( param_name == NULL ) {
		return ( NULL );
	}

	if ( sf_does_prefixed_param_exists( param_name ) == FALSE ) {
		return ( ucfg_get_param_string_next( param_name ) );
	}
 	
	sprintf( prefixed_param, "%.*s.%.*s",
		 UCFG_CLT_MAX_PREFIX_LEN,
		 s_prefix,
		 UCFG_MAX_PARAM_NAME_LEN,
		 param_name);

	return ( ucfg_get_param_string_next( prefixed_param ) );
} /* end of ucfg_clt_get_param_string_next() */




int ucfg_clt_display_all_params( FILE *fi_out )
{
 return ( ucfg_display_all_params( fi_out ) );
}



void ucfg_clt_set_prefix( char *prefix, int len_prefix )
{
	int max_prefix_len;
 
	if ( ( len_prefix == 0 ) || ( prefix == NULL ) ) {
		return;
	}

	max_prefix_len = ( len_prefix > UCFG_CLT_MAX_PREFIX_LEN ) ?
			UCFG_CLT_MAX_PREFIX_LEN : len_prefix;

	memset( s_prefix, '\0', sizeof( s_prefix ) );
	strncpy( s_prefix, prefix, max_prefix_len );
}



char *ucfg_clt_get_prefix( void )
{
	return ( s_prefix );
}

/* -------------------------------------------- end of ucfg.c ----------- */

