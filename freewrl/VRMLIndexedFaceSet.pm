# Copyright (C) 1998 Bernhard Reiter and Tuomas J. Lukka
# Copyright (C) 2000, 2002 John Stewart, CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# This is the IndexedFaceSet -> polyrep code, used by VRMLC.pm to generate
# VRMLFunc.xs &c.


'

int cin = $f_n(coordIndex);
int cpv = $f(colorPerVertex);
int npv = $f(normalPerVertex);
int tcin = $f_n(texCoordIndex);
int colin = $f_n(colorIndex); 
int norin = $f_n(normalIndex);
float creaseAngle = $f(creaseAngle);
int ccw = $f(ccw);

int curpoly;
int ntri = 0;
int nvert = 0;
int npoints = 0;
int nnormals=0;
int ncolors=0;
int ntexCoords = 0;
int vert_ind = 0;	
int calc_normind = 0;

/* flags for errors */
int ntexerrors = 0;

float a[3]; float b[3];
struct SFColor *c1,*c2,*c3;
struct SFColor *points; 
struct SFVec2f *texCoords; 
struct VRML_PolyRep *rep_ = this_->_intern;
struct SFColor *normals;
struct SFColor *colors;

int *cindex;		/* Coordinate Index	*/
int *colindex;		/* Color Index		*/
int *tcindex;		/* Tex Coord Index	*/
int *norindex;		/* Normals Index	*/

int faces=0;
int pointctr;
int facectr;
int max_points_per_face = 0;
int min_points_per_face = 99999;
struct pt *facenormals;
int	*pointfaces;

GLdouble tess_v[3];             /*param.to gluTessVertex()*/
int *tess_vs;              /* pointer to space needed */


int i,j;	/* general purpose counters */
int tmp_a, tmp_b;
int this_face, this_coord, this_normal, this_normalindex;



/* if the last coordIndex == -1, ignore it */
if($f(coordIndex,cin-1) == -1) { cin--; }
	

/* texture coords IndexedFaceSet coords colors and normals */
$fv_null(texCoord, texCoords, get2, &ntexCoords);
$fv(coord, points, get3, &npoints);
$fv_null(normal, normals, get3, &nnormals); 
$fv_null(color, colors, get3, &ncolors); 

/************************************************************************
Rules from the spec:

 If the texCoord field is not NULL, it shall contain a TextureCoordinate node. 
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
*****************************************************************************/


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



/* Once per freewrl Invocation, the smooth_normals flag is initialized */
initialize_smooth_normals();
if (!smooth_normals){
	creaseAngle = 0.0;  /* trick following code into doing things quick */
}

/* count the faces in this polyrep and allocate memory. */
faces = count_IFS_faces (cin,this_);
facenormals = malloc(sizeof(*facenormals)*faces);
pointfaces = malloc(sizeof(*pointfaces)*npoints*POINT_FACES); /* save max x points */

/* in C always check if you got the mem you wanted...  >;->		*/
if(!(pointfaces && facenormals )) {
	die("Not enough memory for IndexedFaceSet internals... ;(");
} 

/* generate the face-normals table, so for each face, we know the normal 
   and for each point, we know the faces that it is in */

IFS_face_normals (facenormals,pointfaces,faces,npoints,cin,points,this_);

/* wander through to see how much memory needs allocating for triangles */
for(i=0; i<cin; i++) {
	if($f(coordIndex,i) == -1) {
		if(nvert < 3) {
			die("Too few vertices in indexedfaceset poly");
		} 
                        if(tcin > 0  && $f(texCoordIndex,i) != -1) {
			/* Rule F part 2 see above */
                                printf ("IndexedFaceSet, Rule F, part 2: coordIndex[%d] = -1 => expect texCoordIndex[%d] = -1 (but is %d)\\n", i, i, $f(texCoordIndex,i));
			ntexerrors = 1;
                        }
		ntri += nvert-2;
		nvert = 0;
	} else {
		if (tcin > 0 && $f(texCoordIndex,i) >= ntexCoords) {
			/* Rule F, part 3 see above */
			printf ("IndexedFaceSet, Rule F, part 3: TexCoordIndex[%d] %d is greater than num texCoord (%d)\n",i, $f(texCoordIndex,i),
				ntexCoords);
			ntexerrors = 1;
		}
		nvert ++;
	}
}
if(nvert>2) {ntri += nvert-2;}

