/* alogelvl.h
 * ----------
 *
 *
 *  Copyright (c) 1994-1996 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/include/alogelvl.h,v $
 * $Header: /ext_hd/cvs/include/alogelvl.h,v 1.4 2004/10/15 20:23:10 jf Exp $
 *
 * Defines the levels of errors (in log file).
 *
 * -----------------------------------------------------------------------
 * Versions:
 * 1994sept09,jft: Moved these defines from "apl_log1.h" to here.
 * 1995mar07,jft: added ALOG_APP_ERR, ALOG_APP_WARN
 * 1996jan17,jft: added ALOG_NOTIFY, ALOG_OPERATOR, ALOG_PANIC
 */

#ifndef _ALOGELVL_H_DONE_
#define _ALOGELVL_H_DONE_	done

/* Log System types */

#define    ALOG_ERROR         0
#define    ALOG_INFO          1
#define    ALOG_WARN          2
#define    ALOG_SRVERR        3
#define    ALOG_SRVMSG        4
#define    ALOG_UNKNOWN       5
#define    ALOG_APP_WARN	11
#define    ALOG_APP_ERR		12

#define	   ALOG_NOTIFY		20
#define    ALOG_OPERATOR	21

#define    ALOG_PANIC		99


#endif	   /* #ifndef _ALOGELVL_H_DONE_ */

/* --------------------------------- end of alogelvl.h ------------------ */
