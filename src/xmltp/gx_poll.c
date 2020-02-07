/* gx_poll.c
 * ----------
 *
 *  Copyright (c) 2001-2003 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/xmltp/gx_poll.c,v $
 * $Header: /ext_hd/cvs/xmltp/gx_poll.c,v 1.9 2004/10/13 18:23:43 blanchjj Exp $
 *
 *
 * *** gx_poll.c:  poll() module for GX (Gateway XMLTP)
 *
 *
 * Python/C wrapper functions to use  poll()
 *
 *
 * Warnings and limitations:
 * (1) There is a possibility of a DoS attack. The module might receives so many
 *     connections that the poll_tab[] becomes full. And then, the calling Python script
 *     would not be able to add back one or more fd's to the poll_tab[] even though
 *     "older" fd should have more priority. There are two solutions to this.
 *	(A) keep a reserved number of entries in the poll_tab[] (best solution,
 *	    but more complex to implement well: issues of sizing the right number
 *	    reserved, maybe dynamically, and, of doing proper clean-up)
 *	(B) let the calling Python script keep a list of fd's which could not be added
 *	    back into the list. This can be the preferred solution for a first
 *	    version. (jft,2001nov06)
 *
 * --------------------------------------------------------------------------
 * 2001oct22,jft: first version
 * 2002jun10,jft: . gx_poll_poll_clients(): sf_process_drop_list(pyObj_dropList) first, add after. This is safer.
 * 2002jul21,jft: . gx_poll_get_poll_tab(): fixed memory leak: Py_XDECREF(pyObj_activeList);
 *		  . gx_poll_poll_clients(): fixed memory leak: Py_XDECREF() on both lists returned in tuple.
 * 2002sep10,jft: . gx_poll_init_listen(): now use sock_socket_bind_and_listen_ext() with arg b_so_reuseaddr = 1
 * 2003jan29,jft: + static int sf_compress_poll_tab(char *msg, int note_last_error_level)*
 * 		  . BUG FIX: gx_poll_poll_clients(): when poll() fails with EINVAL, call sf_compress_poll_tab()
 *			Fixes the fatal error: "poll() failed   Invalid argument (errno=22)" (s_high_ctr_poll_fd_used >= 1025)
 * 2003feb07,jft: . gx_poll_get_poll_tab(): if more than 100 inactive entries in poll_tab, call sf_compress_poll_tab()
 *		  . sf_reuse_old_poll_fd(): call sf_note_last_error(200, ...) to show value of s_position_first_fd_free
 *		  . sf_process_drop_list(): updates s_high_ctr_poll_fd_used
 *		  . sf_compress_poll_tab(): updates s_high_ctr_poll_fd_used, s_position_first_fd_free
 * 2003feb08,jft: . sf_compress_poll_tab(): fix ctr_used, and, s_position_first_fd_free = s_high_ctr_poll_fd_used;
 *		  . sf_note_last_error(): now does s_last_err_priority = priority;
 * 2003feb09,jft: . gx_poll_poll_clients(): if no activity and gx_poll_get_last_err_priority() > 200, return a tuple (err_priority, err_msg)
 * 2003feb10,jft: . gx_poll_poll_clients(): now only two (2) args: timeout_ms, addList (no more dropList !)
 *		  - removed sf_process_drop_list() and other related obsolete functions.
 * 2004feb16,jft: . sf_decode_revents(): put some statements within #ifdef POLLRDBAND ... to make it more portable.
 */

#include "common_1.h"	/* #include several needed headers */

#include <time.h>
#include <signal.h>

#ifdef LINUX
#define __USE_XOPEN	1
#define __USE_GNU	1
#endif
#include <poll.h>

#include <sys/socket.h>

#include "sock_fn.h"

#include "Python.h"


/* ------------------------------------ Things not found in #include :  */

#ifdef HPUX
extern	char	*sys_errlist[];
#endif

#define RELEASE_VERSION_STRING	"1.1.0"

#define RCS_VERSION_ID		"RCS $Header: /ext_hd/cvs/xmltp/gx_poll.c,v 1.9 2004/10/13 18:23:43 blanchjj Exp $"

