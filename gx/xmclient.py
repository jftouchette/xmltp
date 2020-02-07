#!/usr/bin/python

#
# xmclient.py :  XMltp Client object (for re-use)
# ----------------------------------
#
# $Source: /ext_hd/cvs/gx/xmclient.py,v $
# $Header: /ext_hd/cvs/gx/xmclient.py,v 1.4 2006-03-31 17:21:39 toucheje Exp $
#
#  Copyright (c) 2003 Jean-Francois Touchette
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
# TECHNICAL NOTES:
# ---------------
# The RPC which is to be sent using a xmlclient object must be in the usual 
# Python representation for a XMLTP proc call.
#
# Example:
#
#	('trace', [('__return_status__', '-9999',   4, 1, 1), 
#		   ('@p_dbName', 	 'Master', 12, 0, 0)], 0)
#
#
# This xmlclient object uses the function gxxmltpwriter.sendProcCall(rpcFrame) to send
# the RPC all.
# And, it uses xmltp_gx.so to receive the results.
#
# -----------------------------------------------------------------------
# 2003jul23-aug06,jft: + first version -- based on xrpc.py
# 2003aug06,jft: with initial loginProcCall
# 2003aug10,jft: . convertResultSetToTextLines(): if a colName in colFilters is associated with a values-decoding dict, use it
# 2004aug24,jft: xmclient.analyzeResponseReturnResultSet(): if nothing to show, but b_expectReturnStatus then return ret
# 2006mar31,jft: convertResultSetToTextLines(): added optional arg rowsToExclude
#

import time
import sys
import socket
import struct		# for pack()

import gxintercept	# convertToPythonTypeIfNotString()
import gxrpcmonitors	# unEscapeXML(val)
import gxrpcframe
import gxclientconn	# to please gxrpcframe
import gxdestconn	# DestConn required by gxxmltpwriter
import gxxmltpwriter	# for .sendProcCall(rpcFrame)
import gxutils
import gxreturnstatus
import xmltp_hosts	# for xmltp_hosts.getServerHostAndPort()
import xmltp_tags	# for JAVA_SQL_INTEGER, JAVA_SQL_VARCHAR, JAVA_SQL_DOUBLE

try:
	import xmltp_gx
except ImportError:
	print "'import xmltp_gx' failed. Module is not in the current directory nor installed."
	print "WARNING: subsequent calls to xmltp_gx.parseResponse() will fail."
	print
	xmltp_gx = None

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/xmclient.py,v 1.4 2006-03-31 17:21:39 toucheje Exp $"

MODULE_NAME = "xmclient.py"


#
# Global which must be assigned by the calling modules:
#
srvlog = None
s_debug_level = 0	# value will be assigned by main module

def debugLog(level, n, msg, requiredDebugLevel=10):
	if (s_debug_level >= requiredDebugLevel):
		srvlog.writeLog(MODULE_NAME, level, n, msg)

#
# global used for debugging:
#
g_debug_resp = "[nothing yet]"

#
# Constants defined by this module:
#
ERR_DISCONNECT		= 1
ERR_CONNECT_FAILED	= 2
ERR_LOGIN_FAILED	= 5
RESP_ERR_DISCONNECT	= -9999		# special value for a response





#
# Constants and dict used by convertResultSetToTextLines :
#
FMT_TIMESTAMP_STR = "%23.23s"
FMT_INT_STR	  = "%6d"
FMT_NUM_STR	  = "%12.2f"
FMT_STR_STR	  = "%-*.*s"
FMT_UNKNOWN_STR	  = "'%s'"

