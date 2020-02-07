#!/usr/bin/python

#
# xrpc.py :  XMLTP "irpc" shell
# -----------------------------
#
# $Source: /ext_hd/cvs/gx/xrpc.py,v $
# $Header: /ext_hd/cvs/gx/xrpc.py,v 1.3 2004/09/20 11:22:46 blanchjj Exp $
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
# xrpc.py :  XMLTP "irpc" shell		similar to "IRPC".
# -----------------------------
#
#
#! Version 0.2, 2002fev20:
#  	Ajouts:
#		+ meta-commande "/cap" ou "/capture" passe un flag a xmltp_gx pour
#		  capturer la reponse dans des objets Python. Ceux-ci sont
#		  ensuite affiches.
#		+ meta-commande "?" ou "/h" ou "/help" affiche un "help"
#		+ affiche des resultats un peu mieux.
#	NOTES:
#		. les meta-commandes suivantes existaient deja:
#			+ exit   (meme chose que ctrl-D sur Unix ou ctrl-Z sur NT)
#			+ reset
#
# Limitations de la version 0.1, 2002fev03 :
#	. un seul serveur
#	. la ligne de commande doit contenir l'adresse IP et le numero de port
#	. le login doit etre fait manuellement (avec un RPC!)
#	. output params are NOT supported now.
#	. le programme ne se reconnecte pas si la connexion socket avec le serveur
#	  est brisee. Il faut redemarrer pour etablir une nouvelle connexion.
#	. le formattage des resultats est tres simple, meme laid.
#
#
# USAGE NOTES:
# -----------
#
#
# The syntax of a RPC read from stdin must be:
#
#   rpc_name  [ [ @param_name = ] param_value ] [ , [ @param_name = ] param_value ] ...  { ';' | GO }
#
# "GO" indicates the end of the RPC. It can also be written as "go" or as ";"
#
# All parameter names must start with a "@" character.
# 
# Supported parameter value types are:
#	. string	ex.: "Jeremy Bender", 'an "other" string here'
#	. integer	ex.: 154123
#	. float		ex.: 98453.512
#
#| NOTE:  the DATE or DATETIME type is not supported in version 0.1, please use string type.
#
# Comments begin with a "#" character.
#
#
# TECHNICAL NOTES:
# ---------------
# Internally, the RPC which is read from stdin is transformed into the usual 
# Python representation for a XMLTP proc call.
#
# Example:
#
#	('trace', [('__return_status__', '-9999',   4, 1, 1), 
#		   ('@p_dbName', 	 'Master', 12, 0, 0)], 0)
#
#
# This script use the function gxxmltpwriter.sendProcCall(rpcFrame) to send
# the RPC all.
# And, it uses xmltp_gx.so to receive the results.
#
# -----------------------------------------------------------------------
# 2002feb02,jft: + first version
# 2002feb20,jft: + xrpc.displayRespTuple(resp)
#		 + xrpc.b_capture_results
# 2002mar11,jft: . xrpc.displayRespTuple(): simplified output
#		 . xrpc.processOneRPC(): handle exception around self.getOneRPC()
# 2002mar22,jft: . sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
# 2002may13,jft: . displayRespTuple(): update size of response (len(resp) must be == 16)
# 2002jul05,jft: . call xmltp_gx.parseResponse(..., 210) nb_sec timeout
# 2002sep14,jft: . reduce output on stdout
# 2002sep16,jft: + printResultSet(colNames, rows)
# 2002sep17,jft: . displayRespTuple(): for each msg call gxrpcmonitors.unEscapeXML(m)
# 2002sep18,jft: . printResultSet(): format double with only 2 decimals
#		 . class xrpc, __init__(): default debugLevel=0  (was 1)
# 2002sep26,jft: . displayRespTuple(): display at least return status, even without any result set
# 2003jan18,jft: . expectParamValue(): PATCH pour supporter les output params!!!
# 2003mar22,jft: . now use xmltp_hosts.getServerHostAndPort()
# 2003jul30,jft: . ProcCallBuilder.expectParamValue(): support .9 as a Float
#

import time
import sys
import shlex		# for shlex() class
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
import TxLog
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
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/xrpc.py,v 1.3 2004/09/20 11:22:46 blanchjj Exp $"

MODULE_NAME = "xrpc.py"

