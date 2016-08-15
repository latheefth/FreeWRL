/*


???

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"



#if defined(_MSC_VER)
typedef  void (__stdcall *_GLUfuncptr)();
#endif

/* WIN32 p.411 openGL programmers guide - windows needs CALLBACK, unix not */
#ifndef CALLBACK 
#define CALLBACK
#endif


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
 * GLDOUBLE s with enough space)
 * After calling gluTessEndPolygon() these vector will be filled.
 * global_tess_polyrep->ntri will contain the absolute
 * number of triangles in global_tess_polyrep after tessellation.
 */


typedef struct pTess{
	int global_IFS_Coords[TESS_MAX_COORDS]; //200,000
}* ppTess;
void *Tess_constructor(){
	void *v = MALLOCV(sizeof(struct pTess));
	memset(v,0,sizeof(struct pTess));
	return v;
}
void Tess_init(struct tTess *t){
	//public
//int global_IFS_Coord_count=0;

	//private
	t->prv = Tess_constructor();
	{
		ppTess p = (ppTess)t->prv;
		t->global_IFS_Coords = p->global_IFS_Coords;
	}
}
//ppTess p = (ppTess)gglobal()->Tess.prv;

/* OpenGL-ES 2.0 does not have tessellator */
/* and now all the callback functions, which will be called
	by OpenGL automatically, if the Polygon is specified	*/

void CALLBACK FW_tess_begin(GLenum e) {
               /*printf(" FW_tess_begin   e = %s\n", (e == GL_TRIANGLES ? "GL_TRIANGLES" : "UNKNOWN")); */
		/* we only should get GL_TRIANGLES as type, because
		we defined  the edge_flag callback		*/
		/* check, if the structure is there		*/
	if(e!=GL_TRIANGLES)
		freewrlDie("Something went wrong while tessellating!");
}

void CALLBACK FW_tess_end(void) {
	/*printf("FW_tess_end: Tesselation done.\n"); */
	/* nothing to do	*/
}

void CALLBACK FW_tess_edgeflag(GLenum flag) {
	/*printf("FW_tess_edgeflag: An edge was done (flag = %d).\n", flag); */
	/* nothing to do, this function has to be registered
	so that only GL_TRIANGLES are used	*/
}

void CALLBACK FW_IFS_tess_vertex(void *p) {
	int *dp;
	ttglobal tg = gglobal();
	dp =(int*)p;

	if (tg->Tess.global_IFS_Coord_count == TESS_MAX_COORDS) {
		/* printf ("FW_IFS_tess_vertex, too many coordinates in this face, change TESS_MAX_COORDS\n"); */
		/*
		global_IFS_Coord_count++;
		global_IFS_Coords[global_IFS_Coord_count] =
			global_IFS_Coords[global_IFS_Coord_count-1];
		*/
	} else {
		//printf ("FW_IFS_tess_vertex, global_ifs_coord count %d, pointer %d\n",tg->Tess.global_IFS_Coord_count,*dp);
		//if(*dp < 0){
		//	printf("dp pointer = %p\n",dp);
		//}
		tg->Tess.global_IFS_Coords[tg->Tess.global_IFS_Coord_count++] = *dp;
	}

}

void CALLBACK FW_tess_error(GLenum e) {
	/* Prints out tesselation errors. Older versions of at least MESA would
	 give errors, so for now at least, lets just ignore them.
	*/
	 printf("FW_tess_error %d: >%s<\n",(int) e,GL_ERROR_MSG); 
}



