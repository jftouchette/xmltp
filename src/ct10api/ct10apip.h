/* ct10apip.h
 * ----------
 *
 *
 *  Copyright (c) 1996-1997 Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/ct10api/ct10apip.h,v $
 * $Header: /ext_hd/cvs/ct10api/ct10apip.h,v 1.11 2004/10/15 20:16:40 jf Exp $
 *
 *
 * ct10apip.h - CT10 Library API, Private data structures
 *
 *
 * Private Data structures shared by ct10api.c and ct10reco.c
 *
 *
 * ---------------------------------------------------------------------------
 * Versions:
 * 1996avr03,jft: first version
 * 1996avr10,jbl: change CL_DATA_XDEF_T by CT10_DATA_XDEF_T.
 * 1997aug15,jbl: move VERSION_ID from ct10api.h to ct10apip.h .
 *
 */


#ifndef _CT10APIP_H_
#define _CT10APIP_H_

/*
 * Version informations:
 */

#define VERSION_ID	("1.0.2 " __FILE__ ": " __DATE__ " - " __TIME__)


/*
 * Connection status.
 *
 */

#define CT10_CONSTAT_CONNECTED		0x0001
#define CT10_CONSTAT_NULL_PTR		0X0002
#define CT10_CONSTAT_UNKNOWN		0X0004
#define CT10_CONSTAT_DEAD		0X0008

/*
 * Result state bit mask.
 */

#define CT10_RESULT_NOT_PRESENT		0x0001
#define CT10_RESULT_PRESENT		0x0002

/* 
 * CT10_CONN_T	-- Private object shared by ct10api.c and ct10reco.c
 *		--
 *		-- Returned by ct10api.c as a (void *)
 *
 *		-- Only ct10api.c and ct10reco.c know about the exact 
 *		-- content of this struct.
 */
typedef struct {

	unsigned long	 signature;

	CS_CONNECTION	*p_conn;
	CS_COMMAND	*p_cmd;
	CS_BLKDESC	*p_blk;

	char		 login_id[CS_MAX_NAME + 1],
			 password[CS_MAX_NAME + 1],
			 app_name[CS_MAX_NAME + 1],
			 host_name[CS_MAX_NAME + 1],
			 server_name[CS_MAX_NAME + 1],
			 init_string[MAX_CONN_INIT_STRING + 1];

	long		 options_mask;

	int		 nb_params,
			 nb_cols;
	CT10_DATA_XDEF_T *p_params,
			 *p_cols;

	CS_INT		 return_status;

	int		 retry_ctr,
			 max_retry,
			 retry_interval;	/* set by recovery module */


	/* The connection makes itself unusable if there are too many
	 * fatal errors:
	 */
	int		 ctr_of_fatal_errors;

	long		 diagnostic_mask,	/* set by recovery module */
			 diag_sev_mask,
			 action_mask,
			 result_state_mask;	/* tell if rows received... */

	int		 ret_err_code;


	/* pointers to function that customize the results processing:
	 */
	int		 (* pf_process_row_or_params)();


	/* Pointer to function that provides application dependent 
	 * context info for the error logging:
	 */
	char		*(* pf_get_appl_context_info)();

} CT10_CONN_T;



#endif /* #ifndef _CT10APIP_H_ */




/* -----------------Begin of private function declaration. ----------------- */

int ct10api_test_p_conn(void *pt_val, void *pt_struct);
/*
 * Test function use by linklist module. 
 * Test a CS_CONNECTION agains the CT10_CONN_T->p_conn.
 *
 */


int ct10api_process_results(CT10_CONN_T *conn, CS_COMMAND *cmd);
/*
 * Standard results processing function.
 *
 */


int ct10api_process_1_result_set(	CT10_CONN_T	*conn,
					CS_COMMAND	*cmd,
					CS_RETCODE	row_type);
/*
 * Processing of one (1) result set.
 *
 */



int  ct10api_process_row_or_params (	void	 	 *p_ct10_conn, 
					int 		 b_is_row,
					CT10_DATA_XDEF_T *data_array[],
					int		 nb_data,
					int		 result_set_no,
					CS_RETCODE	 result_type,
					int		 header_ind);
/*
 * Default application level result handler.
 *
 */



static CS_RETCODE ct10api_ct_close(CS_CONNECTION *p_conn, CS_INT opt);
int ct10api_get_conn_status(CS_CONNECTION *p_conn);
int ct10api_drop_connection(CS_CONNECTION **p_conn);
int ct10api_close_connection2(CS_CONNECTION *p_conn, CS_INT close_mode);
int ct10api_close_and_drop_connection(CS_CONNECTION **p_conn);
void ct10api_log_ct10_conn(int rc,CT10_CONN_T *pt,char *posi);
void ct10api_init_recov_info(CT10_CONN_T *pt);

int ct10api_save_recov_info(CT10_CONN_T **conn, CT10_CONN_T **conn_bak);
int ct10api_restore_recov_info(CT10_CONN_T **conn, CT10_CONN_T **conn_bak);


/* -------------------------------------------- end of ct10apip.h --------- */