/* Tesselation MAY use more triangles; lets estimate how many more */
if(!$f(convex)) { ntri =ntri*2; }

cindex = rep_->cindex = malloc(sizeof(*(rep_->cindex))*3*(ntri));
colindex = rep_->colindex = malloc(sizeof(*(rep_->colindex))*3*(ntri));
norindex = rep_->norindex = malloc(sizeof(*(rep_->norindex))*3*ntri);

/* if we calculate normals, we use a normal per point, NOT per triangle */ 
if (!nnormals) {  		/* 3 vertexes per triangle, and 3 points per tri */
	rep_->normal = malloc(sizeof(*(rep_->normal))*3*3*ntri);
} else { 			/* dont do much, but get past check below */
	rep_->normal = malloc(1);
}


if (ntexerrors == 0) {
	if ((ntexCoords) && (HAVETODOTEXTURES)) {
		tcindex = rep_->tcindex = malloc(sizeof(*(rep_->tcindex))*3*(ntri));
	}
} else {
	ntexCoords = 0; tcin = 0; 
	printf ("killing textures %x\n",this_->texCoord);
}

/* in C always check if you got the mem you wanted...  >;->		*/
if(!(cindex && colindex && norindex && rep_->normal )) {
	die("Not enough memory for IndexFaceSet node triangles... ;(");
} 

if (!(tcindex) && HAVETODOTEXTURES && ntexCoords) {
	die("Not enough memory for IndexFaceSet textures... ;(");
} 



/* Concave faces - use the OpenGL Triangulator to give us the triangles */
tess_vs=malloc(sizeof(*(tess_vs))*(ntri)*3);

