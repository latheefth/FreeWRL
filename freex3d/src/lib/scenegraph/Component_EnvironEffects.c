
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


/*******************************************************************

	X3D Environmental Effects Component

*********************************************************************/

/*
FOG - state as of July 2016: 
	- our conformance page says local_fog and fog_coordinates are unimplemented
	- Bindable.c
		- render_fog
		- #ifndef GL_ES_VERSION_2_0 // this should be handled in material shader
	Unclear if global Fog is working 

	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#LightingModel
	- Use of fog and fog coords in lighting eqn.
	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/enveffects.html#FogSemantics
	- meaning of fog nodes
	- background nodes are not affected by fog

	VRML2 fog examples:
		http://www.web3d.org/x3d-resources/content/examples/Vrml2.0Sourcebook/Chapter23-Fog/
		http://www.web3d.org/x3d/content/examples/Vrml2.0Sourcebook/Chapter23-Fog/Figure23.3cExponentialFogVisibility20.x3d
		- Octaga shows light fog increasing exponentially with distance (but shows ground white)
		- InstantReality shows heavy fog increasing exponentially with distance (then bombs on exit)
		- Vivaty - medium " works well
		- freewrl desktop win32 - no fog effect
	X3D fog in kelp forest example:
		http://x3dgraphics.com/examples/X3dForWebAuthors/Chapter11-LightingEnvironmentalEffects/
		http://x3dgraphics.com/examples/X3dForWebAuthors/Chapter11-LightingEnvironmentalEffects/Fog-KelpForestMain.x3d
		- freewrl Aug 8, 2016
			x no fog
		- fog (and scene) looks great in Octaga, InstantReality, Vivaty (although Octaga crashes if left running 8min)

	Non-X3D links:
		http://www.mbsoftworks.sk/index.php?page=tutorials&series=1&tutorial=15
		- fog shader with linear and exponential
		http://www.geeks3d.com/20100228/fog-in-glsl-webgl/
		- frag shader, webgl
	Fog Scale:
		The fog distance is in the coordinate system of the Fog Node
		- that means scale, relative to current shape/leaf/renderable node
		When visiting Fog or LocalFog node in scenegraph:
		1. transform a unit vector from Fog node to viewpoint/eye: vec3 = ModelViewMatrix x [0 0 1]
			fogScale = 1.0/length(vec3) //so as to scale from eye/viewpoint to fognode scale
		2. when at Shape node, send fogScale to shader
			in shader, apply fogScale _after_ vertex/fogCoord is transformed by modelview into viewpoint/eyesapce
	LocalFog vs Fog
		LocalFog over-rides Fog, and its the most-local Fog that applies
		- so there should be a stack of fogParameters 
		- LocalFog should push params and bitmask in prep_localFog and pop in fin_localFog
		- render_hier at root level on VF_Blend/VF_Geom pass should see if there's a bound Fog enabled, 
			- if so push its parameters and bitmask
		Fog - on bind_Fog it sets itself as the global fog node
			bind: this means the node type has a 'singleton' effect on the scene ie only one can be active at a time

	Shader pseudo-algo 1:
		dV = distance from point on geometry to viewer's position, in coordinate system (scale) of current fog node
		if(usingFogCoords)
		  dv = fog_depth * fogScale;  //fog_depth[i] is VBO of floats, one per vertex
		else
		  dv = (modelviewMatrix * gl_position).z * fogScale;

		f0 = fog(useFog,fogType,fogVisibility,dv){
			f0 = 1;
			if(useFog){
				f0 = 0;
				if(fogType==LINEAR)
					if(dv < fogVisibility)
						f0 = (fogVisibility-dv)/fogVisibility;
				else //EXPONENTIAL
					if(dv < fogVisibility)
						f0 = exp(-dv/(fogVisibillity -dv) );
			}
			return f0;
		}
	Pseudo-algo 2:
		FOG VERTEX SNIPPET
		vec4 eyeSpacePos = modelViewMatrix*vec4(inPosition, 1.0); 
		if(fogCoords) eyeSpacePos.z = fogCoord;
		eyeSpacePos *= fogScale;

		FOG FRAGMENT SNIPPET
		uniform struct fogParams
		{ 
			vec4 fogColor; 
			float visibilityRange; 
			int fogType; // 0 None, 1= FOGTYPE_LINEAR, 2 = FOGTYPE_EXPONENTIAL
		} fogParams; 

		float fogFactor(fogParams params, float fogDepth);
		float fogDepth = abs(eyeSpacePos.z/eyeSpacePos.w);

		float fogFactor(fogParams params, float fogDepth) 
		{ 
			float ff = 1.0;
			if(params.fogType > 0){
				ff = 0.0; 
				if(params.fogType==FOGTYPE_LINEAR)
					if(dv < fogVisibility)
						fF = (fogVisibility-dv)/fogVisibility;
				else //FOGTYPE_EXPONENTIAL
					if(dv < fogVisibility)
						fF = exp(-dv/(fogVisibillity -dv) );
				ff = clamp(ff, 0.0, 1.0); 
			}
			return ff; 
		}

	Shader requirements signalling, for compiling fog capabilities into shader:
	in render_hier, if VF_Blend || VF_Geom
		set global shader requirements top of stack bitmask to 00000000
		if bound global fog, set fog bit
	in render_node, prep_localFog
		if enableed, 
			copy top-of-stack bitmask
			set local fog bit flag
			push bitmask on bitmask stack
	in render_node, fin_localFog
		if enabled,
			pop bitmask stack
	in render_shape
		//add any locallights, fog, clipplane bitmask to shape's shader requirements bitmask:
		bitmask = node->_shadernode_requirements_bitmask
		bitmask = bitmask |= bitmaskStackTop
		request_shader(bitmask)
		//send fog data to shader program:
		if(fogset(bitmask))
			copy fogTopOfStack to fogParams
			if(have_fogCoords)
				set haveFogCoords in fogParams
				send_fogCoords
			send fogParams


*/
#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CRoutes.h"
#include "../opengl/OpenGL_Utils.h"
#include "../x3d_parser/Bindable.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../scenegraph/Component_Geospatial.h"
#include "../scenegraph/RenderFuncs.h"
#include "../scenegraph/Component_ProgrammableShaders.h"
#include "../scenegraph/Component_Shape.h"
#include "../ui/common.h"
#include "../scenegraph/LinearAlgebra.h"
#define FOGTYPE_LINEAR 1
#define FOGTYPE_EXPONENTIAL 2
unsigned int getShaderFlags();
void pushShaderFlags(unsigned int flags);
void popShaderFlags();
struct X3D_Node *getFogParams();
void pushFogParams(struct X3D_Node *fogparams);
void popFogParams();
struct X3D_Node *getFogParams(); //see renderfuncs.c
//in child_shape or in general in renderable-leaf-nodes, 
// you would call getShaderFlags() and |= to node->_shaderIndex requirement bits before requesting a shader
// and check the fog bit
// if fogbit set, you would call getFogParams() and send them to the shader


