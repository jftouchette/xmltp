/* dynarray.c
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
 * $Source: /ext_hd/cvs/dynarray/dynarray.c,v $
 * $Header: /ext_hd/cvs/dynarray/dynarray.c,v 1.9 2004/10/15 20:21:58 jf Exp $
 *
 *
 * Functions prefix:	dynarray_
 *
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
 * 1997jan31,jft: added long dynarray_get_nb_elem_stored(void *p_dynarray_obj)
 * 1997may12,deb: ported to unix
 * 1997jun05,jbl: added: dynarray_qsort and dynarray_bsearch function.
 * 1997jun09,jbl: fixed: dynarray_bsearch when search key greater than last
 *			 element of array.
 * 1997jun10,deb: fixed: s_last_error_description assigned everywhere an error
 *			 occurs.
 * 1997jun11,deb: added: DESCR_ERR_MAX_ELT_EXCEEDED
 * 1997jun20,deb: checked for an empty dynarray in dynarray_qsort() and
 *                dynarray_bsearch()
 * 2000jun30,deb: Fixed for AIX compiler that doesn't like assignment between
 *                (void *) and (const void *)
 */

/* Don't include ntunixfn.h for unix port
 */
#ifdef WIN32
#include <ntunixfn.h>
#endif	/* ifdef WIN32 */

#include <assert.h>

#include "dynarray.h"
#include "memalloc.h"
#include <stdlib.h>


/* -------------------------------------------------- Private Defines: */

/* Signature of a valid dynarray object
 */
#define DYN_ARRAY_OBJ_SIGNATURE		0x01ff0f01


/* Different error strings
 */
#define DESCR_NO_ERROR_YET		"[no error yet]"
#define DESCR_ERR_WRONG_ELEM_SIZE	"(b_elems_are_dynarray is TRUE) and\
 sizeof_element != sizeof(DYN_ARRAY_OBJ_T) "
#define DESCR_ERR_SIZEOF_ELEMENT	"sizeof_element must be >= 4"
#define DESCR_ERR_MAX_ALLOCATABLE	"max_allocatable must be in [2..1000000] range"
#define DESCR_ERR_GROWTH_INCREMENT	"growth_increment must be in [1..1000] range"
#define DESCR_ERR_INITIAL_SIZE		"initial_size must be in [1..1000] range"
#define DESCR_ERR_OUT_OF_MEMORY		"out of memory in dynarray.c"
#define DESCR_ERR_NULL_PTR		"null pointer"
#define DESCR_ERR_INVALID_SIGNATURE	"invalid signature on object"
#define DESCR_ERR_INTERNAL_LOGIC_ERROR	"internal logic error in dynarray.c"
#define DESCR_ERR_MAX_ELT_EXCEEDED	"max number of element exceeded"


/* Minimum size of an element
 * Minimum value accepted as the maximum size of a dynarray
 * Maximum value accepted as the maximum nb of element in a dynarray
 * Minimum growth increment of a dynarray object
 * Maximum growth increment of a dynarray object
 * Minimum initial size of a dynarray
 * Maximum initial size of a dynarray
 */
#define VAL_MIN_SIZEOF_ELEMENT		4
#define VAL_MIN_MAX_ALLOCATABLE		2
#define VAL_MAX_MAX_ALLOCATABLE		1000000
#define VAL_MIN_GROWTH_INCREMENT	1
#define VAL_MAX_GROWTH_INCREMENT	1000
#define VAL_MIN_INITIAL_SIZE		1
#define VAL_MAX_INITIAL_SIZE		1000

#ifndef FALSE
#define FALSE           0
#endif

#ifndef TRUE
#define TRUE            1
#endif


/* -------------------------------------------------- Private Structures: */


/*
 * Pointer to the custom compare function. This pointer is use by 
 * dynarray_bsearch function. The user custom compare function pointer
 * will not be given directly to bsearch. The compare function will
 * be wraped and the wraping function will use this pointer to 
 * call the user function.
 *
 */
int (*custom_compare_function)(const void * , const void *);

