# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# The C routines to render various nodes quickly
#
# Field values by subs so that generalization possible..
#
# getf(Node,"fieldname",[index,...]) returns c string to get field name.
# getaf(Node,"fieldname",n) returns comma-separated list of all the field values.
# getfn(Node,"fieldname"
#
# Of these, VP is taken into account by Transform
#
# Why so elaborate code generation?
#  - makes it easy to change structs later
#  - makes it very easy to add fast implementations for new proto'ed 
#    node types
#
#
# TODO:
#  Test indexedlineset
#
# $Log$
# Revision 1.38  2001/07/24 13:22:06  crc_canada
# 1) reduce memory usage for textures.
# 2) more parameter checking on indexedfacesets
#
# Revision 1.37  2001/07/18 14:09:29  crc_canada
# IFS tecCoordIndex with selected coordinates from within a large Coordinate node
# verified to work.
#
# Revision 1.36  2001/07/17 19:09:17  crc_canada
# render_polyrep to generate default tex coords with IFS a little bit better.
#
# Revision 1.35  2001/07/16 18:55:18  crc_canada
# Scale textures to fit texture limits of OpenGL implementation. (do_texture changed)
#
# Revision 1.34  2001/07/12 15:50:23  kitfox
# Merging extrusion_normals branch to the main branch
#
# Revision 1.33  2001/07/11 20:43:04  ayla
#
#
# Fixed problem with Plugin/Source/npfreewrl.c, so all debugging info. is turned
# off.
#
# Committing merge between NetscapeIntegration and the trunk.
#
# Revision 1.32  2001/07/05 16:04:00  crc_canada
# Initial IFS default texture rewrite.
#
# Revision 1.31  2001/06/25 18:35:01  crc_canada
# ElevationGrid textures now ok.
#
# Revision 1.30  2001/06/18 17:24:50  crc_canada
# IRIX compile warnings removed
#
# Revision 1.29  2001/06/15 19:43:29  crc_canada
# Alains Smooth Shading for Extrusions added.
#
# Revision 1.28.2.4  2001/07/10 14:05:01  kitfox
# Code Cleanup after fixing the last bug
#
# Revision 1.28.2.3  2001/07/10 13:22:13  kitfox
# Fixed a bug with smooth shading for elevation grids.  The bug had to do
# with some indexing mix-ups.
#
# Revision 1.28.2.2  2001/07/04 16:49:24  uid53492
#
# Applied smooth normals to ElevationGrids using the extrusion code.
#
# Revision 1.28.2.1  2001/06/27 20:48:03  kitfox
#
# Added a significant chunk of code that calculates normals that allow
# smooth shading on extrusions.
#
# Revision 1.28  2001/05/16 16:00:47  crc_canada
# check for degenerate triangles in render_ray_polyrep, and skip it if one is found.
#
# Revision 1.27  2001/04/27 16:49:43  crc_canada
# Working display lists for Shape nodes
#
# Revision 1.26  2001/04/24 19:55:00  crc_canada
# Display list work
#
# Revision 1.25  2001/03/26 17:40:36  crc_canada
# Background fixes for some implementations of OpenGL.
# IndexedLineSet no longer disrupts other renderings.
#
# Revision 1.24  2001/03/23 16:01:45  crc_canada
# Unknown
#
# Revision 1.23  2001/03/13 16:18:59  crc_canada
# Pre 0.28 checkin
#
# Revision 1.22  2001/02/16 18:31:52  crc_canada
# Cleaned up pixeltextures in do_texture routine
#
# Revision 1.21  2000/12/20 17:28:14  crc_canada
# more IndexedFaceSet work - normals this time
#
# Revision 1.20  2000/12/18 21:16:56  crc_canada
# IndexedFaceSet colorIndex and colorPerVertex now working correctly.
#
# Revision 1.19  2000/12/13 14:40:21  crc_canada
# Bug with texcoords field and Extrusions. (not being zeroed)
#
# Revision 1.18  2000/12/07 19:11:18  crc_canada
# IndexedFaceSet texture mapping
#
# Revision 1.17  2000/11/16 18:45:05  crc_canada
# Bug in do_textures; was not setting GL_CLAMP correctly, boolean test added
#
# Revision 1.16  2000/11/15 18:25:28  crc_canada
# Changed texture quality to give us good quality when gluScaleTexture is called.
#
# Revision 1.15  2000/11/15 15:26:35  crc_canada
# Removed a printf "Render tex coord".
#
# Revision 1.14  2000/11/07 14:51:36  crc_canada
# Better glGenTexture initialization
#
# Revision 1.13  2000/11/03 21:51:36  crc_canada
# texture pbjects for increased speed
#
# Revision 1.12  2000/10/28 17:40:57  crc_canada
# EAI addchildren, etc, should be ok now.
#
# Revision 1.11  2000/09/15 22:48:40  rcoscali
# Add patch from bob kozdemba for concave polygon fix
#
# Revision 1.10  2000/09/03 20:17:50  rcoscali
# Made some test for blending
# Tests are displayed with 38 & 39.wrlð
#
# Revision 1.9  2000/09/02 23:54:39  rcoscali
# Fixed the core dump for 27.wrl and 28.wrl
#
# Revision 1.8  2000/08/31 22:49:23  rcoscali
# Add depth 2 support (2 channels/color components) which isMINANCE_ALPHAre (wi
#
# Revision 1.7  2000/08/31 07:59:52  rcoscali
# Terminate fix of image loading
# Add flip_image routine in JPEG.xs
# Handle it in VRMLNodes & take care of depth in VRMLC.pm
#
# Revision 1.6  2000/08/30 00:04:19  rcoscali
# Comment out a glDisable(GL_LIGHTING) (uncommented by mistake ??)
# Fixed a problem in cone rendering (order of the vertexes)i
#
# Revision 1.5  2000/08/13 14:27:55  rcoscali
# Fixed a trace
#
# Revision 1.4  2000/08/08 21:15:36  rcoscali
# Fixed Image Texture rendering problem (depth parameter on gluScaleImage)
#
# Revision 1.3  2000/08/07 02:41:01  rcoscali
# Removed the comment of John (we discuss it and I think not usefull anymore in pixel tex code). Was image relevant.
#
# Revision 1.2  2000/08/06 19:48:37  rcoscali
# Fixed Cylinder. Now attacking cone.
#
# Revision 1.1.1.1  2000/08/04 07:07:38  rcoscali
# initial import (creation) of sources repository
#
#

# To allow faster internal representations of nodes to be calculated,
# there is the field '_change' which can be compared between the node
# and the internal rep - if different, needs to be regenerated.
#
# the rep needs to be allocated if _intern == 0.
# XXX Freeing?!!?

require 'VRMLFields.pm';
require 'VRMLNodes.pm';
require 'VRMLRend.pm';

#######################################################################
#######################################################################
#######################################################################
#
# RendRay --
#  code for checking whether a ray (defined by mouse pointer)
#  intersects with the geometry of the primitive.
#
#

# Y axis rotation around an unit vector:
# alpha = angle between Y and vec, theta = rotation angle
#  1. in X plane ->
#   Y = Y - sin(alpha) * (1-cos(theta))
#   X = sin(alpha) * sin(theta)
#
#  
# How to find out the orientation from two vectors (we are allowed
# to assume no negative scales)
#  1. Y -> Y' -> around any vector on the plane midway between the 
#                two vectors
#     Z -> Z' -> around any vector ""
#
# -> intersection.
#
# The plane is the midway normal between the two vectors
# (if the two vectors are the same, it is the vector).

%RendRayC = (
Box => '
	float x = $f(size,0)/2;
	float y = $f(size,1)/2;
	float z = $f(size,2)/2;
	/* 1. x=const-plane faces? */
	if(!XEQ) {
		float xrat0 = XRAT(x);
		float xrat1 = XRAT(-x);
		if(verbose) printf("!XEQ: %f %f\\n",xrat0,xrat1);
		if(TRAT(xrat0)) {
			float cy = MRATY(xrat0);
			if(verbose) printf("TRok: %f\\n",cy);
			if(cy >= -y && cy < y) {
				float cz = MRATZ(xrat0);
				if(verbose) printf("cyok: %f\\n",cz);
				if(cz >= -z && cz < z) {
					if(verbose) printf("czok:\\n");
					HIT(xrat0, x,cy,cz, 1,0,0, -1,-1, "cube x0");
				}
			}
		}
		if(TRAT(xrat1)) {
			float cy = MRATY(xrat1);
			if(cy >= -y && cy < y) {
				float cz = MRATZ(xrat1);
				if(cz >= -z && cz < z) {
					HIT(xrat1, -x,cy,cz, -1,0,0, -1,-1, "cube x1");
				}
			}
		}
	}
	if(!YEQ) {
		float yrat0 = YRAT(y);
		float yrat1 = YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = MRATX(yrat0);
			if(cx >= -x && cx < x) {
				float cz = MRATZ(yrat0);
				if(cz >= -z && cz < z) {
					HIT(yrat0, cx,y,cz, 0,1,0, -1,-1, "cube y0");
				}
			}
		}
		if(TRAT(yrat1)) {
			float cx = MRATX(yrat1);
			if(cx >= -x && cx < x) {
				float cz = MRATZ(yrat1);
				if(cz >= -z && cz < z) {
					HIT(yrat1, cx,-y,cz, 0,-1,0, -1,-1, "cube y1");
				}
			}
		}
	}
	if(!ZEQ) {
		float zrat0 = ZRAT(z);
		float zrat1 = ZRAT(-z);
		if(TRAT(zrat0)) {
			float cx = MRATX(zrat0);
			if(cx >= -x && cx < x) {
				float cy = MRATY(zrat0);
				if(cy >= -y && cy < y) {
					HIT(zrat0, cx,cy,z, 0,0,1, -1,-1,"cube z0");
				}
			}
		}
		if(TRAT(zrat1)) {
			float cx = MRATX(zrat1);
			if(cx >= -x && cx < x) {
				float cy = MRATY(zrat1);
				if(cy >= -y && cy < y) {
					HIT(zrat1, cx,cy,-z, 0,0,-1,  -1,-1,"cube z1");
				}
			}
		}
	}
',

# Distance to zero as function of ratio is
# sqrt(
#	((1-r)t_r1.x + r t_r2.x)**2 +
#	((1-r)t_r1.y + r t_r2.y)**2 +
#	((1-r)t_r1.z + r t_r2.z)**2
# ) == radius
# Therefore,
# radius ** 2 == ... ** 2 
# and 
# radius ** 2 = 
# 	(1-r)**2 * (t_r1.x**2 + t_r1.y**2 + t_r1.z**2) +
#       2*(r*(1-r)) * (t_r1.x*t_r2.x + t_r1.y*t_r2.y + t_r1.z*t_r2.z) +
#       r**2 (t_r2.x**2 ...)
# Let's name tr1sq, tr2sq, tr1tr2 and then we have
# radius ** 2 =  (1-r)**2 * tr1sq + 2 * r * (1-r) tr1tr2 + r**2 tr2sq
# = (tr1sq - 2*tr1tr2 + tr2sq) r**2 + 2 * r * (tr1tr2 - tr1sq) + tr1sq
# 
# I.e.
# 
# (tr1sq - 2*tr1tr2 + tr2sq) r**2 + 2 * r * (tr1tr2 - tr1sq) + 
#	(tr1sq - radius**2) == 0
#
# I.e. second degree eq. a r**2 + b r + c == 0 where
#  a = tr1sq - 2*tr1tr2 + tr2sq
#  b = 2*(tr1tr2 - tr1sq)
#  c = (tr1sq-radius**2)
# 
# 
Sphere => '
	float r = $f(radius);
	/* Center is at zero. t_r1 to t_r2 and t_r1 to zero are the vecs */
	float tr1sq = VECSQ(t_r1);
	float tr2sq = VECSQ(t_r2);
	float tr1tr2 = VECPT(t_r1,t_r2);
	struct pt dr2r1;
	float dlen;
	float a,b,c,disc;

	VECDIFF(t_r2,t_r1,dr2r1);
	dlen = VECSQ(dr2r1);

	a = dlen; /* tr1sq - 2*tr1tr2 + tr2sq; */
	b = 2*(VECPT(dr2r1, t_r1));
	c = tr1sq - r*r;

	disc = b*b - 4*a*c; /* The discriminant */
	
	if(disc > 0) { /* HITS */
		float q ;
		float sol1 ;
		float sol2 ;
		float cx,cy,cz;
		q = sqrt(disc);
		/* q = (-b+(b>0)?q:-q)/2; */
		sol1 = (-b+q)/(2*a);
		sol2 = (-b-q)/(2*a);
		/*
		printf("SPHSOL0: (%f %f %f) (%f %f %f)\\n",
			t_r1.x, t_r1.y, t_r1.z, t_r2.x, t_r2.y, t_r2.z);
		printf("SPHSOL: (%f %f %f) (%f) (%f %f) (%f) (%f %f)\\n",
			tr1sq, tr2sq, tr1tr2, a, b, c, und, sol1, sol2);
		*/
		cx = MRATX(sol1);
		cy = MRATY(sol1);
		cz = MRATZ(sol1);
		HIT(sol1, cx,cy,cz, cx/r,cy/r,cz/r, -1,-1, "sphere 0");
		cx = MRATX(sol2);
		cy = MRATY(sol2);
		cz = MRATZ(sol2);
		HIT(sol2, cx,cy,cz, cx/r,cy/r,cz/r, -1,-1, "sphere 1");
	}
',

# Cylinder: first test the caps, then against infinite cylinder.
Cylinder => '
	float h = $f(height)/2; /* pos and neg dir. */
	float r = $f(radius);
	float y = h;
	/* Caps */
	if(!YEQ) {
		float yrat0 = YRAT(y);
		float yrat1 = YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = MRATX(yrat0);
			float cz = MRATZ(yrat0);
			if(r*r > cx*cx+cz*cz) {
				HIT(yrat0, cx,y,cz, 0,1,0, -1,-1, "cylcap 0");
			}
		}
		if(TRAT(yrat1)) {
			float cx = MRATX(yrat1);
			float cz = MRATZ(yrat1);
			if(r*r > cx*cx+cz*cz) {
				HIT(yrat1, cx,-y,cz, 0,-1,0, -1,-1, "cylcap 1");
			}
		}
	}
	/* Body -- do same as for sphere, except no y axis in distance */
	if((!XEQ) && (!ZEQ)) {
		float dx = t_r2.x-t_r1.x; float dz = t_r2.z-t_r1.z;
		float a = dx*dx + dz*dz;
		float b = 2*(dx * t_r1.x + dz * t_r1.z);
		float c = t_r1.x * t_r1.x + t_r1.z * t_r1.z - r*r;
		float und;
		b /= a; c /= a;
		und = b*b - 4*c;
		if(und > 0) { /* HITS the infinite cylinder */
			float sol1 = (-b+sqrt(und))/2;
			float sol2 = (-b-sqrt(und))/2;
			float cy,cx,cz;
			cy = MRATY(sol1);
			if(cy > -h && cy < h) {
				cx = MRATX(sol1);
				cz = MRATZ(sol1);
				HIT(sol1, cx,cy,cz, cx/r,0,cz/r, -1,-1, "cylside 1");
			}
			cy = MRATY(sol2);
			if(cy > -h && cy < h) {
				cx = MRATX(sol2);
				cz = MRATZ(sol2);
				HIT(sol2, cx,cy,cz, cx/r,0,cz/r, -1,-1, "cylside 2");
			}
		}
	}
