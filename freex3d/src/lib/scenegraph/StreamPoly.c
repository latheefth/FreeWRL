/*


???

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"
#include "../scenegraph/RenderFuncs.h"
#include "../scenegraph/Component_Shape.h"

#include "Polyrep.h"

#define NO_TEXCOORD_NODE (r->tcoordtype==0)


static void defaultTextureMap(struct X3D_Node *p, struct X3D_PolyRep *r); 

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
//int Sindex;
//int Tindex;
//GLfloat minVals[3];
//GLfloat Ssize;

typedef struct pStreamPoly{
	int Sindex;
	int Tindex;
	GLfloat minVals[3];
	GLfloat Ssize;
}* ppStreamPoly;
void *StreamPoly_constructor(){
	void *v = MALLOCV(sizeof(struct pStreamPoly));
	memset(v,0,sizeof(struct pStreamPoly));
	return v;
}
void StreamPoly_init(struct tStreamPoly *t){
	//public
	//private
	t->prv = StreamPoly_constructor();

	// JAS {ppStreamPoly p = (ppStreamPoly)t->prv;}
}
//ppStreamPoly p = (ppStreamPoly)gglobal()->StreamPoly.prv;

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

	/* if (isRGBA) printf ("do_glColor4fv, isRGBA\n"); else printf ("do_glColor4fv, NOT RGBA, setting alpha to thisTransparency %f\n",thisTransparency); */

	/* parameter checks */
	for (i=0; i<pc; i++) {
		if ((param[i] < 0.0) || (param[i] >1.0)) {
			param[i] = 0.5f;
		}
	}
	dest->c[0] = param[0];
	dest->c[1] = param[1];
	dest->c[2] = param[2];

	/* does this color have an alpha channel? */
	if (isRGBA) {
		dest->c[3] = param[3];
	} else {
		/* we calculate the transparency of the node. VRML 0.0 = fully visible, OpenGL 1.0 = fully visible */
		dest->c[3] = 1.0f - thisTransparency;
	}
	/* printf ("do_glColor4fv, resulting is R %f G %f B %f A %f\n",dest->c[0],dest->c[1],dest->c[2], dest->c[3]); */
}


