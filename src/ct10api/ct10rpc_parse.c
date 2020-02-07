/*
 *****************************************************************************
 * ct10rpc_parse.c
 * 
 * These functions use automaton to validate RPC syntax and to
 * convert the RPC into and array of CT10_DATA_XDEF_T.
 *
 * *** Require ct10api.h ***
 *
 * LIST OF CHANGES:
 *
 * 1996-04-15, blanchjj:	Initial creation.
 * 1996-04-30, blanchjj:  Add support for smallint in dump function.
 * 1996-05-06, blanchjj:  Remove uppercase from RPC name.
 *			Fix problem with \n into the RPC.
 * 1996-07-17, blanchjj:  Fix core dump on "sp_titi fdsfdssdf dsfsd". +
 *			Add some comments.
 *
 * 1996-12-20, blanchjj:  Comments on sybase bugs:
 *			It's impossible to pass a string of len 0 ("") as a 
 *			parameter to a stored proc. If len is 0 sybase
 *			convert it to a NULL. Because of this There is a 
 *			special check if len is 0, len is change to 1.
 *
 *			The string in the store proc look OK when printed
 *			but there is a binary 0 in the string wich is of
 *			len 1. This may cause problems in the stored proc.
 *
 *			To fix this, I initialize the char 1 with a space, 
 *			no more garbadge will be in it.
 *
 * 1996-12-20, blanchjj:	Replace all malloc by calloc.
 *
 * 1997jan13,jft: cleaned up for port to NT (stays compatible with HP-UX!)
 *		  ---->	static char *version_info = RPC_VERSION_ID;
 *
 * 1997-02-05, blanchjj:	Characters from 128 to 255 need to be
 *				valid in a string. 
 *					Extend ascii_value_column_in_auto
 *					from 127 to 255 rows.
 *					Accept value from 0 to 255.
 *					Change char by unsigned char.
 *
 * 1997-02-06, blanchjj:	Add support for RPC_CS_DATETIME_NULL.
 * 1997-02-18, benoitde:	Added conditional include for Windows NT port.
 * 1997-03-14, blanchjj:	Added support for DATE type parameter.
 * 1997-09-16, blanchjj:	Added:  level 13 to automaton to look for
 *					the end of a date string.
 * 2000jun27,deb: Fixed fmt.maxlength for VARCHAR parameters with Sybase 12.0
 *                and onward versions.
 * 2002-02-14,jbl: Remove cs_convert for the version TRG-X compatible.
 *		   Native sybase date format wont be supported anymore.
 *		   All date will be process as char.
 *		   Use of #ifdef to keep old and new version
 */



#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#ifndef WIN32
#include	<strings.h>
#endif

#include	<ctpublic.h> 
#include	<malloc.h>

#include	"ct10api.h"
#include	"check_num_string.h"
#include	"ct10rpc_parse.h"
#include	"ct10str.h"

#ifdef WIN32
#include	"ntunixfn.h"
#endif

/*
 *********************************************************
 This array matches an ascii value with the corresponding
 column in the 'automaton'. The ascii value is use to find
 the offset in the table.

 The ascii value is a character from the input RPC string.

 example:

	rpc string: "spoe_insert_stock @dte="19960101"

	if we are position on the first character ("s") we 
	use the ascii value of "s" as the indice in the table:
	ascii_value_column_in_auto. The value we find there is
	the column in the automaton where letter a to z are
	coded.

 This table is good for rpc syntax validation only.
 *********************************************************/

