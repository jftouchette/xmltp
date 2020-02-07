#
# dulib/makefile
#
#  Copyright (c) 2001-2003 Jean-Francois Touchette
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#  (The file "COPYING" or "LICENSE" in a directory above this source file
#  should contain a copy of the GNU Library General Public License text).
#  -------------------------------------------------------------------------
#
 
OBJDIR=../obj/
OBJ=

CC=gcc

XMLTP_CFLAGS=-g -I ../include

# this rule moves the .o file to the $(OBJDIR) directory
.c.o:
	$(CC) -c $(XMLTP_CFLAGS) $(XML2DB_FLAGS) -o $@ $<
	mv $@ $(OBJDIR)

OBJECTS=$(OBJ)check_num_string.o $(OBJ)parse_argument.o $(OBJ)term_util.o

all: $(OBJECTS)

