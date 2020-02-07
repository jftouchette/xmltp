/*
 * xmltp_parser_gx.c
 * ------------------
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_parser_gx.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_parser_gx.c,v 1.10 2004/10/14 12:18:52 blanchjj Exp $
 *
 *
 * XML-TP Parser -- implementation of "xmltp_parser.h" API
 * -------------------------------------------------------
 *
 * Functions Prefix:	xmltp_parser_XXX()	parser actions functions
 * ----------------				(high-level)
 *
 * This is the multi-threaded implementation used by any GX servers.
 * It uses the Python C API.
 * ------------------------
 *
 * This module implements the "xmltp_parser.h" API.
 *
 * The functions here are the Actions called by the XML-TP parser.
 *
 *
 * WARNING: All types of values (int, double float, string) are stored
 *	    as string in the rowsList of every result set.
 *
 *	    The Python program which is calling the parser will have to
 *	    look in the colNames list of a result set to find what is
 *	    the datatype of a value.
 *	
 *
 * ------------------------------------------------------------------------
 * 2001oct11,jft: prototype, to test the Bison parser
 * 2001oct13,jft: completed xmltp_parser_build_param().
 * 2002feb19,jft: . void *xmltp_parser_build_param(void *p, char *name, char *val, char *attrib);
 *			Now receives p_ctx as first argument.
 * 2002feb18-19,jft: build the response in the PY_RESP_STRUCT of the context.
 * 2002may11,jft: . xmltp_parser_build_param(): added argument char *opt_size
 *		  . xmltp_parser_add_colname(): added argument char *opt_size
 * 2002sep14,jft: . xmltp_parser_add_colname(): do not print type of column if debug level is zero
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Python.h"

#include "alogelvl.h"		/* for ALOG_INFO, ALOG_ERROR	*/

#define	XMLTP_GX	1	/* to include required typedef and prototypes */
#include "xmltp_ctx.h"		/* for xmltp_ctx_XXX() "context" functions */

#include "xmltp_util.h"		/* for xmltp_util_get_datatype_etc_from_attrib() */

#include "xmltp_parser.h"	/* the "xmltp_parser.h"	API definition	*/


/* ------------------------------------ Things not found in #include :  */

#ifdef HPUX
extern	char	*sys_errlist[];
#endif


#define DEBUG_BITMASK_RESPONSE_ON_STDOUT	1
#define DEBUG_BITMASK_DEBUG_PROCCALL		16
#define DEBUG_BITMASK_DEBUG_PROCCALL_FULL	256


/*
 * Module private variables:
 */
static	int	s_debug_bitmask = 0;

static	int	s_b_debug_proccall = 0;

static	int	s_params_ctr = 0;	/* for unit testing */


/* --------------------------------------- PRIVATE FUNCTIONS:  */

static int sf_should_display_response_on_stdout()
{
	return (DEBUG_BITMASK_RESPONSE_ON_STDOUT & s_debug_bitmask);
}


/* ----------------------------------------- PUBLIC FUNCTIONS:  */

#if 0	/* xmltp_parser_abort_parser() is UNUSED */
void xmltp_parser_abort_parser(void *p_ctx, int rc, char *msg1, char *msg2)
/*
 * Used by the parser to tell the main program that too many
 * parse errors have occurred, that the XML-TP protocol is not
 * followed, and, that it should stop parsing and drop the connection.
 *
 * The explanations about the error conditions are in the strings
 * msg1, and, msg2.
 */
{
	fprintf(stderr, "@@ Prototype version, we abort the program. Error %d, %s %s\n",
		rc, msg1, msg2);
	exit(22);
}
#endif


void xmltp_parser_set_debug_bitmask(int val)
/*
 * Set s_debug_bitmask in the xmltp_parser_xxx.c module
 */
{
	s_debug_bitmask = val;

	if (val & DEBUG_BITMASK_DEBUG_PROCCALL) {
		s_b_debug_proccall = 1;
	}
	if (val & DEBUG_BITMASK_DEBUG_PROCCALL_FULL) {
		s_b_debug_proccall = 10;
	}

} /* end of xmltp_parser_set_debug_bitmask() */



/* --------------------------------------- PARSER (action) functions: */


/*
 * ---------------------- Actions used in parsing of XMLTP proc call:
 */


