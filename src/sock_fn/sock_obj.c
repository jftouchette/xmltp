/* sock_obj.c
 * ----------
 *
 *
 *  Copyright (c) 1996-2005 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/sock_fn/sock_obj.c,v $
 * $Header: /ext_hd/cvs/sock_fn/sock_obj.c,v 1.24 2006/01/03 17:05:11 toucheje Exp $
 *
 *
 * Functions prefix:	sock_obj_
 *
 *
 * High-Level Functions for creating/opening/reading/writing  sockets
 * ------------------------------------------------------------------
 *
 * This module offers robust high-level functions that operate on a
 * SOCK_OBJ abstract datatype (void *).
 *
 * By using the sock_obj_xxx() functions, the programmer does not have to
 * learn the details of the lower-level functions from "sock_fn.c" and 
 * the BSD sockets functions themselves.
 *
 *
 * *** LIMITATIONS:
 *
 * For now, only "TCP" sockets are supported (no UDP).
 *
 * The first version of this module only implements functions required for
 * building "client" application.  To build "server" programs, you still 
 * have to use "sock_fn.c" (see "rlogrecv.c" as example).
 *
 *
 *
 * *** Reference Book Used for programming of sockets:
 *	Comer, Douglas E. and Stevens, David L.:
 *	"Internetworking With TCP/IP Vol. III: Client-Server Progamming
 *	 And Applications (BSD Socket Version)". Prentice-Hall, Inc. 
 *	Englewood Cliffs, New Jersey, 1993.
 *
 *	See Chapters: 7.17, 7.7, 9.2, 10.*
 *
 * -------------------------------------------------------------------------
 * Versions:
 * 1996jul31,jft: first version
 * 1999dec17,deb: . added case EINTR in both sock_obj_send()
 *		    and sock_obj_recv() because HP-UX 10.0 does
 *                  not clear this condition on a socket!
 * 2002apr09,deb: . sock_obj_send Checks explicitly the number of
 *                  bytes sent
 *                . Removed EINTR HPUX 10's patch in sock_obj_send
 * 2002-05-22, fb: Add Win32 Definition And Error Message.
 * 2002-08-22, fb: Add Winsock Error Managment Into Sock_obj_send
 *                 And Sock_obj_recv. Use For Reconnection When
 *                 Connection Is Broken.
 * 2002-08-30, fb: Change errno for WSAGetLastError For Win32 Socket.
 * 2002-09-19, fb: Redefine the closesocket() function to close()
 * 2003-07-07,jbl: Added: sock_obj_create_socket_object_from_fd
 * 			  sf_set_socket
 * 2005dec30,jft: + sock_obj_send_ext()
 *		  . clean-up bad indent and ^M left by editor on Windows
 */

		/* for using sockets ----- BEGIN */
#define _INCLUDE_HPUX_SOURCE	yes	/* to get u_long in sys/types.h */
#define _INCLUDE_POSIX_SOURCE	yes
#define _INCLUDE_XOPEN_SOURCE	yes	/* more of the same shit	*/

#include "common_1.h"	/* standard #includes and #defines */
#include <time.h>	/* UNIX time_t types and functions */

#include    <sys/types.h>

#ifndef WIN32   /* Added On 2002-05-22 */

#include    <sys/socket.h>
#include    <netinet/in.h>

#if 0
#include    <arpa/inet.h>
#endif

#include    <netdb.h>

#else	/* WIN32 */

#include <winsock.h>
#include <io.h>
#include <process.h>
#define  close(____file)	closesocket((____file))
#endif	/* ifndef WIN32 */

#include    <errno.h>
		/* for using sockets ----- END */

#include "debuglvl.h"
#include "sock_fn.h"
#include "sock_obj.h"


/* ------------------------------------------------- Private Defines: */

#define SOCK_OBJ_SIGNATURE		0x8fbf41ad

#define ERR_DESC_OUT_OF_MEMORY		"[Out of memory]"
#define ERR_DESC_BAD_SIGNATURE		"[Bad signature]"
#define ERR_DESC_NULL_PTR		"[Null pointer]"
#define ERR_DESC_HOST_NULL		"[Host arg NULL]"
#define ERR_DESC_SERVICE_NULL		"[Service arg NULL]"
#define ERR_DESC_BAD_RETRY_PARAMS	"[Bad retry param(s)]"

/* ------------------------------------------------- Private Structures: */

/*
 * Private definition of object SOCK_OBJ:
 */
typedef struct {
	long			 signature;

	char			*host,		/* ptr to copy of hostname */
				*service;

	struct sockaddr_in	 sin;

	int			 fd;		/* -1 when not connected */

	int			 last_error_code,
				 connect_max_retry,	   /* 0 == forever */
				 connect_retry_interval,   /* nb of seconds */
				 b_reconnect_on_reset_by_peer;
} SOCK_OBJ;


/* ------------------------------------------------- Private Variables: */

static	int	 s_debug_trace_flag = 0;
static	char	*s_last_error_description = "[no error yet]";


