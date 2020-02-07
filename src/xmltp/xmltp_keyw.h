/*
 * xmltp_keyw.h
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_keyw.h,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_keyw.h,v 1.6 2004/10/14 12:18:50 blanchjj Exp $
 *
 *
 * XML-TP parser-lexer:  Keywords table
 * ------------------------------------
 *
 * This header file is included by xmltp_lex.c
 * ===========================================
 *
 * ------------------------------------------------------------------------
 * 2001aug25,jft: begin
 * 2002jan04,jft: added nbrows,totalrows
 * 2002jan07,jft: use tags constants from xmltp_tags.h
 * 2002jan14,jft: XMLTP_TAG_TOTALROWS and XMLTP_TAG_NBROWS should NOT have VALUE_FOLLOWS_IS_INTVAL
 * 2002may11,jft: added XMLTP_TAG_SZ, associated to VALUE_FOLLOWS_IS_SIZE
 */

#ifndef _XMLTP_TAGS_H_
#include "xmltp_tags.h"
#endif
 
#ifndef _XMLTP_KEYW_H_
#define _XMLTP_KEYW_H_

struct st_keywords_table {
	int	 token_val;
	char	*keyword;
	int	 b_value_follows;
 } s_keywords_tab[] = {
			/* WARNING:  Must be kept sorted because a binary */
			/* -------   search is used.			  */
	
	{ END_ATTR,		XMLTP_TAG_END_ATTR,			0 },
	{ END_COL,		XMLTP_TAG_END_COL,			0 },
	{ END_COLNAMES,		XMLTP_TAG_END_COLNAMES,			0 },
	{ END_DATE,		XMLTP_TAG_END_DATE,			0 },
	{ END_INT,		XMLTP_TAG_END_INT,			0 },
	{ END_ISNULL,		XMLTP_TAG_END_ISNULL,			0 },
	{ END_MSG,		XMLTP_TAG_END_MSG,			0 },
	{ END_MSGLIST,		XMLTP_TAG_END_MSGLIST,			0 },
	{ END_NAME,		XMLTP_TAG_END_NAME,			0 },
	{ END_NBROWS,		XMLTP_TAG_END_NBROWS,			0 },
	{ END_NUM,		XMLTP_TAG_END_NUM,			0 },
	{ END_OUTPARAMS,	XMLTP_TAG_END_OUTPARAMS,		0 },
	{ END_PARAM,		XMLTP_TAG_END_PARAM,			0 },
	{ END_PROC,		XMLTP_TAG_END_PROC,			0 },
	{ END_PROCCALL,		XMLTP_TAG_END_PROCCALL,			0 },
	{ END_RESPONSE,		XMLTP_TAG_END_RESPONSE,			0 },
	{ END_RESULTSET,	XMLTP_TAG_END_RESULTSET,		0 },
	{ END_RESULTSETSLIST,	XMLTP_TAG_END_RESULTSETSLIST,		0 },
	{ END_RETURNSTATUS,	XMLTP_TAG_END_RETURNSTATUS,		0 },
	{ END_ROW,		XMLTP_TAG_END_ROW,			0 },
	{ END_ROWS,		XMLTP_TAG_END_ROWS,			0 },
	{ END_STR,		XMLTP_TAG_END_STR,			0 },
	{ END_SZ,		XMLTP_TAG_END_SZ,			0 },
	{ END_TOTALROWS,	XMLTP_TAG_END_TOTALROWS,		0 },
	{ XMLVERSION_AND_ENCODING, XMLTP_TAG_XMLVERSION_AND_ENCODING,	0 },
	{ ATTR,			XMLTP_TAG_ATTR,		VALUE_FOLLOWS_IS_ATTRIB },
	{ COL,			XMLTP_TAG_COL,			0 },
	{ COLNAMES,		XMLTP_TAG_COLNAMES,		0 },
	{ DATE,			XMLTP_TAG_DATE,		VALUE_FOLLOWS_IS_STRVAL },
	{ INT,			XMLTP_TAG_INT,		VALUE_FOLLOWS_IS_INTVAL },
	{ ISNULL,		XMLTP_TAG_ISNULL,	VALUE_FOLLOWS_IS_ATTRIB },
	{ MSG,			XMLTP_TAG_MSG,			0 },
	{ MSGLIST,		XMLTP_TAG_MSGLIST,		0 },
	{ NAME,			XMLTP_TAG_NAME,		VALUE_FOLLOWS_IS_NAME },
	{ NBROWS,		XMLTP_TAG_NBROWS,		0 },
	{ NUM,			XMLTP_TAG_NUM,		VALUE_FOLLOWS_IS_STRVAL },
	{ OUTPARAMS,		XMLTP_TAG_OUTPARAMS,		0 },
	{ PARAM,		XMLTP_TAG_PARAM,		0 },
	{ PROC,			XMLTP_TAG_PROC,		VALUE_FOLLOWS_IS_NAME },
	{ PROCCALL,		XMLTP_TAG_PROCCALL,		0 },
	{ RESPONSE,		XMLTP_TAG_RESPONSE,		0 },
	{ RESULTSET,		XMLTP_TAG_RESULTSET,		0 },
	{ RESULTSETSLIST, 	XMLTP_TAG_RESULTSETSLIST,	0 },
	{ RETURNSTATUS,		XMLTP_TAG_RETURNSTATUS,		0 },
	{ ROW,			XMLTP_TAG_ROW,			0 },
	{ ROWS,			XMLTP_TAG_ROWS,			0 },
	{ STR,			XMLTP_TAG_STR,		VALUE_FOLLOWS_IS_STRVAL },
	{ SZ,			XMLTP_TAG_SZ,		VALUE_FOLLOWS_IS_SIZE   },
	{ TOTALROWS,		XMLTP_TAG_TOTALROWS,		0 },

};



#endif


/* ------------------------------------------ end of xmltp_keyw.h ------------ */
 
