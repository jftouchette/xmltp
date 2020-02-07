/* alogtypt.h
 * ----------
 *
 *
 *  Copyright (c) 1994-1995 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/include/alogtypt.h,v $
 * $Header: /ext_hd/cvs/include/alogtypt.h,v 1.3 2004/10/15 20:23:48 jf Exp $
 *
 * Section of table that match Strings to shared ALOG_ log "types" (levels).
 *
 *
 * IMPORTANT: The ALOG_xxx must be kept in ASCENDING order of their values.
 * ---------
 *	      See "ALOGELVL.h" to know their values.
 *
 * -----------------------------------------------------------------------
 * Version:
 * 1994sept14,jft: first version
 * 1995mar07,jft: added ALOG_APP_ERR, ALOG_APP_WARN
 */

/* This source code expect to be embedded within the initialization of 
 * array like:
 *	static struct int_str_tab_entry	s_type_strings_table[] = {
 *	#include "alogtypt.h"
 *		{ ISTR_NUM_DEFAULT_VAL,	 "?     " }
 *	 };
 */

	{ ALOG_ERROR, 		 "ERROR " },
	{ ALOG_INFO, 		 "MSG   " },
	{ ALOG_WARN, 		 "WARN  " },
	{ ALOG_SRVERR, 		 "ERRHDL" },
	{ ALOG_SRVMSG,	 	 "MSGHDL" },
	{ ALOG_APP_WARN,	 "APP_W " },
	{ ALOG_APP_ERR, 	 "APPERR" },

/* --------------------------------------------- end of alogtypt.h ------ */
