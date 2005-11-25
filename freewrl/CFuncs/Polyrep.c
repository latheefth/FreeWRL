/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/
#include "Polyrep.h"

/* reset colors to defaults, if we have to */
GLfloat diffuseColor[] = {0.8, 0.8, 0.8,1.0};
GLfloat ambientIntensity[] = {0.16, 0.16, 0.16, 1.0}; /*VRML diff*amb defaults */
GLfloat specularColor[] = {0.0, 0.0, 0.0, 1.0};
GLfloat emissiveColor[] = {0.0, 0.0, 0.0, 1.0};

GLfloat last_color[] = {0.0,0.0,0.0};


/* GENERIC POLYREP SMOOTH NORMAL DATABASE GENERATION 		*/


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

int IFS_face_normals (
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
	int i,checkpoint;
	int facectr;
	int pt_1, pt_2, pt_3;
	float AC, BC;
	struct SFColor *c1,*c2,*c3;
	float a[3]; float b[3];

	tmp_a = 0;
	int retval = FALSE;


	/*  Assume each face is ok for now*/
	for(i=0; i<faces; i++) {
		faceok[i] = TRUE;
	}

	/*  calculate normals for each face*/
	for(i=0; i<faces; i++) {
		if (tmp_a >= cin-2) {
			printf ("last face in Indexed Geometry has not enough vertexes\n");
			faceok[i] = FALSE;
		} else {
			/* does this face have at least 3 vertexes? */
			if ((this_IFS->coordIndex.p[tmp_a] == -1) ||
			    (this_IFS->coordIndex.p[tmp_a+1] == -1) ||
			    (this_IFS->coordIndex.p[tmp_a+2] == -1)) {
				printf ("IndexedFaceNormals: have a face with two or less vertexes\n");
				faceok[i] = FALSE;

				if (this_IFS->coordIndex.p[tmp_a] != -1) tmp_a++;
			} else {
				/* check to see that the coordIndex does not point to a
				   point that is outside the range of our point array */
				checkpoint = tmp_a;
				while (checkpoint < cin) {
					if (this_IFS->coordIndex.p[checkpoint] == -1) {
						checkpoint = cin; /*  stop the scan*/
					} else {
						/* printf ("verifying %d for face %d\n",this_IFS->coordIndex.p[checkpoint],i);*/
						if ((this_IFS->coordIndex.p[checkpoint] < 0) ||
						    (this_IFS->coordIndex.p[checkpoint] >= npoints)) {
							printf ("Indexed Geometry face %d has a point out of range,",i);
							printf (" point is %d, should be between 0 and %d\n",
								this_IFS->coordIndex.p[checkpoint],npoints-1);
							faceok[i] = FALSE;
						}
						checkpoint++;
					}
				}
			}
		}

		/* face has passed checks so far... */
		if (faceok[i]) {
			/* check for degenerate triangles -- if found, try to select another point */
			tmp_c = FALSE;
			pt_1 = tmp_a;
			if (ccw) {
				/* printf ("IFS face normals CCW\n");*/
				pt_2 = tmp_a+1; pt_3 = tmp_a+2;
			} else {
				/* printf ("IFS face normals *NOT* CCW\n");*/
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

				/* printf ("vector length is %f\n",calc_vector_length (facenormals[i]));*/

				if (fabs(calc_vector_length (facenormals[i])) < 0.00001) {
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

							/*  Mark this face to be bad*/
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

			/*
			printf ("vertices \t%f %f %f\n\t\t%f %f %f\n\t\t%f %f %f\n",
				c1->c[0],c1->c[1],c1->c[2],
				c2->c[0],c2->c[1],c2->c[2],
				c3->c[0],c3->c[1],c3->c[2]);
			printf ("normal %f %f %f\n\n",facenormals[i].x,
				facenormals[i].y,facenormals[i].z);
			*/
			

		}

		/* skip forward to next ifs - we have the normal - but check for bad Points!*/
		if (i<faces-1) {
			if (tmp_a <= 0) {
				/* this is an error in the input file; lets try and continue */
				tmp_a = 1;
			} 

			if (tmp_a > 0) {
				while (((this_IFS->coordIndex.p[tmp_a-1]) != -1) && (tmp_a < cin-2)) {
					/* printf ("skipping past %d for face %d\n",this_IFS->coordIndex.p[tmp_a-1],i);*/
					tmp_a++;
				}
			}
		}
	}


	/* do we have any valid faces??? */
	for(i=0; i<faces; i++) {
		if (faceok[i] == TRUE) {
			retval = TRUE;
		}
	}
	if (!retval) return retval; /* nope, lets just drop out of here */
	

	/* now, go through each face, and make a point-face list
	   so that I can give it a point later, and I will know which face(s)
	   it belong to that point */
	/* printf ("\nnow generating point-face list\n");   */
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
			/* 	printf ("skipping add_to_face for invalid face %d\n",facectr);*/
			}
		}
	}

	/*
	 printf ("\ncheck \n");
	for (i=0; i<npoints; i++) {
		int tmp_b;

		tmp_a = i*POINT_FACES;
		printf ("point %d is in %d faces, these are:\n ", i, pointfaces[tmp_a]);
		for (tmp_b=0; tmp_b<pointfaces[tmp_a]; tmp_b++) {
			printf ("%d ",pointfaces[tmp_a+tmp_b+1]);
		}
		printf ("\n");
	}
	*/

	return retval;
	
}



