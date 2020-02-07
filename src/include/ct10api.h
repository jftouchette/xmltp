/* ct10api.h
 * ---------
 *
 *  Copyright (c) 1996-2003 Jocelyn Blanchet
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
 *
 * Functions prefix:	ct10api_
 *
 * 
 * Generic Client-Libray functions. 
 *
 * Higher-level functions over Sybase Open Client CT-Lib and CS-Lib.
 *
 *
 * The usual start-up sequence of calls is:
 *
 *	ct10api_init_log(log_name);
 *	ct10api_load_config(config_file, section_name);
 *	ct10api_init_cs_context();
 *
 * Then, one or many
 *	ct10api_create_connection(login_id, password, app_name,	host_name,
 *				  server_name);
 *
 * Optionnally, one call to:
 *	ct10api_assign_process_row_function(void *p_ct10_conn, ...)
 *
 * Then, (many) calls to any other functions to execute RPC or language...
 *
 * And, finally,
 *	ct10api_log_stats_and_cleanup_everything();
 *
 *
 * ------------------------------------------------------------------------
 * Versions:
 * 1996apr01,jft: begun header file
 * 1996apr09,jft: completed first draft
 *		  added void *ct10api_get_owner_of_conn(CS_CONNECTION *p_conn)
 * 1996apr10,blanchjj: Add version information
 * 1996jun06,jft: + ct10api_get_server_name_of_conn(void *p_conn)
 * 1996sep25,jft: added: void ct10api_set_log_mode_quiet(int new_val)
 * 1996oct08,jbl: Added: ct10api_set_packet_size(CS_INT p_size)
 * 1996dec05,jbl: Add definition of ct10api_get_ct_version.
 * 1997fev06,jbl: Add CT10_ERR_RPC_PARSING_ERROR and CT10_ERR_NULL_ARG.
 * 1997fev10,jbl: Change to version 1.0.1 (no more beta).
 * 1997mar03,deb: Added ct10api_get_context() to allow client to call 
 *			cs_dt_crack()
 * 1997may01,jbl: Added: #define CT10_DIAG_NO_CALLBACK_ERROR    0x80000000
 * 1997may29,jbl: Added: int ct10api_change_exec_timeout_value(CS_INT 
 *				timeout_value);
 * 1997may29,jbl: Added:	ct10api_exec_rpc_value
 *				ct10api_free_tab_ct10dataxdef
 *				ct10api_convert_tab_csdatafmt_to_ct10dataxdef
 * 1997aug15,jbl: move VERSION_ID from ct10api.h to ct10apip.h .
 * 2000jun27,deb: Added UCFG_LOGIN_TIMEOUT
 * 2001apr13,deb: Added explicit void argument in:
 *                . char *ct10api_get_version(void);
 *                . char *ct10api_get_ct_version(void);
 *                . int ct10api_get_debug_trace_flag(void);
 *                . int ct10api_init_cs_context(void);
 *                . int ct10api_log_stats_and_cleanup_everything(void);
 *                to please compiler
 * 2001oct31,jbl: Added: ct10api_change_connection_params.
 * 2001nov21,jbl: Added: ct10api_change_and_exec_init_string, not done by 
 *			 ct10api_change_connection_params because
 *			 we want to execute the init_string.
 * 2002apr11,deb: + ct10api_bulk_init_set_cs_datafmt()
 * 2002jun12,jbl: Added: ct10api_convert_date_to_string_std
 * 2003jul10,jbl: Added: prototype for ct10api_set_row_count
 */

#ifndef _CT10API_H_
#define _CT10API_H_


/* -------------------------------------------- Public Defines:  */

/*
 * Make sure we have good defines for TRUE and FALSE:
 */
#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE		1
#define FALSE		0


/*
 * Log system definition.
 */

#define MODULE_NAME		(__FILE__)
#define DEFAULT_NOTIFY		"nobody"
#define LOG_BUFFER_LEN		4096

/*
 * Some common values:
 */
#define USUAL_CONNECT_MAX_RETRY			 2
#define USUAL_EXECUTE_MAX_RETRY			 3