static const void *last_elem_compare;


/* DYN_ARRAY_OBJ_T:
 *
 * The DYN_ARRAY_OBJ_T control is used to manage an array of whatever-
 * type that can grow as needed until a predefined maximum of elements 
 * (max_allocatable) is reached.
 * 
 * signature		 is always == DYN_ARRAY_OBJ_SIGNATURE when the struct
 *			 is properly initialized. It is set to -1 just before
 *			 the struct is freed with free().
 * b_elems_are_dynarray	 if this indicator is TRUE, then dynarray_free()
 *			 can recurse in elements.
 * sizeof_element	 is the number of bytes in each element of the array
 * max_allocatable	 imposes a limit to the growth of the array (with 
 *			 successive bigger realloc() calls)
 * growth_increment	 is the number added to nb_allocated before calling
 *			 realloc()
 * nb_allocated		 is the number of elements allocated in the array
 *			 pointed to by  p_allocated_array
 * ctr_used		 is the number of array elements currently initialized
 *			 and containing data (NOTE: unused allocated elements 
 *			 should be entirely set to binary zeroes).
 * iterator_current_i	 is the current indice used to get the next element
 *			 in the array through standard iterator functions,
 *			 dynarray_get_next_elem() and dynarray_get_first_elem()
 * p_allocated_array	 points the memory buffer that holds the array itself.
 *
 *
 * *** WARNING: The following relation must always stay true:
 *
 *	  iterator_current_i < ctr_used <=  nb_allocated <= max_allocatable
 */
typedef struct {
	long	signature;
	int		b_elems_are_dynarray;
	long	sizeof_element,
			max_allocatable,
			growth_increment,
			nb_allocated,
			ctr_used,
			iterator_current_i;
	char	*p_allocated_array;
} DYN_ARRAY_OBJ_T;


/* -------------------------------------------------- Private variables: */

static char *s_last_error_description = DESCR_NO_ERROR_YET;


/* -------------------------------------------------- Private functions: */

static int sf_validate_signature( void *p_dynarray_obj )
{
	if (p_dynarray_obj == NULL) {
		s_last_error_description = DESCR_ERR_NULL_PTR;
		return (DYNARRAY_ERR_NULL_OBJ_PTR);
	}
	if ( ((DYN_ARRAY_OBJ_T *) p_dynarray_obj)->signature
	     != DYN_ARRAY_OBJ_SIGNATURE) {
		s_last_error_description = DESCR_ERR_INVALID_SIGNATURE;
		return (DYNARRAY_ERR_INVALID_SIGNATURE);
	}
	return (0);	/* signature is OK */

} /* end of sf_validate_signature() */





/* STEPS:
 *	check signature
 *	check if (nb_allocated + growth_nb_elems) < max_allocatable
 * 	use realloc() to increase the size of p_allocated_array
 *	 NOTE: the new size will be:
 *		( (nb_allocated + growth_increment) * sizeof_element )
 *
 * RETURN:
 *	0				OK, success
 *	DYNARRAY_ERR_MAX_ALLOC		max_allocatable maximum reached 
 *	DYNARRAY_ERR_OUT_OF_MEMORY	calloc() or realloc() failed
 *	DYNARRAY_ERR_NULL_OBJ_PTR	p_dynarray_obj is NULL
 *	DYNARRAY_ERR_INVALID_SIGNATURE	p_dynarray_obj has invalid signature
 */
static int sf_grow_dynarray( void *p_dynarray_obj, int growth_nb_elems )
{
	int		rc = 0;
	void	*p_buffer = NULL;
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( rc );
	}

	if ( p_dynarray->nb_allocated + growth_nb_elems <= p_dynarray->max_allocatable ) {

		/* Make the reallocation into a temporary buffer because
		 * if we are out of memory, the original buffer is preserved
		 * and a NULL pointer is returned
		 */
		p_buffer =
			mem_realloc( p_dynarray->p_allocated_array,
				( p_dynarray->nb_allocated + growth_nb_elems ) *
				p_dynarray->sizeof_element );

		if ( p_buffer == NULL ) {
			s_last_error_description = DESCR_ERR_OUT_OF_MEMORY;

			return( DYNARRAY_ERR_OUT_OF_MEMORY );
		} else {

			/* Increment the counter of element allocated
			 */
			p_dynarray->nb_allocated += growth_nb_elems;
			p_dynarray->p_allocated_array = p_buffer;

			return ( 0 );
		}

	} else {
		s_last_error_description = DESCR_ERR_MAX_ELT_EXCEEDED;
		return( DYNARRAY_ERR_MAX_ALLOC );
	}
}




