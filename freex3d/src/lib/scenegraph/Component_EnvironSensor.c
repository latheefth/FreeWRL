/*


X3D Environmental Sensors Component

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
#include "../vrml_parser/CRoutes.h"
#include "../main/headers.h"

#include "LinearAlgebra.h"
#include "Component_Geospatial.h"
#include "../opengl/Frustum.h"
#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"


///* can we do a VisibiltySensor? Only if we have OpenGL support for OcclusionCulling */
//int candoVisibility = TRUE;
typedef struct pComponent_EnvironSensor{
	/* can we do a VisibiltySensor? Only if we have OpenGL support for OcclusionCulling */
	int candoVisibility;// = TRUE;

}* ppComponent_EnvironSensor;
void *Component_EnvironSensor_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_EnvironSensor));
	memset(v,0,sizeof(struct pComponent_EnvironSensor));
	return v;
}
void Component_EnvironSensor_init(struct tComponent_EnvironSensor *t){
	//public
	//private
	t->prv = Component_EnvironSensor_constructor();
	{
		ppComponent_EnvironSensor p = (ppComponent_EnvironSensor)t->prv;
		/* can we do a VisibiltySensor? Only if we have OpenGL support for OcclusionCulling */
		p->candoVisibility = TRUE;

	}
}

#ifdef VISIBILITYOCCLUSION

static void rendVisibilityBox (struct X3D_VisibilitySensor *node);
#endif

PROXIMITYSENSOR(ProximitySensor,center,,);


/* VisibilitySensors - mimic what Shape does to display the box. */





void transformMBB(GLDOUBLE *rMBBmin, GLDOUBLE *rMBBmax, GLDOUBLE *matTransform, GLDOUBLE* inMBBmin, GLDOUBLE* inMBBmax);
int transformMBB4d(GLDOUBLE *rMBBmin, GLDOUBLE *rMBBmax, GLDOUBLE *matTransform, GLDOUBLE* inMBBmin, GLDOUBLE* inMBBmax, int isAffine);
int __gluInvertMatrixd(const GLDOUBLE m[16], GLDOUBLE invOut[16]);
void __gluMultMatrixVecd(const GLDOUBLE matrix[16], const GLDOUBLE in[4], GLDOUBLE out[4]);

void twoPoints2RayMatrix(double *ptnear, double* ptfar, double* rayMatrix){
	double R1[16], R2[16], R3[16], T[16], rayMatrixInverse[16];
	double *A, *B, C[3];
	double yaw, pitch;

	A = ptnear;
	B = ptfar;
	//nearside point
	mattranslate(T,A[0],A[1],A[2]);
	vecdifd(C,B,A);
	vecnormald(C,C);
	if(0) printf("Cdif %f %f %f\n",C[0],C[1],C[2]);
	yaw = atan2(C[0],-C[2]);
	matrixFromAxisAngle4d(R1, -yaw, 0.0, 1.0, 0.0);
	transformAFFINEd(C,C,R1);
	if(0) printf("Yawed Cdif %f %f %f\n",C[0],C[1],C[2]);
	pitch = atan2(C[1],-C[2]);
	if(0) printf("atan2 yaw=%f pitch=%f\n",yaw,pitch);
	pitch = -pitch;
	if(0) printf("[yaw=%f pitch=%f\n",yaw,pitch);

	matrixFromAxisAngle4d(R1, pitch, 1.0, 0.0, 0.0);
	if(0) printmatrix2(R1,"pure R1");
	matrixFromAxisAngle4d(R2, yaw, 0.0, 1.0, 0.0);
	if(0) printmatrix2(R2,"pure R2");
	matmultiplyAFFINE(R3,R1,R2);
	if(0) printmatrix2(R3,"R3=R1*R2");
	matmultiplyAFFINE(rayMatrixInverse,R3, T); 
	matinverseAFFINE(rayMatrix,rayMatrixInverse);

}

