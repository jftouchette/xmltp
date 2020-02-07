/*  NAME: intstrtb.c
 *
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
 * $Source: /ext_hd/cvs/log_fn/intstrtb.c,v $
 * $Header: /ext_hd/cvs/log_fn/intstrtb.c,v 1.4 2004/10/15 20:55:17 jf Exp $
 *
 *
 *      General INT to STRing TaBle functions.
 *
 *
 *  FUNCTIONS PREFIX:  istb_
 * 
 *
 *
 *  PURPOSE:
 *	Provide low-level functions that convert numbers to string constants
 *	and string constants to numbers according to a table.
 *
 *
 *
 *  CALLING MODULES:	apl_log1.c
 *	(this list might be incomplete)
 *
 *
 *  FUNCTIONS:
 *
 *	char *istb_string_for_num(struct int_str_tab_entry  *p_int_str_tab,
 *				  int sizeof_table,
 *				  int n);
 *	  Parameters:
 *		p_int_str_tab	is the address of the conversion table
 *		sizeof_table	is the size (in bytes) of the table
 *		n		the number to match to a string
 *
 *	  Return:
 *		char *		the matching string or NULL
 *
 *
 *	int istb_num_for_string(struct int_str_tab_entry  *p_int_str_tab,
 *				int sizeof_table,
 *				char *s);
 *	  Parameters:
 *		p_int_str_tab	is the address of the conversion table
 *		sizeof_table	is the size (in bytes) of the table
 *		s		the string to match to a number
 *
 *	  Return:
 *		int		the number associated to "s" or
 *				ISTR_ERR_STRING_NOT_FOUND
 *
 *
 *	int istb_check_int_strings_table(
 *				struct int_str_tab_entry  *p_int_str_tab,
 *			  	int  sizeof_table);
 *	  Parameters:
 *		p_int_str_tab	is the address of the conversion table
 *		sizeof_table	is the size (in bytes) of the table
 *		s		the string to match to a number
 *
 *	  Return:
 *		int		ISTR_STRING_TABLE_IS_OK
 *				or ISTR_ERR_NUMBERS_NOT_SORTED
 *				or ISTR_ERR_NULL_STRING_FOUND
 *
 *	  NOTE: The numbers in a table should be sorted in ascending
 *		sequence if you want the bsearch() function to be used.
 *
 *		To enable the use of bsearch() in istb_string_for_num()
 *		you must call istb_check_int_strings_table() before
 *		(one time is enough).
 *
 *		The use of bsearch() is automatically disabled if:
 *		  - istb_string_for_num() is called with a new value for
 *		    p_int_str_tab (another table)
 *		  - istb_check_int_strings_table() has never been called
 *		  - istb_check_int_strings_table() returned an error code.
 *
 *
 *
 *  AUTHOR: J.F.Touchette, 29juin1994
 *
 * ----------------------------------------------------------------------
 * 1996jan18,jft: added istb_num_for_string_first_byte()
 * 1997feb18,deb: Added conditional include for Windows NT port
 */

#ifdef WIN32
#include "ntunixfn.h"
#endif

#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "intstrtb.h"

/* -------------------------------------- hidden variables: ---------- */

static struct int_str_tab_entry  *s_p_last_table_checked = NULL;


/* --------------------------------- public (visible) functions: ---------- */



char *istb_string_for_num(struct int_str_tab_entry  *p_int_str_tab,
			  int sizeof_table,	/* nb of bytes ! */
			  int n)
{
	int	nb_entries,
		i;

	if (p_int_str_tab == NULL) {
		return (NULL);
	}

	nb_entries = sizeof_table / (sizeof(struct int_str_tab_entry));


	/* bsearch() is not used for now - only sequential search:
	 */
 	for (i=0; i < nb_entries; i++, p_int_str_tab++) {
		if (p_int_str_tab->num == n) {
			return (p_int_str_tab->p_string);
		}
	}
	return NULL;

} /* end of istb_string_for_num() */





static int istb_num_for_string_ext(struct int_str_tab_entry  *p_int_str_tab,
				   int	 sizeof_table,	/* nb of bytes ! */
				   char *s,
				   int	 only_compare_first_byte)
{
	int	nb_entries,
		i;

	if (p_int_str_tab == NULL) {
		return (ISTR_ERR_STRING_NOT_FOUND);
	}

	nb_entries = sizeof_table / (sizeof(struct int_str_tab_entry));

	for (i=0; i < nb_entries; i++, p_int_str_tab++) {
		if (only_compare_first_byte) {
			if (p_int_str_tab->p_string[0] == s[0]) {
				return (p_int_str_tab->num);
			}
			continue;
		}
		if (!strcmp(p_int_str_tab->p_string, s)) {
			return (p_int_str_tab->num);
		}
	}
	return ISTR_ERR_STRING_NOT_FOUND;

} /* end of istb_num_for_string_ext() */




int istb_num_for_string(struct int_str_tab_entry  *p_int_str_tab,
			int sizeof_table,	/* nb of bytes ! */
			char *s)
{
	/* With only_compare_first_byte == FALSE (0)
	 */
	return (istb_num_for_string_ext(p_int_str_tab, sizeof_table, s, 0) );

} /* end of istb_num_for_string() */




int istb_num_for_string_first_byte(struct int_str_tab_entry  *p_int_str_tab,
				   int sizeof_table,	/* nb of bytes ! */
				   char *s)
{
	/* With only_compare_first_byte == TRUE (1)
	 */
	return (istb_num_for_string_ext(p_int_str_tab, sizeof_table, s, 1) );

} /* end of istb_num_for_string_first_byte() */





int istb_check_int_strings_table(struct int_str_tab_entry *p_int_str_tab,
			  	 int  sizeof_table)
{
	struct int_str_tab_entry  *p_work = NULL;

	int	nb_entries,
		i,
		prev_num = INT_MIN;	/* from limits.h */

	s_p_last_table_checked = NULL;	/* not NULL only if good */

	if (p_int_str_tab == NULL) {
		return (ISTR_ERR_STRING_NOT_FOUND);
	}

	nb_entries = sizeof_table / (sizeof(struct int_str_tab_entry));

	for (p_work=p_int_str_tab, i=0; i < nb_entries; i++, p_work++) {
		if (p_work->num  <  prev_num) {
			return (ISTR_ERR_NUMBERS_NOT_SORTED);
		}
		if (p_work->p_string == NULL) {
			return (ISTR_ERR_NULL_STRING_FOUND);
		}
	}
	s_p_last_table_checked = p_int_str_tab;

	return (ISTR_STRING_TABLE_IS_OK);

} /* end of istb_check_int_strings_table() */


/* --------------------------------------- end of intstrtb.c ------------- */


