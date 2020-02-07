/* ucfg.c			(To Read a UNIX style CONFIGURATION file)
 * ------
 *
 *  Copyright (c) 1994-2001 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/cfg_read/ucfg.c,v $
 * $Header: /ext_hd/cvs/cfg_read/ucfg.c,v 1.11 2004/10/13 18:23:21 blanchjj Exp $
 *
 *
 * Public Functions prefix:	ucfg_
 *
 *
 * DESCRIPTION:
 *
 * This file contains the implementation of the functions to read a 
 * UNIX style CONFIGURATION file.
 *
 * Public functions prototypes and descriptions and public constants
 * (defines) are in "ucfg.h"
 *
 *
 * LIMITATIONS:
 *
 * The design of this module assumes that each executable program that uses
 * it only use ONE config file. (The module only keeps the values for the
 * last config file opened!)
 *
 * The maximum length of a parameter value (string) is 200 bytes
 * (UCFG_MAX_VALUE_LEN).
 *
 * This first version associates only one value to each parameter.
 *
 * The string returned by ucfg_get_param_string_first/_next() is a pointer to 
 * a static area (where the value has been copied). Consequently, using
 * ucfg_get_param_string_first/_next(x) twice or more as arguments of a 
 * function call will NOT give the expected results.
 *
 * Parameter value should not contain embedded spaces or tabs.
 *
 * ---------------------------------------------------------------------------
 * Versions:
 * 1994Aug02,jft: first implementation
 * 1995oct18,jft: + ucfg_is_string_in_param_values_list_pattern(...)
 * 1997aug22,deb: Added ucfg_open_cfg_at_section_ext()
 * 2001jan03,deb: Adapted s_fi_substitution_report definition to linux
 */

#include "common_1.h"	/* for TRUE and FALSE */

#ifdef WIN32
#include "ntunixfn.h"
#endif

#include <stdio.h>
#include <stdlib.h>	/* mem functions prototypes */
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "linklist.h"
#include "ucfg.h"


#ifndef WIN32
char *strdup(const char *s);		/* missing prototype */
#endif



/* ------------------------------------------------ Private defines: */

#define A_QUOTE			'"'
#define EMPTY_STRING		"\"\""

#define COMMENT_CHAR		'#'
#define CONTINUATION_CHAR	'\\'

#ifdef VERSION_ID
#undef VERSION_ID
#define VERSION_ID	(__FILE__ ": " __DATE__ " - " __TIME__)
#endif

/* #define TEST_DEBUG	yes
 */


#define SECTION_NAME_FOUND	1
#define END_OF_SECTION		5



/* ----------------------------------------------- Private variables: */

/* error codes, status and counters:
 */
static int 	s_ucfg_error_code		= 0,
		s_ucfg_errno			= 0,
		s_param_loaded			= 0,
		s_nb_params			= 0;


/* booleans that modify the behaviour of the loading:
 */
static int	s_reject_second_param_def		= 1,
		s_keep_old_param_values_if_open_fails	= 0;


/* File pointer used to report the new values of duplicate parameter
 * when duplicate are allowed
 */
#ifndef __GNUC__
static FILE *s_fi_substitution_report = stderr;
#else
static FILE *s_fi_substitution_report = NULL;
#endif /* ifndef linux */


static struct st_param_node {
	struct st_param_node *p_left,
			     *p_right;
	
	char		 *id_param;

	void		 *val_list;	/* from "linklist.h" */
	
 } s_param_tree_root = {
 	NULL, NULL,
 	"Master - dummy\t",	/* 'M' middle of uppercase sort order */
	NULL
 };




/* ------------------------------------------------ Private functions: */


static int string_is_number(char *s)
{
 char	*valid_chars = "1234567890.,";

 for ( ; *s; s++) {
	if (strchr(valid_chars, *s) == NULL) {
		return (0);
	}
 }
 return (1);
}
 



