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
# Revision 1.48  2002/01/20 15:05:25  crc_canada
# if display lists change when in creation, don't do the glEnd for them
#
# Revision 1.47  2002/01/09 16:14:45  crc_canada
# removal of some debugging statements, and it no longer dies on some proximity
# sensor errors.
#
# Revision 1.46  2002/01/08 18:55:59  crc_canada
# standard text and fontstyle now works - not quite up to standard, though.
#
# Revision 1.45  2002/01/04 20:19:01  crc_canada
# FontStyle and Text node work
#
# Revision 1.44  2001/12/14 17:52:34  crc_canada
# ElevationGrid and IndexedFaceSet texture over colour problem resolved.
#
# Revision 1.43  2001/12/14 16:35:34  crc_canada
# TextureTransform bug fixed - texture transforms were not within the
# associated Display List for the Shape. Moved creating display list to
# before texture code in Shape node.
#
# Revision 1.42  2001/12/14 14:46:06  crc_canada
# Transparency issues fixed, this time with debug statements removed!
#
# Revision 1.41  2001/12/14 14:29:20  crc_canada
# Material transparency now handled by the GL_STIPPLE method. This appears
# to be how the browser in the NIST test renders transparency.
#
# Revision 1.40  2001/12/12 19:17:16  crc_canada
# LODS now spec compliant
#
# Revision 1.39  2001/08/16 16:56:25  crc_canada
# Viewpoint work
#
# Revision 1.38  2001/07/31 16:20:33  crc_canada
# more Background node work
#
# Revision 1.37  2001/07/05 16:04:33  crc_canada
# Initial IFS texture default code re-write
#
# Revision 1.36  2001/06/25 18:34:42  crc_canada
# ElevationGrid default textures now correct.
#
# Revision 1.35  2001/06/18 19:11:50  crc_canada
# Background ground angle bug if > 3 angles fixed.
#
# Revision 1.34  2001/06/18 17:24:32  crc_canada
# IRIX compile warnings removed.
#
# Revision 1.33  2001/06/15 19:32:18  crc_canada
# lighting disabled as per spec if no Material and/or no Appearance
#
# Revision 1.32  2001/06/01 15:37:36  crc_canada
# ProximitySensor now has correct axis for rotations when rotating about
# the X axis. This affects many things, particularly the HUD test (24.wrl)
#
# Revision 1.31  2001/05/16 18:04:17  crc_canada
# changes to allow compiling on Irix without errors (still warnings, though)
#
# Revision 1.30  2001/05/16 16:00:06  crc_canada
# Check for glError at the end of Shape node.
#
# Revision 1.29  2001/05/11 15:00:11  crc_canada
# Cone Normals now correct
#
# Revision 1.28  2001/05/04 19:51:32  crc_canada
# some extraneous debugging code removed.
#
# Revision 1.27  2001/05/03 20:24:08  crc_canada
# Proper use of Display lists and Textures for Shape nodes and below.
#
# Revision 1.26  2001/04/27 16:50:03  crc_canada
# Working display lists for Shape nodes
#
# Revision 1.25  2001/04/24 19:54:16  crc_canada
# Display list work
#
# Revision 1.24  2001/03/26 17:41:50  crc_canada
# Background rendering for some OpenGL implementations fixed.
# IndexedLineSet no longer disrupts other renderings.
#
# Revision 1.23  2001/03/23 16:00:02  crc_canada
# IndexedLineSet culling disabled - it was affecting other shapes in the scene graph.
#
# Revision 1.22  2001/03/13 16:19:57  crc_canada
# Pre 0.28 checkin
#
# Revision 1.20  2000/12/20 17:27:52  crc_canada
# more IndexedFaceSet work - normals this time.
#
# Revision 1.19  2000/12/18 21:19:59  crc_canada
# IndexedFaceSet colorPerVertex and colorIndex now working correctly
#
# Revision 1.18  2000/12/08 13:10:39  crc_canada
# Two fixes to Background nodes:
# 	- possible core dump if only one sky angle and no ground angles.
# 	- make background sphere a lot larger.
#
# Revision 1.17  2000/12/07 19:12:25  crc_canada
# code cleanup, and IndexedFaceSet texture mapping
#
# Revision 1.16  2000/11/16 18:46:02  crc_canada
# Nothing much - just comments and verbose code.
#
# Revision 1.15  2000/11/15 16:37:01  crc_canada
# Removed some extra opengl calls from box.
#
# Revision 1.14  2000/11/03 21:52:16  crc_canada
# Speed updates - texture objects added
#
# Revision 1.13  2000/10/28 17:42:51  crc_canada
# EAI addchildren, etc, should be ok now.
#
# Revision 1.12  2000/09/03 20:17:50  rcoscali
# Made some test for blending
# Tests are displayed with 38 & 39.wrlð
#
# Revision 1.11  2000/08/31 23:00:00  rcoscali
# Add depth 2 support (2 channels/color components) which isMINANCE_ALPHAre (wi
#
# Revision 1.10  2000/08/30 00:04:18  rcoscali
# Comment out a glDisable(GL_LIGHTING) (uncommented by mistake ??)
# Fixed a problem in cone rendering (order of the vertexes)i
#
# Revision 1.9  2000/08/07 01:24:45  rcoscali
# Removed a glShadeModel(mooth) forgotten after a test
#
# Revision 1.8  2000/08/07 01:03:09  rcoscali
# Fixed another little problem on texture of cone. It was mapped in the wrong way.
#
# Revision 1.7  2000/08/07 00:28:44  rcoscali
# Removed debug macros and traces for sphere
#
# Revision 1.6  2000/08/06 22:55:17  rcoscali
# Ok ! All is like it should be for 0.26
# I tag the release as A_0_26
#
# Revision 1.5  2000/08/06 20:44:46  rcoscali
# Cone OK ... Now doing sphere ...
#
# Revision 1.4  2000/08/06 20:14:41  rcoscali
# Box Ok too. Now doing cone ...
#
# Revision 1.3  2000/08/06 19:48:37  rcoscali
# Fixed Cylinder. Now attacking cone.
#
# Revision 1.2  2000/08/04 22:54:41  rcoscali
# Add a cvs header, mainly for test
#
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
# XXX Tex coords wrong :(
Box => (join '',
	'
	 float x = $f(size,0)/2;
	 float y = $f(size,1)/2;
	 float z = $f(size,2)/2;

	/* for shape display list redrawing */
	this_->_myshape = last_visited_shape; 

	 glPushAttrib(GL_LIGHTING);
	 glShadeModel(GL_FLAT);
	 glBegin(GL_QUADS);

		/* front side */
		glNormal3f(0,0,1);
		glTexCoord2f(1,1);
		glVertex3f(x,y,z);
		glTexCoord2f(0,1);
		glVertex3f(-x,y,z);
		glTexCoord2f(0,0);
		glVertex3f(-x,-y,z);
		glTexCoord2f(1,0);
		glVertex3f(x,-y,z);

		/* back side */
		glNormal3f(0,0,-1);
		glTexCoord2f(0,0);
		glVertex3f(x,-y,-z);
		glTexCoord2f(1,0);
		glVertex3f(-x,-y,-z);
		glTexCoord2f(1,1);
		glVertex3f(-x,y,-z);
		glTexCoord2f(0,1);
		glVertex3f(x,y,-z);

		/* top side */
		glNormal3f(0,1,0);
		glTexCoord2f(0,0);
		glVertex3f(-x,y,z);
		glTexCoord2f(1,0);
		glVertex3f(x,y,z);
		glTexCoord2f(1,1);
		glVertex3f(x,y,-z);
		glTexCoord2f(0,1);
		glVertex3f(-x,y,-z);

		/* down side */
		glNormal3f(0,-1,0);
		glTexCoord2f(0,0);
		glVertex3f(-x,-y,-z);
		glTexCoord2f(1,0);
		glVertex3f(x,-y,-z);
		glTexCoord2f(1,1);
		glVertex3f(x,-y,z);
		glTexCoord2f(0,1);
		glVertex3f(-x,-y,z);

		/* right side */
		glNormal3f(1,0,0);
		glTexCoord2f(0,0);
		glVertex3f(x,-y,z);
		glTexCoord2f(1,0);
		glVertex3f(x,-y,-z);
		glTexCoord2f(1,1);
		glVertex3f(x,y,-z);
		glTexCoord2f(0,1);
		glVertex3f(x,y,z);

		/* left side */
		glNormal3f(-1,0,0);
		glTexCoord2f(1,0);
		glVertex3f(-x,-y,z);
		glTexCoord2f(1,1);
		glVertex3f(-x,y,z);
		glTexCoord2f(0,1);
		glVertex3f(-x,y,-z);
		glTexCoord2f(0,0);
		glVertex3f(-x,-y,-z);
		glEnd();
				glDepthMask(GL_TRUE);
	glPopAttrib();
	',
),


