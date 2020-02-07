#!/usr/local/bin/python

#
# gxbdbqueue.py : GX BerkeleyDB Queue
# -----------------------------------
#
# Class gxbdbqueue uses bdbVarQueue.py to implement gxbdbqueue, a subclass of gxqueue.gxqueue
# -------------------------------------------------------------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxbdbqueue.py,v $
# $Header: /ext_hd/cvs/gx/gxbdbqueue.py,v 1.5 2006-06-13 01:16:31 toucheje Exp $
#
#  Copyright (c) 2006 Jean-Francois Touchette
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
# Class gxbdbqueue is a subclass of gxqueue.gxqueue
#
# A gxbdbqueue uses bdbVarQueue.py to implement a persistent queue with mostly
# the same API as gxqueue.gxqueue
#
# The queues are stored on disk with the use of the Berkeley DB library.
# See bdbVarQueue.py for implementation details.
#
# Limitations of this first version:
# Even if the queues are stored on disk and can be shared between 2 (or more)
# instances of the program, the statistics about each queue are held in the
# memory of each running instances. 
# See gxqueue.py for details.
#
# ----------------------------------------------------------------------------
# 2006jan30,jft: first version
# 2006feb08,jft: upd. comments
# 2006mar16,jft: shutdownBDBenvironment() write log msg before calling g_VarQueueEnv.close()
# 2006mar17,jft: GET_LOCK_SLEEP_DELAY = 0.25	# 250 ms -- was 400 ms
# 2006jun12,jft: . try: except: around import bdbVarQueue. If it fails, just assign bdbVarQueue = None
#		 . prepareBDBenvironment(): if bdbVarQueue == None then write diagnostic about failed import with gxServer.addErrMsg()
#

import Queue		# for Queue.Full Queue.Empty exceptions
import time		# for time.sleep()

import gxqueue
import gxutils

try:
	import bdbVarQueue
except ImportError:
	bdbVarQueue = None	# it might fail if bsddb3 or Berkeley DB are not installed.
	bdbVarQueue_excMsg = gxutils.formatExceptionInfoAsString()

import gxparam
import TxLog

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxbdbqueue.py,v 1.5 2006-06-13 01:16:31 toucheje Exp $"

# Constants:
#
MODULE_NAME = "gxbdbqueue"

#
# Private constants:
#
GET_LOCK_SLEEP_DELAY = 0.25	# 250 ms
PUT_LOCK_SLEEP_DELAY = 0.2	# 200 ms

#
# Module variables:
#
gxServer = None			# points back to main gxserver instance

g_listOfBdbQueues = []		# populated by gxbdbqueue.createQueue()
g_BDB_homedir	  = None
g_VarQueueEnv	  = None	# initialized by prepareBDBenvironment()


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
	return (RCS_VERSION_ID)

#
# (End of standard trace functions)
#

#
# module scoped functions:
#
def bdbvarqueue_logTraceFunction(moduleName, level, m1, m2):
	"""This function implements the g_logTraceFunction() API in bdbVarQueue.py
	"""
	msg = " ".join( [m1, m2] )
	gxServer.writeLog(moduleName, level, 0, msg)

def prepareBDBenvironment():
	global g_BDB_homedir
	global g_VarQueueEnv

	if bdbVarQueue == None:
		m1 = "prepareBDBenvironment(): bsddb3 and/or Berkeley DB are not installed. import bdbVarQueue had failed with exception. "
		try:
			m1 = m1 + bdbVarQueue_excMsg
		except:
			pass
		gxServer.addErrMsg(m1)
		return -9

	if g_VarQueueEnv:
		m1 = "Cannot create a VarQueueEnv BDBenvironment, because one is already active in: %s, %s, %s" %  (bdbDir, g_BDB_homedir, g_VarQueueEnv, )
		gxServer.addErrMsg(m1)
		return -1

	bdbVarQueue.g_logTraceFunction = bdbvarqueue_logTraceFunction	# assign a callback pointing to bdbvarqueue_logTraceFunction() above

	g_BDB_homedir = gxServer.getOptionalParam(gxparam.GXPARAM_PERSISTENT_QUEUES_DIR)
	if not g_BDB_homedir:
		m1 = "prepareBDBenvironment(): MISSING param: %s. NOTE: you also have to create that directory before starting this server" % (gxparam.GXPARAM_PERSISTENT_QUEUES_DIR, )
		gxServer.addErrMsg(m1)
		return -2

	g_VarQueueEnv = bdbVarQueue.VarQueueEnv(g_BDB_homedir)

	try:
		g_VarQueueEnv.open()
	except:
		ex = gxutils.formatExceptionInfoAsString()
		m1 = "prepareBDBenvironment(): g_VarQueueEnv.open() FAILED. g_BDB_homedir=%s ex=%s." % (g_BDB_homedir, ex, )
		gxServer.addErrMsg(m1)
		return -3

	if not g_VarQueueEnv.isOpen():
		m1 = "prepareBDBenvironment(): FAILED: g_VarQueueEnv %s is NOT open." % (g_BDB_homedir,  )
		gxServer.addErrMsg(m1)
		return -5

	return 0

