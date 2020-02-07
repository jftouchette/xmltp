#!/usr/bin/python

#
# gxrpcframe.py : Gateway-XML - RPC frame
# ---------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxrpcframe.py,v $
# $Header: /ext_hd/cvs/gx/gxrpcframe.py,v 1.4 2005/11/02 21:27:36 toucheje Exp $
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
# This module contains the definitions of (2) classes:
#	gxProcCall
#	gxRPCframe
#
# A "RPC frame" is a transient data structure that lasts for
# the timeframe of a RPC.
#
# Each incoming RPC request is associated with a gxRPCframe 
# instance.
#
# Each RPC frame is associated with one gxClientConn.
#
# Each gxClientConn is associated with one or zero gxRPCframe.
#
# Each gxThread that is not a service thread has a reference
# to a gxRPCframe
#
#
# -----------------------------------------------------------------------
#
# 2001nov21,jft: first version
# 2001dec28,jft: + self.prevExcInfo
#		 + getPrevExcInfo()
#		 + rememberPrevExcInfo()
# 2002jan29,jft: + added standard trace functions
# 2002feb10,jft: + asTuple()
#		 . clean-up stepsDict 
# 2002feb16,jft: + gxProcCall.changeProcName()
# 2002mar01,jft: + getValueOfParam(param): this public function used to be in gxregprocs.py
#		 + gxRPCframe: new attribute, interceptor
#		 + gxRPCframe.getInterceptor()
# 		 . class gxRPCframe is sub-classed from gxqueue.gxQueueItem to allow stats
# 2002mar12,jft: + gxRPCframe.assignReplyDone(self, val)
# 2002mar25,jft: . STEP_ASSIGN_PROCCALL	= 1.2  # changed from 2.x, so that it is distinct from ASSIGN_MONITOR step
#		 . use a few more distinct steps (from 0 to 13)
# 2002apr02,jft: + gxProcCall.getPreviousName(self)
#		 . gxProcCall.changeProcName(self, newName): memorize the previous name of the proc
# 2002apr07,jft: + gxProcCall.insertParam(self, param, pos=0): to support custom/legacy regprocs
# 2002apr12,jft: + gxRPCFrame.storeTempInfo(self, id, val)
#		 + gxRPCFrame.getTempInfo(self, id)
# 2002apr13,jft: + gxProcCall.getParamByName(self, name)
# 2002jul07,jft: . gxRPCFrame.assignStep(): can assign STEP_RPC_CALL_SENT or STEP_WAIT_RESPONSE more than once
# 2002aug25,jft: + gxRPCFrame.resetTimeBeginRPC(): used by Active Queue to avoid false timeout 
# 2002aug29,jft: . gxRPCFrame.resetTimeBeginRPC(): also calls self.assignStep(STEP_ASSIGN_PROCCALL, b_isReset=1)
# 2002sep11,jft: . use localtime() instead of gmtime()
# 2003jan17,jft: + buildDictOfValueFromParams(definitionsDict, paramsList, defaultValues=None)
# 2005sep29,jft: . gxRPCFrame.getElapsedMs(): return now - self.getTimeRPCBegan()
# 2005oct14,jft: + getParamName(param), getParamValueRaw(param), getParamDatatype(param), 
#		 + paramIsNull(param), paramIsOutput(param), getParamCharLength(param)
#


import time
import threading
import sys
import gxqueue
import gxutils
import gxreturnstatus

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxrpcframe.py,v 1.4 2005/11/02 21:27:36 toucheje Exp $"

#
# module Constants:
# ----------------

# CONSTANTS used with gxrpcframe.rememberException() :
#
TO_CLIENT	= 1
TO_DESTINATION 	= 2
PARSING		= 3
REGPROC_ACTION	= 4
INTERCEPT	= 5
INTERNAL	= 6


# Constants used with gxrpcframe.assignReturnStatusAndFinalDiagnosis() :
#
PROC_CALL_NOT_VALID	= "ProcCall not valid"
DS_SERVER_NOT_AVAILABLE	= "DS server not available"
CLIENT_DISCONNECTED	= "client disconnected"
DESTINATION_GATE_CLOSED	= "destination gate closed"
RPC_GATE_CLOSED		= "RPC gate closed"
CONNECT_GATE_CLOSED	= "Connect gate closed"
CONFIG_ERROR		= "Config error"