/* Tesselated faces MAY have the wrong normal calculated. re-calculate after tesselation	*/

void Extru_check_normal (
	struct pt *facenormals,
	int this_face,
	int direction,
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

	/* first three coords give us the normal */
 	c1 = (struct SFColor *) &rep_->coord[3*global_IFS_Coords[0]];
 	c2 = (struct SFColor *) &rep_->coord[3*global_IFS_Coords[zz1]];
 	c3 = (struct SFColor *) &rep_->coord[3*global_IFS_Coords[zz2]];

	/*printf ("Extru_check_normal, coords %d %d %d\n",global_IFS_Coords[0],
		global_IFS_Coords[1],global_IFS_Coords[2]);
	printf ("Extru_check_normal vertices \t%f %f %f\n\t\t%f %f %f\n\t\t%f %f %f\n",
		c1->c[0],c1->c[1],c1->c[2],
		c2->c[0],c2->c[1],c2->c[2],
		c3->c[0],c3->c[1],c3->c[2]);
	*/

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
	/* printf ("facenormal for %d is %f %f %f\n",this_face, facenormals[this_face].x,*/
	/* 		facenormals[this_face].y, facenormals[this_face].z);*/
}

/* Tesselated faces MAY have the wrong normal calculated. re-calculate after tesselation	*/


