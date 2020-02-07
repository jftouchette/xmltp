/*
 * xmltp_ctx.h
 * -----------
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_ctx.h,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_ctx.h,v 1.9 2003/08/29 21:20:40 toucheje Exp $
 *
 *
 * XMLTP "Context" of parser -- "Context" ~object~
 * -----------------------------------------------
 *
 * Functions Prefixes:
 * ------------------
 *	xmltp_ctx_XXX()	access to Parser Context (an "Abstract DataType;)
 *
 *
 * -----------------------------------------------------------------------------------
 * 2001sept19,jft: extracted xmltp_ctx_XX() functions from xmltp_parser.c (old module)
 * 2002feb18,jft: + added typedef  struct st_py_resp_struct {...} PY_RESP_STRUCT;
 *		  + PY_RESP_STRUCT *xmltp_ctx_get_py_resp_struct(void *p);
 *		  + int xmltp_ctx_get_client_socket(void *p);
 *		  + int xmltp_ctx_get_client_errno(void *p);
 *		  + int xmltp_ctx_assign_client_socket_fd(void *p, int sd);
 * 2002may30,jft: + int xmltp_ctx_send(int sd, char *buff, int n_bytes);
 *		  . each function without any arg now has void as argument list
 * 2003jun05,jft: #ifdef XMLTP_GX_PY controls the conditional compile to use Python objects 
 *		   and functions and PyObject. Typically, to build xmltp_gx.so C/Python module.
 * 2003aug29,jft: #define MAX_ID_LENGTH   512  -- was 32
 *		  #define MAX_ONE_STRING 2048  -- was 512, see comments for explanations.
 */

#ifndef _XMLTP_CTX_H_
#define _XMLTP_CTX_H_


#ifdef XMLTP_GX_PY
/*
 * PY_RESP_STRUCT: While yyparse() process a "response", yyparse() calls 
 *		   various xmltp_parser_XXX() functions.
 *		   These functions will fill the PY_RESP_STRUCT, which
 *		   is part of the XMLTP_PARSER_CONTEXT.
 */
typedef struct st_py_resp_struct {	/* used by xmltp_parser_gx.c */

	PyObject	*result_sets_list,		/* R */
			*curr_colnames_list,		/* c */
			*curr_rows_list,		/* r */
			*curr_row_values_list;		/* v */

	long		 curr_nb_rows;

	PyObject	*msg_list;			/* M */

	long		 total_rows;
	int		 return_status;

	/* internal fields used to check consistency of data manipulation:
	 */
	int		 b_found_old_data,		/* f */
			 b_parsing_started,		/* s */
			 b_parsing_completed,		/* c */
			 b_response_returned;		/* r */

	int		 ctr_manipulation_errors;

	/* if (p_ctx->py_params_list != NULL) then 'O' in old_param string.
	 */
	char		 old_data_map[40], 	/* "RcrvMO fscr" */
			 resp_handle_msg[80],
			 z_fill[4];

 } PY_RESP_STRUCT;
#endif


/* ----------------------------------------- PUBLIC DEFINEs:  */

#define MAX_ID_LENGTH 	 512	/* should be enough to hold a 32 to 64 characters RPC name */
#define MAX_ONE_STRING	2048	/* 2048 allows hundreds of "&amp;" and "&apos;" in a 256 characters string! */

#define XMLTP_CTX_INP_BUFFER_SIZE	1024

#define VALUE_FOLLOWS_IS_NAME		1
#define VALUE_FOLLOWS_IS_ATTRIB		2
#define VALUE_FOLLOWS_IS_STRVAL		3
#define VALUE_FOLLOWS_IS_INTVAL		4
#define VALUE_FOLLOWS_IS_SIZE		5
#define VALUE_FOLLOWS_HAS_ENDED		-999


/* ----------------------------------------- PUBLIC FUNCTIONS:  */

void xmltp_ctx_assign_debug_level(int n);

