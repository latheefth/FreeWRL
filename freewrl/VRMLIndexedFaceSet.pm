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

int curpoly;
int ntri = 0;
int nvert = 0;
int npoints = 0;
int nnormals=0;
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
	

/* texture coords IndexedFaceSet coords and normals */
$fv_null(texCoord, texCoords, get2, &ntexCoords);
$fv(coord, points, get3, &npoints);
$fv_null(normal, normals, get3, &nnormals); 

	
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


if (ntexerrors == 0)
	tcindex = rep_->tcindex = malloc(sizeof(*(rep_->tcindex))*3*(ntri));
else {
	ntexCoords = 0; tcin = 0; 
	printf ("killing textures %x\n",this_->texCoord);
}

/* in C always check if you got the mem you wanted...  >;->		*/
if(!(cindex && colindex && tcindex && norindex && rep_->normal )) {
	die("Not enough memory for IndexFaceSet node triangles... ;(");
} 


/* Concave faces - use the OpenGL Triangulator to give us the triangles */
tess_vs=malloc(sizeof(*(tess_vs))*(ntri)*3);

/* Tell the tesselator that we are doing an IndexedFaceSet */
tesselize_ifs();

this_coord = 0;
this_normal = 0;
this_normalindex = 0;
i = 0;
for (this_face=0; this_face<faces; this_face++) {
	int my_coord;			/* temp, used if not tesselating	*/
	int initind, lastind;  		/* coord indexes 			*/
	int initnori, lastnori;		/* normalIndex indexes			*/
	int my_IFS_Coord_count = 0;
	int my_IFS_normal_count = 0;
	int my_normalindexes[cin*3];

	global_IFS_Coord_count = 0;

	/* printf ("working on face %d coord %d total coords %d coordIndex %d\n",
		this_face,this_coord,cin,$f(coordIndex, this_coord)); */

	/* create the global_IFS_coords array, at least this time */
	my_coord = this_coord;
	initind  = $f(coordIndex, my_coord++);
	lastind  = $f(coordIndex, my_coord++);
	
	/* keep track of normal indexes */
	if (nnormals) { 
		/* start off this face properly */
		if (npv) {
			/* Normals per Vertex */
			if (norin) {
				initnori = $f(normalIndex, this_normal++);
				lastnori = $f(normalIndex, this_normal++);
			} else {
				initnori = this_normal++;
				lastnori = this_normal++;
			}
		} else {
			/* increment normal per face  works with or without normalIndex */
			if (this_face > 0) this_normal++;
			if (this_normal >= nnormals) {
				printf ("Warning: not enough normals in this IFS\n");
				this_normal = 0;
			}
		}
	}

	while (i != -1) {
		/* take coordinates and make triangles out of them */
		global_IFS_Coords[my_IFS_Coord_count++] = initind;
		global_IFS_Coords[my_IFS_Coord_count++] = lastind;
		global_IFS_Coords[my_IFS_Coord_count++] = $f(coordIndex,my_coord);
		lastind = $f(coordIndex,my_coord++);

		/* if we have a normal index, keep it in step with triangles */
		if (nnormals) {
			if (norin) {
				/* we have a NormalIndex */
				if (npv) {
					my_normalindexes[my_IFS_normal_count++] = initnori;
					my_normalindexes[my_IFS_normal_count++] = lastnori;
					my_normalindexes[my_IFS_normal_count++] = $f(normalIndex,this_normal);
					lastnori = $f(normalIndex,this_normal++);
				} else {
					my_normalindexes[my_IFS_normal_count++] = $f(normalIndex,this_normal);
					my_normalindexes[my_IFS_normal_count++] = $f(normalIndex,this_normal);
					my_normalindexes[my_IFS_normal_count++] = $f(normalIndex,this_normal);
				}
			} else {
				/* no normalIndex */
				if (npv) {
					my_normalindexes[my_IFS_normal_count++] = initnori++;
					my_normalindexes[my_IFS_normal_count++] = lastnori++;
					my_normalindexes[my_IFS_normal_count++] = this_normal;
					lastnori = this_normal++;
				} else {
					my_normalindexes[my_IFS_normal_count++] = this_normal;
					my_normalindexes[my_IFS_normal_count++] = this_normal;
					my_normalindexes[my_IFS_normal_count++] = this_normal;
				}
			}
		}

		if (my_coord == cin) {
			i = -1;
		} else {
			i = $f(coordIndex, my_coord);
		}
	}


	/* If we have concave, and we have more than 3 points, tesselate! */
	if ((!$f(convex)) && (my_IFS_Coord_count >= 3)) {
		gluBeginPolygon(global_tessobj);
	
		i = $f(coordIndex, this_coord);
		while (i != -1) {
			/* printf ("\nwhile, i is %d this_coord %d\n",i,this_coord); */
			c1 = &(points[i]);
			tess_v[0] = c1->c[0];
			tess_v[1] = c1->c[1];
			tess_v[2] = c1->c[2];
			tess_vs[this_coord] = i;
	
			/* printf ("  coord for i %d is %d\n",this_coord,$f(coordIndex, this_coord));
			printf ("  vals are %f %f %f\n",tess_v[0],tess_v[1],tess_v[2]); */
			gluTessVertex(global_tessobj,tess_v,&tess_vs[this_coord]);
			this_coord ++;
			if (this_coord == cin) { 
				i = -1; 
			} else {
				i = $f(coordIndex, this_coord);
			}
				
		}
		gluEndPolygon(global_tessobj);
	
	
		/* Tesselated faces may have a different normal than calculated previously */
		IFS_check_normal (facenormals,this_face,points);
	} else {
		/* dont have to tesselate, so what we have is good */
		this_coord = my_coord;
		global_IFS_Coord_count = my_IFS_Coord_count;
	}

	for (i=0; i<global_IFS_Coord_count; i++) {
		/* printf ("vertex  %d %d\n",vert_ind,global_IFS_Coords[i]); */

		/* Triangle Coordinate */
		cindex [vert_ind] = global_IFS_Coords[i];


		/* Vertex Normal */
		if(nnormals) {
			norindex[vert_ind] = my_normalindexes[i]; 
		} 

		/* no normals, so use either pre-calculated ones 
		   (if smooth normals) or calculate quickies here */
		if (!nnormals) {
			if (fabs(creaseAngle) > 0.00001) {
				/* normalize each vertex */
				normalize_ifs_face (&rep_->normal[calc_normind*3],
					facenormals, pointfaces, 
					global_IFS_Coords[i], this_face, creaseAngle);
				rep_->norindex[vert_ind] = calc_normind++;
			} else {
				/* printf ("normals for vertex %d face %d are %f %f %f\n",
					vert_ind,this_face,facenormals[this_face].x,
					facenormals[this_face].y,facenormals[this_face].z);*/
				rep_->normal[vert_ind*3+0]=facenormals[this_face].x;
				rep_->normal[vert_ind*3+1]=facenormals[this_face].y;
				rep_->normal[vert_ind*3+2]=facenormals[this_face].z;
				rep_->norindex[vert_ind] = vert_ind;
			}
		}



		vert_ind++;
	}

	/* skip past the -1 in the coords, if it is there */
	if (this_coord != cin) this_coord++;

}

/* we have an accurate triangle count now... */
rep_->ntri = vert_ind/3;

free (tess_vs);
if ((!nnormals) && (fabs(creaseAngle) > 0.00001)) {
		free (facenormals); free (pointfaces);
}

';
