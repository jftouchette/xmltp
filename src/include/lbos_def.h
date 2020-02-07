/* lbos_def.h
 * ----------
 *
 *
 *  Copyright (c) 1994 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/include/lbos_def.h,v $
 * $Header: /ext_hd/cvs/include/lbos_def.h,v 1.3 2004/10/15 20:30:47 jf Exp $
 *
 *
 * Custom error/return codes (LBOS_...) used in OS programs.
 *
 * ------------------------------------------------------------------------
 * Versions:
 * 1994sept14,jft: first implementation
 */


#ifndef _LBOS_DEF_H_
#define _LBOS_DEF_H_

/* Defines of some ASCII values:
 */
#ifndef NUL
#define NUL		'\0'
#endif
#ifndef TAB
#define TAB		'\t'
#endif
#ifndef CR
#define CR		'\r'
#endif
#ifndef NL
#define NL		'\n'
#endif



/* Maximums related to DBMS implementation:
 */
#define LBOS_MAXCOLS		255		/* max of columns in a table */
#define	LBOS_MAXPARAMS		(LBOS_MAXCOLS)	/* max nb of params in RPC */


/* Standard exit() codes (custom):
 */
#define LBOS_NORMAL_EXIT		0
#define LBOS_INIT_ERROR_EXIT		1
#define LBOS_SRV_RUN_ERROR_EXIT		2
#define LBOS_SRV_STOP_FAST_EXIT		3

/* Standard error/return codes (custom):
 */
#define LBOS_ABORT_SERVER_RUN	-20
#define LBOS_ABORT_SERVER_INIT	-10
#define LBOS_ABORT_WORK		 -8
#define LBOS_CANCEL_RESULTS	 -5
#define LBOS_ERR_OUT_OF_MEMORY	 -3
#define LBOS_ERROR		 -2
#define LBOS_FAILED		 (LBOS_ERROR)
#define LBOS_NOT_FOUND		 -1
#define LBOS_OK			  0

/* Local definition of TRUE and FALSE:
*/
#define LBOS_TRUE		  1
#define LBOS_FALSE		  0


#endif	 /* _LBOS_DEF_H_ */

/* -------------------------------------------- end of lbos_def.h -------- */
