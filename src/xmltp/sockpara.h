/*
 * sockpara.h
 * ----------
 *
 *
 *  Copyright (c) 2001 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/xmltp/sockpara.h,v $
 * $Header: /ext_hd/cvs/xmltp/sockpara.h,v 1.3 2004/10/13 18:23:43 blanchjj Exp $
 *
 *
 * SOCKet servers config file PARAmeters
 * -------------------------------------
 *
 *
 * ------------------------------------------------------------------------
 * 2001sept11,jft: begin
 */

#ifndef _SOCKPARA_H_
#define _SOCKPARA_H_

/* 
 * Parameters names:
 */
#define SOCKPARA_SLEEP_RETRY_BIND	"SLEEP_RETRY_BIND"
#define SOCKPARA_NB_RETRY_LISTEN	"NB_RETRY_LISTEN"
#define SOCKPARA_QLEN			"QLEN"
#define SOCKPARA_LISTEN_PORT		"LISTEN_PORT"


/* 
 * Default values for parameters omitted:
 */
#define SOCKPARA_SLEEP_RETRY_BIND_DEFAULT	15
#define SOCKPARA_NB_RETRY_LISTEN_DEFAULT	 5
#define SOCKPARA_QLEN_DEFAULT			 8


#endif


/* -------------------------------------- end of sockpara.h ------------ */


