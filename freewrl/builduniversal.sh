#!/bin/sh

# Make i386 library first
make distclean
cp -f vrml.conf.i386 vrml.conf.aqua
perl Makefile.PL
make install
cp -f blib/arch/auto/VRML/VRMLFunc/VRMLFunc.bundle VRMLFunc_i386.dylib

# Clean and make ppc library
make distclean
cp -f vrml.conf.ppc vrml.conf.aqua
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
make install
./t2script
cp -f blib/arch/auto/VRML/VRMLFunc/VRMLFunc.bundle VRMLFunc_ppc.dylib

# Glue the two libraries together and make sure the library has a good name
lipo -create VRMLFunc_i386.dylib VRMLFunc_ppc.dylib -output libFreeWRLFunc.dylib
install_name_tool -id libFreeWRLFunc.dylib libFreeWRLFunc.dylib
cp libFreeWRLFunc.dylib /usr/local/lib/libFreeWRLFunc.dylib

# Clean up
rm -f VRMLFunc_i386.dylib
rm -f VRMLFunc_ppc.dylib
rm -f libFreeWRLFunc.dylib
