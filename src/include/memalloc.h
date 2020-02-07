/* memalloc.h
 * ----------
 *
 *
 *  Copyright (c) 1997-2003 Jocelyn Blanchet.
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
 * $Source: /ext_hd/cvs/include/memalloc.h,v $
 * $Header: /ext_hd/cvs/include/memalloc.h,v 1.3 2004/10/13 18:23:36 blanchjj Exp $
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
 * 1997jul24,deb: Added mem_assign_debug_trace()
 */

#ifndef MEMALLOC_H
#define MEMALLOC_H

#ifndef WIN32
#include <stdlib.h>
#endif




/* If b_new_value == TRUE, subsequent calls will print
 * debug information in the log file. Otherwise no
 * information of memory allocation will be logged.
 * (This behaviour is changed if the optional trace
 *  flag is NONE, then nothing is logged even if
 *  b_new_value is TRUE
 */
void mem_assign_debug_trace( int b_new_value );



/* Wraps the call to calloc()
 */
void * mem_calloc( size_t num, size_t size );



/* Wraps the call to realloc()
 */
void * mem_realloc( void *p_mem_block, size_t size );



/* Wraps the call to strdup()
 */
char * mem_strdup( const char *string_to_duplicate );



/* Wraps the call to free()
 */
void mem_free( void *p_memblock );

#endif	/* #ifndef MEMALLOC_H */
