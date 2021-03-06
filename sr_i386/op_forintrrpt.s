#################################################################
#								#
#	Copyright 2001 Sanchez Computer Associates, Inc.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

#	PAGE	,132
	.title	op_forintrrpt.s
	.sbttl	op_forintrrpt

#	.386
#	.MODEL	FLAT, C

.include "linkage.si"

	.DATA
.extern	neterr_pending
.extern	restart_pc

	.text
.extern	gvcmz_neterr
.extern	async_action
.extern	outofband_clear

# PUBLIC	op_forintrrpt
ENTRY op_forintrrpt
# smw 1999/12/14 C9905-001096 - was pop ever correct:  if so, still need it
#	popl	%eax
#	subl	$6,%eax
#	movl	%eax,restart_pc
	cmpb	$0,neterr_pending
	je	l1
	call	outofband_clear
	pushl	$0
	call	gvcmz_neterr
	addl	$4,%esp
l1:	pushl	$0
	call	async_action
	addl    $4,%esp
	ret
# op_forintrrpt ENDP

# END
