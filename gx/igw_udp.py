#!/usr/bin/python
#
# igw_udp.py -- Interface GateWay UDP module
# ----------
#
# $Source: /ext_hd/cvs/gx/igw_udp.py,v $
# $Header: /ext_hd/cvs/gx/igw_udp.py,v 1.3 2004/11/05 16:27:42 toucheje Exp $
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
# medium-high level functions and objects to send UDP datagram to various 
# Interface Gateways (TICS, OMS, FundsOE, ...).
#
#-------------------------------------------------------------------------
# 2002apr04-06,jft: first version
# 2002apr07,jft: + constants UDP_CMD_EOD, UDP_CMD_INACTIVATE
# 2002apr09,jft: . server_test(): try to display data a bit better
# 2002apr11,jft: + OMS_UDP_CMD_NB_FILLS
# 2004nov05,jft: . igw_udp.sendDatagram(): self.sd.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
#

import sys
import struct
import socket

import gxutils			# contains formatExceptionInfoAsString()
import TxLog			# for get_log_time()


#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header: /ext_hd/cvs/gx/igw_udp.py,v 1.3 2004/11/05 16:27:42 toucheje Exp $"

#
# --- Module Constants:
#
MODULE_NAME = "igw_udp"


#
# Public CONSTANTS:
#

#*
#* Values of field cmd_type:	(from oms_udp.h)
#*
UDP_CMD_NB_FILLS		= 'F'		# OMS_UDP_CMD_NB_FILLS in oms_udp.h


#* Values of field cmd_type:	(from osggqudp.h)
#*
UDP_CMD_HEARTBEAT		= 'B'		# OSGGQUDP_CMD_HEARTBEAT in osggqudp.h
UDP_CMD_NEW_REQUEST_AVAIL	= 'N'		# OSGGQUDP_CMD_NEW_REQUEST_AVAIL
UDP_CMD_NEW_ROW_AVAILABLE	= 'N'		# synonym of UDP_CMD_NEW_REQUEST_AVAIL
UDP_CMD_SHUTDOWN		= 'S'		# OSGGQUDP_CMD_SHUTDOWN
UDP_CMD_RESET_LINES		= 'R'		# OSGGQUDP_CMD_RESET_LINES
UDP_CMD_SET_TRACE		= 'T'		# OSGGQUDP_CMD_SET_TRACE
						# * Note: clashes with DTBB_UDP_MSG_TYPE_NEW_TRADE
UDP_CMD_WRITE_DEBUG_INFO	= 'D'		# OSGGQUDP_CMD_WRITE_DEBUG_INFO

UDP_CMD_OPEN			= 'O'		# OSGGQUDP_CMD_OPEN  or  OMS_UDP_CMD_OPEN
UDP_CMD_CLOSE			= 'C'		# OSGGQUDP_CMD_CLOSE or  OMS_UDP_CMD_CLOSE

OMS_UDP_CMD_NB_FILLS		= 'F'		# from oms_udp.h




#* Values of field cmd_type:	(from dtbb_udp.h)
#*
UDP_CMD_EOD			= 'E'		# DTBB_UDP_OPER_CMD_EOD
UDP_CMD_INACTIVATE		= 'I'		# DTBB_UDP_OPER_CMD_INACTIVATE



UDP_OPT_TRACE_NONE		= '0'		# OSGGQUDP_OPT_TRACE_NONE
UDP_OPT_TRACE_LOW		= '1'		# OSGGQUDP_OPT_TRACE_LOW
UDP_OPT_TRACE_MEDIUM		= '5'		# OSGGQUDP_OPT_TRACE_MEDIUM
UDP_OPT_TRACE_FULL		= '9'		# OSGGQUDP_OPT_TRACE_FULL
UDP_OPT_SHUTDOWN_SLOW		= 'S'		# OSGGQUDP_OPT_SHUTDOWN_SLOW
UDP_OPT_SHUTDOWN_FAST		= 'F'		# OSGGQUDP_OPT_SHUTDOWN_FAST




# srvlog must be initialized by the main module
#
srvlog = None

## srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "...")


