/*
 * xmltp_ctx.c
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_ctx.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_ctx.c,v 1.25 2005/10/03 17:55:07 toucheje Exp $
 *
 *
 * XMLTP: "Context" of parser -- "Context" ~object~ or, an ADT (*)
 * -----------------------------------------------------------
 *
 * Functions Prefixes:
 * ------------------
 *		xmltp_ctx_XXX()		access to Parser Context
 *
 *
 * (*) ADT: "Abstract Datatype", a way to encapsulate data using
 *          procedural languages.  Data encapsulation is usually one
 *	    of the most essential features of object-oriented 
 *	    languages. 
 *
 * -----------------------------------------------------------------------------------
 * 2001sept19,jft: extracted xmltp_ctx_XX() functions from xmltp_parser.c (old module)
 * 2002feb13,jft: . sf_recv_from_socket(): fwrite() raw XML on stderr if (s_b_raw_xml_on_stdout)
 * 2002feb18,jft: + struct st_xmltp_parser_context: added PY_RESP_STRUCT py_resp_struct;
 *							  int	 clt_socket;
 *							  int	 clt_errno;
 * 2002may11,jft: + added support for VALUE_FOLLOWS_IS_SIZE, added size_value in XMLTP_PARSER_CONTEXT
 * 2002may30,jft: + int xmltp_ctx_send(int sd, char *buff, int n_bytes) 
 *		  . each function without any arg now has void as argument list
 * 2002jun13,jft: . xmltp_ctx_reset_lexer_context(): also reset p_ctx->proc_name to ""
 * 2002jun23,jft: + int xmltp_ctx_assign_recv_timeout(void *p, int nb_sec)
 *		  + int xmltp_ctx_get_total_bytes_received(void *p)
 * 		  . xmltp_ctx_get_input_char(): if required, do setsockopt(p_ctx->socket, ... SO_RCVTIMEO ...)
 * 2002jun25,jft: sf_recv_from_socket(): xmltp_ctx_assign_b_eof_disconnect(p_ctx, (EAGAIN == errno) ? 2 : 1);
 * 2003mar11,jft: #define DEF_DEBUG_LEVEL  1  -- to reduce verbosity in log when disconnect happens.
 *		  . xmltp_ctx_get_input_char(): last arg "0" in call to xml2db_abort_program() to indicate low trace level.
 * 2003jun05,jbl: Remove include sys/socket, replace recv with sock_recv.
 * 		  move sf_set_sock_timeout to sock_fn (sock_set_timeout)
 * 2003jun05,jft: #ifdef XMLTP_GX_PY controls the conditional compile to use Python objects 
 *		   and functions and PyObject. Typically, to build xmltp_gx.so C/Python module.
 * 2003aug29,jft: new comment for proc_name[MAX_ID_LENGTH] in XMLTP_PARSER_CONTEXT struct
 * 2003sep04,jft: avec trace debug dans sf_recv_from_socket()
 * 2003sep05,jft: sprintf() complet du buffer de recv()
 * 2005sep18,jft: xmltp_ctx_get_input_char(), sf_prepare_recv_timeout(): support timeout > 327 sec.
 * 2005sep28,jft: sf_recv_from_socket(): if (errno == EAGAIN) return (-1); and let xmltp_ctx_get_input_char() take care.
 * 2005sep30,jft: sf_prepare_recv_timeout(): if time exhausted, then: xmltp_ctx_assign_b_eof_disconnect(p_ctx, 2);
 *		  #define DEBUG_SOCKET_TIMEOUT    0
 */

#include <stdio.h>
#include <stdlib.h>	/* for calloc(), free(), ... */
#include <string.h>
#include <unistd.h>	/* for sleep() 	*/
#include <errno.h>
#include <sys/types.h>	/* for the constant used in call to setsockopt() */

#include <time.h>	/* for time(), time_t */

#ifdef XMLTP_GX_PY
#include "Python.h"
#endif

#include "alogelvl.h"	/* for ALOG_INFO, ALOG_ERROR	*/

#ifdef XML2DB
#include "xml2db.h"	/* xml2db_XXX() prototypes -- IDEALLY should be more abstract interface */
			/* probably, call to functions whose addresses have been assigned in */
			/* static pointers here.  To be improved. (jft,2001sept19) */
#endif
#include "xmltp_eot.h"	/* for XMLTP_EOT_TAG	*/


#include "xmltp_ctx.h"	/* our prototypes */


/* ------------------------------------ Things not found in #include :  */

#ifdef HPUX
extern	char	*sys_errlist[];
#endif


#define DEBUG_SOCKET_TIMEOUT	0	/* 0 for Production release -- to printf() stuff to debug timeout on recv() */


/* ------------------------------------------ PRIVATE defines: */

/* Sizes, maximums and limits...
 */
#define MAX_NB_PARAMS		256

#define MAX_SEC_SO_RCVTIMEO	327	/* SO_RCVTIMEO cannot be more than 327 sec.(32700 ms : 16 bits?) */


/* Possible values of p_ctx->curr_state :
 */
#define STATE_IDLE		0
#define STATE_NEW_PROC_CALL	1
#define STATE_ADDING_PARAMS	2
#define STATE_END_OF_PARAMS	3

#define DEBUG_FLAG_TURN_ON_RAW_XML_ON_STDOUT	1001
#define DEBUG_FLAG_TURN_OFF_RAW_XML_ON_STDOUT	1002



/* ------------------------------------------ PRIVATE struct: */

#define	XMLTP_CTX_SIGNATURE	0xFF01f9e9L

/*
 * While the struct XMLTP_PARSER_CONTEXT might appear as primitive,
 * it is designed to simplify the chores of keeping track of
 * memory allocation.  This structure requires very few calloc().
 *
 * MEMORY MANAGEMENT NOTES: 
 *  (0) note (1) applies to single-thread version ONLY.
 *  (1) if p_ctx->nb_params is > 0, then, each non-NULL pointer in tab_p_params[]
 *	should be destroyed by calling xml2db_free_param_struct().
 *  (2) other strings in the context are held directly as buffers (not pointers)
 */
