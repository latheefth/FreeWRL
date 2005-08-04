# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# Name:        VRMLRend.c
# Description:
#              Fills Hash Variables with "C" Code. They are used by VRMLC.pm
#              to write the C functions-source to render different nodes.
#
#              Certain Abbreviation are used, some are substituted in the
#              writing process in get_rendfunc() [VRMLC.pm].
#              Others are "C-#defines".
#              e.g. for #define glTexCoord2f(a,b) glTexCoord2f(a,b) see gen() [VRMLC.pm]
#
#              Hashes filled in this file:
#                      %RendC, %PrepC, %FinC, %ChildC, %LightC
#
# $Log$
# Revision 1.158  2005/08/04 14:39:38  crc_canada
# more work on moving elevationgrid to streaming polyrep structure
#
# Revision 1.157  2005/08/03 18:41:41  crc_canada
# Working on Polyrep structure.
#
# Revision 1.156  2005/08/02 13:22:45  crc_canada
# Move ElevationGrid to new face set order.
#
# Revision 1.155  2005/06/29 17:00:13  crc_canada
# EAI and X3D Triangle code added.
#
# Revision 1.154  2005/06/24 12:35:10  crc_canada
# Changes to help with 64 bit compiles.
#
# Revision 1.153  2005/06/09 14:52:50  crc_canada
# ColorRGBA nodes supported.
#
# Revision 1.152  2005/04/18 20:40:46  crc_canada
# Some work on removing startup memory errors.
#
# Revision 1.151  2005/04/18 15:27:16  crc_canada
# speedup shapes that do not have textures by removing a glPushAttrib and glPopAttrib
#
# Revision 1.150  2005/03/30 21:56:55  crc_canada
# remove GL_TEXTURE_BIT on Shape - speeds up rendering.
#
# Revision 1.149  2005/03/22 15:15:47  crc_canada
# more compile bugs; binary files were dinked in last upload.
#
# Revision 1.148  2005/03/21 13:39:04  crc_canada
# change permissions, remove whitespace on file names, etc.
#
# Revision 1.147  2005/03/01 15:16:56  crc_canada
# 1.11 pre2 first files checked in; some bugs, internal X3D parsing start, etc.
#
# Revision 1.146  2005/02/10 14:50:25  crc_canada
# LineSet implemented.
#
# Revision 1.145  2005/01/16 20:55:08  crc_canada
# Various compile warnings removed; some code from Matt Ward for Alldev;
# some perl changes for generated code to get rid of warnings.
#
# Revision 1.144  2005/01/06 14:29:01  crc_canada
# IFS with color nodes and no appearance now rendered.
#
# Revision 1.143  2005/01/05 21:40:57  crc_canada
# Spheres of negative radius not displayed.
#
# Revision 1.142  2004/11/18 18:19:19  crc_canada
# SoundEngine work.
#
# Revision 1.141  2004/11/09 16:11:43  crc_canada
# added "pan" field for sound rendering.
#
# Revision 1.140  2004/10/22 19:02:42  crc_canada
# javascript work.
#
# Revision 1.139  2004/10/15 19:26:13  crc_canada
# Javascript bugs where if there were 2 scripts, with MFVecXf's, and all
# elements were not filled in, things would segfault. Believe it or not.
#
# Revision 1.138  2004/10/06 13:39:44  crc_canada
# Debian patches from Sam Hocevar.
#
# Revision 1.137  2004/09/21 17:52:46  crc_canada
# make some rendering improvements.
#
# Revision 1.136  2004/09/08 18:58:58  crc_canada
# More Frustum culling work.
#
# Revision 1.135  2004/08/23 17:46:26  crc_canada
# Bulk commit: IndexedLineWidth width setting, and more Frustum culling work.
#
# Revision 1.134  2004/08/06 18:36:55  crc_canada
# textureTransform nodes now reset the parameters in Appearance after a
# transform, instead of always before a texture is drawn. This fixes a
# Background bug (last texturetransform was applied to background images)
# and speeds up rendering of non-textureTransformed textures.
#
# Revision 1.133  2004/07/13 19:46:20  crc_canada
# solid flag for simple shapes.
#
# Revision 1.131  2004/06/21 15:15:20  crc_canada
# 1.07 pre changes.
#
# Revision 1.130  2004/06/21 13:12:58  crc_canada
# pre-1.07 changes.
#
# Revision 1.129  2004/06/10 20:05:52  crc_canada
# Extrusion (with concave endcaps) bug fixed; some javascript changes.
#
# Revision 1.128  2004/05/25 18:18:51  crc_canada
# more sorting of nodes
#
# Revision 1.126  2003/12/22 18:49:01  crc_canada
# replace URL.pm; now do via C or browser
#
# Revision 1.125  2003/12/04 18:33:57  crc_canada
# Basic threading ok
#
# Revision 1.124  2003/11/28 16:17:05  crc_canada
# Bindables now registered and handled in C
#
# Revision 1.123  2003/11/26 16:31:06  crc_canada
# First pass at threading.
#
# Revision 1.122  2003/10/24 14:04:08  crc_canada
# Fast drawing for polyreps - try1
#
# Revision 1.121  2003/10/22 19:36:29  crc_canada
# glDrawArrays and glDrawElements for simple shapes.
#
# Revision 1.119  2003/10/16 17:24:59  crc_canada
# remove unused code
#
# Revision 1.118  2003/10/10 14:10:17  crc_canada
# Compile time option for display lists added.
#



#######################################################################
#######################################################################
#######################################################################
#
# Rend --
#  actually render the node
#
#

# Rend = real rendering

%RendC = (

#Bindable nodes in seperate file now
#Viewpoint => ' ',
#GeoViewpoint => ' ',

NavigationInfo => 'render_NavigationInfo ((struct VRML_NavigationInfo *) this_);',

Fog => '
	if (!render_geom) printf ("rendering fog while not geom\n");
render_Fog((struct VRML_Fog *) this_);',

Background => '
	if (!render_geom) printf ("rendering background while not geom\n");
		render_Background ((struct VRML_Background *) this_); ',


Box => '
	extern GLfloat boxtex[];		/*  in CFuncs/statics.c*/
	extern GLfloat boxnorms[];		/*  in CFuncs/statics.c*/
	float *pt;
	float x = $f(size,0)/2;
	float y = $f(size,1)/2;
	float z = $f(size,2)/2;

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


	if(!$f(solid)) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
	}

	/*  Draw it; assume VERTEX and NORMALS already defined.*/
	if (HAVETODOTEXTURES) glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__points);
	glNormalPointer (GL_FLOAT,0,boxnorms);
	if (HAVETODOTEXTURES) glTexCoordPointer (2,GL_FLOAT,0,boxtex);

	/* do the array drawing; sides are simple 0-1-2-3, 4-5-6-7, etc quads */
	glDrawArrays (GL_QUADS, 0, 24);

	if (HAVETODOTEXTURES) glDisableClientState (GL_TEXTURE_COORD_ARRAY);

	if(!$f(solid)) { glPopAttrib(); }
',


Cylinder => '
	#define CYLDIV 20
	float h = $f(height)/2;
	float r = $f(radius);
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

	if(!$f(solid)) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
	}


	/*  Display the shape*/
	if (HAVETODOTEXTURES) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__points);

	if ($f(side)) {
		glNormalPointer (GL_FLOAT,0,cylnorms);
		if (HAVETODOTEXTURES) glTexCoordPointer (2,GL_FLOAT,0,cylsidetex);

		/* do the array drawing; sides are simple 0-1-2,3-4-5,etc triangles */
		glDrawArrays (GL_QUAD_STRIP, 0, (CYLDIV+1)*2);
	}
	if (HAVETODOTEXTURES) glTexCoordPointer (2,GL_FLOAT,0,cylendtex);
	if($f(bottom)) {
		glDisableClientState (GL_NORMAL_ARRAY);
		glNormal3f(0.0,-1.0,0.0);
		glDrawElements (GL_TRIANGLE_FAN, CYLDIV+2 ,GL_UNSIGNED_BYTE,cylbotindx);
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if ($f(top)) {
		glDisableClientState (GL_NORMAL_ARRAY);
		glNormal3f(0.0,1.0,0.0);
		glDrawElements (GL_TRIANGLE_FAN, CYLDIV+2 ,GL_UNSIGNED_BYTE,cyltopindx);
		glEnableClientState(GL_NORMAL_ARRAY);
	}
	/* set things back to normal - Textures and ColoUrs disabled. */
	if (HAVETODOTEXTURES) glDisableClientState(GL_TEXTURE_COORD_ARRAY);


	if(!$f(solid)) { glPopAttrib(); }
',


Cone => '
	/*  DO NOT change this define, unless you want to recalculate statics below....*/
	#define  CONEDIV 20

	float h = $f(height)/2;
	float r = $f(bottomRadius);
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


	if(!$f(solid)) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
	}


	/*  OK - we have vertex data, so lets just render it.*/
	/*  Always assume GL_VERTEX_ARRAY and GL_NORMAL_ARRAY are enabled.*/

	if (HAVETODOTEXTURES) glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	if($f(bottom)) {
		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__botpoints);
		if (HAVETODOTEXTURES) glTexCoordPointer (2,GL_FLOAT,0,tribottex);
		glNormal3f(0.0,-1.0,0.0);
		glDrawElements (GL_TRIANGLE_FAN, CONEDIV+2, GL_UNSIGNED_BYTE,tribotindx);
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if($f(side)) {
		glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__sidepoints);
		glNormalPointer (GL_FLOAT,0,(GLfloat *)this_->__normals);
		if (HAVETODOTEXTURES) glTexCoordPointer (2,GL_FLOAT,0,trisidtex);

		/* do the array drawing; sides are simple 0-1-2,3-4-5,etc triangles */
		glDrawArrays (GL_TRIANGLES, 0, 60);
	}
	/*  set things back to normal - Textures and ColoUrs disabled.*/
	if (HAVETODOTEXTURES) glDisableClientState(GL_TEXTURE_COORD_ARRAY);


	if(!$f(solid)) { glPopAttrib(); }