/* -------------------------------------------- Public Functions: */

char *dynarray_get_description_of_last_error( void )
{
	return ( s_last_error_description );
}




/* Some callers might want to pass the value sizeof(DYN_ARRAY_OBJ_T) as
 * argument sizeof_element when they call dynarray_create_new_dynamic_array()
 * (because they want to create a dynarray of dynarray!)
 *
 * Return: 	sizeof(DYN_ARRAY_OBJ_T)
 */
int dynarray_get_size_of_dynarray_struct( void )
{
	return ( sizeof( DYN_ARRAY_OBJ_T ) );
}






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
					int  b_elems_are_dynarray )
{
	DYN_ARRAY_OBJ_T	*p_new = NULL;

	s_last_error_description = DESCR_NO_ERROR_YET;

	if ( sizeof_element < VAL_MIN_SIZEOF_ELEMENT ) {
		s_last_error_description = DESCR_ERR_SIZEOF_ELEMENT;
		return( NULL );
	}

	if ( ( max_allocatable < VAL_MIN_MAX_ALLOCATABLE ) ||
		 ( max_allocatable > VAL_MAX_MAX_ALLOCATABLE ) ) {
		s_last_error_description = DESCR_ERR_MAX_ALLOCATABLE;
		return( NULL );
	}

	if ( ( growth_increment < VAL_MIN_GROWTH_INCREMENT ) ||
		 ( growth_increment > VAL_MAX_GROWTH_INCREMENT ) ) {
		s_last_error_description = DESCR_ERR_GROWTH_INCREMENT;
		return( NULL );
	}

	if ( ( initial_size < VAL_MIN_INITIAL_SIZE ) ||
		 ( initial_size > VAL_MAX_INITIAL_SIZE ) ) {
		s_last_error_description = DESCR_ERR_INITIAL_SIZE;
		return( NULL );
	}

	if ( b_elems_are_dynarray == TRUE ) {
		b_elems_are_dynarray = TRUE;
	} else {
		b_elems_are_dynarray = FALSE;
	}

	if ( ( b_elems_are_dynarray == TRUE ) &&
		 ( sizeof_element != sizeof( DYN_ARRAY_OBJ_T ) ) ) {
		s_last_error_description = DESCR_ERR_WRONG_ELEM_SIZE;
		return( NULL );
	}

	if ( ( p_new = mem_calloc( 1, sizeof( DYN_ARRAY_OBJ_T ) ) ) == NULL ) {
		s_last_error_description = DESCR_ERR_OUT_OF_MEMORY;
		return ( NULL );
	}

	p_new->signature		= DYN_ARRAY_OBJ_SIGNATURE;
	p_new->b_elems_are_dynarray	= b_elems_are_dynarray;
	p_new->max_allocatable		= max_allocatable;
	p_new->sizeof_element		= sizeof_element;
	p_new->growth_increment		= growth_increment;
	p_new->nb_allocated		= 0;
	p_new->ctr_used			= 0;
	p_new->iterator_current_i	= 0;
	p_new->p_allocated_array	= NULL;

	if ( sf_grow_dynarray(p_new, initial_size) != 0 ) {
		mem_free( p_new );
		return ( NULL );
	}

	return ( p_new ); 

} /* end of dynarray_create_new_dynamic_array() */







