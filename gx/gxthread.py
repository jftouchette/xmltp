#!/usr/bin/python

#
# gxthread.py : Gateway-XML Thread
# --------------------------------
#
# $Source: /ext_hd/cvs/gx/gxthread.py,v $
# $Header: /ext_hd/cvs/gx/gxthread.py,v 1.4 2005/11/11 19:21:19 toucheje Exp $
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
#
# Higher level class based upon threading.Thread, contains extra
# attributes and methods.
#
# -----------------------------------------------------------------------
#
# 2001nov09,jft: first version
# 2001dec03,jft: added getListOfSelectedThreads(bit_mask),
#			getListOfServiceThreads()
#			getListOfRPCThreads()
#		 . gxthread.__init__(): self.setDaemon(isDaemon), self.setName(id)
# 2002jan29,jft: + added standard trace functions
# 2002feb10,jft: + asTuple()
#		 + getListOfAllGXThreads()
# 2002sep11,jft: . use localtime() instead of gmtime()
# 2005avr29,jft: + gxthread.getThreadIdent()
# 2005mai02,jft: + getIdentOfThreadBygxthread_id(id)
#		 + getTracebackOfThreadByIdent(thr_ident)
# 2005nov11,jft: . gxthread.asTuple(): check isinstance(self.frame, gxrpcframe.gxRPCframe) before doing frameLs = self.frame.asTuple()
#

import time
import threading
import thread		# for thread.get_ident()
import sys
import traceback	# used for asynchronous debugging of other threads
try:
	import threadframe	# used for asynchronous debugging of other threads
	getThreadFrameDict = threadframe.dict
	threadframe_available = 1
except:
	threadframe_available = 0
	def getThreadFrameDict():
		return {}

import gxrpcframe	# for isinstance(self.frame, gxrpcframe.gxRPCframe)


#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxthread.py,v 1.4 2005/11/11 19:21:19 toucheje Exp $"

#
# CONSTANTS:
#
STATE_INIT = "INIT"
STATE_WAIT = "WAIT"
STATE_WAIT_RESOURCE = "WAIT_RESOURCE"
STATE_GOT_RESOURCE = "GOT_RESOURCE"
STATE_SENDING_BACK = "SENDING_BACK"
STATE_ACTIVE = "ACTIVE"
STATE_PREPARE = "PREPARE"
STATE_POLL = "POLL"
STATE_PARSE = "PARSE"
STATE_ERROR  = "ERROR"
STATE_COMPLETED = "COMPLETED"
STATE_SHUTDOWN  = "SHUTDOWN"
STATE_ABORTED   = "ABORTED"

THREADTYPE_RPC     = 1
THREADTYPE_REGPROC = 2
THREADTYPE_SERVICE = 16
THREADTYPE_QUEUE   = 64
THREADTYPE_POLL    = 128
THREADTYPE_NOT_GX  = 0		# the MainThread is NOT a gxthread!

#
# Module variables:
#
frameTupleColNames = None	# must be assigned at run-time by gxserver

threadTypesDict = {
	THREADTYPE_RPC     : "RPC",
	THREADTYPE_REGPROC : "REGPROC",
	THREADTYPE_SERVICE : "SERVICE",
	THREADTYPE_QUEUE   : "QUEUE",
	THREADTYPE_POLL    : "POLL",
	THREADTYPE_NOT_GX  : "NOT_GX",
	}

#
# module-scope debug_level:
#
debug_level = 0

#
# Module functions for debug and trace:
#
def assignDebugLevel(val):
	global debug_level
	debug_level = val

def getDebugLevel():
	return (debug_level)

def getRcsVersionId():
	if threadframe_available:
		s = RCS_VERSION_ID + " With threadframe."
	else:
		s = RCS_VERSION_ID + "threadframe NOT available."
	return (s)

#
# (End standard trace functions)
#

#
# Module Functions:
#
def asTupleColNames():
	ls = ["thread id                ", " state       ", "timestamp    ", "monitor-pool   ", "resource       "]
	if (frameTupleColNames):
		ls.extend(frameTupleColNames)
	return tuple(ls)