def convertServiceNameToPortNo(portNo, protocolName="udp"):
	#
	# if the first char of the string portNo is a digit,
	# try to convert it to an int.
	#
	# if it is not a digit, or if the conversion failed,
	# look up the service name that match this string (portNo),
	# and, return it.
	#
	if (not portNo  or  type(portNo) == type(1) ):
		return portNo

	if (portNo[0].isdigit() ):
		try:
			n = int(portNo)
			return (n)
		except:
			pass

	return (socket.getservbyname(portNo, protocolName)  )




class igw_udp:
	#
	# NOTES: the format of the message sent by 
	#        is a re-implementation of the struct @@@ defined in osggqudp.h (2000july12).
	#
	#	 The following sizes are used, like these "C" defines:
	#		#define OSGGQUDP_TIMESTAMP_STR_SIZE		55
	#		#define OSGGQUDP_SERVER_NAME_SIZE		32
	#		#define OSGGQUDP_OPTION_STR_SIZE		50
	#		#define OSGGQUDP_FILLER_SIZE			100

	#
	def __init__(self, gwHostname, portNo, origin_server_name):
		self.hostname = gwHostname	# hostname where IGW runs

		## print "gwHostname:", gwHostname, "portNo:", portNo, origin_server_name

		self.portNo   = convertServiceNameToPortNo(portNo, "udp")

		self.origin_server_name = origin_server_name

		self.sd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


	def sendDatagram(self, msg, b_broadcast=1):
		rc = 0
		if b_broadcast:
			try:
				self.sd.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
			except:
				rc = 1

		nBytes = self.sd.sendto(msg, (self.hostname, self.portNo) )
		#
		# nBytes should be equal to len(msg)
		#
		return rc

	def sendUDPcmdWithOptions(self, cmd_char, option_char, options_str):
		timeStr = TxLog.get_log_time()

		msg = struct.pack("cc55s32s50s100s", cmd_char, option_char, 
							timeStr,
							self.origin_server_name,
							options_str,
							"[filler]")

		return (self.sendDatagram(msg, b_broadcast=0) )



if __name__ == '__main__':
	def print_usage():
		print 'Usage:    igw_udp.py  { server | client}  port_no  [ "client_msg_string" ]'
		print "example:  igw_udp.py   server  7799"
		print 'example:  igw_udp.py   client  7799  "Hello, world!" '
		print 'example:  igw_udp.py   client  7799  CMD  cmd_char, option_char, options_str'

	def server_test(portNo):
		portNo = convertServiceNameToPortNo(portNo, "udp")
		s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		print "portNo:", portNo
		try:
			s.bind( ("", portNo) )
			print "UDP server started on port:", portNo, "..."
			while (1):
				data, address = s.recvfrom(500)
				print "received from:", address, "data:", repr(data), "[EndData]."

				if (data == "/stop"):
					break
		except:
			ex = gxutils.formatExceptionInfoAsString(10)
			print "Server aborted, exception:", ex
		print "*** UDP Server stopping. ***"			
		raise SystemExit

	def client_test(portNo, msgString):
		clt = igw_udp("localhost", portNo, "UnitTestClient/" + MODULE_NAME)
		clt.sendDatagram(msgString)

		if (msgString.lower() == "cmd"):
			if (len(sys.argv) < 7):
				print "Missing argument(s)!"
				print 'example:  igw_udp.py   client  7799  CMD  cmd_char, option_char, options_str'
				return
			clt.sendUDPcmdWithOptions(sys.argv[4], sys.argv[5], sys.argv[6])

	if (len(sys.argv) >= 2 and sys.argv[1] == "broadcast"):
		udp = igw_udp("localhost", 20000, __name__+" test broadcast")
		rc = udp.sendDatagram("allo test")
		print "rc=", rc
		raise SystemExit


	if (len(sys.argv) < 3):
		print_usage()
		raise SystemExit


	if (sys.argv[1] == "client" and len(sys.argv) < 4):
		print_usage()
		raise SystemExit

	if (sys.argv[1] == "server"):
		server_test(sys.argv[2] )
		print "*** UDP Server stopping. ***"
		raise SystemExit

	if (sys.argv[1] == "client"):
		client_test(sys.argv[2], sys.argv[3] )
		raise SystemExit

	print "Unknown mode:", sys.argv[2]
	print
	print_usage()
