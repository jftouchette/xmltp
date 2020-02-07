#!/usr/bin/python
#
# gxrpcmonitors.py : subclasses of gxmonitor, for RPC execution
# -------------------------------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxrpcmonitors.py,v $
# $Header: /ext_hd/cvs/gx/gxrpcmonitors.py,v 1.9 2006/02/17 03:04:56 toucheje Exp $
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
# A "monitor" is the object whose methods are executed within a thread.
# (see gxmonitor.py for details).
# 
# The subclasses of gxmonitor.gxmonitor which are in this module are used
# to execute RPC call or registered procedures:
#
#	gxGenRPCAndRegProcMonitor(gxmonitor.gxmonitor): is an "abstract class", It is
#						the super-class of:
#								gxRegProcMonitor
#								gxRPCmonitor
#								gxClientLoginMonitor
#								gxServiceRPCmonitor
#	gxRegProcMonitor			execute a registered procedure
#	gxRPCmonitor				forward RPC to outgoing connection
#	gxInterceptRPCMonitor(gxRPCmonitor):	forward RPC, with results interception
#	gxClientLoginMonitor			perform an AuthenticationRPC
#	gxDestConnLoginMonitor(gxInterceptRPCMonitor):	does a 'LOGIN' for a DestConn
#	gxServiceRPCmonitor(gxGenRPCAndRegProcMonitor):  used by service threads to send a RPC.
#
# ***NOTE about conversion of "&amp;", "&gt;" and "&lt;" in XML input: 
#    ----------------------------------------------------------------
#	The runThread() method of the following classes and subclasses (indirectly) calls 
# 	normalizeParamsOfProcCall(frame.getProcCall()):
#		gxRegProcMonitor
#		gxRPCmonitor
#		gxInterceptRPCMonitor(gxRPCmonitor)
#
# Also, this module contains the "ProcNameToMonitorMapper", namely, the function
# mapProcnameToRPCmonitor().
#
# And, also, the function queueProcCallToDestPool(originObject, frame, procCall)
# which allows a service thread to initiate a RPC call.
#
#
# ----------------------------------------------------------------------------------------------------
#
# 2001nov25,jft: first version
# 2001dec23,jft: + gxDestConnLoginMonitor
# 2001dec24,jft: . gxRPCmonitor.runThread(): call rc = aThread.resource.verifyDestConn()
#		 . receiveResponse(): write full_resp to log
#		 . debugLevel = 2   to get RPC timings
# 2001dec28,jft: . gxRPCmonitor.runThread(): write debug msg in log when frame.getPrevExcInfo()
#		 . debugLevel = 10 for testings
# 2002jan09,jft: . gxRPCmonitor.runThread(): better diag info if aThread.resource.verifyDestConn() != 0
# 2002jan17,jft: . gxGenRPCAndRegProcMonitor.recoverRPCexecError(): call self.writeEndRPClogMsg()
# 2002jan29,jft: + added standard trace functions
# 2002jan30,jft: + import gxregprocs
#		 . gxRegProcMonitor.runThread(): calls gxregprocs.execRegProc(aThread, frame, clt_conn)
# 2002feb13,jft: . gxRegProcMonitor.runThread(): only do clt_conn.assignState() if (not clt_conn.isDisconnected() )
# 2002feb16,jft: + class gxClientLoginMonitor
# 2002feb26,jft: + class gxInterceptRPCMonitor
# 2002mar01,jft: . gxClientLoginMonitor.runThread(): changed call gxrpcframe.getValueOfParam(), was gxregprocs.getValueOfParam()
#		 . mapProcnameToRPCmonitor(): calls interceptor = gxintercept.mapProcCallToInterceptor(procCall)
#					      if (interceptor): return (interceptor, mon)
# 2002mar15,jft: + getCustomRegProcMonForProcNameX(x) -- added support for custom regprocs
#		 + addCustomProcToRegProcMonDict(procName, monClass)
#		 . mapProcnameToRPCmonitor(): call getCustomRegProcMonForProcNameX()
# 2002apr07,jft: + gxRegProcMonitor.execRegProc(self, aThread, frame, clt_conn): added specifically to allow override in a subclass
# 2002apr14,jft: + queueProcCallToDestPool(originObject, frame, procCall): function to do service RPC calls
#		 + class gxServiceRPCmonitor(gxGenRPCAndRegProcMonitor)
# 2002apr16,jft: . gxRegProcMonitor.execRegProc(): calls normalizeParamsOfProcCall(procCall) which normalizes
#			the object types of the params values. (The XMLTP parser make them all String).
#		   NOTE: the other RPC monitors do NOT do this normalization as the params values will be 
#			 simply forwarded as is... as string in the XMLTP datastream.
#		 + normalizeParamsOfProcCall(procCall)
# 2002apr17,jft: . gxGenRPCAndRegProcMonitor.receiveResponse(): fixed heuristics to find returnstatus and EOT in the recv() stream.
#		 . gxRPCmonitor.receiveResponseAndSendBackToClient(): same, fixed heuristics to find returnstatus and EOT.
# 2002may13,jft: . support of <sz> : normalizeParamsOfProcCall(procCall): expect sz element in param tuple
# 2002jun07,jft: . gxGenRPCAndRegProcMonitor.sendRPCcallAndReceiveResponse(): generalized for use by most by 
#			sub-classes (except for result interception), and, added retry of gxxmltpwriter.sendProcCall()
# 2002jun09,jft: . use larger buffer for recv(), increase from 1000 to 5000 bytes
# 2002jun30,jft: . gxRPCmonitor.receiveResponse(): if partial response sent to client, disconnect client (do NOT send XMLTP reset)
#		 . sendMessageAndReturnStatusToClient(): improved, will disconnect client if required
#		 . gxRPCmonitor.receiveResponse() and gxInterceptRPCMonitor.processResponse(): now use sendMessageAndReturnStatusToClient()
# 2002jul03,jft: . gxInterceptRPCMonitor.sendRPCcallAndReceiveResponse(): do timeout and recover disconnect when calling xmltp_gx.parseResponse()
# 2002jul07,jft: . moved frame.assignStep(gxrpcframe.STEP_RPC_CALL_COMPLETED) before the various self.writeEndRPClogMsg() 
#			so that the "Timings" log entry can be be more regular and accurate.
# 2002jul09,jft: . with accurate timeout when multiple recv() for ordinary RPC
# 2002aug21,jft: . normalizeParamsOfProcCall(procCall): calls unEscapeXML()
#		 . normalizeParamsOfProcCall(): fixed, was dropping sz when normalizing a non-string param
#		 + unEscapeXML(val)
#		 . gxRPCmonitor.runThread(): calls normalizeParamsOfProcCall(frame.getProcCall() )
# 2002sep26,jft: . both implementations of receiveResponse(): check rc from
#			dest_conn.verifyDestConn(1) before return -1
# 2002oct02,jft: . queueProcCallToDestPool(): new argument pool=None
# 2002oct06,jft: + markDestConnDirtyIfRPCfailedBad(frame, destConn)
#		 . runThread() method in (3) monitor classes now call markDestConnDirtyIfRPCfailedBad(). 
#		   These (3) monitor classes are: gxRPCmonitor, gxClientLoginMonitor, gxServiceRPCmonitor.
# 2002oct13,jft: . gxRPCmonitor: if client disconnect, mark DestConn as dirty (it might have incomplete response on it)
#		 . gxInterceptRPCMonitor.processResponse(): if client disconnect, or stange resp, mark DestConn as dirty
# 2002oct20,jft: . class gxDestConnLoginMonitor now inherits from gxInterceptRPCMonitor
#		 . queueProcCallToDestPool(): new arg: b_privReq=0
#		 . gxInterceptRPCMonitor.interceptResponse(): use list from frame.getTempInfo(TEMP_INFO_LIST_RESP_PARTS_TO_GRAB), if available
# 2002oct26,jft: . gxServiceRPCmonitor.runThread(): if frame.sleepBeforeCall exists, sleep() that many seconds before sending RPC
#		 . markDestConnDirtyIfRPCfailedBad(): ret==400 does _not_ mark DestConn "dirty"
# 2003jan19,jft: . queueProcCallToDestPool(): handle case when there are No Destination (no RPC forwarding in config file).
# 2003jun10,jft: . gxRPCmonitor.runThread(): if module level function g_fn_ValidateParams is not None, call it.
# 2003jun11,jft: . gxRPCmonitor.runThread(): if rc from g_fn_ValidateParams() != 0, assign "audit param bad or missing"
#		 . if timeouts or broken conn, call DestConn.assignOutcomeOfUse(x,x, b_kill=1)
# 2003aug19,jft: . unEscapeXML(): takes care of xmltp_tags.META_CHAR_APOS and META_CHAR_QUOT
# 2004may26,as/jbl:	 . Dont assign a monitor if already in frame in queueProcCallToDestPool.
# 2004aug01,jft: . Class gxServiceRPCmonitor now derived from gxInterceptRPCMonitor. To allow result set interception.
#		 . queueProcCallToDestPool(): one more optional arg, interceptor=None. It is passed to frame.assignMonitor(monitor, interceptor)
#		 . gxInterceptRPCMonitor.interceptResponse(): if (0 == interceptor):	return None -- this means we should not intercept anything.
# 2004aug07,jft: . gxServiceRPCmonitor.runThread(): add arg clt_conn=None in call to self.sendRPCcallAndReceiveResponse()
# 2004aug17,jft: . normalizeParamsOfProcCall(): corrected text of debug message
# 2005jun30,jft: . gxInterceptRPCMonitor.processResponse(): if (clt_conn and ...): frame.assignStep(gxrpcframe.STEP_REPLYING_DONE)
# 2005sep29,jft: . gxRPCmonitor.receiveResponse(): try recv() again if errno.errno == errno.EAGAIN
#		 . gxGenRPCAndRegProcMonitor.calculateTimeoutForRecvResponse(): max value returned is MAX_SEC_SO_RCVTIMEO (327 seconds)
# 2005sep30,jft: + gxGenRPCAndRegProcMonitor.recvResp() -- remove duplicate code in .receiveResponse() of gxRPCmonitor and gxGenRPCAndRegProcMonitor
#			.recvResp() does not use errno.errno, it handles an exception of timeout
#		 . gxGenRPCAndRegProcMonitor.calculateTimeoutForRecvResponse(): only enforce max value if b_with_max
#		 . removed arg b_isDestConnLogin from signature of methods receiveResponse(), sendRPCcallAndReceiveResponse(), calculateTimeoutForRecvResponse()
# 2006jan29,jft: . gxGenRPCAndRegProcMonitor.writeEndRPClogMsg(): now writes the value of the <Audit User Id> param, if it is present in the procCall.
# 2006feb16,jft: . gxGenRPCAndRegProcMonitor.writeEndRPClogMsg(): now gives details of ms
#

import time
import sys
import socket
import struct

import TxLog
import gxmonitor
import gxthread
import gxrpcframe
import gxclientconn
import gxregprocs
import gxxmltpwriter
import gxintercept	# for gxintercept.mapProcCallToInterceptor() and .convertToPythonTypeIfNotString()
import gxutils
import gxreturnstatus
import gxconst
import gxparam		# for gxparam.GXPARAM_AUDIT_USERID_PARAM_NAME
import xmltp_tags	# need a few tags to find EOT and returnstatus
import xmltp_gx		# for xmltp_gx.parseResponse()

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxrpcmonitors.py,v 1.9 2006/02/17 03:04:56 toucheje Exp $"

