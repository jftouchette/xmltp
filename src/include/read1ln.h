/* read1ln.h
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
 * $Source: /ext_hd/cvs/include/read1ln.h,v $
 * $Header: /ext_hd/cvs/include/read1ln.h,v 1.3 2004/10/15 20:53:12 jf Exp $
 *
 * read_one_line() is used in cfg_read/ucfg.c
 *
 * ------------------------------------------------------------------------
 * 14juin1994,jft
 */


int read_one_line(char li[], int max_len, FILE *fi);

