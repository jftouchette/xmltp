/*
 *****************************************************************************
 * check_num_string.c (CNS)
 *
 *  Copyright (c) 1996-2001 Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/dulib/check_num_string.c,v $
 * $Header: /ext_hd/cvs/dulib/check_num_string.c,v 1.2 2004/10/13 18:23:29 blanchjj Exp $
 *
 * These functions use automaton to validate numerical (float and int) string.
 *
 * LIST OF CHANGES:
 * 1996-04-04, blanchjj:	Initial creation.
 * 1996-04-29, blanchjj:  Change to float automaton,accept float ending with '.'
 */


#include <stdio.h>
#include "check_num_string.h"

#define NB_LIGNE_AUTOMATON	20
#define NB_COLUMN_AUTOMATON	20

/*
 *********************************************************
 This array matches an ascii value with the corresponding
 column in the 'automaton'. The ascii value is use to find
 the offset in the table.

 This table is good for float and int type validation.
 *********************************************************/

int float_string_ascii_column[] = {
/* ascii: \0 (0)  */ 5,
/* ascii:  (1)  */ 6, 
/* ascii:  (2)  */ 6, 
/* ascii:  (3)  */ 6, 
/* ascii:  (4)  */ 6, 
/* ascii:  (5)  */ 6, 
/* ascii:  (6)  */ 6, 
/* ascii:  (7)  */ 6, 
/* ascii:  (8)  */ 6, 
/* ascii:  (9)  */ 6, 
/* ascii: (10)  */ 6, 
/* ascii: (11)  */ 6, 
/* ascii:  (12)  */ 6, 
/* ascii:  (13)  */ 6, 
/* ascii:  (14)  */ 6, 
/* ascii:  (15)  */ 6, 
/* ascii:  (16)  */ 6, 
/* ascii:  (17)  */ 6, 
/* ascii:  (18)  */ 6, 
/* ascii:  (19)  */ 6, 
/* ascii:  (20)  */ 6, 
/* ascii:  (21)  */ 6, 
/* ascii:  (22)  */ 6, 
/* ascii:  (23)  */ 6, 
/* ascii:  (24)  */ 6, 
/* ascii:  (25)  */ 6, 
/* ascii:  (26)  */ 6, 
/* ascii:  (27)  */ 6, 
/* ascii:  (28)  */ 6, 
/* ascii:  (29)  */ 6, 
/* ascii:  (30)  */ 6, 
/* ascii:  (31)  */ 6, 
/* ascii:   (32)  */ 0, 
/* ascii: ! (33)  */ 6, 
/* ascii: " (34)  */ 6, 
/* ascii: # (35)  */ 6, 
/* ascii: $ (36)  */ 6, 
/* ascii: % (37)  */ 6, 
/* ascii: & (38)  */ 6, 
/* ascii: ' (39)  */ 6, 
/* ascii: ( (40)  */ 6, 
/* ascii: ) (41)  */ 6, 
/* ascii: * (42)  */ 6, 
/* ascii: + (43)  */ 2, 
/* ascii: , (44)  */ 6, 
/* ascii: - (45)  */ 3, 
/* ascii: . (46)  */ 4, 
/* ascii: / (47)  */ 6, 
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
/* ascii: : (58)  */ 6, 
/* ascii: ; (59)  */ 6, 
/* ascii: < (60)  */ 6, 
/* ascii: = (61)  */ 6, 
/* ascii: > (62)  */ 6, 
/* ascii: ? (63)  */ 6, 
/* ascii: @ (64)  */ 6, 
/* ascii: A (65)  */ 6, 
/* ascii: B (66)  */ 6, 
/* ascii: C (67)  */ 6, 
/* ascii: D (68)  */ 6, 
/* ascii: E (69)  */ 6, 
/* ascii: F (70)  */ 6, 
/* ascii: G (71)  */ 6, 
/* ascii: H (72)  */ 6, 
/* ascii: I (73)  */ 6, 
/* ascii: J (74)  */ 6, 
/* ascii: K (75)  */ 6, 
/* ascii: L (76)  */ 6, 
/* ascii: M (77)  */ 6, 
/* ascii: N (78)  */ 6, 
/* ascii: O (79)  */ 6, 
/* ascii: P (80)  */ 6, 
/* ascii: Q (81)  */ 6, 
/* ascii: R (82)  */ 6, 
/* ascii: S (83)  */ 6, 
/* ascii: T (84)  */ 6, 
/* ascii: U (85)  */ 6, 
/* ascii: V (86)  */ 6, 
/* ascii: W (87)  */ 6, 
/* ascii: X (88)  */ 6, 
/* ascii: Y (89)  */ 6, 
/* ascii: Z (90)  */ 6, 
/* ascii: [ (91)  */ 6, 
/* ascii: \ (92)  */ 6, 
/* ascii: ] (93)  */ 6, 
/* ascii: ^ (94)  */ 6, 
/* ascii: _ (95)  */ 6, 
/* ascii: ` (96)  */ 6, 
/* ascii: a (97)  */ 6, 
/* ascii: b (98)  */ 6, 
/* ascii: c (99)  */ 6, 
/* ascii: d (100)  */ 6, 
/* ascii: e (101)  */ 6, 
/* ascii: f (102)  */ 6, 
/* ascii: g (103)  */ 6, 
/* ascii: h (104)  */ 6, 
/* ascii: i (105)  */ 6, 
/* ascii: j (106)  */ 6, 
/* ascii: k (107)  */ 6, 
/* ascii: l (108)  */ 6, 
/* ascii: m (109)  */ 6, 
/* ascii: n (110)  */ 6, 
/* ascii: o (111)  */ 6, 
/* ascii: p (112)  */ 6, 
/* ascii: q (113)  */ 6, 
/* ascii: r (114)  */ 6, 
/* ascii: s (115)  */ 6, 
/* ascii: t (116)  */ 6, 
/* ascii: u (117)  */ 6, 
/* ascii: v (118)  */ 6, 
/* ascii: w (119)  */ 6, 
/* ascii: x (120)  */ 6, 
/* ascii: y (121)  */ 6, 
/* ascii: z (122)  */ 6, 
/* ascii: { (123)  */ 6, 
/* ascii: | (124)  */ 6, 
/* ascii: } (125)  */ 6, 
/* ascii: ~ (126)  */ 6, 
/* ascii:  (127)  */ 6 
};


