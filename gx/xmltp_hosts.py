#!/usr/bin/python

#
# xmltp_hosts.py :  support for the "xmltp_hosts" file
#
#
# $Source: /ext_hd/cvs/gx/xmltp_hosts.py,v $
# $Header: /ext_hd/cvs/gx/xmltp_hosts.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $
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
#   getServerHostAndPort(srvName): given srvName, retrieve 
#				(host, port) tuple from the 
#				"xmltp_hosts" file.
#
# NOTES: Use the XMLTP environment variable.
#	 The value of "XMLTP" is cached in a module-level variable
#	 and, if it did not exist, a msg is written to the log
#	 (if the srvlog variable is set) and "./" is used as
#	 default value.
#
# -----------------------------------------------------------------------
# 2002nov26,jft: first version
#

import os
import gxutils
import TxLog


#
# CONSTANTS
#
COMMENT_CHAR	  = '#'		# NOTE: copied from Cfg.py


#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/xmltp_hosts.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $"
MODULE_NAME = "xmltp_hosts.py" 


#
# module variables:
#
srvlog = None

envValueXMLTP = None
g_lineNo = 0
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

def getEnvValueXMLTP():
	#
	# Called by:	getServerHostAndPort(srvName)
	#
	global envValueXMLTP
	if (envValueXMLTP):
		return envValueXMLTP

	try:
		val = os.environ["XMLTP"]
		envValueXMLTP = val
		return val
	except:
		envValueXMLTP = "./"

		ex = gxutils.formatExceptionInfoAsString(2)

		m1 = "Cannot get 'XMLTP' environment variable. Using default value '%s'. Exception: %s." % (envValueXMLTP, ex)
		if (srvlog):
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
		else:
			print "Error:", m1

	return envValueXMLTP

def getServerHostAndPort(srvName):
	#
	# Given srvName, retrieve (host, port) tuple from the 
	# "xmltp_hosts" file.
	#
	# NOTE: For matching purposes, srvName is converted to uppercase,
	#	and, server names read from the "xmltp_hosts" file are
	#	also converted to uppercase.
	#
	# Return:
	#	(hosts, port)	from matching entry in the "xmltp_hosts" file
	#	a String	error msg
	#	None		no entry matched to srvName
	#
	global g_lineNo

	hostsFileName = "xmltp_hosts"
	envValueXMLTP = None
	filePath      = None
	g_lineNo      = 0
	step  = None
	f     = None
	entry = None
	try:
		step = "convert srvName to uppercase"
		srvName = srvName.upper()

		step = "prepare filepath"
		envValueXMLTP = getEnvValueXMLTP()
		if (envValueXMLTP[-1:] != '/'):
			sep = '/'
		else:
			sep = ''
		filePath = "%s%s%s" % (envValueXMLTP, sep, hostsFileName)

		step = "open file"
		f = open(filePath, 'r')

		step = "read and search"
		g_lineNo = 0
		entry = findMatchingEntry(f, srvName)

		f.close()
		f = None

		if not entry:
			return entry		# Not found

		if (type(entry) == type("s") ):
			return entry		# that's an error msg!

		step = "convert portStr to integer"
		hostName = entry[0]
		portStr  = entry[1]
		try:
			port = int(portStr)
		except:
			m1 = "port '%s' is not an integer" % (portStr, )
			return m1

		step = "return tuple"
		return (hostName, port)
	except IOError:
		ex = gxutils.formatExceptionInfoAsString(1)
		m1 = "getServerHostAndPort(): failed to access filePath='%s', step='%s'. Exception: %s." % (step, filePath, ex)
		if (srvlog):
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
		else:
			print "Error:", m1
	except:
		ex = gxutils.formatExceptionInfoAsString(3)

		m1 = "getServerHostAndPort(): failed at line %s, step '%s', srvName='%s', filePath='%s' entry='%s'. Exception: %s." % (g_lineNo, step, srvName, filePath, entry, ex)
		if (srvlog):
			srvlog.writeLog(MODULE_NAME, TxLog.L_WARNING, 0, m1)
		else:
			print "Error:", m1

	if (f):
		f.close()


def findMatchingEntry(f, srvName):
	#
	# Called by:	getServerHostAndPort()
	#
	while (1):
		line = getNonEmptyLine(f)
		if (not line):
			return None

		words = line.split()

		srv = words[0].upper()
		if (srv == srvName):
			if (len(words) >= 3):
				return words[1:4]
			msg = "Entry at line %s for '%s' is not valid: %s." % (g_lineNo, srvName, words)
			return msg

		if (g_lineNo > 20000):
			return "MORE than 20000 lines or infinite loop"


def getNonEmptyLine(fi):
	#
	# Called by:	findMatchingEntry()
	#
	# NOTE: source code copied from Cfg.getNonEmptyLine(), then modified (ex. g_lineNo).
	#
	global g_lineNo
	while (1):
		aLine = fi.readline()
		if (not aLine):
			return None

		g_lineNo = g_lineNo + 1

		# print "aLine: '%s'" % (aLine, )

		if (len(aLine) == 0):	# skip empty line
			continue

		if (aLine[0] == COMMENT_CHAR):
			continue
			
		s = aLine.lstrip()
		if (len(s) == 0):	# skip empty line (after lstrip)
			continue

		if (s[0] == COMMENT_CHAR):
			continue

		return (s)	## return line stripped of leading, trailing space

if __name__ == '__main__':
	print getRcsVersionId()
	print

	ls = ["XMDETR80", "XMDEcg80", "xmdecg81", "ToTo_1", "Bad_1", "bad_2", "toto_2", ]

	for srv in ls:
		x = getServerHostAndPort(srv)

		print srv, "\t", x


#
# End of xmltp_hosts.py
#