#define FILE_REL_DATE_TIME_STRING	(__FILE__ " rel. " RELEASE_VERSION_STRING " : " __DATE__ " - " __TIME__)
#define COMPILED_VERSION_ID	(FILE_REL_DATE_TIME_STRING)



/* ------------------------------- Prototypes of functions defined below: */

PyObject *gx_poll_get_version_id(PyObject *self, PyObject *args);
PyObject *gx_poll_init_listen(PyObject *self, PyObject *args);
PyObject *gx_poll_poll_clients(PyObject *self, PyObject *args);
PyObject *gx_poll_get_poll_tab(PyObject *self, PyObject *args);


/* ----------------------------------------- Python "glue" tricks: */

/* Method table mapping names to wrappers:
 */
static PyMethodDef gx_poll_methods[] = {
	{ "getVersionId",  gx_poll_get_version_id,  METH_VARARGS },
	{ "initListen",  gx_poll_init_listen,  METH_VARARGS },
	{ "pollClients", gx_poll_poll_clients, METH_VARARGS },
	{ "getPollTab",  gx_poll_get_poll_tab, METH_VARARGS },
	{ NULL, NULL }
};

/* Module initialization function:
 */
initgx_poll(void) {
	Py_InitModule("gx_poll", gx_poll_methods);
}



/* -------------------------------------------------- Private Defines: */


#define DEFAULT_DEBUG		5	/* to DEBUG */

#define LISTEN_QUEUE_LEN	10	/* we want to accept() many */
					/* connections */

#define POLLFD_TABLE_SZ		5000	/* max number of incoming connections */


/* -------------------------------------------------- Private variables: */

static struct pollfd	s_poll_tab[POLLFD_TABLE_SZ];


static int	 s_high_ctr_poll_fd_used = 0,	/* ctr of max fd once used in s_poll_tab[] */
		 s_position_first_fd_free = 1;

static int	 s_ctr_closed_pollhup = 0;	/* updated by sf_event_handler_client_connection() */

static char	s_last_err_msg[2000] = "[no error]";
static time_t	s_last_err_time	    = 0;
static int	s_last_err_priority = 0;


static char	*s_pgm_name	 = __FILE__;

static int	 s_debug_mode	 = DEFAULT_DEBUG;

static long	 s_total_ctr_req  = 0L,
		 s_total_ctr_recv = 0L;



/* ---------------------------- Private functions: (UTILITY or LOW-LEVEL) */


static char *sf_get_version_id(int b_rcs_version_id)
/*
 * Called by:	gx_poll_get_version_id()
 */
{
	if (b_rcs_version_id) {
		return (RCS_VERSION_ID);
	}
	return (COMPILED_VERSION_ID);
}


/* ---------------------------- functions about last error message noted: */

void gx_poll_reset_last_err_msg()
{
	s_last_err_priority = 0;
	strcpy(s_last_err_msg, "[no error]");
	s_last_err_time	    = 0;
}

int gx_poll_get_last_err_priority()
{
	return (s_last_err_priority);
}


time_t gx_poll_get_last_err_time()
{
	return (s_last_err_time);
}


char *gx_poll_get_last_err_msg()
{
	return (s_last_err_msg);
}


static void sf_note_last_error(int priority, char *msg1, char *msg2, char *msg3)
{
	if (priority < s_last_err_priority) {	/* keep the last most important msg */
		return;
	}
	
	s_last_err_priority = priority;

	if (msg1 == NULL) {
		msg1 = "[null msg1]";
	}
	if (msg2 == NULL) {
		msg2 = "";
	}
	if (msg3 == NULL) {
		msg3 = "";
	}

	time( &s_last_err_time);

	sprintf(s_last_err_msg, "%.90s %.90s %.90s %.90s (errno=%d).",
			msg1, msg2, msg3,
			(errno != 0) ? sys_errlist[errno] : "",
			errno);

} /* end of sf_note_last_error() */




static char *sf_int_to_string(int n)
{
 static	char	answer[30] = "euh?";

	sprintf(answer, "%d", n);

	return (answer);

} /* end of sf_int_to_string() */







/* ----------------------------------- Private Functions: (MIDDLE LEVEL) */



