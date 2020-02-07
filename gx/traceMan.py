#!/usr/bin/python

#
# traceMan.py	  trace Manager : assign and check trace levels in registered
# -----------			  objects and modules
#
# $Source: /ext_hd/cvs/gx/traceMan.py,v $
# $Header: /ext_hd/cvs/gx/traceMan.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $
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
# Each module or object which registers itself here with:
#
#	register(name,  object,  groupName=None)
#
#	   NOTE: to put a module or object in two or more groups, register() it again.
#
# must have the following methods or functions:
#
#	assignDebugLevel(val)
#	getDebugLevel()
#	getRcsVersionId()	which returns a string containing "RCS $Header: /ext_hd/cvs/gx/traceMan.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $" as 
#				expanded by the RCS "co" command.
#
# Once one or many modules or objects instances have been registered with register(),
# the following functions can be used:
#
#	assignTraceLevel(groupOrObjectName, level)
#						where <level> is one of these:
#						   0 or TRACE_LEVEL_NONE or "none"
#						   1 or TRACE_LEVEL_LOW  or "low"
#						   5 or TRACE_LEVEL_MEDIUM or "medium"
#						  10 or TRACE_LEVEL_FULL   or "full"
#						  <an Integer>
#						  <a String>
#
#	    NOTE: if groupOrObjectName is found as a registered groupName, all modules
#		  or objects associated with that groupName will be assigned the
#		  specified trace level.
#
#
#	getTraceLevel(groupOrObjectName)
#
#	    NOTE: if groupOrObjectName is found as a registered groupName, this function
#		  will return a list of integers (not an integer).
#
#
#	getVersionId(groupOrObjectName)
#
#	    NOTE: if groupOrObjectName is found as a registered groupName, this function
#		  will return a list of strings.
#
#
#	queryTrace()	return a list of tuples. Each tuple contains:
#
#					( groupName, name, traceLevel, versionId)
#
#
# ------------------------------------------------------------------------------
# 2002jan28,jft: first version
# 2002sep16,jft: + cleanVersionId()
# 2003jan17,jft: + class ObjectDebugLevelCapsule (see NOTES in there for details)
#

import gxutils		# for formatExceptionInfoAsString()


#
# CONSTANTS:
#
TRACE_LEVEL_NONE   = 0
TRACE_LEVEL_LOW    = 1
TRACE_LEVEL_MEDIUM = 5
TRACE_LEVEL_FULL   = 10


#
# module level variables:
#
groupsDict = {}
objNameDict = {}

lsGroups = []		# chrono order, unique group names only
lsNames  = []		# chrono order, unique names


traceLevelsDict = {  
	"none"   : TRACE_LEVEL_NONE,
	"low"    : TRACE_LEVEL_LOW,
	"medium" : TRACE_LEVEL_MEDIUM,
	"full"   : TRACE_LEVEL_FULL,
	}

#
# Public functions:
#
def register(name, obj, groupName=None):
	if (groupName):
		try:
			ls = groupsDict[groupName]
		except KeyError:
			lsGroups.append(groupName)
			ls = []
			groupsDict[groupName] = ls
		ls.append(name)

	if (not objNameDict.has_key(name) ):
		lsNames.append(name)

	objNameDict[name] = obj


def convertTraceLevelStringToNumber(level):
	#
	# Private. 
	#
	# Called by:	assignTraceLevel()
	#
	if (type(level) == type("s") ):
		s = level.lower()
		s = s.strip()
		try:
			return (traceLevelsDict[s] )

		except KeyError:
			if (len(s) >= 1 and s[0].isdigit() ):
				try:
					return (int(float(s) ) )
				except ValueError:
					return (level)
	return (level)



def getObjList(name):
	#
	# Private. 
	#
	# Called by:	assignTraceLevel()
	#		getTraceLevel()
	#		getVersionId()
	#
	# Return:
	#	[ list ]	list of objMod for that groupName, or a single entry list
	#			 if name is the name of a single objMod
	#	<string>	if no such groupName or objName
	#
	try:
		lsNames = groupsDict[name]
		ls = []
		for objName in lsNames:
			try:
				obj = objNameDict[objName]
				ls.append(obj)
			except KeyError:
				pass
			
		return (ls)
	except KeyError:
		try:
			obj = objNameDict[name]
			ls = [obj, ]
			return (ls)
		except KeyError:
			pass

	msg = "'%s' is not a registered groupName or objectName" % (name, )
	return (msg)