typedef struct st_xmltp_parser_context {

	unsigned long   signature;

	/* fields used to read and buffer the socket:
	 */
	int	 b_eof_disconnect;	/* if true, socket disconnect or EOF */
	int	 socket;
	int	 nb_bytes,
		 curr_pos;

	int	 total_bytes_received;

	int 	 timeout_recv;		/* if 0, then no timeout on recv() */
	time_t	 timeout_begin_tstamp;

	unsigned char	 inp_buffer[XMLTP_CTX_INP_BUFFER_SIZE],
			 x_end[4];	/* full of nul-bytes, simpler string handling in inp_buffer */

	/* if XML-TP results were sent from within a C module in a 
	 * multi-thread program,  the (static) state variables used by
	 * that module would need to be move in here, in a context.
	 * Each thread would have to use one context and the state 
	 * variables within it. 
	 * At this moment, this is not required. (2001oct15,jft)
	 * When a program that uses XMLT-TP is multi-threaded,
	 * the XML-TP results are written by Python.
	 */

	/* fields used by the lexer and parser:
	 */
	int	 curr_state,		/* one of the STATE_xxx values */
		 b_value_follows,	/* state used by lexer */
		 lexer_errctr,
		 parser_errctr,
		 last_known_token,
		 ctr_tokens;

	int	 nb_params;		/* number of params in tab_p_params[] */

#ifdef XML2DB
	void	*tab_p_params[MAX_NB_PARAMS];
#endif
#ifdef XMLTP_GX_PY
	/* The following fields are only used in conjunction with xmltp_parser_gx.c :
	 */
	PyObject	*py_params_list;	/* used when parsing a Proc Call */

	PY_RESP_STRUCT	 py_resp_struct;	/* has a few (PyObject *) holding RESPONSE data */
	int		 b_capture_all_results;	/* if false, only the return status is captured */

	int		 clt_socket;		/* if it is > 0, the data received is sent to it */
	int		 clt_errno;		/* errno when error with send() on clt_socket */
#endif

	char	 proc_name[MAX_ID_LENGTH],	/* this is more than 200 characters, see xmltp_ctx.h */
		 x0[4];

	char	 parser_errmsg[MAX_ONE_STRING],
		 x1[4],
		 lexer_errmsg[MAX_ONE_STRING],
		 x2[4];
	/*
	 * While yyparse() is running, xmltp_ctx_get_lexer_value() returns
	 * the address of one of the following buffers.
	 *
	 * The choice depends on the value of b_value_follows, which is
	 * actually decided according to the previous XML-TP tag.
	 * The relationship between <tag> and b_value_follows is
	 * defined in the table contained in xmltp_keyw.h
	 * (See that file for the definitive relationship).
	 *
	 * The tags in the comments beside each buffer is indicative only,
	 * but it was true on 2001sept04.
	 */
	char 	 name_value[MAX_ONE_STRING],	/* <proc>, <name> */
		 x3[4],
		 attrib_value[MAX_ONE_STRING],	/* <attr>, <isnull> */
		 x4[4],
		 str_value[MAX_ONE_STRING],	/* <str>, <num>, <date>, */
		 x5[4],
		 int_value[MAX_ONE_STRING],	/* <int> */
		 x6[4],
		 size_value[MAX_ONE_STRING],	/* <sz> */
		 x7[4];

} XMLTP_PARSER_CONTEXT;



/* ----------------------------------------- PRIVATE VARIABLES:  */

#ifndef DEF_DEBUG_LEVEL
#define DEF_DEBUG_LEVEL	1	/* if defined, use the one already defined */
#endif
static int     s_debug_level   = DEF_DEBUG_LEVEL;

static int     s_b_raw_xml_on_stdout = 0;

static void (* s_pf_log_function)() = NULL;



/* ----------------------------------------- PRIVATE FUNCTIONS:  */



