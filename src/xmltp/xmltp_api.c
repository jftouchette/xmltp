/***************************************************************
 xmltp_api.c

 All programs using ct10api should be able to work
 with a TRG-X using this module to replace ct10api.

 All input and output are compatible with ct10api.
 The main structure use is CT10_DATA_XDEF. 

 Since name of functions and some arguments are not the same
 a compatibility module is require: xmltp_api_compat.

 TO DO
 -----

 1. Free of the sock object (should be a function in sock_obj module?)
 2. Free memory in xmltp_api_create_connection if error after calloc
 3. Use sock_obj for socket send to replace sock fn *** DONE ***
 4. Do we need a retry loop on the rpc_send??? **YES** xmltp_api_reco required

 LIST OF CHANGES:
 ----------------

 2002-02-13, jbl:	Initial release.
 2002-03-14, jbl: 	Start work on processing results.
 2003-06-02, jbl:	Rename xmltp_writer_add_param with
 			xmltp_writer_param_value
 2003-06-26, jbl:	Add functions prototype, define,
 			some clean-up, move to version 0.9
 2003-08-03, jbl:	set trace level in xmltp_ctx
 			val + 10, impossible to move back to 0 for now.
			Work required in vbacs and ct10dll.
 2003-10-10, jbl:	Fixed alligment problem (bus error) on HP-UX.
 			Replace static char 256X256 with
			a calloc (256) X 256 column
			calloc make sure that any datatype will be
			well allign.
 2003-10-20, jbl:	Fixe problem of a static sock_obj in xmltp_writer.
 2004-03-30, jbl:	Added support for timeout.
 2004-04-02, jbl:	Added: xmltp_api_reco 

 ***************************************************************/

#define _INCLUDE_XOPEN_SOURCE_EXTENDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>

#include "xmltp_api_compat.h"

#include "sock_obj.h"

#include "apl_log1.h"
#include "debuglvl.h"

#include "javaSQLtypes.h"

#include "xmltp_hosts.h"

#include "xmltp_api.h"
#include "xml2db.h"

#include "xmltp_api_reco.h"



/******* Defines **********************************************/

#define VERSION_NUM		  "1.0"
#define MODULE_NAME		  (__FILE__)
#define VERSION_ID		  (MODULE_NAME " " VERSION_NUM " - " __DATE__ " - " __TIME__)
#define APL_LOG_DEFAULT_NOTIFY	  "nobody"

#define SUCCESS			   0
#define FAILED			   -1

#define CONN_SIGNATURE            0x3C12AB12
#define INVALID_SIGNATURE         -40
#define VALID_SIGNATURE           SUCCESS

#define MAX_NB_PARM		 256
#define MAX_STRING_SIZE		 256

/* ct10rpc_parse stuff */
#define RPC_SYNTAX_OK		-999

#define HEADER_IND_FALSE        0
#define HEADER_IND_TRUE         1
#define HEADER_IND_NB_ROW       2

#define RPC_MAXSIZE		1024

/* xmltp_hosts related stuff */
#define ENV_XMLTP               "XMLTP"
#define PATH_SIZE               256
#define HOSTNAME_SIZE           256
#define LISTEN_PORT_SIZE        32
#define XMLTP_HOST_FILE_NAME    "xmltp_hosts"


#define CFG_STRING_SOCKET_TIMEOUT		"EXEC_TIMEOUT"


/******* Private data ****************************************/

static int s_debug_trace_level=DEBUG_TRACE_LEVEL_NONE; 

static char s_log_buf[8096];

/* Log application message type table. */
static struct int_str_tab_entry s_type_strings_table[] = {
#include "alogtypt.h"
 
        { ISTR_NUM_DEFAULT_VAL,  "?     " }
};

static char *s_appl_name; /* application name use by log functions */

/* Not thread safe, trick to find active connection from call back */
static XMLTP_API_CONN_T *s_p_current_active_conn;

/* Needed for result set processing */
/* p_val is a void pointer, for char result no strcpy required	*/
/* we will use the pointer from yyparse callback		*/
/* for int and float result we need a place to put the 		*/
/* converted value.						*/

static XMLTP_API_DATA_XDEF_T	s_one_row_result[MAX_NB_PARM]; 
static XMLTP_API_DATA_XDEF_T	*sp_one_row_result[MAX_NB_PARM];
static int			s_nb_column=0;
static int			s_result_set_no=0;

/* Needed for error recovery */
static int			s_result_present=0;
static int			s_error_class=0;



/******* Private functions prototype ***********************/

static void handle_pipe_signal( int signal_no );
static void sf_default_error_msg_handler( char	*msg_no, char	*msg_text);
static void sf_init_static_array_of_pointer();
static int sf_init_log(char *logname);
static int sf_log(	int     err_level,
			int     msg_no,
			char    *msg_text,
			int     level_to_log);
static int sf_check_xmltp_api_conn_signature(void *p_api_ctx);

static void sf_change_connection_params(XMLTP_API_CONN_T *p_api_ctx,
                                char *login_id,
                                char *password,
                                char *app_name,
                                char *host_name,
                                char *server_name,
				char *service);
static int sf_open_connection(void *p_api_ctx);
static int sf_login(void *p_api_ctx);
static int sf_write_params(	XMLTP_API_CONN_T 		*p_api_ctx,
                     		XMLTP_API_DATA_XDEF_T	*params_array[],
                     		int 			nb_params);
static int sf_print_string(int width, char *string, int b_align_char_type);
static int sf_print_string(int width, char *string, int b_align_char_type);
static int  sf_default_process_result_rows_or_params (
	void	 	 	*p_api_ctx,
	int 		 	b_is_row,
	XMLTP_API_DATA_XDEF_T 	*data_array[],
	int		 	nb_data,
	int		 	result_set_no,
	int	 		result_type,
	int		 	header_ind );
static void sf_fake_csdatafmt_from_yyparse_callback(char *name, int type, CS_DATAFMT *fmt, char *opt_size);

static void (* s_pf_error_msg_handler)() =  sf_default_error_msg_handler;


/******* Private functions ***********************************/



/*******************************************************************
       Dummy signal handler the inhibits the default behaviour when
       this signal is caught.  By default the process is aborted.
       this signal handler does nothing.
 *******************************************************************/

static void handle_pipe_signal( int signal_no )
{
#ifndef WIN32
        signal( SIGPIPE, SIG_IGN );
        signal( SIGPIPE, handle_pipe_signal );
#endif 

        return;

} /* ------- End of handle_pipe_signal() ---------------- */



/***********************************************************
 To be compatible with ct10api we need a array of pointer to
 the XDEF structure. Since I dont want any dynamic allocation
 here we affect the pointer from a real array. 
 
 At the same time we affect p_val with a 256 bytes space.
 ***********************************************************/

static void sf_init_static_array_of_pointer()
{
	int i;

	for (i=0; i<MAX_NB_PARM; i++) {

		if ( (s_one_row_result[i].p_val = calloc(1,MAX_STRING_SIZE + 1))  == NULL ) {
			sprintf(s_log_buf,"sf_init_static_array_of_pointer: calloc memory allocation error");
			sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
			return;
		}

		(sp_one_row_result[i]) = &(s_one_row_result[i]);
	}

	return;

} /* ----- End of sf_init_static_array_of_pointer ---------- */