dictTypeToFormatString = {
	xmltp_tags.JAVA_SQL_DATE		:  (FMT_TIMESTAMP_STR, 23),
	xmltp_tags.JAVA_SQL_TIME		:  (FMT_TIMESTAMP_STR, 23),
	xmltp_tags.JAVA_SQL_TIMESTAMP		:  (FMT_TIMESTAMP_STR, 23),
	xmltp_tags.JAVA_SQL_TINYINT		:  (FMT_INT_STR,  7),
	xmltp_tags.JAVA_SQL_SMALLINT		:  (FMT_INT_STR,  7),
	xmltp_tags.JAVA_SQL_BIGINT		:  (FMT_INT_STR,  7),
	xmltp_tags.JAVA_SQL_BIT			:  (FMT_INT_STR,  7),
	xmltp_tags.JAVA_SQL_INTEGER		:  (FMT_INT_STR,  7),
	xmltp_tags.JAVA_SQL_NUMERIC		:  (FMT_NUM_STR,  12),
	xmltp_tags.JAVA_SQL_REAL		:  (FMT_NUM_STR,  12),
	xmltp_tags.JAVA_SQL_DECIMAL		:  (FMT_NUM_STR,  12),
	xmltp_tags.JAVA_SQL_DOUBLE		:  (FMT_NUM_STR,  12),
	xmltp_tags.JAVA_SQL_FLOAT		:  (FMT_NUM_STR,  12),
	xmltp_tags.JAVA_SQL_CHAR		:  (FMT_STR_STR,  None),
	xmltp_tags.JAVA_SQL_VARCHAR		:  (FMT_STR_STR,  None),
	xmltp_tags.JAVA_SQL_LONGVARCHAR		:  (FMT_STR_STR,  None),
	xmltp_tags.JAVA_SQL_BINARY		:  (FMT_UNKNOWN_STR, 5),
	xmltp_tags.JAVA_SQL_VARBINARY		:  (FMT_UNKNOWN_STR, 5),
	xmltp_tags.JAVA_SQL_LONGVARBINARY	:  (FMT_UNKNOWN_STR, 5),
	xmltp_tags.JAVA_SQL_OTHER		:  (FMT_UNKNOWN_STR, 5),
	xmltp_tags.JAVA_SQL_NULL		:  (FMT_UNKNOWN_STR, 5),
 }


def convertResultSetToTextLines(res, b_withColnames, srvName=None, colFilters=None, rowsToExclude=None):
	#
	# take a result set, return a list of lines (text)
	#
	colNames, rows, nbRows = res

	vLines = []
	colFormatList = []
	hdrStrList = []
	##underlineStrList = []

	if srvName:
		hdrStrList = ["serverName  ", ]
		##underlineStrList = ["--------",]

	iColsToKeep = {}	# reused when processing rows...

	iCols = 0
	for col in colNames:
		if (len(col) == 3):
			name, typ, sz = col
		else:
			sz = None
			name, typ = col

		if (colFilters and not colFilters.has_key(name) ):
			iCols = iCols + 1
			continue

		if colFilters:
			x = colFilters[name]
		else:
			x = None
		if x and type(x) == type({}):		# is this a value-decoding dict ?
			iColsToKeep[iCols] = x		# yes, remember value-decoding dict
		else:
			iColsToKeep[iCols] = None
		iCols = iCols + 1

		fmt, width = dictTypeToFormatString.get(typ, (FMT_UNKNOWN_STR, 5) )

		if (width == None):		# string value (VARCHAR, CHAR), has a "sz"
			if (sz == None):
				sz = 3			# arbitrary value

			width = sz		# use this value to prepare columns headings
		else:
			sz = None

		if (len(name) > width):
			width = len(name)

		valToExclude = None
		if rowsToExclude:
			valToExclude = rowsToExclude.get(name, None)

		fmtTuple = (fmt, typ, width, sz, (iCols - 1), valToExclude )

		colFormatList.append(fmtTuple)	# colFormatList is reused later

		if b_withColnames:
			hdr = "%*.*s" % (width, width, name)
			hdrStrList.append(hdr)

		## underStr = "-" * width
		## underlineStrList.append(underStr)

	if b_withColnames:
		vLines.append( " ".join(hdrStrList) )

	### " ".join(underlineStrList)

	ctr = 0
	for row in rows:
		if srvName:
			x = "%12.12s" % (srvName, )
			valuesList = [x, ]
		else:
			valuesList = []

		for colFmt in colFormatList:
			fmt, typ, width, sz, iCols, valToExclude = colFmt

			if iColsToKeep and not iColsToKeep.has_key(iCols):
				continue

			val, isNull = row[iCols]

			if valToExclude and val[:len(valToExclude)] == valToExclude:	# then must skip this row entirely
				valuesList = None
				break

			if (isNull):
				valFmt = "NULL"
			else:
				# NOTE: convertToPythonTypeIfNotString() returns None is val does not
				#	require to be converted.
				#
				try:
					newValue = gxintercept.convertToPythonTypeIfNotString(val, typ, isNull)

					if (newValue == None):	# if None, then val is a string...
						newValue = gxrpcmonitors.unEscapeXML(val)

					if iColsToKeep and iColsToKeep.has_key(iCols):
						decodeValuesDict = iColsToKeep[iCols]
						if decodeValuesDict:
							valFmt = decodeValuesDict.get(newValue, None)
						else:
							valFmt = None
					else:
						valFmt = None						

					if valFmt == None:
						if (sz == None):
							valFmt = fmt % (newValue, )
						else:
							valFmt = fmt % (sz, sz, newValue, )
				except:
					ex = gxutils.formatExceptionInfoAsString(5)
					debugLog( 0, 0,  ex, requiredDebugLevel=0)
					valFmt = "'%s'" % (val, )

			# now, make all values follow the fixed width of that column:
			#
			valStr = "%*.*s" % (width, width, valFmt)

			valuesList.append(valStr)

		if valuesList != None:		# if it is None, then the row must be skipped (excluded)
			vLines.append( " ".join(valuesList) )

	return vLines


