/* linklist.c
 * ----------
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
 * $Source: /ext_hd/cvs/misc_fn/linklist.c,v $
 * $Header: /ext_hd/cvs/misc_fn/linklist.c,v 1.8 2004/10/15 21:13:43 jf Exp $
 *
 *
 * Single linked list (or queue) of anything (blocks of n bytes, type void *).
 * 
 * When AS_OPEN_SERVER_MODULE is defined, conditional compilation is used
 * to produce a module which will be linked to a "generic" Open Server
 * 10.0 set of modules. The name of the ".o" should then be "oslklist.o"
 *
 * WARNING: For use in a Open Server, please use the API functions of
 *	    "oslklstq.c". They provide the same functionalities WITH
 *	    mutex protection and errors logging.
 *
 *
 * NOTES: See the important "NOTES" at the top of "linklist.h"
 *	  They describe the behaviour expected from the caller for
 *	  memory allocation/free.
 *
 *
 * ----------------------------------------------------------------------
 * Versions:
 * 1994aug04,jft: first implementation.
 * 1994oct5-7,jft: Upgraded to be used through "oslklstq.c":
 * 1995nov15,jft: . lkls_is_string_in_list_ext() corrected prefix match logic
 * 1997feb18,deb: . Added conditional include for Windows NT port
 * 2002aug26,deb?: . sf_remove_node_from_list(): now update p_linklist->p_current_iter 
 */

#ifdef WIN32
#include "ntunixfn.h"
#endif

#ifdef AS_OPEN_SERVER_MODULE
#include "os10stdf.h"   /* standard #includes and #defines for OS 10.0  */
#include "osmemstr.h"	/* declares custom mem alloc/free/strdup() funct. */
#else
#include <stdio.h>
#include <stdlib.h>     /* mem functions prototypes */
#endif

#include "linklist.h"


/* -------------------------------------------- Private Defines:  */

#ifndef AS_OPEN_SERVER_MODULE
#define VERSION_ID	(__FILE__ ": " __DATE__ " - " __TIME__)
#endif

#define LKLS_SIGNATURE	0xD1B2FD5E	/* not like a HP-UX malloc() address */


/* -------------------------------------------- Private structures: */

typedef struct lkls_simple_link_list_elem {

	struct lkls_simple_link_list_elem	*p_next;

	char					*p_datum;

 } LINKLIST_ELEM_T;

typedef struct lkls_simple_link_list_holder {

	long					 signature,
						 nb_elem;

	struct lkls_simple_link_list_elem	*p_first,
						*p_last,
						*p_current_iter;

 } LINKLIST_HOLDER_T;






/* -------------------------------------------- Private functions: */


static int sf_check_linklist_signature(LINKLIST_HOLDER_T *p_linklist)
{
 if (p_linklist == NULL) {
	return (LKLS_ERR_NULL_LIST_PTR);
 }
  if (p_linklist->signature != LKLS_SIGNATURE) {
	return (LKLS_ERR_BAD_SIGNATURE);
 }
 return (0);	/* OK */

} /* end of sf_check_linklist_signature() */





int sf_add_elem_head_or_tail(void *linked_list, void *datum, int at_head)
/*
 * Return 0 (LKLS_OK) if element could be added to the list
 * or LKLS_ERR_OUT_OF_MEM if out of memory
 * or LKLS_ERR_NULL_LIST_PTR if linked_list == NULL 
 * or LKLS_ERR_BAD_SIGNATURE if linked_list is not a good handle
 * (detection is NOT guaranteed)
 * or LKLS_ERR_NULL_DATUM_PTR if datum == NULL.
 *
 * WARNING: The datum is NOT copied.  Only its address is.
 *
 *	    The calling module is responsible of memory alloc/free of datum.
 */
{
 int			 rc;
 LINKLIST_HOLDER_T	*p_linklist;
 LINKLIST_ELEM_T	*p_new_elem;

 if (datum == NULL) {
	return (LKLS_ERR_NULL_DATUM_PTR);
 }

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (rc);
 }

#ifdef AS_OPEN_SERVER_MODULE
 p_new_elem =  (LINKLIST_ELEM_T  *)
 		    osmem_alloc(sizeof(LINKLIST_ELEM_T));
#else
 p_new_elem = (LINKLIST_ELEM_T  *)
		   calloc(1, sizeof(LINKLIST_ELEM_T));
