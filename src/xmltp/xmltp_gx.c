/* xmltp_gx.c
 * ----------
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_gx.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_gx.c,v 1.8 2004/10/14 12:18:48 blanchjj Exp $
 *
 * xmltp_gx.c: high-level module to let Python use the XML-TP parser
 * ----------
 *
 * --------------------------------------------------------------------------
 * Versions:
 * 2001oct14,jft: first version
 * 2001nov14,jft: . xmltp_gx_parse_procCall(): return None if Disconnected
 * 2002feb03,jft: + xmltp_gx_parse_response()
 * 2002feb18,jft: . xmltp_gx_parse_procCall(): handle (NULL == py_params_list) more gracefully
 * 2002feb19,jft: . xmltp_gx_parse_response(p_ctx, sd, client_sd, b_capture_all_results): completed.
 *		  . xmltp_gx_parse_procCall(): build py_response, then, if not NULL, Py_XDECREF(py_params_list);
 * 2002feb20,jft: . xmltp_gx_parse_response(): use xmltp_ctx_get_lexer_or_parser_errmsg()
 * 2002may11,jft: . xmltp_gx_parse_response(): does NOT return None when detect the sd disconnected, instead,
 *			always put one more elements at the end of the reponse tuple, the boolean value (int) 
 *			from xmltp_ctx_get_b_eof_disconnect(p_ctx)
 *		  + char *xmltp_gx_get_version_id(int b_rcs_version_id)
 * 2002jun26,jft: . xmltp_gx_parse_response(): new arg, nb_sec
 *			call xmltp_ctx_assign_recv_timeout(p_ctx, nb_sec);
 *			last element of resp tuple contains disc_timeout_data_mask
 * 2002jul05,jft: . xmltp_gx_parse_response(): if (NULL == p_resp->result_sets_list), don't try to return result sets
 * 2002sep14,jft: . commented out #define DEBUG  99
 */

#ifdef DEBUG
#undef DEBUG
#endif
/* #define DEBUG	99
 */

#include <stdlib.h>

#include "Python.h"

#define XMLTP_GX	1	/* to include required prototypes in xmltp_ctx.h */
#include "xmltp_ctx.h"

#include "xmltp_gx.h"  /* our functions prototypes */


#define RCS_VERSION_ID		"RCS $Header: /ext_hd/cvs/xmltp/xmltp_gx.c,v 1.8 2004/10/14 12:18:48 blanchjj Exp $"

#define FILE_DATE_TIME_STRING	(__FILE__ ": " __DATE__ " - " __TIME__)
#define COMPILED_VERSION_ID	(FILE_DATE_TIME_STRING)

/* ----------------------------------------------- Public Functions: */

char *xmltp_gx_get_version_id(int b_rcs_version_id)
/*
 * Called by:	xmltp_gxwrapper.c (Python C module wrapper)
 */
{
	if (b_rcs_version_id) {
		return (RCS_VERSION_ID);
	}
	return (COMPILED_VERSION_ID);
}