/* ------------------------------------------------- Private Functions: */

static int sf_check_signature(SOCK_OBJ *p_sock_obj)
{
	if (p_sock_obj == NULL) {
		s_last_error_description = ERR_DESC_NULL_PTR;
		return (SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR);
	}

	if (p_sock_obj->signature != SOCK_OBJ_SIGNATURE) {
		s_last_error_description = ERR_DESC_BAD_SIGNATURE;
		return (SOCK_OBJ_ERROR_BAD_SIGNATURE);
	}
	return (0);

} /* end of sf_check_signature() */



static char *sf_get_descr_of_last_error_code(err_code)
{
 static	char	default_msg[70] = "euh?";

	switch (err_code) {
	default:
		break;
	}
	sprintf(default_msg, "[err_code=%d]", err_code);

	return (default_msg);

} /* end of sf_get_descr_of_last_error_code() */



int sf_connect_ext(void *p_sock_obj_arg, int b_retry_on_econnrefused)
/*
 * Called by:	sock_obj_connect()
 *
 * SUMMARY:
 *	Connect to host / service defined in *p_sock_obj.
 *
 *	Can fail if p_sock_obj->host is not up or if cannot create a socket fd
 *	or if server process associated to p_sock_obj->service is not running.
 *
 * STEPS:
 *	1) Check if there is already a p_sock_obj->fd
 *	2) Create a new socket fd
 *	3) Connect to the end-point (host.service previously initialized)
 *
 * RETURN:
 *	SOCK_OBJ_WARNING_ALREADY_CONNECTED  already connected
 *	0				    connected OK
 *	SOCK_OBJ_ERROR_SOCKET_CREATE_FAILED socket() failed
 *	SOCK_OBJ_ERROR_CONNECT_FAILED	    connect() failed
 *	SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR    p_sock_obj is NULL
 *	SOCK_OBJ_ERROR_BAD_SIGNATURE	    *p_sock_obj has a bad signature
 */
{
	SOCK_OBJ *p_sock_obj = NULL;
	int	  retry_ctr = 0,
		  rc = 0;

	p_sock_obj = (SOCK_OBJ *) p_sock_obj_arg;

	rc = sf_check_signature(p_sock_obj);
	if (rc != 0) {
		return (rc);
	}

	if (p_sock_obj->fd >= 0) {
		p_sock_obj->last_error_code = 
				SOCK_OBJ_WARNING_ALREADY_CONNECTED;
		return (SOCK_OBJ_WARNING_ALREADY_CONNECTED);
	}

	/* Connect to the other end-point, retry if needed and if configured
	 * as such:
	 */
	for (retry_ctr = 0; ; retry_ctr++) {

		/* Find TCP protocol number and create the socket:
		 */
		p_sock_obj->fd = sock_create_tcp_socket();

		if (p_sock_obj->fd <= -1) {
			p_sock_obj->last_error_code = 
					SOCK_OBJ_ERROR_SOCKET_CREATE_FAILED;
			return (SOCK_OBJ_ERROR_SOCKET_CREATE_FAILED);
		}

		rc = connect(p_sock_obj->fd, 
			     (struct sockaddr *) &(p_sock_obj->sin),
			     sizeof(p_sock_obj->sin) );
		if (rc == 0) {
			p_sock_obj->last_error_code = 0;
			return (0);			/* CONNECT  OK */
		}

#ifdef WIN32
		p_sock_obj->last_error_code = WSAGetLastError();
#else
		p_sock_obj->last_error_code = errno;
#endif

		if (sock_obj_get_debug_trace_flag() >= DEBUG_TRACE_LEVEL_FULL) {
			fprintf(stderr, 
				"pid=%d, connect() failed, %s, p_sock_obj->last_error_code=%d\n",
				getpid(),
				strerror(p_sock_obj->last_error_code),
				p_sock_obj->last_error_code);
		}

		/* Retry is useless if errno is NOT one of these:
		 */
#ifndef WIN32
		if (p_sock_obj->last_error_code != ETIMEDOUT
		 && p_sock_obj->last_error_code != ENETUNREACH
		 && p_sock_obj->last_error_code != ENETDOWN
		 && p_sock_obj->last_error_code != EINTR
		 && (  !b_retry_on_econnrefused 
		     || p_sock_obj->last_error_code != ECONNREFUSED) 
		   ) {
			break;
		}
#else
		if (p_sock_obj->last_error_code != WSAETIMEDOUT
		 && p_sock_obj->last_error_code != WSAENETUNREACH
		 && p_sock_obj->last_error_code != WSAENETDOWN
		 && p_sock_obj->last_error_code != WSAEINTR
		 && (  !b_retry_on_econnrefused 
		     || p_sock_obj->last_error_code != WSAECONNREFUSED) 
		   ) {
			break;
		}
#endif

		if (p_sock_obj->connect_max_retry == 0
		 || retry_ctr < p_sock_obj->connect_max_retry) {
			close(p_sock_obj->fd);
			sleep(p_sock_obj->connect_retry_interval);
			continue;
		}
		break;	/* failed on first (only) try OR timeout on retry */
	}

	close(p_sock_obj->fd);
	p_sock_obj->fd = -1;

	return (SOCK_OBJ_ERROR_CONNECT_FAILED);

} /* end of sf_connect_ext() */