#
# Emulation classes used to allow the use of gx modules:
#
class DummyGxServer:
	def writeLog(self, moduleName, logLevel, msgNo, msg):
		srvlog.writeLog(MODULE_NAME, logLevel, msgNo, msg)


dummyGxServer = DummyGxServer()

gxclientconn.gxServer = dummyGxServer

# class xmProcCall can be use to build a ProcCall from a list like:
#	[ "anyProcName", "aStringValue", 123, 4.56, ]
#
class xmProcCall:
	def __init__(self):
		self.errmsg = "[no errmsg yet]"
		self.procCall  = None
		self.procName  = None
		self.paramList = None
		self.paramNameCtr = 0

	def buildProcCallFromList(self, ls):
		if not ls:
			self.errmsg = "Error: list is empty or None"
			return self.errmsg

		self.procName = ls[0]
		if (not self.procName[0].isalpha() ):
			self.errmsg = "procName must begin with alpha character"
			return ("Error: " + self.errmsg)
		self.paramList = []
		self.procCall = (self.procName, self.paramList, 0)
		self.paramNameCtr = 1
		for x in ls[1:] :
			self.appendParamValue(x)

		return self.procCall

	def appendParamValue(self, token):
		valType = xmltp_tags.JAVA_SQL_VARCHAR
		if (token[0].isdigit() ):
			try:
				val = int(token)

				valType = xmltp_tags.JAVA_SQL_INTEGER
			except ValueError:
				try:
					val = float(token)

					valType = xmltp_tags.JAVA_SQL_DOUBLE
				except ValueError:
					self.errmsg = "this is not a good number: " + token
					raise RuntimeError, ("Error: " + self.errmsg)

		if (token[0] == '"' or token[0] == "'"):
			if (len(token) < 3):
				val = ""
			else:
				val = token[1:len(token) - 1]
		else:
			val = token

		paramName = "@p_%03d" % (self.paramNameCtr, )
		self.paramNameCtr = self.paramNameCtr + 1

		tup = (paramName, str(val), valType, 0, 0)	# isOutput = 0

		self.paramList.append(tup)

		return (0)



