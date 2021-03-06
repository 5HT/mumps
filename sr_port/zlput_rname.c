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
#include "rtnhdr.h"
#include "stack_frame.h"
#include "hashtab_mname.h"
#include "fix_pages.h"
#include "zbreak.h"
#include "private_code_copy.h"
#include "urx.h"
#include "min_max.h"
#include "hashtab.h"
#include "stringpool.h"
#include "gtm_text_alloc.h"

#define S_CUTOFF 		7
#define FREE_RTNTBL_SPACE 	17

GBLREF rtn_tabent	*rtn_fst_table, *rtn_names, *rtn_names_end, *rtn_names_top;
GBLREF stack_frame	*frame_pointer;
GBLREF hash_table_mname rt_name_tbl;	/* globally defined routine table name */

bool zlput_rname (rhdtyp *hdr)
{
	rhdtyp		*old_rhead, *rhead;
	rtn_tabent	*rbot, *mid, *rtop;
	stack_frame	*fp;
	char		*src, *new, *old_table;
	int		comp;
	ht_ent_mname    *tabent;
	mname_entry	key;
	uint4		*src_tbl, entries;
	mstr		*curline;
	mident		*rtn_name;
	size_t		size, src_len;

	rtn_name = &hdr->routine_name;
	rbot = rtn_names;
	rtop = rtn_names_end;
	for (;;)
	{	/* See if routine exists in list via a binary search which reverts to serial
		   search when # of items drops below the threshold S_CUTOFF.
		*/
		if ((rtop - rbot) < S_CUTOFF)
		{
			comp = -1;
			for (mid = rbot; mid <= rtop ; mid++)
			{
				MIDENT_CMP(&mid->rt_name, rtn_name, comp);
				if (0 <= comp)
					break;
			}
			break;
		} else
		{	mid = rbot + (rtop - rbot)/2;
			MIDENT_CMP(&mid->rt_name, rtn_name, comp);
			if (0 == comp)
				break;
			else if (0 > comp)
			{
				rbot = mid + 1;
				continue;
			}
			else {
				rtop = mid - 1;
				continue;
			}
		}
	}
	if (comp)
	{	/* Entry was not found. Add in a new one */
		old_table = (char *)0;
		src = (char *) mid;
		src_len = (char *)rtn_names_end - (char *)mid + sizeof(rtn_tabent);
		if (rtn_names_end >= rtn_names_top)
		{ /* Not enough room, recreate table in larger area */
			size = (char *)rtn_names_end - (char *)rtn_names + (sizeof(rtn_tabent) * FREE_RTNTBL_SPACE);
			new = malloc(size);
			memcpy(new, rtn_names, (char *)mid - (char *)rtn_names);
			mid = (rtn_tabent *)((char *)mid + (new - (char *)rtn_names));
			old_table = (char *) rtn_names;
			rtn_names_end = (rtn_tabent *)((char *)rtn_names_end + (new - (char *)rtn_names));
			rtn_names = (rtn_tabent *)new;
			rtn_names_top = (rtn_tabent *)(new + size - sizeof(rtn_tabent));
			memset(rtn_names_end + 1, 0, size - ((char *)(rtn_names_end + 1) - new));
		}
		memmove(mid + 1, src, src_len);
		mid->rt_name = *rtn_name;
		rtn_names_end++;
		if (old_table && old_table != (char *)rtn_fst_table)
			free(old_table);		/* original table can't be freed */
		assert(NON_USHBIN_ONLY(!hdr->old_rhead_ptr) USHBIN_ONLY(!hdr->old_rhead_adr));
	} else
	{	/* Entry exists. Update it */
		old_rhead = (rhdtyp *)mid->rt_adr;
		/* Verify routine is not currently active. If it is, we cannot replace it */
		for (fp = frame_pointer; fp ; fp = fp->old_frame_pointer)
		{	/* Check all possible versions of each routine header */
			for (rhead = CURRENT_RHEAD_ADR(old_rhead); rhead;
			     rhead = (rhdtyp *)NON_USHBIN_ONLY(rhead->old_rhead_ptr)USHBIN_ONLY(rhead->old_rhead_adr))
				if (fp->rvector == rhead)
					return FALSE;
		}
		zr_remove(old_rhead); /* get rid of the now inactive breakpoints and release any private code section */

		/* If source has been read in for old routine, free space. Since routine name is the key, do this before
		   (in USHBIN builds) we release the literal text section as part of the releasable read-only section.
		*/
		tabent = NULL;
		if (rt_name_tbl.base)
		{
			key.var_name = mid->rt_name;
			COMPUTE_HASH_MNAME(&key);
			if (NULL != (tabent = lookup_hashtab_mname(&rt_name_tbl, &key)) && tabent->value)
			{
				src_tbl = (uint4 *)tabent->value;
				entries = *(src_tbl + 1);
				if (0 != entries)
					/* Don't count line 0 which we bypass */
					entries--;
				/* curline start is 2 uint4s into src_tbl and then space past line 0 or
				   we end up freeing the storage for line 0/1 twice since they have the
				   same pointers.
				*/
				for (curline = RECAST(mstr *)(src_tbl + 2) + 1; 0 != entries; --entries, ++curline)
				{
					assert(curline->len);
					free(curline->addr);
				}
				free(tabent->value);
				tabent->value = 0;
			} else
				tabent = NULL;
		}
		NON_USHBIN_ONLY(
			hdr->old_rhead_ptr = (int4)old_rhead;
			if (!old_rhead->old_rhead_ptr)
			{
			        fix_pages((unsigned char *)old_rhead, (unsigned char *)LNRTAB_ADR(old_rhead)
					  + (sizeof(lnr_tabent) * old_rhead->lnrtab_len));
			}
		)
		USHBIN_ONLY(
			if (!old_rhead->shlib_handle)
		        { 	/* Migrate text literals pointing into text area we are about to throw away into the stringpool.
				   We also can release the read-only releasable segment as it is no longer needed.
				*/
				stp_move((char *)old_rhead->literal_text_adr,
					 (char *)(old_rhead->literal_text_adr + old_rhead->literal_text_len));
				if (tabent)
				{	/* There was $TEXT info released thus an hash entry with a program name probably
					   pointing into the readonly storage we are about to release. Replace the mident
					   key in the hashtable with the routine name mident from the new header.
					*/
					assert(MSTR_EQ(&tabent->key.var_name, rtn_name));
					tabent->key.var_name = *rtn_name;	/* Update key with newly saved mident */
				}
				zlmov_lnames(old_rhead); /* copy the label names from literal pool to malloc'd area */
				GTM_TEXT_FREE(old_rhead->ptext_adr);
				/* Reset the routine header pointers to the sections we just freed up.
				 * NOTE: literal_text_adr shouldn't be reset as it points to the label area malloc'd
				 * in zlmov_lnames() */
				old_rhead->ptext_adr = old_rhead->ptext_end_adr = NULL;
				old_rhead->lnrtab_adr = NULL;
			}
			urx_remove(old_rhead->linkage_adr, old_rhead->linkage_len);
			free(old_rhead->literal_adr);	/* Release the read-write releasable segments */
			old_rhead->literal_adr = NULL;
			old_rhead->vartab_adr = NULL;

			free(old_rhead->linkage_adr);	/* Release the old linkage section */
			old_rhead->linkage_adr = NULL;
			hdr->old_rhead_adr = old_rhead;
		)
		mid->rt_name = *rtn_name;
	}
	mid->rt_adr= hdr;
	return TRUE;
}
