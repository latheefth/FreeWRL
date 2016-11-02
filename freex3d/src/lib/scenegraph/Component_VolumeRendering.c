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
	http://teem.sourceforge.net/ 
	- same place as nrrd file format lib
	- unu.exe commandline program is handy:
		print nrrd file header:
			unu head brain.nrrd
		resize an image ie from 512x512x512 to 128x128x128:
			unu resample -s 128 128 128 -i brain.nrrd -o brain128.nrrd
				
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


COMPONENT VOLUMERENDERING ISSUES Oct 29, 2016
http://dug9.users.sourceforge.net/web3d/tests/volume/
- these sample scenes use max size 128 .nrrd textures

By Node:

VoiumeData
	- works
SegmentedVolumeData
	- no confirmation its working
	- SegmentedVentricles.x3d - cycles every 3 seconds or so
IsoSurfaceVolumeData
	- no confirm
	- IsoSurfaceSkull.x3d

BlendedVolumeStyle
	- blackscreens
	- BlendedBodyInternals.x3d - blackscreens
	- BlendedComposedVolumes.x3d - blackscreens
BoundaryEnhancementVolumeStyle
	- no confirm
	- BoundaryEnhancementInternals.x3d - blackscreens
	- BlendedComposedVolumes.x3d - blackscreens
CartoonVolumeStyle
	- works
	- CartoonBackpack.x3d 
	- BlendedComposedVolumes.x3d
ComposedVolumeStyle
	- no confirm
	- ComposedBackpack.x3d
	- BlendedComposedVolumes.x3d
EdgeEnhancementVolumeStyle
	- works
	- EdgeBrain.x3d
	- ComposedBackpack.x3d
	- BlendedComposedVolumes.x3d
OpacityMapVolumeStyle
	- no TransferFunction verification
	- basics (implicit) works, plus explicit:
	- BlendedComposedVolumes.x3d
	- SegmentedVentricles.x3d
ProjectionVolumeStyle
	- works
	- ProjectionMaxVentricles.x3d 
ShadedVolumeStyle
	- no confirm
	- ShadedBrain.x3d - no evidence of shading
SillouetteEnhancementVolumeStyle
	- works but hard to see a difference
	- SilhouetteSkull.x3d
	- ComposedBackpack.x3d
	- BlendedComposedVolumes.x3d
ToneMappedVolumeStyle
	- works
	- BlendedComposedVolumes.x3d
	- ToneInternal.x3d - blackscreens


By Technique:

gradients:
	IsoSurfaceVolumeData

surfaceNormals:
	CartoonVolumeStyle
	EdgeEnhacementVolumeStyle
	ShadedVolumeStyle
	SilhouetteEnhancementVolumeStyle
	ToneMappedVolumeStyle

segmentIdentifiers:
	SegmentedVolumeData

Texture2D as transfer function:
	BlendedVolumeStyle > weightTransferFunction1, weightTransferFunction2
	OpacityMapVolumeStyle > transferFunction

MF list:
MFNode -renderStyle > ComposedVolumeStyle, IsoSurfaceVolumeData, SegmentedVolumeStyle
MFFloat -surfaceValues > IsoSurfaceVolumeData


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




//6 faces x 2 triangles per face x 3 vertices per triangle x 3 scalars (xyz) per vertex = 6 x 2 x 3 x 3 = 108
GLfloat box [108] = {1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, };

