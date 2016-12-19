
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
#include "Component_Geometry3D.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"

#include "Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"
#include "../vrml_parser/CRoutes.h"

#include <float.h>
#if defined(_MSC_VER) && _MSC_VER < 1500
#define cosf cos
#define sinf sin
#endif

typedef struct pComponent_NURBS{
	void *nada;// = 0;

}* ppComponent_NURBS;
void *Component_NURBS_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_NURBS));
	memset(v,0,sizeof(struct pComponent_NURBS));
	return v;
}
void Component_NURBS_init(struct tComponent_NURBS *t){
	//public
	//private
	t->prv = Component_NURBS_constructor();
	{
		ppComponent_NURBS p = (ppComponent_NURBS)t->prv;
		p->nada = NULL;
	}
}
//ppComponent_NURBS p = (ppComponent_NURBS)gglobal()->Component_NURBS.prv;

/*
Specs:
http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/nurbs.html
Examples:
http://www.web3d.org/x3d/content/examples/Basic/NURBS/

Book: The NURBS Book
Piegl, Les and Tiller, Wayne; The NURBS Book, 2nd Edition, Springer-Verlag (Berlin), 1997, ISBN:  3-540-61545-8.
- about $120 new, softcover or kindle
- some university libraries have it, you don't absolutely need it, the swept and swung ideas are in it

Nov 2014 - dug9 did some nurbs using libnurbs2, and following The Nurbs Book
	- didn't finish the component
	- personal notes say Nurbs Level 1 done
July 2016
Not sure what's done and what's not, nothing indicaded on Conformace page: 
http://freewrl.sourceforge.net/conformance.html
												code
Level 1						Conformance page	review 
CoordinateDouble			(not mentioned)		DONE
NurbsCurve					Not Implemented		DONE
NurbsOrientationInterpolatorNot Implemented		stub
NurbsPatchSurface			Not Implemented		DONE
NurbsPositionInterpolator	Not Implemented		stub
NurbsSurfaceInterpolator	Not Implemented		stub
NurbsTextureCoordinate		Not Implemented		-
Level 2
NurbsSet					Not Implemented		-
Level 3
NurbsCurve2D				Not Implemented		DONE
ContourPolyline2D			Not Implemented		DONE
NurbsSweptSurface			Not Implemented		-
NurbsSwungSurface			Not Implemented		-
Level 4
Contour2D					Not Implemented		DONE
NurbsTrimmedSurface			Not Implemented		DONE?

Perl: all the above including CoordinateDouble are in perl

Analysis 
dug9, 2016: 
Chapter 12 of Redbook has lots of glu nurbs functions.
freewrl doesn't use opengl glu. 
There is some source for it:
	http://mesa3d.org/
	ftp://ftp.freedesktop.org/pub/mesa/glu/
	http://oss.sgi.com/projects/ogl-sample/
So we're left to re-implement the hard way.
I find nurbs libs are always disappointing in documentation. 
I think that's because there's not a lot to nurbs, mostly plumbing and little meat: 
- Each tesselation point Ci is computed as a weighted blend of control points: Ci = f(Pn,wn)
- There are a few blending functions: linear, quadratic, cubic - in theory could go higher
- there's a trick for doing sharp corners: knots (repeating a control point)
- hard part: adaptive triangulation, might need callback plumbing, libnurbs2 has some of that
Did I make a mistake trying to use libnurbs in 2014? Is it too much lib plumbing for too little meat?
Here's a smaller MIT .c
https://github.com/retuxx/tinyspline
Or you can just read a bit more and manually implement nurbs.


HARD PARTS:
1. nurbsSet:
	Q1.how smooth between nurbs 'tiles'? C0, C1, C2 continuity?
	Q1a. how to find matching/joining/adjacent edges from 2+ patches?
	Q1b. how to use adjoinging patch info to modify/blend on/near/in-overalap of the join?
	
2. trimmed surfaces - procedure? 
	Hint: however we triangulate font glyphs in Component_Text would be a good example,
	- might be able to borrow code from it
	Terminology:
	  Tesselate: Nurbs term for interpolate/blend a given target interpolation point
	  Triangulate: take cloud of points, and join them with edges to make surface triangles

	Fuzzy guess algo:
	a) tesselate 2D curve in uv space of surface => A2dtess
	b) tesselate 3D surface tesselation grid by itself => Btess 
		[option skip points inside A2dtess here instead of c)]
	c) use point-in-poly test to remove Btess surface tesselation points 
		inside A2dtess tesselated curve: Btess - inside(A2dtess) => Ctess
	d) go over A2dtess 2D curve point by point, interpolating/blending/tesselating in 3D like they 
		were surface tesselation points A2dtess => Dtess
	e) add Dtess points to Ctess points => Etess
	f) triangulate Etess => Etris
	g) follow A2dtess points in Etris, swapping triangles so triangle edges are along A2dtess
	h) do point-in-poly of Etris 2D triangle centroids, using A2dtess polygon, 
		to remove triangles inside A2dtess => Ftris
	Ftris should be a trimmed surface
	
3. Adaptive triangulation
	a regular grid in uv space does not transform into a uniform grid in xyz space
	with nurbs. And so there's a lot of plumbing in glu and other libs for 
	refining the xyz grid a) in xyz space or b) in screenspace (pixels), using some
	measure of error and/or chordlength.

	http://people.scs.carleton.ca/~c_shu/Publications/stmalo99.pdf
	- Triangulating Trimmed NURBS Surfaces
	http://www.saccade.com/writing/graphics/RE-PARAM.PDF
	- Arc Length Parameterization of Spline Curves
	- uses yet-another nurbs curve to interpolate XY space regular length chords


*/



#ifdef NURBS_LIB
#include <libnurbs2.h>
static int DEBG = 0; //glu nurbs surface and trim calls
static int DEBGC = 0; //curve calls

void CALLBACK nurbsError(GLenum errorCode)
{
   const GLubyte *estring;

   //estring = gluErrorString(errorCode);
   //fprintf (stderr, "Nurbs Error: %s\n", estring);
   printf("ouch from nurbsError\n");
  // exit (0);
}

