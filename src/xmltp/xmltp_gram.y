/* xmltp_gram.y
 * ------------
 *
 *
 *  Copyright (c) 2001-2003 Jean-Francois Touchette
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  (The file "COPYING" or "LICENSE" in a directory above this source file
 *  should contain a copy of the GNU Lesser General Public License text).
 *  -------------------------------------------------------------------------
 *
 * $Source: /ext_hd/cvs/xmltp/xmltp_gram.y,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_gram.y,v 1.10 2004/11/15 16:33:03 toucheje Exp $
 *
 *
 * Bison grammar for the XML-TP
 * ----------------------------
 *
 * NOTES: . Uses Bison extension %pure_parser
 *	  . this version is re-entrant (be careful if you change anything where p_ctx
 *		or a xmltp_ctx_xxx() function is involved).
 *	  . The data specific to the current executing thread is either on the
 *	    stack (local C variables) or in the "context", to which the p_ctx
 *	    function argument points to.  (See xmltp_ctx.c for details).
 *
 * ------------------------------------------------------------------------
 * 2001oct07,jft: modified to please Bison (fork from xmltp_para.y)
 * 2001oct10,jft: added %pure_parser declaration
 * 2001oct13,jft: #ifdef XMLTP_GX use YYACCEPT; when get a complete procCallPart
 * 2002jan08,jft: added %token  <p_context>	NBROWS	   END_NBROWS
 *			%token  <p_context>	TOTALROWS  END_TOTALROWS
 * 2002jan13,jft: added actions for various xmltpResponse rules and sub-rules,
 * 2002feb19,jft: . yyparse(): pass p_ctx as first arg to xmltp_parser_build_param()
 * 2002may11,jft: added optional <sz>..</sz> within a param and colName
 * 2002jun05,jft: added rule for rowsStruct without any row (empty result set)
 * 2002jun13,jft: handle error token in xmltpStream and procCallPart rules: call xmltp_ctx_parser_error()
 *		  Updated general NOTES at the top of this source file.
 * 2003jan23,jft: All fprintf(stderr, ...) diagnostocs now conditional to #ifdef VERBOSE_STDERR
 * 2003sep06,jft: xmltpCall, xmltpResponse: both now end with EOT token (moved processing of EOT within the grammar)
 *		  remove various #ifdef XMLTP_GX which were never used (not define on cc command line in makefile)
 * 2004feb18,jft: removed all ";" before a "|" to please Bison version 1.875
 * 2004jul15,jft: added returnStatus token type on line 133 (existed already in Sept.2003)
 * 2004nov01,jft: removed line 130, %type <p_context> returnStatus, because returnStatus 's type is defined below (line 134) and Bison 1.875 does not want that
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "xmltp_parser.h"

/* Pass the "context" (void *p_ctx)  to yyparse() and yylex() :
 */
#define YYPARSE_PARAM	p_ctx
#define YYLEX_PARAM	p_ctx
%}

/*
 * Tell Bison that we want a re-entrant parser:
 */
%pure_parser

/*
 * FINALS:
 *
 * XMLVERSION_AND_ENCODING = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>"
 *
 */


%union {
	char	*str_val;
	void	*p_context;
}

/* -------------------------------------- tokens for keywords: */

%token  <p_context>	XMLVERSION_AND_ENCODING
%token  <p_context>	EOT
%token  <p_context>	PROCCALL	 END_PROCCALL
%token  <p_context>	PROC		 END_PROC
%token  <p_context>	PARAM		 END_PARAM
%token  <p_context>	NAME		 END_NAME
%token  <p_context>	ATTR		 END_ATTR
%token  <p_context>	NUM		 END_NUM
%token  <p_context>	DATE		 END_DATE
%token  <p_context>	INT		 END_INT
%token  <p_context>	STR		 END_STR
%token  <p_context>	SZ		 END_SZ

%token  <p_context>	RESPONSE	 END_RESPONSE
%token  <p_context>	RETURNSTATUS	 END_RETURNSTATUS
%token  <p_context>	OUTPARAMS	 END_OUTPARAMS
%token  <p_context>	MSGLIST		 END_MSGLIST
%token  <p_context>	MSG		 END_MSG
%token  <p_context>	RESULTSETSLIST	 END_RESULTSETSLIST
%token  <p_context>	RESULTSET	 END_RESULTSET
%token  <p_context>	COLNAMES	 END_COLNAMES
%token  <p_context>	COL		 END_COL
%token  <p_context>	NBROWS		 END_NBROWS
%token  <p_context>	TOTALROWS	 END_TOTALROWS
%token  <p_context>	ROWS		 END_ROWS
%token  <p_context>	ROW		 END_ROW
%token  <p_context>	ISNULL		 END_ISNULL