//      6  7 //back far z
//      4  5
// 2  3   //front near z
// 0  1 
float boxvert [24] = {
-.5f,-.5f, .5f, .5f,-.5f, .5f, -.5f,.5f, .5f, .5f,.5f, .5f, //near z LL LR UL UR
-.5f,-.5f,-.5f, .5f,-.5f,-.5f, -.5f,.5f,-.5f, .5f,.5f,-.5f, //far z
};
//ccw tris
ushort boxtriindccw [48] = {
0, 1, 3, -1,  //near z
3, 2, 0, -1,
1, 5, 7, -1, //right
7, 3, 1, -1,
5, 4, 6, -1, //back z
6, 7, 5, -1, 
4, 0, 2, -1, //left
2, 6, 4, -1,
2, 3, 7, -1, //top y
7, 6, 2, -1,
4, 5, 1, -1, //bottom y
1, 0, 4, -1,
};
ushort boxtriindcw [48] = {
0, 3, 1, -1,  //near z
3, 0, 2, -1,
1, 7, 5, -1, //right
7, 1, 3, -1,
5, 6, 4, -1, //back z
6, 5, 7, -1, 
4, 2, 0, -1, //left
2, 4, 6, -1,
2, 7, 3, -1, //top y
7, 2, 6, -1,
4, 1, 5, -1, //bottom y
1, 4, 0, -1,
};
void compile_VolumeData(struct X3D_VolumeData *node){
	int i,j,itri, ind, jvert;
	float *boxtris;

	ConsoleMessage("compile_volumedata\n");
	if(node->_boxtris == NULL){
		node->_boxtris = MALLOC(void *,108 * sizeof(float));
	}
	boxtris = (float*)node->_boxtris;
	if(0)
	for(i=0;i<36;i++){
		for(j=0;j<3;j++)
			boxtris[i*3 + j] = .5f * node->dimensions.c[j] * box[i*3 + j];  //raw triangles are -1 to 1, dimensions are absolute

	}
	if(1)
	for(itri=0;itri<12;itri++){
		for(jvert=0;jvert<3;jvert++) {
			float *vert;
			ind = boxtriindccw[itri*4 + jvert];
			vert = &boxvert[ind*3];
			for(j=0;j<3;j++){
				boxtris[(itri*3 +jvert)*3 + j] = node->dimensions.c[j]*vert[j];
			}
		}
	}
	//for(i=0;i<36;i++)
	//	printf("%f %f %f\n",boxtris[i*3 +0],boxtris[i*3 +1],boxtris[i*3 +2]);
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
textureTableIndexStruct_s *getTableTableFromTextureNode(struct X3D_Node *textureNode);

unsigned int prep_volumestyle(struct X3D_Node *vstyle, unsigned int volflags){
	struct X3D_OpacityMapVolumeStyle *style0 = (struct X3D_OpacityMapVolumeStyle*)vstyle;
	if(style0->enabled){
		switch(vstyle->_nodeType){
			case NODE_OpacityMapVolumeStyle:
				volflags = volflags << 4;
				volflags |= SHADERFLAGS_VOLUME_STYLE_OPACITY;
				break;
			case NODE_BlendedVolumeStyle:
				//volflags = volflags << 4;
				//volflags |= SHADERFLAGS_VOLUME_STYLE_BLENDED;
				//do nothing, so parent renders as default, or gets a style via composed
				break;
			case NODE_BoundaryEnhancementVolumeStyle:
				volflags = volflags << 4;
				volflags |= SHADERFLAGS_VOLUME_STYLE_BOUNDARY;
				break;
			case NODE_CartoonVolumeStyle:
				volflags = volflags << 4;
				volflags |= SHADERFLAGS_VOLUME_STYLE_CARTOON;
				break;
			case NODE_ComposedVolumeStyle:
				{
					struct X3D_ComposedVolumeStyle *style = (struct X3D_ComposedVolumeStyle*)vstyle;
					//volflags = volflags << 4;
					//volflags |= SHADERFLAGS_VOLUME_STYLE_COMPOSED;
					// I 'unroll' composed here, into a bit-shifted list with 4 bits per entry
					for(int i=0;i<style->renderStyle.n;i++){
						volflags = prep_volumestyle(style->renderStyle.p[i], volflags);
					}
				}
				break;
			case NODE_EdgeEnhancementVolumeStyle:
				volflags = volflags << 4;
				volflags |= SHADERFLAGS_VOLUME_STYLE_EDGE;
				break;
			case NODE_ProjectionVolumeStyle:
				volflags = volflags << 4;
				volflags |= SHADERFLAGS_VOLUME_STYLE_PROJECTION;
				break;
			case NODE_ShadedVolumeStyle:
				volflags = volflags << 4;
				volflags |= SHADERFLAGS_VOLUME_STYLE_SHADED;
				break;
			case NODE_SilhouetteEnhancementVolumeStyle:
				volflags = volflags << 4;
				volflags |= SHADERFLAGS_VOLUME_STYLE_SILHOUETTE;
				break;
			case NODE_ToneMappedVolumeStyle:
				volflags = volflags << 4;
				volflags |= SHADERFLAGS_VOLUME_STYLE_TONE;
				break;
			default:
				break;
		}
	}
	return volflags;
}
void render_volume_data(struct X3D_Node *renderStyle, struct X3D_Node *voxels, struct X3D_VolumeData *node);
struct X3D_Material *get_material_oneSided();
struct X3D_TwoSidedMaterial *get_material_twoSided();

void render_volumestyle(struct X3D_Node *vstyle, GLint myProg){
	struct X3D_OpacityMapVolumeStyle *style0 = (struct X3D_OpacityMapVolumeStyle*)vstyle;
	if(style0->enabled){
		switch(vstyle->_nodeType){
			case NODE_OpacityMapVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#OpacityMapVolumeStyle
					// Q. how do the transfer function on GPU? its defined like its on the CPU ie 
					//   with integers. On GPU the .a will be 0.0 to 1.0. I guess that can be used as 1D texture coordinate.
					int havetexture;
					struct X3D_OpacityMapVolumeStyle *style = (struct X3D_OpacityMapVolumeStyle*)vstyle;
					havetexture = 0;
					if(style->transferFunction){
						//load texture
						struct X3D_Node *tmpN;
						ttglobal tg = gglobal();

						POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, style->transferFunction,tmpN);
						tg->RenderFuncs.texturenode = (void*)tmpN;

						//problem: I don't want it sending image dimensions to my volume shader,
						// which could confuse the voxel sampler
						//render_node(tmpN); //render_node(node->texture); 
						loadTextureNode(tmpN,NULL);
						textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tmpN);
						if(tti && tti->status >= TEX_LOADED){
							glActiveTexture(GL_TEXTURE0+3); 
							glBindTexture(GL_TEXTURE_2D,tti->OpenGLTexture); 
							havetexture = 1;
						}
					}
					GLint iopactex;
					iopactex = GET_UNIFORM(myProg,"fw_opacTexture");
					glUniform1i(iopactex,havetexture);

				}
				break;
			case NODE_BlendedVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#BlendedVolumeStyle
					struct X3D_BlendedVolumeStyle *style = (struct X3D_BlendedVolumeStyle*)vstyle;
					//FBO blending
					//a)  render the parent volumeData to fbo:
					//	- in prep Blended push fbo
					//	- render main to fbo
					//	- in fin Blended: read pixels or save renderbuffer texture 0
					//b)  in fin Blended: render blended (voxels,stye) as VolumeData to fbo
					//	- read pixels or same renderbuffer texture 1
					//c)   pop fbo
					//d)  set 2 pixelsets as textures
					//     and render via a special little shader that blends 2 textures and sends to GL_BACK. 
					#define BLENDED 1
					#ifdef BLENDED
					int *fbohandles = style->_fbohandles.p;
					if(fbohandles[0] == 0){
						// https://www.opengl.org/wiki/Framebuffer_Object
						glGenFramebuffers(1, &fbohandles[0]);
						pushnset_framebuffer(fbohandles[0]); //binds framebuffer. we push here, in case higher up we are already rendering the whole scene to an fbo

						glGenTextures(1,&fbohandles[1]);
						glBindTexture(GL_TEXTURE_2D, fbohandles[1]);

						GLint iviewport[4];
						glGetIntegerv(GL_VIEWPORT, iviewport); //xmin,ymin,w,h

						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iviewport[2], iviewport[3], 0, GL_RGBA , GL_UNSIGNED_BYTE, 0);
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbohandles[1], 0);

						glGenTextures(1,&fbohandles[2]);
						glBindTexture(GL_TEXTURE_2D, fbohandles[2]);

						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iviewport[2], iviewport[3], 0, GL_RGBA , GL_UNSIGNED_BYTE, 0);
						//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+1, GL_TEXTURE_2D, fbohandles[2], 0);
						//--dont assign the second texture till after the parent VolumeData has drawn itself

						//popnset_framebuffer(); //pop after drawing
					}else{
						pushnset_framebuffer(fbohandles[0]);
					}

					#endif //BLENDED
				}
				break;
			case NODE_BoundaryEnhancementVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#BoundaryEnhancementVolumeStyle
					struct X3D_BoundaryEnhancementVolumeStyle *style = (struct X3D_BoundaryEnhancementVolumeStyle*)vstyle;
					//SFFloat     [in,out] boundaryOpacity  0.9     [0,1]
					//SFFloat     [in,out] opacityFactor    2       [0,?)
					//SFFloat     [in,out] retainedOpacity  0.2     [0,1]
					GLint ibebound, iberetain, ibefactor;
					ibebound = GET_UNIFORM(myProg,"fw_boundaryOpacity");
					glUniform1f(ibebound,style->boundaryOpacity);
					iberetain = GET_UNIFORM(myProg,"fw_retainedOpacity");
					glUniform1f(iberetain,style->retainedOpacity);
					ibefactor = GET_UNIFORM(myProg,"fw_opacityFactor");
					glUniform1f(ibefactor,style->opacityFactor);
				}
				break;
			case NODE_CartoonVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#CartoonVolumeStyle
					struct X3D_CartoonVolumeStyle *style = (struct X3D_CartoonVolumeStyle*)vstyle;
					//SFInt32     [in,out] colorSteps       4       [1,64]
					//SFColorRGBA [in,out] orthogonalColor  1 1 1 1 [0,1]
					//SFColorRGBA [in,out] parallelColor    0 0 0 1 [0,1]
					//SFNode      [in,out] surfaceNormals   NULL    [X3DTexture3DNode]
					GLint itoonsteps, itoonortho, itoonparallel;
					itoonsteps = GET_UNIFORM(myProg,"fw_colorSteps");
					glUniform1i(itoonsteps,style->colorSteps);
					itoonortho = GET_UNIFORM(myProg,"fw_orthoColor");
					glUniform4fv(itoonortho,1,style->orthogonalColor.c);
					itoonparallel = GET_UNIFORM(myProg,"fw_paraColor");
					glUniform4fv(itoonparallel,1,style->parallelColor.c);

				}
				break;
			case NODE_ComposedVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#ComposedVolumeStyle
					struct X3D_ComposedVolumeStyle *style = (struct X3D_ComposedVolumeStyle*)vstyle;
					for(int i=0;i<style->renderStyle.n;i++){
						render_volumestyle(style->renderStyle.p[i], myProg);
					}
				}
				break;
			case NODE_EdgeEnhancementVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#EdgeEnhancementVolumeStyle
					//SFColorRGBA [in,out] edgeColor         0 0 0 1 [0,1]
					//SFBool      [in,out] enabled           TRUE
					//SFFloat     [in,out] gradientThreshold 0.4     [0,PI]
					//SFNode      [in,out] metadata          NULL    [X3DMetadataObject]
					//SFNode      [in,out] surfaceNormals    NULL    [X3DTexture3DNode]
					struct X3D_EdgeEnhancementVolumeStyle *style = (struct X3D_EdgeEnhancementVolumeStyle*)vstyle;
					GLint iedgeColor, igradientThreshold;
					float *rgba;
					rgba = style->edgeColor.c;
					iedgeColor = GET_UNIFORM(myProg,"fw_edgeColor");
					glUniform4fv(iedgeColor,1,rgba);
					igradientThreshold = GET_UNIFORM(myProg,"fw_cosGradientThreshold");
					glUniform1f(igradientThreshold,cosf(style->gradientThreshold));
					//printf("edge uniforms color %d gradthresh %d\n",iedgeColor,igradientThreshold);
				}
				break;
			case NODE_ProjectionVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#ProjectionVolumeStyle
					struct X3D_ProjectionVolumeStyle *style = (struct X3D_ProjectionVolumeStyle*)vstyle;
					//SFFloat  [in,out] intensityThreshold 0     [0,1]
					//SFString [in,put] type               "MAX" ["MAX", "MIN", "AVERAGE"]
					GLint iintensity, itype;
					int ktype;
					char *ctype;
					iintensity = GET_UNIFORM(myProg,"fw_intensityThreshold");
					glUniform1f(iintensity,style->intensityThreshold);
					itype = GET_UNIFORM(myProg,"fw_projType");
					if(style->_type == 0){
						ctype = style->type->strptr;
						if(!strcmp(ctype,"MIN")) 
							ktype = 1;
						else if(!strcmp(ctype,"MAX")) 
							ktype = 2;
						else if(!strcmp(ctype,"AVERAGE")) 
							ktype = 3;
						style->_type = ktype;
					}
					glUniform1i(itype,style->_type);
				}
				break;
			case NODE_ShadedVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#ShadedVolumeStyle
					struct X3D_ShadedVolumeStyle *style = (struct X3D_ShadedVolumeStyle*)vstyle;
					//SFBool   [in,out] lighting       FALSE
					//SFNode   [in,out] material       NULL                [X3DMaterialNode]
					//SFBool   [in,out] shadows        FALSE
					//SFNode   [in,out] surfaceNormals NULL                [X3DTexture3DNode]
					//SFString []       phaseFunction  "Henyey-Greenstein" ["Henyey-Greenstein","NONE",...]
					//MATERIAL
					if(style->material){
						struct fw_MaterialParameters defaultMaterials = {
									{0.0f, 0.0f, 0.0f, 1.0f}, /* Emission */
									{0.0f, 0.0f, 0.0f, 1.0f}, /* Ambient */
									{0.8f, 0.8f, 0.8f, 1.0f}, /* Diffuse */
									{0.0f, 0.0f, 0.0f, 1.0f}, /* Specular */
									10.0f};                   /* Shininess */

						struct matpropstruct *myap = getAppearanceProperties();

						memcpy (&myap->fw_FrontMaterial, &defaultMaterials, sizeof (struct fw_MaterialParameters));
						memcpy (&myap->fw_BackMaterial, &defaultMaterials, sizeof (struct fw_MaterialParameters));

						RENDER_MATERIAL_SUBNODES(style->material);
						//struct matpropstruct matprop;
						//s_shader_capabilities_t mysp;
						//sendFogToShader(mysp); 
						struct X3D_Material *matone;
						struct X3D_TwoSidedMaterial *mattwo;
						matone = get_material_oneSided();
						mattwo = get_material_twoSided();
						//sendMaterialsToShader(mysp);
						if (matone != NULL) {
							memcpy (&myap->fw_FrontMaterial, matone->_verifiedColor.p, sizeof (struct fw_MaterialParameters));
							memcpy (&myap->fw_BackMaterial, matone->_verifiedColor.p, sizeof (struct fw_MaterialParameters));
							/* copy the emissive colour over for lines and points */
							memcpy(&myap->emissionColour,matone->_verifiedColor.p, 3*sizeof(float));

						} else if (mattwo != NULL) {
							memcpy (&myap->fw_FrontMaterial, mattwo->_verifiedFrontColor.p, sizeof (struct fw_MaterialParameters));
							memcpy (&myap->fw_BackMaterial, mattwo->_verifiedBackColor.p, sizeof (struct fw_MaterialParameters));
							/* copy the emissive colour over for lines and points */
							memcpy(&myap->emissionColour,mattwo->_verifiedFrontColor.p, 3*sizeof(float));
						} else {
							/* no materials selected.... */
						}


						struct fw_MaterialParameters *fw_FrontMaterial;
						struct fw_MaterialParameters *fw_BackMaterial;

						if (!myap) return;
						fw_FrontMaterial = &myap->fw_FrontMaterial;
						fw_BackMaterial = &myap->fw_BackMaterial;


						PRINT_GL_ERROR_IF_ANY("BEGIN sendMaterialsToShader");

						/* eventually do this with code blocks in glsl */
						GLint myMaterialAmbient;
						GLint myMaterialDiffuse;
						GLint myMaterialSpecular;
						GLint myMaterialShininess;
						GLint myMaterialEmission;

						GLint myMaterialBackAmbient;
						GLint myMaterialBackDiffuse;
						GLint myMaterialBackSpecular;
						GLint myMaterialBackShininess;
						GLint myMaterialBackEmission;


						myMaterialEmission = GET_UNIFORM(myProg,"fw_FrontMaterial.emission");
						myMaterialDiffuse = GET_UNIFORM(myProg,"fw_FrontMaterial.diffuse");
						myMaterialShininess = GET_UNIFORM(myProg,"fw_FrontMaterial.shininess");
						myMaterialAmbient = GET_UNIFORM(myProg,"fw_FrontMaterial.ambient");
						myMaterialSpecular = GET_UNIFORM(myProg,"fw_FrontMaterial.specular");

						myMaterialBackEmission = GET_UNIFORM(myProg,"fw_BackMaterial.emission");
						myMaterialBackDiffuse = GET_UNIFORM(myProg,"fw_BackMaterial.diffuse");
						myMaterialBackShininess = GET_UNIFORM(myProg,"fw_BackMaterial.shininess");
						myMaterialBackAmbient = GET_UNIFORM(myProg,"fw_BackMaterial.ambient");
						myMaterialBackSpecular = GET_UNIFORM(myProg,"fw_BackMaterial.specular");


						profile_start("sendvec");
						GLUNIFORM4FV(myMaterialAmbient,1,fw_FrontMaterial->ambient);
						GLUNIFORM4FV(myMaterialDiffuse,1,fw_FrontMaterial->diffuse);
						GLUNIFORM4FV(myMaterialSpecular,1,fw_FrontMaterial->specular);
						GLUNIFORM4FV(myMaterialEmission,1,fw_FrontMaterial->emission);
						GLUNIFORM1F(myMaterialShininess,fw_FrontMaterial->shininess);

						GLUNIFORM4FV(myMaterialBackAmbient,1,fw_BackMaterial->ambient);
						GLUNIFORM4FV(myMaterialBackDiffuse,1,fw_BackMaterial->diffuse);
						GLUNIFORM4FV(myMaterialBackSpecular,1,fw_BackMaterial->specular);
						GLUNIFORM4FV(myMaterialBackEmission,1,fw_BackMaterial->emission);
						GLUNIFORM1F(myMaterialBackShininess,fw_BackMaterial->shininess);
						profile_end("sendvec");


					}
					if(style->lighting){
						//LIGHT
						//FOG
						//-these are from the scenegraph above the voldata node, and -like clipplane- can/should be
						//set generically
					}

					//phasefunc
					if(style->_phaseFunction == 0){
						if(!strcmp(style->phaseFunction->strptr,"NONE"))
							style->_phaseFunction = 1;
						else if(!strcmp(style->phaseFunction->strptr,"Henyey-Greenstein"))
							style->_phaseFunction = 2;
					}
					GLint iphase, ilite, ishadow;
					iphase = GET_UNIFORM(myProg,"fw_phase");
					glUniform1i(iphase,style->_phaseFunction);
					ilite = GET_UNIFORM(myProg,"fw_lighting");
					glUniform1i(ilite,style->lighting);
					ishadow = GET_UNIFORM(myProg,"fw_shadows");
					glUniform1i(ishadow,style->shadows);
				}
				break;
			case NODE_SilhouetteEnhancementVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#SilhouetteEnhancementVolumeStyle
					struct X3D_SilhouetteEnhancementVolumeStyle *style = (struct X3D_SilhouetteEnhancementVolumeStyle*)vstyle;
					//SFFloat [in,out] silhouetteBoundaryOpacity 0    [0,1]
					//SFFloat [in,out] silhouetteRetainedOpacity 1    [0,1]
					//SFFloat [in,out] silhouetteSharpness       0.5  [0,8)
					GLint isilbound, isilretain, isilsharp;
					isilbound = GET_UNIFORM(myProg,"fw_BoundaryOpacity");
					glUniform1f(isilbound,style->silhouetteBoundaryOpacity);
					isilretain = GET_UNIFORM(myProg,"fw_RetainedOpacity");
					glUniform1f(isilretain,style->silhouetteRetainedOpacity);
					isilsharp = GET_UNIFORM(myProg,"fw_Sharpness");
					glUniform1f(isilsharp,style->silhouetteSharpness);
				}
				break;
			case NODE_ToneMappedVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#ToneMappedVolumeStyle
					//SFColorRGBA [in,out] coolColor      0 0 1 0 [0,1]
					//SFColorRGBA [in,out] warmColor      1 1 0 0 [0,1]
					//SFNode      [in,out] surfaceNormals NULL    [X3DTexture3DNode]
					struct X3D_ToneMappedVolumeStyle *style = (struct X3D_ToneMappedVolumeStyle*)vstyle;
					//send warm, cool to shader
					GLint icool, iwarm;
					icool = GET_UNIFORM(myProg,"fw_coolColor");
					glUniform4fv(icool,1,style->coolColor.c);
					iwarm = GET_UNIFORM(myProg,"fw_warmColor");
					glUniform4fv(iwarm,1,style->warmColor.c);
				}
				break;
			default:
				break;
		}
	}
}
static struct {
const char *ctype;
int itype;
} blendfuncs [] = {
{"CONSTANT",1},
{"ALPHA1",2},
{"ALPHA2",3},
{"TABLE",4},
{"ONE_MINUS_ALPHA1",5},
{"ONE_MINUS_ALPHA2",6},
{NULL,0},
};
int lookup_blendfunc(const char *funcname){
	int iret, i;
	i = 0;
	iret = 0;
	do{
		if(!strcmp(blendfuncs[i].ctype,funcname)){
			iret = blendfuncs[i].itype;
			break;
		}
		i++;
	}while(blendfuncs[i].ctype);
	return iret;
}
void render_GENERIC_volume_data(s_shader_capabilities_t *caps, struct X3D_Node **renderStyle, int nstyle, struct X3D_Node *voxels, struct X3D_VolumeData *node );
s_shader_capabilities_t * getVolumeProgram(struct X3D_Node **renderStyle, int nstyle, int VOLUME_DATA_FLAG);
void fin_volumestyle(struct X3D_Node *vstyle, struct X3D_VolumeData *dataParent){
	struct X3D_OpacityMapVolumeStyle *style0 = (struct X3D_OpacityMapVolumeStyle*)vstyle;
	if(style0->enabled){
		switch(vstyle->_nodeType){
			case NODE_OpacityMapVolumeStyle:
				{
					ttglobal tg = gglobal();
					//do I need to do this?
					tg->RenderFuncs.textureStackTop = 0;
					tg->RenderFuncs.texturenode = NULL;
				}
			break;
			case NODE_BlendedVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#BlendedVolumeStyle
					struct X3D_BlendedVolumeStyle *style = (struct X3D_BlendedVolumeStyle*)vstyle;
					//FBO blending
					//a)  render the parent volumeData to fbo:
					//	- in prep Blended push fbo
					//	- render main to fbo
					//	- in fin Blended: read pixels or save renderbuffer texture 0
					//b)  in fin Blended: render blended (voxels,stye) as VolumeData to fbo
					//	- read pixels or same renderbuffer texture 1
					//c)   pop fbo
					//d)  set 2 pixelsets as textures
					//     and render via a special little shader that blends 2 textures and sends to GL_BACK. 
				#ifdef BLENDED
					GLuint pixelType = GL_RGBA;
					int *fbohandles = style->_fbohandles.p;
					if(fbohandles[0] > 0){
						//readpixels from parent volumedata render
						static int iframe = 0;
						iframe++;
						//FW_GL_READPIXELS (0,0,isize,isize,pixelType,GL_UNSIGNED_BYTE, ttip->texdata);
						if(1) if(iframe==1000){
							//write out whats in the framebuffer, and use as texture in test scene, to see fbo rendered OK
							textureTableIndexStruct_s ttipp, *ttip;
							ttip = &ttipp;
							GLint iviewport[4];
							glGetIntegerv(GL_VIEWPORT, iviewport); //xmin,ymin,w,h

							ttip->texdata = MALLOC (GLvoid *, 4*iviewport[2]*iviewport[3]);

							/* grab the data */
							//FW_GL_PIXELSTOREI (GL_UNPACK_ALIGNMENT, 1);
							//FW_GL_PIXELSTOREI (GL_PACK_ALIGNMENT, 1);
							
							// https://www.khronos.org/opengles/sdk/docs/man/xhtml/glReadPixels.xml
							FW_GL_READPIXELS (iviewport[0],iviewport[1],iviewport[2],iviewport[3],pixelType,GL_UNSIGNED_BYTE, ttip->texdata);
							ttip->x = iviewport[2];
							ttip->y = iviewport[3];
							ttip->z = 1;
							ttip->hasAlpha = 1;
							ttip->channels = 4;
							//write out tti as web3dit image files for diagnostic viewing, can use for BackGround node
							//void saveImage_web3dit(struct textureTableIndexStruct *tti, char *fname)
							char namebuf[100];
							sprintf(namebuf,"%s%d.web3dit","blended_fbo_",0);
							saveImage_web3dit(ttip, namebuf);
							FREE_IF_NZ(ttip->texdata);
						}
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbohandles[2], 0);
						//render blended as volumedata to fbo
						//render_volume_data(style->renderStyle,style->voxels,dataParent);
						s_shader_capabilities_t *caps = getVolumeProgram(NULL,0, SHADERFLAGS_VOLUME_DATA_BASIC);
						//render generic volume 
						render_GENERIC_volume_data(caps,NULL,0,style->voxels,(struct X3D_VolumeData*)dataParent );
						//render_GENERIC_volume_data(caps,style->renderStyle,1,style->voxels,(struct X3D_VolumeData*)dataParent );

						//read blended from fbo
						//FW_GL_READPIXELS (0,0,isize,isize,pixelType,GL_UNSIGNED_BYTE, ttip->texdata);
						if(1) if(iframe==1000){
							//write out whats in the framebuffer, and use as texture in test scene, to see fbo rendered OK
							textureTableIndexStruct_s ttipp, *ttip;
							ttip = &ttipp;
							GLint iviewport[4];
							glGetIntegerv(GL_VIEWPORT, iviewport); //xmin,ymin,w,h

							ttip->texdata = MALLOC (GLvoid *, 4*iviewport[2]*iviewport[3]);

							/* grab the data */
							//FW_GL_PIXELSTOREI (GL_UNPACK_ALIGNMENT, 1);
							//FW_GL_PIXELSTOREI (GL_PACK_ALIGNMENT, 1);
							// https://www.khronos.org/opengles/sdk/docs/man/xhtml/glReadPixels.xml
							FW_GL_READPIXELS (iviewport[0],iviewport[1],iviewport[2],iviewport[3],pixelType,GL_UNSIGNED_BYTE, ttip->texdata);
							ttip->x = iviewport[2];
							ttip->y = iviewport[3];
							ttip->z = 1;
							ttip->hasAlpha = 1;
							ttip->channels = 4;
							//write out tti as web3dit image files for diagnostic viewing, can use for BackGround node
							//void saveImage_web3dit(struct textureTableIndexStruct *tti, char *fname)
							char namebuf[100];
							sprintf(namebuf,"%s%d.web3dit","blended_fbo_",1);
							saveImage_web3dit(ttip, namebuf);
							FREE_IF_NZ(ttip->texdata);
						}
						popnset_framebuffer();

						//render 2 textures as blended multitexture, or in special shader for blending, 
						//2 textures are fbohandles[0] (parent voldata), fbohandles[1] (blend voldata)
						//over window-filling quad
						//Options: 
						//1) draw our cube again, to get the depth (and skip unneeded pixels)
						//   but use gl_fragCoords as texture interpolator
						//2) draw quad in ortho mode (but where depth buffer?)
						GLuint myProg;
						int method_draw_cube, method_draw_quad;
						method_draw_cube = method_draw_quad = 0;
						method_draw_cube = 1;
						if(method_draw_cube){
							shaderflagsstruct shaderflags, shader_requirements;
							s_shader_capabilities_t *caps;
							memset(&shader_requirements,0,sizeof(shaderflagsstruct));
							shader_requirements.volume = SHADERFLAGS_VOLUME_STYLE_BLENDED << 4; //send the following through the volume ubershader
							// by default we'll mash it in: shader_requirements.volume |= TEX3D_SHADER;
							caps = getMyShaders(shader_requirements);
							enableGlobalShader(caps);
							GLint myProg =  caps->myShaderProgram;

							//send the usual matrices - same vertex shader as volumedata
							//but simpler frag shader that uses gl_fragCoords, and does blend

							//set shader flags
							//build or get shader program
							//set attributes 
							//SFFloat  [in,out] weightConstant1         0.5        [0,1]
							//SFFloat  [in,out] weightConstant2         0.5        [0,1]
							//BRUTZMAN: ALPHA0,1 here should be ALPHA1,2 to match table 14.1
							//SFString [in,out] weightFunction1         "CONSTANT" ["CONSTANT", "ALPHA0", "ALPHA1", "TABLE",
							//													"ONE_MINUS_ALPHA0", "ONE_MINUS_ALPHA1" ] 
							//SFString [in,out] weightFunction2         "CONSTANT" ["CONSTANT", "ALPHA0", "ALPHA1", "TABLE",
							//													"ONE_MINUS_ALPHA0", "ONE_MINUS_ALPHA1" ] 
							//SFNode   [in,out] weightTransferFunction1 NULL       [X3DTexture2DNode]
							//SFNode   [in,out] weightTransferFunction2 NULL       [X3DTexture2DNode]

							GLint iwtc1, iwtc2, iwtf1, iwtf2;
							iwtc1 = GET_UNIFORM(myProg,"fw_iwtc1");
							iwtc2 = GET_UNIFORM(myProg,"fw_iwtc2");
							iwtf1 = GET_UNIFORM(myProg,"fw_iwtf1");
							iwtf2 = GET_UNIFORM(myProg,"fw_iwtf2");
							glUniform1f(iwtc1,style->weightConstant1);
							glUniform1f(iwtc2,style->weightConstant2);
							if(style->_weightFunction1 == 0)
								style->_weightFunction1 = lookup_blendfunc(style->weightFunction1->strptr);
							if(style->_weightFunction2 == 0)
								style->_weightFunction2 = lookup_blendfunc(style->weightFunction2->strptr);
							glUniform1i(iwtf1,style->_weightFunction1);
							glUniform1i(iwtf2,style->_weightFunction2);

							//set the 2 textures from the fbo rendering
							glActiveTexture ( GL_TEXTURE0 );
							glBindTexture(GL_TEXTURE_2D,style->_fbohandles.p[1]);
							glActiveTexture ( GL_TEXTURE0+1 );
							glBindTexture(GL_TEXTURE_2D,style->_fbohandles.p[2]);

							//set the 2 transfer function textures
							int havetextures;
							havetextures = 0;
							if(style->weightTransferFunction1){
								//load texture
								struct X3D_Node *tmpN;
								ttglobal tg = gglobal();

								POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, style->weightTransferFunction1,tmpN);
								tg->RenderFuncs.texturenode = (void*)tmpN;

								//problem: I don't want it sending image dimensions to my volume shader,
								// which could confuse the voxel sampler
								//render_node(tmpN); //render_node(node->texture); 
								loadTextureNode(tmpN,NULL);
								textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tmpN);
								if(tti && tti->status >= TEX_LOADED){
									glActiveTexture(GL_TEXTURE0+2); 
									glBindTexture(GL_TEXTURE_2D,tti->OpenGLTexture); 
									havetextures |= 1;
								}
							}
							if(style->weightTransferFunction2){
								//load texture
								struct X3D_Node *tmpN;
								ttglobal tg = gglobal();

								POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, style->weightTransferFunction2,tmpN);
								tg->RenderFuncs.texturenode = (void*)tmpN;

								//problem: I don't want it sending image dimensions to my volume shader,
								// which could confuse the voxel sampler
								//render_node(tmpN); //render_node(node->texture); 
								loadTextureNode(tmpN,NULL);
								textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tmpN);
								if(tti && tti->status >= TEX_LOADED){
									glActiveTexture(GL_TEXTURE0+3); 
									glBindTexture(GL_TEXTURE_2D,tti->OpenGLTexture); 
									havetextures |= 2;
								}
							}
							GLint iopactex;
							iopactex = GET_UNIFORM(myProg,"fw_haveTransfers");
							glUniform1i(iopactex,havetextures);


							//draw the box
							GLint Vertices = GET_ATTRIB(myProg,"fw_Vertex");
							glEnableVertexAttribArray(Vertices);
							glVertexAttribPointer(Vertices, 3, GL_FLOAT, GL_FALSE, 0, dataParent->_boxtris);
							glDrawArrays(GL_TRIANGLES,0,36); //36 vertices for box
							
						}else if(method_draw_quad){
							////we need a shader that doesn't bother with matrices - just draws quad like ortho
							//GLint Vertices = GET_ATTRIB(myProg,"fw_Vertex");
							//glEnableVertexAttribArray(Vertices);
							//glVertexAttribPointer(Vertices, 3, GL_FLOAT, GL_FALSE, 0, box);
							//glDrawArrays(GL_TRIANGLES,0,6); //6 vertices for quad
						}

					}
				#endif //BLENDED
				}
			case NODE_ComposedVolumeStyle:
				{
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#ComposedVolumeStyle
					struct X3D_ComposedVolumeStyle *style = (struct X3D_ComposedVolumeStyle*)vstyle;
					for(int i=0;i<style->renderStyle.n;i++){
						fin_volumestyle(style->renderStyle.p[i],dataParent);
					}
				}
				break;

		default:
			break;
		}
	}
}
int volstyle_needs_normal(struct X3D_Node *vstyle){
	//IDEA: compute image gradient and store in RGB, if a style requests it
	// then surfaceNormal = normalize(gradient)
	//SFNode [in,out] surfaceNormals NULL [X3DTexture3DNode
	// Cartoon
	// Edge
	// Shaded
	// SilhouetteEnhancement
	// ToneMappedVolumeStyle
	//
	//SFNode [in,out] gradients NULL [X3DTexture3DNode]
	// IsoSurfaceVolumeData
	//
	//SFNode [in,out] segmentIdentifiers NULL [X3DTexture3DNode]
	// SegmentedVolumeData
	int need_normal;
	struct X3D_OpacityMapVolumeStyle *style0 = (struct X3D_OpacityMapVolumeStyle*)vstyle;
	need_normal = FALSE;
	if(style0->enabled){
		switch(vstyle->_nodeType){
			case NODE_ComposedVolumeStyle:
				{
					struct X3D_ComposedVolumeStyle *style = (struct X3D_ComposedVolumeStyle*)vstyle;
					for(int i=0;i<style->renderStyle.n;i++){
						need_normal = need_normal || volstyle_needs_normal(style->renderStyle.p[i]);
					}
				}
				break;
			case NODE_CartoonVolumeStyle:
			case NODE_EdgeEnhancementVolumeStyle:
			case NODE_ShadedVolumeStyle:
			case NODE_SilhouetteEnhancementVolumeStyle:
			case NODE_ToneMappedVolumeStyle:
				{
					//in perl structs, for these nodes its all the 3rd field after enabled, metadata, surfacenormals
					struct X3D_ToneMappedVolumeStyle *style = (struct X3D_ToneMappedVolumeStyle*)vstyle;
					need_normal = need_normal || (style->surfaceNormals == NULL);
				}
				break;
			default:
				break;
		}
	}
	return need_normal;
}
void sendExplicitMatriciesToShader (GLint ModelViewMatrix, GLint ProjectionMatrix, GLint NormalMatrix, GLint *TextureMatrix, GLint ModelViewInverseMatrix);

