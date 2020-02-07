#
# bdbVarQueue.py -- Berkeley DB Variable-length record Queue
# --------------
#
# $Source: /ext_hd/cvs/gx/bdbVarQueue.py,v $
# $Header: /ext_hd/cvs/gx/bdbVarQueue.py,v 1.4 2006-03-19 19:17:52 toucheje Exp $
#
#  Copyright (c) 2005-2006 Jean-Francois Touchette
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
# NOTE: This Python source file uses hard TAB characters for all indentation. 
#	Please do the same if you modify this file.
#
# bdbVarQueue.py
# --------------
#
# 2005oct20,jft: version 0.1
# 2006jan29,jft: varQueue.get_raw(): calls self.db_data.delete() -- disk space usage is now stable.
#		 varQueue.open() .close(): call self.acquireThreadLock()
#		 replace print statements by conditional calls to logTrace()
# 2006mar15,jft: VarQueue.getStats() .open() .close() .get() .put() : release lock after (DBError) exception
# 2006mar17,jft: VarQueue.getQueueSize(): if not self.b_isOpen: return -2
#		 VarQueue.close(): we set self.b_isOpen before self.dbenv.close() because it seems to hang sometimes(?)
#

import gxutils
import thread
import time
import StringIO
import cPickle
import bsddb3

try:
	import TxLog
	L_MSG	  = TxLog.L_MSG
	L_DEBUG	  = TxLog.L_DEBUG
	L_WARNING = TxLog.L_WARNING 
	L_ERROR	  = TxLog.L_ERROR
except ImportError:
	L_MSG	  = 0
	L_DEBUG	  = 1
	L_WARNING = 2
	L_ERROR	  = 3

# Constants:
#
MODULE_NAME    = "bdbVarQueue"
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/bdbVarQueue.py,v 1.4 2006-03-19 19:17:52 toucheje Exp $"

#
# Module functions for debug and trace:
#
debug_level = 0

def assignDebugLevel(val):
	global debug_level
	debug_level = val

def getDebugLevel():
	return (debug_level)

def getRcsVersionId():
	return (RCS_VERSION_ID)


# For your custom log writing neeeds, please assign a function with the argument list to g_logTraceFunction
# module variable (see below).
#
def stdoutLogTrace(moduleName, level, m1, m2):
	print moduleName, level, m1,m2

g_logTraceFunction = stdoutLogTrace	# the default is the function define just here above

def logTrace(level, m1, m2):
	"""Write log and trace messages by calling the function bound to g_logTraceFunction.
	"""
	g_logTraceFunction(MODULE_NAME, level, m1, m2)

#
# Module variables:
#
g_defaultVarQueueEnv = None

#
# Class definitions...
#
class VarQueueEnv:
	def __init__(self, homeDir=None):
		self.homeDir = homeDir
		self.dbenv   = bsddb3.db.DBEnv()
		self.b_isOpen  = False
		#
		# Some settings which apply to the whole DB Environment:
		#
		self.dbenv.set_tx_max(50)  # Max of (n) concurrent transactions, implies max of (n) VarQueue(s).

	def open(self):
		self.dbenv.open(self.homeDir, bsddb3.db.DB_INIT_MPOOL | bsddb3.db.DB_INIT_LOCK | 
			bsddb3.db.DB_INIT_LOG | bsddb3.db.DB_INIT_TXN | bsddb3.db.DB_THREAD  |
			bsddb3.db.DB_CREATE, 0)
		self.b_isOpen  = True

	def close(self):
		if not self.b_isOpen:
			return 1
		self.b_isOpen  = False		# we set self.b_isOpen before self.dbenv.close() because it seems to hang sometimes(?)
		self.dbenv.close()
		return 0

	def isOpen(self):
		return self.b_isOpen

