#!/bin/tcsh

rm -f Darwin_DBG.OBJ/libjs.dylib
rm -f Darwin_DBG.OBJ/jscpucfg.o
rm -f Darwin_DBG.OBJ/jscpucfg.h
rm -f Darwin_DBG.OBJ/jscpucfg
rm -f editline/Darwin_DBG.OBJ/libedit.a
rm -f editline/Darwin_DBG.OBJ/*.o

cp Makefile.ppc.ref Makefile.ref
cp rules.ppc.mk rules.mk
cp config/Darwin.ppc.mk config/Darwin.mk
make -f Makefile.ref clean
make -f Makefile.ref