',

Sphere => '

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
	float rad = $f(radius);

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

	if(!$f(solid)) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
	}


	/*  Display the shape*/
	if (HAVETODOTEXTURES) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer (2,GL_FLOAT,0,spheretex);
	}
	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)this_->__points);
	glNormalPointer (GL_FLOAT,0,spherenorms);

	/* do the array drawing; sides are simple 0-1-2,3-4-5,etc triangles */
	for (count = 0; count < SPHDIV; count ++) {
		glDrawArrays (GL_QUAD_STRIP, count*(SPHDIV+1)*2, (SPHDIV+1)*2);
	}

	if (HAVETODOTEXTURES) glDisableClientState (GL_TEXTURE_COORD_ARRAY);


	if(!$f(solid)) { glPopAttrib(); }
',
IndexedFaceSet => '
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct SFColor *normals=0; int nnormals=0;
		struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;

		/* get "coord", "color", "normal", "texCoord", "colorIndex" */
		/* these use methods to get the values...		    */
		$fv_null(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);


		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}


		$mk_streamable_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);

		}
		render_polyrep(this_,
			npoints, points,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords, ct, ((struct VRML_PolyRep *)this_->_intern)->streamed);
		if(!$f(solid)) {
			glPopAttrib();
		}
',

IndexedTriangleFanSet => '
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct SFColor *normals=0; int nnormals=0;
		struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;

		/* get "coord", "color", "normal", "texCoord", "colorIndex" */
		/* these use methods to get the values...		    */
		$fv_null(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);


		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_streamable_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);

		}
		render_polyrep(this_,
			npoints, points,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords,ct, ((struct VRML_PolyRep *)this_->_intern)->streamed);
		if(!$f(solid)) {
			glPopAttrib();
		}
',
IndexedTriangleSet => '
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct SFColor *normals=0; int nnormals=0;
		struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;

		/* get "coord", "color", "normal", "texCoord", "colorIndex" */
		/* these use methods to get the values...		    */
		$fv(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);


		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_streamable_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);

		}
		render_polyrep(this_,
			npoints, points,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords,ct, ((struct VRML_PolyRep *)this_->_intern)->streamed);
		if(!$f(solid)) {
			glPopAttrib();
		}
',
IndexedTriangleStripSet => '
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct SFColor *normals=0; int nnormals=0;
		struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;

		/* get "coord", "color", "normal", "texCoord", "colorIndex" */
		/* these use methods to get the values...		    */
		$fv(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);


		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_streamable_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);

		}
		render_polyrep(this_,
			npoints, points,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords,ct, ((struct VRML_PolyRep *)this_->_intern)->streamed);
		if(!$f(solid)) {
			glPopAttrib();
		}
',
TriangleFanSet => '
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct SFColor *normals=0; int nnormals=0;
		struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;

		/* get "coord", "color", "normal", "texCoord", "colorIndex" */
		/* these use methods to get the values...		    */
		$fv(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);


		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_streamable_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);

		}
		render_polyrep(this_,
			npoints, points,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords,ct, ((struct VRML_PolyRep *)this_->_intern)->streamed);
		if(!$f(solid)) {
			glPopAttrib();
		}
',
TriangleStripSet => '
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct SFColor *normals=0; int nnormals=0;
		struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;

		/* get "coord", "color", "normal", "texCoord", "colorIndex" */
		/* these use methods to get the values...		    */
		$fv(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);


		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_streamable_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);

		}
		render_polyrep(this_,
			npoints, points,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords,ct, ((struct VRML_PolyRep *)this_->_intern)->streamed);
		if(!$f(solid)) {
			glPopAttrib();
		}
',
TriangleSet => '
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct SFColor *normals=0; int nnormals=0;
		struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;

		/* get "coord", "color", "normal", "texCoord", "colorIndex" */
		/* these use methods to get the values...		    */
		$fv(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);


		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_streamable_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);

		}
		render_polyrep(this_,
			npoints, points,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords,ct, ((struct VRML_PolyRep *)this_->_intern)->streamed);
		if(!$f(solid)) {
			glPopAttrib();
		}
',

LineSet => '
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */

	int vtc;		/* which vertexCount[] we should be using for this line segment */
	int ncoc;		/* which coord we are using for this vertex */
	int punt;		/* how many vetex points are in this line segment */
	int c;			/* temp variable */
	/* int *p;*/
	struct SFColor *coord=0; int ncoord;
	struct SFColor *color=0; int ncolor=0;
	int *vertexC; int nvertexc;


	glPushAttrib(GL_ENABLE_BIT);

	/* do we have to re-verify LineSet? */
	if (this_->_ichange != this_->_change) {
		/*  re-draw every time. this_->_ichange = this_->_change;*/

		nvertexc = (this_->vertexCount).n; vertexC = (this_->vertexCount).p;
		$fv(coord, coord, get3, &ncoord);
		$fv_null(color, color, get3, &ncolor);

		/* printf ("we have %d coords, %d colors\n",ncoord,ncolor);*/
		ncoc = 0;

		if ((nvertexc == 0) || (ncoord == 0)) {
			printf ("LineSet, no vertexCounts or no coords\n");
			this_->_ichange = this_->_change; /* make this error show only once */

			return;
		}


		if (ncolor != 0) {
			glDisable (GL_LIGHTING);
			glEnable(GL_COLOR_MATERIAL);
		} else {
			glNormal3f(0.0, 0.0, 1.0);
		}

		/* if we are re-genning; remalloc */
		/* if (this_->__points) free ((void *)this_->__points);*/

		/* if (!this_->__points) this_->__points = malloc (sizeof(unsigned int)*(nvertexc));*/
		/* if (!this_->__points) {*/
		/* 	printf ("can not malloc memory for LineSet points\n");*/
		/* 	this_->_ichange = this_->_change; make this error show only once */
		/* 	return;*/
		/* }*/
		/* p = (int *)this_->__points;*/


		/* go through the vertex count array and verify that all is good. */
		for (vtc = 0; vtc < nvertexc; vtc++) {
			/* save the pointer to the vertex array for the GL call */
			/* *p = vertexC;*/
			/* p++;*/

			punt = *vertexC;

			/* there HAVE to be 2 or more vertexen in a segment */
			if (punt < 2) {
				printf ("LineSet, vertexCount[%d] has %d vertices...\n",vtc,punt);
				this_->_ichange = this_->_change; /* make this error show only once */
				/* free ((void *)this_->__points);*/
glPopAttrib();
				return;
			}

			/* do we have enough Vertex Coords? */
			if ((punt + ncoc) > ncoord) {
				printf ("LineSet, ran out of vertices at vertexCount[%d] has %d vertices...\n",vtc,punt);
				this_->_ichange = this_->_change; /* make this error show only once */
				/* free ((void *)this_->__points);*/
glPopAttrib();
				return;
			}
			if (ncolor != 0) {
			if ((punt + ncoc) > ncolor) {
				printf ("LineSet, ran out of vertices at vertexCount[%d] has %d vertices...\n",vtc,punt);
				this_->_ichange = this_->_change; /* make this error show only once */
				/* free ((void *)this_->__points);*/
glPopAttrib();
				return;
			}
			}

			/*  do this for glMultiDrawElements ncoc += punt;*/
			glBegin(GL_LINE_STRIP);

			/* draw the line */
			if (ncolor!= 0) {
				glColor3f( color[ncoc].c[0],color[ncoc].c[1],color[ncoc].c[2]);
			}
			glVertex3f( coord[ncoc].c[0],coord[ncoc].c[1],coord[ncoc].c[2]);
			ncoc++;


			for (c=1; c<punt; c++) {
				if (ncolor!= 0) {
					glColor3f( color[ncoc].c[0],color[ncoc].c[1],color[ncoc].c[2]);
				}
				glVertex3f( coord[ncoc].c[0],coord[ncoc].c[1],coord[ncoc].c[2]);
				ncoc++;
			}

			/* now, lets go and get ready for the next vertexCount */
			vertexC++;
			glEnd();


		}
		if (ncolor != 0) {
			glEnable (GL_LIGHTING);
		}
	}

	/* now, actually draw array */
	/* if (this_->__points) {*/
	/* 	printf ("calling glMultiDrawElements, promcount %d\n",(this_->vertexCount).n);*/
	/* 	glMultiDrawElements(GL_LINE_STRIP,(this_->vertexCount).p,GL_FLOAT,*/
	/* 			this_->__points,(this_->vertexCount).n);*/
	/* }*/
glDisable(GL_COLOR_MATERIAL);
glPopAttrib();