Cylinder => '
		int div = horiz_div;
		float df = div;
		float h = $f(height)/2;
		float r = $f(radius);
		float a,a1,a2;
		DECL_TRIG1
		int i = 0;
		INIT_TRIG1(div)
        
		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape;

		if($f(top)) {
			            /*	printf ("Cylinder : top\n"); */
			glBegin(GL_POLYGON);
			glNormal3f(0,1,0);
			START_TRIG1
			for(i=0; i<div; i++) {
				glTexCoord2f( 0.5 - 0.5*SIN1, 0.5 - 0.5*COS1);
				glVertex3f( -r*SIN1, (float)h, r*COS1 );
				UP_TRIG1
			}
			glEnd();
		} else {
			/* printf ("Cylinder : NO top\n"); */
		}

		if($f(bottom)) {
            		/* printf ("Cylinder : bottom\n"); */
			glBegin(GL_POLYGON);
			glNormal3f(0,-1,0);
			START_TRIG1
			for(i=0; i<div; i++) {
				glTexCoord2f(0.5+0.5*SIN1,0.5+0.5*COS1);
				glVertex3f(r*SIN1,(float)-h,r*COS1);
				UP_TRIG1
			}
			glEnd();
		} else {
			/* printf ("Cylinder : NO bottom\n"); */
		} 

		if($f(side)) {
				/* if(!nomode) {
				glPushAttrib(GL_LIGHTING);
				# glShadeModel(GL_SMOOTH);
				} */
			glBegin(GL_QUADS);
			START_TRIG1
			for(i=0; i<div; i++) {
				float lsin = SIN1;
				float lcos = COS1;
				UP_TRIG1;

				glNormal3f(lsin, 0.0, lcos);
 				glTexCoord2f(1.0-((float)i/df), 1.0);
				 glVertex3f((float)r*lsin, (float)h, (float)r*lcos);

				glNormal3f(SIN1, 0.0, COS1);
 				glTexCoord2f(1.0-(((float)i+1.0)/df), 1.0);
				glVertex3f(r*SIN1,  (float)h, r*COS1);

				/* glNormal3f(SIN1, 0.0, COS1); (same) */
				glTexCoord2f(1.0-(((float)i+1.0)/df), 0.0);
				glVertex3f(r*SIN1, (float)-h, r*COS1);

				glNormal3f(lsin, 0.0, lcos);
				glTexCoord2f(1.0-((float)i/df), 0.0);
				glVertex3f((float)r*lsin, (float)-h, (float)r*lcos);


			}
			glEnd();
				/*
				if(!nomode) {
				glPopAttrib();
				}
				*/
		}
',


Cone => '
	/*==================================================================
	May 10th 2001, Alain Gagnon.
	This block of code was fixed from the original because of normals.  
	 y axis
	 ^
	 |                                           
	 |       /|\             /|                     
	 |      / | \          /  |                   
	 |     /  |  \ml     /hyp |                  
	 |    / hreal \    /      |htwo
	 |   /    |   T\ /G        |
	 |   -----+----- ---------+    
	 |
	 |  |__r__|         rtwo
	 +--------------------------->x axis
	 |
	To understand these variables is to understand this code.
		r     : the radius of the cone bottom 
		hreal : the actual height of the cone
		h     : hreal/2
	        ml    : the hypothenuse on the cone side
	 
		hyp   : a unit vector (length = 1) which is perpendicular to
			ml.  Is is a normal for a given triangle making up 
			the cone side.
		htwo  : the height of the hyp vector
		rtwo  : a radius tha yields the start point of each normal
			vector.
		div   : the number of triangles that make up the cone surface
		d_div : div * 2

		theta : the angle located at T. 
		gamma : the angle located at G. 
	===================================================================*/

		int div = horiz_div;
		int d_div = div * 2;
		float df = div;
		float h = $f(height)/2;
		float r = $f(bottomRadius); 
		float a,a1;
		int i;
		DECL_TRIG1


		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape;

		if(h <= 0 && r <= 0) {return;}

		/* The angular distance of each step in the rotations that create  */
		/* the bottom and the sides of the cone is halved.                 */
		/* This permits the correct direction of the normals at the top of */
		/* the cone.  Otherwise, the normals would be slighlty off.        */
		INIT_TRIG1(d_div)

		if($f(bottom)) {
            		/* printf ("Cone : bottom\n"); */
			glBegin(GL_POLYGON);
			glNormal3f(0,-1,0);
			START_TRIG1
			for(i=0; i<d_div; i++) {
				if (!(i % 2))
				{
					glTexCoord2f(0.5+0.5*SIN1,0.5+0.5*COS1);
					glVertex3f(r*SIN1,(float)-h,r*COS1);
				}
				UP_TRIG1
			}
			glEnd();
		} else {
			/* printf ("Cone : NO bottom\n"); */
		} 

		if($f(side)) {
			double hreal = $f(height);
			double ml = sqrt(hreal*hreal + r * r);
			double mlh = h / ml;
			double mlr = r / ml;
			
			/*This code is a bug fix for the normals*/

			/* use atan(hreal/r) to get the angle for the bottom corner */
			double theta = atan(hreal / r);    

			/* calculate the vertical angle for the normal vector*/
                        double gamma = ( PI/2 ) - theta;  

			/* find the dimensions */
			double htwo  =  sin(gamma);
			double rtwo  =  cos(gamma);
                        
			glBegin(GL_TRIANGLES);
			START_TRIG1

			for(i=0; i<div; i++) {
				float lsin = SIN1;
				float lcos = COS1;

				/* perform an angular displacement */	
				UP_TRIG1;  

				/* place the top point and normal */
				glNormal3f(rtwo*SIN1, htwo, rtwo*COS1);
				glTexCoord2f(1.0-((i+0.5)/df), 1.0);
				glVertex3f(0.0, (float)h, 0.0);

				/* perform another angular displacement */
				UP_TRIG1;

				/* place the bottom points and normals */
				glNormal3f(rtwo*SIN1, htwo, rtwo*COS1);
				glTexCoord2f(1.0-((i+1.0)/df), 0.0);
				glVertex3f(r*SIN1, (float)-h, r*COS1);

				glNormal3f(rtwo*lsin, htwo, rtwo*lcos);
				glTexCoord2f(1.0-((float)i/df), 0.0);
				glVertex3f(r*lsin, (float)-h, r*lcos);

			}
			glEnd();
		}
