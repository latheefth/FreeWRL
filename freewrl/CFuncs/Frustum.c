/*******************************************************************
 Copyright (C) 2004 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*********************************************************************
 * Frustum calculations. Definitive work (at least IMHO) is thanks to
 * Steven Baker - look at http://sjbaker.org/steve/omniv/frustcull.html
 *
 * Thanks Steve!
 * 
 */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"


#include <math.h>
#ifdef AQUA
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "Structs.h"
#include "headers.h"

static double frustumConeAngle = 0.0;
  
/* take the measurements of a geometry (eg, box), and save it. Note 
 * that what is given is a Shape, the values get pushed up to the
 * Geometries grouping node parent. */

void setExtent(float x, float y, float z, struct VRML_Box *me) {
#ifdef BOUNDINGBOX
	int c,d;
	struct VRML_Box *shapeParent;
	struct VRML_Box *geomParent;

	//printf ("setExtent - Shape node has %d parents\n",me->_nparents);
	for (c=0; c<(me->_nparents); c++) {
		//printf ("parent %d of %d is %d\n",c,me,me->_parents[c]);
		shapeParent = (struct VRML_Box *)me->_parents[c];
		//printf ("setExtent - Geometry has %d parents \n",shapeParent->_nparents); 
		//printf ("parent %d of %d is %d\n",c,shapeParent,shapeParent->_parents[c]);

		for (d=0; d<(shapeParent->_nparents); d++) {
			geomParent = (struct VRML_Box *)shapeParent->_parents[d];
			if (x > geomParent->_extent[0]) geomParent->_extent[0] = x; 
			if (y > geomParent->_extent[1]) geomParent->_extent[1] = y; 
			if (z > geomParent->_extent[2]) geomParent->_extent[2] = z; 
		}
	}
	//printf ("setExtent, for %f %f %f, node %d\n",x,y,z,me);
#endif

}

/* for children nodes; set the parent grouping nodes extent - we expect the center
 * of the group to be passed in in the floats x,y,z */
void propagateExtent(float x, float y, float z, struct VRML_Box *me) {
#ifdef BOUNDINGBOX
	int i;
	struct VRML_Box *parent;

	//printf ("propextent, bboxcenter %f %f %f, myextent %f %f %f\n",
	//		x,y,z,me->_extent[0],me->_extent[1],me->_extent[2]);

	/* calculate the maximum of the current position, and add the previous extent */
	x =fabs(x)+me->_extent[0];y=fabs(y)+me->_extent[1];z=fabs(z)+me->_extent[2];
	
	for (i=0; i<(me->_nparents); i++) {
		parent = (struct VRML_Box *)me->_parents[i];
		if (x > parent->_extent[0]) parent->_extent[0] = x; 
		if (y > parent->_extent[1]) parent->_extent[1] = y; 
		if (z > parent->_extent[2]) parent->_extent[2] = z; 
	}
#endif
}

void BoundingBox(struct SFColor bbc,struct SFColor bbs, int PIV) {
#ifdef BOUNDINGBOX
	float x,y,z;
	x = bbs.c[0];
	y = bbs.c[1];
	z = bbs.c[2];
	//printf ("BoundingBox, size %f %f %f %d\n",x,y,z, PIV);
	if ((x<0.001) && (y<0.001) & (z<0.001)) return;
	if (PIV == 0) return;


	return;
	/* calculate distance to this box from the Frustum */

	
	/* show a bounding box around each grouping node */
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_COLOR_MATERIAL); 
	glDisable(GL_CULL_FACE);

	if (PIV >= 2) {

		glColor3f(1.0, 1.0, 0.0);
	} else if (PIV >= 1) {
		glColor3f(0.0, 1.0, 0.0);
	}
	else glColor3f(0, 0.0, 1.0);

	/* top of box */
	glBegin(GL_LINE_STRIP);
	glVertex3f(-x, y, z);
	glVertex3f(x, y, z);
	glVertex3f(x, y, -z);
	glVertex3f(-x, y, -z);
	glVertex3f(-x, y, z);
	glEnd();

	/* bottom of box */
	glBegin(GL_LINE_STRIP);
	glVertex3f(-x, -y, z);
	glVertex3f(x, -y, z);
	glVertex3f(x, -y, -z);
	glVertex3f(-x, -y, -z);
	glVertex3f(-x, -y, z);
	glEnd();

	/* vertical bars */
	glBegin(GL_LINE_STRIP);
	glVertex3f(-x, y, z);
	glVertex3f(-x, -y, z);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(x, y, z);
	glVertex3f(x, -y, z);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(x, y, -z);
	glVertex3f(x, -y, -z);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(-x, y, -z);
	glVertex3f(-x, -y, -z);
	glEnd();

	glDisable(GL_COLOR_MATERIAL); 
	glPopAttrib();
#endif
}
	
/* set the "static" frustum variables. Changes only when window params change.
 *
 * A Frustum is a truncated pyramid. It is used to test as to whether another
 * cube intersects with it; if so, the other cube is "visible"; if not, the
 * other cube does not need to be rendered.
 *
 * With fast GPU's, a few extra triangles is not a worry, but matimatical 
 * calculations on the main cpu are, so we use a Cone for Frustum testing, not
 * a 6 sided truncated pyramid.
 *
 * check out the paper by Dave Eberly, Magic-Sofware.com for this technique.
 *
 * Used for speeding up large worlds. */

