/*


X3D Shape Component

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
#include "../opengl/Frustum.h"
#include "../opengl/Material.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"
#include "Component_ProgrammableShaders.h"
#include "Component_Shape.h"
#include "RenderFuncs.h"

#define NOTHING 0

typedef struct pComponent_Shape{

	struct matpropstruct appearanceProperties;

	/* pointer for a TextureTransform type of node */
	struct X3D_Node *  this_textureTransform;  /* do we have some kind of textureTransform? */

	/* for doing shader material properties */
	struct X3D_TwoSidedMaterial *material_twoSided;
	struct X3D_Material *material_oneSided;

	/* Any user defined shaders here? */
	struct X3D_Node * userShaderNode;
    
}* ppComponent_Shape;

void *Component_Shape_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_Shape));
	memset(v,0,sizeof(struct pComponent_Shape));
	return v;
}
void Component_Shape_init(struct tComponent_Shape *t){
	//public
	//private
	t->prv = Component_Shape_constructor();
}

//getters
struct matpropstruct *getAppearanceProperties(){
	ppComponent_Shape p = (ppComponent_Shape)gglobal()->Component_Shape.prv;

	return &p->appearanceProperties;
}

// see if the Appearance node has a valid Shader node as a child.
static int hasUserDefinedShader(struct X3D_Node *node) {
#define NO_SHADER 99999
	unsigned int rv = NO_SHADER;

	if (node==NULL) return 0;

	//ConsoleMessage ("hasUserDeginedShader, node ptr %p",node);
	//ConsoleMessage ("hasUserDefinedShader, nodeType %s",stringNodeType(node->_nodeType));
	if (node->_nodeType == NODE_Appearance) {
		struct X3D_Appearance *ap;
		POSSIBLE_PROTO_EXPANSION(struct X3D_Appearance *, node, ap);
		//ConsoleMessage ("appearance node shaders is %d %p",ap->shaders.n, ap->shaders.p);

		if (ap->shaders.n != 0) {
			struct X3D_ComposedShader *cps;

			/* ok - what kind of node is here, and are there more than two? */
			if (ap->shaders.n > 1) {
				ConsoleMessage ("warning, Appearance->shaders has more than 1 node, using only first one");
			}
            
			POSSIBLE_PROTO_EXPANSION(struct X3D_ComposedShader *, ap->shaders.p[0], cps);

			//ConsoleMessage ("node type of shaders is :%s:",stringNodeType(cps->_nodeType));
			if (cps->_nodeType == NODE_ComposedShader) {
				// might be set in compile_Shader already
				//ConsoleMessage ("cps->_initialized %d",cps->_initialized);
				//ConsoleMessage ("cps->_retrievedURLData %d",cps->_retrievedURLData);

				if (cps->_retrievedURLData) {
					if (cps->_shaderUserNumber == -1) cps->_shaderUserNumber = getNextFreeUserDefinedShaderSlot();
					rv = cps->_shaderUserNumber;
				}
			} else if (cps->_nodeType == NODE_PackagedShader) {
				// might be set in compile_Shader already
				if (X3D_PACKAGEDSHADER(cps)->_retrievedURLData) {
					if (X3D_PACKAGEDSHADER(cps)->_shaderUserNumber == -1) 
						X3D_PACKAGEDSHADER(cps)->_shaderUserNumber = getNextFreeUserDefinedShaderSlot();
					rv = X3D_PACKAGEDSHADER(cps)->_shaderUserNumber;
				}
			} else if (cps->_nodeType == NODE_ProgramShader) {
				// might be set in compile_Shader already
				if (X3D_PROGRAMSHADER(cps)->_retrievedURLData) {
				if (X3D_PROGRAMSHADER(cps)->_shaderUserNumber == -1) 
					X3D_PROGRAMSHADER(cps)->_shaderUserNumber = getNextFreeUserDefinedShaderSlot();

				rv = X3D_PROGRAMSHADER(cps)->_shaderUserNumber;
				}
			} else {
				ConsoleMessage ("shader field of Appearance is a %s, ignoring",stringNodeType(cps->_nodeType));
			}

		} 
	} 

	//ConsoleMessage ("and ste is %d",ste);

	// ok - did we find an integer between 0 and some MAX_SUPPORTED_USER_SHADERS value?
	if (rv == NO_SHADER) rv = 0; 
	else rv = USER_DEFINED_SHADER_START * (rv+1);

	//ConsoleMessage ("rv is going to be %x",rv);

	//ConsoleMessage ("hasUserDefinedShader, returning %p",*shaderTableEntry);
	return rv;
}

// Save the shader node from the render_*Shader so that we can send in parameters when required
void setUserShaderNode(struct X3D_Node *me) {
	ppComponent_Shape p = (ppComponent_Shape)gglobal()->Component_Shape.prv;
	p->userShaderNode = me;
}

struct X3D_Node *getThis_textureTransform(){
	ppComponent_Shape p = (ppComponent_Shape)gglobal()->Component_Shape.prv;
	return p->this_textureTransform;
}