',
Sphere => 'int vdiv = vert_div;
		int hdiv = horiz_div;
	   	float vf = vert_div;
	   	float hf = horiz_div;
		int v; int h;
		float va1,va2,van,ha1,ha2,han;
		DECL_TRIG1
		DECL_TRIG2

		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape; 

		INIT_TRIG1(vdiv) 
		INIT_TRIG2(hdiv)
		glPushMatrix();
        if (0) { /* Just tweaking around */
		       /* glPushAttrib(GL_LIGHTING); This does what? */
			   /* glShadeModel(GL_SMOOTH); This makes a smoother shading */
		}
		glScalef($f(radius), $f(radius), $f(radius));
		glBegin(GL_QUAD_STRIP);
		START_TRIG1
		for(v=0; v<vdiv; v++) {
			float vsin1 = SIN1;
			float vcos1 = COS1, vsin2,vcos2;
			UP_TRIG1
			vsin2 = SIN1;
			vcos2 = COS1;
			START_TRIG2
			for(h=0; h<=hdiv; h++) {
				float hsin1 = SIN2;
				float hcos1 = COS2;
				UP_TRIG2
/* 
 * Round a tex coord just barely greater than 1 to 1 : Since we are 
 * counting modulo 1, we do not want 1 to become zero spuriously.
 */
/* We really want something more rigorous here */
#define MY_EPS 0.000001
#define MOD_1(x) ( (x)<=1 ? (x) : ((x)<=(1.0+MY_EPS))? 1.0 : (x)-1.0 )

				glNormal3f(vsin2 * hcos1, vcos2, vsin2 * hsin1);
				glTexCoord2f(MOD_1(h / hf), 2.0 * ((v + 1.0) / vf));
				glVertex3f(vsin2 * hcos1, vcos2, vsin2 * hsin1);

				glNormal3f(vsin1 * hcos1, vcos1, vsin1 * hsin1); 
				glTexCoord2f(MOD_1(h / hf), 2.0 * (v/vf));
				glVertex3f(vsin1 * hcos1, vcos1, vsin1 * hsin1); 
#undef MOD_1
#undef MY_EPS
			}
		}
		glEnd();
		glPopMatrix();
					/* if(!$nomode) {
						glPopAttrib();
					} */
',

IndexedFaceSet =>  ( join '',
		'
		struct SFColor *points; int npoints;
		struct SFColor *colors; int ncolors=0;
		struct SFColor *normals; int nnormals=0;
		struct SFVec2f *texcoords; int ntexcoords=0;

		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape; 

		/* get "coord", "color", "normal", "texCoord", "colorIndex" */
		$fv(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);

		$mk_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);

		}
		render_polyrep(this_, 
			npoints, points,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords);
		if(!$f(solid)) {
			glPopAttrib();
		}
'),

# XXX emissiveColor not taken :(

# XXX Coredump possible for stupid input.

