/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "Structs.h"
#include "headers.h"
#include "LinearAlgebra.h"

extern int smooth_normals;


void fwnorprint (float *norm) {
		printf ("normals %f %f %f\n",norm[0],norm[1],norm[2]);
}

void normalize_ifs_face (float *point_normal,	
			 struct pt *facenormals,	
			 int *pointfaces,		
			int mypoint,
			int curpoly,
			float creaseAngle) {

	/* IndexedFaceSet (and possibly sometime, others)
	   normal generator 

	  Passed in:
		point_normal	- where to put the calculated normal
		facenormals	- normals of each face of a polygon
		pointfaces	- each point - which face(s) is it part of
		mypoint		- which point are we looking at
		curpoly		- which poly (face) we are working on
		creaseAngle	- creaseAngle of polygon
	*/
	int tmp_a;
	int tmp_b; 
	int facecount;
	float zz;
	struct pt temp;

	point_normal[0] = 0.0; point_normal[1] = 0.0; point_normal[2] = 0.0;

	// printf ("my normal is %f %f %f\n", facenormals[curpoly].x,
	// 	facenormals[curpoly].y,facenormals[curpoly].z);
	
	/* short cut for a point in only 1 face */
	if (pointfaces[mypoint*POINT_FACES] == 1) {
		point_normal[0]=facenormals[curpoly].x;
		point_normal[1]=facenormals[curpoly].y;
		point_normal[2]=facenormals[curpoly].z;
		return;
	}

	/* ok, calculate normal */
	facecount = 0;
	for (tmp_b=0; tmp_b<pointfaces[mypoint*POINT_FACES]; tmp_b++) {
		tmp_a = pointfaces[mypoint*POINT_FACES+tmp_b+1];
		// printf ("comparing myface %d to %d\n",curpoly,tmp_a);  
	
		if (curpoly == tmp_a) {
			zz = 0.0;
		} else {
			zz = calc_angle_between_two_vectors(facenormals[curpoly],facenormals[tmp_a] );
		}
		// printf ("angle between faces is %f, creaseAngle is %f\n",zz,creaseAngle); 

		
		if (zz <= creaseAngle) {
			// printf ("count this one in; adding %f %f %f\n",facenormals[tmp_a].x,facenormals[tmp_a].y,facenormals[tmp_a].z);
			point_normal[0] += facenormals[tmp_a].x;
			point_normal[1] += facenormals[tmp_a].y;
			point_normal[2] += facenormals[tmp_a].z;
		}
	}
	temp.x = point_normal[0]; temp.y=point_normal[1]; temp.z=point_normal[2];	
	normalize_vector(&temp); 
	point_normal[0]=temp.x; point_normal[1]=temp.y; point_normal[2]=temp.z;

	// printf ("normalized vector is %f %f %f\n",point_normal[0], point_normal[1], point_normal[2]);
}
	
void initialize_smooth_normals() {

	/* first complex face - are we running in fast or good mode... */
	if (smooth_normals == -1) {
		/* Needs to be initialized */
		glGetIntegerv (GL_SHADE_MODEL, &smooth_normals);
		smooth_normals = smooth_normals == GL_SMOOTH;
	}
}