#endif

 if ((p_new_elem) == NULL) {
	return (LKLS_ERR_OUT_OF_MEM);
 }
 p_new_elem->p_datum = datum;
 p_new_elem->p_next  = NULL;

 if (p_linklist->p_first == NULL) {		/* list is new - empty */
	p_linklist->p_first	   = p_new_elem;
	p_linklist->p_last	   = p_new_elem;
	p_linklist->p_current_iter = NULL;
	p_linklist->nb_elem	   = 1;
	return (0);
 }
 if (at_head) {					/* Insert at head: */
	if (p_linklist->p_first == NULL) {
		return (-99);			/* LOGIC ERROR */
	}
	p_new_elem->p_next  = p_linklist->p_first; /* forward chaining */
	p_linklist->p_first = p_new_elem;	   /* remember first one */

 } else {					/* Insert at tail: */
	if (p_linklist->p_last == NULL) {
		return (-99);			/* LOGIC ERROR */
	}
	p_linklist->p_last->p_next = p_new_elem;  /* forward chaining */
	p_linklist->p_last	   = p_new_elem;  /* remember last one */
 }
 p_linklist->nb_elem++;

 return (0);

} /* end of sf_add_elem_head_or_tail() */





int sf_remove_node_from_list(LINKLIST_HOLDER_T *p_linklist,
			LINKLIST_ELEM_T *p_node, LINKLIST_ELEM_T *p_prev)
/* 
 * We are handling a single linked list here...
 *
 * So, to be able to remove a node and re-establish the forward chaining
 * we need the calling function to give us a pointer to the previous
 * node in the chain (p_prev).
 *
 * The memory area allocated to the node is freed after all links
 * manipulations are completed.
 */
{
 int	rc;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (rc);
 }

 if (p_node == NULL) {		/* possible, don't complain */
	return (0);
 }
 if (p_node == p_linklist->p_first) {	/* Special case: removing first */
	if (p_node->p_next == NULL) {	/* special: first and last */
		p_linklist->p_first = NULL;
		p_linklist->p_last  = NULL;
		p_linklist->p_current_iter = NULL;
	} else {
		p_linklist->p_first = p_node->p_next;
		p_linklist->p_current_iter = NULL;
	}
 } else {				/* not first, then there is p_prev */
	if (p_prev != NULL) {
		p_prev->p_next = p_node->p_next;	/* re-link forward */
		p_linklist->p_current_iter = p_prev; /* rewind the current_iter for lkls_get_datum_in_list_next call */
	}
	else{
		p_linklist->p_current_iter = NULL;
	}
 }
 if (p_node == p_linklist->p_last) {	/* Special: removing last */
	p_linklist->p_last  = p_prev;	/* can be previous or NULL */
 }

#ifdef AS_OPEN_SERVER_MODULE
 osmem_free(p_node);
#else
 free(p_node);
#endif

 return (LKLS_FOUND);

} /* end of sf_remove_node_from_list() */



/* ----------------------------------------------- Public functions: */



char *lkls_version_id()
{
	return (VERSION_ID);
}




void *lkls_new_linked_list()
/*
 * Returns new handle to a linked list or NULL if out of memory.
 *
 */
{
 LINKLIST_HOLDER_T	*p_linklist;


#ifdef AS_OPEN_SERVER_MODULE
 p_linklist = (LINKLIST_HOLDER_T *)
 		    osmem_alloc(sizeof(LINKLIST_HOLDER_T));
#else
 p_linklist = (LINKLIST_HOLDER_T *)
 		    calloc(1, sizeof(LINKLIST_HOLDER_T) );
#endif

 if (p_linklist == NULL) {
	return ( (void *) NULL);
 }
 p_linklist->signature = LKLS_SIGNATURE;

 return ( (void *) p_linklist);

} /* end of lkls_new_linked_list() */





int lkls_add_elem(void  *linked_list,  void  *datum)
/*
 * Append at tail of list.
 *
 * Return 0 (LKLS_OK) if element could be added to the list
 * or LKLS_ERR_OUT_OF_MEM if out of memory
 * or LKLS_ERR_NULL_LIST_PTR if linked_list == NULL 
 * or LKLS_ERR_BAD_SIGNATURE if linked_list is not a good handle
 * (detection is NOT guaranteed)
 * or LKLS_ERR_NULL_DATUM_PTR if datum == NULL.
 *
 * WARNING: The datum is NOT copied.  Only its address is.
 *
 *	    The calling module is responsible of memory alloc/free of datum.
 */
{
 return (sf_add_elem_head_or_tail(linked_list, datum, 0) );   /* at tail */

} /* end of lkls_add_elem() */





int lkls_insert_elem_at_head(void  *linked_list,  void  *datum)
/*
 * Insert at head of list.
 *
 * See comments for lkls_add_elem()
 *
 */
{
 return (sf_add_elem_head_or_tail(linked_list, datum, 1) );   /* at head */

} /* end of lkls_add_elem() */





int lkls_append_elem_at_tail(void  *linked_list,  void  *datum)
/*
 * Append at tail of list.
 *
 * See comments for lkls_add_elem()
 *
 */
{
 return (sf_add_elem_head_or_tail(linked_list, datum, 0) );   /* at tail */

} /* end of lkls_add_elem() */





