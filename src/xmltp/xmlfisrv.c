/* xmlfisrv.c
 * ---------
 *
 *
 *  Copyright (c) 2004 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/xmltp/xmlfisrv.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmlfisrv.c,v 1.3 2005/10/03 17:55:07 toucheje Exp $
 *
 *
 * xmlfisrv.c: XMLTP file server -- limited version, used for load testing
 * ----------
 *
 * Command line:
 *		xmlfisrv PortNumber [spName ... ]
 * 
 * This XMLTP server emulates the behaviour of XML2SYB in a very limited way.
 * When some specified RPC calls come in, the server send back "canned" XML
 * responses. These XMLTP responses are stored in files and each of those 
 * files has the same name as the proc name.
 * 
 * These proc names are enumerated in the table s_v_list_of_RPC_names .
 *
 * Other features:
 *  . same regprocs as in xml2syb.c
 *  . writes log to a "xmltpsrv.log" file
 *  . 100% independent of CT-Lib and ct10api
 *
 * Limitations:
 *  . no support of config file
 *  . no support of xmltp_hosts file
 *  . no RPC audit file
 *
 *
 * --------------------------------------------------------------------------
 * Versions:
 * 2004feb15,jft: first version (based on xml2syb.c)
 *		  increase XML_FILE_BUFFER_SIZE from 100 KB to 1000 KB
 * 2004jul18,jft: the names of the SP allowed are give on the command line
 * 2005jan06,jft: + regproc "setwait" qui permet de forcer une attente de (n) secondes avant de retourner la reponse.
 * 2006jun11,jft: #ifdef CT10_DATA_XDEF_T in sf_setwait() to avoid depending on CT10_DATA_XDEF_T to compile.
 *		  only include ctpublic.h and ct10api.h #ifndef NO_CT10API
 */

#ifndef LINUX

#ifndef _INCLUDE_HPUX_SOURCE
#define _INCLUDE_HPUX_SOURCE
#include <time.h>       	/* for time_t, struct timeval */
#undef  _INCLUDE_HPUX_SOURCE
#else
#include <time.h>       	/* for time_t, struct timeval */
#endif

#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>		/* for calloc(), free(), ... */
#include <unistd.h>		/* for sleep() 	*/
#include <errno.h>

#include <sys/file.h>
#include <signal.h>

/* ===== */
#ifndef NO_CT10API
#include "ctpublic.h"		/* Sybase CTlib stuff */
#include "ct10api.h"
#endif

#include "oserror.h"		/* for SRV_MAXERROR */
/* ===== */

#include "common_1.h"		/* does #include of several common C headers */

#include "ucfg.h"

#include "alogelvl.h"
#include "apl_log1.h"
#ifndef CS_INT
#define CS_INT		int
#define CS_CHAR		char
#define CS_SMALLINT	short
#define CS_RETCODE	int
#endif
#include "errutl_1.h"		/* for errutl_severity_to_alog_err_level() */
#include "debuglvl.h"		/* for DEBUG_TRACE_LEVEL_NONE */
#include "linklist.h"		/* for lkls_xxx() functions */

#include "sock_fn.h"		/* for sock_XXX() */
#include "sockpara.h"		/* for #define SOCKPARA_xxx */

#include "xmltp_lex.h"		/* for xmltp_lex_get_single_thread_context() */
#include "xmltp_parser.h"
#include "xmltp_writer.h"
#include "xmltp_ctx.h"		/* for xmltp_ctx_get_socket() */

#include "javaSQLtypes.h"	/* for JAVA_SQL_CHAR, ... */
#include "xml2db.h"		/* xml2db_XXX() functions prototypes */
#include "xmltp_hosts.h"	/* logical name, host name, port number */

#include <strings.h>

/* ------------------------------------ Things not found in #include :  */

#ifndef LINUX
extern	char	*sys_errlist[];
#endif

/* ----------------------------------------------- Private Defines: */

/*
 * Usage and Version info:
 */
#define VERSION_NUM	"0.1.1"

#ifdef VERSION_ID
#undef VERSION_ID
#endif
#define VERSION_ID	\
		(__FILE__ " " VERSION_NUM " - " __DATE__ " - " __TIME__)


/*
 * Application specific constants:
 */
#define APPNAME_IN_LOG_FILE		"xmlfisrv"

#define LOG_FILENAME			"xmltpsrv.log"


/*
 * Limits:
 */

#define MAX_SYB_VALUE_SIZE		256
#define MAX_SYB_NB_PARAMS		256

#define WORK_BUFFER_SIZE		256

#define XML_FILE_BUFFER_SIZE		1024000	/* 1000 KB */




/*
 * Required to replace return stat from RPC
 * for specific error or if no return stat
 * is available.
 */

#define RPC_RETURN_STAT_FILE_NOT_ALLOWED	-500
#define RPC_RETURN_STAT_FILE_OPEN_FAILED	-1
#define	RPC_RETURN_STAT_FOR_SUCCESS		 0

/*
 * Registered procedure stuff...
 */

#define MSG_BUFF_SIZE			2048

#define REG_PROC_XML2DB_DEBUG_LOW	"xml2db_debug_low"		/* turn on Low debug level in xmltp_ctx.c */
#define REG_PROC_XML2DB_DEBUG_FULL	"xml2db_debug_full"		/* turn on Full debug level in xmltp_ctx.c */
#define REG_PROC_XML2DB_PING		"xml2db_ping"			/* simply send back return status == 0 */
#define REG_PROC_PING			"ping"				/* like "xml2db_ping", but NO logging */
#define REG_PROC_DB_LOGIN		"login"				/* Login, password to sybase + TRG name for log */
#define REG_PROC_KILL			"kill"				/* Kill a PID */
#define REG_PROC_SETWAIT		"setwait"			/* set to wait (sleep) N seconds before replying to the next SP calls */