#
# CONSTANTS
#
MODULE_NAME = "gxrpcmonitors.py"


# TCP/IP contants and limits:
#
MAX_SEC_SO_RCVTIMEO = 327

# Constants to specify special behavior in Intercept:
#
TEMP_INFO_LIST_RESP_PARTS_TO_GRAB = "LIST_RESP_PARTS_TO_GRAB"

# internal, private constants:
#
#RECV_BUFFER_SIZE = 32000
RECV_BUFFER_SIZE = 8000


# EOT_tag is used by receiveResponseAndSendBackToClient() 
#	  and other methods.
#
EOT_tag = "<" + xmltp_tags.XMLTP_TAG_EOT + ">"



#
# module variables:
#
gxServer = None		# reference to the master gxserver instance

customProcNameToRegProcMonDict = {}	# might be loaded by subclass of gxserver

g_fn_ValidateParams = None	# other modules may assign a ref to it. Called by gxRPCmonitor.runThread()

g_paramWithAuditUserId = None

#
# module-scope debug_level:
#
debug_level = 0			# default value 0 is for production, use 3 for debugging.

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
# module Functions:
#
def getCustomRegProcMonForProcNameX(x):
	#
	# Called by:	mapProcnameToRPCmonitor()
	#
	try:
		return (customProcNameToRegProcMonDict[x] )
	except KeyError:
		return None


def addCustomProcToRegProcMonDict(procName, monClass):
	#
	# Called by:	by sub-class of gxserver, when self.initializeCustom()
	#		is called initialize().
	#
	customProcNameToRegProcMonDict[procName] = monClass


def getListOfCustomRegProcNames():
	ls = customProcNameToRegProcMonDict.keys()
	ls.sort()
	return ls


def mapProcnameToRPCmonitor(procCall, b_isLoginProc=0, b_isAdminUser=0, master=None):
	#
	# Called by:	gxcallparser.py
	#
	# Return:
	#	a monitor		:	a RegProc, Login or RPC monitor
	# or
	#	a tuple:
	#	(interceptor, monitor)	:	the interceptor knows how to intercept this procCall.
	#					the monitor will use it later to do the
	#					RPC interception.
	#	
	#	(ret, msg)		:	this indicates that a RPC
	#					intercept gate/queue is closed or full.
	#
	interceptor = None
	procName = procCall.getProcName()

	if (b_isLoginProc and not b_isAdminUser):
		classOfRPCmonitor = gxClientLoginMonitor

	elif (getCustomRegProcMonForProcNameX(procName) != None):
		classOfRPCmonitor = getCustomRegProcMonForProcNameX(procName)

	elif (gxregprocs.isRegProcName(procName) ):
		classOfRPCmonitor = gxRegProcMonitor

	else:
		interceptor = gxintercept.mapProcCallToInterceptor(procCall)
		if (interceptor):
			if (type(interceptor) == type( () ) ):	# RPC rejected, queue full or closed
				return (interceptor)

			classOfRPCmonitor = gxInterceptRPCMonitor
		else:
			classOfRPCmonitor = gxRPCmonitor

	# use <class>.__name__ as name for the monitor:
	#
	monitorName = classOfRPCmonitor.__name__

	# return an instance of the class:
	#
	mon = classOfRPCmonitor(name=monitorName, masterMonitor=master)

	if (interceptor):
		return (interceptor, mon)

	return (mon)


#
# Utility functions (can be used by custom regprocs module also):
#
def unEscapeXML(val):
	#
	# Called by:	gxrpcmonitors.normalizeParamsOfProcCall()
	#		gxintercept.normalizeRowValues()
	#
	# NOTE: If you update this function, you must also update gxxmltpwriter.escapeForXML()
	#
	if (type(val) != type("s") ):
		return val				# do not touch val, it is not a string

	val = val.replace(xmltp_tags.META_CHAR_APOS, "'")
	val = val.replace(xmltp_tags.META_CHAR_QUOT, '"')

	val = val.replace(xmltp_tags.META_CHAR_GT, ">")
	val = val.replace(xmltp_tags.META_CHAR_LT, "<")
	return val.replace(xmltp_tags.META_CHAR_AMP, "&")	# AMP must be done as last.


def normalizeParamsOfProcCall(procCall):
	#
	# Called by:	gxRPCmonitor.runThread()
	#		gxRegProcMonitor.execRegProc()
	#		custom regprocs module (maybe)
	#
	# NOTE: The XMLTP parser stores all procCall parameters values as String,
	#	even if the datatype of a parameter is Int or Float. 
	#
	# The following loop fixes those instances...
	#
	if (not procCall):
		return

	paramsList = procCall.getParamsList()

	idx = 0
	ctrDone = 0
	for param in paramsList:
		name, val, jType, isNull, isOutput, sz  = param

		# NOTE: gxintercept.convertToPythonTypeIfNotString() returns None if val does not
		#	require to be converted.
		#
		newValue = gxintercept.convertToPythonTypeIfNotString(val, jType, isNull)

		if (newValue != None):
			ctrDone = ctrDone + 1
		else:
			newValue = unEscapeXML(val)

		paramsList[idx] = (name, newValue, jType, isNull, isOutput, sz)			

		idx = idx + 1

	if (getDebugLevel() >= 20):
		m1 = "normalized %s parameters (with %s int or float)." % (len(paramsList), ctrDone, )
		gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)


#
# Module function which allows a service thread to initiate a RPC call:
#
def queueProcCallToDestPool(originObject, frame, procCall, pool=None, b_privReq=0, interceptor=None):
	#
	# NOTES:
	#	. this function allows a service thread to execute arbitrary RPC calls asynchronously.
	#	. the "frame" must hold a client connection (an instance of gxclientconn.PseudoClientConn)
	#	. synchronisation after RPC completion will be done when the RPC exec monitor calls
	#	  <PseudoClientConn>.rpcEndedReturnConnToPollList().
	#	  As <PseudoClientConn>.rpcEndedReturnConnToPollList() will call:
	#		self.masterToNotify.notifyRPCcomplete(b_rpc_ok, msg)
	#	  See NOTES in gxclientconn.PseudoClientConn class definition.
	#
	# This function goes through these steps:
	#	create a gxServiceRPCmonitor instance
	#	 check if the RPC gate is closed/open
	#	find the Destination matching the procCallName
	#	 check if this Destination is closed/open
	#	queue the RPCframe on that Destination
	#

	clt_conn = frame.getClientConn()

	if (getDebugLevel() >= 10):
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, repr(procCall) )

	frame.assignProccall(procCall)	# here procCall is a tuple

	procCall = frame.getProcCall()	# here procCall is a gxrcframe.gxProcCall instance

	procName = frame.getProcName()	# we use procName for logging

	if (getDebugLevel() >= 7):
		m1 = "queueProcCallToDestPool(): Begin... proc '%s' from %s." % (procName, clt_conn)
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0,  m1)

	try:
		state = gxclientconn.STATE_RPC_CALL_IN_PROGRESS

		clt_conn.assignState(state)
	except:
		ex = gxutils.formatExceptionInfoAsString(3)
		m1 = "assignState(%s) denied on %s EXCEPTION: %s." % (state, clt_conn, ex)
		gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0,  m1)

		clt_conn.rpcEndedReturnConnToPollList(0, "clt state invalid")
		return (None)

	try:
		if (gxServer.isThisRPCallowed(procName) != 1):
			tup = gxServer.isThisRPCallowed(procName)
			if (type(tup) == type( () ) ):
				ret, msg = tup
			else:
 				ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY
				msg = "RPC not allowed (%s)" % (tup, )

			writeEndRPClogMsg(frame, clt_conn, ret, msg, procName)

			clt_conn.rpcEndedReturnConnToPollList(0, "RPC not allowed")

			return (None)

		#
		# Create a service RPC monitor:
		#

		if not frame.getMonitor():
			monitor = gxServiceRPCmonitor(name="gxServiceRPCmonitor")
			frame.assignMonitor(monitor, interceptor)


		if (pool == None):
			pool = gxServer.mapProcnameToPool(procName)


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

			clt_conn.rpcEndedReturnConnToPollList(0, "Dest closed")

			writeEndRPClogMsg(frame, clt_conn, ret, msg, procName)

			frame.assignMonitor(None)

			return (None)

		#
		# OK, add the RPC request (the frame) on the pool queue:
		#
		if (b_privReq):
			pool.addPrivRequest(frame)
		else:
			pool.addRequest(frame)		

		if (getDebugLevel() >= 7):
			m1 = "proc '%s' from %s, fd=%s, queued on pool %s." % (procName, 
						clt_conn.getUserId(), clt_conn.getSd(),
						pool.getName() )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0,  m1)
	except:
		m1 = "queueProcCallToDestPool(): EXCEPTION: %s" % (gxutils.formatExceptionInfoAsString(), )
		gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0,  m1)

	return (None)

#
# Functions used within this module:
#
def markDestConnDirtyIfRPCfailedBad(frame, destConn):
	#
	# Called by:	gxRPCmonitor.runThread() 
	#		gxClientLoginMonitor.runThread() 
	#		gxServiceRPCmonitor.runThread() 
	#
	if (frame == None  or  destConn == None):
		return
	try:
		ret = frame.getReturnStatus()
		if (ret >= 0):
			return

		if (ret == 400			# 400 is a special condition detected by the SP, not an error
		 or ret == gxreturnstatus.OS_RPC_GATE_CLOSED
		 or ret == gxreturnstatus.OS_RPC_NAME_BLOCKED
		 or ret == gxreturnstatus.GW_QUE_SIZE ):
		 	# RPC failed, but, conn not dirty:
			destConn.assignOutcomeOfUse(b_success=0, b_dirty=0)
			return

		destConn.assignOutcomeOfUse(b_success=0, b_dirty=1)

		if (getDebugLevel() >= 30):
			m1 = "marked this destConn as 'dirty': %s" % (destConn, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
	except:
		return


def getNameOfParamWithAuditUserId():
	""" Called by:	gxGenRPCAndRegProcMonitor.writeEndRPClogMsg()
	"""
	global g_paramWithAuditUserId

	if not g_paramWithAuditUserId:
		g_paramWithAuditUserId = gxServer.getOptionalParam(gxparam.GXPARAM_AUDIT_USERID_PARAM_NAME)
		if g_paramWithAuditUserId or getDebugLevel() >= 5:
			m1 = "%s: %s" % (gxparam.GXPARAM_AUDIT_USERID_PARAM_NAME, g_paramWithAuditUserId, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

	return g_paramWithAuditUserId


def writeEndRPClogMsg_UNUSED_2006jan30(frame, clt_conn, retStat, errMsg, procName):
	""" NOTE: I have checked and this function is now unused. (2006jan30, jft)

	    Write a line to summarize the execution of the proc call. It looks like this:
		End of 'gate' 14 ms, from op, fd=36, stat=0 [OK], 'OK'.
	    or
		End of 'spzz_get_toto_record' 14 ms, from userid001/servlet, fd=36, stat=0 [OK], 'OK'.
	"""

	if (clt_conn != None):
		userid = clt_conn.getUserId()
		fd     = clt_conn.getSd()
	else:
		userid = "[unknown user]"
		fd     = -1

	if (frame != None):
		ms = frame.getElapsedMs()
		p  = frame.getProcCall().getParamByName(getNameOfParamWithAuditUserId() )
		if p:
			auditUserId = p.getValueOfParam()
			userIdSep = "/"
		else:
			auditUserId = ""
			userIdSep = ""	
	else:
		ms = "??"
		auditUserId = ""
		userIdSep = ""

	if (not errMsg):
		errMsg = "OK"

	m1 = "end of '%s' %s ms, from %s%s%s, fd=%s, stat=%s %s, '%s'." % (procName, 
				ms, 
				auditUserId, userIdSep, userid, 
				fd, 
				retStat, 
				gxreturnstatus.getDescriptionForValue(retStat),
				errMsg)

	gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, retStat, m1)

	if (frame and (retStat <= -998 or getDebugLevel() >= 2) ):

		m1 = "Timings of '%s' from %s:%s." % (procName, userid, frame.getEventsList(1) )

		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, retStat, m1)



