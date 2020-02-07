#!/usr/bin/python

#
# gxclientconn.py : Gateway-XML - Client Connection
# -------------------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxclientconn.py,v $
# $Header: /ext_hd/cvs/gx/gxclientconn.py,v 1.3 2006-05-17 17:01:13 toucheje Exp $
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
# A "clientConn" (gxClientConn instance) is a semi-persistent 
# data structure that lives as long as a client is connected to
# the server.
#
#
# Each gxClientConn is associated with one or zero gxRPCframe.
#
# -----------------------------------------------------------------------
#
# 2001nov21,jft: first version
# 2002jan17,jft: . processDisconnect(): call gxServer.writeLogClientDisconnect()
# 2002jan29,jft: + added standard trace functions
# 2002jan30,jft: + import gxconst for PERM_xxx constants
# 2002feb10,jft: + asTuple()
# 2002feb20,jft: + releaseClientsListAndDictLock(): used by gxClientConn.__init__() ...
#		 + acquireClientsListAndDictLock():  and by gxClientConn.processDisconnect()
#		 + clientsListAndDictLock = thread.allocate_lock()
#		  	 NOTE: threading.Lock() acquire(), release() seem bogus.
# 2002feb22,jft: . removed listOfClients, now only use dictSdToClientConn.
# 2002apr14,jft: . rpcEndedReturnConnToPollList(): fixed if's around self.assignRPCframe(None, msg)
#		 + class PseudoClientConn(gxClientConn)
# 2002apr16,jft: . gxclientconn.disconnectAllClients(): disconnect all clients, but, not conn == clt_conn (arg)
# 2002sep11,jft: . use localtime() instead of gmtime()
# 2002oct02,jft: + Class TempPseudoClientConn(PseudoClientConn)
#		 + gxclientconn.removeFromdictSdToClientConn(sd, methodName)
# 2002oct21,jft: . TempPseudoClientConn.rpcEndedReturnConnToPollList(): if (destConn.b_processingPrivRequest), do not complain about failed Test RPC.
# 2003feb10,jft: . gxclientconn.processDisconnect(): does NOT call addClientToListOfThoseToRemoveFromPollList()
#			 call self.removeFromdictSdToClientConn(sd, "processDisconnect()") BEFORE self.socket.close()
# 2006apr05,jft: . gxclientconn.__init__() and .removeFromdictSdToClientConn() : log "STRANGE..." msg only if sd >= 0
#


import time
import sys
import thread		# for thread.allocate_lock()
import TxLog
import gxconst
import gxutils

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxclientconn.py,v 1.3 2006-05-17 17:01:13 toucheje Exp $"

MODULE_NAME = "gxclientconn.py"


#
# module Constants:
#
# the STATE_xxx constants are used as indice in the .eventsTable[] array
#
STATE_INIT		   = 0
STATE_LOGIN_IN_PROGRESS	   = 1		# obsolete
STATE_NO_LOGIN_YET	   = 1		# should use this one instead
STATE_FIRST_CALL_PARSE     = 2
STATE_LOGIN_FAILED	   = 3
STATE_LOGIN_OK		   = 4
STATE_RPC_CALL_IN_PROGRESS = 5.0
STATE_REGPROC_IN_PROGRESS  = 5.1
STATE_RPC_CALL_ENDED_OK	    = 6.0
STATE_RPC_CALL_ENDED_FAILED = 6.1
STATE_WAIT_FOR_POLL_EVENT  = 7
STATE_DISCONNECTING	   = 8
STATE_INVALID_TRANSITION   = 9



statesDict = { 	STATE_INIT		: "INIT",
		STATE_NO_LOGIN_YET	: "NO_LOGIN_YET",
		STATE_FIRST_CALL_PARSE  : "1st_Call_parse",
		STATE_LOGIN_FAILED	: "LOGIN_FAILED",
		STATE_LOGIN_OK		: "LOGIN_OK",
		STATE_RPC_CALL_IN_PROGRESS : "RPC_in_progress",
		STATE_REGPROC_IN_PROGRESS  : "RegProc",
		STATE_RPC_CALL_ENDED_OK	   : "RPC_success",
		STATE_RPC_CALL_ENDED_FAILED: "RPC_failed",
		STATE_WAIT_FOR_POLL_EVENT  : "WAIT_for_poll_event",
		STATE_DISCONNECTING	   : "Disconnecting",
		STATE_INVALID_TRANSITION   : "InvalidTransition",
	    }

