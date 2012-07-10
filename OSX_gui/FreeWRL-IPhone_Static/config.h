/* specific for iPhone build */

#define FRONTEND_HANDLES_DISPLAY_THREAD 1
#define FRONTEND_GETS_FILES 1

#define HAVE_CONFIG_H 1
#define HAVE_CTYPE_H 1
#define HAVE_ERRNO_H 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_MATH_H 1
#define HAVE_MEMCPY 1
#define HAVE_STDARG_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_PTHREAD 1
#define HAVE_UNISTD_H 1


#define XP_UNIX 1
#define X_DISPLAY_MISSING 1
#define TARGET_AQUA 1
#define _REENTRANT 1

// not yet #define DO_COLLISION_GPU 1
#define HAVE_LIBXML_PARSER_H 1
#define IPHONE
#define AQUA 1


#define FONTS_DIR "/usr/share/freewrl/fonts"

#define FWVER "1.22.12"
#define FREEWRL_MESSAGE_WRAPPER "/usr/bin/say"
#define BROWSER "/usr/bin/open"

#include <stdio.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>