int frustumHitsMBB(float *extent){
	//goal say if an extent is maybe inside (err toward inside) of the view frustum
	//http://cgvr.informatik.uni-bremen.de/teaching/cg_literatur/lighthouse3d_view_frustum_culling/index.html
	// overlap tests: https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
	// cuboid space: frustum is a -1 to 1 cube
	GLDOUBLE modelMatrix[16], projectionMatrix[16];
	int i, j, isIn, iret;
	GLDOUBLE smin[3], smax[3], shapeMBBmin[3], shapeMBBmax[3];
	int retval;
	retval = FALSE;

	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);
	FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projectionMatrix);

	/* generate mins and maxes for avatar cylinder in avatar space to represent the avatar collision volume */
	for(i=0;i<3;i++)
	{
		shapeMBBmin[i] = extent[i*2 + 1];
		shapeMBBmax[i] = extent[i*2];
	}
	//check view space is it behind the frontplane / viewpoint
	transformMBB(smin,smax,modelMatrix,shapeMBBmin,shapeMBBmax); //transform shape's MBB into view space
	isIn = TRUE;
	isIn = smin[2] < 0.0; //-z is in front of viewpoint
	if(isIn){
		//plane check: rotate so lower left corner of screen is in center and do xy comparison
		// repeat for upper-right
		/*
		frustum-plane aligned method 
		- in view space (apply modelview, but not projection)
		- plus a translation and rotation so looking straight down a side of frustum
		- then can do simple X or Y test if something is out
		- to reduce flops, can rotate so looking down corner-edge (2 planes intersect) and do X,Y 
		- then only need 2 runs to check 4 planes
		How: - (similar to pickray-style matrix method - see mainloop.c setup_pickray0() about line 5409 july 2016)
		1. generate near+far frustum corner points:
			- unproject a couple of cuboid points along a corner of the frustom: x-1,y-1, near (z=1) and far (z=-1 or vice versa)
		2. from the 2 points compute a yaw, and a pitch, using atan2 etc 
		3. make a rotation matrix from yaw and pitch
		4. from the near point make a Translation
		5. multiply the rotation and translation
		6. maybe inverse, depending on which way you're going.
		7. then you could concatonate that matrix with your modelview
		8. transform your geom node extent using this concatonated matrix
		9. check transformed shape MBB/AABB against x=0 and y=0 planes: if to the left, then out, if above then out.
		10. repeat for diagonal edge/corner of frustum x=1,y=1 in cuboid space
		11. check near and far planes if you want to, although near is done by cone planes except a little near-cone, and far: who cares.
		*/
		int k;
		double rayMatrix[16], modelMatrixPlus[16], projInverse[16], nearplane, farplane;
		double A[4], B[4], a[4], b[4];

		//we are working in something close to view space, so 
		//-transform frustum cuboid to view via projection inverse, to set up 'plus' 
		//-transform node extent to view via modelview matrix, 'plus' a bit to align with frustum

		iret = __gluInvertMatrixd( projectionMatrix, projInverse);

		//k: from lower-left corner of frustum to upper-right corner...
		for(k=-1;k<=1;k+=2) 
		{
			double xy;
			xy = 1.0*k; //use -1,-1 for lower-left, 1,1 for upper-right
			a[0] = a[1] = b[0] = b[1] = xy;
			a[3] = b[3] = 1.0; //homogenous .w for full 4x4

			//nearside point
			a[2] = -1.0;
			__gluMultMatrixVecd(projInverse, a, A);
			vecscaled(A,A,1.0/A[3]); //divide by homogenous .w
			nearplane = A[2]; //near and far set elsewhere: ie .1 - 21000, here we recover nearplane, could use for testing
			
			//farside point
			b[2] = 1.0; 
			__gluMultMatrixVecd(projInverse, b, B);
			vecscaled(B,B,1.0/B[3]);
			farplane = B[2];

			twoPoints2RayMatrix(A,B,rayMatrix); //compute 'plus' part

			matmultiplyAFFINE(modelMatrixPlus,modelMatrix,rayMatrix);

			//transform shape's MBB into frustum-plane (frustum corner) aligned space
			transformMBB(smin,smax,modelMatrixPlus,shapeMBBmin,shapeMBBmax); 
			//now fustum corner is at xy 0,0, so its a simple xy test against 0
			for(i=0;i<2;i++){
				if(k==-1)
					isIn = isIn && smax[i] > 0.0;
				if(k==1)
					isIn = isIn && smin[i] < 0.0;
			}
			//isIn = isIn && smax[2] > nearplane; doesn't work right H: sign wrong? and don't seem to need nearplane -its a tiny cone we miss
			//isIn = isIn && smin[2] < farplane; who cares about farplane
		}
	}

	return isIn;
}
void other_VisibilitySensor (struct X3D_VisibilitySensor *node) {
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/envsensor.html#VisibilitySensor
	ttrenderstate rs;
	ttglobal tg = gglobal();
	/* if not enabled, do nothing */
	if (!node) return;
	if (!node->enabled) 
		return;
	{
		ppComponent_EnvironSensor p = (ppComponent_EnvironSensor)tg->Component_EnvironSensor.prv;
		if (!p->candoVisibility) return;
		//aabb method
		/* first time through, if we have a visibility sensor, but do not have the OpenGL ability to
		   use it, we print up a console message */
		//1. transform local axix-aligned bounding box AABB from local to cuboid
		//   cuboid_aabb = projection * modelview * local_aabb
		//2. intersect cuboid_aabb with cuboid which is -1 to 1 in 3 dimensions
		//3. if they don't intersect, not visible, else maybe visible
		{
			//update extent in case size or center changed
			int i, ihit;
			float emin[3], emax[3];
			vecadd3f(emax, node->center.c, node->size.c);
			vecdif3f(emin, node->center.c, node->size.c);
			for(i=0;i<3;i++)
			{
				node->_extent[i*2 + 1] = emin[i];
				node->_extent[i*2]     = emax[i];
			}
			ihit = frustumHitsMBB(node->_extent);
			if(ihit){
				node->__Samples++; //initialized to 0 once each frame elsewhere
				//can come in here multiple times per frame: once per each viewport (stereo 2, quad 4) x once per each DEF/USE
				//if any DEF/USE or viewport visible, then __visible = TRUE:
				//if visibility was different on last frame, then send events

			}
		}

#ifdef VISIBILITYOCCLUSION
		//occlusion method
		if (tg->Frustum.OccFailed) {
			p->candoVisibility = FALSE;
			ConsoleMessage("VisibilitySensor: OpenGL on this machine does not support GL_ARB_occlusion_query");
			return;
		}

		rs = renderstate();
		RECORD_DISTANCE

		if (rs->render_blend) { 

			//BEGINOCCLUSIONQUERY
			beginOcclusionQuery(node,renderstate()->render_geom);
			LIGHTING_OFF
			DISABLE_CULL_FACE 

			rendVisibilityBox(node);
			
			ENABLE_CULL_FACE
			LIGHTING_ON
			
			//ENDOCCLUSIONQUERY
			endOcclusionQuery(node,renderstate()->render_geom);
		}
#endif
	}
}

