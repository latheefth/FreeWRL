#!/bin/sh

# Make i386 library first
make distclean
cp -f JS/js1.5/src/config/Darwin.mk.i386 JS/js1.5/src/config/Darwin.mk
cp -f JS/js1.5/src/editline/Makefile.ref.i386 JS/js1.5/src/editline/Makefile.ref
cp -f JS/js1.5/src/Makefile.ref.uni JS/js1.5/src/Makefile.ref
cp -f JS/js1.5/src/Makefile.in.uni JS/js1.5/src/Makefile.in
cp -f JS/js1.5/src/urles.mk.uni JS/js1.5/src/rules.mk
cp -f vrml.conf.i386 vrml.conf.aqua
cp JS/Makefile.i386 JS/Makefile
perl Makefile.PL
make install
cp -f blib/arch/auto/VRML/VRMLFunc/VRMLFunc.bundle VRMLFunc_i386.dylib
cp -f blib/arch/auto/VRML/JS/JS.bundle JS_i386.bundle

# Clean and make ppc library
make distclean
cp -f vrml.conf.ppc vrml.conf.aqua
cp -f JS/js1.5/src/config/Darwin.mk.ppc JS/js1.5/src/config/Darwin.mk
cp -f JS/js1.5/src/editline/Makefile.ref.ppc JS/js1.5/src/editline/Makefile.ref
perl Makefile.PL

# Have to change gcc version here, as MakeMaker doesn't let you pass it as an option?
sed -e 's/CC = cc/CC = \/usr\/bin\/gcc-3.3/' Makefile > Makefile.t
rm -f Makefile
sed -e 's/env MACOSX_DEPLOYMENT_TARGET=10.3 cc/\/usr\/bin\/gcc-3.3/' Makefile.t > Makefile
rm -f Makefile.t
sed -e 's/CC = cc/CC = \/usr\/bin\/gcc-3.3/' CFuncs/Makefile > CFuncs/Makefile.t
rm -f CFuncs/Makefile
sed -e 's/env MACOSX_DEPLOYMENT_TARGET=10.3 cc/\/usr\/bin\/gcc-3.3/' CFuncs/Makefile.t > CFuncs/Makefile
rm -f CFuncs/Makefile.t
cp JS/Makefile.ppc JS/Makefile
make install
./t2script
cp -f blib/arch/auto/VRML/VRMLFunc/VRMLFunc.bundle VRMLFunc_ppc.dylib
cp -f blib/arch/auto/VRML/JS/JS.bundle JS_ppc.bundle

# Glue the two libraries together and make sure the library has a good name
lipo -create VRMLFunc_i386.dylib VRMLFunc_ppc.dylib -output libFreeWRLFunc.dylib
install_name_tool -id libFreeWRLFunc.dylib libFreeWRLFunc.dylib
cp libFreeWRLFunc.dylib /usr/local/lib/libFreeWRLFunc.dylib
cp libFreeWRLFunc.dylib /Library/Perl/5.8.6/darwin-thread-multi-2level/auto/VRML/VRMLFunc/libFreeWRLFunc.dylib
cp libFreeWRLFunc.dylib /Library/Perl/5.8.6/darwin-thread-multi-2level/auto/VRML/VRMLFunc/VRMLFunc.bundle

lipo -create JS_i386.bundle JS_ppc.bundle -output JS.bundle
cp JS.bundle /Library/Perl/5.8.6/darwin-thread-multi-2level/auto/VRML/JS/JS.bundle

# Clean up
rm -f VRMLFunc_i386.dylib
rm -f VRMLFunc_ppc.dylib
rm -f libFreeWRLFunc.dylib
