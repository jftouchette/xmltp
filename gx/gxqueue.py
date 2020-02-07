#!/usr/bin/python

#
# gxqueue.py : Gateway-XML Queue
# ------------------------------
#
#
# $Source: /ext_hd/cvs/gx/gxqueue.py,v $
# $Header: /ext_hd/cvs/gx/gxqueue.py,v 1.3 2006/02/02 14:53:14 toucheje Exp $
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
# gxQueueItem:	objects to be queued on a gxqueue should be sub-classed from 
#		this super-class.
#		The sub-class will inherit the following methods:
#
#			getTimeOfPut()
#			getTimeOfGet()
#			getNbSecondsQueued()
#			getNbSecondsOutside()
#
#		When such sub-class objects are queued, a gxqueue can compute
#		statistics about how long items were queued into it. 
#
#		Also, if a resource pool returns gxQueueItem-derived resources 
#		to a gxqueue after they have been used outside of the queue,
#		the gxqueue will compute stats about the average duration
#		of their use.
#
# gxqueue:	Higher level class which uses Queue.Queue, contains extra
# 		attributes and methods.
#
# This subclass fixes the problem of the sporadic "full" exception
# that happens when doing .put() while many threads are active
# concurrently.  (This happens even though the method is suppose
# to wait -- not like with put_nowait() which raise that exception
# normally. This was observed with Python 2.1.1 on Linux 2.4.7,
# Red Hat 7.2, 2001dec10, jft).
#
# NOTES: the behavior of the put() and get() methods is slightly different
#	 from the behaviour of the same methoods in the queue.queue class in Python.
#	 You better look at the source below. In a few words, it is
#	 like this. If the methods fail, they write a ERROR msg in the 
#	 server log.  Also, put() return (-1) on failure and (0) on success
#	 (even if a retry was required to succeed). As for get(), it returns
#	 an item (whatever was inserted in the queue) when it succeeds
#	 and, None if it fails.  These methods do NOT raise exceptions.
#
# -----------------------------------------------------------------------
#
# 2001dec10,jft: first version
# 2002jan23,jft: a queue can be closed for put() and/or for get()
# 2002feb27,jft: + class gxQueueItem, a super-class from which to derive objects to be queued
# 2002mar01,jft: . gxqueue.__repr__(): also show self.b_openPut, self.b_openGet
#		 + class gxStats
# 2002mar06,jft: + gxqueue.getStatsN(self, statsType)
# 2002jul25,jft: . gxqueue.put(): fixed race condition, moved item.gxQitem_PutTime = putTime
#			before the put().
# 2002sep10,jft: + gxStats.getTimeReset(), gxqueue.getStatsTimeReset(self, statsType)
# 2002sep18,jft: + argument b_summary=0 to gxStats.getCtrBuckets()
#					and to gxqueue.getStatsCtrBuckets()
# 2002oct21,jft: . gxqueue.put(): to avoid thread race condition, calculate tOut before the actual .put()
# 2006jan29,jft: . class gxQueue is _not_ a subclass of Queue.Queue now, but, it emulates the API of Queue.Queue
#		   and it uses an instance of Queue.Queue
#

import sys
import time
import threading
import Queue
import TxLog

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxqueue.py,v 1.3 2006/02/02 14:53:14 toucheje Exp $"

# Constants:
#
MODULE_NAME = "gxqueue"

#
# Public constants, to use when calling:
# 	gxqueue.getStatsCtrBuckets(statsType)
#	gxqueue.getStatsAverage(statsType)
#
STATS_QUEUED_TIME	= 0
STATS_OUT_OF_QUEUE	= 1
STATS_OUT_OF_QUEUE_ALT	= 2


#
# Private constants:
#
# Contant used by class gxStats:
#
gxStats_bucketsLimits = [ 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 50.0, 100.0, 200.0, 500.0 ] 
gxStats_bucketsLimitsAsColNames = None		# will become a list of string after first try... (see function)

gxStats_summaryBucketsLimits = [ 0.02, 0.1, 0.5, 1.0, 10.0 ]
gxStats_summaryBucketsLimitsAsColNames = [ "20 ms", "100 ms", "500 ms", "1 s", "10 s" ]



#
# Module variables:
#
listOfQueues = []

gxServer = None

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
# Public, Module Functions:
#
def getStatsBucketsLimits(b_summary=0):
	if (b_summary):
		return (gxStats_summaryBucketsLimits)

	return (gxStats_bucketsLimits)

def getStatsBucketsLimitsAsColNames(b_summary=0):
	global gxStats_bucketsLimitsAsColNames
	
	if (b_summary):
		return (gxStats_summaryBucketsLimitsAsColNames)

	if (gxStats_bucketsLimitsAsColNames):
		return (gxStats_bucketsLimitsAsColNames)

	ls = []
	for lim in gxStats_bucketsLimits:
		if (lim <= 0.9):
			str = "%d ms" % (lim * 1000, )
		else:
			str = "%d s" % (lim, )
		ls.append(str)

	gxStats_bucketsLimitsAsColNames = ls
	return (ls)


