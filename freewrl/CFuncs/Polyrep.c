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
#include "LinearAlgebra.h"


/* transformed ray */
extern struct pt t_r1;
extern struct pt t_r2;
extern struct pt t_r3;

/* reset colors to defaults, if we have to */
GLfloat diffuseColor[] = {0.8, 0.8, 0.8,1.0};
GLfloat ambientIntensity[] = {0.16, 0.16, 0.16, 1.0}; /*VRML diff*amb defaults */
GLfloat specularColor[] = {0.0, 0.0, 0.0, 1.0};
GLfloat emissiveColor[] = {0.0, 0.0, 0.0, 1.0};

GLfloat last_color[] = {0.0,0.0,0.0};
GLfloat last_normal[] = {0.0,0.0,0.0};

void add_to_face ( int point, int face, int *pointfaces);

/* GENERIC POLYREP SMOOTH NORMAL DATABASE GENERATION 		*/
/* 								*/


/* How many faces are in this IndexedFaceSet?			*/

int count_IFS_faces(int cin, struct VRML_IndexedFaceSet *this_IFS) {	
	/* lets see how many faces we have */
	int pointctr=0;
	int max_points_per_face = 0;
	int min_points_per_face = 99999;
	int i;
	int faces = 0;

	for(i=0; i<cin; i++) {

		if(((this_IFS->coordIndex.p[i]) == -1) || (i==cin-1)) {
			if((this_IFS->coordIndex.p[i]) != -1) {
				pointctr++;
			}

			faces++;
			if (pointctr > max_points_per_face) 
				max_points_per_face = pointctr;
			if (pointctr < min_points_per_face) 
				min_points_per_face = pointctr;
			pointctr = 0;
		} else pointctr++;
	}

		
	/*
	printf ("this structure has %d faces\n",faces);
	printf ("	max points per face %d\n",max_points_per_face);
	printf ("	min points per face %d\n\n",min_points_per_face);
	*/
	
	if (faces < 1) {
		printf("an IndexedFaceSet with no faces found\n");
		return (0);
	}
	return faces;
}


/* Generate the normals for each face of an IndexedFaceSet	*/
/* create two datastructures:					*/
/* 	- face normals; given a face, tell me the normal	*/
/*	- point-face;   for each point, tell me the face(s)	*/