void sf_set_socket(SOCK_OBJ *p_sock_obj, int fd)
/*
 * Called by:	sock_obj_create_socket_object_from_fd()
 *
 * SUMMARY:
 *	Add an open socket to a SOCK_OBJ object.
 */
{

	p_sock_obj->fd = fd;

	return;
} /* end of sf_set_socket */



/* ------------------------------------------------- Public Functions: */


char *sock_obj_get_version_id()
/*
 * Return string describing version of this module.
 */
{
	return (VERSION_ID);
}


char *sock_obj_get_version_id_low_level()
/*
 * Return string describing version of "sock_fn.c"
 */
{
	return (sock_get_version_id() );	/* version id of sock_fn.c */
}



char *sock_obj_get_last_error_diag_msg(void *p_sock_obj)
/*
 * Return description of last error related to *p_sock_obj.
 */
{
	if (p_sock_obj == NULL 
	 || sf_check_signature( (SOCK_OBJ *) p_sock_obj) != 0) {
		return (s_last_error_description);
	}

	return (sf_get_descr_of_last_error_code(
			( (SOCK_OBJ *) p_sock_obj)->last_error_code ) );

} /* end of sock_obj_get_last_error_diag_msg() */




int sock_obj_get_debug_trace_flag()
{
	return (s_debug_trace_flag);
}


void sock_obj_assign_debug_trace_flag(int new_val)
/*
 * debug level is also propagated to sock_fn.c
 */
{
	s_debug_trace_flag = new_val;

	sock_assign_debug_trace_flag(new_val);
}




void *sock_obj_create_socket_object_ext(char *host,  char *service,
					int connect_max_retry,
					int connect_retry_interval,
					int b_reconnect_on_reset_by_peer)
/*
 * Called by:	sock_obj_create_socket_object()
 *		[other modules]
 *
 * SUMMARY:
 *	Create a high-level socket handle (a pointer to a SOCK_OBJ).
 *	Return NULL if fails.
 *
 * NOTES:
 *	Once such a socket handle is created, all sock_obj_xxx() functions
 *	know how to use, recover, reset and clean it.
 *
 *	There are NO reason to free() a SOCK_OBJ handle.
 *
 *	To reset or re-initialize a SOCK_OBJ connection, call 
 *	sock_obj_disconnect(), then call sock_obj_connect().
 *
 * PARAMETERS:
 *	host			name of host to connect to
 *	service			name of service  (found in /etc/services)
 *	connect_max_retry	nb of retry for connect(). NOTE: 0 == forever
 *	connect_retry_interval	interval (in seconds) between retry
 *	b_reconnect_on_reset_by_peer
 *				If TRUE, a send() or recv() that fails because
 *				"connection reset by peer" will be retried
 *				automatically after connect() is re-established.
 *					
 * STEPS:
 *	1) Validate arguments
 *	2) call sock_init_sockaddr_infos(() to:
 *		- find port associated to service
 *		- find address of host
 *	3) allocate memory for SOCK_OBJ structure
 *	4) initialize SOCK_OBJ, copy struct sockaddr_in into it
 *	5) return pointer to this struct
 *
 * RETURN:
 *	NULL		Creation failed. Diagnostic information is available
 *			 by calling:  sock_obj_get_last_error_diag_msg(NULL);
 *	(void *)	Pointer to a SOCK_OBJ structure
 */
{
	SOCK_OBJ	*p_new = NULL;
	struct sockaddr_in ip_infos;		/* Internet endpoint address */
	int		 rc = 0;

	/* Validate arguments:
	 */
	if (NULL == host) {
		s_last_error_description = ERR_DESC_HOST_NULL;
		return (NULL);
	}
	if (NULL == service) {
		s_last_error_description = ERR_DESC_SERVICE_NULL;
		return (NULL);
	}
	if (connect_max_retry < 0
	 || connect_retry_interval < 0
	 || connect_retry_interval > 300) {
		s_last_error_description = ERR_DESC_BAD_RETRY_PARAMS;
		return (NULL);
	}

	/* Clear ip_infos structure:
	 */
	memset( (void *) &ip_infos, '\0', sizeof(ip_infos) );


	/* Find service and host and init ip_infos structure:
	 */
	rc = sock_init_sockaddr_infos(host, service, 
				      &ip_infos,
				      sizeof(ip_infos) );
	if (rc < 0) {
		s_last_error_description = sock_get_last_error_diag_msg();
		return (NULL);
	}


	/* Allocate memory for structure and initialize it:
	 */
	p_new = (SOCK_OBJ *) calloc(1, sizeof(SOCK_OBJ) );
	if (p_new == NULL) {
		s_last_error_description = ERR_DESC_OUT_OF_MEMORY;
		return (NULL);
	}

	p_new->signature	= SOCK_OBJ_SIGNATURE;

	p_new->host		= strdup(host);
	p_new->service		= strdup(service);

	if (p_new->host == NULL
	 || p_new->service == NULL) {
		s_last_error_description = ERR_DESC_OUT_OF_MEMORY;
		p_new->signature = -1;
		return (NULL);
	}
	 	
	memcpy( &(p_new->sin), &ip_infos, sizeof(p_new->sin) );

	p_new->fd			= -1;	/* NO socket yet ! */
	p_new->last_error_code		= 0;
	p_new->connect_max_retry	= connect_max_retry;
	p_new->connect_retry_interval	= connect_retry_interval;
	p_new->b_reconnect_on_reset_by_peer = b_reconnect_on_reset_by_peer;

	return (p_new);

} /* end of sock_obj_create_socket_object_ext() */





