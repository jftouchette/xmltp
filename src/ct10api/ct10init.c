/*****************************************************
 ct10init.c

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

 LIST OF CHANGES:
 ================

 1997-08-19, blanchjj:	Initial Creation.
 2000jan03,deb: Changed s_passwd for passwd so it does not use a
                "private" implementation dependent structure in
                ct10init_getlogin()
 2001dec21,jbl: Remove a compilation warning.
 2003dec16,jbl: ALOG_INFO in log msg on lines 483 and 494.

 *****************************************************/

#include <stdio.h>
#include <ctpublic.h>
#include <string.h>
#include <unistd.h>


#include "common_1.h"
#include "parse_argument.h"
#include "opt_log.h"
#include "ct10api.h"
#include "ct10reco.h"
#include "ct10init.h"

#ifdef WIN32
//#include <winsock2.h>
#include "ntunixfn.h"
#endif


#define _INCLUDE_HPUX_SOURCE
#include <pwd.h>


static char	buf[OPT_LOG_BUFFER_LEN];
static int	rc;

static  char	*ct10init_version_id = CT10INIT_VERSION_ID;



/****************************************************************************
 Get module version information string.
 ****************************************************************************/

char *ct10init_get_version_info()
{
	return ct10init_version_id;
}




#if 0

/****************************************************************************
 Syntax display function.

 This is only an example  that is not use. Copy in the main code and add
 application specific option.

 ****************************************************************************/

void ct10init_syntax(char *prog)
{

	fprintf(stderr,"\n");
	fprintf(stderr,"Syntax: %s <options>...\n",prog);
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
	fprintf(stderr,"       -D <Debug mode bit mask>\n");
	fprintf(stderr,"       -L <Log file path and name>\n");
	fprintf(stderr,"       -p <Dont ask for password file>\n");
	fprintf(stderr,"\n");

	exit(1);

} /* ------------- End of ct10init_syntax -------------------------- */

#endif

/****************************************************************************
 Get current system hostname.
 ****************************************************************************/

int ct10init_get_host_name(char *hostname, int size)
{

if (gethostname(hostname,size) != 0) {
	return CT10INIT_FAIL;
}

return CT10INIT_SUCCESS;

} /* --------------------- End of ct10init_get_host_name ---------------- */



/****************************************************************************
 Get current user name.
 ****************************************************************************/

int ct10init_getlogin(char *login, int size)
{
char *p_login;
struct passwd *p_s_passwd;

if ((p_login = (char *) getlogin()) == NULL) {

	if ((p_s_passwd = (void *) getpwuid(getuid())) == NULL) {
		return CT10INIT_FAIL;
	}
	else {
		p_login = p_s_passwd->pw_name;
	}
}

strncpy(login, p_login, size);

return CT10INIT_SUCCESS;

} /* ------------------- End of ct10init_getlogin -------------------- */



/****************************************************************************
 If an option is not found on the command line the value is read from
 the config file.
 ****************************************************************************/

char *ct10init_get_option_or_config_value(	char option_letter, 
						char *config_file_string)
{

/****
sprintf(buf,"ct10init_get_option_or_config_value %c, %s",
		option_letter,
		config_file_string);
rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
			ALOG_INFO,
			0,
			buf,
			DEBUG_TRACE_MODE_CT10INIT_CALL);
****/


/* First we look for an option on the command line */
if (pa_get_flag(option_letter)) {

	return  pa_get_arg(option_letter);
}

/* If not, we get the corresponding value from the config file. */
else {
	return (char *) ucfg_clt_get_param_string_first(config_file_string);
}

} /* ----------- End of ct10init_get_option_or_config_value -------------- */



/****************************************************************************
 if a flag is set as a command line argument the value true is returne if
 not the config file is check for an entry if found true is return else
 false is return.
 ****************************************************************************/