int ascii_value_column_in_auto[] = {
/* ascii: \0 (0)  */ 9,
/* ascii:  (1)  */ 10, 
/* ascii:  (2)  */ 10, 
/* ascii:  (3)  */ 10, 
/* ascii:  (4)  */ 10, 
/* ascii:  (5)  */ 10, 
/* ascii:  (6)  */ 10, 
/* ascii:  (7)  */ 10, 
/* ascii:  (8)  */ 10, 
/* ascii:  (9)  */ 5, 
/* ascii: (10)  */ 5, 
/* ascii: (11)  */ 10, 
/* ascii:  (12)  */ 10, 
/* ascii:  (13)  */ 5, 
/* ascii:  (14)  */ 10, 
/* ascii:  (15)  */ 10, 
/* ascii:  (16)  */ 10, 
/* ascii:  (17)  */ 10, 
/* ascii:  (18)  */ 10, 
/* ascii:  (19)  */ 10, 
/* ascii:  (20)  */ 10, 
/* ascii:  (21)  */ 10, 
/* ascii:  (22)  */ 10, 
/* ascii:  (23)  */ 10, 
/* ascii:  (24)  */ 10, 
/* ascii:  (25)  */ 10, 
/* ascii:  (26)  */ 10, 
/* ascii:  (27)  */ 10, 
/* ascii:  (28)  */ 10, 
/* ascii:  (29)  */ 10, 
/* ascii:  (30)  */ 10, 
/* ascii:  (31)  */ 10, 
/* ascii:   (32)  */ 5, 
/* ascii: ! (33)  */ 10, 
/* ascii: " (34)  */ 7, 
/* ascii: # (35)  */ 10, 
/* ascii: $ (36)  */ 10, 
/* ascii: % (37)  */ 10, 
/* ascii: & (38)  */ 10, 
/* ascii: ' (39)  */ 10, 
/* ascii: ( (40)  */ 10, 
/* ascii: ) (41)  */ 10, 
/* ascii: * (42)  */ 10, 
/* ascii: + (43)  */ 2, 
/* ascii: , (44)  */ 8, 
/* ascii: - (45)  */ 2, 
/* ascii: . (46)  */ 3, 
/* ascii: / (47)  */ 10, 
/* ascii: 0 (48)  */ 1, 
/* ascii: 1 (49)  */ 1, 
/* ascii: 2 (50)  */ 1, 
/* ascii: 3 (51)  */ 1, 
/* ascii: 4 (52)  */ 1, 
/* ascii: 5 (53)  */ 1, 
/* ascii: 6 (54)  */ 1, 
/* ascii: 7 (55)  */ 1, 
/* ascii: 8 (56)  */ 1, 
/* ascii: 9 (57)  */ 1, 
/* ascii: : (58)  */ 10, 
/* ascii: ; (59)  */ 10, 
/* ascii: < (60)  */ 10, 
/* ascii: = (61)  */ 6, 
/* ascii: > (62)  */ 10, 
/* ascii: ? (63)  */ 10, 
/* ascii: @ (64)  */ 4, 
/* ascii: A (65)  */ 0, 
/* ascii: B (66)  */ 0, 
/* ascii: C (67)  */ 0, 
/* ascii: D (68)  */ 0, 
/* ascii: E (69)  */ 0, 
/* ascii: F (70)  */ 0, 
/* ascii: G (71)  */ 0, 
/* ascii: H (72)  */ 0, 
/* ascii: I (73)  */ 0, 
/* ascii: J (74)  */ 0, 
/* ascii: K (75)  */ 0, 
/* ascii: L (76)  */ 0, 
/* ascii: M (77)  */ 0, 
/* ascii: N (78)  */ 0, 
/* ascii: O (79)  */ 0, 
/* ascii: P (80)  */ 0, 
/* ascii: Q (81)  */ 0, 
/* ascii: R (82)  */ 0, 
/* ascii: S (83)  */ 0, 
/* ascii: T (84)  */ 0, 
/* ascii: U (85)  */ 0, 
/* ascii: V (86)  */ 0, 
/* ascii: W (87)  */ 0, 
/* ascii: X (88)  */ 0, 
/* ascii: Y (89)  */ 0, 
/* ascii: Z (90)  */ 0, 
/* ascii: [ (91)  */ 10, 
/* ascii: \ (92)  */ 10, 
/* ascii: ] (93)  */ 10, 
/* ascii: ^ (94)  */ 10, 
/* ascii: _ (95)  */ 0, 
/* ascii: ` (96)  */ 10, 
/* ascii: a (97)  */ 0, 
/* ascii: b (98)  */ 0, 
/* ascii: c (99)  */ 0, 
/* ascii: d (100)  */ 0, 
/* ascii: e (101)  */ 0, 
/* ascii: f (102)  */ 0, 
/* ascii: g (103)  */ 0, 
/* ascii: h (104)  */ 0, 
/* ascii: i (105)  */ 0, 
/* ascii: j (106)  */ 0, 
/* ascii: k (107)  */ 0, 
/* ascii: l (108)  */ 0, 
/* ascii: m (109)  */ 0, 
/* ascii: n (110)  */ 0, 
/* ascii: o (111)  */ 0, 
/* ascii: p (112)  */ 0, 
/* ascii: q (113)  */ 0, 
/* ascii: r (114)  */ 0, 
/* ascii: s (115)  */ 0, 
/* ascii: t (116)  */ 0, 
/* ascii: u (117)  */ 0, 
/* ascii: v (118)  */ 0, 
/* ascii: w (119)  */ 0, 
/* ascii: x (120)  */ 0, 
/* ascii: y (121)  */ 0, 
/* ascii: z (122)  */ 0, 
/* ascii: { (123)  */ 11, 
/* ascii: | (124)  */ 10, 
/* ascii: } (125)  */ 12, 
/* ascii: ~ (126)  */ 10, 
/* ascii:  (127)  */ 10, 
/* ascii: ~ (128)  */ 10,
/* ascii: ~ (129)  */ 10,
/* ascii: ~ (130)  */ 10,
/* ascii: ~ (131)  */ 10,
/* ascii: ~ (132)  */ 10,
/* ascii: ~ (133)  */ 10,
/* ascii: ~ (134)  */ 10,
/* ascii: ~ (135)  */ 10,
/* ascii: ~ (136)  */ 10,
/* ascii: ~ (137)  */ 10,
/* ascii: ~ (138)  */ 10,
/* ascii: ~ (139)  */ 10,
/* ascii: ~ (140)  */ 10,
/* ascii: ~ (141)  */ 10,
/* ascii: ~ (142)  */ 10,
/* ascii: ~ (143)  */ 10,
/* ascii: ~ (144)  */ 10,
/* ascii: ~ (145)  */ 10,
/* ascii: ~ (146)  */ 10,
/* ascii: ~ (147)  */ 10,
/* ascii: ~ (148)  */ 10,
/* ascii: ~ (149)  */ 10,
/* ascii: ~ (150)  */ 10,
/* ascii: ~ (151)  */ 10,
/* ascii: ~ (152)  */ 10,
/* ascii: ~ (153)  */ 10,
/* ascii: ~ (154)  */ 10,
/* ascii: ~ (155)  */ 10,
/* ascii: ~ (156)  */ 10,
/* ascii: ~ (157)  */ 10,
/* ascii: ~ (158)  */ 10,
/* ascii: ~ (159)  */ 10,
/* ascii: ~ (160)  */ 10,
/* ascii: ~ (161)  */ 10,
/* ascii: ~ (162)  */ 10,
/* ascii: ~ (163)  */ 10,
/* ascii: ~ (164)  */ 10,
/* ascii: ~ (165)  */ 10,
/* ascii: ~ (166)  */ 10,
/* ascii: ~ (167)  */ 10,
/* ascii: ~ (168)  */ 10,
/* ascii: ~ (169)  */ 10,
/* ascii: ~ (170)  */ 10,
/* ascii: ~ (171)  */ 10,
/* ascii: ~ (172)  */ 10,
/* ascii: ~ (173)  */ 10,
/* ascii: ~ (174)  */ 10,
/* ascii: ~ (175)  */ 10,
/* ascii: ~ (176)  */ 10,
/* ascii: ~ (177)  */ 10,
/* ascii: ~ (178)  */ 10,
/* ascii: ~ (179)  */ 10,
/* ascii: ~ (180)  */ 10,
/* ascii: ~ (181)  */ 10,
/* ascii: ~ (182)  */ 10,
/* ascii: ~ (183)  */ 10,
/* ascii: ~ (184)  */ 10,
/* ascii: ~ (185)  */ 10,
/* ascii: ~ (186)  */ 10,
/* ascii: ~ (187)  */ 10,
/* ascii: ~ (188)  */ 10,
/* ascii: ~ (189)  */ 10,
/* ascii: ~ (190)  */ 10,
/* ascii: ~ (191)  */ 10,
/* ascii: ~ (192)  */ 10,
/* ascii: ~ (193)  */ 10,
/* ascii: ~ (194)  */ 10,
/* ascii: ~ (195)  */ 10,
/* ascii: ~ (196)  */ 10,
/* ascii: ~ (197)  */ 10,
/* ascii: ~ (198)  */ 10,
/* ascii: ~ (199)  */ 10,
/* ascii: ~ (200)  */ 10,
/* ascii: ~ (201)  */ 10,
/* ascii: ~ (202)  */ 10,
/* ascii: ~ (203)  */ 10,
/* ascii: ~ (204)  */ 10,
/* ascii: ~ (205)  */ 10,
/* ascii: ~ (206)  */ 10,
/* ascii: ~ (207)  */ 10,
/* ascii: ~ (208)  */ 10,
/* ascii: ~ (209)  */ 10,
/* ascii: ~ (210)  */ 10,
/* ascii: ~ (211)  */ 10,
/* ascii: ~ (212)  */ 10,
/* ascii: ~ (213)  */ 10,
/* ascii: ~ (214)  */ 10,
/* ascii: ~ (215)  */ 10,
/* ascii: ~ (216)  */ 10,
/* ascii: ~ (217)  */ 10,
/* ascii: ~ (218)  */ 10,
/* ascii: ~ (219)  */ 10,
/* ascii: ~ (220)  */ 10,
/* ascii: ~ (221)  */ 10,
/* ascii: ~ (222)  */ 10,
/* ascii: ~ (223)  */ 10,
/* ascii: ~ (224)  */ 10,
/* ascii: ~ (225)  */ 10,
/* ascii: ~ (226)  */ 10,
/* ascii: ~ (227)  */ 10,
/* ascii: ~ (228)  */ 10,
/* ascii: ~ (229)  */ 10,
/* ascii: ~ (230)  */ 10,
/* ascii: ~ (231)  */ 10,
/* ascii: ~ (232)  */ 10,
/* ascii: ~ (233)  */ 10,
/* ascii: ~ (234)  */ 10,
/* ascii: ~ (235)  */ 10,
/* ascii: ~ (236)  */ 10,
/* ascii: ~ (237)  */ 10,
/* ascii: ~ (238)  */ 10,
/* ascii: ~ (239)  */ 10,
/* ascii: ~ (240)  */ 10,
/* ascii: ~ (241)  */ 10,
/* ascii: ~ (242)  */ 10,
/* ascii: ~ (243)  */ 10,
/* ascii: ~ (244)  */ 10,
/* ascii: ~ (245)  */ 10,
/* ascii: ~ (246)  */ 10,
/* ascii: ~ (247)  */ 10,
/* ascii: ~ (248)  */ 10,
/* ascii: ~ (249)  */ 10,
/* ascii: ~ (250)  */ 10,
/* ascii: ~ (251)  */ 10,
/* ascii: ~ (252)  */ 10,
/* ascii: ~ (253)  */ 10,
/* ascii: ~ (254)  */ 10,
/* ascii: ~ (255)  */ 10
};


