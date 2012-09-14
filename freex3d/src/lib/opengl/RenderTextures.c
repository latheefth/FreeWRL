/*
=INSERT_TEMPLATE_HERE=

$Id$

Texturing during Runtime 
texture enabling - works for single texture, for multitexture. 

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
#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"

#include "Textures.h"
#include "Material.h"


#ifdef TEXVERBOSE
#define SET_TEXTURE_UNIT_AND_BIND(aaa,bbb,ccc) { \
	printf ("cubeFace %d textureUnit %d texture %d at %d\n",aaa,bbb,ccc,__LINE__); \
    glActiveTexture(GL_TEXTURE0+bbb); \
	if (aaa==0) glBindTexture(GL_TEXTURE_2D,ccc); else glBindTexture(GL_TEXTURE_CUBE_MAP,ccc); }
#else
#define SET_TEXTURE_UNIT_AND_BIND(aaa,bbb,ccc) { \
    glActiveTexture(GL_TEXTURE0+bbb); \
    if (aaa==0) glBindTexture(GL_TEXTURE_2D,ccc); else glBindTexture(GL_TEXTURE_CUBE_MAP,ccc); }
#endif



void *RenderTextures_constructor(){
	void *v = malloc(sizeof(struct pRenderTextures));
	memset(v,0,sizeof(struct pRenderTextures));
	return v;
}
void RenderTextures_init(struct tRenderTextures *t){
	//t->textureParameterStack[];
	t->prv = RenderTextures_constructor();
//	{
//		ppRenderTextures p = (ppRenderTextures)t->prv;
//		/* variables for keeping track of status */
//	}
}


/* function params */
static void haveTexCoord(struct X3D_TextureCoordinate *myTCnode);
static void passedInGenTex(struct textureVertexInfo *genTex);
static void haveMultiTexCoord(struct X3D_MultiTextureCoordinate *myMTCnode);

/* which texture unit are we going to use? is this texture not OFF?? Should we set the
   background coloUr??? Larry the Cucumber, help! */

static int setActiveTexture (int c, GLfloat thisTransparency,  GLint *texUnit, GLint *texMode) 
{
	ttglobal tg = gglobal();

	/* which texture unit are we working on? */
    
	/* tie each fw_TextureX uniform into the correct texture unit */
    
	/* here we assign the texture unit to a specific number. NOTE: in the current code, this will ALWAYS
	 * be [0] = 0, [1] = 1; etc. */
	texUnit[c] = c;

#ifdef TEXVERBOSE
	if (getAppearanceProperties()->currentShaderProperties != NULL) {
		printf ("setActiveTexture %d, boundTextureStack is %d, sending to uniform %d\n",c,
			tg->RenderFuncs.boundTextureStack[c],
			getAppearanceProperties()->currentShaderProperties->TextureUnit[c]);
	} else {
		printf ("setActiveTexture %d, boundTextureStack is %d, sending to uniform [NULL--No Shader]\n",c,
			tg->RenderFuncs.boundTextureStack[c]);
	}
#endif
    
	/* is this a MultiTexture, or just a "normal" single texture?  When we
	 * bind_image, we store a pointer for the texture parameters. It is
	 * NULL, possibly different for MultiTextures */

	if (tg->RenderTextures.textureParameterStack[c].multitex_mode == INT_ID_UNDEFINED) {
        
		#ifdef TEXVERBOSE
		printf ("setActiveTexture - simple texture NOT a MultiTexture \n"); 
		#endif

		/* should we set the coloUr to 1,1,1,1 so that the material does not show
		 * through a RGB texture?? */
		/* only do for the first texture if MultiTexturing */
		if (c == 0) {
			#ifdef TEXVERBOSE
			printf ("setActiveTexture - firsttexture  \n"); 
			#endif
			texMode[c]= GL_MODULATE;
		} else {
			texMode[c]=GL_ADD;
		}

	} else {
	/* printf ("muititex source for %d is %d\n",c,tg->RenderTextures.textureParameterStack[c].multitex_source); */
		if (tg->RenderTextures.textureParameterStack[c].multitex_source != MTMODE_OFF) {
		} else {
			glDisable(GL_TEXTURE_2D); /* DISABLE_TEXTURES */
			return FALSE;
		}
	}


	PRINT_GL_ERROR_IF_ANY("");

	return TRUE;
}


void textureDraw_start(struct textureVertexInfo* genTex) {
#ifdef TEXVERBOSE
    ConsoleMessage("textureDraw_start");
#endif
		passedInGenTex(genTex);
}