void *sock_obj_create_socket_object(char *host,  char *service,
				    int connect_max_retry,
				    int b_reconnect_on_reset_by_peer)
/*
 * Called by:	[other modules]
 *
 * SUMMARY:
 *	Create a high-level socket handle (a pointer to a SOCK_OBJ).
 *	Return NULL if fails.
 *
 * NOTES:
 *	Once such a socket handle is created, all sock_obj_xxx() functions
 *	know how to use, recover, reset and clean it.
 *
 *	There are NO reason to free() a SOCK_OBJ handle.
 *
 *	To reset or re-initialize a SOCK_OBJ connection, call 
 *	sock_obj_disconnect(), then call sock_obj_connect().
 *
 * PARAMETERS:
 *	host			name of host to connect to
 *	service			name of service  (found in /etc/services)
 *	connect_max_retry	nb of retry for connect(). NOTE: 0 == forever
 *	(See DEFAULT VALUE below)
 *	b_reconnect_on_reset_by_peer
 *				If TRUE, a send() or recv() that fails because
 *				"connection reset by peer" will be retried
 *				automatically after connect() is re-established.
 *					
 *	DEFAULT VALUE:  With this function, the interval between retry is 
 *			SOCK_OBJ_DEFAULT_RETRY_INTERVAL seconds.
 *
 * STEPS: See sock_obj_create_socket_object_ext()
 *
 * RETURN:
 *	NULL		Creation failed. Diagnostic information is available
 *			 by calling:  sock_obj_get_last_error_diag_msg(NULL);
 *	(void *)	Pointer to a SOCK_OBJ structure
 */
{
  return (sock_obj_create_socket_object_ext(host, service,
					    connect_max_retry,
					    SOCK_OBJ_DEFAULT_RETRY_INTERVAL,
					    b_reconnect_on_reset_by_peer) );

} /* end of sock_obj_create_socket_object() */




void *sock_obj_create_socket_object_from_fd(char *host,  char *service,
				    int connect_max_retry,
				    int b_reconnect_on_reset_by_peer,
				    int fd)
/*
 * Called by:	[other modules]
 *
 * SUMMARY:
 *	Create a high-level socket handle (a pointer to a SOCK_OBJ)
 *	with a socket already existing (open).
 *	Return NULL if fails.
 *
 * NOTES:
 *	Once such a socket handle is created, all sock_obj_xxx() functions
 *	know how to use, recover, reset and clean it.
 *
 *	There are NO reason to free() a SOCK_OBJ handle.
 *
 *	To reset or re-initialize a SOCK_OBJ connection, call 
 *	sock_obj_disconnect(), then call sock_obj_connect().
 *
 * PARAMETERS:
 *	host			name of host to connect to
 *	service			name of service  (found in /etc/services)
 *	connect_max_retry	nb of retry for connect(). NOTE: 0 == forever
 *	(See DEFAULT VALUE below)
 *	b_reconnect_on_reset_by_peer
 *				If TRUE, a send() or recv() that fails because
 *				"connection reset by peer" will be retried
 *				automatically after connect() is re-established.
 *					
 *	DEFAULT VALUE:  With this function, the interval between retry is 
 *			SOCK_OBJ_DEFAULT_RETRY_INTERVAL seconds.
 *	fd		Socket
 *
 * STEPS: See sock_obj_create_socket_object_ext()
 *
 * RETURN:
 *	NULL		Creation failed. Diagnostic information is available
 *			 by calling:  sock_obj_get_last_error_diag_msg(NULL);
 *	(void *)	Pointer to a SOCK_OBJ structure
 */
{
	SOCK_OBJ	*p_new = NULL;
	struct sockaddr_in ip_infos;		/* Internet endpoint address */
	int		 rc = 0;

	/* Validate arguments:
	 */
	if (NULL == host) {
		s_last_error_description = ERR_DESC_HOST_NULL;
		return (NULL);
	}
	if (NULL == service) {
		s_last_error_description = ERR_DESC_SERVICE_NULL;
		return (NULL);
	}
	if (connect_max_retry < 0) {
		s_last_error_description = ERR_DESC_BAD_RETRY_PARAMS;
		return (NULL);
	}

	/* Clear ip_infos structure:
	 */
	memset( (void *) &ip_infos, '\0', sizeof(ip_infos) );


	/* Find service and host and init ip_infos structure:
	 */
	rc = sock_init_sockaddr_infos(host, service, 
				      &ip_infos,
				      sizeof(ip_infos) );

	/* Allocate memory for structure and initialize it:
	 */
	p_new = (SOCK_OBJ *) calloc(1, sizeof(SOCK_OBJ) );
	if (p_new == NULL) {
		s_last_error_description = ERR_DESC_OUT_OF_MEMORY;
		return (NULL);
	}

	p_new->signature	= SOCK_OBJ_SIGNATURE;

	p_new->host		= strdup(host);
	p_new->service		= strdup(service);

	if (p_new->host == NULL
	 || p_new->service == NULL) {
		s_last_error_description = ERR_DESC_OUT_OF_MEMORY;
		p_new->signature = -1;
		return (NULL);
	}
	 	
	memcpy( &(p_new->sin), &ip_infos, sizeof(p_new->sin) );

	p_new->fd			= fd;	/* NO socket yet ! */
	p_new->last_error_code		= 0;
	p_new->connect_max_retry	= connect_max_retry;
	p_new->connect_retry_interval	= SOCK_OBJ_DEFAULT_RETRY_INTERVAL;
	p_new->b_reconnect_on_reset_by_peer = b_reconnect_on_reset_by_peer;

	return (p_new);

} /* end of sock_obj_create_socket_object_from_fd() */