/* ----------------------------------------- tokens for non-keywords: */

%token	<str_val>	STRVAL	

/* ---------------------------------------- types of non-terminals: */

%type	<str_val>	value stringValue intValue dateValue floatValue
%type	<str_val>	name  attributes isNull
%type	<str_val>	opt_size

%type	<p_context>	initResponse
%type	<p_context>	response resultSetsList outParams respTrailer
%type	<p_context>	msgList totalrowsStruct returnStatus
%type	<p_context>	initOutParams  paramList
%type	<p_context>	initMsgList  msgListArray 
%type	<p_context>	initResultSetsList  resultSetStructArray
%type	<p_context>	initResultSet initColNames  colNamesStruct rowsStruct
%type	<str_val>	nbrowsStruct 
%type	<p_context>	initRows  rowsArray
%type	<p_context>	initRowElems rowElemList





/* ------------------------------------ Beginning of grammar rules... */

%%
xmltpStream :	  xmltpCall |  xmltpResponse
		| error {
			xmltp_ctx_parser_error(p_ctx, "ERROR", "no initial XMLVERSION tag?");
#ifdef VERBOSE_STDERR
			fprintf(stderr, "\nno initial XMLVERSION tag?\n");
#endif
			};

/* ------------------------------------------------------ PROC call: */

xmltpCall :	  XMLVERSION_AND_ENCODING procCallPart eot_part { YYACCEPT;  };  /* we can leave yyparse() now */ 

procCallPart :	  PROCCALL procCall END_PROCCALL   { 
			xmltp_parser_proc_call_parse_end($1); 
			} 
		| error {
			xmltp_ctx_parser_error(p_ctx, "ERROR", "error in procCallPart");
#ifdef VERBOSE_STDERR
			fprintf(stderr, "\nError in procCallPart\n");
#endif
			};

eot_part:	EOT  {  
#ifdef VERBOSE_STDERR
			fprintf(stderr, "/ngot EOT./n");
#else
			;
#endif
			} ;


/* 2002jan13,jft: paramList is optional in procCall
 */
procCall :	  procNamePart paramList
		| procNamePart ;

procNamePart :	  PROC STRVAL END_PROC { xmltp_parser_set_proc_name( $1, $2 ); } ;

paramList :	  paramStruct		   { ; } 
		| paramList paramStruct    { ; } ;

paramStruct :	  PARAM name value attributes          END_PARAM 
			{ xmltp_parser_add_param_to_list( $1,
					xmltp_parser_build_param(p_ctx, $2, $3, $4, NULL ));
			}
		| PARAM name value attributes opt_size END_PARAM 
			{ xmltp_parser_add_param_to_list( $1,
					xmltp_parser_build_param(p_ctx, $2, $3, $4, $5 ));
			};

name :		  NAME STRVAL END_NAME { $$ = $2; } ;

attributes :	  ATTR STRVAL  END_ATTR  { $$ = $2; } ;

opt_size :	  SZ   STRVAL  END_SZ    { $$ = $2; } ;

value :		  stringValue     { $$ = $1; } 
		| intValue	  { $$ = $1; } 
		| floatValue 	  { $$ = $1; } 
		| dateValue	  { $$ = $1; } ;


/* NOTE: the lexer return all types of values as a STRVAL (string).
 *	 You see this in the four (4) following rules:
 */

stringValue :	  STR STRVAL END_STR { $$ = $2; } ;

intValue :	  INT STRVAL END_INT { $$ = $2; } ;

floatValue :	  NUM STRVAL END_NUM { $$ = $2; } ;

dateValue :	  DATE STRVAL END_DATE { $$ = $2; } ;


/* ------------------------------------------------------ Response/Results: */

xmltpResponse :	  XMLVERSION_AND_ENCODING responsePart eot_part  { YYACCEPT;  /* we can leave yyparse() now */   };

responsePart :	  initResponse  response END_RESPONSE {
			xmltp_parser_response_parse_end($1); 
			};

initResponse :	  RESPONSE  {
			xmltp_parser_response_parse_begin($1); 
			} ;

response :	  resultSetsList outParams respTrailer  { ; }
		| resultSetsList 	   respTrailer  { ; }
		|		 outParams respTrailer  { ; }
		| respTrailer  { ; };


respTrailer :	  msgList totalrowsStruct returnStatus   { ; }
		| msgList 		  returnStatus 	 { ; }
		| 	  totalrowsStruct returnStatus   { ; }
		| returnStatus				 { ; }
		| error returnStatus {
			xmltp_ctx_parser_error(p_ctx, "ERROR", "before returnStatus");
#ifdef VERBOSE_STDERR
			fprintf(stderr, "\nERROR before returnStatus.\n");
#endif
			};