#define DEADLOCK_WAIT_DELAY			 1	/* 1 second */
#define TIMEOUT_EXEC_WAIT_DELAY			 0	/* do not wait */

#define MAX_CONN_INIT_STRING			1024

#define DEFAULT_EXEC_TIMEOUT			120
#define CONN_SIGNATURE				0x3C12AB13
#define INVALID_SIGNATURE			-1
#define VALID_SIGNATURE				0

#define MAX_NB_PARM				255
#define MAX_STRING_SIZE				255


/*
 * Value pass to the user define result handler.
 *
 */

#define CT10API_HEADER_IND_FALSE	0
#define CT10API_HEADER_IND_TRUE		1
#define CT10API_HEADER_IND_NB_ROW	2


/*
 * Create connection Option Mask
 *
 */

#define CT10API_BULK_LOGIN			0x00000001


/*
 * UCFG definitions.
 *
 */

#define UCFG_LOG_FILENAME	"LOG_FILENAME"
#define UCFG_EXEC_TIMEOUT	"EXEC_TIMEOUT"
#define UCFG_LOGIN_TIMEOUT	"LOGIN_TIMEOUT"

/*
 * Return Values from ct10api_xxx() functions indicate severity of error,
 * if any...
 *
 * (They tell HOW the application should react to the error).
 *
 */

#define CT10_NOTHING_TO_DO			  1
#define CT10_ERR_INTERVAL			 -4

#define CT10_SUCCESS				  0
#define CT10_ERR_FAILED				 -1
#define CT10_ERR_DO_NOT_RETRY			 -5
#define CT10_ERR_FATAL_ERROR			 -9
#define CT10_ERR_RPC_PARSING_ERROR		-10
#define CT10_ERR_NULL_ARG			-11
#define CT10_ERR_TOO_MANY_FATALS_ABORTED	-13
#define CT10_ERR_BAD_CONN_HANDLE		-40



/*
 * Values indicating Diagnostic of error:
 *
 * ( CT10_CONN.diagnostic_mask )
 *
 * -- Tell WHY call to API function failed --
 *
 */
#define CT10_DIAG_INFO_MSG_RECEIVED		0x00000001
#define CT10_DIAG_BAD_RETURN_STATUS		0x00000002
#define CT10_DIAG_CTLIB_CALL_FAILED		0x00000004

#define CT10_DIAG_ABORTED_BY_CUSTOM_RECOVERY	0x00000008


#define CT10_DIAG_DEAD_CONNECTION		0x00000010
#define CT10_DIAG_RESULTS_BEGUN			0x00000020

#define CT10_DIAG_NO_SUCH_SERVER		0x00000100
#define CT10_DIAG_CANNOT_FIND_HOST		0x00000200
#define CT10_DIAG_LOGIN_REJECTED		0x00000400
#define CT10_DIAG_CONNECT_TIMEOUT		0x00000800	/* not used */

#define CT10_DIAG_DUPLICATE_KEY			0x00001000
#define CT10_DIAG_ATTEMPT_INSERT_NULL		0x00002000
#define CT10_DIAG_TRANSACTION_COUNT_IMBALANCE	0x00004000

#define CT10_DIAG_DEADLOCK			0x00010000
#define CT10_DIAG_TIMEOUT			0x00020000
#define CT10_DIAG_DISCONNECTED_BY_PEER		0x00040000

#define CT10_DIAG_MISSING_PROC			0x00100000
#define CT10_DIAG_MISSING_OBJECT		0x00200000
#define CT10_DIAG_BAD_PARAM			0x00400000
#define CT10_DIAG_CONVERT_ERROR			0x00800000
#define CT10_DIAG_EXEC_PERMISSION_DENIED	0x01000000
#define CT10_DIAG_ROLE_DENIED			0x02000000

#define CT10_DIAG_CIG_RESOURCE_ERROR		0x04000000  /* unusual	*/
#define CT10_DIAG_CIG_GATE_CLOSE		0x08000000  /* possible */