static char *remember_bad_param_name(char *id_param, int reset_ind)
{
 static	char	bad_param[UCFG_MAX_PARAM_NAME_LEN + 1];

 if (id_param == NULL) {
 	if (!*bad_param) {
 		return (NULL);	/* indicate no bad param found */
 	}
 	return (bad_param);
 }
 if (reset_ind) {		/* cancel previous bad_param[] */
 	bad_param[0] = '\0';
 	
 } else if (!*bad_param) {	/* memorize only if not set yet */

 	strncpy(bad_param, id_param, UCFG_MAX_PARAM_NAME_LEN);
 	bad_param[UCFG_MAX_PARAM_NAME_LEN] = '\0';
 }
 return (bad_param);

} /* end of remember_bad_param_name() */




static int note_error(int errnum, char *param_name)
{
 extern int	errno;

 s_ucfg_error_code = errnum;
 s_ucfg_errno	   = errno;

 if (param_name != NULL) {
	remember_bad_param_name(param_name, 0);
 }
#ifdef TEST_DEBUG
 fprintf(stdout, "\nError (ucfg.c): err=%d, errno=%d, param=%s.\n", 
 	errnum,
 	errno, 
 	(param_name == NULL) ? "nil" : param_name);
#endif

 return (errnum);

} /* end of note_error() */





static void *find_param_val(struct st_param_node *p_node,
			    char		  id_param[])
{
 int	compar;

 if (p_node == (struct st_param_node *) NULL ) {
	return ( NULL );
 }

 compar = strncmp(id_param, p_node->id_param, UCFG_MAX_PARAM_NAME_LEN);

 if (compar < 0)
	return (find_param_val(p_node->p_left, id_param) );

 if (compar > 0)
	return (find_param_val(p_node->p_right, id_param) );

 return ( p_node->val_list );

} /* end of find_param_val() */




static int assign_param_id_to_node(struct st_param_node *p_node, 
				   char id[])
{
 if (p_node->id_param != NULL) {
  	return (note_error(UCFG_PARAM_DUPLICATED, p_node->id_param) );
 }
 p_node->id_param = strdup(id);

 if (p_node->id_param == NULL) {
  	return (note_error(UCFG_OUT_OF_MEMORY, id) );
 }
 return (0);

} /* end of assign_param_id_to_node() */




static int destroy_values_list(void *values_list)
{
 void	*p_datum;

 for (p_datum = lkls_get_datum_in_list_first(values_list);
      p_datum != NULL;
      p_datum = lkls_get_datum_in_list_next(values_list) ) {
	free(p_datum);
 }
 return (lkls_destroy_linked_list(values_list) );

} /* end of destroy_values_list() */





static void report_new_values(char *id_param, char *values_string)
{
	if ((s_fi_substitution_report != NULL) &&
	    (id_param != NULL) &&
	    (values_string != NULL)) {

		fprintf(s_fi_substitution_report,
			"Value of parameter [%.*s] changed to: %.50s%s\n",
			UCFG_MAX_PARAM_NAME_LEN, id_param, values_string,
			strlen(values_string) > 50 ? "..." : "");
	}
} /* end of report_new_values() */





static int assign_param_val_to_node(struct st_param_node *p_node, 
				    void  *values_list, char *values_string)
{
 int	rc = 0;

 if (p_node->val_list != NULL
  && s_reject_second_param_def) {
  	return (note_error(UCFG_PARAM_DUPLICATED, p_node->id_param) );
 }
 if (p_node->val_list != NULL) {
	rc = destroy_values_list(p_node->val_list);
	if (rc != 0) {
		return (note_error(UCFG_LINKLIST_ERROR, p_node->id_param) );
	}

	report_new_values(p_node->id_param, values_string);
 }
 p_node->val_list = values_list;

 return (0);

} /* end of assign_param_val_to_node() */




