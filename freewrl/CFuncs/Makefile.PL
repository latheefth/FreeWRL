
# Simple Makefile.PL for making the CFuncs.

# All we really need to do is to get the Perl include directory and compile...

	open MAKEFILE, ">Makefile";
	
	print MAKEFILE "INCLUDE = ";
	foreach (@{INC}) {
		print MAKEFILE "-I$_ ";
	}
	print MAKEFILE "\n";

        print MAKEFILE '
LIBDIR  = 


COMPILERFLAGS = -Wall
CC = gcc
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)
LIBRARIES = 

all:  NormalCalcs.o Polyrep.o Tess.o Textures.o

clean:
	rm NormalCalcs.o Polyrep.o Tess.o Textures.o

NormalCalcs.o: NormalCalcs.c
	$(CC) $(CFLAGS) -c -o $@ $(LIBDIR) $< $(LIBRARIES)

Polyrep.o: Polyrep.c
	$(CC) $(CFLAGS) -c -o $@ $(LIBDIR) $< $(LIBRARIES)

';