',
IndexedLineSet => '
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
		int i;
		int cin = $f_n(coordIndex);
		int colin = $f_n(colorIndex);
		int cpv = $f(colorPerVertex);
		int plno = 0;
		int ind;
		int c;
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;


		if(verbose) printf("Line: cin %d colin %d cpv %d\n",cin,colin,cpv);
		$fv(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
                glPushAttrib(GL_ENABLE_BIT);
		glEnable(GL_COLOR_MATERIAL);
                glDisable(GL_CULL_FACE);

		if(ncolors && !cpv) {
			if (verbose) printf("glColor3f(%f,%f,%f);\n",
				  colors[plno].c[0],
				  colors[plno].c[1],
				  colors[plno].c[2]);
			glColor3f(colors[plno].c[0],
				  colors[plno].c[1],
				  colors[plno].c[2]);
		}

		glBegin(GL_LINE_STRIP);
		for(i=0; i<cin; i++) {
			ind = $f(coordIndex,i);
			if(verbose) printf("Line: %d %d\n",i,ind);

			if(ind==-1) {
				glEnd();
				plno++;
				if(ncolors && !cpv) {
					c = plno;
					if((!colin && plno < ncolors) ||
					   (colin && plno < colin)) {
						if(colin) {
							c = $f(colorIndex,c);
						}
						if (c<ncolors) {
						      glColor3f(colors[c].c[0],
						        colors[c].c[1],
						   	colors[c].c[2]);
						} else {
						      glColor3f(colors[0].c[0],
					        	colors[0].c[1],
						   	colors[0].c[2]);
						}

					}
				}
				glBegin(GL_LINE_STRIP);
			} else {
				if(ncolors && cpv) {
					c = i;
					if(colin) {
						c = $f(colorIndex,c);
					}
					if (c<ncolors) {
					      glColor3f(colors[c].c[0],
					        colors[c].c[1],
					   	colors[c].c[2]);
					} else {
					      glColor3f(colors[0].c[0],
					        colors[0].c[1],
					   	colors[0].c[2]);
					}

				}
				glVertex3f(
					points[ind].c[0],
					points[ind].c[1],
					points[ind].c[2]
				);
			}
		}
		glEnd();
		glDisable(GL_COLOR_MATERIAL);
                glPopAttrib();
',


PointSet => '
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	int i;
	struct SFColor *points=0; int npoints=0;
	struct SFColor *colors=0; int ncolors=0;

	$fv(coord, points, get3, &npoints);
	$fv_null(color, colors, get3, &ncolors);
	if(ncolors && ncolors < npoints) {
		printf ("PointSet has less colors than points - removing color\n");
		ncolors = 0;
	}
	glDisable(GL_LIGHTING);
	glBegin(GL_POINTS);
	if(verbose) printf("PointSet: %d %d\n", npoints, ncolors);
	for(i=0; i<npoints; i++) {
		if(ncolors) {
			if(verbose) printf("Color: %f %f %f\n",
				  colors[i].c[0],
				  colors[i].c[1],
				  colors[i].c[2]);
			glColor3f(colors[i].c[0],
				  colors[i].c[1],
				  colors[i].c[2]);
		}
		glVertex3f(
			points[i].c[0],
			points[i].c[1],
			points[i].c[2]
		);
	}
	glEnd();
	glEnable(GL_LIGHTING);
',
GeoElevationGrid => '
		struct SFColor *colors=0; int ncolors=0;
                struct SFVec2f *texcoords; int ntexcoords=0;
		struct SFColor *normals=0; int nnormals=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;


		/* these use methods to get the values...		    */
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);


		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_,
			0, NULL,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords,
			ct, -1
		);
		if(!$f(solid)) {
			glPopAttrib();
		}
',

JASElevationGrid =>  '
		struct SFColor *colors=0; int ncolors=0;
                struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct SFColor *normals=0; int nnormals=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;


		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);

		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_,
			0, NULL,
			ncolors, colors,
			nnormals, normals,
			/*JAS - ntexcoords, texcoords */
			0, NULL,
			ct, ((struct VRML_PolyRep *)this_->_intern)->streamed
			
		);
		if(!$f(solid)) {
			glPopAttrib();
		}
',
ElevationGrid =>  '
		struct SFColor *colors=0; int ncolors=0;
                struct SFVec2f *texcoords=0; int ntexcoords=0;
		struct SFColor *normals=0; int nnormals=0;
		struct VRML_ColorRGBA *thc;
		int ct=0;


		/* these use methods to get the values...		    */
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);

		/* get whether this is an RGB or an RGBA color node */
		if (colors != NULL) {
			thc = this_->color;
			ct = thc->__isRGBA;
		}

		$mk_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_,
			0, NULL,
			ncolors, colors,
			nnormals, normals,
			/*JAS - ntexcoords, texcoords */
			0, NULL,
			ct, -1
		);
		if(!$f(solid)) {
			glPopAttrib();
		}
',

Extrusion => '
		$mk_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_,0,NULL,0,NULL,0,NULL,0,NULL,0, ((struct VRML_PolyRep *)this_->_intern)->streamed);
		if(!$f(solid)) {
			glPopAttrib();
		}
',

# FontStyle params handled in Text.
FontStyle => 'UNUSED(this_);',

# Text is a polyrep, as of freewrl 0.34
Text => '
		$mk_polyrep();

		/* always Text is visible from both sides */
                glPushAttrib(GL_ENABLE_BIT);
                glDisable(GL_CULL_FACE);

		render_polyrep(this_,0,NULL,0,NULL,0,NULL,0,NULL,0,-1);

		glPopAttrib();
',

Material =>  '
		int i;
		float dcol[4];
		float ecol[4];
		float scol[4];
		float shin;
		float amb;
		float trans;

#ifndef X3DMATERIALPROPERTY
		/* We have to keep track of whether to reset diffuseColor if using
		   textures; no texture or greyscale, we use the diffuseColor, if
		   RGB we set diffuseColor to be grey */
		if (last_texture_depth >1) {
			dcol[0]=0.8, dcol[1]=0.8, dcol[2]=0.8;
		} else {
#endif

			for (i=0; i<3;i++){ dcol[i] = $f(diffuseColor,i); }
#ifndef X3DMATERIALPROPERTY
		}
#endif

		/* set the transparency here for the material */
		trans = 1.0 - $f(transparency);
		/* printf ("Material, trans %f\n",DOLLARf(transparency));*/
		if (trans<0.0) trans = 0.0;
		if (trans>=0.99) trans = 0.99;

		/* and, record that we have a transparency here */
		/* and record transparency value, in case we have an
		   indexedfaceset with colour node */
		if (trans <=0.99) {
			have_transparency++;
			if ((this_->_renderFlags & VF_Blend) != VF_Blend)
				update_renderFlag(this_,VF_Blend);
			last_transparency=trans;
		}

		dcol[3] = trans;

		do_glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dcol);

		amb = $f(ambientIntensity);
		for(i=0; i<3; i++) {
			dcol[i] *= amb;
		}
		do_glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, dcol);

		for (i=0; i<3;i++){ scol[i] = $f(specularColor,i); }
		scol[3] = trans;
		/* scol[3] = 1.0;*/
		do_glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, scol);

		for (i=0; i<3;i++){ ecol[i] = $f(emissiveColor,i); }
		ecol[3] = trans;
		/* ecol[3] = 1.0;*/

		do_glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ecol);
		glColor3f(ecol[0],ecol[1],ecol[2]);

		shin = 128.0*$f(shininess);
		do_shininess(shin);
',

TextureTransform => '
	have_textureTransform=TRUE;
       	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	/*  Render transformations according to spec.*/

	glTranslatef(-$f(center,0),-$f(center,1), 0);		/*  5*/
	glScalef($f(scale,0),$f(scale,1),1);			/*  4*/
	glRotatef($f(rotation)/3.1415926536*180,0,0,1);		/*  3*/
	glTranslatef($f(center,0),$f(center,1), 0);		/*  2*/
	glTranslatef($f(translation,0), $f(translation,1), 0); 	/*  1*/

	glMatrixMode(GL_MODELVIEW);
',

# Pixels and Images are all handled the same way now - the methods are identical.
PixelTexture => '
	loadPixelTexture(this_);
	last_bound_texture = this_->__texture;
',

ImageTexture => '
	loadImageTexture(this_);
	last_bound_texture = this_->__texture;
',

######
MovieTexture => '
	/* really simple, the texture number is calculated, then simply sent here.
	   The last_bound_texture field is sent, and, made current */

	/*  if this is attached to a Sound node, tell it...*/
	sound_from_audioclip = FALSE;

	loadMovieTexture(this_);
	last_bound_texture = this_->__ctex;
',



