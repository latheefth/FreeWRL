/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "CORE/EXTERN.h"
#include "CORE/perl.h"
#include "CORE/XSUB.h"
#include <math.h>
 
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "Structs.h" 
#include "headers.h"  


/* transformed ray */
extern struct pt t_r1;
extern struct pt t_r2;
extern struct pt t_r3;



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
	

	/* bounds check  XXX should free all mallocd memory */	
	if (min_points_per_face < 3) { 
		printf ("have an IFS with a face with too few vertex\n"); 
		return(1);
	}
	if (faces < 1) {
		printf("an IndexedFaceSet with no faces found\n");
		return (1);
	}
	return faces;
}


/* Generate the normals for each face of an IndexedFaceSet	*/
/* create two datastructures:					*/
/* 	- face normals; given a face, tell me the normal	*/
/*	- point-face;   for each point, tell me the face(s)	*/

void IFS_face_normals (
	struct pt *facenormals,
	int *pointfaces,
	int faces, 
	int npoints,
	int cin,
	struct SFColor *points,
	struct VRML_IndexedFaceSet *this_IFS) {

	int tmp_a, tmp_b, tmp_c;
	int i;
	int facectr;
	int pt_1, pt_2, pt_3;
	float AC, BC;
	struct SFColor *c1,*c2,*c3;
	float a[3]; float b[3];

	tmp_a = 0;
	for(i=0; i<faces; i++) {
		/* check for degenerate triangles -- if found, try to select another point */
		tmp_c = FALSE;
		pt_1 = tmp_a; pt_2 = tmp_a+1; pt_3 = tmp_a+2;

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

			/* printf ("vector length is %f\n",calc_vector_length (facenormals[i])); */

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

		/* skip forward to next ifs - we have the normal */
		if (i<faces-1) {
			while ((this_IFS->coordIndex.p[tmp_a-1]) != -1) {
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
			tmp_a*=POINT_FACES;
			/* is this point in too many faces? if not, record it */
			if (pointfaces[tmp_a] < (POINT_FACES-1)) {
				pointfaces[tmp_a]++;
				pointfaces[tmp_a+ pointfaces[tmp_a]] = facectr; 
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
	GLfloat Ssize, Tsize = 0.0;
	GLfloat Xsize, Ysize, Zsize = 0.0;
	int Sindex, Tindex = 0;


	v = *(struct VRML_Virt **)node;
	p = node;
	r = p->_intern;

	/*	
	printf ("rp, p is %x r is %x r-norm is %x\n",p,r,r->normal);
	printf("Render polyrep %d '%s' (%d %d): %d\n",node,v->name, 
			p->_change, r->_change, r->ntri);
	printf ("\tnpoints %d ncolors %d nnormals %d\n",
			npoints,ncolors,nnormals);
	printf("\tntexcoords = %d    texcoords = 0x%lx\n",
			ntexcoords, texcoords);
	*/	

	/* do we need to generate default texture mapping? */
	if (glIsEnabled(GL_TEXTURE_2D) && (ntexcoords == 0) && (!r->tcoord)) {
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


	/* Do we have any colours? Are textures NOT enabled? */
	hasc = ((ncolors || r->color) && (!glIsEnabled(GL_TEXTURE_2D)));
	if(hasc) {
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);
	}

	glBegin(GL_TRIANGLES);
	  for(i=0; i<r->ntri*3; i++) {
		int nori = i;
		int coli = i;
		int tci = i;
		int ind = r->cindex[i];

		/* printf ("rp, i, ntri*3 %d %d\n",i,r->ntri*3);   */

		/* get normals and colors, if any	*/
		if(r->norindex) {nori = r->norindex[i];}
		else nori = ind;
		if(r->colindex) {
			coli = r->colindex[i];
		}
		else coli = ind;

		/* get texture coordinates, if any	*/
		if (glIsEnabled(GL_TEXTURE_2D)) {
		if((r->tcindex) && (ntexcoords)) {tci = r->tcindex[i]; }
		}

		/* get the normals, if there are any	*/
		if(nnormals) {
			if(nori >= nnormals) {
				/* this should be caught before here JAS */
				warn("Too large normal index -- help??");
			}
			glNormal3fv(normals[nori].c);
		} else if(r->normal) {
			/*
			printf ("r->normal nori %d ",nori);
			fwnorprint(r->normal+3*nori);
			*/
			glNormal3fv(r->normal+3*nori);
		}

		if(hasc && prevcolor != coli) {
			if(ncolors) { 
				/* ColorMaterial -> these set Material too */
				glColor3fv(colors[coli].c);
			} else if(r->color) {
				glColor3fv(r->color+3*coli);
			}
		}
		prevcolor = coli;


		/* Coordinate points	*/
		if(points) {
			XYZ[0]= points[ind].c[0]; XYZ[1]= points[ind].c[1]; XYZ[2]= points[ind].c[2];  
			/* 
			printf("Render (points) #%d = [%.5f, %.5f, %.5f]\n",ind,XYZ[0],XYZ[1],XYZ[2]);  
			*/ 
		} else if(r->coord) {	
			XYZ[0]=r->coord[3*ind+0]; XYZ[1]=r->coord[3*ind+1]; XYZ[2]=r->coord[3*ind+2]; 
			/* 
			printf("Render (r->coord) #%d = [%.5f, %.5f, %.5f]\n",ind,XYZ[0],XYZ[1],XYZ[2]);  
			*/ 
		}

		/* Textures	*/
		if (glIsEnabled(GL_TEXTURE_2D)) {
		    if(texcoords && ntexcoords) {
			// printf ("tc1 %f %f\n",texcoords[tci].c[0],texcoords[tci].c[1]); 
		  	glTexCoord2fv(texcoords[tci].c);
		    } else if (r->tcoord) {
			// printf ("tc2 %f %f\n", r->tcoord[3*ind+0], r->tcoord[3*ind+2]);
		  	glTexCoord2f( r->tcoord[3*ind+0], r->tcoord[3*ind+2]);
		    } else {
			/* default textures */
			/* we want the S values to range from 0..1, and the 
			   T values to range from 0...S/T */
		  	/* printf ("tc3, %f %f\n", (XYZ[Sindex] - minVals[Sindex])/Ssize,
                                        (XYZ[Tindex] - minVals[Tindex])/Ssize);
			*/
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
				 HIT(tmp2, hitpoint.x,hitpoint.y,hitpoint.z,
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

		if (!(p->_intern)) free (p->_intern);

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
	v->mkpolyrep(node);
}

