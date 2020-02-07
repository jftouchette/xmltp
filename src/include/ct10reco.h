/* ct10reco.h
 * ----------
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
 * Functions prefix:	ct10reco_
 *
 * 
 * Recovery Defines, Struct and Functions used with/by "ct10api.c"
 *
 *
 *
 *
 * ------------------------------------------------------------------------
 * Versions:
 * 1996apr09,jft: first version done
 * 1996apr10,jft: + void ct10reco_assign_application_name(char *app_name);
 *		  . fixed declarde of ct10reco_assign_master_recov_tab()
 * 1996jun05,jft: + void ct10reco_assign_pf_display_err_msg()
 * 1997sep19,jbl: Added: define for RECOV_PAR_FIRST_RAISERROR_MSG_NUMBER.
 *		         and MIN_FIRST_RAISERROR_MSG_NUMBER
 * 2001apr13,deb: Added explicit void argument in:
 *                . int ct10reco_reconfigure_master_recov_table_from_config_params(void);
 *                . void ct10reco_reset_current_batch_counters(void);
 *                . void ct10reco_reset_current_batch_counters(void);
 *                to please compiler
 */


#ifndef _CT10RECO_H_
#define _CT10RECO_H_


/* -------------------------------------------- Public Defines:  */


/*
 * NOTE:
 *	The CT10_ERR_xxx and CT10_DIAG_xxx defines are in "ct10api.h"
 *
 */



/*
 * Debug Trace Levels:
 * -------------------
 */
#define DEBUG_TRACE_LEVEL_NONE			0
#define DEBUG_TRACE_LEVEL_LOW			1
#define DEBUG_TRACE_LEVEL_MEDIUM		5
#define DEBUG_TRACE_LEVEL_FULL			10


/*
 * Some of the most frequently used values of max_curr_to_escalate:
 */
#define MAX_ESC_NO_LIMIT			   0L
#define MAX_ESC_MANY_APP_ERR			2000L
#define MAX_ESC_MANY_SERIOUS			 200L
#define MAX_ESC_MANY_UNRECOV			  15L
#define MAX_ESC_MANY_FATAL			   5L




/* 
 * Bit-mask values indicating how to react to and recover from errors:
 * ------------------------------------------------------------------
 *
 * NOTES:
 *	These defines are semi-private. They should be used only within 
 *	ct10reco.c and ct10api.c
 *
 *	The default action is to:
 *			- log the msg
 *			- return CS_SUCCEED (from within error handler)
 *			- retry
 *			- keep the CS_CONNECTION.
 *
 *	These bit-masks modify the default action.
 */
#define RECOV_ACT_DEFAULT			0
#define RECOV_ACT_RESET_CONNECTION		0x00000010
#define RECOV_ACT_DO_NOT_LOG_MSG		0x00000001
#define RECOV_ACT_FLUSH_RESULTS			0x00000002
#define RECOV_ACT_DO_NOT_RETRY			0x00000004
#define RECOV_ACT_RETURN_CS_FAIL		0x00000008
#define RECOV_ACT_FORGET_IT		(  RECOV_ACT_FLUSH_RESULTS	\
					 | RECOV_ACT_DO_NOT_RETRY )




/*
 * Constants defining names of parameters used in 2nd Level Recovery Table:
 *
 */
#define RECOV_PAR_MAX_CURR_TO_ESCALATE		"MAX_CURR_TO_ESCALATE"
#define RECOV_PAR_MAX_RETRY			"MAX_RETRY"
#define RECOV_PAR_RETRY_INTERVAL		"RETRY_INTERVAL"
#define RECOV_PAR_MIN_DEBUG_LEVEL_TO_LOG	"MIN_DEBUG_LEVEL_TO_LOG"
#define RECOV_PAR_ACTION_MASK			"ACTION_MASK"

#define RECOV_PAR_FIRST_RAISERROR_MSG_NUMBER	"FIRST_RAISERROR_MSG_NUMBER"
#define MIN_FIRST_RAISERROR_MSG_NUMBER		20000


/*
 * Error classes:
 * -------------
 *
 * Values of error_class, which is "join key" between the 1st level and 2nd
 * level recovery tables.
 *
 *
 */
#define ERR_CLASS_UNKNOWN			-1

#define ERR_CLASS_INFO_MSG_TO_IGNORE		 0
#define ERR_CLASS_INFO_MSG_TO_LOG		 1

#define ERR_CLASS_BAD_RETURN_STATUS		 2

#define ERR_CLASS_ABORTED_BY_CUSTOM_RECOVERY	 3

#define ERR_CLASS_DEAD_CONNECTION		 5

