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
 * A substantial amount of code has been adapted from the embedding
 * tutorials from the SpiderMonkey web pages
 * (http://www.mozilla.org/js/spidermonkey/)
 * and from js/src/js.c, which is the sample application included with
 * the javascript engine.
 *
 */

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

MODULE = VRML::JS	PACKAGE = VRML::JS
PROTOTYPES: ENABLE

