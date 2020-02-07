#!/usr/bin/python

#
# TxLog.py
# --------
#
# $Source: /ext_hd/cvs/gx/TxLog.py,v $
# $Header: /ext_hd/cvs/gx/TxLog.py,v 1.1.1.1 2004/06/08 14:08:58 sawanai Exp $
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
# TxLog.py
# --------
#
# 2001avr08,jft: version 0.1
# 2001jun20,jft: writeLogLineRaw(): added semaphore
# 2002aug30,jft: new implementation with open/close at each write
#		 . writeLog(): faster (?): does not call get_thread_id(), nor, get_msg_level(level)
# 2002dec21,jft: . TxLog.writeLogLineRaw(): also catch IOError if do sys.stdout.write(), if such an error, only return -2
# 2003sep11,jft: . TxLog.writeLogLineRaw(): moved f.close() within the try: except: because (IOError, 28, "No space left on device") is caught during f.close() not during f.write()
#

import time
import sys		# for sys.stdout
import os		# for getpid()
import thread		# for get_ident()
import threading	# for currentThread(), getName()

#
# Public Constants:
#
L_MSG	 = 0
L_DEBUG	 = 1
L_WARNING = 2
L_ERROR	 = 3


MsgLevels = { 	L_MSG:"Msg",
		L_DEBUG:"Debug", 
		L_WARNING:"WARN", 
		L_ERROR:"ERROR" 	
		}

#
# Public functions:
#
def get_msg_level(level):
	try:
		return (MsgLevels[level])
	except KeyError:
		return ("Err_%d" % (level, ) ) 

