/*	$NetBSD: fpu_emu.c,v 1.3 2001/07/22 11:29:44 wiz Exp $ */

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fpu.c	8.1 (Berkeley) 6/11/93
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/signalvar.h>

#include <powerpc/instr.h>
#include <machine/reg.h>
#include <machine/fpu.h>

#include <powerpc/fpu/fpu_emu.h>
#include <powerpc/fpu/fpu_extern.h>


/* FPSR exception masks */
#define FPSR_EX_MSK	(FPSCR_VX|FPSCR_OX|FPSCR_UX|FPSCR_ZX|		\
			FPSCR_XX|FPSCR_VXSNAN|FPSCR_VXISI|FPSCR_VXIDI|	\
			FPSCR_VXZDZ|FPSCR_VXIMZ|FPSCR_VXVC|FPSCR_VXSOFT|\
			FPSCR_VXSQRT|FPSCR_VXCVI)
#define	FPSR_EX		(FPSCR_VE|FPSCR_OE|FPSCR_UE|FPSCR_ZE|FPSCR_XE)
#define	FPSR_EXOP	(FPSR_EX_MSK&(~FPSR_EX))


int fpe_debug = 0;

#ifdef DDB
extern vaddr_t opc_disasm(vaddr_t loc, int opcode);
#endif

#ifdef DEBUG
/*
 * Dump a `fpn' structure.
 */
void
fpu_dumpfpn(struct fpn *fp)
{
	static char *class[] = {
		"SNAN", "QNAN", "ZERO", "NUM", "INF"
	};

	printf("%s %c.%x %x %x %xE%d", class[fp->fp_class + 2],
		fp->fp_sign ? '-' : ' ',
		fp->fp_mant[0],	fp->fp_mant[1],
		fp->fp_mant[2], fp->fp_mant[3], 
		fp->fp_exp);
}
#endif

/*
 * fpu_execute returns the following error numbers (0 = no error):
 */
#define	FPE		1	/* take a floating point exception */
#define	NOTFPU		2	/* not an FPU instruction */
#define	FAULT		3


/*
 * Emulate a floating-point instruction.
 * Return zero for success, else signal number.
 * (Typically: zero, SIGFPE, SIGILL, SIGSEGV)
 */
int
fpu_emulate(struct trapframe *frame, struct fpreg *fpf)
{
	static union instr insn;
	static struct fpemu fe;
	static int lastill = 0;
	int sig;

	/* initialize insn.is_datasize to tell it is *not* initialized */
	fe.fe_fpstate = fpf;
	fe.fe_cx = 0;

	/* always set this (to avoid a warning) */

	if (copyin((void *) (frame->srr0), &insn.i_int, sizeof (insn.i_int))) {
#ifdef DEBUG
		printf("fpu_emulate: fault reading opcode\n");
#endif
		return SIGSEGV;
	}

	DPRINTF(FPE_EX, ("fpu_emulate: emulating insn %x at %p\n",
	    insn.i_int, (void *)frame->srr0));


	if ((insn.i_any.i_opcd == OPC_TWI) ||
	    ((insn.i_any.i_opcd == OPC_integer_31) &&
	    (insn.i_x.i_xo == OPC31_TW))) {
		/* Check for the two trap insns. */
		DPRINTF(FPE_EX, ("fpu_emulate: SIGTRAP\n"));
		return (SIGTRAP);
	}
	sig = 0;
	switch (fpu_execute(frame, &fe, &insn)) {
	case 0:
		DPRINTF(FPE_EX, ("fpu_emulate: success\n"));
		frame->srr0 += 4;
		break;

	case FPE:
		DPRINTF(FPE_EX, ("fpu_emulate: SIGFPE\n"));
		sig = SIGFPE;
		break;

	case FAULT:
		DPRINTF(FPE_EX, ("fpu_emulate: SIGSEGV\n"));
		sig = SIGSEGV;
		break;

	case NOTFPU:
	default:
		DPRINTF(FPE_EX, ("fpu_emulate: SIGILL\n"));
#ifdef DEBUG
		if (fpe_debug & FPE_EX) {
			printf("fpu_emulate:  illegal insn %x at %p:",
			insn.i_int, (void *) (frame->srr0));
			opc_disasm((vaddr_t)(frame->srr0), insn.i_int);
		}
#endif
		/*
		* XXXX retry an illegal insn once due to cache issues.
		*/
		if (lastill == frame->srr0) {
			sig = SIGILL;
#ifdef DEBUG
			if (fpe_debug & FPE_EX)
				Debugger();
#endif
		}
		lastill = frame->srr0;
		break;
	}

	return (sig);
}

