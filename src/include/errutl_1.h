/* errutl_1.h
 * ----------
 *
 *  Copyright (c) 1994-1997 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/include/errutl_1.h,v $
 * $Header: /ext_hd/cvs/include/errutl_1.h,v 1.3 2004/10/15 20:27:43 jf Exp $
 *
 *
 * Functions prefix:	errutl_
 * 
 * This file contains Utility functions used by the generic Error handlers
 * functions (those in ct_errh1.c)
 *
 * -------------------------------------------------------------------------
 * Versions:
 * 1994oct20,jft: first version
 */


/* ------------------------------------------------ Public functions: */

int errutl_severity_to_alog_err_level(CS_INT severity);



void errutl_terminate_msg_text(CS_CHAR *p_text, CS_INT text_len);



/* ---------------------------------------- end of errutl_1.h ------------ */

