/*
 * xmltp_parser_out.c
 * ------------------
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_parser_out.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_parser_out.c,v 1.4 2004/10/14 12:18:53 blanchjj Exp $
 *
 *
 * XML-TP Parser -- implementation of "xmltp_parser.h" API
 * -------------------------------------------------------
 *
 *
 * WARNING: This module is for UNIT TESTING ONLY.
 * **********************************************
 *
 *
 * Functions Prefix:	xmltp_parser_XXX()	parser actions functions
 * ----------------				(high-level)
 *
 *
 * This module implements the "xmltp_parser.h" API.
 *
 * The functions here are the Actions called by the XML-TP parser.
 *
 * NOTES:
 *   The implementation here is for UNIT TESTING of the response part of
 *   the XMLTP grammar.
 *
 * ------------------------------------------------------------------------
 * 2002jan14,jft: results output on stdout, debug trace (if any) on stderr
 * 2002feb19,jft: . void *xmltp_parser_build_param(void *p, char *name, char *val, char *attrib);
 *			Now receives p_ctx as first argument.
 * 2002may11,jft: . xmltp_parser_build_param(): added argument char *opt_size
 *		  . xmltp_parser_add_colname(): added argument char *opt_size
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alogelvl.h"		/* for ALOG_INFO, ALOG_ERROR	*/

#include "xmltp_ctx.h"		/* for xmltp_ctx_XXX() "context" functions */

#include "xmltp_util.h"		/* for xmltp_util_get_datatype_etc_from_attrib() */

#include "xml2db.h"		/* xml2db_XXX() functions prototypes 	*/

#include "xmltp_parser.h"	/* the "xmltp_parser.h"	API definition	*/


/* ------------------------------------ Things not found in #include :  */

#ifndef LINUX
extern	char	*sys_errlist[];
#endif



/* ----------------------------------------- PUBLIC FUNCTIONS:  */

static	int	s_params_ctr = 0;	/* @@@ for unit testing */

static	int	s_b_debug_print = 0;

int xmltp_parser_assign_debug_level(int val)
{
	s_b_debug_print  = val;

	return (0);
}


/* --------------------------------------- PARSER (action) functions: */



int xmltp_parser_proc_call_parse_end(void *p)
/*
 * Calls the proc defined in the context.
 *
 */
{
	if (s_b_debug_print >= 1) {
		printf("xmltp_parser_proc_call_parse_end(p=%x), %d params\n",
			p,
			s_params_ctr );
	}

	return (0);

} /* end of xmltp_parser_proc_call_parse_end() */



int xmltp_parser_set_proc_name(void *p, char *proc_name)
/*
 * This is the beginning of a new procedure call.
 *
 * This function puts the proc_name in the context and clears
 * any data (params) left by the previous call, if any.
 *
 */
{
	if (s_b_debug_print >= 1) {
		printf("xmltp_parser_set_proc_name(p=%x, proc='%.80s')\n",
			p,
			proc_name);
	}

	s_params_ctr = 0;	/* for debug */

	return (0);

} /* end of xmltp_parser_set_proc_name() */




int xmltp_parser_add_param_to_list(void *p, void *p_param)
/*
 * Add p_param to the list (table) of parameters.
 *
 * NOTE: The pseudo-parameter __return_status__ is NOT added!
 *       instead, it will be freed.
 */
{
	char	*p_name = NULL;

	if (s_b_debug_print >= 5) {
		printf("xmltp_parser_add_param_to_list(p=%x, p_param=%x)\n",
			p, p_param);
	}
	/* STUB */
	
	return (0);

} /* end of xmltp_parser_add_param_to_list() */




void *xmltp_parser_build_param(void *p, char *name, char *val, char *attrib, char *opt_size)
/*
 * Called by:	yyparse()
 *
 * Takes the arguments and build a parameter structure.
 *
 * Return:
 *	(void *)	OK, success
 *	NULL		failed, some error occurred
 */
{
	int	datatype  = 0,
		is_null   = 0,
		is_output = 0,
		rc	  = -1;
	void	*p_new = NULL;

	if (NULL == opt_size) {
		opt_size = "[NULL]";
	}

	if (s_b_debug_print >= 9) {
	  printf("\t'%.80s'='%.80s', attrib='%.80s', size=%.30s\n", name, val, attrib, opt_size);
	}

	rc = xmltp_util_get_datatype_etc_from_attrib(attrib,  &datatype,
						      &is_null, &is_output);
	if (rc <= -1) {
		return (NULL);
	}

	return (NULL);	/* STUB !!! */

} /* end of xmltp_parser_build_param() */


/*
 * Actions used in parsing of XMLTP response:
 */
static	int	 s_ctr_results_sets = 0,
		 s_ctr_columns = 0,
		 s_ctr_rows = 0;
static	void	*s_prev_p_ctx = NULL;

int xmltp_parser_response_parse_end(void *p)
{
	fprintf(stderr, "response end. p=%x \n", p);

	printf("[end response]\n");

	return (0);
}
 