PyObject *xmltp_gx_parse_procCall(void *p_ctx, int sd)
/*
 * Called by:	xmltp_gxwrapper.c (Python C module wrapper)
 *
 * args:	p_ctx	must have been created by xmltp_ctx_create_new_context()
 *		previously
 *	sd	this is the fd (file descriptor) of a socket
 *
 * Return:
 * (PyObject *) which can be:
 *!		None	  meaning the socket was disconnected (or EOF)
 *!	      or,
 *		a String, which contains an error message
 *	      or,
 *		a Tuple, containing (3) three elements:
 *			procName (a String)
 *			a List of parameters, each is:
 *				a Tuple, of (5) elements:
 *					name (a String)
 *					value (a String)
 *					datatypeId (int)
 *					isNull (int)
 *					isOutput (int)
 *			bEOTreceived (int), which tell if the 
 *				<EOT/> was found in the buffer 
 *				of the p_ctx.
 */
{
	int		 rc = 0,
			 ctr_err = 0,
			 b_found_eot = 0;
	char		*p_procName = NULL;

	PyObject	*py_params_list = NULL,
			*py_response	= NULL;

	char		 msg_buff[200] = "[ms_buff?]";

	rc = xmltp_ctx_assign_socket_fd(p_ctx, sd);
	if (rc != 0) {
		sprintf(msg_buff, 
			"cannot assign socket %d into parser context (%x). rc=%d\n",
			sd, p_ctx, rc);
		return (Py_BuildValue("s", msg_buff) );
	}

	rc = xmltp_ctx_reset_lexer_context(p_ctx);
	if (rc != 0) {
		sprintf(msg_buff, 
			"failed to reset parser context (%x). rc=%d\n",
			p_ctx, rc);
		return (Py_BuildValue("s", msg_buff) );
	}

	rc = yyparse(p_ctx);

	if (xmltp_ctx_get_b_eof_disconnect(p_ctx) ) {
		/* @@@ more clean-up if Disconnected while parsing?
		 * We should check if xmltp_ctx_get_py_params_list() return NULL or not.
		 * If not, we should Py_XDECREF it...
		 */
		return (Py_BuildValue("") );		/* None indicates Disconnect */
	}

	b_found_eot = xmltp_ctx_buffer_contains_eot(p_ctx);

	/* The rc returned by yyparse() is not very useful to know
	 * if a syntax error has happened or not.
	 * We look at the parser error counter for that:
	 */
	ctr_err = xmltp_ctx_get_lexer_or_parser_errors_ctr(p_ctx);
	if (ctr_err != 0) {
		sprintf(msg_buff, 
			"parsing failed with %d errors. rc=%d, EOT in buffer=%d, p_ctx=%x",
			ctr_err, rc, b_found_eot, p_ctx);
		return (Py_BuildValue("s", msg_buff) );
	}

	/* NOTE: xmltp_ctx_get_py_params_list() reset the reference in the p_ctx,
	 * 	 but, it does NOT do py_DECREF().
	 */
	py_params_list = xmltp_ctx_get_py_params_list(p_ctx);

	p_procName = xmltp_ctx_get_procname(p_ctx);

	if (NULL == p_procName) {
		sprintf(msg_buff, 
			"should not have NULL values. p_procName=%x, py_params_list=%x, p_ctx=%x",
			p_procName, py_params_list, p_ctx);
		return (Py_BuildValue("s", msg_buff) );
	}

	if (NULL == py_params_list) {	/* can be NULL if client disconnects abruptly */
		py_response = Py_BuildValue("(s[]i)",  p_procName, b_found_eot);
	} else {
		py_response = Py_BuildValue("(sOi)",  p_procName, py_params_list, b_found_eot);
	}

	/* Now that the params list is held in the response tuple,
	 * it is time to release it, to py_DECREF it (if is not NULL).
	 */
	if (NULL != py_params_list) {
		Py_XDECREF(py_params_list);
	}

	return (py_response);
	
} /* end of xmltp_gx_parse_procCall() */





