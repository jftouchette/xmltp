/******************************************************
 xmltp_api_compat.c

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

 xmltp_api - ct10api compatibility module. 

 TO DO:
 ------


 LIST OF CHANGES:
 ----------------

 2002-02-13, jbl:	Initial release.
 2003-06-17, jbl:	Version beta completed
 2003-06-18, jbl:	Clean-up for version 1
 2003-08-27, jbl:	Fixed float conversion problem in cs_convert

 ******************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "apl_log1.h"
#include "debuglvl.h"
#include "xmltp_api_compat.h"


/******* Defines **********************************************/

#define VERSION_NUM "0.9 (Beta)"
#define MODULE_NAME (__FILE__)
#define VERSION_ID  (MODULE_NAME " " VERSION_NUM " - " __DATE__ " - " __TIME__)

#define CT10API_VERSION_ID ("1.0.2 " __FILE__ ": " __DATE__ " - " __TIME__)

#define APL_LOG_DEFAULT_NOTIFY	"nobody"
#define ARG_APPL_SERVICE	's'
#define CONFIG_STRING_SERVICE	"SERVICE"

/******* Private data ****************************************/

static char *version=VERSION_NUM;
static char s_log_buf[4096];
static char *s_appl_name; 		/* application name use by log functions */
static int  s_debug_trace_level=0;

/*
 * Pointers to function that customize the error recovery and
 * diagnostic:
 */

static int      (* s_pf_display_err_msg)() =  NULL;


static struct int_str_tab_entry s_type_strings_table[] = {
#include "alogtypt.h"

        { ISTR_NUM_DEFAULT_VAL,  "?     " }
};


/* XMLTP-L date structure */

typedef struct {
	char year[4];
	char space1[1];
	char month[2];
	char space2[1];
	char day[2];
	char space3[1];
	char hour[2];
	char space4[1];
	char minute[2];
	char space5[1];
	char second[2];
	char space6[1];
	char milli[3];
} XMLTP_API_CHAR_DATE_T;

/******* Private functions prototype ***********************/ 

static void sf_display_err_msg(char *msg_number, char *msg_text);

static int sf_log(int err_level, int msg_no, char *msg_text, int level_to_log);

static int sf_default_display_err_msg( int	b_to_log, char 	*p_app_context_info,
 			int	msg_no, int	severity, int	layer, int	origin,
 			int	msg_text_len, char	*msg_text, int	srv_name_len,	
 			char	*srv_name, int	proc_name_len, char	*proc_name, int	os_msg_len,
 			char	*os_msg, int	proc_line_or_os_number, char	*error_category_name );


/******* Private functions ***********************************/ 



/*********************************************
 Write log message.
 
 Error level:
 
        ALOG_ERROR 0 (always log)
        ALOG_INFO  1 (depend on level to log)
        ALOG_WARN  2 (always log)
 
 Trace level (from debuglvl.h):
 
        DEBUG_TRACE_LEVEL_NONE   0
        DEBUG_TRACE_LEVEL_LOW    1
        DEBUG_TRACE_LEVEL_MEDIUM 5
        DEBUG_TRACE_LEVEL_FULL   10
 
 *********************************************/
 
static int sf_log(      int     err_level,      /* ALOG_INFO, ALOG_WARN, ... */
                        int     msg_no,         /* Message number.           */
                        char    *msg_text,      /* message text.             */
                        int     level_to_log)   /* Trace level needed to log.*/
{
int rc=0;
 
if ((err_level != ALOG_INFO) || (level_to_log <= s_debug_trace_level)) {
        rc=alog_aplogmsg_write( err_level, MODULE_NAME, s_appl_name,
                                msg_no, APL_LOG_DEFAULT_NOTIFY, msg_text);
}
return rc;
} /* --------------------------- End of sf_log --------------------- */



/*********************************************************
 This default error handler is normally not use.
 Compatibility module should only be use if the client
 use his own error handler. I just copy it from ct10reco in
 case we need it.

 If the client does not change the default callback with:
 ct10reco_assign_pf_display_err_msg, the default wont be change
 in xmltp_api. This mean that message wont reach the
 compatibility module.

 We could add a new function to activate the logging using
 the old compatible mode.
 *********************************************************/