',

# For cone, this is most difficult. We have
# sqrt(
#	((1-r)t_r1.x + r t_r2.x)**2 +
#	((1-r)t_r1.z + r t_r2.z)**2
# ) == radius*( -( (1-r)t_r1.y + r t_r2.y )/(2*h)+0.5)
# == radius * ( -( r*(t_r2.y - t_r1.y) + t_r1.y )/(2*h)+0.5)
# == radius * ( -r*(t_r2.y-t_r1.y)/(2*h) + 0.5 - t_r1.y/(2*h))

#
# Other side: r*r*(
Cone => '
	float h = $f(height)/2; /* pos and neg dir. */
	float y = h;
	float r = $f(bottomRadius);
	float dx = t_r2.x-t_r1.x; float dz = t_r2.z-t_r1.z;
	float dy = t_r2.y-t_r1.y;
	float a = dx*dx + dz*dz - (r*r*dy*dy/(2*h*2*h));
	float b = 2*(dx*t_r1.x + dz*t_r1.z) +
		2*r*r*dy/(2*h)*(0.5-t_r1.y/(2*h));
	float tmp = (0.5-t_r1.y/(2*h));
	float c = t_r1.x * t_r1.x + t_r1.z * t_r1.z 
		- r*r*tmp*tmp;
	float und;
	b /= a; c /= a;
	und = b*b - 4*c;
	/* 
	printf("CONSOL0: (%f %f %f) (%f %f %f)\\n",
		t_r1.x, t_r1.y, t_r1.z, t_r2.x, t_r2.y, t_r2.z);
	printf("CONSOL: (%f %f %f) (%f) (%f %f) (%f)\\n",
		dx, dy, dz, a, b, c, und);
	*/
	if(und > 0) { /* HITS the infinite cylinder */
		float sol1 = (-b+sqrt(und))/2;
		float sol2 = (-b-sqrt(und))/2;
		float cy,cx,cz;
		float cy0;
		cy = MRATY(sol1);
		if(cy > -h && cy < h) {
			cx = MRATX(sol1);
			cz = MRATZ(sol1);
			/* XXX Normal */
			HIT(sol1, cx,cy,cz, cx/r,0,cz/r, -1,-1, "conside 1");
		}
		cy0 = cy;
		cy = MRATY(sol2);
		if(cy > -h && cy < h) {
			cx = MRATX(sol2);
			cz = MRATZ(sol2);
			HIT(sol2, cx,cy,cz, cx/r,0,cz/r, -1,-1, "conside 2");
		}
		/*
		printf("CONSOLV: (%f %f) (%f %f)\\n", sol1, sol2,cy0,cy);
		*/
	}
	if(!YEQ) {
		float yrat0 = YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = MRATX(yrat0);
			float cz = MRATZ(yrat0);
			if(r*r > cx*cx + cz*cz) {
				HIT(yrat0, cx, -y, cz, 0, -1, 0, -1, -1, "conbot");
			}
		}
	}
',

