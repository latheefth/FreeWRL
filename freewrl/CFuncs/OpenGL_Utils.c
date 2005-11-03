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

	/* stuff common to all textureTransforms - gets undone at finish_textureTransform */
       	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_TEXTURE);
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
}


/* keep track of lighting */
void lightState(GLint light, int status) {
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
