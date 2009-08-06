/*
=INSERT_TEMPLATE_HERE=

$Id$

Proximity sensor macro.

*/

#ifndef __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__
#define __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__

/* for X3D_Node */
#include "../vrml_parser/Structs.h"

/* function protos */
int nextlight(void);
void render_node(struct X3D_Node *node);
void initializeShapeCompileThread(void);

#endif /* __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__ */
