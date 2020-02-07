/* sock_fn.c
 * ----------
 *
 *
 *  Copyright (c) 1996-2002 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/sock_fn/sock_fn.c,v $
 * $Header: /ext_hd/cvs/sock_fn/sock_fn.c,v 1.17 2004/10/13 18:23:40 blanchjj Exp $
 *
 *
 * Functions prefix:	sock_
 *
 *
 * Functions for creating/opening SOCK_STREAM sockets
 * --------------------------------------------------
 *
 *
 * *** Reference Book Used:
 *	Comer, Douglas E. and Stevens, David L.:
 *	"Internetworking With TCP/IP Vol. III: Client-Server Progamming
 *	 And Applications (BSD Socket Version)". Prentice-Hall, Inc.
 *	Englewood Cliffs, New Jersey, 1993.
 *
 *	See Chapters: 7.17, 7.7, 9.2, 10.*
 *
 * -------------------------------------------------------------------------
 * Versions:
 * 1996jul16,jft: initial version
 * 1996jul17,jft: with sp_step_description
 * 1996jul23,jft: sock_accept_connection(): retry accept() if fail with EINTR
 * 1996jul23,jft: added bibliographic comment
 * 1996aug02,jft: created sock_init_sockaddr_infos() out from
 *		  sock_create_tcp_socket()
 * 1999oct18,dm:  Add an ifdef _AIX because the definition of the
 *			function accept is differente.
 * 1999dec09,deb: Ported to WIN32
 * 2001jun08,jft: sock_init_sockaddr_infos(): if service not found and begins
 *		  with a digit, do atoi() on it to get a port number.
 * 2002jan10,jbl: Added htons() to init p_sin->sin_port.
 * 2002jan31,deb: Removed htons() if the service is found with getservbyname()
 * 2002-05-29. fb: sock_init_sockaddr_infos(): gethostbyname does not work if an ip address
 *					is passed. Use int_addr instead with Win32.
 * 2002-09-19, fb: Change the redefinition of the close() function to closesocket() for Win32.
 * 2002sep10,jft; + sock_socket_bind_and_listen_ext() which can do: setsockopt(..., SO_REUSEADDR, ...)
 * 2003jun04,jbl: Added sock_send.
 * 2003juno5,jbl: Added sock_recv.
 */

#include "common_1.h"	/* standard #includes and #defines */
#include <time.h>	/* UNIX time_t types and functions */
#include <sys/time.h>	/* for OSX and other BSD */

		/* for using sockets ----- BEGIN */
#define _INCLUDE_HPUX_SOURCE	yes	/* to get u_long in sys/types.h */
#ifndef _INCLUDE_POSIX_SOURCE
#define _INCLUDE_POSIX_SOURCE	yes
#endif
#define _INCLUDE_XOPEN_SOURCE	yes	/* more of the same shit	*/
#if 0
#include    <fcntl.h>
#endif
#include    <sys/types.h>

#ifndef WIN32
#include    <sys/socket.h>
#include    <netinet/in.h>

#if 0
#include    <arpa/inet.h>
#endif  /*  if 0 */

#include    <netdb.h>

#else	/* WIN32 */

#include <winsock.h>
#include <io.h>
//#define  close(____file)	_close((____file))
#define  close(____file)	closesocket((____file))
#endif	/* ifndef WIN32 */

#include    <errno.h>
		/* for using sockets ----- END */

#include "sock_fn.h"
#include "rlogrecv.h"


/* -------------------------------------------- Private Defines: */

#define SOCK_SOCK_PROTOCOL	"tcp"


/* -------------------------------------------- Private structures: */


/* ------------------------------------------------- Private Variables: */

static	int	 s_debug_trace_flag = 0;
static	int	 s_errno = 0;

static	char	*sp_step_description = "[not started yet]";



/* ------------------------------------------------- Private Functions: */


static int sf_is_debug_trace_on()
{
	return (s_debug_trace_flag);
}