#define CT10_DIAG_BAD_REG_PROC_PARAMS		0x10000000  /* bad RP params */
#define CT10_DIAG_BAD_REG_PROC_DEFINITION	0x20000000  /* bug in the RP */

#define CT10_DIAG_UNKNOWN_ERROR			0x40000000
#define CT10_DIAG_SEV_DIAG_POSTED		0x80000000

/*
 * Values indicating Diagnostic of error:  (by severity)
 *
 * ( CT10_CONN.diag_sev_mask )
 *
 */
#define CT10_DIAG_SEV_CTLIB_CONFIG_ERROR	 0x00000001
#define CT10_DIAG_SEV_CTLIB_API_CALL_BAD	 0x00000002
#define CT10_DIAG_SEV_CTLIB_ERROR_RETRYABLE	 0x00000004
#define CT10_DIAG_SEV_CTLIB_COMM_ERROR_RETRYABLE 0x00000008
#define CT10_DIAG_SEV_CTLIB_INTERNAL_RES_FAIL	 0x00000010
#define CT10_DIAG_SEV_CTLIB_FATAL		 0x00000020

#define CT10_DIAG_SEV_OBJECT_NOT_FOUND		 0x00000100
#define CT10_DIAG_SEV_WRONG_DATATYPE		 0x00000200
#define CT10_DIAG_SEV_TRANSACTION_SCOPE_ERROR	 0x00000400
#define CT10_DIAG_SEV_PERMISSION_DENIED		 0x00000800
#define CT10_DIAG_SEV_SYNTAX_ERROR		 0x00001000
#define CT10_DIAG_SEV_MISC_USER_ERROR		 0x00002000
#define CT10_DIAG_SEV_INSUFFICIENT_SPACE_RES	 0x00004000
#define CT10_DIAG_SEV_FATAL_THREAD_ERROR	 0x00008000
#define CT10_DIAG_SEV_FATAL_RESOURCE_ERROR	 0x00010000
#define CT10_DIAG_SEV_FATAL_DB_PROCESSES_ERROR	 0x00020000
#define CT10_DIAG_SEV_FATAL_TABLE_ERROR		 0x00040000
#define CT10_DIAG_SEV_FATAL_DATABASE_ERROR	 0x00080000
#define CT10_DIAG_SEV_FATAL_SERVER_ERROR	 0x00100000

/* It look like from time to time ct lib function call will    */
/* fail and no call back error will be received by the client. */
/* In that case custom error recovery may use this bitmask to  */
/* matche with the default recovery.			       */

#define CT10_DIAG_NO_CALLBACK_ERROR		 0x80000000


typedef union {
	CS_SMALLINT	i;
	CS_INT 		l;
	CS_FLOAT	d;
} ALL_TYPE;


/* -------------------------------------------- Public Struct/Types: */

/* Public CT10_DATA_XDEF_T struct:
 *	used to describe, bind and xfer a param or column
 *
 * Creating one array of [nb_params (or nb_cols)] CT10_DATA_XDEF_T is
 * more practical than creating 4 four arrays fmt[n], len[n], null_ind[n],
 * and p_val[n] !!!
 */
struct st_ct10_data_xdef {
	CS_DATAFMT	 fmt;
				/* Next 3 fields used with ct_bind(),
				 * or ct_param():
				 */
	CS_INT		 len;		/* length of data 		*/
	CS_SMALLINT	 null_ind;	/* NULL value indicator		*/
	ALL_TYPE	 *p_val;	/* ptr to buffer to hold value	*/
};

typedef struct st_ct10_data_xdef CT10_DATA_XDEF_T;



/* Public CT10_DATA_VALUE_T struct:
 *
 * Basic elements require to define a parameter.
 *
 */
struct st_ct10_data_value {
	CS_INT		 len;		/* length of data 		*/
	CS_SMALLINT	 null_ind;	/* NULL value indicator		*/
	ALL_TYPE	 *p_val;	/* ptr to buffer to hold value	*/
};
typedef struct st_ct10_data_value CT10_DATA_VALUE_T;



/*
 * Public CT10_PARAM_VALUE_T struct:
 *
 * This structure is use to separate data values from it's
 * definition.
 *
 */
