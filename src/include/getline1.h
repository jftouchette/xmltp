/* getline1.h
 * ----------
 *
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
 * $Source: /ext_hd/cvs/include/getline1.h,v $
 * $Header: /ext_hd/cvs/include/getline1.h,v 1.3 2004/10/15 20:28:49 jf Exp $
 *
 *
 * Function prefix:	g1ln_
 *
 * Implemented in:	misc_fn/getline1.c
 *
 *
 * char *g1ln_get_first_line_of_file(char *filename) 
 * 
 * 	is a function to read the first line of a file. 
 *
 *
 * NOTE: The name of the file should be a fully qualified path name.
 *
 * ----------------------------------------------------------------------
 *
 * Versions:
 * 1994sept12,jft: first version
 *
 */

char *g1ln_get_first_line_of_file(char *filename);
/*
 * Return the first line (pointer to static char xxx[]) of the file named
 * filename.
 *
 * Return NULL if file cannot be opened in Read mode or if file is empty.
 */