void IFS_check_normal (
	struct pt *facenormals,
	int this_face,
	struct SFColor *points, int base,
	struct VRML_IndexedFaceSet *this_IFS, int ccw) {

	struct SFColor *c1,*c2,*c3;
	float a[3]; float b[3];


	/* printf ("IFS_check_normal, base %d points %d %d %d\n",base,*/
	/* 	global_IFS_Coords[0],global_IFS_Coords[1],global_IFS_Coords[2]);*/
	/* printf ("normal was %f %f %f\n\n",facenormals[this_face].x,*/
	/* 	facenormals[this_face].y,facenormals[this_face].z);*/


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

	/* printf ("vector length is %f\n",calc_vector_length (facenormals[this_face]));*/

	if (fabs(calc_vector_length (facenormals[this_face])) < 0.0001) {
		/* printf ("warning: Tesselated surface has invalid normal - if this is an IndexedFaceSet, check coordinates of ALL faces\n");*/
	} else {

		normalize_vector(&facenormals[this_face]);


		/* printf ("vertices \t%f %f %f\n\t\t%f %f %f\n\t\t%f %f %f\n",*/
		/* 	c1->c[0],c1->c[1],c1->c[2],*/
		/* 	c2->c[0],c2->c[1],c2->c[2],*/
		/* 	c3->c[0],c3->c[1],c3->c[2]);*/
		/* printf ("normal %f %f %f\n\n",facenormals[this_face].x,*/
		/* 	facenormals[this_face].y,facenormals[this_face].z);*/
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

	/* printf ("Elev_Tri Triangle %d %d %d\n",A,D,E); */

	/* generate normals in a clockwise manner, reverse the triangle */
	if (!(ccw)) {
		tmp = D;
		D = E;
		E = tmp;
	}


	this_Elev->cindex[vertex_ind] = A;
	this_Elev->cindex[vertex_ind+1] = D;
	this_Elev->cindex[vertex_ind+2] = E;

	/*
	printf ("Elev_Tri, vertices for vertex_ind %d are:",vertex_ind);
               c1 = (struct SFColor *) &this_Elev->coord[3*A];
               c2 = (struct SFColor *) &this_Elev->coord[3*D];
               c3 = (struct SFColor *) &this_Elev->coord[3*E];

	printf ("\n%f %f %f\n%f %f %f\n%f %f %f\n\n",
		c1->c[0], c1->c[1],c1->c[2],c2->c[0],c2->c[1],c2->c[2],
		c3->c[0],c3->c[1],c3->c[2]);
	*/


	if (NONORMALS) {
		/* calculate normal for this triangle */
                c1 = (struct SFColor *) &this_Elev->coord[3*A];
                c2 = (struct SFColor *) &this_Elev->coord[3*D];
                c3 = (struct SFColor *) &this_Elev->coord[3*E];

		/*
		printf ("calc norms \n%f %f %f\n%f %f %f\n%f %f %f\n",
		c1->c[0], c1->c[1],c1->c[2],c2->c[0],c2->c[1],c2->c[2],
		c3->c[0],c3->c[1],c3->c[2]);
		*/

		a[0] = c2->c[0] - c1->c[0];
		a[1] = c2->c[1] - c1->c[1];
		a[2] = c2->c[2] - c1->c[2];
		b[0] = c3->c[0] - c1->c[0];
		b[1] = c3->c[1] - c1->c[1];
		b[2] = c3->c[2] - c1->c[2];

		facenormals[this_face].x = a[1]*b[2] - b[1]*a[2];
		facenormals[this_face].y = -(a[0]*b[2] - b[0]*a[2]);
		facenormals[this_face].z = a[0]*b[1] - b[0]*a[1];

		/*
		printf ("facenormals index %d is %f %f %f\n",this_face, facenormals[this_face].x,
				facenormals[this_face].y, facenormals[this_face].z);
		*/

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

	/* struct SFColor *c1,*c2,*c3; */
	int j;

	/* bounds check */
	/* printf ("Extru_tex, tcindexsize %d, vertex_ind %d\n",tcindexsize, vertex_ind); */

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
		 /* printf ("for textures, coord vals %f %f for sec %d\n",*/
		 /* Vals[x*2+0], Vals[x*2+1],x);*/
		if (Vals[x*2+0] < minS) minS = Vals[x*2+0];
		if (Vals[x*2+0] > maxS) maxS = Vals[x*2+0];
		if (Vals[x*2+1] < minT) minT = Vals[x*2+1];
		if (Vals[x*2+1] > maxT) maxT = Vals[x*2+1];
	}
	Srange = maxS -minS;
	Trange = maxT - minT;

	/* I hate divide by zeroes. :-) */
	/* if (Srange == 0.0) Srange = 0.001; */
	if (APPROX(Srange, 0.0)) Srange = 0.001;
	/* if (Trange == 0.0) Trange = 0.001; */
	if (APPROX(Trange, 0.0)) Trange = 0.001;

	/* printf ("minS %f Srange %f minT %f Trange %f\n",minS,Srange,minT,Trange);*/

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
		this_Extru->GeneratedTexCoords[tci*3+0] = (Vals[(tci-Point_Zero)*2+0] - minS) / Srange ;

		/* not used by render_polyrep */
		this_Extru->GeneratedTexCoords[tci*3+1] = 0;

		/* T value */
		this_Extru->GeneratedTexCoords[tci*3+2] = (Vals[(tci-Point_Zero)*2+1] - minT) / Trange;


		/* for second vertex */
		tci = this_Extru->tcindex[triind_start*3+1];
		ci = this_Extru->cindex[triind_start*3+1];

		if ((tci*3+2) >= tcoordsize) {
			printf ("INTERNAL ERROR: Extru_ST_map(2), index %d greater than %d \n",(tci*3+2),tcoordsize);
			return;
		}

		/* S value */
		this_Extru->GeneratedTexCoords[tci*3+0] = (Vals[(tci-Point_Zero)*2+0] - minS) / Srange ;

		/* not used by render_polyrep */
		this_Extru->GeneratedTexCoords[tci*3+1] = 0;

		/* T value */
		this_Extru->GeneratedTexCoords[tci*3+2] = (Vals[(tci-Point_Zero)*2+1] - minT) / Trange;


		/* for third vertex */
		tci = this_Extru->tcindex[triind_start*3+2];
		ci = this_Extru->cindex[triind_start*3+2];

		if ((tci*3+2) >= tcoordsize) {
			printf ("INTERNAL ERROR: Extru_ST_map(3), index %d greater than %d \n",(tci*3+2),tcoordsize);
			return;
		}

		/* S value */
		this_Extru->GeneratedTexCoords[tci*3+0] = (Vals[(tci-Point_Zero)*2+0] - minS) / Srange ;

		/* not used by render_polyrep */
		this_Extru->GeneratedTexCoords[tci*3+1] = 0;

		/* T value */
		this_Extru->GeneratedTexCoords[tci*3+2] = (Vals[(tci-Point_Zero)*2+1] - minT) / Trange;

		triind_start++;
	}
}