int ct10init_get_flag_or_config_value(	char option_letter, 
					char *config_file_string)
{

/* First we look for an option on the command line */
if (pa_get_flag(option_letter)) {

	return  TRUE;
}

/* If not, we get the corresponding value from the config file. */
else {
	if (ucfg_is_string_in_param_values_list_ext(
		config_file_string, "", 0) == -1) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

} /* ----------- End of ct10init_get_flag_or_config_value -------------- */



/****************************************************************************
 Extract info from config file for all dynarray initialization.

int ct10init_get_dynarray_init_info(	char  *suffix_string,
					long  *initial_size,
					long  *extend_size,
					long  *nb_extend)
{
char tmp_buf[255];
char *value;


sprintf(buf,"ct10init_get_dynarray_init_info [%s]",
		suffix_string);
rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
			ALOG_INFO,
			0,
			buf,
			DEBUG_TRACE_MODE_CT10INIT_CALL);


strncpy(tmp_buf,CT10INIT_CONFIG_STRING_DYNARRAY_INITIAL_SIZE,80);
strncat(tmp_buf,suffix_string,80);

if ((value = (char *) ucfg_clt_get_param_string_first(tmp_buf)) != NULL) {
	*initial_size = atol(value);
}
else {
	sprintf(buf,"ct10init_get_dynarray_init_info Error reading cfg for %s",
			CT10INIT_CONFIG_STRING_DYNARRAY_INITIAL_SIZE);
	rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
				ALOG_ERROR,
				0,
				buf,
				0);

	return CT10INIT_FAIL;

}



strncpy(tmp_buf,CT10INIT_CONFIG_STRING_DYNARRAY_NB_EXTEND,80);
strncat(tmp_buf,suffix_string,80);

if ((value = (char *) ucfg_clt_get_param_string_first(tmp_buf)) != NULL) {
	*nb_extend = atol(value);
}
else {
	if ((value = (char *) ucfg_clt_get_param_string_first(
			CT10INIT_CONFIG_STRING_DEFAULT_NB_EXTEND)) != NULL) {
		*nb_extend = atol(value);
	}
	else {

	sprintf(buf,"ct10init_get_dynarray_init_info Error reading cfg for %s",
			CT10INIT_CONFIG_STRING_DEFAULT_NB_EXTEND);
	rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
				ALOG_ERROR,
				0,
				buf,
				0);

	return CT10INIT_FAIL;
	}

}



strncpy(tmp_buf,CT10INIT_CONFIG_STRING_DYNARRAY_EXTEND_SIZE,80);
strncat(tmp_buf,suffix_string,80);

if ((value = (char *) ucfg_clt_get_param_string_first(tmp_buf)) != NULL) {
	*extend_size = atol(value);
}
else {

	sprintf(buf,"ct10init_get_dynarray_init_info Error reading cfg for %s",
			CT10INIT_CONFIG_STRING_DYNARRAY_EXTEND_SIZE);
	rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
				ALOG_ERROR,
				0,
				buf,
				0);

	return CT10INIT_FAIL;

}

sprintf(buf,"ct10init_get_dynarray_init_info rc:%ld [%s] [%ld] [%ld] [%ld]",
		CT10INIT_SUCCESS,
		suffix_string,
		*initial_size,
		*extend_size,
		*nb_extend);
rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
			ALOG_INFO,
			0,
			buf,
			DEBUG_TRACE_MODE_CT10INIT_CALL);

return CT10INIT_SUCCESS;

}  ------------- End of ct10init_get_dynarray_init_info --------------- ****/



/****************************************************************************
 Standerd ct10api initialization function.
 ****************************************************************************/

int ct10init_standard_ct10api_init(
	void	**ct10_connect,
	char	*arg_string,
	int	ac,
	char	*av[],
	char	*config_section,
	int	config_prefix_ind,
	char	*module_for_log,
	char	*p_appl_name)
{

/* Variables use by argument parsing */
char *prog;             /* Name of the program to print with the syntax */
extern int optind;      /* To be used by getopts */

char *log_file_name;
char *user_name;
char *passwd;
char *passwd_file;
char *appl_name;
char *server_name;
char *init_string;
char *config_file_section;
char *config_prefix;

char hostname[S_HOSTNAME+1];
char login_name[S_LOGIN_NAME+1];

char *debug_mode;

char p_default_appl_name[]={CT10INIT_DEFAULT_APPL_NAME};



/****************************************************************************
 Parsing command line arguments.
 ****************************************************************************/

prog = av[0];           /* av[0] contain the program name. */ 

if ((rc = pa_parse_argument(ac,av,arg_string)) != 0) {
	return CT10INIT_INVALID_ARGUMENT;
}


/****************************************************************************
 Mandatory command line arguments validation.
 ****************************************************************************/


if (!pa_get_flag(CT10INIT_ARG_CONFIG_FILE)) {
	fprintf(stderr,"\nOption %c is require\n",CT10INIT_ARG_CONFIG_FILE);
	return CT10INIT_INVALID_ARGUMENT;
}

/****************************************************************************
 If config section is given as a command line argument dont use the
 application default.
 ****************************************************************************/


if (pa_get_flag(CT10INIT_ARG_CONFIG_FILE_SECTION)) {

	config_file_section = pa_get_arg(CT10INIT_ARG_CONFIG_FILE_SECTION);

	if (config_file_section == NULL) {
		fprintf(stderr,"\nNULL config file section.\n");
		return CT10INIT_INVALID_ARGUMENT;
	}
}
else {
	if (config_section == NULL) {
		fprintf(stderr,"\nNULL default config file section.\n");
		return CT10INIT_INVALID_ARGUMENT;
	}
	else {
		config_file_section = config_section;
	}
}


/****************************************************************************
 The first thing to do is to read the config file.
 ****************************************************************************/

if (pa_get_arg(CT10INIT_ARG_CONFIG_FILE) != NULL) {

	if ((rc=ucfg_clt_open_cfg_at_section(
				pa_get_arg(CT10INIT_ARG_CONFIG_FILE),
				config_file_section)) != 0) {
		fprintf(stderr,"\nError reading config file '%s','%s'.\n",
			pa_get_arg(CT10INIT_ARG_CONFIG_FILE),
			config_file_section);
		return CT10INIT_INVALID_ARGUMENT;
	}

}
else {
	fprintf(stderr,"\nInvalid config file name.\n");
	return CT10INIT_INVALID_ARGUMENT;
}


/****************************************************************************
 From here we need the login name to initialize the log. Why not to get
 the hostname at the same time.
 ****************************************************************************/

 if ((rc = ct10init_get_host_name(hostname,S_HOSTNAME)) != CT10INIT_SUCCESS) {
	fprintf(stderr,"\nError reading hostname.\n");
	return CT10INIT_FAIL;
 }


 if ((rc = ct10init_getlogin(login_name,S_LOGIN_NAME)) != CT10INIT_SUCCESS) {
	fprintf(stderr,"\nError reading login name.\n");
	return CT10INIT_FAIL;
 }



/****************************************************************************
 Next we must try to initialize the log.
 ****************************************************************************/


log_file_name = ct10init_get_option_or_config_value(
			CT10INIT_ARG_LOG_FILE,
			CT10INIT_CONFIG_STRING_LOG_FILENAME);

if (log_file_name != NULL) {
	/* Return 0 for good or bad log file. if initialization fail here */
	/* All message send to log will be send to stdout                 */

	rc = opt_log_init_log(log_file_name, module_for_log, login_name); 	
}
else {
	fprintf(stderr,"\nInvalid log file name.\n");
	return CT10INIT_INVALID_ARGUMENT;
}



/****************************************************************************
 		From here all messages will be write to the log.
 ****************************************************************************/



/****************************************************************************
 Set default debug trace mode.
 ****************************************************************************/

debug_mode = ct10init_get_option_or_config_value(
			CT10INIT_ARG_DEBUG_MODE,
			CT10INIT_CONFIG_STRING_DEBUG_MODE);

if (debug_mode == NULL) {
		rc=opt_log_set_trace_bit_mask( 0 );
}
else {

		rc=opt_log_set_trace_bit_mask( atoi(debug_mode) );
		sprintf(buf,"New debug trace mode: rc: %ld, bit mask: %x",
				rc, atoi(debug_mode) );
		rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
					ALOG_INFO,
					0,
					buf,
					0);
}

/****************************************************************************
 Logging modules version information.
 ****************************************************************************/

		rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
					ALOG_INFO,
					0,
					ct10init_get_version_info(),
					0);


