#!/usr/bin/python

#
# Cfg.py :  Config file class and utility functions
# --------------------------------------------------
#
#
# $Source: /ext_hd/cvs/gx/Cfg.py,v $
# $Header: /ext_hd/cvs/gx/Cfg.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $
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
# Cfg is an object which can load a config file in memory and then allows 
# to retrieve parameters values.
#
#
# The syntax of a config file to be read by a Cfg object is:
#
#	# this is a comment...
#	#
#	# NOTES: a sectionName begins on the extreme left side of a line.
#	#	 all paramNames begin after an initial TAB character.
#	#	 values are separated by whitespace (TAB or SPACE).
#	#	 values can continue on the next line if the current line
#	#	 ends with a '\' character.
#	#	 Anything that follows a '#' is a comment.
#	#	 Empty lines are ignored.
#	#	 A section ends with the next sectionName, or at end of file.
#	#
#	section_A
#		paramName_1	val_1	val_2	val_3	val_4	val_4
#
#		paramName_2	val_1	\
#				val_2	\ # comment after a \
#				val_3
#
#	section_B
#		param_1		val_1...
#	
#
# -----------------------------------------------------------------------
#
# 2002jan19,jft: first version
# 2002jan29,jft: + added standard trace functions
# 2002feb20,jft: . ParamVal.getNextValue(), ParamVal.getFirstValue(): try to be more thread-safe
# 2002apr16,jft: + Cfg.getListOfUnusedParams()
# 2002apr17,jft: + support multiple section names on one (1) line
# 2002may20,jft: . Cfg.__init__(): self.optPrefix = optPrefix
#


import sys

#
# CONSTANTS
#
TAB_CHAR	  = '\t'
COMMENT_CHAR	  = '#'
CONTINUATION_CHAR = '\\'

A_QUOTE		  = '"'
EMPTY_STRING	  = '""'


#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/Cfg.py,v 1.2 2004/06/08 14:25:54 sawanai Exp $"



# module variables:
#
debug_level = 0


# Module functions:
#
def assignDebugLevel(val):
	global debug_level
	debug_level = val

def getDebugLevel():
	return (debug_level)


def getRcsVersionId():
	return (RCS_VERSION_ID)


#
# Classes: ParamVal, Cfg
#
class ParamVal:
	def __init__(self, firstVal, filename, section, lineNo):
		self.values  = [firstVal, ]
		self.iterPos = 0
		self.ctrGet  = 0
		self.filename = filename
		self.section  = section
		self.lineNo   = lineNo

	def append(self, val):
		self.values.append(val)


	def getFirstValue(self):
		if (len(self.values) <= 0 ):
			return (None)

		self.iterPos = 1

		return (self.values[0] )


	def getNextValue(self):
		#
		# WARNING: this implementation is not thread safe and it can cause the
		#	   exception:  IndexError, ('list index out of range',)
		#
		# The current version of this method tries to beat the odds,
		# but precautions should be taken to NOT use this method under
		# multi-thread conditions.
		#
		i = self.iterPos	# copy it before cheking the value

		if (self.iterPos >= len(self.values) ):
			return (None)

		self.iterPos = self.iterPos + 1

		return (self.values[i])

	def getList(self):
		return (self.values[:])	# return a shallow copy

	def isValInValuesList(self, val):
		return (val in self.values)