static int insert_param_def(struct st_param_node *p_node, char id_param[],
			    void *values_list, char *values_string)
{
 struct st_param_node	*p_prev,
			*p_new;
 int			 compar,
			 rc;

 p_prev = p_node;

 while (p_node != (struct st_param_node *) NULL ) {

	p_prev = p_node;

	compar = strncmp(id_param, p_node->id_param, UCFG_MAX_PARAM_NAME_LEN);
	if (compar < 0) {
		p_node = p_node->p_left;
	} else if (compar > 0) {
		p_node = p_node->p_right;
	} else {
		return (assign_param_val_to_node(p_node, values_list,
			values_string) );
	}
 }
 if (compar != 0) {
	p_new = (struct st_param_node *) 
		  calloc(1, sizeof(struct st_param_node) );
	if (p_new == NULL) {
		return (note_error(UCFG_OUT_OF_MEMORY, id_param) );
	}
	if ((rc = assign_param_id_to_node(p_new, id_param)) != 0) {
		return (rc);
	}
	if ((rc = assign_param_val_to_node(p_new, values_list,
		values_string)) != 0) {

		return (rc);
	}
	s_nb_params++;

	if (compar < 0)
		p_prev->p_left = p_new;
	else if (compar > 0)
		p_prev->p_right = p_new;
 }
 return (0);

} /* end of insert_param_def() */




static void free_param_tree(struct st_param_node *p_node)
{
 if (p_node == (struct st_param_node *) NULL)
	return;
	
 free_param_tree(p_node->p_left);
 free_param_tree(p_node->p_right);

 if (p_node != &s_param_tree_root) {	/* any other node but the root */
 	free(p_node->id_param);
 	destroy_values_list(p_node->val_list);
 	free(p_node);
 } else {				/* root is a special case */
	p_node->p_left = NULL;
	p_node->p_right = NULL;
 }

} /* end of free_param_tree() */





static int is_tab_or_space(int	c)
{
	return (c == ' ' ||  c == '\t');
}




static char *skip_spaces(char *p)
{
	while (*p && is_tab_or_space(*p) ) {	/* skip to next space or tab */
		p++;
	}
	return (p);
}




static char *skip_over_non_space(char *p)
{
	while (*p && !is_tab_or_space(*p) ) {	 /* finds next non space-tab */
		p++;
	}
	return (p);
}





static int find_continuation_char(char *a_buff, int nb_bytes)
/* 
 * used by: get_one_line_skip_comments()
 */
{
 int	i;

 for (i = 0; i < nb_bytes; i++) {
	if (a_buff[i] == CONTINUATION_CHAR) {
		return (i);
	}
 }
 return (-1);	/* not found */

} /* end of find_continuation_char() */






static char *get_one_line_skip_comments(FILE *fi)
/*
 * Read one logical line (physical line can be continued with a '\'),
 * skipping empty lines and comments (begin with a COMMENT_CHAR aka '#').
 *
 * Returns NULL on EOF.
 */
{
 static	char	 a_line[UCFG_MAX_LINE_LEN + 4];
	char	*p,
		*p_read_into;
 	int	 nb_bytes_filled	= 0,
 		 max_next_read		= 0,
 		 nb_bytes_read		= 0,
 		 is_continuation	= 0,
 		 offset_continuation_char = -1;

 if (fi == NULL  ||  feof(fi) ) {
	return (NULL);
 }
 while (1) {
	max_next_read = UCFG_MAX_LINE_LEN - nb_bytes_filled;

	p_read_into = &a_line[nb_bytes_filled];

	nb_bytes_read = read_one_line(p_read_into, max_next_read, fi);

	if (nb_bytes_read < 0) {		/* EOF */
		break;
	}
	offset_continuation_char = find_continuation_char(p_read_into, 
							  nb_bytes_read);

	if (!is_continuation) {
		p = skip_spaces(p_read_into);
		if (strlen(p) == 0) {
			continue;	/* skip empty (white) line */ 
		}
		if (*p == COMMENT_CHAR) {
		 	continue;	/* skip comments */
		}
	}
	if (offset_continuation_char >= 0) {
		nb_bytes_read = offset_continuation_char;
			/* forget all bytes from the continuation char
			 * and after.
			 */
		is_continuation = 1;	  /* set flag for next iteration */
	} else {
		is_continuation = 0;	  /* set flag for break... */
	}

	nb_bytes_filled += nb_bytes_read;	 /* update count */

	if (!is_continuation) {
		break;
	}
	if (nb_bytes_filled >= UCFG_MAX_LINE_LEN) {
		break;				     /* line buffer full */
	}
 } /* end of the loop to find a line - continue line */
 
 if (nb_bytes_filled <= 0) {			/* empty line, EOF */
	return (NULL);
 }
 if (nb_bytes_filled < UCFG_MAX_LINE_LEN) {
	 a_line[nb_bytes_filled] = '\0';	/* terminate string */
 }
 return (a_line);
 
} /* end of get_one_line_skip_comments() */