/* Not used */
#define REG_PROC_DB_LOGOUT		"logout"			/* Clean termination of xmlfisrv */
#define REG_PROC_DUMP_STAT		"dump_stats"			/* Dump stat to oesyssrv.log */
#define REG_PROC_RESET_STAT		"reset_stats"			/* Reset all stats counter to zero */
#define REG_PROC_SET_TRACE_LEVEL	"set_trace_level"		/* Change the trace level to allow prod debug */
#define REG_PROC_CHANGE_ROW_COUNT_LIMIT	"change_row_count_limit"	/* Required for Batch TRG to raise the limit */

#define RP_DB_LOGIN_PARAM_USERNAME_POSI	0
#define RP_DB_LOGIN_PARAM_PASSWORD_POSI	1
#define RP_DB_LOGIN_PARAM_TRG_NAME_POSI	2

#define RP_DB_LOGIN_PARAM_USERNAME_SIZE	256
#define RP_DB_LOGIN_PARAM_PASSWORD_SIZE (RP_DB_LOGIN_PARAM_USERNAME_SIZE)	
#define RP_DB_LOGIN_PARAM_TRG_NAME_SIZE	(RP_DB_LOGIN_PARAM_USERNAME_SIZE)


/* ----------------------------------------------- Global Variable: */

int		 g_debug = 0;		/* for xmltp_lex.c	*/
int		 verbose = 0;		/* to please aplogmsg.c */

int		 g_wait_n_seconds = 0;	/* assigned by "setwait" regproc. Default is zero, no wait before doing a SP */


/* ----------------------------------------------- Private Variables: */


/* Log application message type table. 
 */
static struct int_str_tab_entry s_type_strings_table[] = {
#include "alogtypt.h"
 
        { ISTR_NUM_DEFAULT_VAL,  "?     " }
};


#if 0	/* UNUSED */
struct st_simple_param_struct {
	char	name[256];
	int	param_type,
		is_null,
		is_output;
	int	int_value;
	double	double_value;
	char	char_date_value[256];
} ;
#endif	


static int	 s_debug_level	   = DEBUG_TRACE_LEVEL_NONE;

static char	*s_pgm_name	   = __FILE__;

/* Once s_b_login_proc_done is True, the following (3) values should not be modified:
 */
static char	s_user_name[RP_DB_LOGIN_PARAM_USERNAME_SIZE] = "[username]",
		s_password[RP_DB_LOGIN_PARAM_USERNAME_SIZE]  = "[password]",
		s_trg_name[RP_DB_LOGIN_PARAM_USERNAME_SIZE]  = "[trg_name]";

static int	s_b_login_proc_done = 0,
		s_ctr_params	    = 0;

static int 	s_row_count = 0;


/*
 * required static variables to logs xmltpsrv.log
 *
 */

static struct timeval    st_tval_begin_rpc;	/* Time of RPC start */
static struct timeval    st_tval_end_rpc;	/* Time of RPC end */
static int		 s_rpc_nb_rows = 0;	       /* Total number of rows for all results set */
static int		 s_rpc_return_status = -99999; /* Stored procedure return status */



/* Others static variables */

static char s_msg_buff[MSG_BUFF_SIZE+1]; 			/* Use to build log messages */
static int  s_timeout_ind = 0;

/* xmltp_hosts related stuff */
#define ENV_XMLTP		"XMLTP"
#define PATH_SIZE		256
#define HOSTNAME_SIZE		256
#define LISTEN_PORT_SIZE	32
#define XMLTP_HOST_FILE_NAME	"xmltp_hosts"

static char	s_host_name[HOSTNAME_SIZE];
static char	s_listen_port[LISTEN_PORT_SIZE];

/*
 * The address of this string indicates that this is a Dummy parameter structure.
 */
static char	*s_dummy_param_struct = "[DummyParamStruct]";


/*
 * The following RPC names are emulated by sending the content of the files
 * with the _same_ names in the current directory (all these files must
 * contain a valid XMLTP response):
 */
static char *s_v_list_of_rpc_names[] = {
	"spjf_t20",
	"spjf_t40",
	"spjf_t50",
	"spjf_t100",
	"spjf_t200",
	"spjf_t500",
	"spjf_t1000",
	"spjf_t2000",
	NULL
	};
#define NB_RPC_NAMES_IN_LIST  (sizeof(s_v_list_of_rpc_names) / sizeof(char *) )


static char **s_p_v_list_of_RPC_allowed = s_v_list_of_rpc_names;
static int    s_nb_rpc_allowed		= NB_RPC_NAMES_IN_LIST;



/*
 * s_xml_file_buffer[] is a very simple cache of the last XML file read.
 *
 * If the requested XML filename is the same as s_last_file_read,
 * then, the file will not be opened and read, the content of the
 * buffer with be sent. That's it.
 *
 * This naive caching will not work well if the filesize is larger than sizeof(s_xml_file_buffer)
 * which should be >= 1000 KB. See #define XML_FILE_BUFFER_SIZE above.
 */
static char s_last_file_read[PATH_SIZE] = "[noFileReadYet]";
static char s_xml_file_buffer[XML_FILE_BUFFER_SIZE] = "";
static int  s_n_bytes_in_xml_buffer = 0;






/* ----------------------------------------------- Private Functions: */


static void sf_syntax(char *pgm_name)
{

        fprintf(stderr,"\n");
        fprintf(stderr,"Syntax: %s <listenPort> [ <spName> ... ]\n", pgm_name);
        fprintf(stderr,"\n");
        fprintf(stderr,"Version: %s\n", VERSION_ID);
        fprintf(stderr,"\n");

} /* End of sf_syntax() */




/*
 * Copy from logrpcfn.c
 *
 * Format time value for log entry.
 *
 */

static int sf_format_usec_time( char  *edit_buffer, int buffer_sz,
                                unsigned long   t_sec,
                                long            t_usec)
{
        char    timwork[60];

        if (edit_buffer == NULL) {
                return (-1);
        }
        if (buffer_sz < 50) {
                return (-2);
        }

	memset(timwork, 0, sizeof(timwork) );

        strftime(timwork, sizeof(timwork), "%Y-%m-%d %H:%M:%S",
                 localtime( (time_t *) &(t_sec) ) );

        /* Concat miliseconds (usec / 1000) to date-time string
         * in the edit_buffer:
         */
        sprintf(edit_buffer, "%.44s:%03d",
                             timwork,
                             ( (t_usec + 499L) / 1000L) );

        return (0);

} /* End of sf_format_usec_time() */