int sock_obj_disconnect(void *p_sock_obj_arg)
/*
 * Called by:	[other modules]
 *
 * SUMMARY:
 *	Close the connection associated to p_sock_obj.
 *
 *	Does NOT free p_sock_obj.  
 *
 *	The p_sock_obj can be re-used later (with sock_obj_connect() ) 
 *	if it is needed again before the program terminates.
 *
 *
 * RETURN:
 *	SOCK_OBJ_WARNING_ALREADY_DISCONNECTED
 *					  No connection to close (already done)
 *	0				  Close OK
 *	SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR  p_sock_obj is NULL
 *	SOCK_OBJ_ERROR_BAD_SIGNATURE	  *p_sock_obj has a bad signature
 */
{
	SOCK_OBJ *p_sock_obj = NULL;
	int	  rc = 0;

	p_sock_obj = (SOCK_OBJ *) p_sock_obj_arg;

	rc = sf_check_signature( (SOCK_OBJ *) p_sock_obj);
	if (rc != 0) {
		return (rc);
	}

	if (p_sock_obj->fd <= -1) {
		p_sock_obj->last_error_code = 
				SOCK_OBJ_WARNING_ALREADY_DISCONNECTED;
		return (SOCK_OBJ_WARNING_ALREADY_DISCONNECTED);
	}

	close(p_sock_obj->fd);

	p_sock_obj->fd = -1;

	return (0);

} /* end of sock_obj_disconnect() */





int sock_obj_connect(void *p_sock_obj_arg)
/*
 * Called by:	[other modules]
 *
 * SUMMARY:
 *	Connect to host / service defined in *p_sock_obj.
 *
 *	Can fail if p_sock_obj->host is not up or if cannot create a socket fd
 *	or if server process associated to p_sock_obj->service is not running.
 *
 * STEPS:
 *	1) Check if there is already a p_sock_obj->fd
 *	2) Create a new socket fd
 *	3) Connect to the end-point (host.service previously initialized)
 *
 * RETURN:
 *	SOCK_OBJ_WARNING_ALREADY_CONNECTED  already connected
 *	0				    connected OK
 *	SOCK_OBJ_ERROR_SOCKET_CREATE_FAILED socket() failed
 *	SOCK_OBJ_ERROR_CONNECT_FAILED	    connect() failed
 *	SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR    p_sock_obj is NULL
 *	SOCK_OBJ_ERROR_BAD_SIGNATURE	    *p_sock_obj has a bad signature
 */
{
	/* 0 means: do NOT retry on ECONNREFUSED
	 */
	return (sf_connect_ext(p_sock_obj_arg, 0) );

} /* end of sock_obj_connect() */





