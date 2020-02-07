# !/usr/bin/python

#
# gxintercept.py : Gateway-XML INTERCEPT queues and custom intercept functions
# ---------------------------------------------
#
#
# $Source: /ext_hd/cvs/gx/gxintercept.py,v $
# $Header: /ext_hd/cvs/gx/gxintercept.py,v 1.24 2006-03-31 17:21:39 toucheje Exp $
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
# Intercept definitions, filters instances and Interceptors are all
# defined according to parameters from the config file.
#
# At server initialization time, initInterceptDefinitions() will create
# various  instances of InterceptDefinition, InterceptFilter, 
# InterceptQueue or InterceptFunction. 
# Also, initInterceptDefinitions() will load the dictionary
# dictProcNamesToInterceptDefinitions with procNames and map them to 
# various InterceptDefinition instances.
#
# Overview of the hierarchy of objects:
# ------------------------------------
#
#    dictProcNamesToInterceptDefinitions
#	contains:
#	   mapping of procNames to InterceptDefinition instances
#	   (this is the first level to know if a RPC must be intercepted or not).
#
#		an InterceptDefinition has a List of InterceptFilter(s). The first that 
#		 match with the params RPC will take care of the interception.
#
#		    an InterceptFilter has these attributes:
#			param		:	a tuple: (paramName, valueToMatch), or, None
#						if not None, the procCall must have a matching
#						(paramName, value) parameter, otherwise 
#						the proc is _not_ intercepted by this filter.
#
#			columns		:	a list of the required column names.
#						If the result set(s) don't have all these column names,
#						then it is _not_ intercepted.
#			trans_id	:	the name of the column whose value must be
#						 written to the log.
#			
#			interceptType	:	constant: QUEUE, QUEUE_SWITCH, ACTIVE_QUEUE, MULTI,
#						or CUSTOM_INTERCEPT
#
#			interceptor	:	an InterceptQueue or InterceptFunction or InterceptMultiQueues instance
#
#			an InterceptMultiQueues has:
#			   multiQueuesList	:	list of InterceptQueue
#
#			an InterceptFunction has:
#			   paramFunctionName	:	a string that can be mapped to...
#			   paramFunction	:	 a Python function
#			   paramConfArgs	:	arguments from the config, 
#							 to be passed to the function
#			   resultFunctionName	:	string
#			   resultFunction	:	function
#			   resultConfArgs	:	arguments from the config,
#							 to be passed to the function
#|			   columns (optional)	:	a list of the required column names.
#|						If the result set(s) don't have all these column names,
#|						then it is _not_ intercepted.
#
#|			an InterceptNotifyUDP has:
#|			   notifyUDPaddress	: ipAddress:port  or  hostname:service
#|			   columns (optional)	: a list of the required column names.
#|						If the result set(s) don't have all these column names,
#|						then it is _not_ intercepted.
#
#			an InterceptQueue has:
#			    queueMax	    :	maximum number of rows to be queued.
#						At the time of checking the proc parameters, if the
#						queue is already full, the RPC should be aborted,
#						the return status sent back to the client will
#						be GW_QUE_SIZE (-1202).
#
#			    maxRowsInGet    :	maximum number of rows to be returned 
#						 for one "get" regproc call.
#
#			    notifyUDPaddress:	ipAddress:port  or  hostname:service
#			    notifyInterval  :	number of seconds between UDP notify 
#						 when queue not empty
#			    logMarker	    :	string before the trans_id in the log if RPC succeeded
#			    logMarkerFailed :	string before the trans_id in the log if RPC failed
#			
#			an InterceptQueueSwitch has:
#				NOTE: the first queue that can match the columns names will be used.
#|			    queue_1	    :	a reference to a first InterceptQueue or InterceptFunction
#|			    queue_2	    :	a reference to a second InterceptQueue or InterceptFunction
#
# -----------------------------------------------------------------------
#
# 2002mar01,jft: first version
# 2002apr08,jft: . InterceptQueue.notifyGateway(): implementation completed
#		 + InterceptQueue.sendUDPcmdWithOptions(self, cmd_char, option_char, options_str)
# 2002apr11,jft: . RPCinterceptor.getCustomFunctionsParams(): get both self.paramConfArgs and self.resultConfArgs as list of values
#		 + <all Intercept... classes>.__repr__(self)
# 2002apr12,jft: . callCustomInterceptResponse(), processRows(): added resSetNo to arg list
# 2002apr14,jft: + support for ACTIVE_QUEUE
#		 + class ActiveQueueMonitor(gxmonitor.gxmonitor)
#		 + InterceptQueue.startActiveQueueMonitorThread()
#		 + InterceptQueue.notifyRPCcomplete(self, b_rpc_ok, msg)
#		 + InterceptQueue.waitForActiveRPCcompletion(self)
# 2002apr15,jft: + shutdownAndDumpAllQueues(b_quiet): shutdown and dump all queues
#		 + InterceptQueue.shutdownAndDump(self, b_quiet)
# 2002apr17,jft: . moved .normalizeRowValues(self, colNames, row) from InterceptQueue class to InterceptQueueSuperClass,
# 2002may13,jft; . InterceptQueue.buildDictColumnsAndCheckIfHasRequiredColumns(): support sz in colName
#		 . InterceptQueue.buildParamsListFromRowAndColNames(): if col tuple has sz, put it in param tuple also.
#		 . InterceptQueueSuperClass.normalizeRowValues(): support sz in col tuple
#		 . InterceptFilter.getInterceptorIfRPCparamsMatch(): support sz in param tuple
# 2002aug20,jft: . InterceptQueue.buildParamsListFromRowAndColNames(): prefixes "@p_" to name
#		 . getOneRPCandSendIt(): smarter, with retry when RPC fails
#		 + closeQueueAndWaitItIsReopenedByOperator()
# 2002aug21,jft: . normalizeRowValues(): calls gxrpcmonitors.unEscapeXML(val)
# 2002aug25,jft: . ActiveQueueMonitor.getOneRPCandSendIt(): if sleep(), then, calls frame.resetTimeBeginRPC() to avoid false timeout 
# 2002aug28,jft: . saveUnusableResponse(), saveUnusableResultSet(): simple but better versions than previous stubs
#		 . InterceptQueue.logTransId(): log only if self.trans_id (column name) or error or debug level >= 
# 2002jan24,jft: . use gxparam.GXPARAM_QUEUE_DUMP_DIR
# 2003mar05,jft: + added "kick gateway" daemon thread: + startKickGatewayDaemonThread()
#		 + assignEnableIntercept(b_enable)
#		 + getEnableInterceptStatus()
#		 . InterceptFilter.getInterceptorIfRPCparamsMatch(): return None if getEnableInterceptStatus() is False
# 2003jul04,jft: + getNamesOfAllQueues()
#		 + gxserver.getStatusAsRow(), gxserver.getStatusAsRowHeaders()
# 2004may26,jbl: + __init__ in class RPCinterceptor (variable used by an ouside class but not define)
# 2004jul16,jft: . classRPCinterceptor: moved __init__() after class comments
# 2004jul17,jft: + InterceptQueue.queueRpcParamsList(self, paramsList)
#		 + InterceptQueue.queueOneRow(self, colNames, row)
# 2004jul22,jft: + in class RPCinterceptor and, specifically, InterceptQueue, added support for 5 callback functions:
#			self.postGetqFunction
#			self.processDumpQueueFunction
#			self.postPutqFunction
#			self.postGetqActiveQueueFunction
#			self.postExecRpcFunction
# 2004jul23,jft: + in class RPCinterceptor, added field:   self.dataPath -- used by above 5 callbacks.
# 2004jul30,jft: + in class RPCinterceptor, added field:   self.extraDataList -- used by self.getExtraDataList()
# 2004aug01,jft: . ActiveQueueMonitor.getOneRPCandSendIt(): use result interceptor if item.getColNames() contains one.
#		 . ActiveQueueMonitor.getOneRPCandSendIt(): if ret >= 1, call write ERROR message in log and call self.logParamsOfRPCnotDone(procName, params)
#		 . InterceptQueue.queueRpcParamsList(): added optional arg, interceptor=None
# 2004aug08,jft: . InterceptQueue.processOneRow(): self.queueRpcParamsList(paramsList, interceptor=0) because we do not want another interception when the RPC is sent.
#		 . ActiveQueueMonitor.closeQueueAndWaitItIsReopenedByOperator(): write log message as TxLog.L_ERROR
# 2004aug09,jft: . InterceptQueue.queueOneRow(): call self.notifyGateway(1)
# 2004aug11,jft: . RPCinterceptor.getCustomFunctionsParams(): assign paramName = None at the beginning of try: block
#		 . InterceptQueue.initAttributes(): except: write exception traceback to log
# 2004aug15,jft: + in class InterceptFunction, added method: buildDictColumnsAndCheckIfHasRequiredColumns(self, colNames)
#		   See comments of this method for details.
# 2004aug17,jft: + InterceptFunction.__init__(): now does dictOfInterceptQueues[self.name.lower()] = self
# 2004aug18,jft: + InterceptQueueSwitch.__init__():  does dictOfInterceptQueues[self.name.lower()] = self
# 2004aug19,jft: . class RPCinterceptor: added default method .shutdownAndDump()
#		 . RPCinterceptor.getCustomFunctionsParams(): write values of callback functions names in the log.
# 2004aug25,jft: . convertToPythonTypeIfNotString(): try: newValue = int(val) except ValueError: newValue = long(val)
#		 . getStatusAsRowHeaders(): "name_______" instead of "name____"
# 2004sep01,deb: . Fixed incrementation of variable i in InterceptQueue.buildParamsListFromRowAndColNames()
# 2004oct05,jft: . ActiveQueueMonitor.getOneRPCandSendIt(): improved retry, now handles DS_RPC_TIMEOUT, DS_DEADLOCK, -6, OS_RPC_GATE_CLOSED
#		 + ActiveQueueMonitor.doProcCallWithRetry()
# 2004nov03,jft: . ActiveQueueMonitor.doProcCallWithRetry(): more resilient to SP errors, almost never close the queue
# 2004nov06,jft: . InterceptFunction.captureResponse(): if self.b_intercept_return_status: call self.callCustomInterceptResponse()
#		 . RPCinterceptor.getCustomFunctionsParams(): assign self.b_intercept_return_status
# 2005jan15,jft: . InterceptFunction.captureResponse(): if self.b_intercept_return_status AND NOT result_sets_list: rc = self.callCustomInterceptResponse(frame, -1, [], [1,], ret_stat)
# 2005feb09,jft: . InterceptQueue.getRowsAndColNamesList(): is now more robust and recovers when a row has a different number of columns from a previous row.
#		 + InterceptQueue.queueGetNowait()
#		 + InterceptQueue.getQueueQsize() 
# 2005feb10,jft: . InterceptQueue.getRowsAndColNamesList(): don't complain (no Error in log) if the lenght of colNames changes. It can be normal (usually, and, if not, the client side will complain).
# 2005sep30,jft: + class InterceptMultiQueues : MULTI interceptions 
# 2005nov17,jft: . ActiveQueueMonitor.doProcCallWithRetry(): RPC made by an Active Queue can be intercepted 
#		 . mapProcCallToInterceptor(): procCall arg can be tuple or gxrpcframe.gxProcCall 
#		 . InterceptFilter.getInterceptorIfRPCparamsMatch(): procCall arg can be tuple or gxrpcframe.gxProcCall
#		 . ActiveQueueMonitor.doProcCallWithRetry(): call mapProcCallToInterceptor() if interceptor is None
# 2005nov18,jft: . moved buildDictColumnsAndCheckIfHasRequiredColumns() from InterceptQueue to InterceptQueueSuperClass to allow InterceptFunction to call it
#		 . an InterceptFunction can have optional <interceptor>_COLUMNS config param.  
#		   This allow an InterceptFunction to be used properly in an InterceptQueueSwitch.
# 2005dec14,jft: + support for interception type NOTIFY_UDP (class InterceptNotifyUDP)
#		 . moved methods .prepareUDPsocketToGateway(), .sendUDPcmdWithOptions() and .notifyGateway()
#		    from class InterceptQueue to InterceptQueueSuperClass
#		 . InterceptDefinition.mapProcNames(): dictProcNamesToInterceptDefinitions[val.upper()] // [val.lower()] = self 
# 2006jan31,jft: + support for queue_STORAGE PERSISTENT
#		 . InterceptQueue.shutdownAndDump(): if self.b_active: self.b_finalStop = 1    # tell the ActiveQueueMonitor thread to stop
#		 . shutdownAndDumpAllQueues(): g_KickGatewaysDaemon_stopFlag = 1
# 2006mar24,jft: . InterceptQueue.__init__(): write log before creating gxbdbqueue.gxbdbqueue instance
#

import time
import sys
import thread		# for thread.allocate_lock()

import gxbdbqueue	# for gxbdbqueue.gxbdbqueue class
import gxqueue		# for gxqueue.gxqueue class
import gxthread
import gxrpcframe
import gxclientconn	# for PseudoClientConn class
import gxmonitor
import gxrpcmonitors
import gxutils
import gxreturnstatus
import gxparam		# for GXPARAM_INTERCEPT_DEFINITIONS, ...
import igw_udp
import xmltp_tags
import TxLog

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxintercept.py,v 1.24 2006-03-31 17:21:39 toucheje Exp $"

