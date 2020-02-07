/*
 * xmltp_writer.h
 * --------------
 *
 *  Copyright (c) 2001-2003 Jean-Francois Touchette and Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_writer.h,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_writer.h,v 1.6 2004/10/14 12:18:55 blanchjj Exp $
 *
 *
 * XMLTP	 Writer	module to write XMLTP to a socket (or fd)
 * -----
 *
 * Funct Prefixes:	xmltp_writer_XXX()
 * --------------
 *
 * Used by:	xml2syb.c     (and, other xml2db.h daemons) 
 *		ct10api_xml.c (to send the SP call) [not done yet]
 *
 * ------------------------------------------------------------------------
 * 2001sept13,jft: first version
 * 2001oct16, jbl: added: nb_rows to xmltp_writer_begin_result_set,
 *			  xmltp_writer_begin_output_params,
 *			  xmltp_writer_end_response
 * 2003oct20,jbl: added void xmltp_writer_set_sock_obj(void *sock_obj);
 */

#ifndef _XMLTP_WRITER_H_
#define _XMLTP_WRITER_H_

/* ----------------------------------------- PUBLIC FUNCTIONS:  */

void xmltp_writer_set_sock_obj(void *sock_obj);

int xmltp_writer_reset(void *p_ctx);

int xmltp_writer_note_error(void *p_ctx, char *errmsg);

int xmltp_writer_begin_output_params(void *p_ctx, int result_set_no, int nb_rows);

int xmltp_writer_param_name(	void *p_ctx,
			char *name,
			int   datatype);

int xmltp_writer_begin_result_set(void *p_ctx, int result_set_no, int nb_rows);

int xmltp_writer_column_name(	void *p_ctx,
			char *name,
			int   datatype,
			int   opt_size);

int xmltp_writer_param_value(	void *p_ctx,
			char *name,
			int   datatype,
			int   opt_size,
			int   is_output,
			int   null_ind,
			char *strval);

int xmltp_writer_column_value(	void *p_ctx,
			int   datatype,
			int   null_ind,
			char *strval);

int xmltp_writer_end_param_values(void *p_ctx);

int xmltp_writer_end_colum_values(void *p_ctx);

int xmltp_writer_end_response(void *p_ctx,
			int return_status, 
			int rc,
			int nb_rows);

int xmltp_writer_begin_msg(void *p_ctx, int nb_rows);

int xmltp_writer_msg(	void *p_ctx,
			int  msg_num,
			char *msg_text);

int xmltp_writer_end_msg(void *p_ctx);

#endif

/* --------------------------- end of xmltp_writer.c ---------------- */