def assignTraceLevel(name, level, b_strict=0):
	#
	#
	#   NOTE: if <name> is found as a registered groupName, all modules
	#	  or objects associated with that groupName will be assigned the
	#	  specified trace level.
	#
	# Return:
	#	None	 if everything is OK
	#	errMsg	 if an error was found.
	#
	level = convertTraceLevelStringToNumber(level)	# "best effort" only!

	if (b_strict and type(level) != type(1) ):
		msg = "level '%s' is not an integer or a valid TRACE_LEVEL_xxx keyword" % (level, )
		return (msg)

	ls = getObjList(name)

	if (type(ls) == type("str") ):		# error found
		return (ls)

	msg = None

	for objMod in ls:
		try:
			objMod.assignDebugLevel(level)
		except AttributeError:
			print "DEBUG -- ls:", ls
			msg = "'%s' (%s) does not have a .assignDebugLevel() method" % (name, objMod)
		except:
			exc = gxutils.formatExceptionInfoAsString()
			msg = "%s .assignDebugLevel() caused exception: %s" % (name, exc)
			return (msg)

	return (msg)	# msg is None if all went OK.
		


def getTraceLevel(name):
	#
	#    NOTE: if groupOrObjectName is found as a registered groupName, this function
	#	  will return a list of integers (not an integer).
	#
	ls = getObjList(name)

	if (type(ls) == type("str") ):		# error found
		return (ls)

	msg = None
	replyLs = []
	try:
		for objMod in ls:
			try:
				level = objMod.getDebugLevel()
				replyLs.append(level)
			except AttributeError:
				msg = "'%s' (%s) does not have a .getDebugLevel() method" % (name, objMod)

		if (len(ls) == 1):
			return (replyLs[0])	# name was an objectName, not a groupName

		return (replyLs)
	except:
		exc = gxutils.formatExceptionInfoAsString()
		msg = "%s .getTraceLevel() caused exception: %s" % (name, exc)
		return (msg)


def getVersionId(name):
	#
	#    NOTE: if <name> is found as a registered groupName, this function
	#	  will return a list of strings.
	#
	ls = getObjList(name)

	if (type(ls) == type("str") ):		# error found
		return (ls)

	msg = None
	replyLs = []
	try:
		for objMod in ls:
			try:
				v = objMod.getRcsVersionId()
				replyLs.append(v)
			except AttributeError:
				msg = "'%s' (%s) does not have a .getRcsVersionId() method" % (name, objMod)

		if (len(ls) == 1):
			return (replyLs[0])	# name was an objectName, not a groupName

		return (replyLs)
	except:
		exc = gxutils.formatExceptionInfoAsString()
		msg = "%s .getVersionId() caused exception: %s" % (name, exc)
		return (msg)


def cleanVersionId(s):
	#
	# Remove the "RCS $Header: " at the beginning of a version id string.
	#
	if (not s):
		return s

	rcsHeaderStr = "RCS $Header: "

	pos = s.find(rcsHeaderStr)
	if (pos <= -1):
		return s

	pos = pos + len(rcsHeaderStr)

	return s[pos:]		# return what is after


