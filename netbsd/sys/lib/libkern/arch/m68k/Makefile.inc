#	$NetBSD: Makefile.inc,v 1.19 1998/08/04 03:31:28 perry Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.S \
	bcmp.S bzero.S ffs.S \
	memchr.c memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strncmp.S strncasecmp.c strncpy.S strrchr.S \
	scanc.S skpc.S \
	htonl.S htons.S ntohl.S ntohs.S \
	random.S