void compile_IsoSurfaceVolumeData(struct X3D_IsoSurfaceVolumeData *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#IsoSurfaceVolumeData
	// VolumeData + 4 fields:
	//SFFloat [in,out] contourStepSize  0        (-INF,INF)
	//SFNode  [in,out] gradients        NULL     [X3DTexture3DNode]
	//SFFloat [in,out] surfaceTolerance 0        [0,INF)
	//MFFloat [in,out] surfaceValues    []       (-INF,INF)
	printf("compile_isosurfacevolumedata not implemented\n");
	compile_VolumeData((struct X3D_VolumeData *)node);
}



void compile_SegmentedVolumeData(struct X3D_SegmentedVolumeData *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#SegmentedVolumeData
	// VolumeData + 2 fields:
	//MFBool  [in,out] segmentEnabled     []
	//SFNode  [in,out] segmentIdentifiers NULL     [X3DTexture3DNode]
	printf("compile_segmentedvolumedata \n");
	compile_VolumeData((struct X3D_VolumeData *)node);
}
s_shader_capabilities_t * getVolumeProgram(struct X3D_Node **renderStyle, int nstyle, int VOLUME_DATA_FLAG){
	static int once = 0;
	unsigned int volflags;
	ttglobal tg = gglobal();

	if(!once)
		ConsoleMessage("getVolumeProgram\n");
	volflags = 0;
	if(nstyle){
		for(int i=0;i<nstyle;i++){
			struct X3D_OpacityMapVolumeStyle *style0 = (struct X3D_OpacityMapVolumeStyle*)renderStyle[i];
			if(style0->enabled){
				volflags = prep_volumestyle(renderStyle[i], volflags); //get shader flags
			}
		}
	}else{
		volflags = SHADERFLAGS_VOLUME_STYLE_DEFAULT;
	}

	if(!once){
		printf("volflags= ");
		for(int i=0;i<8;i++)
			printf("%d ",((volflags >> (8-i-1)*4) & 0xF)); //show 4 int
		printf("\n");
	}

	//render 
	//Step 1: set the 3D texture
	//if(node->voxels)
	//	render_node(node->voxels);
	//Step 2: get rays to cast: start point and direction vector for each ray to cast

	//method: use cpu math to compute a few uniforms so frag shader can do box intersections
	//http://prideout.net/blog/?p=64
	//- one step raycasting using gl_fragCoord
	//- we modified this general method to use gluUnproject math instead of focallength

	//Step 3: accumulate along rays and render opacity fragment in one step
	//GPU VERSION
	shaderflagsstruct shaderflags, shader_requirements;
	s_shader_capabilities_t *caps;
	int old_shape_way = 0;

	memset(&shader_requirements,0,sizeof(shaderflagsstruct));
	//shaderflags = getShaderFlags();
	shader_requirements.volume = VOLUME_DATA_FLAG; //SHADERFLAGS_VOLUME_DATA_BASIC; //send the following through the volume ubershader
	shader_requirements.volume |= (volflags << 4); //SHADERFLAGS_VOLUME_STYLE_OPACITY;
	//CLIPPLANES ?
	shader_requirements.base |= getShaderFlags().base & CLIPPLANE_SHADER; 
	// by default we'll mash it in: shader_requirements.volume |= TEX3D_SHADER;
	caps = getMyShaders(shader_requirements);
	enableGlobalShader(caps);
	GLint myProg =  caps->myShaderProgram;
	//Step 1: set the 3D texture
	once = 1;
	//return myProg;
	return caps; 
}

