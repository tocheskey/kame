#
# Copyright (c) 1998 Robert Nordier
# All rights reserved.
#
# Redistribution and use in source and binary forms are freely
# permitted provided that the above copyright notice and this
# paragraph and the following disclaimer are duplicated in all
# such forms.
#
# This software is provided "AS IS" and without any express or
# implied warranties, including, without limitation, the implied
# warranties of merchantability and fitness for a particular
# purpose.
#

#	$Id: btxldr.s,v 1.1.2.1 1999/02/06 07:37:13 kato Exp $

#
# Prototype BTX loader program, written in a couple of hours.  The
# real thing should probably be more flexible, and in C.
#

#
# Memory locations.
#
		.set MEM_STUB,0x600		# Real mode stub
		.set MEM_ESP,0x1000		# New stack pointer
		.set MEM_TBL,0x5000		# BTX page tables
		.set MEM_ENTRY,0x9010		# BTX entry point
		.set MEM_DATA,0x101000		# Data segment
#
# Segment selectors.
#
		.set SEL_SCODE,0x8		# 4GB code
		.set SEL_SDATA,0x10		# 4GB data
		.set SEL_RCODE,0x18		# 64K code
		.set SEL_RDATA,0x20		# 64K data
#
# Paging constants.
#
		.set PAG_SIZ,0x1000		# Page size
		.set PAG_ENT,0x4		# Page entry size
#
# Screen constants.
#
.ifdef PC98
		.set SCR_MAT,0xe1		# Mode/attribute
.else
		.set SCR_MAT,0x7		# Mode/attribute
.endif
		.set SCR_COL,0x50		# Columns per row
		.set SCR_ROW,0x19		# Rows per screen
#
# BIOS Data Area locations.
#
.ifdef PC98
		.set BDA_MEM,0xa1501		# Free memory
		.set BDA_POS,0xa153e		# Cursor position
.else
		.set BDA_MEM,0x413		# Free memory
		.set BDA_SCR,0x449		# Video mode
		.set BDA_POS,0x450		# Cursor position
.endif
#
# Required by aout gas inadequacy.
#
		.set SIZ_STUB,0x1a		# Size of stub
#
# We expect to be loaded by boot2 at 0x100000.
#
		.globl start
#
# BTX program loader for ELF clients.
#
start:		cld				# String ops inc
.ifdef PC98
		cli
gdcwait.1:	inb $0x60,%al
		testb $0x04,%al
		jz gdcwait.1
		movb $0xe0,%al
		outb %al,$0x62
		nop
gdcwait.2:	inb $0x60,%al
		testb $0x01,%al
		jz gdcwait.2
		inb $0x62,%al
		movb %al,%dl
		inb $0x62,%al
		movb %al,%dh
		inb $0x62,%al
		inb $0x62,%al
		inb $0x62,%al
		shlw $1,%dx
		movl $BDA_POS,%ebx
		movw %dx,(%ebx)
.endif
		movl $m_logo,%esi		# Identify
		call putstr			#  ourselves
		movzwl BDA_MEM,%eax		# Get base memory
.ifdef PC98
		andl $0x7,%eax
		incl %eax
		shll $0x11,%eax			#  in bytes
.else
		shll $0xa,%eax			#  in bytes
.endif
		movl %eax,%ebp			# Base of user stack
		movl $m_mem,%esi		# Display
		call dhexout			#  amount of
		call dputstr			#  base memory
		lgdt gdtdesc			# Load new GDT
#
# Relocate caller's arguments.
#
		movl $m_esp,%esi		# Display
		movl %esp,%eax			#  caller's
		call dhexout			#  stack
		call dputstr			#  pointer
		movl $m_args,%esi		# Format string
		leal 0x4(%esp,1),%ebx		# First argument
		movl $0x6,%ecx			# Count
