/**************************************************************

 xmltp_parser_api.c
 ------------------

 *  Copyright (c) 2001-2003 Jocelyn Blanchet
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

 This module implement the xmltp_parser.h functions.

 TO DO
 -----

 1. Add loggin
 	Require a new config param read by xmltp_api.c 
	to set trace level inside of xmltp_parser_api.

 LIST OF CHANGES:
 ----------------

 2002-02-21, jbl:	Initial release.

 **************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "apl_log1.h"
#include "debuglvl.h"

#include "xmltp_api_compat.h"
#include "xmltp_parser.h"
#include "xmltp_api.h"


/******* Defines **********************************************/
 
#define VERSION_NUM             "0.9 (Beta)"
#define MODULE_NAME             (__FILE__)
#define VERSION_ID              (MODULE_NAME " " VERSION_NUM " - " __DATE__ " - " __TIME__)

#define MAX_SYB_VALUE_SIZE       256

 
/******* Private data ****************************************/
 
static int s_debug_trace_level=DEBUG_TRACE_LEVEL_NONE;

static int s_result_type;

static char s_work_buffer[MAX_SYB_VALUE_SIZE+1];

/* Since output param name, type and value all	*/
/* come at the same time, we need to emulate	*/
/* resulset behavior: send all param name and	*/
/* type, follow by all params value.		*/
/* In order to do that we need to accumulate	*/
/* all params value.				*/

/* We also need to keep null indicator.  	*/

static char st_output_param[256][256];
static int  st_null_ind[256];
static int  s_nb_output_param = 0;
 
 

/* --------------------------------------- PUBLIC functions: */

void xmltp_parser_set_debug_bitmask(int val)
{

	s_debug_trace_level = val;

	return;
}


/* --------------------------------------- PARSER (action) functions: */



/* ---------------------- Actions used in parsing of XMLTP proc call: */



int xmltp_parser_proc_call_parse_end(void *p)
{

	return 0;

} /* ------ End of xmltp_parser_proc_call_parse_end ------- */



int xmltp_parser_set_proc_name(void *p, char *proc_name)
{

	return 0;
} /* ----- End of xmltp_parser_set_proc_name ----- */



/* ------ Actions used in parsing of params in *BOTH* proc call AND response: */



int xmltp_parser_add_param_to_list(void *p, void *p_param)
{
	return 0;

} /* ------ End of xmltp_parser_add_param_to_list  ------- */



void *xmltp_parser_build_param(void *p, char *name, char *val, char *attrib, char *opt_size)
{

	char tmp[3];
	int rc;

	/* printf("build_param: name[%s], val[%s], attrib[%s][%d]\n", name, val, attrib, atoi(&(attrib[2]))); */

	if ( (rc = xmltp_decode(val, s_work_buffer)) == 1) {

		strcpy(st_output_param[s_nb_output_param], s_work_buffer);
	}
	else {
		strcpy(st_output_param[s_nb_output_param], val);
	}

	strncpy(tmp,attrib,1);
	st_null_ind[s_nb_output_param] = atoi(tmp);

	++s_nb_output_param;

	xmltp_api_add_column_def(name, atoi(&(attrib[2])), opt_size);

	return 0;

} /* ------ End of xmltp_parser_build_param  ------- */



/* --------------------------- Actions used in parsing of XMLTP response: */



int xmltp_parser_response_parse_end(void *p)
{
	/* printf("xmltp_parser_response_parse_end\n"); */

	return 0;

} /* ------ End of xmltp_parser_response_parse_end  ------- */



int xmltp_parser_response_parse_begin(void *p)
{
	/* printf("xmltp_parser_response_parse_begin\n"); */

	return 0;

} /* ------ End of xmltp_parser_response_parse_begin  ------- */



int xmltp_parser_set_return_status(void *p, char *val)
{
	/* printf("set_return_status: val[%s]\n", val); */

	xmltp_api_set_return_status(atoi(val));

	return 0;

} /* ------ End of xmltp_parser_set_return_status  ------- */



int xmltp_parser_outparams_end(void *p)
{
	int i;

	/* printf("xmltp_parser_outparams_end\n"); */

	xmltp_api_one_row_ready(1, s_result_type, 0);

	xmltp_api_reset_nb_column(); 

	for (i=0; i<s_nb_output_param; i++) {
		
		xmltp_api_add_row_elem(st_output_param[i] , st_null_ind[i]);
	}

	xmltp_api_one_row_ready(0, s_result_type, 0);

	return 0;

} /* ------ End of xmltp_parser_outparams_end  ------- */



int xmltp_parser_outparams_begin(void *p)
{
	/* printf("xmltp_parser_outparams_begin\n"); */

	s_result_type = 4042;

	s_nb_output_param = 0;

	xmltp_api_reset_nb_column();

	return 0;

} /* ------ End of xmltp_parser_outparams_begin ------- */



