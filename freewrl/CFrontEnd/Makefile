# make file for FreeWRL front end.
#

# for making the Perl interface
PFLAGS = -c -I../CFuncs -D_REENTRANT `perl -MExtUtils::Embed -e ccopts`

# for making the freewrl opts file
FWFLAGS = -c -D_REENTRANT

FRFLAGS = -c -DPERLPATH=\"\$(PPATH)\" \
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
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) fwopts.c

freewrl: perlxsi fwopts
	$(CC) $(INC) $(FRFLAGS) $(DEFINE) freewrl.c

	$(CC) -L../blib/arch/auto/VRML/VRMLFunc -o freewrl \
	perlxsi.o freewrl.o fwopts.o \
	`perl -MExtUtils::Embed -e ccopts -e ldopts` \
	$(LIBS) \
	-lpthread -lFreeWRLFunc $(BC)\

clean:
	rm -f *o; rm -f perlxsi*; rm -f freewrl;

distclean: clean