#
# ----------------- The definitions of the various xxxMonitor classes...
#


class gxGenRPCAndRegProcMonitor(gxmonitor.gxmonitor):
	#
	# *** NOTE: this Generic super-class MUST BE sub-classed
	# ***	    It is not complete by itself.
	#
	# sub-class(es):	gxRegProcMonitor
	#			gxRPCmonitor
	#			gxClientLoginMonitor
	#			gxDestConnLoginMonitor
	#
	# The methods here are used by 1, 2 or more sub-classes.
	#
	#

	# BEGIN - New methods related to RPC timeout (2002july03)
	#
	def checkIfMaxWaitInQueueAlreadyTimeout(self, frame, destConn):
		#
		# Called by:	gxInterceptRPCMonitor.sendRPCcallAndReceiveResponse()
		#
		t_now = time.time()
		t_begin = frame.getTimeRPCBegan()

		if (t_now >= (t_begin + destConn.pool.maxWaitInQueue) ):
			return -1

		return 0


	def calculateTimeoutForRecvResponse(self, destConn, t_beforeFirstSend, b_with_max=1):
		#
		# Called by:	gxInterceptRPCMonitor.sendRPCcallAndReceiveResponse()
		#
		# NOTE: b_isDestConnLogin is not used anymore. (jft,2005sept29)
		#
		# The maximum value returned will be MAX_SEC_SO_RCVTIMEO.
		#
		t_now = time.time()

		recvTimeout = destConn.pool.recvTimeout 

		nb_sec = recvTimeout - (t_now - t_beforeFirstSend)
		nb_sec = int(nb_sec)

		if b_with_max and nb_sec > MAX_SEC_SO_RCVTIMEO:
			return MAX_SEC_SO_RCVTIMEO

		return nb_sec


	def abortRPCandTellClient(self, frame, clt_conn, destConn, msg, ret):
		#
		# Called by:	gxInterceptRPCMonitor.sendRPCcallAndReceiveResponse()
		#
		if (destConn != None):		# desperate, we close the socket, no time left to recv() response
			destConn.closeConn()
			msg = msg + "Closed destConn socket!"

		if (clt_conn):
			self.sendMessageAndReturnStatusToClient(frame, clt_conn, None, ret)	# Only return status

		# RPC completed does NOT mean retstat == 0
		#
		frame.assignStep(gxrpcframe.STEP_RPC_CALL_COMPLETED)

		# write RPC entry to log:
		#
		procName = frame.getProcName()

		b_clt_disconnected = (clt_conn and clt_conn.isDisconnected() )  # could have been disconnected by sendMessageAndReturnStatusToClient()

		self.writeEndRPClogMsg(frame, clt_conn, ret, TxLog.L_MSG, msg, b_clt_disconnected)

		if (b_clt_disconnected):
			return (1)

		return 0


	def disconnectClientBecauseOfPartialResponse(self, frame, clt_conn, destConn, msg, ret):
		#
		# Called by:	gxInterceptRPCMonitor.analyzeResponse()
		#
		# A partial response was already sent to the client. 
		# Then a timeout or disconnect happen on the destConn.
		# We _cannot_ send a return status to the client. The best is to disconnect it!
		#
		# Also, maybe we need to close the destConn socket...
		#
		clt_conn.processDisconnect(msg)

		if (destConn != None):
			destConn.closeConn()

		self.writeRPCwarningMsg(msg, frame, clt_conn, ret, "Disconnect client and destConn because of partial response", 1)

		# RPC completed does NOT mean retstat == 0
		#
		frame.assignStep(gxrpcframe.STEP_RPC_CALL_COMPLETED)

		# write RPC entry to log:
		#
		self.writeEndRPClogMsg(frame, clt_conn, ret, TxLog.L_MSG, msg, 0)

		return 1	# indicates "client disconnect" to some caller (?)
	#
	# END - New methods related to RPC timeout (2002july03)
	#


	def checkIfClientStillConnected(self, clt_conn, frame):
		#
		# While a RPC frame waits in a queue, either until an
		# outgoing connection is available, or, until a RegProc exec
		# resource is free, during that wait, it is possible that
		# the client has already disconnected. (It's socket fd
		# was in the poll list**  to detect a disconnect event and
		# such an event was detected. Then a service proceeded
		# to perform the disconnect clean-up on the client conn.)
		#
		# This method here detects this scenario and write a msg
		# about this in the server log.
		# (Therefore, the caller does NOT need to write to the log).
		#
		# Then it return the proper boolean value.  If it is false (0),
		# the caller should NOT continue with the processing of the 
		# RPC or reg proc, but, simply release the resources.
		#
		# ** NOTE: this feature of the poll list is not yet implemented.
		# 	  2001dec08, jft.
		#
		#
		# Return:	1	if client is valid and still connected
		#		0	if client conn is None or client disconnected
		#
		try:
			if (clt_conn != None and not clt_conn.isDisconnected() ):
				return (1)

			excStr = ""
			ret = gxreturnstatus.NO_RETURN_STATUS_YET
			msg = "client disconnected before exec"
			is_warning = 0
		except:
			excStr = gxutils.formatExceptionInfoAsString()
			ret = gxreturnstatus.OS_EXEC_INIT_FAILED	# unusual ret stat

			msg = "checkIfClientStillConnected() FAILED to assert client conn state."
			is_warning = 1
			clt_conn = None

		self.writeRPCwarningMsg(msg, frame, clt_conn, ret, excStr, is_warning)

		return (0)	# clt_conn disconnected or not valid


	def recoverRPCexecError(self, clt_conn, frame, b_sendUserMsg=1):
		#
		# Called by:  runThread()
		#
		# this method is called when there is an exception in
		# runThread().
		#
		# Here we try to analyze the exception and the rpcframe
		# to find which error it is and what to do to recover.
		#
		# Steps:
		#  1. analyze diag, return status and exception info
		#  2. write log
		#  3. send reply to client (if possible, and if b_sendUserMsg).
		# 
		try:
			b_needRPCwarningMsg = 1		# by default, yes, we need

			userMsg = "Procedure failed.(1)"	# usually replaced by better msg
			logMsg  = None
			supplExcMsg = ""
			ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY

			ret, diagMsg = frame.getReturnStatusAndFinalDiagnosis()

			excMsg  = frame.getExceptionMsg()
			## excDiag = frame.getExceptionDiag()  -- not used

			errMsg  = frame.getErrMsg()

			if (diagMsg != None):
				b_needRPCwarningMsg = 0		# "normal" error, no need to verbose
				userMsg = diagMsg
				logMsg  = "%s, %s, excMsg:%s." % (diagMsg, errMsg, excMsg)
			else:
				userMsg = "RPC failed. See server log (09)."
				
				if (frame.lastErrorIsException() ):
					logMsg = "%s, msg=%s." % (frame.getExceptionDetailsAsString(), errMsg)
				else:
					logMsg = "%s, exc=%s." % (errMsg, frame.getExceptionDetailsAsString() )

			newExcDiag, newExcTuple = gxutils.getDiagnosisAndExceptionInfo(9)

			if (newExcDiag != gxutils.EXC_META_EXCEPTION and type(newExcTuple) == type(()) ):
				supplExcMsg = "newExc: %s, %s, %s." %  newExcTuple
			else:
				supplExcMsg = ""

			if (ret == gxreturnstatus.INIT_NOT_ASSIGNED_YET):
				ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY

			if (b_needRPCwarningMsg or getDebugLevel() >= 1):
				self.writeRPCwarningMsg(logMsg,	frame, clt_conn, ret, supplExcMsg)

			# We will also call writeEndRPClogMsg() because the
			# "End of" log msg is important because it is used to
			# compile statistics about RPC executions from the log.
			#
			if (ret != 0):
				level = TxLog.L_WARNING
			else:
				level = TxLog.L_MSG

			self.writeEndRPClogMsg(frame, clt_conn, ret, level, logMsg, 0)

			try:
				clt_conn.assignState(gxclientconn.STATE_RPC_CALL_ENDED_FAILED)
			except:
				pass

		except:
			userMsg = "Procedure failed.(2)"	# usually replaced by better msg

			excStr = gxutils.formatExceptionInfoAsString()

			self.writeRPCwarningMsg("recoverRPCexecError() FAILED", frame, clt_conn, ret, excStr)

			if (ret == None):
				ret = -1105

		if (frame != None and clt_conn != None and not clt_conn.isDisconnected() ):

			if (b_sendUserMsg):
				self.sendMessageAndReturnStatusToClient(frame, clt_conn, userMsg, ret)

			if (clt_conn != None and not clt_conn.isDisconnected() ):
				clt_conn.rpcEndedReturnConnToPollList(0, "RPC failed")

			try:
				clt_conn.assignState(gxclientconn.STATE_RPC_CALL_ENDED_FAILED)
			except:
				pass


	def sendMessageAndReturnStatusToClient(self, frame, clt_conn, msg, ret):
		#
		# Called by:	.recoverRPCexecError()
		#		gxClientLoginMonitor.runThread()
		#		gxRPCmonitor.receiveResponse()
		#		gxInterceptRPCMonitor.processResponse()
		#
		useridNote = ""		# make sure it has a value anyway.

		if (frame != None and clt_conn != None and not clt_conn.isDisconnected() ):
			if (msg != None):
				msgList = [ (ret, msg), ]
			else:
				msgList = None

			rc = gxxmltpwriter.replyErrorToClient(frame, msgList, ret)
			if (rc == 0):
				return (0)

			if (frame.getExceptionDiag() == gxutils.EXC_DISCONNECT):

				# We must disconnect the client connection by ourself.
				# Note the userid and sd first:
				try:
					useridNote = "Client Disconnect: (%s, fd=%s)" % (clt_conn.getUserId(), clt_conn.getSd() )
				except:
					pass

				clt_conn.processDisconnect(msg)
			else:
				useridNote = frame.getExceptionDetailsAsString()
		else:
			rc = -2

		m1 = "Failed to send msg to client: %s, '%s'" % (ret, msg, )

		self.writeRPCwarningMsg(m1, frame, clt_conn, ret, useridNote, (rc != -2) )

		return (rc)


	def writeRPCwarningMsg(self, msg, frame, clt_conn, retStat, excStr, is_warning=1):
		#
		# Called by:	gxGenRPCAndRegProcMonitor.checkIfClientStillConnected()
		#		gxRPCmonitor.receiveResponseAndSendBackToClient()
		#		.sendMessageAndReturnStatusToClient()
		#		...
		#
		userid, fd, procName, ms = self.getUseridFdProcnameMs(clt_conn, frame)

		m1 = "%s. proc='%s' %s ms, %s, fd=%s, stat=%s %s, '%s'." % (msg, procName, ms,
					userid, fd, 
					retStat, 
					gxreturnstatus.getDescriptionForValue(retStat),
					excStr)
		if (is_warning):
			level = TxLog.L_WARNING
		else:
			level = TxLog.L_MSG

		if (gxServer == None):
			print MODULE_NAME, level, retStat, m1
			return None

		gxServer.writeLog(MODULE_NAME, level, retStat, m1)



	def writeEndRPClogMsg(self, frame, clt_conn, retStat, level, errMsg, b_clt_err):
		""" Write a line to summarize the execution of the proc call. It looks like this:
			End of 'gate' 14 ms (0, 0, 14), from op, fd=36, stat=0 [OK], 'OK'.
		    or
			End of 'spzz_get_toto_record' 140 ms (0.1, 0.7, 139.0), from userid001/servlet, fd=36, stat=0 [OK], 'OK'.

		# Sample from OLD "oesyssrv.log":
		#	End of 'SPRE_UPDATE_BALANCE' 572 ms, from batch1, spid=158, stat=0, 1 rows - rc_mask=13.
		#
		# Previous format written by this function:
		#	End of 'SPRE_UPDATE_BALANCE' 572 ms, from batch1, fd=158, stat=0, client Disconnected 'OK'.
		#
		"""
		userid, fd, procName, ms = self.getUseridFdProcnameMs(clt_conn, frame)

		if (b_clt_err):
			discMsg = "client Disconnected "
		else:
			discMsg = ""
		if (errMsg == None or errMsg == ""):
			errMsg = "OK"

		msDetails = self.getMsDetails(frame)

		m1 = "End of '%s' %s ms %s, from %s, fd=%s, stat=%s %s, %s'%s'." % (procName, 
					ms, msDetails,userid, fd, 
					retStat, 
					gxreturnstatus.getDescriptionForValue(retStat),
					discMsg, errMsg)

		if (gxServer == None):
			print MODULE_NAME, level, retStat, m1
			return None

		gxServer.writeLog(MODULE_NAME, level, retStat, m1)

		if (retStat <= -998 or getDebugLevel() >= 3):
			m1 = "Timings of '%s' from %s:%s." % (procName, userid, frame.getEventsList(1) )

			gxServer.writeLog(MODULE_NAME, level, retStat, m1)

	def getMsDetails(self, frame):			
		""" Called by:	writeEndRPClogMsg()
		    Return:
			string "(0.1, 0.1, n.x)" (times for: call parsing, get a destConn, receive/send results)
		"""
		if not frame:
			return "(-2, -1, -1)"
		try:
			# We need four (4) timestamps to compute 3 intervals or deltas (d1, d2, d3).
			# These 4 timestamps are: tBegin, tParsed, tResDest, tDone
			#
			tBegin  = frame.getTimeActivityN(gxrpcframe.STEP_INIT)
			tParsed = frame.getTimeActivityN(gxrpcframe.STEP_ASSIGN_PROCCALL)
			if tParsed < 0.0:
				return "(-1, -1, -1)"

			tResDest = frame.getTimeActivityN(gxrpcframe.STEP_ASSIGN_DEST_CONN)	# if RPC forwarding
			if tResDest < 0.0:
				tResDest = frame.getTimeActivityN(gxrpcframe.STEP_ASSIGN_MONITOR)	# if regproc

			tDone = frame.getTimeActivityN(gxrpcframe.STEP_RECEIVING_RESULTS)	# if regular RPC
			if tDone < 0.0:
				tDone = frame.getTimeActivityN(gxrpcframe.STEP_REPLYING_DONE)	# if regproc or intercepted RPC
			if tDone < 0.0:
				tDone = frame.getTimeActivityN(gxrpcframe.STEP_RPC_CALL_COMPLETED)	# if failed 

			d1 = tParsed - tBegin
			if tResDest < 0.0:
				d2 = -1
			else:
				d2 = tResDest - tParsed
			if tDone < 0.0:
				d3 = -1
			else:
				d3 = tDone - tResDest
			ls = [d1,d2,d3]
			ls2 = []
			for d in ls:
				if d != -1:
					d = round((d * 1000.0), 1)	# convert sec to ms, and, round to 0.1 ms
				ls2.append(d)
			s = "(%1.1f, %1.1f, %1.1f)" % tuple(ls2)
			return s
		except:
			ex = gxutils.formatExceptionInfoAsString(8)
			m1 = "getMsDetails() got exception, frame: %s, exc: %s" % (frame, ex, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0,  m1)
			return "(-3, -1, -1)"

	def getUseridFdProcnameMs(self, clt_conn, frame):
		""" Called by:	writeEndRPClogMsg()
				writeRPCErrorMsg()

		    Return:
			tuple	(auditSlashUserid, fd, procName, ms)
		"""
		if (clt_conn != None):
			userid  = clt_conn.getUserId()
			fd 	= clt_conn.getSd()
		else:
			userid = "[clt_conn=None]"
			fd = -1

		procName = "[unknown--no frame]"
		ms	 = "??"
		auditUserId = ""
		userIdSep   = ""
		if (frame != None):
			procCall = frame.getProcCall()
			if (procCall.getPreviousName() != None):
				procName = procCall.getPreviousName()
			else:
				procName = frame.getProcName()

			ms = frame.getElapsedMs()

			p  = procCall.getParamByName(getNameOfParamWithAuditUserId() )
			if p:
				auditUserId = gxrpcframe.getValueOfParam(p)
				userIdSep = "/"

		auditSlashUserid = "%s%s%s" % (auditUserId, userIdSep, userid, )

		return (auditSlashUserid, fd, procName, ms)


	def sendRPCcallAndReceiveResponse(self, aThread, frame, clt_conn=None, b_doNotTellClient=0):
		#
		# class gxGenRPCAndRegProcMonitor
		#
		# SEND the RPC 
		# ------------
		#
		# NOTES: this is a generic method. Several sub-classes use it:
		#					gxRPCmonitor 
		#					gxClientLoginMonitor
		#					gxDestConnLoginMonitor
		#					gxServiceRPCmonitor
		#
		#	 The gxInterceptRPCMonitor sub-class overrides it. 
		#
		#
		#	 When the outgoing connection is broken, in most cases,
		#	 gxxmltpwriter.sendProcCall() will NOT detect this condition.
		#	 (So it cannot reconnect and retry the send).
		#	 Most often, this error condition will only be found when 
		#	 self.receiveResponse() does a blocking recv().
		#	 So, here, we need a loop that will allow one (1) retry
		#	 when that recv() finds that the connection is broken.
		#

		# First, we must check if we have waited too long in the 
		# queue of the DestConn pool:
		#
		t_beforeFirstSend = time.time()		# remember this time, because maybe delay in sendProcCall()

		destConn = aThread.getResource()

		if (self.checkIfMaxWaitInQueueAlreadyTimeout(frame, destConn) != 0):
			ret = gxreturnstatus.DS_RPC_TIMEOUT
			msg = "Timeout while waiting for DestConn (B)"
			if (b_doNotTellClient):
				clt_conn = None
			return (self.abortRPCandTellClient(frame, clt_conn, None, msg, ret) )

		retry_ctr = 0
		while (retry_ctr <= 1):

			# sendProcCall() does: frame.assignStep(gxrpcframe.STEP_RPC_CALL_SENT)
			#
			rc = gxxmltpwriter.sendProcCall(frame)
			if (rc != 0):
				raise RuntimeError, "gxxmltpwriter.sendProcCall(frame) FAILED" 

			#
			# Receive the results (and send them back to the client, if required):
			#
			aThread.changeState(gxthread.STATE_WAIT)	# wait for response


			rc = self.receiveResponse(aThread, frame, clt_conn, retry_ctr, t_beforeFirstSend)

			# is the destConn socket connection bad?
			# if not, don't retry, leave this loop:
			#
			if (-1 != rc):		
				break

			retry_ctr = retry_ctr + 1		# increment ctr, only ONE retry

			# Check if we have enough time left to retry sending the RPC...
			# If not enough time is left, do NOT retry sending the RPC.
			#
			nb_sec = self.calculateTimeoutForRecvResponse(destConn, t_beforeFirstSend)
			if (nb_sec <= 10):	# should have at least 10 seconds (???)
				msg = "Not enough time left to retry RPC (after reset destConn)(B)"
				ret = gxreturnstatus.DS_RPC_TIMEOUT
				if (b_doNotTellClient):
					clt_conn = None
				return (self.abortRPCandTellClient(frame, clt_conn, None, msg, ret) )

			#
			# End Loop
			#
		return (rc == 1)		# only tell if client conn disconnected


	def recvResp(self, aThread, frame, clt_conn, retry_ctr, t_beforeFirstSend, clt_sock):
		""" .recvResp() does recv() on the dest_conn socket and sends back to the client.
		"""
		#
		# Called by:	gxGenRPCAndRegProcMonitor.receiveResponse()	-- clt_sock is None
		#		gxRPCmonitor.receiveResponse()			-- clt_sock is a socket fd
		#
		# Return:	a Tuple: (b_eot, ret_stat, b_dest_err, statusMsg, b_clt_err, bytesSentCtr, bytesRecvCtr,)
		#		-1     : if dest_conn is disconnected
		#
		dest_conn = aThread.getResource()
		dest_sock = dest_conn.getData()

		prev_buff    = ""
		ret_stat     = -999
		b_eot        = 0
		b_dest_err   = 0
		b_clt_err    = 0
		bytesSentCtr = 0
		bytesRecvCtr = 0
		statusMsg    = ""
		full_resp = ""		# stay empty unless getDebugLevel() >= 55

		buf_size = RECV_BUFFER_SIZE
		frame.assignStep(gxrpcframe.STEP_WAIT_RESPONSE)

		while (not b_eot):
			try:
				# calculate the timeout for recv() of response...
				# (calculate again before each recv(), it becomes smaller 
				# with each iteration).
				#
				nb_sec = self.calculateTimeoutForRecvResponse(dest_conn, t_beforeFirstSend)
				if (nb_sec <= 3):	# less than 3 seconds is pathetic!
					dest_conn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)
					b_dest_err = 1
					statusMsg = "recv() Timeout Exhausted (F)"
					break		# abort loop

				timeout_val = struct.pack("ll", nb_sec, 0)

				dest_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout_val)

				try:
					rbuff = dest_sock.recv(buf_size)
				except:
					diag, excInfoTuple = gxutils.getDiagnosisAndExceptionInfo()
					if diag == gxutils.EXC_RECV_TIMEOUT:
						if (getDebugLevel() >= 10):
							m1 = "receiveResponse(): timeout exception on recv(). %s" % (frame, ) 
							gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
						continue

					excStr = gxutils.formatExceptionInfoAsString()

					dest_conn.assignBadState()
					b_dest_err = 1
					statusMsg = "recv() response FAILED (1). %s" % (excStr, )
					break

				blen  = len(rbuff)
				bytesRecvCtr = bytesRecvCtr + blen

				if (blen <= 0):
					dest_conn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)

					if (bytesRecvCtr == 0 and retry_ctr == 0):
						frame.rememberErrorMsg("recv() error, resetting conn,", "dest_conn.verifyDestConn(1)")
						rc = dest_conn.verifyDestConn(1)	# reset the destConn
						if (rc == 0):
							return -1				# will retry RPC

					b_dest_err = 1
					statusMsg = "recv() ERROR"
					break		# abort loop

				if (getDebugLevel() >= 55):
					full_resp = full_resp + rbuff

				frame.assignStep(gxrpcframe.STEP_RECEIVING_RESULTS)
			except:
				excStr = gxutils.formatExceptionInfoAsString()

				dest_conn.assignBadState()
				b_dest_err = 1
				statusMsg = "recv() response FAILED. %s" % (excStr, )
				break

			# concat previous and current buffers to make simple it to 
			# check if <EOT/> was received:
			#
			str = prev_buff + rbuff

			b_eot, ret_stat = self.checkEOTandReturnStatus(EOT_tag, str)

			if (b_eot):
				statusMsg = EOT_tag + " DONE."

			# if client has disconnected, don't try sending again,
			# but, continue receiving result
			#
			if (clt_sock != None and 0 == b_clt_err):
				try:
					aThread.changeState(gxthread.STATE_SENDING_BACK)

					nbytes = clt_sock.send(rbuff)	# send back rbuff, as received
					if (nbytes > 0):
						bytesSentCtr = bytesSentCtr + nbytes
					else:
						b_clt_err = 1
						statusMsg = "send() ERROR. n=%d" % (nbytes, )
				except:
					excStr = gxutils.formatExceptionInfoAsString()
					b_clt_err = 1
					statusMsg = "send() response to client FAILED. %s" % (excStr, )
					#
					# don't quit loop yet. Finish receiving results first!

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

		# You have to increase the debugLevel if you want to debug calls to the "login" regproc
		# and the check password RPC.
		#
		if (getDebugLevel() >= 55):
			gxServer.writeLog(MODULE_NAME, level, ret_stat, repr(full_resp) )

		return (b_eot, ret_stat, b_dest_err, statusMsg, b_clt_err, bytesSentCtr, bytesRecvCtr, )


	def receiveResponse(self, aThread, frame, clt_conn, retry_ctr, t_beforeFirstSend):
		#
		# Receive the response to a procCall.  
		#
		# Put the return status in the frame.
		# 
		# NOTE: this version does NOT intercept any row values.
		#
		dest_conn = aThread.getResource()

		begin_recv_tm = time.time()

		# .recvResp() does recv() on the dest_conn socket and sends back to the client.
		#
		tup = self.recvResp(aThread, frame, clt_conn, retry_ctr, t_beforeFirstSend, clt_sock=None)
		if tup == -1:
			return tup

		b_eot, ret_stat, b_dest_err, statusMsg, b_clt_err, bytesSentCtr, bytesRecvCtr = tup

		#
		# check if response was completely received:
		#
		level = TxLog.L_MSG

		if (not b_dest_err):
			if (b_eot):	# normal case, response received completely
				pass
			else:		# Strange... log a WARNING
				statusMsg = "WARNING: response complete but b_eot not set!" + statusMsg 
				level = TxLog.L_WARNING
		else:	#
			# error on recv(), b_dest_err is true...
			#
			level = TxLog.L_WARNING

			t_now = time.time()

			if ( (begin_recv_tm + 3) > t_now):
				statusMsg = "receive RPC response failed, conn broken? " + statusMsg 
				ret_stat = gxreturnstatus.DS_FAIL_CANNOT_RETRY
			else:
				statusMsg = "RPC reponse time out. " + statusMsg 
				ret_stat = gxreturnstatus.DS_RPC_TIMEOUT
				#
				# Mark the dest_conn as "dirty" (we do NOT know it's state
				# as for buffers on the server side, etc.)
				#
				dest_conn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)

		# Remember the return status:
		#
		frame.assignReturnStatus(ret_stat)

		# RPC completed does NOT mean retstat == 0
		#
		frame.assignStep(gxrpcframe.STEP_RPC_CALL_COMPLETED)

		# write RPC entry to log:
		#
		self.writeEndRPClogMsg(frame, clt_conn, ret_stat, level, statusMsg, 0)

		return 0	# assume "client" is OK, as we are not sending anything to it.


	def checkEOTandReturnStatus(self, EOT_tag, str):
		#
		# Called by:	.receiveResponse()
		#		gxRPCmonitor.receiveResponseAndSendBackToClient()
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

		if (getDebugLevel() >= 99):
			m1 = "checkEOTandReturnStatus(): looking for %s in '%s'."  % (EOT_tag, str)
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, len(str), m1)

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


	def logPreviousExceptionInfo(self, frame, actionDesc="exec"):
		if (getDebugLevel() >= 5 and frame.getPrevExcInfo() != None):
			m1 = "post %s %s: msg: %s, prev_exc: %s, exc: %s" % (actionDesc,
								frame.getProcName(), 
								frame.getErrMsg(), 
								frame.getPrevExcInfo(), 
								frame.getExceptionDetailsAsString(), )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)



