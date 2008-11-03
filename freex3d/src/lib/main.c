/*******************************************************************
 *
 * FreeX3D support library
 *
 * main.c
 *
 * $Id$
 *
 *******************************************************************/

#include "config.h"
#include "system.h"
#include "display.h"
#include "internal.h"

#ifndef _init
/**
 * library initialization
 */
void __attribute__ ((constructor)) libFreeX3D_init(void)
{
}
#endif

#ifndef _fini
/**
 * library exit routine
 */
void __attribute__ ((destructor)) libFreeX3D_fini(void)
{
}
#endif

/**
 * Explicit initialization
 */
void initFreewrl()
{
}

/**
 * Explicit exit routine
 */
void closeFreewrl()
{
}
