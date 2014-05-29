#
# Copyright (C) 2006  Seznam.cz, a.s.
#
# $Id: Makefile.am 1 2008-03-27 10:43:04Z chlastakov $
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


# warn on all
AM_CXXFLAGS = -Wall -g -O2

# path to includes
AM_CPPFLAGS = -I../include

# compile this library
lib_LTLIBRARIES = libdbglog.la

# from the these sources
libdbglog_la_SOURCES = dbglog.cc

# with these flags (version info etc.)
libdbglog_la_LDFLAGS = @VERSION_INFO@

# test programs
noinst_PROGRAMS = test-dbglog

# test dbglog
test_dbglog_SOURCES = test-dbglog.c
test_dbglog_LDADD = -L. -ldbglog
test_dbglog_DEPENDENCIES = libdbglog.la
