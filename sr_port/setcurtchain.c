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

#include "compiler.h"

GBLDEF	triple *curtchain;
GBLREF	int4	pending_errtriplecode;	/* if non-zero contains the error code to invoke ins_errtriple with */
GBLREF	triple	t_orig;

triple *setcurtchain(triple *x)
{
	triple	*y;

	y = curtchain;
	curtchain = x;
	if (pending_errtriplecode && (curtchain == &t_orig))
	{	/* A compile error was seen while curtchain was temporarily switched and hence an ins_errtriple did not
		 * insert a OC_RTERROR triple then. Now that curtchain is back in the same chain as pos_in_chain, reissue
		 * the ins_errtriple call.
		 */
		 assert(!IS_STX_WARN(pending_errtriplecode));
		 ins_errtriple(pending_errtriplecode);
		 pending_errtriplecode = 0;
	}
	return y;
}
