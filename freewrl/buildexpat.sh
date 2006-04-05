#!/bin/sh

./configure
make clean
rm -f libexpat_ppc.dylib
rm -f libexpat_i386.dylib

export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-3.3;export CXX=/usr/bin/g++-3.3

export LDFLAGS="-isysroot /Developer/SDKs/MacOSX10.3.9.sdk -Wl"
export LDFLAGS2="-arch ppc"

export CPPFLAGS="-I/Developer/SDKs/MacOSX10.3.9.sdk/Developer/Headers/FlatCarbon -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3 -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++ -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++/ppc-darwin -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include"
export CFLAGS="-arch ppc -I/Developer/SDKs/MacOSX10.3.9.sdk/Developer/Headers/FlatCarbon -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3 -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++ -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++/ppc-darwin -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include"


make
mv -f libexpat.la libexpat_ppc.dylib

make clean

export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export LDFLAGS=""
export LDFLAGS2=""
export CPPFLAGS=""
export CFLAGS=""

export LDFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386"
export CPPFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386"

make
mv -f libexpat.la libexpat_i386.dylib
lipo -create libexpat_ppc.dylib libexpat_i386.dylib -output libexpat.dylib
install_name_tool -id libexpat.dylib libexpat.dylib
cp -f libexpat.dylib /usr/local/lib
cp -f lib/expat.h /usr/local/include
cp -f lib/expat_external.h /usr/local/include


export CC="";export CXX=""
export LDFLAGS=""
export CPPFLAGS=""