#ifdef VISIBILITYOCCLUSION

static void rendVisibilityBox (struct X3D_VisibilitySensor *node) {
#ifdef HAVE_TO_REIMPLEMENT
	extern GLfloat boxnorms[];		/*  in CFuncs/statics.c*/
	float *pt;
	float x = ((node->size).c[0])/2;
	float y = ((node->size).c[1])/2;
	float z = ((node->size).c[2])/2;
	float cx = node->center.c[0];
	float cy = node->center.c[1];
	float cz = node->center.c[2];

	/* test for <0 of sides */
	if ((x < 0) || (y < 0) || (z < 0)) return;

	/* for BoundingBox calculations */
	setExtent(cx+x, cx-x, cx+y, cx-y, cx+z, cx-z,X3D_NODE(node));


	/* printf ("VISIBILITY BOXc vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
         render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

	if NODE_NEEDS_COMPILING {
		/*  have to regen the shape*/
		MARK_NODE_COMPILED

		/*  MALLOC memory (if possible)*/
		/* do not worry about the __points.n field; we know the size by default */
		if (!node->__points.p) node->__points.p = MALLOC (struct SFVec3f*, sizeof(struct SFVec3f)*(36));


		/*  now, create points; 4 points per face.*/
		pt = (float *) node->__points.p;

		#define PTF0 *pt++ = cx+x; *pt++ = cy+y; *pt++ = cz+z;
		#define PTF1 *pt++ = cx-x; *pt++ = cy+y; *pt++ = cz+z;
		#define PTF2 *pt++ = cx-x; *pt++ = cy-y; *pt++ = cz+z;
		#define PTF3 *pt++ = cx+x; *pt++ = cy-y; *pt++ = cz+z;
		#define PTR0 *pt++ = cx+x; *pt++ = cy+y; *pt++ = cz-z;
		#define PTR1 *pt++ = cx-x; *pt++ = cy+y; *pt++ = cz-z;
		#define PTR2 *pt++ = cx-x; *pt++ = cy-y; *pt++ = cz-z;
		#define PTR3 *pt++ = cx+x; *pt++ = cy-y; *pt++ = cz-z;


		PTF0 PTF1 PTF2  PTF0 PTF2 PTF3 /* front */
		PTR2 PTR1 PTR0  PTR3 PTR2 PTR0 /* back  */
		PTF0 PTR0 PTR1  PTF0 PTR1 PTF1 /* top   */
		PTF3 PTF2 PTR2  PTF3 PTR2 PTR3 /* bottom */
		PTF0 PTF3 PTR3 	PTF0 PTR3 PTR0 /* right */
		PTF1 PTR1 PTR2  PTF1 PTR2 PTF2 /* left */

		/* finished, and have good data */
	}

	FW_GL_DEPTHMASK(FALSE);
	/* note the ALPHA of zero - totally transparent */
    
	//OLDCODE FW_GL_COLOR4F(0.0f, 1.0f, 0.0f, 0.0f);

	/*  Draw it; assume VERTEX and NORMALS already defined.*/
	FW_GL_VERTEX_POINTER(3,GL_FLOAT,0,(GLfloat *)node->__points.p);
	FW_GL_NORMAL_POINTER(GL_FLOAT,0,boxnorms);

	/* do the array drawing; sides are simple 0-1-2-3, 4-5-6-7, etc quads */
	sendArraysToGPU (GL_TRIANGLES, 0, 36);
	FW_GL_DEPTHMASK(TRUE);
