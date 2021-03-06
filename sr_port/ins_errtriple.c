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
#include "opcode.h"
#include "mdq.h"

GBLREF	triple	*curtchain,pos_in_chain;
GBLREF	int4	pending_errtriplecode;	/* if non-zero contains the error code to invoke ins_errtriple with */
GBLREF	triple	t_orig;

void ins_errtriple(int4 in_error)
{
	triple 		*x, *triptr;
	boolean_t	add_rterror_triple;

	if (!IS_STX_WARN(in_error))
	{	/* For IS_STX_WARN errors, parsing continues, so dont strip the chain */
		if (curtchain != &t_orig)
		{	/* If working with more than 1 chain defer until back to 1 because dqdelchain cannot delete across
			 * multiple chains. Set global variable "pending_errtriplecode" and let "setcurtchain" call here again.
			 */
			if (!pending_errtriplecode)			/* Give user only the first error on the line */
				pending_errtriplecode = in_error;	/* Save error for later insert */
			return;
		}
		x = pos_in_chain.exorder.bl;
		/* If first error in the current line/cmd, delete all triples and replace them with an OC_RTERROR triple. */
		add_rterror_triple = (OC_RTERROR != x->exorder.fl->opcode);
		if (!add_rterror_triple)
		{	/* This is the second error in this line/cmd. Check for triples added after OC_RTERROR and remove them
			 * as there could be dangling references amongst them which could later cause GTMASSERT in emit_code.
			 */
			x = x->exorder.fl;
			assert(OC_RTERROR == x->opcode);/* corresponds to newtriple(OC_RTERROR) in previous ins_errtriple */
			x = x->exorder.fl;
			assert(OC_ILIT == x->opcode);	/* corresponds to put_ilit(in_error) in previous ins_errtriple */
			x = x->exorder.fl;
			assert(OC_ILIT == x->opcode);	/* corresponds to put_ilit(FALSE) in previous ins_errtriple */
		}
		dqdelchain(x, curtchain, exorder);
		assert(!add_rterror_triple || (pos_in_chain.exorder.bl->exorder.fl == curtchain));
		assert(!add_rterror_triple || (curtchain->exorder.bl == pos_in_chain.exorder.bl));
	} else
		add_rterror_triple = TRUE;
	if (add_rterror_triple)
	{
		triptr = newtriple(OC_RTERROR);
		triptr->operand[0] = put_ilit(in_error);
		triptr->operand[1] = put_ilit(FALSE);	/* not a subroutine reference */
	}
}