ElevationGrid => ( '
		$mk_polyrep();
		render_ray_polyrep(this_, 
			0, NULL
		);
'),

Extrusion => ( '
		$mk_polyrep();
		render_ray_polyrep(this_, 
			0, NULL
		);
'),

IndexedFaceSet => '
		struct SFColor *points; int npoints;
		$fv(coord, points, get3, &npoints);
		$mk_polyrep();
		render_ray_polyrep(this_, 
			npoints, points
		);
',

);

#####################################################################3
#####################################################################3
#####################################################################3
#
# GenPolyRep
#  code for generating internal polygonal representations
#  of some nodes (ElevationGrid, Extrusion and IndexedFaceSet)
#
#


# In one sense, we could just plot the polygons here and be done
# with it -- displaylists would speed it up.
#
# However, doing this in a device-independent fashion will help
# us a *lot* in porting to some other 3D api.
%GenPolyRepC = (
# ElevationGrid = 2 triangles per each face.
# No color or normal support yet
ElevationGrid => '

		int x,z;
		int nx = $f(xDimension);
		float xs = $f(xSpacing);
		int nz = $f(zDimension);
		float zs = $f(zSpacing);
		float *f = $f(height);
		float a[3],b[3];
		int *cindex; 
        	int *tcindex;
		float *coord;
		float *tcoord;
		int *colindex;
		int ntri = (nx && nz ? 2 * (nx-1) * (nz-1) : 0);
		int triind;
		int nf = $f_n(height);
		int cpv = $f(colorPerVertex);
		struct SFColor *colors; int ncolors=0;
		struct VRML_PolyRep *rep_ = this_->_intern;
		struct VRML_Extrusion_Adj *adj; 
		float crease_angle = $f(creaseAngle);
		$fv_null(color, colors, get3, &ncolors);
		rep_->ntri = ntri;

		/* printf("Gen elevgrid %d %d %d\\n", ntri, nx, nz); */
		if(nf != nx * nz) {
			die("Elevationgrid: too many / too few: %d %d %d\\n",
				nf, nx, nz);
		}
		if(ncolors) {
			if(!cpv && ncolors < (nx-1) * (nz-1)) {
				die("Elevationgrid: too few colors");
			}
			if(cpv && ncolors < nx*nz) {
				die("Elevationgrid: 2too few colors");
			}
		}
		cindex = rep_->cindex = malloc(sizeof(*(rep_->cindex))*3*(ntri));
		coord = rep_->coord = malloc(sizeof(*(rep_->coord))*nx*nz*3);
		colindex = rep_->colindex = malloc(sizeof(*(rep_->colindex))*3*(ntri));
		if (glIsEnabled(GL_TEXTURE_2D)) {
			/* so, we now have to worry about textures. */
			   
	        	tcindex = rep_->tcindex = malloc(sizeof(*(rep_->tcindex))*3*(ntri));
			tcoord = rep_->tcoord = malloc(sizeof(*(rep_->tcoord))*nx*nz*3);
		}

		/* Flat */
		rep_->normal = malloc(sizeof(*(rep_->normal))*3*ntri*3);
		rep_->norindex = malloc(sizeof(*(rep_->norindex))*3*ntri);	

		adj     = malloc( sizeof(struct VRML_Extrusion_Adj) * nx * nz ); /*AG*/

		/* in C always check if you got the mem you wanted...  >;->		*/
  		if(!(cindex && coord && tcoord && colindex && rep_->normal && rep_->norindex && adj)) {
			die("Not enough memory for ElevationGrid node triangles... ;(");
		} 
 

		/* Prepare the coordinates */

		for(x=0; x<nx; x++) {
		 for(z=0; z<nz; z++) {
		  float h = f[x+z*nx];
		  coord[(x+z*nx)*3+0] = x*xs;
		  coord[(x+z*nx)*3+1] = h;
		  coord[(x+z*nx)*3+2] = z*zs;
		  if (glIsEnabled(GL_TEXTURE_2D)) {
			tcoord[(x+z*nx)*3+0] = (float) x/(nx-1);
		  	tcoord[(x+z*nx)*3+1] = 0;
		  	tcoord[(x+z*nx)*3+2] = (float) z/(nz-1);
		  }
		 }
		}
		/* set the indices to the coordinates		*/
		{
		int A,B,C,D; /* should referr to the four vertices 
				of the polygon	
				(hopefully) counted counter-clockwise, like

				 D----C
				 |    |
				 |    |
				 |    |
				 A----B

				*/
		struct pt ac,bd,/* help vectors	*/
			ab,cd;	/* help vectors	for testing intersection */
		int E,F;	/* third point to be used for the triangles*/
			
		triind = 0;
		for(x=0; x<nx-1; x++) {
		 for(z=0; z<nz-1; z++) {
		  A=x+z*nx;
		  B=(x+1)+z*nx;
		  C=(x+1)+(z+1)*nx;
		  D=x+(z+1)*nx;
		/* calculate the distance A-C and see, 
			if it is smaller as B-D        			*/
		VEC_FROM_COORDDIFF(coord,C,coord,A,ac);
		VEC_FROM_COORDDIFF(coord,D,coord,B,bd);

		if(sqrt(VECSQ(ac))>sqrt(VECSQ(bd))) {
		      E=B; F=D;
		} else {
		      E=C; F=A;
		}
		  
		  /* 1: */
		  cindex[triind*3+0] = D; cindex[triind*3+1] = A; cindex[triind*3+2] = E;
		  if (glIsEnabled(GL_TEXTURE_2D)) {
			tcindex[triind*3+0] =D;
			tcindex[triind*3+1] =A;
			tcindex[triind*3+2] =E;
		  }
		  if(cpv) {
			  colindex[triind*3+0] = D;
			  colindex[triind*3+1] = A;
			  colindex[triind*3+2] = E;
		  } else {
			  colindex[triind*3+0] = x+z*(nx-1);
			  colindex[triind*3+1] = x+z*(nx-1);
			  colindex[triind*3+2] = x+z*(nx-1);
		  }
		rep_->norindex[triind*3+0] = triind;
		rep_->norindex[triind*3+1] = triind;
		rep_->norindex[triind*3+2] = triind;
		  triind ++;
		  /* 2: */
		  cindex[triind*3+0] = B; cindex[triind*3+1] = C; cindex[triind*3+2] = F;
		  if (glIsEnabled(GL_TEXTURE_2D)) {
			tcindex[triind*3+0] = B;
			tcindex[triind*3+1] = C;
			tcindex[triind*3+2] = F;
		  }
		  if(cpv) {
			  colindex[triind*3+0] = B;
			  colindex[triind*3+1] = C;
			  colindex[triind*3+2] = F;
		  } else {
			  colindex[triind*3+0] = x+z*(nx-1);
			  colindex[triind*3+1] = x+z*(nx-1);
			  colindex[triind*3+2] = x+z*(nx-1);
		  }
		rep_->norindex[triind*3+0] = triind;
		rep_->norindex[triind*3+1] = triind;
		rep_->norindex[triind*3+2] = triind;
		  triind ++; 
		 }
		}
		} /* end of block */

		/* smooth normals for the Elevation Grid */
		/* In the part of this system that calculates these normals */
		/* x is north-south,  z is east-west.                       */
		for(x=0; x<nx; x++) {
			for(z=0; z<nz; z++) {
                                if (z == 0){adj[x + z * nx].north_pt = -1;}
                                else {	adj[x + z * nx].north_pt = x + (z-1) * nx; }

                                if (z == nz-1){adj[x + z * nx].south_pt = -1;}
                                else {	adj[x + z * nx].south_pt = x + (z+1) * nx; }

                                if (x == nx-1){adj[x + z * nx].east_pt = -1;}
                                else {	adj[x + z * nx].east_pt = x + (z * nx) + 1;}

                                if (x == 0){adj[x + z * nx].west_pt = -1;}
                                else {	adj[x + z * nx].west_pt = x + (z * nx) - 1;}

                                if ( (	adj[x + z * nx].north_pt == -1) || (adj[x + z * nx].east_pt == -1) ){
                                        adj[x + z * nx].north_east_pt = -1;}
                                else {	adj[x + z * nx].north_east_pt = adj[x + z * nx].north_pt + 1;}

                                if ( (	adj[x + z * nx].north_pt == -1) || (adj[x + z * nx].west_pt == -1) ){
                                        adj[x + z * nx].north_west_pt = -1;}
                                else {	adj[x + z * nx].north_west_pt = adj[x + z * nx].north_pt - 1;}

                                if ( (	adj[x + z * nx].south_pt == -1) || (adj[x + z * nx].east_pt == -1) ){
                                        adj[x + z * nx].south_east_pt = -1;}
                                else {	adj[x + z * nx].south_east_pt = adj[x + z * nx].south_pt + 1;}

                                if ( (	adj[x + z * nx].south_pt == -1) || (adj[x + z * nx].west_pt == -1) ){
                                        adj[x + z * nx].south_west_pt = -1;}
                                else {	adj[x + z * nx].south_west_pt = adj[x + z * nx].south_pt - 1;}
			}
		}

		if (smooth_normals){
			calc_poly_normals_extrusion(rep_, adj, nx, nz, ntri, 0, crease_angle);
		}
		else {
		        calc_poly_normals_flat(rep_);
		}

		if (adj) free(adj);
	',

Extrusion => (do "VRMLExtrusion.pm"),
IndexedFaceSet => '
	int i,j;	/* general purpose counters */

	int cin = $f_n(coordIndex);
	int cpv = $f(colorPerVertex);
	int npv = $f(normalPerVertex);
	int tcin = $f_n(texCoordIndex);
	int colin = $f_n(colorIndex); 
	int norin = $f_n(normalIndex);

	int curpoly;
	int ntri = 0;
	int nvert = 0;
	int npoints = 0;
	int nnormals=0;
	int ntexCoords = 0;

	/* flags for errors */
	int ntexerrors = 0;

	float a[3]; float b[3];
	struct SFColor *c1,*c2,*c3;
	struct SFColor *points; 
        struct SFVec2f *texCoords; 
	struct VRML_PolyRep *rep_ = this_->_intern;
	struct SFColor *normals;

	int *cindex;
	int *colindex;
        int *tcindex;
	int *norindex;

        /* texture coords */

        $fv_null(texCoord, texCoords, get2, &ntexCoords);
	
	/*
        printf("\n\ntexCoords = %lx     ntexCoords = %d\n", texCoords, ntexCoords);
	for (i=0; i<ntexCoords; i++)
           printf( "\\ttexCoord point #%d = [%.5f, %.5f]\\n", i, 
		texCoords[i].c[0], texCoords[i].c[1] ); 
        printf("texCoordIndex count = %d\n", tcin);
	*/


	/* IndexedFaceSet coords and normals */
	$fv(coord, points, get3, &npoints);
	$fv_null(normal, normals, get3, &nnormals); 

	/*	
	printf ("points = %lx \n",npoints);
	for (i=0; i<npoints; i++)
	  printf ("\t point #%d = [%.5f %.5f %.5f]\n", i,
		points[i].c[0], points[i].c[1], points[i].c[2]);
	

	printf ("normalIndex size (norin) %d\n",norin);
	printf ("normals = (nnormals) %d\n",nnormals);
	for (i=0; i<nnormals; i++) {
	  printf ("\t normal #%d = [%.5f %.5f %.5f]\n", i,
		normals[i].c[0],normals[i].c[1],normals[i].c[2]);
	}
	*/



	/* If the texCoord field is not NULL, it shall contain a TextureCoordinate node. 
	   The texture coordinates in that node are applied to the vertices of the 
	   IndexedFaceSet as follows:

    		f.If the texCoordIndex field is not empty, then it is used to choose texture 
		coordinates for each vertex of the IndexedFaceSet in exactly the same
      		manner that the coordIndex field is used to choose coordinates for each vertex 
		from the Coordinate node. 

			1. The texCoordIndex field shall contain at
      			   least as many indices as the coordIndex field, 

			2. and shall contain end-of-face 
			   markers (-1) in exactly the same places as the coordIndex field. 

			3. If the
      			   greatest index in the texCoordIndex field is N, then there shall be 
			   N+1 texture coordinates in the TextureCoordinate node. 

		g.If the texCoordIndex field is empty, then the coordIndex array is used to 
		choose texture coordinates from the TextureCoordinate node. If the
      		greatest index in the coordIndex field is N, then there shall be N+1 texture 
		coordinates in the TextureCoordinate node. 
	*/
        /* printf ("IFS, tcin = %d, ntexCoords %d, cin %d\n",tcin, ntexCoords, cin); */
	if (ntexCoords != 0) { /* texCoord field not NULL */
		if (tcin > 0 && tcin < cin) {
			/* Rule F part 1 */
			printf ("IndexedFaceSet, Rule F part 1: texCoordIndex less than coordIndex (%d %d)\n",
				tcin, cin);
			exit(1);
			ntexerrors = 1;
		}

		if (tcin == 0 && ntexCoords != npoints) { 
			/* rule G */
			printf ("IndexedFaceSet, Rule G: points %d texCoords %d and no texCoordIndex\n",
				npoints,ntexCoords);
			ntexerrors = 1;
			exit(1);
	   	}
	}


	/* wander through to see how much memory needs allocating */
	for(i=0; i<cin; i++) {
		if($f(coordIndex,i) == -1) {
			if(nvert < 3) {
				die("Too few vertices in indexedfaceset poly");
			} 
                        if(tcin > 0  && $f(texCoordIndex,i) != -1) {
				/* Rule F part 2 see above */
                                printf ("IndexedFaceSet, Rule F, part 2: coordIndex[%d] = -1 => expect texCoordIndex[%d] = -1 (but is %d)\\n", i, i, $f(texCoordIndex,i));
				ntexerrors = 1;
				exit(1);
                        }
			ntri += nvert-2;
			nvert = 0;
		} else {
			if (tcin > 0 && $f(texCoordIndex,i) >= ntexCoords) {
				/* Rule F, part 3 see above */
				printf ("IndexedFaceSet, Rule F, part 3: TexCoordIndex[%d] %d is greater than num texCoord (%d)\n",i, $f(texCoordIndex,i),
					ntexCoords);
				ntexerrors = 1;
				exit(1);
			}
			nvert ++;
		}
	}
	if(nvert>2) {ntri += nvert-2;}

	/* printf ("IFS - nvert %d ntri %d\n",nvert,ntri); */

	cindex = rep_->cindex = malloc(sizeof(*(rep_->cindex))*3*(ntri));
	colindex = rep_->colindex = malloc(sizeof(*(rep_->colindex))*3*(ntri));
	norindex = rep_->norindex = malloc(sizeof(*(rep_->norindex))*3*ntri);
	rep_->normal = malloc(sizeof(*(rep_->normal))*3*ntri);
	rep_->ntri = ntri;

	if (ntexerrors == 0)
		tcindex = rep_->tcindex = malloc(sizeof(*(rep_->tcindex))*3*(ntri));
	else {
		ntexCoords = 0; tcin = 0; 
		printf ("killing textures %x\n",this_->texCoord);
		exit(1);
	}

	/* in C always check if you got the mem you wanted...  >;->		*/
  	if(!(cindex && colindex && tcindex && norindex && rep_->normal )) {
		die("Not enough memory for IndexFaceSet node triangles... ;(");
	} 



	if(!$f(convex)) {
               /* Begin a non-convex polygon */
               gluBeginPolygon( global_tessobj );

	} /* else */ {  
		/* coord indexes */
		int initind = -1; 
		int lastind=-1;
		/* texture coord indexes */
		int inittcind = -1; 
		int lasttcind=-1;
		/* color index indexes */
		int initcolind = -1; 
		int lastcolind=-1;
		/* normalIndex indexes */
		int initnorind = -1; 
		int lastnorind = -1;
		
		int triind = 0;
		curpoly = 0;
		for(i=0; i<cin; i++) {
			/* printf ("count is %d Coord index is %d\n",i,$f(coordIndex,i));  */

			if($f(coordIndex,i) == -1) {
				initind=-1; lastind=-1;
                                inittcind = -1; lasttcind = -1;
				initcolind=-1; lastcolind=-1;
				initnorind=-1; lastnorind=-1;
				curpoly ++;
			} else {
				if(initind == -1) {
					/* printf ("initind == -1\n"); */
					initind = $f(coordIndex,i);
					if(tcin) inittcind = $f(texCoordIndex,i);
					if(colin) initcolind = $f(colorIndex,i);
					if(norin) initnorind = $f(normalIndex,i);
				} else if(lastind == -1) {
					/* printf ("lastind == -1\n"); */
					lastind = $f(coordIndex,i);
					if(tcin) lasttcind = $f(texCoordIndex,i);
					if(colin) lastcolind = $f(colorIndex,i);
					if(norin) lastnorind = $f(normalIndex,i);
				} else {
					cindex[triind*3+0] = initind;
					cindex[triind*3+1] = lastind;
					cindex[triind*3+2] = $f(coordIndex,i);

					/* colorIndex bounds check */
        				if (colin && cpv &&(colin <=i)) {
						printf ("IFS - colorIndex too small\n");
						colin = 0; 
					} else if (colin && (colin<=curpoly)) {
						printf ("IFS - colorIndex too small\n");
						colin = 0; 
					}

					/* normalIndex bounds check */
        				if (norin && npv &&(norin <=i)) {
						printf ("IFS - normalIndex too small\n");
						norin = 0; 
					} else if (norin && (norin<=curpoly)) {
						printf ("IFS - normalIndex too small\n");
						norin = 0; 
					}



					/* colour index */
					if(cpv) {
						if (colin) {
						colindex[triind*3+0] = initcolind;
						colindex[triind*3+1] = lastcolind;
						colindex[triind*3+2] = $f(colorIndex,i);
						} else {
						colindex[triind*3+0] = initind;
						colindex[triind*3+1] = lastind;
						colindex[triind*3+2] = $f(coordIndex,i);
						}
					} else {
						/* printf ("cpv, %d %d %d\n",
							curpoly, curpoly, curpoly); */
						if (colin) {
						colindex[triind*3+0] = 
								$f(colorIndex,curpoly);
						colindex[triind*3+1] = 
								$f(colorIndex,curpoly);
						colindex[triind*3+2] = 
								$f(colorIndex,curpoly);
						} else {
						colindex[triind*3+0] = curpoly;
						colindex[triind*3+1] = curpoly;
						colindex[triind*3+2] = curpoly;
						}
					}

					/* normal index */
					if(npv && nnormals) {
						if (norin) {
						norindex[triind*3+0] = initnorind;
						norindex[triind*3+1] = lastnorind;
						norindex[triind*3+2] = $f(normalIndex,i);
						} else {
						norindex[triind*3+0] = initind;
						norindex[triind*3+1] = lastind;
						norindex[triind*3+2] = $f(coordIndex,i);
						}
					} 
					if (!npv && nnormals) {
						if (norin) {
						norindex[triind*3+0] = 
								$f(normalIndex,curpoly);
						norindex[triind*3+1] = 
								$f(normalIndex,curpoly);
						norindex[triind*3+2] = 
								$f(normalIndex,curpoly);
						} else {
						norindex[triind*3+0] = curpoly;
						norindex[triind*3+1] = curpoly;
						norindex[triind*3+2] = curpoly;
						}
					}
					/* no normals, so calculate them */
					if (!nnormals) {
						c1 = &(points[initind]);
						c2 = &(points[lastind]); 
						c3 = &(points[$f(coordIndex,i)]);
						a[0] = c2->c[0] - c1->c[0];
						a[1] = c2->c[1] - c1->c[1];
						a[2] = c2->c[2] - c1->c[2];
						b[0] = c3->c[0] - c1->c[0];
						b[1] = c3->c[1] - c1->c[1];
						b[2] = c3->c[2] - c1->c[2];
						rep_->normal[triind*3+0] =
							a[1]*b[2] - b[1]*a[2];
						rep_->normal[triind*3+1] =
							-(a[0]*b[2] - b[0]*a[2]);
						rep_->normal[triind*3+2] =
							a[0]*b[1] - b[0]*a[1];
						rep_->norindex[triind*3+0] = triind;
						rep_->norindex[triind*3+1] = triind;
						rep_->norindex[triind*3+2] = triind;
					}



                                        if(tcin && ntexCoords) {
						/* printf("tcin && ntexCoords = %d && %d\\n",tcin, ntexCoords);  */
                                                tcindex[triind*3+0] = inittcind;
                                                tcindex[triind*3+1] = lasttcind;
                                                tcindex[triind*3+2] = $f(texCoordIndex,i);
						/* printf ("tcoords are %d %d %d\n",inittcind, lasttcind, $f(texCoordIndex,i)); */
                                        } else if (!tcin && ntexCoords) {
						/* printf("! tcin && ntexCoords = %d && %d\\n", tcin, ntexCoords);  */
                                                /* Use coord index */
                                                tcindex[triind*3+0] = cindex[triind*3+0];
                                                tcindex[triind*3+1] = cindex[triind*3+1];
                                                tcindex[triind*3+2] = cindex[triind*3+2];
                                        }
					lastind = $f(coordIndex,i);
					if(tcin && ntexCoords) {
						lasttcind =  $f(texCoordIndex,i);
					} else {	
						lasttcind = $f(coordIndex,i);
					}
					triind++;
				}
			}
		}
	}
if(!$f(convex)) {

/* End a non-convex polygon */
gluEndPolygon( global_tessobj );

}
',
);

######################################################################
######################################################################
######################################################################
#
# Get3
#  get a coordinate / color / normal array from the node.
#

%Get3C = (
Coordinate => '
	/* ptr for invalidating the shape display list */
	this_->_myshape = last_visited_shape; 
	*n = $f_n(point); 
	return $f(point);
',
Color => '
	/* ptr for invalidating the shape display list */
	this_->_myshape = last_visited_shape;
	*n = $f_n(color); 
	return $f(color);
',
Normal => '
	/* ptr for invalidating the shape display list */
	this_->_myshape = last_visited_shape; 
	*n = $f_n(vector);
	return $f(vector);
'
);

%Get2C = (
TextureCoordinate => '
	/* ptr for invalidating the shape display list */
	this_->_myshape = last_visited_shape; 
	*n = $f_n(point);
	return $f(point);
',
);

######################################################################
######################################################################
######################################################################
#
# Generation
#  Functions for generating the code
#


{
	my %AllNodes = (%RendC, %RendRayC, %PrepC, %FinC, %ChildC, %Get3C, %Get2C, %LightC,
		%ChangedC);
	@NodeTypes = keys %AllNodes;
}

sub assgn_m {
	my($f, $l) = @_;
	return ((join '',map {"m[$_] = ".getf(Material, $f, $_).";"} 0..2).
		"m[3] = $l;");
}

# XXX Might we need separate fields for separate structs?
sub getf {
	my ($t, $f, @l) = @_;
	my $type = $VRML::Nodes{$t}{FieldTypes}{$f};
	if($type eq "") {
		die("Invalid type $t $f '$type'");
	}
	return "VRML::Field::$type"->cget("(this_->$f)",@l);
}

sub getfn {
	my($t, $f) = @_;
	my $type = $VRML::Nodes{$t}{FieldTypes}{$f};
	return "VRML::Field::$type"->cgetn("(this_->$f)");
}

# XXX Code copy :(
sub fvirt {
	my($t, $f, $ret, $v, @a) = @_;
	# Die if not exists
	my $type = $VRML::Nodes{$t}{FieldTypes}{$f};
	if($type ne "SFNode") {
		die("Fvirt must have SFNode");
	}
	if($ret) {$ret = "$ret = ";}
	return "if(this_->$f) {
		  if(!(*(struct VRML_Virt **)(this_->$f))->$v) {
		  	die(\"NULL METHOD $t $f $v\");
		  }
		  $ret ((*(struct VRML_Virt **)(this_->$f))->$v(this_->$f,
		    ".(join ',',@a).")) ;}
 	  else { (die(\"NULL FIELD $t $f $a\"));}";
}

sub fvirt_null {
	my($t, $f, $ret, $v, @a) = @_;
	# Die if not exists
	my $type = $VRML::Nodes{$t}{FieldTypes}{$f};
	if($type ne "SFNode") {
		die("Fvirt must have SFNode");
	}
	if($ret) {$ret = "$ret = ";}
	return "if(this_->$f) {
		  if(!(*(struct VRML_Virt **)(this_->$f))->$v) {
		  	die(\"NULL METHOD $t $f $v\");
		  }
		  $ret ((*(struct VRML_Virt **)(this_->$f))->$v(this_->$f,
		    ".(join ',',@a).")) ;
		}";
}


sub fgetfnvirt_n {
	my($n, $ret, $v, @a) = @_;
	if($ret) {$ret = "$ret = ";}
	return "if($n) {
	         if(!(*(struct VRML_Virt **)n)->$v) {
		  	die(\"NULL METHOD $n $ret $v\");
		 }
		 $ret ((*(struct VRML_Virt **)($n))->$v($n,
		    ".(join ',',@a).")) ;}
	"
}

sub rend_geom {
	return $_[0];
}

################################################################
#
# gen_struct - Generate a node structure, adding fields for
# internal use

sub gen_struct {
	my($name,$node) = @_;
	my @f = keys %{$node->{FieldTypes}};
	my $nf = scalar @f;
	# /* Store actual point etc. later */
       my $s = "struct VRML_$name {\n" .
               " /***/ struct VRML_Virt *v;\n"         	.
               " /*s*/ int _sens; \n"                  	.
               " /*t*/ int _hit; \n"                   	.
               " /*a*/ int _change; \n"                	.
               " /*n*/ int _dlchange; \n"              	.
               " /*d*/ GLuint _dlist; \n"              	.
               " /*a*/ int _dl2change; \n"             	.
               " /*r*/ GLuint _dl2ist; \n"             	.
	       "       void **_parents; \n"	  	.
	       "       int _nparents; \n"		.
	       "       int _nparalloc; \n"		.
	       "       int _ichange; \n"		.
	       "       GLuint _texture; \n"		.
	       " /*disp list JAS*/ struct VRML_Shape *_myshape; \n"		.
               " /*d*/ void *_intern; \n"              	.
               " /***/\n";
	
	my $o = "
void *
get_${name}_offsets(p)
	SV *p;
CODE:
	int *ptr_;
	SvGROW(p,($nf+1)*sizeof(int));
	SvCUR_set(p,($nf+1)*sizeof(int));
	ptr_ = (int *)SvPV(p,PL_na);
";
	my $p = " {
		my \$s = '';
		my \$v = get_${name}_offsets(\$s);
		\@{\$n->{$name}{Offs}}{".(join ',',map {"\"$_\""} @f,'_end_')."} =
			unpack(\"i*\",\$s);
		\$n->{$name}{Virt} = \$v;
 }
	";
	for(@f) {
		my $cty = "VRML::Field::$node->{FieldTypes}{$_}"->ctype($_);
		$s .= "\t$cty;\n";
		$o .= "\t*ptr_++ = offsetof(struct VRML_$name, $_);\n";
	}
	$o .= "\t*ptr_++ = sizeof(struct VRML_$name);\n";
	$o .= "RETVAL=&(virt_${name});
	if(verbose) printf(\"$name virtual: %d\\\n\", RETVAL);
OUTPUT:
	RETVAL
";
	$s .= $ExtraMem{$name};
	$s .= "};\n";
	return ($s,$o,$p);
}

#########################################################
sub get_offsf {
	my($f) = @_;
	my ($ct) = ("VRML::Field::$_")->ctype("*ptr_");
	my ($ctp) = ("VRML::Field::$_")->ctype("*");
	my ($c) = ("VRML::Field::$_")->cfunc("(*ptr_)", "sv_");
	my ($ca) = ("VRML::Field::$_")->calloc("(*ptr_)");
	my ($cf) = ("VRML::Field::$_")->cfree("(*ptr_)");
	return "

void 
set_offs_$f(ptr,offs,sv_)
	void *ptr
	int offs
	SV *sv_
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	{struct VRML_Box *p;
	 p = ptr;
	 p->_change ++;

	/* force a rebuild of the display list for this shape */
	if (p->_myshape != 0) {
        	p->_myshape->_change++;
	}

	}
	$c


void 
alloc_offs_$f(ptr,offs)
	void *ptr
	int offs
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	$ca

void
free_offs_$f(ptr,offs)
	void *ptr
	int offs
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	$cf

"
}
#######################################################

sub get_rendfunc {
	my($n) = @_;
	print "RENDF $n ";
	# XXX
	my @f = qw/Prep Rend Child Fin RendRay GenPolyRep Light Get3 Get2
		Changed/;
	my $f;
	my $v = "
static struct VRML_Virt virt_${n} = { ".
	(join ',',map {${$_."C"}{$n} ? "${n}_$_" : "NULL"} @f).
",\"$n\"};";
	for(@f) {
		my $c =${$_."C"}{$n};
		next if !defined $c;
		print "$_ (",length($c),") ";
		# Substitute field gets

		$c =~ s/\$f\(([^)]*)\)/getf($n,split ',',$1)/ge;
		$c =~ s/\$i\(([^)]*)\)/(this_->$1)/g;
		$c =~ s/\$f_n\(([^)]*)\)/getfn($n,split ',',$1)/ge;
		$c =~ s/\$fv\(([^)]*)\)/fvirt($n,split ',',$1)/ge;
		$c =~ s/\$fv_null\(([^)]*)\)/fvirt_null($n,split ',',$1)/ge;
		$c =~ s/\$mk_polyrep\(\)/if(!this_->_intern || 
			this_->_change != ((struct VRML_PolyRep *)this_->_intern)->_change)
				regen_polyrep(this_);/g;


		$c =~ s/\$endlist\(\)/
			glEndList()
			/g;
		$c =~ s/\$startlist2\(\)/
		        if(!this_->_dl2ist) {
				this_->_dl2ist = glGenLists(1);
			}
			if(this_->_dl2change != this_->_change) {
				glNewList(this_->_dl2ist,GL_COMPILE_AND_EXECUTE);
				this_->_dl2change = this_->_change;
			} else {
				glCallList(this_->_dl2ist); return;
			}/g;

		$c =~ s/\$ntyptest\(([^),]*),([^),]*)\)/
				(((struct VRML_Box *)$1)->v == 	
					& virt_$2)/g;
		if($_ eq "Get3") {
			$f .= "\n\nstruct SFColor *${n}_$_(void *nod_,int *n)";
		} elsif($_ eq "Get2") {
			$f .= "\n\nstruct SFVec2f *${n}_$_(void *nod_,int *n)";
		} else {
			$f .= "\n\nvoid ${n}_$_(void *nod_)";
		}
		$f .= "{ /* GENERATED FROM HASH ${_}C, MEMBER $n */
			struct VRML_$n *this_ = (struct VRML_$n *)nod_;
			{$c}
			}";
	}
	print "\n";
	return ($f,$v);
}

