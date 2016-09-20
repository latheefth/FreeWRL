/*


Proximity sensor macro.

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



#ifndef __FREEWRL_SCENEGRAPH_SHAPE_H__
#define __FREEWRL_SCENEGRAPH_SHAPE_H__


/*******************************************************/


// Bit-wise operations here - these can be OR'd together to
// create the specific shader we want.
//
// DO NOT MESS UP THE BITS! (look at these in binary for 
// proper or-ing of the values)

#define NO_APPEARANCE_SHADER 0x0001
#define MATERIAL_APPEARANCE_SHADER 0x0002
#define TWO_MATERIAL_APPEARANCE_SHADER 0x0004
#define ONE_TEX_APPEARANCE_SHADER 0x0008
#define MULTI_TEX_APPEARANCE_SHADER 0x0010

/* PolyRep (etc) color field present */
#define COLOUR_MATERIAL_SHADER 0x00020

/*  - fillProperties present */
#define FILL_PROPERTIES_SHADER 0x00040

/*  - lines, points */
#define HAVE_LINEPOINTS_COLOR 0x0080
#define HAVE_LINEPOINTS_APPEARANCE 0x00100

/* TextureCoordinateGenerator */
#define HAVE_TEXTURECOORDINATEGENERATOR 0x00200

/* CubeMapTexturing */
#define HAVE_CUBEMAP_TEXTURE  0x00400
#define FOG_APPEARANCE_SHADER 0X00800
#define HAVE_FOG_COORDS       0x01000
#define TEXTURE_REPLACE_PRIOR 0x02000
#define TEXALPHA_REPLACE_PRIOR 0x04000
#define CPV_REPLACE_PRIOR     0x08000
#define SHADINGSTYLE_FLAT    0x100000
#define SHADINGSTYLE_GOURAUD 0x200000
#define SHADINGSTYLE_PHONG   0x400000
#define SHADINGSTYLE_WIRE    0x800000
#define MAT_FIRST            0x1000000
#define WANT_ANAGLYPH        0x2000000
#define TEX3D_SHADER         0X4000000
#define TEX3D_LAYER_SHADER   0x8000000
#define CLIPPLANE_SHADER     0x10000000

/* Component_Shader - user-specified shaders. Currently limited in number */
/* note we start at 0x1000 and count up by 1 for (currently) 255 shaders per program */
//Sept 20, 2016 I've only got 2 bits left in whichOne for user shaders -a single user shader will still work
// but will need to split/double/clone/expand whichOne to give more room for castle Effects and more user shaders
#define USER_DEFINED_SHADER_START	0x40000000 //0x001000   4 0100   
#define USER_DEFINED_SHADER_MASK    0xC0000000 //0x0FF000   C 1100


/*******************************************************/


struct fw_MaterialParameters {
	float emission[4];
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float shininess; 
};

struct matpropstruct {
	/* material properties for current shape */
	struct fw_MaterialParameters fw_FrontMaterial;
	struct fw_MaterialParameters fw_BackMaterial;

	/* which shader is active; 0 = no shader active */
	s_shader_capabilities_t *currentShaderProperties;

	float	transparency;
	GLfloat	emissionColour[3];
	GLint	cubeFace;	/* for cubemapping, if 0, not cube mapping */
	int 	cullFace;	/* is this single-sided or two-sided? Simply used to reduce calls to
						GL_ENABLE(GL_CULL_FACE), etc */

	/* for FillProperties, and LineProperties, line type (NOT pointsize) */
	int algorithm;
	bool hatchedBool;
	bool filledBool;
	GLfloat hatchPercent[2];
	GLfloat hatchScale[2];
	GLfloat hatchColour[4];

	// points now specified in shader, not via an opengl call 
	GLfloat pointSize;   

	//TextureCoordinateGenerator value - a "TCGT_XXX" type
	int texCoordGeneratorType;
};

struct matpropstruct* getAppearanceProperties();
void setUserShaderNode(struct X3D_Node *me);

#define MIN_NODE_TRANSPARENCY 0.0f
#define MAX_NODE_TRANSPARENCY 0.99f  /* if 1.0, then occlusion culling will cause flashing */

#define RENDER_MATERIAL_SUBNODES(which) \
	{ struct X3D_Node *tmpN;   \
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, which,tmpN) \
		if(tmpN) { \
			render_node(tmpN); \
		} \
	}


#define SET_SHADER_SELECTED_FALSE(x3dNode) \
	switch (X3D_NODE(x3dNode)->_nodeType) { \
		case NODE_ComposedShader: \
			X3D_COMPOSEDSHADER(x3dNode)->isSelected = FALSE; \
			break; \
		case NODE_ProgramShader: \
			X3D_PROGRAMSHADER(x3dNode)->isSelected = FALSE; \
			break; \
		case NODE_PackagedShader: \
			X3D_PROGRAMSHADER(x3dNode)->isSelected = FALSE; \
			break; \
		default: { \
			/* this is NOT a shader; should we say something, or just \
			   ignore? Lets ignore, for now */ \
		} \
	}

#define SET_FOUND_GOOD_SHADER(x3dNode) \
	switch (X3D_NODE(x3dNode)->_nodeType) { \
		case NODE_ComposedShader: \
			foundGoodShader = X3D_COMPOSEDSHADER(x3dNode)->isValid; \
			X3D_COMPOSEDSHADER(x3dNode)->isSelected = foundGoodShader; \
			break; \
		case NODE_ProgramShader: \
			foundGoodShader = X3D_PROGRAMSHADER(x3dNode)->isValid; \
			X3D_PROGRAMSHADER(x3dNode)->isSelected = foundGoodShader; \
			break; \
		case NODE_PackagedShader: \
			foundGoodShader = X3D_PROGRAMSHADER(x3dNode)->isValid; \
			X3D_PACKAGEDSHADER(x3dNode)->isSelected = foundGoodShader; \
			break; \
		default: { \
			/* this is NOT a shader; should we say something, or just \
			   ignore? Lets ignore, for now */ \
		} \
	}
#endif /* __FREEWRL_SCENEGRAPH_SHAPE_H__ */