void IFS_face_normals (
	struct pt *facenormals,
	int *faceok,
	int *pointfaces,
	int faces, 
	int npoints,
	int cin,
	struct SFColor *points,
	struct VRML_IndexedFaceSet *this_IFS,
	int ccw) {

	int tmp_a, tmp_c;
	int i;
	int facectr;
	int pt_1, pt_2, pt_3;
	float AC, BC;
	struct SFColor *c1,*c2,*c3;
	float a[3]; float b[3];

	tmp_a = 0;


	// Assume each face is ok for now
	for(i=0; i<faces; i++) {
		faceok[i] = TRUE;
	}

	// calculate normals for each face
	for(i=0; i<faces; i++) {
		if (tmp_a >= cin-2) {
			printf ("last face in IFS has not enough vertexes\n");
			faceok[i] = FALSE;
		} else {
			/* does this face have at least 3 vertexes? */
			if ((this_IFS->coordIndex.p[tmp_a] == -1) ||
			    (this_IFS->coordIndex.p[tmp_a+1] == -1) ||
			    (this_IFS->coordIndex.p[tmp_a+2] == -1)) {
				printf ("have a face with two or less vertexes\n");
				faceok[i] = FALSE;

				if (this_IFS->coordIndex.p[tmp_a] != -1) tmp_a++;
			} else {
	
				
				/* check for degenerate triangles -- if found, try to select another point */
				tmp_c = FALSE;
				pt_1 = tmp_a; 
				if (ccw) {
					//printf ("IFS face normals CCW\n");
					pt_2 = tmp_a+1; pt_3 = tmp_a+2;
				} else {
					//printf ("IFS face normals *NOT* CCW\n");
					pt_3 = tmp_a+1; pt_2 = tmp_a+2;
				}
		
				do {	
					/* first three coords give us the normal */
					c1 = &(points[this_IFS->coordIndex.p[pt_1]]);
					c2 = &(points[this_IFS->coordIndex.p[pt_2]]); 
					c3 = &(points[this_IFS->coordIndex.p[pt_3]]);
		
					a[0] = c2->c[0] - c1->c[0];
					a[1] = c2->c[1] - c1->c[1];
					a[2] = c2->c[2] - c1->c[2];
					b[0] = c3->c[0] - c1->c[0];
					b[1] = c3->c[1] - c1->c[1];
					b[2] = c3->c[2] - c1->c[2];
		
					facenormals[i].x = a[1]*b[2] - b[1]*a[2];
					facenormals[i].y = -(a[0]*b[2] - b[0]*a[2]);
					facenormals[i].z = a[0]*b[1] - b[0]*a[1];
		
					//printf ("vector length is %f\n",calc_vector_length (facenormals[i])); 
		
					if (fabs(calc_vector_length (facenormals[i])) < 0.0001) {
						AC=(c1->c[0]-c3->c[0])*(c1->c[1]-c3->c[1])*(c1->c[2]-c3->c[2]);
						BC=(c2->c[0]-c3->c[0])*(c2->c[1]-c3->c[1])*(c2->c[2]-c3->c[2]);
						/* printf ("AC %f ",AC);
						printf ("BC %f \n",BC); */
		
						/* we have 3 points, a, b, c */
						/* we also have 3 vectors, AB, AC, BC */
						/* find out which one looks the closest one to skip out */
						/* either we move both 2nd and 3rd points, or just the 3rd */
						if (fabs(AC) < fabs(BC)) { pt_2++; }
						pt_3++;
		
						/* skip forward to the next couple of points - if possible */
						/* printf ("looking at %d, cin is %d\n",tmp_a, cin); */
						tmp_a ++;
						if ((tmp_a >= cin-2) || ((this_IFS->coordIndex.p[tmp_a+2]) == -1)) {
							/* printf ("possible degenerate triangle, but no more points\n"); */
							/* put values in there so normals will work out */
							if (fabs(calc_vector_length (facenormals[i])) < 0.0000001) {
								/* we would have a divide by zero in normalize_vector, so... */
								facenormals[i].z = 1.0;
		
								// Mark this face to be bad
								faceok[i] = FALSE;
							}
							tmp_c = TRUE;  tmp_a +=2;
						}
					} else {
						tmp_c = TRUE;
						tmp_a +=3;
					}
		
				} while (!tmp_c);
		
				normalize_vector(&facenormals[i]);
				
				/*printf ("vertices \t%f %f %f\n\t\t%f %f %f\n\t\t%f %f %f\n",
					c1->c[0],c1->c[1],c1->c[2],
					c2->c[0],c2->c[1],c2->c[2],
					c3->c[0],c3->c[1],c3->c[2]);
				printf ("normal %f %f %f\n\n",facenormals[i].x,
					facenormals[i].y,facenormals[i].z);
				*/
				
			}
		}

		/* skip forward to next ifs - we have the normal */
		if (i<faces-1) {
			while (((this_IFS->coordIndex.p[tmp_a-1]) != -1) &&
				(tmp_a < cin-2)) {
				tmp_a++;
			}
		}
	}


	/* now, go through each face, and make a point-face list 
	   so that I can give it a point later, and I will know which face(s) 
	   it belong to that point */

	/* printf ("\nnow generating point-face list\n");  */
	for (i=0; i<npoints; i++) { pointfaces[i*POINT_FACES]=0; }
	facectr=0; 
	for(i=0; i<cin; i++) {
		tmp_a=this_IFS->coordIndex.p[i];
		/* printf ("pointfaces, coord %d coordIndex %d face %d\n",i,tmp_a,facectr); */
		if (tmp_a == -1) {
			facectr++;
		} else {
			if (faceok[facectr]) {
				tmp_a*=POINT_FACES;
				add_to_face (tmp_a,facectr,pointfaces);
			} else {
			//	printf ("skipping add_to_face for invalid face %d\n",facectr);
			}
		}
	}

	/* printf ("\ncheck \n");
	for (i=0; i<npoints; i++) {
		tmp_a = i*POINT_FACES;
		printf ("point %d is in %d faces, these are:\n ", i, pointfaces[tmp_a]);
		for (tmp_b=0; tmp_b<pointfaces[tmp_a]; tmp_b++) {
			printf ("%d ",pointfaces[tmp_a+tmp_b+1]);
		}
		printf ("\n");
	}
	*/
}



/* Tesselated faces MAY have the wrong normal calculated. re-calculate after tesselation	*/

void Extru_check_normal (
	struct pt *facenormals,
	int this_face, 
	float direction,
	struct VRML_PolyRep  *rep_, 
	int ccw) {

	/* only use this after tesselator as we get coord indexes from global var */
	struct SFColor *c1,*c2,*c3;
	float a[3]; float b[3];
	int zz1, zz2;

	if (ccw) {
		zz1 = 1;
		zz2 = 2;
	} else {
		zz1 = 2;
		zz2 = 1;
	}

	//printf ("Extru_check_normal, coords %d %d %d\n",global_IFS_Coords[0],
	//	global_IFS_Coords[1],global_IFS_Coords[2]);

	/* first three coords give us the normal */
 	c1 = (struct SFColor *) &rep_->coord[3*global_IFS_Coords[0]];
 	c2 = (struct SFColor *) &rep_->coord[3*global_IFS_Coords[zz1]];
 	c3 = (struct SFColor *) &rep_->coord[3*global_IFS_Coords[zz2]];

	//printf ("Extru_check_normal, coords %d %d %d\n",global_IFS_Coords[0],
	//	global_IFS_Coords[1],global_IFS_Coords[2]);
	//printf ("Extru_check_normal vertices \t%f %f %f\n\t\t%f %f %f\n\t\t%f %f %f\n",
	//	c1->c[0],c1->c[1],c1->c[2],
	//	c2->c[0],c2->c[1],c2->c[2],
	//	c3->c[0],c3->c[1],c3->c[2]);

	a[0] = c2->c[0] - c1->c[0];
	a[1] = c2->c[1] - c1->c[1];
	a[2] = c2->c[2] - c1->c[2];
	b[0] = c3->c[0] - c1->c[0];
	b[1] = c3->c[1] - c1->c[1];
	b[2] = c3->c[2] - c1->c[2];

	facenormals[this_face].x = a[1]*b[2] - b[1]*a[2] * direction;
	facenormals[this_face].y = -(a[0]*b[2] - b[0]*a[2]) * direction;
	facenormals[this_face].z = a[0]*b[1] - b[0]*a[1] * direction;

	if (fabs(calc_vector_length (facenormals[this_face])) < 0.0001) {
		printf ("INTERNAL ERROR: tesselator should not give degenerate triangles back\n");
	}

	normalize_vector(&facenormals[this_face]);
	//printf ("facenormal for %d is %f %f %f\n",this_face, facenormals[this_face].x,
	//		facenormals[this_face].y, facenormals[this_face].z);
}

