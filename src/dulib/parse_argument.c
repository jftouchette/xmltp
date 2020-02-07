/*****************************************************************************
 * parse_argument.c
 *
 *
 *  Copyright (c) 1995-2003 Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/dulib/parse_argument.c,v $
 * $Header: /ext_hd/cvs/dulib/parse_argument.c,v 1.3 2004/10/13 18:23:30 blanchjj Exp $
 *
 *    Librairie de fonctions servants a la gestion (standard) des argument de
 *    la ligne de commnade.
 *   
 *   
 *    Format supporte par cette librairie:
 *   
 *    flag:	Une lettre (presente: vrai, absente: faux)
 *    argument:	Une lettre avec une valeur
 *   
 *    Exemple: program -a -b -c 123 -d "argument d"
 *   
 *   		les flags -a et -d sont a vrai
 *   		l'argument -c = 123 et -d = "argument d"
 *   
 * MODIFICATIONS:
 * 1995/07/10, jbl: first version
 * 2001/11/26, deb: Initialisation de la structure à NB_ARGUMENT + 1 au lieu de NB_ARGUMENT
 */  



#include	<stdio.h>
#include	"parse_argument.h"


/* Structure privee a ce module renfermant les arguments */

/*
 * Initialisation de la structure à NB_ARGUMENT + 1 au lieu de NB_ARGUMENT
 * parce que la boucle de pa_parse_argument() est de 1 à NB_ARGUMENT
 * inclusivement!!!
 */
static struct S_ARGUMENT t_arg[NB_ARGUMENT + 1];



/*
*****************************************************************************

 Decodage standard des argumnents donnees a un programme.

*****************************************************************************/

int pa_parse_argument(int ac, char **av, char *list_arg)
{
extern char *optarg;	/* Use by getopt to parse arguments. */
extern int optind;	/* Use by getopt to parse arguments. */
int opt;		/* Use by getopt to parse arguments. */

char *i;		/* Utilise pour circuler sur list_arg. */

int rc = 0;		/* Code de retour de la fonction. */


	/*
 	 * Initialisation de la table d'arguments.
 	 */

	for (opt = 1;opt <= NB_ARGUMENT;opt++) {
		t_arg[opt].flag = NOTSET;	
		t_arg[opt].val = NULL;
	}

	/*
	 * Arguments possibles a FALSE.
	 */

	for (i=list_arg;*i!=0;i++) {
		t_arg[*i].flag = FALSE;
	}

	/*
	 * Decodage des arguments. 
	 */

	while (((opt = getopt(ac, av, list_arg)) != -1 ) && (rc == 0) ) {

		/*
		 * Sur ? on desire imprimer la syntaxe.
		 * Un RC de -1 indique au pgm maitre de faire ceci.
		 */

		if (opt == '?') {
			rc = -1;
			break;
		}
		else {
			/*
			 * Le flag est-il dans la liste des flag valide
			 * pour le programme.
			 */

			for (i=list_arg;*i!=0;i++) {
				/*
				 * Si le flag est valide.
				 */
				if (opt == *i) {
						/*
						 * Enregistrer flag et argument.
						 */
						t_arg[opt].flag = TRUE;	
						++i;
						if (*i == ':') {
							t_arg[opt].val = optarg;
						}
						break;
					}
				}
			}

			if (*i == 0) {
				rc = -3;
				break;
			}



	} /* while opt = getopt */

return rc;
}


/*
*****************************************************************************

 Modification d'un FLAG.

*****************************************************************************/

void pa_set_flag(char opt, int flag)
{
	t_arg[opt].flag = flag;
}


/*****************************************************************************

 Lecture d'un FLAG.

 *****************************************************************************/

int pa_get_flag(char opt)
{
	return t_arg[opt].flag;
}


/*****************************************************************************

  Modification de la valeur d'un argument.

 *****************************************************************************/

void pa_set_arg(char opt, char *val)
{
	t_arg[opt].val = val;
}


/*****************************************************************************

 Lecture de la valeur d'un argument.

 ******************************************************************************/

char *pa_get_arg(char opt)
{
	return t_arg[opt].val;
}