/********************************
 Init log system.
 *******************************/
 
static int sf_init_log(char *logname)
{
int     rc;
 
if ((rc = alog_set_log_filename(logname)) != 0) {
        sprintf(s_log_buf,"sf_init_log: alog_set_log_filename error, rc [%d]", rc);
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return FAILED;
}
 
if ((rc = alog_bind_message_types_table(&s_type_strings_table[0],
                                    sizeof(s_type_strings_table) ))
                   != 0) {

        sprintf(s_log_buf,"sf_init_log: alog_bind_message_types_table error rc [%d]", rc);
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return FAILED;
}

sprintf(s_log_buf,"sf_init_log: log openned [%s]", logname);
sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
 
return SUCCESS;
} /* --------------------------- End of sf_init_log -------------------- */



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
 
static int sf_log(	int     err_level,      /* ALOG_INFO, ALOG_WARN, ... */
			int     msg_no,         /* Message number.           */
			char    *msg_text,      /* message text.             */
			int     level_to_log)   /* Trace level needed to log.*/
{
 
return (alog_aplogmsg_write(	err_level, MODULE_NAME, s_appl_name,
			msg_no, APL_LOG_DEFAULT_NOTIFY, msg_text));

} /* --------------------------- End of sf_log --------------------- */


static void sf_log2 (int err_level, int msg_no, char *msg1, char *msg2)
{
	sprintf(s_log_buf,"xmltp_ctx: %.1100s %.1100s", msg1, msg2);
	sf_log(err_level, msg_no, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
}


/*****************************
 Check connection signature.
 *****************************/
 
static int sf_check_xmltp_api_conn_signature(void *p_api_ctx)
{
XMLTP_API_CONN_T *pt;

if (p_api_ctx == NULL) {
        sprintf(s_log_buf,"sf_check_xmltp_api_conn_signature: p_api_ctx is null");
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return INVALID_SIGNATURE;
}

pt = p_api_ctx;
 
if (pt->signature != CONN_SIGNATURE) {
        sprintf(s_log_buf,"sf_check_xmltp_api_conn_signature: Invalid signature");
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return INVALID_SIGNATURE;
}
 
return VALID_SIGNATURE;
 
} /* --------------------------- End of sf_check_xmltp_api_conn_signature --- */



/********************************************************************************
 Connection information setting
 ********************************************************************************/

static void sf_change_connection_params(XMLTP_API_CONN_T *p_api_ctx,
                                char *login_id,
                                char *password,
                                char *app_name,
                                char *host_name,
                                char *server_name,
				char *service) {

	if (sf_check_xmltp_api_conn_signature(p_api_ctx) == INVALID_SIGNATURE) {
		return;
	}
 
        if (login_id != NULL) {
                strncpy(p_api_ctx->login_id, login_id, 32);
        }
 
        if (password != NULL) {
                strncpy(p_api_ctx->password, password, 64);
        }
 
        if (app_name != NULL) {
                strncpy(p_api_ctx->app_name, app_name, 32);
        }
 
        if (host_name != NULL) {
                strncpy(p_api_ctx->host_name, host_name, 32);
        }
 
        if (server_name != NULL) {
                strncpy(p_api_ctx->server_name, server_name, 32);
        }

        if (service != NULL) {
                strncpy(p_api_ctx->service, service, 32);
        }

	return;

} /* ----- End of sf_change_connection_params -------------- */



/************************************************************
 Open xmlt-l connection
 ************************************************************/

static int sf_open_connection(void *p_api_ctx)
{
int rc, timeout_val;
XMLTP_API_CONN_T *pt = NULL;
char *timeout_string;

pt = (XMLTP_API_CONN_T *) p_api_ctx;

#ifndef WIN32
signal( SIGPIPE, handle_pipe_signal );
                /* sock_obj takes care of the EPIPE error
                 * we need to deactivate the SIGPIPE default
                 * treatment that aborts the process
                 */
#endif
 
if ( (rc = sock_obj_connect(pt->s_p_sock_obj)) <= -1) {
        sprintf(s_log_buf, 
	"sf_open_connection: (pid=%d) Connection fail for '%s.%s' Reason: %s",
                getpid(),
                pt->host_name,
                pt->service,
                sock_obj_get_last_error_diag_msg(pt->s_p_sock_obj) );
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return FAILED;
}



/* Default timeout value */
timeout_val = 300;

/* Try to read timeout value from config file */
if ( (timeout_string = (char *) ucfg_get_param_string_first(CFG_STRING_SOCKET_TIMEOUT) ) != NULL ) {
	timeout_val = atoi(timeout_string);
}

if ( ( rc = sock_set_timeout(sock_obj_fd(pt->s_p_sock_obj), timeout_val ) ) != 0 ) {

        sprintf(s_log_buf, 
	"sf_open_connection: (pid=%d) failed to set socket timeout for '%s.%s'",
                getpid(),
                pt->host_name,
                pt->service);
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return FAILED;
}

if ( (rc = xmltp_ctx_assign_socket_fd(pt->p_ctx, sock_obj_fd(pt->s_p_sock_obj) ) ) != SUCCESS) {
        sprintf(s_log_buf, 
	"sf_open_connection: Fail to update SD in xmltp_ctx context (pid=%d) '%s.%s'",
                getpid(),
                pt->host_name,
                pt->service,
                sock_obj_get_last_error_diag_msg(pt->s_p_sock_obj) );
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return FAILED;

}
 
sprintf(s_log_buf, "Socket connection on '%s.%s' successfull [%x], timeout [%d]",
		pt->host_name, pt->service, pt->s_p_sock_obj, timeout_val);
sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);


return SUCCESS;
} /* ---------- End of sf_open_connection --------- */



/************************************************************

  Call the login RP to initiate a xmltp-l connection

  RC:	 0 Success
	-1 exec_rpc_string fail
	-2 login fail
 ************************************************************/