#
# Modules variables
#
srvlog = TxLog.TxLog("xrpc.log", MODULE_NAME)



FMT_TIMESTAMP_STR = "%23.23s"
FMT_INT_STR	  = "%6d"
FMT_NUM_STR	  = "%12.5f"
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



def printResultSet(colNames, rows):
	colFormatList = []
	hdrStrList = []
	underlineStrList = []

	for col in colNames:
		if (len(col) == 3):
			name, typ, sz = col
		else:
			sz = None
			name, typ = col

		fmt, width = dictTypeToFormatString.get(typ, (FMT_UNKNOWN_STR, 5) )

		if (width == None):		# string value (VARCHAR, CHAR), has a "sz"
			if (sz == None):
				sz = 3			# arbitrary value

			width = sz		# use this value to prepare columns headings
		else:
			sz = None

		if (len(name) > width):
			width = len(name)

		fmtTuple = (fmt, typ, width, sz)

		colFormatList.append(fmtTuple)

		hdr = "%*.*s" % (width, width, name)
		hdrStrList.append(hdr)

		underStr = "-" * width
		underlineStrList.append(underStr)

	print " ".join(hdrStrList)
	print " ".join(underlineStrList)

	ctr = 0
	for row in rows:
		valuesList = []
		for i in xrange(len(row) ):

			fmt, typ, width, sz = colFormatList[i]
	
			val, isNull    = row[i]

			if (isNull):
				valFmt = "NULL"
			else:
				# NOTE: convertToPythonTypeIfNotString() returns None is val does not
				#	require to be converted.
				#
				newValue = gxintercept.convertToPythonTypeIfNotString(val, typ, isNull)

				if (newValue == None):	# if None, then val is a string...
					newValue = gxrpcmonitors.unEscapeXML(val)

				if (sz == None):
					valFmt = fmt % (newValue, )
				else:
					valFmt = fmt % (sz, sz, newValue, )

			# now, make all values follow the fixed width of that column:
			#
			valStr = "%*.*s" % (width, width, valFmt)

			valuesList.append(valStr)

		print " ".join(valuesList)
		ctr = ctr + 1

	return ctr



#
# Emulation classes used to allow the use of gx modules:
#
class DummyGxServer:
	def writeLog(self, moduleName, logLevel, msgNo, msg):
		print "writeLog():", moduleName, logLevel, msgNo, msg

dummyGxServer = DummyGxServer()

gxclientconn.gxServer = dummyGxServer


#
# the class ProcCallBuilder has function-based internal states 
# and can build a XMLTP Proc Call object (tuple), like this one:
#
#	('trace', [('__return_status__', '-9999',   4, 1, 1), 
#		   ('@p_dbName', 	 'Master', 12, 0, 0)], 0)
#
# The serie of input token are expect to be following this 
# syntax:
#
#   rpc_name  [ [ @param_name = ] param_value ] [ , [ @param_name = ] param_value ] ...  { ';' | GO }
#
class ProcCallBuilder:
	def __init__(self):
		self.errmsg = "[no errmsg yet]"
		self.nextFunction = None
		self.procCall  = None
		self.procName  = None
		self.paramList = None
		self.currParamName = None

	def getProcCall(self):
		if (self.procCall):
			if (len(self.paramList) == 0):
				p1 = ('__return_status__', '-9999', 4, 1, 1)
				self.paramList.append(p1)
			return (self.procCall)
		else:
			raise RuntimeError, ("Error: " + self.errmsg)

	def addProcName(self, procName):
		c = procName[0]
		if (c == '?' or c == '/'):
			self.nextFunction = self.addProcName
			return (0)
		if (not procName[0].isalpha() ):
			self.errmsg = "procName must begin with alpha character"
			raise RuntimeError, ("Error: " + self.errmsg)
		self.procName = procName
		self.paramList = []
		self.procCall = (self.procName, self.paramList, 0)

		self.nextFunction = self.addNewParam
		return (0)

	def addNewParam(self, token):
		#
		# token can be a parameter name (like "@param_1")
		# or a parameter value (string, int, float value).
		#
		if (token[0] == "@"):
			if (len(token) <= 2):
				self.errmsg = "param name too short: " + token
				raise RuntimeError, ("Error: " + self.errmsg)

			self.currParamName = token
			self.nextFunction = self.expectEqualSign
			return (0)

		paramNo = len(self.paramList) + 1
		self.currParamName = "@p_%03d" % (paramNo, )

		return (self.expectParamValue(token) )


	def expectEqualSign(self, token):
		#
		# token must be "=" or it is an error
		#
		if (token != "="):
			self.errmsg = "expecting '=', got: " + token
			raise RuntimeError, ("Error: " + self.errmsg)

		self.nextFunction = self.expectParamValue
		return (0)

	def expectParamValue(self, token):
		#
		#	('__return_status__', '-9999',   4, 1, 1)
		#	('@p_dbName', 	      'Master', 12, 0, 0)
		#
		if (token == '=' or token == ','):
			self.errmsg = "expect param value, got: " + token
			raise RuntimeError, ("Error: " + self.errmsg)

		valType = xmltp_tags.JAVA_SQL_VARCHAR
		if (token[0].isdigit() or token[0] == '.'):
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

		## PATCH pour supporter les output params:
		##
		if (self.currParamName[1].lower() == "o"):
			isOutput = 1
		else:
			isOutput = 0

		tup = (self.currParamName, str(val), valType, 0, isOutput)

		self.paramList.append(tup)

		self.nextFunction = self.expectComma

		return (0)


	def expectComma(self, token):
		if (token != ','):
			self.errmsg = "expect ',' got: " + token
			raise RuntimeError, ("Error: " + self.errmsg)

		self.nextFunction = self.addNewParam

		return (0)

	def addToken(self, token):
		if (self.nextFunction == None):
			self.nextFunction = self.addProcName

		self.nextFunction(token)

		return (0)

