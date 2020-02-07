/*
 * xmltp_parser_2db.c
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_parser_2db.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_parser_2db.c,v 1.6 2004/10/14 12:18:51 blanchjj Exp $
 *
 *
 * XML-TP Parser -- implementation of "xmltp_parser.h" API
 * -------------------------------------------------------
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
 *   The implementation here is to be used within a "XML-TP to Database
 *   connector" program (daemon).
 *
 * ------------------------------------------------------------------------
 * 2001sept19,jft: extracted from module xmltp_parser.c (split)
 * 2002jan13,jft: added STUB functions for actions used in parsing the response
 * 2002feb19,jft: . void *xmltp_parser_build_param(void *p, char *name, char *val, char *attrib);
 *			Now receives p_ctx as first argument.
 * 2002jun03,jbl: Added param opt_size to xml2db_create_param_struct.
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



/* --------------------------------------- PARSER (action) functions: */

static	int	s_params_ctr = 0;	/* @@@ for unit testing */

static	int	s_b_debug_print = 0;


int xmltp_parser_proc_call_parse_end(void *p)
/*
 * Calls the proc defined in the context.
 *
 */
{
#if 0
	if (s_b_debug_print >= 1) {
		printf("xmltp_parser_proc_call_parse_end(p=%x), %d params\n",
			p,
			s_params_ctr );
	}
#endif
	xmltp_ctx_call_proc_in_context(p);

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
#if 0
	if (s_b_debug_print >= 1) {
		printf("xmltp_parser_set_proc_name(p=%x, proc='%.80s')\n",
			p,
			proc_name);
	}
#endif
	/* xmltp_ctx_write_log_mm(ALOG_INFO, 0, "XML proc name:", proc_name); */

	xmltp_ctx_assign_proc_name_clear_old_params(p, proc_name);

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

#if 0
	if (s_b_debug_print >= 9) {
	  printf("xmltp_parser_add_param_to_list(p=%x, p_param=%x)\n",
		p, p_param);
	}
#endif
	if (NULL == p_param) {
		xmltp_ctx_parser_error(p, "xmltp_parser_add_param_to_list():",
			"p_param is NULL. (error in xml2db_create_param_struct() ?)");
		return (-1);
	}
	/* Maybe JDBC send us the parameter "__return_status__", but, we
	 * do not need it.  If this is this one, free p_param and do NOT
	 * add it to the list.
	 */
	p_name = xml2db_get_name_of_param(p_param);
	if (p_name != NULL
	 && !strcmp(p_name, "__return_status__") ) {
		xml2db_free_param_struct(p_param);
		return (0);
	}
	return (xmltp_ctx_add_param_to_list(p, p_param) );

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

#if 0
	if (s_b_debug_print >= 9) {
	  printf("\t'%.80s'='%.80s', attrib='%.80s'\n", name, val, attrib);
	}
#endif
	rc = xmltp_util_get_datatype_etc_from_attrib(attrib,  &datatype,
						      &is_null, &is_output);
	if (rc <= -1) {
		return (NULL);
	}

	p_new = xml2db_create_param_struct(name, datatype, 
					   is_null, is_output,
					   val, opt_size);
	return (p_new);

} /* end of xmltp_parser_build_param() */


/*
 * Actions used in parsing of XMLTP response:
 */
int xmltp_parser_response_parse_end(void *p)
{
	return (-1);
}
 

int xmltp_parser_response_parse_begin(void *p)
{
	return (-1);
}
 

int xmltp_parser_set_return_status(void *p, char *val)
{
	return (-1);
}


int xmltp_parser_outparams_end(void *p)
{
	return (-1);
}
 

int xmltp_parser_outparams_begin(void *p)
{
	return (-1);
}
 

int xmltp_parser_msglist_end(void *p)
{
	return (-1);
}
 

int xmltp_parser_msglist_begin(void *p)
{
	return (-1);
}
 

int xmltp_parser_add_msg_to_list(void *p, char *msgno_str, char *msg)
{
	return (-1);
}


int xmltp_parser_resultsetslist_end(void *p)
{
	return (-1);
}
 

int xmltp_parser_resultsetslist_begin(void *p)
{
	return (-1);
}
 

int xmltp_parser_resultset_end(void *p)
{
	return (-1);
}
 

int xmltp_parser_resultset_begin(void *p)
{
	return (-1);
}
 

int xmltp_parser_colnames_end(void *p)
{
	return (-1);
}
 

int xmltp_parser_colnames_begin(void *p)
{
	return (-1);
}
 

int xmltp_parser_add_colname(void *p, char *colname, char *type_str, char *opt_size)
{
	return (-1);
}


int xmltp_parser_rows_end(void *p)
{
	return (-1);
}
 

int xmltp_parser_rows_begin(void *p)
{
	return (-1);
}
 

int xmltp_parser_row_elems_end(void *p)
{
	return (-1);
}
 

int xmltp_parser_row_elems_begin(void *p)
{
	return (-1);
}
 

int xmltp_parser_add_row_elem(void *p, char *val, char *is_null)
{
	return (-1);
}


int xmltp_parser_set_nbrows(void *p, char *nbrows_str)
{
	return (-1);
}


int xmltp_parser_set_totalrows(void *p, char *totalrows_str)
{
	return (-1);
}




/* --------------------------- end of xmltp_parser_2db.c ---------------- */