void CALLBACK FW_tess_combine_text_data (GLDOUBLE c[3], GLfloat *d[4], GLfloat w[4], void **out,void *polygondata) {
/*	Component_Text Combiner
	printf("FW_tess_combine data\n"); 
	 printf("combine c:%lf %lf %lf\ndw: %f %f %f %f\n\n",
		c[0],c[1],c[2],w[0],w[1],w[2],w[3]); 
	printf ("vertex 0 %lf %lf %lf, 1 %lf %lf %lf, 2 %lf %lf %lf, 3 %lf %lf %lf\n",
		*d[0]->x,*d[0]->y,*d[0]->z,
		*d[1]->x,*d[1]->y,*d[1]->z,
		*d[2]->x,*d[2]->y,*d[2]->z,
		*d[3]->x,*d[3]->y,*d[3]->z); 

	printf ("d %d %d %d %d\n",d[0],d[1],d[2],d[3]);
	printf ("d %f %f %f %f\n",*d[0],*d[1],*d[2],*d[3]);
	printf ("new coord %d\n",nv);
*/
	if(0){
		GLDOUBLE *nv = MALLOC(GLDOUBLE *, sizeof(GLDOUBLE)*3);

		nv[0] = c[0];
		nv[1] = c[1];
		nv[2] = c[2];
		*out = nv;
	}else{
		int FW_pointctr, RAI_indx;
		text_combiner_data *cbdata;
		float *coords;
		//GLDOUBLE *nv = MALLOC(GLDOUBLE *, sizeof(GLDOUBLE)*6);
		ttglobal tg = gglobal();
		cbdata = (text_combiner_data*) polygondata;

		//OpenGL Redbook says we must malloc a new point. 
		//but in our Component_Text system, that just means adding it to our 
		//over-malloced list of points actualCoords[]
		// and to a few other lists of indexes etc as we do in FW_NewVertexPoint() in Component_Text
		FW_pointctr = *(cbdata->counter);
		RAI_indx = *(cbdata->riaindex);
		tg->Tess.global_IFS_Coords[RAI_indx] = FW_pointctr;
		coords = (float *)cbdata->coords;
		coords[FW_pointctr*3+0] = (float)c[0];
		coords[FW_pointctr*3+1] = (float)c[1];
		coords[FW_pointctr*3+2] = (float)c[2];
		cbdata->ria[(*cbdata->riaindex)] = FW_pointctr;
		*out = &cbdata->ria[(*cbdata->riaindex)]; //tell FW_IFS_tess_vertex the index of the new point
		//printf("combiner, out pointer = %p nv pointer = %p\n",out,*out);
		//THE SECRET TO COMBINDER SUCCESS? *out == (p) in FW_IFS_tess_vertex(void *p)
		*(cbdata->counter) = FW_pointctr + 1;
		(*cbdata->riaindex)++;
	}
}