/* take 3 or 4 floats, bounds check them, and put them in a destination. 
   Used for copying color X3DColorNode values over for streaming the
   structure. */

void do_glColor4fv(struct SFColorRGBA *dest, GLfloat *param, int isRGBA) {
	int i;
	int pc;

	if (isRGBA) pc = 4; else pc = 3;

	/* parameter checks */
	for (i=0; i<pc; i++) {
		if ((param[i] < 0.0) || (param[i] >1.0)) {
			param[i] = 0.5;
		}
	}
	dest->r[0] = param[0];
	dest->r[1] = param[1];
	dest->r[2] = param[2];

	/* does this color have an alpha channel? */
	if (isRGBA) {
		dest->r[3] = param[3];
	} else {
		dest->r[3] = last_transparency;
	}
	/* printf ("do_glColor4fv, %f %f %f %f\n",dest->r[0],dest->r[1],dest->r[2],dest->r[3]); */

}


void do_glNormal3fv(struct SFColor *dest, GLfloat *param) {
	struct pt myp;

	/* normalize all vectors; even if they are coded into a VRML file */

	myp.x = param[0]; myp.y = param[1]; myp.z = param[2];

	normalize_vector (&myp);

	dest->c[0] = myp.x; dest->c[1] = myp.y; dest->c[2] = myp.z;
}





/*********************************************************************
 *
 * render_polyrep : render one of the internal polygonal representations
 * for some nodes
 *
 ********************************************************************/



void render_polyrep(void *node) {
	struct VRML_Virt *v;
	struct VRML_Box *genericNodePtr;
	struct VRML_IndexedFaceSet *IFSNodePtr;
	struct VRML_PolyRep *r;
	struct SFVec2f *tc;

	v = *(struct VRML_Virt **)node;
	genericNodePtr = (struct VRML_Box *)node;
	r = (struct VRML_PolyRep *)genericNodePtr->_intern;

	#ifdef TEXVERBOSE
	printf ("\nrender_polyrep, _nodeType %d\n",genericNodePtr->_nodeType); 
	#endif

	if (r->ntri==0) {
		/* no triangles */
		return;
	}

	/* save these values for streaming the texture coordinates later */
	global_tcin = r->tcindex;
	global_tcin_count = r->ntri*3;

	setExtent(genericNodePtr->_extent[0],genericNodePtr->_extent[1],genericNodePtr->_extent[2],genericNodePtr);

	/* Do we have any colours? Are textures, if present, not RGB? */
	if(r->color) {
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseColor);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientIntensity);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissiveColor);
		glEnable(GL_COLOR_MATERIAL);
	}

	/*  clockwise or not?*/
	if (!r->ccw) { glFrontFace(GL_CW); }

	/*  status bar, text do not have normals*/
	if (r->normal) glNormalPointer(GL_FLOAT,0,(GLfloat *) r->normal);
	else glDisableClientState(GL_NORMAL_ARRAY); 

	/*  textures?*/
	if (r->GeneratedTexCoords) {
			textureDraw_start(NULL,r->GeneratedTexCoords);
	} else {
		IFSNodePtr = (struct VRML_IndexedFaceSet *)node;
		/* printf ("no textures - is this the status bar? %d\n",IFSNodePtr->_nodeType); */
		if (IFSNodePtr->_nodeType != NODE_Statusbar) {
			textureDraw_start(IFSNodePtr, NULL);
		}
	}

	/*  colours?*/
	if (r->color) {
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4,GL_FLOAT,0,r->color);
	}

	/* do the array drawing; sides are simple 0-1-2,3-4-5,etc triangles */
	glVertexPointer(3,GL_FLOAT,0,(GLfloat *) r->coord);
	glDrawElements(GL_TRIANGLES,r->ntri*3,GL_UNSIGNED_INT, r->cindex);

