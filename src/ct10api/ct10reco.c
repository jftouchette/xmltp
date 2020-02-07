/* ct10reco.c
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
 * See Design Documents:  ct10retr.cfl, ct10erp1.cfl  (Corel Flow)
 *
 *
 * ------------------------------------------------------------------------
 * Versions:
 * 1996apr04,jft: begun
 * 1996apr09,jft: default recov tables completed
 * 1996apr12,jft: ct10reco_reconfigure_master_recov_table_from_config_params()
 *			corrected use of p_tab->min_debug_level_to_log
 * 1996may02,jft: sf_recover_from_error() update p_ct10_conn->ret_err_code
 * 			only if current ret_err_code is more severe
 * 1996may06,jft: sf_recover_from_error() escalate error only  if 
 *			(p_tab->max_curr_to_escalate > 0)
 * 1996may07,jft: sf_recover_from_error() now, Or bits in p_conn->action_mask
 * 1996jun05,jft: . sf_recover_from_error(): calls (* s_pf_display_err_msg)()
 *		  + ct10reco_assign_pf_display_err_msg()
 * 1996aug23,jft: . sf_recover_from_error(): fixed if about b_to_log and
 *			p_tab->min_debug_level_to_log
 * 1996dec18,jbl: Add recovery action RECOV_ACT_RETURN_CS_FAIL to 
 *		  timeout in master recovery table.
 * 1997feb18,deb: Added conditional include for Windows NT port
 * 1997may28,jbl: Added RECOV_ACT_RETURN_CS_FAIL to INTERNAL_RES_FAIL.
 * 1997sep19,jbl: Added s_first_raiserror_msg_number and related logic.
 * 2003mar19,deb: . RECOV_PAR_ACTION_MASK is overridable with config
 */

#include <stdio.h>
#include <ctype.h>

#include <ctpublic.h>
#include <cstypes.h>
#include <oserror.h>

#include "alogelvl.h"	/* ALOG_xxx defines			*/
#include "ucfg.h"
#include "errutl_1.h"	/* prototypes for errutl_xxxx functions	*/

#include "ct10api.h"
#include "ct10apip.h"
#include "ct10reco.h"

#ifndef OSCLERR_CONNECT_FAILED
#include "osclerr1.h"
#endif

#ifdef WIN32
#include "ntunixfn.h"
#endif

/* -------------------------------------------- Private Defines:  */

/*
 * Constant Strings used in logging error messages:
 */
#define CT10RECO_MODULE_NAME		"ct10reco"
#define NOTIFY_TO_NOBODY		"nobody"

/*
 * Some Stored Procedures want to print messages to stdout, not to the log.
 * The string PRINT_SRV_MSG_TO_STDOUT_STRING at the beginning of a msg
 * indicates that this special behavior is required.
 */
#define PRINT_SRV_MSG_TO_STDOUT_STRING	"~~"




/* -------------------------------------------- Private Structures:  */


/* -------------------------------------------- Private Default Tables:  */

/*
 * DEFAULT 1st Level Error Recovery (Error Classification) Tables
 * --------------------------------------------------------------
 *
 *
 * There are three (3) default tables: Remote-error, CT-Lib, CS-Lib
 *
 *
 * Each entry of one table has the following fields:
 *
 *	msgno 
 *	layer		-- 0 means any layer
 *	origin		-- 0 means any origin
 *	error_class
 *
 * *** Warning: NOT absolutely in ascending sequence of msgno ***
 *
 */
static ERROR_RECOV_CLASS_T	s_remote_err_recov_tab[] = {
    /*	{ msgno, layer, origin, error_class },  */
	{  201,	0,	0,	ERR_CLASS_BAD_PARAM	   }, /*missing param */
	{  208,	0,	0,	ERR_CLASS_MISSING_OBJECT	},
	{  229,	0,	0,	ERR_CLASS_EXEC_PERMISSION_DENIED },
	{  249,	0,	0,	ERR_CLASS_CONVERT_ERROR		},
	{  257,	0,	0,	ERR_CLASS_CONVERT_ERROR		},
	{  265,	0,	0,	ERR_CLASS_CONVERT_ERROR		},
	{  266,	0,	0,	ERR_CLASS_TRANSACTION_COUNT_IMBALANCE	},
	{  276,	0,	0,	ERR_CLASS_BAD_PARAM		}, /* not out */
	{  515,	0,	0,	ERR_CLASS_ATTEMPT_INSERT_NULL	},
	{  567,	0,	0,	ERR_CLASS_ROLE_DENIED		},
	{  911,	0,	0,	ERR_CLASS_MISSING_OBJECT	}, /*database */
	{ 1205,	0,	0,	ERR_CLASS_DEADLOCK		},
	{ 2601,	0,	0,	ERR_CLASS_DUPLICATE_KEY		},
	{ 2812,	0,	0,	ERR_CLASS_MISSING_PROC		},
	{ 4002,	0,	0,	ERR_CLASS_LOGIN_REJECTED	},
	{ 5701,	0,	0,	ERR_CLASS_INFO_MSG_TO_IGNORE	},
	{ 5703,	0,	0,	ERR_CLASS_INFO_MSG_TO_IGNORE	},
	{ 17001, 0,	0,	ERR_CLASS_INFO_MSG_TO_LOG	},
	{ OSCLERR_CONNECT_FAILED,
		0,	0,	ERR_CLASS_LOGIN_REJECTED	},
	{ OSCLERR_RPC_RPC_GATE_IS_CLOSED,
		0,	0,	ERR_CLASS_CIG_GATE_CLOSE	},
	{ OSCLERR_RPC_LANG_GATE_IS_CLOSED,
		0,	0,	ERR_CLASS_CIG_GATE_CLOSE	},

	{ OSCLERR_DT_REG_PROC_BAD_PARAM,
		0,	0,	ERR_CLASS_BAD_REG_PROC_PARAMS },
	{ OSCLERR_DT_CANNOT_SET_RET_PARAM,
		0,	0,	ERR_CLASS_BAD_REG_PROC_PARAMS },
	{ OSCLERR_DT_CANNOT_GET_PARAM_VAL,
		0,	0,	ERR_CLASS_BAD_REG_PROC_PARAMS },
	{ OSCLERR_OS_RP_FN_CANNOT_SET_RET_PARAM,
		0,	0,	ERR_CLASS_BAD_REG_PROC_PARAMS },
	{ OSCLERR_OS_RP_FN_CANNOT_GET_PARAM_VAL,
		0,	0,	ERR_CLASS_BAD_REG_PROC_PARAMS },
	{ OSCLERR_DT_REG_PROC_RP_ID_NOTFND,
		0,	0,	ERR_CLASS_BAD_REG_PROC_DEFINITION },
	{ OSCLERR_OS_RP_FN_RP_ID_NOTFND,
		0,	0,	ERR_CLASS_BAD_REG_PROC_DEFINITION },

	{ -1,	0,	0,	ERR_CLASS_UNKNOWN		}
 };


