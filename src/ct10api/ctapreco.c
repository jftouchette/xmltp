/**********************************************************************
 * ctapreco.c
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
 * $Source: /ext_hd/cvs/ct10api/ctapreco.c,v $
 * $Header: /ext_hd/cvs/ct10api/ctapreco.c,v 1.4 2004/10/13 18:23:26 blanchjj Exp $
 *
 * ct10api application level recover module.
 *
 * LIST OF CHANGES:
 * 1997-08-14,jbl: Initial creation.
 */

#include <stdio.h>
#include "common_1.h"
#include <ctpublic.h>
#include <string.h>
#include <unistd.h>
#include <ct10api.h>
#include "ctapreco.h"

#include "recov_t.c"


static char *ctapreco_version_id = CTAPRECO_VERSION_ID;

static CTAPRECO_RECOV_TABLE_T	(*p_ctapreco_recov_table)[] = 
						&tab_ctapreco;

static int			nbrow_ctapreco_recov_table = 
						NBROW_TAB_CTAPRECO;

static int			retry_handling = 
						CTAPRECO_NB_RETRY_HANDLING_FNC;



/*
 ***************************************************************************
 Return module version id string.
 ***************************************************************************/

char *ctapreco_get_version_id()
{

	return ctapreco_version_id;

} /* ----------- End of ctapreco_get_version_id ---------------------- */



/*
 ***************************************************************************
 Initialize the static global pointer to the recovery table + it's size.
 Initialize as well the way that the number of retries will be handle.
 	1. Count retry by function.
	2. Count retry by function and error.

 *recovery_table can be passed as NULL to change only nb_retry_handling.

 return code:

	CTAPRECO_PARAM_ERROR
	CTAPRECO_SUCCESS
 ***************************************************************************/

int ctapreco_init_recovery_info(
	void	*recovery_table,	/* Pointer to the recovery table     */
	long	nb_entries,		/* Number of entries in recov table  */
	int	nb_retry_handling	/* count nb retry by function or     */
					/* by the identification info        */
)
{

	if ( (nb_retry_handling != CTAPRECO_NB_RETRY_HANDLING_FNC) &&
	     (nb_retry_handling != CTAPRECO_NB_RETRY_HANDLING_FNC_ERROR) ) {
		return  CTAPRECO_PARAM_ERROR;
	}

	if (recovery_table != NULL) {

		if (nb_entries <= 0) {
			return  CTAPRECO_PARAM_ERROR;
		}

		p_ctapreco_recov_table = recovery_table;
		nbrow_ctapreco_recov_table = nb_entries;
	}

	retry_handling = nb_retry_handling;

	return CTAPRECO_SUCCESS;

} /* --------------- End of ctapreco_intit_recovery_info ------------------ */



/*
 ***************************************************************************
 This must be used before before executing any RPC but outside de retry
 loop. It will work according to the nb_retry_handling.

 return code:

	CTAPRECO_RECOVERY_TABLE_NOT_INITIALIZE
	CTAPRECO_RECOVERY_INFO_NOT_FOUND
	CTAPRECO_SUCCESS

 ***************************************************************************/

int	ctapreco_reset_nb_retry(
	long    fnc_bit_mask           /* application function bit mask     */
)
{
	int i;
	int found_indicator = FALSE;

	if (p_ctapreco_recov_table == NULL) {
		return CTAPRECO_RECOVERY_TABLE_NOT_INITIALIZE;
	}

	if (fnc_bit_mask == 0) {
		return CTAPRECO_PARAM_ERROR;
	}

	for (i=0; i<nbrow_ctapreco_recov_table; i++) {

		if (fnc_bit_mask & (*p_ctapreco_recov_table)[i].fnc_bit_mask) {

			found_indicator = TRUE;
			(*p_ctapreco_recov_table)[i].nb_retry = 0;

			if (retry_handling == CTAPRECO_NB_RETRY_HANDLING_FNC) {
				break;
			}
		}

	} /* End of for */

	if (found_indicator == FALSE) {
		return CTAPRECO_RECOVERY_INFO_NOT_FOUND;
	}
	else {
		return CTAPRECO_SUCCESS;
	}

} /* ----------------- End of ctapreco_reset_nb_retry --------------------- */



/*
 ****************************************************************************

 Compare input parameters with recovery table entry to find a match.

 return code:
	TRUE
	FALSE

 ****************************************************************************/