void CALLBACK FW_tess_combine_polyrep_data (GLDOUBLE c[3], GLfloat *d[4], GLfloat w[4], void **out,void *polygondata) {
/*	PolyRep Combiner (not properly implemented as of Aug 5, 2016)
	printf("FW_tess_combine data\n"); 
	 printf("combine c:%lf %lf %lf\ndw: %f %f %f %f\n\n",
		c[0],c[1],c[2],w[0],w[1],w[2],w[3]); 
	printf ("vertex 0 %lf %lf %lf, 1 %lf %lf %lf, 2 %lf %lf %lf, 3 %lf %lf %lf\n",
		*d[0]->x,*d[0]->y,*d[0]->z,
		*d[1]->x,*d[1]->y,*d[1]->z,
		*d[2]->x,*d[2]->y,*d[2]->z,
		*d[3]->x,*d[3]->y,*d[3]->z); 

	printf ("d %d %d %d %d\n",d[0],d[1],d[2],d[3]);
	printf ("d %f %f %f %f\n",*d[0],*d[1],*d[2],*d[3]);
	printf ("new coord %d\n",nv);
*/
	if(1){
		GLDOUBLE *nv = MALLOC(GLDOUBLE *, sizeof(GLDOUBLE)*3);

		nv[0] = c[0];
		nv[1] = c[1];
		nv[2] = c[2];
		*out = nv;
		// doesn't render right: http://dug9.users.sourceforge.net/web3d/tests/CAD/test_IFS_concave_combiner.x3d
		/*
		geometry DEF FUNNYU IndexedFaceSet {
		convex FALSE
		solid FALSE
		coordIndex [ 0 1 2 3 4 5 6 7 -1]
		coord Coordinate {
		#                                    x-swap-x  inner bottom u point criss crossed to force combiner
		 point [ -2 -2 0, -2 2 0, -1 2 0, 1 -1 0, -1 -1 0, 1 2 0, 2 2 0, 2 -2 0,]
		 }
		}
		 */

	}else{
		//Aug 3, 2016 this doesn't work, didn't pick through polyrep, don't use.
		/*	
		Current polyrep Algo: ignor opengl tips on combiner, and instead try and capture the index into 
		the original node coord, texcoord, normal via 
			tg->Tess.global_IFS_Coords[tg->Tess.global_IFS_Coord_count++] = *dp; 
		in the vertex callback, as we do for Text
		Complication: when adding a point, the result may be more triangles, for which there needs to be more 
		    normals and texcoords etc.

		Hypothesis: the node orig-to-triangle approach in genpolyrep was to save memory back in 2003. We don't need it now.
		Proposed polyrep algo A:
		1. copy node orig data to packed
			a) de-index
			b) convert to double for tess
			c) pack ie [double xyz float rgb float norm float texcoord] for tess, in over-malloced packed array
		2. tesselate 
			a) add combiner generated pack-points to the bottom of packed array
			b) out= weighted combined as redbook shows
		3. copy tesselated to polyrep
			a) convert to float
			b) un-pack
			c) copy unpacked to polyrep for shader
		
		Proposed polyrep algo B:
		1. in combiner, malloc combiner points, texcoords, normals, color-per-vertex on extension arrays
			pass index into extension arrays to *out with a -ve sentinal value, for capture by global_IFS_Coords[] in vertex callback
		2. in make_polyrep and make_extrusion, when using global_IFS_Coords[] array, watch for -ve index and 
			de-index from the extension arrays

		Proposed polyrep algo C?
		- when setting the vertexes, instead of giving [double xyz float rgb], give [double xyz int index] 
		- then in here, the 4 float *d points coming in will deliver index.
		- then (somehow) use those indexes
	

		*/
		//polyrep_combiner_data *cbdata;
	}
}

/* Some tesselators will give back garbage. Lets try and remove it */
/* Text handles errors better itself, so this is just used for Extrusions and IndexedFaceSets */
void verify_global_IFS_Coords(int max) {
	int count;
	ttglobal tg = gglobal();

	for (count = 0; count < tg->Tess.global_IFS_Coord_count; count++) {
		/*printf ("verifying count %d; val is %d, max %d\n",
				count,global_IFS_Coords[count],max); */
		if ((tg->Tess.global_IFS_Coords[count] < 0) ||
			(tg->Tess.global_IFS_Coords[count] >= max)) {

			if (count == 0) {
				tg->Tess.global_IFS_Coords[count] = 0;
			} else {
				tg->Tess.global_IFS_Coords[count] = tg->Tess.global_IFS_Coords[count-1];
			}

		}
	}
}

void CALLBACK FW_tess_combine (GLDOUBLE c[3], void *d[4], GLfloat w[4], void **out) {
	GLDOUBLE *nv = MALLOC(GLDOUBLE *, sizeof(GLDOUBLE)*3);
	/*printf("FW_tess_combine c:%lf %lf %lf\ndw: %f %f %f %f\n\n",
		c[0],c[1],c[2],w[0],w[1],w[2],w[3]); */
	nv[0] = c[0];
	nv[1] = c[1];
	nv[2] = c[2];
	*out = nv;
}


/* next function has to be called once, after an OpenGL context is made
	and before tessellation is started			*/