class VarQueue:
	def __init__(self, name, max=0, VQenv=None):
		self.name = name
		self.fifoDBfileName = name + "_FIFO"
		self.dataDBfileName = name + "_DATA"
		self.max   = max		# @@@ max limit not enforced yet.

		self.threadLock = thread.allocate_lock() # NOTE: threading.Lock() acquire(), release() seem bogus in Python 2.1?
		self.seqNo = 0
		if VQenv:
			self.VQenv = VQenv
		else:
			if not g_defaultVarQueueEnv:
				g_defaultVarQueueEnv = VarQueueEnv()
			self.VQenv = g_defaultVarQueueEnv
			
		self.db_data  = None
		self.db_fifo  = None
		self.b_isOpen = False

	def __repr__(self):
		s = "<%s: %s, %s, %s, %s=%s, %s=%s, VQenv=%s>" % (self.name, self.b_isOpen, self.max, self.seqNo, 
				self.fifoDBfileName, self.db_fifo, self.dataDBfileName, self.db_data, self.VQenv)
		return s

	def open(self):
		try:
			self.acquireThreadLock("open()")
			try:
				self.open_raw()
			except:
				ex = gxutils.formatExceptionInfoAsString()
				m1 = "self.open_raw() Exception ex=%s." % (ex, )
				logTrace(L_WARNING, m1, ex)

			self.releaseThreadLock("open()")
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "self.open_raw() FAILED? ex=%s." % (ex, )
			logTrace(L_WARNING, m1, ex)

	def open_raw(self):
		if not self.VQenv.isOpen():
			self.VQenv.open()

		self.db_data = bsddb3.db.DB(self.VQenv.dbenv)
		self.db_fifo = bsddb3.db.DB(self.VQenv.dbenv)
		self.db_fifo.set_re_len(128)	# set record length must be before .open()
		self.db_fifo.set_q_extentsize(100)  # set extent size (nb pages) must be before .open()

		tx0 = self.VQenv.dbenv.txn_begin()
		self.db_data.open(self.dataDBfileName, None, bsddb3.db.DB_BTREE, bsddb3.db.DB_THREAD | bsddb3.db.DB_CREATE, txn=tx0)
		self.db_fifo.open(self.fifoDBfileName, None, bsddb3.db.DB_QUEUE, bsddb3.db.DB_THREAD | bsddb3.db.DB_CREATE, txn=tx0)
		tx0.commit()

		self.b_isOpen  = True

	def close(self):
		try:
			self.acquireThreadLock("close()")
			try:
				self.close_raw()
			except:
				ex = gxutils.formatExceptionInfoAsString()
				m1 = "self.close_raw() Exception ex=%s." % (ex, )
				logTrace(L_WARNING, m1, ex)

			self.releaseThreadLock("close()")
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "self.close_raw() FAILED? ex=%s." % (ex, )
			logTrace(L_WARNING, m1, ex)

	def close_raw(self):
		if not self.b_isOpen:
			return
		self.db_fifo.close()
		self.db_data.close()
		self.b_isOpen  = False

	def isOpen(self):
		return self.b_isOpen

	def getQueueSize(self):
		""" Return (approximate?) number of items in the queue. 
		    Uses the difference 'cur_recno' - 'first_recno' in the values of the 
		    dictionary returned by self.db_fifo.stat(flags = bsddb3.db.DB_FAST_STAT)
		    to compute the queue size.
		    NOTE: the 'nkeys' element in that dictionary does not provide accurate information, 
			  when (flags = bsddb3.db.DB_FAST_STAT). But, without that flag, the .stat() call
			  can be much too slow.
		    Return -1 if failed.
		    Return -2 if queue is not open.
		    Return -3 if exception caught.
		"""
		try:
			if not self.b_isOpen:
				return -2
			statsDict = self.getStats(b_aboutQueue=1, b_fast=1)
			if not statsDict: 
				if getDebugLevel() >= 10:
					m1 = "getQueueSize(): %s: self.getStats() returned statsDict=%s. %s" % (self.name, statsDict, self, )
					logTrace(L_MSG, m1, "")
				return -1

			curr  = statsDict['cur_recno']		# BSD DB version 4.3+ (2005)
			first = statsDict['first_recno']
			return curr - first
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "getQueueSize() name: %s FAILED? ex=%s." % (self.name, ex, )
			logTrace(L_WARNING, m1, ex)

		return -3

	def getStats(self, b_aboutQueue=1, b_fast=1):
		try:
			stats = None
			self.acquireThreadLock("getStats()")
			try:
				stats = self.getStats_raw(b_aboutQueue, b_fast)
			except:
				ex = gxutils.formatExceptionInfoAsString()
				m1 = "self.getStats_raw() Exception ex=%s." % (ex, )
				logTrace(L_WARNING, m1, ex)

			self.releaseThreadLock("getStats()")

			return stats
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "self.getStats_raw() FAILED? ex=%s." % (ex, )
			logTrace(L_WARNING, m1, ex)
		
	def getStats_raw(self, b_aboutQueue=1, b_fast=1):
		if b_fast:
			flags = bsddb3.db.DB_FAST_STAT
		else:
			flags = 0
		if b_aboutQueue:
			if self.db_fifo == None:
				return
			return self.db_fifo.stat(flags)
		else:
			if self.db_data == None:
				return
			return self.db_data.stat(flags)

	def getIncremSeqNo(self):
		self.seqNo += 1
		return self.seqNo

	def put(self, item):
		try:
			buff = StringIO.StringIO()
			cPickle.dump(item, buff)
			buff.seek(0)
			item = buff.getvalue()

			self.acquireThreadLock("put()")
			try:
				self.put_raw(item)
			except:
				ex = gxutils.formatExceptionInfoAsString()
				m1 = "self.put_raw() FAILED? ex=%s." % (ex, )
				logTrace(L_WARNING, m1, ex)

			self.releaseThreadLock("put()")
			return 0
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "self.put_raw() FAILED? ex=%s." % (ex, )
			logTrace(L_WARNING, m1, ex)
		return -1

	def put_raw(self, item):
		refKey = "%.32s_%20.12f_%s" % (self.name, time.time(), self.getIncremSeqNo(), )
		tx = self.VQenv.dbenv.txn_begin(flags=bsddb3.db.DB_TXN_SYNC)
		#print self.VQenv.dbenv.txn_stat()

		#print "tx.id():", tx.id()
		#print "db_fifo:", self.db_fifo
		if getDebugLevel() >= 10:
			logTrace(L_MSG, "refKey:", refKey)

		if getDebugLevel() >= 50:
			logTrace(L_MSG, "self.db_data.stat():", str(self.db_data.stat() ) )
			logTrace(L_MSG, "self.db_fifo.stat():", str(self.db_fifo.stat() ) )

		#rc = self.db_fifo.append(refKey, tx)	## does NOT work ##
		rc = self.db_fifo.put(0, refKey, txn=tx, flags=bsddb3.db.DB_APPEND)	# works OK

		if getDebugLevel() >= 5:
			m = "self.db_fifo.append() rc=%s" % (rc,)
			logTrace(L_MSG, self.name, m)

		try:
			rc1 = self.db_data.put(refKey, item, txn=tx, flags=0)
			tx.commit()
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "self.db_data.put() FAILED, rc=%s, refKey=%s, item=%s" % (rc1, refKey, item, ) 
			logTrace(L_WARNING, m1, ex)
			tx.abort()

		rc = self.VQenv.dbenv.txn_checkpoint(16, 1, 0)	# (kbyte=8, min=1, flag=0)
		if rc != None and rc != 0:
			m = "self.VQenv.dbenv.txn_checkpoint() FAILED, rc=%s" % (rc, )
			logTrace(L_WARNING, self.name, m)

	def get(self):
		item = None
		try:
			self.acquireThreadLock("get()")
			try:
				item = self.get_raw()
			except:
				ex = gxutils.formatExceptionInfoAsString()
				m1 = "self.get_raw() Exception ex=%s." % (ex, )
				logTrace(L_WARNING, m1, ex)

			self.releaseThreadLock("get()")
			if item != None:
				item = cPickle.load(StringIO.StringIO(item) )
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "self.get_raw() FAILED? ex=%s." % (ex, )
			logTrace(L_WARNING, m1, ex)
		return item

	def get_raw(self):
		tx = self.VQenv.dbenv.txn_begin(flags=bsddb3.db.DB_TXN_SYNC)

		if getDebugLevel() >= 60:
			logTrace(L_MSG, "get_raw() - tx.id():", str(tx.id() ) )

		#print self.db_data.stat()
		#print self.db_fifo.stat()

		tup = self.db_fifo.consume(tx)
		if tup == None:
			tx.abort()
			logTrace(L_MSG, self.name, "get_raw(): Queue is empty.")
			return None

		k, refKey = tup
		refKey = refKey.strip()

		if getDebugLevel() >= 5:
			m = "self.db_fifo.consume() refKey=%s" % (refKey,)
			logTrace(L_MSG, self.name, m)

		item = None
		try:
			item = self.db_data.get(refKey, default=None, txn=tx, flags=0)
			rc = self.db_data.delete(refKey, txn=tx, flags=0)
			tx.commit()

			if getDebugLevel() >= 10:
				m = "self.db_data.get() refKey=%s, self.db_data.delete() rc=%s" % (refKey, rc, )
				logTrace(L_MSG, self.name, m)
		except:
			ex = gxutils.formatExceptionInfoAsString()
			m1 = "self.db_data.get() FAILED, refKey=%s, item=%s" % (refKey, item, ) 
			logTrace(L_WARNING, m1, ex)
			tx.abort()

		rc = self.VQenv.dbenv.txn_checkpoint(16, 1, 0)	# (kbyte=8, min=1, flag=0)
		if rc != None and rc != 0:
			m = "self.VQenv.dbenv.txn_checkpoint() FAILED, rc=%s" % (rc, )
			logTrace(L_WARNING, self.name, m)

		return item		

	def acquireThreadLock(self, funcName):
		#
		# this method is used to BEGIN a  Critical Section
		#
		nbTries = 0
		maxTries = 8		# will let waitDelay increase from 1 usec to 10 seconds
		waitDelay = 0.000001	# 1 usec

		while (nbTries <= maxTries):
			rc = self.threadLock.acquire(0)
			if (rc == 1):
				return (0)	# OK, lock acquired

			if (nbTries >= 4):
				if (nbTries < maxTries):
					m1 = "acquireThreadLock(): acquire() failed %s times, will retry after %s ms. %s." % (nbTries, 
							(waitDelay * 1000), funcName, )
					level = L_MSG
				else:
					m1 = "acquireThreadLock(): acquire() FAILED %s times, NO MORE retry. %s." % (nbTries, funcName, )
					level = L_WARNING
				logTrace(level, self.name, m1)

			if (nbTries < maxTries):
				time.sleep(waitDelay)
				waitDelay = waitDelay * 10

			nbTries = nbTries + 1

		return (-1)


	def releaseThreadLock(self, funcName):
		#
		# to END a Critical Section
		#
		try:
			self.threadLock.release()
		except:
			ex =  gxutils.formatExceptionInfoAsString()
			m1 = "releaseThreadLock(): FAILED release(), Exc: %s, %s." % (ex, funcName, )
			logTrace(L_WARNING, self.name, m1)

			pass	# in case it is called when already unlocked


