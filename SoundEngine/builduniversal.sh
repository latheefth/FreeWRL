#!/bin/sh
cp -f Makefile.i386 Makefile
make clean
make
cp -f FreeWRL_SoundServer FreeWRL_SoundServer_i386
make clean
cp -f Makefile.ppc Makefile
make
cp -f FreeWRL_SoundServer FreeWRL_SoundServer_ppc
lipo -create FreeWRL_SoundServer_i386 FreeWRL_SoundServer_ppc -output FreeWRL_SoundServer
cp -r FreeWRL_SoundServer /usr/bin/FreeWRL_SoundServer