/* Tesselated faces MAY have the wrong normal calculated. re-calculate after tesselation	*/


void IFS_check_normal (
	struct pt *facenormals,
	int this_face, 
	struct SFColor *points, int base,
	struct VRML_IndexedFaceSet *this_IFS, int ccw) {

	struct SFColor *c1,*c2,*c3;
	float a[3]; float b[3];

		
	//printf ("IFS_check_normal, base %d points %d %d %d\n",base, 
	//	global_IFS_Coords[0],global_IFS_Coords[1],global_IFS_Coords[2]);
	//printf ("normal was %f %f %f\n\n",facenormals[this_face].x,
	//	facenormals[this_face].y,facenormals[this_face].z);
		
	
	/* first three coords give us the normal */
	c1 = &(points[this_IFS->coordIndex.p[base+global_IFS_Coords[0]]]);
	if (ccw) {
		c2 = &(points[this_IFS->coordIndex.p[base+global_IFS_Coords[1]]]); 
		c3 = &(points[this_IFS->coordIndex.p[base+global_IFS_Coords[2]]]);
	} else {
		c3 = &(points[this_IFS->coordIndex.p[base+global_IFS_Coords[1]]]); 
		c2 = &(points[this_IFS->coordIndex.p[base+global_IFS_Coords[2]]]);
	}

	a[0] = c2->c[0] - c1->c[0];
	a[1] = c2->c[1] - c1->c[1];
	a[2] = c2->c[2] - c1->c[2];
	b[0] = c3->c[0] - c1->c[0];
	b[1] = c3->c[1] - c1->c[1];
	b[2] = c3->c[2] - c1->c[2];

	facenormals[this_face].x = a[1]*b[2] - b[1]*a[2];
	facenormals[this_face].y = -(a[0]*b[2] - b[0]*a[2]);
	facenormals[this_face].z = a[0]*b[1] - b[0]*a[1];

	//printf ("vector length is %f\n",calc_vector_length (facenormals[this_face])); 

	if (fabs(calc_vector_length (facenormals[this_face])) < 0.0001) {
		//printf ("warning: Tesselated surface has invalid normal - if this is an IndexedFaceSet, check coordinates of ALL faces\n");
	} else {

		normalize_vector(&facenormals[this_face]);
	
		
		//printf ("vertices \t%f %f %f\n\t\t%f %f %f\n\t\t%f %f %f\n",
		//	c1->c[0],c1->c[1],c1->c[2],
		//	c2->c[0],c2->c[1],c2->c[2],
		//	c3->c[0],c3->c[1],c3->c[2]);
		//printf ("normal %f %f %f\n\n",facenormals[this_face].x,
		//	facenormals[this_face].y,facenormals[this_face].z);
	}
	
}


void add_to_face (
	int point,
	int face,
	int *pointfaces) {

	int count;
	if (pointfaces[point] < (POINT_FACES-1)) {
		/* room to add, but is it already there? */
		for (count = 1; count <= pointfaces[point]; count++) {
			if (pointfaces[point+count] == face) return;
		}
		/* ok, we have an empty slot, and face not already added */
		pointfaces[point]++;
		pointfaces[point+ pointfaces[point]] = face;
	}
}
	
/********************************************************************
 *
 * ElevationGrid Triangle
 *
 */