static ERROR_RECOV_CLASS_T	s_ctlib_err_recov_tab[] = {
	{ 3,	0,	0,	ERR_CLASS_NO_SUCH_SERVER	},
	{ 4,	0,	0,	ERR_CLASS_CANNOT_FIND_HOST	},
	{ 6,	0,	0,	ERR_CLASS_DISCONNECTED_BY_PEER	},
	{ 7,	0,	0,	ERR_CLASS_DISCONNECTED_BY_PEER	},
	{ 44,	0,	0,	ERR_CLASS_LOGIN_REJECTED	},
	{ 50,	0,	0,	ERR_CLASS_DEAD_CONNECTION	},
	{ 63,	0,	0,	ERR_CLASS_TIMEOUT		},
	{ 148,	0,	0,	ERR_CLASS_TIMEOUT		},
	{ -1,	0,	0,	ERR_CLASS_UNKNOWN		}
 };


static ERROR_RECOV_CLASS_T	s_CSlib_err_recov_tab[] = {
	{ -1,	0,	0,	ERR_CLASS_UNKNOWN		}
 };




/*
 * Severity Error Recovery (Error Classification) Table struct:
 * ------------------------------------------------------------
 *
 */
static SEV_ERROR_RECOV_CLASS_T	s_severity_err_recov_tab[] = {
    /*	{ severity,		error_class				},  */
	{  0,			ERR_CLASS_INFO_MSG_TO_LOG		},
	{ 10,			ERR_CLASS_INFO_MSG_TO_LOG		},
	{  8,			ERR_CLASS_INFO_MSG_TO_LOG		},
	{  9,			ERR_CLASS_INFO_MSG_TO_LOG		},
	{ CS_SV_INFORM,		ERR_CLASS_INFO_MSG_TO_LOG		},
	{ CS_SV_CONFIG_FAIL,	ERR_CLASS_CTLIB_CONFIG_ERROR		},
	{ CS_SV_RETRY_FAIL,	ERR_CLASS_CTLIB_ERROR_RETRYABLE		},
	{ CS_SV_API_FAIL,	ERR_CLASS_CTLIB_API_CALL_BAD		},
	{ CS_SV_RESOURCE_FAIL,	ERR_CLASS_CTLIB_INTERNAL_RES_FAIL	},
	{ CS_SV_INTERNAL_FAIL,	ERR_CLASS_CTLIB_INTERNAL_RES_FAIL	},
	{ CS_SV_FATAL,		ERR_CLASS_CTLIB_FATAL			},
	{ CS_SV_COMM_FAIL,	ERR_CLASS_CTLIB_COMM_ERROR_RETRYABLE	},
	{ 11,			ERR_CLASS_SEV_OBJECT_NOT_FOUND		},
	{ 12,			ERR_CLASS_SEV_WRONG_DATATYPE		},
	{ 13,			ERR_CLASS_SEV_TRANSACTION_SCOPE_ERROR	},
	{ 14,			ERR_CLASS_SEV_PERMISSION_DENIED		},
	{ 15,			ERR_CLASS_SEV_SYNTAX_ERROR		},
	{ 16,			ERR_CLASS_SEV_MISC_USER_ERROR		},
	{ 17,			ERR_CLASS_SEV_INSUFFICIENT_SPACE_RES	},
	{ 18,			ERR_CLASS_SEV_FATAL_THREAD_ERROR	},
	{ 19,			ERR_CLASS_SEV_FATAL_RESOURCE_ERROR	},
	{ 20,			ERR_CLASS_SEV_FATAL_THREAD_ERROR	},
	{ 21,			ERR_CLASS_SEV_FATAL_DB_PROCESSES_ERROR	},
	{ 22,			ERR_CLASS_SEV_FATAL_TABLE_ERROR		},
	{ 23,			ERR_CLASS_SEV_FATAL_DATABASE_ERROR	},
	{ 24,			ERR_CLASS_SEV_FATAL_SERVER_ERROR	},
	{ 25,			ERR_CLASS_SEV_FATAL_SERVER_ERROR	},
	{ 26,			ERR_CLASS_SEV_FATAL_SERVER_ERROR	},
	{ -1,			ERR_CLASS_SEV_FATAL_SERVER_ERROR	}
 };




/*
 * 2nd Level (Master) Error Recovery Table struct:
 * ----------------------------------------------
 *
 * The following attributes can be modified by the config parameters:
 *	max_curr_to_escalate,
 *	max_retry, 
 *	retry_interval, 
 *	min_debug_level_to_log
 *
 * Specific values come from a parameter named like this:
 *
 *	<override_param_name>.<attribute>
 *
 */
/* -- See struct in ct10reco.h --
 * { error_class,		override_param_name,
 *	max_curr_to_escalate,	curr_batch_ctr, total_ctr,
 *	diagnostic_mask,	diag_sev_mask,
 *	action_mask,
 *	ret_err_code,
 *	max_retry,	retry_interval,	min_debug_level_to_log },
 */