def shutdownBDBenvironment(b_force=0):
	global g_listOfBdbQueues
	global g_VarQueueEnv
	lsFailed = []
	if g_listOfBdbQueues:
		for q in g_listOfBdbQueues:
			try:
				q.closeQueue(b_quiet=0)
			except:
				ex = gxutils.formatExceptionInfoAsString()
				m1 = "shutdownBDBenvironment(): q.close() FAILED q = %s. ex=%s." % (q.name, ex, )
				gxServer.addErrMsg(m1)
				lsFailed.append(q)

	g_listOfBdbQueues = None
	if lsFailed:
		g_listOfBdbQueues = lsFailed
		m1 = "shutdownBDBenvironment(): %s queues failed to close(). %s" % (len(lsFailed), lsFailed, )
		gxServer.addErrMsg(m1)
		if b_force:
			m1 = "shutdownBDBenvironment(): b_force=%s, trying g_VarQueueEnv.close() anyway..." % (b_force, )
			gxServer.addErrMsg(m1)
		else:
			m1 = "shutdownBDBenvironment(): b_force=%s, NOT doing  g_VarQueueEnv.close()." % (b_force, )
			gxServer.addErrMsg(m1)
			return -1

	if not g_VarQueueEnv:
		m1 = "No g_VarQueueEnv to close. It is %s." % (g_VarQueueEnv, )
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		return 0

	try:
		m1 = "shutdownBDBenvironment() is about to call g_VarQueueEnv.close() %s." % (g_VarQueueEnv, )
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		g_VarQueueEnv.close()
		g_VarQueueEnv = None		
	except:
		ex = gxutils.formatExceptionInfoAsString()
		m1 = "shutdownBDBenvironment(): g_VarQueueEnv.close() FAILED. g_BDB_homedir=%s ex=%s." % (g_BDB_homedir, ex, )
		gxServer.addErrMsg(m1)
		return -2

	return 0


