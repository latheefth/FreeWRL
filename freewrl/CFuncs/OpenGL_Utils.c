/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003 John Stewart, Ayla Khan CRC Canada,
 2007 Daniel Tremblay
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
#include "Bindable.h"
#include "CParse.h"

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

void kill_rendering(void);

/* Node Tracking */
void kill_X3DNodes(void);
void createdMemoryTable();
void increaseMemoryTable();
static uintptr_t * memoryTable = NULL;
static int nodeNumber = 0;
static int tableIndexSize = ID_UNDEFINED;
static int nextEntry = 0;
static int i=0;
static struct X3D_Node *forgottenNode;

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
        /* JAS float pos[] = { 0.0, 0.0, 0.0, 1.0 }; */
	/* put the headlight far behind us, so that lighting on close
	   surfaces (eg, just above the surface of a box) is evenly lit */
        float pos[] = { 0.0, 0.0, 100.0, 1.0 };
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

glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
glEnable (GL_RESCALE_NORMAL);

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
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,As);
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

/* for Sarah's front end - should be removed sometime... */
void kill_rendering() {kill_X3DNodes();}


/* if we have a ReplaceWorld style command, we have to remove the old world. */
/* NOTE: There are 2 kinds of of replaceworld commands - sometimes we have a URL
   (eg, from an Anchor) and sometimes we have a list of nodes (from a Javascript
   replaceWorld, for instance). The URL ones can really replaceWorld, the node
   ones, really, it's just replace the rootNode children, as WE DO NOT KNOW
   what the user has programmed, and what nodes are (re) used in the Scene Graph */

void kill_oldWorld(int kill_EAI, int kill_JavaScript, int loadedFromURL) {
        char mystring[20];

	/* consoleMessage - ok, not exactly a kill, more of a reset */
	consMsgCount = 0;

	if (loadedFromURL) {
		/* occlusion testing - zero total count, but keep MALLOC'd memory around */
		zeroOcclusion();

		/* clock events - stop them from ticking */
		kill_clockEvents();

		/* kill DEFS, handles */
		EAI_killBindables();
		kill_bindables();

		/* stop routing */
		kill_routing();

		/* stop rendering */
		((struct X3D_Group*)rootNode)->children.n = 0;


		/* free textures */
		kill_openGLTextures();
	
		/* free scripts */
		kill_javascript();

		/* free EAI */
		if (kill_EAI) {
	        	shutdown_EAI();
		}

		/* free memory */
		kill_X3DNodes();


		#ifndef AQUA
	        sprintf (mystring, "QUIT");
	        Sound_toserver(mystring);
		#endif

		/* reset any VRML Parser data */
		if (globalParser != NULL) {
			parser_destroyData(globalParser);
			globalParser = NULL;
		}


	        /* tell statusbar that we have none */
	        viewer_default();
	        setMenuStatus("NONE");
	} else {
		/* just a replaceWorld from EAI or from Javascript */

		/* stop rendering */
		((struct X3D_Group*)rootNode)->children.n = 0;

	        /* tell statusbar that we have none */
	        viewer_default();
	        setMenuStatus("NONE");
	}	
}

/* for verifying that a memory pointer exists */
int checkNode(struct X3D_Node *node, char *fn, int line) {
	int tc;

	if (node == NULL) {
		printf ("checkNode, node is NULL at %s %d\n",node,fn,line);
		return FALSE;
	}

	if (node == forgottenNode) return TRUE;

	for (tc = 0; tc< nextEntry; tc++)
		if (memoryTable[tc] == (uintptr_t)node) return TRUE;

	printf ("checkNode: did not find %d in memory table at i%s %d\n",node,fn,line);

	return FALSE;
}


/*keep track of node created*/
void registerX3DNode(struct X3D_Node * tmp){	
	/*printf("nextEntry=%d	",nextEntry); printf("tableIndexSize=%d \n",tableIndexSize);*/
	/*is table to small give us some leeway in threads */
	if (nextEntry >= (tableIndexSize-10)){
		/*is table exist*/	
		if (tableIndexSize <= ID_UNDEFINED){
			createdMemoryTable();		
		} else {
			increaseMemoryTable();
		}
	}
	/*adding node in table*/	
	memoryTable[nextEntry] = (uintptr_t) tmp;
	nextEntry+=1;
}