static MASTER_RECOV_ENTRY_T	s_master_recov_tab[] = {

   { ERR_CLASS_INFO_MSG_TO_IGNORE,	"INFO_MSG_TO_IGNORE",
	MAX_ESC_NO_LIMIT,		0L,	0L,
	CT10_DIAG_INFO_MSG_RECEIVED,	0L,
	RECOV_ACT_DO_NOT_LOG_MSG,
	CT10_SUCCESS,
	0,		0,		DEBUG_TRACE_LEVEL_FULL },

   { ERR_CLASS_INFO_MSG_TO_LOG,		"INFO_MSG_TO_LOG",
	MAX_ESC_NO_LIMIT,		0L,	0L,
	CT10_DIAG_INFO_MSG_RECEIVED,	0L,
	RECOV_ACT_DEFAULT,
	CT10_SUCCESS,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_BAD_RETURN_STATUS,	"BAD_RETURN_STATUS",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_BAD_RETURN_STATUS,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_FAILED,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_ABORTED_BY_CUSTOM_RECOVERY,	"ABORTED_BY_CUSTOM_RECOVERY",
	MAX_ESC_MANY_SERIOUS,		0L,	0L,
	CT10_DIAG_ABORTED_BY_CUSTOM_RECOVERY,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_FAILED,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_DEAD_CONNECTION,		"DEAD_CONNECTION",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_DEAD_CONNECTION,	0L,
	RECOV_ACT_DEFAULT,
	CT10_ERR_FAILED,
	USUAL_EXECUTE_MAX_RETRY,	0,	DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_NO_SUCH_SERVER,		"NO_SUCH_SERVER",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_NO_SUCH_SERVER,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CANNOT_FIND_HOST,	"CANNOT_FIND_HOST",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_CANNOT_FIND_HOST,	0,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_LOGIN_REJECTED,		"LOGIN_REJECTED",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_LOGIN_REJECTED,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CONNECT_TIMEOUT,		"CONNECT_TIMEOUT",
	MAX_ESC_MANY_SERIOUS,		0L,	0L,
	CT10_DIAG_CONNECT_TIMEOUT,	0L,
	RECOV_ACT_DEFAULT,
	CT10_ERR_FAILED,
	USUAL_CONNECT_MAX_RETRY,	0,	DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CTLIB_CONFIG_ERROR,	"CTLIB_CONFIG_ERROR",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_CTLIB_CONFIG_ERROR,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_DUPLICATE_KEY,		"DUPLICATE_KEY",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_DUPLICATE_KEY,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_ATTEMPT_INSERT_NULL,	"ATTEMPT_INSERT_NULL",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_ATTEMPT_INSERT_NULL,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_TRANSACTION_COUNT_IMBALANCE,	"TRANSACTION_COUNT_IMBALANCE",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_TRANSACTION_COUNT_IMBALANCE,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_DEADLOCK,	"DEADLOCK",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_DEADLOCK,		0L,
	(RECOV_ACT_RESET_CONNECTION | RECOV_ACT_FLUSH_RESULTS),
	CT10_ERR_FAILED,
	USUAL_EXECUTE_MAX_RETRY,	DEADLOCK_WAIT_DELAY,
					DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_TIMEOUT,	"TIMEOUT",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_TIMEOUT,		0L,
	(RECOV_ACT_RESET_CONNECTION | RECOV_ACT_FLUSH_RESULTS |
	 RECOV_ACT_RETURN_CS_FAIL),
	CT10_ERR_FAILED,
	USUAL_EXECUTE_MAX_RETRY,	TIMEOUT_EXEC_WAIT_DELAY,
					DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_DISCONNECTED_BY_PEER,	"DISCONNECTED_BY_PEER",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_DISCONNECTED_BY_PEER,	0L,
	(RECOV_ACT_RESET_CONNECTION | RECOV_ACT_FLUSH_RESULTS),
	CT10_ERR_FAILED,
	USUAL_EXECUTE_MAX_RETRY,	0,	DEBUG_TRACE_LEVEL_NONE },


   { ERR_CLASS_MISSING_PROC,	"MISSING_PROC",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_MISSING_PROC,		0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_MISSING_OBJECT,	"MISSING_OBJECT",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_MISSING_OBJECT,	0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_BAD_PARAM,	"BAD_PARAM",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_BAD_PARAM,		0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CONVERT_ERROR,	"CONVERT_ERROR",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_CONVERT_ERROR,	0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_EXEC_PERMISSION_DENIED,	"EXEC_PERMISSION_DENIED",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_EXEC_PERMISSION_DENIED, 0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_ROLE_DENIED,	"ROLE_DENIED",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_ROLE_DENIED,		0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CIG_RESOURCE_ERROR,	"CIG_RESOURCE_ERROR",
	MAX_ESC_MANY_SERIOUS,		0L,	0L,
	CT10_DIAG_CIG_RESOURCE_ERROR,	0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CIG_GATE_CLOSE,	"CIG_GATE_CLOSE",
	MAX_ESC_MANY_SERIOUS,		0L,	0L,
	CT10_DIAG_CIG_GATE_CLOSE,	0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_BAD_REG_PROC_PARAMS,	"BAD_REG_PROC_PARAMS",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_BAD_REG_PROC_PARAMS,	0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_BAD_REG_PROC_DEFINITION,	"BAD_REG_PROC_DEFINITION",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_BAD_REG_PROC_DEFINITION, 0L,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CTLIB_API_CALL_BAD,	"CTLIB_API_CALL_BAD",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_CTLIB_API_CALL_BAD,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CTLIB_ERROR_RETRYABLE,	"CTLIB_ERROR_RETRYABLE",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_CTLIB_ERROR_RETRYABLE,
	RECOV_ACT_DEFAULT,	/* retry */
	CT10_ERR_FAILED,
	USUAL_EXECUTE_MAX_RETRY,	0,	DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CTLIB_COMM_ERROR_RETRYABLE,	"CTLIB_COMM_ERROR_RETRYABLE",
	MAX_ESC_MANY_APP_ERR,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,    CT10_DIAG_SEV_CTLIB_COMM_ERROR_RETRYABLE,
	RECOV_ACT_RESET_CONNECTION,
	CT10_ERR_FAILED,
	USUAL_EXECUTE_MAX_RETRY,	0,	DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CTLIB_INTERNAL_RES_FAIL,	"CTLIB_INTERNAL_RES_FAIL",
	MAX_ESC_MANY_SERIOUS,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_CTLIB_INTERNAL_RES_FAIL,
	(RECOV_ACT_RESET_CONNECTION|
	RECOV_ACT_RETURN_CS_FAIL),
	CT10_ERR_DO_NOT_RETRY,
	USUAL_EXECUTE_MAX_RETRY,	0,	DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_CTLIB_FATAL,	"CTLIB_FATAL",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_CTLIB_FATAL,
	(RECOV_ACT_FORGET_IT | RECOV_ACT_RETURN_CS_FAIL),
	CT10_ERR_FATAL_ERROR,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_OBJECT_NOT_FOUND,	"SEV_OBJECT_NOT_FOUND",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_OBJECT_NOT_FOUND,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_WRONG_DATATYPE,	"SEV_WRONG_DATATYPE",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_WRONG_DATATYPE,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_TRANSACTION_SCOPE_ERROR,	"SEV_TRANSACTION_SCOPE_ERROR",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_TRANSACTION_SCOPE_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_PERMISSION_DENIED,	"SEV_PERMISSION_DENIED",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_PERMISSION_DENIED,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_SYNTAX_ERROR,	"SEV_SYNTAX_ERROR",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_SYNTAX_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_MISC_USER_ERROR,	"SEV_MISC_USER_ERROR",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_MISC_USER_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_INSUFFICIENT_SPACE_RES,	"SEV_INSUFFICIENT_SPACE_RES",
	MAX_ESC_MANY_SERIOUS,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_INSUFFICIENT_SPACE_RES,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_FATAL_THREAD_ERROR,	"SEV_FATAL_THREAD_ERROR",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_FATAL_THREAD_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_DO_NOT_RETRY,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_FATAL_RESOURCE_ERROR,	"SEV_FATAL_RESOURCE_ERROR",
	MAX_ESC_MANY_UNRECOV,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_FATAL_RESOURCE_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_FATAL_ERROR,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },


   { ERR_CLASS_SEV_FATAL_DB_PROCESSES_ERROR,	"SEV_FATAL_DB_PROCESSES_ERROR",
	MAX_ESC_MANY_FATAL,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_FATAL_DB_PROCESSES_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_FATAL_ERROR,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },


   { ERR_CLASS_SEV_FATAL_TABLE_ERROR,	"SEV_FATAL_TABLE_ERROR",
	MAX_ESC_MANY_FATAL,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_FATAL_TABLE_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_FATAL_ERROR,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_FATAL_DATABASE_ERROR,	"SEV_FATAL_DATABASE_ERROR",
	MAX_ESC_MANY_FATAL,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_FATAL_DATABASE_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_FATAL_ERROR,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_SEV_FATAL_SERVER_ERROR,	"SEV_FATAL_SERVER_ERROR",
	MAX_ESC_MANY_FATAL,		0L,	0L,
	CT10_DIAG_SEV_DIAG_POSTED,	CT10_DIAG_SEV_FATAL_SERVER_ERROR,
	RECOV_ACT_FORGET_IT,
	CT10_ERR_FATAL_ERROR,
	0,		0,		DEBUG_TRACE_LEVEL_NONE },

   { ERR_CLASS_UNKNOWN,			"ERR_CLASS_UNKNOWN",
	MAX_ESC_MANY_SERIOUS,		0L,	0L,
	CT10_DIAG_UNKNOWN_ERROR,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_FAILED,
	0,		0,		DEBUG_TRACE_LEVEL_NONE }
} ;




