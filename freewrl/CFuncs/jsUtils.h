/*
 * Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License
 * (file COPYING in the distribution) for conditions of use and
 * redistribution, EXCEPT on the files which belong under the
 * Mozilla public license.
 *
 * $Id$
 */

#ifndef __jsUtils_h__
#define __jsUtils_h__

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "jsapi.h" /* JS compiler */
#include "jsdbgapi.h" /* JS debugger */

#include "headers.h"

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#define LARGESTRING 2048
#define STRING 512
#define SMALLSTRING 128

#define FNAME_STUB "file"
#define LINENO_STUB 0

#define BROWSER_SFNODE "__node"
#define BROWSER_RETVAL "__ret"


extern JSBool JSVerbose;
static JSBool reportWarnings = JS_TRUE;

int
ActualrunScript(int num,
				char *script,
				jsval *rval);

int
JSrunScript(int num,
			char *script,
			SV *rstr,
			SV *rnum);

int
JSaddGlobalAssignProperty(int num,
						  char *name,
						  char *str);

int
JSaddSFNodeProperty(int num,
					char *nodeName,
					char *name,
					char *str);

int
JSaddGlobalECMANativeProperty(int num,
							  char *name);

void
reportWarningsOn(void);

void
reportWarningsOff(void);

void
errorReporter(JSContext *cx,
			  const char *message,
			  JSErrorReport *report);

int JSGetProperty(int num, char *script, SV *rstr);
void JSInit(int num, SV *script);
//added Dec 13/04 M. Ward
void JScleanup(int num );

#endif /* __jsUtils_h__ */
