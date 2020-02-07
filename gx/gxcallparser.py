#!/usr/bin/python
#
# gxcallparser.py -- Gateway XML-TP: Call Parser classes
# ---------------
#
# $Source: /ext_hd/cvs/gx/gxcallparser.py,v $
# $Header: /ext_hd/cvs/gx/gxcallparser.py,v 1.3 2005/11/02 21:27:36 toucheje Exp $
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
# Contains classes related to parsing of XMLTP procedure call:
#	CallParserMonitor(gxmonitor.gxmonitor)
#	CallParserRes(gxrespool.gxresource)
# 	CallParserResPool(gxrespool.gxrespool)
#
#-------------------------------------------------------------------------
# 2002jan29,jft: + extracted from gxserver.py
# 2002feb13,jft: . CallParserResPool.continueProcCallProcessing(): enforce login rule:
# 2002feb20,jft: . CallParserMonitor.parseAndProcessRequest(): detect (sd == None), and recover
# 2002mar02,jft: . import gxintercept
# 2002mar08,jft: . CallParserMonitor.continueProcCallProcessing(): Check if pool.isOpen()
# 2002mar11,jft: . CallParserResPool.continueProcCallProcessing(): Check if (not isOrdinaryLoginAllowed() )
# 2002apr14,jft: . changed many "self.returnClientConnToPollList(clt_conn.getSd() )" to clt_conn.rpcEndedReturnConnToPollList(0, "a msg")
# 2002oct02,jft: . CallParserResPool.createNewResource(): less verbose if debug level < 15
# 2003jan19,jft: . CallParserResPool.continueProcCallProcessing(): handle case when there are No Destination (no RPC forwarding in config file).
# 2003jul02,jft: . CallParserResPool.continueProcCallProcessing(): increment g_ctr_total_rpcs if not a regproc call
#		 + def getCtrTotalRPCs()
# 2005nov02,jft: . CallParserResPool.continueProcCallProcessing(): call gxserver.preExecLogFunc(procName, procCallTuple)
#


import sys
import time
import string
import TxLog
import gxthread
import gxmonitor
import gxrespool
import gxrpcframe
import gxclientconn		# for STATE_RPC_CALL_IN_PROGRESS, STATE_REGPROC_IN_PROGRESS
import gxregprocs		# for isRegProcName()
import gxxmltpwriter
import gxrpcmonitors
import gxintercept		# for gxintercept.RPCinterceptor 
import gxutils			# contains formatExceptionInfoAsString()
import gxreturnstatus
import xmltp_gx


#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxcallparser.py,v 1.3 2005/11/02 21:27:36 toucheje Exp $"

#
# --- Module Constants:
#
MODULE_NAME = "gxcallparser"

#
# Module-scope variables:
#
srvlog = None		# will be assigned by main module
gxServer = None		# referenced by CallParserMonitor.continueProcCallProcessing()

g_ctr_total_rpcs = 0	# incremented by CallParserMonitor.continueProcCallProcessing()

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
	return (RCS_VERSION_ID)

#
# (End standard trace functions)
#

#
# Module level functions:
#
def getCtrTotalRPCs():
	return g_ctr_total_rpcs		# that ctr is incremented by CallParserMonitor.continueProcCallProcessing()



#
# ---------------------------- Classes: -----------------------------
#