static int sf_add_to_fd_list(PyObject *pyObj_List, int fd)
/*
 * Called by:	gx_poll_get_poll_tab()
 */
{
	PyObject *pyObj_int_sd  = NULL;
	int	  rc = 0;

	if (NULL == pyObj_List) {
		return (0);	/* OK, possible */
	}

	/*
	 * Add  fd  to  pyObj_List:
	 *
	 * first, convert fd to a Python int...
	 */
	pyObj_int_sd = Py_BuildValue("i", fd);

	rc = PyList_Append(pyObj_List, pyObj_int_sd);
	if (-1 == rc) {
		return (-1);
	}
	/* because the ref count was incremented by add to list,
	 * we decrement the ref count here:
	 */
	Py_XDECREF(pyObj_int_sd);

	return (0);

} /* end of sf_add_to_fd_list() */



static char *sf_decode_revents(int rep_events)
{
static	char	 msg[90] = "msg?";

	if (rep_events & POLLNVAL){
		return ("POLLNVAL");
	} else if (rep_events & POLLERR){
		return ("POLLERR");
	} else if (rep_events & POLLHUP){
		return ("POLLHUP");
	} else if (rep_events & POLLIN) {
		return ("POLLIN");
	} else if (rep_events & POLLPRI) {
		return ("POLLPRI");
	} else if (rep_events & POLLOUT) {
		return ("POLLOUT");
#ifdef POLLRDBAND
	} else if (rep_events & POLLRDBAND) {
		return ("POLLRDBAND");
#endif
#ifdef POLLWRBAND
	} else if (rep_events & POLLWRBAND) {
		return ("POLLWRBAND");
#endif
#ifdef POLLMSG
	} else if (rep_events & POLLMSG) {
		return ("POLLMSG");
#endif
	}
	sprintf(msg, "revents=%x", rep_events);
	return (msg);

} /* end of sf_decode_revents() */




