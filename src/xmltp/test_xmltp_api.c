/* test_xmltp_api.c
 * ----------------
 * 
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
 *
 * This program use ct10init to test that xmltp_api is compatible with ct10api.
 * xmltp_ct10api compatibility module is require.
 *
 */



#ifndef LINUX
#include <time.h>       	/* for time_t, struct timeval */
#else
#include <sys/time.h>
#endif

#include <stdlib.h>		/* for calloc(), free(), ... */
#include <unistd.h>		/* for sleep() 	*/
#include <errno.h>

#include "ctpublic.h"		/* Sybase CTlib stuff */
#include "oserror.h"		/* for SRV_MAXERROR */

#include "common_1.h"		/* does #include of several common C headers */

#include "ct10api.h"
#ifdef VERSION_ID
#undef VERSION_ID		/* ct10api.h does #define of VERSION_ID, should not */
#endif

#include "ct10reco.h"		/* Ct10api recovery module stuff */
#include "ct10init.h"		/* High level ct10api init stuff */
#include "rpc_null.h"		/* NULL string for all datatype */

#include "ucfg.h"
#include "lbospar1.h"
#include "defpaths.h"

#include "alogelvl.h"
#include "apl_log1.h"
#include "errutl_1.h"		/* for errutl_severity_to_alog_err_level() */
#include "linklist.h"		/* for lkls_xxx() functions */
#include "getline1.h"		/* for g1ln_get_first_line_of_file() */

#include "osclerr1.h"		/* for OSCLERR_CONNECT_FAILED */

#include "ct10str.h"		/* for ct10strncpy() */

#include "sock_fn.h"		/* for sock_XXX() */
#include "sockpara.h"		/* for #define SOCKPARA_xxx */

#include "xmltp_lex.h"		/* for xmltp_lex_get_single_thread_context() */
#include "xmltp_parser.h"	/* for xmltp_ctx_fill_input_buffer() */
#include "xmltp_writer.h"	/* for xmltp_writer_xxx() functions */

#include "javaSQLtypes.h"	/* for JAVA_SQL_CHAR, ... */

#include <strings.h>

/* ------------------------------------ Things not found in #include :  */

#ifndef LINUX
extern	char	*sys_errlist[];
#endif

/* ----------------------------------------------- Private Defines: */

/*
 * Usage and Version info:
 */
#define VERSION_NUM	"0.0.1"

#ifdef VERSION_ID
#undef VERSION_ID
#endif
#define VERSION_ID	\
		(__FILE__ " " VERSION_NUM " - " __DATE__ " - " __TIME__)



/*
 * Parameter names:
 *
 * (These are not already defined in "lbospar1.h").
 *
 */
#define CFG_SECTION			"TEST_XMLTP_API"

/*
 * Define for ct10init
 * See syntax() function for details
 */

#define ARGUMENTS_STRING "F:C:S:U:P:I:A:D:L:d:s:"

/* Options not included in ct10init */

#define ARG_APPL_DEBUG_TRACE_LEVEL 		'd'
#define CONFIG_STRING_APPL_DEBUG_TRACE_LEVEL	"APPL_DEBUG_TRACE_LEVEL"



/* ----------------------------------------------- Private Variables: */

static int	 s_debug_level	   = DEBUG_TRACE_LEVEL_NONE,

		 s_msg_verbose	   = 0,		/* Not use */
		 s_max_failed	   = 5;		/* Not use */

static char	*s_pgm_name	   = __FILE__;

static char	s_log_buf[1024];

static void	*s_p_server_conn   = NULL;

/* ----------------------------------------------- Private Functions: */



/*
 * All these params are require for server connection (ct10init module)
 *
 *
 */

