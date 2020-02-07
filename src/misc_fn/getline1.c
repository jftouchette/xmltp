/* getline1.c
 * ----------
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
 * $Source: /ext_hd/cvs/misc_fn/getline1.c,v $
 * $Header: /ext_hd/cvs/misc_fn/getline1.c,v 1.3 2004/10/15 21:11:26 jf Exp $
 *
 *
 * Function prefix:	g1ln_
 *
 * Implemented in:	misc_fn/getline1.c
 *
 * Function to read the First line of a file.
 *
 * char *g1ln_get_first_line_of_file(char *filename) 
 * 
 * 	is a function to read the first line of a file. 
 *
 * NOTE: The name of the file should be a fully qualified path name.
 *
 * ----------------------------------------------------------------------
 *
 * Versions:
 * 1994sept12,jft: first version
 * 1997feb18,deb:  Added conditional include for Windows NT port
 * 2004oct15,jft:  merge with Dev's version
 */
#ifdef WIN32
#include "ntunixfn.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#define _INCLUDE_POSIX_SOURCE	"yes"	/* pour fcntl.h */
#include <string.h>

#include "read1ln.h"

#define MAX_LINE	128



char *g1ln_get_first_line_of_file(char *filename)
{
static	char	first_line[MAX_LINE + 2];
	int	 rc = 0;
	FILE	*fi;
	
	if (filename == NULL) {
		fprintf(stderr, "\n filename == NULL\n");
		return (NULL);
	}
	if ((fi = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "\ncannot open '%.30s'\n", filename);
		return (NULL);
	}
	if ((rc = read_one_line(first_line, MAX_LINE, fi)) <= 0) {
		/* fprintf(stderr, "\nread_one_line() failed (%d)\n", rc);
		 */
		fclose(fi);
		return (NULL);
	}
	fclose(fi);

	return (first_line);
	
} /* end of g1ln_get_first_line_of_file() */

