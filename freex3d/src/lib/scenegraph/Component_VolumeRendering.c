/*


X3D Volume Rendering Component

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
#include "../opengl/Textures.h"
#include "../scenegraph/Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"

/*
Volumee Rendering aka voxels
http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html
- VolumeData is like shape node with its own equivalents to appearance, geometry
- Style nodes - like appearance, except sometimes composable


Before starting to implement this there are a few other nodes and components that might be needed:
- clipPlane - besides transparent voxels, you may want to slice a volumetric image with a clipplane to look inside IMPLEMENTED SEPT 2016
- we have TextureProperties to get the RST
- Texturing3D component > for 3D image file format reading: 
	http://paulbourke.net/dataformats/volumetric/
	- Simplest 3d texture file, if you are writing your own
	http://www.web3d.org/x3d/content/examples/Basic/VolumeRendering/
	- these examples use nrrd format, not much harder IMPLEMENTED Oct 2, 2016
Links:
	http://http.developer.nvidia.com/GPUGems/gpugems_ch39.html
	- GpuGems online, ideas about volume rendering
		- Note also http://teem.sourceforge.net/ link in References
		- same place as nrrd file format lib
	http://castle-engine.sourceforge.net/compositing_shaders.php
	- in the "Compositing Shaders in X3D" .pdf, page 9 mentions volume nodes
	- the VolumeRendering Component has 10 style nodes, and Kambu is suggesting his Plug/hook method
	http://cs.iupui.edu/~tuceryan/research/Microscopy/vis98.pdf
	- paper: "Image-Based Transfer Function Design for Data Exploration in Volume Visualization"
		pain: hard to stay organized with general functions in volume data
		solution: goal-directed composable steps ie sharpen surface, colorize via 'transfer functions'
		1. Apply transfer functions
			A. Gray = F(Gray) or F(F(F(F(Gray)))) ie can chain grayscale functions for each voxel processed
				a) F() image functions - anything from 2D image processing generalized to 3D
				b) F() spatial functions - edge sharpening ie sobel or smoothing also from image processing
			B. Gray to RGBA - lookup table
		2. do your raycasting on RGBA
	http://demos.vicomtech.org/
	- uses webgl and x3dom
	https://www.slicer.org/
	- Uses teem
	http://teem.sourceforge.net/mite/opts.html
	- teem > Mite - has some transfer tables
	http://graphicsrunner.blogspot.ca/2009/01/volume-rendering-101.html
	- shows 'volume raycasting' method, shader (directx) and results
	http://prideout.net/blog/?tag=volume-rendering
	- shows volume raycasting shader example
	https://www.opengl.org/discussion_boards/showthread.php/174814-Save-vertices-to-texture-problem
	- xyz texture preparation, related to volume raycasting shader technique
	http://http.developer.nvidia.com/GPUGems3/gpugems3_ch30.html
	- see 30.3.1 Volume Rendering for raymarching nuances


*/





void compile_IsoSurfaceVolumeData(struct X3D_IsoSurfaceVolumeData *node){
	printf("compile_isosurfacevolumedata not implemented\n");
	MARK_NODE_COMPILED
}

void child_IsoSurfaceVolumeData(struct X3D_IsoSurfaceVolumeData *node){
	static int once = 0;
	COMPILE_IF_REQUIRED

	if(!once)
		printf("child isosurfacevvolumedata not implemented yet\n");
	once = 1;
}


void compile_SegmentedVolumeData(struct X3D_SegmentedVolumeData *node){
	printf("compile_segmentedvolumedata not implemented\n");
	MARK_NODE_COMPILED
}


