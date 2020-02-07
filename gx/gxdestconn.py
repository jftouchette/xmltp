#!/usr/bin/python

#
# gxdestconn.py :  Gateway-XML - Destination Connections Pool
# -----------------------------------------------------------
#
#
# $Source: /ext_hd/cvs/gx/gxdestconn.py,v $
# $Header: /ext_hd/cvs/gx/gxdestconn.py,v 1.4 2006/02/02 14:53:14 toucheje Exp $
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
# -----------------------------------------------------------------------
# 2002feb02,jft: + extracted from gxserver.py
# 2002feb25,jft: . DestConnPool.createNewResource(): do xmltp_gx.createContext() and store that context in the resource
# 2002mar08,jft: - removed DestConnPool.getDestConnCreationArgsList()
# 2002mar26,jft: . DestConnPool.createSocketAndConnect(): self.sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
# 2002apr15,jft: + DestConnPool.shutdownResources() close all DestConn sockets 
# 2002jun08,jft: . DestConn.resetConn(): reset to normal, not "dirty", state
# 2002jun29,jft; . DestConnPool.createSocketAndConnect(): re-enabled call to sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDTIMEO, timeout_val)
# 2002aug12,jft: . DestConn.loginToServer(self): uses self.reentry_ctr to avoid being called recursively.
# 2002aug14,jft: + support for "clean <dest>" operator command ...
#		 + DestConnPool.doOperation(self, res, b_newCreate)
# 2002sep26,jft: . DestConn.loginToServer(): allow self.reentry_ctr <= 2
# 2002oct02,jft: . DestConnPool.createNewResource(): does NOT call self.createSocketAndConnect() anymore. See NOTE in comments.
#		 + DestConnPool.queueTestRPCtoConnect(self)
# 2002oct13,jft: + DestConn.assignOutcomeOfUse(): override superclass method, close socket if b_dirty is true
# 2002oct20,jft: + DestConnPool.queueAsynchRPCextended(self, procName, params, pseudoUserid, pseudoMsg, b_privReq=0)
#		 + DestConn.queueRPCtoCleanUpPeerResource(self)
#		 . DestConn.loginToServer(): updates self.remoteResourceId when login successfull
#		 . DestConn.assignOutcomeOfUse(): if b_dirty, and (not self.b_processingPrivRequest): call .queueRPCtoCleanUpPeerResource()
# 2002oct26,jft: . DestConn.queueRPCtoCleanUpPeerResource(): pass arg sleepBeforeCall=3 in call to self.pool.queueAsynchRPCextended()
# 2002nov27,jft: . DestConn.loginToServer(): dynamically read password from self.pool.passwordFile
#		 . DestConnPool.createSocketAndConnect(): calls xmltp_hosts.getServerHostAndPort(srvName)
# 2003feb08,jft: . DestConn.loginToServer(): if (self.pool.getDebugLevel() == 12): srvName = TxLog.get_thread_id()
# 2003jun11,jft: . DestConn.assignOutcomeOfUse(): only queue "kill" RPC once, and only if (b_kill)
# 2004aug07,jft: . DestConnPool.queueAsynchRPCextended(): added arg interceptor=0 in call to gxrpcmonitors.queueProcCallToDestPool()
# 2006feb01,jft: . DestConnPool.createSocketAndConnect(): updates self.destSrvName, self.destHostnamePort
#		 + DestConnPool.assignCurrentRoute(): assign self.currentRoute
#		 + DestConnPool.getStatusAsRow() and getStatusAsRowHeaders() override methods with same name in gxrespool.py
#		 + DestConnPool.__repr__() overrides method with same name in gxrespool.py
#		 . DestConnPool.shutdownResources(): if (ctrDone + 1) >= self.maxRes:	msg = "[close() completed]" else: msg = "[PARTIAL close()]"
#

import socket
import threading
import struct