void Elev_Tri (
	int vertex_ind,
	int this_face,
	int A,
	int D,
	int E,
	int NONORMALS,
	struct VRML_PolyRep *this_Elev,
	struct pt *facenormals,
	int *pointfaces,
	int ccw) {

	struct SFColor *c1,*c2,*c3;
	float a[3]; float b[3];
	int tmp;

	//printf ("Triangle %d %d %d\n",A,D,E);

	/* generate normals in a clockwise manner, reverse the triangle */
	if (!(ccw)) {
		tmp = D;
		D = E;
		E = tmp;
	}
		

	this_Elev->cindex[vertex_ind] = A;
	this_Elev->cindex[vertex_ind+1] = D;
	this_Elev->cindex[vertex_ind+2] = E;

	//printf ("Elev_Tri, vertices for vertex_ind %d are:",vertex_ind);
         //       c1 = (struct SFColor *) &this_Elev->coord[3*A];
          //      c2 = (struct SFColor *) &this_Elev->coord[3*D];
         //       c3 = (struct SFColor *) &this_Elev->coord[3*E];

	//	printf ("\n%f %f %f\n%f %f %f\n%f %f %f\n\n",
	//	c1->c[0], c1->c[1],c1->c[2],c2->c[0],c2->c[1],c2->c[2],
	//	c3->c[0],c3->c[1],c3->c[2]);
	

	if (NONORMALS) {
		/* calculate normal for this triangle */
                c1 = (struct SFColor *) &this_Elev->coord[3*A];
                c2 = (struct SFColor *) &this_Elev->coord[3*D];
                c3 = (struct SFColor *) &this_Elev->coord[3*E];

		//printf ("calc norms \n%f %f %f\n%f %f %f\n%f %f %f\n",
		//c1->c[0], c1->c[1],c1->c[2],c2->c[0],c2->c[1],c2->c[2],
		//c3->c[0],c3->c[1],c3->c[2]);

		a[0] = c2->c[0] - c1->c[0];
		a[1] = c2->c[1] - c1->c[1];
		a[2] = c2->c[2] - c1->c[2];
		b[0] = c3->c[0] - c1->c[0];
		b[1] = c3->c[1] - c1->c[1];
		b[2] = c3->c[2] - c1->c[2];

		facenormals[this_face].x = a[1]*b[2] - b[1]*a[2];
		facenormals[this_face].y = -(a[0]*b[2] - b[0]*a[2]);
		facenormals[this_face].z = a[0]*b[1] - b[0]*a[1];

		//printf ("facenormals index %d is %f %f %f\n",this_face, facenormals[this_face].x,
		//		facenormals[this_face].y, facenormals[this_face].z);

		/* add this face to the faces for this point */
		add_to_face (A*POINT_FACES,this_face,pointfaces);
		add_to_face (D*POINT_FACES,this_face,pointfaces);
		add_to_face (E*POINT_FACES,this_face,pointfaces);
	}
}



/***********************************************************************8
 *
 * Extrusion Texture Mapping 	
 *
 ***********************************************************************/

void Extru_tex(
	int vertex_ind,
	int tci_ct,
	int A,
	int B,
	int C,
	struct VRML_PolyRep *this_Elev,
	int ccw,
	int tcindexsize) {

	struct SFColor *c1,*c2,*c3;
	int j;

	/* bounds check */
	if (vertex_ind+2 >= tcindexsize) {
		printf ("INTERNAL ERROR: Extru_tex, bounds check %d >= %d\n",vertex_ind+2,tcindexsize);
	}
	
	/* generate textures in a clockwise manner, reverse the triangle */
	if (!(ccw)) { j = B; B = C; C = j; }

	/* ok, we have to do textures; lets do the tcindexes and record min/max */
	this_Elev->tcindex[vertex_ind] = tci_ct+A;
	this_Elev->tcindex[vertex_ind+1] =tci_ct+B;
	this_Elev->tcindex[vertex_ind+2] =tci_ct+C;

	/*	
	c1 = (struct SFColor *) &this_Elev->coord[3*A];
	c2 = (struct SFColor *) &this_Elev->coord[3*C];
	c3 = (struct SFColor *) &this_Elev->coord[3*B];

	printf ("Extru_tex, vertices are %f %f %f\n%f %f %f\n%f %f %f\n\n",
		c1->c[0], c1->c[1],c1->c[2],c2->c[0],c2->c[1],c2->c[2],
		c3->c[0],c3->c[1],c3->c[2]);
	printf ("for points %d %d %d\n",A,C,B);
	printf ("Extru_tex, vert_ind %d, tcindexes %d %d %d\n",vertex_ind,
			this_Elev->tcindex[vertex_ind],this_Elev->tcindex[vertex_ind+1],
			this_Elev->tcindex[vertex_ind+2]);
	*/
	
}


/*********************************************************************
 *
 * S,T mappings for Extrusions on begin and end caps.
 *
 **********************************************************************/