static void handle_reset_signal(int signal_no)
{
	signal(SIGHUP,  SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	sf_note_last_error(1, "Received an un-requested signal (doing nothing about it)",
			(SIGPIPE == signal_no) ? "SIGPIPE" : "",
			(SIGHUP  == signal_no) ? "SIGHUP" : "" ) ;

	signal(SIGPIPE, handle_reset_signal);
	signal(SIGHUP,  handle_reset_signal);
	
} /* end of handle_reset_signal() */



static int sf_update_position_first_fd_free(int i)
/*
 * Called by:	gx_poll_poll_clients()
 */
{
	if (i < s_position_first_fd_free) {
		s_position_first_fd_free = i;
		return (1);
	}
	return (0);

} /* end of sf_update_position_first_fd_free() */




static int sf_event_handler_client_connection(int conn_sd, int rep_events,
					      PyObject *pyObj_inputList,
					      PyObject *pyObj_disconnectList)
/*
 * Called by:	gx_poll_poll_clients()
 */
{
	PyObject *pyObj_int_sd = NULL;
	char	  errmsg[200] = "msg??";
	int	  rc = 0;

	if (rep_events & (POLLERR | POLLHUP | POLLNVAL)) {
		/*
		 * First, close() the sd,
		 * and,   remove it from the poll_tab...
		 *
		 * Both of these steps are _essential_ !!!
		 */
		close(conn_sd);		/* close() the socket connection */

		s_ctr_closed_pollhup++;

		/*
		 * Add the conn_sd to the pyObj_disconnectList:
		 *
		 * first, convert conn_sd to a Python int...
		 */
		pyObj_int_sd = Py_BuildValue("i", conn_sd);

		rc = PyList_Append(pyObj_disconnectList, pyObj_int_sd);
		if (-1 == rc) {
			sf_note_last_error(550, "FAILED in add of conn_sd to pyObj_disconnectList:",
					sf_int_to_string(conn_sd),
					sf_decode_revents(rep_events) );
			Py_XDECREF(pyObj_int_sd);
			return (-1);
		}
		/* because the ref count was incremented by add to list,
		 * we decrement the ref count here:
		 */
		Py_XDECREF(pyObj_int_sd);

		sprintf(errmsg, "fd=%d, revent=%x", conn_sd, rep_events);

		sf_note_last_error(500, "POLLERR, HUP or NVAL:",
					errmsg,
					sf_decode_revents(rep_events) );

		return (0);	/* OK, done */
	}

	/*
	 * Add the conn_sd to the pyObj_inputList:
	 *
	 * first, convert conn_sd to a Python int...
	 */
	pyObj_int_sd = Py_BuildValue("i", conn_sd);

	rc = PyList_Append(pyObj_inputList, pyObj_int_sd);
	if (-1 == rc) {
		sf_note_last_error(551, "FAILED in add of conn_sd to pyObj_inputList:",
					sf_int_to_string(conn_sd),
					sf_decode_revents(rep_events) );
		Py_XDECREF(pyObj_int_sd);
		return (-1);
	}
	/* because the ref count was incremented by add to list,
	 * we decrement the ref count here:
	 */
	Py_XDECREF(pyObj_int_sd);

	/* the following is an informational msg:
	 */
	sprintf(errmsg, "fd=%d, revent=%x", conn_sd, rep_events);

	sf_note_last_error(100, "POLLIN:",
				errmsg,
				sf_decode_revents(rep_events) );

	return (0);	/* OK, done */

} /* end of sf_event_handler_for_channel() */




static int sf_reuse_old_poll_fd(int conn_sd)
/*
 * Called by:	sf_try_to_accept_new_connection()
 */
{
	int	i = 0;

	sf_note_last_error(150, "sf_reuse_old_poll_fd():", "s_position_first_fd_free=",
							sf_int_to_string(s_position_first_fd_free) );

	for (i = s_position_first_fd_free;
	     i < POLLFD_TABLE_SZ && i < s_high_ctr_poll_fd_used; i++) {

		if (s_poll_tab[i].fd <= -1) {
			s_poll_tab[i].fd = conn_sd;
			s_poll_tab[i].revents = 0;
			s_poll_tab[i].events  = POLLIN;
			s_position_first_fd_free = (i + 1);	/* guess */
			return (0);
		}
	}
	if (1 == s_position_first_fd_free) {	/* already search from beginning? */
		/*
		 * no entry is free, remember that
		 */
		s_position_first_fd_free = s_high_ctr_poll_fd_used;
		return (-1);
	}

	/* start at i=1 because [0] is the master socket, the listener.
	 */
	for (i = 1;
	     i < s_position_first_fd_free
	  && i < POLLFD_TABLE_SZ && i < s_high_ctr_poll_fd_used; i++) {

		if (s_poll_tab[i].fd <= -1) {
			s_poll_tab[i].fd = conn_sd;
			s_poll_tab[i].revents = 0;
			s_poll_tab[i].events  = POLLIN;
			s_position_first_fd_free = (i + 1);	/* guess */
			return (0);
		}
	}
	/*
	 * no entry is free, remember that
	 */
	s_position_first_fd_free = s_high_ctr_poll_fd_used;
	return (-1);

} /* end of sf_reuse_old_poll_fd() */




static int sf_create_new_poll_fd(int conn_sd)
/*
 * Called by:	sf_try_to_accept_new_connection()
 */
{
	if ((s_high_ctr_poll_fd_used + 1) >= POLLFD_TABLE_SZ) {
		return (-1);	/* s_poll_tab[] is full */
	}

	s_poll_tab[s_high_ctr_poll_fd_used].fd     = conn_sd;
	s_poll_tab[s_high_ctr_poll_fd_used].events = POLLIN;

	s_high_ctr_poll_fd_used++;

	return (0);

} /* end of sf_create_new_poll_fd() */




static int sf_add_fd_to_poll_tab(int conn_sd, char *p_msg)
/*
 * Called by:	sf_process_add_list()
 *		sf_try_to_accept_new_connection()
 */
{
	int	rc = 0;

	if (s_position_first_fd_free < s_high_ctr_poll_fd_used
	 && s_position_first_fd_free < POLLFD_TABLE_SZ) {
		if (sf_reuse_old_poll_fd(conn_sd) >= 0) {
			return (0);
		}
	}
	
	if ((s_high_ctr_poll_fd_used + 1) < POLLFD_TABLE_SZ) {
		if (sf_create_new_poll_fd(conn_sd) == 0) {
			return (0);
		}
	}

	sf_note_last_error(800, "poll_tab[] is full. MAX connections reached:", 
				sf_int_to_string(s_high_ctr_poll_fd_used), 
				p_msg);
	return (-1);

} /* end of sf_add_fd_to_poll_tab() */





static int sf_try_to_accept_new_connection(int master_sd)
/*
 * Called by:	sf_event_handler_master_socket()
 *
 */
{
	int	 conn_sd = -1;
	char	*p_msg = NULL;


	conn_sd = sock_accept_connection(master_sd);
	if (conn_sd < 0) {
		sf_note_last_error(1002, "accept new conn failed",
					 sock_get_last_error_diag_msg(),
					 "" );
		return (-1);
	}

	p_msg = "...close() of new connection.";

	sf_note_last_error(190, "new connection,", "sd=", sf_int_to_string(conn_sd) );

	if (sf_add_fd_to_poll_tab(conn_sd, p_msg) == 0) {
		return (conn_sd);
	}

	close(conn_sd);		/* cannot keep the connection, close the socket */

	return (-1);

} /* end of sf_try_to_accept_new_connection() */






static int sf_event_handler_master_socket(int sd, int rep_events)
/*
 * Called by:	gx_poll_poll_clients()
 *
 * This function is called when there is an event on the master listen() socket.
 *
 */
{
	char		*p_event = "event?";
	char		 errmsg[200] = "msg??";

	if (rep_events & (POLLERR | POLLHUP | POLLNVAL)){
		sprintf(errmsg, "revents=%x", rep_events);
		sf_note_last_error(1000, "poll() found error on master listen() socket.",
				errmsg,
				sf_decode_revents(rep_events) );
		return (-1);
	}

	if (rep_events & POLLIN) {
		p_event = "POLLIN";
	} else if (rep_events & POLLPRI) {
		p_event = "POLLPRI";
	} else {
		sprintf(errmsg, "revents=%x, fd=%d", rep_events, sd);
		sf_note_last_error(1001, "unknown event on master listen() socket.",
				errmsg,
				sf_decode_revents(rep_events) );
		return (-1);
	}

	/* OK, it must be a new connection ready for an accept()
	 *
	 * Let's try it:
	 */
	return (sf_try_to_accept_new_connection(sd) );

} /* end of sf_event_handler_master_socket() */



		


static int sf_process_fds_in_add_or_drop_list(PyObject *pyObj_FdList, int b_add, char *msg)
/*
 * Called by:	sf_process_drop_list()
 *		sf_process_add_list()
 *
 * if (b_add)
 *	Add all fd's in addList to s_poll_tab[].
 * else
 *	Remove all fd's in dropList from s_poll_tab[].
 *
 * Return:
 *	0	success
 *	-1	error, table full
 *	-2	pyObj_FdList is weird
 */
{
 static int	s_dropClose_removed = 0;
 static int	s_total_tried_to_remove = 0,
 		s_total_removed = 0;

} /* end of sf_process_fds_in_add_or_drop_list() */






static int sf_process_add_list(PyObject *pyObj_addList)
/*
 * Add all fd's in addList (pyObj_addList) to s_poll_tab[].
 *
 * Return:
 *	0	success
 *	-1	error, table full
 *	-2	pyObj_addList is weird
 */
{
	int	i  = 0,
		sz = 0,
		rc = 0,
		fd = -1;
	char	*msg = "sf_process_add_list():";

	PyObject *py_int = NULL;

	sz = PyList_Size(pyObj_addList);
	if (0 == sz) {
		return (0);	/* trivial case: empty list */
	}
	if (sz <= -1) {
		sf_note_last_error(990, msg, "PyList_Size(pyObj_addList) <= -1",
				"Maybe it is not a list?");
		return (-2);
	}

	for (i = 0; i < sz; i++) {
		py_int = PyList_GetItem(pyObj_addList, i);
		if (NULL == py_int) {
			sf_note_last_error(990, msg, "PyList_GetItem(pyObj_addList, i) return NULL",
					sf_int_to_string(i) );
			return (-2);
		}
		if (!PyInt_Check(py_int) ) {
			sf_note_last_error(990, msg, "py_int fails check. After PyList_GetItem() with i=",
					sf_int_to_string(i) );
			return (-2);
		}
		fd = (int) PyInt_AsLong(py_int);

		rc = sf_add_fd_to_poll_tab(fd, msg);

		if (rc <= -1) {		/* rc values 0 or 1 are OK */
			return (-1);
		}		
	}

	return (0);

} /* end of sf_process_add_list() */





/* -------------------------- Maintenance or Emergency polltab compress (clean-up): */

static int sf_compress_poll_tab(char *msg, int note_last_error_level)
/*
 * Called by:	gx_poll_poll_clients()
 */
{
	int	i = 0,
		i_free = -1,
		ctr_moved = 0,
		ctr_used  = 0,
		old_high  = 0,
		new_high_ctr_poll_fd_used = -1;
	char	msg_str[200] = "{msg?}";

	/* Start at i=1 because [0] is the master socket, the listener.
	 */
	new_high_ctr_poll_fd_used = 1;	/* if table is empty, except for master socket, this will be the new value */
	ctr_used = 0;
	for (i = 1;
	     i < POLLFD_TABLE_SZ && i < s_high_ctr_poll_fd_used; i++) {

		if (-1 == s_poll_tab[i].fd) {
			if (-1 == i_free) {
				i_free = i;
			}
			continue;
		}
		if (i_free >= 1 && i_free < POLLFD_TABLE_SZ  && i_free < s_high_ctr_poll_fd_used) {
			s_poll_tab[i_free] = s_poll_tab[i];	/* copy old element at [i] to free position [i_free] */
			i_free++;
			ctr_moved++;
			new_high_ctr_poll_fd_used = i_free;
		}
		ctr_used++;
	}
	old_high		 = s_high_ctr_poll_fd_used;
	s_high_ctr_poll_fd_used  = ctr_used + 1;	/* add one more for the master socket */
	s_position_first_fd_free = s_high_ctr_poll_fd_used;

	sprintf(msg_str, "sf_compress_poll_tab() done, old_high=%d, new=%d (%d), pos_first_fd_free=%d, ctr_moved=", 
			 old_high,
			 s_high_ctr_poll_fd_used,
			 new_high_ctr_poll_fd_used,
			 s_position_first_fd_free);

	sf_note_last_error(note_last_error_level, msg, msg_str, sf_int_to_string(ctr_moved) );

	return (ctr_moved);

} /* end of sf_compress_poll_tab() */






/* ------------------------------------ Public Python Module Functions: */


PyObject *gx_poll_get_version_id(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Args (PyObject):	int :	b_rcs_version_id
 *
 * Return:
 * 	(PyObject *) (String): version id string, RCS or compiled file & timestamp
 */
{
	int	 b_rcs_version_id = 0;


	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "i", &b_rcs_version_id) ) {
		return (NULL);
	}

	return (Py_BuildValue("s", sf_get_version_id(b_rcs_version_id) ) );

} /* end of gx_poll_get_version_id() */




