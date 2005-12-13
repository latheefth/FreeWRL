/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Geometry 3D Component

*********************************************************************/

#include "headers.h"

void render_ElevationGrid (struct VRML_ElevationGrid *this);
void render_Extrusion (struct VRML_Extrusion *this);


void render_Box (struct VRML_Box *this_) {
	extern GLfloat boxtex[];		/*  in CFuncs/statics.c*/
	extern GLfloat boxnorms[];		/*  in CFuncs/statics.c*/
	float *pt;
	float x = ((this_->size).c[0])/2;
	float y = ((this_->size).c[1])/2;
	float z = ((this_->size).c[2])/2;

	/* test for <0 of sides */
	if ((x < 0) || (y < 0) || (z < 0)) return;

	/* for BoundingBox calculations */
	setExtent(x,y,z,(struct VRML_Box *)this_);


	if (this_->_ichange != this_->_change) {
		/*  have to regen the shape*/

		this_->_ichange = this_->_change;

		/*  malloc memory (if possible)*/
		if (!this_->__points) this_->__points = malloc (sizeof(struct SFColor)*(24));
		if (!this_->__points) {
			printf ("can not malloc memory for box points\n");
			return;
		}

		/*  now, create points; 4 points per face.*/
		pt = (float *) this_->__points;
		/*  front*/
		*pt++ =  x; *pt++ =  y; *pt++ =  z; *pt++ = -x; *pt++ =  y; *pt++ =  z;
		*pt++ = -x; *pt++ = -y; *pt++ =  z; *pt++ =  x; *pt++ = -y; *pt++ =  z;
		/*  back*/
		*pt++ =  x; *pt++ = -y; *pt++ = -z; *pt++ = -x; *pt++ = -y; *pt++ = -z;
		*pt++ = -x; *pt++ =  y; *pt++ = -z; *pt++ =  x; *pt++ =  y; *pt++ = -z;
		/*  top*/
		*pt++ = -x; *pt++ =  y; *pt++ =  z; *pt++ =  x; *pt++ =  y; *pt++ =  z;
		*pt++ =  x; *pt++ =  y; *pt++ = -z; *pt++ = -x; *pt++ =  y; *pt++ = -z;
		/*  down*/
		*pt++ = -x; *pt++ = -y; *pt++ = -z; *pt++ =  x; *pt++ = -y; *pt++ = -z;
		*pt++ =  x; *pt++ = -y; *pt++ =  z; *pt++ = -x; *pt++ = -y; *pt++ =  z;
		/*  right*/
		*pt++ =  x; *pt++ = -y; *pt++ =  z; *pt++ =  x; *pt++ = -y; *pt++ = -z;
		*pt++ =  x; *pt++ =  y; *pt++ = -z; *pt++ =  x; *pt++ =  y; *pt++ =  z;
		/*  left*/
		*pt++ = -x; *pt++ = -y; *pt++ =  z; *pt++ = -x; *pt++ =  y; *pt++ =  z;
		*pt++ = -x; *pt++ =  y; *pt++ = -z; *pt++ = -x; *pt++ = -y; *pt++ = -z;
	}


	if(!this_->solid) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
	}

	/*  Draw it; assume VERTEX and NORMALS already defined.*/
	textureDraw_start(NULL,boxtex);
	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__points);
	glNormalPointer (GL_FLOAT,0,boxnorms);

	/* do the array drawing; sides are simple 0-1-2-3, 4-5-6-7, etc quads */
	glDrawArrays (GL_QUADS, 0, 24);
	textureDraw_end();
	if(!this_->solid) { glPopAttrib(); }
}