Sound => '
/* node fields...
	direction => [SFVec3f, [0, 0, 1]],
	intensity => [SFFloat, 1.0],
	location => [SFVec3f, [0,0,0]],
	maxBack => [SFFloat, 10],
	maxFront => [SFFloat, 10],
	minBack => [SFFloat, 1],
	minFront => [SFFloat, 1],
	priority => [SFFloat, 0],
	source => [SFNode, NULL],
	spatialize => [SFBool,1, ""]	# not exposedfield
*/

	GLdouble mod[16];
	GLdouble proj[16];
	struct pt vec, direction, location;
	double len;
	double angle;
	float midmin, midmax;
	float amp;

	struct VRML_AudioClip *acp;
	struct VRML_MovieTexture *mcp;
	char mystring[256];

	acp = (struct VRML_AudioClip *) $f(source);
	mcp = (struct VRML_MovieTexture *) $f(source);

	/*  MovieTextures NOT handled yet*/
	/*  first - is there a node (any node!) attached here?*/
	if (acp) {
		/*  do the sound registering first, and tell us if this is an audioclip*/
		/*  or movietexture.*/

		render_node(acp);

		/*  if the attached node is not active, just return*/
		/* printf ("in Sound, checking AudioClip isactive %d\n", acp->isActive);*/
		if (acp->isActive == 0) return;

		direction.x = $f(direction,0);
		direction.y = $f(direction,1);
		direction.z = $f(direction,2);

		location.x = $f(location,0);
		location.y = $f(location,1);
		location.z = $f(location,2);

		midmin = (this_->minFront - this_->minBack) / 2.0;
		midmax = (this_->maxFront - this_->maxBack) / 2.0;


		glPushMatrix();

		/*
		first, find whether or not we are within the maximum circle.

		translate to the location, and move the centre point, depending
		on whether we have a direction and differential maxFront and MaxBack
		directions.
		*/

		glTranslatef (location.x + midmax*direction.x,
				location.y + midmax*direction.y,
				location.z + midmax * direction.z);

		/* make the ellipse a circle by scaling...
		glScalef (direction.x*2.0 + 0.5, direction.y*2.0 + 0.5, direction.z*2.0 + 0.5);
		- scaling needs work - we need direction information, and parameter work. */

		if ((fabs(this_->minFront - this_->minBack) > 0.5) ||
			(fabs(this_->maxFront - this_->maxBack) > 0.5)) {
			if (!soundWarned) {
				printf ("FreeWRL:Sound: Warning - minBack and maxBack ignored in this version\n");
				soundWarned = TRUE;
			}
		}



		fwGetDoublev(GL_MODELVIEW_MATRIX, mod);
		fwGetDoublev(GL_PROJECTION_MATRIX, proj);
		gluUnProject (viewport[2]/2,viewport[3]/2,0.0,
			mod,proj,viewport, &vec.x,&vec.y,&vec.z);
		/* printf ("mod %lf %lf %lf proj %lf %lf %lf\n",*/
		/* mod[12],mod[13],mod[14],proj[12],proj[13],proj[14]);*/

		len = sqrt(VECSQ(vec));
		/* printf("Sound: len %f mB %f mF %f angles (%f %f %f)\n",len,*/
		/* 	-this_->maxBack, this_->maxFront,vec.x,vec.y,vec.z);*/


		/*  pan left/right. full left = 0; full right = 1.*/
		if (len < 0.001) angle = 0;
		else {
			if (APPROX (mod[12],0)) {
				/* printf ("mod12 approaches zero\n");*/
				mod[12] = 0.001;
			}
			angle = fabs(atan2(mod[14],mod[12])) - (PI/2.0);
			angle = angle/(PI/2.0);

			/*  Now, scale this angle to make it between -0.5*/
			/*  and +0.5; if we divide it by 2.0, we will get*/
			/*  this range, but if we divide it by less, then*/
			/*  the sound goes "hard over" to left or right for*/
			/*  a bit.*/
			angle = angle / 1.5;

			/*  now scale to 0 to 1*/
			angle = angle + 0.5;

			/*  bounds check...*/
			if (angle > 1.0) angle = 1.0;
			if (angle < 0.0) angle = 0.0;
			/* printf ("angle: %f\n",angle);*/
		}


		amp = 0.0;
		/* is this within the maxFront maxBack? */

		/* this code needs rework JAS */
		if (len < this_->maxFront) {

			/* note: using vecs, length is always positive - need to work in direction
			vector */
			if (len < 0.0) {
				if (len < this_->minBack) {amp = 1.0;}
				else {
					amp = (len - this_->maxBack) / (this_->maxBack - this_->minBack);
				}
			} else {
				if (len < this_->minFront) {amp = 1.0;}
				else {
					amp = (this_->maxFront - len) / (this_->maxFront - this_->minFront);
				}
			}

			/* Now, fit in the intensity. Send along command, with
			source number, amplitude, balance, and the current Framerate */
			amp = amp*this_->intensity;
			if (sound_from_audioclip) {
				sprintf (mystring,"AMPL %d %f %f",acp->__sourceNumber,amp,angle);
			} else {
				sprintf (mystring,"MMPL %d %f %f",mcp->__sourceNumber,amp,angle);
			}
			Sound_toserver(mystring);
		}
		glPopMatrix();
	}
',

AudioClip => '
	/*  register an audioclip*/
	float pitch,stime, sttime;
	int loop;
	unsigned char *filename = (unsigned char *)this_->__localFileName;

	/* tell Sound that this is an audioclip */
	sound_from_audioclip = TRUE;

	/* printf ("_change %d _ichange %d\n",this_->_change, this_->_ichange);  */

	if (!SoundEngineStarted) {
		/* printf ("AudioClip: initializing SoundEngine\n"); */
		SoundEngineStarted = TRUE;
		SoundEngineInit();
	}
#ifndef JOHNSOUND
	if (this_->isActive == 0) return;  /*  not active, so just bow out*/
#endif

	if (!SoundSourceRegistered(this_->__sourceNumber)) {

		 /* printf ("AudioClip: registering clip %d loop %d p %f s %f st %f url %s\n",
			this_->__sourceNumber,  this_->loop, this_->pitch,this_->startTime, this_->stopTime,
			filename);
		*/



		pitch = this_->pitch;
		stime = this_->startTime;
		sttime = this_->stopTime;
		loop = this_->loop;

		AC_LastDuration[this_->__sourceNumber] =
			SoundSourceInit (this_->__sourceNumber, this_->loop,
			(float) pitch,(float) stime, (float) sttime, (char *)filename);
		/* printf ("globalDuration source %d %f\n",
				this_->__sourceNumber,AC_LastDuration[this_->__sourceNumber]); */

		if (filename) free (filename);
	}


 ',

DirectionalLight => '
	/* NOTE: This is called by the Group Children code
	 * at the correct point (in the beginning of the rendering
	 * of the children. We just turn the light on right now.
	 */

	if($f(on)) {
		int light = nextlight();
		if(light >= 0) {
			float vec[4];
			glEnable(light);
			vec[0] = -$f(direction,0);
			vec[1] = -$f(direction,1);
			vec[2] = -$f(direction,2);
			vec[3] = 0;
			glLightfv(light, GL_POSITION, vec);
			vec[0] = $f(color,0) * $f(intensity);
			vec[1] = $f(color,1) * $f(intensity);
			vec[2] = $f(color,2) * $f(intensity);
			vec[3] = 1;
			glLightfv(light, GL_DIFFUSE, vec);
			glLightfv(light, GL_SPECULAR, vec);

			/* Aubrey Jaffer */
			vec[0] = $f(color,0) * $f(ambientIntensity);
			vec[1] = $f(color,1) * $f(ambientIntensity);
			vec[2] = $f(color,2) * $f(ambientIntensity);

			glLightfv(light, GL_AMBIENT, vec);
		}
	}
',

);

#######################################################################
#######################################################################
#######################################################################
#
# Prep --
#  Prepare for rendering a node - e.g. for transforms, do the transform
#  but not the children.
#
#

%PrepC = (
# this creates the Struct values required to allow backend to fill the C values out
ColorInterpolator => 'UNUSED(this_); /* compiler warning */',
PositionInterpolator => 'UNUSED(this_); /* compiler warning */',
GeoPositionInterpolator => 'UNUSED(this_); /* compiler warning */',
ScalarInterpolator => 'UNUSED(this_); /* compiler warning */',
OrientationInterpolator => 'UNUSED(this_); /* compiler warning */',
NormalInterpolator => 'UNUSED(this_); /* compiler warning */',
CoordinateInterpolator => 'UNUSED(this_); /* compiler warning */',
TimeSensor => 'UNUSED(this_); /* compiler warning */',
SphereSensor => 'UNUSED(this_); /* compiler warning */',
CylinderSensor =>'UNUSED(this_); /* compiler warning */',
GeoTouchSensor => 'UNUSED(this_); /* compiler warning */',
TouchSensor => 'UNUSED(this_); /* compiler warning */',
PlaneSensor => 'UNUSED(this_); /* compiler warning */',
VisibilitySensor => 'UNUSED(this_); /* compiler warning */',

GeoOrigin => 'render_GeoOrigin ((struct VRML_GeoOrigin *) this_);',

Viewpoint => '
	/* Viewpoint is in the PrepC side of things, as it is rendered before other nodes */
	if (!render_vp) return;
	render_Viewpoint ((struct VRML_Viewpoint*) this_);',

GeoViewpoint => '
	/* Viewpoint is in the PrepC side of things, as it is rendered before other nodes */
	if (!render_vp) return;
	render_GeoViewpoint ((struct VRML_GeoViewpoint*) this_);',

GeoLocation => '
	if (!render_vp) {
		glPushMatrix();
		render_GeoLocation ((struct VRML_GeoLocation*) this_);
	}',

Transform => '

	GLfloat my_rotation;
	GLfloat my_scaleO=0;
	int	recalculate_dist;

        /* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
         * so we do nothing here in that case -ncoder */

	 /* we recalculate distance on last pass, or close to it, and only
	 once per event-loop tick. we can do it on the last pass - the
	 render_sensitive pass, but when mouse is clicked (eg, moving in
	 examine mode, sensitive node code is not rendered. So, we choose
	 the second-last pass. ;-) */
	recalculate_dist = render_light;

	/* printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

	if(!render_vp) {
                /* glPushMatrix();*/
		fwXformPush(this_);

		/* might we have had a change to a previously ignored value? */
		if (this_->_change != this_->_dlchange) {
			/* printf ("re-rendering for %d\n",this_);*/
			this_->__do_center = verify_translate ((GLfloat *)this_->center.c);
			this_->__do_trans = verify_translate ((GLfloat *)this_->translation.c);
			this_->__do_scale = verify_scale ((GLfloat *)this_->scale.c);
			this_->__do_rotation = verify_rotate ((GLfloat *)this_->rotation.r);
			this_->__do_scaleO = verify_rotate ((GLfloat *)this_->scaleOrientation.r);
			this_->_dlchange = this_->_change;
		}



		/* TRANSLATION */
		if (this_->__do_trans)
			glTranslatef(this_->translation.c[0],this_->translation.c[1],this_->translation.c[2]);

		/* CENTER */
		if (this_->__do_center)
			glTranslatef(this_->center.c[0],this_->center.c[1],this_->center.c[2]);

		/* ROTATION */
		if (this_->__do_rotation) {
			my_rotation = this_->rotation.r[3]/3.1415926536*180;
			glRotatef(my_rotation,
				this_->rotation.r[0],this_->rotation.r[1],this_->rotation.r[2]);
		}

		/* SCALEORIENTATION */
		if (this_->__do_scaleO) {
			my_scaleO = this_->scaleOrientation.r[3]/3.1415926536*180;
			glRotatef(my_scaleO, this_->scaleOrientation.r[0],
				this_->scaleOrientation.r[1],this_->scaleOrientation.r[2]);
		}


		/* SCALE */
		if (this_->__do_scale)
			glScalef(this_->scale.c[0],this_->scale.c[1],this_->scale.c[2]);

		/* REVERSE SCALE ORIENTATION */
		if (this_->__do_scaleO)
			glRotatef(-my_scaleO, this_->scaleOrientation.r[0],
				this_->scaleOrientation.r[1],this_->scaleOrientation.r[2]);

		/* REVERSE CENTER */
		if (this_->__do_center)
			glTranslatef(-this_->center.c[0],-this_->center.c[1],-this_->center.c[2]);

		/* did either we or the Viewpoint move since last time? */
		if (recalculate_dist) {
			/* printf ("calling PointInView for %d\n",this_);*/
			this_->PIV = PointInView(this_);
			/* printf ("ppv %d\n",this_->PIV);*/

	       }
        }