static int sf_login(void *p_api_ctx)
{
int rc;
int return_stat;
char rpc_string[255];
XMLTP_API_CONN_T *pt = NULL;

pt = (XMLTP_API_CONN_T *) p_api_ctx;

        sprintf(rpc_string,
                "login @p_usr_id=\"%.32s\", @p_password=\"%.64s\"",
		pt->login_id, pt->password );

        if ((rc = xmltp_api_exec_rpc_string(
                p_api_ctx,
                rpc_string,
                &return_stat)) != SUCCESS) {
			
			sprintf(s_log_buf, "sf_login: login error, exec_rpc_string rc:[%d]", rc);
			sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
			return FAILED;
	}

	if ( return_stat != 0 ) {

		sprintf(s_log_buf, "sf_login: login error, return_stat :[%d]", return_stat);
		sf_log(ALOG_WARN, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		return FAILED;
	}

	return SUCCESS;


} /* ---- End of sf_login ------ */



/******************************************************
 * Write params on the xmltp-l output stream
 ******************************************************/

static int sf_write_params( XMLTP_API_CONN_T 		*p_api_ctx,
                     XMLTP_API_DATA_XDEF_T	*params_array[],
                     int 			nb_params)
{
	int 			rc=0;
	char 			buff[300];

	int			java_type;
	int			i=0;

	if  (DEBUG_TRACE_LEVEL_LOW <= s_debug_trace_level) {
		sprintf(s_log_buf, "sf_write_params: nb_params [%d]", nb_params);
		sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_LOW);
	}

	for (i=0; i<nb_params; i++) {

		switch ( (params_array[i])->fmt.datatype) {
		case CS_FLOAT_TYPE:
					sprintf(buff, "%f", *((double *)(params_array[i]->p_val)) );
					java_type = JAVA_SQL_FLOAT;
					break;
		case CS_INT_TYPE:
					sprintf(buff, "%d", *((int *)(params_array[i]->p_val)) );
					java_type = JAVA_SQL_INTEGER;
					break;
		case CS_DATETIME_TYPE:
					sprintf(buff, "%.256s", (params_array[i])->p_val);
					java_type = JAVA_SQL_TIMESTAMP;
					break;
		case CS_CHAR_TYPE:
					sprintf(buff, "%.256s", (params_array[i])->p_val);
					java_type = JAVA_SQL_VARCHAR;
					break;
		default:
					sprintf(s_log_buf, "sf_write_params: Unsupported, type [%d], Param no[%d]", 
							(params_array[i])->fmt.datatype, i+1);
					sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
					return FAILED;
					break;
		}

		if  (DEBUG_TRACE_LEVEL_LOW <= s_debug_trace_level) {
			sprintf(s_log_buf, "sf_write_params: pos[%d], name[%.20s], value[%.100s]",
				       	i, params_array[i]->fmt.name, buff);
			sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_LOW);
		}
		
		if (( rc = xmltp_writer_param_value(
				p_api_ctx->p_ctx,
				params_array[i]->fmt.name,
				java_type,
				0,		/* We dont care for the size of string on params */
				((params_array[i])->fmt.status == CS_RETURN)
					? 1 : 0 ,
				((params_array[i])->null_ind == -1)
					? 1 : 0,
				 buff)) != 0) {

			sprintf(s_log_buf, "sf_write_params: xmltp_writer_param_value error  rc:[%d]", rc);
			sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

			return FAILED;
		} 	
	}

	return SUCCESS;

} /* ---------- End of sf_write_params ------------- */



/*******************************************************************
 Print string[] on stdout on a field of width characters.
 If b_align_char_type is true, align on left, otherwise of right.

 use only in the default result handler

 Return 0.
 *******************************************************************/

static int sf_print_string(int width, char *string, int b_align_char_type)
{
        printf( (b_align_char_type) ? " %-*.*s" : " %*.*s",
                width,
                width,
                string);
 
        return SUCCESS;
 
} /* end of sf_print_string() */



/**********************************************************************
 Default results processing.
 Output to stdout (isql format)

 This function is call many times for every parts of the results.

	Result set nb rows
		header_ind = HEADER_IND_NB_ROW
		if result_set_no <> -1
		then result_set_no = (number or rows affected)
		return

	Output params header
		header_ind  = HEADER_IND_TRUE
		result_type = PARAM_RESULT

		print "Return parameter:"
		continue

	Result set & output param header
		header_ind  = HEADER_IND_TRUE
		result_type = PARAM_RESULT, ROW_RESULT

		print header
		return
		
	Result set & output param data
		header_ind  = HEADER_IND_FALSE
		result_type = PARAM_RESULT, ROW_RESULT

		convert & print data
		return

	Status result
		Not process here


 **********************************************************************/

static int  sf_default_process_result_rows_or_params (
	void	 	 	*p_api_ctx, 	/* xmltp_api connection context */
	int 		 	b_is_row,	/* Not used */
	XMLTP_API_DATA_XDEF_T 	*data_array[],	/* Array of columns/params result */
	int		 	nb_data,	/* Nb columns or params */
	int		 	result_set_no,  /* Result set no or nb rows */
	int	 		result_type,	/* PARAM_RESULT, ROW_RESULT */
	int		 	header_ind )	/* HEADER_IND_TRUE, HEADER_IND_FALSE */
{
int i;
char buf[RPC_MAXSIZE];
int print_len[RPC_MAXSIZE];
char *dash={"--------------------------------------------------------------------------------------------------"};
char plurial[2];
int rc2;


if (header_ind == HEADER_IND_NB_ROW) {

	if (result_set_no != -1) {
		if (result_set_no > 1)
			strcpy(plurial,"s");
		else
			strcpy(plurial,"");

		printf("\n(%d row%s affected)\n",result_set_no,plurial); 
	}

	return SUCCESS;
}

if ((header_ind == HEADER_IND_TRUE) && 
    (result_type == CS_PARAM_RESULT)) {

	printf("\nReturn parameter:\n\n");
}

for (i=0; i<nb_data; ++i) {

	/* setting output size base on datatype. */
	switch ((data_array[i])->fmt.datatype ) {
		case CS_INT_TYPE:	print_len[i] = 11;
					break;

		case CS_DATETIME_TYPE:	print_len[i] = 23;
					break;

		case CS_CHAR_TYPE:	print_len[i] = (data_array[i])->fmt.maxlength;
					break;

		case CS_FLOAT_TYPE:	print_len[i] = 20; /* Why 20??? */
					break;

		default:		
				sprintf(s_log_buf, "sf_default_process_result_rows_or_params: 1.unsupported type [%d]",
					       	(data_array[i])->fmt.datatype);
				sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
				print_len[i] = 15; /* why 15??? */
				break;
	}

	/* Adjust size of column to size of header. */
	if ( ((int) strlen((data_array[i])->fmt.name)) > print_len[i]) {
		print_len[i] = strlen((data_array[i])->fmt.name);
	}
	
	/* Print columns or params header   */
	if (header_ind == HEADER_IND_TRUE) {
		sf_print_string(print_len[i], (data_array[i])->fmt.name,
				1);	/* 1==align string */
	}
	else {
		/* Prepare to print values */

		if ((data_array[i])->null_ind == -1) {
			strcpy(buf,"NULL");
		}
		else {

			/* Convert to string */
			switch ((data_array[i])->fmt.datatype ) {
				case CS_INT_TYPE:	sprintf(buf,"%d", *((int *) data_array[i]->p_val));
							break;
				case CS_CHAR_TYPE:	
				case CS_DATETIME_TYPE:	strncpy(buf, (data_array[i])->p_val, (data_array[i])->fmt.maxlength);
							break;
				case CS_FLOAT_TYPE:	sprintf(buf,"%f", *((double *) data_array[i]->p_val) );
							break;
				default:
						sprintf(s_log_buf, "sf_default_process_result_rows_or_params: 2.unsupported type [%d]",
					       	(data_array[i])->fmt.datatype);
						sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
						break;
			} /* End of switch */

		} /* End of else not null */

		sf_print_string(print_len[i], buf, 
			((data_array[i])->fmt.datatype == CS_CHAR_TYPE) );
	}
 } /* for each column */


 printf("\n");

 if (header_ind == HEADER_IND_TRUE) {
	for (i=0; i<nb_data; ++i) {
		sf_print_string(print_len[i], dash, 1);	/* 1==align string */
	}
	printf("\n");
 }

return SUCCESS;
} /* ---------- End of sf_default_process_result_rows_or_params  ------- */