/* 
 * Use to log regular message to log
 */

static void sf_write_error_to_log(int err_level, int msg_no, char *err_msg)
/*
 * Called by:	sf_handle_error_msg()
 *		sf_diag_rpc_exec_error()
 */
{
	alog_aplogmsg_write(err_level,
			    s_trg_name,
			    APPNAME_IN_LOG_FILE,
			    msg_no,
			    "<noUser>",
			    err_msg);

} /* End of sf_write_error_to_log() */




/*
 * Use to save the time at the start of a stored procedure.
 */

static void sf_save_start_time()
{
	struct timezone   tz_dummy;

	gettimeofday(&st_tval_begin_rpc, &tz_dummy );

	return;
} /* End of sf_save_start_time() */




/*
 * Use to save the time at the end of a stored procedure.
 */

static void sf_save_end_time()
{
	struct timezone   tz_dummy;

	gettimeofday(&st_tval_end_rpc, &tz_dummy );

	return;
} /* End of sf_save_end_time */




/*
 * This code is a copy from the cig-trg source code.
 * It is required to log duration of stored proc in milliseconds.
 */

static long sf_delta_tval_in_ms(struct timeval *p_tv1, struct timeval *p_tv2)
/*
 * Return difference tv2 - tv1 in ms (milliseconds)
 */
{
        struct timeval  delta;
	long	tv_usec, tv_sec;;

	tv_usec = p_tv2->tv_usec;
	tv_sec  = p_tv2->tv_sec;

 
        if (p_tv1->tv_usec > p_tv2->tv_usec) {
                tv_usec += 1000000;
                tv_sec--;
        }
        delta.tv_sec  = tv_sec  - p_tv1->tv_sec;
        delta.tv_usec = tv_usec - p_tv1->tv_usec;
 
        return (  (delta.tv_sec  * 1000L)
                + (delta.tv_usec / 1000L) );
 
} /* End of sf_delta_tval_in_ms() */



/* 
 * Use to log init message to oesyssrv.log.
 * Message with 2 separate strings.
 */

static void sf_log_init_msg(int err_level, int msg_no, char *msg1, char *msg2)
{
	char	msg_buff[700] = "initmsg?!";

	sprintf(msg_buff, "%.200s: %.200s", msg1, msg2);

	sf_write_error_to_log(err_level, msg_no, msg_buff);

} /* End of sf_log_init_msg() */
	



/*
 * Look like sf_log_init_msg but with bigger messages.
 */

static void sf_write_log_msg_mm(int err_level, int msg_no, char *msg1, char *msg2)
{
	char	msg_buff[900] = "msg?!";

	sprintf(msg_buff, "%.300s %.300s", msg1, msg2);

	sf_write_error_to_log(err_level, msg_no, msg_buff);

} /* End of sf_write_log_msg_mm() */







/*
 * This function write the rpc completion message into oesyssrv.log
 * NOTE: rc_mask, spid are hardcoded to zero,
 *	 from is hardcoded to trgx
 */

static void sf_log_rpc(char *proc_name)
{
	char	msg_buff[600] = "msg?!";

	sf_save_end_time();

	sprintf(msg_buff, "%.8s,  End of '%.100s' %ld ms, from %s, spid=%d, stat=%d, %d bytes - rc_mask=%ld",	
		"xmlfisrv",
		proc_name,
		sf_delta_tval_in_ms(&st_tval_begin_rpc, &st_tval_end_rpc),
		"trgx",
		getpid(),
		s_rpc_return_status,
		s_n_bytes_in_xml_buffer,	/* usually it would be s_rpc_nb_rows */
		0);

	sf_write_error_to_log(ALOG_INFO, 0, msg_buff);

} /* End of sf_log_rpc() */




/*
 * Not use 
 * We may have to log hostname.
 */

static char *sf_get_hostname()
{
 static char	hostname_buff[80] = "[hostname?]";

	if (gethostname(hostname_buff, sizeof(hostname_buff)) == 0) {
		return (hostname_buff);
	}
	return ("[gethostname() failed]");

} /* End of sf_get_hostname() */








/*
 * Before a new SP these variables must be reset.
 */

static void sf_reset_static_values()
{

	sf_save_start_time();

	s_rpc_nb_rows = 1;		/* We start with 1 for the return status */
	s_rpc_return_status = -999;

} /* End of sf_reset_static_values() */






/*
 * A few think not done in ct10init
 */

static int sf_init_cfg_log_ct10api()
/*
 * Called by:	main()
 *
 * RETURN:
 *	0	OK
 *	< 0	Failed
 */
{
	if (s_debug_level >= DEBUG_TRACE_LEVEL_LOW) {
		printf("\nversion: %.80s\n", VERSION_ID);
		fflush(stdout);
	}

	return (0);

} /* end of sf_init_cfg_log_ct10api() */