class gxRegProcMonitor(gxGenRPCAndRegProcMonitor):
	#
	# for REG.PROCS
	# -------------
	#
	# NOTE: The real processing for registered procedures is in "gxregprocs.py".
	# 
	# 	The workhorse function is gxregprocs.execRegProc(aThread, frame, clt_conn)
	#
	#
	def needDestConn(self):
		#
		# override the superclass method
		#
		return (0)	# no, a RegProc does not use a destConn

	def execRegProc(self, aThread, frame, clt_conn):
		#
		# this method could be overriden by a subclass
		#

		# NOTE: The XMLTP parser stores all procCall parameters values as String,
		#	even if the datatype of a parameter is Int or Float.  We need to 
		#	convert those values now to float or int.
		#
		normalizeParamsOfProcCall(frame.getProcCall() )

		return (gxregprocs.execRegProc(aThread, frame, clt_conn) )

	def runThread(self, aThread):
		#
		# override the gxmonitor.gxmonitor superclass .runThread() method
		#
		# WARNING: There are only two (2) "return" points in this method
		#	   (the one implicit at the end and one "return" statement).
		#	   Be very careful to do proper clean-up if you add another one.
		#
		clt_conn = None
		frame	 = None		
		b_success = 0
		try:
			frame    = aThread.getFrame()
			clt_conn = frame.getClientConn()

			if (not self.checkIfClientStillConnected(clt_conn, frame) ):
				#
				# client has disconnected; release the resource, leave.
				#
				if (aThread.pool != None):
					aThread.pool.releaseResource(aThread.resource, b_useSuccessful=1)

				aThread.changeState(gxthread.STATE_COMPLETED)
				return None

			aThread.changeState(gxthread.STATE_ACTIVE)

			#
			# Call the registered procedure function:
			#
			rc = self.execRegProc(aThread, frame, clt_conn)

			if (rc == 0):	# if clt_conn still connected...
				#
				# the regproc "login" could have disconnected the clt_conn...
				#
				if (not clt_conn.isDisconnected() ):
					clt_conn.assignState(gxclientconn.STATE_RPC_CALL_ENDED_OK)
					clt_conn.rpcEndedReturnConnToPollList(1)

			frame.assignStep(gxrpcframe.STEP_REPLYING_DONE)

			# RPC completed does NOT mean retstat == 0
			#
			frame.assignStep(gxrpcframe.STEP_RPC_CALL_COMPLETED)

			# write RPC entry to log:
			#
			ret = frame.getReturnStatus()

			b_success = (ret == 0)

			if (getDebugLevel() >= 1):
				self.writeEndRPClogMsg(frame, clt_conn, ret, TxLog.L_MSG, None, rc)

			if (frame.getPrevExcInfo() != None):
				self.logPreviousExceptionInfo(frame, actionDesc="regProc")

		except:
			aThread.changeState(gxthread.STATE_ERROR)
			self.recoverRPCexecError(clt_conn, frame)
			
		if (aThread.pool != None):
			aThread.pool.releaseResource(aThread.resource, b_useSuccessful=b_success)

		aThread.changeState(gxthread.STATE_COMPLETED)