/* ------------------------------------------------- Public Functions: */

char *sock_get_version_id()
{
	return (VERSION_ID);
}



char *sock_get_last_error_diag_msg()
{
 static	char	 error_descr[200] = "euh?!";
 	char	*err_msg = NULL;

	if (s_errno == 0) {
		err_msg = "Failed";
	} else {
		err_msg = strerror(s_errno);	/* get standard Unix msg */
	}

	if (err_msg != NULL) {
		sprintf(error_descr, "%.50s: %.80s",
			sp_step_description,
			err_msg);
	} else {
		sprintf(error_descr, "%.50s: Failed, errno=%d",
			sp_step_description,
			s_errno);
	}

	return (error_descr);

} /* end of sock_get_last_error_diag_msg() */





int sock_get_debug_trace_flag()
{
	return (s_debug_trace_flag);
}



void sock_assign_debug_trace_flag(int new_val)
{
	s_debug_trace_flag = new_val;
}




int sock_init_sockaddr_infos(char *host, char *service,
			     struct sockaddr_in *p_sin,
			     int  size_of_sin)
/*
 * Called by:	sock_open_and_connect_tcp_socket()
 *		sock_socket_bind_and_listen()
 *		sock_obj.c
 *		(other modules)
 *
 * Steps:
 *	1) find port associated to service
 *	2) find address of host
 *
 * If any step fails, update the private module variables s_errno and
 * return (-1),
 * Else return (0).
 *
 * Return:
 *	0	OK
 *	-1	if failed
 */
{
 extern int		 errno;
	struct hostent	*p_hostent = NULL;	/* host entry		*/
	struct servent	*p_servent = NULL;	/* service entry	*/
	int		 rc = 0;
	unsigned int addr;

	if (p_sin == NULL
	 || size_of_sin != sizeof(struct sockaddr_in) ) {
		sp_step_description = "sock_init_sockaddr_infos() [Bad args]";
		return (-1);
	}

	p_sin->sin_family = AF_INET;

	/* Get port of service:
	 */
	p_servent = getservbyname(service, SOCK_SOCK_PROTOCOL);
	if (p_servent != NULL) {
		p_sin->sin_port = p_servent->s_port;

	} else if (!strcmp(service, RLOGRECV_SERVICE) ) {

		p_sin->sin_port = htons(RLOGRECV_PORT);

	} else if (isdigit(service[0]) ) {	/* begins with a digit?  must be a number */

		p_sin->sin_port = htons(atoi(service));

	} else {
		s_errno = errno;
		sp_step_description = "getservbyname()";
		return (-1);
	}

	/* Is host == ANY_HOST_ADDRESS ?
	 */
	if (!strcmp(host, ANY_HOST_ADDRESS) ) {		/* Yes... */
		p_sin->sin_addr.s_addr = INADDR_ANY;
	} else {					/* No... */
		/*
		 * Get endpoint address of host:
		 */
		p_hostent = gethostbyname(host);
#ifdef WIN32
	    if ( p_hostent == NULL ) {
	    	/* Convert nnn.nnn address to a usable one */
         	addr = inet_addr( host );
         	if( INADDR_NONE == addr ){
	   			sp_step_description = "inet_addr()";
         	   return(-1);
         	}
         	p_sin->sin_addr.s_addr = addr;
         	return (0);
      	}
#endif	/* ifndef WIN32 */
		if (p_hostent == NULL) {
			s_errno = errno;
			sp_step_description = "gethostbyname()";
			return (-1);
		}
		memcpy( (void *) &(p_sin->sin_addr),
			(void *) p_hostent->h_addr,
			p_hostent->h_length );
	}

	return (0);

} /* end of sock_init_sockaddr_infos() */