void calculateFrustumCone () {
	GLdouble mod[16];
	GLdouble proj[16];
	
#ifdef BOUNDINGBOX

	glGetDoublev (GL_PROJECTION_MATRIX, proj);
	glGetDoublev (GL_MODELVIEW_MATRIX, mod);

	//printf ("calculateFrustum\n");
	//printf ("nearPlane    %f\n",nearPlane);
	//printf ("farPlane     %f\n",farPlane);
	//printf ("screenRatio  %f\n",screenRatio);
	//printf ("fieldofview  %f\n",fieldofview);
	//printf ("screenWidth  %d\n",screenWidth);
	//printf ("screenHeight %d\n",screenHeight);

	frustumConeAngle = fieldofview;
	//printf ("frust axis %f %f %f\n",
//			0.0, 0.0, 0.0);
//	printf ("Frust vertex %f %f %f\n",
//			0.0,0.0,0.0);
#endif
}

/************************************************************
 * inCheck - check if a point is "within" expected values
 *
 * What we do is check a point in X and in Y, using the viewport
 * angle.
 *
 * The equation is simple trig - angle A is 1/2 of viewport, we
 * know the distance, etc:
 *
 *     C       a      B
 *     ----------------
 *     |              /
 *     |             /
 *     |            / 
 *     |           /
 *     |          /
 *     |         /
 *    b|        / c
 *     |       /
 *     |      /
 *     |     /
 *     |    /		A = 1/2 "viewport" diagonal angle
 *     |   /		b = Z distance
 *     |  /		a = maximum distance for a point
 *     | /		Tan(A) = a/b; Tan(A)*b = a
 *     |/
 *     A
 *
 *     if a point, B is greater than a away from the centre, then
 *     we assume that it is outside of the viewing area.
 *
 *     We check x and y seperately.
 *
 *************************************************************/

void inCheck(GLdouble Distance,GLdouble bb,GLdouble cc, int *xcount, int *pointok) {
	GLdouble xx;	
	
	xx = tan(0.3)*Distance;

	//printf ("        comparing %f with %f, %f ",xx, bb,cc);

	// Point is behind viewer
	if (Distance<0.0) {
		//printf (" Xcount %d\n",0);
		return;
	}

	// are both points positive?
	if ((bb>0.0) && (xx > bb) && 
		(cc>0.0) && (xx > cc)) (*xcount)++;
	
	if ((bb>0.0) && (cc>0.0)) (*pointok)++;

	//printf (" Xcount %d pok %d\n",*xcount,*pointok);
}


int PointInView(struct VRML_Transform *nod) {
	GLdouble xx,yy,Distance,bb,cc,dd,ee,ff,ex_X,ex_Y,ex_Z;
	GLdouble modelMatrix[16];
	int retval;
	int xcount, pointok;

	retval = 0;
	xcount=0;
	pointok=0;

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	nod->bboxCenter.c[0] = modelMatrix[12];
	nod->bboxCenter.c[1] = modelMatrix[13];
	nod->bboxCenter.c[2] = modelMatrix[14];

#ifdef BOUNDINGBOX

	// get the x,y values from the modelMatrix
	bb = modelMatrix[12]; // x ish
	cc = modelMatrix[13]; // y ish
	//printf ("\nbb %f cc %f\n",bb,cc);

	// get the extent from the VRML Struct passed in
	ex_X=nod->_extent[0]; 
	ex_Y=nod->_extent[1];
	ex_Z=nod->_extent[2];

	// check the 4 points closer to us
	Distance = -modelMatrix[14]+ex_Z; // distance
	inCheck(Distance, bb+ex_X, cc+ex_Y,&xcount,&pointok);
	inCheck(Distance, -(bb-ex_X), cc+ex_Y,&xcount,&pointok);
	inCheck(Distance, bb+ex_X, -(cc-ex_Y),&xcount,&pointok);
	inCheck(Distance, -(bb-ex_X), -(cc-ex_Y),&xcount,&pointok);

	/* check the 4 points farther from us */
	Distance = -modelMatrix[14]-ex_Z; // distance
	inCheck(Distance, bb+ex_X, cc+ex_Y,&xcount,&pointok);
	inCheck(Distance, -(bb-ex_X), cc+ex_Y,&xcount,&pointok);
	inCheck(Distance, bb+ex_X, -(cc-ex_Y),&xcount,&pointok);
	inCheck(Distance, -(bb-ex_X), -(cc-ex_Y),&xcount,&pointok);

	//printf ("PIV %d %d\n",xcount,pointok);
#endif
	nod->_dist = modelMatrix[14];
	//printf ("getDist - recalculating distance, it is %f for %d\n", 
	//	nod->_dist,nod);
	
	// are all 8 points within the view?
	if (pointok==8) return 2;
	if ((pointok>1) && (xcount>1)) return 1;
	
	
	return retval;
}

