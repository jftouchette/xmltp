#!/bin/sh
#
# start-up shell script for: XMLTP_RRG
#
# NOTE: you must have installed Python from source in its usual directory, under /usr/local
#	The Python executable will then be: /usr/local/bin/python
#


# get the env var: PYTHONPATH  XMLTP
#
. pythonpath.sh

nohup /usr/local/bin/python rrgx.py XMLTP_RRG gxconfig  ./gxserver.log &


