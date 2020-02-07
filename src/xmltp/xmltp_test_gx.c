/* xmltp_test_gx.c
 * ---------------
 *
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_test_gx.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_test_gx.c,v 1.3 2004/10/14 12:18:54 blanchjj Exp $
 *
 *
 * xmltp_test_gx.c: Unit Test program for XML-TP Gateway-X module
 * ---------------
 *
 * 12oct2001 version: to test Bison "pure_parser" parser (for ProcCall)
 *
 *
 * --------------------------------------------------------------------------
 * Versions:
 * 2001oct12,jft: unit test program: test Bison "pure_parser" ProcCall parser
 */


#include <stdlib.h>

#if 0
#include "xmltp_lex.h"
#include "xmltp_parser.h"
#endif

#include "xmltp_ctx.h"


/* ----------------------------------------------- Private Defines: */

/* ----------------------------------------------- Global Variable: */

int		 g_debug = 0;		/* for xmltp_lex.c	*/

/* ----------------------------------------------- Private Variables: */

/* ----------------------------------------------- Private Functions: */


/* ----------------------------------- main() function: */

int main(int argc, char **argv)
{
	void	*p_ctx = NULL;
	int	 rc = 0;
	char	msg_buff[200] = "[ms_buff?]";

	printf("Unit test for Bison generated parser and GX modules.\n");
	printf("Type or paste a XMLTP proc call...\n");

	p_ctx = xmltp_ctx_create_new_context();

	rc = xmltp_ctx_assign_socket_fd(p_ctx, 0);	/* get input from stdin (0) */
	if (rc != 0) {
		printf("ERROR: cannot assign socket into parser context. rc=%d\n", rc);
		return (55);
	}
		
	while (1) {
		printf("ready...\n");

		rc = xmltp_ctx_reset_lexer_context(p_ctx);
		if (rc != 0) {
			printf("ERROR: reset context failed. rc=%d", rc);
			return (56);
		}

		rc = yyparse(p_ctx);

		printf("yyparse() returned %d\n", rc);
	} 

	return (0);

} /* end of main() */




/* -------------------------------------- end of xmltp_test_gx.c ----------- */