#
# Module variables:
#
gxServer = None		# the master object of this GX server

frameTupleColNames = None	# must be assigned at run-time by gxserver 

# WARNING: We must manipulate dictSdToClientConn in
#	   a way that is thread-safe. 
#	   We use a "lock" to do this: clientsListAndDictLock
#
clientsListAndDictLock = thread.allocate_lock()		# NOTE: threading.Lock() acquire(), release() seem bogus.

dictSdToClientConn = {}	# map a socket fd to the gxClientConn which holds it

#
# Removed listOfClients, now only use dictSdToClientConn. (2002feb22,jft)
#
### listOfClients = []	# list of all client connections created here and
###			# still connected (not yet fully disconnected).

#
# module-scope debug_level:
#
debug_level = 0


# Module functions:
#
def asTupleColNames():
	ls = ["user id   ", " sd ", "perm", " state       ", "timestamp         "]
	if (frameTupleColNames):
		ls.extend(frameTupleColNames)
	return tuple(ls)


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


def acquireClientsListAndDictLock(funcName, connDescription):
	#
	# this function is used to BEGIN a  Critical Section
	#
	nbTries = 0
	maxTries = 8		# will let waitDelay increase from 1 usec to 10 seconds
	waitDelay = 0.000001	# 1 usec

	while (nbTries <= maxTries):
		# m1 = "acquireClientsListAndDictLock(): ABOUT to acquire() for %s time. %s, %s." % (nbTries, funcName, connDescription)
		# gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		rc = clientsListAndDictLock.acquire(0)
		if (rc == 1):
			return (0)	# OK, lock acquired

		if (nbTries >= 4):
			if (nbTries < maxTries):
				m1 = "acquireClientsListAndDictLock(): acquire() failed %s times, will retry after %s ms. %s, %s." % (nbTries, 
									(waitDelay * 1000), funcName, connDescription)
				level = TxLog.L_MSG
			else:
				m1 = "acquireClientsListAndDictLock(): acquire() FAILED %s times, NO MORE retry. %s, %s." % (nbTries, 
													funcName, connDescription)
				level = TxLog.L_WARNING
			gxServer.writeLog(MODULE_NAME, level, 0, m1)

		if (nbTries < maxTries):
			time.sleep(waitDelay)
			waitDelay = waitDelay * 10

		nbTries = nbTries + 1

	return (-1)


def releaseClientsListAndDictLock(funcName, connDescription):
	#
	# to END a Critical Section
	#
	try:
		clientsListAndDictLock.release()

		# m1 = "releaseClientsListAndDictLock(): SUCCESS release(). %s, %s." % (funcName, connDescription)
		# gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
	except:
		ex =  gxutils.formatExceptionInfoAsString()
		m1 = "releaseClientsListAndDictLock(): FAILED release(), Exc: %s, %s, %s." % (ex, funcName, connDescription)
		gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

		pass	# in case it is called when already unlocked



def getStateDescription(state):
	try:
		return ( statesDict[state] )
	except KeyError:
		return ( "[unknown state %s]" % (state, ) )

def getListOfClients(sortType=1):
	#
	# sortType:
	#	0	unsorted
	#	1	sort keys first (sd), then build list of values (DEFAULT)
	#	2	sort the values() themselves (this order is Not so useful)
	#
	# Used to be:
	# 	return (listOfClients)
	#
	if (dictSdToClientConn == None or len(dictSdToClientConn) == 0):
		return []

	if (sortType == 1 or sortType == "1"):
		ls = []
		keysSorted = dictSdToClientConn.keys()
		keysSorted.sort()
		for k in keysSorted:
			ls.append(dictSdToClientConn[k] )
		return (ls)

	if (sortType == 2 or sortType == "2"):
		valuesSorted = dictSdToClientConn.values()
		valuesSorted.sort()
		return (valuesSorted)

	# un-sorted:
	#
	return (dictSdToClientConn.values() )


def getNumberOfClients():
	if (dictSdToClientConn == None):
		return (0)

	return (len(dictSdToClientConn) )


def getClientConnFromSd(sd):
	try:
		return (dictSdToClientConn[sd] )
	except KeyError:
		return None