#
# the class xrpc runs the shlex input processor and send the RPC calls
#
class xrpc:
	def __init__(self, srvIp="127.0.0.1", srvPort=None, debugLevel=0, rawXml=0):
		if (srvIp):
			self.srvIp = srvIp
		else:
			self.srvIp = "127.0.0.1"

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
		self.shlex = shlex.shlex()
		self.shlex.wordchars = self.shlex.wordchars + "@."
		#
		## print "modified self.shlex.wordchars to:", repr(self.shlex.wordchars)

		print "Will connect to server at %s : %s -- debug=%s" % (self.srvIp, self.srvPort, self.debugLevel)

		self.ctx = None

		if (xmltp_gx != None):
			self.ctx = xmltp_gx.createContext("ctx_for_xrpc.py")
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
		self.clt_conn   = None
		self.sock       = None
		self.destConn   = None
		self.procCall   = None

		if (self.debugLevel >= 2):
			srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Starting xrpc...")

	def runShlex(self):
		if (self.debugLevel >= 10):
			print "shlex=", self.shlex

		try:
			self.connectToServer()

			print "connected to", self.srvPort, " OK."
			print "NOTE: it is up to you to do 'login' with an RPC."

			while (1):
				rc = self.processOneRPC()
				if (rc != 0 and rc != None):
					if (rc <= -1):
						print "Aborting, rc:", rc
					break
		except:
			ex = gxutils.formatExceptionInfoAsString(2)
			print "Failed. Exception:", ex

	def connectToServer(self):
		self.sock = self.createSocketAndConnect(self.srvIp, self.srvPort)

	def createSocketAndConnect(self, h, p):
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

		if (self.debugLevel >= 2):
			print "host:", h, " port:", p

		sock.connect( (h, p) )

		timeout_val = struct.pack("ll", self.rcvTimeout, 0)

		sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout_val)

		if (self.debugLevel >= 2):
			print "socket.SO_RCVTIMEO set to:", self.rcvTimeout, "seconds"

		sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
		if (self.debugLevel >= 2):
			print "sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)... DONE"

		return (sock)

	def processOneRPC(self):
		try:
			procCall = self.getOneRPC()
		except:
			ex = gxutils.formatExceptionInfoAsString(2)
			print "Failed parsing input. Exception:", ex

			self.ctrErrors = self.ctrErrors + 1
			if (self.ctrErrors >= 10):
				print "getOneRPC(). Too many errors, stopping. self.ctrErrors:", self.ctrErrors
				return (-98)
			return None

		if (procCall == None or procCall == -1 or procCall == 1):
			return (procCall)

		try:
			rc = self.sendRPC(procCall)
			if (rc <= -1):
				return (rc)

			rc = self.receiveResults(self.b_capture_results)
			return (rc)
		except:
			ex = gxutils.formatExceptionInfoAsString(2)
			print "Failed. Exception:", ex

			self.ctrErrors = self.ctrErrors + 1
			if (self.ctrErrors >= 10):
				print "Too many errors, stopping. self.ctrErrors:", self.ctrErrors
				return (-99)
		return (-22)

	def displayHelp(self):
		print
		print "?, /h or /help		display this help"
		print "GO or ';'		complete RPC input and sends it"
		print "reset			empty the input buffer"
		print "/  			toggle response capture mode"
		print "login <userid>, <pwd>;	send 'login' RPC to server (Do this first!)"
		print "exit or ctrl-D		exit this program"
		print "RPC syntax:"
		print "	procName [ [ paramName1 =] 'str'|str|int|float ] [, <another param>]..."
		print " GO | ;"
		print


	def getOneRPC(self):
		print "xrpc ==> ",

		procCallBuilder = ProcCallBuilder()

		rc = 0
		while (rc == 0):
			token = self.shlex.get_token()
			if (not token or token.lower() == "exit"):
				print "[EOF]"
				rc = 1
				break
			if (self.debugLevel >= 10):
				print "token '%s'" % (token, )
			if (token.lower() == "go" or token == ";"):
				print "[GO or ';']"
				break

			if (token.lower() == "/" or token.lower() == "/cap"  or token.lower() == "/capture"):
				self.b_capture_results = (not self.b_capture_results)
				if (self.b_capture_results):
					print "[Capture response ON]"
				else:
					print "[Capture response OFF]"
				return (None)

			if (token.lower() == "?" or token.lower() == "h" or token.lower() == "/h" or token.lower() == "/help"):
				self.displayHelp()
				return None

			if (token.lower() == "reset"):
				print "[reset]"
				return (None)

			try:
				procCallBuilder.addToken(token)
			except:
				ex = gxutils.formatExceptionInfoAsString(2)
				print "RPC parsing failed:", ex
				rc = -1

		if (rc == 0):
			return (procCallBuilder.getProcCall() )

		if (rc == 1):
			if (procCallBuilder.procCall == None):
				return (1)
			return (procCallBuilder.getProcCall() )

		return (-1)
		
	def sendRPC(self, procCall):
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
			self.destConn = gxdestconn.DestConn("xrpc_DestConn", data=self.sock, pool=None)

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


	def receiveResults(self, b_capture=1):
		#
		if (self.debugLevel >= 1):
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
			print "*** receive and parse response failed:", ex
			resp = None
		try:
			if (type(resp) == type( () ) ):
				self.displayRespTuple(resp)
			else:
				print "resp:", resp
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			print "*** Exception while displaying response:", ex

		return (0)


	def displayRespTuple(self, resp):
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
			print "*** Failed to get tuple element:", ex

		if (cltErrno  or  ctr_err  or  rc):
			print "clt_errno:", cltErrno
			print "rc:", rc, "ctr_err:", ctr_err
			print "old_map:", old_map
			if (parser_msg):
				print "parser_msg:", parser_msg
			if (lexer_msg):
				print "lexer_msg:", lexer_msg
			if (errMsg):
				print "errMsg:", errMsg

		if (resp_hand or manip_err):
			print "manip_err:", manip_err, "resp_hand:", resp_hand

		if (not resList  and  not parList  and  not msgList):	# if nothing to show, return
			print "return status: ", ret
			return None

		m1 = "%s results sets, %s output params, %s messages" % (len(resList), len(parList), len(msgList), )
		print m1

		try:
			for resSet in resList:
				colNamesList, rowsList, nbRows = resSet

				ctr = printResultSet(colNamesList, rowsList)

				print "nbRows:", nbRows, "==", ctr
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			print "*** Failed to display all result sets:", ex
			
		try:
			if (len(parList) > 0):
				for param in parList:
					print param
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			print "*** Failed to display all params:", ex

		try:
			if (len(msgList) > 0):
				for m in msgList:
					num, msg = m
					print  num, ":", gxrpcmonitors.unEscapeXML(msg)
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			print "*** Failed to display all messages:", ex

		if (tot_rows):
			print "return status: ", ret, " tot_rows:",tot_rows
		else:
			print "return status: ", ret

		if (eot):
			print "EOT:", eot




if __name__ == '__main__':
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