def getThreadsList():
	return (threading.enumerate() )


def getListOfSelectedThreads(bit_mask):
	ls = []
	for t in threading.enumerate():
		try:
			tType = t.getThreadType() 
		except:			# some threads are not gxthread
			tType = THREADTYPE_NOT_GX
		if (tType & bit_mask):
			ls.append(t)
	return (ls)

def getListOfServiceThreads():
	return (getListOfSelectedThreads(THREADTYPE_SERVICE | THREADTYPE_QUEUE | THREADTYPE_POLL) )

def getListOfRPCThreads():
	return (getListOfSelectedThreads(THREADTYPE_RPC | THREADTYPE_REGPROC) ) 

def getListOfAllGXThreads():
	return (getListOfSelectedThreads(THREADTYPE_RPC | THREADTYPE_REGPROC | THREADTYPE_SERVICE | THREADTYPE_QUEUE | THREADTYPE_POLL) ) 


def printThreadsList():
	for t in threading.enumerate():
		print t
		# print "%s\t%s\t%s" % (t.id, t.describeState(), t.getTimeLastActivity() )

def getDescOfTypeOfThread(thread):
	try:
		typ = thread.getThreadType()
	except:
		typ = THREADTYPE_NOT_GX
	try:
		return (threadTypesDict[typ])
	except:
		"Thread??"

#
# Thread frame debugging functions:
#
def getIdentOfThreadBygxthread_id(id):
	ls = getListOfAllGXThreads()
	## ls = threading.enumerate()	## serait bogus a cause de mainThread qui n'est pas un gxthread
	for t in ls:
		try:
			if id.lower() == t.id.lower():
				return t.getThreadIdent()
		except AttributeError:
			pass	## '_MainThread' object has no attribute 'getThreadIdent', and neither .id
		except:
			pass

def getTracebackOfThreadByIdent(thr_ident):
	"""Return a list of lines. These show the traceback of that thread.
	The value of arg thr_ident must come from the .getThreadIdent() method of a gxthread object.
	The function getIdentOfThreadBygxthread_id(id) is useful for that purpose.
	NOTE: if the threadframe module is not available, getTracebackOfThreadByIdent() will simply return an error message. 
	Without the threadframe module, this function cannot find the frame of any thread.
	"""
	framesDict = getThreadFrameDict()
	frame = framesDict.get(thr_ident, None)
	if not frame:
		s = "Cannot find thread ident %s in framesDict. threadframe_available=%s" % (thr_ident, threadframe_available, )
		return [s, ]
	stack = traceback.extract_stack(frame)
	###print "stack=", stack
	ls = traceback.format_list(stack)
	return ls