/* IMPORTANT WARNING to caller:
 *	If the elements stored in the array themselves contain pointer(s) to
 *	allocated memory, then the caller must take care of doing the free()
 *	of these allocations.
 *
 * SECOND IMPORTANT WARNING to caller:
 *  This call does not free the dynarray itself, just the array pointed
 *  by the dynarray.  This is for dynarrays of dynarrays.  Since these
 *  arrays are contiguous in the "parent" dynarray, if we would free
 *  them we would free ALL the "child" dynarrays of the "parent" dynarray.
 *  So it is the responsibility of the caller to free the dynarray itself.
 *  The dynarray MUST be free by the caller because it is unusable after
 *  the call to dynarray_free(), the signature of the dynarray object
 *  is no longer valid.
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
 *	free p_allocated_array
 *	set signature = -1
 *	set nb_allocated = 0
 *	set ctr_used = 0
 *
 * RETURN:
 *	0				OK, success
 *	DYNARRAY_ERR_NULL_OBJ_PTR	p_dynarray_obj is NULL
 *	DYNARRAY_ERR_INVALID_SIGNATURE	p_dynarray_obj has an invalid signature
 *  DYNARRAY_ERR_INTERNAL_LOGIC_ERROR All the elements of the dynarray could not
 *					  be accessed
 */
int dynarray_free( void *p_dynarray_obj )
{
	int				rc = 0;
	int				i = 0;
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	void			*p_element_dynarray = NULL;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( rc );
	}

	if ( p_dynarray->b_elems_are_dynarray == TRUE ) {
		assert( p_dynarray->sizeof_element == sizeof( DYN_ARRAY_OBJ_T ) );

		for ( i = 0; i < p_dynarray->ctr_used; i++ ) {
			p_element_dynarray = dynarray_get_nth_elem( p_dynarray, i );
			if ( p_element_dynarray == NULL ) {
				s_last_error_description = DESCR_ERR_INTERNAL_LOGIC_ERROR;
				return ( DYNARRAY_ERR_INTERNAL_LOGIC_ERROR );
			}
			dynarray_free( p_element_dynarray );
		}
	}

	/* Invalidate signature, just in case...
	 */
	p_dynarray->signature = -1;
	p_dynarray->nb_allocated = 0;
	p_dynarray->ctr_used = 0;

	mem_free( p_dynarray->p_allocated_array );
	
	return ( 0 );

} /* end of dynarray_free() */






/* Return the size of the elements stored in the dynarray
 *
 * RETURN:
 *  0								OK
 *	DYNARRAY_ERR_NULL_OBJ_PTR		p_dynarray_obj is NULL
 *	DYNARRAY_ERR_INVALID_SIGNATURE	p_dynarray_obj has an invalid signature
 */
int dynarray_get_sizeof_element( void *p_dynarray_obj, long *p_elem_len )
{
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	int				rc = 0;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( rc );
	}

	*p_elem_len = p_dynarray->sizeof_element;

	return ( 0 );
} /* end of dynarray_get_sizeof_element() */





/* STEPS:
 *	Check signature of p_dynarray_obj
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
int dynarray_add_new_element( void *p_dynarray_obj, void *p_elem )
{
	int				rc = 0;
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( rc );
	}

	if ( p_elem == NULL ) {
		s_last_error_description = DESCR_ERR_NULL_PTR;
		return ( DYNARRAY_ERR_NULL_ARG );
	}

	if ( p_dynarray->nb_allocated <= p_dynarray->ctr_used ) {
		rc = sf_grow_dynarray( p_dynarray, p_dynarray->growth_increment );
		if ( rc != 0 ) {
			return ( rc );
		}
	}

	memcpy( p_dynarray->p_allocated_array +
			p_dynarray->ctr_used * p_dynarray->sizeof_element,
			p_elem, p_dynarray->sizeof_element );
	p_dynarray->ctr_used++;

	return ( 0 );

} /* end of dynarray_add_new_element() */






/* RETURN:
 *	-1	error
 *	>= 0	number of elements currently stored (== ctr_used)
 */