int sock_obj_send_ext(void *p_sock_obj_arg, char *buffer, int n_bytes, int b_no_reconnect)
/*
 * Called by:	sock_obj_send()
 *		[other modules]
 *
 * SUMMARY:
 *	This function writes n_bytes to the socket connection currently 
 *	held by *p_sock_obj.
 *
 *	A call to this function will block (wait) if the output buffers are
 *	full.
 *
 *	Will fail if the connection is not already established (or if 
 *	p_sock_obj is not valid) or if connection had been reset and 
 *	it could not be re-established or if configuration indicates
 *!	p_sock_obj->b_reconnect_on_reset_by_peer == 0.
 *!   NOTE: the behaviour related to p_sock_obj->b_reconnect_on_reset_by_peer == 0
 *!	    has never been implemented yet. Be very careful before changing
 *!	    this as several application might implicitly rely on this side effect.
 *!	    See sock_obj_send_ext() if you want more control over automatic
 *!	    reconnect. (jft, 2005dec30)
 *
 * RETURN:
 *!	SOCK_OBJ_ERROR_DISCONNECT_BY_PEER if send() failed and errno == EPIPE or EINVAL and b_no_reconnect
 *	SOCK_OBJ_WARNING_RECONNECT_AFTER_RESET	(> 0)
 *					  send was completed after connection
 *					  has been re-established (reset had
 *					  occurred before).
 *	0				  sent OK (n_bytes were sent OK)
 *	SOCK_OBJ_ERROR_NOT_YET_CONNECTED  not connected yet
 *	SOCK_OBJ_ERROR_DISCONNECT_BY_PEER send failed because connection was
 *					  broken by peer (server)
 *	SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR  p_sock_obj is NULL
 *	SOCK_OBJ_ERROR_BAD_SIGNATURE	  *p_sock_obj has a bad signature
 *	SOCK_OBJ_ERROR_BAD_ARGS		  buffer == NULL or n_bytes < 0
 */
{
	SOCK_OBJ *p_sock_obj = NULL;
	int	  retry_ctr = 0,
		  rc = 0;

	p_sock_obj = (SOCK_OBJ *) p_sock_obj_arg;

	rc = sf_check_signature( (SOCK_OBJ *) p_sock_obj);
	if (rc != 0) {
		return (rc);
	}

	/* Validate arguments:
	 */
	if (NULL == buffer || n_bytes < 0) {
		return (SOCK_OBJ_ERROR_BAD_ARGS);
	}
	if (n_bytes == 0) {
		return (0);	/* nothing to send, it's OK */
	}

	/* Check if socket fd is valid:
	 */
	if (p_sock_obj->fd <= -1) {
		return (SOCK_OBJ_ERROR_NOT_YET_CONNECTED);
	}

	for (retry_ctr = 0; p_sock_obj->connect_max_retry == 0
	 || retry_ctr < p_sock_obj->connect_max_retry; retry_ctr++) {

		rc = send(p_sock_obj->fd, buffer, n_bytes, 0);
		if (rc >= 1) {			/* n_bytes written OK */
			/* If send() has written less bytes than expected,
			 * thanks to signal handlers, even with SA_RESTART :(
			 * Then we check here if the number of bytes is the
			 * number expected, if not, we increment the buffer
			 * pointer to point to the first place not written
			 * and decrement the number of bytes to write with the
			 * number of bytes already written, and we try again...
			 */
			if (rc < n_bytes) {
				buffer += rc;
				n_bytes -= rc;
				continue;
			}

			return ( (retry_ctr <= 1) ? 0
				   : SOCK_OBJ_WARNING_RECONNECT_AFTER_RESET);
		}

#ifdef WIN32
		p_sock_obj->last_error_code = WSAGetLastError();
#else
		p_sock_obj->last_error_code = errno;
#endif
		if (EINTR == p_sock_obj->last_error_code) {
			/*
			 * Ignore this error....
			 */
			continue;
		}

		if (retry_ctr == 0
		 || sock_obj_get_debug_trace_flag() >= DEBUG_TRACE_LEVEL_FULL) {
			fprintf(stderr, "pid=%d, send() failed, %s, p_sock_obj->last_error_code=%d\n",
				getpid(),
				(p_sock_obj->last_error_code == EINVAL) 
					 ? "Connection broken by peer"
					 : strerror(p_sock_obj->last_error_code),
					p_sock_obj->last_error_code);
		}

		switch(p_sock_obj->last_error_code) {
#ifdef WIN32
		case 0:
		case WSAENETDOWN:
		case WSAENETRESET:
		case WSAENOTCONN:
		case WSAESHUTDOWN:
		case WSAEHOSTUNREACH:
		case WSAECONNABORTED:
		case WSAECONNRESET:
		case WSAETIMEDOUT:
#endif
		case EPIPE:
		case EINVAL:
			if (b_no_reconnect) {	/* if true, then no retry (except on EINTR) and no reconnect */
				return (SOCK_OBJ_ERROR_DISCONNECT_BY_PEER);
			}

			/* Re-connect to the other end:
			 */
			sock_obj_disconnect(p_sock_obj);
			
			/* Connect... retry if ECONNREFUSED:
			 */
			rc = sf_connect_ext( (void *) p_sock_obj, 1);
			if (rc >= 0) {
				continue;
			}
			return (SOCK_OBJ_ERROR_SEND_FAILED_AFTER_RETRY);

		default:
			return (SOCK_OBJ_ERROR_SEND_FAILED);
		}
	}
	return (SOCK_OBJ_ERROR_SEND_FAILED_AFTER_RETRY);

} /* end of sock_obj_send_ext() */