/******************************************************************
 Construct a CS_DATAFMT from the yyparse callback.
 ******************************************************************/

static void sf_fake_csdatafmt_from_yyparse_callback(char *name, int type, CS_DATAFMT *fmt, char *opt_size)
{

	if  (DEBUG_TRACE_LEVEL_FULL <= s_debug_trace_level) {
		sprintf(s_log_buf, "sf_fake_csdatafmt_from_yyparse_callback: name [%s], type [%d]\n", name, type);
		sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_FULL);
	}

	fmt->count = 1;
	fmt->locale = NULL;
	fmt->status = 0;
	fmt->scale = 0;
	fmt->precision = 0;
	fmt->status = 0;
	fmt->usertype = 0;

	if (strlen(name) == 0) {
		strcpy(fmt->name, "");
		fmt->namelen = 0; 
	}
	else {
		strncpy(fmt->name, name, CS_MAX_NAME);
		fmt->namelen = strlen(fmt->name);
	}

	switch (type) {
		case JAVA_SQL_TIMESTAMP:
		case JAVA_SQL_DATE:
		case JAVA_SQL_TIME:
					fmt->datatype = CS_DATETIME_TYPE;
					fmt->format = CS_FMT_NULLTERM;
					fmt->maxlength = 23;
					break;
		case JAVA_SQL_NUMERIC:
		case JAVA_SQL_DOUBLE:
		case JAVA_SQL_FLOAT:
					fmt->datatype = CS_FLOAT_TYPE;
					fmt->maxlength = 8;
					fmt->format = CS_FMT_UNUSED;
					break;
		case JAVA_SQL_INTEGER:
		case JAVA_SQL_SMALLINT:
					fmt->datatype = CS_INT_TYPE;
					fmt->maxlength = 4;
					fmt->format = CS_FMT_UNUSED;
					break;
		case JAVA_SQL_VARCHAR:
		case JAVA_SQL_CHAR:
					fmt->datatype = CS_CHAR_TYPE;
					fmt->format = CS_FMT_NULLTERM;
					if (opt_size == NULL) {
						sprintf(s_log_buf,
						"sf_fake_csdatafmt_from_yyparse_callback: opt_size is NULL size defaulted to 255");
						sf_log(ALOG_WARN, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

						fmt->maxlength = MAX_STRING_SIZE;
					}
					else {
						fmt->maxlength = atoi(opt_size);
					}

					break;
		default:
					sprintf(s_log_buf,
					"sf_fake_csdatafmt_from_yyparse_callback: Invalid JAVA_SQL_TYPE [%d]\n", type);
					sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
					break;

	}

	return;

} /* ------- End of sf_fake_csdatafmt_from_yyparse_callback ------------ */



/*********************************************************
 Default message handler
 *********************************************************/

static void sf_default_error_msg_handler(
 			char	*msg_no,
 			char	*msg_text)
{

	sprintf( s_log_buf,"errno [%s]: %s", msg_no, msg_text );
      	sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

	return;

} /* ------ End of sf_default_error_msg_handler ------ */



/******* Public functions ************************************/



/*******************************************
 Set trace level.
 *******************************************/

void xmltp_api_set_debug_trace_flag(int val)
{
        s_debug_trace_level = val;
	sprintf(s_log_buf, "xmltp_api_set_debug_trace_flag: New trace level [%d]", val);
	sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

	xmltp_ctx_assign_log_function_pf(sf_log2);

	xmltp_ctx_assign_debug_level(val + 10);
	 
	return;
} /* ------- End of xmltp_api_set_debug_trace_flag ----- */



/*******************************************
 Get trace level.
 *******************************************/

int xmltp_api_get_debug_trace_flag()
{
        return (s_debug_trace_level);
} /* ------- End of xmltp_api_set_debug_trace_flag ----- */



/******************************************
 Assign result processing function pointer.
 ******************************************/
 
int xmltp_api_assign_process_row_function(void *p_api_ctx,
        int (* pf_process_row_or_params)(
			void 			*p_api_ctx, 
			int 			b_is_row,
			XMLTP_API_DATA_XDEF_T	*data_array[],
			int 			nb_data,
			int 			result_set_no,
			int 			result_type,
			int 			header_ind)  )
{
	XMLTP_API_CONN_T *pt;

	if (sf_check_xmltp_api_conn_signature(p_api_ctx) == INVALID_SIGNATURE) {
		return XMLTP_ERR_BAD_CONN_HANDLE;
	}
 
	pt = p_api_ctx;
 
	pt->pf_process_row_or_params = pf_process_row_or_params;
 
	return XMLTP_SUCCESS;
 
} /* ----- End of xmltp_api_assign_process_row_function ------------- */



/*************************
 Create connection.
 *************************/
 
void *xmltp_api_create_connection(	char *login_id, 
					char *password,
                                	char *app_name, 
                                	char *server_name,
					char *init_string,
					long options_mask)
{
XMLTP_API_CONN_T	*p_api_ctx=NULL;
char			xmltp_host_file[PATH_SIZE];
int			rc;
char			host_name[HOSTNAME_SIZE];
char			listen_port[LISTEN_PORT_SIZE];


/* Build path and name of xmltp_host file from environment variable */
if (getenv(ENV_XMLTP) != NULL) {

	strncpy(xmltp_host_file, getenv(ENV_XMLTP), PATH_SIZE);

	if ( xmltp_host_file[strlen(xmltp_host_file)-1] != '/' ) {
		strncat(xmltp_host_file, "/", PATH_SIZE);
	}

	strncat(xmltp_host_file, XMLTP_HOST_FILE_NAME, PATH_SIZE);

	sprintf(s_log_buf,"xmltp_api_create_connection: XMLTP hosts file location [%s]", xmltp_host_file);
        sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

} else {
	sprintf(s_log_buf,"xmltp_api_create_connection: XMLTP environment variable not set");
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
	return NULL;
}


/* Get host name and socket number from the xmltp_hosts file */
if (rc = xmltp_hosts(   xmltp_host_file,
			server_name,
			host_name,
			listen_port ) == 0 ) {

	sprintf(s_log_buf, "xmltp_api_create_connection: hosts file [%.64s],logical[%.20s], hosts[%.20s], port[%.6s]",
		xmltp_host_file,
		server_name,
		host_name,
		listen_port);
	sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

}
else {
	sprintf(s_log_buf,"xmltp_api_create_connection: logical name [%.20s] not found in [%.64s]",
			server_name,
			xmltp_host_file);
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
	return NULL;
}


sprintf(s_log_buf, 
"xmltp_api_create_connection(login[%.20s], app[%.20s], host[%.20s], srv[%.20s], init[%.50s], service[%.6s])",
login_id, app_name, host_name, server_name, init_string, listen_port);
sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);