void Extru_ST_map(
	int triind_start,
	int start,
	int end,
	float *Vals,
	int nsec,
	struct VRML_PolyRep *this_Extru,
	int tcoordsize) {

	int x;
	GLfloat minS = 9999.9; 
	GLfloat maxS = -9999.9;
	GLfloat minT = 9999.9;
	GLfloat maxT = -9999.9;

	GLfloat Srange = 0.0;
	GLfloat Trange = 0.0;

	int Point_Zero;	/* the point that all cap tris start at. see comment below */

	/* find the base and range of S, T */
	for (x=0; x<nsec; x++) {
		 //printf ("for textures, coord vals %f %f for sec %d\n",
		 //Vals[x*2+0], Vals[x*2+1],x);
		if (Vals[x*2+0] < minS) minS = Vals[x*2+0];
		if (Vals[x*2+0] > maxS) maxS = Vals[x*2+0];
		if (Vals[x*2+1] < minT) minT = Vals[x*2+1];
		if (Vals[x*2+1] > maxT) maxT = Vals[x*2+1];
	}
	Srange = maxS -minS;
	Trange = maxT - minT;

	/* I hate divide by zeroes. :-) */
	if (Srange == 0.0) Srange = 0.001;
	if (Trange == 0.0) Trange = 0.001;

	//printf ("minS %f Srange %f minT %f Trange %f\n",minS,Srange,minT,Trange);

	/* Ok, we know the min vals of S and T; and the ranges. The way that end cap
	 * triangles are drawn is that we have one common point, the first point in
	 * each triangle. Use this as a base into the Vals index, to generate a S,T
	 * tex coord mapping for the [0,1] range
	 */

	for(x=start; x<end; x++) {
		int tci, ci;
		
		/*
		printf ("Extru_ST_Map: triangle has tex vertices:%d %d %d ",
			this_Extru->tcindex[triind_start*3],
			this_Extru->tcindex[triind_start*3+1] ,
			this_Extru->tcindex[triind_start*3+2]);
		printf ("Extru_ST_Map: coord vertices:%d %d %d\n",
			this_Extru->cindex[triind_start*3],
			this_Extru->cindex[triind_start*3+1] ,
			this_Extru->cindex[triind_start*3+2]);
		*/


		/* for first vertex */
		tci = this_Extru->tcindex[triind_start*3];
		ci = this_Extru->cindex[triind_start*3];
		Point_Zero = tci;

		if ((tci*3+2) >= tcoordsize) { 
			printf ("INTERNAL ERROR: Extru_ST_map(1), index %d greater than %d \n",(tci*3+2),tcoordsize);
			return;
		}

		/* S value */
		this_Extru->tcoord[tci*3+0] = (Vals[(tci-Point_Zero)*2+0] - minS) / Srange ;

		/* not used by render_polyrep */
		this_Extru->tcoord[tci*3+1] = 0;

		/* T value */
		this_Extru->tcoord[tci*3+2] = (Vals[(tci-Point_Zero)*2+1] - minT) / Trange;


		/* for second vertex */
		tci = this_Extru->tcindex[triind_start*3+1];
		ci = this_Extru->cindex[triind_start*3+1];

		if ((tci*3+2) >= tcoordsize) { 
			printf ("INTERNAL ERROR: Extru_ST_map(2), index %d greater than %d \n",(tci*3+2),tcoordsize);
			return;
		}

		/* S value */
		this_Extru->tcoord[tci*3+0] = (Vals[(tci-Point_Zero)*2+0] - minS) / Srange ;

		/* not used by render_polyrep */
		this_Extru->tcoord[tci*3+1] = 0;

		/* T value */
		this_Extru->tcoord[tci*3+2] = (Vals[(tci-Point_Zero)*2+1] - minT) / Trange;


		/* for third vertex */
		tci = this_Extru->tcindex[triind_start*3+2];
		ci = this_Extru->cindex[triind_start*3+2];

		if ((tci*3+2) >= tcoordsize) { 
			printf ("INTERNAL ERROR: Extru_ST_map(3), index %d greater than %d \n",(tci*3+2),tcoordsize);
			return;
		}

		/* S value */
		this_Extru->tcoord[tci*3+0] = (Vals[(tci-Point_Zero)*2+0] - minS) / Srange ;

		/* not used by render_polyrep */
		this_Extru->tcoord[tci*3+1] = 0;

		/* T value */
		this_Extru->tcoord[tci*3+2] = (Vals[(tci-Point_Zero)*2+1] - minT) / Trange;

		triind_start++;
	}
}



/* verify and prune colour/normals */

void do_color_normal_reset() {
	/* set color and normal to an invalid value, so next one is sure to do */
	last_color[0] = -99.0; 
	last_normal[0] = -99.0;
}

void do_glColor3fv(GLfloat *param) {
	int i,diff;

	/* parameter checks */
	for (i=0; i<3; i++) { 
		if ((param[i] < 0.0) || (param[i] >1.0)) {
			return; /* bounds check error found, break out */
		}
	}

	/* last values with new */
	diff = FALSE;
	for (i=0; i<3; i++) {
		if (fabs(last_color[i]-param[i]) > 0.001) {
			diff = TRUE;
			break;
		}
	}


/* 	printf ("last color %f %f %f this %f %f %f\n",last_color[0],
		last_color[1],last_color[2],param[0],param[1],param[2]); */

	if (diff) {
		for (i=0;i<3;i++) { last_color[i] = param[i];}
		glColor3fv(param);
	}
}


void do_glNormal3fv(GLfloat *param) {
	int i,diff;

	/* parameter checks */
	for (i=0; i<3; i++) { 
		if ((param[i] < -1.0) || (param[i] >1.0)) {
			return; /* bounds check error found, break out */
		}
	}

	/* last values with new */
	diff = FALSE;
	for (i=0; i<3; i++) {
		if (fabs(last_normal[i]-param[i]) > 0.001) {
			diff = TRUE;
			break;
		}
	}


	/* printf ("last normal %f %f %f this %f %f %f\n",last_normal[0],
		last_normal[1],last_normal[2],param[0],param[1],param[2]); */

	if (diff) {
		for (i=0;i<3;i++) { last_normal[i] = param[i];}
		glNormal3fv(param);
	}
}





