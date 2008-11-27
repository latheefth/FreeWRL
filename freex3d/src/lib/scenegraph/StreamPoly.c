/*
=INSERT_TEMPLATE_HERE=

$Id$

???

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeX3D.h>

#include "../vrml_parser/Structs.h" /* point_XYZ */
#include "../main/headers.h"

#include "Polyrep.h"


#define NO_TCOORD_GEN_IN_SHAPE (r->GeneratedTexCoords == 0)
#define NO_TEXCOORD_NODE (r->tcoordtype==0)
#define MUST_GENERATE_TEXTURES (NO_TCOORD_GEN_IN_SHAPE && NO_TEXCOORD_NODE)

void defaultTextureMap(struct X3D_IndexedFaceSet *p, struct X3D_PolyRep *r, struct SFColor *points, int npoints);

/********************************************************************
*
* stream_polyrep
*
*  convert a polyrep into a structure format that displays very
*  well, especially on fast graphics hardware
*
* many shapes go to a polyrep structure; Extrusions, ElevationGrids,
* IndexedFaceSets, and all of the Triangle nodes.
*
* This is stage 2 of the polyrep build process; the first stage is
* (for example) make_indexedfaceset(node); it creates a polyrep
* structure. 
*
* This stage takes that polyrep structure, and finishes it in a 
* generic fashion, and makes it "linear" so that it can be rendered
* very quickly by the GPU.
*
* we ALWAYS worry about texture coords, even if this geometry does not
* have a texture in the associated geometry node; you *never* know
* when that Appearance node will change. Some nodes, eg ElevationGrid,
* will automatically generate texture coordinates so they are outside
* of this scope.
*
*********************************************************************/

/* texture generation points... */
int Sindex;
int Tindex;
GLfloat minVals[3];
GLfloat Ssize;
/*
static GLfloat maxVals[] = {-99999.9, -999999.9, -99999.0};
static GLfloat Tsize = 0.0;
static GLfloat Xsize = 0.0;
static GLfloat Ysize = 0.0;
static GLfloat Zsize = 0.0;
*/

/* take 3 or 4 floats, bounds check them, and put them in a destination. 
   Used for copying color X3DColorNode values over for streaming the
   structure. */

static void do_glColor4fv(struct SFColorRGBA *dest, GLfloat *param, int isRGBA, GLfloat thisTransparency) {
	int i;
	int pc;

	if (isRGBA) pc = 4; else pc = 3;

	/* parameter checks */
	for (i=0; i<pc; i++) {
		if ((param[i] < 0.0) || (param[i] >1.0)) {
			param[i] = 0.5;
		}
	}
	dest->r[0] = param[0];
	dest->r[1] = param[1];
	dest->r[2] = param[2];

	/* does this color have an alpha channel? */
	if (isRGBA) {
		dest->r[3] = param[3];
	} else {
		dest->r[3] = thisTransparency;
	}
}

