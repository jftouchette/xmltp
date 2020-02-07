/****************************************************************************
 * recov_t3.c 
 *
 *  Copyright (c) 1997-2001 Jocelyn Blanchet
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  (The file "COPYING" or "LICENSE" in a directory above this source file
 *  should contain a copy of the GNU Library General Public License text).
 *  -------------------------------------------------------------------------
 *
 * $Source: /ext_hd/cvs/ct10api/recov_t3.c,v $
 * $Header: /ext_hd/cvs/ct10api/recov_t3.c,v 1.3 2004/10/13 18:23:28 blanchjj Exp $
 *
 *  Standard default recovery that must be at the end of the recovery table.
 *  These entries will trap any error that was not trap before and force
 *  the program to stop.
 *
 *
 * 1997-08-15,jbl:	Initial creation.
 */


/*********************************************************************
 The next recovery entries must alway be at the end of the table in
 order to catch any error with no recovery.
 *********************************************************************/

{
/* CT10_ERR_FAILED */
CTAPRECO_FNC_ALL,
CT10_ERR_FAILED,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_SKIP_RPC,
"ct10api CT10_ERR_FAILED with no recovery, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* CT10_ERR_DO_NOT_RETRY */
CTAPRECO_FNC_ALL,
CT10_ERR_DO_NOT_RETRY,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_SKIP_RPC,
"ct10api CT10_ERR_DO_NOT_RETRY with no recovery, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* CT10_ERR_INTERVAL */
CTAPRECO_FNC_ALL,
CT10_ERR_INTERVAL,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_SKIP_RPC,
"ct10api CT10_ERR_INTERVAL with no recovery, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* CT10_ERR_FATAL_ERROR */
CTAPRECO_FNC_ALL,
CT10_ERR_FATAL_ERROR,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_SKIP_RPC,
"ct10api CT10_ERR_FATAL_ERROR with no recovery, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* CT10_ERR_RPC_PARSING_ERROR */
CTAPRECO_FNC_ALL,
CT10_ERR_RPC_PARSING_ERROR,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_SKIP_RPC,
"ct10api CT10_ERR_RPC_PARSING_ERROR with no recovery, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* CT10_ERR_NULL_ARG */
CTAPRECO_FNC_ALL,
CT10_ERR_NULL_ARG,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_SKIP_RPC,
"ct10api CT10_ERR_NULL_ARG with no recovery, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* CT10_ERR_TOO_MANY_FATALS_ABORTED */
CTAPRECO_FNC_ALL,
CT10_ERR_TOO_MANY_FATALS_ABORTED,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_EXIT_PROGRAM,
"ct10api CT10_ERR_TOO_MANY_FATALS_ABORTED with no recovery, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* CT10_ERR_BAD_CONN_HANDLE */
CTAPRECO_FNC_ALL,
CT10_ERR_BAD_CONN_HANDLE,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_EXIT_PROGRAM,
"ct10api CT10_ERR_BAD_CONN_HANDLE with no recovery, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
}


/* ---------------------- End of recov_t3.c -------------------------------- */