IndexedLineSet => '
		int i;
		int cin = $f_n(coordIndex);
		int colin = $f_n(colorIndex);
		int cpv = $f(colorPerVertex);
		int plno = 0;
		int ind1,ind2;
		int ind;
		int c;
		struct SFColor *points; int npoints;
		struct SFColor *colors; int ncolors=0;


		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape; 

		if(verbose) printf("Line: cin %d colin %d cpv %d\n",cin,colin,cpv);
		$fv(coord, points, get3, &npoints);
		$fv_null(color, colors, get3, &ncolors);
		glEnable(GL_COLOR_MATERIAL); 
                glPushAttrib(GL_ENABLE_BIT);
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
					      glColor3f(colors[c].c[0],
					        colors[c].c[1],
					   	colors[c].c[2]);
					}
				}
				glBegin(GL_LINE_STRIP);
			} else {
				if(ncolors && cpv) {
					c = i;
					if(colin) {
						c = $f(colorIndex,c);
					}
					glColor3f(colors[c].c[0],
						  colors[c].c[1],
						  colors[c].c[2]);
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
	int i; 
	struct SFColor *points; int npoints=0;
	struct SFColor *colors; int ncolors=0;

		/* for shape display list redrawing */
	this_->_myshape = last_visited_shape; 

	$fv(coord, points, get3, &npoints);
	$fv_null(color, colors, get3, &ncolors);
	if(ncolors && ncolors != npoints) {
		die("Not same number of colors and points");
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

ElevationGrid => ( '
		struct SFColor *colors; int ncolors=0;
                struct SFVec2f *texcoords; int ntexcoords=0;
		struct SFColor *normals; int nnormals=0;

		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape; 

		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
		$fv_null(texCoord, texcoords, get2, &ntexcoords);

		$mk_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_, 
			0, NULL,
			ncolors, colors,
			nnormals, normals,
			ntexcoords, texcoords
		);
		if(!$f(solid)) {
			glPopAttrib();
		}
'),

Extrusion => ( '

		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape; 

		$mk_polyrep();
		if(!$f(solid)) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_, 
			0, NULL,
			0, NULL,
			0, NULL,
			0, NULL
		);
		if(!$f(solid)) {
			glPopAttrib();
		}
'),

# FontStyle params handled in Text.
FontStyle => '',


Text => '
	void (*f)(int n, SV **p,int nl, float *l, float maxext, float spacing, float size, unsigned int fsparams);
        float spacing = 1.0;
        float size = 0;
	unsigned int fsparams = 0;

	/* for shape display list redrawing */
	this_->_myshape = last_visited_shape; 

	/* We need both sides */
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);
	f = (void *)$f(__rendersub);

	if($f(fontStyle)) {
		/* We have a FontStyle. Parse params (except size and spacing) and
		   make up an unsigned int with bits indicating params, to be
		   passed to the Text Renderer 
	
			bit:	0	horizontal  (boolean)
			bit:	1	leftToRight (boolean)
			bit:	2	topToBottom (boolean)
			  (style)
			bit:	3	BOLD	    (boolean)
			bit:	4	ITALIC	    (boolean)
			  (family)
			bit:	5	SERIF
			bit:	6	SANS	
			bit:	7	TYPEWRITER
			bit:	8 	indicates exact font pointer (future use)
			  (Justify - major)
			bit:	9	FIRST
			bit:	10	BEGIN
			bit:	11	MIDDLE
			bit:	12	END
			  (Justify - minor)
			bit:	13	FIRST
			bit:	14	BEGIN
			bit:	15	MIDDLE
			bit:	16	END

			bit: 17-31	spare
		*/

		struct VRML_FontStyle *fsp = $f(fontStyle);
		unsigned char *lang = SvPV((fsp->language),PL_na);
		unsigned char *style = SvPV((fsp->style),PL_na);
		struct Multi_String family = fsp->family;	 
		struct Multi_String justify = fsp->justify;
		int tmp; int tx;
		SV **svptr;
		unsigned char *stmp;


		/* Step 1 - record the spacing and size, for direct use */
		spacing = fsp->spacing;
		size = fsp->size;

		/* Step 2 - do the SFBools */
        	fsparams = (fsp->horizontal)|(fsp->leftToRight<<1)|(fsp->topToBottom<<2); 


		/* Step 3 - the SFStrings - style and language */
		/* actually, language is not parsed yet */
	
		if (strlen(style)) {
			if (!strcmp(style,"ITALIC")) {fsparams |= 0x10;}
			else if(!strcmp(style,"BOLD")) {fsparams |= 0x08;}
			else if (!strcmp(style,"BOLDITALIC")) {fsparams |= 0x18;}
			else if (strcmp(style,"PLAIN")) {
				printf ("Warning - FontStyle style %s  assuming PLAIN\n",style);}
		}
		if (strlen(lang)) {printf ("Warning - FontStyle - language param unparsed\n");}


		/* Step 4 - the MFStrings now. Family, Justify. */
		/* family can be blank, or one of the pre-defined ones. Any number of elements */

		svptr = family.p;
		for (tmp = 0; tmp < family.n; tmp++) {
			stmp = SvPV(svptr[tmp],PL_na);
			if (strlen(stmp) == 0) {fsparams |=0x20; } 
			else if (!strcmp(stmp,"SERIF")) { fsparams |= 0x20;}
			else if(!strcmp(stmp,"SANS")) { fsparams |= 0x40;}
			else if (!strcmp(stmp,"TYPEWRITER")) { fsparams |= 0x80;}
			else { printf ("Warning - FontStyle family %s unknown\n",stmp);}
		}

		svptr = justify.p;
		tx = justify.n;
		/* default is "BEGIN" "FIRST" */
		if (tx == 0) { fsparams |= 0x2400; }
		else if (tx == 1) { fsparams |= 0x2000; }
		else if (tx > 2) {
			printf ("Warning - FontStyle, max 2 elements in Justify\n");
			tx = 2;
		}
		
		for (tmp = 0; tmp < tx; tmp++) {
			stmp = SvPV(svptr[tmp],PL_na);
			if (strlen(stmp) == 0) {
				if (tmp == 0) {fsparams |= 0x400;
				} else {fsparams |= 0x2000;
				}
			} 
			else if (!strcmp(stmp,"FIRST")) { fsparams |= (0x200<<(tmp*4));}
			else if(!strcmp(stmp,"BEGIN")) { fsparams |= (0x400<<(tmp*4));}
			else if (!strcmp(stmp,"MIDDLE")) { fsparams |= (0x800<<(tmp*4));}
			else if (!strcmp(stmp,"END")) { fsparams |= (0x1000<<(tmp*4));}
			else { printf ("Warning - FontStyle family %s unknown\n",stmp);}
		}
	}
	if(f) {
		f($f_n(string),$f(string),$f_n(length),$f(length),
			$f(maxExtent),spacing,size,fsparams);
	}
	glPopAttrib();
',


Material => ( join '',
	"	float m[4]; int i; 
		GLubyte quartertone[] = {
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
		      0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22
		   };
		GLubyte halftone[] = {
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55
		   };
		GLubyte threequartertone[] = {
		      0xF7, 0xF7, 0xF7, 0xF7, 0xBF, 0xBF, 0xBF, 0xBF,
		      0xFB, 0xFB, 0xFB, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF,
		      0xEF, 0xEF, 0xEF, 0xEF, 0xFE, 0xFE, 0xFE, 0xFE,
		      0xFD, 0xFD, 0xFD, 0xFD, 0xBF, 0xBF, 0xBF, 0xBF,
		      0xF7, 0xF7, 0xF7, 0xF7, 0xBF, 0xBF, 0xBF, 0xBF,
		      0xFB, 0xFB, 0xFB, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF,
		      0xEF, 0xEF, 0xEF, 0xEF, 0xFE, 0xFE, 0xFE, 0xFE,
		      0xFD, 0xFD, 0xFD, 0xFD, 0xBF, 0xBF, 0xBF, 0xBF,
		      0xF7, 0xF7, 0xF7, 0xF7, 0xBF, 0xBF, 0xBF, 0xBF,
		      0xFB, 0xFB, 0xFB, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF,
		      0xEF, 0xEF, 0xEF, 0xEF, 0xFE, 0xFE, 0xFE, 0xFE,
		      0xFD, 0xFD, 0xFD, 0xFD, 0xBF, 0xBF, 0xBF, 0xBF,
		      0xF7, 0xF7, 0xF7, 0xF7, 0xBF, 0xBF, 0xBF, 0xBF,
		      0xFB, 0xFB, 0xFB, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF,
		      0xEF, 0xEF, 0xEF, 0xEF, 0xFE, 0xFE, 0xFE, 0xFE,
		      0xFD, 0xFD, 0xFD, 0xFD, 0xBF, 0xBF, 0xBF, 0xBF
		   };
		",assgn_m(diffuseColor,1),";

		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape; 

		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m);
		for(i=0; i<3; i++) {
			m[i] *= ", getf(Material, ambientIntensity),";
		}
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m);
		",assgn_m(specularColor,1),";
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m);

		",assgn_m(emissiveColor,1),';
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, m);

		glColor3f(m[0],m[1],m[2]);

		if (fabs($f(transparency)) > 0.01) {
			glEnable(GL_POLYGON_STIPPLE);
			if (fabs($f(transparency)) < 0.26) {
				glPolygonStipple (threequartertone);
			} else if (fabs($f(transparency)) < 0.51) {
				glPolygonStipple (halftone);
			} else { glPolygonStipple (quartertone);}
		}

		if(fabs($f(shininess) - 0.2) > 0.001) {
			/* printf("Set shininess: %f\n",$f(shininess)); */
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 
				128 * $f(shininess));
				/* 128*$f(shininess)*$f(shininess)); */
				/* 128-(128*$f(shininess))); */
				/* 1.0/((",getf(Material,shininess),"+1)/128.0)); */
		}
'),

TextureTransform => '


		/* for shape display list redrawing */
	this_->_myshape = last_visited_shape; 

       	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef($f(translation,0), $f(translation,1), 0);
	glTranslatef(-$f(center,0),-$f(center,1), 0);
	glRotatef($f(rotation)/3.1415926536*180,0,0,1);
	glScalef($f(scale,0),$f(scale,1),1);
	glTranslatef($f(center,0),$f(center,1), 0);
	glMatrixMode(GL_MODELVIEW);
',