void stream_polyrep(void *node, void *coord, void *color, void *normal, void *texCoord) {

	struct X3D_IndexedFaceSet *p;
	struct X3D_PolyRep *r;
	int i, j;
	int hasc;
	GLfloat thisTrans;

	struct SFColor *points=0; int npoints;
	struct SFColor *colors=0; int ncolors=0;
	struct SFColor *normals=0; int nnormals=0;
	int isRGBA = FALSE;

	struct X3D_Coordinate *xc;
	struct X3D_Color *cc;
	struct X3D_Normal *nc;
	struct X3D_TextureCoordinate *tc;


	/* new memory locations for new data */
	int *newcindex;
	int *newtcindex;
	struct SFColor *newpoints;
	struct SFColor *newnorms;
	struct SFColorRGBA *newcolors;
	struct SFColorRGBA *oldColorsRGBA;
	float *newtc;

	/* get internal structures */
	p = (struct X3D_IndexedFaceSet *)node;
	r = (struct X3D_PolyRep *)p->_intern;

	/* printf ("stream_polyrep, at start, we have %d triangles texCoord %u\n",r->ntri,texCoord);  */

	/* does this one have any triangles here? (eg, an IFS without coordIndex) */
	if (r->ntri==0) {
		printf ("stream IFS, at start, this guy is empty, just returning \n");
		return;
	}


	/* sanity check parameters, and get numbers */
	if(coord) {
		xc = (struct X3D_Coordinate *) coord;
		if (xc->_nodeType != NODE_Coordinate) {
			printf ("stream_polyrep, coord expected %d, got %d\n",NODE_Coordinate, xc->_nodeType);
			r->ntri=0; return;
		} else { points = xc->point.p; npoints = xc->point.n; }
	}

	if (color) {
		cc = (struct X3D_Color *) color;
		if ((cc->_nodeType != NODE_Color) && (cc->_nodeType != NODE_ColorRGBA)) {
			printf ("stream_polyrep, expected %d got %d\n", NODE_Color, cc->_nodeType);
			r->ntri=0; return;
		} else { 
			colors = cc->color.p; 
			ncolors = cc->color.n; 
			isRGBA = (cc->_nodeType == NODE_ColorRGBA); 
		}
	}
	
	if(normal) {
		nc = (struct X3D_Normal *) normal;
		if (nc->_nodeType != NODE_Normal) {
			printf ("stream_polyrep, normal expected %d, got %d\n",NODE_Normal, nc->_nodeType);
			r->ntri=0; return;
		} else { normals = nc->vector.p; nnormals = nc->vector.n; }
	}

	if (texCoord) {
		tc = (struct X3D_TextureCoordinate *) texCoord;
		if ((tc->_nodeType != NODE_TextureCoordinate) && 
			(tc->_nodeType != NODE_MultiTextureCoordinate) &&
			(tc->_nodeType != NODE_TextureCoordinateGenerator )) {
			printf ("stream_polyrep, TexCoord expected %d, got %d\n",NODE_TextureCoordinate, tc->_nodeType);
			r->ntri=0; return;
		}
	}

	#ifdef STREAM_POLY_VERBOSE
	printf ("\nstart stream_polyrep ncoords %d ncolors %d nnormals %d ntri %d\n",
			npoints, ncolors, nnormals, r->ntri);
	#endif


	#ifdef STREAM_POLY_VERBOSE
	printf ("stream polyrep, have an intern type of %d GeneratedTexCoords %d tcindex %d\n",r->tcoordtype, r->GeneratedTexCoords,r->tcindex);
	printf ("polyv, points %d coord %d ntri %d rnormal nnormal\n",points,r->actualCoord,r->ntri,r->normal, nnormals);
	#endif

	/* Do we have any colours? Are textures, if present, not RGB? */
	hasc = ((ncolors || r->color) && (last_texture_type!=TEXTURE_NO_ALPHA));

	if MUST_GENERATE_TEXTURES {
		#ifdef STREAM_POLY_VERBOSE
		printf ("mustGenerateTextures, MALLOCing newtc\n");
		#endif

		newtc = (float *) MALLOC (sizeof (float)*2*r->ntri*3);
	} else {
		newtc = 0;  	/*  unless we have to use it; look for MALLOC below*/
	}

	newcolors=0;	/*  only if we have colours*/

	/* MALLOC required memory */
	newcindex = (int*)MALLOC (sizeof (int)*r->ntri*3);
	newtcindex = (int*)MALLOC (sizeof (int)*r->ntri*3);

	newpoints = (struct SFColor*)MALLOC (sizeof (struct SFColor)*r->ntri*3);

	if ((nnormals) || (r->normal)) {
		newnorms = (struct SFColor*)MALLOC (sizeof (struct SFColor)*r->ntri*3);
	} else newnorms = 0;


	/* if we have colours, make up a new structure for them to stream to, and also
	   copy pointers to ensure that we index through colorRGBAs properly. */
	if (hasc) {
		newcolors = (struct SFColorRGBA*)MALLOC (sizeof (struct SFColorRGBA)*r->ntri*3);
		oldColorsRGBA = (struct SFColorRGBA*) colors;
	}

	/* gather the min/max values for x,y, and z for default texture mapping, and Collisions */
	for (j=0; j<3; j++) {
		if (points) {
			r->minVals[j] = points[r->cindex[0]].c[j];
			r->maxVals[j] = points[r->cindex[0]].c[j];
		} else {
			if (r->actualCoord!=NULL) {
				r->minVals[j] = r->actualCoord[3*r->cindex[0]+j];
				r->maxVals[j] = r->actualCoord[3*r->cindex[0]+j];
			}
		}
	}


	for(i=0; i<r->ntri*3; i++) {
	  int ind = r->cindex[i];
	  for (j=0; j<3; j++) {
	      if(points) {
		    if (r->minVals[j] > points[ind].c[j]) r->minVals[j] = points[ind].c[j];
		    if (r->maxVals[j] < points[ind].c[j]) r->maxVals[j] = points[ind].c[j];
	      } else if(r->actualCoord) {
		    if (r->minVals[j] >  r->actualCoord[3*ind+j]) r->minVals[j] =  r->actualCoord[3*ind+j];
		    if (r->maxVals[j] <  r->actualCoord[3*ind+j]) r->maxVals[j] =  r->actualCoord[3*ind+j];
	      }
	  }
	}

	/* do we need to generate default texture mapping? */
	if (MUST_GENERATE_TEXTURES) defaultTextureMap(p, r, points, npoints);

	/* figure out transparency for this node. Go through scene graph, and looksie for it. */
	thisTrans = 0.0;
	/* 
	printf ("figuring out what the transparency of this node is \n");
	printf ("nt %s\n",stringNodeType(X3D_NODE(node)->_nodeType));
	*/
	/* parent[0] should be a NODE_Shape */
	{ 
		struct X3D_Shape *parent;
		if (X3D_NODE(node)->_nparents != 0) {
			parent = X3D_SHAPE(X3D_NODE(node)->_parents[0]);
			/* printf ("nt, parent is of type %s\n",stringNodeType(parent->_nodeType)); */
			if (parent->_nodeType == NODE_Shape) {
				struct X3D_Appearance *app;
                		POSSIBLE_PROTO_EXPANSION(parent->appearance,app)
				/* printf ("appearance is of type %s\n",stringNodeType(app->_nodeType)); */
				if (app != NULL)  {
					if (app->_nodeType == NODE_Appearance) {
						struct X3D_Material *mat;
                				POSSIBLE_PROTO_EXPANSION(app->material,mat)
						/* printf ("material is of type %s\n",stringNodeType(mat->_nodeType)); */

						if (mat != NULL) {
							if (mat->_nodeType == NODE_Material) {
								thisTrans = mat->transparency;
								/* printf ("Set transparency to %f\n",thisTrans); */
							}
						}
					}
				}
			}
		}
	}

	/* now, lets go through the old, non-linear polyrep structure, and
	   put it in a stream format */

	for(i=0; i<r->ntri*3; i++) {
		int nori = i;
		int coli = i;
		int ind = r->cindex[i];

		/* new cindex, this should just be a 1.... ntri*3 linear string */
		newcindex[i] = i;
		newtcindex[i]=i;

		#ifdef STREAM_POLY_VERBOSE
		printf ("rp, i, ntri*3 %d %d\n",i,r->ntri*3);
		#endif

		/* get normals and colors, if any	*/
		if(r->norindex) { nori = r->norindex[i];}
		else nori = ind;

		if(r->colindex) {
			coli = r->colindex[i];
		}
		else coli = ind;

		/* get texture coordinates, if any	*/
		if (r->tcindex) {
			newtcindex[i] = r->tcindex[i];
			#ifdef STREAM_POLY_VERBOSE
				printf ("have textures, and tcindex i %d tci %d\n",i,newtcindex[i]);
			#endif
		}
		/* printf ("for index %d, tci is %d\n",i,newtcindex[i]); */

		/* get the normals, if there are any	*/
		if(nnormals) {
			if(nori >= nnormals) {
				/* bounds check normals here... */
				nori=0;
			}
			#ifdef STREAM_POLY_VERBOSE
				printf ("nnormals at %d , nori %d ",(int) &normals[nori].c,nori);
				fwnorprint (normals[nori].c);
			#endif

			do_glNormal3fv(&newnorms[i], normals[nori].c);
		} else if(r->normal) {
			#ifdef STREAM_POLY_VERBOSE
				printf ("r->normal nori %d ",nori);
				fwnorprint(r->normal+3*nori);
			#endif

			do_glNormal3fv(&newnorms[i], r->normal+3*nori);
		}

		if(hasc) {
			if(ncolors) {
				/* ColorMaterial -> these set Material too */
				/* bounds check colors[] here */
				if (coli >= ncolors) {
					/* printf ("bounds check for Colors! have %d want %d\n",ncolors-1,coli);*/
					coli = 0;
				}
				#ifdef STREAM_POLY_VERBOSE
					printf ("coloUr ncolors %d, coli %d",ncolors,coli);
					fwnorprint(colors[coli].c);
					printf ("\n");
				#endif
				if (isRGBA)
					do_glColor4fv(&newcolors[i],oldColorsRGBA[coli].r,isRGBA,thisTrans);
				else
					do_glColor4fv(&newcolors[i],colors[coli].c,isRGBA,thisTrans);
			} else if(r->color) {
				#ifdef STREAM_POLY_VERBOSE
					printf ("coloUr");
					fwnorprint(r->color+3*coli);
					printf ("\n");
				#endif
				if (isRGBA)
					do_glColor4fv(&newcolors[i],r->color+4*coli,isRGBA,thisTrans);
				else
					do_glColor4fv(&newcolors[i],r->color+3*coli,isRGBA,thisTrans);
			}
		}


		/* Coordinate points	*/
		if(points) {
			memcpy (&newpoints[i], &points[ind].c[0],sizeof (struct SFColor));
			/* XYZ[0]= points[ind].c[0]; XYZ[1]= points[ind].c[1]; XYZ[2]= points[ind].c[2];*/
			#ifdef STREAM_POLY_VERBOSE
				printf("Render (points) #%d = [%.5f, %.5f, %.5f]\n",i,
					newpoints[i].c[0],newpoints[i].c[1],newpoints[i].c[2]);
			#endif
		} else if(r->actualCoord) {
			memcpy (&newpoints[i].c[0], &r->actualCoord[3*ind], sizeof(struct SFColor));
			/* XYZ[0]=r->actualCoord[3*ind+0]; XYZ[1]=r->actualCoord[3*ind+1]; XYZ[2]=r->actualCoord[3*ind+2];*/
			#ifdef STREAM_POLY_VERBOSE
				printf("Render (r->actualCoord) #%d = [%.5f, %.5f, %.5f]\n",i,
					newpoints[i].c[0],newpoints[i].c[1],newpoints[i].c[2]);
			#endif
		}


		/* Textures	*/
		if (MUST_GENERATE_TEXTURES) {
			/* default textures */
			/* we want the S values to range from 0..1, and the
			   T values to range from 0...S/T */
			newtc[i*2]   = (newpoints[i].c[Sindex] - minVals[Sindex])/Ssize;
			newtc[i*2+1] = (newpoints[i].c[Tindex] - minVals[Tindex])/Ssize;
		}

		/* calculate maxextents */
		if (newpoints[i].c[0] > p->EXTENT_MAX_X) p->EXTENT_MAX_X = newpoints[i].c[0];
		if (newpoints[i].c[0] < p->EXTENT_MIN_X) p->EXTENT_MIN_X = newpoints[i].c[0];
		if (newpoints[i].c[1] > p->EXTENT_MAX_Y) p->EXTENT_MAX_Y = newpoints[i].c[1];
		if (newpoints[i].c[1] < p->EXTENT_MIN_Y) p->EXTENT_MIN_Y = newpoints[i].c[1];
		if (newpoints[i].c[2] > p->EXTENT_MAX_Z) p->EXTENT_MAX_Z = newpoints[i].c[2];
		if (newpoints[i].c[2] < p->EXTENT_MIN_Z) p->EXTENT_MIN_Z = newpoints[i].c[2];
	}

	/* free the old, and make the new current. Just in case threading on a multiprocessor
	   machine comes walking through and expects to stream... */
	FREE_IF_NZ(r->actualCoord);
	r->actualCoord = (float *)newpoints;
	FREE_IF_NZ(r->normal);
	r->normal = (float *)newnorms;
	FREE_IF_NZ(r->cindex);
	r->cindex = newcindex;

	/* did we have to generate tex coords? */
	if (newtc != 0) {
		FREE_IF_NZ(r->GeneratedTexCoords);
		r->GeneratedTexCoords = newtc;
	}

	FREE_IF_NZ(r->color);
	FREE_IF_NZ(r->colindex);
	r->color = (float *)newcolors;

	/* texture index */
	FREE_IF_NZ(r->tcindex);
	r->tcindex=newtcindex; 

	/* we dont require these indexes any more */
	FREE_IF_NZ(r->norindex);

	#ifdef STREAM_POLY_VERBOSE
		printf ("end stream_polyrep - ntri %d\n\n",r->ntri);
	#endif

	/* finished streaming, tell the rendering thread that we can now display this one */
	r->streamed=TRUE;

	/* record the transparency, in case we need to re-do this field */
	r->transparency = thisTrans;
	r->isRGBAcolorNode = isRGBA;
}