static int sf_init_master_socket_listen()
/*
 * Called by:	main()
 *
 * Do bind() and listen() for the "master" socket (only in the
 * parent process, not in the children).
 *
 * Return:
 *	>=0	a valid listen_socket
 *	-1	retry limit exhausted to try bind and listen
 *	-2	config file param missing
 */
{
	int	 listen_socket 	= -1;
	int	 sleep_retry	= 0,
		 nb_retry_listen = 0,
		 queue_lenth	= 0;
	char	 msg[300] = "[msg?]";

	/* This is a load test program. So we hard code some typical values:
	 */
	sleep_retry     = SOCKPARA_SLEEP_RETRY_BIND_DEFAULT;

	nb_retry_listen = SOCKPARA_NB_RETRY_LISTEN_DEFAULT;

	queue_lenth     = SOCKPARA_QLEN_DEFAULT;
	if (queue_lenth < 5) {
		queue_lenth = 5;
	}

	if (NULL == s_listen_port) {
		sf_write_log_msg_mm(ALOG_ERROR, 0, "Parameter missing:",
						SOCKPARA_LISTEN_PORT ); 
		return (-2);
	}

	/* Write the parameters values in the log (to help troubleshooting):
	 */
	sprintf(msg, "%.30s=%d, %.30s=%d, %.30s=%d, %.30s=%.30s (NOT from config file)",
		SOCKPARA_SLEEP_RETRY_BIND,	sleep_retry,
		SOCKPARA_NB_RETRY_LISTEN,	nb_retry_listen,
		SOCKPARA_QLEN,			queue_lenth,
		SOCKPARA_LISTEN_PORT, 		s_listen_port  );

	sf_write_log_msg_mm(ALOG_INFO, 0, "service socket parameters:", msg);

	
	while ( nb_retry_listen > 0 ) {

		listen_socket =	sock_socket_bind_and_listen_ext(ANY_HOST_ADDRESS,
							    s_listen_port,
							    queue_lenth, 1 );
		if ( listen_socket >= 0 ){
			return (listen_socket);
		}
		sf_write_log_msg_mm(ALOG_INFO, 0,
				"Failed, sock_socket_bind_and_listen_ext():",
				sock_get_last_error_diag_msg() );
		sleep( sleep_retry );

		nb_retry_listen--;
	}

	sf_write_log_msg_mm(ALOG_ERROR, 0,
		"ERROR: bind/listen() failed too many times in sf_init_master_socket_listen():",
		sock_get_last_error_diag_msg() );

	return( -1 );

} /* End of sf_init_master_socket_listen() */




/* -------------------------------------- Functions to process RPC: */	



/* -------------------------------- PUBLIC xml2db_ API functions: */

static	int	s_abort_flag	= 0,
		s_abort_rc	= 0;
static	char	s_abort_reason1[300] = "[no reason yet]",
		s_abort_reason2[300] = "[no reason2]";


void xml2db_abort_program(int rc, char *msg1, char *msg2)
/*
 * Used by the parser to tell the main program that too many
 * parse errors have occurred, that the XML-TP protocol is not
 * followed, and, that it should drop the connection and abort.
 *
 * The explanations about the error conditions are in the strings
 * msg1, and, msg2.
 */
{
	int error_level = 0;

	s_abort_flag++;
	s_abort_rc = rc;
	if (NULL == msg1) {
		msg1 = "";
	}
	if (NULL == msg2) {
		msg2 = "";
	}
	strncpy(s_abort_reason1, msg1, (sizeof(s_abort_reason1) - 1) );
	strncpy(s_abort_reason2, msg2, (sizeof(s_abort_reason2) - 1) );

	if ( (rc < -1) || (rc >= 0) ) {
		error_level = ALOG_WARN;
	}
	else {
		error_level = ALOG_INFO;
	}

	/* Do NOT write log if msg2[0] == '0', unless debug level is 0 :
	 */
	if (msg2 != NULL && msg2[0] == '0' && s_debug_level < 3) {
		return;
	}
	sf_write_log_msg_mm(error_level, rc, msg1, msg2);

} /* end of xml2db_abort_program() */





void xml2db_write_log_msg_mm(int errlvl, int rc, char *msg1, char *msg2)
{
	sf_write_log_msg_mm(errlvl, rc, msg1, msg2);

} /* end of xml2db_write_log_msg_mm() */





int xml2db_jdbc_datatype_id_to_native(int jdbc_type_id)
{
 static int	already_said = 0;	/* only write log once about invalid type */
	char	errstr[80] = "[other?]";
 
	switch (jdbc_type_id) {
	case JAVA_SQL_VARCHAR:
	case JAVA_SQL_CHAR:
		return (1);

	case JAVA_SQL_DATE:
	case JAVA_SQL_TIME:
	case JAVA_SQL_TIMESTAMP:
		return (2);

	case JAVA_SQL_INTEGER:
		return (3);

	case JAVA_SQL_DOUBLE:
	case JAVA_SQL_FLOAT:
		return (4);

	/* All those which follow are not supported at this moment.
	 * Normally, they are not generated by the Java framework.
	 * See classes: com.cjc.common.sql.lowLevel.SQLValue
	 *		com.cjc.common.sql.lowLevel.SQLParam
	 * (2001sept05,jft)
	 */
	case JAVA_SQL_BIGINT:
	case JAVA_SQL_BINARY:
	case JAVA_SQL_BIT:
	case JAVA_SQL_DECIMAL:
	case JAVA_SQL_LONGVARBINARY:
	case JAVA_SQL_LONGVARCHAR:
	case JAVA_SQL_NULL:
	case JAVA_SQL_NUMERIC:
	case JAVA_SQL_OTHER:
	case JAVA_SQL_REAL:
	case JAVA_SQL_SMALLINT:
	case JAVA_SQL_TINYINT:
	case JAVA_SQL_VARBINARY:
	default:
		break;
	}
	if (!already_said) {
		already_said++;
		sprintf(errstr, "[Unsupported JDBC type %d]", jdbc_type_id);
		sf_write_log_msg_mm(ALOG_ERROR, jdbc_type_id, errstr,
					"xml2db_jdbc_datatype_id_to_native()");
	}

	return (-999);	/* usually type <= -1 are illegal in all databases */

} /* end of xml2db_jdbc_datatype_id_to_native() */





