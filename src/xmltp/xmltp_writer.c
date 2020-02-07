/*
 * xmltp_writer.c
 * --------------
 *
 *  Copyright (c) 2001-2003 Jean-Francois Touchette and Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_writer.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_writer.c,v 1.16 2005/10/03 17:55:07 toucheje Exp $
 *
 * XMLTP Writer: 	module to write XMLTP to a socket (or fd)
 * ------------
 *
 * Funct Prefixes:	xmltp_writer_XXX()
 * --------------
 *
 * Used by:	xml2syb.c     (and, other xml2db.h daemons) 
 *		ct10api_xml.c (to send the SP call) [not done yet]
 *
 * Usual sequence of XMLTP tags written out:
 * ----------------------------------------- 
 * *** this does NOT show all tags! ***
 *
 * <?xml version="1.0" encoding="ISO-8859-1" ?>
 *    <response>
 *	 <resultSetsList>
 *	    <resultSet>
 *	       <colNames> ... </colNames>
 *	       <rows> ... </rows>
 *	    </resultSet>
 *	    <resultSet> ...  </resultSet>
 *	    ...
 *	 </resultSetsList>
 *	 <outParams> ...  </outParams>
 *	 <msgList> ... </msgList>
 *	 <returnStatus><int>0</int></returnStatus>
 *    </response>
 * <EOT/>
 *
 * NOTES:
 * ------
 * XMLTP follows specific grammar rules. Yet, CT10API (Sybase CT-Lib)
 * and other database API, have their own peculiar sequence for returning
 * results. The sequence does not always explicitly express the end of
 * one type or set of results. Often, the end of the previous type (or set) 
 * of results is known because a new type (or set) of results begins.
 * Therfore, this module keeps some state variables to remember what "sets"
 * (in the general meaning) are open, in other words, which XML tags are
 * opened, without their matching end-tag.
 *
 * PROTOTYPE is NOT re-entrant:
 * ---------------------------
 * The first version use static state variables, but, all public functions
 * have a p_ctx argument, and, the state variables can be put in the 
 * context when a re-entrant version will be required.
 *
 * ------------------------------------------------------------------------
 * 2001sept13,jft: first version
 * 2001sept14,jft: cleaned-up many mistakes,
 *	        NO functions for messages at this moment.
 * 2001sept16,jft: + with output buffer to make fewer calls to send()
 *		   . xmltp_writer_end_response() call sf_begin_xml_response_if_not_done()
 * 2001oct16, jbl: *** support for NBROWS of result set into xml stream ***
 *		   Added: sf_write_nb_rows + call it from xmltp_writer_begin_result_set,
 *		   	  sf_end_rows_if_started, ... + added a nb_rows param to these function.
 *		   Added: nb_rows param to: xmltp_writer_begin_output_params,xmltp_writer_end_response
 *		   *** support for messages ***
 *		   Added: xmltp_writer_msg, xmltp_writer_end_msg, xmltp_writer_begin_msg
 * 2002feb06, jbl: Added: sf_begin_xml_response_if_not_done(sd) to:
 *			xmltp_writer_begin_msg
 *			xmltp_writer_begin_output_params
 * 2002feb06, jbl: Added: 	xmltp_writer_begin_proc_call,
 *				xmltp_writer_add_param
 *				xmltp_writer_end_proc_call
 * 2002jun04, jbl: Added tag SZ if JAVA_SQL_CHAR to
 *			xmltp_writer_column_name(),
 *			xmltp_writer_param_value(),
 *			xmltp_writer_add_param()
 * 2002jul30, jbl: Added: xmltp_encode to escape xml special delimiter.
 * 2003jun06, jbl: Replace send with sock_send
 * 2003jul07, jbl: Replace sock_send with sock_obj_send
 * 2003jul14, jbl: Replace sock_send with sock_obj_send on the second sock_send
 * 2003oct20, jbl: Added: xmltp_writer_set_sock_obj
 * 			For irpc to support multi connection
 * 2004feb17, jbl: Oups, I left the sock_send in place when I added the sock_obj_send 
 * 		   in the special case when a string is to big to fit in the send buffer.
 *		   I removed the sock_send.
 * 2004avr20, jbl: Add support for numeric type.
 * 2004may13, deb: . size tag is returned for JAVA_SQL_VARCHAR
 * 2005sep28, jft: sf_create_sock_obj_if_not_done(): call sock_obj_create_socket_object_from_fd() with hostname "localhost" instead of "DUMMY" 
 *		   this avoids a 30 seconds timeout delay on OS/X (and maybe on other operating systems?)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <time.h>

#include "alogelvl.h"		/* for ALOG_INFO, ALOG_ERROR	*/

