/* ct10api.c
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
 * $Source: /ext_hd/cvs/ct10api/ct10api.c,v $
 * $Header: /ext_hd/cvs/ct10api/ct10api.c,v 1.47 2004/10/13 18:23:22 blanchjj Exp $
 *
 * Functions prefix:	ct10api_
 *
 * 
 * Generic Client-Libray functions. 
 *
 *
 * Higher-level functions over Sybase Open Client CT-Lib and CS-Lib.
 *
 *
 *
 * ------------------------------------------------------------------------
 * Versions:
 * 1996apr04,jft: begun defining private (hidden) variables
 * 1996apr10,blanchjj: begun programming stub functions (no error handling).
 * 1996may02,blanchtt: Add error handling.
 * 1996jun06,jft: + ct10api_get_server_name_of_conn(void *p_conn)
 * 1996jun10,jbl: ct10api_process_row_or_params, remove printing of number of
 *		  rows if egal to -1.
 * 1996jun12,jbl: Add of error handling in bulk functions.
 * 1996jun14,jbl: Don`t do a ct_param if param_array[x] is null.
 * 1996aug08,jbl: Replace return code fatal by error for bulk function.
 * 1996sep25,jft: ct10api_apl_log() return immediately if (s_log_mode_quiet)
 *		  added: void ct10api_set_log_mode_quiet(int new_val)
 * 1996sep25,jbl: Replace malloc by calloc, to fix core dump problem.
 * 1996oct08,jbl: Add function ct10api_set_packet_size(CS_INT p_size).
 * 1996oct16,jbl: Add call to ct10rpc_free in rpc_string function.
 * 1996nov15,jbl: Add 1 to retry_ctr to stop after to many retry.
 * 1996dec03,jbl: Fix timeout problem: Do not return a FATAL_ERROR after
 *		  an open client function call that fail. Return what
 *		  ct10reco says to return. If ct10reco says SUCCESS then
 *		  return FAIL but not FATAL_ERROR.
 * 1996dec05,jbl: If the drop command fail on a language event we must
 *		  reset the connect right now. The reset connection 
 *		  wont work before the next language event because
 *		  recovery info are reinitialize.
 * 1996dec05,jbl: Add function ct10api_get_ct_version() to get open client
 *		  version info.
 * 1996dec17,jbl: The first change made on dec 05, 1996 was not good. 
 *		  The result of reconnecting after a fail language event 
 *		  was the lost of recovery info. I fix the proc:
 *		  ct10api_check_and_reset_connection 
 *		  Now it detect a dead connection before the language event.
 * 1996dec17,jbl: add 2 functions: 	ct10api_save_recov_info
 *					ct10api_restore_recov_info
 *		  To fix reconnect recovery error. When a retry need a 
 *		  reconnect and at the same time there is an init string
 *		  to execute we were loosing the recovery info.
 *
 * 1997jan13,jft: cleaned up for port to NT (stays compatible with HP-UX!)
 *		  ---->	static char *version_info = VERSION_ID;
 * 1997jan24,jft: ct10api_process_row_or_params() changed print format string
 *		  to make it work with the MS C compiler under Windows NT
 * 1997fev05,jbl: Drop connection before a new open.
 * 1997fev06,jbl: ct10api_exec_rpc_string return CT10_ERR_RPC_PARSING_ERROR
 *		  on a syntax error.
 * 1997fev12,jbl: p_conn=NULL after drop connection.
 * 1997feb18,deb: . Added conditional include for Windows NT port
 *		  . Added conditional declaration of ct10api_cancel_conn_all()
 *		  . Added conditional Sleep(0) in ct10api_ct_close
 * 1997mar03,deb: Added ct10api_get_context() to allow client to call cs_dt_crack()
 * 1997mar11,jbl: Added trace params in create_connection.
 * 1997mar15,jbl: fix NT warning message.
 * 1997mar15,jbl: Added more trace: DEBUG_TRACE_LEVEL_LOW will now trace all ct10api call.
 * 1997may29,jbl: Added: int ct10api_change_exec_timeout_value(CS_INT exec_timeout)
 * 1997may29,jbl: Added: 	ct10api_exec_rpc_value
 *				ct10api_free_tab_ct10dataxdef
 *				ct10api_convert_tab_csdatafmt_to_ct10dataxdef
 * 
 * 1997jun18,jbl: Change made to: ct10api_convert_tab_csdatafmt_to_ct10dataxdef
 *    ((*p_tab_p_ct10_data_xdef)[i])->len = ((*p_tab_p_ct10_data_xdef)[i])->fmt.maxlength;
 * 1997jul17,deb: Fixed the free of the p_val in ct10api_free_tab_ct10dataxdef()
 *
 * 1997jul17,jbl: Add these date conversion functions:
 *			ct10api_convert_date_to_long
 *			ct10api_crack_date
 *			ct10api_convert_date_to_string
 * 1997jul28,jbl: Return output parameters even if there is a flush_result
 *		  active. This change is require for the 32 bits vb 
 *		  application and the raise error stuff.
 * 1997sep16,jbl: Initialize return status to -999.
 * 1997sep17,deb: If there is a parsing error in ct10api_exec_rpc_string(), do
 *		  not try to dump the rpc to the log
 * 1997oct17,jbl: ct10api_convert_date_to_long: Add 1 to the month return
 *		  by datecrack because it is zero base.
 * 2000jun27,deb: . Added sf_get_login_timeout_param(void) to get the timeout of
 *                  the login operation, the default is the "old" behaviour,
 *                  that is there is no timeout on the login, but it can be set
 *                  by a parameter: UCFG_LOGIN_TIMEOUT
 * 2001oct31,jbl: Added: ct10api_change_connection_params.
 * 2001nov21,jbl: Added: ct10api_change_and_exec_init_string.
 * 2002feb20,jbl: Added: ct10api_convert_date_to_string_std.
 * 2002jul27,jbl: check in changes by Denis Benoit for BULK stuff.
 * 		  Added: ct10api_get_result_state
 * 2002sep05,jbl: Fixed: month -1 problem in ct10api_convert_date_to_string_std
 * 2002oct20,jbl: Added: ct10api_get_action_mask required for xml2syb 
 * 2003jul10,jbl: Added: ct10api_set_row_count + call ct_option to set rowcount
 * 2003jul11,jbl: Fixed return status -999 when ct10api_exec_rpc return before the end
 * 2003aug29,jbl: Fixe error reporting when ct_option fail because of disconnect
 * 		  in ct10api_exec_rpc.
 *
 */

#ifdef WIN32
#include "ntunixfn.h"
#endif

#include        <stdio.h>       /* Definition pour stdio standard de UNIX. */
#include        <ctpublic.h>    /* Definition ralative a open client */
#include	<bkpublic.h>	/* Bulk Lib. */
#include        <string.h>      /* Definition pour fonctions de chaine. */
#include	<malloc.h>

#include "ucfg.h"
#include "check_num_string.h"
#include "linklist.h"

#include "sh_fun_d.h"
#include "apl_log1.h"

#include "ct10api.h"
#include "ct10apip.h"
#include "ct10reco.h"

#include "ct10rpc_parse.h"


/*
 * ------------------------------------ Local variables :
 * 
 */

static int	s_log_mode_quiet = 0;	/* FALSE: ct10api_apl_log() will */
					/* really write to the log	 */

static char buf[LOG_BUFFER_LEN];	/* Use to log informations. */

static char s_safety_buffer[10000];	/* in case global buf[] overflows */

/* The VC++ compiler complains because the function is used before
 * it is defined...
 */
#ifdef WIN32
int ct10api_cancel_conn_all(CS_COMMAND *p_cmd, CS_CONNECTION *p_conn);
#endif

/*
 * ------------------------------------ Global static (private) variables :
 * 
 */


/* 
 * sp_current_active_conn points to the current active CT10_CONN_T.
 *
 * It is used by the CS-Lib error handler to identify which CT10_CONN_T
 * has caused the error.
 *
 * This trick would be unreliable in a multi-threaded or asynchronous 
 * enviroment.
 */
static  CT10_CONN_T	*sp_current_active_conn=NULL;



/*
 * sp_list_of_conn is the list of all CT10_CONN_T
 *
 * This list is searched (by p_curr->p_conn) to find the CT10_CONN_T that
 * owns a specific CS_CONNECTION.
 *
 */
static void		*sp_list_of_conn=NULL;


/*
 * Trace level to be used by ct10api.
 *
 */
static int debug_trace_flag=DEBUG_TRACE_LEVEL_NONE;

/*
 * Version information.
 *
 */

static char *version_info = VERSION_ID;


/*
 * application name use by log functions. 
 *
 */

static char *appl_name;

/*
 * Context to be used.
 *
 */

static CS_CONTEXT *context;

/*
 * Log application message type table.
 *
 */

static struct int_str_tab_entry s_type_strings_table[] = {
#include "alogtypt.h"

	{ ISTR_NUM_DEFAULT_VAL,  "?     " }
};



/*
 * TDS packet size.
 *
 */

static CS_INT ct10_packet_size=0;


static int s_row_count=-1;


/*
 * ---------------------------- Begin of global functions ................ */


/*
 *****************************************************************************
 Fonction de Callback pour les messages de serveur.
 Use for test only. All real callback function are in ct10reco.c.
 *****************************************************************************/

CS_RETCODE srv_callback_msg(	CS_CONNECTION	*connection,
				CS_COMMAND	*cmd,
				CS_SERVERMSG	*srvmsg)
{
	sprintf(buf,"%ld: %s\n", srvmsg->msgnumber, srvmsg->text);
	ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	return CS_SUCCEED;
} /* --------------------------- End of srv_callback_msg -------------------- */



/*
 *****************************************************************************
 Fonction de Callback pour les messages du client.
 Use for test only. All real callback function are in ct10reco.c.
 *****************************************************************************/

CS_RETCODE clt_callback_msg(	CS_CONTEXT	*context,
				CS_CONNECTION	*connection,
				CS_CLIENTMSG	*cltmsg)
{
	sprintf(buf,"%ld %s\n", cltmsg->msgnumber,cltmsg->msgstring);
	ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	return CS_SUCCEED;
} /* --------------------------- End of clt_callback_msg -------------------- */



/*
 *********************
 Get version info.
 *********************/

char *ct10api_get_version()
{
	return version_info;
} /* --------------------------- End of ct10api_get_version ----------------- */



/*
 *****************************
 Get open client version info.
 *****************************/

char *ct10api_get_ct_version()
{
static char ct_version[1024];
CS_RETCODE rc;
CS_INT outlen=0,size=1024;

	rc=ct_config(context,CS_GET,CS_VER_STRING,ct_version,size,&outlen);
	return ct_version;
} /* --------------------------- End of ct10api_get_version ----------------- */



/*
 *************************
 Get trace level flag. 
 *************************/

