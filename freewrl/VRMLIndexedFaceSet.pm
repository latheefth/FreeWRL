# Copyright (C) 1998 Bernhard Reiter and Tuomas J. Lukka
# Copyright (C) 2000, 2002 John Stewart, CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# This is the Extrusion -> polyrep code, used by VRMLC.pm to generate
# VRMLFunc.xs &c.

# Extrusion generates 2 triangles per each extrusion step (like elevgrid..)
# The caps, if present, also add some triangles.
'
	int i,j;	/* general purpose counters */

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

	int *cindex;
	int *colindex;
        int *tcindex;
	int *norindex;

	/* GENERIC POLYREP */

	int faces=0;
	int pointctr;
	int facectr;
	int max_points_per_face = 0;
	int min_points_per_face = 99999;
	int tmp_a, tmp_b, tmp_c;
	int pt_1, pt_2, pt_3;
	float AB, AC, BC;
	struct pt *facenormals;
	int	*pointfaces;

	/* END OF GENERIC POLYREP */

	/* if the last coordIndex == -1, ignore it */
	if($f(coordIndex,cin-1) == -1) { cin--; }
	

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

	/* first complex face - are we running in fast or good mode... */
	if (smooth_normals == -1) {
		/* Needs to be initialized */
		glGetIntegerv (GL_SHADE_MODEL, &smooth_normals);
		smooth_normals = smooth_normals == GL_SMOOTH;
	}
	if (!smooth_normals){
		creaseAngle = 0.0;  /* trick following code into doing things quick */
	}

	if ((!nnormals) && (fabs(creaseAngle) > 0.00001)) {

		/* GENERIC POLYREP SMOOTH NORMAL DATABASE GENERATION 		*/
		/* 								*/
		/* create two datastructures:					*/
		/* 	- face normals; given a face, tell me the normal	*/
		/*	- point-face;   for each point, tell me the face(s)	*/

	
		/* lets see how many faces we have */
		pointctr=0;
		for(i=0; i<cin; i++) {
			if(($f(coordIndex,i) == -1) || (i==cin-1)) {
				if($f(coordIndex,i) != -1) {
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
			return;
		}
		if (faces < 1) {
			printf("an IndexedFaceSet with no faces found\n");
			return;
		}
	
		/* generate the face-normals table */
		facenormals = malloc(sizeof(*facenormals)*faces);
		pointfaces = malloc(sizeof(*pointfaces)*npoints*16); /* save max 15 */

		/* in C always check if you got the mem you wanted...  >;->		*/
  		if(!(pointfaces && facenormals )) {
			die("Not enough memory for IndexedFaceSet internals... ;(");
		} 

		tmp_a = 0;
		for(i=0; i<faces; i++) {
			/* check for degenerate triangles -- if found, try to select another point */
			tmp_c = FALSE;
			pt_1 = tmp_a; pt_2 = tmp_a+1; pt_3 = tmp_a+2;

			do {	
				/* first three coords give us the normal */
				/* printf ("face %d, coords %d %d %d  at %d\n", i,
					$f(coordIndex,pt_1), $f(coordIndex,pt_2), 
					$f(coordIndex,pt_3), tmp_a); */
				c1 = &(points[$f(coordIndex,pt_1)]);
				c2 = &(points[$f(coordIndex,pt_2)]); 
				c3 = &(points[$f(coordIndex,pt_3)]);

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
					if ((tmp_a >= cin-2) || ($f(coordIndex,tmp_a+2) == -1)) {
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
				while ($f(coordIndex,tmp_a-1) != -1) {
					tmp_a++;
				}
			}
		}
	
	
		/* now, go through each face, and make a point-face list 
		   so that I can give it a point later, and I will know which face(s) 
		   it belong to that point */

		/* printf ("\nnow generating point-face list\n");  */
		for (i=0; i<npoints; i++) { pointfaces[i*16]=0; }
		facectr=0; 
		for(i=0; i<cin; i++) {
			tmp_a=$f(coordIndex,i);
			/* printf ("pointfaces, coord %d coordIndex %d face %d\n",i,tmp_a,facectr); */
			if (tmp_a == -1) {
				facectr++;
			} else {
				tmp_a*=16;
				/* is this point in too many faces? if not, record it */
				if (pointfaces[tmp_a] < 15) {
					pointfaces[tmp_a]++;
					pointfaces[tmp_a+ pointfaces[tmp_a]] = facectr; 
				}
			}
		}

		/*
		printf ("\ncheck \n");
		for (i=0; i<npoints; i++) {
			tmp_a = i*16;
			printf ("point %d is in %d faces, these are:\n ",
				i, pointfaces[tmp_a]);

			for (tmp_b=0; tmp_b<pointfaces[tmp_a]; tmp_b++) {
				printf ("%d ",pointfaces[tmp_a+tmp_b+1]);
			}
			printf ("\n");
		}
		printf ("done generic polyrep data structure building\n");
		*/
	
		/* End of generating smooth normals for IFS */
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
							rep_->norindex[triind*3+0] = calc_normind;
							calc_normind ++;
	
							normalize_ifs_face (&rep_->normal[calc_normind*3],
								facenormals, pointfaces, lastind,
								curpoly, creaseAngle);
							rep_->norindex[triind*3+1] = calc_normind;
							calc_normind ++;
	
							normalize_ifs_face (&rep_->normal[calc_normind*3],
								facenormals, pointfaces, $f(coordIndex,i),
								curpoly, creaseAngle);
							rep_->norindex[triind*3+2] = calc_normind;
							calc_normind ++;
						} else {
							/* calculate quickie normals */
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

	if ((!nnormals) && (fabs(creaseAngle) > 0.00001)) {
			free (facenormals); free (pointfaces);
	}
	}
if(!$f(convex)) {

/* End a non-convex polygon */
gluEndPolygon( global_tessobj );

}
';
