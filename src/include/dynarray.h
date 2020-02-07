/* dynarray.h
 * ----------
 *
 *  Copyright (c) 1997-2001 Jean-Francois Touchette et Jocelyn Blanchet.
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
 * $Source: /ext_hd/cvs/include/dynarray.h,v $
 * $Header: /ext_hd/cvs/include/dynarray.h,v 1.6 2004/10/13 18:23:35 blanchjj Exp $
 *
 *
 * Functions prefix:	dynarray_
 * 
 * Dynamic Sized Array Objects
 * ---------------------------
 * 
 * The objects of this module are dynamically sized arrays that can grow 
 * in size up to a pre-defined maximum number of elements.
 *
 * NOTE: The functions of this module are used by "ct10dll.c"
 *
 * -------------------------------------------------------------------------
 * Versions:
 * 1997jan30,jft: struct and specification of functions
 * 1997jun05,jbl: Added: dynarray_qsort and dynarray_bsearch functions.
 * 2000jun30,deb: Fixed for AIX compiler that doesn't like assignment between
 *                (void *) and (const void *)
 * 2001apr13,deb: .  changed dynarray_get_description_of_last_error();
 *                   to dynarray_get_description_of_last_error( void );
 *                .  changed dynarray_get_size_of_dynarray_struct();
 *                   to dynarray_get_size_of_dynarray_struct( void );
 */


/* -------------------------------------------------- Public Defines: */

#define DYNARRAY_ERR_NOT_IMPLEMENTED		-55
#define DYNARRAY_ERR_MAX_ALLOC			-3
#define DYNARRAY_ERR_OUT_OF_MEMORY		-9
#define DYNARRAY_ERR_INTERNAL_LOGIC_ERROR	-10
#define DYNARRAY_ERR_WRONG_ELEM_SIZE		-15
#define DYNARRAY_ERR_NULL_ARG			-20
#define DYNARRAY_ERR_NULL_OBJ_PTR		-21
#define DYNARRAY_ERR_INVALID_SIGNATURE		-22



/* -------------------------------------------- Public Functions: */

char *dynarray_get_description_of_last_error( void );




/* Some callers might want to pass the value sizeof(DYN_ARRAY_OBJ_T) as
 * argument sizeof_element when they call dynarray_create_new_dynamic_array()
 * (because they want to create a dynarray of dynarray!)
 *
 * Return: 	sizeof( DYN_ARRAY_OBJ_T )
 */
int dynarray_get_size_of_dynarray_struct( void );




/* Steps:
 *	Validate args, if wrong, set s_last_error_description to point to 
 *	 diagnostic message string and return NULL.
 *
 *	Allocate a new DYN_ARRAY_OBJ_T struct with p_new = calloc(...)
 * 
 *	Init the fields of the struct:
 *		p_new->signature = DYN_ARRAY_OBJ_SIGNATURE;
 *		p_new->nb_allocated = 0;
 *		p_new->p_allocated_array = NULL;
 *		... (copy the args values in the fields of the struct) ...
 *
 *	Make array as big as initial_size, with call:
 *		sf_grow_dynarray(p_new, initial_size);
 *
 *	Return the pointer to the  new DYN_ARRAY_OBJ_T struct as a (void *).
 *
 * Validations:
 *	sizeof_element  	must be >= 4,
 *	max_allocatable		must be in [2..1000000] range
 *	growth_increment	must be in [1..1000] range
 *	initial_size		must be in [1..1000] range
 *
 *	WARNING: if (b_elems_are_dynarray is TRUE) then we must have:
 *			sizeof_element == sizeof(DYN_ARRAY_OBJ_T)
 *
 *		 Otherwise, set s_last_error_description to
 *		 DESCR_ERR_WRONG_ELEM_SIZE and return NULL.
 *
 * RETURN:
 *	NULL		Error, see dynarray_get_description_of_last_error()
 *	(void *)	success
 */
void *dynarray_create_new_dynamic_array(
			long sizeof_element,
			long max_allocatable,
			long growth_increment,
			long initial_size,
			int  b_elems_are_dynarray );




/* IMPORTANT WARNING to caller:
 *	If the elements stored in the array themselves contain pointer(s) to
 *	allocated memory, then the caller must take care of doing the free()
 *	of these allocations.
 *
 *	  NOTE: If the elements stored are also dynarray objects, then
 *		this function does a full cleanup using recursion.
 *
 * STEPS:
 *	Check signature of  p_dynarray_obj
 *	If b_elems_are_dynarray, iterate on each element and call
 *		dynarray_free(...)
 *
 *	WARNING: if (b_elems_are_dynarray is TRUE) then assert that:
 *			sizeof_element == sizeof(DYN_ARRAY_OBJ_T)
 *
 *	set ctr_used = 0
 *	set iterator_current_i = 0
 *
 * RETURN:
 *	0				OK, success
 *	DYNARRAY_ERR_NULL_OBJ_PTR	p_dynarray_obj is NULL
 *	DYNARRAY_ERR_INVALID_SIGNATURE	p_dynarray_obj has an invalid signature
 *  DYNARRAY_ERR_INTERNAL_LOGIC_ERROR the number of element stored in
 *								p_dynarray_obj does not agree with the number
 *								of element reported by ctr_used
 */
int dynarray_free( void *p_dynarray_obj );