static char *get_a_section_name_line(FILE *fi)
{
 char *p_line;

 while ((p_line = get_one_line_skip_comments(fi)) != NULL) {

	if (!is_tab_or_space(*p_line))
	 	break;
 }
 return (p_line);

} /* end of get_a_section_name_line() */




static char *get_next_section_name_in_line(char *a_line, int line_has_changed)
{
 static char	 a_section_name[UCFG_MAX_SECTION_NAME_LEN + 1];
 static	char	*p_prev_line = NULL,
		*p_prev_pos  = NULL;
	char	*p;

 if (line_has_changed) {
	p_prev_line = a_line;
	p_prev_pos  = NULL;
 }
 if (p_prev_pos == NULL) {
	p_prev_pos = p_prev_line;
 }
 p = skip_spaces(p_prev_pos);

 if (!*p) {			/* end of string */
	p_prev_pos = p;
	return (NULL);
 }
 p_prev_pos = skip_over_non_space(p);	/* position for next call */

 /* make a copy of the section name:
  */
 strncpy(a_section_name, p, UCFG_MAX_SECTION_NAME_LEN);

 p = skip_over_non_space(a_section_name);

 *p = '\0';	/* terminate string after section name */

 return (a_section_name);

} /* end of get_next_section_name_in_line() */
 




static int find_section_name_in_file(char *section_name, FILE *fi)
{
 char	*a_line,		/* line with section name(s) */
	*a_section_name;
 int	 line_is_new;

 while ( (a_line = get_a_section_name_line(fi)) != NULL) {

	line_is_new = 1;

	while (1) {
		a_section_name = get_next_section_name_in_line(a_line,
							       line_is_new);
		line_is_new = 0;

		if (a_section_name == NULL) {
			break;
		}
		if (!strcmp(a_section_name, section_name) ) {
			return (SECTION_NAME_FOUND);
		}
	} /* end loop - all section names in one line */
 
 } /* end loop - to EOF */

 return (UCFG_SECTION_NAME_NOTFND); 
 
} /* end of find_section_name_in_file() */




static void *make_values_list_from_values_string(char *val_string,
						char *val_string_list)
{
 char 	*p_value,
	*p,
	*p_string_copy;
 void	*p_new_list;
 int	 end_found;

 if ((p_new_list = lkls_new_linked_list() ) == NULL) {
	return (NULL);
 }

 /* At first, val_string[] points to a non-blank char, the first value.
  * Other values are separated by TAB or SPACE, as usual.
  * Watch out: end of values list can happen with a COMMENT_CHAR
  * or with a NUL byte. Do NOT jump over the terminating NUL byte to
  * go looking for the next value! (boolean var end_found is for this)
  */
 for (end_found = 0, p_value = val_string; *p_value;  ) {
 	if (*p_value == COMMENT_CHAR) {
		break;
	}

	/* 
	 * Is the value a quoted string or not?
	 */
	p = NULL;	/* this value is tested later! */

	if (*p_value == A_QUOTE) {
		p = strchr(&p_value[1], A_QUOTE);
		if (p != NULL) {
			*p = '\0';
			p_string_copy = strdup(&p_value[1]);
		}
	}
	/*
	 * If (p == NULL) Then we fall back on non-quoted string
	 * behaviour:
	 */
	if (p == NULL) {
		p = skip_over_non_space(p_value);
		if (!*p) {			/* '\0' byte found ? */
			end_found = 1;
		}
		*p = '\0';

		p_string_copy = strdup(p_value);
	}
	if (p_string_copy == NULL) {
		return (NULL);
	}
	if ((s_ucfg_errno = lkls_add_elem(p_new_list, p_string_copy)) != 0) {
		return (NULL);
	}

	/* Append the value to the string containing the list of the
	 * values of the parameter.
	 * Note: No need to check the length of val_string_list, it
	 * is the same length as val_string, and will contain the same
	 * data MINUS extra '\t' and ' ' characters.
	 */
	if (strlen(val_string_list) > 0) {
		/* Separate each value with ' ' */
		strcat(val_string_list, " ");
	}
	strcat(val_string_list, p_string_copy);

	if (end_found) {
		break;
	}

	p++;
	p_value = skip_spaces(p);
 } /* end loop all values */

 return (p_new_list);

} /* end of make_values_list_from_values_string() */




