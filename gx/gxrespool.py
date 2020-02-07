#!/usr/bin/python

#
# gxrespool.py : Gateway-XML RESources POOL
# -----------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxrespool.py,v $
# $Header: /ext_hd/cvs/gx/gxrespool.py,v 1.3 2006/02/02 14:53:14 toucheje Exp $
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
# A "gxrespool" allows the use of a fixed number of "resources" kept in a pool.
#
# Typically, there are (2) two kinds of resources: 
#	- active resources: such as threads performing short-lived
#			    tasks (like parsing or executing "registered 
#			    procedures")
#	- passive resources: such as large buffers, files, or,
#			     database connections.
#
# NOTE: In the design of the GX server(s), we have decided to 
#	implement databases connections as active resources (threads).
#
# 	The implementation in this module is currently only for 
#	active resources.
# 
#
# For more details, see the HOWTO of class gxrespool (below).
# --------------------------------------------------
#
# -----------------------------------------------------------------------
#
# 2001nov12,jft: first version
# 2001dec10,jft: . import gxqueue, do not import Queue
# 2002jan25,jft: + startQueueThreadInAllResPools(): start all queue threads in all pools
#		 + StopAllResPools()
# 2002feb20,jft: . ResPoolMonitor.processOneRequest(): try to recover exception in call to res.processRequest(request)
# 2002feb25,jft: + gxresource.getData2()
# 2002mar01,jft: + gxrespool.getWaitQueue()
#		 + gxrespool.getResourcesQueue()
#		 + gxrespool.getEnableStatus()
# 		 . class gxresource is sub-classed from gxqueue.gxQueueItem to allow stats on gxrespool's queues
# 2002mar11,jft: . ResPoolMonitor.processOneRequest(): res.gxQitem_GetTime = time.time() # to compute the right stats
# 2002apr15,jft: . StopAllResPools(): also call the optional method shutdownResources() for each pool
#		 . ResPoolMonitor.processOneRequest(): if (aThread.pool.b_stopThread) put back res in the pool res queue
# 2002aug12,jft: . gxresource.__init__(): self.reentry_ctr = 0	# could be use by a method that has to avoid recursion
# 2002aug14,jft: + assignOperationToAllPoolsOfThisType(oper, poolType, b_force)
#		 . ResPoolMonitor.processOneRequest(): added support of "operation" to do on all resources of a pool
#		 + gxrespool.putResInQueueExt(self, res, b_newCreate=0, b_altStats=0)
#		 + gxrespool.getOperationToDo(self)
# 2002aug20,jft: . ResPoolMonitor.processOneRequest(): make sure stats are not altered when doing an operation
# 2002oct19,jft: . gxresource.__init__(): added field remoteResourceId
# 2002oct20,jft: + ResPoolMonitor.spawnRequestThread(self, aTread, res, request), which is called by ResPoolMonitor.processOneRequest()
#		 . ResPoolMonitor.processOneRequest(): first check if privileged request available from aThread.pool.privRequestsQueue.get(block=0)
#		 . gxrespool.__init__(): self.privRequestsQueue = gxqueue.gxqueue(name+"PrivRQ", owner=self, max=100)
#		 + gxrespool.addPrivRequest(self, request), which might be called by error recovery code in subclass
# 2002oct21,jft: . gxresource.__init__(): added field b_processingPrivRequest
#		 . ResPoolMonitor.processOneRequest(): assigns res.b_processingPrivRequest = 0 or 1 before calling spawnRequestThread()
# 2003jul04,jft: + gxserver.getStatusAsRow(), gxserver.getStatusAsRowHeaders()
# 2006feb01,jft: . write fewer TxLog.L_WARNING in the log. Replaced 3 of 4 with TxLog.L_MSG
#

import time
import sys
import gxqueue
import gxthread
import gxmonitor
import gxutils
import TxLog

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxrespool.py,v 1.3 2006/02/02 14:53:14 toucheje Exp $"

#
# Module Constants:
#
DEFAULT_DEBUG_LEVEL = 1
MODULE_NAME	    = "gxrespool"



#
# module variables:
#
gxServer = None
ListOfAllResPools = []
CtrOfUnnamedPools = 0