long dynarray_get_nb_elem_stored( void *p_dynarray_obj )
{
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;

	s_last_error_description = DESCR_NO_ERROR_YET;

	if ( sf_validate_signature( p_dynarray_obj ) != 0 ) {
		return ( -1 );
	}

	return ( p_dynarray->ctr_used );
} 






/* STEPS:
 *	Check signature of  p_dynarray_obj
 *	if at end of array ( iterator_current_i >= ctr_used ) then
 *		return NULL
 *	p_elem = &p_allocated_array[iterator_current_i * sizeof_element]
 *	increment iterator_current_i
 *	return p_elem
 *
 * RETURN:
 *	NULL	  at end of array OR bad signature OR NULL  p_dynarray_obj
 *	(void *)  OK, success
 */
void *dynarray_get_next_elem( void *p_dynarray_obj )
{
	int				rc = 0;
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	void			*p_elem = NULL;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( NULL );
	}

	if ( p_dynarray->iterator_current_i >= p_dynarray->ctr_used ) {
		return ( NULL );
	}

	p_elem = p_dynarray->p_allocated_array +
			 p_dynarray->iterator_current_i * p_dynarray->sizeof_element;
	
	p_dynarray->iterator_current_i++;
	
	return ( p_elem );

} /* end of dynarray_get_next_elem() */







/* STEPS:
 *	Check signature of  p_dynarray_obj *	set iterator_current_i = 0
 *	return ( dynarray_get_next_elem(p_dynarray_obj) )
 *
 * RETURN:
 *	NULL	  at end of array OR bad signature OR NULL  p_dynarray_obj
 *	(void *)  OK, success
 */
void *dynarray_get_first_elem( void *p_dynarray_obj )
{
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	int				rc = 0;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( NULL );
	}
	
	p_dynarray->iterator_current_i = 0;

	return ( dynarray_get_next_elem( p_dynarray ) );

} /* end of dynarray_get_first_elem() */








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
void *dynarray_get_nth_elem( void *p_dynarray_obj, long i )
{
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	int				rc = 0;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( NULL );
	}

	if ( ( i < 0 ) || ( i >= p_dynarray->ctr_used ) ) {
		return ( NULL );
	}
	
	return ( p_dynarray->p_allocated_array + i * p_dynarray->sizeof_element );

} /* end of dynarray_get_nth_elem() */








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
int dynarray_empty_dynarray( void *p_dynarray_obj )
{
	int				rc = 0;
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	DYN_ARRAY_OBJ_T	*p_element_dynarray = NULL;
	int				i = 0;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( rc );
	}
	if ( p_dynarray->b_elems_are_dynarray == TRUE ) {
		for ( i = 0; i < p_dynarray->ctr_used; i++ ) {
			p_element_dynarray = dynarray_get_nth_elem( p_dynarray, i );
			if ( p_element_dynarray == NULL ) {
				s_last_error_description = DESCR_ERR_INTERNAL_LOGIC_ERROR;
				return ( DYNARRAY_ERR_INTERNAL_LOGIC_ERROR );
			}
			dynarray_free( p_element_dynarray );
		}
	}

	p_dynarray->ctr_used		= 0;
	p_dynarray->iterator_current_i	= 0;

	return ( 0 );
}







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
void *dynarray_get_current_elem( void *p_dynarray_obj )
{
	int				rc = 0;
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	void			*p_elem = NULL;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( NULL );
	}

	/* This function MUST be called AFTER the current row is
	 * retrieved, so the iterator is 1 more than what is
	 * desired
	 */
	if ( ( p_dynarray->iterator_current_i > p_dynarray->ctr_used ) ||
		 ( p_dynarray->iterator_current_i < 1 ) ) {
		return ( NULL );
	}

	p_elem = p_dynarray->p_allocated_array +
			 ( p_dynarray->iterator_current_i - 1 ) * p_dynarray->sizeof_element;
	
	return ( p_elem );

} /* end of dynarray_get_current_elem() */





/* For Debugging purposes, return the value of the iterator.  The iterator
 * is incremented AFTER a call dynarray_get_next_elem
 *
 *	RETURN:
 *	< 0	  bad signature OR NULL  p_dynarray_obj
 *	long	current value of iterator of dynarray object
 */