######################################################################
######################################################################
######################################################################
#
# gen - the main function. this contains much verbatim code
#
#

sub gen {
	for(@VRML::Fields) {
		push @str, ("VRML::Field::$_")->cstruct . "\n";
		push @xsfn, get_offsf($_);
	}
        push @str, "\n/* and now the structs for the nodetypes */ \n";
	for(@NodeTypes) {
		my $no = $VRML::Nodes{$_}; 
		my($str, $offs, $perl) = gen_struct($_, $no);
		push @str, $str;
		push @xsfn, $offs;
		push @poffsfn, $perl;
		my($f, $vstru) = get_rendfunc($_);
		push @func, $f;
		push @vstruc, $vstru;
	}
	open XS, ">VRMLFunc.xs";
	print XS '
/* VRMLFunc.c generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */

/* Code here comes almost verbatim from VRMLC.pm */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "OpenGL/OpenGL.m"

#define offset_of(p_type,field) ((unsigned int)(&(((p_type)NULL)->field)-NULL))

#ifdef M_PI
#define PI M_PI
#else
#define PI 3.141592653589793
#endif

/* Faster trig macros (thanks for Robin Williams) */
/* fixed code, thanks to Etienne Grossmann */

/*
   (t_aa,  t_ab)  constants for the rotation params 
   (t_sa,  t_ca)  this 2D point will be rotated by UP_TRIG1
   (t_sa1, t_ca1) temp vars
*/
#define DECL_TRIG1 float t_aa, t_ab, t_sa, t_ca, t_sa1, t_ca1;
#define INIT_TRIG1(div) t_aa = sin(PI/(div)); t_aa *= 2*t_aa; t_ab = -sin(2*PI/(div));
#define START_TRIG1 t_sa = 0; t_ca = -1;
#define UP_TRIG1 t_sa1 = t_sa; t_sa -= t_sa*t_aa - t_ca * t_ab; t_ca -= t_ca * t_aa + t_sa1 * t_ab;
#define SIN1 t_sa
#define COS1 t_ca


#define DECL_TRIG2 float t2_aa, t2_ab, t2_sa, t2_ca, t2_sa1, t2_ca1;
#define INIT_TRIG2(div) t2_aa = sin(PI/(div)); t2_aa *= 2*t2_aa; t2_ab = -sin(2*PI/(div));
/* Define starting point of horizontal rotations */
#define START_TRIG2 t2_sa = -1; t2_ca = 0;
/* #define START_TRIG2 t2_sa = 0; t2_ca = -1; */
#define UP_TRIG2 t2_sa1 = t2_sa; t2_sa -= t2_sa*t2_aa - t2_ca * t2_ab; t2_ca -= t2_ca * t2_aa + t2_sa1 * t2_ab;
#define SIN2 t2_sa
#define COS2 t2_ca


D_OPENGL;


/* Rearrange to take advantage of headlight when off */
int curlight = 0;
int nlightcodes = 7;
int lightcode[7] = {
	GL_LIGHT1,
	GL_LIGHT2,
	GL_LIGHT3,
	GL_LIGHT4,
	GL_LIGHT5,
	GL_LIGHT6,
	GL_LIGHT7,
};
int nextlight() {
	if(curlight == nlightcodes) { return -1; }
	return lightcode[curlight++];
}



struct pt {GLdouble x,y,z;};

struct VRML_Extrusion_Adj {
	int south_pt;
	int north_pt;
	int east_pt;
	int west_pt;

	int north_east_pt;
	int south_east_pt;
	int south_west_pt;
	int north_west_pt;

	struct pt north_vec;
	struct pt south_vec;
	struct pt east_vec;
	struct pt west_vec;
	
	struct pt first_quad_diag_vec;
	struct pt second_quad_diag_vec;
	struct pt third_quad_diag_vec;
	struct pt fourth_quad_diag_vec;

	struct pt north_edge_vec;
	struct pt east_edge_vec;
	struct pt south_edge_vec;
	struct pt west_edge_vec;

	struct pt cumul_normal_vec;
};



struct VRML_Virt {
	void (*prep)(void *);
	void (*rend)(void *); 
	void (*children)(void *);
	void (*fin)(void *);
	void (*rendray)(void *);
	void (*mkpolyrep)(void *);
	void (*light)(void *);
	/* And get float coordinates : Coordinate, Color */
	/* XXX Relies on MFColor repr.. */
	struct SFColor *(*get3)(void *, int *); /* Number in int */
	struct SFVec2f *(*get2)(void *, int *); /* Number in int */
	void (*changed)(void *);
	char *name;
};

/* Internal representation of IndexedFaceSet, Extrusion & ElevationGrid:
 * set of triangles.
 * done so that we get rid of concave polygons etc.
 */
struct VRML_PolyRep { /* Currently a bit wasteful, because copying */
	int _change;
	int ntri; /* number of triangles */
	int alloc_tri; /* number of allocated triangles */
	int *cindex;   /* triples (per triangle) */
	float *coord; /* triples (per point) */
	int *colindex;   /* triples (per triangle) */
	float *color; /* triples or null */
	int *norindex;
	float *normal; /* triples or null */
        int *tcindex; /* triples or null */
        float *tcoord;	/* triples (per triangle) of texture coords */
};

';
	print XS join '',@str;
	print XS '

int verbose;

int reverse_trans;
int render_vp; 
int render_geom;
int render_light;
int render_sensitive;
int render_blend;

/* in Shape/Appearance, we want two kicks at the can */
int render_textures;
GLuint last_bound_texture;
struct VRML_Shape *last_visited_shape = 0;

int horiz_div; int vert_div;
int vp_dist = 200000;

/* flag which specifies smooth or rough shading for extrusions */
int smooth_normals = 0; 

int cur_hits=0;

/* These two points define a ray in window coordinates */


struct pt r1 = {0,0,-1},r2 = {0,0,0},r3 = {0,1,0};
struct pt t_r1,t_r2,t_r3; /* transformed ray */
void *hypersensitive = 0; int hyperhit = 0;
struct pt hyper_r1,hyper_r2; /* Transformed ray for the hypersensitive node */

GLint viewport[4] = {-1,-1,2,2};

/* These three points define 1. hitpoint 2., 3. two different tangents
 * of the surface at hitpoint (to get transformation correctly */ 

/* All in window coordinates */

struct pt hp, ht1, ht2;
double hpdist; /* distance in ray: 0 = r1, 1 = r2, 2 = 2*r2-r1... */

struct currayhit {
void *node; /* What node hit at that distance? */
GLdouble modelMatrix[16]; /* What the matrices were at that node */
GLdouble projMatrix[16];
} rh,rph,rhhyper;
 /* used to test new hits */

/* defines for raycasting: */
#define APPROX(a,b) (fabs(a-b)<0.00000001)
#define NORMAL_VECTOR_LENGTH_TOLERANCE 0.00001
/* (test if the vector part of a rotation is normalized) */
#define IS_ROTATION_VEC_NOT_NORMAL(rot)        ( \
       fabs(1-sqrt(rot.r[0]*rot.r[0]+rot.r[1]*rot.r[1]+rot.r[2]*rot.r[2])) \
               >NORMAL_VECTOR_LENGTH_TOLERANCE \
)

/* defines for raycasting: */
#define XEQ (APPROX(t_r1.x,t_r2.x))
#define YEQ (APPROX(t_r1.y,t_r2.y))
#define ZEQ (APPROX(t_r1.z,t_r2.z))
/* xrat(a) = ratio to reach coordinate a on axis x */
#define XRAT(a) (((a)-t_r1.x)/(t_r2.x-t_r1.x))
#define YRAT(a) (((a)-t_r1.y)/(t_r2.y-t_r1.y))
#define ZRAT(a) (((a)-t_r1.z)/(t_r2.z-t_r1.z))
/* mratx(r) = x-coordinate gotten by multiplying by given ratio */
#define MRATX(a) (t_r1.x + (a)*(t_r2.x-t_r1.x))
#define MRATY(a) (t_r1.y + (a)*(t_r2.y-t_r1.y))
#define MRATZ(a) (t_r1.z + (a)*(t_r2.z-t_r1.z))
/* trat: test if a ratio is reasonable */
#undef TRAT
#define TRAT(a) 1
#undef TRAT
#define TRAT(a) ((a) > 0 && ((a) < hpdist || hpdist < 0))

#define VECSQ(a) VECPT(a,a)
#define VECPT(a,b) ((a).x*(b).x + (a).y*(b).y + (a).z*(b).z)
#define VECDIFF(a,b,c) {(c).x = (a).x-(b).x;(c).y = (a).y-(b).y;(c).z = (a).z-(b).z;}
#define VEC_FROM_CDIFF(a,b,r) {(r).x = (a).c[0]-(b).c[0];(r).y = (a).c[1]-(b).c[1];(r).z = (a).c[2]-(b).c[2];}
#define VECCP(a,b,c) {(c).x = (a).y*(b).z-(b).y*(a).z; (c).y = -((a).x*(b).z-(b).x*(a).z); (c).z = (a).x*(b).y-(b).x*(a).y;}
#define VECSCALE(a,c) {(a).x *= c; (a).y *= c; (a).z *= c;}

/*special case ; used in Extrusion.GenPolyRep and ElevationGrid.GenPolyRep: 
 *	Calc diff vec from 2 coordvecs which must be in the same field 	*/
#define VEC_FROM_COORDDIFF(f,a,g,b,v) {\
	(v).x= (f)[(a)*3]-(g)[(b)*3];	\
	(v).y= (f)[(a)*3+1]-(g)[(b)*3+1];	\
	(v).z= (f)[(a)*3+2]-(g)[(b)*3+2];	\
}

/* rotate a vector along one axis				*/
#define VECROTATE_X(c,angle) { \
	/*(c).x =  (c).x	*/ \
	  (c).y = 		  cos(angle) * (c).y 	- sin(angle) * (c).z; \
	  (c).z = 		  sin(angle) * (c).y 	+ cos(angle) * (c).z; \
	}
#define VECROTATE_Y(c,angle) { \
	  (c).x = cos(angle)*(c).x +			+ sin(angle) * (c).z; \
	/*(c).y = 				(c).y 	*/ \
	  (c).z = -sin(angle)*(c).x 			+ cos(angle) * (c).z; \
	}
#define VECROTATE_Z(c,angle) { \
	  (c).x = cos(angle)*(c).x - sin(angle) * (c).y;	\
	  (c).y = sin(angle)*(c).x + cos(angle) * (c).y; 	\
	/*(c).z = s						 (c).z; */ \
	}

