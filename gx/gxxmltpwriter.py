#!/usr/bin/python

#
# gxxmltpwriter.py : Gateway-XML XMLTP writer
# -------------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxxmltpwriter.py,v $
# $Header: /ext_hd/cvs/gx/gxxmltpwriter.py,v 1.3 2005/03/18 17:21:49 toucheje Exp $
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
# Functions to write XMLTP (XML) on socket connection(s), also, class SocketBuffer
# ------------------------------------------------------
#
# DESCRIPTION of this module:
#
# There are (2) two types of functions:
#	. the one to send a remote procedure call (on a DestConn, if the 
#		caller is gxserver.py)
#	. those to return a response to a client. These responses
#	  could be:
#		- return status only
#		- return status and message(s)
#		- the above combined with output parameters
#		- the above, also with one or many result set(s).
#
# The API has (2) two levels:
#	. high-level: functions which have a gxRPCframe argument, 
#			(which contains a gxClientConn or a gxDestConn).
#			These high-level functions will trap exceptions
#			and put suitable states and diagnostics in 
#			the "frame".
#	. low-level: functions which receive a socket argument.
#			These low-level functions do NOT handle exception
#			by themselves. Their caller must use "try: except:"
#			to recover exceptions related to socket I/O.
#
#	  NOTE: the socket arg passed to low-level functions can be a SocketBuffer
#		object, or an ordinary Python socket object. See the functions
#		definitions for hints about usage.
#
# Examples of high-level functions:
#
#  Response...
#	sendResultSetAndReturnStatusToClient(rpcFrame, colList, rowsList, returnStatus)
#
#	replyErrorToClient(rpcFrame, msgList, returnStatus, b_sendXMLTPreset=0)
#		sends an error response to rpcFrame.getClientConn()
#
#	replyToClientExt(rpcFrame, msgList, returnStatus, outParamsList=None, b_sendXMLTPreset=0)
#
#	sendResponseToClient(rpcFrame, msgList, returnStatus, resultSetsList=None, outParamsList=None, callingFunctName=None, b_sendXMLTPreset=0)
#		sends a response to rpcFrame.getClientConn(), including (or not) result set(s), output params, ...
#		** sendResponseToClient() is the most general function to send a reponse to a client connection.
#
#  ProcCall...
#	sendProcCall(rpcFrame)
#		sends the rpcFrame.getProcCall() to the rpcFrame.getDestConn(), with retry.
#
#
# -----------------------------------------------------------------------
#
# 2001nov23,jft: first version
# 2001dec11,jft: . sendProcCallWithRetry(): on except: do not return too quick on sock == None, wait for retry instead.
# 2001dec24,jft: . sendProcCallWithRetry(): on error, do: rc = destConn.verifyDestConn(1) before retrying
# 2002jan29,jft: + added standard trace functions
# 2002feb09,jft: + sendResultSetAndReturnStatusToClient(rpcFrame, colList, rowsList, returnStatus)
#		 + getXMLTPvalueTagForPythonObject(obj)
# 2002feb10,jft: . sendOneRow(row): call row = row.asTuple() when row is not already a sequence.
# 2002mar02,jft: + escapeForXML(val)
# 2002mar12,jft: . replyErrorToClient(): check if (rpcFrame.isReplyDone() )
# 2002mar27,jft: + SocketBuffer: a class used to send a ProcCall in one single sock.send() 
# 2002apr13,jft: + replyToClientExt(rpcFrame, msgList, returnStatus, outParamsList=None, b_sendXMLTPreset=0):
#		 + sendoutParamsList(outParamsList, sock)
# 2002may13,jft: . sendColNames(): support sz in col tuple
# 2002may24,jft: . moved sockBuff.flush() from sendProcCall() to sendProcCallWithRetry(), this allows proper
#			retry if the connection is broken. The function only finds out at send().
# 2002may31,jft: . SocketBuffer.flush() use xmltp_gx.sendString()
# 2002aug20,jft: . escapeForXML(): use "&gt;"  "&lt;"  "&amp;"
#			. prepValueForXML(): only call escapeForXML() if val is a string with a & < or >
# 2002aug21,jft: . sendParams(): value = prepValueForXML(value)
# 2003jan28,jft: . class SocketBuffer: second version, use "Python Cookbook" recipe to concat many strings efficiently:
#		 	SocketBuffer.send(): self.chunksList.append(str)
#		 	SocketBuffer.flush(): str = "".join(self.chunksList)
#		 + sendResponseToClient(rpcFrame, msgList, returnStatus, resultSetsList=None, outParamsList=None, callingFunctName=None, b_sendXMLTPreset=0)
#		 . sendResultSetAndReturnStatusToClient(), replyErrorToClient(), ... call sendResponseToClient()
#		 + sendResponseOnSocket(sock, msgList, returnStatus, resultSetsList=None, outParamsList=None, b_flushSocket=1, b_sendXMLTPreset=0)
# 2003aug19,jft: . escapeForXML(): takes care of xmltp_tags.META_CHAR_APOS and META_CHAR_QUOT
# 2004mar16,jft: . sendOneResultSet(): pass colsDatatypesList to sendOneRow()
#		 . sendOneRow(): uses xmltp_tags.getValueTagForType() ... like sendParams() does
# 2005mar17,jft: . sendResponseToClient(): on exception, log traceback.
#

