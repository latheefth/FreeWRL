#INCLUDE = -I/usr/X11R6/include/
#LIBS  = -L. -L/usr/X11R6/lib -L/usr/local/lib

COMPILERFLAGS = -Wall
CC = gcc
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)
LIBRARIES = -lm
SVROBJ = main.o wavmaker.o mixer.o

all: main

main: $(SVROBJ)
	$(CC) $(SVROBJ) $(CFLAGS) -o FreeWRL_SoundServer $(LIBS) $(LIBRARIES)
	strip FreeWRL_SoundServer

mixer: mixer.o
	$(CC) $(CFLAGS) -o $mixer $(LIBS) $< $(LIBRARIES)

wavmaker: wavmaker.o
	$(CC) $(CFLAGS) -o $wavmaker $(LIBS) $< $(LIBRARIES)

clean:
	rm -f *.o FreeWRL_SoundServer

install:  main
	cp FreeWRL_SoundServer $(BINDIR)