#define ERR_CLASS_NO_SUCH_SERVER		10
#define ERR_CLASS_CANNOT_FIND_HOST		11
#define ERR_CLASS_LOGIN_REJECTED		12
#define ERR_CLASS_CONNECT_FAILED		13
#define ERR_CLASS_CONNECT_TIMEOUT		14  /* cannot be recognized */
#define ERR_CLASS_CTLIB_CONFIG_ERROR		15

#define ERR_CLASS_DUPLICATE_KEY			20
#define ERR_CLASS_ATTEMPT_INSERT_NULL		22
#define ERR_CLASS_TRANSACTION_COUNT_IMBALANCE	23

#define ERR_CLASS_DEADLOCK			25
#define ERR_CLASS_TIMEOUT			26
#define ERR_CLASS_DISCONNECTED_BY_PEER		27

#define ERR_CLASS_MISSING_PROC			30
#define ERR_CLASS_MISSING_OBJECT		31
#define ERR_CLASS_BAD_PARAM			32
#define ERR_CLASS_CONVERT_ERROR			33
#define ERR_CLASS_EXEC_PERMISSION_DENIED	34
#define ERR_CLASS_ROLE_DENIED			35

#define ERR_CLASS_CIG_RESOURCE_ERROR		40
#define ERR_CLASS_CIG_GATE_CLOSE		41

#define ERR_CLASS_BAD_REG_PROC_PARAMS		50
#define ERR_CLASS_BAD_REG_PROC_DEFINITION	51

#define ERR_CLASS_CTLIB_API_CALL_BAD		70
#define ERR_CLASS_CTLIB_ERROR_RETRYABLE		71
#define ERR_CLASS_CTLIB_COMM_ERROR_RETRYABLE	72
#define ERR_CLASS_CTLIB_INTERNAL_RES_FAIL	73
#define ERR_CLASS_CTLIB_FATAL			74

#define ERR_CLASS_SEV_OBJECT_NOT_FOUND		80
#define ERR_CLASS_SEV_WRONG_DATATYPE		81
#define ERR_CLASS_SEV_TRANSACTION_SCOPE_ERROR	82
#define ERR_CLASS_SEV_PERMISSION_DENIED		83
#define ERR_CLASS_SEV_SYNTAX_ERROR		84
#define ERR_CLASS_SEV_MISC_USER_ERROR		85

#define ERR_CLASS_SEV_INSUFFICIENT_SPACE_RES	86

#define ERR_CLASS_SEV_FATAL_THREAD_ERROR	90
#define ERR_CLASS_SEV_FATAL_RESOURCE_ERROR	91
#define ERR_CLASS_SEV_FATAL_DB_PROCESSES_ERROR	92

#define ERR_CLASS_SEV_FATAL_TABLE_ERROR		95
#define ERR_CLASS_SEV_FATAL_DATABASE_ERROR	96
#define ERR_CLASS_SEV_FATAL_SERVER_ERROR	97





/* -------------------------------------------- Public Structures:  */


/*
 * 1st Level Error Recovery (Error Classification) Table struct:
 * ------------------------------------------------------------
 *
 */
typedef struct {

	int	msgno,
		layer,			/* 0 means any layer 	*/
		origin;			/* 0 means any origin	*/

	int	error_class;

} ERROR_RECOV_CLASS_T;



/*
 * Severity Error Recovery (Error Classification) Table struct:
 * ------------------------------------------------------------
 *
 * Local CT-Lib and Remote severity numbers are merged in one table
 * because their range do not overlap.  There is no ambiguity.
 * 
 * (See the default table, s_severity_err_recov_tab[], in ct10reco.c)
 *
 */
typedef struct {

	int	severity;

	int	error_class;

} SEV_ERROR_RECOV_CLASS_T;





/*
 * 2nd Level (Master) Error Recovery Table struct:
 * ----------------------------------------------
 *
 * The following attributes can be modified by the config parameters:
 *	max_curr_to_escalate,
 *	max_retry, retry_interval, 
 *	min_debug_level_to_log
 *
 * Specific values come from a parameter named like this:
 *
 *	<override_param_name>.<attribute>
 *
 */
typedef struct {

	int	 error_class;		/* identify this class of error   */

	char	*override_param_name;	/* Param name prefix to find override
					 * values of the recovery values
					 * for this error class.
					 * Usually, a name that describes the
					 * error_class.
					 */

	long	 max_curr_to_escalate,	/* escalate when curr reach this max */
		 curr_batch_ctr,	/* ctr is reset before each batch */
		 total_ctr;		/* total cumulative ctr of errors */

	long	 diagnostic_mask,	/* mask of CT10_DIAG_xxx values	  */
		 diag_sev_mask,		/* mask of CT10_DIAG_SEV_xxx values */
		 action_mask;		/* mask of RECO_ACT_xxx  values	  */

	int	 ret_err_code,		/* proposed CT10_ERR_xxx value	  */
		 max_retry,		/* nb of retry that can be done	  */
		 retry_interval,	/* nb seconds to wait before retry */
		 min_debug_level_to_log;  /* min debug level needed to log */

} MASTER_RECOV_ENTRY_T;