void render_SEGMENTED_volume_data(s_shader_capabilities_t *caps, struct X3D_Node *segmentIDs, int itexture, int *enabledIDs, int nIDs ) {
	int myProg;
	myProg = caps->myShaderProgram;
	if(segmentIDs){
		struct X3D_Node *tmpN;
		ttglobal tg = gglobal();

		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, segmentIDs,tmpN);
		tg->RenderFuncs.texturenode = (void*)tmpN;

		render_node(tmpN); //render_node(node->voxels); 

		textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tmpN);
		if(tti && tti->status >= TEX_LOADED){
			if(0){
				//in theory these will be set by the main voxel texture and should match
				GLint ttiles = GET_UNIFORM(myProg,"tex3dTiles");
				GLUNIFORM1IV(ttiles,3,tti->tiles);

				//me->tex3dUseVertex = GET_UNIFORM(myProg,"tex3dUseVertex");
				GLint tex3dUseVertex = GET_UNIFORM(myProg,"tex3dUseVertex");
				glUniform1i(tex3dUseVertex,0); 
				GLint repeatSTR = GET_UNIFORM(myProg,"repeatSTR");
				glUniform1iv(repeatSTR,3,tti->repeatSTR);
				GLint magFilter = GET_UNIFORM(myProg,"magFilter");
				glUniform1i(magFilter,tti->magFilter);
			}
			glActiveTexture(GL_TEXTURE0+itexture); 
			glBindTexture(GL_TEXTURE_2D,tti->OpenGLTexture); 
		}
	}
	GLint inids = GET_UNIFORM(myProg,"fw_nIDs");
	glUniform1i(inids,nIDs); 
	GLint ienable = GET_UNIFORM(myProg,"fw_enableIDs");
	glUniform1iv(ienable,nIDs,enabledIDs); 

}

