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
#include "cmd.h"

GBLREF char window_token;

int m_ztstart(void)
{
	error_def(ERR_SPOREOL);

	if (window_token != TK_EOL && window_token != TK_SPACE)
	{	stx_error(ERR_SPOREOL);
		return FALSE;
	}
	newtriple(OC_ZTSTART);
	return TRUE;
}
