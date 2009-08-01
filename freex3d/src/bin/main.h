/*
=INSERT_TEMPLATE_HERE=

$Id$

FreeWRL/X3D main program.
Internal header: helper macros.

*/

#ifndef __FREEWRL_MAIN_H__
#define __FREEWRL_MAIN_H__

/* LOG, WARNING, ERROR macros */

#if defined(FW_DEBUG)
# define DEBUG_(_expr) _expr
#else
# define DEBUG_(...)
#endif

#define TRACE_MSG(_formargs...) DEBUG_(fprintf(stdout, ##_formargs))
#define WARN_MSG(_formargs...)  DEBUG_(fprintf(stdout, ##_formargs))
#define ERROR_MSG(_formargs...) DEBUG_(fprintf(stderr, ##_formargs))


#endif /* __FREEWRL_MAIN_H__ */