if __name__ == '__main__':
	import sys
	class Blippy:
		def __init__(self, a,b,c):
			self.a = a
			self.b = b
			self.c= c
		def __repr__(self):
			str = "Blippy(%s,%s,%s)" % (self.a,self.b,self.c,)
			return str

	if len(sys.argv) < 2:
		print "usage: bdbVarQueue.py { G | P }"
		raise SystemExit
	queue_action = sys.argv[1].upper()

	if len(sys.argv) >= 3 and sys.argv[2].upper() == "DEBUG":
		assignDebugLevel(10)
		print "DEBUG", getDebugLevel()
	
	logTrace(L_MSG, sys.argv[0], "starting...")

	vqe = VarQueueEnv("t1")
	q1  = VarQueue("Q1", VQenv=vqe)
	q1.open()
	q2  = VarQueue("Q2", VQenv=vqe)
	q2.open()
	#print "q1.getStats():", q1.getStats()
	#print "q2.getStats():", q2.getStats()
	print "q1.getQueueSize():", q1.getQueueSize()
	print "q2.getQueueSize():", q2.getQueueSize()

	if queue_action == 'G':
		x = q1.get()
		while (x):
			print "q1.get():", x
			x = q1.get()
	else:
		for i in xrange(1000):
			q1.put( Blippy("item__________________________________________________________________________80",i,time.time() ) )
	logTrace(L_MSG, sys.argv[0], "Stopping.")

	#print "q1.getStats():", q1.getStats()
	#print "q2.getStats():", q2.getStats()
	#print "q1.getStats() data:", q1.getStats(b_aboutQueue=0)
	#print "q1.getStats() queue, no fast:", q1.getStats(b_fast=0)
	print "q1.getQueueSize():", q1.getQueueSize()
	print "q2.getQueueSize():", q2.getQueueSize()
	print "q1.getStats():", q1.getStats()
	print "q1.getStats() queue, no fast:", q1.getStats(b_fast=0)
	print "q1.getStats() data, no fast:", q1.getStats(b_aboutQueue=0, b_fast=0)

	q1.close()
	q2.close()
	vqe.close()
