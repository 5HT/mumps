/****************************************************************
 *								*
 *	Copyright 2005, 2008 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/
#ifndef __JNL_GET_CHECKSUM_H_
#define __JNL_GET_CHECKSUM_H_

#define INIT_CHECKSUM_SEED 1
#define CHKSUM_SEGLEN4 8

#define ADJUST_CHECKSUM(sum, num4)  (((sum) >> 4) + ((sum) << 4) + (num4))

#ifdef DEBUG
GBLREF	uint4			donot_commit;	/* see gdsfhead.h for the purpose of this debug-only global */
#endif

/* This macro is to be used whenever we are computing the checksum of a block that has been acquired. */
#define	JNL_GET_CHECKSUM_ACQUIRED_BLK(cse, csd, old_blk, bsize)						\
{													\
	cache_rec_ptr_t	cr;										\
	/* Record current database tn before computing checksum of acquired block. This is used		\
	 * later by the commit logic to determine if the block contents have changed (and hence		\
	 * if recomputation of checksum is necessary). For BG, we have two-phase commit where		\
	 * phase2 is done outside of crit. So it is possible that we note down the current database	\
	 * tn and then compute checksums outside of crit and then get crit and yet in the validation	\
	 * logic find the block header tn is LESSER than the noted dbtn (even though the block		\
	 * contents changed after the noted dbtn). This will cause us to falsely validate this block	\
	 * as not needing checksum recomputation. To ensure the checksum is recomputed inside crit,	\
	 * we note down a tn of 0 in case the block is locked for update (cr->in_tend is non-zero).	\
	 */												\
	assert((gds_t_acquired == cse->mode) || (gds_t_create == cse->mode));				\
	assert(cse->old_block == (sm_uc_ptr_t)(old_blk));						\
	assert((bsize) <= csd->blk_size);								\
	/* Since this macro is invoked only in case of before-image journaling and since MM does not	\
	 * support before-image journaling, we can safely assert that BG is the only access method.	\
	 */												\
	assert(dba_bg == csd->acc_meth);								\
	/* In rare cases cse->cr can be NULL even though this block is an acquired block. This is 	\
	 * possible if we are in TP and this block was part of the tree in the initial phase of the	\
	 * transaction but was marked free (by another process concurrently) in the later phase of	\
	 * the same TP transaction. But this case is a sureshot restart situation so be safe and 	\
	 * ensure recomputation happens inside of crit just in case we dont restart. Also add asserts	\
	 * (using donot_commit variable) to ensure we do restart this transaction.			\
	 */												\
	cr = cse->cr;											\
	assert((NULL != cr) || dollar_tlevel);								\
	DEBUG_ONLY(if (NULL == cr) donot_commit |= DONOTCOMMIT_JNLGETCHECKSUM_NULL_CR;)			\
	cse->tn = (((NULL == cr) || cr->in_tend) ? 0 : csd->trans_hist.curr_tn);			\
	cse->blk_checksum = jnl_get_checksum((uint4 *)(old_blk), (bsize));				\
}

uint4 jnl_get_checksum(uint4 *buff, int bufflen);
uint4 jnl_get_checksum_entire(uint4 *buff, int bufflen);

#endif