/*We don't register the first node created for reload reason*/
void doNotRegisterThisNodeForDestroy(struct X3D_Node * nodePtr){
	if((uintptr_t)nodePtr==(memoryTable[nextEntry-1])){
		nextEntry-=1;
		forgottenNode = nodePtr;
	}
}

/*creating node table*/
void createdMemoryTable(){
	int count;

	tableIndexSize=5000;
	memoryTable = MALLOC(tableIndexSize * sizeof(uintptr_t));

	/* initialize this to a known state */
	for (count=0; count < tableIndexSize; count++) {
		memoryTable[count] = NULL;
	}
}

/*making table bigger*/
void increaseMemoryTable(){
	int count;
	int oldhigh;

	oldhigh = tableIndexSize;

	
	tableIndexSize*=2;
	memoryTable = REALLOC (memoryTable, tableIndexSize * sizeof(memoryTable) );

	/* initialize this to a known state */
	for (count=oldhigh; count < tableIndexSize; count++) {
		memoryTable[count] = NULL;
	}
	/*printf("increasing memory table=%d\n",sizeof(memoryTable));*/
}

/* zero the Visibility flag in all nodes */
void zeroVisibilityFlag(void) {
	struct X3D_Node* node;
	int i;
	int ocnum;

	ocnum=-1;

	/* do we have GL_ARB_occlusion_query, or are we still parsing Textures? */
	if ((OccFailed) || isTextureParsing()) {

		/* if we have textures still loading, display all the nodes, so that the textures actually
		   get put into OpenGL-land. If we are not texture parsing... */
		/* no, we do not have GL_ARB_occlusion_query, just tell every node that it has visible children 
		   and hope that, sometime, the user gets a good computer graphics card */
		for (i=0; i<nextEntry; i++){		
			node = X3D_NODE(memoryTable[i]);	
			node->_renderFlags = node->_renderFlags | VF_hasVisibleChildren;
		}	
	} else {
		/* we do... lets zero the hasVisibleChildren flag */
		for (i=0; i<nextEntry; i++){		
			node = X3D_NODE(memoryTable[i]);		
			/* printf ("zeroVisibility - %d is a %s, flags %x\n",i,stringNodeType(node->_nodeType), (node->_renderFlags) & VF_hasVisibleChildren); */
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_hasVisibleChildren);
	
			/* do we have a tie in here for node-name? */
			if (node->_nodeType == NODE_Shape) ocnum = ((struct X3D_Shape *)node)->__OccludeNumber;
			if (node->_nodeType == NODE_VisibilitySensor) ocnum = ((struct X3D_VisibilitySensor*)node)->__OccludeNumber;
			if ((ocnum>=0) & (ocnum < OccQuerySize)) {
				if (OccNodes[ocnum]==0) {
					/* printf ("zeroVis, recording occlude %d as %d was %d\n",ocnum,node,OccNodes[ocnum]); */
					OccNodes[ocnum]=(void *)node;
				}
				ocnum=-1;
			
			}
		}			
	}
}

/* go through the linear list of nodes, and do "special things" for special nodes, like
   Sensitive nodes, Viewpoint nodes, ... */

#define BEGIN_NODE(thistype) case NODE_##thistype:
#define END_NODE break;