static int read_and_insert_one_param_def(struct st_param_node *p_root, FILE *fi)
/*
 * Read one line, add the parameter name and value in the tree.
 *
 * Return END_OF_SECTION when EOF is reached OR when the end of the current
 * section is reached (next section begins with a section name).
 */
{
 char	 param_name[UCFG_MAX_PARAM_NAME_LEN + 2],
 	 values_string[UCFG_MAX_VALUE_LEN + 2],
 	 values_string_list[UCFG_MAX_VALUE_LEN + 2] = "";
 void	*values_list;
 char	*a_line,
 	*p,
 	*p_name;

 if ((a_line = get_one_line_skip_comments(fi)) == NULL) {
	return (END_OF_SECTION);
 }
 if (!is_tab_or_space(*a_line) ) {	/* found section name */
	return (END_OF_SECTION);
 }

 /* extract param NAME from line:
  */
 p_name = skip_spaces(a_line);			/* to beginning of name */
 
 strncpy(param_name, p_name, UCFG_MAX_PARAM_NAME_LEN);  /* make a copy */

 param_name[UCFG_MAX_PARAM_NAME_LEN] = '\0';	/* make sure string is ended */

 p = skip_over_non_space(param_name);
 *p = '\0';					/* truncate after name */

 /* extract param VALUE from line:
  */
 p = skip_over_non_space(p_name);		/* to end of name (in line) */
 p = skip_spaces(p);				/* to beginning of value */

 strncpy(values_string, p, UCFG_MAX_VALUE_LEN);	/* make a copy */

 values_string[UCFG_MAX_VALUE_LEN] = '\0';	/* make sure string is ended */

 if ((values_list = make_values_list_from_values_string(values_string,
			values_string_list)) == NULL) {
	return (note_error(UCFG_OUT_OF_MEMORY, p_name) );
 }
 return (insert_param_def(p_root, param_name, values_list, values_string_list));

} /* end of read_and_insert_one_param_def() */


 



static print_arbre(struct st_param_node *p_arbre, FILE *fi_out)
{
 void	*p_datum;
 int	nb = 0;

 if (p_arbre == (struct st_param_node *) NULL)
	return ( 0 );
	
 nb = print_arbre(p_arbre->p_left, fi_out);


 fprintf(fi_out, "\n===>%-20s : %x",
	p_arbre->id_param, p_arbre->val_list);


 for (p_datum = lkls_get_datum_in_list_first(p_arbre->val_list);
      p_datum != NULL;
      p_datum = lkls_get_datum_in_list_next(p_arbre->val_list) ) {
	fprintf(fi_out, " '%s'", p_datum);
 }
 nb += print_arbre(p_arbre->p_right, fi_out);

 return (nb + 1);
} /* end of print_arbre() */