void child_Appearance (struct X3D_Appearance *node) {
	struct X3D_Node *tmpN;
	
	/* printf ("in Appearance, this %d, nodeType %d\n",node, node->_nodeType);
	   printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	   render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */
	
	/* Render the material node... */
	RENDER_MATERIAL_SUBNODES(node->material);
	
	if (node->fillProperties) {
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->fillProperties,tmpN);
		render_node(tmpN);
	}
	
	/* set line widths - if we have line a lineProperties node */
	if (node->lineProperties) {
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->lineProperties,tmpN);
		render_node(tmpN);
	}
	
	if(node->texture) {
		/* we have to do a glPush, then restore, later */
		/* glPushAttrib(GL_ENABLE_BIT); */
		
		ppComponent_Shape p = (ppComponent_Shape)gglobal()->Component_Shape.prv;    


		/* is there a TextureTransform? if no texture, fugutaboutit */
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->textureTransform,p->this_textureTransform);
		
		/* now, render the texture */
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->texture,tmpN);

		render_node(tmpN);
	}

	/* shaders here/supported?? */
	if (node->shaders.n !=0) {
		int count;
		int foundGoodShader = FALSE;
		
		for (count=0; count<node->shaders.n; count++) {
			POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->shaders.p[count], tmpN);
			
			/* have we found a valid shader yet? */
			if (foundGoodShader) {
				/* printf ("skipping shader %d of %d\n",count, node->shaders.n); */
				/* yes, just tell other shaders that they are not selected */
				SET_SHADER_SELECTED_FALSE(tmpN);
			} else {
				/* render this node; if it is valid, then we call this one the selected one */
				SET_FOUND_GOOD_SHADER(tmpN);
				DEBUG_SHADER("running shader (%s) %d of %d\n",
				stringNodeType(X3D_NODE(tmpN)->_nodeType),count, node->shaders.n);
				render_node(tmpN);
			}
		}
	}
}


void render_Material (struct X3D_Material *node) {
	COMPILE_IF_REQUIRED
	{
	ppComponent_Shape p = (ppComponent_Shape)gglobal()->Component_Shape.prv;

	/* record this node for OpenGL-ES and OpenGL-3.1 operation */
	p->material_oneSided = node;
	}
}


/* bounds check the material node fields */
void compile_Material (struct X3D_Material *node) {
	int i;
	float trans;

	/* verify that the numbers are within range */
	if (node->ambientIntensity < 0.0f) node->ambientIntensity=0.0f;
	if (node->ambientIntensity > 1.0f) node->ambientIntensity=1.0f;
	if (node->shininess < 0.0f) node->shininess=0.0f;
	if (node->shininess > 1.0f) node->shininess=1.0f;
	if (node->transparency < 0.0f) node->transparency=MIN_NODE_TRANSPARENCY;
	if (node->transparency >= 1.0f) node->transparency=MAX_NODE_TRANSPARENCY;

	for (i=0; i<3; i++) {
		if (node->diffuseColor.c[i] < 0.0f) node->diffuseColor.c[i]=0.0f;
		if (node->diffuseColor.c[i] > 1.0f) node->diffuseColor.c[i]=1.0f;
		if (node->emissiveColor.c[i] < 0.0f) node->emissiveColor.c[i]=0.0f;
		if (node->emissiveColor.c[i] > 1.0f) node->emissiveColor.c[i]=1.0f;
		if (node->specularColor.c[i] < 0.0f) node->specularColor.c[i]=0.0f;
		if (node->specularColor.c[i] > 1.0f) node->specularColor.c[i]=1.0f;
	}

	/* set the transparency here for the material */
	/* Remember, VRML/X3D transparency 0.0 = solid; OpenGL 1.0 = solid, so we reverse it... */
	trans = 1.0f - node->transparency;

	/* we now keep verified params in a structure that maps to Shaders well...
	struct gl_MaterialParameters {
		vec4 emission;
		vec4 ambient;
		vec4 diffuse;
		vec4 specular;
		float shininess;
	};
	which is stored in the _verifiedColor[17] array here.
	emission [0]..[3];
	ambient [4]..[7];
	diffuse [8]..[11];
	specular [12]..[15];
	shininess [16]
*/
	/* first, put in the transparency */
	node->_verifiedColor.p[3] = trans;
	node->_verifiedColor.p[7] = trans;
	node->_verifiedColor.p[11] = trans;
	node->_verifiedColor.p[15] = trans;

	/* DiffuseColor */
	memcpy((void *)(&node->_verifiedColor.p[8]), node->diffuseColor.c, sizeof (float) * 3);

	/* Ambient  - diffuseColor * ambientIntensity */
	for(i=0; i<3; i++) { node->_verifiedColor.p[i+4] = node->_verifiedColor.p[i+8] * node->ambientIntensity; }

	/* Specular */
	memcpy((void *)(&node->_verifiedColor.p[12]), node->specularColor.c, sizeof (float) * 3);

	/* Emissive */
	memcpy((void *)(&node->_verifiedColor.p[0]), node->emissiveColor.c, sizeof (float) * 3);

	/* Shininess */
	node->_verifiedColor.p[16] = node->shininess * 128.0f;

#define MAX_SHIN 128.0f
#define MIN_SHIN 0.01f
		if ((node->_verifiedColor.p[16] > MAX_SHIN) || (node->_verifiedColor.p[16] < MIN_SHIN)) {
			if (node->_verifiedColor.p[16]>MAX_SHIN){node->_verifiedColor.p[16] = MAX_SHIN;}else{node->_verifiedColor.p[16]=MIN_SHIN;}
		}
#undef MAX_SHIN
#undef MIN_SHIN

	MARK_NODE_COMPILED
}

#define CHECK_COLOUR_FIELD(aaa) \
	case NODE_##aaa: { \
		struct X3D_##aaa *me = (struct X3D_##aaa *)realNode; \
		if (me->color == NULL) return NOTHING; /* do not add any properties here */\
		else return COLOUR_MATERIAL_SHADER; \
		break; \
	} 

/* if this is a LineSet, PointSet, etc... */
// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#Lightingoff
// 'shapes that represent points or lines are unlit
static bool getIfLinePoints(struct X3D_Node *realNode) {
	if (realNode == NULL) return false;
	switch (realNode->_nodeType) {
		case NODE_IndexedLineSet:
		case NODE_LineSet:
		case NODE_PointSet:
		case NODE_Polyline2D:
		case NODE_Polypoint2D:
		case NODE_Circle2D:
		case NODE_Arc2D:
			return  true;
	}
	return false; // do not add any capabilites here.
}