/*
 * This other (single) MASTER_RECOV_ENTRY_T is used when the
 * Master Recovery Table become unusable...
 *
 */
static MASTER_RECOV_ENTRY_T	s_fallback_master_recov_entry =
   { ERR_CLASS_UNKNOWN,			"***FALLBACK_RECOVERY_ENTRY***",
	MAX_ESC_MANY_SERIOUS,		0L,	0L,
	CT10_DIAG_UNKNOWN_ERROR,	0L,
	RECOV_ACT_DO_NOT_RETRY,
	CT10_ERR_FAILED,
	0,		0,		DEBUG_TRACE_LEVEL_NONE
   };



/* --------------------------------------------------- Private Variables:  */

/*
 * Various private variables:
 * -------------------------
 *
 */
static char	s_application_name[CS_MAX_NAME + 1] = "AnonymAp";



/* 
 * Pointers to active recovery tables:
 * ----------------------------------
 *
 * NOTE: Initially, they point to the default tables.
 *
 */
static ERROR_RECOV_CLASS_T	*sp_remote_err_recov_tab = 
							 s_remote_err_recov_tab,
				*sp_ctlib_err_recov_tab = s_ctlib_err_recov_tab,
				*sp_CSlib_err_recov_tab = s_CSlib_err_recov_tab;

static SEV_ERROR_RECOV_CLASS_T	*sp_severity_err_recov_tab =
						s_severity_err_recov_tab;

static MASTER_RECOV_ENTRY_T	*sp_master_recov_tab = s_master_recov_tab;



static int			s_sizeof_remote_err_recov_tab = 
						sizeof(s_remote_err_recov_tab),
				s_sizeof_ctlib_err_recov_tab = 
						sizeof(s_ctlib_err_recov_tab),
				s_sizeof_CSlib_err_recov_tab = 
						sizeof(s_CSlib_err_recov_tab);

static int			s_sizeof_severity_err_recov_tab = 
					     sizeof(s_severity_err_recov_tab);

static int			s_sizeof_master_recov_tab = 
						sizeof(s_master_recov_tab);



/* 
 * Pointers to function that customize the error recovery and
 * diagnostic:
 */
static int	(* s_pf_display_err_msg)() = NULL;



/*
 * This value must be zero or a positive number greater
 * than 20000.
 *
 * It will be used to change the return code return by 
 * ct10api. This new value will be used when the error
 * class is ERR_CLASS_SEV_MISC_USER_ERROR and the error 
 * number is >= to this value.
 *
 * The first time that we need this value the program will
 * read it from the config file. If it's not there
 * it will be initialized to zero.
 *
 * A value of zero mean that we dont test this value.
 *
 */

static int s_first_raiserror_msg_number = -1;



/* --------------------------------------------------- PRIVATE Functions:  */


static int sf_find_error_class_by_msgno(int msg_no, int layer, int origin,
					ERROR_RECOV_CLASS_T *p_tab,
					int tab_sz)
/*
 * Called by:	sf_recover_from_error()
 *
 * Return:
 *	ERR_CLASS_UNKNOWN	msg_no not found or table empty or NULL
 *	p_tab->error_class	error_class matching msg_no, layer, origin
 */
{
	int	i = 0;

	if (p_tab == NULL) {
		return (ERR_CLASS_UNKNOWN);
	}

	for (i = 0; i < (tab_sz / sizeof(ERROR_RECOV_CLASS_T)); i++, p_tab++) {

		if (p_tab->msgno <= -1) {	/* end of table */
			break;
		}

		if ( p_tab->msgno == msg_no
		 && (p_tab->layer == 0   ||  p_tab->layer == layer)
		 && (p_tab->origin == 0  ||  p_tab->origin == origin)
		   ) {
			return (p_tab->error_class);	/* found */
		}
	}
		
	return (ERR_CLASS_UNKNOWN);

} /* end of sf_find_error_class_by_msgno() */





static int sf_find_error_class_by_severity(int severity)
/*
 * Called by:	sf_recover_from_error()
 *
 * Use sp_severity_err_recov_tab to find the error_class matching severity.
 *
 * Return:
 *	ERR_CLASS_UNKNOWN	severity not found or table NULL
 *	p_tab->error_class	error_class matching severity
 */
{
	SEV_ERROR_RECOV_CLASS_T	*p_tab = NULL;
	int			 i = 0;

	for (i = 0, p_tab = sp_severity_err_recov_tab;
	     i < (s_sizeof_severity_err_recov_tab
		  / sizeof(SEV_ERROR_RECOV_CLASS_T) );
	     i++, p_tab++) {

		if (p_tab->severity <= -1) {	/* end of table */
			break;
		}

		if ( p_tab->severity == severity) {
			return (p_tab->error_class);	/* found */
		}
	}
		
	return (ERR_CLASS_UNKNOWN);

} /* end of sf_find_error_class_by_severity() */




static MASTER_RECOV_ENTRY_T *sf_find_recov_entry_for_error_class(
							int error_class)
/*
 * Called by:	sf_recover_from_error()
 *
 * Return:
 *	pointer to a MASTER_RECOV_ENTRY_T
 */
{
	MASTER_RECOV_ENTRY_T	*p_tab = NULL;
	int			 i = 0;

	for (i = 0, p_tab = sp_master_recov_tab;
	     i < (s_sizeof_master_recov_tab / sizeof(MASTER_RECOV_ENTRY_T) );
	     i++, p_tab++) {

		if (p_tab->error_class <= -1) {
			break;
		}
		if (p_tab->error_class == error_class) {
			return (p_tab);
		}
	}

	/* The error_class was NOT found in the Master Recovery Table.
	 * At least ERR_CLASS_UNKNOWN should have been in the Master Table.
	 *
	 * We fallback to a default recovery entry:
	 */
	return ( &s_fallback_master_recov_entry );

} /* end of sf_find_recov_entry_for_error_class() */