this_coord = 0;
this_normal = 0;
this_normalindex = 0;
i = 0;
for (this_face=0; this_face<faces; this_face++) {
	int relative_coord;		/* temp, used if not tesselating	*/
	int initind, lastind;  		/* coord indexes 			*/
	int initnori, lastnori;		/* normalIndex indexes			*/

	global_IFS_Coord_count = 0;
	relative_coord = 0;

	//printf ("working on face %d coord %d total coords %d coordIndex %d\n",
	//	this_face,this_coord,cin,$f(coordIndex, this_coord)); 

	/* create the global_IFS_coords array, at least this time 	*/
	/*								*/
	/* What we do is to create a series of triangle vertex 		*/
	/* relative to the current coord index, then use that		*/
	/* to generate the actual coords further down. This helps	*/
	/* to map normals, textures, etc when tesselated and the	*/
	/*  *perVertex modes are set.					*/

	/* If we have concave, tesselate! */
	if (!$f(convex)) { 
		gluBeginPolygon(global_tessobj); 
	} else {
		initind = relative_coord++;
		lastind = relative_coord++;
	}

	i = $f(coordIndex, relative_coord + this_coord);

	while (i != -1) {
		if (!$f(convex)) {
			// printf ("\nwhile, i is %d this_coord %d rel coord %d\n",i,this_coord,relative_coord); 
			c1 = &(points[i]);
			tess_v[0] = c1->c[0];
			tess_v[1] = c1->c[1];
			tess_v[2] = c1->c[2];
			tess_vs[relative_coord] = relative_coord;
			gluTessVertex(global_tessobj,tess_v,&tess_vs[relative_coord++]);
		} else {
			/* take coordinates and make triangles out of them */
			global_IFS_Coords[global_IFS_Coord_count++] = initind;
			global_IFS_Coords[global_IFS_Coord_count++] = lastind;
			global_IFS_Coords[global_IFS_Coord_count++] = relative_coord;
			// printf ("triangle %d %d %d\n",initind,lastind,relative_coord);
			lastind = relative_coord++;
		}

		if (relative_coord + this_coord == cin) { 
			i = -1; 
		} else {
			i = $f(coordIndex, relative_coord + this_coord);
		}
	}

	if (!$f(convex)) { 
		gluEndPolygon(global_tessobj); 
	
		/* Tesselated faces may have a different normal than calculated previously */
		IFS_check_normal (facenormals,this_face,points, this_coord, this_); 
	}


	/* now store this information for the whole of the polyrep */
	for (i=0; i<global_IFS_Coord_count; i++) {

		/* Triangle Coordinate */
		cindex [vert_ind] = $f(coordIndex,this_coord+global_IFS_Coords[i]);

		//printf ("vertex  %d  gic %d cindex %d\n",vert_ind,global_IFS_Coords[i],cindex[vert_ind]); 

		/* Vertex Normal */
		if(nnormals) {
			if (norin) {
				/* we have a NormalIndex */
				if (npv) {
					norindex[vert_ind] = $f(normalIndex,this_coord+global_IFS_Coords[i]);
					// printf ("norm1, index %d\n",norindex[vert_ind]);
				} else {
					norindex[vert_ind] = $f(normalIndex,this_face);
					// printf ("norm2, index %d\n",norindex[vert_ind]);
				}
			} else {
				/* no normalIndex  - use the coordIndex */
				if (npv) {
					norindex[vert_ind] = $f(coordIndex,this_coord+global_IFS_Coords[i]);
					//printf ("norm3, index %d\n",norindex[vert_ind]);
				} else {
					norindex[vert_ind] = this_face;
					//printf ("norm4, index %d\n",norindex[vert_ind]);
				}
			}

		} else { 
			if (fabs(creaseAngle) > 0.00001) {
				/* normalize each vertex */
				normalize_ifs_face (&rep_->normal[calc_normind*3],
					facenormals, pointfaces, cindex[vert_ind],
					this_face, creaseAngle);
				rep_->norindex[vert_ind] = calc_normind++;
			} else {
				/* use the calculated normals */
				rep_->normal[vert_ind*3+0]=facenormals[this_face].x;
				rep_->normal[vert_ind*3+1]=facenormals[this_face].y;
				rep_->normal[vert_ind*3+2]=facenormals[this_face].z;
				rep_->norindex[vert_ind] = vert_ind;
			}
		}

		/* Vertex Colours */
		if(ncolors) {
			if (colin) {
				/* we have a colorIndex */
				if (cpv) {
					colindex[vert_ind] = $f(colorIndex,this_coord+global_IFS_Coords[i]);
					//printf ("col1, index %d\n",colindex[vert_ind]);
				} else {
					colindex[vert_ind] = $f(colorIndex,this_face);
					// printf ("col2, index %d\n",colindex[vert_ind]);
				}
			} else {
				/* no colorIndex  - use the coordIndex */
				if (cpv) {
					colindex[vert_ind] = $f(coordIndex,this_coord+global_IFS_Coords[i]);
					// printf ("col3, index %d\n",colindex[vert_ind]);
				} else {
					colindex[vert_ind] = this_face;
					// printf ("col4, index %d\n",colindex[vert_ind]);
				}
			}
		}


		/* Texture Coordinates */
		if ((ntexCoords) && (HAVETODOTEXTURES)) {
			if (tcin) {
				tcindex[vert_ind] = $f(texCoordIndex,this_coord+global_IFS_Coords[i]);
				//printf ("ntexCoords,tcin,  index %d\n",tcindex[vert_ind]);
			} else {
				/* no texCoordIndex, use the Coord Index */
				tcindex[vert_ind] = $f(coordIndex,this_coord+global_IFS_Coords[i]);
				//printf ("ntexcoords, notcin, vertex %d point %d\n",vert_ind,tcindex[vert_ind]);
			}
		}

		vert_ind++;
	}

	/* for the next face, we work from a new base */
	this_coord += relative_coord;
	if ($f(coordIndex,this_coord) == -1) {this_coord++;}
}

/* we have an accurate triangle count now... */
rep_->ntri = vert_ind/3;

free (tess_vs);
free (facenormals); 
free (pointfaces);

';
