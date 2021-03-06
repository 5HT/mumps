/****************************************************************
 *								*
 *	Copyright 2004, 2005 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "hashtab_mname.h"	/* needed for lv_val.h */
#include "lv_val.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"

#include "mvalconv.h"
#include "op.h"		/* for op_add prototype */

/* this prototype is currently not added into op.h because the type "lv_val" is not recognized by op.h unless lv_val.h is
 * included within and it is not a preferred practice to recursively include header files.
 */
void	op_fnincr(lv_val *local_var, mval *increment, mval *result);

LITREF	mval		literal_null;

void	op_fnincr(lv_val *local_var, mval *increment, mval *result)
{
	mval		*lv_mval;
	unsigned char	buff[MAX_ZWR_KEY_SZ], *end;

	error_def(ERR_UNDEF);

	MV_FORCE_NUM(increment); /* we do this operation in op_gvincr(), so it should be no different in op_fnincr() */
	lv_mval = &local_var->v;
	if (!MV_DEFINED(lv_mval))
		*lv_mval = literal_null;
	op_add(lv_mval, increment, lv_mval); /* FALSE is to indicate NOT subtraction (i.e. addition) */
	*result = *lv_mval;
}