float *getTransformedClipPlanes();
int getClipPlaneCount();
void sendFogToShader(s_shader_capabilities_t *me);
void render_GENERIC_volume_data(s_shader_capabilities_t *caps, struct X3D_Node **renderStyle, int nstyle, struct X3D_Node *voxels, struct X3D_VolumeData *node ) {
	static int once = 0;
	int myProg;
	unsigned int volflags;
	ttglobal tg = gglobal();

	myProg = caps->myShaderProgram;

	if(voxels){
		struct X3D_Node *tmpN;
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, voxels,tmpN);
		tg->RenderFuncs.texturenode = (void*)tmpN;

		//gradient > Oct 2016 we compute in textures.c if channels==1 and z>1 and put in rgb
		// - saves mallocing another RGBA 
		// - for scalar images RGB is unused or just 111 anyway
		// - takes 1 second on desktop CPU for 17 Mpixel image
		//if(node->renderStyle){
		//	if(volstyle_needs_normal(node->renderStyle)){
		//		switch(tmpN->_nodeType){
		//			case NODE_PixelTexture3D:
		//				((struct X3D_PixelTexture3D*)tmpN)->_needs_gradient = TRUE; break;
		//			case NODE_ImageTexture3D:
		//				((struct X3D_ImageTexture3D*)tmpN)->_needs_gradient = TRUE; break;
		//		}
		//	}
		//}
		//render_node(voxels) should keep pulling the texture through all stages of loading and opengl
		render_node(tmpN); //render_node(node->voxels); 

		textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tmpN);
		if(tti && tti->status >= TEX_LOADED){
			GLint ttiles = GET_UNIFORM(myProg,"tex3dTiles");
			GLUNIFORM1IV(ttiles,3,tti->tiles);

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
	if(nstyle){
		for(int i=0;i<nstyle;i++){
			struct X3D_OpacityMapVolumeStyle *style0 = (struct X3D_OpacityMapVolumeStyle*)renderStyle[i];
			if(style0->enabled){
				render_volumestyle(renderStyle[i],myProg); //send uniforms
				// if style uses a texture, it should be the next texture ie GL_TEXTURE0+1,2..
			}
		}
	}
	//3.1 set uniforms: dimensions, focal length, fov (field of view), window size, modelview matrix
	//    set attributes vertices of triangles of bounding box
	// set box with vol.dimensions with triangles
	GLint Vertices = GET_ATTRIB(myProg,"fw_Vertex");
	GLint mvm = GET_UNIFORM(myProg,"fw_ModelViewMatrix"); //fw_ModelViewMatrix
	GLint proj = GET_UNIFORM(myProg,"fw_ProjectionMatrix"); //fw_ProjectionMatrix
	if(!once)
		ConsoleMessage("vertices %d mvm %d proj %d\n",Vertices,mvm,proj);
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


//SEND CLIPPLANES?
	//sendClipplanesToShader(mysp);
	float *clipplanes = getTransformedClipPlanes();
	int nsend = getClipPlaneCount();
	GLint iclipplanes, inclipplanes;
	iclipplanes = GET_UNIFORM(myProg,"fw_clipplanes");
	inclipplanes = GET_UNIFORM(myProg,"fw_nclipplanes");

	GLUNIFORM4FV(iclipplanes,nsend,clipplanes);
	GLUNIFORM1I(inclipplanes,nsend);

//SEND LIGHTS IF WE HAVE A SHADER STYLE 
	//int haveShaderStyle = FALSE;
	//if(nstyle){
	//	for(int i=0;i<nstyle;i++){
	//		haveShaderStyle = haveShaderStyle || (renderStyle[i]->_nodeType == NODE_ShadedVolumeStyle); 
	//	}
	//}
	//if(haveShaderStyle){
		//send lights
		if (caps->haveLightInShader) {
			sendLightInfo(caps);
			sendFogToShader(caps);
		}
	//}



	//get the current viewport
	GLint iviewport[4];
	float viewport[4];
	glGetIntegerv(GL_VIEWPORT, iviewport); //xmin,ymin,w,h

	GLint vp = GET_UNIFORM(myProg,"fw_viewport");
	viewport[0] = iviewport[0]; //xmin
	viewport[1] = iviewport[1]; //ymin
	viewport[2] = iviewport[2]; //width
	viewport[3] = iviewport[3]; //height
	GLUNIFORM4F(vp,viewport[0],viewport[1],viewport[2],viewport[3]);
	GLint dim = GET_UNIFORM(myProg,"fw_dimensions");
	float *dimensions = node->dimensions.c;
	GLUNIFORM3F(dim,dimensions[0],dimensions[1],dimensions[2]);

	if(!once) ConsoleMessage("dim %d vp %d \n",dim,vp );

	//3.2 draw with shader
	glEnableVertexAttribArray(Vertices);
	glVertexAttribPointer(Vertices, 3, GL_FLOAT, GL_FALSE, 0, node->_boxtris);
	// https://www.opengl.org/wiki/Face_Culling
	glEnable(GL_CULL_FACE);
	//we want to draw only either back/far or front/near triangles, not both
	//so that we comput a ray only once.
	//and because we want to use clipplane (or frustum near side) to slice into
	//volumes, we want to make sure we are still getting ray fragments when slicing
	//so instead of drawing the front faces (which would slice away fragments/rays)
	//we want to draw only the far/back triangles so even when slicing, we'll get
	//fragment shader calls, and can compute rays.
	//assuming our triangles are defined CCW (normal)
	//setting front-face to GL_CW should ensure only the far/back triangles are rendered
	glFrontFace(GL_CW); 
	glDrawArrays(GL_TRIANGLES,0,36);
	glDisable(GL_CULL_FACE);
	if(voxels){
		tg->RenderFuncs.textureStackTop = 0;
		tg->RenderFuncs.texturenode = NULL;
	}
	if(nstyle){
		for(int i=0;i<nstyle;i++)
			fin_volumestyle(renderStyle[i],node);
	}
	once = 1;

} 