class gxRPCmonitor(gxGenRPCAndRegProcMonitor):
	#
	# RPC forwarding
	# --------------
	#
	# this thread monitor sends a ProcCall to a destConn,
	# then, receives the response and sends it back to the Client Connection
	# After that, it release the resource (the destConn).
	#
	def needDestConn(self):
		#
		# override the superclass method
		#
		return (1)	# yes, will need a destConn

	def runThread(self, aThread):
		#
		# override the gxmonitor.gxmonitor superclass .runThread() method
		#
		# WARNING: There are only two (2) "return" points in this method
		#	   (the one implicit at the end and one "return" statement).
		#	   Be very careful to do proper clean-up if you add another one.
		#
		clt_conn = None
		frame	 = None		
		b_success = 0
		try:
			frame    = aThread.getFrame()
			clt_conn = frame.getClientConn()

			if (not self.checkIfClientStillConnected(clt_conn, frame) ):
				#
				# client has disconnected; release the resource, leave.
				#
				if (aThread.pool != None):
					aThread.pool.releaseResource(aThread.resource, b_useSuccessful=1)

				aThread.changeState(gxthread.STATE_COMPLETED)
				return None

			aThread.changeState(gxthread.STATE_ACTIVE)

			normalizeParamsOfProcCall(frame.getProcCall() )		## convert params to internal representation

			if (g_fn_ValidateParams):
				rc = g_fn_ValidateParams(frame.getProcCall() )	## custom code may want to validate params

				if (rc != 0):
					frame.rememberErrorMsg("g_fn_ValidateParams()", ("Params not valid, rc=%s" % (rc, )) )
					frame.assignReturnStatusAndFinalDiagnosis(gxreturnstatus.DS_FAIL_CANNOT_RETRY,
							"audit param bad or missing")
					raise RuntimeError, ("Params not valid, custom funct rc=%s" % (rc, ))

			rc = aThread.resource.verifyDestConn()	# destconn must be connected and login
			if (rc != 0):
				frame.rememberErrorMsg("destConn.verifyDestConn()", ("FAILED rc=%s" % (rc, )) )
				frame.assignReturnStatusAndFinalDiagnosis(gxreturnstatus.DS_NOT_AVAIL,
							gxrpcframe.DS_SERVER_NOT_AVAILABLE)
				raise RuntimeError, ("DestConn cannot connect to destination, rc=%s" % (rc, ))

			rc = self.sendRPCcallAndReceiveResponse(aThread, frame, clt_conn)

			if (rc == 0):	# if clt_conn still connected...
				clt_conn.assignState(gxclientconn.STATE_RPC_CALL_ENDED_OK)
				clt_conn.rpcEndedReturnConnToPollList(1)

			frame.assignStep(gxrpcframe.STEP_REPLYING_DONE)

			if (frame.getPrevExcInfo() != None):
				self.logPreviousExceptionInfo(frame)

			b_success = (frame.getReturnStatus() == 0)

		except:
			aThread.changeState(gxthread.STATE_ERROR)
			self.recoverRPCexecError(clt_conn, frame)

		if (aThread.pool != None):
			markDestConnDirtyIfRPCfailedBad(frame, aThread.resource)
			aThread.pool.releaseResource(aThread.resource, b_useSuccessful=b_success)

		aThread.changeState(gxthread.STATE_COMPLETED)


	def receiveResponse(self, aThread, frame, clt_conn, retry_ctr, t_beforeFirstSend):
		#
		# *** OVERRIDE super-class method ***
		#
		# Called by:	<superclass>.sendRPCcallAndReceiveResponse() 
		#
		# Return:
		#	0	OK, normal
		#	1	clt_conn was or got disconnected
		#
		# send back every byte of the response until after <EOT/> is 
		# received.  Send back the "<EOT/>" also.
		#
		# Other purposes here:
		# 	* grab the return status in the response stream.
		#	  It is at the end of the stream:
		#	    ...<returnstatus> <int>-12345</int> </returnstatus>
		#	    </response><EOT/>
		#
		#	  This function here keeps a buffer of 70 bytes to search
		#	  what is just before <EOT/>.
		#
		clt_sock = clt_conn.getSocket()

		dest_conn = aThread.getResource()

		begin_recv_tm = time.time()
		userMsg	     = None	# tell to not put error message in response

		# .recvResp() does recv() on the dest_conn socket and sends back to the client.
		#
		tup = self.recvResp(aThread, frame, clt_conn, retry_ctr, t_beforeFirstSend, clt_sock)
		if tup == -1:
			return tup

		b_eot, ret_stat, b_dest_err, statusMsg, b_clt_err, bytesSentCtr, bytesRecvCtr = tup


		# check if re-transmission of response to client connection was done completely
		#
		# Cases table:			(1)	(2)	(3) 	 (4) 	 (5)
		#				normal  cltDisc dest err no resp netwk down
		#				------	-------	-------- ------- ----------
		#   Receive from dest_conn:	OK	OK	partial	 timeout broken
		#   Send back to client_conn:	OK	broken	OK	 OK?	 OK?
		#
		# Actions:
		#	(1) write the normal "End of RPC" message to the log
		#	(2) write warning about RPC completed and Client Disconnect;
		#	    call clt_conn.processDisconnect().
		#	(3) This happens if the dest_conn times out or disconnects.
		#	    This is the most difficult case. 
		#	    Maybe the XML-TP stream sent to the client is incomplete... 
		#	    ...the syntax will not parse!  How can we make it clean?
		#	    Break the client connection (disconnect). (Choice made in april 2002 with JB,DB)
		#
		#	(4) send a DS_RPC_TIMEOUT (-1101) return status response to the
		#	    client. 
		#	    Write RPC failed entry to the log.
		#	(5) This is strange: the RPC was sent OK, but there was an error
		#	    on recv. But it was not a timeout.
		#
		#	    Write RPC failed and warning to the log. 
		#
		level = TxLog.L_MSG
		case = 0
		b_send_ret_stat    = 0		# assume it's already done.
		b_must_disc_client = 0		# assume the XMLTP stream sent back was OK.

		if (not b_dest_err):
			if (b_clt_err):
				case = 2		# client disconnected
				b_must_disc_client = 1	# so, must disconnect client gxclientconn.

				# maybe something left on the connection..
				# mark it dirty:
				dest_conn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)
			else:
				case = 1		# normal case, response fully sent back
				if (bytesSentCtr != bytesRecvCtr):
					level = TxLog.L_WARNING
					statusMsg = "WARNING: recv=%s, sent=%s bytes!" % (bytesRecvCtr, bytesSentCtr)

		else:	# error on recv(), b_dest_err is true...
			#
			if (bytesSentCtr >= 1):
				userMsg = "RPC failed. See server log."

				case = 3		# sent back partial reponse!
				b_must_disc_client = 1	# so, must disconnect client gxclientconn.

				ret_stat = gxreturnstatus.DS_FAIL_CANNOT_RETRY
				level = TxLog.L_WARNING
				statusMsg = "PARTIAL response received! " + statusMsg

				# maybe something left on the connection..
				# mark it dirty:
				dest_conn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)
			else:
				b_send_ret_stat = 1		# ret_stat has NOT been sent to client yet,
								# we need to send it.
				t_now = time.time()
				if ( (begin_recv_tm + 3) > t_now):
					case = 5	# recv() got disconnect
					ret_stat = gxreturnstatus.DS_FAIL_CANNOT_RETRY
				else:
					case = 4	# recv() timeout
					ret_stat = gxreturnstatus.DS_RPC_TIMEOUT
					#
					# Mark the dest_conn as "dirty" (we do NOT know it's state
					# as for buffers on the server side, etc.)
					#
					dest_conn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)

		if (not b_eot and (case == 1 or case == 2) ):	# Strange... log a WARNING
			statusMsg = "WARNING: response complete but b_eot not set!" + statusMsg 
			level = TxLog.L_WARNING

		# Remember the return status:
		#
		frame.assignReturnStatus(ret_stat)


		# send return status to client if not done yet:
		#
		if (b_send_ret_stat):
			self.sendMessageAndReturnStatusToClient(frame, clt_conn, userMsg, ret_stat)

		# RPC completed does NOT mean retstat == 0
		#
		frame.assignStep(gxrpcframe.STEP_RPC_CALL_COMPLETED)

		# write RPC entry to log:
		#
		self.writeEndRPClogMsg(frame, clt_conn, ret_stat, level, statusMsg, b_clt_err)

		# if client has disconnected or a partial response has been sent, must, clean-up:
		#
		if (b_must_disc_client):
			clt_conn.processDisconnect(statusMsg)
			return (1)

		if (clt_conn.isDisconnected() ):	# could have been disconnected by sendMessageAndReturnStatusToClient()
			return (1)

		return (0)


