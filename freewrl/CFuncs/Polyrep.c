
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
 *********************************************************************
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

