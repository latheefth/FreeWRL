/*******************************************************************
 * Copyright (C) 2004 John Stewart, CRC Canada.
 * Copyright (C) 2000 2002 John Stewart, CRC Canada.
 * Copyright (C) 1998 Bernhard Reiter and Tuomas J. Lukka
 *
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License (file COPYING in the distribution)
 * for conditions of use and redistribution.
 **********************************************************************/
                                                                                
/*****************************************
 * 
 * Complex Geometry; ElevationGrid, Extrusion, IndexedFaceSet, Text.
 *
 * This code generates a Polyrep structure, that is further
 * streamlined then streamed to OpenGL. 
 * 
 * Polyreps are streamed in Polyrep.c
 * 
 *******************************************/

#include "headers.h"
#include "Structs.h"

//added M. Ward Dec 6/04
extern void initialize_smooth_normals();
extern void Elev_Tri (int vertex_ind,int this_face,int A,int D,int E,int NONORMALS,struct VRML_PolyRep *this_Elev,struct pt *facenormals,int *pointfaces,int ccw);
extern int count_IFS_faces(int cin, struct VRML_IndexedFaceSet *this_IFS);
extern void IFS_face_normals(struct pt *facenormals,int *faceok,int *pointfaces,int faces,int npoints,int cin,struct SFColor *points,struct VRML_IndexedFaceSet *this_IFS,int ccw);
extern void verify_global_IFS_Coords(int max);
extern void IFS_check_normal(struct pt *facenormals,int this_face,struct SFColor *points,int base,struct VRML_IndexedFaceSet *this_IFS,int ccw);
extern void Extru_tex(int vertex_ind,int tci_ct,int A,int B,int C,struct VRML_PolyRep *this_Elev,int ccw,int tcindexsize);
extern void Extru_ST_map(int triind_start,int start,int end,float *Vals,int nsec,struct VRML_PolyRep *this_Extru, int tcoordsize);
extern void Extru_check_normal(struct pt *facenormals,int this_face,int dire,struct VRML_PolyRep *rep_,int ccw);



void make_text (struct VRML_Text *this_) {
	struct VRML_PolyRep *rep_ = (VRML_PolyRep *)this_->_intern;
        double spacing = 1.0;
        double size = 1.0;
	unsigned int fsparams = 0;

	/* We need both sides */
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);

	if((this_->fontStyle)) {
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

		struct VRML_FontStyle *fsp;
		int xx;
		unsigned char *lang;
		unsigned char *style;
		struct Multi_String family;	 
		struct Multi_String justify;
		int tmp; int tx;
		SV **svptr;
		unsigned char *stmp;
		
		/* step 0 - is the FontStyle a proto? */ 
		fsp = (VRML_FontStyle *)this_->fontStyle;
		
		/* step 0.5 - now that we know FontStyle points ok, go for
		 * the other pointers */
		lang = (unsigned char *)SvPV((fsp->language),(STRLEN&)xx);
		style = (unsigned char *)SvPV((fsp->style),(STRLEN&)xx);

		family = fsp->family;	 
		justify = fsp->justify;


		/* Step 1 - record the spacing and size, for direct use */
		spacing = fsp->spacing;
		size = fsp->size;

		/* Step 2 - do the SFBools */
        	fsparams = (fsp->horizontal)|(fsp->leftToRight<<1)|(fsp->topToBottom<<2); 


		/* Step 3 - the SFStrings - style and language */
		/* actually, language is not parsed yet */
	
		if (strlen((const char *)style)) {
			if (!strcmp((const char *)style,"ITALIC")) {fsparams |= 0x10;}
			else if(!strcmp((const char *)style,"BOLD")) {fsparams |= 0x08;}
			else if (!strcmp((const char *)style,"BOLDITALIC")) {fsparams |= 0x18;}
			else if (strcmp((const char *)style,"PLAIN")) {
				printf ("Warning - FontStyle style %s  assuming PLAIN\n",style);}
		}
		if (strlen((const char *)lang)) {printf ("Warning - FontStyle - language param unparsed\n");}


		/* Step 4 - the MFStrings now. Family, Justify. */
		/* family can be blank, or one of the pre-defined ones. Any number of elements */

		svptr = family.p;
		for (tmp = 0; tmp < family.n; tmp++) {
			stmp = (unsigned char *)SvPV(svptr[tmp],(STRLEN&)xx);
			if (strlen((const char *)stmp) == 0) {fsparams |=0x20; } 
			else if (!strcmp((const char *)stmp,"SERIF")) { fsparams |= 0x20;}
			else if(!strcmp((const char *)stmp,"SANS")) { fsparams |= 0x40;}
			else if (!strcmp((const char *)stmp,"TYPEWRITER")) { fsparams |= 0x80;}
			//else { printf ("Warning - FontStyle family %s unknown\n",stmp);}
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
			stmp = (unsigned char *)SvPV(svptr[tmp],(STRLEN&)xx);
			if (strlen((const char *)stmp) == 0) {
				if (tmp == 0) {fsparams |= 0x400;
				} else {fsparams |= 0x2000;
				}
			} 
			else if (!strcmp((const char *)stmp,"FIRST")) { fsparams |= (0x200<<(tmp*4));}
			else if(!strcmp((const char *)stmp,"BEGIN")) { fsparams |= (0x400<<(tmp*4));}
			else if (!strcmp((const char *)stmp,"MIDDLE")) { fsparams |= (0x800<<(tmp*4));}
			else if (!strcmp((const char *)stmp,"END")) { fsparams |= (0x1000<<(tmp*4));}
			//else { printf ("Warning - FontStyle family %s unknown\n",stmp);}
		}
	} else {
		/* send in defaults */
		fsparams = 0x2427;
	}


	// do the Text parameters, guess at the number of triangles required
	rep_->ntri = 0;
	//printf ("Text, calling FW_rendertext\n");

	/* call render text - NULL means get the text from the string */
	FW_rendertext (((this_->string).n),((this_->string).p),NULL, ((this_->length).n),(double *) ((this_->length).p),
			(this_->maxExtent),spacing,size,fsparams,rep_);

	//printf ("Text, tris = %d\n",rep_->ntri);

	glPopAttrib();
}

