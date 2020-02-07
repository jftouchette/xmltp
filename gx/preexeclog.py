#!/usr/local/bin/python

#
# preexeclog.py
# -------------
#
# $Source: /ext_hd/cvs/gx/preexeclog.py,v $
# $Header: /ext_hd/cvs/gx/preexeclog.py,v 1.2 2005/11/03 19:55:49 toucheje Exp $
#
#  Copyright (c) 2005 Jean-Francois Touchette
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
# preexeclog.py
# -------------
#
# 2005nov02,jft: first version
# 2005nov03,jft: add timestamp to a tuple before calling cPickle.dump()
#

import time
import StringIO
import cPickle
import TxLog

# Constants:
#
MODULE_NAME = "preexeclog"
PREFIX_END_MARK = ">> "

# module scope variable(s):
#
g_log = None


def initLogFilePath(serverName, filePath):
	global g_log
	if g_log == None:
		g_log = TxLog.TxLog(filePath, serverName)
		g_log.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "### log started ###")
		writePreExecLog("### log started ###", MODULE_NAME)
	else:
		g_log.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, "### initLogFilePath() called repeatedly!")

def writePreExecLog(procName, procCallObject):
	"""Writes a line in the pre-exec log. The line format is:
		aLine = "%s;%10.15s;%-64.64s, %s\n" % (timeStr, g_log.srvName, procName, procCallString, )
	   The beginning of the line looks like:
2005-11-02 14:48:05.549;ANY_Server;spxx_super_extremely_long_procedure_name_just_to_really_see_trun>> 
	"""
	timeStr = TxLog.get_log_time()

	obj  = (time.time(), procCallObject)
	buff = StringIO.StringIO()
	cPickle.dump(obj, buff)
	buff.seek(0)

	aLine = "%s;%10.15s;%-64.64s%s%s\n" % (timeStr, g_log.srvName, procName, 
						PREFIX_END_MARK, repr(buff.getvalue()), )

	return g_log.writeLogLineRaw(aLine)


if __name__ == '__main__':

	initLogFilePath("ANY_Server", "PreExec.log")
###	initLogFilePath("ANY_Server", "PreExec.log")

	writePreExecLog("spoe_test_proc", "[serialisation de l'objet ProcCall]")
	writePreExecLog("spoe_test_proc", "[serialisation de l'objet ProcCall]")
	writePreExecLog("spxx_super_long_procedure_name_just_to_see_truncation", "[serialisation de l'objet ProcCall]")
	writePreExecLog("spxx_super_very_long_procedure_name_just_to_see_truncation", "[serialisation de l'objet ProcCall]")
	writePreExecLog("spxx_super_extremely_long_procedure_name_just_to_see_truncation", "[serialisation de l'objet ProcCall]")
	writePreExecLog("spxx_super_extremely_long_procedure_name_just_to_really_see_truncation", "[serialisation de l'objet ProcCall]")
	ls = [1, 2, 3, 4, 5, "abc",]
	tup = (1, 2,3, ls)
	writePreExecLog("spoe_liste", ls)
	writePreExecLog("spoe_Tuple", tup)



