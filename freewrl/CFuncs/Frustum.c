/*******************************************************************
 Copyright (C) 2004 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*********************************************************************
 * OLD - NOW USE Occlusion tests
 * Frustum calculations. Definitive work (at least IMHO) is thanks to
 * Steven Baker - look at http://sjbaker.org/steve/omniv/frustcull.html/
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


/* Occlusion VisibilitySensor code */
#ifdef OCCLUSION
GLuint *OccQueries = 0;
void* *OccNodes = 0;
int *OccActive = 0;
GLint *OccSamples = 0;
/*
#define MAXOCCQUERIES 100
GLuint OccQueries[MAXOCCQUERIES];
void* OccNodes[MAXOCCQUERIES];
int OccActive[MAXOCCQUERIES];
GLint OccSamples[MAXOCCQUERIES];
*/

int maxShapeFound = 0;
int OccQuerySize=0;
GLint queryCounterBits;
int OccInitialized = FALSE;
#endif

/* take the measurements of a geometry (eg, box), and save it. Note
 * that what is given is a Shape, the values get pushed up to the
 * Geometries grouping node parent. */


/* this is used for collision in transformChildren - don't bother going through
   children of a transform if there is nothing close... */

void setExtent(float x, float y, float z, struct X3D_Box *me) {
	int c,d;
	struct X3D_Box *shapeParent;
	struct X3D_Box *geomParent;

	/* printf ("setExtent - Shape node has %d parents\n",me->_nparents); */
	for (c=0; c<(me->_nparents); c++) {
		/*printf ("parent %d of %d is %d\n",c,me,me->_parents[c]);*/
		shapeParent = (struct X3D_Box *)me->_parents[c];
		/*printf ("setExtent - Geometry has %d parents \n",shapeParent->_nparents);*/
		/*printf ("parent %d of %d is %d\n",c,shapeParent,shapeParent->_parents[c]);*/

		for (d=0; d<(shapeParent->_nparents); d++) {
			geomParent = (struct X3D_Box *)shapeParent->_parents[d];
			if (x > geomParent->_extent[0]) geomParent->_extent[0] = x;
			if (y > geomParent->_extent[1]) geomParent->_extent[1] = y;
			if (z > geomParent->_extent[2]) geomParent->_extent[2] = z;
		}
	}
	/* printf ("setExtent, for %f %f %f, node %d\n",x,y,z,me); */
}

/* for children nodes; set the parent grouping nodes extent - we expect the center
 * of the group to be passed in in the floats x,y,z */
void propagateExtent(float x, float y, float z, struct X3D_Box *me) {
	int i;
	struct X3D_Box *parent;

	/*printf ("propextent, xDistoxcenter %f %f %f, myextent %f %f %f me %d parents %d\n",*/
	/*		x,y,z,me->_extent[0],me->_extent[1],me->_extent[2],me,*/
	/*		me->_nparents);*/

	/* calculate the maximum of the current position, and add the previous extent */
	x =fabs(x)+me->_extent[0];y=fabs(y)+me->_extent[1];z=fabs(z)+me->_extent[2];

	for (i=0; i<(me->_nparents); i++) {
		parent = (struct X3D_Box *)me->_parents[i];
		if (x > parent->_extent[0]) parent->_extent[0] = x;
		if (y > parent->_extent[1]) parent->_extent[1] = y;
		if (z > parent->_extent[2]) parent->_extent[2] = z;
	/*	printf ("propextent, me %d my parent %d is %d ext %4.2f %4.2f %4.2f\n",me,i,parent, parent->_extent[0],parent->_extent[1],parent->_extent[2]);*/
	}
}

void BoundingBox(struct SFColor xDistc,struct SFColor xDists) {
#ifdef BOUNDINGBOX
	float x,y,z;
	x = xDists.c[0];
	y = xDists.c[1];
	z = xDists.c[2];

	if ((x<0.001) && (y<0.001) & (z<0.001)) return;

	/* calculate distance to this box from the Frustum */


	/* show a bounding box around each grouping node */
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);

	glColor3f(1.0, 0.0, 0.0);

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

void recordDistance(struct X3D_Transform *nod) {
	GLdouble modelMatrix[16];
	int retval;
	int xcount, pointok;

	retval = 0;
	xcount=0;
	pointok=0;

	fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	nod->bboxCenter.c[0] = modelMatrix[12];
	nod->bboxCenter.c[1] = modelMatrix[13];
	nod->bboxCenter.c[2] = modelMatrix[14];

	nod->_dist = modelMatrix[14];
}

/***************************************************************************/


