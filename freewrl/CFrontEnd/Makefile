# make file for FreeWRL front end.
#

# for making the Perl interface
PFLAGS = -c -I../CFuncs -D_REENTRANT `perl -MExtUtils::Embed -e ccopts` 

# for making the freewrl opts file
FWFLAGS = -c -D_REENTRANT 

FRFLAGS = -c -DFWVER=\"\$(FWVER)\" -DPERLPATH=\"\$(PPATH)\" \
		 -I../CFuncs -D_REENTRANT `perl -MExtUtils::Embed -e ccopts`

all:	freewrl

install:  freewrl
	strip freewrl
	cp freewrl $(BINDIR)
	cp fw2init.pl $(BINDIR)

perlxsi: 
	perl -MExtUtils::Embed -e xsinit -- -o perlxsi.c;
	$(CC) $(INC) $(PFLAGS) perlxsi.c

fwopts: 
	$(CC) $(INC) $(FWFLAGS) fwopts.c
#
#cc -o freewrl \
#	-L../blib/arch/auto/VRML/VRMLFunc \
#	-L/usr/freeware/lib32 \
#	-L../JS/js/src/IRIX6.5_OPT.OBJ \
#	perlxsi.o fwopts.o freewrl.o `perl -MExtUtils::Embed -e ldopts`  \
#	-lFreeWRLFunc\
#	-lGL -lGLU -lXext -lX11 -lpng -lpthread -ljs\
#
#
#
#
freewrl: perlxsi fwopts
	$(CC) $(INC) $(FRFLAGS) freewrl.c \
	-L/usr/people/luigi/freewrl/FreeWRL-1.04.3/blib 

	$(CC) -L../blib/arch/auto/VRML/VRMLFunc -o freewrl \
	perlxsi.o freewrl.o fwopts.o \
	`perl -MExtUtils::Embed -e ccopts -e ldopts` \
	$(LIBS) \
	-lpthread -lFreeWRLFunc \

clean:
	rm *o; rm perlxsi*; rm freewrl;

distclean: clean
