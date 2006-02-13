# make file for FreeWRL front end.
#

# for making the freewrl opts file
FWFLAGS = -c -D_REENTRANT -I../CFuncs

FRFLAGS = -c -I../CFuncs -D_REENTRANT `perl -MExtUtils::Embed -e ccopts`

all:	freewrl

install:  freewrl
	strip freewrl
	cp freewrl $(BINDIR)

fwopts:
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) fwopts.c

freewrl: fwopts
	$(CC) $(INC) $(FRFLAGS) $(DEFINE) freewrl.c

	$(CC) -L../blib/arch/auto/VRML/VRMLFunc -o freewrl \
	freewrl.o fwopts.o \
	`perl -MExtUtils::Embed -e ccopts -e ldopts` \
	$(LIBS) \
	-lpthread -lFreeWRLFunc $(BC)\

clean:
	rm -f *o; rm -f perlxsi*; rm -f freewrl;

distclean: clean
