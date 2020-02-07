/* xmltp_gxwrapper.c
 * ---------------
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_gxwrapper.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_gxwrapper.c,v 1.9 2004/10/14 12:18:49 blanchjj Exp $
 *
 *
 * xmltp_gxwrapper.c: Python C wrapper module for xmltp_gx.c
 * -----------------
 *
 * Python/C wrapper functions to use the XML-TP parser
 *
 * --------------------------------------------------------------------------
 * Versions:
 * 2001oct14,jft: first version
 * 2002feb13,jft: xmltp_gxwrapper_set_debug_mask():  if (1001 == val || 1002 == val)
 *		    call xmltp_ctx_assign_debug_level(val) to turn on/off raw XML on stderr
 * 2002feb19,jft: . xmltp_gxwrapper_parse_response(): now gives client_sd, b_capture_all_results
 *			to xmltp_gx_parse_response(p_ctx, sd, client_sd, b_capture_all_results);
 * 2002may11,jft: added Python function: getVersionId()
 * 2002may30,jft: added Python function: sendString(sd, string)
 * 2002jun26,jft: parseResponse: one more arg, (int) nb_sec_timeout_recv.
 */


#include "Python.h"

#define XMLTP_GX	1	/* to include required prototypes in xmltp_ctx.h */
#include "xmltp_ctx.h"	/* the functions that we will use */

#include "xmltp_gx.h"	/* the functions that we will use */


/* ------------------------------- Prototypes of functions defined below: */

PyObject *xmltp_gxwrapper_get_version_id(PyObject *self, PyObject *args);
PyObject *xmltp_gxwrapper_set_debug_mask(PyObject *self, PyObject *args);
PyObject *xmltp_gxwrapper_send_string(PyObject *self, PyObject *args);
PyObject *xmltp_gxwrapper_create_context(PyObject *self, PyObject *args);
PyObject *xmltp_gxwrapper_parse_procCall(PyObject *self, PyObject *args);
PyObject *xmltp_gxwrapper_parse_response(PyObject *self, PyObject *args);

/* ----------------------------------------- Python "glue" tricks: */

/* Method table mapping names to wrappers:
 */
static PyMethodDef xmltp_gx_methods[] = {

	{ "getVersionId",  xmltp_gxwrapper_get_version_id, METH_VARARGS },
	{ "setDebugMask",  xmltp_gxwrapper_set_debug_mask, METH_VARARGS },
	{ "sendString",    xmltp_gxwrapper_send_string,    METH_VARARGS },
	{ "createContext", xmltp_gxwrapper_create_context, METH_VARARGS },
	{ "parseProcCall", xmltp_gxwrapper_parse_procCall, METH_VARARGS },
	{ "parseResponse", xmltp_gxwrapper_parse_response, METH_VARARGS },
	{ NULL, NULL }
};

/* Module initialization function:
 */
initxmltp_gx(void) {
	Py_InitModule("xmltp_gx", xmltp_gx_methods);
}


/* ----------------------------------------------- Public Functions: */


PyObject *xmltp_gxwrapper_get_version_id(PyObject *self, PyObject *args)
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

	return (Py_BuildValue("s", xmltp_gx_get_version_id(b_rcs_version_id) ) );

} /* end of xmltp_gxwrapper_get_version_id() */




PyObject *xmltp_gxwrapper_set_debug_mask(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Return:
 * 	(PyObject *) None
 */
{
	int	 val = 0;

	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "i", &val) ) {
		return (NULL);
	}
	if (1001 == val || 1002 == val) {
		xmltp_ctx_assign_debug_level(val);	/* 1001: turn On, 1002: turn Off */
	} else {
		xmltp_parser_set_debug_bitmask(val);
	}
	
	return (Py_BuildValue("") );		/* None is normal */

} /* end of xmltp_gxwrapper_set_debug_mask() */





PyObject *xmltp_gxwrapper_send_string(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Send a string on a socket sd (fileno), with multi-threading enabled.
 *
 * Return:
 * 	(PyObject *) Int, the number of bytes sent, or, error code, see below:
 *		(n)	value returned by send(). See send() man page for details.
 *		-10	sd < 0
 *		-9	buff == NULL
 *		-8	n_bytes <= 0
 *		-5	XMLTP_GX not defined (this function should not be used in this case)
 */
{
	int	 sd	 = 0,
		 n_bytes = 0,
		 n 	 = 0;
	char	*str = NULL;

	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "is#", &sd, &str, &n_bytes) ) {
		return (NULL);
	}

	n = xmltp_ctx_send(sd, str, n_bytes);	/* this send() allows multi-threading */
	
	return (Py_BuildValue("i", n) );

} /* end of xmltp_gxwrapper_send_string() */





