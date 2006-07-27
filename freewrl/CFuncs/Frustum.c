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

#include <math.h>
#include "headers.h"


/* Occlusion VisibilitySensor code */
#ifdef OCCLUSION
GLuint *OccQueries = 0;
void* *OccNodes = 0;
int *OccActive = 0;
GLint *OccSamples = 0;

int maxShapeFound = 0;
int OccQuerySize=0;
GLint queryCounterBits;
int OccInitialized = FALSE;
static int OccFailed = FALSE;
#endif

/* take the measurements of a geometry (eg, box), and save it. Note
 * that what is given is a Shape, the values get pushed up to the
 * Geometries grouping node parent. */


/* this is used for collision in transformChildren - don't bother going through
   children of a transform if there is nothing close... */

void setExtent(float maxx, float minx, float maxy, float miny, float maxz, float minz, struct X3D_Box *me) {
	int c,d;
	struct X3D_Box *shapeParent;
	struct X3D_Box *geomParent;

	#ifdef FRUSTUMVERBOSE
	printf ("setExtent - Shape node has %d parents\n",me->_nparents);
	#endif

	for (c=0; c<(me->_nparents); c++) {
		/*printf ("parent %d of %d is %d\n",c,me,me->_parents[c]);*/
		shapeParent = (struct X3D_Box *)me->_parents[c];
		/*printf ("setExtent - Geometry has %d parents \n",shapeParent->_nparents);*/
		/*printf ("parent %d of %d is %d\n",c,shapeParent,shapeParent->_parents[c]);*/

		for (d=0; d<(shapeParent->_nparents); d++) {
			geomParent = (struct X3D_Box *)shapeParent->_parents[d];
			if (maxx > geomParent->EXTENT_MAX_X) geomParent->EXTENT_MAX_X = maxx;
			if (minx < geomParent->EXTENT_MIN_X) geomParent->EXTENT_MIN_X = minx;
			if (maxy > geomParent->EXTENT_MAX_Y) geomParent->EXTENT_MAX_Y = maxy;
			if (miny < geomParent->EXTENT_MIN_Y) geomParent->EXTENT_MIN_Y = miny;
			if (maxz > geomParent->EXTENT_MAX_Z) geomParent->EXTENT_MAX_Z = maxz;
			if (minz < geomParent->EXTENT_MIN_Z) geomParent->EXTENT_MIN_Z = minz;
		}
	}
	/* printf ("setExtent, for %f %f %f, node %d\n",x,y,z,me); */
}

/* for children nodes; set the parent grouping nodes extent - we expect the center
 * of the group to be passed in in the floats x,y,z */

void propagateExtent(struct X3D_Box *me) {
	float minx, miny, minz, maxx, maxy, maxz;
	int i;
	struct X3D_Box *geomParent;
	struct X3D_Transform *trans;

	#ifdef FRUSTUMPRINT
	printf ("propextent Iam %s, myExtent (%f %f) (%f %f) (%f %f) me %d parents %d\n",
			stringNodeType(me->_nodeType),
			me->EXTENT_MAX_X, me->EXTENT_MIN_X,
			me->EXTENT_MAX_Y, me->EXTENT_MIN_Y,
			me->EXTENT_MAX_Z, me->EXTENT_MIN_Z,
			me, me->_nparents);
	#endif


	/* calculate the maximum of the current position, and add the previous extent */
	maxx = me->EXTENT_MAX_X; minx = me->EXTENT_MIN_X;
	maxy = me->EXTENT_MAX_Y; miny = me->EXTENT_MIN_Y;
	maxz = me->EXTENT_MAX_Z; minz = me->EXTENT_MIN_Z;

	/* is this a transform? Should we add in the translated position?? */
	if (me->_nodeType == NODE_Transform) {
		trans = (struct X3D_Transform *)me;
		maxx += trans->bboxCenter.c[0];
		minx += trans->bboxCenter.c[0];
		maxy += trans->bboxCenter.c[1];
		miny += trans->bboxCenter.c[1];
		maxz += trans->bboxCenter.c[2];
		minz += trans->bboxCenter.c[2];
	}
	

	for (i=0; i<(me->_nparents); i++) {
		geomParent = (struct X3D_Box *)me->_parents[i];
		#ifdef FRUSTUMPRINT
		printf ("propextent, me %d my parent %d is %d (%s) ext %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f\n",
			me,i,geomParent, stringNodeType(geomParent->_nodeType),
			geomParent->EXTENT_MAX_X, geomParent->EXTENT_MIN_X,
			geomParent->EXTENT_MAX_Y, geomParent->EXTENT_MIN_Y,
			geomParent->EXTENT_MAX_Z, geomParent->EXTENT_MIN_Z);
		#endif

		if (maxx > geomParent->EXTENT_MAX_X) geomParent->EXTENT_MAX_X = maxx;
		if (minx < geomParent->EXTENT_MIN_X) geomParent->EXTENT_MIN_X = minx;
		if (maxy > geomParent->EXTENT_MAX_Y) geomParent->EXTENT_MAX_Y = maxy;
		if (miny < geomParent->EXTENT_MIN_Y) geomParent->EXTENT_MIN_Y = miny;
		if (maxz > geomParent->EXTENT_MAX_Z) geomParent->EXTENT_MAX_Z = maxz;
		if (minz < geomParent->EXTENT_MIN_Z) geomParent->EXTENT_MIN_Z = minz;
		#ifdef FRUSTUMPRINT
		printf ("now, propextent, me %d my parent %d is %d (%s) ext %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f\n",
			me,i,geomParent, stringNodeType(geomParent->_nodeType),
			geomParent->EXTENT_MAX_X, geomParent->EXTENT_MIN_X,
			geomParent->EXTENT_MAX_Y, geomParent->EXTENT_MIN_Y,
			geomParent->EXTENT_MAX_Z, geomParent->EXTENT_MIN_Z);
		#endif
	}
}

