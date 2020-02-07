#!/usr/bin/python

#
# gxutils.py :  Gateway-XML  general utility functions
# ----------------------------------------------------
#
# $Source: /ext_hd/cvs/gx/gxutils.py,v $
# $Header: /ext_hd/cvs/gx/gxutils.py,v 1.5 2006-05-17 17:01:13 toucheje Exp $
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
# General Functions that can be used by any GX module.
#
# -----------------------------------------------------------------------
#
# 2001nov25,jft: first version
# 2001dec01,jft: . formatExceptionInfoAsString(maxTBlevel=5): added default arg
# 2001dec06,jft: + getDiagnosisFromExcInfo()
#		 + getDiagnosisAndExceptionInfo()
# 2001dec10,jft: . getDiagnosisAndExceptionInfo(): default arg: maxTBlevel=5
#		 + EXC_DS_SERVER_NOT_AVAIL
#		 . diagnose repr( ('AttributeError', ("'None' object has no attribute 'send'",) ) ) as EXC_DISCONNECT
# 2002jan29,jft: + added standard trace functions
# 2002apr15,jft: + getTimeStringForFileName(tm1=None)
# 2002may31,jft: + repr( ("RuntimeError", ("send(): connection broken",) ) ) :  EXC_DISCONNECT,
# 2002june4,jft: + repr( ("RuntimeError", ("recv(): connection broken",) ) ) :  EXC_DISCONNECT,
# 2005sep29,jft: + repr( ('error', (35, 'Resource temporarily unavailable') ) ) : EXC_RECV_TIMEOUT,
# 2006jan04,jft: + repr( ('error', (48, 'Address already in use')  ) ) : EXC_ADDRESS_ALREADY_IN_USE, 
# 2006mai12,jft: + repr( ('error', (11, 'Resource temporarily unavailable') ) ) : EXC_RECV_TIMEOUT,
#

import time
import sys
import traceback

#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/gxutils.py,v 1.5 2006-05-17 17:01:13 toucheje Exp $"

#
# CONSTANTS:
#
EXC_META_EXCEPTION	= 0	# exception generated after a first original exception
EXC_DISCONNECT		= 1
EXC_DS_SERVER_NOT_AVAIL = 2
EXC_RECV_TIMEOUT	= 3
EXC_ADDRESS_ALREADY_IN_USE = 4

#
# Exceptions Diagnosis Dictionary:
#
# NOTE: do not forget the repr() surrounding the tuple as strings are
#	the only real way here to get the expected results with this 
#	dictionary. Using a tuple directly does not work: no match are
#	ever found, unless the exact same instance/object/tuple is used.
#	String keys are less restrictive.
#
ExcDiagDict = {
		repr( ("error", (32, 'Broken pipe') ) )	: EXC_DISCONNECT,
		repr( ('error', (107, 'Transport endpoint is not connected') ) ) : EXC_DISCONNECT,
		repr( ('AttributeError', ("'None' object has no attribute 'send'",) ) ) :  EXC_DISCONNECT,
		repr( ("RuntimeError", ("recv(): connection broken",) ) )		:  EXC_DISCONNECT,
		repr( ("RuntimeError", ("send(): connection broken",) ) )		:  EXC_DISCONNECT,
		repr( ('error', (111, 'Connection refused') ) ) : EXC_DS_SERVER_NOT_AVAIL,
		repr( ("RuntimeError", ('gxxmltpwriter.sendProcCall(frame) FAILED',)) ): EXC_META_EXCEPTION,
		repr( ('error', (35, 'Resource temporarily unavailable') ) ) : EXC_RECV_TIMEOUT,
		repr( ('error', (11, 'Resource temporarily unavailable') ) ) : EXC_RECV_TIMEOUT,
		repr( ('error', (48, 'Address already in use')  ) ) : EXC_ADDRESS_ALREADY_IN_USE, 
	}

#
# module-scope debug_level:
#
debug_level = 0


#
# There is no class definition here, only functions.
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
# Public functions:
#
def getTimeStringForFileName(tm1=None):
	"""return a string with date, time (with miliseconds)
	   in the format:
			YYYYMMDD.HHMMSS.mmm
	"""
	if (tm1 == None):
		tnow = time.time()
	else:
		tnow = tm1
	ms = (tnow - int(tnow) ) * 1000
	if (ms >= 1000):
		ms = ms - 1	# we don't want hh:mm:ss.1000 (ms)
		tnow = tnow + 1
	timeTuple = time.localtime(tnow)
	s1 = time.strftime("%Y%m%d.%H%M%S", timeTuple)
	s2 = ("%s.%03d" % (s1, ms) )
	return (s2)




def getDiagnosisFromExcInfo(excInfoTuple):
	#
	# convert a tuple returned by formatExceptionInfo() to
	# a simpler constant, such as EXC_DISCONNECT
	#
	# if there is no match, return None
	#
	if (type(excInfoTuple) != type( () ) ):	# be very careful here
		return None
	if (len(excInfoTuple) < 2):	# make sure we don't mess recovery!
		return None

	excPattern = (excInfoTuple[0], excInfoTuple[1])

	try:
		return ExcDiagDict[ repr(excPattern) ]
	except KeyError:
		return None

def getDiagnosisAndExceptionInfo(maxTBlevel=5):
	excInfoTuple = formatExceptionInfo(maxTBlevel)

	return (getDiagnosisFromExcInfo(excInfoTuple), excInfoTuple)

def formatExceptionInfo(maxTBlevel=5):
	#
	# return a tuple containing (3) strings: exception name, arg and traceback.
	#
	cla, exc, trbk = sys.exc_info()
	excName = cla.__name__
	try:
		excArgs = exc.__dict__["args"]
	except KeyError:
		excArgs = "<no args>"
	excTb = traceback.format_tb(trbk, maxTBlevel)

	return (excName, excArgs, excTb)


def formatExceptionInfoAsString(maxTBlevel=5):
	return "%s: %s, %s." % formatExceptionInfo(maxTBlevel)



if __name__ == '__main__':
	try:
		impossible = undefXXX + 1
	except:
		print formatExceptionInfoAsString()
		print formatExceptionInfo()

	a = ("error", (32, 'Broken pipe') )
	print "a:", a

	print "getDiagnosisFromExcInfo( (\"error\", (32, 'Broken pipe') ):", \
		getDiagnosisFromExcInfo( a )
	print "getDiagnosisFromExcInfo( 'allo'):", getDiagnosisFromExcInfo( "allo" )

