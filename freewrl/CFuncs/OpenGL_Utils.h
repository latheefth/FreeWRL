/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#ifndef __OPENGL_UTILS_H_
#define __OPENGL_UTILS_H_

/*
 * $Id$
 *
 */

#ifdef AQUA 

#include <gl.h>
#include <glu.h>
#include <glext.h>

#else

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#endif

#ifdef LINUX
#include <GL/glext.h>
#endif


#include "headers.h"


int
get_now_mapped(void);


void
set_now_mapped(int val);


void
glpOpenGLInitialize(void);


void
BackEndClearBuffer(void);

void 
BackEndLightsOff(void);

void 
BackEndHeadlightOff(void);

void
BackEndHeadlightOn(void);

#ifndef AQUA
extern Display *dpy;
extern Window win;
extern void openMainWindow(Display *dpy, Window *win, GLXContext *glocx);
extern void resetGeometry();
#endif
extern void glpOpenGLInitialize(void);

#endif /* __OPENGL_UTILS_H_ */
