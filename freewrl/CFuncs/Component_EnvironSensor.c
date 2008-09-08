/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Environmental Sensors Component

*********************************************************************/

#include <math.h>
#include "headers.h"
#include "installdir.h"
#include "LinearAlgebra.h"

/* can we do a VisibiltySensor? Only if we have OpenGL support for OcclusionCulling */
int candoVisibility = TRUE;

void rendVisibilityBox (struct X3D_VisibilitySensor *node);


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
	struct point_XYZ t_zvec, t_yvec, t_orig;
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];

	if(!((node->enabled))) return;

	/* printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

	/* transforms viewers coordinate space into sensors coordinate space.
	 * this gives the orientation of the viewer relative to the sensor.
	 */
	fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(orig.x,orig.y,orig.z,modelMatrix,projMatrix,viewport,
		&t_orig.x,&t_orig.y,&t_orig.z);
	gluUnProject(zvec.x,zvec.y,zvec.z,modelMatrix,projMatrix,viewport,
		&t_zvec.x,&t_zvec.y,&t_zvec.z);
	gluUnProject(yvec.x,yvec.y,yvec.z,modelMatrix,projMatrix,viewport,
		&t_yvec.x,&t_yvec.y,&t_yvec.z);

/*
printf ("\n");
printf ("unprojected, t_orig (0,0,0) %lf %lf %lf\n",t_orig.x, t_orig.y, t_orig.z);
printf ("unprojected, t_yvec (0,0.05,0) %lf %lf %lf\n",t_yvec.x, t_yvec.y, t_yvec.z);
printf ("unprojected, t_zvec (0,0,-0.05) %lf %lf %lf\n",t_zvec.x, t_zvec.y, t_zvec.z);
*/
	cx = t_orig.x - ((node->center).c[0]);
	cy = t_orig.y - ((node->center).c[1]);
	cz = t_orig.z - ((node->center).c[2]);

	if(((node->size).c[0]) == 0 || ((node->size).c[1]) == 0 || ((node->size).c[2]) == 0) return;

	if(fabs(cx) > ((node->size).c[0])/2 ||
	   fabs(cy) > ((node->size).c[1])/2 ||
	   fabs(cz) > ((node->size).c[2])/2) return;

	/* Ok, we now have to compute... */
	(node->__hit) /*cget*/ = 1;

	/* Position */
	((node->__t1).c[0]) = t_orig.x;
	((node->__t1).c[1]) = t_orig.y;
	((node->__t1).c[2]) = t_orig.z;

	VECDIFF(t_zvec,t_orig,dr1r2);  /* Z axis */
	VECDIFF(t_yvec,t_orig,dr2r3);  /* Y axis */
/*
printf ("      dr1r2 %lf %lf %lf\n",dr1r2.x, dr1r2.y, dr1r2.z);
printf ("      dr2r3 %lf %lf %lf\n",dr2r3.x, dr2r3.y, dr2r3.z);
*/

	len = sqrt(VECSQ(dr1r2)); VECSCALE(dr1r2,1/len);
	len = sqrt(VECSQ(dr2r3)); VECSCALE(dr2r3,1/len);
/*
printf ("scaled dr1r2 %lf %lf %lf\n",dr1r2.x, dr1r2.y, dr1r2.z);
printf ("scaled dr2r3 %lf %lf %lf\n",dr2r3.x, dr2r3.y, dr2r3.z);
*/

	#ifdef RENDERVERBOSE
	printf("PROX_INT: (%f %f %f) (%f %f %f) (%f %f %f)\n (%f %f %f) (%f %f %f)\n",
		t_orig.x, t_orig.y, t_orig.z,
		t_zvec.x, t_zvec.y, t_zvec.z,
		t_yvec.x, t_yvec.y, t_yvec.z,
		dr1r2.x, dr1r2.y, dr1r2.z,
		dr2r3.x, dr2r3.y, dr2r3.z
		);
	#endif

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
		((node->__t2).r[0]) = 0;
		((node->__t2).r[1]) = 0;
		((node->__t2).r[2]) = 1;
		((node->__t2).r[3]) = atan2(-dr2r3.x,dr2r3.y);
	} else if(APPROX(dr2r3.y,1.0)) {
		/* rotation */
		((node->__t2).r[0]) = 0;
		((node->__t2).r[1]) = 1;
		((node->__t2).r[2]) = 0;
		((node->__t2).r[3]) = atan2(dr1r2.x,dr1r2.z);
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

		((node->__t2).r[3]) = -atan2(sqrt(VECSQ(ins)), VECPT(nor1,nor2));

		/* rotation  - should normalize sometime... */
		((node->__t2).r[0]) = ins.x;
		((node->__t2).r[1]) = ins.y;
		((node->__t2).r[2]) = ins.z;
	}
	#ifdef RENDERVERBOSE
	printf("NORS: (%f %f %f) (%f %f %f) (%f %f %f)\n",
		nor1.x, nor1.y, nor1.z,
		nor2.x, nor2.y, nor2.z,
		ins.x, ins.y, ins.z
	);
	#endif
}

/* VisibilitySensors - mimic what Shape does to display the box. */