import gxrpcmonitors
import gxrpcframe
import gxintercept	# for .convertToPythonTypeIfNotString()
import gxclientconn	# for class TempPseudoClientConn
import gxrespool
import gxthread
import gxutils
import xmltp_hosts	# for .getServerHostAndPort(srvName)
import TxLog

import xmltp_gx		# for xmltp_gx.createContext()


#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxdestconn.py,v 1.4 2006/02/02 14:53:14 toucheje Exp $"
MODULE_NAME = "gxdestconn.py" 

#
# module variables:
#
srvlog = None

debug_level = 0


#
# CONSTANTS:
#
DESTCONNPOOL_POOLTYPE	= "DestConnPool"

OPER_CLEAN		= "Clean"

#
# CONSTANTS for DestConn.state :
#
DESTCONN_INIT		     = 0
DESTCONN_RESET_NOT_CONNECT   = 1
DESTCONN_CONNECTED_NOT_LOGIN = 2
DESTCONN_LOGIN_IN_PROGRESS   = 3
DESTCONN_LOGIN_OK	     = 5
DESTCONN_CLOSED		     = 90
DESTCONN_CONNECT_FAILED	     = 91
DESTCONN_LOGIN_FAILED	     = 92
DESTCONN_DISCONNECTED	     = 99	# disconnected by peer
DESTCONN_ERROR_PEER_STATE_UNKNOWN = 999



