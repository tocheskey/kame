# New ports collection makefile for:	totd
# Version required:     1.1
# Date created:		6 Oct 1999
# Whom:			feico@pasta.cs.uit.no
#
# $Id: Makefile,v 1.4 2000/04/02 13:06:33 itojun Exp $

DISTNAME=	totd-1.1p4
CATEGORIES=	net
MASTER_SITES=	ftp://ftp.pasta.cs.uit.no/pub/Vermicelli/

MAINTAINER=	feico@pasta.cs.uit.no
HOMEPAGE=	http://www.vermicelli.cs.uit.no/ipv6/software.html

WRKSRC=         ${WRKDIR}/totd-1.1p4

GNU_CONFIGURE=	YES

post-install:
	${INSTALL_SCRIPT} ${FILESDIR}/totd.conf.sample ${PREFIX}/etc/totd.conf.sample

.include <bsd.port.mk>