static int sf_check_signature(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	if (NULL == p) {
		return (-1);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;
	if (XMLTP_CTX_SIGNATURE != p_ctx->signature) {
#if 0
printf("BAD signature (%x) in p_ctx=%x\n", p_ctx->signature, p);
#endif
		return (-2);
	}
	return (0);

} /* end of sf_check_signature() */



static void sf_write_log_debug_ctx(int min_debug, int msg_no, void *p_ctx, char *msg)
{
	char	msg_buff[900] = "[buff?]";
	char	*p_buff = "p_buff???";

	if (xmltp_ctx_get_debug_level() < min_debug) {
		return;
	}
	if (sf_check_signature(p_ctx) != 0) {
		sprintf(msg_buff, "BAD p_ctx=%x, pid=%d", p_ctx, getpid() );
		p_buff = "[bad p_ctx]";
	} else {
		XMLTP_PARSER_CONTEXT *p2 = NULL;
		p2 = (XMLTP_PARSER_CONTEXT *) p_ctx;
		
		p_buff = &p2->inp_buffer[0];
		sprintf(msg_buff, "p_ctx=%x, pid=%d, eof=%d parser_msg=%.200s lexer_msg=%.200s sd=%d nbytes=%d pos=%d last_token=%d, msg=%.200s, buff:", 
			p_ctx, getpid(),
			p2->b_eof_disconnect, p2->parser_errmsg, p2->lexer_errmsg,
			p2->socket, p2->nb_bytes,
			p2->curr_pos,
			p2->last_known_token,
		 	msg);
	}
	xmltp_ctx_write_log_mm(ALOG_INFO, msg_no, msg_buff, p_buff);

} /* end of sf_write_log_debug_ctx() */


/* ----------------------------------------- PUBLIC FUNCTIONS:  */

void xmltp_ctx_assign_debug_level( int  n)
{
	if (DEBUG_FLAG_TURN_ON_RAW_XML_ON_STDOUT == n) {
		s_b_raw_xml_on_stdout = 1;
		return;
	}
	if (DEBUG_FLAG_TURN_OFF_RAW_XML_ON_STDOUT == n) {
		s_b_raw_xml_on_stdout = 0;
		return;
	}

	if (n < 0) {
		n = 0;
	}
	s_debug_level = n;
}

int xmltp_ctx_get_debug_level(void)
{
	return (s_debug_level);
}

void xmltp_ctx_assign_log_function_pf( void (* pf_func)(
		int err_level, int msg_no, char *msg1, char *msg2)  )
/*
 * Bind address of log function to this module.
 */
{
	s_pf_log_function = pf_func;
}


void xmltp_ctx_write_log_mm(int err_level, int msg_no, char *msg1, char *msg2)
/*
 * If xmltp_ctx_assign_log_function_pf() has been called before,
 * this function here will call the log function whose address has been
 * assigned to the static function pointer.
 * Else, it will write to stderr.
 */
{
	if (s_pf_log_function != NULL) {
		(*s_pf_log_function)(err_level, msg_no, msg1, msg2);
		return;
	}
	fprintf(stderr, "errlvl=%d, %d, %.1200s %.1200s\n",
		      err_level, msg_no, msg1, msg2);

} /* end of xmltp_ctx_write_log_mm() */


/* ------------------------- socket functions used by Python code, not related to context: */

int xmltp_ctx_send(int sd, char *buff, int n_bytes)
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
 *	-5	XMLTP_GX_PY not defined (this function should not be used in this case)
 */
{
	int	n = 0;

	if (sd < 0) {
		return (-10);
	}
	if (NULL == buff) {
		return (-9);
	}
	if (n_bytes <= 0) {
		return (-8);
	}

	/* NOTE: the send() is _compiled_ ONLY if XMLTP_GX is defined.
	 */
#ifdef XMLTP_GX_PY

	/*
	 * send() might block, so, enable multi-threading in the Python interpreter:
	 */
	sf_write_log_debug_ctx(5, n_bytes, NULL, "Py_BEGIN_ALLOW_THREADS. About to send()... in xmltp_ctx_send()");
	Py_BEGIN_ALLOW_THREADS

	n = send(sd, (void *) buff, n_bytes, 0);


	Py_END_ALLOW_THREADS
	sf_write_log_debug_ctx(5, n, NULL, "Py_END_ALLOW_THREADS done. send() done. in xmltp_ctx_send()");

	return ( n );
#else
	return (-5);	/* this function should not be used if XMLTP_GX is not defined */

#endif /* XMLTP_GX_PY */

} /* end of xmltp_ctx_send() */





/* ---------------------------------------- CONTEXT functions: */

void *xmltp_ctx_create_new_context(void)
{
	XMLTP_PARSER_CONTEXT *p_new = NULL;
	
	p_new = (XMLTP_PARSER_CONTEXT *) calloc(1, sizeof(XMLTP_PARSER_CONTEXT) );
	if (NULL == p_new) {
		return (NULL);
	}
	p_new->signature = XMLTP_CTX_SIGNATURE;

	return ( (void *) p_new);

} /* end of xmltp_ctx_create_new_context() */



#ifdef XMLTP_GX_PY
void xmltp_ctx_destroy_context(void *p)
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
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return;
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_ctx->signature = -1;
	free (p_ctx);

} /* end of xmltp_ctx_destroy_context() */
#endif



int xmltp_ctx_buffer_contains_eot(void *p)
/*
 * Called by:	Python wrapper module
 *
 * Return:
 *	-1	bad p_ctx
 *	0	EOT tag is NOT in the buffer of the context
 *	1	EOT found, with (almost) nothing else
 *	n > 1	EOT, but buffer contains (n) bytes
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	char	*p_tag = NULL;
	char	*p_eot_tag = XMLTP_EOT_TAG;
	char	*p_curr = NULL;
	int	 buff_len = 0;


	if (sf_check_signature(p) <= -1) {
		return (-1);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_eot_tag++;	/* skip the initial '<' */

	if (p_ctx->curr_pos < 0
	 || p_ctx->curr_pos > sizeof(p_ctx->inp_buffer) ) {	/* sanity check */
		return (-44);	/* abnormal */
	}
	if (p_ctx->curr_pos >= p_ctx->nb_bytes) {
		return (0);	/* empty buffer, no EOT tag */
	}

	p_curr = &p_ctx->inp_buffer[p_ctx->curr_pos];

	p_tag = strstr(p_curr, p_eot_tag);

	if (NULL == p_tag) {
		return (0);	/* not found */
	}
	buff_len = strlen(p_curr);

	if (buff_len > (p_ctx->nb_bytes - p_ctx->curr_pos) ) {
		buff_len = (p_ctx->nb_bytes - p_ctx->curr_pos);
	}

	/* Verify that there are not too many other characters
	 * in the buffer. (A few spaces or \r\n would be OK).
	 */
	if (buff_len > (strlen(XMLTP_EOT_TAG) + 5) ) {
		return (buff_len);
	}
	return (1);	/* OK, found it, and it is clean */

} /* end of xmltp_ctx_buffer_contains_eot() */



#ifdef XMLTP_GX_PY
PY_RESP_STRUCT *xmltp_ctx_get_py_resp_struct(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (NULL);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return ( &(p_ctx->py_resp_struct) );

} /* end of xmltp_ctx_get_py_resp_struct() */



int xmltp_ctx_get_b_capture_all_results(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (0);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return (p_ctx->b_capture_all_results);

} /* end of xmltp_ctx_get_b_capture_all_results() */



int xmltp_ctx_get_client_socket(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (0);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return (p_ctx->clt_socket);

} /* end of xmltp_ctx_get_client_socket() */


int xmltp_ctx_get_client_errno(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return (p_ctx->clt_errno);

} /* end of xmltp_ctx_get_client_errno() */
#endif


int xmltp_ctx_get_socket(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (0);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return (p_ctx->socket);

} /* end of xmltp_ctx_get_socket() */




int xmltp_ctx_get_b_eof_disconnect(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return (p_ctx->b_eof_disconnect);

} /* end of xmltp_ctx_get_b_eof_disconnect() */




int xmltp_ctx_assign_b_eof_disconnect(void *p, int b_eof)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_ctx->b_eof_disconnect = b_eof;

	return (0);

} /* end of xmltp_ctx_assign_b_eof_disconnect() */