int xmltp_parser_proc_call_parse_end(void *p)
/*
 * Calls the proc defined in the context.
 *
 */
{
	if (s_b_debug_proccall >= 1) {
		printf("xmltp_parser_proc_call_parse_end(p=%x), %d params\n",
			p,
			s_params_ctr );
	}

	/* Nothing special to do here.
	 * The parser will return.
	 * Then the module wrapper should retrieve the procName and py_params_list
	 * from the p_ctx and give all this back to the Python interpreter.
	 */
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
	if (s_b_debug_proccall >= 1) {
		printf("xmltp_parser_set_proc_name(p=%x, proc='%.80s')\n",
			p,
			proc_name);
	}

#if DEF_DEBUG_LEVEL >= 5
	xmltp_ctx_write_log_mm(ALOG_INFO, 0, "XML proc name:", proc_name);
#endif

	xmltp_ctx_assign_proc_name_clear_old_params(p, proc_name);

	s_params_ctr = 0;	/* for debug */

	return (0);

} /* end of xmltp_parser_set_proc_name() */



/*
 * --------- Actions used in parsing of params in *BOTH* proc call AND response:
 */


static int sf_must_capture_params(void *p_ctx)
/*
 * Called by:	xmltp_parser_add_param_to_list()
 *		xmltp_parser_build_param()
 *
 * NOTE: When parsing a response, the procname in the context is used to
 *	 indicate if the params must be captured or not.
 *	 If procname is "", they must not. Otherwise, they should.
 *
 *	 When parsing a proc call, the procname is assigned before the
 *	 params start to be captured.
 *
 *	 Therefore, the function here is compatible with both types of
 *	 parsing, response and proc call.
 */
{
	char	*p_proc_name = NULL;
	
	p_proc_name = xmltp_ctx_get_procname(p_ctx);
	if (NULL == p_proc_name) {
		return (0);
	}
	if (*p_proc_name) {
		return (1);
	}
	return (0);

} /* end of sf_must_capture_params() */





int xmltp_parser_add_param_to_list(void *p, void *p_param)
/*
 * Add p_param to the list (table) of parameters.
 *
 * NOTE: The pseudo-parameter __return_status__ is NOT added!
 *       instead, it will be freed.
 */
{
	char	*p_name = NULL;

	if (s_b_debug_proccall >= 9) {
	  printf("xmltp_parser_add_param_to_list(p=%x, p_param=%x)\n",
		p, p_param);
	}

	if (sf_must_capture_params(p) == 0) {
		return (0);
	}

	if (NULL == p_param) {
		xmltp_ctx_parser_error(p, "xmltp_parser_add_param_to_list():",
			"p_param is NULL. (error in xml2db_create_param_struct() ?)");
		return (-1);
	}

	/* IMPLEMENTATION NOTE:
	 * Maybe JDBC send us the parameter "__return_status__", but, we
	 * do not need it.  The xml2db version of this module checks for this
	 * param name and avoids adding it to the param list.
	 *
	 * The module here does NOT do this filtering.
	 *
	 * Here, we let it go. The Python script will take care of
	 * removing it if it exists.
	 */
	return (xmltp_ctx_add_param_to_list(p, p_param) );

} /* end of xmltp_parser_add_param_to_list() */




void *xmltp_parser_build_param(void *p, char *name, char *val, char *attrib, char *opt_size)
/*
 * Called by:	yyparse()
 *
 * Takes the arguments and build a parameter structure.
 *
 * NOTE: the version here returns a PyObject * (a tuple) casted to a (void *).
 *	like this:
 *
 *	py_new = Py_BuildValue("(ssiiii)", name, val, datatype, is_null, is_output, sz);
 *
 * Return:
 *	(void *)	OK, success (it is a PyObject, a tuple)
 *	NULL		failed, some error occurred
 */
{
	int	datatype  = 0,
		is_null   = 0,
		is_output = 0,
		sz	  = 0,
		rc	  = -1;
	char	*p_sz	  = NULL;
	PyObject	*py_new = NULL;

	if (s_b_debug_proccall >= 9) {
	  if (NULL == opt_size) {
		p_sz = "[NULL]";
	  } else {
		p_sz = opt_size;
	  }
	  printf("\t'%.80s'='%.80s', attrib='%.80s', size=%.30s\n", name, val, attrib, p_sz);
	}

	if (NULL == opt_size) {
		sz = 0;
	} else {
		sz = atoi(opt_size);
	}

	rc = xmltp_util_get_datatype_etc_from_attrib(attrib,  &datatype,
						      &is_null, &is_output);
	if (rc <= -1) {
		return (NULL);
	}

	if (sf_must_capture_params(p) == 0) {
		return ("DoNotCaptureParams");
	}

	py_new = Py_BuildValue("(ssiiii)", name, val, datatype, is_null, is_output, sz);

	return ( (void *) py_new);

} /* end of xmltp_parser_build_param() */