#include "javaSQLtypes.h"	/* for JAVA_SQL_CHAR, ...	*/
#include "xml2db.h"		/* for xml2db_abort_program()	*/
#include "xmltp_tags.h"
#include "xmltp_writer.h"
 
#include "sock_obj.h"

/* ------------------------------------ Things not found in #include :  */

#ifndef LINUX
extern	char	*sys_errlist[];
#endif

/* ------------------------------------------ PRIVATE defines: */

#define SEND_BUFFER_SIZE	8192

#define MAX_STRING_SIZE 255
#define ESC_BUF_SIZE	(MAX_STRING_SIZE*4) +1


/* ----------------------------------------- PRIVATE VARIABLES:  */

/* Real module-level private variables:
 */
static	int	s_ctr_socket_write_errors = 0;
static	int	s_b_human_readable	  = 1;	/* for TESTING */

static	char	s_esc_buf[ESC_BUF_SIZE];	/* Buffer use to escape delimiter */

static	void	*s_p_sock_obj=NULL;


/* BEGIN-PROTOTYPE-LAZY-TRICK for state variables
 *
 * The following state variables should be in the context (p_ctx)
 * to make the functions here re-entrant.
 *
 * As these variables have the "static" type of storage (as it is
 * called in C), the functions that use them are NOT re-entrant.
 * Therefore, they cannot be used reliably in a multi-threaded
 * program.
 */
static	int	s_b_xml_response_header_done = 0,
		s_b_results_sets_started  = 0,
		s_b_rows_started          = 0,
		s_b_within_one_row        = 0,
		s_b_output_params_started = 0,
		s_b_messages_started      = 0,
		s_b_end_response_done     = 0,
		s_prev_result_set_no      = -999;

static	time_t	s_t_begin  = 0L;

static	int	s_ctr_errors = 0;
static	char	s_note_errmsg[500] = "";

static	int	s_n_bytes_in_buff = 0;
static	char	s_send_buffer[SEND_BUFFER_SIZE] = "?send_buffer?";
/*
 * END-PROTOTYPE-LAZY-TRICK for state variables
 */


/* ----------------------------------------- PRIVATE FUNCTIONS:  */



int sf_create_sock_obj_if_not_done(int sd) 
/*
 * Create a sock obj from an existing fd in order
 * to use sock_obj_send.
 *
 * return values:
 * 	 0: Success
 * 	-1: Failed
 */
{
	if ( s_p_sock_obj == NULL ) {

		if ((s_p_sock_obj = (void *) sock_obj_create_socket_object_from_fd(
			"localhost",		/* Dummy host name -- was "DUMMY" <*/
			"0000",			/* Dummy service */
                        0,   			/* connect_max_retry            */
                        0,       		/* b_reconnect_on_reset_by_peer */
			sd			/* Socket */
			)) == NULL) {

			xml2db_write_log_msg_mm(ALOG_INFO, 0, "sf_create_sock_obj_if_not_done", "SOCK_OBJ creation error");
			return -1;
		}
	}

	return 0;

} /* End of sf_create_sock_obj_if_not_done */ 



static int sf_flush_send_buffer(int sd)
/*
 * Called by:	sf_write_string()
 */
{
	int	rc = 0;
	int	error=0;

	if (s_n_bytes_in_buff > sizeof(s_send_buffer) ) {
		s_n_bytes_in_buff = sizeof(s_send_buffer);
	}

	/*
	xml2db_write_log_msg_mm(ALOG_INFO, s_n_bytes_in_buff, "flush send() buffer. (n_bytes_in_buff)",
				__FILE__);
	*/
	error = 0;
 
	if (( rc = sf_create_sock_obj_if_not_done(sd)) != 0) {
		return -1;
	}

	rc = sock_obj_send(s_p_sock_obj, s_send_buffer, s_n_bytes_in_buff);

	s_n_bytes_in_buff = 0;	/* reset nb bytes in buffer */

	if (rc == 0) {		/* n_bytes written OK */
		return ( 0 );
	}
	xml2db_abort_program(error, "send() failed (flush),", (char *) sys_errlist[errno]);
	
	s_ctr_socket_write_errors++;

	return (-1);

} /* end of sf_flush_send_buffer() */	



