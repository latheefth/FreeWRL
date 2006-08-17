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

extern jsval global_return_val;

static JSBool reportWarnings = JS_TRUE;

int jsrrunScript(JSContext *_context, JSObject *_globalObj, char *script, jsval *rval);
int
ActualrunScript(uintptr_t num,
				char *script,
				jsval *rval);

int
JSrunScript(uintptr_t num,
			char *script,
			SV *rstr,
			SV *rnum);

int
JSaddGlobalAssignProperty(uintptr_t num,
						  char *name,
						  char *str);

int
JSaddSFNodeProperty(uintptr_t num,
					char *nodeName,
					char *name,
					char *str);

int
JSaddGlobalECMANativeProperty(uintptr_t num,
							  char *name);

void
reportWarningsOn(void);

void
reportWarningsOff(void);

void
errorReporter(JSContext *cx,
			  const char *message,
			  JSErrorReport *report);

int JSGetProperty(uintptr_t num, char *script, SV *rstr);
void JSInit(uintptr_t num);

#endif /* __jsUtils_h__ */
