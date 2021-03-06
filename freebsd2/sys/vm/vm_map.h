/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	from: @(#)vm_map.h	8.3 (Berkeley) 3/15/94
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 * $Id: vm_map.h,v 1.15.2.3 1998/09/22 01:30:55 jdp Exp $
 */

/*
 *	Virtual memory map module definitions.
 */

#ifndef	_VM_MAP_
#define	_VM_MAP_

/*
 *	Types defined:
 *
 *	vm_map_t		the high-level address map data structure.
 *	vm_map_entry_t		an entry in an address map.
 *	vm_map_version_t	a timestamp of a map, for use with vm_map_lookup
 */

/*
 *	Objects which live in maps may be either VM objects, or
 *	another map (called a "sharing map") which denotes read-write
 *	sharing with other maps.
 */

union vm_map_object {
	struct vm_object *vm_object;	/* object object */
	struct vm_map *share_map;	/* share map */
	struct vm_map *sub_map;		/* belongs to another map */
};

/*
 *	Address map entries consist of start and end addresses,
 *	a VM object (or sharing map) and offset into that object,
 *	and user-exported inheritance and protection information.
 *	Also included is control information for virtual copy operations.
 */
struct vm_map_entry {
	struct vm_map_entry *prev;	/* previous entry */
	struct vm_map_entry *next;	/* next entry */
	vm_offset_t start;		/* start address */
	vm_offset_t end;		/* end address */
	union vm_map_object object;	/* object I point to */
	vm_ooffset_t offset;		/* offset into object */
	u_char eflags;			/* map entry flags */
	/* Only in task maps: */
	vm_prot_t protection;		/* protection code */
	vm_prot_t max_protection;	/* maximum protection */
	vm_inherit_t inheritance;	/* inheritance */
	int wired_count;		/* can be paged if = 0 */
};

#define MAP_ENTRY_IS_A_MAP		0x1
#define MAP_ENTRY_IS_SUB_MAP		0x2
#define MAP_ENTRY_COW			0x4
#define MAP_ENTRY_NEEDS_COPY		0x8
#define MAP_ENTRY_NOFAULT		0x10
#define MAP_ENTRY_USER_WIRED		0x20

/*
 *	Maps are doubly-linked lists of map entries, kept sorted
 *	by address.  A single hint is provided to start
 *	searches again from the last successful search,
 *	insertion, or removal.
 */
struct vm_map {
	struct pmap *pmap;		/* Physical map */
	lock_data_t lock;		/* Lock for map data */
	struct vm_map_entry header;	/* List of entries */
	int nentries;			/* Number of entries */
	vm_size_t size;			/* virtual size */
	boolean_t is_main_map;		/* Am I a main map? */
	int ref_count;			/* Reference count */
	vm_map_entry_t hint;		/* hint for quick lookups */
	vm_map_entry_t first_free;	/* First free space hint */
	boolean_t entries_pageable;	/* map entries pageable?? */
	unsigned int timestamp;		/* Version number */
#define	min_offset		header.start
#define max_offset		header.end
};

/* 
 * Shareable process virtual address space.
 * May eventually be merged with vm_map.
 * Several fields are temporary (text, data stuff).
 */
struct vmspace {
	struct vm_map vm_map;	/* VM address map */
	struct pmap vm_pmap;	/* private physical map */
	int vm_refcnt;		/* number of references */
	caddr_t vm_shm;		/* SYS5 shared memory private data XXX */
	vm_object_t vm_upages_obj;	/* UPAGES object */
/* we copy from vm_startcopy to the end of the structure on fork */
#define vm_startcopy vm_rssize
	segsz_t vm_rssize;	/* current resident set size in pages */
	segsz_t vm_swrss;	/* resident set size before last swap */
	segsz_t vm_tsize;	/* text size (pages) XXX */
	segsz_t vm_dsize;	/* data size (pages) XXX */
	segsz_t vm_ssize;	/* stack size (pages) */
	caddr_t vm_taddr;	/* user virtual address of text XXX */
	caddr_t vm_daddr;	/* user virtual address of data XXX */
	caddr_t vm_maxsaddr;	/* user VA at max stack growth */
	caddr_t vm_minsaddr;	/* user VA at max stack growth */
};


/*
 *	Map versions are used to validate a previous lookup attempt.
 *
 *	Since lookup operations may involve both a main map and
 *	a sharing map, it is necessary to have a timestamp from each.
 *	[If the main map timestamp has changed, the share_map and
 *	associated timestamp are no longer valid; the map version
 *	does not include a reference for the embedded share_map.]
 */
typedef struct {
	int main_timestamp;
	vm_map_t share_map;
	int share_timestamp;
} vm_map_version_t;

/*
 *	Macros:		vm_map_lock, etc.
 *	Function:
 *		Perform locking on the data portion of a map.
 */

#define	vm_map_lock(map) { \
	lock_write(&(map)->lock); \
	(map)->timestamp++; \
}
#define	vm_map_unlock(map)	lock_write_done(&(map)->lock)
#define	vm_map_lock_read(map)	lock_read(&(map)->lock)
#define	vm_map_unlock_read(map)	lock_read_done(&(map)->lock)

