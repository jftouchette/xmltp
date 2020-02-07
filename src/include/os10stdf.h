
/* os10stdf.h
 * ----------
 *
 * Standard #include and #define's for OS-Lib 10.0 (on HP-UX) or CT-lib only on Linux
 *
 *  Copyright (c) 1994-2003 Jean-Francois Touchette
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  (The file "COPYING" or "LICENSE" in a directory above this source file
 *  should contain a copy of the GNU Lesser General Public License text).
 *  -------------------------------------------------------------------------
 *
 * $Source$
 * $Header$
 *
 * NOTE: this is a revised version of "os10stdf.h" to be used in compiling
 *	 various XMLTP/L modules
 *
 * ------------------------------------------------------------------------
 * Versions:
 * 1994sep13,jft: first implementation
 * 2003jan23,jft: version for XMLTP/L
 */

#ifndef _OS10STDF_H_
#define _OS10STDF_H_

#if (__STDC__) || defined(__cplusplus)
#define FILE_DATE_TIME_STRING	(__FILE__ ": " __DATE__ " - " __TIME__)
#define VERSION_ID		(FILE_DATE_TIME_STRING)
#endif


/*
** Operating system header files required by most *.c modules
*/
#include        <stdio.h>
#include        <sys/signal.h>
#include        <string.h>
#include        <ctype.h>

/*
** Sybase header files required by modules using the Open Client API:
*/
#include <csconfig.h>
#include <cstypes.h>
#include <cspublic.h>
#define SRV_MAXERROR	20000	/* the only constant from the old oserror.h that is still used */


/*
** Standard error/return codes defines:
*/
#include "lbos_def.h"	/* all LBOS_xxxx defines		*/
#include "alogelvl.h"	/* ALOG_xxx defines			*/




/* We include prototypes only is the compiler is used in Standard ANSI C:
 */
#if (__STDC__) || defined(__cplusplus) || defined(_AIX)
#include "apl_log1.h"	/* functions prototypes alog_xxx() and apl_log() */
#endif


/*
 * As found with "select distinct severity from sysmessages",
 * the highest severity level in Sybase SQL Server 10.x.x is 26.
 * (jft,3avril1995)
 */
#define OS10_HIGHEST_SEVERITY_LEVEL	26


#endif	 /* _OS10STDF_H_ */

/* -------------------------------------------- end of os10stdf.h -------- */
