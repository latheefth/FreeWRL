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
			boxtris[i*3 + j] = .5f * node->dimensions.c[j] * box[i*3 + j];  //raw triangles are -1 to 1, dimensions are absolute

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

void __gluMultMatricesd(const GLDOUBLE a[16], const GLDOUBLE b[16],	GLDOUBLE r[16]);
int __gluInvertMatrixd(const GLDOUBLE m[16], GLDOUBLE invOut[16]);
ivec4 get_current_viewport();
void sendExplicitMatriciesToShader (GLint ModelViewMatrix, GLint ProjectionMatrix, GLint NormalMatrix, GLint *TextureMatrix, GLint ModelViewInverseMatrix);
void child_VolumeData(struct X3D_VolumeData *node){
	static int once = 0;
	ttglobal tg = gglobal();
	ppComponent_VolumeRendering p = (ppComponent_VolumeRendering)tg->Component_VolumeRendering.prv;
	COMPILE_IF_REQUIRED

	if (renderstate()->render_blend == (node->_renderFlags & VF_Blend)) {
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

			//method B: use cpu math to compute a few uniforms so frag shader can do box intersections
			//http://prideout.net/blog/?p=64
			//- one step raycasting using gl_fragCoord
			//

			//Step 3: accumulate along rays and render opacity fragment in one step
			//GPU VERSION
			shaderflagsstruct shaderflags, shader_requirements;
			s_shader_capabilities_t *caps;
			int old_shape_way = 0;

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
				struct X3D_Node *tmpN;
				POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->voxels,tmpN);
				tg->RenderFuncs.texturenode = (void*)tmpN;

				render_node(tmpN); //render_node(node->voxels);

				if(old_shape_way){
					struct textureVertexInfo mtf = {boxtex,2,GL_FLOAT,0,NULL,NULL};
					textureDraw_start(&mtf);
				}else{
					textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tmpN);

					//me->tex3dDepth = GET_UNIFORM(myProg,"tex3dDepth");
					GLint tdepth = GET_UNIFORM(myProg,"tex3dDepth");
					GLUNIFORM1I(tdepth,tti->z);
					//me->tex3dUseVertex = GET_UNIFORM(myProg,"tex3dUseVertex");
					GLint tex3dUseVertex = GET_UNIFORM(myProg,"tex3dUseVertex");
					glUniform1i(tex3dUseVertex,0); 
					GLint repeatSTR = GET_UNIFORM(myProg,"repeatSTR");
					glUniform1iv(repeatSTR,3,tti->repeatSTR);
					GLint magFilter = GET_UNIFORM(myProg,"magFilter");
					glUniform1i(magFilter,tti->magFilter);

					glActiveTexture(GL_TEXTURE0); 
					glBindTexture(GL_TEXTURE_2D,tti->OpenGLTexture); 
				}
			}

			//3.1 set uniforms: dimensions, focal length, fov (field of view), window size, modelview matrix
			//    set attributes vertices of triangles of bounding box
			// set box with vol.dimensions with triangles
			GLint Vertices = GET_ATTRIB(myProg,"fw_Vertex");
			GLint mvm = GET_UNIFORM(myProg,"fw_ModelViewMatrix"); //fw_ModelViewMatrix
			GLint proj = GET_UNIFORM(myProg,"fw_ProjectionMatrix"); //fw_ProjectionMatrix
			static int once = 0;
			if(!once)
				printf("vertices %d mvm %d proj %d\n",Vertices,mvm,proj);
			sendExplicitMatriciesToShader(mvm,proj,-1,NULL,-1);
			double modelviewMatrix[16], mvmInverse[16], projMatrix[16], mvp[16], mvpinverse[16];
			FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
			FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projMatrix);
			if(1){
				//see gluUnproject in Opengl_Utils.c
				__gluMultMatricesd(modelviewMatrix, projMatrix, mvp);
				if (!__gluInvertMatrixd(mvp, mvpinverse)) return;
			}else{
				matmultiplyFULL(mvp,modelviewMatrix,projMatrix);
				//matmultiplyFULL(mvp,projMatrix,modelviewMatrix);
				//if (!__gluInvertMatrixd(mvp, mvpinverse)) return;
				matinverseFULL(mvpinverse,mvp); //seems different than glu's. H0: just wrong H1: transopose H2: full inverse vs factorized
			}
			float spmat[16];
			matdouble2float4(spmat,mvpinverse);

			GLint mvpi = GET_UNIFORM(myProg,"fw_ModelViewProjInverse");
			GLUNIFORMMATRIX4FV(mvpi,1,GL_FALSE,spmat);


			glEnableVertexAttribArray(Vertices);
			float *boxtris = (float*)node->_boxtris;
			glVertexAttribPointer(Vertices, 3, GL_FLOAT, GL_FALSE, 0, boxtris);

			//get the current viewport
			GLint iviewport[4];
			float viewport[4];
			glGetIntegerv(GL_VIEWPORT, iviewport); //xmin,ymin,w,h

			//get the current fieldOfView
			float FieldOfView = tg->Mainloop.fieldOfView;
			//printf("current viewport= %d %d %d %d\n",iviewport[0],iviewport[1],iviewport[2],iviewport[3]);
			//printf("current FOV = %f\n",FieldOfView);
			FieldOfView *= PI/180.0;
			float focalLength = 1.0f / tan(FieldOfView / 2.0f);
			GLint focal = GET_UNIFORM(myProg,"fw_FocalLength"); //fw_ModelViewMatrix
			GLUNIFORM1F(focal,focalLength);
			GLint vp = GET_UNIFORM(myProg,"fw_viewport");
			viewport[0] = iviewport[0]; //xmin
			viewport[1] = iviewport[1]; //ymin
			viewport[2] = iviewport[2]; //width
			viewport[3] = iviewport[3]; //height
			GLUNIFORM4F(vp,viewport[0],viewport[1],viewport[2],viewport[3]);
			GLint dim = GET_UNIFORM(myProg,"fw_dimensions");
			GLUNIFORM3F(dim,node->dimensions.c[0],node->dimensions.c[1],node->dimensions.c[2]);

			//ray origin: the camera position 0,0,0 transformed into geometry local (box) coords
			GLint orig = GET_UNIFORM(myProg,"fw_RayOrigin");
			float eyeLocal[3];
			double origind[3], eyeLocald[3];
			origind[0] = origind[1] = origind[2] = 0.0;
			FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);

			matinverseAFFINE(mvmInverse,modelviewMatrix);
			transformAFFINEd(eyeLocald,origind,mvmInverse);

			for(int i=0;i<3;i++) eyeLocal[i] = eyeLocald[i];
			GLUNIFORM3F(orig,eyeLocal[0],eyeLocal[1],eyeLocal[2]);
			//printf("rayOrigin= %f %f %f\n",eyeLocal[0],eyeLocal[1],eyeLocal[2]);
			if(!once) printf("orig %d dim %d vp %d focal %d\n",orig,dim,vp,focal );

			//3.2 draw with shader
			glDrawArrays(GL_TRIANGLES,0,36);

			if(node->voxels){
				if(old_shape_way){
					textureDraw_end();
				}else{
					tg->RenderFuncs.textureStackTop = 0;
					tg->RenderFuncs.texturenode = NULL;
				}
			}
			once = 1;
		} 


	} //VF_Blend

}
