# Library: main FreeWRL library
# Will generates its own pkg-config definition file (soon).
#

# Get global variables for Makefile defined by toplevel Makefile.globals
include $(top_srcdir)/Makefile.globals

# Compiler options
# freewrl library code uses a lot of type-punned pointers; turning off strict-aliasing
# (this enable -O2 compile to work)
AM_CFLAGS += -fno-strict-aliasing

# Just add variables tested and checked by toplevel configure

# General libraries used with all platforms
AM_CPPFLAGS += \
				$(ZLIB_CFLAGS) \
				$(PTHREAD_CFLAGS) \
				$(FREETYPE_CFLAGS) \
				$(FONTCONFIG_CFLAGS) \
				$(EXPAT_CFLAGS) \
				$(XML_CFLAGS) \
				$(GL_CFLAGS) \
				$(JPEG_CFLAGS) \
				$(PNG_CFLAGS) \
				$(LIBCURL_CFLAGS) \
				$(IMLIB2_CFLAGS) \
				$(JAVASCRIPT_ENGINE_CFLAGS)

EXTERNAL_LIBS = \
				$(ZLIB_LIBS) \
				$(PTHREAD_LIBS) \
				$(FREETYPE_LIBS) \
				$(FONTCONFIG_LIBS) \
				$(JAVASCRIPT_ENGINE_LIBS) \
				$(EXPAT_LIBS) \
				$(XML_LIBS) \
				$(GL_LIBS) \
				$(JPEG_LIBS) \
				$(PNG_LIBS) \
				$(LIBCURL_LIBS) \
				$(IMLIB2_LIBS) \
				$(LIBLO_LIBS) \
				$(LIBM)


# X11 flags specific to x11 target (default on Linux -- optional on Mac)
if WINDOW_USE_X11

AM_CPPFLAGS += \
				$(X_CFLAGS) \
				$(XAW_CFLAGS)
EXTERNAL_LIBS += \
				$(X_PRE_LIBS) \
				$(X_LIBS) $(X_EXTRA_LIBS) \
				$(XAW_LIBS) \
				$(OPENCL_LIBS)
endif

# Motif flags specific to motif target (only on Linux at the moment)
if WINDOW_USE_MOTIF

AM_CPPFLAGS += \
				$(X_CFLAGS) \
				$(XAW_CFLAGS) \
				$(MOTIF_CFLAGS)
EXTERNAL_LIBS += \
				$(X_PRE_LIBS) \
				$(MOTIF_LIBS) \
				$(X_LIBS) $(X_EXTRA_LIBS) \
				$(XAW_LIBS) \
				$(OPENCL_LIBS)
endif

if OS_MAC

# I found this in spidermonkey build files: -Wall -Wno-format -no-cpp-precomp -fno-common -DJS_THREADSAFE -DXP_UNIX -DSVR4 -DSYSV -D_BSD_SOURCE -DPOSIX_SOURCE -DDARWIN  -UDEBUG -DNDEBUG -UDEBUG_root -DJS_THREADSAFE -DEDITLINE

AM_CPPFLAGS += -Wno-format -no-cpp-precomp -fno-common 

endif

_EV=$(top_srcdir)/versions/LIBFREEWRL
_EV2=$(top_srcdir)/versions/LIBFREEWRL_LTVERSION
export

templ=$(top_srcdir)/versions/template/version.c.in
component=libFreeWRL
LIBFREEWRL_VERSION=`cat $$_EV`
LIBFREEWRL_LTVERSION=`cat $$_EV2`

# Generate the version source file
internal_version.c: $(_EV) $(_EV2)
	$(top_builddir)/vtempl $(component) \
	$(LIBFREEWRL_VERSION) < $(templ) > $@
# And add it to the list of files to clean
CLEANFILES=internal_version.c

newversion:
	-rm -f internal_version.*
	$(MAKE)

# To be able to check for libFreeWRL with pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libFreeWRL.pc

lib_LTLIBRARIES = libFreeWRL.la
include_HEADERS = libFreeWRL.h
libFreeWRL_la_LDFLAGS = -version-info $(LIBFREEWRL_LTVERSION) $(AM_LDFLAGS)

# To enable libtool to build DLL on Windows
if OS_WIN32
libFreeWRL_la_LDFLAGS += -no-undefined
endif

libFreeWRL_la_LIBADD = $(EXTERNAL_LIBS)

include Makefile.sources

libFreeWRL_la_SOURCES += internal_version.c

#
# WINDOW_USE_* are exclusive
#
if WINDOW_USE_X11
libFreeWRL_la_SOURCES += ui/fwCommonX11.c ui/fwBareWindow.c ui/fwBareWindow.h
AM_CPPFLAGS+=-DTARGET_X11
endif

if WINDOW_USE_MOTIF
libFreeWRL_la_SOURCES += ui/fwCommonX11.c ui/fwMotifWindow.c
AM_CPPFLAGS+=-DTARGET_MOTIF
endif

if WINDOW_USE_AQUA
libFreeWRL_la_SOURCES += ui/fwWindowAqua.c
AM_CPPFLAGS+=-DTARGET_AQUA -DAQUA
endif

if WINDOW_USE_WIN32
libFreeWRL_la_SOURCES += ui/fwWindow32.c
AM_CPPFLAGS+=-DTARGET_WIN32
endif

if STATUSBAR_STD
libFreeWRL_la_SOURCES += opengl/RasterFont.c ui/statusbar.c
AM_CPPFLAGS+=-DSTATUSBAR_STD
endif

if STATUSBAR_HUD
libFreeWRL_la_SOURCES += ui/statusbarHud.c
AM_CPPFLAGS+=-DSTATUSBAR_HUD
endif


# We'll try to maintain a dependency for generated code (first try here)
scenegraph/GeneratedCode.c vrml_parser/NodeFields.h vrml_parser/Structs.h: $(top_srcdir)/codegen/VRMLC.pm
	cd $(top_srcdir)/codegen && perl VRMLC.pm
