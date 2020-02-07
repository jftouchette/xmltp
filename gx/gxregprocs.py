#!/usr/local/bin/python
#
# gxregprocs.py :  functions implementing Registered Procedures
# -------------    --------------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxregprocs.py,v $
# $Header: /ext_hd/cvs/gx/gxregprocs.py,v 1.5 2006/02/02 14:53:14 toucheje Exp $
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
# Some regprocs are related to intercept queues:
#	+  rpggq_get_rows  <queue>
#	+  getq <queue>
#	+  ggq   <queue>, <cmd> [, <opt>]
#	+  gwcmd <queue>, <cmd> [, <opt>]
#
# Many regprocs are "operator commands":
#	+  help
#	+  route {A|B|C|D|E} [, dest]  -- A|B is usual.
#	+  shutdown	  -- this is the "now" (fast) version.
#	+  trace <module>, { none | low | medium | full | <level> }
#	  or
#	   trace query [, <module> ]
#	+  who [ <sort_order> ]
#	+  ps  [ rpc | service ]
#	+  login <userid>, <password>
#
#!!	+  tquery { server | trace | gate | <dest> | <queue> | Q | ALLQUEUES | QUEUES | D | ALLDEST }
#	+  query { server | trace | gate | <dest> | <queue> }
#	+  stats { <dest> | <queue> | poll | parser | regprocpool | customprocpool | ALLPOOLS } [, reset|FULL|SHORT ]
#
#	+  gate { open|close|query}, { rpc | conn | <gateName> | <queue> | <dest>  }
#	+  clean { <dest> | all } [ , force ]
#	+  getfile nameOfParamGivingFilename
#	+  { enable | disable } intercept 
#
# NOT YET implemented:
#
#	__ ping [ <dest> ]
#	__ route { A|B|C|D } 
#		change la "route" des RPCs pour toutes les DESTINATIONS concernees.
#	__ shutdown slow
#		ferme la porte des RPCs et attend RPC_EXEC_TIMEOUT secondes 
#		pour laisser les RPCs en cours se terminer normalement.
#	__ ?terminate, ...
#
#
#
# This module is used by gxrpcmonitors.py
# ---------------------------------------
#
# The central PUBLIC functions here are:
#
#		execRegProc(aThread, frame, clt_conn)
#
#		isRegProcName(procName)
#
#		getListRegProcNames()
#
#
# WHEN ARE THEY CALLED?
#
# gxRegProcMonitor.runThread() calls gxregprocs.execRegProc().
#
# This happens each time that 
#  CallParserResPool.continueProcCallProcessing() [1] calls 
#
#	monitor = gxrpcmonitors.mapProcnameToRPCmonitor(procName)
#
# and that monitor is an instance of the class gxRegProcMonitor.
#
# gxrpcmonitors.mapProcnameToRPCmonitor() will return such an instance
# after it get True on the test (gxregprocs.isRegProcName(procName) ).
#
# (See gxrpcmonitors.py for details on the class gxRegProcMonitor.)
# 
#
# But, how did we get to [1]?
#
# The beginning of the call tree is:
#
#  class ClientsPollMonitor(gxmonitor.gxmonitor):
#	runThread():
#		pollOneTime():
#			processInputList():
#				self.master.addClientRequestToCallParserQueue(req)
#
#  class gxserver:
#	addClientRequestToCallParserQueue(...):
#		self.callParsersPool.addRequest(frame)
#		[...get from queue...]
#
#  class CallParserMonitor(gxmonitor.gxmonitor):
#	runThread():
#		self.parseAndProcessRequest(aThread)
#			aThread.pool.continueProcCallProcessing(aThread.getFrame(), procCall)
#
# -----------------------------------------------------------------------
#
# 2002jan29,jft: + first version begin
# 2002feb10,jft: + added: "who", "ps"
# 2002feb13,jft: . loginProc(): disconnect clt_conn if login fails
# 2002feb15,jft: . isLoginProc(): return a tuple (b_isLoginProc, b_isAdminUser)
# 2002feb16,jft: . loginProc(): ordinary login are _not_ done here. See gxrpcmonitors.py
# 2002feb22,jft: + who [ <sortMode> ] in whoProc()
# 2002mar01,jft: + getq <queue>
#		 + rpggq_get_rows <queue>
#		 + query { server | trace | gate | <dest> | ALLDEST | <queue> }
#		 + stats {<dest>|<queueName>} [, reset ]
# 2002mar08,jft: + gate { open|close|query}, { rpc | conn | <gateName> | <queue> | <dest>  }
# 2002apr07,jft: + "gwcmd"... same a "ggq" regproc (both are synonyms)
#		 + "stats" also accept "customprocpool" as name of a pool, also, name of pool can be "allpools"
# 2002apr13,jft: + sendReplyToClient(frame, msg, ret=None, msgNo=None, outParamsList=None)
# 2002aug16,jft: + clean { <dest> | all } [, force ]
# 2002aug16,jft: + support sz in colnames (simple version):
#		 . buildColumnsListFromNames(): if datatype is VARCHAR, sz = len(colname)
# 2002sep14,jft: . stats display time of stats reset
#		 + "help" regproc
# 2002sep17,jft: . execRegProc(): if needRpcGateOpen is True in the proc tuple, check if RPC gate is closed.
# 2002sep18,jft: . stats: new options FULL | SHORT, avg returned as a double
# 2002sep26,jft: . stats: 'reset' implies 'full'
# 2002oct20,jft: + "kill" -- return NotImplementedMsgTuple
# 2002nov30,jft: + getfile nameOfParamGivingFilename
#		 . sendQueryGate(): always tell about  Connect and RPC gates status, even if they are Open.
#				    larger column width (to avoid truncating some status msg)
# 2003mar05,jft: + { enable | disable } intercept 
# 2003jul04,jft: + tquery { server | trace | gate | <dest> | queue <queue> | <queue> | Q | ALLQUEUES | QUEUES | D | ALLDEST }
# 2003jul11,jft: . tquery gate: return one row per gate
# 2003jul29,jft: . tquery gate, alt: return one column per gate (useful in a monitor client)
# 2004may13,jbl: . Remove "login accepted" message to please VB
# 2004aug24,jft: . statsProc(): handle cases where some types of "intercept queue" (switch, function) are not really a queue.
# 2004aug25,jft: . gateProc(): convert boolean to int() in this comparison: b_open = int(name == KW_OPEN)
# 2005mai02,jft: + "inspect" regproc
# 2006feb01,jft: + "route" regproc / operator command
#


import time
import sys
import os.path		# for os.path.exists()

import TxLog
import traceMan
import gxparam
import gxmonitor
import gxthread
import gxrpcframe
import gxclientconn
import gxdestconn
import gxrespool
import gxintercept	# all RPC Intercept Queue functions
import gxqueue		# for .getStatsBucketsLimitsAsColNames()
import gxxmltpwriter
import gxutils
import gxreturnstatus
import gxconst
import xmltp_tags	# need tags to reply to client
import igw_udp		# for the constants

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxregprocs.py,v 1.5 2006/02/02 14:53:14 toucheje Exp $"

MODULE_NAME = "gxregprocs.py"

#
# CONSTANTS
#

# command names and keywords:
#
KW_LOGIN	= "login"

KW_HELP		= "help"
KW_GETFILE	= "getfile"

KW_ROUTE	= "route"
KW_SHUTDOWN	= "shutdown"
KW_STOP		= "stop"
KW_NOW		= "now"
KW_IMMEDIATE	= "immediate"
KW_SLOW		= "slow"
KW_FAST		= "fast"

KW_INACTIVATE	= "inactivate"

KW_ENABLE	= "enable"
KW_DISABLE	= "disable"
KW_INTERCEPT	= "intercept"

KW_TRACE	= "trace"
KW_LOG		= "log"
#
# Very few keyword constants for "trace" options because TraceMan.py knows many already
# ("low", "medium", "full").
#

KW_RPGGQ_GET_ROWS = "rpggq_get_rows"
KW_GETQ		= "getq"

KW_GWCMD	= "gwcmd"
KW_GGQ		= "ggq"
KW_DEBUG	= "debug"


KW_TEST		= "test"
# 
#
KW_WHO		= "who"
KW_SD		= "sd"

KW_PS		= "ps"
KW_RPC		= "rpc"
KW_SERVICE	= "service"

KW_INSPECT	= "inspect"

