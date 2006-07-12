# make file for FreeWRL front end.
#

# for making the freewrl opts file
FWFLAGS = -c -I /usr/include/GL -I../CFuncs -D_REENTRANT `perl -MExtUtils::Embed -e ccopts`

all:	freewrl

install:  freewrl
	strip freewrl
	cp freewrl $(BINDIR)

fwopts:
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) fwopts.c

fwWindow:
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) fwWindow.c

fwMotifWindow:
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) fwMotifWindow.c

fwBareWindow:
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) fwBareWindow.c

GLwDrawA:
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) GLwDrawA.c

freewrl: fwopts fwWindow fwBareWindow fwMotifWindow GLwDrawA
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) freewrl.c

	$(CC) -L../blib/arch/auto/VRML/VRMLFunc -o freewrl \
	freewrl.o fwopts.o fwWindow.o fwBareWindow.o fwMotifWindow.o GLwDrawA.o \
	`perl -MExtUtils::Embed -e ccopts -e ldopts` \
	$(LIBS) \
	-L/usr/X11R6/lib \
	-lXt -lpthread -lFreeWRLFunc $(BC)\

clean:
	rm -f *o; rm -f perlxsi*; rm -f freewrl;

distclean: clean
