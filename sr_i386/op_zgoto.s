#################################################################
#								#
#	Copyright 2001, 2002 Sanchez Computer Associates, Inc.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

#	PAGE	,132
	.title	op_zgoto.s

#	.386
#	.MODEL	FLAT, C

.include "linkage.si"
	.INCLUDE	"g_msf.si"

	.sbttl	op_zgoto
#	PAGE	+
	.DATA
.extern	ERR_GTMCHECK
.extern	ERR_LABELUNKNOWN
.extern	frame_pointer

	.text
.extern	auto_zlink
.extern	flush_jmp
.extern	golevel
.extern	rts_error

# PUBLIC	op_zgoto
ENTRY op_zgoto
	putframe
	addl	$4,%esp
	call	golevel
	addl	$4,%esp
	popl	%edx
	cmpl	$0,%edx
	je	l2
	popl	%eax
	cmpl	$0,%eax
	je	l4
l1:	movl	(%eax),%eax
	addl	mrt_curr_ptr(%edx),%eax
	addl	%edx,%eax
	pushl	%eax
	pushl	$0
	pushl	%edx
	call	flush_jmp
	addl	$12,%esp
	getframe
	ret

l2:	movl	%esp,%eax
	pushl	%eax
	movl	frame_pointer,%eax
	pushl	msf_mpc_off(%eax)
	call	auto_zlink
	addl	$8,%esp
	cmpl	$0,%eax
	je	l3
	movl	%eax,%edx
	popl	%eax
	cmpl	$0,%eax
	jne	l1
	jmp	l4

l3:	addl	$4,%esp
	pushl	ERR_GTMCHECK
	pushl	$1
	call	rts_error
	addl	$8,%esp
	getframe
	ret

l4:	pushl	ERR_LABELUNKNOWN
	pushl	$1
	call	rts_error
	addl	$8,%esp
	getframe
	ret
# op_zgoto ENDP

# END