/*
 ******************************************************************
 Automaton use for RPC syntax validation.
 ******************************************************************/

 struct RPC_AUTO_S {
	int level;		/* Next level to go. */
	int func;		/* processing to do on the current char. */
 };

struct RPC_AUTO_S 
	rpc_automaton[RPC_NB_LIGNE_AUTOMATON][RPC_NB_COLUMN_AUTOMATON] = {

/*      A-Z      0-9      +/-      .        @        <tab>    =        "        ,        \0          * 	     {	         } */

/*0*/{{ 1,100},{ 1,100},{-1,  0},{-1,  0},{-1,  0},{ 0,  0},{-1,  0},{-1,  0},{-1,  0},{  -1,  0},{-1,  0},{-1,  0},{-1,  0} },
/*1*/{{ 1,100},{ 1,100},{-1,  0},{ 1,100},{-1,  0},{ 3,116},{-1,  0},{-1,  0},{-1,  0},{-999,116},{-1,  0},{-1,  0},{-1,  0} },
/*2*/{{ 1,  0},{ 1,  0},{-1,  0},{-1,  0},{-1,  0},{-1,  0},{-1,  0},{-1,  0},{-1,  0},{  -1,  0},{-1,  0},{-1,  0},{-1,  0} },
/*3*/{{ 8,103},{ 8,103},{ 8,103},{ 8,103},{ 5,101},{ 3,  0},{-2,  0},{ 9,108},{-2,  0},{-999,  0},{-2,  0},{13,117},{-1,  0} },
/*4*/{{ 5,101},{ 5,101},{-2,  0},{-2,  0},{-2,  0},{-2,  0},{-2,  0},{-2,  0},{-2,  0},{  -2,  0},{-2,  0},{-1,  0},{-1,  0} },
/*5*/{{ 5,102},{ 5,102},{-2,  0},{-2,  0},{-2,  0},{ 6,105},{ 7,105},{-2,  0},{-2,  0},{  -2,  0},{-2,  0},{-1,  0},{-1,  0} },
/*6*/{{-2,  0},{-2,  0},{-2,  0},{-2,  0},{-2,  0},{ 6,  0},{ 7,  0},{-2,  0},{-2,  0},{  -2,  0},{-2,  0},{-1,  0},{-1,  0} },
/*7*/{{ 8,103},{ 8,103},{ 8,103},{ 8,103},{-2,  0},{ 7,  0},{-2,  0},{ 9,108},{-2,  0},{  -2,  0},{ 8,103},{13,117},{ 8,103} },
/*8*/{{ 8,104},{ 8,104},{ 8,104},{ 8,104},{ 8,104},{10,106},{ 8,104},{ 8,104},{ 3,111},{-999,111},{ 8,104},{ 8,104},{ 8,104} },
/*9*/{{ 9,107},{ 9,107},{ 9,107},{ 9,107},{ 9,107},{ 9,107},{ 9,107},{10,109},{ 9,107},{  -2,  0},{ 9,107},{ 9,107},{ 9,107} },
/*0*/{{11,112},{-2,  0},{-2,  0},{-2,  0},{-2,  0},{10,  0},{-2,  0},{-2,  0},{ 3,110},{-999,110},{-2,  0},{-1,  0},{-1,  0} },
/*1*/{{11,113},{-2,  0},{-2,  0},{-2,  0},{-2,  0},{12,114},{-2,  0},{-2,  0},{ 3,115},{-999,115},{-2,  0},{-1,  0},{-1,  0} },
/*2*/{{-2,  0},{-2,  0},{-2,  0},{-2,  0},{-2,  0},{12,  0},{-2,  0},{-2,  0},{ 3,110},{-999,110},{-2,  0},{-1,  0},{-1,  0} },
/*3*/{{13,107},{13,107},{13,107},{13,107},{13,107},{13,107},{13,107},{13,107},{13,107},{  -2,  0},{13,107},{13,107},{10,109} },
};