int xmltp_parser_response_parse_begin(void *p)
{
	int rc = 0;
	
	fprintf(stderr, "response begin... p=%x \n", p);

	if ((rc = xmltp_ctx_buffer_contains_eot(p)) <= -1) {
		fprintf(stderr, "BAD p_ctx=%x rc=%d\n", p, rc);
		fprintf(stdout, "BAD p_ctx=%x rc=%d\n", p, rc);
		exit(22);
	}
	s_prev_p_ctx = p;	/* remember the valid context for this response */

	s_ctr_results_sets = 0;
	s_ctr_columns = 0; 
	s_ctr_rows = 0; 

	printf("response:\n");

	return (0);
}


static int sf_validate_context(void *p)
{
	if (p == s_prev_p_ctx) {
		return (0);	/* OK, p_ctx has not changed */
	}

	/* the p_ctx should NOT change for all the duration of one
	 * response.
	 */
	fprintf(stderr, "FATAL ERROR: s_prev_p_ctx=%x NOT EQUAL p=%x\nABORTING.\n",
			 s_prev_p_ctx, p);

	printf("FATAL ERROR: s_prev_p_ctx=%x NOT EQUAL p=%x\n",
			 s_prev_p_ctx, p);
	exit (55);
}

	
 

int xmltp_parser_set_return_status(void *p, char *val)
{
	fprintf(stderr, "return status: %.20s, %x\n", val);
	printf("return status: %.20s\n", val);

	sf_validate_context(p);
	return (0);
}


int xmltp_parser_outparams_end(void *p)
{
	fprintf(stderr, "end of outparams.\n");

	printf("*** end of outparams.\n");
	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_outparams_begin(void *p)
{
	fprintf(stderr, "outparams...\n");

	printf("outparams:\n");
	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_msglist_end(void *p)
{
	fprintf(stderr, "end of msglist.\n");

	printf("*** end of msglist:\n");
	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_msglist_begin(void *p)
{
	fprintf(stderr, "msglist...\n");

	printf("msglist:\n");
	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_add_msg_to_list(void *p, char *msgno_str, char *msg)
{
	fprintf(stderr, "msg: %.30s: '%.300s'.\n", msgno_str, msg);

	printf("msg: %.30s: '%.300s'.\n", msgno_str, msg);

	sf_validate_context(p);
	return (0);
}


int xmltp_parser_resultsetslist_end(void *p)
{
	fprintf(stderr, "end of all %d result sets.\n", s_ctr_results_sets);

	printf("*** end of all %d results sets\n", s_ctr_results_sets);
	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_resultsetslist_begin(void *p)
{
	fprintf(stderr, "results sets...\n");

	printf("results sets:\n");
	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_resultset_end(void *p)
{
	fprintf(stderr, "end of result. %d columns, %d rows.\n",
		s_ctr_columns,
		s_ctr_rows);

	printf("** end of result: %d columns, %d rows.\n",
		s_ctr_columns,
		s_ctr_rows);

	s_ctr_results_sets++;

	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_resultset_begin(void *p)
{
	fprintf(stderr, "result... \n");

	s_ctr_columns = 0; 
	s_ctr_rows = 0; 

	printf("result:\n");

	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_colnames_end(void *p)
{
	fprintf(stderr, "\nend col names.\n");

	printf("\n");
	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_colnames_begin(void *p)
{
	fprintf(stderr, "colnames...\n");

	printf("colnames: ");
	sf_validate_context(p);
	return (0);
}


int xmltp_parser_add_colname(void *p, char *colname, char *type_str, char *opt_size)
{
	int	sz = 0;

	if (NULL == opt_size) {
		opt_size = "[NULL]";
		sz = 0;
	} else {
		sz = atoi(opt_size);
	}

	fprintf(stderr, " '%.200s' (%.30s, %d)", colname, type_str, sz);

	printf("%.200s(%.30s, %d) ", colname, type_str, sz);

	s_ctr_columns++;

	sf_validate_context(p);
	return (0);
}


int xmltp_parser_rows_end(void *p)
{
	fprintf(stderr, "end rows.\n");

	printf("*\n");

	sf_validate_context(p);
	printf("(context validated OK)\n");
	return (0);
}
 

int xmltp_parser_rows_begin(void *p)
{
	fprintf(stderr, "rows...\n");

	printf("rows:\n");

	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_row_elems_end(void *p)
{
	fprintf(stderr, " [end row].\n");

	printf("\n");

	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_row_elems_begin(void *p)
{
	fprintf(stderr, "row: ");

	printf("> ");

	s_ctr_rows++;
	sf_validate_context(p);
	return (0);
}
 

int xmltp_parser_add_row_elem(void *p, char *val, char *is_null)
{
	fprintf(stderr, " '%.300s' (%.30s)", val, is_null);

	printf("%.300s(%.30s) ",  val, is_null);

	sf_validate_context(p);
	return (0);
}


int xmltp_parser_set_nbrows(void *p, char *nbrows_str)
{
	fprintf(stderr, "nbrows: %.20s\n", nbrows_str);
	printf("nbrows: %.20s\n", nbrows_str);

	sf_validate_context(p);
	return (0);
}


int xmltp_parser_set_totalrows(void *p, char *totalrows_str)
{
	fprintf(stderr, "totalrows: %.20s\n", totalrows_str);
	printf("totalrows: %.20s\n", totalrows_str);

	sf_validate_context(p);
	return (0);
}


/* --------------------------- end of xmltp_parser_out.c ---------------- */