long sf_get_first_raise_error_msg_number()
/*
 * Called by:	sf_recover_from_error.
 *
 * Notes:
 *		If value is found in the config file, value is returned.
 *		If value is not found or if value is invalid then zero
 *		is returned.
 *
 * Return: 
 *		first raise error message number or zero.
 *
 */
{
	long first_raiserror_msg_number;
	char *p_param_val	= NULL;

	p_param_val = ucfg_get_param_string_first(
			RECOV_PAR_FIRST_RAISERROR_MSG_NUMBER);

	if (p_param_val == NULL) {
		first_raiserror_msg_number = 0;
	}
	else {
		while (isspace(*p_param_val)) {
			p_param_val++;
		}
		first_raiserror_msg_number = atoi(p_param_val);

		if (first_raiserror_msg_number < 
		    MIN_FIRST_RAISERROR_MSG_NUMBER ) {

			first_raiserror_msg_number = 0;
		}
	}


	return first_raiserror_msg_number;

} /* end of sf_get_first_raise_error_msg_number() */




static CS_RETCODE sf_recover_from_error(CS_CONTEXT	*p_cs_context,
					CS_CONNECTION	*p_cs_conn,
					CS_SERVERMSG	*p_srv_msg,
					CS_CLIENTMSG 	*p_client_msg,
					ERROR_RECOV_CLASS_T *p_msgno_tab,
					int   tab_sz)
/*
 * Called by:	All errors handlers.
 *
 * NOTES:
 *   Argument p_tab value is either = sp_remote_err_recov_tab 
 *   or sp_ctlib_err_recov_tab, or sp_CSlib_err_recov_tab.
 *
 *
 * Return:
 *	CS_RETCODE value to be returned by the error handler.
 */
{
	CS_RETCODE		 cs_rc		= CS_SUCCEED;

	CT10_CONN_T		*p_ct10_conn	= NULL;
	MASTER_RECOV_ENTRY_T	*p_tab		= NULL;

	int			 error_class	= 0;

	int			 msg_no		= 0,
				 layer		= 0,
				 origin		= 0,
				 severity	= 0;

	char			*msg_text	= "empty msg!",
				*os_msg		= "?os_msg?",
				*srv_name	= "?srv_name?",
				*proc_name	= "?proc_name";

	int 			 msg_text_len	= 0,
				 os_msg_len	= 0,
				 srv_name_len	= 0,
				 proc_name_len	= 0,
				 os_number	= 0,
				 proc_line	= 0;

	int			 b_to_log	= 0,
				 ret_err_code	= 0;

	char			*p_app_context_info = "";

	char			 msg_buff[CS_MAX_MSG + 2000] = "?msg?";


	/*
	 * Find which CT10_CONN_T owns p_cs_conn:
	 */
	p_ct10_conn = (CT10_CONN_T *) ct10api_get_owner_of_conn(p_cs_conn);

	if (ct10api_check_ct10_conn_signature( (void *) p_ct10_conn) != 0) {
		p_ct10_conn = NULL;
	}


	/*
	 * Extract error msg information fields from p_srv_msg
	 * or from p_client_msg:
	 */
	if (p_srv_msg != NULL) {

		msg_no		= p_srv_msg->msgnumber;
		severity	= p_srv_msg->severity;

		msg_text	= &(p_srv_msg->text[0]);
		msg_text_len	= p_srv_msg->textlen;

		srv_name	= &(p_srv_msg->svrname[0]);
		srv_name_len	= p_srv_msg->svrnlen;

		proc_name	= &(p_srv_msg->proc[0]);
		proc_name_len	= p_srv_msg->proclen;
		proc_line	= p_srv_msg->line;

	} else if (p_client_msg != NULL) {

		msg_no		= CS_NUMBER(p_client_msg->msgnumber);
		severity	= p_client_msg->severity;
		layer		= CS_LAYER( p_client_msg->msgnumber);
		origin		= CS_ORIGIN(p_client_msg->msgnumber);

		msg_text	= &(p_client_msg->msgstring[0]);
		msg_text_len	= p_client_msg->msgstringlen;

		os_msg		= &(p_client_msg->osstring[0]);
		os_msg_len	= p_client_msg->osstringlen;

		os_number	= p_client_msg->osnumber;
	}

	/* 
	 * Try to find the error by its msg_no:
	 */
	error_class = sf_find_error_class_by_msgno(msg_no, layer, origin,
						   p_msgno_tab, tab_sz);

	/* 
	 * If it could not be found by its number, try by severity:
	 */
	if (error_class == ERR_CLASS_UNKNOWN) {
		error_class = sf_find_error_class_by_severity(severity);
	}

	/*
	 * Find the Master Recovery Entry to use for this error_class:
	 */
	p_tab = sf_find_recov_entry_for_error_class(error_class);


	/*
	 * Decide if msg should be written to the log:
	 */
	b_to_log = !(p_tab->action_mask & RECOV_ACT_DO_NOT_LOG_MSG);

	if (b_to_log) {
		if (ct10api_get_debug_trace_flag() >= DEBUG_TRACE_LEVEL_FULL
		 || ct10api_get_debug_trace_flag()
		 		 >= p_tab->min_debug_level_to_log) {
			b_to_log = TRUE;
		} else {
			b_to_log = FALSE;
		}
	}

	/*
	 * Choose value of cs_rc to be returned:
	 */
	if (p_tab->action_mask & RECOV_ACT_RETURN_CS_FAIL) {
		cs_rc = CS_FAIL;
	} else {
		cs_rc = CS_SUCCEED;
	}


	/*
	 * Increment counters of errors in Recovery Entry:
	 */
	p_tab->curr_batch_ctr++;
	p_tab->total_ctr++;

	/*
	 * Decide if return error code must be taken from table
	 * or if from a raise error. In that case a success is
	 * returned.
	 */

	if (s_first_raiserror_msg_number <= -1) {

		s_first_raiserror_msg_number = 
					sf_get_first_raise_error_msg_number();
	}

	if (	s_first_raiserror_msg_number != 0
	 	&& msg_no > s_first_raiserror_msg_number 
		&& error_class == ERR_CLASS_SEV_MISC_USER_ERROR) {

	 	ret_err_code = CT10_SUCCESS;

	 } else  {

	 	ret_err_code = p_tab->ret_err_code;
	 }


	/*
	 * Decide if ret_err_code is to be escalated:
	 */

	if (p_tab->max_curr_to_escalate > 0
	 && p_tab->curr_batch_ctr >= p_tab->max_curr_to_escalate) {
		ret_err_code += CT10_ERR_INTERVAL;
	}

	/* 
	 * If p_ct10_conn is not NULL, then post the recovery information
	 * in its fields:
	 */
	if (p_ct10_conn != NULL) {

		if (p_ct10_conn->pf_get_appl_context_info != NULL) {
			p_app_context_info = (char *) (* (
				p_ct10_conn->pf_get_appl_context_info)) ();

			if (p_app_context_info == NULL) {
				p_app_context_info = "[appctx?]";
			}
		}

		/* The diagnostic_mask accumulates many bit values
		 * with a logical-OR:
		 */
		p_ct10_conn->diagnostic_mask	|= p_tab->diagnostic_mask;
		p_ct10_conn->diag_sev_mask	|= p_tab->diag_sev_mask;


		/*
		 * Decide if retry should be done:
		 */
		p_ct10_conn->action_mask	|= p_tab->action_mask;

		if (p_ct10_conn->retry_ctr >= p_tab->max_retry) {
			p_ct10_conn->action_mask |= RECOV_ACT_DO_NOT_RETRY;
		}

		/*
		 * Accumulate Fatal Errors:
		 */
		if (ret_err_code <= CT10_ERR_FATAL_ERROR) {
			p_ct10_conn->ctr_of_fatal_errors++;
		}

		/*
		 * Post proposed return code and other retry info:
		 */
		if (ret_err_code < p_ct10_conn->ret_err_code) {
			p_ct10_conn->ret_err_code = ret_err_code;
		}
		p_ct10_conn->max_retry		= p_tab->max_retry;
		p_ct10_conn->retry_interval	= p_tab->retry_interval;
	}

	/*
	 * Call custom error msg display function if ptr not NULL:
	 */
	if (s_pf_display_err_msg != NULL) {
		(* s_pf_display_err_msg)(
				b_to_log,
				p_app_context_info,
				msg_no,
				severity,
				layer,
				origin,
				msg_text_len,
				msg_text,
				srv_name_len,	
				srv_name,
				proc_name_len,
				proc_name,
				os_msg_len,
				os_msg,
				(proc_name_len > 0) ? proc_line : os_number,
				p_tab->override_param_name);
		return (cs_rc);
	}


	/* 
	 * Return immediately if we do NOT have to log:
	 */
	if (!b_to_log) {
		return (cs_rc);
	}

	/*
	 * Write to the error log:
	 */
	sprintf(msg_buff, 
		("%.30s%.2smsg %d, sev %d, (L:%d,O:%d), "
		 "%.*s%.8s%.*s%.8s%.*s%.8s%.*s (%d) "
		 "[%.30s]"),
		p_app_context_info,
		(p_app_context_info[0]) ? ": " : "",
		msg_no,
		severity,
		layer,
		origin,
		msg_text_len,
		msg_text,
		(srv_name_len > 0) ? ", srv=" : "",
		srv_name_len,	
		srv_name,
		(proc_name_len > 0) ? ", proc=" : "",
		proc_name_len,
		proc_name,
		(os_msg_len > 0) ? ", OS-msg=" : "",
		os_msg_len,
		os_msg,
		(proc_name_len > 0) ? proc_line : os_number,
		p_tab->override_param_name);

	alog_aplogmsg_write(errutl_severity_to_alog_err_level(severity),
			    CT10RECO_MODULE_NAME,
			    s_application_name,
			    msg_no,
			    NOTIFY_TO_NOBODY,
			    msg_buff);


	return (cs_rc);

} /* end of sf_recover_from_error() */