#
# Module functions:
#
def startQueueThreadInAllResPools():
	#
	# NOTE: This function can only be called ONE TIME, after ALL gxrespool have been created.
	#
	m1 = "Starting the queue threads of %s gxrespool(s)..." % (len(ListOfAllResPools), )
	gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

	for resPool in ListOfAllResPools:
		rc = resPool.startQueueThread()
		if (rc != 0):
			m1 = "Start of queue thread of '%s' FAILED." % (resPool.getName(), )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			raise RuntimeError, m1


def assignOperationToAllPoolsOfThisType(oper, poolType, b_force):
	#
	# Called by:	gxregprocs.py
	#
	# Return:
	#	list of error messages (empty if complete success).
	#
	ctr = 0
	ls = []
	for resPool in ListOfAllResPools:
		if (resPool.getResPoolType() != poolType):
			continue

		rc = resPool.assignOperation(oper, b_force)
		if (rc):
			ls.append(rc)
		ctr = ctr + 1
	if (ctr == 0):
		m1 = "No respool of type '%s'." % (poolType, )
		return (m1, )
	return ls



def StopAllResPools(b_quiet=0):
	#
	# NOTE: This function can only be called ONE TIME, for the shutdown of the server.
	#
	if (not b_quiet):
		m1 = "Stopping all %s gxrespool(s): >>>%s<<<" % (len(ListOfAllResPools), repr(ListOfAllResPools) )
		gxServer.writeLog(MODULE_NAME,TxLog.L_WARNING, 0, m1)

	for r in ListOfAllResPools:
		if (not b_quiet):
			print "Stopping", r.getName()

		r.assignStopThreadFlagAsTrue()
		try:
			r.shutdownResources(b_quiet)
		except AttributeError:		# this method may exist or not
			pass

		if (not b_quiet):
			print "Stop", r.getName(), "Done"


