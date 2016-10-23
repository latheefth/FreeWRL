/*


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


typedef struct pRenderTextures{
	struct multiTexParams textureParameterStack[MAX_MULTITEXTURE];
}* ppRenderTextures;
void *RenderTextures_constructor(){
	void *v = MALLOCV(sizeof(struct pRenderTextures));
	memset(v,0,sizeof(struct pRenderTextures));
	return v;
}
void RenderTextures_init(struct tRenderTextures *t){
	t->prv = RenderTextures_constructor();
	{
		ppRenderTextures p = (ppRenderTextures)t->prv;
		/* variables for keeping track of status */
		t->textureParameterStack = (void *)p->textureParameterStack;
	}
}


/* function params */
static void passedInGenTex(struct textureVertexInfo *genTex);

/* which texture unit are we going to use? is this texture not OFF?? Should we set the
   background coloUr??? Larry the Cucumber, help! */

static int setActiveTexture (int c, GLfloat thisTransparency,  GLint *texUnit, GLint *texMode) 
{
	ppRenderTextures p;
	ttglobal tg = gglobal();
	p = (ppRenderTextures)tg->RenderTextures.prv;

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

	if (p->textureParameterStack[c].multitex_mode[0] == INT_ID_UNDEFINED) {
        
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
		if (p->textureParameterStack[c].multitex_source[0] != MTMODE_OFF) {
		} else {
			//glDisable(GL_TEXTURE_2D); /* DISABLE_TEXTURES */
			//return FALSE;
			// we do OFF right in the shader
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
	int j;
	ttglobal tg = gglobal();
    
#ifdef TEXVERBOSE
	printf ("start of textureDraw_end\n");
#endif

	/* DISABLE_TEXTURES */
	/* setting this ENSURES that items, like the HUD, that are not within the normal
	   rendering path do not try and use textures... */
	FW_GL_MATRIX_MODE(GL_TEXTURE);
	for(j=0;j<tg->RenderFuncs.textureStackTop;j++)
		FW_GL_POP_MATRIX(); //pushed in passedInGenTex

	tg->RenderFuncs.textureStackTop = 0;
	tg->RenderFuncs.texturenode = NULL;
	FW_GL_MATRIX_MODE(GL_MODELVIEW);
}

/* did we have a TextureTransform in the Appearance node? */
void do_textureTransform (struct X3D_Node *textureNode, int ttnum) {

	/* is this a simple TextureTransform? */
	if (textureNode->_nodeType == NODE_TextureTransform) {
		//ConsoleMessage ("do_textureTransform, node is indeed a NODE_TextureTransform");
		struct X3D_TextureTransform  *ttt = (struct X3D_TextureTransform *) textureNode;
		/*  Render transformations according to spec.*/
		//http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/texturing.html#TextureTransform
		//specs say 'translate, rotate, then scale'
		FW_GL_TRANSLATE_F(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
		FW_GL_SCALE_F(((ttt->scale).c[0]),((ttt->scale).c[1]),1);			/*  4*/
		FW_GL_ROTATE_RADIANS(ttt->rotation,0,0,1);					/*  3*/
		FW_GL_TRANSLATE_F(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
		FW_GL_TRANSLATE_F(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/
	/* is this a MultiTextureTransform? */
	} else  if (textureNode->_nodeType == NODE_MultiTextureTransform) {
		struct X3D_MultiTextureTransform *mtt = (struct X3D_MultiTextureTransform *) textureNode;
		if (ttnum < mtt->textureTransform.n) {
			struct X3D_TextureTransform *ttt = (struct X3D_TextureTransform *) mtt->textureTransform.p[ttnum];
			/* is this a simple TextureTransform? */
			if (ttt->_nodeType == NODE_TextureTransform) {
				/*  Render transformations according to spec.*/
				FW_GL_TRANSLATE_F(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
				FW_GL_SCALE_F(((ttt->scale).c[0]),((ttt->scale).c[1]),1);			/*  4*/
				FW_GL_ROTATE_RADIANS(ttt->rotation,0,0,1);					/*  3*/
				FW_GL_TRANSLATE_F(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
				FW_GL_TRANSLATE_F(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/
			} else {
				static int once = 0;
				if(!once){
					printf ("MultiTextureTransform expected a textureTransform for texture %d, got %d \n",
					ttnum, ttt->_nodeType);
					once = 1;
				}
			}
		} else {
			static int once = 0;
			if(!once){
				printf ("not enough transforms in MultiTextureTransform -will fill with Identity matrix\n");
				once = 1;
			}
		}
	} else if (textureNode->_nodeType == NODE_TextureTransform3D) {
		//ConsoleMessage ("do_textureTransform, node is indeed a NODE_TextureTransform");
		struct X3D_TextureTransform3D  *ttt = (struct X3D_TextureTransform3D *) textureNode;
		/*  Render transformations according to spec.*/
		FW_GL_TRANSLATE_F(-((ttt->center).c[0]),-((ttt->center).c[1]), -((ttt->center).c[2]));		/*  5*/
		FW_GL_SCALE_F(((ttt->scale).c[0]),((ttt->scale).c[1]),((ttt->scale).c[2]));			/*  4*/
		FW_GL_ROTATE_RADIANS(ttt->rotation.c[3], ttt->rotation.c[0],ttt->rotation.c[1],ttt->rotation.c[2]);
		FW_GL_TRANSLATE_F(((ttt->center).c[0]),((ttt->center).c[1]), ((ttt->center).c[2]));		/*  2*/
		FW_GL_TRANSLATE_F(((ttt->translation).c[0]), ((ttt->translation).c[1]), ((ttt->translation).c[2]));	/*  1*/
	} else if (textureNode->_nodeType == NODE_TextureTransformMatrix3D) {
		//ConsoleMessage ("do_textureTransform, node is indeed a NODE_TextureTransform");
		int i;
		double mat[16];
		struct X3D_TextureTransformMatrix3D  *ttt = (struct X3D_TextureTransformMatrix3D *) textureNode;
		for(i=0;i<16;i++)
			mat[i] = (double)ttt->matrix.c[i];
		FW_GL_SETDOUBLEV(GL_TEXTURE_MATRIX,mat);
	} else {
		static int once = 0;
		if(!once){
			printf ("expected a textureTransform node, got %d\n",textureNode->_nodeType);
			once = 1;
		}
	}

	//FW_GL_MATRIX_MODE(GL_MODELVIEW);
}

/***********************************************************************************/
int isMultiTexture(struct X3D_Node *node){
	int ret = FALSE;
	if(node && node->_nodeType == NODE_MultiTexture)
		ret = TRUE;
	return ret;
}
textureTableIndexStruct_s *getTableTableFromTextureNode(struct X3D_Node *textureNode);
int isTex3D(struct X3D_Node *node);
static void passedInGenTex_OLD(struct textureVertexInfo *genTex) {
	int c;
	int i, isStrict, isMulti, isIdentity;
	GLint texUnit[MAX_MULTITEXTURE];
	GLint texMode[MAX_MULTITEXTURE];
	s_shader_capabilities_t *me;
	struct textureVertexInfo *genTexPtr;
	struct X3D_Node *tnode;

	ppRenderTextures p;
	ttglobal tg = gglobal();
	p = (ppRenderTextures)tg->RenderTextures.prv;
	tnode = tg->RenderFuncs.texturenode;

    me = getAppearanceProperties()->currentShaderProperties;

	#ifdef TEXVERBOSE
	printf ("passedInGenTex, using passed in genTex, textureStackTop %d\n",tg->RenderFuncs.textureStackTop);
	printf ("passedInGenTex, cubeFace %d\n",getAppearanceProperties()->cubeFace);
	#endif 

    FW_GL_MATRIX_MODE(GL_TEXTURE);

    //printf ("passedInGenTex, B\n");
	isStrict = 1;  //web3d specs say if its a multitexture, 
		//and you give it a single textureTransform instead of multitexturetransform 
		//it should ignore the singleTextureTransform and use identities. 
		//strict: This is a change of functionality for freewrl Aug 31, 2016
	genTexPtr = genTex;
	isIdentity = TRUE;
	for (c=0; c<tg->RenderFuncs.textureStackTop; c++) {
		FW_GL_PUSH_MATRIX(); //POPPED in textureDraw_end
		FW_GL_LOAD_IDENTITY();
		//printf ("passedInGenTex, c=%d\n",c);
		/* are we ok with this texture yet? */
		if (tg->RenderFuncs.boundTextureStack[c]!=0) {
			isMulti = isMultiTexture(tg->RenderFuncs.texturenode);
			//printf ("passedInGenTex, C, boundTextureStack %d\n",tg->RenderFuncs.boundTextureStack[c]);
			if (setActiveTexture(c,getAppearanceProperties()->transparency,texUnit,texMode)) {
				//printf ("passedInGenTex, going to bind to texture %d\n",tg->RenderFuncs.boundTextureStack[c]);
				GLuint texture;
				struct X3D_Node *tt = getThis_textureTransform();
				if (tt!=NULL) {
					int match = FALSE;
					match = isMulti && tt->_nodeType == NODE_MultiTextureTransform;
					match = match || !isMulti && tt->_nodeType != NODE_MultiTextureTransform;
					if(isStrict){
						if(match){
							 do_textureTransform(tt,c);
							 isIdentity = FALSE;
						}
					}else{
						do_textureTransform(tt,c);
						 isIdentity = FALSE;
					}
				} 
				//TEXTURE 3D
				if(isTex3D(tnode)){
					textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tnode);
					if(tnode->_nodeType != NODE_ComposedTexture3D){
						//pixelTexture3D, imageTexture3D (but not composedTexture3D which uses textureCount above)
						if(me){
							if(tti){
								glUniform1iv(me->tex3dTiles,3,tti->tiles);
							}
						}
					}
					//all texture3d
					if(tg->RenderFuncs.shapenode && isIdentity && genTexPtr->TC_size < 3){
						//_if_ no TextureTransform3D was explicitly specified for Texture3D, 
						//_and_ no textureCoordinate3D or textureCoordinate4D was explicilty specified with the goem node
						//_then_ bounding box of shape, in local coordinates, is used to scale/translate
						//geometry vertices into 0-1 range on each axis for re-use as default texture3D coordinates
						float bbox[6], *bmin, *bmax;
						struct X3D_Node *gn;
						struct X3D_Shape *sn = (struct X3D_Shape *)tg->RenderFuncs.shapenode;
						POSSIBLE_PROTO_EXPANSION(struct X3D_Node *,sn->geometry,gn);
						if(gn){
							//first vec3 is minimum xyz
							bmin = bbox;
							bmax = &bbox[3];
							for(i=0;i<3;i++){
								bmin[i] = gn->_extent[i*2 + 1];
								bmax[i] = gn->_extent[i*2];
							}
							//second vec3 is 1/size - so can be applied directly in vertex shader
							vecdif3f(bmax,bmax,bmin);
							for(i=0;i<3;i++){
								if(bmax[i] != 0.0f)
									bmax[i] = 1.0f/bmax[i];
								else
									bmax[i] = 1.0f;
							}
							//if(fabs(bmin[0]) > 10.0f)
							//	printf("bbox shift [%f %f %f] scale [%f %f %f]\n",bmin[0],bmin[1],bmin[2],bmax[0],bmax[1],bmax[2]);
							
							//special default texture transform for 3D textures posing as 2D textures

							//the order of applying transform elements seems reversed for texture transforms
							//H: related to order of operands in mat * vec in shader:
							//   fw_TexCoord[0] = vec3(fw_TextureMatrix0 *vec4(texcoord,1.0)); \n\
							// but sign on elements is what you expect
							//flip z from RHS to LHS in fragment shader plug_tex3d apply
							//printf("default tt\n");
							FW_GL_SCALE_F(bmax[0],bmax[1],bmax[2]);  
							FW_GL_TRANSLATE_F(-bmin[0],-bmin[1],-bmin[2]);
						}
					}
					if(me){
						if(tg->RenderFuncs.shapenode && genTexPtr->TC_size < 3){
							//3D but no 3D coords supplied - gen from vertex in vertex shader
							glUniform1i(me->tex3dUseVertex,1); //vertex shader flag to over-ride texCoords with vertex
						}else{
							glUniform1i(me->tex3dUseVertex,0); 
						}
						if(tti){
							if(me->repeatSTR > -1)
								glUniform1iv(me->repeatSTR,3,tti->repeatSTR);
							if(me->magFilter > -1)
								glUniform1i(me->magFilter,tti->magFilter);
						}
					}
				}

				texture = tg->RenderFuncs.boundTextureStack[c];

				// SET_TEXTURE_UNIT_AND_BIND
				glActiveTexture(GL_TEXTURE0+c); 
				//printf("active texture %d texture %d c %d\n",GL_TEXTURE0+c,texture,c);
				if (getAppearanceProperties()->cubeFace==0) {
					glBindTexture(GL_TEXTURE_2D,texture); 
				} else {
					glBindTexture(GL_TEXTURE_CUBE_MAP,texture); 
				}
				if(genTexPtr->VBO)
					FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,genTexPtr->VBO);

				if (genTexPtr->pre_canned_textureCoords != NULL) {
					/* simple shapes, like Boxes and Cones and Spheres will have pre-canned arrays */
					FW_GL_TEXCOORD_POINTER (2,GL_FLOAT,0,genTexPtr->pre_canned_textureCoords,c);
				}else{
					FW_GL_TEXCOORD_POINTER (genTexPtr->TC_size, 
						genTexPtr->TC_type,
						genTexPtr->TC_stride,
						genTexPtr->TC_pointer,c);
				}
			}
		}
		genTexPtr = genTexPtr->next ? genTexPtr->next : genTexPtr; //duplicate the prior coords if not enough for all MultiTextures
	}
	/* set up the selected shader for this texture(s) config */
	if (me != NULL) {
		tnode = tg->RenderFuncs.texturenode;
		//printf ("passedInGenTex, we have tts %d tc %d\n",tg->RenderFuncs.textureStackTop, me->textureCount);

		if (me->textureCount != -1) {
			glUniform1i(me->textureCount, tg->RenderFuncs.textureStackTop);
		}
		if(tg->RenderFuncs.textureStackTop){
			if(isMultiTexture(tg->RenderFuncs.texturenode)){
				struct X3D_MultiTexture * mtnode = (struct X3D_MultiTexture *)tg->RenderFuncs.texturenode;
				glUniform4f(me->multitextureColor,mtnode->color.c[0],mtnode->color.c[1],mtnode->color.c[2],mtnode->alpha);
			}
		}
		for (i=0; i<tg->RenderFuncs.textureStackTop; i++) {
			//static int once = 0;
			//if(once < 10) {
			//printf (" sending in i%d tu %d mode %d src %d fnc %d\n",i,i,
			//	p->textureParameterStack[i].multitex_mode,
			//	p->textureParameterStack[i].multitex_source,
			//	p->textureParameterStack[i].multitex_function);
			//	once++;
			//}
			glUniform1i(me->TextureUnit[i],i);
			//the 2i wasn't working for me even with ivec2 in shader
			glUniform2i(me->TextureMode[i],p->textureParameterStack[i].multitex_mode[0], p->textureParameterStack[i].multitex_mode[1]);
			glUniform2i(me->TextureSource[i],p->textureParameterStack[i].multitex_source[0], p->textureParameterStack[i].multitex_source[1]);
			//glUniform1i(me->TextureMode[i],p->textureParameterStack[i].multitex_mode[0]);
			//glUniform1i(me->TextureSource[i],p->textureParameterStack[i].multitex_source[0]);
			glUniform1i(me->TextureFunction[i],p->textureParameterStack[i].multitex_function);
		}
	#ifdef TEXVERBOSE
	} else {
		printf (" NOT sending in %d i+tu+mode because currentShaderProperties is NULL\n",tg->RenderFuncs.textureStackTop);
	#endif
	}

	FW_GL_MATRIX_MODE(GL_MODELVIEW);

	PRINT_GL_ERROR_IF_ANY("");
}
static void passedInGenTex(struct textureVertexInfo *genTex) {
	int c;
	int i, isStrict, isMulti, isIdentity;
	GLint texUnit[MAX_MULTITEXTURE];
	GLint texMode[MAX_MULTITEXTURE];
	s_shader_capabilities_t *me;
	struct textureVertexInfo *genTexPtr;
	struct X3D_Node *tnode;

	ppRenderTextures p;
	ttglobal tg = gglobal();
	p = (ppRenderTextures)tg->RenderTextures.prv;
	tnode = tg->RenderFuncs.texturenode;

    me = getAppearanceProperties()->currentShaderProperties;

	genTexPtr = genTex;
	for (c=0; c<tg->RenderFuncs.textureStackTop; c++) {
		if(genTexPtr->VBO)
			FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,genTexPtr->VBO);

		if (genTexPtr->pre_canned_textureCoords != NULL) {
			/* simple shapes, like Boxes and Cones and Spheres will have pre-canned arrays */
			FW_GL_TEXCOORD_POINTER (2,GL_FLOAT,0,genTexPtr->pre_canned_textureCoords,c);
		}else{
			FW_GL_TEXCOORD_POINTER (genTexPtr->TC_size, 
				genTexPtr->TC_type,
				genTexPtr->TC_stride,
				genTexPtr->TC_pointer,c);
		}
		genTexPtr = genTexPtr->next ? genTexPtr->next : genTexPtr; //duplicate the prior coords if not enough for all MultiTextures
	}
}


void textureTransform_start() {
	int c;
	int i, isStrict, isMulti, isIdentity;
	GLint texUnit[MAX_MULTITEXTURE];
	GLint texMode[MAX_MULTITEXTURE];
	s_shader_capabilities_t *me;
	struct X3D_Node *tnode;

	ppRenderTextures p;
	ttglobal tg = gglobal();
	p = (ppRenderTextures)tg->RenderTextures.prv;
	tnode = tg->RenderFuncs.texturenode;

    me = getAppearanceProperties()->currentShaderProperties;

	#ifdef TEXVERBOSE
	printf ("passedInGenTex, using passed in genTex, textureStackTop %d\n",tg->RenderFuncs.textureStackTop);
	printf ("passedInGenTex, cubeFace %d\n",getAppearanceProperties()->cubeFace);
	#endif 

    FW_GL_MATRIX_MODE(GL_TEXTURE);

    //printf ("passedInGenTex, B\n");
	isStrict = 1;  //web3d specs say if its a multitexture, 
		//and you give it a single textureTransform instead of multitexturetransform 
		//it should ignore the singleTextureTransform and use identities. 
		//strict: This is a change of functionality for freewrl Aug 31, 2016
	isIdentity = TRUE;
	for (c=0; c<tg->RenderFuncs.textureStackTop; c++) {
		FW_GL_PUSH_MATRIX(); //POPPED in textureDraw_end
		FW_GL_LOAD_IDENTITY();
		//printf ("passedInGenTex, c=%d\n",c);
		/* are we ok with this texture yet? */
		if (tg->RenderFuncs.boundTextureStack[c]!=0) {
			isMulti = isMultiTexture(tg->RenderFuncs.texturenode);
			//printf ("passedInGenTex, C, boundTextureStack %d\n",tg->RenderFuncs.boundTextureStack[c]);
			if (setActiveTexture(c,getAppearanceProperties()->transparency,texUnit,texMode)) {
				//printf ("passedInGenTex, going to bind to texture %d\n",tg->RenderFuncs.boundTextureStack[c]);
				GLuint texture;
				struct X3D_Node *tt = getThis_textureTransform();
				if (tt!=NULL) {
					int match = FALSE;
					match = isMulti && tt->_nodeType == NODE_MultiTextureTransform;
					match = match || !isMulti && tt->_nodeType != NODE_MultiTextureTransform;
					if(isStrict){
						if(match){
							 do_textureTransform(tt,c);
							 isIdentity = FALSE;
						}
					}else{
						do_textureTransform(tt,c);
						 isIdentity = FALSE;
					}
				} 
				//TEXTURE 3D
				if(isTex3D(tnode)){
					textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tnode);
					if(tnode->_nodeType != NODE_ComposedTexture3D){
						//pixelTexture3D, imageTexture3D (but not composedTexture3D which uses textureCount above)
						if(me){
							if(tti){
								glUniform1iv(me->tex3dTiles,3,tti->tiles);
							}
						}
					}
					//all texture3d
					if(tg->RenderFuncs.shapenode && isIdentity ) { //&& genTexPtr->TC_size < 3){
						//_if_ no TextureTransform3D was explicitly specified for Texture3D, 
						//_and_ no textureCoordinate3D or textureCoordinate4D was explicilty specified with the goem node
						//_then_ bounding box of shape, in local coordinates, is used to scale/translate
						//geometry vertices into 0-1 range on each axis for re-use as default texture3D coordinates
						float bbox[6], *bmin, *bmax;
						struct X3D_Node *gn;
						struct X3D_Shape *sn = (struct X3D_Shape *)tg->RenderFuncs.shapenode;
						POSSIBLE_PROTO_EXPANSION(struct X3D_Node *,sn->geometry,gn);
						if(gn){
							//first vec3 is minimum xyz
							bmin = bbox;
							bmax = &bbox[3];
							for(i=0;i<3;i++){
								bmin[i] = gn->_extent[i*2 + 1];
								bmax[i] = gn->_extent[i*2];
							}
							//second vec3 is 1/size - so can be applied directly in vertex shader
							vecdif3f(bmax,bmax,bmin);
							for(i=0;i<3;i++){
								if(bmax[i] != 0.0f)
									bmax[i] = 1.0f/bmax[i];
								else
									bmax[i] = 1.0f;
							}
							//if(fabs(bmin[0]) > 10.0f)
							//	printf("bbox shift [%f %f %f] scale [%f %f %f]\n",bmin[0],bmin[1],bmin[2],bmax[0],bmax[1],bmax[2]);
							
							//special default texture transform for 3D textures posing as 2D textures

							//the order of applying transform elements seems reversed for texture transforms
							//H: related to order of operands in mat * vec in shader:
							//   fw_TexCoord[0] = vec3(fw_TextureMatrix0 *vec4(texcoord,1.0)); \n\
							// but sign on elements is what you expect
							//flip z from RHS to LHS in fragment shader plug_tex3d apply
							//printf("default tt\n");
							FW_GL_SCALE_F(bmax[0],bmax[1],bmax[2]);  
							FW_GL_TRANSLATE_F(-bmin[0],-bmin[1],-bmin[2]);
						}
					}
					if(me){
						if(tg->RenderFuncs.shapenode ) { //&& genTexPtr->TC_size < 3){
							//3D but no 3D coords supplied - gen from vertex in vertex shader
							glUniform1i(me->tex3dUseVertex,1); //vertex shader flag to over-ride texCoords with vertex
						}else{
							glUniform1i(me->tex3dUseVertex,0); 
						}
						if(tti){
							if(me->repeatSTR > -1)
								glUniform1iv(me->repeatSTR,3,tti->repeatSTR);
							if(me->magFilter > -1)
								glUniform1i(me->magFilter,tti->magFilter);
						}
					}
				}

				texture = tg->RenderFuncs.boundTextureStack[c];

				// SET_TEXTURE_UNIT_AND_BIND
				glActiveTexture(GL_TEXTURE0+c); 
				//printf("active texture %d texture %d c %d\n",GL_TEXTURE0+c,texture,c);
				if (getAppearanceProperties()->cubeFace==0) {
					glBindTexture(GL_TEXTURE_2D,texture); 
				} else {
					glBindTexture(GL_TEXTURE_CUBE_MAP,texture); 
				}
			}
		}
	}
	/* set up the selected shader for this texture(s) config */
	if (me != NULL) {
		tnode = tg->RenderFuncs.texturenode;
		//printf ("passedInGenTex, we have tts %d tc %d\n",tg->RenderFuncs.textureStackTop, me->textureCount);

		if (me->textureCount != -1) {
			glUniform1i(me->textureCount, tg->RenderFuncs.textureStackTop);
		}
		if(tg->RenderFuncs.textureStackTop){
			if(isMultiTexture(tg->RenderFuncs.texturenode)){
				struct X3D_MultiTexture * mtnode = (struct X3D_MultiTexture *)tg->RenderFuncs.texturenode;
				glUniform4f(me->multitextureColor,mtnode->color.c[0],mtnode->color.c[1],mtnode->color.c[2],mtnode->alpha);
			}
		}
		for (i=0; i<tg->RenderFuncs.textureStackTop; i++) {
			//static int once = 0;
			//if(once < 10) {
			//printf (" sending in i%d tu %d mode %d src %d fnc %d\n",i,i,
			//	p->textureParameterStack[i].multitex_mode,
			//	p->textureParameterStack[i].multitex_source,
			//	p->textureParameterStack[i].multitex_function);
			//	once++;
			//}
			glUniform1i(me->TextureUnit[i],i);
			//the 2i wasn't working for me even with ivec2 in shader
			glUniform2i(me->TextureMode[i],p->textureParameterStack[i].multitex_mode[0], p->textureParameterStack[i].multitex_mode[1]);
			glUniform2i(me->TextureSource[i],p->textureParameterStack[i].multitex_source[0], p->textureParameterStack[i].multitex_source[1]);
			//glUniform1i(me->TextureMode[i],p->textureParameterStack[i].multitex_mode[0]);
			//glUniform1i(me->TextureSource[i],p->textureParameterStack[i].multitex_source[0]);
			glUniform1i(me->TextureFunction[i],p->textureParameterStack[i].multitex_function);
		}
	#ifdef TEXVERBOSE
	} else {
		printf (" NOT sending in %d i+tu+mode because currentShaderProperties is NULL\n",tg->RenderFuncs.textureStackTop);
	#endif
	}

	FW_GL_MATRIX_MODE(GL_MODELVIEW);

	PRINT_GL_ERROR_IF_ANY("");
}