ImageTexture => ('
	unsigned char *ptr = SvPV((this_->__data),PL_na);

		/* for shape display list redrawing */
	this_->_myshape = last_visited_shape; 

	if(!this_->_texture) {
		glGenTextures(1,&this_->_texture);
	}

	/* save the reference globally */
	last_bound_texture = this_->_texture;

	glBindTexture (GL_TEXTURE_2D, this_->_texture);
	do_texture ((this_->__depth), (this_->__x), (this_->__y), ptr,
		((this_->repeatS)) ? GL_REPEAT : GL_CLAMP, 
		((this_->repeatT)) ? GL_REPEAT : GL_CLAMP,
		GL_LINEAR);
'),

PixelTexture => ('

	unsigned char *ptr = SvPV((this_->__data),PL_na);

		/* for shape display list redrawing */
	this_->_myshape = last_visited_shape; 

	if(!this_->_texture) {
		glGenTextures(1,&this_->_texture);
	}

	/* save the reference globally */
	last_bound_texture = this_->_texture;

	glBindTexture (GL_TEXTURE_2D, this_->_texture);
	do_texture ((this_->__depth), (this_->__x), (this_->__y), ptr,
		((this_->repeatS)) ? GL_REPEAT : GL_CLAMP, 
		((this_->repeatT)) ? GL_REPEAT : GL_CLAMP,
		GL_NEAREST);
'),
     

# GLBackend is using 200000 as distance - we use 100000 for background
# XXX Should just make depth test always fail.
Background => '
	#define BACKTEX		0
	#define FRONTTEX	1
	#define LEFTTEX		2
	#define RIGHTTEX	3
	#define TOPTEX		4
	#define BOTTEX		5

	GLdouble mod[16];
	GLdouble proj[16];
	GLdouble unit[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	struct pt vec[4]; struct pt vec2[4]; struct pt vec3[4];
	int i,j; int ind=0;
	GLdouble x,y,z;
	GLdouble x1,y1,z1;
	GLdouble sx, sy, sz;
	struct SFColor *c1,*c2;
	int hdiv = horiz_div;
	int h,v;
	double va1, va2, ha1, ha2;	/* JS - vert and horiz angles 	*/
	double vatemp;		
	GLuint mask;
	GLfloat bk_emis[4];		/* background emissive colour	*/
	float	sc;

	/* only do background lighting, etc, once for textures */
	/*
	do_texture(int depth,int x,int y,unsigned char * ptr, 
        	GLint Sgl_rep_or_clamp, GLint Tgl_rep_or_clamp,GLint Image);
	*/

	/* Background Texture Objects.... */
	static int bcklen,frtlen,rtlen,lftlen,toplen,botlen;
	unsigned char *bckptr,*frtptr,*rtptr,*lftptr,*topptr,*botptr;
	static GLuint BackTextures[6];


	static unsigned int displayed_node = 0;
	static int background_display_list = -1;
	static GLfloat light_ambient[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_diffuse[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_specular[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};

	if(!((this_->isBound))) {return;}

	bk_emis[3]=0.0; /* always zero for backgrounds */

	/* Cannot start_list() because of moving center, so we do our own list later */

	glPushAttrib(GL_LIGHTING_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT);
	glShadeModel(GL_SMOOTH);
	glPushMatrix();
	glGetDoublev(GL_MODELVIEW_MATRIX, mod);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	/* Get origin */
	gluUnProject(0,0,0,mod,proj,viewport,&x,&y,&z);
	glTranslatef(x,y,z);

	glDisable (GL_LIGHTING);

	gluUnProject(0,0,0,mod,unit,viewport,&x,&y,&z);
	/* Get scale */
	gluProject(x+1,y,z,mod,unit,viewport,&x1,&y1,&z1);
	sx = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	gluProject(x,y+1,z,mod,unit,viewport,&x1,&y1,&z1);
	sy = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	gluProject(x,y,z+1,mod,unit,viewport,&x1,&y1,&z1);
	sz = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );

	/* Undo the translation and scale effects */
	glScalef(sx,sy,sz);


	/* now, is this the same background as before??? */
	if(displayed_node==(unsigned int) nod_) {
		glCallList(background_display_list);
		glPopMatrix();
		glPopAttrib();
		return;
	}

	/* if not, then generate a display list */
	displayed_node = (unsigned int) nod_;
	if (background_display_list != -1) {
		/* delete old list, to make room for new one */
		/* printf ("deleting old background display list %d\n",background_display_list); */
		glDeleteLists(background_display_list,1);
	}
	background_display_list = glGenLists(1);

	/* do we have any background textures? if so, bind them here, before
	   the display list is started */
	frtptr = SvPV((this_->__data_front),frtlen); 
	bckptr = SvPV((this_->__data_back),bcklen);
	topptr = SvPV((this_->__data_top),toplen);
	botptr = SvPV((this_->__data_bottom),botlen);
	lftptr = SvPV((this_->__data_left),lftlen);
	rtptr = SvPV((this_->__data_right),rtlen);

	if (frtptr || bckptr || topptr || botptr || lftptr || rtptr) {
		unsigned char *ptr;
		glGenTextures (6,BackTextures);

 		if (frtptr && frtlen) {
			ptr = SvPV((this_->__data_front),PL_na);
			glBindTexture (GL_TEXTURE_2D, BackTextures[FRONTTEX]);
                	do_texture(this_->__depth_front, this_->__x_front,
                       		 this_->__y_front,ptr,GL_REPEAT,GL_REPEAT,GL_LINEAR);
		}

 		if (bckptr && bcklen) {
			ptr = SvPV((this_->__data_back),PL_na);
			glBindTexture (GL_TEXTURE_2D, BackTextures[BACKTEX]);
                	do_texture(this_->__depth_back, this_->__x_back,
                       		 this_->__y_back,ptr,GL_REPEAT,GL_REPEAT,GL_LINEAR);
		}

 		if (rtptr && rtlen) {
			ptr = SvPV((this_->__data_right),PL_na);
			glBindTexture (GL_TEXTURE_2D, BackTextures[RIGHTTEX]);
                	do_texture(this_->__depth_right, this_->__x_right,
                       		 this_->__y_right,ptr,GL_REPEAT,GL_REPEAT,GL_LINEAR);
		}

 		if (topptr && toplen) {
			ptr = SvPV((this_->__data_top),PL_na);
			glBindTexture (GL_TEXTURE_2D, BackTextures[TOPTEX]);
                	do_texture(this_->__depth_top, this_->__x_top,
                       		 this_->__y_top,ptr,GL_REPEAT,GL_REPEAT,GL_LINEAR);
		}

 		if (botptr && botlen) {
			ptr = SvPV((this_->__data_bottom),PL_na);
			glBindTexture (GL_TEXTURE_2D, BackTextures[BOTTEX]);
                	do_texture(this_->__depth_bottom, this_->__x_bottom,
                       		 this_->__y_bottom,ptr,GL_REPEAT,GL_REPEAT,GL_LINEAR);
		}


 		if (lftptr && lftlen) {
			ptr = SvPV((this_->__data_left),PL_na);
			glBindTexture (GL_TEXTURE_2D, BackTextures[LEFTTEX]);
                	do_texture(this_->__depth_left, this_->__x_left,
                       		 this_->__y_left,ptr,GL_REPEAT,GL_REPEAT,GL_LINEAR);
		}

	}

	/* printf ("new background display list is %d\n",background_display_list); */
	glNewList(background_display_list,GL_COMPILE_AND_EXECUTE);


	if(verbose)  printf("TS: %f %f %f,      %f %f %f\n",x,y,z,sx,sy,sz);

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);


	sc = 2000.0; /* where to put the sky quads */
	glBegin(GL_QUADS);
	if(((this_->skyColor).n) == 1) {
		c1 = &(((this_->skyColor).p[0]));
		va1 = 0;
		va2 = PI/2; 
		bk_emis[0]=c1->c[0]; bk_emis[1]=c1->c[1]; bk_emis[2]=c1->c[2];
		glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
		glColor3f(c1->c[0], c1->c[1], c1->c[2]);

		for(v=0; v<2; v++) {
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;

				glVertex3f(sin(va2)*sc * cos(ha1), cos(va2)*sc, sin(va2) * sin(ha1)*sc);
				glVertex3f(sin(va2)*sc * cos(ha2), cos(va2)*sc, sin(va2) * sin(ha2)*sc);
				glVertex3f(sin(va1)*sc * cos(ha2), cos(va1)*sc, sin(va1) * sin(ha2)*sc);
				glVertex3f(sin(va1)*sc * cos(ha1), cos(va1)*sc, sin(va1) * sin(ha1)*sc);
			}
			va1 = va2;
			va2 = PI;
		}
	} else {
		va1 = 0;
		for(v=0; v<((this_->skyColor).n)-1; v++) {
			c1 = &(((this_->skyColor).p[v]));
			c2 = &(((this_->skyColor).p[v+1]));
			va2 = ((this_->skyAngle).p[v]);
			
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;

				bk_emis[0]=c2->c[0]; bk_emis[1]=c2->c[1]; bk_emis[2]=c2->c[2];
				glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
				glColor3f(c2->c[0], c2->c[1], c2->c[2]);
				glVertex3f(sin(va2) * cos(ha1)*sc, cos(va2)*sc, sin(va2) * sin(ha1)*sc);
				glVertex3f(sin(va2) * cos(ha2)*sc, cos(va2)*sc, sin(va2) * sin(ha2)*sc);
				bk_emis[0]=c1->c[0]; bk_emis[1]=c1->c[1]; bk_emis[2]=c1->c[2];
				glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
				glColor3f(c1->c[0], c1->c[1], c1->c[2]);
				glVertex3f(sin(va1) * cos(ha2)*sc, cos(va1)*sc, sin(va1) * sin(ha2)*sc);
				glVertex3f(sin(va1) * cos(ha1)*sc, cos(va1)*sc, sin(va1) * sin(ha1)*sc);
			}
			va1 = va2;
		}

		/* now, the spec states: "If the last skyAngle is less than pi, then the
		  colour band between the last skyAngle and the nadir is clamped to the last skyColor." */
		if (va2 < (PI-0.01)) {
			bk_emis[0]=c2->c[0]; bk_emis[1]=c2->c[1]; bk_emis[2]=c2->c[2];
			glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
			glColor3f(c2->c[0], c2->c[1], c2->c[2]);
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;
	
				glVertex3f(sin(PI) * cos(ha1)*sc, cos(PI)*sc, sin(PI) * sin(ha1)*sc);
				glVertex3f(sin(PI) * cos(ha2)*sc, cos(PI)*sc, sin(PI) * sin(ha2)*sc);
				glVertex3f(sin(va2) * cos(ha2)*sc, cos(va2)*sc, sin(va2) * sin(ha2)*sc);
				glVertex3f(sin(va2) * cos(ha1)*sc, cos(va2)*sc, sin(va2) * sin(ha1)*sc);
			}
		}
	}
	glEnd();


	/* Do the ground, if there is anything  to do. */
	if ((this_->groundColor).n>0) {
		sc = 1250.0; /* where to put the ground quads */
		glBegin(GL_QUADS);
		if(((this_->groundColor).n) == 1) {
			c1 = &(((this_->groundColor).p[0]));
			bk_emis[0]=c1->c[0]; bk_emis[1]=c1->c[1]; bk_emis[2]=c1->c[2];
			glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
			glColor3f(c1->c[0], c1->c[1], c1->c[2]);
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;

				glVertex3f(sin(PI) * cos(ha1)*sc, cos(PI)*sc, sin(PI) * sin(ha1)*sc);
				glVertex3f(sin(PI) * cos(ha2)*sc, cos(PI)*sc, sin(PI) * sin(ha2)*sc);
				glVertex3f(sin(PI/2) * cos(ha2)*sc, cos(PI/2)*sc, sin(PI/2) * sin(ha2)*sc);
				glVertex3f(sin(PI/2) * cos(ha1)*sc, cos(PI/2)*sc, sin(PI/2) * sin(ha1)*sc);
			}
		} else {
			va1 = PI;
			for(v=0; v<((this_->groundColor).n)-1; v++) {
				c1 = &(((this_->groundColor).p[v]));
				c2 = &(((this_->groundColor).p[v+1]));
				va2 = PI - ((this_->groundAngle).p[v]);
		
				for(h=0; h<hdiv; h++) {
					ha1 = h * PI*2 / hdiv;
					ha2 = (h+1) * PI*2 / hdiv;

					bk_emis[0]=c1->c[0]; bk_emis[1]=c1->c[1]; bk_emis[2]=c1->c[2];
					glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
					glColor3f(c1->c[0], c1->c[1], c1->c[2]);
					glVertex3f(sin(va1) * cos(ha1)*sc, cos(va1)*sc, sin(va1) * sin(ha1)*sc);
					glVertex3f(sin(va1) * cos(ha2)*sc, cos(va1)*sc, sin(va1) * sin(ha2)*sc);

					bk_emis[0]=c2->c[0]; bk_emis[1]=c2->c[1]; bk_emis[2]=c2->c[2];
					glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
					glColor3f(c2->c[0], c2->c[1], c2->c[2]);
					glVertex3f(sin(va2) * cos(ha2)*sc, cos(va2)*sc, sin(va2) * sin(ha2)*sc);
					glVertex3f(sin(va2) * cos(ha1)*sc, cos(va2)*sc, sin(va2) * sin(ha1)*sc);
				}
				va1 = va2;
			}
		}
		glEnd();
	}




	/* now, for the textures, if they exist */

	if (frtptr || bckptr || topptr || botptr || lftptr || rtptr) {
        	GLfloat mat_emission[] = {1.0,1.0,1.0,1.0};
       	 	GLfloat col_amb[] = {1.0, 1.0, 1.0, 1.0};
       	 	GLfloat col_dif[] = {1.0, 1.0, 1.0, 1.0};

        	glEnable (GL_LIGHTING);
        	glEnable(GL_TEXTURE_2D);
        	glColor3f(1,1,1);

		sc = 500.0; /* where to put the tex vertexes */

        	glMaterialfv(GL_FRONT,GL_EMISSION, mat_emission);
        	glLightfv (GL_LIGHT0, GL_AMBIENT, col_amb);
        	glLightfv (GL_LIGHT0, GL_DIFFUSE, col_dif);

		/* go through each of the 6 possible sides */

		if(bckptr && bcklen) {
			glBindTexture (GL_TEXTURE_2D, BackTextures[BACKTEX]);
			glBegin(GL_QUADS);
			glNormal3f(0,0,1); 
			glTexCoord2f(1, 1); glVertex3f(-sc, -sc, sc);
			glTexCoord2f(1, 0); glVertex3f(-sc, sc, sc);
			glTexCoord2f(0, 0); glVertex3f(sc, sc, sc);
			glTexCoord2f(0, 1); glVertex3f(sc, -sc, sc);
			glEnd();
		};

		if(frtptr && frtlen) {
			glBindTexture (GL_TEXTURE_2D, BackTextures[FRONTTEX]);
			glBegin(GL_QUADS);
			glNormal3f(0,0,-1);
			glTexCoord2f(1,0); glVertex3f(sc,sc,-sc);
			glTexCoord2f(0,0); glVertex3f(-sc,sc,-sc);
			glTexCoord2f(0,1); glVertex3f(-sc,-sc,-sc);
			glTexCoord2f(1,1); glVertex3f(sc,-sc,-sc); 
			glEnd();
		};

		if(topptr && toplen) {
			glBindTexture (GL_TEXTURE_2D, BackTextures[TOPTEX]);
			glBegin(GL_QUADS);
			glNormal3f(0,1,0);
			glTexCoord2f(1,0); glVertex3f(sc,sc,sc);
			glTexCoord2f(0,0); glVertex3f(-sc,sc,sc);
			glTexCoord2f(0,1); glVertex3f(-sc,sc,-sc);
			glTexCoord2f(1,1); glVertex3f(sc,sc,-sc);
			glEnd();
		};

		if(botptr && botlen) {
			glBindTexture (GL_TEXTURE_2D, BackTextures[BOTTEX]);
			glBegin(GL_QUADS);
			glNormal3f(0,-(1),0);
			glTexCoord2f(1,0); glVertex3f(sc,-sc,-sc);
			glTexCoord2f(0,0); glVertex3f(-sc,-sc,-sc);
			glTexCoord2f(0,1); glVertex3f(-sc,-sc,sc);
			glTexCoord2f(1,1); glVertex3f(sc,-sc,sc);
			glEnd();
		};

		if(rtptr && rtlen) {
			glBindTexture (GL_TEXTURE_2D, BackTextures[RIGHTTEX]);
			glBegin(GL_QUADS);
			glNormal3f(1,0,0);
			glTexCoord2f(1,0); glVertex3f(sc,sc,sc);
			glTexCoord2f(0,0); glVertex3f(sc,sc,-sc);
			glTexCoord2f(0,1); glVertex3f(sc,-sc,-sc);
			glTexCoord2f(1,1); glVertex3f(sc,-sc,sc);
			glEnd();
		};

		if(lftptr && lftlen) {
			glBindTexture (GL_TEXTURE_2D, BackTextures[LEFTTEX]);
			glBegin(GL_QUADS);
			glNormal3f(-1,0,0);
			glTexCoord2f(1,0); glVertex3f(-sc,sc, -sc);
			glTexCoord2f(0,0); glVertex3f(-sc,sc,  sc); 
			glTexCoord2f(0,1); glVertex3f(-sc,-sc, sc);
			glTexCoord2f(1,1); glVertex3f(-sc,-sc,-sc);
			glEnd();
		 };
	}

	/* end of textures... */
	glEndList();
	glPopMatrix();
	glPopAttrib();
