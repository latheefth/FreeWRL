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

void BoundingBox(struct SFColor bbc,struct SFColor bbs) {
#ifdef BOUNDINGBOX
	float x,y,z;
	x = bbs.c[0];
	y = bbs.c[1];
	z = bbs.c[2];
	//printf ("BoundingBox, size %f %f %f\n",x,y,z);
	if ((x<0.001) && (y<0.001) & (z<0.001)) return;

	/* calculate distance to this box from the Frustum */

	
	/* show a bounding box around each grouping node */
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_COLOR_MATERIAL); 
	glDisable(GL_CULL_FACE);

	glColor3f(1.0, 1.0, 0.0);

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

void calculateFrustum () {

#ifdef BOUNDINGBOX
	printf ("calculateFrustum\n");
	printf ("nearPlane    %f\n",nearPlane);
	printf ("farPlane     %f\n",farPlane);
	printf ("screenRatio  %f\n",screenRatio);
	printf ("fieldofview  %f\n",fieldofview);
	printf ("screenWidth  %d\n",screenWidth);
	printf ("screenHeight %d\n",screenHeight);

	frustumConeAngle = fieldofview;
#endif
}

int pointIntersectsCone(struct pt S, struct pt P) {
}