int xmltp_ctx_assign_recv_timeout(void *p, int nb_sec)
/*
 * Assign p_ctx->timeout_recv
 *
 * Also, reset p_ctx->timeout_begin_tstamp, like this:
 *
 *		time( &p_ctx->timeout_begin_tstamp );
 *
 * NOTES: 
 * (1) if nb_sec is NOT zero, all calls to recv() will be constrained
 *     by a time limit assigned with setsockopt().
 *     If nb_sec IS zero, then, there is no time limit for recv()
 *     (unless one was already set on the socket prior to the 
 *     call to yyparse() ).
 * (2) For the timeout limit to be applied accurately, your code 
 *     should cal lxmltp_ctx_assign_recv_timeout() just before calling
 *     yyparse(p_ctx). 
 * (3) Your code must call xmltp_ctx_assign_recv_timeout() AFTER calling
 *     xmltp_ctx_reset_lexer_context().
 *
 * Return:
 *	0	OK
 *	-1	p_ctx is NULL or bad signature in p_ctx
 *	-99	this should NOT be called if XML2DB is defined.
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

#ifdef XML2DB
	return (-99);
#else
	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_ctx->timeout_recv = nb_sec;
	time( &(p_ctx->timeout_begin_tstamp) );

	return (0);
#endif

} /* end of xmltp_ctx_assign_recv_timeout() */




int xmltp_ctx_get_total_bytes_received(void *p)
/*
 * Return:
 *	>= 0	total number of bytes received through recv() since reset
 *	-1	p_ctx is NULL or bad signature in p_ctx
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return (p_ctx->total_bytes_received);

} /* end of xmltp_ctx_get_total_bytes_received() */




/* ------------------  Functions used by xmltp_lexer.c or the parser actions: */


int xmltp_ctx_check_lexer_value_ended(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (0);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return (VALUE_FOLLOWS_HAS_ENDED == p_ctx->b_value_follows);

} /* end of xmltp_ctx_check_lexer_value_ended() */



int xmltp_ctx_check_lexer_value_follows(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (0);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	return (   p_ctx->b_value_follows != VALUE_FOLLOWS_HAS_ENDED
		&& p_ctx->b_value_follows != 0);

} /* end of xmltp_ctx_check_lexer_value_follows() */



int xmltp_ctx_assign_lexer_value_follows(void *p, int b_follows)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_ctx->b_value_follows = b_follows;

	return (0);

} /* end of xmltp_ctx_assign_lexer_value_follows() */




int xmltp_ctx_assign_lexer_last_known_token(void *p, int token)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_ctx->ctr_tokens++;
	p_ctx->last_known_token = token;

	return (0);

} /* end of xmltp_ctx_assign_lexer_last_known_token() */




char *xmltp_ctx_get_lexer_value(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	char	buff[90] = "[buff]";

	if (sf_check_signature(p) <= -1) {
		return (NULL);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	switch (p_ctx->b_value_follows) {
	case VALUE_FOLLOWS_IS_NAME:
		return (p_ctx->name_value);

	case VALUE_FOLLOWS_IS_ATTRIB:
		return (p_ctx->attrib_value);

	case VALUE_FOLLOWS_IS_INTVAL:
		return (p_ctx->int_value);

	case VALUE_FOLLOWS_IS_STRVAL:
		return (p_ctx->str_value);

	case VALUE_FOLLOWS_IS_SIZE:
		return (p_ctx->size_value);

	default:
		sprintf(buff, "%d", p_ctx->b_value_follows);	
		xmltp_ctx_lexer_error(p, "cannot get value, strange b_value_follows=", buff);
		break;
	}
	return ("[cannot get value, because unknown b_value_follows]");

} /* end of xmltp_ctx_get_lexer_value() */



int xmltp_ctx_assign_lexer_str_value(void *p, char *s)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	char	buff[90] = "[buff]";

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	if (NULL == s) {
		s = "";
	}

	switch (p_ctx->b_value_follows) {
	case VALUE_FOLLOWS_IS_NAME:
		strncpy(p_ctx->name_value, s, sizeof(p_ctx->name_value) );
		break;
	case VALUE_FOLLOWS_IS_ATTRIB:
		strncpy(p_ctx->attrib_value, s, sizeof(p_ctx->attrib_value) );
		break;
	case VALUE_FOLLOWS_IS_INTVAL:
		strncpy(p_ctx->int_value, s, sizeof(p_ctx->int_value) );
		break;
	case VALUE_FOLLOWS_IS_STRVAL:
		strncpy(p_ctx->str_value, s, sizeof(p_ctx->str_value) );
		break;
	case VALUE_FOLLOWS_IS_SIZE:
		strncpy(p_ctx->size_value, s, sizeof(p_ctx->size_value) );
		break;

	default:
		sprintf(buff, "%d", p_ctx->b_value_follows);	
		xmltp_ctx_lexer_error(p, "cannot assign value, strange b_value_follows=", buff);
		break;
	}
	return (0);

} /* end of xmltp_ctx_assign_lexer_str_value() */



int xmltp_ctx_reset_lexer_context(void *p)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	if ( xmltp_ctx_get_debug_level() >= 10) {
		xmltp_ctx_write_log_mm(ALOG_INFO, (p_ctx->nb_bytes - p_ctx->curr_pos), "reset context (nb_bytes - curr_pos)",
					"xmltp_ctx_reset_lexer_context()" );
	}

	strncpy(p_ctx->proc_name,    "", sizeof(p_ctx->proc_name) );

	strncpy(p_ctx->name_value,   "", sizeof(p_ctx->name_value) );
	strncpy(p_ctx->attrib_value, "", sizeof(p_ctx->attrib_value) );
	strncpy(p_ctx->int_value,    "", sizeof(p_ctx->int_value) );
	strncpy(p_ctx->str_value,    "", sizeof(p_ctx->str_value) );
	sprintf(p_ctx->lexer_errmsg,  "%.80s", "[no error]");
	sprintf(p_ctx->parser_errmsg, "%.80s", "[no error]");
	p_ctx->lexer_errctr  = 0;
	p_ctx->parser_errctr = 0;
	p_ctx->ctr_tokens    = 0;
	p_ctx->last_known_token = 0;
	p_ctx->b_value_follows = 0;

	p_ctx->b_eof_disconnect = 0;

	p_ctx->timeout_recv	    = 0;	/* 0 means: no timeout limit applied */
	p_ctx->timeout_begin_tstamp = 0L;
	p_ctx->total_bytes_received = 0;

	p_ctx->curr_pos = 0;
	p_ctx->nb_bytes = 0;

	return (0);

} /* end of xmltp_ctx_reset_lexer_context() */



