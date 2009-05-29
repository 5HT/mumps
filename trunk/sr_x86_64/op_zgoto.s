#################################################################
#								#
#	Copyright 2007 Fidelity Information Services, Inc	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

#      Args:
#              level   - stack frame nesting level to which to transfer control
#              rhdaddr - address of routine header of routine containing the label
#              labaddr - address of address of offset into routine to which to transfer cont
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
	addq	$8,REG_SP		#Pass return address
	pushq	REG64_ARG2	#labaddr at 8(REG_SP)
	pushq	REG64_ARG1	#rhdaddr at 0(REG_SP)
	call	golevel		#Incoming REG64_ARG0 goes as input to golevel
	popq	REG64_ARG0
	popq	REG64_ARG1
	cmpq	$0,REG64_ARG0	#rhdaddr = 0
	je	l2
	cmpq	$0,REG64_ARG1	#labaddr = 0
	je	l4
l1:	movq	(REG64_ARG1),REG64_ACCUM
	cmpq	$0,REG64_ACCUM
	je	l4
	movq	mrt_ptext_adr(REG64_ARG0),REG64_ARG1
	movl	0(REG64_ACCUM),REG32_ACCUM
	cltq
	movq	mrt_lit_ptr(REG64_ARG0),REG_LITERAL_BASE
	addq	REG64_ARG1,REG64_ACCUM
	movq	REG64_ACCUM,REG64_ARG2
	movq	mrt_lnk_ptr(REG64_ARG0),REG64_ARG1
	call	flush_jmp
	getframe
	movq	frame_pointer(REG_IP),REG64_ACCUM
	movq	REG_LITERAL_BASE,msf_literal_ptr_off(REG64_ACCUM)
	ret

l2:	#Stack pointer is passed to auto_zlink,
	#Value is poped in REG64_ARG1 below
	subq	$8,REG_SP
	movq	REG_SP,REG64_ARG1
	movq	frame_pointer(REG_IP),REG64_ACCUM
	movq	msf_mpc_off(REG64_ACCUM),REG64_ARG0
	call	auto_zlink
	cmpq	$0,REG64_RET0
	je	l3
	movq	REG64_RET0,REG64_ARG0
	popq	REG64_ARG1
	cmpq	$0,REG64_ARG1
	jne	l1
	jmp	l4

l3:
	movl	ERR_GTMCHECK(REG_IP),REG32_ARG1
	movl	$1,REG32_ARG0
	movb	$0,REG8_ACCUM             # variable length argument
	call	rts_error
	getframe
	ret

l4:	movl	ERR_LABELUNKNOWN(REG_IP),REG32_ARG1
	movl	$1,REG32_ARG0
	movb	$0,REG8_ACCUM             # variable length argumentt
	call	rts_error
	getframe
	ret
# op_zgoto ENDP

# END