#define MATRIX_ROTATION_X(angle,m) {\
	m[0][0]=1; m[0][1]=0; m[0][2]=0; \
	m[1][0]=0; m[1][1]=cos(angle); m[1][2]=- sin(angle); \
	m[2][0]=0; m[2][1]=sin(angle); m[2][2]=cos(angle); \
}
#define MATRIX_ROTATION_Y(angle,m) {\
	m[0][0]=cos(angle); m[0][1]=0; m[0][2]=sin(angle); \
	m[1][0]=0; m[1][1]=1; m[1][2]=0; \
	m[2][0]=-sin(angle); m[2][1]=0; m[2][2]=cos(angle); \
}
#define MATRIX_ROTATION_Z(angle,m) {\
	m[0][0]=cos(angle); m[0][1]=- sin(angle); m[0][2]=0; \
	m[1][0]=sin(angle); m[1][1]=cos(angle); m[1][2]=0; \
	m[2][0]=0; m[2][1]=0; m[2][2]=1; \
}

/* next matrix calculation comes from comp.graphics.algorithms FAQ	*/
/* the axis vector has to be normalized					*/
#define MATRIX_FROM_ROTATION(ro,m) { \
	struct { double x,y,z,w ; } __q; \
        double sinHalfTheta = sin(0.5*(ro.r[3]));\
        double xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;\
        __q.x = (ro.r[0])*sinHalfTheta;\
        __q.y = (ro.r[1])*sinHalfTheta;\
        __q.z = (ro.r[2])*sinHalfTheta;\
        __q.w = cos(0.5*(ro.r[3]));\
        xs = 2*__q.x;  ys = 2*__q.y;  zs = 2*__q.z;\
        wx = __q.w*xs; wy = __q.w*ys; wz = __q.w*zs;\
        xx = __q.x*xs; xy = __q.x*ys; xz = __q.x*zs;\
        yy = __q.y*ys; yz = __q.y*zs; zz = __q.z*zs;\
        m[0][0] = 1 - (yy + zz); m[0][1] = xy - wz;      m[0][2] = xz + wy;\
        m[1][0] = xy + wz;       m[1][1] = 1 - (xx + zz);m[1][2] = yz - wx;\
        m[2][0] = xz - wy;       m[2][1] = yz + wx;      m[2][2] = 1-(xx + yy);\
}

/* matrix multiplication */
#define VECMM(m,c) { \
	double ___x=(c).x,___y=(c).y,___z=(c).z; \
	(c).x= m[0][0]*___x + m[0][1]*___y + m[0][2]*___z; \
	(c).y= m[1][0]*___x + m[1][1]*___y + m[1][2]*___z; \
	(c).z= m[2][0]*___x + m[2][1]*___y + m[2][2]*___z; \
}

	
/* next define rotates vector c with rotation vector r and angle */
/*  after section 5.8 of the VRML`97 spec			 */

#define VECROTATE(rx,ry,rz,angle,nc) { \
	double ___x=(nc).x,___y=(nc).y,___z=(nc).z; \
	double ___c=cos(angle),  ___s=sin(angle), ___t=1-___c; \
	(nc).x=   (___t*((rx)*(rx))+___c)     *___x    \
	        + (___t*(rx)*(ry)  -___s*(rz))*___y    \
	        + (___t*(rx)*(rz)  +___s*(ry))*___z ;  \
	(nc).y=   (___t*(rx)*(ry)  +___s*(rz))*___x    \
	        + (___t*((ry)*(ry))+___c)     *___y    \
	        + (___t*(ry)*(rz)  -___s*(rx))*___z ;  \
	(nc).z=   (___t*(rx)*(rz)  -___s*(ry))*___x    \
	        + (___t*(ry)*(rz)  +___s*(rx))*___y    \
	        + (___t*((rz)*(rz))+___c)     *___z ;  \
	}


/*
#define VECROTATE(rx,ry,rz,angle,c) { \
	double ___c=cos(angle),  ___s=sin(angle), ___t=1-___c; \
	(c).x=   (___t*((rx)*(rx))+___c)     *(c).x    \
	       + (___t*(rx)*(ry)  +___s*(rz))*(c).y    \
	       + (___t*(rx)*(rz)  -___s*(ry))*(c).z ;  \
	(c).y=   (___t*(rx)*(ry)  -___s*(rz))*(c).x    \
	       + (___t*((ry)*(ry))+___c)     *(c).y    \
	       + (___t*(ry)*(rz)  +___s*(rx))*(c).z ;  \
	(c).z=   (___t*(rx)*(rz)  +___s*(ry))*(c).x    \
	       + (___t*(ry)*(rz)  -___s*(rx))*(c).y    \
	       + (___t*((rz)*(rz))+ ___c)    *(c).z ;  \
	}

*/
/* next define abbreviates VECROTATE with use of the SFRotation struct	*/
#define VECRROTATE(ro,c) VECROTATE((ro).r[0],(ro).r[1],(ro).r[2],(ro).r[3],c)	



#define HIT rayhit

/* Sub, rather than big macro... */
void rayhit(float rat, float cx,float cy,float cz, float nx,float ny,float nz, 
float tx,float ty, char *descr)  {
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLdouble wx, wy, wz;
	/* Real rat-testing */
	if(verbose) printf("RAY HIT %s! %f (%f %f %f) (%f %f %f)\nR: (%f %f %f) (%f %f %f)\n",
		descr, rat,cx,cy,cz,nx,ny,nz,
		t_r1.x, t_r1.y, t_r1.z,
		t_r2.x, t_r2.y, t_r2.z
		);
	if(rat<0 || (rat>hpdist && hpdist >= 0)) {
		return;
	}
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluProject(cx,cy,cz, modelMatrix, projMatrix, viewport,
		&hp.x, &hp.y, &hp.z);
	hpdist = rat;
	rh=rph;
	rhhyper=rph;
}

/* Call this when modelview and projection modified */
void upd_ray() {
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(r1.x,r1.y,r1.z,modelMatrix,projMatrix,viewport,
		&t_r1.x,&t_r1.y,&t_r1.z);
	gluUnProject(r2.x,r2.y,r2.z,modelMatrix,projMatrix,viewport,
		&t_r2.x,&t_r2.y,&t_r2.z);
	gluUnProject(r3.x,r3.y,r3.z,modelMatrix,projMatrix,viewport,
		&t_r3.x,&t_r3.y,&t_r3.z);
/*	printf("Upd_ray: (%f %f %f)->(%f %f %f) == (%f %f %f)->(%f %f %f)\n",
		r1.x,r1.y,r1.z,r2.x,r2.y,r2.z,
		t_r1.x,t_r1.y,t_r1.z,t_r2.x,t_r2.y,t_r2.z);
*/
}


void *what_vp;
int render_anything; /* Turned off when we hit the viewpoint */
void render_node(void *node);
void render_polyrep(void *node, 
	int npoints, struct SFColor *points,
	int ncolors, struct SFColor *colors,
	int nnormals, struct SFColor *normals,
	int ntexcoords, struct SFVec2f *texcoords
	);
void regen_polyrep(void *node) ;
void calc_poly_normals_flat(struct VRML_PolyRep *rep);

void calc_poly_normals_extrusion(struct VRML_PolyRep *rep, 
			struct VRML_Extrusion_Adj *adj,
			int nspi, int nsec, int ntri, int nctri,
			float crease_angle);
void calc_vector_product(struct pt a, struct pt b, struct pt *c );
float calc_vector_length(struct pt p);
float calc_vector_scalar_product(struct pt a, struct pt b);
float calc_angle_between_two_vectors(struct pt a, struct pt b);
int find_edge_config_at_this_point(struct VRML_PolyRep *rep,
				struct VRML_Extrusion_Adj *adj,
				int index_pt, float creaseAngle);
int find_the_quadrant_of_this_triangle(struct VRML_PolyRep *rep,
				struct VRML_Extrusion_Adj *adj, 
				int index_pt, int a_pt, int b_pt);
void normalize_vector(struct pt *vec); 

void render_ray_polyrep(void *node,
	int npoints, struct SFColor *points);
	
	
/*********************************************************************
 * General tessellation functions
 *
 * to use the tessellation function, you have to
 * let global_tess_polyrep point towards a structure.
 * global_tess_polyrep->ntri is the first index number which will
 * be filled by the routines (which is the number of triangles
 * already represented in global_tess_polyrep)
 * global_tess_polyrep->cindex and global_tess_polyrep->coords have
 * to point towards enough memory.
 * (And you have to give gluTessVertex a third argument, in which
 * the new coords are written, it has to be a vector of
 * GLdouble s with enough space)
 * After calling gluTessEndPolygon() these vector will be filled.
 * global_tess_polyrep->ntri will contain the absolute
 * number of triangles in global_tess_polyrep after tessellation.
 */
  
/* GLUtesselator *global_tessobj;	/* this would be GLU 1.2 */
/* unfortunately we have to use GLU v1.1			 */

GLUtriangulatorObj *global_tessobj;	
struct VRML_PolyRep *global_tess_polyrep=NULL;

/* and now all the callback functions, which will be called
	by OpenGL automatically, if the Polygon is specified	*/

void ber_tess_begin(GLenum e) {
/*               printf(" ber_tess_begin   e = %s\\n", (e == GL_TRIANGLES ? "GL_TRIANGLES" : "UNKNOWN")); */
		/* we only should get GL_TRIANGLES as type, because
		we defined  the edge_flag callback		*/
		/* check, if the structure is there		*/
	if(e!=GL_TRIANGLES) 
		die("Something went wrong while tessellating!");
}

void ber_tess_end(void) {
/*printf("ber_tess_end: Tesselation done.\\n");*/
	/* nothing to do	*/
}

void ber_tess_edgeflag(GLenum flag) {
/* printf("ber_tess_end: An edge was done (flag = %d).\\n", flag); */
	/* nothing to do, this function has to be registered
	so that only GL_TRIANGLES are used	*/
}

void ber_tess_vertex(void *p) {
	static int x=0;
	GLdouble *dp=p;

#define GTP global_tess_polyrep
	
	if(GTP->ntri >= GTP->alloc_tri) {
		die("Too many tesselated triangles!");
	}

/*printf("ber_tess_vertex: A vertex is coming   x = %d   ntri = %d.\\n", x, GTP->ntri); */
#undef GTP
	
	global_tess_polyrep->coord[(global_tess_polyrep->ntri)*9+x*3]  =dp[0];
	global_tess_polyrep->coord[(global_tess_polyrep->ntri)*9+x*3+1]=dp[1];
	global_tess_polyrep->coord[(global_tess_polyrep->ntri)*9+x*3+2]=dp[2];
	global_tess_polyrep->cindex[(global_tess_polyrep->ntri)*3+x]=
					(global_tess_polyrep->ntri)*3+x;
	if(x==2) {
		x=0;
		(global_tess_polyrep->ntri)++;
	} else x++;
}

void ber_tess_error(GLenum e) {
	printf("ERROR %d: >%s<\n",e,gluErrorString(e));
}

/* next function has to be called once, after an OpenGL context is made
	and before tessellation is started			*/
	
void new_tessellation(void) {
	global_tessobj=gluNewTess();
	if(!global_tessobj)
		die("Got no memory for Tessellation Object!");
		
	/* register the CallBackfunctions				*/
	gluTessCallback(global_tessobj,GLU_BEGIN,ber_tess_begin);
	gluTessCallback(global_tessobj,GLU_EDGE_FLAG,ber_tess_edgeflag);
	gluTessCallback(global_tessobj,GLU_VERTEX,ber_tess_vertex);
	gluTessCallback(global_tessobj,GLU_ERROR,ber_tess_error);
	gluTessCallback(global_tessobj,GLU_END,ber_tess_end);
	
	if(verbose){ printf("Tessellation Initialized!\n"); }
}

/* next function should be called once at the end, but where?	*/
void destruct_tessellation(void) {
	gluDeleteTess(global_tessobj);
	if(verbose){ printf("Tessellation Object deleted!\n"); }
}



/*********************************************************************
 * Code here is generated from the hashes in VRMLC.pm and VRMLRend.pm
 */
	';

#######################################################j


	print XS join '',@func;
	print XS join '',@vstruc;
#######################################################
	print XS <<'ENDHERE'

/*********************************************************************
 * Code here again comes almost verbatim from VRMLC.pm 
 */