int xmltp_ctx_lexer_parser_error_ext(void *p, char *msg, char* sval, int b_lexer)
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		xmltp_ctx_write_log_mm(ALOG_ERROR, b_lexer, msg, sval);
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	if (NULL == msg) {
		msg = "[no error msg]";
	}
	if (NULL == sval) {
		sval = "";
	}

	if (b_lexer) {
		sprintf(p_ctx->lexer_errmsg, "%.80s, %.80s", msg, sval);
		p_ctx->lexer_errctr++;
		sf_write_log_debug_ctx(2,     p_ctx->lexer_errctr, 
				  p_ctx, p_ctx->lexer_errmsg);
	} else {
		sprintf(p_ctx->parser_errmsg, "%.80s, %.80s", msg, sval);
		p_ctx->parser_errctr++;
		sf_write_log_debug_ctx(2,     p_ctx->parser_errctr, 
				  p_ctx, p_ctx->parser_errmsg);
	}
	return (0);

} /* end of xmltp_ctx_lexer_parser_error_ext() */



int xmltp_ctx_lexer_error(void *p, char *msg, char* sval)
{
	return (xmltp_ctx_lexer_parser_error_ext(p, msg, sval, 1) );

} /* end of xmltp_ctx_lexer_error() */



int xmltp_ctx_parser_error(void *p, char *msg, char* sval)
{
	return (xmltp_ctx_lexer_parser_error_ext(p, msg, sval, 0) );

} /* end of xmltp_ctx_parser_error() */




#ifdef XMLTP_GX_PY
int xmltp_ctx_assign_b_capture_all_results(void *p, int b_val)
/*
 * Return:
 *	0	OK
 *	-1	p_ctx is NULL
 *	-2	bad signature in p_ctx
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_ctx->b_capture_all_results = b_val;

	return (0);

} /* end of xmltp_ctx_assign_b_capture_all_results() */



int xmltp_ctx_assign_client_socket_fd(void *p, int sd)
/*
 * Return:
 *	0	OK
 *	-1	p_ctx is NULL
 *	-2	bad signature in p_ctx
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_ctx->clt_socket = sd;
	p_ctx->clt_errno  = 0;		/* reset clt_errno */

	return (0);

} /* end of xmltp_ctx_assign_client_socket_fd() */
#endif



int xmltp_ctx_assign_socket_fd(void *p, int sd)
/*
 * Return:
 *	0	OK
 *	-1	p_ctx is NULL
 *	-2	bad signature in p_ctx
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	p_ctx->socket = sd;

	return (0);

} /* end of xmltp_ctx_assign_socket_fd() */





static int sf_recv_from_socket(int sd, char buff[], int sz_buff, void *p)
/*
 * Called by:	xmltp_ctx_get_input_char()
 *
 * Return:
 *	> 0	n_bytes
 *	-1	recv() failed
 *	-2	recv() failed - "Connection broken by peer"
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	int	n_bytes = 0;
	int	n = 0;
	int	x_error = 0;
	char	debug_msg[700] = "[debug_msg??]";	/* to debug Windows n_bytes == 1 bug */

	if (sf_check_signature(p) <= -1) {
		return (-1);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	while (0 == n_bytes) {
		errno = 0;	/* errno appears to be very sticky, reset it here */

#ifdef XMLTP_GX_PY
		/* enable multi-threading in the Python interpreter
		 * because recv() might block :
		 */
		sf_write_log_debug_ctx(5, 0, p_ctx, "Py_BEGIN_ALLOW_THREADS. About to recv().");
		Py_BEGIN_ALLOW_THREADS
#endif
		if ( xmltp_ctx_get_debug_level() >= 10) {
			xmltp_ctx_write_log_mm(ALOG_INFO, sd, "about to recv()", "sf_recv_from_socket()" );
		}

#ifdef FAKE_RECV
		n_bytes = read(sd, buff, sz_buff);	/* allow to use stdin for unit test */
#else
		n_bytes = sock_recv(sd, buff, sz_buff, 0, &x_error);
#endif
		if ( xmltp_ctx_get_debug_level() >= 10) {
			xmltp_ctx_write_log_mm(ALOG_INFO, n_bytes, "recv() done. (n_bytes)", "sf_recv_from_socket()" );
		}

#ifdef XMLTP_GX_PY
		/* NOTE:
		 * "The C extension must _not_ invoke any functions in the
		 *  Python C API while the lock is released". 
		 * p.318, "Python Essential Reference", 2nd edition.
		 */
		Py_END_ALLOW_THREADS
		sf_write_log_debug_ctx(5, n_bytes, p_ctx, "Py_END_ALLOW_THREADS done. recv() done.");
#endif /* XMLTP_GX_PY */

		if (n_bytes < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN) {	/* the calling function will handle this case */
				return (-1);
			}
			xmltp_ctx_assign_b_eof_disconnect(p_ctx, (EAGAIN == errno) ? 2 : 1);	/* remember if timeout, or if disconnected */

			if ( xmltp_ctx_get_debug_level() >= 3) {
				xmltp_ctx_write_log_mm(ALOG_INFO, errno, 
						(__FILE__ ": recv() failed."),
						(char *) sys_errlist[errno] );
			}
			if (s_b_raw_xml_on_stdout) {
				fprintf(stderr, "\nrecv() failed. Disconnect. (-1)\n");
			}
			return (-1);
		}

		if (n_bytes == 0) {
			xmltp_ctx_assign_b_eof_disconnect(p_ctx, (EAGAIN == errno) ? 2 : 1);	/* remember Disconnected */

			if ( xmltp_ctx_get_debug_level() >= 3) {
				xmltp_ctx_write_log_mm(ALOG_INFO, errno, 
					(__FILE__ ": recv() failed - Connection broken by peer."),
					(char *) sys_errlist[errno] );
			}
			if (s_b_raw_xml_on_stdout) {
				fprintf(stderr, "\nrecv() failed. Disconnect. (-2)\n");
			}
			return (-2);
		}
		/* Received something, n_bytes is > 0 
		 */
		break;
	}
	if (s_b_raw_xml_on_stdout) {
		fwrite(buff, 1, n_bytes, stderr);
	}

	/* NOTE: the send() on p_ctx->clt_socket is only _compiled_ if XMLTP_GX_PY is defined.
	 */
