/*
 * xmltp_lex.h
 * -----------
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_lex.h,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_lex.h,v 1.5 2003/08/29 21:21:54 toucheje Exp $
 *
 *
 * Custom lexer for the XML-TP syntax
 * ----------------------------------
 *
 * Prototypes for PUBLIC functions
 *
 * ------------------------------------------------------------------------
 * 2001sept10,jft: first version
 * 2002jan08,jft: adapted to let xml2syb uses Bison grammar (with XMLTP_GX defined)
 *		  function xmltp_lex_get_single_thread_context() created when #ifdef XML2DB
 *		  added prototype for int yylex(YYSTYPE *p_lval, void *p_ctx);
 * 2002june14,jft: + prototypes: xmltp_lex_get_name_from_token2_val(), xmltp_lex_get_keyword_from_token_val()
 */

#ifndef _XMLTP_LEX_
#define _XMLTP_LEX_

/* Misc public functions (actually they are used like private!): */

char *xmltp_lex_get_name_from_token2_val(int token_val);
char *xmltp_lex_get_keyword_from_token_val(int token_val);




#ifdef XML2DB
void *xmltp_lex_get_single_thread_context();
/*
 * Called by: 	single thread main program
 *
 */
#endif


#ifdef XMLTP_GX
/* The order of args yylex(YYSTYPE *p_lval, void *p_ctx) is required by the Bison
 * "pure_parser" and YYLEX_PARAM define.
 */
int yylex(YYSTYPE *p_lval, void *p_ctx);
#else
int yylex();
#endif


int yyerror(char *s);



#endif

/* ------------------------------------------ end of xmltp_lex.h --------- */
