#INCLUDE = -I/usr/X11R6/include/
#LIBS  = -L. -L/usr/X11R6/lib -L/usr/local/lib

COMPILERFLAGS = -Wall
CC = gcc
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)
# use this for better borders on window LIBRARIES = -lXaw3d
LIBRARIES = -lXaw
SVROBJ = FreeWRL_Message.o

all: main 

main: $(SVROBJ)
	$(CC) $(SVROBJ) $(CFLAGS) -o FreeWRL_Message $(LIBS) $(LIBRARIES)
	strip FreeWRL_Message

FreeWRL_Message: FreeWRL_Message.o
	$(CC) $(CFLAGS) -o $FreeWRL_Message $(LIBS) $< $(LIBRARIES)

clean:
	rm -f *.o FreeWRL_Message

install:  main
	cp FreeWRL_Message $(BINDIR)