struct st_ct10_param_value {
	int			param_id;
	CT10_DATA_VALUE_T	value;
};
typedef struct st_ct10_param_value CT10_PARAM_VALUE_T;



/* -------------------------------------------- Public Functions:  */


char *ct10api_get_version(void);
/*
 * Return ptr to static string indicating version info (compile date
 * and time, version number, module name, etc.)
 */

char *ct10api_get_ct_version(void);
/*
 * Return ptr to static string indicating open client version info.
 */

int ct10api_get_debug_trace_flag(void);
/*
 * Return value of trace flag currently set.
 */



void ct10api_assign_debug_trace_flag(int new_val);
/*
 * Assign value of (debug) trace flag.
 */
 


void ct10api_assign_application_name(char *app_name);
/*
 * Set application name to use with the log functions of ct10api and
 * ct10reco modules.
 */



int ct10api_init_log(char *log_name);
/*
 * Prepare "apl_log1.c" to be used for standard logging.
 *
 * Return:
 *	0	OK
 *	-1	Error.
 */



int ct10api_apl_log(    int     err_level,      /* ALOG_INFO, ALOG_WARN, ... */
			int     msg_no,         /* Message number.           */
			char    *msg_text,      /* message text.             */
			int     level_to_log);  /* Trace level needed to log.*/
/*
 * Call to apl_log to be used by ct10api module.
 *
 * Return:
 *	0	OK
 *	<0	Error.
 *
 */



int ct10api_load_config(char *config_file, char *section_name);
/*
 * Call the ucfg.c module to load the config from file config_file, section
 * section_name.
 *
 * Return:
 *	0	OK
 *	-1	Error. See diagnostic in log.
 *
 * IMPLEMENTATION NOTES:
 *	First, call ucfg_open_cfg_at_section(config_file, section_name)
 *
 *	If it is OK, 
 *	 call ct10reco_reconfigure_master_recov_table_from_config_params()
 */



int ct10api_init_cs_context(void);
/*
 * Create and initialize a CS_CONTEXT...
 *
 * Also, configure CS_CONTEXT to use the callback error handler functions
 * already defined (assigned) in ct_recov.c:
 *		 	ct10reco_CS_lib_error_handler()
 *			ct10reco_remote_error_handler()
 *			ct10reco_ct_lib_error_handler()
 *
 * Return:
 *	0	OK
 *	-1	Error. See diagnostic in log.
 */



int ct10api_set_packet_size(CS_INT p_size);
/*
 * Allow to change TDS packet size. Minimum and maximun values
 * depend on the SQL server setting.
 *
 * This must be called before the creation of the connexion.
 *
 * RETURN:
 *	CT10_SUCCESS
 *	CT10_ERR_FAILED
 *
 */



void *ct10api_create_connection(char *login_id, char *password, 
				char *app_name,	char *host_name,
				char *server_name,
				char *init_string,
				long options_mask);
/*
 * Create a high-level CT10_CONN_T connection structure containing
 * many lower level fields (including a CS_CONNECTION).
 *
 * It is up to the caller to decide where to get login_id, password, etc.
 * from.  These values can come from the config params or from the command
 * line.
 *
 * Return:
 *	NULL	  Cannot create structure because out of memory. See log.
 *	(void *)  Pointer to the CT10_CONN_T
 *
 * IMPLEMENTATION NOTES:
 *	Validate arguments: none should be null or of length zero.
 *	Then:
 *		create a CT10_CONN_T struct (alloc and clear)
 *		initialize its signature
 *		add the CT10_CONN_T to the linklist, sp_list_of_conn,
 *		 (if the list is NULL, create it)
 *		?alloc a CS_CONNECTION
 *		copy values of arguments to fields of the CT10_CONN_T:
 *			login_id[]
 *			password[]
 *			app_name[]
 *			host_name[]
 *			server_name[]
 *			init_string[].
 */


int ct10api_change_connection_params(   void *p_ct10_conn,
                                        char *login_id,
                                        char *password,
                                        char *app_name,
                                        char *host_name,
                                        char *server_name);