class gxInterceptRPCMonitor(gxRPCmonitor):
	#
	# RPC forwarding with results Interception
	# --------------      --------------------
	#
	# this thread monitor sends a ProcCall to a destConn, then, while
	# it receives the response and sends it back to the Client Connection,
	# it also capture the response. After the RPC is done, it put some 
	# or all rows in a specific queue.
	# Finally, it releases the resource (the destConn).
	#
	# NOTES: Two (2) methods are inherited from the class gxRPCmonitor:
	# 
	#		def needDestConn(self):
	#		def runThread(self, aThread):
	#
	#	 The class here only overrides the sendRPCcallAndReceiveResponse()
	#	 method defined in gxRPCmonitor.
	# (2002feb25,jft)
	#

	def sendRPCcallAndReceiveResponse(self, aThread, frame, clt_conn, b_doNotTellClient=0):
		#
		# SEND the RPC 
		# ------------
		#
		# NOTE: argument b_doNotTellClient is NOT used in this 
		#	version of this method. This argument is here only to keep the
		#	method signature the same in its various implementations (two, so far).
		#	(jft,2002jul09;  2005sept30: removed b_isDestConnLogin)
		#
		t_beforeFirstSend = time.time()		# remember this time, because maybe delay in sendProcCall()

		destConn = aThread.getResource()

		if (self.checkIfMaxWaitInQueueAlreadyTimeout(frame, destConn) != 0):
			ret = gxreturnstatus.DS_RPC_TIMEOUT
			msg = "Timeout while waiting for DestConn"

			return (self.abortRPCandTellClient(frame, clt_conn, None, msg, ret) )

		rc = 0
		retry_ctr = 0
		while (retry_ctr <= 1):
			#
			# First, check if we have enough time left to try sending the RPC 
			# (there might have been a reset of the DestConn, including a slow login!).
			# If not enough time is left, do NOT send the RPC.
			#
			nb_sec = self.calculateTimeoutForRecvResponse(destConn, t_beforeFirstSend)
			if (nb_sec <= 15):	# should have at least 15 seconds (???)
				if (retry_ctr >= 1):
					msg = "Not enough time left to retry RPC (after reset destConn)"
				else:
					msg = "NOT ENOUGH time for sending RPC (first time)"
				ret = gxreturnstatus.DS_RPC_TIMEOUT

				return (self.abortRPCandTellClient(frame, clt_conn, None, msg, ret) )

			#
			# sendProcCall() does: frame.assignStep(gxrpcframe.STEP_RPC_CALL_SENT)
			#
			# NOTE: if the outgoing connection was bad,
			#	gxxmltpwriter.sendProcCall() will try to reconnect and retry to send
			#	one time.
			#
			rc = gxxmltpwriter.sendProcCall(frame)
			if (rc != 0):
				raise RuntimeError, "gxxmltpwriter.sendProcCall(frame) FAILED" 


			# calculate the timeout for recv() of response...
			# (calculate because sendProcCall might have lasted several
			# seconds)
			#
			nb_sec = self.calculateTimeoutForRecvResponse(destConn, t_beforeFirstSend, b_with_max=0)
			if (nb_sec <= 3):	# less than 3 seconds is pathetic!
				if (retry_ctr >= 1):
					msg = "Not enough time left to recv response (second try)"
				else:
					msg = "NOT ENOUGH time left to recv response (first time)"
				ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY

				return (self.abortRPCandTellClient(frame, clt_conn, destConn, msg, ret) )


			#
			# Receive the results and send them back to the client:
			#
			aThread.changeState(gxthread.STATE_WAIT)	# wait for response

			# 
			# when we call xmltp_gx.parseResponse(), we give it the clt_conn.getSd(),
			# (to let the results flow back to the client) and the flag b_capture=1
			# (to capture the results).
			#
			dest_sock = destConn.getData()
			dest_sd   = dest_sock.fileno()
			ctx	  = destConn.getData2()	# the context is also part of the DestConn resource

			if clt_conn:
				clt_sd = clt_conn.getSd()
			else:
				clt_sd = 0
			if (clt_sd < 0):
				clt_sd = 0

			frame.assignStep(gxrpcframe.STEP_WAIT_RESPONSE)

			resp = xmltp_gx.parseResponse(ctx, dest_sd, clt_sd, 1, nb_sec)	# 1: capture the whole response


			rc = self.processResponse(frame, clt_conn, resp, destConn)


			if (-1 == rc):			# is the destConn socket connection bad?
				if (retry_ctr == 0):
					retry_ctr = retry_ctr + 1
					frame.rememberErrorMsg("recv() error in xmltp_gx, resetting conn,", "destConn.verifyDestConn(1)")
					rc2 = destConn.verifyDestConn(1)	# reset the destConn
					if (rc2 != 0):
						ret = gxreturnstatus.DS_NOT_AVAIL
						msg = "destConn re-connect failed"
						return (self.abortRPCandTellClient(frame, clt_conn, None, msg, ret) )
				else:
					ret = gxreturnstatus.DS_NOT_AVAIL
					msg = "destConn disconnected (again)"
					return (self.abortRPCandTellClient(frame, clt_conn, None, msg, ret) )
			else:
				break		# for all other cases, leave the loop.

		return (rc == 1)		# only tell if client conn disconnected


	def processResponse(self, frame, clt_conn, resp, destConn):
		#
		# Look at the response, and log the RPC entry in the log.
		#
		# If response indicates some error, send a return status to
		# the client, or, disconnect the clt_conn.
		#
		# 
		# resp can be None, a string, or, a Tuple...
		#
		# If there was a major error, resp is a string.
		#
		# Normally, the response is a Tuple and it is built like this:
		#
		#    py_response = Py_BuildValue("(OOOiiiiiiisssssi)", 
		#				p_resp->result_sets_list,
		#			        py_params_list,
 		#			        p_resp->msg_list,
		#			        p_resp->total_rows,
		#			        p_resp->return_status,
		#			        b_found_eot,
		#			        client_errno,
		#			        rc,
		#			        ctr_err,
		#			        p_resp->ctr_manipulation_errors,
		#			        p_resp->old_data_map,
		#			        p_resp->resp_handle_msg,
		#				p_parser_msg,
		#				p_lexer_msg,
		#			        msg_buff 
		#				disc_timeout_data_mask  );
		#
		if (getDebugLevel() >= 250):
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, resp)	# NOT to be use in Prod


		# Initialize all local variables first:
		#
		ret_stat  = gxreturnstatus.NO_RETURN_STATUS_YET
		level     = TxLog.L_MSG
		statusMsg = "[no msg]"
		userMsg	  = None
		clt_errno = 0
		b_send_ret_stat       = 0	# normally, return status was already sent
		b_client_disconnected = 0	# assume client is still connected
		b_response_is_valid   = 0	# assume response is not valid

		try:
			tup = self.analyzeResponse(frame, clt_conn, resp, destConn)
			if (tup == -1):
				return -1	# indicate that higher level must retry RPC

			ret_stat, statusMsg, level, b_send_ret_stat, clt_errno = tup

			b_response_is_valid   = 1	# OK, response is valid

			if (clt_errno != 0):
				b_client_disconnected = 1
				b_send_ret_stat       = 0
		except:
			m1 = "Exc: %s" % (gxutils.formatExceptionInfoAsString(2), )
			level     = TxLog.L_ERROR
			statusMsg = "analyzeResponse() FAILED"

			self.writeRPCwarningMsg(statusMsg, frame, clt_conn, ret_stat, m1)


		if (clt_conn and b_response_is_valid and not b_send_ret_stat):
			frame.assignStep(gxrpcframe.STEP_REPLYING_DONE)


		# Remember the return status:
		#
		frame.assignReturnStatus(ret_stat)


		# Intercept the response, queue the rows (if required), and, 
		# write the "transaction id" to the log:
		#
		if (b_response_is_valid):
			self.interceptResponse(frame, clt_conn, resp)
		else:
			# The destConn is strange (?), mark it "dirty":
			# 
			destConn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)


		# Send return status to client if not done yet:
		#
		if (b_send_ret_stat):
			self.sendMessageAndReturnStatusToClient(frame, clt_conn, userMsg, ret_stat)


		# RPC completed does NOT mean retstat == 0
		#
		frame.assignStep(gxrpcframe.STEP_RPC_CALL_COMPLETED)

		# write RPC entry to log:
		#
		self.writeEndRPClogMsg(frame, clt_conn, ret_stat, level, statusMsg, clt_errno)


		# if client has disconnected, clean-up:
		#
		if (b_client_disconnected):
			# maybe something left on the connection..
			# mark it dirty:
			destConn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)

			if (clt_conn):
				clt_conn.processDisconnect(statusMsg)
			return (1)

		if (clt_conn and clt_conn.isDisconnected() ):	# could have been disconnected by sendMessageAndReturnStatusToClient()
			# maybe something left on the connection..
			# mark it dirty:
			destConn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)

			return (1)

		return (0)


	def analyzeResponse(self, frame, clt_conn, resp, destConn):
		#
		# Called by:	processResponse()
		#
		# Return:
		#	-1	 --- if destConn disconnect, without partial data
		#	(ret_stat, statusMsg, level, b_send_ret_stat, clt_errno)
		#
		#
		#    py_response = Py_BuildValue("(OOOiiiiiiisssssi)", 
		# 0				p_resp->result_sets_list,
		# 1			        py_params_list,
 		# 2			        p_resp->msg_list,
		# 3			        p_resp->total_rows,
		# 4 __________________________  p_resp->return_status,
		# 5			        b_found_eot,
		# 6			        client_errno,
		# 7			        rc,
		# 8 __________________________  ctr_err,
		# 9			        p_resp->ctr_manipulation_errors,
		# 10			        p_resp->old_data_map,
		# 11			        p_resp->resp_handle_msg,
		# 12				p_parser_msg,
		# 13				p_lexer_msg,
		# 14			        msg_buff,
		# 15				disc_timeout_data_mask),
		#

		# Give common values to all variable to be returned:
		#
		ret_stat  = gxreturnstatus.DS_FAIL_CANNOT_RETRY
		statusMsg = ""
		level     = TxLog.L_MSG
		clt_errno = 0
		b_send_ret_stat = 1	# we must send a return status to the client
		ctr_err_parser  = 0	# ctr errors while parsing
		ctr_err_manip   = 0	# ctr errors while manipulating response
		various_errmsg  = []

		if (resp == None):		# old behaviour of xmltp_gx.parseResponse()##
			ret_stat  = gxreturnstatus.DS_RPC_TIMEOUT
			statusMsg = "(resp == None)"
			level     = TxLog.L_MSG

			frame.assignStep(gxrpcframe.STEP_RECEIVING_RESULTS)

			return (ret_stat, statusMsg, level, b_send_ret_stat, clt_errno)


		if (type(resp) == type("str") ):
			ret_stat  =  gxreturnstatus.DS_FAIL_CANNOT_RETRY
			statusMsg = resp
			level     = TxLog.L_WARNING

			return (ret_stat, statusMsg, level, b_send_ret_stat, clt_errno)

		if (type(resp) != type( () ) ):
			ret_stat  =  gxreturnstatus.DS_FAIL_CANNOT_RETRY
			statusMsg = "INVALID response, not tuple, type is: %s" % (type(resp), )
			level     = TxLog.L_ERROR

			return (ret_stat, statusMsg, level, b_send_ret_stat, clt_errno)

		b_send_ret_stat = 0	# response already forwarded to client.
		try:
			ret_stat  = resp[4]
			clt_errno = resp[6]
			ctr_err_parser = resp[8]
			ctr_err_manip  = resp[9]
			disc_timeout_data = resp[15]	# bitmap about disconnect, timeout, data

			various_errmsg = resp[8:]	# from [8] to the last items
			#
			# old_data_map   = resp[10]
			# resp_handle_msg = resp[11]
			# parser_msg	= resp[12]
			# lexer_msg	= resp[13]
			# xmltp_gx_msg	= resp[14]
		except:
			statusMsg = "EXCEPTION in resp: %s" % (gxutils.formatExceptionInfoAsString(2), )
			level     = TxLog.L_ERROR

		# If there are any error(s), write all error msg to the log:
		#
		if (ctr_err_parser  or  ctr_err_manip):
			m1 = repr(various_errmsg)
			self.writeRPCwarningMsg("Errors in Response", frame, clt_conn, ret_stat, m1)
		#
		# disc_timeout_data is a bitmap:
		#	1 :	disconnect
		#	2 :	timeout (search "errno == EAGAIN" in xmltp_ctx.c for details)
		#	4 :	some bytes have been received on the DestConn
		#
		# The only bit combination that allows to do a retry (return -1) is
		# disconnect _without_ data. That's the bit for 0x01 (1).
		#
		if (disc_timeout_data == 1):
			if (getDebugLevel() >= 10):
				ret = gxreturnstatus.DS_NOT_AVAIL

				msg = "xmltp_gx.parseResponse() found disconnect on destConn"
				self.writeRPCwarningMsg(msg, frame, clt_conn, ret, "")

			return -1	# higher level can retry

		#
		# Check if response was received completely (no disconnect, no timeout):
		#
		if (disc_timeout_data == 0):
			return (ret_stat, statusMsg, level, b_send_ret_stat, clt_errno)

		#
		# if we come here, we had a disconnect or a timeout...
		#

		# Mark the destConn as "dirty" (we do NOT know it's state
		# as for buffers on the server side, etc.)
		#
		destConn.assignOutcomeOfUse(b_success=0, b_dirty=1, b_kill=1)


		if (disc_timeout_data & 1):
			ret_stat  = gxreturnstatus.DS_NOT_AVAIL
			statusMsg = "destConn disconnect"
		else:
			ret_stat  = gxreturnstatus.DS_RPC_TIMEOUT
			statusMsg = "recv() timeout"

		if (disc_timeout_data & 4):
			ret_stat =  gxreturnstatus.DS_FAIL_CANNOT_RETRY
			statusMsg = statusMsg + " with partial response already sent back!"
			level     = TxLog.L_WARNING

			self.disconnectClientBecauseOfPartialResponse(frame, clt_conn, destConn, statusMsg, ret_stat)
		else:
			b_send_ret_stat = 1	# we must send a return status to the client


		return (ret_stat, statusMsg, level, b_send_ret_stat, clt_errno)


	def interceptResponse(self, frame, clt_conn, resp):
		#
		# If there is an interceptor in the frame:
		#   Intercept the response, queue the rows (if required), and, 
		#   write the "transaction id" to the log.
		#
		#   NOTE: all the special work will be done by the interceptor
		#	   already held by the RPCframe.
		#
		# Else:
		#  grab some part(s) of the response...
		#
		interceptor = frame.getInterceptor()

		if (0 == interceptor):	# this means we should not intercept anything.
			return None

		if (interceptor):
			return (interceptor.captureResponse(frame, resp) )

		ls = frame.getTempInfo(TEMP_INFO_LIST_RESP_PARTS_TO_GRAB)
		if (not ls):
			msg = "interceptResponse() FAILED: no frame.getInterceptor() and no grab list."
			self.writeRPCwarningMsg(msg, frame, clt_conn, frame.getReturnStatus(), "")
			return None

		if (getDebugLevel() >= 20):
			m1 = "interceptResponse(): %s = %s." % (TEMP_INFO_LIST_RESP_PARTS_TO_GRAB, ls)
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		try:
			for tup in ls:
				partNum, tempInfoKey = tup

				frame.storeTempInfo(tempInfoKey, resp[partNum])
		except:
			m1 = "interceptResponse(): ls=%s, EXCEPTION: %s" % (ls, gxutils.formatExceptionInfoAsString(2), )
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0,  m1)

		return None