PyObject *xmltp_gx_parse_response(void *p_ctx, int sd, int client_sd, int b_capture_all_results, int nb_sec)
/*
 * Called by:	xmltp_gxwrapper.c (Python C module wrapper)
 *
 * args:
 *	p_ctx		must have been created by xmltp_ctx_create_new_context()
 *			previously
 *	sd		fd (file descriptor) of a socket connected to the server
 *	client_sd	fd of a socket connected to the client. If > 0, it will receive
 *			a copy of every byte received from the server (sd).
 *	b_capture_all_results	If true, all results are captured while parsing the response.
 *				If false, only the return status is captured.
 *			nb_sec		number of second before timeout
 *
 * Return:
 * (PyObject *) which can be:
 *		None	  meaning the socket was disconnected (or EOF)
 *	      or,
 *		a String, which contains an error message
 *	      or,
 *		a Tuple, with the various Python objects which hold the response.
 */
{
	PyObject	*py_params_list = NULL,
			*py_response	= NULL;
	PY_RESP_STRUCT	*p_resp       = NULL;
	int		 rc 	      = 0,
			 ctr_err      = 0,
			 b_found_eot  = 0,
			 client_errno = 0;
	int		 ctr_err_ctx  = 0;
	int		 disc_timeout_data_mask = 0;
	char		*p_msg_socket	     = "",
			*p_msg_clt_socket    = "",
			*p_msg_reset_ctx     = "",
			*p_msg_b_capture_all = "";
	char		*p_parser_msg  = "",
			*p_lexer_msg   = "";
	char		 msg_buff[200] = "";	/* do NOT make this buffer smaller than 200 bytes */

	ctr_err_ctx = 0;

	rc = xmltp_ctx_reset_lexer_context(p_ctx);
	if (rc != 0) {
		p_msg_reset_ctx = "reset";
		ctr_err_ctx++;
	}

	rc = xmltp_ctx_assign_socket_fd(p_ctx, sd);
	if (rc != 0) {
		p_msg_socket = "socket_fd";
		ctr_err_ctx++;
	}

	rc = xmltp_ctx_assign_client_socket_fd(p_ctx, client_sd);
	if (rc != 0) {
		p_msg_clt_socket = "client_socket";
		ctr_err_ctx++;
	}

	rc = xmltp_ctx_assign_b_capture_all_results(p_ctx, b_capture_all_results);
	if (rc != 0) {
		p_msg_b_capture_all = "b_capture";
		ctr_err_ctx++;
	}

	rc = xmltp_ctx_assign_recv_timeout(p_ctx, nb_sec);
	if (rc != 0) {
		p_msg_b_capture_all = "assign_recv_timeout";
		ctr_err_ctx++;
	}


	if (ctr_err_ctx >= 1) {
		sprintf(msg_buff, 
			"Failed to access parser context (%x) %d times (%.15s %.15s %.15s %.15s)",
			p_ctx,
			ctr_err_ctx,
			p_msg_reset_ctx,
			p_msg_socket,
			p_msg_clt_socket,
			p_msg_b_capture_all
			);
		return (Py_BuildValue("s", msg_buff) );
	}
#ifdef DEBUG
	printf("\nabout to call yyparse()\n");
	fflush(stdout);
#endif

	rc = yyparse(p_ctx);

#ifdef DEBUG
	printf("\nAFTER yyparse()\n" );
	fflush(stdout);
#endif

#if 0	/* old behaviour -- changed 2002may11 */
	if (xmltp_ctx_get_b_eof_disconnect(p_ctx) ) {
		/*
		 * if Disconnected while parsing, the clean-up of the response data in the
		 * p_ctx will be done the next time that this p_ctx is used to parse
		 * a response.
		 */
		return (Py_BuildValue("") );		/* None indicates Disconnect */
	}
#endif

	b_found_eot = xmltp_ctx_buffer_contains_eot(p_ctx);

	/* The rc returned by yyparse() is not very useful to know
	 * if a syntax error has happened or not.
	 * We look at the parser error counter for that:
	 */
	ctr_err = xmltp_ctx_get_lexer_or_parser_errors_ctr(p_ctx);
	if (ctr_err != 0) {
		sprintf(msg_buff, 
			"%d parse errors, rc=%d, EOT in buffer=%d, p_ctx=%x",
			ctr_err, rc, b_found_eot, p_ctx);
		/*
		 * Do NOT return here. The message will be returned to Python later.
		 */
	}

	/* Check if the client has received the response without problem:
	 */
	if (client_sd > 0) {
		client_errno = xmltp_ctx_get_client_errno(p_ctx);
	} else {
		client_errno = 0;
	}

	/* Get the response from the parser context...
	 */
	p_resp = xmltp_ctx_get_py_resp_struct(p_ctx);
	if (NULL == p_resp) {
		sprintf(msg_buff, 
			"Failed to get the PY_RESP_STRUCT from the parser context (%x). rc=%d\n",
			p_ctx, rc);
		return (Py_BuildValue("s", msg_buff) );
	}

	/* py_params_list is the list of Output params. It could be empty (== NULL).
	 *
	 * xmltp_ctx_get_py_params_list() reset the reference in the p_ctx,
	 * but, it does NOT do py_DECREF().
	 */
	py_params_list = xmltp_ctx_get_py_params_list(p_ctx);

	/* Get parser and lexer errmsg fron the context, if any.
	 */
	p_parser_msg = xmltp_ctx_get_lexer_or_parser_errmsg(p_ctx, 1);
	p_lexer_msg  = xmltp_ctx_get_lexer_or_parser_errmsg(p_ctx, 2);

	/* disc_timeout_data_mask indicates if:
	 *		1 :	disconnect
	 *		2 : timeout
	 *		4 : response already begun
	 */
	disc_timeout_data_mask = xmltp_ctx_get_b_eof_disconnect(p_ctx);
	if (disc_timeout_data_mask >= 1
	 && xmltp_ctx_get_total_bytes_received(p_ctx) ) {
		disc_timeout_data_mask |= 4;	/* indicates response had begun already */
	}
	/*
	 * There are (3) ways to build the response tuple:
	 *	with only the returnStatus	(when b_capture_all_results == 0)
	 *	with results sets list and msg list, but no output params
	 *	with all three lists.
	 */
	if (!b_capture_all_results || NULL == p_resp->result_sets_list) {
		py_response = Py_BuildValue("([][][]iiiiiiisssssi)",  0,  /* total rows == 0 */
					        p_resp->return_status,
					        b_found_eot,
					        client_errno,
					        rc,
					        ctr_err,
					        p_resp->ctr_manipulation_errors,
					        p_resp->old_data_map,
					        p_resp->resp_handle_msg,
						p_parser_msg,
						p_lexer_msg,
					        msg_buff,
						disc_timeout_data_mask);

	} else if (NULL == py_params_list) {
		py_response = Py_BuildValue("(O[]Oiiiiiiisssssi)", p_resp->result_sets_list,
					        p_resp->msg_list,
					        p_resp->total_rows,
					        p_resp->return_status,
					        b_found_eot,
					        client_errno,
					        rc,
					        ctr_err,
					        p_resp->ctr_manipulation_errors,
					        p_resp->old_data_map,
					        p_resp->resp_handle_msg,
						p_parser_msg,
						p_lexer_msg,
					        msg_buff,
						disc_timeout_data_mask);
	} else {
		py_response = Py_BuildValue("(OOOiiiiiiisssssi)", p_resp->result_sets_list,
					        py_params_list,
					        p_resp->msg_list,
					        p_resp->total_rows,
					        p_resp->return_status,
					        b_found_eot,
					        client_errno,
					        rc,
					        ctr_err,
					        p_resp->ctr_manipulation_errors,
					        p_resp->old_data_map,
					        p_resp->resp_handle_msg,
						p_parser_msg,
						p_lexer_msg,
					        msg_buff,
						disc_timeout_data_mask);
	}

	/* Now that the various lists are held in the response tuple,
	 * it is time to released them from the PY_RESP_STRUCT,
	 * to py_DECREF each which is not NULL.
	 */
	if (NULL != py_params_list) {
		Py_XDECREF(py_params_list);
	}
	if (NULL != p_resp->result_sets_list ) {
		Py_XDECREF(p_resp->result_sets_list);
		p_resp->result_sets_list     = NULL;
	}
	if (NULL != p_resp->msg_list) {
		Py_XDECREF(p_resp->msg_list);
		p_resp->msg_list	     = NULL;
	}

	/* NOTE: other lists held in the  PY_RESP_STRUCT should have already
	 *	 been cleared:
	 *		curr_colnames_list
	 *		curr_rows_list
	 *		curr_row_values_list
	 *	We do NOT do anything with them here.
	 */

	/* Now, we indicate in the PY_RESP_STRUCT of the parser context
	 * that we are returning the various Python objects to the
	 * interpreter:
	 */	
	p_resp->b_response_returned = 1;

	return (py_response);
				        
} /* end of xmltp_gx_parse_response() */


/* -------------------------------------- end of xmltp_gx.c ----------- */