void stream_polyrep(void *innode, void *coord, void *fogCoord, void *color, void *normal, struct X3D_TextureCoordinate *texCoordNode) {
    
	struct X3D_Node *node;
	struct X3D_PolyRep *r;
	int i, j, k, nmtexcoord;
	int hasc;
	GLfloat thisTrans;

	struct SFVec3f *points= NULL; int npoints=0;
	struct SFColor *colors= NULL; int ncolors=0;
	struct SFVec3f *normals= NULL; int nnormals=0;
	float *fogpoints = NULL;
	int isRGBA = FALSE;

	struct X3D_Coordinate *xc;
	struct X3D_FogCoordinate *fc;
	struct X3D_Color *cc;
	struct X3D_Normal *nc;

	/* new memory locations for new data */
	GLuint *newcindex = NULL;
	GLuint *newtcindex = NULL;
	struct SFVec3f *newpoints = NULL;
	float *newfog = NULL;
	struct SFVec3f *newnorms = NULL;
	struct SFColorRGBA *newcolors = NULL;
	struct SFColorRGBA *oldColorsRGBA = NULL;
	float *newTexCoords[MAX_MULTITEXTURE];
	bool temp_points = FALSE;
    struct Multi_Vec2f *textureCoordPoint[MAX_MULTITEXTURE];
	int ntexdim[MAX_MULTITEXTURE];
	for(k=0;k<MAX_MULTITEXTURE;k++){
		textureCoordPoint[k] = NULL;
		newTexCoords[k] = NULL;
		ntexdim[k] = 2;
	}
	nmtexcoord = 0; //number of multitextureCoordinates 0-4

	/* get internal structures */
	node = X3D_NODE(innode);
	r = node->_intern;
	r->actualFog = NULL;
    
	#ifdef STREAM_POLY_VERBOSE
	printf ("start spv for %p extents %lf %lf, %lf %lf, %lf %lf\n",node,
		node->EXTENT_MIN_X,
		node->EXTENT_MAX_X,
		node->EXTENT_MIN_Y,
		node->EXTENT_MAX_Y,
		node->EXTENT_MIN_Z,
		node->EXTENT_MAX_Z
	);
	#endif

	/* printf ("stream_polyrep, at start, we have %d triangles texCoord %u\n",r->ntri,texCoord);  */

	/* does this one have any triangles here? (eg, an IFS without coordIndex) */
	if (r->ntri==0) {
		printf ("stream IFS, at start, this guy is empty, just returning \n");
		return;
	}
    
	/* sanity check parameters, and get numbers */
	if (coord) {
		xc = (struct X3D_Coordinate *) coord;
		if (xc->_nodeType != NODE_Coordinate && xc->_nodeType != NODE_GeoCoordinate ) {
			printf ("stream_polyrep, coord expected %d, got %d\n",NODE_Coordinate, xc->_nodeType);
			r->ntri=0; 
			return;
		} else if(xc->_nodeType == NODE_GeoCoordinate){
			//int i,j;
			struct X3D_GeoCoordinate *xgc = (struct X3D_GeoCoordinate *) coord;
			if(0)
			{
				temp_points = true;
				points = MALLOC(struct SFVec3f *, sizeof(struct SFVec3f)*(xgc->point.n));
				npoints = xgc->point.n;
				for(i=0;i<npoints;i++)
				{
					for(j=0;j<3;j++)
						points[i].c[j] = (float) xgc->point.p[i].c[j]; //point is in geographic lat/lon
				}
			}else{
				points = xgc->__movedCoords.p;  //moved is in GC - GO m
				npoints = xgc->__movedCoords.n;
			}
		} else { points = xc->point.p; npoints = xc->point.n; }
	}

	#ifdef STREAM_POLY_VERBOSE
	printf ("so, points is %p, npoints is %d ntri %d\n",points, npoints,r->ntri);
	#endif
	if(fogCoord){
		if (((struct X3D_Node*)fogCoord)->_nodeType == NODE_FogCoordinate){
			struct X3D_FogCoordinate * xfc = (struct X3D_FogCoordinate *)fogCoord;
			fogpoints = xfc->depth.p;
		}
	}
	if (color) {
		cc = (struct X3D_Color *) color;
		if ((cc->_nodeType != NODE_Color) && (cc->_nodeType != NODE_ColorRGBA)) {
			ConsoleMessage ("stream_polyrep, expected %d got %d\n", NODE_Color, cc->_nodeType);
			r->ntri=0; 
			return;
		} else { 
			colors = cc->color.p; 
			ncolors = cc->color.n; 
			isRGBA = (cc->_nodeType == NODE_ColorRGBA); 
		}
	}
	
	if(normal) {
		nc = (struct X3D_Normal *) normal;
		if (nc->_nodeType != NODE_Normal) {
			ConsoleMessage ("stream_polyrep, normal expected %d, got %d\n",NODE_Normal, nc->_nodeType);
			r->ntri=0; 
			return;
		} else { normals = nc->vector.p; nnormals = nc->vector.n; }
	}

	if (r->tcoordtype) {
		if ((r->tcoordtype != NODE_TextureCoordinate) && 
			(r->tcoordtype != NODE_TextureCoordinate3D) &&
			(r->tcoordtype != NODE_TextureCoordinate4D) &&
			(r->tcoordtype != NODE_MultiTextureCoordinate) &&
			(r->tcoordtype != NODE_TextureCoordinateGenerator )) {
			ConsoleMessage ("stream_polyrep, TexCoord expected %d, got %d\n",NODE_TextureCoordinate, r->tcoordtype);
			r->ntri=0; 
			return;
		}

		if (r->tcoordtype == NODE_TextureCoordinate) {
			//ConsoleMessage ("have textureCoord, point.n = %d",tc->point.n);
			textureCoordPoint[0] = &(texCoordNode->point);
			nmtexcoord = 1;
			ntexdim[0] = 2;
		}
		if (r->tcoordtype == NODE_TextureCoordinate3D) {
			//ConsoleMessage ("have textureCoord, point.n = %d",tc->point.n);
			struct X3D_TextureCoordinate3D *tcn = (struct X3D_TextureCoordinate3D *)texCoordNode;
			//we'll downcast MFVec3f to MFVec2f to get .n, .p later, and upcaste then based on nexdim
			textureCoordPoint[0] = (struct Multi_Vec2f*) &(tcn->point);
			nmtexcoord = 1;
			ntexdim[0] = 3;
		}
		if (r->tcoordtype == NODE_TextureCoordinate4D) {
			//ConsoleMessage ("have textureCoord, point.n = %d",tc->point.n);
			struct X3D_TextureCoordinate4D *tcn = (struct X3D_TextureCoordinate4D *)texCoordNode;
			//we'll downcast MFVec3f to MFVec2f to get .n, .p later, and upcaste then based on nexdim
			textureCoordPoint[0] =(struct Multi_Vec2f*) &(tcn->point); 
			nmtexcoord = 1;
			ntexdim[0] = 4;
		}

		if (r->tcoordtype == NODE_MultiTextureCoordinate) {
			//improper hack to get at least the first multitexture coords right
			struct X3D_MultiTextureCoordinate * mtc = (struct X3D_MultiTextureCoordinate*)texCoordNode;
			if(mtc->texCoord.n && mtc->texCoord.p){
				for(k=0;k<min(mtc->texCoord.n,MAX_MULTITEXTURE);k++){
					textureCoordPoint[k] = NULL;
					if( mtc->texCoord.p[k]->_nodeType == NODE_TextureCoordinate){
						struct X3D_TextureCoordinate * ttcc = (struct X3D_TextureCoordinate*)mtc->texCoord.p[k];
						textureCoordPoint[k] = &(ttcc->point);
						ntexdim[k] = 2;
						nmtexcoord++;
					}
				}
			}
		}

     
		// TextureCoordinateGenerator, make the r->texgentype match the TCGT_ definition of the field 
		if (r->tcoordtype == NODE_TextureCoordinateGenerator) {
			r->texgentype = findFieldInARR(((struct X3D_TextureCoordinateGenerator *)texCoordNode)->mode->strptr, TEXTURECOORDINATEGENERATOR, TEXTURECOORDINATEGENERATOR_COUNT);    
			//ConsoleMessage("have texgen, type %d",r->texgentype);
		}
	}

	#ifdef STREAM_POLY_VERBOSE
	printf ("\nstart stream_polyrep ncoords %d ncolors %d nnormals %d ntri %d\n",
			npoints, ncolors, nnormals, r->ntri);
	#endif


	#ifdef STREAM_POLY_VERBOSE
	printf ("stream polyrep, have an intern type of %d GeneratedTexCoords %p tcindex %p\n",r->tcoordtype, r->GeneratedTexCoords,r->tcindex);
	printf ("polyv, points %p coord %p ntri %d rnormal %p nnormal %d\n",points,r->actualCoord,r->ntri,r->normal, nnormals);
	#endif

	/* Do we have any colours? Are textures, if present, not RGB? */
	hasc = ((ncolors || r->color) && (gglobal()->RenderFuncs.last_texture_type!=TEXTURE_NO_ALPHA));


	// if (r->GeneratedTexCoords) for (i=0; i<10; i++) printf ("start stream, tc %d gt[i] %f\n",i,r->GeneratedTexCoords[i]);

	#ifdef STREAM_POLY_VERBOSE
	printf ("mustGenerateTextures, MALLOCing newtc\n");
	#endif

	// some nodes will generate our tex coords for us, eg GeoElevationGrid
	if (!r->GeneratedTexCoords[0]) {
		newTexCoords[0] = MALLOC (float *, sizeof (float)*ntexdim[0]*r->ntri*3); //always malloc at least one
		for(k=1;k<nmtexcoord;k++)
			newTexCoords[k] = MALLOC (float *, sizeof (float)*ntexdim[k]*r->ntri*3);
	}
    
	newcolors=0;	/*  only if we have colours*/

	/* MALLOC required memory */
	newcindex = MALLOC (GLuint *, sizeof (GLuint)*r->ntri*3);
	newtcindex = MALLOC (GLuint *, sizeof (GLuint)*r->ntri*3);

	newpoints = MALLOC (struct SFVec3f *, sizeof (struct SFVec3f)*r->ntri*3);
	if(fogpoints)
		newfog = MALLOC (float *, sizeof(float)*r->ntri*3);

	if ((nnormals) || (r->normal)) {
		newnorms = MALLOC (struct SFVec3f *, sizeof (struct SFVec3f)*r->ntri*3);
	} else newnorms = 0;


	/* if we have colours, make up a new structure for them to stream to, and also
	   copy pointers to ensure that we index through colorRGBAs properly. */
	if (hasc) {
		newcolors = MALLOC (struct SFColorRGBA *, sizeof (struct SFColorRGBA)*r->ntri*3);
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
				if (ind >= npoints) { 
					/* bounds checking... */
					r->minVals[j]=0.0f;
					r->maxVals[j]=0.0f;
					printf ("spv, warning, index %d >= npoints %d\n",ind,npoints);
				} else {
					if (r->minVals[j] > points[ind].c[j]) r->minVals[j] = points[ind].c[j];
					if (r->maxVals[j] < points[ind].c[j]) r->maxVals[j] = points[ind].c[j];
				}
			} else if(r->actualCoord) {
				if (r->minVals[j] >  r->actualCoord[3*ind+j]) r->minVals[j] =  r->actualCoord[3*ind+j];
				if (r->maxVals[j] <  r->actualCoord[3*ind+j]) r->maxVals[j] =  r->actualCoord[3*ind+j];
			} else {
				r->minVals[j]=0.0f;
				r->maxVals[j]=0.0f;
			}
		}
	}

	if (NO_TEXCOORD_NODE) {
		defaultTextureMap(node, r);
	}


	/* figure out transparency for this node. Go through scene graph, and looksie for it. */
	thisTrans = 0.0f; /* 0.0 = solid, OpenGL 1.0 = solid, we reverse it when writing buffers */
	 
	// printf ("figuring out what the transparency of this node is \n");
	// printf ("nt %s\n",stringNodeType(X3D_NODE(node)->_nodeType));
	
	/* parent[0] should be a NODE_Shape */
	if(0){ 
		//Sept 1, 2016: we are modulating CPV transparency with material transparency 
		// .. in the UberShader, so we don't need to modulate it here
		struct X3D_Shape *parent;

		if (node->_parentVector != NULL) {
			if (vectorSize(node->_parentVector) != 0) {
				parent = vector_get(struct X3D_Shape *, node->_parentVector, 0);
				// printf ("nt, parent is of type %s\n",stringNodeType(parent->_nodeType)); 
				if (parent->_nodeType == NODE_Shape) {
					struct X3D_Appearance *app;
					POSSIBLE_PROTO_EXPANSION(struct X3D_Appearance *, parent->appearance,app)
					if (app != NULL)  {
						// printf ("appearance is of type %s\n",stringNodeType(app->_nodeType)); 
						if (app->_nodeType == NODE_Appearance) {
							struct X3D_Material *mat;
							POSSIBLE_PROTO_EXPANSION(struct X3D_Material *, app->material,mat)
							if (mat != NULL) {
								// printf ("material is of type %s\n",stringNodeType(mat->_nodeType)); 
								if (mat->_nodeType == NODE_Material) {
									thisTrans = mat->transparency;
									// printf ("Set transparency to %f\n",thisTrans);
								}
							}
						}
					}
				}
			}
		}
	}

	/* now, lets go through the old, non-linear polyrep structure, and
	   put it in a stream format */

	#ifdef STREAM_POLY_VERBOSE
	printf ("before streaming for %p, extents %f %f, %f %f, %f %f\n",
		node,
		node->EXTENT_MAX_X,
		node->EXTENT_MIN_X,
		node->EXTENT_MAX_Y,
		node->EXTENT_MIN_Y,
		node->EXTENT_MAX_Z,
		node->EXTENT_MIN_Z);
	#endif



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
					do_glColor4fv(&newcolors[i],oldColorsRGBA[coli].c,isRGBA,thisTrans);
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
			//printf ("... hav points, ind %d npoints %d\n",ind,npoints);
			if (ind>=npoints) {
				/* bounds checking */
				newpoints[i].c[0] = 0.0f;
				newpoints[i].c[1] = 0.0f;
				newpoints[i].c[2] = 0.0f;
				//printf ("spv, warning, index %d >= npoints %d\n",ind,npoints);
			} else {
				memcpy (&newpoints[i], &points[ind].c[0],sizeof (struct SFColor));
				if(newfog) memcpy(&newfog[i],&fogpoints[ind],sizeof(float));
				#ifdef STREAM_POLY_VERBOSE
				printf("Render (points) #%d = [%.5f, %.5f, %.5f] from [%.5f, %.5f, %.5f]\n",i,
					newpoints[i].c[0],newpoints[i].c[1],newpoints[i].c[2],
					points[ind].c[0], points[ind].c[1],points[ind].c[2]);
				#endif
			}
		} else if(r->actualCoord) {
			memcpy (&newpoints[i].c[0], &r->actualCoord[3*ind], sizeof(struct SFColor));
			#ifdef STREAM_POLY_VERBOSE
				printf("Render (r->actualCoord) #%d = [%.5f, %.5f, %.5f]\n",i,
					newpoints[i].c[0],newpoints[i].c[1],newpoints[i].c[2]);
			#endif
		} else {
			#ifdef STREAM_POLY_VERBOSE
			printf ("spv, no points and no coords, setting to 0,0,0\n");
			#endif
			newpoints[i].c[0] = 0.0f; newpoints[i].c[1]=0.0f;newpoints[i].c[2]=0.0f;
		}

		/* TextureCoordinates	*/

		//printf ("textureCoordPoint %p\n",textureCoordPoint);

		if (!r->GeneratedTexCoords[0]) {
			for(k=0;k<(max(1,nmtexcoord));k++){ //always do the first one
				if (textureCoordPoint[k] != NULL) {
					int ndim;
					int j = newtcindex[i];
					//struct SFVec2f me;
					float *me;
            
					// bounds checking
					if (j>=(textureCoordPoint[k]->n)) {
						ConsoleMessage ("stream_polyrep, have tcindex %d, tex coords %d, overflow",j,textureCoordPoint[k]->n);
						j=0;
					}

					// textureCoordPoint is a pointer to struct Multi_Vec2f;
					// struct Multi_Vec2f is struct Multi_Vec2f { int n; struct SFVec2f  *p; };
					// struct SFVec2f is struct SFVec2f { float c[2]; };
 
					// get the 2 tex coords from here, and copy them over to newTexCoords
					ndim = ntexdim[k];
					me = (float*)textureCoordPoint[k]->p; //[j]; //lets hope struct SFVec2f is same layout as float[2]
					me = &me[j*ndim];
					newTexCoords[k][i*ndim] = me[0]; //me.c[0];
					newTexCoords[k][i*ndim+1] = me[1]; //me.c[1];
					if(ndim>2)
						newTexCoords[k][i*ndim+2] = me[2]; //me.c[1];
					if(ndim>3)
						newTexCoords[k][i*ndim+3] = me[3]; //me.c[1];
				} else if(k==0) {
					/* default textures */
					/* we want the S values to range from 0..1, and the
						T values to range from 0...S/T */
					int ndim;
					ppStreamPoly p = (ppStreamPoly)gglobal()->StreamPoly.prv;
					ndim = ntexdim[k];
					newTexCoords[k][i*ndim]   = (newpoints[i].c[p->Sindex] - p->minVals[p->Sindex])/p->Ssize;
					newTexCoords[k][i*ndim+1] = (newpoints[i].c[p->Tindex] - p->minVals[p->Tindex])/p->Ssize;
					if(ndim>2){
						//problem doesn't seem to be a Rindex, lets hope we never get here?
						newTexCoords[k][i*ndim+2] = (newpoints[i].c[p->Tindex] - p->minVals[p->Tindex])/p->Ssize;; 
					}
					if(ndim>3)
						newTexCoords[k][i*ndim+3] = 1.0f;  //homogenous w

				}
			}
		}

		/* calculate maxextents */
		/*
		printf ("sp %u, looking at pts %f %f %f for %d\n",p,newpoints[i].c[0],
			newpoints[i].c[1], newpoints[i].c[2],i); 
		*/

		if (newpoints[i].c[0] > node->EXTENT_MAX_X) node->EXTENT_MAX_X = newpoints[i].c[0];
		if (newpoints[i].c[0] < node->EXTENT_MIN_X) node->EXTENT_MIN_X = newpoints[i].c[0];
		if (newpoints[i].c[1] > node->EXTENT_MAX_Y) node->EXTENT_MAX_Y = newpoints[i].c[1];
		if (newpoints[i].c[1] < node->EXTENT_MIN_Y) node->EXTENT_MIN_Y = newpoints[i].c[1];
		if (newpoints[i].c[2] > node->EXTENT_MAX_Z) node->EXTENT_MAX_Z = newpoints[i].c[2];
		if (newpoints[i].c[2] < node->EXTENT_MIN_Z) node->EXTENT_MIN_Z = newpoints[i].c[2];
	}

	/* free the old, and make the new current. Just in case threading on a multiprocessor
	   machine comes walking through and expects to stream... */
	FREE_IF_NZ(r->actualCoord);
	r->actualCoord = (float *)newpoints;
	FREE_IF_NZ(r->normal);
	r->normal = (float *)newnorms;
	FREE_IF_NZ(r->flat_normal);
	FREE_IF_NZ(r->cindex);
	r->cindex = newcindex;
	FREE_IF_NZ(r->actualFog);
	r->actualFog = (float*)newfog;

    //printf ("now, newTexCoords %p\n",newTexCoords);
    //for (i=0; i<10; i++) printf ("rightpere, tc %d gt[i] %f\n",i,r->GeneratedTexCoords[0][i]);
	/* did we have to generate tex coords? */
	for(k=0;k<max(1,nmtexcoord);k++){
		if (newTexCoords[k] != NULL) {
			FREE_IF_NZ(r->GeneratedTexCoords[k]);
			r->GeneratedTexCoords[k] = newTexCoords[k];
			//printf("k=%d\n",k);
			//for(int kk=0;kk<r->ntri*3;kk++){
			//	printf("%d %f %f\n",kk,r->GeneratedTexCoords[k][kk*2],r->GeneratedTexCoords[k][kk*2 +1]);
			//}
		}
	}
	r->ntcoord = nmtexcoord;
	memcpy(r->ntexdim,ntexdim,4*sizeof(int));
	FREE_IF_NZ(r->color);
	FREE_IF_NZ(r->colindex);

	if(temp_points) {
		FREE_IF_NZ(points);
	}

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

	/* send the data to VBOs if required */
	/* printf("stream polyrep, uploading vertices to VBO %u and %u\n",r->VBO_buffers[VERTEX_VBO], r->VBO_buffers[INDEX_VBO]); */
	if (r->normal) {
		if (r->VBO_buffers[NORMAL_VBO] == 0) glGenBuffers(1,&r->VBO_buffers[NORMAL_VBO]);
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,r->VBO_buffers[NORMAL_VBO]);
		glBufferData(GL_ARRAY_BUFFER,r->ntri*sizeof(struct SFColor)*3,r->normal, GL_STATIC_DRAW);
		//FREE_IF_NZ(r->normal);
	}

	if (r->color) {
		if (r->VBO_buffers[COLOR_VBO] == 0) glGenBuffers(1,&r->VBO_buffers[COLOR_VBO]);            
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,r->VBO_buffers[COLOR_VBO]);
		glBufferData(GL_ARRAY_BUFFER,r->ntri*sizeof(struct SFColorRGBA)*3,r->color, GL_STATIC_DRAW);
		// needed by recalculateColorField ... FREE_IF_NZ(r->color);
	}
	if (newfog) {
		if (r->VBO_buffers[FOG_VBO] == 0) glGenBuffers(1,&r->VBO_buffers[FOG_VBO]);            
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,r->VBO_buffers[FOG_VBO]);
		glBufferData(GL_ARRAY_BUFFER,r->ntri*sizeof(float)*3,r->actualFog, GL_STATIC_DRAW);
	}

	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,r->VBO_buffers[VERTEX_VBO]);
	glBufferData(GL_ARRAY_BUFFER,r->ntri*sizeof(struct SFColor)*3,r->actualCoord, GL_STATIC_DRAW);

	FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER,r->VBO_buffers[INDEX_VBO]);

	// OpenGL ES can use GL_UNSIGNED_SHORT or GL_UNSIGNED_BYTE for glDrawElements; force the indices to be this way.
	if(0){
		//surface triangle indices - dug9 aug 2016: H: we don't need/use these triangle indices
		//because we do glDrawArrays(GL_TRIANGLES,,,) in Polyrep.c which doesn't use indices. 
		//(glDrawElements uses indices, used below for wireframe)
		r->tri_indices = MALLOC(GLushort *, sizeof(GLushort) * r->ntri*3);

		int i;
		GLushort *to = r->tri_indices;
		unsigned int *from = r->cindex;

		for (i=0; i<r->ntri*3; i++) {
			//printf ("and, index %d is %d\n",i,*from);
			*to = (GLushort) *from; to++; from++;
		}
 
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof (GLushort)*r->ntri*3,r->tri_indices,GL_STATIC_DRAW); /* OpenGL-ES */
		//FREE_IF_NZ(myindicies);
	}
	{
		//wireframe lines - prepare in case someone does SHADINGSTYLE_WIRE
		int i, i3, i6;
		GLushort *lindex = MALLOC(GLushort *, sizeof(GLushort) * r->ntri*3*2);
		
		for(i=0;i<r->ntri;i++){
			i3 = i*3;
			i6 = i*6;
			lindex[i6+0] = i3 + 0;
			lindex[i6+1] = i3 + 1;
			lindex[i6+2] = i3 + 1;
			lindex[i6+3] = i3 + 2;
			lindex[i6+4] = i3 + 2;
			lindex[i6+5] = i3 + 0;
		}
		//we just save them, don't set them here. 
		r->wire_indices = lindex;
		//then in Polyrep, when drawing on each frame, if we go into SHADINGSTYLE_WIRE then
		// we swap in wire indices and call glDrawElements without coming back here
		//glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof (GLushort)*r->ntri*3*2,r->wire_indices,GL_STATIC_DRAW); /* OpenGL-ES */
		//FREE_IF_NZ(myindicies);
	}
	{
		//prepare flat normals / face normals for SHADINGSTYLE_FLAT
		int i9;
		r->flat_normal = MALLOC(GLfloat*,r->ntri*sizeof(struct SFColor)*3);
		for(i=0;i<r->ntri;i++){
			float a[3],b[3],c[3],d[3], e[3], f[3], g[3];
			i9 = i*9; //9 floats per triangle
			memcpy(a,&r->actualCoord[i9 +0],sizeof(struct SFColor));
			memcpy(b,&r->actualCoord[i9 +3],sizeof(struct SFColor));
			memcpy(c,&r->actualCoord[i9 +6],sizeof(struct SFColor));
			vecdif3f(d,b,a);
			vecdif3f(e,c,a);
			veccross3f(f,d,e);
			vecnormalize3f(g,f);
			memcpy(&r->flat_normal[i9 +0],g,sizeof(struct SFColor));
			memcpy(&r->flat_normal[i9 +3],g,sizeof(struct SFColor));
			memcpy(&r->flat_normal[i9 +6],g,sizeof(struct SFColor));
		}
	}
		// Can we free this here, or do we need it later? FREE_IF_NZ(r->cindex);

	for(k=0;k<max(1,nmtexcoord);k++){
		if (r->GeneratedTexCoords[k]) {
			if (r->VBO_buffers[TEXTURE_VBO0+k] == 0) glGenBuffers(1,&r->VBO_buffers[TEXTURE_VBO0+k]);
			FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,r->VBO_buffers[TEXTURE_VBO0+k]);
			glBufferData(GL_ARRAY_BUFFER,sizeof (float)*ntexdim[k]*r->ntri*3,r->GeneratedTexCoords[k], GL_STATIC_DRAW);
			/* finished with these - if we did not use it as a flag later, we could get rid of it */
			//FREE_IF_NZ(r->GeneratedTexCoords);
		}
	}


	#ifdef STREAM_POLY_VERBOSE
	printf ("end spv for %p, extents %f %f, %f %f, %f %f\n",
		node,
		node->EXTENT_MAX_X,
		node->EXTENT_MIN_X,
		node->EXTENT_MAX_Y,
		node->EXTENT_MIN_Y,
		node->EXTENT_MAX_Z,
		node->EXTENT_MIN_Z);
	#endif

}
    