static int sf_write_string(int sd, char *str)
/* 
 * Use s_send_buffer[] to minimize the number of calls to send().
 *
 * sf_flush_send_buffer(sd) is the function that really calls send(),
 * except when str is too large. It that case, send() is called
 * directly from here.
 */
{
	int	rc = 0,
		slen = 0,
		error=0;

	/* printf("write_string: str[%s]\n", str); */

	if (s_ctr_socket_write_errors >= 1) {
		return (-1);
	}
	if (NULL == str) {
		return (0);	/* strange */
	}
	slen = strlen(str);
	if (0 == slen) {
		return (0);	/* strange */
	}
	/* if buffer too full to add this string, flush it:
	 */
	if (slen >= (sizeof(s_send_buffer) - s_n_bytes_in_buff) ) {
		rc = sf_flush_send_buffer(sd);
		if (rc <= -1) {
			return (rc);
		}
		/* OK, continue with normal processing...
		 */
	}
	/* if str too large for buffer, send it directly, do not buffer:
	 */
	if (slen >= sizeof(s_send_buffer) ) {
		errno = 0;

		if (( rc = sf_create_sock_obj_if_not_done(sd)) != 0) {
			return -1;
		}

		rc = sock_obj_send(s_p_sock_obj, str, slen);

		if (rc != 0) {
			xml2db_abort_program(errno, "send() failed (large string),", 
					(char *) sys_errlist[errno]);
			s_ctr_socket_write_errors++;
			return (rc);
		}
		return (0);   /* str sent, return now */
	}

	/* there is room in the buffer, append str inside it:
	 */
	strncpy( &s_send_buffer[s_n_bytes_in_buff], str, slen);
	s_n_bytes_in_buff += slen;
	
	return (0);

} /* end of sf_write_string() */	





static int sf_make_readable(int sd, int level)
{
	char	*p = "";

	if (!s_b_human_readable) {
		return (0);
	}

	switch (level) {
	case 1:		p = "\n";		break;
	case 2:		p = "\n ";		break;
	case 3:		p = "\n  ";		break;
	case 4:		p = "\n   ";		break;
	case 5:		p = "\n    ";		break;
	case 6:		p = "\n      "; 	break;
	case 7:		p = "\n       ";	break;
	default:	p = "\n\t ";		break;
	}
	return (sf_write_string(sd, p) );

} /* end of sf_make_readable() */



static int sf_write_tag(int sd, char *tag)
{
	char	buff[120] = "[buff?]";

	sprintf(buff, "<%.90s>", tag);

	return (sf_write_string(sd, buff) );

} /* end of sf_write_tag() */



static int sf_begin_xml_response_if_not_done(int sd)
{
	if (!s_b_xml_response_header_done) {
		s_b_xml_response_header_done = 1;

		sf_write_tag(sd, XMLTP_TAG_XMLVERSION_AND_ENCODING);
		sf_make_readable(sd, 1);
		sf_write_tag(sd, XMLTP_TAG_RESPONSE);

		time( &s_t_begin );
		/* xml2db_write_log_msg_mm(ALOG_INFO, 0, "Begin XML response...", ""); */
	}
	return (0);

} /* end of sf_begin_xml_response_if_not_done() */



static int sf_begin_resultSetsList_if_not_done(int sd)
{
	if (!s_b_results_sets_started) {
		s_b_results_sets_started = 1;

		sf_make_readable(sd, 2);
		sf_write_tag(sd, XMLTP_TAG_RESULTSETSLIST);
	}
	return (0);

} /* end of sf_begin_resultSetsList_if_not_done() */



static int sf_write_nb_rows(int sd, int nb_rows)
{
	char	buff[80] = "[buff?]";

	sf_make_readable(sd, 4);
	sf_write_tag(sd, XMLTP_TAG_NBROWS);
	sf_write_tag(sd, XMLTP_TAG_INT);

	sprintf(buff, "%d", nb_rows);
	sf_write_string(sd, buff);

	sf_write_tag(sd, XMLTP_TAG_END_INT);
	sf_write_tag(sd, XMLTP_TAG_END_NBROWS);

	return (0);

} /* end of sf_write_nb_rows() */