char *xml2db_describe_jdbc_datatype_id(int jdbc_type_id)
/*
 * Return a string that describe this JDBC datatype.
 */
{
 static char	errstr[80] = "[other?]";

	switch (jdbc_type_id) {
	case JAVA_SQL_BIGINT:
		return ("JAVA_SQL_BIGINT");
	case JAVA_SQL_BINARY:
		return ("JAVA_SQL_BINARY");
	case JAVA_SQL_BIT:
		return ("JAVA_SQL_BIT");
	case JAVA_SQL_CHAR:
		return ("JAVA_SQL_CHAR");
	case JAVA_SQL_DATE:
		return ("JAVA_SQL_DATE");
	case JAVA_SQL_DECIMAL:
		return ("JAVA_SQL_DECIMAL");
	case JAVA_SQL_DOUBLE:
		return ("JAVA_SQL_DOUBLE");
	case JAVA_SQL_FLOAT:
		return ("JAVA_SQL_FLOAT");
	case JAVA_SQL_INTEGER:
		return ("JAVA_SQL_INTEGER");
	case JAVA_SQL_LONGVARBINARY:
		return ("JAVA_SQL_LONGVARBINARY");
	case JAVA_SQL_LONGVARCHAR:
		return ("JAVA_SQL_LONGVARCHAR");
	case JAVA_SQL_NULL:
		return ("JAVA_SQL_NULL");
	case JAVA_SQL_NUMERIC:
		return ("JAVA_SQL_NUMERIC");
	case JAVA_SQL_OTHER:
		return ("JAVA_SQL_OTHER");
	case JAVA_SQL_REAL:
		return ("JAVA_SQL_REAL");
	case JAVA_SQL_SMALLINT:
		return ("JAVA_SQL_SMALLINT");
	case JAVA_SQL_TIME:
		return ("JAVA_SQL_TIME");
	case JAVA_SQL_TIMESTAMP:
		return ("JAVA_SQL_TIMESTAMP");
	case JAVA_SQL_TINYINT:
		return ("JAVA_SQL_TINYINT");
	case JAVA_SQL_VARBINARY:
		return ("JAVA_SQL_VARBINARY");
	case JAVA_SQL_VARCHAR:
		return ("JAVA_SQL_VARCHAR");
	default:
		break;
	}
	sprintf(errstr, "[Unknown JDBC type %d]", jdbc_type_id);

	return (errstr);

} /* end of xml2db_describe_jdbc_datatype_id() */





void *xml2db_create_param_struct(char *name,
				 int   datatype,
				 int   is_null,
				 int   is_output,
				 char *str_value,
				 char *opt_size)
/*
 * Called by:	xmltp_parser_build_param()
 *
 * Create a parameter structure.
 *
 * Arguments:
 *	char *name	-- a string, name of the parameter
 *	int   datatype	-- a JDBC datatype, see javaSQLtypes.h
 *			-- probably needs to be converted to native value
 *	int   is_null	-- 1 if NULL,   0: non-NULL
 *	int   is_output -- 1 if output, 0: input parameter (normal)
 *	char *str_value	-- pointer to string value
 *			-- might need to be converted to native value
 *
 * Return:
 *	NULL		out of memory, or invalid arguments (see log)
 *	(void *)	pointer to a native parameter structure
 */
{
	int			 syb_type = -1;

	/* WARNING: this version of this function is mostly a stub!
	 */

	/* The first RPC call that we should receive is "login"...
	 * If this is the case, we save the values of (first) 3 params.
	 * We can cheat and copy the values directly because we know
	 * that they are strings, and, it's unlikely that they contain
	 * metachar.
	 */
	if (!s_b_login_proc_done && s_ctr_params < 3) {
		switch (s_ctr_params) {
		case (0):
			strncpy(s_user_name, str_value, sizeof(s_user_name) );
			break;
		case (1):
			strncpy(s_password, str_value, sizeof(s_password) );
			break;
		case (2):
			strncpy(s_trg_name, str_value, sizeof(s_trg_name) );
			break;
		default:
			break;
		}
		s_ctr_params++;
	}

	syb_type = xml2db_jdbc_datatype_id_to_native(datatype);
	if (syb_type <= -1) {
		sf_write_log_msg_mm(ALOG_ERROR, datatype, "Unsupported datatype in parameter",
				xml2db_describe_jdbc_datatype_id(datatype) );
		return (NULL);
	}


	return (s_dummy_param_struct);	/* This is NOT the address of a real param struct! */

} /* end of xml2db_create_param_struct() */





int xml2db_free_param_struct(void *p_param)
/*
 * Destroy a parameter structure that had been created by
 * xml2db_create_param_struct().
 *
 * Afterwards, the calling program should not referenced again p_param.
 *
 * Return:
 *	0	p_param was a valid parameter structure
 *	-1	p_param was NULL
 *	<= -2	p_param was not a valid struct (detection not guaranteed)
 */
{
	if (p_param == s_dummy_param_struct) {
		return (0);
	}

	sf_write_log_msg_mm(ALOG_ERROR, (int) p_param, "xml2db_free_param_struct():", "this is a stub, cannot free() p_param" );

	return (-2);

} /* end of xml2db_free_param_struct() */





char *xml2db_get_name_of_param(void *p_param)
/*
 * Return the name of the parameter held in *p_param.
 *
 * Return:
 *	NULL	p_param was NOT a valid parameter structure
 *	char *	name of the parameter
 */
{
	return ("[DummyParamName]");		/* STUB version s_dummy_param_struct is a string */

} /* end of xml2db_get_name_of_param() */



/***********************************
 *
 * Kill another xmlfisrv process.
 * The process number is get
 * as a int parameter from
 * the TRGX.
 * 
 ***********************************/

void sf_kill_process (
	void	*tab_params[], 
	int	nb_params,
	int	*p_return_status ) 
{
	void	*p_ctx = NULL;
	
	p_ctx = (void *) xmltp_lex_get_single_thread_context();

	sf_write_error_to_log(ALOG_WARN, 0, "sf_kill_process(): STUB version!");

	xmltp_writer_end_response(p_ctx, 0, 0, 1);

} /* End of sf_kill_process */





/*
 * sf_process_db_login() STUB version:
 * 	. Log username and password...
 *	. Fake resultset with PID  and return status
 */
void sf_process_db_login (
	void	*tab_params[], 
	int	nb_params,
	int	*p_return_status ) 
{
	void	*p_ctx	= NULL;
	int	 ret_stat = 0;
	char	 msg_buff[500] = "[msg_buff??]";

	p_ctx = (void *) xmltp_lex_get_single_thread_context();

	sprintf(msg_buff, "login RPC received: %.40s, %.40s, %.40s.", 
			s_user_name,
			s_password,
			s_trg_name );

	sf_write_error_to_log(ALOG_INFO, 0, msg_buff);

	s_b_login_proc_done = 1;	/* after this, the values of s_user_name, s_password, s_trg_name will not change */

	ret_stat = 0;


	/* Fake xml result PID, return stat */
	/************************************/

	sprintf(s_msg_buff, "%ld", getpid());
	
	xmltp_writer_begin_result_set(p_ctx, 1, 0);

	xmltp_writer_column_name(p_ctx, "PID", 4, 0);

	xmltp_writer_column_value(p_ctx, 4, FALSE, s_msg_buff);

	xmltp_writer_end_colum_values(p_ctx);

	xmltp_writer_end_response(p_ctx, ret_stat, 0, 1);

	sf_write_error_to_log(ALOG_INFO, 0, "sent reply to 'login'.");

} /* End of sf_process_db_login() */