#define SIBLING_SENSITIVE(thistype) \
			/* make Sensitive */ \
			if (((struct X3D_##thistype *)node)->enabled) { \
				nParents = ((struct X3D_##thistype *)node)->_nparents; \
				pp = (((struct X3D_##thistype *)node)->_parents); \
			}  

#define ANCHOR_SENSITIVE(thistype) \
			/* make THIS Sensitive - most nodes make the parents sensitive, Anchors have children...*/ \
			anchorPtr = (struct X3D_Anchor *)node;
#ifdef xxx
			nParents = ((struct X3D_Anchor *)node)->children.n; pp = ((struct X3D_Anchor *)node)->children.p; 
#endif

#define VIEWPOINT(thistype) \
			setBindPtr = (unsigned int *)(node+ offsetof (struct X3D_##thistype, set_bind));

#define CHILDREN_NODE(thistype) \
			addChildren = NULL; removeChildren = NULL; \
			if (((struct X3D_##thistype *)node)->addChildren.n > 0) { \
				addChildren = &((struct X3D_##thistype *)node)->addChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->children; \
			} \
			if (((struct X3D_##thistype *)node)->removeChildren.n > 0) { \
				removeChildren = &((struct X3D_##thistype *)node)->removeChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->children; \
			} 

#define CHILDREN_SWITCH_NODE(thistype) \
			addChildren = NULL; removeChildren = NULL; \
			if (((struct X3D_##thistype *)node)->addChildren.n > 0) { \
				addChildren = &((struct X3D_##thistype *)node)->addChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->choice; \
			} \
			if (((struct X3D_##thistype *)node)->removeChildren.n > 0) { \
				removeChildren = &((struct X3D_##thistype *)node)->removeChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->choice; \
			} 

#define CHILDREN_LOD_NODE(thistype) \
			addChildren = NULL; removeChildren = NULL; \
			if (((struct X3D_##thistype *)node)->addChildren.n > 0) { \
				addChildren = &((struct X3D_##thistype *)node)->addChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->level; \
			} \
			if (((struct X3D_##thistype *)node)->removeChildren.n > 0) { \
				removeChildren = &((struct X3D_##thistype *)node)->removeChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->level; \
			}

#define EVIN_AND_FIELD_SAME(thisfield, thistype) \
			if ((((struct X3D_##thistype *)node)->set_##thisfield.n) > 0) { \
				((struct X3D_##thistype *)node)->thisfield.n = 0; \
				FREE_IF_NZ (((struct X3D_##thistype *)node)->thisfield.p); \
				((struct X3D_##thistype *)node)->thisfield.p = ((struct X3D_##thistype *)node)->set_##thisfield.p; \
				((struct X3D_##thistype *)node)->thisfield.n = ((struct X3D_##thistype *)node)->set_##thisfield.n; \
				((struct X3D_##thistype *)node)->set_##thisfield.n = 0; \
				((struct X3D_##thistype *)node)->set_##thisfield.p = NULL; \
			} 

void startOfLoopNodeUpdates(void) {
	struct X3D_Node* node;
	struct X3D_Node* parents;
	struct X3D_Anchor* anchorPtr;
	void **pp;
	int nParents;
	int i,j;
	unsigned int *setBindPtr;

	struct Multi_Node *addChildren;
	struct Multi_Node *removeChildren;
	struct Multi_Node *childrenPtr;


	/* assume that we do not have any sensitive nodes at all... */
	HaveSensitive = FALSE;

	/* go through the node table, and zero any bits of interest */
	for (i=0; i<nextEntry; i++){		
		node = X3D_NODE(memoryTable[i]);	
		if (node != NULL) {
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_Sensitive);
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_hasSensitiveChildren);
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_Viewpoint);
		}
	}

	/* go through the list of nodes, and "work" on any that need work */
	nParents = 0;
	setBindPtr = NULL;
	childrenPtr = NULL;
	anchorPtr = NULL;

	for (i=0; i<nextEntry; i++){		
		node = X3D_NODE(memoryTable[i]);		
		if (node != NULL) {
			switch (node->_nodeType) {
				/* some nodes, like Extrusions, have "set_" fields same as normal internal fields,
				   eg, "set_spine" and "spine". Here we just copy the fields over, and remove the
				   "set_" fields. */
				BEGIN_NODE(IndexedLineSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedLineSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedLineSet)
				END_NODE
				BEGIN_NODE(IndexedTriangleFanSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedTriangleFanSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedTriangleFanSet)
					EVIN_AND_FIELD_SAME(normalIndex,IndexedTriangleFanSet)
					EVIN_AND_FIELD_SAME(texCoordIndex,IndexedTriangleFanSet)
					EVIN_AND_FIELD_SAME(height,IndexedTriangleFanSet)
				END_NODE
				BEGIN_NODE(IndexedTriangleSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedTriangleSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedTriangleSet)
					EVIN_AND_FIELD_SAME(normalIndex,IndexedTriangleSet)
					EVIN_AND_FIELD_SAME(texCoordIndex,IndexedTriangleSet)
					EVIN_AND_FIELD_SAME(height,IndexedTriangleSet)
				END_NODE
				BEGIN_NODE(IndexedTriangleStripSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedTriangleStripSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedTriangleStripSet)
					EVIN_AND_FIELD_SAME(normalIndex,IndexedTriangleStripSet)
					EVIN_AND_FIELD_SAME(texCoordIndex,IndexedTriangleStripSet)
					EVIN_AND_FIELD_SAME(height,IndexedTriangleStripSet)
				END_NODE
				BEGIN_NODE(TriangleFanSet)
					EVIN_AND_FIELD_SAME(colorIndex,TriangleFanSet)
					EVIN_AND_FIELD_SAME(coordIndex,TriangleFanSet)
					EVIN_AND_FIELD_SAME(normalIndex,TriangleFanSet)
					EVIN_AND_FIELD_SAME(texCoordIndex,TriangleFanSet)
					EVIN_AND_FIELD_SAME(height,TriangleFanSet)
				END_NODE
				BEGIN_NODE(TriangleStripSet)
					EVIN_AND_FIELD_SAME(colorIndex,TriangleStripSet)
					EVIN_AND_FIELD_SAME(coordIndex,TriangleStripSet)
					EVIN_AND_FIELD_SAME(normalIndex,TriangleStripSet)
					EVIN_AND_FIELD_SAME(texCoordIndex,TriangleStripSet)
					EVIN_AND_FIELD_SAME(height,TriangleStripSet)
				END_NODE
				BEGIN_NODE(TriangleSet)
					EVIN_AND_FIELD_SAME(colorIndex,TriangleSet) 
					EVIN_AND_FIELD_SAME(coordIndex,TriangleSet) 
					EVIN_AND_FIELD_SAME(normalIndex,TriangleSet) 
					EVIN_AND_FIELD_SAME(texCoordIndex,TriangleSet) 
					EVIN_AND_FIELD_SAME(height,TriangleSet) 
				END_NODE
				BEGIN_NODE(ElevationGrid)
					EVIN_AND_FIELD_SAME(colorIndex,ElevationGrid)
					EVIN_AND_FIELD_SAME(coordIndex,ElevationGrid)
					EVIN_AND_FIELD_SAME(normalIndex,ElevationGrid)
					EVIN_AND_FIELD_SAME(texCoordIndex,ElevationGrid)
					EVIN_AND_FIELD_SAME(height,ElevationGrid)
				END_NODE
				BEGIN_NODE(Extrusion)
					EVIN_AND_FIELD_SAME(crossSection,Extrusion)
					EVIN_AND_FIELD_SAME(orientation,Extrusion)
					EVIN_AND_FIELD_SAME(scale,Extrusion)
					EVIN_AND_FIELD_SAME(spine,Extrusion)
				END_NODE
				BEGIN_NODE(IndexedFaceSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(normalIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(texCoordIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(height,IndexedFaceSet)
				END_NODE
				BEGIN_NODE(GeoElevationGrid)
					EVIN_AND_FIELD_SAME(height,GeoElevationGrid)
				END_NODE
				/* these are actually compiled in by the GeoViewpoint code 
				BEGIN_NODE(GeoViewpoint)
					EVIN_AND_FIELD_SAME(orientation,GeoViewpoint) 
					EVIN_AND_FIELD_SAME(position,GeoViewpoint)
				END_NODE
				*/
	
				/* get ready to mark these nodes as Mouse Sensitive */
				BEGIN_NODE(PlaneSensor) SIBLING_SENSITIVE(PlaneSensor) END_NODE
				BEGIN_NODE(SphereSensor) SIBLING_SENSITIVE(SphereSensor) END_NODE
				BEGIN_NODE(CylinderSensor) SIBLING_SENSITIVE(CylinderSensor) END_NODE
				BEGIN_NODE(TouchSensor) SIBLING_SENSITIVE(TouchSensor) END_NODE
				BEGIN_NODE(GeoTouchSensor) SIBLING_SENSITIVE(GeoTouchSensor) END_NODE
	
				/* Anchor is Mouse Sensitive, AND has Children nodes */
				BEGIN_NODE(Anchor)
				ANCHOR_SENSITIVE(Anchor)
				CHILDREN_NODE(Anchor)
				END_NODE
				
				/* maybe this is the current Viewpoint? */
				BEGIN_NODE(Viewpoint) VIEWPOINT(Viewpoint) END_NODE
				BEGIN_NODE(GeoViewpoint) VIEWPOINT(GeoViewpoint) END_NODE
	
				/* does this one possibly have add/removeChildren? */
				BEGIN_NODE(Group) CHILDREN_NODE(Group) END_NODE
				BEGIN_NODE(Transform) CHILDREN_NODE(Transform) END_NODE
				BEGIN_NODE(NurbsGroup) CHILDREN_NODE(NurbsGroup) END_NODE
				BEGIN_NODE(Contour2D) CHILDREN_NODE(Contour2D) END_NODE
				BEGIN_NODE(HAnimSite) CHILDREN_NODE(HAnimSite) END_NODE
				BEGIN_NODE(HAnimSegment) CHILDREN_NODE(HAnimSegment) END_NODE
				BEGIN_NODE(HAnimJoint) CHILDREN_NODE(HAnimJoint) END_NODE
				BEGIN_NODE(Billboard) CHILDREN_NODE(Billboard) END_NODE
				BEGIN_NODE(Collision) CHILDREN_NODE(Collision) END_NODE
				BEGIN_NODE(Switch) CHILDREN_SWITCH_NODE(Switch) END_NODE
				BEGIN_NODE(LOD) CHILDREN_LOD_NODE(LOD) END_NODE
	
			}
		}

		/* now, act on this node  for Sensitive nodes. here we tell the PARENTS that they are sensitive */
		if (nParents != 0) {
			for (j=0; j<nParents; j++) {
				struct X3D_Node *n = X3D_NODE(pp[j]);
				n->_renderFlags = n->_renderFlags  | VF_Sensitive;

 				/* and tell the rendering pass that there is a sensitive node down this branch */
				update_renderFlag(n,VF_hasSensitiveChildren);
			}

			/* tell mainloop that we have to do a sensitive pass now */
			HaveSensitive = TRUE;
			nParents = 0;
		}

		/* Anchor nodes are slightly different than sibling-sensitive nodes */
		if (anchorPtr != NULL) {
			anchorPtr->_renderFlags = anchorPtr->_renderFlags  | VF_Sensitive;

 			/* and tell the rendering pass that there is a sensitive node down this branch */
			update_renderFlag(X3D_NODE(anchorPtr),VF_hasSensitiveChildren);

			/* tell mainloop that we have to do a sensitive pass now */
			HaveSensitive = TRUE;
			anchorPtr = NULL;
		}

		/* do BINDING of Viewpoint Nodes */
		if (setBindPtr != NULL) {
			/* check the set_bind eventin to see if it is TRUE or FALSE */
			if (*setBindPtr < 100) {
				/* printf ("Found a vp to modify %d\n",node); */
				/* up_vector is reset after a bind */
				if (*setBindPtr==1) reset_upvector();
				bind_node ((void *)node, &viewpoint_tos,&viewpoint_stack[0]);
			}
			setBindPtr = NULL;
		}

		/* this node possibly has to do add/remove children? */
		if (childrenPtr != NULL) {
			if (addChildren != NULL) {
				AddRemoveChildren(node,childrenPtr,(uintptr_t *) addChildren->p,addChildren->n,1);
				addChildren->n=0;
			}
			if (removeChildren != NULL) {
				AddRemoveChildren(node,childrenPtr,(uintptr_t *) removeChildren->p,removeChildren->n,2);
				removeChildren->n=0;
			}
			childrenPtr = NULL;
		}
	}

	/* now, we can go and tell the grouping nodes which ones are the lucky ones that contain the current Viewpoint node */
	if (viewpoint_stack[viewpoint_tos] != 0) {
		update_renderFlag(X3D_NODE(viewpoint_stack[viewpoint_tos]), VF_Viewpoint);
	}

}


/*delete node created*/
void kill_X3DNodes(void){
	int i=0;
	int j=0;
	uintptr_t *fieldOffsetsPtr;
	char * fieldPtr;
	struct X3D_Node* structptr;
	struct Multi_Float* MFloat;
	struct Multi_Rotation* MRotation;
	struct Multi_Vec3f* MVec3f;
	struct Multi_Bool* Mbool;
	struct Multi_Int32* MInt32;
	struct Multi_Node* MNode;
	struct Multi_Color* MColor;
	struct Multi_ColorRGBA* MColorRGBA;
	struct Multi_Time* MTime;
	struct Multi_String* MString;
	struct Multi_Vec2f* MVec2f;
	uintptr_t * VPtr;
	struct Uni_String *MyS;

	/*go thru all node until table is empty*/
	for (i=0; i<nextEntry; i++){		
		structptr = X3D_NODE(memoryTable[i]);		
		/* printf("\nNode pointer	= %d entry %d of %d\n",structptr,i,nextEntry);
		printf("\nNode Type	= %s\n",stringNodeType(structptr->_nodeType));  */

		/* kill any parents that may exist. */
		FREE_IF_NZ (structptr->_parents);

		fieldOffsetsPtr = (uintptr_t *) NODE_OFFSETS[structptr->_nodeType];
		/*go thru all field*/				
		while (*fieldOffsetsPtr != -1) {
			fieldPtr=(char*)structptr+(*(fieldOffsetsPtr+1));
			/* printf ("looking at field %s type %s\n",FIELDNAMES[*fieldOffsetsPtr],FIELDTYPES[*(fieldOffsetsPtr+2)]); */

			switch(*(fieldOffsetsPtr+2)){
				case FIELDTYPE_MFFloat:
					MFloat=(struct Multi_Float *)fieldPtr;
					MFloat->n=0;
					FREE_IF_NZ(MFloat->p);
					break;
				case FIELDTYPE_MFRotation:
					MRotation=(struct Multi_Rotation *)fieldPtr;
					MRotation->n=0;
					FREE_IF_NZ(MRotation->p);
					break;
				case FIELDTYPE_MFVec3f:
					MVec3f=(struct Multi_Vec3f *)fieldPtr;
					MVec3f->n=0;
					FREE_IF_NZ(MVec3f->p);
					break;
				case FIELDTYPE_MFBool:
					Mbool=(struct Multi_Bool *)fieldPtr;
					Mbool->n=0;
					FREE_IF_NZ(Mbool->p);
					break;
				case FIELDTYPE_MFInt32:
					MInt32=(struct Multi_Int32 *)fieldPtr;
					MInt32->n=0;
					FREE_IF_NZ(MInt32->p);
					break;
				case FIELDTYPE_MFNode:
					MNode=(struct Multi_Node *)fieldPtr;
					MNode->n=0;
					FREE_IF_NZ(MNode->p);
					break;
				case FIELDTYPE_MFColor:
					MColor=(struct Multi_Color *)fieldPtr;
					MColor->n=0;
					FREE_IF_NZ(MColor->p);
					break;
				case FIELDTYPE_MFColorRGBA:
					MColorRGBA=(struct Multi_ColorRGBA *)fieldPtr;
					MColorRGBA->n=0;
					FREE_IF_NZ(MColorRGBA->p);
					break;
				case FIELDTYPE_MFTime:
					MTime=(struct Multi_Time *)fieldPtr;
					MTime->n=0;
					FREE_IF_NZ(MTime->p);
					break;
				case FIELDTYPE_MFString: 
					MString=(struct Multi_String *)fieldPtr;
					struct Uni_String* ustr;
					for (j=0; j<MString->n; j++) {
						ustr=MString->p[j];
						ustr->len=0;
						ustr->touched=0;
						FREE_IF_NZ(ustr->strptr);
					}
					MString->n=0;
					FREE_IF_NZ(MString->p);
					break;
				case FIELDTYPE_MFVec2f:
					MVec2f=(struct Multi_Vec2f *)fieldPtr;
					MVec2f->n=0;
					FREE_IF_NZ(MVec2f->p);
					break;
				case FIELDTYPE_FreeWRLPTR:
					VPtr = (uintptr_t *) fieldPtr;
					FREE_IF_NZ(*VPtr);
					break;
				case FIELDTYPE_SFString:
					VPtr = (uintptr_t *) fieldPtr;
					MyS = (struct Uni_String *) *VPtr;
					MyS->len = 0;
					FREE_IF_NZ(MyS->strptr);
					FREE_IF_NZ(MyS);
					break;
					
				default:; /* do nothing - field not malloc'd */
			}
			fieldOffsetsPtr+=4;	
		}
		FREE_IF_NZ(memoryTable[i]);
		memoryTable[i]=0;
	}
	FREE_IF_NZ(memoryTable);
	memoryTable=NULL;
	tableIndexSize=0;
	nextEntry=0;
}