/* is the texCoord field a TextureCoordinateGenerator or not? */
static int getShapeTextureCoordGen(struct X3D_Node *realNode) {
	struct X3D_Node *tc = NULL;
	int *fieldOffsetsPtr = NULL;

	//ConsoleMessage ("getShapeTextureCoordGen, node type %s\n",stringNodeType(realNode->_nodeType));
	if (realNode == NULL) return 0;

	fieldOffsetsPtr = (int *)NODE_OFFSETS[realNode->_nodeType];
	/*go thru all field*/
	while (*fieldOffsetsPtr != -1) {
		if (*fieldOffsetsPtr == FIELDNAMES_texCoord) {
			// get the pointer stored here...
			memcpy(&tc,offsetPointer_deref(void*, realNode,*(fieldOffsetsPtr+1)),sizeof(struct X3D_Node *));
			//ConsoleMessage ("tc is %p\n",tc);
			//ConsoleMessage ("tc is of type %s\n",stringNodeType(tc->_nodeType));
			if (tc != NULL) {
				if (tc->_nodeType == NODE_TextureCoordinateGenerator) return HAVE_TEXTURECOORDINATEGENERATOR;
			}
		}
		fieldOffsetsPtr += 5;
	}

	return 0;
}


/* Some shapes have Color nodes - if so, then we have other shaders */
static int getShapeColourShader (struct X3D_Node *myGeom) {
	struct X3D_Node *realNode;

	POSSIBLE_PROTO_EXPANSION(struct X3D_Node *,myGeom,realNode);

	if (realNode == NULL) return NOTHING;

	/* go through each node type that can have a Color node, and if it is not NULL
	   we know we have a Color node */

	switch (realNode->_nodeType) {
		CHECK_COLOUR_FIELD(IndexedFaceSet);
		CHECK_COLOUR_FIELD(IndexedLineSet);
		CHECK_COLOUR_FIELD(IndexedTriangleFanSet);
		CHECK_COLOUR_FIELD(IndexedTriangleSet);
		CHECK_COLOUR_FIELD(IndexedTriangleStripSet);
		CHECK_COLOUR_FIELD(LineSet);
		CHECK_COLOUR_FIELD(PointSet);
		CHECK_COLOUR_FIELD(TriangleFanSet);
		CHECK_COLOUR_FIELD(TriangleStripSet);
		CHECK_COLOUR_FIELD(TriangleSet);
		CHECK_COLOUR_FIELD(ElevationGrid);
		CHECK_COLOUR_FIELD(GeoElevationGrid);
		CHECK_COLOUR_FIELD(QuadSet);
		CHECK_COLOUR_FIELD(IndexedQuadSet);
	}

	/* if we are down here, we KNOW we do not have a color field */
	return NOTHING; /* do not add any capabilites here */
}

static int getAppearanceShader (struct X3D_Node *myApp) {
	struct X3D_Appearance *realAppearanceNode;
	struct X3D_Node *realMaterialNode;


	int retval = NOTHING;

	/* if there is no appearance node... */
	if (myApp == NULL) return retval;

	POSSIBLE_PROTO_EXPANSION(struct X3D_Appearance *, myApp,realAppearanceNode);
	if (realAppearanceNode->_nodeType != NODE_Appearance) return retval;
    
	if (realAppearanceNode->material != NULL) {
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, realAppearanceNode->material,realMaterialNode);
		        
		if (realMaterialNode->_nodeType == NODE_Material) {
			retval |= MATERIAL_APPEARANCE_SHADER;
		}
		if (realMaterialNode->_nodeType == NODE_TwoSidedMaterial) {
			retval |= TWO_MATERIAL_APPEARANCE_SHADER;
		}
	}


	if (realAppearanceNode->fillProperties != NULL) {
		struct X3D_Node *fp;
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, realAppearanceNode->fillProperties,fp);
		if (fp->_nodeType != NODE_FillProperties) {
			ConsoleMessage("getAppearanceShader, fillProperties has a node type of %s",stringNodeType(fp->_nodeType));
		} else {
			// is this a FillProperties node, but is it enabled?
			if (X3D_FILLPROPERTIES(fp)->_enabled)
				retval |= FILL_PROPERTIES_SHADER;
		}
	}


	if (realAppearanceNode->texture != NULL) {
		//printf ("getAppearanceShader - rap node is %s\n",stringNodeType(realAppearanceNode->texture->_nodeType));
		struct X3D_Node *tex;

		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, realAppearanceNode->texture,tex);
		if ((tex->_nodeType == NODE_ImageTexture) || 
			(tex->_nodeType == NODE_MovieTexture) || 
			(tex->_nodeType == NODE_PixelTexture)){
			retval |= ONE_TEX_APPEARANCE_SHADER;
		} else if (tex->_nodeType == NODE_MultiTexture) {
			retval |= MULTI_TEX_APPEARANCE_SHADER;
		} else if ((tex->_nodeType == NODE_ComposedCubeMapTexture) ||
					(tex->_nodeType == NODE_ImageCubeMapTexture) || 
					(tex->_nodeType == NODE_GeneratedCubeMapTexture)) {
			retval |= HAVE_CUBEMAP_TEXTURE;
		} else {
			ConsoleMessage ("getAppearanceShader, texture field %s not supported yet\n",
			stringNodeType(tex->_nodeType));
		}
	}
    
	#ifdef SHAPE_VERBOSE
	printf ("getAppearanceShader, returning %x\n",retval);
	#endif //SHAPE_VERBOSE

	return retval;
}


/* find info on the appearance of this Shape and create a shader */
/* 
	The possible sequence of a properly constructed appearance field is:

	Shape.appearance -> Appearance
	
	Appearance.fillProperties 	-> FillProperties
	Appearance.lineProperties 	-> LineProperties
	Appearance.material 		-> Material
					-> TwoSidedMaterial	
	Appearance.shaders		-> ComposedShader
	Appearance.shaders		-> PackagedShader
	Appearance.shaders		-> ProgramShader

	Appearance.texture		-> Texture
	Appearance.texture		-> MultiTexture
	Appearance.textureTransform	->

*/

