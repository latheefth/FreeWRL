#!make
#
# $Id$
#
# Mozilla plugin makefile, based on
#
################################################################################
# Copyright (c) 1996 Netscape Communications. All rights reserved.
################################################################################
#
# and
#
################################################################################
# Copyright (C) 1998 Netscape Communications Corporation. All rights reserved.
################################################################################
#
# Simple Sample plugin makefile

ifndef ${MOZILLA_INST}

MOZILLA_INST:=/usr/lib/mozilla-1.0.0

endif

ifndef ${MOZILLA_INC}

MOZILLA_INC:=/usr/include/mozilla-1.0.0

endif


## Mozilla 1.0 sources are needed to rebuild the idl generated files.
MOZILLA_SRC:=/mnt/drive2/devel/mozilla

## The idl compiler should only be used if changes are made to the Plugin's API.
## Otherwise, use the idl-generated header and typelib files provided
## (see ${IDLGEN_HEADER} and ${IDLGEN_TYPELIB})
##IDL=${MOZILLA_SRC}/dist/bin/xpidl
IDL=


## Change as required for your installation or override either on the
## command line or through the Perl module Makefile.PL scripts.
PLUGIN_DIR:=${MOZILLA_INST}/plugins
PLUGIN_GLUE_DIR:=..
CFUNCS_DIR:=${PLUGIN_GLUE_DIR}/CFuncs
LIB_DIR:=_lib
OBJ_DIR:=_obj
DEP_DIR:=_dep
IDLGEN_DIR:=_xpidlgen
## linux, irix, ...
OSNAME:=linux

CC=gcc
CXX=g++


PLUGIN_DEFINES:=\
                -D_IMPL_NS_PLUGIN\
                -DOJI\
                -DMOZILLA_CLIENT

## Override on the command line:
##
## e.g. $ make DEBUG=1
##
## Can also override through the Perl module Makefile.PL scripts.

PLUGIN_DEBUG:=\
              -DDEBUG\
              -D_DEBUG\
              -DTRACING


DEBUG:=0
ifeq (${DEBUG},1)

PLUGIN_DEFINES+=${PLUGIN_DEBUG}

endif

ifeq ($(strip ${OSNAME}),linux)

## in addition to config-rhlinux72-i686.h
PLATFORM_DEFINES:=\
                  -DXP_UNIX=1\
                  -DLINUX=1\
                  -DOSTYPE=\"Linux2.4\"\
                  -DOSARCH=\"Linux\"\
                  -DMDCPUCFG=\"linux.cfg\"\
                  -D_GNU_SOURCE=1

endif

## -D_XOPEN_SOURCE=600


PLUGIN_DEFINES+=${PLATFORM_DEFINES}

## Path depends on installation - only needed if files in ${IDLGEN_HEADER}
## and ${IDLGEN_TYPELIB} don't exist.
ifneq (${IDL},)

IDL_INC=\
         -I${MOZILLA_SRC}/xpcom/base

IDL_FLAGS=-w ${IDL_INC} -I${MOZILLA_SRC}/dist/idl -o ${basename $@}

endif

IDL_SRC=\
         nsISimplePluginInstance.idl\
         nsIScriptablePlugin.idl

IDLGEN_HEADER=${IDL_SRC:%.idl=${IDLGEN_DIR}/%.h}

IDLGEN_TYPELIB=${IDL_SRC:%.idl=${IDLGEN_DIR}/%.xpt}

INC:=\
     -Iinclude\
     -I${PLUGIN_GLUE_DIR}/CFuncs\
     -I${MOZILLA_INC}/plugin\
     -I${MOZILLA_INC}/xpcom\
     -I${MOZILLA_INC}/java\
     -I${MOZILLA_INC}/nspr\
     -I${MOZILLA_INC}/gtkxtbin\
     -I${MOZILLA_INC}/widget

ifeq ($(strip ${OSNAME}),linux)

INC+=\
     -I/usr/X11R6/include\
     -I/usr/include\
     -I/usr/include/gtk-1.2\
     -I/usr/include/glib-1.2\
     -I/usr/lib/glib/include\
     -include include/config-rhlinux72-i686.h ## for platform defines

endif

OPTIMIZER=-O2

ifeq (${DEBUG},1)

OPTIMIZER+=-ggdb

endif

CFLAGS:=\
        ${PLUGIN_DEFINES}\
        ${OPTIMIZER}\
        ${INC}\
        -pedantic\
        -fshort-wchar\
        -fno-exceptions\
        -pthread\
        -pipe\
        -Wall\
        -Wmissing-prototypes\
        -Wpointer-arith\
        -Wno-long-long\

