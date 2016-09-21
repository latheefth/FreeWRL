/*


X3D Geometry 3D Component

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



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../opengl/Frustum.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"
#include "../scenegraph/Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"


#include "Collision.h"
#include "Polyrep.h"
#include "LinearAlgebra.h"
#include "Component_Geometry3D.h"


/* used for vertices for VBOs. Note the tc coordinate... */
struct MyVertex
 {
   struct SFVec3f vert;        //Vertex
   struct SFVec3f norm;     //Normal
   struct SFVec2f tc;         //Texcoord0
   struct SFVec3f flat_norm;
 };

 typedef int cquad[4];
typedef int ctri[3];
struct sCollisionGeometry
{
	struct point_XYZ *pts;
	struct point_XYZ *tpts;
	ctri *tris;
	int ntris;
	cquad *quads;
	int nquads;
	int npts;
	double smin[3],smax[3];
};



typedef struct pComponent_Geometry3D{
	int junk; //filler, no variables unless/untill vbos done
	//SphereGeomVBO ShpereIndxVBO - are these per-instance or sharable?
	struct sCollisionGeometry collisionSphere;
	struct sCollisionGeometry collisionCylinder;
	struct sCollisionGeometry collisionCone;
	//struct point_XYZ *collisionSphere_tpts;
	//struct point_XYZ *collisionCylinder_tpts;
	//struct point_XYZ *collisionCone_tpts;
}* ppComponent_Geometry3D;
void *Component_Geometry3D_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_Geometry3D));
	memset(v,0,sizeof(struct pComponent_Geometry3D));
	return v;
}
void Component_Geometry3D_init(struct tComponent_Geometry3D *t){
	//public
	//private
	t->prv = Component_Geometry3D_constructor();
	{
		ppComponent_Geometry3D p = (ppComponent_Geometry3D)t->prv;
		//generic geometry could in theory be static -its read only- but because we calculate it we malloc it,
		// and because we malloc it we need to free it, and because of that we make it 1:1 with gglobal / browser
		// so it can be freed on clear_.
		bzero(&p->collisionSphere,sizeof(struct sCollisionGeometry)); //= {NULL,NULL,NULL,0,NULL,0,0, {0.0,0.0,0.0},{0.0,0.0,0.0}};
		bzero(&p->collisionCylinder,sizeof(struct sCollisionGeometry)); // = {NULL,NULL,NULL,0,NULL,0,0, {0.0,0.0,0.0},{0.0,0.0,0.0}};
		bzero(&p->collisionCone,sizeof(struct sCollisionGeometry)); //=  {NULL,NULL,NULL,0,NULL,0,0, {0.0,0.0,0.0},{0.0,0.0,0.0}};
		//transformed points are not read only -computed once on each frame- and _must_ be per gglobal
		//p->collisionSphere_tpts;
		//p->collisionCylinder_tpts;
		//p->collisionCone_tpts;
	}
}
void Component_Geometry3D_clear(struct tComponent_Geometry3D *t){
	//public
	//private
	{
		ppComponent_Geometry3D p = (ppComponent_Geometry3D)t->prv;
		FREE_IF_NZ(p->collisionCone.tpts);
		FREE_IF_NZ(p->collisionCylinder.tpts);
		FREE_IF_NZ(p->collisionSphere.tpts);

		FREE_IF_NZ(p->collisionCone.tris);
		FREE_IF_NZ(p->collisionCylinder.quads);
		FREE_IF_NZ(p->collisionCylinder.tris);
		FREE_IF_NZ(p->collisionSphere.tris);

		FREE_IF_NZ(p->collisionCone.pts);
		FREE_IF_NZ(p->collisionCylinder.pts);
		FREE_IF_NZ(p->collisionSphere.pts);
	}
}
//ppComponent_Geometry3D p = (ppComponent_Geometry3D)gglobal()->Component_Geometry3D.prv;


static GLfloat VBO_coneSideTexParams[]={
	0.55f, 0.525f, 0.50f,
	0.60f, 0.575f, 0.55f,
	0.65f, 0.625f, 0.60f,
	0.70f, 0.675f, 0.65f,
	0.75f, 0.725f, 0.70f,
	0.80f, 0.775f, 0.75f,
	0.85f, 0.825f, 0.80f,
	0.90f, 0.875f, 0.85f,
	0.95f, 0.925f, 0.90f,
	1.00f, 0.975f, 0.95f,
	0.05f, 0.025f, 0.00f,
	0.10f, 0.075f, 0.05f,
	0.15f, 0.125f, 0.10f,
	0.20f, 0.175f, 0.15f,
	0.25f, 0.225f, 0.20f,
	0.30f, 0.275f, 0.25f,
	0.35f, 0.325f, 0.30f,
	0.40f, 0.375f, 0.35f,
	0.45f, 0.425f, 0.40f,
	0.50f, 0.475f, 0.45f,
	0.55f, 0.525f, 0.50f
};

/*******************************************************************************/

/*  have to regen the shape*/
void compile_Box (struct X3D_Box *node) {
	struct SFVec3f *pt;
	struct SFVec3f *ptr;
	float x = ((node->size).c[0])/2;
	float y = ((node->size).c[1])/2;
	float z = ((node->size).c[2])/2;

	MARK_NODE_COMPILED

	/*  MALLOC memory (if possible)*/
	if (!node->__points.p) ptr = MALLOC (struct SFVec3f *,sizeof(struct SFVec3f)*(36));
	else ptr = node->__points.p;

	/*  now, create points; 6 points per face.*/
	pt = ptr;

	
#define PTF0 (*pt).c[0] =  x; (*pt).c[1] =  y; (*pt).c[2] =  z; pt++;
#define PTF1 (*pt).c[0] = -x; (*pt).c[1] =  y; (*pt).c[2] =  z; pt++;
#define PTF2 (*pt).c[0] = -x; (*pt).c[1] = -y; (*pt).c[2] =  z; pt++;  
#define PTF3 (*pt).c[0] =  x; (*pt).c[1] = -y; (*pt).c[2] =  z; pt++;
#define PTR0 (*pt).c[0] =  x; (*pt).c[1] =  y; (*pt).c[2] =  -z; pt++;
#define PTR1 (*pt).c[0] = -x; (*pt).c[1] =  y; (*pt).c[2] =  -z; pt++;
#define PTR2 (*pt).c[0] = -x; (*pt).c[1] = -y; (*pt).c[2] =  -z; pt++;
#define PTR3 (*pt).c[0] =  x; (*pt).c[1] = -y; (*pt).c[2] =  -z; pt++;


	PTF0 PTF1 PTF2  PTF0 PTF2 PTF3 /* front */
	PTR2 PTR1 PTR0  PTR3 PTR2 PTR0 /* back  */
	PTF0 PTR0 PTR1  PTF0 PTR1 PTF1 /* top   */
	PTF3 PTF2 PTR2  PTF3 PTR2 PTR3 /* bottom */
	PTF0 PTF3 PTR3 	PTF0 PTR3 PTR0 /* right */
	PTF1 PTR1 PTR2  PTF1 PTR2 PTF2 /* left */

	/* finished, and have good data */
	node->__points.p = ptr;
}
#undef PTF0
#undef PTF1
#undef PTF2
#undef PTR0
#undef PTR1
#undef PTR2
#define DESIRE(whichOne,zzz) ((whichOne & zzz)==zzz)
void render_Box (struct X3D_Box *node) {
	extern GLfloat boxtex[];		/*  in CFuncs/statics.c*/
	extern GLfloat boxnorms[];		/*  in CFuncs/statics.c*/
	extern ushort boxwireindices[];
	struct textureVertexInfo mtf = {boxtex,2,GL_FLOAT,0,NULL,NULL};

	float x = ((node->size).c[0])/2;
	float y = ((node->size).c[1])/2;
	float z = ((node->size).c[2])/2;

	/* test for <0 of sides */
	if ((x < 0) || (y < 0) || (z < 0)) return;

	COMPILE_IF_REQUIRED
	if (!node->__points.p) return; /* still compiling */

	/* for BoundingBox calculations */
	setExtent(x,-x,y,-y,z,-z,X3D_NODE(node));

	CULL_FACE(node->solid)

	/*  Draw it; assume VERTEX and NORMALS already defined.*/
	textureDraw_start(&mtf);
	FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(GLfloat *)node->__points.p);
	FW_GL_NORMAL_POINTER (GL_FLOAT,0,boxnorms);

	/* do the array drawing; sides are simple 0-1-2-3, 4-5-6-7, etc quads */
	//if(fwl_getShadingStyle() == 3){
	if(DESIRE(getShaderFlags(),SHADINGSTYLE_WIRE)){
		//wireframe triangles
		sendElementsToGPU(GL_LINES,72,boxwireindices);
	}else{
		sendArraysToGPU (GL_TRIANGLES, 0, 36);
	}

	textureDraw_end();
	gglobal()->Mainloop.trisThisLoop += 24;
}


/*******************************************************************************/

void compile_Cylinder (struct X3D_Cylinder * node) {
	#define CYLDIV 20
	float h = (node->height)/2;
	float r = node->radius;
	int i = 0;
	struct MyVertex cylVert[CYLDIV * 4 * 3];
	int indx = 0;

	/*  have to regen the shape*/
	MARK_NODE_COMPILED


	/* use VBOS for Cylinders? */

		if (node->__cylinderVBO == 0) {
			glGenBuffers(1,(GLuint*) &node->__cylinderVBO);
		}

		/* we create two triangle fans - the cone, and the bottom. */
		/* first, the flat bit on the bottom */
		indx=0;

		if (node->bottom) {
			for (i=0; i<CYLDIV; i++) {
				double angle = PI*2*i/(double)CYLDIV;
				double next_angle = PI*2*(i+1)/(double)CYLDIV;
				/* vertex #1 */
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = -1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(angle); 
				cylVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(angle); 
				indx++;
	
				/* vertex #2 - in the centre */
				cylVert[indx].vert.c[0] = 0.0f;
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = 0.0f;
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = -1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f; 
				cylVert[indx].tc.c[1] = 0.5f; 
				indx++;
	
				/* vertex #3 */
				cylVert[indx].vert.c[0] = r* (float) sin(next_angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(next_angle);
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = -1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(next_angle); 
				cylVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(next_angle); 
				indx++;
			}
		}
		if (node->top) {
			/* same as bottom, but wind'em the other way */
			for (i=0; i<CYLDIV; i++) {
				double angle = PI*2*i/(double)CYLDIV;
				double next_angle = PI*2*(i+1)/(double)CYLDIV;
				/* vertex #1 */
				cylVert[indx].vert.c[0] = r* (float) sin(next_angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(next_angle);
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = 1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(next_angle); 
				cylVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(next_angle); 
				indx++;
	
				/* vertex #2 - in the centre */
				cylVert[indx].vert.c[0] = 0.0f;
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = 0.0f;
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = 1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f; 
				cylVert[indx].tc.c[1] = 0.5f; 
				indx++;
	
				/* vertex #3 */
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = 1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(angle); 
				cylVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(angle); 
				indx++;
			}
		}
	


		/* now, the sides */
		if (node->side) {
			for (i=0; i<CYLDIV; i++) {
				double angle;

				/* Triangle #1 for this segment of the side */
				/* vertex #1 - bottom right */
				angle = (double) (PI * 2 * (i+1.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);

				/* note the angle for normals is half way between faces */
				angle = (double) (PI * 2 * (i+0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle);
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);

				/* note that we use the Cone TC array; assume same division */
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+0];
				cylVert[indx].tc.c[1] = 0.0f; 
				indx++;
	
				/* vertex #2 - top left */
				angle = (double) (PI * 2 * (i+0.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i-0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle); 
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+2];
				cylVert[indx].tc.c[1] = 1.0f; 
				indx++;
	
				/* vertex #3 - bottom left */
				angle = (double) (PI * 2 * (i+0.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i-0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle); 
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+2];
				cylVert[indx].tc.c[1] = 0.0f; 
				indx++;

				/* Triangle #2 for this segment of the side */
				/* vertex #1 - bottom right */
				angle = (double) (PI * 2 * (i+1.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i+0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle);
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+0];
				cylVert[indx].tc.c[1] = 0.0f;
				indx++;
	
				/* vertex #2 - top right */
				angle = (double) (PI * 2 * (i+1.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i+0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle); 
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+0];
				cylVert[indx].tc.c[1] = 1.0f; 
				indx++;
	
				/* vertex #3 - top left */
				angle = (float) (PI * 2 * (i+0.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i-0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle); 
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+2];
				cylVert[indx].tc.c[1] = 1.0f; 
				indx++;
			}

		}

		node->__cylinderTriangles = indx; //more precisely, number of vertices, (3 vertices per triangle)
		{
			//prepare flat normals for flat shading style
			int i3;
			float *flat_normal = MALLOC(GLfloat*,indx*sizeof(struct SFColor));
			for(i=0;i<indx/3;i++){
				i3 = i*3;
				float a[3],b[3],c[3],d[3], e[3], f[3], g[3];
				memcpy(a,cylVert[i3 +0].vert.c,sizeof(struct SFColor));
				memcpy(b,cylVert[i3 +1].vert.c,sizeof(struct SFColor));
				memcpy(c,cylVert[i3 +2].vert.c,sizeof(struct SFColor));
				vecdif3f(d,b,a);
				vecdif3f(e,c,a);
				veccross3f(f,d,e);
				vecnormalize3f(g,f);
				memcpy(cylVert[i3 +0].flat_norm.c,g,sizeof(struct SFColor));
				memcpy(cylVert[i3 +1].flat_norm.c,g,sizeof(struct SFColor));
				memcpy(cylVert[i3 +2].flat_norm.c,g,sizeof(struct SFColor));
			}
		}
		{
			//prepare wireframe indices
			int i3, i6;
			ushort *lindex = MALLOC(ushort *,indx * 2 * sizeof(ushort));
			for(i=0;i<indx/3;i++){
				i3 = i*3;
				i6 = i*6;
				lindex[i6+0] = i3 + 0;
				lindex[i6+1] = i3 + 1;
				lindex[i6+2] = i3 + 1;
				lindex[i6+3] = i3 + 2;
				lindex[i6+4] = i3 + 2;
				lindex[i6+5] = i3 + 0;
			}
			node->__wireindices = lindex;
		}

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, node->__cylinderVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(struct MyVertex)*indx, cylVert, GL_STATIC_DRAW);

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);

}
void clear_Cylinder (struct X3D_Node *node) {
	struct X3D_Cylinder *tnode = (struct X3D_Cylinder *)node;
	if(tnode->__cylinderVBO)
		glDeleteBuffers(1, (const GLuint *) &tnode->__cylinderVBO);
	if(tnode->__wireindices)
		FREE_IF_NZ(tnode->__wireindices);
}

void render_Cylinder (struct X3D_Cylinder * node) {
	extern GLfloat cylsidetex[];		/*  in CFuncs/statics.c*/
	struct textureVertexInfo mtf = {cylsidetex,2,GL_FLOAT,0,NULL,NULL};    

	float h = (node->height)/2;
	float r = node->radius;


	if ((h < 0) || (r < 0)) {return;}

	/* for BoundingBox calculations */
	setExtent(r,-r,h,-h,r,-r,X3D_NODE(node));

	COMPILE_IF_REQUIRED

	CULL_FACE(node->solid)

		// taken from the OpenGL.org website:
		#define BUFFER_OFFSET(i) ((char *)NULL + (i))

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, node->__cylinderVBO);

		FW_GL_VERTEX_POINTER(3, GL_FLOAT, (GLsizei)sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(0));
		   //The starting point of the VBO, for the vertices
		if(DESIRE(getShaderFlags(),SHADINGSTYLE_FLAT)){
			//The starting point of normals, (3+3+2)*4  = 32 +  bytes away
			FW_GL_NORMAL_POINTER(GL_FLOAT, (GLsizei) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(32));   
		}else{
			//The starting point of normals, 3*4  = 12 +  bytes away
			FW_GL_NORMAL_POINTER(GL_FLOAT, (GLsizei) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(12));   
		}
		/* set up texture drawing for this guy */
        mtf.pre_canned_textureCoords = NULL;
		mtf.TC_size = 2;
		mtf.TC_type = GL_FLOAT;
		mtf.TC_stride = sizeof(struct MyVertex);
		mtf.TC_pointer = BUFFER_OFFSET(24);
		textureDraw_start(&mtf);
		/* FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, ConeIndxVBO); */

		if(DESIRE(getShaderFlags(),SHADINGSTYLE_WIRE)){
			//wireframe triangles
			sendElementsToGPU(GL_LINES,node->__cylinderTriangles *2,node->__wireindices);
		}else{
			sendArraysToGPU(GL_TRIANGLES,0,node->__cylinderTriangles);
		}
		/* turn off */
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
	textureDraw_end();
}



/*******************************************************************************/

void compile_Cone (struct X3D_Cone *node) {
	/*  DO NOT change this define, unless you want to recalculate statics below....*/
	#define  CONEDIV 20

	float h = (node->height)/2;
	float r = node->bottomRadius;
	int i;
	struct MyVertex coneVert[CONEDIV * 2 * 3];
	int indx = 0;

	/*  have to regen the shape*/
	MARK_NODE_COMPILED


	if (node->__coneVBO == 0) {
		glGenBuffers(1,(GLuint *) &node->__coneVBO);
	}

	/* we create two triangle fans - the cone, and the bottom. */
	/* first, the flat bit on the bottom */
	indx=0;

	if (node->bottom) {
		for (i=0; i<CONEDIV; i++) {
			double angle = PI*2*i/(double)CONEDIV;
			double next_angle = PI*2*(i+1)/(double)CONEDIV;
			/* vertex #1 */
			coneVert[indx].vert.c[0] = r* (float) sin(angle);
			coneVert[indx].vert.c[1] = (float) -h;
			coneVert[indx].vert.c[2] = r* (float) cos(angle);
			coneVert[indx].norm.c[0] = 0.0f; 
			coneVert[indx].norm.c[1] = -1.0f; 
			coneVert[indx].norm.c[2] = 0.0f;
			coneVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(angle); 
			coneVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(angle); 
			indx++;
	
			/* vertex #2 - in the centre */
			coneVert[indx].vert.c[0] = 0.0f;
			coneVert[indx].vert.c[1] = (float) -h;
			coneVert[indx].vert.c[2] = 0.0f;
			coneVert[indx].norm.c[0] = 0.0f; 
			coneVert[indx].norm.c[1] = -1.0f; 
			coneVert[indx].norm.c[2] = 0.0f;
			coneVert[indx].tc.c[0] = 0.5f; 
			coneVert[indx].tc.c[1] = 0.5f; 
			indx++;
	
			/* vertex #3 */
			coneVert[indx].vert.c[0] = r* (float) sin(next_angle);
			coneVert[indx].vert.c[1] = (float) -h;
			coneVert[indx].vert.c[2] = r* (float) cos(next_angle);
			coneVert[indx].norm.c[0] = 0.0f; 
			coneVert[indx].norm.c[1] = -1.0f; 
			coneVert[indx].norm.c[2] = 0.0f;
			coneVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(next_angle); 
			coneVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(next_angle); 
			indx++;
		}
	}


	/* now, the sides */
	if (node->side) {
		GLfloat *tcp =  VBO_coneSideTexParams;

		for (i=0; i<CONEDIV; i++) {
			double angle;

			/* vertex #1 */
			angle = (double) (PI * 2 * (i+1.0f)) / (double) (CONEDIV);
			coneVert[indx].vert.c[0] = r* (float) sin(angle);
			coneVert[indx].vert.c[1] = (float) -h;
			coneVert[indx].vert.c[2] = r* (float) cos(angle);
			coneVert[indx].norm.c[0] = (float) sin(angle);
			coneVert[indx].norm.c[1] = (float)h/(r*2);
			coneVert[indx].norm.c[2] = (float) cos(angle);

			angle = (double) (PI * 2 * (i+0.0f)) / (double) (CONEDIV);
			coneVert[indx].tc.c[0] = *tcp; tcp++;
			coneVert[indx].tc.c[1] = 0.0f; 
			indx++;
	
			/* vertex #2 - in the centre */
			angle = (float) (PI * 2 * (i+0.5f)) / (double)(CONEDIV);
			coneVert[indx].vert.c[0] = 0.0f;
			coneVert[indx].vert.c[1] = (float) h;
			coneVert[indx].vert.c[2] = 0.0f;
			coneVert[indx].norm.c[0] = (float) sin(angle); 
			coneVert[indx].norm.c[1] = (float)h/(r*2);
			coneVert[indx].norm.c[2] = (float) cos(angle);

			coneVert[indx].tc.c[0] = *tcp; tcp++; 
			coneVert[indx].tc.c[1] = 1.0f;
			indx++;
	
			/* vertex #3 */
			angle = (float) (PI * 2 * (i+0.0f)) / (double) (CONEDIV);
			coneVert[indx].vert.c[0] = r* (float) sin(angle);
			coneVert[indx].vert.c[1] = (float) -h;
			coneVert[indx].vert.c[2] = r* (float) cos(angle);
			coneVert[indx].norm.c[0] = (float) sin(angle); 
			coneVert[indx].norm.c[1] = (float)h/(r*2);
			coneVert[indx].norm.c[2] = (float) cos(angle);

			angle = (float) (PI * 2 * (i+1.0f)) / (double) (CONEDIV);
			coneVert[indx].tc.c[0] = *tcp; tcp++; 
			coneVert[indx].tc.c[1] = 0.0f; 
			indx++;
		}

	}

	node->__coneTriangles = indx;
	{
		//prepare flat normals for flat shading style
		int i3;
		float *flat_normal = MALLOC(GLfloat*,indx*sizeof(struct SFColor));
		for(i=0;i<indx/3;i++){
			i3 = i*3;
			float a[3],b[3],c[3],d[3], e[3], f[3], g[3];
			memcpy(a,coneVert[i3 +0].vert.c,sizeof(struct SFColor));
			memcpy(b,coneVert[i3 +1].vert.c,sizeof(struct SFColor));
			memcpy(c,coneVert[i3 +2].vert.c,sizeof(struct SFColor));
			vecdif3f(d,b,a);
			vecdif3f(e,c,a);
			veccross3f(f,d,e);
			vecnormalize3f(g,f);
			memcpy(coneVert[i3 +0].flat_norm.c,g,sizeof(struct SFColor));
			memcpy(coneVert[i3 +1].flat_norm.c,g,sizeof(struct SFColor));
			memcpy(coneVert[i3 +2].flat_norm.c,g,sizeof(struct SFColor));
		}
	}
	{
		//prepare wireframe indices
		int i3, i6;
		ushort *lindex = MALLOC(ushort *,indx * 2 * sizeof(ushort));
		for(i=0;i<indx/3;i++){
			i3 = i*3;
			i6 = i*6;
			lindex[i6+0] = i3 + 0;
			lindex[i6+1] = i3 + 1;
			lindex[i6+2] = i3 + 1;
			lindex[i6+3] = i3 + 2;
			lindex[i6+4] = i3 + 2;
			lindex[i6+5] = i3 + 0;
		}
		node->__wireindices = lindex;
	}

	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, (GLuint) node->__coneVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct MyVertex)*indx, coneVert, GL_STATIC_DRAW);

	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);

	/* no longer needed */
	FREE_IF_NZ(node->__botpoints.p);
	FREE_IF_NZ(node->__sidepoints.p);
	FREE_IF_NZ(node->__normals.p);    
}
void clear_Cone (struct X3D_Node *node) {
	struct X3D_Cone *tnode = (struct X3D_Cone *)node;
	if(tnode->__coneVBO)
		glDeleteBuffers(1, (const GLuint *) &tnode->__coneVBO);
}