PyObject *xmltp_gxwrapper_create_context(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Return:
 * 	(PyObject *) a CObject that wraps a (void *) p_ctx
 */
{
	char	*p_context_desc_string = "[any-p_ctx]";
	void	*p_ctx = NULL;
	PyObject	*p_CObject_p_ctx = NULL;

	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "s", &p_context_desc_string) ) {
		return (NULL);
	}
	/* NOTE: p_context_desc_string is not used beyond this point
	 *  because i wanted to use PyCObject_FromVoidPtrAndDesc(),
	 *  but, i did not understand the 2 args for the destroy
	 * function. So, i used PyCObject_FromVoidPtr() instead.
	 */

	p_ctx = xmltp_ctx_create_new_context();
	if (NULL == p_ctx) {
		return (Py_BuildValue("s", "NULL returned by xmltp_ctx_create_new_context()") );
	}
	
	p_CObject_p_ctx = PyCObject_FromVoidPtr(p_ctx, xmltp_ctx_destroy_context);
	if (NULL == p_CObject_p_ctx) {
		return (Py_BuildValue("s", "NULL returned by PyCObject_FromVoidPtr()") );
	}
	/* Hopefully, the reference count is already incremeted!
	 */
	return (p_CObject_p_ctx);

} /* end of xmltp_gxwrapper_create_context() */



PyObject *xmltp_gxwrapper_parse_procCall(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Args:	(CObject) p_ctx
 *	(int) sd
 *
 * Return:		-- See xmltp_gx.c --
 * (PyObject *) which can be:
 *		a String, which contains an error message
 *	      or:
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
{
	PyObject	*pyObj_p_ctx = NULL;
	void	*p_ctx = NULL;
	int	 sd = -1;

	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "Oi", &pyObj_p_ctx, &sd) ) {
		return (NULL);
	}
	if (NULL == pyObj_p_ctx) {
		return (Py_BuildValue("s", "ERROR: (NULL == pyObj_p_ctx)") );
	}
	if (!PyCObject_Check(pyObj_p_ctx) ) {
		return (Py_BuildValue("s", "PyCObject_Check() Failed.") );
	}
	p_ctx = PyCObject_AsVoidPtr(pyObj_p_ctx);

	return (xmltp_gx_parse_procCall(p_ctx, sd) );

} /* end of xmltp_gxwrapper_parse_procCall() */




PyObject *xmltp_gxwrapper_parse_response(PyObject *self, PyObject *args)
/*
 * Called by:	Python
 *
 * Args:	(CObject) p_ctx
 *		(int) sd
 *		(int) client_sd	 --- 0 if bytes received must not be copied to client.
 *		(int) b_capture_all_results --- if 0, only capture return status.
 *		(int) nb_sec_timeout_recv
 *
 * Return:		-- See xmltp_gx.c --
 * (PyObject *) which can be:
 *			a String, which contains an error message
 *			None
 *			a Tuple (to be defined, see xmltp_gx.c)
 */
{
	PyObject	*pyObj_p_ctx = NULL;
	void		*p_ctx = NULL;
	int		 sd 	   = -1,
			 client_sd = 0,
			 b_capture_all_results = 0,
			 nb_sec = 0;

	/* Get Python arguments:
	 */
	if (!PyArg_ParseTuple(args, "Oiiii", &pyObj_p_ctx, &sd, &client_sd, &b_capture_all_results, &nb_sec) ) {
		return (NULL);
	}
	if (NULL == pyObj_p_ctx) {
		return (Py_BuildValue("s", "ERROR: (NULL == pyObj_p_ctx)") );
	}
	if (!PyCObject_Check(pyObj_p_ctx) ) {
		return (Py_BuildValue("s", "PyCObject_Check() Failed.") );
	}
	p_ctx = PyCObject_AsVoidPtr(pyObj_p_ctx);

	return (xmltp_gx_parse_response(p_ctx, sd, client_sd, b_capture_all_results, nb_sec) );

} /* end of xmltp_gxwrapper_parse_response() */

/* -------------------------------------- end of xmltp_gxwrapper.c ----------- */