class Cfg:
	def __init__(self, name="config",  filename=None,  section=None, 
			   optPrefix=None, b_allowRedefine=0):
		self.name       = name
		self.filename   = filename
		self.section    = section
		self.b_allowRedefine = b_allowRedefine
		#
		# NOTE: if optPrefix is NOT None, getParamFirstValue() and
		#	getParamNextValue() will try to prefix "<optPrefix>."
		#	in front of the paramName in a first try.
		#	If such a param exists, its values will be used.
		#	If not, these functions will try again with the plain
		#	paramName.
		#
		self.optPrefix   = optPrefix
		#
		# instance variables with initial None values:
		#
		self.paramsDict    = None
		self.lastParamRead = None
		self.lastError     = None
		self.lastProgressMsg = None
		self.lineNo	   = -1
		self.doneOK = None

	def loadConfig(self, section=None, filename=None, b_allowRedefine=None):
		#
		# The caller must be ready to handle various kinds of exception
		# if the file does not exist, if the section is not found,
		# or whatever...
		#
		self.lineNo = 0
		self.doneOK = 0

		if (self.paramsDict == None):
			self.paramsDict = {}

		if (not section):
			section = self.section
		else:
			self.section = section

		if (not section):
			raise RuntimeError, "section cannot be None or empty"

		if (not filename):
			filename = self.filename
		else:
			self.filename = filename
		if (not filename):
			raise RuntimeError, "filename cannot be None or empty"

		if (b_allowRedefine == None):
			b_allowRedefine = self.b_allowRedefine


		fi = self.openFileAndFindSection(filename, section)

		self.loadParamsInCurrentSection(fi, b_allowRedefine)

		self.doneOK = 1

				
	def openFileAndFindSection(self, filename, section):
		#
		# if open() fails, it will raise an exception.
		#
		fi = open(filename)

		while(1):
			aLine = self.getNonEmptyLine(fi)
			if (not aLine):
				fi.close()
				m1 = "Section '%s' not found, EOF on '%s'." % (section, filename)
				raise RuntimeError, m1

			if (TAB_CHAR == aLine[0]):
				continue

			words = aLine.split()

			if (not words):
				self.lastError = "aLine.split() is None or []"
				continue

			if (section in words):
				self.lastProgressMsg = ("found section: " + section)
				return (fi)		# found section

	def getNonEmptyLine(self, fi):
		while (1):
			aLine = fi.readline()
			if (not aLine):
				return None

			self.lineNo = self.lineNo + 1

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

			return (aLine)		# return un-modified line

	def convertToNumberIfPossible(self, val):
		#
		# if the first character is a digit, try to convert value to a number.
		# But, be careful if it is an IP address, as it should be kept as string!
		#
		try:
			if (len(val) >= 1 and val[0].isdigit() ):
				n =  val.count(".")

				if (n == 0):
					return (long(val) )
				if (n == 1):
					return (float(val) )
				return (val)	# must be a "nn.nn.nn.nn" IP address

			return (val)		# must be a string
		except ValueError:
			return (val)
		except:
			return (val)

	def assignParamValue(self, param, val, b_nextValue, b_allowRedefine):
		val = self.convertToNumberIfPossible(val)
		try:
			values = self.paramsDict[param]
			if (not b_nextValue and not b_allowRedefine):
				m1 = "Illegal redefinition of '%s' as '%s' at %s" % (param, val, self.lineNo)
				raise RuntimeError, m1

			if (b_nextValue):
				values.append(val)
			else:
				# remember previous parameter definition:
				#
				prevParamName = "%s.Previous!" % (param, )
				self.paramsDict[prevParamName] = values

				lineNo = self.lineNo + 0

				filename = "%s" % (self.filename, )
				section  = "%s" % (self.section,  )

				values = ParamVal(val, filename, section, lineNo)

				self.paramsDict[param] = values
		except KeyError:
			lineNo = self.lineNo + 0

			filename = "%s" % (self.filename, )
			section  = "%s" % (self.section,  )

			values = ParamVal(val, filename, section, lineNo)

			self.paramsDict[param] = values


	def getQuotedValues(self, s, param, aLine):
		#
		# This is heuristic parsing, not a real lexer or anything fancy.
 
		# Skip the parameter name (param), if there is one:
		#
		if (param != None):
			s = s[len(param):]

		##print "line:", self.lineNo, "param:", param, "s:", s, "aLine:", aLine

		# Trim whatever follows the comment char "#".
		#
		# NOTE: the algorithm here is naive. It will cut a line
		#	after "#", even if the # appears within 2 quotes!
		#	This will probably a "mismatch quote char" exception
		#	later (see below).
		#
		i = s.find(COMMENT_CHAR)
		if (i >= 0):
			s = s[:i]

		# Trim whatever follows the continuation char "\".
		# If one is found, remember about it.
		# Later, a "\" value will be appended to the list which is
		# returned.
		#
		i = s.find(CONTINUATION_CHAR)
		if (i >= 0):
			s = s[:i]
			continuation_value = CONTINUATION_CHAR
		else:
			continuation_value = None

		# strip() leading and trailing whitespaces:
		#
		s = s.strip()

		# split() s into values (strings) at each QUOTE:
		#
		vals = s.split(A_QUOTE)


		# If the number of values is an even number, then there is
		# a mismatch on opening/closing quote chars:
		#
		if ( (len(vals) % 2) == 0):
			m1 = "Mismatch quote char, at %d: '%s'" % (self.lineNo, aLine)
			raise RuntimeError, m1

		# Only keep the strings within the "xx", not the empty ones outside:
		#
		values = []
		for v in vals:
			if (v.strip() != ""):
				values.append(v)

		# If no values remain, then, there must have been at least one 
		# empty "" value, create one:		
		#
		if (len(values) == 0):
			values = [ '""', ]

		# If a continuation char was found previously, append it now:
		#
		if (continuation_value):
			values.append(continuation_value)

		return (values)


	def loadParamsInCurrentSection(self, fi, b_allowRedefine):

		is_continuation = 0

		while(1):
			aLine = self.getNonEmptyLine(fi)
			if (not aLine):
				fi.close()
				self.lastProgressMsg = "EOF on '%s'." % (self.filename, )
				return (0)

			if (len(aLine) <= 0):
				continue

			if (aLine[0] != TAB_CHAR and aLine[0] != ' '):
				fi.close()
				self.lastProgressMsg = "found end of section."
				return (0)
			
			s = aLine.strip()	# remove leading and trailing whitespace

			if (len(s) <= 0):
				continue

			words = s.split()	# separate line in "words"

			if (len(words) <= 0):
				continue

			### print "words:", words, "aLine:", aLine

			if (is_continuation):
				values = words
			else:
				if (len(words) < 2):
					m1 = "Invalid param def: no value, at %d: '%s'" % (self.lineNo, aLine)
					raise RuntimeError, m1

				if (words[1] == COMMENT_CHAR or words[1] == CONTINUATION_CHAR):
					m1 = "Invalid param def: comment or continuation without value, at %d: '%s'" % (self.lineNo, aLine)
					raise RuntimeError, m1

				param  = words[0]
				values = words[1:]

			if (s.find(A_QUOTE) >= 0):
				if (is_continuation):
					toSkip = None
				else:
					toSkip = param
				values = self.getQuotedValues(s, toSkip, aLine)

			found_continuation = 0
			breakAfterThisOne = 0		# to handle '\' glued to the right of a value

			ctr = 0
			for val in values:
				if (val[0] == CONTINUATION_CHAR):
					found_continuation = 1
					break
				if (val[0] == COMMENT_CHAR):
					break

				# Sometimes, a '\' or '#' is glued to the right of a value.
				# The boolean variable breakAfterThisOne and the following
				# checks take care of that case.
				#
				chunks = val.split(CONTINUATION_CHAR)
				if (len(chunks) >= 2):
					breakAfterThisOne = 1
					found_continuation = 1
					val = chunks[0]
				else:
					chunks = val.split(COMMENT_CHAR)
					if (len(chunks) >= 2):
						breakAfterThisOne = 1
						val = chunks[0]

				if (val == EMPTY_STRING):
					val = ""

				ctr = ctr + 1
				if (is_continuation or ctr > 1):
					b_nextValue = 1
				else:
					b_nextValue = 0
				self.assignParamValue(param, val, b_nextValue, b_allowRedefine)

				if (breakAfterThisOne):
					break

			is_continuation = found_continuation 
		#
		# End of loop
		#
		return (0)


	def assignOptPrefix(self, prefix):
		#
		# see NOTE for getParamVal()
		#
		self.optPrefix = prefix

	def getParamVal(self, paramName):
		#
		# WARNING: This is a low-level function and it should NOT be
		# -------  be called directly.
		#	   Your application should use getParamFirstValue(),
		#	   getParamNextValue(), or, isValInParamValuesList()
		#	   instead.
		#
		# NOTE: if self.optPrefix is NOT None, getParamVal()
		#	will try to prefix "<optPrefix>."
		#	in front of the paramName in a first try.
		#
		#	If such a param exists, its values will be used.
		#
		#	If not, getParamVal() will try again with the plain
		#	paramName.
		#
		if (self.optPrefix):
			prefixedName = self.optPrefix + "." + paramName
			try:
				val = self.paramsDict[prefixedName]
				val.ctrGet = val.ctrGet + 1
				return (val)
			except KeyError:
				pass

		try:
			val = self.paramsDict[paramName]
			val.ctrGet = val.ctrGet + 1
			return (val)
		except KeyError:
			return (None)

	def getParamFirstValue(self, paramName, defaultValue=None):
		#
		# NOTE: if self.optPrefix is NOT None, getParamFirstValue() and
		#	getParamNextValue() will try to prefix "<optPrefix>."
		#	in front of the paramName in a first try.
		#
		#	If such a param exists, its values will be used.
		#
		#	If not, these functions will try again with the plain
		#	paramName.
		#
		# NOTE-2: if defaultValue is provided and there is no such
		#	  parameter named paramName in the config, defaultValue
		#	  is returned.
		#
		pVal = self.getParamVal(paramName)
		if (pVal == None):
			return (defaultValue)

		return (pVal.getFirstValue() )

	def getParamNextValue(self, paramName):
		#
		# NOTES: (1) see NOTE for getParamFirstValue().
		# 	 (2) do NOT expect consistent results if getParamNextValue()
		#	     is used by multiple threads for the same paramName.
		#
		pVal = self.getParamVal(paramName)
		if (pVal == None):
			return None

		return (pVal.getNextValue() )


	def isValInParamValuesList(self, str, paramName):
		#
		# NOTES: (1) see NOTE for getParamFirstValue().
		# 	 (2) this function is thread-safe (so far as values list
		#	     don't change after the config is loaded!)
		#
		pVal = self.getParamVal(paramName)
		if (pVal == None):
			return (0)	# false

		return (pVal.isValInValuesList(str) )

	def dumpAllParamsValues(self):
		lsKeys = self.paramsDict.keys()
		lsKeys.sort()
		for k in lsKeys:
			print k, ":", self.paramsDict[k].getList()

	def getListOfUnusedParams(self):
		#
		# NOTE: for the most accurate results, this function should be called
		#	when the program is about to shutdown.
		#	Because parameters can be used dynamically, all the while when
		#	the program is running, not only during its init phase.
		#
		# Return a list of unused parameters, with the filename, section, lineNo
		#
		# The tuples in the list are made so that, after formatting in a text file,
		# that file will be easy to sort, and, then, easy to read by a human. 
		#
		# (This is to help in editing old config files with obsolete entries).
		#
		listUnused = []

		lsKeys = self.paramsDict.keys()
		lsKeys.sort()

		for name in lsKeys:
			paramVal = self.paramsDict[name]

			if (paramVal.ctrGet > 0):
				continue

			tup = (paramVal.filename, paramVal.section, paramVal.lineNo, name)

			listUnused.append(tup)

		return listUnused

	def writeListOfUnusedParamsToFileObj(self, fileObj):
		ls = self.getListOfUnusedParams()

		str = "List of unused params has %s entries. Params Dict size: %s\n" % (len(ls), len(self.paramsDict) )

		fileObj.write(str)

		for x in ls:
			str = "%20.20s %10.10s %05d %s\n" % x

			fileObj.write(str)


		str = "[End of List]\n"

		fileObj.write(str)


