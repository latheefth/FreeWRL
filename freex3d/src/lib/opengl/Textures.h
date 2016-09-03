/*


Screen snapshot.

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


#ifndef __FREEWRL_TEXTURES_H__
#define __FREEWRL_TEXTURES_H__

#define TEXTURE_INVALID 0

/* Texture loading table :
   newer Texture handling procedures
   each texture has this kind of structure
*/
struct textureTableIndexStruct {
	struct X3D_Node*	scenegraphNode;
	int    nodeType;
	int    status;
	int    hasAlpha;
	GLuint OpenGLTexture;
	int    frames;
	char   *filename;
    int    x;
    int    y;
	int    z;
    unsigned char *texdata;
    GLint  Src;
    GLint  Trc;
	int textureNumber;
	int channels; //number of original image file image channels/components 0=no texture default, 1=Intensity 2=IntensityAlpha 3=RGB 4=RGBA
};
typedef struct textureTableIndexStruct textureTableIndexStruct_s;

//extern textureTableIndexStruct_s* loadThisTexture;
//extern GLuint defaultBlankTexture;

/* Vertex Array to Vertex Buffer Object migration - used to have a passedInGenTex() 
   when we had (for instance) Cone textures - put this as part of the VBO. */

textureTableIndexStruct_s *getTableIndex(int indx);

struct textureVertexInfo {
    GLfloat *pre_canned_textureCoords;
	GLint TC_size; 		/* glTexCoordPointer - size param */
	GLenum TC_type;		/* glTexCoordPointer - type param */	
	GLsizei TC_stride;	/* glTexCoordPointer - stride param */
	GLvoid *TC_pointer;	/* glTexCoordPointer - pointer to first element */
	void *next; //next textureVertexInfo for MultitextureCoordinate
	GLint VBO;
};

/* for texIsloaded structure */
#define TEX_NOTLOADED       0
#define TEX_LOADING         1
#define TEX_READ            2
#define TEX_NEEDSBINDING    3
#define TEX_LOADED          4
#define TEX_UNSQUASHED      5
#define TEX_NOTFOUND		6

const char *texst(int num);


/* do we have to do textures?? */
#define HAVETODOTEXTURES (gglobal()->RenderFuncs.textureStackTop != 0)

extern void textureDraw_start(struct textureVertexInfo *tex);
extern void textureDraw_end(void);

struct X3D_Node *getThis_textureTransform();

extern int fwl_isTextureLoaded(int texno);
extern int isTextureAlpha(int n);
extern int display_status;


/* appearance does material depending on last texture depth */
#define NOTEXTURE 0
#define TEXTURE_NO_ALPHA 1
#define TEXTURE_ALPHA 2

void loadTextureNode (struct X3D_Node *node, struct multiTexParams *param);
void bind_image(int type, struct Uni_String *parenturl, struct Multi_String url,
				GLuint *texture_num,
				int repeatS,
				int repeatT,
				void  *param);

/* other function protos */
void init_multitexture_handling(void);


#endif /* __FREEWRL_TEXTURES_H__ */