void render_Cone (struct X3D_Cone *node) {
	extern float tribottex[];		/*  in CFuncs/statics.c*/
	struct textureVertexInfo mtf = {tribottex,2,GL_FLOAT,0,NULL,NULL};
				
	float h = (node->height)/2;
	float r = node->bottomRadius;

	if ((h < 0) || (r < 0)) {return;}
	COMPILE_IF_REQUIRED

	/* for BoundingBox calculations */
	setExtent(r,-r,h,-h,r,-r,X3D_NODE(node));


	CULL_FACE(node->solid)
	/*  OK - we have vertex data, so lets just render it.*/
	/*  Always assume GL_VERTEX_ARRAY and GL_NORMAL_ARRAY are enabled.*/
	// taken from the OpenGL.org website:
	#define BUFFER_OFFSET(i) ((char *)NULL + (i))

	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, node->__coneVBO);

	FW_GL_VERTEX_POINTER(3, GL_FLOAT, (GLsizei) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(0));   //The starting point of the VBO, for the vertices
	if(DESIRE(getShaderFlags(),SHADINGSTYLE_FLAT)){
		//The starting point of normals, (3+3+2)*4  = 32 +  bytes away
		FW_GL_NORMAL_POINTER(GL_FLOAT, (GLsizei) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(32));   
	}else{
		//The starting point of normals, 3*4  = 12 +  bytes away
		FW_GL_NORMAL_POINTER(GL_FLOAT, (GLsizei) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(12));   
	}

	/* set up texture drawing for this guy */
    mtf.pre_canned_textureCoords = NULL;
	mtf.TC_size = 2;
	mtf.TC_type = GL_FLOAT;
	mtf.TC_stride = sizeof(struct MyVertex);
	mtf.TC_pointer = BUFFER_OFFSET(24);
	textureDraw_start(&mtf);
PRINT_GL_ERROR_IF_ANY("END1 render_geom");
	if(DESIRE(getShaderFlags(),SHADINGSTYLE_WIRE)){
		//wireframe triangles
		sendElementsToGPU(GL_LINES,node->__coneTriangles *2,node->__wireindices);
	}else{
		sendArraysToGPU(GL_TRIANGLES,0,node->__coneTriangles);
	}
	
PRINT_GL_ERROR_IF_ANY("END2 render_geom");
	/* turn off */
	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
	FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);
	textureDraw_end();
}



/*******************************************************************************/
/* how many triangles in this sphere? SPHDIV strips, each strip
	has 2 triangles, and each triangles has 3 vertices and there are
	SPHDIV rows */
#define SPHDIV 20
#define TRISINSPHERE (SPHDIV*3* SPHDIV)


void compile_Sphere (struct X3D_Sphere *node) {
	#define INIT_TRIG1(div) t_aa = (float) sin(PI/(div)); t_aa *= 2*t_aa; t_ab =(float) -sin(2*PI/(div));
	#define START_TRIG1 t_sa = 0; t_ca = -1;
	#define UP_TRIG1 t_sa1 = t_sa; t_sa -= t_sa*t_aa - t_ca * t_ab; t_ca -= t_ca * t_aa + t_sa1 * t_ab;
	#define SIN1 t_sa
	#define COS1 t_ca
	#define INIT_TRIG2(div) t2_aa = (float) sin(PI/(div)); t2_aa *= 2*t2_aa; t2_ab = (float) -sin(2*PI/(div));
	#define START_TRIG2 t2_sa = -1; t2_ca = 0;
	#define UP_TRIG2 t2_sa1 = t2_sa; t2_sa -= t2_sa*t2_aa - t2_ca * t2_ab; t2_ca -= t2_ca * t2_aa + t2_sa1 * t2_ab;
	#define SIN2 t2_sa
	#define COS2 t2_ca

	/*  make the divisions 20; dont change this, because statics.c values*/
	/*  will then need recaculating.*/
	extern GLfloat spherenorms[];		/*  side normals*/
	extern float spheretex[];		/*  in CFuncs/statics.c*/

	int count;
	float rad = node->radius;
	struct SFVec3f *ptr;

	int v; int h;
	float t_aa, t_ab, t_sa, t_ca, t_sa1;
	float t2_aa, t2_ab, t2_sa, t2_ca, t2_sa1;
	struct SFVec3f *pts;
	//ushort *pindices;

	/*  have to regen the shape*/
	MARK_NODE_COMPILED

	/*  MALLOC memory (if possible)*/
	/*  2 vertexes per points. (+1, to loop around and close structure)*/
	if (!node->__points.p) {
		// malloc points. We seem to never need the ".n" size param, but initialize it
		// anyway to keep things clean and even.
		ptr = MALLOC (struct SFVec3f *,sizeof(struct SFVec3f) * (SPHDIV+1) * (SPHDIV+1) * 2);
		node->__points.n = SPHDIV * (SPHDIV+1) * 2;
	} else ptr = node->__points.p;

	pts = ptr;
	count = 0;

	INIT_TRIG1(SPHDIV)
	INIT_TRIG2(SPHDIV)

	START_TRIG1
	{
		extern GLfloat spherenorms[];		/*  side normals*/
		extern float spheretex[];		/*  in CFuncs/statics.c*/

		//int myVertexVBOSize = (int) (sizeof(struct SFVec3f) +
		//							sizeof(struct SFVec3f) +
		//							sizeof(struct SFVec2f)) * SPHDIV * (SPHDIV+1) * 2;
		int myVertexVBOSize = (int) sizeof(struct MyVertex) * SPHDIV * (SPHDIV+1) * 2;
		struct MyVertex *SphVBO = MALLOC(struct MyVertex *, myVertexVBOSize);
		struct SFVec3f *myNorms = (struct SFVec3f*)spherenorms;
		struct SFVec2f *myTex = (struct SFVec2f*)spheretex;

		if (node->_sideVBO == 0) {
			glGenBuffers(1,(GLuint *) &node->_sideVBO);
		}
		for(v=0; v<SPHDIV; v++) {
			float vsin1 = SIN1;
			float vcos1 = COS1, vsin2,vcos2;
			UP_TRIG1
			vsin2 = SIN1;
			vcos2 = COS1;
			START_TRIG2
			for(h=0; h<=SPHDIV; h++) {
				float hsin1 = SIN2;
				float hcos1 = COS2;
				UP_TRIG2
				pts[count].c[0] = rad * vsin2 * hcos1;
				pts[count].c[1] = rad * vcos2;
				pts[count].c[2] = rad * vsin2 * hsin1;
				
				/* copy these points into the MyVertex Sphere VBO */
				memcpy (SphVBO[count].vert.c, pts[count].c, sizeof (struct SFVec3f));
				memcpy (SphVBO[count].norm.c, myNorms[count].c, sizeof (struct SFVec3f));
				memcpy (SphVBO[count].tc.c, myTex[count].c, sizeof (struct SFVec2f));
				count++;
				pts[count].c[0] = rad * vsin1 * hcos1;
				pts[count].c[1] = rad * vcos1;
				pts[count].c[2] = rad * vsin1 * hsin1;
				/* copy these points into the MyVertex Sphere VBO */
				memcpy (SphVBO[count].vert.c, pts[count].c, sizeof (struct SFVec3f));
				memcpy (SphVBO[count].norm.c, myNorms[count].c, sizeof (struct SFVec3f));
				memcpy (SphVBO[count].tc.c, myTex[count].c, sizeof (struct SFVec2f));
				count++;
			}
		}


		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, (GLuint) node->_sideVBO);
		glBufferData(GL_ARRAY_BUFFER, myVertexVBOSize, SphVBO, GL_STATIC_DRAW);

		if (node->__SphereIndxVBO == 0) {
			ushort pindices[TRISINSPHERE*2];
			ushort *pind; // = pindices;
			int row;
			int indx;
			pind = pindices;

			glGenBuffers(1,(GLuint *)&node->__SphereIndxVBO);
			//for (count=0; count<TRISINSPHERE*2; count++) pindices[count]=0;
			for (row=0; row<SPHDIV; row++) {
				indx=42*row;
				for (count = 0; count < SPHDIV*2; count+=2) {
					*pind = indx; pind++;
					*pind = indx+1; pind++;
					*pind = indx+2; pind++;
					//printf ("wrote %u %u %u\n",indx, indx+1, indx+2);
					*pind = indx+2; pind++;
					*pind = indx+1; pind++;
					*pind = indx+3; pind++;
					//printf ("wrote %u %u %u\n",indx+2, indx+1, indx+3);
					indx+=2;
				}
			}
			node->__pindices = pindices;
 			FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, node->__SphereIndxVBO);
 			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*TRISINSPHERE*2, pindices, GL_STATIC_DRAW);

			{
				//prepare wireframe indices - we'll use the pindices from above, still on the stack
				int i, i3, i6, nlines, ntris;
				ushort lindex[SPHDIV*SPHDIV*2*3*2];
				ntris = SPHDIV * SPHDIV * 2;
				nlines = ntris * 3;
				//ushort *lindex = MALLOC(ushort *,nlines * 2 * sizeof(ushort));
				glGenBuffers(1,(GLuint *) &node->__wireindicesVBO);
				for(i=0;i<ntris;i++){
					i3 = i*3;
					i6 = i*6;
					lindex[i6+0] = pindices[i3 + 0];
					lindex[i6+1] = pindices[i3 + 1];
					lindex[i6+2] = pindices[i3 + 1];
					lindex[i6+3] = pindices[i3 + 2];
					lindex[i6+4] = pindices[i3 + 2];
					lindex[i6+5] = pindices[i3 + 0];
				}
				//node->__wireindices = lindex;
 				FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, node->__wireindicesVBO);
 				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*SPHDIV*SPHDIV*2*3, lindex, GL_STATIC_DRAW);
			}
		}


		FREE_IF_NZ(SphVBO);
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);

	}
	/* finished - for threading */
	node->__points.p = ptr;
}

void clear_Sphere (struct X3D_Node *node) {
	struct X3D_Sphere *tnode = (struct X3D_Sphere *)node;
	if(tnode->_sideVBO)
		glDeleteBuffers(1, (const GLuint *) &tnode->_sideVBO);
	if(tnode->__SphereIndxVBO)
		glDeleteBuffers(1, (const GLuint *) &tnode->__SphereIndxVBO);
}


void render_Sphere (struct X3D_Sphere *node) {
	/*  make the divisions 20; dont change this, because statics.c values*/
	/*  will then need recaculating.*/
	
	extern GLfloat spherenorms[];		/*  side normals*/
	extern float spheretex[];		/*  in CFuncs/statics.c*/

	struct textureVertexInfo mtf = {spheretex,2,GL_FLOAT,0,NULL,NULL};


	float rad = node->radius;

	if (rad<=0.0) { return;}

	/* for BoundingBox calculations */
	setExtent(rad,-rad,rad,-rad,rad,-rad,X3D_NODE(node));

	COMPILE_IF_REQUIRED

	CULL_FACE(node->solid)

	/*  Display the shape*/

	// taken from the OpenGL.org website:
	#define BUFFER_OFFSET(i) ((char *)NULL + (i))

	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, (GLuint) node->_sideVBO);

	FW_GL_VERTEX_POINTER(3, GL_FLOAT, sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(0));   //The starting point of the VBO, for the vertices
	FW_GL_NORMAL_POINTER(GL_FLOAT, sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(12));   //The starting point of normals, 12 bytes away
	mtf.pre_canned_textureCoords = NULL;
	mtf.TC_size = 2;
	mtf.TC_type = GL_FLOAT;
	mtf.TC_stride = sizeof(struct MyVertex);
	mtf.TC_pointer = BUFFER_OFFSET(24);
	textureDraw_start(&mtf);

	if(DESIRE(getShaderFlags(),SHADINGSTYLE_WIRE)){
		//wireframe triangles
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, node->__wireindicesVBO);
		sendElementsToGPU(GL_LINES,TRISINSPHERE *3, (ushort *)BUFFER_OFFSET(0)); //node->__wireindices);
	}else{
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, node->__SphereIndxVBO);
		sendElementsToGPU (GL_TRIANGLES, TRISINSPHERE, (ushort *)BUFFER_OFFSET(0));   //The starting point of the IBO
	}

	/* turn off */
	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
	FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);

	textureDraw_end();

///* DJTRACK_PICKSENSORS */
//#ifdef DJTRACK_PICKSENSORS
//	if(active_picksensors()) {
//		struct X3D_Node *chained ;
//		int pickable = FALSE ;
//		chained = (struct X3D_Node *) node ;
//
//		node->_renderFlags = node->_renderFlags & (0xFFFF^VF_inPickableGroup);
//		chainUpPickableTree((struct X3D_Node *) node, chained, -1) ;
//		pickable = (node->_renderFlags & VF_inPickableGroup) ;
//
//		if(pickable) {
//			rewind_picksensors();
//			/* dump_scene(stdout, 0, (struct X3D_Node *)rootNode); */
//			//dump_scene(stdout, 0, (struct X3D_Node *)node);
//			while (more_picksensors() == TRUE) {
//				void * XX = get_picksensor();
//				printf("%s,%d render_Sphere %p test PickSensor\n",__FILE__,__LINE__,XX);
//				//dump_scene (stdout, 0, XX) ;
//				advance_picksensors();
//			}
//		}
//	}
//#endif
}

void render_IndexedFaceSet (struct X3D_IndexedFaceSet *node) {
	COMPILE_POLY_IF_REQUIRED (node->coord, node->fogCoord, node->color, node->normal, node->texCoord)
	if (!node->_intern) return;
	CULL_FACE(node->solid)
	render_polyrep(node);
}

void render_ElevationGrid (struct X3D_ElevationGrid *node) {
	COMPILE_POLY_IF_REQUIRED (NULL, node->fogCoord, node->color, node->normal, node->texCoord)
	CULL_FACE(node->solid)
	render_polyrep(node);
}

void render_Extrusion (struct X3D_Extrusion *node) {
	COMPILE_POLY_IF_REQUIRED (NULL,NULL,NULL,NULL,NULL)
	CULL_FACE(node->solid)
	render_polyrep(node);
}

/* Fast culling possibilities:
   a) sphere-sphere
   b) MBB (axes-aligned Minimum Bounding Box or 'extent') (pr->maxVals[i], pr->minVals[i] i=0,1,2 or equivalent)
   two optional spaces in which to do the comparison: shape space or avatar(collision) space
   1. shape sphere/MBB > [Shape2Collision] > collision space coordinates. Or - 
   2. avatar collision volume > [Collision2Shape] > into shape coords
   which ever way, we expect false positives, but don't want any false negatives which could make you fall through 
   terrain or slide through walls. To be safe, error on the side of bigger collision volumes for shape and avatar.
*/

/* note as of Jan 16, 2010 - not all collide_<shape> scenarios come through the following avatarCollisionVolumeIntersectMBB. 
   All walking scnearios do. 
   collide_generic, collide_box, collide_extrusion, collide_Text, collide_Rectangle2D: all walk/fly/examine do. 
   collide_sphere,collide_cylinder,collide_cone - only walk comes through here. fly/examine done with the original analytical shape/analytical avatar code.
*/

int avatarCollisionVolumeIntersectMBB(double *modelMatrix, double *prminvals, double* prmaxvals) 
{
	/* returns 1 if your shape MBB overlaps the avatar collision volume
	   modelMatrix[16] == shape2collision transform. collision space is like avatar space, except vertical aligned to gravity - current bound viewpoint(walk), or avatar(fly/examine) or global gravity
	   prminvals[3],prmaxvals[3] - MBB minimum bounding box or extent of shape, in shape space
	   the fastTestMethod can be set in mainloop.c render_collisions()
	*/
	struct sNaviInfo *naviinfo;
	ttglobal tg = gglobal();
	struct sFallInfo* fi = FallInfo();
	naviinfo = (struct sNaviInfo*)tg->Bindable.naviinfo;
	if(fi->walking)
	{
		/* cylindrical / popcycle shaped avatar collision volume */
		GLDOUBLE awidth = naviinfo->width; /*avatar width*/
		GLDOUBLE atop = naviinfo->width; /*top of avatar (relative to eyepoint)*/
		GLDOUBLE abottom = -naviinfo->height; /*bottom of avatar (relative to eyepoint)*/
		GLDOUBLE astep = -naviinfo->height+naviinfo->step;

		/* the following 2 flags are checked a few levels down, in the triangle/quad intersect avatar code get_poly_disp_2(p, 3, nused) */
		fi->checkCylinder = 1; /* 1= shape MBB overlaps avatar collision MBB, else 0 */
		fi->checkFall = 1;     /* 1= shape MBB overlaps avatar fall/climb line segment else 0 */
		fi->checkPenetration = 1;

		{
		   /*  minimum bounding box MBB test in avatar/collision space */
			double foot = abottom;
			if(fi->allowClimbing) foot = astep; /* < popcycle shaped avatar collision volume */
			/* do fall/climb bounds test (popcycle stick) */
			fi->checkFall = fi->canFall; /* only do the falling/climbing if we aren't already colliding - colliding over-rules falling/climbing */
			if(fi->checkFall) fi->checkFall = fast_ycylinder_MBB_intersect_collisionSpace(-fi->fallHeight,atop,0.0, modelMatrix, prminvals, prmaxvals);
			/* do collision volume bounds test (popcycle succulent part)*/
			fi->checkCylinder = fast_ycylinder_MBB_intersect_collisionSpace(foot,atop,awidth, modelMatrix, prminvals, prmaxvals);
			fi->checkPenetration = 0;
			if( fi->canPenetrate )
				fi->checkPenetration = overlapMBBs(fi->penMin,fi->penMax,prminvals,prmaxvals);
			if(!fi->checkCylinder && !fi->checkFall && !fi->checkPenetration){/*printf("$");*/ return 0;} 
		}
	}
	else
	{
		/* examine/fly spherical avatar collision volume */
		GLDOUBLE awidth = naviinfo->width; /*avatar width - used as avatar sphere radius*/
		if( !fast_sphere_MBB_intersect_collisionSpace(awidth, modelMatrix, prminvals, prmaxvals )) return 0;
	}
	return 1;
}

int avatarCollisionVolumeIntersectMBBf(double *modelMatrix, float *minVals, float *maxVals)
{
	/* converts pr.floats to doubles and re-delegates */
	int i;
	GLDOUBLE prminvals[3],prmaxvals[3];
	for(i=0;i<3;i++)
	{
		prminvals[i] = minVals[i]; 
		prmaxvals[i] = maxVals[i];
	}
	return avatarCollisionVolumeIntersectMBB(modelMatrix, prminvals,prmaxvals);
}

void collide_genericfaceset (struct X3D_IndexedFaceSet *node ){
	GLDOUBLE modelMatrix[16];
	struct point_XYZ delta = {0,0,0};
	#ifdef RENDERVERBOSE
	struct point_XYZ t_orig = {0,0,0};
	#endif
	struct X3D_PolyRep pr;
	prflags flags = 0;
	int change = 0;

	/* JAS - first pass, intern is probably zero */
	if (node->_intern == NULL) return;

	/* JAS - no triangles in this text structure */
	if (node->_intern->ntri == 0) return;

	/*save changed state.*/
	if(node->_intern) change = node->_intern->irep_change;
	COMPILE_POLY_IF_REQUIRED (NULL, NULL, NULL, NULL, NULL)


	if(node->_intern) node->_intern->irep_change = change;
	/*restore changes state, invalidates mk_polyrep work done, so it can be done
		correclty in the RENDER pass */

	if(!node->solid) {
		flags = flags | PR_DOUBLESIDED;
	}

	pr = *(node->_intern);


	/* IndexedFaceSets are "different", in that the user specifies points, among
		other things.  The rendering pass takes these external points, and streams
		them to make rendering much faster on hardware accel. We have to check to
		see whether we have got here before the first rendering of a possibly new
		IndexedFaceSet */
	if (!pr.actualCoord) {
		struct Multi_Vec3f* tmp;
		tmp = getCoordinate(node->coord,"Collision");
		pr.actualCoord = (float *) tmp->p;
	}

	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);
	/* 
	For examine and fly navigation modes, there's no gravity direction. 
	Avatar collision volume is aligned to avatar and is spherical and 
	symmetrically directional. This is the simple case. Specifications say for 
	walk navigation mode gravity vector is down {0,-1,0} with respect to (wrt) the 
	currently bound viewpoint, not including the viewpoint's orientation 
	field and not including avatar navigation/tilt away from its parent 
	bound-viewpoint pose. When you collide in walk mode, the avatar collision
	volume is aligned to bound-viewpoint. This is a slightly more complex case.
	To generalize the 2 cases, some	Definitions:
	Spaces:
		Avatar space 
			- gravity is avatar-down
			- Avatar is at {0,0,0} and +Y up is as you see up in your current view, 
				+Z aft and +X to the right
		BoundViewpoint space - currently bound viewpoint (transform parent to avatar)
		BVVA space - Bound-Viewpoint-Vertically-aligned Avatar-centric 
			- same as Avatar space with avatar at {0,0,0} except tilted so that gravity 
				is in the direction of down {0,-1,0} for the currently bound viewpoint 
				(instead for avatar), as per specs
			- gravity is bound-viewpoint down
		Collision space - Fly/Examine mode: Avatar space. Walk mode: BVVA space
			- the avatar collision volume - height, stepsize, width - are defined for 
				collision space and axes aligned with it
		Shape space - raw shape <Coordinate> space
	Transforms:
		Bound2Avatar     - transforms from BoundViewpoint space to Avatar space 
				            - computed from viewer.quat,viewer.pos
		Avatar2BVVA,BVVA2Avatar 
						- computed from downvector in BoundViewpoint space transformed
							via Bound2Avatar into Avatar space
						- constant for a frame, computable once navigation mode and 
							avatar pose is know for the frame
		Avatar2Collision - Fly/Examine: Identity,    Walk: Avatar2BVVA
		Collision2Avatar - Fly/Examine: Identity,    Walk: BVVA2Avatar
		Shape2Collision  - Fly/Examine: modelMatrix, Walk: Avatar2BVVA*modelMatrix

	goal: transform shape geometry into Collision space. (The avatar collision 
			    volume is by definition in collision space.)
			Do some collisions between shape and avatar.
			Transform collision correction deltas from collision space to avatar space, 
				apply deltas to avatar position.
	implementation:
		transform shape into collision space - Fly/Examine:modelMatrix 
				or Walk:(Avatar2BVVA * modelMatrix)
		transform collision correction deltas from collision space to avatar space: 
			- done in Mainloop.c get_collisionoffset() with FallInfo.collision2avatar
	*/

	matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
	//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 


	#ifdef RENDERVERBOSE
	t_orig.x = modelMatrix[12];
	t_orig.y = modelMatrix[13];
	t_orig.z = modelMatrix[14];
	#endif

	/* at this point, whichever method - modelMatrix is Shape2Collision 
		and upvecmat is Collision2Avatar 
		- pr.actualCoord - these are Shape space coordinates
		They will be transformed into CollisionSpace coordinates by the modelMatrix transform.
	*/
	if(!avatarCollisionVolumeIntersectMBBf(modelMatrix, pr.minVals, pr.maxVals))return;
	/* passed fast test. Now for gruelling test */
	delta = polyrep_disp2(pr,modelMatrix,flags); //polyrep_disp(abottom,atop,astep,awidth,pr,modelMatrix,flags);
	/* delta is in collision space */
	/* lets say you are floating above ground by 3 units + avatar.height 1.75 = 4.75. 
	Then delta = (0,3,0)*/
	vecscale(&delta,&delta,-1);
	accumulate_disp(CollisionInfo(),delta); /* we are accumulating in collision space (fly/examine: avatar space, walk: BVVA space) */

