INCLUDE = -I/usr/X11R6/include/
LIBDIR  = -L. -L/usr/X11R6/lib -L/usr/local/lib

COMPILERFLAGS = -Wall
CC = gcc
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)
LIBRARIES = -lm
SVROBJ = main.o wavmaker.o mixer.o

All: main 

main: $(SVROBJ)
	$(CC) $(SVROBJ) $(CFLAGS) -o FreeWRL_SoundServer $(LIBDIR) $(LIBRARIES)
	@ls -al FreeWRL_SoundServer;strip FreeWRL_SoundServer;ls -al FreeWRL_SoundServer

mixer: mixer.o
	$(CC) $(CFLAGS) -o $mixer $(LIBDIR) $< $(LIBRARIES)

wavmaker: wavmaker.o
	$(CC) $(CFLAGS) -o $wavmaker $(LIBDIR) $< $(LIBRARIES)

clean:
	rm -f *.o FreeWRL_SoundServer

install:  main
	cp FreeWRL_SoundServer /usr/local/bin/.

