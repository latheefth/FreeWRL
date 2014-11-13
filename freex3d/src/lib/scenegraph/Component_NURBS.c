
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


/*******************************************************************

	X3D NURBS Component

*********************************************************************/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"

#include "Collision.h"
#include "LinearAlgebra.h"
#include "../opengl/Frustum.h"
#include "../opengl/Material.h"
#include "../opengl/Textures.h"
#include "Component_Geometry3D.h"
#include "../opengl/OpenGL_Utils.h"

#include "Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"

#include <float.h>
#if defined(_MSC_VER) && _MSC_VER < 1500
#define cosf cos
#define sinf sin
#endif
#ifdef NURBS_LIB
#include <libnurbs2.h>
void CALLBACK nurbsError(GLenum errorCode)
{
   const GLubyte *estring;

   //estring = gluErrorString(errorCode);
   //fprintf (stderr, "Nurbs Error: %s\n", estring);
   printf("ouch from nurbsError\n");
  // exit (0);
}
void CALLBACK nurbscurveBegincb(GLenum type, void *ud)
{
	struct X3D_NurbsCurve *node = (struct X3D_NurbsCurve *)ud;
	printf("nurbscurveBegin\n");
}
void CALLBACK nurbscurveVertexcb(GLfloat *vertex, void *ud)
{
	int i, np,ns;
	struct SFVec3f *pp;
	struct X3D_NurbsCurve *node = (struct X3D_NurbsCurve *)ud;
	ns = node->__points.n; 
	np = node->__numPoints;
	if(np+1 > ns) {
		ns = np *2;
		node->__points.p = realloc(node->__points.p,ns * sizeof(struct SFVec3f));
		node->__points.n = ns;
	}
	pp = &node->__points.p[np];
	for(i=0;i<3;i++)
		pp->c[i] = vertex[i];
	node->__numPoints ++;
	//node->__points.n++;
	printf("nurbscurveVertex\n");
}
void CALLBACK nurbscurveNormalcb(GLfloat *nml, void *ud)
{
	struct X3D_NurbsCurve *node = (struct X3D_NurbsCurve *)ud;
	printf("nurbscurveNormal\n");
}
void CALLBACK nurbscurveEndcb(void *ud)
{
	struct X3D_NurbsCurve *node = (struct X3D_NurbsCurve *)ud;
	//node->__numPoints = node->__points.n;
	printf("nurbscurveEnd\n");
}
#endif

int generateUniformKnotVector(int order, int ncontrol, float *knots){
	//caller: please malloc knots = malloc( (ncontrol + order ) * sizeof(float))
	int j,k,m, p = order -1;
	float uniform;
	m = ncontrol - p + 1;
	k = 0;
	uniform = 1.0f/(float)(ncontrol-p);
	for(j=0;j<p;k++,j++){
		knots[k] = 0.0f;
	}
	for(j=0;j<m;j++,k++){
		knots[k] =uniform*(float)j;
	}
	for(j=0;j<p;j++,k++){
		knots[k] = 1.0f;
	}
	return m;
}


