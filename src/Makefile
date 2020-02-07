
#
# {xmltp}/src/Makefile
#
# This Makefile should build the ../lib/xmltp_gx.so shared library
# and the ../bin/xml2syb executable.
#
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

CFLAGS  =    -g 
XMLTP_CFLAGS = -g -I ../include


all:
	(cd cfg_read; make; cd ..)
	(cd dulib; make; cd ..)
	(cd dynarray; make; cd ..)
	(cd log_fn; make; cd ..)
	(cd misc_fn; make; cd ..)
	(cd sock_fn; make; cd ..)
	### You need a good SYBASE environment variable to be able to make ct10api and xmltp.
	### SYBASE = $(SYBASE)
	###
	(cd ct10api; make; cd ..)
	(cd xmltp; make; cd ..)

clean:
	rm -f ../gx/*.pyc
	rm -f obj/*.o
	rm -f ../bin/xml*
	rm -f ../lib/*.so
	rm -f xmltp/xmltp_gram.tab.?

rmo:
	rm -f obj/*.o	
	rm -f xmltp/xmltp_gram.tab.?

#
# End of {xmltp}/src/Makefile
#