static void sf_write_stats_for_recovery_entry(MASTER_RECOV_ENTRY_T *p_entry)
/*
 * Called by:	ct10reco_log_master_recov_stats()
 *
 * If (p_entry->total_ctr > 0) write statistics for entry to the log.
 *
 */
{
	char	msg_buff[CS_MAX_MSG + 2000] = "?msg?";


	if (p_entry == NULL
	 || p_entry->total_ctr == 0) {
	 	return;
	}

	sprintf(msg_buff, "%.30s: %d/%d total: %d",
		p_entry->override_param_name,
		p_entry->curr_batch_ctr,
		p_entry->max_curr_to_escalate,
		p_entry->total_ctr);

	alog_aplogmsg_write(ALOG_INFO,
			    CT10RECO_MODULE_NAME,
			    s_application_name,
			    0,		/* msg_no */
			    NOTIFY_TO_NOBODY,
			    msg_buff);

} /* end of sf_write_stats_for_recovery_entry() */






static int sf_modify_value_if_param_in_config(int *p_val, 
					      char *err_class_name,
					      char *attrib_name,
					      int  min_val,
					      int  max_val)
/*
 * Called by:	sf_modify_value_if_param_in_config_with_diag()
 *
 * Return:
 *	1	OK: new value assigned
 *	0	OK: no such param
 *	-1	p_val is NULL
 *	-2	err_class_name or attrib_name is NULL
 *	-3	param value out of range
 */
{
	char	 param_name[(UCFG_MAX_PARAM_NAME_LEN * 2) + 3] = "euh?";
	char	*p_param_val = NULL;
	int	 new_val = 0;

	if (p_val == NULL) {
		return (-1);
	}
	if (err_class_name == NULL  || attrib_name == NULL) {
	 	return (-2);
	}

	sprintf(param_name, "%.*s.%.*s",
		UCFG_MAX_PARAM_NAME_LEN,
		err_class_name,
		UCFG_MAX_PARAM_NAME_LEN,
		attrib_name);

	p_param_val = ucfg_get_param_string_first(param_name);

	if (p_param_val == NULL) {
		return (0);		/* not found, OK */
	}

	while (isspace(*p_param_val)) {
		p_param_val++;
	}

	new_val = atoi(p_param_val);

	if (new_val < min_val  ||  new_val > max_val) {
		return (-3);
	}

	*p_val = new_val;

	return (1);
	
} /* end of sf_modify_value_if_param_in_config() */





static int sf_modify_value_if_param_in_config_with_diag(int *p_val, 
							char *err_class_name,
							char *attrib_name,
							int  min_val,
							int  max_val)