/*
 * -------------------------- Actions used in parsing of XMLTP response:
 */


static void sf_complain_cannot_get_py_resp_struct(void *p_ctx)
{
 static	int	b_said = 0;
	if (b_said) {
		return;
	}
	fprintf(stderr, "\nERROR: cannot get py_resp_struct from p_ctx=%x !!\n", p_ctx);
	b_said = 1;
}


static void sf_complain_BAD_p_ctx(void *p_ctx, int rc)
{
 static	int	b_said = 0;
	if (b_said) {
		return;
	}
	fprintf(stderr, "\nERROR: p_ctx=%x is bad! rc=%d\n", p_ctx, rc);
	b_said = 1;
}




static int sf_note_data_manipulation_error(void *p_ctx, char *msg, char *func_name)
/*
 * Called by:	many  xmltp_parser_XXX() functions
 */
{
	PY_RESP_STRUCT	*p_resp = NULL;

	p_resp = xmltp_ctx_get_py_resp_struct(p_ctx);

	if (NULL == msg) {
		msg = "NULL msg!";
	}
	if (NULL == func_name) {
		func_name = "NO func_name";
	}
	
	if (sf_should_display_response_on_stdout() ) {
		if (NULL == p_resp) {
			printf("ERROR: %.80s: %.80s, p_ctx=%x, p_resp=NULL\n", func_name, msg, p_ctx);
		} else {
			if (p_resp->ctr_manipulation_errors < 100) {
				printf("ERROR: %.80s: %.80s, p_ctx=%x, p_resp=%x, %d errors.\n",
					func_name, msg,
					p_ctx, 	   p_resp,
					p_resp->ctr_manipulation_errors);
			}
			p_resp->ctr_manipulation_errors++;
		}
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p_ctx);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p_ctx);
		return (0);
	}

	strncpy(p_resp->resp_handle_msg, msg, sizeof(p_resp->resp_handle_msg) );

	xmltp_ctx_parser_error(p_ctx, msg, func_name);
	
	return (0);

} /* end of sf_note_data_manipulation_error() */




static int sf_clean_up_previous_reponse(PY_RESP_STRUCT	*p_resp,
					void		*p_ctx,
					PyObject 	*py_params_list)