/*
	{
		int i;
		int *cin;
		float *cod;
		float *tcod;
		tcod = r->GeneratedTexCoords;
		cod = r->coord;
		cin = r->cindex;

printf ("\n\nrender_polyrep:\n");
		for (i=0; i<r->ntri*3; i++) {
			printf ("i %d cindex %d vertex %f %f %f",i,cin[i],
				cod[cin[i]*3+0],
				cod[cin[i]*3+1],
				cod[cin[i]*3+2]);

			if (tcod != 0) {
			printf (" tex %f %f",
				tcod[cin[i]*2+0],
				tcod[cin[i]*2+1]);
			}
			printf ("\n");
		}
	}
*/

	/*  put things back to the way they were;*/
	if (!r->normal) glEnableClientState(GL_NORMAL_ARRAY);
	if (r->color) {
		glDisable(GL_COLOR_MATERIAL);
		glDisableClientState(GL_COLOR_ARRAY);
	}
	if (r->GeneratedTexCoords) {
			textureDraw_end();
	} else {
		IFSNodePtr = (struct VRML_IndexedFaceSet *)node;
		/* printf ("no textures - is this the status bar? %d\n",IFSNodePtr->_nodeType); */
		if (IFSNodePtr->_nodeType != NODE_Statusbar) {
			textureDraw_end();
		}
	}


	if (!r->ccw) glFrontFace(GL_CCW);

}


/*********************************************************************
 *
 * render_ray_polyrep : get intersections of a ray with one of the
 * polygonal representations
 */

void render_ray_polyrep(void *node, struct SFColor *points)
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

	if (!p->_intern) {
		printf ("render_ray_polyrep - no internal structure, returning\n");
		return;
	}


	ray.x = t_r2.x - t_r1.x;
	ray.y = t_r2.y - t_r1.y;
	ray.z = t_r2.z - t_r1.z;
	v = *(struct VRML_Virt **)node;
	p =(struct VRML_Box *) node;
	r = (struct VRML_PolyRep *)p->_intern;

	/*
	printf("render_ray_polyrep %d '%s' (%d %d): %d\n",node,v->name,
		p->_change, r->_change, r->ntri);
	*/

	for(i=0; i<r->ntri; i++) {
		for(pt = 0; pt<3; pt++) {
			int ind = r->cindex[i*3+pt];
			if (r->coord) {
				point[pt] = (r->coord+3*ind);
			} else if (points) {
				point[pt] = (points[ind].c);
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
				 rayhit(((float)(tmp2)),
					((float)(hitpoint.x)),
					((float)(hitpoint.y)),
					((float)(hitpoint.z)),
				 	((float)(v3.x)),
					((float)(v3.y)),
					((float)(v3.z)),
					((float)-1),((float)-1), "polyrep");
			 }
		/*
		} else {
			printf ("render_ray_polyrep, skipping degenerate triangle\n");
		*/
		}
	}
}

/* make the internal polyrep structure - this will contain the actual RUNTIME parameters for OpenGL */
void regen_polyrep(void *node, void *coord, void *color, void *normal, void *texCoord) {
	struct VRML_Virt *v;
	struct VRML_Box *p;
	struct VRML_PolyRep *r;

	v = *(struct VRML_Virt **)node;
	p = (struct VRML_Box *)node;

	/* first time through; make the intern structure for this polyrep node */
	if(!p->_intern) {
		p->_intern = malloc(sizeof(struct VRML_PolyRep));
		if (!(p->_intern)) 
			freewrlDie("Not enough memory to regen_polyrep... ;(");

		r = (struct VRML_PolyRep *)p->_intern;
		r->ntri = -1;
		r->cindex = 0; r->coord = 0; r->colindex = 0; r->color = 0;
		r->norindex = 0; r->normal = 0; r->GeneratedTexCoords = 0;
		r->tcindex = 0; 
		r->tcoordtype = 0;
	}
	r = (struct VRML_PolyRep *)p->_intern;
	r->_change = p->_change;

	FREE_IF_NZ(r->cindex);
	FREE_IF_NZ(r->coord);
	FREE_IF_NZ(r->GeneratedTexCoords);
	FREE_IF_NZ(r->colindex);
	FREE_IF_NZ(r->color);
	FREE_IF_NZ(r->norindex);
	FREE_IF_NZ(r->normal);
	FREE_IF_NZ(r->tcindex);


	/* make the node by calling the correct method */
	v->mkpolyrep(node);

	/* now, put the generic internal structure into OpenGL arrays for faster rendering */
	/* if there were errors, then rep->ntri should be 0 */
	if (r->ntri != 0)
		stream_polyrep(node, coord, color, normal, texCoord);

}