KW_QUERY	= "query"
KW_TQUERY	= "tquery"
KW_SERVER	= "server"
KW_TRACE	= "trace"
KW_GATE		= "gate"
KW_ALT		= "alt"
KW_GET		= "get"
KW_PUT		= "put"
KW_D		= "d"
KW_ALLDEST	= "alldest"
KW_Q		= "q"
KW_ALLQUEUES	= "allqueues"
KW_QUEUES	= "queues"
KW_QUEUE	= "queue"
KW_FULL		= "full"
KW_PING 	= "ping"

KW_GATE 	= "gate"
KW_OPEN		= "open"
KW_CLOSE	= "close"
KW_QUERY	= "query"
KW_RPC		= "rpc"
KW_CONN		= "conn" 

KW_PARSER	= "parser"
KW_REGPROCPOOL	= "regprocpool"
KW_CUSTOMPROCPOOL = "customprocpool"
KW_ALLPOOLS	= "allpools"

KW_STATS	= "stats"
KW_POLL		= "poll"
KW_CTR		= "ctr"
KW_NONE		= "none"
KW_LOW		= "low"
KW_MEDIUM	= "medium"
KW_FULL		= "full"
KW_RESET	= "reset"
KW_FULL	= "full"
KW_SHORT	= "short"
KW_RESWAIT 	= "reswait"
KW_EOD		= "eod"

KW_CLEAN	= "clean"
KW_ALL		= "all"
KW_FORCE	= "force"

KW_ROUTE	= "route"

KW_TERMINATE	= "terminate"



#
# module variables:
#
gxServer = None

dictRegProcs = None	# will be initialized on first call to mapRegProcNameToFunctionTuple()

dictAdminUsers = None


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
# Module variables:
#
m_ctrRejectsByMode = 0

OKdoneTuple		     = (0, "OK, done." )
NotImplementedMsgTuple       = (gxreturnstatus.GW_OPER_CMD_FAILED, "Not implemented")
MsgInternalErrorNoProcCall   = "Internal error #1"
InternalErrorNoProcCallTuple = (gxreturnstatus.GW_OPER_CMD_FAILED, MsgInternalErrorNoProcCall)

#
# PRIVATE functions:
#


#
# ---------- BEGIN ---------- sub-functions called by loginProc():
#
def writeLoginEntry(userid, clt_conn, answer):
	#
	# Called by:	loginProc()
	#		gxrpcmonitors.py
	#
	perm = getPermOfAdminUser(userid)
	if (perm != None):
		admin_str = ", (admin user, perm=%X)" % (perm, )
	else:
		admin_str = ""

	level = TxLog.L_MSG

	ret = -1
	if (type(answer) == type( () ) and len(answer) >= 1):
		ret = answer[0]

	if (answer == 0 or ret == 0):
		outcome = "OK"
	else:
		outcome = "Rejected %s" % (answer, )
		if (perm != None):
			level = TxLog.L_WARNING

	m1 = "Login: '%s' %s%s, %s." % (userid, outcome, admin_str, clt_conn)
	gxServer.writeLog(MODULE_NAME, level, ret, m1)


def checkLoginAdminUser(aThread, frame, clt_conn, userid, password):
	#
	# Called by:	loginProc()
	#
	perm = getPermOfAdminUser(userid)
	if (perm == None):
		answer = (-108, "Not an admin userid")	# strange because isAdminUser() was already done
	else:
		authPass = getPasswordForUser(userid)

		if (not authPass):
			answer = (-107, "problem with authPass, see init log.")

		elif (authPass == password):
			answer = assignCltConnUserid(clt_conn, userid, perm, None)
			return (answer)
		else:
			answer = (-100, "authentification rejected")

	assignCltConnUserid(clt_conn, None, 0, userid)

	return (answer)


def assignCltConnUserid(clt_conn, userid, perm, bogusId=None):
	try:
		clt_conn.assignUserid(userid, perm)
		#return ( (0, "login accepted") )		# authentification successful
		#Message removed to please the VB apps
		return ( (0, None) )				# authentification successful
	except:
		ex = gxutils.formatExceptionInfoAsString(1)

		if (userid != None):
			m1 = "assignCltConnUserid(): failed to assign userid %s, %s to %s. Exc: %s." % (userid, perm, clt_conn, ex)
		else:
			m1 = "assignCltConnUserid(): bogusId=%s, could NOT assign LOGIN_FAILED state in %s." % (bogusId, clt_conn)

		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		return ( (-105, "Already login or multiple attempts") )


def getPasswordForUser(userid):
	#
	# Called by:	checkLoginAdminUser()
	#
	param = gxparam.GXPARAM_PWD_FILE_OF_ + userid
	pwdFileName = gxServer.getOptionalParam(param)

	if (not pwdFileName):
		return None

	try:
		f = open(pwdFileName)
		s = f.readline()
		f.close()

		return s.strip()
	except:
		return None

	

def isAdminUser(userid):
	#
	# Called by:	loginProc()
	#
	return (getPermOfAdminUser(userid) != None)


def getPermOfAdminUser(userid):
	#
	# Called by:	checkLoginAdminUser()
	#		isAdminUser()
	#
	loadAdminUsersDict()	# make sure that dictAdminUsers is loaded

	try:
		return (dictAdminUsers[userid])
	except KeyError:
		return None

def loadAdminUsersDict():
	global dictAdminUsers

	if (dictAdminUsers != None):
		return

	dictAdminUsers = {}

	listOfConfigParamsAndPerm = [ 
		 ( gxparam.GXPARAM_WHO_HAS_LANG_PERM,     gxconst.PERM_LANG ),
		 ( gxparam.GXPARAM_WHO_HAS_OPER_PERM,     gxconst.PERM_OPER ),
		 ( gxparam.GXPARAM_WHO_HAS_STATS_PERM,    gxconst.PERM_GETFILE | gxconst.PERM_STATS),
		 ( gxparam.GXPARAM_WHO_HAS_SHUTDOWN_PERM, gxconst.PERM_SHUTDOWN),
		]

	for paramAndPerm in listOfConfigParamsAndPerm:
		param, perm = paramAndPerm

		userid = gxServer.getOptionalParam(param)
		while (userid):
			addPermToAdminUserInDict(userid, perm)

			userid = gxServer.getParamNextValue(param)

	m1 = "loadAdminUsers(): %d admin users in dict: %s" % (len(dictAdminUsers), dictAdminUsers)
	gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)


def addPermToAdminUserInDict(userid, newPerm):
	#
	# Called by:	loadAdminUsers() --- ONLY ---
	#
	try:
		perm = dictAdminUsers[userid]

	except KeyError:	# new one, check a few things first, then add it to the dictionary...
		perm = 0
		param = gxparam.GXPARAM_PWD_FILE_OF_ + userid
		pwdFileName = gxServer.getOptionalParam(param)

		if (not pwdFileName):
 			m1 = "addPermToAdminUserInDict(%s, %s): cfg param %s not found." % (userid, newPerm, param)
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

		elif (not os.path.exists(pwdFileName) ):
 			m1 = "addPermToAdminUserInDict(%s, %s): '%s' does not exist." % (userid, newPerm, pwdFileName)
			gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
		#
		# add it to the dictionary anyway...

	perm = perm | newPerm

	dictAdminUsers[userid] = perm

	return (0)	


#
# ---------- END ---------- End of sub-functions called by loginProc()
#

#
# NOTE: all xxxProc() functions are called by execRegProc()
#
def notImplementedProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	return NotImplementedMsgTuple

def loginProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	if (isAdminUser(p1) ):
		answer = checkLoginAdminUser(aThread, frame, clt_conn, p1, p2)

	elif (not (gxServer.getServerMode() & gxconst.MODE_PRIMARY) ):
		answer = (-102, "Logins are rejected. The server is not in Primary MODE.")
	else:
		# ordinary login not implemented here, but, in gxrpcmonitors.py
		# a bug in gxcallparser.py must be have brought us here!
		#
		answer = (-109, "normal login not implemented.")

	writeLoginEntry(p1, clt_conn, answer)

	rc = sendAnswerTupleToClient(frame, answer)

	if (not clt_conn.isLoginOK() ):
		clt_conn.processDisconnect("admin login has failed.")

	return (rc)



def shutdownProc(aThread, frame, clt_conn, usage, p1, p2, p3):

	gxServer.shutdown(clt_conn=clt_conn, b_immediate=1, b_quiet=0)

	tup = (0, "shutdown proceeding." )

	return (tup)


def getfileProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# + getfile nameOfParamGivingFilename
	#
	# Send back the content of the file whose name is given
	# by the value of the config param whose name is given by p1.
	#
	if (gxServer.isValInParamValuesList(p1, gxparam.GXPARAM_ALLOW_GETFILE_ON) ):
		fileName = gxServer.getOptionalParam(p1)
	else:
		msg = "param does not match values allowed by config file"
		return (gxreturnstatus.GW_OPER_CMD_FAILED, msg)

	# One column, 255 characters width
	#
	colName = "%-255.255s" % (fileName, )		## or should be: p1.strip() ??
	colList = buildColumnsListFromNames([ colName, ] )

	rowsList = []

	f = None
	try:
		f = open(fileName)
		while (1):
			s = f.readline()
			if not s:
				break
			rowsList.append([ s, ] )
			
		f.close()
		f = None
	except IOError:
		if (f):
			f.close()
			return (gxreturnstatus.GW_OPER_CMD_FAILED, "Error while processing file")
		return (gxreturnstatus.GW_OPER_CMD_FAILED, "File open failed")

	return (sendListOfRowsToClient(frame, rowsList, colList) )



def enableProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	return enableDisableProc_ext(aThread, frame, clt_conn, usage, p1, p2, p3, b_enable=1)

def disableProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	return enableDisableProc_ext(aThread, frame, clt_conn, usage, p1, p2, p3, b_enable=0)


def enableDisableProc_ext(aThread, frame, clt_conn, usage, p1, p2, p3, b_enable):
	#
	# + { enable | disable } intercept 
	#
	if (KW_INTERCEPT != p1.lower() ):
		m1 = "The only valid keyword is '%s'" % (KW_INTERCEPT, )
		return (gxreturnstatus.GW_OPER_CMD_FAILED, m1)

	gxintercept.assignEnableIntercept(b_enable)

	if b_enable:
		x = KW_ENABLE
	else:
		x = KW_DISABLE
	m1 = "'%s %s' done" % (x, KW_INTERCEPT)
	return (0, m1)


def helpProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# + help
	#
	# Send back a list of "usage" strings.
	#
	colList = buildColumnsListFromNames([ "usage of operator commands                                                             ", ])

	rowsList = []

	ls = getListRegProcNames()
	ls.sort()
	for name in ls:
		usage = getUsageOfRegProc(name)
		if (not usage):		# skip null value(s)
			continue

		valList = [ usage, ]

		rowsList.append(valList)

	return (sendListOfRowsToClient(frame, rowsList, colList) )



def traceQueryProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# Called by:	traceProc()
	#
	if (p2 and p2 != KW_LOG):
		info = traceMan.getTraceLevel(p2)

		msg = "%s: %s" % (p2, info)

		if (type(info) == type("s") ):
			ret = gxreturnstatus.GW_OPER_CMD_FAILED
		else:
			ret = 0

		return (ret, msg)

	if (p2 == KW_LOG):
		m1 = "Trace levels: \n%s" % (repr(traceMan.queryTrace() ) )

		gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		tup = (0, "Trace levels written to log." )
		return (tup)

	ls = traceMan.queryTrace()
	#
	# Return a LIST of TUPLES. Each tuple contains:
	#
	#		( groupName, name, traceLevel, versionId)
	#
	# return a string (an error msg) if fails.
	#
	if (type(ls) == type("s") ):
		msg = "error in traceMan.queryTrace(): %s" % (ls, )
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		return (ret, msg)

	colNames = ("groupName  ", "moduleName       ", "traceLevel", "versionId                                                                      ")

	return (sendListOfRowsWithColNamesToClient(frame, colNames, ls) )


def traceProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	#   trace <module>, { none | low | medium | full | <level> }
	# or
	#   trace query [, <module> ]
	#
	if (p1 == KW_QUERY):
		return (traceQueryProc(aThread, frame, clt_conn, usage, p1, p2, p3) )

	if (p2 == None):
		msg = "level argument is missing. Usage is: %s" % (usage, )
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		return (ret, msg)

	errMsg = traceMan.assignTraceLevel(p1, p2)

	if (errMsg == None):
		return (OKdoneTuple)

	msg = "'trace %.20s, %.20s' failed: %.170s" % (p1, p2, errMsg)
	ret = gxreturnstatus.GW_OPER_CMD_FAILED

	return (ret, msg)


def psProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	#  ps  [ rpc | service ]
	#
	if (p1 == KW_RPC):
		ls = gxthread.getListOfRPCThreads()
	elif (p1 == KW_SERVICE):
		ls = gxthread.getListOfServiceThreads()
	else:
		ls = gxthread.getListOfAllGXThreads()

	return (sendListOfRowsToClient(frame, ls) )

def inspectProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	#  inspect { thread_id | ALL }
	#
	if (p1 == KW_SERVICE):
		ls_threads = gxthread.getListOfServiceThreads()
		ls_idents_threads = map(lambda x: (x.getThreadIdent(), repr(x)), ls_threads)
	elif (not p1):
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		return (ret, usage)
	else:
		x = gxthread.getIdentOfThreadBygxthread_id(p1)
		if not x:
			msg = "'%s': no such thread_id." % (p1, )
			ret = gxreturnstatus.GW_OPER_CMD_FAILED
			return (ret, msg)
		ls_idents_threads = [ (x, p1), ]
					
	colList = buildColumnsListFromNames([ "thread_traceback" + 120*"_", ] )
	rowsList = []
	for tup in ls_idents_threads:
		ident, thr = tup
		rowsList.append( [repr(thr), ] )	# append the thread representation or thread_id (from p1)

		ls = gxthread.getTracebackOfThreadByIdent(ident)
		for s in ls:
			lines = s.splitlines()
			for x in lines:
				rowsList.append( [x, ] )	# append each line of a traceback
	rowsList.append( [70*"-", ] )

	return sendListOfRowsToClient(frame, rowsList, colList)


def whoProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	#  who [ sd, <sd> ]
	#  who [ sortMode ]
	#
	if (p1 == None):
		ls = gxclientconn.getListOfClients()
		return (sendListOfRowsToClient(frame, ls) )

	if (p1 != KW_SD):	# then p1 must be the sortMode
		try:
			sortMode = int(p1)
			ls = gxclientconn.getListOfClients(sortMode)
		except:
			ls = gxclientconn.getListOfClients(p1)

		return (sendListOfRowsToClient(frame, ls) )

	# then p2 must be a sd:
	#
	try:
		sd = int(p2)
	except:
		msg = "'%s' is not an integer" % (p2, )
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		return (ret, msg)

	clt_conn = gxclientconn.getClientConnFromSd(sd)
	if (clt_conn == None):
		msg = "'%s' is not a client connection." % (p1, )
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		return (ret, msg)

	ls = [ clt_conn, ]

	return (sendListOfRowsToClient(frame, ls) )


def getqProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# +  rpggq_get_rows  <queue>
	# +  getq	     <queue>
	# 
	ret = gxreturnstatus.GW_OPER_CMD_FAILED
	if (not p1):
		msg = "queueName argument missing"
		return (ret, msg)

	queue = gxintercept.getQueueByName(p1.lower() )
	if (queue == None):
		ret = gxreturnstatus.Q_POLL_CLOSED
		msg = "queue '%s' does not exist" % (p1, )
		return (ret, msg)

	tup = queue.getRowsAndColNamesList()
	if (tup == None):
		ret = gxreturnstatus.Q_POLL_EMPTY
		msg = None		# don't send msg for this case
		return (ret, msg)

	if (type(tup) != type(()) ):
		ret = gxreturnstatus.Q_POLL_CLOSED
		msg = "queue '%s' is closed" % (p1, )
		return (ret, msg)

	try:
		colList, rowsList = tup
	except:
		m1 = "getqProc(): '%s' queue.getRowsAndColNamesList(): returned bad tuple" % (p1, )
		gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, ret, m1)

		ret = gxreturnstatus.Q_POLL_CLOSED
		msg = "queue '%s' is Unusable" % (p1, )
		return (ret, msg)

	return (sendListOfRowsToClient(frame, rowsList, colList) )


