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

extern VRML_Viewer Viewer; //in VRMLC.pm
void statusbar_position ();


int new_status = TRUE; 		// do we need to re-calculate status bar
GLuint status_dlist = 0;	// status bar display list
int viewer_type = 0;		// WALK, etc...
char vpname[25];		// Viewpoint name
int vplen = 0;			// Viewpoint length
GLdouble initial_angle = 0.0;	// initial rotation after vp bind
int get_angle = TRUE;		// record the initial angle

/* trigger a update */
void update_status() {
	new_status = TRUE;
}

/* changed viewpoints - get the name */
void viewpoint_name_status (char *str) {
	vplen = strlen(str);
	if (vplen > 20) vplen = 20;
	else vplen = strlen(str);
	strncpy (vpname,str,vplen);
	vpname[vplen] = 0; // make sure terminated
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

	/* make up new line to display */
	if (vplen == 0) {
		strcat (vpname, "NONE");
		vplen = strlen (vpname);
	}

	sprintf (statusline, "VP: %s   FPS: %5.2f  NAV: %s  (? help)", 
		vpname, BrowserFPS, VIEWER_STRING(viewer_type));

	/* we are here; compile and display a new background! */
	status_dlist = glGenLists(1);
	new_status = FALSE;
	glNewList(status_dlist,GL_COMPILE_AND_EXECUTE);

	glDisable (GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
        glColor3f(1.0,1.0,1.0);


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

	FW_rendertext (1,		// lines
		NULL,			// Perl SV pointer
		statusline,	 	// text to display
		0,			// number of length lines
		0,			// pointer to length lines
		0.0,			// max extent
		1.0,			// spacing
		0.2,			// size
		//0x2427,			// Font, etc
		0x8827,			// Font, etc
		&rep_);			// pointer to polyrep structure


	/* now that we have the text, go somewhere and render it */
	glTranslated (0.0, -1.0, -3.0);
	render_polyrep(&holder,0,NULL,0,NULL,0,NULL,0,NULL);

	glEndList();
	glPopMatrix();
	glPopAttrib();
}


void statusbar_position () {
	double len;
	struct pt dr1r2;
	struct pt dr2r3;
	struct pt vec;
	struct pt nor1,nor2;
	struct pt ins;
	static const struct pt yvec = {0,0.05,0};
	static const struct pt zvec = {0,0,-0.05};
	static const struct pt zpvec = {0,0,0.05};
	static const struct pt orig = {0,0,0};
	struct pt t_zvec, t_yvec, t_orig;
	GLdouble modelMatrix[16]; 
	GLdouble projMatrix[16];
	GLdouble rotx, roty, rotz, rota;

	/* transforms viewers coordinate space into sensors coordinate space. 
	 * this gives the orientation of the viewer relative to the sensor.   
	 */
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(orig.x,orig.y,orig.z,modelMatrix,projMatrix,viewport,
		&t_orig.x,&t_orig.y,&t_orig.z);
	gluUnProject(zvec.x,zvec.y,zvec.z,modelMatrix,projMatrix,viewport,
		&t_zvec.x,&t_zvec.y,&t_zvec.z);
	gluUnProject(yvec.x,yvec.y,yvec.z,modelMatrix,projMatrix,viewport,
		&t_yvec.x,&t_yvec.y,&t_yvec.z);

	VECDIFF(t_zvec,t_orig,dr1r2);  /* Z axis */
	VECDIFF(t_yvec,t_orig,dr2r3);  /* Y axis */

	len = sqrt(VECSQ(dr1r2)); VECSCALE(dr1r2,1/len);
	len = sqrt(VECSQ(dr2r3)); VECSCALE(dr2r3,1/len);

	//printf("PROX_INT pos : (%f %f %f)\n", t_orig.x, t_orig.y, t_orig.z);
	glTranslated (t_orig.x, t_orig.y, t_orig.z);
	
	if(APPROX(dr1r2.z,1.0)) {
		/* rotation */
		rotx = 0;
		roty = 0;
		rotz = 1;
		rota = atan2(-dr2r3.x,dr2r3.y);
	} else if(APPROX(dr2r3.y,1.0)) {
		/* rotation */
		rotx = 0;
		roty = 1;
		rotz = 0;
		rota = atan2(dr1r2.x,dr1r2.z);
	} else {
		/* Get the normal vectors of the possible rotation planes */
		nor1 = dr1r2;
		nor1.z -= 1.0;
		nor2 = dr2r3;
		nor2.y -= 1.0;

		/* Now, the intersection of the planes, obviously cp */
		VECCP(nor1,nor2,ins);

		len = sqrt(VECSQ(ins)); VECSCALE(ins,1/len);

		/* the angle */
		VECCP(dr1r2,ins, nor1);
		VECCP(zpvec, ins, nor2);
		len = sqrt(VECSQ(nor1)); VECSCALE(nor1,1/len);
		len = sqrt(VECSQ(nor2)); VECSCALE(nor2,1/len);
		VECCP(nor1,nor2,ins);

		rota = -atan2(sqrt(VECSQ(ins)), VECPT(nor1,nor2));

		/* rotation  - should normalize sometime... */
		rotx = ins.x;
		roty = ins.y;
		rotz = ins.z;

	}
	rota =  rota* 3.1415926536*180 / 10.0;
	//printf("rot: (%f %f %f %f)\n", rota,rotx,roty,rotz);
	glRotated (rota,rotx,roty,rotz);
}
