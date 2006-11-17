/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003 John Stewart, Ayla Khan CRC Canada
 Portions Copyright (C) 1998 John Breen
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*
 * $Id$
 *
 */
#include "headers.h"

#include "OpenGL_Utils.h"
#ifdef AQUA
#include <OpenGL.h>
extern CGLContextObj myglobalContext;
extern Boolean isMacPlugin;
extern AGLContext aqglobalContext;
#else 
Display *Xdpy;
Window Xwin;
Window GLwin;
GLXContext GLcx;
XVisualInfo *Xvi;
int fullscreen = 0;
#endif


#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}

/* lights status. Light 0 is the headlight */
static int lights[8];

/* is this 24 bit depth? 16? 8?? Assume 24, unless set on opening */
int displayDepth = 24;


float cc_red = 0.0f, cc_green = 0.0f, cc_blue = 0.0f, cc_alpha = 1.0f;
int cc_changed = FALSE;

/******************************************************************/
/* textureTransforms of all kinds */

/* change the clear colour, selected from the GUI, but do the command in the
   OpenGL thread */

void setglClearColor (float *val) {
	cc_red = *val; val++;
	cc_green = *val; val++;
	cc_blue = *val;
#ifdef AQUA
	val++;
	cc_alpha = *val;
#endif
	cc_changed = TRUE;
}        

void doglClearColor() {
	glClearColor(cc_red, cc_green, cc_blue, cc_alpha);
	cc_changed = FALSE;
}


