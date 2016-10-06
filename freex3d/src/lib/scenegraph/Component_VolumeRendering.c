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
#include "../scenegraph/LinearAlgebra.h"

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

typedef struct pComponent_VolumeRendering{
	GLuint front_texture;
	GLuint back_texture;
	GLint ifbobuffer;
	GLint idepthbuffer;
	int width, height;
	GLfloat *quad;
}* ppComponent_VolumeRendering;
void *Component_VolumeRendering_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_VolumeRendering));
	memset(v,0,sizeof(struct pComponent_VolumeRendering));
	return v;
}
void Component_VolumeRendering_init(struct tComponent_VolumeRendering *t){
	//public
	//private
	t->prv = Component_VolumeRendering_constructor();
	{
		ppComponent_VolumeRendering p = (ppComponent_VolumeRendering)t->prv;
		p->back_texture = 0;
		p->front_texture = 0;
		p->ifbobuffer = 0;
		p->idepthbuffer = 0;
		p->width = 0;
		p->height = 0;
		p->quad = NULL;
	}
}
void Component_VolumeRendering_clear(struct tComponent_VolumeRendering *t){
	//public
	//private
	{
		ppComponent_VolumeRendering p = (ppComponent_VolumeRendering)t->prv;
		//p->front_texture;
		//p->back_texture;
		FREE_IF_NZ(p->quad);
	}
}
//ppComponent_VolumeRendering p = (ppComponent_VolumeRendering)gglobal()->Component_VolumeRendering.prv;




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
void pushnset_framebuffer(int ibuffer);
void popnset_framebuffer();