/* Allocate XMLTP_API_CONN_T structure. */
if ( (p_api_ctx = calloc(1,sizeof(XMLTP_API_CONN_T)) ) == NULL ) {
        sprintf(s_log_buf,"xmltp_api_create_connection: calloc memory allocation error");
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return NULL;
}

/* Initialize CT10_CONN_T structure. */
 
p_api_ctx->signature = CONN_SIGNATURE;

sf_change_connection_params(
        p_api_ctx,
        login_id,
        password,
        app_name,
        host_name,
        server_name,
	listen_port);
 
if (init_string != NULL) {
        strcpy(p_api_ctx->init_string, init_string);
}
else {
        strcpy(p_api_ctx->init_string, "");
}
 
p_api_ctx->options_mask = options_mask;


/* Here we install the default result set processing function	*/
/* The default is to output to stdout the results		*/
/* For anything else custom result processing function is 	*/
/* required.							*/

if (xmltp_api_assign_process_row_function(
		p_api_ctx, 
		sf_default_process_result_rows_or_params)  != SUCCESS ) {

        sprintf(s_log_buf,"xmltp_api_create_connection: Error in xmltp_api_assign_process_row_function");
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
	return NULL;
}

/* Create context for yyparse */
if ( (p_api_ctx->p_ctx = (void *) xmltp_ctx_create_new_context()) == NULL) {
        sprintf(s_log_buf,"xmltp_api_create_connection: Error in xmltp_ctx_create_new_context");
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return NULL;
}

/* Ready yyparse ... */
if ( xmltp_ctx_reset_lexer_context(p_api_ctx->p_ctx) != 0) {
        sprintf(s_log_buf,"xmltp_api_create_connection: Error in xmltp_ctx_reset_lexer_context");
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return NULL;
}

/* This is required only one time but could be recall for each connection... */
sf_init_static_array_of_pointer();

/* Creation of sock_obj structure */
p_api_ctx->s_p_sock_obj = sock_obj_create_socket_object(
			p_api_ctx->host_name,	/* Host name, require entry in /etc/hosts */
			p_api_ctx->service,	/* service name, require entry in /etc/services */
                        0,   			/* connect_max_retry            */
                        0       		/* b_reconnect_on_reset_by_peer */
                        );
 
if (p_api_ctx->s_p_sock_obj == NULL) {

        sprintf(s_log_buf, 
	"xmltp_api_create_connection: (pid=%d) failed to create SOCK_OBJ for '%s.%s' Reason: %s",
                getpid(),
                p_api_ctx->host_name,
                p_api_ctx->service,
                sock_obj_get_last_error_diag_msg(p_api_ctx->s_p_sock_obj) );
        sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        return NULL;
}


return p_api_ctx;
} /* ------- End of xmltp_api_create_connection ------- */



/************************************************************
 Require a valid connection context structure.

 If the connection is already open, it will be closed and
 reopen.

 ************************************************************/

int xmltp_api_connect(void *p_api_ctx) 
{
	int rc;

	if (sf_check_xmltp_api_conn_signature(p_api_ctx) == INVALID_SIGNATURE) {
        	sprintf(s_log_buf,"xmltp_api_connect: invalid signature");
        	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		s_error_class = ERR_CLASS_CLT_INTERNAL_ERR;
		return XMLTP_ERR_BAD_CONN_HANDLE;
	}

	if ( (rc = xmltp_api_close_connection(p_api_ctx) ) == XMLTP_ERR_BAD_CONN_HANDLE) {
        	sprintf(s_log_buf,"xmltp_api_connect: Error closing connection (bad signature) rc[%d]", rc);
        	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		s_error_class = ERR_CLASS_CLT_INTERNAL_ERR;
		return rc;
	}

	if ( rc != SUCCESS ) {
        	sprintf(s_log_buf,"xmltp_api_connect: Error closing connection (will try to connect anyway) rc[%d]", rc);
        	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
	}

	if ( (rc = sf_open_connection(p_api_ctx) ) != SUCCESS) {
        	sprintf(s_log_buf,"xmltp_api_connect: Error openning connection rc[%d]", rc);
        	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		s_error_class = ERR_CLASS_CLT_SOCKET_CONNECT_ERR;
		return XMLTP_ERR_FAILED;
	}

	if ( (rc = sf_login(p_api_ctx) ) != SUCCESS) {

        	sprintf(s_log_buf,"xmltp_api_connect: Error on login rc[%d]", rc);
        	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		s_error_class = ERR_CLASS_CLT_LOGIN_ERR;

		if ( (rc = xmltp_api_close_connection(p_api_ctx) ) == XMLTP_ERR_BAD_CONN_HANDLE) {
			sprintf(s_log_buf,"xmltp_api_connect: Error closing connection after sf_login (bad signature) rc[%d]", rc);
			sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
			s_error_class = ERR_CLASS_CLT_INTERNAL_ERR;
			return rc;
		}

		if ( rc != SUCCESS ) {
			sprintf(s_log_buf,"xmltp_api_connect: Error closing connection after sf_login (will try to connect anyway) rc[%d]", rc);
			sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		}

		return XMLTP_ERR_FAILED;
	}

	return XMLTP_SUCCESS;

} /* --------- End of xmltp_api_connect ----------- */



/****************************************************
 Close xmltp-l connection 
 ****************************************************/

int xmltp_api_close_connection( void *p_api_ctx)
{
	int			rc;
	XMLTP_API_CONN_T	*pt;
	pt = p_api_ctx;

	if (sf_check_xmltp_api_conn_signature(p_api_ctx) == INVALID_SIGNATURE) {
        	sprintf(s_log_buf,"xmltp_api_close_connection: invalid signature");
        	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		return XMLTP_ERR_BAD_CONN_HANDLE;
	}

	/* not already closed */
	if (sock_obj_fd(pt->s_p_sock_obj) != -1 ) {
		if ((rc = sock_obj_disconnect(pt->s_p_sock_obj)) != 0) {
        		sprintf(s_log_buf,"xmltp_api_close_connection: sock_obj_disconnect error rc [%d]", rc);
        		sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
			return XMLTP_ERR_FAILED;
		}

		/* xmltp_writer_drop_sock_obj(); */
	}

	return XMLTP_SUCCESS;
} /* ---------- End of xmltp_api_close_connection ----------- */



/****************************************************
 Free structure use for one connection
 ****************************************************/

int xmltp_api_drop_connection( void *p_api_ctx)
{
	int			rc;
	XMLTP_API_CONN_T	*pt;

	pt = (XMLTP_API_CONN_T *) p_api_ctx;

	if (sf_check_xmltp_api_conn_signature(p_api_ctx) == INVALID_SIGNATURE) {
        	sprintf(s_log_buf,"xmltp_api_drop_connection: invalid signature");
        	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		return XMLTP_ERR_BAD_CONN_HANDLE;
	}

	free(pt->s_p_sock_obj); /* sock_obj do not supply a way to drop structure */
	free(p_api_ctx);

	return XMLTP_SUCCESS;
} /* ---------- End of xmltp_api_drop_connection ----------- */