int ct10api_get_debug_trace_flag()
{
	return debug_trace_flag;
} /* --------------------------- End of ct10api_get_debug_trace_flag -------- */



/*
 **************************
 Set trace level flag. 
 **************************/

void ct10api_assign_debug_trace_flag(int new_val)
{
	debug_trace_flag = new_val;
} /* --------------------------- End of ct10api_assign_debug_trace_flag ----- */



/*
 *************************************************
 Set application name to use with log functions. 
 *************************************************/

void ct10api_assign_application_name(char *app_name) 
{
	appl_name = app_name;

	/* Set application name to be used by ct10reco module. */
	ct10reco_assign_application_name(app_name);
} /* --------------------------- End of ct10api_assign_application_name ----- */



/*
 *******************************
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



/*
 ****************************
 Write log message.
 ****************************/

int ct10api_apl_log(	int	err_level, 	/* ALOG_INFO, ALOG_WARN, ... */
			int	msg_no,		/* Message number.	     */
			char	*msg_text,	/* message text.             */
			int	level_to_log)   /* Trace level needed to log.*/
{
int rc=0;

if (s_log_mode_quiet) {
	return (0);
}

if ((err_level != ALOG_INFO) || (level_to_log <= debug_trace_flag)) {
	rc=alog_aplogmsg_write(	err_level, MODULE_NAME, appl_name,
	  			msg_no, DEFAULT_NOTIFY, msg_text);
}
return rc;
} /* --------------------------- End of ct10api_apl_log --------------------- */



/*
 ************************
 Config file reading. 
 ************************/

int ct10api_load_config(char *config_file, char *section_name)
{
int rc=0;
if ((rc=ucfg_open_cfg_at_section(config_file, section_name)) != 0) {

	fprintf(stderr,"Error openning config file '%s', section '%s', rc: '%d'\n",
	config_file,
	section_name,
	rc);
}

return rc;
} /* --------------------------- End of ct10api_load_config ----------------- */



/*
 ***************************************
 Setting network TDS packet size.
 ***************************************/