if __name__ == '__main__':
	print getRcsVersionId()
	print

	cfg = Cfg("TestConfig", "gxsrvcfg", "TRG")

	cfg.loadConfig(section="BASE")

	print "lineNo:", cfg.lineNo, " lastError:", cfg.lastError
	print "lastProgressMsg: ", cfg.lastProgressMsg

	cfg.loadConfig(section="CIG", b_allowRedefine=1)

	cfg.loadConfig(section="TRG", b_allowRedefine=1)

	cfg.loadConfig(filename="gxsrvcfg.naga08", section="NAGACG80", b_allowRedefine=1)
	cfg.loadConfig(filename="gxsrvcfg.naga08", section="NAGATR80", b_allowRedefine=1)
	print

	values = cfg.getParamVal("ACCEPTABLE_APPL_NAME")
	print values

	x = cfg.getParamFirstValue("ACCEPTABLE_APPL_NAME")
	while (x != None):
		print x
		x = cfg.getParamNextValue("ACCEPTABLE_APPL_NAME")

	print "---------"

	print len(cfg.paramsDict), "params found."
	print

	print "SERVER_LISTENPORT", cfg.getParamNextValue("SERVER_LISTENPORT")
	print

	val_dest = cfg.getParamVal("DESTINATIONS")
	ls_dest = val_dest.getList()

	val_groups = cfg.getParamVal("GROUPS_OF_RPC")
	ls_groups = val_groups.getList()

	print "DESTINATIONS:", ls_dest
	print "GROUPS_OF_RPC:", ls_groups
	print


	fileObj = open("Test_ListUnused_params.txt", "w")

	cfg.writeListOfUnusedParamsToFileObj(fileObj)

	sys.exit()



	ls = cfg.getListOfUnusedParams()

	print "List of unused params has", len(ls), "entries."

	for x in ls:
		str = "%20.20s %10.10s %05d %s" % x
		print str

	print "[End of List]"

	sys.exit()

#	print "groups NOT in list of DESTINATIONS:"
#	for g in ls_groups:
#		if (not g in ls_dest):
#			print "group", g
#	print
#	print "Dest NOT in GROUPS_OF_RPC :"
#	for d in ls_dest:
#		if (not d in ls_groups):
#			print "Dest", d
#
#	print

	for k in cfg.paramsDict.keys():
		v = cfg.getParamVal(k)
		if (v.ctrGet > 1):
			print k, "was requested", (v.ctrGet - 1), "times."

	print "*** All param values:"

	cfg.dumpAllParamsValues()

	print "*** DONE ***"

	raise SystemExit

###
	print "TAB_CHAR=%s." % (TAB_CHAR, )
	print "TAB_CHAR==<TAB> ?", (TAB_CHAR == "	")

	print "A_QUOTE", A_QUOTE  
	print "EMPTY_STRING", EMPTY_STRING 

	print "COMMENT_CHAR", COMMENT_CHAR 
	print "CONTINUATION_CHAR", CONTINUATION_CHAR 
