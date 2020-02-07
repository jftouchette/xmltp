#!/usr/bin/python

# XML-tp LOADtest client : xml_load.py
# ------------------------------------
#
#
# $Source: /u/home/jf/src/uxsrv/gx/RCS/xml_load.py,v $
# $Header: /u/home/jf/src/uxsrv/gx/RCS/xml_load.py,v 1.4 2003/01/24 10:19:44 jf Exp $
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
# Multi-thread client for testing (with random sleep) to test 
# a XML-TP server.
#
# ** xml_load.py is based on xml_clt.py
#
# FEATURES:
#	. thread-safe random sleep between RPC calls
#	. more complete global average report:
#		. avg wait between RPC calls (per thread)
#		. ctr failed/success
#		. metrics: TPM target
#		.	   TPM theoretical
#		. 	   TPM observed
#
# TO IMPLEMENT:
#		. best/worse/avg
#	. stop if any login fails
#	. parameters in RPC call
#
#
# -----------------------------------------------------------------------
#
# 2002jul22,jft: the value for sleep() is completely different from the one in xml_clt.py
# 2002jul25,jft: corrected observed_TPM, added inter-arrival
# 2002aug03,jft: + output one-line comma-separated report in "loadtest.csv"
# 2002aug12,jft: . corrected observed_TPM, added delta_elapsed, avg_elapsed in .csv report
# 2002sep01,jft: + can have one param at end of  command line
#		 . write procName in .csv report file
# 2002sep02,jft: . writeCSVheader(): added "sp_name" to end of header
# 2004feb16,jft: cxMon.userid = "oper" and same for .password
#

import struct
import types
import os
import time
import random		# better than whrandom
import socket
import threading
import string
import sys
import TxLog
import gxutils
import gxqueue		# for class gxStats
import xmltp_tags


b_printBuffer = 0

# ------------------------------------------- Constants:
#
XMLTP_EOT_TAG = "<EOT/>"
LOG_FILENAME  = "gxserver.log"

MODULE_NAME = "xml_load"

srvlog = TxLog.TxLog(LOG_FILENAME, MODULE_NAME)
srvlog.setServerName(MODULE_NAME)
srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "XML LOADTEST client starting...")

gxqueue.gxServer = srvlog	# module gxqueue will call .writeLog()

# Name of report file:
#
REPORT_FILENAME = "loadtest.csv"

# Constants:
#
ACTION_INIT = 0
ACTION_WAIT = 1
ACTION_RECV = 2
ACTION_SEND = 3
ACTION_CLOSE = 4
ACTION_CONNECT = 5
ACTION_NEW_SOCKET = 6
ACTION_SHUTDOWN   = 7
ACTION_ABORTED    = 9
ACTION_COMPLETED  = 10