def decodeShutdownOption(opt):
	#
	# Called by:	gwcmdProc()
	#
	if (opt == KW_NOW  or  opt == KW_IMMEDIATE  or  opt == KW_FAST):
		return igw_udp.UDP_OPT_SHUTDOWN_FAST

	if (opt == KW_SLOW):
		return igw_udp.UDP_OPT_SHUTDOWN_SLOW

	if (opt and len(opt) >= 1):
		return opt[0].upper()		# best guess

	return None


def decodeTraceOption(opt):
	#
	# Called by:	gwcmdProc()
	#
	if (opt == KW_NONE):
		return igw_udp.UDP_OPT_TRACE_NONE

	if (opt == KW_LOW):
		return igw_udp.UDP_OPT_TRACE_LOW

	if (opt == KW_MEDIUM):
		return igw_udp.UDP_OPT_TRACE_MEDIUM

	if (opt == KW_FULL):
		return igw_udp.UDP_OPT_TRACE_FULL

	return None



def gwcmdProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# Send a interface gateway command through a UDP message
	#

	# First, find the queue matching the queue name (p1)
	#
	qName = p1.lower()
	queue = gxintercept.getQueueByName(qName)
	if (queue == None):
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		msg = "queue '%s' does not exist." % (p1, )
		return (ret, msg)

	# Decode the command (p2) and its option (p3)
	#
	if (not p2):
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		msg = "You must give a command after the name of the queue."
		return (ret, msg)

	cmd = p2.lower()

	if (p3 == None):
		p3 = ""
	opt = p3.lower()

	n_cmd = -1			# -1 is not a valid n_cmd value
	optionChar = '\0'		# by default, no option char
	optionStr  = "\0"			# by default, empty string

	if (cmd == KW_RESET):
		n_cmd = igw_udp.UDP_CMD_RESET_LINES

	elif (cmd == KW_DEBUG):
		n_cmd = igw_udp.UDP_CMD_WRITE_DEBUG_INFO

	elif (cmd == KW_EOD):
		n_cmd = igw_udp.UDP_CMD_EOD

	elif (cmd == KW_INACTIVATE):
		n_cmd = igw_udp.UDP_CMD_INACTIVATE

	elif (cmd == KW_SHUTDOWN):
		n_cmd = igw_udp.UDP_CMD_SHUTDOWN
		optionChar = decodeShutdownOption(opt)
		if (optionChar == None):
			optionChar = '\0'

	elif (cmd == KW_TRACE):
		n_cmd = igw_udp.UDP_CMD_SET_TRACE
		optionChar = decodeTraceOption(opt)
		if (optionChar == None):
			optionChar = '\0'
		optionStr = p3			# send option string as is, not converted to lowercase

	elif (cmd == KW_OPEN):
		n_cmd = igw_udp.UDP_CMD_OPEN
		optionStr = p3			# send gate name as is, not converted to lowercase

	elif (cmd == KW_CLOSE):
		n_cmd = igw_udp.UDP_CMD_CLOSE
		optionStr = p3			# send gate name as is, not converted to lowercase

	if (n_cmd == -1):
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		msg = "command '%s' is not a recognized gateway command." % (p2, )
		return (ret, msg)

	queue.sendUDPcmdWithOptions(n_cmd, optionChar, optionStr)

	return (0, "Done")

def routeProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	""" route {A|B|C|D|E} [, dest]  -- A|B is usual.
	    If dest is ommited, then the route command applies to all destinations.
	"""
	if (not p1):
		msg = usage
		return (gxreturnstatus.GW_OPER_CMD_FAILED, msg)

	route = p1.upper()	# the routes are always specified in uppercase in the config file.

	if route < 'A' or route > 'E':
		msg = usage
		return (gxreturnstatus.GW_OPER_CMD_FAILED, msg)

	if p2:
		name = p2.lower()
		d = gxServer.getDestConnPoolByName(name)
		if not d:
			msg = "destination '%s' not found" % (name, )
			return (gxreturnstatus.GW_OPER_CMD_FAILED, msg)
		d.assignCurrentRoute(route)
		msg = "route changed to %s for dest '%s'. Do 'clean %s' to reconnect dest to new route. Check with 'tquery %s'." % (route, name, name, name, )
		return (0, msg)

	lsDestNames = gxServer.getAllDestConnPools()
	if not lsDestNames:
		return (gxreturnstatus.GW_OPER_CMD_FAILED, "No destination defined. List is empty.")

	for name in lsDestNames:
		d = gxServer.getDestConnPoolByName(name)
		if not d:
			continue

		d.assignCurrentRoute(route)

	msg = "Route changed to %s for all destinations. Do 'clean all' to reconnect all dest to new route. Check with 'tquery d' or 'tquery ALLDEST'." % (route,  )
	return (0, msg)


def tabularQueryProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# + tquery { server | trace | gate | <dest> | queue <queue> | <queue> | Q | ALLQUEUES | QUEUES | D | ALLDEST }
	#
	# "tquery" is NOT like "query". "tquery" returns a (tabular) result set, and, it has more options.
	#
	if (not p1):
		msg = usage
		return (gxreturnstatus.GW_OPER_CMD_FAILED, msg)

	arg = p1.lower()

	if (arg == KW_TRACE):			# Same as is queryProc()
		return (traceQueryProc(aThread, frame, clt_conn, usage, p1, p2, p3) )

	if (arg == KW_ALLQUEUES or arg == KW_QUEUES or arg == KW_Q):
		lsQueues = gxintercept.getNamesOfAllQueues()
		if not lsQueues:
			return (gxreturnstatus.GW_OPER_CMD_FAILED, "No intercept queues, list is empty.")

		lsQueues.sort()
		rows = []
		lastQ = None
		for name in lsQueues:
			q = gxintercept.getQueueByName(name)
			r = q.getStatusAsRow()
			rows.append(r)
			lastQ = q

		cols = lastQ.getStatusAsRowHeaders()

		return sendListOfRowsWithColNamesToClient(frame, cols, rows)
			
	if (arg == KW_D or arg == KW_ALLDEST):
		lsDestNames = gxServer.getAllDestConnPools()
		if not lsDestNames:
			return (gxreturnstatus.GW_OPER_CMD_FAILED, "No destination defined. List is empty.")

		lsDestNames.sort()
		rows = []
		lastDest = None
		for name in lsDestNames:
			d = gxServer.getDestConnPoolByName(name)
			if not d:
				continue
			r = d.getStatusAsRow()
			rows.append(r)
			lastDest = d

		if lastDest:
			cols = lastDest.getStatusAsRowHeaders()
		else:
			msg = "Cannot find any of %s destinations." % (len(lsDestNames), )
			return (gxreturnstatus.GW_OPER_CMD_FAILED, msg)

		return sendListOfRowsWithColNamesToClient(frame, cols, rows)


	if (arg == KW_GATE):
		b_manyColsOneRow = (p2 and p2.lower() == KW_ALT)
		if b_manyColsOneRow:
			x = gxServer.getGatesStatusAsRow(b_manyColsOneRow)	# one row, one column per gate
			rows = [ x, ]
		else:
			rows = gxServer.getGatesStatusAsRow()			# many rows, one row per gate

		cols = gxServer.getGatesStatusAsRowHeaders(b_manyColsOneRow)

		return sendListOfRowsWithColNamesToClient(frame, cols, rows)

	if (arg == KW_SERVER):
		x = gxServer.getStatusAsRow()
		rows = [ x, ]
		cols = gxServer.getStatusAsRowHeaders()

		return sendListOfRowsWithColNamesToClient(frame, cols, rows)

	if (arg == KW_PARSER):
		pool = gxServer.getCallParserPool()

		x = pool.getStatusAsRow()
		rows = [ x, ]
		cols = pool.getStatusAsRowHeaders()

		return sendListOfRowsWithColNamesToClient(frame, cols, rows)

	if (arg == KW_QUEUE):
		if (not p2):
			msg = "Missing second argument after '%s' keyword" % (KW_QUEUE, )
			ret = gxreturnstatus.GW_OPER_CMD_FAILED
			return (ret, msg)
		arg = p2.lower()

	obj = gxintercept.getQueueByName(arg)
	if not obj:
		obj = gxServer.getDestConnPoolByName(arg)

	if obj:
		x = obj.getStatusAsRow()
		rows = [ x, ]
		cols = obj.getStatusAsRowHeaders()

		return sendListOfRowsWithColNamesToClient(frame, cols, rows)

	msg = "'%s' is not a known destConnPool or InterceptQueue" % (arg, )
	ret = gxreturnstatus.GW_OPER_CMD_FAILED

	return (ret, msg)



def queryProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# + query { parser | server | trace | <dest> | <queue> | gate | QUEUES | ALLDEST }
	#
	# "query" is NOT like "tquery". "query" returns a message.
	#
	ret = gxreturnstatus.GW_OPER_CMD_FAILED
	if (not p1):
		msg = usage
		return (ret, msg)

	arg = p1.lower()

	if (arg == KW_SERVER):
		msg = gxServer.getShortStatus()
		ret = 0
		return (ret, msg)

	if (arg == KW_PARSER):
		msg = "CallParserPool: %s" % (gxServer.getCallParserPool(), )
		ret = 0
		return (ret, msg)

	if (arg == KW_TRACE):
		return (traceQueryProc(aThread, frame, clt_conn, usage, p1, p2, p3) )

	if (arg == KW_GATE):
		return (sendQueryGate(aThread, frame, clt_conn) )

	if (arg == KW_QUEUES or arg == KW_ALLDEST):
		tup =  (-1001, "NOT implemented!")
		return (tup)

	if (arg == KW_QUEUE):
		if (not p2):
			msg = "Missing second argument after '%s' keyword" % (KW_QUEUE, )
			ret = gxreturnstatus.GW_OPER_CMD_FAILED
			return (ret, msg)
		arg = p2.lower()

	queue = gxintercept.getQueueByName(arg)
	if (queue != None):
		msg = "queue: %s." % (queue, )
		ret = 0
		return (ret, msg)

	dest = gxServer.getDestConnPoolByName(arg)
	if (dest != None):
		msg = "destConnPool: %s." % (dest, )
		ret = 0
		return (ret, msg)

	msg = "'%s' is not a known destConnPool or InterceptQueue" % (arg, )
	ret = gxreturnstatus.GW_OPER_CMD_FAILED

	return (ret, msg)


def sendQueryGate(aThread, frame, clt_conn):
	#
	# Called by:	queryProc()
	#
	colList = buildColumnsListFromNames( ["Closed gates:                         ", ] )

	rowsList = []

	if (gxServer.isOrdinaryLoginAllowed() ):
		s = "Connect gate Open"
	else:
		s = "Connect gate CLOSED"
	rowsList.append( [ s, ] )

	if (gxServer.isRPCgateOpen() ):
		s = "RPC gate Open"
	else:
		s = "RPC gate CLOSED"

	rowsList.append( [ s, ] )

	for gateName in gxServer.getListOfClosedGates():
		valList = [ gateName, ]
		rowsList.append(valList)

	if (len(rowsList) <=2 ):
		rowsList.append( [ "<< No specific RPC gate is closed >>", ] )

	return (sendListOfRowsToClient(frame, rowsList, colList) )



def cleanProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# "clean { <dest> | all }  [ , force ]"
	#
	ret = gxreturnstatus.GW_OPER_CMD_FAILED
	if (not p1):
		msg = usage
		return (ret, msg)

	msgOperAccepted = "Operation accepted. See the results of the 'clean' operation in the log."

	b_force = (p2 != None  and  p2.lower() == KW_FORCE)

	name = p1.lower()

	if (name == KW_ALL):
		ls = gxrespool.assignOperationToAllPoolsOfThisType(gxdestconn.OPER_CLEAN, gxdestconn.DESTCONNPOOL_POOLTYPE, b_force)
		if (ls):
			return (ret, ls)
		return (0, msgOperAccepted)

	pool = gxServer.getDestConnPoolByName(name)
	if (not pool):
		msg = "No such pool '%s'" % (name, )
		return (ret, msg)


	msg = pool.assignOperation(gxdestconn.OPER_CLEAN, b_force)
	if (msg):
		return (ret, msg)		

	return (0, msgOperAccepted)




def statsProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# + stats {parser|regprocpool|<dest>|<queue>} [, reset | reswait ]
	#
	ret = gxreturnstatus.GW_OPER_CMD_FAILED
	if (not p1):
		msg = usage
		return (ret, msg)

	b_reswait = (p2 != None  and  p2.lower() == KW_RESWAIT) 
	b_reset   = (p2 != None  and  p2.lower() == KW_RESET)
	b_summary = 1
	if (p2 != None  and  p2.lower() == KW_FULL):
		b_summary = 0
	if (p3 != None  and  p3.lower() == KW_FULL):
		b_summary = 0

	if (b_reset):		# we want to have the FULL stats in the log.
		b_summary = 0

	name = p1.lower()
	if (name == KW_POLL):
		return (sendStatsForPollProcess(aThread, frame, clt_conn, gxServer.getPollProcessStats(), b_reset, b_summary) )

	resPoolList = None
	pool	    = None

	if (name == KW_PARSER):
		pool = gxServer.getCallParserPool()
	elif (name == KW_REGPROCPOOL):
		pool = gxServer.getRegProcExecPool()
	elif (name == KW_CUSTOMPROCPOOL):
		pool = gxServer.getCustomProcExecPool()
	elif (name == KW_ALLPOOLS):
		p1 = gxServer.getCallParserPool()
		p2 = gxServer.getRegProcExecPool()
		p3 = gxServer.getCustomProcExecPool()
		resPoolList = [p1, p2, p3]

		return (sendStatsForResPoolList(aThread, frame, clt_conn, resPoolList, b_reset, b_reswait, b_summary) )
	else:
		pool = gxServer.getDestConnPoolByName(name)



	if (pool != None):
		resPoolList = [pool, ]
		return (sendStatsForResPoolList(aThread, frame, clt_conn, resPoolList, b_reset, b_reswait, b_summary) )

	queue = gxintercept.getQueueByName(name)
	if (queue != None):
		try:
			x = queue.getQueue()
		except:		# some types of "intercept queue" (switch, function) are not really a queue...
			msg = "'%s' does not have stats." % (p1, )
			ret = gxreturnstatus.GW_OPER_CMD_FAILED
			return (ret, msg)

		queuesList = [queue, ]
		return (sendStatsForQueuesList(aThread, frame, clt_conn, queuesList, b_reset, b_summary) )

	msg = "'%s' is not a known destConnPool or InterceptQueue" % (p1, )
	ret = gxreturnstatus.GW_OPER_CMD_FAILED

	return (ret, msg)



def sendStatsForPollProcess(aThread, frame, clt_conn, pollProcessStats, b_reset, b_summary):
	#
	# Called by:	statsProc()
	#
	# Send back the stats about the poll process delay.
	#

	colList = buildColumnsListFromNames([ "timeOfReset            ", ])

	# gxqueue.getStatsBucketsLimitsAsColNames() returns a list of headers for
	# the serie of stats "buckets" counters.
	#
	cols2 = ["Poll delay avg (ms)", ] +  gxqueue.getStatsBucketsLimitsAsColNames(b_summary)

	# buildColumnsListFromNames() with the datatype=xmltp_tags.JAVA_SQL_DOUBLE
	# argument will build a colList full of DOUBLE datatype...
	#
	colList = colList + buildColumnsListFromNames(cols2, datatype=xmltp_tags.JAVA_SQL_DOUBLE)

	rowsList = []

	lagAvg		= pollProcessStats.getAverage()
	lagBucketsList	= pollProcessStats.getCtrBuckets(b_summary)

	
	tReset = pollProcessStats.getTimeReset()
	tResetStr = TxLog.get_log_time(tReset)

	valList = [ tResetStr, (lagAvg * 1000), ]

	valList.extend(lagBucketsList)

	rowsList.append(valList)

	if (b_reset):
		m1 = "poll() thread stats: %s." % (colList, )
		gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		pollProcessStats.resetCounters()

	return (sendListOfRowsToClient(frame, rowsList, colList) )



