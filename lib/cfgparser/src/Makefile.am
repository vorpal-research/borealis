#
# Copyright (C) 2006  Seznam.cz, a.s.
#
# $Id: Makefile.am 4247 2008-01-03 13:38:05Z michal.bukovsky $
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

# warn on all
AM_CXXFLAGS = -Wall -g

# path to includes
AM_CPPFLAGS = -I${srcdir}/../include

# compile this library
lib_LTLIBRARIES = libcfgparser.la
libcfgparser_la_LDFLAGS = @VERSION_INFO@
libcfgparser_la_SOURCES = cfgparser.cc configwrapper.cc

# compile program
bin_PROGRAMS = cfggetvalue
cfggetvalue_SOURCES = cfggetvalue.cc
cfggetvalue_LDADD = -L${srcdir}/../src/.libs -lcfgparser
cfggetvalue_DEPENDENCIES = libcfgparser.la

# test noinst program
noinst_PROGRAMS = test-cfgparser
test_cfgparser_SOURCES = test-cfgparser.cc
test_cfgparser_LDADD = -L${srcdir}/../src/.libs -lcfgparser
test_cfgparser_DEPENDENCIES = libcfgparser.la