class ConnXorMonitor:
	def __init__(self, host, port, maxXor, maxCycles, delayStartNew, sp_name):
		self.text_1 = None
		self.text_2 = None
		self.b_noSleep = 0
		self.maxSleepSeconds = 1.0	# default, will be changed
		self.debugLevel = 0
		self.saidSO_RCVTIMEOnotSupported = None	# not checked yet
		self.abortFlag = 0
		self.text = None
		self.sp_name = sp_name
		self.param1 = None
		self.xorList = []
		self.userid   = "op"
		self.password = "op"
		#
		# self.semCtrRunningXor is used to synchronize
		# access to self.ctrRunningXor :
		self.semCtrRunningXor = threading.Semaphore()
		self.ctrRunningXor  = 0
		# 
		# create self.semCheckCtrRunningXor as already
		# acquired:
		self.semCheckCtrRunningXor = threading.Semaphore(0)
		
		self.host = host
		self.port = port
		self.maxXor  = maxXor
		if (maxXor <= 0):
			print "maxXor cannot be < 1. It is:", maxXor
			return (None)	# ABORT

		if (maxCycles <= 0):
			print "maxCycles reset from", maxCycles, "to 1."
			self.maxCycles = 1
		else:
			self.maxCycles = maxCycles
		if (delayStartNew < 0):
			print "delayStartNew reset from", delayStartNew, "to 0"
			self.delayStartNew = 0
		else:
			self.delayStartNew = delayStartNew

		self.b_said_setsockopt_RCVTIMEO    = 0
		self.b_said_setsockopt_TCP_NODELAY = 0

		print "ConnXorMonitor Instance:", self, "Starting..."
		self.createAllConnXor()
		print len(self.xorList), "ConnXor in list."
		print "Each ConnXor will do at most", maxCycles, "cycles."

	#
	# removed old random stuff that was here
	#

	def getAbortFlag(self):
		return (self.abortFlag)

	def setAbortFlagOn(self):
		self.abortFlag = 1

	def createAllConnXor(self):
		from random import Random		
		g = Random(None)		# use time as seed
		delta = 1000000

		for i in xrange(0, self.maxXor):
			print "Normal #", i
			self.createOneConnXor(g)	# g: a Random number generator
			laststate = g.getstate()			
			g = Random()			
			g.setstate(laststate)			
			g.jumpahead(delta	)

	def genConnXorId(self):
		return ("cx_" + repr(self.ctrRunningXor) )

	def createOneConnXor(self, g):
		self.ctrRunningXor = self.ctrRunningXor + 1
		cx = ConnXor(self, self.genConnXorId(), g)
		self.xorList.append(cx)

	def showAllConnXor(self, verbose=0):
		for xor in self.xorList:
			print xor

	def notifyConnXorThreadCompleting(self):
		#
		# use Semaphore synchronize threads access to shared 
		# counter before decrementing it:
		#
		self.semCtrRunningXor.acquire()		# blocking mode
		self.ctrRunningXor = self.ctrRunningXor - 1
		self.semCtrRunningXor.release()
		# 
		# release self.semCheckCtrRunningXor to notify
		# the main thread and let it check the counter.
		# The main thread is startAllConnXor().
		#
		self.semCheckCtrRunningXor.release()

	def waitForAllConnXor(self, verbose=0):
		if (verbose):
			msg = "threads to complete..."
		else:
			msg = "threads..."

		while (self.ctrRunningXor >= 1):
			print "waiting for", self.ctrRunningXor, msg
			self.semCheckCtrRunningXor.acquire()

	def startAllConnXor(self):

		t_begin = time.time()		# reference time of beginning of test

		try:
			# self.ctrRunningXor will be use to monitor
			# threads execution completion:
			#
			self.ctrRunningXor = len(self.xorList)

			for xor in self.xorList:
				t = xor.start()
### ###				time.sleep(self.delayStartNew)
			print self.ctrRunningXor, "ConnXor threads started."
			
			# Wait for all threads to finish...
			#
			self.waitForAllConnXor(1) # verbose=1
		except KeyboardInterrupt:
			self.abortFlag = 1
			print "CTRL-C received", self.ctrRunningXor, \
				"threads still running."

		# try to wait for the remaining threads,
		# but, do not trap CTRL-C :
		# 
		self.waitForAllConnXor(0)

		totalAvg = 0.0		
		for xor in self.xorList:
			if (xor.nbIter <= 0):
				avg = 0
			else:
				avg = xor.totalWait / xor.nbIter
			totalAvg = totalAvg + avg

		avgAvg = (totalAvg / len(self.xorList) ) * 1000.0	# convert to ms

		if (self.abortFlag):
			spName = "Aborted? " + self.sp_name
		else:
			spName = self.sp_name

		m1 = "%s global avg: %5.1f ms (%s xor, %s iter)" % (spName, avgAvg, self.maxXor, self.maxCycles)

		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		self.writeReport(t_begin)


	def writeReport(self, t_begin):
#		. avg wait between RPC calls (per thread)
#		. ctr failed/success
#		. TPM target
#		. TPM achieved

		t_finish = time.time()
		delta_elapsed = t_finish - t_begin

		nbXor = len(self.xorList)

		tot_done   = 0.0
		tot_OK     = 0.0
		tot_failed = 0.0

		tot_avg_wait   = 0.0
		tot_avg_OK     = 0.0
		tot_avg_failed = 0.0

		tot_elapsed	= 0.0
		tot_nbIter	= 0.0

		for xor in self.xorList:
			if (not xor.elapsed):		### @@@ to be checked-in !!!
				continue;
			tot_nbIter  = tot_nbIter + xor.nbIter
			tot_elapsed = tot_elapsed + xor.elapsed

			tot_done   = tot_done   + xor.statsSleep.getN()
			tot_OK     = tot_OK     + xor.statsOK.getN()
			tot_failed = tot_failed + xor.statsFailed.getN()

			tot_avg_wait   = tot_avg_wait   + xor.statsSleep.getAverage()
			tot_avg_OK     = tot_avg_OK     + xor.statsOK.getAverage()
			tot_avg_failed = tot_avg_failed + xor.statsFailed.getAverage() 