/*
 *	Functions implemented as macros
 */
#define		vm_map_min(map)		((map)->min_offset)
#define		vm_map_max(map)		((map)->max_offset)
#define		vm_map_pmap(map)	((map)->pmap)

/* XXX: number of kernel maps and entries to statically allocate */
#define MAX_KMAP	10
#define	MAX_KMAPENT	128

/*
 * Copy-on-write flags for vm_map operations
 */
#define MAP_COPY_NEEDED 0x1
#define MAP_COPY_ON_WRITE 0x2
#define MAP_NOFAULT 0x4

/*
 * vm_fault option flags
 */
#define VM_FAULT_NORMAL 0		/* Nothing special */
#define VM_FAULT_CHANGE_WIRING 1	/* Change the wiring as appropriate */
#define VM_FAULT_USER_WIRE 2		/* Likewise, but for user purposes */
#define VM_FAULT_WIRE_MASK (VM_FAULT_CHANGE_WIRING|VM_FAULT_USER_WIRE)
#define	VM_FAULT_HOLD 4			/* Hold the page */
#define VM_FAULT_DIRTY 8		/* Dirty the page */

#ifdef KERNEL
extern vm_offset_t kentry_data;
extern vm_size_t kentry_data_size;

boolean_t vm_map_check_protection __P((vm_map_t, vm_offset_t, vm_offset_t, vm_prot_t));
int vm_map_copy __P((vm_map_t, vm_map_t, vm_offset_t, vm_size_t, vm_offset_t, boolean_t, boolean_t));
struct pmap;
vm_map_t vm_map_create __P((struct pmap *, vm_offset_t, vm_offset_t, boolean_t));
void vm_map_deallocate __P((vm_map_t));
int vm_map_delete __P((vm_map_t, vm_offset_t, vm_offset_t));
int vm_map_find __P((vm_map_t, vm_object_t, vm_ooffset_t, vm_offset_t *, vm_size_t, boolean_t, vm_prot_t, vm_prot_t, int));
int vm_map_findspace __P((vm_map_t, vm_offset_t, vm_size_t, vm_offset_t *));
int vm_map_inherit __P((vm_map_t, vm_offset_t, vm_offset_t, vm_inherit_t));
void vm_map_init __P((struct vm_map *, vm_offset_t, vm_offset_t, boolean_t));
int vm_map_insert __P((vm_map_t, vm_object_t, vm_ooffset_t, vm_offset_t, vm_offset_t, vm_prot_t, vm_prot_t, int));
int vm_map_lookup __P((vm_map_t *, vm_offset_t, vm_prot_t, vm_map_entry_t *, vm_object_t *,
    vm_pindex_t *, vm_prot_t *, boolean_t *, boolean_t *));
void vm_map_lookup_done __P((vm_map_t, vm_map_entry_t));
boolean_t vm_map_lookup_entry __P((vm_map_t, vm_offset_t, vm_map_entry_t *));
int vm_map_pageable __P((vm_map_t, vm_offset_t, vm_offset_t, boolean_t));
int vm_map_user_pageable __P((vm_map_t, vm_offset_t, vm_offset_t, boolean_t));
int vm_map_clean __P((vm_map_t, vm_offset_t, vm_offset_t, boolean_t, boolean_t));
int vm_map_protect __P((vm_map_t, vm_offset_t, vm_offset_t, vm_prot_t, boolean_t));
void vm_map_reference __P((vm_map_t));
int vm_map_remove __P((vm_map_t, vm_offset_t, vm_offset_t));
void vm_map_simplify __P((vm_map_t, vm_offset_t));
void vm_map_startup __P((void));
int vm_map_submap __P((vm_map_t, vm_offset_t, vm_offset_t, vm_map_t));
void vm_map_madvise __P((vm_map_t, pmap_t, vm_offset_t, vm_offset_t, int));
void vm_map_simplify_entry __P((vm_map_t, vm_map_entry_t));

#endif
#endif				/* _VM_MAP_ */