void sf_setwait(
	void	*tab_params[], 
	int	nb_params,
	int	*p_return_status ) 
{
	long			rc;
#ifdef CT10_DATA_XDEF_T
	CT10_DATA_XDEF_T        **pt_params;
#endif
	void			*p_ctx	= NULL;
	int			ret_stat=0;

	char			str_nb_secs[32] = "[???]";

#ifdef CT10_DATA_XDEF_T
	p_ctx = (void *) xmltp_lex_get_single_thread_context();

	pt_params = (CT10_DATA_XDEF_T**) tab_params;


	/* First param is username */
	/***************************/

	if ( (pt_params[0])->len > sizeof(str_nb_secs) ) {
		sprintf(s_msg_buff, "sf_setwait(): param[0] len [%ld] to long", 
			(pt_params[0])->len);
		sf_write_error_to_log(ALOG_ERROR, 0, s_msg_buff);
		xmltp_writer_end_response(p_ctx, ret_stat, 0, 0);	/* xxxxxxx */
		return;
	}
	memset( (void *) str_nb_secs, 0, sizeof(str_nb_secs) );

	strncpy(str_nb_secs, (char *) (pt_params[0])->p_val, sizeof(str_nb_secs) );

	g_wait_n_seconds = atoi(str_nb_secs);

	if (g_wait_n_seconds < 0 || g_wait_n_seconds > 1000) {
		sf_write_error_to_log(ALOG_INFO, g_wait_n_seconds, "sf_setwait(): g_wait_n_seconds set, but STRANGE.");
	} else {
		sf_write_error_to_log(ALOG_INFO, g_wait_n_seconds, "sf_setwait(): g_wait_n_seconds set!");
	}
#else
	sf_write_error_to_log(ALOG_INFO, g_wait_n_seconds, "sf_setwait(): not implemented because CT10_DATA_XDEF_T not defined.");
#endif
	xmltp_writer_end_response(p_ctx, ret_stat, 0, 0);

} /* End of sf_setwait() */




void sf_send_back_return_status(int ret_stat)
/* 
 * Called by:	sf_check_and_process_registered_proc()
 */
{
	void	*p_ctx	= NULL;

	p_ctx = (void*) xmltp_lex_get_single_thread_context();

	xmltp_writer_end_response(p_ctx, ret_stat, 0, 0);

} /* end of sf_send_back_return_status() */





static int sf_send_raw_xml(char *xml_buffer, int n_bytes)
/*
 * Called by:	sf_send_back_xml_response_from_file()
 *
 * send back an XML response
 */
{
	int	sd = -1;
	int	rc = 0,
		error_x = 0; 

	sd = xmltp_ctx_get_socket( (void *) xmltp_lex_get_single_thread_context() );
	if (sd <= -1) {
		return (-2);
	}

	rc = sock_send(sd, xml_buffer, n_bytes, 0, &error_x);
	
	if (rc <= -1) {
		sf_write_log_msg_mm(ALOG_WARN, error_x, "sock_send() failed", s_last_file_read );

		/* return (-111);
		 */
	}

	return (0);

} /* end of sf_send_raw_xml() */





static int sf_send_back_xml_response_from_file(char *file_name)
/*
 * Called by:	sf_get_xml_file_and_send_it_back()
 *
 * Check if the file is the one currently in the cache (the last one read).
 * If so, send back the content of the buffer.
 *
 * If not, open the file...
 * If the file cannot be opened, return -1 (will become the return status).
 *
 * Read the file in the buffer (the cache), send it back.
 *
 * NOTE: in the first version, the maximum file size is the size of that cache buffer (about 100 KB).
 *
 * If the file is too big, return -3.
 */
{
	int	fd = -1;
	ssize_t n_bytes = 0;
	int	rc = 0;
	
	if (!strcmp(s_last_file_read, file_name) && s_n_bytes_in_xml_buffer > 10) {
		rc = sf_send_raw_xml(s_xml_file_buffer, s_n_bytes_in_xml_buffer);	/* send back the response */
		return (rc);
	}

	s_n_bytes_in_xml_buffer = 0;	/* reset ctr of bytes in buffer to Zero */

	fd = open(file_name, O_RDONLY);
	if (-1 == fd) {
		sf_write_log_msg_mm(ALOG_INFO, errno, "Cannot open:", file_name);

		return (-1);	/* cannot read file */
	}

	n_bytes = read(fd, (void *) s_xml_file_buffer, sizeof(s_xml_file_buffer) );

	close(fd);	/* close the file Now -- we are done reading it, whether it succeeded or not */

	if (0 == n_bytes) {
		sf_write_log_msg_mm(ALOG_INFO, errno, "read() EOF on:", file_name);

		return (-2);	/* EOF */
	}
	if (n_bytes <= -1) {
		sf_write_log_msg_mm(ALOG_INFO, errno, "read() error on:", file_name);

		if (0 == errno) {
			return (-901);	/* unlikely */
		}
		return (0 - errno);	/* make errno negative and return it as return status */
	}

	/* update Cache information :
	 */
	strncpy(s_last_file_read, file_name, sizeof(s_last_file_read) );
	s_n_bytes_in_xml_buffer = n_bytes;

	if (n_bytes > 1 && '\n' == s_xml_file_buffer[n_bytes - 1]) {	/* is the last byte a '\n' ? */
		for ( ; n_bytes > 1; n_bytes--) {
			if (s_xml_file_buffer[n_bytes - 1] != '\n') {
				break;
			}
		}
		s_n_bytes_in_xml_buffer = n_bytes;
		sf_write_log_msg_mm(ALOG_INFO, n_bytes, "Removed '\\n' at end of xml_buffer for:", file_name);
	}

	rc = sf_send_raw_xml(s_xml_file_buffer, n_bytes);	/* send back the response */
	
	return (rc);

} /* end of sf_send_back_xml_response_from_file() */




