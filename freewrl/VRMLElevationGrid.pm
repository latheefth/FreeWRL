# Copyright (C) 1998 Bernhard Reiter and Tuomas J. Lukka
# Copyright (C) 2000 2002 John Stewart, CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

'

int x,z;
int nx = $f(xDimension);
float xs = $f(xSpacing);
int nz = $f(zDimension);
float zs = $f(zSpacing);
float *f = $f(height);
float a[3],b[3];
int *cindex; 
float *coord;
float *tcoord;
int *colindex;
int ntri = (nx && nz ? 2 * (nx-1) * (nz-1) : 0);
int vertex_ind;
int nf = $f_n(height);
int cpv = $f(colorPerVertex);
struct SFColor *colors; 
int ncolors=0;
struct VRML_PolyRep *rep_ = this_->_intern;
float creaseAngle = $f(creaseAngle);
int npv = $f(normalPerVertex);
struct SFColor *normals;
int nnormals = 0;
int ccw = $f(ccw);

struct SFVec2f *texcoords;
int ntexcoords = 0;

/* variables for calculating smooth normals */
int 	HAVETOSMOOTH;
struct 	pt *facenormals = 0;
int	*pointfaces = 0;
int 	faces;
int	this_face = 0;
int	tmp;
int 	tmp_polygon;
float point_normal[3];
int 	calc_normind;

int nquadpercol, nquadperrow, nquadcol, nquadrow;

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
	



$fv_null(color, colors, get3, &ncolors);
$fv_null(normal, normals, get3, &nnormals);
$fv_null(texCoord, texcoords, get2, &ntexcoords);

/* Render_Polyrep will use this number of triangles */
rep_->ntri = ntri;

/* ccw or not? */
rep_->ccw = 1;

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
	colindex = rep_->colindex = malloc(sizeof(*(rep_->colindex))*3*(ntri));
	if (!(colindex)) { 
		die("Not enough memory for ElevationGrid node color index ");
	}
}

if (HAVETODOTEXTURES) {
	/* so, we now have to worry about textures. */
	tcoord = rep_->tcoord = malloc(sizeof(*(rep_->tcoord))*nx*nz*3);
	if (!(tcoord)) die ("Not enough memory ElevGrid Tcoords");

	rep_->tcindex = 0; // we will generate our own mapping
	/* do we have to generate a default texture map?? */
	if ((ntexcoords > 0) && (ntexcoords < (nx*nz))) {
		printf ("too few TextureCoordinates for ElevationGrid, expect %d have %d\n",
			nx*nz, ntexcoords);
		ntexcoords = 0; // set it to zero, so we calculate them
	}
}




/* Coords, CoordIndexes, and Normals are needed all the time */
cindex = rep_->cindex = malloc(sizeof(*(rep_->cindex))*3*(ntri));
coord = rep_->coord = malloc(sizeof(*(rep_->coord))*nx*nz*3);
rep_->norindex = malloc(sizeof(*(rep_->norindex))*3*ntri);	
if(!(cindex && coord && rep_->norindex)) {
	die("Not enough memory for ElevationGrid node triangles... ;(");
} 

/* we are calculating Normals */
if (nnormals == 0) {
	rep_->normal = malloc(sizeof(*(rep_->normal))*3*ntri*3);
	if (!(rep_->normal)) {
		die("Not enough memory for ElevationGrid node normals");
	}
}


/* determine if we need to smooth out norms */ 
initialize_smooth_normals();
HAVETOSMOOTH = smooth_normals && (nnormals == 0) && (fabs(creaseAngle) > 0.00001);

if (nnormals == 0) {
	faces = (nx-1) * (nz-1) * 2; /* we treat each triangle as a face = makes really nice normals */ 
	
	/* for each face, we can look in this structure and find the normal */
	facenormals = malloc(sizeof(*facenormals)*faces);
	
	/* for each point, tell us which faces it is in, first index is  the face count */
	pointfaces = malloc(sizeof(*pointfaces)*nz*nx*POINT_FACES);
	for (tmp=0; tmp<nz*nx; tmp++) { pointfaces[tmp*POINT_FACES]=0; }

	if (!(pointfaces && facenormals)) {
		die("Not enough memory for ElevationGrid node normal point calcs... ");
	}
}