/*
 * Allow change of connection parameters after the create connection.
 */



void *ct10api_get_owner_of_conn(CS_CONNECTION	*p_conn);
/*
 * Return a (void *) pointer to the CT10_CONN_T that owns the CS_CONNECTION
 * p_conn.
 *
 * Return NULL if none owns it.
 *
 * If (p_conn == NULL) {
 *    return pointer to the current active CT10_CONN_T  (sp_current_active_conn)
 */



long ct10api_get_diagnostic_mask(void *p_ct10_conn);
/*
 * Return value of p_ct10_conn->diagnostic_mask.
 *
 * This function can be called to know the cause of the failure of a call
 * to the API functions:
 *		ct10api_connect()
 *		ct10api_exec_rpc()
 *		ct10api_exec_lang()
 *
 * Return:
 *	-1L				p_ct10_conn is not valid. See log.
 *	CT10_DIAG_SEV_DIAG_POSTED	You should look to diag_sev_mask,
 *					 use ct10api_get_diag_sev_mask()
 *	mask of CT10_DIAG_xxx		(mask of or'ed bit values)
 */



long ct10api_get_diag_sev_mask(void *p_ct10_conn);
/*
 * Return value of p_ct10_conn->diag_sev_mask.
 *
 * Return:
 *	-1L				p_ct10_conn is not valid. See log.
 *	mask of CT10_DIAG_SEV_xxx	(mask of or'ed bit values)
 */



char *ct10api_get_server_name_of_conn(void *p_conn);
/*
 * Return (p_conn->server_name)
 * or NULL if p_conn is not a valid CT10_CONN_T struct.
 */





int ct10api_assign_get_appl_context_info_function(void *p_ct10_conn,
				char *(* pf_get_appl_context_info)() );
/*
 * Assigns the pointer to the function that will be called by the various
 * error handlers when they write an error message to the log.
 *
 *
 * WARNING:
 * The function (* pf_get_appl_context_info)() ) returns a pointer to
 * a static string that contains something like "File: XXX Line: nn".
 * 
 * 
 * Return:
 *	CT10_SUCCESS				OK
 *	CT10_ERR_BAD_CONN_HANDLE		p_ct10_conn is NULL or bad.
 */



int ct10api_assign_process_row_function(void *p_ct10_conn,
	int (* pf_process_row_or_params)(void *p_ct10_conn, int b_is_row,
				     CT10_DATA_XDEF_T  *data_array[],  
				     int nb_data,
				     int result_set_no,
				     CS_RETCODE result_type,
				     int header_ind)  );
/*
 * Assigns the pointer to the function that process each row of result
 * (if b_is_row is TRUE) or a set of output parameters.
 *
 * WARNING:
 * The function (* pf_process_row_or_params)() should only _look_ in the 
 * values.  It must _not_ do any free() !!!
 * 
 * 
 * Return:
 *	CT10_SUCCESS				OK
 *	CT10_ERR_BAD_CONN_HANDLE		p_ct10_conn is NULL or bad.
 */


int  ct10api_process_row_or_params (	void	 	 *p_ct10_conn, 
					int 		 b_is_row,
					CT10_DATA_XDEF_T *data_array[],
					int		 nb_data,
					int		 result_set_no,
					CS_RETCODE	 result_type,
					int		 header_ind);




int ct10api_connect(void *p_ct10_conn);
/*
 * Perform a ct_connect() to connect to connection->server_name.
 * 
 * The number of attempts in case of failure is controlled by the recovery
 * table.
 *
 * A call to this function is optional. It is useful to check that the
 * connection is already established before getting into further steps
 * of processing.
 *
 * If a CT10_CONN_T is not connected and it is used to execute a RPC, 
 * a connect() will be done automatically.
 *
 * Return:
 *	CT10_SUCCESS				OK
 *	CT10_ERR_FAILED				Failed
 *	CT10_ERR_DO_NOT_RETRY			It is useless to try again
 *	CT10_ERR_FATAL_ERROR			Application should stop.
 *	CT10_ERR_TOO_MANY_FATALS_ABORTED	Fatal catatonic sleep mode.
 *	CT10_ERR_BAD_CONN_HANDLE		p_ct10_conn is NULL or bad.
 */