static int all_values_are_ok(void *p_val_list, int val_type)
/*
 * Called by ucfg_check_all_param_defs()
 */
{
 char	*p_val_string;

 p_val_string = lkls_get_datum_in_list_first(p_val_list);
 if (p_val_string == NULL) {
	return (-2);		/* empty values list !? */
 }
 while (p_val_string != NULL) {
	if (val_type == UCFG_NUMERIC_PARAM) {
		if (!string_is_number(p_val_string) ) {
			return (-1);
		}
	}
	/* No specific validation for UCFG_STRING_PARAM.
	 * Empty string is allowed to support some
	 * un-standard client apps that pass "" as their
	 * name! (20oct1995,jft)
	 */
	p_val_string = lkls_get_datum_in_list_next(p_val_list);
 }
 return (0);

} /* end of all_values_are_ok() */






static char *get_param_string_priv(char *param_name, int first_one)
/*
 * Called by ucfg_get_param_string_first() 
 *    and by ucfg_get_param_string_next()
 *
 * Returns the first or next string value associated to the parameter or NULL
 * if not found
 *
 */
{
 char	*p_val_list;
 void	*p_val_string;		/* casted to (char *) later */

 if (!s_param_loaded)
	return (NULL);

 p_val_list = find_param_val(&s_param_tree_root, param_name);
 
 if (p_val_list == NULL)
 	return (NULL);

 if (first_one) {
	 p_val_string = lkls_get_datum_in_list_first(p_val_list);
 } else {
	 p_val_string = lkls_get_datum_in_list_next(p_val_list);
 }

 if (p_val_string == NULL)
	return (NULL);

 return ((char *) p_val_string);

} /* end of get_param_string_priv() */





static int sf_is_string_in_param_values_list_ext(char *param_name, 
						 char *a_string,
						 int   use_pattern_matching)
/*
 * Called by:	ucfg_is_string_in_param_values_list_ext()
 *		ucfg_is_string_in_param_values_list()
 *		ucfg_is_string_in_param_values_list_pattern()
 *
 * Return:
 *	-1	if param_name not found
 *	 1	if a_string is in the list of values for param param_name
 *	 0	if param_name exist but a_string is not amongst its values
 */
{
 char	*p_val_list;

 if (!s_param_loaded) {
	return (-1);
 }
 p_val_list = find_param_val(&s_param_tree_root, param_name);
 
 if (p_val_list == NULL) {
 	return (-1);
 }

 return (lkls_is_string_in_list_ext(p_val_list, a_string,
				    use_pattern_matching) );

} /* end of sf_is_string_in_param_values_list_ext() */




/* ------------------------------------------------ Public functions: */

char *ucfg_version_id()
{
	return (VERSION_ID);
}



int ucfg_open_cfg_at_section_ext(char *cfg_file_name,
				char *section_name,
				int b_allow_duplicate_params,
				int b_destroy_old_params)