PyObject *gx_poll_init_listen(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Args (PyObject):	int	listen_port_no
 *
 * Return:
 * 	(PyObject *) (Python int): the max number of client connections
 *				   supported by this version of gx_poll.c
 * 	(PyObject *) (Python String): error message explaining error
 */
{
	int	 listen_port_no = -1;
	int	 master_sd = -1;
	char	 buff_port_no[30] = "port?";

	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "i", &listen_port_no) ) {
		return (NULL);
	}

	/* check if already initialized :
	 */
	if (s_high_ctr_poll_fd_used >= 1) {
		return (Py_BuildValue("s", "gx_poll_init_listen() already init!") );
	}
	
	sprintf(buff_port_no, "%d", listen_port_no);
	master_sd = sock_socket_bind_and_listen_ext(ANY_HOST_ADDRESS,
						    buff_port_no,
						    LISTEN_QUEUE_LEN,
						    1);		/* b_so_reuseaddr = 1 */
	if (master_sd < 0) {
		sf_note_last_error(9999, "failed to start listening on:",
				 buff_port_no,
				 sock_get_last_error_diag_msg() );
		return (Py_BuildValue("s", gx_poll_get_last_err_msg() ) );
	}

	/* prepare s_poll_tab[0] entry with master_sd socket for listen:
	 */
	s_poll_tab[0].fd     = master_sd;
	s_poll_tab[0].events = POLLIN;

	s_high_ctr_poll_fd_used  = 1;	/* ready for first client to come */
	s_position_first_fd_free = 1;

	/* set signal handlers:
	 */
	signal(SIGHUP,  handle_reset_signal);
	signal(SIGPIPE, handle_reset_signal);

	return (Py_BuildValue("i", POLLFD_TABLE_SZ) );

} /* end of gx_poll_init_listen() */




