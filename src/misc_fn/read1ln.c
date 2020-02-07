/* read1ln.c
 *
 *
 *  Copyright (c) 1994-1997 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/misc_fn/read1ln.c,v $
 * $Header: /ext_hd/cvs/misc_fn/read1ln.c,v 1.3 2004/10/15 21:14:27 jf Exp $
 *
 * read_one_line() is used in cfg_read/ucfg.c
 *
 * ------------------------------------------------------------------------
 * 14juin1994,jft
 * 1997feb18,deb:	Added conditional include for Windows NT port
 */

#ifdef WIN32
#include "ntunixfn.h"
#endif

#include <stdio.h>
#include "read1ln.h"


int read_one_line(char li[], int max_len, FILE *fi)
{
	int	line_len;
	
	if (fgets(li, max_len, fi) == NULL) {
		return (-1);
	}
	line_len = strlen(li);

	if (line_len > 0  &&  li[line_len - 1] == '\n') {
		line_len--;
		li[line_len] = '\0';
	}
	return (line_len);

} /* end of read_one_line() */