def getQueuesList():
	return (listOfQueues)



#
# Class definition...
#
class gxQueueItem:
	#
	# to avoid clash with attributes names of a subclass, all attributes
	# begin with the prefix "gxQitem_"...
	#
	def __init__(self):
		self.gxQitem_CreateTime = time.time()
		self.gxQitem_PutCtr  = 0
		self.gxQitem_PutTime = None
		self.gxQitem_GetTime = None

	def getTimeOfPut(self):
		return (self.gxQitem_PutTime)

	def getTimeOfGet(self):
		return (self.gxQitem_GetTime)

	def getNbSecondsQueued(self):
		if (self.gxQitem_PutTime == None or self.gxQitem_GetTime == None):
			return (None)

		return (self.gxQitem_GetTime - self.gxQitem_PutTime)

	def getNbSecondsOutside(self):
		if (self.gxQitem_PutTime == None or self.gxQitem_GetTime == None):
			return (None)

		if (self.gxQitem_GetTime > self.gxQitem_PutTime):
			return (None)

		return (self.gxQitem_PutTime - self.gxQitem_GetTime)



class gxStats:
	def __init__(self, maxPrevList=5):
		self.initMsg = None

		if (maxPrevList <= 0 or maxPrevList > 1000):	# avoid absurd maxPrevList values
			self.maxPrevList = 5
			self.initMsg = "Fixed maxPrevList from %s to %s" % (maxPrevList, self.maxPrevList)
		else:
			self.maxPrevList = maxPrevList

		self.timeReset = time.time()
		self.prevAverages = []
		self.resetCountersLowLevel()

	def resetCounters(self):
		if (len(self.prevAverages) >= self.maxPrevList):
			del self.prevAverages[0]

		self.prevAverages.append(self.getAverage() )

		self.resetCountersLowLevel()

	def resetCountersLowLevel(self):
		self.timeReset = time.time()
		self.n		   = 0.0
		self.totalOfValues = 0.0
		self.ctrBuckets  = len(gxStats_bucketsLimits) * [0]

	def getCounters(self):
		return (self.n, self.totalOfValues)

	def getTimeReset(self):
		return self.timeReset

	def getN(self):
		return (self.n)

	def addToStats(self, value):
		self.n		   = self.n + 1.0
		self.totalOfValues = self.totalOfValues + value

		i = 0
		for limit in gxStats_bucketsLimits:
			if (value <= limit):
				self.ctrBuckets[i] = self.ctrBuckets[i] + 1
				break
			i = i + 1

	def getCtrBuckets(self, b_summary=0):
		if (not b_summary):
			return (self.ctrBuckets)

		maxBuckets = len(gxStats_bucketsLimits)
		ls = []
		idx = 0
		for lim in gxStats_summaryBucketsLimits:
			if (idx >= maxBuckets):
				break
			sumBuckets = 0
			while (idx < maxBuckets and gxStats_bucketsLimits[idx] <= lim):
				sumBuckets = sumBuckets + self.ctrBuckets[idx]
				idx = idx + 1
			ls.append(sumBuckets)
		return (ls)


	def getAverage(self):
		if (self.n <= 0.0):
			return (0)

		return (self.totalOfValues / self.n)

	def getAverageHistory(self):
		return (self.prevAverages)