/* did we have a TextureTransform in the Appearance node? */
void start_textureTransform (void *textureNode, int ttnum) {
	struct X3D_TextureTransform  *ttt;
	struct X3D_MultiTextureTransform *mtt;
	
	/* first, is this a textureTransform, or a MultiTextureTransform? */
	ttt = (struct X3D_TextureTransform *) textureNode;

	/* stuff common to all textureTransforms - gets undone at end_textureTransform */
	glMatrixMode(GL_TEXTURE);
       	/* done in RenderTextures now glEnable(GL_TEXTURE_2D); */
	glLoadIdentity();

	/* is this a simple TextureTransform? */
	if (ttt->_nodeType == NODE_TextureTransform) {
		/*  Render transformations according to spec.*/
        	glTranslatef(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
        	glScalef(((ttt->scale).c[0]),((ttt->scale).c[1]),1);			/*  4*/
        	glRotatef((ttt->rotation) /3.1415926536*180,0,0,1);			/*  3*/
        	glTranslatef(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
        	glTranslatef(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/

	/* is this a MultiTextureTransform? */
	} else  if (ttt->_nodeType == NODE_MultiTextureTransform) {
		mtt = (struct X3D_MultiTextureTransform *) textureNode;
		if (ttnum < mtt->textureTransform.n) {
			ttt = (struct X3D_TextureTransform *) mtt->textureTransform.p[ttnum];
			/* is this a simple TextureTransform? */
			if (ttt->_nodeType == NODE_TextureTransform) {
				/*  Render transformations according to spec.*/
        			glTranslatef(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
        			glScalef(((ttt->scale).c[0]),((ttt->scale).c[1]),1);			/*  4*/
        			glRotatef((ttt->rotation) /3.1415926536*180,0,0,1);			/*  3*/
        			glTranslatef(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
        			glTranslatef(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/
			} else {
				printf ("MultiTextureTransform expected a textureTransform for texture %d, got %d\n",
					ttnum, ttt->_nodeType);
			}
		} else {
			printf ("not enough textures in MultiTextureTransform....\n");
		}

	} else {
		printf ("expected a textureTransform node, got %d\n",ttt->_nodeType);
	}
	glMatrixMode(GL_MODELVIEW);
}

/* did we have a TextureTransform in the Appearance node? */
void end_textureTransform (void *textureNode, int ttnum) {
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

/* keep track of lighting */
void lightState(GLint light, int status) {
	if (light<0) return; /* nextlight will return -1 if too many lights */
	if (lights[light] != status) {
		if (status) glEnable(GL_LIGHT0+light);
		else glDisable(GL_LIGHT0+light);
		lights[light]=status;
	}
}

void glpOpenGLInitialize() {
	int i;
        float pos[] = { 0.0, 0.0, 0.0, 1.0 };
        float s[] = { 1.0, 1.0, 1.0, 1.0 };
        float As[] = { 0.0, 0.0, 0.0, 1.0 };

        #ifdef AQUA
	/* aqglobalContext is found at the initGL routine in MainLoop.c. Here
	   we make it the current Context. */

        /* printf("OpenGL at start of glpOpenGLInitialize globalContext %p\n", aqglobalContext); */
        if (isMacPlugin) {
                aglSetCurrentContext(aqglobalContext);
        } else {
                CGLSetCurrentContext(myglobalContext);
        }

        /* already set aqglobalContext = CGLGetCurrentContext(); */
        /* printf("OpenGL globalContext %p\n", aqglobalContext); */
        #endif

	/* Configure OpenGL for our uses. */

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
	glClearColor(cc_red, cc_green, cc_blue, cc_alpha);
	glShadeModel(GL_SMOOTH);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glLineWidth(gl_linewidth);
	glPointSize (gl_linewidth);

	/*
     * JAS - ALPHA testing for textures - right now we just use 0/1 alpha
     * JAS   channel for textures - true alpha blending can come when we sort
     * JAS   nodes.
	 */

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_COLOR_BUFFER_BIT);

	/* end of ALPHA test */
	glEnable(GL_NORMALIZE);
	LIGHTING_INITIALIZE
	COLOR_MATERIAL_INITIALIZE

	/* keep track of light states; initial turn all lights off except for headlight */
	for (i=0; i<8; i++) {
		lights[i] = 9999;
		lightState(i,FALSE);
	}
	lightState(0, TRUE);

        glLightfv(GL_LIGHT0, GL_POSITION, pos);
        glLightfv(GL_LIGHT0, GL_AMBIENT, As);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, s);
        glLightfv(GL_LIGHT0, GL_SPECULAR, s);

	/* ensure state of GL_CULL_FACE */
	CULL_FACE_INITIALIZE

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_ALIGNMENT,1);

	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (float) (0.2 * 128));
}

void BackEndClearBuffer() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* turn off all non-headlight lights; will turn them on if required. */
void BackEndLightsOff() {
	int i;
	for (i=1; i<8; i++) {
		lightState(i, FALSE);
	}
}

/* OpenGL tuning stuff - cache the modelview matrix */

static int myMat = -111;
static int MODmatOk = FALSE;
static int PROJmatOk = FALSE;
static double MODmat[16];
static double PROJmat[16];
static int sav = 0;
static int tot = 0;

void invalidateCurMat() {
	if (myMat == GL_PROJECTION) PROJmatOk=FALSE;
	else if (myMat == GL_MODELVIEW) MODmatOk=FALSE;
	else {printf ("fwLoad, unknown %d\n",myMat);}
}

void fwLoadIdentity () {
	glLoadIdentity();
	invalidateCurMat();
}
	
void fwMatrixMode (int mode) {
	if (myMat != mode) {
		/*printf ("fwMatrixMode ");
		if (mode == GL_PROJECTION) printf ("GL_PROJECTION\n");
		else if (mode == GL_MODELVIEW) printf ("GL_MODELVIEW\n");
		else {printf ("unknown %d\n",mode);}
		*/

		myMat = mode;
		glMatrixMode(mode);
	}
}

#ifdef DEBUGCACHEMATRIX
void pmat (double *mat) {
	int i;
	for (i=0; i<16; i++) {
		printf ("%3.2f ",mat[i]);
	}
	printf ("\n");
}

void compare (char *where, double *a, double *b) {
	int count;
	double va, vb;

	for (count = 0; count < 16; count++) {
		va = a[count];
		vb = b[count];
		if (fabs(va-vb) > 0.001) {
			printf ("%s difference at %d %lf %lf\n",
					where,count,va,vb);
		}

	}
}
#endif

void fwGetDoublev (int ty, double *mat) {
#ifdef DEBUGCACHEMATRIX
	double TMPmat[16];
	/*printf (" sav %d tot %d\n",sav,tot); */
	tot++;

#endif


	if (ty == GL_MODELVIEW_MATRIX) {
		if (!MODmatOk) {
			glGetDoublev (ty, MODmat);
			MODmatOk = TRUE;

#ifdef DEBUGCACHEMATRIX
		} else sav ++;

		// debug memory calls
		glGetDoublev(ty,TMPmat);
		compare ("MODELVIEW", TMPmat, MODmat);
		memcpy (MODmat,TMPmat, sizeof (MODmat));
		// end of debug
#else
		}
#endif

		memcpy (mat, MODmat, sizeof (MODmat));

	} else if (ty == GL_PROJECTION_MATRIX) {
		if (!PROJmatOk) {
			glGetDoublev (ty, PROJmat);
			PROJmatOk = TRUE;
#ifdef DEBUGCACHEMATRIX
		} else sav ++;

		// debug memory calls
		glGetDoublev(ty,TMPmat);
		compare ("PROJECTION", TMPmat, PROJmat);
		memcpy (PROJmat,TMPmat, sizeof (PROJmat));
		// end of debug
#else
		}
#endif

		memcpy (mat, PROJmat, sizeof (PROJmat));
	} else {
		printf ("fwGetDoublev, inv type %d\n",ty);
	}
}

void fwXformPush(struct X3D_Transform *me) {
	glPushMatrix(); 
	MODmatOk = FALSE;
}

void fwXformPop(struct X3D_Transform *me) {
	glPopMatrix(); 
	MODmatOk = FALSE;
}


/********************************************************************
 *
 * to remove the old scene graph.
 * go recursively through old scene graph, deleting memory when one
 * can. 
 *
 * there are potential problems - DEF/USE gives us duplicate pointers;
 * if we remove the MFNode, then we have a "hanging" pointer to a free'd
 * memory location... so for MFNodes, we do not do the free 
 *
 *********************************************************************/
#undef KILLVERBOSE

void kill_rendering (void *thisnode);

void kill_MFNode (struct Multi_Node *par) {
	int childCount;
	int i;

	if (par->n==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_MFNode\n");
	#endif

	childCount = par->n;
	par->n = 0;
	for (i=0; i<childCount; i++) {
		kill_rendering (par->p[i]);
		/* FREE_IF_NZ(par->p[i]); */
	}
}

void kill_SFString (struct Uni_String *str) {
	/* ok - here we maybe could destroy strings with a SV_REFCNT_dec(str) call, but, it
		would be better to get the Browser to destroy this string itself next time it
		garbage collects (via perl) so lets just ignore this here for now */

	#ifdef KILLVERBOSE
	printf ("kill this string \n");
	#endif
}

void kill_MFString (struct Multi_String *par) {
	int i;
	if (par->n==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_MFString\n");
	#endif

	for (i=0; i<par->n; i++) {
		kill_SFString(par->p[i]);
	}
	par->n=0;
}

void kill_MFFloat (struct Multi_Float *par) {
	if (par->n==0) return;
	par->n=0;

	#ifdef KILLVERBOSE
	printf("killing MFFloat\n");
	#endif

	FREE_IF_NZ(par->p);
}

void kill_MFRotation (struct Multi_Rotation *par) {
	if (par->n==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_MFROtation\n");
	#endif

	par->n=0;
	FREE_IF_NZ(par->p);
}

void kill_MFVec2f (struct Multi_Vec2f *par) {
	if (par->n==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_MFVec2f\n");
	#endif

	par->n=0;
	FREE_IF_NZ(par->p);
}

void kill_MFInt32 (struct Multi_Int32 *par) {
	if (par->n==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_MFint32\n");
	#endif

	par->n=0;
	FREE_IF_NZ(par->p);
}

void kill_MFColorRGBA (struct Multi_ColorRGBA *par) {
	if (par->n==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_mfcolorrghba\n");
	#endif

	par->n=0;
	FREE_IF_NZ(par->p);
}

void kill_MFColor (struct Multi_Color *par) {
	if (par->n==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_mfcolor\n");
	#endif

	par->n=0;
	FREE_IF_NZ(par->p);
}

void kill_MFVec3f (struct Multi_Vec3f *par) {
	if (par->n==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_mfvec3f\n");
	#endif

	par->n=0;
	FREE_IF_NZ(par->p);
}

void kill_FreeWRLPTR (void * par) {
	if (par==0) return;

	#ifdef KILLVERBOSE
	printf ("kill_freewrlptr %d\n",par);
	#endif

	/* cant do this because we might be DEF/USEd 
	FREE_IF_NZ(par);
	*/
/* cant even do this - just ignore for now, because reallocing to zero
   twice fails... 
	rv=realloc (par,1); 
	#ifdef KILLVERBOSE
	printf ("\treallocd to zero size\n");
	#endif
*/
}

void kill_texture (int *tn, int cnt) {
	glDeleteTextures (cnt, (GLuint *)tn);
}

/* go through the nodes from the root, and remove all malloc'd memory */

void kill_rendering(void *thisnode) {
	struct X3D_Group *rn;

	/* is there anything here? */
	if (thisnode == 0) return;
	
	rn = (struct X3D_Group *) thisnode;
	/* printf ("kill_rendering, killing node type %d\n",rn->_nodeType);  */


	switch (rn->_nodeType) {
		/* Time Component */
			case NODE_TimeSensor:  break;

		/* Networking Component */
			case NODE_Anchor: {
				struct X3D_Anchor *thisNode;
				thisNode = (struct X3D_Anchor *) thisnode;
				kill_MFNode( &thisNode->children);
				kill_SFString(thisNode->description);
				kill_SFString(thisNode->__parenturl);
				kill_MFString(&thisNode->url);
				break; }

			case NODE_Inline: {
				struct X3D_Inline *thisNode;
				thisNode = (struct X3D_Inline *) thisnode;
				kill_MFString (&thisNode->url);
				kill_MFNode (&thisNode->__children);
				kill_SFString (thisNode->__parenturl);
				break; }

			case NODE_LoadSensor: {
				struct X3D_LoadSensor *thisNode;
				thisNode = (struct X3D_LoadSensor *) thisnode;
				kill_MFNode (&thisNode->watchList);
				break; }
				

		/* Grouping Component */

			case NODE_Group: {
				struct X3D_Group * thisNode;
				thisNode = (struct X3D_Group *) thisnode;
				kill_MFNode(& thisNode->children);
				break; }

			case NODE_StaticGroup: {
				struct X3D_StaticGroup *thisNode;
				thisNode = (struct X3D_StaticGroup *) thisnode;
				kill_MFNode(& thisNode->children);
				if (thisNode->__transparency != -1) {
					glDeleteLists (thisNode->__transparency,1); 
					thisNode->__transparency == -1;
				}
				if (thisNode->__solid != -1) {
					glDeleteLists (thisNode->__solid,1); 
					thisNode->__solid == -1;
				}

				break; }

			case NODE_Switch: {
				struct X3D_Switch * thisNode;
				thisNode = (struct X3D_Switch *) thisnode;
				kill_MFNode(& thisNode->choice);
				break; }

			case NODE_Transform: {
				struct X3D_Transform * thisNode;
				thisNode = (struct X3D_Transform *) thisnode;
				kill_MFNode(& thisNode->children);
				break; }

			case NODE_WorldInfo: 
				/* no C rendering for this one - it is just ignored at parse */
				break; 


		/* Rendering Component */			
			case NODE_Color: {
				struct X3D_Color *thisNode;
				thisNode = (struct X3D_Color *) thisnode;
				kill_MFColor (&thisNode->color);
				break; }

			case NODE_ColorRGBA: {
				struct X3D_ColorRGBA *thisNode;
				thisNode = (struct X3D_ColorRGBA *) thisnode;
				kill_MFColorRGBA (&thisNode->color);
				break; }

			case NODE_Coordinate: {
				struct X3D_Coordinate *thisNode;
				thisNode = (struct X3D_Coordinate *) thisnode;
				kill_MFVec3f (&thisNode->point);
				break; }

			case NODE_IndexedLineSet: {
				struct X3D_IndexedLineSet *thisNode;
				thisNode = (struct X3D_IndexedLineSet *) thisnode;
				kill_rendering (thisNode->color);
				kill_rendering (thisNode->coord);
				kill_MFInt32 (&thisNode->colorIndex);
				kill_MFInt32 (&thisNode->coordIndex);
				break; }

			/* these are handled the same */
			case NODE_IndexedFaceSet:
			case NODE_ElevationGrid:
			case NODE_TriangleSet:
			case NODE_TriangleStripSet:
			case NODE_TriangleFanSet:
			case NODE_IndexedTriangleSet:
			case NODE_IndexedTriangleStripSet:
			case NODE_IndexedTriangleFanSet: {
				struct X3D_IndexedTriangleFanSet *thisNode;
				thisNode = (struct X3D_IndexedTriangleFanSet *) thisnode;
				kill_rendering (thisNode->color);
				kill_rendering (thisNode->coord);
				kill_rendering (thisNode->normal);
				kill_rendering (thisNode->texCoord);
				kill_MFInt32 (&thisNode->colorIndex);
				kill_MFInt32 (&thisNode->coordIndex);
				kill_MFInt32 (&thisNode->normalIndex);
				kill_MFInt32 (&thisNode->texCoordIndex);
				kill_MFInt32 (&thisNode->fanCount);
				kill_MFInt32 (&thisNode->stripCount);
				kill_MFFloat (&thisNode->height);
				break; }

			case NODE_LineSet: {
				struct X3D_LineSet *thisNode;
				thisNode = (struct X3D_LineSet *) thisnode;
				kill_rendering (thisNode->color);
				kill_rendering (thisNode->coord);
				kill_MFInt32 (&thisNode->vertexCount);
				kill_FreeWRLPTR (thisNode->__vertArr);
				kill_FreeWRLPTR (thisNode->__vertIndx);
				break; }

			case NODE_Normal: {
				struct X3D_Normal *thisNode;
				thisNode = (struct X3D_Normal *) thisnode;
				kill_MFVec3f (&thisNode->vector);
				break; }

			case NODE_PointSet: {
				struct X3D_PointSet *thisNode;
				thisNode = (struct X3D_PointSet *) thisnode;
				kill_rendering (thisNode->color);
				kill_rendering (thisNode->coord);
				break; }

		/* Shape Component */
			case NODE_Appearance: {
				struct X3D_Appearance *thisNode;
				thisNode = (struct X3D_Appearance *) thisnode;
				kill_rendering (thisNode->material);
				kill_rendering (thisNode->texture);
				kill_rendering (thisNode->textureTransform);
				kill_rendering (thisNode->lineProperties);
				kill_rendering (thisNode->fillProperties);
				break; }

			case NODE_FillProperties: break;
			case NODE_LineProperties: break;
			case NODE_Material: break;
			
			case NODE_Shape: {
				struct X3D_Shape *thisNode;
				thisNode = (struct X3D_Shape *) thisnode;
				kill_rendering (thisNode->appearance);
				kill_rendering (thisNode->geometry);
				break; }

		/* Geometry 3D Component */
			case NODE_Box: {
				struct X3D_Box *thisNode;
				thisNode = (struct X3D_Box *) thisnode;
				kill_FreeWRLPTR(thisNode->__points);
				break; }

			case NODE_Cone: {
				struct X3D_Cone *thisNode;
				thisNode = (struct X3D_Cone *) thisnode;
				kill_FreeWRLPTR(thisNode->__sidepoints);
				kill_FreeWRLPTR(thisNode->__botpoints);
				kill_FreeWRLPTR(thisNode->__normals);
				break; }

			case NODE_Cylinder: {
				struct X3D_Cylinder *thisNode;
				thisNode = (struct X3D_Cylinder *) thisnode;
				kill_FreeWRLPTR(thisNode->__points);
				kill_FreeWRLPTR(thisNode->__normals);
				break; }

			case NODE_Extrusion: {
				struct X3D_Extrusion *thisNode;
				thisNode = (struct X3D_Extrusion *) thisnode;
				kill_MFVec2f (&thisNode->crossSection);
				kill_MFRotation(&thisNode->orientation);
				kill_MFVec2f (&thisNode->scale);
				kill_MFVec3f (&thisNode->spine);
				break; }

			case NODE_Sphere: {
				struct X3D_Sphere *thisNode;
				thisNode = (struct X3D_Sphere *) thisnode;
				kill_FreeWRLPTR(thisNode->__points);
				break; }

		/* Geometry 2D Component */
			case NODE_Arc2D: {
				struct X3D_Arc2D *thisNode;
				thisNode = (struct X3D_Arc2D *) thisnode;
				kill_FreeWRLPTR(thisNode->__points);
				break; }

			case NODE_ArcClose2D: {
				struct X3D_ArcClose2D *thisNode;
				thisNode = (struct X3D_ArcClose2D *) thisnode;
				kill_FreeWRLPTR(thisNode->__points);
				break; }

			case NODE_Circle2D: {
				struct X3D_Circle2D *thisNode;
				thisNode = (struct X3D_Circle2D *) thisnode;
				kill_FreeWRLPTR(thisNode->__points);
				break; }

			case NODE_Polyline2D: {
				struct X3D_Polyline2D *thisNode;
				thisNode = (struct X3D_Polyline2D *) thisnode;
				kill_MFVec2f(&thisNode->lineSegments);
				break; }

			case NODE_Polypoint2D: {
				struct X3D_Polypoint2D *thisNode;
				thisNode = (struct X3D_Polypoint2D *) thisnode;
				kill_MFVec2f(&thisNode->point);
				break; }

			case NODE_Rectangle2D: break;

			case NODE_Disk2D: {
				struct X3D_Disk2D *thisNode;
				thisNode = (struct X3D_Disk2D *) thisnode;
				kill_FreeWRLPTR(thisNode->__points);
				kill_FreeWRLPTR(thisNode->__texCoords);
				break; }

			case NODE_TriangleSet2D: {
				struct X3D_TriangleSet2D *thisNode;
				thisNode = (struct X3D_TriangleSet2D *) thisnode;
				kill_MFVec2f(&thisNode->vertices);
				kill_FreeWRLPTR(thisNode->__texCoords);
				break; }


		/* Text Component */

			case NODE_Text: {
				struct X3D_Text *thisNode;
				thisNode = (struct X3D_Text *) thisnode;
				kill_rendering(thisNode->fontStyle);
				kill_MFString (&thisNode->string);
				kill_MFFloat(&thisNode->length);
				break; }

			case NODE_FontStyle: break;

		/* Sound Component */

			case NODE_AudioClip: {
				struct X3D_AudioClip *thisNode;
				thisNode = (struct X3D_AudioClip *) thisnode;
				kill_SFString (thisNode->description);
				kill_MFString (&thisNode->url);
				kill_SFString (thisNode->__parenturl);
				kill_FreeWRLPTR(thisNode->__localFileName);
				break; }

			case NODE_Sound: break;

		/* Lighting Component */

			case NODE_DirectionalLight:
			case NODE_PointLight:
			case NODE_SpotLight: break;

		/* Texturing Component */

			case NODE_ImageTexture: {
				struct X3D_ImageTexture *thisNode;
				thisNode = (struct X3D_ImageTexture *) thisnode;
				kill_MFString (&thisNode->url);
				kill_SFString (thisNode->__parenturl);
				kill_texture (&thisNode->__texture,1);
				break; }

			case NODE_MovieTexture: {
				struct X3D_MovieTexture *thisNode;
				thisNode = (struct X3D_MovieTexture *) thisnode;
				kill_MFString (&thisNode->url);
				kill_SFString (thisNode->__parenturl);
				kill_texture (&thisNode->__texture0_,thisNode->__texture1_ -
								thisNode->__texture0_);
				kill_MFString (&thisNode->__oldurl);
				break; }


			case NODE_MultiTexture: {
				struct X3D_MultiTexture *thisNode;
				thisNode = (struct X3D_MultiTexture *) thisnode;
				kill_MFString (&thisNode->function);
				kill_MFString (&thisNode->mode);
				kill_MFString (&thisNode->source);
				kill_MFNode (&thisNode->texture);
				break; }

			case NODE_MultiTextureCoordinate: {
				struct X3D_MultiTextureCoordinate *thisNode;
				thisNode = (struct X3D_MultiTextureCoordinate *) thisnode;
				kill_MFNode (&thisNode->texCoord);
				break; }

			case NODE_MultiTextureTransform: {
				struct X3D_MultiTextureTransform *thisNode;
				thisNode = (struct X3D_MultiTextureTransform *) thisnode;
				kill_MFNode (&thisNode->textureTransform);
				break; }

			case NODE_PixelTexture: {
				struct X3D_PixelTexture *thisNode;
				thisNode = (struct X3D_PixelTexture *) thisnode;
				kill_SFString (thisNode->image);
				kill_SFString (thisNode->__parenturl);
				kill_texture (&thisNode->__texture,1);
				break; }

			case NODE_TextureCoordinate: {
				struct X3D_TextureCoordinate *thisNode;
				thisNode = (struct X3D_TextureCoordinate *) thisnode;
				kill_MFVec2f (&thisNode->point);
				kill_MFVec2f (&thisNode->__compiledpoint);
				break; }

			case NODE_TextureCoordinateGenerator: {
				struct X3D_TextureCoordinateGenerator *thisNode;
				thisNode = (struct X3D_TextureCoordinateGenerator *) thisnode;
				kill_MFFloat (&thisNode->parameter);
				kill_SFString (thisNode->mode);
				break; }

			case NODE_TextureTransform: break;


		/* Interpolation Component */

			case NODE_ColorInterpolator: {
				struct X3D_ColorInterpolator *thisNode;
				thisNode = (struct X3D_ColorInterpolator *) thisnode;
				kill_MFFloat(&thisNode->key);
				kill_MFVec3f ((struct Multi_Vec3f *) &thisNode->keyValue);
				break; }

			case NODE_CoordinateInterpolator: /* same as NormalInterpolator */
			case NODE_NormalInterpolator: {
				struct X3D_CoordinateInterpolator *thisNode;
				thisNode = (struct X3D_CoordinateInterpolator *) thisnode;
				kill_MFFloat (&thisNode->key);
				kill_MFVec3f (&thisNode->keyValue);
				break; }

			case NODE_PositionInterpolator: {
				struct X3D_PositionInterpolator *thisNode;
				thisNode = (struct X3D_PositionInterpolator *) thisnode;
				kill_MFFloat(&thisNode->key);
				kill_MFVec3f (&thisNode->keyValue);
				break; }

			case NODE_OrientationInterpolator: {
				struct X3D_OrientationInterpolator *thisNode;
				thisNode = (struct X3D_OrientationInterpolator *) thisnode;
				kill_MFFloat(&thisNode->key);
				kill_MFRotation (&thisNode->keyValue);
				break; }


			case NODE_ScalarInterpolator: {
				struct X3D_ScalarInterpolator *thisNode;
				thisNode = (struct X3D_ScalarInterpolator *) thisnode;
				kill_MFFloat(&thisNode->key);
				kill_MFFloat (&thisNode->keyValue);
				break; }

		/* Pointing Device Component */
	
			/* potential memory leak, if the Description field is not blank */
			case NODE_TouchSensor: break;
			case NODE_PlaneSensor: break;
			case NODE_SphereSensor: break;
			case NODE_CylinderSensor: break;

		/* Key Device Component */

		/* Environmental Sensor Component */

			case NODE_ProximitySensor: break;
			case NODE_VisibilitySensor: break;
		
		/* Navigation Component */

			case NODE_LOD: {
				struct X3D_LOD *thisNode;
				thisNode = (struct X3D_LOD *) thisnode;
				kill_MFNode (&thisNode->level);
				kill_MFFloat(&thisNode->range);
				break; }

			case NODE_Billboard: {
				struct X3D_Billboard *thisNode;
				thisNode = (struct X3D_Billboard *) thisnode;
				kill_MFNode (&thisNode->children);
				break; }

			case NODE_Collision: {
				struct X3D_Collision *thisNode;
				thisNode = (struct X3D_Collision *) thisnode;
				kill_MFNode (&thisNode->children);
				kill_rendering (thisNode->proxy);
				break; }

			case NODE_Viewpoint: break;  /* possible description memory leak */

			case NODE_NavigationInfo: break; /* possible avatarSize, type, transitionType memory leak */

		/* Environmental Effects Component */

			case NODE_Fog: break;

			case NODE_TextureBackground: {
				struct X3D_TextureBackground *thisNode;
				thisNode = (struct X3D_TextureBackground *) thisnode;
				kill_MFFloat(&thisNode->groundAngle);
				kill_MFVec3f((struct Multi_Vec3f *)&thisNode->groundColor);
				kill_MFFloat(&thisNode->skyAngle);
				kill_MFVec3f((struct Multi_Vec3f *)&thisNode->skyColor);
				kill_SFString(thisNode->__parenturl);
				kill_FreeWRLPTR(thisNode->__points);
				kill_FreeWRLPTR(thisNode->__colours);
				kill_rendering (thisNode->frontTexture);
				kill_rendering (thisNode->backTexture);
				kill_rendering (thisNode->topTexture);
				kill_rendering (thisNode->bottomTexture);
				kill_rendering (thisNode->leftTexture);
				kill_rendering (thisNode->rightTexture);
				kill_MFFloat (&thisNode->transparency);
				break; }

			case NODE_Background: {
				struct X3D_Background *thisNode;
				thisNode = (struct X3D_Background *) thisnode;
				kill_MFFloat(&thisNode->groundAngle);
				kill_MFVec3f((struct Multi_Vec3f *)&thisNode->groundColor);
				kill_MFFloat(&thisNode->skyAngle);
				kill_MFVec3f((struct Multi_Vec3f *)&thisNode->skyColor);
				kill_SFString(thisNode->__parenturl);
				kill_FreeWRLPTR(thisNode->__points);
				kill_FreeWRLPTR(thisNode->__colours);
				kill_texture (&thisNode->__texturefront,1);
				kill_texture (&thisNode->__textureback,1);
				kill_texture (&thisNode->__texturetop,1);
				kill_texture (&thisNode->__texturebottom,1);
				kill_texture (&thisNode->__textureleft,1);
				kill_texture (&thisNode->__textureright,1);
				kill_MFString(&thisNode->frontUrl);
				kill_MFString(&thisNode->backUrl);
				kill_MFString(&thisNode->topUrl);
				kill_MFString(&thisNode->bottomUrl);
				kill_MFString(&thisNode->leftUrl);
				kill_MFString(&thisNode->rightUrl);
				break; }


		/* Geospatial Component */


		/* H-Anim Component */

		/* NURBS Component */

		/* Scripting Component */
			case NODE_Script: break; /* no data stored here -all in perl */

		/* old VRML nodes */
			case NODE_InlineLoadControl: break; /* memory leak, but out of date node */

			

		default: {
			printf ("kill_rendering, unhandled node type %d - ",rn->_nodeType);
			printf ("potential memory leak\n");
		}

	}


}



/* if we have a ReplaceWorld style command, we have to remove the old world. */

void kill_oldWorld(int kill_EAI, int kill_JavaScript, int kill_JavaClass) {
        char mystring[20];

	/* consoleMessage - ok, not exactly a kill, more of a reset */
	consMsgCount = 0;

	/* occlusion testing - zero total count, but keep malloc'd memory around */
	#ifdef OCCLUSION
	maxShapeFound = 0;
	#endif

	/* clock events - stop them from ticking */
	kill_clockEvents();

	/* kill DEFS, handles */
	EAI_killBindables();

	/* stop routing */
	kill_routing();

	/* stop rendering */
	kill_rendering(rootNode);


	/* free textures */
	
	/* free scripts */
	kill_javascript();

	/* free EAI */
	if (kill_EAI) {
        	shutdown_EAI();
	}

	/* free java Class invocation */


	#ifndef AQUA
        sprintf (mystring, "QUIT");
        Sound_toserver(mystring);
	#endif


        /* tell statusbar that we have none */
        viewer_default();
        setMenuStatus("NONE");
}

