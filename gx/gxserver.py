#!/usr/bin/python
#
# gxserver.py -- Gateway XML-TP Server:  main module
# -----------
#
# $Source: /ext_hd/cvs/gx/gxserver.py,v $
# $Header: /ext_hd/cvs/gx/gxserver.py,v 1.8 2006/02/02 14:53:14 toucheje Exp $
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
# gxserver.py -- Gateway XML-TP Server:  main module
# -----------
# Contains gxserver class and various subclasses of gxrespool and gxmonitor.
# 
# GENERAL NOTES:
# =============
# The gxserver program is a socket server which replies to RPC requests.
#
# There are many threads in the gxserver, some are service threads,
# some are threads which are processing RPC or regproc calls.
# (You can see the various threads in the server if you use the "ps"
# operator command).
#
# HOW RPC Requests are processed:
# ------------------------------
# The "ClientsPollThread" use the gx_poll.c module to handle many socket 
# connections. When a new connection comes is, gx_poll.c accept() it and
# adds the fd to its internal poll_list[]. When an input event occurs,
# the fd which has it is placed in the inputList[]. The call to the
# gx_poll module returns that list to the ClientsPollThread function.
# This function adds all the fd(s) in that list to the Call Parser queue 
# by calling addClientRequestToCallParserQueue() for each fd.
#
# The processing of a RPC request begins with addClientRequestToCallParserQueue().
# Each RPC request must be processed through the following steps:
#	1. basic validity check
#	2. parse the XMLTP proc call
#	3a. forward the RPC and stream back the response, or,
#	3b. execute a registered procedure (aka, regproc), such as "login"
#	4. send back a complete response to the client connection (if this
#	   was not done entirely at step 3a or 3b)
#	5a. return the socket fd in the poll_list[] of gx_poll, or,
#	5b. disconnect the client connection.
#
# WARNING: to fulfill the XMLTP protocol with the client connection,
#	   the gxserver must always reply with a response with valid
#	   XMLTP syntax, or, disconnect the socket.
#	   If the server does not disconnect the client, it must
#	   put back the fd in the inputList of gx_poll.
#
# STEP 1:
# If the validity check at step 1. indicates that the data received 
# are spurious bytes or that the client connection is already considered
# disconnected, steps 2 to 4 are not done, and, step 5b, disconnect is 
# the only subsequent step.
#  Example case: if a "login" proc call was padded with extra bytes after
#		 its </EOT>, the socket buffer might generate one more
#		 input event (depending on various implementation details)
#		 before the disconnect clean-up actions are all done.
#
# NOTE: after a failed "login", the client must disconnect. The gxserver
#	should close() the client socket. Normally, no further should
#	come from the same client connection. If one comes, it must be
#	rejected.
#
# If there are no client connection with has this "fd", one is created
# in step 1.
#
# STEP 2:
# First, the request (a RPC frame bound to this client connection) is
# placed in the Call parsers queue.
# Eventually, it comes out of the queue and it is processed by one 
# of the call parser of the call parsers pool.
# If the socket was found to be disconnected, the next step will be 5b.
# If there was an error in the parsing, an error msg is send to the 
# client (step 4).
# Otherwise the Proc Call structure contains a procName that is 
# either the name of a regProc or of a RPC.
# Calls to a regProc are placed in the queue of the "RegProcExecPool".
# Calls to a RPC are routed (queued) to their matching DestConnPool.
#
# NOTE: if the client has not already successfully done a "login",
#	the only request accepted is a regproc call to "login".
#
# STEP 3a, 3b:
# In both cases, a complete XMLTP response should be sent back to the
# client connection.
# If an exception occurs, step 4 must recover.
#
# STEP 4:
# If something went wrong in step 3a or 3b, this step sends back a
# response to the client connection.
#
# STEP 5a, 5b:
# The client connection's fd must be returned to the gx_poll inputList
# or be disconnected (after a failed login or special condition).
#
#
#-------------------------------------------------------------------------
# 2001nov14,jft: first version
# 2002jan29,jft: . moved CallParserResPool to gxcallparser.py
# 2002feb15,jft: + gxserver.getCheckPasswordDestConnPool()
#		 + gxserver.getNameOfAuthenticationRPC()
# 2002feb20,jft: . gxserver.main(): signal.signal(signal.SIGPIPE, signal.SIG_IGN)
#				    signal.signal(signal.SIGHUP,  signal.SIG_IGN)
# 2002mar02,jft: . gxserver.pollOneTime(): gx_poll.pollClients(3, ...) # 3 ms, was 20 ms before
#		 . gxserver.initialize(): gxintercept.assignDebugLevel(self.debugLevel)
# 2002mar06,jft: . gxserver.createDestPool(): get more params from config: sendTimeout, recvTimeout, loginProc
# 2002mar07,jft: . gxserver.addClientRequestToCallParserQueue(): enforce limit of number of clients sockets
#			Also, does: sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout_val)
#		 . gxserver.initialize(): init self.max_clients_sockets, self.max_clt_connections,
#			self.client_recv_timeout, self.client_send_timeout from config params.
#		 + gxserver.isRPCgateOpen(self):
#		 + gxserver.getListOfClosedGates(self):
#		 + gxserver.checkIfValidRPCgroupGateName(self, groupName):
#		   ...
# 2002mar26,jft: . gxserver.addClientRequestToCallParserQueue(): sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
# 2002apr14,jft: . gxserver.addClientRequestToCallParserQueue(): changed call 
#			"self.returnClientConnToPollList(conn_sd)" to conn.rpcEndedReturnConnToPollList(0, msg)
# 2002may28,jft: . ClientsPollMonitor.pollOneTime(): if debugLevel >= 20, trace (addList, dropList )
# 2002jun29,jft: . gxserver.createDestPool(): assign destPool.maxWaitInQueue
# 2002sep17,jft: . gxserver.isThisRPCallowed(): when checking (not self.b_rpc_allowed), should check also (and  not gxregprocs.isRegProcName(rpcName) )
#						also, check self.dictRPCsBlockedByGates.has_key(prefix)
# 2002oct02,jft: . gxserver.startUp(): for each destConn, call dest.queueTestRPCtoConnect()
# 2002nov27,jft: . gxserver.initialize(): get port number from xmltp_hosts.getServerHostAndPort(self.srvName)
#					  assign xmltp_hosts.srvlog, register it with TraceMan
#		 . gxserver.createDestPool(): call buildRoutesDict()
#					      remember destPool.passwordFile (used to be destPool.password)
#		 + gxserver.buildRoutesDict()
# 2002nov30,jft: + gxserver.isValInParamValuesList(str, paramName)
#		 . gxserver.assignRPCsGroupGateState(): fixed spurious errmsg: "Strange, '%s' is closed, but, cannot del dictRPCsBlockedByGates['%s']" % (groupName, rpcName)
# 2003jan19,jft: . gxserver.initialize(): support for turning off RPC forwarding when DEFAULT_DESTINATION and DESTINATIONS are "nil"
# 2003feb08,jft: . ClientsPollMonitor.pollOneTime(): do .processDiscList(x[1]) before .processInputList( x[0] )
#		 . ClientsPollMonitor.runThread(): if debug level higher, get pollTab more often.
# 2003feb09,jft: . gxserver.startUp(): log compile and RCS versions from  gx_poll.getVersionId(b_RCS)
# 2003feb10,jft: . ClientsPollMonitor.pollOneTime(): x = gx_poll.pollClients(...), conditionally display diagnostic msg when x is a tuple of len == 2
#		 . Class gxserver: fix "disconnect_connect_fd_race_bug": removed self.queueOfClientsToDropFromPollList,
#			- removed addClientToListOfThoseToRemoveFromPollList()
# 2003mar05,jft: + with "kick gateway" daemon thread in gxintercept.py
#		 . gxserver.getShortStatus(): also show gxintercept.getEnableInterceptStatus()
# 2003jun14,jft: . gxserver.loadConfig(): if no (self.cfgFile + "." + self.hostname), try to load local config from (self.cfgFile + ".localhost") 
# 2003jul04,jft: + gxserver.getGatesStatusAsRow(), gxserver.getGatesStatusAsRowHeaders()
#		 + gxserver.getStatusAsRow(),      gxserver.getStatusAsRowHeaders()
#		 + gxserver.getAllDestConnPools(), 
# 2003jul11,jft: . gxserver.getGatesStatusAsRow(): one row per gate
# 2003jul23,jft: . ClientsPollMonitor.runThread(): log "gx_poll.getPollTab():..." less often. Better intervals depending on trace level.
# 2003jul24,jft: . gxserver.startUp(): adjust trace levels at start up time, according to params listed by TRACE_INIT
# 2003jul29,jft; . gxserver.getGatesStatusAsRow(), .getGatesStatusAsRowHeaders(): added optional arg b_oneRowManyCols=0
# 2004nov03,jft: . gxserver.startUp() always call self.adjustInitialTraceLevels()
# 2005aug01,rna: gxserver.main() trap exception AttributeError when trying to install signal handling
#                this is to permit running the gxserver inside a thread of another process             
# 2005nov02,jft: . gxserver.initialize() if cfg param PRE_EXEC_LOG exists, then, assign self.preExecLogFunc = preexeclog.writePreExecLog
# 2006jan29,jft: + if the server aborts, repeat the critical log messages just before exit.
#		 + gxserver.addErrMsg()
#		 . gxserver.initialize(): call self.addErrMsg() before all return -1
# 2006jan31,jft: . gxserver.initialize(): gxbdbqueue.gxServer = self
#		 . traceMan.register("gxbdbqueue",    gxbdbqueue,	  "intercept")
#		 . traceMan.register("bdbvarqueue",   bdbVarQueue,	  "intercept")
# 2006feb01,jft: . gxserver.createDestPool(): initialize destPool.destSrvName = None
#			