start.1:	movl (%ebx),%eax		# Get argument and
		addl $0x4,%ebx			#  bump pointer
		call dhexout			# Display it
		loop start.1			# Till done
		call dputstr			# End message
		movl $0x48,%ecx 		# Allocate space
		subl %ecx,%ebp			#  for bootinfo
		movl 0x18(%esp,1),%esi		# Source
		movl %ebp,%edi			# Destination
		rep				# Copy
		movsb				#  it
		movl %ebp,0x18(%esp,1)		# Update pointer
		movl $m_rel_bi,%esi		# Display
		movl %ebp,%eax			#  bootinfo
		call dhexout			#  relocation
		call dputstr			#  message
		movl $0x18,%ecx 		# Allocate space
		subl %ecx,%ebp			#  for arguments
		leal 0x4(%esp,1),%esi		# Source
		movl %ebp,%edi			# Destination
		rep				# Copy
		movsb				#  them
		movl $m_rel_args,%esi		# Display
		movl %ebp,%eax			#  argument
		call dhexout			#  relocation
		call dputstr			#  message
#
# Set up BTX kernel.
#
		movl $MEM_ESP,%esp		# Set up new stack
		movl $MEM_DATA,%ebx		# Data segment
		movl $m_vers,%esi		# Display BTX
		call putstr			#  version message
		movb 0x5(%ebx),%al		# Get major version
		addb $'0',%al			# Display
		call putchr			#  it
		movb $'.',%al			# And a
		call putchr			#  dot
		movb 0x6(%ebx),%al		# Get minor
		xorb %ah,%ah			#  version
		movb $0xa,%dl			# Divide
		divb %dl,%al			#  by 10
		addb $'0',%al			# Display
		call putchr			#  tens
		movb %ah,%al			# Get units
		addb $'0',%al			# Display
		call putchr			#  units
		call putstr			# End message
		movl %ebx,%esi			# BTX image
		movzwl 0x8(%ebx),%edi		# Compute
		orl $PAG_SIZ/PAG_ENT-1,%edi	#  the
		incl %edi			#  BTX
		shll $0x2,%edi			#  load
		addl $MEM_TBL,%edi		#  address
		pushl %edi			# Save
		movzwl 0xa(%ebx),%ecx		# Image size
		pushl %ecx			# Save
		rep				# Relocate
		movsb				#  BTX
		movl %esi,%ebx			# Keep place
		movl $m_rel_btx,%esi		# Restore
		popl %eax			#  parameters
		call dhexout			#  and
		popl %ebp			#  display
		movl %ebp,%eax			#  the
		call dhexout			#  relocation
		call dputstr			#  message
		addl $PAG_SIZ,%ebp		# Display
		movl $m_base,%esi		#  the
		movl %ebp,%eax			#  user
		call dhexout			#  base
		call dputstr			#  address
#
# Set up ELF-format client program.
#
		cmpl $0x464c457f,(%ebx) 	# ELF magic number?
		je start.3			# Yes
		movl $e_fmt,%esi		# Display error
		call putstr			#  message
start.2:	jmp start.2			# Hang
start.3:	movl $m_elf,%esi		# Display ELF
		call dputstr			#  message
		movl $m_segs,%esi		# Format string
		movl $0x2,%edi			# Segment count
		movl 0x1c(%ebx),%edx		# Get e_phoff
		addl %ebx,%edx			# To pointer
		movzwl 0x2c(%ebx),%ecx		# Get e_phnum
start.4:	cmpl $0x1,(%edx)		# Is p_type PT_LOAD?
		jne start.6			# No
		movl 0x4(%edx),%eax		# Display
		call dhexout			#  p_offset
		movl 0x8(%edx),%eax		# Display
		call dhexout			#  p_vaddr
		movl 0x10(%edx),%eax		# Display
		call dhexout			#  p_filesz
		movl 0x14(%edx),%eax		# Display
		call dhexout			#  p_memsz
		call dputstr			# End message
		pushl %esi			# Save
		pushl %edi			#  working
		pushl %ecx			#  registers
		movl 0x4(%edx),%esi		# Get p_offset
		addl %ebx,%esi			#  as pointer
		movl 0x8(%edx),%edi		# Get p_vaddr
		addl %ebp,%edi			#  as pointer
		movl 0x10(%edx),%ecx		# Get p_filesz
		rep				# Set up
		movsb				#  segment
		movl 0x14(%edx),%ecx		# Any bytes
		subl 0x10(%edx),%ecx		#  to zero?
		jz start.5			# No
		xorb %al,%al			# Then
		rep				#  zero
		stosb				#  them