/************************************************
 *
 * Check for yyparse error and return the
 * appropriate error class.
 *
 ************************************************/

static int sf_check_yyparse_error(XMLTP_API_CONN_T *pt, int rc)
{
	if (xmltp_ctx_get_b_eof_disconnect(pt->p_ctx) == 1 ) {

		sprintf(s_log_buf,"yyparse failed:  yyparse rc[%d], b_eof[%d], b_eot[%d], err_ctr[%d]",
						rc,
						xmltp_ctx_get_b_eof_disconnect(pt->p_ctx),
						xmltp_ctx_buffer_contains_eot(pt->p_ctx),
						xmltp_ctx_get_lexer_or_parser_errors_ctr(pt->p_ctx));
       		sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		return(ERR_CLASS_CLT_SOCKET_RECV_ERR);
	} 
	else if (xmltp_ctx_get_b_eof_disconnect(pt->p_ctx) == 2 ) {

		sprintf(s_log_buf,"yyparse failed:  yyparse rc[%d], b_eof[%d], b_eot[%d], err_ctr[%d]",
						rc,
						xmltp_ctx_get_b_eof_disconnect(pt->p_ctx),
						xmltp_ctx_buffer_contains_eot(pt->p_ctx),
						xmltp_ctx_get_lexer_or_parser_errors_ctr(pt->p_ctx));
       		sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		return(ERR_CLASS_CLT_SOCKET_RECV_TIMEOUT);
	}
	else {

		if (xmltp_ctx_get_lexer_or_parser_errors_ctr(pt->p_ctx) != 0) {

			sprintf(s_log_buf,"yyparse failed:  yyparse rc[%d], b_eof[%d], b_eot[%d], err_ctr[%d]",
							rc,
							xmltp_ctx_get_b_eof_disconnect(pt->p_ctx),
							xmltp_ctx_buffer_contains_eot(pt->p_ctx),
							xmltp_ctx_get_lexer_or_parser_errors_ctr(pt->p_ctx));
       			sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
			return(ERR_CLASS_CLT_SOCKET_RECV_ERR);

		}
		else {
			/* yyparse RC not zero */
			if (rc != 0) {

				sprintf(s_log_buf,"yyparse failed:  yyparse rc[%d], b_eof[%d], b_eot[%d], err_ctr[%d]",
								rc,
								xmltp_ctx_get_b_eof_disconnect(pt->p_ctx),
								xmltp_ctx_buffer_contains_eot(pt->p_ctx),
								xmltp_ctx_get_lexer_or_parser_errors_ctr(pt->p_ctx));
				sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
				return(ERR_CLASS_CLT_SOCKET_RECV_ERR);

			} /* if (rc != 0) */

		} /* if (xmltp_ctx_get_lexer_or_parser_errors_ctr(pt->p_ctx) != 0) */

	} /* if (xmltp_ctx_get_b_eof_disconnect(pt->p_ctx) == 2 ) */

	return(0);
} /* ----------------- End of sf_check_yyparse_error ---------------------- */



/*********************************************************************
 Execute an RPC
 *********************************************************************/

int xmltp_api_exec_rpc( void *p_api_ctx,
                        char *rpc_name,
                        XMLTP_API_DATA_XDEF_T  *params_array[],
                        int nb_params,
                        int *p_return_status)
{
	int retry=1;
	int rc=0;
	int retry_count = 0;
	int final_rc=0;
	MASTER_RECOV_ENTRY_T *p_recov_entry;
	
	XMLTP_API_CONN_T *pt;
	pt = p_api_ctx;

	if (sf_check_xmltp_api_conn_signature(p_api_ctx) == INVALID_SIGNATURE) {
        	sprintf(s_log_buf,"xmltp_api_exec_rpc: invalid signature");
        	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
		return XMLTP_ERR_BAD_CONN_HANDLE;
	}


	
	/* require for the callback from yyparse to find the active p_api_ctx */
	s_p_current_active_conn = p_api_ctx;

	/* inside of xmltp_writer sock_obj is a static we need to set it before each rpc */
	xmltp_writer_set_sock_obj(pt->s_p_sock_obj);

	/* to count the number of retry */
	retry_count = 0;

	while (retry==1) {


		++retry_count;

		/* This function ans sub-function will set this to indicate any error */
		s_error_class = 0;

		/* In case we never get a return status from yyparse callback */
		s_p_current_active_conn->return_status=-999;
		*p_return_status=-999;

		/* Socket connect and login if not already done. 		*/
		/* Here we have the possibility of infinite loop because	*/
		/* at some point xmltp_api_connect will call xmltp_api_exec_rpc */
		/* but in order to get there sock_obj_fd(pt->s_p_sock_obj) will */
		/* not be = to -1 (socket connection will be open).		*/
		/* This mean that we should never enter this a second time.     */
		if ( sock_obj_fd(pt->s_p_sock_obj) == -1 ) {

			if ((rc = xmltp_api_connect(p_api_ctx)) != XMLTP_SUCCESS) {
				sprintf(s_log_buf,"xmltp_api_exec_rpc: error in xmltp_api_connect, rc [%d]", rc);
				sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
			}

			if ( rc == XMLTP_ERR_BAD_CONN_HANDLE) {
					return rc;
			}
		}

		/* RPC processing must stop as soon as an error is detected */
		if (s_error_class == 0) {

			/* will be set to 1 as soon a row result are received */
			s_result_present = 0;


			/* We must reset lexer before every RPC call */
			if ( (rc = xmltp_ctx_reset_lexer_context(pt->p_ctx)) != 0) {
        			sprintf(s_log_buf,"xmltp_api_exec_rpc: error in xmltp_ctx_reset_lexer_context, rc [%d]", rc);
        			sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
				s_error_class = ERR_CLASS_CLT_SOCKET_RECV_ERR;
			}

		}

		/* Send the RPC call, xmltp_writer could use a callback to */
		/* set s_error_class = 1 */
		if (s_error_class == 0) {

			rc = xmltp_writer_begin_proc_call(pt->p_ctx, rpc_name);

			rc = sf_write_params(pt, params_array, nb_params);

			rc = xmltp_writer_end_proc_call(pt->p_ctx, rpc_name);
		}

		/* Wait for and process response */
		if (s_error_class == 0) {

			rc = yyparse(pt->p_ctx);

			s_error_class = sf_check_yyparse_error(pt, rc);

		} /* (s_error_class == 0) */

		if (s_error_class == 0) {

			/* From here the RPC execute find between xmltp_api and */
			/* the CIG/TRG. We must have a valid return stat from   */
			/* the RPC and this function must return with success.  */
			final_rc = XMLTP_SUCCESS;

			/* A callback from yyparse did set the return status */
			*p_return_status = s_p_current_active_conn->return_status;

			/* For server side error we must try to convert the return stat */
			/* into an error class. */
			if (*p_return_status != 0) {
				 s_error_class = xmltp_api_reco_find_error_class_by_return_stat(*p_return_status);
				 printf("Trace [%d], [%d]\n", s_error_class, *p_return_status);
			}

		}
		else {
			final_rc = XMLTP_ERR_FAILED;
		}

		/* If no error class we exit with success */
		if (s_error_class == 0) {
			break; 
		}
		else {
			/* We we apply recovery actions */

			p_recov_entry = (MASTER_RECOV_ENTRY_T *) xmltp_api_reco_find_recov_entry_for_error_class(s_error_class);

			sprintf(s_log_buf,"Error class info: search[%d], class[%d], actions[%x], max_retry[%d]",
						s_error_class,
			      			p_recov_entry->error_class,
			      			p_recov_entry->action_mask,
			      			p_recov_entry->max_retry );
			sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);


			if (p_recov_entry->action_mask & RECOV_ACT_RESET_CONNECTION){

				if ( (rc = xmltp_api_close_connection(p_api_ctx) ) == XMLTP_ERR_BAD_CONN_HANDLE) {
					sprintf(s_log_buf,"xmltp_api_exec_rpc: Error closing connection (bad signature) rc[%d]", rc);
					sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
					return rc;
				}

				if ( rc != SUCCESS ) {
					sprintf(s_log_buf,"xmltp_api_exec_rpc: Error closing connection (will retry rpc anyway) rc[%d]", rc);
					sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
				}

			}

			if (p_recov_entry->action_mask & RECOV_ACT_DO_NOT_RETRY){
				return(final_rc);
			}

			if (retry_count > p_recov_entry->max_retry) {
				return(final_rc);
			}

			if (s_result_present == 1) {
				return(final_rc);
			}

			if (p_recov_entry->retry_interval > 0)  {
				sleep(p_recov_entry->retry_interval);
			}
		}

	} /* End while (retry==1) */

	return XMLTP_SUCCESS;

} /* ---------- End of xmltp_api_exec_rpc -------------- */



