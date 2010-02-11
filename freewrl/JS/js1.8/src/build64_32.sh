#!/bin/tcsh

rm -f Darwin_DBG.OBJ/libjs_i386.dylib
rm -f Darwin_DBG.OBJ/libjs_x86_64.dylib
rm -f Darwin_DBG.OBJ/libjs_ppc.dylib
rm -f Darwin_DBG.OBJ/libjs.dylib
rm -f Darwin_DBG.OBJ/jscpucfg.o
rm -f Darwin_DBG.OBJ/jscpucfg.h
rm -f Darwin_DBG.OBJ/jscpucfg
rm -f editline/Darwin_DBG.OBJ/libedit.a
rm -f editline/Darwin_DBG.OBJ/*.o

cp Makefile.i386.ref Makefile.ref
cp rules.i386.mk rules.mk
cp config/Darwin.i386.mk config/Darwin.mk
make -f Makefile.ref clean
make -f Makefile.ref
cp Darwin_DBG.OBJ/libjs.dylib Darwin_DBG.OBJ/libjs_i386.dylib

cp Makefile.x86_64.ref Makefile.ref
cp rules.x86_64.mk rules.mk
cp config/Darwin.x86_64.mk config/Darwin.mk
make -f Makefile.ref clean
rm -f Darwin_DBG.OBJ/jscpucfg.o
rm -f Darwin_DBG.OBJ/jscpucfg.h
rm -f Darwin_DBG.OBJ/jscpucfg
rm -f editline/Darwin_DBG.OBJ/libedit.a
rm -f editline/Darwin_DBG.OBJ/*.o
make -f Makefile.ref
cp Darwin_DBG.OBJ/libjs.dylib Darwin_DBG.OBJ/libjs_x86_64.dylib

cp Makefile.ppc.ref Makefile.ref
cp rules.ppc.mk rules.mk
cp config/Darwin.ppc.mk config/Darwin.mk
make -f Makefile.ref clean
rm -f Darwin_DBG.OBJ/jscpucfg.o
rm -f Darwin_DBG.OBJ/jscpucfg.h
rm -f Darwin_DBG.OBJ/jscpucfg
rm -f editline/Darwin_DBG.OBJ/libedit.a
rm -f editline/Darwin_DBG.OBJ/*.o
make -f Makefile.ref
cp Darwin_DBG.OBJ/libjs.dylib Darwin_DBG.OBJ/libjs_ppc.dylib

lipo -create Darwin_DBG.OBJ/libjs_i386.dylib Darwin_DBG.OBJ/libjs_x86_64.dylib Darwin_DBG.OBJ/libjs_ppc.dylib -output Darwin_DBG.OBJ/libjs.dylib
rm -f Darwin_DBG.OBJ/libjs_i386.dylib
rm -f Darwin_DBG.OBJ/libjs_x86_64.dylib
rm -f Darwin_DBG.OBJ/libjs_ppc.dylib
