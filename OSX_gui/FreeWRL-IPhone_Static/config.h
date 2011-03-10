/* specific for iPhone build */

#define AQUA 1
#define FRONTEND_HANDLES_DISPLAY_THREAD 1
#define FWVER "1.22.10"
#define HAVE_CONFIG_H 1
#define HAVE_CTYPE_H 1
#define HAVE_ERRNO_H 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_LIBXML_PARSER_H 1
#define HAVE_MATH_H 1
#define HAVE_MEMCPY 1
#define HAVE_PTHREAD 1
#define HAVE_STDARG_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_WAIT_H 1
#define IPHONE 1
#define SHADERS_2011 1
#define TARGET_AQUA 1
#define XP_UNIX 1
#define _REENTRANT 1

/* these really need looking at for the iPhone - will get to it... */
#define BROWSER "/usr/bin/open"
#define BUILDDIR "/FreeWRL/freewrl/freex3d/appleOSX"
#define FONTS_DIR "/usr/share/freewrl/fonts"
#define FREEWRL_MESSAGE_WRAPPER "/usr/bin/say"
#define FREEX3D_MESSAGE_WRAPPER "/usr/bin/say"
#define INSTALLDIR "/usr/share/freewrl"
#define REWIRE_SERVER "/usr/bin/freewrlReWireServer"
#define SOUNDSERVERBINARY "/usr/bin/FreeWRL_SoundServer"
#define WGET "/usr/bin/curl"

#include <dirent.h>
#include <fcntl.h>
#include <internal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
