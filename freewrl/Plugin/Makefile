#!make
################################################################################
# Copyright (c) 1996 Netscape Communications. All rights reserved.
################################################################################
#
# Simple Sample plugin makefile
#
# Platform: Linux 1.2 (ELF)
#
# The output of the make process will be npsimple.so
# Install this file either in
#	/usr/lib/netscape/plugins/
#	(or)
#	~/.netscape/plugins/
#	(or) in any convenient directory and point environment variable
#	     NPX_PLUGIN_PATH to point to the directory. It is advisable
#	     that the plugins (.so) are the only files in that directory.
#
# This makefile contains some of our defines for the compiler:
#
# XP_UNIX	This needs to get defined for npapi.h on unix platforms.
# PLUGIN_TRACE	Enable this define to get debug prints whenever the plugin
#		api gets control.
#
# WARNING: Motif libraries are built static into the navigator and cannot
#          be accessed from the plugin.
#

# PLUGIN_DEFINES= -DXP_UNIX -DPLUGIN_TRACE
PLUGIN_DEFINES= -DXP_UNIX -Inetscape/include

CC= gcc
OPTIMIZER= -g
CFLAGS=  $(OPTIMIZER) $(PLUGIN_DEFINES) -I../CFuncs -I/usr/include

SRC= netscape/source/npfreewrl.c netscape/source/common/npunix.c
OBJ= npfreewrl.o npunix.o

SHAREDTARGET=npfreewrl.so

install: npfreewrl
	cp npfreewrl.so $(PLUGDIR)

all:	npfreewrl

npfreewrl: $(OBJ)
	$(CC) -shared -o $(SHAREDTARGET) $(OBJ) $(LDFLAGS)

npfreewrl.o: netscape/source/npfreewrl.c
	$(CC) -c $(CFLAGS) netscape/source/npfreewrl.c

npunix.o: netscape/source/common/npunix.c
	$(CC) -c $(CFLAGS) netscape/source/common/npunix.c

clean:
	$(RM) $(OBJ) $(SHAREDTARGET)

###
#FWFLAGS = -c -D_REENTRANT
#
#FRFLAGS = -c -DFWVER=\"\$(FWVER)\" -DPERLPATH=\"\$(PPATH)\" \
#		 -I../CFuncs -D_REENTRANT `perl -MExtUtils::Embed -e ccopts`
#
#all:	freewrl
#
#install:  freewrl
#	strip freewrl
#	cp freewrl $(BINDIR)
#	cp fw2init.pl $(BINDIR)
#
#perlxsi:
#	perl -MExtUtils::Embed -e xsinit -- -o perlxsi.c;
#	$(CC) $(INC) $(PFLAGS) perlxsi.c
#
#fwopts:
#	$(CC) $(INC) $(FWFLAGS) $(DEFINE) fwopts.c
#
#freewrl: perlxsi fwopts
#	$(CC) $(INC) $(FRFLAGS) $(DEFINE) freewrl.c
#
#	$(CC) -L../blib/arch/auto/VRML/VRMLFunc -o freewrl \
#	perlxsi.o freewrl.o fwopts.o \
#	`perl -MExtUtils::Embed -e ccopts -e ldopts` \
#	$(LIBS) \
#	-lpthread -lFreeWRLFunc \
#
#clean:
#	rm *o; rm perlxsi*; rm freewrl;
#
#distclean: clean