//typedef struct fogParams {
//	float color[3];
//	float visibilityRange;
//	float scale;
//	int type; //0=NONE 1=LINEAR 2=EXPONENTIAL
//} fogParams;
double calculateFogScale(){
	//call this when scenegraph render_hier() visiting a Fog or LocalFog node to update its fogScale
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/enveffects.html#Fog
	// "The distances are calculated in the coordinate space of the Fog node."
	// H: that's mathemeatically/numerically equivalent to scaling visibilityRange into the viewpoint system
	// via the transform stack or more precisely the scale effects of the transform stack at the Fog/LocalFog node 
	// in the scenegraph ie:
	// Transform { scale 10 10 10 children [Fog { visibilityRange 2, then fogScale is 10, and viewspace visrange is 10*2 = 20
	GLDOUBLE modelviewMatrix[16], fogLocal[3], eyeLocal[3], eyeLocalB[3], eyeDepth, fogScale;
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
	fogLocal[0] = 0.0; fogLocal[1] = 0.0; fogLocal[2] = 0.0; //point in fog-local
	transformAFFINEd(eyeLocal,fogLocal,modelviewMatrix);
	fogLocal[2] = 1.0; //second point in fog local, just 1 unit away from first point
	transformAFFINEd(eyeLocalB,fogLocal,modelviewMatrix);
	vecdifd(eyeLocal,eyeLocal,eyeLocalB); //get transformed unit vector (how long a unit vector in Fog coords is in View system)
	eyeDepth = veclengthd(eyeLocal);
	if(eyeDepth != 0.0)
		fogScale = eyeDepth; //in shader or before sending to shader, effectively multiplies visibility range by fogScale to get in view scale
	else
		fogScale = 1.0;
	return fogScale;
}
void prep_LocalFog(struct X3D_Node *node){
	//LocalFog applies to siblings and descendents of parent Group
	//Q. how do we handle sibling effects in freewrl?
	struct X3D_LocalFog *fog = (struct X3D_LocalFog*)node;
	if(fog->enabled){
		unsigned int shaderflags;
		//compute fogScale
		fog->__fogScale = calculateFogScale();
		//push fog parameters on fogParams stack
		//copy and push shader requirements stack with fog bit set
		shaderflags = getShaderFlags();
		shaderflags |= FOG_APPEARANCE_SHADER;
		pushShaderFlags(shaderflags);
		pushFogParams((struct X3D_Node*)fog);
	}
}
void fin_LocalFog(struct X3D_Node *node){
	struct X3D_LocalFog *fog = (struct X3D_LocalFog*)node;
	if(fog->enabled){
		popFogParams();
		popShaderFlags();
	}
}

void push_boundFog(){
	//call before render_hier for geom or blend
	//if there's a bound fog, copy its state to fog_state
	ttglobal tg = gglobal();
	if(vectorSize(getActiveBindableStacks(tg)->fog) > 0){
		unsigned int shaderflags;
		//there's a bound fog, bound fogs are enabled
		struct X3D_Fog *fog = stack_top(struct X3D_Fog*,getActiveBindableStacks(tg)->fog);
		//copy and push renderflags
		shaderflags = getShaderFlags();
		//set fog bit in renderflags
		shaderflags |= FOG_APPEARANCE_SHADER;
		pushShaderFlags(shaderflags);
		//push fogparams
		pushFogParams((struct X3D_Node*)fog);
	}
}
void pop_boundFog(){
	//call after render_hier for geom or blend
	ttglobal tg = gglobal();
	if(vectorSize(getActiveBindableStacks(tg)->fog) > 0){
		//pop fogParms
		popFogParams();
		//pop renderflags
		popShaderFlags();
	}
}

void render_Fog(struct X3D_Fog *node) {
	//this can be done on either a prep pass, or rendering pass in render_hier, or all passes
	// - its just to get the fog scale
	//if Fog DEF/USED (multiple scales possible) we use the last one calculated
	int fogType = 0;
	node->__fogScale = calculateFogScale();
	if(node->fogType->strptr){
		if(!strcmp(node->fogType->strptr,"LINEAR")) fogType = FOGTYPE_LINEAR; //1
		if(!strcmp(node->fogType->strptr,"EXPONENTIAL")) fogType = FOGTYPE_EXPONENTIAL; //2
	}
	node->__fogType = fogType;
}