#
# Classes used for "RPC Call Parsers Pool":
#
class CallParserMonitor(gxmonitor.gxmonitor):
	def runThread(self, aThread):
		try:
			self.parseAndProcessRequest(aThread)
		except:
			self.ctrErrors = self.ctrErrors + 1

			excStr = gxutils.formatExceptionInfoAsString()

			m1 = "Error in CallParserMonitor (%s) exc: %s." %  (aThread, excStr)

			srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, self.ctrErrors, m1)


	def parseAndProcessRequest(self, aThread):
		aThread.changeState(gxthread.STATE_ACTIVE)

		if (getDebugLevel() >= 5):
			m1 = "about to parse request: frame=%s, pool=%s, mon=%s, %s" % (aThread.getFrame(),
				aThread.pool.getName(), aThread.getMonitor(), aThread.getId())
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )
		
		# get the socket fd from the client conn held in the RPC frame
		#
		frame = aThread.getFrame()
		conn  = frame.getClientConn()
		sd = conn.getSd()

		# The resource "data"is a XML-TP parser context bound to the resource.
		# Get the resource from the thread, then the data from the resource.
		#
		# res = aThread.getResource()
		# data = res.getData()
		#
		if (sd == None):	# sd could be None is client has already disconnected!
			procCall = None
		else:
			try:
				procCall = xmltp_gx.parseProcCall(aThread.resource.data, sd)
			except:
				procCall = None
				ex = gxutils.formatExceptionInfoAsString(1)
				m1 = "xmltp_gx.parseProcCall() failed, client %s EXCEPTION: %s" % (conn, ex)
				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0,  m1)

		#
		# print "parsing DONE --", aThread, "releasing res=", aThread.resource, "..."
		#
		aThread.pool.releaseResource(aThread.resource, b_useSuccessful=(procCall!=None) )

		aThread.pool.continueProcCallProcessing(aThread.getFrame(), procCall)

		aThread.changeState(gxthread.STATE_COMPLETED)


class CallParserRes(gxrespool.gxresource):
	def processRequest(self, request):
		threadIdSuffix = repr(self.pool.incrementGetThreadsCtr() )

		if (getDebugLevel() >= 30):
			print "processRequest(): resource=", self, "request=", request

		t = gxthread.gxthread(self.name + "Thr" + threadIdSuffix,
					monitor=self.monitor,
					resource=self,
					frame=request,
					pool=self.pool,
					threadType=gxthread.THREADTYPE_QUEUE )

		self.assignUsedByThread(t)

		t.start()


