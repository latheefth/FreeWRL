/*******************************************************************
 *
 * FreeWRL support library
 *
 * main.c
 *
 * $Id$
 *
 *******************************************************************/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>


/**
 * General variables
 */

/**
 * library initialization
 */
void __attribute__ ((constructor)) libFreeWRL_init(void)
{
}

/**
 * library exit routine
 */
void __attribute__ ((destructor)) libFreeWRL_fini(void)
{
}

/**
 * Explicit initialization
 */
int initFreeWRL()
{
    if (!display_initialize()) {
	ERROR_MSG("error in initialization.\n");
	return FALSE;
    }
    return TRUE;
}

/**
 * Explicit exit routine
 */
void closeFreeWRL()
{
}