/*
 ******************************************************************************
 List of functions:

 100: Current position is the RPC name. Copy this letter in the RPC name buffer.
 101: First Letter of parameter name (copy letter, alloc struct). 
 102: Next letter of parameter name (copy letter).

 105: End of parameter name (copy \0).

 103: First letter of value (copy letter, if not done alloc struct).
 104: Next letters of value (copy letter).
 106: End of value copy (copy \0).

 107: Next letters of value (quote)
 108: Begin of quote (no letter to copy)
 109: End of quote value (copy \0)

 110: Get ready for next parm, after ','
 111: 106 + 110

 112: First output indicator letter.
 113: Next output indicateur letter.
 114: End of output indicator

 115: 114 + 110

 116: End of proc name. NULL terminate rpc_name.

 117: Begin special quote for date (no letter to copy).

 List of level:

 0:	Before stored procedure name, Space, tab, return are skip until 
	something found.
 1:	Stored proc name found, reading letter from name until space, tab
	or return.
 2:	After a '.' we don't want another '.'. This is not use because two
	'.' caractere can be consecutive ex: cdboe..sp_add_order.
 3:	After the stored proc name we look for a parm name, a parm
	value or the end.
 4:	After '@' reading first letter of parm name.
 5:	Reading Next parm name letter.
 6:	After a parm name, looking for '='.
 7:	After '=' looking for a parm value.
 8:	Reading Next parm value caracters Not quoted.
 9:	Reading Next parm value caracters Quoted.
10:	Looking for ',', output or End of string .
11:	Reading output indicator letters (out or OUTPUT)
12:	Skipping spaces and tabs after output indicator.
13:	Reading next parm value date quoted "{}"

 ******************************************************************************/


/*
 ***********************************************
 Symbols use by many function of this module
 We don't want to pass to many paramter.
 ***********************************************/

