/*

  FreeWRL support library.
  Internal functions: some very usefull functions are not always
  present (example: strndup, ...).

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#include <config.h>
#include <system.h>
#include <internal.h>
#include <libFreeWRL.h>
#include <threads.h>

#if defined(HAVE_STDARG_H)
# include <stdarg.h>
#endif

#if defined(HAVE_ERRNO_H)
# include <errno.h>
#endif

#if !defined(HAVE_STRNLEN)

/* Find the length of STRING, but scan at most MAXLEN characters.
   If no '\0' terminator is found in that many characters, return MAXLEN.  */

size_t __fw_strnlen(const char *s, size_t maxlen)
{
  const char *end = memchr(s, '\0', maxlen);
  return end ? (size_t) (end - s) : maxlen;
}

#endif

#if !defined(HAVE_STRNDUP)

char *__fw_strndup(const char *s, size_t n)
{
    size_t len = strnlen(s, n);
    char *new = MALLOC(char *, len + 1);
    
    if (!new)
	return NULL;
    
    new[len] = '\0';
    memcpy(new, s, len);
    /* although we could return the output of memcpy, OSX cacks on it, so return mallocd area */
    return new;
}

#endif


void fw_perror(FILE *f, const char *format, ...)
{
    int e;
    va_list ap;

    va_start(ap, format);	
    e = errno;
    vfprintf(f, format, ap);
    va_end(ap);

#ifdef HAVE_STRERROR
    FPRINTF(f, "[System error: %s]\n", strerror(e));
#else
    FPRINTF(f, "[System error: %d]\n", e);
#endif
    fflush(f);
}

/* Global FreeWRL options (will become profiles ?) */

//bool global_strictParsing = FALSE;
//bool global_plugin_print = FALSE;
//bool global_occlusion_disable = FALSE;
//bool global_print_opengl_errors = FALSE;
//bool global_trace_threads = FALSE;
//


void internalc_init(struct tinternalc* ic)
{
	//public
ic->global_strictParsing = FALSE;
ic->global_plugin_print = FALSE;
ic->global_occlusion_disable = FALSE;
ic->user_request_texture_size = 0;
ic->global_print_opengl_errors = FALSE;
ic->global_trace_threads = FALSE;

	//private
}


/* Set up global environment, usually from environment variables */
void fwl_set_strictParsing	(bool flag) { 
	gglobal()->internalc.global_strictParsing = flag ; 

	//struct tinternalc *ic = &gglobal()->internalc;
	//ic->global_strictParsing = flag ; 

	//getchar();
}
void fwl_set_plugin_print	(bool flag) { gglobal()->internalc.global_plugin_print = flag ; }
void fwl_set_occlusion_disable	(bool flag) { gglobal()->internalc.global_occlusion_disable = flag; }
void fwl_set_print_opengl_errors(bool flag) { gglobal()->internalc.global_print_opengl_errors = flag;}
void fwl_set_trace_threads	(bool flag) { gglobal()->internalc.global_trace_threads = flag;}

void fwl_set_texture_size	(unsigned int texture_size) { 

		// save this one.

		gglobal()->internalc.user_request_texture_size = texture_size;

		// how does this fit with our current system?
		// is it BIGGER than we can support in hardware?
		// eg, are we asking for 2048, but system supports 1024 max?
		if (texture_size > gglobal()->display.rdr_caps.system_max_texture_size) 
			gglobal()->display.rdr_caps.runtime_max_texture_size = gglobal()->display.rdr_caps.system_max_texture_size;
		else 
			// it is ok, smaller than the system hardware support.
			gglobal()->display.rdr_caps.runtime_max_texture_size = texture_size;

		//ConsoleMessage ("user request texture size %d,  system %d, runtime %d",texture_size,
				//gglobal()->display.rdr_caps.system_max_texture_size,
				//gglobal()->display.rdr_caps.runtime_max_texture_size);
}

#ifdef FREEWRL_THREAD_COLORIZED

/* == Interal printf and fprintf function to output colors ==
   See threads.c for details.
*/

int printf_with_colored_threads(const char *format, ...)
{
	int ret;
	va_list args;
	va_start( args, format );

	printf("\033[22;%im", fw_thread_color(fw_thread_id()));
	
	ret = vprintf( format, args );

	printf("\033[22;%im", 39 /* Default color */);

	va_end( args );

	return ret;
}

int fprintf_with_colored_threads(FILE *stream, const char *format, ...)
{
	int ret;
	va_list args;
	va_start( args, format );

	fprintf(stream, "\033[22;%im", fw_thread_color(fw_thread_id()));
	
	ret = vfprintf( stream, format, args );

	fprintf(stream, "\033[22;%im", 39 /* Default color */);

	va_end( args );

	return ret;
}

#endif
