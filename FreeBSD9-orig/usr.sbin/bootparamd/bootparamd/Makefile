#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
# $FreeBSD: release/9.0.0/usr.sbin/bootparamd/bootparamd/Makefile 156813 2006-03-17 18:54:44Z ru $

.include <bsd.own.mk>

PROG=	bootparamd
MAN=	bootparams.5 bootparamd.8
SRCS=	bootparamd.c main.c ${GENSRCS}
GENSRCS=bootparam_prot.h bootparam_prot_svc.c bootparam_prot_xdr.c

CFLAGS+= -DTFTP_DIR=\"/tftpboot\" -I.
.if ${MK_NIS} != "no"
CFLAGS+= -DYP
.endif

CLEANFILES= ${GENSRCS}

RPCSRC=	${DESTDIR}/usr/include/rpcsvc/bootparam_prot.x

bootparam_prot_svc.c: ${RPCSRC}
	rpcgen -C -m -o ${.TARGET} ${RPCSRC}

bootparam_prot_xdr.c: ${RPCSRC}
	rpcgen -C -c -o ${.TARGET} ${RPCSRC}

bootparam_prot.h: ${RPCSRC}
	rpcgen -C -h -o ${.TARGET} ${RPCSRC}

.include <bsd.prog.mk>