/* 
 * Open file cfg_file_name, locate section section_name, and read the 
 * parameters values.
 *
 * If b_allow_duplicate_params == TRUE
 *     If a parameter is found with the same name as an existing parameter
 *     the new value replace the old one, and the new value is reported on
 *     stderr (unless the file pointer was changed by a call to
 *     ucfg_assign_substitution_report_file_handle()).
 * If b_allow_duplicate_params != TRUE
 *     If a parameter is found with the same name as an existing parameter
 *     the new value is ignored and the parameter name is kept for
 *     reporting by ucfg_err_bad_param_name().
 *
 * If b_destroy_old_params == TRUE
 *     The list of parameter is destroyed before reading the new config
 *     file.
 * If b_destroy_old_params != TRUE
 *     The parameters of the config file are appended to the list of parameters
 *     already read (if a config file was already read, otherwise the parameter
 *     has no effect).
 *
 * Return 0 or error code
 */
{
 FILE	*fi_param;
 int	 rc;

 if (!s_keep_old_param_values_if_open_fails) {
	s_param_loaded		= 0;	/* re-init  status indicators */
 }

 remember_bad_param_name("reset", 1);	/* reset bad param name */

 if ((fi_param = fopen(cfg_file_name, "r")) == NULL) {
	return (note_error(UCFG_OPEN_FAILED, cfg_file_name) );
 }
 if ((rc = find_section_name_in_file(section_name, fi_param))
     != SECTION_NAME_FOUND) {
	fclose(fi_param);
	return (note_error(rc, section_name) );
 }

 if (b_allow_duplicate_params == FALSE) {
	s_reject_second_param_def = TRUE;
 } else {
	s_reject_second_param_def = FALSE;
 }

 s_param_loaded		= 1;		/* re-init status indicators */
 s_ucfg_error_code	= 0;
 s_ucfg_errno		= 0;

 if (b_destroy_old_params == TRUE) {
	s_nb_params		= 0;

	free_param_tree(&s_param_tree_root); /* free previous data structure */
 }

 while (1) {
	rc = read_and_insert_one_param_def(&s_param_tree_root, fi_param);

	if (rc == END_OF_SECTION) {		/* EOF or End of section */
		rc = 0;
		break;
	}
	if (rc != 0   &&  rc != UCFG_PARAM_DUPLICATED) {
		fclose(fi_param);
		return (rc);	/* error already noted */
	}
 }
 fclose(fi_param);

 return (rc);

} /* end of ucfg_open_cfg_at_section_ext() */




int ucfg_open_cfg_at_section(char *cfg_file_name,
				char *section_name)
{
	return(ucfg_open_cfg_at_section_ext(
				cfg_file_name,
				section_name,
				FALSE,	/* No duplicate params allowed */
				TRUE)); /* Destroy old values */
} /* end of ucfg_open_cfg_at_section() */



int ucfg_check_all_param_defs(int  nb_params_required,
			      struct ucfg_check_param_def_entry val_tab[])
/*
 * DESCRIPTION:
 *	Checks that all names in val_tab[] can be found in the config that
 *	was previously opened and loaded with ucfg_open_cfg_at_section().
 *	Checks that the param_val is a numeric string if param_type ==
 *	UCFG_NUMERIC_PARAM.
 *	Also tell if the number of loaded params is > than sz_tab
 *
 * Return Value:
 *	0 or
 *	UCFG_PARAM_MISSING
 *	UCFG_PARAM_HAS_BAD_VAL		should be number, but is not
 *	UCFG_SUPPL_PARAM_FOUND
 */ 
{
 int	 i,
 	 rc = 0;
 void	*p_val_list;
 char	*p_name;


 for (i = 0; i < nb_params_required; i++) {
 	p_name = val_tab[i].param_name;
 	
	p_val_list = find_param_val(&s_param_tree_root, p_name);
	if (p_val_list == NULL) {
		/* Clear bad param name that might have been set by
		 * a UCFG_PARAM_HAS_BAD_VAL error:
		 */
		remember_bad_param_name("reset", 1);

		/* Memorize the missing param name:
		 */
		remember_bad_param_name(p_name, 0);

		return (UCFG_PARAM_MISSING);	/* return critical error */
	}
	if (all_values_are_ok(p_val_list, val_tab[i].param_type) < 0) {
		remember_bad_param_name(p_name, 0);
		rc = UCFG_PARAM_HAS_BAD_VAL;
	}
 } /* end loop - all params in table */

 if (rc != 0)
	return (rc);

 return ( (s_nb_params > nb_params_required)
 	 ? UCFG_SUPPL_PARAM_FOUND : 0);

} /* end of ucfg_check_all_param_defs() */





char *ucfg_err_bad_param_name()
/*
 * if ucfg_open_cfg_at_section() has returned UCFG_PARAM_VALUE_TRUNCATED or
 * UCFG_PARAM_DUPLICATED, you can use this function to get the name of the
 * problem parameter.
 *
 * Return NULL (if there was no error specific to one parameter)
 * or the name of the problem parameter.
 */
{
	return (remember_bad_param_name(NULL, 0) );

} /* end of ucfg_err_bad_param_name() */