/* now works with our pushing matricies (norm, proj, modelview) but not for complete shader appearance replacement */
void render_FillProperties (struct X3D_FillProperties *node) {
	GLfloat hatchX;
	GLfloat hatchY;
	GLint algor;
	GLint hatched;
	GLint filled;

	struct matpropstruct *me= getAppearanceProperties();

	hatchX = 0.80f; hatchY = 0.80f;
	algor = node->hatchStyle; filled = node->filled; hatched = node->hatched;
	switch (node->hatchStyle) {
		case 0: break; /* bricking - not standard X3D */
		case 1: hatchX = 1.0f; break; /* horizontal lines */
		case 2: hatchY = 1.0f; break; /* vertical lines */
		case 3: hatchY=1.0f; break; /* positive sloped lines */
		case 4: hatchY=1.0f; break; /* negative sloped lines */
		case 5: break; /* square pattern */
		case 6: hatchY = 1.0f; break; /* diamond pattern */

		default :{
			node->hatched = FALSE; /* woops - something wrong here disable */
		}
	}

	me->filledBool = filled;
	me->hatchedBool = hatched;
	me->hatchPercent[0] = hatchX;
	me->hatchPercent[1] = hatchY;
	me->hatchScale[0] = node->_hatchScale.c[0];
	me->hatchScale[1] = node->_hatchScale.c[1];
	me->algorithm = algor;
	me->hatchColour[0]=node->hatchColor.c[0]; me->hatchColour[1]=node->hatchColor.c[1]; me->hatchColour[2] = node->hatchColor.c[2];
	me->hatchColour[3] = 1.0;
}


void render_LineProperties (struct X3D_LineProperties *node) {
	#ifdef NEED_TO_ADD_TO_SHADER
	much of this was working in older versions of FreeWRL,
	before we went to 100% shader based code. Check FreeWRL
	from (say) 2011 to see what the shader code looked like

	GLushort pat;
	#endif

	if (node->applied) {
		//ppComponent_Shape p = (ppComponent_Shape)gglobal()->Component_Shape.prv;

		if (node->linewidthScaleFactor > 1.0) {
			struct matpropstruct *me;
			glLineWidth(node->linewidthScaleFactor);
			me= getAppearanceProperties();
			me->pointSize = node->linewidthScaleFactor;
		}


		#ifdef NEED_TO_ADD_TO_SHADER
		if (node->linetype > 1) {
			pat = 0xffff; /* can not support fancy line types - this is the default */
			switch (node->linetype) {
				case 2: pat = 0xff00; break; /* dashed */
				case 3: pat = 0x4040; break; /* dotted */
				case 4: pat = 0x04ff; break; /* dash dot */
				case 5: pat = 0x44fe; break; /* dash dot dot */
				case 6: pat = 0x0100; break; /* optional */
				case 7: pat = 0x0100; break; /* optional */
				case 10: pat = 0xaaaa; break; /* optional */
				case 11: pat = 0x0170; break; /* optional */
				case 12: pat = 0x0000; break; /* optional */
				case 13: pat = 0x0000; break; /* optional */
				default: {}
			}
		}
		#endif 
	}
}

textureTableIndexStruct_s *getTableTableFromTextureNode(struct X3D_Node *textureNode);
// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#Lightingmodel
// colorsources: Crgb, TxCrgb, Trgb, White, TTT, Drgb, TxDrgb
// alphasources: One, AT, MA
static enum {
	Crgb, TxCrgb, Trgb, White, TTT, Drgb, TxDrgb,
	One, AT, MA,
} lighting_enum;
static char* lighting_names [] = {"Crgb", "TxCrgb","Trgb","White","TTT","Drgb","TxDrgb","One","AT","MA"};
static int lookupColorAlpha[2][2][5][2] = {
	{	//unlit
		// unlit geometry, table 17-2
		// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#Lightingoff
		//table 17-2 UnLit Color and alpha mapping
		//Texturetype		Color per-vertex		Color Node NULL
		//					or per face		
		// No Texture	0	Crgb		1			(1,1,1)		1
		// Intensity	1	T x Crgb	1			(T,T,T)		1
		// Intens+Alph	2	T x Crgb	AT			(T,T,T)		AT
		// RGB			3	Trgb		1			Trgb		1
		// RGBA			4	Trgb		AT			Trgb		AT
		//					OutColor	OutAlpha	OutColor	OutAlpha
		//              ^number of texture channels/compoents 
		// Crgb - from color node, T-from texture, White = (1,1,1), One = 1.0
		// MA- Material Alpha (1-Transparency), AT- texture alpha
		{ //colorNode
			{Crgb,  One},
			{TxCrgb,One},
			{TxCrgb,AT},
			{Trgb,  One},
			{Trgb,  AT},
		},
		{ //no colorNode
			{White,One},
			{TTT  ,One},
			{TTT,  AT},
			{Trgb, One},
			{Trgb, AT},
		},
	},
	{  //lit
		// Lit geometry, table 17-3
		// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#t-Litcolourandalpha
		//table 17-3 Lit Color and alpha mapping
		//Texturetype		Color per-vertex		Color Node NULL
		//					or per face		
		// No Texture	0	Crgb		MA			Drgb		MA
		// Intensity	1	T x Crgb	MA			T x Drgb	MA
		// Intens+Alph	2	T x Crgb	AT			T x Drgb	AT
		// RGB			3	Trgb		MA			Trgb		MA
		// RGBA			4	Trgb		AT			Trgb		AT
		//					OutDiffuse	OutAlpha	OutDiffuse	OutAlpha
		//              ^number of texture channels/compoents 
		// Crgb - from color node, IT-from texture, Drgb - material diffuse
		// MA- Material Alpha (1-Transparency), AT- texture alpha
		{ //colorNode
			{Crgb,  MA},
			{TxCrgb,MA},
			{TxCrgb,AT},
			{Trgb,  MA},
			{Trgb,  AT},
		},
		{ //no colorNode
			{Drgb,  MA},
			{TxDrgb,MA},
			{TxDrgb,AT},
			{Trgb,  MA},
			{Trgb,  AT},
		},
	},
};