def queryTrace():
	#
	# Return a LIST of TUPLES. Each tuple contains:
	#
	#		( groupName, name, traceLevel, versionId)
	#
	# return a string (an error msg) if fails.
	#
	namesDoneDict = {}

	replyLs = []

	for groupName in lsGroups:
		try:
			ls = groupsDict[groupName]

			for objName in ls:
				namesDoneDict[objName] = groupName	# remember was done
				level = getTraceLevel(objName)
				version = cleanVersionId( getVersionId(objName) )
				tup = (groupName, objName, level, version)
				replyLs.append(tup)
		except KeyError:
			tup = (groupName, "[Not Found]", 0, "")
			replyLs.append(tup)
		except:
			exc = gxutils.formatExceptionInfoAsString()
			msg = "queryTrace() failed when processing group %s. exception: %s" % (groupName, exc)
			return (msg)


	for objName in lsNames:
		try:
			if (not namesDoneDict.has_key(objName) ):
				level = getTraceLevel(objName)
				version = cleanVersionId( getVersionId(objName) )
				tup = ("", objName, level, version)

				replyLs.append(tup)
		except:
			exc = gxutils.formatExceptionInfoAsString()
			msg = "queryTrace() failed when processing group %s. exception: %s" % (objName, exc)
			return (msg)
		
	return (replyLs)



class ObjectDebugLevelCapsule:
	#
	# This class implements the (3) standard Debug methods.
	# It can be instantiated in some special cases. See NOTES here.
	#
	# NOTES:
	# There are 2 types of standard trace/debug technique:
	#	. methods of an object
	#	. functions of a module
	#
	# A main module cannot do: 
	#		traceMan("MainModuleName", __This_Module__)	# is there anything like __This_Module__ ??
	#
	# Also, a subclass of a major server class (such "gxserver"), cannot 
	# cleanly override the debug/trace of that superclass as this
	# would create lots of conflicts and confusion!
	#
	# USAGE NOTES:
	#
	# Your module should create an instance of this class and 
	# keep a reference in a module-level variable.
	# Also, the getDebugLevel() in your module should call 
	# the getDebugLevel() method of this object. Your code should 
	# look like this:
	#   import traceMan
	#   g_debugObj = traceMan.ObjectDebugLevelCapsule(RCS_VERSION_ID)
	#
	#   def getDebugLevel():
	#	return (g_debugObj.getDebugLevel() )
	#
	# If your module creates a subclass of gxserver, then, that subclass
	# will need to have this in its preInitializeCustom() method:
	#
	#	traceMan.register("myGXclass", g_debugObj, "MyServer")
	#
	#
	# (jft, 17jan2003)
	#
	def __init__(self, version_id):
		self.debugLevel = 0
		self.version_id = version_id

	def assignDebugLevel(self, val):	# this method will be called by traceMan
		self.debugLevel = val

	def getDebugLevel(self):		# this method will be called by traceMan AND your module
		return (self.debugLevel)

	def getRcsVersionId(self):		# this method will be called by traceMan
		return (self.version_id)






#
# UNIT TEST follows...
#
if __name__ == '__main__':
	class a:
		def __init__(self):
			self.debugLevel = 0

		def assignDebugLevel(self, val):
			self.debugLevel = val

		def getDebugLevel(self):
			return (self.debugLevel)

		def getRcsVersionId(self):
			str = "RCS version of %s" % (self, )
			return (str)
	class b(a):
		pass
	class c(a):
		pass

	a1 = a()
	a2 = a()
	b1 = b()
	c1 = c()
	c2 = c() 

	register("a1", a1)
	register("a2", a2, "abc")
	register("b1", b1, "abc")
	register("c1", c1, "abc")
	register("b1", b1, "bc")
	register("c2", c2, "bc")
	register("a", a)

	print 'lsGroups:', lsGroups
	print 'lsNames:', lsNames
		
	print 'assignTraceLevel("a1", 2):', assignTraceLevel("a1", 2)
	print 'assignTraceLevel("b1", 5):', assignTraceLevel("b1", 5)
	print 'assignTraceLevel("c2", 5):', assignTraceLevel("c2", 5)
	print 'assignTraceLevel("abc", TRACE_LEVEL_FULL):', assignTraceLevel("abc", TRACE_LEVEL_FULL)

	print 'getTraceLevel("c2"):', getTraceLevel("c2")

	print
	print "This will cause an exception, and, it will be handled..."
	print
	print 'getTraceLevel("a"):', getTraceLevel("a")

	print
	print 'register("a", a1):', register("a", a1)

	print 'queryTrace():'
	ls = queryTrace()
	for t in ls:
		print t

	c1.assignDebugLevel(333) 

#
# end of traceMan.py
# 
