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
#include <GL/glext.h>
#endif

#include <unistd.h>
#include <stdio.h>

#ifndef AQUA 
#include <X11/cursorfont.h>
#ifdef XF86V4
#include <X11/extensions/xf86vmode.h>
#endif
#include <X11/keysym.h>
#endif


#define OPENGL_NOVIRT


void
set_render_frame();


#endif /* __OPENGL_UTILS_H_ */
