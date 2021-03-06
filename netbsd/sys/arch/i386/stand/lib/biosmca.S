/*	$NetBSD: biosmca.S,v 1.2 2001/05/14 22:14:47 jdolecek Exp $	*/

/*
 * Ported to boot 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 *
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
  Copyright 1988, 1989, 1990, 1991, 1992 
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/* extracted from netbsd:sys/arch/i386/stand/bios_disk.S */

#include <machine/asm.h>

#define	addr32	.byte 0x67
#define	data32	.byte 0x66

	.data
	.globl _C_LABEL(biosmca_ps2model)
_C_LABEL(biosmca_ps2model):	.long 0

	.text
/*
# BIOS call "INT 0x15 Function 0xc0" to read extended sys config info on PS/2
#	Return:		no return value
#
# This function initializes biosmca_ps2model with model number as
# identified by BIOS, if the machine is a PS/2 box (i.e. has MCA bus
# instead of ISA).
*/
ENTRY(biosmca)
	pushl	%ebp
	movl    %esp, %ebp
	pushl	%ebx
	push	%ecx
	push	%edx
	push	%esi
	push	%edi
	push	%eax

	call	_C_LABEL(prot_to_real)	# enter real mode

	# zero %cx
	data32
	xorl	%cx, %cx

	data32
	xorl	%ax, %ax
	movb	$0xc0, %ah	# subfunction
	int	$0x15
	jc	back

	# check feature byte 1 if MCA bus present and replaces ISA
	addr32
	movb	%es:5(%ebx), %ax
	andw	$0x02, %ax	# bit 1 set means MCA instead of ISA
	cmpw	$0x02, %ax	# see also arch/i386/mca/mca_machdep.c
	jne	back
	
	# save model and submodel bytes to %cx
	addr32
	movb	%es:2(%ebx), %ch	# model (1 byte)
	addr32
	movb	%es:3(%ebx), %cl	# submodel (1 byte)

back:
	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	# save model
	movl	%ecx, _C_LABEL(biosmca_ps2model)

	pop	%eax
	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ecx
	popl	%ebx
	popl	%ebp
	ret
