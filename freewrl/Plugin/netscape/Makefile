#!make
#
# Netscape 4.x plugin makefile, based on:
#
################################################################################
# Copyright (c) 1996 Netscape Communications. All rights reserved.
################################################################################
#
# Simple Sample plugin makefile
#
# Platform: Linux 1.2 (ELF)
#
# The output of the make process will be npfreewrl.so
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
#
# 

## Change as required for your installation or override either on the
## command line or through the Perl module Makefile.PL scripts.
ifndef ${NETSCAPE_INST}

NETSCAPE_INST:=/usr/lib/netscape

endif

PLUGIN_DIR:= ${NETSCAPE_INST}/plugins
LIB_DIR:=./_lib
OBJ_DIR:=./_obj
LIBS:=

CC:=gcc
OPTIMIZER:=-O2
PLUGIN_DEFINES:=-DXP_UNIX
PLUGIN_GLUE:=..
## linux, irix, ...
OSNAME:=linux

## Override on the command line:
##
## e.g. $ make DEBUG=1
##
## Can also override through the Perl module Makefile.PL scripts.
DEBUG:=0
ifeq (${DEBUG},1)
    PLUGIN_DEFINES+=-D_DEBUG
##PLUGIN_DEFINES+=-DPLUGIN_TRACE 
endif

INC:=\
    -I/usr/include\
    -I.\
    -Iinclude\
    -I${PLUGIN_GLUE}/CFuncs

CFLAGS=${OPTIMIZER} ${PLUGIN_DEFINES} ${INC}

ifeq ($(strip ${OSNAME}),linux)

CFLAGS+=-fPIC

endif

SRC:=\
    npfreewrl.c\
    npunix.c

OBJ:=${SRC:%c=${OBJ_DIR}/%o}

vpath %.c ./source ./source/common

SHAREDTARGET=npfreewrl.so

all :: ${LIB_DIR}/${SHAREDTARGET}

.PHONY: all clean realclean

lib_dir ::
	mkdir -p ${LIB_DIR}

obj_dir ::
	mkdir -p ${OBJ_DIR}

install :: ${LIB_DIR}/${SHAREDTARGET}
	cp -f ${LIB_DIR}/${SHAREDTARGET} ${PLUGIN_DIR}

${LIB_DIR}/${SHAREDTARGET}: lib_dir obj_dir ${OBJ}
	${CC} -shared ${LIBS} ${OBJ} ${LDFLAGS} -o ${LIB_DIR}/${SHAREDTARGET}

${OBJ_DIR}/%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

clean ::
	${RM} ${OBJ} ${LIB_DIR}/${SHAREDTARGET}

realclean :: clean
	${RM} -Rf ${OBJ_DIR} ${LIB_DIR} ${PLUGIN_DIR}/${SHAREDTARGET}