ifneq ($(strip ${OSNAME}),irix)

CFLAGS+=\
        -fPIC

endif

ifeq (${DEBUG},1)

CFLAGS+=\
        -W\
        -Wshadow\
        -Wcast-align
endif

CXXFLAGS:=\
          -fno-rtti\
          -Woverloaded-virtual\
          -Wsynth\
          -Wctor-dtor-privacy\
          -Wnon-virtual-dtor\
          -Wreorder

ifeq (${DEBUG},0)

CXXFLAGS+=\
          -Wno-unused

endif

## unused
#-Werror\

LIBS:=\
      -L${MOZILLA_INST}\
      -liberty\
      -lxpcom\
      -lplds4\
      -lplc4\
      -lnspr4\
      -lgmodule\
      -lpthread\
      -ldl\
      -lm\
      -lc   

ifeq ($(strip ${OSNAME}),linux)

LIBS+=\
      -L/usr/lib\
      -L/usr/X11R6/lib\
      -lgtkxtbin\
      -lXt\
      -lgtk\
      -lgdk\
      -lXi\
      -lXext\
      -lX11\
      -lglib

endif

## -lgtkembedmoz\
## -lgtksuperwin\

LDFLAGS:=\
          ${LIBS}\
          -shared\
          -rdynamic
#-Wl,-h -Wl,libnullplugin.so

CXXSRC:=\
     npFreeWRLInstance.cpp\
     np_entry.cpp\
     npp_gate.cpp\
     npn_gate.cpp

OBJ=${CXXSRC:%cpp=${OBJ_DIR}/%o}

CFUNCS_SRC:=\
     pluginSocket.c\
     pluginUtils.c\

OBJ+=${CFUNCS_SRC:%c=${PLUGIN_GLUE_DIR}/%o}

DEP=${DEP_DIR}/${@F:.o=.pp}

##vpath %.h ./include
vpath %.c   ${PLUGIN_GLUE_DIR}/CFuncs
vpath %.cpp ./source

SHAREDTARGET=npFreeWRL.so

all :: ${LIB_DIR}/${SHAREDTARGET}

.PHONY: all clean realclean

${DEP_DIR} :
	mkdir -p ${DEP_DIR}

${OBJ_DIR} :
	mkdir -p ${OBJ_DIR}

${LIB_DIR} :
	mkdir -p ${LIB_DIR}

install :: ${LIB_DIR}/${SHAREDTARGET}
	cp ${LIB_DIR}/${SHAREDTARGET} ${PLUGIN_DIR}

## Developer's note: make sure that IDL gen. files are not re-made if they already exist!!!
##${LIB_DIR}/${SHAREDTARGET}: ${IDLGEN_DIR} ${IDLGEN_HEADER} ${IDLGEN_TYPELIB} ${LIB_DIR} ${OBJ}
${LIB_DIR}/${SHAREDTARGET} : ${DEP_DIR} ${OBJ_DIR} ${LIB_DIR} ${OBJ}
	${CXX} -Wl,--start-group ${OBJ} -Wl,--end-group ${LDFLAGS} -o ${LIB_DIR}/${SHAREDTARGET}
	chmod 755 ${LIB_DIR}/${SHAREDTARGET}


ifneq (${IDL},)

idlgen : ${IDLGEN_DIR} ${IDLGEN_HEADER} ${IDLGEN_TYPELIB}

${IDLGEN_DIR} :
	mkdir -p ${IDLGEN_DIR}

## generates C++ header and XPConnect typelib
${IDLGEN_HEADER} : ${IDL_SRC}
	${IDL} ${IDL_FLAGS} -m header $<

${IDLGEN_TYPELIB} : ${IDL_SRC} ${IDLGEN_DIR}
	${IDL} ${IDL_FLAGS} -m typelib $<

endif

${OBJ_DIR}/%.o : %.cpp
	${CXX} -c $< ${CFLAGS} -Wp,-MD,${DEP} -o $@

${PLUGIN_GLUE_DIR}/%.o : %.c
	${CC} -c $< ${CFLAGS} -Wp,-MD,${DEP} -o $@

clean ::
	${RM} ${OBJ_DIR}/* ${LIB_DIR}/*

realclean :: clean
	${RM} -Rf ${PLUGIN_DIR}/${SHAREDTARGET} ${DEP_DIR} ${OBJ_DIR} ${LIB_DIR}

print :
	@echo ${OBJ}