void child_SegmentedVolumeData(struct X3D_SegmentedVolumeData *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#SegmentedVolumeData
	// VolumeData + 2 fields:
	//MFBool  [in,out] segmentEnabled     []
	//SFNode  [in,out] segmentIdentifiers NULL     [X3DTexture3DNode]
	s_shader_capabilities_t *caps;
	static int once = 0;
	COMPILE_IF_REQUIRED

	if (renderstate()->render_blend == (node->_renderFlags & VF_Blend)) {

		if(!once)
			printf("child segmentedvolumedata \n");
		int nstyles = 0;
		if(node->renderStyle) nstyles = 1;

		caps = getVolumeProgram(&node->renderStyle,nstyles, SHADERFLAGS_VOLUME_DATA_SEGMENT);
		//get and set segment-specific uniforms
		int itexture = 1; //voxels=0,segmentIDs=1
		render_SEGMENTED_volume_data(caps,node->segmentIdentifiers,itexture,node->segmentEnabled.p,node->segmentEnabled.n);
		//render generic volume 
		render_GENERIC_volume_data(caps,&node->renderStyle,nstyles,node->voxels,(struct X3D_VolumeData*)node );
		once = 1;
	} //if VF_Blend

}
void render_ISO_volume_data(s_shader_capabilities_t *caps,struct X3D_IsoSurfaceVolumeData *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#IsoSurfaceVolumeData
	// VolumeData + 4 fields, minus 1 field
	//SFFloat [in,out] contourStepSize  0        (-INF,INF)
	//SFNode  [in,out] gradients        NULL     [X3DTexture3DNode]
	//SFFloat [in,out] surfaceTolerance 0        [0,INF)
	//MFFloat [in,out] surfaceValues    []       (-INF,INF)
	//MFNode  [in,out] renderStyle      []       [X3DVolumeRenderStyleNode]
	//minus SFNode renderStyle
	int myProg = caps->myShaderProgram;
	GLint istep = GET_UNIFORM(myProg,"fw_stepSize");
	glUniform1f(istep,node->contourStepSize); 
	GLint itol = GET_UNIFORM(myProg,"fw_tolerance");
	glUniform1f(itol,node->surfaceTolerance); 

	GLint ivals = GET_UNIFORM(myProg,"fw_surfaceVals");
	glUniform1fv(ivals,node->surfaceValues.n,node->surfaceValues.p); 
	GLint invals = GET_UNIFORM(myProg,"fw_nVals");
	glUniform1i(invals,node->surfaceValues.n);
	if(node->renderStyle.n){
		int *styleflags = MALLOC(int*,sizeof(int)*node->renderStyle.n);
		for(int i=0;i<node->renderStyle.n;i++){
			styleflags[i] = prep_volumestyle(node->renderStyle.p[i],0);
		}
		GLint istyles = GET_UNIFORM(myProg,"fw_surfaceStyles");
		glUniform1iv(ivals,node->renderStyle.n,styleflags);
		int instyles = GET_UNIFORM(myProg,"fw_nStyles");
		glUniform1i(instyles,node->renderStyle.n);
	}

	//renderstyle handling?
	//Options:
	// a) include all renderstyles in the shader
	// b) go through the list and |= (multiple times should show up as once?)
	// then make a special shader to (equivalent to switch-case) on each voxel/gradient after iso value has been computed
}