static int sf_end_previous_result_set(int sd, int nb_rows)
{
	if (s_b_rows_started) {
		s_b_rows_started = 0;
		sf_make_readable(sd, 4);
		sf_write_tag(sd, XMLTP_TAG_END_ROWS);

		sf_write_nb_rows(sd, nb_rows);
	}
	else {
		/* This is a special case for empty result set */
		sf_make_readable(sd, 4);
		sf_write_tag(sd, XMLTP_TAG_END_COLNAMES);
		sf_write_tag(sd, XMLTP_TAG_ROWS);
		sf_write_tag(sd, XMLTP_TAG_END_ROWS);

		sf_write_nb_rows(sd, 0);
	}

	sf_make_readable(sd, 3);
	sf_write_tag(sd, XMLTP_TAG_END_RESULTSET);

	return (0);

} /* end of sf_end_previous_result_set() */



/* ----------------------------------------- PUBLIC FUNCTIONS:  */



void xmltp_writer_set_sock_obj(void *sock_obj)
{

	s_p_sock_obj = sock_obj;

} /* end of xmltp_writer_set_sock_obj */



int xmltp_writer_reset(void *p_ctx)
{
	/* @@@ should call:
	 *	xmltp_ctx_reset_xml_writer_state_variables(p_ctx);
	 */
	s_b_xml_response_header_done = 0;
	s_b_results_sets_started  = 0;
	s_b_rows_started          = 0;
	s_b_within_one_row        = 0;
	s_b_output_params_started = 0;
	s_b_messages_started      = 0;
	s_b_end_response_done     = 0;
	s_prev_result_set_no      = -999;

	s_ctr_errors = 0;
	memset(s_note_errmsg, 0, sizeof(s_note_errmsg) );

	return (0);

} /* end of xmltp_writer_reset() */



int xmltp_writer_note_error(void *p_ctx, char *errmsg)
{
	s_ctr_errors++;
	strncpy(s_note_errmsg, errmsg, sizeof(s_note_errmsg) );

	return (0);

} /* end of xmltp_writer_note_error() */


int xmltp_writer_begin_result_set(void *p_ctx, int result_set_no, int nb_rows)
{
	int	sd = -1;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}
	sf_begin_xml_response_if_not_done(sd);

	if (s_b_results_sets_started
	 && s_prev_result_set_no != result_set_no) {
		sf_end_previous_result_set(sd, nb_rows);
	}
	sf_begin_resultSetsList_if_not_done(sd);

	sf_make_readable(sd, 3);
	sf_write_tag(sd, XMLTP_TAG_RESULTSET);
	sf_make_readable(sd, 4);
	sf_write_tag(sd, XMLTP_TAG_COLNAMES);
	
	s_prev_result_set_no = result_set_no;

	return (0);

} /* end of xmltp_writer_begin_result_set() */



int xmltp_writer_column_name(void *p_ctx,
			     char *name,
			     int   datatype,
			     int   opt_size)
/*
 * Write a long <col>...</col> tag with the name and the datatype.
 * The datatype is in a <int> </int> tag.
 */
{
	int	sd = -1;
	char	buff[80] = "[buff?]";

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}
	sf_make_readable(sd, 5);
	sf_write_tag(sd, XMLTP_TAG_COL);
	sf_write_tag(sd, XMLTP_TAG_NAME);
	sf_write_string(sd, name);
	sf_write_tag(sd, XMLTP_TAG_END_NAME);
	sf_write_tag(sd, XMLTP_TAG_INT);
	sprintf(buff, "%d", datatype);
	sf_write_string(sd, buff);
	sf_write_tag(sd, XMLTP_TAG_END_INT);
	
	/* Here we write an optional string size */
	/*
	 * Beware!!!  Here there shouldn't be a JAVA_SQL_CHAR
	 * because it would be an upstream bug.
	 */
	if ( datatype == JAVA_SQL_VARCHAR ) {
		sf_write_tag(sd, XMLTP_TAG_SZ);
		sprintf(buff, "%d", opt_size);
		sf_write_string(sd, buff);
		sf_write_tag(sd, XMLTP_TAG_END_SZ);
	}

	sf_write_tag(sd, XMLTP_TAG_END_COL);

	return (0);

} /* end of xmltp_writer_column_name() */




