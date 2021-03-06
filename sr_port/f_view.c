/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
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
#include "toktyp.h"
#include "advancewindow.h"

GBLREF char window_token;

int f_view( oprtype *a, opctype op )
{
	triple *root, *last, *curr;
	oprtype argv[CHARMAXARGS], *argp;
	int argc;
	error_def(ERR_FCHARMAXARGS);

	argp = &argv[0];
	argc = 0;
	if (!expr(argp))
		return FALSE;
	assert(argp->oprclass == TRIP_REF);
	argc++;
	argp++;
	for (;;)
	{
		if (window_token != TK_COMMA)
			break;
		advancewindow();
		if (!expr(argp))
			return FALSE;
		assert(argp->oprclass == TRIP_REF);
		argc++;
		argp++;
		if (argc >= CHARMAXARGS - 1)
		{	stx_error(ERR_FCHARMAXARGS);
			return FALSE;
		}
	}
	root = last = maketriple(op);
	root->operand[0] = put_ilit(argc + 1);
	argp = &argv[0];
	for (; argc > 0 ;argc--, argp++)
	{
		curr = newtriple(OC_PARAMETER);
		curr->operand[0] = *argp;
		last->operand[1] = put_tref(curr);
		last = curr;
	}
	ins_triple(root);
	*a = put_tref(root);
	return TRUE;
}
