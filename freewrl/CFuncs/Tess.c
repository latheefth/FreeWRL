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
	printf ("tess vertex, %d count now is %d\n",*dp,global_IFS_Coord_count);

}

void FW_Extrusion_tess_vertex(void *p) {
	static int x=0;
	GLdouble *dp=p;

	if(global_tess_polyrep->ntri >= global_tess_polyrep->alloc_tri) {
		die("Too many tesselated triangles!");
	}

	/* printf ("FW_tess_vertex, %f %f %f\n",dp[0],dp[1],dp[2]);*/
	/* printf("FW_tess_vertex: A vertex is coming   x = %d   ntri = %d.\n", x, global_tess_polyrep->ntri); */
	

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

void FW_tess_error(GLenum e) {
	/* printf("ERROR %d: >%s<\n",e,gluErrorString(e));*/
}

void tesselize_ifs() {
	gluTessCallback(global_tessobj,GLU_VERTEX,FW_IFS_tess_vertex);
}

void tesselize_extrusion() {
	gluTessCallback(global_tessobj,GLU_VERTEX,FW_Extrusion_tess_vertex);
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
}

/* next function should be called once at the end, but where?	*/
void destruct_tessellation(void) {
	gluDeleteTess(global_tessobj);
	printf("Tessellation Object deleted!\n"); 
}