class gxClientLoginMonitor(gxGenRPCAndRegProcMonitor):
	#
	# ordinary "login" (regproc) is processed by send a AuthenticationRPC
	# ----------------
	#
	# NOTES:
	# After a client connect to the server, it will send a "login"
	# RPC (regproc!) call.
	#
	# If the userid parameter in the call is *not* an admin user,
	# then this class here will check the password and userid 
	# by calling a specific SP.
	#
	# The results of that SP call are not sent back directly to
	# the client conn, they are interpreted and a standard 
	# return status is send back.
	# Also, if the login is rejected, we will disconnect the client.
	#
	def needDestConn(self):
		#
		# override the superclass method
		#
		return (1)	# yes, will need a destConn


	def runThread(self, aThread):
		#
		# override the gxmonitor.gxmonitor superclass .runThread() method
		#
		# This runThread() method is different because:
		#	. the results of that SP call are not sent back directly to the clt_conn.
		#		they are interpreted and a standard response is sent back.
		#	. if the login fails, we disconnect the clt_conn.
		#
		# WARNING: There are only two (2) "return" points in this method
		#	   (the one implicit at the end and one "return" statement).
		#	   Be very careful to do proper clean-up if you add another one.
		#
		clt_conn = None
		frame	 = None		
		b_reject = 0
		b_abort  = 0
		ret	 = -997
		msg	 = None
		userid	 = None
		b_success = 0
		try:
			frame    = aThread.getFrame()
			clt_conn = frame.getClientConn()

			# get the userid now, we will need it for logging, no matter what happen.
			#
			procCall = frame.getProcCall()

			userid = gxrpcframe.getValueOfParam(procCall.getParamN(0) )

			# First, check some essential conditions...
			#
			if (not self.checkIfClientStillConnected(clt_conn, frame) ):
				#
				# client has disconnected, we should not continue.
				#
				b_abort = 1	# will release resource and return

			authRPCname = gxServer.getNameOfAuthenticationRPC()
			#
			# if the AuthenticationRPC parameter is not configured or it contains "@",
			# the server does not accept ordinary logins...
			#
			if (authRPCname == None  or authRPCname.find("@") >= 0):
				ret = -109
				msg = "This server is not configured to accept ordinary logins"
				b_reject = 1

			if (not (gxServer.getServerMode() & gxconst.MODE_PRIMARY) ):
				ret = -102 
				msg = "Logins are rejected. The server is not in Primary mode."
				b_reject = 1

			if (not b_abort and b_reject ):
				try:
					self.sendMessageAndReturnStatusToClient(frame, clt_conn, msg, ret)
					gxregprocs.writeLoginEntry(userid, clt_conn, (ret, msg) )
				except:
					pass

				clt_conn.processDisconnect(msg)

				b_abort = 1	# will release resource and return

			if (b_abort):
				# Should not proceed any further; release the resource, leave.
				#
				if (aThread.pool != None):
					aThread.pool.releaseResource(aThread.resource, b_useSuccessful=1)

				aThread.changeState(gxthread.STATE_COMPLETED)
				return None

			aThread.changeState(gxthread.STATE_ACTIVE)

			# Secondly, make sure the DestConn is good...
			#
			rc = aThread.resource.verifyDestConn()	# destconn must be connected and login
			if (rc != 0):
				frame.rememberErrorMsg("destConn.verifyDestConn()", ("FAILED rc=%s" % (rc, )) )

				frame.assignReturnStatusAndFinalDiagnosis(gxreturnstatus.DS_NOT_AVAIL,
							gxrpcframe.DS_SERVER_NOT_AVAILABLE)

				raise RuntimeError, ("DestConn cannot connect to destination, rc=%s" % (rc, ))

			# we have the AuthenticationRPC name, change it in the procCall...
			#
			rc = procCall.changeProcName(authRPCname)

			if (rc != 0):
				m1 = "login could not change procName to '%s', rc=%s" % (authRPCname, rc)
				raise RuntimeError, m1

			# Send the RPC with the new name, authRPCname:
			#
			self.sendRPCcallAndReceiveResponse(aThread, frame, b_doNotTellClient=1)

			# Verify the return status, assign the userid to the clt_conn, or, disconnect it.
			#
			ret = frame.getReturnStatus()

			if (ret == 0):
				ret, msg = gxregprocs.assignCltConnUserid(clt_conn, userid, 0)
				#
				# if the userid can be assigned properly, ret will stay == 0
				#
				b_success = 1
			else:
				gxregprocs.assignCltConnUserid(clt_conn, None, 0, userid)
				msg = "Login Failed"
				b_success = 0

			rc = self.sendMessageAndReturnStatusToClient(frame, clt_conn, msg, ret)

			gxregprocs.writeLoginEntry(userid, clt_conn, (ret, msg) )

			if (not clt_conn.isLoginOK() ):
				clt_conn.processDisconnect(msg)

			elif (rc == 0):		# if clt_conn still connected...
				clt_conn.assignState(gxclientconn.STATE_RPC_CALL_ENDED_OK)
				clt_conn.rpcEndedReturnConnToPollList(1)

			frame.assignStep(gxrpcframe.STEP_REPLYING_DONE)

			if (frame.getPrevExcInfo() != None):
				self.logPreviousExceptionInfo(frame, actionDesc="loginCheck")

		except:
			aThread.changeState(gxthread.STATE_ERROR)

			if (clt_conn  and  not clt_conn.isLoginOK() ):
				ret = -106
				msg = "Authentification procedure could not be completed"
				try:
					rc = self.sendMessageAndReturnStatusToClient(frame, clt_conn, msg, ret)

					clt_conn.processDisconnect(msg)
				except:
					pass
			try:
				# handle any further exception because self.recoverRPCexecError() is important
				#
				gxregprocs.writeLoginEntry(userid, clt_conn, (ret, msg) )
			except:
				pass

			self.recoverRPCexecError(clt_conn, frame, b_sendUserMsg=0)
			b_success = 0

		if (aThread.pool != None):
			markDestConnDirtyIfRPCfailedBad(frame, aThread.resource)
			aThread.pool.releaseResource(aThread.resource, b_useSuccessful=b_success)

		aThread.changeState(gxthread.STATE_COMPLETED)