#@@@@@
		m1 = "%s RPC sent: %s OK, %s failed." % (tot_done, tot_OK, tot_failed)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		avg_wait_ms = (tot_avg_wait  / nbXor ) * 1000.0	# convert to ms
		avg_OK_ms   = (tot_avg_OK    / nbXor ) * 1000.0	# convert to ms
		avg_failed = (tot_avg_failed/ nbXor ) * 1000.0	# convert to ms
		interArrival = avg_wait_ms / nbXor

		m1 = "averages: sleep: %6.1f ms,  inter-arrival: %6.1f ms,  OK: %6.2f ms,  failed: %6.2f ms" % (avg_wait_ms, interArrival, avg_OK_ms, avg_failed)

		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		avg_wait   = (tot_avg_wait  / nbXor )
		avg_OK     = (tot_avg_OK    / nbXor )
		if (avg_wait):
			target_TPM = (60 / avg_wait)
		else:
			target_TPM = 0
		if (avg_OK):
			theoretical_TPM = (60 / avg_OK)
		else:
			theoretical_TPM = 0

		observed_TPM = (tot_nbIter / delta_elapsed) * 60.0

		avg_elapsed = (tot_elapsed / nbXor)

		m1 = "target TPM  : %6.1f TPM  (if response time was zero 0 ms)." % (target_TPM, )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		m1 = "theoretical : %6.1f TPM (based on avg OK only)." % (theoretical_TPM, )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		m1 = "observed TPM: %6.1f TPM  ((tot_nbIter %6.1f / delta_elapsed %6.1f) * 60.0)." % (observed_TPM, tot_nbIter, delta_elapsed)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "****************")

		# write .csv file, with these columns:
		#  timestamp, port, nbXor, maxCycles, maxSleepSeconds, 
		#  tot_done, tot_OK, tot_failed,
		#  avg_wait_ms, interArrival, avg_OK_ms, avg_failed,
		#	 target_TPM, theoretical_TPM,
		#  observed_TPM, tot_nbIter, delta_elapsed, avg_elapsed,
		#  sp_name
		#
		m1 = "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, \n" % (TxLog.get_log_time(), self.port, nbXor, self.maxCycles, self.maxSleepSeconds, tot_done, tot_OK, tot_failed, 
			avg_wait_ms, interArrival, avg_OK_ms, avg_failed, target_TPM, theoretical_TPM, observed_TPM, tot_nbIter, delta_elapsed, avg_elapsed, self.sp_name)
		self.writeCSVline(m1)

	def writeCSVheader(self):
		m1 = "timestamp, port, nbXor, axCycles, maxSleepSeconds, tot_done, tot_OK, tot_failed, avg_wait_ms, interArrival, avg_OK_ms, avg_failed, target_TPM, theoretical_TPM, observed_TPM, tot_nbIter, delta_elapsed, avg_elapsed, sp_name,\n"
		self.writeCSVline(m1)

	def writeCSVline(self, str):
		f = open(REPORT_FILENAME, "a")
		f.write(str)
		f.close()


	def getHost(self):
		return (self.host)

	def getPort(self):
		return (port)

	def getText_LOGIN(self, userid=None):
		was_empty = (self.text == None)
		if (userid == None):
			userid = self.userid
		# 
		# WARNING: next string is very long...
		#
		a  = '<?xml version="1.0" encoding="ISO-8859-1" ?><procCall><proc>'
		a2 ='login</proc> '
		b  = '<param>  <name>@p_userid</name> <str>'
		b2 = userid + '</str> <attr>0012</attr>  </param> '
		c  = '<param> <name>@p_password</name> <str>'
		c2 = self.password + '</str> <attr>0012</attr> </param></procCall><EOT/>'
		self.text = a + a2 + b + b2+ c + c2

		if (was_empty):
			print "login XML text is:", self.text

		return (self.text + "\n")


	def getText_1(self, procName):
		if (self.text_1 == None):
			# 
			# WARNING: next string is very long...
			#
			a = '<?xml version="1.0" encoding="ISO-8859-1" ?><procCall><proc>'
			a2 = '%s</proc> '
			b = '<param>  <name>__return_status__</name> <int>-9999</int> <attr>114</attr> </param> %s'
			self.text_1 = a + a2 + b
		return ( self.text_1 % (procName, "") )