import time
import sys
import traceback
import gxthread
import gxrpcframe
import gxutils
import gxreturnstatus
import xmltp_tags
import TxLog
import xmltp_gx		# only for sendString()

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxxmltpwriter.py,v 1.3 2005/03/18 17:21:49 toucheje Exp $"
MODULE_NAME = "gxxmltpwriter"

#
# Module variables:
#
srvlog = None		# will be assigned by main module

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
# Classes:
#
class SocketBuffer:
	#
	# This is the second version of class SocketBuffer.
	#
	# Here, we use a "Python Cookbook" recipe to optimise string manipulations.
	# Multiple strings concatenations in a growng "buffer" are costly.
	# (Each time a new, larger, string object is created).
	# Here, a SocketBuffer object provides a functionnaly equivalent
	# strategy:
	#   .send() appends str to a list, self.chunksList
	#   .flush() concatenates allo those only on time using the 
	#		more efficient String method .join().
	#   the resulting large string is sent in one single call to xmltp_gx.sendString()
	#   NOTE: self.sock.send(str) would be OK except that it seems it forgets to
	#	  to detect peer disconnect, sometimes. (jft,2003jan28)
	#
	# FINE-TUNING of VERY LARGE RESULT SETS:
	# If you send very large result set, you might want to add a instance variable .maxChunksBeforeFlush
	# and modify the method .send() to call .flush() when the size of self.chunksList reaches
	# that limit.
	# In that case, .send() should return the value returned by .flush() if it is <= -1
	# and the caller of .send() should check this return code to detect a disconnect
	# or other network I/O error.
	#
	def __init__(self, sock):
		self.sock   = sock
		self.chunksList = []
		self.errmsg = None
		self.err_rc = 0
	
	def __repr__(self):
		str = "<SocketBuffer: nb chunks=%d, sock=%s>" % (len(self.chunksList), self.sock)
		return str

	def send(self, str):
		self.chunksList.append(str)	# this is more efficient than multiple string concatenations
		return len(str)

	def flush(self):
		str = "".join(self.chunksList)	# this is more efficient than multiple string concatenations

		self.chunksList = []	## reset "buffer"

		# rc = self.sock.send(str)
		#
		# NOTE: xmltp_gx.sendString() releases the "global interpreter lock"
		#	for the duration of the send(). This allows better multi-threading.
		#	META-NOTE: Python implementation of socket.send() method also releases
		#		that lock before doing the network I/O. (jft,2003jan28).
		#
		#	Also, xmltp_gx.sendString() should detect peer disconnect every time.
		#	<socket>.send() seems to be unreliable for this. (jft, 2002mai30)
		#
		n = xmltp_gx.sendString(self.sock.fileno(), str)

		if (n != len(str) ):
			if (n <= -5):
				self.errmsg = "internal error %s in xmltp_gx.sendString()" % (n, )
				self.err_rc = n
				m1 = "SocketBuffer.flush(): %s" % (self.errmsg, )
				raise RuntimeError, m1

			self.errmsg = "send() failed. See .err_rc"
			self.err_rc = n

			raise RuntimeError, "send(): connection broken"

		return 0

	def reset(self):
		self.chunksList = []	## reset "buffer"