void CALLBACK XXtessA() { printf ("GLU_TESS_BEGIN\n"); }
void CALLBACK XXtessB() { printf ("GLU_TESS_BEGIN_DATA\n"); }
void CALLBACK XXtessC() { printf ("GLU_TESS_EDGE\n"); }
void CALLBACK XXtessD() { printf ("GLU_TESS_EDGE_FLAG_DATA\n"); }
void CALLBACK XXtessE() { printf ("GLU_TESS_VERTEX\n"); }
void CALLBACK XXtessF() { printf ("GLU_TESS_VERTEX_DATA\n"); }
void CALLBACK XXtessG() { printf ("GLU_TESS_END\n"); }
void CALLBACK XXtessH() { printf ("GLU_TESS_END_DATA\n"); }
void CALLBACK XXtessI() { printf ("GLU_TESS_COMBINE_DATA\n"); }
void CALLBACK XXtessJ() { printf ("GLU_TESS_ERROR\n"); }
void CALLBACK XXtessK() { printf ("GLU_TESS_ERROR_DATA\n"); }


void new_tessellation(void) {
	ttglobal tg = gglobal();
	tg->Tess.global_tessobj=FW_GLU_NEW_TESS();
	if(!tg->Tess.global_tessobj)
		freewrlDie("Got no memory for Tessellation Object!");

	/* register the CallBackfunctions				*/
	FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj,GLU_TESS_BEGIN,(_GLUfuncptr)FW_tess_begin);
	FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj,GLU_TESS_EDGE_FLAG,(_GLUfuncptr)FW_tess_edgeflag);
	//FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj,GLU_VERTEX,(_GLUfuncptr)FW_IFS_tess_vertex);
	FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj,GLU_TESS_VERTEX,(_GLUfuncptr)FW_IFS_tess_vertex);
	FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj,GLU_TESS_ERROR,(_GLUfuncptr)FW_tess_error);
	FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj,GLU_TESS_END,(_GLUfuncptr)FW_tess_end);
	FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj, GLU_TESS_COMBINE_DATA,(_GLUfuncptr)FW_tess_combine_polyrep_data); //default combiner, Text must reset to this after doing its own FW_tess_combine_text_data
	//FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj, GLU_TESS_COMBINE,(_GLUfuncptr)FW_tess_combine);

	    /* Unused right now. */
/*
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_BEGIN, (_GLUfuncptr)XXtessA);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_BEGIN_DATA,(_GLUfuncptr)XXtessB);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_EDGE_FLAG,(_GLUfuncptr)XXtessC);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_EDGE_FLAG_DATA,(_GLUfuncptr)XXtessD);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_VERTEX,(_GLUfuncptr)XXtessE);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_VERTEX_DATA,(_GLUfuncptr)XXtessF);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_END,(_GLUfuncptr)XXtessG);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_END_DATA,(_GLUfuncptr)XXtessH);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_COMBINE_DATA,(_GLUfuncptr)XXtessI);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_ERROR,(_GLUfuncptr)XXtessJ);
	    FW_GLU_TESS_CALLBACK(global_tessobj, GLU_TESS_ERROR_DATA,(_GLUfuncptr)XXtessK);
*/
/*	    */
}
void register_Text_combiner(){
	//called before tesselating Text in Component_Text.c
	ttglobal tg = gglobal();
	if(tg->Tess.global_tessobj){
		//FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj, GLU_TESS_COMBINE_DATA,(_GLUfuncptr)NULL);
		FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj, GLU_TESS_COMBINE_DATA,(_GLUfuncptr)FW_tess_combine_text_data);
	}
}
void register_Polyrep_combiner(){
	//called after tesselating Text in Component_Text.c, so in make_polyrep and make_extrusion in GenPolyrep.c this will be the default
	ttglobal tg = gglobal();
	if(tg->Tess.global_tessobj){
		//FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj, GLU_TESS_COMBINE_DATA,(_GLUfuncptr)NULL);
		FW_GLU_TESS_CALLBACK(tg->Tess.global_tessobj, GLU_TESS_COMBINE_DATA,(_GLUfuncptr)FW_tess_combine_polyrep_data);
	}
}
/* next function should be called once at the end, but where?	*/
void destruct_tessellation(void) {
	ttglobal tg = gglobal();
	FW_GLU_DELETETESS(tg->Tess.global_tessobj);
	printf("Tessellation Object deleted!\n");
}