int xmltp_parser_msglist_end(void *p)
{
	/* printf("xmltp_parser_msglist_end\n"); */

	return 0;

} /* ------ End of xmltp_parser_msglist_end ------- */



int xmltp_parser_msglist_begin(void *p)
{
	/* printf("xmltp_parser_msglist_begin\n"); */

	return 0;

} /* ------ End of xmltp_parser_msglist_begin ------- */



int xmltp_parser_add_msg_to_list(void *p, char *msgno_str, char *msg)
{

	int rc;
	/* printf("add_msg_to_list: msgno_str[%s], msg[%s]\n", msgno_str, msg); */

	if ( (rc = xmltp_decode(msg, s_work_buffer)) == 1) {

		xmltp_api_error_msg_handler(msgno_str, s_work_buffer);
	}
	else {
		xmltp_api_error_msg_handler(msgno_str, msg);
	}

	return 0;

} /* ------ End of xmltp_parser_add_msg_to_list ------- */



int xmltp_parser_resultsetslist_end(void *p)
{
	/* printf("xmltp_parser_resultsetslist_end\n"); */

	return 0;

} /* ------ End of xmltp_parser_resultsetslist_end ------- */



int xmltp_parser_resultsetslist_begin(void *p)
{
	/* printf("xmltp_parser_resultsetslist_begin\n"); */

	xmltp_api_reset_result_set_no();

	return 0;

} /* ------ End of xmltp_parser_resultsetslist_begin ------- */



int xmltp_parser_resultset_end(void *p)
{
	/* printf("xmltp_parser_resultset_end\n"); */
	xmltp_api_next_result_set();

	return 0;

} /* ------ End of xmltp_parser_resultset_end ------- */



int xmltp_parser_resultset_begin(void *p)
{
	/* printf("xmltp_parser_resultset_begin\n"); */

	s_result_type = 4040;

	xmltp_api_reset_nb_column();
	return 0;

} /* ------ End of xmltp_parser_resultset_begin ------- */



int xmltp_parser_colnames_end(void *p)
{
	/* printf("xmltp_parser_colnames_end\n"); */

	xmltp_api_one_row_ready(1, s_result_type, 0);

	return 0;

} /* ------ End of xmltp_parser_colnames_end ------- */



int xmltp_parser_colnames_begin(void *p)
{
	/* printf("xmltp_parser_colnames_begin\n"); */

	return 0;

} /* ------ End of xmltp_parser_colnames_begin ------- */



int xmltp_parser_add_colname(void *p, char *colname, char *type_str, char *opt_size)
{
	/* printf("add_colname: colname[%s], type_str[%s]\n", colname, type_str); */

	xmltp_api_add_column_def(colname, atoi(type_str), opt_size);

	return 0;

} /* ------ End of xmltp_parser_add_colname ------- */



int xmltp_parser_rows_end(void *p)
{
	/* printf("xmltp_parser_rows_end\n"); */

	return 0;

} /* ------ End of xmltp_parser_rows_end ------- */



int xmltp_parser_rows_begin(void *p)
{
	/* printf("xmltp_parser_rows_begin\n"); */

	return 0;

} /* ------ End of xmltp_parser_rows_begin ------- */



int xmltp_parser_row_elems_end(void *p)
{
	/* printf("xmltp_parser_row_elems_end\n"); */

	xmltp_api_one_row_ready(0, s_result_type, 0);

	return 0;

} /* ------ End of xmltp_parser_row_elems_end ------- */



int xmltp_parser_row_elems_begin(void *p)
{
	/* printf("xmltp_parser_row_elems_begin\n"); */

	xmltp_api_reset_nb_column();

	return 0;

} /* ------ End of xmltp_parser_row_elems_begin ------- */



int xmltp_parser_add_row_elem(void *p, char *val, char *is_null)
{
	int rc;
	/* printf("add_row_elem: val[%s], is_nul[%s]\n", val, is_null); */

	if ( (rc = xmltp_decode(val, s_work_buffer)) == 1) {

		xmltp_api_add_row_elem(s_work_buffer , atoi(is_null));
	}
	else {
		xmltp_api_add_row_elem(val , atoi(is_null));
	}

	return 0;

} /* ------ End of xmltp_parser_add_row_elem ------- */



int xmltp_parser_set_nbrows(void *p, char *nbrows_str)
{
	/* printf("set_nbrows: nbrows_str[%s]\n", nbrows_str); */

	xmltp_api_one_row_ready(2, s_result_type, atoi(nbrows_str));

	return 0;

} /* ------ End of xmltp_parser_set_nbrows ------- */



int xmltp_parser_set_totalrows(void *p, char *totalrows_str)
{
	printf("set_totalrows: totalrows_str[%s]\n", totalrows_str);

	return 0;

} /* ------ End of xmltp_parser_set_totalrows ------- */



/* --------------------------- End of xmltp_parser_clt.h ---------------- */