#
# Module functions:
#
# There is no class definition here, only functions.
#

#
# Utility functions: (PUBLIC)
#
dictPythonTypeToXMLTPtag = {	type(1)   : xmltp_tags.XMLTP_TAG_INT,
				type(2.9) : xmltp_tags.XMLTP_TAG_NUM,
				type("s") : xmltp_tags.XMLTP_TAG_STR,
			   }

def getXMLTPvalueTagForPythonObject(obj):
	try:
		return (dictPythonTypeToXMLTPtag[type(obj)] )
	except:
		return (xmltp_tags.XMLTP_TAG_STR)


#
# >>>>> HIGH-LEVEL functions:
#

#
# sendProcCall(rpcFrame) should be here with the other high-level functions, but it is further below!
#

# Functions to send responses :
#
def sendResultSetAndReturnStatusToClient(rpcFrame, colList, rowsList, returnStatus):
	#
	# Called by:	gxregprocs.py
	#
	# Send a result set and a return status to the clientConn.
	#
	resSetsLs = [ (colList, rowsList), ]	# make a result sets list with one result set.

	rc = sendResponseToClient(rpcFrame, msgList=None, returnStatus=returnStatus, resultSetsList=resSetsLs, outParamsList=None, callingFunctName="sendResultSetAndReturnStatusToClient()")
	return rc

def replyErrorToClient(rpcFrame, msgList, returnStatus, b_sendXMLTPreset=0):
	#
	# Sends an (error) response to rpcFrame.getClientConn()
	#

	# pass None for outParamsList param:
	#
	return replyToClientExt(rpcFrame, msgList, returnStatus, outParamsList=None, b_sendXMLTPreset=b_sendXMLTPreset)


def replyToClientExt(rpcFrame, msgList, returnStatus, outParamsList=None, b_sendXMLTPreset=0):
	#
	# Sends a response to rpcFrame.getClientConn()
	#
	rc = sendResponseToClient(rpcFrame, msgList, returnStatus, resultSetsList=None, outParamsList=outParamsList, callingFunctName="replyToClientExt", b_sendXMLTPreset=b_sendXMLTPreset)
	return rc


def sendResponseToClient(rpcFrame, msgList, returnStatus, resultSetsList=None, outParamsList=None, callingFunctName=None, b_sendXMLTPreset=0):
	#
	# sendResponseToClient() is the most general and complete of the high-level functions which send a reponse
	#	to a clientConn.
	#
	# Called by:	All other high-level functions which reply to a client (in this module)
	#		Other modules.
	#
	# NOTES: msgList, resultSetsList and outParamsList could be None.
	#	 These sections of a XMLTP response are optional.
	#
	#	 If (b_sendXMLTPreset) then we send a special <reset/> tag first.
	#
	if (not callingFunctName):
		callingFunctName = "sendResponseToClient()"

	try:
		rpcFrame.resetExceptionInfo()

		cltConn = rpcFrame.getClientConn()

		if (cltConn == None or cltConn.isDisconnected() ):
			rpcFrame.assignReturnStatusAndFinalDiagnosis(gxreturnstatus.DS_FAIL_CANNOT_RETRY,
							gxrpcframe.CLIENT_DISCONNECTED)
			return (1)

		if (rpcFrame.isReplyDone() ):
			m1 = "reply already done! Cannot reply: ret=%s, msg=%s, to cltConn=%s." % (returnStatus, msgList, cltConn)
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
			return (0)

		sock = cltConn.getSocket()

		sockBuff = SocketBuffer(sock)

		# sendResponseOnSocket() will flush the sockBuff...
		#
		rc = sendResponseOnSocket(sockBuff, msgList, returnStatus, resultSetsList, outParamsList, b_flushSocket=1, b_sendXMLTPreset=b_sendXMLTPreset)

		rpcFrame.assignStep(gxrpcframe.STEP_REPLYING_DONE)

		return (rc)
	except:
		ex = gxutils.formatExceptionInfoAsString(10)		
		m1 = "sendResponseToClient() FAILED with Exception while sending reply. ret=%s, msg=%s, cltConn=%s, Exc=%s." % (returnStatus, msgList, cltConn, ex)
		srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)

		rpcFrame.rememberException(callingFunctName, gxrpcframe.TO_CLIENT)

		rpcFrame.assignStep(gxrpcframe.STEP_REPLYING_FAILED)
		return (-1)

	return (0)



