/*******************************************************************
 *
 * FreeX3D support library
 *
 * internal header - internal.h
 *
 * Library internal declarations.
 *
 * $Id$
 *
 *******************************************************************/

#ifndef __LIBFREEX3D_DECL_H__
#define __LIBFREEX3D_DECL_H__


/**
 * internal stuff needed by multiple C files in the library
 */
#ifdef _DEBUG
#  define DEBUG_(_expr) _expr
#else
#  define DEBUG_(_expr)
#endif

#define TRACE(_formargs...) DEBUG_(fprintf(stdout, ##_formargs))
#define ERROR(_formargs...) DEBUG_(fprintf(stderr, ##_formargs))


#endif /* __LIBFREEX3D_DECL_H__ */