#
# Several classes...
#
class ResPoolMonitor(gxmonitor.gxmonitor):
	""" Used by the Requests Queue Thread of any gxrespool instance 
	    (and subclass instances).
	"""
	def runThread(self, aThread):
		#
		# NOTE: aThread is a gxThread instance and has more
		#	attributes than a basic Python thread.
		#	See gxThread module for details.
		#
		pool = None
		poolName = "UNKNOWN-yet"
		try:
			pool = aThread.pool
			poolName = pool.getName()

			while (not aThread.pool.b_stopThread):
				self.processOneRequest(aThread)

			m1 = "resPool '%s' is stopping. %s" % (poolName, pool)
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "Exception in ResPoolMonitor of '%s': %s" % (poolName, ex)
			if (pool):
				try:
					pool.exceptionMsg = m1
				except: pass
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)


	def processOneRequest(self, aThread):
		#
		# Called by:	runThread()
		#
		aThread.changeState(gxthread.STATE_WAIT_RESOURCE)

		# GET a RESOURCE, block if none available:
		#
		res = aThread.pool.resourcesQueue.get()

		if (aThread.pool.b_stopThread):
			aThread.pool.resourcesQueue.put(res)	# put back res in the pool res queue
			aThread.changeState(gxthread.STATE_SHUTDOWN)
			return (1)

		aThread.changeState(gxthread.STATE_WAIT)

		# Check if there is a priviledged request to process:
		#
		privReq = aThread.pool.privRequestsQueue.get(block=0)
		if (privReq):
			res.b_processingPrivRequest = 1		# indicate this resource is processing a priviledged request
			return self.spawnRequestThread(aThread, res, privReq)
		# 
		# usually, privReq is None

		# GET a REQUEST, block if none is available:
		#
		request = aThread.pool.requestsQueue.get()

		if (aThread.pool.b_stopThread):
			#
			# NOTE: if this flag is set, the request will never be processed.
			#
			aThread.changeState(gxthread.STATE_SHUTDOWN)
			return (2)

		# If there is an operation to do on the res of this pool,
		# process each res until we get one whose .tStampLastOper 
		# is >= the .tStampOperation of the pool.
		# Loop until we have a res == None after get or until
		# all resources have been processed.
		#
		while (aThread.pool.operationToDo and res and res.tStampLastOper < aThread.pool.tStampOperation):
			res.gxQitem_GetTime = None	# make sure stats are not updated
			aThread.pool.putResInQueueExt(res)  # this does the operation
			res = None
			ctr = 0
			while (not res and ctr < 5):
				res = aThread.pool.resourcesQueue.get(block=0)
				if (res):
					break
				ctr = ctr + 1
				time.sleep(0.001)		# sleep() 1 ms

		# If we have exhausted the resources in the previous
		# loop, when doing several operations, get a good res
		# here:
		if (not res):
			res = aThread.pool.resourcesQueue.get()

		res.b_processingPrivRequest = 0		# indicate this resource is processing a Normal request

		return self.spawnRequestThread(aThread, res, request)

	def spawnRequestThread(self, aThread, res, request):
		#
		# Called by:	processOneRequest()
		#

		# The update of the _GetTime in the resource is required in order
		# to have meaningfull statististics about resources use.
		#
		# res.gxQitem_GetTime had been updated when the resource was fetched out
		# of the .resourcesQueue, but, this could have happen a few seconds ago!
		#
		if (isinstance(res, gxqueue.gxQueueItem) ):
			res.gxQitem_GetTime = time.time()

		if (type(request) == type("anyString") ):
			# then this is a dummy request, release the res, do nothing.
			#
			res.gxQitem_GetTime = None
			aThread.pool.releaseResource(res)
			return 0

		aThread.changeState(gxthread.STATE_ACTIVE)

		# PROCESS the REQUEST:
		#
		# res.processRequest(request) must spawn a Thread
		# to process the request.
		#
		try:
			res.processRequest(request)
		except:
			# It seems that t.start() can fail under very special conditions (??)
			# if this happens, log a message, and release the resource, and,
			# put back the client in the poll list of the server...
			#
			# The client will timeout after a while... 
			# (This is a rare event! It is difficult to recover well).
			#
			ex = gxutils.formatExceptionInfoAsString(3)

			pool = None
			poolName = "UnknownPoolName"
			frameInfo = "request/frame/client unknown"
			try:
				pool = aThread.pool
				poolName = pool.getName()
				frameInfo = repr(request)
			except:
				pass

			m1 = "EXCEPTION caught by '%s' in processOneRequest() of %s, Exc: %s" % (poolName, frameInfo, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			try:
				aThread.pool.releaseResource(res)
				request.clientConn.rpcEndedReturnConnToPollList(0, "Exception caught")
			except:
				ex = gxutils.formatExceptionInfoAsString(3)
				m1 = "RECOVERY FAILED by '%s' in processOneRequest() of %s, Exc: %s" % (poolName, frameInfo, ex)
				gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)


		return (0)


class gxresource(gxqueue.gxQueueItem):
	#
	# NOTE: gxresource is sub-classed from gxqueue.gxQueueItem specifically
	#	to allow the gxrespool resource queue to compute stats.
	#
	def __init__(self, name=None, data=None, monitor=None, owner=None, pool=None, data2=None):

		gxqueue.gxQueueItem.__init__(self)	# superclass __init__()

		if (name == None):
			self.name = "gxresource_X"
		else:
			self.name = name
		self.state = None

		self.remoteResourceId        = None	# current one (if one exists)
		self.b_processingPrivRequest = None	# currently processing priviledged request (or not)

		self.data  = data	# usually data is a Dest Conn or a parser Context

		self.data2 = data2	# some resources need to hold an other data

		self.monitor = monitor	# the monitor that will run in the thread
		self.owner = owner
		self.pool  = pool
		self.usedByThread = None  # will get a value later, when the resource is used

		self.reentry_ctr = 0	# could be use by a method that has to avoid recursion

		self.b_outcomeOfUseSuccessful = None
		self.b_dirty = 0

		self.tStampLastOper = time.time()

	def __repr__(self):
		if (self.usedByThread != None):
			usedByThreadid = self.usedByThread.getId()
		else:
			usedByThreadid = None
		if (self.monitor != None):
			monName = ", mon=" + self.monitor.getName()
		else:
			monName = ""
		return ("<%s, %s, data=%s%s, owner=%s, pool=%s, used by:%s>" % (self.name, self.state,
			self.data, monName, self.owner.getId(), self.pool.getName(), usedByThreadid ) )

	def getName(self):
		return (self.name)

	def assignUsedByThread(self, thr):
		self.usedByThread = thr

		self.b_outcomeOfUseSuccessful = None	# maybe, maybe not

	def assignOutcomeOfUse(self, b_success=1, b_dirty=0):
		self.b_outcomeOfUseSuccessful = b_success
		self.b_dirty = b_dirty

	def assignMonitor(self, mon):
		self.monitor = mon

	def assignState(self, val):
		self.state = val

	def getState(self):
		return (self.state)

	def processRequest(self, request):
		#
		# This Method MUST be overidden by a subclass!
		#
		print "<gxresource>.processRequest() must be overidden. Request:", request
		return None
	

	def getData(self):
		return (self.data)

	def getData2(self):
		return (self.data2)

	def destroy(self):
		#
		# default method -- usually, it is overidden by subclass
		#
		return None	# do nothing

	def isSane(self):
		#
		# return (1) if resource res is OK, else, return (0)
		#
		# default method -- usually, it is overidden by subclass
		#
		return (self.b_dirty == 0)