int ctapreco_match_found(
	long	fnc_bit_mask,		/* Application function bit mask     */
	long	ct10api_rc,		/* Return code from ct10api          */
	long	rpc_ret_sta,		/* Stored procedure return status    */
	long	ct10_diag_bit_mask1,	/* ct10reco bit mask 1               */
	long	ct10_diag_bit_mask2,	/* ct10reco bit mask 2               */
	int	i			/* Position in recovery table        */
)
{


/*******
printf("R [%x] [%ld] [%ld] [%x] [%x]\n",
	fnc_bit_mask,
	ct10api_rc,
	rpc_ret_sta,
	ct10_diag_bit_mask1,
	ct10_diag_bit_mask2);
printf("T [%x] [%ld] [%ld] [%x] [%x]\n",
	(*p_ctapreco_recov_table)[i].fnc_bit_mask,
	(*p_ctapreco_recov_table)[i].ct10api_rc,
	(*p_ctapreco_recov_table)[i].rpc_ret_sta,
	(*p_ctapreco_recov_table)[i].ct10_diag_bit_mask1,
	(*p_ctapreco_recov_table)[i].ct10_diag_bit_mask2);
*****/


if (fnc_bit_mask & (*p_ctapreco_recov_table)[i].fnc_bit_mask) {

	if ( ( (*p_ctapreco_recov_table)[i].ct10api_rc ==
		CTAPRECO_CT10API_RC_ANY ) ||

               ((*p_ctapreco_recov_table)[i].ct10api_rc == 
	        ct10api_rc) ) {

		if ( ( (*p_ctapreco_recov_table)[i].rpc_ret_sta_low ==
			CTAPRECO_RPC_RET_STA_ANY  ) ||

       		       (((*p_ctapreco_recov_table)[i].rpc_ret_sta_low <=
		       rpc_ret_sta) && 
			((*p_ctapreco_recov_table)[i].rpc_ret_sta_high >=
		       rpc_ret_sta) ) ) {

			if ( (((*p_ctapreco_recov_table)[i].ct10_diag_bit_mask1
				== CTAPRECO_CT10_DIAG_ANY) ||
    			      ((*p_ctapreco_recov_table)[i].ct10_diag_bit_mask1
				 & ct10_diag_bit_mask1)) ||

 			   (((*p_ctapreco_recov_table)[i].ct10_diag_bit_mask2
				== CTAPRECO_CT10_DIAG_ANY) ||
    			      ((*p_ctapreco_recov_table)[i].ct10_diag_bit_mask2
				 & ct10_diag_bit_mask2)) 
			){

				return TRUE;
			} /* 4 */


		} /* 3 */


	} /* 2 */


} /* 1 */



return FALSE;


} /* -------------- End of ctapreco_match_found -------------------------- */



/*
 ****************************************************************************
 Base on the number or retries and the max retry return recovery action and
 log message.
 ****************************************************************************/

void ctapreco_return_recovery_action(
	int	i,
	int	n,
	long	*recovery_actions,	/* Recovery actions                  */
	char	**message_to_log,	/* Message to log                    */
	void	**custom_recovery_info  /* may be null                       */
)
{

if (custom_recovery_info != NULL) {
	*custom_recovery_info = 
	(*p_ctapreco_recov_table)[i].custom_recovery_info;
}


if ( ( (*p_ctapreco_recov_table)[n].nb_retry < 
       (*p_ctapreco_recov_table)[i].max_retry ) ||
     ( (*p_ctapreco_recov_table)[i].max_retry == CTAPRECO_COUNTER_NO_LIMIT )){

	*recovery_actions = (*p_ctapreco_recov_table)[i].recovery_action1;
	*message_to_log = (*p_ctapreco_recov_table)[i].message_to_log1;
}
else if ( (*p_ctapreco_recov_table)[n].nb_retry == 
          (*p_ctapreco_recov_table)[i].max_retry ) {
	*recovery_actions = (*p_ctapreco_recov_table)[i].recovery_action2;
	*message_to_log = (*p_ctapreco_recov_table)[i].message_to_log2;
}
else {
	*recovery_actions = (*p_ctapreco_recov_table)[i].recovery_action3;
	*message_to_log = (*p_ctapreco_recov_table)[i].message_to_log3;
}



}/* ------------- End of ctapreco_return_recovery_action ------------------ */



/*
 ****************************************************************************

 Base on input parameters an the number of retries this function will 
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
)
{
	int i;
	int found_indicator = FALSE;
	int posi_nb_retry   = -1;

	if (p_ctapreco_recov_table == NULL) {
		*recovery_actions = 0;
		*message_to_log = 0;
		if (custom_recovery_info != NULL) {
			*custom_recovery_info = NULL;
		}
		return CTAPRECO_RECOVERY_TABLE_NOT_INITIALIZE;
	}

	for (i=0; i<nbrow_ctapreco_recov_table; i++) {

		if ( (retry_handling == CTAPRECO_NB_RETRY_HANDLING_FNC) &&
		     (posi_nb_retry < 0) ) {

			if (fnc_bit_mask & 
				(*p_ctapreco_recov_table)[i].fnc_bit_mask) {
					posi_nb_retry = i;
			}
		}

		if (ctapreco_match_found(	fnc_bit_mask,
						ct10api_rc,
						rpc_ret_sta,
						ct10_diag_bit_mask1,
						ct10_diag_bit_mask2,
						i
				) == TRUE) {

			found_indicator = TRUE;

			if (retry_handling != CTAPRECO_NB_RETRY_HANDLING_FNC) {
				posi_nb_retry = i;
			}

			break;
		}

	} /* End of for */

	if (found_indicator == FALSE) {
		*recovery_actions = 0;
		*message_to_log = 0;
		if (custom_recovery_info != NULL) {
			*custom_recovery_info = NULL;
		}
		return CTAPRECO_RECOVERY_INFO_NOT_FOUND;
	}
	else {
		++((*p_ctapreco_recov_table)[posi_nb_retry].nb_retry);

		ctapreco_return_recovery_action(
			i,posi_nb_retry,
			recovery_actions,
			message_to_log,
			custom_recovery_info);

		return CTAPRECO_SUCCESS;
	}

} /* ------------ ENd of ctapreco_find_recover_info ---------------------- */




/* ------------------- End of ctapreco.c ---------------------------------- */
