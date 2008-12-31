/*
=INSERT_TEMPLATE_HERE=

$Id$

Screen snapshot.

*/

#ifndef __FREEX3D_OPENGL_UTILS_H__
#define __FREEX3D_OPENGL_UTILS_H__


void start_textureTransform (void *textureNode, int ttnum);
void end_textureTransform (void *textureNode, int ttnum);

void
glpOpenGLInitialize(void);


void
BackEndClearBuffer(void);

void
BackEndLightsOff(void);

void lightState (GLint light, int state);


extern void glpOpenGLInitialize(void);


#endif /* __FREEX3D_OPENGL_UTILS_H__ */