',
Billboard => '
	struct pt vpos, ax, cp, cp2, arcp;
	static const struct pt orig = {0.0, 0.0, 0.0};
	static const struct pt zvec = {0.0, 0.0, 1.0};
	struct orient viewer_orient;
	GLdouble mod[16];
	GLdouble proj[16];

	int align;
	double len, len2, angle;
	int sign;
	ax.x = $f(axisOfRotation,0);
	ax.y = $f(axisOfRotation,1);
	ax.z = $f(axisOfRotation,2);
	align = (APPROX(VECSQ(ax),0));

	quaternion_to_vrmlrot(&(Viewer.Quat),
		&(viewer_orient.x), &(viewer_orient.y),
		&(viewer_orient.z), &(viewer_orient.a));

	glPushMatrix();

	fwGetDoublev(GL_MODELVIEW_MATRIX, mod);
	fwGetDoublev(GL_PROJECTION_MATRIX, proj);
	gluUnProject(orig.x, orig.y, orig.z, mod, proj,
		viewport, &vpos.x, &vpos.y, &vpos.z);

	len = VECSQ(vpos);
	if (APPROX(len, 0)) { return; }
	VECSCALE(vpos, 1/sqrt(len));

	if (align) {
		ax.x = viewer_orient.x;
		ax.y = viewer_orient.y;
		ax.z = viewer_orient.z;
	}

	VECCP(ax, zvec, arcp);
	len = VECSQ(arcp);
	if (APPROX(len, 0)) { return; }

	len = VECSQ(ax);
	if (APPROX(len, 0)) { return; }
	VECSCALE(ax, 1/sqrt(len));

	VECCP(vpos, ax, cp); /* cp is now 90deg to both vector and axis */
	len = sqrt(VECSQ(cp));
	if (APPROX(len, 0)) {
		glRotatef(-viewer_orient.a/3.1415926536*180, ax.x, ax.y, ax.z);
		return;
	}
	VECSCALE(cp, 1/len);

	/* Now, find out angle between this and z axis */
	VECCP(cp, zvec, cp2);

	len2 = VECPT(cp, zvec); /* cos(angle) */
	len = sqrt(VECSQ(cp2)); /* this is abs(sin(angle)) */

	/* Now we need to find the sign first */
	if (VECPT(cp, arcp) > 0) { sign = -1; } else { sign = 1; }
	angle = atan2(len2, sign*len);

	glRotatef(angle/3.1415926536*180, ax.x, ax.y, ax.z);
',

);

#######################################################################
#######################################################################
#######################################################################
#
# Fin --
#  Finish the rendering i.e. restore matrices and whatever to the
#  original state.
#
#

# Finish rendering
%FinC = (
GeoLocation => (join '','
	UNUSED(this_);
	if (!render_vp) glPopMatrix();
	'),

Transform => (join '','

	if(!render_vp) {
            /* glPopMatrix();*/
	    fwXformPop(this_);
	} else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if(found_vp) {
		glTranslatef(',(join ',',map {getf(Transform,center,$_)} 0..2),'
		);
		glRotatef(',getf(Transform,scaleOrientation,3),'/3.1415926536*180,',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glScalef(',(join ',',map {"1.0/(".getf(Transform,scale,$_).")"} 0..2),'
		);
		glRotatef(-(',getf(Transform,scaleOrientation,3),'/3.1415926536*180),',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glRotatef(-(',getf(Transform,rotation,3),')/3.1415926536*180,',
			(join ',',map {getf(Transform,rotation,$_)} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,center,$_).")"} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,translation,$_).")"}
			0..2),'
		);
            }
        }

'),
Billboard => (join '','
	UNUSED (this_);
	glPopMatrix();
'),
);

#######################################################################
#######################################################################
#######################################################################
#
# Child --
#  Render the actual children of the node.
#
#

# Render children (real child nodes, not e.g. appearance/geometry)
%ChildC = (
	Group => 'groupingChild(this_); ',
	Billboard => 'billboardChild(this_); ',
	Transform => 'transformChild(this_); ',
	Anchor => 'anchorChild(this_); ',
	GeoLocation => 'geolocationChild(this_); ',
	Inline => 'inlineChild(this_); ',
	InlineLoadControl => 'inlinelodChild (this_); ',
	Switch => '
		/* exceedingly simple - render only one child */
		int wc = $f(whichChoice);
		if(wc >= 0 && wc < $f_n(choice)) {
			void *p = $f(choice,wc);
			render_node(p);
		}
	',
	GeoLOD => 'UNUSED (this_);',
	LOD => 'lodChild(this_);',
	Collision => 'collisionChild(this_);',

	Appearance => '
		if($f(texture)) {
			/* we have to do a glPush, then restore, later */
			have_texture=TRUE;
			glPushAttrib(GL_ENABLE_BIT); 

			/* is there a TextureTransform? if no texture, fugutaboutit */
		    	if($f(textureTransform))   {
				render_node($f(textureTransform));
			}

			/* now, render the texture */
			render_node($f(texture));
#ifndef X3DMATERIALPROPERTY
		} else {
			last_texture_depth = 0;
			last_transparency = 1.0;
#endif
		}


		/* if we have a material, do it. last_texture_depth is used to select diffuseColor */
		if($f(material)) {
			render_node($f(material));
		} else {
			/* no material, so just colour the following shape */
                       	/* Spec says to disable lighting and set coloUr to 1,1,1 */
                       	glDisable (GL_LIGHTING);
			glColor3f(1.0,1.0,1.0);
			lightingOn = FALSE;
		}
	',
	Shape => '
		int trans;
		int should_rend;
		GLdouble modelMatrix[16];

		if(!(this_->geometry)) { return; }

		/* do we need to do some distance calculations? */
		if (((!render_vp) && render_light)) {
			fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
			this_->_dist = modelMatrix[14];
			/* printf ("getDist - recalculating distance, it is %f for %d\n",*/
			/* 	this_->_dist,this_);*/
		}

		if((render_collision) || (render_sensitive)) {
			/* only need to forward the call to the child */
			render_node((this_->geometry));
			return;
		}


		/* JAS - if not collision, and render_geom is not set, no need to go further */
		/* printf ("render_Shape vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
		/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

		/* a texture and a transparency flag... */
		last_bound_texture = 0;
		trans = have_transparency;
		have_textureTransform = FALSE;
		have_texture = FALSE;

                /* assume that lighting is enabled. Absence of Material or Appearance
                   node will turn lighting off; in this case, at the end of Shape, we
                   have to turn lighting back on again. */
                lightingOn = TRUE;


		/* is there an associated appearance node? */
       	        if($f(appearance)) {
                        render_node($f(appearance));
       	        } else {
                        /* no material, so just colour the following shape */
                       	/* Spec says to disable lighting and set coloUr to 1,1,1 */
                       	glDisable (GL_LIGHTING);
       	                glColor3f(1.0,1.0,1.0);
			lightingOn = FALSE;

			/* tell the rendering passes that this is just "normal" */
			last_texture_depth = 0;
			last_transparency = 1.0;
                }


		/* lets look at texture depth, and if it has alpha, call
		it a transparent node */
		if (last_texture_depth >3) {
			have_transparency++;
			if ((this_->_renderFlags & VF_Blend) != VF_Blend)
				update_renderFlag(this_,VF_Blend);
		}

		/* printf ("Shape, last_trans %d this trans %d last_texture_depth %d\n",*/
		/* 	have_transparency, trans, last_texture_depth);*/

		should_rend = FALSE;
		/* now, are we rendering blended nodes? */
		if (render_blend) {
			if (have_transparency!=trans) {
					should_rend = TRUE;
			}

		/* no, maybe we are rendering straight nodes? */
		} else {
			if (have_transparency == trans) {
					should_rend = TRUE;
			}
		}

		/* if (should_rend) {printf ("RENDERING THIS ONE\n");*/
		/* } else { printf ("NOT RENDERING THIS ONE\n");}*/

		/* should we render this node on this pass? */
		if (should_rend) {
			if (last_bound_texture != 0) {
				/* we had a texture */
				glEnable (GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D,last_bound_texture);
			}
			/* Now, do the geometry */
			render_node((this_->geometry));

			/* disable the texture */
			if (last_bound_texture != 0) {
				glDisable (GL_TEXTURE_2D);
			}
		}

		/* did we have a TextureTransform in the Appearance node? */
		if (have_textureTransform) {
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}

               /* did the lack of an Appearance or Material node turn lighting off? */
                if (!lightingOn) {
                        glEnable (GL_LIGHTING);
                }


		if (have_texture) glPopAttrib(); 
	',
);