/* -------------------------------------------- PUBLIC FUNCTIONS:  */


/* --------------------------------- Callback functions: */


CS_RETCODE CS_PUBLIC ct10reco_remote_error_handler(CS_CONTEXT	*p_cs_context,
					 CS_CONNECTION	*p_conn,
					 CS_SERVERMSG	*p_srvmsg);
/*
 * Referenced by:	ct10api_init_cs_context()
 *
 *
 * CT-Lib server msg (remote connection error) handler:
 *
 *  Parameters:
 *	p_cs_context	ptr to Context structure.
 *	p_conn		ptr to CS_CONNECTION
 *	p_srvmsg	Pointer to CS_SERVERMSG structure.
 *
 *  Return:
 *	CS_SUCCEED	(Other value are ILLEGAL and have strange effects)
 */




CS_RETCODE CS_PUBLIC ct10reco_ct_lib_error_handler(CS_CONTEXT	*p_cs_context,
					 CS_CONNECTION	*p_conn,
					 CS_CLIENTMSG	*p_libmsg);
/*
 * Referenced by:	ct10api_init_cs_context()
 *
 *
 * CT-Lib msg (local lib) handler.
 *
 *
 *  Parameters:
 *	p_cs_context	ptr to CS_CONTEXT structure.
 *	p_conn		ptr to CS_CONNECTION
 *	p_libmsg	ptr to CT-Lib CS_CLIENTMSG structure.
 *
 *  Return:
 *	CS_SUCCEED or CS_FAIL	depending on error recovery.
 */




CS_RETCODE ct10reco_CS_lib_error_handler(CS_CONTEXT   *p_cs_context,
					 CS_CLIENTMSG *p_libmsg);
/*
 * Referenced by:	ct10api_init_cs_context()
 *
 *
 * CS-Lib error handler.
 *
 *
 *  Parameters:
 *	p_cs_context	ptr to CS_CONTEXT structure.
 *	p_libmsg	ptr to CT-Lib CS_CLIENTMSG structure.
 *
 *  Return:
 *	CS_SUCCEED or CS_FAIL	depending on error recovery.
 */




/* ---------------------- Functions to assign pointers to functions: */


int ct10reco_assign_remote_err_recov_tab(ERROR_RECOV_CLASS_T  *p_recov_tab,
					 int	sizeof_table);
/* 
 * Assign private pointer  sp_remote_err_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */


int ct10reco_assign_ctlib_err_recov_tab(ERROR_RECOV_CLASS_T  *p_recov_tab,
					int	sizeof_table);
/* 
 * Assign private pointer  sp_ctlib_err_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */



int ct10reco_assign_CSlib_err_recov_tab(ERROR_RECOV_CLASS_T  *p_recov_tab,
					int	sizeof_table);
/* 
 * Assign private pointer  sp_CSlib_err_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */


int ct10reco_assign_severity_err_recov_tab(SEV_ERROR_RECOV_CLASS_T
							*p_recov_tab,
					   int	 sizeof_table);
/* 
 * Assign private pointer  sp_severity_err_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */


int ct10reco_assign_master_recov_tab(MASTER_RECOV_ENTRY_T *p_recov_tab,
				     int		  sizeof_table);
/* 
 * Assign private pointer  sp_master_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */



/* ---------------------- Configuration and counter reset functions: */


void ct10reco_assign_application_name(char *app_name);
/*
 * Called by:	ct10api.c
 *
 * Assigns the application_name that will be used when ct10reco.c calls
 * alog_aplogmsg_write().
 *
 */



int ct10reco_reconfigure_master_recov_table_from_config_params(void);
/*
 * Called by:	ct10api_load_config()
 *
 * Return:
 *	0	config file had been previously opened correctly.
 *	-1	config file not already loaded.
 *	-2	invalid value in one param (see log).
 */


void ct10reco_reset_current_batch_counters(void);
/* 
 * Reset 'curr_batch_ctr' to 0 in all MASTER_RECOV_ENTRY_T entries
 * of the Master Recov Table.
 *
 * Should be called before each "batch" of transactions.
 *
 *
 * NOTE: 'curr_batch_ctr' is used to decide when to escalate an error
 *	 that has occurred many times to a higher error return code.
 * 
 */



void ct10reco_log_master_recov_stats(void);
/* 
 * Writes to log one line per entry of the Master Recov table that has a
 * total error count > 0
 */




void ct10reco_assign_pf_display_err_msg(int (* ptr_funct)() );
/*
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

#endif /* #ifndef _CT10RECO_H_ */


/* --------------------------------------------- End of ct10reco.h --------- */