void compile_NurbsCurve(struct X3D_NurbsCurve *node){
	MARK_NODE_COMPILED
#ifdef NURBS_LIB
	{
		int i,j, n, nk;
		GLfloat *xyzw, *knots;
		nk = n = 0;
		xyzw = knots = NULL;
		if(node->controlPoint){
			if(node->controlPoint->_nodeType == NODE_CoordinateDouble){
				struct Multi_Vec3d *mfd;
				mfd = &((struct X3D_CoordinateDouble *)(node->controlPoint))->point;
				n = mfd->n;
				xyzw = malloc(n * 4 * sizeof(GLfloat));
				for(i=0;i<mfd->n;i++){
					for(j=0;j<3;j++){
						xyzw[i*4 + j] = mfd->p[i].c[j];
					}
				}
			}else if(node->controlPoint->_nodeType == NODE_Coordinate){
				struct Multi_Vec3f *mff;
				mff = &((struct X3D_Coordinate *)(node->controlPoint))->point;
				n = mff->n;
				xyzw = malloc(n * 4 * sizeof(GLfloat));
				for(i=0;i<mff->n;i++){
					for(j=0;j<3;j++){
						xyzw[i*4 + j] = mff->p[i].c[j];
					}
				}
			}
		}else{
			n = 0;
		}
		if(node->weight.n && node->weight.n == n){
			double w;
			int m,im;
			m = min(node->weight.n, n);
			for(i=0;i<n;i++){
				im = i < m ? i : m-1;
				w = node->weight.p[im];
				xyzw[i*4 + 3] = w;
			}
		}else{
			for(i=0;i<n;i++) xyzw[i*4 + 3] = 1.0;
		}
		if(node->knot.n && node->knot.n == n + node->order ){
			nk = node->knot.n;
			knots = malloc(nk * sizeof(GLfloat));
			for(i=0;i<nk;i++){
				knots[i] = (GLfloat)node->knot.p[i];
			}
			//printf("good knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);

		}else{
			//generate uniform knot vector 
			nk = n + node->order ;
			//caller: please malloc knots = malloc( (ncontrol + order ) * sizeof(float))
			knots = malloc(nk *sizeof(GLfloat));
			generateUniformKnotVector(node->order,n, knots);
			//printf("bad knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);
			//nk = 0;
		}

		if(n && nk && nk >= n){
			GLUnurbsObj *theNurb;
			int ntess, mtess;
			mtess = node->order + 1;
			ntess = node->tessellation;
			theNurb = gluNewNurbsRenderer();
			gluNurbsProperty(theNurb, GLU_NURBS_MODE, GLU_NURBS_TESSELLATOR);
			gluNurbsCallbackData(theNurb,(GLvoid*)node);
			if(0){
				//chord length or automatic - not implemented properly nor tested thoroughly
				//if you do chord length in pixels, you need to manually pass in sampling matrices
				//and somehow you need to trigger a recompile: another call to this compile_
				// as you/avatar moves closer (father) from the nurbs node
				double model[16], proj[16];
				float modelf[16], projf[16];
				int viewPort[10];
				if(ntess > 0) 
					mtess = ntess;
				else if(ntess < 0) 
					mtess = -ntess;
				node->__points.p = malloc(sizeof(struct SFVec3f)*n*10); // just a guess to get started
				node->__points.n = n*10;  //.n will be used for realloc test in callbacks

				gluNurbsProperty(theNurb, GLU_SAMPLING_TOLERANCE, (float)(mtess)); //25.0);
				if(ntess < 0)
					gluNurbsProperty(theNurb,GLU_SAMPLING_METHOD,GLU_PATH_LENGTH); //pixels, the default
				else
					gluNurbsProperty(theNurb,GLU_SAMPLING_METHOD,GLU_PARAMETRIC_TOLERANCE);
				gluNurbsProperty(theNurb, GLU_AUTO_LOAD_MATRIX,GL_FALSE);
				
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, model);
				FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, proj);
				FW_GL_GETINTEGERV(GL_VIEWPORT, viewPort);
				for(i=0;i<16;i++){
					modelf[i] = (float)model[i];
					projf[i] = (float)proj[i];
				}
				gluLoadSamplingMatrices(theNurb,modelf,projf,viewPort);
			}
			if(1){
				//uniform spacing of sampling points in u (or uv) parameter space - works
				//node must specify tesselation value. see specs for interpretation 
				// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/nurbs.html#NurbsCurve
				if(ntess > 0) 
					mtess = max(mtess,ntess+1);
				else if(ntess < 0) 
					mtess = max(mtess,(-ntess * n) + 1);
				else
					mtess = max(mtess,2*n + 1);
				node->__points.p = malloc(sizeof(struct SFVec3f)*mtess+1);
				node->__points.n = mtess;  //.n will be used for realloc test in callbacks
				gluNurbsProperty(theNurb,GLU_SAMPLING_METHOD,GLU_DOMAIN_DISTANCE);
				gluNurbsProperty(theNurb,GLU_U_STEP,(GLfloat)mtess);
			}
			gluNurbsProperty(theNurb, GLU_DISPLAY_MODE, GLU_FILL);
			gluNurbsCallback(theNurb, GLU_ERROR, nurbsError);
			gluNurbsCallback(theNurb, GLU_NURBS_BEGIN_DATA, nurbscurveBegincb);
			gluNurbsCallback(theNurb, GLU_NURBS_VERTEX_DATA, nurbscurveVertexcb);
			gluNurbsCallback(theNurb, GLU_NURBS_NORMAL_DATA, nurbscurveNormalcb);
			gluNurbsCallback(theNurb, GLU_NURBS_END_DATA, nurbscurveEndcb);
			gluBeginCurve(theNurb);
				node->__numPoints = 0;
				gluNurbsCurve(theNurb,nk,knots,4,xyzw,node->order,GL_MAP1_VERTEX_4);
			gluEndCurve(theNurb);
			gluDeleteNurbsRenderer(theNurb);
			node->__points.n = node->__numPoints;
		}
	}
#endif
}
void render_NurbsCurve(struct X3D_NurbsCurve *node){
	ttglobal tg = gglobal();
	COMPILE_IF_REQUIRED
	// from Arc2D
	if (node->__numPoints>0) {	
		// for BoundingBox calculations
		setExtent( node->EXTENT_MAX_X, node->EXTENT_MIN_X, 
			node->EXTENT_MAX_Y, node->EXTENT_MIN_Y, 0.0f,0.0f,X3D_NODE(node));

		//OLDCODE GET_COLOUR_POINTER
	        LIGHTING_OFF
	        DISABLE_CULL_FACE
		//OLDCODE DO_COLOUR_POINTER

		FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(GLfloat *)node->__points.p);
        	sendArraysToGPU (GL_LINE_STRIP, 0, node->__numPoints);
		tg->Mainloop.trisThisLoop += node->__numPoints;
	}


}