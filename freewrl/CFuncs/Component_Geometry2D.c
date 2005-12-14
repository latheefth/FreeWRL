/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Geometry2D  Component

*********************************************************************/

#include "headers.h"

#define SEGMENTS_PER_CIRCLE 36
#define PIE 10
#define CHORD 20
#define NONE 30

void *createLines (float start, float end, float radius, int closed, int *size);

void render_Arc2D (struct VRML_Arc2D *node) {
        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;
		
		FREE_IF_NZ (node->__points);
		node->__numPoints = 0;
		node->__points = createLines (node->startAngle,
			node->endAngle, node->radius, NONE, &node->__numPoints);
	}

	if (node->__numPoints>0) {	
        	glPushAttrib(GL_ENABLE_BIT);
	        glDisable (GL_LIGHTING);
	        glDisable(GL_COLOR_MATERIAL);
	        glDisable(GL_CULL_FACE);
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->__points);
        	glDrawArrays (GL_LINE_STRIP, 0, node->__numPoints);
		glEnableClientState (GL_NORMAL_ARRAY);

		glPopAttrib();
	}
}

void render_ArcClose2D (struct VRML_ArcClose2D *node) {
	STRLEN xx;
	char *ct;

        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;
		
		FREE_IF_NZ (node->__points);
		node->__numPoints = 0;

		ct = SvPV(node->closureType,xx);

		if (strncmp(ct,"PIE",xx) == 0) {
			node->__points = createLines (node->startAngle,
				node->endAngle, node->radius, PIE, &node->__numPoints);
		} else if (strncmp(ct,"CHORD",xx) == 0) {
			node->__points = createLines (node->startAngle,
				node->endAngle, node->radius, CHORD, &node->__numPoints);
		} else {
			printf ("ArcClose2D, closureType %s invalid\n",node->closureType);
		}
	}


	if (node->__numPoints>0) {	
        	glPushAttrib(GL_ENABLE_BIT);
        	glDisable (GL_LIGHTING);
        	glDisable(GL_COLOR_MATERIAL);
        	glDisable(GL_CULL_FACE);
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->__points);
        	glDrawArrays (GL_LINE_STRIP, 0, node->__numPoints);
		glEnableClientState (GL_NORMAL_ARRAY);
		glPopAttrib();
	}
}

void render_Circle2D (struct VRML_Circle2D *node) {
        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;
		
		FREE_IF_NZ (node->__points);
		node->__numPoints = 0;
		node->__points = createLines (0.0, 0.0,
			node->radius, NONE, &node->__numPoints);
	}

	if (node->__numPoints>0) {	
        	glPushAttrib(GL_ENABLE_BIT);
	        glDisable (GL_LIGHTING);
	        glDisable(GL_COLOR_MATERIAL);
	        glDisable(GL_CULL_FACE);
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->__points);
        	glDrawArrays (GL_LINE_STRIP, 0, node->__numPoints);
		glEnableClientState (GL_NORMAL_ARRAY);

		glPopAttrib();
	}
}

void render_Polyline2D (struct VRML_Polyline2D *node){
	if (node->lineSegments.n>0) {
        	glPushAttrib(GL_ENABLE_BIT);
	        glDisable (GL_LIGHTING);
	        glDisable(GL_COLOR_MATERIAL);
	        glDisable(GL_CULL_FACE);
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->lineSegments.p);
        	glDrawArrays (GL_LINE_STRIP, 0, node->lineSegments.n);
		glEnableClientState (GL_NORMAL_ARRAY);

		glPopAttrib();
	}
}

void render_Polypoint2D (struct VRML_Polypoint2D *node){
	if (node->point.n>0) {
        	glPushAttrib(GL_ENABLE_BIT);
	        glDisable (GL_LIGHTING);
	        glDisable(GL_COLOR_MATERIAL);
	        glDisable(GL_CULL_FACE);
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->point.p);
        	glDrawArrays (GL_POINTS, 0, node->point.n);
		glEnableClientState (GL_NORMAL_ARRAY);

		glPopAttrib();
	}
}

void render_Disk2D (struct VRML_Disk2D *node){}
void render_Rectangle2D (struct VRML_Rectangle2D *node){}
void render_TriangleSet2D (struct VRML_TriangleSet2D *node){}

void *createLines (float start, float end, float radius, int closed, int *size) {
	int i;
	int isCircle;
	int numPoints;
	GLfloat tmp;
	GLfloat *points;
	GLfloat *fp;
	int arcpoints;

	*size = 0;

	/* is this a circle? */
	isCircle =  APPROX(start,end);

	/* bounds check, and sort values */
	if ((start < PI*2.0) || (start > PI*2.0)) start = 0;
	if ((end < PI*2.0) || (end > PI*2.0)) end = PI/2;
	if (radius<0.0) radius = 1.0;

	if (end > start) {
		tmp = start;
		start = end;
		end = tmp;
	}
		

	if (isCircle) {
		numPoints = SEGMENTS_PER_CIRCLE;
		closed = NONE; /* this is a circle, CHORD, PIE dont mean anything now */
	} else {
		numPoints = ((float) SEGMENTS_PER_CIRCLE * (start-end)/(PI*2.0));
		if (numPoints>SEGMENTS_PER_CIRCLE) numPoints=SEGMENTS_PER_CIRCLE;
	}

	/* we always have to draw the line - we have a line strip, and we calculate
	   the beginning points; we have also to calculate the ending point. */
	numPoints++;
	arcpoints = numPoints;

	/* closure type */
	if (closed == CHORD) numPoints++;
	if (closed == PIE) numPoints+=2;

	points = malloc (sizeof(float)*numPoints*2);
	fp = points;

	for (i=0; i<arcpoints; i++) {
		*fp = -radius * sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
		*fp = radius * cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
	}

	/* do we have to draw any pies, cords, etc, etc? */
	if (closed == CHORD) {
		/* loop back to origin */
		*fp = -radius * sinf(0.0/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
		*fp = radius * cosf(0.0/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
	} else if (closed == PIE) {
		/* go to origin */
		*fp = 0.0; fp++; *fp=0.0; fp++; 
		*fp = -radius * sinf(0.0/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
		*fp = radius * cosf(0.0/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
	}

		
	*size = numPoints;
	return (void *)points;
}