/**********************************************
 RPC exec (String format).
 **********************************************/
 
int xmltp_api_exec_rpc_string(	void	*p_api_ctx,
				char	*rpc_string,
                            	int	*p_return_status)
{
int rc;
XMLTP_API_DATA_XDEF_T  *params_array[MAX_NB_PARM];
char rpc_name[MAX_STRING_SIZE];
int nb_params;
XMLTP_API_CONN_T *pt;
 
 
 
if  (DEBUG_TRACE_LEVEL_LOW <= s_debug_trace_level) {
	sprintf(s_log_buf,"xmltp_api_exec_rpc_string: rpc_string[%s]", rpc_string);
	sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_LOW);
}
 
 
if ((rc=sf_check_xmltp_api_conn_signature(p_api_ctx)) == INVALID_SIGNATURE) {
       	sprintf(s_log_buf,"xmltp_api_exec_rpc_string: invalid signature");
       	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
	return XMLTP_ERR_BAD_CONN_HANDLE;
}
pt = p_api_ctx;
 
/* Parsing the string fromat RPC. */
if ((rc = ct10rpc_parse(p_api_ctx, rpc_string, rpc_name, MAX_STRING_SIZE,
        params_array, MAX_NB_PARM, &nb_params)) != RPC_SYNTAX_OK) {

       	sprintf(s_log_buf,"xmltp_api_exec_rpc_string: invalid RPC syntax");
       	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

	return XMLTP_ERR_RPC_PARSING_ERROR;
}
 
if  (DEBUG_TRACE_LEVEL_FULL <= s_debug_trace_level) {
        xmltp_api_rpc_dump(rpc_name, params_array, nb_params);
}
 
rc = xmltp_api_exec_rpc(p_api_ctx,
                        rpc_name,
                        params_array,
                        nb_params,
                        p_return_status);
 
ct10rpc_free(rpc_name, params_array, &nb_params);
 
return rc;

} /* ----------------------- End of xmltp_api_exec_rpc_string ----------------- */



/*************************************************
 called from xmltp_parser_api.c
 *************************************************/

void xmltp_api_reset_result_set_no()
{
	s_result_set_no = 1;

	return;
} /* ---- End of xmltp_api_reset_result_set_no() ------ */



/*************************************************
 called from xmltp_parser_api.c
 *************************************************/

void xmltp_api_reset_nb_column()
{
	s_nb_column = 0;

	return;
} /* ---- End of xmltp_api_reset_nb_column() ------ */



/*************************************************
 called from xmltp_parser_api.c
 *************************************************/

void xmltp_api_next_result_set()
{

	++s_result_set_no;

	return;
} /* ---- End of xmltp_api_next_result_set --- */



/*************************************************
 called from xmltp_parser_api.c
 *************************************************/

void xmltp_api_add_column_def(char *name, int type, char *opt_size)
{
	memset(&(s_one_row_result[s_nb_column].fmt), 0, sizeof(s_one_row_result[0].fmt));
	sf_fake_csdatafmt_from_yyparse_callback(name, type, &(s_one_row_result[s_nb_column].fmt), opt_size );

	if  (DEBUG_TRACE_LEVEL_FULL <= s_debug_trace_level) {
		sprintf(s_log_buf, 
			"xmltp_api_add_column_def: colname [%s] [%s] datatype [%d] maxlen [%d] format [%d] colnum [%d]\n", 
			s_one_row_result[s_nb_column].fmt.name,
			(sp_one_row_result[s_nb_column])->fmt.name,
			(sp_one_row_result[s_nb_column])->fmt.datatype,
			(sp_one_row_result[s_nb_column])->fmt.maxlength,
			(sp_one_row_result[s_nb_column])->fmt.format,
			s_nb_column);
        	sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_MEDIUM);
	}

	++s_nb_column;

	return;

} /* ---- End of xmltp_api_add_column_def ----- */



/*************************************************
 called from xmltp_parser_api.c
 Call the result processing function. Could be the
 default or a user define function.
 *************************************************/

void xmltp_api_one_row_ready(
	int header_ind, /* 2:HEADER_IND_NB_ROW, 1:HEADER_IND_TRUE, 0:HEADER_IND_FALSE */
	int result_type,/* 4042:CS_PARAM_RESULT, 4040:CS_ROW_RESULT */
	int nb_rows) 	/* Only use for HEADER_IND_NB_ROW */
{
	int rc;
	int tmp;


	if  (DEBUG_TRACE_LEVEL_FULL <= s_debug_trace_level) {
		sprintf(s_log_buf,
			"xmltp_api_one_row_ready, header_ind[%d], result_type[%d], nb_rows[%d], nb_column[%d]\n",
				header_ind,
				result_type,
				nb_rows,
				s_nb_column );
        	sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_MEDIUM);
	}

	/* error recovery is influence be the presence of row result */
	if ( result_type == 4040 ) {
		s_result_present = 1;
	}


	if (header_ind == HEADER_IND_NB_ROW) {
		tmp = nb_rows;
	}
	else {
		tmp = s_result_set_no;
	}


	rc = (s_p_current_active_conn->pf_process_row_or_params)(
		s_p_current_active_conn,
		0,
		sp_one_row_result,
		s_nb_column,
		tmp,
		result_type,
		header_ind);


	return;
} /* ------- End of xmltp_api_one_row_ready ---------- */