/*
 * Called by:	xmltp_parser_response_parse_begin()
 */
{
	int	b_found_res_sets_or_msg = 0;
	char	msg_buff[100] = "[no msg]";

	if (NULL == p_resp->result_sets_list
	 && NULL == p_resp->msg_list
	 && NULL == p_resp->curr_colnames_list
	 && NULL == p_resp->curr_rows_list
	 && NULL == p_resp->curr_row_values_list) {
		b_found_res_sets_or_msg = 0;
	 } else {
		b_found_res_sets_or_msg = 1;
	 }

	if (b_found_res_sets_or_msg || py_params_list != NULL) {
		p_resp->b_found_old_data = 1;
	} else {
		p_resp->b_found_old_data = 0;

		return (0);	/* everything is clean already */
	}

	if (sf_should_display_response_on_stdout() ) {
		printf("*** Found old response objects in p_resp=%x '%.50s'\n",
			p_resp,
			p_resp->old_data_map);
	}
	
	/* Clean-up the old stuff still in p_ctx and p_resp...
	 */

	/* The py_params_list could easily have been left by a partially
	 * parsed proc call which aborted when the client disconnected.
	 * It is safer to just reset it to NULL.
	 * xmltp_ctx_assign_proc_name_clear_old_params() will do that
	 * for us:
	 */
	if (py_params_list != NULL) {
		/* Note: it is OK to make procName "" here. It will be reset later.
		 */
		xmltp_ctx_assign_proc_name_clear_old_params(p_ctx, "");
	}

	if (!b_found_res_sets_or_msg) {		/* no complicated object left, ... */
		return (0);			/* return now */
	}
	
	/* If the previous response was returned to the Python interpreter,
	 * then we must NOT try to Py_DECREF anything!
	 *
	 * Just reset all pointers to NULL.
	 */
	if (p_resp->b_response_returned) {
		p_resp->result_sets_list     = NULL;
		p_resp->curr_colnames_list   = NULL;
		p_resp->curr_rows_list	     = NULL;
		p_resp->curr_row_values_list = NULL;
		p_resp->msg_list	     = NULL;

		return (0);	/* enough clean up for this case */
	}

	sprintf(msg_buff, "xmltp_parser_gx.c: Trying to clean py_resp_struct %x in p_ctx=%x",
			  p_resp,
			  p_ctx);
	
	xmltp_ctx_write_log_mm(ALOG_WARN, 0, msg_buff, p_resp->old_data_map);

	if (sf_should_display_response_on_stdout() ) {
		printf("*** %.80s, %.80s\n",  msg_buff, p_resp->old_data_map);
		fflush(stdout);
	}

	/*
	 * Is Py_XDECREF() enough to really free all the objects within a list?
	 *
	 * We will have to try it to see if it is or not!
	 */	
	if (NULL != p_resp->result_sets_list ) {
		Py_XDECREF(p_resp->result_sets_list);
		p_resp->result_sets_list     = NULL;
	}
	if (NULL != p_resp->curr_colnames_list ) {
		Py_XDECREF(p_resp->curr_colnames_list);
		p_resp->curr_colnames_list   = NULL;
	}
	if (NULL != p_resp->curr_rows_list ) {
		Py_XDECREF(p_resp->curr_rows_list);
		p_resp->curr_rows_list	     = NULL;
	}
	if (NULL != p_resp->curr_row_values_list ) {
		Py_XDECREF(p_resp->curr_row_values_list);
		p_resp->curr_row_values_list = NULL;
	}
	if (NULL != p_resp->msg_list) {
		Py_XDECREF(p_resp->msg_list);
		p_resp->msg_list	     = NULL;
	}
	
	return (0);

} /* end of sf_clean_up_previous_reponse() */




int xmltp_parser_response_parse_begin(void *p)
/*
 * This function is more complex because it tries to clean-up the previous 
 * (incompletely processed?) response that might have been left in
 * PY_RESP_STRUCT.
 */
{
	PY_RESP_STRUCT	*p_resp 	= NULL;
	PyObject 	*py_params_list = NULL;
	int		 rc     	= 0;
	
	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1 ) {
		printf("response begin... p_ctx=%x \n", p);
		fflush(stdout);
	}

	if ((rc = xmltp_ctx_buffer_contains_eot(p)) <= -1) {
		sf_complain_BAD_p_ctx(p, rc);
		return (0);
	}

	/* Check if the previous response was properly completed
	 * and returned to the Python interpreter...
	 */

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	py_params_list = xmltp_ctx_get_py_params_list(p);
	
	/* For diagnosis purposes, write "RcrvMO fscr OK|BOGUS??"
	 * into p_resp->old_data_map
	 *
	 * if (py_params_list != NULL) then 'O' in old_param string.
	 */
	sprintf(p_resp->old_data_map, "%c%c%c%c%c%c %c%c%c%c %.10s",
			p_resp->result_sets_list     != NULL ? 'R' : ' ',
			p_resp->curr_colnames_list   != NULL ? 'c' : ' ',
			p_resp->curr_rows_list	     != NULL ? 'r' : ' ',
			p_resp->curr_row_values_list != NULL ? 'v' : ' ',
			p_resp->msg_list	     != NULL ? 'M' : ' ',
			py_params_list		     != NULL ? 'O' : ' ',
			p_resp->b_found_old_data	     ? 'f' : ' ',
			p_resp->b_parsing_started	     ? 's' : ' ',
			p_resp->b_parsing_completed	     ? 'c' : ' ',
			p_resp->b_response_returned	     ? 'r' : ' ',
			(p_resp->b_response_returned
			 || p_resp->b_parsing_started)       ? "OK" : "BOGUS??"
		);


	/* clean up old stuff first, if any is there:
	 */
	sf_clean_up_previous_reponse(p_resp, p, py_params_list);

	/* If (b_capture_all_results), then the result_sets_list and
	 * msg_list must be created immediately.
	 *
	 * These lists will stay empty if no results and/or no messages are parsed.
	 */
	rc = 0;

	if (xmltp_ctx_get_b_capture_all_results(p) >= 1) {
		/*
		 * NOTE: the procName as "something" means to capture params.
		 */
		xmltp_ctx_assign_proc_name_clear_old_params(p, "!capture params!");

		p_resp->result_sets_list = PyList_New(0);
		p_resp->msg_list	 = PyList_New(0);

		if (NULL == p_resp->msg_list
		 || NULL == p_resp->result_sets_list) {
			xmltp_ctx_parser_error(p, "Failed, PyList_New(0) return NULL",
						  "xmltp_parser_response_parse_begin()");
			rc = -4;
		 }
	} else {
		/*
		 * NOTE: the procName as "" means NOT to capture params.
		 */
		xmltp_ctx_assign_proc_name_clear_old_params(p, "");

		p_resp->result_sets_list = NULL;
		p_resp->msg_list	 = NULL;
	}

	/*
	 * All other list are transient and they will be created when needed.
	 */
	p_resp->curr_colnames_list   = NULL;
	p_resp->curr_rows_list       = NULL;
	p_resp->curr_row_values_list = NULL;

	p_resp->curr_nb_rows  = 0L;

	/* Assign default values to those items:
	 */
	p_resp->total_rows    = 0L;
	p_resp->return_status = -995;	/* it will be set again while parsing */

	/* Remember that parsing has started:
	 */
	p_resp->b_parsing_started = 1;

	/* Clear the error message buffer:
	 */
	p_resp->ctr_manipulation_errors = 0;
	strncpy(p_resp->resp_handle_msg, "", sizeof(p_resp->resp_handle_msg) );
	
	return (rc);	/* might be non-zero, see above */

} /* end of xmltp_parser_response_parse_begin() */




