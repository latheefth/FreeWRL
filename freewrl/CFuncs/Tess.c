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
int global_IFS_Coords[500];
int global_IFS_Coord_count=0;

/* and now all the callback functions, which will be called
	by OpenGL automatically, if the Polygon is specified	*/

void FW_tess_begin(GLenum e) {
               /* printf(" FW_tess_begin   e = %s\n", (e == GL_TRIANGLES ? "GL_TRIANGLES" : "UNKNOWN"));  */
		/* we only should get GL_TRIANGLES as type, because
		we defined  the edge_flag callback		*/
		/* check, if the structure is there		*/
	if(e!=GL_TRIANGLES) 
		die("Something went wrong while tessellating!");
}

void FW_tess_end(void) {
	/* printf("FW_tess_end: Tesselation done.\n"); */
	/* nothing to do	*/
}

void FW_tess_edgeflag(GLenum flag) {
	/* printf("FW_tess_edgeflag: An edge was done (flag = %d).\n", flag); */
	/* nothing to do, this function has to be registered
	so that only GL_TRIANGLES are used	*/
}

void FW_IFS_tess_vertex(void *p) {
	int *dp=p;

	global_IFS_Coords[global_IFS_Coord_count++] = *dp;
	/* printf ("tess vertex, %d count now is %d\n",*dp,global_IFS_Coord_count); */

}

void FW_tess_error(GLenum e) {
	/* printf("ERROR %d: >%s<\n",e,gluErrorString(e));*/
}



void FW_tess_combine (GLdouble c[3], void *d[4], GLfloat w[4], void **out) {
	GLdouble *nv = (GLdouble *) malloc(sizeof(GLdouble)*3);
	//printf("FW_tess_combine\n");
	nv[0] = c[0];
	nv[1] = c[1];
	nv[2] = c[2];
	*out = nv; 
}


/* next function has to be called once, after an OpenGL context is made
	and before tessellation is started			*/
	
void new_tessellation(void) {
	global_tessobj=gluNewTess();
	if(!global_tessobj)
		die("Got no memory for Tessellation Object!");
		
	/* register the CallBackfunctions				*/
	gluTessCallback(global_tessobj,GLU_BEGIN,FW_tess_begin);
	gluTessCallback(global_tessobj,GLU_EDGE_FLAG,FW_tess_edgeflag);
	gluTessCallback(global_tessobj,GLU_VERTEX,FW_IFS_tess_vertex);
	gluTessCallback(global_tessobj,GLU_ERROR,FW_tess_error);
	gluTessCallback(global_tessobj,GLU_END,FW_tess_end);
	gluTessCallback(global_tessobj, GLU_TESS_COMBINE,FW_tess_combine);


	    /* Unused right now.
	    gluTessCallback(triang, GLU_TESS_BEGIN, FW_GLU_TESS_BEGIN);
	    gluTessCallback(triang, GLU_TESS_BEGIN_DATA,FW_GLU_TESS_BEGIN_DATA);
	    gluTessCallback(triang, GLU_TESS_EDGE_FLAG,FW_GLU_TESS_EDGE_FLAG);
	    gluTessCallback(triang, GLU_TESS_EDGE_FLAG_DATA,FW_GLU_TESS_EDGE_FLAG_DATA);
	    gluTessCallback(triang, GLU_TESS_VERTEX,FW_GLU_TESS_VERTEX);
	    gluTessCallback(triang, GLU_TESS_VERTEX_DATA,FW_GLU_TESS_VERTEX_DATA);
	    gluTessCallback(triang, GLU_TESS_END,FW_GLU_TESS_END);
	    gluTessCallback(triang, GLU_TESS_END_DATA,FW_GLU_TESS_END_DATA);
	    gluTessCallback(triang, tess_combine_DATA,FW_tess_combine_DATA);
	    gluTessCallback(triang, GLU_TESS_ERROR,FW_GLU_TESS_ERROR);
	    gluTessCallback(triang, GLU_TESS_ERROR_DATA,FW_GLU_TESS_ERROR_DATA);
	    */
}

/* next function should be called once at the end, but where?	*/
void destruct_tessellation(void) {
	gluDeleteTess(global_tessobj);
	printf("Tessellation Object deleted!\n"); 
}