def sendStatsForResPoolList(aThread, frame, clt_conn, resPoolList, b_reset, b_reswait=0, b_summary=0):
	#
	# Called by:	statsProc()
	#
	# NOT USED:
	#   resQ.getStatsAverageHistory(self, statsType):
	#

	# The first 4 columns in the colList are 
	# the only columns which have a VARCHAR type (string).
	# All other ones are DOUBLE.
	#
	colList = buildColumnsListFromNames( ["PoolName____timeOfReset", "Status", "Action", ] )

	# gxqueue.getStatsBucketsLimitsAsColNames() returns a list of headers for
	# the serie of stats "buckets" counters.
	#
	cols2 = [ "Avg (ms)", "Total Ctr", ] + gxqueue.getStatsBucketsLimitsAsColNames(b_summary)

	# buildColumnsListFromNames() with the datatype=xmltp_tags.JAVA_SQL_DOUBLE
	# argument will build a colList full of DOUBLE datatype...
	#
	colList = colList + buildColumnsListFromNames(cols2, datatype=xmltp_tags.JAVA_SQL_DOUBLE)

	rowsList = []
	for pool in resPoolList:
		#
		# For each Pool in the resPoolList, (3) three rows will be added in the rowsList:
		#	. one with the stats of the waitQ
		#	. 2nd with the stats of the succesfull uses of the res from the resQ
		#	. 3rd with the stats of the failed uses of the resources.
		#
		# If (b_reswait), a fourth row of stats will be added. It will show how log
		# resources have waited before being used. It might be useful to pinpoint 
		# bottlenecks. 
		#
		waitQ = pool.getWaitQueue()
		resQ  = pool.getResourcesQueue()

		tReset = waitQ.getStatsTimeReset(gxqueue.STATS_QUEUED_TIME)
		tResetStr = TxLog.get_log_time(tReset)

		tupWait = (pool.getName(), pool.getEnableStatus(), "Wait-Q", waitQ, gxqueue.STATS_QUEUED_TIME)
		tupOK   = (tResetStr,	   "",			   "Use OK", resQ,  gxqueue.STATS_OUT_OF_QUEUE)
		tupFail = ( "",		   "",			   "Failed", resQ,  gxqueue.STATS_OUT_OF_QUEUE_ALT)

		if (b_reswait):
			tupResWait = ( "", "",			   "res Wq", resQ,  gxqueue.STATS_QUEUED_TIME)

		queuesList = [ tupWait, tupOK, tupFail ]
		if (b_reswait):
			queuesList.append(tupResWait)

		for qTup in queuesList:
			poolName, qStat, descr, qQ, statsType = qTup

			avg	    = qQ.getStatsAverage(statsType)
			n	    = qQ.getStatsN(statsType)
			bucketsList = qQ.getStatsCtrBuckets(statsType, b_summary)

			avg = avg * 1000.0

			values = [ poolName, qStat, descr, avg, n, ]

			values.extend(bucketsList)

			rowsList.append(values)

			if (b_reset):
				m1 = "pool '%s' stats: %s." % (pool.getName(), values)
				gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

		if (b_reset):
			waitQ.resetStats()
			resQ.resetStats()

	return (sendListOfRowsToClient(frame, rowsList, colList) )


def sendStatsForQueuesList(aThread, frame, clt_conn, queuesList, b_reset, b_summary):
	#
	# Called by:	statsProc()
	#
	# NOT USED:
	#   resQ.getStatsAverageHistory(self, statsType):
	#

	# The first 3 columns in the colList have a VARCHAR type (string).
	# The following ones are DOUBLE.
	#
	colList = buildColumnsListFromNames( ["Queue Name", "Status", "timeOfReset            ", ] )

	# gxqueue.getStatsBucketsLimitsAsColNames() returns a list of headers for
	# the serie of stats "buckets" counters.
	#
	cols2 = ["Wait avg (ms)", ] + gxqueue.getStatsBucketsLimitsAsColNames(b_summary)

	# buildColumnsListFromNames() with the datatype=xmltp_tags.JAVA_SQL_DOUBLE
	# argument will build a colList full of DOUBLE datatype...
	#
	colList = colList + buildColumnsListFromNames(cols2, datatype=xmltp_tags.JAVA_SQL_DOUBLE)

	rowsList = []
	for gwQueue in queuesList:
		waitQ = gwQueue.getQueue()

		waitAvg		= waitQ.getStatsAverage(gxqueue.STATS_QUEUED_TIME)
		waitBucketsList = waitQ.getStatsCtrBuckets(gxqueue.STATS_QUEUED_TIME, b_summary)

		waitAvg = waitAvg * 1000.0

		tReset = waitQ.getStatsTimeReset(gxqueue.STATS_QUEUED_TIME)
		tResetStr = TxLog.get_log_time(tReset)

		values = [ gwQueue.getName(), gwQueue.getEnableStatus(), tResetStr, waitAvg ]

		values.extend(waitBucketsList)

		rowsList.append(values)

		if (b_reset):
			m1 = "queue '%s' stats: %s." % (gwQueue.getName(), values)
			gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)

			waitQ.resetStats()

	return (sendListOfRowsToClient(frame, rowsList, colList) )


def gateProc(aThread, frame, clt_conn, usage, p1, p2, p3):
	#
	# + gate { open|close|query}, { rpc | conn | <gateName> | <queue> | <dest> }
	#
	ret = gxreturnstatus.GW_OPER_CMD_FAILED
	if (not p1):
		msg = usage
		return (ret, msg)

	name = p1.lower()
	if (name == KW_QUERY):
		return (sendQueryGate(aThread, frame, clt_conn) )

	if (name != KW_CLOSE and name != KW_OPEN):
		msg = usage
		return (ret, msg)

	b_open = int(name == KW_OPEN)
	if (b_open):
		msg_2 = "Open"
	else:
		msg_2 = "Close"

	if (not p2):
		msg = usage
		return (ret, msg)

	arg = p2.lower()
	if (arg == KW_RPC):
		gxServer.assignRPCsAllowed(b_open)
		msg = "RPC gate: " + msg_2
		ret = 0
		return (ret, msg)

	if (arg == KW_CONN):
		gxServer.assignOrdinaryLoginAllowed(b_open)
		msg = "Connect gate: " + msg_2
		ret = 0
		return (ret, msg)

	if (gxServer.checkIfValidRPCgroupGateName(arg) ):
		rc = gxServer.assignRPCsGroupGateState(arg, b_open)
		if (rc == 0):
			msg = "Gate '%s': %s" % (arg, msg_2)
			ret = 0
			return (ret, msg)

		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		if (rc == -1):
			msg = "Gate '%s' does not exist" % (arg, )
		elif (rc == -2):
			msg = "Gate '%s' is already %s." % (arg, msg_2)
		else:
			msg = "Gate '%s': error: '%s'." % (arg, rc)

		return (ret, msg)

	queue = gxintercept.getQueueByName(arg)
	if (queue != None):
		b_openGet = None 
		b_openPut = None
		if (p3):
			opt = p3.lower()
		else:
			opt = None
		if (opt == KW_GET):
			b_openGet = b_open
		elif (opt == KW_PUT):
			b_openPut = b_open
		elif (opt == None):
			b_openGet = b_open
			b_openPut = b_open
		else:
			msg = "gate %s, %s, '%s' : unknown option '%s'" % (name, arg, opt, opt)
			ret = gxreturnstatus.GW_OPER_CMD_FAILED
			return (ret, msg)

		queue.assignOpenStatus(b_openGet=b_openGet, b_openPut=b_openPut)

		s = queue.getEnableStatus()

		msg = "%s: %s" % (arg, s)
		return (0, msg)

	pool = gxServer.getDestConnPoolByName(arg)
	if (pool != None):
		pool.assignOpenStatus(b_open)
		msg = "%s %s: done." % (arg, name)
		return (0, msg)
	
	msg = "'%s' is not a known RPC group name, nor a DestConnPool, nor an InterceptQueue" % (arg, )
	ret = gxreturnstatus.GW_OPER_CMD_FAILED

	return (ret, msg)



#
# sub-functions called by various xxxProc() functions:
#
dictPythonTypeToJavaSQLtype = { type(1)   : xmltp_tags.JAVA_SQL_INTEGER,
				type(2.9) : xmltp_tags.JAVA_SQL_DOUBLE,
				type("s") : xmltp_tags.JAVA_SQL_VARCHAR,
			      }

def getJavaSQLtypeOfPythonObject(obj):
	#
	# Called by:	buildColumnsListFromNamesAndFirstRowValues()
	#
	try:
		return (dictPythonTypeToJavaSQLtype[type(obj)] )
	except:
		return (xmltp_tags.JAVA_SQL_VARCHAR)