/*********************************************************************
 *********************************************************************
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
	int pt;
	int pti;
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
	printf("Render polyrep %d '%s' (%d %d): %d\n",node,v->name, 
			p->_change, r->_change, r->ntri);
	printf ("\tnpoints %d ncolors %d nnormals %d\n",
			points,colors,normals);
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


	/* Do we have any colours?	*/
	hasc = (ncolors || r->color);
	if(hasc) {
		glEnable(GL_COLOR_MATERIAL);
	}

	glBegin(GL_TRIANGLES);
	for(i=0; i<r->ntri*3; i++) {
		int nori = i;
		int coli = i;
		int tci = i;
		int ind = r->cindex[i];
		GLfloat color[4];
		GLfloat a,b,c = 0.0;

		/*  printf ("rp, i, ntri*3 %d %d\n",i,r->ntri*3); */

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
			/* printf ("tc1 %f %f\n",texcoords[tci].c[0],texcoords[tci].c[1]); */
		  	glTexCoord2fv(texcoords[tci].c);
		    } else if (r->tcoord) {
			/* printf ("tc2 %f %f\n", r->tcoord[3*ind+0], r->tcoord[3*ind+2]); */
		  	glTexCoord2f( r->tcoord[3*ind+0], r->tcoord[3*ind+2]);
		    } else {
			/* default textures */
			/* we want the S values to range from 0..1, and the 
			   T values to range from 0...S/T */
		  	/* printf ("tc3, %f %f\n", (XYZ[Sindex] - minVals[Sindex])/Ssize,
                                        (XYZ[Tindex] - minVals[Tindex])/Ssize); */
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
	int pti;
	float *point[3];
	struct pt v1, v2, v3;
	struct pt x1, x2, x3;
	struct pt ray;
	float pt1, pt2, pt3;
	struct pt hitpoint;
	float tmp1,tmp2,tmp3;
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
		float len;
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

/* Assuming that norindexes set and that cindex is set */
void calc_poly_normals_flat(struct VRML_PolyRep *rep) 
{
	int i;
	float a[3],b[3], *v1,*v2,*v3;
	for(i=0; i<rep->ntri; i++) {
		v1 = rep->coord+3*rep->cindex[i*3+0];
		v2 = rep->coord+3*rep->cindex[i*3+1];
		v3 = rep->coord+3*rep->cindex[i*3+2];
		
		/* 
		printf ("cpnf %d using cindex %d %d %d\n",
		i,rep->cindex[i*3],rep->cindex[i*3+1],rep->cindex[i*3+2]);
		printf ("	v1 %f %f %f\n",v1[0],v1[1],v1[2]);
		printf ("	v2 %f %f %f\n",v2[0],v2[1],v2[2]);
		printf ("	v3 %f %f %f\n",v3[0],v3[1],v3[2]);
		*/

		a[0] = v2[0]-v1[0];
		a[1] = v2[1]-v1[1];
		a[2] = v2[2]-v1[2];
		b[0] = v3[0]-v1[0];
		b[1] = v3[1]-v1[1];
		b[2] = v3[2]-v1[2];
		rep->normal[i*3+0] =
			a[1]*b[2] - b[1]*a[2];
		rep->normal[i*3+1] =
			-(a[0]*b[2] - b[0]*a[2]);
		rep->normal[i*3+2] =
			a[0]*b[1] - b[0]*a[1];
	}
}




void calc_vector_product(struct pt a, struct pt b, struct pt *c )
{
	c->x = a.y * b.z - a.z * b.y;
	c->y = a.z * b.x - a.x * b.z;
	c->z = a.x * b.y - a.y * b.x;
}



float calc_vector_length( struct pt p )
{
	return sqrt(p.x*p.x + p.y*p.y + p.z*p.z); 
}



float calc_vector_scalar_product(struct pt a, struct pt b)
{
	return (a.x*b.x) + (a.y*b.y) + (a.z*b.z);
}



float calc_angle_between_two_vectors(struct pt a, struct pt b)
{
	float length_a, length_b, scalar, temp;
	scalar = calc_vector_scalar_product(a,b);
	length_a = calc_vector_length(a);
	length_b = calc_vector_length(b);

	/*printf("scalar: %f  length_a: %f  length_b: %f \n", scalar, length_a, length_b);*/
	
	if (scalar == 0){
		return PI/2;	
	}

	if ( (length_a <= 0)  || (length_b <= 0)){
		printf("Divide by 0 in calc_angle_between_two_vectors():  No can do! \n");
		return 0;
	}
	
	temp = scalar /(length_a * length_b);
	/*printf("temp: %f", temp);*/

	/*acos() appears to be unable to handle 1 and -1  */
	if ((temp >= 1) || (temp <= -1)){
		return 0;
	}
	return acos(temp);
}


void normalize_vector(struct pt *vec)
{
	float vector_length;

	vector_length = calc_vector_length(*vec);
	
	vec->x = vec->x / vector_length;
	vec->y = vec->y / vector_length;
	vec->z = vec->z / vector_length;
}


/* This fuction returns a code that determines whether edges   */
/* at this point need to smoothed over or not. This code is    */
/* number from -1 to 16. These correspond to 16 possible       */
/* ways the edges can be configured at this point.             */
/* This function uses the data in the adj struct to make       */
/* these determinations. Macros would have been nice for this  */
/* , but this system has enough already.                       */
/*                                                             */
/*  0       1      2      3       0-3 are corners              */
/* ---*      |    |      *---                                  */
/*    |      |    |      |                                     */
/*    |   ---*    *---   |   s means the edge is smoothed over */
/*                                                             */
/*   4      5         6          7                             */
/*    |      |       |          s       codes 4-11 are for     */
/*    |      |       |          s       sides where one edge   */
/* ---*   sss*    ---*---       s       may or may not have    */ 
/*    |      |               ---*---    to be smoothed over    */
/*    |      |                                                 */
/*                                                             */
/*    8      9      10          11                             */
/*  |       |     ---*---     ---*---     code 16 is for error */
/*  |       |        |           s                             */
/*  *---    *sss     |           s                             */
/*  |       |        |           s                             */
/*  |       |                                                  */
/*                                                             */
/*      12       13          14         15                     */
/*       s        |          |           s                     */
/*       s        |          |           s                     */
/*    ---*---  sss*sss    ---*---     sss*sss                  */
/*       s        |          |           s                     */
/*       s        |          |           s                     */
/*                                                             */
int find_edge_config_at_this_point(struct VRML_PolyRep *rep,
				struct VRML_Extrusion_Adj *adj,
				int index_pt, float creaseAngle){
	
	float angle_north_south, angle_east_west;

	/* find out if we are in the north_east corner */
	if (	(adj[index_pt].north_pt == -1)
		&&(adj[index_pt].east_pt  == -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  != -1)){
		return 0;		
	}
	
	/* find out if we are in the south_east corner */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  == -1)
		&&(adj[index_pt].south_pt == -1)
		&&(adj[index_pt].west_pt  != -1)){
		return 1;		
	}

	/* find out if we are in the south_west corner */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt == -1)
		&&(adj[index_pt].west_pt  == -1)){
		return 2;		
	}

	/* find out if we are in the north_west corner */
	if (	(adj[index_pt].north_pt == -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  == -1)){
		return 3;		
	}

	/* find out if we are the east edge */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  == -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  != -1)){

		angle_north_south = calc_angle_between_two_vectors(
				adj[index_pt].south_vec, adj[index_pt].north_vec);
		if (angle_north_south <= creaseAngle){return 4;}
		else {return 5;}
	}

	/* find out if we are the south edge */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt == -1)
		&&(adj[index_pt].west_pt  != -1)){

		angle_east_west = calc_angle_between_two_vectors(
				adj[index_pt].west_vec, adj[index_pt].east_vec);
		if (angle_east_west <= creaseAngle){return 6;}
		else {return 7;}
	}

	/* find out if we are the west edge */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  == -1)){

		angle_north_south = calc_angle_between_two_vectors(
				adj[index_pt].south_vec, adj[index_pt].north_vec);
		if (angle_north_south <= creaseAngle){return 8;}
		else {return 9;}
	}

	/* find out if we are the north edge */
	if (	(adj[index_pt].north_pt == -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  != -1)){

		angle_east_west = calc_angle_between_two_vectors(
				adj[index_pt].west_vec, adj[index_pt].east_vec);
		if (angle_east_west <= creaseAngle){return 10;}
		else {return 11;}
	}

	/* general case */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  != -1)){

		angle_north_south = calc_angle_between_two_vectors(
				adj[index_pt].south_vec, adj[index_pt].north_vec);
		angle_east_west = calc_angle_between_two_vectors(
				adj[index_pt].west_vec, adj[index_pt].east_vec);
	
		if ( 	(angle_east_west <= creaseAngle)
			&&(angle_north_south <= creaseAngle)){return 14;}

		if ( 	(angle_east_west > creaseAngle)
			&&(angle_north_south <= creaseAngle)){return 12;}

		if ( 	(angle_east_west <= creaseAngle)
			&&(angle_north_south > creaseAngle)){return 13;}

		if ( 	(angle_east_west > creaseAngle)
			&&(angle_north_south > creaseAngle)){return 15;}
	}
/*	printf("angle_north_south: %f angle_east_west: %f \n", angle_north_south, angle_east_west);
        printf("%i   north_vec:  %f  %f  %f \n", index_pt, adj[index_pt].north_vec.x,  
				adj[index_pt].north_vec.y, adj[index_pt].north_vec.z);
        printf("%i   south_vec:  %f  %f  %f \n", index_pt, adj[index_pt].south_vec.x,  
				adj[index_pt].south_vec.y, adj[index_pt].south_vec.z);

        printf("%i   north_pt: %i\n", index_pt, adj[index_pt].north_pt);
        printf("%i   east_pt: %i\n", index_pt, adj[index_pt].east_pt);
        printf("%i   south_pt: %i\n", index_pt, adj[index_pt].south_pt);
        printf("%i   west_pt: %i\n", index_pt, adj[index_pt].west_pt);
        printf("%i   north_east_pt: %i\n", index_pt, adj[index_pt].north_east_pt);
        printf("%i   south_east_pt: %i\n", index_pt, adj[index_pt].south_east_pt);
        printf("%i   south_west_pt: %i\n", index_pt, adj[index_pt].south_west_pt);
        printf("%i   north_west_pt: %i\n", index_pt, adj[index_pt].north_west_pt);
        printf("-----------------------------------------------\n");
*/
	return 16;
}




/* This function answers with a number from 1 thru 4. It determines */
/* quadrant the triangle belongs to with reference to this point.   */
/*   4th          1st                                               */
/*       +---+---+      The triangle we are looking for is one of   */
/*       |\  |\  |      the 8.                                      */
/*       | \ | \ |                                                  */
/*       |  \|  \|                                                  */
/*       +---*---+                                                  */
/*       |\  |\  |                                                  */
/*       | \ | \ |                                                  */
/*       |  \|  \|                                                  */
/*   3rd +---+---+ 2nd                                              */
/*                                                                  */
int find_the_quadrant_of_this_triangle(struct VRML_PolyRep *rep,
				struct VRML_Extrusion_Adj *adj, 
				int index_pt, int a_pt, int b_pt){

	/*first group of cases, diagonal goes from south_east to north_west*/
	/*case where the triangle is in the first quadrant*/
	if ( 	(adj[index_pt].north_pt == a_pt ) && (adj[index_pt].east_pt == b_pt)
		||(adj[index_pt].north_pt == b_pt ) && (adj[index_pt].east_pt == a_pt)	){
		return 1;
	}
		
	/*case where the triangle is in the third quadrant*/
	if ( 	(adj[index_pt].south_pt == a_pt ) && (adj[index_pt].west_pt == b_pt)
		||(adj[index_pt].south_pt == b_pt ) && (adj[index_pt].west_pt == a_pt)	){
		return 3;
	}

	/*case where the triangle is in the second quadrant*/
	if ( 	(adj[index_pt].east_pt == a_pt ) && (adj[index_pt].south_east_pt == b_pt)
		||(adj[index_pt].east_pt == b_pt ) && (adj[index_pt].south_east_pt == a_pt)	
		||(adj[index_pt].south_pt == a_pt ) && (adj[index_pt].south_east_pt == b_pt)	
		||(adj[index_pt].south_pt == b_pt ) && (adj[index_pt].south_east_pt == a_pt)	){
		return 2;
	}

	/*case where the triangle is in the second quadrant*/
	if ( 	(adj[index_pt].west_pt == a_pt ) && (adj[index_pt].north_west_pt == b_pt)
		||(adj[index_pt].west_pt == b_pt ) && (adj[index_pt].north_west_pt == a_pt)	
		||(adj[index_pt].north_pt == a_pt ) && (adj[index_pt].north_west_pt == b_pt)	
		||(adj[index_pt].north_pt == b_pt ) && (adj[index_pt].north_west_pt == a_pt)	){
		return 4;
	}



	/*second group of cases, diagonal goes from south_west to north_east*/

	/*case where the triangle is in the first quadrant*/
	if ( 	(adj[index_pt].east_pt == a_pt ) && (adj[index_pt].north_east_pt == b_pt)
		||(adj[index_pt].east_pt == b_pt ) && (adj[index_pt].north_east_pt == a_pt)	
		||(adj[index_pt].north_pt == a_pt ) && (adj[index_pt].north_east_pt == b_pt)	
		||(adj[index_pt].north_pt == b_pt ) && (adj[index_pt].north_east_pt == a_pt)	){
		return 1;
	}

	/*case where the triangle is in the second quadrant*/
	if ( 	(adj[index_pt].south_pt == a_pt ) && (adj[index_pt].east_pt == b_pt)
		||(adj[index_pt].south_pt == b_pt ) && (adj[index_pt].east_pt == a_pt)	){
		return 2;
	}
		
	/*case where the triangle is in the third quadrant*/
	if ( 	(adj[index_pt].west_pt == a_pt ) && (adj[index_pt].south_west_pt == b_pt)
		||(adj[index_pt].west_pt == b_pt ) && (adj[index_pt].south_west_pt == a_pt)	
		||(adj[index_pt].south_pt == a_pt ) && (adj[index_pt].south_west_pt == b_pt)	
		||(adj[index_pt].south_pt == b_pt ) && (adj[index_pt].south_west_pt == a_pt)	){
		return 3;
	}

	/*case where the triangle is in the fourth quadrant*/
	if ( 	(adj[index_pt].north_pt == a_pt ) && (adj[index_pt].west_pt == b_pt)
		||(adj[index_pt].north_pt == b_pt ) && (adj[index_pt].west_pt == a_pt)	){
		return 4;
	}



			
        printf("%i   south_pt: %i\n", index_pt, adj[index_pt].south_pt);
        printf("%i   north_pt: %i\n", index_pt, adj[index_pt].north_pt);
        printf("%i   east_pt: %i\n", index_pt, adj[index_pt].east_pt);
        printf("%i   west_pt: %i\n", index_pt, adj[index_pt].west_pt);
        printf("%i   north_east_pt: %i\n", index_pt, adj[index_pt].north_east_pt);
        printf("%i   south_east_pt: %i\n", index_pt, adj[index_pt].south_east_pt);
        printf("%i   south_west_pt: %i\n", index_pt, adj[index_pt].south_west_pt);
        printf("%i   north_west_pt: %i\n", index_pt, adj[index_pt].north_west_pt);
        printf("-----------------------------------------------\n");
        printf("index_pt: %i  a_pt: %i  b_pt: %i \n", index_pt, a_pt, b_pt);
        printf("-----------------------------------------------\n");
	

	return 0;
}