PyObject *gx_poll_poll_clients(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Args (PyObject):	int :	timeout_ms
 *			List:	fd(s) to add to poll_tab
 *
 * Return:
 * 	(PyObject *) (Tuple): a Tuple of (6) elements:
 *				- List of fd with input available
 *				- List of fd that have disconnected
 *				- return code (rc), rc <= -1 indicates an error
 *				- the last error message
 *				- the priority of the last error message (0 if not error).
 *				- new conn_sd (if new connection accepted), or, 0 (no new connection)
 *	(PyObject *) (None):   no input event has occured. Timeout expired normally. 
 * 	(PyObject *) (String): error message explaining error
 */
{
	PyObject *pyObj_addList  = NULL;
	PyObject *pyObj_inputList      = NULL,
		 *pyObj_disconnectList = NULL;
	PyObject *pyObj_answer         = NULL;

	int	  timeout_ms = 500,
		  rc	     = 0,
		  n	     = 0,
		  i 	     = 0,
		  this_fd    = -1,
		  new_conn_sd = 0,
		  rep_events = 0;

	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "iO", &timeout_ms, &pyObj_addList) ) {
		return (NULL);
	}

	gx_poll_reset_last_err_msg();			/* reset err_prio and err_msg */
	
	rc = sf_process_add_list(pyObj_addList);	/* add back some fd (sd) to the poll_tab[] */

	if (rc <= -1) {
		return (Py_BuildValue("s", gx_poll_get_last_err_msg() ) );
	}

	if (timeout_ms < 1) {
		timeout_ms = 1;
		sf_note_last_error(400, "timeout_ms arg less than 1 ms.",
				 	 "Value was corrected to 1 ms.",
				 	 "");
	}

	/* enable multi-threading in the Python interpreter
	 * because poll() will block :
	 */
	Py_BEGIN_ALLOW_THREADS

	/* poll() for input or new connection, timeout if no activity:
	 */
	n = poll( &s_poll_tab[0], s_high_ctr_poll_fd_used, timeout_ms);

	/* NOTE:
	 * "The C extension must _not_ invoke any functions in the
	 *  Python C API while the lock is released". 
	 * p.318, "Python Essential Reference", 2nd edition.
	 *
	 * Therefore, we set the lock again, with Py_END_ALLOW_THREADS.
	 */
	Py_END_ALLOW_THREADS


	if (n <= -1) {
		sf_note_last_error(7000, "Error: poll() failed", "", "");
		if (EINVAL == errno) {
			sf_compress_poll_tab("poll() got EINVAL.", 7010);	/* 7010 > 7000, msg will be noted */
		}
		return (Py_BuildValue("s", gx_poll_get_last_err_msg() ) );
	}

	if (n == 0) {	/* no fd is ready, timeout has occurred */
		if (gx_poll_get_last_err_priority() <= 200) {
			return (Py_BuildValue("") );	/* return None */
		} else {
			/* Some error or warning, return a tuple with err_priority and err_msg:
			 */
			return (Py_BuildValue("(is)", gx_poll_get_last_err_priority(), gx_poll_get_last_err_msg() ) );
		}
	}

	/* create Python lists that will be returned:
	 */
	pyObj_inputList      = PyList_New(0);
	pyObj_disconnectList = PyList_New(0);
		
	/* process each entry in s_poll_tab[]... 
	 */
	new_conn_sd = 0;
	for (i=0; i < POLLFD_TABLE_SZ && i < s_high_ctr_poll_fd_used; i++) {

		if (s_poll_tab[i].revents) {	/* if some bits set... */
			rep_events = s_poll_tab[i].revents;
			this_fd    = s_poll_tab[i].fd;

			/* event on master_sd (listen):
			 */
			if (0 == i) {
				rc = sf_event_handler_master_socket(this_fd, rep_events);
				if (rc <= -1) {
					break;
				}
				new_conn_sd = rc;	/* new connection accepted */
				rc = 0;
				continue;
			}
			/* event on a client connection:
			 */
			rc = sf_event_handler_client_connection(this_fd, rep_events,
								pyObj_inputList,
								pyObj_disconnectList);
			/*
			 * whether input, disconnect or error, remove client fd from
			 * s_poll_tab[] :
			 */
			s_poll_tab[i].fd = -1;		/* not polled again until added back */

			sf_update_position_first_fd_free(i);

			if (rc <= -1) {
				break;
			}
		}
	}

	/* This debug msg about s_ctr_closed_pollhup, will only show up if no more important msg have 
	 * been noted, and, if debug level is high enough:
	 */
	sf_note_last_error(180, "After poll_tab processing,", "s_ctr_closed_pollhup=", sf_int_to_string(s_ctr_closed_pollhup) );

	pyObj_answer = Py_BuildValue("(OOisii)", pyObj_inputList, pyObj_disconnectList,
					rc,
					gx_poll_get_last_err_msg(),
					gx_poll_get_last_err_priority(),
					new_conn_sd
				    );

	/* already one reference to each list in the answer tuple, remove the 
	 * references held by local variables:
	 */
	if (NULL != pyObj_disconnectList) {
		Py_XDECREF(pyObj_disconnectList);
	}
	if (NULL != pyObj_inputList) {
		Py_XDECREF(pyObj_inputList);
	}

	return (pyObj_answer);

} /* end of gx_poll_poll_clients() */