static int sf_get_xml_file_and_send_it_back(char *proc_name)
/*
 * Called by:	xml2db_exec_proc()
 *
 * Check if proc_name is in the tale s_v_list_of_RPC_names[]
 * If so, read the file and sends back its content (it should be XML!).
 * If the file cannot be opened, return -1 (will become the return status).
 * If the proc_name is not in the table return RPC_RETURN_STAT_FILE_NOT_ALLOWED
 */
{
	int	i = 0;

	for (i=0; i < s_nb_rpc_allowed; i++) {
		if (NULL == s_p_v_list_of_RPC_allowed[i]) {	/* NULL indicates the end of the table */
			break;
		}
		if (!strcmp(s_p_v_list_of_RPC_allowed[i], proc_name)) {
			return (sf_send_back_xml_response_from_file(proc_name) );
		}
	}
	return (RPC_RETURN_STAT_FILE_NOT_ALLOWED);

} /* end of sf_get_xml_file_and_send_it_back() */




/*
 * Called by:	xml2db_exec_proc()
 *
 * Look for registered procedure in xmlfisrv and execute it.
 */
static int sf_check_and_process_registered_proc (
			char	*proc_name, 
			void	*tab_params[], 
			int	nb_params,
			int	*p_return_status )  {

	if (s_debug_level >= DEBUG_TRACE_LEVEL_LOW) {
		sprintf(s_msg_buff,"sf_check_and_process_registered_proc: proc_name[%.30s]", proc_name);
		sf_write_error_to_log(ALOG_INFO, 0, s_msg_buff);
	}

	if (strcmp(proc_name, REG_PROC_DB_LOGIN) == 0) {

		sf_process_db_login (	tab_params, 
					nb_params,
					p_return_status );

		return 1;

	} else if (strcmp(proc_name, REG_PROC_XML2DB_DEBUG_LOW) == 0) {

		xmltp_ctx_assign_debug_level( 1 );	/* 1 is low */

		sf_write_error_to_log(ALOG_INFO, 0, proc_name);

		sf_send_back_return_status( 0 );
		return 1;

	} else if (strcmp(proc_name, REG_PROC_XML2DB_DEBUG_FULL) == 0) {

		xmltp_ctx_assign_debug_level( 10 );	/* 10 is Full */

		sf_write_error_to_log(ALOG_INFO, 0, proc_name);

		sf_send_back_return_status( 0 );
		return 1;


	} else if (strcmp(proc_name, REG_PROC_XML2DB_PING) == 0) {	/* this "ping" write an entry in the log */

		sf_reset_static_values();	/* set begin timestamp */

		s_rpc_return_status = 0;

		sf_send_back_return_status( 0 );

		sf_log_rpc(proc_name);		/* write elapsed time (ms) in  the (server).log */

		return 1;

	} else if (strcmp(proc_name, REG_PROC_PING) == 0) {	/* this "ping" does NOT have timestamp or logging */

		s_rpc_return_status = 0;

		sf_send_back_return_status( 0 );

		return 1;

	} else if (strcmp(proc_name, REG_PROC_KILL) == 0) {

		sf_kill_process (	tab_params, 
					nb_params,
					p_return_status );

		return 1;

	} else if (strcmp(proc_name, REG_PROC_SETWAIT) == 0) {

		sf_setwait (	tab_params, 
				nb_params,
				p_return_status );

		return 1;
	}

	return 0;

} /* End of sf_check_and_process_registered_proc() */






int xml2db_exec_proc(void *p_conn, char *proc_name, 
		void *tab_params[], int nb_params,
		int  *p_return_status)
/*
 * EMULATE the database procedure proc_name by reading a file with the same name
 * and returning its content (which must be XMLTP XML content).
 *
 * First the proc_name is checked to see if it is a regproc name or not...
 *
 * Each parameter pointers to by the pointers in tab_params must have been
 * created with xml2db_create_param_struct().
 *
 * Return:
 *	0	OK
 */
{
	CS_INT	 return_status	= 0;
	int	 rc		= 0;
	int	 retry_count	= 0;

	sf_reset_static_values();

	xmltp_writer_reset( (void*) xmltp_lex_get_single_thread_context() );

	/* Here we process registerd procedure            */
	/* Return 1 if the proc_name is a registered proc */
	/* If so we get out of here, the function that    */
	/* handle the Reg Proc will fake the necessery    */
 	/* result. 					  */

	if ( sf_check_and_process_registered_proc (
						proc_name, 
						tab_params, 
						nb_params,
						p_return_status) != 0 ) {

		return 0;
	}

	if (g_wait_n_seconds > 0) {
		sf_write_log_msg_mm(ALOG_INFO, g_wait_n_seconds, "waiting (n) seconds...", "");
		sleep(g_wait_n_seconds);
	}

	/*
	 * General case: emulate the response to a RPC call by returning the content of a file:
	 */
	rc = sf_get_xml_file_and_send_it_back(proc_name);

	s_rpc_return_status = rc;

	sf_log_rpc(proc_name); 		/* write the xmltpsrv.log */
	
	if (rc != 0) {	/* if rc != 0, then rc is the return_status value and we have to complete the XML reponse: */

		xmltp_writer_end_response( (void*) xmltp_lex_get_single_thread_context(), rc, 0, 0);

	} else {	/* else a full XML response has been sent, we must NOT send anything more! */

		return 0;
	}

	return (0);

} /* end of xml2db_exec_proc() */


/* ----------------------------------- Child process main loop: */