def disconnectAllClients(clt_conn, b_quiet):
	#
	# Called by:	gxserver.shutdownFinal()
	#
	# Disconnect all clients, but, ourself (clt == clt_conn)
	#
	ctrDone = 0
	msgOurself = ""
	try:
		ls = getListOfClients(0)	# 0: unsorted
		for clt in ls:
			if (clt == clt_conn):	# do not disconnect ourself!
				msgOurself = "(but not ourself!)"
				continue
			clt.processDisconnect("[server shutdown]", b_shutdown=1, b_quiet=b_quiet)
			ctrDone = ctrDone + 1

		m1 = "disconnectAllClients(): closed %s of %s client conn. %s" % (ctrDone, len(ls), msgOurself)
		gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
	except:
		ex = gxutils.formatExceptionInfoAsString()
		m1 = "disconnectAllClients(): FAILED after closing %s client conn, Exc: %s." % (ctrDone, ex)
		gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

#
# Classes:
#
class gxClientConn:
	#
	# Each Client Connection has several attributes, not only a
	# socket.
	#
	# Also the .eventsTable[] array gives a snapshot of the last 
	# events that have occurred on this client connection.
	# See getEventsList() for details.
	#
	def __init__(self, socket, optionDetailedTimings=0):
		self.socket	= socket
		self.optionDetailedTimings = optionDetailedTimings
		self.userid	= None	# after successful login, a string
		self.b_loginFailed = 0	# become true after failed login attempt
		self.operPerm   = gxconst.PERM_NONE
		self.RPCframe	= None	# gxRPCframe
		self.msg	= "init"
		self.msgTime	= None
		self.timeCreated = time.time()
		self.eventsTable = [ None, None, None, None,  
				     None, None, None, None,
				     None, None ]	 # 10 elements, 9 used

		self.state	= None # avoid "chicken and egg" paradox...
		self.assignState(STATE_INIT)

		acquireClientsListAndDictLock("__init__()", self.getSd() )	# Critical Section BEGIN
		#
		# Under heavy load, it becomes obvious that dict or list manipulations are not thread safe!
		#
		# Therefore, the following " critical section" is between an acquire() and release() on a lock.
		#
		sd = self.getSd()
		try:
			oldConn = dictSdToClientConn[sd]
			if sd >= 0:
				m1 = "STRANGE: creating new client conn with sd=%s, but, found old conn with same sd: %s. Replacing it anyway." % (sd, oldConn)
				gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		except KeyError:
			pass	# OK, this is expected.

		dictSdToClientConn[sd] = self
	
		releaseClientsListAndDictLock("__init__()", self.getSd())	# Critical Section END


	def __repr__(self):
		if (STATE_DISCONNECTING == self.state):	# be careful with multi-threads interactions!
			return "<Disconnecting>"

		sd	= self.getSd()
		userId	= self.getUserId()
		state	= self.describeState()

		return ("<%s %s %s %s>" % (sd, userId, state,  
					   self.formatTimestamp() ) )

	def getShortRepr(self):
		if (STATE_DISCONNECTING == self.state):	# be careful with multi-threads interactions!
			return "<Disconnecting>"

		sd	= self.getSd()
		userId	= self.getUserId()
		return ("<%s %s>" % (sd, userId) )


	def asTuple(self, b_onlyColNames=0):
		if (b_onlyColNames):
			return (asTupleColNames() )

		#
		# ("user id   ", " sd ", "perm", " state       ", "timestamp    ")
		#
		if (STATE_DISCONNECTING == self.state):	# be careful with multi-threads interactions!
			ls1 = [ "<Disconnecting>", ]
			ls2 = [ "", ]
			ls2 = ls2 * 9
			ls1.extend(ls2)
			return tuple(ls1)

		id      = "%10.20s" % (self.getUserId(),   )
		sd      = "%3d"     % (self.getSd(), 	   )
		perm    = "%02X"    % (self.getOperPerm(), )
		state   = "%13.13s" % (self.describeState(),  )
		tStamp  = self.formatTimestamp()

		ls = [id, sd, perm, state, tStamp]

		if (self.RPCframe == None):
			frameLs = [ "", ]
			frameLs = frameLs * 5
		else:
			frameLs = self.RPCframe.asTuple()

		ls.extend( frameLs )

		return tuple(ls)


	def formatTimestamp(self):
		if (STATE_DISCONNECTING == self.state):
			return ("[disc-noTime]")	# avoid crashing
		tm = self.getTimeLastActivity()
		i = int(tm)
		ms = tm - i
		lt = time.localtime(i)
		s1 = time.strftime("%d-%b %H:%M:%S", lt)
		return ("%s.%03d" % (s1, ms * 1000) )

	def getTimeLastActivity(self):
		if (STATE_DISCONNECTING == self.state):
			return (-1)	# avoid crashing
		i = int(self.state)
		step, tm, msg = self.eventsTable[i]
		return (tm)

	def getSocket(self):
		if (STATE_DISCONNECTING == self.state):
			return None	# this is safer if disconnecting

		return (self.socket)

	def getSd(self):
		if (STATE_DISCONNECTING == self.state):
			return None	# this is safer if disconnecting

		if (self.socket != None):
			return (self.socket.fileno() )
		return (-1)

	def getOptionDetailedTimings(self):
		return (self.optionDetailedTimings)

	def getUserId(self):
		if (None == self.userid):
			return "[not login]"
		return (self.userid)

	def isLoginOK(self):
		if (STATE_DISCONNECTING == self.state):
			return None	# this is safer if disconnecting

		return (self.userid != None and self.userid != "")

	def getOperPerm(self):
		return (self.operPerm)

	def getRPCframe(self):
		if (STATE_DISCONNECTING == self.state):
			return None	# this is safer if disconnecting

		return self.RPCframe

	def describeState(self):
		return getStateDescription(self.state)

	def isDisconnected(self):
		return (STATE_DISCONNECTING == self.state)

	def assignState(self, state, msg=None, b_isLoginProc=0):
		#
		# remember state and time when state was assigned.
		# Also remember msg if present.

		# Some state transitions would be mistakes or security breach:
		#
		if (STATE_DISCONNECTING == state):	# this state cannot assigned like this.
			raise RuntimeError, "assignState(STATE_DISCONNECTING) denied"

		if (STATE_DISCONNECTING == self.state):
			raise RuntimeError, "clientConn disconnecting, assignState() denied"

		if (self.userid == None  and  STATE_RPC_CALL_IN_PROGRESS == state):
			raise RuntimeError, "clientConn Not login yet, assignState() RPC_CALL_IN_PROGRESS denied"

		if ( self.userid == None  and  STATE_REGPROC_IN_PROGRESS == state \
		 and (self.state != STATE_FIRST_CALL_PARSE  or  not b_isLoginProc) ):
			raise RuntimeError, "clientConn login non complete, assignState() STATE_REGPROC_IN_PROGRESS denied"

		self.state = state
		i = int(state)	# some state values are 5.0, 5.1, convert to int!
		self.eventsTable[i] = (state, time.time(), msg )

	def getEventsList(self):
		#
		# return a list of the recent states-events and timeStamps 
		# this connection has gone through.
		#
		# each element of the list is a tuple:
		#		 (stateDescriptionString, timeStamp, msg)
		#
		# This list is useful for debugging, server snapshots (like "who")
		#
		if (STATE_DISCONNECTING == self.state):
			return []	# avoid crashing
		lst = []
		for event in self.eventsTable:
			if (event == None):
				continue
			state, tm, msg = event
			lst.append( (getStateDescription(state), tm, msg) )
		return (lst)

	def assignRPCframe(self, frame, msg=None):
		#
		# Raise exception if already disconnecting, or Login never started...
		#
		if (STATE_DISCONNECTING == self.state):
			raise RuntimeError, "clientConn disconnecting, assign RPCframe denied"

		if (STATE_INIT == self.state):
			raise RuntimeError, "clientConn has not Login yet, assign RPCframe denied"

		if (self.b_loginFailed):
			raise RuntimeError, "clientConn has FAILED Login, assign RPCframe denied"
		#
		# if frame == None AND msg != None, then, RPC has failed 
		# and msg is a diagnostic message.
		#
		self.RPCframe = frame

		# if (frame == None), this is case when RPC has ended...
		#
		if (None == frame):
			if (msg != None):
				self.assignState(STATE_RPC_CALL_ENDED_FAILED)
			else:
				self.assignState(STATE_RPC_CALL_ENDED_OK)
			return (0)

		# if (frame != None), then this is case when RPC begins...
		#
		if (self.userid == None):
			self.assignState(STATE_FIRST_CALL_PARSE)
			return (0)

		self.assignState(STATE_RPC_CALL_IN_PROGRESS)


	def assignUserid(self, userid, operPerm=0):
		if (STATE_DISCONNECTING == self.state):
			m1 = "clientConn disconnecting, assign userid denied (%s)" % (userid, )
			raise RuntimeError, m1
			return None	# do not do anything if disconnecting

		self.userid = userid
		if (None == userid):
			self.assignState(STATE_LOGIN_FAILED)
			self.b_loginFailed = 1
			self.operPerm = gxconst.PERM_NONE
		else:
			self.operPerm = operPerm
			self.assignState(STATE_LOGIN_OK)


	def rpcEndedReturnConnToPollList(self, b_rpc_ok, msg=None):
		if (STATE_DISCONNECTING == self.state):
			return None	# do not do anything if disconnecting

		if (not b_rpc_ok):
			if (None == msg):
				self.assignRPCframe(None, "[RPC failed??]")
			else:
				self.assignRPCframe(None, msg)
		else:
			self.assignRPCframe(None)

		self.assignState(STATE_WAIT_FOR_POLL_EVENT)

		if (gxServer != None):
			gxServer.returnClientConnToPollList(self.getSd() )


	def processDisconnect(self, msg=None, b_shutdown=0, b_quiet=0):
		#
		# This method is dangerous. 
		# Other threads might access this object (self) while the method 
		# is proceeding here.
		#
		# So, all other methods accessing this type of objects should first
		# test if (STATE_DISCONNECTING == self.state).
		# If this condition is true, they must return ASAP.
		# They should NOT access any other attribute!
		#
		if (STATE_DISCONNECTING == self.state):
			return None	# cannot be done twice.

		prevState = self.state

		self.state = STATE_DISCONNECTING

		if (self.socket != None):
			sd = self.socket.fileno()	# copy value while still available
		else:
			sd = -2
			if (gxServer != None  and  not b_quiet):
				gxServer.writeLogClientDisconnect(sd,   self.userid, getStateDescription(prevState), msg)

			return (None)	# no socket to close

		try:
			if (gxServer != None):
				try:
					if (not b_quiet):
						gxServer.writeLogClientDisconnect(sd,   self.userid, 
								  getStateDescription(prevState), msg)

					self.removeFromdictSdToClientConn(sd, "processDisconnect()")
				except:
					ex = gxutils.formatExceptionInfoAsString()
					m1 = "processDisconnect() did NOT complete entirely (sd=%s), Exc: %s." % (sd, ex)
					gxServer.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
		except:
			exc_inf = sys.exc_info()
			print "processDisconnect()!!! Exc: %s, %s, %s" % exc_inf

		if (self.socket != None):
			#
			# We do close() HERE. And, that's all: gx_poll has _already_ removed the fd from the poll_tab.
			#
			try:
				self.socket.close()
			except: pass

		self.socket = None



	def removeFromdictSdToClientConn(self, sd, methodName):
		#
		# Called by:	gxClientConn.processDisconnect()
		#		TempPseudoClientConn.rpcEndedReturnConnToPollList()
		#

		acquireClientsListAndDictLock(methodName, sd)		# Critical Section BEGIN
		#
		# Under heavy load, it becomes obvious that dict or list manipulations are not thread safe!
		#
		# Therefore, the following " critical section" is between an acquire() and release() on a lock.
		#
		try:
			del dictSdToClientConn[sd]
		except KeyError:
			if sd >= 0:
				m1 = "STRANGE: disconnecting sd=%s, but, not found in dict of clients" % (sd, )
				gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		releaseClientsListAndDictLock(methodName, sd)		# Critical Section END


	def rememberErrorMsg(self, m1, m2):
		self.msg = "%s, %s" % (m1, m2)
		self.msgTime = time.time()


