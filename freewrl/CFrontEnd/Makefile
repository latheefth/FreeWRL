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

freewrl: fwopts fwWindow
	$(CC) $(INC) $(FWFLAGS) $(DEFINE) freewrl.c

	$(CC) -L../blib/arch/auto/VRML/VRMLFunc -o freewrl \
	freewrl.o fwopts.o fwWindow.o \
	`perl -MExtUtils::Embed -e ccopts -e ldopts` \
	$(LIBS) \
	-L/usr/X11R6/lib \
	-lXm -lGLw -lXt -lpthread -lFreeWRLFunc $(BC)\

clean:
	rm -f *o; rm -f perlxsi*; rm -f freewrl;

distclean: clean