def buildColumnsListFromNames(colNames, datatype=None):
	#
	# Called by:	sendListOfRowsToClient()
	#		sendStatsForResPoolList()
	#		sendStatsForQueuesList()
	#		sendStatsForPollProcess()
	#
	# Return:
	#	<string>	error message
	#	<list>		a list containing tuples such as:
	#				( "name_1",  xmltp_tags.JAVA_SQL_VARCHAR, 6 )
	#				( "name_22",  xmltp_tags.JAVA_SQL_VARCHAR, 7 )
	#
	if (type(colNames) != type( () ) and type(colNames) != type( [] ) ):
		return "colNames is not a tuple or a list"

	if (datatype == None):
		datatype = xmltp_tags.JAVA_SQL_VARCHAR

	ls = []
	for col in colNames:
		if (datatype == xmltp_tags.JAVA_SQL_VARCHAR):
			sz = len(col)
		else:
			sz = 0
		tup = (col, datatype, sz)
		ls.append(tup)

	return (ls)


def buildColumnsListFromNamesAndFirstRowValues(colNames, aRow):
	#
	# Called by:	sendListOfRowsWithColNamesToClient()
	#
	# Return:
	#	<string>	error message
	#	<list>		a list containing tuples such as:
	#				( "name   ",  xmltp_tags.JAVA_SQL_VARCHAR, 7 )
	#				( "level", xmltp_tags.JAVA_SQL_INTEGER,    0 )
	#
	try:
		if (len(colNames) != len(aRow) ):
			msg = "colNames and aRow do not have the same number of elements (%s != %s)", (len(colNames), len(aRow) )
			return (msg)
	except:
		msg = "colNames and/or aRow are not tuple or list"
		return (msg)

	ls = []
	n = len(aRow)
	for i in xrange(0, n):
		col = colNames[i]
		val = aRow[i]
		jType = getJavaSQLtypeOfPythonObject(val)
		if (jType == xmltp_tags.JAVA_SQL_VARCHAR):
			sz = len(col)		# not perfect, but good enough
		else:
			sz = 0
		tup = (col, jType, sz)
		ls.append(tup)

	return (ls)
		

def sendListOfRowsWithColNamesToClient(frame, colNames, rowsList):
	#
	# Called by:	traceQueryProc()
	#
	try:
		colList = buildColumnsListFromNamesAndFirstRowValues(colNames, rowsList[0] )
	except:
		colList = gxutils.formatExceptionInfoAsString(1)

	if (type(colList) == type("s") ):
		msg = "error: %s" % (colList, )
		ret = gxreturnstatus.GW_OPER_CMD_FAILED

		return (sendErrMsgToClient(frame, msg, ret) )

	return (sendListOfRowsToClient(frame, rowsList, colList) )




def sendListOfRowsToClient(frame, rowsList, colList=None):
	#
	# Called by:	sendListOfRowsWithColNamesToClient()
	#		whoProc()
	#		psProc()
	#		getqProc()
	#
	# NOTES:
	# whoProc() and psProc() call this function with colList=None.
	#
	# if colList is None, then we assume that rowsList is a list of Objects
	# which all have a asTuple() method. Also, we call rowsList[0].asTuple(1)
	# to get a list of colNames and buildColumnsListFromNames() will assume 
	# that each element in the tuple returned by obj.asTuple can be 
	# defined as a xmltp_tags.JAVA_SQL_VARCHAR.
	#
	if (type(rowsList) != type( () ) and type(rowsList) != type( [] ) ):
		msg = "error: rowsList is not a list!"
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		return (sendErrMsgToClient(frame, msg, ret) )

	if (len(rowsList) <= 0):
		msg = "List is empty. No rows."
		return (sendErrMsgToClient(frame, msg, 0) )

	if (colList == None):
		try:
			colList = buildColumnsListFromNames(rowsList[0].asTuple(1) )
		except:
			colList = gxutils.formatExceptionInfoAsString(2)

	if (type(colList) == type("s") ):
		msg = "error: %s" % (colList, )
		ret = gxreturnstatus.GW_OPER_CMD_FAILED

		return (sendErrMsgToClient(frame, msg, ret) )

	rc = gxxmltpwriter.sendResultSetAndReturnStatusToClient(frame, colList, rowsList, 0)

	if (rc != 0):
		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		m1 = "sendResultAndReturnStatusToClient(): Could not send rows to %s." % (frame, )
		gxServer.writeLog(MODULE_NAME,TxLog.L_WARNING, ret, m1)

		raise RuntimeError, "sendListOfRowsToClient(): gxxmltpwriter.sendResultSetAndReturnStatusToClient() FAILED"

	return (0)

#
# sub-functions called by execRegProc():
#

def sendAnswerTupleToClient(frame, answer):
	#
	# answer can be a tuple: (ret, msg) or an integer...
	#
	try:
		ret, msg = answer
	except:
		if (type(answer) == type(1) ):
			ret = answer
		else:
			ret = -997
		msg = None

	return (sendErrMsgToClient(frame, msg, ret) )	# if success, return (0)


#
# module Functions -- PUBLIC functions:
#
def sendErrMsgToClient(frame, msg, ret=None, msgNo=None):
	#
	# Called by:	regprocs in gxregprocs.py (here)
	#		regprocs in custom modules
	#
	return (sendReplyToClient(frame, msg, ret, msgNo, outParamsList=None) )


def sendReplyToClient(frame, msg, ret=None, msgNo=None, outParamsList=None):
	#
	# Called by:	sendErrMsgToClient()
	#		regprocs in custom modules
	#
	if (ret == None):
		ret = gxreturnstatus.GW_OPER_CMD_FAILED

	if (msgNo == None):
		msgNo = ret

	if (msg == None):
		msgList = None
	else:
		msgList = [ (msgNo, msg), ]

	return (sendReplyToClientWithMsgList(frame, msgList, ret, msgNo, outParamsList) )


def sendReplyToClientWithMsgList(frame, msgList, ret=None, msgNo=None, outParamsList=None):
	#
	# Called by:	sendReplyToClient()
	#		execRegProc()
	#
	if (ret == None):
		ret = gxreturnstatus.GW_OPER_CMD_FAILED

	if (msgNo == None):
		msgNo = ret

	# If msgList is a list, but, not a list of tuples, such a [ (msgNo, str), ...]
	# then make it into one like that:
	#
	if (msgList and type(msgList[0]) != type( () ) ):
		ls = []
		for m in msgList:
			tup = (msgNo, m)
			ls.append(tup)
		msgList = ls


	rc = gxxmltpwriter.replyToClientExt(frame, msgList, ret, outParamsList)

	if (rc != 0):
		m1 = "sendReplyToClient(): Could not send reply, with msgList to %s." % (frame, )
		gxServer.writeLog(MODULE_NAME,TxLog.L_WARNING, ret, m1)

		raise RuntimeError, "sendReplyToClient(): gxxmltpwriter.replyToClientExt() FAILED"

	return (0)