/****************************************************************************
 Parsing arguments needed for connection to SQL server.
 ****************************************************************************/


/****************************************************************************
 			SQL server NAME
 ****************************************************************************/

server_name = ct10init_get_option_or_config_value(
			CT10INIT_ARG_SERVER_NAME,
			CT10INIT_CONFIG_STRING_SERVER_NAME);

if (server_name == NULL) {
		sprintf(buf,"Invalid server name");
		rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
					ALOG_ERROR,
					0,
					buf,
					0);
		fprintf(stderr,"\n%s\n",buf);
		return CT10INIT_INVALID_ARGUMENT;
}


/****************************************************************************
 		application NAME to use with SQL server
 ****************************************************************************/

if (p_appl_name == NULL) {
	appl_name = ct10init_get_option_or_config_value(
				CT10INIT_ARG_APPL_NAME,
				CT10INIT_CONFIG_STRING_APPL_NAME);

	if (appl_name == NULL) {
		appl_name =  (char *) p_default_appl_name;
	}
}
else {
	appl_name = p_appl_name;
}


/****************************************************************************
 		init string to execute after the connection 
 ****************************************************************************/

init_string = ct10init_get_option_or_config_value(
			CT10INIT_ARG_INIT_STRING,
			CT10INIT_CONFIG_STRING_INIT_STRING);