/********************************************************
 Here we add row elem into the DATA_XDEF structure for
 one row.
 ********************************************************/

void xmltp_api_add_row_elem(char *val, int is_null)
{

	if ( s_debug_trace_level >= DEBUG_TRACE_LEVEL_FULL ) {
		sprintf(s_log_buf,
			"xmltp_api_add_row_elem: val [%s], is_null [%d] intval [%d] colnum [%d]\n", 
			val, is_null, atoi(val), s_nb_column);
        	sf_log(ALOG_INFO, 0, s_log_buf, DEBUG_TRACE_LEVEL_FULL);
	}

	if (is_null == 1) {
		s_one_row_result[s_nb_column].len = 0;
		s_one_row_result[s_nb_column].null_ind = -1;
	}
	else {
		s_one_row_result[s_nb_column].null_ind = 0;

		switch (s_one_row_result[s_nb_column].fmt.datatype) {
		case CS_INT_TYPE:
					*( (int *) s_one_row_result[s_nb_column].p_val) = atol(val);
					s_one_row_result[s_nb_column].len = 4;
					break;

		case CS_FLOAT_TYPE:
					*((double *) (s_one_row_result[s_nb_column].p_val)) = strtod(val, (char **) NULL);
					s_one_row_result[s_nb_column].len = 8;
					break;

		case CS_CHAR_TYPE:
		case CS_DATETIME_TYPE:
					strncpy((char *) (s_one_row_result[s_nb_column].p_val), val, MAX_STRING_SIZE);
					s_one_row_result[s_nb_column].len = strlen(val);
					break;
		default:
					sprintf(s_log_buf,
						"xmltp_api_add_row_elem: Invalid type [%d] for column [%s]\n", 
						s_one_row_result[s_nb_column].fmt.datatype,
						s_one_row_result[s_nb_column].fmt.name);
					sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
					break;
		}

	}
	
	++s_nb_column;

	return;
} /* ------- End of xmltp_api_add_row_elem ------ */



/*******************************************************
 Set user define message handler.
 *******************************************************/

void xmltp_api_assign_error_msg_handler(void (* ptr_funct)() )
{

	s_pf_error_msg_handler = ptr_funct;

} /* ------- End of xmltp_api_assign_error_msg_handler ------- */






/********************************************************
 This function is callback from xmltp_parser_api.
 It call the default message handler or the user
 define message handler.
 ********************************************************/

void xmltp_api_error_msg_handler(char *msg_number, char *msg_text) 
{

	(* s_pf_error_msg_handler)(msg_number, msg_text);

} /* ------- End of xmltp_api_error_msg_handler ------ */



/***********************************************************
 Set the SP return status from yyparse
 ***********************************************************/
 
void xmltp_api_set_return_status(int return_status)
{
		s_p_current_active_conn->return_status = return_status;

} /* ------- End of xmltp_api_set_return_status ------- */



/***************************************************************
 Call from xmltp_writer to abort program if any write error

 But we dont want to abort. We need to execute the appropriate
 recovery.
 ***************************************************************/

void xml2db_abort_program(int errno, char *msg1, char *msg2)
{
	int rc=0;

	s_error_class = ERR_CLASS_CLT_SOCKET_SND_ERR;

	/* rc = xmltp_api_close_connection(s_p_current_active_conn);
	rc = xmltp_api_drop_connection(s_p_current_active_conn);	*/

	sprintf(s_log_buf, "xml2db_abort_program:  xmltp_writer request abort errno[%d], msg1[%s], msg2[%s]", 
	      		errno, msg1, msg2 );
	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

} /* ------- End of xml2db_abort_program ------- */



/******************************************************
 Look like sf_log_init_msg but with bigger messages.
 ******************************************************/

void xml2db_write_log_msg_mm(int err_level, int msg_no, char *msg1, char *msg2)
{
        char    msg_buff[CS_MAX_MSG + 600] = "msg?!";

        sprintf(msg_buff, "%.300s %.300s", msg1, msg2);

        sf_log(err_level, msg_no, msg_buff, DEBUG_TRACE_LEVEL_NONE);

} /* End of sf_write_log_msg_mm() */



/*******************************************************
 Return (p_conn->server_name)
 or NULL if p_conn is not a valid CT10_CONN_T struct.
 *******************************************************/

char *xmltp_api_get_server_name_of_conn(void *p_api_ctx)
{
        XMLTP_API_CONN_T     *pt = NULL;
	int rc;

	if ((rc=sf_check_xmltp_api_conn_signature(p_api_ctx)) == INVALID_SIGNATURE) {
       		sprintf(s_log_buf,"xmltp_api_get_server_name_of_conn: invalid signature");
       		sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        	return (NULL);
	}
	pt = p_api_ctx;

        return ( &(pt->server_name[0]) );

} /* ----- End of xmltp_api_get_server_name_of_conn ----- */



/***************************************************************
 Application name required for log 
 ***************************************************************/

void xmltp_api_assign_application_name(char *app_name)
{
	s_appl_name = app_name;

	return;
} /* --------------------------- End of ct10api_assign_application_name ----- */



/************************************************************
 Log RPC structure 
 ************************************************************/

void xmltp_api_rpc_dump(char              *rpc_name,
                        XMLTP_API_DATA_XDEF_T  *t_parm[],
                        int               nb_parm)
{
int x;

sprintf(s_log_buf, "RPC Name: [%s]\n", rpc_name);
sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

for (x=0; x<nb_parm; ++x)
{
        sprintf(s_log_buf, "%d name[%s] len[%ld] ",x+1,t_parm[x]->fmt.name,t_parm[x]->fmt.namelen);
      	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        switch ((t_parm[x])->fmt.datatype) {
                case CS_CHAR_TYPE:
                                sprintf(s_log_buf, "type[C] val[%.*s] ",t_parm[x]->len,
                                                            t_parm[x]->p_val);
                                break;
                case CS_INT_TYPE:
                                sprintf(s_log_buf, "type[I] val[%ld] ",
                                        *((CS_INT *) t_parm[x]->p_val));
                                break;
                case CS_SMALLINT_TYPE:
                                sprintf(s_log_buf, "type[I] val[%d] ",
                                        *((CS_SMALLINT *) t_parm[x]->p_val));
                                break;
                case CS_FLOAT_TYPE:
                                sprintf(s_log_buf, "type[F] val[%f] ",
                                        *((CS_FLOAT *) t_parm[x]->p_val));
                                break;
                case CS_DATETIME_TYPE:
                                sprintf(s_log_buf, "type[D] ");
                                break;
		default:
                                sprintf(s_log_buf, "Error invalid type");
                                break;

        }
      	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);
        sprintf(s_log_buf, "len[%ld] null[%d] stat[%ld]",
                                t_parm[x]->len,
                                t_parm[x]->null_ind,
                                t_parm[x]->fmt.status);
      	sf_log(ALOG_ERROR, 0, s_log_buf, DEBUG_TRACE_LEVEL_NONE);

}

return;

} /* ---------- End of xmltp_api_rpc_dump ---------------- */



/* ------------------------- End of xmltp_api.c ----------------------------- */
