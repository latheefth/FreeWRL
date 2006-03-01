/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Texturing Component

*********************************************************************/

#include <math.h>
#include "headers.h"
#include "installdir.h"

/* verify the TextureCoordinateGenerator node - if the params are ok, then the internal
   __compiledmode is NOT zero. If there are problems, the __compiledmode IS zero */

void render_TextureCoordinateGenerator(struct X3D_TextureCoordinateGenerator *this) {
	STRLEN xx;
	char *modeptr;

	if (this->_ichange != this->_change) {
		this->_ichange = this->_change;

		modeptr = SvPV (this->mode,xx);

		/* make the __compiledmode reflect actual OpenGL parameters */
		if(strncmp("SPHERE-REFLECT-LOCAL",modeptr,strlen("SPHERE-REFLECT-LOCAL"))==0) {
			this->__compiledmode = GL_SPHERE_MAP;
		} else if(strncmp("SPHERE-REFLECT",modeptr,strlen("SPHERE-REFLECT"))==0) {
			this->__compiledmode = GL_SPHERE_MAP;
		} else if(strncmp("SPHERE-LOCAL",modeptr,strlen("SPHERE-LOCAL"))==0) {
			this->__compiledmode = GL_SPHERE_MAP;
		} else if(strncmp("SPHERE",modeptr,strlen("SPHERE"))==0) {
			this->__compiledmode = GL_SPHERE_MAP;
		} else if(strncmp("CAMERASPACENORMAL",modeptr,strlen("CAMERASPACENORMAL"))==0) {
			this->__compiledmode = GL_NORMAL_MAP;
		} else if(strncmp("CAMERASPACEPOSITION",modeptr,strlen("CAMERASPACEPOSITION"))==0) {
			this->__compiledmode = GL_OBJECT_LINEAR;
		} else if(strncmp("CAMERASPACEREFLECTION",modeptr,strlen("CAMERASPACEREFLECTION"))==0) {
			this->__compiledmode = GL_REFLECTION_MAP;
		} else if(strncmp("COORD-EYE",modeptr,strlen("COORD-EYE"))==0) {
			this->__compiledmode = GL_EYE_LINEAR;
		} else if(strncmp("COORD",modeptr,strlen("COORD"))==0) {
			this->__compiledmode = GL_EYE_LINEAR;
		} else if(strncmp("NOISE-EYE",modeptr,strlen("NOISE-EYE"))==0) {
			this->__compiledmode = GL_EYE_LINEAR;
		} else if(strncmp("NOISE",modeptr,strlen("NOISE"))==0) {
			this->__compiledmode = GL_EYE_LINEAR;
		} else {
			printf ("TextureCoordinateGenerator - error - %s invalid as a mode\n",modeptr);
		}
	}

	


}

void render_TextureCoordinate(struct X3D_TextureCoordinate *this) {
	int i;
	int op;
	struct SFVec2f oFp;

	#ifdef TEXVERBOSE
	struct SFVec2f nFp;
	#endif

	float *fptr;

	#ifdef TEXVERBOSE
	printf ("rendering TextureCoordinate node __compiledpoint %d\n",this->__compiledpoint);
	printf ("tcin %d tcin_count %d oldpoint.n %d\n",global_tcin, global_tcin_count, this->point.n);
	#endif

	/* is this the statusbar? we should *always* have a global_tcin textureIndex */
	if (global_tcin == 0) return;

	if (this->_ichange != this->_change) {
		this->_ichange = this->_change;

		if (this->__compiledpoint.n == 0) {
			this->__compiledpoint.n = global_tcin_count;
			this->__compiledpoint.p = (struct SFVec2f *) malloc (sizeof(float) *2 * global_tcin_count);
		} 
	
		fptr = (float *) this->__compiledpoint.p;
		
		/* ok, we have a bunch of triangles, loop through and stream the texture coords
		   into a format that matches 1 for 1, the coordinates */
	
		for (i=0; i<global_tcin_count; i++) {
			op = global_tcin[i];
	
			/* bounds check - is the tex coord greater than the number of points? 	*/
			/* this should have been checked before hand...				*/
			if (op >= this->point.n) {
				#ifdef TEXVERBOSE
				printf ("renderTextureCoord - op %d npoints %d\n",op,this->point.n);
				#endif
				*fptr = 0.0; fptr++; 
				*fptr = 0.0; fptr++; 
			} else {
				oFp = this->point.p[op];
	
				#ifdef TEXVERBOSE
				printf ("TextureCoordinate copying %d to %d\n",op,i);	
				printf ("	op %f %f\n",oFp.c[0], oFp.c[1]);
				#endif
	
				*fptr = oFp.c[0]; fptr++; *fptr = oFp.c[1]; fptr++;
			}
		}
		
			
		#ifdef TEXVERBOSE
		for (i=0; i<global_tcin_count; i++) {
			nFp = this->__compiledpoint.p[i];
			printf ("checking... %d %f %f\n",i,nFp.c[0], nFp.c[1]);
		}
		#endif
	}

	if (this->__compiledpoint.n < global_tcin_count) {
		printf ("TextureCoordinate - problem %d < %d\n",this->__compiledpoint.n,global_tcin_count);
	}

}


void render_PixelTexture (struct X3D_PixelTexture *node) {
	loadPixelTexture(node,NULL);
	texture_count=1; /* not multitexture - should have saved to bound_textures[0] */
}

void render_ImageTexture (struct X3D_ImageTexture *node) {
	loadImageTexture(node,NULL);
	texture_count=1; /* not multitexture - should have saved to bound_textures[0] */
}

void render_MultiTexture (struct X3D_MultiTexture *node) {
	loadMultiTexture(node);
}

void render_MovieTexture (struct X3D_MovieTexture *node) {
	/* really simple, the texture number is calculated, then simply sent here.
	   The bound_textures field is sent, and, made current */

	/*  if this is attached to a Sound node, tell it...*/
	sound_from_audioclip = FALSE;

	loadMovieTexture(node,NULL);
	bound_textures[texture_count] = node->__ctex;
	/* not multitexture, should have saved to bound_textures[0] */
	
	texture_count=1;
}