#
# Class definition...
#
class gxthread(threading.Thread):
	#
	# All threads in a GxServer should be derived from this gxthread class
	#
	def __init__(self, id, parent=None, monitor=None, resource=None,
				frame=None, context=None, pool=None,
				threadType=THREADTYPE_RPC,
				isDaemon=1):
		threading.Thread.__init__(self)	# essential
		self.thread_ident = -1	# will be set later, in self.run()
		self.id = id
		self.setDaemon(isDaemon)
		self.setName(id)
		self.threadType = threadType
		self.parent = parent	# parent thread
		self.monitor = monitor	# monitor object which will execute in this thread
		self.resource = resource # a Resource from a pool
		self.frame = frame	# "frame" of data used by the monitor to execute
		self.context = context	# an XML-TP context, if required
		self.pool = pool	# the pool to which this thread belongs (if applicable)
		self.changeState(STATE_INIT)
		self.msg   = "init"
		self.timeCreated = time.time()


	def __repr__(self):
		if (self.resource == None):
			return ("<%s:%s %s %s ms>" % (self.id, self.state, self.getThreadIdent(),
						   self.getTimestamp() ) )

		return ("<%s:%s %s %s ms, res=%s>" % (self.id, self.state,  self.getThreadIdent(),
					self.getTimestamp(),
					self.resource ) )

	def asTuple(self, b_onlyColNames=0):
		if (b_onlyColNames):
			return (asTupleColNames() )

		#
		# ("thread id      ", " state       ", "timestamp    ", "monitor-pool   ", "resource       ")
		#
		if (self.resource == None):
			res = "None"
		else:
			res = self.resource.getName()

		if (self.monitor == None):
			if (self.pool == None):
				mon = "None"
			else:
				mon = self.pool.getName()
		else:
			mon = self.monitor.getName()

		id      = "%25.25s" % (self.id, )
		state   = "%13.13s" % (self.describeState(),  )
		tStamp  = self.getTimestamp()
		res     = "%15.15s" % (res, )
		mon     = "%15.15s" % (mon, )

		ls = [id, state, tStamp, mon, res]

		if (self.frame == None or not isinstance(self.frame, gxrpcframe.gxRPCframe) ):
			frameLs = [ "", ]
			frameLs = frameLs * 5
		else:
			frameLs = self.frame.asTuple()

		ls.extend( frameLs )

		return tuple(ls)


	def getTimestamp(self):
		i = int(self.timeStamp)
		lt = time.localtime(i)
		ms = self.timeStamp - i
		s1 = time.strftime("%H:%M:%S", lt)
		return ("%s.%03d" % (s1, ms * 1000) )

	def getId(self):
		return (self.id)

	def getThreadIdent(self):
		""" Return the value of thread.get_ident() that self.run() got when it started. 
		If self.run() has not been called yet, then, return -1.
		"""
		return self.thread_ident

	def getThreadType(self):
		return (self.threadType)

	def describeState(self):
		return (self.state)

	def getTimeLastActivity(self):
		return (self.timeStamp)

	def changeState(self, newState):
		self.timeStamp = time.time()
		self.state = newState
		
	def rememberErrorMsg(self, m1, m2):
		self.msg = "%s, %s" % (m1, m2)

	def getErrMsg(self):
		return (self.msg)

	def getParent(self):
		return (self.parent)

	def getMonitor(self):
		return (self.monitor)

	def getResource(self):
		return (self.resource)

	def getFrame(self):
		return (self.frame)

	def getContext(self):
		return (self.context)

	def getPool(self):
		return (self.pool)

	def assignResource(self, val):
		if (val != None):
			val.assignUsedByThread(self)
			self.changeState(STATE_GOT_RESOURCE)
		self.resource = val

	def assignFrame(self, val):
		self.frame = val

	def assignContext(self, val):
		self.context = val

	def assignPool(self, val):
		self.pool = val

	def run(self):
		self.thread_ident = thread.get_ident()

		if (self.monitor != None):
			self.monitor.runThread(self)
		else:
			n = 10
			print self, "starting... will wait", n, "seconds."
			self.changeState(STATE_WAIT)
			time.sleep(n)
			self.changeState(STATE_COMPLETED)
			print self, "DONE"
			sys.exit()

def printAllThreadsTracebacks():
	print getThreadFrameDict()
	for t in threading.enumerate():
		print t
		try:
			### ident = t.getThreadIdent()
			ident = getIdentOfThreadBygxthread_id(t.id)	### to test that function
			print "t.id:", t.id, "ident:", ident
			ls = getTracebackOfThreadByIdent(ident)
			if type(ls) == type("s"):
				print ls
			else:
				## print ls
				for s in ls:
					lines = s.splitlines()
					for x in lines:
						print x
						## print repr(x)
		except AttributeError:	## '_MainThread' object has no attribute 'getThreadIdent'
			pass


if __name__ == '__main__':

	t1 = gxthread("gxt-1")
	t2 = gxthread("gxt-2")
	t3 = gxthread("gxt-3")
	t1.start()
	time.sleep(1)
	t2.start()
	time.sleep(1)
	t3.start()

	printThreadsList()
	print getThreadsList()
	printAllThreadsTracebacks()

	time.sleep(10)

	print getThreadsList()
	printAllThreadsTracebacks()

	time.sleep(10)

	print getThreadsList()
	printAllThreadsTracebacks()