#
# >>>>> LOW-LEVEL functions (send to socket):
#

#
# Sub-functions used to send a response:
#
def sendResponseOnSocket(sock, msgList, returnStatus, resultSetsList=None, outParamsList=None, b_flushSocket=1, b_sendXMLTPreset=0):
	#
	# Sends a response on socket sock. Also... if (b_flushSocket): sock.flush()
	#
	# NOTE: if b_flushSocket is false, then sock could be a normal socket.
	#	if b_flushSocket is true, then sock must be a SocketBuffer, as this
	#	function will call its .flush() method.
	#
	# WARNING: This function is ALWAYS called to reply to the client. If it fails,
	#	   then the client will NEVER get any response back.
	#	   You most probably DO NOT need to touch this function.
	#	   Normally, this function will NOT need to be modified unless the
	#	   syntax of XMLTP changes. (jft,2003jan28)
	#
	# The response will be sent like this:
	#
	# <?xml version="1.0" encoding="ISO-8859-1" ?>
	# <response>
	# 	  [ result set(s)... ]
	#   <outParams>
	#	  <param> <name>@o_newseq_no</name> <int>10205039</int> <attr>002</attr> </param>
	#   </outParams>
	#   <msgList>
	#     <msg><int>25003</int> <str>This is message 25003</str> </msg>
	#   </msgList>
	#   <returnStatus>
	#     <int>0</int>
	#  </returnStatus>
	# </response>
	# <EOT/>
	#
	# NOTES: The msgList section is optional.
	#	 The outParams section is optional.
	#	 The resultSetsList section is optional.
	#	 If (b_sendXMLTPreset) then we send a special <reset/> tag first.
	#
	if (b_sendXMLTPreset):
		#
		# this tag tells the parser of the client to reset itself.
		# The client must flush its previous values and input, 
		# and, start parsing again.
		#
		writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_RESET)
	#
	# Begin the XML-TP response stream:
	#
	sendXMLTPprefix(sock)

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_RESPONSE)

	if (resultSetsList):
		if (getDebugLevel() >= 10):
			m1 = "sendResponseOnSocket(): %s result set(s) to send on sockBuff: %s." % (len(resultSetsList), sock, )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

		sendBeginResultsSetsList(sock)

		for resultSet in resultSetsList:
			colList, rowsList = resultSet
			sendOneResultSet(sock, colList, rowsList)

		sendEndResultsSetsList(sock)

		if (getDebugLevel() >= 10):
			m1 = "sendResponseOnSocket(): %s result set(s) buffered..." % (len(resultSetsList),  )
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	if (outParamsList):
		sendoutParamsList(outParamsList, sock)

	if (msgList):
		sendMsgList(msgList, sock)

	sendReturnStatus(sock, returnStatus)

	#
	# End the XML-TP response stream:
	#
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_RESPONSE, 1)

	sendXmltpEOT(sock)

	if (not b_flushSocket):	# do NOT flush() the SocketBuffer, return now.
		if (getDebugLevel() >= 10):
			m1 = "sendResponseOnSocket(): b_flushSocket == False"
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)
		return 0

	if (getDebugLevel() >= 10):
		m1 = "sendResponseOnSocket(): about to sockBuff.flush()... sockBuff: %s." % (sock, )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	rc = sock.flush()	# do a single .send() to send the whole response in XMLTP

	if (getDebugLevel() >= 10):
		m1 = "sendResponseOnSocket(): sockBuff.flush() Done. rc=%s. sockBuff: %s." % (rc, sock, )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	return (rc) 


#
# >>>> VERY LOW-LEVEL functions (INTERNAL, PRIVATE):
#
def sendBeginResultsSetsList(sock):
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_RESULTSETSLIST)

def sendEndResultsSetsList(sock):
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_RESULTSETSLIST, 1)

def sendOneResultSet(sock, colList, rowsList):
	sendBeginOneResultSet(sock)

	colsDatatypesList = sendColNames(sock, colList)

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_ROWS)

	nbRows = 0
	for row in rowsList:
		sendOneRow(sock, row, colsDatatypesList)
		nbRows = nbRows + 1

	sendEndOneResultSet(sock, nbRows)