#
# pseudoClientConn_lastPseudoSd is used by PseudoClientConn:
#
pseudoClientConn_lastPseudoSd = -10		# see PseudoClientConn.__init__()


class PseudoClientConn(gxClientConn):
	#
	# Instances of this class pretends to be gxClientConn, but, are not.
	#
	# These instances are used by service threads which send ProcCall. 
	# For example, the thread of an "active queue" (see gxintercept.py
	# to find these).
	#
	# NOTES:
	#   When the instance is created, __init__() must received the argument masterToNotify.
	#   This masterToNotify object must have a method .notifyRPCcomplete(b_rpc_ok, msg)
	#
	#   That method will be called by <PseudoClientConn>.rpcEndedReturnConnToPollList()...
	#   typically, at the end of the RPC call. (On success or failure).
	#
	#   This mechanism should allow to implement proper thread synchronisation in 
	#   masterToNotify (which will probably use queueProcCallToDestPool()
	#   in gxrpcmonitors to execute arbitrary RPC calls asynchronously).
	#
	#
	# Only some methods are overidden. These are the methods that cannot behave
	# exactly like the real gxClientConn methods:
	#	getSd(self)
	#	rpcEndedReturnConnToPollList(self, b_rpc_ok, msg=None):
	#	processDisconnect(self, msg=None)
	#	
	def __init__(self, masterToNotify=None):
		global pseudoClientConn_lastPseudoSd

		if (not masterToNotify):
			raise RuntimeError, "masterToNotify argument is missing!"

		self.masterToNotify = masterToNotify	### master must have a method: self.masterToNotify.notifyRPCcomplete(b_rpc_ok, msg)

		if (pseudoClientConn_lastPseudoSd >= 0):
			raise RuntimeError, "pseudoClientConn_lastPseudoSd MUST be negative!"

		self.pseudoSd = pseudoClientConn_lastPseudoSd

		pseudoClientConn_lastPseudoSd = pseudoClientConn_lastPseudoSd - 1	# Yes, decrement!

		gxClientConn.__init__(self, None)      # superclass __init__()

	def getSd(self):
		return (self.pseudoSd)


	def rpcEndedReturnConnToPollList(self, b_rpc_ok, msg=None):

		self.masterToNotify.notifyRPCcomplete(b_rpc_ok, msg)

		if (not b_rpc_ok):
			if (None == msg):
				self.assignRPCframe(None, "[RPC failed??]")
			else:
				self.assignRPCframe(None, msg)
		else:
			self.assignRPCframe(None)

		self.assignState(STATE_WAIT_FOR_POLL_EVENT)	# we wait, yes, but not for a poll event...

		### we do not return the sd to the poll list!  We do not have a real sd!


	def processDisconnect(self, msg=None, b_shutdown=0, b_quiet=0):
		#
		# This method should NEVER be called for a PseudoClientConn!!!
		#
		# It is absurd to call this method here...
		#
		# So, if this method is called, it only log an ERROR message in the log and
		# does nothing else.
		#
		if (b_shutdown):	# this method will be called at shutdown, never mind.
			return
		m1 = "PseudoClientConn.processDisconnect() was called!!! sd=%s, userid=%s, msg=%s." % (self.getSd(), self.userid, msg)
		gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1)