/* lets disable textures here */
void textureDraw_end(void) {
	int c;
	ppRenderTextures p;
	ttglobal tg = gglobal();
	p = (ppRenderTextures)tg->RenderTextures.prv;
    
#ifdef TEXVERBOSE
	printf ("start of textureDraw_end\n");
#endif

	if (tg->display.rdr_caps.av_multitexture) { // test the availability at runtime of multi textures

	    for (c=0; c<tg->RenderFuncs.textureStackTop; c++) {

		FW_GL_DISABLECLIENTSTATE(GL_TEXTURE_COORD_ARRAY);

	    }

	} else {

		FW_GL_DISABLECLIENTSTATE(GL_TEXTURE_COORD_ARRAY);

	}

	/* DISABLE_TEXTURES */
	/* setting this ENSURES that items, like the HUD, that are not within the normal
	   rendering path do not try and use textures... */
	tg->RenderFuncs.textureStackTop = 0;

        FW_GL_MATRIX_MODE(GL_MODELVIEW);
}

/***********************************************************************************/

static void passedInGenTex(struct textureVertexInfo *genTex) {
	int c;
	int i;
	GLint texUnit[MAX_MULTITEXTURE];
	GLint texMode[MAX_MULTITEXTURE];
	ttglobal tg = gglobal();

    s_shader_capabilities_t *me = getAppearanceProperties()->currentShaderProperties;

	#ifdef TEXVERBOSE
	printf ("passedInGenTex, using passed in genTex, textureStackTop %d\n",tg->RenderFuncs.textureStackTop);
        printf ("passedInGenTex, cubeFace %d\n",getAppearanceProperties()->cubeFace);
	#endif 

    /* simple shapes, like Boxes and Cones and Spheres will have pre-canned arrays */
	if (genTex->pre_canned_textureCoords != NULL) {
       // printf ("passedInGenTex, A\n");
		for (c=0; c<tg->RenderFuncs.textureStackTop; c++) {
            //printf ("passedInGenTex, c= %d\n",c);
			/* are we ok with this texture yet? */
			if (tg->RenderFuncs.boundTextureStack[c]!=0) {
                //printf ("passedInGenTex, B\n");
				if (setActiveTexture(c,getAppearanceProperties()->transparency,texUnit,texMode)) {
                    struct X3D_Node *tt = getThis_textureTransform();
                    //printf ("passedInGenTex, C\n");
                    if (tt!=NULL) do_textureTransform(tt,c);
                    SET_TEXTURE_UNIT_AND_BIND(getAppearanceProperties()->cubeFace,c,tg->RenderFuncs.boundTextureStack[c]);
                   
                    FW_GL_TEXCOORD_POINTER (2,GL_FLOAT,0,genTex->pre_canned_textureCoords);
                    sendClientStateToGPU(TRUE,GL_TEXTURE_COORD_ARRAY);
				}
			}
}
	} else {
        //printf ("passedInGenTex, B\n");
		for (c=0; c<tg->RenderFuncs.textureStackTop; c++) {
            //printf ("passedInGenTex, c=%d\n",c);
			/* are we ok with this texture yet? */
			if (tg->RenderFuncs.boundTextureStack[c]!=0) {
                //printf ("passedInGenTex, C, boundTextureStack %d\n",tg->RenderFuncs.boundTextureStack[c]);
				if (setActiveTexture(c,getAppearanceProperties()->transparency,texUnit,texMode)) {
                    //printf ("passedInGenTex, going to bind to texture %d\n",tg->RenderFuncs.boundTextureStack[c]);
                    struct X3D_Node *tt = getThis_textureTransform();
                    if (tt!=NULL) do_textureTransform(tt,c);
                    SET_TEXTURE_UNIT_AND_BIND(getAppearanceProperties()->cubeFace,c,tg->RenderFuncs.boundTextureStack[c]);
                    
					FW_GL_TEXCOORD_POINTER (genTex->TC_size, 
						genTex->TC_type,
						genTex->TC_stride,
						genTex->TC_pointer);
					sendClientStateToGPU(TRUE,GL_TEXTURE_COORD_ARRAY);
				}
			}
		}

	}
    
    /* set up the selected shader for this texture(s) config */
	if (me != NULL) {
        //printf ("passedInGenTex, we have tts %d tc %d\n",tg->RenderFuncs.textureStackTop, me->textureCount);
        
        if (me->textureCount != -1) 
        glUniform1i(me->textureCount, tg->RenderFuncs.textureStackTop);
        
        
	    for (i=0; i<tg->RenderFuncs.textureStackTop; i++) {
        	//printf (" sending in i%d tu %d mode %d\n",i,i,tg->RenderTextures.textureParameterStack[i].multitex_mode);
            glUniform1i(me->TextureUnit[i],i);
            glUniform1i(me->TextureMode[i],tg->RenderTextures.textureParameterStack[i].multitex_mode);
        }
	#ifdef TEXVERBOSE
	} else {
		printf (" NOT sending in %d i+tu+mode because currentShaderProperties is NULL\n",tg->RenderFuncs.textureStackTop);
	#endif
	}

    
    
	PRINT_GL_ERROR_IF_ANY("");
}