/*********************************************************************
 *
 * render_polyrep : render one of the internal polygonal representations
 * for some nodes
 */
 




void render_polyrep(void *node, 
	int npoints, struct SFColor *points,
	int ncolors, struct SFColor *colors,
	int nnormals, struct SFColor *normals,
	int ntexcoords, struct SFVec2f *texcoords)
{
	struct VRML_Virt *v;
	struct VRML_Box *p;
	struct VRML_PolyRep *r;
	int prevcolor = -1;
	int i;
	int hasc;


	/* temporary place for X,Y,Z */
	GLfloat XYZ[] = {0.0, 0.0, 0.0};


	/* texture generation points... */
	int j;
	GLfloat minVals[] = {99999.9, 99999.9, 99999.9};
	GLfloat maxVals[] = {-99999.9, -999999.9, -99999.0};
	GLfloat Ssize = 0.0;
	GLfloat Tsize = 0.0;
	GLfloat Xsize = 0.0;
	GLfloat Ysize = 0.0;
	GLfloat Zsize = 0.0;
	int Sindex = 0;
	int Tindex = 0;

	int polyrep_verbose = 0;

	v = *(struct VRML_Virt **)node;
	p = node;
	r = p->_intern;

	if (r->ntri==0) {
		if (polyrep_verbose) printf ("Render polyrep, no triangles\n");
		return;
	}
		
	if (polyrep_verbose) {
		printf("Render polyrep %d '%s' (%d %d): %d\n",node,v->name, 
			p->_change, r->_change, r->ntri);
		printf ("\tnpoints %d ncolors %d nnormals %d\n",
			npoints,ncolors,nnormals);
		printf("\tntexcoords = %d    texcoords = 0x%lx\n",
			ntexcoords, texcoords);
		printf ("\ttcindex %d\n",r->tcindex);
	}
	
		

	/* do we need to generate default texture mapping? */
	if (HAVETODOTEXTURES && (ntexcoords == 0) && (!r->tcoord)) {
		for(i=0; i<r->ntri*3; i++) {
		  int ind = r->cindex[i];
		  for (j=0; j<3; j++) {
		      if(points) {
			    if (minVals[j] > points[ind].c[j]) minVals[j] = points[ind].c[j];
			    if (maxVals[j] < points[ind].c[j]) maxVals[j] = points[ind].c[j];
		      } else if(r->coord) {	
			    if (minVals[j] >  r->coord[3*ind+j]) minVals[j] =  r->coord[3*ind+j];
			    if (maxVals[j] <  r->coord[3*ind+j]) maxVals[j] =  r->coord[3*ind+j];
		      }
		  } 
		}

		/* find the S,T mapping. */
		Xsize = maxVals[0]-minVals[0]; 
		Ysize = maxVals[1]-minVals[1];
		Zsize = maxVals[2]-minVals[2];

		if ((Xsize >= Ysize) && (Xsize >= Zsize)) {
			/* X size largest */
			Ssize = Xsize;
			Sindex = 0;
			if (Ysize >= Zsize) {
				Tsize = Ysize;
				Tindex = 1;
			} else {
				Tsize = Zsize;
				Tindex = 2;
			}
		} else if ((Ysize >= Xsize) && (Ysize >= Zsize)) {
			/* Y size largest */
			Ssize = Ysize;
			Sindex = 1;
			if (Xsize >= Zsize) {
				Tsize = Xsize;
				Tindex = 0;
			} else {
				Tsize = Zsize;
				Tindex = 2;
			}
		} else {
			/* Z is the largest */
			Ssize = Zsize;
			Sindex = 2;
			if (Xsize >= Ysize) {
				Tsize = Xsize;
				Tindex = 0;
			} else {
				Tsize = Ysize;
				Tindex = 1;
			}
		}
	}


	/* Do we have any colours? Are textures, if present, not RGB? */
	hasc = ((ncolors || r->color) && (last_texture_depth<=1));
	if(hasc) { 
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseColor);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientIntensity);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissiveColor);
		glEnable(GL_COLOR_MATERIAL);
	}


	/* reset the do normal/color verification and pruning */
	do_color_normal_reset();

	// clockwise or not?
	if (r->ccw) { 
		glFrontFace(GL_CCW);
	} else {
		glFrontFace(GL_CW);
	}

	glBegin(GL_TRIANGLES);
	  for(i=0; i<r->ntri*3; i++) {
		int nori = i;
		int coli = i;
		int tci = i;
		int ind = r->cindex[i];

		if (polyrep_verbose) printf ("rp, i, ntri*3 %d %d\n",i,r->ntri*3); 

		/* get normals and colors, if any	*/
		if(r->norindex) { nori = r->norindex[i];}
		else nori = ind;

		if(r->colindex) {
			coli = r->colindex[i];
		}
		else coli = ind;

		/* get texture coordinates, if any	*/
		if (HAVETODOTEXTURES && r->tcindex) {
			tci = r->tcindex[i]; 
			if (polyrep_verbose) printf ("have textures, and tcindex i %d tci %d\n",i,tci);
		}

		/* get the normals, if there are any	*/
		if(nnormals) {
			if(nori >= nnormals) {
				/* this should be caught before here JAS */
				warn("Too large normal index %d nnormals %d-- help??",nori, nnormals);
			}
			if (polyrep_verbose) {
				printf ("nnormals at %d , nori %d ",&normals[nori].c,nori);
				fwnorprint (normals[nori].c);
			}

			do_glNormal3fv(normals[nori].c);
		} else if(r->normal) {
			if (polyrep_verbose) {
				printf ("r->normal nori %d ",nori);
				fwnorprint(r->normal+3*nori);
			}
			
			do_glNormal3fv(r->normal+3*nori);
		}

		if(hasc && prevcolor != coli) {
			if(ncolors) { 
				/* ColorMaterial -> these set Material too */
				if (polyrep_verbose) {
					printf ("coloUr"); 
					fwnorprint(colors[coli].c); 
					printf ("\n");
				}
				do_glColor3fv(colors[coli].c);
			} else if(r->color) {
				if (polyrep_verbose) {
					printf ("coloUr"); 
					fwnorprint(r->color+3*coli); 
					printf ("\n"); 
				}
				do_glColor3fv(r->color+3*coli);
			} 
		}
		prevcolor = coli;


		/* Coordinate points	*/
		if(points) {
			XYZ[0]= points[ind].c[0]; XYZ[1]= points[ind].c[1]; XYZ[2]= points[ind].c[2];  
			if (polyrep_verbose) 
				printf("Render (points) #%d = [%.5f, %.5f, %.5f]\n",ind,XYZ[0],XYZ[1],XYZ[2]);  
		} else if(r->coord) {	
			XYZ[0]=r->coord[3*ind+0]; XYZ[1]=r->coord[3*ind+1]; XYZ[2]=r->coord[3*ind+2]; 
			if (polyrep_verbose) 
				printf("Render (r->coord) #%d = [%.5f, %.5f, %.5f]\n",ind,XYZ[0],XYZ[1],XYZ[2]);  
		}


		/* Textures	*/
		if (HAVETODOTEXTURES) {
		    if(texcoords && ntexcoords) {
			// did we run out of tex coords? Hanim-Nancy does this...
			if (tci < ntexcoords) {
			    if (polyrep_verbose) {
				printf ("tc1 tci %d %f %f\n",tci,texcoords[tci].c[0],texcoords[tci].c[1]); 
			    }
		  	    glTexCoord2fv(texcoords[tci].c);
			} else {
				 if (polyrep_verbose)
					 printf ("caught ntexcoord problem: index %d gt %d\n",tci,ntexcoords);
			}
		    } else if (r->tcoord) {
			if (r->tcindex) {
				if (polyrep_verbose) 
					printf ("tc2a %f %f\n", r->tcoord[3*tci+0], r->tcoord[3*tci+2]);
		  		glTexCoord2f( r->tcoord[3*tci+0], r->tcoord[3*tci+2]);
			} else {
				if (polyrep_verbose) 
					printf ("tc2b %f %f\n", r->tcoord[3*ind+0], r->tcoord[3*ind+2]);
		  		glTexCoord2f( r->tcoord[3*ind+0], r->tcoord[3*ind+2]);
			}
		    } else {
			/* default textures */
			/* we want the S values to range from 0..1, and the 
			   T values to range from 0...S/T */
		  	if (polyrep_verbose) printf ("tc3, %f %f\n", (XYZ[Sindex] - minVals[Sindex])/Ssize,
                                        (XYZ[Tindex] - minVals[Tindex])/Ssize);
			
			glTexCoord2f( (XYZ[Sindex] - minVals[Sindex])/Ssize,
					(XYZ[Tindex] - minVals[Tindex])/Ssize);
		    }

		}

		/* now, make the Vertex */
		glVertex3fv (XYZ);
	}

	glEnd();

	if(hasc) {
		glDisable(GL_COLOR_MATERIAL);
	}

	if (polyrep_verbose)
		printf ("end render_polyrep\n\n");
}