class TempPseudoClientConn(PseudoClientConn):
	#
	# Instances of this class are similar to a PseudoClientConn, but,
	# they remove themselves from the dictionary of client conn when
	# .rpcEndedReturnConnToPollList() is called.
	#

	def rpcEndedReturnConnToPollList(self, b_rpc_ok, msg=None):
		#
		# NOTE: this is an unusual implementation of a method that is
		#	called by usually generic RPC monitors. 
		#	This version here is only called for after a "test" (first) RPC
		#	has been sent by a Destination (a pool).
		#
		### we do not return the sd to the poll list!  We do not have a real sd!

		frame = self.getRPCframe()
		ret = frame.getReturnStatus()

		if (ret == 0):
			gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Test RPC successful")
		else:
			destConn = frame.getDestConn()
			if (destConn.b_processingPrivRequest):
				m1 = "priviledged RPC, ret=%s" % (ret, )
				gxServer.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
			else:
				gxServer.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, "Test RPC FAILED -- ATTENTION!")

		self.assignRPCframe(None)

		self.removeFromdictSdToClientConn(self.pseudoSd, "processDisconnect()")



if __name__ == '__main__':
	conn = gxClientConn(None)	# socket == None! only for testing

	print conn

	conn.assignState(STATE_NO_LOGIN_YET, "dummyUserid")

	print "events list:", conn.getEventsList()

	conn.assignRPCframe("dummyRPCframe_NOT_VALID")

	print "events list:", conn.getEventsList()

	print getListOfClients(0)

	print "getClientConnFromSd( 22 )", getClientConnFromSd(22)
	print "getClientConnFromSd( -1 )", getClientConnFromSd(-1)

	conn.assignRPCframe(None, "dummyRPC has failed (this is unit test)")

	print "events list:", conn.getEventsList()

	conn.assignUserid("USER_006")

	for clt in getListOfClients(1):
		print clt

	conn.processDisconnect()

	print "events list:", conn.getEventsList()

	print getListOfClients(1)
	print "getClientConnFromSd( -1 )", getClientConnFromSd(-1)