#
# Module Constants:
#
DEFAULT_DEBUG_LEVEL = 5
MODULE_NAME	    = "gxintercept"

# Dictionaries g_dictDropRetValues and g_dictRetryRetValues
# are used by ActiveQueueMonitor.doProcCallWithRetry()
#
g_dictDropRetValues = {	gxreturnstatus.SP_EVENT_NB_DUPLICATE:1,
			gxreturnstatus.SP_EVENT_NB_BAD_SEQ:1,
			gxreturnstatus.SP_FAILED:1,
			gxreturnstatus.DS_FAIL_CANNOT_RETRY:1,
			gxreturnstatus.DS_EXECUTION_FAILED:1,
		      }
g_dictRetryRetValues = { gxreturnstatus.OS_RPC_GATE_CLOSED:1,
			gxreturnstatus.OS_RPC_NAME_BLOCKED:1,
			gxreturnstatus.OS_RESOURCES_ERROR:1,
			gxreturnstatus.OS_EXEC_INIT_FAILED:1,
			gxreturnstatus.GW_RPC_TIMEOUT:1,
			gxreturnstatus.GW_QUE_SIZE:1,
			gxreturnstatus.OS_NOT_AVAIL:1,
			gxreturnstatus.GW_IN_QUEUE_FULL:1,
			gxreturnstatus.REJECTED_WHEN_INACTIVE:1,
			gxreturnstatus.REJECTED_WHEN_PRIMARY:1,
			gxreturnstatus.REJECTED_WHEN_SECONDARY:1,
		      }


#
# module variables:
#
gxServer = None



# dictProcNamesToInterceptDefinitions maps procNames to InterceptDefinition objects:
#
dictProcNamesToInterceptDefinitions = {}


# Dictionary of all InterceptQueue(s), by their names:
#
dictOfInterceptQueues = {}


#
# funcNameToCustomFuncDict  maps function names to custom functions
#
funcNameToCustomFuncDict = {}	# empty by default. Can be loaded by subclass of gxserver.

#
# module level boolean to enable or disable interceptions:
#
s_b_enable_intercept = 1	# the default value is True


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
# KickGatewaysDaemonMonitor: monitor of the "kick gateways" daemon thread
#
g_KickGatewaysDaemon_stopFlag = 0

class KickGatewaysDaemonMonitor(gxmonitor.gxmonitor):
	
	def runThread(self, aThread):
		ctrErrors = 0

		ls = getListOfAllQueues()
		if not ls:
			m1 = "There are no queue in '%s'. KickGatewaysDaemonMonitor is stopping." % (MODULE_NAME, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			aThread.changeState(gxthread.STATE_SHUTDOWN)
			return

		while (ctrErrors < 50):
			try:
				aThread.changeState(gxthread.STATE_WAIT)
				time.sleep(5)	# sleep 5 seconds before next "kick"

				if g_KickGatewaysDaemon_stopFlag:
					gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "g_KickGatewaysDaemon_stopFlag is set. Quiting.")
					return

				self.kickGatewaysOneTime(aThread)
			except:
				ex = gxutils.formatExceptionInfoAsString(5)
				m1 = "KickGatewaysDaemonMonitor: .kickGatewaysOneTime() got exception: %s" % (ex, )
				gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )
				ctrErrors = ctrErrors + 1

		m1 = "KickGatewaysDaemonMonitor: Too many exceptions, aborting."
		gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1 )

		aThread.changeState(gxthread.STATE_ABORTED)
		sys.exit()

	def kickGatewaysOneTime(self, aThread):
		aThread.changeState(gxthread.STATE_ACTIVE)
		ls = getListOfAllQueues()
		ctr = 0
		for q in ls:
			if (isinstance(q, InterceptQueue) and (not q.b_active) and q.getQueueQsize() > 0):
				nbRows = q.getQueueQsize()
				q.notifyGateway(nbRows)
				ctr = ctr + 1

		if (getDebugLevel() >= 10):
			m1 = "re-notified %s gateways from a list of %s queues" % (ctr, len(ls), )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

#
# Module functions:
#
def assignEnableIntercept(b_enable):
	global s_b_enable_intercept
	s_b_enable_intercept = b_enable

def getEnableInterceptStatus():
	return s_b_enable_intercept


def getQueueByName(name):
	try:
		return (dictOfInterceptQueues[name])
	except KeyError:
		return None

def getListOfAllQueues():
	return dictOfInterceptQueues.values()

def getNamesOfAllQueues():
	return dictOfInterceptQueues.keys()

def getNamesOfAllPersistentQueues():
	ls = []
	for qname in getNamesOfAllQueues():
		q = dictOfInterceptQueues.get(qname, None)
		if q and q.b_persistent:
			ls.append(qname)
	return ls

def getListOfAllPersistentQueues():
	ls = []
	for q in getListOfAllQueues():
		if q.b_persistent:
			ls.append(q)
	return ls

def startKickGatewayDaemonThread():
	#
	# Called by:	gxserver.startUp()
	#
	mon = KickGatewaysDaemonMonitor("KickGatewaysMon", gxServer)
	thr = gxthread.gxthread("KickGatewaysDaemon", monitor=mon, threadType=gxthread.THREADTYPE_SERVICE)
	thr.start()

	m1 = "KickGatewaysDaemon thread started: %s." % (repr(thr), )
	gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


def shutdownAndDumpAllQueues(b_quiet):
	#
	# Called by:	gxserver.shutdownFinal()
	#
	#  shutdown and dump all queues
	#
	global g_KickGatewaysDaemon_stopFlag

	if (getDebugLevel() >= 1  or  not b_quiet):
		m1 = "about to shutdown and dump %s intercept queues..." % (len(dictOfInterceptQueues) )
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	# stop the KickGatewaysDaemon:
	#
	g_KickGatewaysDaemon_stopFlag = 1

	ctrDone = 0
	try:
		for queue in dictOfInterceptQueues.values():
			queue.shutdownAndDump(b_quiet)
			ctrDone = ctrDone + 1

		if (getDebugLevel() >= 1  or  not b_quiet):
			m1 = "shutdown and dump %s/%s intercept queues: DONE." % (ctrDone, len(dictOfInterceptQueues) )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
	except:
		ex = gxutils.formatExceptionInfoAsString(10)
		m1 = "shutdownAndDumpAllQueues(): FAILED after %s queues, Exc: %s." % (ctrDone, ex, )
		gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )

	# close the storage for all persistent queues (if any):
	#
	try:
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Calling gxbdbqueue.shutdownBDBenvironment()...")

		rc = gxbdbqueue.shutdownBDBenvironment()
		if rc == -1:
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Calling gxbdbqueue.shutdownBDBenvironment(b_force=1)...")
			rc = gxbdbqueue.shutdownBDBenvironment(b_force=1)

		if rc <= -1:
			m1 = "gxbdbqueue.shutdownBDBenvironment() FAILED, rc=%s." % (rc, )
			level = TxLog.L_ERROR
		else:
			m1 = "gxbdbqueue.shutdownBDBenvironment() completed successfully, rc=%s." % (rc, )
 			level = TxLog.L_MSG
		gxServer.writeLog(MODULE_NAME, level, 0, m1)
	except:
		ex = gxutils.formatExceptionInfoAsString(10)
		m1 = "shutdownAndDumpAllQueues(): FAILED calling gxbdbqueue.shutdownBDBenvironment(), Exc: %s." % (ex, )
		gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )


def mapProcCallToInterceptor(procCall):
	#
	# Called by:	gxrpcmonitors.mapProcnameToRPCmonitor()
	#
	# Return:
	#	a RPCinterceptor subclass instance
	# or
	#	a tuple: (ret, msg) indicating why the queue/gate is closed or full
	#
	if type(procCall) == type( () ):
		procName = procCall[0]
	else:
		procName = procCall.getProcName()

	if (getDebugLevel() >= 30):
		m1 = "mapProcCallToInterceptor(): checking if '%s' should be intercepted." % (procName, )
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	interceptDef = mapProcNameToInterceptDefinition(procName)

	if (interceptDef == None):
		return (None)

	if (getDebugLevel() >= 10):
		m1 = "mapProcCallToInterceptor(): '%s' could be intercepted by '%s'. Checking %s filters." % (procName, interceptDef.name, len(interceptDef.filters))
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	# let the interceptDef.filters inspect the param names, values...
	#
	for interceptFilter in interceptDef.filters:
		interceptor = interceptFilter.getInterceptorIfRPCparamsMatch(procCall)
		if (interceptor != None):
			return (interceptor)	# could also be a (ret, msg) tuple

	return None



def mapProcNameToInterceptDefinition(procName):
	#
	# Called by:	mapProcCallToInterceptor()
	#
	try:
		return (dictProcNamesToInterceptDefinitions[procName] )

	except KeyError:
		return None



def initInterceptDefinitions():
	#
	# Called by:	gxserver.initialize()
	#
	# This function initialize the intercept definitions from the config file parameters:
	#
	val = gxServer.getOptionalParam(gxparam.GXPARAM_INTERCEPT_DEFINITIONS)

	if (val == None):
		m1 = "param '%s' not found. This server will NOT do any RPC interception." % (gxparam.GXPARAM_INTERCEPT_DEFINITIONS, )
		gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
		return (0)

	while (val != None):
		try:
			definition = InterceptDefinition(val)
		except:
			m1 = "Failed creating InterceptDefinition '%s'. See log entries above." % (val, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)
			return (-1)

		if (definition.mapProcNames() <= -1):
			return (-1)

		val = gxServer.getParamNextValue(gxparam.GXPARAM_INTERCEPT_DEFINITIONS)

	if (getDebugLevel() >= 1):
		allProcNames = dictProcNamesToInterceptDefinitions.keys()
		allProcNames.sort()
		mapLs = []
		for proc in allProcNames:
			idef = dictProcNamesToInterceptDefinitions[proc]
			tup = (proc, idef.name)
			mapLs.append(tup)
		m1 = "initInterceptDefinitions(): %d procs mapped: %s." % (len(dictProcNamesToInterceptDefinitions), mapLs)
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	return (0)


#
# module functions used to bind Custom Intercept Functions:
#
def getCustomFunctionForFuncNameX(x):
	#
	# Called by:	InterceptFunction.initAttributes()
	#
	if not x:
		return None

	try:
		return (funcNameToCustomFuncDict[x] )
	except KeyError:
		m1 = "getCustomFunctionForFuncNameX(): no function bound to name '%s' (Error in config file?)." % (x, )
		gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
		return None


def addCustomFunctionToCustomFuncDict(funcName, funcRef):
	#
	# Called by:	by sub-class of gxserver, when self.initializeCustom()
	#		is called in .initialize().
	#
	# The arguments of the custom result function are:
	#
	#	func(interceptor, frame, result_sets_list, ret_stat, configArgsValuesList)
	#
	# NOTES: the argument configArgsValuesList is the values list from the config
	#	 parameter  xxx_RESULT_FUNCTION_ARGS
	#
	funcNameToCustomFuncDict[funcName] = funcRef


def getListOfCustomFunctions():
	ls = funcNameToCustomFuncDict.keys()
	ls.sort()
	return ls



#
# Private Module functions:
#
def convertToPythonTypeIfNotString(val, datatype, isNull):
	#
	# Called by:	InterceptQueueSuperClass.normalizeRowValues()
	#		InterceptFilter.getInterceptorIfRPCparamsMatch()
	#
	# NOTE: convertToPythonTypeIfNotString() returns None is val does not
	#	require to be converted.
	#
	newValue = None
	if (xmltp_tags.getValueTagForType(datatype) == xmltp_tags.XMLTP_TAG_INT):
		if (isNull):
			newValue = 0
		else:
			try:
				newValue = int(val)
			except ValueError:
				newValue = long(val)
	elif (xmltp_tags.getValueTagForType(datatype) == xmltp_tags.XMLTP_TAG_NUM):
		if (isNull):
			newValue = 0.0
		else:
			newValue = float(val)
	return (newValue)

def saveUnusableResponse(frame, resp):
	#
	# Called by:	<interceptor>.captureResponse()
	#
	try:
		m1 = "Unusable response! frame: %s, resp: %s" % (frame, resp)
	except:
		m1 = "Unable to format and log unusable response"
	gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

def saveUnusableResultSet(frame, resultSet):
	#
	# Called by:	InterceptQueue.addResultSetToQueue()
	#
	try:
		m1 = "Unusable result set! frame: %s, resultSet: %s" % (frame, resultSet)
	except:
		m1 = "Unable to format and log unusable resultSet"
	gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)