#ifdef XMLTP_GX_PY
	if (n_bytes > 0  &&  p_ctx->clt_socket > 0  &&  0 == p_ctx->clt_errno) {
		/*
		 * send() might block, so, enable multi-threading in the Python interpreter:
		 */
		sf_write_log_debug_ctx(5, n_bytes, p_ctx, "Py_BEGIN_ALLOW_THREADS. About to send()...");
		Py_BEGIN_ALLOW_THREADS

		n = sock_send(p_ctx->clt_socket, (void *) buff, n_bytes, 0, &x_error);

		/* NOTE:  See NOTE near recv() above...
		 */
		Py_END_ALLOW_THREADS
		sf_write_log_debug_ctx(5, n, p_ctx, "Py_END_ALLOW_THREADS done. send() done.");

		if (n != n_bytes) {	/* check if send() failed, client might disconnect */
			if (errno != 0) {
				p_ctx->clt_errno = errno;
			} else {
				p_ctx->clt_errno = -1;
			}
		}
	}
#endif /* XMLTP_GX_PY */

	if ( xmltp_ctx_get_debug_level() >= 20) {
		sprintf(debug_msg, "recv(): n_bytes=%d, bytes=%d, pos=%d, buff[] hex=%02x%02x%02x%02x, buff:", 
			n_bytes,
			p_ctx->nb_bytes,
			p_ctx->curr_pos,
			buff[0], buff[1], buff[2], buff[3]
			);
		if (1 == n_bytes) {
			xmltp_ctx_write_log_mm(ALOG_WARN, sd, debug_msg, buff);
		} else {
			xmltp_ctx_write_log_mm(ALOG_INFO, sd, debug_msg, buff);
		}
	}

	return (n_bytes);

} /* end of sf_recv_from_socket() */



static long sf_calculate_nb_seconds_remaining_for_recv(XMLTP_PARSER_CONTEXT *p_ctx)
/*
 * Called by:	sf_prepare_recv_timeout()
 *
 * Return:
 * 	>= 1	nb of seconds remaining
 *	== 0	timeout elapsed
 * 	<= -1	some error
 */
{
	time_t	t_now;
	time_t	nb_secs_elapsed = 0L;

#ifdef XML2DB
	return (-99L);
#else
	if (p_ctx->timeout_recv <= 0) {		/* timeout on recv() applies only if > 0 */
		return (9999L);
	}

	time( &t_now);

	nb_secs_elapsed = t_now - p_ctx->timeout_begin_tstamp;

	if (nb_secs_elapsed >= p_ctx->timeout_recv) {
		xmltp_ctx_assign_b_eof_disconnect(p_ctx, 2);	/* remember this is a timeout */
		return (0L);			/* timeout limit elapsed already */
	}

	return (p_ctx->timeout_recv - nb_secs_elapsed);
#endif

} /* end of sf_calculate_nb_seconds_remaining_for_recv() */





static int sf_prepare_recv_timeout(XMLTP_PARSER_CONTEXT *p_ctx)
/*
 * Called by:	xmltp_ctx_get_input_char()
 *
 * If required, do setsockopt(p_ctx->socket, ... SO_RCVTIMEO ...)
 *
 * Return:
 * 	0	OK
 * 	<= -1	some error
 */
{
	time_t	t_now;
	long	nb_secs_remaining = 0L;
	int	nb_secs_timeout   = 0;
	int	rc = 0;

#ifdef XML2DB
	return (-99);
#else

	if (p_ctx->timeout_recv <= 0) {		/* timeout on recv() applies only if > 0 */
		return (0);
	}

	nb_secs_remaining = sf_calculate_nb_seconds_remaining_for_recv(p_ctx);

	if (nb_secs_remaining <= 0L) {
#if DEBUG_SOCKET_TIMEOUT  /* DEBUG */
		printf("\ntimeout recv(): nb_secs_elapsed >= p_ctx->timeout_recv.");
#endif /* END DEBUG */
		xmltp_ctx_parser_error(p_ctx, "timeout recv():", "nb_secs_elapsed >= p_ctx->timeout_recv");
		return (-2);
	}

	if (nb_secs_remaining > MAX_SEC_SO_RCVTIMEO) {
		nb_secs_timeout = MAX_SEC_SO_RCVTIMEO;
	} else{
		nb_secs_timeout = nb_secs_remaining;
	}
#if DEBUG_SOCKET_TIMEOUT  /* DEBUG */
	printf("\nnb_secs_remaining=%d, nb_secs_timeout=%d", nb_secs_remaining, nb_secs_timeout);
#endif /* END DEBUG */

	rc = sock_set_timeout(p_ctx->socket, nb_secs_timeout );
	if (rc <= -1) {
		xmltp_ctx_parser_error(p_ctx, "setsockopt() SO_RCVTIMEO failed",
						(char *) sys_errlist[errno] );
		return (rc);
	}
	return (0);
#endif

} /* end of sf_prepare_recv_timeout() */