int color_alpha_source(struct X3D_Node *appearanceNode, struct X3D_Node *geometry, int *colorSource, int *alphaSource, int *imgchannels){

	// returns isLit 1/0, *colorSource, *alphaSource see table above
	// unlit = apearance == NULL || appearance.material == NULL || points || lines
	//appearance = (struct X3D_Appearance *)tmpN;
	// channels = appearance && appearance.texture? appearance.texture.channels : 0
	int channels, imgalpha, isLit, isUnlitGeometry, hasColorNode, whichShapeColorShader;
	int colorSourceA, colorSourceB, colorSourceC, alphaSourceA, alphaSourceB, haveTexture;
	struct X3D_Appearance *appearance;
	channels = 0;
	imgalpha = 0;
	isLit = 0;
	haveTexture = 0;

	*colorSource = 0;
	*alphaSource = 0;
	*imgchannels = 0; //no texture

	if(!geometry) return 0;
	appearance = (struct X3D_Appearance*)appearanceNode;
	isUnlitGeometry = getIfLinePoints(geometry);
	whichShapeColorShader = getShapeColourShader(geometry);
	// colorNode = geometry && geometry.color ? TRUE : FALSE
	hasColorNode = whichShapeColorShader == NOTHING ? 0 : 1;

	if(appearance){
		isLit = appearance->material? 1 : 0;
		isLit = isUnlitGeometry ? 0 : isLit;
		//do I need possible proto expansione of texture, or will compile_appearance have done that?
		if(appearance->texture){
			//mutli-texture? should loop over multitexture->texture nodes? 
			// --to get to get max channels or hasAlpha, or need channels for each one?
			// H0: if nay of the multitextures has an alpha, then its alpha replaces material alpha
			// H1: multitexture alpha is only for composing textures, assumed to take material alpha 
			//single texture:
			textureTableIndexStruct_s *tti = getTableTableFromTextureNode(appearance->texture);
			haveTexture = 1;
			if(tti){
				//new Aug 6, 2016, check LoadTextures.c for your platform channel counting
				//NoImage=0, Luminance=1, LuminanceAlpha=2, RGB=3, RGBA=4
				//PROBLEM: if tti isn't loaded -with #channels, alpha set-, we don't want to compile child
				channels = tti->channels; 
				imgalpha = tti->hasAlpha;
				if(tti->status < TEX_NEEDSBINDING) 
					printf("."); //should Unmark node compiled
			}
		}
	}

	// colorsource = lookupColorAlpha[lit/unlit 0/1][colorNode? 0/1][channels 0-4][0=color]
	// alphasource = lookupColorAlpha[lit/unlit 0/1][colorNode? 0/1][channels 0-4][1=alpha]
	colorSourceA = lookupColorAlpha[isLit][1-hasColorNode][channels][0];
	alphaSourceA = lookupColorAlpha[isLit][1-hasColorNode][channels][1];
	//should be the same as (somewhat less readable):
	alphaSourceB = imgalpha ? AT : isLit ? MA : One;
	colorSourceB = hasColorNode ? channels > 2 ? Trgb : channels ? TxCrgb : Crgb :
		channels > 2 ? Trgb : channels ? !isLit ?  TTT : TxDrgb : !isLit? White : Drgb;
	colorSourceC;
	if(hasColorNode){
		if(channels > 2) colorSourceC = Trgb;
		else if(channels) colorSourceC = TxCrgb;
		else colorSourceC = Crgb;

	}else{
		if(channels > 2) colorSourceC = Trgb;
		else if(channels){
			if(!isLit) colorSourceC = TTT;
			else colorSourceC = TxDrgb;
		}else{
			if(!isLit) colorSourceC = White;
			else colorSourceC = Drgb;
		}
	}
	if(!(colorSourceC == colorSourceB && colorSourceB == colorSourceA)) 
		printf("ouch - colorsource confusion\n");
	if(alphaSourceA != alphaSourceB)
		printf("ouch - alphasource confusione\n");
	*colorSource = colorSourceA;
	*alphaSource = alphaSourceA;
	*imgchannels = haveTexture ? channels : -1;
	return isLit;
}
unsigned int getShaderFlags();
struct X3D_Node *getFogParams();
void child_Shape (struct X3D_Shape *node) {
	struct X3D_Node *tmpNG;  
	int channels;
	ppComponent_Shape p;
    	ttglobal tg = gglobal();
	struct fw_MaterialParameters defaultMaterials = {
				{0.0f, 0.0f, 0.0f, 1.0f}, /* Emission */
				{0.0f, 0.0f, 0.0f, 1.0f}, /* Ambient */
				{0.8f, 0.8f, 0.8f, 1.0f}, /* Diffuse */
				{0.0f, 0.0f, 0.0f, 1.0f}, /* Specular */
				10.0f};                   /* Shininess */

	COMPILE_IF_REQUIRED

	/* JAS - if not collision, and render_geom is not set, no need to go further */
	/* printf ("child_Shape vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	 render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

	if(!(node->geometry)) { return; }

	RECORD_DISTANCE

	if((renderstate()->render_collision) || (renderstate()->render_sensitive)) {
		/* only need to forward the call to the child */
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *,node->geometry,tmpNG);
		render_node(tmpNG);
		return;
	}

	p = (ppComponent_Shape)tg->Component_Shape.prv;

	/* initialization. This will get overwritten if there is a texture in an Appearance
	   node in this shape (see child_Appearance) */
	gglobal()->RenderFuncs.last_texture_type = NOTEXTURE;
	
	/* copy the material stuff in preparation for copying all to the shader */
	memcpy (&p->appearanceProperties.fw_FrontMaterial, &defaultMaterials, sizeof (struct fw_MaterialParameters));
	memcpy (&p->appearanceProperties.fw_BackMaterial, &defaultMaterials, sizeof (struct fw_MaterialParameters));

	/* now, are we rendering blended nodes or normal nodes?*/
	if (renderstate()->render_blend == (node->_renderFlags & VF_Blend)) {
		int colorSource, alphaSource, isLit;  
		unsigned int shader_requirements;

		RENDER_MATERIAL_SUBNODES(node->appearance);



		if (p->material_oneSided != NULL) {
			memcpy (&p->appearanceProperties.fw_FrontMaterial, p->material_oneSided->_verifiedColor.p, sizeof (struct fw_MaterialParameters));
			memcpy (&p->appearanceProperties.fw_BackMaterial, p->material_oneSided->_verifiedColor.p, sizeof (struct fw_MaterialParameters));
			/* copy the emissive colour over for lines and points */
			memcpy(p->appearanceProperties.emissionColour,p->material_oneSided->_verifiedColor.p, 3*sizeof(float));

		} else if (p->material_twoSided != NULL) {
			memcpy (&p->appearanceProperties.fw_FrontMaterial, p->material_twoSided->_verifiedFrontColor.p, sizeof (struct fw_MaterialParameters));
			memcpy (&p->appearanceProperties.fw_BackMaterial, p->material_twoSided->_verifiedBackColor.p, sizeof (struct fw_MaterialParameters));
			/* copy the emissive colour over for lines and points */
			memcpy(p->appearanceProperties.emissionColour,p->material_twoSided->_verifiedFrontColor.p, 3*sizeof(float));
		} else {
			/* no materials selected.... */
		}

		/* enable the shader for this shape */
		//ConsoleMessage("turning shader on %x",node->_shaderTableEntry);



		if (p->userShaderNode != NULL) {
			//ConsoleMessage ("have a shader of type %s",stringNodeType(p->userShaderNode->_nodeType));
			switch (p->userShaderNode->_nodeType) {
				case NODE_ComposedShader:
					if (X3D_COMPOSEDSHADER(p->userShaderNode)->isValid) {
						if (!X3D_COMPOSEDSHADER(p->userShaderNode)->_initialized) {
							sendInitialFieldsToShader(p->userShaderNode);
						}
					}
					break;
				case NODE_ProgramShader:
					if (X3D_PROGRAMSHADER(p->userShaderNode)->isValid) {
						if (!X3D_PROGRAMSHADER(p->userShaderNode)->_initialized) {
							sendInitialFieldsToShader(p->userShaderNode);
						}
					}

					break;
				case NODE_PackagedShader:
					if (X3D_PACKAGEDSHADER(p->userShaderNode)->isValid) {
						if (!X3D_PACKAGEDSHADER(p->userShaderNode)->_initialized) {
							sendInitialFieldsToShader(p->userShaderNode);
						}
					}

					break;
			}
		}

		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->geometry,tmpNG);

		//if(node->_shaderTableEntry == builtinShader ){

		channels = 0;
		if(1){
			//Aug 6, 2016, dug9: OK here now we have the parameters for where builtin shader
			//should get some things.
			isLit = color_alpha_source(node->appearance,tmpNG,&colorSource,&alphaSource,&channels);
			//printf("isLit = %d chnls %d %s %s\n",isLit,channels,lighting_names[colorSource],lighting_names[alphaSource]);
			//if shader is builtin
		}
		//_shaderTableEntry has bit flags for things like:
		// isLit, haveMaterial, one/twoMats, haveTexture, MultiTexture, TextureAlpha, hatching, LinePoints
		shader_requirements = node->_shaderTableEntry;  
		
		//for Luminance and Luminance-Alpha images, we have to tinker a bit in the Vertex shader
		shader_requirements |= channels == 1 || channels == 2 ? WANT_LUMINANCE : 0;

		//getShaderFlags() are from non-leaf-node shader influencers: 
		//   fog, local_lights, clipplane, Effect/EffectPart (for CastlePlugs) ...
		// - as such they may be different for the same shape node DEF/USEd in different branches of the scenegraph
		// - so they are ORd here before selecting a shader permutation
		shader_requirements |= getShaderFlags(); 
		//if(shader_requirements & FOG_APPEARANCE_SHADER)
		//	printf("halleluja - fog in child_shape\n");
		enableGlobalShader (getMyShader(shader_requirements)); //node->_shaderTableEntry));

		//see if we have to set up a TextureCoordinateGenerator type here
		if (node->geometry->_intern) {
			if (node->geometry->_intern->tcoordtype == NODE_TextureCoordinateGenerator) {
				getAppearanceProperties()->texCoordGeneratorType = node->geometry->_intern->texgentype;
				//ConsoleMessage("shape, matprop val %d, geom val %d",getAppearanceProperties()->texCoordGeneratorType, node->geometry->_intern->texgentype);
			}
		}

		#ifdef SHAPEOCCLUSION
		beginOcclusionQuery((struct X3D_VisibilitySensor*)node,renderstate()->render_geom); //BEGINOCCLUSIONQUERY;
		#endif
		//call stack to get from child_shape to sendMaterialsToShader and sendLightInfo as of Aug 13, 2016
		//child_shape
		//- render_node
		//-- render_indexedfaceset (or other specific shape)
		//--- render_polyrep
		//---- sendArraysToGPU
		//----- setupShader
		//------ sendMaterialsToShader
		//          Uniforms being sent for materials and hatching
		//--------- sendLightInfo
		//           Uniforms sent for lights

		render_node(tmpNG);

		#ifdef SHAPEOCCLUSION
		endOcclusionQuery((struct X3D_VisibilitySensor*)node,renderstate()->render_geom); //ENDOCCLUSIONQUERY;
		#endif

	}

	/* any shader turned on? if so, turn it off */
	//ConsoleMessage("turning shader off");
	finishedWithGlobalShader();
	p->material_twoSided = NULL;
	p->material_oneSided = NULL;
	p->userShaderNode = NULL;
    
	/* load the identity matrix for textures. This is necessary, as some nodes have TextureTransforms
		and some don't. So, if we have a TextureTransform, loadIdentity */
    
	if (p->this_textureTransform) {
		p->this_textureTransform = NULL;
		FW_GL_MATRIX_MODE(GL_TEXTURE);
		FW_GL_LOAD_IDENTITY();
		FW_GL_MATRIX_MODE(GL_MODELVIEW);
	}
    
	/* LineSet, PointSets, set the width back to the original. */
	{
		float gl_linewidth = tg->Mainloop.gl_linewidth;
		glLineWidth(gl_linewidth);
		p->appearanceProperties.pointSize = gl_linewidth;
	}

	/* did the lack of an Appearance or Material node turn lighting off? */
	LIGHTING_ON;

	/* turn off face culling */
	DISABLE_CULL_FACE;
}