int lkls_destroy_linked_list(void *linked_list)
/*
 * Frees every node in the list, then frees the list holder structure.
 *
 * Return 0 if linked_list was properly freed
 * or LKLS_ERR_NULL_LIST_PTR if linked_list == NULL 
 * or LKLS_ERR_BAD_SIGNATURE if linked_list is not a good handle
 * (detection is NOT guaranteed)
 *
 * WARNING:  1) Unpredictable (bad) results will happen if an old (freed)
 *		handle to a linked list is used after being destroyed.
 *
 *	     2) The datum in each node is NOT freed!
 *
 *		The calling module is responsible of memory alloc/free of datum.
 */
{
 int			 rc;
 LINKLIST_HOLDER_T	*p_linklist;
 LINKLIST_ELEM_T	*p_next_elem,
			*p_current;

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (rc);
 }

 p_linklist->signature = -1L;		/* make signature non-valid */

 for (p_current = p_linklist->p_first;
      p_current != NULL;
      p_current = p_next_elem) {
	p_next_elem = p_current->p_next;
#ifdef AS_OPEN_SERVER_MODULE
	osmem_free(p_current);
#else
	free(p_current);
#endif
 }
#ifdef AS_OPEN_SERVER_MODULE
 osmem_free(p_linklist);
#else
 free(p_linklist);
#endif

 return (0);

} /* end of lkls_destroy_linked_list() */






int lkls_remove_element_from_list(void *linked_list, void *p_datum)
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
{
 int			 rc,
			 is_found = 0;
 LINKLIST_HOLDER_T	*p_linklist;
 LINKLIST_ELEM_T	*p_curr,
			*p_prev;

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (rc);
 }

 if (p_datum == NULL) {
	return (LKLS_ERR_NULL_DATUM_PTR);
 }
 if (p_linklist->p_first == NULL) {
	return (LKLS_OK);
 }


 for (p_prev = NULL, p_curr = p_linklist->p_first;
      p_curr != NULL; p_curr = p_curr->p_next) {

 	if (p_curr->p_datum == p_datum) {
 		is_found = 1;
 		break;
 	}
	p_prev = p_curr;	/* remember previous node in list */
 }
 if (!is_found) {
	return (LKLS_OK);
 }

 sf_remove_node_from_list(p_linklist, p_curr, p_prev);

 return (LKLS_FOUND);

} /* end of lkls_remove_element_from_list() */







void *lkls_pop_first_element_from_list(void *linked_list)
/*
 * Return the p_datum of the first node of the list.
 * Removes the first node of the list.
 *
 * Return:
 *	p_datum		p_datum of the first element of the list OR
 *	NULL		if list is empty OR has bad signature.
 */
{
 int			 rc;
 LINKLIST_HOLDER_T	*p_linklist;
 void			*p_data;

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (NULL);
 }

 if (p_linklist->p_first == NULL) {
	return (NULL);
 }
 p_data = p_linklist->p_first->p_datum;

 sf_remove_node_from_list(p_linklist, p_linklist->p_first, NULL);

 return (p_data);

} /* end of lkls_pop_first_element_from_list() */







int lkls_copy_data_to_array(void *linked_list, char *ptr_array[], int max_sz)
/*
 * Copy each datum pointer of the list to one element of ptr_array[].
 *
 * Return number of elements or LKLS_ERR_ARRAY_OVERFLOW if ctr >= max_sz
 * or LKLS_ERR_NULL_LIST_PTR if linked_list == NULL 
 * or LKLS_ERR_BAD_SIGNATURE if linked_list is not a good handle
 * (detection is NOT guaranteed).
 */
{
 int			 ctr = 0,
 			 rc;
 LINKLIST_HOLDER_T	*p_linklist;
 LINKLIST_ELEM_T	*p_curr;

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (rc);
 }

 if (p_linklist->p_first == NULL) {
	return (0);			/* empty list */
 }
 for(p_curr = p_linklist->p_first; p_curr != NULL;
     p_curr = p_curr->p_next, ctr++) {
     	if (ctr >= max_sz) {
     		return (LKLS_ERR_ARRAY_OVERFLOW);
     	}
	ptr_array[ctr] = p_curr->p_datum;
 }
 if (ctr < max_sz ) {
	ptr_array[ctr] = NULL;
 }
 return (ctr);

} /* end of lkls_copy_data_to_array() */






int lkls_is_string_in_list(void *linked_list, char *a_string)
/*
 * Return 1 if one of the datum in the list is equal to a_string
 * else return 0
 */
{
	return (lkls_is_string_in_list_ext(linked_list, a_string, 0) );
}



