/*
 * xml2db.h
 * --------
 *
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
 * $Source: /ext_hd/cvs/xmltp/xml2db.h,v $
 * $Header: /ext_hd/cvs/xmltp/xml2db.h,v 1.5 2004/10/13 18:23:44 blanchjj Exp $
 *
 *
 *
 * Generic API for XMLTP to DataBase daemon
 * ----------------------------------------
 *
 *
 * -------------------------------------------------------------------
 * 2001sept04,jft: first version
 * 2001sept14,jft: added xml2db_write_log_msg_mm()
 * 2002jun03, jbl: added opt_size to xml2db_create_param_struct.
 */

#ifndef _XML2DB_H_
#define _XML2DB_H_


void xml2db_abort_program(int rc, char *msg1, char *msg2);
/*
 * Used by the parser to tell the main program that too many
 * parse errors have occurred, that the XML-TP protocol is not
 * followed, and, that it should drop the connection and abort.
 *
 * The explanations about the error conditions are in the strings
 * msg1, and, msg2.
 */


void xml2db_write_log_msg_mm(int errlvl, int rc, char *msg1, char *msg2);


int xml2db_jdbc_datatype_id_to_native(int jdbc_type_id);
/*
 * Convert JDBC datatype id to native database type id.
 */

char *xml2db_describe_jdbc_datatype_id(int jdbc_type_id);
/*
 * Return a string that describe this JDBC datatype.
 */

 
void *xml2db_create_param_struct(char *name,
				 int   datatype,
				 int   is_null,
				 int   is_output,
				 char *str_value,
				 char *opt_size);
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
 * Return:
 *	NULL		out of memory, or invalid arguments (see log)
 *	(void *)	pointer to a parameter structure
 */



int xml2db_free_param_struct(void *p_param);
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


char *xml2db_get_name_of_param(void *p_param);
/*
 * Return the name of the parameter held in *p_param.
 *
 * Return:
 *	NULL	p_param was NOT a valid parameter structure
 *	char *	name of the parameter
 */


int xml2db_exec_proc(void *p_conn, char *proc_name, 
		void *tab_params[], int nb_params,
		int  *p_return_status);
/*
 * Call the database procedure proc_name with parameters given in tab_params[].
 *
 * Each parameter pointers to by the pointers in tab_params must have been
 * created with xml2db_create_param_struct().
 *
 * Return:
 *	0	OK
 *	<= -1	some error occurred
 */


#endif


/* ------------------------------------------ end of xml2db.h ------------ */


