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


int
get_now_mapped()
{
	return now_mapped;
}


void
set_now_mapped(int val)
{
	now_mapped = val;
}


void
glpOpenGLInitialize()
{
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
	glEnable(GL_LIGHT0);
	glEnable(GL_CULL_FACE);

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_ALIGNMENT,1);

	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (float) (0.2 * 128));
}

void
BackEndClearBuffer()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
BackEndLightsOff()
{
	glDisable(GL_LIGHT1); /* Put them all off first (except headlight)*/
	glDisable(GL_LIGHT2);
	glDisable(GL_LIGHT3);
	glDisable(GL_LIGHT4);
	glDisable(GL_LIGHT5);
	glDisable(GL_LIGHT6);
	glDisable(GL_LIGHT7);
}

void
BackEndHeadlightOff()
{
	glDisable(GL_LIGHT0); /* headlight off (or other, if no headlight) */
}


void
BackEndHeadlightOn()
{
	float pos[] = { 0.0, 0.0, 1.0, 0.0 };
	float s[] = { 1.0, 1.0, 1.0, 1.0 };

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, s);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, s);
	glLightfv(GL_LIGHT0, GL_SPECULAR, s);
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