int xmltp_ctx_get_debug_level(void);


void xmltp_ctx_assign_log_function_pf( void (* pf_func)(
		int err_level, int msg_no, char *msg1, char *msg2)  );
/*
 * Bind address of log function to this module.
 */



void xmltp_ctx_write_log_mm(int err_level, int msg_no, char *msg1, char *msg2);
/*
 * If xmltp_ctx_assign_log_function_pf() has been called before,
 * this function here will call the log function whose address has been
 * assigned to the static function pointer.
 * Else, it will write to stderr.
 */


/* ------------------------- socket functions used by Python code, not related to context: */

int xmltp_ctx_send(int sd, char *buff, int n_bytes);
/*
 * Called by:	xmltp_gx.c
 *
 * This function uses send() to send n_bytes to socket sd. It can and must be called from 
 * Python. 
 *
 * The send() is done within a pair of Py_BEGIN_ALLOW_THREADS, Py_END_ALLOW_THREADS
 * to allow multi-threading activity if the send() blocks (or even if not).
 *
 * Return:
 *	(n)	value returned by send(). See send() man page for details.
 *	-10	sd < 0
 *	-9	buff == NULL
 *	-8	n_bytes <= 0
 *	-5	XMLTP_GX not defined (this function should not be used in this case)
 */



/* ---------------------------------------- CONTEXT functions: */

void *xmltp_ctx_create_new_context(void);


#ifdef XMLTP_GX
void xmltp_ctx_destroy_context(void *p);
/*
 * Called by:	C/Python module wrapper
 *
 * NOTE: XML2DB API programs do NOT need this function and should NOT use it.
 *
 * This function destroy the context pointed by p.
 * This is done like this:
 *	the signature is made non-valid
 *	the pointer is freed: free(p)
 *	NOTE: py_params_list is not manipulated here because its
 *	 ownership must already have been given back to the 
 *	 Python interpreter.
 *
 * Return:	 void
 */
#endif

int xmltp_ctx_buffer_contains_eot(void *p);
/*
 * Called by:	Python wrapper module
 *
 * Return:
 *	-1	bad p_ctx
 *	0	EOT tag is NOT in the buffer of the context
 *	1	EOT found, with (almost) nothing else
 *	n > 1	EOT, but buffer contains (n) bytes
 */


int xmltp_ctx_get_socket(void *p);
/*
 * Return:
 *	n != 0		OK, a socket descriptor (sd)
 *	0		p is not a valid context
 */

#ifdef XMLTP_GX_PY
PY_RESP_STRUCT *xmltp_ctx_get_py_resp_struct(void *p);

int xmltp_ctx_get_client_socket(void *p);
int xmltp_ctx_get_client_errno(void *p);

int xmltp_ctx_assign_client_socket_fd(void *p, int sd);

int xmltp_ctx_assign_b_capture_all_results(void *p, int b_val);
int xmltp_ctx_get_b_capture_all_results(void *p);
#endif

int xmltp_ctx_get_b_eof_disconnect(void *p);

int xmltp_ctx_assign_b_eof_disconnect(void *p, int b_eof);


/* ------------------  Functions used by xmltp_lexer.c or the parser actions: */

int xmltp_ctx_check_lexer_value_ended(void *p);

int xmltp_ctx_check_lexer_value_follows(void *p);

int xmltp_ctx_assign_lexer_value_follows(void *p, int b_follows);

int xmltp_ctx_assign_lexer_last_known_token(void *p, int token);


char *xmltp_ctx_get_lexer_value(void *p);

int xmltp_ctx_assign_lexer_str_value(void *p, char *s);


int xmltp_ctx_reset_lexer_context(void *p);


int xmltp_ctx_lexer_parser_error_ext(void *p, char *msg, char* sval, int b_lexer);

int xmltp_ctx_lexer_error(void *p, char *msg, char* sval);

