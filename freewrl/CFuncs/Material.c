/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*****************************************

Material.c - only do material settings that "matter"
		and bounds check all values.

******************************************/


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
#include "LinearAlgebra.h"

/* default - set in the OpenGL initialization */
GLfloat default_shininess = 25.6;

/* default material properties */
GLfloat default_diffuse[]  = {0.8,0.8,0.8,1.0};
GLfloat default_ambient[]  = {0.2,0.2,0.2,1.0};
GLfloat default_specular[] = {0.0,0.0,0.0,1.0};
GLfloat default_emission[] = {0.0,0.0,0.0,1.0};

void do_shininess (float shininess) {
	if ((shininess > 128.0) || (shininess < 0.0)) {
		//JAS printf ("Shininess %f outside of bounds\n",shininess/128.0);
		//JAS return;  /* bounds check */
		if (shininess>128.0){shininess = 128.0;}else{shininess=0.0;}
	}

	if (fabs(default_shininess - shininess) > 1.0) {
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (float)default_shininess);
	}
}

void do_glMaterialfv (GLenum face, GLenum pname, GLfloat *param) {
	GLfloat *myfloats;
	int i,diff;

	for (i=0; i<4; i++) {
		if ((param[i] < 0.0) || (param[i] >1.0)) {
			//printf ("do_glMaterialfv, pname %d idx %d val %f out of range\n",
			//		pname,i,param[i]);
			if (param[i]>1.0) {param[i]=1.0;} else {param[i]=0.0;}
			//JAS return; /* bounds check error found, break out */
		}
	}

	switch (pname) {
		case GL_DIFFUSE:	myfloats = default_diffuse; break;
		case GL_AMBIENT:	myfloats = default_ambient; break;
		case GL_SPECULAR:	myfloats = default_specular; break;
		case GL_EMISSION:	myfloats = default_emission; break;
		default:		printf ("do_glMaterialfv - unknown pname\n"); return;
	}

	/* compare default values with new */

	diff = FALSE;
	for (i=0; i<4; i++) {
		if (fabs(myfloats[i]-param[i]) > 0.001) {
			diff = TRUE;
			break;
		}
	}

	if (diff) { glMaterialfv (face,pname,param); }
}



int verify_rotate(GLfloat *params) {
	/* angle very close to zero? */
	if (fabs(params[3]) < 0.001) return FALSE;
	return TRUE;
}

int verify_translate(GLfloat *params) {
	/* no translation? */

	if ((fabs(params[0]) < 0.001) &&
		(fabs(params[1]) < 0.001) &&
		(fabs(params[2]) < 0.001))  return FALSE;
	return TRUE;
}


int verify_scale(GLfloat *params) {
	/* no translation? */

	if ((fabs(params[0]-1.0) < 0.001) &&
		(fabs(params[1]-1.0) < 0.001) &&
		(fabs(params[2]-1.0) < 0.001))  return FALSE;

	return TRUE;
}