int sock_create_tcp_socket()
/*
 * Called by:	sock_open_and_connect_tcp_socket()
 *		sock_socket_bind_and_listen()
 *		sock_obj.c
 *		(other modules)
 *
 * Steps:
 *	1) find protocol number of "tcp"
 *	2) create socket
 *
 * If any step fails, update the private module variables s_errno and
 * return (-1),
 * Else return socket descriptor (socket_fd).
 *
 * Return:
 *	file descriptor of socket ( >= 0 )
 *	-1, if failed
 */
{
 extern int		 errno;
	struct protoent *p_protoent = NULL;	/* protocol entry	*/
	int		 socket_fd = -1,
			 rc = 0;

	/* Get protocol entry:
	 */
	p_protoent = getprotobyname(SOCK_SOCK_PROTOCOL);
	if (p_protoent == NULL) {
		s_errno = errno;
		sp_step_description = "getprotobyname()";
		return (-1);
	}

	/* Create the socket:
	 */
	socket_fd = socket(PF_INET, SOCK_STREAM, p_protoent->p_proto);
	if (socket_fd < 0) {
		s_errno = errno;
		sp_step_description = "sockect()";
		return (-1);
	}

	return (socket_fd);

} /* end of sock_create_tcp_socket() */




int sock_open_and_connect_tcp_socket(char *host, char *service)
/*
 * Called by:	(other modules ?)
 *
 * Steps:
 *	1-2) find port associated to service, find address of host
 *	3-4) find protocol number of "tcp", and, create socket
 *	5) connect socket
 *
 * If any step fails, update the private module variables s_errno and
 * return (-1),
 * Else return socket descriptor (socket_fd).
 *
 * Return:
 *	file descriptor of socket ( >= 0 )
 *	-1, if failed
 */
{
 extern int		 errno;
	struct sockaddr_in ip_infos;		/* Internet endpoint address */
	int		 socket_fd = -1,
			 rc = 0;

	/* Initialize ip_infos structure
	 * and set address family: Internet
	 */
	memset( (void *) &ip_infos, '\0', sizeof(ip_infos) );


	/* Find service and host and init ip_infos structure:
	 */
	rc = sock_init_sockaddr_infos(host, service,
				      &ip_infos,
				      sizeof(ip_infos) );
	if (rc < 0) {
		return (-1);
	}


	/* Find TCP protocol number and create the socket:
	 */
	socket_fd = sock_create_tcp_socket();

	if (socket_fd < 0) {
		return (-1);
	}


	/* Connect the socket:
	 */
	rc = connect(socket_fd,
		     (struct sockaddr *) &ip_infos,
		     sizeof(ip_infos) );
	if (rc < 0) {
		close(socket_fd);
		s_errno = errno;
		sp_step_description = "connect()";
		return (-1);
	}

	return (socket_fd);

} /* end of sock_open_and_connect_tcp_socket() */




int sock_socket_bind_and_listen(char *host, char *service, int queue_len)
/*
 * Called by:	(other modules)
 *
 * Steps:
 *	1-2) find port associated to service, find address of host,
 *	3-4) find protocol number of "tcp", and, create socket
 *	5) bind
 *	6) listen
 *
 * If any step fails, update the private module variables s_errno and
 * return (-1),
 * Else return socket descriptor (socket_fd).
 *
 * Return:
 *	file descriptor of socket ( >= 0 )
 *	-1, if failed
 */
{
	return (sock_socket_bind_and_listen_ext(host, service, queue_len, 0) );
}