static void defaultTextureMap(struct X3D_Node *p, struct X3D_PolyRep * r) { //, struct SFVec3f *points, int npoints) {
	ppStreamPoly psp = (ppStreamPoly)gglobal()->StreamPoly.prv;

	/* variables used only in this routine */
	GLfloat Tsize = 0.0f;
	GLfloat Xsize = 0.0f;
	GLfloat Ysize = 0.0f;
	GLfloat Zsize = 0.0f;

	/* initialize variables used in other routines in this file. */
	psp->Sindex = 0; psp->Tindex = 0;
	psp->Ssize = 0.0f;
	psp->minVals[0]=r->minVals[0]; 
	psp->minVals[1]=r->minVals[1]; 
	psp->minVals[2]=r->minVals[2]; 

	#ifdef STREAM_POLY_VERBOSE
	printf ("have to gen default textures\n");
	#endif

	UNUSED(Tsize); // compiler warnings mitigation

	if ((p->_nodeType == NODE_IndexedFaceSet) ||(p->_nodeType == NODE_ElevationGrid) 
        
        ) {

		/* find the S,T mapping. */
		Xsize = r->maxVals[0]-psp->minVals[0];
		Ysize = r->maxVals[1]-psp->minVals[1];
		Zsize = r->maxVals[2]-psp->minVals[2];

		/* printf ("defaultTextureMap, %f %f %f\n",Xsize,Ysize,Zsize); */

		if ((Xsize >= Ysize) && (Xsize >= Zsize)) {
			/* X size largest */
			psp->Ssize = Xsize; psp->Sindex = 0;
			if (Ysize >= Zsize) { Tsize = Ysize; psp->Tindex = 1;
			} else { Tsize = Zsize; psp->Tindex = 2; }
		} else if ((Ysize >= Xsize) && (Ysize >= Zsize)) {
			/* Y size largest */
			psp->Ssize = Ysize; psp->Sindex = 1;
			if (Xsize >= Zsize) { Tsize = Xsize; psp->Tindex = 0;
			} else { Tsize = Zsize; psp->Tindex = 2; }
		} else {
			/* Z is the largest */
			psp->Ssize = Zsize; psp->Sindex = 2;
			if (Xsize >= Ysize) { Tsize = Xsize; psp->Tindex = 0;
			} else { Tsize = Ysize; psp->Tindex = 1; }
		}
	}
}