static char *sf_get_data_tag_for_datatype(int datatype)
{
	switch (datatype) {
	case JAVA_SQL_SMALLINT:
	case JAVA_SQL_INTEGER:
	case JAVA_SQL_NUMERIC:
		return (XMLTP_TAG_INT);

	case JAVA_SQL_TIMESTAMP:
		return (XMLTP_TAG_DATE);

	case JAVA_SQL_FLOAT:
		return (XMLTP_TAG_NUM);

	case JAVA_SQL_VARCHAR:
	case JAVA_SQL_CHAR:
	default:
			break;
	}
	return (XMLTP_TAG_STR);

} /* end of sf_get_data_tag_for_datatype() */




static char *sf_get_data_end_tag_for_datatype(int datatype)
{
	switch (datatype) {
	case JAVA_SQL_SMALLINT:
	case JAVA_SQL_INTEGER:
	case JAVA_SQL_NUMERIC:
		return (XMLTP_TAG_END_INT);

	case JAVA_SQL_TIMESTAMP:
		return (XMLTP_TAG_END_DATE);

	case JAVA_SQL_FLOAT:
		return (XMLTP_TAG_END_NUM);

	case JAVA_SQL_VARCHAR:
	case JAVA_SQL_CHAR:
	default:
			break;
	}
	return (XMLTP_TAG_END_STR);

} /* end of sf_get_data_end_tag_for_datatype() */




int xmltp_writer_column_value(	void *p_ctx,
			 	int   datatype,
				int   null_ind,
				char *strval)
{
	int	sd = -1;
	char	buff[80] = "[buff?]";

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}
	if (!s_b_rows_started) {
		s_b_rows_started = 1;
		sf_make_readable(sd, 4);
		sf_write_tag(sd, XMLTP_TAG_END_COLNAMES);
		sf_make_readable(sd, 4);
		sf_write_tag(sd, XMLTP_TAG_ROWS);
		s_b_within_one_row = 0;
	}
	if (!s_b_within_one_row) {
		s_b_within_one_row = 1;
		sf_make_readable(sd, 5);
		sf_write_tag(sd, XMLTP_TAG_ROW);
	}

	sf_make_readable(sd, 6);
	sf_write_tag(sd, sf_get_data_tag_for_datatype(datatype) );

	if ( xmltp_encode(strval, s_esc_buf)  == 0 ) {
		sf_write_string(sd, strval);
	}
	else {
		sf_write_string(sd, s_esc_buf);
	}


	sf_write_tag(sd, sf_get_data_end_tag_for_datatype(datatype) );
	sf_write_tag(sd, XMLTP_TAG_ISNULL);
	sprintf(buff, "%d", (null_ind != 0) );
	sf_write_string(sd, buff);
	sf_write_tag(sd, XMLTP_TAG_END_ISNULL);

	return (0);

} /* end of xmltp_writer_column_value() */



int xmltp_writer_end_colum_values(void *p_ctx)
/*
 * This is the end of a row.
 */
{
	int	sd = -1;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}

	s_b_within_one_row = 0;

	sf_make_readable(sd, 5);
	sf_write_tag(sd, XMLTP_TAG_END_ROW);

	return (0);

} /* end of xmltp_writer_end_column_values() */




static int sf_end_rows_if_started(int sd, int nb_rows)
/*
 * Called by:	xmltp_writer_begin_output_params()
 *		xmltp_writer_end_response()
 */
{
	if (s_b_results_sets_started) {

		s_b_results_sets_started = 0;

		sf_end_previous_result_set(sd, nb_rows);

		sf_make_readable(sd, 2);
		sf_write_tag(sd, XMLTP_TAG_END_RESULTSETSLIST);
	}

	return (0);

} /* end of sf_end_rows_if_started() */
				


int xmltp_writer_begin_output_params(void *p_ctx, int result_set_no, int nb_rows)
{
	int	sd = -1;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}
	sf_begin_xml_response_if_not_done(sd);

	/* sf_end_rows_if_started() is used also by
	 * the function that writes the error_messages
	 */
	sf_end_rows_if_started(sd, nb_rows);

	if (!s_b_output_params_started) {
		s_b_output_params_started = 1;

		sf_make_readable(sd, 2);
		sf_write_tag(sd, XMLTP_TAG_OUTPARAMS);
	}
	return (0);

} /* end of xmltp_writer_begin_output_params() */