int xmltp_parser_response_parse_end(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout() ) {
		printf("---\n");
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Remember that the parsing of the response was completed:
	 */
	p_resp->b_parsing_completed = 1;

	return (0);

} /* end of xmltp_parser_response_parse_end() */
 



 

int xmltp_parser_set_return_status(void *p, char *val)
/*
 * NOTE: the return status is ALWAYS captured even if b_capture_all_results == 0.
 */
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout() ) {
		printf("return status: %.20s\n", val);
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Capture the value:
	 */
	if (NULL == val) {
		p_resp->return_status = -1;
	} else {
		p_resp->return_status = atoi(val);
	}
	return (0);

} /* end of xmltp_parser_set_return_status() */



int xmltp_parser_outparams_end(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("*** end of outparams.\n");
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Nothing to do. The params are held in p_ctx->py_params_list already.
	 */
	
	return (0);

} /* end of xmltp_parser_outparams_end() */



int xmltp_parser_outparams_begin(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("outparams:\n");
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Nothing to do here.
	 *
	 * The params will be added to p_ctx->py_params_list
	 * while they are parsed.
	 */

	return (0);

} /* end of xmltp_parser_outparams_begin() */

 

int xmltp_parser_msglist_end(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("*** end of msglist\n");
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Nothing to do here for this event.
	 */
	return (0);

} /* end of xmltp_parser_msglist_end() */
 

int xmltp_parser_msglist_begin(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("msglist:\n");
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Nothing to do here for this event.
	 */
	return (0);

} /* end of xmltp_parser_msglist_begin() */

 

