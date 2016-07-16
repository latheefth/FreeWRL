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

//PROXIMITYSENSOR(ProximitySensor,center,,);

/* ProximitySensor and GeoProximitySensor are same "code" at this stage of the game */
//#define PROXIMITYSENSOR(type,center,initializer1,initializer2) 
void proximity_ProximitySensor (struct X3D_ProximitySensor *node) {
	/* Viewer pos = t_r2 */
	double cx,cy,cz;
	double len;
	struct point_XYZ dr1r2;
	struct point_XYZ dr2r3;
	struct point_XYZ nor1,nor2;
	struct point_XYZ ins;
	static const struct point_XYZ yvec = {0,0.05,0};
	static const struct point_XYZ zvec = {0,0,-0.05};
	static const struct point_XYZ zpvec = {0,0,0.05};
	static const struct point_XYZ orig = {0,0,0};
	struct point_XYZ t_zvec, t_yvec, t_orig, t_center;
	GLDOUBLE modelMatrix[16];
	GLDOUBLE projMatrix[16];
	GLDOUBLE view2prox[16];
 
	if(!((node->enabled))) return;
	//initializer1
	//initializer2
 
	/* printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/ 
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/ 
 
	/* transforms viewers coordinate space into sensors coordinate space. 
	 * this gives the orientation of the viewer relative to the sensor. 
	 */ 
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix); 
	FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projMatrix); 
	FW_GLU_UNPROJECT(orig.x,orig.y,orig.z,modelMatrix,projMatrix,viewport,
		&t_orig.x,&t_orig.y,&t_orig.z);
	FW_GLU_UNPROJECT(zvec.x,zvec.y,zvec.z,modelMatrix,projMatrix,viewport,
		&t_zvec.x,&t_zvec.y,&t_zvec.z);
	FW_GLU_UNPROJECT(yvec.x,yvec.y,yvec.z,modelMatrix,projMatrix,viewport,
		&t_yvec.x,&t_yvec.y,&t_yvec.z);
	matinverse(view2prox,modelMatrix);
    transform(&t_center,&orig, view2prox);
 
 
	/*printf ("\n"); 
	printf ("unprojected, t_orig (0,0,0) %lf %lf %lf\n",t_orig.x, t_orig.y, t_orig.z); 
	printf ("unprojected, t_yvec (0,0.05,0) %lf %lf %lf\n",t_yvec.x, t_yvec.y, t_yvec.z); 
	printf ("unprojected, t_zvec (0,0,-0.05) %lf %lf %lf\n",t_zvec.x, t_zvec.y, t_zvec.z); 
	*/ 
	cx = t_center.x - ((node->center ).c[0]); 
	cy = t_center.y - ((node->center ).c[1]); 
	cz = t_center.z - ((node->center ).c[2]); 
 
	if(((node->size).c[0]) == 0 || ((node->size).c[1]) == 0 || ((node->size).c[2]) == 0) return; 
 
	if(fabs(cx) > ((node->size).c[0])/2 || 
	   fabs(cy) > ((node->size).c[1])/2 || 
	   fabs(cz) > ((node->size).c[2])/2) return; 
	/* printf ("within (Geo)ProximitySensor\n"); */ 
 
	/* Ok, we now have to compute... */ 
	(node->__hit) /*cget*/ = 1; 
 
	/* Position */ 
	((node->__t1).c[0]) = (float)t_center.x; 
	((node->__t1).c[1]) = (float)t_center.y; 
	((node->__t1).c[2]) = (float)t_center.z; 
 
	VECDIFF(t_zvec,t_orig,dr1r2);  /* Z axis */ 
	VECDIFF(t_yvec,t_orig,dr2r3);  /* Y axis */ 
 
	/* printf ("      dr1r2 %lf %lf %lf\n",dr1r2.x, dr1r2.y, dr1r2.z); 
	printf ("      dr2r3 %lf %lf %lf\n",dr2r3.x, dr2r3.y, dr2r3.z); 
	*/ 
 
	len = sqrt(VECSQ(dr1r2)); VECSCALE(dr1r2,1/len); 
	len = sqrt(VECSQ(dr2r3)); VECSCALE(dr2r3,1/len); 
 
	/* printf ("scaled dr1r2 %lf %lf %lf\n",dr1r2.x, dr1r2.y, dr1r2.z); 
	printf ("scaled dr2r3 %lf %lf %lf\n",dr2r3.x, dr2r3.y, dr2r3.z); 
	*/ 
 
	/* 
	printf("PROX_INT: (%f %f %f) (%f %f %f) (%f %f %f)\n (%f %f %f) (%f %f %f)\n", 
		t_orig.x, t_orig.y, t_orig.z, 
		t_zvec.x, t_zvec.y, t_zvec.z, 
		t_yvec.x, t_yvec.y, t_yvec.z, 
		dr1r2.x, dr1r2.y, dr1r2.z, 
		dr2r3.x, dr2r3.y, dr2r3.z 
		); 
	*/ 
 
	if(fabs(VECPT(dr1r2, dr2r3)) > 0.001) { 
		printf ("Sorry, can't handle unevenly scaled ProximitySensors yet :(" 
		  "dp: %f v: (%f %f %f) (%f %f %f)\n", VECPT(dr1r2, dr2r3), 
		  	dr1r2.x,dr1r2.y,dr1r2.z, 
		  	dr2r3.x,dr2r3.y,dr2r3.z 
			); 
		return; 
	} 
 
 
	if(APPROX(dr1r2.z,1.0)) { 
		/* rotation */ 
		((node->__t2).c[0]) = (float) 0; 
		((node->__t2).c[1]) = (float) 0; 
		((node->__t2).c[2]) = (float) 1; 
		((node->__t2).c[3]) = (float) atan2(-dr2r3.x,dr2r3.y); 
	} else if(APPROX(dr2r3.y,1.0)) { 
		/* rotation */ 
		((node->__t2).c[0]) = (float) 0; 
		((node->__t2).c[1]) = (float) 1; 
		((node->__t2).c[2]) = (float) 0; 
		((node->__t2).c[3]) = (float) atan2(dr1r2.x,dr1r2.z); 
	} else { 
		/* Get the normal vectors of the possible rotation planes */ 
		nor1 = dr1r2; 
		nor1.z -= 1.0; 
		nor2 = dr2r3; 
		nor2.y -= 1.0; 
 
		/* Now, the intersection of the planes, obviously cp */ 
		VECCP(nor1,nor2,ins); 
 
		len = sqrt(VECSQ(ins)); VECSCALE(ins,1/len); 
 
		/* the angle */ 
		VECCP(dr1r2,ins, nor1); 
		VECCP(zpvec, ins, nor2); 
		len = sqrt(VECSQ(nor1)); VECSCALE(nor1,1/len); 
		len = sqrt(VECSQ(nor2)); VECSCALE(nor2,1/len); 
		VECCP(nor1,nor2,ins); 
 
		((node->__t2).c[3]) = (float) -atan2(sqrt(VECSQ(ins)), VECPT(nor1,nor2)); 
 
		/* rotation  - should normalize sometime... */ 
		((node->__t2).c[0]) = (float) ins.x; 
		((node->__t2).c[1]) = (float) ins.y; 
		((node->__t2).c[2]) = (float) ins.z; 
	} 
	/* 
	printf("NORS: (%f %f %f) (%f %f %f) (%f %f %f)\n", 
		nor1.x, nor1.y, nor1.z, 
		nor2.x, nor2.y, nor2.z, 
		ins.x, ins.y, ins.z 
	); 
	*/ 
} 