void child_IsoSurfaceVolumeData(struct X3D_IsoSurfaceVolumeData *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#IsoSurfaceVolumeData
	// VolumeData + 4 fields:
	//SFFloat [in,out] contourStepSize  0        (-INF,INF)
	//SFNode  [in,out] gradients        NULL     [X3DTexture3DNode]
	//SFFloat [in,out] surfaceTolerance 0        [0,INF)
	//MFFloat [in,out] surfaceValues    []       (-INF,INF)
	static int once = 0;
	COMPILE_IF_REQUIRED
	if (renderstate()->render_blend == (node->_renderFlags & VF_Blend)) {
		unsigned int voldataflags;
		s_shader_capabilities_t *caps;

		if(!once)
			printf("child segmentedvolumedata \n");
		voldataflags = SHADERFLAGS_VOLUME_DATA_ISO;

		int MODE;
		MODE = node->surfaceValues.n == 1 ? 1 : 3;
		MODE = node->contourStepSize != 0.0f && MODE == 1 ? 2 : 1;
		if(MODE == 3)
			voldataflags |= SHADERFLAGS_VOLUME_DATA_ISO_MODE3;
		caps = getVolumeProgram(node->renderStyle.p,node->renderStyle.n, voldataflags);
		//get and set ISO-specific uniforms
		int itexture = 1; //voxels=0,segmentIDs=1
		render_ISO_volume_data(caps,node);
		//render generic volume 
		render_GENERIC_volume_data(caps,node->renderStyle.p,node->renderStyle.n,node->voxels,(struct X3D_VolumeData*)node );
		once = 1;
	} //if VF_Blend
}

void child_VolumeData(struct X3D_VolumeData *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/volume.html#VolumeData
	// VolumeData 
	s_shader_capabilities_t *caps;
	static int once = 0;
	COMPILE_IF_REQUIRED

	if (renderstate()->render_blend == (node->_renderFlags & VF_Blend)) {

		if(!once)
			printf("child volumedata \n");
		int nstyles = 0;
		if(node->renderStyle) nstyles = 1;
		caps = getVolumeProgram(&node->renderStyle,nstyles, SHADERFLAGS_VOLUME_DATA_BASIC);
		//render generic volume 
		render_GENERIC_volume_data(caps,&node->renderStyle,nstyles,node->voxels,(struct X3D_VolumeData*)node );
		once = 1;
	} //if VF_Blend

}