int sock_obj_send(void *p_sock_obj_arg, char *buffer, int n_bytes)
/*
 * Called by:	[other modules]
 *
 * SUMMARY:
 *	This function writes n_bytes to the socket connection currently 
 *	held by *p_sock_obj.
 *
 *	A call to this function will block (wait) if the output buffers are
 *	full.
 *
 *	Will fail if the connection is not already established (or if 
 *	p_sock_obj is not valid) or if connection had been reset and 
 *	it could not be re-established or if configuration indicates
 *!	p_sock_obj->b_reconnect_on_reset_by_peer == 0.
 *!   NOTE: the behaviour related to p_sock_obj->b_reconnect_on_reset_by_peer == 0
 *!	    has never been implemented yet. Be very careful before changing
 *!	    this as several application might implicitly rely on this side effect.
 *!	    See sock_obj_send_ext() if you want more control over automatic
 *!	    reconnect. (jft, 2005dec30)
 * RETURN:
 *	SOCK_OBJ_WARNING_RECONNECT_AFTER_RESET	(> 0)
 *					  send was completed after connection
 *					  has been re-established (reset had
 *					  occurred before).
 *	0				  sent OK (n_bytes were sent OK)
 *	SOCK_OBJ_ERROR_NOT_YET_CONNECTED  not connected yet
 *	SOCK_OBJ_ERROR_DISCONNECT_BY_PEER send failed because connection was
 *					  broken by peer (server)
 *	SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR  p_sock_obj is NULL
 *	SOCK_OBJ_ERROR_BAD_SIGNATURE	  *p_sock_obj has a bad signature
 *	SOCK_OBJ_ERROR_BAD_ARGS		  buffer == NULL or n_bytes < 0
 */
{
	return (sock_obj_send_ext(p_sock_obj_arg, buffer, n_bytes, 0) );
} /* end of sock_obj_send() */





int sock_obj_recv(void *p_sock_obj_arg, char *buffer, int *n_bytes, int b_read_full_buff)
/*
 * Called by:   [other modules]
 *
 * SUMMARY:
 *      This function receives n_bytes from the socket connection currently
 *      held by *p_sock_obj.
 *
 *      A call to this function will block (wait) if nothing is to receive.
 *
 *      It will also block if b_read_full_buff != FALSE until n_bytes are
 *      received, a signal is caught or an error is detected on the socket.
 *      otherwise (b_bread_full_buff == FALSE), it will return as soon as
 *      something is received on the socket (or an error is detected on
 *      it, or a signal is caught)
 *
 *      Will fail if the connection is not already established (or if
 *      p_sock_obj is not valid) or if connection had been reset and
 *      it could not be re-established or if configuration indicates
 *      p_sock_obj->b_reconnect_on_reset_by_peer == 0.
 *
 *      on return from this call, n_bytes returns the number of bytes
 *      received.
 *
 *
 * RETURN:
 *      SOCK_OBJ_WARNING_RECONNECT_AFTER_RESET  (> 0)
 *                                        receive was completed after connection
 *                                        has been re-established (reset had
 *                                        occurred before).
 *      0                                 sent OK (n_bytes were received OK)
 *      SOCK_OBJ_ERROR_NOT_YET_CONNECTED  not connected yet
 *      SOCK_OBJ_ERROR_DISCONNECT_BY_PEER received failed because connection was
 *                                        broken by peer (server)
 *      SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR  p_sock_obj is NULL
 *      SOCK_OBJ_ERROR_BAD_SIGNATURE      *p_sock_obj has a bad signature
 *      SOCK_OBJ_ERROR_BAD_ARGS           buffer == NULL or n_bytes < 0
 *	SOCK_OBJ_ERROR_RECEIVE_FAILED_AFTER_RETRY
 *					  The data could not be read after a
 *					  reconnection has been tried
 *	SOCK_OBJ_ERROR_RECEIVE_FAILED	  The data could not be read
 */
{
	SOCK_OBJ *p_sock_obj = p_sock_obj_arg;
	int	  retry_ctr = 0,
		  rc = 0,
        reconn = 0,
		  bytes_received = 0;

	p_sock_obj = (SOCK_OBJ *) p_sock_obj_arg;

	rc = sf_check_signature( (SOCK_OBJ *) p_sock_obj);
	if (rc != 0) {
		return (rc);
	}

	/* Validate arguments:
	 */
	if ( ( NULL == buffer ) || ( n_bytes == NULL ) || ( *n_bytes < 0 ) ) {
		if ( n_bytes != NULL ) {
			*n_bytes = 0;
		}
		return (SOCK_OBJ_ERROR_BAD_ARGS);
	}
	if ( *n_bytes == 0 ) {
		return ( 0 );	/* nothing to receive, it's OK */
	}

	/* Check if socket fd is valid:
	 */
	if ( p_sock_obj->fd <= -1 ) {
		*n_bytes = 0;
		return (SOCK_OBJ_ERROR_NOT_YET_CONNECTED);
	}

	for (retry_ctr = 0; p_sock_obj->connect_max_retry == 0
	 || retry_ctr < p_sock_obj->connect_max_retry;
	 retry_ctr++) {

		/* This recv() will block until someting is received
		 * in the reception buffer or until there is an error:
		 */
		rc = recv( p_sock_obj->fd, buffer + bytes_received,
			   *n_bytes - bytes_received, 0 );

		if ( rc >= 1 ) {
			bytes_received += rc;
			if ( ( b_read_full_buff == 0 ) ||
				( bytes_received == *n_bytes ) ) {

				*n_bytes = bytes_received;
				return ( (retry_ctr <= 1) ? 0
					   : SOCK_OBJ_WARNING_RECONNECT_AFTER_RESET);
			}
		} else {		/* An error occured */

#ifdef WIN32
			p_sock_obj->last_error_code = WSAGetLastError();
#else
			p_sock_obj->last_error_code = errno;
#endif
			if (retry_ctr == 0
			 || sock_obj_get_debug_trace_flag() >= DEBUG_TRACE_LEVEL_FULL) {
				fprintf(stderr, "pid=%d, recv() failed, %s, p_sock_obj->last_error_code=%d\n",
					getpid(),
					(p_sock_obj->last_error_code == EINVAL) 
						 ? "Connection broken by peer"
						 : strerror(p_sock_obj->last_error_code),
						p_sock_obj->last_error_code);
			}

			/*
			 * Temporary patch for HP-UX 10.0, library does not clear
			 * the EINTR signal on a socket!
			 */

			switch(p_sock_obj->last_error_code) {
#ifdef WIN32
			 case 0:
			 case WSAENETDOWN:
			 case WSAENETRESET:
			 case WSAENOTCONN:
			 case WSAESHUTDOWN:
			 case WSAEHOSTUNREACH:
			 case WSAECONNABORTED:
			 case WSAECONNRESET:
			 case WSAETIMEDOUT:
#endif
			 case EPIPE:
			 case EINTR:
			 case EINVAL:
				reconn = 1;
			 default:
				return (SOCK_OBJ_ERROR_RECEIVE_FAILED);
		         }

			if ((rc == 0) || (reconn == 1)){
				reconn = 0;
				/* Re-connect to the other end:
				 */
				sock_obj_disconnect(p_sock_obj);

				/* Connect... retry if ECONNREFUSED:
				 */
				rc = sf_connect_ext( (void *) p_sock_obj, 1);
				if (rc >= 0) {
					continue;
				}

				*n_bytes = bytes_received;
				return (SOCK_OBJ_ERROR_RECEIVE_FAILED_AFTER_RETRY);
			} else if ( rc < 0 ) {
				*n_bytes = bytes_received;
				return (SOCK_OBJ_ERROR_RECEIVE_FAILED);
			}
		}
	}

	*n_bytes = bytes_received;
	return (SOCK_OBJ_ERROR_RECEIVE_FAILED_AFTER_RETRY);
} /* end of sock_obj_recv() */