int ucfg_copy_param_values_list_to_array(char *param_name,
					 char *ptr_array[], int max_sz)
/*
 * Copy each datum pointer of the list to one element of ptr_array[].
 *
 * Return number of values or LKLS_ERR_ARRAY_OVERFLOW if ctr >= max_sz
 * or LKLS_ERR_NULL_LIST_PTR if linked_list == NULL
 * or LKLS_ERR_BAD_SIGNATURE if linked_list is not a good handle
 * (detection is NOT guaranteed).
 */
{
 char	*p_val_list;
 int	 nb_values = -1;

 if (!s_param_loaded)
	return (UCFG_CONFIG_FILE_NOT_LOADED);

 p_val_list = find_param_val(&s_param_tree_root, param_name);
 
 if (p_val_list == NULL)
 	return (UCFG_PARAM_NOT_FOUND);

 nb_values = lkls_copy_data_to_array(p_val_list, ptr_array, max_sz);

 return (nb_values);

} /* end of ucfg_copy_param_values_list_to_array() */




int ucfg_is_string_in_param_values_list_ext(char *param_name, char *a_string,
					    int   b_with_pattern)
/*
 * Return:
 *	-1	if param_name not found
 *	 1	if a_string is in the list of values for param param_name
 *	 0	if param_name exist but a_string is not amongst its values
 */
{
  return (sf_is_string_in_param_values_list_ext(param_name, a_string, 
  						b_with_pattern) );
}



int ucfg_is_string_in_param_values_list_pattern(char *param_name,
						char *a_string)
/*
 * Return 1 if a_string is in the list of values for param param_name
 * else return 0
 */
{
	int	a_bool = 0;

	/* 1 == with '*' pattern matching
	 */
	a_bool = sf_is_string_in_param_values_list_ext(param_name, a_string, 1);

	/* a_bool might == -1 if param_name was not found
	 */
	return (a_bool >= 1);
}


int ucfg_is_string_in_param_values_list(char *param_name, char *a_string)
/*
 * Return 1 if a_string is in the list of values for param param_name
 * else return 0
 */
{
	int	a_bool = 0;

	/* 0 == as is, without '*' pattern matching
	 */
	a_bool = sf_is_string_in_param_values_list_ext(param_name, a_string, 0);

	/* a_bool might == -1 if param_name was not found
	 */
	return (a_bool >= 1);
}




char *ucfg_get_param_string_first(char *param_name)
/*
 * Returns the FIRST string value associated to the parameter or NULL if not
 * found.
 */
{
 return (get_param_string_priv(param_name, 1) );

} /* end of ucfg_get_param_string_first() */




char *ucfg_get_param_string_next(char *param_name)
/*
 * Returns the NEXT string value associated to the parameter or NULL if not
 * found or if list of values is exhausted.
 */
{
 return (get_param_string_priv(param_name, 0) );

} /* end of ucfg_get_param_string_next() */




int ucfg_display_all_params( FILE *fi_out)
{
 return (print_arbre(&s_param_tree_root, fi_out) );
}




int ucfg_nb_params( void )
{
	return ( s_nb_params );
}



char *ucfg_remember_bad_param_name( char *id_param, int reset_ind )
{
	return ( remember_bad_param_name( id_param, reset_ind ) );
}




int ucfg_string_is_number( char *value )
{
	return ( string_is_number( value ) );
}



int ucfg_assign_substitution_report_file_handle(FILE *fi)
/*
 * Used to change the file pointer used to report the new values of
 * duplicate parameters (when duplicates are allowed).
 * Default value of file pointer: stderr
 *
 * RETURNS: 0  
 *
 * Note:
 * It is the responsibility of the calling module to open the
 * file, to close it and to assure that the pointer passed is
 * a valid file pointer.  A NULL argument disables the reporting
 * of the duplicate parameters.
 */
{
	s_fi_substitution_report = fi;	

	return (0);
}

/* -------------------------------------------- end of ucfg.c ----------- */
