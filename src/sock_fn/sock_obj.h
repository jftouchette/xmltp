/* sock_obj.h
 * ----------
 *
 *  Copyright (c) 1996-2001 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/sock_fn/sock_obj.h,v $
 * $Header: /ext_hd/cvs/sock_fn/sock_obj.h,v 1.5 2006/01/03 17:05:11 toucheje Exp $
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
 * -------------------------------------------------------------------------
 * Versions:
 * 1996aug02,jft: first version
 * 1997jun02,deb: . added sock_obj_fd()
 *		  . added SOCK_OBJ_ERROR_RECEIVE_FAILED_AFTER_RETRY
 *		  . added SOCK_OBJ_ERROR_RECEIVE_FAILED
 *		  . changed the parameter max_bytes for n_bytes
 *		    in sock_obj_recv()
 * 1997dec12,deb: . added sock_obj_change_connect_max_retry()
 * 2002/08/20 FB: Win32 Error Redefinition.
 * 2003jul18,jbl: + added void *sock_obj_create_socket_object_from_fd(...)
 * 2005dec30,jft: + int sock_obj_send_ext(void *p_sock_obj_arg, char *buffer, int n_bytes, int b_no_reconnect);
 */

/* ------------------------------------------------- Public Defines: */

#define SOCK_OBJ_DEFAULT_RETRY_INTERVAL			5	/* 5 seconds */


#define SOCK_OBJ_WARNING_ALREADY_DISCONNECTED	 	1
#define SOCK_OBJ_WARNING_ALREADY_CONNECTED	 	2
#define SOCK_OBJ_WARNING_RECONNECT_AFTER_RESET	 	3

#define SOCK_OBJ_ERROR_NOT_YET_CONNECTED		-3

#define SOCK_OBJ_ERROR_SEND_FAILED_AFTER_RETRY		-5
#define SOCK_OBJ_ERROR_SEND_FAILED			-6
#define SOCK_OBJ_ERROR_RECEIVE_FAILED_AFTER_RETRY	-5
#define SOCK_OBJ_ERROR_RECEIVE_FAILED			-6
#define SOCK_OBJ_ERROR_SOCKET_CREATE_FAILED		-7
#define SOCK_OBJ_ERROR_CONNECT_FAILED			-8
#define SOCK_OBJ_ERROR_DISCONNECT_BY_PEER		-9

#define SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR		-20
#define SOCK_OBJ_ERROR_BAD_SIGNATURE			-21
#define SOCK_OBJ_ERROR_BAD_ARGS				-22

/* Win 32 Error Code Redefinition */
#ifdef WIN32
#define NOTINITIALISED  WSANOTINITIALISED
#define ENETDOWN        WSAENETDOWN
#define EFAULT          WSAEFAULT
#define ENOTCONN        WSAENOTCONN
#define EINTR           WSAEINTR
#define EINPROGRESS     WSAEINPROGRESS
#define ENETRESET       WSAENETRESET
#define ENOTSOCK        WSAENOTSOCK
#define EOPNOTSUPP      WSAEOPNOTSUPP
#define ESHUTDOWN       WSAESHUTDOWN
#define EWOULDBLOCK     WSAEWOULDBLOCK
#define EMSGSIZE        WSAEMSGSIZE
#define EINVAL          WSAEINVAL
#define ECONNABORTED    WSAECONNABORTED
#define ETIMEDOUT       WSAETIMEDOUT
#define ECONNRESET      WSAECONNRESET
#endif


/* ------------------------------------------------- Public Functions: */

char *sock_obj_get_version_id();
/*
 * Return string describing version of this module.
 */


char *sock_obj_get_version_id_low_level();
/*
 * Return string describing version of "sock_fn.c"
 */



char *sock_obj_get_last_error_diag_msg(void *p_sock_obj);
/*
 * Return description of last error related to *p_sock_obj.
 */



int sock_obj_get_debug_trace_flag();