def sendColNames(sock, colList):
	if (getDebugLevel() >= 10):
		indent = "  "
		nl = "\n"
	else:
		indent = ""
		nl = ""

	colsDatatypesList = []

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_COLNAMES)

	for col in colList:
		if (len(col) == 3):
			name, typ, sz = col
			szStr = "<%s>%s</%s>" % (xmltp_tags.XMLTP_TAG_SZ, sz, xmltp_tags.XMLTP_TAG_SZ)
		else:
			szStr = ""
			name, typ = col

		colsDatatypesList.append(typ)

		s = "%s<%s><%s>%s</%s><%s>%s</%s>%s</%s>%s" % (indent, 
					xmltp_tags.XMLTP_TAG_COL,
					xmltp_tags.XMLTP_TAG_NAME,
					name,
					xmltp_tags.XMLTP_TAG_NAME,
					xmltp_tags.XMLTP_TAG_INT,
					typ,
					xmltp_tags.XMLTP_TAG_INT,
					szStr,
					xmltp_tags.XMLTP_TAG_COL,
					nl)
		writeToSocket(sock, s)

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_COLNAMES, 1)

	return colsDatatypesList


def escapeForXML(val):
	#
	# NOTE: If you update this function, you must also update gxrpcmonitors.unEscapeXML()
	#
	val = val.replace("&", xmltp_tags.META_CHAR_AMP)	# AMP must be done first

	val = val.replace("'", xmltp_tags.META_CHAR_APOS)
	val = val.replace('"', xmltp_tags.META_CHAR_QUOT)

	val = val.replace("<", xmltp_tags.META_CHAR_LT)
	return val.replace(">", xmltp_tags.META_CHAR_GT)



def prepValueForXML(val):
	typ = type(val)

	if (typ == type("s") ):
		if (val.find("&") >= 0 or val.find("<") >= 0 or val.find(">") >= 0 ):
			return escapeForXML(val)
		else:
			return val

	return (repr(val) )



def sendOneRow(sock, row, colsDatatypesList):
	if (getDebugLevel() >= 10):
		indent = "   "
		nl = "\n"
	else:
		indent = ""
		nl = ""

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_ROW)

	# In some cases, the "row" is not a sequence of values, but, an object.
	#
	# In these case, the method row.asTuple() will convert the object in a 
	# proper sequence of values.
	#
	# NOTE: care should be taken to have the same number of column names 
	#	and of values in a row. Also, the datatype should be conistent.
	#
	if (type(row) != type( () ) and type(row) != type( [] ) ):
		row = row.asTuple()

	i = 0
	for valTuple in row:
		if (type(valTuple) == type( () ) ):
			val, isNull = valTuple
		else:
			val = valTuple
			if (val == None):
				isNull = 1
			else:
				isNull = 0

		## valTag = getXMLTPvalueTagForPythonObject(val)

		valTag = xmltp_tags.getValueTagForType( colsDatatypesList[i] )

		s = "%s<%s>%s</%s><%s>%s</%s>%s" % (indent, 
					valTag,
					prepValueForXML(val),
					valTag,
					xmltp_tags.XMLTP_TAG_ISNULL,
					isNull,
					xmltp_tags.XMLTP_TAG_ISNULL,
					nl)
		writeToSocket(sock, s)

		i = i + 1	# iterate in colsDatatypesList

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_ROW, 1)



def sendBeginOneResultSet(sock):
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_RESULTSET)

def sendEndOneResultSet(sock, nbRows):
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_ROWS, 1)

	s = " <%s><%s>%s</%s></%s>" %  (xmltp_tags.XMLTP_TAG_NBROWS,
					xmltp_tags.XMLTP_TAG_INT,
					nbRows,
					xmltp_tags.XMLTP_TAG_INT,
					xmltp_tags.XMLTP_TAG_NBROWS)
	writeToSocket(sock, s)
					
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_RESULTSET, 1)