/*
 * Called by:	ct10reco_reconfigure_master_recov_table_from_config_params()
 *
 * Return:
 *	1	OK: new value assigned
 *	0	OK: no such param
 *	-1	p_val is NULL
 *	-2	err_class_name or attrib_name is NULL
 *	-3	param value out of range
 */
{
	int	 rc = 0;
	char	*p_reason = "what?";
	char	 diag_msg[500] = "hein?";

	rc = sf_modify_value_if_param_in_config(p_val, 
						err_class_name,
						attrib_name,
						min_val,
						max_val);
	if (rc >= 0) {
		return (rc);	/* OK - no diagnostic needed */
	}

	if (err_class_name == NULL) {
		err_class_name = "[null]";
	}
	if (attrib_name == NULL) {
		attrib_name = "[null]";
	}

	switch (rc) {
	 case -1:	p_reason = "p_val is NULL";
			break;
	 case -2:	p_reason = "err_class_name or attrib_name is NULL";
			break;
	 case -3:	p_reason = "param value out of range";
			break;
	 default:	p_reason = "Other error";
	}

	sprintf(diag_msg, "Bad config param '%.*s.%.*s': %.30s [%d..%d]",
		UCFG_MAX_PARAM_NAME_LEN,
		err_class_name,
		UCFG_MAX_PARAM_NAME_LEN,
		attrib_name,
		p_reason,
		min_val,
		max_val);

	alog_aplogmsg_write(ALOG_ERROR, CT10RECO_MODULE_NAME,
			    s_application_name,
			    0,			/* msg_no */
			    NOTIFY_TO_NOBODY,
			    diag_msg);

	return (rc);


} /* end of sf_modify_value_if_param_in_config_with_diag() */





static void sf_update_ctr_according_to_rc(int *p_ctr_errors, 
					  int *p_ctr_modifs, 
					  int rc)
/*
 * Called by:	ct10reco_reconfigure_master_recov_table_from_config_params()
 */
{
	if (rc == 0) {
		return;
	}
	if (p_ctr_errors == NULL || p_ctr_modifs == NULL) {
		return;
	}
	if (rc > 0) {
		(*p_ctr_modifs)++;
	}
	if (rc < 0) {
		(*p_ctr_errors)++;
	}

} /* end of sf_update_ctr_according_to_rc() */






/* --------------------------------------------------- PUBLIC Functions:  */


/* --------------------------------- Callback functions: */


CS_RETCODE CS_PUBLIC ct10reco_remote_error_handler(CS_CONTEXT	*p_cs_context,
					 CS_CONNECTION	*p_cs_conn,
					 CS_SERVERMSG	*p_srv_msg)
/*
 * Referenced by:	ct10api_init_cs_context()
 *
 *
 * CT-Lib server msg (remote connection error) handler:
 *
 *  Parameters:
 *	p_cs_context	ptr to Context structure.
 *	p_conn		ptr to CS_CONNECTION
 *	p_srvmsg	ptr to CS_SERVERMSG structure.
 *
 *  Return:
 *	CS_SUCCEED	(Other value are ILLEGAL and have strange effects)
 */
{
	int	len = 0;

	/*
	 * Special processing if text begins with PRINT_SRV_MSG_TO_STDOUT_STRING
	 *
	 */
	len = strlen(PRINT_SRV_MSG_TO_STDOUT_STRING);

	if (p_srv_msg->textlen > len
	 && (p_srv_msg->severity == 0  ||  p_srv_msg->severity == 10)
	 && !strncmp(p_srv_msg->text, PRINT_SRV_MSG_TO_STDOUT_STRING, len) ) {

		fprintf(stdout, "%.*s\n",
			(p_srv_msg->textlen - len),
			&p_srv_msg->text[len] );

		fflush(stdout);

		return (CS_SUCCEED);
	}

	sf_recover_from_error(  p_cs_context, 	p_cs_conn,
				p_srv_msg,	NULL,
				sp_remote_err_recov_tab,
				s_sizeof_remote_err_recov_tab );

	return (CS_SUCCEED);

} /* end of ct10reco_remote_error_handler() */






CS_RETCODE CS_PUBLIC ct10reco_ct_lib_error_handler(CS_CONTEXT	*p_cs_context,
					 CS_CONNECTION	*p_cs_conn,
					 CS_CLIENTMSG	*p_libmsg)
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
 *	p_libmsg	ptr to CS_CLIENTMSG structure.
 *
 *  Return:
 *	CS_SUCCEED or CS_FAIL	depending on error recovery.
 */
{
	CS_RETCODE	cs_rc = CS_SUCCEED;

	cs_rc = sf_recover_from_error(p_cs_context, 	p_cs_conn,
				      NULL,		p_libmsg,
				      sp_ctlib_err_recov_tab,
				      s_sizeof_ctlib_err_recov_tab );

	return (cs_rc);

} /* end of ct10reco_ct_lib_error_handler() */






CS_RETCODE ct10reco_CS_lib_error_handler(CS_CONTEXT   *p_cs_context,
					 CS_CLIENTMSG *p_libmsg)
/*
 * Referenced by:	ct10api_init_cs_context()
 *
 *
 * CS-Lib error handler.
 *
 *
 *  Parameters:
 *	p_cs_context	ptr to CS_CONTEXT structure.
 *	p_libmsg	ptr to CS_CLIENTMSG structure.
 *
 *  Return:
 *	CS_SUCCEED or CS_FAIL	depending on error recovery.
 */
{
	CS_RETCODE	cs_rc = CS_SUCCEED;

	cs_rc = sf_recover_from_error(p_cs_context, 	NULL,
				      NULL,		p_libmsg,
				      sp_CSlib_err_recov_tab,
				      s_sizeof_CSlib_err_recov_tab );

	return (cs_rc);

} /* end of ct10reco_CS_lib_error_handler() */






/* ---------------------- Functions to assign pointers to functions: */


int ct10reco_assign_remote_err_recov_tab(ERROR_RECOV_CLASS_T  *p_recov_tab,
					 int	sizeof_table)
/* 
 * Assign private pointer  sp_remote_err_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */
{
	if (p_recov_tab == NULL) {
		return (-1);
	}

	if (sizeof_table <= 0
	 || (sizeof_table % sizeof(ERROR_RECOV_CLASS_T)) != 0) {
	 	return (-2);
	}

	sp_remote_err_recov_tab = p_recov_tab;
	s_sizeof_remote_err_recov_tab = sizeof_table;

	return (0);

} /* end of ct10reco_assign_remote_err_recov_tab() */





int ct10reco_assign_ctlib_err_recov_tab(ERROR_RECOV_CLASS_T  *p_recov_tab,
					int	sizeof_table)
/* 
 * Assign private pointer  sp_ctlib_err_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */
{
	if (p_recov_tab == NULL) {
		return (-1);
	}

	if (sizeof_table <= 0
	 || (sizeof_table % sizeof(ERROR_RECOV_CLASS_T)) != 0) {
	 	return (-2);
	}

	sp_ctlib_err_recov_tab = p_recov_tab;
	s_sizeof_ctlib_err_recov_tab = sizeof_table;

	return (0);

} /* end of ct10reco_assign_ctlib_err_recov_tab() */






int ct10reco_assign_CSlib_err_recov_tab(ERROR_RECOV_CLASS_T  *p_recov_tab,
					int	sizeof_table)