import sys
import time
import socket
import threading		# for threading.currentThread()
import os			# for os.uname(), os.close()
import os.path			# for os.path.exists()
import signal			# to ignore SIGHUP and SIGINT
import struct			# for struct.pack()

import preexeclog
import traceMan
import TxLog
import Cfg
import xmltp_hosts		# to find the port number to listen on
import gxparam			# for names of config parameters
import gxthread
import gxqueue
import gxbdbqueue		# to set gxbdbqueue.gxServer
import bdbVarQueue		# to bind it to traceMan
import gxmonitor
import gxrespool
import gxdestconn		# for DestConn and DestConnPool
import gxrpcframe
import gxclientconn
import gxintercept		# for gxintercept.initInterceptDefinitions()
import gxrpcmonitors
import gxregprocs
import gxxmltpwriter
import gxutils			# contains formatExceptionInfoAsString()
import gxreturnstatus
import gxconst
import gxcallparser
import gx_poll


#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxserver.py,v 1.8 2006/02/02 14:53:14 toucheje Exp $"

#
# --- Module Constants:
#
MODULE_NAME = "gxserver"

PARAM_PRE_EXEC_LOG = "PRE_EXEC_LOG"

#
# --- Open the server LOG with a default filename.
# --- It will be re-assigned to another name after the config file
# --- is loaded.
#
if __name__ == '__main__':
	srvlog = TxLog.TxLog("gxserver.log", "gxserv01")
	srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Loading gxserver.py...")
else:
	srvlog = None		# must be initialized by the module which is importing this one!


#
# --- Module Functions:
#
def logException(monitor, aThread):
	cla, exc, trbk = sys.exc_info()
	excName = cla.__name__

	excStr = gxutils.formatExceptionInfoAsString()

	m1 = "Exception '%s' in %s, %s. %s" %  (excName, monitor, aThread, excStr)

	srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)




#
# --------------------- Classes:
#

# The Clients Connections Poll() monitor:
#
class ClientsPollMonitor(gxmonitor.gxmonitor):
	
	def runThread(self, aThread):
		#
		# NOTE on USAGE of:
		#	x = gx_poll.getPollTab(0 | 1)
		#
		# gx_poll.getPollTab() is bound to the following "C" function is gx_poll.c :
		#
		#	PyObject *gx_poll_get_poll_tab(PyObject *self, PyObject *args)
		#	/*
		#	 * Args (PyObject):	int :	b_counters_only
		#	 *
		#	 * Return:
		#	 * 	(PyObject *) (Tuple): a Tuple of (6) elements:
		#	 *				- List of active fd (always empty if b_counters_only is true)
		#	 *				- s_high_ctr_poll_fd_used
		#	 *				- ctr of (fd != -1) in the poll_tab[]
		#	 *				- highest fd value
		#	 *				- the last error message
		#	 *				- the priority of the last error message (0 if not error).
		#	 *  or
		#	 * 	(PyObject *) (String): error message explaining error
		#	 */
		#
		b_fatalError = 0
		iter = 0

		self.tOut = 0.0		# used in .pollOneTime()

		while(not self.master.b_finalStop):
			try:
				self.pollOneTime(aThread)
			except:
				logException(self, aThread)
				self.ctrErrors = self.ctrErrors + 1

			if (self.ctrErrors >= 20):
				b_fatalError = 1
				break

			iter = iter + 1
			if (self.master.getDebugLevel() >= 40):
				each_n_iter = 100		# every 2 seconds
			elif (self.master.getDebugLevel() >= 30):
				each_n_iter = 250		# every 5 seconds
			elif (self.master.getDebugLevel() >= 20):
				each_n_iter = 500		# every 10 seconds
			elif (self.master.getDebugLevel() >= 10):
				each_n_iter = 1500		# every 30 seconds
			else:
				each_n_iter = 3000		# every 1 minute
			if (iter >= each_n_iter):
				iter = 0			# reset counter to zero
				try:
					x = gx_poll.getPollTab(0)	# (0): get list of active fd from poll_tab[]
				except:
					excStr = gxutils.formatExceptionInfoAsString(3)
					x = "getPollTab() failed: %s" % (excStr, )

				m1 = "gx_poll.getPollTab(): %s" % (x, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )

		if (b_fatalError):
			m1 = "ERROR: too many errors, Clients poll() loop ABORTING. %s, %s" % (self, aThread)
			srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )

			aThread.changeState(gxthread.STATE_ABORTED)
			#
			# get more details to help to diagnose...
			#
			try:
				x = gx_poll.getPollTab(0)	# (0): get list of active fd from poll_tab[]
			except:
				excStr = gxutils.formatExceptionInfoAsString(2)
				x = "getPollTab() failed: %s" % (excStr, )

			m1 = "gx_poll.getPollTab(): %s" % (x, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )


		aThread.changeState(gxthread.STATE_SHUTDOWN)
		sys.exit()

	def pollOneTime(self, aThread):
		aThread.changeState(gxthread.STATE_PREPARE)
		addList = []
		while(1):
			x = self.master.getClientToReturnToPollList()
			if (x == None):
				break
			addList.append(x)

		if (self.tOut != 0):
			tNow = time.time()
			lag = tNow - self.tOut
			self.master.pollProcessStats.addToStats(lag)

		aThread.changeState(gxthread.STATE_POLL)

		if (self.master.getDebugLevel() >= 20 and addList ):
			m1 = "poll: addList=%s." %  (addList, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )

		# poll() all waiting client connections:
		#
		x = gx_poll.pollClients(3, addList)		# was  20 ms before (2002mar02,jft)

		self.tOut = time.time()		# used to accumulate stats about lag

		aThread.changeState(gxthread.STATE_ACTIVE)

		if (x == None):
			return None	# timeout, no activity

		if (type(x) == type("str") ) :
			m1 = "gx_poll.pollClients() last msg: '%s'." % (x, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1 )
			return x

		if (len(x) == 2):	# debug, diagnostic or warning message
			err_prio, err_msg = x
			if (err_prio >= 500 or self.master.getDebugLevel() >= 20):
				m1 = "gx_poll.pollClients() last msg: '%s', prio=%s." % (err_msg, err_prio, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )
			return x

		# then, x must be a list... and it must contains:
		#	- List of fd with input available
		#	- List of fd that have disconnected
		#	- return code (rc), rc <= -1 indicates an error
		#	- the last error message
		#	- the priority of the last error message (0 if not error).
		#
		if ((len(x[0]) > 0 or len(x[1]) > 0) and self.master.getDebugLevel() >= 5):
			m1 = "%d input, %d disc, x=%s" % (len(x[0]), len(x[1]), x)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )

		if (self.master.b_finalStop):
			return (1)

		self.processDiscList(  x[1] )	# process the disconnect first
		self.processInputList( x[0] )
		self.processErrMsg( x[2], x[3], x[4] )

	def processDiscList(self, discList):
		if (len(discList) <= 0):	# trivial case: nothing to do
			return None

		if (self.master.getDebugLevel() >= 5):
			m1 = "%d clients have disconnected: %s" % (len(discList), discList )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )


		for sd in discList:
			conn = gxclientconn.getClientConnFromSd(sd)
			if (conn != None):
				m1 = "Disconnect: %s" % (conn, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )

				conn.processDisconnect()
			else:
				m1 = "Disconnect: sd=%s (no clientConn)" % (sd, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )


		
	def processInputList(self, inputList):
		if (self.master.getDebugLevel() >= 15):
			m1 = "%d req from clients: %s" % (len(inputList), inputList )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )

		for req in inputList:
			self.master.addClientRequestToCallParserQueue(req)
			##
			##  to be fine-tuned to detect disconnect @@@@


	def processErrMsg(self, rc, errMsg, msgPrio):
		if (rc == 0 and (msgPrio == 0 or (msgPrio <= 200 and self.master.getDebugLevel() < 10) ) ):
			return None

		m1 = "gx_poll.pollClients(): rc=%s, '%s', prio=%s" % (rc, errMsg, msgPrio)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1 )

		



class RegProcExecRes(gxrespool.gxresource):
	def processRequest(self, rpcFrame):
		threadIdSuffix = repr(self.pool.incrementGetThreadsCtr() )

		t = gxthread.gxthread(self.name + "Thr" + threadIdSuffix,
					monitor  = rpcFrame.getMonitor(),
					resource = self,
					frame    = rpcFrame,
					pool     = self.pool,
					threadType=gxthread.THREADTYPE_REGPROC )


		self.assignUsedByThread(t)

		t.start()