void calc_poly_normals_extrusion(struct VRML_PolyRep *rep, 
			struct VRML_Extrusion_Adj *adj,
			int nspi, int nsec, int ntri, int nctri,
			float crease_angle) 
{

/* Each point in the coord array now has a corresponding entry in */
/* the adj array which specifies which points are adjacent to a   */
/* given point.  Values of -1 in the adj indicate that the point  */
/* being considered is on an edge of the extrusion.(i.e. there is */
/* no adjacent point)  In this case, only the points present are  */
/* considered */

	int   i, j, p[3], edge_config, quadrant; 
	float vector_length;
	struct pt cap_vec1, cap_vec2, cap_normal;	

        for(i=0; i < (nsec * nspi) ; i++) {

                if ( adj[i].north_pt != -1 ){
			adj[i].north_vec.x = 
				(rep->coord[ adj[i].north_pt *3+0] - rep->coord[i*3+0]);
                        adj[i].north_vec.y = 
				(rep->coord[ adj[i].north_pt *3+1] - rep->coord[i*3+1]);
                        adj[i].north_vec.z = 
				(rep->coord[ adj[i].north_pt *3+2] - rep->coord[i*3+2]);
                }
                else {
                        adj[i].north_vec.x = 0;
			adj[i].north_vec.y = 0;
			adj[i].north_vec.z = 0;
                }
/*
		printf("%i  next_layer vector: %f %f %f   \n",i, adj[i].north_vec.x, 
			adj[i].north_vec.y, adj[i].north_vec.z );
*/


                if ( adj[i].south_pt != -1 ){
                        adj[i].south_vec.x =
                                (rep->coord[ adj[i].south_pt *3+0] - rep->coord[i*3+0]);
                        adj[i].south_vec.y =
                                (rep->coord[ adj[i].south_pt *3+1] - rep->coord[i*3+1]);
                        adj[i].south_vec.z =
                                (rep->coord[ adj[i].south_pt *3+2] - rep->coord[i*3+2]);
                }
                else {
                        adj[i].south_vec.x = 0;
			adj[i].south_vec.y = 0;
			adj[i].south_vec.z = 0;
                }
/*
                printf("%i  prev_layer vector: %f %f %f   \n",i, adj[i].south_vec.x,
                        adj[i].south_vec.y, adj[i].south_vec.z );
*/


                if ( adj[i].east_pt != -1 ){
                        adj[i].east_vec.x =
                                (rep->coord[ adj[i].east_pt *3+0] - rep->coord[i*3+0]);
                        adj[i].east_vec.y =
                                (rep->coord[ adj[i].east_pt *3+1] - rep->coord[i*3+1]);
                        adj[i].east_vec.z =
                                (rep->coord[ adj[i].east_pt *3+2] - rep->coord[i*3+2]);
                }
                else {
                        adj[i].east_vec.x = 0;
			adj[i].east_vec.y = 0;
			adj[i].east_vec.z = 0;
                }
/*
                printf("%i  next_cross vector: %f %f %f   \n",i, adj[i].east_vec.x,
                        adj[i].east_vec.y, adj[i].east_vec.z );
*/


                if ( adj[i].west_pt != -1 ){
                        adj[i].west_vec.x =
                                (rep->coord[ adj[i].west_pt *3+0] - rep->coord[i*3+0]);
                        adj[i].west_vec.y =
                                (rep->coord[ adj[i].west_pt *3+1] - rep->coord[i*3+1]);
                        adj[i].west_vec.z =
                                (rep->coord[ adj[i].west_pt *3+2] - rep->coord[i*3+2]);
                }
                else {
                        adj[i].west_vec.x = 0;
			adj[i].west_vec.y = 0;
			adj[i].west_vec.z = 0;
                }
/*
                printf("%i  prev_cross vector: %f %f %f   \n",i, adj[i].west_vec.x,
                        adj[i].west_vec.y, adj[i].west_vec.z );
*/
	}/*for*/


	/*Calculate the diag-normals*/
        for(i=0; i < (nsec * nspi) ; i++) {
		if((adj[i].north_pt != -1) && (adj[i].east_pt != -1)){

			calc_vector_product(adj[i].north_vec, adj[i].east_vec, 
							&adj[i].first_quad_diag_vec);

			vector_length = calc_vector_length( adj[i].first_quad_diag_vec);

			adj[i].first_quad_diag_vec.x = (adj[i].first_quad_diag_vec.x / vector_length) ;
			adj[i].first_quad_diag_vec.y = (adj[i].first_quad_diag_vec.y / vector_length) ;
			adj[i].first_quad_diag_vec.z = (adj[i].first_quad_diag_vec.z / vector_length) ;

		}				
		else{
			adj[i].first_quad_diag_vec.x = 0;
			adj[i].first_quad_diag_vec.y = 0;
			adj[i].first_quad_diag_vec.z = 0;
		}

/*		printf("%i   first_quad_diag_vec:  %f  %f  %f  \n",i, adj[i].first_quad_diag_vec.x, 
			adj[i].first_quad_diag_vec.y, adj[i].first_quad_diag_vec.z);
*/
		if((adj[i].east_pt != -1) && (adj[i].south_pt != -1)){

			calc_vector_product(adj[i].east_vec, adj[i].south_vec,
							&adj[i].second_quad_diag_vec);

			vector_length = calc_vector_length( adj[i].second_quad_diag_vec);

			adj[i].second_quad_diag_vec.x = (adj[i].second_quad_diag_vec.x / vector_length);
			adj[i].second_quad_diag_vec.y = (adj[i].second_quad_diag_vec.y / vector_length);
			adj[i].second_quad_diag_vec.z = (adj[i].second_quad_diag_vec.z / vector_length);
		}				
		else{
			adj[i].second_quad_diag_vec.x = 0;
			adj[i].second_quad_diag_vec.y = 0;
			adj[i].second_quad_diag_vec.z = 0;
		}

/*		printf("%i   second_quad_diag_vec:  %f  %f  %f  \n",i, adj[i].second_quad_diag_vec.x, 
			adj[i].second_quad_diag_vec.y, adj[i].second_quad_diag_vec.z);
*/
		if((adj[i].south_pt != -1) && (adj[i].west_pt != -1)){

			calc_vector_product(adj[i].south_vec, adj[i].west_vec, 
									&adj[i].third_quad_diag_vec);

			vector_length = calc_vector_length( adj[i].third_quad_diag_vec);

			adj[i].third_quad_diag_vec.x = (adj[i].third_quad_diag_vec.x / vector_length);
			adj[i].third_quad_diag_vec.y = (adj[i].third_quad_diag_vec.y / vector_length);
			adj[i].third_quad_diag_vec.z = (adj[i].third_quad_diag_vec.z / vector_length);
		}				
		else{
			adj[i].third_quad_diag_vec.x = 0;
			adj[i].third_quad_diag_vec.y = 0;
			adj[i].third_quad_diag_vec.z = 0;
		}

/*		printf("%i   third_quad_diag_vec:  %f  %f  %f  \n",i, adj[i].third_quad_diag_vec.x, 
			adj[i].third_quad_diag_vec.y, adj[i].third_quad_diag_vec.z);
*/
		if((adj[i].west_pt != -1) && (adj[i].north_pt != -1)){

			calc_vector_product(adj[i].west_vec, adj[i].north_vec,
									&adj[i].fourth_quad_diag_vec);

			vector_length = calc_vector_length( adj[i].fourth_quad_diag_vec);

			adj[i].fourth_quad_diag_vec.x = (adj[i].fourth_quad_diag_vec.x / vector_length);
			adj[i].fourth_quad_diag_vec.y = (adj[i].fourth_quad_diag_vec.y / vector_length);
			adj[i].fourth_quad_diag_vec.z = (adj[i].fourth_quad_diag_vec.z / vector_length);
		}				
		else{
			adj[i].fourth_quad_diag_vec.x = 0;
			adj[i].fourth_quad_diag_vec.y = 0;
			adj[i].fourth_quad_diag_vec.z = 0;
		}

/*		printf("%i   fourth_quad_diag_vec:  %f  %f  %f  \n",i, adj[i].fourth_quad_diag_vec.x, 
			adj[i].fourth_quad_diag_vec.y, adj[i].fourth_quad_diag_vec.z);
*/
	}/*for*/


	for (i=0; i<(nsec*nspi); i++ ){
		adj[i].cumul_normal_vec.x = 	(adj[i].first_quad_diag_vec.x
						+ adj[i].second_quad_diag_vec.x
						+ adj[i].third_quad_diag_vec.x
						+ adj[i].fourth_quad_diag_vec.x) ;

		adj[i].cumul_normal_vec.y = 	(adj[i].first_quad_diag_vec.y
						+ adj[i].second_quad_diag_vec.y
						+ adj[i].third_quad_diag_vec.y
						+ adj[i].fourth_quad_diag_vec.y) ;
 
		adj[i].cumul_normal_vec.z = 	(adj[i].first_quad_diag_vec.z
						+ adj[i].second_quad_diag_vec.z
						+ adj[i].third_quad_diag_vec.z
						+ adj[i].fourth_quad_diag_vec.z) ;
		normalize_vector(&adj[i].cumul_normal_vec); 

		adj[i].north_edge_vec.x = adj[i].fourth_quad_diag_vec.x + adj[i].first_quad_diag_vec.x;	
		adj[i].north_edge_vec.y = adj[i].fourth_quad_diag_vec.y + adj[i].first_quad_diag_vec.y;	
		adj[i].north_edge_vec.z = adj[i].fourth_quad_diag_vec.z + adj[i].first_quad_diag_vec.z;	
		normalize_vector(&adj[i].north_edge_vec); 

		adj[i].east_edge_vec.x = adj[i].first_quad_diag_vec.x + adj[i].second_quad_diag_vec.x;	
		adj[i].east_edge_vec.y = adj[i].first_quad_diag_vec.y + adj[i].second_quad_diag_vec.y;	
		adj[i].east_edge_vec.z = adj[i].first_quad_diag_vec.z + adj[i].second_quad_diag_vec.z;	
		normalize_vector(&adj[i].east_edge_vec); 

		adj[i].south_edge_vec.x = adj[i].second_quad_diag_vec.x + adj[i].third_quad_diag_vec.x;	
		adj[i].south_edge_vec.y = adj[i].second_quad_diag_vec.y + adj[i].third_quad_diag_vec.y;	
		adj[i].south_edge_vec.z = adj[i].second_quad_diag_vec.z + adj[i].third_quad_diag_vec.z;	
		normalize_vector(&adj[i].south_edge_vec); 

		adj[i].west_edge_vec.x = adj[i].third_quad_diag_vec.x + adj[i].fourth_quad_diag_vec.x;	
		adj[i].west_edge_vec.y = adj[i].third_quad_diag_vec.y + adj[i].fourth_quad_diag_vec.y;	
		adj[i].west_edge_vec.z = adj[i].third_quad_diag_vec.z + adj[i].fourth_quad_diag_vec.z;	
		normalize_vector(&adj[i].west_edge_vec); 

/*		printf("%i  %f  %f  %f   \n ",i ,adj[i].cumul_normal_vec.x, 
				adj[i].cumul_normal_vec.y, adj[i].cumul_normal_vec.z );*/
	}/*for*/


	for (i=0; i<ntri; i++){
		/* Get indexes to the three points */	
		p[0] = rep->cindex[i*3+0];
		p[1] = rep->cindex[i*3+1];
		p[2] = rep->cindex[i*3+2];
		
		/* specify the normal indexes */
		rep->norindex[i*3+0] = i*3+0;  
		rep->norindex[i*3+1] = i*3+1;  
		rep->norindex[i*3+2] = i*3+2;

		/* specify the normals */

		/* Insert decision code right here */
		
		for (j=0; j<3; j++){
			
			edge_config = find_edge_config_at_this_point(rep, adj, p[j], crease_angle);
			switch(j){
				case(0):
					quadrant = find_the_quadrant_of_this_triangle(rep, adj, p[j], p[1], p[2]);
					break;
				case(1):
					quadrant = find_the_quadrant_of_this_triangle(rep, adj, p[j], p[0], p[2]);
					break;
				case(2):
					quadrant = find_the_quadrant_of_this_triangle(rep, adj, p[j], p[0], p[1]);
					break;
				default:
					printf("Error in calc_poly_normals_extrusion(): loop error \n");
					break; 
			}
			/*printf("edge_config %i ", edge_config);*/	

			switch(edge_config){

				case(0):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].third_quad_diag_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].third_quad_diag_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].third_quad_diag_vec.z;
					break;

				case(1):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].fourth_quad_diag_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].fourth_quad_diag_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].fourth_quad_diag_vec.z;
					break;

				case(2):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].first_quad_diag_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].first_quad_diag_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].first_quad_diag_vec.z;
					break;

				case(3):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].second_quad_diag_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].second_quad_diag_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].second_quad_diag_vec.z;
					break;

				case(4):
					switch(quadrant){
					case(3):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].south_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].south_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].south_edge_vec.z;
						break;
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].north_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].north_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].north_edge_vec.z;
						break;
					case(1):
					case(2):
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 4\n");
						break;
					}/*switch quadrant*/
					break;
					
				case(5):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].west_edge_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].west_edge_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].west_edge_vec.z;
					break;
					
				case(6):
					switch(quadrant){
					case(1):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].east_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].east_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].east_edge_vec.z;
						break;
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].west_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].west_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].west_edge_vec.z;
						break;
					case(2):
					case(3):
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 6\n");
						break;
					}/*switch quadrant*/
					break;

				case(7):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].north_edge_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].north_edge_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].north_edge_vec.z;
					break;

				case(8):
					switch(quadrant){
					case(1):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].north_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].north_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].north_edge_vec.z;
						break;
					case(2):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].south_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].south_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].south_edge_vec.z;
						break;
					case(3):
					case(4):
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 8\n");

						break;
					}/*switch quadrant*/
					break;

				case(9):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].east_edge_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].east_edge_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].east_edge_vec.z;
					break;

				case(10):
					switch(quadrant){
					case(2):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].east_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].east_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].east_edge_vec.z;
						break;
					case(3):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].west_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].west_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].west_edge_vec.z;
						break;
					case(1):
					case(4):
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 10\n");
						break;
					}/*switch quadrant*/
					break;

				case(11):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].south_edge_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].south_edge_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].south_edge_vec.z;
					break;

				case(12):
					switch(quadrant){
					case(1):
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].north_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].north_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].north_edge_vec.z;
						break;
					case(2):
					case(3):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].south_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].south_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].south_edge_vec.z;
						break;
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 12\n");
						break;
					}/*switch quadrant*/
					break;
				
				case(13):
					switch(quadrant){
					case(1):
					case(2):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].east_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].east_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].east_edge_vec.z;
						break;
					case(3):
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].west_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].west_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].west_edge_vec.z;
						break;
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 13\n");
						break;
					}/*switch quadrant*/
					break;

				case(14):
					switch(quadrant){
					case(1):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].first_quad_diag_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].first_quad_diag_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].first_quad_diag_vec.z;
						break;
					case(2):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].second_quad_diag_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].second_quad_diag_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].second_quad_diag_vec.z;
						break;
					case(3):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].third_quad_diag_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].third_quad_diag_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].third_quad_diag_vec.z;
						break;
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].fourth_quad_diag_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].fourth_quad_diag_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].fourth_quad_diag_vec.z;
						break;
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 14\n");
						break;
					}/*switch quadrant*/
					break;

				case(15):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].cumul_normal_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].cumul_normal_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].cumul_normal_vec.z;
					break;

				default:
					printf("Error in calc_poly_normals_extrusion(): unknown case error\n");
					break;
			}/*switch edge_config*/	

		}/*for j */

/*
		rep->normal[i*9+0] = adj[ p[0] ].cumul_normal_vec.x;
		rep->normal[i*9+1] = adj[ p[0] ].cumul_normal_vec.y;
		rep->normal[i*9+2] = adj[ p[0] ].cumul_normal_vec.z;
		  
		rep->normal[i*9+3] = adj[ p[1] ].cumul_normal_vec.x;
		rep->normal[i*9+4] = adj[ p[1] ].cumul_normal_vec.y;
		rep->normal[i*9+5] = adj[ p[1] ].cumul_normal_vec.z;
		
		rep->normal[i*9+6] = adj[ p[2] ].cumul_normal_vec.x;
		rep->normal[i*9+7] = adj[ p[2] ].cumul_normal_vec.y;
		rep->normal[i*9+8] = adj[ p[2] ].cumul_normal_vec.z;
*/

	}/*for i */


	/*calculate the normals for the endcaps*/
	for (i=ntri; i < (nctri+ntri) ; i++){
		p[0] = rep->cindex[i*3+0];
		p[1] = rep->cindex[i*3+1];
		p[2] = rep->cindex[i*3+2];

		cap_vec1.x = rep->coord[p[1]*3+0] - rep->coord[p[0]*3+0];
		cap_vec1.y = rep->coord[p[1]*3+1] - rep->coord[p[0]*3+1];
		cap_vec1.z = rep->coord[p[1]*3+2] - rep->coord[p[0]*3+2];

		cap_vec2.x = rep->coord[p[2]*3+0] - rep->coord[p[0]*3+0];
		cap_vec2.y = rep->coord[p[2]*3+1] - rep->coord[p[0]*3+1];
		cap_vec2.z = rep->coord[p[2]*3+2] - rep->coord[p[0]*3+2];

		calc_vector_product(cap_vec1, cap_vec2, &cap_normal);

		/* specify the normal indexes */
		rep->norindex[i*3+0] = i*3+0;  
		rep->norindex[i*3+1] = i*3+1;  
		rep->norindex[i*3+2] = i*3+2;

		/* specify the normals */
		rep->normal[i*9+0] = cap_normal.x;
		rep->normal[i*9+1] = cap_normal.y;
		rep->normal[i*9+2] = cap_normal.z;
		  
		rep->normal[i*9+3] = cap_normal.x;
		rep->normal[i*9+4] = cap_normal.y;
		rep->normal[i*9+5] = cap_normal.z;
		
		rep->normal[i*9+6] = cap_normal.x;
		rep->normal[i*9+7] = cap_normal.y;
		rep->normal[i*9+8] = cap_normal.z;
	}/*for*/


}/*calc_poly_normals_extrusion*/