def get_log_time(tm1=None):
	"""return a string with date, time (with miliseconds)
	   in the format:
			YYYY-MM-DD HH:MM:SS.mmm
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
	s1 = time.strftime("%Y-%m-%d %H:%M:%S", timeTuple)
	s2 = ("%s.%03d" % (s1, ms) )
	return (s2)

def get_thread_id():
	"""Return the name of the current thread 
	"""
	try:
		curr = threading.currentThread()
	except Exception:
		curr = None
	if (curr == None):
		return "[noThread]"
	try:
		threadName = curr.getName()
	except Exception:
		return ("[%03d]" % (thread.get_ident(), ))
	return threadName

class TxLog:
	def __init__(self, logFileName, serverName):
		self.debugTraceLevel = 1
		self.origFileName = logFileName
		self.srvName      = serverName
		self.semWriteLog  = threading.Semaphore()
		self.setFileName(logFileName, "a")

	def setServerName(self, serverName):
		self.srvName = serverName

	def writeConsole(self, str):			# lowest level
		"""Private -- lowest level function. Writes to stderr
		"""
		sys.stderr.write(str + "\n")
		sys.stderr.flush()

	def writeLogLineRaw(self, str):		#lowest level
		"""Private -- lowest level function. open/write/close.
			Writes to stdout if cannot write to self.fileName
		"""
		rc = 0
		self.semWriteLog.acquire()	# blocking mode SEMAPHORE
		if (self.fileName):
			try:
				f = open(self.fileName, "a")
			except IOError:
				f = None			# if open fails, will write to stdout
		else:
			f = None

		b_done = 0
		if (f):
			try:
				f.write(str)
				f.close()
				f = None
				b_done = 1
			except:	
				pass	# cannot do anything here
			if (f):
				f.close()
				f = None
		if (not b_done):
			try:		# then, try to write on stdout...
				sys.stdout.write(str)
				sys.stdout.flush()
			except IOError:
				pass	   # we cannot write, but, should not crash!
				rc = -2	   # maybe will be checked, maybe not.

		self.semWriteLog.release()		# release SEMAPHORE
		return rc


	## unused methods:
	#
	##def closeIfNotStdout(self, fd):	# lowest level
	##	return			# nothing to do
	##
	##def resetToStdoutPriv(self):# private -- recovery method
	##	self.fileName = None
	##
	##def resetToStdout(self):		# private -- recovery method
	##	self.resetToStdoutPriv()
	##	self.writeConsole("WARNING ### Log reset to sys.stdout ###")
	##
	##def reOpenFile(self, fName, fMode="a"):
	##	self.setFileName(fName, fMode)
		
	def setFileName(self, fName, fMode="a"):
		""" Called by constructor, by __init__()
			NOTE: fMode is now ignored. It is always 'a'.
	  	"""
		self.writeConsole("MSG: setting log file to: " + fName)
		self.fileName = fName


	def writeLog(self, modName, level, msgNo, msg):
		"""writeLog()
			params: 	modName	identifier of module (string, 10 chars) 
					level   0:Msg, 1:Debug, 2:Warning, 3:ERROR
					msgNo	an int (0..99999)
					msg	a string
		  The msg is written to the log file with the format:
			    YYYY-MM-DD HH:MM:SS.mmm;Level;servName;<pid>;threadId;
	  		msgNo;moduleName, msg...\n
		  NOTE: each thread should do setName() with a name of at most
	        	12 characters.
		"""
		try:		## does not call get_thread_id():
			threadName = threading.currentThread().getName()
		except:
			threadName = "[unknownThread]"

		timeStr = get_log_time()
		levelStr = MsgLevels.get(level, "levl?")  # like get_msg_level(level)

		aLine = ("%s;%-5.5s;%10.15s;%5d;%15.25s;%5d;%-14.14s, %s\n" % 
		 (timeStr, levelStr, self.srvName, 
		  os.getpid(),
		  threadName,
		  msgNo, 
		  modName, 
		  msg) )
		return self.writeLogLineRaw(aLine)



	def writeLogSLOW(self, modName, level, msgNo, msg):
		#
		# SLOWER version!
		#
		try:		## does not call get_thread_id():
			threadName = threading.currentThread().getName()
		except:
			threadName = "[unknownThread]"

		timeStr = get_log_time()
		levelStr = MsgLevels.get(level, "levl?")  # get_msg_level(level)

		levelStr = get_msg_level(level)		## SLOWER
		threadName = get_thread_id()		## SLOWER

		aLine = ("%s;%-5.5s;%10.15s;%5d;%15.25s;%5d;%-14.14s, %s\n" % 
		 (timeStr, levelStr, self.srvName, 
		  os.getpid(),
		  threadName,
		  msgNo, 
		  modName, 
		  msg) )
		return self.writeLogLineRaw(aLine)



if __name__ == '__main__':

	log = TxLog("bogus/test.log", "server1")
	log.writeLog("Module1", 0, 12345, "msg ordinaire")
	log.writeLog("Module2", 1, 66600, "msg debug (1)")
	log.writeLog("Module2", 2, 7777, "warning (2)")
	log.writeLog("Module2", 3, 44455, "Error (3)")
	log.writeLog("ModuleXX", 9, 9999, "level = 9")

	log.writeLog("Module1", L_MSG, 1, "before sleep()...")
	# print "will sleep 10 seconds..."
	# time.sleep(10)

	rc = log.writeLog("Module1", L_MSG, 2, "after sleep()...")
	m1 = "rc=%s\n" % (rc, )
	sys.stderr.write(m1)

	sys.exit()

	# max_iter = 2001
	max_iter = 10

	t_begin = time.time()
	for i in xrange(0, max_iter):
		log.writeLog("Module1", L_MSG, i, "msg xxx.")

	t_now = time.time()
	m1 = "%s writeLog(): %5.3f seconds elapsed" % (i, t_now - t_begin, )
	log.writeLog("Module1", L_MSG, 0, m1)


	t_begin = time.time()
	for i in xrange(0, max_iter):
		log.writeLogSLOW("Module1", L_MSG, i, "msg xxx.")

	t_now = time.time()
	m1 = "%s writeLogSLOW(): %5.3f seconds elapsed" % (i, t_now - t_begin, )
	log.writeLog("Module1", L_MSG, 0, m1)