class RegProcExecThreadsPool(gxrespool.gxrespool):
	def createNewResource(self, aList): 
		#
		# this function receives the pool.resCreationArgsList
		# as argument (aList) and it creates a new resource.
		#
		try:
			res = RegProcExecRes(name=self.name + "Res" + repr(self.resCreatedCtr), 
					 monitor = None,	# will be assigned when destConn gets used
					 data  = None,		# no resource data for regproc execution
					 owner = self.reqQueueThread, 
					 pool  = self)

			m1 = "RegProcExecRes created: %s" % (res, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		except:
			excStr = gxutils.formatExceptionInfoAsString()

			m1 = "%s create RegProcExecRes FAILED. Exception: %s." % (self.getName(), excStr)

			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

			return None

		return (res)

### 
### moved class DestConn, DestConnPool to gxdestconn.py  (2002feb02,jft)
### ---------------------------------------------------

#
# to create custom versions, you can subclass the following class, gxserver.
#
class gxserver:
	def __init__(self, srvName="gxserv01", srvType=None, cfgFile=None, debugLevel=1):
		self.srvName    = srvName
		self.srvType	= srvType
		self.cfgFile	= cfgFile
		self.debugLevel = debugLevel

		# if the config file has a PRE_EXEC_LOG parameter, preexeclog.writePreExecLog will be assigned to self.preExecLogFunc
		#
		self.preExecLogFunc = None		

		self.serverMode = gxconst.MODE_PRIMARY		# !!! will depend on config !!!
		self.b_finalStop = 0

		self.initDoneOK = 0
		self.ctrErrors  = 0
		self.errMsg     = []

		m1 = "Creating gxserver '%s', cfg=%s, debugLevel=%d." % (srvName, cfgFile, debugLevel)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		print m1

		# these will be created by initialize():
		#
		self.cfg		= None
		self.hostname		= None

		self.dictOfRPCgroups	= None      # loaded by initialize() -- mandatory --

		self.dictGatesClosed        = {}    # initially empty. Gates can be closed by operator
		self.dictRPCsBlockedByGates = {}    # initially empty. (Related to self.dictGatesClosed)

		self.dictRPCsAlwaysBlocked  = {}    # optionally loaded by initialize()

		self.b_connect_allowed	= 1	# Maybe initialize() will turn it to False (0).
		self.b_rpc_allowed	= 1

		self.max_clt_connections = 20
		self.max_clients_sockets = 30

		self.pathOfUnavailabityMessages = "./"	# currrent directory

		self.callParsersPool    = None
		self.regProcExecPool	= None
		self.customProcExecPool	= None
		self.dictDestPools	= None
		self.DestinationMapper	= None
		self.defaultDestName	= None
		self.dictRegProcs	= None
		self.pollProcessStats	= gxqueue.gxStats()

		# these will be created by startUp():
		#
		self.clientsPollMonitor = None
		self.clientsPollThread  = None

		self.queueOfClientsToReturnToPollList = gxqueue.gxqueue("AddPollQ", owner=self, max=0)
		#
		# NOTES: initialize() completes the initialization.
		#	 startUp() starts various threads.
		#
		self.listenPort = None


	def __repr__(self):
		return ("<%s: poll:%s (%d), b_conn=%s, b_rpc=%s, CallParsers:%s DestPools: %s, %s" %
			(self.srvName, 
			 repr(self.clientsPollThread),
			 self.queueOfClientsToReturnToPollList.qsize(),
			 self.b_connect_allowed,
			 self.b_rpc_allowed,
			 self.callParsersPool,
			 self.dictDestPools,
			 self.regProcExecPool) )

	def getShortStatus(self):
		conn_str = "Conn:open, "
		rpc_str  = "RPC:open, "
		intercept_str = "Intercept:enabled, "
		if (not self.b_connect_allowed):
			conn_str = "Conn:CLOSED, "
		if (not self.b_rpc_allowed):
			rpc_str  = "RPC:CLOSED, "
		if (not gxintercept.getEnableInterceptStatus() ):
			intercept_str = "Intercept:DISABLED, "
		return ("%s: %s%s%s %s/%s clients, max %s sockets, Poll:%s, poll lag: %5.1f ms, PollList upd Q: %d to return." %
			(self.srvName, 
			 conn_str,
			 rpc_str,
			 intercept_str,
			 gxclientconn.getNumberOfClients(),
	 		 self.max_clt_connections,
			 self.max_clients_sockets,
			 repr(self.clientsPollThread),
			 self.pollProcessStats.getAverage() * 1000,
			 self.queueOfClientsToReturnToPollList.qsize(),
			) )

	def getServerName(self):
		return self.srvName

	def getStatusAsRow(self):
		row = [ self.srvName, 				# string
			self.b_rpc_allowed,			# boolean (integer)
			self.b_connect_allowed, 		# boolean (integer)
 			gxintercept.getEnableInterceptStatus(),	# boolean (integer)
			gxclientconn.getNumberOfClients(), 	# integer
			self.max_clt_connections, 		# integer
			self.max_clients_sockets,		# integer
			gxcallparser.getCtrTotalRPCs(),		# integer
		      ]

		return row

	def getStatusAsRowHeaders(self):
		h = [ "server_name", "RPC_gate", "Conn_gate", "Intercept", "ctr_conn", "max_conn", "max_sock", "Total_RPCs", ]
		return h

	def getGatesStatusAsRow(self, b_oneRowManyCols=0):
		#
		# Return:
		#	if b_oneRowManyCols: a list of booleans: 1 means gate open, 0 means gate is closed.
		#
		# 	if not b_oneRowManyCols: a list of rows. Each row is: [ gateName, boolean]
		#	boolean: 1 means gate open, 0 means gate is closed.
		#
		if b_oneRowManyCols:
			ls = [ self.b_rpc_allowed, self.b_connect_allowed, ]
		else:
			ls = [ [ "RPC", self.b_rpc_allowed ], [ "Conn", self.b_connect_allowed], ]
		if not self.dictOfRPCgroups:
			return ls

		lsNames = self.dictOfRPCgroups.keys()
		lsNames.sort()
		for name in lsNames:
			x = self.dictGatesClosed.get(name, None)
			if x:
				b_open = 0
			else:
				b_open = 1
			if b_oneRowManyCols:
				ls.append(b_open)
			else:
				ls.append( [name, b_open] )
		return ls

	def getGatesStatusAsRowHeaders(self, b_oneRowManyCols=0):
		if not b_oneRowManyCols:
			return [ "gateName", "status", ]

		#
		# Return:	a list of column names, one per "gate" 
		#
		# NOTE: Each column name is 6 characters long, padded with spaces, or truncated.
		#	Why 6 characters?
		#	1. The column name width is a hint about how wide the column value should be displayed.
		#	2. The client is expected to expand each of the booleans returned by getGatesStatusAsRow() 
		#	   to "Open" or "CLOSED" ("closed" is 6 characters long)
		#	3. No "gate" name should be larger than 6 characters. Otherwise it will appear truncated 
		#	   in the list returned here.
		#
		ls = [ "RPC   ", "Conn  ", ]
		if not self.dictOfRPCgroups:
			return ls

		lsNames = self.dictOfRPCgroups.keys()
		lsNames.sort()
		for name in lsNames:
			x = "%-6.6s" % (name, )		# pad or truncate name to 6 characters, left justified
			ls.append(x)

		return ls


	def getAllDestConnPools(self):
		if not self.dictDestPools:
			return []
		return self.dictDestPools.keys()

	def getPollProcessStats(self):
		return (self.pollProcessStats)

	def assignDebugLevel(self, val):
		self.debugLevel = val

	def getDebugLevel(self):
		return (self.debugLevel)

	def getRcsVersionId(self):
		return (RCS_VERSION_ID)

	def getServerMode(self):
		return (self.serverMode)


	def assignOrdinaryLoginAllowed(self, val):
		self.b_connect_allowed = val

	def isOrdinaryLoginAllowed(self):
		return (self.b_connect_allowed)

	def assignRPCsAllowed(self, val):
		self.b_rpc_allowed = val

	def isRPCgateOpen(self):
		return (self.b_rpc_allowed)

	def getListOfClosedGates(self):
		ls = self.dictGatesClosed.keys()
		if (len(ls) <= 1):
			return (ls)

		ls.sort()
		return (ls)

	def checkIfValidRPCgroupGateName(self, groupName):
		return (self.dictOfRPCgroups.has_key(groupName) )


	def isThisRPCallowed(self, rpcName):
		#
		# Called by:	gxcallparser.py (??)
		#
		# Return:
		#	1		RPC is allowed
		#	(ret, msg)	RPC is not allowed
		#
		if (not self.b_rpc_allowed  and  not gxregprocs.isRegProcName(rpcName) ):
			#
			# some regprocs are allowed when the RPC gate is closed,
			# but, all forwarded RPC calls are denied.
			#
			ret = gxreturnstatus.OS_RPC_GATE_CLOSED
			msg = self.getUnavailabityMessage(0, "rpc")
			return (ret, msg)

		rpcName = rpcName.lower()	# all dictionaries were loaded with lowercase keys

		if (self.dictRPCsAlwaysBlocked.has_key(rpcName) ):
			ret = gxreturnstatus.OS_RPC_NAME_BLOCKED
			msg = self.getUnavailabityMessage(0, "blockedRPC")
			return (ret, msg)

		if (not self.dictRPCsBlockedByGates.has_key(rpcName) ):
			if (len(rpcName) <= 5):	# too short to have prefix?
				return (1)	# OK, allowed
			prefix = rpcName[:5]
			prefix = prefix + "*"
			if (not self.dictRPCsBlockedByGates.has_key(prefix) ):
				return (1)	# OK, this RPC is allowed!
			else:
				rpcName = prefix	# simplifies finding gateName just here below...


		# At this point, we know this rpcName is blocked by a group gate...
		#
		try:
			gateName = self.dictRPCsBlockedByGates[rpcName]
		except KeyError:
			return (1)	# strange... but this mean this RPC is allowed

		ret = gxreturnstatus.OS_RPC_NAME_BLOCKED
		msg = self.getUnavailabityMessage(1, gateName)
		return (ret, msg)


	def assignRPCsGroupGateState(self, groupName, b_open):
		#
		# Called by:	gxregprocs.gateProc()
		#
		# Return:
		#	0	OK, done
		#	-1	no such groupName
		#	-2	groupName already open, or, already closed
		#	string	error msg about incoherent data structures
		#
		groupName = groupName.lower()

		if (not self.dictOfRPCgroups.has_key(groupName) ):
			return -1

		is_closed = self.dictGatesClosed.has_key(groupName)

		if (b_open  and  not is_closed):
			return -2

		if (not b_open  and  is_closed):
			return -2

		#
		# For blocking or un-blocking, we need to have the list of RPC name:
		#
		try:
			rpcNamesList = self.dictOfRPCgroups[groupName]
		except KeyError:
			msg = "Strange, cannot get dictOfRPCgroups['%s']" % (groupName, )
			return (msg)

		if (b_open):
			#
			# Let's un-block this RPC group:
			#
			msg = None
			rpcNamesDoneDict = {}
			for rpcName in rpcNamesList:
				try:
					del self.dictRPCsBlockedByGates[rpcName.lower()]
					rpcNamesDoneDict[rpcName.lower()] = rpcName
				except KeyError:
					if not rpcNamesDoneDict.has_key(rpcName.lower()):
						msg = "Strange, '%s' is closed, but, cannot del dictRPCsBlockedByGates['%s']" % (groupName, rpcName)

			msg2 = None
			try:
				del self.dictGatesClosed[groupName]
			except KeyError:
				msg2 = "Strange, '%s' is closed, but, cannot del dictGatesClosed['%s']" % (groupName, groupName)

			if (msg2):
				return (msg2)
			if (msg):
				return (msg)

			return (0)	# OK, done.

		# blocking a RPC group is simpler:
		#
		for rpcName in rpcNamesList:
			self.dictRPCsBlockedByGates[rpcName.lower()] = groupName

		self.dictGatesClosed[groupName] = 1

		return (0)	# OK, done.


	def getUnavailabityMessage(self, typeOfGate, gateOrDestName):
		#
		# Called by:	self.isThisRPCallowed()
		#		gxcallparser.py	--- before queueing the RPC to a DestConnPool
		#
		#
		# typeOfGate:	0: server gate ("conn" or "rpc")
		#		1: RPC group
		#		2: destConnPool
		#
		if (typeOfGate == 0):
			msgFile = "%s_closed" % (gateOrDestName, )
		elif (typeOfGate == 1):
			msgFile = "closed_%s" % (gateOrDestName, )
		else:
			msgFile = "down_%s" % (gateOrDestName, )

		fullPath = self.pathOfUnavailabityMessages + msgFile

		try:
			f = open(fullPath)
			s = f.readline()
			f.close()

			return s.strip()
		except:
			pass

		altMsg = "'%s' is closed temporarily" % (gateOrDestName, )

		return altMsg


	def loadConfig(self):
		#
		# Called by:	.initialize()
		#
		# Load the configuration from the config file(s) in three (3) steps.
		# See (1), (2), (3) in comments below.
		#
		# 
		# NOTE: Cfg.loadConfig() can generate various exceptions if some errors
		# 	occur while reading the config file.
		#
		#	The except: clause below is very verbose to help to debug
		#	the config file.
		# 
		try:
			lastCfgFilename = self.cfgFile

			self.cfg = Cfg.Cfg("gxConfig", self.cfgFile, optPrefix=self.srvName)

			# (1) load BASE section config parameters:
			#
			sectionName = "BASE"
			self.cfg.loadConfig(section=sectionName)

			m1 = "Loaded config section '%s' from file ' %s'." % (sectionName, lastCfgFilename)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			# (2) load the <srvType> section (CIG or TRG) config parameters:
			#
			self.cfg.loadConfig(section=self.srvType, b_allowRedefine=1)

			m1 = "Loaded config section '%s' from file ' %s'." % (self.srvType, lastCfgFilename)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			# (3) if that file exists, load the <srvName> section from the
			#     file named <cfgFile>.<hostName> :
			#
			try:
				ls = os.uname()		# only good on Unix or Posix systems
				self.hostname = ls[1]	# hostname is the second entry.
			except:
				self.hostname = "[unknownHostname]"

			# Try to load local config from (self.cfgFile + "." + self.hostname)
			# or from (self.cfgFile + ".localhost") :
			#
			localCfgFile = self.cfgFile + "." + self.hostname
			b_localCfgFound = 0
			if (os.path.exists(localCfgFile) ):
				lastCfgFilename = localCfgFile
				b_localCfgFound = 1
			else:
				localCfgFile = self.cfgFile + ".localhost"
				if (os.path.exists(localCfgFile) ):
					lastCfgFilename = localCfgFile
					b_localCfgFound = 1

			if (b_localCfgFound):
				self.cfg.loadConfig(section=self.srvName, filename=localCfgFile, b_allowRedefine=1)

				m1 = "Loaded config section '%s' from file ' %s'." % (self.srvName, lastCfgFilename)
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			return (0)
		except:
			excStr = gxutils.formatExceptionInfoAsString()

			m1 = "... loadConfig() failed."
			try:
				m1 = "Error loading config: %s, lineNo: %s, lastError: %s, lastProgressMsg: %s." % \
					(lastCfgFilename, self.cfg.lineNo, self.cfg.lastError, self.cfg.lastProgressMsg)
			except: pass

			srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)

			m1 = "Exception while loading config: %s" % (excStr, )

			srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)

			print "Error loading config file:"
			try:
				print "lineNo:", self.cfg.lineNo, " lastError:", self.cfg.lastError
				print "lastProgressMsg: ", self.cfg.lastProgressMsg
			except: pass


	def getParamValuesList(self, paramName):
		#
		# NOTE: this function return a list of values (version of 2002mar07)
		#
		# paramVal is a Cfg.ParamVal object.
		# paramVal.getList() return a List of values.
		#
		paramVal = self.cfg.getParamVal(paramName) 

		if (paramVal != None):
			return (paramVal.getList() )

		raise RuntimeError, ("parameter %s not found" % (paramName, ) )

	def getMandatoryParam(self, paramName, b_addErrMsg=0):
		val = self.cfg.getParamFirstValue(paramName)
		if (val != None):
			return (val)

		msg = ("parameter %s not found" % (paramName, ) )
		if b_addErrMsg:
			self.addErrMsg(msg)

		raise RuntimeError, msg

	def getParamWarnIfDefaultValue(self, paramName, defaultValue):
		val = self.cfg.getParamFirstValue(paramName)
		if (val != None):
			return (val)

		m1 = "%s parameter not found. Using default value: %s" % (paramName, defaultValue)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		return (defaultValue)

	def getOptionalParam(self, paramName, defaultValue=None):
		val = self.cfg.getParamFirstValue(paramName, defaultValue)
		return (val)


	def getParamNextValue(self, paramName):
		return (self.cfg.getParamNextValue(paramName) )


	def isValInParamValuesList(self, str, paramName):
		return (self.cfg.isValInParamValuesList(str, paramName) )

	def initialize(self):
		#
		# Called by:	main
		#
		# initialize() create various internal data structures,
		# resources, pools, etc.
		# Also, it establish mutual references in other modules.
		#

		# Other modules need a reference to this gxserver (self) to
		# be able to call the following methods:
		#	.writelog()
		#
		gxqueue.gxServer       = self
		gxrpcmonitors.gxServer = self
		gxregprocs.gxServer    = self
		gxclientconn.gxServer  = self
		gxrespool.gxServer     = self
		gxintercept.gxServer   = self
		gxcallparser.gxServer  = self
		gxbdbqueue.gxServer    = self

		#
		# Other modules need to write to the srvlog:
		#
		gxcallparser.srvlog    = srvlog
		gxdestconn.srvlog      = srvlog
		gxxmltpwriter.srvlog   = srvlog
		xmltp_hosts.srvlog     = srvlog

		# Some modules need to know about gxrpcframe asTuple() colNames:
		#
		gxclientconn.frameTupleColNames = gxrpcframe.asTupleColNames()
		gxthread.frameTupleColNames	= gxrpcframe.asTupleColNames()

		#
		# load the config parameters from the CONFIG FILE:
		#
		if (self.loadConfig() != 0):
			m1 = "Aborting gxserver.initialize() because loadConfig() failed."
			print m1, "See log for details."
			self.addErrMsg(m1)
			return (-1)

		if (self.debugLevel >= 20):
			self.dumpAllParamsValues()


		#
		# sub-classes can have a self.preInitializeCustom() method :
		#
		try:
			rc = self.preInitializeCustom()
			if (rc <= -1):
				self.addErrMsg("self.preInitializeCustom() <= -1")
				return (rc)
		except AttributeError:
			pass


		# LISTEN PORT
		# -----------
		#
		x = xmltp_hosts.getServerHostAndPort(self.srvName)
		if (not x or type(x) == type("s") ):
			m1 = "Aborting. Cannot find port for '%s'. Errmsg: %s." % (self.srvName, x)
			print m1, "See log for details."
			self.addErrMsg(m1)
			return (-1)

		hostName, self.listenPort = x

		if (hostName != self.hostname and hostName != "localhost"):
			m1 = "Aborting. Hostname mismatch: '%s', %s." % (self.hostname, hostName)
			print m1, "See log for details."
			self.addErrMsg(m1)
			return (-1)

		m1 = "Server name: %s, listen port: %s. Hostname(s): %s, %s" % (self.srvName, self.listenPort, self.hostname, hostName)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


		# Maximum of clients connections and sockets
		# ------------------------------------------
		#
		# NOTE: self.max_clients_sockets is NOT the same as 
		#	self.max_clt_connections
		#
		self.max_clt_connections = self.getMandatoryParam(gxparam.GXPARAM_MAX_CLT_CONNECTIONS, 1)
		self.max_clients_sockets = self.getMandatoryParam(gxparam.GXPARAM_MAX_CLIENTS_SOCKETS, 1)

		self.client_recv_timeout = self.getMandatoryParam(gxparam.GXPARAM_CLIENT_RECV_TIMEOUT, 1)
		self.client_send_timeout = self.getMandatoryParam(gxparam.GXPARAM_CLIENT_SEND_TIMEOUT, 1)


		# GROUPS_OF_RPC	(mandatory)
		# -------------
		#
		self.dictOfRPCgroups = {}

		try:
			listOfRPCgroups = self.getParamValuesList(gxparam.GXPARAM_GROUPS_OF_RPC)
		except:
			self.addErrMsg("Missing param: " + gxparam.GXPARAM_GROUPS_OF_RPC)
			return -1

		ctrErrors = 0
		for grp in listOfRPCgroups:
			paramName = gxparam.GXPARAM_RPCS_OF_GROUP_ + grp

			if (self.getOptionalParam(paramName) == None):
				m1 = "parameter missing: %s" % (paramName, )
				self.addErrMsg(m1)
				ctrErrors = ctrErrors + 1
			else:
				grp = grp.lower()	# all group names keys are lowercase
				self.dictOfRPCgroups[grp] = self.getParamValuesList(paramName)

		if (ctrErrors > 0):
			m1 = "%s groups in %s are NOT defined. See messages above." % (ctrErrors, gxparam.GXPARAM_GROUPS_OF_RPC)
			self.addErrMsg(m1)
			return (-1)

		m1 = "%s definitions loaded from %s." % (len(self.dictOfRPCgroups), gxparam.GXPARAM_GROUPS_OF_RPC)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


		# PATH_OF_UNAVAILABITY_MESSAGES
		# -----------------------------
		#
		self.pathOfUnavailabityMessages = self.getMandatoryParam(gxparam.GXPARAM_PATH_OF_UNAVAILABITY_MESSAGES, 1)

		if (not os.path.exists(self.pathOfUnavailabityMessages) ):
			m1 = "'%s' is not a valid path/directory. (%s)." % (self.pathOfUnavailabityMessages, gxparam.GXPARAM_PATH_OF_UNAVAILABITY_MESSAGES)
			self.addErrMsg(m1)
			return (-1)


		# RPCS_ALWAYS_BLOCKED (optional)
		# -------------------
		#
		if (self.getOptionalParam(gxparam.GXPARAM_RPCS_ALWAYS_BLOCKED) != None):
			self.dictRPCsAlwaysBlocked  = {}

			listAlwaysBlocked = self.getParamValuesList(gxparam.GXPARAM_RPCS_ALWAYS_BLOCKED)

			for rpcName in listAlwaysBlocked:
				self.dictRPCsAlwaysBlocked[rpcName.lower()] = 0

			ls = self.dictRPCsAlwaysBlocked.keys()
			ls.sort()

			m1 = "%s RPC(s) will ALWAYS be blocked: %s." % (len(ls), ls)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


		# INITIAL_GATE_STATUS (optional)
		# -------------------
		#
		val = self.getOptionalParam(gxparam.GXPARAM_INITIAL_GATE_STATUS)
		if (val != None):
			if (val.lower() == gxparam.GXPARAM_VAL_GATE_CLOSED):
				self.b_connect_allowed = 0	# only admin users will be allowed to connect
				level = TxLog.L_WARNING
				msg   = "REJECTED"
			else:
				self.b_connect_allowed = 1
				level = TxLog.L_MSG
				msg   = "allowed"

			m1 = "%s: ordinary logins will be '%s' after startup. (%s : '%s')" % (self.srvName, msg, gxparam.GXPARAM_INITIAL_GATE_STATUS, val.lower() )
			srvlog.writeLog(MODULE_NAME, level, 0, m1)


		# RPC interception definitions
		# ----------------------------
		#
		# Let's make sure we can debug the config of the RPC interception:
		#
		if (self.debugLevel >= 10):
			gxintercept.assignDebugLevel(5)
		else:
			gxintercept.assignDebugLevel(self.debugLevel)


		# Initialize the RPC interception filters and queues:
		#
		if (gxintercept.initInterceptDefinitions() <= -1):
			self.addErrMsg("gxintercept.initInterceptDefinitions() <= -1")
			return (-1)

		#
		# create the "CallParsersPool"
		#            -----------------
		#
		# NOTE: maxRes should come from the config file.
		#
		parserMon = gxcallparser.CallParserMonitor("CallParserMon")

		maxRes = self.getParamWarnIfDefaultValue(gxparam.GXPARAM_CALLPARSERSPOOL_MAXRES, 5)

		self.callParsersPool = gxcallparser.CallParserResPool(name="CallParsersPool", 
						maxRes = maxRes,
						resCreationArgsList=[parserMon, ],
						maxRequests = 0,
						master = self)

		self.callParsersPool.assignDebugLevel(self.debugLevel)
		#
		# The previous assignDebugLevel() has little or no effect,
		# but, the following one has real effect:
		#
		gxcallparser.assignDebugLevel(self.debugLevel)

		m1 = "callParsersPool started: %s." % (repr(self.callParsersPool), )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


		#
		# create the "RegProcExecPool"
		#            -----------------
		#
		maxRes = self.getParamWarnIfDefaultValue(gxparam.GXPARAM_REGPROCEXECPOOL_MAXRES, 5)

		self.regProcExecPool = RegProcExecThreadsPool(name="RegProcPool", 
						maxRes = maxRes,
						resCreationArgsList = [],
						maxRequests = 0,
						master = self)

		self.regProcExecPool.assignDebugLevel(self.debugLevel)

		m1 = "RegProc Exec Pool created: %s." % (repr(self.regProcExecPool), )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		#
		# create the "Custom RegProcs Exec Pool"
		#             -------------------------
		#
		maxRes = self.getParamWarnIfDefaultValue(gxparam.GXPARAM_CUSTOM_REGPROCEXECPOOL_MAXRES, 5)

		self.customProcExecPool = RegProcExecThreadsPool(name="CustomProcPool", 
						maxRes = maxRes,
						resCreationArgsList = [],
						maxRequests = 0,
						master = self)

		self.customProcExecPool.assignDebugLevel(self.debugLevel)

		m1 = "Custom RegProc Exec Pool created: %s." % (repr(self.customProcExecPool), )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


		#
		# traceMan.register() allows to modify debug trace levels dynamically
		# -------------------
		#

		# First, objects:
		#
		traceMan.register("server", self)
		traceMan.register("RegProcExecPool", self.regProcExecPool, "regproc")
		traceMan.register("CustomProcExecPool", self.customProcExecPool, "regproc")


		# Then, modules:
		#
		traceMan.register("gxintercept",   gxintercept,	  "intercept")
		traceMan.register("gxbdbqueue",    gxbdbqueue,	  "intercept")
		traceMan.register("bdbvarqueue",   bdbVarQueue,	  "intercept")

		traceMan.register("gxcallparser",  gxcallparser,  "callparser")
		traceMan.register("gxrpcmonitors", gxrpcmonitors, "rpc")
		traceMan.register("gxregprocs",    gxregprocs,	  "regproc")
		traceMan.register("gxxmltpwriter", gxxmltpwriter, "xml")
		traceMan.register("gxqueue", 	   gxqueue)
		traceMan.register("cfg",	   Cfg)

		#
		# At this time, 29jan2002, the following modules only really usefully
		# implement getVersionId():
		#
		traceMan.register("gxmonitor",	  gxmonitor,	"monitor")
		traceMan.register("gxclientconn", gxclientconn,	"client")
		traceMan.register("gxrpcframe",   gxrpcframe, 	"frame" )
		traceMan.register("gxthread",	  gxthread,	"thread")
		traceMan.register("gxutils", 	  gxutils)
		traceMan.register("xmltp_hosts",  xmltp_hosts)

		#
		# for the DestConnPool's, each one must be processed with traceMan.register().
		# See in the loop below...
		#

		# initialize DestinationMapper:
		#            -------------------
		#
		# NOTE: more work is done when the Destination pool(s) are created.
		#	See call to self.updateDestinationMapper(destName) below.
		#
		#
		self.defaultDestName = self.getMandatoryParam(gxparam.GXPARAM_DEFAULT_DESTINATION, 1)
		if (self.defaultDestName.lower() == "nil"):
			m1 = "NOTE: config param %s is 'NIL'." % (gxparam.GXPARAM_DEFAULT_DESTINATION, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			self.defaultDestName = None

		self.DestinationMapper = {}	# empty at first, elements are added when each destination is created

		#
		# create the Destination pool(s):
		#            -------------------
		#
		self.dictDestPools = {}

		destName = self.getMandatoryParam(gxparam.GXPARAM_DESTINATIONS, 1)
		if (destName.lower() == "nil"):
			m1 = "NOTE: config param %s is 'NIL'. RPC forwarding is turned OFF." % (gxparam.GXPARAM_DESTINATIONS, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			destName = None		## this will terminate the following loop immediately.

		ctrErrors = 0
		while (destName != None):
			try:
				# Do self.updateDestinationMapper(destName) because it will abort with
				# an exception if something is wrong.
				# And it is cleaner to abort before creating a DestPool.
				#
				self.updateDestinationMapper(destName)	# prepare RPC routing dictionnary

				destPool = self.createDestPool(destName)

				self.dictDestPools[destName] = destPool

				destPool.assignDebugLevel(self.getDebugLevel() )

				traceMan.register(destName, destPool, "connpool")

				m1 = "DestPool created: %s." % (repr(destPool), )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			except:
				ctrErrors = ctrErrors + 1
				ex = gxutils.formatExceptionInfoAsString(3)
				m1 = "creating destPool '%s' failed: %s" % (destName, ex)
				self.addErrMsg(m1)

			destName = self.getParamNextValue(gxparam.GXPARAM_DESTINATIONS)


		m1 = "%d DestPool's: %s." % (len(self.dictDestPools), self.dictDestPools.keys() )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		if (ctrErrors > 0):
			m1 = "%s errors found. See messages above." % (ctrErrors, )
			self.addErrMsg(m1)
			return (-2)


		# 
		# list all RegProcs names in the log:
		#
		lsRegProcs = gxregprocs.getListRegProcNames()

		lsRegProcs.sort()

		m1 = "%d regprocs: %s." % (len(lsRegProcs), lsRegProcs )

		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		#
		# initialize the pre-exec log hook, if enabled:
		#
		preExecLogPath = self.getOptionalParam(PARAM_PRE_EXEC_LOG, None)
		if preExecLogPath != None:
			preexeclog.initLogFilePath(self.srvName, preExecLogPath)
			self.preExecLogFunc = preexeclog.writePreExecLog

		#
		# sub-classes can have a self.initializeCustom() method :
		#
		try:
			rc = self.initializeCustom()
			if (rc <= -1):
				self.addErrMsg("self.initializeCustom() <= -1")
				return (rc)
		except AttributeError:
			pass

		#
		# .startUp() will be allowed to proceed :
		#
		self.initDoneOK  = 1


		return (0)


	def createDestPool(self, destName):
		#
		# Called by:	.initialize()
		#
		# First, get all required parameters.
		# Then, create the gxdestconn.DestConnPool.
		#
		maxRes      = self.getParamWithPrefixIfPossible(destName, gxparam.GXPARAM_MAX_CONNECTIONS, b_addErrMsg=1)

		sendTimeout = self.getParamWithPrefixIfPossible(destName, gxparam.GXPARAM_DEST_SEND_TIMEOUT, b_addErrMsg=1)

		recvTimeout = self.getParamWithPrefixIfPossible(destName, gxparam.GXPARAM_RPC_TIMEOUT, b_addErrMsg=1)

		maxWaitInQueue = self.getParamWithPrefixIfPossible(destName, gxparam.GXPARAM_MAX_WAIT_IN_DEST_QUEUE, b_addErrMsg=1)

		loginProc = self.getParamWithPrefixIfPossible(destName, gxparam.GXPARAM_DEST_LOGIN_PROC, b_addErrMsg=1)

		userid    = self.getParamWithPrefixIfPossible(destName, gxparam.GXPARAM_USR_ID, b_addErrMsg=1)

		passwordFile = self.getParamWithPrefixIfPossible(destName, gxparam.GXPARAM_USR_PWD_FILE, b_addErrMsg=1)
		if (passwordFile == None):
			m1 = "BAD destination config for '%s': passwordFile is None." % (destName, )
			raise RuntimeError, m1

		password = self.readFirstLineOfFile(passwordFile)

		if (password == None):
			m1 = "BAD destination config for '%s': password is None." % (destName, )
			raise RuntimeError, m1

		destPool = gxdestconn.DestConnPool(name = destName, 
					maxRes = maxRes,
					resCreationArgsList = [],
					maxRequests = 0,
					master = self,
					b_postponeBuildRes = 1)
		if (destPool == None):
			return (destPool)

		# Attributes specific to DestConnPool (not in gxrespool):
		#

		# Attributes that will be used to establish a socket connection:
		#
		destPool.routesDict	= self.buildRoutesDict(destName)
		destPool.currentRoute	= self.getParamWithPrefixIfPossible(destName, gxparam.GXPARAM_INITIAL_ROUTE, b_addErrMsg=1)
		destPool.destSrvName	= "-"
		destPool.destHostnamePort = "-"


		# Attributes that will be used to do a login to the destination (server):
		#
		destPool.loginProc	= loginProc
		destPool.userid		= userid
		destPool.passwordFile	= passwordFile
		destPool.origSrvName	= self.srvName	# the origin-Server-Name is the name of this server

		# Attributes related to wait time for obtaining a DestConn and RPC execution timeout:
		#
		destPool.sendTimeout	= sendTimeout
		destPool.recvTimeout	= recvTimeout
		destPool.maxWaitInQueue = maxWaitInQueue

		m1 = "%s - Attributes: %s, currentRoute=%s, SNDTIMEO=%s RCVTIMEO=%s maxWaitInQueue=%s, %s, %s, %s (%s), %s" % (destName, destPool.routesDict, destPool.currentRoute, sendTimeout, recvTimeout, maxWaitInQueue, loginProc, userid, password, passwordFile, self.srvName)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		# Try to get the srvName of the currentRoute (INITIAL_ROUTE).
		# This might fail and then the startup will abort...
		#		
		x = destPool.routesDict[destPool.currentRoute]

		# Now, create the (maxRes) DestConn...
		#
		destPool.buildAllResources()

		return (destPool)

	def buildRoutesDict(self, destName):
		possibleRoutes = [ "A", "B", "C", "D", "E" ]	# more than enough for now
		dict = {}
		for route in possibleRoutes:
			paramName = "%s%s" % (gxparam.GXPARAM_DEST_ROUTE_, route)
			srvName = self.getParamWithPrefixIfPossible(destName, paramName, b_optional=1)
			if srvName:
				dict[route] = srvName
		return dict


	def getParamWithPrefixIfPossible(self, prefix, paramName, concatStr="_", altParamName=None, b_optional=0, b_addErrMsg=0):
		#
		# Called by:	createDestPool()
		#
		if (concatStr):
			pName = prefix + concatStr + paramName
		else:
			pName = prefix + paramName
		val= self.getOptionalParam(pName)
		if (val != None):
			return (val)

		if (altParamName):
			paramName = altParamName

		if (b_optional):
			return (self.getOptionalParam(paramName) )

		return (self.getMandatoryParam(paramName, b_addErrMsg) )


	def readFirstLineOfFile(self, fileName):
		#
		# Called by:	createDestPool()
		#
		f = None
		try:
			f = open(fileName)
			aLine = f.readline(200)
			if (aLine):
				aLine = aLine.rstrip()	# remove trailing space(s) or '\n'
			f.close()
			return (aLine)
		except:
			ex = gxutils.formatExceptionInfoAsString(3)
			m1 = "Error while reading file '%s': %s" % (fileName, ex)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

		if (f != None):
			f.close()
		return (None)


	def updateDestinationMapper(self, destName):
		#
		# Called by:	.initialize()
		#
		# First, find the param which contains the list of RPC names and/or prefixes
		#
		paramName = gxparam.GXPARAM_RPC_FOR_DEST_ + destName

		if (self.getOptionalParam(paramName) == None):
			oldParam = paramName
			paramName = gxparam.GXPARAM_RPCS_OF_GROUP_ + destName

			m1 = "%s not found, will use values from %s instead." % (oldParam, paramName)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

		# add all RPC names or prefixes to the .DestinationMapper dictionary
		#
		rpcName = self.getOptionalParam(paramName)
		if (rpcName == None):
			m1 = "Destination %s will not directly route RPC because param '%s' was not found." % (destName, paramName)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
			return (1)

		while (rpcName != None):
			self.addRpcNameToDestinationMapper(destName, rpcName)

			rpcName = self.getParamNextValue(paramName)

	def addRpcNameToDestinationMapper(self, destName, rpcName):
		#
		# Called by:	.updateDestinationMapper()
		#
		iStar = rpcName.find("*")
		if (iStar >= 1):
			i = rpcName.find("_")
			if (i < iStar):
				rpcName = rpcName[:iStar]
			#
			# if not, keep rpcName as is. It is not a valid prefix.
			#
		try:
			prevDest = self.DestinationMapper[rpcName]
			m1 = "Replacing previous destination mapping: '%s' --> %s by --> %s." % (rpcName, prevDest, destName)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
		except:
			pass

		self.DestinationMapper[rpcName] = destName
		return (0)


	def startUp(self):
		#
		# Called by:	main
		#
		#
		# PURPOSE: start listener and other threads.
		#
		#
		# __main__() should call startUp() after initialize().
		#
		# If these methods exist, .startup() will call them:
		#	self.preStartUp()
		#	self.postStartUp()
		#
		rc = 0
		if (self.ctrErrors >= 1 or (not self.initDoneOK) ):
			m1 = "startUp() rejected because initialize() did not complete successfully."
			self.addErrMsg(m1)
			srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, self.ctrErrors, m1)
			return (-3)

		#
		# sub-classes can have a self.preStartUp() method
		#
		try:
			rc = self.preStartUp()
			if (rc <= -1):
				self.addErrMsg("self.preStartUp() <= -1")
				return (rc)
		except AttributeError:
			pass

		#
		# Initialize the gx_poll module:
		#
		xx = gx_poll.initListen(self.listenPort)
		if (type(xx) == type("str") ):
			self.addErrMsg(xx)
			return (-1)

		m1 = "gx_poll module init OK. max %s connections." % (xx, )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		# log the version of gx_poll that we are using:
		#
		compileTime = gx_poll.getVersionId(0)
		rcsVersion  = gx_poll.getVersionId(1)
		m1 = "gx_poll module version: %s, %s" % (compileTime, rcsVersion)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		#
		# Start the Queue threads in the various gxrespool(s):
		#
		# gxrespool.startQueueThreadInAllResPools() will raise an exception if 
		# something goes wrong.
		#
		gxrespool.startQueueThreadInAllResPools()


		#
		# Start the Clients poll() thread:
		#
		self.clientsPollMonitor = ClientsPollMonitor("ClientsPollMon", self)
		self.clientsPollThread  = gxthread.gxthread(  "ClientsPollThread", 
						monitor = self.clientsPollMonitor,
						threadType=gxthread.THREADTYPE_POLL)
		self.clientsPollThread.start()

		m1 = "clientsPollThread started: %s." % (repr(self.clientsPollThread), )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		#
		# Queue a test RPC on each DestConn:
		#
		for dest in self.dictDestPools.values():
			dest.queueTestRPCtoConnect()


		#
		# start the "kick gateway" daemon thread in gxintercept.py :
		#
		gxintercept.startKickGatewayDaemonThread()

		m1 = "gxintercept.startKickGatewayDaemonThread() started"
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		#
		# sub-classes can have a self.postStartUp() method
		#
		try:
			rc = self.postStartUp()
		except AttributeError:		# this method may exist or not
			pass

		# Adjust the trace levels of various modules and groups according 
		# to the config parameters (see param TRACE_INIT, TRACE_rpc, etc.) :
		#
		self.adjustInitialTraceLevels()

		return rc


	def adjustInitialTraceLevels(self):
		#
		# Called by:	 startUp()
		#
		# Adjust the trace levels of various modules and groups according 
		# to the config parameters (see param TRACE_INIT, TRACE_rpc, etc.)
		#

		x = self.getOptionalParam(gxparam.GXPARAM_TRACE_INIT)

		if (x == None):
			m1 = "Param %s not found. Trace levels will keep their default values." % (gxparam.GXPARAM_TRACE_INIT, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			return

		while (x != None):
			try:
				paramName = "%s%s" % (gxparam.GXPARAM_TRACE_, x)
				val = self.getOptionalParam(paramName)
				if val:
					m1 = "Settting trace %s, %s" % (x, val)
					srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

					errMsg = traceMan.assignTraceLevel(x, val)
					if errMsg:
						srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, errMsg)
				else:
					m1 = "Param %s not found." % (paramName, ) 
					srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			except:
				ex = gxutils.formatExceptionInfoAsString(3)
				m1 = "setting trace level '%s' failed: %s" % (x, ex)
				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

			x = self.getParamNextValue(gxparam.GXPARAM_TRACE_INIT)



	#
	# ---------------- Methods called when server is running: -----------------------
	#

	def getDestnameForProcName(self, procName):
		# 
		# Called by:	.mapProcnameToPool()
		#
		# First, try with the whole procName. If found, that's it.
		#
		# If not found, isolate the prefix. Like this:
		#	"sp_who"	 --> "sp_"
		#	"spgl_add_entry" --> "spgl_"
		#
		# Then, try to find the prefix.
		#
		# A special case is "sp_ss_*AnyName*", which should be
		# associated to the prefix "sp_ss_", not to "sp_".
		#
		try:
			return (self.DestinationMapper[procName] )
		except KeyError:
			pass

		if (len(procName) <= 5 ):
			return (self.defaultDestName)

		i = procName.find("_")
		if (i <= 0):
			return (self.defaultDestName)

		if (i == 2 and procName[5] == "_"):	# special case of "sp_ss_XXX"
			i = 5

		prefix = procName[:i+1]

		try:
			return (self.DestinationMapper[prefix] )
		except KeyError:
			return (self.defaultDestName)
	
	def getNameOfAuthenticationRPC(self):
		#
		# WARNING: can return None if the server is not configured to accept
		#	   ordinary login.
		#
		return (self.getOptionalParam(gxparam.GXPARAM_AUTHENTICATION_RPC) )

	def getCheckPasswordDestConnPool(self):
		#
		# Called by:	CallParserResPool.continueProcCallProcessing()
		#
		procName = self.getNameOfAuthenticationRPC()

		return (self.mapProcnameToPool(procName) )


	def mapProcnameToPool(self, procName):
		#
		# Called by:	CallParserResPool.continueProcCallProcessing()
		#
		# Return a pool, a DestConn pool or the RegProcExecPool.
		#
		if (procName == None):
			return None

		if (gxrpcmonitors.getCustomRegProcMonForProcNameX(procName) ):
			return (self.customProcExecPool)

		if (gxregprocs.isRegProcName(procName) ):
			return (self.regProcExecPool)

		destName = self.getDestnameForProcName(procName)

		return (self.getDestConnPoolByName(destName) )

	def getDestConnPoolByName(self, name):
		try:
			return (self.dictDestPools[name])
		except KeyError:
			return None

	def writeLog(self, moduleName, logLevel, msgNo, msg):
		srvlog.writeLog(moduleName, logLevel, msgNo, msg)

	def writeLogClientDisconnect(self, fd, userId, stateDesc, msg=None):
		#
		# Called by:	gxclientconn.processDisconnect()
		#
		if (fd == None):
			fd = -1

		if (msg == None):
			msg = ""
		else:
			msg = ", %s" % (msg, )
		m1 = "Disconnect: '%s' (%s)%s." % (userId, stateDesc, msg)
		srvlog.writeLog("disconnect", TxLog.L_MSG, fd, m1)


	def getMaxCltConnections(self):
		return self.max_clt_connections

	def addClientRequestToCallParserQueue(self, conn_sd):
		#
		# conn_sd has to be converted to a RPC frame:
		#	find the client connection
		#		if does not exist, create one
		#	create a gxRPCframe
		#	bind the frame to the clientConn
		#	queue the frame in CallParsersQueue...
		#
		conn = gxclientconn.getClientConnFromSd(conn_sd)

		if (None == conn):	# this means, we need to create a new clt_conn...
			#
			# If this server has too many clients sockets open, it will
			# reject the connection immediately.
			#
			# NOTE: self.max_clients_sockets is NOT the same as 
			#	self.max_clt_connections
			#
			if (gxclientconn.getNumberOfClients() >= self.max_clients_sockets):
				os.close(conn_sd)	# disconnect this client

				m1 = "TOO MANY client sockets (max=%s). We disconnect this client %s." % (self.max_clients_sockets, conn_sd, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
				return (None)

			#
			# NOTE: excerpt from socketmodule.c, fromfd() function:
			#
			#	/* Dup the fd so it and the socket can be closed independently */
			#	fd = dup(fd);
			#
			# Therefore, we will need to do:
			#		os.close(conn_sd)
			#
			# after doing:
			#		sock = socket.fromfd(conn_sd, ...)
			#
			stepDesc = None
			try:
				stepDesc = "sock = socket.fromfd(conn_sd, ...)"

				sock = socket.fromfd(conn_sd, socket.AF_INET, socket.SOCK_STREAM)


				stepDesc = "sock.setsockopt(,socket.SO_RCVTIMEO,)"

				timeout_val = struct.pack("ll", self.client_recv_timeout, 0)

				sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout_val)


				stepDesc = "sock.setsockopt(,socket.SO_SNDTIMEO,)"
		
				timeout_val = struct.pack("ll", self.client_send_timeout, 0)

				sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDTIMEO, timeout_val)


				stepDesc = "sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)"

				sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
			except:
				excStr = gxutils.formatExceptionInfoAsString(2)
				try:
					os.close(conn_sd)	# disconnect this client, it is the best action
				except:
					pass

				m1 = "FAILED to create socket from conn_sd. We disconnect this client %s. step:%s, Exc: %s." % (conn_sd, stepDesc, excStr)
				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
				return (None)

			conn = gxclientconn.gxClientConn(sock)
			conn.assignState(gxclientconn.STATE_NO_LOGIN_YET, "newConnection")

			new_sd = conn.getSd()
			old_sd = conn_sd	# we need to copy it now as we will zap it soon.
			if (new_sd != conn_sd):
				try:
					os.close(conn_sd)
					msg = "orig conn_sd closed, OK"
					conn_sd = new_sd	# there is an other use of conn_sd here later.
				except:
					msg = "conn_sd.close() FAILED"
			else:
				msg = "STRANGE: new_sd == conn_sd" 

			m1  = "New conn: orig conn_sd: %s, new_sd = conn.getSd(): %s. %s." % (old_sd, new_sd, msg)
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		elif (not conn.isLoginOK() ):
			conn.processDisconnect("RPC request rejected after failed login.")
			return (None)

		frame = gxrpcframe.gxRPCframe(conn)
		try:
			conn.assignRPCframe(frame)
		except:
			# new RPC request could be denied...
			#
			excStr = gxutils.formatExceptionInfoAsString(2)
			ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY

			m1  = "RPC request rejected on %s. Exception: %s. %s" % (conn, excStr, 
						gxreturnstatus.getDescriptionForValue(ret) )
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, ret, m1)

			msg = "RPC rejected"

			gxxmltpwriter.replyErrorToClient(frame, [ (ret, msg), ], ret)

			conn.rpcEndedReturnConnToPollList(0, msg)
			return (None)

		self.callParsersPool.addRequest(frame)

	def getCallParserPool(self):
		return (self.callParsersPool)


	def getRegProcExecPool(self):
		return (self.regProcExecPool)

	def getCustomProcExecPool(self):
		return (self.customProcExecPool)

	def returnClientConnToPollList(self, sd):
		#
		# Called by:	gxclientconn.rpcEndedReturnConnToPollList()
		#
		if (self.debugLevel >= 20):
			m1 = "returnClientConnToPollList(): sd=%s" % (sd, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		return (self.queueOfClientsToReturnToPollList.put(sd) )

	def getClientToReturnToPollList(self):
		return (self.queueOfClientsToReturnToPollList.get_nowait() )

	def addErrMsg(self, msg):
		srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, msg)
		self.errMsg.append(msg)

	def getErrmsg(self):
		if not self.errMsg:
			return "<no initialization error yet>"
		return (self.errMsg)

	#
	# ---------- Methods used to get status of server: -----------------------
	#
	def gatherClientConnInfo(self, clt_conn):
		sd	= clt_conn.getSd()
		userId	= clt_conn.getUserId()
		state	= clt_conn.describeState()
		cltTime = clt_conn.formatTimestamp()

		frame   = clt_conn.getRPCframe()
		if (frame == None):
			return (sd, userId, state, cltTime, "[No RPCframe]")

		step	  = frame.describeStep()
		procName  = frame.getProcName()
		rpcTime   = frame.formatTimestamp()
		rpcMs	  = frame.getElapsedMs()
		frameMsg  = frame.getLastEventMsg()

		return (sd, userId, state, cltTime, procName, step, rpcTime, rpcMs, frameMsg)


	def getListOfClientsWithDetailsHeaders(self):
		return ("sd", "userId", "state", "cltTime", "procName", "step", "rpcTime", "rpcMs", "frameMsg")

	def getListOfClientsWithDetails(self):
		ls = []
		for clt in gxclientconn.getListOfClients():
			clt_info = self.gatherClientConnInfo(clt)
			ls.append(clt_info)
		return (ls)

	def displayStatusInfos(self):
		print "clients:", self.getListOfClientsWithDetails()
		print "Services:", gxthread.getListOfServiceThreads()
		print "RPCs:", gxthread.getListOfRPCThreads()

	#
	# ------------------------- DEBUG Methods: ---------------------
	#
	def dumpAllParamsValues(self):
		#
		# Display (on stdout) all parameters that were loaded from the config file
		# 
		self.cfg.dumpAllParamsValues()

	#
	# ------------------------- Server shutdown methods : ---------------------
	#
	def shutdown(self, clt_conn=None, b_immediate=1, b_quiet=0):
		#
		# NOTE: !!! this is a first, very simple version !!!
		#
		m1 = "server shutdown(), b_immediate=%s." % (b_immediate, )
	        srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

	        srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, self.getShortStatus() )

		self.shutdownFinal(clt_conn, b_quiet)

		if (self.getDebugLevel() == 129):
			try:
				filename = "list_of_unused_params.txt"
				m1 = "About to write list of unused params to '%s'." % (filename, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

				fd = open(filename, "w")
				self.cfg.writeListOfUnusedParamsToFileObj(fd)
			except:
				ex = gxutils.formatExceptionInfoAsString(2)
				m1 = "FAILED to write list of unused params. Exc: %s." % (ex, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

		time.sleep(1)

	        srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, self.getShortStatus() )


	def shutdownFinal(self, clt_conn=None, b_quiet=0):
		#
		# This is the final method to be called in the shutdown sequence.
		#
		# This method assign the self.b_finalStop flag (to tell the
		# MainThread to stop), and, it stops all service threads
		# (which all are related to gxrespools).
		#
		self.b_finalStop = 1

		gxclientconn.disconnectAllClients(clt_conn, b_quiet)

		gxintercept.shutdownAndDumpAllQueues(b_quiet)

		gxrespool.StopAllResPools(b_quiet)	# also closes all DestConn sockets



	#
	# ------------------------- MAIN Method : ---------------------
	#
	def main(self):
		rc = 0

		# Make sure this server process ignores SIGPIPE and SIGHUP
		# Either signal could happen when a socket connection is reset by 
		# the other end!
		#
		try:
			signal.signal(signal.SIGPIPE, signal.SIG_IGN)
			signal.signal(signal.SIGHUP,  signal.SIG_IGN)
		except AttributeError:
			m1 = "error when calling signal.signal(). Probably not running under a Linux/Unix OS!"
			srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)
		# this is to prevent the error below:
		# ValueError: signal only works in main thread
		except ValueError:
			m1 = "Running inside another thread"
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


		# .initialize() completes the initialization of instance variables
		#
		rc = self.initialize()

		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, self.getShortStatus()  )

		if (rc <= -1):
			m1 = ("self.initialize() failed, rc=%s" % (rc, ) )
			self.addErrMsg(m1)
			raise RuntimeError, m1

		#
		# write the versions of the modules in the log:
		#
		m1 = "versions of modules, as given by traceMan.queryTrace()..."
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		m1 = repr(traceMan.queryTrace() )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		#
		# .startUp() starts various service threads :
		#
		rc = self.startUp()
		if (rc <= -1):
			m1 = ("self.startUp() failed, rc=%s" % (rc, ) )
			self.addErrMsg(m1)
			raise RuntimeError, m1


		time.sleep(1)
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, self.getShortStatus()  )

		ctr = 0
		while(not self.b_finalStop):
			time.sleep(1)
			ctr = ctr + 1
			if (ctr >= 60):
				ctr = 0
				### self.displayStatusInfos()

		if (self.b_finalStop):
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Main thread stopping, as requested.")
		else:
			m1 = "server stopping??? Last errmsg: %s. %s" % (self.getErrmsg(), repr(self))
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1 )