int ct10api_set_packet_size(CS_INT p_size)
{
sprintf(buf,"ct10api_set_packet_size(p_size[%ld])", p_size);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ( (p_size >= 512) && (p_size <= 2048) ) {
	ct10_packet_size=p_size;
	return CT10_SUCCESS;
}
else {
	sprintf(buf,"Packet size setting error, value out of range");
	ct10api_apl_log(ALOG_WARN, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FAILED;
}

} /* ................. End of ct10api_set_packet_size ................ */



/*
 *****************************************************************
 This function decide what return status to use when a ct10 
 function call fail. If ct10reco detected the error we return
 this value if not we must return CT10_ERR_FAILED.
 *****************************************************************/

int ct10api_ct10fail_return_value(CT10_CONN_T *pt)
{
if (pt->ret_err_code < CT10_SUCCESS) {
	return pt->ret_err_code;
}
else {
	return CT10_ERR_FAILED;
}

} /* .................. End of ct10api_ct10fail_return_value .............. */




static int sf_get_login_timeout_param(void) {
	int  timeout = CS_NO_LIMIT;
	char *p_val;
	char msgbuf[LOG_BUFFER_LEN];	/* Use to log informations. */

	if ((p_val = ucfg_get_param_string_first(UCFG_LOGIN_TIMEOUT)) != NULL) {
		if (cns_check_int_string(p_val) == CNS_VALID) {
			timeout = atol(p_val);
		} else {
			sprintf(msgbuf,"Invalid config parameter '%s'",
				UCFG_LOGIN_TIMEOUT);
			ct10api_apl_log(ALOG_WARN, 0, msgbuf, DEBUG_TRACE_LEVEL_NONE);
		}
	} else {
		sprintf(msgbuf,"Config parameter '%s' not set, using default",
			UCFG_LOGIN_TIMEOUT);
		ct10api_apl_log(ALOG_INFO, 0, msgbuf, DEBUG_TRACE_LEVEL_NONE);
	}

	return timeout;
} /* .................... End of sf_get_login_timeout_param ................ */



/*
 ****************************
 Context initialization. 
 ****************************/

int ct10api_init_cs_context() 
{
CS_RETCODE	rc2;		/* Return code by open client lib. */

CS_INT	login_timeout,
	exec_timeout;

char	*buf2;

sprintf(buf,"ct10api_init_cs_context");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

/******************************************
 Login time out must be set to CS_NO_LIMIT
 because of a bug of ct_lib.

 TRUE only for sybase 10.0.3 and below
 ******************************************/

login_timeout = sf_get_login_timeout_param();

/* Initialize exec timeout. */

if ((buf2=ucfg_get_param_string_first(UCFG_EXEC_TIMEOUT)) != NULL) {
	if ( cns_check_int_string(buf2)	== CNS_VALID ) {
		exec_timeout = atol(buf2);
	}
	else {
		sprintf(buf,"Invalid config parameter '%s'",UCFG_EXEC_TIMEOUT);
		ct10api_apl_log(ALOG_WARN, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		exec_timeout  = DEFAULT_EXEC_TIMEOUT;
	}
}
else {
	exec_timeout  = DEFAULT_EXEC_TIMEOUT;
}


/*
 * Allocation de la memoire necessaire au context.O
 */

if ((rc2 = cs_ctx_alloc(CS_VERSION_100, &context)) != CS_SUCCEED) {
	sprintf(buf,"cs_ctx_alloc error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

/*
 * Initialisation du context.
 */

#ifdef WIN32
	Sleep( 0 );
#endif

if ((rc2 = ct_init(context, CS_VERSION_100)) != CS_SUCCEED) {
	sprintf(buf,"ct_init error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

#ifdef WIN32
	Sleep( 0 );
#endif

/*
 * Modification des valeurs defauts du context.
 */


/* Time out pour execution. */

if ((rc2=ct_config(	context, CS_SET,CS_TIMEOUT,
			(CS_VOID *) &exec_timeout,
			CS_UNUSED, NULL)) != CS_SUCCEED) {
	sprintf(buf,"Exec timeout setting error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

/* Time out pour login. */

if ((rc2=ct_config(	context, CS_SET, CS_LOGIN_TIMEOUT,
			(CS_VOID *) &login_timeout,
			CS_UNUSED, NULL)) != CS_SUCCEED) {
	sprintf(buf,"Login timeout setting error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}


/*
 * Installation des fonctions de callback.
 */

if ((rc2=ct_callback(context, NULL, CS_SET, CS_SERVERMSG_CB,
		(CS_VOID *) ct10reco_remote_error_handler)) != CS_SUCCEED) {
	sprintf(buf,"CS_SERVERMSG_CB error on ct_callback, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

if ((rc2=ct_callback(context, NULL, CS_SET, CS_CLIENTMSG_CB,
		(CS_VOID *) ct10reco_ct_lib_error_handler)) != CS_SUCCEED) {
	sprintf(buf,"CS_CLIENTMSG_CB error on ct_callback, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

if ((rc2=cs_config(context, CS_SET, CS_MESSAGE_CB,
		(CS_VOID *) ct10reco_CS_lib_error_handler,
		CS_UNUSED,NULL)) != CS_SUCCEED) {
	sprintf(buf,"CS_MESSAGE_CB error on cs_config, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

return CT10_SUCCESS;
} /* --------------------------- End of ct10api_init_cs_context ------------- */


/*
 * We need to change connection information after the call to ct10init.
 * This is require for xml2syb. The program xml2syb start and wait for
 * a Connect Registered proc that will change default connection info and
 * try to connect to sybase. 
 */

static void sf_change_connection_params(CT10_CONN_T *pt, 
				char *login_id, 
				char *password,
				char *app_name, 
				char *host_name,
				char *server_name) {

	if (login_id != NULL) {
		strcpy(pt->login_id, login_id);
	}

	if (password != NULL) {
		strcpy(pt->password, password);
	}

	if (app_name != NULL) {
		strcpy(pt->app_name, app_name);
	}

	if (host_name != NULL) {
		strcpy(pt->host_name, host_name);
	}

	if (server_name != NULL) {
		strcpy(pt->server_name, server_name);
	}

} /* End of sf_change_connection_params */



/*
 ******************************************
 Initialise the bulk for a table
 ******************************************/

static int sf_bulk_init(void *p_ct10_conn,
                        char *tbl_name,
			int with_ident_data)
{
CT10_CONN_T *pt;
int i;
CS_RETCODE rc2;
CS_INT prop_value = CS_TRUE;

pt = p_ct10_conn;

if ((rc2=blk_alloc(pt->p_conn, BLK_VERSION_100, &(pt->p_blk)))!=CS_SUCCEED) {
	sprintf(buf,"blk_alloc error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	if (pt->ret_err_code < CT10_ERR_FAILED) {
		return pt->ret_err_code;
	}
	else {
		return CT10_ERR_FAILED;
	}
}

if (with_ident_data) {
	if ((rc2=blk_props(pt->p_blk, CS_SET, BLK_IDENTITY,
			&prop_value, CS_UNUSED, NULL))!=CS_SUCCEED) {

		sprintf(buf,"blk_props error, rc '%ld'",rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

		if (pt->ret_err_code < CT10_ERR_FAILED) {
			return pt->ret_err_code;
		}
		else {
			return CT10_ERR_FAILED;
		}
	}
}


if ((rc2=blk_init(pt->p_blk,CS_BLK_IN, tbl_name, CS_NULLTERM))!=CS_SUCCEED) {
	sprintf(buf,"blk_init error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	if (pt->ret_err_code < CT10_ERR_FAILED) {
		return pt->ret_err_code;
	}
	else {
		return CT10_ERR_FAILED;
	}
}

return CT10_SUCCESS;
} /*....................... End of sf_bulk_init .......................*/




/*
 ******************************************
 bind the columns for a bulk
 ******************************************/

static int sf_bulk_bind(void *p_ct10_conn,
                        char *tbl_name,
                        CT10_DATA_XDEF_T  *columns_array[],
                        int nb_columns,
			char *ident_col_name,
			int with_ident_data)
{
CT10_CONN_T *pt;
int i;
CS_RETCODE rc2;

pt = p_ct10_conn;

for(i=0;i<nb_columns;i++) {

if (columns_array[i] != NULL) {
	if ((!with_ident_data) &&
	    (NULL != ident_col_name) &&
	    (!strncasecmp(columns_array[i]->fmt.name, ident_col_name, CS_MAX_NAME))) {

		/* Found the identity column!  DON'T BIND! */
		continue;
	}

	if ((rc2=blk_bind(pt->p_blk, i+1, 	&(columns_array[i]->fmt),
						columns_array[i]->p_val,
						&(columns_array[i]->len),
						&(columns_array[i]->null_ind)))
			!= CS_SUCCEED) {
		sprintf(buf,"blk_bind error, column '%d', rc '%ld'",i+1,rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

		if (pt->ret_err_code < CT10_ERR_FAILED) {
			return pt->ret_err_code;
		}
		else {
			return CT10_ERR_FAILED;
		}
	}
}

}

return CT10_SUCCESS;

} /*....................... End of sf_bulk_bind .......................*/




/*
 *************************
 Create connection. 
 *************************/

void *ct10api_create_connection(char *login_id, char *password,
				char *app_name, char *host_name,
				char *server_name,
				char *init_string,
				long options_mask)
{
int rc;

sprintf(buf, "ct10api_create_connection(login[%.8s] app[%.8s] host[%.8s] srv[%.8s] init[%.25s])",
	login_id,app_name,host_name,server_name,init_string);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

/* If first connection creation. */
if (sp_list_of_conn == NULL) {

	/* Link list initialization for connect. */
	if ( ( sp_list_of_conn = lkls_new_linked_list() ) == NULL ) {
		sprintf(buf,"Link List memory allocation error");
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return NULL;
	}
}

/* Allocate CT10_CONN_T structure. */
if ( (sp_current_active_conn = calloc(1,sizeof(CT10_CONN_T)) ) == NULL ) {
	sprintf(buf,"calloc memory allocation error");
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return NULL;
}


/* Initialize CT10_CONN_T structure. */

sp_current_active_conn->signature = CONN_SIGNATURE;

sf_change_connection_params(
	sp_current_active_conn,
	login_id,
	password,
	app_name,
	host_name,
	server_name);

if (init_string != NULL) {
	strcpy(sp_current_active_conn->init_string, init_string);
}
else {
	strcpy(sp_current_active_conn->init_string, "");
}

sp_current_active_conn->options_mask = options_mask;


/* Add structure to link list. */
if (( rc=lkls_append_elem_at_tail( sp_list_of_conn, 
				sp_current_active_conn) ) != LKLS_OK ) {
	sprintf(buf,"Error adding into link list rc=%d",rc);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return NULL;
}

/* ct10reco_display_trace_values(sp_current_active_conn, "init connection"); */

return sp_current_active_conn;
} /* --------------------------- End of ct10api_create_connection ----------- */



/*
 * Change connection params.
 * Do not support init string and option mask. These 2 must
 * be set with ct10api_create_connection.
 */

int ct10api_change_connection_params(	void *p_ct10_conn,
					char *login_id, 
					char *password,
					char *app_name, 
					char *host_name,
					char *server_name) {
int rc;
CT10_CONN_T *pt;

sprintf(buf, "ct10api_change_connection_params(login[%.8s] app[%.8s] host[%.8s] srv[%.8s])",
	login_id,app_name,host_name,server_name);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}

pt = p_ct10_conn;

if (ct10api_close_and_drop_connection(&(pt->p_conn) ) != CT10_SUCCESS) {
	return CT10_ERR_FATAL_ERROR;
}

sf_change_connection_params(
	pt,
	login_id,
	password,
	app_name,
	host_name,
	server_name);

return CT10_SUCCESS;

} /* --------------------------- End of ct10api_change_connection_params ------------- */



/*
 *************************
 Connect to server 
 *************************/

int ct10api_connect(void *p_ct10_conn)
{
CS_RETCODE rc2;
CT10_CONN_T *pt;
int rc;
CS_BOOL bool=CS_TRUE;
long diag_mask;

sprintf(buf,"ct10api_connect");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}

pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;

if (ct10api_close_and_drop_connection(&(pt->p_conn) ) != CT10_SUCCESS) {
	return CT10_ERR_FATAL_ERROR;
}

/* Memory allocation for the connection structure. */
if ((rc2=ct_con_alloc(context, &(pt->p_conn) )) !=CS_SUCCEED) {
	sprintf(buf,"ct_con_alloc error, rc [%ld]",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

/* Binding connection informations. */
if ((rc2=ct_con_props(pt->p_conn, CS_SET, CS_USERNAME, 
		pt->login_id, CS_NULLTERM, NULL)) != CS_SUCCEED) {
	sprintf(buf,"ct_con_props error, CS_USERNAME, rc [%ld]",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

if ((rc2=ct_con_props(pt->p_conn, CS_SET, CS_PASSWORD,
		pt->password, CS_NULLTERM, NULL)) != CS_SUCCEED ) {
	sprintf(buf,"ct_con_props error, CS_PASSWORD, rc [%ld]",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

if (pt->app_name != NULL) {
if ((rc2=ct_con_props(pt->p_conn, CS_SET, CS_APPNAME,
		pt->app_name, CS_NULLTERM, NULL)) != CS_SUCCEED ) {
	sprintf(buf,"ct_con_props error, CS_APPNAME, rc [%ld]",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}
}

if (pt->host_name != NULL) {
if ((rc2=ct_con_props(pt->p_conn, CS_SET, CS_HOSTNAME,
		pt->host_name, CS_NULLTERM, NULL)) != CS_SUCCEED ) {
	sprintf(buf,"ct_con_props error, CS_HOSTNAME, rc [%ld]",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}
}

if (pt->options_mask & CT10API_BULK_LOGIN) {

if ((rc2=ct_con_props(pt->p_conn, CS_SET, CS_BULK_LOGIN,
		      (CS_VOID *) &bool, CS_UNUSED, NULL)) != CS_SUCCEED ) {

	sprintf(buf,"ct_con_props error, CS_BULK_LOGIN, rc [%ld]",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

}

if (ct10_packet_size != 0) {

if ((rc2=ct_con_props(pt->p_conn, CS_SET, CS_PACKETSIZE, 
		(CS_VOID *) &ct10_packet_size, CS_UNUSED,NULL)) != CS_SUCCEED) {
	sprintf(buf,"Packet size setting error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

}

/* Connecting to server. */
if ((rc2=ct_connect(pt->p_conn, pt->server_name, 
		CS_NULLTERM)) != CS_SUCCEED ) {

	sprintf(buf,"ct_connect error, rc [%ld], mask: %x %x",
			rc2,
			ct10api_get_diagnostic_mask(pt),
			ct10api_get_diag_sev_mask(pt));
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	diag_mask=ct10api_get_diagnostic_mask(pt);
	if (diag_mask & CT10_DIAG_CANNOT_FIND_HOST) {
		return CT10_ERR_FAILED;
	}
	else if (diag_mask & CT10_DIAG_LOGIN_REJECTED) {
		return CT10_ERR_DO_NOT_RETRY;
	}
	else if (diag_mask & CT10_DIAG_NO_SUCH_SERVER) {
		return CT10_ERR_DO_NOT_RETRY;
	}
	else {
		return CT10_ERR_FATAL_ERROR;
	}

}


/* Executing initialization string. */
if ((pt->init_string != NULL) && (strcmp(pt->init_string,"") != 0)) {
	if ((rc = ct10api_exec_lang(pt,pt->init_string)) != CT10_SUCCESS) {
		sprintf(buf,"Error exec init string, rc [%d]",rc);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}
}

return CT10_SUCCESS;
} /* --------------------------- End of ct10api_connect --------------------- */



/*
 ***********************************
 Get owner of a connection.
 ***********************************/

void *ct10api_get_owner_of_conn(CS_CONNECTION   *p_conn)
{
if (p_conn == NULL) {
	return sp_current_active_conn;
}

return ( lkls_find_elem_using_function( sp_list_of_conn,
					p_conn,
					&ct10api_test_p_conn ) );

} /* --------------------------- End of ct10api_get_owner_of_conn --------- */



/*
 ************************
 Get result state mask.
 ************************/

long ct10api_get_result_state_mask(void *p_ct10_conn)
{
CT10_CONN_T *pt;
int rc;

sprintf(buf,"ct10api_get_result_state_mask");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return rc;
}

pt = p_ct10_conn;

return pt->result_state_mask;

} /* --------------------------- End of ct10api_get_diagnostic_mask --------- */


/*
 ************************
 Get diagnostic mask.
 ************************/

long ct10api_get_diagnostic_mask(void *p_ct10_conn)
{
CT10_CONN_T *pt;
int rc;

sprintf(buf,"ct10api_get_diagnostic_mask");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return rc;
}

pt = p_ct10_conn;

return pt->diagnostic_mask;

} /* --------------------------- End of ct10api_get_diagnostic_mask --------- */



/*
 ********************************
 Get severity mask.
 ********************************/

long ct10api_get_diag_sev_mask(void *p_ct10_conn)
{
CT10_CONN_T *pt;
int rc;

sprintf(buf,"ct10api_get_diag_sev_mask");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return rc;
}

pt = p_ct10_conn;

return pt->diag_sev_mask;

} /* --------------------------- End of ct10api_get_diag_sev_mask ----------- */



/*
 ********************************
 Get action mask.
 ********************************/

long ct10api_get_action_mask(void *p_ct10_conn)
{
CT10_CONN_T *pt;
int rc;

sprintf(buf,"ct10api_get_action_mask");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return rc;
}

pt = p_ct10_conn;

return pt->action_mask;

} /* --------------------------- End of ct10api_get_diag_sev_mask ----------- */



char *ct10api_get_server_name_of_conn(void *p_conn)
/*
 * Return (p_conn->server_name)
 * or NULL if p_conn is not a valid CT10_CONN_T struct.
 */
{
	CT10_CONN_T	*pt = NULL;


	sprintf(buf,"ct10api_get_server_name_of_conn");
	ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

	if (ct10api_check_ct10_conn_signature(p_conn) == INVALID_SIGNATURE) {
		return (NULL);
	}

	pt = p_conn;

	return ( &(pt->server_name[0]) );

} /* end of ct10api_get_server_name_of_conn() */




/*
 **************************************
 "Application level" info to log.
 **************************************/

int ct10api_assign_get_appl_context_info_function(void *p_ct10_conn,
				char *(* pf_get_appl_context_info)() )
{
CT10_CONN_T *pt;
int rc;

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}

pt = p_ct10_conn;

pt->pf_get_appl_context_info = pf_get_appl_context_info;

return CT10_SUCCESS;

} /* ---------------- End of ct10api_assign_get_appl_context_info_function -- */



/*
 ******************************************
 Assign result processing function pointer.
 ******************************************/

int ct10api_assign_process_row_function(void *p_ct10_conn,
	int (* pf_process_row_or_params)(void *p_ct10_conn, int b_is_row,
				     CT10_DATA_XDEF_T  *data_array[],  
				     int nb_data,
				     int result_set_no,
				     CS_RETCODE result_type,
				     int	header_ind)  )
{
CT10_CONN_T *pt;
int rc;

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}

pt = p_ct10_conn;

pt->pf_process_row_or_params = pf_process_row_or_params;

return CT10_SUCCESS;

} /* ---------------- End of ct10api_assign_get_appl_context_info_function -- */



/*
 *****************************
 Check connection signature.
 *****************************/

int ct10api_check_ct10_conn_signature(void *p_ct10_conn)
{
CT10_CONN_T *pt;

if (p_ct10_conn == NULL) {
	sprintf(buf,"Invalid signature");
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return INVALID_SIGNATURE;
}

pt = p_ct10_conn;

if (pt->signature != CONN_SIGNATURE) {
	sprintf(buf,"Invalid signature");
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return INVALID_SIGNATURE;
}

return VALID_SIGNATURE;

} /* --------------------------- End of ct10api_check_ct10_conn_signature --- */



/*
 **********************************************
 RPC exec (String format). 
 **********************************************/

int ct10api_exec_rpc_string(void *p_ct10_conn,  char *rpc_string,
			    CS_INT *p_return_status)
{
int rc;
CT10_DATA_XDEF_T  *params_array[MAX_NB_PARM];
char rpc_name[MAX_STRING_SIZE];
int nb_params;
CT10_CONN_T *pt;



sprintf(buf,"ct10api_exec_rpc_string");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);


if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;

/* Parsing the string fromat RPC. */
rc = ct10rpc_parse(context, rpc_string, rpc_name, MAX_STRING_SIZE, 
	params_array, MAX_NB_PARM, &nb_params);

if  ((DEBUG_TRACE_LEVEL_FULL <= debug_trace_flag) && (rc == RPC_SYNTAX_OK)) {
	ct10rpc_dump(rpc_name, params_array, nb_params);
}

/* If valid, execute the RPC. */
if (rc == RPC_SYNTAX_OK) {
	rc = ct10api_exec_rpc(	p_ct10_conn,
				rpc_name,
	     			params_array,
				nb_params,
	  			p_return_status);

	ct10rpc_free(rpc_name, params_array, &nb_params);

	return rc;
}
else {
        sprintf(buf,"RPC syntax error, rc [%d]",rc);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_RPC_PARSING_ERROR;
}

} /* ----------------------- End of ct10api_exec_rpc_string ----------------- */



/*
 ******************************************************************
 This function allocate space and save CT10_CONN_T structure.
 ******************************************************************/

int ct10api_save_recov_info(CT10_CONN_T **conn, CT10_CONN_T **conn_bak)
{

/* Allocate tmp CT10_CONN_T structure. */
if ( (*conn_bak = calloc(1,sizeof(CT10_CONN_T)) ) == NULL ) {
	sprintf(buf,"calloc memory allocation error");
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

(*conn_bak)->retry_ctr            = (*conn)->retry_ctr;
(*conn_bak)->ctr_of_fatal_errors  = (*conn)->ctr_of_fatal_errors;
(*conn_bak)->diagnostic_mask      = (*conn)->diagnostic_mask;
(*conn_bak)->diag_sev_mask        = (*conn)->diag_sev_mask;
(*conn_bak)->action_mask          = (*conn)->action_mask;
(*conn_bak)->result_state_mask    = (*conn)->result_state_mask;
(*conn_bak)->ret_err_code         = (*conn)->ret_err_code;

return CT10_SUCCESS;

}



/*
 ******************************************************************
 This function retore a CT10_CONN_T structure and free memory. 
 ******************************************************************/

int ct10api_restore_recov_info(CT10_CONN_T **conn, CT10_CONN_T **conn_bak)
{

(*conn)->retry_ctr            = (*conn_bak)->retry_ctr;
(*conn)->ctr_of_fatal_errors  = (*conn_bak)->ctr_of_fatal_errors;
(*conn)->diagnostic_mask      = (*conn_bak)->diagnostic_mask;
(*conn)->diag_sev_mask        = (*conn_bak)->diag_sev_mask;
(*conn)->action_mask          = (*conn_bak)->action_mask;
(*conn)->result_state_mask    = (*conn_bak)->result_state_mask;
(*conn)->ret_err_code         = (*conn_bak)->ret_err_code;

/* Free memory. */
free(*conn_bak);

return CT10_SUCCESS;

}



/*
 *****************************************************
 Check and reset connection before any send to the
 server.
 *****************************************************/

ct10api_check_and_reset_connection(CT10_CONN_T *conn)
{
int rc=0;
CT10_CONN_T	*conn_bak=NULL;

ct10api_log_ct10_conn(rc, conn, "Dans check");

/* If not connected or ct10reco tell us to reset connection or. */
/* if the connection is marked dead,                            */
/* we must close and drop the connection and reconnect.         */

if ( (!(ct10api_get_conn_status(conn->p_conn) & CT10_CONSTAT_CONNECTED )) ||
       (ct10api_get_conn_status(conn->p_conn) & CT10_CONSTAT_DEAD ) ||
       (conn->action_mask & RECOV_ACT_RESET_CONNECTION) ) {


	/*****************************************************************
	 This part was move into ct10api_connect.

	if ((rc = ct10api_close_and_drop_connection(&(conn->p_conn))) 
		!= CT10_SUCCESS) {
        	sprintf(buf,"close_and_drop_connection error");
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}
	 *****************************************************************/

	if (ct10api_save_recov_info(&conn, &conn_bak) != CT10_SUCCESS) {
        	sprintf(buf,"Impossible to save recovery info");
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}

	if ((rc = ct10api_connect(conn)) != CT10_SUCCESS) {
        	sprintf(buf,"connect error");
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}

	if (ct10api_restore_recov_info(&conn, &conn_bak) != CT10_SUCCESS) {
        	sprintf(buf,"Impossible to restore recovery info");
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}

	return CT10_SUCCESS;
}
else {
	return CT10_NOTHING_TO_DO;
}
} /* ............... End of ct10api_check_and_reset_connection ............ */



/*
 ***********************************************
 Initializing recovery information.
 ***********************************************/

void ct10api_init_recov_info(CT10_CONN_T *pt)
{

	pt->retry_ctr = 0;
	pt->action_mask = 0;
	pt->result_state_mask = CT10_RESULT_NOT_PRESENT;
	pt->diagnostic_mask = 0;
	pt->diag_sev_mask = 0;
	pt->ret_err_code = 0;
	pt->return_status = -999;

} /* ...................... End of ct10api_init_recov_info ............... */



/*
 **********************************************
 RPC exec (pre-formatted param).
 **********************************************/

int ct10api_exec_rpc(	void *p_ct10_conn,
			char *rpc_name,
	     		CT10_DATA_XDEF_T  *params_array[],
			int nb_params,
	  		CS_INT *p_return_status)
{
CT10_CONN_T *pt;
int rc, i, retry=1;
CS_RETCODE rc2;

sprintf(buf,"ct10api_exec_rpc(rpc[%.20s])", rpc_name);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;

/* Initializing recovery informations. */
ct10api_init_recov_info(pt);


while (retry==1) {

ct10api_log_ct10_conn(rc, pt, "debut boucle");

/******************************************************************
   Check status of connection and reset it if needed.
   RC: 
	CT10_ERR_FATAL_ERROR	Impossible to reset the connection.
	CT10_SUCCESS		Connection was reset successfully.
	CT10_NOTHING_TO_DO	Connection did not have to be reset.

 ******************************************************************/

if ((rc = ct10api_check_and_reset_connection(pt)) == CT10_ERR_FATAL_ERROR) {
        sprintf(buf,"check_and_reset_connection error");
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	*p_return_status = pt->return_status;
	return rc;	
}


ct10api_log_ct10_conn(rc, pt, "Apres check");

/******************************************************************
   Resetting action_mask and ret_err_code.

   We can't do this at the end of the loop because 
   ct10api_check_and_reset_connection need the action_mask of the
   previous execution of the loop.
 ******************************************************************/

pt->action_mask = 0;
pt->ret_err_code = 0;

/*******************************************************************
 If it's the fisrt try or if we had to reset the connection we need
 to create the command.
 *******************************************************************/

if ((pt->retry_ctr == 0) || (rc == CT10_SUCCESS)) {

	if ((rc2=ct_cmd_alloc(pt->p_conn, &(pt->p_cmd))) != CS_SUCCEED) {
        	sprintf(buf,"ct_cmd_alloc error, rc [%ld]",rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		*p_return_status = pt->return_status;
		return CT10_ERR_FATAL_ERROR;
	}

	if ((rc2=ct_command(pt->p_cmd, CS_RPC_CMD, rpc_name, CS_NULLTERM, 
			    CS_UNUSED)) != CS_SUCCEED) {
        	sprintf(buf,"ct_command error, rc [%ld]",rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		*p_return_status = pt->return_status;
		return CT10_ERR_FATAL_ERROR;
	}

	/* Bind all parameters. */
	for (i=0;i<nb_params;i++)
	{
	
	if (params_array[i] != NULL) {
		if ((rc2=ct_param(	pt->p_cmd, 
					&((params_array[i])->fmt), 
					(params_array[i])->p_val,
			  		(params_array[i])->len, 
					(params_array[i])->null_ind)) 
					!= CS_SUCCEED) {
        		sprintf(buf,"ct_param error. rc [%ld]", rc2);
			ct10api_apl_log(ALOG_ERROR, 0, buf, 
				DEBUG_TRACE_LEVEL_NONE);
			*p_return_status = pt->return_status;
			return CT10_ERR_FATAL_ERROR;
		}
	}

	}

} /* if (pt->retry_ctr == 0) || (rc == CT10_SUCCESS) */

/* Before executing the RPC we set the rowcount limit */
if (s_row_count != -1 ) {

	if ((rc = ct_options(pt->p_conn, CS_SET, CS_OPT_ROWCOUNT, &s_row_count, CS_UNUSED, NULL)) != CS_SUCCEED) {
		sprintf(buf,"ct10api_exec_rpc: ct_option (for set rowcount) error rc [%d]", rc);
		ct10api_apl_log(ALOG_WARN, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		*p_return_status = pt->return_status;
		return ct10api_ct10fail_return_value(pt);
	}
}

/* Executing the RPC. */
if ((rc2=ct_send(pt->p_cmd)) != CS_SUCCEED) {
        sprintf(buf,"ct_send error, rc [%ld]",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	*p_return_status = pt->return_status;
	return ct10api_ct10fail_return_value(pt);
}

ct10api_log_ct10_conn(rc, pt, "Apres ct_send");

/* Results processing. */
rc=ct10api_process_results(p_ct10_conn, pt->p_cmd);

ct10api_log_ct10_conn(rc, pt, "Apres result");

/* if and error as occur, and we can retry, and no result as arrive yet */
if ( (rc != CT10_SUCCESS) && 
     (!(pt->action_mask & RECOV_ACT_DO_NOT_RETRY)) &&
     (pt->result_state_mask & CT10_RESULT_NOT_PRESENT)) {

	sleep(pt->retry_interval);
	++pt->retry_ctr;
}
else {
	retry = 0;
}

/* If no retry to do or if we must reset connection. */
if ((retry == 0) || (pt->action_mask & RECOV_ACT_RESET_CONNECTION) ) {

	ct10api_log_ct10_conn(rc, pt, "avant cmd drop");

	if ((rc2 = ct_cmd_drop(pt->p_cmd)) != CS_SUCCEED) {
		pt->p_cmd = NULL;
		sprintf(buf,"ct_cmd_drop error, rc '%ld'",rc2);
		ct10api_apl_log(ALOG_WARN, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		/* Even if the ct_cmd_drop fail we don't want to stop. */
	}

	ct10api_log_ct10_conn(rc, pt, "apres cmd drop");

}


} /* While... */

*p_return_status = pt->return_status;

return rc;

} /* ---------------- End of ct10api_exec_rpc ------------------------------- */



/*
 *******************************************
 Trace for CT10_CONN_T.
 *******************************************/

void ct10api_log_ct10_conn(int rc,CT10_CONN_T *pt,char *posi)
{

if  (DEBUG_TRACE_LEVEL_FULL <= debug_trace_flag) {
printf("%s rc [%d], action_mask [%#x], diag_mask [%#x], result status [%#x] rc_code [%d]\n",
		posi,
		rc,
		pt->action_mask,
		pt->diagnostic_mask,
		pt->result_state_mask,
		pt->ret_err_code
		);
}
} /* .................... End of ct10api_log_ct10_conn ..................... */



/*
 *******************************
 Results processing.
 *******************************/

int ct10api_process_results(CT10_CONN_T *conn, CS_COMMAND *cmd)
{
CS_RETCODE rc2, res_type,last_row_type,ct_result_rc;
CS_INT outl,nbr_rows;
int rc,rc3;
int b_cancel_done=0;
int error_code = 0;

conn->result_state_mask = CT10_RESULT_NOT_PRESENT;
rc = CT10_SUCCESS;

while (error_code == 0)
{

	ct_result_rc = ct_results(cmd, &res_type);

	/* If there is a problem with ct_results and */
	/* recovery module tell us to flush results  */
	/* and the cancel connection is not done.    */
	if (ct_result_rc != CS_SUCCEED) {

		if ((conn->action_mask & RECOV_ACT_FLUSH_RESULTS) &&
		   (!b_cancel_done) ) {

			ct10api_cancel_conn_all(cmd,conn->p_conn);
			b_cancel_done = 1;
		}
		break;
	}

	/* Process any result type. */
	switch (res_type)
	{
	case CS_ROW_RESULT:

		conn->result_state_mask = CT10_RESULT_PRESENT;
		rc=ct10api_process_1_result_set(conn,cmd,CS_ROW_RESULT);
		last_row_type = CS_ROW_RESULT;
		break;

	case CS_PARAM_RESULT:

		conn->result_state_mask = CT10_RESULT_PRESENT;
		rc=ct10api_process_1_result_set(conn,cmd,CS_PARAM_RESULT);
		last_row_type = CS_PARAM_RESULT;
		break;

	case CS_STATUS_RESULT:

		/* conn->result_state_mask = CT10_RESULT_PRESENT; */
		rc=ct10api_process_1_result_set(conn,cmd,CS_STATUS_RESULT);
		last_row_type = CS_STATUS_RESULT;
		break;

	case CS_CMD_SUCCEED:
		break;

	case CS_CMD_FAIL:
		break;

	case CS_CMD_DONE:

		/* If it's not the end of a status result.  */
		/* We need to get the number of rows and    */
		/* pass it to the results handler.          */

		if (last_row_type != CS_STATUS_RESULT) {

			if ((rc2=ct_res_info(cmd,CS_ROW_COUNT,&nbr_rows,
				sizeof(nbr_rows), &outl)) != CS_SUCCEED) {
				sprintf(buf,"ct_res_info error, rc '%ld'",rc2);
				ct10api_apl_log(ALOG_ERROR, 0, buf, 
						DEBUG_TRACE_LEVEL_NONE);
				return CT10_ERR_FATAL_ERROR;
			}

			/* If we are in flus results mode we don't call */
			/* the results handler.				*/

			/* If not in flush result mode or if in flush result */
			/* mode but for output parameters.                   */

			if ( (!(conn->action_mask & RECOV_ACT_FLUSH_RESULTS)) ||
	     		( (conn->action_mask & RECOV_ACT_FLUSH_RESULTS) &&
	       		(last_row_type == CS_PARAM_RESULT) ) ) { 

			/* We call the default result handler or the    */
			/* user define result handler.			*/

			if ( conn->pf_process_row_or_params == NULL) {

				rc3 =  ct10api_process_row_or_params (
					conn,
					0,
					NULL,
					(int) NULL,
					nbr_rows,
					(long) NULL,
					CT10API_HEADER_IND_NB_ROW);
			}
			else {
				rc3 = (conn->pf_process_row_or_params)(
					conn,
					0,
					NULL,
					(int) NULL,
					nbr_rows,
					(long) NULL,
					CT10API_HEADER_IND_NB_ROW);
			}
			}
		}
		break;

	default:
		error_code = CT10_ERR_FATAL_ERROR;
		rc = CT10_ERR_FATAL_ERROR;
		sprintf(buf,"Error ct_result, result type '%ld'",res_type);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		break;
	}

	/* Fatal error. */
	if (rc != CT10_SUCCESS) {
		sprintf(buf,"Error, result not flush");
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		/* rc = CT10_ERR_FATAL_ERROR; */
		break;
	}


	/***************

	Because we want to return the output parameter event if
	there is a flush result active we must not cancel the 
	connection here but only at the top of the loop if
	ct_result fail.


	if ((conn->action_mask & RECOV_ACT_FLUSH_RESULTS) &&
    	(!b_cancel_done) ) {
		ct10api_cancel_conn_all(cmd,conn->p_conn);
		b_cancel_done = 1;
	}

	****************/


} /* While */


switch (ct_result_rc)
{
	case CS_END_RESULTS:
		break;
	case CS_FAIL:
		rc = CT10_ERR_FAILED;
		break;
	default:
	/* For a deadlock this part will be executed. */
		sprintf(buf,"Error ct_result, rc '%ld'",ct_result_rc);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		rc =  CT10_ERR_FAILED;
		break;
}


if ( rc > conn->ret_err_code) {
	rc = conn->ret_err_code;
}

return rc;
} /* ---------------- End of ct10api_process_results ------------------------ */



/*
 *******************************************************************
 Process a results set.

 NOTE: For output parameters of type char or varchar, fmt.maxlength 
       is always egal to 255. The print of char/varchar output 
       parameters is always of length 255.
 *******************************************************************/

int ct10api_process_1_result_set(	CT10_CONN_T	*conn,
					CS_COMMAND	*cmd,
					CS_RETCODE	row_type)
{
CT10_DATA_XDEF_T 	*data_array[MAX_NB_PARM];
CS_INT 			nbr_columns,count,
			outl;
CS_RETCODE 		rc2;
int 			i;
int 			rc3;
CS_INT			result_set_no;



	/* Get number of columns. */
	if ((rc2=ct_res_info(cmd,CS_NUMDATA,&nbr_columns,
		sizeof(nbr_columns),&outl) ) != CS_SUCCEED)
	{
		sprintf(buf,"ct_res_info CS_NUMDATA error, rc '%d'",rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}

	/* Get result set number */
	if ((rc2=ct_res_info(cmd,CS_CMD_NUMBER,&result_set_no,
		sizeof(result_set_no),&outl) ) != CS_SUCCEED)
	{
		sprintf(buf,"ct_res_info CS_CMD_NUMBER error, rc '%d'",rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}


	/* Allocate space get info and bind all columns. */
	for (i=0; i<nbr_columns; ++i) {


	/* Allocate CT10_DATA_XDEF_T structure. */
	if ( ( data_array[i] = calloc(1,sizeof(CT10_DATA_XDEF_T)) ) == NULL ) {
		sprintf(buf,"calloc memory allocation error");
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}

	/* Get CS_DATAFMT into. */
	ct_describe(cmd, i+1, &(data_array[i]->fmt));

	/* Add 1 to len if char. To make room for '\0' */
	if ((data_array[i])->fmt.datatype == CS_CHAR_TYPE) {
		++(data_array[i]->fmt.maxlength);
		(data_array[i])->fmt.format = CS_FMT_NULLTERM;
		(data_array[i])->fmt.locale = NULL;
	}

	/* Allocate data value space. */
	if ( ( data_array[i]->p_val = calloc(1,data_array[i]->fmt.maxlength) ) 
			== NULL ) {
		sprintf(buf,"calloc memory allocation error");
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}

	/* Bind column result to memory buffer. */
	if ((rc2=ct_bind(cmd, i+1, &(data_array[i]->fmt), 
	     data_array[i]->p_val, &(data_array[i]->len),
	     &(data_array[i]->null_ind))) != CS_SUCCEED) {

		sprintf(buf,"ct_bind error, rc '%ld'", rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;

	}

	sprintf(buf,"Result[%1ld] [%2d] na[%s] [%2ld] ty[%ld] len[%3ld] st[%x]",
		result_set_no,
		i,
		data_array[i]->fmt.name,
		data_array[i]->fmt.namelen,
		data_array[i]->fmt.datatype,
		data_array[i]->fmt.maxlength,
		data_array[i]->fmt.status
		);
	ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_FULL);

	}

	/* Call results handling with header_ind set to 1. */
	if (row_type != CS_STATUS_RESULT) {

	/* If not in flush result mode or if in flush result mode */
	/* but for output parameters.                             */
	if ( (!(conn->action_mask & RECOV_ACT_FLUSH_RESULTS)) ||
	     ( (conn->action_mask & RECOV_ACT_FLUSH_RESULTS) &&
	       (row_type == CS_PARAM_RESULT) ) ) { 

	/* Check if user result handler exist */
	if ( conn->pf_process_row_or_params == NULL) {
		rc3 =  ct10api_process_row_or_params (
			conn,
			0,
			data_array,
			nbr_columns,
			result_set_no,
			row_type,
			CT10API_HEADER_IND_TRUE);
	}
	else {
		rc3 = (conn->pf_process_row_or_params)(
			conn,
			0,
			data_array,
			nbr_columns,
			result_set_no,
			row_type,
			CT10API_HEADER_IND_TRUE);
	}
	}
	}

	/* Fetch all data rows of the current result set. */
	while (( (rc2 = ct_fetch(cmd, CS_UNUSED, CS_UNUSED,
		CS_UNUSED, &count)) == CS_SUCCEED) || 
		(rc2 == CS_ROW_FAIL))
	{
		if (rc2 == CS_ROW_FAIL) {
			sprintf(buf,"ct_fetch error, rc '%d'",rc2);
			ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
			return ct10api_ct10fail_return_value(conn);
		}

		switch (row_type) {
		case CS_ROW_RESULT:
		case CS_PARAM_RESULT:
			/* If not in flush result mode or if in flush result */
			/* mode but for output parameters.                    */

			if ( (!(conn->action_mask & RECOV_ACT_FLUSH_RESULTS)) ||
	     		   ( (conn->action_mask & RECOV_ACT_FLUSH_RESULTS) &&
	       		     (row_type == CS_PARAM_RESULT) ) ) { 

			if ( conn->pf_process_row_or_params == NULL) {
				rc3 =  ct10api_process_row_or_params (
					conn,
					0,
					data_array,
					nbr_columns,
					result_set_no,
					row_type,
					CT10API_HEADER_IND_FALSE);

				if (rc3 != CT10_SUCCESS) {
				}
			}
			else {
				rc3 = (conn->pf_process_row_or_params)(
					conn,
					0,
					data_array,
					nbr_columns,
					result_set_no,
					row_type,
					CT10API_HEADER_IND_FALSE);

				if (rc3 != CT10_SUCCESS) {
				}
			}
			}
			break;
		case CS_STATUS_RESULT:
			conn->return_status=*((CS_INT *)(data_array[0]->p_val));
			break;
		}
	}


	/* Free buffer space. */
	for (i=0; i<nbr_columns; ++i) {
		if (data_array[i] != NULL) {
			if (data_array[i]->p_val != NULL) {
				free(data_array[i]->p_val);
				data_array[i]->p_val = NULL;
			}
			free(data_array[i]);
			data_array[i] = NULL;
		}
	}

return CT10_SUCCESS;
} /* ---------------- End of ct10api_process_1_result_set ------------------- */



static int sf_print_string(int width, char *string, int b_align_char_type)
/*
 * Called by:	ct10api_process_row_or_params()
 *
 * Print string[] on stdout on a field of width characters.
 * If b_align_char_type is true, align on left, otherwise of right.
 *
 * Return 0.
 */
{
	printf( (b_align_char_type) ? " %-*.*s" : " %*.*s",
		width,
		width,
		string);

	return (0);

} /* end of sf_print_string() */





/*
 **********************************************************************
 Default results processing.
 **********************************************************************/

int  ct10api_process_row_or_params (	void	 	 *p_ct10_conn, 
					int 		 b_is_row,
					CT10_DATA_XDEF_T *data_array[],
					int		 nb_data,
					int		 result_set_no,
					CS_RETCODE	 result_type,
					int		 header_ind)
{
int i;
CS_DATAFMT fmt_out, fmt_in;
char buf[RPC_MAXSIZE];
CS_INT outl;
int print_len[RPC_MAXSIZE];
char dash[RPC_MAXSIZE];
char plurial[2];
CS_RETCODE rc2;

if (header_ind == CT10API_HEADER_IND_NB_ROW) {

	if (result_set_no != -1) {
		if (result_set_no > 1)
			strcpy(plurial,"s");
		else
			strcpy(plurial,"");

		printf("\n(%d row%s affected)\n",result_set_no,plurial); 
	}

	return 0;
}

if ((header_ind == CT10API_HEADER_IND_TRUE) && 
    (result_type == CS_PARAM_RESULT)) {
	printf("\nReturn parameter:\n\n");
}

memset(dash,'-',RPC_MAXSIZE);

memset(&fmt_out,0,sizeof(CS_DATAFMT));
fmt_out.datatype = CS_CHAR_TYPE;
fmt_out.maxlength = RPC_MAXSIZE;
fmt_out.locale = NULL;
fmt_out.format = CS_FMT_NULLTERM;

for (i=0; i<nb_data; ++i) {

	/* setting output size base on datatype. */
	switch ((data_array[i])->fmt.datatype ) {
		case CS_TINYINT_TYPE:	print_len[i] = 4;
					break;
		case CS_SMALLINT_TYPE:	print_len[i]= 6;
					break;
		case CS_INT_TYPE:	print_len[i] = 11;
					break;
		case CS_DATETIME_TYPE:	print_len[i] = 27;
					break;
		case CS_CHAR_TYPE:	print_len[i] = 
					(data_array[i])->fmt.maxlength;
					break;
		case CS_FLOAT_TYPE:	print_len[i] = 20;
					/*printf("FLOAT [%f]\n", *((CS_FLOAT *) data_array[i]->p_val));*/
					break;
		default:		print_len[i] = 15;
					break;
	}

	/* Adjust size of column to size of header. */
	if ( ((int) strlen((data_array[i])->fmt.name)) > print_len[i]) {
		print_len[i] = strlen((data_array[i])->fmt.name);
	}

	
	/* If not header convert and print value. */
	if (header_ind == CT10API_HEADER_IND_FALSE) {

		/* If not null convert data in char type. */
		if ((data_array[i])->null_ind == 0) {
			/* Ajust actual datalen to be converted. */
			memcpy(&fmt_in, &((data_array[i])->fmt),
				sizeof(CS_DATAFMT));
			fmt_in.maxlength=(data_array[i])->len;

			/* Convert data to char. */
			if ((rc2=cs_convert(context, &fmt_in, 
					(data_array[i])->p_val,
			    		&fmt_out, buf, &outl)) != CS_SUCCEED) {
				
				sprintf(buf,"cs_convert error, rc '%d'",rc2);
				ct10api_apl_log(ALOG_ERROR, 0, buf,
						DEBUG_TRACE_LEVEL_NONE);
				return CT10_ERR_FATAL_ERROR;
			}
		}
		/* Set output buffer to NULL string. */
		else {
			strcpy(buf,"NULL");
		}

		sf_print_string(print_len[i], buf, 
			((data_array[i])->fmt.datatype == CS_CHAR_TYPE) );
	}
	/* Print header information. */
	else {
		sf_print_string(print_len[i], (data_array[i])->fmt.name,
				1);	/* 1==align string */
	}
 }
 printf("\n");

 if (header_ind == CT10API_HEADER_IND_TRUE) {
	for (i=0; i<nb_data; ++i) {
		sf_print_string(print_len[i], dash, 1);	/* 1==align string */
	}
	printf("\n");
 }


return CT10_SUCCESS;
} /* ----------------------- End of ct10api_process_row_or_params ....... */



/*
 ********************************************************
 Test function to find CT10_CONN_T base on CS_CONNECTION.
 ********************************************************/

int ct10api_test_p_conn(void *pt_val, void *pt_struct) {
CT10_CONN_T *pt;
pt=pt_struct;

if ( pt_val == pt->p_conn )  {
	return 1;
}
else {
	return 0;
}

} /*...................... End of ct10api_test_p_conn .................*/



/*
 ********************************************************
 Execution of a language event.
 ********************************************************/

int ct10api_exec_lang(void *p_ct10_conn, char *lang_string)
{
CT10_CONN_T *pt;
int rc;
CS_RETCODE rc2;


sprintf(buf,"ct10api_exec_lang");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;

ct10api_init_recov_info(pt);

if ((rc = ct10api_check_and_reset_connection(pt)) == CT10_ERR_FATAL_ERROR) {
        sprintf(buf,"check_and_reset_connection error");
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return rc;	
}

if ((rc2=ct_cmd_alloc(pt->p_conn, &(pt->p_cmd))) != CS_SUCCEED) {
	sprintf(buf,"ct_cmd_alloc error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

if ((rc2=ct_command(pt->p_cmd, CS_LANG_CMD,lang_string,CS_NULLTERM, CS_UNUSED)) 
		!= CS_SUCCEED) {
	sprintf(buf,"ct_command error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

if ((rc2=ct_send(pt->p_cmd)) != CS_SUCCEED) {
	sprintf(buf,"ct_send error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return ct10api_ct10fail_return_value(pt);
}

rc=ct10api_process_results(p_ct10_conn, pt->p_cmd);

if ((rc2 = ct_cmd_drop(pt->p_cmd)) != CS_SUCCEED) {
	sprintf(buf,"ct_cmd_drop error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_WARN, 0, buf, DEBUG_TRACE_LEVEL_NONE);
}

return rc;
} /*...................... End of ct10api_exec_lang .................*/



/*
 ************************************************
 Change and execute connection init string.
 ************************************************/

int ct10api_change_and_exec_init_string(void *p_ct10_conn, char *init_string)
{
CT10_CONN_T *pt;
int rc;

sprintf(buf,"ct10api_change_init_string");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;


if ( init_string == NULL ) {
	strcpy(pt->init_string, "");
}
else {
	strcpy(pt->init_string, init_string);
}

/* Executing initialization string. */
if ((pt->init_string != NULL) && (strcmp(pt->init_string,"") != 0)) {
	if ((rc = ct10api_exec_lang(pt,pt->init_string)) != CT10_SUCCESS) {
		sprintf(buf,"ct10api_change_init_string: error exec init string, rc [%d]",rc);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
		return CT10_ERR_FATAL_ERROR;
	}
}

return CT10_SUCCESS;

} /* ............. End of ct10api_change_init_string ................. */



/*
 *******************************************
 Closing connection.
 *******************************************/

int ct10api_close_connection(void *p_ct10_conn)
{
CT10_CONN_T *pt;
int rc;

sprintf(buf,"ct10api_close_connection");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;

if ((rc = ct10api_close_and_drop_connection(&(pt->p_conn))) 
	!= CT10_SUCCESS) {
	return rc;
}

if ((rc=lkls_remove_element_from_list(sp_list_of_conn, p_ct10_conn)) 
		!= LKLS_FOUND) {
	return CT10_ERR_FATAL_ERROR;
}

return CT10_SUCCESS;
} /*...................... End of ct10api_close_connection .................*/



/*
 ******************************************
 Close context, cleanup and log stat.
 ******************************************/

int ct10api_log_stats_and_cleanup_everything()
{
CS_RETCODE rc2;

if ((rc2=ct_exit(context,CS_UNUSED)) != CS_SUCCEED) {
	return CT10_ERR_FATAL_ERROR;
}

if ((rc2=cs_ctx_drop(context)) != CS_SUCCEED) {
	return CT10_ERR_FATAL_ERROR;
}

return CT10_SUCCESS;
} /*............... End of ct10api_log_stats_and_cleanup_everything ..........*/




/*
 ***********************************************
 Bulk insert initialization.
 ***********************************************/

int ct10api_bulk_init(	void *p_ct10_conn,
			char *tbl_name, 
			CT10_DATA_XDEF_T  *columns_array[],
			int nb_columns)
{
CT10_CONN_T *pt;
int rc,i;
CS_RETCODE rc2;

sprintf(buf,"ct10api_bulk_init(tbl[%.10s]", tbl_name);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

rc = CT10_SUCCESS;

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;

ct10api_init_recov_info(pt);

if ((rc=sf_bulk_init(p_ct10_conn, tbl_name, FALSE)) != CT10_SUCCESS) {
	return CT10_ERR_FAILED;
}
if ((rc=sf_bulk_bind(p_ct10_conn, tbl_name, columns_array,
		nb_columns, NULL, FALSE)) != CT10_SUCCESS) {
	return CT10_ERR_FAILED;
}

if ( rc > pt->ret_err_code) {
	rc = pt->ret_err_code;
}

return rc;
} /*...................... End of ct10api_bulk_init .......................*/





/*
 ***********************************************
 Bulk insert initialization.

 This functions gets the CS_DATAFMT from the
 database and calls ct10api_bulk_init afterward
 ***********************************************/

int ct10api_bulk_init_set_cs_datafmt(	void *p_ct10_conn,
			char *tbl_name, 
			CT10_DATA_XDEF_T  *columns_array[],
			int nb_columns,
			char *ident_col_name,
			int with_ident_data)
{
CT10_CONN_T *pt;
int rc,i;
CS_RETCODE rc2;

sprintf(buf,"ct10api_bulk_init_set_cs_datafmt(tbl[%.10s]", tbl_name);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

rc = CT10_SUCCESS;

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;

ct10api_init_recov_info(pt);

if ((rc = sf_bulk_init(p_ct10_conn, tbl_name, with_ident_data)) != CT10_SUCCESS) {
	return rc;
}

for(i=0;i<nb_columns;i++) {
	if(columns_array[i] != NULL) {
		if ((rc2=blk_describe(pt->p_blk, i+1, &(columns_array[i]->fmt)))
			!= CS_SUCCEED) {

			sprintf(buf,
			"blk_describe error, column '%d' rc '%ld'",i+1,rc2);

			ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

			return CT10_ERR_FAILED;
		}

		columns_array[i]->fmt.count = 1;
	}
}


rc = sf_bulk_bind(p_ct10_conn, tbl_name, columns_array,
		nb_columns, ident_col_name, with_ident_data);

if ( rc > pt->ret_err_code) {
	rc = pt->ret_err_code;
}

return rc;

} /*...................... End of ct10api_bulk_init_set_cs_datafmt .......................*/




/*
 **********************************************************************
 Sending one bulk row to data server.
 **********************************************************************/

int ct10api_bulk_send_1_row (void *p_ct10_conn)
{
CT10_CONN_T *pt;
int rc;
CS_RETCODE rc2;

rc = CT10_SUCCESS;

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;


if ( rc > pt->ret_err_code) {
	return pt->ret_err_code;
}


if ((rc2=blk_rowxfer(pt->p_blk))!=CS_SUCCEED) {
	sprintf(buf,"blk_rowxfer, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	if (pt->ret_err_code < CT10_ERR_FAILED) {
		return pt->ret_err_code;
	}
	else {
		return CT10_ERR_FAILED;
	}
}

if ( rc > pt->ret_err_code) {
	rc = pt->ret_err_code;
}

return rc;
} /*...................... End of ct10api_bulk_send_1_row .................*/



/*
 *************************************************
 Commit a bulk batch.
 *************************************************/

int ct10api_bulk_commit (void *p_ct10_conn, CS_INT *outrow)
{
CT10_CONN_T *pt;
int rc;
CS_RETCODE rc2;

sprintf(buf,"ct10api_bulk_commit");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

rc = CT10_SUCCESS;

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn; 

if ((rc2=blk_done(pt->p_blk, CS_BLK_BATCH, outrow))!=CS_SUCCEED) {
	sprintf(buf,"blk_done CS_BLK_BATCH error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	rc = ct10api_bulk_close(p_ct10_conn,1);

	if (pt->ret_err_code < CT10_ERR_FAILED) {
		return pt->ret_err_code;
	}
	else {
		return CT10_ERR_FAILED;
	}

}

if ( rc > pt->ret_err_code) {
	rc = ct10api_bulk_close(p_ct10_conn,1);

	if (pt->ret_err_code < CT10_ERR_FAILED) {
		return pt->ret_err_code;
	}
	else {
		return CT10_ERR_FAILED;
	}
}

return CT10_SUCCESS;
} /*...................... End of ct10api_bulk_commit .................*/



/*
 ************************************************
 Closing a bulk copy operation or aborting.
 ************************************************/

int ct10api_bulk_close (void *p_ct10_conn, int cancel_indicator) 
{
CT10_CONN_T *pt;
int rc;
CS_RETCODE rc2;
CS_INT outrow;

sprintf(buf,"ct10api_bulk_close(cxlind[%d])", cancel_indicator);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

rc = CT10_SUCCESS;

if ((rc=ct10api_check_ct10_conn_signature(p_ct10_conn)) == INVALID_SIGNATURE) {
	return CT10_ERR_BAD_CONN_HANDLE;
}
pt = p_ct10_conn;
sp_current_active_conn = p_ct10_conn;

if (cancel_indicator == 0) {

	if ((rc2=blk_done(pt->p_blk, CS_BLK_ALL, &outrow))!=CS_SUCCEED) {

	sprintf(buf,"blk_done CS_BLK_ALL error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	if ((rc2=blk_done(pt->p_blk, CS_BLK_CANCEL, &outrow))!=CS_SUCCEED) {
		sprintf(buf,"blk_done CS_BLK_CANCEL error, rc '%ld'",rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

		if (pt->ret_err_code < CT10_ERR_FAILED) {
			return pt->ret_err_code;
		}
		else {
			return CT10_ERR_FAILED;
		}
	}

	}

}
else {

	if ((rc2=blk_done(pt->p_blk, CS_BLK_CANCEL, &outrow))!=CS_SUCCEED) {
		sprintf(buf,"blk_done CS_BLK_CANCEL error, rc '%ld'",rc2);
		ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

		if (pt->ret_err_code < CT10_ERR_FAILED) {
			return pt->ret_err_code;
		}
		else {
			return CT10_ERR_FAILED;
		}
	}

}


if ((rc2=blk_drop(pt->p_blk))!=CS_SUCCEED) {
	sprintf(buf,"blk_drop error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);

	if (pt->ret_err_code < CT10_ERR_FAILED) {
		return pt->ret_err_code;
	}
	else {
		return CT10_ERR_FAILED;
	}
}

if ( rc > pt->ret_err_code) {
	rc = pt->ret_err_code;
}

return rc;
} /*...................... End of ct10api_bulk_close ..................*/



/*
 ********************************************
 Like ct_close(), but with debug trace.
 ********************************************/

static CS_RETCODE ct10api_ct_close(CS_CONNECTION *p_conn, CS_INT opt)
{
	/* The windows NT library does not like multiple ct_close() in
	 * a row.  It behaves normally if some times is given to the
	 * library.
	 */
#ifdef WIN32
	Sleep( 0 );
#endif
								 
	return ( ct_close(p_conn, opt) );

#ifdef WIN32
	Sleep( 0 );
#endif

} /* ........................ End of ct10api_ct_close ................... */



/*
 **************************************************
 Return the connection status.
 **************************************************/

int ct10api_get_conn_status(CS_CONNECTION *p_conn)
{
	CS_INT		a_status;
	CS_RETCODE	rc;
	int		bit_mask_val = 0;

	if (p_conn == NULL) {
		return (CT10_CONSTAT_NULL_PTR);
	}

	rc = ct_con_props(p_conn, CS_GET, CS_CON_STATUS, 
			 &a_status, CS_UNUSED, (CS_INT *) NULL);

	if (rc != CS_SUCCEED) {
		return (CT10_CONSTAT_UNKNOWN);
	}
	if (a_status & CS_CONSTAT_CONNECTED) {
		bit_mask_val |= CT10_CONSTAT_CONNECTED;
	}
	if (a_status & CS_CONSTAT_DEAD) {
		bit_mask_val |= CT10_CONSTAT_DEAD;
	}
	return (bit_mask_val);

} /* .................... End of ct10api_get_conn_status ............... */



/*
 ***************************************************
 Dropping a connection.
 ***************************************************/

int ct10api_drop_connection(CS_CONNECTION **p_conn)
{

	if ( (p_conn != NULL) && (ct_con_drop(*p_conn) ) != CS_SUCCEED) {
		return (CT10_ERR_FAILED);
	}
	*p_conn=NULL;
	return (CT10_SUCCESS);

} /* ...................... End of ct10api_drop_connection ............. */



/*
 *******************************************************************
 Close a connection (cancels results if need be).
 
 WARNING: This routine assumes that there is no cursor active.
 *******************************************************************/

int ct10api_close_connection2(CS_CONNECTION *p_conn, CS_INT close_mode)
{
 int	b_was_connected;

 b_was_connected = (ct10api_get_conn_status(p_conn) & CT10_CONSTAT_CONNECTED);

 /* First try of ct_close():
  */
 if (ct10api_ct_close(p_conn, close_mode) != CS_SUCCEED) {
	if (b_was_connected
	 && !(ct10api_get_conn_status(p_conn) & CT10_CONSTAT_CONNECTED) ) {
		return (CT10_SUCCESS);
	}
	if (ct10api_get_conn_status(p_conn) & CT10_CONSTAT_DEAD) {
		ct10api_ct_close(p_conn, CS_FORCE_CLOSE);
		return (CT10_SUCCESS);
	}
	ct_cancel(p_conn, (CS_COMMAND *) NULL, CS_CANCEL_ALL);

	/* Second try of ct_close():
	 */
	if (ct10api_ct_close(p_conn, close_mode) != CS_SUCCEED) {
		if (b_was_connected
		 && !(ct10api_get_conn_status(p_conn)
		      & CT10_CONSTAT_CONNECTED) ) {
			return (CT10_SUCCESS);
		}
		if (ct10api_get_conn_status(p_conn) & CT10_CONSTAT_DEAD) {
			ct10api_ct_close(p_conn, CS_FORCE_CLOSE);
			return (CT10_SUCCESS);
		}

		return (CT10_ERR_FAILED);
	}
 }
 return (CT10_SUCCESS);

} /* ................... End of ct10api_close_connection ................. */


/*
 *****************************************************************
 Close a connection (cancels results if need be),
 then Drop it.

 WARNING: This routine assumes that there is no cursor active.
 *****************************************************************/

int ct10api_close_and_drop_connection(CS_CONNECTION **p_conn)
{
	int	rc;

	sprintf(buf,"ct10api_close_and_drop_connection");
	ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

	rc = ct10api_get_conn_status(*p_conn);

	if (rc & CT10_CONSTAT_NULL_PTR) {
		return CT10_SUCCESS;
	}

	if (ct10api_get_conn_status(*p_conn) & CT10_CONSTAT_DEAD) {
		rc = ct10api_close_connection2(*p_conn, CS_FORCE_CLOSE);
	} else {
		rc = ct10api_close_connection2(*p_conn, CS_UNUSED);
	}
	if (rc != CT10_SUCCESS) {
		ct10api_close_connection2(*p_conn, CS_FORCE_CLOSE);
	}

	return (ct10api_drop_connection(p_conn) );

} /* end of ct10api_close_and_drop_connection() */



/*
 *****************************************************************
 Cancel everything on a connection.
 *****************************************************************/

int ct10api_cancel_conn_all(CS_COMMAND *p_cmd, CS_CONNECTION *p_conn)
{
	CS_INT	cs_rc = 0;
	int	b_conn_is_dead = 0;

	b_conn_is_dead = ct10api_get_conn_status(p_conn) & CT10_CONSTAT_DEAD;

	if (b_conn_is_dead) {
		return (1);	/* cannot do ct_cancel() */
	}
	
	cs_rc = ct_cancel(p_conn, (CS_COMMAND *) NULL, CS_CANCEL_ALL);

	if (cs_rc != CS_SUCCEED) {

	    return (-1);
	}
	return (0);

} /* end of ct10api_cancel_conn_all() */



/*
 ******************************************************
 This is require to cancel a connection inside 
 the result processing loop. The client program
 need a call with p_ct10_conn not 
 with p_ct10_conn->p_conn.
 ******************************************************/
int ct10api_cancel_con(void *p_ct10_conn)
{
CT10_CONN_T *pt;
pt = p_ct10_conn;

return (ct10api_cancel_conn_all(pt->p_cmd, pt->p_conn) );

} /* End of ct10api_cancel_con */



void ct10api_set_log_mode_quiet(int new_val)
/*
 * If ct10api_set_log_mode_quiet(1) is called, then ct10api_apl_log()
 * will stop writing to the application log (through apl_log1.c).
 */
{
	s_log_mode_quiet = new_val;
}



CS_CONTEXT *ct10api_get_context(void)
/*
 * Get pointer to context (a must if the client want to convert dates)
 */
{
	return ( context );
}



int ct10api_change_exec_timeout_value(CS_INT exec_timeout)
/*
 * Change rpc exec timeout. Il the connection is drop and
 * recreate new timeout value will be used.
 */
{
CS_RETCODE rc2;

sprintf(buf,"ct10api_change_exec_timeout_value new exec timeout[%ld]",
		exec_timeout);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if ((rc2=ct_config(	context, CS_SET,CS_TIMEOUT,
			(CS_VOID *) &exec_timeout,
			CS_UNUSED, NULL)) != CS_SUCCEED) {
	sprintf(buf,"Exec timeout setting error, rc '%ld'",rc2);
	ct10api_apl_log(ALOG_ERROR, 0, buf, DEBUG_TRACE_LEVEL_NONE);
	return CT10_ERR_FATAL_ERROR;
}

return CT10_SUCCESS;
} /* ---------- End of ct10api_change_exec_timeout_value -------------- */



int ct10api_convert_tab_csdatafmt_to_ct10dataxdef(
	CS_DATAFMT 		(*in_param_def)[],
	CT10_DATA_XDEF_T	*(**pp_tab_p_ct10_data_xdef)[],
	int 			nb_params)
/*
 * Convert an array of CS_DATAFMT into an array of pointers to
 * CT10_DATA_XDEF_T. The address of a pointer is given and the
 * array of pointers is allocate and all CT10_DATA_XDEF are also
 * allocate.
 */
{
int i;

/* This pointer will be use in this function to eliminate on level */
/* of indirection. 						   */

CT10_DATA_XDEF_T *(*p_tab_p_ct10_data_xdef)[];

sprintf(buf,"ct10api_convert_tab_csdatafmt_to_ct10dataxdef");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);


/* Allocating array of pointer to CT10_DATA_XDEF_T */

if ( (p_tab_p_ct10_data_xdef = calloc(nb_params,sizeof(long))) 
	== NULL ) {
	return CT10_ERR_FATAL_ERROR;
}


/* We stop when i = nb_params because array indice start at 0 not 1 */
for (i=0; i<nb_params; i++) {

	/* Allocating CT10_DATA_XDEF_T */

	if (( (*p_tab_p_ct10_data_xdef)[i]=calloc(1,sizeof(CT10_DATA_XDEF_T)))
		== NULL ) {
		return CT10_ERR_FATAL_ERROR;
	}

	/* Moving CS_DATAFMT from CS_DATADMT array to CT10_DATA_XDEF_T */

	memcpy( &(((*p_tab_p_ct10_data_xdef)[i])->fmt),
		&((*in_param_def)[i]),
		sizeof(CS_DATAFMT) );

	((*p_tab_p_ct10_data_xdef)[i])->len = ((*p_tab_p_ct10_data_xdef)[i])->fmt.maxlength;
}

/* Give back the address of the new array. */

*pp_tab_p_ct10_data_xdef = p_tab_p_ct10_data_xdef;


return CT10_SUCCESS;

} /* ----- End of ct10api_convert_tab_csdatafmt_to_ct10dataxdef ------ */



int ct10api_free_tab_ct10dataxdef(
		CT10_DATA_XDEF_T	*(**pp_tab_p_ct10_data_xdef)[],
		int 			nb_params)
/*
 * This function free everything that was allocated by 
 * ct10api_convert_tab_csdatafmt_to_ct10dataxdef.
 * Note: p_val are not free by this function.
 */
{
int x = 0;
CT10_DATA_XDEF_T *(*p_tab_p_ct10_data_xdef)[];

sprintf(buf,"ct10api_free_tab_ct10dataxdef");
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);

if (pp_tab_p_ct10_data_xdef == NULL ) {
	return CT10_SUCCESS;
}

p_tab_p_ct10_data_xdef = *pp_tab_p_ct10_data_xdef;


if (p_tab_p_ct10_data_xdef != NULL) {
	for (x=0; x<nb_params; ++x)
	{
		if ((*p_tab_p_ct10_data_xdef)[x] != NULL) {
			free((*p_tab_p_ct10_data_xdef)[x]);
		}
	}
	free(p_tab_p_ct10_data_xdef);

	*pp_tab_p_ct10_data_xdef = NULL;
}

return CT10_SUCCESS;
} /* ------- End of ct10api_free_tab_ct10dataxdef ------------- */
	


ct10api_exec_rpc_value(	void			*p_ct10_conn,
			char			*name, 
			CT10_DATA_XDEF_T	*(*p_tab_param_def)[],
			CT10_PARAM_VALUE_T	(*tab_param_values)[],
			int			nb_params,
			CS_INT			*p_return_status)
/*
 * This fonction execute a rpc which has the value and the definition 
 * separated.
 *
 */
{
int i;
int rc;

sprintf(buf,"ct10api_exec_rpc_value [%s]", name);
ct10api_apl_log(ALOG_INFO, 0, buf, DEBUG_TRACE_LEVEL_LOW);


for (i=0; i<nb_params; i++) {

    (((*p_tab_param_def)[i])->p_val) =  (*tab_param_values)[i].value.p_val;
    (((*p_tab_param_def)[i])->len) =    (*tab_param_values)[i].value.len;
    (((*p_tab_param_def)[i])->null_ind)=(*tab_param_values)[i].value.null_ind;
}

rc = ct10api_exec_rpc(	p_ct10_conn,
			name, 
			(*p_tab_param_def),
			nb_params,
			p_return_status);


return rc;
} /* ------------- End of ct10api_exec_rpc_value ---------------- */



/* --------------------------------------------- End of ct10api.c --------- */




/**************************************************************************
 Convert a native sybase datatime datatype into a string.
 Calling process must make sure that string_dte point to a space
 big enought to hold the date in string format.

 NOTE: This function is not reliable because we dont know the format of
       the resulting string.
 **************************************************************************/

int ct10api_convert_date_to_string(void *dte, char *string_dte)
{
CS_DATAFMT fmt_dte = {
"",			/* Param name */
0,			/* Len of param name */
CS_DATETIME_TYPE, 	/* Data type */
CS_FMT_UNUSED,		/* Format */
8,			/* maxlen */
0,			/* scale */
0,			/* precision */
0,			/* in or out */
0,			/* count */
0,			/* user type */
NULL			/* locale */
};

CS_DATAFMT fmt_string_dte = {
"",			/* Param name */
0,			/* Len of param name */
CS_CHAR_TYPE, 		/* Data type */
CS_FMT_NULLTERM,	/* Format */
255,			/* maxlen */
0,			/* scale */
0,			/* precision */
0,			/* in or out */
0,			/* count */
0,			/* user type */
NULL			/* locale */
};


long rc;
long outl;


	if ((rc=cs_convert(	context,
				&fmt_dte,
				dte,
				&fmt_string_dte,
				string_dte,
				&outl)) != CS_SUCCEED) {
	return CT10_ERR_FAILED;;
}

return CT10_SUCCESS;;
} /* ----------- End of ct10api_convert_sybase_date_to_string ------------- */




/*
 ****************************************************
 This function break a date into all her part.
 ****************************************************/

int ct10api_crack_date(void *dte, CS_DATEREC *date_rec)
{
	CS_RETCODE rc;
	
	if ((rc = cs_dt_crack(context, CS_DATETIME_TYPE, dte, date_rec)) != CS_SUCCEED) {
		return CT10_ERR_FAILED;
	}

	return CT10_SUCCESS;

} /* ------------ End of ct10api_crack_date -------------------- */



/*
 ****************************************************
 Convert a date into a long. format YYYYMMDD.
 ****************************************************/

int ct10api_convert_date_to_long(void *dte, long *out_dte)
{
	int rc;
	CS_DATEREC date_rec;

	if ((rc = ct10api_crack_date(dte, &date_rec)) !=  CT10_SUCCESS) {
		printf("failed\n");
		return CT10_ERR_FAILED;
	}

	*out_dte = (date_rec.dateyear*10000)+((date_rec.datemonth+1)*100)+date_rec.datedmonth;

	return CT10_SUCCESS;
} /* --------------- End of ct10api_convert_date_to_long ------------- */



/*
 ***********************************************************
 Using date crack to convert sybase date into string is
 the best way to always get the same output format.

 Using local to do so is more complex and very limited.

 string_date must be 24 long

 ***********************************************************/

int ct10api_convert_date_to_string_std(void *dte, char *string_dte)
{
CS_DATEREC date_rec;
int rc;

if ( (rc = ct10api_crack_date(dte, &date_rec)) != CT10_SUCCESS ) {
	return rc;
}

sprintf(string_dte, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d:%.3d",
	date_rec.dateyear,
	date_rec.datemonth+1,
	date_rec.datedmonth,
	date_rec.datehour,
	date_rec.dateminute,
	date_rec.datesecond,
	date_rec.datemsecond);

return CT10_SUCCESS;

} /* ---------------- End of ct10api_convert_date_to_string_std -------- */



/*
 ***************************************************************
 Set the row count limit using ct_option

 set to -1 to disable rowcount setting before a proc call
 set to 0 for no limit
 ***************************************************************/

void ct10api_set_row_count( int nb_rows)
{
s_row_count = nb_rows;

return;

} /* ---------------- End of ct10api_set_row_count ------------- */