## 		return ( self.text_1 % (procName, "\n\n") )


	def getText_2(self):
		if (self.text_2 == None):
			if (self.param1):
				c = '<param> <name>@p_01</name> <str>%s</str> <attr>0012</attr> </param></procCall><EOT/>' % (self.param1, )
			else:
				c = '<param> <name>@p_dbName</name>  <str>Master</str> <attr>0012</attr> </param></procCall><EOT/>'
			self.text_2 = c
###		return (self.text_2)
		return (self.text_2 + "\n\n")


	def getMaxCycles(self):
		return (self.maxCycles)




class ConnXor(threading.Thread):
	def __init__(self, monitor, id, g):
		threading.Thread.__init__(self)	# essential
		self.monitor = monitor
		self.id = id
		self.g  = g
		self.statsSleep  = gxqueue.gxStats()
		self.statsOK     = gxqueue.gxStats()
		self.statsFailed = gxqueue.gxStats()
		self.elapsed	 = None


		self.sock = None
		self.currentAction = ACTION_INIT
		self.resetCounters()
		self.initParams()
		self.nbIter = 0
		self.totalWait = 0.0
		self.debugLevel = 1
	
	def __repr__(self):
		id  = repr(self.getId() )
		ncy = repr(self.nCycles)
		m   = repr(self.maxCycles)
		act = self.describeCurrentAction()
		fd  = repr(self.getFd() )
		cbs = repr(self.currBytesSent)

		return ("<" + id  + ":" + ncy + "/" + m + "," + act + 
			", fd=" + fd + 
		###	", " + cbs + "oct. " + 
			">")
	def getId(self):
		return (self.id)

	def getFd(self):
		if (self.sock == None):
			return (-1)
		return (self.sock.fileno() )

	def describeCycleType(self):
		return "Normal"

	def describeAction(self, action):
		if (action == ACTION_INIT):
			return ("INIT")
		if (action == ACTION_WAIT):
			return ("WAIT")
		if (action == ACTION_RECV):
			return ("RECV")
		if (action == ACTION_SEND):
			return ("SEND")
		if (action == ACTION_CLOSE):
			return ("CLOSE")
		if (action == ACTION_CONNECT):
			return ("CONNECT")
		if (action == ACTION_NEW_SOCKET):
			return ("NEW_SOCKET")
		if (action == ACTION_SHUTDOWN):
			return ("SHUTDOWN")
		if (action == ACTION_COMPLETED):
			return ("COMPLETED")
		return ("[unknown=" + repr(action) + "]")

	def describeCurrentAction(self):
		return (self.describeAction(self.currentAction) )

	def getDebugLevel(self):
		return (self.debugLevel)

	def resetCounters(self):
		self.nCycles = 0
		self.nConnOpened = 0
		self.nRecvTimeouts = 0
		self.nSendTimeouts = 0
		self.nRecv = 0
		self.nSend = 0
		self.currBytesSent = 0  # used by Markov

	def initParams(self):
		self.currentAction = ACTION_INIT
		self.maxCycles = self.monitor.getMaxCycles()
		self.avgWait   = 1
		self.recvRetry   = 1
		self.retryWait   = 1
		self.ioChunkSize  = 0
		return (0)

	def getText_LOGIN(self, userid=None):
		return (self.monitor.getText_LOGIN(userid) )

	def getText(self):
		return (self.monitor.getText() )

	def getText_1(self):
		return (self.monitor.getText_1() )

	def getText_2(self):
		return (self.monitor.getText_2() )

	def closeSocketIfOpen(self, doNotClosePrevious=0):
		if (self.getFd() >= 0):
			if (doNotClosePrevious):
				self.monitor.rememberOrphanSocket(self.sock)
				return	# Markov cycle can do this
			self.currentAction = ACTION_SHUTDOWN
			#
			print self.getId(), "self.sock.shutdown(2)..."
			self.sock.shutdown(2)

	def connect(self, doNotClosePrevious=0):
		self.currBytesSent = 0	# this reset is important for Markov
		self.closeSocketIfOpen(doNotClosePrevious)
		self.currentAction = ACTION_NEW_SOCKET
		self.sock = socket.socket(socket.AF_INET,
					 socket.SOCK_STREAM)
		self.currentAction = ACTION_CONNECT

		# self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, self.keepAlive)

		timeout_sec = 210
		timeout_val = struct.pack("ll", timeout_sec, 0)

		self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout_val)

		if (not self.monitor.b_said_setsockopt_RCVTIMEO):
			self.monitor.b_said_setsockopt_RCVTIMEO = 1
			m1 = "setsockopt(...SO_RCVTIMEO, %s) OK" % (timeout_sec, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		self.sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)


		if (not self.monitor.b_said_setsockopt_TCP_NODELAY):
			self.monitor.b_said_setsockopt_TCP_NODELAY = 1
			m1 = "setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)... DONE"
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		h = self.monitor.getHost()
		p = self.monitor.getPort()

		#print "h:", h, "p:", p, self
		# self.sock.connect( h, p )
		try:
			if (self.getDebugLevel() >= 2):
				m1 = "about to connect(%s, %s)..." % (h, p)
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
				# print m1
			#
			# if (0):
			#	self.sock.connect( (h, p) )
			#	return (0)
			#print "about to connect() self.getDebugLevel()", self.getDebugLevel()

			self.sock.connect( (h, p) )

			# rc = self.sock.connect_ex( (h, p) )	# always rc=0 on NT4 with Py 2.0 !!
			#
			# print "after connect(), rc:", rc

			# Removed all kind of crap here, checking rc, etc.
		except:
			exc = gxutils.formatExceptionInfoAsString()

			m1 = "connect(%s, %s): FAILED %s (%s)" % (h, p, exc, self)
			srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)

			if (self.getDebugLevel() >= 4):
				print "connect() Failed. Exception:", exc, self
			return (-1)

		self.doLogin()

		return (0)

	def doLogin(self):
		if (self.monitor.userid == "!"):
			userid = self.id
		else:
			userid = None
		text_1 = self.getText_LOGIN(userid)

		self.sendRPCandReceiveResponse(text_1, None, verbose=0)
		#
		# There is no check of the return status but the next send()
		# will fail if the login was rejected (as the connection 
		# would be broken after the login is rejected).
		#

	def displayWhyThreadStops(self, exc, val=None):
		print "ConnXor", self.id, "stopping.", \
			self.describeCurrentAction(), \
			"err:", exc, val, ".sock:", self.sock

	def run(self):
		print self, "starting..."
		self.sock = None
		try:
			if (self.connect() != 0):
				self.displayWhyThreadStops("connect() failed", "Aborting thread.")
				self.monitor.notifyConnXorThreadCompleting()
				return (-1)

			m1 = "%s iterations starting (%s)" % (self.maxCycles, self)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			tnow = time.time()

			for i in xrange(0, self.maxCycles):
				self.nCycles = self.nCycles + 1
				if (self.monitor.getAbortFlag()):
					break
				self.oneCycle(self.monitor.debugLevel)

			delta = (time.time() - tnow)

			if (self.nbIter <= 0):
				avg = 0
			else:
				avg = self.totalWait / self.nbIter

			self.elapsed = delta

			m1 = "%s iter DONE avg response: %5.4f s, total elapsed %4.3f seconds (%s)" % (self.nbIter, avg, delta, self)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			self.closeSocketIfOpen()

			if (self.getDebugLevel() >= 5):
				m1 = "socket CLOSED (%s)" % (self, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		except:
			exc = gxutils.formatExceptionInfoAsString()
			self.displayWhyThreadStops("Exc:", exc)
		else:
			if (self.monitor.getAbortFlag()):
				msg = "Abort requested"
			else:
				msg = "completed"
			print "ConnXor", self.id, msg, \
				"(", self.describeCurrentAction(), ")."
		# 
		# this thread will end. Notify the monitor:
		#
		self.monitor.notifyConnXorThreadCompleting()

	def oneCycle(self, verbose=0):
		self.oneNormalDialogCycle(verbose)


	def oneNormalDialogCycle(self, verbose=0):
		text_1 = self.monitor.getText_1(self.monitor.sp_name)
		text_2 = self.getText_2()

		self.sendRPCandReceiveResponse(text_1, text_2, verbose)


	def getRandomNbSec(self):
		nbSec = self.g.random() * self.monitor.maxSleepSeconds
		return nbSec


	def sendRPCandReceiveResponse(self, text_1, text_2, verbose=0):
		#
		# CALLED BY:	doLogin()
		#		oneNormalDialogCycle()
		#
		# Apology: there is all kind of old code still in this function... 
		#	   Please just ignore it.
		#
		self.currentAction = ACTION_SEND
		if (verbose >= 5):	print self

		# use self.getId() as a parameter to see if it comes
		# back in the response...
		#
		# param_1 = self.getId()     # unused

		
		n = self.getRandomNbSec()

		self.statsSleep.addToStats(n)

		if (self.getDebugLevel() >= 5):
			m1 = "sleep(%s) seconds before send()...  %s" % (n, self )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		if (n > 0):
			time.sleep(n)

		if (text_2):
			all_text = text_1 + text_2
		else:
			all_text = text_1

		if (self.getDebugLevel() >= 5):
			print "Sending: '", all_text, "'."

		t_before = time.time()		### moved before sock.send() -- 2002mar27

		self.sock.send(all_text)

		self.currentAction = ACTION_RECV

		if (verbose >= 5):	print self

		self.b_idFound = 0
		self.recvStatus = ""
		self.nbIter = self.nbIter + 1

		### t_before = time.time()

		ret_stat = self.receiveResponse()

		t_done = time.time()
		delta = t_done - t_before
		self.totalWait = self.totalWait + delta

		if (ret_stat == 0):
			self.statsOK.addToStats(delta)
		else:
			self.statsFailed.addToStats(delta)

		if (1 == 0):
			if (self.b_idFound):
				r = "id found OK"
				logLevel = TxLog.L_MSG
			else:
				r = "id NOT found!"
				logLevel = TxLog.L_WARNING
		else:
			r = ""
			logLevel = TxLog.L_MSG

		if (self.getDebugLevel() >= 10 or logLevel != TxLog.L_MSG):
			m1 = "receiveResponse(): ret=%s, %s. %5.3f sec %s %s" % (ret_stat, r, delta, self.recvStatus, self)
			srvlog.writeLog(MODULE_NAME, logLevel, 0, m1)

	def receiveResponse(self):
		if (self.getDebugLevel() >= 7):
			m1 = "receiveResponse()...  %s" % (self, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		self.b_idFound = 0
		b_eot = 0
		prev_buff = ""
		ret_stat  = -995

		while (not b_eot):
			try:
				rbuff = self.sock.recv(5000)
				blen = len(rbuff)
				if (blen <= 0):
					self.recvStatus = "recv() ERROR"
					return -991

			except:
				exc = gxutils.formatExceptionInfoAsString()
				m1 = "recv(): FAILED %s (%s)" % (exc, self)
				srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)
				self.recvStatus = "recv() Failed. Exception: %s" % (exc, )
				return -992

			# concat previous and current buffers to make simple it to 
			# check if <EOT/> was received:
			#
			str = prev_buff + rbuff

			if (b_printBuffer):
				print self.getId(), "buffer: '", str, "'."

			## if (string.find(str, self.getId() ) >= 0):
			##	self.b_idFound = 1

			b_eot, ret_stat = self.checkEOTandReturnStatus(XMLTP_EOT_TAG, str)

			if (b_eot):
				self.recvStatus = XMLTP_EOT_TAG + " DONE."

			len_str = len(str)

			if (len_str < 72):		# str is very small, keep it all.
				prev_buff = str
				continue

			# keep the ending of the previous buffer in prev_buff
			# in case "<EOT/>" is partially in the next and previous
			# buffers...
			# Or maybe <returnStatus>...</returnStatus> is split between
			# those 2 buffers...
			#
			prev_buff = str[(len_str - 70):]
		#
		# End of loop

		return ret_stat



	def checkEOTandReturnStatus(self, EOT_tag, str):
		#
		# Called by:	.receiveResponse()
		#		--- 	adapted from gxrpcmonitors.py, class gxRPCmonitor.receiveResponseAndSendBackToClient()
		#
		# First try to find <EOT/>.
		# if not found, 
		#	return (0, -999)
		# else:
		#	grab the return status in the last chunk, str.
		#	    ...<returnstatus> <int>-12345</int> </returnstatus>
		#	    </response><EOT/>
		#
		#	NOTE: this parsing is done with heuristics, not formal
		#		syntax parsing. But, it should be good enough.
		#
		#	return (1, ret_stat)

		if (self.getDebugLevel() >= 99):
			m1 = "checkEOTandReturnStatus(): looking for %s in '%s'."  % (EOT_tag, str)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, len(str), m1)

		b_eot    = 0
		ret_stat = -999

		if (str.find(EOT_tag) >= 0):
			b_eot = 1
		else:
			return (b_eot, ret_stat)	# EOT not received yet

		# <EOT/> found, we will check for <returnstatus> now:
		#
		tag = "<" + xmltp_tags.XMLTP_TAG_RETURNSTATUS + ">"
		idx = str.find(tag)
		if (idx < 0):
			if (getDebugLevel() >= 99):
				ret_stat = -995

			return (b_eot, ret_stat)	# <returnstatus> not found

		idx = idx + len(tag)
		s2  = str[idx:]		# jump over that tag...

		tag = "<"  + xmltp_tags.XMLTP_TAG_INT + ">"

		idx = s2.find(tag)
		if (idx < 0):
			if (getDebugLevel() >= 99):
				ret_stat = -994

			return (b_eot, ret_stat)	# <int> not found

		idx = idx + len(tag)
		s2 = s2[idx:]		# jump over that tag...

		tag = "</" + xmltp_tags.XMLTP_TAG_INT + ">"	# closing tag

		idx = s2.find(tag)
		if (idx < 0):
			if (getDebugLevel() >= 99):
				ret_stat = -993

			return (b_eot, ret_stat)	# </int> not found

		s2 = s2[:idx]		# truncate s2 before </int>
		try:
			ret_stat = int(s2)
		except ValueError:
			ret_stat = -998

		return (b_eot, ret_stat)


