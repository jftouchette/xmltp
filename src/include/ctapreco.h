/**********************************************************************
 * ctapreco.h
 *
 *  Copyright (c) 1997 Jocelyn Blanchet
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
 * $Source: /ext_hd/cvs/include/ctapreco.h,v $
 * $Header: /ext_hd/cvs/include/ctapreco.h,v 1.3 2004/10/13 18:23:26 blanchjj Exp $
 *
 * ct10api application level recover module.
 *
 *
 * This module can be use directly by the application or the recovery
 * table can be replace to handle more specific case needed by the
 * application.
 *
 * There is 2 ways for counting the number of retry:
 *
 *	1. Any error in the retry loop will count as +1 for
 *	   the current RPC.
 *
 *	2. Each different error on the same RPC retry loop will
 *	   be increment separatly. This mean that a RPC could
 *	   fail 2 times in a row with 2 different error but it's retry 
 *	   count will be only at 1. This mode will be there only for
 *	   compatibility with other recovery module. Now I think that
 *	   this mode is useless.
 *
 * ------------------------------------------------------------------------
 * LIST OF CHANGES:
 * 1997-08-14,jbl: Initial creation.
 */


#define CTAPRECO_VERSION_NUM	"1.0"
#define CTAPRECO_VERSION_ID	\
		( CTAPRECO_VERSION_NUM __FILE__ ": " __DATE__ " - " __TIME__)
#define CTAPRECO_MODULE		(__FILE__)


/* Module return code */

#define CTAPRECO_SUCCESS			0
#define CTAPRECO_FAIL				1
#define CTAPRECO_PARAM_ERROR			2
#define	CTAPRECO_RECOVERY_INFO_NOT_FOUND	3
#define	CTAPRECO_RECOVERY_TABLE_NOT_INITIALIZE	4


/* 2 different handling of number of retries */

#define CTAPRECO_NB_RETRY_HANDLING_FNC		1
#define CTAPRECO_NB_RETRY_HANDLING_FNC_ERROR	2


/* Match any function */

#define CTAPRECO_FNC_ALL			( 0xFFFFFFFFUL )


/* RPC success return status */

#define CTAPRECO_RPC_RET_STA_SUCCESS	0


/* For these 2 dont use FFFFFFFF because it's = to -1 */

#define CTAPRECO_RPC_RET_STA_ANY		( 0x0FFFFFFFUL )
#define CTAPRECO_CT10API_RC_ANY			( 0x0FFFFFFFUL )


/* 2 ct10 diag usefull values */ 

#define CTAPRECO_CT10_DIAG_EMPTY		( 0x00000000UL )
#define CTAPRECO_CT10_DIAG_ANY			( 0xFFFFFFFFUL )


/* Use this to rrtry for ever */

#define CTAPRECO_COUNTER_NO_LIMIT        	0       


/* Standard recovery actions */

#define CTAPRECO_ACTION_NONE			( 0x00000000UL )
#define CTAPRECO_ACTION_RETRY_RPC		( 0x00000001UL )
#define CTAPRECO_ACTION_SKIP_RPC		( 0x00000002UL )
#define CTAPRECO_ACTION_EXIT_PROGRAM		( 0x00000004UL )
#define CTAPRECO_ACTION_WAIT_SHORT		( 0x00000008UL )
#define CTAPRECO_ACTION_WAIT_MEDIUM		( 0x00000010UL )
#define CTAPRECO_ACTION_WAIT_LONG		( 0x00000020UL )

#define CTAPRECO_ACTION_LOG_WARNING		( 0x00000040UL )
#define CTAPRECO_ACTION_SUCCESS			( 0x00000080UL )

#define CTAPRECO_ACTION_NULL_EVT_SEQ		( 0x00000100UL )




/* Master application level recovery table */

struct s_ctapreco_recov_table {

	/* Identification of the error */
	long	fnc_bit_mask;		/* application function bit mask     */
	long	ct10api_rc;		/* return code from ct10api          */
	long	rpc_ret_sta_low;	/* stored procedure return status    */
	long	rpc_ret_sta_high;	/* stored procedure return status    */
	long	ct10_diag_bit_mask1;	/* ct10reco bit mask 1               */
	long	ct10_diag_bit_mask2;	/* ct10reco bit mask 2               */

	/* Retry information */
	long	max_retry;		/* Maximum number of retry,0 for ever*/
	long	nb_retry;		/* Current number of retry           */

	/* Recovery action and message to log base on number of retry */
	long	recovery_action1;	/* When nb_retry < max_retry         */
	char	message_to_log1[80];	/* When nb_retry < max_retry         */
	long	recovery_action2;	/* When nb_retry = max_retry         */
	char	message_to_log2[80];	/* When nb_retry = max_retry         */
	long	recovery_action3;	/* When nb_retry > max_retry         */
	char	message_to_log3[80];	/* When nb_retry > max_retry         */

	/* Custom recovery info */
	void	*custom_recovery_info;	/* When nb_retry < max_retry         */
};

typedef struct s_ctapreco_recov_table CTAPRECO_RECOV_TABLE_T;



/*
 ***************************************************************************
 Return module version id string.
 ***************************************************************************/

char *ctapreco_get_version_id();



/*
 ***************************************************************************
 Initialize the static global pointer to the recovery table + it's size.
 Initialize as well the way that the number of retries will be handle.
 	1. Count retry by function.
	2. Count retry by function and error.

 return code:

	CTAPRECO_PARAM_ERROR
	CTAPRECO_SUCCESS

 ***************************************************************************/

int ctapreco_intit_recovery_info(
	void	*recovery_table,	/* Pointer to the recovery table     */
	long	nb_entries,		/* Number of entries in recov table  */
	int	nb_retry_handling	/* count nb retry by function or     */
					/* by the identification info        */
					/* or bye the first matching fnc     */
);



/*
 ***************************************************************************
 This must be used before before executing any RPC but outside de retry
 loop. It will work according to the nb_retry_handling.

 return code:

	CTAPRECO_RECOVERY_TABLE_NOT_INITIALIZE
	CTAPRECO_RECOVERY_INFO_NOT_FOUND
	CTAPRECO_PARAM_ERROR
	CTAPRECO_SUCCESS

 ***************************************************************************/

int	ctapreco_reset_nb_retry(
	long    fnc_bit_mask            /* application function bit mask     */
);



/*
 ****************************************************************************

 Base on input parameters and the number of retries this function will 
 return the appropriate recovery actions bit mark.
 
 return code:

	CTAPRECO_RECOVERY_TABLE_NOT_INITIALIZE
	CTAPRECO_RECOVERY_INFO_NOT_FOUND
	CTAPRECO_SUCCESS

 ****************************************************************************/

int	ctapreco_find_recovery_info (
	long	fnc_bit_mask,		/* Application function bit mask     */
	long	ct10api_rc,		/* Return code from ct10api          */
	long	rpc_ret_sta,		/* Stored procedure return status    */
	long	ct10_diag_bit_mask1,	/* ct10reco bit mask 1               */
	long	ct10_diag_bit_mask2,	/* ct10reco bit mask 2               */
	long	*recovery_actions,	/* Recovery actions                  */
	char	**message_to_log,	/* Message to log                    */
	void	**custom_recovery_info  /* may be null                       */
);