#endif// HAVE_TO_REIMPLEMENT
}                        
#endif // VISIBILITYOCCLUSION


void do_VisibilitySensorTick (void *ptr) {
	struct X3D_VisibilitySensor *node = (struct X3D_VisibilitySensor *) ptr;

	/* if not enabled, do nothing */
	if (!node) return;
	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_VisibilitySensor, enabled));
	}
	if (!node->enabled) return;
	/* are we enabled? */

	#ifdef SEVERBOSE
	printf ("do_VisibilitySensorTick, samples %d\n",node->__samples);
	#endif
	
	if (node->__Samples > 0) {
		if (!node->isActive) {
			#ifdef SEVERBOSE
			printf ("visibilitysensor - now active\n");
			#endif

			node->isActive = 1;
			node->enterTime = TickTime();
			MARK_EVENT (ptr, offsetof(struct X3D_VisibilitySensor, isActive));
			MARK_EVENT (ptr, offsetof(struct X3D_VisibilitySensor, enterTime));
		}
	} else {
		if (node->isActive) {
			#ifdef SEVERBOSE
			printf ("visibilitysensor - going inactive\n");
			#endif

			node->isActive = 0;
			node->exitTime = TickTime();
			MARK_EVENT (ptr, offsetof(struct X3D_VisibilitySensor, isActive));
			MARK_EVENT (ptr, offsetof(struct X3D_VisibilitySensor, exitTime));
		}
	}
	node->__Samples = 0; //clear for next frame count
}
