/* osclerr1.h
 * ----------
 *
 *
 *  Copyright (c) 1994-1996 Jean-Francois Touchette
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
 * $Source: /ext_hd/cvs/include/osclerr1.h,v $
 * $Header: /ext_hd/cvs/include/osclerr1.h,v 1.11 2004/10/15 20:46:57 jf Exp $
 *
 *
 * Defines prefix:	OSCLERR_
 *
 *
 * Defines of error numbers send to client with srv_sendinfo()
 *
 *
 * --------------------------------------------------------------------------
 *
 * 1994dec19,jft: first version
 * 1996apr17,jft: + OSCLERR_RPC_RPC_NAME_BLOCKED
 */

#ifndef _OSCLERR1_H_
#define _OSCLERR1_H_


#define OSCLERR_FIRST_NUM			50000
#define OSCLERR_CONNECT_FIRST_N			(OSCLERR_FIRST_NUM + 0)
#define OSCLERR_OPER_CMD_FIRST_N		(OSCLERR_FIRST_NUM + 1000)
#define OSCLERR_RPC_PROC_FIRST_N		(OSCLERR_FIRST_NUM + 2000)
#define OSCLERR_OS_REG_PROC_FIRST_N		(OSCLERR_FIRST_NUM + 2500)
#define OSCLERR_DT_REG_PROC_FIRST_N		(OSCLERR_FIRST_NUM + 3000)

/* 55000..55999 reserved for SQL Server application Stored Procedure:
 */
#define OSCLERR_SP_ERROR_MSG_FIRST_N		(OSCLERR_FIRST_NUM + 5000)


/* PLEASE: create new ranges of error numbers using the schema
 * 	    used above...
 */

#define OSCLERR_CONNECT_OK			(OSCLERR_CONNECT_FIRST_N + 1)
#define OSCLERR_CONNECT_REJECT_QUIET		(OSCLERR_CONNECT_FIRST_N + 2)

#if 0
#define OSCLERR_CONNECT_FAILED			(OSCLERR_CONNECT_FIRST_N + 5)
#endif
			/*
			 * Patch to look like CIG version 1:
			 */
#define OSCLERR_CONNECT_FAILED			(SRV_MAXERROR + 1)

#define OSCLERR_CONNECT_FAILED_LOGIN_FAILED	(OSCLERR_CONNECT_FIRST_N + 6)
#define OSCLERR_CONNECT_DONE_UNCLEAN		(OSCLERR_CONNECT_FIRST_N + 7)

#define OSCLERR_CONNECT_FAILED_AUTH_PROBLEM	(OSCLERR_CONNECT_FIRST_N + 11)
#define OSCLERR_CONNECT_CREATE_USERDATA_FAILED	(OSCLERR_CONNECT_FIRST_N + 12)



#define OSCLERR_OPER_CMD_OK			(OSCLERR_OPER_CMD_FIRST_N + 1)
#define OSCLERR_OPER_CMD_UNKNOWN_CMD		(OSCLERR_OPER_CMD_FIRST_N + 2)
#define OSCLERR_OPER_CMD_UNKNOWN_OPTION		(OSCLERR_OPER_CMD_FIRST_N + 3)
#define OSCLERR_OPER_CMD_MISSING_ARG		(OSCLERR_OPER_CMD_FIRST_N + 4)
#define OSCLERR_OPER_CMD_TOO_MANY_ARGS		(OSCLERR_OPER_CMD_FIRST_N + 5)

#define OSCLERR_OPER_CMD_NO_SUCH_DEST		(OSCLERR_OPER_CMD_FIRST_N + 10)
#define OSCLERR_OPER_CMD_FAILED			(OSCLERR_OPER_CMD_FIRST_N + 11)
#define OSCLERR_OPER_CMD_REJECT_NOT_ADMIN	(OSCLERR_OPER_CMD_FIRST_N + 12)

#define OSCLERR_OPER_CMD_CANNOT_DO_NOW		(OSCLERR_OPER_CMD_FIRST_N + 21)


/* msgno used in or_rpc.c and similar modules:
 */
#define OSCLERR_RPC_CANNOT_GET_PROC_USERDATA	(OSCLERR_RPC_PROC_FIRST_N + 1)
#define OSCLERR_RPC_CANNOT_CREATE_TRANS_FRAME	(OSCLERR_RPC_PROC_FIRST_N + 2)
#define OSCLERR_RPC_CANNOT_ASSIGN_P_TRANS_FRAME	(OSCLERR_RPC_PROC_FIRST_N + 3)
#define OSCLERR_RPC_CANNOT_REMOVE_P_TRANS_FRAME	(OSCLERR_RPC_PROC_FIRST_N + 4)

#define OSCLERR_RPC_RPC_GATE_IS_CLOSED		(OSCLERR_RPC_PROC_FIRST_N + 10)
#define OSCLERR_RPC_LANG_GATE_IS_CLOSED		(OSCLERR_RPC_PROC_FIRST_N + 11)
#define OSCLERR_RPC_LANG_NOT_SUPPORTED		(OSCLERR_RPC_PROC_FIRST_N + 12)

#define OSCLERR_RPC_CANNOT_UPDATE_USER_PROFILE	(OSCLERR_RPC_PROC_FIRST_N + 20)

#define OSCLERR_RPC_LANG_GET_PROFILE_FAILED	(OSCLERR_RPC_PROC_FIRST_N + 30)
#define OSCLERR_RPC_LANG_PERMISSION_DENIED	(OSCLERR_RPC_PROC_FIRST_N + 31)


#define OSCLERR_RPC_RPC_NAME_BLOCKED		(OSCLERR_RPC_PROC_FIRST_N + 40)


/*
 * Msg issued from Registered Procedures in dt_calrp.c:
 */
#define OSCLERR_DT_REG_PROC_RP_ID_NOTFND (OSCLERR_DT_REG_PROC_FIRST_N + 1)
#define OSCLERR_DT_REG_PROC_BAD_PARAM	 (OSCLERR_DT_REG_PROC_FIRST_N + 2)
#define OSCLERR_DT_REG_PROC_LIBR_ERROR	 (OSCLERR_DT_REG_PROC_FIRST_N + 3)
#define OSCLERR_DT_CANNOT_SET_RET_PARAM	 (OSCLERR_DT_REG_PROC_FIRST_N + 5)
#define OSCLERR_DT_CANNOT_GET_PARAM_VAL	 (OSCLERR_DT_REG_PROC_FIRST_N + 6)


/*
 * Msg issued from Registered Procedures calling general functions of
 * os_rp_fn.c:
 */
#define OSCLERR_OS_RP_FN_RP_ID_NOTFND	      (OSCLERR_OS_REG_PROC_FIRST_N + 1)
#define OSCLERR_OS_RP_FN_CANNOT_SET_RET_PARAM (OSCLERR_OS_REG_PROC_FIRST_N + 2)
#define OSCLERR_OS_RP_FN_CANNOT_GET_PARAM_VAL (OSCLERR_OS_REG_PROC_FIRST_N + 3)

#endif /* #ifndef _OSCLERR1_H_ */


/* -------------------------------------- end of osclerr1.h ------------- */
