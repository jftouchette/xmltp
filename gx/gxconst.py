#!/usr/bin/python

#
# gxconst.py :  Gateway-XML - Common CONSTANTS
# --------------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxconst.py,v $
# $Header: /ext_hd/cvs/gx/gxconst.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $
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
# -----------------------------------------------------------------------
# 2002jan30,jft: + first version
# 2002feb11,jft: + PERM_LANG
#

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxconst.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $"

#
# CONSTANTS:
#

#
# "MODE" constants, used by: 	gxregprocs.py, ...
#
MODE_ANY	= 0
MODE_PRIMARY	= 1
MODE_SECONDARY	= 2
MODE_INACTIVE	= 16

#
# "PERM_" constants, used by: 	gxclientconn.py,
#				gxregprocs.py, ...
#
PERM_ANY	= 0
PERM_NONE	= 0
PERM_GETFILE	= 1
PERM_STATS	= 2
PERM_OPER	= 4
PERM_SHUTDOWN	= 8
PERM_LANG	= 16

#
# *** This module should only contains constants ***
#

if __name__ == '__main__':

	print "No Unit Test in", RCS_VERSION_ID
