#!/usr/bin/python

#
# gxmonitor.py : Gateway-XML thread MONITORs
# ------------------------------------------
#
#
# $Source: /ext_hd/cvs/gx/gxmonitor.py,v $
# $Header: /ext_hd/cvs/gx/gxmonitor.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $
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
# A "monitor" is the object whose methods are executed within a thread.
#
# The code in all methods of a monitor MUST be re-entrant as they
# are always used by multiple threads.
#
# That's why each gxthread keeps its data in its frame and context
# attributes.
#
# -----------------------------------------------------------------------
#
# 2001nov12,jft: first version
# 2001nov17,jft: added 	getDebugLevel(self), assignDebugLevel(self, val)
# 2001nov30,jft: added  needDestConn(self),  assignMaster(self, val)
# 2002jan29,jft: + added standard trace functions
#

import time
import sys
import gxthread

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxmonitor.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $"



# module-scope debug_level:
#
debug_level = 0


# Module functions:
#
def assignDebugLevel(val):
	global debug_level
	debug_level = val

def getDebugLevel():
	return (debug_level)


def getRcsVersionId():
	return (RCS_VERSION_ID)

#
# (End standard trace functions)
#

#
# Classes:
#
class gxmonitor:
	#
	# All Monitor(s) in a GxServer should be derived from this gxmonitor class
	#
	def __init__(self, name=None, masterMonitor=None):
		if (name == None):
			self.name = "gxmonitor"
		else:
			self.name = name
		self.master = masterMonitor
		self.ctrErrors = 0
		self.msg    = ""

	def __repr__(self):
		return ("<%s>" % (self.name, ) )
		# return ("<%s, master:%s>" % (self.name, self.master) )

	def getName(self):
		return (self.name)

	def assignMaster(self, val):
		#
		# in some case, the program only knows the master after
		# the instance is created.
		#
		self.master = val

	def needDestConn(self):
		#
		# default method. Often overidden by subclass.
		#
		return (0)

	def rememberErrorMsg(self, m1, m2):
		self.msg = "%s, %s" % (m1, m2)

	def runThread(self, aThread):
		n = 10
		print aThread, "generic gxmonitor running... will wait", n, "seconds and stop."
		aThread.changeState(gxthread.STATE_WAIT)
		time.sleep(n)
		aThread.changeState(gxthread.STATE_COMPLETED)
		print aThread, "DONE"
		sys.exit()





if __name__ == '__main__':
	m = gxmonitor()
	t1 = gxthread.gxthread("t1", monitor=m)
	t1.start()

	print t1
	time.sleep(10)
	print t1