int xmltp_ctx_get_input_char(void *p)
/*
 * Return:
 *	!= -1	one character out from buffer
 *	== -1	buffer is empty and no other input from socket 
 *		(disconnect or timeout)
 *	== -1	bad p_ctx or strange error
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	int		      c = -1,
			      n = 0,
			      n_bytes = 0;
	int		      rc = 0;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	/* If inp_buffer[] is empty, refill it by reading from socket:
	 */	
	if (p_ctx->curr_pos >= p_ctx->nb_bytes) {
		for (n_bytes = 0; 0 == n_bytes; ) {
#ifndef XML2DB		/* xml2syb, xml2db do blocking recv(), it should not use timeouts */

			rc = sf_prepare_recv_timeout(p_ctx);
			if (rc != 0) {
				return (-1);		/* overall timeout period exhausted, or, bug */
			}
#endif
			n_bytes = sf_recv_from_socket( p_ctx->socket,
					&(p_ctx->inp_buffer[0]),
					 (sizeof(p_ctx->inp_buffer) - 1),	/* avoid dropping last byte */
					 p );

#if DEBUG_SOCKET_TIMEOUT  /* DEBUG */
			printf("\nReceived %d bytes on socket. errno=%d.\n", n_bytes, errno);
#endif /* END DEBUG */
			if (n_bytes <= -1) {		/* disconnect? */
#ifdef XML2DB
				xml2db_abort_program(n_bytes, "socket recv() failed.", "0" ); /* "0" means low trace level */
#else
				if (EAGAIN == errno || EINTR == errno) {  /* timeout on recv() ? */
					n_bytes = 0;
					continue;
				}
#if DEBUG_SOCKET_TIMEOUT  /* DEBUG */
				printf("\nAbout to call xmltp_ctx_parser_error(). errno=%d.\n", errno);
#endif /* END DEBUG */
				xmltp_ctx_parser_error(p, "socket recv() failed.", "0" ); /* "0" means low trace level */
#endif
				return (-1);	/* EOF */
			}
			if (0 == n_bytes) {	/* ??? */
				sleep(1);	/* avoid hard loop */
			} else {
				p_ctx->total_bytes_received += n_bytes;
			}
		}
		/* terminate the string in the buffer with a nul-byte:
		 */
		if (n_bytes >= sizeof(p_ctx->inp_buffer) ) {
			n = (sizeof(p_ctx->inp_buffer) - 1);
		} else {
			n = n_bytes;
		}
		p_ctx->inp_buffer[n] = '\0';
#if 0 /* DEBUG */
		printf("\nBuffer: '%s'.", p_ctx->inp_buffer );
#endif /* END DEBUG */

		/* OK, use the bytes which were received in inp_buffer[]:
		 */
		p_ctx->curr_pos = 0;
		p_ctx->nb_bytes = n_bytes;
	}
	if (p_ctx->curr_pos < 0
	 || p_ctx->curr_pos > sizeof(p_ctx->inp_buffer) ) {	/* sanity check */
		return (-1);	/* abnormal... but we can only return -1 as per spec. */
	}
	c = (unsigned int) p_ctx->inp_buffer[p_ctx->curr_pos];

	p_ctx->curr_pos++;

	return (c);

} /* end of xmltp_ctx_get_input_char() */




int xmltp_ctx_fill_input_buffer(void *p, char *s)
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
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	int		      nb_too_many = 0,
			      slen	  = 0;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	if (NULL == s) {
		return (-3);
	}
	slen = strlen(s);

	if (slen > sizeof(p_ctx->inp_buffer) ) {
		nb_too_many = slen - sizeof(p_ctx->inp_buffer);
	} else {
		nb_too_many = 0;
	}
	strncpy( &(p_ctx->inp_buffer[0]), s, sizeof(p_ctx->inp_buffer) );

	/* The filler x_end[] after inp_buffer[] always contains
	 * nul-bytes because the struct was allocated by calloc()
	 * which initialized all bytes to '\0'
	 */
	p_ctx->curr_pos = 0;
	p_ctx->nb_bytes = slen;

/* DEBUG */
	printf("xmltp_ctx_fill_input_buffer(): slen=%d, nb_too_many=%d\n",
		slen, nb_too_many);
	printf("inp_buff[]='%s'.\n", &(p_ctx->inp_buffer[0]) );
/* END DEBUG */

	return (nb_too_many);

} /* end of xmltp_ctx_fill_input_buffer() */



/* ------------------------------------------ PUBLIC  Proc Call functions: */


int xmltp_ctx_get_lexer_or_parser_errors_ctr(void *p)
/*
 * Return the number of parser errors, or, if zero of these,
 * the number of lexer errors.
 * Basically, if the parsing was OK (no errors at all), return (0) zero.
 *
 * Return -1 if pointer to context (p) is bad.
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return (-1);	/* cannot do much about that */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	if (p_ctx->parser_errctr > 0) {
		return (p_ctx->parser_errctr);
	}
	return (p_ctx->lexer_errctr);

} /* end of xmltp_ctx_get_lexer_or_parser_errors_ctr() */



char *xmltp_ctx_get_lexer_or_parser_errmsg(void *p, int choice)
/*
 * Return:
 *	"?!"			if pointer to context (p) is bad.
 *	p_ctx->parser_errmsg	if choice is 1
 *	p_ctx->lexer_errmsg	if choice is 2
 *	one or the other	if choice is not (1, 2). The decision
 *				on (p_ctx->parser_errctr > 0).
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;

	if (sf_check_signature(p) <= -1) {
		return ("?!");		/* indicates p_ctx is bad */
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	switch (choice) {
	 case 1:		return (p_ctx->parser_errmsg);
	 case 2:		return (p_ctx->lexer_errmsg);
	 default:		break;
	}	
	if (p_ctx->parser_errctr > 0) {
		return (p_ctx->parser_errmsg);
	}
	return (p_ctx->lexer_errmsg);

} /* end of xmltp_ctx_get_lexer_or_parser_errmsg() */