#
# Several classes...
#
# The ACTIVE QUEUE monitor:
#
class ActiveQueueMonitor(gxmonitor.gxmonitor):
	
	def runThread(self, aThread):
		#
		#
		b_fatalError = 0

		while(not self.master.b_finalStop):
			try:
				self.getOneRPCandSendIt(aThread)
			except:
				ex = gxutils.formatExceptionInfoAsString(10)
				m1 = "Exception in getOneRPCandSendIt(): Exc: %s." % (ex, )
				gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )

				self.ctrErrors = self.ctrErrors + 1

			if (self.ctrErrors >= 20):
				b_fatalError = 1
				break

		if (b_fatalError):
			m1 = "ERROR: too many errors, ActiveQueueMonitor() loop ABORTING. %s, %s" % (self, aThread)
			gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )

			aThread.changeState(gxthread.STATE_ABORTED)

		aThread.changeState(gxthread.STATE_SHUTDOWN)
		sys.exit()

	def getOneRPCandSendIt(self, aThread):
		#
		# STEPS:
		#	get an item from the queue (wait until one is available...)
		#	get a temporary frame
		#	SEND the RPC, with various retries if it fails...
		#
		aThread.changeState(gxthread.STATE_PREPARE)

		clt_conn = self.master.pseudoClientConn

		if (getDebugLevel() >= 5):
			m1 = "%s -- waiting for a queue item (RPC to send)..." % (self.master.getName(), )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		aThread.changeState(gxthread.STATE_WAIT)

		#
		# Wait for an item to become available in the queue...
		#
		item = self.master.getItemRaw_withWait()

		if (item == 0):
			m1 = "%s -- queue is closed for get(). Waiting until it open again..." % (self.master.getName(), )
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

			ctr = 0
			while (not self.master.isOpen(forWhat=1) ):
				time.sleep(5)		# check every 5 seconds...
				ctr = ctr + 1
				if (ctr > 120):		# get out of this loop after 10 min = 600 s = (5 * 120 sec)
					break
			return 0

		aThread.changeState(gxthread.STATE_ACTIVE)

		if (self.master.b_finalStop):	# we do NOT send the RPC if we see this flag!
			if (item != None):
				stuff = "[item.getRow() failed]"
				try:
					stuff = item.getRow()
				except:  pass
			else:
				stuff = None
			m1 = "%s -- Found b_finalStop! Dropping item: %s." % (self.master.getName(), stuff)
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
			return 1

		# Now, we have a queue item, and it contains the parameters list for the procCall.
		#
		# We also need the name of the proc...
		#
		procName = self.master.rpc_name

		params = item.getRow()		# contains standard procCall params list

		interceptor = item.getColNames() # if there is no RPC result interceptor function, it contains None. 

		# Optional callback to custom function:
		#
		try:
			funcName = "UnknownName"
			if self.master.postGetqActiveQueueFunction:
				funcName = self.master.postGetqActiveQueueFunctionName
				self.master.postGetqActiveQueueFunction(self.master.dataPath, params)
		except:
			ex = gxutils.formatExceptionInfoAsString(5)
			m1 = "%s - getOneRPCandSendIt(): CALLBACK '%s' FAILED: params=%s, ex=%s" % (self.master.getName(), funcName, params, ex)
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

		frame = gxrpcframe.gxRPCframe(clt_conn)

		try:
			ret = None
			ret = self.doProcCallWithRetry(aThread, frame, procName, params, interceptor)
		except:
			ex = gxutils.formatExceptionInfoAsString(10)
			m1 = "%s - doProcCallWithRetry(): FAILED: proc=%s, params=%s, ex=%s" % (self.master.getName(), procName, params, ex)
			gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)
			return 0

		if (self.master.b_finalStop):
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Found finalStop flag while trying to execute RPC")
			self.logParamsOfRPCnotDone(procName, params)

		self.callPostExecRpcFunction(params, ret)	## called only from here
		return 0


	def doProcCallWithRetry(self, aThread, frame, procName, params, interceptor):
		#
		# called by:	getOneRPCandSendIt()
		#
		# return:
		#	return status from RPC call
		#
		ret = None
		ctr = 0
		modulo_msg = 10
		while (ret == None):
			ctr = ctr + 1
			aThread.changeState(gxthread.STATE_ACTIVE)

			if ctr > 1:
				frame.resetTimeBeginRPC()		# avoid false detection of timeout

			if interceptor == None or interceptor == 0:
				x = mapProcCallToInterceptor( (procName, params, 0) )
				if type(x) == type( () ):
					ret, msg = x
				else:
					interceptor = x
				if ((ctr % modulo_msg) == 0 or ctr == 1) and getDebugLevel() >= 20:
					m1 = "%s -- '%s' has interceptor '%s'" % (self.getName(), procName, x,)
					gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			# ret can become != None if the queue of the interceptor is closed:
			#
			if ret == None:
				self.doProcCallOneTime(frame, procName, params, interceptor)
				ret = frame.getReturnStatus()

			if (ret == 0):
				return 0

			if (ret >= 1):
				gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, ret, "Active queue RPC call failed, ret >= 1. See params values below.")
				self.logParamsOfRPCnotDone(procName, params)
				return ret

			if (ctr % modulo_msg) == 0 or getDebugLevel() >= 5:
				m1 = "%s -- RPC '%s' failed %s times, ret=%s." % (self.getName(), procName, ctr, ret)
				gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, ctr, m1)

			if ret == gxreturnstatus.OS_RPC_GATE_CLOSED:
				aThread.changeState(gxthread.STATE_WAIT)
				time.sleep(10)
				modulo_msg = 30		# there is maintenance going on... no need to log msg too often.
				ret = None
				continue

			if ret == -6 or ret == gxreturnstatus.DS_DEADLOCK or ret == gxreturnstatus.DS_RPC_TIMEOUT:
				aThread.changeState(gxthread.STATE_WAIT)
				time.sleep(2)	# to avoid hard loop
				modulo_msg = 3
				ret = None
				continue

			if g_dictRetryRetValues.has_key(ret):
				aThread.changeState(gxthread.STATE_WAIT)
				time.sleep(5)	# to avoid hard loop
				modulo_msg = 5
				ret = None
				continue

			if ret == gxreturnstatus.DS_NOT_AVAIL:
				if getDebugLevel() >= 1:
					m1 = "%s -- RPC '%s' failed, ret=%s. Will retry with 'ping' until can connect..." % (self.getName(), procName, ret)
					gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

				while (ret == gxreturnstatus.DS_NOT_AVAIL):
					aThread.changeState(gxthread.STATE_WAIT)
					time.sleep(20)	# every 20 seconds ??? -- (was 60 before, 2004oct05,jft)

					if (self.master.b_finalStop):
						gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Found finalStop flag while waiting for good ping!")
						return ret

					aThread.changeState(gxthread.STATE_ACTIVE)
					frame.resetTimeBeginRPC()		# avoid false detection of timeout
					self.doProcCallOneTime(frame, "ping", [], interceptor=0)  #@@@ "ping" is not so good. It should be "ping <dest>"
					ret = frame.getReturnStatus()

				ret = None
				continue

			# Recuperation d'erreur pour les autres cas: ici, on fait comme dans la queue tiapno
			# de la vieille version du serveur. On ne ferme la queue que pour des cas precis,
			# sinon, on "drop" le RPC et on passe au suivant. 
			#
			# Le if suivant est tres large, pour de nombreux cas, on "drop" et on passe au suivant:
			#
			if g_dictDropRetValues.has_key(ret) or ret < gxreturnstatus.GW_NOT_TO_RETRY_END or ret > gxreturnstatus.GW_NOT_TO_RETRY_BEGIN:
				gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, ret, "Active queue RPC call failed, ret <= -1. See params values below.")
				self.logParamsOfRPCnotDone(procName, params)
				return ret

			# if we come here, the return status has some other value...
			#
			modulo_msg = 5
			self.closeQueueAndWaitItIsReopenedByOperator(aThread, procName, params, ret)

			if (self.master.b_finalStop):
				gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Found finalStop flag while waiting for queue re-open!")
				return ret

			ret = None	## to keep looping...

		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "** Should not come here **")
		return -9999

	def callPostExecRpcFunction(self, params, ret):
		#
		# called by:	getOneRPCandSendIt()
		#
		# Optional callback to custom function:
		#
		try:
			funcName = "UnknownName"
			if self.master.postExecRpcFunction:
				funcName = self.master.postExecRpcFunctionName
				self.master.postExecRpcFunction(self.master.dataPath, params, ret, self.master.b_finalStop)
		except:
			ex = gxutils.formatExceptionInfoAsString(5)
			m1 = "%s - callPostExecRpcFunction(): CALLBACK '%s' FAILED: params=%s, ex=%s" % (self.master.getName(), funcName, params, ex)
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)


	def logParamsOfRPCnotDone(self, procName, params):
		m1 = "%s -- Active queue RPC '%s' aborted, params: %s." % (self.getName(), procName, params)
		gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)


	def closeQueueAndWaitItIsReopenedByOperator(self, aThread, procName, params, ret):
		aThread.changeState(gxthread.STATE_WAIT)
		self.master.assignOpenStatus(b_openGet=0)	# close this queue (for get)

		m1 = "%s -- CLOSED this Queue! RPC '%s' SUSPENDED, RPC call failed with ret=%s. Will wait until operator re-open this queue!" % (self.getName(), procName, ret )
		gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)

		while (not self.master.isOpen(forWhat=1) ):
			time.sleep(5)		# check every 5 seconds...
			if (self.master.b_finalStop):
				return

		m1 = "%s -- Found queue was re-opened. Will try RPC '%s' again." % (self.getName(), procName, )
		gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)


	def doProcCallOneTime(self, frame, procName, params, interceptor):
		#
		# Called by:	getOneRPCandSendIt()
		#
		#	build procCall with procName and params
		#	acquire the sync lock for the first time
		#	SEND the RPC
		#	acquire the sync lock again (wait for RPC completion...)
		#	release the lock
		#
		procCall = (procName, params, 0)

		# The synchronization lock is initially free. 
		# We need to acquire it one time first before we can block on it. 
		# Let's do that now:
		#
		self.master.acquireQueueThreadSyncLock()

		# Send the RPC:
		# ------------
		#
		gxrpcmonitors.queueProcCallToDestPool(self.master, frame, procCall, interceptor=interceptor)

		self.master.waitForActiveRPCcompletion()	# we block until the RPC completes...

		# ... The RPC has completed. Release the lock that we just have acquired:
		#
		self.master.releaseQueueThreadSyncLock()

		#
		# we can write the return status to the log (for debugging):
		#
		if (getDebugLevel() >= 5):
			m1 = "%s -- RPC '%s' completed, ret=%s." % (self.getName(), procName, frame.getReturnStatus() )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


#
# Intercept... classes:
#
class InterceptDefinition:
	def __init__(self, name):
		self.name = name
		self.listOfRPCs = []
		self.filters = []

		if (self.initFilters() != 0):
			raise RuntimeError, "InitFilters() failed"

	def __repr__(self):
		s = "<InterceptDefinition: %s>" % (self.name, )
		return s

	def initFilters(self):
		# an InterceptDefinition has a List of InterceptFilter:
		#
		param = self.name + gxparam.GXPARAM__FILTERS
		val = gxServer.getOptionalParam(param)

		if (val == None):
			m1 = "param '%s' not found. This intercept definition is not valid." % (param, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-1)

		while (val != None):
			filter = InterceptFilter(val)

			if (filter == None):
				return (-1)

			self.filters.append(filter)

			val = gxServer.getParamNextValue(param)

		return (0)


	def mapProcNames(self):
		#  map the specified procNames to InterceptDefinition instance:
		#
		param = self.name + gxparam.GXPARAM__INTERCEPT_RPCS
		val = gxServer.getOptionalParam(param)

		if (val == None):
			m1 = "param '%s' not found. NOTE: each intercept definition must be bound to RPC names." % (param, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-1)

		while (val != None):
			self.listOfRPCs.append(val)
			try:
				x = dictProcNamesToInterceptDefinitions[val]
				if x != self:
					m1 = "duplicate intercept definition for proc '%s'. Previous def: %s" % (val, x.name)
					gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
					return (-1)

			except KeyError:
				dictProcNamesToInterceptDefinitions[val] = self
				dictProcNamesToInterceptDefinitions[val.upper()] = self		# make it NOT case-sensitive
				dictProcNamesToInterceptDefinitions[val.lower()] = self		# make it NOT case-sensitive

			val = gxServer.getParamNextValue(param)

		return (0)

	def getName(self):
		return (self.name)