static int sf_default_display_err_msg(
			int	b_to_log,
 			char 	*p_app_context_info,
 			int	msg_no,
 			int	severity,
 			int	layer,
 			int	origin,
 			int	msg_text_len,
 			char	*msg_text,
 			int	srv_name_len,	
 			char	*srv_name,
 			int	proc_name_len,
 			char	*proc_name,
 			int	os_msg_len,
 			char	*os_msg,
 			int	proc_line_or_os_number,
 			char	*error_category_name )
{

	char *s_p_current_active_conn = NULL;
	int rc;

	sprintf( s_log_buf,
			( "%.20s: %.*s%.10smsg %d, sev %d: "
			  "'%.*s' "
			  "%.10s%.*s"
			  "%.10s%.*s (%d) "
			  "[%.30s]" ),
			  "connection",
			/*( s_p_current_active_conn != NULL) ?
				xmltp_api_get_server_name_of_conn( s_p_current_active_conn ) :
				"[No active conn?]", */
			srv_name_len,
			srv_name,
			( srv_name_len > 0 ) ? ": " : "",
			msg_no,
			severity,
			msg_text_len,
			msg_text,
			( proc_name_len > 0 ) ? "proc=" : "",
			proc_name_len,
			proc_name,
			( os_msg_len > 0) ? ", " : "",
			os_msg_len,
			os_msg,
			proc_line_or_os_number,
			error_category_name );

        rc=alog_aplogmsg_write( ALOG_ERROR, MODULE_NAME, s_appl_name,
                                msg_no, APL_LOG_DEFAULT_NOTIFY, s_log_buf);

	return CS_SUCCEED;

} /* ------ End of sf_default_display_err_msg ------ */



/********************************************************
 This function is callback from xmltp_api
 ********************************************************/

static void sf_display_err_msg(char *msg_number, char *msg_text) 
{

	(* s_pf_display_err_msg)(

		1,			/* To be log */
		NULL,			/* Context info */
		atoi(msg_number),	/* Message number */
		0,			/* Severity */
		0,			/* Layer */
		0,			/* origin */
		strlen(msg_text),	/* Message text len */
		msg_text,		/* Message text */
		0,			/* Server name len */
		"",			/* Server name */
		0,			/* Proc name len */
		"",			/* Proc name */
		0,			/* Os message len */
		"",			/* Os message */
		0, 			/* Proc line or OS number */
		"");			/* Error category name */

} /* ------- End of sf_display_err_msg ------ */



/******* Public functions ************************************/ 



/********************************
 Get severity mask.
 ********************************/
 
long ct10api_get_diag_sev_mask(void *p_ct10_conn)
{

	return 0;
} /* --------------------------- End of ct10api_get_diag_sev_mask ----------- */



/*************************************************
 Set application name to use with log functions.
 *************************************************/
 
void ct10api_assign_application_name(char *app_name)
{
	s_appl_name = app_name;

	xmltp_api_assign_application_name(app_name);

	return;
} /* --------------------------- End of ct10api_assign_application_name ----- */



/****************************
 Context initialization.
 ****************************/
 
int ct10api_init_cs_context()
{

	return 0;
} /* ------------------- End of ct10api_init_cs_context ------ */



/************************
 Get diagnostic mask.
 ************************/
 
long ct10api_get_diagnostic_mask(void *p_ct10_conn)
{

	return 0;
} /* --------------------------- End of ct10api_get_diagnostic_mask --------- */



/*******************************************
 Create connection.
 *******************************************/
 
void *ct10api_create_connection(char *login_id, 
				char *password,
                                char *app_name, 
				char *host_name,
                                char *server_name,
                                char *init_string,
                                long options_mask)
{

	return (void *) (xmltp_api_create_connection(
			login_id,
			password,
			app_name,
			server_name,
			init_string,
			options_mask));
	

} /* -------- End of ct10api_create_connection ------------- */



/*********************************************************************
  Reconfigure master recovery table from config file.
 *********************************************************************/

int ct10reco_reconfigure_master_recov_table_from_config_params()
{

	return 0;
} /* ------ End of ct10reco_reconfigure_master_recov_table_from_config_params --- */



/*************************************
 Connect to server
 *************************************/

int ct10api_connect(void *p_api_ctx)
{

	return (xmltp_api_connect(p_api_ctx));

} /* ---------------- End of ct10api_connect ------------------ */



/*************************************
 Exec RPC
 *************************************/

int ct10api_exec_rpc(   void *p_api_ctx,
                        char *rpc_name,
                        void  *params_array[],
                        int nb_params,
                        int *p_return_status)
{

	return (xmltp_api_exec_rpc ( 	p_api_ctx,
					rpc_name,
					params_array,
					nb_params,
					p_return_status) );

} /* ----------------- End of ct10api_exec_rpc ------------------ */



/*************************************
 Exec RPC String
 *************************************/

int ct10api_exec_rpc_string(void    *p_api_ctx,
                            char    *rpc_string,
                            int     *p_return_status)
{
	return ( xmltp_api_exec_rpc_string(p_api_ctx, rpc_string, p_return_status) );

} /* ---------------- End of ct10api_exec_rpc_string ------------- */



/*************************************
 Assign custom result set handler
 *************************************/

