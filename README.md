# xmltp-light
XMLTP/L sample code for the article "XMLTP/L, XMLTP Light"
published by Jean-Francois Touchette
on March 25, 2003 in Linux Journal
https://www.linuxjournal.com/article/6743
NOTES: To build the .so libraries (using the makefile-s) and to run the code, you need:
(1) Python 2.3 compiled from source (you need PyConfig.h to compile the xmltp_gx.so module).
(2) Sybase Open Client Library from that year, 2003 or a bit earlier (cslib and ctlib version 10.x).
Since that time, Sybase has been acquired by SAP. 
Sybase Open Client Library is still available, but, a more recent version.
This old source code is 99.9% still compatible with the new version.
You will have to pay attention to the cs_version() library initialization call.
WARNING: This source code is obsolete and it is not currently used.
-------  Technology has moved on and we all write better code now, or we hope to do so :)
AUTHORS: Many source files were written by JF Touchette, but, SEVERAL SOURCE FILES HAVE BEEN WRITTEN BY OTHER PERSONS.
-------  Please see the author's name in the Copyright notice and comments at beginning of each file.
2020-2-07.
