/****************************************************************************
 * recov_t2.c
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
 * $Source: /ext_hd/cvs/ct10api/recov_t2.c,v $
 * $Header: /ext_hd/cvs/ct10api/recov_t2.c,v 1.4 2004/10/13 18:23:28 blanchjj Exp $
 *
 *   Here we place all recovery entries that must be handle first for 
 *   specific error or success state.
 *
 *
 * 1997-08-15,jbl:	Initial creation.
 */


{
/* Most probable case success all the way */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
CTAPRECO_RPC_RET_STA_SUCCESS, CTAPRECO_RPC_RET_STA_SUCCESS,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_SUCCESS,
"",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* Nothing to do is converte as a success */
CTAPRECO_FNC_ALL,
CT10_NOTHING_TO_DO,
CTAPRECO_RPC_RET_STA_SUCCESS, CTAPRECO_RPC_RET_STA_SUCCESS,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

CTAPRECO_COUNTER_NO_LIMIT,
0,

CTAPRECO_ACTION_SUCCESS,
"",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

/**************************************************************************
 Here we place any error specific recovery.
 **************************************************************************/

{
/* deadlock */
CTAPRECO_FNC_ALL,
CTAPRECO_CT10API_RC_ANY,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CT10_DIAG_DEADLOCK,
CTAPRECO_CT10_DIAG_EMPTY,

2,
0,

CTAPRECO_ACTION_RETRY_RPC,
"",
CTAPRECO_ACTION_EXIT_PROGRAM,
"To many deadlock on the same rpc, must stop",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* timeout */
CTAPRECO_FNC_ALL,
CTAPRECO_CT10API_RC_ANY,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CT10_DIAG_TIMEOUT,
CTAPRECO_CT10_DIAG_EMPTY,

4,
0,

CTAPRECO_ACTION_WAIT_SHORT|CTAPRECO_ACTION_RETRY_RPC,
"",
CTAPRECO_ACTION_EXIT_PROGRAM,
"To many timeout on the same rpc, must stop",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* dead connection */
CTAPRECO_FNC_ALL,
CTAPRECO_CT10API_RC_ANY,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CT10_DIAG_DISCONNECTED_BY_PEER|CT10_DIAG_DEAD_CONNECTION,
CTAPRECO_CT10_DIAG_EMPTY,

4,
0,

CTAPRECO_ACTION_WAIT_MEDIUM|CTAPRECO_ACTION_RETRY_RPC,
"Lost connection, will wait a try to reconnect",
CTAPRECO_ACTION_EXIT_PROGRAM,
"To many lost connection, must stop",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* proc not found */
CTAPRECO_FNC_ALL,
CTAPRECO_CT10API_RC_ANY,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CT10_DIAG_MISSING_PROC,
CTAPRECO_CT10_DIAG_EMPTY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" Stored procedure not found",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* LOGIN_REJECTED| */
CTAPRECO_FNC_ALL,
CTAPRECO_CT10API_RC_ANY,
CTAPRECO_RPC_RET_STA_ANY, CTAPRECO_RPC_RET_STA_ANY,
CT10_DIAG_LOGIN_REJECTED|CT10_DIAG_NO_SUCH_SERVER|CT10_DIAG_CANNOT_FIND_HOST,
CTAPRECO_CT10_DIAG_EMPTY,

0,
0,

CTAPRECO_ACTION_EXIT_PROGRAM,
"Connection to server problem, must stop",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error 100 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
100, 100,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return 100, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error 200 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
200, 200,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return 200, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error 250 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
250, 250,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return 250, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error 300 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
300, 300,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return 300, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error 400 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
400, 400,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return 400, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error 100 a 999 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
100, 999,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return code between 100 and 999, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error -100 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
-100, -100,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return -100, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error -200 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
-200, -200,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return -200, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error -100 to -999 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
-999, -100,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_SKIP_RPC,
" RPC return code between -100 and -999, will be skipped",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error -1102 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
-1102, -1102,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_RETRY_RPC|CTAPRECO_ACTION_NULL_EVT_SEQ,
" RPC return -1102, will retry with null evt_seq",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error -1103 */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
-1103, -1103,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

0,
0,

CTAPRECO_ACTION_RETRY_RPC|CTAPRECO_ACTION_NULL_EVT_SEQ,
" RPC return -1103, will retry with null evt_seq",
CTAPRECO_ACTION_NONE,
"",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error -1360 TRG inactif rpc refuses */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
-1360, -1360,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

4,
0,

CTAPRECO_ACTION_WAIT_SHORT|CTAPRECO_ACTION_RETRY_RPC,
"RPC return -1360, will wait and retry",
CTAPRECO_ACTION_EXIT_PROGRAM,
"To many retry, program will stop",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error -1361 TRG primary rpc refuse */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
-1361, -1361,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

4,
0,

CTAPRECO_ACTION_WAIT_SHORT|CTAPRECO_ACTION_RETRY_RPC,
"RPC return -1361, will wait and retry",
CTAPRECO_ACTION_EXIT_PROGRAM,
"To many retry, program will stop",
CTAPRECO_ACTION_NONE,
"",

NULL
},

{
/* stored proc error -1362 TRG secondary rpc refuses */
CTAPRECO_FNC_ALL,
CT10_SUCCESS,
-1362, -1362,
CTAPRECO_CT10_DIAG_ANY,
CTAPRECO_CT10_DIAG_ANY,

4,
0,

CTAPRECO_ACTION_WAIT_SHORT|CTAPRECO_ACTION_RETRY_RPC,
"RPC return -1362, will wait and retry",
CTAPRECO_ACTION_EXIT_PROGRAM,
"To many retry, program will stop",
CTAPRECO_ACTION_NONE,
"",

NULL
},



/* ------------------- End of recov_t2.c ----------------------------- */