void defaultTextureMap(struct X3D_IndexedFaceSet *p, struct X3D_PolyRep * r, struct SFColor *points, int npoints) {

	/* variables used only in this routine */
	GLfloat Tsize = 0.0;
	GLfloat Xsize = 0.0;
	GLfloat Ysize = 0.0;
	GLfloat Zsize = 0.0;

	/* initialize variables used in other routines in this file. */
	Sindex = 0; Tindex = 0;
	Ssize = 0.0;
	minVals[0]=r->minVals[0]; 
	minVals[1]=r->minVals[1]; 
	minVals[2]=r->minVals[2]; 

		#ifdef STREAM_POLY_VERBOSE
		printf ("have to gen default textures\n");
		#endif

		if ((p->_nodeType == NODE_IndexedFaceSet) ||(p->_nodeType == NODE_ElevationGrid)) {

			/* find the S,T mapping. */
			Xsize = r->maxVals[0]-minVals[0];
			Ysize = r->maxVals[1]-minVals[1];
			Zsize = r->maxVals[2]-minVals[2];

			/* printf ("defaultTextureMap, %f %f %f\n",Xsize,Ysize,Zsize); */

	
			if ((Xsize >= Ysize) && (Xsize >= Zsize)) {
				/* X size largest */
				Ssize = Xsize; Sindex = 0;
				if (Ysize >= Zsize) { Tsize = Ysize; Tindex = 1;
				} else { Tsize = Zsize; Tindex = 2; }
			} else if ((Ysize >= Xsize) && (Ysize >= Zsize)) {
				/* Y size largest */
				Ssize = Ysize; Sindex = 1;
				if (Xsize >= Zsize) { Tsize = Xsize; Tindex = 0;
				} else { Tsize = Zsize; Tindex = 2; }
			} else {
				/* Z is the largest */
				Ssize = Zsize; Sindex = 2;
				if (Xsize >= Ysize) { Tsize = Xsize; Tindex = 0;
				} else { Tsize = Ysize; Tindex = 1; }
			}
		}
}