void render_Cylinder (struct VRML_Cylinder * this_) {
	#define CYLDIV 20
	float h = (this_->height)/2;
	float r = this_->radius;
	int i = 0;
	struct SFColor *pt;
	float a1, a2;
	extern GLfloat cylnorms[];		/*  in CFuncs/statics.c*/
	extern unsigned char cyltopindx[];	/*  in CFuncs/statics.c*/
	extern unsigned char cylbotindx[];	/*  in CFuncs/statics.c*/
	extern GLfloat cylendtex[];		/*  in CFuncs/statics.c*/
	extern GLfloat cylsidetex[];		/*  in CFuncs/statics.c*/

	if ((h < 0) || (r < 0)) {return;}

	/* for BoundingBox calculations */
	setExtent(r,h,r,(struct VRML_Box *)this_);

	if (this_->_ichange != this_->_change) {
		/*  have to regen the shape*/

		this_->_ichange = this_->_change;

		/*  malloc memory (if possible)*/
		if (!this_->__points) this_->__points = malloc(sizeof(struct SFColor)*2*(CYLDIV+4));
		if (!this_->__normals) this_->__normals = malloc(sizeof(struct SFColor)*2*(CYLDIV+1));
		if ((!this_->__normals) || (!this_->__points)) {
			printf ("error mallocing memory for Cylinder\n");
			return;
		}
		/*  now, create the vertices; this is a quad, so each face = 4 points*/
		pt = (struct SFColor *) this_->__points;
		for (i=0; i<CYLDIV; i++) {
			a1 = PI*2*i/(float)CYLDIV;
			a2 = PI*2*(i+1)/(float)CYLDIV;
			pt[i*2+0].c[0] = r*sin(a1);
			pt[i*2+0].c[1] = (float) h;
			pt[i*2+0].c[2] = r*cos(a1);
			pt[i*2+1].c[0] = r*sin(a1);
			pt[i*2+1].c[1] = (float) -h;
			pt[i*2+1].c[2] = r*cos(a1);
		}

		/*  wrap the points around*/
		memcpy (&pt[CYLDIV*2].c[0],&pt[0].c[0],sizeof(struct SFColor)*2);

		/*  center points of top and bottom*/
		pt[CYLDIV*2+2].c[0] = 0.0; pt[CYLDIV*2+2].c[1] = (float) h; pt[CYLDIV*2+2].c[2] = 0.0;
		pt[CYLDIV*2+3].c[0] = 0.0; pt[CYLDIV*2+3].c[1] = (float)-h; pt[CYLDIV*2+3].c[2] = 0.0;
	}

	if(!this_->solid) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
	}


	/*  Display the shape*/
	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__points);

	if (this_->side) {
		glNormalPointer (GL_FLOAT,0,cylnorms);
		textureDraw_start(NULL,cylsidetex);

		/* do the array drawing; sides are simple 0-1-2,3-4-5,etc triangles */
		glDrawArrays (GL_QUAD_STRIP, 0, (CYLDIV+1)*2);
	}
	if(this_->bottom) {
		textureDraw_start(NULL,cylendtex);
		glDisableClientState (GL_NORMAL_ARRAY);
		glNormal3f(0.0,-1.0,0.0);
		glDrawElements (GL_TRIANGLE_FAN, CYLDIV+2 ,GL_UNSIGNED_BYTE,cylbotindx);
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if (this_->top) {
		textureDraw_start(NULL,cylendtex);
		glDisableClientState (GL_NORMAL_ARRAY);
		glNormal3f(0.0,1.0,0.0);
		glDrawElements (GL_TRIANGLE_FAN, CYLDIV+2 ,GL_UNSIGNED_BYTE,cyltopindx);
		glEnableClientState(GL_NORMAL_ARRAY);
	}
	textureDraw_end();

	if(!this_->solid) { glPopAttrib(); }
}