int ct10api_exec_rpc_string(void *p_ct10_conn,  char *rpc_string,
			    CS_INT  *p_return_status);
/*
 * Parse the RPC string, prepare the RPC call, send it, process results.
 *
 * Unless a custom results output function address has been assigned 
 * previously, the results are formatted and written to stdout.
 *
 * The value of the Return Status is copied into *p_return_status.
 *
 * Return:
 *	CT10_SUCCESS				OK
 *	CT10_ERR_FAILED				Failed
 *	CT10_ERR_DO_NOT_RETRY			It is useless to try again
 *	CT10_ERR_FATAL_ERROR			Application should stop.
 *	CT10_ERR_TOO_MANY_FATALS_ABORTED	Fatal catatonic sleep mode.
 *	CT10_ERR_BAD_CONN_HANDLE		p_ct10_conn is NULL or bad.
 */



int ct10api_exec_rpc(void *p_ct10_conn, char *rpc_name,
		     CT10_DATA_XDEF_T  *params_array[],  int nb_params,
		     CS_INT  *p_return_status);
/*
 * Prepare the RPC call, send it, process results.
 *
 * Unless a custom results output function address has been assigned 
 * previously, the results are formatted and written to stdout.
 *
 * The value of the Return Status is copied into *p_return_status.
 *
 * Return:
 *	CT10_SUCCESS				OK
 *	CT10_ERR_FAILED				Failed
 *	CT10_ERR_DO_NOT_RETRY			It is useless to try again
 *	CT10_ERR_FATAL_ERROR			Application should stop.
 *	CT10_ERR_TOO_MANY_FATALS_ABORTED	Fatal catatonic sleep mode.
 *	CT10_ERR_BAD_CONN_HANDLE		p_ct10_conn is NULL or bad.
 */



int ct10api_exec_lang(void *p_ct10_conn, char *lang_string);
/*
 * Send the SQL language string, process results.
 *
 * Unless a custom results output function address has been assigned 
 * previously, the results are formatted and written to stdout.
 *
 * Return:
 *	CT10_SUCCESS				OK
 *	CT10_ERR_FAILED				Failed
 *	CT10_ERR_DO_NOT_RETRY			It is useless to try again
 *	CT10_ERR_FATAL_ERROR			Application should stop.
 *	CT10_ERR_TOO_MANY_FATALS_ABORTED	Fatal catatonic sleep mode.
 *	CT10_ERR_BAD_CONN_HANDLE		p_ct10_conn is NULL or bad.
 */




int ct10api_change_and_exec_init_string(void *p_ct10_conn, char *init_string);
/*
 * Change the SQL statement string that is execute after each
 * connection.
 *
 * Also execute the language event.
 *
 * Return:
 *	CT10_SUCCESS				OK
 *	CT10_ERR_FAILED				Failed
 *	CT10_ERR_DO_NOT_RETRY			It is useless to try again
 *	CT10_ERR_FATAL_ERROR			Application should stop.
 *	CT10_ERR_TOO_MANY_FATALS_ABORTED	Fatal catatonic sleep mode.
 *	CT10_ERR_BAD_CONN_HANDLE		p_ct10_conn is NULL or bad.
 */
 




int ct10api_close_connection(void *p_ct10_conn);
/*
 *
 * Close the CS_CONNECTION.
 *
 *
 * NOTE: The p_ct10_conn handler remains valid.
 *
 *	 This function will probably only be useful in a long running 
 *	 program that wants to be "light" on resources and release its
 *	 connection(s) to the data server.
 *
 *
 * Return:
 *	CT10_SUCCESS				OK
 *	CT10_ERR_FAILED				Failed
 *	CT10_ERR_DO_NOT_RETRY			It is useless to try again
 *	CT10_ERR_FATAL_ERROR			Application should stop.
 *	CT10_ERR_TOO_MANY_FATALS_ABORTED	Fatal catatonic sleep mode.
 *	CT10_ERR_BAD_CONN_HANDLE		p_ct10_conn is NULL or bad.
 */



