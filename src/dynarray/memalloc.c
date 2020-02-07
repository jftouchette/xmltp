/* memalloc.c
 * ----------
 *
 *  Copyright (c) 1997-2003 Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/dynarray/memalloc.c,v $
 * $Header: /ext_hd/cvs/dynarray/memalloc.c,v 1.6 2004/10/13 18:23:31 blanchjj Exp $
 *
 *
 * Functions prefix:	mem_
 *
 * 
 * Memory allocation functions
 * ---------------------------
 * 
 * This module is used to "wrap" the memory management calls
 * to the libc library.  It keeps track of the amount of
 * memory allocated to the application.
 *
 * NOTE: The functions of this module are used by "ct10dll.c"
 *
 * -------------------------------------------------------------------------
 * Versions:
 * 1997apr24,deb: added standard RCS header
 * 1997may12,deb: ported to UNIX
 * 1997may14,deb: changed the way the allocated memory is counted so it uses
 *                the allocated block "real" size instead of the block size
 *                requested
 * 1997jul11,deb: . Added the optional logging trace of the function calls
 *                . Added mem_assign_debug_trace()
 * 1999oct05,dm:  Added an AIX version of the <_msize> function. (TICS)
 */

#ifdef __hppa
#define  _INCLUDE_HPUX_SOURCE
#define  _INCLUDE_XOPEN_SOURCE
#include <common_1.h>
#endif

#ifdef WIN32
#include <malloc.h>
#endif	/* ifndef WIN32 */

#include <stdlib.h>
#include <string.h>
#include "opt_log.h"
#include "debuglvl.h"



#ifndef TRUE
#define TRUE				1
#endif
#ifndef FALSE
#define FALSE				0
#endif


#define MEM_ALLOC_MAX_STRING_SIZE	80
#define MEM_ALLOC_LOG_MESSAGE		0xFFFFFFFFUL


/* Static variable that holds the memory allocated
 * in the application at all time
 */
static unsigned long s_memory_allocated = 0;


/* Static indicating if debug information should be printed
 * in the log file
 */
static int s_debug_trace =  FALSE;

/* The _msize function is a WIN32 function that returns the size of a memory block,
 * there is no equivalent on UNIX, this function plays the same role
 */
#ifdef __hppa
static size_t _msize( void *p )
{
	long    *header = NULL;
	size_t  sizeof_ptr = 0;

	if ( p != NULL ) {
		header = ( ( long * ) p ) - 1;
		sizeof_ptr = ( *header ) - 9;   /* the size of the memory block is 9 bytes before the pointer */

		/* Free blocks are marked by setting their lengths
		 * to an uneven size, this next line catch this condition
		 * and return a size of 0.
		 */
		if ( sizeof_ptr % 2 != 0 ) {
			sizeof_ptr = 0;
		}
	}

	return sizeof_ptr;
}
#endif	/* ifdef __hppa */


/* Version AIX of the _msize function */
/*	1) the length of the buffer is 4 bytes before the pointer */
/*	2) and if the block is free the value (of those 4 bytes before the pointer) is 0 */

#ifdef _AIX
static size_t _msize( void *p )
{
	long	*header = NULL;
	size_t	sizeof_ptr = 0;

	if ( p != NULL ) {
		header = ( ( long * ) p ) - 1;
		sizeof_ptr =  *header;	/* the size of the memory block is 4 bytes before the pointer */

		return sizeof_ptr;
	}
	else
		printf("pointeur null");

	return sizeof_ptr;
}
#endif


#ifdef __GNUC__
/*
 * Linux version of the _msize() function, a call to the undocumented
 * malloc_usable_size() function from glibc...
 */
static size_t _msize( void *p )
{
	return malloc_usable_size( p );
}
#endif



/* If b_new_value == TRUE, subsequent calls will print
 * debug information in the log file. Otherwise no
 * information of memory allocation will be logged.
 * (This behaviour is changed if the optional trace
 *  flag is NONE, then nothing is logged even if
 *  b_new_value is TRUE
 */
void mem_assign_debug_trace( int b_new_value )
{
	if ( b_new_value == TRUE ) {
		s_debug_trace =  TRUE;
	} else {
		s_debug_trace =  FALSE;
	}
}




/* Wraps the call to calloc()
 */
void * mem_calloc( size_t num, size_t size )
{
	char message[200] = "";
	void *p = NULL;

	p = calloc( num, size );

	if ( p == NULL ) {
		return ( NULL );
	}

	s_memory_allocated += _msize( p );

	if ( s_debug_trace == TRUE ) {
		sprintf( message,
			 "%lX = mem_calloc(%d, %d);  size: %d (allocated: %d)",
			 p, num, size, _msize( p ), s_memory_allocated );
		opt_log_aplogmsg( "memalloc.c", ALOG_INFO, 1, message, MEM_ALLOC_LOG_MESSAGE );
	}

	return ( p );
}



/* Wraps the call to realloc()
 */
void * mem_realloc( void *p_mem_block, size_t size )
{
	char message[200] = "";
	void   *p = NULL;
	size_t size_before = 0;	

	if ( p_mem_block != NULL ) {
		size_before = _msize( p_mem_block );
	}

	p = realloc( p_mem_block, size );

	if ( p == NULL ) {
		return ( NULL );
	}

	s_memory_allocated += _msize( p ) - size_before;

	if ( s_debug_trace == TRUE ) {
		sprintf( message,
			 "%lX = mem_realloc(%d);  size: %d, before: %d  (allocated: %d)",
			 p, size, _msize( p ), size_before, s_memory_allocated );
		opt_log_aplogmsg( "memalloc.c", ALOG_INFO, 2, message, MEM_ALLOC_LOG_MESSAGE );
	}

	return ( p );
}



/* Wraps the call to strdup()
 */
char * mem_strdup( const char *string_to_duplicate )
{
	char message[200] = "";
	char *new_string = NULL;

	new_string = strdup( string_to_duplicate );
	if ( new_string != NULL ) {
		s_memory_allocated += _msize( new_string );
	}

	if ( s_debug_trace == TRUE ) {
		sprintf( message,
			 "%lX = mem_strdup('%.*s');  size: %d (allocated: %d)",
			 new_string, MEM_ALLOC_MAX_STRING_SIZE, new_string, _msize( new_string ), s_memory_allocated );
		opt_log_aplogmsg( "memalloc.c", ALOG_INFO, 3, message, MEM_ALLOC_LOG_MESSAGE );
	}

	return ( new_string );
}



/* Wraps the call to free()
 */
void mem_free( void *p_memblock )
{
	char message[200] = "";

	if ( p_memblock != NULL ) {
		if ( s_debug_trace == TRUE ) {
			sprintf( message,
				"%lX : mem_free();  size: %d (allocated: %d)",
				p_memblock, _msize( p_memblock ), s_memory_allocated - _msize( p_memblock ) );
			opt_log_aplogmsg( "memalloc.c", ALOG_INFO, 4, message, MEM_ALLOC_LOG_MESSAGE );
		}

		s_memory_allocated -= _msize( p_memblock );
		free( p_memblock );
	}
}