/*
 ******************************************************************
 Float string automaton:

	Valid Values Examples:

		"   +12345.6789  "
		"   -123.567     "
		"    123.567     "
		"    .567        "
		"    -.567       "
		"    +.567       "
		"    +.0         "
		"   12345        "
 ******************************************************************/

int float_string_automaton[NB_LIGNE_AUTOMATON][NB_COLUMN_AUTOMATON] = {

		/* SPACE	DIGIT	+	-	.	\0	*    */

	/* 0 */	  { +0,		+4,	+1,	+1,	+2,	-1,	-1},
	/* 1 */	  { -1,		+4,	-1,	-1,	+2,	-1,	-1},
	/* 2 */	  { +5,		+3,	-1,	-1,	-1,	-2,	-1},
	/* 3 */	  { +5,		+3,	-1,	-1,	-1,	-2,	-1},
	/* 4 */	  { +5,		+4,	-1,	-1,	+2,	-2,	-1},
	/* 5 */	  { +5,		-1,	-1,	-1,	-1,	-2,	-1}
		};



/******************************************************************
 INT string automaton.

 Valid Values Examples:

	" 123345 "
	"-12345"
	"+12345"
	"+0"
	"-0"
 
 ******************************************************************/

int int_string_automaton[NB_LIGNE_AUTOMATON][NB_COLUMN_AUTOMATON] = {

		/* SPACE	DIGIT	+	-	.	\0	*    */

	/* 0 */	  { +0,		+4,	+1,	+1,	-1,	-1,	-1},
	/* 1 */	  { -1,		+4,	-1,	-1,	-1,	-1,	-1},
	/* 2 */	  { -1,		+3,	-1,	-1,	-1,	-1,	-1},
	/* 3 */	  { +5,		+3,	-1,	-1,	-1,	-2,	-1},
	/* 4 */	  { +5,		+4,	-1,	-1,	-1,	-2,	-1},
	/* 5 */	  { +5,		-1,	-1,	-1,	-1,	-2,	-1}
		};


/*
 ******************************************************************
 Genaral automaton evaluation function.

 ******************************************************************/
int cns_check_string(int tbl[],
		int automaton[NB_LIGNE_AUTOMATON][NB_COLUMN_AUTOMATON],
		int niveau,
		char *chaine)
{
int i;
int rc;

i = automaton[niveau][tbl[(int) chaine[0]]];

/***************************************************************************
 For debugging remove this comment.
 ==================================

 printf("Niveau: [%d], chaine: [%s], car: [%c] rc: [%d] col: [%d]\n",
	niveau,
	chaine,
	chaine[0],
	i,
	tbl[(int) chaine[0]]);
 ***************************************************************************/

if (i < 0 )
	return i;
else
	rc=cns_check_string(tbl,automaton,i,++chaine);

return rc;
}


/*
 ***************************************************************************
 FLOAT string validation
	-1: valid
	-2: invalid
 ***************************************************************************/

int cns_check_float_string(char *chaine)
{

return (cns_check_string(	float_string_ascii_column,
				float_string_automaton,
				0,
				chaine));
}

/*
 ***************************************************************************
 INT string validation
	-1: valid
	-2: invalid
 ***************************************************************************/


int cns_check_int_string(char *chaine)
{

return (cns_check_string(	float_string_ascii_column,
				int_string_automaton,
				0,
				chaine));
}


/**********************************************************************
 Use that main code to test new automaton.
 =========================================

main(argc,argv)
int argc;
char **argv;
{
int  rc;


rc = check_int_string(argv[1]);

printf("Chiffre: [%s], rc: %d\n",argv[1],rc);


}

 **********************************************************************/