void OcclusionStartofEventLoop() {
	int i;

        #ifdef OCCLUSION

	/* have we been through this yet? */
	if (OccInitialized == FALSE) {
        	/* printf ("aqDisplayThread, extensions %s\n",glGetString(GL_EXTENSIONS)); */
        	if (strstr(glGetString(GL_EXTENSIONS),"GL_ARB_occlusion_query") != 0) {
        	        /* printf ("have OcclusionQuery\n"); */

			OccQuerySize = 100;
			OccQueries = malloc (sizeof(int) * OccQuerySize);
			OccNodes = malloc (sizeof (void *) * OccQuerySize);
			OccActive = malloc (sizeof (int) * OccQuerySize);
			OccSamples = malloc (sizeof (int) * OccQuerySize);
                	glGenQueries(OccQuerySize,OccQueries);
			OccInitialized = TRUE;
			for (i=0; i<OccQuerySize; i++) {
				OccNodes[i] = 0;
				OccSamples[i]=0;
			}

        	}

	}

	/* can we do OcclusionQueries? */
	if (!OccInitialized) return;

	/* did we find more shapes than before? */
        if (maxShapeFound > OccQuerySize) {
                printf ("have to regen queries\n");
		glDeleteQueries (OccQuerySize, OccQueries);
		OccQuerySize = maxShapeFound + 100;
		OccQueries = realloc (OccQueries,sizeof (int) * maxShapeFound);
		OccNodes = realloc (OccNodes,sizeof (void *) * maxShapeFound);
		OccActive = realloc (OccActive,sizeof (int) * maxShapeFound);
		OccSamples = realloc (OccSamples,sizeof (int) * maxShapeFound);
                glGenQueries(OccQuerySize,OccQueries);
		for (i=0; i<OccQuerySize; i++) {
			OccNodes[i] = 0;
			OccSamples[i]=0;
		}

       }
        maxShapeFound = -1;

        /* glGetQueryiv(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &queryCounterBits);
        printf ("queryCounterBits %d\n",queryCounterBits); */
        #endif

}


void OcclusionCulling ()  {


	#ifdef OCCLUSION
	int i;
	struct X3D_Shape *xx;
	#endif
	
	
	
	if (maxShapeFound >= 0) {

		/* lets look at "producers" - Shapes, and VisibilitySensors first */
		for (i=0; i<=maxShapeFound; i++) {
		        glGetQueryObjectiv (OccQueries[i], GL_QUERY_RESULT, &OccSamples[i]);
			if (OccNodes[i] != 0) {
				xx = (struct X3D_Shape *) OccNodes[i];

				/* if this is a VisibilitySensor, record the samples */
				if (xx->_nodeType == NODE_VisibilitySensor) {
					/* printf ("vis found\n"); */
					((struct X3D_VisibilitySensor *)xx)->__Samples =  OccSamples[i]; 
				}
	
	
				/* is this is Shape? */
				else if (xx->_nodeType == NODE_Shape) {
		        		printf ("Occ %d fragments %d active %d ",i,OccSamples[i],OccActive[i]);
					printf (" nodeType %s",stringNodeType(xx->_nodeType));
					xx = (struct X3D_Box *) xx->geometry;
					if (xx != 0) {
						printf (" (%s)",stringNodeType(xx->_nodeType));
					}
					printf ("\n");
	
					if (OccSamples[i] > 0) {
						update_renderFlag (xx,VF_hasVisibleChildren  |
								VF_hasGeometryChildren  |
								VF_hasBeenScannedForGeometryChildren);
					} else {
						update_renderFlag (xx,
								VF_hasGeometryChildren |
								VF_hasBeenScannedForGeometryChildren);
					}
								
				}
			}
		}
	
		for (i=0; i<=maxShapeFound; i++) {
			if (OccNodes[i] != 0) {
				xx = (struct X3D_Shape *) OccNodes[i];
		
				if (xx->_nodeType == NODE_Transform) {
		        		printf ("Occ %d fragments %d active %d ",i,OccSamples[i],OccActive[i]);
					printf (" nodeType %s",stringNodeType(xx->_nodeType));
					printf (" %d renderFlags (",xx,xx->_renderFlags);
					if (xx->_renderFlags & VF_hasGeometryChildren) printf (" GEOM ");
					if (xx->_renderFlags & VF_hasVisibleChildren) printf (" VIS ");
					if (xx->_renderFlags & VF_hasBeenScannedForGeometryChildren) printf (" SCANNED ");
					printf (")\n");
	
					/* remove the hasVisibleChildren flag */
					xx->_renderFlags = xx->_renderFlags & VF_removeHasVisibleChildren;
				}
			}
			OccActive[i] = FALSE;
		}
	}
}

