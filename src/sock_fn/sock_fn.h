/* sock_fn.h
 * ----------
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
 * $Source: /ext_hd/cvs/sock_fn/sock_fn.h,v $
 * $Header: /ext_hd/cvs/sock_fn/sock_fn.h,v 1.4 2004/10/13 18:23:41 blanchjj Exp $
 *
 *
 * Functions prefix:	sock_
 *
 *
 * Functions for creating/opening SOCK_STREAM sockets
 * --------------------------------------------------
 *
 *
 * -------------------------------------------------------------------------
 * Versions:
 * 1996jul16,jft: initial version
 * 2002sep10,jft: added sock_socket_bind_and_listen_ext(char *host, char *service, int queue_len, int b_so_reuseaddr);
 */


/* ------------------------------------------------- Public Defines: */

#define ANY_HOST_ADDRESS	"INADDR_ANY"


/* ------------------------------------------------- Public Functions: */

char *sock_get_version_id();


char *sock_get_last_error_diag_msg();
/*
 * Return description of last error that occured.
 */


int sock_get_debug_trace_flag();


void sock_assign_debug_trace_flag(int new_val);




int sock_init_sockaddr_infos(char *host, char *service, 
			     struct sockaddr_in *p_sin,
			     int  size_of_sin);
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




int sock_create_tcp_socket();
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




int sock_open_and_connect_tcp_socket(char *host, char *service);
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




int sock_socket_bind_and_listen(char *host, char *service, int queue_len);
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



int sock_socket_bind_and_listen_ext(char *host, char *service, int queue_len, int b_so_reuseaddr);
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




int sock_accept_connection(int master_sd);
/*
 * Accept a socket connection...
 *
 * Return:
 *	file descriptor of socket ( >= 0 )
 *	-1, if failed
 */




/* ---------------------------------------- end of sock_fn.h ------------ */
