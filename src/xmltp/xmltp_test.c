/* xmltp_test.c
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
 * $Source: /ext_hd/cvs/xmltp/xmltp_test.c,v $
 * $Header: /ext_hd/cvs/xmltp/xmltp_test.c,v 1.3 2003/09/07 20:09:38 toucheje Exp $
 *
 *
 * xmltp_test.c:  Unit Test program for XMLTP parser
 * ------------
 *
 * --------------------------------------------------------------------------
 * Versions:
 * 2002jan14,jft: unit test program for full XMLTP parser
 * 2002feb18,jft: check xmltp_ctx_get_b_eof_disconnect(p_ctx) first
 */


#include <stdlib.h>
#include <stdio.h>

#include "xmltp_parser.h"

#include "xmltp_ctx.h"


/* ----------------------------------------------- Private Defines: */

/* ----------------------------------------------- Global Variable: */

int		 g_debug = 0;		/* for xmltp_lex.c ???? */

/* ----------------------------------------------- Private Variables: */

/* ----------------------------------------------- Private Functions: */


/* ----------------------------------- main() function: */

static void sf_write_log(int err_level, int msg_no, char *msg1, char *msg2)
{
	fprintf(stderr, "\n%d %d, %.1200s %.1200s\n", err_level, msg_no, msg1, msg2);
}



int main(int argc, char **argv)
{
	void	*p_ctx = NULL;
	int	 rc = 0,
		 b_found_eot = 0,
		 ctr_err = 0;
	char	msg_buff[200] = "[ms_buff?]";

	printf("Unit test for XMLTP parser.\n");
	printf("you should redirect stderr. Like this:  xmltp_test 2>xxx.err\n");
	printf("Type or paste a XMLTP response or call...\n");

	xmltp_ctx_assign_log_function_pf( sf_write_log );

	xmltp_parser_assign_debug_level( 555 );		/* verbose output when parsing proc call */

	p_ctx = xmltp_ctx_create_new_context();

	rc = xmltp_ctx_assign_socket_fd(p_ctx, 0);	/* get input from stdin (0) */
	if (rc != 0) {
		printf("ERROR: cannot assign fd into parser context. rc=%d\n", rc);
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

		b_found_eot = xmltp_ctx_buffer_contains_eot(p_ctx);

		if (xmltp_ctx_get_b_eof_disconnect(p_ctx) ) {
			xmltp_ctx_parser_error(p_ctx, "Apres EOF ^D", "");
			printf("\n[ EOF or disconnect! ]\n");
			break;
		}

		ctr_err = xmltp_ctx_get_lexer_or_parser_errors_ctr(p_ctx);
		if (ctr_err != 0) {
			xmltp_ctx_parser_error(p_ctx, "Apres erreur parse", "Stopping");

			printf("\nparsing failed with %d errors. rc=%d, <EOT/>=%d, %s p_ctx=%x\n",
				ctr_err, rc, 
				b_found_eot,
				xmltp_ctx_get_b_eof_disconnect(p_ctx) ? "Disc/EOF," : ",",
				p_ctx);

			break;
		}

	} 

	return (0);

} /* end of main() */




/* -------------------------------------- end of xmltp_test_gx.c ----------- */