class DestConn(gxrespool.gxresource):
	def processRequest(self, rpcFrame):
		threadIdSuffix = repr(self.pool.incrementGetThreadsCtr() )

		t = gxthread.gxthread( self.name + "Th" + threadIdSuffix,
					monitor  = rpcFrame.getMonitor(),
					resource = self,
					frame    = rpcFrame,
					pool     = self.pool,
					threadType=gxthread.THREADTYPE_RPC )

		rpcFrame.assignDestconn(self)

		self.assignUsedByThread(t)

		self.assignMonitor(rpcFrame.getMonitor() )

		t.start()


	def getSocket(self):
		return (self.data)


	def assignOutcomeOfUse(self, b_success=1, b_dirty=0, b_kill=0):
		#
		# NOTE: override superclass method (!)
		#
		# Called by:	gxrpcmonitors.py
		#
		self.b_outcomeOfUseSuccessful = b_success

		prev_b_dirty = self.b_dirty
		self.b_dirty = b_dirty

		if (b_dirty):
			if (b_kill and not prev_b_dirty):	# only queue "kill" RPC once
				if (not self.b_processingPrivRequest):		# avoid infinite loop
					self.queueRPCtoCleanUpPeerResource()
			self.closeConn()	# only way to tell xml2syb to stop long SP

	def queueRPCtoCleanUpPeerResource(self):
		#
		# Queue a RPC that will clean up (zap!) the peer
		# resource (a program which implement the XML2DB specs,
		# probably).
		#
		procName = "kill"
		try:
			poolName = self.pool.getName()
			if (self.pool.getDebugLevel() >= 10):
				m1 = "queueRPCtoCleanUpPeerResource(): '%s', will send proc: %s, pid = %s." % (poolName, procName, self.remoteResourceId)
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			
			if (not self.remoteResourceId):
				return None

			params = [ ('@p_pid', self.remoteResourceId, 4, 0, 0), ]

			pseudoUserid = "%s_%s_%s" % (poolName, procName, self.remoteResourceId)
			pseudoMsg = "To stop runaway xml2db process"

			# NOTE: we queue this RPC on the priviledged requests queue:
			#
			self.pool.queueAsynchRPCextended(procName, params, pseudoUserid, pseudoMsg, b_privReq=1, sleepBeforeCall=3)
		except:
			es = gxutils.formatExceptionInfoAsString(3)
			m1 = "queueRPCtoCleanUpPeerResource(): failed with: %s" % ( es, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)


	def verifyDestConn(self, b_reset=0):
		#
		# Called by:	gxRPCmonitor.runThread() (with b_reset=0)
		#		[gxxmlwriter]sendProcCallWithRetry() with b_reset=1
		#
		# First, check if there is a socket connection,
		# and if our state is DESTCONN_LOGIN_OK.
		# If so, 
		#	try to recv() from that socket (non-blocking)
		#	this should return (0) bytes.
		#	if so,
		#		return (0), meaning everything looks OK.
		#
		# Otherwise, connect (if not already connected) and
		# login to the destination (the server).
		#
		if (b_reset  or  self.b_dirty):
			if (self.pool.getDebugLevel() >= 1):
				m1 = "reset DestConn %s..." % (self, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			self.resetConn()

		if (self.data != None and self.getState() == DESTCONN_LOGIN_OK):
			rc = self.drainConnection(1)	# b_mustBeEmpty=1
			if (rc == 0):
				return (0)	# connection is good.

			if (self.pool.getDebugLevel() >= 10):
				m1 = "drainConnection() returned %s for DestConn %s." % (rc, self, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		if (self.data == None or (self.getState() != DESTCONN_CONNECTED_NOT_LOGIN 
		    and self.getState() !=  DESTCONN_LOGIN_FAILED) ):
			if (self.pool.getDebugLevel() >= 10):
				if (self.data): 
					st = self.getState()
				else:
					st = "[data==None]"
				m1 = "verifyDestConn(): state=%s, DestConn=%s." % (st, self, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			self.resetConn()
			if (self.data == None):
				return (-2)	# cannot connect to server

		rc = self.loginToServer()

		return (rc)



	def loginToServer(self):
		# Called by:	verifyDestConn()
		#
		# NOTE: this function uses self.reentry_ctr to avoid being called recursively!
		#
		# Send a ProcCall like:
		#   	RP_DB_LOGIN @p_username = "user01", @p_password = "xyz", @p_trg_name = "OSDETR80"
		#

		if (self.data == None or (self.getState() != DESTCONN_CONNECTED_NOT_LOGIN 
		    and self.getState() !=  DESTCONN_LOGIN_FAILED) ):
			m1 = "loginToServer() cannot be called when state=%s, %s" % (self.getState(), self)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
			return (-9)

		try:
			# check if we are called recursively:
			#
			if (self.reentry_ctr >= 2):
				if (self.pool.getDebugLevel() >= 1):
					m1 = "loginToServer(): avoiding recursion. DestConn %s." % (self, )
					srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
				return (-8)

			self.remoteResourceId = None

			self.reentry_ctr = self.reentry_ctr + 1
				
			if (self.pool.getDebugLevel() >= 10):
				m1 = "loginToServer() by DestConn %s. self.reentry_ctr=%s" % (self, self.reentry_ctr)
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			mon = gxrpcmonitors.gxDestConnLoginMonitor("DestConnLoginMonitor")

			frame = gxrpcframe.gxRPCframe(None)	# create frame _without_ client conn

			frame.assignMonitor(mon)
			frame.assignDestconn(self)	# bind DestConn to frame

			procName = self.pool.loginProc
			userid   = self.pool.userid
			passwd   = self.pool.master.readFirstLineOfFile(self.pool.passwordFile)

			if (self.pool.getDebugLevel() == 12):		# special debug level to put our thread id in srvName
				srvName = TxLog.get_thread_id()
			else:
				srvName = self.pool.origSrvName		# the origin-Server-Name

			procCall = (procName, [ ('@p_username', userid, 12, 0, 0),
					        ('@p_password', passwd, 12, 0, 0),
					        ('@p_trg_name', srvName, 12,0, 0) ], 0)

			frame.assignProccall(procCall)

			tempInfoKey = "resultSets"
			val = [(0, tempInfoKey), ]

			frame.storeTempInfo(gxrpcmonitors.TEMP_INFO_LIST_RESP_PARTS_TO_GRAB, val)

			self.assignState(DESTCONN_LOGIN_IN_PROGRESS)

			mon.doLogin(threading.currentThread(), frame)	# could trigger recursion if it gets a disconnect

			if (frame.getReturnStatus() == 0):
				pid = self.getPidOfXml2dbFromTempInfo(frame, tempInfoKey)

				self.remoteResourceId = pid	  # pid of the "XML2DB" process

				self.assignState(DESTCONN_LOGIN_OK)
				self.reentry_ctr = 0		# release recursion avoidance flag
				return (0)	# OK, success
			else:
				self.assignState(DESTCONN_LOGIN_FAILED)
		except:
			es = gxutils.formatExceptionInfoAsString()
			m1 = "loginToServer() on %s: Exception: %s" % (self, es)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

			self.assignState(DESTCONN_LOGIN_FAILED)
			self.reentry_ctr = 0		# release recursion avoidance flag
			return (-5)

		self.reentry_ctr = 0		# release recursion avoidance flag
		return (-1)	# login failed

	def getPidOfXml2dbFromTempInfo(self, frame, tempInfoKey):
		#
		# Called by:	loginToServer()
		#
		if (self.pool.getDebugLevel() >= 20):
			m1 = "getPidOfXml2dbFromTempInfo(): tempInfoKey=%s, frame=%s." % (tempInfoKey, frame)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		if (not frame or not tempInfoKey):
			return None

		ls = frame.getTempInfo(tempInfoKey)
		if (not ls):
			return None

		if (self.pool.getDebugLevel() >= 10):
			m1 = "loginToServer() trying to get self.remoteResourceId from %s: %s." % (tempInfoKey, ls, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		# The "remoteResourceId" is in the first column of the first row of the first result set...
		# We go abruptly, but, we protect ourself with a try: except: construct.
		#
		try:
			cols, rows, nbRows = ls[0]	# first result set
			name, jType, sz = cols[0]	# first column definition
			row1 = rows[0]			# first row
			val, isNull = row1[0]		# first value in first row
		except:
			es = gxutils.formatExceptionInfoAsString(2)
			m1 = "getPidOfXml2dbFromTempInfo() extracting result set data failed: Exception: %s" % (self, es)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

		## recipe for "remoteResourceId" in outputParams:
		##
		##	tup = ls[0]	# there should only (1) output param. It is this one.
		##	name, val, jType, isNull, isOutput, sz  = tup

		# NOTE: gxintercept.convertToPythonTypeIfNotString() returns None if val 
		#	is a string.
		#
		newValue = gxintercept.convertToPythonTypeIfNotString(val, jType, isNull)

		return newValue


	def assignBadState(self, b_veryBad=0):
		# Called by:	[gxxmlwriter]sendProcCallWithRetry()
		#		gxRPCmonitor.receiveResponseAndSendBackToClient()
		#		gxDestConnLoginMonitor.receiveResponse()
		#
		if (b_veryBad):
			self.assignState(DESTCONN_DISCONNECTED)
		else:
			self.assignState(DESTCONN_ERROR_PEER_STATE_UNKNOWN)


	def drainConnection(self, b_mustBeEmpty=0, b_verbose=0):
		if (self.data == None):
			self.assignState(DESTCONN_DISCONNECTED)
			return (-1)

		try:
			rc = 0
			self.data.setblocking(0)	# we want non-blocking recv() here
			has_data = 1

			if (self.pool.getDebugLevel() >= 10):
				m1 = "drainConnection() about to check if data available..." 
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			while (has_data):
				try:
					if (self.pool.getDebugLevel() >= 10):
						srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "drainConnection() about to recv()" )

					r_buff = self.data.recv(100)

					if (r_buff != None and r_buff != ""):
						if (b_mustBeEmpty):
							m1 = "WEIRD: data found on DestConn %s, data: %s." % (self, repr(r_buff))
							srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, len(r_buff), m1)

							self.closeConn()	# disconnect from server!
							rc = -3
						break
					else:
						has_data = 0
				except:
					has_data = 0
					if (b_verbose):
						excStr = gxutils.formatExceptionInfoAsString()
						m1 = "drainConnection(%s): recv() failed with: %s" % (self,excStr)
						srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
		except:
			excStr = gxutils.formatExceptionInfoAsString()
			m1 = "drainConnection(%s): failed with: %s" % (self, excStr)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

			self.closeConn()	# disconnect from server!
			rc = -2

		if (self.data != None):
			self.data.setblocking(1)	# restore blocking mode (normal mode)

		return (rc)



	def closeConn(self):
		#
		# Called by:	resetConn()
		#		drainConnection()
		#
		# shutdown the socket, close it:
		#
		self.remoteResourceId = None

		if (self.data != None):			# if socket connection exists,
			try:
				self.data.shutdown(2)	# then disconnect
				self.assignState(DESTCONN_CLOSED)
			except:
				pass

			try:
				self.data.close()
				self.assignState(DESTCONN_CLOSED)
			except:
				print "DestConn: self.data.close() failed" 

			self.data = None
		else:
			self.assignState(DESTCONN_CLOSED)

		return (0)



	def resetConn(self):
		self.closeConn()		# disconnect from server

		self.b_dirty = 0;		# reset to normal, not "dirty", state.

		self.assignState(DESTCONN_RESET_NOT_CONNECT)

		self.connectToDestination()	# (re) connect to server


	def connectToDestination(self):
		self.data = self.pool.createSocketAndConnect()

		if (self.data != None):
			self.assignState(DESTCONN_CONNECTED_NOT_LOGIN)
		else:
			self.assignState(DESTCONN_CONNECT_FAILED)

	def isSane(self):
		#
		# WARNING: Override the superclass method!
		#
		return (self.b_dirty >= 0)

	def mustBeReset(self):
		#
		# NOTE: not exactly the same meaning, nor actions as for (not self.isSane() )...
		#
		return (self.b_dirty != 0)


class DestConnPool(gxrespool.gxrespool):

	def getRcsVersionId(self):
		return (RCS_VERSION_ID)

	def getResPoolType(self):
		# this method allows to identify a type of pools when iterating
		# on the list of all pools.
		return DESTCONNPOOL_POOLTYPE

	def assignCurrentRoute(self, route):
		self.currentRoute = route	# routeProc() in gxregprocs validates the input
		self.destHostnamePort = "--"
		self.destSrvName      = "--"

	def __repr__(self):
		""" Override method with same name in gxrespool.py 
		    __repr__() here also shows self.currentRoute, self.destSrvName and self.destHostnamePort
		"""
		return "{%s: %s, res:%s/%s, requests:%s/%s, route=%s, srv=%s, host,port=%s, reqQthread=%s} " % (self.name, 
				self.getEnableStatus(),
				self.resourcesQueue.qsize(), self.maxRes,
				self.requestsQueue.qsize(),  self.maxRequests,
				self.currentRoute, self.destSrvName, str(self.destHostnamePort),
				self.reqQueueThread)

	def getStatusAsRow(self):
		""" Override method with same name in gxrespool.py 
		    getStatusAsRow() here return self.currentRoute, self.destSrvName and self.destHostnamePort in the list, but, not self.exceptionMsg
		"""
		status = self.getEnableStatus()

		row = [ self.name, status,
			self.resourcesQueue.qsize(), self.maxRes,
			self.requestsQueue.qsize(),  self.maxRequests,
			self.currentRoute, self.destSrvName, str(self.destHostnamePort),
		      ]

		return row

	def getStatusAsRowHeaders(self):
		""" Override method with same name in gxrespool.py 
		    getStatusAsRowHeaders() here return "destSrvName" and "destHostnamePort" in the list, but, not "exception_message_____"
		"""
		h = [ "name____", "status", 
		      "nRes", "maxRes", 
		      "nRequests", "max_Req", 
		      "route", "destSrvName__", "destHostnamePort__________",
		    ]

		return h

	def doOperation(self, res, b_newCreate):
		# 
		# Called by:	gxrespool.putResInQueueExt() -- ONLY
		#
		# NOTE: this method implement a real method (the superclass has a stub method).
		#
		oper = self.getOperationToDo()

		if (oper == OPER_CLEAN):
			res.closeConn()
		else:
			pass		# do nothing!


	def shutdownResources(self, b_quiet=0):
		#
		# close all DestConn sockets in this pool
		#
		# STEPS:
		#	make sure resourcesqueue is open for both get() and put()
		#	get() each DestConn, and,
		#	close its socket
		#	put() back the DestConn in the resourcesqueue
		#	close the resourcesqueue for get() and put().
		#
		try:
			ctrDone = 0

			self.resourcesQueue.assignOpenStatus(b_openGet=1, b_openPut=1)

			for i in xrange(0, self.maxRes):
				destConn = self.resourcesQueue.get_nowait()
				if (destConn and isinstance(destConn, DestConn) ):
					destConn.closeConn()
					ctrDone = ctrDone + 1

				self.resourcesQueue.put(destConn, block=0)   # block=0 will retry a few times if fails

			self.resourcesQueue.assignOpenStatus(b_openGet=0, b_openPut=0)

			if (b_quiet):
				return None		# no confirmation msg in log

			if (ctrDone + 1) >= self.maxRes:
				msg = "[close() completed]"
			else:
				msg = "[PARTIAL close()]"

			m1 = "%s: closed %s of %s destConn: %s." % (self.name, ctrDone, self.maxRes, msg)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		except:
			ex = gxutils.formatExceptionInfoAsString(5)
			m1 = "%s - shutdownResources(): FAILED with Exc: %s. ctrDone=%s." % (self.name, ex, ctrDone)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)


	def createSocketAndConnect(self, b_logError=1):
		#
		# Called by: 	DestConn.connectToDestination()
		#		DestConnPool.createNewResource()
		#
		# This function creates a socket, then, it attempts to connect().
		#
		# The actual hostname and port number come from the xmltp_hosts file.
		# These infos are retrieved dynamically each time before doing connect()
		# therefore, the file can be updated while the server is running
		# and the changes will take effect at the next call of this function.
		#
		stepDesc = None
		try:
			self.destSrvName = "..."
			self.destHostnamePort = "..."

			stepDesc = "socket.socket()"

			sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

			# setsockopt() BEFORE connect()...
			#
			stepDesc = "sock.setsockopt(,socket.SO_RCVTIMEO,)"
			timeout_val = struct.pack("ll", self.recvTimeout, 0)

			sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout_val)

			# setsockopt() with SO_SNDTIMEO and TCP_NODELAY...
			#
			stepDesc = "sock.setsockopt(,socket.SO_SNDTIMEO,)"
			timeout_val = struct.pack("ll", self.sendTimeout, 0)
			sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDTIMEO, timeout_val)

			stepDesc = "sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)"

			sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)

			if (self.getDebugLevel() >= 10):
				m1 = stepDesc + " DONE."
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			#
			# Use instance attributes .currentRoute and .routesDict to dynamically
			# find which host & port to connect to:
			#
			stepDesc = "resolve current route with dest routesDict"

			self.destSrvName = "?"
			self.destHostnamePort = "?"

			srvName = self.routesDict.get(self.currentRoute, None)
			if (not srvName):
				m1 = "%s createSocketAndConnect(): cannot resolve route '%s' using %s." % (self.getName(), self.currentRoute, self.routesDict)
				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
				return None

			self.destSrvName = srvName	# remember it to make it available for operator command later
	
			# Look up srvName in xmltp_hosts...
			#
			stepDesc = "resolve srvName to (hostname, port)"

			x = xmltp_hosts.getServerHostAndPort(srvName)
			if (not x or type(x) == type("s") ):
				m1 = "%s createSocketAndConnect(): Cannot find host,port for '%s'. Route: %s, Errmsg: %s." % (self.getName(), srvName, self.currentRoute, x)
				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
				return None

			h, p = x

			self.destHostnamePort = x	# remember it to make it available for operator command later

			if (self.getDebugLevel() >= 5):
				m1 = "%s about to connect to '%s' (%s, %s). Route: %s." % (self.getName(), srvName, h, p, self.currentRoute)
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			stepDesc = "connect()"

			sock.connect( (h, p) )		# CONNECT to destination/server

			self.destSrvName = srvName + "+"	# remember it to make it available for operator command later
		except:
			if (b_logError):
				ex = gxutils.formatExceptionInfoAsString()

				m1 = "%s createSocketAndConnect() failed on: %s, Exc: %s." % (self.getName(), stepDesc, ex)

				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

			return (None)

		return (sock)
		

	def createNewResource(self, aList): 
		#
		# this function creates a new resource, a new connection to a "destination".
		#
		# NOTE: this function receives an argument (aList) because this is
		#	the calling convention for the createNewResource() of ALL 
		#	subclasses of gxrespool, but, it does NOT use it.
		#
		#	Using attributes should make it easier to understand and maintain
		#	this code.
		#
		# NOTE-2: The new version of this method does NOT create the socket!
		#	  The first RPC which use this resource will trigger the creation
		#	  of the socket and the connect(), and the login also. (jft, 2002oct02)
		#

		# the context held by a DestConn will be used to intercept RPC results.
		#
		ctx = xmltp_gx.createContext("ctx_" + repr(self.resCreatedCtr) )

		try:
			sock = None	# The first RPC will trigger the creation of a socket and connect()

			destConn = DestConn(name = "%sConn%s" % (self.name, self.resCreatedCtr), 
					 monitor = None,	# will be assigned when destConn gets used
					 data  = sock,		# a socket connection
					 owner = self.reqQueueThread, 
					 pool  = self,
					 data2 = ctx)

			destConn.assignState(DESTCONN_INIT)

			if (self.getDebugLevel() >= 15):
				m1 = "DestConn created: %s" % (destConn, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		except:
			excStr = gxutils.formatExceptionInfoAsString()

			m1 = "%s create DestConn FAILED. Exception: %s." % (self.getName(), excStr)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

			return None

		return (destConn)


	def queueAsynchRPCextended(self, procName, params, pseudoUserid, pseudoMsg, b_privReq=0, sleepBeforeCall=None):
		#
		# this method queue one RPC on the request queue or on the priviledged requests queue of this pool.
		#
		procCall = (procName, params, 0)

		clt_conn = gxclientconn.TempPseudoClientConn("[no master object]")

		frame = gxrpcframe.gxRPCframe(clt_conn)

		clt_conn.assignUserid(pseudoUserid) 
		clt_conn.assignRPCframe(frame, pseudoMsg)

		if (sleepBeforeCall):
			if (sleepBeforeCall > 10):	# sleep() more than 10 seconds is not valid.
				sleepBeforeCall = 3
			elif (sleepBeforeCall < 0):
				sleepBeforeCall = 0
			frame.sleepBeforeCall = sleepBeforeCall

		# NOTE: the argument pool=self in the call to queueProcCallToDestPool()
		#	forces that function to queue the RPC on this DestConnPool
		# and the arg b_privReq decides which requests queue.
		#
		gxrpcmonitors.queueProcCallToDestPool(self, frame, procCall, pool=self, b_privReq=b_privReq, interceptor=0)


	def queueTestRPCtoConnect(self):
		#
		# this method queue one RPC to Test the pool...
		#
		pseudoUserid = self.getName()+"_testRPC"
		pseudoMsg = "Will be useful to check return status later"

		# use "ping" for now. May change later.
		#
		self.queueAsynchRPCextended("ping", [], pseudoUserid, pseudoMsg, b_privReq=0)



#
### END of FILE ###
#
