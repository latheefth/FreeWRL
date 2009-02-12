# src dir Makefile.am
#
# $Id$
#
# Conditional build of sub-directories

DIST_SUBDIRS = lib bin plugin sound message libeai
SUBDIRS = lib bin 

SUBDIRS+=$(MAYBE_PLUGIN)
SUBDIRS+=$(MAYBE_LIBEAI)

if OS_LINUX
SUBDIRS+=sound message
endif