int ct10api_log_stats_and_cleanup_everything(void);
/*
 * Logs stats available from ct10reco.c or internally from ct10api.c,
 * and close and free each CT10_CONN_T created.
 *
 * Return:
 *	number of CT10_CONN_T that were still active.
 */



int ct10api_check_ct10_conn_signature(void *p_ct10_conn);
/*
 * Return 0 if p_ct10_conn != NULL And p_ct10_conn->signature is valid
 *  else
 * Return -1.
 */

int ct10api_bulk_init(	void *p_ct10_conn,
			char *tbl_name, 
			CT10_DATA_XDEF_T  *columns_array[],
			int nb_columns);
/*
 * Initialize BULK copy and bind columns to memory pointer. 
 *
 * Columns in columns_array must match exactly table columns.
 * The columns sequence is the indice in columns_array + 1.
 * If column_array[i] is NULL than this column is not Bind.
 * This is not the same thing as binding a null value.
 *
 * Return:
 *      CT10_SUCCESS                            OK
 *      CT10_ERR_FAILED                         Failed
 *      CT10_ERR_DO_NOT_RETRY                   It is useless to try again
 *      CT10_ERR_FATAL_ERROR                    Application should stop.
 *      CT10_ERR_TOO_MANY_FATALS_ABORTED        Fatal catatonic sleep mode.
 *      CT10_ERR_BAD_CONN_HANDLE                p_ct10_conn is NULL or bad.
 *
 */




/*
 * Initializes BULK copy, get columns description from database and
 * bind columns to memory pointer
 *
 * This functions gets the CS_DATAFMT from the database and calls
 * ct10api_bulk_init afterward.  So the client does not need to fill
 * the CS_DATAFMT of the CT10_DATA_XDEF_T structures, only allocate
 * them.
 *
 * If the table has an identity column, its name MUST be specified
 * in the ident_col_name parameter.  When the load encounters this
 * column, it ignores its value and skip to the next column.  The
 * column MUST be present in the load data, but the value will be
 * ignored and the identity value will be get from the database.
 *
 * The parameter with_ident_data, if true (non zero), will force
 * the value of the identity column into the table.  This option
 * is presently buggy, so use with care...  If false (zero), the
 * data ot the identity column in the ct10api_bulk_send_1_row will
 * be ignored and the value will be assigned by the database.
 *
 * Return:
 *      CT10_SUCCESS                            OK
 *      CT10_ERR_FAILED                         Failed
 *      CT10_ERR_DO_NOT_RETRY                   It is useless to try again
 *      CT10_ERR_FATAL_ERROR                    Application should stop.
 *      CT10_ERR_TOO_MANY_FATALS_ABORTED        Fatal catatonic sleep mode.
 *      CT10_ERR_BAD_CONN_HANDLE                p_ct10_conn is NULL or bad.
 */
int ct10api_bulk_init_set_cs_datafmt(   void *p_ct10_conn,
		                        char *tbl_name,
		                        CT10_DATA_XDEF_T  *columns_array[],
					int nb_columns,
					char *ident_col_name,
					int with_ident_data);



int ct10api_bulk_send_1_row (void *p_ct10_conn);
/*
 * Sending 1 row of bulk data to the SQL server.
 *
 * Return:
 *      CT10_SUCCESS                            OK
 *      CT10_ERR_FAILED                         Failed
 *      CT10_ERR_DO_NOT_RETRY                   It is useless to try again
 *      CT10_ERR_FATAL_ERROR                    Application should stop.
 *      CT10_ERR_TOO_MANY_FATALS_ABORTED        Fatal catatonic sleep mode.
 *      CT10_ERR_BAD_CONN_HANDLE                p_ct10_conn is NULL or bad.
 */