#
# the STEP_xxx constants are used as indice in the .eventsTable[] array
#
STEP_INIT		= 0
STEP_RESET_TIME_BEGIN_RPC = 0.1	# must only be used in .resetTimeBeginRPC()
STEP_WAIT_CALL_PARSER	= 1.0
STEP_ASSIGN_CONTEXT	= 2.0	# CallParser about to start
STEP_ASSIGN_PROCCALL	= 3.0	# proc call XML parsing completed
STEP_GET_MONITOR	= 4.0	# also check gates at that step
STEP_ASSIGN_MONITOR	= 5.0
STEP_WAIT_REGPROC_RES	= 6.0	# wait for RegProc resource OR DestConn
STEP_WAIT_DEST_CONN	= 6.1
STEP_ASSIGN_DEST_CONN	= 7.0
STEP_ASSIGN_THREAD	= 8.1
STEP_SENDING_RPC_CALL	= 8.2	# send RPC call
STEP_RPC_CALL_SENT	= 8.3	# RPC call sent
STEP_REGPROC_EXECUTING	= 8.4	#  OR execute RegProc
STEP_WAIT_RESPONSE	= 9
STEP_RECEIVING_RESULTS	= 10.0	# receive results from database and forward
STEP_SENDING_RESULTS	= 10.1	#  OR send them from an internal queue
STEP_REPLYING_DONE	= 12.0
STEP_REPLYING_DENY	= 12.1
STEP_REPLYING_TIMEOUT	= 12.2	# timeout on acquire resource or on wait response
STEP_REPLYING_FAILED	= 12.3	# failed due to internal error
STEP_RPC_CALL_COMPLETED = 13.0	# RPC from service thread, or, from client
STEP_RPC_CALL_FAILED    = 13.1	# indicates that an exception has been trapped in a mon.runThread()