int sock_obj_recv_peek(void *p_sock_obj, char *buffer, int max_bytes)
{
	int	 rc = 0;

	rc = sf_check_signature( (SOCK_OBJ *) p_sock_obj);
	if (rc != 0) {
		return (rc);
	}

	return (-1);
}





int sock_obj_fd(void *p_sock_obj_arg)
/*
 * Called by:   [other modules]
 *
 * SUMMARY:
 *      This function returns the file descriptor associated with the
 *      sock_obj object pointed by p_sock_obj.  It is used by the calling
 *      module to control or get information from the socket or to use
 *      the select() system call on the socket.
 *
 * RETURN:
 *      -1      socket is not connected, p_sock_obj == NULL or bad signature
 *              of sock_obj object
 *      >= 0    file descriptor associated with the socket of p_sock_obj
 */
{
	int	 rc = 0;
	SOCK_OBJ *p_sock_obj = p_sock_obj_arg;

	rc = sf_check_signature( p_sock_obj );
	if ( rc != 0 ) {
		return ( - 1 );
	}

	return ( p_sock_obj->fd );
}





int sock_obj_change_connect_max_retry(void *p_sock_obj_arg,
				      int connect_max_retry)
/*
 * Called by:   [other modules]
 *
 * SUMMARY:
 *      This function changes the number of connection attempt to try before
 *	giving up.
 *
 * RETURN:
 *      0                                 connect_max_retry changed
 *	SOCK_OBJ_ERROR_BAD_ARGS		  connect_max_retry < 0 or
 *					  connect_max_retry > 300
 *      SOCK_OBJ_ERROR_BAD_SIGNATURE      *p_sock_obj has a bad signature
 */
{
	int	 rc = 0;
	SOCK_OBJ *p_sock_obj = p_sock_obj_arg;

	rc = sf_check_signature(p_sock_obj);
	if (rc != 0) {
		return (rc);
	}

	if ((connect_max_retry < 0) || (connect_max_retry > 300)) {
		return (SOCK_OBJ_ERROR_BAD_ARGS);
	}

	p_sock_obj->connect_max_retry = connect_max_retry;

	return (0);
}



/* ---------------------------------------- end of sock_obj.c ------------ */