#ifdef GL_DEPTH_COMPONENT32
#define FW_GL_DEPTH_COMPONENT GL_DEPTH_COMPONENT32
#else
#define FW_GL_DEPTH_COMPONENT GL_DEPTH_COMPONENT16
#endif
ivec4 get_current_viewport();
void sendExplicitMatriciesToShader (GLint ModelViewMatrix, GLint ProjectionMatrix, GLint NormalMatrix, GLint *TextureMatrix, GLint ModelViewInverseMatrix);
void child_VolumeData(struct X3D_VolumeData *node){
	static int once = 0;
	ttglobal tg = gglobal();
	ppComponent_VolumeRendering p = (ppComponent_VolumeRendering)tg->Component_VolumeRendering.prv;
	COMPILE_IF_REQUIRED
	if(node->voxels)
		render_node(node->voxels);
	if(!once)
		printf("child volumedata not implemented yet\n");
	once = 1;
	if(node->renderStyle == NULL){

		//render 
		//Step 1: set the 3D texture
		//if(node->voxels)
		//	render_node(node->voxels);
		//Step 2: get rays to cast: start point and direction vector for each ray to cast
		if(0){
			shaderflagsstruct trickflags;
			s_shader_capabilities_t *caps;
			float *boxtris;
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

			//A.1 setup a framebuffer with texture targets
			if(p->ifbobuffer == 0){
				glGenFramebuffers(1, &p->ifbobuffer);
				pushnset_framebuffer(p->ifbobuffer); 
				glGenRenderbuffers(1, &p->idepthbuffer);
				glBindRenderbuffer(GL_RENDERBUFFER, p->idepthbuffer);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, p->idepthbuffer);
				glGenTextures(1,&p->front_texture);
				glGenTextures(1,&p->back_texture);
				popnset_framebuffer();
			}
			pushnset_framebuffer(p->ifbobuffer); //binds framebuffer. we push here, in case higher up we are already rendering the whole scene to an fbo
			glBindRenderbuffer(GL_RENDERBUFFER, p->idepthbuffer);
				
			ivec4 ivp = get_current_viewport();
			if(ivp.W != p->width || ivp.H != p->height){
				//regenerate fbo buffers
				int j;
				// https://www.opengl.org/wiki/Framebuffer_Object
				glRenderbufferStorage(GL_RENDERBUFFER, FW_GL_DEPTH_COMPONENT, p->width,p->height);
				glBindTexture(GL_TEXTURE_2D, p->front_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p->width,p->height, 0, GL_RGBA , GL_UNSIGNED_BYTE, 0);
				glBindTexture(GL_TEXTURE_2D, p->back_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p->width,p->height, 0, GL_RGBA , GL_UNSIGNED_BYTE, 0);

				if(p->quad == NULL)
					p->quad = MALLOC(float *,18*sizeof(float));
				memcpy(p->quad,box,18*sizeof(float));
				//q. do I need to scale a quad to fit the screen?


			}

			//renderbox
			//A.2 render front faces to front texture target (normal rendering)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p->front_texture, 0);
			glDrawArrays(GL_TRIANGLES,0,36);

			//A.3 render back faces to back texture target (cull counter-clockwise triangles as seen from viewpoint)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p->back_texture, 0);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			//render box
			glDrawArrays(GL_TRIANGLES,0,36);
			glCullFace(GL_BACK);
			glDisable(GL_CULL_FACE);
			//A.4 subtract front from back - can do in shader
			popnset_framebuffer(p->ifbobuffer); //sets back to normal GL_BACK
			//now we should have fresh XYZ front_texture and back_texture
		
			//Step 3: accumulate along rays and render opacity fragment in one step
			if(1){
				shaderflagsstruct shaderflags, shader_requirements;
				memset(&shader_requirements,0,sizeof(shaderflagsstruct));
				//shaderflags = getShaderFlags();
				shader_requirements.volume = SHADERFLAGS_VOLUME_BASIC; //send the following through the volume ubershader
				shader_requirements.volume |= SHADERFLAGS_VOLUME_OPACITY;
				enableGlobalShader(getMyShaders(shader_requirements));
				//3.1 set quad geometry scaled to fill viewport
				//    set 2 textures
				//3.2 set inverse modelview and dimensions in shader, along with the usuals
				//3.3 tell shader to do the raycasting
				/*
				in fragment:
				for each fragment of the screen viewport-filling Quad
					sample front texture
					sample back texture
					if(both 0) discard fragment //we're not on the cube
					construct vertex front and direction vector
					loop along ray from front to back, 
						constructing a new vertex p at each ray point
						transform p back into local coords of Volume using modelviewmatrix inverse
						scale by 1/dimension to get back into perfect cube
						use either texture3D OES 
							- or something like my slices, as used for tex3d, to get 3D texture scalar value
						accumulate opacity
						if(opaque) break;
					gl_frag = opacity
				*/


			}


		}else{
			//method B: use cpu math to compute a few uniforms so frag shader can do box intersections
			//http://prideout.net/blog/?p=64
			//- one step raycasting using gl_fragCoord
			//

			//Step 3: accumulate along rays and render opacity fragment in one step
			if(1){
				shaderflagsstruct shaderflags, shader_requirements;
				s_shader_capabilities_t *caps;

				memset(&shader_requirements,0,sizeof(shaderflagsstruct));
				//shaderflags = getShaderFlags();
				shader_requirements.volume = SHADERFLAGS_VOLUME_BASIC; //send the following through the volume ubershader
				shader_requirements.volume |= SHADERFLAGS_VOLUME_OPACITY;
				shader_requirements.volume |= TEX3D_SHADER;
				caps = getMyShaders(shader_requirements);
				enableGlobalShader(caps);
				GLint myProg =  caps->myShaderProgram;
				//Step 1: set the 3D texture
				if(node->voxels){

					render_node(node->voxels);
					if(0){
						textureTableIndexStruct_s *tti = getTableTableFromTextureNode(X3D_NODE(node->voxels));
						GLint texloc = GET_UNIFORM(myProg,"fw_Texture_unit0");
						glUniform1i ( texloc, tti->OpenGLTexture );
					}
				}

				//3.1 set uniforms: dimensions, focal length, fov (field of view), window size, modelview matrix
				//    set attributes vertices of triangles of bounding box
				// set box with vol.dimensions with triangles
				GLint Vertices = GET_ATTRIB(myProg,"fw_Vertex");
				GLint mvm = GET_UNIFORM(myProg,"fw_ModelViewMatrix"); //fw_ModelViewMatrix
				GLint proj = GET_UNIFORM(myProg,"fw_ProjectionMatrix"); //fw_ProjectionMatrix
				sendExplicitMatriciesToShader(mvm,proj,-1,NULL,-1);
				glEnableVertexAttribArray(Vertices);
				float *boxtris = (float*)node->_boxtris;
				glVertexAttribPointer(Vertices, 3, GL_FLOAT, GL_FALSE, 0, boxtris);

				//get the current viewport
				GLint iviewport[4];
				float viewport[4];
				glGetIntegerv(GL_VIEWPORT, iviewport);

				//get the current fieldOfView
				float FieldOfView = tg->Mainloop.fieldOfView;
				//printf("current viewport= %d %d %d %d\n",iviewport[0],iviewport[1],iviewport[2],iviewport[3]);
				//printf("current FOV = %f\n",FieldOfView);
				FieldOfView *= PI/180.0;
				float focalLength = 1.0f / tan(FieldOfView / 2.0f);
				GLint focal = GET_UNIFORM(myProg,"fw_focalLength"); //fw_ModelViewMatrix
				GLUNIFORM1F(focal,focalLength);
				GLint vp = GET_UNIFORM(myProg,"fw_viewport");
				viewport[0] = iviewport[2] - iviewport[0];
				viewport[1] = iviewport[3] - iviewport[1];
				viewport[2] = iviewport[0];
				viewport[3] = iviewport[1];
				GLUNIFORM4F(vp,viewport[0],viewport[1],viewport[2],viewport[3]);
				GLint dim = GET_UNIFORM(myProg,"fw_dimensions");
				GLUNIFORM3F(dim,node->dimensions.c[0],node->dimensions.c[1],node->dimensions.c[2]);

				//ray origin: the camera position 0,0,0 transformed into geometry local (box) coords
				GLint orig = GET_UNIFORM(myProg,"fw_RayOrigin");
				float eyeLocal[3];
				double origind[3], eyeLocald[3];
				origind[0] = origind[1] = origind[2] = 0.0;
				double modelviewMatrix[16], mvmInverse[16];
				//GL_GET_MODELVIEWMATRIX
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
				matinverseAFFINE(mvmInverse,modelviewMatrix);
				transformAFFINEd(eyeLocald,origind,mvmInverse);
				for(int i=0;i<3;i++) eyeLocal[i] = eyeLocald[i];
				GLUNIFORM3F(orig,eyeLocal[0],eyeLocal[1],eyeLocal[2]);
				//printf("rayOrigin= %f %f %f\n",eyeLocal[0],eyeLocal[1],eyeLocal[2]);


				//3.2 draw with shader
				glDrawArrays(GL_TRIANGLES,0,36);

				/*
				in vertex:
					render bounding box, color not important
				in fragment:
				for each fragment of the rendered bounding box
					compute ray direction
					transform ray direction and viewpoint origin from view space to local / model space
						- perhaps further into cuboid space of volume texture, via 1/dimensions[3]
					intersect ray with bounding box to get 2 intersection points front and back
					loop along ray from front to back, 
						constructing a new vertex p at each ray point
						transform p back into local coords of Volume using modelviewmatrix inverse
						scale by 1/dimension to get back into perfect cube
						use either texture3D OES 
							- or something like my slices, as used for tex3d, to get 3D texture scalar value
						accumulate opacity
						if(opaque) break;
					gl_frag = opacity
				*/


			}

		}

	}

}