void render_Cone (struct VRML_Cone *this_) {
	/*  DO NOT change this define, unless you want to recalculate statics below....*/
	#define  CONEDIV 20

	float h = (this_->height)/2;
	float r = this_->bottomRadius;
	float angle;
	int i;
	struct SFColor *pt;			/*  bottom points*/
	struct SFColor *spt;			/*  side points*/
	struct SFColor *norm;			/*  side normals*/
	extern unsigned char tribotindx[];	/*  in CFuncs/statics.c*/
	extern float tribottex[];		/*  in CFuncs/statics.c*/
	extern float trisidtex[];		/*  in CFuncs/statics.c*/

	if ((h < 0) || (r < 0)) {return;}

	/* for BoundingBox calculations */
	setExtent(r,h,r,(struct VRML_Box *)this_);

	if (this_->_ichange != this_->_change) {
		/*  have to regen the shape*/
		this_->_ichange = this_->_change;

		/*  malloc memory (if possible)*/
		if (!this_->__botpoints) this_->__botpoints = malloc (sizeof(struct SFColor)*(CONEDIV+3));
		if (!this_->__sidepoints) this_->__sidepoints = malloc (sizeof(struct SFColor)*3*(CONEDIV+1));
		if (!this_->__normals) this_->__normals = malloc (sizeof(struct SFColor)*3*(CONEDIV+1));
		if ((!this_->__normals) || (!this_->__botpoints) || (!this_->__sidepoints)) {
			printf ("failure mallocing more memory for Cone rendering\n");
			return;
		}

		/*  generate the vertexes for the triangles; top point first. (note: top point no longer used)*/
		pt = (struct SFColor *)this_->__botpoints;
		pt[0].c[0] = 0.0; pt[0].c[1] = (float) h; pt[0].c[2] = 0.0;
		for (i=1; i<=CONEDIV; i++) {
			pt[i].c[0] = r*sin(PI*2*i/(float)CONEDIV);
			pt[i].c[1] = (float) -h;
			pt[i].c[2] = r*cos(PI*2*i/(float)CONEDIV);
		}
		/*  and throw another point that is centre of bottom*/
		pt[CONEDIV+1].c[0] = 0.0; pt[CONEDIV+1].c[1] = (float) -h; pt[CONEDIV+1].c[2] = 0.0;

		/*  and, for the bottom, [CONEDIV] = [CONEDIV+2]; but different texture coords, so...*/
		memcpy (&pt[CONEDIV+2].c[0],&pt[CONEDIV].c[0],sizeof (struct SFColor));

		/*  side triangles. Make 3 seperate points per triangle... makes glDrawArrays with normals*/
		/*  easier to handle.*/
		/*  rearrange bottom points into this array; top, bottom, left.*/
		spt = (struct SFColor *)this_->__sidepoints;
		for (i=0; i<CONEDIV; i++) {
			/*  top point*/
			spt[i*3].c[0] = 0.0; spt[i*3].c[1] = (float) h; spt[i*3].c[2] = 0.0;
			/*  left point*/
			memcpy (&spt[i*3+1].c[0],&pt[i+1].c[0],sizeof (struct SFColor));
			/* right point*/
			memcpy (&spt[i*3+2].c[0],&pt[i+2].c[0],sizeof (struct SFColor));
		}

		/*  wrap bottom point around once again... ie, final right point = initial left point*/
		memcpy (&spt[(CONEDIV-1)*3+2].c[0],&pt[1].c[0],sizeof (struct SFColor));

		/*  Side Normals - note, normals for faces doubled - see malloc above*/
		/*  this gives us normals half way between faces. 1 = face 1, 3 = face2, 5 = face 3...*/
		norm = (struct SFColor *)this_->__normals;
		for (i=0; i<=CONEDIV; i++) {
			/*  top point*/
			angle = PI * 2 * (i+0.5) / (float) (CONEDIV);
			norm[i*3+0].c[0] = sin(angle); norm[i*3+0].c[1] = (float)h/r; norm[i*3+0].c[2] = cos(angle);
			/* left point*/
			angle = PI * 2 * (i+0) / (float) (CONEDIV);
			norm[i*3+1].c[0] = sin(angle); norm[i*3+1].c[1] = (float)h/r; norm[i*3+1].c[2] = cos(angle);
			/*  right point*/
			angle = PI * 2 * (i+1) / (float) (CONEDIV);
			norm[i*3+2].c[0] = sin(angle); norm[i*3+2].c[1] = (float)h/r; norm[i*3+2].c[2] = cos(angle);
		}
	}


	if(!this_->solid) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
	}


	/*  OK - we have vertex data, so lets just render it.*/
	/*  Always assume GL_VERTEX_ARRAY and GL_NORMAL_ARRAY are enabled.*/

	if(this_->bottom) {
		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__botpoints);
		textureDraw_start(NULL,tribottex);
		glNormal3f(0.0,-1.0,0.0);
		glDrawElements (GL_TRIANGLE_FAN, CONEDIV+2, GL_UNSIGNED_BYTE,tribotindx);
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if(this_->side) {
		glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__sidepoints);
		glNormalPointer (GL_FLOAT,0,(GLfloat *)this_->__normals);
		textureDraw_start(NULL,trisidtex);

		/* do the array drawing; sides are simple 0-1-2,3-4-5,etc triangles */
		glDrawArrays (GL_TRIANGLES, 0, 60);
	}
	textureDraw_end();


	if(!this_->solid) { glPopAttrib(); }

}