void child_SegmentedVolumeData(struct X3D_SegmentedVolumeData *node){
	static int once = 0;
	COMPILE_IF_REQUIRED

	if(!once)
		printf("child segmentedvolumedata not implemented yet\n");
	once = 1;
}
//6 faces x 2 triangles per face x 3 vertices per triangle x 3 scalars (xyz) per vertex = 6 x 2 x 3 x 3 = 108
GLfloat box [108] = {1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, };
void compile_VolumeData(struct X3D_VolumeData *node){
	int i,j;
	float *boxtris;

	printf("compile_volumedata not implemented\n");
	if(node->_boxtris == NULL){
		node->_boxtris = MALLOC(void *,108 * sizeof(float));
	}
	boxtris = (float*)node->_boxtris;
	for(i=0;i<36;i++){
		for(j=0;j<3;j++)
			boxtris[i*3 + j] = node->dimensions.c[j] * box[i*3 + j];

	}
	MARK_NODE_COMPILED
}
void sendExplicitMatriciesToShader (GLint ModelViewMatrix, GLint ProjectionMatrix, GLint NormalMatrix, GLint *TextureMatrix, GLint ModelViewInverseMatrix);
void child_VolumeData(struct X3D_VolumeData *node){
	static int once = 0;
	COMPILE_IF_REQUIRED
	if(node->voxels)
		render_node(node->voxels);
	if(!once)
		printf("child volumedata not implemented yet\n");
	once = 1;
	if(node->renderStyle == NULL){
		shaderflagsstruct shaderflags, shader_requirements;
		memset(&shader_requirements,0,sizeof(shaderflagsstruct));
		//shaderflags = getShaderFlags();
		shader_requirements.volume = SHADERFLAGS_VOLUME_BASIC; //send the following through the volume ubershader

		//render 
		//Step 1: 
		//Step 2: get rays to cast: start point and direction vector for each ray to cast
		if(1){
			shaderflagsstruct trickflags;
			s_shader_capabilities_t *caps;
			float *boxtris;
			memset(&shader_requirements,0,sizeof(shaderflagsstruct));
			memset(&trickflags,0,sizeof(shaderflagsstruct));
			trickflags.volume = SHADERFLAGS_VOLUME_BASIC;
			//method A: get per-ray direction vector by subtracting 2 rendered xyz images (near and far) of bounding box

			//A.0 set xyz shaderflag
			trickflags.volume |= SHADERFLAGS_VOLUME_XYZ;
			caps = getMyShaders(trickflags);
			enableGlobalShader(caps);
			//A.0 set box with vol.dimensions with triangles
			//sendAttribToGPU(FW_VERTEX_POINTER_TYPE, 3, GL_FLOAT, GL_FALSE,0, box,0,__FILE__,__LINE__);
			GLint myProg =  caps->myShaderProgram;
			GLint Vertices = GET_ATTRIB(myProg,"fw_Vertex");
			GLint mvm = GET_UNIFORM(myProg,"fw_ModelViewMatrix"); //fw_ModelViewMatrix
			GLint proj = GET_UNIFORM(myProg,"fw_ProjectionMatrix"); //fw_ProjectionMatrix
			sendExplicitMatriciesToShader(mvm,proj,-1,NULL,-1);
			glEnableVertexAttribArray(Vertices);
			boxtris = (float*)node->_boxtris;
			glVertexAttribPointer(Vertices, 3, GL_FLOAT, GL_FALSE, 0, boxtris);

			//GLuint  = GET_UNIFORM(myProg,"fw_BackMaterial.emission");
			//A.2 render front faces to front texture target (normal rendering)
			//renderbox
			if(0)
				glDrawArrays(GL_TRIANGLES,0,36);

			//A.3 render back faces to back texture target (cull counter-clockwise triangles as seen from viewpoint)
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			//glFrontFace(GL_CW); //Android ndk, uwp/angle has this
			//render box
			if(1)
				glDrawArrays(GL_TRIANGLES,0,36);

			//glFrontFace(GL_CCW);
			glCullFace(GL_BACK);
			glDisable(GL_CULL_FACE);
			//A.4 subtract front from back - can do in shader
		
		}else{
			//method B: use cpu math to compute start and direction textures or front and back textures
		}

		//Step 3: accumulate along rays and render opacity fragment in one step
		//shader_requirements.base = shaderflags.base;
		//shader_requirements.effects = shaderflags.effects;
		enableGlobalShader(getMyShaders(shader_requirements));


	}

}