static int sf_process_client_connection(int child_socket)
{
	void	*p_ctx = NULL;
	int	 rc    = 0,
		 ctr_errs = 0,
		 b_found_eot;

	p_ctx = (void *) xmltp_lex_get_single_thread_context();

	rc = xmltp_ctx_assign_socket_fd(p_ctx, child_socket);
	if (rc != 0) {
		sf_write_error_to_log(ALOG_ERROR, rc,
			 "ABORTING: cannot assign socket into parser context");
		return (55);
	}
		
	sf_write_log_msg_mm(ALOG_INFO, child_socket,
				"Ready to parse from socket connection.",
				"");

	/* s_abort_flag could be set by xml2db_abort_program()
	 */
	while (!s_abort_flag) {

		if ( (rc = xmltp_ctx_reset_lexer_context(p_ctx)) != 0) {
			return (5);
		}

		/* NOTE: The functions called in the actions of the parser
		 * will do callbacks to various functions, including the
		 * xml2db_xxx() functions here.
		 */
		rc = yyparse(p_ctx);	/* the re-entrant parser generated by Bison 
					 * needs the p_ctx. (jft, 2002jan08)
					 */

		b_found_eot = xmltp_ctx_buffer_contains_eot(p_ctx);

		if (xmltp_ctx_get_b_eof_disconnect(p_ctx) ) {
			rc = 1;		/* force rc to indicate EOF, disconnect */

			if (s_debug_level >= DEBUG_TRACE_LEVEL_LOW) {
				sf_write_log_msg_mm(ALOG_INFO, 0, "Disconnect detected.", "");
			}
		} else if ( ( ctr_errs = xmltp_ctx_get_lexer_or_parser_errors_ctr(p_ctx)) != 0) {

			sprintf(s_msg_buff,"yyparse failed: ctr_err[%d], yyparse rc[%d], b_found_eot[%d], <EOT/>[%s], p_ctx[%x]",
				ctr_errs, rc, b_found_eot,
				xmltp_ctx_get_b_eof_disconnect(p_ctx) ? "Disc/EOF" : "No Disc/EOF",
				p_ctx);
			sf_write_log_msg_mm(ALOG_ERROR, 0, s_msg_buff, "");
		}

		switch (rc) {
		case 0:
			/* sf_write_log_msg_mm(ALOG_INFO, 0,
				"Request parsed completely without error.",
				""); */
			break;
		case 1:
		case 101:	/* EOF ?? */
			sf_write_log_msg_mm(ALOG_INFO, rc,
				"EOF in parser. This child process is stopping.",
				"");
			return (0);
		default:
			ctr_errs++;
			if (ctr_errs >= 5) {
				sf_write_error_to_log(ALOG_ERROR, rc,
				 "*** Too many XML-TP parse errors ABORTING ***");
				return (60);
			}
			sleep(1);	/* to avoid a hard loop */
		}
	} 

	sf_write_log_msg_mm(ALOG_WARN, getpid(),
				"XML-to-Sybase connection closing.",
				(s_abort_flag) ? "Abort flag set." : "(see reason before).");

	return (0);

} /* end of sf_process_client_connection() */



/* --------------------------------------------------------- Main(): */


int main(int argc, char **argv)
{
	int	 rc = 0;
	int	 i  = 0;

	int	listen_socket	= -1,
		child_socket	= -1;

	pid_t	pid_child	= 0;

	char	msg_buff[200] = "[ms_buff?]";


	/* the only arg expected is the port number to listen on:
	 */
	if (argc < 2) {
        	sf_syntax(s_pgm_name);
        	exit(1);
	}
	strncpy(s_listen_port, argv[1], sizeof(s_listen_port) );
	if (argc >= 3) {
		s_nb_rpc_allowed = argc - 2;
		s_p_v_list_of_RPC_allowed = &argv[2];
		printf("%d RPC names accepted:\n", s_nb_rpc_allowed);
		for (i=0; i < s_nb_rpc_allowed; i++) {
			printf("%.255s\n", s_p_v_list_of_RPC_allowed[i] );
		}
		printf("***\n");
	}
	printf("will listen on port %s ...\n", s_listen_port);

	/* START the LOG:
	 */
	rc = alog_set_log_filename(LOG_FILENAME);


	/* We need to log the pid not the parent pid (which is the default of aplogmsg):
	 */
	(void) alog_set_log_parent_pid_to_false();


	/* apl_log1.c need a table to decode the log level values to strings:
	 */
	alog_bind_message_types_table(&s_type_strings_table[0], sizeof(s_type_strings_table) );


	xmltp_ctx_assign_log_function_pf( sf_write_log_msg_mm);


	listen_socket = sf_init_master_socket_listen();
	if (listen_socket <= -1) {
		exit(3);
	}

	signal( SIGCHLD, SIG_IGN );	/* maybe could be trapped & processed */
	signal( SIGPIPE, SIG_IGN );	/* happens the peer closes the socket */

	
	while (1) {
		child_socket = sock_accept_connection ( listen_socket );

		if ( child_socket <= -1 ) {
			if ( errno == EINTR ) {
				continue;
			}
			/* Connection problem:
			 */
			sf_write_log_msg_mm(ALOG_ERROR, errno,
				"MASTER LISTERNER ABORTING. sock_accept_connection() failed:",
				(char *) sys_errlist[errno] );
			
			exit(1);
		}

		pid_child = fork();	/* --------- fork() --------- */

		switch ( pid_child ) {

		case 0:				/* Child */
			close( listen_socket );
			sf_write_log_msg_mm(ALOG_INFO, 0, "Child starting.",
						"close() master socket done." );
			
			return (sf_process_client_connection(child_socket));

		case -1	:			/* fork() ERROR */
			sf_write_log_msg_mm(ALOG_ERROR, errno,
				"FAILED. fork() failed. client socket closed.",
				(char *) sys_errlist[errno] );
		
			close( child_socket );
			return( 1 );

		default	:			/* Parent */
			close( child_socket );

			sprintf(msg_buff, "pid_child=%d", pid_child);

			sf_write_log_msg_mm(ALOG_INFO, pid_child,
				"Parent has one more child process. ",
				msg_buff);
			break;
		}
	}
	return (0);

} /* end of main() */


/* -------------------------------------- end of xmlfisrv.c ----------- */
