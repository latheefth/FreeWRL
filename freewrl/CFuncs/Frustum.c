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

#undef OCCLUSIONVERBOSE

/* Occlusion VisibilitySensor code */
GLuint *OccQueries = NULL;
void* *OccNodes = NULL;
int *OccInvisibleCount = NULL;
GLint *OccSamples = NULL;

int maxOccludersFound = 0;
int OccQuerySize=0;
int QueryCount = 0;
GLint queryCounterBits;
int OccInitialized = FALSE;
int OccFailed = FALSE;

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

	/* did we have a failure here ? */
	if (OccFailed) return;

	/* have we been through this yet? */
	if (OccInitialized == FALSE) {
        	/* printf ("aqDisplayThread, extensions %s\n",glGetString(GL_EXTENSIONS));  */
        	if (strstr((const char *)glGetString(GL_EXTENSIONS),"GL_ARB_occlusion_query") != 0) {
			#ifdef OCCLUSIONVERBOSE
        	        printf ("OcclusionStartofEventLoop: have OcclusionQuery\n"); 
			#endif


			/* we make the OccQuerySize larger than the maximum number of occluders,
			   so we don't have to realloc too much */
			OccQuerySize = maxOccludersFound + 1000;
			OccQueries = MALLOC (sizeof(int) * OccQuerySize);
			OccNodes = MALLOC (sizeof (void *) * OccQuerySize);
			OccInvisibleCount = MALLOC (sizeof (int) * OccQuerySize);
			OccSamples = MALLOC (sizeof (int) * OccQuerySize);
                	glGenQueries(OccQuerySize,OccQueries);
			OccInitialized = TRUE;
			for (i=0; i<OccQuerySize; i++) {
				OccNodes[i] = 0;
				OccSamples[i]=0;
				OccInvisibleCount[i]=0;
			}
			QueryCount = maxOccludersFound; /* for queries - we can do this number */
			#ifdef OCCLUSIONVERBOSE
			printf ("QueryCount now %d\n",QueryCount);
			#endif

        	} else {
			#ifdef OCCLUSIONVERBOSE
        	        printf ("OcclusionStartofEventLoop: DO NOT have OcclusionQuery\n"); 
			#endif

			/* we dont seem to have this extension here at runtime! */
			/* this happened, eg, on my Core4 AMD64 box with Mesa	*/
			OccFailed = TRUE;
			return;
		}

	}

	/* did we find more shapes than before? */
	if (maxOccludersFound > QueryCount) {
        	if (maxOccludersFound > OccQuerySize) {
        	        /* printf ("have to regen queries\n"); */
			QueryCount = 0;

			/* possibly previous had zero occluders, lets just not bother deleting for zero */
			if (OccQuerySize > 0) {
				glDeleteQueries (OccQuerySize, OccQueries);
				glFlush();
			}

			OccQuerySize = maxOccludersFound + 1000;
			OccQueries = REALLOC (OccQueries,sizeof (int) * OccQuerySize);
			OccNodes = REALLOC (OccNodes,sizeof (void *) * OccQuerySize);
			OccInvisibleCount = REALLOC (OccInvisibleCount,sizeof (int) * OccQuerySize);
			OccSamples = REALLOC (OccSamples,sizeof (int) * OccQuerySize);
        	        glGenQueries(OccQuerySize,OccQueries);
			for (i=0; i<OccQuerySize; i++) {
				OccNodes[i] = 0;
				OccSamples[i]=0;
			}
		}
		QueryCount = maxOccludersFound; /* for queries - we can do this number */
		#ifdef OCCLUSIONVERBOSE
		printf ("QueryCount here is %d\n",QueryCount);
		#endif

       }

	#ifdef OCCLUSIONVERBOSE
        glGetQueryiv(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &queryCounterBits);
        printf ("queryCounterBits %d\n",queryCounterBits);
        #endif
}


void OcclusionCulling ()  {
	int i;
	int maxcount;
	struct X3D_Shape *xx;
	
	/* did we have some problem with Occlusion ? */
	if (OccFailed) return;

	/* Step 1. go through list of assigned nodes, and REMOVE the VF_hasVisibleChildren flag. */
	zeroVisibilityFlag();
	 
	/* Step 2. go through the list of "OccludeCount" nodes, and determine if they are visible. 
	   If they are not, then, we have to, at some point, make them visible, so that we can test again. */
 
	for (i=0; i<=QueryCount; i++) {
	        glGetQueryObjectiv (OccQueries[i], GL_QUERY_RESULT, &OccSamples[i]);
		if (OccNodes[i] != 0) {
			xx = (struct X3D_Shape *) OccNodes[i];

			/* if this is a VisibilitySensor, record the samples */
			if (xx->_nodeType == NODE_VisibilitySensor) {
				((struct X3D_VisibilitySensor *)xx)->__Samples =  OccSamples[i]; 
				#ifdef OCCLUSIONVERBOSE
				printf ("OcclusionCulling, found VisibilitySensor at %d, fragments %d active %d\n",i,OccSamples[i],OccInvisibleCount[i]);
				#endif
			}


			/* is this is Shape? */
			else if (xx->_nodeType == NODE_Shape) {
				#ifdef OCCLUSIONVERBOSE
	        		printf ("OcclusionCulling, Shape found,  %d OccSamples %d OccInvisibleCount %d\n",i,OccSamples[i],OccInvisibleCount[i]);
				#endif

				/* is this node visible? If so, tell the parents! */
				if (OccSamples[i] > 0) {
					update_renderFlag (xx,VF_hasVisibleChildren);
					OccInvisibleCount[i] = OccSamples[i];
				} else {
					/* has this just gone invisible? */
					if (OccInvisibleCount[i] > 0) 
						OccInvisibleCount[i] = 0; 
				}
			}
		}
	}

	/* determine if we should try a node again */
	for (i=0; i<QueryCount; i++) {
		/* printf ("VisibleCount for %d is %d\n",i,OccInvisibleCount[i]);  */
		(OccInvisibleCount[i])--;
		/* once in a while, try to see if any of these are visible. Stagger
		   tries, so we don't oscillate. (note the last term, below */
		/* if (OccInvisibleCount[i] < -(32 + (i&0x0f))) { */
		if (OccInvisibleCount[i] < -(i&0x01f)) {
			xx = (struct X3D_Shape *) OccNodes[i];
			OccInvisibleCount[i] = 0;
			if (xx != 0) {
				update_renderFlag(xx,VF_hasVisibleChildren);
				/* printf ("OcclusionCulling, node %d is invisible, trying it again\n",i); */
			}
		}
	}
}

int newOcclude() {
	int retval;
	if (!OccFailed) {
		retval = maxOccludersFound;
		maxOccludersFound ++;
	} else {
		retval = 0;
	}
	return retval;
}

/* shut down the occlusion stuff */
void zeroOcclusion(void) {
	if (OccFailed) return;

	QueryCount = 0;
	glDeleteQueries (OccQuerySize, OccQueries);
	glFlush();
	
	OccQuerySize=0;
	maxOccludersFound = 0;
	FREE_IF_NZ(OccQueries);
	FREE_IF_NZ(OccNodes);
	FREE_IF_NZ(OccInvisibleCount);
	FREE_IF_NZ(OccSamples);
}
