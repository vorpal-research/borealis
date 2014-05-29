#
# Copyright (C) 2006  Seznam.cz, a.s.
#
# $Id: Makefile.am 5069 2008-04-30 10:09:38Z vladimir.kobzev $
#
# DESCRIPTION
# A makefile template for cfgparser library.
#
# AUTHORS
# Vaclav Blazek <blazek@firma.seznam.cz>
#
# HISTORY
# 2007-02-22  (vasek)
#             Created.
#


SUBDIRS = src include

# debian packaging scripts
EXTRA_DIST = debian/make.sh debian/libcfgparser.control \
			 debian/libcfgparser.postinst debian/libcfgparser.shlibs

test:
	(cd src/test && make clean && make all)