/****************************************************************************
 			USER NAME
 ****************************************************************************/

user_name = ct10init_get_option_or_config_value(
			CT10INIT_ARG_USER_NAME,
			CT10INIT_CONFIG_STRING_USER_NAME);

if (user_name == NULL) {
		sprintf(buf,"Invalid user name");
		rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
					ALOG_ERROR,
					0,
					buf,
					0);
		fprintf(stderr,"\n%s\n",buf);
		return CT10INIT_INVALID_ARGUMENT;
}


/****************************************************************************
 Logging SQL server connection info.	
 ****************************************************************************/

 sprintf(buf,"Server: %s, Appl: %s, User: %s, Init: %s",
		server_name, appl_name, user_name,init_string);
 rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
			ALOG_APP_WARN,
			0,
			buf,
			0);


/****************************************************************************
 			SQL server password
 ****************************************************************************/

if (pa_get_flag(CT10INIT_ARG_PASSWORD_FILENAME)) {

	passwd_file =  (char *) pa_get_arg(CT10INIT_ARG_PASSWORD_FILENAME);
}
else {
	passwd_file = (char *) ucfg_clt_get_param_string_first(
				CT10INIT_CONFIG_STRING_PASSWORD_FILENAME);
}

if (passwd_file != NULL) {
	passwd = (char *) g1ln_get_first_line_of_file(passwd_file);
}
else {
	passwd = (char *) getpass("Password:");
	if (passwd == NULL) {
		sprintf(buf,"Invalid password");
		rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
					ALOG_ERROR,
					0,
					buf,
					0);
		fprintf(stderr,"\n%s\n",buf);
		return CT10INIT_INVALID_ARGUMENT;
	}
}



/****************************************************************************
 Set config module prefix.
 ****************************************************************************/

 if (config_prefix_ind == CT10INIT_CONFIG_PREFIX_IND_SERVER) {
 	ucfg_clt_set_prefix( server_name, strlen(server_name) );
 }
 else if (config_prefix_ind == CT10INIT_CONFIG_PREFIX_IND_APPL_NAME) {
 	ucfg_clt_set_prefix( appl_name, strlen(appl_name) );
 }

/****************************************************************************
 Pass module name to ct10api and ct10reco.
 ****************************************************************************/

ct10api_assign_application_name(module_for_log);



/****************************************************************************
 Reconfiguration of master recovery table of ct10reco module from the
 config file.
 ****************************************************************************/

if ((rc = ct10reco_reconfigure_master_recov_table_from_config_params()) != 0) { 

	sprintf(buf,"Reconfig of ct10reco fail, RC: %d", rc);
	rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
				ALOG_ERROR,
				0,
				buf,
				0);
	return CT10INIT_FAIL;
}


/****************************************************************************
 Creating CT10API connection.
 ****************************************************************************/

if ((rc=ct10api_init_cs_context()) !=  CT10_SUCCESS) {

	sprintf(buf,"ct10api_init_cs_context fail, RC: %d", rc);
	rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
				ALOG_ERROR,
				0,
				buf,
				0);
	return CT10INIT_FAIL;
}


if ((*ct10_connect=ct10api_create_connection(user_name, passwd,
				  appl_name, hostname, server_name,
				  init_string, (long) NULL)) == NULL) {

	sprintf(buf,"ct10api_create_connection fail, %x, %x",
			ct10api_get_diagnostic_mask(*ct10_connect),
			ct10api_get_diag_sev_mask(*ct10_connect)   );

	rc = opt_log_aplogmsg(	CT10INIT_MODULE, 
				ALOG_ERROR,
				0,
				buf,
				0);
	return CT10INIT_FAIL;

}



return CT10INIT_SUCCESS;

} /* -------------- End of ct10init_standard_ct10api_init ---------------- */
