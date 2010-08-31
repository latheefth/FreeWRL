# src dir Makefile.am
#
# $Id$
#
# Conditional build of sub-directories

DIST_SUBDIRS = lib bin java plugin sound message libeai
SUBDIRS = lib bin 

SUBDIRS+=$(MAYBE_PLUGIN)
SUBDIRS+=$(MAYBE_LIBEAI)
SUBDIRS+=$(MAYBE_JAVA)

if OS_LINUX
SUBDIRS+=$(MAYBE_SOUND)
SUBDIRS+=message
endif