',

ProximitySensor => q~
	/* Viewer pos = t_r2 */
	double cx,cy,cz;
	double len;
	struct pt dr1r2;
	struct pt dr2r3;
	struct pt vec;
	struct pt nor1,nor2;
	struct pt ins;
	static const struct pt yvec = {0,0.05,0};
	static const struct pt zvec = {0,0,-0.05};
	static const struct pt zpvec = {0,0,0.05};
	static const struct pt orig = {0,0,0};
	struct pt t_zvec, t_yvec, t_orig;
	GLdouble modelMatrix[16]; 
	GLdouble projMatrix[16];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(orig.x,orig.y,orig.z,modelMatrix,projMatrix,viewport,
		&t_orig.x,&t_orig.y,&t_orig.z);
	gluUnProject(zvec.x,zvec.y,zvec.z,modelMatrix,projMatrix,viewport,
		&t_zvec.x,&t_zvec.y,&t_zvec.z);
	gluUnProject(yvec.x,yvec.y,yvec.z,modelMatrix,projMatrix,viewport,
		&t_yvec.x,&t_yvec.y,&t_yvec.z);

	cx = t_orig.x - $f(center,0);
	cy = t_orig.y - $f(center,1);
	cz = t_orig.z - $f(center,2);

	if(!$f(enabled)) return;
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
/* don't know why this is here JAS

		if(APPROX(VECSQ(ins),0)) {
			printf ("Should die here: Proximitysensor problem!\n");
		}
*/

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
			vec[0] *= $f(ambientIntensity);
			vec[1] *= $f(ambientIntensity);
			vec[2] *= $f(ambientIntensity);
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
Transform => (join '','
	glPushMatrix();

	if(!reverse_trans) {
		glTranslatef(',(join ',',map {getf(Transform,translation,$_)} 0..2),'
		);
		glTranslatef(',(join ',',map {getf(Transform,center,$_)} 0..2),'
		);
		glRotatef(',getf(Transform,rotation,3),'/3.1415926536*180,',
			(join ',',map {getf(Transform,rotation,$_)} 0..2),'
		);
		glRotatef(',getf(Transform,scaleOrientation,3),'/3.1415926536*180,',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glScalef(',(join ',',map {getf(Transform,scale,$_)} 0..2),'
		);
		glRotatef(-(',getf(Transform,scaleOrientation,3),'/3.1415926536*180),',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,center,$_).")"} 0..2),'
		);
	} else {
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
	/*
	$endlist();
	*/
'),

##JAS Collision=> ('
##JAS printf ("Start of Collision - dont think this is needed...JAS \n");
##JAS 
##JAS 	glPushMatrix();
##JAS 	$start_list();
##JAS 	$end_list();
##JAS '),

# Simplistic...
Billboard => '
	GLdouble mod[16];
	GLdouble proj[16];
	struct pt vec, ax, cp, z = {0,0,1}, cp2,cp3, arcp;
	int align;
	double len; double len2;
	double angle;
	int sign;
	ax.x = $f(axisOfRotation,0);
	ax.y = $f(axisOfRotation,1);
	ax.z = $f(axisOfRotation,2);
	align = (APPROX(VECSQ(ax),0));
	glPushMatrix();

	glGetDoublev(GL_MODELVIEW_MATRIX, mod);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	gluUnProject(0,0,0,mod,proj,viewport,
		&vec.x,&vec.y,&vec.z);
	len = VECSQ(vec); if(APPROX(len,0)) {return;}
	VECSCALE(vec,1/sqrt(len));
	/* printf("Billboard: (%f %f %f) (%f %f %f)\n",vec.x,vec.y,vec.z,	
		ax.x, ax.y, ax.z); */
	if(!align) {
		VECCP(ax,z,arcp);
		VECCP(ax,arcp,cp3);
		len = VECSQ(ax); VECSCALE(ax,1/sqrt(len));
		VECCP(vec,ax,cp); /* cp is now 90deg to both vector and axis */
		len = sqrt(VECSQ(cp)); 
		if(APPROX(len,0)) {return;} /* Cant do a thing */
		VECSCALE(cp, 1/len)
		/* printf("Billboard: (%f %f %f) (%f %f %f)\n",cp.x,cp.y,cp.z,	
			cp3.x, cp3.y, cp3.z); */
		/* Now, find out angle between this and z axis */
		VECCP(cp,z,cp2);
		len2 = VECPT(cp,z); /* cos(angle) */
		len = sqrt(VECSQ(cp2)); /* this is abs(sin(angle)) */
		/* Now we need to find the sign first */
		if(VECPT(cp, arcp)>0) sign=-1; else sign=1;
		angle = atan2(len2,sign*len);
		/* printf("Billboard: sin angle = %f, cos angle = %f\n, sign: %d,
			atan2: %f\n", len, len2,sign,angle); */
		glRotatef(angle/3.1415926536*180, ax.x,ax.y,ax.z);
	} else {
		/* cp is the axis of the first rotation... */
		VECCP(z,vec,cp); len = sqrt(VECSQ(cp)); 
		VECSCALE(cp,1/len);
		VECCP(z,cp,cp2); 
		angle = asin(VECPT(cp2,vec));
		glRotatef(angle/3.1415926536*180, ax.x,ax.y,ax.z);
		
		/* XXXX */
		/* die("Cant do 0 0 0 billboard"); */

	}
',


Viewpoint => (join '','
	if(render_vp) {
		GLint vp[10];
		double a1;
		double angle;
		if(verbose) printf("Viewpoint: %d IB: %d..\n", 
			this_,$f(isBound));
		if(!$f(isBound)) {return;}
		render_anything = 0; /* Stop rendering any more */
		/* These have to be in this order because the viewpoint
		 * rotates in its place */
		glRotatef(-(',getf(Viewpoint,orientation,3),')/3.1415926536*180,',
			(join ',',map {getf(Viewpoint,orientation,$_)} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Viewpoint,position,$_).")"} 
			0..2),'
		);

		if (verbose) { 
		printf ("Rotation %f %f %f %f\n",
		-(',getf(Viewpoint,orientation,3),')/3.1415926536*180,',
			(join ',',map {getf(Viewpoint,orientation,$_)} 0..2),'
		);

		printf ("Translation %f %f %f\n",
		',(join ',',map {"-(".getf(Viewpoint,position,$_).")"} 
			0..2),'
		);
		}



		glGetIntegerv(GL_VIEWPORT, vp);
		if(vp[2] > vp[3]) {
			a1=0;
			angle = $f(fieldOfView)/3.1415926536*180;
		} else {
			a1 = $f(fieldOfView);
			a1 = atan2(sin(a1),vp[2]/((float)vp[3]) * cos(a1));
			angle = a1/3.1415926536*180;
		}
		if(verbose) printf("Vp: %d %d %d %d %f %f\n", vp[0], vp[1], vp[2], vp[3],
			a1, angle);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix(); /* This is so we do picking right */
		/* glLoadIdentity(); */
		gluPerspective(angle,vp[2]/(float)vp[3],0.1,vp_dist);
		glMatrixMode(GL_MODELVIEW);
	}
