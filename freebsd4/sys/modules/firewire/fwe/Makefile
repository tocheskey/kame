# $FreeBSD: src/sys/modules/firewire/fwe/Makefile,v 1.2.2.1 2002/11/03 10:58:20 simokawa Exp $

# Makefile for the fwe(4) module (Ethernet over IEEE1394)

.PATH: ${.CURDIR}/../../../dev/firewire

KMOD	= if_fwe
SRCS	= bus_if.h device_if.h \
	  opt_inet.h \
	  if_fwe.c if_fwevar.h \
	  firewire.h firewirereg.h

.include <bsd.kmod.mk>

