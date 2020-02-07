/* linklist.h
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
 * $Source: /ext_hd/cvs/include/linklist.h,v $
 * $Header: /ext_hd/cvs/include/linklist.h,v 1.5 2004/10/13 18:23:36 blanchjj Exp $
 *
 *
 * Public functions prefix: lkls_
 *
 *
 * Simple (forward) linked list of anything (blocks of n bytes, type void *).
 *
 * WARNING: For use in a Open Server, please use the API functions of
 *	    "oslklstq.c". They provide the same functionalities WITH
 *	    mutex protection and errors logging.
 *
 *
 * NOTES: The calling module is responsible of memory alloc/free of the
 *	  datum whose address is kept in an element of the list. (The datum
 *	  is NOT copied in the element, its _address_ is).
 *
 *	  There is no function to remove one element from a list.
 *
 *	  Destruction applies to all the elements and the list handle itself.
 *
 *	  Before destroying a list, the calling module must free the memory
 *	  associated to the datum of every element of the list.
 *
 *	  Each new element is added to the end of the list (FIFO style).
 *
 *
 * 
 * --------------------------------------------------------------------------
 * Versions:
 * 1994aug04,jft: first implementation.
 * 1994oct5-7,jft: upgraded to be used through "oslklstq.c"
 * 2001apr13,deb: Changed to suppress warning from compiler:
 *                . char *lkls_version_id()
 *                  to  char *lkls_version_id( void )
 *                . void *lkls_new_linked_list()
 *                  to  void *lkls_new_linked_list( void )
 */

#define LKLS_FOUND			 1
#define LKLS_OK				 0
#define LKLS_ERR_OUT_OF_MEM		-1
#define LKLS_ERR_NULL_LIST_PTR		-2
#define LKLS_ERR_BAD_SIGNATURE		-3
#define LKLS_ERR_NULL_DATUM_PTR		-4
#define LKLS_ERR_ARRAY_OVERFLOW		-8

/* ----------------------------------------------- Public functions: */


char *lkls_version_id( void );



void *lkls_new_linked_list( void );
/*
 * Returns new handle to a linked list or NULL if out of memory.
 *
 */


int lkls_add_elem(void  *linked_list,  void  *datum);
/*
 * Append at tail of list.
 *
 * Return 0 if element could be added to the list or -1 if out of memory
 * or -2 if linked_list == NULL or -3 if linked_list is not a good handle
 * (detection is NOT guaranteed) or -4 if datum == NULL.
 *
 * WARNING: The datum is NOT copied.  Only its address is.
 *
 *	    The calling module is responsible of memory alloc/free of datum.
 */



int lkls_insert_elem_at_head(void  *linked_list,  void  *datum);
/*
 * Insert at head of list.
 *
 * See comments for lkls_add_elem()
 *
 */



int lkls_append_elem_at_tail(void  *linked_list,  void  *datum);
/*
 * Append at tail of list.
 *
 * See comments for lkls_add_elem()
 *
 */



int lkls_destroy_linked_list(void *linked_list);
/*
 * Frees every node in the list, then frees the list holder structure.
 *
 * Return 0 if linked_list was properly freed
 * or -2 if linked_list == NULL or -3 if linked_list is not a good handle
 * (detection is NOT guaranteed).
 *
 * WARNING:  1) Unpredictable (bad) results will happen if an old (freed)
 *		handle to a linked list is used after being destroyed.
 *
 *	     2) The datum in each node is NOT freed!
 *
 *		The calling module is responsible of memory alloc/free of datum.
 */


int lkls_remove_element_from_list(void *linked_list, void *p_datum);
/*
 * Scan the list to find a node p_curr such as:
 *
 *	 p_curr->p_datum == p_datum
 *
 * If one is found, remove the node from the list and return LKLS_FOUND.
 *
 * If none is found, return 0  (LKLS_OK).
 *
 * If list is bad (bad signature) return a negative error code.
 *
 */



void *lkls_pop_first_element_from_list(void *linked_list);
/*
 * Return the p_datum of the first node of the list.
 * Removes the first node of the list.
 *
 * Return:
 *	p_datum		p_datum of the first element of the list OR
 *	NULL		if list is empty OR has bad signature.
 */




int lkls_copy_data_to_array(void *linked_list, char *ptr_array[], int max_sz);
/*
 * Copy each datum pointer of the list to one element of ptr_array[].
 *
 * Return number of elements or LKLS_ERR_ARRAY_OVERFLOW if ctr >= max_sz
 * or LKLS_ERR_NULL_LIST_PTR if linked_list == NULL 
 * or LKLS_ERR_BAD_SIGNATURE if linked_list is not a good handle
 * (detection is NOT guaranteed).
 */



int lkls_is_string_in_list(void *linked_list, char *a_string);
/*
 * Return 1 if one of the datum in the list is equal to a_string
 * else return 0
 */


int lkls_is_string_in_list_ext(void *linked_list, char *a_string,
				int  use_pattern_matching);
/*
 * Called by:	ucfg.c, ...
 *		lkls_is_string_in_list()
 *
 * Return 1 if one of the datum in the list is equal to a_string
 * else return 0
 *
 * If use_pattern_matching == TRUE
 * AND a value in the list ends with a '*', say a_string matches if (len - 1)
 * first bytes are the same.
 */



void *lkls_find_elem_using_function(void *linked_list, void *p_datum,
			     int (* funct_ptr)(void *p_dat, void *p_curr) );
/*
 * Use (*funct_ptr)() to find an element in the list.
 *
 * The function (*funct_ptr)() is called once for each node in the list
 * until it returns a non-zero value.
 *
 * The parameters types of (*funct_ptr)() should be like:
 *
 *		int find_element(void *p_datum, void *p_curr)
 *
 * Returns:
 *	NULL		if end-of-list was reached
 *			OR if (*funct_ptr)() returned a value < 0, OR
 *	void *p_datum	a pointer to the datum of the current node 
 *			(which triggered (*funct_ptr)() to return n > 0).
 */



void *lkls_get_datum_in_list_first(void *linked_list);
/*
 * Return ptr to datum of the FIRST node in the list or NULL if list is 
 * empty.
 */


void *lkls_get_datum_in_list_next(void *linked_list);
/*
 * WARNING: Not to be used in multi-threaded environment (i.e.Open Server):
 *	    as this function uses a static variable it is NOT re-entrant.
 *
 * Return ptr to datum of the NEXT node in the list or NULL if at the
 * end of the list.
 */


/* -------------------------------------------- end of linklist.h -------- */