static CT10_DATA_XDEF_T	**parm_tbl;
static int 		parm_tbl_size;	      /* Size of parm_tbl.	     */
static int		current_parm=0;	      /* Current posi in parm_tbl.   */
static int		first_parm_ind=1;     /* 1:first parm, 0: next parms.*/
static int		quote_ind;	      /* Current parm is quoted.     */
static char		p_rpc_name[RPC_MAXSIZE];  /* RPC name buffer.        */
static char		p_parm_name[RPC_MAXSIZE]; /* PARM name.		     */
static char		p_parm_val[RPC_MAXSIZE];  /* PARM value.	     */
static char		p_output_ind[RPC_MAXSIZE];/* PARM output indicator.  */
static int		dest_ind;	      /* current posi in output buf. */	
static int		output_ind=0;	      /* To remember that a parm is  */
					      /* output.                     */
static CS_CONTEXT	*tmp_context;	      /* We need a context to use    */
					      /* cs_convert.                 */


/*
 * Version information.
 */

static char *version_info = RPC_VERSION_ID;


/*^L
 *********************
 Get version info.
 *********************/

char *ct10rpc_get_version()
{
   return version_info;
} /* --------------------------- End of ct10rpc_get_version ----------------- */


/*
 ***********************************
 free space use by a RPC structure.
 ***********************************/

void ct10rpc_free (char *rpc_name, CT10_DATA_XDEF_T  *t_parm[], int *nb_parm)
{
int x;

for (x=0; x<*nb_parm; ++x)
{

	if (t_parm[x] != NULL) {
		if (t_parm[x]->p_val != NULL) {
			free(t_parm[x]->p_val);
		}
		free(t_parm[x]);
		t_parm[x] = NULL;
	}
}

*nb_parm = 0;
rpc_name[0] = '\0';

} /* --------------------------- End of ct10rpc_free ----------------- */


/*
 ******************************************************************
 RPC validation automaton.
 ******************************************************************/

int ct10rpc_parse (	CS_CONTEXT 	  *context,
			char              *chaine_rpc,
			char              *rpc_name,
			int		  rpc_name_size,
			CT10_DATA_XDEF_T  *t_parm[],
			int		  t_parm_size,
			int               *nb_parm)
{
int rc;

/* We initialize output params */

*nb_parm = 0;
strncpy(rpc_name,"",rpc_name_size);

/* Parameter validation. */
if (	(context == NULL)	||
	(chaine_rpc == NULL)	||
	(rpc_name == NULL)	||
	(rpc_name_size <= 0)	||
	(t_parm == NULL)	||
	(t_parm_size <= 0) )	{

	return RPC_SYNTAX_FAIL;
}

/* Initializations. */
tmp_context=context;
current_parm=0;
first_parm_ind=1;
output_ind=0;
parm_tbl=t_parm;
parm_tbl_size = t_parm_size;

/* Get ready to copy name of the RPC. */
/*dest=p_rpc_name;	*/
dest_ind=0;

/* Make sure all buffer are null terminated. */
p_rpc_name[0] = '\0';
p_parm_name[0] = '\0';
p_parm_val[0] = '\0';
p_output_ind[0] = '\0';

/* RPC parsing. */
rc=ct10rpc_parse_via_automaton( (unsigned char *) chaine_rpc);

/* Copy RPC name and number of parameters to output parameter. */
if (rc == RPC_SYNTAX_OK) {
	ct10strncpy(rpc_name, p_rpc_name, rpc_name_size);
	*nb_parm=current_parm;
}

return rc;
} /* ------------------------------ End of ct10rpc_parse ---------------- */



/*
 *************************************
 Use automaton to parse RPC.
 *************************************/

int ct10rpc_parse_via_automaton(unsigned char *chaine)
{
int 	l,		/* level to use for next input character.        */
	f,		/* Processing to apply to the current character. */
	rc;		/* Functions return code. use to report suntax   */
			/* error and overflow.                           */

/* We always start at level 0. */
l = 0;

/* this loop will stop:                 */
/*	on a syntzx error.              */
/*	on an overflow of any buffer.   */
/*      on the end of the input string. */

while (1) {

	/* Check if number of characters in current string is to big. */
	/* name of stored proc, name of parameters or values must     */
	/* exceed 255.                                                */

	if (dest_ind > RPC_MAXSIZE) {
		return RPC_SYNTAX_FAIL;
	}

	/* Something is not working if value is not between 0 and 255 */
	/* (A valid printable ascii character.)                       */

	/****
	      Unsigned char may not be out of this range.
	if ((chaine[0] < 0) || (chaine[0] > 255)) {
		return RPC_SYNTAX_FAIL;
	}
	*****/

	/* Check if the ascii value from source point to a good */
	/* column in the automaton.                             */
	/* If not there is a problem with the table:            */
	/*        ascii_value_column_in_auto                    */

	if ((ascii_value_column_in_auto[(int) chaine[0]] < 0) ||
	    (ascii_value_column_in_auto[(int) chaine[0]] > 
		RPC_NB_COLUMN_AUTOMATON) ) {
			return RPC_SYNTAX_FAIL;
	}

	/* Extract operation to perform on the current character from */
	/* the automaton.                                             */

	f = rpc_automaton[l][ascii_value_column_in_auto[(int) chaine[0]]].func;

	/* Extract next level to go from the automaton. */

	l = rpc_automaton[l][ascii_value_column_in_auto[(int) chaine[0]]].level;

	#ifdef RPC_DEBUG_MODE
 	printf("Niveau: [%d], chaine: [%s], car: [%c] fnc: [%d] col: [%d]\n",
		l,
		chaine,
		chaine[0],
		f,
		ascii_value_column_in_auto[(int) chaine[0]]);
	#endif

	/* Process 1 charater from the source. */

	if ((rc=ct10rpc_process_1_rpc_char(f, chaine[0])) 
	     != RPC_SYNTAX_CONTINUE) {
		return RPC_SYNTAX_FAIL;
	}

	/* A negative level indicate the end (valid or error). */
	if (l < 0 ) {
		return l;
	}
	else {
		++chaine;
	}

} /* while */

} /* ---------------------- End of ct10rpc_parse_via_automaton -------------- */



