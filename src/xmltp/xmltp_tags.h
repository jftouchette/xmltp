/*
 * xmltp_tags.h
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_tags.h,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_tags.h,v 1.6 2004/10/14 12:18:53 blanchjj Exp $
 *
 *
 * XMLTP  TAGS
 * ------------
 *
 * NOTE: (1) the "<" and ">" are added by xmltp_writer.c
 *	 (2) the defines here are used as is (without < >) by xmltp_keyw.h
 *
 * ------------------------------------------------------------------------
 * 2001sept13,jft: begin
 * 2001oct16, jbl: Added: XMLTP_TAG_NBROWS, XMLTP_TAG_END_NBROWS.
 * 2002jan08,jft: fixed string concatenation in XMLTP_TAG_END_** constants ("/" XMLTP_...)
 * 2002may11,jft: added XMLTP_TAG_SZ
 */

#ifndef _XMLTP_TAGS_H_
#define _XMLTP_TAGS_H_

#define XMLTP_TAG_XMLVERSION_AND_ENCODING	"?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?"

#define XMLTP_TAG_EOT			"EOT/"

#define XMLTP_TAG_ATTR			"attr"
#define XMLTP_TAG_COL			"col"
#define XMLTP_TAG_COLNAMES		"colnames"
#define XMLTP_TAG_DATE			"date"
#define XMLTP_TAG_INT			"int"
#define XMLTP_TAG_ISNULL		"isnull"
#define XMLTP_TAG_MSG			"msg"
#define XMLTP_TAG_MSGLIST		"msglist"
#define XMLTP_TAG_NAME			"name"
#define XMLTP_TAG_NUM			"num"
#define XMLTP_TAG_OUTPARAMS		"outparams"
#define XMLTP_TAG_PARAM			"param"
#define XMLTP_TAG_PROC			"proc"
#define XMLTP_TAG_PROCCALL		"proccall"
#define XMLTP_TAG_RESPONSE		"response"
#define XMLTP_TAG_RESULTSET		"resultset"
#define XMLTP_TAG_RESULTSETSLIST	"resultsetslist"
#define XMLTP_TAG_RETURNSTATUS		"returnstatus"
#define XMLTP_TAG_ROW			"row"
#define XMLTP_TAG_ROWS			"rows"
#define XMLTP_TAG_STR			"str"
#define XMLTP_TAG_NBROWS		"nbrows"
#define XMLTP_TAG_TOTALROWS		"totalrows"
#define XMLTP_TAG_SZ			"sz"

#define XMLTP_TAG_END_ATTR		("/" XMLTP_TAG_ATTR)
#define XMLTP_TAG_END_COL		("/" XMLTP_TAG_COL)
#define XMLTP_TAG_END_COLNAMES		("/" XMLTP_TAG_COLNAMES)
#define XMLTP_TAG_END_DATE		("/" XMLTP_TAG_DATE)
#define XMLTP_TAG_END_INT		("/" XMLTP_TAG_INT)
#define XMLTP_TAG_END_ISNULL		("/" XMLTP_TAG_ISNULL)
#define XMLTP_TAG_END_MSG		("/" XMLTP_TAG_MSG)
#define XMLTP_TAG_END_MSGLIST		("/" XMLTP_TAG_MSGLIST)
#define XMLTP_TAG_END_NAME		("/" XMLTP_TAG_NAME)
#define XMLTP_TAG_END_NUM		("/" XMLTP_TAG_NUM)
#define XMLTP_TAG_END_OUTPARAMS		("/" XMLTP_TAG_OUTPARAMS)
#define XMLTP_TAG_END_PARAM		("/" XMLTP_TAG_PARAM)
#define XMLTP_TAG_END_PROC		("/" XMLTP_TAG_PROC)
#define XMLTP_TAG_END_PROCCALL		("/" XMLTP_TAG_PROCCALL)
#define XMLTP_TAG_END_RESPONSE		("/" XMLTP_TAG_RESPONSE)
#define XMLTP_TAG_END_RESULTSET		("/" XMLTP_TAG_RESULTSET)
#define XMLTP_TAG_END_RESULTSETSLIST	("/" XMLTP_TAG_RESULTSETSLIST)
#define XMLTP_TAG_END_RETURNSTATUS	("/" XMLTP_TAG_RETURNSTATUS)
#define XMLTP_TAG_END_ROW		("/" XMLTP_TAG_ROW)
#define XMLTP_TAG_END_ROWS		("/" XMLTP_TAG_ROWS)
#define XMLTP_TAG_END_STR		("/" XMLTP_TAG_STR)
#define XMLTP_TAG_END_NBROWS		("/" XMLTP_TAG_NBROWS)
#define XMLTP_TAG_END_TOTALROWS		("/" XMLTP_TAG_TOTALROWS)
#define XMLTP_TAG_END_SZ		("/" XMLTP_TAG_SZ)

#endif


/* ------------------------------------------ end of xmltp_tags.h ------------ */
 
