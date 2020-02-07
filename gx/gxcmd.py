#
# gxcmd.py
#
# $Header: /ext_hd/cvs/gx/gxcmd.py,v 1.2 2006-06-04 18:23:15 toucheje Exp $
#
#  Copyright (c) 2006 Jean-Francois Touchette
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
# Send an operator command to one or many gxserver(s).
#
# Usage:
#	python  gxcmd.py -U userid "command option..." [DEBUG] servername(s) ... 
#	python  gxcmd.py help
#
# If only the argument "help" is given on the command line, then
# the program displays the following help text defined in the variable 
# HELP_TEXT (just below).
#
# ------------------------------------------------------------------------
# 2006avr04,jft: first version, uses xmclient. 
# 2006jun04,jft: added GNU LGPL
#

import os		# environ dictionary
import sys
import time

import gxutils
import TxLog
import xmclient		# XMLTP client object

HELP_TEXT = """
 gxcmd.py sends "command option..." to each server in the servernames list,
 in sequence.

 The program will ask the password interactively, unless there is an
 environment variable PASSWORD_FILE_PATH defined.

 You should also define a LOG_PATH environment variable which points
 to the log file where you want the program to write to,
 otherwise, it will write to "./gxcmd.log"
"""

VERSION_ID = "v0.1 $Header: /ext_hd/cvs/gx/gxcmd.py,v 1.2 2006-06-04 18:23:15 toucheje Exp $"
MODULE_NAME = "gxcmd"

#
# Modules variables
#
logPath = os.environ.get("LOG_PATH", "./gxcmd.log")
srvlog = TxLog.TxLog(logPath, MODULE_NAME)
xmclient.srvlog = srvlog


# DEBUG level global variable:
#
s_debug_level          = 0		# 1 for unit testing, 0 for Prod, 10 for intensive debugging.
xmclient.s_debug_level = s_debug_level	# propage cette valeur dans xmclient.py


def debugLog(level, n, msg, requiredDebugLevel=10):
	if (s_debug_level >= requiredDebugLevel):
		writeLog(level, n, msg)

def writeLog(level, n, msg, ex=None):
	srvlog.writeLog(MODULE_NAME, level, n, msg)
	if ex:
		print "Log:", level, n, msg, ex

def displayAndLog(msg):
	print msg
	writeLog(0, 0, msg)


class gxcmd:
	def __init__(self, userid, passwd, cmdText, srvList):
		self.userid = userid
		self.passwd = passwd
		
		self.cmdText = cmdText
		self.procCall = self.makeProcCallFromCmdText(cmdText)

		self.srvList = srvList


	def makeProcCallFromCmdText(self, cmdText):
		#
		# Called by:	__init__()
		#
		if not cmdText:
			return
		ls = cmdText.split()
		if not ls:
			return

		ls2 = []
		for arg in ls:
			ls2.append( arg.strip(",") )   # remove "," between args

		x = xmclient.xmProcCall()
		ret = x.buildProcCallFromList(ls2)
		if ret != 0:
			displayAndLog(ret)
			return ret		# ret is an error msg

		return x


	def createXMClient(self, srv, b_createOnly=0):
		x = xmclient.xmclient(self.userid, self.passwd, srvPort=srv)

		if b_createOnly:	# at instance creation time
			return x

		msg = x.prepareConnection()
		if msg:
			m1 = "%s - No Connection: %s" % (srv, msg)
			displayAndLog(m1)
			return 

		return x

			
	def sendCommandToAllServers(self):
		m1 = "About to send command '%s' to %s ..." % (self.cmdText, self.srvList, )
		displayAndLog(m1)

		try:
			for srvname in self.srvList:
				clt = self.createXMClient(srvname)
				if not clt:
					continue

				res = self.sendCommand(clt, self.cmdText, self.procCall, srvname)

				self.processResults(res, srvname)
		except:
			ex = gxutils.formatExceptionInfoAsString(7)
			m1 ="sendCommandToAllServers() '%s' to %s got exception: %s" % (self.cmdText, srvname, ex)
			displayAndLog(m1)

	def sendCommand(self, clt, cmdText, procCall, srvname):
		st = "sendCommand()..."
		try:
			st = "clt.sendRPCandReceiveResults()"
			res = clt.sendRPCandReceiveResults(self.procCall, b_expectReturnStatus=0)
			#
			# res could be a string if there has been an error
		except:
			ex = gxutils.formatExceptionInfoAsString(1)
			m1 = "%s %s %s Exc: %s" % (st, srvname, cmdText, ex)
			return m1
			
		return res

	def processResults(self, res, srvname):
		msgNo = 0
		if res == None:
			vLines = [ "[No result set]", ]
		elif type(res) == type("s"):
			vLines = [ res, ]
		elif type(res) == type(1):
			msgNo = res
			vLines = [ str(res), ]
		else:
			vLines = xmclient.convertResultSetToTextLines(res, 1)
###			vLines = xmclient.convertResultSetToTextLines(res, 1, srvname)

		m1 = "%s : %s" % (srvname, vLines, )
		writeLog(0, msgNo, m1)
		if len(vLines) <= 1:
			print m1
		else:
			print srvname, ":"
			for aLine in vLines:
				print aLine


def readFirstLineOfFile(fileName):
	## copied from gxserver.gxserver
	##
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


def print_usage(b_with_helptext):
	print "Usage:  \n    python  gxcmd.py -U userid 'command option...' [DEBUG] servername(s) ... "
	if b_with_helptext:
		print HELP_TEXT

if __name__ == '__main__':
	if len(sys.argv) < 5 or sys.argv[1].upper() != "-U":
		if len(sys.argv) <= 1 or sys.argv[1].lower == "help":
			b_with_helptext = 1
		else:
			b_with_helptext = 0
		print_usage(b_with_helptext)
		raise SystemExit

	userid = sys.argv[2]

	cmdText = sys.argv[3]

	if sys.argv[4].upper() == "DEBUG":
		s_debug_level          = 10
		xmclient.s_debug_level = s_debug_level	# propage cette valeur dans xmclient.py
		pos = 5
	else:
		pos = 4

	srvList = sys.argv[pos:]

	passwordFilePathEnvVar = "PASSWORD_FILE_PATH"
	passwdFile = os.environ.get(passwordFilePathEnvVar, None)
	if passwdFile:
		passwd = readFirstLineOfFile(passwdFile)
		if not passwd:
			print "Failed to read password from:", passwordFilePathEnvVar
	else:
		print "The", passwordFilePathEnvVar, "environment variable is not defined."
		passwd = None
	if not passwd:
		import getpass
		passwd = getpass.getpass()

	aGxCmd = gxcmd(userid, passwd, cmdText, srvList)

	aGxCmd.sendCommandToAllServers()

	raise SystemExit