def runServer(srv, b_verbose=1):
    srvName = "[undef srvName]"
    try:
       srvName = srv.srvName
       if (b_verbose):
           print "starting srv.main()... srv:", srv.getShortStatus()

       srv.main()	# init, start and run the server...

    except:
        ex = gxutils.formatExceptionInfoAsString(10)
        errmsg = "[No errmsg]"
        try:
          if (srv != None):
                errmsg = srv.getErrmsg()
        except:
          srv = "[srv not defined]"

        m1 = "Server %s FAILED. Last ERRMSG: %s. Exc:{%s}. srv=%s" % (srvName, errmsg, ex, srv.getShortStatus() )
        srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )

        print "server", srvName, "FAILED", ex
        print "See log for details."

    # Stop the server:   
    # ---------------
    #
    try:
        if (b_verbose):
           print srv
        srv.shutdown(b_quiet=1)		# this is probably the second call to .shutdown(), do this one quietly
        if (b_verbose):
           print srv
    except:
        excStr = gxutils.formatExceptionInfoAsString(10)
        m1 = "srv.shutdown() failed: %s." % (excStr, )
        srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )

    m1 = "Server %s STOPPED. Last ERRMSG: %s." % (srvName, srv.getErrmsg(), )
    srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )
    #
    #


if __name__ == '__main__':
    if (len(sys.argv) < 4):
	print "Usage:   gxserver.py serverName srvType configFile"
	print "example: gxserver.py  XmDEVc01    CIG    gxsrvcfg"
	raise SystemExit

    srvName = sys.argv[1]
    srvlog.setServerName(srvName)

    srvType = sys.argv[2]
    cfgFile = sys.argv[3]


    try:
       srv = gxserver(srvName, srvType=srvType, cfgFile=cfgFile, debugLevel=10)

       runServer(srv)

    except:
        ex = gxutils.formatExceptionInfoAsString(10)

        m1 = "server %s FAILED. Exc: %s. srv=%s" % (srvName, ex, srv )
        srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )

        print "server", srvName, "FAILED to start:", ex, "\nSee log for details."

    #
    #
    sys.exit()
