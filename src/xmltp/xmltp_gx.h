/* xmltp_gx.h
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_gx.h,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_gx.h,v 1.6 2004/10/14 12:18:49 blanchjj Exp $
 *
 * xmltp_gx.c: high-level module to let Python use the XML-TP parser
 * ----------
 *
 * --------------------------------------------------------------------------
 * Versions:
 * 2001oct14,jft: first version
 * 2001nov14,jft: updated comment of xmltp_gx_parse_procCall()
 * 2002feb19,jft: + PyObject *xmltp_gx_parse_response(void *p_ctx, int sd, int client_sd, int b_capture_all_results);
 * 2002may11,jft: + char *xmltp_gx_get_version_id(int b_rcs_version_id)
 * 2002jun26,jft: . xmltp_gx_parse_response(): new arg, nb_sec
 */

#ifndef _XMLTP_GX_H_
#define _XMLTP_GX_H_


/* ----------------------------------------------- Public Functions: */


char *xmltp_gx_get_version_id(int b_rcs_version_id);


PyObject *xmltp_gx_parse_procCall(void *p_ctx, int sd);
/*
 * Called by:	xmltp_gxmodule.c (Python C module wrapper)
 *
 * args:	p_ctx	must have been created by xmltp_ctx_create_new_context()
 *		previously
 *	sd	this is the fd (file descriptor) of a socket
 *
 * Return:
 * (PyObject *) which can be:
 *!		None	  meaning the socket was disconnected (or EOF)
 *!	      or,
 *		a String, which contains an error message
 *	      or,
 *		a Tuple, containing (3) three elements:
 *			procName (a String)
 *			a List of parameters, each is:
 *				a Tuple, of (5) elements:
 *					name (a String)
 *					value (a String)
 *					datatypeId (int)
 *					isNull (int)
 *					isOutput (int)
 *			bEOTreceived (int), which tell if the 
 *				<EOT/> was found in the buffer 
 *				of the p_ctx.
 */

 
PyObject *xmltp_gx_parse_response(void *p_ctx, int sd, int client_sd, int b_capture_all_results, int nb_sec);
/*
 * Called by:	xmltp_gxwrapper.c (Python C module wrapper)
 *
 * args:
 *	p_ctx		must have been created by xmltp_ctx_create_new_context()
 *			previously
 *	sd		fd (file descriptor) of a socket connected to the server
 *	client_sd	fd of a socket connected to the client. If > 0, it will receive
 *			a copy of every byte received from the server (sd).
 *
 * Return:
 * (PyObject *) which can be:
 *!		None	  meaning the socket was disconnected (or EOF)
 *!	      or,
 *		a String, which contains an error message
 *	      or,
 *		a Tuple, which the Python representation of the response.
 */


 
#endif

/* -------------------------------------- end of xmltp_gx.h ----------- */