/*
 * Execute an FPU instruction (one that runs entirely in the FPU; not
 * FBfcc or STF, for instance).  On return, fe->fe_fs->fs_fsr will be
 * modified to reflect the setting the hardware would have left.
 *
 * Note that we do not catch all illegal opcodes, so you can, for instance,
 * multiply two integers this way.
 */
int
fpu_execute(struct trapframe *tf, struct fpemu *fe, union instr *insn)
{
	struct fpn *fp;
	union instr instr = *insn;
	int *a;
	vaddr_t addr;
	int ra, rb, rc, rt, type, mask, fsr, cx, bf, setcr, cond;
	struct fpreg *fs;

	/* Setup work. */
	fp = NULL;
	fs = fe->fe_fpstate;
	fe->fe_fpscr = ((int *)&fs->fpscr)[1];

	/*
	 * On PowerPC all floating point values are stored in registers
	 * as doubles, even when used for single precision operations.
	 */
	type = FTYPE_DBL;
	cond = instr.i_any.i_rc;
	setcr = 0;

#if defined(DDB) && defined(DEBUG)
	if (fpe_debug & FPE_EX) {
		vaddr_t loc = tf->srr0;

		printf("Trying to emulate: %p ", (void *)loc);
		opc_disasm(loc, instr.i_int);
	}
#endif

	/*
	 * `Decode' and execute instruction.
	 */

	if ((instr.i_any.i_opcd >= OPC_LFS && instr.i_any.i_opcd <= OPC_STFDU) ||
	    instr.i_any.i_opcd == OPC_integer_31) {
		/*
		 * Handle load/store insns:
		 *
		 * Convert to/from single if needed, calculate addr,
		 * and update index reg if needed.
		 */
		double buf;
		size_t size = sizeof(float);
		int store, update;

		cond = 0; /* ld/st never set condition codes */


		if (instr.i_any.i_opcd == OPC_integer_31) {
			if (instr.i_x.i_xo == OPC31_STFIWX) {
				/* Store as integer */
				ra = instr.i_x.i_ra;
				rb = instr.i_x.i_rb;
				DPRINTF(FPE_INSN, ("reg %d has %x reg %d has %x\n",
					ra, tf->fixreg[ra], rb, tf->fixreg[rb]));

				addr = tf->fixreg[rb];
				if (ra != 0)
					addr += tf->fixreg[ra];
				rt = instr.i_x.i_rt;
				a = (int *)&fs->fpreg[rt];
				DPRINTF(FPE_INSN,
					("fpu_execute: Store INT %x at %p\n",
						a[1], (void *)addr));
				if (copyout(&a[1], (void *)addr, sizeof(int)))
					return (FAULT);
				return (0);
			}

			if ((instr.i_x.i_xo & OPC31_FPMASK) != OPC31_FPOP)
				/* Not an indexed FP load/store op */
				return (NOTFPU);

			store = (instr.i_x.i_xo & 0x80);
			if (instr.i_x.i_xo & 0x40)
				size = sizeof(double);
			else
				type = FTYPE_SNG;
			update = (instr.i_x.i_xo & 0x20);
			
			/* calculate EA of load/store */
			ra = instr.i_x.i_ra;
			rb = instr.i_x.i_rb;
			DPRINTF(FPE_INSN, ("reg %d has %x reg %d has %x\n",
				ra, tf->fixreg[ra], rb, tf->fixreg[rb]));
			addr = tf->fixreg[rb];
			if (ra != 0)
				addr += tf->fixreg[ra];
			rt = instr.i_x.i_rt;
		} else {
			store = instr.i_d.i_opcd & 0x4;
			if (instr.i_d.i_opcd & 0x2)
				size = sizeof(double);
			else
				type = FTYPE_SNG;
			update = instr.i_d.i_opcd & 0x1;

			/* calculate EA of load/store */
			ra = instr.i_d.i_ra;
			addr = instr.i_d.i_d;
			DPRINTF(FPE_INSN, ("reg %d has %x displ %lx\n",
				ra, tf->fixreg[ra], addr));
			if (ra != 0)
				addr += tf->fixreg[ra];
			rt = instr.i_d.i_rt;
		}

		if (update && ra == 0)
			return (NOTFPU);

		if (store) {
			/* Store */
			if (type != FTYPE_DBL) {
				DPRINTF(FPE_INSN,
					("fpu_execute: Store SNG at %p\n",
						(void *)addr));
				fpu_explode(fe, fp = &fe->fe_f1, FTYPE_DBL, rt);
				fpu_implode(fe, fp, type, (u_int *)&buf);
				if (copyout(&buf, (void *)addr, size))
					return (FAULT);
			} else {
				DPRINTF(FPE_INSN, 
					("fpu_execute: Store DBL at %p\n",
						(void *)addr));
				if (copyout(&fs->fpreg[rt], (void *)addr, size))
					return (FAULT);
			}
		} else {
			/* Load */
			DPRINTF(FPE_INSN, ("fpu_execute: Load from %p\n",
				(void *)addr));
			if (copyin((const void *)addr, &fs->fpreg[rt], size))
				return (FAULT);
			if (type != FTYPE_DBL) {
				fpu_explode(fe, fp = &fe->fe_f1, type, rt);
				fpu_implode(fe, fp, FTYPE_DBL, 
					(u_int *)&fs->fpreg[rt]);
			}
		}
		if (update) 
			tf->fixreg[ra] = addr;
		/* Complete. */
		return (0);
#ifdef notyet
	} else if (instr.i_any.i_opcd == OPC_load_st_62) {
		/* These are 64-bit extenstions */
		return (NOTFPU);
#endif
	} else if (instr.i_any.i_opcd == OPC_sp_fp_59 ||
		instr.i_any.i_opcd == OPC_dp_fp_63) {


		if (instr.i_any.i_opcd == OPC_dp_fp_63 &&
		    !(instr.i_a.i_xo & OPC63M_MASK)) {
			/* Format X */
			rt = instr.i_x.i_rt;
			ra = instr.i_x.i_ra;
			rb = instr.i_x.i_rb;


			/* One of the special opcodes.... */
			switch (instr.i_x.i_xo) {
			case	OPC63_FCMPU:
				DPRINTF(FPE_INSN, ("fpu_execute: FCMPU\n"));
				rt >>= 2;
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fpu_compare(fe, 0);
				/* Make sure we do the condition regs. */
				cond = 0;
				/* N.B.: i_rs is already left shifted by two. */
				bf = instr.i_x.i_rs & 0xfc;
				setcr = 1;
				break;

			case	OPC63_FRSP:
				/*
				 * Convert to single: 
				 *
				 * PowerPC uses this to round a double
				 * precision value to single precision,
				 * but values in registers are always 
				 * stored in double precision format.
				 */
				DPRINTF(FPE_INSN, ("fpu_execute: FRSP\n"));
				fpu_explode(fe, fp = &fe->fe_f1, FTYPE_DBL, rb);
				fpu_implode(fe, fp, FTYPE_SNG, 
					(u_int *)&fs->fpreg[rt]);
				fpu_explode(fe, fp = &fe->fe_f1, FTYPE_SNG, rt);
				type = FTYPE_DBL;
				break;
			case	OPC63_FCTIW:
			case	OPC63_FCTIWZ:
				DPRINTF(FPE_INSN, ("fpu_execute: FCTIW\n"));
				fpu_explode(fe, fp = &fe->fe_f1, type, rb);
				type = FTYPE_INT;
				break;
			case	OPC63_FCMPO:
				DPRINTF(FPE_INSN, ("fpu_execute: FCMPO\n"));
				rt >>= 2;
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fpu_compare(fe, 1);
				/* Make sure we do the condition regs. */
				cond = 0;
				/* N.B.: i_rs is already left shifted by two. */
				bf = instr.i_x.i_rs & 0xfc;
				setcr = 1;
				break;
			case	OPC63_MTFSB1:
				DPRINTF(FPE_INSN, ("fpu_execute: MTFSB1\n"));
				fe->fe_fpscr |= 
					(~(FPSCR_VX|FPSR_EX) & (1<<(31-rt)));
				break;
			case	OPC63_FNEG:
				DPRINTF(FPE_INSN, ("fpu_execute: FNEGABS\n"));
				memcpy(&fs->fpreg[rt], &fs->fpreg[rb],
					sizeof(double));
				a = (int *)&fs->fpreg[rt];
				*a ^= (1 << 31);
				break;
			case	OPC63_MCRFS:
				DPRINTF(FPE_INSN, ("fpu_execute: MCRFS\n"));
				cond = 0;
				rt &= 0x1c;
				ra &= 0x1c;
				/* Extract the bits we want */
				mask = (fe->fe_fpscr >> (28 - ra)) & 0xf;
				/* Clear the bits we copied. */
				fe->fe_cx =
					(FPSR_EX_MSK | (0xf << (28 - ra)));
				fe->fe_fpscr &= fe->fe_cx;
				/* Now shove them in the right part of cr */
				tf->cr &= ~(0xf << (28 - rt));
				tf->cr |= (mask << (28 - rt));
				break;
			case	OPC63_MTFSB0:
				DPRINTF(FPE_INSN, ("fpu_execute: MTFSB0\n"));
				fe->fe_fpscr &=
					((FPSCR_VX|FPSR_EX) & ~(1<<(31-rt)));
				break;
			case	OPC63_FMR:
				DPRINTF(FPE_INSN, ("fpu_execute: FMR\n"));
				memcpy(&fs->fpreg[rt], &fs->fpreg[rb],
					sizeof(double));
				break;
			case	OPC63_MTFSFI:
				DPRINTF(FPE_INSN, ("fpu_execute: MTFSFI\n"));
				rb >>= 1;
				rt &= 0x1c; /* Already left-shifted 4 */
				fe->fe_cx = rb << (28 - rt);
				mask = 0xf<<(28 - rt);
				fe->fe_fpscr = (fe->fe_fpscr & ~mask) | 
					fe->fe_cx;
/* XXX weird stuff about OX, FX, FEX, and VX should be handled */
				break;
			case	OPC63_FNABS:
				DPRINTF(FPE_INSN, ("fpu_execute: FABS\n"));
				memcpy(&fs->fpreg[rt], &fs->fpreg[rb],
					sizeof(double));
				a = (int *)&fs->fpreg[rt];
				*a |= (1 << 31);
				break;
			case	OPC63_FABS:
				DPRINTF(FPE_INSN, ("fpu_execute: FABS\n"));
				memcpy(&fs->fpreg[rt], &fs->fpreg[rb],
					sizeof(double));
				a = (int *)&fs->fpreg[rt];
				*a &= ~(1 << 31);
				break;
			case	OPC63_MFFS:
				DPRINTF(FPE_INSN, ("fpu_execute: MFFS\n"));
				memcpy(&fs->fpreg[rt], &fs->fpscr,
					sizeof(fs->fpscr));
				break;
			case	OPC63_MTFSF:
				DPRINTF(FPE_INSN, ("fpu_execute: MTFSF\n"));
				if ((rt = instr.i_xfl.i_flm) == -1)
					mask = -1;
				else {
					mask = 0;
					/* Convert 1 bit -> 4 bits */
					for (ra = 0; ra < 8; ra ++)
						if (rt & (1<<ra))
							mask |= (0xf<<(4*ra));
				}
				a = (int *)&fs->fpreg[rt];
				fe->fe_cx = mask & a[1];
				fe->fe_fpscr = (fe->fe_fpscr&~mask) | 
					(fe->fe_cx);
/* XXX weird stuff about OX, FX, FEX, and VX should be handled */
				break;
			case	OPC63_FCTID:
			case	OPC63_FCTIDZ:
				DPRINTF(FPE_INSN, ("fpu_execute: FCTID\n"));
				fpu_explode(fe, fp = &fe->fe_f1, type, rb);
				type = FTYPE_LNG;
				break;
			case	OPC63_FCFID:
				DPRINTF(FPE_INSN, ("fpu_execute: FCFID\n"));
				type = FTYPE_LNG;
				fpu_explode(fe, fp = &fe->fe_f1, type, rb);
				type = FTYPE_DBL;
				break;
			default:
				return (NOTFPU);
				break;
			}
		} else {
			/* Format A */
			rt = instr.i_a.i_frt;
			ra = instr.i_a.i_fra;
			rb = instr.i_a.i_frb;
			rc = instr.i_a.i_frc;

			type = FTYPE_SNG;
			if (instr.i_any.i_opcd & 0x4)
				type = FTYPE_DBL;
			switch ((unsigned int)instr.i_a.i_xo) {
			case	OPC59_FDIVS:
				DPRINTF(FPE_INSN, ("fpu_execute: FDIV\n"));
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fp = fpu_div(fe);
				break;
			case	OPC59_FSUBS:
				DPRINTF(FPE_INSN, ("fpu_execute: FSUB\n"));
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fp = fpu_sub(fe);
				break;
			case	OPC59_FADDS:
				DPRINTF(FPE_INSN, ("fpu_execute: FADD\n"));
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fp = fpu_add(fe);
				break;
			case	OPC59_FSQRTS:
				DPRINTF(FPE_INSN, ("fpu_execute: FSQRT\n"));
				fpu_explode(fe, &fe->fe_f1, type, rb);
				fp = fpu_sqrt(fe);
				break;
			case	OPC63M_FSEL:
				DPRINTF(FPE_INSN, ("fpu_execute: FSEL\n"));
				a = (int *)&fe->fe_fpstate->fpreg[ra];
				if ((*a & 0x80000000) && (*a & 0x7fffffff)) 
					/* fra < 0 */
					rc = rb;
				DPRINTF(FPE_INSN, ("f%d => f%d\n", rc, rt));
				memcpy(&fs->fpreg[rt], &fs->fpreg[rc],
					sizeof(double));
				break;
			case	OPC59_FRES:
				DPRINTF(FPE_INSN, ("fpu_execute: FPRES\n"));
				fpu_explode(fe, &fe->fe_f1, type, rb);
				fp = fpu_sqrt(fe);
				/* now we've gotta overwrite the dest reg */
				*((int *)&fe->fe_fpstate->fpreg[rt]) = 1;
				fpu_explode(fe, &fe->fe_f1, FTYPE_INT, rt);
				fpu_div(fe);
				break;
			case	OPC59_FMULS:
				DPRINTF(FPE_INSN, ("fpu_execute: FMUL\n"));
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rc);
				fp = fpu_mul(fe);
				break;
			case	OPC63M_FRSQRTE:
				/* Reciprocal sqrt() estimate */
				DPRINTF(FPE_INSN, ("fpu_execute: FRSQRTE\n"));
				fpu_explode(fe, &fe->fe_f1, type, rb);
				fe->fe_f2 = *fp;
				/* now we've gotta overwrite the dest reg */
				*((int *)&fe->fe_fpstate->fpreg[rt]) = 1;
				fpu_explode(fe, &fe->fe_f1, FTYPE_INT, rt);
				fpu_div(fe);
				break;
			case	OPC59_FMSUBS:
				DPRINTF(FPE_INSN, ("fpu_execute: FMULSUB\n"));
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rc);
				fp = fpu_mul(fe);
				fe->fe_f1 = *fp;
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fp = fpu_sub(fe);
				break;
			case	OPC59_FMADDS:
				DPRINTF(FPE_INSN, ("fpu_execute: FMULADD\n"));
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rc);
				fp = fpu_mul(fe);
				fe->fe_f1 = *fp;
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fp = fpu_add(fe);
				break;
			case	OPC59_FNMSUBS:
				DPRINTF(FPE_INSN, ("fpu_execute: FNMSUB\n"));
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rc);
				fp = fpu_mul(fe);
				fe->fe_f1 = *fp;
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fp = fpu_sub(fe);
				/* Negate */
				fp->fp_sign ^= 1;
				break;
			case	OPC59_FNMADDS:
				DPRINTF(FPE_INSN, ("fpu_execute: FNMADD\n"));
				fpu_explode(fe, &fe->fe_f1, type, ra);
				fpu_explode(fe, &fe->fe_f2, type, rc);
				fp = fpu_mul(fe);
				fe->fe_f1 = *fp;
				fpu_explode(fe, &fe->fe_f2, type, rb);
				fp = fpu_add(fe);
				/* Negate */
				fp->fp_sign ^= 1;
				break;
			default:
				return (NOTFPU);
				break;
			}
		}
	} else {
		return (NOTFPU);
	}

	/*
	 * ALU operation is complete.  Collapse the result and then check
	 * for exceptions.  If we got any, and they are enabled, do not
	 * alter the destination register, just stop with an exception.
	 * Otherwise set new current exceptions and accrue.
	 */
	if (fp)
		fpu_implode(fe, fp, type, (u_int *)&fs->fpreg[rt]);
	cx = fe->fe_cx;
	fsr = fe->fe_fpscr;
	if (cx != 0) {
		fsr &= ~FPSCR_FX;
		if ((cx^fsr)&FPSR_EX_MSK)
			fsr |= FPSCR_FX;
		mask = fsr & FPSR_EX;
		mask <<= (25-3);
		if (cx & mask) 
			fsr |= FPSCR_FEX;
		if (cx & FPSCR_FPRF) {
			/* Need to replace CC */
			fsr &= ~FPSCR_FPRF;
		}
		if (cx & (FPSR_EXOP))
			fsr |= FPSCR_VX;
		fsr |= cx;
		DPRINTF(FPE_INSN, ("fpu_execute: cx %x, fsr %x\n", cx, fsr));
	}

	if (cond) {
		cond = fsr & 0xf0000000;
		/* Isolate condition codes */
		cond >>= 28;
		/* Move fpu condition codes to cr[1] */
		tf->cr &= (0x0f000000);
		tf->cr |= (cond<<24);
		DPRINTF(FPE_INSN, ("fpu_execute: cr[1] <= %x\n", cond));
	}

	if (setcr) {
		cond = fsr & FPSCR_FPCC;
		/* Isolate condition codes */
		cond <<= 16;
		/* Move fpu condition codes to cr[1] */
		tf->cr &= ~(0xf0000000>>bf);
		tf->cr |= (cond>>bf);
		DPRINTF(FPE_INSN, ("fpu_execute: cr[%d] (cr=%x) <= %x\n", bf/4, tf->cr, cond));
	}

	((int *)&fs->fpscr)[1] = fsr;
	if (fsr & FPSCR_FEX)
		return(FPE);
	return (0);	/* success */
}