class gxbdbqueue(gxqueue.gxqueue):
	""" gxbdbqueue is a BDB specific version of a gxqueue.
	    All queues in a GxServer should be derived from the gxqueue.gxqueue class. This one is.
	    @@@@@
	"""
	def __init__(self, name, owner=None, max=0, b_openGet=1, b_openPut=1, statsHistLen=10):
		global g_listOfBdbQueues

		gxqueue.gxqueue.__init__(self, name, owner, max, b_openGet, b_openPut, statsHistLen)

		#The super class __init__ does these:
		#self.name  = name
		#self.owner = owner 	# pool or gxserver which has created this gxqueue
		#self.max   = max
		#self.b_openGet = b_openGet	
		#self.b_openPut = b_openPut
		#self.timeGet = time.time()
		#self.timePut = 0
		#self.ctrTotalPut = 0
		#self.inQueueStats     = gxStats(statsHistLen)
		#self.outQueueStats    = gxStats(statsHistLen)
		#self.outQueueAltStats = gxStats(statsHistLen)
		#self.createQueue(max)	# create the actual queue, specific to this implementation or to a subclass

		if self.queue == None:
			raise RuntimeError, "gxbdbqueue.__init__() or .createQueue() FAILED name: " + name

		g_listOfBdbQueues.append(self)

	# Supplemental methods, used with bdbVarQueue :
	#
	def closeQueue(self, b_quiet=1):
		self.assignOpenStatus(b_openGet=0, b_openPut=0)
		if self.queue.isOpen():
			try:
				self.queue.close()
				return 0
			except:
				ex = gxutils.formatExceptionInfoAsString(10)
				m1 = "gxbdbqueue.closeQueue(): queue %s FAILED, got EXCEPTION, ex=%s." % (self.name, ex, )
				gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)
				return -1
		if not b_quiet:
			m1 = "gxbdbqueue.closeQueue(): queue %s is already closed." % (self.name,  )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		return 0

	#
	# The following methods override the methods with the same names in gxqueue.gxqueue and
	# implement whatever is required for bdbVarQueue.py
	#
	def createQueue(self, max):
		""" version specific to use bdbVarQueue ...
		"""
		self.queue = None	# if this value stays after __init__() then the creation of the queue failed.

		if not g_VarQueueEnv:
			m1 = "%s -- gxbdbqueue.createQueue(): about to call prepareBDBenvironment()..." % (self.name, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			rc = prepareBDBenvironment()
			if rc != 0:
				m1 = "gxbdbqueue.createQueue(): name %s, prepareBDBenvironment() FAILED rc=%s." % (self.name, rc, )
				gxServer.addErrMsg(m1)
				return -1

		try:
			m1 = "%s -- gxbdbqueue.createQueue(): about to call bdbVarQueue.VarQueue()..." % (self.name, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			self.queue = bdbVarQueue.VarQueue(self.name, max, g_VarQueueEnv)
		except:
			ex = gxutils.formatExceptionInfoAsString(10)
			m1 = "gxbdbqueue.createQueue(): name %s, create instance bdbVarQueue.VarQueue() got EXCEPTION, ex=%s." % (self.name, ex, )
			gxServer.addErrMsg(m1)
			return -2

		try:
			m1 = "%s -- gxbdbqueue.createQueue(): about to call self.queue.open()..." % (self.name, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			self.queue.open()
		except:
			ex = gxutils.formatExceptionInfoAsString(10)
			m1 = "gxbdbqueue.createQueue(): name %s, self.queue.open() got EXCEPTION, ex=%s." % (self.name, ex, )
			gxServer.addErrMsg(m1)
			return -2

	def qsize(self):
		""" emulate the Queue.Queue method with the same name
		"""
		return self.queue.getQueueSize()

	def empty(self):
		""" emulate the Queue.Queue method with the same name
		"""
		return self.qsize() <= 0

	def full(self):
		""" emulate the Queue.Queue method with the same name
		"""
		if self.qsize() <= -1:
			return True

		return self.qsize() >= self.max

	def queue_put(self, item, block=1):
		""" Called by:	.put() in super class, gxqueue.gxqueue

		    A subclass can override this low-level method and use another kind of queue implementation than Queue.Queue
		    API specs:
			if optional argument block is 1 (default), the caller blocks until the queue can accept the new item.
			Otherwise (block == 0), if the queue is full, the method raises the Queue.Full exception.
		"""
		while 1:
			if self.full():
				if block:
					time.sleep(PUT_LOCK_SLEEP_DELAY)
					continue
				else:
					raise Queue.Full

			rc = self.queue.put(item) # this implementation uses a bdbVarQueue.VarQueue instance
			if rc <= -1:
				raise Queue.Full	# that's the API specs!

			return

	def queue_get(self, block=1):
		""" Called by:	.get()

		    A subclass can override this low-level method and use another kind of queue implementation than Queue.Queue
		    API specs:
			if optional argument block is 1 (default), the caller blocks until the queue has an item available.
			Otherwise (block == 0), if the queue is empty, the method raises the Queue.Empty exception.
		"""
		while 1:
			if self.empty():
				if block:
					time.sleep(GET_LOCK_SLEEP_DELAY)
					continue
				else:
					raise Queue.Empty

			item = self.queue.get() #this implementation uses a bdbVarQueue.VarQueue instance
			if item == None:
				raise Queue.Empty
			return item

	#
	# End of methods re-implemented to use bdbVarQueue.py
	#