int ct10api_assign_process_row_function(void *p_ct10_conn,
        int (* pf_process_row_or_params)(void *p_ct10_conn, int b_is_row,
                                     void  *data_array[],
                                     int nb_data,
                                     int result_set_no,
                                     int result_type,
                                     int header_ind)  )
{

	return ( xmltp_api_assign_process_row_function(p_ct10_conn, pf_process_row_or_params) );

} /* -------- End of ct10api_assign_process_row_function ----- */



/*************************************
 Close connection
 *************************************/

int ct10api_close_connection(void *p_ct10_conn)
{
	return (xmltp_api_close_connection(p_ct10_conn));

} /* -------- End of ct10api_close_connection ------- */



/*
 * This function as a lot of parameters that are not required anymore
 * for xmltp_api. We need them for backward compatibility with ct10api, ct10reco.
 *
 * Here we will only fake all old parameters not existing in xmltp_api.
 * The real callback from xmltp_api is very simple. We only received 
 * the message number and text.
 *
 * Here is the original comments from ct10reco:
 *
 * Assign the address of a custom function that would display (or log)
 * any error message that will occur.
 *
 * If no custom function is defined for this purpose, the default behaviour
 * is to write most error messages to the error log.
 *
 * The arguments passed to the custom function are:
 *		(	int	 b_to_log,
 *			char 	*p_app_context_info,
 *			int	 msg_no,
 *			int	 severity,
 *			int	 layer,
 *			int	 origin,
 *			int	 msg_text_len,
 *			char	*msg_text,
 *			int	 srv_name_len,	
 *			char	*srv_name,
 *			int	 proc_name_len,
 *			char	*proc_name,
 *			int	 os_msg_len,
 *			char	*os_msg,
 *			int	 proc_line_or_os_number,
 *			char	*error_category_name
 *		)
 */

void ct10reco_assign_pf_display_err_msg(int (* ptr_funct)() )
{
	/* Here we set the client callback */

	s_pf_display_err_msg = ptr_funct;


	/* We need to change the default callback function in */
	/* xmltp_api. All message originate from there.       */

	xmltp_api_assign_error_msg_handler(sf_display_err_msg);

} /* ----- End of ct10reco_assign_pf_display_err_msg ----- */



/*******************************
 Init log system.
 *******************************/

int ct10api_init_log(char *logname)
{
int     rc;

if ((rc = alog_set_log_filename(logname)) != 0) {
        return rc;
}

if ((rc = alog_bind_message_types_table(&s_type_strings_table[0],
                                    sizeof(s_type_strings_table) ))
                   != 0) {
        return rc;
}

return (0);
} /* --------------------------- End of ct10api_init_log -------------------- */



/*******************************
 Stub, drop feature
 *******************************/

void ct10api_set_log_mode_quiet(int new_val)
/*
 * If ct10api_set_log_mode_quiet(1) is called, then ct10api_apl_log()
 * will stop writing to the application log (through apl_log1.c).
 */
{
	return;
} /* ----------- End of  ct10api_set_log_mode_quiet -------------- */



/******************************************
 Close and drop open client cs context.
 Stub, not required anymore with xmltp.
 The name of this function is not good.
 ******************************************/

int ct10api_log_stats_and_cleanup_everything()
{
	return (0);

} /* -------------- End of ct10api_log_stats_and_cleanup_everything ---------- */



/***************************************
 Get version info.
 ***************************************/

char *ct10api_get_version()
{
        return (CT10API_VERSION_ID);

} /* --------------------------- End of ct10api_get_version ----------------- */



/*********************
 Return server name 
 *********************/

char *ct10api_get_server_name_of_conn(void *p_conn)
{
        return ( (char *) xmltp_api_get_server_name_of_conn(p_conn) );

} /* ----- End of ct10api_get_server_name_of_conn() ------ */



/**************************
 Set trace level flag.
 **************************/

void ct10api_assign_debug_trace_flag(int new_val)
{
	s_debug_trace_level = new_val;

	xmltp_api_set_debug_trace_flag(new_val);

	return;
} /* --------------------------- End of ct10api_assign_debug_trace_flag() ----- */



/*
 **************************
 Get trace level flag.
 **************************/

int ct10api_get_debug_trace_flag()
{
	return (s_debug_trace_level);
} /* ------------- End of ct10api_get_debug_trace_flag() --------------- */



/*************************
 Config file reading.
 ************************/

int ct10api_load_config(char *config_file, char *section_name)
{
int rc=0;
if ((rc=ucfg_open_cfg_at_section(config_file, section_name)) != 0) {

        fprintf(stderr,"Error openning config file '%s', section '%s', rc: '%d'\n",
        config_file, section_name, rc);

}

return rc;
} /* --------------------------- End of ct10api_load_config ----------------- */



/*******************************************
 Return xmltp_api context
 Stub, not required with xmltp
 *******************************************/

void ct10api_get_context()
{
	return;
} /* -------- End of ct10api_get_context -------- */