int xmltp_parser_add_msg_to_list(void *p, char *msgno_str, char *msg)
{
	PY_RESP_STRUCT	*p_resp = NULL;
	PyObject	*py_new = NULL;
	int		 msg_no = 0;
	int		 rc = 0;

	if (sf_should_display_response_on_stdout() ) {
		printf("msg: %.30s: '%.300s'.\n", msgno_str, msg);
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	if (NULL == msgno_str) {
		msg_no = -1;
	} else {
		msg_no = atol(msgno_str);
	}
	
	if (NULL == msg) {
		msg = "[NULL ptr]";
	}

	if (NULL == p_resp->msg_list) {
		sf_note_data_manipulation_error(p, "p_resp->msg_list is NULL", "xmltp_parser_add_msg_to_list()" );
		return (-4);
	}
	
	py_new = Py_BuildValue("(is)", msg_no, msg);
	if (NULL == py_new) {
		sf_note_data_manipulation_error(p, "Py_BuildValue() of msg failed.", "xmltp_parser_add_msg_to_list()" );
		return (-3);
	}
	
	rc = PyList_Append(p_resp->msg_list, py_new);

	if (-1 == rc) {
		sf_note_data_manipulation_error(p, "PyList_Append() to msg_list failed.", "xmltp_parser_add_msg_to_list()" );
		return (-5);
	}

	return (0);

} /* end of xmltp_parser_add_msg_to_list() */



int xmltp_parser_resultsetslist_end(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("*** end of all results sets\n");
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}
	return (0);

} /* end of xmltp_parser_resultsetslist_end() */
 

int xmltp_parser_resultsetslist_begin(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("results sets:\n");
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Nothing to do here for this event.
	 */

	return (0);

} /* end of xmltp_parser_resultsetslist_begin() */


int xmltp_parser_resultset_end(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;
	PyObject	*py_new = NULL;
	int		 rc = 0;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("** end of result.\n");
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Check a few things before building the resultSet tuple...
	 */
	if (NULL == p_resp->curr_colnames_list) {
		sf_note_data_manipulation_error(p, "Cannot build resultSet: curr_colnames_list is NULL.",
				"xmltp_parser_resultset_end()" );
		return (-3);
	}
	if (NULL == p_resp->curr_rows_list) {
		sf_note_data_manipulation_error(p, "Cannot build resultSet: curr_rows_list is NULL.",
				"xmltp_parser_resultset_end()" );
		return (-3);
	}
	if (NULL == p_resp->result_sets_list) {
		sf_note_data_manipulation_error(p, "Cannot add resultSet: result_sets_list is NULL.",
				"xmltp_parser_resultset_end()" );
		return (-2);
	}

	/* Build the resultSet tuple, then append it to the resultSetsList:
	 */		
	py_new = Py_BuildValue("(OOi)", p_resp->curr_colnames_list, p_resp->curr_rows_list,
					p_resp->curr_nb_rows);
	if (NULL == py_new) {
		sf_note_data_manipulation_error(p, "Py_BuildValue() of a resultSet tuple failed.", 
				"xmltp_parser_resultset_end()" );
		return (-4);
	}
	
	rc = PyList_Append(p_resp->result_sets_list, py_new);

	if (-1 == rc) {
		sf_note_data_manipulation_error(p, "PyList_Append() to result_sets_list failed.", 
				"xmltp_parser_resultset_end()" );
		return (-5);
	}

	/* Manage the reference count of the various objects which are to be
	 * cleared or not to be referenced anymore...
	 */
	Py_DECREF(py_new);
	py_new = NULL;	/* optional as this local variable will go out of scope */

	if (p_resp->curr_colnames_list != NULL) {
		Py_DECREF(p_resp->curr_colnames_list);
		p_resp->curr_colnames_list = NULL;
	}
	if (p_resp->curr_rows_list != NULL) {
		Py_DECREF(p_resp->curr_rows_list);
		p_resp->curr_rows_list = NULL;
	}
	p_resp->curr_nb_rows = 0;
	
	return (0);

} /* end of xmltp_parser_resultset_end() */


 

int xmltp_parser_resultset_begin(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("result:\n");
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Both p_resp->curr_colnames_list and p_resp->curr_rows_list
	 * should be NULL at this point.
	 *
	 * If not, just complain about it but do NOT try to Py_DECREF them,
	 * or anything.
	 */ 
	if (p_resp->curr_colnames_list != NULL) {
		sf_note_data_manipulation_error(p, "curr_colnames_list is NOT NULL!",
					"xmltp_parser_resultset_begin()");
	}
	if (p_resp->curr_rows_list != NULL) {
		sf_note_data_manipulation_error(p, "curr_rows_list is NOT NULL!",
					"xmltp_parser_resultset_begin()");
	}
	p_resp->curr_nb_rows = 0L;
	
	p_resp->curr_colnames_list = PyList_New(0);
	p_resp->curr_rows_list     = PyList_New(0);

	if (NULL == p_resp->curr_colnames_list
	 || NULL == p_resp->curr_rows_list ) {
		sf_note_data_manipulation_error(p, "curr_colnames_list or curr_rows_list could not be created!",
					"xmltp_parser_resultset_begin()");
	}
	
	return (0);

} /* end of xmltp_parser_resultset_begin() */
 