#ifdef RENDERVERBOSE
	if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
		/*		   printmatrix(modelMatrix);*/
		fprintf(stderr,"COLLISION_IFS: (%f %f %f) (%f %f %f)\n",
			t_orig.x, t_orig.y, t_orig.z,
			delta.x, delta.y, delta.z
			);
	}
#endif

}


static void collisionSphere_init(struct X3D_Sphere *node)
{
	int i,j,count;
	/* for debug int k, biggestNum; */
	double radinverse;
	struct SFVec3f *pts = node->__points.p;
	ppComponent_Geometry3D p = (ppComponent_Geometry3D)gglobal()->Component_Geometry3D.prv;

	/*  re-using the compile_sphere node->__points data which is organized into GL_QUAD_STRIPS
		my understanding: there are SPHDIV/2 quad strips. Each quadstrip has SPHDIV quads, and enough points to do that many quads
		without sharing points with other quadstrips. So to make SPHDIV quads, you need 2 rows of SPHDIV+1 points.
		because there's 2 triangles per quad, there should be 2x as many triangles as quads.
		num quads = SPHDIV/2 x SPHDIV
		num points = SPHDIV/2 X [(SPHDIV+1) X 2] = SPHDIV*(SPHDIV+1)
		num tris = quads x 2 = SPHDIV X SPHDIV 
	*/

	p->collisionSphere.npts = SPHDIV*(SPHDIV+1);
	p->collisionSphere.pts = MALLOC(void *, p->collisionSphere.npts * sizeof(struct point_XYZ));
	p->collisionSphere.tpts = MALLOC(void *, p->collisionSphere.npts * sizeof(struct point_XYZ));
	/* undo radius field on node so radius == 1 (generic, for all spheres, scale back later) */
	radinverse = 1.0;
	if( !APPROX(node->radius,0.0) ) radinverse = 1.0/node->radius;
	for(i=0;i<p->collisionSphere.npts;i++)
	{
		p->collisionSphere.pts[i].x = pts[i].c[0] * radinverse;
		p->collisionSphere.pts[i].y = pts[i].c[1] * radinverse;
		p->collisionSphere.pts[i].z = pts[i].c[2] * radinverse;
	}


	p->collisionSphere.ntris = SPHDIV * SPHDIV;
	p->collisionSphere.tris = MALLOC(void *, p->collisionSphere.ntris * sizeof(ctri));
	p->collisionSphere.nquads = 0;
	count = 0;
	for(i = 0; i < SPHDIV/2; i ++)  
	{ 
		/* one quad strip  of  SPHDIV quads or SPHDIV*2 triangles */
		for(j=0;j<(2*SPHDIV);j+=2) //=+)
		{
			/* first triangle */
			p->collisionSphere.tris[count][0] = i*(SPHDIV+1)*2 + j;
			p->collisionSphere.tris[count][1] = i*(SPHDIV+1)*2 + j+1;
			p->collisionSphere.tris[count][2] = i*(SPHDIV+1)*2 + j+2; 
			count ++;
			/* second triangle */
			p->collisionSphere.tris[count][0] = i*(SPHDIV+1)*2 + j+3; 
			p->collisionSphere.tris[count][1] = i*(SPHDIV+1)*2 + j+2; 
			p->collisionSphere.tris[count][2] = i*(SPHDIV+1)*2 + j+1; 
			count ++;
		}
	}
	/* count should == num triangles 
	debug check on indexing - biggestNum should == npts -1
	biggestNum = 0;
	for(i=0;i<collisionSphere.ntris;i++)
		for(j=0;j<3;j++)
			biggestNum = max(biggestNum,collisionSphere.tris[i][j]);
	*/
	/* MBB */
	for(i=0;i<3;i++)
	{
		p->collisionSphere.smin[i] = -1.0; //rad;
		p->collisionSphere.smax[i] =  1.0; //rad;
	}

}
#ifdef DEBUGGING_CODE
DEBUGGING_CODEint collisionSphere_render(double radius)
DEBUGGING_CODE{
DEBUGGING_CODE	/* I needed to verify the collision mesh sphere was good, and it uses triangles, so I drew it the triangle way and it looked good 
DEBUGGING_CODE	   to see it draw, you need to turn on collision and get close to a sphere - then it will initialize and start drawing it.
DEBUGGING_CODE	*/
DEBUGGING_CODE	int i,j,count,highest;
DEBUGGING_CODE	count = 0;
DEBUGGING_CODE	highest = 0;
DEBUGGING_CODE	for(i =0; i < collisionSphere.ntris; i++)  
DEBUGGING_CODE	{ 
DEBUGGING_CODE		struct point_XYZ pts[3]; //,a,b,n;
DEBUGGING_CODE		pts[0] = collisionSphere.pts[collisionSphere.tris[i][0]];
DEBUGGING_CODE		pts[1] = collisionSphere.pts[collisionSphere.tris[i][1]];
DEBUGGING_CODE		pts[2] = collisionSphere.pts[collisionSphere.tris[i][2]];
DEBUGGING_CODE		FW_GL_BEGIN(GL_TRIANGLES);
DEBUGGING_CODE		for(j=0;j<3;j++)
DEBUGGING_CODE			FW_GL_VERTEX3D(pts[j].x*radius,pts[j].y*radius,pts[j].z*radius);
DEBUGGING_CODE#define FW_GL_END() glEnd()
DEBUGGING_CODE		FW_GL_END();
DEBUGGING_CODE	}
DEBUGGING_CODE	return 0;
DEBUGGING_CODE}
#endif

struct point_XYZ get_poly_disp_2(struct point_XYZ* p, int num, struct point_XYZ n);
#define FLOAT_TOLERANCE 0.00000001
void collide_Sphere (struct X3D_Sphere *node) {
	struct point_XYZ t_orig = {0,0,0}; /*transformed origin*/
	struct point_XYZ p_orig= {0,0,0} ; /*projected transformed origin */
	struct point_XYZ n_orig = {0,0,0}; /*normal(unit length) transformed origin */
	GLDOUBLE modelMatrix[16];
	GLDOUBLE awidth,atop,abottom,dist2;
	struct point_XYZ delta = {0,0,0};
	GLDOUBLE radius;
	struct sNaviInfo *naviinfo;
	ttglobal tg = gglobal();
	ppComponent_Geometry3D p = (ppComponent_Geometry3D)gglobal()->Component_Geometry3D.prv;
	naviinfo = (struct sNaviInfo*)tg->Bindable.naviinfo;

	/*easy access, naviinfo.step unused for sphere collisions */
	awidth = naviinfo->width; /*avatar width*/
	atop = naviinfo->width; /*top of avatar (relative to eyepoint)*/
	abottom = -naviinfo->height; /*bottom of avatar (relative to eyepoint)*/

		/* this sucker initialized yet? */
		if (node->__points.p == NULL) return;


	/* get the transformed position of the Sphere, and the scale-corrected radius. */
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

	/* apply radius to generic r=1 sphere */
	//radscale.x = radscale.y = radscale.z = node->radius;
	//scale_to_matrix (modelMatrix, &radscale);
	//matmultiply(modelMatrix,FallInfo.avatar2collision,modelMatrix); 

	if(FallInfo()->walking)
	{
		/* mesh method */

		int i;
		double disp;
		struct point_XYZ n;
		struct point_XYZ a,b, dispv, maxdispv = {0,0,0};
		struct point_XYZ radscale;
		double maxdisp = 0;
		radscale.x = radscale.y = radscale.z = node->radius;
		scale_to_matrix (modelMatrix, &radscale);
		matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
		//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 

		if(!p->collisionSphere.npts) collisionSphere_init(node);
		if( !avatarCollisionVolumeIntersectMBB(modelMatrix, p->collisionSphere.smin,p->collisionSphere.smax)) return;

		for(i=0;i<p->collisionSphere.npts;i++)
			transform(&p->collisionSphere.tpts[i],&p->collisionSphere.pts[i],modelMatrix);

		for(i = 0; i < p->collisionSphere.ntris; i++) 
		{
			/*only clip faces "facing" origin */
			//if(vecdot(&n[ci],&middle) < 0.) 
			{
				struct point_XYZ pts[3];
				pts[0] = p->collisionSphere.tpts[p->collisionSphere.tris[i][0]];
				pts[1] = p->collisionSphere.tpts[p->collisionSphere.tris[i][1]];
				pts[2] = p->collisionSphere.tpts[p->collisionSphere.tris[i][2]];
				/* compute normal - could compute once in shapespace then transform */
				VECDIFF(pts[1],pts[0],a);
				VECDIFF(pts[2],pts[1],b); /* or [2] [0] direction not sensitive for some functions */
				veccross(&n,a,b); /* 6 multiplies */
				vecnormal(&n,&n); 
				dispv = get_poly_disp_2(pts,3,n);
				disp = vecdot(&dispv,&dispv);
				if( (disp > FLOAT_TOLERANCE) && (disp > maxdisp) ){
					maxdisp = disp;
					maxdispv = dispv;
				}
			}
		}
		delta = maxdispv;
		    vecscale(&delta,&delta,-1);
	}else{
		/* easy analytical sphere-sphere stuff */
		matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
		//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 

		t_orig.x = modelMatrix[12];
		t_orig.y = modelMatrix[13];
		t_orig.z = modelMatrix[14];
		radius = pow(det3x3(modelMatrix),1./3.) * node->radius;

		/* squared distance to center of sphere (on the y plane)*/
		dist2 = t_orig.x * t_orig.x + t_orig.z * t_orig.z;

		/* easy tests. clip as if sphere was a box */
		/*clip with cylinder */

		if(dist2 - (radius + awidth) * (radius +awidth) > 0) {
			return;
		}
		/*clip with bottom plane */
		if(t_orig.y + radius < abottom) {
			return;
		}
		/*clip with top plane */
		if(t_orig.y-radius > atop) {
			return;
		}

		/* project onto (y x t_orig) plane */
		p_orig.x = sqrt(dist2);
		p_orig.y = t_orig.y;
		p_orig.z = 0;
		/* we need this to unproject rapidly */
		/* n_orig is t_orig.y projected on the y plane, then normalized. */
		n_orig.x = t_orig.x;
		n_orig.y = 0.0;
		n_orig.z = t_orig.z;
		VECSCALE(n_orig,1.0/p_orig.x); /*equivalent to vecnormal(n_orig);, but faster */
		#ifdef RENDERVERBOSE
		printf ("sphere, p_orig %lf %lf %lf, n_orig %lf %lf %lf\n",p_orig.x, p_orig.y, p_orig.z, n_orig.x, n_orig.y, n_orig.z);
		#endif

		/* 5 cases : sphere is over, over side, side, under and side, under (relative to y axis) */
		/* these 5 cases correspond to the 5 vornoi regions of the cylinder */
		if(p_orig.y > atop) {
			if(p_orig.x < awidth) {
				#ifdef RENDERVERBOSE
				printf(" /* over, we push down. */ \n");
				#endif
				delta.y = (p_orig.y - radius) - (atop);
			} else {
				struct point_XYZ d2s;
				GLDOUBLE ratio;
				#ifdef RENDERVERBOSE
				printf(" /* over side */ \n");
				#endif
				/* distance vector from corner to center of sphere*/
				d2s.x = p_orig.x - awidth;
				d2s.y = p_orig.y - (atop);
				d2s.z = 0;
				ratio = 1- radius/sqrt(d2s.x * d2s.x + d2s.y * d2s.y);
				if(ratio >= 0) {
					/* no collision */
					return;
				}
				/* distance vector from corner to surface of sphere, (do the math) */
				VECSCALE(d2s, ratio );
				/* unproject, this is the fastest way */
				delta.y = d2s.y;
				delta.x = d2s.x* n_orig.x;
				delta.z = d2s.x* n_orig.z;
			}
		} else if(p_orig.y < abottom) {
			if(p_orig.x < awidth) {
				#ifdef RENDERVERBOSE
				printf(" /* under, we push up. */ \n");
				#endif
				delta.y = (p_orig.y + radius) -abottom;
			} else {
				struct point_XYZ d2s;
				GLDOUBLE ratio;
				#ifdef RENDERVERBOSE
				printf(" /* under side */ \n");
				#endif
				/* distance vector from corner to center of sphere*/
				d2s.x = p_orig.x - awidth;
				d2s.y = p_orig.y - abottom;
				d2s.z = 0;
				ratio = 1- radius/sqrt(d2s.x * d2s.x + d2s.y * d2s.y);
				if(ratio >= 0) {
					/* no collision */
					return;
				}

				/* distance vector from corner to surface of sphere, (do the math) */
				VECSCALE(d2s, ratio );

				/* unproject, this is the fastest way */
				delta.y = d2s.y;
				delta.x = d2s.x* n_orig.x;
				delta.z = d2s.x* n_orig.z;
			}
		} else {
			#ifdef RENDERVERBOSE
			printf(" /* side */ \n");
			#endif
			/* push to side */
			delta.x = ((p_orig.x - radius)- awidth) * n_orig.x;
			delta.z = ((p_orig.x - radius)- awidth) * n_orig.z;
		}
	}
	accumulate_disp(CollisionInfo(),delta);

	#ifdef RENDERVERBOSE
	if((delta.x != 0. || delta.y != 0. || delta.z != 0.))
		printf("COLLISION_SPH: (%f %f %f) (%f %f %f) (px=%f nx=%f nz=%f)\n",
		t_orig.x, t_orig.y, t_orig.z,
		delta.x, delta.y, delta.z,
		p_orig.x, n_orig.x, n_orig.z
		);
	#endif
}


void collide_Box (struct X3D_Box *node) {
	/*easy access, naviinfo.step unused for sphere collisions */
	struct sNaviInfo *naviinfo;
	struct point_XYZ iv = {0,0,0};
	struct point_XYZ jv = {0,0,0};
	struct point_XYZ kv = {0,0,0};
	struct point_XYZ ov = {0,0,0};
	struct point_XYZ delta;
	GLDOUBLE awidth, atop, abottom, astep, modelMatrix[16];
	ttglobal tg = gglobal();

	naviinfo = (struct sNaviInfo*)tg->Bindable.naviinfo;
	awidth = naviinfo->width; /*avatar width*/
	atop = naviinfo->width; /*top of avatar (relative to eyepoint)*/
	abottom = -naviinfo->height; /*bottom of avatar (relative to eyepoint)*/
	astep = -naviinfo->height+naviinfo->step;

	iv.x = ((node->size).c[0]);
	jv.y = ((node->size).c[1]);
	kv.z = ((node->size).c[2]);
	ov.x = -((node->size).c[0])/2; ov.y = -((node->size).c[1])/2; ov.z = -((node->size).c[2])/2;

	/* get the transformed position of the Box, and the scale-corrected radius. */
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);
	matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
	//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 
	{
		int i;
		double shapeMBBmin[3],shapeMBBmax[3];
		for(i=0;i<3;i++)
		{
			shapeMBBmin[i] = DOUBLE_MIN(-(node->size).c[i]*.5,(node->size).c[i]*.5);
			shapeMBBmax[i] = DOUBLE_MAX(-(node->size).c[i]*.5,(node->size).c[i]*.5);
		}
		if( !avatarCollisionVolumeIntersectMBB(modelMatrix, shapeMBBmin,shapeMBBmax)) return;
	}
	/* get transformed box edges and position */
	transform(&ov,&ov,modelMatrix);
	transform3x3(&iv,&iv,modelMatrix);
	transform3x3(&jv,&jv,modelMatrix);
	transform3x3(&kv,&kv,modelMatrix);

	delta = box_disp(abottom,atop,astep,awidth,ov,iv,jv,kv);
	vecscale(&delta,&delta,-1);
	accumulate_disp(CollisionInfo(),delta);

	#ifdef RENDERVERBOSE
	if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
		printf("COLLISION_BOX: (%f %f %f) (%f %f %f)\n",
		ov.x, ov.y, ov.z,
		delta.x, delta.y, delta.z
		);
	if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
		printf("iv=(%f %f %f) jv=(%f %f %f) kv=(%f %f %f)\n",
		iv.x, iv.y, iv.z,
		jv.x, jv.y, jv.z,
		kv.x, kv.y, kv.z
		);
	#endif
}


#define  CONEDIV 20

static void collisionCone_init(struct X3D_Cone *node)
{
	//we pass in a node instance, so we don't have to split up the compile_ code that generates
	// geometry - we can just copy some
	/* for debug ctri ct; */
	/* for debug struct point_XYZ a,b,n; */
	int i,count;
	/* for debug int j,k,biggestNum; */
	double h,r,inverseh,inverser;
	struct SFVec3f *pts;// = node->__botpoints;
	struct SFVec3f *pt;
	struct Multi_Vec3f botpoints;
	ppComponent_Geometry3D p = (ppComponent_Geometry3D)gglobal()->Component_Geometry3D.prv;
    	
	/*  re-using the compile_cone node->__points data which is organized into GL_TRAIANGLE_FAN (bottom) and GL_TRIANGLES (side)

		my understanding: 
		bottom: there are CONEDIV triangles arranged in a fan around a center point, with CONEDIV perimeter points 
		side: there are CONEDIV side triangles formed with the top point and perimeter points
		num triangles: CONEDIV x 2
		num points: CONEDIV perimeter + center bottom + centre top = CONEDIV+2 
		__botpoints:
			pt[0] - top point of cone
			pt[1-CONEDIV] - perimeter points
			pt[CONEDIV+1] - centre of bottom
		(there are sidepoints too, but duplicate the points in botpoints) 
	*/
	p->collisionCone.npts = CONEDIV+2;
	p->collisionCone.pts = MALLOC(void *, p->collisionCone.npts * sizeof(struct point_XYZ));
	p->collisionCone.tpts = MALLOC(void *, p->collisionCone.npts * sizeof(struct point_XYZ));

	p->collisionCone.ntris = CONEDIV *2;
	p->collisionCone.tris = MALLOC(void *, p->collisionCone.ntris * sizeof(ctri));
	count = 0;
	h = (node->height); ///2; //we inverse this to get back to normal geometry
	r = node->bottomRadius;
	inverseh = 1.0;
	inverser = 1.0;
	if( !APPROX(h,0.0) ) inverseh = 1.0/h;
	if( !APPROX(r,0.0) ) inverser = 1.0/r;


	/* ok - we copy the non-VBO code here so that Doug Sandens Cylinder Collision code
		uses the same algorithm whether running in VBO mode or not */

	/*  MALLOC memory (if possible)*/
	botpoints.p = MALLOC (struct SFVec3f *, sizeof(struct SFVec3f)*(CONEDIV+3));

	/*  generate the vertexes for the triangles; top point first. (note: top point no longer used)*/
	pt = botpoints.p;
	pt[0].c[0] = 0.0f; pt[0].c[1] = (float) h; pt[0].c[2] = 0.0f;
	for (i=1; i<=CONEDIV; i++) {
		pt[i].c[0] = (float) (r* sin(PI*2*i/(float)CONEDIV));
		pt[i].c[1] = (float) -h;
		pt[i].c[2] = (float) (r* cos(PI*2*i/(float)CONEDIV));
	}
	/*  and throw another point that is centre of bottom*/
	pt[CONEDIV+1].c[0] = 0.0f; pt[CONEDIV+1].c[1] = (float) -h; pt[CONEDIV+1].c[2] = 0.0f;

	/*  and, for the bottom, [CONEDIV] = [CONEDIV+2]; but different texture coords, so...*/
	memcpy (&pt[CONEDIV+2].c[0],&pt[CONEDIV].c[0],sizeof (struct SFVec3f));

#ifdef fwIGNORE
	/*  side triangles. Make 3 seperate points per triangle... makes sendArraysToGPU with normals*/
	/*  easier to handle.*/
	/*  rearrange bottom points into this array; top, bottom, left.*/
	spt = sidepoints.p;
	for (i=0; i<CONEDIV; i++) {
		/*  top point*/
		spt[i*3].c[0] = 0.0f; spt[i*3].c[1] = (float) h; spt[i*3].c[2] = 0.0f;
		/*  left point*/
		memcpy (&spt[i*3+1].c[0],&pt[i+1].c[0],sizeof (struct SFVec3f));
		/* right point*/
		memcpy (&spt[i*3+2].c[0],&pt[i+2].c[0],sizeof (struct SFVec3f));
	}

	/*  wrap bottom point around once again... ie, final right point = initial left point*/
	memcpy (&spt[(CONEDIV-1)*3+2].c[0],&pt[1].c[0],sizeof (struct SFVec3f));
#endif
	


	pts = botpoints.p;
	for(i=0;i<(CONEDIV+2);i++)
	{
		/* points */
		p->collisionCone.pts[i].x = pts[i].c[0]*inverser;
		p->collisionCone.pts[i].y = pts[i].c[1]*inverseh;
		p->collisionCone.pts[i].z = pts[i].c[2]*inverser;
	}
	for(i=0;i<CONEDIV;i++)
	{
		/* side triangles */
		p->collisionCone.tris[count][0] = 0;   /* top point */
		p->collisionCone.tris[count][1] = i +1;
		p->collisionCone.tris[count][2] = i > (CONEDIV-2)? 1 : i+2; /*wrap-around, normally i+2 */
		count ++;
	}
	for(i=0;i<CONEDIV;i++)
	{
		/* bottom triangles */
		p->collisionCone.tris[count][0] = CONEDIV+1; /* bottom center point */
		p->collisionCone.tris[count][1] = i +1;
		p->collisionCone.tris[count][2] = i > (CONEDIV-2)?  1 : i+2; 
		count ++;
	}


	/* count should == num triangles 
	debug check on indexing - biggestNum should == npts -1 
	biggestNum = 0;
	for(i=0;i<collisionCone.ntris;i++)
		for(j=0;j<3;j++)
			biggestNum = max(biggestNum,collisionCone.tris[i][j]);
	*/
	/* MBB */
	for(i=0;i<3;i+=2)
	{
		p->collisionCone.smin[i] = -1.0; //r;
		p->collisionCone.smax[i] =  1.0; //r;
	}
	p->collisionCone.smin[1] = -1.0; //-h;
	p->collisionCone.smax[1] =  1.0; //h;
	
	/* ok - we copy the non-VBO code here so that Doug Sandens Cylinder Collision code
		uses the same algorithm whether running in VBO mode or not */
	FREE_IF_NZ(botpoints.p);
}
#ifdef DEBUG_COLLISIONCONE
int collisionCone_render(double r, double h)
{
	/* I needed to verify the collision mesh was good, and it uses triangles, so I drew it the triangle way and it looked good 
	   to see it draw, you need to turn on collision and get close to the mesh object - then it will initialize and start drawing it.
	*/
	int i,j;
	for(i =0; i < collisionCone.ntris; i++)  
	{ 
		struct point_XYZ pts[3]; //,a,b,n;
		pts[0] = collisionCone.pts[collisionCone.tris[i][0]];
		pts[1] = collisionCone.pts[collisionCone.tris[i][1]];
		pts[2] = collisionCone.pts[collisionCone.tris[i][2]];
		glBegin(GL_TRIANGLES);
		for(j=0;j<3;j++)
			FW_GL_VERTEX3D(pts[j].x*r,pts[j].y*h,pts[j].z*r);
		glEnd();
	}
	return 0;
}
#endif
void collide_Cone (struct X3D_Cone *node) {
	/*easy access, naviinfo.step unused for sphere collisions */
	struct sNaviInfo *naviinfo;
	GLDOUBLE awidth, atop, abottom, astep, scale, modelMatrix[16];
	float h,r;
	struct point_XYZ iv = {0,0,0};
	struct point_XYZ jv = {0,0,0};
	struct point_XYZ t_orig = {0,0,0};
	struct point_XYZ delta;
	ttglobal tg = gglobal();
	ppComponent_Geometry3D p = (ppComponent_Geometry3D)tg->Component_Geometry3D.prv;
	naviinfo = (struct sNaviInfo*)tg->Bindable.naviinfo;

	awidth = naviinfo->width; /*avatar width*/
	atop = naviinfo->width; /*top of avatar (relative to eyepoint)*/
	abottom = -naviinfo->height; /*bottom of avatar (relative to eyepoint)*/
	astep = -naviinfo->height+naviinfo->step;

	h = (node->height) /2;
	r = (node->bottomRadius) ;

	scale = 0.0; /* FIXME: won''t work for non-uniform scales. */

	/* is this node initialized? if not, get outta here and do this later */
	if (node->__coneVBO == 0) return;

	iv.y = h; jv.y = -h;

	/* get the transformed position of the Sphere, and the scale-corrected radius. */
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

	//matmultiply(modelMatrix,FallInfo.avatar2collision,modelMatrix); 
	if(FallInfo()->walking)
	{
		/* mesh method */
		int i;
		double disp;
		struct point_XYZ n;
		struct point_XYZ a,b, dispv, maxdispv = {0,0,0};
		double maxdisp = 0;
		struct point_XYZ radscale;

		if(!p->collisionCone.npts) collisionCone_init(node);
#ifdef DEBUG_COLLISIONCONE
		collisionCone_render(5.0,5.0);
#endif
		radscale.x = radscale.z = node->bottomRadius;
		radscale.y = node->height;
		scale_to_matrix (modelMatrix, &radscale);
		matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
		//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 
		if( !avatarCollisionVolumeIntersectMBB(modelMatrix, p->collisionCone.smin,p->collisionCone.smax)) return;


		for(i=0;i<p->collisionCone.npts;i++)
			transform(&p->collisionCone.tpts[i],&p->collisionCone.pts[i],modelMatrix);
		for(i = 0; i < p->collisionCone.ntris; i++) 
		{
			/*only clip faces "facing" origin */
			//if(vecdot(&n[ci],&middle) < 0.) {
			{
				struct point_XYZ pts[3];
				pts[0] = p->collisionCone.tpts[p->collisionCone.tris[i][0]];
				pts[1] = p->collisionCone.tpts[p->collisionCone.tris[i][1]];
				pts[2] = p->collisionCone.tpts[p->collisionCone.tris[i][2]];
				/* compute normal - could compute once in shapespace then transform */
				VECDIFF(pts[1],pts[0],a);
				VECDIFF(pts[2],pts[1],b); /* or [2] [0] direction not sensitive for some functions */
				veccross(&n,a,b); /* 6 multiplies */
				vecnormal(&n,&n); 
				dispv = get_poly_disp_2(pts,3,n);
				disp = vecdot(&dispv,&dispv);
				if( (disp > FLOAT_TOLERANCE) && (disp > maxdisp) ){
					maxdisp = disp;
					maxdispv = dispv;
				}
			}
		}
		delta = maxdispv;
	}else{
		/* values for rapid test */
		matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
		//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 

		t_orig.x = modelMatrix[12];
		t_orig.y = modelMatrix[13];
		t_orig.z = modelMatrix[14];
		scale = pow(det3x3(modelMatrix),1./3.);

		if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;

		/* get transformed box edges and position */
		transform(&iv,&iv,modelMatrix);
		transform(&jv,&jv,modelMatrix);

		delta = cone_disp(abottom,atop,astep,awidth,jv,iv,scale*r);
	}
	vecscale(&delta,&delta,-1);

	accumulate_disp(CollisionInfo(),delta);

	#ifdef RENDERVERBOSE
		if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
			printf("COLLISION_CON: (%f %f %f) (%f %f %f)\n",
			iv.x, iv.y, iv.z,
			delta.x, delta.y, delta.z
			);
		if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
			printf("iv=(%f %f %f) jv=(%f %f %f) bR=%f\n",
			iv.x, iv.y, iv.z,
			jv.x, jv.y, jv.z,
			scale*r
			);
	#endif
}

