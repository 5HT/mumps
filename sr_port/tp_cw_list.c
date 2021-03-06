/****************************************************************
 *								*
 *	Copyright 2001, 2007 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"

#include "gdsroot.h"
#include "gdskill.h"
#include "gdsbt.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsfhead.h"
#include "gdscc.h"
#include "filestruct.h"
#include "jnl.h"
#include "buddy_list.h"		/* needed for tp.h */
#include "hashtab_int4.h"	/* needed for tp.h */
#include "tp.h"

GBLREF	sgm_info	*sgm_info_ptr;
GBLREF	gd_region	*gv_cur_region;
GBLREF	sgmnt_addrs	*cs_addrs;
GBLREF	boolean_t	is_updproc;
GBLREF	boolean_t	mupip_jnl_recover;

void tp_cw_list(cw_set_element **cs)
{
	cw_set_element	*last_cse, *tempcs, *prev_last;

	error_def(ERR_TRANS2BIG);

	if (dba_bg == cs_addrs->hdr->acc_meth)
		if (sgm_info_ptr->cw_set_depth + 2 >= (cs_addrs->hdr->n_bts >> 1))
		{	/* catch the case where MUPIP recover or update process gets into this situation */
			assert(!mupip_jnl_recover && !is_updproc);
			rts_error(VARLSTCNT(4) ERR_TRANS2BIG, 2, REG_LEN_STR(gv_cur_region));
		}

	tempcs = (cw_set_element *)get_new_element(sgm_info_ptr->cw_set_list, 1);
	/* secshr_db_clnup relies on the cw_set_element (specifically the "mode" field) being initialized to a value
	 * that is not "gds_t_committed". This needs to be done before setting sgm_info_ptr->first_cw_set. */
	memset(tempcs, 0, sizeof(cw_set_element));
	assert(gds_t_committed != tempcs->mode);	/* ensure secshr_db_clnup's check will not be affected */
	if (sgm_info_ptr->first_cw_set == NULL)
		sgm_info_ptr->first_cw_set = tempcs;
	else
	{
		last_cse = sgm_info_ptr->last_cw_set;
		assert(last_cse);
		for (; last_cse; last_cse = last_cse->high_tlevel)
			last_cse->next_cw_set = tempcs;
	}

	prev_last = sgm_info_ptr->last_cw_set;
	sgm_info_ptr->last_cw_set = *cs = tempcs;
	tempcs->prev_cw_set = prev_last;
}
