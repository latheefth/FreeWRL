/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*********************************************************************
 * General tessellation functions
 *
 * to use the tessellation function, you have to
 * let global_tess_polyrep point towards a structure.
 * global_tess_polyrep->ntri is the first index number which will
 * be filled by the routines (which is the number of triangles
 * already represented in global_tess_polyrep)
 * global_tess_polyrep->cindex and global_tess_polyrep->coords have
 * to point towards enough memory.
 * (And you have to give gluTessVertex a third argument, in which
 * the new coords are written, it has to be a vector of
 * GLdouble s with enough space)
 * After calling gluTessEndPolygon() these vector will be filled.
 * global_tess_polyrep->ntri will contain the absolute
 * number of triangles in global_tess_polyrep after tessellation.
 */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "Structs.h"
#include "headers.h"

  
GLUtriangulatorObj *global_tessobj;	
struct VRML_PolyRep *global_tess_polyrep=NULL;

/* and now all the callback functions, which will be called
	by OpenGL automatically, if the Polygon is specified	*/

void ber_tess_begin(GLenum e) {
               //printf(" ber_tess_begin   e = %s\\n", (e == GL_TRIANGLES ? "GL_TRIANGLES" : "UNKNOWN")); 
		/* we only should get GL_TRIANGLES as type, because
		we defined  the edge_flag callback		*/
		/* check, if the structure is there		*/
	if(e!=GL_TRIANGLES) 
		die("Something went wrong while tessellating!");
}

void ber_tess_end(void) {
	//printf("ber_tess_end: Tesselation done.\\n");
	/* nothing to do	*/
}

void ber_tess_edgeflag(GLenum flag) {
	// printf("ber_tess_end: An edge was done (flag = %d).\\n", flag); 
	/* nothing to do, this function has to be registered
	so that only GL_TRIANGLES are used	*/
}

void ber_tess_vertex(void *p) {
	static int x=0;
	GLdouble *dp=p;

#define GTP global_tess_polyrep
	
	if(GTP->ntri >= GTP->alloc_tri) {
		die("Too many tesselated triangles!");
	}

	//printf("ber_tess_vertex: A vertex is coming   x = %d   ntri = %d.\\n", x, GTP->ntri); 
#undef GTP
	
	global_tess_polyrep->coord[(global_tess_polyrep->ntri)*9+x*3]  =dp[0];
	global_tess_polyrep->coord[(global_tess_polyrep->ntri)*9+x*3+1]=dp[1];
	global_tess_polyrep->coord[(global_tess_polyrep->ntri)*9+x*3+2]=dp[2];
	global_tess_polyrep->cindex[(global_tess_polyrep->ntri)*3+x]=
					(global_tess_polyrep->ntri)*3+x;
	if(x==2) {
		x=0;
		(global_tess_polyrep->ntri)++;
	} else x++;
}

void ber_tess_error(GLenum e) {
	printf("ERROR %d: >%s<\n",e,gluErrorString(e));
}

/* next function has to be called once, after an OpenGL context is made
	and before tessellation is started			*/
	
void new_tessellation(void) {
	global_tessobj=gluNewTess();
	if(!global_tessobj)
		die("Got no memory for Tessellation Object!");
		
	/* register the CallBackfunctions				*/
	gluTessCallback(global_tessobj,GLU_BEGIN,ber_tess_begin);
	gluTessCallback(global_tessobj,GLU_EDGE_FLAG,ber_tess_edgeflag);
	gluTessCallback(global_tessobj,GLU_VERTEX,ber_tess_vertex);
	gluTessCallback(global_tessobj,GLU_ERROR,ber_tess_error);
	gluTessCallback(global_tessobj,GLU_END,ber_tess_end);
	
	//printf("Tessellation Initialized!\n");
}

/* next function should be called once at the end, but where?	*/
void destruct_tessellation(void) {
	gluDeleteTess(global_tessobj);
	printf("Tessellation Object deleted!\n"); 
}