int lkls_is_string_in_list_ext(void *linked_list, char *a_string,
				int  use_pattern_matching)
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
{
 int			 rc,
			 len = 0,
			 b_with_prefix_match = 0;
 char			*p_str;
 LINKLIST_HOLDER_T	*p_linklist;
 LINKLIST_ELEM_T	*p_curr;

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (0);  /* we cannot tell about error, so we say "not found" */
 }

 if (p_linklist->p_first == NULL) {
	return (0);					/* empty list */
 }

 for(p_curr = p_linklist->p_first; p_curr != NULL;
     p_curr = p_curr->p_next) {
	if (p_curr->p_datum == NULL) {	/* bypass any weird pointers */
		continue;
	}
	p_str = (char *) p_curr->p_datum;

	/* Copy original value of param use_pattern_matching...
	 * It will be modified later in the loop...
	 */
	b_with_prefix_match = use_pattern_matching;

	if (use_pattern_matching) {
		len = strlen(p_str);
		if (len > 0  &&  p_str[len - 1] == '*') {
			len--;
		} else {
			b_with_prefix_match = 0;	/* No '*' !!! */
		}
	}
	if (!b_with_prefix_match) {
		/* look for a_string as is:
		 */
		if (!strcmp(p_str, a_string) ) {
			return (1);			/* Found */
		}
		continue;
	}
	/* b_with_prefix_match == TRUE
	 * Only compare first 'len' bytes:
	 */
	if (!strncmp(p_str, a_string, len) ) {
		return (1);			/* Found */
	}
 }
 return (0);						/* NOT Found */

} /* end of lkls_is_string_in_list_ext() */





void *lkls_find_elem_using_function(void *linked_list, void *p_datum,
			     int (* funct_ptr)(void *p_dat, void *p_curr) )
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
{
 int			 rc;
 LINKLIST_HOLDER_T	*p_linklist;
 LINKLIST_ELEM_T	*p_curr;

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (NULL);
 }

 if (p_datum == NULL  ||  p_linklist->p_first == NULL) {
	return (NULL);
 }
 for(p_curr = p_linklist->p_first; p_curr != NULL; p_curr = p_curr->p_next) {
 	if (p_curr->p_datum == NULL) {
 		continue;
 	}
	rc = (* funct_ptr)(p_datum, p_curr->p_datum);
	if (rc < 0) {
		return (NULL);		/* we're told: no need to continue */
	} else if (rc > 0) {
		return (p_curr->p_datum);	/* found */
	}
 } /* end for */

 return (NULL);

} /* end of lkls_find_elem_using_function() */






void *lkls_get_datum_in_list_first(void *linked_list)
/*
 * Return ptr to datum of the FIRST node in the list or NULL if list is 
 * empty.
 *
 * 1994oct07,jft: Even though
 *
 *			 return (p_linklist->p_first->p_datum);
 *
 *		  is more robust than the previous version, it is still
 *		  possible that p_linklist->p_first could be changed by
 *		  another thread... Any Open Server caller should use a
 *		  some mutex protocol to ensure reliability.
 */
{
 int			 rc;
 LINKLIST_HOLDER_T	*p_linklist;

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (NULL);
 }

 if (p_linklist->p_first == NULL) {
	return (NULL);
 }
 p_linklist->p_current_iter = p_linklist->p_first;

 return (p_linklist->p_first->p_datum);	    /* More robust in Open Server */

 /* Old version (before 1994oct07), less robust for Open Server: */
 /* return (p_linklist->p_current_iter->p_datum); */

} /* end of lkls_get_datum_in_list_first() */





void *lkls_get_datum_in_list_next(void *linked_list)
/*
 * WARNING: Not to be used in multi-threaded environment (i.e.Open Server):
 *	    as this function uses a static variable it is NOT re-entrant
 *	    (p_linklist->p_current_iter).
 *
 * Return ptr to datum of the NEXT node in the list or NULL if at the
 * end of the list.
 */
{
 int			 rc;
 LINKLIST_ELEM_T	*p_elem;
 LINKLIST_HOLDER_T	*p_linklist;

 p_linklist = (LINKLIST_HOLDER_T  *) linked_list;

 rc = sf_check_linklist_signature(p_linklist);
 if (rc < 0) {
	return (NULL);
 }

 if (p_linklist->p_current_iter == NULL) {	/* _first not called yet! */
	return (lkls_get_datum_in_list_first(linked_list) );
 }
 if (p_linklist->p_current_iter == p_linklist->p_last) {
	return (NULL);
 }
 p_linklist->p_current_iter = p_linklist->p_current_iter->p_next;
 
 return (p_linklist->p_current_iter->p_datum);

} /* end of lkls_get_datum_in_list_next() */


/* -------------------------------------------- end of linklist.c -------- */