class gxqueue:
	""" All queues in a GxServer should be derived from this gxqueue class
	"""
	def __init__(self, name, owner=None, max=0, b_openGet=1, b_openPut=1, statsHistLen=10):
		self.name  = name
		self.owner = owner 	# pool or gxserver which has created this gxqueue
		self.max   = max
		self.b_openGet = b_openGet	
		self.b_openPut = b_openPut
		self.timeGet = time.time()
		self.timePut = 0
		self.ctrTotalPut = 0

		self.inQueueStats     = gxStats(statsHistLen)
		self.outQueueStats    = gxStats(statsHistLen)
		self.outQueueAltStats = gxStats(statsHistLen)

		self.createQueue(max)	# create the actual queue, specific to this implementation or to a subclass

	#
	# The following methods CAN be re-implemented in a subclass.
	#
	def createQueue(self, max):
		""" a subclass can override this method and use another kind of queue implementation than Queue.Queue
		"""
		self.queue = Queue.Queue(max)

	def qsize(self):
		""" emulate the Queue.Queue method with the same name
		"""
		return self.queue.qsize()

	def empty(self):
		""" emulate the Queue.Queue method with the same name
		"""
		return self.queue.empty()

	def full(self):
		""" emulate the Queue.Queue method with the same name
		"""
		return self.queue.full()

	def queue_put(self, item, block=1):
		""" Called by:	.put()

		    A subclass can override this low-level method and use another kind of queue implementation than Queue.Queue
		    API specs:
			if optional argument block is 1 (default), the caller blocks until the queue can accept the new item.
			Otherwise (block == 0), if the queue is full, the method raises the Queue.Full exception.
		"""
		self.queue.put(item, block)	# this implementation uses a Queue.Queue instance

	def queue_get(self, block=1):
		""" Called by:	.get()

		    A subclass can override this low-level method and use another kind of queue implementation than Queue.Queue
		    API specs:
			if optional argument block is 1 (default), the caller blocks until the queue has an item available.
			Otherwise (block == 0), if the queue is empty, the method raises the Queue.Empty exception.
		"""
		return self.queue.get(block)	# this implementation uses a Queue.Queue instance

	#
	# The following methods should _not_ be re-implemented in a subclass.
	#
	def get_nowait(self):
		return self.get(0)

	def put_nowait(self, item):
		return self.put(item, 0)

	def __repr__(self):
		return ("<%s=%s/%s %s:%s>" (self.name, self.qsize(), self.max, self.b_openPut, self.b_openGet) )

	def isOpen(self, forWhat=0):
		if (forWhat == 0):
			return (self.b_openPut)

		if (forWhat == 1):
			return (self.b_openGet)

		return (self.b_openPut and self.b_openGet)


	def assignOpenStatus(self, b_openGet=None, b_openPut=None):
		#
		# Both values are Boolean, where, 1:true, 0:false
		#
		# BE CAREFUL! 
		# System-internal queues should only be close at very
		# specific stages of the server shutdown.
		#
		# Application-level queues can closed when it is 
		# required to do so.
		#
		if (b_openGet == None and b_openPut == None):
			return (-1)

		if (b_openGet != None):
			self.b_openGet = b_openGet

		if (b_openPut != None):
			self.b_openPut = b_openPut

		return (0)

	def put(self, item, block=1, quiet=0, altStats=0):
		if (not self.b_openPut):
			return (-1)

		msg = "Failed 5 times to put()"

		for i  in xrange(0, 5):
			try:
				# asssign item.gxQitem_PutTime and calculate
				# tOut BEFORE the put()
				# to avoid multi-thread side effects
				# (like a get() before the assignment).
				#
				putTime = time.time()
				if (isinstance(item, gxQueueItem) ):
					item.gxQitem_PutTime = putTime
					tOut = item.getNbSecondsOutside()
				else:
					tOut = None

				self.queue_put(item, block)

				self.ctrTotalPut = self.ctrTotalPut + 1
				self.timePut = putTime

				if (tOut != None):
					if (altStats):
						self.outQueueAltStats.addToStats(tOut)
					else:
						self.outQueueStats.addToStats(tOut)
				return (0)

			except Queue.Full:
				if (self.qsize() >= self.max):
					msg = "Queue full, cannot put()"
					break

				if (getDebugLevel() >= 5 or block):
					m1 = "Queue.Full when put() %s to %s. qsize()=%s" % (
						item, self.name, self.qsize() )

					gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

				time.sleep(1)	# wait 1 second and try again

		if (not quiet):
			m1 = "%s '%s' to %s. qsize()=%s" % (msg, item, self.name, self.qsize() )

			gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)

		return (-2)


	def get(self, block=1):
		if (not self.b_openGet):
			return (None)

		try:
			item = self.queue_get(block)

			if (isinstance(item, gxQueueItem) ):
				item.gxQitem_GetTime = time.time()

				tIn = item.getNbSecondsQueued()
				if (tIn != None):
					self.inQueueStats.addToStats(tIn)

				elif (getDebugLevel() >= 25):
					m1 = "item.getNbSecondsQueued() is None: '%s'" % (item, )
					gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			elif (getDebugLevel() >= 25):
				m1 = "not an instance of gxQueueItem: '%s'" % (item, )
				gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			return (item)
		except Queue.Empty:
			return None


	def getStatsObjForType(self, statsType):
		if (statsType == STATS_QUEUED_TIME):
			return (self.inQueueStats)

		if (statsType == STATS_OUT_OF_QUEUE):
			return (self.outQueueStats)

		return (self.outQueueAltStats)

	def getStatsCtrBuckets(self, statsType, b_summary=0):
		stats = self.getStatsObjForType(statsType)
		return (stats.getCtrBuckets(b_summary) )

	def getStatsAverage(self, statsType):
		stats = self.getStatsObjForType(statsType)
		return (stats.getAverage() )

	def getStatsTimeReset(self, statsType):
		stats = self.getStatsObjForType(statsType)
		return (stats.getTimeReset() )

	def getStatsN(self, statsType):
		stats = self.getStatsObjForType(statsType)
		return (stats.getN() )

	def getStatsAverageHistory(self, statsType):
		stats = self.getStatsObjForType(statsType)
		return (stats.getAverageHistory() )

	def resetStats(self):
		self.inQueueStats.resetCounters()
		self.outQueueStats.resetCounters()
		self.outQueueAltStats.resetCounters()


if __name__ == '__main__':

	gxServer = TxLog.TxLog("toto", "toto1")


	print "getStatsBucketsLimitsAsColNames():"
	print getStatsBucketsLimitsAsColNames()

	q = gxqueue("Queue01", owner=None, max=3)

	q.put("Elem1")
	q.put("Elem2")
	q.put("Elem3")

	print "q.put(), 1,2,3 done"

	q.put("Elem4", 0)

	print q.get()
	print q.get()
	print q.get()
	print q.get()
	print q.get()