static void sf_syntax(char *pgm_name)
{

        fprintf(stderr,"\n");
        fprintf(stderr,"Syntax: %s <options>...\n", pgm_name);
        fprintf(stderr,"\n");
        fprintf(stderr,"Version: %s\n", VERSION_ID);
        fprintf(stderr,"\n");
        fprintf(stderr,"       options:\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"       -F <config File path and name>\n");
        fprintf(stderr,"       -C <Config file section");
        fprintf(stderr,"       -S <Server name>\n");
        fprintf(stderr,"       -U <User name>\n");
        fprintf(stderr,"       -P <Password file path nad name>\n");
        fprintf(stderr,"       -I <Init string>\n");
        fprintf(stderr,"       -A <Application name>\n");
        fprintf(stderr,"       -D <CT10init debug trace mask >\n");
        fprintf(stderr,"            1) info only, 4294967295) full\n");
        fprintf(stderr,"       -d <test_xmltp_api debug trace level>\n");
        fprintf(stderr,"            0)none, 1)low, 5)medium, 10)full, 15)detailed\n");
        fprintf(stderr,"       -L <Log file path and name>\n");
        fprintf(stderr,"\n");

} /* End of sf_syntax() */

int main(int argc, char **argv)
{
	int	rc = 0;

	int	return_stat;

	char	rpc_string[4096];

	CS_DATEREC dte;

	/* xmltp_api_set_debug_bitmask(15); */

	/* Standard ct10api initialization. */
 
	if ((rc =  ct10init_standard_ct10api_init(
                                        &s_p_server_conn,
                                        ARGUMENTS_STRING,
                                        argc,
                                        argv,
                                        CFG_SECTION,
                                        CT10INIT_CONFIF_PREFIX_IND_NONE,
                                        s_pgm_name,
                                        NULL
                        )) != CT10INIT_SUCCESS) {
 
        	sf_syntax(s_pgm_name);
        	exit(1);
 
	}

	/* ct10api_assign_debug_trace_flag(15); */

	/* We need to log the pid not the parent pid wich is the default */
	alog_set_log_parent_pid_to_false();

	ct10init_get_option_or_config_value(	CT10INIT_ARG_SERVER_NAME, 
						CT10INIT_CONFIG_STRING_SERVER_NAME);

	/* test_xmltp_api debug trace level setting from option or config file */
	if ( ct10init_get_option_or_config_value(ARG_APPL_DEBUG_TRACE_LEVEL, 
						CONFIG_STRING_APPL_DEBUG_TRACE_LEVEL) != NULL ) {

		s_debug_level = atoi(ct10init_get_option_or_config_value(ARG_APPL_DEBUG_TRACE_LEVEL,
                                                CONFIG_STRING_APPL_DEBUG_TRACE_LEVEL) );

		if (errno != 0)  {
			sprintf(s_log_buf, "main: invalid config or option for debug level [%.10s], intval [%d], errno [%d]",
				 ct10init_get_option_or_config_value(ARG_APPL_DEBUG_TRACE_LEVEL, 
						CONFIG_STRING_APPL_DEBUG_TRACE_LEVEL),
				 s_debug_level,
				 errno);

			s_debug_level	   = DEBUG_TRACE_LEVEL_NONE;

		}

	}


	/* rc = ct10api_connect(s_p_server_conn);  */

/****
	strcpy(rpc_string,
		"LOGIN @p_username=\"wk_client_gw\", @p_password=\"bigwart1\", @p_clt_name=\"TRG-X\"");
	printf("RPC[%s]", rpc_string);

	rc = ct10api_exec_rpc_string(
		s_p_server_conn,
		rpc_string,
		&return_stat);
****/


/**
	rc = ct10api_exec_rpc_string(
		s_p_server_conn,
		"sp_who",
		&return_stat);

 	rc = ct10api_exec_rpc_string(
		s_p_server_conn,
		"spwk_test_timeout",
		&return_stat);

	printf("\nReturn status: %d\n", return_stat);
	exit(1);
***/

	return_stat = -888;
 	rc = ct10api_exec_rpc_string(
		s_p_server_conn, "sp_who2", &return_stat);

	printf("\nReturn status: %d, rc: %d\n", return_stat, rc);

	return_stat = -888;

 	rc = ct10api_exec_rpc_string(
		s_p_server_conn, "SPCT_MANAGE_KC_FILLS @p_usr_id = \"NB027NB\", @p_bs_typ = \"Bought\", @p_ext_sym_cde = \"NT\", @p_fil_qty = 500, @p_fil_prc = 4.740000, @p_bal_qty = 0, @p_txn_cat = \"\", @p_tck_id = \"1084E1I9.1A\", @p_fil_dte = \"2004/05/26 08:39:53\", @p_fil_stp_id = 977, @p_snt_dte = \"2004/05/26 08:39:53\", @p_xch_bkr_id = \"80\", @p_seq_no = 849, @p_mkt_cde = \"U\", @p_cxl_ind = \"0\", @p_rec_id = 977, @p_ext_aid = \"\", @p_xch_sym_cde1 = \"$$usdcad.bnf\", @p_xch_rte_tim1 = \"\", @p_ext_xch_rte_bid1 = NULL_F, @p_ext_xch_rte_ask1 = NULL_F, @p_xch_rte_bid_spd1 = NULL_F, @p_xch_rte_ask_spd1 = NULL_F, @p_xch_sym_cde2 = \"$$usdcad\", @p_xch_rte_tim2 = \"\", @p_ext_xch_rte_bid2 = NULL_F, @p_ext_xch_rte_ask2 = NULL_F, @p_xch_rte_bid_spd2 = NULL_F, @p_xch_rte_ask_spd2 = NULL_F, @p_xch_sym_cde3 = \"$$usdcad.rbc\", @p_xch_rte_tim3 = \"\", @p_ext_xch_rte_bid3 = NULL_F, @p_ext_xch_rte_ask3 = NULL_F, @p_xch_rte_bid_spd3 = NULL_F, @p_xch_rte_ask_spd3 = NULL_F, @p_tim_spd_ind = \"T\", @p_pas_ord = \"\", @p_ass_stk = \"\", @p_crs_trd = \"N\"", &return_stat);

	printf("\nReturn status: %d, rc: %d\n", return_stat, rc);

	return_stat = -888;

 	rc = ct10api_exec_rpc_string(
		s_p_server_conn, "SPCT_MANAGE_KC_FILLS @p_usr_id = \"NB027NB\", @p_bs_typ = \"Bought\", @p_ext_sym_cde = \"BBD.B\", @p_fil_qty = 500, @p_fil_prc = 4.740000, @p_bal_qty = 0, @p_txn_cat = \"\", @p_tck_id = \"1084E1I9.1A\", @p_fil_dte = \"2004/05/26 08:39:53\", @p_fil_stp_id = 977, @p_snt_dte = \"2004/05/26 08:39:53\", @p_xch_bkr_id = \"80\", @p_seq_no = 849, @p_mkt_cde = \"U\", @p_cxl_ind = \"0\", @p_rec_id = 977, @p_ext_aid = \"\", @p_xch_sym_cde1 = \"$$usdcad.bnf\", @p_xch_rte_tim1 = \"\", @p_ext_xch_rte_bid1 = NULL_F, @p_ext_xch_rte_ask1 = NULL_F, @p_xch_rte_bid_spd1 = NULL_F, @p_xch_rte_ask_spd1 = NULL_F, @p_xch_sym_cde2 = \"$$usdcad\", @p_xch_rte_tim2 = \"\", @p_ext_xch_rte_bid2 = NULL_F, @p_ext_xch_rte_ask2 = NULL_F, @p_xch_rte_bid_spd2 = NULL_F, @p_xch_rte_ask_spd2 = NULL_F, @p_xch_sym_cde3 = \"$$usdcad.rbc\", @p_xch_rte_tim3 = \"\", @p_ext_xch_rte_bid3 = NULL_F, @p_ext_xch_rte_ask3 = NULL_F, @p_xch_rte_bid_spd3 = NULL_F, @p_xch_rte_ask_spd3 = NULL_F, @p_tim_spd_ind = \"T\", @p_pas_ord = \"\", @p_ass_stk = \"\", @p_crs_trd = \"N\"", &return_stat);

	printf("\nReturn status: %d, rc: %d\n", return_stat, rc);

	exit(1);

 	 rc = ct10api_exec_rpc_string(
		s_p_server_conn, "spwk_test_int @p_int=5000000000", &return_stat);

	printf("\nReturn status: %d, rc: %d\n", return_stat, rc);
	

 	rc = ct10api_exec_rpc_string(
		s_p_server_conn,
		"spwk_test_trgx @p1=1 out, @p2=1.123 out, @p3=\"allo123\" out, @p4=\"2002-05-01\" out",
		&return_stat);

	printf("\nReturn status: %d\n", return_stat);

	exit(1);

 	rc = ct10api_exec_rpc_string(
		s_p_server_conn,
		"spwk_test_trgx @p1=1 out, @p2=1.123 out, @p3=\"allo123\" out, @p4=\"2002-05-01\" out",
		&return_stat);

	printf("\nReturn status: %d\n", return_stat);

 	rc = ct10api_exec_rpc_string(
		s_p_server_conn,
		"sp_who2",
		&return_stat);

	printf("\nReturn status: %d\n", return_stat);

	rc = cs_dt_crack(NULL, 0, "2003-06-12 09:45:37:036", &dte);

	printf("Year: %d\n", dte.dateyear); 


 	rc = ct10api_exec_rpc_string(
		s_p_server_conn,
		"spwk_test_float @p_f6 = 10.123",
		&return_stat);

	printf("\nReturn status: %d\n", return_stat);

 	rc = ct10api_exec_rpc_string(
		s_p_server_conn,
		"spoe_list_held_order @p_adt_usr_id = \"oliviesy\", @p_lng = \"eng\"",
		&return_stat);

	printf("\nReturn status: %d\n", return_stat);



	/* ct10api_assign_process_row_function(s_p_server_conn, sf_process_row); */

	rc = ct10api_close_connection(s_p_server_conn);
	printf("Close connection rc [%d]\n", rc);

	rc = xmltp_api_drop_connection(s_p_server_conn);
	printf("Drop connection rc [%d]\n", rc);

	return (0);

} /* end of main() */


/* -------------------------------------- end of test_xmltp_api.c ----------- */