returnStatus :	  RETURNSTATUS intValue END_RETURNSTATUS {
				 xmltp_parser_set_return_status( $1, $2 );
			 } ;

outParams :	  initOutParams  paramList END_OUTPARAMS   {
				xmltp_parser_outparams_end($1); 
			} ;

initOutParams :   OUTPARAMS  {
				xmltp_parser_outparams_begin($1); 
			} ;


msgList :	  initMsgList  msgListArray END_MSGLIST   {
				xmltp_parser_msglist_end($1); 
			} ;

initMsgList :	  MSGLIST    {
				xmltp_parser_msglist_begin($1); 
			} ;


msgListArray :	  msgStruct			 { ; }
		| msgListArray msgStruct  	 { ; };

msgStruct :	  MSG intValue stringValue END_MSG   {
				xmltp_parser_add_msg_to_list( $1, $2, $3);
			};

resultSetsList :  initResultSetsList  resultSetStructArray END_RESULTSETSLIST   {
				xmltp_parser_resultsetslist_end(p_ctx); 
			};

initResultSetsList : RESULTSETSLIST {
				xmltp_parser_resultsetslist_begin($1); 
			};

resultSetStructArray :	  resultSetStruct	   { ; }
			| resultSetStructArray resultSetStruct    { ; };

resultSetStruct : initResultSet colNamesStruct rowsStruct nbrowsStruct END_RESULTSET {
				xmltp_parser_resultset_end(p_ctx); 
			}
		| initResultSet colNamesStruct rowsStruct error  END_RESULTSET {
			xmltp_ctx_parser_error(p_ctx, "ERROR", "before END_RESULTSET");
#ifdef VERBOSE_STDERR
			fprintf(stderr, "\nERROR before END_RESULTSET.\n");
#endif
			};

initResultSet :   RESULTSET {
				xmltp_parser_resultset_begin($1); 
			};


colNamesStruct :  initColNames  colNamesArray END_COLNAMES {
				xmltp_parser_colnames_end($1); 
			};

initColNames :	  COLNAMES  {
				xmltp_parser_colnames_begin($1); 
			} ;

colNamesArray :		  colName
			| colNamesArray colName { ; };

colName :	  COL name intValue          END_COL   {
				xmltp_parser_add_colname( $1, $2, $3, NULL );
			}
		| COL name intValue opt_size END_COL   {
				xmltp_parser_add_colname( $1, $2, $3, $4 );
			};

rowsStruct :	  initRows  rowsArray END_ROWS {
				xmltp_parser_rows_end(p_ctx); 
			} 
		| initRows  	      END_ROWS {
				xmltp_parser_rows_end(p_ctx); 
			} ;

initRows :	  ROWS {
				xmltp_parser_rows_begin($1); 
			} ;

rowsArray :	  row		 { ; }
		| rowsArray row  { ; };

row :	  	  initRowElems rowElemList END_ROW   {
				xmltp_parser_row_elems_end($1); 
			} 
		| initRowElems error END_ROW   {
			xmltp_ctx_parser_error(p_ctx, "ERROR", "before END_ROW");
#ifdef VERBOSE_STDERR
			fprintf(stderr, "\nERROR before END_ROW\n");
#endif
			} ;

initRowElems :	  ROW {
				xmltp_parser_row_elems_begin($1); 
			} ;


rowElemList :	  rowElem	  { ; }
		| rowElemList rowElem  { ; };


rowElem :	  value isNull   {
				xmltp_parser_add_row_elem(p_ctx, $1, $2);
			}
		| error isNull   {
			xmltp_ctx_parser_error(p_ctx, "ERROR", "before isNull");
#ifdef VERBOSE_STDERR
			fprintf(stderr, "\nERROR before isNull.\n");
#endif
			} 
		| value error {
			xmltp_ctx_parser_error(p_ctx, "ERROR", "after value");
#ifdef VERBOSE_STDERR
			fprintf(stderr, "\nERROR after value.\n");
#endif
			} ;

nbrowsStruct :	  NBROWS intValue END_NBROWS   {
				 xmltp_parser_set_nbrows( $1, $2 );
			 } ;

totalrowsStruct : TOTALROWS intValue  END_TOTALROWS   {
				 xmltp_parser_set_totalrows( $1, $2 );
			 } ;

isNull :	  ISNULL STRVAL  END_ISNULL   { $$ = $2; } ;



%%
    /* ---------------------- END of GRAMMAR ---------------------- */



/* ------------------------------------------ end of xmltp_gram.y -------------- */