/* ProximitySensor code for ClockTick */
void do_ProximitySensorTick( void *ptr) {
	struct X3D_ProximitySensor *node = (struct X3D_ProximitySensor *)ptr;

	/* if not enabled, do nothing */
	if (!node) return;
	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_ProximitySensor, enabled));
	}
	if (!node->enabled) return;

	/* did we get a signal? */
	if (node->__hit) {
		if (!node->isActive) {
			#ifdef SEVERBOSE
			printf ("PROX - initial defaults\n");
			#endif

			node->isActive = TRUE;
			node->enterTime = TickTime();
			MARK_EVENT (ptr, offsetof(struct X3D_ProximitySensor, isActive));
			MARK_EVENT (ptr, offsetof(struct X3D_ProximitySensor, enterTime));
		}

		/* now, has anything changed? */
		if (memcmp ((void *) &node->position_changed,(void *) &node->__t1,sizeof(struct SFColor))) {
			#ifdef SEVERBOSE
			printf ("PROX - position changed!!! \n");
			#endif

			memcpy ((void *) &node->position_changed,
				(void *) &node->__t1,sizeof(struct SFColor));
			MARK_EVENT (ptr, offsetof(struct X3D_ProximitySensor, position_changed));
		}
		if (memcmp ((void *) &node->orientation_changed, (void *) &node->__t2,sizeof(struct SFRotation))) {
			#ifdef SEVERBOSE
			printf  ("PROX - orientation changed!!!\n ");
			#endif

			memcpy ((void *) &node->orientation_changed,
				(void *) &node->__t2,sizeof(struct SFRotation));
			MARK_EVENT (ptr, offsetof(struct X3D_ProximitySensor, orientation_changed));
		}
	} else {
		if (node->isActive) {
			#ifdef SEVERBOSE
			printf ("PROX - stopping\n");
			#endif

			node->isActive = FALSE;
			node->exitTime = TickTime();
			MARK_EVENT (ptr, offsetof(struct X3D_ProximitySensor, isActive));

			MARK_EVENT (ptr, offsetof(struct X3D_ProximitySensor, exitTime));
		}
	}
	node->__hit=FALSE;
}



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
		}
		if(isIn){
			//check near and far planes
			transformMBB(smin,smax,modelMatrix,shapeMBBmin,shapeMBBmax); 
			isIn = isIn && smin[2] < nearplane; //these are -ve numbers - z is positive backwards
			isIn = isIn && smax[2] > farplane; 
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
/* don't need - all done in do_TransformSensor
void other_TransformSensor (struct X3D_TransformSensor *node) {
	// if not enabled, do nothing 
	if (!node) return;
	if (!node->enabled) 
		return;

	{
		//aabb method
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
			////ihit = frustumHitsMBB(node->_extent);
			//if(ihit){
			//}
		}
	}
}
*/
 int overlapMBBs(GLDOUBLE *MBBmin1, GLDOUBLE *MBBmax1, GLDOUBLE *MBBmin2, GLDOUBLE* MBBmax2);

/*
TransformSensor
 http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/envsensor.html#TransformSensor
 - added in specs v3.2
 - a targetObject (Node) can be USEd in multiple places, each place with a different modelview matrix state
 - so how implement on the node end? This is similar/analogous to the problem with the picksensor component:
    freewrl is great at viewer / node interactions, but not node/node. we seem to be missing something. But what?
 - a more general node/node interaction table for picksensors, transformsensors and any other node-node interaction
 General USE-USE system
 how: 
 1) renderfuncs struct USEHIT { node *node; modelviewMatrix mvm; }
		functions: usehit_add(node*,mvm,flag), USEHIT = usehit_next(node*); usehit_clear();
 2) (sensornode*,do_sensorfunc) tuple registered, sensornode knows target node and 
     a) flags target and self on one pass with VF_USE, 
     b) then searches on next pass for self and other in double loop and does all USE_USE combinations
     c) end of do_first() calls usehit_clear() for fresh transforms on next pass
*/


// ticks are a good place to summarize activites from various places around the scenegraph
// do_tick is called once per frame, from outside of render_hier, so there's no modelview matrix on stack
// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/envsensor.html#TransformSensor
// "Instanced (DEF/USE) TransformSensor nodes use the union of all the boxes to check for enter and exit."
void do_TransformSensorTick (void *ptr) {
	int ishit,i;
	usehit *mehit, *uhit;
	struct X3D_Node *unode,*menode;
	struct X3D_TransformSensor *node = (struct X3D_TransformSensor *) ptr;

	// if not enabled, do nothing 
	if (!node) return;
	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_TransformSensor, enabled));
	}
	// are we enabled? 
	if (!node->enabled) return;
	//if the sensor has zero size its equivalent to not-enabled - can't send events accroding to specs
	if(((node->size).c[0]) <= 0.0f || ((node->size).c[1]) <= 0.0f || ((node->size).c[2]) <= 0.0f) return; 

	#ifdef SEVERBOSE
	printf ("do_TransformSensorTick enabled\n");
	#endif

	//temp clear hit flag
	ishit = 0;
	mehit = NULL;
	unode = node->targetObject;
	menode = (struct X3D_Node*)node; //upcaste
	if(unode){
		//check all USE-USE combinations of this node and targetObject
		//find next this
		while(mehit = usehit_next(menode,mehit)){
			int iret;
			double meinv[16],memin[3],memax[3];
			float emin[3], emax[3], halfsize[3];

			matinverseAFFINE(meinv,mehit->mvm);
			//iret = __gluInvertMatrixd( mehit->mvm, meinv);

			if(0){
				//check inverse
				double ident[16];
				int j;
				matmultiplyAFFINE(ident,meinv,mehit->mvm);

				printf("inverse check do_TransformSensor\n");
				for(i=0;i<4;i++){
					for(j=0;j<4;j++) printf("%lf ",ident[i*3+j]);
					printf("\n");
				}
				printf("\n");
			}
			//update extent on me, in case center or size has changed
			vecscale3f(halfsize,node->size.c,.5f);
			vecadd3f(emax, node->center.c, halfsize);
			vecdif3f(emin, node->center.c, halfsize);
			for(i=0;i<3;i++)
			{
				node->_extent[i*2 + 1] = emin[i];
				node->_extent[i*2]     = emax[i];
			}
			for(i=0;i<3;i++)
			{
				memin[i] = node->_extent[i*2 + 1];
				memax[i] = node->_extent[i*2];
			}

			//find next target
			uhit = NULL;
			while(uhit = usehit_next(unode,uhit)){
				//see if they intersect, if so do something about it
				//-prepare matrixTarget2this
				double u2me[16], umin[3],umax[3],uumin[3],uumax[3];
				matmultiplyAFFINE(u2me,uhit->mvm,meinv);
				//-transform target AABB/MBB from target space to this space
				for(i=0;i<3;i++)
				{
					umin[i] = unode->_extent[i*2 + 1];
					umax[i] = unode->_extent[i*2];
				}
				transformMBB(uumin,uumax,u2me,umin,umax); 
				//-see if AABB intersect
				if( overlapMBBs(memin, memax, uumin, uumax) ){
					//-if so take action
					static const struct point_XYZ yvec = {0,0.05,0};
					static const struct point_XYZ zvec = {0,0,-0.05};
					static const struct point_XYZ zpvec = {0,0,0.05};
					static const struct point_XYZ orig = {0,0,0};
					struct point_XYZ t_zvec, t_yvec, t_orig, t_center;
					struct point_XYZ nor1,nor2;
					struct point_XYZ ins;
					double len;
					struct point_XYZ dr1r2;
					struct point_XYZ dr2r3;
					double t1u[3], t1me[3];

					ishit++;
					if (!node->isActive) {
						#ifdef SEVERBOSE
						printf ("transformensor - now active\n");
						#endif

						node->isActive = 1;
						node->enterTime = TickTime();
						MARK_EVENT (ptr, offsetof(struct X3D_TransformSensor, isActive));
						MARK_EVENT (ptr, offsetof(struct X3D_TransformSensor, enterTime));
					}
					// has target position or orientation changed wrt transform sensor? 
					// if so send position_changed / orientation_changed events
					//transform position
					for(i=0;i<3;i++) t1u[i] = (umin[i] + umax[i])*.5;
					transformAFFINEd(t1me,t1u,u2me);
					for(i=0;i<3;i++) node->__t1.c[i] = t1me[i] - node->center.c[i];
					if (memcmp ((void *) &node->position_changed,(void *) &node->__t1,sizeof(struct SFColor))) {
						#ifdef SEVERBOSE
						printf ("PROX - position changed!!! \n");
						#endif

						memcpy ((void *) &node->position_changed,
							(void *) &node->__t1,sizeof(struct SFColor));
						MARK_EVENT (ptr, offsetof(struct X3D_TransformSensor, position_changed));
					}
					//transform orientation
					transformAFFINE(&t_yvec,&yvec,u2me);
					transformAFFINE(&t_zvec,&zvec,u2me);
					transformAFFINE(&t_orig,&orig,u2me);
					VECDIFF(t_zvec,t_orig,dr1r2);  /* Z axis */ 
					VECDIFF(t_yvec,t_orig,dr2r3);  /* Y axis */ 
 
					/* printf ("      dr1r2 %lf %lf %lf\n",dr1r2.x, dr1r2.y, dr1r2.z); 
					printf ("      dr2r3 %lf %lf %lf\n",dr2r3.x, dr2r3.y, dr2r3.z); 
					*/ 
 
					len = sqrt(VECSQ(dr1r2)); VECSCALE(dr1r2,1/len); 
					len = sqrt(VECSQ(dr2r3)); VECSCALE(dr2r3,1/len); 
 
					/* printf ("scaled dr1r2 %lf %lf %lf\n",dr1r2.x, dr1r2.y, dr1r2.z); 
					printf ("scaled dr2r3 %lf %lf %lf\n",dr2r3.x, dr2r3.y, dr2r3.z); 
					*/ 
 
					/* 
					printf("PROX_INT: (%f %f %f) (%f %f %f) (%f %f %f)\n (%f %f %f) (%f %f %f)\n", 
						t_orig.x, t_orig.y, t_orig.z, 
						t_zvec.x, t_zvec.y, t_zvec.z, 
						t_yvec.x, t_yvec.y, t_yvec.z, 
						dr1r2.x, dr1r2.y, dr1r2.z, 
						dr2r3.x, dr2r3.y, dr2r3.z 
						); 
					*/ 
 
					if(fabs(VECPT(dr1r2, dr2r3)) > 0.001) { 
						printf ("Sorry, can't handle unevenly scaled ProximitySensors yet :(" 
						  "dp: %f v: (%f %f %f) (%f %f %f)\n", VECPT(dr1r2, dr2r3), 
		  					dr1r2.x,dr1r2.y,dr1r2.z, 
		  					dr2r3.x,dr2r3.y,dr2r3.z 
							); 
						return; 
					} 
 
 
					if(APPROX(dr1r2.z,1.0)) { 
						/* rotation */ 
						((node->__t2).c[0]) = (float) 0; 
						((node->__t2).c[1]) = (float) 0; 
						((node->__t2).c[2]) = (float) 1; 
						((node->__t2).c[3]) = (float) atan2(-dr2r3.x,dr2r3.y); 
					} else if(APPROX(dr2r3.y,1.0)) { 
						/* rotation */ 
						((node->__t2).c[0]) = (float) 0; 
						((node->__t2).c[1]) = (float) 1; 
						((node->__t2).c[2]) = (float) 0; 
						((node->__t2).c[3]) = (float) atan2(dr1r2.x,dr1r2.z); 
					} else { 
						/* Get the normal vectors of the possible rotation planes */ 
						nor1 = dr1r2; 
						nor1.z -= 1.0; 
						nor2 = dr2r3; 
						nor2.y -= 1.0; 
 
						/* Now, the intersection of the planes, obviously cp */ 
						VECCP(nor1,nor2,ins); 
 
						len = sqrt(VECSQ(ins)); VECSCALE(ins,1/len); 
 
						/* the angle */ 
						VECCP(dr1r2,ins, nor1); 
						VECCP(zpvec, ins, nor2); 
						len = sqrt(VECSQ(nor1)); VECSCALE(nor1,1/len); 
						len = sqrt(VECSQ(nor2)); VECSCALE(nor2,1/len); 
						VECCP(nor1,nor2,ins); 
 
						((node->__t2).c[3]) = (float) -atan2(sqrt(VECSQ(ins)), VECPT(nor1,nor2)); 
 
						/* rotation  - should normalize sometime... */ 
						((node->__t2).c[0]) = (float) ins.x; 
						((node->__t2).c[1]) = (float) ins.y; 
						((node->__t2).c[2]) = (float) ins.z; 
					} 

					if (memcmp ((void *) &node->orientation_changed, (void *) &node->__t2,sizeof(struct SFRotation))) {
						#ifdef SEVERBOSE
						printf  ("PROX - orientation changed!!!\n ");
						#endif

						memcpy ((void *) &node->orientation_changed,
							(void *) &node->__t2,sizeof(struct SFRotation));
						MARK_EVENT (ptr, offsetof(struct X3D_TransformSensor, orientation_changed));
					}
				}
			}
		}

		if(!ishit){
			if (node->isActive) {
				#ifdef SEVERBOSE
				printf ("transformsensor - going inactive\n");
				#endif

				node->isActive = 0;
				node->exitTime = TickTime();
				MARK_EVENT (ptr, offsetof(struct X3D_TransformSensor, isActive));
				MARK_EVENT (ptr, offsetof(struct X3D_TransformSensor, exitTime));
			}
		}

		//ask this node and target node to both save their modelviewmatrix for each USE, when visited, on the upcoming frame (do_ called from do_first())
		node->targetObject->_renderFlags |= VF_USE;
	}
	node->_renderFlags |= VF_USE;

}