/*
 *******************************************************
 Special Handler for parsing 1 char from the RPC string.
 *******************************************************/

int ct10rpc_process_1_rpc_char(int func, unsigned char c)
{
int rc;

rc=RPC_SYNTAX_CONTINUE;
switch (func) {
case   0:	break;
case 100:	ct10rpc_process_rpc_name_char(c);
		break;
case 101:	ct10rpc_process_fisrt_parm_name_letter(c);
		break;
case 102:	ct10rpc_process_next_parm_name_letter(c);
		break;
case 105:	ct10rpc_process_end_parm_name();
		break;
case 103:	ct10rpc_process_first_value_letter(c);
		break;
case 104:
case 107:	ct10rpc_process_next_value_letter(c);
		break;
case 106:
case 109:	ct10rpc_process_end_value();
		break;
case 108:	ct10rpc_process_first_quote();
		break;
case 110:	rc=ct10rpc_get_ready_for_next_parm(c);
		break;
case 111:	ct10rpc_process_end_value();
		rc=ct10rpc_get_ready_for_next_parm(c);
		break;
case 112:	ct10rpc_process_first_ouput_ind(toupper(c));
		break;
case 113:	ct10rpc_process_next_ouput_ind(toupper(c));
		break;
case 114:	rc=ct10rpc_process_end_ouput_ind();
		break;
case 115:	if ((rc=ct10rpc_process_end_ouput_ind()) == RPC_SYNTAX_CONTINUE) {
			rc=ct10rpc_get_ready_for_next_parm(c);
		}
		break;
case 116:	ct10rpc_process_end_rpc_name();
		break;
case 117:	ct10rpc_process_first_date_quote();
		break;
default:	rc=RPC_SYNTAX_FAIL;
		break;
}

return rc;

}/* ------------------- End of ct10rpc_process_1_rpc_char ------------------- */



/*
 *****************************************
 Single char processing functions.
 *****************************************/

/* Copy 1 letter of the RPC name. */
void ct10rpc_process_rpc_name_char(unsigned char c)
{
p_rpc_name[dest_ind++] = c;
/*++dest;*/
}


void ct10rpc_process_end_rpc_name()
{
p_rpc_name[dest_ind] = '\0';
/*dest[0] = '\0';*/
}


/* If first parameter, null terminate RPC name, set current_parm to 1. */
void ct10rpc_check_first_parm()
{
if (first_parm_ind == 1) {
	current_parm=1;
	first_parm_ind=0;
}
}


/* Initialize pointer to tempo parm name buffer and copy first char. */
void ct10rpc_process_fisrt_parm_name_letter(unsigned char c)
{
ct10rpc_check_first_parm();
dest_ind = 0;
p_parm_name[dest_ind++] = c;
}


/* Copy 1 more letter from parameter name. */
void ct10rpc_process_next_parm_name_letter(unsigned char c)
{
p_parm_name[dest_ind++] = c;
}


/* Null terminate paramter name. */
void ct10rpc_process_end_parm_name()
{
p_parm_name[dest_ind] = '\0';
}


/* Initialize pointer to tempo value buffer, copy first char.   */
/* Set quote indicator to FALSE (0).				*/

void ct10rpc_process_first_value_letter(unsigned char c)
{
ct10rpc_check_first_parm();
quote_ind=0;
dest_ind = 0;
p_parm_val[dest_ind++] = c;

}


/* Initialize pointer to tempo value buffer, copy first char.   */
/* Set quote indicator to TRUE (1).				*/

void ct10rpc_process_first_quote()
{
ct10rpc_check_first_parm();
quote_ind=1;

dest_ind = 0;

}

/* Initialize pointer to tempo value buffer, copy first char.   */
/* Set quote indicator to TRUE (1).				*/

void ct10rpc_process_first_date_quote()
{
ct10rpc_check_first_parm();
quote_ind=2;

dest_ind = 0;

}


/* Copy next caracters to tempo value buffer. */
void ct10rpc_process_next_value_letter(unsigned char c)
{
p_parm_val[dest_ind++] = c;

}


/* Null terminate parameter value buffer and call function to initialize */
/* CT10_DATA_XDEF_T structure.						 */

void ct10rpc_process_end_value()
{
p_parm_val[dest_ind++] = '\0';

}


/* Add 1 to current parameter pointer. */
int ct10rpc_get_ready_for_next_parm(unsigned char c)
{
int rc;

rc = ct10rpc_process_1_full_parm();

if (c == ',') {
	if (current_parm >= parm_tbl_size) {
		rc = RPC_SYNTAX_FAIL;
	}
	else {
		++current_parm;
	}
}
return rc;
}

void ct10rpc_process_first_ouput_ind(unsigned char c)
{
dest_ind = 0;
p_output_ind[dest_ind++] = c;

}


void ct10rpc_process_next_ouput_ind(unsigned char c)
{
p_output_ind[dest_ind++] = c;

}