start.5:	popl %ecx			# Restore
		popl %edi			#  working
		popl %esi			#  registers
		decl %edi			# Segments to do
		je start.7			# If none
start.6:	addl $0x20,%edx 		# To next entry
		loop start.4			# Till done
start.7:	movl $m_done,%esi		# Display done
		call dputstr			#  message
		movl $start.8,%esi		# Real mode stub
		movl $MEM_STUB,%edi		# Destination
		movl $SIZ_STUB,%ecx		# Size
		rep				# Relocate
		movsb				#  it
		ljmp $SEL_RCODE,$MEM_STUB	# To 16-bit code
start.8:	xorl %eax,%eax			# Data
		movb $SEL_RDATA,%al		#  selector
		movl %eax,%ss			# Reload SS
		movl %eax,%ds			# Reset
		movl %eax,%es			#  other
		movl %eax,%fs			#  segment
		movl %eax,%gs			#  limits
		movl %cr0,%eax			# Switch to
		decl %eax			#  real
		movl %eax,%cr0			#  mode
		.byte 0xea			# Jump to
		.word MEM_ENTRY 		# BTX entry
		.word 0x0			#  point
start.9:
#
# Output message [ESI] followed by EAX in hex.
#
dhexout:
.ifndef BTXLDR_VERBOSE
		ret
.endif
hexout: 	pushl %eax			# Save
		call putstr			# Display message
		popl %eax			# Restore
		pushl %esi			# Save
		pushl %edi			#  caller's
		movl $buf,%edi			# Buffer
		pushl %edi			# Save
		call hex32			# To hex
		xorb %al,%al			# Terminate
		stosb				#  string
		popl %esi			# Restore
hexout.1:	lodsb				# Get a char
		cmpb $'0',%al			# Leading zero?
		je hexout.1			# Yes
		testb %al,%al			# End of string?
		jne hexout.2			# No
		decl %esi			# Undo
hexout.2:	decl %esi			# Adjust for inc
		call putstr			# Display hex
		popl %edi			# Restore
		popl %esi			#  caller's
		ret				# To caller
#
# Output zero-terminated string [ESI] to the console.
#
dputstr:
.ifndef BTXLDR_VERBOSE
		ret
.else
		jmp putstr
.endif
putstr.0:	call putchr			# Output char
putstr: 	lodsb				# Load char
		testb %al,%al			# End of string?
		jne putstr.0			# No
		ret				# To caller
#
# Output character AL to the console.
#
dputchr:
.ifndef BTXLDR_VERBOSE
		ret
.endif
putchr: 	pusha				# Save
		xorl %ecx,%ecx			# Zero for loops
		movb $SCR_MAT,%ah		# Mode/attribute
		movl $BDA_POS,%ebx		# BDA pointer
		movw (%ebx),%dx 		# Cursor position
.ifdef PC98
		movl $0xa0000,%edi		# Regen buffer (color)
.else
		movl $0xb8000,%edi		# Regen buffer (color)
		cmpb %ah,BDA_SCR-BDA_POS(%ebx)	# Mono mode?
		jne putchr.1			# No
		xorw %di,%di			# Regen buffer (mono)
.endif
putchr.1:	cmpb $0xa,%al			# New line?
		je putchr.2			# Yes
.ifdef PC98
		movw %dx,%cx
		movb %al,(%edi,%ecx,1)		# Write char
		addl $0x2000,%ecx
		movb %ah,(%edi,%ecx,1)		# Write attr
		addw $0x2,%dx
		jmp putchr.3
putchr.2:	movw %dx,%ax
		movb $SCR_COL*2,%dl
		div %dl
		incb %al
		mul %dl
		movw %ax,%dx
putchr.3:	cmpw $SCR_COL*SCR_ROW*2,%dx
.else
		xchgl %eax,%ecx 		# Save char
		movb $SCR_COL,%al		# Columns per row
		mulb %dh			#  * row position
		addb %dl,%al			#  + column
		adcb $0x0,%ah			#  position
		shll %eax			#  * 2
		xchgl %eax,%ecx 		# Swap char, offset
		movw %ax,(%edi,%ecx,1)		# Write attr:char
		incl %edx			# Bump cursor
		cmpb $SCR_COL,%dl		# Beyond row?
		jb putchr.3			# No