#######################################################################
#######################################################################
#######################################################################
#
# Light --
#  Render a light. XXX This needs work to be like the spec :(
#
#

# NO startlist -- nextlight() may change :(
%LightC = (
	PointLight => '
		if($f(on)) {
			int light = nextlight();
			if(light >= 0) {
				float vec[4];
				glEnable(light);
				vec[0] = $f(direction,0);
				vec[1] = $f(direction,1);
				vec[2] = $f(direction,2);
				vec[3] = 1;
				glLightfv(light, GL_SPOT_DIRECTION, vec);
				vec[0] = $f(location,0);
				vec[1] = $f(location,1);
				vec[2] = $f(location,2);
				vec[3] = 1;
				glLightfv(light, GL_POSITION, vec);

				glLightf(light, GL_CONSTANT_ATTENUATION,
					$f(attenuation,0));
				glLightf(light, GL_LINEAR_ATTENUATION,
					$f(attenuation,1));
				glLightf(light, GL_QUADRATIC_ATTENUATION,
					$f(attenuation,2));


				vec[0] = $f(color,0) * $f(intensity);
				vec[1] = $f(color,1) * $f(intensity);
				vec[2] = $f(color,2) * $f(intensity);
				vec[3] = 1;
				glLightfv(light, GL_DIFFUSE, vec);
				glLightfv(light, GL_SPECULAR, vec);

				/* Aubrey Jaffer */
				vec[0] = $f(color,0) * $f(ambientIntensity);
				vec[1] = $f(color,1) * $f(ambientIntensity);
				vec[2] = $f(color,2) * $f(ambientIntensity);

				glLightfv(light, GL_AMBIENT, vec);

				/* XXX */
				glLightf(light, GL_SPOT_CUTOFF, 180);
			}
		}
	',
	SpotLight => '
		if($f(on)) {
			int light = nextlight();
			if(light >= 0) {
				float vec[4];
				glEnable(light);
				vec[0] = $f(direction,0);
				vec[1] = $f(direction,1);
				vec[2] = $f(direction,2);
				vec[3] = 1;
				glLightfv(light, GL_SPOT_DIRECTION, vec);
				vec[0] = $f(location,0);
				vec[1] = $f(location,1);
				vec[2] = $f(location,2);
				vec[3] = 1;
				glLightfv(light, GL_POSITION, vec);

				glLightf(light, GL_CONSTANT_ATTENUATION,
					$f(attenuation,0));
				glLightf(light, GL_LINEAR_ATTENUATION,
					$f(attenuation,1));
				glLightf(light, GL_QUADRATIC_ATTENUATION,
					$f(attenuation,2));


				vec[0] = $f(color,0) * $f(intensity);
				vec[1] = $f(color,1) * $f(intensity);
				vec[2] = $f(color,2) * $f(intensity);
				vec[3] = 1;
				glLightfv(light, GL_DIFFUSE, vec);
				glLightfv(light, GL_SPECULAR, vec);

				/* Aubrey Jaffer */
				vec[0] = $f(color,0) * $f(ambientIntensity);
				vec[1] = $f(color,1) * $f(ambientIntensity);
				vec[2] = $f(color,2) * $f(ambientIntensity);

				glLightfv(light, GL_AMBIENT, vec);

				/* XXX */
				glLightf(light, GL_SPOT_EXPONENT,
					0.5/($f(beamWidth)+0.1));
				glLightf(light, GL_SPOT_CUTOFF,
					$f(cutOffAngle)/3.1415926536*180);
			}
		}
	',
);

#######################################################################
#
# ExtraMem - extra members for the structures to hold
# 	cached info

%ExtraMem = (
	Group => 'int has_light; ',
);

$ExtraMem{Transform} = $ExtraMem{Group};
$ExtraMem{Billboard} = $ExtraMem{Group};
$ExtraMem{Anchor} = $ExtraMem{Group};
$ExtraMem{Collision} = $ExtraMem{Group};
$ExtraMem{GeoLocation} = $ExtraMem{Group};
$ExtraMem{Inline} = $ExtraMem{Group};
$ExtraMem{InlineLoadControl} = $ExtraMem{Group};


#######################################################################
#
# ChangedC - when the fields change, the following code is run before
# rendering for caching the data.
#
#

%ChangedC = (
	Group => '
		int i;
		int nc = $f_n(children);
		struct VRML_Box *p;
		struct VRML_Virt *v;

		$i(has_light) = 0;
		for(i=0; i<nc; i++) {
			p = (struct VRML_Box *)$f(children,i);
			v = *(struct VRML_Virt **)p;
			if (v->rend == DirectionalLight_Rend) {
				/*  printf ("group found a light\n");*/
				$i(has_light) ++;
			}
		}
	',
	Inline => '
		int i;
		int nc = $f_n(__children);
		struct VRML_Box *p;
		struct VRML_Virt *v;

		$i(has_light) = 0;
		for(i=0; i<nc; i++) {
			p = (struct VRML_Box *)$f(__children,i);
			v = *(struct VRML_Virt **)p;
			if (v->rend == DirectionalLight_Rend) {
				/*  printf ("group found a light\n");*/
				$i(has_light) ++;
			}
		}
	'
);


$ChangedC{Transform} = $ChangedC{Group};
$ChangedC{Billboard} = $ChangedC{Group};
$ChangedC{Anchor} = $ChangedC{Group};
$ChangedC{Collision} = $ChangedC{Group};
$ChangedC{GeoLocation} = $ChangedC{Group};
$ChangedC{InlineLoadControl} = $ChangedC{Group};


#######################################################################
#
# ProximityC = following code is run to let proximity sensors send their
# events. This is done in the rendering pass, because the position of
# of the object relative to the viewer is available via the
# modelview transformation matrix.
#