int xmltp_ctx_assign_proc_name_clear_old_params(void *p, char *proc_name)
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
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	void		*p_param = NULL;
	int		 rc = 0,
			 i  = 0;

	rc = sf_check_signature(p);
	if (rc != 0) {
		return (rc);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	if (NULL == proc_name) {
		rc = -3;
#ifdef XML2DB
		xml2db_abort_program(rc, "NULL == proc_name",
			"xmltp_ctx_assign_proc_name_clear_old_params()");
#else
		xmltp_ctx_parser_error(p, "NULL == proc_name",
			"xmltp_ctx_assign_proc_name_clear_old_params()");
#endif
		return (rc);
	}

	strncpy(p_ctx->proc_name, proc_name, sizeof(p_ctx->proc_name) );

	/* Clean-up old parameters:
	 */
#ifdef XML2DB
	if ( xmltp_ctx_get_debug_level() >= 10) {
		xmltp_ctx_write_log_mm(ALOG_INFO, p_ctx->nb_params, "(n) params to clear in xmltp_ctx_assign_proc_name_clear_old_params(), new proc:", proc_name);
	}

	for (i=0; i < p_ctx->nb_params && i < MAX_NB_PARAMS; i++) {
		p_param = p_ctx->tab_p_params[i];
		if (p_param != NULL) {
			xml2db_free_param_struct(p_param);
		}
		p_ctx->tab_p_params[i] = NULL;
	}

	if ( xmltp_ctx_get_debug_level() >= 10) {
		xmltp_ctx_write_log_mm(ALOG_INFO, p_ctx->nb_params, "(n) params cleared by xmltp_ctx_assign_proc_name_clear_old_params(), new proc:", proc_name);
	}
#endif
#ifdef XMLTP_GX_PY
	/* 
	 * Normally, py_params_list is always NULL when we come here.
	 * This is because it is cleared in xmltp_ctx_get_py_params_list().
	 *
	 * Also, the PyObject reference ownership has already be 
	 * given back to the interpreter.
	 *
	 * Therefore, we should not Py_XDECREF(p_ctx->py_params_list) because
	 * the module has already given back the ownership to the 
	 * interpretor.
	 *
	 * So, we simple reset it to NULL.
	 */
	if (p_ctx->py_params_list != NULL) {
		p_ctx->py_params_list = NULL;
	}
#endif
	p_ctx->nb_params = 0;

	return (0);

} /* end of xmltp_ctx_assign_proc_name_clear_old_params() */




int xmltp_ctx_add_param_to_list(void *p, void *p_param)
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
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	int		 rc = 0;

	rc = sf_check_signature(p);
	if (rc != 0) {
		return (rc);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

#ifdef XML2DB
	if (p_ctx->nb_params >= MAX_NB_PARAMS) {
		rc = -3;
		xml2db_abort_program(rc, p_ctx->proc_name,
			"Too many parameters in xmltp_ctx_add_param_to_list()");
		return (rc);
	}
	p_ctx->tab_p_params[p_ctx->nb_params] = p_param;
	p_ctx->nb_params++;

	if ( xmltp_ctx_get_debug_level() >= 10) {
		xmltp_ctx_write_log_mm(ALOG_INFO, p_ctx->nb_params, "param added. (n) params", "xmltp_ctx_add_param_to_list()");
	}
#endif
#ifdef XMLTP_GX_PY
	if (NULL == p_param) {
		xmltp_ctx_parser_error(p, "p_param is NULL",
			"xmltp_ctx_add_param_to_list()");
		return (-3);
	}
		
	if (NULL == p_ctx->py_params_list) {
		p_ctx->py_params_list = PyList_New(0);
		if (NULL == p_ctx->py_params_list) {
			xmltp_ctx_parser_error(p, "Failed, PyList_New(3) return NULL",
				"xmltp_ctx_add_param_to_list()");
			return (-4);
		}
	}
	rc = PyList_Append(p_ctx->py_params_list, (PyObject *) p_param);
	if (-1 == rc) {
		xmltp_ctx_parser_error(p, "PyList_Append() failed.",
			"xmltp_ctx_add_param_to_list()");
		return (-5);
	}

	/* The reference count of p_param was incremented by PyList_Append().
	 *
	 * Decrement this other reference because it is now going out of scope:
	 */
	Py_DECREF((PyObject *) p_param);
#endif
	return (rc);

} /* end of xmltp_ctx_add_param_to_list() */



#ifdef XMLTP_GX_PY
PyObject *xmltp_ctx_get_py_params_list(void *p)
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
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	int	 rc = 0;
	PyObject	*py_list = NULL;

	rc = sf_check_signature(p);
	if (rc != 0) {
		sf_write_log_debug_ctx(1, 0, p_ctx, "xmltp_ctx_get_py_params_list(): BAD p_ctx signature");
		return (NULL);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	if (p_ctx->py_params_list != NULL) {
		py_list = p_ctx->py_params_list;
		p_ctx->py_params_list = NULL;
	} else {
		sf_write_log_debug_ctx(5, p_ctx->b_eof_disconnect, p_ctx, "xmltp_ctx_get_py_params_list(): p_ctx->py_params_list == NULL.");
		return (NULL);
	}
	sf_write_log_debug_ctx(5, (int) py_list, p_ctx, "xmltp_ctx_get_py_params_list() returning p_ctx->py_params_list.");

	return (py_list);

} /* end of xmltp_ctx_get_py_params_list() */
#endif


#ifdef XMLTP_GX
char *xmltp_ctx_get_procname(void *p)
/*
 * Called by:	C/Python module wrapper
 *
 * Return:
 *	NULL		if bad p_ctx
 *	p_ctx->proc_name	NOTE: could be empty string ""
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	int	 rc = 0;

	rc = sf_check_signature(p);
	if (rc != 0) {
		return (NULL);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;
	
	return (p_ctx->proc_name);

} /* end of xmltp_ctx_get_procname() */
#endif








#ifdef XML2DB  /* the following function is only used in a XML2DB program */

int xmltp_ctx_call_proc_in_context(void *p)
/*
 * Called by:	xmltp_parser.c functions
 *
 * Return:
 *	0	OK
 *	-1	NULL == p
 *	-2	bad signature in context (p)
 *	<= -1	some other error
 */
{
	XMLTP_PARSER_CONTEXT *p_ctx = NULL;
	int	 rc = 0,
		 return_status = 0;

	rc = sf_check_signature(p);
	if (rc != 0) {
		return (rc);
	}
	p_ctx = (XMLTP_PARSER_CONTEXT *) p;

	if (xmltp_ctx_get_debug_level() >= 4) {
		xmltp_ctx_write_log_mm(ALOG_INFO, 0, "XML parsing done. About to call:", 
						     p_ctx->proc_name);
	}

	rc = xml2db_exec_proc(NULL,	/* No p_conn here, the function will find one */
			 p_ctx->proc_name, 
			 p_ctx->tab_p_params, p_ctx->nb_params,
			&return_status );

	return (rc); /* ??? */

} /* end of xmltp_ctx_call_proc_in_context() */
#endif


/* --------------------------- end of xmltp_ctx.c ------------------------- */