int xmltp_writer_param_name(	void *p_ctx,
			char *name,
			int   datatype)
/*
 * We write everything in xmltp_writer_param_value()
 */
{
	return (0);	/* do nothing here */

} /* end of xmltp_writer_param_name() */



int xmltp_writer_param_value(	void *p_ctx,
				char *name,
				int   datatype,
				int   opt_size,
				int   is_output,
				int   null_ind,
				char *strval)
/*
 * We combine null_ind and datatype in one <attr>ONDD</attr> field.
 * (O:output, N:null, DD:datatype).
 *
 * <param> <name>@o_newseq_no</name>
 * 	<int>10205039</int> <attr>002</attr> </param>
 */
{
	int	sd = -1;
	char	buff[80] = "[buff?]";

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}

	sf_make_readable(sd, 3);
	sf_write_tag(sd, XMLTP_TAG_PARAM);

	sf_write_tag(sd, XMLTP_TAG_NAME);
	sf_write_string(sd, name);
	sf_write_tag(sd, XMLTP_TAG_END_NAME);

	sf_write_tag(sd, sf_get_data_tag_for_datatype(datatype) );

	if ( xmltp_encode(strval, s_esc_buf) == 0 ) {
		sf_write_string(sd, strval);
	}
	else {
		sf_write_string(sd, s_esc_buf);
	}

	sf_write_tag(sd, sf_get_data_end_tag_for_datatype(datatype) );

	sf_write_tag(sd, XMLTP_TAG_ATTR);
	sprintf(buff, "%d%d%d",
			(null_ind != 0),
			(is_output != 0),
			datatype);
	sf_write_string(sd, buff);
	sf_write_tag(sd, XMLTP_TAG_END_ATTR);


	/* Here we write an optional string size */
	/*
	 * Beware!!!  Here there shouldn't be a JAVA_SQL_CHAR
	 * because it would be an upstream bug.
	 */
	if ( datatype == JAVA_SQL_VARCHAR ) {
		sf_write_tag(sd, XMLTP_TAG_SZ);
		sprintf(buff, "%d", opt_size);
		sf_write_string(sd, buff);
		sf_write_tag(sd, XMLTP_TAG_END_SZ);
	}

	sf_write_tag(sd, XMLTP_TAG_END_PARAM);

	return (0);

} /* end of xmltp_writer_param_value() */




int xmltp_writer_end_param_values(void *p_ctx)
{
	int	sd = -1;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}

	/* do nothing here!
	 * 
	 * Output parameters come in the result handler function like
	 * one "result set" per output parameter, with each result set
	 * containing one row with one column.
	 *
	 * xmltp_writer_end_param_values() is called after one such
	 * "row" (one param) is processed. NOT after all output params.
	 */
	return (0);

} /* end of xmltp_writer_end_param_values() */



static int sf_end_output_params_if_started(void *p_ctx)
{
	int	sd = -1;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}
	if (s_b_output_params_started) {
		s_b_output_params_started = 0;
		sf_make_readable(sd, 2);
		sf_write_tag(sd, XMLTP_TAG_END_OUTPARAMS);
	}
	return (0);

} /* end of sf_end_output_params_if_started() */



int xmltp_writer_begin_msg(void *p_ctx, int nb_rows)
{
	int	sd = -1;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}
	sf_begin_xml_response_if_not_done(sd);

	/* Close the result sets, if not done yet:
	 */
	sf_end_rows_if_started(sd, nb_rows);

	/* Close the output params, if open:
	 */
	sf_end_output_params_if_started(p_ctx);

	sf_make_readable(sd, 2);
	sf_write_tag(sd, XMLTP_TAG_MSGLIST);
	
	return (0);

} /* end of xmltp_writer_begin_msg() */