def sendoutParamsList(outParamsList, sock):
	#
	# Called by:	replyToClientExt()
	#
	# Send:
	#   <outParams>
	#	  <param> <name>@o_newseq_no</name> <int>10205039</int> <attr>002</attr> </param>
	#   </outParams>
	#
	if (not outParamsList):
		return None

	if (getDebugLevel() >= 10):
		indent = "  "
	else:
		indent = ""

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_OUTPARAMS)

	sendParams(sock, outParamsList)		# same syntax as procCall params list

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_OUTPARAMS, 1)



def sendMsgList(msgList, sock):
	# send:
	#   <msgList>
	#     <msg><int>25003</int> <str>This is message 25003</str> </msg>
	#     ...
	#   </msgList>
	#
	if (msgList == None):
		return None
	if (getDebugLevel() >= 10):
		indent = "  "
	else:
		indent = ""

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_MSGLIST)

	for msg in msgList:
		msgNo, msgStr = msg

		s = "%s<%s><%s>%s</%s><%s>%s</%s></%s>" % (indent, 
					xmltp_tags.XMLTP_TAG_MSG,
					xmltp_tags.XMLTP_TAG_INT,
					msgNo,
					xmltp_tags.XMLTP_TAG_INT,
					xmltp_tags.XMLTP_TAG_STR,
					escapeForXML(msgStr),
					xmltp_tags.XMLTP_TAG_STR,
					xmltp_tags.XMLTP_TAG_MSG)
		writeToSocket(sock, s)

	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_MSGLIST, 1)


def sendReturnStatus(sock, returnStatus):
	#
	#   <returnStatus>
	#     <int>0</int>
	#  </returnStatus>
	#
	if (getDebugLevel() >= 10):
		indent = "  "
	else:
		indent = ""
	s = "%s<%s><%s>%s</%s></%s>" % (indent, 
					xmltp_tags.XMLTP_TAG_RETURNSTATUS,
					xmltp_tags.XMLTP_TAG_INT,
					returnStatus,
					xmltp_tags.XMLTP_TAG_INT,
					xmltp_tags.XMLTP_TAG_RETURNSTATUS)
	writeToSocket(sock, s)

#
# >>>>> HIGH-LEVEL function: to send a procedure call...
#

#
# Functions used to send a procCall:
#
def sendProcCall(rpcFrame):
	#
	# Sends the rpcFrame.getProcCall() in XMLTP to the rpcFrame.getDestConn()
	#
	rpcFrame.assignStep(gxrpcframe.STEP_SENDING_RPC_CALL)
	rc = validateProcCall(rpcFrame)
	if (rc != 0):
		rpcFrame.assignReturnStatusAndFinalDiagnosis(gxreturnstatus.PARSE_ERROR_CANNOT_GET_PARAMS,
						gxrpcframe.PROC_CALL_NOT_VALID)
		return (rc)

	try:
		rpcFrame.resetExceptionInfo()

		destConn = rpcFrame.getDestConn()

		if (None == destConn and getDebugLevel() >= 10):
			sock = None	# allows unit testing without a destConn
		else:
			sock = destConn.getSocket()

		sockBuff = SocketBuffer(sock)

		procCall = rpcFrame.getProcCall()

		rc = sendProcCallWithRetry(rpcFrame, procCall, destConn, sockBuff)
		if (rc != 0):
			return (rc)

		# if (rc != 0):
		#	return (rc)
	except:
		rpcFrame.rememberException("sendProcCall() failed", gxrpcframe.TO_DESTINATION)

		return (-1)

	rpcFrame.assignStep(gxrpcframe.STEP_RPC_CALL_SENT)
	return (0)