PyObject *gx_poll_get_poll_tab(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Args (PyObject):	int :	b_counters_only
 *
 * Return:
 * 	(PyObject *) (Tuple): a Tuple of (6) elements:
 *				- List of active fd (always empty if b_counters_only is true)
 *				- s_high_ctr_poll_fd_used
 *				- ctr of (fd != -1) in the poll_tab[]
 *				- highest fd value
 *				- the last error message
 *				- the priority of the last error message (0 if not error).
 *  or
 * 	(PyObject *) (String): error message explaining error
 */
{
	PyObject *pyObj_activeList = NULL,
		 *pyObj_answer     = NULL;

	int	  b_counters_only = 0,
		  ctr_active_fd   = 0,
		  highest_fd	  = -1;

	int	  i 	     = 0,
		  this_fd    = -1;


	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "i", &b_counters_only) ) {
		return (NULL);
	}

	gx_poll_reset_last_err_msg();	/* we need that before calling sf_note_last_error() */

	/* create Python list that will be returned (if requested):
	 */
	if (b_counters_only) {
		pyObj_activeList = NULL;
	} else {
		pyObj_activeList = PyList_New(0);
	}

		
	/* look at each entry in s_poll_tab[] to see if it still active:
	 */
	for (i=0; i < POLLFD_TABLE_SZ && i < s_high_ctr_poll_fd_used; i++) {

		this_fd = s_poll_tab[i].fd;	/* hand-optimization */

		if (this_fd != -1 || s_poll_tab[i].revents) {
			ctr_active_fd++;

			if (this_fd > highest_fd) {
				highest_fd = this_fd;
			}
			if (pyObj_activeList != NULL) {
				sf_add_to_fd_list(pyObj_activeList, this_fd);
			}
		}
	}

	if (s_high_ctr_poll_fd_used > (ctr_active_fd + 100) ) {
		sf_compress_poll_tab("Found more than 100 inactive entries.", 195);
	}

	if (NULL == pyObj_activeList) {
		return (Py_BuildValue("([]iiisi)", s_high_ctr_poll_fd_used,
						ctr_active_fd,
						highest_fd,
						gx_poll_get_last_err_msg(),
						gx_poll_get_last_err_priority()
				      ) );
	}
	pyObj_answer = Py_BuildValue("(Oiiisi)", pyObj_activeList,
						s_high_ctr_poll_fd_used,
						ctr_active_fd,
						highest_fd,
						gx_poll_get_last_err_msg(),
						gx_poll_get_last_err_priority()
				     );

	Py_XDECREF(pyObj_activeList);	/* already one reference in answer tuple, remove this one */

	return (pyObj_answer);

} /* end of gx_poll_get_poll_tab() */


	
/* --------------------------------- end of gx_poll.c -------------------- */