void sock_obj_assign_debug_trace_flag(int new_val);
/*
 * debug level is also propagated to sock_fn.c
 */




void *sock_obj_create_socket_object_ext(char *host,  char *service,
					int connect_max_retry,
					int connect_retry_interval,
					int b_reconnect_on_reset_by_peer);
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




void *sock_obj_create_socket_object(char *host,  char *service,
				    int connect_max_retry,
				    int b_reconnect_on_reset_by_peer);
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




void *sock_obj_create_socket_object_from_fd(char *host,  char *service,
				    int connect_max_retry,
				    int b_reconnect_on_reset_by_peer,
				    int fd);
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





int sock_obj_disconnect(void *p_sock_obj_arg);
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




int sock_obj_connect(void *p_sock_obj_arg);
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



int sock_obj_send_ext(void *p_sock_obj_arg, char *buffer, int n_bytes, int b_no_reconnect);
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



int sock_obj_send(void *p_sock_obj_arg, char *buffer, int n_bytes);
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
 *
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




int sock_obj_recv(void *p_sock_obj, char *buffer, int *n_bytes, int b_read_full_buff);
/*
 * Called by:	[other modules]
 *
 * SUMMARY:
 *	This function receives n_bytes from the socket connection currently 
 *	held by *p_sock_obj.
 *
 *	A call to this function will block (wait) if nothing is to receive.
 *
 *	It will also block if b_read_full_buff == TRUE until n_bytes are
 *	received, a signal is caught or an error is detected on the socket.
 *	otherwise (b_bread_full_buff != TRUE), it will return as soon as
 *	something is received on the socket (or an error is detected on
 *	it, or a signal is caught)
 *
 *	Will fail if the connection is not already established (or if 
 *	p_sock_obj is not valid) or if connection had been reset and 
 *	it could not be re-established or if configuration indicates
 *	p_sock_obj->b_reconnect_on_reset_by_peer == 0.
 *
 *	on return from this call, n_bytes returns the number of bytes
 *	received.
 *
 * RETURN:
 *	SOCK_OBJ_WARNING_RECONNECT_AFTER_RESET	(> 0)
 *					  receive was completed after connection
 *					  has been re-established (reset had
 *					  occurred before).
 *	0				  sent OK (n_bytes were received OK)
 *	SOCK_OBJ_ERROR_NOT_YET_CONNECTED  not connected yet
 *	SOCK_OBJ_ERROR_DISCONNECT_BY_PEER received failed because connection was
 *					  broken by peer (server)
 *	SOCK_OBJ_ERROR_BAD_SIGN_NULL_PTR  p_sock_obj is NULL
 *	SOCK_OBJ_ERROR_BAD_SIGNATURE	  *p_sock_obj has a bad signature
 *	SOCK_OBJ_ERROR_BAD_ARGS		  buffer == NULL or n_bytes < 0
 *	SOCK_OBJ_ERROR_RECEIVE_FAILED_AFTER_RETRY
 *					  The data could not be read after a
 *					  reconnection has been tried
 *	SOCK_OBJ_ERROR_RECEIVE_FAILED	  The data could not be read
 */




int sock_obj_recv_peek(void *p_sock_obj, char *buffer, int max_bytes);
/*
 * NOT IMPLEMENTED !!!
 */



int sock_obj_fd(void *p_sock_obj);
/*
 * Called by:	[other modules]
 *
 * SUMMARY:
 *	This function returns the file descriptor associated with the
 *	sock_obj object pointed by p_sock_obj.  It is used by the calling
 *	module to control or get information from the socket or to use
 *	the select() system call on the socket.
 *
 * RETURN:
 * 	-1	socket is not connected, p_sock_obj == NULL or bad signature
 *		of sock_obj object
 *	>= 0	file descriptor associated with the socket of p_sock_obj
 */





int sock_obj_change_connect_max_retry(void *p_sock_obj_arg,
				      int connect_max_retry);
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

/* ---------------------------------------- end of sock_obj.h ------------ */