static void collisionCylinder_init(struct X3D_Cylinder *node)
{
	/* for debug ctri ct; */
	/* for debug struct point_XYZ a,b,n; */
	int i, tcount, qcount;
	/* for debug - int j,k,biggestNum; */
	double h,r,inverseh,inverser;
	struct SFVec3f *pts;// = node->__botpoints;
	ppComponent_Geometry3D p = (ppComponent_Geometry3D)gglobal()->Component_Geometry3D.prv;
	
	/*  re-using the compile_cylinder node->__points data which is organized into GL_TRAIANGLE_FAN (bottom and top)
		and GL_QUADS (side)

		my understanding: 
		bottom and top: there are CYLDIV triangles arranged in a fan around a center point, with CYLDIV perimeter points 
		side: there are CYLDIV side quads formed with the top and bottom perimeter points
		num triangles: CYLDIV x 2
		num quads: CYLDIV
		num points: CYLDIV * 2 + center bottom + centre top = CYLDIV*2 +2 
		__points:
			pt[0- CYLDIV*2-1 + 2] - perimeter points including wrap-around points ordered as follows:
			    +h,-h,+h,-h .. vertical pair order, with 2 extra for easy wrap-around indexing
			pt[CYLDIV*2+2] - top center point of cylinder
			pt[CYLDIV*2+3] - centre of bottom
	*/
	p->collisionCylinder.npts = CYLDIV*2+2+2;
	p->collisionCylinder.pts = MALLOC(void *, p->collisionCylinder.npts * sizeof(struct point_XYZ));
	p->collisionCylinder.tpts = MALLOC(void *, p->collisionCylinder.npts * sizeof(struct point_XYZ));

	p->collisionCylinder.ntris = CYLDIV *2;
	p->collisionCylinder.tris = MALLOC(void *, p->collisionCylinder.ntris * sizeof(ctri));
	p->collisionCylinder.nquads = CYLDIV;
	p->collisionCylinder.quads = MALLOC(void *, p->collisionCylinder.nquads * sizeof(cquad));

	tcount = 0;
	qcount = 0;
	h = node->height;
	r = node->radius;
	inverseh = inverser = 1.0;
	if(!APPROX(h,0.0)) inverseh = 1.0/h;
	if(!APPROX(r,0.0)) inverser = 1.0/r;
	{
		float a1;
		/* ok - we copy the non-VBO code here so that Doug Sandens Cylinder Collision code
		   uses the same algorithm whether running in VBO mode or not */
		pts = MALLOC(struct SFVec3f *,sizeof(struct SFVec3f)*2*(CYLDIV+4));
	
		/*  now, create the vertices; this is a quad, so each face = 4 points*/
		for (i=0; i<CYLDIV; i++) {
			a1 = (float) (PI*2*i)/(float)CYLDIV;
			pts[i*2+0].c[0] = (float) (r* sin(a1));
			pts[i*2+0].c[1] = (float) h;
			pts[i*2+0].c[2] = (float) (r* cos(a1));
			pts[i*2+1].c[0] = (float) (r* sin(a1));
			pts[i*2+1].c[1] = (float) -h;
			pts[i*2+1].c[2] = (float) (r* cos(a1));
		}
	
		/*  wrap the points around*/
		memcpy (&pts[CYLDIV*2].c[0],&pts[0].c[0],sizeof(struct SFVec3f)*2);
	
		/*  center points of top and bottom*/
		pts[CYLDIV*2+2].c[0] = 0.0f; pts[CYLDIV*2+2].c[1] = (float) h; pts[CYLDIV*2+2].c[2] = 0.0f;
		pts[CYLDIV*2+3].c[0] = 0.0f; pts[CYLDIV*2+3].c[1] = (float)-h; pts[CYLDIV*2+3].c[2] = 0.0f;
	}
	for(i=0;i<p->collisionCylinder.npts;i++)
	{
		/* points */
		p->collisionCylinder.pts[i].x = pts[i].c[0]*inverser;
		p->collisionCylinder.pts[i].y = pts[i].c[1]*inverseh;
		p->collisionCylinder.pts[i].z = pts[i].c[2]*inverser;
	}
	for(i=0;i<CYLDIV;i++)
	{
		/* side quads */
		p->collisionCylinder.quads[qcount][0] = i*2;   /* top point */
		p->collisionCylinder.quads[qcount][1] = i*2 +1;
		p->collisionCylinder.quads[qcount][2] = i*2 +3; /*wrap-around, normally i+2 */
		p->collisionCylinder.quads[qcount][3] = i*2 +2;
		qcount ++;
	}
	//pt[CYLDIV*2+2].c[0] = 0.0; pt[CYLDIV*2+2].c[1] = (float) h; pt[CYLDIV*2+2].c[2] = 0.0;
	//pt[CYLDIV*2+3].c[0] = 0.0; pt[CYLDIV*2+3].c[1] = (float)-h; pt[CYLDIV*2+3].c[2] = 0.0;

	for(i=0;i<CYLDIV;i++)
	{
		/* bottom triangles */
		p->collisionCylinder.tris[tcount][0] = CYLDIV*2+3; /* bottom center point */
		p->collisionCylinder.tris[tcount][1] = i*2 +1;
		p->collisionCylinder.tris[tcount][2] = (i+1)*2 +1; 
		tcount ++;
	}
	for(i=0;i<CYLDIV;i++)
	{
		/* top triangles */
		p->collisionCylinder.tris[tcount][0] = CYLDIV*2+2; /* top center point */
		p->collisionCylinder.tris[tcount][1] = i*2;
		p->collisionCylinder.tris[tcount][2] = (i+1)*2; 
		tcount ++;
	}

	/* count should == num triangles 
	debug check on indexing - biggestNum should == npts -1 
	biggestNum = 0;
	for(i=0;i<collisionCylinder.ntris;i++)
		for(j=0;j<3;j++)
			biggestNum = max(biggestNum,collisionCylinder.tris[i][j]);
	*/
	/* MBB */
	for(i=0;i<3;i+=2)
	{
		p->collisionCylinder.smin[i] = -1.0; //r;
		p->collisionCylinder.smax[i] =  1.0; //r;
	}
	p->collisionCylinder.smin[1] = -1.0; //-h/2;
	p->collisionCylinder.smax[1] =  1.0; //h/2;

	FREE_IF_NZ(pts);
}

#ifdef DEBUGGING_CODE
DEBUGGING_CODEint collisionCylinder_render(double r, double h)
DEBUGGING_CODE{
DEBUGGING_CODE	/* I needed to verify the collision mesh was good, and it uses triangles, so I drew it the triangle way and it looked good 
DEBUGGING_CODE	   to see it draw, you need to turn on collision and get close to the mesh object - then it will initialize and start drawing it.
DEBUGGING_CODE	*/
DEBUGGING_CODE	int i,j;
DEBUGGING_CODE	for(i =0; i < collisionCylinder.ntris; i++)  
DEBUGGING_CODE	{ 
DEBUGGING_CODE		struct point_XYZ pts[3]; //,a,b,n;
DEBUGGING_CODE		pts[0] = collisionCylinder.pts[collisionCylinder.tris[i][0]];
DEBUGGING_CODE		pts[1] = collisionCylinder.pts[collisionCylinder.tris[i][1]];
DEBUGGING_CODE		pts[2] = collisionCylinder.pts[collisionCylinder.tris[i][2]];
DEBUGGING_CODE		FW_GL_BEGIN(GL_TRIANGLES);
DEBUGGING_CODE		for(j=0;j<3;j++)
DEBUGGING_CODE			FW_GL_VERTEX3D(pts[j].x*r,pts[j].y*h,pts[j].z*r);
DEBUGGING_CODE		FW_GL_END();
DEBUGGING_CODE	}
DEBUGGING_CODE	for(i =0; i < collisionCylinder.nquads; i++)  
DEBUGGING_CODE	{ 
DEBUGGING_CODE		struct point_XYZ pts[4]; //,a,b,n;
DEBUGGING_CODE		pts[0] = collisionCylinder.pts[collisionCylinder.quads[i][0]];
DEBUGGING_CODE		pts[1] = collisionCylinder.pts[collisionCylinder.quads[i][1]];
DEBUGGING_CODE		pts[2] = collisionCylinder.pts[collisionCylinder.quads[i][2]];
DEBUGGING_CODE		pts[3] = collisionCylinder.pts[collisionCylinder.quads[i][3]];
DEBUGGING_CODE		FW_GL_BEGIN(GL_QUADS);
DEBUGGING_CODE		for(j=0;j<4;j++)
DEBUGGING_CODE			FW_GL_VERTEX3D(pts[j].x*r,pts[j].y*h,pts[j].z*r);
DEBUGGING_CODE		FW_GL_END();
DEBUGGING_CODE	}
DEBUGGING_CODE	return 0;
DEBUGGING_CODE}
#endif


void collide_Cylinder (struct X3D_Cylinder *node) {
	/*easy access, naviinfo.step unused for sphere collisions */
	struct sNaviInfo *naviinfo;
	GLDOUBLE awidth,atop,abottom,astep,scale,modelMatrix[16];
	float h,r;
	struct point_XYZ iv = {0,0,0};
	struct point_XYZ jv = {0,0,0};
	struct point_XYZ t_orig = {0,0,0};
	struct point_XYZ delta;
	ttglobal tg = gglobal();
	ppComponent_Geometry3D p = (ppComponent_Geometry3D)tg->Component_Geometry3D.prv;
	naviinfo = (struct sNaviInfo*)tg->Bindable.naviinfo;

	awidth = naviinfo->width; /*avatar width*/
	atop = naviinfo->width; /*top of avatar (relative to eyepoint)*/
	abottom = -naviinfo->height; /*bottom of avatar (relative to eyepoint)*/
	astep = -naviinfo->height+naviinfo->step;

	h = (node->height)/2;
	r = (node->radius);


	scale=0.0; /* FIXME: won''t work for non-uniform scales. */

	iv.y = h;
	jv.y = -h;

	/* get the transformed position of the Sphere, and the scale-corrected radius. */
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

	//matmultiply(modelMatrix,FallInfo.avatar2collision,modelMatrix); 
	if(FallInfo()->walking)
	{
		/* mesh method */
		int i;
		double disp;
		struct point_XYZ n;
		struct point_XYZ a,b, dispv, radscale, maxdispv = {0,0,0};
		double maxdisp = 0;

		if(!p->collisionCylinder.npts) 
			collisionCylinder_init(node);
		radscale.x = radscale.z = node->radius;
		radscale.y = node->height;
		scale_to_matrix (modelMatrix, &radscale);
		matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
		//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 
		if( !avatarCollisionVolumeIntersectMBB(modelMatrix, p->collisionCylinder.smin,p->collisionCylinder.smax)) return;

		for(i=0;i<p->collisionCylinder.npts;i++)
			transform(&p->collisionCylinder.tpts[i],&p->collisionCylinder.pts[i],modelMatrix);

		for(i = 0; i < p->collisionCylinder.ntris; i++) 
		{
			struct point_XYZ pts[3];
			pts[0] = p->collisionCylinder.tpts[p->collisionCylinder.tris[i][0]];
			pts[1] = p->collisionCylinder.tpts[p->collisionCylinder.tris[i][1]];
			pts[2] = p->collisionCylinder.tpts[p->collisionCylinder.tris[i][2]];
			/* compute normal - could compute once in shapespace then transform */
			VECDIFF(pts[1],pts[0],a);
			VECDIFF(pts[2],pts[1],b); /* or [2] [0] direction not sensitive for some functions */
			veccross(&n,a,b); /* 6 multiplies */
			vecnormal(&n,&n); 
			dispv = get_poly_disp_2(pts,3,n);
			disp = vecdot(&dispv,&dispv);
			if( (disp > FLOAT_TOLERANCE) && (disp > maxdisp) ){
				maxdisp = disp;
				maxdispv = dispv;
			}
		}
		for(i = 0; i < p->collisionCylinder.nquads; i++) 
		{
			struct point_XYZ pts[4];
			pts[0] = p->collisionCylinder.tpts[p->collisionCylinder.quads[i][0]];
			pts[1] = p->collisionCylinder.tpts[p->collisionCylinder.quads[i][1]];
			pts[2] = p->collisionCylinder.tpts[p->collisionCylinder.quads[i][2]];
			pts[3] = p->collisionCylinder.tpts[p->collisionCylinder.quads[i][3]];
			/* compute normal - could compute once in shapespace then transform */
			VECDIFF(pts[1],pts[0],a);
			VECDIFF(pts[2],pts[1],b); /* or [2] [0] direction not sensitive for some functions */
			veccross(&n,a,b); /* 6 multiplies */
			vecnormal(&n,&n); 
			dispv = get_poly_disp_2(pts,4,n);
			disp = vecdot(&dispv,&dispv);
			if( (disp > FLOAT_TOLERANCE) && (disp > maxdisp) ){
				maxdisp = disp;
				maxdispv = dispv;
			}
		}
		delta = maxdispv;
	}else{
		/* values for rapid test */
		matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
		//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 
		t_orig.x = modelMatrix[12];
		t_orig.y = modelMatrix[13];
		t_orig.z = modelMatrix[14];
		scale = pow(det3x3(modelMatrix),1./3.);
		if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;
		/* get transformed box edges and position */
		transform(&iv,&iv,modelMatrix);
		transform(&jv,&jv,modelMatrix);
		delta = cylinder_disp(abottom,atop,astep,awidth,jv,iv,scale*r);
	}
	vecscale(&delta,&delta,-1);
	accumulate_disp(CollisionInfo(),delta);
	#ifdef RENDERVERBOSE
		if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
			printf("COLLISION_CYL: (%f %f %f) (%f %f %f)\n",
			iv.x, iv.y, iv.z,
			delta.x, delta.y, delta.z
			);
		if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
			printf("iv=(%f %f %f) jv=(%f %f %f) bR=%f\n",
			iv.x, iv.y, iv.z,
			jv.x, jv.y, jv.z,
			scale*r
			);
	#endif
}

void collide_Extrusion (struct X3D_Extrusion *node) {
	GLDOUBLE modelMatrix[16];
	struct point_XYZ delta = {0,0,0};
	#ifdef RENDERVERBOSE
	struct point_XYZ t_orig = {0,0,0};
	#endif
	struct X3D_PolyRep pr;
	prflags flags = 0;
	int change = 0;

	/* JAS - first pass, intern is probably zero */
	if (node->_intern == NULL) return;
	/* JAS - no triangles in this text structure */
	if (node->_intern->ntri == 0) return;

	/*save changed state.*/
	if(node->_intern) change = node->_intern->irep_change;
	COMPILE_POLY_IF_REQUIRED(NULL, NULL, NULL, NULL, NULL)
	if(node->_intern) node->_intern->irep_change = change;
	/*restore changes state, invalidates compile_polyrep work done, so it can be done
	correclty in the RENDER pass */

	if(!node->solid) {
		flags = flags | PR_DOUBLESIDED;
	}
	/*	printf("_PolyRep = %d\n",node->_intern);*/
	pr = *(node->_intern);
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

	matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
	//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 

	#ifdef RENDERVERBOSE
	t_orig.x = modelMatrix[12];
	t_orig.y = modelMatrix[13];
	t_orig.z = modelMatrix[14];
	#endif

	if(!avatarCollisionVolumeIntersectMBBf(modelMatrix, pr.minVals, pr.maxVals))return;
	delta = polyrep_disp2(pr,modelMatrix,flags); 
	vecscale(&delta,&delta,-1);
	accumulate_disp(CollisionInfo(),delta);

#ifdef RENDERVERBOSE
	if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
/*		printmatrix(modelMatrix);*/
	fprintf(stderr,"COLLISION_EXT: (%f %f %f) (%f %f %f)\n",
		t_orig.x, t_orig.y, t_orig.z,
		delta.x, delta.y, delta.z
		);
	}
#endif
}



void rendray_Sphere (struct X3D_Sphere *node) {
	struct point_XYZ t_r1,t_r2;
	float r = node->radius;
	/* Center is at zero. t_r1 to t_r2 and t_r1 to zero are the vecs */

	float tr1sq;// = (float) VECSQ(t_r1);
	struct point_XYZ dr2r1;
	float dlen;
	float a,b,c,disc;
	//VECCOPY(t_r1,tg->RenderFuncs.t_r1);
	//VECCOPY(t_r2,tg->RenderFuncs.t_r2);
	get_current_ray(&t_r1, &t_r2);

	tr1sq = (float) VECSQ(t_r1);

	VECDIFF(t_r2,t_r1,dr2r1);
	dlen = (float) VECSQ(dr2r1);

	a = dlen; /* tr1sq - 2*tr1tr2 + tr2sq; */
	b = 2.0f*((float)VECPT(dr2r1, t_r1));
	c = tr1sq - r*r;

	disc = b*b - 4*a*c; /* The discriminant */

	if(disc > 0) { /* HITS */
		float q ;
		float sol1 ;
		float sol2 ;
		float cx,cy,cz;
		q = (float) sqrt(disc);
		/* q = (-b+(b>0)?q:-q)/2; */
		sol1 = (-b+q)/(2*a);
		sol2 = (-b-q)/(2*a);
		/*
		printf("SPHSOL0: (%f %f %f) (%f %f %f)\n",
				t_r1.x, t_r1.y, t_r1.z, t_r2.x, t_r2.y, t_r2.z);
		printf("SPHSOL: (%f %f %f) (%f) (%f %f) (%f) (%f %f)\n",
				tr1sq, tr2sq, tr1tr2, a, b, c, und, sol1, sol2);
		*/ 
		cx = (float) MRATX(sol1);
		cy = (float) MRATY(sol1);
		cz = (float) MRATZ(sol1);
		rayhit(sol1, cx,cy,cz, cx/r,cy/r,cz/r, -1,-1, "sphere 0");
		cx = (float) MRATX(sol2);
		cy = (float) MRATY(sol2);
		cz = (float) MRATZ(sol2);
		rayhit(sol2, cx,cy,cz, cx/r,cy/r,cz/r, -1,-1, "sphere 1");
	}

}