stepsDict = { 	STEP_INIT		: "INIT",
		STEP_RESET_TIME_BEGIN_RPC: "RESET_TIME_BEGIN_RPC",
		STEP_WAIT_CALL_PARSER	: "WAIT_CALL_PARSER",
		STEP_ASSIGN_CONTEXT	: "ASSIGN_CONTEXT",
		STEP_ASSIGN_PROCCALL	: "ASSIGN_PROCCALL",
		STEP_GET_MONITOR	: "GET_MONITOR",
		STEP_ASSIGN_MONITOR	: "ASSIGN_MONITOR",
		STEP_WAIT_REGPROC_RES	: "WAIT_REGPROC_RES",
		STEP_WAIT_DEST_CONN	: "WAIT_DEST_CONN",
		STEP_ASSIGN_DEST_CONN	: "ASSIGN_DEST_CONN",
		STEP_ASSIGN_THREAD	: "ASSIGN_THREAD",
		STEP_SENDING_RPC_CALL	: "SENDING_RPC_CALL",
		STEP_RPC_CALL_SENT	: "RPC_CALL_SENT",
		STEP_REGPROC_EXECUTING	: "REGPROC_EXECUTING",
		STEP_WAIT_RESPONSE	: "WAIT_RESPONSE",
		STEP_RECEIVING_RESULTS	: "RECEIVING_RESULTS",
		STEP_SENDING_RESULTS	: "SENDING_RESULTS",
		STEP_REPLYING_DONE	: "REPLYING_DONE",
		STEP_REPLYING_DENY	: "REPLYING_DENY",
		STEP_REPLYING_TIMEOUT	: "REPLYING_TIMEOUT",
		STEP_REPLYING_FAILED	: "REPLYING_FAILED",
		STEP_RPC_CALL_COMPLETED : "RPC_COMPLETED",
		STEP_RPC_CALL_FAILED	: "RPC_FAILED",
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
	return (RCS_VERSION_ID)

#
# (End standard trace functions)
#

#
# module Functions -- PUBLIC functions:
#
def buildDictOfValueFromParams(definitionsDict, paramsList, defaultValues=None):
	#
	# Called by:	custom regprocs functions
	#
	# RETURN:
	#	Dict of paramName:value		if validation of params was successful
	#	error msg (String)		if validation failed 
	#	exception detected msg start with "**buildDictOfValueFromParams()..."
	#
	# STEPS:
	#	- create an empty dictionary: valuesDict
	#	- load the defaultValues in it (if any)
	#	- for each param in paramsList:
	#		. find it in defsDict
	#		. validates its datatype, isOutput
	#		. put its value in valuesDict
	#	- if len(definitionsDict) != len(valuesDict):
	#		find which param is missing, return error msg about this
	#	- return valuesDict
	#
	# NOTES: all parameter name comparisons (search) are done in lower case.
	#
	valuesDict = {}
	step = "load defaultValues"
	try:
		if defaultValues:
			for defaultValueEntry in defaultValues:
				name, val, jType, isNull, isOutput = defaultValueEntry
				valuesDict[name] = val

		step = "validating paramsList"
		for paramEntry in paramsList:
			step = "un-tuple paramEntry"
			name, val, jType, isNull, isOutput, sz = paramEntry

			name = name.lower()	## Convert param name to lower case ##

			validationEntry = definitionsDict.get(name, None)
			if (not validationEntry):
				msg = "param '%s' is not expected" % (name, )
				return msg

			step = "un-tuple validationEntry"
			v2, expectedType, expNull, expectedIsOutput = validationEntry

			step = "validate datatype and isOutput"
			if (expectedType != jType):
				msg = "param '%s' has type %s, should be %s" % (name, jType, expectedType)
				return msg
			if (expectedIsOutput != isOutput):
				msg = "param '%s' has isOutput=%s, should be %s" % (name, isOutput, expectedIsOutput)
				return msg

			if (isNull):
				val = None

			valuesDict[name] = val

		step = "validate number of params"
		if (len(definitionsDict) != len(valuesDict) ):
			if (len(definitionsDict) < len(valuesDict) ):
				msg = "ERROR: defaultValues names mismatch with definitionsDict??"
				return msg
			step = "finding missing param"
			keys = definitionsDict.keys()
			missing_lst = []
			for k in keys:
				k = k.lower()		## Convert expected param name to lower case ##
				try:
					v = valuesDict[k]
				except KeyError:
					missing_lst.append(k)
			if missing_lst:
				msg = "missing params: %s" % (missing_lst, )
				return msg

		return valuesDict	### validation successful ###
	except:
		ex = gxutils.formatExceptionInfoAsString(3)
		msg = "**buildDictOfValueFromParams() FAILED in step '%s', Exc: %s." % (step, ex)
		return msg		

#
# The following functions operates on a param tuple.
# Such a tuple is built this way:
#	('@param_name', value_as_string, datatype, isNull, isOutput, charLengthOrZero)
# Examples:
#	('@p_param_x', 'stringvalue', 12, 0, 0, 0)
#	('@p_null_char_example', ' ', 12, 1, 0, 0),
#	('@p_null_int_example', '0', 4, 1, 0, 0)
#
def getValueOfParam(param):
	if (param == None):
		return None

	if (type(param) != type( () )  or len(param) < 2):
		return (param)

	if (param[0] == '__return_status__'):	# this one is not an input parameter!
		return None

	return (param[1])

def getParamName(param):
	"""Return name part of param tuple.
	   If param is not a Tuple, an exception will occur.
	"""
	return param[0]

def getParamValueRaw(param):
	"""Return value part of param tuple.
	   If param is not a Tuple, an exception will occur.
	"""
	return param[1]

def getParamDatatype(param):
	"""Return datatype part of param tuple. The JAVA_SQL_xxx datatypes constants are defined in xmltp_tags.py
	   If param is not a Tuple, an exception will occur.
	"""
	return param[2]

def paramIsNull(param):
	"""Return True if isNull attribute is 1.
	   return False if isNull is not 1.
	   If param is not a Tuple, an exception will occur.
	"""
	return param[3] == 1

def paramIsOutput(param):
	"""Return True if isOutput attribute is 1.
	   return False if isOutput is not 1.
	   If param is not a Tuple, an exception will occur.
	"""
	return param[4] == 1

def getParamCharLength(param):
	"""Return charLength part of param tuple. NOTE: this is often '0' even though the lenght of the CHAR value is not zero.
	   If param is not a Tuple, an exception will occur.
	"""
	return param[5]

#
# Module functions:
#
def getStepDescription(step):
		try:
			return ( stepsDict[step] )
		except KeyError:
			return ( "=[unknown step %s]" % (step, ) )


def asTupleColNames():
	t = ("procName                 ", " step          ", "rpcTime     ", "rpcMs ", "frameMsg")
	return t

#
# Classes:
#
class gxRPCframe(gxqueue.gxQueueItem):
	#
	# Each incoming RPC request is associated with a gxRPCframe instance.
	# Such instances are defined here.
	#
	# NOTE: gxRPCframe is sub-classed from gxqueue.gxQueueItem specifically
	#	to allow various queues to compute stats.
	#
	def __init__(self, clientConn):

		gxqueue.gxQueueItem.__init__(self)	# superclass __init__()

		self.clientConn = clientConn

		self.b_replyDone = 0	# if True, reply has been sent to client

		self.prevExcInfo = None
		self.excMsg	= None
		self.resetExceptionInfo()
		self.returnStatus = gxreturnstatus.INIT_NOT_ASSIGNED_YET
		self.finalDiagnosis = None

		self.context	= None	# XML-TP parser context, will get one to parse the call
		self.procCall	= None	# will get one after parsing
		self.monitor	= None	# monitor which will execute the RPC call
		self.interceptor = None	# object which knows how to intercept the RPC call (or None)
		self.destConn	= None	# destConn from a Destination (connections pool)
		self.thread	= None	# execution thread, None when frame is in a queue

		self.tempInfoDict = None  # a Dictionary used to store temporary information (while the RPC lasts)

		self.msg	= ""
		self.msgTime	= None
		self.timeCreated = time.time()
		self.eventsTable = [ None, None, None, None,  
				     None, None, None, None,
				     None, None, None, None,
				     None, None, None, None,
				     None, None, None, None ]	 # 20 elements, 14 used
		self.assignStep(STEP_INIT)


	def __repr__(self):
		if (self.clientConn == None):
			cltUserid = "[NoClient]"
		else:
			cltUserid = self.clientConn.getUserId()
		step	  = self.describeStep()
		procName  = self.getProcName()
		return ("<%s %s %s %s %s ms>" % (cltUserid, procName, step,  
					   self.formatTimestamp(),
					   self.getElapsedMs()    ) )

	def asTuple(self, b_onlyColNames=0):
		if (b_onlyColNames):
			return (asTupleColNames() )

		#
		# ("procName", "step", "rpcTime", "rpcMs", "frameMsg")
		#
		step	  = "%15.15s" % (self.describeStep(), )
		procName  = "%25.50s" % (self.getProcName(),  )
		rpcTime   = self.formatTimestamp()
		rpcMs	  = "%3d ms" % (self.getElapsedMs(), )
		frameMsg  = self.getLastEventMsg()

		return (procName, step, rpcTime, rpcMs, frameMsg)


	def formatTimestamp(self):
		tm = self.getTimeLastActivity()
		i = int(tm)
		ms = tm - i
		lt = time.localtime(i)
		s1 = time.strftime("%H:%M:%S", lt)
		return ("%s.%03d" % (s1, ms * 1000) )

	def getLastEvent(self):
		n = int(self.step)
		if (n < 0 or n >= len(self.eventsTable) ):
			return (None)

		event = self.eventsTable[n]
		return (event)

	def getLastEventMsg(self):
		event = self.getLastEvent()

		if (event == None):
			return (None)

		step, tm, msg = event
		return (msg)


	def getTimeActivityN(self, n):
		n = int(n)
		if (n < 0 or n >= len(self.eventsTable) ):
			return (-1)

		event = self.eventsTable[n]
		if (event == None):
			return (-1)

		step, tm, msg = event
		return (tm)

	def getTimeLastActivity(self):
		tm = self.getTimeActivityN(self.step)
		if (tm == -1):
			return (time.time() )
		return (tm)

	def getTimeRPCBegan(self):
		tm = self.getTimeActivityN(0)
		if (tm == -1):
			return (self.timeCreated)
		return (tm)

	def getElapsedMs(self):
		## t_last  = self.getTimeLastActivity()
		t_last = time.time()
		t_begin = self.getTimeRPCBegan()
		return ( int( (t_last - t_begin) * 1000) )

	def describeStep(self):
		return getStepDescription(self.step)

	def resetTimeBeginRPC(self):
		#
		# Called by: (only!)	ActiveQueueMonitor.getOneRPCandSendIt() -- gxintercept.py
		#
		# @@@@ self.resetExceptionInfo()
		#
		self.assignStep(STEP_RESET_TIME_BEGIN_RPC, b_isReset=1)
		self.assignStep(STEP_ASSIGN_PROCCALL, b_isReset=1)	# also this one
		self.resetEvent(STEP_ASSIGN_MONITOR)
		self.resetEvent(STEP_ASSIGN_DEST_CONN)
		self.resetEvent(STEP_RPC_CALL_SENT)
		self.resetEvent(STEP_WAIT_RESPONSE)
		self.resetEvent(STEP_RECEIVING_RESULTS)
		self.resetEvent(STEP_RPC_CALL_COMPLETED)


	def resetEvent(self, step):
		#
		# Called by:	resetTimeBeginRPC()  -- Only --
		#
		i = int(step)	# some step values are 3.1, 4.1, convert to int!

		if (i < 0 or i >= len(self.eventsTable) ):
			return None

		self.eventsTable[i] = None



	def assignStep(self, step, msg=None, b_isReset=0):
		#
		# remember step and time when step was assigned.
		# Also remember msg if present.
		#
		if (STEP_REPLYING_DONE == step or STEP_REPLYING_DENY == step):
			self.assignReplyDone(1)

		self.step = step
		i = int(step)	# some step values are 3.1, 4.1, convert to int!

		if (self.eventsTable[i] != None):
			prevStep, t, m = self.eventsTable[i]
			if (not b_isReset and prevStep == step and step != STEP_RPC_CALL_SENT and step != STEP_WAIT_RESPONSE):
				return None	# do NOT re-assign exactly same
						# remember the time of first
		self.eventsTable[i] = (step, time.time(), msg )

	def getEventsList(self, b_withDescriptions=0):
		#
		# return a list of the various steps-events and timeStamps this RPCframe
		# has gone through.
		#
		# each element of the list is a tuple:
		#	if (b_withDescriptions), then each tuple is:
		#		 (stepDescriptionString, ms_duration, msg)
		#	else:
		#		 (step, timeStamp, msg)
		#
		# This list is useful for debugging, server snapshots or as diagnostic
		# information after a RPC has failed.
		#
		t_prev = self.getTimeRPCBegan()
		lst = []
		for event in self.eventsTable:
			if (event == None):
				continue
			if (b_withDescriptions):
				step, tm, msg = event
				ms = int( (tm - t_prev) * 1000)
				t_prev = tm
				lst.append( (getStepDescription(step), ms, msg) )
			else:
				lst.append(event)	# raw form
		return (lst)


	def assignReplyDone(self, val):
		#
		# Called by:	self.assignStep()
		#
		self.b_replyDone = val

	def isReplyDone(self):
		#
		# Called by:	gxxmltpwriter.replyErrorToClient()
		#
		return (self.b_replyDone)

	def assignContext(self, context):
		self.context = context
		self.assignStep(STEP_ASSIGN_CONTEXT)

	def assignProccall(self, procCall):
		self.procCall = gxProcCall(procCall)
		self.assignStep(STEP_ASSIGN_PROCCALL)

	def assignDestconn(self, destConn):
		self.destConn = destConn
		self.assignStep(STEP_ASSIGN_DEST_CONN)

	def assignMonitor(self, mon, interceptor=None):
		self.monitor = mon
		self.interceptor = interceptor
		self.assignStep(STEP_ASSIGN_MONITOR)

	def assignThread(self, thread):
		self.thread = thread
		self.assignStep(STEP_ASSIGN_THREAD)

	def getProcName(self):
		if (self.procCall == None):
			return None
		return self.procCall.getProcName()

	def getClientConn(self):
		return self.clientConn

	def getContext(self):
		return self.context

	def getProcCall(self):
		return self.procCall

	def getMonitor(self):
		return self.monitor

	def getInterceptor(self):
		return (self.interceptor)

	def getDestConn(self):
		return self.destConn

	def getThread(self):
		return self.thread

	def storeTempInfo(self, id, val):
		if (self.tempInfoDict == None):
			self.tempInfoDict = {}

		self.tempInfoDict[id] = val

	def getTempInfo(self, id):
		if (self.tempInfoDict == None):
			self.tempInfoDict = {}
		try:
			return (self.tempInfoDict[id] )
		except KeyError:
			return None

	def getErrMsg(self):
		return (self.msg)

	def getMsgTime(self):
		return (self.msgTime)

	def rememberErrorMsg(self, m1, m2):
		self.msg = "%s, %s" % (m1, m2)
		self.msgTime = time.time()

	def lastErrorIsException(self):
		if (self.excTime == None):
			return (0)
		if (self.msgTime == None):
			return (1)
		return (self.excTime > self.msgTime)


	def getPrevExcInfo(self):
		return (self.prevExcInfo)

	def rememberPrevExcInfo(self):
		if (self.excMsg == None):
			return None

		try:
			str = "%s, '%s', diag=%s, act=%s" % (self.excMsg, 
						repr( (self.excInfoTuple[0], self.excInfoTuple[1]) ),
						self.excDiag,
						self.excActivityType)
			self.prevExcInfo = str
		except:
			self.prevExcInfo = gxutils.formatExceptionInfoAsString(2)
		

	def resetExceptionInfo(self):
		if (self.excMsg != None):
			self.rememberPrevExcInfo()

		self.excMsg	= None
		self.excInfoTuple = None
		self.excDiag	= None
		self.excTime	= None
		self.excActivityType = None
		
	def getExceptionMsg(self):
		return self.excMsg

	def getExceptionInfo(self):
		return self.excInfoTuple

	def getExceptionDiag(self):
		return self.excDiag

	def getExceptionTime(self):
		return self.excTime

	def getExceptionActivityType(self):
		return self.excActivityType

	def getExceptionDetailsAsString(self):
		if (self.excMsg == None):
			return "[no exception yet]"

		str = "%s, '%s', diag=%s, act=%s" % (self.excMsg, 
					repr(self.excInfoTuple),
					self.excDiag,
					self.excActivityType)
		return (str)

	def rememberException(self, msg, activityType, tbLevel=5):
		#
		# the RPC frame only remember one exception at a time.
		# after an exception is memorized here, if the program
		# can recover, it should call gxrpcframe.resetExceptionInfo()
		# before trying any other actions.
		#
		if (self.excMsg != None):	# only remember one exception
			return None
		excInfoTuple = gxutils.formatExceptionInfo(tbLevel)
		diag = gxutils.getDiagnosisFromExcInfo(excInfoTuple)
		self.excMsg	= msg
		self.excInfoTuple = excInfoTuple
		self.excDiag	= diag
		self.excTime	= time.time()
		self.excActivityType = activityType

	def assignReturnStatus(self, val):
		self.returnStatus = val

	def getReturnStatus(self):
		return (self.returnStatus)

	def getFinalDiagnosis(self):
		return (self.finalDiagnosis)

	def getReturnStatusAndFinalDiagnosis(self):
		#
		# return a tuple with both
		#
		return (self.returnStatus, self.finalDiagnosis)

	def assignReturnStatusAndFinalDiagnosis(self, retStat, diag):
		#
		# this method should only be called once, 
		# and, as soon as a fatal definitive error condition
		# is known.
		#
		if (self.finalDiagnosis != None):
			return None
		self.finalDiagnosis = diag
		self.returnStatus = retStat


class gxProcCall:
	#
	# The constructor __init__() receices the argument procCall, a list, 
	# which is a list generated by the Proc Call Parser. This list
	# has (3) elements:
	#
	#	procCall[0]	procName
	#	procCall[1]	paramsList is a list of (n) parameters.
	#			each param[i] is a (5) elements tuple:
	#				name
	#				value
	#				type
	#				isNull
	#				isOutput
	#	procCall[2]	extra processingData that is returned by parser, 
	#			not used to send the RPC call.
	#	
	#	Example:
	#		('procNameXX', 
	#			[
	#			  ('__returnStatus__', '-9999', 4, 1, 1), 
	#			  ('@p_dbName', 'Master', 12, 0, 0)
	#			], 
	#		 0
	#		)
	#	
	#
	def __init__(self, procCall):
		self.procCall = procCall
		self.previousName = None
		self.paramsDict = None	# only built if getParamByName() is used


	def changeProcName(self, newName):
		if (None == self.procCall):
			return (-1)
		if (None == newName):
			return (-2)

		# a tuple does not allow to do: 
		#	self.procCall[0] = newName
		#
		# so we build a new one (using newName for element 0) 
		# and we replace the old one.
		#
		self.previousName = self.procCall[0]

		newProcCall = (newName, self.procCall[1], self.procCall[2] )

		self.procCall = newProcCall

		return (0)

	def getProcName(self):
		if (None == self.procCall):
			return None

		return (self.procCall[0] )

	def getPreviousName(self):
		return (self.previousName)

	def getParamN(self, n):
		if (None == self.procCall):
			return None

		ls = self.procCall[1]

		if (None == ls):
			return None

		if (n <= -1 or n >= len(ls) ):
			return None

		return (ls[n])

	def getParamsList(self):
		if (None == self.procCall):
			return None

		return (self.procCall[1])

	def buildParamsDict(self):
		if (self.paramsDict != None):	# if already built, return now
			return

		self.paramsDict = {}

		try:
			lsParams = self.getParamsList()
			if (not  lsParams):
				return		# empty list, empty dictionary

			for param in lsParams:
				name = param[0]
				self.paramsDict[name] = param
		except:
			pass	# we must NOT crash! But do what? failure will show as getParamByName() will return None


	def getParamByName(self, name):
		if (self.paramsDict == None):
			self.buildParamsDict()

		try:
			return self.paramsDict[name]
		except KeyError:
			return None

	def insertParam(self, param, pos=0):
		if (not self.procCall):
			return None

		self.procCall[1].insert(pos, param)


	def checkValidity(self):
		if (type(self.procCall) != type( () ) ):
			return "gxProcCall: self.procCall is not a tuple"

		if (len(self.procCall) < 3):		# size must be >= 3
			return "gxProcCall: len(self.procCall) = %d, must be >= 3" % (len(self. procCall), )

		return None	# means it is OK




if __name__ == '__main__':
	#
	# unit test script:
	#
	import xmltp_tags
	ls = [
		('@p_param_x', 'stringvalue', 12, 0, 0, 0),
		('@p_null_char_example', ' ', 12, 1, 0, 0),
		('@p_null_int_example', '0', 4, 1, 0, 0),
	    ]
	for param in ls:
		print "getParamName(param)      :", getParamName(param)
		print "getParamValueRaw(param)  :", getParamValueRaw(param)
		print "getParamDatatype(param)  :", getParamDatatype(param), xmltp_tags.getValueTagForType(getParamDatatype(param))
		print "paramIsNull(param)       :", paramIsNull(param)
		print "paramIsOutput(param)     :", paramIsOutput(param)
		print "getParamCharLength(param):", getParamCharLength(param)


	raise SystemExit

	import gxclientconn
	conn1 = gxclientconn.gxClientConn(None)	# socket = None, only for test
	conn2 = gxclientconn.gxClientConn(None)	# socket = None, only for test
	f1 = gxRPCframe(conn1)
	f2 = gxRPCframe(conn2)
	print "CONN: ", conn1
	print "FRAME:", f1
	try:
		conn1.assignRPCframe(f1)
	except RuntimeError, e:
		print "EXCEPTION:", e, conn1, f1

	conn1.assignState(gxclientconn.STATE_LOGIN_IN_PROGRESS, "dummyUserid_1")

	print "events list 1:", conn1.getEventsList()

	conn1.assignRPCframe(f1)

	print "events list 1:", conn1.getEventsList()

	print "getClientConnFromSd( -1 )", gxclientconn.getClientConnFromSd(-1)

###	conn1.assignRPCframe(None, "dummyRPC has failed (this is unit test)")

	print "events list:", conn1.getEventsList()

	conn2.assignUserid("USER_006")

	for clt in gxclientconn.getListOfClients():
		print clt


	f2.assignProccall(  ('procNameXX', [ ('__returnStatus__', '-9999', 4, 1, 1), 
		  			     ('@p_dbName', 'Master', 12, 0, 0)
					   ], 
				0 )  )

	print f2
	for x in f2.getEventsList(1):
		print x

	print f2.getEventsList(0)

	for clt in gxclientconn.getListOfClients():
		print clt, clt.getRPCframe()