/*********************************************************************
 *********************************************************************
 *
 * render_node : call the correct virtual functions to render the node
 * depending on what we are doing right now.
 */

void render_node(void *node) {
	struct VRML_Virt *v;
	struct VRML_Box *p;
	int srg;
	int sch;
	struct currayhit srh;
	if(verbose) printf("\nRender_node %d\n",node);
	if(!node) {return;}
	v = *(struct VRML_Virt **)node;
	p = node;
	if(verbose)
	  {
	    printf("=========================================NODE RENDERED===================================================\n");
	    printf("Render_node_v %d (%s) PREP: %d REND: %d CH: %d FIN: %d RAY: %d HYP: %d\n",v,
		   v->name, 
		   v->prep, 
		   v->rend, 
		   v->children, 
		   v->fin, 
		   v->rendray,
		   hypersensitive);
	    printf("Render_state any %d geom %d light %d sens %d\n",
		   render_anything, 
		   render_geom, 
		   render_light, 
		   render_sensitive);
	    printf ("pchange %d pichange %d vchanged %d\n",p->_change, p->_ichange,v->changed);
	  }

	if(p->_change != p->_ichange && v->changed) 
	  {
	    if (verbose) printf ("rs 1 pch %d pich %d vch %d\n",p->_change,p->_ichange,v->changed);
	    v->changed(node);
	    p->_ichange = p->_change;
	  }

	if(render_anything && v->prep) 
	  {
	    if (verbose) printf ("rs 2\n");
	    v->prep(node);
	    if(render_sensitive && !hypersensitive) 
	      {
		upd_ray();
	      }
	  }
	if(render_anything && render_geom && !render_sensitive && v->rend) 
	  {
	    if (verbose) printf ("rs 3\n");
	    v->rend(node);
	  }
	if(render_anything && render_light && v->light) 
	  {
	    if (verbose) printf ("rs 4\n");
	    v->light(node);
	  }
	/* Future optimization: when doing VP/Lights, do only 
	 * that child... further in future: could just calculate
	 * transforms myself..
	 */
	if(render_anything && render_sensitive && p->_sens) 
	  {
	    if (verbose) printf ("rs 5\n");
	    srg = render_geom;
	    render_geom = 1;
	    if(verbose) printf("CH1 %d: %d\n",node, cur_hits, p->_hit);
	    sch = cur_hits;
	    cur_hits = 0;
	    /* HP */
	      srh = rph;
	    rph.node = node;
	    glGetDoublev(GL_MODELVIEW_MATRIX, rph.modelMatrix);
	    glGetDoublev(GL_PROJECTION_MATRIX, rph.projMatrix);
	  }
	if(render_anything && render_geom && render_sensitive && !hypersensitive && v->rendray) 
	  {
	    if (verbose) printf ("rs 6\n");
	    v->rendray(node);
	  }
	if(hypersensitive == node) 
	  {
	    if (verbose) printf ("rs 7\n");
	    hyper_r1 = t_r1;
	    hyper_r2 = t_r2;
	    hyperhit = 1;
	  }
	if(render_anything && v->children) {
	    if (verbose) printf ("rs 8\n");
	  v->children(node);
	}
	if(render_anything && render_sensitive && p->_sens) 
	  {
	    if (verbose) printf ("rs 9\n");
	    render_geom = srg;
	    cur_hits = sch;
	    if(verbose) printf("CH3: %d %d\n",cur_hits, p->_hit);
	    /* HP */
	      rph = srh;
	  }
	if(render_anything && v->fin) 
	  {
	    if (verbose) printf ("rs A\n");
	    v->fin(node);
	    if(render_sensitive && v == &virt_Transform) 
	      { 
		upd_ray();
	      }
	  }
	if (verbose) printf("(end render_node)\n");
}
#undef verbose

/*
 * The following code handles keeping track of the parents of a given
 * node. This enables us to traverse the scene on C level for optimizations.
 *
 * We use this array code because VRML nodes usually don't have
 * hundreds of children and don't usually shuffle them too much.
 */

#define NODE_ADD_PARENT(a) add_parent(a,ptr)

void add_parent(void *node_, void *parent_) {
	struct VRML_Box *node = node_;
	struct VRML_Box *parent = parent_;
	if(!node) return;
	node->_nparents ++;
	if(node->_nparents > node->_nparalloc) {
		node->_nparalloc += 10;
		node->_parents = 
		 (node->_parents ? 
			realloc(node->_parents, sizeof(node->_parents[0])*
							node->_nparalloc) 
							:
			malloc(sizeof(node->_parents[0])* node->_nparalloc) 
		 );
	}
	node->_parents[node->_nparents-1] = parent_;
}

#define NODE_REMOVE_PARENT(a) add_parent(a,ptr)

void remove_parent(void *node_, void *parent_) {
	struct VRML_Box *node = node_;
	struct VRML_Box *parent = parent_;
	int i;
	if(!node) return;
	node->_nparents --;
	for(i=0; i<node->_nparents; i++) {
		if(node->_parents[i] == parent) {
			break;
		}
	}
	for(; i<node->_nparents; i++) {
		node->_parents[i] = node->_parents[i+1];
	}
}

/*
 * General Texture objects
 */

do_texture(depth,x,y,ptr,Sgl_rep_or_clamp, Tgl_rep_or_clamp,Image)
	int x,y,depth;
	GLint Sgl_rep_or_clamp;
	GLint Tgl_rep_or_clamp;
	GLint Image;
	unsigned char *ptr;


{

	GLint texSize;
	int rx,ry,sx,sy;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	/* Image should be GL_LINEAR for pictures, GL_NEAREST for pixelTs */
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Image );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Image );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Sgl_rep_or_clamp);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Tgl_rep_or_clamp);

	/* find out the largest size that a texture can have. */
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);

	if((depth) && x && y) {
		unsigned char *dest = ptr;
		rx = 1; sx = x;
		while(sx) {sx /= 2; rx *= 2;}
		if(rx/2 == x) {rx /= 2;}
		ry = 1; sy = y;
		while(sy) {sy /= 2; ry *= 2;}
		if(ry/2 == y) {ry /= 2;}
		if(rx != x || ry != y || rx > texSize || ry > texSize) {
			/* do we have texture limits??? */
			if (rx > texSize) rx = texSize;
			if (ry > texSize) ry = texSize;

			/* We have to scale */
			/* printf ("Do_Texture scaling from rx %d ry %d to x %d y %d\n",x,y,rx,ry);  */
			dest = malloc((depth) * rx * ry);
			gluScaleImage(
			     ((depth)==1 ? GL_LUMINANCE : 
			     ((depth)==2 ? GL_LUMINANCE_ALPHA : 
			     ((depth)==3 ? GL_RGB : 
			     GL_RGBA))),
			     x, y, GL_UNSIGNED_BYTE, ptr, rx, ry, 
			     GL_UNSIGNED_BYTE, dest);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, depth,  rx, ry, 0,
			     ((depth)==1 ? GL_LUMINANCE : 
			     ((depth)==2 ? GL_LUMINANCE_ALPHA : 
			     ((depth)==3 ? GL_RGB : 
			     GL_RGBA))),
			     GL_UNSIGNED_BYTE, dest);
		printf ("do_texture idest %x ptr %x sz %d\n",dest, ptr, depth *rx *ry);
		if(ptr != dest) free(dest);
	}
}


MODULE = VRML::VRMLFunc PACKAGE = VRML::VRMLFunc

PROTOTYPES: ENABLE

void *
alloc_struct(siz,virt)
	int siz
	void *virt
CODE:
	void *ptr = malloc(siz);
	struct VRML_Box *p = ptr;
	if(verbose) printf("Alloc: %d %d -> %d\n", siz, virt, ptr); 
	*(struct VRML_Virt **)ptr = (struct VRML_Virt *)virt;
	p->_sens = p->_hit = 0;
	p->_intern = 0;
	p->_change = 153;
	p->_dlchange = 0;
	p->_dlist = 0;
	p->_dl2change = 0;
	p->_dl2ist = 0;
        p->_parents = 0;
        p->_nparents = 0;
        p->_nparalloc = 0;
	p->_ichange = 0;
	p->_texture = 0;
	p->_myshape = last_visited_shape; /* This is for display list regeneration */
	RETVAL=ptr;
OUTPUT:
	RETVAL

void
release_struct(ptr)
	void *ptr
CODE:
	struct VRML_Box *p = ptr;
	if(p->_parents) free(p->_parents);
	if(p->_dlist) glDeleteLists(p->_dlist,1);
	if(p->_dl2ist) glDeleteLists(p->_dl2ist,1);
	if(p->_texture) glDeleteTextures(1,&p->_texture);
	free(ptr); /* COULD BE MEMLEAK IF STUFF LEFT INSIDE */

void
set_sensitive(ptr,sens)
	void *ptr
	int sens
CODE:
	/* Choose box randomly */
	struct VRML_Box *p = ptr;
	p->_sens = sens;

void 
set_hypersensitive(ptr)
	void *ptr
CODE:	
	hypersensitive = ptr;
	hyperhit = 0;

int
get_hyperhit(x1,y1,z1,x2,y2,z2)
	double x1
	double y1
	double z1
	double x2
	double y2
	double z2
CODE:
	GLdouble projMatrix[16];
	/*
	if(hyperhit) {
		x1 = hyper_r1.x;
		y1 = hyper_r1.y;
		z1 = hyper_r1.z;
		x2 = hyper_r2.x;
		y2 = hyper_r2.y;
		z2 = hyper_r2.z;
		RETVAL=1;
	} else RETVAL = 0;
	*/
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(r1.x, r1.y, r1.z, rhhyper.modelMatrix,
		projMatrix, viewport, &x1, &y1, &z1);
	gluUnProject(r2.x, r2.y, r2.z, rhhyper.modelMatrix,
		projMatrix, viewport, &x2, &y2, &z2);
	RETVAL=1;
OUTPUT:
	RETVAL
	x1
	y1
	z1
	x2
	y2
	z2
	

int
get_hits(ptr)
	void *ptr
CODE:
	struct VRML_Box *p = ptr;
	RETVAL = p->_hit;
	p->_hit = 0;
OUTPUT:
	RETVAL

void
zero_hits(ptr)
	void *ptr
CODE:
	struct VRML_Box *p = ptr;
	p->_hit = 0;

void 
render_verbose(i)
	int i;
CODE:
	verbose=i;

void
render_geom(p)
	void *p
CODE:
	struct VRML_Virt *v;
	if(!p) {
		die("Render_geom null!??");
	}
	v = *(struct VRML_Virt **)p;
	v->rend(p);

void 
render_hier(p,revt,rvp,rgeom,rlight,rsens,rblend,wvp)
	void *p
	int revt
	int rvp
	int rgeom
	int rlight
	int rsens
  	int rblend
	void *wvp
CODE:
	reverse_trans = revt;
	render_vp = rvp;
	render_geom =  rgeom;
	render_light = rlight;
	render_sensitive = rsens;
	render_blend = rblend;
	curlight = 0;
	what_vp = wvp;
	render_anything = 1;
	hpdist = -1;
	if(!p) {
		die("Render_hier null!??");
	}
	if(verbose)
  		printf("Render_hier rev_trans=%d vp=%d geom=%d light=%d sens=%d blend=%d what_vp=%d\n", p, revt, rvp, rgeom, rlight, rblend, wvp);

	if(render_sensitive) 
		upd_ray();
	render_node(p);

	/* Get raycasting results */
	if(render_sensitive) {
		if(hpdist >= 0) {
			if(verbose) printf("RAY HIT!\n");
		}
	}

void *
get_rayhit(x,y,z,nx,ny,nz,tx,ty)
	double x
	double y
	double z
	double nx
	double ny
	double nz
	double tx
	double ty
CODE:
	if(hpdist >= 0) {
		gluUnProject(hp.x,hp.y,hp.z,rh.modelMatrix,rh.projMatrix,viewport,&x,&y,&z);
		RETVAL = rh.node;
	} else {
		RETVAL=0;
	}
OUTPUT:
	RETVAL
	x
	y
	z
	nx
	ny
	nz
	tx
	ty

void 
get_proximitysensor_vecs(node,hit,x1,y1,z1,x2,y2,z2,q2)
	void *node
	int hit
	double x1
	double y1
	double z1
	double x2
	double y2
	double z2
	double q2
CODE:
	struct VRML_ProximitySensor *px = node;
	hit = px->__hit;
	px->__hit = 0;
	x1 = px->__t1.c[0];
	y1 = px->__t1.c[1];
	z1 = px->__t1.c[2];
	x2 = px->__t2.r[0];
	y2 = px->__t2.r[1];
	z2 = px->__t2.r[2];
	q2 = px->__t2.r[3];
	/* printf ("get_proximitysensor_vecs, x %f y %f z %f, x2 %f y2 %f z2 %f q2 %f\n", 
			x1,y1,z1,x2,y2,z2,q2);  */
OUTPUT:
	hit
	x1
	y1
	z1
	x2
	y2
	z2
	q2

void
set_divs(horiz,vert)
int horiz
int vert
CODE:
	horiz_div = horiz;
	vert_div = vert;

void
set_smooth_normals(flag)
int flag
CODE:
	smooth_normals = flag;

void
set_vpdist(dist)
int dist
CODE:
	vp_dist = dist;

ENDHERE
;
	print XS join '',@xsfn;
	print XS '

BOOT:
	I_OPENGL;
	new_tessellation();
	
';

	open PM, ">VRMLFunc.pm";
	print PM "
# VRMLFunc.pm, generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD
package VRML::VRMLFunc;
require DynaLoader;
\@ISA=DynaLoader;
bootstrap VRML::VRMLFunc;
sub load_data {
	my \$n = \\\%VRML::CNodes;
";
	print PM join '',@poffsfn;
	print PM "
}
";
}


gen();