void render_Sphere (struct VRML_Sphere *this_) {
	#define INIT_TRIG1(div) t_aa = sin(PI/(div)); t_aa *= 2*t_aa; t_ab = -sin(2*PI/(div));
	#define START_TRIG1 t_sa = 0; t_ca = -1;
	#define UP_TRIG1 t_sa1 = t_sa; t_sa -= t_sa*t_aa - t_ca * t_ab; t_ca -= t_ca * t_aa + t_sa1 * t_ab;
	#define SIN1 t_sa
	#define COS1 t_ca
	#define INIT_TRIG2(div) t2_aa = sin(PI/(div)); t2_aa *= 2*t2_aa; t2_ab = -sin(2*PI/(div));
	#define START_TRIG2 t2_sa = -1; t2_ca = 0;
	#define UP_TRIG2 t2_sa1 = t2_sa; t2_sa -= t2_sa*t2_aa - t2_ca * t2_ab; t2_ca -= t2_ca * t2_aa + t2_sa1 * t2_ab;
	#define SIN2 t2_sa
	#define COS2 t2_ca

	/*  make the divisions 20; dont change this, because statics.c values*/
	/*  will then need recaculating.*/
	#define SPHDIV 20

	extern GLfloat spherenorms[];		/*  side normals*/
	extern float spheretex[];		/*  in CFuncs/statics.c*/
	int count;
	float rad = this_->radius;

	if (rad<=0.0) {
		/* printf ("invalid sphere rad %f\n",rad);*/
		return;}

	/* for BoundingBox calculations */
	setExtent(rad,rad,rad,(struct VRML_Box *)this_);

	if (this_->_ichange != this_->_change) {
		int v; int h;
		float t_aa, t_ab, t_sa, t_ca, t_sa1;
		float t2_aa, t2_ab, t2_sa, t2_ca, t2_sa1;
		struct SFColor *pts;
		int count;

		/*  have to regen the shape*/

		this_->_ichange = this_->_change;

		/*  malloc memory (if possible)*/
		/*  2 vertexes per points. (+1, to loop around and close structure)*/
		if (!this_->__points) this_->__points =
			malloc (sizeof(struct SFColor) * SPHDIV * (SPHDIV+1) * 2);
		if (!this_->__points) {
			printf ("can not malloc memory in Sphere\n");
			return;
		}
		pts = (struct SFColor *) this_->__points;
		count = 0;

		INIT_TRIG1(SPHDIV)
		INIT_TRIG2(SPHDIV)

		START_TRIG1
		for(v=0; v<SPHDIV; v++) {
			float vsin1 = SIN1;
			float vcos1 = COS1, vsin2,vcos2;
			UP_TRIG1
			vsin2 = SIN1;
			vcos2 = COS1;
			START_TRIG2
			for(h=0; h<=SPHDIV; h++) {
				float hsin1 = SIN2;
				float hcos1 = COS2;
				UP_TRIG2
				pts[count].c[0] = rad * vsin2 * hcos1;
				pts[count].c[1] = rad * vcos2;
				pts[count].c[2] = rad * vsin2 * hsin1;
				count++;
				pts[count].c[0] = rad * vsin1 * hcos1;
				pts[count].c[1] = rad * vcos1;
				pts[count].c[2] = rad * vsin1 * hsin1;
				count++;
			}
		}
	}

	if(!this_->solid) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
	}


	/*  Display the shape*/
	textureDraw_start(NULL,spheretex);
	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__points);
	glNormalPointer (GL_FLOAT,0,spherenorms);

	/* do the array drawing; sides are simple 0-1-2,3-4-5,etc triangles */
	/* for (count = 0; count < SPHDIV; count ++) { */
	for (count = 0; count < SPHDIV/2; count ++) { 
		glDrawArrays (GL_QUAD_STRIP, count*(SPHDIV+1)*2, (SPHDIV+1)*2);
	}

	textureDraw_end();

	if(!this_->solid) { glPopAttrib(); }
}

void render_IndexedFaceSet (struct VRML_IndexedFaceSet *this_) {
		if (!this_->_intern || this_->_change != ((struct VRML_PolyRep *)this_->_intern)->_change)  
			regen_polyrep(this_, this_->coord, this_->color, this_->normal, this_->texCoord);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}

void render_ElevationGrid (struct VRML_ElevationGrid *this_) {
                if(!this_->_intern || this_->_change != ((struct VRML_PolyRep *)this_->_intern)->_change)
                        regen_polyrep(this_, NULL, this_->color, this_->normal, this_->texCoord);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}

void render_Extrusion (struct VRML_Extrusion *this_) {
                if(!this_->_intern || this_->_change != ((struct VRML_PolyRep *)this_->_intern)->_change)
                        regen_polyrep(this_, NULL,NULL,NULL,NULL);


		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}
