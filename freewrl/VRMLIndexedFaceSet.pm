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


int i,j;	/* general purpose counters */
int tmp_a, tmp_b;


/* if the last coordIndex == -1, ignore it */
if($f(coordIndex,cin-1) == -1) { cin--; }
	

/* texture coords IndexedFaceSet coords and normals */
$fv_null(texCoord, texCoords, get2, &ntexCoords);
$fv(coord, points, get3, &npoints);
$fv_null(normal, normals, get3, &nnormals); 

	
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

if ((!nnormals) && (fabs(creaseAngle) > 0.00001)) {
	/* count the faces in this polyrep */
	faces = count_IFS_faces (cin,this_);

	/* generate the face-normals table */
	facenormals = malloc(sizeof(*facenormals)*faces);
	pointfaces = malloc(sizeof(*pointfaces)*npoints*POINT_FACES); /* save max x points */

	/* in C always check if you got the mem you wanted...  >;->		*/
  		if(!(pointfaces && facenormals )) {
		die("Not enough memory for IndexedFaceSet internals... ;(");
	} 

	IFS_face_normals (facenormals,pointfaces,faces,npoints,cin,points,this_);
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

/* if we calculate normals, we use a normal per point, NOT per triangle */ 
/* use a normal per triangle */
if (!nnormals) {  		/* 3 vertexes per triangle, and 3 points per tri */
	rep_->normal = malloc(sizeof(*(rep_->normal))*3*3*ntri);
} else { 			/* dont do much, but get past check below */
	rep_->normal = malloc(1);
}

rep_->ntri = ntri;

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
	int calc_normind=0; /* if we calculate normals ourselves */

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
				/* no normals, so use either pre-calculated ones 
				   (if smooth normals) or calculate quickies here */
				if (!nnormals) {
					if (fabs(creaseAngle) > 0.00001) {
						/* normalize each vertex */
						normalize_ifs_face (&rep_->normal[calc_normind*3],
							facenormals, pointfaces, initind,
							curpoly, creaseAngle);
						rep_->norindex[triind*3+0] = calc_normind++;

						normalize_ifs_face (&rep_->normal[calc_normind*3],
							facenormals, pointfaces, lastind,
							curpoly, creaseAngle);
						rep_->norindex[triind*3+1] = calc_normind++;

						normalize_ifs_face (&rep_->normal[calc_normind*3],
							facenormals, pointfaces, $f(coordIndex,i),
							curpoly, creaseAngle);
						rep_->norindex[triind*3+2] = calc_normind++;
					} else {
						/* calculate quickie normals */
						c1 = &(points[initind]);
						c2 = &(points[lastind]); 
						c3 = &(points[$f(coordIndex,i)]);
						a[0]=c2->c[0]-c1->c[0]; a[1]=c2->c[1]-c1->c[1];
						a[2]=c2->c[2]-c1->c[2]; b[0]=c3->c[0]-c1->c[0];
						b[1]=c3->c[1]-c1->c[1]; b[2]=c3->c[2]-c1->c[2];
						rep_->normal[triind*3+0]=a[1]*b[2]-b[1]*a[2];
						rep_->normal[triind*3+1]=-(a[0]*b[2]-b[0]*a[2]);
						rep_->normal[triind*3+2]=a[0]*b[1]-b[0]*a[1];
						rep_->norindex[triind*3+0] = triind;
						rep_->norindex[triind*3+1] = triind;
						rep_->norindex[triind*3+2] = triind;
					}
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
				if(tcin && ntexCoords) { lasttcind =  $f(texCoordIndex,i);
				} else { lasttcind = $f(coordIndex,i); }
				triind++;
			}
		}
	}

if ((!nnormals) && (fabs(creaseAngle) > 0.00001)) {
		free (facenormals); free (pointfaces);
}
}
if(!$f(convex)) {

/* End a non-convex polygon */
gluEndPolygon( global_tessobj );

}
';