int ct10rpc_process_end_ouput_ind()
{
p_output_ind[dest_ind] = '\0';


if ((strcmp(p_output_ind, "OUTPUT") == 0) || (strcmp(p_output_ind, "OUT") == 0) ) {
	output_ind = 1;
	return RPC_SYNTAX_CONTINUE;		
}
else {
	return RPC_SYNTAX_ERROR_PARM;		
}

}



/*
 ********************************************************
 Creation of the CT10_DATA_XDEF_T structure.
 ********************************************************/

int ct10rpc_process_1_full_parm()
{
CS_DATAFMT cnv;
CS_INT outl;
CS_RETCODE rc2;
int initial_size_zero=FALSE;

/* Allocate CT10_DATA_XDEF_T data structure. */
if (( (parm_tbl[current_parm-1]) = calloc(1,sizeof(CT10_DATA_XDEF_T))) 
	== NULL ) {
	return RPC_SYNTAX_FAIL;
}

(parm_tbl[current_parm-1])->fmt.count = 1;
(parm_tbl[current_parm-1])->fmt.locale = NULL;

/******************************
 Checking for NULL value. 
 ******************************/

(parm_tbl[current_parm-1])->null_ind = 0;

/* This is for compatibility with version 1 only. */
if (strcmp(p_parm_val, RPC_NULL_DEFAULT) == 0) {
	(parm_tbl[current_parm-1])->null_ind = -1;
	if (quote_ind == 0) {
		(parm_tbl[current_parm-1])->fmt.datatype = CS_INT_TYPE;
	}
	else if (quote_ind == 1) {
		(parm_tbl[current_parm-1])->fmt.datatype = CS_CHAR_TYPE;
	}
	else if (quote_ind == 2) {
		(parm_tbl[current_parm-1])->fmt.datatype = CS_DATETIME_TYPE;
	}
}
else if (quote_ind == 0) {
/* Version 2 Null value identification. Only if not quoted. */

	if (strcmp(p_parm_val,RPC_CS_CHAR_NULL) == 0) {
		(parm_tbl[current_parm-1])->null_ind = -1;
		(parm_tbl[current_parm-1])->fmt.datatype = CS_CHAR_TYPE;
	}
	else if (strcmp(p_parm_val,RPC_CS_INT_NULL) == 0) {
		(parm_tbl[current_parm-1])->null_ind = -1;
		(parm_tbl[current_parm-1])->fmt.datatype = CS_INT_TYPE;
	}
	else if (strcmp(p_parm_val,RPC_CS_FLOAT_NULL) == 0) {
		(parm_tbl[current_parm-1])->null_ind = -1;
		(parm_tbl[current_parm-1])->fmt.datatype = CS_FLOAT_TYPE;
	}
	else if (strcmp(p_parm_val, RPC_CS_DATETIME_NULL) == 0) {
		(parm_tbl[current_parm-1])->null_ind = -1;
		(parm_tbl[current_parm-1])->fmt.datatype = CS_DATETIME_TYPE;
	}
	
} /* if quote_ind == 0 */

/* If it's not a null value we need to find the data type. */
if ((parm_tbl[current_parm-1])->null_ind == 0) {
	if (quote_ind == 1) {
		(parm_tbl[current_parm-1])->fmt.datatype = CS_CHAR_TYPE;
	}
	else if (quote_ind == 2) {
		(parm_tbl[current_parm-1])->fmt.datatype = CS_DATETIME_TYPE;
	}
	else if (cns_check_int_string(p_parm_val)==CNS_VALID) {
		(parm_tbl[current_parm-1])->fmt.datatype = CS_INT_TYPE;
	}
	else if (cns_check_float_string(p_parm_val)==CNS_VALID) {
		(parm_tbl[current_parm-1])->fmt.datatype = CS_FLOAT_TYPE;
	}
	else {
		(parm_tbl[current_parm-1])->fmt.datatype = CS_CHAR_TYPE;
	}
}

/* Set size of buffer to hold data. */
switch ((parm_tbl[current_parm-1])->fmt.datatype) {
	case CS_INT_TYPE:	(parm_tbl[current_parm-1])->len = 4;
				(parm_tbl[current_parm-1])->fmt.maxlength = 4;
				break;
	case CS_FLOAT_TYPE:	(parm_tbl[current_parm-1])->len = 8;
				(parm_tbl[current_parm-1])->fmt.maxlength = 8;
				break;
	case CS_CHAR_TYPE:	(parm_tbl[current_parm-1])->len = 
				strlen(p_parm_val);

				/* An empty string must be process as a */
				/* string of len 1.			*/
				if ((parm_tbl[current_parm-1])->len == 0) {
				    initial_size_zero=TRUE;
				    ++(parm_tbl[current_parm-1])->len;
				}

				/*
				 * The maxlength parameter thing is known
				 * to be fixed in Sybase 12, so set it
				 * to CS_MAX_CHAR - 1 for this version, and
				 * onward, otherwise, use the "regular"
				 * patch code for previous Sybase version.
				 *
				 * By the way, a ct_lib defines the
				 * CS_VERSION_XXX version for its version
				 * AND all previous version, so the fix should
				 * work for all subsequent version.
				 */
				(parm_tbl[current_parm-1])->fmt.maxlength =
#ifdef CS_VERSION_120
					CS_MAX_CHAR - 1;
#else
					(parm_tbl[current_parm-1])->len+1;
#endif


				break;
	case CS_DATETIME_TYPE:	
#ifndef TRGX
				(parm_tbl[current_parm-1])->len = 24;
				(parm_tbl[current_parm-1])->fmt.maxlength = 24;
#else
				(parm_tbl[current_parm-1])->len = 8;
				(parm_tbl[current_parm-1])->fmt.maxlength = 8;
#endif
				break;
	default:		return RPC_SYNTAX_FAIL;
				break;
}


/* Allocate buffer. */
if (((parm_tbl[current_parm-1])->p_val=
		(void *) calloc(1, (parm_tbl[current_parm-1])->fmt.maxlength )) 
	== NULL ) {
	return RPC_SYNTAX_FAIL;
}

/* Copy parameter name. */
ct10strncpy((parm_tbl[current_parm-1])->fmt.name, p_parm_name, CS_MAX_NAME);

/* Set parameter name size. */
if (strlen(p_parm_name) == 0) {
	(parm_tbl[current_parm-1])->fmt.namelen=0;
}
else {
	(parm_tbl[current_parm-1])->fmt.namelen=CS_NULLTERM;
}

/* Set output-input parameter. */
if (output_ind == 0) {
	(parm_tbl[current_parm-1])->fmt.status = CS_INPUTVALUE;
}
else {
	(parm_tbl[current_parm-1])->fmt.status = CS_RETURN;
}

if ((parm_tbl[current_parm-1])->null_ind != -1) {
/* Copy value to buffer. */
switch ((parm_tbl[current_parm-1])->fmt.datatype) {
/****
#ifdef TRGX
	case CS_DATETIME_TYPE:
				return RPC_SYNTAX_FAIL;
				break;
	case CS_FLOAT_TYPE:
				*((float*) parm_tbl[current_parm-1]->p_val) = atof(p_parm_val);

				 Set format to unused. 
				(parm_tbl[current_parm-1])->fmt.format = 
					CS_FMT_UNUSED;
				break;
	case CS_INT_TYPE:
				*((int*) parm_tbl[current_parm-1]->p_val) = atoi(p_parm_val);

				 Set format to unused. 
				(parm_tbl[current_parm-1])->fmt.format = 
					CS_FMT_UNUSED;
				break;

#else
**** */
	case CS_DATETIME_TYPE:
	case CS_FLOAT_TYPE:
	case CS_INT_TYPE: 	memset(&cnv,0,sizeof(CS_DATAFMT));
				cnv.datatype = CS_CHAR_TYPE;
				cnv.maxlength = strlen(p_parm_val);
				cnv.locale = NULL;
				rc2=cs_convert(tmp_context, &cnv, p_parm_val,
				   	&((parm_tbl[current_parm-1])->fmt),
				   	(parm_tbl[current_parm-1])->p_val,
				   	&outl);
				if (rc2 == CS_FAIL) {
					return RPC_SYNTAX_FAIL;
				}
				/* Set format to unused. */
				(parm_tbl[current_parm-1])->fmt.format = 
					CS_FMT_UNUSED;
				break;
	case CS_CHAR_TYPE:


				if (initial_size_zero == TRUE) {

				ct10strncpy(
				    (char *)  (parm_tbl[current_parm-1])->p_val,
				    " ",
				    (parm_tbl[current_parm-1])->fmt.maxlength);
				}
				else {

				ct10strncpy(
				    (char *)  (parm_tbl[current_parm-1])->p_val,
				    p_parm_val,
				    (parm_tbl[current_parm-1])->fmt.maxlength);
				}

				/* Set format to nullterm. */
				(parm_tbl[current_parm-1])->fmt.format = 
					CS_FMT_NULLTERM;
				break;
}
}

/* Getting ready for next parameter. */
output_ind = 0;
p_parm_name[0] = '\0';
p_parm_val[0] = '\0';
p_output_ind[0] = '\0';

return RPC_SYNTAX_CONTINUE;

} /* --------------- End of ct10rpc_process_1_full_parm -------------- */