def sendProcCallWithRetry(rpcFrame, procCall, destConn, sock):
	#
	# try to send the procCall.
	# If there is a disconnect error, try to reconnect
	# and send one more time.
	#
	nb_try = 0
	while (nb_try <= 1):
		try:
			rpcFrame.resetExceptionInfo()	# need to reset here again

			sendXMLTPprefix(sock)

			sendProcName(sock, procCall.getProcName() )

			sendParams(sock, procCall.getParamsList() )

			writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_PROCCALL, 1)
			sendXmltpEOT(sock)

			if (getDebugLevel() >= 10):
				m1 = "about to sockBuff.flush()... sockBuff: %s." % (sock, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			rc = sock.flush()	# do a single .send() to send the whole procCall in XMLTP

			if (getDebugLevel() >= 10):
				m1 = "sockBuff.flush() Done. rc=%s. sockBuff: %s." % (rc, sock, )
				srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

			return (0)		# send OK
		except:
			rpcFrame.rememberException("sendProcCallWithRetry() failed", gxrpcframe.TO_DESTINATION)

			if (nb_try >= 1):
				return (-1)	# failed on retry

			nb_try = nb_try + 1

			if (rpcFrame.getExceptionDiag() != gxutils.EXC_DISCONNECT):
					destConn.assignBadState(1)
			else:
				try:
					rpcFrame.rememberErrorMsg("trying:", "destConn.verifyDestConn(1)")
					rc = destConn.verifyDestConn(1)	# reset the destConn
					if (rc == 0):
						sock = SocketBuffer(destConn.getSocket() )
					else:
						rpcFrame.rememberErrorMsg("destConn.verifyDestConn(1)", "FAILED rc!=0")
						rpcFrame.assignReturnStatusAndFinalDiagnosis(gxreturnstatus.DS_NOT_AVAIL,
							gxrpcframe.DS_SERVER_NOT_AVAILABLE)
						return (-8)
				except:
					rpcFrame.rememberErrorMsg("destConn.verifyDestConn(1)", "FAILED (exc)")
					rpcFrame.resetExceptionInfo()	# reset to be able to remember reconnect error

					rpcFrame.rememberException("sendProcCallWithRetry(): reconnect failed", gxrpcframe.TO_DESTINATION)

					if (rpcFrame.getExceptionDiag() == gxutils.EXC_DS_SERVER_NOT_AVAIL):
						# remember final diagnosis:
						# ------------------------
						rpcFrame.assignReturnStatusAndFinalDiagnosis(gxreturnstatus.DS_NOT_AVAIL,
							gxrpcframe.DS_SERVER_NOT_AVAILABLE)
							
					return (-9)
	#
	# should never come here.
	#
	return (-7)


def validateProcCall(rpcFrame):
	procCall = rpcFrame.getProcCall()

	if (None == procCall):
		rpcFrame.rememberErrorMsg("validateProcCall() failed", 
					  "getProcCall() returned None")
		return -6

	errMsg = procCall.checkValidity()
	if (errMsg != None):
		rpcFrame.rememberErrorMsg("validateProcCall() failed", errMsg)
		return -5

	procName   = procCall.getProcName()
	paramsList = procCall.getParamsList()

	if (type(procName) != type("str") ):
		rpcFrame.rememberErrorMsg("validateProcCall() failed", 
		  "procName must be a String, not a %s" % (type(procName), ) ) 
		return -4

	if (type(paramsList) != type( [] ) ):
		rpcFrame.rememberErrorMsg("validateProcCall() failed", 
		  "paramsList must be a List, not a %s" % (type(paramsList), ) ) 
		return -3
	return (0)


#
# >>>> VERY LOW-LEVEL functions (INTERNAL, PRIVATE):
#
def writeToSocket(sock, str):
	#
	# NOTE: exceptions are handled in sendProcCall(rpcFrame)
	#
	if (getDebugLevel() >= 10):
		if (None == sock):
			print str,
			return (None)
		elif (getDebugLevel() >= 20):
			print str,
	sock.send(str)

def writeTagToSocket(sock, tag, b_endTag=0):
	#
	# enclose the tag xxx with bracket "<xxx>" and write it.
	# if (b_endTag), write "</xxx>".
	#
	if (1 & b_endTag):
		str = "</%s>" % (tag, )
	else:
		str = "<%s>" % (tag, )
	if (2 & b_endTag):
		str = str + "\n"
	writeToSocket(sock, str)
	

def sendXMLTPprefix(sock):
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_XMLVERSION_AND_ENCODING)

def sendProcName(sock, procName):
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_PROCCALL)
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_PROC)
	writeToSocket(sock, procName)
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_PROC, 1)

