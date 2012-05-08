
/*
	Android compatible config.h
*/

#define MAX_MULTITEXTURE 4

#define GLES2 1



#define TARGET_ANDROID

/* Path to internet browser */
#define BROWSER "C:/Program Files/Mozilla Firefox/firefox.exe"

/* Directory for fonts. C:/Program Files/CRC/fonts */
#define FONTS_DIR "../fonts"

/* The FreeWRL message wrapper program name. */
#define FREEWRL_MESSAGE_WRAPPER "time"


/* The FreeWRL program name. */
#define FREEWRL_PROGRAM freeWrl.exe


/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

#define HAVE_LIBXML_PARSER_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1


/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPE_H 1


/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1


/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Define to 1 if you have the <sched.h> header file. */
#define HAVE_SCHED 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if stdbool.h conforms to C99. */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1
/* #define HAVE_STRINGS_H 1 WIN32 HAS string.h */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/ipc.h> header file. */
#define HAVE_SYS_IPC_H 1

/* Define to 1 if you have the <sys/msg.h> header file. */

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <time.h> header file. */
//#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Define to 1 if you have the <X11/extensions/xf86vmode.h> header file. */

/* Define if you have the Xxf86vm library. */

/* Define to 1 if the system has the type `_Bool'. */

/* Path to image converter. set to NULL if none */
#define IMAGECONVERT NULL

/* Name of package */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT " freewrl "

/* Define to the full name of this package. */

/* Define to the full name and version of this package. */

/* Define to the one symbol short name of this package. */

/* Define to the version of this package. */

/* Directory where to install browser plugin. */

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Path to sound converter */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Path to zip decompressor "C:/Program Files/GnuWin32/bin/unzip.exe"*/
#define UNZIP "unzip.exe"

/* Version number of package */

//#define WANT_WGET 1
/* Path to http fetcher "C:/Program Files/GnuWin32/bin/wget.exe"*/
//#define WGET "wget.exe"
#ifdef WANT_WGET
char *getWgetPath();
#define WGET getWgetPath() 
//#else WinInet by default
#endif


/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* enable debugging (default=off) */

/* Define to the type of a signed integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
//#define int32_t long

#define ushort unsigned short

/* Define to rpl_malloc if the replacement function should be used. */

/* Define to `int' if <sys/types.h> does not define. */

/* Define as `fork' if `vfork' does not work. */
#define vfork fork

#define HAVE_DIRECT_H 1
//#define HAVE_LIBGLEW 1
//#define FREEWRL_STEREO_RENDERING 1

#define HAVE_GETTIMEOFDAY 1

#define HAVE_STDARG_H 1

#define GL_ES_VERSION_2_0 1

#define FRONTEND_GETS_FILES 1
#define FRONTEND_HANDLES_DISPLAY_THREAD 1
#define SHADERS_2011 1

#ifdef _ANDROID
void DROIDDEBUG( const char*pFmtStr, ...);
#endif