/* Prepare the coordinates */
/*   NOTE: if this is changed, collision detection might not work. check Collision.c:elevationgrid_disp() */
for(z=0; z<nz; z++) {
	for(x=0; x<nx; x++) {
		float h = f[x+z*nx];
		coord[(x+z*nx)*3+0] = x*xs;
		coord[(x+z*nx)*3+1] = h;
		coord[(x+z*nx)*3+2] = z*zs;
		if (HAVETODOTEXTURES) {
			tcoord[(x+z*nx)*3+1] = 0;
			if (ntexcoords > 0) {
				// TextureCoordinate passed in
				tcoord[(x+z*nx)*3+0] = texcoords[x+z*nx].c[0];
				tcoord[(x+z*nx)*3+2] = texcoords[x+z*nx].c[1];
			} else {
				tcoord[(x+z*nx)*3+0] = (float) x/(nx-1);
				tcoord[(x+z*nx)*3+2] = (float) z/(nz-1);
			}
			//printf ("EV TC %d %d %f %f\n",
			//	z,x,tcoord[(x+z*nx)*3+0], tcoord[(x+z*nx)*3+2] );
		}
	}
}

/* set the indices to the coordinates		*/
/*   NOTE: if this is changed, collision detection might not work. check Collision.c:elevationgrid_disp() */
vertex_ind = 0;
for(z=0; z<nz-1; z++) {
	for(x=0; x<nx-1; x++) {
		A=x+z*nx;
		B=(x+1)+z*nx;
		C=(x+1)+(z+1)*nx;
		D=x+(z+1)*nx;
		/* calculate the distance A-C and see, 
			if it is smaller as B-D        			*/
		VEC_FROM_COORDDIFF(coord,C,coord,A,ac);
		VEC_FROM_COORDDIFF(coord,D,coord,B,bd);


		if(sqrt(VECSQ(ac))>sqrt(VECSQ(bd))) { E=B; F=D; 
		} else { E=C; F=A;}

		/* first triangle: */
		Elev_Tri(vertex_ind,this_face,A,D,E,(nnormals==0), 
			rep_,facenormals,pointfaces,ccw);
		vertex_ind += 3;
		this_face++;

		/* second triangle: */
		Elev_Tri(vertex_ind,this_face,C,B,F,(nnormals==0),
			rep_,facenormals,pointfaces,ccw);
		vertex_ind += 3;
		this_face++;
	}
}

/* Normals */
if (nnormals == 0) {
	calc_normind = 0;
	for (x=0; x<vertex_ind; x++) {
		this_face = x/3;
	
		/* if we have to calculate normals */
		if (HAVETOSMOOTH) {
			/* normalize each vertex */
			normalize_ifs_face (&rep_->normal[calc_normind*3],
				facenormals, pointfaces, cindex[x],
				this_face, creaseAngle);
			rep_->norindex[x] = calc_normind++;
		} else {
			rep_->normal[x*3+0]=facenormals[this_face].x;
			rep_->normal[x*3+1]=facenormals[this_face].y;
			rep_->normal[x*3+2]=facenormals[this_face].z;
			rep_->norindex[x] = x; 
		}
	}
} else {
	/* we have been supplied normals */
	if(npv){
		/*normal per vertex*/
		for (x=0; x<ntri; x++){
			rep_->norindex[x*3+0] = rep_->cindex[x*3+0];
			rep_->norindex[x*3+1] = rep_->cindex[x*3+1];
			rep_->norindex[x*3+2] = rep_->cindex[x*3+2];
		}
	} else {
		/*normal per quad*/
		for (x=0; x < vertex_ind; x++){
			/* supplied norms/face- face has 3 points, 2 tris... */
			rep_->norindex[x] = x/6;
		}
	}
}

/* ColoUrs */
if (ncolors) {
	/* we have been supplied colors */
	if(cpv){
		/*colour per vertex*/
		for (x=0; x<ntri; x++){
			rep_->colindex[x*3+0] = rep_->cindex[x*3+0];
			rep_->colindex[x*3+1] = rep_->cindex[x*3+1];
			rep_->colindex[x*3+2] = rep_->cindex[x*3+2];
		}
	} else {
		/*colour per quad*/
		for (x=0; x < vertex_ind; x++){
			/* supplied colours/face- face has 3 points, 2 tris... */
			rep_->colindex[x] = x/6;
		}
	}
}

if (nnormals == 0) {
	free (facenormals);
	free (pointfaces);
}

';

