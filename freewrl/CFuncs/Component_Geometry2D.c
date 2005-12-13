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
void *createLines (float start, float end, float radius, int closed, int *size);

void render_Arc2D (struct VRML_Arc2D *this_) {
        if (this_->_ichange != this_->_change) {
                /*  have to regen the shape*/
                this_->_ichange = this_->_change;
		
		printf ("render_Arc2D, endAngle %f startAngle %f radius %f\n",
			this_->endAngle, this_->startAngle, this_->radius);

		FREE_IF_NZ (this_->__points);
		this_->__numPoints = 0;
		this_->__points = createLines (this_->startAngle,
			this_->endAngle, this_->radius, FALSE, &this_->__numPoints);
	}

	if (this_->__numPoints>0) {	
		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__points);
        	glDrawArrays (GL_LINE_STRIP, 0, this_->__numPoints);
		glEnableClientState (GL_NORMAL_ARRAY);
	}
}

void render_ArcClose2D (struct VRML_ArcClose2D *this_){}
void render_Circle2D (struct VRML_Circle2D *this_){}
void render_Disk2D (struct VRML_Disk2D *this_){}
void render_Polyline2D (struct VRML_Polyline2D *this_){}
void render_Polypoint2D (struct VRML_Polypoint2D *this_){}
void render_Rectangle2D (struct VRML_Rectangle2D *this_){}
void render_TriangleSet2D (struct VRML_TriangleSet2D *this_){}

void *createLines (float start, float end, float radius, int closed, int *size) {
	int i;
	int isCircle;
	int numPoints;
	float tmp;
	float *points;
	float *fp;

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
		printf ("createLines, this is a circle!\n");
		numPoints = SEGMENTS_PER_CIRCLE;
	} else {
		numPoints = ((float) SEGMENTS_PER_CIRCLE * (start-end)/(PI*2.0));
		if (numPoints>SEGMENTS_PER_CIRCLE) numPoints=SEGMENTS_PER_CIRCLE;
	}

	/* we always have to draw the line - we have a line strip, and we calculate
	   the beginning points; we have also to calculate the ending point. */
	numPoints++;

	printf ("numPoints %d start-end %f\n",numPoints, start-end);

	points = malloc (sizeof(float)*numPoints*3);
	fp = points;

	for (i=0; i<numPoints; i++) {
		*fp = radius* sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
		*fp = radius * cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
		*fp=0.0; /* z is always zero */
		fp++;
	}

	/* do we have to draw any pies, cords, etc, etc? */

		
	*size = numPoints;
	return (void *)points;
}
	