void rendray_Box (struct X3D_Box *node) {
	float x,y,z;
	struct point_XYZ t_r1,t_r2;

	//VECCOPY(t_r1,tg->RenderFuncs.t_r1);
	//VECCOPY(t_r2,tg->RenderFuncs.t_r2);

	get_current_ray(&t_r1, &t_r2);

	x = ((node->size).c[0])/2;
	y = ((node->size).c[1])/2;
	z = ((node->size).c[2])/2;
	/* 1. x=const-plane faces? */
	if(!XEQ) {
		float xrat0 = (float) XRAT(x);
		float xrat1 = (float) XRAT(-x);
		#ifdef RENDERVERBOSE 
		printf("!XEQ: %f %f\n",xrat0,xrat1);
		#endif

		if(TRAT(xrat0)) {
			float cy = (float) MRATY(xrat0);
			#ifdef RENDERVERBOSE 
			printf("TRok: %f\n",cy);
			#endif

			if(cy >= -y && cy < y) {
				float cz = (float) MRATZ(xrat0);
				#ifdef RENDERVERBOSE 
				printf("cyok: %f\n",cz);
				#endif

				if(cz >= -z && cz < z) {
					#ifdef RENDERVERBOSE 
					printf("czok:\n");
					#endif

					rayhit(xrat0, x,cy,cz, 1,0,0, -1,-1, "cube x0");
				}
			}
		}
		if(TRAT(xrat1)) {
			float cy = (float) MRATY(xrat1);
			if(cy >= -y && cy < y) {
				float cz = (float) MRATZ(xrat1);
				if(cz >= -z && cz < z) {
					rayhit(xrat1, -x,cy,cz, -1,0,0, -1,-1, "cube x1");
				}
			}
		}
	}
	if(!YEQ) {
		float yrat0 = (float) YRAT(y);
		float yrat1 = (float) YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = (float) MRATX(yrat0);
			if(cx >= -x && cx < x) {
				float cz = (float) MRATZ(yrat0);
				if(cz >= -z && cz < z) {
					rayhit(yrat0, cx,y,cz, 0,1,0, -1,-1, "cube y0");
				}
			}
		}
		if(TRAT(yrat1)) {
			float cx = (float) MRATX(yrat1);
			if(cx >= -x && cx < x) {
				float cz = (float) MRATZ(yrat1);
				if(cz >= -z && cz < z) {
					rayhit(yrat1, cx,-y,cz, 0,-1,0, -1,-1, "cube y1");
				}
			}
		}
	}
	if(!ZEQ) {
		float zrat0 = (float) ZRAT(z);
		float zrat1 = (float) ZRAT(-z);
		if(TRAT(zrat0)) {
			float cx = (float) MRATX(zrat0);
			if(cx >= -x && cx < x) {
				float cy = (float) MRATY(zrat0);
				if(cy >= -y && cy < y) {
					rayhit(zrat0, cx,cy,z, 0,0,1, -1,-1,"cube z0");
				}
			}
		}
		if(TRAT(zrat1)) {
			float cx = (float) MRATX(zrat1);
			if(cx >= -x && cx < x) {
				float cy = (float) MRATY(zrat1);
				if(cy >= -y && cy < y) {
					rayhit(zrat1, cx,cy,-z, 0,0,-1,  -1,-1,"cube z1");
				}
			}
		}
	}
}


void rendray_Cylinder (struct X3D_Cylinder *node) {

	float h,r,y;
	struct point_XYZ t_r1,t_r2;
	//VECCOPY(t_r1,tg->RenderFuncs.t_r1);
	//VECCOPY(t_r2,tg->RenderFuncs.t_r2);
	get_current_ray(&t_r1, &t_r2);

	h = (node->height) /*cget*//2; /* pos and neg dir. */
	r = (node->radius) /*cget*/;
	y = h;
	/* Caps */
	if(!YEQ) {
		float yrat0 = (float) YRAT(y);
		float yrat1 = (float) YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = (float) MRATX(yrat0);
			float cz = (float) MRATZ(yrat0);
			if(r*r > cx*cx+cz*cz) {
				rayhit(yrat0, cx,y,cz, 0,1,0, -1,-1, "cylcap 0");
			}
		}
		if(TRAT(yrat1)) {
			float cx = (float) MRATX(yrat1);
			float cz = (float) MRATZ(yrat1);
			if(r*r > cx*cx+cz*cz) {
				rayhit(yrat1, cx,-y,cz, 0,-1,0, -1,-1, "cylcap 1");
			}
		}
	}
	/* Body -- do same as for sphere, except no y axis in distance */
	//if((!XEQ) && (!ZEQ)) //can't pick with ortho on initial load
	//can pick with ortho on initial load:  if(!(XEQ && ZEQ))
	if(!(XEQ && ZEQ)){   
		float dx = (float)(t_r2.x-t_r1.x); 
		float dz = (float)(t_r2.z-t_r1.z);
		float a = (float)(dx*dx + dz*dz);
		float b = (float) (2*(dx * t_r1.x + dz * t_r1.z));
		float c = (float) (t_r1.x * t_r1.x + t_r1.z * t_r1.z - r*r);
		float und;
		b /= a; c /= a;
		und = b*b - 4*c;
		if(und > 0) { /* HITS the infinite cylinder */
			float sol1 = (-b+(float) sqrt(und))/2;
			float sol2 = (-b-(float) sqrt(und))/2;
			float cy,cx,cz;
			cy = (float) MRATY(sol1);
			if(cy > -h && cy < h) {
				cx = (float) MRATX(sol1);
				cz = (float) MRATZ(sol1);
				rayhit(sol1, cx,cy,cz, cx/r,0,cz/r, -1,-1, "cylside 1");
			}
			cy = (float) MRATY(sol2);
			if(cy > -h && cy < h) {
				cx = (float) MRATX(sol2);
				cz = (float) MRATZ(sol2);
				rayhit(sol2, cx,cy,cz, cx/r,0,cz/r, -1,-1, "cylside 2");
			}
		}
	}
}

void rendray_Cone (struct X3D_Cone *node) {
	float h,y,r,dx,dy,dz,a,b,c,tmp,und;
	struct point_XYZ t_r1,t_r2;

	// VECCOPY(t_r1,tg->RenderFuncs.t_r1);
	// VECCOPY(t_r2,tg->RenderFuncs.t_r2);

	get_current_ray(&t_r1, &t_r2);

	h = (node->height) /*cget*//2; /* pos and neg dir. */
	y = h;
	r = (node->bottomRadius) /*cget*/;
	dx = (float) (t_r2.x-t_r1.x); 
	dz = (float) (t_r2.z-t_r1.z);
	dy = (float) (t_r2.y-t_r1.y);
	a = dx*dx + dz*dz - (r*r*dy*dy/(2*h*2*h));
	b = (float) (2*(dx*t_r1.x + dz*t_r1.z) +
		2*r*r*dy/(2*h)*(0.5-t_r1.y/(2*h)));
	tmp = (float)((0.5-t_r1.y/(2*h)));
	c = (float)(t_r1.x * t_r1.x + t_r1.z * t_r1.z)
		- r*r*tmp*tmp;
	
	b /= a; c /= a;
	und = b*b - 4*c;
	/*
	printf("CONSOL0: (%f %f %f) (%f %f %f)\n",
		t_r1.x, t_r1.y, t_r1.z, t_r2.x, t_r2.y, t_r2.z);
	printf("CONSOL: (%f %f %f) (%f) (%f %f) (%f)\n",
		dx, dy, dz, a, b, c, und);
	*/
	if(und > 0) { /* HITS the infinite cylinder */
		float sol1 = (-b+(float)sqrt(und))/2;
		float sol2 = (-b-(float)sqrt(und))/2;
		float cy,cx,cz;
		cy = (float)MRATY(sol1);
		if(cy > -h && cy < h) {
			cx = (float)MRATX(sol1);
			cz = (float)MRATZ(sol1);
			/* XXX Normal */
			rayhit(sol1, cx,cy,cz, cx/r,0,cz/r, -1,-1, "conside 1");
		}
		cy = (float) MRATY(sol2);
		if(cy > -h && cy < h) {
			cx = (float) MRATX(sol2);
			cz = (float) MRATZ(sol2);
			rayhit(sol2, cx,cy,cz, cx/r,0,cz/r, -1,-1, "conside 2");
		}
		/*
		printf("CONSOLV: (%f %f) (%f %f)\n", sol1, sol2,cy0,cy);
		*/
	}
	if(!YEQ) {
		float yrat0 = (float) YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = (float) MRATX(yrat0);
			float cz = (float) MRATZ(yrat0);
			if(r*r > cx*cx + cz*cz) {
				rayhit(yrat0, cx, -y, cz, 0, -1, 0, -1, -1, "conbot");
			}
		}
	}
}

// TEAPOT sept 2016
// http://www.web3d.org/x3d/content/examples/Basic/ExperimentalBinaryCompression/Teapot.html
// https://www.sjbaker.org/wiki/index.php?title=The_History_of_The_Teapot#Images_of_the_Complete_Dataset
// tests teapot-no_shaders.wrl