int xmltp_writer_msg(	void *p_ctx,
			int msg_num,
			char *msg_text)
{
	int	sd = -1;
	char	buff[80] = "[buff?]";

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}

	sf_make_readable(sd, 3);
	sf_write_tag(sd, XMLTP_TAG_MSG);

	/* Message number */
	sf_write_tag(sd, XMLTP_TAG_INT);
	sprintf(buff, "%d", msg_num);
	sf_write_string(sd, buff);
	sf_write_tag(sd, XMLTP_TAG_END_INT);

	/* Message text */
	sf_write_tag(sd, XMLTP_TAG_STR);


	if ( xmltp_encode(msg_text, s_esc_buf) == 0 ) {
		sf_write_string(sd, msg_text);
	}
	else {
		sf_write_string(sd, s_esc_buf);
	}

	sf_write_tag(sd, XMLTP_TAG_END_STR);

	sf_write_tag(sd, XMLTP_TAG_END_MSG);

	return (0);

} /* end of xmltp_writer_msg() */



int xmltp_writer_end_msg(void *p_ctx)
{
	int	sd = -1;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}

	sf_make_readable(sd, 2);
	sf_write_tag(sd, XMLTP_TAG_END_MSGLIST);

	return (0);

} /* end of xmltp_writer_end_msg() */



int xmltp_writer_end_response(void *p_ctx,
				int return_status, 
				int rc,
				int nb_rows)
{
	time_t	t_now = 0L;
	int	sd = -1;
	char	buff[80] = "[buff?]";

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}

	/* if the RPC failed, there might not have been any rows before
	 * and the XML stream was never started. Begin it now, if
	 * required:
	 */
	sf_begin_xml_response_if_not_done(sd);

	/* Close the result sets, if not done yet:
	 */
	sf_end_rows_if_started(sd, nb_rows);

	/* Close the output params, if open:
	 */
	sf_end_output_params_if_started(p_ctx);

	/* Si return_status >= 0, mais rc <= -1,
	 * return_status doit devenir = (-1500 + rc)
	 */
	if (return_status >= 0
	 && rc <= -1) {
		return_status = (-1500 + rc);
	}
	sf_make_readable(sd, 2);
	sf_write_tag(sd, XMLTP_TAG_RETURNSTATUS);
	sf_write_tag(sd, XMLTP_TAG_INT);
	sprintf(buff, "%d", return_status);
	sf_write_string(sd, buff);
	sf_write_tag(sd, XMLTP_TAG_END_INT);
	sf_write_tag(sd, XMLTP_TAG_END_RETURNSTATUS);

	sf_make_readable(sd, 1);
	sf_write_tag(sd, XMLTP_TAG_END_RESPONSE);
	sf_make_readable(sd, 1);
	sf_write_tag(sd, XMLTP_TAG_EOT);

	rc = sf_flush_send_buffer(sd);	/* send() to socket now */

	time( &t_now );

	/* xml2db_write_log_msg_mm(ALOG_INFO, (int) (t_now - s_t_begin), "END XML response.", ""); */

	return (rc);

} /* end of xmltp_writer_end_response() */



int xmltp_writer_begin_proc_call(void *p_ctx, char *proc_name)
{
	int	sd = -1;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}

	sf_write_tag(sd, XMLTP_TAG_XMLVERSION_AND_ENCODING);
	sf_make_readable(sd, 1);
	sf_write_tag(sd, XMLTP_TAG_PROCCALL);
	sf_make_readable(sd, 2);
	sf_write_tag(sd, XMLTP_TAG_PROC);
	sf_write_string(sd, proc_name);
	sf_write_tag(sd, XMLTP_TAG_END_PROC);

	return (0);

} /* end of xmltp_writer_begin_proc_call() */



int xmltp_writer_end_proc_call(void *p_ctx)
{
	int	sd = -1;
	int	rc = 0;

	sd = xmltp_ctx_get_socket(p_ctx);
	if (sd <= -1) {
		return (-2);
	}

	sf_make_readable(sd, 1);
	sf_write_tag(sd, XMLTP_TAG_END_PROCCALL);
	sf_make_readable(sd, 1);
	sf_write_tag(sd, XMLTP_TAG_EOT);

	rc = sf_flush_send_buffer(sd);  /* send() to socket now */

	return (0);

} /* end of xmltp_writer_end_proc_call() */



void xmltp_writer_drop_sock_obj()
{

	free(s_p_sock_obj);
	s_p_sock_obj=NULL;

	return;

} /* End of xmltp_writer_drop_sock_obj */



/* --------------------------- end of xmltp_writer.c ---------------- */
