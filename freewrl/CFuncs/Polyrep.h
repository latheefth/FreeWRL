/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

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


#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}


/* transformed ray */
extern struct pt t_r1;
extern struct pt t_r2;
extern struct pt t_r3;



int
count_IFS_faces(int cin,
				struct VRML_IndexedFaceSet *this_IFS);

int 
IFS_face_normals(struct pt *facenormals,
				 int *faceok,
				 int *pointfaces,
				 int faces,
				 int npoints,
				 int cin,
				 struct SFColor *points,
				 struct VRML_IndexedFaceSet *this_IFS,
				 int ccw);

void
IFS_check_normal(struct pt *facenormals,
				 int this_face,
				 struct SFColor *points,
				 int base,
				 struct VRML_IndexedFaceSet *this_IFS,
				 int ccw);

void
add_to_face(int point,
			int face,
			int *pointfaces);

void
Elev_Tri(int vertex_ind,
		 int this_face,
		 int A,
		 int D,
		 int E,
		 int NONORMALS,
		 struct VRML_PolyRep *this_Elev,
		 struct pt *facenormals,
		 int *pointfaces,
		 int ccw);

void
Extru_tex(int vertex_ind,
		  int tci_ct,
		  int A,
		  int B,
		  int C,
		  struct VRML_PolyRep *this_Elev,
		  int ccw,
		  int tcindexsize);

void
Extru_ST_map(int triind_start,
			 int start,
			 int end,
			 float *Vals,
			 int nsec,
			 struct VRML_PolyRep *this_Extru,
			 int tcoordsize);

void
Extru_check_normal(struct pt *facenormals,
				   int this_face,
				   int dire,
				   struct VRML_PolyRep *rep_,
				   int ccw);

void
do_color_normal_reset(void);

void
do_glColor4fv(struct SFColorRGBA *dest, GLfloat *param, int isRGBA);

void
do_glNormal3fv(struct SFColor *dest, GLfloat *param);

void
stream_polyrep(void *node,
			   struct SFColor *points,
			   int ncolors,
			   struct SFColor *colors,
			   int nnormals,
			   struct SFColor *normals,
        struct VRML_TextureCoordinate *tc,

				int isRGBA);

void
render_ray_polyrep(void *node,
				   struct SFColor *points);

void regen_polyrep(void *node, void *coord, void *color, void *normal, void *texCoord);

