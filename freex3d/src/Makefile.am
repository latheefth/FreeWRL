# src dir Makefile.am
#
# $Id$
#
# Conditional build of sub-directories

DIST_SUBDIRS = lib bin plugin sound message
SUBDIRS = lib bin 

SUBDIRS+=$(MAYBE_PLUGIN)

if OS_LINUX
SUBDIRS+=sound message
endif
