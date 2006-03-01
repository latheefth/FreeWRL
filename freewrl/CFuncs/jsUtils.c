/*
 * Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License
 * (file COPYING in the distribution) for conditions of use and
 * redistribution, EXCEPT on the files which belong under the
 * Mozilla public license.
 *
 * $Id$
 *
 * A substantial amount of code has been adapted from js/src/js.c,
 * which is the sample application included with the javascript engine.
 *
 */

#include "headers.h"
#include "jsUtils.h"

void
reportWarningsOn() { reportWarnings = JS_TRUE; }


void
reportWarningsOff() { reportWarnings = JS_FALSE; }


void
errorReporter(JSContext *context, const char *message, JSErrorReport *report)
{
	char *errorReport = 0;
	size_t len = 0, charPtrSize = sizeof(char *);

    if (!report) {
        fprintf(stderr, "%s\n", message);
        return;
    }

    /* Conditionally ignore reported warnings. */
    if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings) {
		return;
	}

	len = (strlen(report->filename) + 1) + (strlen(message) + 1);

	errorReport = (char *) JS_malloc(context, (len + STRING) * charPtrSize);
	if (!errorReport) {
		return;
	}


    if (JSREPORT_IS_WARNING(report->flags)) {
		sprintf(errorReport,
				"%swarning in %s at line %u:\n\t%s\n",
				JSREPORT_IS_STRICT(report->flags) ? "strict " : "",
				report->filename ? report->filename : "",
				report->lineno ? report->lineno : 0,
				message ? message : "No message.");
	} else {
		sprintf(errorReport,
				"error in %s at line %u:\n\t%s\n",
				report->filename ? report->filename : "",
				report->lineno ? report->lineno : 0,
				message ? message : "No message.");
	}

	fprintf(stderr, "Javascript -- %s", errorReport);

	JS_free(context, errorReport);
}