%ProximityC = (
ProximitySensor => q~
	/* Viewer pos = t_r2 */
	double cx,cy,cz;
	double len;
	struct pt dr1r2;
	struct pt dr2r3;
	struct pt nor1,nor2;
	struct pt ins;
	static const struct pt yvec = {0,0.05,0};
	static const struct pt zvec = {0,0,-0.05};
	static const struct pt zpvec = {0,0,0.05};
	static const struct pt orig = {0,0,0};
	struct pt t_zvec, t_yvec, t_orig;
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];

	if(!$f(enabled)) return;

	/* printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

	/* transforms viewers coordinate space into sensors coordinate space.
	 * this gives the orientation of the viewer relative to the sensor.
	 */
	fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(orig.x,orig.y,orig.z,modelMatrix,projMatrix,viewport,
		&t_orig.x,&t_orig.y,&t_orig.z);
	gluUnProject(zvec.x,zvec.y,zvec.z,modelMatrix,projMatrix,viewport,
		&t_zvec.x,&t_zvec.y,&t_zvec.z);
	gluUnProject(yvec.x,yvec.y,yvec.z,modelMatrix,projMatrix,viewport,
		&t_yvec.x,&t_yvec.y,&t_yvec.z);

	cx = t_orig.x - $f(center,0);
	cy = t_orig.y - $f(center,1);
	cz = t_orig.z - $f(center,2);

	if($f(size,0) == 0 || $f(size,1) == 0 || $f(size,2) == 0) return;

	if(fabs(cx) > $f(size,0)/2 ||
	   fabs(cy) > $f(size,1)/2 ||
	   fabs(cz) > $f(size,2)/2) return;

	/* Ok, we now have to compute... */
	$f(__hit) = 1;

	/* Position */
	$f(__t1,0) = t_orig.x;
	$f(__t1,1) = t_orig.y;
	$f(__t1,2) = t_orig.z;

	VECDIFF(t_zvec,t_orig,dr1r2);  /* Z axis */
	VECDIFF(t_yvec,t_orig,dr2r3);  /* Y axis */

	len = sqrt(VECSQ(dr1r2)); VECSCALE(dr1r2,1/len);
	len = sqrt(VECSQ(dr2r3)); VECSCALE(dr2r3,1/len);

	if(verbose) printf("PROX_INT: (%f %f %f) (%f %f %f) (%f %f %f)\n (%f %f %f) (%f %f %f)\n",
		t_orig.x, t_orig.y, t_orig.z,
		t_zvec.x, t_zvec.y, t_zvec.z,
		t_yvec.x, t_yvec.y, t_yvec.z,
		dr1r2.x, dr1r2.y, dr1r2.z,
		dr2r3.x, dr2r3.y, dr2r3.z
		);

	if(fabs(VECPT(dr1r2, dr2r3)) > 0.001) {
		printf ("Sorry, can't handle unevenly scaled ProximitySensors yet :("
		  "dp: %f v: (%f %f %f) (%f %f %f)\n", VECPT(dr1r2, dr2r3),
		  	dr1r2.x,dr1r2.y,dr1r2.z,
		  	dr2r3.x,dr2r3.y,dr2r3.z
			);
		return;
	}


	if(APPROX(dr1r2.z,1.0)) {
		/* rotation */
		$f(__t2,0) = 0;
		$f(__t2,1) = 0;
		$f(__t2,2) = 1;
		$f(__t2,3) = atan2(-dr2r3.x,dr2r3.y);
	} else if(APPROX(dr2r3.y,1.0)) {
		/* rotation */
		$f(__t2,0) = 0;
		$f(__t2,1) = 1;
		$f(__t2,2) = 0;
		$f(__t2,3) = atan2(dr1r2.x,dr1r2.z);
	} else {
		/* Get the normal vectors of the possible rotation planes */
		nor1 = dr1r2;
		nor1.z -= 1.0;
		nor2 = dr2r3;
		nor2.y -= 1.0;

		/* Now, the intersection of the planes, obviously cp */
		VECCP(nor1,nor2,ins);

		len = sqrt(VECSQ(ins)); VECSCALE(ins,1/len);

		/* the angle */
		VECCP(dr1r2,ins, nor1);
		VECCP(zpvec, ins, nor2);
		len = sqrt(VECSQ(nor1)); VECSCALE(nor1,1/len);
		len = sqrt(VECSQ(nor2)); VECSCALE(nor2,1/len);
		VECCP(nor1,nor2,ins);

		$f(__t2,3) = -atan2(sqrt(VECSQ(ins)), VECPT(nor1,nor2));

		/* rotation  - should normalize sometime... */
		$f(__t2,0) = ins.x;
		$f(__t2,1) = ins.y;
		$f(__t2,2) = ins.z;
	}
	if(verbose) printf("NORS: (%f %f %f) (%f %f %f) (%f %f %f)\n",
		nor1.x, nor1.y, nor1.z,
		nor2.x, nor2.y, nor2.z,
		ins.x, ins.y, ins.z
	);
~,


);

#######################################################################
#
# CollisionC = following code is run to do collision detection
#
# In collision nodes:
#    if enabled:
#       if no proxy:
#           passes rendering to its children
#       else (proxy)
#           passes rendering to its proxy
#    else
#       does nothing.
#
# In normal nodes:
#    uses gl modelview matrix to determine distance from viewer and
# angle from viewer. ...
#
#
#	       /* the shape of the avatar is a cylinder */
#	       /*                                           */
#	       /*           |                               */
#	       /*           |                               */
#	       /*           |--|                            */
#	       /*           | width                         */
#	       /*        ---|---       -                    */
#	       /*        |     |       |                    */
#	       /*    ----|() ()| - --- | ---- y=0           */
#	       /*        |  \  | |     |                    */
#	       /*     -  | \ / | |head | height             */
#	       /*    step|     | |     |                    */
#	       /*     -  |--|--| -     -                    */
#	       /*           |                               */
#	       /*           |                               */
#	       /*           x,z=0                           */