#
# -------------- All regprocs are executed through execRegProc(): -------------------
#
# NOTE: all regproc functions are private.
#
def execRegProc(aThread, frame, clt_conn):
	#
	# Called by:	gxrpcmonitors.py :: gxRegProcMonitor.runThread()
	#
	# Return:
	#	0	OK, clt_conn still good
	#	<= -1	could not reply to client
	#
	# Possible return status :
	#	0
	#	gxreturnstatus.GW_OPER_CMD_PERM_DENIED
	#	gxreturnstatus.GW_OPER_CMD_FAILED
	#	gxreturnstatus.DS_FAIL_CANNOT_RETRY	-- for weird internal errors
	#
	global  m_ctrRejectsByMode

	if (aThread == None or frame == None or clt_conn == None):
		m1 = "Bad args to execRegProc(): %s %s %s." % (aThread == None, frame == None, clt_conn == None)
		gxServer.writeLog(MODULE_NAME,TxLog.L_MSG, 0, m1)
		return (-9)

	procName = frame.getProcName()

	tup = mapRegProcNameToFunctionTuple(procName)

	if (tup == None or type(tup) != type( () )  or len(tup) != 6 ):
		m1 = "regproc '%s' not defined. (%d)." % (procName, tup == None )

		msgNo = gxreturnstatus.OS_BAD_REG_PROC_DEF

		gxServer.writeLog(MODULE_NAME,TxLog.L_WARNING, msgNo, m1)

		ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY
		msg = "regproc '%s' cannot be executed. (See log)." % (procName, )

		frame.assignReturnStatus(ret)

		return (sendErrMsgToClient(frame, msg, ret) )

	#
	# Unpack the tuple from the regProc definitions table:
	#
	func, reqPerm, reqMode, needRpcGateOpen, minNbParams, usage = tup

	#
	# check the permission, and, check the mode:
	#
	if (reqPerm != gxconst.PERM_ANY and not (clt_conn.getOperPerm() & reqPerm) ):
		m1 = "Permission denied on regproc '%s' (%d) to client %s (%d)." % (procName, reqPerm, clt_conn, clt_conn.getOperPerm() )

		ret = gxreturnstatus.GW_OPER_CMD_PERM_DENIED

		gxServer.writeLog(MODULE_NAME,TxLog.L_WARNING, ret, m1)

		msg = "Permission denied."

		frame.assignReturnStatus(ret)

		return (sendErrMsgToClient(frame, msg, ret) )


	if (reqMode != gxconst.MODE_ANY and not (gxServer.getServerMode() & reqMode) ):

		ret = gxreturnstatus.getReturnStatusForModeMismatch(gxServer.getServerMode() )

		m_ctrRejectsByMode = m_ctrRejectsByMode + 1
		if (m_ctrRejectsByMode < 10):
			m1 = "regproc '%s' reject because current mode is %s." % (procName, gxServer.getServerMode() )
			gxServer.writeLog(MODULE_NAME,TxLog.L_WARNING, ret, m1)

		msg = "Server is not 'ready' (%s)." %(gxServer.getServerMode(), )

		frame.assignReturnStatus(ret)

		return (sendErrMsgToClient(frame, msg, ret) )

	#
	# if needRpcGateOpen, then check if RPC gate is open...
	#
	if (needRpcGateOpen and not gxServer.b_rpc_allowed):
		ret = gxreturnstatus.OS_RPC_GATE_CLOSED
		msg = gxServer.getUnavailabityMessage(0, "rpc")
		frame.assignReturnStatus(ret)

		return (sendErrMsgToClient(frame, msg, ret) )

	
	#
	# check if there are enough parameters:
	#
	procCall = frame.getProcCall()

	if (procCall == None):
		ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY
		msg = MsgInternalErrorNoProcCall
		frame.assignReturnStatus(ret)
		return (sendErrMsgToClient(frame, msg, ret) )

	ls = procCall.getParamsList()
	if (ls):
		nbParams = len(ls)
	else:
		nbParams = 0

	if (nbParams < minNbParams):
		msg = "Missing param(s) for %s. Usage is %s." % (procName, usage)

		ret = gxreturnstatus.GW_OPER_CMD_FAILED
		frame.assignReturnStatus(ret)

		return (sendErrMsgToClient(frame, msg, ret) )

	#
	# Get the first three (3) parameters from the procCall structure:
	#
	p1 = gxrpcframe.getValueOfParam(procCall.getParamN(0) )
	p2 = gxrpcframe.getValueOfParam(procCall.getParamN(1) )
	p3 = gxrpcframe.getValueOfParam(procCall.getParamN(2) )


	#
	# Execute the Registered Procedure function:
	#
	rc = func(aThread, frame, clt_conn, usage, p1, p2, p3)

	if (type(rc) != type( () ) ):
		if (rc == 0):		# OK, successful
			frame.assignReturnStatus(0)
			return (rc)

		if (rc <= -1):
			m1 = "Failed: execRegProc(): '%s' could NOT reply to client %s. (%s)" % (procName, clt_conn, rc)

			gxServer.writeLog(MODULE_NAME,TxLog.L_WARNING, rc, m1)
			return (rc)

		ret = gxreturnstatus.DS_FAIL_CANNOT_RETRY
		msg = "'%s' failed with %s." (procName, rc)
	else:
		#
		# rc is a tuple (ret, msg). Try to send the msg to the client connection:
		#
		ret, msg = rc

	frame.assignReturnStatus(ret)

	if (msg != None and type(msg) == type( [] ) ):
		return (sendReplyToClientWithMsgList(frame, msg, ret) )

	return (sendErrMsgToClient(frame, msg, ret) )	# if success, return (0)
#
# End of execRegProc()
#


def resetCtrRejectsByMode():
	global m_ctrRejectsByMode

	m_ctrRejectsByMode = 0


def getListRegProcNames():
	#
	# make sure that dictRegProcs is initialized:
	#
	x = mapRegProcNameToFunctionTuple("trace")

	return (dictRegProcs.keys() )	

def isRegProcName(procName):
	x = mapRegProcNameToFunctionTuple(procName)
	return (x != None)


def isLoginProc(procCall):
	#
	# Called by:	CallParserResPool.continueProcCallProcessing() --- ONLY ---
	#
	procName = procCall.getProcName()

	userid = gxrpcframe.getValueOfParam(procCall.getParamN(0) )

	b_isLoginProc = (procName == KW_LOGIN)

	return (b_isLoginProc, isAdminUser(userid) )


def mapRegProcNameToFunctionTuple(procName):
	#
	# return the function for procName, or, None if no match.
	#
	global dictRegProcs

	if (dictRegProcs == None):
		dictRegProcs = { 
		  KW_LOGIN	: (loginProc,	 gxconst.PERM_ANY,	gxconst.MODE_ANY,	0, 2, None),
		  KW_HELP	: (helpProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 0, "help"),
		  KW_ROUTE	: (routeProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	1, 1, "route {A|B|C|D|E} [, dest]  -- A|B is usual."),
		  KW_SHUTDOWN	: (shutdownProc, gxconst.PERM_SHUTDOWN,	gxconst.MODE_ANY,	0, 0, "shutdown"),
		  KW_TRACE	: (traceProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 1, "trace module, { NONE | LOW | MEDIUM | FULL | level }"),
		  KW_GETFILE	: (getfileProc,	 gxconst.PERM_STATS,	gxconst.MODE_ANY,	0, 1, "getfile nameOfParamGivingFilename"),
		  KW_WHO	: (whoProc,	 gxconst.PERM_STATS,	gxconst.MODE_ANY,	0, 0, "who [ sort_order ]"),
		  KW_PS		: (psProc,	 gxconst.PERM_STATS,	gxconst.MODE_ANY,	0, 0, "ps  [ RPC | SERVICE ]"),
		  KW_INSPECT	: (inspectProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	1, 1, "inspect thread_id (from 'ps' list). Do NOT use casually in Prod!"),
		  KW_QUERY	: (queryProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 1, "query { SERVER | TRACE | GATE | dest | queue }"),
		  KW_TQUERY	: (tabularQueryProc, gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 1, "tquery { SERVER | TRACE | GATE | dest | queue | Q | ALLQUEUES | QUEUES | D | ALLDEST }"), 
		  KW_GATE	: (gateProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 1, "gate { OPEN|CLOSE|QUERY}, { RPC | CONN | gate | dest | queue }"),
		  KW_STATS 	: (statsProc,	 gxconst.PERM_STATS,	gxconst.MODE_ANY,	0, 1, "stats {dest|queue|POLL|PARSER|REGPROCPOOL|CUSTOMPROCPOOL|ALLPOOLS} [, RESET|FULL|SHORT ]"),
		  KW_CLEAN	: (cleanProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 1, "clean { dest | ALL }  [ , FORCE ]"),
		  KW_DISABLE	: (disableProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 1, "disable intercept"),
		  KW_ENABLE	: (enableProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 1, "enable intercept"),
		  KW_GGQ	: (gwcmdProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 2, "ggq   queue, cmd [, opt ]"),
		  KW_GWCMD	: (gwcmdProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 2, "gwcmd queue, cmd [, opt ]"),
		  KW_GETQ	: (getqProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY,	1, 1, "getq  queue"),
		  KW_RPGGQ_GET_ROWS:(getqProc,	 gxconst.PERM_OPER,	gxconst.MODE_ANY, 	1, 1, "rpggq_get_rows queue"),
		  "kill": (notImplementedProc, gxconst.PERM_OPER,	gxconst.MODE_ANY,	0, 0, "kill Not implemented "),
		}

	try:
		return (dictRegProcs[procName] )
	except KeyError:
		return (None)


def getUsageOfRegProc(procName):
	tup = mapRegProcNameToFunctionTuple(procName)
	if (not tup):
		return "Unknown usage."

	if (len(tup) < 6):
		s = "No usage string for %s" % (procName, )
		return s

	return tup[5]



if __name__ == '__main__':

	print "mapRegProcNameToFunctionTuple('test'):", mapRegProcNameToFunctionTuple('test')

####	print "No UNIT TEST yet !"
