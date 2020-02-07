/*****************************************************************************
 * parse_argument.h
 *
 *
 *  Copyright (c) 1995 Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/include/parse_argument.h,v $
 * $Header: /ext_hd/cvs/include/parse_argument.h,v 1.2 2004/10/13 18:23:30 blanchjj Exp $
 *
 *  Fichier a inclure pour l'utilisation de la librairie standard de decodage
 *  des arguments de la ligne de commande.
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
 *  LISTE DES PARAMETRES:
 * 
 *  ac:		Nombre d'arguments.
 *  av:		Pointeur sur la liste des arguments.
 *  list_arg:	Liste des argument donnees a getopts. ex: ":U:S:P:t?"
 *  opt:		Un caractere representant le flag.
 *  flag:		Valeur du flag: TRUE,FALSE,NOTSET.
 *  val:		Valeur d'un argument.
 * 
 * MODIFICATIONS:
 * 1995/07/10, jbl: first version
 */


#define NOTSET -1
#define FALSE 0
#define TRUE 1

#define NB_ARGUMENT 256

struct S_ARGUMENT {
	int flag;
	char *val;
};

int pa_parse_argument(int ac,char **av,char *list_arg);

void pa_set_flag(char opt, int flag);

int pa_get_flag(char opt);

void pa_set_arg(char opt, char *val);

char *pa_get_arg(char opt);