'),

NavigationInfo => '
        if(verbose) printf("NavigationInfo: %d IB: %d..\n",
                this_,$f(isBound));
        if(!$f(isBound)) {return;}
        /* I have no idea what else should go in here -- John Breen */
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
##JAS Collision => (join '','
##JAS 	glPopMatrix();
##JAS '),
Transform => (join '','
	glPopMatrix();
'),
Billboard => (join '','
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
	Group => '
		int nc = $f_n(children); 
		int i;
		int savedlight = curlight;
		struct VRML_Virt virt_DirectionalLight;

		if(verbose) {printf("RENDER GROUP START %d (%d)\n",this_, nc);}
		if($i(has_light)) {
			glPushAttrib(GL_LIGHTING_BIT|GL_ENABLE_BIT);
			for(i=0; i<nc; i++) {
				void *p = $f(children,i);
				if($ntyptest(p,DirectionalLight)) {
					render_node(p);
				}
			}
		}
		for(i=0; i<nc; i++) {
			void *p = $f(children,i);
			if(verbose) {printf("RENDER GROUP %d CHILD %d\n",this_, p);}
			/* Hmm - how much time does this consume? */
			/* Not that much. */
			if(!$i(has_light) || !$ntyptest(p,DirectionalLight)) {
				render_node(p);
			}
		}
		if($i(has_light)) {
			glPopAttrib();
		}
		if(verbose) {printf("RENDER GROUP END %d\n",this_);}

		curlight = savedlight;
	',
	Switch => '
		int wc = $f(whichChoice);
		if(wc >= 0 && wc < $f_n(choice)) {
			void *p = $f(choice,wc);
			render_node(p);
		}
	',
	LOD => '
		GLdouble mod[16];
		GLdouble proj[16];
		struct pt vec;
		double dist;
		int nran = $f_n(range);
		int nnod = $f_n(level);
		int i;
		void *p;

		if(!nran) {
			void *p = $f(level, 0);
			render_node(p);
			return;
		}

		glGetDoublev(GL_MODELVIEW_MATRIX, mod);
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		gluUnProject(0,0,0,mod,proj,viewport,
			&vec.x,&vec.y,&vec.z);
		vec.x -= $f(center,0);
		vec.y -= $f(center,1);
		vec.z -= $f(center,2);
		dist = sqrt(VECSQ(vec));
		i = 0;

		while (i<nran) {
			if(dist < $f(range,i)) {
				break;
			}
			i++;
		}
		if(i >= nnod) {i = nnod-1;}

		p = $f(level,i);
		render_node(p);

	',
	Appearance => '
		/* for shape display list redrawing */
		this_->_myshape = last_visited_shape; 

		if($f(textureTransform)) 
			render_node($f(textureTransform));

		if($f(texture)) {
			render_node($f(texture));
		} else {

			if($f(material)) {
				render_node($f(material));
			} else {
				/* no material, so just colour the following shape */
                        	/* Spec says to disable lighting and set coloUr to 1,1,1 */
                        	glDisable (GL_LIGHTING);
				glColor3f(1.0,1.0,1.0);
			} 
		}
	
	',
	Shape => '
		GLenum glError;


		if(!(this_->geometry)) { return; }

		/* Display lists used here. The name of the game is to use the
		display list for everything, except for sensitive nodes. */

		/* Appearance, Material, Shape, will make a new list, via this pointer */
		last_visited_shape = this_;

		/* a texture flag... */
		last_bound_texture = 0;
	

		glPushAttrib(GL_LIGHTING_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT);
		/* if we are rendering the geometry, see if we have a disp. list */
		if ((render_geom) && (!render_sensitive)) { 
			if(this_->_dlist) {
				if(this_->_dlchange == this_->_change) {
					glCallList(this_->_dlist); 
					glPopAttrib();
					return;
				} else {
					glDeleteLists(this_->_dlist,1);
				}
			}
			this_->_dlist = glGenLists(1);
			this_->_dlchange = this_->_change;
			glNewList(this_->_dlist,GL_COMPILE_AND_EXECUTE);

			/* is there an associated appearance node? */	
        	        if($f(appearance)) {
	                        render_node($f(appearance));
        	        } else {
				if (render_geom) {
	                            /* no material, so just colour the following shape */
                        	    /* Spec says to disable lighting and set coloUr to 1,1,1 */
                        	    glDisable (GL_LIGHTING);
        	                    glColor3f(1.0,1.0,1.0);
				}
	                }

			if (last_bound_texture != 0) {
				/* we had a texture */
				glEnable (GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D,last_bound_texture);
			}
		}

		/* Now, do the geometry */
		render_node((this_->geometry));

		if ((render_geom) && (!render_sensitive)) {
			if (this_->_dlchange == this_->_change) {
				/* premature ending of dlists */
				glEndList();
			# } else {
			# 	printf ("hmmm - display list changed on me\n");
			}

			glError = glGetError();
			while (glError != GL_NO_ERROR) {
				printf ("VRMLRend.pm::Shape:glError: %s\n",gluErrorString(glError));
				glError = glGetError();
			}
		}
		last_visited_shape = 0;
		glPopAttrib();
	',
);

$ChildC{Transform} = $ChildC{Group};
$ChildC{Billboard} = $ChildC{Group};
$ChildC{Anchor} = $ChildC{Group};
$ChildC{Collision} = $ChildC{Group};  ## JAS 

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
				vec[0] *= $f(ambientIntensity);
				vec[1] *= $f(ambientIntensity);
				vec[2] *= $f(ambientIntensity);
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
				vec[0] *= $f(ambientIntensity);
				vec[1] *= $f(ambientIntensity);
				vec[2] *= $f(ambientIntensity);
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


#######################################################################
#
# ChangedC - when the fields change, the following code is run before
# rendering for caching the data.
# 	

%ChangedC = (
	Group => '
		int i;
		int nc = $f_n(children); 
		struct VRML_Virt virt_DirectionalLight;
		$i(has_light) = 0;
		for(i=0; i<nc; i++) {
			void *p = $f(children,i);
			if($ntyptest(p,DirectionalLight)) {
				$i(has_light) ++;
			}
		}
	'
);


$ChangedC{Transform} = $ChangedC{Group};
$ChangedC{Billboard} = $ChangedC{Group};
$ChangedC{Anchor} = $ChangedC{Group};
$ChangedC{Collision} = $ChangedC{Group};