%CollisionC = (
Sphere => q~
	       struct pt t_orig; /*transformed origin*/
	       struct pt p_orig; /*projected transformed origin */
	       struct pt n_orig; /*normal(unit length) transformed origin */
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       GLdouble dist2;
	       struct pt delta = {0,0,0};
	       GLdouble radius;

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/

	       struct pt tupv = {0,1,0};

		/* are we initialized yet? */
		if (this_->__points==0) {
			return;
		}

	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       radius = pow(det3x3(modelMatrix),1./3.) * $f(radius);

	       /* squared distance to center of sphere (on the y plane)*/
	       dist2 = t_orig.x * t_orig.x + t_orig.z * t_orig.z;

	       /* easy tests. clip as if sphere was a box */
	       /*clip with cylinder */
	       if(dist2 - (radius + awidth) * (radius +awidth) > 0) {
		   return;
	       }
	       /*clip with bottom plane */
	       if(t_orig.y + radius < abottom) {
		   return;
	       }
	       /*clip with top plane */
	       if(t_orig.y-radius > atop) {
		   return;
	       }

	       /* project onto (y x t_orig) plane */
	       p_orig.x = sqrt(dist2);
	       p_orig.y = t_orig.y;
	       p_orig.z = 0;
	       /* we need this to unproject rapidly */
	       /* n_orig is t_orig.y projected on the y plane, then normalized. */
	       n_orig.x = t_orig.x;
	       n_orig.y = 0.0;
	       n_orig.z = t_orig.z;
	       VECSCALE(n_orig,1.0/p_orig.x); /*equivalent to vecnormal(n_orig);, but faster */

	       /* 5 cases : sphere is over, over side, side, under and side, under (relative to y axis) */
	       /* these 5 cases correspond to the 5 vornoi regions of the cylinder */
	       if(p_orig.y > atop) {

		   if(p_orig.x < awidth) {
		       if(verbose) printf(" /* over, we push down. */ \n");
		       delta.y = (p_orig.y - radius) - (atop);
		   } else {
		       struct pt d2s;
		       GLdouble ratio;
		       if(verbose) printf(" /* over side */ \n");

		       /* distance vector from corner to center of sphere*/
		       d2s.x = p_orig.x - awidth;
		       d2s.y = p_orig.y - (atop);
		       d2s.z = 0;

		       ratio = 1- radius/sqrt(d2s.x * d2s.x + d2s.y * d2s.y);

		       if(ratio >= 0) {
			   /* no collision */
			   return;
		       }

		       /* distance vector from corner to surface of sphere, (do the math) */
		       VECSCALE(d2s, ratio );

		       /* unproject, this is the fastest way */
		       delta.y = d2s.y;
		       delta.x = d2s.x* n_orig.x;
		       delta.z = d2s.x* n_orig.z;
		   }
	       } else if(p_orig.y < abottom) {
		   if(p_orig.x < awidth) {
		       if(verbose) printf(" /* under, we push up. */ \n");
		       delta.y = (p_orig.y + radius) -abottom;
		   } else {
		       struct pt d2s;
		       GLdouble ratio;
		       if(verbose) printf(" /* under side */ \n");

		       /* distance vector from corner to center of sphere*/
		       d2s.x = p_orig.x - awidth;
		       d2s.y = p_orig.y - abottom;
		       d2s.z = 0;

		       ratio = 1- radius/sqrt(d2s.x * d2s.x + d2s.y * d2s.y);

		       if(ratio >= 0) {
			   /* no collision */
			   return;
		       }

		       /* distance vector from corner to surface of sphere, (do the math) */
		       VECSCALE(d2s, ratio );

		       /* unproject, this is the fastest way */
		       delta.y = d2s.y;
		       delta.x = d2s.x* n_orig.x;
		       delta.z = d2s.x* n_orig.z;
		   }

	       } else {
		   if(verbose) printf(" /* side */ \n");

		   /* push to side */
		   delta.x = ((p_orig.x - radius)- awidth) * n_orig.x;
		   delta.z = ((p_orig.x - radius)- awidth) * n_orig.z;
	       }


	       transform3x3(&delta,&delta,upvecmat);
	       accumulate_disp(&CollisionInfo,delta);

	       if(verbose_collision && (delta.x != 0. || delta.y != 0. || delta.z != 0.))
	           printf("COLLISION_SPH: (%f %f %f) (%f %f %f) (px=%f nx=%f nz=%f)\n",
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z,
			  p_orig.x, n_orig.x, n_orig.z
			  );


	       ~,
Box => q~

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;

	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       struct pt iv = {0,0,0};
	       struct pt jv = {0,0,0};
	       struct pt kv = {0,0,0};
	       struct pt ov = {0,0,0};

	       struct pt t_orig = {0,0,0};
	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */

	       struct pt delta;
	       struct pt tupv = {0,1,0};

		iv.x = $f(size,0);
		jv.y = $f(size,1);
		kv.z = $f(size,2);
		ov.x = -$f(size,0)/2; ov.y = -$f(size,1)/2; ov.z = -$f(size,2)/2;


	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_box_intersect(abottom,atop,awidth,t_orig,scale*$f(size,0),scale*$f(size,1),scale*$f(size,2))) return;



	       /* get transformed box edges and position */
	       transform(&ov,&ov,modelMatrix);
	       transform3x3(&iv,&iv,modelMatrix);
	       transform3x3(&jv,&jv,modelMatrix);
	       transform3x3(&kv,&kv,modelMatrix);


	       delta = box_disp(abottom,atop,astep,awidth,ov,iv,jv,kv);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

	       if(verbose_collision && (fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_BOX: (%f %f %f) (%f %f %f)\n",
			  ov.x, ov.y, ov.z,
			  delta.x, delta.y, delta.z
			  );
	       if(verbose_collision && (fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) kv=(%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  kv.x, kv.y, kv.z
			  );


	       ~,

Cone => q~

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;

		float h = $f(height)/2;
		float r = $f(bottomRadius);

	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       struct pt iv = {0,0,0};
	       struct pt jv = {0,0,0};
	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */
	       struct pt t_orig = {0,0,0};

	       struct pt delta;
	       struct pt tupv = {0,1,0};

	       iv.y = h; jv.y = -h;

	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);

	       if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;

	       /* get transformed box edges and position */
	       transform(&iv,&iv,modelMatrix);
	       transform(&jv,&jv,modelMatrix);

	       delta = cone_disp(abottom,atop,astep,awidth,jv,iv,scale*r);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

	       if(verbose_collision && (fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_CON: (%f %f %f) (%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  delta.x, delta.y, delta.z
			  );
	       if(verbose_collision && (fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) bR=%f\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  scale*r
			  );


	       ~,

Cylinder => q~

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;

		float h = $f(height)/2;
		float r = $f(radius);

	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       struct pt iv = {0,0,0};
	       struct pt jv = {0,0,0};
	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */
	       struct pt t_orig = {0,0,0};

	       struct pt tupv = {0,1,0};
	       struct pt delta;


		iv.y = h;
		jv.y = -h;

	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;



	       /* get transformed box edges and position */
	       transform(&iv,&iv,modelMatrix);
	       transform(&jv,&jv,modelMatrix);


	       delta = cylinder_disp(abottom,atop,astep,awidth,jv,iv,scale*r);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

	       if(verbose_collision && (fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_CYL: (%f %f %f) (%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  delta.x, delta.y, delta.z
			  );
	       if(verbose_collision && (fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) bR=%f\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  scale*r
			  );


	       ~,


JASElevationGrid => q~
		collideIndexedFaceSet ((struct VRML_IndexedFaceSet *) this_);
~,
IndexedFaceSet => q~
		collideIndexedFaceSet ((struct VRML_IndexedFaceSet *) this_);
~,

IndexedTriangleFanSet => q~
		collideIndexedFaceSet ((struct VRML_IndexedFaceSet *) this_);
~,


IndexedTriangleSet => q~
		collideIndexedFaceSet ((struct VRML_IndexedFaceSet *) this_);
~,


IndexedTriangleStripSet => q~
		collideIndexedFaceSet ((struct VRML_IndexedFaceSet *) this_);
~,


TriangleFanSet => q~
		collideIndexedFaceSet ((struct VRML_IndexedFaceSet *) this_);
~,

TriangleStripSet => q~
		collideIndexedFaceSet ((struct VRML_IndexedFaceSet *) this_);
~,


TriangleSet => q~
		collideIndexedFaceSet ((struct VRML_IndexedFaceSet *) this_);
~,

Extrusion => q~


	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];

	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */
	       struct pt t_orig = {0,0,0};
	       static int refnum = 0;

	       struct pt tupv = {0,1,0};
	       struct pt delta = {0,0,0};

	       struct VRML_PolyRep pr;
	       prflags flags = 0;
	       int change = 0;

		/* JAS - first pass, intern is probably zero */
		if (((struct VRML_PolyRep *)this_->_intern) == 0) return;

		/* JAS - no triangles in this text structure */
		if ((((struct VRML_PolyRep *)this_->_intern)->ntri) == 0) return;


	       /*save changed state.*/
	       if(this_->_intern) change = ((struct VRML_PolyRep *)this_->_intern)->_change;
	       $mk_polyrep();
 	       if(this_->_intern) ((struct VRML_PolyRep *)this_->_intern)->_change = change;
	       /*restore changes state, invalidates mk_polyrep work done, so it can be done
	         correclty in the RENDER pass */

	       if(!$f(solid)) {
		   flags = flags | PR_DOUBLESIDED;
	       }
/*	       printf("_PolyRep = %d\n",this_->_intern);*/
	       pr = *((struct VRML_PolyRep*)this_->_intern);
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);
/*	       if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;*/

/*	       printf("ntri=%d\n",pr.ntri);
	       for(i = 0; i < pr.ntri; i++) {
		   printf("cindex[%d]=%d\n",i,pr.cindex[i]);
	       }*/
	       delta = polyrep_disp(abottom,atop,astep,awidth,pr,modelMatrix,flags);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

	       if(verbose_collision && (fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
/*		   printmatrix(modelMatrix);*/
		   fprintf(stderr,"COLLISION_EXT: ref%d (%f %f %f) (%f %f %f)\n",refnum++,
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );

	       }

~,

Text => q~
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];

	       struct pt t_orig = {0,0,0};
	       static int refnum = 0;

		/*JAS - normals are always this way - helps because some
			normal calculations failed because of very small triangles
			which made collision calcs fail, which moved the Viewpoint...
			so, if there is no need to calculate normals..., why do it? */
	       struct pt tupv = {0,1,0};
	       struct pt delta = {0,0,-1};
	       struct VRML_PolyRep pr;
	       int change = 0;


		/* JAS - first pass, intern is probably zero */
		if (((struct VRML_PolyRep *)this_->_intern) == 0) return;

		/* JAS - no triangles in this text structure */
		if ((((struct VRML_PolyRep *)this_->_intern)->ntri) == 0) return;


	       /*save changed state.*/
	       if(this_->_intern) change = ((struct VRML_PolyRep *)this_->_intern)->_change;
	       $mk_polyrep();
 	       if(this_->_intern) ((struct VRML_PolyRep *)this_->_intern)->_change = change;
	       /*restore changes state, invalidates mk_polyrep work done, so it can be done
	         correclty in the RENDER pass */

	       pr = *((struct VRML_PolyRep*)this_->_intern);
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
/*	       if(!fast_ycylinder_sphere_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return; must find data*/

	       delta = planar_polyrep_disp(abottom,atop,astep,awidth,pr,modelMatrix,PR_DOUBLESIDED,delta); /*delta used as zero*/

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

	       if(verbose_collision && (fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
		   fprintf(stderr,"COLLISION_TXT: ref%d (%f %f %f) (%f %f %f)\n",refnum++,
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );

	       }

~,

GeoElevationGrid => q~
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];

	       struct pt t_orig = {0,0,0};
	       static int refnum = 0;

	       struct pt tupv = {0,1,0};
	       struct pt delta = {0,0,0};

	       struct VRML_PolyRep pr;
	       prflags flags = 0;
	       int change = 0;
		STRLEN xx;

		float xSpacing = 0.0;	/* GeoElevationGrid uses strings here */
		float zSpacing = 0.0;	/* GeoElevationGrid uses strings here */
		sscanf (SvPV (this_->xSpacing,xx),"%f",&xSpacing);
		sscanf (SvPV(this_->zSpacing,xx),"%f",&zSpacing);

		/* JAS - first pass, intern is probably zero */
		if (((struct VRML_PolyRep *)this_->_intern) == 0) return;

		/* JAS - no triangles in this text structure */
		if ((((struct VRML_PolyRep *)this_->_intern)->ntri) == 0) return;


	       /*save changed state.*/
	       if(this_->_intern) change = ((struct VRML_PolyRep *)this_->_intern)->_change;
	       $mk_polyrep();
 	       if(this_->_intern) ((struct VRML_PolyRep *)this_->_intern)->_change = change;
	       /*restore changes state, invalidates mk_polyrep work done, so it can be done
	         correclty in the RENDER pass */

	       if(!$f(solid)) {
		   flags = flags | PR_DOUBLESIDED;
	       }
	       pr = *((struct VRML_PolyRep*)this_->_intern);
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
/*	       if(!fast_ycylinder_sphere_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return; must find data*/


	       delta = elevationgrid_disp(abottom,atop,awidth,astep,pr,$f(xDimension),$f(zDimension),xSpacing,zSpacing,
				modelMatrix,flags);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

	       if(verbose_collision && (fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
		   fprintf(stderr,"COLLISION_ELG: ref%d (%f %f %f) (%f %f %f)\n",refnum++,
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );

	       }
~,

ElevationGrid => q~
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];

	       struct pt t_orig = {0,0,0};
	       static int refnum = 0;

	       struct pt tupv = {0,1,0};
	       struct pt delta = {0,0,0};

	       struct VRML_PolyRep pr;
	       prflags flags = 0;
	       int change = 0;

		/* JAS - first pass, intern is probably zero */
		if (((struct VRML_PolyRep *)this_->_intern) == 0) return;

		/* JAS - no triangles in this text structure */
		if ((((struct VRML_PolyRep *)this_->_intern)->ntri) == 0) return;

	       /*save changed state.*/
	       if(this_->_intern) change = ((struct VRML_PolyRep *)this_->_intern)->_change;
	       $mk_polyrep();
 	       if(this_->_intern) ((struct VRML_PolyRep *)this_->_intern)->_change = change;
	       /*restore changes state, invalidates mk_polyrep work done, so it can be done
	         correclty in the RENDER pass */

	       if(!$f(solid)) {
		   flags = flags | PR_DOUBLESIDED;
	       }
	       pr = *((struct VRML_PolyRep*)this_->_intern);
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
/*	       if(!fast_ycylinder_sphere_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return; must find data*/


	       delta = elevationgrid_disp(abottom,atop,awidth,astep,pr,$f(xDimension),$f(zDimension),$f(xSpacing),$f(zSpacing),modelMatrix,flags);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

	       if(verbose_collision && (fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
		   fprintf(stderr,"COLLISION_ELG: ref%d (%f %f %f) (%f %f %f)\n",refnum++,
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );

	       }

~,

);



1;