int sock_socket_bind_and_listen_ext(char *host, char *service, int queue_len, int b_so_reuseaddr)
/*
 * Called by:	(other modules)
 *		sock_socket_bind_and_listen() -- for backward compatibility
 *
 * Steps:
 *	1-2) find port associated to service, find address of host,
 *	3-4) find protocol number of "tcp", and, create socket
 *!	4b) if (b_so_reuseaddr)
 *!		setsockopt(..., SO_REUSEADDR, ...)
 *	5) bind
 *	6) listen
 *
 * If any step fails, update the private module variables s_errno and
 * return (-1),
 * Else return socket descriptor (socket_fd).
 *
 * Return:
 *	file descriptor of socket ( >= 0 )
 *	-1, if failed
 */
{
 extern int		 errno;
	struct sockaddr_in ip_infos;		/* Internet endpoint address */
	int		 flag = 1;
	int		 socket_fd = -1,
			 rc = 0;

	/* Initialize ip_infos structure
	 * and set address family: Internet
	 */
	memset( (void *) &ip_infos, '\0', sizeof(ip_infos) );


	/* Find service and host and init ip_infos structure:
	 */
	rc = sock_init_sockaddr_infos(host, service,
				      &ip_infos,
				      sizeof(ip_infos) );
	if (rc < 0) {
		return (-1);
	}


	/* Find TCP protocol number and create the socket:
	 */
	socket_fd = sock_create_tcp_socket();

	if (socket_fd < 0) {
		return (-1);
	}


	/* use SO_REUSEADDR option to simplify restarting the server
	 * after abort or normal shutdown:
	 */
	if (b_so_reuseaddr) {
		flag = 1;
		rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag) );
		if (rc < 0) {
			close(socket_fd);
			s_errno = errno;
			sp_step_description = "setsockopt(..., SO_REUSEADDR, ...)";
			return (-1);
		}
	}


	/* Bind the socket:
	 */
	rc = bind(socket_fd, (struct sockaddr *) &ip_infos, sizeof(ip_infos) );
	if (rc < 0) {
		close(socket_fd);
		s_errno = errno;
		sp_step_description = "bind()";
		return (-1);
	}

	/* Listen with the socket:
	 */
	rc = listen(socket_fd, queue_len);
	if (rc < 0) {
		close(socket_fd);
		s_errno = errno;
		sp_step_description = "listen()";
		return (-1);
	}

	return (socket_fd);

} /* end of sock_socket_bind_and_listen() */




int sock_accept_connection(int master_sd)
/*
 * Accept a socket connection...
 *
 * Return:
 *	file descriptor of socket ( >= 0 )
 *	-1, if failed
 */
{
 extern int		   errno;
	int		   conn_sd = 0;
#if defined( _AIX ) || defined ( WIN32 )
	struct sockaddr    ip_infos;		/* Internet endpoint address */
	unsigned long	   a_len = sizeof(ip_infos);
#else
	struct sockaddr_in ip_infos;		/* Internet endpoint address */
	int		   a_len = 0;
#endif

	while (1) {
		conn_sd = accept(master_sd, &ip_infos, &a_len);

		if (conn_sd >= 0) {
			break;
		}
#ifndef WIN32
		if (errno == EINTR) {
			continue;
		}

      s_errno = errno;
#else
		if (errno == WSAEINTR) {
			continue;
		}

      s_errno = WSAGetLastError();
#endif
		sp_step_description = "accept()";
		return (-1);
	}
	return (conn_sd);

} /* end of sock_accept_connection() */


int sock_send(int s, void *msg, int len, int flags, int *error)
/*
 *
 *
 */
{
	int rc;
	rc = send(s, msg, len, flags);

	if ( rc >= 1 ) {
		*error = 0;
	}
	else {

#ifdef WIN32
	*error = WSAGetLastError();
#else
	*error = errno;
#endif
	}

	return rc;

} /* end of sock_send */



int sock_recv(int s, void *buf, size_t len, int flags, int *error)
/*
 *
 *
 */
{
	int rc;
	rc = recv(s, buf, len, flags);

	if ( rc >= 1 ) {
		*error = 0;
	}
	else {

#ifdef WIN32
	*error = WSAGetLastError();
#else
	*error = errno;
#endif
	}

	return rc;

} /* End of sock_recv() */



int sock_set_timeout(int sd, int nb_sec)
/*
 *
 *
 */
{
	int             rc = 0;
	struct timeval  tv;

	tv.tv_sec   = nb_sec;
	tv.tv_usec  = 0;

	rc = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );

	return (rc);

} /* end of sock_set_timeout() */



/* ---------------------------------------- end of sock_fn.c ------------ */
