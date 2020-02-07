/* common_1.h
 * ----------
 *
 *
 *  Copyright (c) 1996-2001 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/include/common_1.h,v $
 * $Header: /ext_hd/cvs/include/common_1.h,v 1.7 2004/10/13 18:23:32 blanchjj Exp $
 *
 *
 * Standard #includes and #defines for common "C" programs.
 *
 *
 *  WARNING:	This header can be included in ANSI and NON-ANSI compilations!
 *		It must be compatible with both options.
 *
 *
 * ------------------------------------------------------------------------
 * Versions:
 * 1996jul16,jft: first version
 * 1999dec09,deb: Fixed VERSION_ID for windows
 */


#ifndef _COMMON_1_H_
#define _COMMON_1_H_

#if defined( __STDC__ ) || defined( __cplusplus ) || defined( _AIX ) || defined( WIN32 )
#define FILE_DATE_TIME_STRING	(__FILE__ ": " __DATE__ " - " __TIME__)
#define VERSION_ID		(FILE_DATE_TIME_STRING)
#endif

#ifndef _INCLUDE_POSIX_SOURCE
#define _INCLUDE_POSIX_SOURCE		"yes!"
#endif


#ifndef TRUE
#define TRUE			1
#endif
#ifndef FALSE
#define FALSE			0
#endif


/*
** Operating system header files required by most *.c modules
*/
#include <stdio.h>

#ifndef WIN32
#include <sys/signal.h>
#endif  /* ifndef WIN32 */

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#endif /* _COMMON_1_H_ */
