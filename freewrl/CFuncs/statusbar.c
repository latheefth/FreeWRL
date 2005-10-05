/*
 * Copyright(C) 1998 Tuomas J. Lukka, 2001, 2002 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>
#ifdef AQUA
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "Structs.h"
#include "headers.h"
#include "quaternion.h"
#include "Viewer.h"

#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}

#ifdef AQUA
#define HELPER "command-?"
#else
#define HELPER "?"
#endif


void statusbar_position (void);


int new_status = TRUE; 		/*  do we need to re-calculate status bar*/
GLuint status_dlist = 0;	/*  status bar display list*/
int viewer_type = 0;		/*  WALK, etc...*/
char vpname[25];		/*  Viewpoint name*/
int vplen = 0;			/*  Viewpoint length*/
GLdouble initial_angle = 0.0;	/*  initial rotation after vp bind*/
int get_angle = TRUE;		/*  record the initial angle*/

/* trigger a update */
void update_status() {
	new_status = TRUE;
}

/* changed viewpoints - get the name */
void viewpoint_name_status (char *str) {
	vplen = strlen(str);
	if (vplen > 20) vplen = 20;
	else vplen = strlen(str);
	strncpy (vpname,str,(size_t) vplen);
	vpname[vplen] = 0; /*  make sure terminated*/
	new_status = TRUE;
	get_angle = TRUE;
}

/* we have changed viewer types */
void viewer_type_status (int type) {
	viewer_type = type;
	new_status = TRUE;
}

void render_status () {
	struct VRML_PolyRep rep_;
	struct VRML_IndexedFaceSet holder;

	char statusline[200];

	static GLfloat light_ambient[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_diffuse[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_specular[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};

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

	/* make up new line to display */
	if (vplen == 0) {
		strcat (vpname, "No Viewpoint");
		vplen = strlen (vpname);
	}
	if (isPerlParsing() || isTextureParsing() || (!isPerlinitialized())) {
		sprintf (statusline, "(Loading...)  speed: %2.0f   Mode: %s  HelpKey: %s",
			BrowserFPS, VIEWER_STRING(viewer_type),HELPER);

		/*  let the other threads run, too*/
		sched_yield();

	} else {
		sprintf (statusline, "Viewpoint: %s  Speed: %2.0f  Mode: %s  HelpKey: %s",
		vpname, BrowserFPS, VIEWER_STRING(viewer_type),HELPER);
	}

	/* we are here; compile and display a new background! */
	new_status = FALSE;

	glDisable (GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
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
	rep_.tcoord = 0;

	holder._intern = &rep_;
	holder._nparents=0;		/*  stops boundingbox calcs from propagating*/

	FW_rendertext (1,		/*  lines*/
		NULL,			/*  Perl SV pointer*/
		statusline,	 	/*  text to display*/
		0,			/*  number of length lines*/
		0,			/*  pointer to length lines*/
		0.0,			/*  max extent*/
		1.0,			/*  spacing*/
		0.2,			/*  size*/
		0x8827,			/*  Font, etc*/
		&rep_);			/*  pointer to polyrep structure*/


	/* now that we have the text, go somewhere and render it */
	glTranslated (0.0, -1.0, -3.0);
	render_polyrep(&holder,0,NULL,0,NULL,0,NULL,0,NULL,0,-1);

	/* free the malloc'd memory; the string is now in a display list */
        FREE_IF_NZ(rep_.cindex);
        FREE_IF_NZ(rep_.coord);
        FREE_IF_NZ(rep_.tcoord);
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