/*********************************************************************
 *
 * render_ray_polyrep : get intersections of a ray with one of the
 * polygonal representations
 */

void render_ray_polyrep(void *node,
	int npoints, struct SFColor *points)
{
	struct VRML_Virt *v;
	struct VRML_Box *p;
	struct VRML_PolyRep *r;
	int i;
	int pt;
	float *point[3];
	struct pt v1, v2, v3;
	struct pt ray;
	float pt1, pt2, pt3;
	struct pt hitpoint;
	float tmp1,tmp2;
	float v1len, v2len, v3len;
	float v12pt;
	ray.x = t_r2.x - t_r1.x;
	ray.y = t_r2.y - t_r1.y;
	ray.z = t_r2.z - t_r1.z;
	v = *(struct VRML_Virt **)node;
	p = node;
	r = p->_intern;
	/*
	printf("render_ray_polyrep %d '%s' (%d %d): %d\n",node,v->name, 
		p->_change, r->_change, r->ntri);
	*/
	
	for(i=0; i<r->ntri; i++) {
		for(pt = 0; pt<3; pt++) {
			int ind = r->cindex[i*3+pt];
			if(points) {
				point[pt] = (points[ind].c);
			} else if(r->coord) {
				point[pt] = (r->coord+3*ind);
			}

		}
		/* First we need to project our point to the surface */
		/* Poss. 1: */
		/* Solve s1xs2 dot ((1-r)r1 + r r2 - pt0)  ==  0 */
		/* I.e. calculate s1xs2 and ... */
		v1.x = point[1][0] - point[0][0];
		v1.y = point[1][1] - point[0][1];
		v1.z = point[1][2] - point[0][2];
		v2.x = point[2][0] - point[0][0];
		v2.y = point[2][1] - point[0][1];
		v2.z = point[2][2] - point[0][2];
		v1len = sqrt(VECSQ(v1)); VECSCALE(v1, 1/v1len);
		v2len = sqrt(VECSQ(v2)); VECSCALE(v2, 1/v2len);
		v12pt = VECPT(v1,v2);

		/* this will get around a divide by zero further on JAS */
		if (fabs(v12pt-1.0) < 0.00001) continue;

		/* if we have a degenerate triangle, we can't compute a normal, so skip */
		
		if ((fabs(v1len) > 0.00001) && (fabs(v2len) > 0.00001)) {

			/* v3 is our normal to the surface */
			VECCP(v1,v2,v3);
			v3len = sqrt(VECSQ(v3)); VECSCALE(v3, 1/v3len);
	
			pt1 = VECPT(t_r1,v3);
			pt2 = VECPT(t_r2,v3);
			pt3 = v3.x * point[0][0] + v3.y * point[0][1] + 
				v3.z * point[0][2]; 
			/* Now we have (1-r)pt1 + r pt2 - pt3 = 0
			 * r * (pt1 - pt2) = pt1 - pt3
			 */
			 tmp1 = pt1-pt2;
			 if(!APPROX(tmp1,0)) {
			 	float ra, rb;
				float k,l;
				struct pt p0h;

			 	tmp2 = (pt1-pt3) / (pt1-pt2);
				hitpoint.x = MRATX(tmp2);
				hitpoint.y = MRATY(tmp2);
				hitpoint.z = MRATZ(tmp2);
				/* Now we want to see if we are in the triangle */
				/* Projections to the two triangle sides */
				p0h.x = hitpoint.x - point[0][0];
				p0h.y = hitpoint.y - point[0][1];
				p0h.z = hitpoint.z - point[0][2];
				ra = VECPT(v1, p0h);
				if(ra < 0) {continue;}
				rb = VECPT(v2, p0h);
				if(rb < 0) {continue;}
				/* Now, the condition for the point to
				 * be inside 
				 * (ka + lb = p)
				 * (k + l b.a = p.a)
				 * (k b.a + l = p.b)
				 * (k - (b.a)**2 k = p.a - (b.a)*p.b)
				 * k = (p.a - (b.a)*(p.b)) / (1-(b.a)**2)
				 */
				 k = (ra - v12pt * rb) / (1-v12pt*v12pt);
				 l = (rb - v12pt * ra) / (1-v12pt*v12pt);
				 k /= v1len; l /= v2len;
				 if(k+l > 1 || k < 0 || l < 0) {
				 	continue;
				 }
				 rayhit(tmp2, hitpoint.x,hitpoint.y,hitpoint.z,
				 	v3.x,v3.y,v3.z, -1,-1, "polyrep");
			 }
		/*
		} else {
			printf ("render_ray_polyrep, skipping degenerate triangle\n");
		*/
		}
	}
}

