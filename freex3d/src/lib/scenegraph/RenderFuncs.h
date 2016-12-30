/*


Proximity sensor macro.

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#ifndef __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__
#define __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__

void enableGlobalShader(s_shader_capabilities_t *);
void resetGlobalShader(void);
void finishedWithGlobalShader(void);

/* trat: test if a ratio is reasonable */
#define TRAT(a) ((a) > 0 && ((a) < gglobal()->RenderFuncs.hitPointDist || gglobal()->RenderFuncs.hitPointDist < 0))

/* structure for rayhits */
struct currayhit {
	struct X3D_Node *hitNode; /* What node hit at that distance? */
	GLDOUBLE modelMatrix[16]; /* What the matrices were at that node */
	GLDOUBLE projMatrix[16];
	GLDOUBLE justModel[16]; //view taken off, so transfroms from geometry-local to World
};

void get_current_ray(struct point_XYZ* p1, struct point_XYZ* p2);
extern struct point_XYZ r1, r2;         /* in VRMLC.pm */


/* function protos */
int nextlight(void);
void render_node(struct X3D_Node *node);

struct X3D_Anchor *AnchorsAnchor();
void setAnchorsAnchor(struct X3D_Anchor* anchor);

void clearLightTable();

void setLightState(GLint light, int status);
void setLightType(GLint light, int type);
//JAS void saveLightState2(int *ls);
//JAS void restoreLightState2(int ls);
void setLightChangedFlag(GLint light);
void fwglLightfv (int light, int pname, GLfloat *params);
void fwglLightf (int light, int pname, GLfloat param);
void initializeLightTables(void);
void sendAttribToGPU(int myType, int mySize, int  xtype, int normalized, int stride, float *pointer, int, char*, int);
void sendArraysToGPU (int mode, int first, int count);
void sendBindBufferToGPU (GLenum target, GLuint buffer,char *, int);
void sendElementsToGPU (int mode, int count, unsigned short *indices);
void render_hier(struct X3D_Node *p, int rwhat);
void sendLightInfo (s_shader_capabilities_t *me);
void restoreGlobalShader();


typedef struct ivec4 {int X; int Y; int W; int H;} ivec4;
typedef struct ivec2 {int X; int Y;} ivec2;
void pushviewport(Stack *vpstack, ivec4 vp);
void popviewport(Stack *vpstack);
void setcurrentviewport(Stack *_vpstack);
int currentviewportvisible(Stack *vpstack);

typedef struct usehit {
	struct X3D_Node *node;
	double mvm[16];
	void *userdata;
} usehit;
void usehit_add(struct X3D_Node *node, double *modelviewmatrix);
void usehit_add2(struct X3D_Node *node, double *modelviewmatrix, void *userdata);
usehit * usehit_next(struct X3D_Node *node, usehit* lasthit);
void usehit_clear();

#endif /* __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__ */
