INCLUDE = -I/usr/X11R6/include/
LIBDIR  = -L. -L/usr/X11R6/lib -L/usr/local/lib

COMPILERFLAGS = -Wall
CC = gcc
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)
LIBRARIES = -lX11 -lXi -lXmu -lglut -lGL -lGLU -lm
SVROBJ = main.o wavmaker.o mixer.o

install:  main
	cp FreeWRL_SoundServer /usr/local/bin

All: main 

main: $(SVROBJ)
	$(CC) $(SVROBJ) $(CFLAGS) -o FreeWRL_SoundServer $(LIBDIR) $(LIBRARIES)

mixer: mixer.o
	$(CC) $(CFLAGS) -o $mixer $(LIBDIR) $< $(LIBRARIES)

wavmaker: wavmaker.o
	$(CC) $(CFLAGS) -o $wavmaker $(LIBDIR) $< $(LIBRARIES)