long dynarray_get_value_of_iterator( void *p_dynarray_obj )
{
	int	rc = 0;
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;

	s_last_error_description = DESCR_NO_ERROR_YET;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( -1 );
	}

	return ( p_dynarray->iterator_current_i );
}


/*
 * Special compare function use by dynarray_bsearch.
 *
 * This function is use only to remember the last element
 * that qsearch access. We need it in case the search
 * element is not in the dynarray. In that case bsearch
 * return null, but dynarray_bsearch will return the
 * two elements located before and after where the search value
 * should have be located.
 */

int dynarray_bsearch_compare(const void *elem1, const void *elem2)
{
int rc;

rc = (custom_compare_function)(elem1,elem2); 
last_elem_compare = elem2;

return rc;
}


/*
 * This function search a key in a dynarray.
 *
 * The found value is return and also the value
 * just before and after.
 *
 * Any unavailable value is return as NULL.
 *
 */

int dynarray_bsearch(	void *p_dynarray_obj,
			void *key,
			int (*compare)(const void * , const void *),
			void **found_value,
			const void **before_value,
			const void **after_value)
{
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	int	rc = 0;
	long	ind = 0;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( rc );
	}

	/* If the dynarray is empty, then return NULL
	 * for all values (found, before and after)
	 */
	if ( p_dynarray->ctr_used == 0 ) {
		*found_value = NULL;
		*before_value = NULL;
		*after_value = NULL;

		return ( 0 );
	}

	custom_compare_function = compare;

	*found_value = bsearch(
				key,
				p_dynarray->p_allocated_array,
				p_dynarray->ctr_used,
				p_dynarray->sizeof_element,
				&dynarray_bsearch_compare
				);


/*
 * This part of the code is only there to find the
 * before and after elements.
 */


/*
 * Convert last element access by bsearch in a corresponding
 * array indice.
 */
	ind =	(long) (((long) last_elem_compare - 
		((long) p_dynarray->p_allocated_array)) / 
		p_dynarray->sizeof_element);

/*
 * If the search element is found the before and after element are
 * not located the same way than if the search element is not found.
 *
 */
	if (*found_value != NULL) {
		if (ind > 0) {
			*before_value = (void *) (((long) *found_value) - 
					p_dynarray->sizeof_element);
		}
		else {
			*before_value = NULL;
		}

		if (ind < (p_dynarray->ctr_used-1) ) {
			*after_value = (void *) (((long) *found_value) + 
					p_dynarray->sizeof_element);
		}
		else {
			*after_value = NULL;
		}
	}
	else {
		if (compare(key, last_elem_compare) < 0 ) {
			*after_value = last_elem_compare;

			if (ind > 0) {
				*before_value = (void *) 
						(((long) last_elem_compare) - 
						p_dynarray->sizeof_element);
			}
			else {
				*before_value = NULL;
			}
		}
		else {
			*before_value = last_elem_compare;

			if (ind < p_dynarray->ctr_used-1) {
				*after_value = (void *) 
						(((long) last_elem_compare) + 
						p_dynarray->sizeof_element);
			}
			else {
				*after_value = NULL;
			}
		}
	}

	return 0;
}


/*
 * This function sort a dynarray.
 *
 */
int dynarray_qsort(	void *p_dynarray_obj,
			int (*compare)(const void * , const void *))
{
	DYN_ARRAY_OBJ_T	*p_dynarray = p_dynarray_obj;
	int	rc = 0;

	rc = sf_validate_signature( p_dynarray_obj );
	if ( rc != 0 ) {
		return ( rc );
	}

	/* Don't sort an empty dynarray or a dynarray with
	 * just 1 element
	 */
	if ( p_dynarray->ctr_used <= 1 ) {
		return ( 0 );
	}

	qsort(	p_dynarray->p_allocated_array,
		p_dynarray->ctr_used,
		p_dynarray->sizeof_element,
		compare
		);
	return 0;
}

/* --------------------------------- end of dynarray.c ------------------- */
