/*************************************************************************
 * xmltp_hosts.h
 * -------------
 *
 *  Copyright (c) 2003 Jocelyn Blanchet
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
 * Look for a host name and a port number from a logical xmltp server name.
 *
 * LIST OF CHANGES:
 * ----------------
 *
 * 2003feb04, jbl:	Initial creation.
 * 
 *************************************************************************/



/**************************************************
 *
 * read a host name and a port number from 
 * a xmltp_host file.
 *
 * format: logical_name<tab/space>host_name<tab/space>port_number
 * 	empty line and comment (#) are supported.
 *
 * Logical name found on an invalid format line will be
 * reported as not found.
 * 
 * Return values
 * -------------
 *
 * 0: Success
 * 1: File not found or any open error
 * 2: Logical name not found
 *
 ***************************************************/

int xmltp_hosts(	char	*xmltp_host_file_name,	/* File path and name */
			char	*logical_name,		/* xmltp hosts logical name */
			char	*host_name,		/* regular unix hosts name  */
			char	*port_number);		/* Port number */
	    