int xmltp_ctx_parser_error(void *p, char *msg, char* sval);



int xmltp_ctx_assign_socket_fd(void *p, int sd);
/*
 * Return:
 *	0	OK
 *	-1	p_ctx is NULL
 *	-2	bad signature in p_ctx
 */


int xmltp_ctx_get_input_char(void *p);
/*
 * Called by:	xmltp_lexer.c
 *
 *
 * Return:
 *	!= -1	one character out from buffer
 *	== -1	buffer is empty and no other input from socket
 *	== -1	bad p_ctx or strange error
 */


int xmltp_ctx_fill_input_buffer(void *p, char *s);
/*
 * Called by:	unit testing program
 *
 * Fill the input buffer held within the p_ctx struct.
 *
 * Return:
 *	0	OK, success
 *	n >= 1	the string is too long, n is the nb of bytes more than
 *		XMLTP_CTX_INP_BUFFER_SIZE
 *	-1	p is NULL pointer
 *	-2	bad signature in context
 *	-3	s is NULL
 */


/* ---------------------------------------- PUBLIC  Proc Call functions: */


int xmltp_ctx_get_lexer_or_parser_errors_ctr(void *p);
/*
 * Return the number of parser errors, or, if zero of these,
 * the number of lexer errors.
 * Basically, if the parsing was OK (no errors at all), return (0) zero.
 *
 * Return -1 if pointer to context (p) is bad.
 */


char *xmltp_ctx_get_lexer_or_parser_errmsg(void *p, int choice);
/*
 * Return:
 *	"?!"			if pointer to context (p) is bad.
 *	p_ctx->parser_errmsg	if choice is 1
 *	p_ctx->lexer_errmsg	if choice is 2
 *	one or the other	if choice is not (1, 2). The decision
 *				on (p_ctx->parser_errctr > 0).
 */


int xmltp_ctx_assign_proc_name_clear_old_params(void *p, char *proc_name);
/*
 * Called by:	xmltp_parser.c functions
 *
 * Make the context memorize the value of proc_name (copy it).
 *
 * Return:
 *	0	OK
 *	-1	NULL == p
 *	-2	bad signature in context (p)
 *	-3	(NULL == proc_name)
 */



int xmltp_ctx_add_param_to_list(void *p, void *p_param);
/*
 * Called by:	xmltp_parser.c functions
 *
 * Add p_param (the pointer value itself, NOT what it points to)
 * to the list of parameters within the Context.
 *
 * Return:
 *	0	OK
 *	-1	NULL == p
 *	-2	bad signature in context (p)
 *	-3	too many parameters
 */


#ifdef XMLTP_GX_PY
PyObject *xmltp_ctx_get_py_params_list(void *p);
/*
 * Called by:	C/Python module wrapper
 *
 * WARNING: this function should be called ONCE after the ProcCall
 *	 had been parsed completely.
 *	The function here clears p_ctx->py_params_list = NULL before returning
 *	the value of the pointer.
 *
 * NOTES: After this function is called, the module here will assume that
 *	the PyObject reference ownership has been given back to the interpreter.
 *
 * Return:
 *	NULL		if any error, or, if no param in ProcCall
 *	p_ctx->py_params_list
 */
#endif


#ifdef XMLTP_GX
char *xmltp_ctx_get_procname(void *p);
/*
 * Called by:	C/Python module wrapper
 *
 * Return:
 *	NULL		if bad p_ctx
 *	p_ctx->proc_name	NOTE: could be empty string ""
 */
#endif



int xmltp_ctx_call_proc_in_context(void *p);
/*
 * Called by:	xmltp_parser.c functions (Only XML2DB program)
 *
 * Return:
 *	0	OK
 *	-1	NULL == p
 *	-2	bad signature in context (p)
 *	<= -1	some other error
 */

#endif

 
/* --------------------------- end of xmltp_ctx.h ------------------------- */
