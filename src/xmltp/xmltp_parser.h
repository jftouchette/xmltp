/*
 * xmltp_parser.h
 * --------------
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_parser.h,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_parser.h,v 1.12 2004/10/14 12:18:51 blanchjj Exp $
 *
 *
 * XML-TP Parser -- Parser Actions API
 * -----------------------------------
 *
 * Functions Prefix:	xmltp_parser_XXX()	parser actions functions
 * ----------------				(high-level)
 *
 *
 * This module defines a general API for funtions called by the
 * "actions" of the XML-TP parser.
 * *
 * ------------------------------------------------------------------------
 * 2001aug29,jft: first version
 * 2002feb19,jft: . void *xmltp_parser_build_param(void *p, char *name, char *val, char *attrib);
 *			Now receives p_ctx as first argument.
 * 2002may11,jft: . xmltp_parser_build_param(): added argument char *opt_size
 *		  . xmltp_parser_add_colname(): added argument char *opt_size
 */

#ifndef _XMLTP_PARSER_H_
#define _XMLTP_PARSER_H_


/* ----------------------------------------- PUBLIC FUNCTIONS:  */

#ifdef XMLTP_GX	/* the following function is used only in some programs */

void xmltp_parser_abort_parser(void *p_ctx, int rc, char *msg1, char *msg2);
/*
 * Used by the parser to tell the main program that too many
 * parse errors have occurred, that the XML-TP protocol is not
 * followed, and, that it should stop parsing and drop the connection.
 *
 * The explanations about the error conditions are in the strings
 * msg1, and, msg2.
 */
#endif


void xmltp_parser_set_debug_bitmask(int val);
/*
 * Set s_debug_bitmask in the xmltp_parser_xxx.c module
 */



/* --------------------------------------- PARSER (action) functions: */

/*
 * Actions used in parsing of XMLTP proc call:
 */
int xmltp_parser_proc_call_parse_end(void *p);

int xmltp_parser_set_proc_name(void *p, char *proc_name);

/*
 * Actions used in parsing of params in *BOTH* proc call AND response:
 */
int xmltp_parser_add_param_to_list(void *p, void *p_param);

void *xmltp_parser_build_param(void *p, char *name, char *val, char *attrib, char *opt_size);

/*
 * Actions used in parsing of XMLTP response:
 */
int xmltp_parser_response_parse_end(void *p); 

int xmltp_parser_response_parse_begin(void *p); 

int xmltp_parser_set_return_status(void *p, char *val);

int xmltp_parser_outparams_end(void *p); 

int xmltp_parser_outparams_begin(void *p); 

int xmltp_parser_msglist_end(void *p); 

int xmltp_parser_msglist_begin(void *p); 

int xmltp_parser_add_msg_to_list(void *p, char *msgno_str, char *msg);

int xmltp_parser_resultsetslist_end(void *p); 

int xmltp_parser_resultsetslist_begin(void *p); 

int xmltp_parser_resultset_end(void *p); 

int xmltp_parser_resultset_begin(void *p); 

int xmltp_parser_colnames_end(void *p); 

int xmltp_parser_colnames_begin(void *p); 

int xmltp_parser_add_colname(void *p, char *colname, char *type_str, char *opt_size);

int xmltp_parser_rows_end(void *p); 

int xmltp_parser_rows_begin(void *p); 

int xmltp_parser_row_elems_end(void *p); 

int xmltp_parser_row_elems_begin(void *p); 

int xmltp_parser_add_row_elem(void *p, char *val, char *is_null);

int xmltp_parser_set_nbrows(void *p, char *nbrows_str);

int xmltp_parser_set_totalrows(void *p, char *totalrows_str);



#endif

/* --------------------------- end of xmltp_parser.h ---------------- */