class InterceptFilter:
	def __init__(self, name):
		self.name = name

		self.RPCparamToMatch	= None
		self.columns		= None
		self.trans_id		= None
		self.interceptType	= None
		self.interceptor	= None

		if (self.initAttributes() <= -1):
			raise RuntimeError, "InterceptFilter.initAttributes() failed"

	def __repr__(self):
		s = "<InterceptFilter: %s>" % (self.name, )
		return s

	def initAttributes(self):
		try:
			# get the interceptType to decide which kind of interceptor to create:
			#
			paramName = self.name + gxparam.GXPARAM__TYPE

			self.interceptType = gxServer.getMandatoryParam(paramName)


			if (self.interceptType == gxparam.GXPARAM_QUEUE):

				rc = self.initBaseQueueAttributes()
				if (rc != 0):
					return (rc)

				self.interceptor = InterceptQueue(self.name, self.columns, self.trans_id)

			elif (self.interceptType == gxparam.GXPARAM_ACTIVE_QUEUE):
				
				rc = self.initBaseQueueAttributes()
				if (rc != 0):
					return (rc)

				self.interceptor = InterceptQueue(self.name, self.columns, self.trans_id, b_active=1)

			elif (self.interceptType == gxparam.GXPARAM_NOTIFY_UDP ):

				self.interceptor = InterceptNotifyUDP(self.name)

			elif (self.interceptType == gxparam.GXPARAM_MULTI ):

				self.interceptor = InterceptMultiQueues(self.name)

			elif (self.interceptType == gxparam.GXPARAM_QUEUE_SWITCH):
				
				self.interceptor = InterceptQueueSwitch(self.name)

			elif (self.interceptType == gxparam.GXPARAM_CUSTOM_INTERCEPT):
				
				self.interceptor = InterceptFunction(self.name)

			else:
				m1 = "create InterceptFilter '%s' FAILED: interceptor of type '%s' is NOT implemented. param: %s." % (self.name, self.interceptType, paramName)

				gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
				return (-1)

		except:
			ex = gxutils.formatExceptionInfoAsString(12)
			m1 = "Failed to create InterceptFilter '%s': Exception? OR missing param '%s'. Exc: %s" % (self.name, paramName, ex)

			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-1)

		return (0)



	def initBaseQueueAttributes(self):
		#
		# get param values for:	 params, columns, trans_id column name
		#
		try:
			paramName = self.name + gxparam.GXPARAM__PARAM
			#
			# NOTE: we really want to get the list of values associated to paramName
			#
			self.RPCparamToMatch = gxServer.getParamValuesList(paramName)

			if (not self.RPCparamToMatch):
				m1 = "Failed to create InterceptFilter '%s': %s_PARAM is None or empty (%s). '%s'." % (self.name, self.name, self.RPCparamToMatch, paramName)
				gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
				return (-1)

			if (len(self.RPCparamToMatch) != 2):
				if (len(self.RPCparamToMatch) != 1 or self.RPCparamToMatch[0].lower() != "none"):
					m1 = "Failed to create InterceptFilter '%s': %s_PARAM is NOT a list of 2 elements (%s). '%s'." % (self.name, self.name, self.RPCparamToMatch, paramName)
					gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
					return (-1)

			# Convert "none" to None...
			#
			if (len(self.RPCparamToMatch) == 1 and self.RPCparamToMatch[0].lower() == "none"):
				self.RPCparamToMatch = None

			paramName = self.name + gxparam.GXPARAM__COLUMNS

			self.columns = gxServer.getParamValuesList(paramName)
			if (not self.columns):
				m1 = "Failed to create InterceptFilter '%s': %s_COLUMNS is NOT a list of 1 or more elements (%s)." % (self.name, self.name, self.columns)
				gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
				return (-1)

			# Convert ["none"] to []...
			#
			if (len(self.columns) == 1 and self.columns[0].lower() == "none"):
				self.columns = []

			paramName = self.name + gxparam.GXPARAM__TRANS_ID

			self.trans_id = gxServer.getMandatoryParam(paramName)

			# Convert "none" to None...
			#
			if (self.trans_id.lower() == "none"):
				self.trans_id = None
		except:
			ex = gxutils.formatExceptionInfoAsString(3)
			m1 = "initBaseQueueAttributes(): Failed for  InterceptFilter '%s': Exception? OR missing param '%s'. Exc: %s" % (self.name, paramName, ex)

			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-1)

		return (0)



	def getName(self):
		return (self.name)


	def getInterceptorIfRPCparamsMatch(self, procCall):
		#
		# Called by:	mapProcCallToInterceptor(procCall) -- module function
		#
		# NOTE: procCall can be gxrpcframe.gxProcCall or a procCall tuple (procName, params, 0)
		#
		if (not getEnableInterceptStatus() ):	# interceptions are disabled: do not intercept anything
			if (getDebugLevel() >= 20):
				m1 = "%s - Interceptions are disabled. NOT intercepting %s." % (self.name, procCall)
				gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			return None

		if (getDebugLevel() >= 7):
			try:
				m1 = "%s - Trying to match param: %s ?? %s..." % (self.name, self.RPCparamToMatch, procCall)
			except:
				m1 = "%s - debug log entry failed in getInterceptorIfRPCparamsMatch()" % (self.name, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		try:
			if type(procCall) == type( () ):
				paramsList = procCall[1]
			else:
				paramsList = procCall.getParamsList()

			# if there is no self.RPCparamToMatch, then, just return the interceptor now...
			#
			if (self.RPCparamToMatch == None):
				if (self.interceptor.paramFunctionName != None):
					if (self.interceptor.interceptParams(paramsList) != 0):
						return (None) 
				return (self.interceptor.returnInterceptorIfReady() )

			nameToMatch, valueToMatch = self.RPCparamToMatch

			nameToMatch = nameToMatch.lower()	# compare all names in lowercase

			if (getDebugLevel() >= 15):
				m1 = "%s.RPCparamToMatch: %s, paramsList: %s." % (self.name, self.RPCparamToMatch, paramsList )
				gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			if (not paramsList):
				return (None)		# no params to look at

			for param in paramsList:
				if (len(param) == 6):
					name, value, datatype, isNull, isOutput, sz = param
				else:
					sz = None
					name, value, datatype, isNull, isOutput = param

				if (name.lower() != nameToMatch):
					continue	# not this, try next param...

				newVal = convertToPythonTypeIfNotString(value, datatype, isNull)
				if (newVal != None):
					value = newVal

				if (getDebugLevel() >= 20):
					m1 = "'%s' == '%s' ... '%s' ==? '%s' (datatype:%s, isNull:%s), type(valueToMatch):%s." % (name.lower(),
							 nameToMatch, value, valueToMatch, datatype, isNull, type(valueToMatch) )
					gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

				if (value == valueToMatch):
					return (self.interceptor.returnInterceptorIfReady() )

			return None
		except:
			ex = gxutils.formatExceptionInfoAsString(3)

			m1 = "%s - getInterceptorIfRPCparamsMatch() FAILED, Exc: %s"  % (self.name, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

		return None


class InterceptQueueItem(gxqueue.gxQueueItem):
	#
	# InterceptQueueItem is sub-classed from gxqueue.gxQueueItem specifically
	# to allow all InterceptQueue.queue to compute stats.
	#
	def assignValues(self, colNames, row):
		self.colNames = colNames
		self.row      = row

	def getColNames(self):
		return (self.colNames)

	def getRow(self):
		return (self.row)


class RPCinterceptor:
	#
	# super-class of:  InterceptQueueSuperClass, InterceptFunction
	#
	# NOTE: this super-class allows test in gxcallparser.py to know if the value returned by
	#	mapProcCallToInterceptor(), InterceptFilter.getInterceptorIfRPCparamsMatch() 
	#	is an interceptor or not.
	#

        def __init__(self):
                self.name = None

                self.paramFunctionName  = None
                self.paramFunction      = None
                self.paramConfArgs      = None

                self.resultFunctionName = None
                self.resultFunction     = None
                self.resultConfArgs     = None

		self.dataPath			= None
		self.extraDataList		= None

		self.postGetqFunction		= None
		self.processDumpQueueFunction	= None
		self.postPutqFunction		= None
		self.postGetqActiveQueueFunction = None
		self.postExecRpcFunction	= None

		self.postGetqFunctionName		= None
		self.processDumpQueueFunctionName	= None
		self.postPutqFunctionName		= None
		self.postGetqActiveQueueFunctionName	= None
		self.postExecRpcFunctionName		= None

	def __repr__(self):
		try:
			x = self.name
		except:
			self.name = "[no name]"
		s = "<InterceptDefinition: %s>" % (self.name, )
		return s

	def getStatusAsRow(self):
		#
		# NOTE: also implemented in InterceptQueue and in InterceptNotifyUDP
		#
		a = "[no stats available for this kind of interceptor]"
		i = 0
		n = 0.0
		return [ a, i, i, i, i, n, n, n, i ]

	def getStatusAsRowHeaders(self):
		h = [ "name______", "openPut", "openGet", 
		      "N_queued", "max_Qsize", 
		      "Total___", "Done____", "Reject__", 
		      "shutdown" ]
		return h

	def shutdownAndDump(self, b_quiet):	# default implementation does nothing
		return


	def prefixGetOptionalParam(self, x):
		#
		# Called by:	getCustomFunctionForFuncNameX()
		#
		return gxServer.getOptionalParam(self.name + x)

	def getCustomFunctionsParams(self):
		#
		# Called by:	InterceptQueue or InterceptFunction.initAttributes()
		#
		# NOTE: self.name is already assigned by the sub-class.
		#
		try:
			paramName = None		# make sure we have one if except: occurs

			self.b_intercept_return_status = 0

			self.paramFunctionName	= None
			self.paramFunction	= None
			self.paramConfArgs	= None

			self.resultFunctionName	= None
			self.resultFunction	= None
			self.resultConfArgs	= None

			self.dataPath			= None
			self.extraDataList		= None

			self.postGetqFunction		= None
			self.processDumpQueueFunction	= None
			self.postPutqFunction		= None
			self.postGetqActiveQueueFunction = None
			self.postExecRpcFunction	= None

			self.postGetqFunctionName		= None
			self.processDumpQueueFunctionName	= None
			self.postPutqFunctionName		= None
			self.postGetqActiveQueueFunctionName	= None
			self.postExecRpcFunctionName		= None

			x = self.prefixGetOptionalParam(gxparam.GXPARAM__INTERCEPT_RETURN_STATUS)
			self.b_intercept_return_status = (x in [1,"1", "yes","YES", "Yes", "oui", "Oui", "OUI" ] )

			self.paramFunctionName = self.prefixGetOptionalParam(gxparam.GXPARAM__PARAM_FUNCTION)

			if (self.paramFunctionName != None):
				paramName = self.name + gxparam.GXPARAM__PARAM_FUNCTION_ARGS
				self.paramConfArgs = gxServer.getParamValuesList(paramName)


			self.resultFunctionName = self.prefixGetOptionalParam(gxparam.GXPARAM__RESULT_FUNCTION)

			if (self.resultFunctionName):
				paramName = self.name + gxparam.GXPARAM__RESULT_FUNCTION_ARGS
				self.resultConfArgs = gxServer.getParamValuesList(paramName)


			self.paramFunction = getCustomFunctionForFuncNameX(self.paramFunctionName)
			self.resultFunction = getCustomFunctionForFuncNameX(self.resultFunctionName)

			self.dataPath			  = self.prefixGetOptionalParam(gxparam.GXPARAM__DATA_PATH)

			if self.prefixGetOptionalParam(gxparam.GXPARAM__EXTRA_DATA_LIST):
				paramName = self.name + gxparam.GXPARAM__EXTRA_DATA_LIST
				self.extraDataList = gxServer.getParamValuesList(paramName)


			self.postGetqFunctionName	  = self.prefixGetOptionalParam(gxparam.GXPARAM__POST_GETQ_FUNCTION)
			self.processDumpQueueFunctionName = self.prefixGetOptionalParam(gxparam.GXPARAM__PROCESS_DUMP_QUEUE_FUNCTION)
			self.postPutqFunctionName	  = self.prefixGetOptionalParam(gxparam.GXPARAM__POST_PUTQ_FUNCTION)
			self.postGetqActiveQueueFunctionName= self.prefixGetOptionalParam(gxparam.GXPARAM__POST_GETQ_ACTIVE_QUEUE_FUNCTION)
			self.postExecRpcFunctionName	  = self.prefixGetOptionalParam(gxparam.GXPARAM__POST_EXEC_RPC_FUNCTION)

			self.postGetqFunction		= getCustomFunctionForFuncNameX(self.postGetqFunctionName)
			self.processDumpQueueFunction	= getCustomFunctionForFuncNameX(self.processDumpQueueFunctionName)
			self.postPutqFunction		= getCustomFunctionForFuncNameX(self.postPutqFunctionName)
			self.postGetqActiveQueueFunction = getCustomFunctionForFuncNameX(self.postGetqActiveQueueFunctionName)
			self.postExecRpcFunction	= getCustomFunctionForFuncNameX(self.postExecRpcFunctionName)

			ls = [ 
				( "paramFunction",		self.paramFunctionName,		self.paramFunction ),
				( "resultFunction",		self.resultFunctionName,		self.resultFunction ),
				( "postGetqFunction",		self.postGetqFunctionName, self.postGetqFunction ),
				( "postExecRpcFunction",	self.postExecRpcFunctionName, self.postExecRpcFunction ),
				( "postGetqActiveQueueFunction", self.postGetqActiveQueueFunctionName, self.postGetqActiveQueueFunction ),
				( "postPutqFunction",		self.postPutqFunctionName, self.postPutqFunction ),
				( "processDumpQueueFunction",	self.processDumpQueueFunctionName, self.processDumpQueueFunction ),
			    ]
			for x in ls:
				v, fname, fn = x
				m1 = "%s -- %s \t: %s \t= %s" % (self.name, v, fname, fn, )
				gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		except:
			ex = gxutils.formatExceptionInfoAsString(5)
			m1 = "getCustomFunctionsParams(): Failed while used by '%s', at param name: %s, Exc: %s" % (self.name, paramName, ex)

			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-1)

		return (0)	


	def getDataPath(self):
		return self.dataPath

	def getExtraDataList(self):
		return self.extraDataList


	def callCustomInterceptResponse(self, frame, resSetNo, colNames, rowsList, ret_stat):
		#
		# Called by:	InterceptQueue.processRows()
		#
		# NOTE: <interceptor> could be a InterceptFunction or an InterceptQueue...
		#	This is why this method is in this super-class.
		#
		#
		# Pass the result_sets_list to the custom intercept function.
		#
		#
		# NOTES: We only intercept the result_sets_list.
		#
		#	 We ignore the outParams, the msg_list, and the various 
		#	 diagnostic fields after that.
		#
		#	 The custom function must follow the rules for Return, as here below.
		#
		# Return:
		#	0	Nothing special, remainder of general results interception should proceed
		#	1	Custom function has done everything requried. General intercept should 
		#		do nothing more.
		#	-1	Some error happened, general intercept should NOT do anything more.
		#
		try:
			if (self.resultFunction == None):
				return (0)		# no binding to internal function, do nothing.

			if (getDebugLevel() >= 20):
				m1 = "%s -- about to call resultFunction(), with columns: %s, rows: %s, ret_stat: %s" % (self.name, colNames, rowsList, ret_stat)
				gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

			if (rowsList == None or len(rowsList) == 0):
				return (0)		# OK, this can happen

			#
			# Call the custom function now:
			#
			rc = self.resultFunction(self, frame, resSetNo, colNames, rowsList, ret_stat, self.resultConfArgs)

			return (rc)
		except:
			ex = gxutils.formatExceptionInfoAsString()

			m1 = "%s - Failed in call to custom result intercept function '%s', frame: %s, Exc: %s" % (self.name, self.resultFunctionName, frame, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

		return (-1)


class InterceptQueueSuperClass(RPCinterceptor):
	#
	# super-class of:  InterceptQueue, InterceptQueueSwitch, InterceptMultiQueues, InterceptFunction, InterceptNotifyUDP
	#
	def __repr__(self):
		try:
			x = self.name
		except:
			self.name = "[no name]"
		s = "<InterceptQueueSuperClass: %s>" % (self.name, )
		return s

	def assignOpenStatus(self, b_openGet=None, b_openPut=None):
		"""This is a stub implementation. The subclasses can implement a version which does some real work"""
		m1 = "%s - assignOpenStatus(): this is a stub implementation of this method." % (self.name, )
		gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
		return 0

	def getEnableStatus(self):
		"""This is a stub implementation. The subclasses can implement a version which does some real work"""
		return "AlwaysOpen"

	def prepareUDPsocketToGateway(self):
		"""This method is in class InterceptQueueSuperClass because it is used by InterceptQueue and InterceptNotifyUDP
		"""
		ls = self.notifyUDPaddress.split(":")
		if (not ls  or len(ls) != 2):
			raise RuntimeError, "InterceptQueue.prepareUDPsocketToGateway() failed: .notifyUDPaddress is not a list of 2 elements separated by ':'"

		hostName = ls[0]
		portNo   = ls[1]

		self.udp = igw_udp.igw_udp(hostName, portNo, gxServer.getServerName() )


	def sendUDPcmdWithOptions(self, cmd_char, option_char, options_str):
		"""This method is in class InterceptQueueSuperClass because it is used by InterceptQueue and InterceptNotifyUDP
		"""
		if (self.b_active):	# active queue do not send UDP notification
			return
		self.udp.sendUDPcmdWithOptions(cmd_char, option_char, options_str)

	def notifyGateway(self, ctrRows):
		"""This method is in class InterceptQueueSuperClass because it is used by InterceptQueue and InterceptNotifyUDP
		"""
		self.sendUDPcmdWithOptions(igw_udp.UDP_CMD_NEW_REQUEST_AVAIL, '\0', "")

		# NOTE: As in os_ggq.c, sf_notify_gw(p_qtask), only the notification is sent.
		#	The ctrRows is NOT sent.

		if (getDebugLevel() >= 20):
			m1 = "%s - notifyGateway() %s rows %s" % (self.name, ctrRows, (self.notifyUDPaddress, self.notifyInterval, self.maxRowsInGet) ) 
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

	#
	# Both sub-classes (InterceptQueue, InterceptQueueSwitch) call 
	# self.addResultSetToQueue(), but, after that, each subclass has 
	# a different implementation of the method checkColNamesAndProcessRows().
	#
	def captureResponse(self, frame, resp):
		#
		# Called by:	gxInterceptRPCMonitor.interceptResponse() as:  frame.interceptor.captureResponse(frame, resp)
		#
		# Intercept (capture) the response, queue the rows (if required), and, 
		# write the "transaction id" to the log.
		#
		#
		# NOTES: We only capture (queue) the rows from the result_sets_list.
		#
		#	 We ignore the outParams, the msg_list, and the various 
		#	 diagnostic fields after that.
		#
		try:
			if (getDebugLevel() >= 10):
				respData = resp[0:5]
				print "respData:", respData

			result_sets_list = resp[0]

			ret_stat = resp[4]

			if (result_sets_list == None or len(result_sets_list) == 0):
				return (None)		# OK, this can happen

		except:
			ex = gxutils.formatExceptionInfoAsString(2)
			try:
				saveUnusableResponse(frame, resp)
			except:
				pass
			m1 = "%s - Failed to queue any set from results_sets_list of captured response. frame: %s, Exc: %s" % (self.name, frame, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return None

		if (getDebugLevel() >= 3):
			m1 = "%s - BEGIN captureResponse(): '%s', ret=%s, %s result sets in list." % (self.name, 
								frame.getProcName(), ret_stat, len(result_sets_list) )
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		#
		# If the columns of a resultSet match self.columns, add its rows to the queue:
		#
		resSetNo = 0
		for resultSet in result_sets_list:
			ctrRows = self.addResultSetToQueue(resSetNo, resultSet, frame, ret_stat)
			if (ctrRows <= -1):
				break	# if ctrRows >= 1: success (??? break?), if ctrRows <= -1: error. @@@
			resSetNo  = resSetNo + 1

		return None

		# NOTES:
		#
		# the Response tuple layout (from xmltp_gx.c) is:
		#
		#    py_response = Py_BuildValue("(OOOiiiiiiisssss)", 
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
		# 14			        msg_buff   );
		#

	def addResultSetToQueue(self, resSetNo, resultSet, frame, ret_stat):
		#
		# Called by:	self.captureResponse()
		#
		# If the columns of a resultSet match self.columns, add its rows to the queue,
		# and, write the "transaction id" to the log.
		#
		# NOTES:
		#
		# Each element of the result_sets_list is a tuple which contains (3) elements:
		#
		#	colNames	a list of column names. Each is a tuple: (colname, datatype)
		#	rowsList	a list of rows. 
		#			Each row is a list whose elements values are tuples: (val, b_isNull)
		#	nbRows		an integer
		#
		#
		try:
			if (getDebugLevel() >= 20):
				print "resultSet:", resultSet

			if (resultSet == None):
				return (0)		# weird, but, never mind

			colNames, rowsList, nbRows  = resultSet


			if (rowsList == None or len(rowsList) == 0):
				return (0)		# OK, never mind
		except:
			ex = gxutils.formatExceptionInfoAsString(2)
			try:
				saveUnusableResultSet(frame, resultSet)
			except:
				pass
			m1 = "%s - Failed to un-tuple resultSet. frame: %s, Exc: %s" % (self.getName(), frame, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-2)

		return (self.checkColNamesAndProcessRows(resSetNo, colNames, rowsList, nbRows, frame, ret_stat) )

	def checkColNamesAndProcessRows(self, resSetNo, colNames, rowsList, nbRows, frame, ret_stat):
		#
		# This method exists in these subclasses: InterceptQueueSwitch, InterceptMultiQueues
		# but, with a different implementation.
		#
		# This version here (in InterceptQueue) is the simplest.
		#
		dictColNames = self.buildDictColumnsAndCheckIfHasRequiredColumns(colNames)
		if (dictColNames == None):
			return (0)

		if (self.trans_id  and  self.getTransIdColumnIndex(dictColNames) <= -1):
			return (0)

		return (self.processRows(resSetNo, colNames, rowsList, nbRows, dictColNames, frame, ret_stat) )

	def buildDictColumnsAndCheckIfHasRequiredColumns(self, colNames):
		"""
		 Called by:	self.addResultSetToQueue()
		
		 Return:
			None		colNames does not contains all the required column 
					and trans_id column
			dictColNames 

		 NOTE: this method is in InterceptQueueSuperClass because we need to call it from these
		  subclasses:
			InterceptQueue
			InterceptFunction
		"""
		dictColNames = {}
		i = 0
		for col in colNames:
			if (len(col) == 3):
				name, dataType, sz = col
			else:
				name, dataType = col
			dictColNames[name] = i
			i = i + 1

		if (not self.columns):		# None or empty list
			return dictColNames

		for reqCol in self.columns:
			if (not dictColNames.has_key(reqCol) ):
				if (getDebugLevel() >= 10):
					m1 = "%s - column '%s' not found by buildDictColumnsAndCheckIfHasRequiredColumns()." % (self.name, reqCol)
					gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
				return None

		return dictColNames

	def returnInterceptorIfReady(self):
		return self		# default method implementation, can be overidden by subclass


	def normalizeRowValues(self, colNames, row):
		#
		# Called by:	InterceptQueue.processOneRow()
		#		various custom intercept functions, called by InterceptFunction
		#
		# all values in the row are string, but, some should be integer or float.
		# we must normalize them before adding the row to the queue.
		#
		# Nothing to do for string, or time/date/timestamps (??).
		#
		# NOTE: log as most as 5 errors, after that, be quiet.
		#
		idx = -1	# used for diagnostic when exception occurs
		try:
			for i in xrange(len(row) ):
				idx = i
				col = colNames[i]
				if (len(col) == 3):
					name, datatype, sz = col
				else:
					sz = None
					name, datatype = col

				val, isNull    = row[i]

				# NOTE: convertToPythonTypeIfNotString() returns None is val does not
				#	require to be converted.
				#
				newValue = convertToPythonTypeIfNotString(val, datatype, isNull)

				if (newValue == None):	# if None, then val is a string...
					newValue = gxrpcmonitors.unEscapeXML(val)

				row[i] = (newValue, isNull)

			return (0)
		except:
			ex = gxutils.formatExceptionInfoAsString(2)
			try:
				if (self.b_told_error_in_normalizeRowValues >= 5):
					return (-2)
			except:
				self.b_told_error_in_normalizeRowValues = 0

			self.b_told_error_in_normalizeRowValues = self.b_told_error_in_normalizeRowValues + 1

			m1 = "%s - normalizeRowValues(): idx=%s, Exc: %s, colNames: %s, row: %s." % (self.name, idx, ex, colNames, row) 
			gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)

			if (self.b_told_error_in_normalizeRowValues >= 5):
				m1 = "%s - normalizeRowValues(): maximum of error messages reached." % (self.name, )
				gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

		return (-2) 	# -2, distinct rc from queue.put() error

class InterceptMultiQueues(InterceptQueueSuperClass):
	def __init__(self, name):
		self.name    = name
		self.multiQueuesList = []
		self.RPCparamToMatch = None
		self.paramFunctionName = None
		rc = self.initAttributes()

		if not self.multiQueuesList or rc != 0:
			m1 = "create InterceptMultiQueues '%s' FAILED: .multiQueuesList is empty or rc!=0: %s rc: %s." % (name, self.multiQueuesList, rc,)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			raise RuntimeError, "InterceptMultiQueues.__init__() failed. See log."

	def __repr__(self):
		try:
			x = self.name
		except:
			self.name = "[no name]"
		s = "<InterceptMultiQueues: %s>" % (self.name, )
		return s

	def initAttributes(self):
		rc = 0
		paramName = self.name + gxparam.GXPARAM__MULTI_LIST

		queueName = gxServer.getOptionalParam(paramName, None)

		if not queueName:
			m1 = "create InterceptMultiQueues '%s' FAILED: parameter '%s' is missing." % (self.name, paramName, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return -2

		while queueName != None:
			queue = getQueueByName(queueName)

			if queue == None:
				m1 = "create InterceptMultiQueues '%s' ERROR: queue '%s' does NOT exist (yet). All Queues must precede InterceptMultiQueues in INTERCEPT_DEFINITIONS list." % (self.name, queueName, )
				gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
				rc = -1
			else:
				self.multiQueuesList.append(queue)

			queueName = gxServer.getParamNextValue(paramName)

		return rc

	def checkColNamesAndProcessRows(self, resSetNo, colNames, rowsList, nbRows, frame, ret_stat):
		#
		# This implementation overrides the method of same name in InterceptQueueSuperClass
		#
		# This version here (in InterceptMultiQueues) is the more general and flexible one.
		#
		# We try the all queues in self.multiQueuesList, for each wich can build the dictColNames,
		# we let it do processRows().
		#

		if (getDebugLevel() >= 30):
			m1 = "%s -MULTI- checkColNamesAndProcessRows(): Begin...  cols: %s. " % (self.name, colNames, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		for q in self.multiQueuesList:
			dictColNames = q.buildDictColumnsAndCheckIfHasRequiredColumns(colNames)
			if not dictColNames:
				continue

			if q.trans_id  and  q.getTransIdColumnIndex(dictColNames) <= -1:
				continue

			q.processRows(resSetNo, colNames, rowsList, nbRows, dictColNames, frame, ret_stat)

		return 0


class InterceptQueueSwitch(InterceptQueueSuperClass):
	def __init__(self, name):
		self.name    = name
		self.queue_1 = None
		self.queue_2 = None

		self.paramFunctionName	= None	# make sure we have this attribute here anyway.
		self.resultFunctionName	= None
		self.resultFunction	= None

		self.initAttributes()

		if (self.queue_1 == None  or  self.queue_2 == None):
			m1 = "create InterceptQueueSwitch '%s' FAILED: one or both queue(s) are None. %s, %s." % (name, self.queue_1, self.queue_2)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			raise RuntimeError, "InterceptQueueSwitch.__init__() failed. See log."

		dictOfInterceptQueues[self.name.lower()] = self

	def __repr__(self):
		try:
			x = self.name
		except:
			self.name = "[no name]"
		s = "<InterceptQueueSwitch: %s>" % (self.name, )
		return s


	def initAttributes(self):
		self.queue_1 = self.getQueueForParamName(gxparam.GXPARAM__QUEUE_1)
		self.queue_2 = self.getQueueForParamName(gxparam.GXPARAM__QUEUE_2)

	def getQueueForParamName(self, paramWithQueueName):
		paramName = self.name + paramWithQueueName
		queueName = gxServer.getMandatoryParam(paramName)

		queue = getQueueByName(queueName)
		if (queue == None):
			m1 = "create InterceptQueueSwitch '%s' FAILED: queue '%s' does NOT exist (yet). Queues must precede QueueSwitch in filters list." % (self.name, queueName)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			raise RuntimeError, "InterceptQueueSwitch.initAttributes() failed"

		return queue

	def checkColNamesAndProcessRows(self, resSetNo, colNames, rowsList, nbRows, frame, ret_stat):
		#
		# This implementation overrides the method of same name in InterceptQueueSuperClass
		#
		# This version here (in InterceptQueueSwitch) allows to choose between one of two queues.
		#
		# We try the first queue, if it can build the dictColNames,
		# we let it handle the rest of the processing.
		# If it cannot, we try with the second queue...
		#

		if (getDebugLevel() >= 30):
			m1 = "%s - checkColNamesAndProcessRows(): Begin...  cols: %s, resSetNo: %s." % (self.name, colNames, resSetNo, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		q = self.queue_1	# try with first queue

		dictColNames = q.buildDictColumnsAndCheckIfHasRequiredColumns(colNames)
		if (dictColNames):
			if (q.trans_id  and  q.getTransIdColumnIndex(dictColNames) <= -1):
				return (0)
		else:
			q = self.queue_2	# try with Second queue

			dictColNames = q.buildDictColumnsAndCheckIfHasRequiredColumns(colNames)
			if (dictColNames == None):
				return (0)

			if (q.trans_id  and  q.getTransIdColumnIndex(dictColNames) <= -1):
				return (0)

		return (q.processRows(resSetNo, colNames, rowsList, nbRows, dictColNames, frame, ret_stat) )


class InterceptQueue(InterceptQueueSuperClass):
	def __init__(self, name, columns, trans_id, b_active=0):
		self.name     = name

		self.columns  = columns
		self.trans_id = trans_id
		self.no_Trans_id_SaidAlready = 0

		self.b_shutdown_done	= 0		# set to True by shutdownAndDump()

		self.b_active 		= b_active
		self.b_persistent	= 0
		self.b_finalStop	= 0		# only relevant for active queue
		self.pseudoClientConn	= None		# only relevant for active queue
		self.queueMonitor	= None
		self.queueThread	= None
		self.queueThreadSyncLock = None		# only relevant for active queue
		self.rpc_name		= None		# will be assigned only if ACTIVE_QUEUE

		self.notifyUDPaddress	= None		# only relevant if NOT active queue
		self.notifyInterval	= None		# only relevant if NOT active queue
		self.maxRowsInGet	= None		# only relevant if NOT active queue	
		self.heldQitem		= None		# needed to "push back" one item on the queue (!)
		self.queueMax		= None	
		self.logMarker		= None	
		self.logMarkerFailed	= None	
		self.b_openPut		= 1	# set in self.initAttributes()
		self.b_openGet		= 1
		self.queue_closed_msg	= None
		self.queue_full_msg	= None
		self.b_intercept_return_status = 0

		self.paramFunctionName	= None	# make sure we have this attribute here anyway.
		self.resultFunctionName	= None
		self.resultFunction	= None

		self.b_told_error_in_getValueOfColumnIdx = 0

		if (self.initAttributes() <= -1):
			raise RuntimeError, "InterceptQueue.initAttributes() failed"

		self.udp = None
		if (not b_active):			# active queue don't have a UDP socket
			self.prepareUDPsocketToGateway()	# will assign self.udp

		if self.b_persistent:
			m1 = "%s -- about to create gxbdbqueue.gxbdbqueue ..." % (self.name, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			self.queue = gxbdbqueue.gxbdbqueue(name+"_Q", owner=self, max=self.queueMax)
		else:
			self.queue = gxqueue.gxqueue(name+"_Q", owner=self, max=self.queueMax)

		dictOfInterceptQueues[self.name.lower()] = self

		if (self.b_active):	# if ACTIVE_QUEUE, start the Queue monitor thread:
			self.startActiveQueueMonitorThread()


	def __repr__(self):
		status = self.getEnableStatus()

		str = "<%s: %s, queued: %s/%s. (%s)>" % (self.name, status, self.getQueueQsize(), self.queueMax, self.b_shutdown_done)

		return (str)

	def getQueueQsize(self):
		if self.heldQitem:
			x = 1
		else:
			x = 0
		return (self.queue.qsize() + x)

	def queueGetNowait(self):
		if self.heldQitem:
			item = self.heldQitem
			self.heldQitem = None
			return item

		return self.queue.get_nowait()

	def initAttributes(self):
		try:
			paramName = self.name + gxparam.GXPARAM__Q_MAX
			self.queueMax = gxServer.getMandatoryParam(paramName)

			paramName = self.name + gxparam.GXPARAM__STORAGE
			storage_type = gxServer.getOptionalParam(paramName)

			self.b_persistent = 0	# for compatibility with older config files, the default is non-persistent (IN_MEMORY)
			if storage_type and storage_type.upper() == gxparam.GXPARAM_PERSISTENT:
				self.b_persistent = 1
				
			if (not self.b_active):		# active queue don't have these attributes:
				paramName = self.name + gxparam.GXPARAM__MAX_ROWS_IN_GET
				self.maxRowsInGet = gxServer.getMandatoryParam(paramName)

				paramName = self.name + gxparam.GXPARAM__NOTIFY_UDP_ADDRESS
				self.notifyUDPaddress = gxServer.getMandatoryParam(paramName)

				paramName = self.name + gxparam.GXPARAM__NOTIFY_INTERVAL
				self.notifyInterval = gxServer.getMandatoryParam(paramName)

			paramName = self.name + gxparam.GXPARAM__LOG_MARKER_FAILED
			self.logMarkerFailed = gxServer.getMandatoryParam(paramName)

			paramName = self.name + gxparam.GXPARAM__LOG_MARKER
			self.logMarker = gxServer.getMandatoryParam(paramName)


			paramName = self.name + gxparam.GXPARAM__OPEN_AT_INIT
			open_at_init = gxServer.getOptionalParam(paramName)
			if (not open_at_init or open_at_init.lower() == "yes" or open_at_init == 1 or open_at_init == "1"):
				self.b_openPut = 1
				self.b_openGet = 1
			else:
				self.b_openPut = 0
				self.b_openGet = 0

			paramName = self.name + gxparam.GXPARAM__QUEUE_CLOSED_MSG
			self.queue_closed_msg = gxServer.getOptionalParam(paramName)

			paramName = self.name + gxparam.GXPARAM__QUEUE_FULL_MSG
			self.queue_full_msg = gxServer.getOptionalParam(paramName)

			paramName = "[ self.getCustomFunctionsParams() ]"

			self.getCustomFunctionsParams()		# bind optional resultFunction, if specified


			if (self.b_active):	# self.rpc_name is relevant only for ACTIVE_QUEUE
				paramName = self.name + gxparam.GXPARAM__RPC_NAME

				self.rpc_name  = gxServer.getMandatoryParam(paramName)
		except:
			ex = gxutils.formatExceptionInfoAsString(10)
			m1 = "Failed to create InterceptQueue '%s': missing param '%s'. Exc: %s." % (self.name, paramName, ex)

			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-1)

		return (0)	

	def getStatusAsRow(self):
		nTotal = self.queue.getStatsN(gxqueue.STATS_QUEUED_TIME)
		nDone  = self.queue.getStatsN(gxqueue.STATS_OUT_OF_QUEUE)
		nFailed= self.queue.getStatsN(gxqueue.STATS_OUT_OF_QUEUE_ALT)

		row = [ self.name, self.b_openPut, self.b_openGet, 
		        self.getQueueQsize(), self.queueMax, 
			nTotal, nDone, nFailed, 
			self.b_shutdown_done,  ]

		return row

	def shutdownAndDump(self, b_quiet):
		if (self.b_shutdown_done):
			return None

		self.b_shutdown_done = 1	# we do not want to run this method more than once.
	
		self.b_openPut = 0		# close the queue!
		self.b_openGet = 0

		if self.b_active:
			self.b_finalStop = 1	# tell the ActiveQueueMonitor thread to stop

		if self.b_persistent:
			m1 = "%s -- Persistent queue closing... The queue holds %s items. Calling q.closeQueue()..." % (self.name, self.getQueueQsize() )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			rc = self.queue.closeQueue()
			if rc == 0:
				m1 = "%s -- Persistent queue closed successfully." % (self.name, )
				level = TxLog.L_MSG
			else:
				m1 = "%s -- Persistent queue did not close correctly. q.closeQueue() rc=%s." % (self.name, rc, )
				level = TxLog.L_ERROR
			gxServer.writeLog(MODULE_NAME, level, 0, m1)
			return

		if (getDebugLevel() >= 1  or  not b_quiet):
			m1 = "%s -- queue closed, preparing to dump %s items..." % (self.name, self.getQueueQsize() )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		dumpDir = gxServer.getOptionalParam(gxparam.GXPARAM_QUEUE_DUMP_DIR)
		if (not dumpDir):
			dumpDir = "./"

		timeStr = gxutils.getTimeStringForFileName()

		filePath = "%s/dump_Q_%s.%s" % (dumpDir, self.name, timeStr)

		try:
			fd = open(filePath, "a")
		except IOError:
			ex = gxutils.formatExceptionInfoAsString(2)
 			m1 = "shutdownAndDump(): FAILED to open() filePath: '%s', will use stderr! Exc: %s." % (filePath, ex, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			fd = sys.stderr
			filePath = "[stderr]"

		nbItems = self.getQueueQsize()
		ctrDone = 0
		try:
			if (self.b_active):
				strActive = "ACTIVE "
			else:
				strActive = ""
			m1 = "%s -- ready to dump to '%s'. %sQueue has %s items. (fd=%s)." % (self.name, filePath, strActive, nbItems, fd.fileno() )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			str = "# %s\n" % (m1, )
			fd.write(str)

			str = "# each line is: \"[ row list ], [ colName list ]\"  or  \"[ row list ], None\" (for Active queue)."

			self.callProcessDumpQueueFunction(None, None, nbItems)	# there are (3) such calls to callback...

			while (1):
				qItem = self.queueGetNowait()
				if (qItem == None):
					break

				colNames = qItem.getColNames()
				row	 = qItem.getRow()

				str = "%s, %s\n" % (row, colNames)
				fd.write(str)

				self.callProcessDumpQueueFunction(row, colNames, nbItems)

				ctrDone = ctrDone + 1

			fd.write("# EOF\n")

 			m1 = "shutdownAndDump(): DONE %s of %s items written to '%s'." % (ctrDone, nbItems, filePath, )
			str = "# %s\n" % (m1, )
			fd.write(str)

			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			if (fd != sys.stderr):		# do NOT close stderr!
				fd.close()

			self.callProcessDumpQueueFunction(None, None, None)	# tell custom callback that we are done.

			self.b_shutdown_done = 2
		except:
			ex = gxutils.formatExceptionInfoAsString(2)
 			m1 = "shutdownAndDump(): FAILED while writing to filePath: '%s', %s/%s items, done, Exc: %s." % (filePath, ex, ctrDone, nbItems)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			if (fd != sys.stderr):		# do NOT close stderr!
				fd.close()

			self.b_shutdown_done = -999

	def callProcessDumpQueueFunction(self, row, colNames, nbItems):
		#
		# Optional callback to custom function:
		#
		try:
			funcName = "UnknownName"
			if self.processDumpQueueFunction:
				funcName = self.processDumpQueueFunctionName
				self.processDumpQueueFunction(self.dataPath, row, colNames, nbItems)
		except:
			ex = gxutils.formatExceptionInfoAsString(5)
			m1 = "%s - callProcessDumpQueueFunction(): CALLBACK '%s' FAILED. row=%s, ex=%s" % (self.getName(), funcName, row, ex)
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)


	def startActiveQueueMonitorThread(self):
		#
		# Before we start the Active Queue Monitor thread, we need all kinds of
		# objects to allow RPC calls and threads synchronisation.
		#

		self.queueThreadSyncLock = thread.allocate_lock()	# NOTE: threading.Lock() acquire(), release() seem bogus.


		# self.pseudoClientConn is required to send RPC later:
		#
		self.pseudoClientConn = gxclientconn.PseudoClientConn(masterToNotify=self)

		# we need a userid as we will appear in the "who" list and also, 
		# a userif is required before a RPC call is allowed:
		#
		userid = "Q_" + self.name
		self.pseudoClientConn.assignUserid(userid, operPerm=0)

		# We need a monitor and a thread:
		#
		monName    = "ActiveQueueMon_" + self.name
		threadName = "ActiveQueueThr_" + self.name

		self.queueMonitor = ActiveQueueMonitor(monName, self)

		self.queueThread = gxthread.gxthread(threadName, monitor=self.queueMonitor, threadType=gxthread.THREADTYPE_QUEUE )

		self.queueThread.start()

		m1 = "Active Queue thread started: %s." % (repr(self.queueThread), )
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


	def notifyRPCcomplete(self, b_rpc_ok, msg):
		#
		# Called by:	self.pseudoClientConn.rpcEndedReturnConnToPollList()
		#
		# NOTE: This function is a callback done by another thread, not by self.queueThread
		#
		m1 = "[RPC completed: %s, %s]" % (b_rpc_ok, msg)
		self.releaseQueueThreadSyncLock(m1, 10)


	def waitForActiveRPCcompletion(self):
		self.acquireQueueThreadSyncLock("Wait for RPC completion", 10)


	def acquireQueueThreadSyncLock(self, msg="first lock", minDebugLevel=15):
		if (getDebugLevel() >= minDebugLevel):
			m1 = "%s -- about to self.queueThreadSyncLock.acquire()... '%s'" % (self.name, msg)
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		self.queueThreadSyncLock.acquire()

		if (getDebugLevel() >= minDebugLevel):
			m1 = "%s -- OK, self.queueThreadSyncLock.acquire() DONE. '%s'" % (self.name, msg)
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


	def releaseQueueThreadSyncLock(self, msg="released by queue thread", minDebugLevel=15):
		if (getDebugLevel() >= minDebugLevel):
			m1 = "%s -- about to self.queueThreadSyncLock.release(). '%s'" % (self.name, msg)
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		self.queueThreadSyncLock.release()

		if (getDebugLevel() >= minDebugLevel):
			m1 = "%s -- OK, self.queueThreadSyncLock.release() DONE. '%s'" % (self.name, msg)
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


	def getName(self):
		return (self.name)

	def getEnableStatus(self):
		if (self.b_openPut):
			m1 = "put:Open,"
		else:
			m1 = "put:CLOSED,"

		if (self.b_openGet):
			m2 = "get:Open"
		else:
			m2 = "get:CLOSED"

		return (m1 + m2)

	def getQueue(self):
		return (self.queue)

	def isOpen(self, forWhat=0):
		#
		# Called by:	self.returnInterceptorIfReady()
		#		self.getRowsAndColNamesList()
		#
		if (forWhat == 0):
			return (self.b_openPut)

		if (forWhat == 1):
			return (self.b_openGet)

		return (self.b_openPut and self.b_openGet)

	def assignOpenStatus(self, b_openGet=None, b_openPut=None):
		#
		# Both values are Boolean, where, 1:true, 0:false
		#
		if (b_openGet == None and b_openPut == None):
			return (-1)

		if (b_openGet != None):
			self.b_openGet = b_openGet

		if (b_openPut != None):
			self.b_openPut = b_openPut

		return (0)

	def getItemRaw_withWait(self):
		#
		# Called by:	self.queueThread -- self.queueMonitor.runThread()
		#
		# Return:
		#	0	if queue is closed
		#	qItem	when an item is available
		#
		if (not self.isOpen(forWhat=1) ):
			return (0)

		qItem = self.queue.get()

		return (qItem)



	def getRowsAndColNamesList(self):
		#
		# Called by:	gxregprocs.getqProc()
		#
		# Return:
		#	0			if queue is closed
		#	-1			exception occurred
		#	None			if queue is empty
		#	(colList, rowsList)	when 1 or more rows are available
		#
		if (not self.isOpen(forWhat=1) ):
			return (0)

		qItem = self.queueGetNowait()
		if (qItem == None):
			return None		# queue is empty

		rowsList = None		# to simplify recovery later, if exception occurs
		colNames = None
		len_firstColNames = -1
		try:
			colNames = qItem.getColNames()

			len_firstColNames = len(colNames)
		
			ctrRows  = 0
			rowsList = []

			while (qItem != None):
				rowsList.append(qItem.getRow() )
				ctrRows = ctrRows + 1
				if (ctrRows >= self.maxRowsInGet):
					break

				if qItem.gxQitem_PutCtr == -1:	# was it a "held" qItem ?  if so, break out now.
					break			# make a result with only one row!

				qItem = self.queueGetNowait()
				if not qItem:		# queue empty?
					break

				if len(qItem.getColNames()) != len_firstColNames:	## result set MUST HAVE constant number of columns:
					qItem.gxQitem_PutCtr = -1	# indicates this is a "held" qItem
					self.heldQitem = qItem		# "push back" item on the "queue" (!)

					if (getDebugLevel() >= 10):
						m1 = "%s - getRowsAndColNamesList() Different colNames=%s, First colNames=%s, row=%s" % (self.name, qItem.getColNames(), colNames, qItem.getRow() )
						gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

					break				## stop that result set here.
		except:
			ex = gxutils.formatExceptionInfoAsString(3)
			m1 = "%s - getRowsAndColNamesList() FAILED, Exc: %s" % (self.name, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			if (rowsList  and  colNames):
				pass	# continue to try to return the rows
			else:
				return (-1)

		try:
			if (getDebugLevel() >= 30):
				m1 = "%s -- about to call self.postGetqFunction() %s, with columns: %s, rows: %s." % (self.name, self.postGetqFunction, colNames, rowsList, )
				gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

			if (self.postGetqFunction and rowsList and colNames):
				self.postGetqFunction(self.dataPath, colNames, rowsList)
		except:
			ex = gxutils.formatExceptionInfoAsString(5)
			m1 = "%s - getRowsAndColNamesList() CALLBACK to self.postGetqFunction() FAILED, Exc: %s" % (self.name, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

		return (colNames, rowsList)


	def returnInterceptorIfReady(self):
		#
		# Called by:	InterceptFilter.getInterceptorIfRPCparamsMatch()
		#
		# Return:
		#	self		if ready, open, queue not full
		#	(ret, msg)	if closed or queue full
		#
		if (self.queue_closed_msg):
			queue_closed_msg = self.queue_closed_msg
		else:
			queue_closed_msg = "RPC rejected: intercept Queue is closed"

		ret = gxreturnstatus.GW_QUE_SIZE	# same return status for Queue Full or Queue Closed.

		if (not self.isOpen(0) ):
			msg = "%s. (%s)" % (queue_closed_msg, self.name)
			return (ret, msg)

		if (self.queue == None):
			msg = "%s. (%s_??Q)" % (queue_closed_msg, self.name)
			return (ret, msg)

		if (not self.queue.isOpen() ):
			msg = "%s. (%s_Q)" % (queue_closed_msg, self.name)
			return (ret, msg)

		if ( (self.getQueueQsize() + 1) >= self.queueMax):	# .qsize() + 1 to allow for border effects
			if (self.queue_full_msg):
				queue_full_msg = self.queue_full_msg
			else:
				queue_full_msg = "RPC rejected: intercept Queue is full"

			msg = "%s. (%s_Q)" % (queue_full_msg, self.name)
			return (ret, msg)

		return (self)


	def processRows(self, resSetNo, colNames, rowsList, nbRows, dictColNames, frame, ret_stat):
		#
		# Called by:	self.checkColNamesAndProcessRows()
		#

		# check if there is a custom result intercept function, if so, call it...
		#
		if (self.resultFunction != None):
			rc = self.callCustomInterceptResponse(frame, resSetNo, colNames, rowsList, ret_stat)

			if (getDebugLevel() >= 10):
				m1 = "%s - captureResponse(): rc = %s from self.callCustomInterceptResponse()." % (self.name, rc)
				gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

			if (rc <= -1):		# if rc <= -1, error occurred, no further interception of results.
				return rc
			if (rc >= 1):		# if rc >= 1, the current result set was intercepted correctly
				return 0	# return 0 to allow interception of next result set
			#
			# otherwise, rc == 0 and we will try to add the rows to the queue...
			#
		ctrRowsQueued  = 0
		ctrQputFailed  = 0
		ctrOtherErrors = 0
		for row in rowsList:
			rc = self.processOneRow(dictColNames, colNames, row, (ret_stat == 0) )
			if (rc == 0):
				ctrRowsQueued = ctrRowsQueued + 1
			elif (rc == -1):
				ctrQputFailed = ctrQputFailed + 1
			else:
				ctrOtherErrors = ctrOtherErrors + 1

		if (ctrRowsQueued > 0):
			self.notifyGateway(ctrRowsQueued)

		if (ctrRowsQueued != len(rowsList) ):
			m1 = "%s - Result set queueing FAILED: %s rows, %s queued, %s err put(), %s other err. Proc:'%s'." \
				% (self.name, len(rowsList), ctrRowsQueued, ctrQputFailed, ctrOtherErrors, frame.getProcName() )

			gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)


		if (getDebugLevel() >= 3):
			m1 = "%s - END processRows(): '%s', %s rows queued." % (self.name, 
									frame.getProcName(), ctrRowsQueued)
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		return 0	# return 0 to allow interception of next result set
		## return (ctrRowsQueued)


	def processOneRow(self, dictColNames, colNames, row, b_rpc_succeeded):
		#
		# Called by:	self.addResultSetToQueue()
		#
		rc = -6
		if (b_rpc_succeeded):
			rc = -5
			try:
				# all values in the row are string, but, some should be integer or float.
				# we must normalize them before adding the row to the queue:
				#
				rc = self.normalizeRowValues(colNames, row)
				if (rc != 0):
					return rc

				# For ACTIVE_QUEUE, qItem contains a params list, not a row
				# and the colNames.
				#
				if (self.b_active):
					paramsList = self.buildParamsListFromRowAndColNames(colNames, row)
					rc = self.queueRpcParamsList(paramsList, interceptor=0)  # interceptor=0 because we do not want another interception when the RPC is sent.
				else:
					qItem = InterceptQueueItem()		# object to hold the row
					qItem.assignValues(colNames, row)
					rc = self.queue.put(qItem, block=0)	# do NOT block if the queue is full, report an error immediately
			except:
				ex = gxutils.formatExceptionInfoAsString(2)

				m1 = "%s - processOneRow() FAILED: rc=%s, Exc: %s, colNames: %s, row: %s." % (self.name, rc, ex, colNames, row) 
				gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
		else:
			rc = 0

		self.logTransId(dictColNames, row, b_rpc_succeeded, rc)

		return (rc)

	def queueOneRow(self, colNames, row):
		#
		# Called by:	custom module(s)
		#
		# Return:
		#	0	success
		#	-5	exception was caught, see log for details.
		#	-1	queue closed -- see gxqueue.py put() API.
		#	-2	queue full
		#
		# NOTE: Both colNames and row must be usual structure used in result set here.
		#
		rc = -5
		try:
			qItem = InterceptQueueItem()		# object to hold the row

			qItem.assignValues(colNames, row)

			rc = self.queue.put(qItem, block=0)	# do NOT block if the queue is full, report an error immediately

			if 0 == rc:
				self.notifyGateway(1)
		except:
			ex = gxutils.formatExceptionInfoAsString(5)

			m1 = "%s - queueOneRow() FAILED: rc=%s, Exc: %s, colNames: %s, row: %s." % (self.name, rc, ex, colNames, row)

			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

		return (rc)


	def queueRpcParamsList(self, paramsList, interceptor=None):
		#
		# Called by:	self.processOneRow() -- without interceptor arg
		#		custom module(s) -- could be with interceptor arg.
		#
		# Return:
		#	0	success
		#	-5	exception was caught, see log for details.
		#	-1	queue closed -- see gxqueue.py put() API.
		#	-2	queue full
		#
		# NOTE: paramsList must contain normalized values (Python string, integer or float).
		#	paramsList must conform to the usual RPC params list used here. It will be used directly.
		#
		rc = -5
		step = "?"
		try:
			step = "put()"
			qItem = InterceptQueueItem()		# object to hold the paramsList

			qItem.assignValues(interceptor, paramsList)

			rc = self.queue.put(qItem, block=0)	# do NOT block if the queue is full, report an error immediately

			step = "CALLBACK"
			if self.postPutqFunction:
				self.postPutqFunction(self.dataPath, paramsList, rc)
		except:
			ex = gxutils.formatExceptionInfoAsString(5)

			m1 = "%s - queueRpcParamsList() %s FAILED: rc=%s, Exc: %s, paramsList: %s." % (self.name, step, rc, ex, paramsList)

			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

		return (rc)


	def buildParamsListFromRowAndColNames(self, colNames, row):
		#
		# NOTE: a param tuple is like this: (name, value, jType, isNull, isOutput)
		#
		paramsList = []
		i = 0
		for col in colNames:
			if (len(col) == 3):
				name, jType, sz = col
			else:
				sz = None
				name, jType = col
			value, isNull = row[i]
			i = i + 1
			name = "@p_" + name
			if (sz != None):
				param = (name, value, jType, isNull, 0, sz)
			else:
				param = (name, value, jType, isNull, 0)
			paramsList.append(param)

		return paramsList


	def logTransId(self, dictColNames, row, b_rpc_succeeded, rc_queue):
		#
		# si self.trans_id est None, alors pas besoin de logger le trans_id
		#  (mais si debug >= 5, alors, il faut logger le put dans la queue)
		# sinon (si self.trans_id est un nom de colonne), il faut ramasser la valeur
		# et ecrire une ligne dans le log (qui dit aussi si le put queue est OK)
		#
		b_mustLog = 0
		level = TxLog.L_MSG
		if (not self.trans_id and not self.no_Trans_id_SaidAlready):
			self.no_Trans_id_SaidAlready = 1
			m1 = "NOTE: queue '%s' definition does not have a trans id column name." % (self.name, )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		if (b_rpc_succeeded):
			marker = self.logMarker

			if (rc_queue == 0):
				msg   = "OK, queued to be sent."
				level = TxLog.L_MSG
			else:
				msg   = "Add to queue FAILED. rc=%s." % (rc_queue, )
				level = TxLog.L_ERROR
				b_mustLog = 1
		else:
			b_mustLog = 1
			marker = self.logMarkerFailed
			msg    = "Row with trans_id found in Failed RPC."
			level  = TxLog.L_WARNING

		if (self.trans_id):
			b_mustLog = 1
			xid = self.getTransIdFromRow(dictColNames, row)
			if (xid == None):
				msg = msg + " WARNING: xid is None/Null."
				level = TxLog.L_WARNING

			m1 = "%s %s %s - %s" % (marker, xid, msg, self.name)
		else:
			m1 = "%s - %s" % (msg, self.name)
			xid = -1

		if (b_mustLog  or  getDebugLevel() >= 5):
			gxServer.writeLog(MODULE_NAME, level, 0, m1)




	def getTransIdFromRow(self, dictColNames, row):
		idx = self.getTransIdColumnIndex(dictColNames)

		if (idx <= -1):		# maybe there is no transId column
			return None

		return (self.getValueOfColumnIdx(row, idx) )

	def getValueOfColumnIdx(self, row, idx):
		tup = None

		try:
			tup = row[idx]
			val, isNull = tup
			if (isNull):
				return None
			return val
		except:
			if (self.b_told_error_in_getValueOfColumnIdx >= 5):
				return None

			ex = gxutils.formatExceptionInfoAsString(5)

			self.b_told_error_in_getValueOfColumnIdx = self.b_told_error_in_getValueOfColumnIdx + 1

			m1 = "%s - getValueOfColumnIdx(): idx=%s, tup=%s, Exc: %s." % (self.name, idx, tup, ex) 
			gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)

			if (self.b_told_error_in_getValueOfColumnIdx >= 5):
				m1 = "%s - getValueOfColumnIdx(): maximum of error messages reached." % (self.name, )
				gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
		return None			


	def getTransIdColumnIndex(self, dictColNames):
		if (not dictColNames):
			return (-1)
		try:
			return (dictColNames[self.trans_id] )
		except KeyError:
			if (getDebugLevel() >= 10):
				m1 = "%s - trans_id column '%s' not found by getTransIdColumnIndex()." % (self.name, self.trans_id)
				gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
			return (-1)


class InterceptFunction(InterceptQueueSuperClass):
	#
	#	an InterceptFunction has:
	#	   paramFunctionName	:	a string that can be mapped to...
	#	   paramFunction	:	 a Python function
	#	   paramConfArgs	:	arguments from the config, 
	#					 to be passed to the function
	#	   resultFunctionName	:	string
	#	   resultFunction	:	function
	#	   resultConfArgs	:	arguments from the config,
	#					 to be passed to the function
	#
	def __init__(self, name):
		self.name    = name

		self.columns		= None	# to suport hybrid behaviour of .buildDictColumnsAndCheckIfHasRequiredColumns()
		self.trans_id		= None

		self.paramFunctionName	= None
		self.paramFunction	= None
		self.paramConfArgs	= None

		self.resultFunctionName	= None
		self.resultFunction	= None
		self.resultConfArgs	= None

		self.b_intercept_return_status = 0

		self.initAttributes()

		dictOfInterceptQueues[self.name.lower()] = self


	def __repr__(self):
		try:
			x = self.name
		except:
			self.name = "[no name]"
		s = "<InterceptFunction: %s>" % (self.name, )
		return s

	def initAttributes(self):
		try:
			paramName = self.name + gxparam.GXPARAM__COLUMNS
			self.columns = gxServer.getParamValuesList(paramName)
		except RuntimeError:
			pass	# this config param is optional. Don't fuss if it is absent.

		if (self.getCustomFunctionsParams() != 0):
			m1 = "create InterceptFunction '%s' FAILED in getCustomFunctionsParams(). See log." % (self.name, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			raise RuntimeError, "InterceptFunction.initAttributes() failed"

		if (self.paramFunction == None  and  self.resultFunction == None):
			m1 = "create InterceptFunction '%s' FAILED: NO BINDINGS to both paramFunction '%s' and resultFunction '%s'." % (self.name, self.paramFunctionName, self.resultFunctionName, )
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

			raise RuntimeError, "InterceptFunction.initAttributes() failed"


	def interceptParams(self, paramList):
		#
		# Called by:	InterceptFilter.getInterceptorIfRPCparamsMatch()
		#
		#
		# Pass the paramList to the custom params intercept function.
		#
		# The custom function must return:
		#	0	if the interception must proceed
		#	<= -1	if an error occurred
		#	>= 1	if no interception should be done.
		#
		try:
			if (self.paramFunction == None):
				return (0)		# no binding to internal function, it's OK.

			if (getDebugLevel() >= 10):
				print "params:", paramList

			if (paramList == None or len(paramList) == 0):
				return (0)		# OK, proceed with intercept anyway.

			#
			# Call the custom function now:
			#
			rc = self.paramFunction(self, paramList, self.paramConfArgs)

			return (rc)
		except:
			ex = gxutils.formatExceptionInfoAsString()

			m1 = "%s - Failed in call to custom param intercept function '%s', Exc: %s" % (self.name, self.paramFunctionName, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

		return -1

	def returnInterceptorIfReady(self):
		#
		# Called by:	InterceptFilter.getInterceptorIfRPCparamsMatch()
		#
		# Return:
		#	self		if ready, open, queue not full
		#	(ret, msg)	if closed or queue full
		#
		return (self)	# an InterceptFunction has no state (here), it's always ready.


	def captureResponse(self, frame, resp):
		#
		# Called by:	gxInterceptRPCMonitor.interceptResponse() as:  frame.interceptor.captureResponse(frame, resp)
		#
		#
		# Pass the result_sets_list to the custom intercept function.
		#
		if (self.resultFunction == None):
			return None		# no binding to internal function, do nothing.

		rc = -99
		try:
			ret_stat	 = resp[4]
			result_sets_list = resp[0]

			resSetNo = 0
			for res in result_sets_list:
				colNames = res[0]
				rowsList = res[1]
				rc = self.processRows(resSetNo, colNames, rowsList, 0, None, frame, ret_stat)
				if (rc != 0):
					break
				resSetNo = resSetNo + 1

			if self.b_intercept_return_status and not result_sets_list:
				rc = self.callCustomInterceptResponse(frame, -1, [], [1,], ret_stat)	# [1,] is a trick
		except:
			ex = gxutils.formatExceptionInfoAsString(5)
			m1 = "%s - captureResponse() got Exc: %s" % (self.name, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-22)

		return (rc)

	def processRows(self, resSetNo, colNames, rowsList, nbRows, dictColNames, frame, ret_stat):
		return self.callCustomInterceptResponse(frame, resSetNo, colNames, rowsList, ret_stat)

	def buildDictColumnsAndCheckIfHasRequiredColumns(self, colNames):
		"""This method and the optional <queue>_COLUMNS config parameter allows 
		   an InterceptFunction to be used properly within a InterceptQueueSwitch.
		"""
		if not self.columns:
			return self.buildDictColumnsAndCheckIfHasRequiredColumns_CustomCallback(colNames)
		else:
			return InterceptQueueSuperClass.buildDictColumnsAndCheckIfHasRequiredColumns(self, colNames)

	def buildDictColumnsAndCheckIfHasRequiredColumns_CustomCallback(self, colNames):
		#
		# NOTES: This method is a special implementation of .buildDictColumnsAndCheckIfHasRequiredColumns()
		#
		# This method is only called if this InterceptFunction instance is 
		# referenced by an InterceptQueueSwitch. (The config file controls that).
		#
		# The purpose of this method (when an InterceptFunction is the first
		# of 2 queues in a InterceptQueueSwitch), would be to call a custom
		# self.resultFunction() which would trigger (or not) some event 
		# depending of the content of colNames and self.resultConfArgs
		#
		# Because this method return None, the InterceptQueueSwitch will always
		# let the second queue try to process the result.
		#
		# Return:	None	(always)
		#
		try:
			if (self.resultFunction == None):
				return None		# no binding to internal function, do nothing.

			if (getDebugLevel() >= 20):
				m1 = "%s - buildDictColumnsAndCheckIfHasRequiredColumns(): about to call resultFunction() '%s', with columns: %s. " % (self.name, self.resultFunctionName, colNames, )
				gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

			#
			# Call the custom function now:
			#
			rc = self.resultFunction(self, None, None, colNames, None, None, self.resultConfArgs)

			return None
		except:
			ex = gxutils.formatExceptionInfoAsString(10)

			m1 = "%s - buildDictColumnsAndCheckIfHasRequiredColumns() failed when calling to custom result intercept function '%s', Exc: %s" % (self.name, self.resultFunctionName, ex)
			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)

		return None

	#
	# End of class InterceptFunction
	#

class InterceptNotifyUDP(InterceptQueueSuperClass):
	"""This class is an interceptor for the interception of type NOTIFY_UDP
	"""
	def __init__(self, name):
		self.name    = name

		self.columns		= None	# to suport hybrid behaviour of .buildDictColumnsAndCheckIfHasRequiredColumns()
		self.trans_id		= None	# it has to be None see .addResultSetToQueue() and .buildDictColumnsAndCheckIfHasRequiredColumns() in super class
		self.paramFunctionName	= None

		self.notifyUDPaddress	= None
		self.b_active		= False	# to please .sendUDPcmdWithOptions() -- in super class
		self.notifyInterval	= 0	# to please the debug trace in .notifyGateway() -- in super class
		self.maxRowsInGet	= 0	# to please the debug trace in .notifyGateway()
		self.udp		= None	# will be built by .prepareUDPsocketToGateway() -- in super class
		self.ctrNotifySent	= 0

		if (self.initAttributes() <= -1):
			raise RuntimeError, "InterceptNotifyUDP.initAttributes() failed"

		self.prepareUDPsocketToGateway()	# will assign self.udp

		dictOfInterceptQueues[self.name.lower()] = self

	def __repr__(self):
		try:
			x = self.name
		except:
			self.name = "[no name]"
		s = "<InterceptNotifyUDP: %s>" % (self.name, )
		return s

	def getStatusAsRow(self):
		""" Override method in super class RPCinterceptor"""		
		##a = "[no stats available for this kind of interceptor]"
		i = 0
		n = 0.0
		return [ self.name, 1, 1, 0, 99, float(self.ctrNotifySent), n, n, i ]

	def initAttributes(self):
		try:
			paramName = self.name + gxparam.GXPARAM__COLUMNS
			self.columns = gxServer.getParamValuesList(paramName)
		except RuntimeError:
			pass	# this config param is optional. Don't fuss if it is absent.

		try:
			paramName = self.name + gxparam.GXPARAM__NOTIFY_UDP_ADDRESS
			self.notifyUDPaddress = gxServer.getMandatoryParam(paramName)
		except:
			ex = gxutils.formatExceptionInfoAsString(10)
			m1 = "Failed to create InterceptNotifyUDP '%s': missing param '%s'. Exc: %s." % (self.name, paramName, ex)

			gxServer.writeLog(MODULE_NAME,TxLog.L_ERROR, 0, m1)
			return (-1)

		return (0)	


	def returnInterceptorIfReady(self):
		#
		# Called by:	InterceptFilter.getInterceptorIfRPCparamsMatch()
		#
		# Return:
		#	self		if ready, open, queue not full
		#	(ret, msg)	if closed or queue full
		#
		return (self)	# an InterceptNotifyUDP has no state (here), it's always ready.

	def processRows(self, resSetNo, colNames, rowsList, nbRows, dictColNames, frame, ret_stat):
		#
		# Called by:	self.checkColNamesAndProcessRows() -- in super class
		#
		self.notifyGateway(len(rowsList) )

		self.ctrNotifySent = self.ctrNotifySent	+ 1

		if (getDebugLevel() >= 5):
			m1 = "%s - END processRows(): '%s' Notify UDP sent." % (self.name, frame.getProcName(), )
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		return 0	# return 0 to allow interception of next result set


if __name__ == '__main__':
	print "No UNIT test"