#
# the class xmclient can send the RPC calls using the XMLTP protocol
#
class xmclient:
	def __init__(self, userid, passwd, srvIp=None, srvPort=None, debugLevel=0, rawXml=0):
		self.userid = userid
		self.passwd = passwd
		if (srvIp):
			self.srvIp = srvIp
		else:
			self.srvIp = "127.0.0.1"

		self.srvName = srvPort

		if srvPort.isdigit():
			self.srvPort = int(srvPort)
		else:
			x = xmltp_hosts.getServerHostAndPort(srvPort)
			if (not x or type(x) == type("s") ):
				m1 = "Cannot find host,port for '%s'. Errmsg: %s." % (srvPort, x)
				print m1
				self.srvPort = None
			else:
				h, p = x
				self.srvPort = p
				self.srvIp   = h

		self.b_capture_results = 1	# now, 1 by default, 2002sept16,jft

		if (debugLevel == None):
			debugLevel = 0
		self.debugLevel = debugLevel

		self.ctx = None

		if (xmltp_gx != None):
			self.ctx = xmltp_gx.createContext("ctx_for_xmclient.py")
			#
			# set the xmltp_gx module to display the XMLTP response on stdout:
			#  3 = (1 | 2)   --- response on stdout (1) | debug proccall (2)
			#
			if (debugLevel >= 5):
				xmltp_gx.setDebugMask(3)
			if (rawXml == 1001):	# will send raw XML code to stderr
				xmltp_gx.setDebugMask(rawXml)

		self.rcvTimeout = 210
		self.ctrErrors  = 0
		self.err_state  = 0		# no error yet
		self.loginProcCall = None
		self.clt_conn   = None
		self.sock       = None
		self.destConn   = None
		self.procCall   = None
		self.ctr_reconnect = 0
		self.errMsg2 = None

	def resetConn(self, errCause):
		m1 = "resetConn(), errCause=%s" % (errCause, )
		debugLog( 0, 0,  m1)

		self.err_state = errCause
		try:
			if self.sock:
				self.sock.close()
			self.sock = None
			self.destConn = None
		except:
			pass


	def isConnected(self):
		if not self.sock:
			return 0		# not connect
		return 1
	
	def canTryReconnect(self):
		if self.err_state == ERR_LOGIN_FAILED:
			return 0
		return 1

	def getErrState(self):
		return self.err_state

	def prepareConnection(self):
		#
		# 0. if connected, do nothing, return
		# 1. build login procCall (if not done yet)
		# 2. connect()
		# 3. send login RPC.. check that it is successful
		#
		debugLog( 0, (self.sock != None),  "prepareConnection()" )
		if self.sock:
			return None		# already connected, OK

		if self.err_state == ERR_LOGIN_FAILED:
			m1 = "%s: ERR_LOGIN_FAILED before. No re-connect." % (self.srvName, )
			return m1


		try:
			self.ctr_reconnect = self.ctr_reconnect + 1

			if not self.loginProcCall:
				x = xmProcCall()
				p = x.buildProcCallFromList( ["login", self.userid, self.passwd, MODULE_NAME] )
				if type(p) == type("s"):
					return p
				self.loginProcCall = p

			debugLog( 0, 0,  "prepareConnection(): about to connectToServer()" )

			self.connectToServer()

			debugLog( 0, (self.sock != None),  "prepareConnection(): connect done, BEFORE sendRPC() login..." )

			ret = self.sendRPCandReceiveResults(self.loginProcCall, b_expectReturnStatus=1)
			if ret != 0:
				self.resetConn(ERR_LOGIN_FAILED)
				return ret

			self.err_state = 0
		except:
			exTuple = gxutils.formatExceptionInfo(1)
			diag = gxutils.getDiagnosisFromExcInfo(exTuple)
			if (diag == gxutils.EXC_DS_SERVER_NOT_AVAIL):
				self.resetConn(ERR_CONNECT_FAILED)
				m1 = "%s: SERVER_NOT_AVAILABLE (ctr=%s)" % (self.srvName, self.ctr_reconnect)
				return m1

			ex = gxutils.formatExceptionInfoAsString(1)
			m1 = "connectToServer() %s failed, exc: %s" % (self.srvName, ex)
			self.resetConn(m1)
			m1 = "connectToServer() %s failed, diag=%s, exc: %s" % (self.srvName, diag, ex)
			return m1


	def connectToServer(self):
		self.sock = self.createSocketAndConnect(self.srvIp, self.srvPort)


	def createSocketAndConnect(self, h, p):
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

		if (self.debugLevel >= 20):
			print "host:", h, " port:", p

		sock.connect( (h, p) )

		timeout_val = struct.pack("ll", self.rcvTimeout, 0)

		sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout_val)

		if (self.debugLevel >= 20):
			print "socket.SO_RCVTIMEO set to:", self.rcvTimeout, "seconds"

		sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
		if (self.debugLevel >= 20):
			print "sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)... DONE"

		return (sock)



	def sendRPCandReceiveResults(self, procCall, b_expectReturnStatus=0):
		typ = type(procCall)
		if (typ == type("s") or typ == type(1) or procCall == None):
			return (procCall)

		ret = -22

		try:
			debugLog( 0, 0,  "sendRPCandReceiveResults(): BEFORE sendRPC()") 

			rc = self.sendRPC(procCall, b_expectReturnStatus)

			debugLog( 0, rc,  "sendRPCandReceiveResults(): AFTER sendRPC()") 
			if (rc <= -1):
				return ("sendRPC() returned -1")

			debugLog( 0, 0,  "sendRPCandReceiveResults(): BEFORE receiveResults()") 

			ret = self.receiveResults(self.b_capture_results, b_expectReturnStatus)

			debugLog( 0, 0,  "sendRPCandReceiveResults(): AFTER receiveResults()") 
			if RESP_ERR_DISCONNECT == ret:
				self.resetConn(ERR_DISCONNECT)
				ret = "%s: SERVER_DISCONNECT while receive response" % (self.srvName, )
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			if (gxutils.getDiagnosisFromExcInfo(ex) == gxutils.EXC_DISCONNECT):
				self.resetConn(ERR_DISCONNECT)
				m1 = "%s: SERVER_DISCONNECT" % (self.srvName, )
				return m1

			m1 = "sendRPCandReceiveResults() Exc: %s" % (ex, )
			return m1

		if type(ret) == type("s"):
			self.errMsg2 = ret

		return (ret)



		
	def sendRPC(self, procCall, b_expectReturnStatus=0):
		if (self.debugLevel >= 1):
			print "procCall:", repr(procCall)

		if (self.clt_conn == None):
			self.clt_conn = gxclientconn.gxClientConn(None)	# no client conn!

		# NOTE: we would need something like a DestConnPool to make the DestConn
		#	able to reconnect after a "connection reset by peer" error.
		#
		#	For now, pool=None in the following instantiation of DestConn.
		# 
		if (self.destConn == None):
			self.destConn = gxdestconn.DestConn("xmclient_DestConn", data=self.sock, pool=None)

		frame = gxrpcframe.gxRPCframe(self.clt_conn)

		self.clt_conn.assignState(gxclientconn.STATE_LOGIN_IN_PROGRESS, "dummyUserid")
		self.clt_conn.assignRPCframe(frame)

		if (self.debugLevel >= 10):
			print "sock: ", self.sock
			print "CONN: ", self.clt_conn
			print "FRAME:", frame
			print "events list:", self.clt_conn.getEventsList()

		frame.assignProccall( procCall )

		frame.assignDestconn(self.destConn)

		if (self.debugLevel >= 10):
			print "FRAME:", frame
			### print "DestConn:", self.destConn	# cannot because .pool == None

		if (self.debugLevel >= 1):
			print "sending RPC..."

		rc = gxxmltpwriter.sendProcCall(frame)

		if (self.debugLevel >= 1):
			print "gxxmltpwriter.sendProcCall() done, rc =", rc

		if (self.debugLevel >= 10):
			print "frame.getLastEvent():", frame.getLastEvent()
			print "frame.getErrMsg():", frame.getErrMsg()
			print "frame.getPrevExcInfo():", frame.getPrevExcInfo()
			print "frame.getExceptionDetailsAsString():", frame.getExceptionDetailsAsString()

		return (0)


	def receiveResults(self, b_capture=1, b_expectReturnStatus=0):
		#
		if (self.debugLevel >= 10):
			print "Using ctx=%s to receive response on socket %s." % (self.ctx, self.sock.fileno() )

		###print "...",
		#
		# the xmltp_gx.so module has a method called "parseResponse()". 
		# Call it to receive the response and return it here as a Tuple...
		#
		try:
			resp = xmltp_gx.parseResponse(self.ctx, self.sock.fileno(), 0, b_capture, self.rcvTimeout)
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			m1 = "xmltp_gx.parseResponse() Exc: %s" % (ex, )
			return m1

		return self.analyzeResponseReturnResultSet(resp, b_expectReturnStatus)


	def analyzeResponseReturnResultSet(self, resp, b_expectReturnStatus):
		#
		# We assume that the response was built by xmltp_gx.c like this:
		#
		#	py_response = Py_BuildValue("(OOOiiiiiiisssss)", p_resp->result_sets_list,
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
		#			        msg_buff   );
		#
		global g_debug_resp
		if (s_debug_level >= 50):
			g_debug_resp = "%s" % (resp, )

		if (len(resp) != 16):
			print "WARNING! the response tuple has", len(resp), "elements! It should be 15."
			print "This program will probably catch an exception."

		resList = []
		parList = []
		msgList = []
		tot_rows = -99
		ret      = -99999
		eot      = 99
		cltErrno = -99
		rc       = 99
		ctr_err  = -99
		manip_err = -99
		old_map    = "[old_map]"
		resp_hand  = "[resp_handle_msg]"
		parser_msg = "[parser_msg]"
		lexer_msg  = "[lexer_msg]"
		errMsg     = "[errmsg]"

		try:
			resList = resp[0]
			parList = resp[1]
			msgList = resp[2]
			tot_rows = resp[3]
			ret      = resp[4]
			eot      = resp[5]
			cltErrno = resp[6]
			rc       = resp[7]
			ctr_err  = resp[8]
			manip_err = resp[9]
			old_map    = resp[10]
			resp_hand  = resp[11]
			parser_msg = resp[12]
			lexer_msg  = resp[13]
			errMsg     = resp[14]
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			m1 = "*** Failed to get tuple element:" % (ex, )
			return m1

		if (ctr_err == 2 and cltErrno == 0):
			return RESP_ERR_DISCONNECT

		if (cltErrno  or  ctr_err  or  rc):
			errmsg = "clt_errno=%s, rc=%s, ctr_err=%s" % (cltErrno, rc, ctr_err)
			if (parser_msg):
				errmsg = errmsg + ("parser_msg: %s" % (parser_msg, ))
			if (lexer_msg):
				errmsg = errmsg + ("lexer_msg: %s" % (lexer_msg, ))
			if (errMsg):
				errmsg = errmsg + ("errMsg: %s" % (errMsg, ))
			return errmsg

		if (resp_hand or manip_err):
			x = " manip_err:%s, resp_hand:%s" % (manip_err, resp_hand)
			errmsg = errmsg + x
			return errmsg

		if (not resList  and  not parList  and  not msgList):	# if nothing to show, return
			if b_expectReturnStatus:
				return ret			# special case: return status 0 is more important than msg

			m1 = "No result set. ret=%s" % (ret, )
			return m1

		if (self.debugLevel >= 1):
			m1 = "%s results sets, %s output params, %s messages" % (len(resList), len(parList), len(msgList), )
			print m1

		if resList:
			return resList[0]		# NOTE: supports only (1) result set
			
		if parList:
			m1 = "No result set, but %s output params. (unsupported)" % (len(parList), )
			return m1

		if msgList:
			if b_expectReturnStatus and 0 == ret:
				return ret			# special case: return status 0 is more important than msg

			m = msgList[0]
			num, msg = m
			m1 = "MSG: %s - %s" % (num, gxrpcmonitors.unEscapeXML(msg) )
			return m1

		if b_expectReturnStatus:	# in this case the caller wants the retunr status as integer
			return ret

		m1 = "ret=%s" % (ret, )
		return m1



if __name__ == '__main__':
	x =abc
	if (len(sys.argv) < 2):
		print "Usage:     xrpc.py  srvPort  [srvIpAddress]  [DEBUG] [XML]"
		print "   or:     xrpc.py  xmltp_hosts_name         [DEBUG] [XML]"
		print "examples:  xrpc.py   7778                NOTE: default to 127.0.0.1"
		print "           xrpc.py  XMLOTRGX"
		print "           xrpc.py   7778    127.0.0.1"
		raise SystemExit

	srvPort    = sys.argv[1]

	debugLevel = None
	srvIp      = None
	rawXml     = None

	if (len(sys.argv) >= 3):
		srvIp = sys.argv[2]
		if (srvIp == "DEBUG"):
			debugLevel = 10
			srvIp      = None
		if (srvIp == "XML"):
			rawXml     = 1001
			srvIp      = None
		if (len(sys.argv) >= 4 and sys.argv[3] == "DEBUG"):
			debugLevel = 10
		if (len(sys.argv) >= 4 and sys.argv[3] == "XML"):
			rawXml     = 1001

	x = xrpc(srvIp, srvPort, debugLevel, rawXml)

	x.runShlex()

	print "[End of xrpc.py]"