int ct10api_bulk_commit (void *p_ct10_conn, CS_INT *outrow);
/*
 * Commit bulk batch.
 *
 * Return:
 *      CT10_SUCCESS                            OK
 *      CT10_ERR_FAILED                         Failed
 *      CT10_ERR_DO_NOT_RETRY                   It is useless to try again
 *      CT10_ERR_FATAL_ERROR                    Application should stop.
 *      CT10_ERR_TOO_MANY_FATALS_ABORTED        Fatal catatonic sleep mode.
 *      CT10_ERR_BAD_CONN_HANDLE                p_ct10_conn is NULL or bad.
 *
 */


int ct10api_bulk_close (void *p_ct10_conn, int cancel_indicator);
/*
 * Terminate and close current bulk insert.
 *
 * cancel_indicator:
 *	0: Do not cancel batch. (user must call ct10api_bulk_commit before).
 *	1: Cancel current batch and terminate current bulk job.
 *
 * After a cancel it is impossible to resume the current job.
 * a new ct10_bulk_init must be called.
 *
 * Return:
 *      CT10_SUCCESS                            OK
 *      CT10_ERR_FAILED                         Failed
 *      CT10_ERR_DO_NOT_RETRY                   It is useless to try again
 *      CT10_ERR_FATAL_ERROR                    Application should stop.
 *      CT10_ERR_TOO_MANY_FATALS_ABORTED        Fatal catatonic sleep mode.
 *      CT10_ERR_BAD_CONN_HANDLE                p_ct10_conn is NULL or bad.
 *
 */



void ct10api_set_log_mode_quiet(int new_val);
/*
 * If ct10api_set_log_mode_quiet(1) is called, then ct10api_apl_log()
 * will stop writing to the application log (through apl_log1.c).
 */


CS_CONTEXT *ct10api_get_context(void);
/*
 * Get pointer to context (a must if the client want to convert dates)
 */


int ct10api_change_exec_timeout_value(CS_INT exec_timeout);
/*
 * Change rpc exec timeout. Il the connection is drop and
 * recreate new timeout value will be used.
 */



int ct10api_convert_tab_csdatafmt_to_ct10dataxdef(
	CS_DATAFMT		(*in_param_def)[],
	CT10_DATA_XDEF_T	*(**pp_tab_p_ct10_data_xdef)[],
	int			nb_params
);
/*
 * Convert an array of CS_DATAFMT into an array of pointers to
 * CT10_DATA_XDEF. The address of a pointer is given and the
 * array of pointers is allocate and all CT10_DATA_XDEF are also
 * allocate.
 */



int ct10api_free_tab_ct10dataxdef(
		CT10_DATA_XDEF_T *(**pp_tab_p_ct10_data_xdef)[],
		int nb_params);
/*
 * This function free everything that was allocated by 
 * ct10api_convert_tab_csdatafmt_to_ct10dataxdef.
 * Note: p_val are not free by this function.
 */



int ct10api_exec_rpc_value(	void			*p_ct10_conn,
				char			*name, 
				CT10_DATA_XDEF_T	*(*p_tab_param_def)[],
				CT10_PARAM_VALUE_T	(*tab_param_values)[],
				int			nb_params,
				CS_INT			*p_return_status);
/*
 * This fonction execute a rpc which has the value and the definition 
 * separated.
 *
 */



int ct10api_convert_date_to_string(void *dte, char *string_dte);

/*
 * Convert a native sybase datatime datatype into a string.
 * Calling process must make sure that string_dte point to a space
 * big enought to hold the date in string format.
 *
 */

int ct10api_crack_date(void *dte, CS_DATEREC *date_rec);

/*
 * Use cs_dt_crack ti crack a date.
 *
 */

int ct10api_convert_date_to_long(void *dte, long *out_dte);
/*
 * Convert a CS_DATE_TIME into a long of the format:
 * YYYYMMDD
 */




int ct10api_convert_date_to_string_std(void *dte, char *string_dte);
/*
 *
 */


/*
 ***************************************************************
 Set the row count limit using ct_option

 set to -1 to disable rowcount setting before a proc call
 set to 0 for no limit
 ***************************************************************/

void ct10api_set_row_count(int nb_rows);


#endif /* #ifndef _CT10API_H_ */

/* --------------------------------------------- End of ct10api.h --------- */
