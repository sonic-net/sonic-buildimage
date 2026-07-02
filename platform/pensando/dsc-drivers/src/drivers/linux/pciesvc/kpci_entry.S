/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
 */

/*
 *  Low Level Functions for kpcimgr (a.k.a. pciesvc glue layer)
 *
 *  Author: rob.gardner@oracle.com
 */
	
#include "kpci_constants.h"
	
#ifndef SYM_CODE_START
#define SYM_CODE_START(name) \
	.globl name		;\
        name:			;

#define SYM_CODE_END(name) \
	.size name, .-name
#endif

/* Calling conventions for printl: */
/* We use x12 as the branch link register and x13 as the first function arg */
#define	return_addr 	x12
#define arg0 		x13
	
/* defines for exception count and cpuid */
#define ex_count	tpidr_el0
#define cpuidreg	tpidr_el1
	
	/* macro to print a char given in x17 */
	.macro putc0
	mov	x16, PEN_UART
	strb	w17, [x16, #UART_THR]
11:	ldrb	w17, [x16, #UART_LSR]
	tbz	x17, #UART_THRE_BIT, 11b
	.endm

	/* macro to print a given literal char */
	.macro putc, c
	mov	x17, \c
	putc0
	.endm

	/* macro to print a literal string */
	.macro print, msg
	adr	x18, 77f
76:	ldrb	w17, [x18], #1
	cbz	x17, 78f
	putc0
	b	76b
77:	.asciz "\msg"
	.align	2
78:
	.endm
	
	/* macro to print a literal string with added cr/lf */
	.macro println, msg
	print	"\msg"
	print	"\r\n"
	.endm
	
	/* macro to print a system register */
	.macro	printsr, reg
	print	"\reg"
	putc	':'
	mrs	arg0, \reg
	adr	return_addr, 99f
	b	printl
99:	
	.endm
	
	/* print delineation marker */
	.macro	delineate, c
	mov	x15, #4
4:	putc	\c
	sub	x15, x15, 1
	cbnz	x15, 4b
	.endm
	
	/* macro to drop to exception level 1 */
	.macro	drop_to_el1
	mrs	x29, CurrentEL
	asr	x29, x29, 2
	tbnz	x29, #0, 88f	/* what? already at EL1 */
	putc	'2'
	
	/* set up EL2 exception vectors */
	adr	x29, xcpt_vectors
	msr	vbar_el2, x29
	isb

	/* do the actual drop to EL1 */
	putc	'#'
	adr	x29, 88f
	msr	elr_el2, x29
	eret
88:	
	putc	'1'
	msr	cpuidreg, x2	/* save cpu number */

	/* save original address of spin table */
	adr	x29, spin_table_start_addr
	str	x0, [x29]
	
	/* limit number of times the exception handler runs */
	mov	x16, 2
	msr	ex_count, x16
	putc	'!'

	/* set up EL1 exception vectors */
	adr	x29, xcpt_vectors
	msr	vbar_el1, x29
	isb
	putc	'V'

	/* unmask Serror */
	msr	daifclr, #(1|4|8)
	putc	'D'
	.endm

	/* macro to print the exception count value */
	.macro	print_ex_count
	putc	'('
	print	"ex_count:"
	mrs	x15, ex_count
	add	x17, x15, '0'
	putc0
	putc	')'
	.endm
	
	/* macro to print the cpu number */
	.macro	print_cpuid
	putc	'<'
	print	"CPU"
	mrs	x17, cpuidreg
	add	x17, x17, '0'
	putc0
	putc	'>'
	.endm

	/* macro to print exception level */
	.macro	print_el
	putc	'['
	print	"EL"
	mrs	x17, CurrentEL
	asr	x17, x17, 2
	add	x17, x17, '0'
	putc0
	putc	']'
	.endm
	
	
	/*
	 * This is the actual entry point for the first
	 * cpu to be hijacked. After dropping to EL1,
	 * we just need to set up a stack and we can
	 * jump to C code to do the real work.
	 */
	SYM_CODE_START(__kpcimgr_cpu_holding_pen)

	delineate '>'
	drop_to_el1

	/* load kstate base and set initial stack pointer */
	adr	x0, kstate_paddr
	ldr	x0, [x0]
	add	x3, x0, KSTATE_STACK_OFFSET
	mov	sp, x3
	
	/* jump to the real holding pen */
	bl	kpcimgr_cpu_holding_pen

	/* when C returns control here, we're done */
	putc	'='
	/* trap to EL2 and return to spin table */
	mov	x0, #1
	hvc	#0

	/* we should never get here */
	putc	'Q'
	b	.exit
	
	SYM_CODE_END(__kpcimgr_cpu_holding_pen)

	/*
	 * This is the entry point for the second hijacked
	 * cpu. Its job is to run the serial thread, which
	 * can interact with a console user should the need
	 * arise. Similar to the holding pen thread, we
	 * drop to EL1, set up our own unique stack, and
	 * jump to C.
	 */
	SYM_CODE_START(__kpcimgr_serial_thread)

	delineate ']'
	drop_to_el1
	
	putc	'\\'
	adr	x1, kstate_paddr
	ldr	x0, [x1]
	add	x3, x0, KSTATE_STACK_OFFSET
	sub	x3, x3, 0x2000	/* need a stack, different from other thread */
	mov	sp, x3
	bl	kpcimgr_serial_thread
	putc	'+'
	
	/* trap to EL2 and return to spin table */
	mov	x0, #1
	hvc	#0

	/* we should never get here */
	b	.exit
	SYM_CODE_END(__kpcimgr_serial_thread)
	
	/* C callable functions */
	
	/* long read_el(void) */
	SYM_CODE_START(read_el)
	mrs	x0, CurrentEL
	lsr	x0, x0, #2
	ret
	SYM_CODE_END(read_el)
	
	/* int cpuid(void) */
	SYM_CODE_START(cpuid)
	mrs	x0, cpuidreg
	ret
	SYM_CODE_END(cpuid)

	/* int release(void) */
	SYM_CODE_START(release)
	adr	x1, spin_table_start_addr
	ldr	x1, [x1]
	ldr	x0, [x1,#0x10]
	ret
	SYM_CODE_END(release)
	
/*
 * printl, basically performs a printf("[%lx]")
 *
 * We use a few registers indiscriminately, but I am
 * reasonably sure they are not used elsewhere
 */
#define shiftval 	x14
#define nchars 		x15
#define nibble 		x17
	
	SYM_CODE_START(printl)
	putc	'['
	mov	nchars, #0	/* number of characters actually printed */
	mov	shiftval, #64
.loop_top:
	sub	shiftval, shiftval, 4
	lsr	nibble, arg0, shiftval
	and	nibble, nibble, #0xf

	cbnz	nibble, .print		/* always print a non-zero nibble */
	cbz	shiftval, .print	/* always print the last nibble, even if zero */
	cbz	nchars, .loop_bottom	/* don't print leading zeros */
	
.print:	
	add	nchars, nchars, 1
	add	nibble, nibble, #'0'
	cmp	nibble, #'0'+0xA
	b.lt	1f
	add	nibble, nibble, #-0xA-'0'+'A'
1:	putc0
.loop_bottom:
	cbnz	shiftval, .loop_top
	
	putc	']'
	br	return_addr
	SYM_CODE_END(printl)

	/*
	 * Exception handler
	 *
	 * Mainly used to deal with Serror
	 *
	 * EL2 exceptions are fatal, but exceptions that arrive here
	 * at EL1 cause some useful output to the console, and return.
	 * The number of exceptions handled this way is limited to a few.
	 * The Serror exception is an exception to this rule.
	 */
	SYM_CODE_START(exception_handler)
	print_el
	mrs	x29, CurrentEL
	cmp	x29, #8
	b.ne	1f

	/* EL2 (fatal) */
	printsr elr_el2
	b	.exit

	/* EL1 */
1:	printsr	elr_el1
	printsr far_el1
	printsr	spsr_el1
	printsr esr_el1
	printsr sctlr_el1

	print_ex_count

	/* limit number of times we go through this code */
	/* to avoid an infinite stream of exceptions */
	mrs	x15, ex_count
	cbz	x15, .exit
	sub	x15, x15, 1
	msr	ex_count, x15
	
	print	"\r\n"
	eret

	/*
	 * Finish by jumping back to the original
	 * spin table
	 */
.exit:	
	print_el
	print_cpuid
	println	"done"
	adr	x29, spin_table_start_addr
	ldr	x0, [x29]
	br	x0
	
	SYM_CODE_END(exception_handler)
	
	.macro	hyper, c
	.align	7
	putc	\c
	b	.exit
	.endm
	
	.macro	exlog, c
	.align	7
	putc	\c
	print_el
	b	exception_handler
	.endm

	.macro	serror, c
	.align	7
	putc	\c
	mov	x16, #3
	msr	ex_count, x16
	b	exception_handler
	.endm
	

	.align 3
spin_table_start_addr:
	.dword
	
	/* The actual Exception Vector table, used for both EL1 and EL2 */
	.align 11
xcpt_vectors:
/* Current exception level with SP_EL0 */
	exlog	'A'	/* Sync */	
	exlog	'B'	/* IRQ/vIRQ */
	exlog	'C'	/* FIQ/cFIQ */
	exlog	'D'	/* SError/vSError */
/* Current exception level with SP_ELx, x>0 */
	hyper	'H'	/* Sync */
	exlog	'I'	/* IRQ/vIRQ */
	exlog	'Q'	/* FIQ/cFIQ */
	serror	'S'	/* SError/vSError */

	
