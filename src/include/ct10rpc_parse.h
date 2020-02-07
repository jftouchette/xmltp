/*****************************************************************************
 * ct10rpc_parse.h
 * 
 *
 *  Copyright (c) 1996-2000 Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/include/ct10rpc_parse.h,v $
 * $Header: /ext_hd/cvs/include/ct10rpc_parse.h,v 1.12 2004/10/13 18:23:25 blanchjj Exp $
 *
 * These functions use automaton to validate RPC syntax and to
 * convert the RPC into and array of CT10_DATA_XDEF_T.
 *
 * ct10rpc_parse.h (ct10rpc)
 *
 * LIST OF CHANGES:
 * 1996-04-18, jbl: Creation.
 * 1997-09-16, jbl: Added:  level 13 to automaton to look for
 *					the end of a date string.
 * 1997-09-17, jbl: Change version ID to 2.1.
 */

#include "rpc_null.h"

/*
 * Version informations.
 */

#define RPC_VERSION_ID   ("2.1 " __FILE__ ": " __DATE__ " - " __TIME__)

/*
 * RPC parsing function Return value.
 */

#define RPC_SYNTAX_OK		-999	/* RPC is valid. */
#define RPC_SYNTAX_ERROR_RPC	-1	/* Syntax error on the rpc name. */
#define RPC_SYNTAX_ERROR_PARM	-2	/* Syntax error on a parameter. */
#define RPC_SYNTAX_FAIL		-10	/* Any other error. */
#define RPC_SYNTAX_CONTINUE	0	/* Keep on validating syntax. */

/*
 * Other define.
 */

#define RPC_MAXNBPARM	256	/* Maximum number of argument in a RPC. */ 
#define RPC_MAXSIZE	1024 	/* Size of working buffer. To hold parm */
				/* value before conversion.	        */

/*
 * Size of automaton.
 */

#define RPC_NB_LIGNE_AUTOMATON	14
#define RPC_NB_COLUMN_AUTOMATON	13


/*
 * Define RPC_DEBUG_MODE to include trace printing.
 */

#define RPC_DEBUG_MODE
#undef	RPC_DEBUG_MODE


/*
 * Public functions definition.
 */


/*
 *******************************************************
 RPC parsing.

 The RPC is convert from a string into 3 parts:

	*rpc_name, *t_parm, *nb_parm

 It's up to the calling program to allocate space for these
 3 pointers. For t_parm only space for the table of pointer 
 must be allocate.

 Space to  hold parameters information will be allocate by
 this function.
 

 RETURN VALUE:

	RPC_SYNTAX_OK
	RPC_SYNTAX_ERROR_RPC
	RPC_SYNTAX_ERROR_PARM
	RPC_SYNTAX_FAIL

 *******************************************************/

int ct10rpc_parse (
	CS_CONTEXT        *context,	/* Current Context	 	     */
	char		  *chaine_rpc,	/* RPC string.			     */
	char		  *rpc_name,	/* String to receive RPC name.	     */
	int		  rpc_name_size,/* Size of rpc_name.		     */
	CT10_DATA_XDEF_T  *t_parm[],	/* Table of pointer to receive parms.*/
	int		  t_parm_size,  /* Size of table t_parm.             */
	int 		  *nb_parm);	/* Pt to int to receive nb of parms. */



/*
 ******************************************************
 This function dump the RPC to stdout.
 ******************************************************/

void ct10rpc_dump(	char		  *rpc_name,
			CT10_DATA_XDEF_T  *t_parm[],
			int 		  nb_parm);


/*
 ******************************************************
 This function free space use by parms info.
 ******************************************************/

void ct10rpc_free (char *rpc_name, CT10_DATA_XDEF_T *t_parm[], int *nb_parm);



/*
 ***************************************************** 
 Get version information.
 *****************************************************/

char *ct10rpc_get_version();





/*
 * private function definition.
 */

int ct10rpc_parse_via_automaton(unsigned char *chaine);
int ct10rpc_process_1_rpc_char(int func, unsigned char c);
void ct10rpc_process_rpc_name_char(unsigned char c);
void ct10rpc_process_end_rpc_name();
void ct10rpc_process_first_value_letter(unsigned char c);
void *ct10rpc_alloc_rpc_parm_struc();
void ct10rpc_check_first_parm();
void ct10rpc_process_fisrt_parm_name_letter(unsigned char c);
void ct10rpc_process_next_parm_name_letter(unsigned char c);
void ct10rpc_process_end_parm_name();
void ct10rpc_process_end_value();
int ct10rpc_get_ready_for_next_parm(unsigned char c);
int ct10rpc_process_1_full_parm();
void ct10rpc_process_first_quote();
void ct10rpc_process_next_value_letter(unsigned char c);
void ct10rpc_process_first_ouput_ind(unsigned char c);
void ct10rpc_process_next_ouput_ind(unsigned char c);
int ct10rpc_process_end_ouput_ind();
void ct10rpc_process_first_date_quote();
