#!/bin/sh

export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-3.3;export CXX=/usr/bin/g++-3.3
export LDFLAGS="-arch ppc"
export LDFLAGS2="-arch ppc"
export CPPFLAGS="-arch ppc"
export CFLAGS="-arch ppc"

export LDFLAGS="-isysroot /Developer/SDKs/MacOSX10.3.9.sdk -Wl -arch ppc"
export CPPFLAGS="-I/Developer/SDKs/MacOSX10.3.9.sdk/Developer/Headers/FlatCarbon -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3 -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++ -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++/ppc-darwin -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include"
export CFLAGS="-I/Developer/SDKs/MacOSX10.3.9.sdk/Developer/Headers/FlatCarbon -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3 -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++ -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++/ppc-darwin -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include"

rm -f libpng_ppc.a
rm -f libpng_i386.a
rm -f libpng_i386.3.
rm -f libpng_ppc.3.
rm -f libpng.a

make clean

make
mv -f libpng12.0.1.2.5.dylib libpng_ppc12.0.1.2.5.dylib
mv libpng.3 libpng_ppc.3.

make clean

export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export LDFLAGS=""
export LDFLAGS2=""
export CPPFLAGS=""
export CFLAGS=""

export LDFLAGS="-L. -L/usr/local/lib -lz -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386"
export CPPFLAGS="-I../zlib -Wall -03 -funroll-loops -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386"

make -e
mv libpng12.0.1.2.5.dylib libpng_i38612.0.1.2.5.dylib 
mv libpng.3 libpng_i386.3.
lipo -create libpng_i38612.0.1.2.5.dylib libpng_ppc12.0.1.2.5.dylib  -output libpng12.0.1.2.5.dylib
lipo -create libpng_i386.3. libpng_ppc.3. -output libpng.3


export CC="";export CXX=""
export LDFLAGS=""
export CPPFLAGS=""
cp -f png.h /usr/local/include/
cp -f pngconf.h /usr/local/include/
cp -f libpng*dylib /usr/local/lib/