int xmltp_parser_colnames_end(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout() ) {
		printf("\n");
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* everything required was done at the End of the result set
	 */

	return (0);

} /* end of xmltp_parser_colnames_end() */

 

int xmltp_parser_colnames_begin(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("colnames:\n");
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* everything required was done at the Beginning of the result set
	 */

	return (0);

} /* end of xmltp_parser_colnames_begin() */


int xmltp_parser_add_colname(void *p, char *colname, char *type_str, char *opt_size)
{
	PY_RESP_STRUCT	*p_resp = NULL;
	PyObject	*py_new = NULL;
	int		 datatype = 0;
	int		 sz = 0;
	int		 rc = 0;


	if (NULL == opt_size) {
		sz = 0;
	} else {
		sz = atoi(opt_size);
	}

	if (sf_should_display_response_on_stdout() ) {
		if (s_b_debug_proccall >= 1) {
			printf("%10.200s(%.30s, %d) ", colname, type_str, sz);
		} else {
			printf("%10.200s ", colname);
		}
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* First check if the curr_colnames_list is OK:
	 */
	if (NULL == p_resp->curr_colnames_list) {
		sf_note_data_manipulation_error(p, "p_resp->curr_colnames_list is NULL",
				"xmltp_parser_add_colname()" );
		return (-4);
	}

	if (NULL == colname) {
		colname = "[null ptr]";
	}
	if (NULL == type_str) {
		datatype = -99;
	} else {
		datatype = atoi(type_str);
	}

	py_new = Py_BuildValue("(sii)", colname, datatype, sz);
	if (NULL == py_new) {
		sf_note_data_manipulation_error(p, "Py_BuildValue() of a colName element failed.",
				"xmltp_parser_add_colname()" );
		return (-3);
	}

	rc = PyList_Append(p_resp->curr_colnames_list, py_new);

	if (-1 == rc) {
		sf_note_data_manipulation_error(p, "PyList_Append() on curr_colnames_list failed.", 
				"xmltp_parser_add_colname()" );
		return (-5);
	}

	/* The reference count of py_new was incremented by PyList_Append().
	 *
	 * Decrement this other reference because it is now going out of scope:
	 */
	Py_DECREF(py_new);

	py_new = NULL;	/* optional as this local variable will go out of scope */

	return (0);

} /* end of xmltp_parser_add_colname() */




int xmltp_parser_rows_end(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout() ) {
		printf("*\n");
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* everything required was done at the End of the result set
	 */

	return (0);

} /* end of xmltp_parser_rows_end() */
 

int xmltp_parser_rows_begin(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout()  &&  s_b_debug_proccall >= 1) {
		printf("rows:\n");
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* everything required was done at the Beginning of the result set
	 */

	return (0);

} /* end of xmltp_parser_rows_begin() */
 

 
int xmltp_parser_row_elems_end(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;
	int		 rc = 0;

	if (sf_should_display_response_on_stdout() ) {
		printf("\n");
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* Check a few things before adding the current row value to the curr_rows_list...
	 */
	if (NULL == p_resp->curr_rows_list) {
		sf_note_data_manipulation_error(p, "Cannot append to curr_rows_list: it is NULL.",
				  "xmltp_parser_row_elems_end()");
		return (-3);
	}
	if (NULL == p_resp->curr_row_values_list) {
		sf_note_data_manipulation_error(p, "Cannot add row: curr_row_values_list is NULL.",
				  "xmltp_parser_row_elems_end()");
		return (-2);
	}

	rc = PyList_Append(p_resp->curr_rows_list, p_resp->curr_row_values_list);

	if (-1 == rc) {
		sf_note_data_manipulation_error(p, "PyList_Append() to curr_rows_list failed.",
				  "xmltp_parser_row_elems_end()");
		return (-5);
	}

	/* Manage the reference count of the various objects which are to be
	 * cleared or not to be referenced anymore...
	 */
	if (p_resp->curr_row_values_list != NULL) {
		Py_DECREF(p_resp->curr_row_values_list);
		p_resp->curr_row_values_list = NULL;
	}

	return (0);

} /* end of xmltp_parser_row_elems_end() */
 

 
int xmltp_parser_row_elems_begin(void *p)
{
	PY_RESP_STRUCT	*p_resp = NULL;
#if 0
	if (sf_should_display_response_on_stdout() ) {
		printf(" ");
	}
#endif
	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	/* p_resp->curr_row_values_list should be NULL at this point.
	 *
	 * If not, just complain about it but do NOT try to Py_DECREF them,
	 * or anything.
	 */ 
	if (p_resp->curr_row_values_list != NULL) {
		sf_note_data_manipulation_error(p, "curr_row_values_list is NOT NULL!",
					"xmltp_parser_row_elems_begin()");
	}

	p_resp->curr_row_values_list = PyList_New(0);

	if (NULL == p_resp->curr_row_values_list) {
		sf_note_data_manipulation_error(p, "curr_row_values_list could not be created!",
					"xmltp_parser_row_elems_begin()");
	}
		
	return (0);

} /* end of xmltp_parser_row_elems_begin() */


 

int xmltp_parser_add_row_elem(void *p, char *val, char *is_null)
{
	PY_RESP_STRUCT	*p_resp = NULL;
	PyObject	*py_new = NULL;
	int		 b_isNull = 0;
	int		 rc = 0;

	if (NULL == is_null) {
		b_isNull = 1;
	} else {
		b_isNull = atoi(is_null);
	}

	if (sf_should_display_response_on_stdout() ) {
		if (b_isNull == 1) {
			val = "null";
		}
		printf("%10.300s ",  val);
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}
	
	if (NULL == val) {
		val = "[NULL ptr]";
	}

	if (NULL == p_resp->curr_row_values_list) {
		sf_note_data_manipulation_error(p, "Cannot add row elem: p_resp->curr_row_values_list is NULL", 
				"xmltp_parser_add_row_elem()");
		return (-4);
	}

	/* WARNING: All types of values (int, double float, string) are stored
	 *	    as string.
	 *	    The Python program which is calling the parser will have to
	 *	    look in the colNames list to find what is the datatype of a
	 *	    value.
	 */	
	py_new = Py_BuildValue("(si)", val, b_isNull);
	if (NULL == py_new) {
		sf_note_data_manipulation_error(p, "Py_BuildValue() of a row value element failed.",
				"xmltp_parser_add_row_elem()");
		return (-3);
	}
	
	rc = PyList_Append(p_resp->curr_row_values_list, py_new);

	if (-1 == rc) {
		sf_note_data_manipulation_error(p, "PyList_Append() on curr_row_values_list failed.", 
				"xmltp_parser_add_row_elem()");
		return (-5);
	}

	/* The reference count of py_new was incremented by PyList_Append().
	 *
	 * Decrement this other reference because it is now going out of scope:
	 */
	Py_DECREF(py_new);

	py_new = NULL;	/* optional as this local variable will go out of scope */

	return (0);

} /* end of xmltp_parser_add_row_elem() */




int xmltp_parser_set_nbrows(void *p, char *nbrows_str)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout() ) {
		printf("nbrows: %.20s\n", nbrows_str);
	}

	if (xmltp_ctx_get_b_capture_all_results(p) <= 0) {
		return (0);		/* do not capture this here */
	}

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	if (NULL == nbrows_str) {
		p_resp->curr_nb_rows = -1;
	} else {
		p_resp->curr_nb_rows = atol(nbrows_str);
	}

	return (0);

} /* end of xmltp_parser_set_nbrows() */



int xmltp_parser_set_totalrows(void *p, char *totalrows_str)
{
	PY_RESP_STRUCT	*p_resp = NULL;

	if (sf_should_display_response_on_stdout() ) {
		printf("totalrows: %.20s\n", totalrows_str);
	}


	/* Always capture this data, because it is easy.
	 */

	/* First, get the PY_RESP_STRUCT...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p);
	if (NULL == p_resp) {
		sf_complain_cannot_get_py_resp_struct(p);
		return (0);
	}

	if (NULL == totalrows_str) {
		p_resp->total_rows = -1;
	} else {
		p_resp->total_rows = atol(totalrows_str);
	}

	return (0);

} /* end of xmltp_parser_set_totalrows() */




/* --------------------------- end of xmltp_parser_gx.c ---------------- */