/* STEPS:
 *	Check signature of  p_dynarray_obj
 *	If required, grow elements array in p_dynarray_obj.
 *	Copy element pointed by p_elem to end of p_allocated_array[],
 *	 at position [ctr_used * sizeof_element].
 *	increment ctr_used
 *
 * RETURN:
 *	0				OK, success
 *	DYNARRAY_ERR_MAX_ALLOC		max_allocatable maximum reached 
 *	DYNARRAY_ERR_OUT_OF_MEMORY	calloc() or realloc() failed
 *	DYNARRAY_ERR_NULL_ARG		p_elem is NULL
 *	DYNARRAY_ERR_NULL_OBJ_PTR	p_dynarray_obj is NULL
 *	DYNARRAY_ERR_INVALID_SIGNATURE	p_dynarray_obj has invalid signature
 */
int dynarray_add_new_element( void *p_dynarray_obj, void *p_elem );





/* RETURN:
 *	-1	error
 *	>= 0	number of elements currently stored (== ctr_used)
 */
long dynarray_get_nb_elem_stored( void *p_dynarray_obj );





/* STEPS:
 *	Check signature of  p_dynarray_obj
 *	if at end of array (iterator_current_i >= ctr_used) then
 *		return NULL
 *	p_elem = &p_allocated_array[iterator_current_i * sizeof_element]
 *	increment iterator_current_i
 *	return p_elem
 *
 * RETURN:
 *	NULL	  at end of array OR bad signature OR NULL  p_dynarray_obj
 *	(void *)  OK, success
 */
void *dynarray_get_next_elem( void *p_dynarray_obj );





/* STEPS:
 *	Check signature of  p_dynarray_obj
 *	set iterator_current_i = 0
 *	return ( dynarray_get_next_elem(p_dynarray_obj) )
 *
 * RETURN:
 *	NULL	  at end of array OR bad signature OR NULL  p_dynarray_obj
 *	(void *)  OK, success
 */
void *dynarray_get_first_elem( void *p_dynarray_obj );





/* STEPS:
 *	Check signature of  p_dynarray_obj
 *  validate i
 *	return ( p_dynarray_obj->p_allocated_array +
 *			 i * p_dynarray->sizeof_element )
 *
 * NOTE:	  The array is considered 0 based, as in standard "C"
 * RETURN:
 *	NULL	  end or past end of array OR bad signature OR NULL  p_dynarray_obj
 *	(void *)  OK, success
 */
void *dynarray_get_nth_elem( void *p_dynarray_obj, long i );




/* Return the size of the elements stored in the dynarray
 *
 * RETURN:
 *  0								OK
 *	DYNARRAY_ERR_NULL_OBJ_PTR		p_dynarray_obj is NULL
 *	DYNARRAY_ERR_INVALID_SIGNATURE	p_dynarray_obj has an invalid signature
 */
int dynarray_get_sizeof_element( void *p_dynarray_obj, long *p_elem_len );




/* IMPORTANT WARNING to caller:
 *	If the elements stored in the array themselves contain pointer(s) to
 *	allocated memory, then the caller must take care of doing the free()
 *	of these allocations.
 *
 *	  NOTE: If the elements stored are also dynarray objects, then
 *		this function does a full cleanup using recursion.
 *
 * STEPS:
 *	Check signature of  p_dynarray_obj
 *	If b_elems_are_dynarray, iterate on each element and call
 *		dynarray_free(...)
 *
 *	WARNING: if (b_elems_are_dynarray is TRUE) then assert that:
 *			sizeof_element == sizeof(DYN_ARRAY_OBJ_T)
 *
 *	set ctr_used = 0
 *	set iterator_current_i = 0
 *
 * RETURN:
 *	0				OK, success
 *	DYNARRAY_ERR_NULL_OBJ_PTR	p_dynarray_obj is NULL
 *	DYNARRAY_ERR_INVALID_SIGNATURE	p_dynarray_obj has an invalid signature
 *  DYNARRAY_ERR_INTERNAL_LOGIC_ERROR the number of element stored in
 *					p_dynarray_obj does not agree with the number
 *					of element reported by ctr_used
 */
int dynarray_empty_dynarray( void *p_dynarray_obj );




/* STEPS:
 *	Check signature of p_dynarray_obj
 *	if at end of array ( iterator_current_i > ctr_used ) then
 *		return NULL
 *	p_elem = &p_allocated_array[iterator_current_i - 1 * sizeof_element]
 *	return p_elem
 * This function MUST be called AFTER the current row is
 * retrieved, so the iterator is 1 more than what is
 * desired
 *
 * RETURN:
 *	NULL	  at end of array OR bad signature OR NULL  p_dynarray_obj
 *	(void *)  OK, success
 */
void *dynarray_get_current_elem( void *p_dynarray_obj );





/* For Debugging purposes, return the value of the iterator.  The iterator
 * is incremented AFTER a call dynarray_get_next_elem
 *
 *	RETURN:
 *	< 0	  bad signature OR NULL  p_dynarray_obj
 *	long	current value of iterator of dynarray object
 */
long dynarray_get_value_of_iterator( void *p_dynarray_obj );




/*
 * This function will use bsearch to search (key) a value in
 * a dynarray.
 *
 * A custom compare function must me supply by the calling function.
 * This function must return 0 if both values are =, -1 if the 
 * first is < than the second else +1.
 *
 * 3 values are return by dynarray_bsearch. The found values (if found) and
 * The values just before and after the found values or where the found value
 * will have be located.
 *
 */
int dynarray_bsearch(	void *p_dynarray_obj,
			void *key,
			int (*compare)(const void * , const void *),
			void **found_value,
			const void **before_value,
			const void **after_value);




/*
 * This function call qsort to sort a dynarray.
 *
 * See dynarray_bsearch for more information about the compare function.
 *
 */
int dynarray_qsort(	void *p_dynarray_obj,
			int (*compare)(const void * , const void *));


/* --------------------------------- end of dynarray.h ------------------- */
