/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* YIPS @(#)$Id: cfparse.h,v 1.4 2000/01/09 01:31:21 itojun Exp $ */

#define DEFAULT_CF_FILE "/usr/local/etc/racoon.conf"

#define CF_PATHTYPE_INCLUDE	0
#define CF_PATHTYPE_PSK		1
#define CF_PATHTYPE_CERT	2
#define CF_PATHTYPE_MAX		3

#define CF_PLADDR_ASSOCIATION	0
#define CF_PLADDR_NEGOTIATION	1

#define CF_SIDE_LOCAL		0
#define CF_SIDE_REMOTE		1

#define CF_LIFETYPE_TIME	0
#define CF_LIFETYPE_BYTE	1

#define CF_UNITTYPE_B	1
#define CF_UNITTYPE_KB	1024
#define CF_UNITTYPE_MB	(1024*1024)
#define CF_UNITTYPE_TB	(1024*1024*1024)
#define CF_UNITTYPE_S	1
#define CF_UNITTYPE_M	60
#define CF_UNITTYPE_H	(60*60)

/* cfparse.y */
extern int yyparse __P((void));
extern int cfparse __P((void));
extern int cfreparse __P((void));