void compile_Shape (struct X3D_Shape *node) {
	int whichAppearanceShader = 0;
	int whichShapeColorShader = 0;
	bool isUnlitGeometry = false;
	int hasTextureCoordinateGenerator = 0;
	int whichUnlitGeometry = 0;
	struct X3D_Node *tmpN = NULL;
	struct X3D_Node *tmpG = NULL;
	struct X3D_Appearance *appearance = NULL;
	int userDefinedShader = 0;
	int colorSource, alphaSource, channels, isLit;


	// ConsoleMessage ("**** Compile Shape ****");


	POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->geometry,tmpG);
	whichShapeColorShader = getShapeColourShader(tmpG);

	isUnlitGeometry = getIfLinePoints(tmpG);
	hasTextureCoordinateGenerator = getShapeTextureCoordGen(tmpG);

	POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->appearance,tmpN);


	/* first - does appearance have a shader node? */
	userDefinedShader = hasUserDefinedShader(tmpN);
	// if(!Appearance.shader) use lightingModel

	if(1){
		//Aug 6, 2016, dug9: OK here now we have some parameters for builtin shader
		//int colorSource, alphaSource, channels, isLit;
		isLit = color_alpha_source(tmpN,tmpG,&colorSource,&alphaSource,&channels);
		if(0) printf("isLit = %d chnls %d %s %s\n",isLit,channels,lighting_names[colorSource],lighting_names[alphaSource]);
		//PROBLEM: the texture may not loaded or bound yet, so the channels (number of image components ie RGBA=4) will be 0
		//colorSource and alphaSource depend on channels count
		//therefore values coming out here may not be the final values
		//SOLUTION: Textures.c > new_bind_image > about line 1530 in case TEX_NEEDSBINDING > we ichange++ the Shape

	}

	/* Lines, points - can get the emission colour from an appearance node */
	if (isUnlitGeometry) {

		int myAppShad =  getAppearanceShader(tmpN);

		/* Ok. We if whichShapeColorShader is non-zero, we have a Colour node.
		if it is zero, we either have an appearance node, or we have no-material. */
		//ConsoleMessage ("unlit geometry, appearance shader is %d whichShapeColorShader is %d",myAppShad,whichShapeColorShader);
		if ((whichShapeColorShader != 0) || (myAppShad != 0)) {
			//ConsoleMessage ("Unlit geometry, but with either a Color node, or an appearance node");

			/* which one first? */
			if (whichShapeColorShader != 0) whichUnlitGeometry = HAVE_LINEPOINTS_COLOR;
			else whichUnlitGeometry = HAVE_LINEPOINTS_APPEARANCE;
		}
	} else {

		if(0){
			/* if we have a Colour field, put this first */
			//old way - KelpForest > SharkLefty renders with CPV instead of IMG
			if (whichShapeColorShader != COLOUR_MATERIAL_SHADER) {
					whichAppearanceShader = getAppearanceShader(tmpN);
			}
		}else if(1){
			//for late arriving channels > recompile strategy (versus if-else conditional in compound shader)
			//SharkLefty renders correctly
			if(colorSource != Crgb){
				whichAppearanceShader = getAppearanceShader(tmpN);
				whichShapeColorShader = NOTHING;
			}
		}
		//else{
		//	//these more ellaborate use permutations dont' work.
		//	if(colorSource == Trgb){
		//		whichAppearanceShader = getAppearanceShader(tmpN);
		//		whichShapeColorShader = NOTHING;
		//	}
		//	if(colorSource == Crgb || colorSource == Drgb){
		//		whichAppearanceShader = NOTHING;
		//		whichShapeColorShader = whichShapeColorShader;
		//	}
		//	if(colorSource == TxCrgb || colorSource == TxDrgb){
		//		whichAppearanceShader = getAppearanceShader(tmpN);
		//		whichShapeColorShader = whichShapeColorShader;
		//	}

		//}
	}


		/* in case we had no appearance, etc, we do the bland NO_APPEARANCE_SHADER */
	node->_shaderTableEntry= (whichShapeColorShader | whichAppearanceShader | 
				hasTextureCoordinateGenerator | whichUnlitGeometry | userDefinedShader);




	if (node->_shaderTableEntry == NOTHING) 
		node->_shaderTableEntry = NO_APPEARANCE_SHADER;

	//printf ("compile_Shape, node->_shaderTableEntry is %x\n",node->_shaderTableEntry);

	MARK_NODE_COMPILED
}