def sendParams(sock, paramsList):
	if (getDebugLevel() == 155):
		m1 = "DEBUG sendparams(), paramsList=%s." % (paramsList, )
		srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, m1)

	if (getDebugLevel() >= 10):
		indent = "  "
	else:
		indent = ""

	for param in paramsList:
		if (len(param) == 6):
			name, value, jType, isNull, isOutput, sz = param
		else:
			sz = None
			name, value, jType, isNull, isOutput = param

		valueTag = xmltp_tags.getValueTagForType(jType)

		value = prepValueForXML(value)

		valStr  = "%s<%s>%s</%s>" % (indent, valueTag, value, valueTag)

		if (isNull):
			indNull = "1"
		else:
			indNull = "0"
		if (isOutput):
			indOut = "1"
		else:
			indOut = "0"
		attrStr = "%s<%s>%c%c%02d</%s>" % (indent, 
						xmltp_tags.XMLTP_TAG_ATTR,
						indNull, indOut, jType, 
						xmltp_tags.XMLTP_TAG_ATTR)

		if (sz != None):
			szStr = "<%s>%s</%s>" % (xmltp_tags.XMLTP_TAG_SZ, sz, xmltp_tags.XMLTP_TAG_SZ)

		writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_PARAM)
		writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_NAME)
		writeToSocket(sock, name)
		writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_NAME, 1)
		writeToSocket(sock, valStr)
		writeToSocket(sock, attrStr)
		if (sz != None):
			writeToSocket(sock, szStr)
		writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_PARAM, 1)

def sendXmltpEOT(sock):
	writeTagToSocket(sock, xmltp_tags.XMLTP_TAG_EOT)


if __name__ == '__main__':
	import gxclientconn

	assignDebugLevel(99)	# we want to see the output on stdout here
	print "debug level:", getDebugLevel()

	conn = gxclientconn.gxClientConn(None)	# socket == None! for testing

	conn.assignState(gxclientconn.STATE_LOGIN_IN_PROGRESS, "dummyUserid")

	f1 = gxrpcframe.gxRPCframe(conn)

	conn.assignUserid("USER_006")
	conn.assignRPCframe(f1)

	print gxclientconn.getListOfClients()

	f1.assignProccall(  ( 'procNameXX',
				[ ('__return_status__', '-9999', 4, 1, 1), 
		  		  ('@p_dbName', 'Master', 12, 0, 0),
		  		  ('@p_Other', 'Other', 1111, 0, 1),
		  		  ('@p_tinyInt', 127,    -6, 0, 1),
		  		  ('@p_Null',  'NONE',    4, 1, 0),
				   ], 
				0 )  )

	for clt in gxclientconn.getListOfClients():
		print clt

	
	rc = sendProcCall(f1)

	if (rc != 0):
		print "error, rc:", rc, "msg:", f1.getErrMsg(), f1.getEventsList(1)
	else:
		print "success", f1, f1.getEventsList(1)

	print "TESTING replyErrorToClient()..."

	rc = replyErrorToClient(f1, [ (123, "msg un deux trois"),
				 (-1100, "serveur non-disponible"), ],
			   -1100)

	if (rc != 0):
		print "error, rc:", rc, "msg:", f1.getErrMsg(), f1.getEventsList(1)
	else:
		print "success", f1, f1.getEventsList(1)

	rc = replyErrorToClient(f1, None, -1105,  1)

	if (rc != 0):
		print "error, rc:", rc, "msg:", f1.getErrMsg(), f1.getEventsList(1)
	else:
		print "success", f1, f1.getEventsList(1)

	rc = replyErrorToClient(f1, [], -1100)

	if (rc != 0):
		print "error, rc:", rc, "msg:", f1.getErrMsg(), f1.getEventsList(1)
	else:
		print "success", f1, f1.getEventsList(1)


	sys.exit() #### enough ####


	f1.assignProccall(  ( ('procNameXX__ERROR', 'aTupleNotString'),
				[ ('__return_status__', '-9999', 4, 1, 1), 
		  		  ('@p_dbName', 'Master', 12, 0, 0),
		  		  ('@p_Other', 'Other', 1111, 0, 1),
		  		  ('@p_tinyInt', 127,    -6, 0, 1),
		  		  ('@p_Null',  'NONE',    4, 1, 0),
				   ], 
				0 )  )

	rc = sendProcCall(f1)

	if (rc != 0):
		print "error, rc:", rc, "msg:", f1.getErrMsg()

	if (f1.getMsgTime() == None):
		print "SUCCESS -- no error"
	else:
		print "f1.getErrMsg():", f1.getErrMsg()
		print f1