class gxrespool:

 # All resources pools in a GxServer should be derived from this gxrespool class
 #
 # HOWTO:
 # -----
 # Each derived class should override the following method:
 #	. createNewResource(aList): it receives the resCreationArgsList argument
 #				as argument and it create a new resource.
 #
 # Also, it might override these methods, but, the generic version of each
 # might be enough:
 #	. destroyResource(res): perform any cleanup required on the resource,
 #				then, free it.
 #				The default version calls res.destroy()
 #				and decrements resCreatedCtr.
 #	. isResourceOK(res):    return (1) if resource res is OK, 
 #				otherwise, return (0).
 #				The default version return res.isSane()
 #
 # The following method does NOT need to be overridden:
 #	. releaseResource(res): if isResourceOK(res), simply adds back res
 #				to the queue resourcesQueue.
 #				Else, it calls destroyResource(res),
 #				and, then, calls rebuildRes().
 #
 # At the time to create an instance object of a subclass of gxrespool, 
 # the following arguments are required:
 #
 #	. name:	  the name of the pool (to document operation procedures).
 #
 #	. maxRes: maximum number of resources elements allocated at any time.
 #		  This maximum MUST BE specified and it should be reasonable.
 #		  NOTE: This max must respect operating system limitations 
 #		  	about the number of threads per process.
 #
 #	. resCreationArgsList: a list containing informations required to
 #				create a new resource instance.
 #
 # other arguments are optional:
 ###	. resThreadMonitor:    (attribute removed 2002jan24,jft)
 #
 #	. maxRequests:	usually, the default, 0, is used. It means there
 #			are no limits to the size of the requests queue.
 #
 # Important instance attributes:
 #	. reqQueueMon: request queue monitor, this instance of a gxmonitor
 #			subclass runs a thread that pop() requests
 #			out of the requests queue when a resources is 
 #			available. (If no resource is available, it blocks
 #			until one is).
 #
 #!	. b_stopThread: Boolean, if True, it indicates that the monitor thread
 #			should exit as soon as possible and that no further request
 #			should be processed (as for a shutdown).
 #
 # ---------------------------------------------------------------------


	def __init__(self, name=None, 
			   maxRes=1, 
			   resCreationArgsList=None, 
			   maxRequests=0,
			   master=None,
			   b_postponeBuildRes=0):
		if (maxRes <= 0  or  maxRes >= 100):
			m1 = "maxRes = %d is not a valid value" % (maxRes, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			raise RuntimeError, m1


		if (maxRequests < 0):
			m1 = "maxRequests = %d is not a valid value" % (maxRequests, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			raise RuntimeError, m1

		global CtrOfUnnamedPools

		if (name == None):
			CtrOfUnnamedPools = CtrOfUnnamedPools + 1
			self.name = "gxrespool_%d" % (CtrOfUnnamedPools, )
		else:
			self.name = name

		self.maxRes = maxRes
		self.resCreationArgsList = resCreationArgsList
		self.maxRequests = maxRequests
		self.master = master

		self.b_open	  = 1
		self.b_stopThread = 0
		self.exceptionMsg = ""

		self.ctrResDone = 0
		self.operationToDo = None		# no operation to do initially
		self.tStampOperation = time.time()

		self.resourcesQueue = gxqueue.gxqueue(name+"ResQ",  owner=self, max=maxRes)
		self.requestsQueue  = gxqueue.gxqueue(name+"WaitQ", owner=self, max=maxRequests)
		self.privRequestsQueue = gxqueue.gxqueue(name+"PrivRQ", owner=self, max=100)

		self.msg = ""
		self.debugLevel = DEFAULT_DEBUG_LEVEL

		self.reqQueueMon = ResPoolMonitor(self.name + "Mon")
		t = gxthread.gxthread(self.name + "QueueThr", monitor=self.reqQueueMon, 
					pool = self,
					threadType=gxthread.THREADTYPE_QUEUE)

		self.reqQueueThread = t
		#
		# Do NOT start() self.reqQueueThread immediately.
		#
		# self.reqQueueThread will be started later by calling .startQueueThread()
		#

		self.resThreadsCtr = 0
		self.resCreatedCtr = 0
		self.resIdentifierCtr = 0

		if (not b_postponeBuildRes):
			self.buildAllResources()

		ListOfAllResPools.append(self)


	def __repr__(self):
		status = self.getEnableStatus()

		if (self.exceptionMsg):
			status = status + " " + self.exceptionMsg

		return ("{%s: %s, res:%s/%s, requests:%s/%s reqQthread=%s} " % (self.name, 
				status,
				self.resourcesQueue.qsize(), self.maxRes,
				self.requestsQueue.qsize(),  self.maxRequests,
				self.reqQueueThread) )

	def getName(self):
		return (self.name)

	def getResPoolType(self):
		# this method allows to identify a type of pools when iterating
		# on the list of all pools.
		return 0		# subclass might have another type

	def getStatusAsRow(self):
		status = self.getEnableStatus()

		if (self.exceptionMsg):
			excMsg = self.exceptionMsg
		else:
			excMsg = ""

		row = [ self.name, status,
			self.resourcesQueue.qsize(), self.maxRes,
			self.requestsQueue.qsize(),  self.maxRequests,
			excMsg ]

		return row

	def getStatusAsRowHeaders(self):
		h = [ "name____", "status", 
		      "nRes", "maxRes", 
                      "nRequests", "max_Req", 
                      "exception_message_____" ]

		return h

	def isOpen(self):
		return (self.b_open  and  not self.b_stopThread)

	def assignOpenStatus(self, b_open):
		self.b_open = b_open

	def getWaitQueue(self):
		return (self.requestsQueue)

	def getResourcesQueue(self):
		return (self.resourcesQueue)

	def getEnableStatus(self):
		if (self.b_stopThread):
			return ("STOP")
		if (self.b_open):
			return ("Open")
		return ("CLOSED")

	def assignDebugLevel(self, val):
		self.debugLevel = val

	def getDebugLevel(self):
		return (self.debugLevel)

	def getRcsVersionId(self):
		return (RCS_VERSION_ID)


	def startQueueThread(self):
		#
		# Must be called after __init__() to start the queue thread.
		#
		try:
			self.reqQueueThread.start()

			m1 = "Queue thread of gxrespool '%s' started successfully." % (self.name, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
			return (0)
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "start() of queue thread of gxrespool '%s' failed with: %s" % (self.name, ex)
			
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			self.rememberErrorMsg("self.reqQueueThread.start() FAILED", ex)
		return (-1)

	def assignStopThreadFlagAsTrue(self):
		#
		#
		# BE CAREFUL! 
		# System-internal threads should only be stopped at very
		# specific stages of the server shutdown.
		#
		if (self.b_stopThread):
			m1 = "b_stopThread flag already set for Queue of gxrespool '%s'..." % (self.name, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
			return (0)

		m1 = "About to STOP Queue thread of gxrespool '%s'..." % (self.name, )
		gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		self.b_open	  = 0
		self.b_stopThread = 1

		#
		# Add Dummy request and resource to unblock the monitor thread:
		#
		self.requestsQueue.put("DummyRequest_ForStopping", block=0)

		self.resourcesQueue.put("DummyResource_ForStopping", block=0, quiet=1)

		return (0)


	def rememberErrorMsg(self, m1, m2):
		self.msg = "%s, %s" % (m1, m2)

	def buildAllResources(self):
		for i in xrange(self.resCreatedCtr, self.maxRes):
			self.rebuildRes()

	def assignOperation(self, oper, b_force):
		# Return:
		#		0	OK
		#		string	if failed
		#
		if (self.operationToDo and not b_force and self.ctrResDone < self.maxRes):
			m1 = "Cannot assign operation %s on '%s' because previous oper %s is not completed (%s of %s res done)." % (oper, self.getName(), self.operationToDo, self.ctrResDone, self.maxRes)
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
			return m1

		self.ctrResDone = 0
		self.operationToDo = oper
		self.tStampOperation = time.time()

		# this thread must NOT block on a put()
		# That would kill (deadloack) this resource pool!
		#
		self.requestsQueue.put("DummyReq_kick_for_operation", block=0)

		return 0

	def getOperationToDo(self):
		return self.operationToDo

	def doOperation(self, res, b_newCreate):
		# 
		# Called by:	doOperation_Protected() -- ONLY
		#
		# NOTE: each subclass must override this method!
		#
		return		# generic version does nothing

	def doOperation_Protected(self, res, b_newCreate):
		#
		# This PRIVATE method must NOT be overidden by a subclass!
		#
		# Called by:	putResInQueueExt() -- ONLY
		#
		oper = "[unknownOper]"
		resName = "[unknownResName]"
		try:
			resName = res.getName()
			oper = self.getOperationToDo()

			self.doOperation(res, b_newCreate) 
		except:
			excStr = gxutils.formatExceptionInfoAsString(2)
			m1 = "doOperation(): %s on '%s': failed with Exc: %s." % (oper, resName, excStr)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	def putResInQueueExt(self, res, b_newCreate=0, b_altStats=0):
		#
		# Called by:	rebuildRes()
		#		releaseResource()
		#
		# Return:
		#	0	success
		#	-1	failed
		#
		if (self.operationToDo and res.tStampLastOper < self.tStampOperation):
			self.doOperation_Protected(res, b_newCreate)

			res.tStampLastOper = time.time()
			self.ctrResDone = self.ctrResDone + 1

			if (self.ctrResDone >= self.maxRes):
				oper = self.operationToDo
				self.operationToDo = None
				if (self.getDebugLevel() >= 10):
					m1 = "completed operation %s on '%s'. %s res done." % (oper, self.getName(), self.ctrResDone)
					gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
		
		return  self.resourcesQueue.put(res, altStats=b_altStats)

	def rebuildRes(self):
		r = self.createNewResource(self.resCreationArgsList)
		if (r != None):
			if (self.putResInQueueExt(r, b_newCreate=1) == 0):
				self.resCreatedCtr = self.resCreatedCtr + 1

	def createNewResource(self, aList): 
		#
		# This Method MUST be overidden by a subclass!
		# --------------------------------------------
		#
		# this function receives the pool.resCreationArgsList
		# as argument (aList) and it creates a new resource.
		#
		res = gxresource(name=self.name + "Res_" + repr(self.resIdentifierCtr), 
				 data=None,		# usually comes from aList
				 monitor=None, 		# usually comes from aList
				 owner=self.reqQueueThread, 
				 pool=self)
		self.resIdentifierCtr = self.resIdentifierCtr + 1
		return (res)

	def destroyResource(self, res): 
		# perform any cleanup required on the resource,
		# then, free it.
		#
 		res.destroy()
		self.resCreatedCtr = self.resCreatedCtr - 1

	def isResourceOK(self, res):
		# return (1) if resource res is OK, 
		# otherwise, return (0).
		#
		return (res.isSane() )

	def releaseResource(self, res, b_useSuccessful=None): 
		#
		# First, remove links between resource and thread.
		# Then, if isResourceOK(res), simply adds back res
		# to the queue resourcesQueue.
		# Else, calls destroyResource(res),
	 	# and, then, calls rebuildRes().
		#

		# First, remove links between resource and thread:
		#
		if (res.usedByThread != None):
			res.usedByThread.assignResource(None)

		res.assignUsedByThread(None)

		# If still good, re-queue the resource,
		# if not, re-create it...
		#
		if (b_useSuccessful != None):
			b_altStats = not b_useSuccessful
		else:
			b_altStats = (res.b_outcomeOfUseSuccessful == 0)

		if (self.isResourceOK(res) ):
			self.putResInQueueExt(res, b_altStats=b_altStats)
		else:
			self.destroyResource(res)
			self.rebuildRes()

	def incrementGetThreadsCtr(self):
		self.resThreadsCtr = self.resThreadsCtr + 1
		return (self.resThreadsCtr)

	def addRequest(self, request):
		# return (0)  if acccepted (queue not full, or no limit)
		# return (-1) if rejected  (queue full or pool stopped)
		#
		if (self.b_stopThread):
			return (-1)	# pool is stopped already!

		if (self.maxRequests != 0  and  self.requestsQueue.full() ):
			return (-1)	# requests queue is full

		return (self.requestsQueue.put(request) )

	def addPrivRequest(self, request):
		#
		# queue a Priviledged Request
		#
		# return (0)  if acccepted (queue not full, or no limit)
		# return (-1) if rejected  (pool stopped)
		# return (-2) if queue full
		#
		if (self.b_stopThread):
			return (-1)	# pool is stopped already!

		if (self.privRequestsQueue.full() ):
			return (-2)	# this is weird, could be a loop in recovery code of caller?

		rc = self.privRequestsQueue.put(request)

		# Add Dummy request to unblock the monitor thread:
		#
		self.requestsQueue.put("DummyRequest_forUnblockingMon_afterPut_PrivRequestsQueue", block=0)

		return rc



#
# Classes used for UNIT TESTING:
#
class UnitTestResourceMonitor(gxmonitor.gxmonitor):
	def runThread(self, aThread):
		aThread.changeState(gxthread.STATE_ACTIVE)
		print "TEST -- Processing request: frame=%s, pool=%s, monitor=%s, %s" % (aThread.frame,
			aThread.pool.getName(), aThread.monitor, aThread.getId())
		aThread.changeState(gxthread.STATE_WAIT)
		time.sleep(2)
		aThread.changeState(gxthread.STATE_COMPLETED)
		#
		# print "DONE --", aThread, "releasing res=", aThread.resource, "..."
		#
		aThread.pool.releaseResource(aThread.resource)




class UnitTestResource(gxresource):
	def processRequest(self, request):
		#
		# This Method is only for Unit Testing of this module
		#
		threadIdSuffix = repr(self.pool.incrementGetThreadsCtr() )

		print "processRequest(): resource=", self, "request=", request

		t = gxthread.gxthread(self.name + "ResThr_" + threadIdSuffix,
					monitor=self.monitor,
					resource=self,
					frame=request,
					pool=self.pool,
					threadType=gxthread.THREADTYPE_RPC)

		t.start()



class UnitTestResPool(gxrespool):
	def createNewResource(self, aList): 
		#
		# this function receives the pool.resCreationArgsList
		# as argument (aList) and it creates a new resource.
		#
		mon = aList[0]
		res = UnitTestResource(name="UnitTestRes_" + repr(self.resCreatedCtr), 
				 monitor=mon, 		# mon comes from aList[0]
				 data ="NoData",		# ...usually comes from aList
				 owner=self.reqQueueThread, 
				 pool=self)
		print "new resource created:", res
		return (res)




if __name__ == '__main__':

	unitTestResourceMonInstance = UnitTestResourceMonitor("UnitTestResourceMon")

	pool = UnitTestResPool(name="TestPool", 
				maxRes=3, 
				resCreationArgsList=[unitTestResourceMonInstance, ],
				maxRequests=0)

	pool.addRequest("req_01")
	pool.addRequest("req_02")
	pool.addRequest("req_03")
	print pool
	print "3 requests added, adding 4th, 5th..."
	pool.addRequest("req_04")
	print "4th added"
	pool.addRequest("req_05")
	print "5th added"
	print pool
	pool.addRequest("req_06")
	print "6th added"
	print "sleep 5 seconds before adding 7,8,9"
	time.sleep(5)
	pool.addRequest("req_07")
	pool.addRequest("req_08")
	pool.addRequest("req_09")
	print "7,8,9 added "
	print pool
	time.sleep(8)
	print pool
	pool.addRequest("req_10")
	pool.addRequest("req_11")
	pool.addRequest("req_12")
	pool.addRequest("req_13")
	pool.addRequest("req_14")
	print "10,11,12,13,14 added "
	print pool
	time.sleep(8)
	print pool
	time.sleep(8)
	print pool