void child_VisibilitySensor (struct X3D_VisibilitySensor *node) {
		if (!node) return;
		if (!node->enabled) return;
		if (!candoVisibility) return;

		/* first time through, if we have a visibility sensor, but do not have the OpenGL ability to
		   use it, we print up a console message */
		if (OccFailed) {
			candoVisibility = FALSE;
			ConsoleMessage("VisibilitySensor: OpenGL on this machine does not support GL_ARB_occlusion_query");
			return;
		}

		RECORD_DISTANCE

		if (render_blend == VF_Blend) { 
                        #ifdef VISIBILITYOCCLUSION

			BEGINOCCLUSIONQUERY
			LIGHTING_OFF
			DISABLE_CULL_FACE 

			rendVisibilityBox(node);
			
			ENABLE_CULL_FACE
			LIGHTING_ON
			
			ENDOCCLUSIONQUERY
                        #endif
		}

}

void rendVisibilityBox (struct X3D_VisibilitySensor *node) {
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


	if NODE_NEEDS_COMPILING {
		/*  have to regen the shape*/

		MARK_NODE_COMPILED

		/*  MALLOC memory (if possible)*/
		if (!node->__points) node->__points = MALLOC (sizeof(struct SFColor)*(24));

		/*  now, create points; 4 points per face.*/
		pt = (float *) node->__points;
		/*  front*/
		*pt++ = cx+x; *pt++ = cy+y; *pt++ = cz+z; *pt++ = cx-x; *pt++ = cy+y; *pt++ = cz+z;
		*pt++ = cx-x; *pt++ = cy-y; *pt++ = cz+z; *pt++ = cx+x; *pt++ = cy-y; *pt++ = cz+z;
		/*  back*/
		*pt++ = cx+x; *pt++ = cy-y; *pt++ = cz-z; *pt++ = cx-x; *pt++ = cy-y; *pt++ = cz-z;
		*pt++ = cx-x; *pt++ = cy+y; *pt++ = cz-z; *pt++ = cx+x; *pt++ = cy+y; *pt++ = cz-z;
		/*  top*/
		*pt++ = cx-x; *pt++ = cy+y; *pt++ = cz+z; *pt++ = cx+x; *pt++ = cy+y; *pt++ = cz+z;
		*pt++ = cx+x; *pt++ = cy+y; *pt++ = cz-z; *pt++ = cx-x; *pt++ = cy+y; *pt++ = cz-z;
		/*  down*/
		*pt++ = cx-x; *pt++ = cy-y; *pt++ = cz-z; *pt++ = cx+x; *pt++ = cy-y; *pt++ = cz-z;
		*pt++ = cx+x; *pt++ = cy-y; *pt++ = cz+z; *pt++ = cx-x; *pt++ = cy-y; *pt++ = cz+z;
		/*  right*/
		*pt++ = cx+x; *pt++ = cy-y; *pt++ = cz+z; *pt++ = cx+x; *pt++ = cy-y; *pt++ = cz-z;
		*pt++ = cx+x; *pt++ = cy+y; *pt++ = cz-z; *pt++ = cx+x; *pt++ = cy+y; *pt++ = cz+z;
		/*  left*/
		*pt++ = cx-x; *pt++ = cy-y; *pt++ = cz+z; *pt++ = cx-x; *pt++ = cy+y; *pt++ = cz+z;
		*pt++ = cx-x; *pt++ = cy+y; *pt++ = cz-z; *pt++ = cx-x; *pt++ = cy-y; *pt++ = cz-z;
	}

	/* note the ALPHA of zero - totally transparent */
	glColor4f(0.0, 1.0, 0.0, 0.0);

	/*  Draw it; assume VERTEX and NORMALS already defined.*/
	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)node->__points);
	glNormalPointer (GL_FLOAT,0,boxnorms);

	/* do the array drawing; sides are simple 0-1-2-3, 4-5-6-7, etc quads */
	glDrawArrays (GL_QUADS, 0, 24);
}


void do_VisibilitySensorTick (void *ptr) {
	struct X3D_VisibilitySensor *node = (struct X3D_VisibilitySensor *) ptr;

	/* are we enabled? */
	if (!node) return;
	if (!node->enabled) return;

	#ifdef SEVERBOSE
	printf ("do_VisibilitySensorTick, samples %d\n",node->__samples);
	#endif
	
	if (node->__Samples > 0) {
		/* we are here... */
                if (!node->isActive) {
                        #ifdef SEVERBOSE
                        printf ("visibilitysensor - now active\n");
                        #endif

                        node->isActive = 1;
                        node->enterTime = TickTime;
                        MARK_EVENT (ptr, offsetof(struct X3D_VisibilitySensor, isActive));
                        MARK_EVENT (ptr, offsetof(struct X3D_VisibilitySensor, enterTime));

                }
	} else {
		/* we are here... */
		if (node->isActive) {
                        #ifdef SEVERBOSE
                        printf ("visibilitysensor - going inactive\n");
                        #endif

                        node->isActive = 0;
                        node->exitTime = TickTime;
                        MARK_EVENT (ptr, offsetof(struct X3D_VisibilitySensor, isActive));
                        MARK_EVENT (ptr, offsetof(struct X3D_VisibilitySensor, exitTime));
		}
	}
}