/*
 ***************************************************************
 Dumping CT10_DATA_XDEF_T structure to stdout.
 ***************************************************************/

void ct10rpc_dump(	char		  *rpc_name,
			CT10_DATA_XDEF_T  *t_parm[],
			int               nb_parm)
{
int x;

printf("RPC Name: [%s]\n", rpc_name);

for (x=0; x<nb_parm; ++x)
{
	printf("%d name[%s] len[%ld] ",x+1,t_parm[x]->fmt.name,t_parm[x]->fmt.namelen);
	switch ((t_parm[x])->fmt.datatype) {
		case CS_CHAR_TYPE:
				printf("type[C] val[%.*s] ",t_parm[x]->len,
						   	    t_parm[x]->p_val);
				break;
		case CS_INT_TYPE:
				printf("type[I] val[%ld] ",
					*((CS_INT *) t_parm[x]->p_val));
				break;
		case CS_SMALLINT_TYPE:
				printf("type[I] val[%d] ",
					*((CS_SMALLINT *) t_parm[x]->p_val));
				break;
		case CS_FLOAT_TYPE:
				printf("type[F] val[%f] ",
					*((CS_FLOAT *) t_parm[x]->p_val));
				break;
		case CS_DATETIME_TYPE:
				printf("type[D] ");
				break;
	}
	printf("len[%ld] null[%d] stat[%ld]\n",
				t_parm[x]->len,
				t_parm[x]->null_ind,
				t_parm[x]->fmt.status);
}


} /* -------------------- End of ct10rpc_dump --------------------------- */
