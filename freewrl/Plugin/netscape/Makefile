#!make
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

## Change as required for your installation:
PLUGIN_DIR=/usr/lib/netscape/plugins
JAVA_DIR=/usr/java/jdk1.3.1

CC=gcc
OPTIMIZER:=-O2
PLUGIN_DEFINES:=-DXP_UNIX

LIB_JAVA:=-L/usr/java/jdk1.3.1/jre/lib/i386 -ljava

DEBUG:=0
ifeq (${DEBUG},1)
    PLUGIN_DEFINES+=-D_DEBUG -ggdb
##PLUGIN_DEFINES+=-DPLUGIN_TRACE 
endif

INC:=\
    -I/usr/include\
    -I${JAVA_DIR}/include\
    -I${JAVA_DIR}/include/linux\
    -I.\
    -I./include

##CFLAGS=${OPTIMIZER} -fPIC -Wall ${PLUGIN_DEFINES} ${INC}
CFLAGS=${OPTIMIZER} -Wall ${PLUGIN_DEFINES} ${INC}
LIBS=${LIB_JAVA}

SRC:=\
    npfreewrl.c\
    npunix.c

OBJ:=\
    npfreewrl.o\
    npunix.o

vpath %.c ./source ./source/common

SHAREDTARGET=npfreewrl.so

.PHONY: all clean realclean

default all: ${SHAREDTARGET}

install: ${SHAREDTARGET}
	cp -f ${SHAREDTARGET} ${PLUGIN_DIR}

${SHAREDTARGET}: ${OBJ}
	${CC} -shared ${LIBS} ${OBJ} ${LDFLAGS} -o ${SHAREDTARGET}

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

clean:
	${RM} ${OBJ} ${SHAREDTARGET}

realclean: clean
	${RM} ${PLUGIN_DIR}/${SHAREDTARGET}
