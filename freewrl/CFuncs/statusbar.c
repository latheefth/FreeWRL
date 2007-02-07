/*
 * Copyright(C) 1998 Tuomas J. Lukka, 2001, 2002 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#include <math.h>

#include "headers.h"
#include "quaternion.h"
#include "Viewer.h"

#ifdef AQUA
#define HELPER "command-?"
#else
#define HELPER "?"
#endif


void statusbar_position (void);

int new_status = TRUE; 		/*  do we need to re-calculate status bar*/
GLuint status_dlist = 0;	/*  status bar display list*/
int MAX_STATUS_LENGTH = 256;
char status_msg[256];
GLdouble initial_angle = 0.0;	/*  initial rotation after vp bind*/
int get_angle = TRUE;		/*  record the initial angle*/

/* trigger a update */
void update_status(char* msg) {
	strncpy(status_msg, msg, MAX_STATUS_LENGTH);
	new_status = TRUE;
}

void clear_status() {
	bzero(status_msg, MAX_STATUS_LENGTH);
}

void render_status () {
	struct X3D_PolyRep rep_;
	struct X3D_IndexedFaceSet holder;

	glPushAttrib(GL_LIGHTING_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT);
	glShadeModel(GL_SMOOTH);
	glPushMatrix();

	/* perform translation and rotation for text posn */
	statusbar_position ();

	/* lets do this with a display list for now. */
	/* now, is this the same background as before??? */
	if (status_dlist) {
		if (!new_status) {
			glCallList(status_dlist);
			glPopMatrix();
			glPopAttrib();
			return;
		} else {
			glDeleteLists(status_dlist,1);
		}
	}
	status_dlist = glGenLists(1);
	glNewList(status_dlist,GL_COMPILE_AND_EXECUTE);


	/* we are here; compile and display a new background! */
	//new_status = FALSE;

	glDisable (GL_LIGHTING);
	lightState(0,TRUE);
        glColor3d(1.0,1.0,1.0);
	glScalef(0.5,1.0, 1.0);


	rep_.ntri = 0;
        rep_.ccw = 0;        /* ccw field for single faced structures */
        rep_.alloc_tri = 0; /* number of allocated triangles */
        rep_.cindex = 0;   /* triples (per triangle) */
        rep_.colindex = 0;   /* triples (per triangle) */
        rep_.norindex = 0;
        rep_.tcindex = 0; /* triples or null */

	rep_.cindex = 0;
	rep_.coord = 0;
	rep_.colindex = 0;
	rep_.color = 0;
	rep_.norindex = 0;
	rep_.normal = 0;
	rep_.tcindex = 0;
	rep_.GeneratedTexCoords = 0;

	holder._intern = &rep_;
	holder._nparents=0;		/*  stops boundingbox calcs from propagating*/
	holder._nodeType=NODE_Group;/*  ensure that textureGeneration not done in render_polyrep */


	printf("FW_rendertext with message: %s\n", status_msg);
	//usleep(50000);
	FW_rendertext (1,		/*  lines*/
		NULL,			/*  Perl SV pointer*/
		status_msg,	 	/*  text to display*/
		0,			/*  number of length lines*/
		0,			/*  pointer to length lines*/
		0.0,			/*  max extent*/
		1.0,			/*  spacing*/
		0.1,			/*  size*/
		0x8827,			/*  Font, etc*/
		&rep_);			/*  pointer to polyrep structure*/


	/* now that we have the text, go somewhere and render it */
	glTranslated (0.0, -1.0, -3.0);
 
	render_polyrep(&holder);

	/* free the malloc'd memory; the string is now in a display list */
        FREE_IF_NZ(rep_.cindex);
        FREE_IF_NZ(rep_.coord);
        FREE_IF_NZ(rep_.GeneratedTexCoords);
        FREE_IF_NZ(rep_.colindex);
        FREE_IF_NZ(rep_.color);
        FREE_IF_NZ(rep_.norindex);
        FREE_IF_NZ(rep_.normal);
        FREE_IF_NZ(rep_.tcindex);

	glEndList();
	glPopMatrix();
	glPopAttrib();
}


void statusbar_position () {
	glMatrixMode(GL_PROJECTION_MATRIX);
	glLoadIdentity();
	return;

}
