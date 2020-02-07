/* intstrtb.h
 * ----------
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
 * $Source: /ext_hd/cvs/include/intstrtb.h,v $
 * $Header: /ext_hd/cvs/include/intstrtb.h,v 1.4 2004/10/15 21:18:56 jf Exp $
 *
 *
 * General INT to STRing TaBle functions.
 *
 * 
 *  CONTENT:
 *	Defines, types (struct) and functions prototypes for all callers
 *	of the intstrtb.c functions.
 *
 *
 *  PURPOSE:
 *	Provide low-level functions that convert numbers to string constants
 *	and string constants to numbers according to a table.
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
|*	int istb_num_for_string_first_byte(
|*				struct int_str_tab_entry  *p_int_str_tab,
|*				int sizeof_table,
|*				char *s);
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
 *  AUTHOR: J.F.Touchette, 29juin1994
 *
 * ----------------------------------------------------------------------
 * 1994sept23,jft: added #ifndef _INTSTRTB_H_ ...#endif
 * 1996jan18,jft: added istb_num_for_string_first_byte()
 */

#ifndef _INTSTRTB_H_
 #define _INTSTRTB_H_

#ifndef INT_MIN
#include <limits.h>
#endif

#define _INTSTRTB_H_

#define ISTR_STRING_TABLE_IS_OK		 0
#define ISTR_ERR_PTR_TO_TABLE_IS_NULL	-1
#define ISTR_ERR_TABLE_EMPTY		-2
#define ISTR_ERR_NULL_STRING_FOUND	-5
#define ISTR_ERR_NUMBERS_NOT_SORTED	-6

#define ISTR_ERR_STRING_NOT_FOUND	(INT_MIN)	/* from limits.h */

#define ISTR_ERR_NUM_NOT_FOUND		NULL

#define ISTR_NUM_DEFAULT_VAL		(INT_MAX)


 struct int_str_tab_entry {

	int	num;

	char	*p_string;

 };

char *istb_string_for_num(struct int_str_tab_entry  *p_int_str_tab,
			  int sizeof_table,	/* nb of bytes ! */
			  int n);

int istb_num_for_string(struct int_str_tab_entry  *p_int_str_tab,
			int sizeof_table,	/* nb of bytes ! */
			char *s);

int istb_num_for_string_first_byte(struct int_str_tab_entry  *p_int_str_tab,
				   int sizeof_table,	/* nb of bytes ! */
				   char *s);

int istb_check_int_strings_table(struct int_str_tab_entry  *p_int_str_tab,
			  	 int  sizeof_table);	/* nb of bytes ! */


#endif /* _INTSTRTB_H_ */

/* --------------------------------------- end of intstrtb.h ------------- */

