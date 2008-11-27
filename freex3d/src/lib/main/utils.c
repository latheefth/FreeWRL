/*
=INSERT_TEMPLATE_HERE=

$Id$

General utility functions.

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeX3D.h>

#include "../vrml_parser/Structs.h"
#include "headers.h"


char *BrowserFullPath = NULL;
char *BrowserName = "FreeWRL VRML/X3D Browser";

const char* freex3d_get_browser_program()
{
    char *tmp;

    /*
      1. Check BROWSER environment variable
      2. Use configuration value BROWSER
    */

    tmp = getenv("BROWSER");
    if (!tmp) {
	tmp = BROWSER;
    }
    return tmp;
}