void regen_polyrep(void *node) 
{
	struct VRML_Virt *v;
	struct VRML_Box *p;
	struct VRML_PolyRep *r;
	v = *(struct VRML_Virt **)node;

	p = node;
	/* printf("Regen polyrep %d '%s'\n",node,v->name); */
	if(!p->_intern) {

		p->_intern = malloc(sizeof(struct VRML_PolyRep));

		/* in C always check if you got the mem you wanted...  >;->		*/
		if (!(p->_intern)) {
			die("Not enough memory to regen_polyrep... ;(");
		} 
 
		r = p->_intern;
		r->ntri = -1;
		r->cindex = 0; r->coord = 0; r->colindex = 0; r->color = 0;
		r->norindex = 0; r->normal = 0; r->tcoord = 0;
		r->tcindex = 0;  
	}
	r = p->_intern;
	r->_change = p->_change;

#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}
	FREE_IF_NZ(r->cindex);
	FREE_IF_NZ(r->coord);
	FREE_IF_NZ(r->tcoord);
	FREE_IF_NZ(r->colindex);
	FREE_IF_NZ(r->color);
	FREE_IF_NZ(r->norindex);
	FREE_IF_NZ(r->normal);
	FREE_IF_NZ(r->tcindex);
	v->mkpolyrep(node);
}