class gxDestConnLoginMonitor(gxInterceptRPCMonitor):
	#
	# use "LOGIN" to login to the server to which the DestConn connects
	# ---------------------------------------------------------------------
	#
	# This monitor sends a ProcCall (usually "LOGIN")
	# to a destConn,
	# then, it receives the response and memorized if the login was
	# successful or not.
	#
	# USAGE:
	# 1) if required, prepare response parts to grab first, see interceptResponse()
	# 2) call:  mon.doLogin(aThread)
	#
	# After that, it release the resource (the destConn).
	#
	def needDestConn(self):
		#
		# override the superclass method
		#
		return (1)	# yes, will need a destConn

	def runThread(self, aThread):
		#
		# override the gxmonitor.gxmonitor superclass .runThread() method
		#
		raise RuntimeError, "Do NOT call gxDestConnLoginMonitor.runThread()"


	def doLogin(self, aThread, frame):
		#
		# this method should be called instead of .runThread()
		#
		# This method is different from runThread() as:
		#	. the frame argument is NOT the same one held normally by aThread
		#	. it does NOT call aThread.pool.releaseResource(aThread.resource)
		#	. it does NOT call aThread.changeState(gxthread.STATE_COMPLETED)
		#
		clt_conn = None		# NOTE: no client connection
		ret_stat = -997
		try:
			aThread.changeState(gxthread.STATE_ACTIVE)

			self.sendRPCcallAndReceiveResponse(aThread, frame, clt_conn=None, b_doNotTellClient=1)

			ret_stat = frame.getReturnStatus()
		except:
			if (frame != None):
				frame.assignStep(gxrpcframe.STEP_RPC_CALL_FAILED)
			
			self.recoverRPCexecError(clt_conn, frame)

		aThread.changeState(gxthread.STATE_WAIT)	# wait or what else?
		#
		# NOTE: the caller will do more things on this same thread.
		#
		return (ret_stat)




class gxServiceRPCmonitor(gxInterceptRPCMonitor):
	#
	# This type of RPC monitor is used by service threads to send a RPC.
	# -----------------------------------------------------------------
	#
	# NOTES:
	#
	# The results of that SP call are not sent back directly to
	# the client conn as it is a "pseudo client conn". 
	# 
	# Only the return status is interesting, Its value is stored in the RPC frame.
	#
	#
	def needDestConn(self):
		#
		# override the superclass method
		#
		return (1)	# yes, will need a destConn


	def runThread(self, aThread):
		#
		# override the gxmonitor.gxmonitor superclass .runThread() method
		#
		# This runThread() method is different because:
		#	. Mostly, the we ignore the results of that SP call. Only the
		#	  return status is stored in the RPC frame.
		#
		# WARNING: There are only two (2) "return" points in this method
		#	   (the one implicit at the end and one "return" statement).
		#	   Be very careful to do proper clean-up if you add another one.
		#
		clt_conn = None
		frame	 = None		
		b_success = 0

		try:
			frame    = aThread.getFrame()
			clt_conn = frame.getClientConn()

			# No need to check if client is still connected, it is always because
			# it is a service thread.

			aThread.changeState(gxthread.STATE_ACTIVE)

			# Make sure the DestConn is good...
			#
			rc = aThread.resource.verifyDestConn()	# destconn must be connected and login
			if (rc != 0):
				frame.rememberErrorMsg("destConn.verifyDestConn()", ("FAILED rc=%s" % (rc, )) )

				frame.assignReturnStatusAndFinalDiagnosis(gxreturnstatus.DS_NOT_AVAIL,
							gxrpcframe.DS_SERVER_NOT_AVAILABLE)

				raise RuntimeError, ("DestConn cannot connect to destination, rc=%s" % (rc, ))

			#
			# Some special service RPC might have to sleep() before being sent:
			#
			try:
				sleepBeforeCall = frame.sleepBeforeCall
			except:
				sleepBeforeCall = None

			if (sleepBeforeCall):
				if (getDebugLevel() >= 20):
					m1 = "gxServiceRPCmonitor.runThread(): about to sleep() %s seconds" % (sleepBeforeCall, )
					gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
				time.sleep(sleepBeforeCall)

			# Send the RPC...
			#
			self.sendRPCcallAndReceiveResponse(aThread, frame, clt_conn, b_doNotTellClient=1)


			clt_conn.assignState(gxclientconn.STATE_RPC_CALL_ENDED_OK)
			clt_conn.rpcEndedReturnConnToPollList(1)

			if (frame.getPrevExcInfo() != None):
				self.logPreviousExceptionInfo(frame, actionDesc="serviceRPC")

			b_success = (frame.getReturnStatus() == 0)
		except:
			aThread.changeState(gxthread.STATE_ERROR)
			try:
				frame.assignStep(gxrpcframe.STEP_RPC_CALL_FAILED)
			except:
				pass

			self.recoverRPCexecError(clt_conn, frame, b_sendUserMsg=0)	# do NOT send msg to client!

		if (aThread.pool != None):
			markDestConnDirtyIfRPCfailedBad(frame, aThread.resource)
			aThread.pool.releaseResource(aThread.resource, b_useSuccessful=b_success)

		aThread.changeState(gxthread.STATE_COMPLETED)




if __name__ == '__main__':

	m = mapProcnameToRPCmonitor("login")

	print m


	m = mapProcnameToRPCmonitor("spss_get_permissions")

	print m

	t1 = gxthread.gxthread("t1", monitor=m)
	t1.start()

	print t1
	time.sleep(10)
	print t1


