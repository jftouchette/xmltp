/********************************************
 xmltp_api.h
 -----------

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

 Public functions prototype.

 LIST OF CHANGES:
 ----------------

 2003-06-26, jbl:	Initial release.



 ********************************************/

#define MAX_CONN_INIT_STRING_SIZE 1024

typedef struct {

        CS_DATAFMT      fmt;
        CS_INT          len;           /* length of data               */
        CS_SMALLINT     null_ind;      /* NULL value indicator         */
        void         	*p_val;        /* ptr to buffer to hold value  */

} XMLTP_API_DATA_XDEF_T;

typedef struct {

	unsigned long	 signature;

	char		 login_id[CS_MAX_NAME + 1],
			 password[CS_MAX_NAME + 1],
			 app_name[CS_MAX_NAME + 1],
			 host_name[CS_MAX_NAME + 1],
			 server_name[CS_MAX_NAME + 1],
			 service[CS_MAX_NAME + 1],
			 init_string[MAX_CONN_INIT_STRING_SIZE + 1];

	void		 *s_p_sock_obj;

	long		 options_mask;

	int		 nb_params,
			 nb_cols;

	XMLTP_API_DATA_XDEF_T	*p_params,
			 	*p_cols;

	int		 return_status;

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

	void		*p_ctx;

} XMLTP_API_CONN_T;




void xmltp_api_set_debug_trace_flag(int val);

int xmltp_api_get_debug_trace_flag();

int xmltp_api_assign_process_row_function(void *p_api_ctx,
	int (* pf_process_row_or_params)(
		void                    *p_api_ctx,
		int                     b_is_row,
		XMLTP_API_DATA_XDEF_T   *data_array[],
		int                     nb_data,
		int                     result_set_no,
		int                     result_type,
		int                     header_ind)  );

void *xmltp_api_create_connection(      char *login_id,
					char *password,
					char *app_name,
					char *server_name,
					char *init_string,
					long options_mask);

int xmltp_api_connect(void *p_api_ctx);

int xmltp_api_close_connection( void *p_api_ctx);

int xmltp_api_exec_rpc( void *p_api_ctx,
			char *rpc_name,
			XMLTP_API_DATA_XDEF_T  *params_array[],
			int nb_params,
			int *p_return_status);

int xmltp_api_exec_rpc_string(  void    *p_api_ctx,
				char    *rpc_string,
				int     *p_return_status);

void xmltp_api_reset_result_set_no();

void xmltp_api_reset_nb_column();

void xmltp_api_next_result_set();

void xmltp_api_add_column_def(char *name, int type, char *opt_size);

void xmltp_api_one_row_ready(int header_ind, int result_type, int nb_rows);

void xmltp_api_add_row_elem(char *val, int is_null);

void xmltp_api_assign_error_msg_handler(void (* ptr_funct)() );

void xmltp_api_error_msg_handler(char *msg_number, char *msg_text);

void xmltp_api_set_return_status(int return_status);

char *xmltp_api_get_server_name_of_conn(void *p_api_ctx);

void xmltp_api_rpc_dump(char *rpc_name, XMLTP_API_DATA_XDEF_T  *t_parm[], int nb_parm);

void xmltp_api_assign_application_name(char * app_name);
