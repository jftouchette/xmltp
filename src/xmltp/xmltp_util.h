/*
 * xmltp_util.h
 * ------------
 *
 *
 *  Copyright (c) 2001-2003 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_util.h,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_util.h,v 1.4 2004/10/14 12:18:54 blanchjj Exp $
 *
 *
 * XML-TP Utility Public functions
 * -------------------------------
 *
 * Functions Prefixes:
 * ------------------
 *		xmltp_util_XXX()	utility functions
 *
 * ------------------------------------------------------------------------
 * 2001sept19,jft: extracted from module xmltp_parser.c (split)
 */

#ifndef _XMLTP_UTIL_H_
#define _XMLTP_UTIL_H_


/* ------------------------------------------------ PUBLIC functions: */


int xmltp_util_get_datatype_etc_from_attrib(char *attrib,
					     int *p_datatype,
					     int *p_is_null,
					     int *p_is_output);
/*
 * Called by:	xmltp_parser_build_param()
 *
 * NOTE: See Java source code in com.cjc.common.sql.lowLevel.SQLParam
 *
 * The encoded string found at attrib has the following format:
 *	[0]	'1': isNull TRUE,   '0': isNull FALSE
 *	[1]	'1': isOutput TRUE, '0': isOutput FALSE
 *	[2..]	atoi(&attrib[2]) should give an int with a value defined
 *		in javaSQLtypes.h
 *
 * Return:
 *	0	OK
 *	-1	NULL == attrib
 *	-2	p_datatype OR p_is_null OR p_is_output == NULL
 *	-3	strlen(attrib) < 3
 *	<= -4	other error
 */


#endif


/* --------------------------- end of xmltp_util.h ---------------- */
