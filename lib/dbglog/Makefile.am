#
# Copyright (C) 2006  Seznam.cz, a.s.
#
# $Id: Makefile.am 29 2011-01-20 13:10:22Z burlog $
#
# DESCRIPTION
# A makefile template for dbglog library.
#
# AUTHORS
# Vaclav Blazek <blazek@firma.seznam.cz>
#
# HISTORY
# 2007-02-22  (vasek)
#             Created.
#


SUBDIRS = src include

EXTRA_DIST = debian/libdbglog.control debian/libdbglog-dev.control \
             debian/libdbglog.postinst debian/libdbglog.shlibs \
             debian/make.sh solaris/libdbglog.pkginfo \
             solaris/libdbglog-dev.pkginfo solaris/make.sh \
             LICENSE

pkgconfigdir=@libdir@/pkgconfig
pkgconfig_DATA=libdbglog.pc