void compile_TwoSidedMaterial (struct X3D_TwoSidedMaterial *node) {
	int i;
	float trans;

	/* verify that the numbers are within range */
	if (node->ambientIntensity < 0.0) node->ambientIntensity=0.0f;
	if (node->ambientIntensity > 1.0) node->ambientIntensity=1.0f;
	if (node->shininess < 0.0) node->shininess=0.0f;
	if (node->shininess > 1.0) node->shininess=1.0f;
	if (node->transparency < 0.0) node->transparency=MIN_NODE_TRANSPARENCY;
	if (node->transparency >= 1.0) node->transparency=MAX_NODE_TRANSPARENCY;

	if (node->backAmbientIntensity < 0.0) node->backAmbientIntensity=0.0f;
	if (node->backAmbientIntensity > 1.0) node->backAmbientIntensity=1.0f;
	if (node->backShininess < 0.0) node->backShininess=0.0f;
	if (node->backShininess > 1.0) node->backShininess=1.0f;
	if (node->backTransparency < 0.0) node->backTransparency=0.0f;
	if (node->backTransparency > 1.0) node->backTransparency=1.0f;

	for (i=0; i<3; i++) {
		if (node->diffuseColor.c[i] < 0.0) node->diffuseColor.c[i]=0.0f;
		if (node->diffuseColor.c[i] > 1.0) node->diffuseColor.c[i]=1.0f;
		if (node->emissiveColor.c[i] < 0.0) node->emissiveColor.c[i]=0.0f;
		if (node->emissiveColor.c[i] > 1.0) node->emissiveColor.c[i]=1.0f;
		if (node->specularColor.c[i] < 0.0) node->specularColor.c[i]=0.0f;
		if (node->specularColor.c[i] > 1.0) node->specularColor.c[i]=1.0f;

		if (node->backDiffuseColor.c[i] < 0.0) node->backDiffuseColor.c[i]=0.0f;
		if (node->backDiffuseColor.c[i] > 1.0) node->backDiffuseColor.c[i]=1.0f;
		if (node->backEmissiveColor.c[i] < 0.0) node->backEmissiveColor.c[i]=0.0f;
		if (node->backEmissiveColor.c[i] > 1.0) node->backEmissiveColor.c[i]=1.0f;
		if (node->backSpecularColor.c[i] < 0.0) node->backSpecularColor.c[i]=0.0f;
		if (node->backSpecularColor.c[i] > 1.0) node->backSpecularColor.c[i]=1.0f;
	}

	/* first, put in the transparency */
	trans = 1.0f - node->transparency;
	node->_verifiedFrontColor.p[3] = trans;
	node->_verifiedFrontColor.p[7] = trans;
	node->_verifiedFrontColor.p[11] = trans;
	node->_verifiedFrontColor.p[15] = trans;
	trans = 1.0f - node->backTransparency;
	node->_verifiedBackColor.p[3] = trans;
	node->_verifiedBackColor.p[7] = trans;
	node->_verifiedBackColor.p[11] = trans;
	node->_verifiedBackColor.p[15] = trans;


	/* DiffuseColor */
	memcpy((void *)(&node->_verifiedFrontColor.p[8]), node->diffuseColor.c, sizeof (float) * 3);

	/* Ambient  - diffuseFrontColor * ambientIntensity */
	for(i=0; i<4; i++) { node->_verifiedFrontColor.p[i+4] = node->_verifiedFrontColor.p[i+8] * node->ambientIntensity; }

	/* Specular */
	memcpy((void *)(&node->_verifiedFrontColor.p[12]), node->specularColor.c, sizeof (float) * 3);

	/* Emissive */
	memcpy((void *)(&node->_verifiedFrontColor.p[0]), node->emissiveColor.c, sizeof (float) * 3);

	/* Shininess */
	node->_verifiedFrontColor.p[16] = node->shininess * 128.0f;

#define MAX_SHIN 128.0f
#define MIN_SHIN 0.01f
	if ((node->_verifiedFrontColor.p[16] > MAX_SHIN) || (node->_verifiedFrontColor.p[16] < MIN_SHIN)) {
		if (node->_verifiedFrontColor.p[16]>MAX_SHIN){node->_verifiedFrontColor.p[16] = MAX_SHIN;}else{node->_verifiedFrontColor.p[16]=MIN_SHIN;}
	}
#undef MAX_SHIN
#undef MIN_SHIN

	if (node->separateBackColor) {

		/* DiffuseColor */
		memcpy((void *)(&node->_verifiedBackColor.p[8]), node->backDiffuseColor.c, sizeof (float) * 3);
	
		/* Ambient  - diffuseBackColor * ambientIntensity */
		for(i=0; i<3; i++) { node->_verifiedBackColor.p[i+4] = node->_verifiedBackColor.p[i+8] * node->ambientIntensity; }
	
		/* Specular */
		memcpy((void *)(&node->_verifiedBackColor.p[12]), node->backSpecularColor.c, sizeof (float) * 3);
	
		/* Emissive */
		memcpy((void *)(&node->_verifiedBackColor.p[0]), node->backEmissiveColor.c, sizeof (float) * 3);
	
		/* Shininess */
		node->_verifiedBackColor.p[16] = node->shininess * 128.0f;
	
#define MAX_SHIN 128.0f
#define MIN_SHIN 0.01f
		if ((node->_verifiedBackColor.p[16] > MAX_SHIN) || (node->_verifiedBackColor.p[16] < MIN_SHIN)) {
			if (node->_verifiedBackColor.p[16]>MAX_SHIN){node->_verifiedBackColor.p[16] = MAX_SHIN;}else{node->_verifiedBackColor.p[16]=MIN_SHIN;}
		}
#undef MAX_SHIN
#undef MIN_SHIN

	} else {
		/* just copy the front materials to the back */
		memcpy(node->_verifiedBackColor.p, node->_verifiedFrontColor.p, sizeof (float) * 17);
	}


	MARK_NODE_COMPILED
}

void render_TwoSidedMaterial (struct X3D_TwoSidedMaterial *node) {
	
	COMPILE_IF_REQUIRED
	{
	ppComponent_Shape p = (ppComponent_Shape)gglobal()->Component_Shape.prv;

	/* record this node for OpenGL-ES and OpenGL-3.1 operation */
	p->material_twoSided = node;
	}
}