#ifdef DISPLAYBOUNDINGBOX
void BoundingBox(struct X3D_Box * me) {
	int nt;

	nt = me->_nodeType;

	#ifdef FRUSTUMPRINT
	printf ("bbox for %s (%3.2f %3.2f)  (%3.2f %3.2f) (%3.2f %3.2f)\n",stringNodeType(nt),
		me->EXTENT_MIN_X, me->EXTENT_MAX_X,
		me->EXTENT_MIN_Y, me->EXTENT_MAX_Y,
		me->EXTENT_MIN_Z, me->EXTENT_MAX_Z);
	#endif

	/* show a bounding box around each grouping node */
	DISABLE_CULL_FACE
	LIGHTING_OFF
/*
	if (nt == NODE_Transform) 
		glColor3f(1.0, 0.0, 0.0);
	else if (nt == NODE_Group)
		glColor3f(0.0, 1.0, 0.0);
	else
		glColor3f (0.0, 0.0, 1.0);
*/

	/* color if bounding box not set properly */

	if (me->EXTENT_MAX_X <= -999.9) {
		glColor3f (1.0, 1.0, 0.0);
	} else {
		glColor3f(1.0, 0.0, 0.0);
	}
	

	/* top of box */
	glBegin(GL_LINE_STRIP);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MAX_Y, me->EXTENT_MAX_Z);
	glVertex3f(me->EXTENT_MAX_X, me->EXTENT_MAX_Y, me->EXTENT_MAX_Z);
	glVertex3f(me->EXTENT_MAX_X, me->EXTENT_MAX_Y, me->EXTENT_MIN_Z);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MAX_Y, me->EXTENT_MIN_Z);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MAX_Y, me->EXTENT_MAX_Z);
	glEnd();

	/* bottom of box */
	glBegin(GL_LINE_STRIP);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MIN_Y, me->EXTENT_MAX_Z);
	glVertex3f(me->EXTENT_MAX_X, me->EXTENT_MIN_Y, me->EXTENT_MAX_Z);
	glVertex3f(me->EXTENT_MAX_X, me->EXTENT_MIN_Y, me->EXTENT_MIN_Z);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MIN_Y, me->EXTENT_MIN_Z);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MIN_Y, me->EXTENT_MAX_Z);
	glEnd();

	/* vertical bars */
	glBegin(GL_LINE_STRIP);
	glVertex3f(me->EXTENT_MAX_X, me->EXTENT_MAX_Y, me->EXTENT_MAX_Z);
	glVertex3f(me->EXTENT_MAX_X, me->EXTENT_MIN_Y, me->EXTENT_MAX_Z);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MAX_Y, me->EXTENT_MAX_Z);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MIN_Y, me->EXTENT_MAX_Z);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(me->EXTENT_MAX_X, me->EXTENT_MAX_Y, me->EXTENT_MIN_Z);
	glVertex3f(me->EXTENT_MAX_X, me->EXTENT_MIN_Y, me->EXTENT_MIN_Z);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MAX_Y, me->EXTENT_MIN_Z);
	glVertex3f(me->EXTENT_MIN_X, me->EXTENT_MIN_Y, me->EXTENT_MIN_Z);
	glEnd();
	
	LIGHTING_ON
	ENABLE_CULL_FACE
}

#endif

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

	/* did we have a failure here ? */
	if (OccFailed) return;

	/* have we been through this yet? */
	if (OccInitialized == FALSE) {
        	/* printf ("aqDisplayThread, extensions %s\n",glGetString(GL_EXTENSIONS));  */
        	if (strstr((const char *)glGetString(GL_EXTENSIONS),"GL_ARB_occlusion_query") != 0) {
        	        /* printf ("have OcclusionQuery\n");  */

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

        	} else {
			/* we dont seem to have this extension here at runtime! */
			/* this happened, eg, on my Core4 AMD64 box with Mesa	*/
			OccFailed = TRUE;
		}

	}

	/* can we do OcclusionQueries? */
	if (!OccInitialized) return;

	/* did we find more shapes than before? */
        if (maxShapeFound > OccQuerySize) {
                /* printf ("have to regen queries\n"); */
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
	
	/* did we have some problem with Occlusion ? */
	if (OccFailed) return;

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
					xx = (struct X3D_Shape *) xx->geometry;
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
	#endif
}

int newOcclude() {
	int retval;
	#ifdef OCCLUSION
		retval = maxShapeFound;
		maxShapeFound ++;
	#else
		retval = 0;
	#endif	
	return retval;
}