static struct X3D_IndexedFaceSet *teapotifs = NULL;
static struct X3D_Coordinate *teapot_coord;
static int teapot_coordindex_p [] = {1,2,3,4,-1,5,1,4,6,-1,7,5,6,8,-1,9,7,8,10,-1,11,9,10,12,-1,4,3,13,14,-1,6,4,14,15,-1,8,6,15,16,-1,10,8,16,17,-1,12,10,17,18,-1,14,13,19,20,-1,15,14,20,21,-1,16,15,21,22,-1,17,16,22,23,-1,18,17,23,24,-1,20,19,25,26,-1,21,20,26,27,-1,22,21,27,28,-1,23,22,28,29,-1,24,23,29,30,-1,26,25,31,32,-1,27,26,32,33,-1,28,27,33,34,-1,29,28,34,35,-1,30,29,35,36,-1,32,31,37,38,-1,33,32,38,39,-1,34,33,39,40,-1,35,34,40,41,-1,36,35,41,42,-1,38,37,43,44,-1,39,38,44,45,-1,40,39,45,46,-1,41,40,46,47,-1,42,41,47,48,-1,44,43,49,50,-1,45,44,50,51,-1,46,45,51,52,-1,47,46,52,53,-1,48,47,53,54,-1,50,49,55,56,-1,51,50,56,57,-1,52,51,57,58,-1,53,52,58,59,-1,54,53,59,60,-1,56,55,61,62,-1,57,56,62,63,-1,58,57,63,64,-1,59,58,64,65,-1,60,59,65,66,-1,62,61,67,68,-1,63,62,68,69,-1,64,63,69,70,-1,65,64,70,71,-1,66,65,71,72,-1,68,67,2,1,-1,69,68,1,5,-1,70,69,5,7,-1,71,70,7,9,-1,72,71,9,11,-1,73,11,12,74,-1,75,73,74,76,-1,77,75,76,78,-1,79,77,78,80,-1,81,79,80,82,-1,83,81,82,84,-1,74,12,18,85,-1,76,74,85,86,-1,78,76,86,87,-1,80,78,87,88,-1,82,80,88,89,-1,84,82,89,90,-1,85,18,24,91,-1,86,85,91,92,-1,87,86,92,93,-1,88,87,93,94,-1,89,88,94,95,-1,90,89,95,96,-1,91,24,30,97,-1,92,91,97,98,-1,93,92,98,99,-1,94,93,99,100,-1,95,94,100,101,-1,96,95,101,102,-1,97,30,36,103,-1,98,97,103,104,-1,99,98,104,105,-1,100,99,105,106,-1,101,100,106,107,-1,102,101,107,108,-1,103,36,42,109,-1,104,103,109,110,-1,105,104,110,111,-1,106,105,111,112,-1,107,106,112,113,-1,108,107,113,114,-1,109,42,48,115,-1,110,109,115,116,-1,111,110,116,117,-1,112,111,117,118,-1,113,112,118,119,-1,114,113,119,120,-1,115,48,54,121,-1,116,115,121,122,-1,117,116,122,123,-1,118,117,123,124,-1,119,118,124,125,-1,120,119,125,126,-1,121,54,60,127,-1,122,121,127,128,-1,123,122,128,129,-1,124,123,129,130,-1,125,124,130,131,-1,126,125,131,132,-1,127,60,66,133,-1,128,127,133,134,-1,129,128,134,135,-1,130,129,135,136,-1,131,130,136,137,-1,132,131,137,138,-1,133,66,72,139,-1,134,133,139,140,-1,135,134,140,141,-1,136,135,141,142,-1,137,136,142,143,-1,138,137,143,144,-1,139,72,11,73,-1,140,139,73,75,-1,141,140,75,77,-1,142,141,77,79,-1,143,142,79,81,-1,144,143,81,83,-1,120,126,132,-1,120,132,138,-1,120,138,144,-1,120,144,83,-1,120,83,84,-1,120,84,90,-1,120,90,96,-1,120,96,102,-1,120,102,108,-1,120,108,114,-1,2,154,145,3,-1,3,145,146,13,-1,13,147,148,19,-1,19,149,877,25,-1,25,877,150,31,-1,31,150,151,37,-1,37,151,43,-1,43,1180,155,49,-1,49,155,1181,55,-1,55,152,153,61,-1,61,156,67,-1,67,154,2,-1,147,13,146,-1,149,19,148,-1,1180,43,151,-1,55,1181,152,-1,61,153,156,-1,67,156,154,-1,157,158,159,160,-1,161,157,160,162,-1,163,161,162,164,-1,165,163,164,166,-1,167,165,166,168,-1,160,159,169,170,-1,162,160,170,171,-1,164,162,171,172,-1,166,164,172,173,-1,168,166,173,174,-1,170,169,175,176,-1,171,170,176,177,-1,172,171,177,178,-1,173,172,178,179,-1,174,173,179,180,-1,176,175,181,182,-1,177,176,182,183,-1,178,177,183,184,-1,179,178,184,185,-1,180,179,185,186,-1,182,181,187,188,-1,183,182,188,189,-1,184,183,189,190,-1,185,184,190,191,-1,186,185,191,192,-1,188,187,193,194,-1,189,188,194,195,-1,190,189,195,196,-1,191,190,196,197,-1,192,191,197,198,-1,194,193,199,200,-1,195,194,200,201,-1,196,195,201,202,-1,197,196,202,203,-1,198,197,203,204,-1,200,199,205,206,-1,201,200,206,207,-1,202,201,207,208,-1,203,202,208,209,-1,204,203,209,210,-1,206,205,211,212,-1,207,206,212,213,-1,208,207,213,214,-1,209,208,214,215,-1,210,209,215,216,-1,212,211,217,218,-1,213,212,218,219,-1,214,213,219,220,-1,215,214,220,221,-1,216,215,221,222,-1,218,217,223,224,-1,219,218,224,225,-1,220,219,225,226,-1,221,220,226,227,-1,222,221,227,228,-1,224,223,158,157,-1,225,224,157,161,-1,226,225,161,163,-1,227,226,163,165,-1,228,227,165,167,-1,229,167,168,230,-1,231,229,230,232,-1,233,231,232,234,-1,235,233,234,236,-1,237,235,236,238,-1,230,168,174,239,-1,232,230,239,240,-1,234,232,240,241,-1,236,234,241,242,-1,238,236,242,243,-1,239,174,180,244,-1,240,239,244,245,-1,241,240,245,246,-1,242,241,246,247,-1,243,242,247,248,-1,244,180,186,249,-1,245,244,249,250,-1,246,245,250,251,-1,247,246,251,252,-1,248,247,252,253,-1,249,186,192,254,-1,250,249,254,255,-1,251,250,255,256,-1,252,251,256,257,-1,253,252,257,258,-1,254,192,198,259,-1,255,254,259,260,-1,256,255,260,261,-1,257,256,261,262,-1,258,257,262,263,-1,259,198,204,264,-1,260,259,264,265,-1,261,260,265,266,-1,262,261,266,267,-1,263,262,267,268,-1,264,204,210,269,-1,265,264,269,270,-1,266,265,270,271,-1,267,266,271,272,-1,268,267,272,273,-1,269,210,216,274,-1,270,269,274,275,-1,271,270,275,276,-1,272,271,276,277,-1,273,272,277,278,-1,274,216,222,279,-1,275,274,279,280,-1,276,275,280,281,-1,277,276,281,282,-1,278,277,282,283,-1,279,222,228,284,-1,280,279,284,285,-1,281,280,285,286,-1,282,281,286,287,-1,283,282,287,288,-1,284,228,167,229,-1,285,284,229,231,-1,286,285,231,233,-1,287,286,233,235,-1,288,287,235,237,-1,158,289,290,159,-1,159,290,291,169,-1,169,292,175,-1,175,292,293,181,-1,181,293,294,187,-1,187,294,295,193,-1,193,295,199,-1,199,296,297,205,-1,205,297,298,211,-1,211,298,217,-1,217,299,300,223,-1,223,301,289,158,-1,1189,238,243,1182,-1,1182,243,248,1183,-1,1183,248,253,1184,-1,1184,253,258,-1,302,258,263,1185,-1,1185,263,268,-1,1188,268,273,-1,1188,273,278,1186,-1,1186,278,283,303,-1,1187,283,288,-1,305,288,237,304,-1,169,291,292,-1,296,199,295,-1,299,217,298,-1,223,300,301,-1,258,302,1184,-1,268,876,1185,-1,876,268,1188,-1,303,283,1187,-1,1187,288,305,-1,306,307,308,309,-1,310,306,309,311,-1,312,310,311,313,-1,314,312,313,315,-1,316,314,315,317,-1,318,316,317,319,-1,309,308,320,321,-1,311,309,321,322,-1,313,311,322,323,-1,315,313,323,324,-1,317,315,324,325,-1,319,317,325,326,-1,321,320,327,328,-1,322,321,328,329,-1,323,322,329,330,-1,324,323,330,331,-1,325,324,331,332,-1,326,325,332,333,-1,328,327,334,335,-1,329,328,335,336,-1,330,329,336,337,-1,331,330,337,338,-1,332,331,338,339,-1,333,332,339,340,-1,335,334,341,342,-1,336,335,342,343,-1,337,336,343,344,-1,338,337,344,345,-1,339,338,345,346,-1,340,339,346,347,-1,342,341,348,349,-1,343,342,349,350,-1,344,343,350,351,-1,345,344,351,352,-1,346,345,352,353,-1,347,346,353,354,-1,349,348,355,356,-1,350,349,356,357,-1,351,350,357,358,-1,352,351,358,359,-1,353,352,359,360,-1,354,353,360,361,-1,356,355,362,363,-1,357,356,363,364,-1,358,357,364,365,-1,359,358,365,366,-1,360,359,366,367,-1,361,360,367,368,-1,363,362,369,370,-1,364,363,370,371,-1,365,364,371,372,-1,366,365,372,373,-1,367,366,373,374,-1,368,367,374,375,-1,370,369,376,377,-1,371,370,377,378,-1,372,371,378,379,-1,373,372,379,380,-1,374,373,380,381,-1,375,374,381,382,-1,377,376,383,384,-1,378,377,384,385,-1,379,378,385,386,-1,380,379,386,387,-1,381,380,387,388,-1,382,381,388,389,-1,384,383,390,391,-1,385,384,391,392,-1,386,385,392,393,-1,387,386,393,394,-1,388,387,394,395,-1,389,388,395,396,-1,391,390,397,398,-1,392,391,398,399,-1,393,392,399,400,-1,394,393,400,401,-1,395,394,401,402,-1,396,395,402,403,-1,398,397,404,405,-1,399,398,405,406,-1,400,399,406,407,-1,401,400,407,408,-1,402,401,408,409,-1,403,402,409,410,-1,405,404,411,412,-1,406,405,412,413,-1,407,406,413,414,-1,408,407,414,415,-1,409,408,415,416,-1,410,409,416,417,-1,412,411,418,419,-1,413,412,419,420,-1,414,413,420,421,-1,415,414,421,422,-1,416,415,422,423,-1,417,416,423,424,-1,419,418,425,426,-1,420,419,426,427,-1,421,420,427,428,-1,422,421,428,429,-1,423,422,429,430,-1,424,423,430,431,-1,426,425,432,433,-1,427,426,433,434,-1,428,427,434,435,-1,429,428,435,436,-1,430,429,436,437,-1,431,430,437,438,-1,433,432,439,440,-1,434,433,440,441,-1,435,434,441,442,-1,436,435,442,443,-1,437,436,443,444,-1,438,437,444,445,-1,440,439,446,447,-1,441,440,447,448,-1,442,441,448,449,-1,443,442,449,450,-1,444,443,450,451,-1,445,444,451,452,-1,447,446,453,454,-1,448,447,454,455,-1,449,448,455,456,-1,450,449,456,457,-1,451,450,457,458,-1,452,451,458,459,-1,454,453,460,461,-1,455,454,461,462,-1,456,455,462,463,-1,457,456,463,464,-1,458,457,464,465,-1,459,458,465,466,-1,461,460,467,468,-1,462,461,468,469,-1,463,462,469,470,-1,464,463,470,471,-1,465,464,471,472,-1,466,465,472,473,-1,468,467,307,306,-1,469,468,306,310,-1,470,469,310,312,-1,471,470,312,314,-1,472,471,314,316,-1,473,472,316,318,-1,474,318,319,475,-1,476,474,475,477,-1,478,476,477,479,-1,475,319,326,480,-1,477,475,480,481,-1,479,477,481,482,-1,483,479,482,484,-1,485,483,484,486,-1,487,485,486,488,-1,480,326,333,489,-1,481,480,489,490,-1,482,481,490,491,-1,484,482,491,492,-1,486,484,492,493,-1,488,486,493,494,-1,489,333,340,495,-1,490,489,495,496,-1,491,490,496,497,-1,492,491,497,498,-1,493,492,498,499,-1,494,493,499,500,-1,495,340,347,501,-1,496,495,501,502,-1,497,496,502,503,-1,498,497,503,504,-1,499,498,504,505,-1,500,499,505,506,-1,501,347,354,507,-1,502,501,507,508,-1,503,502,508,509,-1,504,503,509,510,-1,505,504,510,511,-1,506,505,511,512,-1,507,354,361,513,-1,508,507,513,514,-1,509,508,514,515,-1,510,509,515,516,-1,511,510,516,517,-1,512,511,517,518,-1,513,361,368,519,-1,514,513,519,520,-1,515,514,520,521,-1,516,515,521,522,-1,517,516,522,523,-1,518,517,523,524,-1,519,368,375,525,-1,520,519,525,526,-1,521,520,526,527,-1,522,521,527,528,-1,523,522,528,529,-1,524,523,529,530,-1,525,375,382,531,-1,526,525,531,532,-1,527,526,532,533,-1,528,527,533,534,-1,529,528,534,535,-1,530,529,535,536,-1,531,382,389,537,-1,532,531,537,538,-1,533,532,538,539,-1,534,533,539,540,-1,535,534,540,541,-1,536,535,541,542,-1,539,538,543,544,-1,540,539,544,545,-1,541,540,545,546,-1,544,543,547,548,-1,545,544,548,549,-1,546,545,549,550,-1,551,403,410,552,-1,547,551,552,553,-1,548,547,553,554,-1,549,548,554,555,-1,550,549,555,556,-1,557,550,556,558,-1,552,410,417,559,-1,553,552,559,560,-1,554,553,560,561,-1,555,554,561,562,-1,556,555,562,563,-1,558,556,563,564,-1,559,417,424,565,-1,560,559,565,566,-1,561,560,566,567,-1,562,561,567,568,-1,563,562,568,569,-1,564,563,569,570,-1,565,424,431,571,-1,566,565,571,572,-1,567,566,572,573,-1,568,567,573,574,-1,569,568,574,575,-1,570,569,575,576,-1,571,431,438,577,-1,572,571,577,578,-1,573,572,578,579,-1,574,573,579,580,-1,575,574,580,581,-1,576,575,581,582,-1,577,438,445,583,-1,578,577,583,584,-1,579,578,584,585,-1,580,579,585,586,-1,581,580,586,587,-1,582,581,587,588,-1,583,445,452,589,-1,584,583,589,590,-1,585,584,590,591,-1,586,585,591,592,-1,587,586,592,593,-1,588,587,593,594,-1,589,452,459,595,-1,590,589,595,596,-1,591,590,596,597,-1,592,591,597,598,-1,593,592,598,599,-1,594,593,599,600,-1,595,459,466,601,-1,596,595,601,602,-1,597,596,602,603,-1,598,597,603,604,-1,599,598,604,605,-1,600,599,605,606,-1,601,466,473,607,-1,602,601,607,608,-1,603,602,608,609,-1,604,603,609,610,-1,605,604,610,611,-1,606,605,611,612,-1,607,473,318,474,-1,608,607,474,476,-1,609,608,476,478,-1,613,614,615,616,-1,617,613,616,618,-1,619,617,618,620,-1,621,619,620,622,-1,623,487,488,624,-1,615,623,624,625,-1,616,615,625,626,-1,618,616,626,627,-1,620,618,627,628,-1,622,620,628,629,-1,624,488,494,630,-1,625,624,630,631,-1,626,625,631,632,-1,627,626,632,633,-1,628,627,633,634,-1,629,628,634,635,-1,630,494,500,636,-1,631,630,636,637,-1,632,631,637,638,-1,633,632,638,639,-1,634,633,639,640,-1,635,634,640,641,-1,636,500,506,642,-1,637,636,642,643,-1,638,637,643,644,-1,639,638,644,645,-1,640,639,645,646,-1,641,640,646,647,-1,642,506,512,648,-1,643,642,648,649,-1,644,643,649,650,-1,645,644,650,651,-1,646,645,651,652,-1,647,646,652,653,-1,648,512,518,654,-1,649,648,654,655,-1,650,649,655,656,-1,651,650,656,657,-1,652,651,657,658,-1,653,652,658,659,-1,654,518,524,660,-1,655,654,660,661,-1,656,655,661,662,-1,657,656,662,663,-1,658,657,663,664,-1,659,658,664,665,-1,660,524,530,666,-1,661,660,666,667,-1,662,661,667,668,-1,663,662,668,669,-1,664,663,669,670,-1,665,664,670,671,-1,666,530,536,672,-1,667,666,672,673,-1,668,667,673,674,-1,669,668,674,675,-1,670,669,675,676,-1,671,670,676,677,-1,672,536,542,678,-1,673,672,678,679,-1,674,673,679,680,-1,675,674,680,681,-1,676,675,681,682,-1,677,676,682,683,-1,680,679,684,685,-1,681,680,685,686,-1,682,681,686,687,-1,683,682,687,688,-1,685,684,689,690,-1,686,685,690,691,-1,687,686,691,692,-1,688,687,692,693,-1,694,557,558,695,-1,689,694,695,696,-1,690,689,696,697,-1,691,690,697,698,-1,692,691,698,699,-1,693,692,699,700,-1,695,558,564,701,-1,696,695,701,702,-1,697,696,702,703,-1,698,697,703,704,-1,699,698,704,705,-1,700,699,705,706,-1,701,564,570,707,-1,702,701,707,708,-1,703,702,708,709,-1,704,703,709,710,-1,705,704,710,711,-1,706,705,711,712,-1,707,570,576,713,-1,708,707,713,714,-1,709,708,714,715,-1,710,709,715,716,-1,711,710,716,717,-1,712,711,717,718,-1,713,576,582,719,-1,714,713,719,720,-1,715,714,720,721,-1,716,715,721,722,-1,717,716,722,723,-1,718,717,723,724,-1,719,582,588,725,-1,720,719,725,726,-1,721,720,726,727,-1,722,721,727,728,-1,723,722,728,729,-1,724,723,729,730,-1,725,588,594,731,-1,726,725,731,732,-1,727,726,732,733,-1,728,727,733,734,-1,729,728,734,735,-1,730,729,735,736,-1,731,594,600,737,-1,732,731,737,738,-1,733,732,738,739,-1,734,733,739,740,-1,735,734,740,741,-1,736,735,741,742,-1,737,600,606,743,-1,738,737,743,744,-1,739,738,744,745,-1,740,739,745,746,-1,741,740,746,747,-1,742,741,747,748,-1,743,606,612,749,-1,744,743,749,750,-1,745,744,750,751,-1,746,745,751,752,-1,747,746,752,753,-1,748,747,753,754,-1,751,750,614,613,-1,752,751,613,617,-1,753,752,617,619,-1,754,753,619,621,-1,755,756,757,-1,758,755,757,759,-1,760,758,759,761,-1,762,760,761,763,-1,764,762,763,765,-1,621,764,765,754,-1,757,756,766,-1,759,757,766,767,-1,761,759,767,768,-1,763,761,768,769,-1,765,763,769,770,-1,754,765,770,748,-1,766,756,771,-1,767,766,771,772,-1,768,767,772,773,-1,769,768,773,774,-1,770,769,774,775,-1,748,770,775,742,-1,771,756,776,-1,772,771,776,777,-1,773,772,777,778,-1,774,773,778,779,-1,775,774,779,780,-1,742,775,780,736,-1,776,756,781,-1,777,776,781,782,-1,778,777,782,783,-1,779,778,783,784,-1,780,779,784,785,-1,736,780,785,730,-1,781,756,786,-1,782,781,786,787,-1,783,782,787,788,-1,784,783,788,789,-1,785,784,789,790,-1,730,785,790,724,-1,786,756,791,-1,787,786,791,792,-1,788,787,792,793,-1,789,788,793,794,-1,790,789,794,795,-1,724,790,795,718,-1,791,756,796,-1,792,791,796,797,-1,793,792,797,798,-1,794,793,798,799,-1,795,794,799,800,-1,718,795,800,712,-1,796,756,801,-1,797,796,801,802,-1,798,797,802,803,-1,799,798,803,804,-1,800,799,804,805,-1,712,800,805,706,-1,801,756,806,-1,802,801,806,807,-1,803,802,807,808,-1,804,803,808,809,-1,805,804,809,810,-1,706,805,810,700,-1,806,756,811,-1,807,806,811,812,-1,808,807,812,813,-1,809,808,813,814,-1,810,809,814,815,-1,700,810,815,693,-1,811,756,816,-1,812,811,816,817,-1,813,812,817,818,-1,814,813,818,819,-1,815,814,819,820,-1,693,815,820,688,-1,816,756,821,-1,817,816,821,822,-1,818,817,822,823,-1,819,818,823,824,-1,820,819,824,825,-1,688,820,825,683,-1,821,756,826,-1,822,821,826,827,-1,823,822,827,828,-1,824,823,828,829,-1,825,824,829,830,-1,683,825,830,677,-1,826,756,831,-1,827,826,831,832,-1,828,827,832,833,-1,829,828,833,834,-1,830,829,834,835,-1,677,830,835,671,-1,831,756,836,-1,832,831,836,837,-1,833,832,837,838,-1,834,833,838,839,-1,835,834,839,840,-1,671,835,840,665,-1,836,756,841,-1,837,836,841,842,-1,838,837,842,843,-1,839,838,843,844,-1,840,839,844,845,-1,665,840,845,659,-1,841,756,846,-1,842,841,846,847,-1,843,842,847,848,-1,844,843,848,849,-1,845,844,849,850,-1,659,845,850,653,-1,846,756,851,-1,847,846,851,852,-1,848,847,852,853,-1,849,848,853,854,-1,850,849,854,855,-1,653,850,855,647,-1,851,756,856,-1,852,851,856,857,-1,853,852,857,858,-1,854,853,858,859,-1,855,854,859,860,-1,647,855,860,641,-1,856,756,861,-1,857,856,861,862,-1,858,857,862,863,-1,859,858,863,864,-1,860,859,864,865,-1,641,860,865,635,-1,861,756,866,-1,862,861,866,867,-1,863,862,867,868,-1,864,863,868,869,-1,865,864,869,870,-1,635,865,870,629,-1,866,756,871,-1,867,866,871,872,-1,868,867,872,873,-1,869,868,873,874,-1,870,869,874,875,-1,629,870,875,622,-1,871,756,755,-1,872,871,755,758,-1,873,872,758,760,-1,874,873,760,762,-1,875,874,762,764,-1,622,875,764,621,-1,537,389,292,-1,538,537,290,-1,538,290,289,543,-1,542,541,546,-1,542,546,305,-1,542,305,1189,-1,403,551,296,-1,295,396,403,296,-1,547,543,299,-1,298,551,547,299,-1,1187,546,550,557,-1,679,678,1184,-1,679,1184,302,-1,679,302,684,-1,389,396,293,-1,389,293,292,-1,301,543,289,-1,290,537,291,-1,296,551,297,-1,543,300,299,-1,305,546,1187,-1,1185,684,302,-1,694,876,1188,-1,1188,557,694,-1,684,1185,876,-1,684,876,694,689,-1,396,295,294,-1,537,292,291,-1,551,298,297,-1,543,301,300,-1,303,557,1186,-1,557,1188,1186,-1,1184,678,1183,-1,678,1182,1183,-1,1187,557,303,-1,146,483,147,-1,148,485,149,-1,609,478,156,-1,610,609,156,-1,611,610,153,152,-1,877,487,623,-1,614,151,623,615,-1,750,749,151,614,-1,478,145,154,-1,154,156,478,-1,483,146,145,-1,478,479,145,-1,145,479,483,-1,483,485,147,-1,147,485,148,-1,149,485,1179,-1,485,487,149,1179,-1,149,487,877,-1,610,156,153,-1,611,152,1181,-1,155,612,1181,-1,612,611,1181,-1,623,150,877,-1,749,612,1180,-1,151,749,1180,-1,623,151,150,-1,1180,612,155,-1,467,460,878,-1,327,320,879,-1,467,878,879,-1,467,879,320,-1,467,320,308,-1,467,308,307,-1,334,327,880,-1,880,327,879,-1,341,334,881,-1,881,334,880,-1,348,341,882,-1,882,341,881,-1,355,348,883,-1,883,348,882,-1,362,355,884,-1,884,355,883,-1,369,362,885,-1,885,362,884,-1,376,369,886,-1,886,369,885,-1,383,376,887,-1,887,376,886,-1,390,383,888,-1,888,383,887,-1,397,390,889,-1,889,390,888,-1,404,397,890,-1,890,397,889,-1,411,404,891,-1,891,404,890,-1,418,411,892,-1,892,411,891,-1,425,418,893,-1,893,418,892,-1,432,425,894,-1,894,425,893,-1,439,432,895,-1,895,432,894,-1,446,439,896,-1,896,439,895,-1,453,446,897,-1,897,446,896,-1,460,453,898,-1,898,453,897,-1,899,460,898,-1,900,460,899,-1,878,460,901,-1,901,460,900,-1,902,903,904,905,-1,906,902,905,907,-1,908,906,907,909,-1,910,908,909,911,-1,912,910,911,913,-1,905,904,914,915,-1,907,905,915,916,-1,909,907,916,917,-1,911,909,917,918,-1,913,911,918,919,-1,915,914,920,921,-1,916,915,921,922,-1,917,916,922,923,-1,918,917,923,924,-1,919,918,924,925,-1,921,920,926,927,-1,922,921,927,928,-1,923,922,928,929,-1,924,923,929,930,-1,925,924,930,931,-1,927,926,932,933,-1,928,927,933,934,-1,929,928,934,935,-1,930,929,935,936,-1,931,930,936,937,-1,933,932,938,939,-1,934,933,939,940,-1,935,934,940,941,-1,936,935,941,942,-1,937,936,942,943,-1,939,938,944,945,-1,940,939,945,946,-1,941,940,946,947,-1,942,941,947,948,-1,943,942,948,949,-1,945,944,950,951,-1,946,945,951,952,-1,947,946,952,953,-1,948,947,953,954,-1,949,948,954,955,-1,951,950,956,957,-1,952,951,957,958,-1,953,952,958,959,-1,954,953,959,960,-1,955,954,960,961,-1,957,956,962,963,-1,958,957,963,964,-1,959,958,964,965,-1,960,959,965,966,-1,961,960,966,967,-1,963,962,968,969,-1,964,963,969,970,-1,965,964,970,971,-1,966,965,971,972,-1,967,966,972,973,-1,969,968,974,975,-1,970,969,975,976,-1,971,970,976,977,-1,972,971,977,978,-1,973,972,978,979,-1,975,974,980,981,-1,976,975,981,982,-1,977,976,982,983,-1,978,977,983,984,-1,979,978,984,985,-1,981,980,986,987,-1,982,981,987,988,-1,983,982,988,989,-1,984,983,989,990,-1,985,984,990,991,-1,987,986,992,993,-1,988,987,993,994,-1,989,988,994,995,-1,990,989,995,996,-1,991,990,996,997,-1,993,992,998,999,-1,994,993,999,1000,-1,995,994,1000,1001,-1,996,995,1001,1002,-1,997,996,1002,1003,-1,999,998,1004,1005,-1,1000,999,1005,1006,-1,1001,1000,1006,1007,-1,1002,1001,1007,1008,-1,1003,1002,1008,1009,-1,1005,1004,1010,1011,-1,1006,1005,1011,1012,-1,1007,1006,1012,1013,-1,1008,1007,1013,1014,-1,1009,1008,1014,1015,-1,1011,1010,1016,1017,-1,1012,1011,1017,1018,-1,1013,1012,1018,1019,-1,1014,1013,1019,1020,-1,1015,1014,1020,1021,-1,1017,1016,1022,1023,-1,1018,1017,1023,1024,-1,1019,1018,1024,1025,-1,1020,1019,1025,1026,-1,1021,1020,1026,1027,-1,1023,1022,1028,1029,-1,1024,1023,1029,1030,-1,1025,1024,1030,1031,-1,1026,1025,1031,1032,-1,1027,1026,1032,1033,-1,1029,1028,1034,1035,-1,1030,1029,1035,1036,-1,1031,1030,1036,1037,-1,1032,1031,1037,1038,-1,1033,1032,1038,1039,-1,1035,1034,1040,1041,-1,1036,1035,1041,1042,-1,1037,1036,1042,1043,-1,1038,1037,1043,1044,-1,1039,1038,1044,1045,-1,1041,1040,903,902,-1,1042,1041,902,906,-1,1043,1042,906,908,-1,1044,1043,908,910,-1,1045,1044,910,912,-1,903,1046,1047,904,-1,904,1048,1049,914,-1,914,1049,1050,920,-1,920,1050,1051,926,-1,926,1051,1052,932,-1,932,1052,938,-1,938,1053,944,-1,944,1054,950,-1,950,1055,956,-1,956,1056,962,-1,962,1057,1058,968,-1,968,1059,1060,974,-1,974,1061,1062,980,-1,980,1063,1064,986,-1,986,1065,992,-1,992,1066,998,-1,998,1067,1004,-1,1004,1080,1068,1010,-1,1010,1069,1070,1016,-1,1016,1071,1072,1022,-1,1022,1073,1074,1028,-1,1028,1075,1076,1034,-1,1034,1077,1078,1040,-1,1040,1079,1046,903,-1,1048,904,1047,-1,1053,938,1052,-1,1054,944,1053,-1,1055,950,1054,-1,1056,956,1055,-1,1057,962,1056,-1,1059,968,1058,-1,1061,974,1060,-1,1063,980,1062,-1,1065,986,1064,-1,1066,992,1065,-1,1067,998,1066,-1,1080,1004,1067,-1,1069,1010,1068,-1,1071,1016,1070,-1,1073,1022,1072,-1,1075,1028,1074,-1,1077,1034,1076,-1,1081,1040,1078,-1,1040,1081,1079,-1,1082,1083,1084,-1,1085,1082,1084,1086,-1,1087,1085,1086,1088,-1,1089,1087,1088,1090,-1,1084,1083,1091,-1,1086,1084,1091,1092,-1,1088,1086,1092,1093,-1,1090,1088,1093,1094,-1,1091,1083,1095,-1,1092,1091,1095,1096,-1,1093,1092,1096,1097,-1,1094,1093,1097,1098,-1,1095,1083,1099,-1,1096,1095,1099,1100,-1,1097,1096,1100,1101,-1,1098,1097,1101,1102,-1,1099,1083,1103,-1,1100,1099,1103,1104,-1,1101,1100,1104,1105,-1,1102,1101,1105,1106,-1,1103,1083,1107,-1,1104,1103,1107,1108,-1,1105,1104,1108,1109,-1,1106,1105,1109,1110,-1,1107,1083,1111,-1,1108,1107,1111,1112,-1,1109,1108,1112,1113,-1,1110,1109,1113,1114,-1,1111,1083,1115,-1,1112,1111,1115,1116,-1,1113,1112,1116,1117,-1,1114,1113,1117,1118,-1,1115,1083,1119,-1,1116,1115,1119,1120,-1,1117,1116,1120,1121,-1,1118,1117,1121,1122,-1,1119,1083,1123,-1,1120,1119,1123,1124,-1,1121,1120,1124,1125,-1,1122,1121,1125,1126,-1,1123,1083,1127,-1,1124,1123,1127,1128,-1,1125,1124,1128,1129,-1,1126,1125,1129,1130,-1,1127,1083,1131,-1,1128,1127,1131,1132,-1,1129,1128,1132,1133,-1,1130,1129,1133,1134,-1,1131,1083,1135,-1,1132,1131,1135,1136,-1,1133,1132,1136,1137,-1,1134,1133,1137,1138,-1,1135,1083,1139,-1,1136,1135,1139,1140,-1,1137,1136,1140,1141,-1,1138,1137,1141,1142,-1,1139,1083,1143,-1,1140,1139,1143,1144,-1,1141,1140,1144,1145,-1,1142,1141,1145,1146,-1,1143,1083,1147,-1,1144,1143,1147,1148,-1,1145,1144,1148,1149,-1,1146,1145,1149,1150,-1,1147,1083,1151,-1,1148,1147,1151,1152,-1,1149,1148,1152,1153,-1,1150,1149,1153,1154,-1,1151,1083,1155,-1,1152,1151,1155,1156,-1,1153,1152,1156,1157,-1,1154,1153,1157,1158,-1,1155,1083,1159,-1,1156,1155,1159,1160,-1,1157,1156,1160,1161,-1,1158,1157,1161,1162,-1,1159,1083,1163,-1,1160,1159,1163,1164,-1,1161,1160,1164,1165,-1,1162,1161,1165,1166,-1,1163,1083,1167,-1,1164,1163,1167,1168,-1,1165,1164,1168,1169,-1,1166,1165,1169,1170,-1,1167,1083,1171,-1,1168,1167,1171,1172,-1,1169,1168,1172,1173,-1,1170,1169,1173,1174,-1,1171,1083,1175,-1,1172,1171,1175,1176,-1,1173,1172,1176,1177,-1,1174,1173,1177,1178,-1,1175,1083,1082,-1,1176,1175,1082,1085,-1,1177,1176,1085,1087,-1,1178,1177,1087,1089,-1,1046,1089,1090,1047,-1,1047,1090,1094,1048,-1,1049,1094,1098,-1,1050,1098,1102,-1,1051,1102,1106,-1,1052,1106,1110,1053,-1,1053,1110,1114,1054,-1,1054,1114,1118,1055,-1,1055,1118,1122,1056,-1,1056,1122,1126,1057,-1,1058,1126,1130,1059,-1,1060,1130,1134,1061,-1,1062,1134,1138,1063,-1,1064,1138,1142,1065,-1,1065,1142,1146,1066,-1,1066,1146,1150,1067,-1,1067,1150,1154,1080,-1,1068,1158,1162,1069,-1,1070,1162,1166,1071,-1,1072,1166,1170,1073,-1,1074,1170,1174,1075,-1,1076,1174,1178,1077,-1,1078,1178,1089,1081,-1,1079,1089,1046,-1,1094,1049,1048,-1,1098,1050,1049,-1,1102,1051,1050,-1,1106,1052,1051,-1,1126,1058,1057,-1,1130,1060,1059,-1,1134,1062,1061,-1,1138,1064,1063,-1,1158,1068,1080,-1,1162,1070,1069,-1,1166,1072,1071,-1,1170,1074,1073,-1,1174,1076,1075,-1,1178,1078,1077,-1,1089,1079,1081,-1,878,912,913,879,-1,879,913,919,880,-1,880,919,925,881,-1,881,925,931,882,-1,882,931,937,883,-1,883,937,943,884,-1,884,943,949,885,-1,885,949,955,886,-1,886,955,961,887,-1,887,961,967,888,-1,888,967,973,889,-1,889,973,979,890,-1,890,979,985,891,-1,891,985,991,892,-1,892,991,997,893,-1,893,997,1003,894,-1,894,1003,1009,895,-1,895,1009,1015,896,-1,896,1015,1021,897,-1,897,1021,1027,898,-1,898,1027,1033,899,-1,899,1033,1039,900,-1,900,1039,1045,901,-1,901,1045,912,878,-1,1154,1158,1080,-1,237,238,1189,-1,237,1189,305,-1,237,305,304,-1,1189,1182,542,-1,1182,678,542,-1,294,293,396,-1};
static int teapot_coordindex_n = 6302;
static float teapot_coord_p [] = {0.000000,-0.750000,0.000000,1.148825,0.059635,0.010277,1.043040,-0.007725,0.010277,1.049830,-0.033315,0.141457,1.158700,0.040090,0.125638,1.207390,0.154080,0.010277,1.218500,0.140885,0.105073,1.245120,0.261025,0.010277,1.257465,0.253720,0.084509,1.288410,0.365885,0.010277,1.303840,0.363235,0.068691,1.363650,0.454090,0.010277,1.385870,0.454090,0.062364,1.066805,-0.097305,0.220164,1.183395,-0.008780,0.194855,1.246280,0.107900,0.161951,1.288330,0.235460,0.129049,1.342430,0.356615,0.103739,1.441430,0.454090,0.093615,1.088870,-0.180480,0.246400,1.215490,-0.072320,0.217927,1.282390,0.065015,0.180911,1.328465,0.211720,0.143895,1.392585,0.348005,0.115422,1.513650,0.454090,0.104032,1.110940,-0.263651,0.220164,1.247590,-0.135850,0.194855,1.318510,0.022135,0.161952,1.368590,0.187980,0.129049,1.442740,0.339395,0.103739,1.585880,0.454090,0.093616,1.127915,-0.327631,0.141457,1.272280,-0.184715,0.125639,1.346290,-0.010850,0.105074,1.399450,0.169720,0.084510,1.481320,0.332770,0.068691,1.641440,0.454090,0.062364,1.134705,-0.353224,0.010277,1.282160,-0.204265,0.010278,1.357400,-0.024045,0.010278,1.411800,0.162415,0.010278,1.496750,0.330120,0.010278,1.663660,0.454090,0.010278,1.127915,-0.327631,-0.120902,1.272280,-0.184715,-0.105083,1.346290,-0.010850,-0.084519,1.399450,0.169720,-0.063955,1.481320,0.332770,-0.048136,1.641440,0.454090,-0.041808,1.110940,-0.263651,-0.199610,1.247590,-0.135850,-0.174300,1.318510,0.022135,-0.141397,1.368590,0.187980,-0.108494,1.442740,0.339395,-0.083184,1.585870,0.454090,-0.073060,1.088870,-0.180480,-0.225846,1.215490,-0.072320,-0.197372,1.282390,0.065015,-0.160357,1.328465,0.211720,-0.123341,1.392585,0.348005,-0.094867,1.513650,0.454090,-0.083477,1.066805,-0.097305,-0.199610,1.183395,-0.008780,-0.174300,1.246280,0.107900,-0.141397,1.288330,0.235460,-0.108494,1.342430,0.356615,-0.083185,1.441430,0.454090,-0.073060,1.049830,-0.033315,-0.120902,1.158700,0.040090,-0.105084,1.218500,0.140885,-0.084519,1.257465,0.253720,-0.063955,1.303840,0.363235,-0.048136,1.385870,0.454090,-0.041809,1.388185,0.469715,0.010277,1.411855,0.470055,0.060796,1.409945,0.479090,0.010278,1.433540,0.479705,0.056769,1.426150,0.482215,0.010278,1.448490,0.482995,0.051295,1.434020,0.479090,0.010278,1.454255,0.479860,0.045387,1.430780,0.469715,0.010278,1.448395,0.470245,0.040058,1.413650,0.454090,0.010278,1.428465,0.454090,0.036320,1.471020,0.470895,0.091069,1.492525,0.481250,0.084355,1.504335,0.484950,0.074865,1.504835,0.481790,0.063984,1.492430,0.471570,0.053104,1.465500,0.454090,0.043613,1.547940,0.471995,0.101124,1.569205,0.483255,0.093268,1.576930,0.487490,0.081766,1.570595,0.484300,0.067920,1.549675,0.473295,0.053032,1.513650,0.454090,0.038404,1.624865,0.473090,0.090992,1.645895,0.485265,0.083739,1.649540,0.490025,0.072781,1.636360,0.486805,0.059046,1.606930,0.475020,0.043459,1.561800,0.454090,0.026945,1.684035,0.473935,0.060700,1.704880,0.486805,0.055998,1.705385,0.491980,0.048692,1.686945,0.488735,0.039215,1.650965,0.476345,0.028002,1.598845,0.454090,0.015487,1.707700,0.474270,0.010278,1.728475,0.487425,0.010278,1.727725,0.492760,0.010278,1.707180,0.489505,0.010278,1.668580,0.476875,0.010278,1.613660,0.454090,0.010278,1.684035,0.473935,-0.040264,1.704880,0.486805,-0.036406,1.705385,0.491980,-0.031390,1.686945,0.488735,-0.026375,1.650965,0.476345,-0.022517,1.598845,0.454090,-0.020974,1.624865,0.473090,-0.070590,1.645895,0.485265,-0.064417,1.649540,0.490025,-0.056392,1.636360,0.486805,-0.048367,1.606930,0.475020,-0.042194,1.561800,0.454090,-0.039725,1.547940,0.471995,-0.080699,1.569205,0.483255,-0.073754,1.576930,0.487490,-0.064726,1.570595,0.484300,-0.055698,1.549675,0.473295,-0.048753,1.513650,0.454090,-0.045975,1.471020,0.470895,-0.070591,1.492525,0.481250,-0.064417,1.504335,0.484950,-0.056392,1.504835,0.481790,-0.048367,1.492430,0.471570,-0.042194,1.465500,0.454090,-0.039725,1.411855,0.470055,-0.040265,1.433540,0.479705,-0.036407,1.448490,0.482995,-0.031391,1.454255,0.479860,-0.026376,1.448395,0.470245,-0.022517,1.428465,0.454090,-0.020974,0.947765,-0.050712,0.145199,0.949770,-0.061850,0.157877,0.954494,-0.122208,0.225588,0.965825,-0.182515,0.242866,0.967288,-0.213253,0.251729,0.973877,-0.379134,0.145447,0.986036,-0.402350,0.016630,0.966995,-0.182515,-0.223702,0.955408,-0.120540,-0.205494,0.955634,-0.019872,0.007642,0.978411,-0.302964,-0.204801,0.949180,-0.052564,-0.128725,-1.076185,0.264745,0.005367,-0.940300,0.268390,0.005368,-0.941550,0.276685,0.067870,-1.081260,0.272770,0.067870,-1.184285,0.254845,0.005367,-1.192155,0.262140,0.067870,-1.263220,0.235575,0.005367,-1.272960,0.241440,0.067869,-1.311600,0.203805,0.005367,-1.322390,0.207315,0.067869,-1.328040,0.156405,0.005367,-1.339150,0.156405,0.067869,-0.944680,0.297420,0.105371,-1.093945,0.292830,0.105371,-1.211830,0.280370,0.105370,-1.297310,0.256100,0.105370,-1.349355,0.216095,0.105370,-1.366930,0.156405,0.105371,-0.948750,0.324380,0.117872,-1.110440,0.318910,0.117871,-1.237410,0.304065,0.117871,-1.328965,0.275160,0.117871,-1.384405,0.227505,0.117871,-1.403040,0.156405,0.117871,-0.952815,0.351335,0.105371,-1.126940,0.344990,0.105371,-1.262990,0.327765,0.105370,-1.360620,0.294220,0.105370,-1.419460,0.238915,0.105370,-1.439150,0.156405,0.105371,-0.955945,0.372075,0.067870,-1.139625,0.365050,0.067870,-1.282665,0.345995,0.067869,-1.384965,0.308880,0.067869,-1.446425,0.247690,0.067869,-1.466935,0.156405,0.067869,-0.957195,0.380370,0.005368,-1.144700,0.373075,0.005367,-1.290535,0.353285,0.005367,-1.394705,0.314745,0.005367,-1.457210,0.251200,0.005367,-1.478045,0.156405,0.005367,-0.955945,0.372075,-0.057135,-1.139625,0.365050,-0.057135,-1.282665,0.345995,-0.057135,-1.384965,0.308880,-0.057135,-1.446425,0.247690,-0.057135,-1.466935,0.156405,-0.057135,-0.952815,0.351335,-0.094636,-1.126940,0.344990,-0.094636,-1.262990,0.327765,-0.094636,-1.360620,0.294220,-0.094636,-1.419460,0.238915,-0.094636,-1.439155,0.156405,-0.094636,-0.948750,0.324380,-0.107136,-1.110440,0.318910,-0.107136,-1.237410,0.304065,-0.107136,-1.328965,0.275160,-0.107137,-1.384405,0.227505,-0.107137,-1.403040,0.156405,-0.107137,-0.944680,0.297420,-0.094636,-1.093945,0.292830,-0.094636,-1.211830,0.280370,-0.094636,-1.297310,0.256100,-0.094636,-1.349355,0.216095,-0.094636,-1.366930,0.156405,-0.094636,-0.941550,0.276685,-0.057134,-1.081260,0.272770,-0.057134,-1.192155,0.262140,-0.057135,-1.272960,0.241440,-0.057135,-1.322390,0.207315,-0.057135,-1.339150,0.156405,-0.057135,-1.319470,0.091820,0.005367,-1.330130,0.088395,0.067869,-1.292850,0.014735,0.005368,-1.302180,0.009075,0.067870,-1.246785,-0.068600,0.005368,-1.253960,-0.075720,0.067870,-1.179885,-0.151935,0.005368,-1.184140,-0.160165,0.067871,-1.090765,-0.229015,0.005368,-1.091375,-0.238435,0.067871,-1.356775,0.079825,0.105371,-1.325495,-0.005070,0.105371,-1.271900,-0.093515,0.105371,-1.194770,-0.180740,0.105372,-1.092895,-0.261988,0.105372,-1.391410,0.068690,0.117871,-1.355820,-0.023460,0.117872,-1.295220,-0.116645,0.117872,-1.208590,-0.207490,0.117872,-1.094875,-0.292604,0.117872,-1.426040,0.057550,0.105371,-1.386135,-0.041850,0.105371,-1.318545,-0.139780,0.105371,-1.222410,-0.234240,0.105371,-1.096855,-0.323220,0.105372,-1.452690,0.048980,0.067869,-1.409455,-0.055995,0.067870,-1.336485,-0.157575,0.067870,-1.233040,-0.254816,0.067871,-1.098375,-0.346770,0.067871,-1.463345,0.045555,0.005367,-1.418780,-0.061655,0.005368,-1.343665,-0.164695,0.005368,-1.237295,-0.263047,0.005368,-1.098985,-0.356191,0.005369,-1.452690,0.048980,-0.057135,-1.409455,-0.055995,-0.057134,-1.336490,-0.157575,-0.057134,-1.233040,-0.254816,-0.057134,-1.098375,-0.346771,-0.057133,-1.426040,0.057550,-0.094636,-1.386135,-0.041850,-0.094636,-1.318545,-0.139780,-0.094635,-1.222410,-0.234240,-0.094635,-1.096855,-0.323220,-0.094635,-1.391410,0.068690,-0.107137,-1.355820,-0.023460,-0.107136,-1.295220,-0.116645,-0.107136,-1.208590,-0.207490,-0.107135,-1.094875,-0.292604,-0.107135,-1.356775,0.079825,-0.094636,-1.325500,-0.005070,-0.094636,-1.271900,-0.093515,-0.094636,-1.194770,-0.180740,-0.094635,-1.092895,-0.261989,-0.094635,-1.330130,0.088395,-0.057135,-1.302180,0.009075,-0.057134,-1.253960,-0.075720,-0.057134,-1.184140,-0.160165,-0.057134,-1.091375,-0.238435,-0.057133,-0.828580,0.268745,0.005368,-0.816635,0.277110,0.067869,-0.802355,0.297945,0.105367,-0.789532,0.322995,0.116933,-0.777300,0.352090,0.105376,-0.772385,0.372900,0.067873,-0.776680,0.381178,0.005252,-0.772445,0.372895,-0.057134,-0.777370,0.352085,-0.094633,-0.789598,0.322992,-0.106200,-0.802420,0.297940,-0.094636,-0.816705,0.277090,-0.057140,-0.828600,0.268770,0.005135,-0.956322,-0.420173,0.067328,-0.974981,-0.326715,-0.094709,-0.992085,-0.284905,0.005292,-0.992115,-0.285221,0.005252,0.697495,0.479390,0.005137,0.706810,0.452045,0.005137,0.682180,0.452045,0.193105,0.673190,0.479390,0.190603,0.698940,0.495795,0.005137,0.674585,0.495795,0.190992,0.708370,0.501265,0.005137,0.683690,0.501265,0.193525,0.723015,0.495795,0.005137,0.697815,0.495795,0.197456,0.740085,0.479390,0.005137,0.714285,0.479390,0.202040,0.756810,0.452045,0.005137,0.730420,0.452045,0.206532,0.612440,0.452045,0.360850,0.604380,0.479390,0.356115,0.605630,0.495795,0.356851,0.613790,0.501265,0.361644,0.626460,0.495795,0.369084,0.641230,0.479390,0.377759,0.655700,0.452045,0.386258,0.503810,0.452045,0.502149,0.497193,0.479390,0.495535,0.498219,0.495795,0.496562,0.504915,0.501265,0.503259,0.515315,0.495795,0.513654,0.527435,0.479390,0.525774,0.539310,0.452045,0.537649,0.362509,0.452045,0.610784,0.357774,0.479390,0.602724,0.358510,0.495795,0.603974,0.363302,0.501265,0.612134,0.370742,0.495795,0.624804,0.379418,0.479390,0.639574,0.387917,0.452045,0.654044,0.194765,0.452045,0.680524,0.192263,0.479390,0.671534,0.192651,0.495795,0.672929,0.195184,0.501265,0.682034,0.199116,0.495795,0.696159,0.203700,0.479390,0.712629,0.208191,0.452045,0.728764,0.006799,0.452045,0.705154,0.006799,0.479390,0.695839,0.006799,0.495795,0.697284,0.006799,0.501266,0.706714,0.006799,0.495795,0.721359,0.006799,0.479391,0.738429,0.006799,0.452045,0.755154,-0.181168,0.452045,0.680524,-0.178666,0.479390,0.671534,-0.179055,0.495795,0.672929,-0.181588,0.501265,0.682034,-0.185520,0.495795,0.696159,-0.190104,0.479390,0.712629,-0.194595,0.452045,0.728764,-0.348912,0.452045,0.610779,-0.344178,0.479390,0.602719,-0.344913,0.495795,0.603969,-0.349706,0.501265,0.612129,-0.357146,0.495795,0.624799,-0.365822,0.479390,0.639574,-0.374320,0.452045,0.654044,-0.490211,0.452045,0.502149,-0.483597,0.479390,0.495533,-0.484623,0.495795,0.496560,-0.491321,0.501265,0.503259,-0.501715,0.495795,0.513654,-0.513835,0.479390,0.525774,-0.525710,0.452045,0.537649,-0.598845,0.452045,0.360848,-0.590785,0.479390,0.356114,-0.592035,0.495795,0.356849,-0.600195,0.501265,0.361642,-0.612865,0.495795,0.369082,-0.627635,0.479390,0.377758,-0.642105,0.452045,0.386256,-0.668585,0.452045,0.193103,-0.659595,0.479390,0.190601,-0.660990,0.495795,0.190990,-0.670095,0.501265,0.193523,-0.684220,0.495795,0.197454,-0.700690,0.479390,0.202039,-0.716825,0.452045,0.206530,-0.693215,0.452045,0.005136,-0.683900,0.479390,0.005136,-0.685345,0.495795,0.005136,-0.694775,0.501265,0.005136,-0.709420,0.495795,0.005136,-0.726490,0.479390,0.005136,-0.743215,0.452045,0.005136,-0.668585,0.452045,-0.182832,-0.659595,0.479390,-0.180330,-0.660990,0.495795,-0.180719,-0.670095,0.501265,-0.183252,-0.684220,0.495795,-0.187183,-0.700690,0.479390,-0.191767,-0.716825,0.452045,-0.196259,-0.598845,0.452045,-0.350577,-0.590785,0.479390,-0.345842,-0.592035,0.495795,-0.346578,-0.600195,0.501265,-0.351371,-0.612865,0.495795,-0.358811,-0.627635,0.479390,-0.367486,-0.642105,0.452045,-0.375985,-0.490213,0.452045,-0.491877,-0.483597,0.479390,-0.485262,-0.484625,0.495795,-0.486289,-0.491322,0.501265,-0.492986,-0.501720,0.495795,-0.503381,-0.513840,0.479390,-0.515501,-0.525715,0.452045,-0.527376,-0.348913,0.452045,-0.600511,-0.344179,0.479390,-0.592451,-0.344914,0.495795,-0.593701,-0.349708,0.501265,-0.601861,-0.357148,0.495795,-0.614531,-0.365823,0.479390,-0.629301,-0.374321,0.452045,-0.643771,-0.181170,0.452045,-0.670251,-0.178668,0.479390,-0.661261,-0.179056,0.495795,-0.662656,-0.181589,0.501265,-0.671761,-0.185521,0.495795,-0.685886,-0.190105,0.479390,-0.702356,-0.194596,0.452045,-0.718491,0.006797,0.452045,-0.694881,0.006797,0.479390,-0.685566,0.006797,0.495795,-0.687011,0.006797,0.501265,-0.696441,0.006797,0.495795,-0.711086,0.006797,0.479390,-0.728156,0.006797,0.452044,-0.744881,0.194763,0.452045,-0.670251,0.192261,0.479390,-0.661261,0.192649,0.495795,-0.662656,0.195183,0.501265,-0.671761,0.199114,0.495795,-0.685886,0.203699,0.479390,-0.702356,0.208190,0.452045,-0.718491,0.362507,0.452045,-0.600511,0.357772,0.479390,-0.592446,0.358508,0.495795,-0.593696,0.363301,0.501265,-0.601861,0.370741,0.495795,-0.614531,0.379416,0.479390,-0.629301,0.387915,0.452045,-0.643771,0.503805,0.452045,-0.491875,0.497191,0.479390,-0.485260,0.498218,0.495795,-0.486287,0.504915,0.501265,-0.492985,0.515310,0.495795,-0.503381,0.527430,0.479390,-0.515501,0.539305,0.452045,-0.527376,0.612440,0.452045,-0.350575,0.604380,0.479390,-0.345841,0.605630,0.495795,-0.346576,0.613790,0.501265,-0.351369,0.626460,0.495795,-0.358809,0.641230,0.479390,-0.367485,0.655700,0.452045,-0.375983,0.682180,0.452045,-0.182830,0.673190,0.479390,-0.180328,0.674585,0.495795,-0.180717,0.683690,0.501265,-0.183250,0.697815,0.495795,-0.187181,0.714285,0.479390,-0.191766,0.730420,0.452045,-0.196257,0.818735,0.320965,0.005138,0.790170,0.320965,0.223159,0.877185,0.190930,0.005138,0.846560,0.190930,0.238854,0.928690,0.062975,0.005138,0.896255,0.062980,0.252685,0.709270,0.320965,0.417724,0.759845,0.190930,0.447426,0.804410,0.062980,0.473598,0.935895,-0.061850,0.263718,0.839955,-0.061850,0.494478,0.962140,-0.182515,0.271022,0.863490,-0.182515,0.508300,0.971630,-0.297965,0.273664,0.872000,-0.297965,0.513300,0.583275,0.320965,0.581619,0.624775,0.190930,0.623119,0.661340,0.062975,0.659684,0.690515,-0.061850,0.688860,0.709825,-0.182515,0.708170,0.716815,-0.297965,0.715155,0.419383,0.320971,0.707614,0.449084,0.190931,0.758184,0.475257,0.062981,0.802754,0.496136,-0.061849,0.838300,0.509960,-0.182514,0.861835,0.514960,-0.297964,0.870345,0.224819,0.320971,0.788509,0.240514,0.190931,0.844904,0.254344,0.062976,0.894599,0.265377,-0.061849,0.934240,0.272682,-0.182514,0.960485,0.275323,-0.297964,0.969975,0.006799,0.320971,0.817079,0.006799,0.190931,0.875529,0.006799,0.062976,0.927034,0.006799,-0.061849,0.968125,0.006799,-0.182514,0.995320,0.006799,-0.297963,1.005165,-0.211222,0.320971,0.788509,-0.226916,0.190931,0.844904,-0.240746,0.062981,0.894599,-0.251779,-0.061849,0.934240,-0.259083,-0.182514,0.960485,-0.261724,-0.297963,0.969975,-0.405786,0.320971,0.707614,-0.435487,0.190931,0.758184,-0.461659,0.062981,0.802749,-0.482538,-0.061849,0.838300,-0.496361,-0.182509,0.861835,-0.501360,-0.297963,0.870345,-0.569680,0.320970,0.581614,-0.611180,0.190930,0.623114,-0.647745,0.062975,0.659684,-0.676920,-0.061850,0.688860,-0.696230,-0.182510,0.708170,-0.703215,-0.297963,0.715155,-0.695675,0.320970,0.417723,-0.746245,0.190930,0.447424,-0.790810,0.062980,0.473596,-0.826355,-0.061850,0.494476,-0.849895,-0.182510,0.508300,-0.858405,-0.297963,0.513300,-0.776570,0.320970,0.223157,-0.832965,0.190930,0.238852,-0.882660,0.062975,0.252683,-0.922300,-0.061850,0.263716,-0.948545,-0.182510,0.271020,-0.958035,-0.297963,0.273661,-0.863590,0.190930,0.005135,-0.915095,0.062975,0.005135,-0.956185,-0.061850,0.005136,-0.983380,-0.182510,0.005136,-0.832965,0.190930,-0.228581,-0.882660,0.062980,-0.242411,-0.922300,-0.061850,-0.253444,-0.948545,-0.182510,-0.260748,-0.776575,0.320970,-0.212886,-0.695675,0.320970,-0.407451,-0.746245,0.190930,-0.437153,-0.790815,0.062980,-0.463325,-0.826360,-0.061850,-0.484204,-0.849895,-0.182510,-0.498027,-0.958035,-0.297963,-0.263389,-0.858405,-0.297964,-0.503025,-0.569680,0.320970,-0.571346,-0.611180,0.190930,-0.612846,-0.647745,0.062975,-0.649411,-0.676920,-0.061850,-0.678585,-0.696230,-0.182510,-0.697895,-0.703215,-0.297964,-0.704880,-0.405787,0.320970,-0.697341,-0.435488,0.190930,-0.747911,-0.461661,0.062979,-0.792481,-0.482540,-0.061851,-0.828025,-0.496363,-0.182511,-0.851560,-0.501365,-0.297964,-0.860070,-0.211223,0.320970,-0.778241,-0.226919,0.190929,-0.834631,-0.240748,0.062974,-0.884326,-0.251781,-0.061851,-0.923965,-0.259085,-0.182516,-0.950210,-0.261726,-0.297965,-0.959700,0.006797,0.320970,-0.806806,0.006797,0.190929,-0.865256,0.006797,0.062974,-0.916761,0.006797,-0.061851,-0.957850,0.006797,-0.182516,-0.985050,0.006797,-0.297965,-0.994890,0.224817,0.320970,-0.778236,0.240512,0.190929,-0.834631,0.254342,0.062979,-0.884326,0.265375,-0.061851,-0.923965,0.272679,-0.182516,-0.950210,0.275321,-0.297965,-0.959700,0.419381,0.320970,-0.697341,0.449082,0.190930,-0.747911,0.475255,0.062979,-0.792476,0.496134,-0.061851,-0.828025,0.509955,-0.182516,-0.851560,0.514955,-0.297965,-0.860070,0.583275,0.320965,-0.571341,0.624775,0.190930,-0.612846,0.661340,0.062975,-0.649411,0.690515,-0.061850,-0.678585,0.709825,-0.182515,-0.697895,0.716810,-0.297965,-0.704880,0.709270,0.320965,-0.407450,0.759840,0.190930,-0.437151,0.804405,0.062980,-0.463323,0.839955,-0.061850,-0.484202,0.863490,-0.182515,-0.498025,0.872000,-0.297965,-0.503025,0.790165,0.320965,-0.212884,0.846560,0.190930,-0.228579,0.896255,0.062975,-0.242409,0.935895,-0.061850,-0.253442,0.962140,-0.182515,-0.260745,0.971630,-0.297965,-0.263387,0.881815,-0.555781,0.005138,0.942000,-0.486856,0.005138,0.909095,-0.486856,0.256259,0.851030,-0.555780,0.240098,0.821630,-0.609079,0.005138,0.792960,-0.609079,0.223937,0.775335,-0.647795,0.005138,0.748295,-0.647795,0.211505,0.756815,-0.672969,0.005138,0.730425,-0.672969,0.206532,0.953765,-0.401264,0.268691,0.855980,-0.401264,0.503890,0.815925,-0.486856,0.480363,0.763850,-0.555780,0.449779,0.711775,-0.609079,0.419195,0.671720,-0.647795,0.395669,0.655700,-0.672969,0.386258,0.703660,-0.401264,0.702005,0.670790,-0.486856,0.669135,0.628060,-0.555780,0.626405,0.585330,-0.609079,0.583675,0.552460,-0.647795,0.550800,0.539310,-0.672969,0.537650,0.505545,-0.401264,0.854325,0.482021,-0.486856,0.814270,0.451438,-0.555780,0.762190,0.420854,-0.609079,0.710120,0.397328,-0.647795,0.670065,0.387917,-0.672969,0.654045,0.270350,-0.401263,0.952110,0.257919,-0.486855,0.907440,0.241757,-0.555779,0.849375,0.225597,-0.609079,0.791300,0.213165,-0.647795,0.746635,0.208192,-0.672968,0.728765,0.006799,-0.401263,0.986640,0.006799,-0.486855,0.940345,0.006799,-0.555779,0.880160,0.006799,-0.609078,0.819975,0.006799,-0.647794,0.773675,0.006799,-0.672968,0.755155,-0.256752,-0.401263,0.952110,-0.244320,-0.486854,0.907440,-0.228159,-0.555779,0.849375,-0.211997,-0.609078,0.791300,-0.199566,-0.647794,0.746635,-0.194594,-0.672968,0.728765,-0.491948,-0.401263,0.854325,-0.468422,-0.486854,0.814265,-0.437838,-0.555779,0.762190,-0.407255,-0.609078,0.710120,-0.383729,-0.647794,0.670065,-0.374319,-0.672968,0.654045,-0.690065,-0.401263,0.702005,-0.657195,-0.486854,0.669135,-0.614465,-0.555779,0.626400,-0.571730,-0.609078,0.583670,-0.538860,-0.647794,0.550800,-0.525710,-0.672968,0.537650,-0.842380,-0.401263,0.503885,-0.802325,-0.486854,0.480361,-0.750250,-0.555779,0.449777,-0.698180,-0.609078,0.419193,-0.658125,-0.647794,0.395668,-0.642105,-0.672968,0.386257,-0.940170,-0.401263,0.268688,-0.895500,-0.486854,0.256257,-0.837430,-0.555779,0.240095,-0.779360,-0.609078,0.223935,-0.734695,-0.647794,0.211503,-0.716825,-0.672967,0.206530,-0.928405,-0.486854,0.005136,-0.868220,-0.555779,0.005136,-0.808030,-0.609078,0.005136,-0.761735,-0.647794,0.005136,-0.743215,-0.672967,0.005136,-0.895500,-0.486855,-0.245985,-0.837435,-0.555779,-0.229823,-0.779360,-0.609078,-0.213663,-0.734695,-0.647794,-0.201231,-0.716825,-0.672968,-0.196258,-0.940170,-0.401263,-0.258417,-0.842385,-0.401263,-0.493615,-0.802330,-0.486855,-0.470089,-0.750250,-0.555779,-0.439505,-0.698180,-0.609078,-0.408921,-0.658125,-0.647794,-0.385395,-0.642105,-0.672968,-0.375984,-0.690065,-0.401263,-0.691730,-0.657195,-0.486855,-0.658860,-0.614465,-0.555779,-0.616130,-0.571735,-0.609078,-0.573400,-0.538865,-0.647794,-0.540525,-0.525710,-0.672968,-0.527375,-0.491951,-0.401264,-0.844050,-0.468424,-0.486856,-0.803995,-0.437840,-0.555779,-0.751915,-0.407257,-0.609079,-0.699845,-0.383732,-0.647794,-0.659790,-0.374321,-0.672968,-0.643770,-0.256754,-0.401264,-0.941835,-0.244322,-0.486856,-0.897165,-0.228161,-0.555780,-0.839100,-0.212000,-0.609079,-0.781030,-0.199568,-0.647795,-0.736360,-0.194595,-0.672969,-0.718490,0.006797,-0.401265,-0.976370,0.006797,-0.486856,-0.930070,0.006797,-0.555780,-0.869885,0.006797,-0.609079,-0.809700,0.006797,-0.647795,-0.763400,0.006797,-0.672969,-0.744880,0.270348,-0.401265,-0.941835,0.257917,-0.486856,-0.897165,0.241756,-0.555781,-0.839100,0.225595,-0.609079,-0.781025,0.213163,-0.647795,-0.736360,0.208191,-0.672969,-0.718490,0.505545,-0.401265,-0.844050,0.482019,-0.486856,-0.803995,0.451436,-0.555780,-0.751915,0.420852,-0.609079,-0.699845,0.397326,-0.647795,-0.659790,0.387916,-0.672969,-0.643770,0.703660,-0.401265,-0.691730,0.670790,-0.486856,-0.658860,0.628060,-0.555780,-0.616130,0.585330,-0.609079,-0.573395,0.552460,-0.647795,-0.540525,0.539310,-0.672969,-0.527375,0.855980,-0.401265,-0.493612,0.815925,-0.486856,-0.470086,0.763845,-0.555781,-0.439503,0.711775,-0.609080,-0.408919,0.671720,-0.647795,-0.385393,0.655700,-0.672969,-0.375983,0.953765,-0.401265,-0.258414,0.909095,-0.486856,-0.245983,0.851030,-0.555781,-0.229821,0.792960,-0.609080,-0.213660,0.748290,-0.647796,-0.201229,0.730420,-0.672969,-0.196256,0.309756,-0.745018,0.005137,0.006799,-0.747970,0.005137,0.299096,-0.745018,-0.076213,0.517920,-0.736858,0.005138,0.499936,-0.736859,-0.132108,0.648995,-0.724532,0.005138,0.626400,-0.724532,-0.167306,0.720700,-0.709081,0.005138,0.695580,-0.709081,-0.186559,0.750740,-0.691546,0.005138,0.724560,-0.691546,-0.194625,0.268912,-0.745018,-0.148811,0.449012,-0.736859,-0.254589,0.562420,-0.724532,-0.321196,0.624455,-0.709081,-0.357633,0.650440,-0.691546,-0.372895,0.221898,-0.745018,-0.209964,0.369694,-0.736859,-0.357760,0.462759,-0.724532,-0.450826,0.513670,-0.709081,-0.501735,0.534995,-0.691546,-0.523060,0.160745,-0.745018,-0.256978,0.266523,-0.736859,-0.437079,0.333131,-0.724532,-0.550485,0.369566,-0.709080,-0.612525,0.384829,-0.691546,-0.638510,0.088148,-0.745018,-0.287162,0.144044,-0.736858,-0.488003,0.179240,-0.724532,-0.614470,0.198494,-0.709080,-0.683650,0.206559,-0.691546,-0.712630,0.006798,-0.745018,-0.297822,0.006798,-0.736858,-0.505985,0.006798,-0.724532,-0.637065,0.006797,-0.709080,-0.708770,0.006797,-0.691545,-0.738805,-0.074552,-0.745018,-0.287162,-0.130448,-0.736858,-0.488003,-0.165644,-0.724532,-0.614470,-0.184898,-0.709080,-0.683650,-0.192963,-0.691545,-0.712630,-0.147149,-0.745018,-0.256979,-0.252927,-0.736858,-0.437080,-0.319535,-0.724531,-0.550485,-0.355971,-0.709080,-0.612525,-0.371233,-0.691545,-0.638510,-0.208301,-0.745018,-0.209964,-0.356098,-0.736858,-0.357762,-0.449163,-0.724531,-0.450828,-0.500070,-0.709080,-0.501735,-0.521395,-0.691545,-0.523060,-0.255316,-0.745018,-0.148811,-0.435416,-0.736858,-0.254590,-0.548820,-0.724531,-0.321198,-0.610860,-0.709080,-0.357634,-0.636845,-0.691545,-0.372897,-0.285499,-0.745018,-0.076213,-0.486338,-0.736858,-0.132110,-0.612805,-0.724531,-0.167307,-0.681985,-0.709079,-0.186561,-0.710965,-0.691545,-0.194626,-0.296158,-0.745018,0.005137,-0.504320,-0.736857,0.005136,-0.635400,-0.724531,0.005136,-0.707105,-0.709079,0.005136,-0.737140,-0.691544,0.005136,-0.285499,-0.745017,0.086487,-0.486338,-0.736857,0.142383,-0.612805,-0.724531,0.177580,-0.681985,-0.709079,0.196833,-0.710965,-0.691544,0.204899,-0.255315,-0.745017,0.159085,-0.435416,-0.736858,0.264863,-0.548820,-0.724531,0.331472,-0.610860,-0.709079,0.367907,-0.636845,-0.691544,0.383169,-0.208301,-0.745017,0.220238,-0.356097,-0.736858,0.368035,-0.449163,-0.724531,0.461101,-0.500070,-0.709079,0.512010,-0.521395,-0.691544,0.533335,-0.147148,-0.745017,0.267253,-0.252926,-0.736858,0.447354,-0.319533,-0.724531,0.560760,-0.355969,-0.709079,0.622795,-0.371231,-0.691545,0.648785,-0.074551,-0.745017,0.297437,-0.130446,-0.736858,0.498277,-0.165643,-0.724531,0.624745,-0.184896,-0.709079,0.693925,-0.192962,-0.691545,0.722905,0.006799,-0.745018,0.308096,0.006799,-0.736858,0.516260,0.006799,-0.724531,0.647340,0.006799,-0.709080,0.719045,0.006799,-0.691545,0.749080,0.088148,-0.745018,0.297437,0.144045,-0.736858,0.498277,0.179241,-0.724531,0.624745,0.198496,-0.709080,0.693925,0.206561,-0.691545,0.722905,0.160746,-0.745018,0.267253,0.266524,-0.736858,0.447355,0.333132,-0.724532,0.560760,0.369568,-0.709080,0.622800,0.384830,-0.691545,0.648785,0.221898,-0.745018,0.220239,0.369695,-0.736858,0.368036,0.462761,-0.724532,0.461102,0.513670,-0.709080,0.512010,0.534995,-0.691545,0.533335,0.268913,-0.745018,0.159085,0.449014,-0.736858,0.264865,0.562420,-0.724532,0.331472,0.624455,-0.709080,0.367908,0.650440,-0.691546,0.383171,0.299096,-0.745018,0.086488,0.499936,-0.736858,0.142384,0.626400,-0.724532,0.177581,0.695580,-0.709080,0.196835,0.724560,-0.691546,0.204901,-0.956670,-0.419490,-0.057225,0.977196,-0.301304,0.226629,0.628770,0.452045,0.001636,0.607010,0.452045,0.167685,0.545405,0.452045,0.315871,0.449443,0.452045,0.440694,0.324623,0.452045,0.536659,0.176442,0.452045,0.598264,0.010397,0.452045,0.620029,-0.155648,0.452045,0.598264,-0.303831,0.452045,0.536659,-0.428650,0.452045,0.440691,-0.524610,0.452045,0.315867,-0.586220,0.452045,0.167681,-0.607980,0.452045,0.001631,-0.586220,0.452045,-0.164418,-0.524615,0.452045,-0.312604,-0.428652,0.452045,-0.437426,-0.303833,0.452045,-0.533391,-0.155652,0.452045,-0.595001,0.010392,0.452045,-0.616761,0.176436,0.452045,-0.595001,0.324618,0.452045,-0.533386,0.449437,0.452045,-0.437423,0.545400,0.452045,-0.312599,0.607010,0.452045,-0.164414,0.185873,0.540155,0.001634,0.110403,0.572105,0.001634,0.106884,0.572105,0.028489,0.179699,0.540155,0.048755,0.297457,0.516545,0.001635,0.287357,0.516545,0.078719,0.422931,0.497100,0.001635,0.408417,0.497100,0.112412,0.540075,0.477655,0.001635,0.521435,0.477655,0.143868,0.626655,0.454045,0.001636,0.604970,0.454045,0.167117,0.096920,0.572105,0.052455,0.162216,0.540155,0.090806,0.258758,0.516545,0.147510,0.367316,0.497100,0.211271,0.468664,0.477655,0.270798,0.543575,0.454045,0.314796,0.081401,0.572105,0.072642,0.134984,0.540155,0.126227,0.214210,0.516545,0.205455,0.303297,0.497100,0.294545,0.386467,0.477655,0.377717,0.447940,0.454045,0.439192,0.061214,0.572105,0.088163,0.099564,0.540155,0.153460,0.156266,0.516545,0.250004,0.220025,0.497100,0.358566,0.279550,0.477655,0.459917,0.323548,0.454045,0.534829,0.037249,0.572105,0.098127,0.057514,0.540155,0.170943,0.087477,0.516545,0.278605,0.121170,0.497100,0.399668,0.152625,0.477655,0.512689,0.175874,0.454045,0.596224,0.010394,0.572105,0.101646,0.010395,0.540155,0.177117,0.010395,0.516545,0.288705,0.010396,0.497100,0.414183,0.010397,0.477655,0.531324,0.010397,0.454045,0.617914,-0.016460,0.572105,0.098126,-0.036725,0.540155,0.170943,-0.066687,0.516545,0.278604,-0.100378,0.497100,0.399667,-0.131832,0.477655,0.512689,-0.155080,0.454045,0.596224,-0.040425,0.572105,0.088162,-0.078774,0.540155,0.153459,-0.135476,0.516545,0.250003,-0.199234,0.497100,0.358564,-0.258758,0.477655,0.459915,-0.302755,0.454045,0.534824,-0.060612,0.572105,0.072642,-0.114195,0.540155,0.126226,-0.193420,0.516545,0.205454,-0.282506,0.497100,0.294543,-0.365676,0.477655,0.377714,-0.427148,0.454045,0.439188,-0.076132,0.572105,0.052455,-0.141427,0.540155,0.090805,-0.237968,0.516545,0.147507,-0.346526,0.497100,0.211268,-0.447874,0.477655,0.270795,-0.522780,0.454045,0.314792,-0.086096,0.572105,0.028489,-0.158910,0.540155,0.048753,-0.266568,0.516545,0.078717,-0.387627,0.497100,0.112409,-0.500645,0.477655,0.143864,-0.584180,0.454045,0.167113,-0.089615,0.572105,0.001633,-0.165084,0.540155,0.001633,-0.276668,0.516545,0.001633,-0.402143,0.497100,0.001632,-0.519285,0.477655,0.001631,-0.605865,0.454045,0.001631,-0.086096,0.572105,-0.025222,-0.158910,0.540155,-0.045488,-0.266569,0.516545,-0.075452,-0.387628,0.497100,-0.109145,-0.500650,0.477655,-0.140601,-0.584180,0.454045,-0.163850,-0.076132,0.572105,-0.049188,-0.141428,0.540155,-0.087538,-0.237969,0.516545,-0.144243,-0.346527,0.497100,-0.208004,-0.447875,0.477655,-0.267531,-0.522785,0.454045,-0.311529,-0.060612,0.572105,-0.069375,-0.114196,0.540155,-0.122960,-0.193421,0.516545,-0.202188,-0.282508,0.497100,-0.291278,-0.365678,0.477655,-0.374450,-0.427151,0.454045,-0.435925,-0.040426,0.572105,-0.084895,-0.078776,0.540155,-0.150193,-0.135477,0.516545,-0.246736,-0.199237,0.497100,-0.355299,-0.258763,0.477655,-0.456650,-0.302759,0.454040,-0.531561,-0.016461,0.572105,-0.094859,-0.036725,0.540155,-0.167676,-0.066689,0.516545,-0.275338,-0.100381,0.497100,-0.396401,-0.131836,0.477655,-0.509421,-0.155085,0.454040,-0.592961,0.010394,0.572105,-0.098378,0.010394,0.540155,-0.173850,0.010393,0.516545,-0.285438,0.010393,0.497100,-0.410916,0.010393,0.477655,-0.528061,0.010392,0.454040,-0.614646,0.037248,0.572105,-0.094859,0.057513,0.540155,-0.167676,0.087475,0.516545,-0.275337,0.121166,0.497100,-0.396400,0.152620,0.477655,-0.509421,0.175869,0.454040,-0.592961,0.061213,0.572105,-0.084895,0.099563,0.540155,-0.150192,0.156264,0.516545,-0.246736,0.220023,0.497100,-0.355297,0.279548,0.477655,-0.456648,0.323543,0.454040,-0.531561,0.081400,0.572105,-0.069375,0.134983,0.540155,-0.122959,0.214208,0.516545,-0.202186,0.303295,0.497100,-0.291276,0.386463,0.477655,-0.374447,0.447937,0.454040,-0.435922,0.096920,0.572105,-0.049187,0.162216,0.540155,-0.087538,0.258756,0.516545,-0.144240,0.367314,0.497100,-0.208001,0.468662,0.477655,-0.267528,0.543570,0.454040,-0.311525,0.106884,0.572105,-0.025221,0.179698,0.540155,-0.045486,0.287356,0.516545,-0.075450,0.408416,0.497100,-0.109142,0.521435,0.477655,-0.140597,0.604970,0.454045,-0.163846,0.101773,0.597790,0.001634,0.099541,0.597780,0.018733,0.097930,0.599735,0.026012,0.089809,0.600477,0.044924,0.075533,0.601728,0.064055,0.056708,0.602448,0.078849,0.034144,0.602580,0.088296,0.008794,0.602075,0.091365,-0.016399,0.600928,0.087737,-0.038665,0.599268,0.078078,-0.057200,0.597255,0.063309,-0.069141,0.596180,0.048360,-0.073328,0.593830,0.040319,-0.079051,0.593840,0.026539,-0.081028,0.591445,0.017577,-0.083117,0.591440,0.001633,-0.082740,0.589205,-0.007009,-0.080561,0.589185,-0.023691,-0.078309,0.587215,-0.030832,-0.070176,0.586437,-0.049086,-0.055962,0.585182,-0.067445,-0.037572,0.584492,-0.081609,0.007194,0.584505,-0.093787,0.010394,0.585330,-0.093934,0.030672,0.585320,-0.091287,0.035937,0.586735,-0.090118,0.052092,0.586720,-0.083416,0.058412,0.588580,-0.080112,0.070786,0.588575,-0.070605,0.076964,0.590745,-0.064939,0.085922,0.590750,-0.053278,0.090819,0.593120,-0.045614,0.096508,0.593130,-0.031898,0.101392,0.595545,-0.007007,-0.015723,0.584400,-0.090724,0.099286,0.595545,-0.023119,0.146564,0.753670,-0.007006,0.007192,0.765130,-0.007007,0.141685,0.753670,0.030540,0.188700,0.723465,-0.007006,0.182344,0.723465,0.041889,0.169716,0.680750,-0.007006,0.164024,0.680755,0.036766,0.125728,0.631785,-0.007007,0.121573,0.631785,0.024901,0.127852,0.753670,0.063973,0.164329,0.723465,0.085428,0.147889,0.680755,0.075750,0.109798,0.631785,0.053328,0.106277,0.753670,0.092083,0.136230,0.723465,0.122036,0.122725,0.680755,0.108531,0.091436,0.631785,0.077240,0.078169,0.753670,0.113659,0.099624,0.723465,0.150137,0.089946,0.680755,0.133696,0.067525,0.631785,0.095602,0.044737,0.753670,0.127492,0.056086,0.723465,0.168153,0.050964,0.680755,0.149831,0.039099,0.631785,0.107379,0.007194,0.753670,0.132372,0.007194,0.723465,0.174509,0.007194,0.680755,0.155524,0.007194,0.631785,0.111533,-0.030351,0.753670,0.127492,-0.041699,0.723465,0.168153,-0.036577,0.680755,0.149831,-0.024712,0.631785,0.107378,-0.063782,0.753670,0.113658,-0.085236,0.723465,0.150136,-0.075559,0.680755,0.133695,-0.053138,0.631785,0.095602,-0.091891,0.753670,0.092082,-0.121844,0.723465,0.122035,-0.108338,0.680755,0.108529,-0.077050,0.631785,0.077239,-0.113467,0.753670,0.063971,-0.149943,0.723465,0.085427,-0.133502,0.680755,0.075748,-0.095412,0.631785,0.053327,-0.127299,0.753670,0.030539,-0.167959,0.723465,0.041887,-0.149638,0.680755,0.036765,-0.107187,0.631785,0.024900,-0.132179,0.753670,-0.007007,-0.174314,0.723465,-0.007008,-0.155330,0.680755,-0.007008,-0.111342,0.631785,-0.007008,-0.127300,0.753670,-0.044554,-0.167959,0.723465,-0.055903,-0.149638,0.680755,-0.050781,-0.107188,0.631785,-0.038915,-0.113467,0.753670,-0.077986,-0.149944,0.723465,-0.099442,-0.133503,0.680750,-0.089764,-0.095412,0.631785,-0.067343,-0.091892,0.753670,-0.106096,-0.121845,0.723465,-0.136051,-0.108340,0.680750,-0.122545,-0.077051,0.631785,-0.091255,-0.063783,0.753670,-0.127673,-0.085238,0.723465,-0.164151,-0.075560,0.680750,-0.147710,-0.053140,0.631785,-0.109617,-0.030352,0.753670,-0.141505,-0.041701,0.723465,-0.182167,-0.036578,0.680750,-0.163846,-0.024714,0.631785,-0.121393,0.007192,0.753670,-0.146385,0.007192,0.723465,-0.188523,0.007192,0.680750,-0.169538,0.007192,0.631785,-0.125548,0.044737,0.753670,-0.141505,0.056084,0.723465,-0.182167,0.050963,0.680750,-0.163845,0.039099,0.631785,-0.121393,0.078167,0.753670,-0.127672,0.099622,0.723465,-0.164150,0.089944,0.680750,-0.147709,0.067525,0.631785,-0.109616,0.106276,0.753670,-0.106095,0.136229,0.723465,-0.136049,0.122724,0.680750,-0.122544,0.091436,0.631785,-0.091254,0.127852,0.753670,-0.077985,0.164329,0.723465,-0.099440,0.147888,0.680750,-0.089763,0.109797,0.631785,-0.067342,0.141685,0.753670,-0.044552,0.182344,0.723465,-0.055901,0.164023,0.680750,-0.050779,0.121573,0.631785,-0.038914,0.967115,-0.212480,0.252557,0.975462,-0.378000,-0.124659,0.968446,-0.212076,-0.231835,-0.974724,-0.328037,0.105130,-0.967126,-0.361197,0.117749,-0.962988,-0.395476,0.102458,-0.960654,-0.427175,0.005343,-0.967270,-0.360816,-0.106910,-0.985147,-0.297940,-0.056185,-0.963310,-0.394371,-0.091242,-0.984843,-0.299288,0.067252};
static int teapot_coord_n = 1190; 
struct X3D_PolyRep * create_polyrep();
void compile_Teapot (struct X3D_Teapot *tnode){
	if(tnode->__ifsnode == NULL){
		if(teapotifs == NULL){
			teapotifs = createNewX3DNode0(NODE_IndexedFaceSet); //IIRC createnewX3DNode0 doesn't add to nodelist or garbage collection
			teapotifs->_intern = create_polyrep();
			teapot_coord = createNewX3DNode0(NODE_Coordinate);
			teapotifs->creaseAngle = PI;
			teapotifs->normalPerVertex = FALSE;
			teapotifs->ccw = TRUE;
			teapotifs->coord = X3D_NODE(teapot_coord);
			teapot_coord->point.p = (struct SFVec3f*)teapot_coord_p;
			teapot_coord->point.n = teapot_coord_n;
			teapotifs->coordIndex.p = teapot_coordindex_p;
			teapotifs->coordIndex.n = teapot_coordindex_n;
		}
		tnode->__ifsnode = teapotifs;
		make_IndexedFaceSet(tnode->__ifsnode);
	}
}
void rendray_Teapot (struct X3D_Teapot *node){
	if(node->__ifsnode == NULL) compile_Teapot(node);
	rendray_IndexedFaceSet(X3D_INDEXEDFACESET(node->__ifsnode));

}
void render_Teapot (struct X3D_Teapot *node){
	if(node->__ifsnode == NULL) compile_Teapot(node);
	render_IndexedFaceSet(X3D_INDEXEDFACESET(node->__ifsnode));
}
void collide_Teapot (struct X3D_Teapot *node){
	if(node->__ifsnode == NULL) compile_Teapot(node);
	collide_IndexedFaceSet(X3D_INDEXEDFACESET(node->__ifsnode));
}



void delete_glbuffers(struct X3D_Node *node){
	//polyrep done in delete_polyrep
	switch(node->_nodeType){
		case NODE_Cylinder:
			clear_Cylinder(node); break;
		case NODE_Cone:
			clear_Cone(node); break;
		case NODE_Sphere:
			clear_Sphere(node); break;
		default:
			break;
	}
}