if __name__ == '__main__':
	if (len(sys.argv) < 6):
		print "Usage:   xml_load.py  port   nbConn nbCycles sp_name [maxSleepSeconds]"
		print "example: xml_load.py   7777    1      10      sp_who  2.0" 
		raise SystemExit

	port = string.atoi(sys.argv[1])
	nbConn = string.atoi(sys.argv[2])
	nbCycles = string.atoi(sys.argv[3])
	sp_name = sys.argv[4]
	if (len(sys.argv) >= 6):
		maxSleepSeconds = string.atof(sys.argv[5])
	else:
		maxSleepSeconds = 2.0

	if (maxSleepSeconds <= 0.0):
		maxSleepSeconds = 0.1
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "maxSleepSeconds corrected to 0.1 sec")

	m1 = "Send to port %d, %d conn, %d cycles, maxSleepSeconds=%s" % (port, nbConn, nbCycles, maxSleepSeconds)
	srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	m1 = "This will generate about %s requests per second." % ((nbConn / (maxSleepSeconds * 0.5) ), )
	srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	#
	# ConnXorMonitor(host, port, maxXor, maxCycles, delayStartNew)
	#
	# host = "142.225.38.1"
	host = "127.0.0.1"
	# host = "naga08"

	cxMon = ConnXorMonitor(host, port, nbConn, nbCycles, 1, sp_name)

	cxMon.debugLevel = 2

	cxMon.maxSleepSeconds = maxSleepSeconds

	#
	# PUT YOUR OWN userid and password for "login" just below here:
	# 
	cxMon.userid = "oper"
	#cxMon.userid = "!"	# use cx id as userid
	cxMon.password = "oper"


	if (len(sys.argv) > 6):
		if (sys.argv[6] == "header" or sys.argv[6] == "HEADER"):
			cxMon.writeCSVheader()
			if (len(sys.argv) > 7):
				cxMon.param1 = sys.argv[7]
		else:
			cxMon.param1 = sys.argv[6]

	cxMon.showAllConnXor(1)	# 1=verbose

	cxMon.startAllConnXor()

	print "Completed??"

	cxMon.showAllConnXor(1)	# show statistics... (1=verbose)

	m1 = "Settings were: port %s, %d conn, %d cycles, maxSleepSeconds=%s" % (port, nbConn, nbCycles, maxSleepSeconds)
	srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "--------------------")


