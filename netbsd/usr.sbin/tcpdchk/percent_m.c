/*	$NetBSD: percent_m.c,v 1.3 1998/05/09 17:22:09 kleink Exp $	*/

 /*
  * Replace %m by system error message.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#) percent_m.c 1.1 94/12/28 17:42:37";
#else
__RCSID("$NetBSD: percent_m.c,v 1.3 1998/05/09 17:22:09 kleink Exp $");
#endif
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

extern int errno;
#ifndef SYS_ERRLIST_DEFINED
extern char *sys_errlist[];
extern int sys_nerr;
#endif

#include "mystdarg.h"
#include "percent_m.h"

char   *percent_m(obuf, ibuf)
char   *obuf;
const char   *ibuf;
{
    char   *bp = obuf;
    const char   *cp = ibuf;

    while ((*bp = *cp) != '\0')
	if (*cp == '%' && cp[1] == 'm') {
	    strcpy(bp, strerror(errno));
	    bp += strlen(bp);
	    cp += 2;
	} else {
	    bp++, cp++;
	}
    return (obuf);
}