class CallParserResPool(gxrespool.gxrespool):
	def createNewResource(self, aList): 
		#
		# this function receives the pool.resCreationArgsList
		# as argument (aList) and it creates a new resource.
		#
		ctx = xmltp_gx.createContext("ctx_" + repr(self.resCreatedCtr) )

		mon = aList[0]
		res = CallParserRes(name="CallParsR" + repr(self.resCreatedCtr), 
				 monitor=mon, 		# mon comes from aList[0]
				 data = ctx,		# a XML-TP parser context
				 owner=self.reqQueueThread, 
				 pool=self)

		if (getDebugLevel() >= 15):
			m1 = "CallParserRes created: %s, with context: %s" % (res, ctx)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		return (res)



	def continueProcCallProcessing(self, frame, procCall):
		#
		# Normal processing:
		#	if parsing error:
		#		send response (error message, -1001) to client
		#		return client conn to clientsPollList
		#	else:
		#		find the proper RPCmonitor
		#		 check if the RPC gate is closed/open
		#		find the Destination matching the procCallName
		#		 check if this Destination is closed/open
		#		queue the RPCframe on that Destination
		#
		global g_ctr_total_rpcs

		clt_conn = frame.getClientConn()

		# Client has disconnected:
		#
		if (procCall == None):
			# this means that client disconnected...
			#
			m1 = "Disconnect: %s. Detected by Proc Call parser. " % (clt_conn, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			# NOTE: self.master.addClientToListOfThoseToRemoveFromPollList(clt_conn.getSd() )
			#	will be done by clt_conn.processDisconnect()
			#
			clt_conn.processDisconnect()

			return (None)

		# Parsing of procCall failed:
		#
		if (type(procCall) is type("s")):
			m1  = "Parse error on %s. Last errmsg: %s" % (clt_conn, procCall, )
			ret = gxreturnstatus.PARSE_ERROR_CANNOT_GET_PARAMS

			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, ret , m1)

			gxxmltpwriter.replyErrorToClient(frame, [ (ret, procCall), ], ret)

			clt_conn.rpcEndedReturnConnToPollList(0, "parsing failed")
			return (None)

		# Continue the processing of this procedure call:
		#	find the proper RPCmonitor
		#	 check if the RPC gate is closed/open
		#	find the Destination matching the procCallName
		#	 check if this Destination is closed/open
		#	queue the RPCframe on that Destination
		#
		if (getDebugLevel() >= 10):
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, repr(procCall) )

		procCallTuple = procCall	# keep a reference to procCall as a Tuple, for writing the pre-exec log later

		frame.assignProccall(procCall)	# here procCall is a tuple

		procCall = frame.getProcCall()	# here procCall is a gxrcframe.gxProcCall instance

		procName = frame.getProcName()

		try:
			state = -1
			b_isLoginProc = 0
			b_isAdminUser = 0
			if (gxregprocs.isRegProcName(procName) ):
				state = gxclientconn.STATE_REGPROC_IN_PROGRESS
				#
				# call isLoginProc() with procCall as arg to be able to 
				# find out if this is an admin user!
				#
				(b_isLoginProc, b_isAdminUser) = gxregprocs.isLoginProc(procCall)
			else:
				state = gxclientconn.STATE_RPC_CALL_IN_PROGRESS
				g_ctr_total_rpcs = g_ctr_total_rpcs + 1

			# If the client is not login already, then the only acceptable
			# procName is login. Otherwise, disconnect the client! 
			#
			if (not clt_conn.isLoginOK() and not b_isLoginProc):
				#
				# We do not send back a response because the client is breaking
				# the protocol.  We disconnect it immediately.
				#
				clt_conn.processDisconnect("Rejecting non-login RPC before login!")
				return (None)

			clt_conn.assignState(state, b_isLoginProc=b_isLoginProc)
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			m1 = "assignState(%s) denied on %s EXCEPTION: %s" % (state, clt_conn, ex)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0,  m1)

			ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY
			msg = "client connection state not valid."

			gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)

			clt_conn.rpcEndedReturnConnToPollList(0, "clt state invalid")
			return (None)

		if (b_isLoginProc and not b_isAdminUser):
			#
			# Check if max number of ordinary clients has been reached:
			#
			if (not gxServer.isOrdinaryLoginAllowed()  or  gxclientconn.getNumberOfClients() >= gxServer.getMaxCltConnections() ):
				if (not gxServer.isOrdinaryLoginAllowed() ):
					ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY
					msg = "Max number of connections reached"
				else:
					ret = -100
					msg = "Connection gate is closed" 

				gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)
				#
				# We also disconnect this client:
				#
				clt_conn.processDisconnect(msg)		# too many client or Connection gate closed

				if (getDebugLevel() >= 3):
					m1 = "login rejected: '%s' conn=%s." % (msg, clt_conn, )
					srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0,  m1)

				return (None)

			if (gxServer.isThisRPCallowed(procName) != 1):
				tup = gxServer.isThisRPCallowed(procName)
				if (type(tup) == type( () ) ):
					ret, msg = tup

					if (getDebugLevel() >= 3):
						m1 = "login rejected: RPC not allowed. %s" (tup, )
						srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0,  m1)

					gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)
					#
					# We also disconnect this client:
					#
					clt_conn.processDisconnect("RPC gate closed, cannot login")

					self.writeEndRPClogMsgGateClosed(frame, clt_conn, ret, msg, procName)

					return (None)

		if (gxServer.isThisRPCallowed(procName) != 1):
			tup = gxServer.isThisRPCallowed(procName)
			if (type(tup) == type( () ) ):
				ret, msg = tup

				gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)

				self.writeEndRPClogMsgGateClosed(frame, clt_conn, ret, msg, procName)

				clt_conn.rpcEndedReturnConnToPollList(0, "RPC not allowed")

				return (None)

		#
		# Choose a monitor: RPC, regproc or Login...
		#
		monitor = gxrpcmonitors.mapProcnameToRPCmonitor(procCall, b_isLoginProc, b_isAdminUser)


		# Check if the monitor is a tuple:
		#
		#	(interceptor, monitor)	:	interceptor must be bound to the frame 
		#					and monitor will use it later to do the
		#					RPC interception.
		#
		#	(ret, msg)		:	that would indicates that a RPC
		#					intercept gate/queue is closed or full.
		#
		interceptor = None		# assume it is None for now

		if (type(monitor) == type( () ) ):
			tup         = monitor
			monitor     = None
			interceptor = None
			if (len(tup) != 2):
				ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY
				msg = "RPC aborted: normal capture cannot be done (see log)"

				m1 = "tuple returned by gxrpcmonitors.mapProcnameToRPCmonitor('%s') is wrong: %s", (procName, tup, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0,  m1)

				gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)

				clt_conn.rpcEndedReturnConnToPollList(0, "invalid interceptor tuple")

				self.writeEndRPClogMsgGateClosed(frame, clt_conn, ret, msg, procName)

				return (None)

			if (isinstance(tup[0], gxintercept.RPCinterceptor) ):
				interceptor, monitor = tup
			else:
				ret, msg = tup

				gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)

				clt_conn.rpcEndedReturnConnToPollList(0, msg)

				self.writeEndRPClogMsgGateClosed(frame, clt_conn, ret, msg, procName)

				return (None)

		try:
			frame.assignMonitor(monitor, interceptor)	# interceptor can be None

			if (b_isLoginProc  and  not b_isAdminUser):
				#
				# All ordinary login checks are done with a special SP,
				# which is associated to a specific pool.
				#
				pool = self.master.getCheckPasswordDestConnPool()
				if (pool == None):
					#
					# name of AUTHENTICATION_RPC not in config file!??
					#
					ret = -106
					msg = "normal login not accepted on this server"

					gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)

					clt_conn.processDisconnect("Login rejected, No login SP in config.")

					self.writeEndRPClogMsgGateClosed(frame, clt_conn, ret, msg, procName)

					return (None)
			else:
				pool = self.master.mapProcnameToPool(procName)

			#
			# Check if this DestConnPool is open:
			#
			if (not pool or not pool.isOpen() ):
				if (not pool):
					ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY
					msg = "No Destination for this RPC"
				else:
					ret = gxreturnstatus.DS_NOT_AVAIL
					msg = gxServer.getUnavailabityMessage(2, pool.getName() )

				gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)

				if (b_isLoginProc  and  not b_isAdminUser):
					clt_conn.processDisconnect("Login rejected, Dest closed")
				else:
					clt_conn.rpcEndedReturnConnToPollList(0, "Dest closed")

				self.writeEndRPClogMsgGateClosed(frame, clt_conn, ret, msg, procName)

				frame.assignMonitor(None)

				return (None)

			#
			# OK, add the RPC request (the frame) on the pool queue:
			#
			pool.addRequest(frame)		

			if (getDebugLevel() >= 7):
				m1 = "proc '%s' from %s, fd=%s, queued on pool %s." % (procName, 
							clt_conn.getUserId(), clt_conn.getSd(),
							pool.getName() )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0,  m1)

			# if the config says so, write the procCallTuple to the pre-exec log:
			#
			if gxServer.preExecLogFunc != None:
				gxServer.preExecLogFunc(procName, procCallTuple)

		except:
			m1 = "frame.assignMonitor(monitor) EXCEPTION: %s" % (gxutils.formatExceptionInfoAsString(), )
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0,  m1)

		return (None)


	def writeEndRPClogMsgGateClosed(self, frame, clt_conn, retStat, errMsg, procName):
		#
		# Similar to writeEndRPClogMsg() in gxrpcmonitors.py
		#
		if (clt_conn != None):
			userid = clt_conn.getUserId()
			fd     = clt_conn.getSd()
		else:
			userid = "[unknown user]"
			fd     = -1

		if (frame != None):
			ms = frame.getElapsedMs()
		else:
			ms = "??"

		if (not errMsg):
			errMsg = "OK"

		m1 = "End of '%s' %s ms, from %s, fd=%s, stat=%s %s, '%s'." % (procName, 
					ms, userid, fd, 
					retStat, 
					gxreturnstatus.getDescriptionForValue(retStat),
					errMsg)

		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, retStat, m1)

		if (frame and retStat <= -998 or getDebugLevel() >= 2):

			m1 = "Timings of '%s' from %s:%s." % (procName, 
					    userid, frame.getEventsList(1) )

			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, retStat, m1)





if __name__ == '__main__':

	parserMon = CallParserMonitor("CallParserMon", masterMonitor=self)

	maxRes = 3

	parsersPool = CallParserResPool(name="CallParsersPool", 
					maxRes = maxRes,
					resCreationArgsList=[parserMon, ],
					maxRequests = 0,
					master = "[UNIT-TEST]")

	parsersPool.assignDebugLevel(10)

	m1 = "callParsersPool started: %s." % (repr(parsersPool), )
	print m1
