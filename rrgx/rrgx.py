#!/usr/bin/python
#
# rrgx.py -- RPC Router Gateway for XMLTP/L
# -------
#
# $Source$
# $Header$
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
# This script starts a "gxserver" as a "RRG" (see gxconfig for explanation
# about "types" of servers)/
#
# See "gxserver.py" for more detailed explanations and comments.
#
#-------------------------------------------------------------------------
# 2003jan24,jft: first version
#

import sys

import gxserver
import gxutils			# contains formatExceptionInfoAsString()
import TxLog


#
# Standard constants:
#
RCS_VERSION_ID = "RCS $Header$"

#
# --- Module Constants:
#
MODULE_NAME = "RRGX"


# --- Open the server LOG with path name given on command line 
# --- (if given, otherwise, use default filename).
#
#	NOTE: the name of the logfile does NOT come from the config file.
#
if (len(sys.argv) >= 4 and sys.argv[3].find(".log") > 0):
	logFilePath = sys.argv[3]
else:
	print "WARNING: logFilePath not given on command line. Using 'gxserver.log'..."
	logFilePath = "gxserver.log"

srvlog = TxLog.TxLog(logFilePath, "rrgx")
srvlog.writeLog(MODULE_NAME, TxLog.L_MSG, 0, "Loading rrgx.py...")

gxServer = None


if __name__ == '__main__':
    if (len(sys.argv) < 3):
	print "Usage:    rrgx.py  ServerName  configFile  logFilePath"
	print "example:  rrgx.py  XMDECG80    gxconfig    ./rrgx.log"
	raise SystemExit

    srvName = sys.argv[1]
    srvlog.setServerName(srvName)

    cfgFile = sys.argv[2]

    srvType = "RRG"

    try:
       gxserver.srvlog = srvlog		# absolutely required!

       srv = gxserver.gxserver(srvName, srvType=srvType, cfgFile=cfgFile, debugLevel=10)

       gxserver.runServer(srv)

    except:
        ex = gxutils.formatExceptionInfoAsString(10)

        m1 = "server %s FAILED. Exc: %s. srv=%s" % (srvName, ex, srv )
        srvlog.writeLog(MODULE_NAME, TxLog.L_ERROR, 0, m1 )

        print "server", srvName, "FAILED to start:", ex, "\nSee log for details."

    #
    #
    sys.exit()
