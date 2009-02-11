/*
=INSERT_TEMPLATE_HERE=

$Id$

FreeWRL support library.
Internal header: threading library, and processor control (sched).

*/

#ifndef __LIBFREEWRL_SYSTEM_THREADS_H__
#define __LIBFREEWRL_SYSTEM_THREADS_H__


#if HAVE_PTHREAD
# include <pthread.h>
#endif

#if HAVE_SCHED_H
#include <sched.h>
#endif


#endif /* __LIBFREEWRL_SYSTEM_THREADS_H__ */
