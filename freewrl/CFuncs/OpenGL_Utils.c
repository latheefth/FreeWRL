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

#include "OpenGL_Utils.h"
#ifdef AQUA
#include <OpenGL.h>
extern CGLContextObj aqglobalContext;
#endif


#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}

static int now_mapped = 1;		/* are we on screen, or minimized? */


/* lights status. Light 0 is the headlight */
static int lights[8];



/******************************************************************/
/* textureTransforms of all kinds */




/* did we have a TextureTransform in the Appearance node? */
void start_textureTransform (void *textureNode, int ttnum) {
	struct VRML_TextureTransform  *ttt;
	struct VRML_MultiTextureTransform *mtt;
	
	/* first, is this a textureTransform, or a MultiTextureTransform? */
	ttt = (struct VRML_TextureTransform *) textureNode;

	/* stuff common to all textureTransforms - gets undone at end_textureTransform */
	glMatrixMode(GL_TEXTURE);
       	glEnable(GL_TEXTURE_2D);
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
		mtt = (struct VRML_MultiTextureTransform *) textureNode;
		if (ttnum < mtt->textureTransform.n) {
			ttt = (struct VRML_TextureTransform *) mtt->textureTransform.p[ttnum];
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


int get_now_mapped() {
	return now_mapped;
}


void set_now_mapped(int val) {
	now_mapped = val;
}


void glpOpenGLInitialize() {
	int i;
        float pos[] = { 0.0, 0.0, 0.0, 1.0 };
        float s[] = { 1.0, 1.0, 1.0, 1.0 };
        float As[] = { 0.0, 0.0, 0.0, 1.0 };

	GLclampf red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 1.0f;
        #ifdef AQUA
        CGLSetCurrentContext(aqglobalContext);
        aqglobalContext = CGLGetCurrentContext();
        //printf("OpenGL globalContext %p\n", aqglobalContext);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        #endif


	/* Configure OpenGL for our uses. */

	glClearColor((float)red, (float)green, (float)blue, (float)alpha);
	glShadeModel(GL_SMOOTH);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

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
	glEnable(GL_LIGHTING);

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

	glEnable(GL_CULL_FACE);

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

void fwLoadIdentity () {
	glLoadIdentity();
	if (myMat == GL_PROJECTION) PROJmatOk=FALSE;
	else if (myMat == GL_MODELVIEW) MODmatOk=FALSE;

	else {printf ("fwLoad, unknown %d\n",myMat);}
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

void fwXformPush(struct VRML_Transform *me) {
	glPushMatrix();
	MODmatOk = FALSE;
}

void fwXformPop(struct VRML_Transform *me) {
	glPopMatrix();
	MODmatOk = FALSE;
}


/********************************************************************/
void kill_rendering (void *thisnode);

void kill_MFNode (struct Multi_Node *par) {
	int childCount;
	int i;

	childCount = par->n;
	par->n = 0;
	for (i=0; i<childCount; i++) {
		kill_rendering (par->p[i]);
		FREE_IF_NZ(par->p[i]);
	}
}

void kill_SFString (SV *str) {
	printf ("kill this string \n");
}

void kill_MFString (struct Multi_String *par) {
	printf ("kill this Multistring\n");
}

void kill_MFFloat (struct Multi_Float *par) {
	printf ("kill this Multistring\n");
}

void kill_MFInt32 (struct Multi_Int32 *par) {
	printf ("kill this Multistring\n");
}

void kill_MFColorRGBA (struct Multi_ColorRGBA *par) {
	printf ("kill this color\n");
}

void kill_MFColor (struct Multi_Color *par) {
	printf ("kill this color\n");
}

void kill_MFVec3f (struct Multi_Vec3f *par) {
	printf ("kill this color\n");
}

void kill_FreeWRLPTR (void * par) {
	printf ("kill this pointer\n");
}


/* go through the nodes from the root, and remove all malloc'd memory */

void kill_rendering(void *thisnode) {
	struct VRML_Group *rn;

	/* is there anything here? */
	if (thisnode == 0) return;
	
	rn = (struct VRML_Group *) thisnode;
	/* printf ("kill_rendering, killing node type %d\n",rn->_nodeType); */


	switch (rn->_nodeType) {
		/* Time Component */
			case NODE_TimeSensor:  break;

		/* Networking Component */
			case NODE_Anchor: {
				struct VRML_Anchor *thisNode;
				thisNode = (struct VRML_Anchor *) thisnode;
				kill_MFNode( &thisNode->children);
				kill_SFString(thisNode->description);
				kill_SFString(thisNode->__parenturl);
				kill_MFString(&thisNode->url);
				break; }

			case NODE_Inline: {
				struct VRML_Inline *thisNode;
				thisNode = (struct VRML_Inline *) thisnode;
				kill_MFString (&thisNode->url);
				kill_MFNode (&thisNode->__children);
				kill_SFString (thisNode->__parenturl);
				break; }

			case NODE_LoadSensor: {
				struct VRML_LoadSensor *thisNode;
				thisNode = (struct VRML_LoadSensor *) thisnode;
				kill_MFNode (&thisNode->watchList);
				break; }
				

		/* Grouping Component */

			case NODE_Group: {
				struct VRML_Group * thisNode;
				thisNode = (struct VRML_Group *) thisnode;
				kill_MFNode(& thisNode->children);
				break; }

			case NODE_StaticGroup: {
				struct VRML_StaticGroup *thisNode;
				thisNode = (struct VRML_StaticGroup *) thisnode;
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
				struct VRML_Switch * thisNode;
				thisNode = (struct VRML_Switch *) thisnode;
				kill_MFNode(& thisNode->choice);
				break; }

			case NODE_Transform: {
				struct VRML_Transform * thisNode;
				thisNode = (struct VRML_Transform *) thisnode;
				kill_MFNode(& thisNode->children);
				break; }

			case NODE_WorldInfo: 
				/* no C rendering for this one - it is just ignored at parse */
				break; 


		/* Rendering Component */			
			case NODE_Color: {
				struct VRML_Color *thisNode;
				thisNode = (struct VRML_Color *) thisnode;
				kill_MFColor (&thisNode->color);
				break; }

			case NODE_ColorRGBA: {
				struct VRML_ColorRGBA *thisNode;
				thisNode = (struct VRML_ColorRGBA *) thisnode;
				kill_MFColorRGBA (&thisNode->color);
				break; }

			case NODE_Coordinate: {
				struct VRML_Coordinate *thisNode;
				thisNode = (struct VRML_Coordinate *) thisnode;
				kill_MFVec3f (&thisNode->point);
				break; }

			case NODE_IndexedLineSet: {
				struct VRML_IndexedLineSet *thisNode;
				thisNode = (struct VRML_IndexedLineSet *) thisnode;
				kill_rendering (thisNode->color);
				kill_rendering (thisNode->coord);
				kill_MFInt32 (&thisNode->colorIndex);
				kill_MFInt32 (&thisNode->coordIndex);
				break; }

			/* these are handled the same */
			case NODE_IndexedFaceSet:
			case NODE_TriangleSet:
			case NODE_TriangleStripSet:
			case NODE_TriangleFanSet:
			case NODE_IndexedTriangleSet:
			case NODE_IndexedTriangleStripSet:
			case NODE_IndexedTriangleFanSet: {
				struct VRML_IndexedTriangleFanSet *thisNode;
				thisNode = (struct VRML_IndexedTriangleFanSet *) thisnode;
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
				struct VRML_LineSet *thisNode;
				thisNode = (struct VRML_LineSet *) thisnode;
				kill_rendering (thisNode->color);
				kill_rendering (thisNode->coord);
				kill_MFInt32 (&thisNode->vertexCount);
				kill_FreeWRLPTR (thisNode->__points);
				break; }

			case NODE_Normal: {
				struct VRML_Normal *thisNode;
				thisNode = (struct VRML_Normal *) thisnode;
				kill_MFVec3f (&thisNode->vector);
				break; }

			case NODE_PointSet: {
				struct VRML_PointSet *thisNode;
				thisNode = (struct VRML_PointSet *) thisnode;
				kill_rendering (thisNode->color);
				kill_rendering (thisNode->coord);
				break; }

		/* Shape Component */
			case NODE_Appearance: {
				struct VRML_Appearance *thisNode;
				thisNode = (struct VRML_Appearance *) thisnode;
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
				struct VRML_Shape *thisNode;
				thisNode = (struct VRML_Shape *) thisnode;
				kill_rendering (thisNode->appearance);
				kill_rendering (thisNode->geometry);
				break; }

		/* Geometry 3D Component */


		default: {
			printf ("kill_rendering, unhandled node type %d - ",rn->_nodeType);
			printf ("potential memory leak\n");
		}

	}


}



/* if we have a ReplaceWorld style command, we have to remove the old world. */

void kill_oldWorld(void) {
        char mystring[20];


	/* kill DEFS, handles */
	printf ("have to kill DEFS\n");

	/* stop routing */
	kill_routing();

	/* remove bindables */
	kill_bindables();

	/* stop rendering */
	kill_rendering(rootNode);


	/* free textures */

	
	/* free scripts */
	kill_javascript();

	/* free EAI */
/*
        if (wantEAI) shutdown_EAI();
*/

	/* free java Class invocation */


	#ifndef AQUA
        sprintf (mystring, "QUIT");
        Sound_toserver(mystring);
	#endif


        /* tell statusbar that we have none */
        viewer_default();
        viewpoint_name_status("NONE");
}