/*****************************************
 ct10api log function
 *****************************************/

int ct10api_apl_log(    int     err_level,      /* ALOG_INFO, ALOG_WARN, ... */
                        int     msg_no,         /* Message number.           */
                        char    *msg_text,      /* message text.             */
                        int     level_to_log)   /* Trace level needed to log.*/
{

	return (sf_log(err_level, msg_no, msg_text, level_to_log));
} /* --------------------------- End of ct10api_apl_log --------------------- */



/*****************************************
 sybase cs_lib date conversion functions
 
 limitation:
 
 Support same datatype as in trgx
 	datetime, char, int, float
 	datetime is support as a char type.

 work only with a new version of
 ct10rpc_parse that allocated the right 
 space for the new format of date. 

 ******************************************/

extern CS_RETCODE CS_PUBLIC cs_convert(
		void *context,		/* Not use with xmltp_api */
		CS_DATAFMT *fmt_in, 	/* type of from field to be converted */
		void *p_val, 		/* Value of from field */
		CS_DATAFMT *fmt_out, 	/* Type of converted field */
		void *buf, 		/* Result buffer */
		int *out_len		/* Size that was place into buff */
		)
{

	if ( fmt_out->datatype == CS_CHAR_TYPE ) {

		switch (fmt_in->datatype) {

			case CS_CHAR_TYPE:	 
			case CS_DATETIME_TYPE:	strncpy((char *) buf,  (char *) p_val, fmt_out->maxlength);
						((char *) buf)[fmt_out->maxlength - 1]  = '\0';
						break;
			case CS_FLOAT_TYPE:	sprintf((char *) buf,"%f", (*(double *)p_val));
						break;
			case CS_INT_TYPE:	sprintf((char *) buf,"%d", (*(int *)p_val));
						break;
			default:
						*out_len = 0;
						return CS_FAIL;
						break;
		}
		*out_len = strlen ((char *) buf) + 1;

	}
	else if ( fmt_in->datatype == CS_CHAR_TYPE ) {

		switch (fmt_out->datatype) {

			case CS_CHAR_TYPE:	 
			case CS_DATETIME_TYPE:	strncpy((char *) buf,  (char *) p_val, fmt_out->maxlength);
						((char *) buf)[fmt_out->maxlength - 1]  = '\0';
						*out_len = strlen ((char *) buf) + 1;
						break;
			case CS_FLOAT_TYPE:	(*(double *)buf) = atof((char *) p_val);
						*out_len = 8;
						break;
			case CS_INT_TYPE:	(*(int *)buf) = atoi((char *) p_val);
						*out_len = 4;
						break;
			default:
						*out_len = 0;
						return CS_FAIL;
						break;
		}
	}
	else {
		return CS_FAIL;
	}

	return CS_SUCCEED;

} /* ---------------- End of cs_convert -------------------- */



/**********************************************************
 cs lib date crack functions

 dateval must be null terminated of of format:
 	YYYY-MM-DD HH:MM:SS:999
 The minimum format must be: YYYY-MM-DD

 **********************************************************/

extern CS_RETCODE CS_PUBLIC cs_dt_crack ( 
		void *context,
		int datetype,
		void *dateval,
		CS_DATEREC *daterec )
{
	char	buf[10];
	int	date_len;
	XMLTP_API_CHAR_DATE_T *pt;

	pt =  (XMLTP_API_CHAR_DATE_T *) dateval;
	date_len = strlen( (char *) dateval);

	/* Date */
	if (date_len >= 10) {

		strncpy(buf,pt->year, 4);
		buf[4]='\0';
		daterec->dateyear = atoi(buf);

		strncpy(buf,pt->month, 2);
		buf[2]='\0';
		daterec->datemonth = atoi(buf)-1;

		strncpy(buf,pt->day, 2);
		buf[2]='\0';
		daterec->datedmonth = atoi(buf);
	}
	else {
		return CS_FAIL;
	}
		
	/* Hour */
	if (date_len >= 13) {

		strncpy(buf,pt->hour, 2);
		buf[2]='\0';
		daterec->datehour = atoi(buf);
	}
	else {
		daterec->datehour = 0;
	}

	/* Minute */
	if (date_len >= 16) {

		strncpy(buf,pt->minute, 2);
		buf[2]='\0';
		daterec->dateminute = atoi(buf);
	}
	else {
		daterec->dateminute = 0;
	}

	/* Second */
	if (date_len >= 19) {

		strncpy(buf,pt->second, 2);
		buf[2]='\0';
		daterec->datesecond = atoi(buf);
	}
	else {
		daterec->datesecond = 0;
	}

	return CS_SUCCEED;

} /* ---------------- End of cs_dt_crack ------------------ */


/* ------------------------- End of xmltp_api_compat.c ----------------------------- */
