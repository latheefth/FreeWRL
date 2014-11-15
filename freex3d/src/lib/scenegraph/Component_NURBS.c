
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

typedef struct pComponent_NURBS{
	void *nada;// = 0;

}* ppComponent_NURBS;
void *Component_NURBS_constructor(){
	void *v = malloc(sizeof(struct pComponent_NURBS));
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

//curve
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
	int j,k,m, p;
	float uniform;
	p = order -1;
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
				// as avatar/viewer moves closer (father) from the nurbs node
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

 struct stripState{
	int type;
	struct Vector pv;
	struct Vector nv;
};

 #ifdef NURBS_LIB
//surface
// strategy: on begin, we'll store the gl_ type, and zero __points
// then accumulate the __points
// and on end, convert the __points to ->_intern->polyrep points and triangle indices
//  using the stored type as a guide
void CALLBACK nurbssurfBegincb(GLenum type, void *ud)
{
	//struct X3D_NurbsPatchSurface *node = (struct X3D_NurbsPatchSurface *)ud;
	struct stripState ss;
	struct Vector * strips = (struct Vector *)ud;
	printf("nurbssurfBegin type = ");
	switch(type){
		case GL_QUAD_STRIP: printf("QUAD_STRIP");break;
		case GL_TRIANGLE_STRIP: printf("TRIANGLE_STRIP");break;
		case GL_TRIANGLE_FAN: printf("TRIANGLE_FAN");break;
		case GL_TRIANGLES: printf("TRIANGLES");break;
		default:
			printf("not sure %x %d",type,type);
	}
	ss.nv.n = 0;
	ss.nv.allocn = 0;
	ss.nv.data = NULL;
	ss.pv.n = 0;
	ss.pv.allocn = 0;
	ss.pv.data = NULL;
	ss.type = type;
	vector_pushBack(struct stripState,strips,ss);
	//node->__numPoints = 0;
	//node->__numNormals = 0;
	//node->__meshtype = type;
	//we leave points.n and points, and just overwrite, to save reallocs
	printf("\n");
}
void CALLBACK nurbssurfVertexcb(GLfloat *vertex, void *ud)
{
	//int i, np,ns;
	struct stripState ss;
	struct SFVec3f pp;
	//struct X3D_NurbsPatchSurface *node = (struct X3D_NurbsPatchSurface *)ud;
	struct Vector * strips = (struct Vector *)ud;
	ss = vector_get(struct stripState,strips,strips->n -1);
	memcpy(&pp,vertex,sizeof(struct SFVec3f));
	vector_pushBack(struct SFVec3f,&ss.pv,pp);
	vector_set(struct stripState,strips,strips->n-1,ss);

	//ns = node->__points.n; 
	//np = node->__numPoints;
	//if(np+1 > ns) {
	//	ns = np *2;
	//	node->__points.p = realloc(node->__points.p,ns * sizeof(struct SFVec3f));
	//	node->__points.n = ns;
	//}
	//pp = &node->__points.p[np];
	//for(i=0;i<3;i++)
	//	pp->c[i] = vertex[i];
	//node->__numPoints ++;
	//node->__points.n++;
	printf("nurbssurfVertex %f %f %f\n",vertex[0],vertex[1],vertex[2]);
}
void CALLBACK nurbssurfNormalcb(GLfloat *nml, void *ud)
{
	//int i, np,ns;
	struct stripState ss;
	struct SFVec3f pp;
	//struct X3D_NurbsPatchSurface *node = (struct X3D_NurbsPatchSurface *)ud;
	struct Vector * strips = (struct Vector *)ud;
	ss = vector_get(struct stripState,strips,strips->n -1);
	memcpy(&pp,nml,sizeof(struct SFVec3f));
	vector_pushBack(struct SFVec3f,&ss.nv,pp);
	vector_set(struct stripState,strips,strips->n-1,ss);

	//ns = node->__normals.n; 
	//np = node->__numNormals;
	//if(np+1 > ns) {
	//	ns = np *2;
	//	node->__normals.p = realloc(node->__normals.p,ns * sizeof(struct SFVec3f));
	//	node->__normals.n = ns;
	//}
	//pp = &node->__normals.p[np];
	//for(i=0;i<3;i++)
	//	pp->c[i] = nml[i];
	//node->__numNormals ++;

	printf("nurbssurfNormal\n");
}
void CALLBACK nurbssurfEndcb(void *ud)
{
	//struct X3D_NurbsPatchSurface *node = (struct X3D_NurbsPatchSurface *)ud;
	struct stripState ss;
	struct Vector * strips = (struct Vector *)ud;
	ss = vector_get(struct stripState,strips,strips->n -1);
	//node->__numPoints = node->__points.n;
	//printf("nurbssurfEnd #p %d #n %d\n",node->__numPoints, node->__numNormals);
	// convert to polyrep points and triangles
	printf("nurbssurfEnd #p %d #n %d\n",ss.pv.n, ss.nv.n);
	for(int i=0;i<ss.pv.n;i++){
		struct SFVec3f pp = vector_get(struct SFVec3f,&ss.pv,i);
		printf("%f %f %f\n",pp.c[0],pp.c[1],pp.c[2]);
	}

}
#endif

void free_polyrep(struct X3D_PolyRep *rep){
	if(rep){
		rep->ntri = 0;
		rep->transparency = 0;
		//Q. are any of these added to GC tables? If not..
		FREE_IF_NZ(rep->actualCoord);
		FREE_IF_NZ(rep->cindex);
		FREE_IF_NZ(rep->colindex);
		FREE_IF_NZ(rep->GeneratedTexCoords);
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
	polyrep->ntri = -1;
	polyrep->cindex = 0; polyrep->actualCoord = 0; polyrep->colindex = 0; polyrep->color = 0;
	polyrep->norindex = 0; polyrep->normal = 0; polyrep->GeneratedTexCoords = 0;
	polyrep->tcindex = 0; 
	polyrep->tcoordtype = 0;
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

	/* printf ("they are %u %u %u %u\n",polyrep->VBO_buffers[0],polyrep->VBO_buffers[1],polyrep->VBO_buffers[2],polyrep->VBO_buffers[3]); */
	return polyrep;
}

void convert_strips_to_polyrep(struct Vector * strips,struct X3D_PolyRep *rep){
	int i, j, npoints, np, ni, ntri, nindex;
	struct stripState ss;

	npoints = nindex = 0;
	for(i=0;i<strips->n;i++){
		ss = vector_get(struct stripState,strips,i);
		npoints += ss.pv.n;
		switch(ss.type){
			case GL_QUAD_STRIP: nindex += (ss.pv.n -2)/2 * 5;break;
			case GL_TRIANGLE_STRIP: nindex += (ss.pv.n -2);break;
			case GL_TRIANGLE_FAN: nindex += (ss.pv.n -2);break;
			case GL_TRIANGLES: nindex += (ss.pv.n -2);break;
			default:
				nindex += (ss.pv.n -2);
		}
	}
	rep->actualCoord = malloc(npoints * 3 * sizeof(float));
	rep->normal = malloc(npoints * 3 * sizeof(float));
	rep->ntri = nindex;
	rep->cindex = malloc(rep->ntri * 4 * sizeof(int));
	rep->norindex = malloc(rep->ntri * 4 * sizeof(int));

	np = 0;
	ni = 0;
	ntri = 0;
	for(i=0;i<strips->n;i++){
		ss = vector_get(struct stripState,strips,i);
		memcpy(&rep->actualCoord[np*3],ss.pv.data,ss.pv.n * 3 * sizeof(float));
		memcpy(&rep->normal[np*3],ss.nv.data,ss.nv.n * 3 * sizeof(float));
		switch(ss.type){
			case GL_QUAD_STRIP: 
				for(j=0;j<ss.pv.n -2;j+=2){
					rep->cindex[ni++] = np+j;
					rep->cindex[ni++] = np+j+1;
					rep->cindex[ni++] = np+j+3;
					//rep->cindex[ni++] = -1;
					rep->cindex[ni++] = np+j+3;
					rep->cindex[ni++] = np+j+2;
					rep->cindex[ni++] = np+j;
					//rep->cindex[ni++] = -1;
					//memcpy(&rep->norindex[ntri*4],&rep->cindex[ntri*4],2*4*sizeof(int));
					memcpy(&rep->norindex[ntri*3],&rep->cindex[ntri*3],2*4*sizeof(int));
					ntri += 2;
				}
				break;
			case GL_TRIANGLE_STRIP: 
				nindex += (ss.pv.n -2);
				break;
			case GL_TRIANGLE_FAN: 
				nindex += (ss.pv.n -2);
				break;
			case GL_TRIANGLES: 
				nindex += (ss.pv.n -2);
				break;
			default:
				nindex += (ss.pv.n -2);
		}
		np += ss.pv.n;
	}
	rep->ntri = ntri;
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

void stream_polyrep(void *innode, void *coord, void *color, void *normal, struct X3D_TextureCoordinate *texCoordNode);
void compile_NurbsPatchSurface(struct X3D_NurbsPatchSurface *node){
	ppComponent_NURBS p = (ppComponent_NURBS)gglobal()->Component_NURBS.prv;
	MARK_NODE_COMPILED
#ifdef NURBS_LIB
	{
		int i,j, n, nu, nv, nku, nkv;
		GLfloat *xyzw, *knotsu, *knotsv;
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
		nu = node->uDimension;
		nv = node->vDimension;
		if(node->uKnot.n && node->uKnot.n == nu + node->uOrder ){
			nku = node->uKnot.n;
			knotsu = malloc(nku * sizeof(GLfloat));
			for(i=0;i<nku;i++){
				knotsu[i] = (GLfloat)node->uKnot.p[i];
			}
			//printf("good knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);

		}else{
			//generate uniform knot vector 
			nku = nu + node->uOrder ;
			//caller: please malloc knots = malloc( (ncontrol + order ) * sizeof(float))
			knotsu = malloc(nku *sizeof(GLfloat));
			generateUniformKnotVector(node->uOrder,nu, knotsu);
			//printf("bad knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);
			//nk = 0;
		}

		if(node->vKnot.n && node->vKnot.n == nv + node->vOrder ){
			nkv = node->vKnot.n;
			knotsv = malloc(nkv * sizeof(GLfloat));
			for(i=0;i<nkv;i++){
				knotsv[i] = (GLfloat)node->vKnot.p[i];
			}
			//printf("good knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);

		}else{
			//generate uniform knot vector 
			nkv = nv + node->vOrder ;
			//caller: please malloc knots = malloc( (ncontrol + order ) * sizeof(float))
			knotsv = malloc(nkv *sizeof(GLfloat));
			generateUniformKnotVector(node->vOrder,nv, knotsv);
			//printf("bad knot nk=%d\n",nk);
			//for(int ii=0;ii<nk;ii++)
			//	printf("[%d]=%f \n",ii,knots[ii]);
			//nk = 0;
		}

		if(n && nku && nkv){
			GLUnurbsObj *theNurb;
			int ntessu, ntessv, mtessu, mtessv;
			mtessu = node->uOrder + 1;
			ntessu = node->uTessellation;
			mtessv = node->vOrder + 1;
			ntessv = node->vTessellation;

			theNurb = gluNewNurbsRenderer();
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
			free_polyrep(node->_intern);
			node->_intern = create_polyrep();
			struct Vector * strips = newVector(struct stripState,20);
			gluNurbsProperty(theNurb, GLU_NURBS_MODE, GLU_NURBS_TESSELLATOR);
			gluNurbsCallbackData(theNurb,(GLvoid*)strips);

			gluBeginSurface(theNurb);
				gluNurbsSurface(theNurb,nku,knotsu,nkv,knotsv,4,4*nu,xyzw,node->uOrder,node->vOrder,GL_MAP2_VERTEX_4);
			gluEndSurface(theNurb);
			gluDeleteNurbsRenderer(theNurb);

			//convert points to polyrep
			convert_strips_to_polyrep(strips,node->_intern);
			if(node->_intern->ntri != 0){
				//void stream_polyrep(void *innode, void *coord, void *color, void *normal, struct X3D_TextureCoordinate *texCoordNode)
				stream_polyrep(node, NULL, NULL, NULL, NULL);
			}
			/* and, tell the rendering process that this shape is now compiled */
			node->_intern->irep_change = node->_change;
		}
	}
#endif
}

//void compile_NurbsPatchSurface (struct X3D_NurbsPatchSurface * node) {
//
//	#ifdef VERBOSE
//	printf ("compiling NurbsPatchSurface\n");
//	#endif
//
//	MARK_NODE_COMPILED
//}
void render_ray_polyrep(void *node);
void collide_genericfaceset(void *node);
void render_polyrep(void *node);

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
		//COMPILE_POLY_IF_REQUIRED (node->coord, node->color, node->normal, node->texCoord)
		COMPILE_IF_REQUIRED
		if (!node->_intern) return;
		CULL_FACE(node->solid)
		render_polyrep(node);
}