/* 
 * Assign private pointer  sp_CSlib_err_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */
{
	if (p_recov_tab == NULL) {
		return (-1);
	}

	if (sizeof_table <= 0
	 || (sizeof_table % sizeof(ERROR_RECOV_CLASS_T)) != 0) {
	 	return (-2);
	}

	sp_CSlib_err_recov_tab = p_recov_tab;
	s_sizeof_CSlib_err_recov_tab = sizeof_table;

	return (0);

} /* end of ct10reco_assign_CSlib_err_recov_tab() */





int ct10reco_assign_severity_err_recov_tab(SEV_ERROR_RECOV_CLASS_T
							*p_recov_tab,
					   int	 sizeof_table)
/* 
 * Assign private pointer  sp_severity_err_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */
{
	if (p_recov_tab == NULL) {
		return (-1);
	}

	if (sizeof_table <= 0
	 || (sizeof_table % sizeof(SEV_ERROR_RECOV_CLASS_T)) != 0) {
	 	return (-2);
	}

	sp_severity_err_recov_tab = p_recov_tab;
	s_sizeof_severity_err_recov_tab = sizeof_table;

	return (0);

} /* end of ct10reco_assign_severity_err_recov_tab() */




int ct10reco_assign_master_recov_tab(MASTER_RECOV_ENTRY_T *p_recov_tab,
				     int		  sizeof_table)
/* 
 * Assign private pointer  sp_master_recov_tab  hidden in ct10reco.c
 *
 * Return:
 *	0	OK
 *	-1	pointer is NULL
 *	-2	weird size
 *
 */
{
	if (p_recov_tab == NULL) {
		return (-1);
	}

	if (sizeof_table <= 0
	 || (sizeof_table % sizeof(MASTER_RECOV_ENTRY_T)) != 0) {
	 	return (-2);
	}

	sp_master_recov_tab = p_recov_tab;
	s_sizeof_master_recov_tab = sizeof_table;

	return (0);

} /* end of ct10reco_assign_master_recov_tab() */




/* ---------------------- Configuration and counter reset functions: */



void ct10reco_assign_application_name(char *app_name)
/*
 * Called by:	ct10api.c
 *
 * Assigns the application_name that will be used when ct10reco.c calls
 * alog_aplogmsg_write().
 *
 */
{
	strncpy(s_application_name, app_name, sizeof(s_application_name) );

	s_application_name[sizeof(s_application_name) - 1] = '\0';
}





int ct10reco_reconfigure_master_recov_table_from_config_params()
/*
 * Called by:	ct10api_load_config()
 *
 * Return:
 *	0	config file had been previously opened correctly.
 *	-1	config file not already loaded.
 *	-2	invalid value in one param (see log).
 */
{
	MASTER_RECOV_ENTRY_T	*p_tab = NULL;
	int			 i = 0,
				 rc = 0,
				 ctr_errors = 0,
				 ctr_modifs = 0;
	char			 diag_msg[500] = "euh!?";


	for (i = 0, p_tab = sp_master_recov_tab, 
	     ctr_errors = 0, ctr_modifs = 0;
	     i < (s_sizeof_master_recov_tab / sizeof(MASTER_RECOV_ENTRY_T) );
	     i++, p_tab++) {

		if (p_tab->error_class <= -1) {
			break;
		}

		rc = sf_modify_value_if_param_in_config_with_diag(
					(int *) &(p_tab->max_curr_to_escalate),
					p_tab->override_param_name,
					RECOV_PAR_MAX_CURR_TO_ESCALATE,
					0,
					1000000);

		sf_update_ctr_according_to_rc(&ctr_errors, &ctr_modifs, rc);

		rc = sf_modify_value_if_param_in_config_with_diag(
					&(p_tab->max_retry),
					p_tab->override_param_name,
					RECOV_PAR_MAX_RETRY,
					0,
					10);

		sf_update_ctr_according_to_rc(&ctr_errors, &ctr_modifs, rc);

		rc = sf_modify_value_if_param_in_config_with_diag(
					&(p_tab->retry_interval),
					p_tab->override_param_name,
					RECOV_PAR_RETRY_INTERVAL,
					0,
					100);

		sf_update_ctr_according_to_rc(&ctr_errors, &ctr_modifs, rc);

		rc = sf_modify_value_if_param_in_config_with_diag(
					&(p_tab->min_debug_level_to_log),
					p_tab->override_param_name,
					RECOV_PAR_MIN_DEBUG_LEVEL_TO_LOG,
					DEBUG_TRACE_LEVEL_NONE,
					DEBUG_TRACE_LEVEL_FULL * 2);

		sf_update_ctr_according_to_rc(&ctr_errors, &ctr_modifs, rc);

		rc = sf_modify_value_if_param_in_config_with_diag(
					&(p_tab->action_mask),
					p_tab->override_param_name,
					RECOV_PAR_ACTION_MASK,
					0,
					31);

		sf_update_ctr_according_to_rc(&ctr_errors, &ctr_modifs, rc);
	}

	if (ctr_errors > 0) {
		return (-2);
	}
	
	if (ctr_modifs > 0) {
		sprintf(diag_msg, 
		     "Master recov table: %d values changed by config params",
		     ctr_modifs);

		alog_aplogmsg_write(ALOG_INFO, CT10RECO_MODULE_NAME,
				    s_application_name,
				    0,			/* msg_no */
				    NOTIFY_TO_NOBODY,
				    diag_msg);
	}

	return (0);

} /* end of ct10reco_reconfigure_master_recov_table_from_config_params() */






void ct10reco_reset_current_batch_counters()
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
{
	MASTER_RECOV_ENTRY_T	*p_tab = NULL;
	int			 i = 0;

	for (i = 0, p_tab = sp_master_recov_tab;
	     i < (s_sizeof_master_recov_tab / sizeof(MASTER_RECOV_ENTRY_T) );
	     i++, p_tab++) {
		
		if (p_tab->error_class <= -1) {
			break;
		}
		p_tab->curr_batch_ctr = 0L;
	}

} /* end of ct10reco_reset_current_batch_counters() */






void ct10reco_log_master_recov_stats()
/* 
 * Writes to log one line per entry of the Master Recov table that has a
 * total error count > 0
 */
{
	MASTER_RECOV_ENTRY_T	*p_tab = NULL;
	int			 i = 0;


	sf_write_stats_for_recovery_entry( &s_fallback_master_recov_entry );

	for (i = 0, p_tab = sp_master_recov_tab;
	     i < (s_sizeof_master_recov_tab / sizeof(MASTER_RECOV_ENTRY_T) );
	     i++, p_tab++) {
		
		if (p_tab->error_class <= -1) {
			break;
		}

		sf_write_stats_for_recovery_entry(p_tab);
	}

} /* end of ct10reco_log_master_recov_stats() */




void ct10reco_assign_pf_display_err_msg(int (* ptr_funct)() )
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
{
	s_pf_display_err_msg = ptr_funct;
}


/* --------------------------------------------- End of ct10reco.c -------- */