void make_elevationgrid(struct VRML_ElevationGrid *this_) {
	int x,z;
	int nx = (this_->xDimension);
	float xs = (this_->xSpacing);
	int nz = (this_->zDimension);
	float zs = (this_->zSpacing);
	float *f = ((this_->height).p);
	int *cindex; 
	float *coord;
	float *tcoord;
	int *colindex;
	int ntri = (nx && nz ? 2 * (nx-1) * (nz-1) : 0);
	int vertex_ind;
	int nf = ((this_->height).n);
	int cpv = ((this_->colorPerVertex));
	struct SFColor *colors; 
	int ncolors=0;
	struct VRML_PolyRep *rep_ = (VRML_PolyRep *)this_->_intern;
	float creaseAngle = (this_->creaseAngle);
	int npv = ((this_->normalPerVertex));
	struct SFColor *normals;
	int nnormals = 0;
	int ccw = ((this_->ccw));
	
	struct SFVec2f *texcoords;
	int ntexcoords = 0;
	
	/* variables for calculating smooth normals */
	int 	HAVETOSMOOTH;
	struct 	pt *facenormals = 0;
	int	*pointfaces = 0;
	int 	faces;
	int	this_face = 0;
	int	tmp;
	int 	calc_normind;
	
	
	int A,B,C,D; /* should referr to the four vertices 
			of the polygon	
			(hopefully) counted counter-clockwise, like
	
			 D----C
			 |    |
			 |    |
			 |    |
			 A----B
	
			*/
	struct pt ac,bd;/* help vectors	*/
	int E,F;	/* third point to be used for the triangles*/
		
	if(this_->color) {
			  if(!(*(struct VRML_Virt **)(this_->color))-> get3) {
			  	freewrlDie("NULL METHOD ElevationGrid color  get3");
			  }
			   colors =  ((*(struct VRML_Virt **)(this_->color))-> get3(this_->color,
			     &ncolors)) ;
			};
	if(this_->normal) {
			  if(!(*(struct VRML_Virt **)(this_->normal))-> get3) {
			  	freewrlDie("NULL METHOD ElevationGrid normal  get3");
			  }
			   normals =  ((*(struct VRML_Virt **)(this_->normal))-> get3(this_->normal,
			     &nnormals)) ;
			};
	if(this_->texCoord) {
			  if(!(*(struct VRML_Virt **)(this_->texCoord))-> get2) {
			  	freewrlDie("NULL METHOD ElevationGrid texCoord  get2");
			  }
			   texcoords =  ((*(struct VRML_Virt **)(this_->texCoord))-> get2(this_->texCoord,
			     &ntexcoords)) ;
			};
	
	/* Render_Polyrep will use this number of triangles */
	rep_->ntri = ntri;
	
	/* ccw or not? */
	rep_->ccw = 1;
	
	//printf ("nf %d nx %d nz %d\n",nf, nx, nz);

	if(nf != nx * nz) {
		freewrlDie("Elevationgrid: x,y vs. height: incorrect count:\n");
	}
	
	
	if(ncolors) {
		if(!cpv && ncolors < (nx-1) * (nz-1)) {
			freewrlDie("Elevationgrid: too few colors");
		}
		if(cpv && ncolors < nx*nz) {
			freewrlDie("Elevationgrid: 2too few colors");
		}
		colindex = rep_->colindex = (int *)malloc(sizeof(*(rep_->colindex))*3*(ntri));
		if (!(colindex)) { 
			freewrlDie("Not enough memory for ElevationGrid node color index ");
		}
	}
	
	if (HAVETODOTEXTURES) {
		/* so, we now have to worry about textures. */
		tcoord = rep_->tcoord = (float *)malloc(sizeof(*(rep_->tcoord))*nx*nz*3);
		if (!(tcoord)) freewrlDie ("Not enough memory ElevGrid Tcoords");
	
		rep_->tcindex = 0; // we will generate our own mapping
		/* do we have to generate a default texture map?? */
		if ((ntexcoords > 0) && (ntexcoords < (nx*nz))) {
			printf ("too few TextureCoordinates for ElevationGrid, expect %d have %d\n",
				nx*nz, ntexcoords);
			ntexcoords = 0; // set it to zero, so we calculate them
		}
	}
	
	
	
	
	/* Coords, CoordIndexes, and Normals are needed all the time */
	cindex = rep_->cindex = (int *)malloc(sizeof(*(rep_->cindex))*3*(ntri));
	coord = rep_->coord = (float *)malloc(sizeof(*(rep_->coord))*nx*nz*3);
	rep_->norindex = (int *)malloc(sizeof(*(rep_->norindex))*3*ntri);	
	if(!(cindex && coord && rep_->norindex)) {
		freewrlDie("Not enough memory for ElevationGrid node triangles... ;(");
	} 
	
	/* we are calculating Normals */
	if (nnormals == 0) {
		rep_->normal = (float *)malloc(sizeof(*(rep_->normal))*3*ntri*3);
		if (!(rep_->normal)) {
			freewrlDie("Not enough memory for ElevationGrid node normals");
		}
	}
	
	
	/* determine if we need to smooth out norms */ 
	initialize_smooth_normals();
	HAVETOSMOOTH = smooth_normals && (nnormals == 0) && (fabs(creaseAngle) > 0.00001);
	
	if (nnormals == 0) {
		faces = (nx-1) * (nz-1) * 2; /* we treat each triangle as a face = makes really nice normals */ 
		
		/* for each face, we can look in this structure and find the normal */
		facenormals = (pt*)malloc(sizeof(*facenormals)*faces);
		
		/* for each point, tell us which faces it is in, first index is  the face count */
		pointfaces = (int *)malloc(sizeof(*pointfaces)*nz*nx*POINT_FACES);
		for (tmp=0; tmp<nz*nx; tmp++) { pointfaces[tmp*POINT_FACES]=0; }
	
		if (!(pointfaces && facenormals)) {
			freewrlDie("Not enough memory for ElevationGrid node normal point calcs... ");
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
}


void make_indexedfaceset(struct VRML_IndexedFaceSet *this_) {
	int cin = ((this_->coordIndex).n);
	int cpv = ((this_->colorPerVertex));
	int npv = ((this_->normalPerVertex));
	int tcin = ((this_->texCoordIndex).n);
	int colin = ((this_->colorIndex).n); 
	int norin = ((this_->normalIndex).n);
	float creaseAngle = (this_->creaseAngle);
	int ccw = ((this_->ccw));
	
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
	
	struct SFColor *c1;
	struct SFColor *points; 
	struct SFVec2f *texCoords; 
	struct VRML_PolyRep *rep_ = (VRML_PolyRep *)this_->_intern;
	struct SFColor *normals;
	struct SFColor *colors;
	
	int *cindex;		/* Coordinate Index	*/
	int *colindex;		/* Color Index		*/
	int *tcindex;		/* Tex Coord Index	*/
	int *norindex;		/* Normals Index	*/
	
	int faces=0;
	struct pt *facenormals; // normals for each face
	int	*faceok;	// is this face ok? (ie, not degenerate triangles, etc)
	int	*pointfaces;
	
	GLdouble tess_v[3];             /*param.to gluTessVertex()*/
	int *tess_vs;              /* pointer to space needed */
	
	
	int i;				/* general purpose counters */
	int this_face, this_coord, this_normal, this_normalindex;
	
	/* record ccw flag */
	rep_->ccw = ccw;
	
	
	//printf ("IFS - cin %d\n",cin);

	/* check to see if there are params to make at least one triangle */
	if (cin<2) {
		//printf ("Null IFS found, returing ntri0\n");
	        rep_->ntri = 0;
	        return;
	}
	
	/* if the last coordIndex == -1, ignore it */
	if(((this_->coordIndex).p[cin-1]) == -1) { cin--; }
		
	
	/* texture coords IndexedFaceSet coords colors and normals */
	if(this_->texCoord) {
			  if(!(*(struct VRML_Virt **)(this_->texCoord))-> get2) {
			  	freewrlDie("NULL METHOD IndexedFaceSet texCoord  get2");
			  }
			   texCoords =  ((*(struct VRML_Virt **)(this_->texCoord))-> get2(this_->texCoord,
			     &ntexCoords)) ;
			};
	if(this_->coord) {
			  if(!(*(struct VRML_Virt **)(this_->coord))-> get3) {
			  	freewrlDie("NULL METHOD IndexedFaceSet coord  get3");
			  }
			   points =  ((*(struct VRML_Virt **)(this_->coord))-> get3(this_->coord,
			     &npoints)) ;}
	 	  else { (freewrlDie("NULL FIELD IndexedFaceSet coord "));};
	if(this_->normal) {
			  if(!(*(struct VRML_Virt **)(this_->normal))-> get3) {
			  	freewrlDie("NULL METHOD IndexedFaceSet normal  get3");
			  }
			   normals =  ((*(struct VRML_Virt **)(this_->normal))-> get3(this_->normal,
			     &nnormals)) ;
			}; 
	if(this_->color) {
			  if(!(*(struct VRML_Virt **)(this_->color))-> get3) {
			  	freewrlDie("NULL METHOD IndexedFaceSet color  get3");
			  }
			   colors =  ((*(struct VRML_Virt **)(this_->color))-> get3(this_->color,
			     &ncolors)) ;
			}; 
	
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
			ntexerrors = 1;
		}
	
		if (tcin == 0 && ntexCoords != npoints) { 
			/* rule G */
			printf ("IndexedFaceSet, Rule G: points %d texCoords %d and no texCoordIndex\n",
				npoints,ntexCoords);
			ntexerrors = 1;
	   	}
	}
	
	
	
	/* Once per freewrl Invocation, the smooth_normals flag is initialized */
	initialize_smooth_normals();
	if (!smooth_normals){
		creaseAngle = 0.0;  /* trick following code into doing things quick */
	}
	
	/* count the faces in this polyrep and allocate memory. */
	faces = count_IFS_faces (cin,this_);
	
	if (faces == 0) {
		rep_->ntri = 0;
		return;
	}

	/* are there any coordinates? */
	if (npoints <= 0) {
		rep_->ntri = 0;
		return;
	}
	
	facenormals = (pt*)malloc(sizeof(*facenormals)*faces);
	faceok = (int*)malloc(sizeof(int)*faces);
	pointfaces = (int*)malloc(sizeof(*pointfaces)*npoints*POINT_FACES); /* save max x points */
	
	/* in C always check if you got the mem you wanted...  >;->		*/
	if(!(faceok && pointfaces && facenormals )) {
		freewrlDie("Not enough memory for IndexedFaceSet internals... ;(");
	} 
	
	/* generate the face-normals table, so for each face, we know the normal 
	   and for each point, we know the faces that it is in */
	
	IFS_face_normals (facenormals,faceok,pointfaces,faces,npoints,cin,points,this_,ccw);
	
	/* wander through to see how much memory needs allocating for triangles */
	for(i=0; i<cin; i++) {
		if(((this_->coordIndex).p[i]) == -1) {
	                       if(tcin > 0  && ((this_->texCoordIndex).p[i]) != -1) {
				/* Rule F part 2 see above */
	                                printf ("IndexedFaceSet, Rule F, part 2: coordIndex[%d] = -1 => expect texCoordIndex[%d] = -1 (but is %d)\n", i, i, ((this_->texCoordIndex).p[i]));
				ntexerrors = 1;
	                        }
			ntri += nvert-2;
			nvert = 0;
		} else {
			if (tcin > 0 && ((this_->texCoordIndex).p[i]) >= ntexCoords) {
				/* Rule F, part 3 see above */
				printf ("IndexedFaceSet, Rule F, part 3: TexCoordIndex[%d] %d is greater than num texCoord (%d)\n",i, ((this_->texCoordIndex).p[i]),
					ntexCoords);
				ntexerrors = 1;
			}
			nvert ++;
		}
	}
	if(nvert>2) {ntri += nvert-2;}
	
	/* Tesselation MAY use more triangles; lets estimate how many more */
	if(!((this_->convex))) { ntri =ntri*2; }
	
	/* fudge factor - leave space for 1 more triangle just in case we have errors on input */
	ntri++;
	
	cindex = rep_->cindex = (int*)malloc(sizeof(*(rep_->cindex))*3*(ntri));
	colindex = rep_->colindex = (int*)malloc(sizeof(*(rep_->colindex))*3*(ntri));
	norindex = rep_->norindex = (int*)malloc(sizeof(*(rep_->norindex))*3*ntri);
	
	/* if we calculate normals, we use a normal per point, NOT per triangle */ 
	if (!nnormals) {  		/* 3 vertexes per triangle, and 3 points per tri */
		rep_->normal = (float*)malloc(sizeof(*(rep_->normal))*3*3*ntri);
	} else { 			/* dont do much, but get past check below */
		rep_->normal = (float*)malloc(1);
	}
	
	
	if (ntexerrors == 0) {
		if ((ntexCoords) && (HAVETODOTEXTURES)) {
			tcindex = rep_->tcindex = (int*)malloc(sizeof(*(rep_->tcindex))*3*(ntri));
		}
	} else {
		ntexCoords = 0; tcin = 0; 
	}
	
	/* in C always check if you got the mem you wanted...  >;->		*/
	if(!(cindex && colindex && norindex && rep_->normal )) {
		freewrlDie("Not enough memory for IndexFaceSet node triangles... ;(");
	} 
	
	if (!(tcindex) && HAVETODOTEXTURES && ntexCoords) {
		freewrlDie("Not enough memory for IndexFaceSet textures... ;(");
	} 
	
	
	
	/* Concave faces - use the OpenGL Triangulator to give us the triangles */
	tess_vs=(int*)malloc(sizeof(*(tess_vs))*(ntri)*3);
	
	this_coord = 0;
	this_normal = 0;
	this_normalindex = 0;
	i = 0;
	
	for (this_face=0; this_face<faces; this_face++) {
		int relative_coord;		/* temp, used if not tesselating	*/
		int initind, lastind;  		/* coord indexes 			*/
	
		global_IFS_Coord_count = 0;
		relative_coord = 0;
	
		if (!faceok[this_face]) {
			//printf ("in generate of faces, face %d is invalid, skipping...\n",this_face);
	
			/* skip past the seperator, except if we are t the end */
	
			// skip to either end or the next -1
			while ((this_coord < cin) && (((this_->coordIndex).p[this_coord]) != -1)) this_coord++;
	
			// skip past the -1
			if ((this_coord < (cin-1)) && (((this_->coordIndex).p[this_coord]) == -1)) this_coord++;
		} else {
	
		
			//printf ("working on face %d coord %d total coords %d coordIndex %d\n",
			//	this_face,this_coord,cin,((this_->coordIndex).p[ this_coord])); 
		
			/* create the global_IFS_coords array, at least this time 	*/
			/*								*/
			/* What we do is to create a series of triangle vertex 		*/
			/* relative to the current coord index, then use that		*/
			/* to generate the actual coords further down. This helps	*/
			/* to map normals, textures, etc when tesselated and the	*/
			/*  *perVertex modes are set.					*/
		
			/* If we have concave, tesselate! */
			if (!((this_->convex))) { 
				gluBeginPolygon(global_tessobj); 
			} else {
				initind = relative_coord++;
				lastind = relative_coord++;
			}
		
			i = ((this_->coordIndex).p[ relative_coord + this_coord]);
		
			while (i != -1) {
				if (!((this_->convex))) {
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
					//printf ("triangle %d %d %d\n",initind,lastind,relative_coord);
					lastind = relative_coord++;
				}
		
				if (relative_coord + this_coord == cin) { 
					i = -1; 
				} else {
					i = ((this_->coordIndex).p[ relative_coord + this_coord]);
				}
			}
		
			if (!((this_->convex))) { 
				gluEndPolygon(global_tessobj); 
			
				/* Tesselated faces may have a different normal than calculated previously */
				/* bounds check, once again */
	
				verify_global_IFS_Coords(cin);
	
				IFS_check_normal (facenormals,this_face,points, this_coord, this_,ccw); 
			}
		
		
			/* now store this information for the whole of the polyrep */
			for (i=0; i<global_IFS_Coord_count; i++) {
				/* Triangle Coordinate */
				cindex [vert_ind] = ((this_->coordIndex).p[this_coord+global_IFS_Coords[i]]);
		
				//printf ("vertex  %d  gic %d cindex %d\n",vert_ind,global_IFS_Coords[i],cindex[vert_ind]); 
		
				/* Vertex Normal */
				if(nnormals) {
					if (norin) {
						/* we have a NormalIndex */
						if (npv) {
							norindex[vert_ind] = ((this_->normalIndex).p[this_coord+global_IFS_Coords[i]]);
							// printf ("norm1, index %d\n",norindex[vert_ind]);
						} else {
							norindex[vert_ind] = ((this_->normalIndex).p[this_face]);
							// printf ("norm2, index %d\n",norindex[vert_ind]);
						}
					} else {
						/* no normalIndex  - use the coordIndex */
						if (npv) {
							norindex[vert_ind] = ((this_->coordIndex).p[this_coord+global_IFS_Coords[i]]);
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
						//printf ("using calculated normals %f %f %f for face %d, vert_ind %d\n",
							//rep_->normal[vert_ind*3+0],rep_->normal[vert_ind*3+1],
							//rep_->normal[vert_ind*3+2],this_face,rep_->norindex[vert_ind]);
					}
				}
		
				/* Vertex Colours */
				if(ncolors) {
					if (colin) {
						/* we have a colorIndex */
						if (cpv) {
							colindex[vert_ind] = ((this_->colorIndex).p[this_coord+global_IFS_Coords[i]]);
							//printf ("col1, index %d\n",colindex[vert_ind]);
						} else {
							colindex[vert_ind] = ((this_->colorIndex).p[this_face]);
							// printf ("col2, index %d\n",colindex[vert_ind]);
						}
					} else {
						/* no colorIndex  - use the coordIndex */
						if (cpv) {
							colindex[vert_ind] = ((this_->coordIndex).p[this_coord+global_IFS_Coords[i]]);
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
						tcindex[vert_ind] = ((this_->texCoordIndex).p[this_coord+global_IFS_Coords[i]]);
						//printf ("ntexCoords,tcin,  index %d\n",tcindex[vert_ind]);
					} else {
						/* no texCoordIndex, use the Coord Index */
						tcindex[vert_ind] = ((this_->coordIndex).p[this_coord+global_IFS_Coords[i]]);
						//printf ("ntexcoords, notcin, vertex %d point %d\n",vert_ind,tcindex[vert_ind]);
					}
				}
				
				/* increment index, but check for baaad errors.	 */
				if (vert_ind < (ntri*3-1)) vert_ind++;
			}
	
			/* for the next face, we work from a new base */
			this_coord += relative_coord;
	
			/* skip past the seperator, except if we are t the end */
			if (this_coord < cin)
				if (((this_->coordIndex).p[this_coord]) == -1) {this_coord++;}
		}
	}
	
	/* we have an accurate triangle count now... */
	rep_->ntri = vert_ind/3;
	
	free (tess_vs);
	free (facenormals); 
	free (faceok);
	free (pointfaces);
	
}



void make_extrusion(struct VRML_Extrusion *this_) {
	/*****begin of Member Extrusion	*/
	/* This code originates from the file VRMLExtrusion.pm */
	
	int tcoordsize;
	int tcindexsize;
	
	int nspi = ((this_->spine).n);			/* number of spine points	*/
	int nsec = ((this_->crossSection).n);		/* no. of points in the 2D curve
						   but note that this is verified
						   and coincident points thrown out */
	
	int nori = ((this_->orientation).n);		/* no. of given orientators
						   which rotate the calculated SCPs =
						   spine-aligned cross-section planes*/ 
	int nsca = ((this_->scale).n);			/* no. of scale parameters	*/
	
	struct SFColor *spine =((this_->spine).p);	/* vector of spine vertices	*/
	struct SFVec2f *curve =((this_->crossSection).p);/* vector of 2D curve points	*/
	struct SFRotation *orientation=((this_->orientation).p);/*vector of SCP rotations*/
	
	struct VRML_PolyRep *rep_=(VRML_PolyRep *)this_->_intern;/*internal rep, we want to fill*/
	
	/* the next variables will point at members of *rep		*/
	int   *cindex;				/* field containing indices into
						   the coord vector. Three together
						   indicate which points form a 
						   triangle			*/
	float *coord;				/* contains vertices building the
						   triangles as x y z values	*/
	
	float *tcoord;				/* contains vertices building the
						   textures as x y z values	*/
	
	int	*tcindex;			/* field containing texture indices
						   for the vertex. 		*/
	
	int   *norindex; 			/* indices into *normal		*/
	float *normal; 				/* (filled in a different function)*/ 
	
	
	int ntri = 0;			 	/* no. of triangles to be used
						   to represent all, but the caps */
	int nctri=0;				/* no. of triangles for both caps*/
	int max_ncoord_add=0;			/* max no. of add coords	*/
	int ncoord_add=0;			/* no. off added coords		*/
	int ncoord=0;				/* no. of used coords		*/
	
	int ncolinear_at_begin=0;		/* no. of triangles which need
						to be skipped, because curve-points
						are in one line at start of curve*/
	int ncolinear_at_end=0;			/* no. of triangles which need
						to be skipped, because curve-points
						are in one line at end of curve*/
	
	int spi,sec,triind,pos_of_last_zvalue;	/* help variables 		*/
	int next_spi, prev_spi;
	int t;					/* another loop var		*/
	
	
	int circular = 0;			/* is spine  closed?		*/
	int tubular=0;				/* is the 2D curve closed?	*/
	int spine_is_one_vertix;		/* only one real spine vertix	*/
	
	float spxlen,spylen,spzlen;		/* help vars for scaling	*/
	
						/* def:struct representing SCPs	*/
	struct SCP { 				/* spine-aligned cross-section plane*/
		struct pt y;			/* y axis of SCP		*/
		struct pt z;			/* z axis of SCP		*/
		int prev,next;			/* index in SCP[]
						prev/next different vertix for 
						calculation of this SCP		*/
		   };
	
	struct SCP *SCP;			/* dyn. vector rep. the SCPs	*/
	
	struct pt spm1,spp1,spy,spz,spx;	/* help vertix vars	*/
	
	int	tci_ct;				/* Tex Gen index counter	*/
	
	/* variables for calculating smooth normals */
	int 	HAVETOSMOOTH;
	struct 	pt *facenormals = 0;
	int	*pointfaces = 0;
	int	*defaultface = 0;
	int	this_face = 0;			/* always counts up		*/
	int	tmp;
	float creaseAngle = (this_->creaseAngle);
	int	ccw = ((this_->ccw));
	int	end_of_sides;			/* for triangle normal generation,
						   keep track of where the sides end
						   and caps begin		*/
	
	/* variables for begin/endcap S,T mapping for textures			*/
	float *beginVals;
	float *endVals;
	struct SFVec2f *crossSection;
	
	int Extru_Verbose = 0;
	
	
	if (Extru_Verbose) 
		printf ("VRMLExtrusion.pm start\n");
	
	/***********************************************************************
	 *
	 * Copy and verify cross section - remove coincident points (yes, virginia,
	 * one of the NIST tests has this - the pie-shaped convex one
	 *
	 ************************************************************************/
	
	/* is there anything to this Extrusion??? */
	if (nsec < 1) { 
		rep_->ntri=0;
		return;
	} else {
		int tmp1, temp_indx;
		int increment, currentlocn;
	
		crossSection     = (SFVec2f*)malloc(sizeof(crossSection)*nsec*2);
		if (!(crossSection)) freewrlDie ("can not malloc memory for Extrusion crossSection");
	
	
		currentlocn = 0;
		for (tmp1=0; tmp1<nsec; tmp1++) {
			/* save this crossSection */
			crossSection[currentlocn].c[0] = curve[tmp1].c[0];
			crossSection[currentlocn].c[1] = curve[tmp1].c[1];
		
			/* assume that it is not duplicated */
			increment = 1;	
	
			for (temp_indx=0; temp_indx<currentlocn; temp_indx++) {
				if ((APPROX(crossSection[currentlocn].c[0],crossSection[temp_indx].c[0])) &&
				    (APPROX(crossSection[currentlocn].c[1],crossSection[temp_indx].c[1]))) {
					/* maybe we have a closed curve, so points SHOULD be the same */
					if ((temp_indx != 0) && (tmp1 != (nsec-1))) {
						//printf ("... breaking; increment = 0\n");
						increment = 0;
						break;
					} else { 
						//printf ("... we are tubular\n");
						tubular = 1;
					}
				}
			}
			/* increment the crossSection index, unless it was duplicated */
			currentlocn += increment;
		}
		
		if (Extru_Verbose) 
			printf ("we had nsec %d coords, but now we have %d\n",nsec,currentlocn);
		nsec = currentlocn;
	}
	
	
	/* now that we have removed possible coincident vertices, we can calc ntris */
	ntri = 2 * (nspi-1) * (nsec-1);
	
	
	if (Extru_Verbose) 
		printf ("so, we have ntri %d nspi %d nsec %d\n",ntri,nspi,nsec);
	
	/* check if the spline is closed					*/
	
	if(APPROX(spine[0].c[0], spine[nspi-1].c[0]) &&
	   APPROX(spine[0].c[1], spine[nspi-1].c[1]) &&
	   APPROX(spine[0].c[2], spine[nspi-1].c[2])) 
		circular = 1;
	
	if (Extru_Verbose) printf ("tubular %d circular %d\n",tubular, circular); 
	 
	
	/************************************************************************
	 * calc number of triangles per cap, if caps are enabled and possible	
	 */
	
	if(((this_->beginCap))||((this_->endCap))) {
		if(tubular?nsec<4:nsec<3) {
			freewrlDie("Only two real vertices in crossSection. Caps not possible!");
		}
	
		if(Extru_Verbose && circular && tubular) {
			printf("Spine and crossSection-curve are closed - how strange! ;-)\n");
			/* maybe we want to fly in this tunnel? Or it is semi 
			   transparent somehow? It is possible to create
			   nice figures if you rotate the cap planes... */
		}
	
		if(tubular)	nctri=nsec-2;
		else		nctri=nsec-1;	
	
		if (Extru_Verbose) printf ("nsec = %d, ntri = %d nctri = %d\n",nsec, ntri,nctri);
	
			/* check if there are colinear points at the beginning of the curve*/
		sec=0;
		while(sec+2<=nsec-1 && 
			/* to find out if two vectors a and b are colinear, 
			   try a.x*b.y=a.y*b.x					*/
	
			APPROX(0,    (crossSection[sec+1].c[0]-crossSection[0].c[0])
				    *(crossSection[sec+2].c[1]-crossSection[0].c[1])
				  -  (crossSection[sec+1].c[1]-crossSection[0].c[1])
				    *(crossSection[sec+2].c[0]-crossSection[0].c[0]))	
		     ) ncolinear_at_begin++, sec++;
	
		/* check if there are colinear points at the end of the curve
			in line with the very first point, because we want to
			draw the triangle to there.				*/
		sec=tubular?(nsec-2):(nsec-1);
		while(sec-2>=0 && 
			APPROX(0,    (crossSection[sec  ].c[0]-crossSection[0].c[0])
				    *(crossSection[sec-1].c[1]-crossSection[0].c[1])
				  -  (crossSection[sec  ].c[1]-crossSection[0].c[1])
				    *(crossSection[sec-1].c[0]-crossSection[0].c[0]))	
		     ) ncolinear_at_end++,sec--;
	
		nctri-= ncolinear_at_begin+ncolinear_at_end;
	
		if(nctri<1) {
			/* no triangle left :(	*/
			freewrlDie("All in crossSection points colinear. Caps not possible!");
	 	}
	 
		/* so we have calculated nctri for one cap, but we might have two*/
		nctri= ((((this_->beginCap)))?nctri:0) + ((((this_->endCap)))?nctri:0) ;
	}
	 
	/* if we have non-convex polygons, we might need a few triangles more	*/
	/* 	The unused memory will be freed with realloc later		*/
	if(!((this_->convex))) {
	
		max_ncoord_add=(nspi-1)*(nsec-1) /* because of intersections	*/
				+nctri;		/* because of cap tesselation	*/
		nctri*=2;	/* we might need more trigs for the caps	*/
	}
	
	/************************************************************************
	 * prepare for filling *rep
	 */
	
	rep_->ccw = 1;
	 
	rep_->ntri = ntri + nctri;	/* Thats the no. of triangles representing
					the whole Extrusion Shape.		*/
	
	/* get some memory							*/
	cindex  = rep_->cindex   = (int *)malloc(sizeof(*(rep_->cindex))*3*(rep_->ntri));
	coord   = rep_->coord    = (float *)malloc(sizeof(*(rep_->coord))*(nspi*nsec+max_ncoord_add)*3);
	normal  = rep_->normal   = (float *)malloc(sizeof(*(rep_->normal))*3*(rep_->ntri)*3);
	norindex= rep_->norindex = (int *)malloc(sizeof(*(rep_->norindex))*3*(rep_->ntri));
	
	/* face normals - one face per quad (ie, 2 triangles) 			*/
	/* have to make sure that if nctri is odd, that we increment by one	*/
	
	
	facenormals = (pt *)malloc(sizeof(*facenormals)*(rep_->ntri+1)/2);
	
	/* for each triangle vertex, tell me which face(s) it is in		*/
	pointfaces = (int *)malloc(sizeof(*pointfaces)*POINT_FACES*3*rep_->ntri);
	
	/* for each triangle, it has a defaultface...				*/
	defaultface = (int *)malloc(sizeof(*defaultface)*rep_->ntri);
	
	
	/*memory for the SCPs. Only needed in this function. Freed later	*/
	SCP     = (struct SCP *)malloc(sizeof(struct SCP)*nspi);
	 
	/* in C always check if you got the mem you wanted...  >;->		*/
	  if(!(pointfaces && defaultface && facenormals && cindex && coord && normal && norindex && SCP )) {
		freewrlDie("Not enough memory for Extrusion node triangles... ;(");
	} 
	
	if (HAVETODOTEXTURES) { /* texture mapping "stuff" */
		/* so, we now have to worry about textures. */
		/* XXX note - this over-estimates; realloc to be exact */
	
		tcoordsize = (nctri + (ntri*2))*3;
	
		if (Extru_Verbose) 
			printf ("tcoordsize is %d\n",tcoordsize);
		tcoord = rep_->tcoord = (float *)malloc(sizeof(*(rep_->tcoord))*tcoordsize);
	
		tcindexsize = rep_->ntri*3;
		if (Extru_Verbose) 
			printf ("tcindexsize %d\n",tcindexsize);
		tcindex  = rep_->tcindex   = (int *)malloc(sizeof(*(rep_->tcindex))*tcindexsize);
	
		/* keep around cross section info for tex coord mapping */
		beginVals = (float *)malloc(sizeof(float) * 2 * (nsec+1)*100);
		endVals = (float *)malloc(sizeof(float) * 2 * (nsec+1)*100);
	
		if (!(tcoord && tcindex && beginVals && endVals)) 
			freewrlDie ("Not enough memory Extrusion Tcoords");
	}
	
	/* Normal Generation Code */
	initialize_smooth_normals();
	HAVETOSMOOTH = smooth_normals && (fabs(creaseAngle)>0.0001);
	for (tmp = 0; tmp < 3*rep_->ntri; tmp++) {
		pointfaces[tmp*POINT_FACES]=0;
	}
	
	
	/************************************************************************
	 * calculate all SCPs 
	 */
	
	spine_is_one_vertix=0;
	
	/* fill the prev and next values in the SCP structs first
	 *
	 *	this is so complicated, because spine vertices can be the same
	 *	They should have exactly the same SCP, therefore only one of
	 *	an group of sucessive equal spine vertices (now called SESVs)
	 *	must be used for calculation.
	 *	For calculation the previous and next different spine vertix
	 *	must be known. We save that info in the prev and next fields of
	 *	the SCP struct. 
	 *	Note: We have start and end SESVs which will be treated differently
	 *	depending on whether the spine is closed or not
	 *
	 */
	 
	for(spi=0; spi<nspi;spi++){
		for(next_spi=spi+1;next_spi<nspi;next_spi++) {
			VEC_FROM_CDIFF(spine[spi],spine[next_spi],spp1);
			if(!APPROX(VECSQ(spp1),0))
				break;
		}
		if(next_spi<nspi) SCP[next_spi].prev=next_spi-1;
	
		if(Extru_Verbose) printf("spi=%d next_spi=%d\n",spi,next_spi); /**/
		prev_spi=spi-1;
		SCP[spi].next=next_spi;
		SCP[spi].prev=prev_spi;
		
		while(next_spi>spi+1) { /* fill gaps */
			spi++;
			SCP[spi].next=next_spi;
			SCP[spi].prev=prev_spi;
		}
	}
	/* now:	start-SEVS .prev fields contain -1				*/
	/* 	and end-SEVS .next fields contain nspi				*/
	
	
	/* calculate the SCPs now...						*/
	
	if (Extru_Verbose) printf (" SCP[0].next = %d, nspi = %d\n",SCP[0].next,nspi);
	
	
	if(SCP[0].next==nspi) {
		spine_is_one_vertix=1;
		if (Extru_Verbose) printf("All spine vertices are the same!\n");
	
		/* initialize all y and z values with zero, they will		*/
		/* be treated as colinear case later then			*/
		SCP[0].z.x=0; SCP[0].z.y=0; SCP[0].z.z=0;
		SCP[0].y=SCP[0].z;
		for(spi=1;spi<nspi;spi++) {
			SCP[spi].y=SCP[0].y;
			SCP[spi].z=SCP[0].z;
		}
	}else{
		if(Extru_Verbose) {
			for(spi=0;spi<nspi;spi++) {
				printf("SCP[%d].next=%d, SCP[%d].prev=%d\n",
					spi,SCP[spi].next,spi,SCP[spi].prev);
			}
		}
		
		/* find spine vertix different to the first spine vertix	*/
		spi=0; 		
		while(SCP[spi].prev==-1) spi++;
	
		/* find last spine vertix different to the last 		*/
		t=nspi-1; 
		while(SCP[t].next==nspi) t--;
	
		if (Extru_Verbose)
			printf ("now, spi = %d, t = %d\n",spi,t);
	
		/* for all but the first + last really different spine vertix	*/
		/* add case for then there are only 2 spines, and spi is already */
		/* spi is already greater than t... JAS				*/
	
		if (spi > t) {
			/* calc y 	*/
			VEC_FROM_CDIFF(spine[1],spine[0],SCP[0].y);
			/* calc z	*/
			VEC_FROM_CDIFF(spine[1],spine[0],spp1);
			VEC_FROM_CDIFF(spine[1],spine[0],spm1);
	 		VECCP(spp1,spm1,SCP[1].z);
			if (Extru_Verbose) {
			printf ("just calculated z for spi 0\n");
			printf("SCP[0].y=[%f,%f,%f], SCP[1].z=[%f,%f,%f]\n",
				SCP[0].y.x,SCP[0].y.y,SCP[0].y.z,
				SCP[1].z.x,SCP[1].z.y,SCP[1].z.z);
			}
		}
		
		else {
			for(; spi<=t; spi++) {
				/* calc y 	*/
				VEC_FROM_CDIFF(spine[SCP[spi].next],spine[SCP[spi].prev],SCP[spi].y);
				/* calc z	*/
				VEC_FROM_CDIFF(spine[SCP[spi].next],spine[spi],spp1);
				VEC_FROM_CDIFF(spine[SCP[spi].prev],spine[spi],spm1);
	 			VECCP(spp1,spm1,SCP[spi].z);
				if (Extru_Verbose) printf ("just calculated z for spi %d\n",spi);
	 		}
		}
	 
	 	if(circular) {
			if (Extru_Verbose) printf ("we are circular\n");
	 		/* calc y for first SCP				*/
			VEC_FROM_CDIFF(spine[SCP[0].next],spine[SCP[nspi-1].prev],SCP[0].y); 
	 		/* the last is the same as the first */	
	 		SCP[nspi-1].y=SCP[0].y;	
	        
			/* calc z */
			VEC_FROM_CDIFF(spine[SCP[0].next],spine[0],spp1);
			VEC_FROM_CDIFF(spine[SCP[nspi-1].prev],spine[0],spm1);
			VECCP(spp1,spm1,SCP[0].z);
			/* the last is the same as the first */	
			SCP[nspi-1].z=SCP[0].z;	
			
	 	} else {
			if (Extru_Verbose) printf ("we are not circular\n");
	
	 		/* calc y for first SCP				*/
			VEC_FROM_CDIFF(spine[SCP[0].next],spine[0],SCP[0].y);
	
	 		/* calc y for the last SCP			*/
			/* in the case of 2, nspi-1 = 1, ...prev = 0	*/
			VEC_FROM_CDIFF(spine[nspi-1],spine[SCP[nspi-1].prev],SCP[nspi-1].y);
	 
			/* z for the start SESVs is the same as for the next SCP */
			SCP[0].z=SCP[SCP[0].next].z; 
	 		/* z for the last SCP is the same as for the one before the last*/
			SCP[nspi-1].z=SCP[SCP[nspi-1].prev].z; 
		
			if (Extru_Verbose) {	
			printf("SCP[0].y=[%f,%f,%f], SCP[0].z=[%f,%f,%f]\n",
				SCP[0].y.x,SCP[0].y.y,SCP[0].y.z,
				SCP[0].z.x,SCP[0].z.y,SCP[0].z.z);
			printf("SCP[1].y=[%f,%f,%f], SCP[1].z=[%f,%f,%f]\n",
				SCP[1].y.x,SCP[1].y.y,SCP[1].y.z,
				SCP[1].z.x,SCP[1].z.y,SCP[1].z.z);
			}
		} /* else */
		
		/* fill the other start SESVs SCPs*/
		spi=1; 
		while(SCP[spi].prev==-1) {
			SCP[spi].y=SCP[0].y;
			SCP[spi].z=SCP[0].z;
			spi++;
		}
		/* fill the other end SESVs SCPs*/
		t=nspi-2; 
		while(SCP[t].next==nspi) {
			SCP[t].y=SCP[nspi-1].y;
			SCP[t].z=SCP[nspi-1].z;
			t--;
		}
	
	} /* else */
	
	
	/* We have to deal with colinear cases, what means z=0			*/
	pos_of_last_zvalue=-1;		/* where a zvalue is found */
	for(spi=0;spi<nspi;spi++) {
		if(pos_of_last_zvalue>=0) { /* already found one?		*/
			if(APPROX(VECSQ(SCP[spi].z),0)) 
				SCP[spi].z= SCP[pos_of_last_zvalue].z;
	
			pos_of_last_zvalue=spi;	
		} else 
			if(!APPROX(VECSQ(SCP[spi].z),0)) {
				/* we got the first, fill the previous		*/
				if(Extru_Verbose) printf("Found z-Value!\n");
				for(t=spi-1; t>-1; t--)
					SCP[t].z=SCP[spi].z;
	 			pos_of_last_zvalue=spi;	
			}
	}
	 
	if(Extru_Verbose) printf("pos_of_last_zvalue=%d\n",pos_of_last_zvalue);
	 
	 
	/* z axis flipping, if VECPT(SCP[i].z,SCP[i-1].z)<0 			*/
	/* we can do it here, because it is not needed in the all-colinear case	*/
	for(spi=(circular?2:1);spi<nspi;spi++) {
		if(VECPT(SCP[spi].z,SCP[spi-1].z)<0) {
			VECSCALE(SCP[spi].z,-1);
			if(Extru_Verbose) 
			    printf("Extrusion.GenPloyRep: Flipped axis spi=%d\n",spi);
		}
	} /* for */
	
	/* One case is missing: whole spine is colinear				*/
	if(pos_of_last_zvalue==-1) {
		if (Extru_Verbose) printf("Extrusion.GenPloyRep:Whole spine is colinear!\n");
	
		/* this is the default, if we don`t need to rotate		*/
		spy.x=0; spy.y=1; spy.z=0;	
		spz.x=0; spz.y=0; spz.z=1;
	
		if(!spine_is_one_vertix) {
			/* need to find the rotation from SCP[spi].y to (0 1 0)*/
			/* and rotate (0 0 1) and (0 1 0) to be the new y and z	*/
			/* values for all SCPs					*/
			/* I will choose roation about the x and z axis		*/
			double alpha,gamma;	/* angles for the rotation	*/
			
			/* search a non trivial vector along the spine */
			for(spi=1;spi<nspi;spi++) {
				VEC_FROM_CDIFF(spine[spi],spine[0],spp1);
				if(!APPROX(VECSQ(spp1),0))
	 				break;
	 		}
	 			
			/* normalize the non trivial vector */	
			spylen=1/sqrt(VECSQ(spp1)); VECSCALE(spp1,spylen);
			if(Extru_Verbose)
				printf("Reference vector along spine=[%f,%f,%f]\n",
					spp1.x,spp1.y,spp1.z);
	
	
			if(!(APPROX(spp1.x,0))) {
				/* get the angle for the x axis rotation	*/
				/* asin of 1.0000 seems to fail sometimes, so */
				if (spp1.x >= 0.99999) { alpha = asin(0.9999);
				} else if (spp1.x <= -0.99999) { alpha = asin(-0.9999);
				} else alpha=asin((double)spp1.x);
				if(APPROX(cos(alpha),0))
					gamma=0;
				else {
					gamma=acos(spp1.y / cos(alpha) );
					if(fabs(sin(gamma)-(-spp1.z/cos(alpha))
						)>fabs(sin(gamma)))
						gamma=-gamma;
				}
	
	 			if(Extru_Verbose) printf("alpha=%f gamma=%f\n",alpha,gamma);
	
				spy.y=-(cos(alpha)*(-sin(gamma)));
				spy.z=cos(alpha)*cos(gamma);
				spy.x=sin(alpha);
				spz.y=-(sin(alpha)*sin(gamma));
				spz.z=(-sin(alpha))*cos(gamma);
				spz.x=cos(alpha);
			} 
			if(!(APPROX(spp1.z,0))) {
				/* get the angle for the z axis rotation	*/
				/* asin of 1.0000 seems to fail sometimes, so */
				if (spp1.z >= 0.99999) { alpha = asin(0.9999);
				} else if (spp1.z <= -0.99999) { alpha = asin(-0.9999);
				} else alpha=asin((double)spp1.z);
				if(APPROX(cos(alpha),0))
					gamma=0;
				else {
					gamma=acos(spp1.y / cos(alpha) );
					if(fabs(sin(gamma)-(-spp1.x/cos(alpha))
						)>fabs(sin(gamma)))
						gamma=-gamma;
				}
	
	 			if(Extru_Verbose) printf("alpha=%f gamma=%f\n",alpha,gamma);
				spy.y=-(cos(alpha)*(-sin(gamma)));
				spy.x=cos(alpha)*cos(gamma);
				spy.z=sin(alpha);
				spz.y=-(sin(alpha)*sin(gamma));
				spz.x=(-sin(alpha))*cos(gamma);
				spz.z=cos(alpha);
			} 
	
	
		} /* else */
	 
		/* apply new y and z values to all SCPs	*/
		for(spi=0;spi<nspi;spi++) {
			SCP[spi].y=spy;
			SCP[spi].z=spz;
		}
	 
	} /* if all colinear */
	 
	if(Extru_Verbose) {
		for(spi=0;spi<nspi;spi++) {
			printf("SCP[%d].y=[%f,%f,%f], SCP[%d].z=[%f,%f,%f]\n",
				spi,SCP[spi].y.x,SCP[spi].y.y,SCP[spi].y.z,
				spi,SCP[spi].z.x,SCP[spi].z.y,SCP[spi].z.z);
		}
	}
	 
	
	/************************************************************************
	 * calculate the coords 
	 */
	
	/* test for number of scale and orientation parameters			*/
	if(nsca>1 && nsca <nspi)
		printf("Extrusion.GenPolyRep: Warning!\n"
		"\tNumber of scaling parameters do not match the number of spines!\n"
		"\tWill revert to using only the first scale value.\n");
	
	if(nori>1 && nori <nspi)
		printf("Extrusion.GenPolyRep: Warning!\n"
		"\tNumber of orientation parameters "
			"do not match the number of spines!\n"
		"\tWill revert to using only the first orientation value.\n");
	
	
	for(spi = 0; spi<nspi; spi++) {
		double m[3][3];		/* space for the rotation matrix	*/
		spy=SCP[spi].y; spz=SCP[spi].z;
		VECCP(spy,spz,spx);
		spylen = 1/sqrt(VECSQ(spy)); VECSCALE(spy, spylen);
		spzlen = 1/sqrt(VECSQ(spz)); VECSCALE(spz, spzlen);
		spxlen = 1/sqrt(VECSQ(spx)); VECSCALE(spx, spxlen);
	
		/* rotate spx spy and spz			*/
		if(nori) {
			int ori = (nori==nspi ? spi : 0);
			
			if(IS_ROTATION_VEC_NOT_NORMAL(orientation[ori]))
				printf("Extrusion.GenPolyRep: Warning!\n"
				  "\tRotationvector #%d not normal!\n"
				  "\tWon`t correct it, because it is bad VRML`97.\n",
				  ori+1); 
	 			
			MATRIX_FROM_ROTATION(orientation[ori],m);
			VECMM(m,spx);
			VECMM(m,spy);
			VECMM(m,spz);
		} 
	 
		for(sec = 0; sec<nsec; sec++) {
			struct pt point;
			float ptx = crossSection[sec].c[0];
			float ptz = crossSection[sec].c[1];
			if(nsca) {
				int sca = (nsca==nspi ? spi : 0);
				ptx *= ((this_->scale).p[sca]).c[0];
				ptz *= ((this_->scale).p[sca]).c[1];
	 		}
			point.x = ptx;
			point.y = 0; 
			point.z = ptz;
	
			//printf ("working on sec %d of %d, spine %d of %d\n", sec, nsec, spi, nspi);
	
	
		  /* texture mapping for caps - keep vals around */
		  if (HAVETODOTEXTURES) {
		  	if (spi == 0) { /* begin cap vertices */
				//printf ("begin cap vertecies index %d %d \n", sec*2+0, sec*2+1);
	
				beginVals[sec*2+0] = ptx;
				beginVals[sec*2+1] = ptz;
		   	} else if (spi == (nspi-1)) {  /* end cap vertices */
				//printf ("end cap vertecies index %d %d size %d\n", sec*2+0, sec*2+1, 2 * (nsec+1));
				endVals[(sec*2)+0]=ptx;
				endVals[(sec*2)+1]=ptz;
		   	} 
	
		   }
		   //printf ("coord index %x sec %d spi %d nsec %d\n",
		   //		&coord[(sec+spi*nsec)*3+0], sec, spi,nsec);
	
		   coord[(sec+spi*nsec)*3+0] = 
		    spx.x * point.x + spy.x * point.y + spz.x * point.z
		    + ((this_->spine).p[spi]).c[0];
		   coord[(sec+spi*nsec)*3+1] = 
		    spx.y * point.x + spy.y * point.y + spz.y * point.z
		    + ((this_->spine).p[spi]).c[1];
		   coord[(sec+spi*nsec)*3+2] = 
		    spx.z * point.x + spy.z * point.y + spz.z * point.z
		    + ((this_->spine).p[spi]).c[2];
	
		} /* for(sec */
	} /* for(spi */
	ncoord=nsec*nspi;
	 
	 
	/* freeing SCP coordinates. not needed anymore.				*/
	if(SCP) free(SCP);
	 
	/************************************************************************
	 * setting the values of *cindex to the right coords
	 */
	 
	triind = 0;
	{
	int x,z; 
	int A,B,C,D; /* should referr to the four vertices of the polygon	
			(hopefully) counted counter-clockwise, like
			 
			 D----C
			 |    |
			 |    |
			 |    |
			 A----B
			 
			*/
	int Atex, Btex, Ctex, Dtex, Etex, Ftex;	/* Tex Coord points */
	
	struct pt ac,bd,	/* help vectors	*/
		ab,cd;		/* help vectors	for testing intersection */
	int E,F;		/* third point to be used for the triangles*/	
	double u,r,		/* help variables for testing intersection */
		denominator,	/* ... */
		numerator;	/* ... */
	
	if(Extru_Verbose) {
		printf("Coords: \n");
	
		for(x=0; x<nsec; x++) {
		 for(z=0; z<nspi; z++) {
		 	int xxx = 3*(x+z*nsec);
		 	printf("coord: %d [%f %f %f] ",(x+z*nsec),
				coord[xxx], coord[xxx+1], coord[xxx+2]);
		 	
		 }
		printf("\n");
		}
		printf("\n");
	}
		
	
	/* Now, lay out the spines/sections, and generate triangles */
	
	for(x=0; x<nsec-1; x++) {
	  for(z=0; z<nspi-1; z++) {
	  A=x+z*nsec;
	  B=(x+1)+z*nsec;
	  C=(x+1)+(z+1)*nsec; 
	  D= x+(z+1)*nsec;
	
	  /* texture mapping coords */
	  Atex = A; Btex = B; Ctex = C; Dtex = D;
	
	  /* if we are circular, check to see if this is the first tri, or the last */
	  /* the vertexes are identical, but for smooth normal calcs, make the    */
	  /* indexes the same, too                                                */
	  /* note, we dont touch tex coords here.				  */
	  // printf ("x %d z %d nsec %d nspi %d\n",x,z,nsec,nspi);
	
	  if (tubular) {
		//printf ("tubular, x %d nsec %d this_face %d\n",x,nsec,this_face);
		if (x==(nsec-2)) {
			B -=(x+1);
			C -=(x+1);
		}
	  }
	
	  if (circular) {
		if (z==(nspi-2)) {
			/* last row in column, assume z=nspi-2, subtract this off */
			C -= (z+1)*nsec; 
			D -= (z+1)*nsec;
		}
	  }
	 
	  /* calculate the distance A-C and see, if it is smaller as B-D	*/
	  VEC_FROM_COORDDIFF(coord,C,coord,A,ac);
	  VEC_FROM_COORDDIFF(coord,D,coord,B,bd);
	
	  if(sqrt(VECSQ(ac))>sqrt(VECSQ(bd))) {
	  	E=B; F=D; Etex=Btex; Ftex=Dtex;
	  } else {
	  	E=C; F=A; Etex=Ctex; Ftex=Atex;
	  }
	
	  /* if concave polygons are expected, we also expect intersecting ones
	  	so we are testing, whether A-B and D-C intersect	*/
	  if(!((this_->convex))) {
	    	VEC_FROM_COORDDIFF(coord,B,coord,A,ab);
	  	VEC_FROM_COORDDIFF(coord,D,coord,C,cd);
		/* ca=-ac */
		if(Extru_Verbose) {
			printf("ab=[%f,%f,%f],cd=[%f,%f,%f]\n",
				ab.x,ab.y,ab.z,cd.x,cd.y,cd.z);
			printf("Orig: %d %d  [%f %f %f] [%f %f %f] (%d, %d, %d) \n",
					D, C,
					coord[D*3], coord[D*3+1], coord[D*3+2],
					coord[C*3], coord[C*3+1], coord[C*3+2],
					ncoord, nsec, nspi
			);
		}
		denominator= ab.y*cd.x-ab.x*cd.y;
		numerator  = (-ac.x)*cd.y-(-ac.y)*cd.x;
		
		r=u=-1;
		if(!APPROX(denominator,0)) {
			u=numerator/denominator;
			r=((-ac.x)*ab.y-(-ac.y)*ab.x)/denominator;
		} else {
			/* lines still may be coincident*/
			if(APPROX(numerator,0)) {
				/* we have to calculate u and r using the z coord*/
				denominator=ab.z*cd.x-ab.x*cd.z;
				numerator  = (-ac.x)*cd.z-(-ac.z)*cd.x;
				if(!APPROX(denominator,0)) {
				u=numerator/denominator;
				r=((-ac.x)*ab.y-(-ac.y)*ab.x)/denominator;
				} 
			}
		} /* else */
		if(Extru_Verbose) printf("u=%f, r=%f\n",u,r);
		if(u>=0 && u<=1 && r>=0 && r<=1 
			&& APPROX((-ac.x)+u*ab.x,r*cd.x)
			&& APPROX((-ac.y)+u*ab.y,r*cd.y)
			&& APPROX((-ac.z)+u*ab.z,r*cd.z)) {
			
			if(Extru_Verbose) printf("Intersection found at P=[%f,%f,%f]!\n",
				coord[A*3]+u*ab.x,
				coord[A*3+1]+u*ab.y,
				coord[A*3+2]+u*ab.y
				);
			coord[(ncoord)*3  ]=coord[A*3  ]+u*ab.x;
			coord[(ncoord)*3+1]=coord[A*3+1]+u*ab.y;
			coord[(ncoord)*3+2]=coord[A*3+2]+u*ab.z;
			E=ncoord;
			F=ncoord;
			ncoord_add++;
			ncoord++;
		}
	
	  } 
	
	  // printf ("Triangle1 %d %d %d\n",D,A,E); 
	  /* first triangle  calculate pointfaces, etc, for this face */
	  Elev_Tri(triind*3, this_face, D,A,E, TRUE , rep_, facenormals, pointfaces,ccw);
	
	  if (HAVETODOTEXTURES) {
		tcindex[triind*3] = Dtex;
		tcindex[triind*3+2] = Etex;
		tcindex[triind*3+1] = Atex;
	  }
	
	  defaultface[triind] = this_face;
	  triind++;
	
	  // printf ("Triangle2 %d %d %d\n",B,C,F);
	  /* second triangle - pointfaces, etc,for this face  */
	  Elev_Tri(triind*3, this_face, B, C, F, TRUE, rep_, facenormals, pointfaces,ccw);
	
	  if (HAVETODOTEXTURES) {
		tcindex[triind*3] = Btex;
		tcindex[triind*3+1] = Ctex;
		tcindex[triind*3+2] = Ftex;
	  }
	
	  if ((HAVETODOTEXTURES) && ((triind*3+2) >= tcindexsize)) 
		printf ("INTERNAL ERROR: Extrusion  - tcindex size too small!\n");
	  defaultface[triind] = this_face;
	  triind ++; 
	  this_face ++;
	
	 }
	}
	
	/* do normal calculations for the sides, here */
	for (tmp=0; tmp<(triind*3); tmp++) {
		if (HAVETOSMOOTH) {
			normalize_ifs_face (&rep_->normal[tmp*3],
				facenormals, pointfaces, cindex[tmp],
				defaultface[tmp/3], creaseAngle);
		} else {
			rep_->normal[tmp*3+0] = facenormals[defaultface[tmp/3]].x;
			rep_->normal[tmp*3+1] = facenormals[defaultface[tmp/3]].y;
			rep_->normal[tmp*3+2] = facenormals[defaultface[tmp/3]].z;
		}
		rep_->norindex[tmp] = tmp;
	}
	/* keep track of where the sides end, triangle count-wise, for Normal mapping */
	end_of_sides = triind*3;
	
	/* tcindexes are TOTALLY different from sides  - set this in case we are 
	   doing textures in the end caps */
	tci_ct = nspi*nsec; 

	if(((this_->convex))) {
		int endpoint;
	
		int triind_start; 	/* textures need 2 passes */
	
		/* if not tubular, we need one more triangle */
		if (tubular) endpoint = nsec-3-ncolinear_at_end;
		else endpoint = nsec-2-ncolinear_at_end;
	
	
		//printf ("beginCap, starting at triind %d\n",triind);
	
		/* this is the simple case with convex polygons	*/
		if(((this_->beginCap))) {
			triind_start = triind;
	
			for(x=0+ncolinear_at_begin; x<endpoint; x++) {
	  			Elev_Tri(triind*3, this_face, 0, x+2, x+1, TRUE , rep_, facenormals, pointfaces,ccw);
	  			defaultface[triind] = this_face;
				if (HAVETODOTEXTURES)
					Extru_tex(triind*3, tci_ct, 0 , +x+2, x+1, rep_,ccw,tcindexsize);
				triind ++;
			}
	
			if(HAVETODOTEXTURES) {
				Extru_ST_map(triind_start,0+ncolinear_at_begin,endpoint,
					beginVals,nsec,rep_,tcoordsize);
				tci_ct+=endpoint-(0+ncolinear_at_begin);
			}
			triind_start+=endpoint-(0+ncolinear_at_begin);
			this_face++;
		} /* if beginCap */
		
		if(((this_->endCap))) {
			triind_start = triind;
	
			for(x=0+ncolinear_at_begin; x<endpoint; x++) {
	  			Elev_Tri(triind*3, this_face, 0  +(nspi-1)*nsec,
					x+1+(nspi-1)*nsec,x+2+(nspi-1)*nsec,
					TRUE , rep_, facenormals, pointfaces,ccw);
	  			defaultface[triind] = this_face;
				if (HAVETODOTEXTURES) 
					Extru_tex(triind*3, tci_ct, 0+(nspi-1)*nsec, 
						x+1+(nspi-1)*nsec, 
						x+2+(nspi-1)*nsec, rep_,ccw,tcindexsize);
				triind ++;
			}
			this_face++;
			if (HAVETODOTEXTURES) 
				Extru_ST_map(triind_start,0+ncolinear_at_begin,endpoint,
					endVals, nsec, rep_,tcoordsize);
		} /* if endCap */
	 	//for (tmp=0;tmp<tcindexsize; tmp++) printf ("index1D %d tcindex %d\n",tmp,tcindex[tmp]);
		
	} else 
	    if(((this_->beginCap))||((this_->endCap))) { 
		/* polygons might be concave-> do tessellation			*/
		/* XXX - no textures yet - Linux Tesselators give me enough headaches; 
		   lets wait until they are all ok before trying texture mapping */
	
		/* give us some memory - this array will contain tessd triangle counts */
		int *tess_vs;
		struct SFColor *c1;
		GLdouble tess_v[3]; 
		int endpoint;

		tess_vs=(int *)malloc(sizeof(*(tess_vs)) * (nsec - 3 - ncolinear_at_end) * 3);
		if (!(tess_vs)) freewrlDie ("Extrusion - no memory for tesselated end caps");
	
		/* if not tubular, we need one more triangle */
		if (tubular) endpoint = nsec-1-ncolinear_at_end;
		else endpoint = nsec-ncolinear_at_end;
	
			
		if(((this_->beginCap))) {
			global_IFS_Coord_count = 0;
			gluBeginPolygon(global_tessobj);
	
			for(x=0+ncolinear_at_begin; x<endpoint; x++) {
				//printf ("starting tv for x %d of %d\n",x,endpoint);
	                	c1 = (struct SFColor *) &rep_->coord[3*x];
				//printf ("and, coords for this one are: %f %f %f\n",
				//		c1->c[0], c1->c[1],c1->c[2]);

				tess_v[0] = c1->c[0]; tess_v[1] = c1->c[1]; tess_v[2] = c1->c[2];
				tess_vs[x] = x;
				gluTessVertex(global_tessobj,tess_v,&tess_vs[x]);
			}
			gluEndPolygon(global_tessobj);
			verify_global_IFS_Coords(ntri*3);

			for (x=0; x<global_IFS_Coord_count; x+=3) {
				//printf ("now, in 2nd for loop, x %d glob %d\n",x,
				//		global_IFS_Coord_count);
	  			Elev_Tri(triind*3, this_face, global_IFS_Coords[x], 
					global_IFS_Coords[x+2], global_IFS_Coords[x+1],
					TRUE , rep_, facenormals, pointfaces,ccw);
	  			defaultface[triind] = this_face;
				triind ++;
			}
			/* Tesselated faces may have a different normal than calculated previously */
			Extru_check_normal (facenormals,this_face,-1,rep_,ccw);
	
			this_face++;
		}	
		
		if(((this_->endCap))){	
			global_IFS_Coord_count = 0;
			gluBeginPolygon(global_tessobj);
	
			for(x=0+ncolinear_at_begin; x<endpoint; x++) {
	                	c1 = (struct SFColor *) &rep_->coord[3*(x+(nspi-1)*nsec)];
				tess_v[0] = c1->c[0]; tess_v[1] = c1->c[1]; tess_v[2] = c1->c[2];
				tess_vs[x] = x+(nspi-1)*nsec;
				gluTessVertex(global_tessobj,tess_v,&tess_vs[x]);
			}
			gluEndPolygon(global_tessobj);
			verify_global_IFS_Coords(ntri*3);
	
			for (x=0; x<global_IFS_Coord_count; x+=3) {
	  			Elev_Tri(triind*3, this_face, global_IFS_Coords[x], 
					global_IFS_Coords[x+1], global_IFS_Coords[x+2],
					TRUE , rep_, facenormals, pointfaces,ccw);
	  			defaultface[triind] = this_face;
				triind ++;
			}
			/* Tesselated faces may have a different normal than calculated previously */
			Extru_check_normal (facenormals,this_face,1,rep_,ccw);
	
			this_face++;
		}
	
		/* get rid of mallocd memory  for tess */
		free (tess_vs);
	
	    } /* elseif */
	} /* end of block */
	
	/* if we have tesselated, we MAY have fewer triangles than estimated, so... */
	rep_->ntri=triind;
	
	//for (tmp=0;tmp<tcindexsize; tmp++) printf ("index2 %d tcindex %d\n",tmp,tcindex[tmp]);
	/* do normal calculations for the caps here note - no smoothing */
	for (tmp=end_of_sides; tmp<(triind*3); tmp++) {
		rep_->normal[tmp*3+0] = facenormals[defaultface[tmp/3]].x;
		rep_->normal[tmp*3+1] = facenormals[defaultface[tmp/3]].y;
		rep_->normal[tmp*3+2] = facenormals[defaultface[tmp/3]].z;
		rep_->norindex[tmp] = tmp;
	}
	
	/* do texture mapping calculations for sides */
	if (HAVETODOTEXTURES) {
		/* range check - this should NEVER happen... */
		if (tcoordsize <= ((nsec-1)+(nspi-1)*(nsec-1)*3+2)) {
			printf ("INTERNAL ERROR: Extrusion side tcoord calcs nspi %d nsec %d tcoordsize %d\n",
				nspi,nsec,tcoordsize);
		}
		for(sec=0; sec<nsec; sec++) {
			for(spi=0; spi<nspi; spi++) {
				//printf ("tcoord idx %d %d %d tcoordsize %d ",
				//(sec+spi*nsec)*3,(sec+spi*nsec)*3+1,(sec+spi*nsec)*3+2,tcoordsize);
				//printf ("side texts sec %d spi %d\n",sec,spi);
				tcoord[(sec+spi*nsec)*3+0] = (float) sec/(nsec-1);
				tcoord[(sec+spi*nsec)*3+1] = 0;
				tcoord[(sec+spi*nsec)*3+2] = (float) spi/(nspi-1);
				//printf (" %f %f\n",tcoord[(sec+spi*nsec)*3+0],tcoord[(sec+spi*nsec)*3+2]);
			}
		}	
	}
	
	if (Extru_Verbose) printf ("done, lets free\n");
	
	/* we no longer need to keep normal-generating memory around */
	free (defaultface);
	free (pointfaces);
	free (facenormals);
	free (crossSection);
	
	if (HAVETODOTEXTURES) {
		free (beginVals); 
		free (endVals);
	}
	
	
	if(Extru_Verbose)
		printf("Extrusion.GenPloyRep: triind=%d  ntri=%d nctri=%d "
		"ncolinear_at_begin=%d ncolinear_at_end=%d\n",
		triind,ntri,nctri,ncolinear_at_begin,ncolinear_at_end);
	 
	if(Extru_Verbose) 
		printf ("end VRMLExtrusion.pm\n");
	
	/*****end of Member Extrusion	*/
}