//curve
void CALLBACK nurbscurveBegincb(GLenum type, void *ud)
{
	struct X3D_NurbsCurve *node = (struct X3D_NurbsCurve *)ud;
	if(DEBGC) printf("nurbscurveBegin\n");
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
		node->__points.p = REALLOC(node->__points.p,ns * sizeof(struct SFVec3f));
		node->__points.n = ns;
	}
	pp = &node->__points.p[np];
	for(i=0;i<3;i++)
		pp->c[i] = vertex[i];
	node->__numPoints ++;
	//node->__points.n++;
	if(DEBGC) printf("nurbscurveVertex\n");
}
void CALLBACK nurbscurveNormalcb(GLfloat *nml, void *ud)
{
	struct X3D_NurbsCurve *node = (struct X3D_NurbsCurve *)ud;
	if(DEBGC) printf("nurbscurveNormal\n");
}
void CALLBACK nurbscurveEndcb(void *ud)
{
	struct X3D_NurbsCurve *node = (struct X3D_NurbsCurve *)ud;
	//node->__numPoints = node->__points.n;
	if(DEBGC) printf("nurbscurveEnd\n");
}

#endif

int generateUniformKnotVector(int order, int ncontrol, float *knots){
	//produced pinned uniform knot vector
	//caller: please malloc knots = malloc( (ncontrol + order ) * sizeof(float))
	// http://www.saccade.com/writing/graphics/KnotVectors.pdf
	//maximum nuber of equalvalue consecutive knots:
	// a) in middle of knot vector: <= order-1
	// b) at start and end of knot vector: <= order (for pinned uniform)
	// exmple order = 4 + ncontrol = 6 => 10 knots
	// 0 0 0 0 .33 .66 1 1 1 1
	// example order = 3 + ncontrol = 3 => 6 knots
	// 0 0 0 1 1 1
	//number of knots == ncontrol + order
	int j,k,m;
	float uniform;
	m = ncontrol - order;
	k = 0;
	uniform = 1.0f/(float)(m + 1);
	for(j=0;j<order;k++,j++){
		knots[k] = 0.0f;
	}
	for(j=0;j<m;j++,k++){
		knots[k] =uniform*(float)(j+1);
	}
	for(j=0;j<order;j++,k++){
		knots[k] = 1.0f;
	}
	return m;
}
int knotsOK(int order, int ncontrol, int nknots, double *knots){
	int ok = TRUE;

	if(nknots < 2 || nknots != ncontrol + order ) 
		ok = FALSE;
	if(ok){
		int nconsec = 1;
		double lastval = knots[0];
		for(int i=1;i<nknots;i++){
			if(lastval == knots[i]) nconsec++;
			else nconsec = 1;
			if(nconsec > order) 
				ok = false;
			if(knots[i] < lastval) 
				ok = false;
			if(!ok) break;
			lastval = knots[i];
		}
	}
	return ok;
}
void compile_ContourPolyline2D(struct X3D_ContourPolyline2D *node){
	MARK_NODE_COMPILED;
	if(node->point.n && !node->controlPoint.n){
		//version v3.0 had a mfvec2f point field, version 3.1+ changed to mfvec2d controlPoint field
		node->controlPoint.p = MALLOC(struct SFVec2d*,node->point.n * sizeof(struct SFVec2d));
		for(int i=0;i<node->point.n;i++)
			float2double(node->controlPoint.p[i].c,node->point.p[i].c,2);
	}
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
				xyzw = MALLOC(void *, n * 4 * sizeof(GLfloat));
				for(i=0;i<mfd->n;i++){
					for(j=0;j<3;j++){
						xyzw[i*4 + j] = mfd->p[i].c[j];
					}
				}
			}else if(node->controlPoint->_nodeType == NODE_Coordinate){
				struct Multi_Vec3f *mff;
				mff = &((struct X3D_Coordinate *)(node->controlPoint))->point;
				n = mff->n;
				xyzw = MALLOC(void *, n * 4 * sizeof(GLfloat));
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
		//if(node->knot.n && node->knot.n == n + node->order ){
		if(knotsOK(node->order,n,node->knot.n,node->knot.p)){

			nk = node->knot.n;
			knots = MALLOC(void *, nk * sizeof(GLfloat));
			for(i=0;i<nk;i++){
				knots[i] = (GLfloat)node->knot.p[i];
			}
			//printf("good knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);

		}else{
			static int once = 0;
			//generate uniform knot vector 
			nk = n + node->order ;
			//caller: please malloc knots = malloc( (ncontrol + order ) * sizeof(float))
			knots = MALLOC(void *, nk *sizeof(GLfloat));
			generateUniformKnotVector(node->order,n, knots);
			if(!once){
				printf("bad knot vector, replacing with:\n");
				for(int ii=0;ii<nk;ii++)
					printf("[%d]=%f \n",ii,knots[ii]);
				once = 1;
			}
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
				// as avatar/viewer moves closer (father) from the nurbs node
				double model[16], proj[16];
				float modelf[16], projf[16];
				int viewPort[10];
				if(ntess > 0) 
					mtess = ntess;
				else if(ntess < 0) 
					mtess = -ntess;
				node->__points.p = MALLOC(void *, sizeof(struct SFVec3f)*n*10); // just a guess to get started
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
				node->__points.p = MALLOC(void *, sizeof(struct SFVec3f)*mtess+1);
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



/* GenPolyrep functions assume a node inherits from X3DGeometryNode.
	NurbsPatchSurface inherits from X3DParametricGeometryNode.
	So we can't use all the genpolyrep stuff, until we have our node compiled into one.
	Then we can delegate back to generic polyrep functions
render - can delegate to polyrep
rendray - can delegate to polyrep
(x make - we will do a compile instead)
collide - can delegate to polyrep
compile - custom: we will convert our parametric surface to a polyrep in here

 */

 //stripstate - used for capturing the callback data when using gluNurbs in TESSELATOR mode
 //we use TESSELATOR mode instead of RENDER mode because we use GLES2, not desktop GL.
 //the libnurbs we have has the GL rendering stuff ifdefed out.
 //so we capture the data in the callbacks, and then we can do what we normally do with
 //mesh data in GLES2

 struct stripState{
	int type;
	struct Vector pv; //vector of vertex points
	struct Vector nv; //vector of normals
	struct Vector tv; //vector of texcoords
};

 #ifdef NURBS_LIB
//surface
// strategy: on begin, we'll store the gl_ type, and zero __points
// then accumulate the __points
// and on end, convert the __points to ->_intern->polyrep points and triangle indices
//  using the stored type as a guide
void CALLBACK nurbssurfBegincb(GLenum type, void *ud)
{
	struct stripState ss;
	struct Vector * strips = (struct Vector *)ud;
	if(0) if(DEBG) printf("callback nurbsSurfaceBegin\n");
	if(0){
		printf("nurbssurfBegin type = ");
		switch(type){
			case GL_QUAD_STRIP: printf("QUAD_STRIP");break;
			case GL_TRIANGLE_STRIP: printf("TRIANGLE_STRIP");break;
			case GL_TRIANGLE_FAN: printf("TRIANGLE_FAN");break;
			case GL_TRIANGLES: printf("TRIANGLES");break;
			default:
				printf("not sure %x %d",type,type);
		}
		printf("\n");
	}
	ss.nv.n = 0;
	ss.nv.allocn = 0;
	ss.nv.data = NULL;
	ss.tv.n = 0;
	ss.tv.allocn = 0;
	ss.tv.data = NULL;
	ss.pv.n = 0;
	ss.pv.allocn = 0;
	ss.pv.data = NULL;
	ss.type = type;
	vector_pushBack(struct stripState,strips,ss);
}
void CALLBACK nurbssurfVertexcb(GLfloat *vertex, void *ud)
{
	struct stripState ss;
	struct SFVec3f pp;
	struct Vector * strips = (struct Vector *)ud;
	ss = vector_get(struct stripState,strips,strips->n -1);
	memcpy(&pp,vertex,sizeof(struct SFVec3f));
	vector_pushBack(struct SFVec3f,&ss.pv,pp);
	vector_set(struct stripState,strips,strips->n-1,ss);

	if(0) printf("callback nurbssurfVertex %f %f %f\n",vertex[0],vertex[1],vertex[2]);
}
void CALLBACK nurbssurfNormalcb(GLfloat *nml, void *ud)
{
	struct stripState ss;
	struct SFVec3f pp;
	struct Vector * strips = (struct Vector *)ud;
	ss = vector_get(struct stripState,strips,strips->n -1);
	memcpy(&pp,nml,sizeof(struct SFVec3f));
	vector_pushBack(struct SFVec3f,&ss.nv,pp);
	vector_set(struct stripState,strips,strips->n-1,ss);

	if(0) printf("callback nurbssurfNormal\n");
}
void CALLBACK nurbssurfEndcb(void *ud)
{
	struct stripState ss;
	struct Vector * strips = (struct Vector *)ud;
	ss = vector_get(struct stripState,strips,strips->n -1);
	if(0){
		printf("nurbssurfEnd #p %d #n %d\n",ss.pv.n, ss.nv.n);
		for(int i=0;i<ss.pv.n;i++){
			struct SFVec3f pp = vector_get(struct SFVec3f,&ss.pv,i);
			printf("%f %f %f\n",pp.c[0],pp.c[1],pp.c[2]);
		}
	}
	if(0) if(DEBG) printf("callback nurbsSurfaceEnd\n");

}
void CALLBACK nurbssurfTexcoordcb(GLfloat *tCrd, void *ud){
	struct stripState ss;
	struct SFVec2f tp;
	struct Vector * strips = (struct Vector *)ud;
	ss = vector_get(struct stripState,strips,strips->n -1);
	memcpy(&tp,tCrd,sizeof(struct SFVec2f));
	vector_pushBack(struct SFVec2f,&ss.tv,tp);
	vector_set(struct stripState,strips,strips->n-1,ss);

	if(0) if(DEBG) 
		printf("callback nurbssufTexcoordcb\n");
}
#endif

void free_polyrep(struct X3D_PolyRep *rep){
	//see also delete_polyrep - did dug9 duplicate the function or is it different?
	if(rep){
		rep->ntri = 0;
		rep->transparency = 0;
		//Q. are any of these added to GC tables? If not..
		glDeleteBuffers(VBO_COUNT, rep->VBO_buffers);
		FREE_IF_NZ(rep->actualCoord);
		FREE_IF_NZ(rep->cindex);
		FREE_IF_NZ(rep->colindex);
		FREE_IF_NZ(rep->GeneratedTexCoords[0]);
		FREE_IF_NZ(rep->norindex);
		FREE_IF_NZ(rep->normal);
		FREE_IF_NZ(rep->tcindex);
		FREE_IF_NZ(rep);
	}
}
struct X3D_PolyRep * create_polyrep(){
	int i;
	struct X3D_PolyRep *polyrep;

	polyrep = MALLOC(struct X3D_PolyRep *, sizeof(struct X3D_PolyRep));
	memset(polyrep,0,sizeof(struct X3D_PolyRep));
	polyrep->ntri = -1;
	//polyrep->cindex = 0; polyrep->actualCoord = 0; polyrep->colindex = 0; polyrep->color = 0;
	//polyrep->norindex = 0; polyrep->normal = 0; polyrep->flat_normal = 0; polyrep->GeneratedTexCoords = 0;
	//polyrep->tri_indices = 0; polyrep->wire_indices = 0; polyrep->actualFog = 0;
	//polyrep->tcindex = 0; 
	//polyrep->tcoordtype = 0;
	//polyrep->last_index_type = 0; polyrep->last_normal_type = 0;
	polyrep->streamed = FALSE;

	/* for Collision, default texture generation */
	polyrep->minVals[0] =  999999.9f;
	polyrep->minVals[1] =  999999.9f;
	polyrep->minVals[2] =  999999.9f;
	polyrep->maxVals[0] =  -999999.9f;
	polyrep->maxVals[1] =  -999999.9f;
	polyrep->maxVals[2] =  -999999.9f;

	for (i=0; i<VBO_COUNT; i++) 
		polyrep->VBO_buffers[i] = 0;

	/* printf ("generating buffers for node %p, type %s\n",p,stringNodeType(p->_nodeType)); */
	glGenBuffers(1,&polyrep->VBO_buffers[VERTEX_VBO]);
	glGenBuffers(1,&polyrep->VBO_buffers[INDEX_VBO]);
	//glGenBuffers(1,&polyrep->VBO_buffers[NORMAL_VBO]);
	//glGenBuffers(1,&polyrep->VBO_buffers[TEXTURE_VBO0+0]);



	/* printf ("they are %u %u %u %u\n",polyrep->VBO_buffers[0],polyrep->VBO_buffers[1],polyrep->VBO_buffers[2],polyrep->VBO_buffers[3]); */
	return polyrep;
}
#ifndef NURBS_LIB
#define GL_QUAD_STRIP				0x0008
#endif
static int USETXCOORD = 1;
void convert_strips_to_polyrep(struct Vector * strips,struct X3D_NurbsTrimmedSurface *node){
	//this is a bit like compile_polyrep, except the virt_make is below

	int i, j, npoints, np, ni, ntri, nindex, ntc;
	struct stripState ss;
	struct X3D_PolyRep *rep_, *polyrep;
	struct X3D_TextureCoordinate * tcnode = NULL;
	GLuint *cindex, *norindex, *tcindex;
	float *tcoord;

	//from compile_polyrep:
	//node = X3D_NODE(innode);
	//virt = virtTable[node->_nodeType];

	/* first time through; make the intern structure for this polyrep node */
	if(node->_intern){
		polyrep = node->_intern;
		FREE_IF_NZ(polyrep->cindex);
		FREE_IF_NZ(polyrep->actualCoord);
		FREE_IF_NZ(polyrep->GeneratedTexCoords[0]);
		FREE_IF_NZ(polyrep->colindex);
		FREE_IF_NZ(polyrep->color);
		FREE_IF_NZ(polyrep->norindex);
		FREE_IF_NZ(polyrep->normal);
		FREE_IF_NZ(polyrep->flat_normal);
		FREE_IF_NZ(polyrep->tcindex);
	}
	if(!node->_intern) 
		node->_intern = create_polyrep();

	rep_ = polyrep = node->_intern;


	/* if multithreading, tell the rendering loop that we are regenning this one */
	/* if singlethreading, this'll be set to TRUE before it is tested	     */

	//<< END FROM Compile_polyrep

	// Start Virt_make_polyrep section >>>
	//texcoord
	if(USETXCOORD){
		rep_->ntexdim[0] = 2;
		rep_->tcoordtype = NODE_TextureCoordinate; //??
		rep_->ntcoord = 1;
		tcnode =  createNewX3DNode(NODE_TextureCoordinate);
	}

	npoints = nindex = ntc = 0;
	for(i=0;i<strips->n;i++){
		ss = vector_get(struct stripState,strips,i);
		npoints += ss.pv.n;
		ntc += ss.tv.n;
		switch(ss.type){
			case GL_QUAD_STRIP: nindex += (ss.pv.n -2)/2 * 5;break;
			case GL_TRIANGLE_STRIP: nindex += (ss.pv.n -2);break;
			case GL_TRIANGLE_FAN: nindex += (ss.pv.n -2);break;
			case GL_TRIANGLES: nindex += (ss.pv.n -2);break;
			default:
				nindex += (ss.pv.n -2);
		}
	}
    if (npoints > 0)
    {
		printf("npoints %d ntc %d\n",npoints,ntc);
        rep_->actualCoord = MALLOC(void *, npoints * 3 * sizeof(float));
        rep_->normal = MALLOC(void *, npoints * 3 * sizeof(float));
		//if(USETXCOORD) rep->GeneratedTexCoords[0] = MALLOC(void *, npoints * 2 * sizeof(float));
		if(USETXCOORD){
			 tcnode->point.p = MALLOC(void *, npoints * 2 * sizeof(float));
			 tcnode->point.n = npoints;
		}
		//rep->t
    }
	rep_->ntri = ntri = nindex; //we'll over-malloc

    if (rep_->ntri > 0)
    {
		cindex = rep_->cindex = MALLOC(GLuint *, sizeof(GLuint)*3*(ntri));
		norindex = rep_->norindex = MALLOC(GLuint *,sizeof(GLuint)*3*ntri);
		//if(USETXCOORD) rep_->tcindex = MALLOC(void *, ntri * 4 * sizeof(GLuint));
		tcindex = rep_->tcindex = MALLOC(GLuint*, sizeof(GLuint)*3*(ntri));
	//	colindex = rep_->colindex = MALLOC(GLuint *, sizeof(*(rep_->colindex))*3*(ntri));

		FREE_IF_NZ(rep_->GeneratedTexCoords[0]);
		tcoord = rep_->GeneratedTexCoords[0] = MALLOC (float *, sizeof (float) * ntri * 2 * 3); 

    }

	np = 0;
	ni = 0;
	ntri = 0;
	for(i=0;i<strips->n;i++){
		ss = vector_get(struct stripState,strips,i);
		printf("ss.pv.n=%d nv.n=%d tv.n=%d\n",ss.pv.n,ss.nv.n,ss.tv.n);
		memcpy(&rep_->actualCoord[np*3],ss.pv.data,ss.pv.n * 3 * sizeof(float));
		memcpy(&rep_->normal[np*3],ss.nv.data,ss.nv.n * 3 * sizeof(float));
		if(USETXCOORD) memcpy(&tcoord[np*2],ss.tv.data,ss.tv.n * 2 * sizeof(float));
		switch(ss.type){
			case GL_QUAD_STRIP: 
				for(j=0;j<ss.pv.n -2;j+=2){
					rep_->cindex[ni++] = np+j;
					rep_->cindex[ni++] = np+j+1;
					rep_->cindex[ni++] = np+j+3;
					//rep->cindex[ni++] = -1;
					rep_->cindex[ni++] = np+j+3;
					rep_->cindex[ni++] = np+j+2;
					rep_->cindex[ni++] = np+j;
					//rep->cindex[ni++] = -1;
					//memcpy(&rep->norindex[ntri*4],&rep->cindex[ntri*4],2*4*sizeof(int));
					memcpy(&rep_->norindex[ntri*3],&rep_->cindex[ntri*3],2*3*sizeof(int));
					if(USETXCOORD) memcpy(&rep_->tcindex[ntri*3],&rep_->cindex[ntri*3],2*3*sizeof(int));
					ntri += 2;
				}
				break;
			case GL_TRIANGLE_STRIP: 
				nindex += (ss.pv.n -2);
				break;
			case GL_TRIANGLE_FAN: 
				//nindex += (ss.pv.n -2);
				for(j=0;j<ss.pv.n -2;j+=1){
					rep_->cindex[ni++] = np;
					rep_->cindex[ni++] = np+j+1;
					rep_->cindex[ni++] = np+j+2;
					memcpy(&rep_->norindex[ntri*3],&rep_->cindex[ntri*3],3*sizeof(int));
					if(USETXCOORD) memcpy(&rep_->tcindex[ntri*3],&rep_->cindex[ntri*3],3*sizeof(int));
					ntri += 1;
				}
				break;
			case GL_TRIANGLES: 
				nindex += (ss.pv.n -2);
				break;
			default:
				nindex += (ss.pv.n -2);
		}
		np += ss.pv.n;
	}
	rep_->ntri = ntri;
	tcnode->point.p = (struct SFVec2f*)tcoord;
	tcnode->point.n = np;
	if(0) for(i=0;i<tcnode->point.n;i++){
		printf("%d %f %f\n",i,tcnode->point.p[i].c[0],tcnode->point.p[i].c[1]);
		if(i % 50 == 0)
			printf("\n");
	}

	//END virt_make_polyrep section <<<<<<

	//FROM Compile_polyrep
	if (polyrep->ntri != 0) {
		//float *fogCoord = NULL;
		stream_polyrep(node, NULL,NULL,NULL,NULL, tcnode);
		/* and, tell the rendering process that this shape is now compiled */
	}
	//else wait for set_coordIndex to be converted to coordIndex
	polyrep->irep_change = node->_change;

	/*
	// dump then can copy and paste to x3d or wrl IndexedFaceSet.coordIndex and Coordinate.point fields
	FILE * fp = fopen("IFS_DUMP.txt","w+");
	fprintf(fp,"#vertices %d\n",np);
	for(i=0;i<np;i++){
		fprintf(fp,"%f %f %f\n",rep->actualCoord[i*3 +0],rep->actualCoord[i*3 +1],rep->actualCoord[i*3 +2]);
	}
	fprintf(fp,"#face indices %d\n",ni);
	for(i=0;i<ni;i++){
		fprintf(fp,"%d ",rep->cindex[i]);
		if((ni+1) % 3 == 0)
			fprintf(fp,"%d ",-1);
	}
	fprintf(fp,"\n");
	fclose(fp);
	*/

}

void compile_NurbsSurface(struct X3D_NurbsPatchSurface *node, struct Multi_Node *trim){
	MARK_NODE_COMPILED
#ifdef NURBS_LIB
	{
		int i,j, n, nu, nv, nku, nkv;
		GLfloat *xyzw, *knotsu, *knotsv;
		ppComponent_NURBS p = (ppComponent_NURBS)gglobal()->Component_NURBS.prv;

		nku = nkv = nu = nv = n = 0;
		xyzw = knotsu = knotsv = NULL;
		// I should call something like:
		// struct Multi_Vec3f *getCoordinate (struct X3D_Node *innode, char *str);
		// to get the control points - it will do proto expansion, compile the coordinate node if needed
		// (should do something similar with texcoord when implemented, 
		//    as I think it needs conversion from controlpoint spacing to sampling/tesselation spacing for use in polyrep)
		// here's an amature shortcut that returns doubles
		if(node->controlPoint){
			if(node->controlPoint->_nodeType == NODE_CoordinateDouble){
				struct Multi_Vec3d *mfd;
				mfd = &((struct X3D_CoordinateDouble *)(node->controlPoint))->point;
				n = mfd->n;
				xyzw = MALLOC(void *, n * 4 * sizeof(GLfloat));
				for(i=0;i<mfd->n;i++){
					for(j=0;j<3;j++){
						xyzw[i*4 + j] = mfd->p[i].c[j];
					}
				}
			}else if(node->controlPoint->_nodeType == NODE_Coordinate){
				struct Multi_Vec3f *mff;
				mff = &((struct X3D_Coordinate *)(node->controlPoint))->point;
				n = mff->n;
				xyzw = MALLOC(void *, n * 4 * sizeof(GLfloat));
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
		nu = node->uDimension;
		nv = node->vDimension;
		//int knotsOK(int order, int ncontrol, int nknots, double *knots)
		//if(node->uKnot.n && node->uKnot.n == nu + node->uOrder ){
		if(knotsOK(node->uOrder,nu,node->uKnot.n,node->uKnot.p)){
			//could do another check: max number of consecutive equal value knots == order
			//could do another check: knot values == or ascending
			nku = node->uKnot.n;
			knotsu = MALLOC(void *, nku * sizeof(GLfloat));
			for(i=0;i<nku;i++){
				knotsu[i] = (GLfloat)node->uKnot.p[i];
			}
			//printf("good knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);

		}else{
			//generate uniform knot vector 
			static int once = 0;
			nku = nu + node->uOrder ;
			//caller: please malloc knots = MALLOC(void *,  (ncontrol + order ) * sizeof(float))
			knotsu = MALLOC(void *, nku *sizeof(GLfloat));
			generateUniformKnotVector(node->uOrder,nu, knotsu);
			if(!once){
				printf("bad u knot vector given, replacing with:\n");
				for(int ii=0;ii<nku;ii++)
					printf("[%d]=%f \n",ii,knotsu[ii]);
				once = 1;
			}
			//nk = 0;
		}

		if(knotsOK(node->vOrder,nv,node->vKnot.n,node->vKnot.p)){
		//if(node->vKnot.n && node->vKnot.n == nv + node->vOrder ){
			nkv = node->vKnot.n;
			knotsv = MALLOC(void *, nkv * sizeof(GLfloat));
			for(i=0;i<nkv;i++){
				knotsv[i] = (GLfloat)node->vKnot.p[i];
			}
			//printf("good knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);

		}else{
			static int once = 0;
			//generate uniform knot vector 
			nkv = nv + node->vOrder ;
			//caller: please malloc knots = MALLOC(void *,  (ncontrol + order ) * sizeof(float))
			knotsv = MALLOC(void *, nkv *sizeof(GLfloat));
			generateUniformKnotVector(node->vOrder,nv, knotsv);
			if(!once){
				printf("bad v knot vector given, replacing with:\n");
				for(int ii=0;ii<nkv;ii++)
					printf("[%d]=%f \n",ii,knotsv[ii]);
				once = 1;
			}
			//nk = 0;
		}

		if(n && nku && nkv){
			GLUnurbsObj *theNurb;
			int ntessu, ntessv, mtessu, mtessv;
			mtessu = node->uOrder + 1;
			ntessu = node->uTessellation;
			mtessv = node->vOrder + 1;
			ntessv = node->vTessellation;

			if(DEBG) printf("gluNewNurbsRenderer\n");
			theNurb = gluNewNurbsRenderer();
			gluNurbsProperty(theNurb, GLU_NURBS_MODE, GLU_NURBS_TESSELLATOR);
			if(0){
				//chord length or automatic - not implemented properly nor tested thoroughly
				//if you do chord length in pixels, you need to manually pass in sampling matrices
				//and somehow you need to trigger a recompile: another call to this compile_
				// as avatar/viewer moves closer (father) from the nurbs node
				double model[16], proj[16];
				float modelf[16], projf[16];
				int viewPort[10];
				if(ntessu > 0) 
					mtessu = ntessu;
				else if(ntessu < 0) 
					mtessu = -ntessu;

				gluNurbsProperty(theNurb, GLU_SAMPLING_TOLERANCE, (float)(mtessu)); //25.0);
				if(ntessu < 0)
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
				if(ntessu > 0) 
					mtessu = max(mtessu,ntessu+1);
				else if(ntessu < 0) 
					mtessu = max(mtessu,(-ntessu * nu) + 1);
				else
					mtessu = max(mtessu,2*nu + 1);
				if(ntessv > 0) 
					mtessv = max(mtessv,ntessv+1);
				else if(ntessv < 0) 
					mtessv = max(mtessv,(-ntessv * nv) + 1);
				else
					mtessv = max(mtessv,2*nv + 1);

				gluNurbsProperty(theNurb,GLU_SAMPLING_METHOD,GLU_DOMAIN_DISTANCE);
				gluNurbsProperty(theNurb,GLU_U_STEP,(GLfloat)mtessu);
				gluNurbsProperty(theNurb,GLU_V_STEP,(GLfloat)mtessv);
			}
			gluNurbsProperty(theNurb, GLU_DISPLAY_MODE, GLU_FILL);
			
			gluNurbsCallback(theNurb, GLU_ERROR, nurbsError);
			gluNurbsCallback(theNurb, GLU_NURBS_BEGIN_DATA, nurbssurfBegincb);
			gluNurbsCallback(theNurb, GLU_NURBS_VERTEX_DATA, nurbssurfVertexcb);
			gluNurbsCallback(theNurb, GLU_NURBS_NORMAL_DATA, nurbssurfNormalcb);
			gluNurbsCallback(theNurb, GLU_NURBS_END_DATA, nurbssurfEndcb);
			gluNurbsCallback(theNurb, GLU_NURBS_TEXTURE_COORD_DATA, nurbssurfTexcoordcb);
			
			struct Vector * strips = newVector(struct stripState,20);
			gluNurbsCallbackData(theNurb,(GLvoid*)strips);

			if(DEBG) printf("gluBeginSurface \n");
			gluBeginSurface(theNurb);
			gluNurbsSurface(theNurb,nku,knotsu,nkv,knotsv,4,4*nu,xyzw,node->uOrder,node->vOrder,GL_MAP2_VERTEX_4);
			gluNurbsSurface(theNurb,nku,knotsu,nkv,knotsv,4,4*nu,xyzw,node->uOrder,node->vOrder,GL_MAP2_TEXTURE_COORD_2);
			if(trim){
				int i;

				if(0){
					if(DEBG) printf("gluBeginTrim \n");
					gluBeginTrim (theNurb);
					//outside border H: scene author is responsible
					if(1){
						// counter clockwise, simple 4 corner uv from redbook sample
						GLfloat edgePt[5][2] = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}};
						if(DEBG) printf("gluPwlCurve 0\n");
						gluPwlCurve (theNurb, 5, &edgePt[0][0], 2, GLU_MAP1_TRIM_2);
					}else{
						// 2 x (node.utessselation u + node.vtesselation v) + 1 edge values
						// except I get a nurbs error with the following
						GLfloat *edges = MALLOC(void *,  (2 * ntessu + 2 * ntessv) *2*sizeof(GLfloat));
						GLfloat uspan, vspan;
						uspan = 1.0/(float)(ntessu -1);
						vspan = 1.0/(float)(ntessv -1);
						for(i=0;i<ntessu-1;i++){
							edges[i*2 +0] = (float)(i)*uspan;
							edges[i*2 +1] = 0.0;
							edges[(ntessu+ntessv+i)*2 + 0] = (float)(ntessu - 1 - i)*uspan;
							edges[(ntessu+ntessv+i)*2 + 1] = 1.0;
						}
						for(i=0;i<ntessv;i++){
							edges[(ntessu+i)*2 + 0] = 1.0;
							edges[(ntessu+i)*2 + 1] = (float)(i)*vspan;
							edges[(ntessu+ntessv+ntessu+i)*2 + 0] = 0.0;
							edges[(ntessu+ntessv+ntessu+i)*2 + 1] = (float)(ntessv - 1 - i)*vspan;
						}
						//close curve
						edges[((ntessu -1)*2 + (ntessv -1)*2)*2 + 0] = 0.0;
						edges[((ntessu -1)*2 + (ntessv -1)*2)*2 + 1] = 0.0;
						if(DEBG) printf("gluPwlCurve 1\n");
						gluPwlCurve (theNurb, 2*(ntessu -1 + ntessv -1) +1, edges, 2, GLU_MAP1_TRIM_2);
					}
					if(DEBG) printf("gluEndTrim\n");
					gluEndTrim (theNurb);
				}

				//interior cutouts
				if(0){
					//redbook example trim curves - these work
					GLfloat curvePt[4][2] = /* clockwise */ 
						{{0.25, 0.5}, {0.25, 0.75}, {0.75, 0.75}, {0.75, 0.5}};
					GLfloat curveKnots[8] = 
						{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
					GLfloat pwlPt[4][2] = /* clockwise */ 
						{{0.75, 0.5}, {0.5, 0.25}, {0.25, 0.5}};

					if(DEBG) printf("gluBeginTrim A\n");
					gluBeginTrim (theNurb);
					if(DEBG) printf("gluNurbsCurve A\n");
					gluNurbsCurve (theNurb, 8, curveKnots, 2, 
									&curvePt[0][0], 4, GLU_MAP1_TRIM_2);
					if(DEBG) printf("gluPwlCurve A\n");
					gluPwlCurve (theNurb, 3, &pwlPt[0][0], 2, GLU_MAP1_TRIM_2);
					if(DEBG) printf("gluEndTrim A\n");
					gluEndTrim (theNurb);
				}
				if(1)
				for(i=0;i<trim->n;i++){
					int m;
					struct X3D_Contour2D * tc = (struct X3D_Contour2D *)trim->p[i];
					if(DEBG) printf("gluBeginTrim B\n");
					gluBeginTrim (theNurb);
					for(m=0;m<tc->children.n;m++)
					{
						int j,k,dim;
						struct X3D_ContourPolyline2D *cp2d;
						struct X3D_NurbsCurve2D *nc2d;
						struct X3D_Node *ctr = tc->children.p[m]; //trim->p[i];
						GLfloat *cknot, *ctrl, *cweight;
						cknot = ctrl = cweight = NULL;
						switch(ctr->_nodeType){
							case NODE_ContourPolyline2D:
								cp2d = (struct X3D_ContourPolyline2D *)ctr;
								ctrl = MALLOC(void *, cp2d->controlPoint.n * 2*sizeof(GLfloat));
								for(j=0;j<cp2d->controlPoint.n;j++) {
									for(k=0;k<2;k++)
										ctrl[j*2 + k] = (float)cp2d->controlPoint.p[j].c[k];
								}
								if(DEBG) printf("gluPwlCurve B\n");
								gluPwlCurve (theNurb, cp2d->controlPoint.n, ctrl, 2, GLU_MAP1_TRIM_2);

								break;
							case NODE_NurbsCurve2D:
								nc2d = (struct X3D_NurbsCurve2D *)ctr;
								dim = 2;
								int nk = nc2d->controlPoint.n + nc2d->order;
								if(nk == nc2d->knot.n)

								cknot = MALLOC(void *, nk * sizeof(GLfloat));
								if(nc2d->weight.n){ // == 3){
									dim = 3;
									cweight = MALLOC(void *, nc2d->controlPoint.n * sizeof(GLfloat));
									if(nc2d->weight.n == nc2d->controlPoint.n){
										for(j=0;j<nc2d->weight.n;j++) cweight[j] = nc2d->weight.p[j];
									}else{
										for(j=0;j<nc2d->controlPoint.n;j++) cweight[j] = 1.0f;
									}
								}
								ctrl = MALLOC(void *, nc2d->controlPoint.n * dim*sizeof(GLfloat));
								for(j=0;j<nc2d->controlPoint.n;j++) {
									for(k=0;k<2;k++){
										ctrl[j*dim + k] = (float)nc2d->controlPoint.p[j].c[k];
										if(dim == 3) ctrl[j*dim + k] *= cweight[j];
										}
									if(dim == 3) ctrl[j*dim + dim-1] = (float)cweight[j];
								}
								if(knotsOK(nc2d->order,nc2d->controlPoint.n,nc2d->knot.n,nc2d->knot.p)){
								//if(nk == nc2d->knot.n){
									for(j=0;j<nc2d->knot.n;j++)
										cknot[j] = (float)nc2d->knot.p[j];
								}else{
									generateUniformKnotVector(nc2d->order,nc2d->controlPoint.n,cknot);
									printf("replacing nurbscurve2D knotvector with:\n");
									for(j=0;j<nk;j++){
										printf("%f ",cknot[j]);
									}
									printf("\n");

								}
								if(DEBGC) {
									printf("knot %d ={",nc2d->knot.n);
									for(j=0;j<nc2d->knot.n;j++){
										printf("%f ",cknot[j]);
									}
									printf("}\n");
									printf("control %d = {\n",nc2d->controlPoint.n);
									for(j=0;j<nc2d->controlPoint.n;j++) {
										for(k=0;k<dim;k++) printf("%f \n",ctrl[j*dim +k]);
										printf("\n");
									}
									printf("}\n");
								}
								if(DEBG) printf("gluNurbsCurve B\n");
								if(1){
									int mtess, ntess;
									mtess = nc2d->order + 1;
									ntess = nc2d->tessellation;
									if(0){
										//chord length or automatic - not implemented properly nor tested thoroughly
										//if you do chord length in pixels, you need to manually pass in sampling matrices
										//and somehow you need to trigger a recompile: another call to this compile_
										// as avatar/viewer moves closer (father) from the nurbs node
										double model[16], proj[16];
										float modelf[16], projf[16];
										int viewPort[10];
										if(ntess > 0) 
											mtess = ntess;
										else if(ntess < 0) 
											mtess = -ntess;

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
										gluNurbsProperty(theNurb,GLU_SAMPLING_METHOD,GLU_DOMAIN_DISTANCE);
										gluNurbsProperty(theNurb,GLU_U_STEP,(GLfloat)mtess);
									}
								}
								gluNurbsCurve (theNurb, nk, cknot, dim, ctrl, nc2d->order, GLU_MAP1_TRIM_2);
								break;
							default:
								ConsoleMessage("%s %d","unknown trimming contour node",ctr->_nodeType);
						}
						FREE_IF_NZ(ctrl);
						FREE_IF_NZ(cknot);
						FREE_IF_NZ(cweight);
					}
					if(DEBG) printf("gluEndTrim B\n");
					gluEndTrim (theNurb);
				}
			}
			if(DEBG) printf("gluEndSurface \n");
			gluEndSurface(theNurb);
			if(DEBG) printf("gluDeleteNurbsRenderer \n");
			gluDeleteNurbsRenderer(theNurb);

			//convert points to polyrep
			convert_strips_to_polyrep(strips,node);
		}
	}
#endif
}

void render_ray_polyrep(void *node);
void collide_genericfaceset(void *node);
void render_polyrep(void *node);

void compile_NurbsPatchSurface(struct X3D_NurbsPatchSurface *node){
	MARK_NODE_COMPILED
	compile_NurbsSurface(node, NULL);
}
void rendray_NurbsPatchSurface (struct X3D_NurbsPatchSurface *node) {
		COMPILE_IF_REQUIRED
		if (!node->_intern) return;
		render_ray_polyrep(node);
}

void collide_NurbsPatchSurface (struct X3D_NurbsPatchSurface *node) {
		COMPILE_IF_REQUIRED
		if (!node->_intern) return;
		collide_genericfaceset(node);
}

void render_NurbsPatchSurface (struct X3D_NurbsPatchSurface *node) {
		COMPILE_IF_REQUIRED
		if (!node->_intern) return;
		CULL_FACE(node->solid)
		render_polyrep(node);
}

void compile_NurbsTrimmedSurface(struct X3D_NurbsTrimmedSurface *node){
	MARK_NODE_COMPILED
	compile_NurbsSurface((struct X3D_NurbsPatchSurface *)node, &node->trimmingContour);
}

void rendray_NurbsTrimmedSurface (struct X3D_NurbsTrimmedSurface *node) {
		COMPILE_IF_REQUIRED
		if (!node->_intern) return;
		render_ray_polyrep(node);
}

void collide_NurbsTrimmedSurface (struct X3D_NurbsTrimmedSurface *node) {
		COMPILE_IF_REQUIRED
		if (!node->_intern) return;
		collide_genericfaceset(node);
}

void render_NurbsTrimmedSurface (struct X3D_NurbsTrimmedSurface *node) {
		COMPILE_IF_REQUIRED
		if (!node->_intern) return;
		CULL_FACE(node->solid)
		render_polyrep(node);
}

void do_NurbsPositionInterpolator (void *node) {
	struct X3D_NurbsPositionInterpolator *px;
	int kin, kvin; //, counter, tmp;
	//struct SFVec3f *kVs; 

	if (!node) return;
	px = (struct X3D_NurbsPositionInterpolator *) node;
	kvin = kin = 0;
	//kvin = px->controlPoint->.n;
	//kVs = px->controlPoint.p;

	MARK_EVENT (node, offsetof (struct X3D_NurbsPositionInterpolator, value_changed)); 

	// make sure we have the keys and keyValues 
	if ((kvin == 0) || (kin == 0)) {
		vecset3f(px->value_changed.c,0.0f,0.0f,0.0f);
		return;
	}

	if (kin>kvin) kin=kvin; // means we don't use whole of keyValue, but... 

	// set_fraction less than or greater than keys 
	//if (px->set_fraction <= ((px->key).p[0])) {
	//	memcpy ((void *)&px->value_changed, (void *)&kVs[0], sizeof (struct SFVec3f));
	//} else if (px->set_fraction >= px->key.p[kin-1]) {
	//	memcpy ((void *)&px->value_changed, (void *)&kVs[kvin-1], sizeof (struct SFVec3f));
	//} else {
	//	// have to go through and find the key before 
	//	counter = find_key(kin,((float)(px->set_fraction)),px->key.p);
	//	for (tmp=0; tmp<3; tmp++) {
	//		px->value_changed.c[tmp] =
	//			(px->set_fraction - px->key.p[counter-1]) /
	//			(px->key.p[counter] - px->key.p[counter-1]) *
	//			(kVs[counter].c[tmp] - kVs[counter-1].c[tmp]) + kVs[counter-1].c[tmp];
	//	}
	//}
	#ifdef SEVERBOSE
	printf ("Pos/Col, new value (%f %f %f)\n",
		px->value_changed.c[0],px->value_changed.c[1],px->value_changed.c[2]);
	#endif
}

/* NurbsOrientationInterpolator				 		
 Called during the "events_processed" section of the event loop,	
 so this is called ONLY when there is something required to do, thus	
 there is no need to look at whether it is active or not
 */
#include "quaternion.h"
void do_NurbsOrientationInterpolator (void *node) {
/*
	struct X3D_NurbsOrientationInterpolator *px;
	int kin, kvin;
	struct SFRotation *kVs;
	int counter;
	float interval;		// where we are between 2 values 
	// UNUSED?? int stzero;
	// UNUSED?? int endzero;	// starting and/or ending angles zero? 

	Quaternion st, fin, final;
	double x,y,z,a;

	if (!node) return;
	px = (struct X3D_NurbsOrientationInterpolator *) node;
	kin = ((px->key).n);
	kvin = ((px->keyValue).n);
	kVs = ((px->keyValue).p);

	#ifdef SEVERBOSE
	printf ("starting do_Oint4; keyValue count %d and key count %d\n",
				kvin, kin);
	#endif


	MARK_EVENT (node, offsetof (struct X3D_NurbsOrientationInterpolator, value_changed));

	// make sure we have the keys and keyValues 
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.c[0] = (float) 0.0;
		px->value_changed.c[1] = (float) 0.0;
		px->value_changed.c[2] = (float) 0.0;
		px->value_changed.c[3] = (float) 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; // means we don't use whole of keyValue, but... 


	// set_fraction less than or greater than keys 
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[0], sizeof (struct SFRotation));
	} else if (px->set_fraction >= ((px->key).p[kin-1])) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[kvin-1], sizeof (struct SFRotation));
	} else {
		counter = find_key(kin,(float)(px->set_fraction),px->key.p);
		interval = (px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]);

		
		// are either the starting or ending angles zero? 
		// unused? stzero = APPROX(kVs[counter-1].c[3],0.0);
		// unused? endzero = APPROX(kVs[counter].c[3],0.0);
		#ifdef SEVERBOSE
			printf ("counter %d interval %f\n",counter,interval);
			printf ("angles %f %f %f %f, %f %f %f %f\n",
				kVs[counter-1].c[0],
				kVs[counter-1].c[1],
				kVs[counter-1].c[2],
				kVs[counter-1].c[3],
				kVs[counter].c[0],
				kVs[counter].c[1],
				kVs[counter].c[2],
				kVs[counter].c[3]);
		#endif
		vrmlrot_to_quaternion (&st, kVs[counter-1].c[0],
                                kVs[counter-1].c[1], kVs[counter-1].c[2], kVs[counter-1].c[3]);
		vrmlrot_to_quaternion (&fin,kVs[counter].c[0],
                                kVs[counter].c[1], kVs[counter].c[2], kVs[counter].c[3]);

		quaternion_slerp(&final, &st, &fin, (double)interval);
		quaternion_to_vrmlrot(&final,&x, &y, &z, &a);
		px->value_changed.c[0] = (float) x;
		px->value_changed.c[1] = (float) y;
		px->value_changed.c[2] = (float) z;
		px->value_changed.c[3] = (float) a;

		#ifdef SEVERBOSE
		printf ("Oint, new angle %f %f %f %f\n",px->value_changed.c[0],
			px->value_changed.c[1],px->value_changed.c[2], px->value_changed.c[3]);
		#endif
	}
*/
}

void do_NurbsSurfaceInterpolator (void *node) {
/*
	struct X3D_NurbsSurfaceInterpolator *px;
	int kin, kvin, counter, tmp;
	struct SFVec3f *kVs; 

	if (!node) return;
	px = (struct X3D_NurbsSurfaceInterpolator *) node;

	kvin = px->keyValue.n;
	kVs = px->keyValue.p;
	kin = px->key.n;

	MARK_EVENT (node, offsetof (struct X3D_NurbsSurfaceInterpolator, position_changed)); 
	MARK_EVENT (node, offsetof (struct X3D_NurbsSurfaceInterpolator, normal_changed)); 

	#ifdef SEVERBOSE
		printf("do_PositionInt: Position/Vec3f interp, node %u kin %d kvin %d set_fraction %f\n",
			   node, kin, kvin, px->set_fraction);
	#endif

	// make sure we have the keys and keyValues 
	if ((kvin == 0) || (kin == 0)) {
		vecset3f(px->position_changed.c,0.0f,0.0f,0.0f);
		vecset3f(px->normal_changed.c,0.0f,0.0f,0.0f);
		return;
	}

	if (kin>kvin) kin=kvin; // means we don't use whole of keyValue, but... 

	// set_fraction less than or greater than keys 
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed, (void *)&kVs[0], sizeof (struct SFVec3f));
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		memcpy ((void *)&px->value_changed, (void *)&kVs[kvin-1], sizeof (struct SFVec3f));
	} else {
		// have to go through and find the key before 
		counter = find_key(kin,((float)(px->set_fraction)),px->key.p);
		for (tmp=0; tmp<3; tmp++) {
			px->value_changed.c[tmp] =
				(px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]) *
				(kVs[counter].c[tmp] - kVs[counter-1].c[tmp]) + kVs[counter-1].c[tmp];
		}
	}
	#ifdef SEVERBOSE
	printf ("Pos/Col, new value (%f %f %f)\n",
		px->value_changed.c[0],px->value_changed.c[1],px->value_changed.c[2]);
	#endif
*/
}