putchr.2:	xorb %dl,%dl			# Zero column
		incb %dh			# Bump row
putchr.3:	cmpb $SCR_ROW,%dh		# Beyond screen?
.endif
		jb putchr.4			# No
		leal 2*SCR_COL(%edi),%esi	# New top line
		movw $(SCR_ROW-1)*SCR_COL/2,%cx # Words to move
		rep				# Scroll
		movsl				#  screen
		movb $' ',%al			# Space
.ifdef PC98
		xorb %ah,%ah
.endif
		movb $SCR_COL,%cl		# Columns to clear
		rep				# Clear
		stosw				#  line
.ifdef PC98
		movw $(SCR_ROW-1)*SCR_COL*2,%dx
putchr.4:	movw %dx,(%ebx) 		# Update position
		shrw $1,%dx
gdcwait.3:	inb $0x60,%al
		testb $0x04,%al
		jz gdcwait.3
		movb $0x49,%al
		outb %al,$0x62
		movb %dl,%al
		outb %al,$0x60
		movb %dh,%al
		outb %al,$0x60
.else
		movb $SCR_ROW-1,%dh		# Bottom line
putchr.4:	movw %dx,(%ebx) 		# Update position
.endif
		popa				# Restore
		ret				# To caller
#
# Convert EAX, AX, or AL to hex, saving the result to [EDI].
#
hex32:		pushl %eax			# Save
		shrl $0x10,%eax 		# Do upper
		call hex16			#  16
		popl %eax			# Restore
hex16:		call hex16.1			# Do upper 8
hex16.1:	xchgb %ah,%al			# Save/restore
hex8:		pushl %eax			# Save
		shrb $0x4,%al			# Do upper
		call hex8.1			#  4
		popl %eax			# Restore
hex8.1: 	andb $0xf,%al			# Get lower 4
		cmpb $0xa,%al			# Convert
		sbbb $0x69,%al			#  to hex
		das				#  digit
		orb $0x20,%al			# To lower case
		stosb				# Save char
		ret				# (Recursive)

		.data
		.p2align 4
#
# Global descriptor table.
#
gdt:		.word 0x0,0x0,0x0,0x0		# Null entry
		.word 0xffff,0x0,0x9a00,0xcf	# SEL_SCODE
		.word 0xffff,0x0,0x9200,0xcf	# SEL_SDATA
		.word 0xffff,0x0,0x9a00,0x0	# SEL_RCODE
		.word 0xffff,0x0,0x9200,0x0	# SEL_RDATA
gdt.1:
gdtdesc:	.word gdt.1-gdt-1		# Limit
		.long gdt			# Base
#
# Messages.
#
m_logo: 	.asciz "\nBTX loader 1.00  "
m_vers: 	.asciz "BTX version is \0\n"
e_fmt:		.asciz "Error: Client format not supported\n"
#.ifdef BTXLDR_VERBOSE
m_mem:		.asciz "Starting in protected mode (base mem=\0)\n"
m_esp:		.asciz "Arguments passed (esp=\0):\n"
m_args: 	.asciz"<howto="
		.asciz" bootdev="
		.asciz" junk="
		.asciz" "
		.asciz" "
		.asciz" bootinfo=\0>\n"
m_rel_bi:	.asciz "Relocated bootinfo (size=48) to \0\n"
m_rel_args:	.asciz "Relocated arguments (size=18) to \0\n"
m_rel_btx:	.asciz "Relocated kernel (size=\0) to \0\n"
m_base: 	.asciz "Client base address is \0\n"
m_elf:		.asciz "Client format is ELF\n"
m_segs: 	.asciz "text segment: offset="
		.asciz " vaddr="
		.asciz " filesz="
		.asciz " memsz=\0\n"
		.asciz "data segment: offset="
		.asciz " vaddr="
		.asciz " filesz="
		.asciz " memsz=\0\n"
m_done: 	.asciz "Loading complete\n"
#.endif
#
# Uninitialized data area.
#
buf:						# Scratch buffer
