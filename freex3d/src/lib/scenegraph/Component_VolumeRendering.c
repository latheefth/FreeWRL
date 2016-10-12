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

// START MIT >>>>>>>>>>>>
//=========== CPU EMULATOR / SOFTWARE RENDERER FOR GLSL SHADER PROGRAMS IN C >>>>>>>>>>>>>>>>>>>>>>>>>
/*	goal: be able to set breakpoints, printf and generally debug complex algorithms destined for shaders
	implementation: because in flat C, no OO shader objects, so syntax for working with vectors, matrices
	is functional ie 
	vec3 p = q.yyz;
	becomes 
	vec3 p = vec3swiz(q,"yyz");
	And that means you need to re-write each line of a GPU shader (or vice versa going the other way).
	And that means if you have both GPU and CPU versions, you need to maintain 2 things to keep them in sync.
	Which is tedious and error prone.
	Which means you can do a lot of permutation tinkering to get your GPU shaders working for the same 
	time and effort as converting to CPU.
	So not recommended in general, but more for when you're getting no where tinkering or new 
	algorithms destined for shaders are complex	and need to be tested in detail - this might help.

	Links:
	https://en.wikibooks.org/wiki/GLSL_Programming/Vector_and_Matrix_Operations
	- GLSL vector and matrix syntax
	http://glm.g-truc.net/0.9.8/index.html
	- C++ GLM: GL Mathematics - has syntax very close to GLSL, wish I had done it with this in .cpp
	https://www.opengl.org/wiki/Rendering_Pipeline_Overview
	- rendering steps in pipeline -especially after vertex shader and before frag shader 

*/
int swizindex(char s){
	int kret;
	switch(s){
		case 'x':
		case 's':
		case 'r':
			kret = 0; break;
		case 'y':
		case 't':
		case 'g':
			kret = 1; break;
		case 'z': 
		case 'u':
		case 'b':
			kret = 2; break;
		case 'w': 
		case 'v':
		case 'a':
			kret = 3; break;
		default:
			kret = 0; break;
	}
	return kret;
}

typedef union {
	struct { float x; float y; };
	struct {float s; float t; };
	struct {float r; float g; };
	float data[2];
} vec2;
vec2 vec2new2(float x, float y ){
	vec2 ret;
	ret.x = x; 
	ret.y = y;
	return ret;
}
vec2 vec2new1(float x ){
	return vec2new2(x,x);
}

vec2 vec2mul(vec2 a, vec2 b){
	vec2 ret;
	ret.x = a.x * b.x;
	ret.y = a.y * b.y;
	return ret;
}
vec2 vec2div(vec2 a, vec2 b){
	vec2 ret;
	ret.x = a.x / b.x;
	ret.y = a.y / b.y;
	return ret;
}
vec2 vec2dif(vec2 a, vec2 b){
	vec2 ret;
	ret.x = a.x - b.x;
	ret.y = a.y - b.y;
	return ret;
}
vec2 vec2add(vec2 a, vec2 b){
	vec2 ret;
	ret.x = a.x + b.x;
	ret.y = a.y + b.y;
	return ret;
}
vec2 vec2inv(vec2 a){
	vec2 ret;
	ret.x = 1.0 / a.x;
	ret.y = 1.0 / a.y;
	return ret;
}
float vec2length(vec2 a){
	return sqrt(a.x*a.x + a.y*a.y);
}
vec2 vec2scale(vec2 a, float s){
	vec2 ret;
	ret = a;
	ret.x *= s;
	ret.y *= s;
	return ret;
}
vec2 vec2normalize(vec2 a){
	vec2 ret;
	float d = 1.0/vec2length(a);
	return vec2scale(a,d);
}
vec2 vec2min(vec2 a, vec2 b){
	vec2 ret;
	ret.x = min(a.x, b.x);
	ret.y = min(a.y, b.y);
	return ret;
}
vec2 vec2max(vec2 a, vec2 b){
	vec2 ret;
	ret.x = max(a.x, b.x);
	ret.y = max(a.y, b.y);
	return ret;
}
vec2 vec2floor(vec2 a){
	vec2 ret = a;
	ret.x = floor(ret.x);
	ret.y = floor(ret.y);
	return ret;
}
vec2 vec2ceil(vec2 a){
	vec2 ret = a;
	ret.x = ceil(ret.x);
	ret.y = ceil(ret.y);
	return ret;
}
vec2 vec2swiz(vec2 a, char *ss){
	vec2 ret;
	ret = a;
	for(int i=0;i<strlen(ss);i++){
		switch(ss[i]){
			case 'x':
			case 's':
				ret.data[i] = a.x; break;
			case 'y':
			case 't':
				ret.data[i] = a.y; break;
		}
	}	
	return ret;
}

void vec2swizcpy(vec2 *a, char *ssa, vec2 b, char *ssb){
	int i,ka,kb;
	for(i=0;i<min(strlen(ssb),strlen(ssa));i++){
		ka = swizindex(ssa[i]);
		kb = swizindex(ssb[i]);
		(*a).data[ka] = b.data[kb];
	}
}

typedef union {
	struct { float x; float y; float z;	};
	struct {float s; float t; float u;};
	struct {float r; float g; float b;};
	float data[3];
} vec3;
vec3 vec3new3(float x, float y, float z ){
	vec3 ret;
	ret.x = x; 
	ret.y = y;
	ret.z = z;
	return ret;
}
vec3 vec3new1(float x ){
	return vec3new3(x,x,x);
}

vec3 vec3mul(vec3 a, vec3 b){
	vec3 ret;
	ret.x = a.x * b.x;
	ret.y = a.y * b.y;
	ret.z = a.z * b.z;
	return ret;
}
vec3 vec3div(vec3 a, vec3 b){
	vec3 ret;
	ret.x = a.x / b.x;
	ret.y = a.y / b.y;
	ret.z = a.z / b.z;
	return ret;
}
vec3 vec3dif(vec3 a, vec3 b){
	vec3 ret;
	ret.x = a.x - b.x;
	ret.y = a.y - b.y;
	ret.z = a.z - b.z;
	return ret;
}
vec3 vec3cross(vec3 a, vec3 b){
	//cross product
	vec3 ret;
	ret.x = a.y * b.z - a.z * b.y;
	ret.y = a.z * b.x - a.x * b.z;
	ret.z = a.x * b.y - a.y * b.x;
	return ret;
}
vec3 vec3add(vec3 a, vec3 b){
	vec3 ret;
	ret.x = a.x + b.x;
	ret.y = a.y + b.y;
	ret.z = a.z + b.z;
	return ret;
}
vec3 vec3inv(vec3 a){
	vec3 ret;
	ret.x = 1.0 / a.x;
	ret.y = 1.0 / a.y;
	ret.z = 1.0 / a.z;
	return ret;
}
float vec3length(vec3 a){
	return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}
vec3 vec3scale(vec3 a, float s){
	vec3 ret;
	ret = a;
	ret.x *= s;
	ret.y *= s;
	ret.z *= s;
	return ret;
}
vec3 vec3normalize(vec3 a){
	float d = 1.0/vec3length(a);
	return vec3scale(a,d);
}
vec3 vec3min(vec3 a, vec3 b){
	vec3 ret;
	ret.x = min(a.x, b.x);
	ret.y = min(a.y, b.y);
	ret.z = min(a.z, b.z);
	return ret;
}
vec3 vec3max(vec3 a, vec3 b){
	vec3 ret;
	ret.x = max(a.x, b.x);
	ret.y = max(a.y, b.y);
	ret.z = max(a.z, b.z);
	return ret;
}
vec3 vec3floor(vec3 a){
	vec3 ret = a;
	ret.x = floor(ret.x);
	ret.y = floor(ret.y);
	ret.z = floor(ret.z);
	return ret;
}
vec3 vec3ceil(vec3 a){
	vec3 ret = a;
	ret.x = ceil(ret.x);
	ret.y = ceil(ret.y);
	ret.z = ceil(ret.z);
	return ret;
}
vec3 vec3clamp(vec3 a,float vmin, float vmax){
	vec3 ret = a;
	ret.x = max(vmin,min(vmax,a.x));
	ret.y = max(vmin,min(vmax,a.y));
	ret.z = max(vmin,min(vmax,a.z));
	return ret;
}
vec3 vec3swiz(vec3 a, char *ss){
	vec3 ret;
	ret = a;
	for(int i=0;i<strlen(ss);i++){
		switch(ss[i]){
			case 'x':
			case 's':
			case 'r':
				ret.data[i] = a.x; break;
			case 'y':
			case 't':
			case 'g':
				ret.data[i] = a.y; break;
			case 'z': 
			case 'u':
			case 'b':
				ret.data[i] = a.z; break;
		}
	}	
	return ret;
}

void vec3swizcpy(vec3 *a, char *ssa, vec3 b, char *ssb){
	int i,ka,kb;
	for(i=0;i<min(strlen(ssb),strlen(ssa));i++){
		ka = swizindex(ssa[i]);
		kb = swizindex(ssb[i]);
		(*a).data[ka] = b.data[kb];
	}
}
typedef union vec4 {
	struct { float x; float y; float z; float w; };
	struct {float s; float t; float u; float v; };
	struct {float r; float g; float b; float a; };
	float data[4];
} vec4;
vec4 vec4new4(float x, float y, float z, float w ){
	vec4 ret;
	ret.x = x; 
	ret.y = y;
	ret.z = z;
	ret.w = w;
	return ret;
}
vec4 vec4new2(vec3 a, float w ){
	return vec4new4(a.x,a.y,a.z,w);
}
vec4 vec4new1(float v ){
	return vec4new4(v,v,v,v);
}
vec4 vec4mul(vec4 a, vec4 b){
	vec4 ret;
	ret.x = a.x * b.x;
	ret.y = a.y * b.y;
	ret.z = a.z * b.z;
	ret.w = a.w * b.w;
	return ret;
}
vec4 vec4div(vec4 a, vec4 b){
	vec4 ret;
	ret.x = a.x / b.x;
	ret.y = a.y / b.y;
	ret.z = a.z / b.z;
	ret.w = a.w / b.w;
	return ret;
}
vec4 vec4dif(vec4 a, vec4 b){
	vec4 ret;
	ret.x = a.x - b.x;
	ret.y = a.y - b.y;
	ret.z = a.z - b.z;
	ret.w = a.w - b.w;
	return ret;
}
vec4 vec4add(vec4 a, vec4 b){
	vec4 ret;
	ret.x = a.x + b.x;
	ret.y = a.y + b.y;
	ret.z = a.z + b.z;
	ret.w = a.w + b.w;
	return ret;
}
vec4 vec4scale(vec4 a, float s){
	vec4 ret;
	ret = a;
	ret.x *= s;
	ret.y *= s;
	ret.z *= s;
	ret.w *= s;
	return ret;
}
vec4 vec4inv(vec4 a){
	vec4 ret;
	ret.x = 1.0 / a.x;
	ret.y = 1.0 / a.y;
	ret.z = 1.0 / a.z;
	ret.w = 1.0 / a.w;
	return ret;
}
vec4 vec4min(vec4 a, vec4 b){
	vec4 ret;
	ret.x = min(a.x, b.x);
	ret.y = min(a.y, b.y);
	ret.z = min(a.z, b.z);
	ret.w = min(a.w, b.w);
	return ret;
}
vec4 vec4max(vec4 a, vec4 b){
	vec4 ret;
	ret.x = max(a.x, b.x);
	ret.y = max(a.y, b.y);
	ret.z = max(a.z, b.z);
	ret.w = max(a.w, b.w);
	return ret;
}
vec4 vec4floor(vec4 a){
	vec4 ret = a;
	ret.x = floor(ret.x);
	ret.y = floor(ret.y);
	ret.z = floor(ret.z);
	ret.w = floor(ret.w);
	return ret;
}
vec4 vec4ceil(vec4 a){
	vec4 ret = a;
	ret.x = ceil(ret.x);
	ret.y = ceil(ret.y);
	ret.z = ceil(ret.z);
	ret.w = ceil(ret.w);
	return ret;
}
vec4 vec4swiz(vec4 a, char *ss){
	vec4 ret;
	ret = a;
	for(int i=0;i<strlen(ss);i++){
		switch(ss[i]){
			case 'x':
			case 's':
			case 'r':
				ret.data[i] = a.x; break;
			case 'y':
			case 't':
			case 'g':
				ret.data[i] = a.y; break;
			case 'z': 
			case 'u':
			case 'b':
				ret.data[i] = a.z; break;
			case 'w': 
			case 'v':
			case 'a':
				ret.data[i] = a.w; break;
		}
	}	
	return ret;
}
void vec4swizcpy(vec4 *a, char *ssa, vec4 b, char *ssb){
	int i,ka,kb;
	for(i=0;i<min(strlen(ssb),strlen(ssa));i++){
		ka = swizindex(ssa[i]);
		kb = swizindex(ssb[i]);
		(*a).data[ka] = b.data[kb];
	}
}
vec3 vec3from4(vec4 a){
	return vec3new3(a.x,a.y,a.z);
}
vec3 vec3from4homog(vec4 a){
	vec3 ret;
	ret = vec3from4(a);
	ret = vec3scale(ret,1.0/a.w);
	return ret;
}
vec4 vec4from4homog(vec4 a){
	vec4 ret;
	ret = vec4scale(a,1.0/a.w);
	ret.w = 1.0;
	return ret;
}
vec2 vec2from4swiz(vec4 a, char *ssa){
	vec2 ret;
	int i,ka;
	for(i=0;i<strlen(ssa);i++){
		ka = swizindex(ssa[i]);
		ret.data[i] = a.data[ka];
	}
	return ret;
}
vec2 vec2from3(vec3 a){
	vec2 ret;
	ret.x = a.x;
	ret.y = a.y;
	return ret;
}
typedef float mat4 [16];
vec4 vec4mulmat4(vec4 a, mat4 m){
	vec4 ret;
	vecmultmat4f(ret.data,a.data,m);
	return ret;
}
vec4 mat4mulvec4(mat4 m,vec4 a){
	vec4 ret;
	matmultvec4f(ret.data,m,a.data);
	return ret;
}
int ifloor(float v){
	int ret = (int)floor(v);
	return ret;
}
int iceil(float v){
	int ret;
	ret = (int)ceil(v);
	return ret;
}

#define sampler3D int
textureTableIndexStruct_s *getTableTableFromTextureNode(struct X3D_Node *textureNode);
static int sampler_count = 0;
vec4 texture3D(sampler3D tunit, vec3 tcoord){
	vec4 ret;
	struct X3D_Node *tnode;
	ttglobal tg = gglobal();
	tnode = tg->RenderFuncs.texturenode;

	ret = vec4new1(0.0);
	if(isTex3D(tnode)){
		textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tnode);
		vec3 imcoord;
		vec3 tclamped;
		tclamped = vec3clamp(tcoord,0.0,1.0);

		imcoord.x = tclamped.x * (tti->x -1);
		imcoord.y = tclamped.y * (tti->y -1);
		imcoord.z = tclamped.z * (tti->z -1);
		//printf("imcoord x %f y %f z %f\n",imcoord.x,imcoord.y,imcoord.z);
		int xrange[2],yrange[2],zrange[2];
		float xyzweight[3];
		int samples[2][2][2];
		int imagesize[3];
		imagesize[0] = tti->x;
		imagesize[1] = tti->y;
		imagesize[2] = tti->z;
		unsigned int *td = (unsigned int *)tti->texdata;
		if(td != NULL){
		
			xrange[0] = ifloor(imcoord.x);
			xrange[1] = iceil(imcoord.x);
			yrange[0] = ifloor(imcoord.y);
			yrange[1] = iceil(imcoord.y);
			zrange[0] = ifloor(imcoord.z);
			zrange[1] = iceil(imcoord.z);
			vec3 xyz = vec3dif(imcoord,vec3floor(imcoord));

			int i,j,k,l,ii,jj,kk;
			for(i=0;i<2;i++){
				ii = zrange[i];
				if(ii < 0 || ii >= imagesize[2]) printf("ouch image z %d\n",ii);
				else
				for(j=0;j<2;j++){
					jj = yrange[j];
					if(jj < 0 || jj >= imagesize[1]) 
						printf("ouch image y %d\n",jj);
					else
					for(k=0;k<2;k++){
						unsigned int pixel;
						unsigned char* rgba;
						kk = xrange[k];
						if(kk < 0 || kk >= imagesize[0]) printf("ouch image x %d\n",kk);
						else{
							int ipixel = ii*imagesize[1]*imagesize[0] + jj*imagesize[0] + kk;
							pixel = td[ipixel];
							//rgba = (unsigned int *)&pixel;
							samples[k][j][i] = pixel; //(int)rgba[3];
							sampler_count++;
						}
					}
				}
			}
			//weight the 8 samples
			float uchar2float = 1.0 / 255.0;
			if(0){
				//for now I'll just take one
				unsigned char *rgba = (unsigned char *)&samples[1][1][1];

				ret.r = uchar2float*(float)(int)rgba[0]; 
				ret.g = uchar2float*(float)(int)rgba[1];
				ret.b = uchar2float*(float)(int)rgba[2];
				ret.a = uchar2float*(float)(int)rgba[3];
			}else{
				//should be a weighted grid interpolation
				//https://en.wikipedia.org/wiki/Bilinear_interpolation
				//-bilinear
				//http://paulbourke.net/miscellaneous/interpolation/
				//-trilinear
				//Vxyz = 	V000 (1 - x) (1 - y) (1 - z) +
				//V100 x (1 - y) (1 - z) +
				//V010 (1 - x) y (1 - z) +
				//V001 (1 - x) (1 - y) z +
				//V101 x (1 - y) z +
				//V011 (1 - x) y z +
				//V110 x y (1 - z) +
				//V111 x y z 
				vec4 frgba = vec4new1(0.0);
				float xw,yw,zw;
				for(i=0;i<2;i++){
					zw = xyz.z;
					if(!i) zw = (1.0f - zw);
					for(j=0;j<2;j++){
						yw = xyz.y;
						if(!j) yw = (1.0f - yw);
						for(k=0;k<2;k++){
							xw = xyz.x;
							if(!k) xw = (1.0f - xw);
							unsigned char *rgba = (unsigned char *)&samples[k][j][i];
							for(l=0;l<4;l++){
								frgba.data[l] += xw*yw*zw*uchar2float*(float)(int)rgba[l];
							}
						}
					}
				}
				ret = frgba;
				//if(ret.a == 0.0) printf(".");
			}
		} //td != NULL
	}
	return ret;
}

#define uniform
#define varying
#define attribute


//bulitin vertex
vec4 gl_Position;

//builtin fragment
vec2 gl_FragCoord;
vec4 gl_FragColor;



//<<<<< END CPU EMULATOR FOR SHADER PROGRAMS =============

// =======SPECIFIC SHADER EXAMPLE >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// VERTEX ==========================================

//bulitins
//vec4 gl_Position;

//uniforms
uniform mat4 fw_ModelViewMatrix;
uniform mat4 fw_ProjectionMatrix;
//uniform vec4 fw_UnlitColor;
//attributes
attribute vec4 fw_Vertex;

/* PLUG-DECLARATIONS */

//varying
varying vec3 vertex_model; 
varying vec4 castle_vertex_eye;
varying vec4 castle_Color; 


void main_vertex(void)
{
  vec4 vertex_object = fw_Vertex;

  castle_vertex_eye = mat4mulvec4(fw_ModelViewMatrix,vertex_object);

   castle_Color = vec4new4(1.0,.5,.5,1.0);

  gl_Position = mat4mulvec4(fw_ProjectionMatrix,castle_vertex_eye);
  //#ifdef XYZ
  vec4swizcpy(&castle_Color,"rgb",gl_Position,"xyz");
  vertex_model = vec3from4(fw_Vertex);
  //#endif

}
// 

// FRAGMENT ==========================================

//builtin
//vec2 gl_FragCoord;
//vec4 gl_FragColor;

//varying
//varying vec3 vertex_model;
//varying vec4 castle_vertex_eye;
//varying vec4 castle_Color;

//uniform
//uniform mat4 fw_ModelViewMatrix;
//uniform mat4 fw_ProjectionMatrix;
uniform float fw_FocalLength;
uniform vec4 fw_viewport;
uniform vec3 fw_dimensions;
uniform vec3 fw_RayOrigin;
sampler3D fw_Texture_unit0;

uniform int tex3dDepth;
uniform int repeatSTR[3];
uniform int magFilter;


typedef struct Ray {
  vec3 Origin;
  vec3 Dir;
} Ray;
typedef struct AABB {
  vec3 Min;
  vec3 Max;
} AABB;


bool IntersectBox(Ray r, AABB aabb, float *t0, float *t1)
{
    vec3 invR = vec3inv(r.Dir);
    vec3 tbot = vec3mul(invR,vec3dif(aabb.Min,r.Origin));
    vec3 ttop = vec3mul(invR,vec3dif(aabb.Max,r.Origin));
    vec3 tmin = vec3min(ttop, tbot);
    vec3 tmax = vec3max(ttop, tbot);
    vec3 t = vec3max(vec3swiz(tmin,"xx"), vec3swiz(tmin,"yz"));
    *t0 = max(t.x, t.y);
    t = vec3min(vec3swiz(tmax,"xx"), vec3swiz(tmax,"yz"));
    *t1 = min(t.x, t.y);
    return t0 <= t1;
}
/* PLUG-DECLARATIONS */

vec3 fw_TexCoord[1];
void main_fragment(void)
{
	float maxDist = 1.414214; //sqrt(2.0);
	float densityFactor = 5.0;
	float Absorption = 1.0;
	int numSamples = 128;
	float fnumSamples = (float)numSamples;
	float stepSize = maxDist/fnumSamples;
	
    vec4 fragment_color;
	vec4 fragment_color_main = castle_Color;
    vec3 rayDirection;
	vec2 temp2;
    temp2 = vec2scale( vec2dif(vec2div(gl_FragCoord,vec2from4swiz(fw_viewport,"zw")),vec2new1(1.0)),2.0);
	rayDirection = vec3new3(temp2.x,temp2.y,1.0);
    rayDirection.z = -fw_FocalLength;
    rayDirection = vec3from4(vec4mulmat4(vec4new2(rayDirection, 0),fw_ModelViewMatrix));

    Ray eye = { fw_RayOrigin, vec3normalize(rayDirection) };
    AABB aabb = {vec3new1(-1.0), vec3new1(+1.0)};

    float tnear, tfar;
    IntersectBox(eye, aabb, &tnear, &tfar);
    if (tnear < 0.0) tnear = 0.0;

    vec3 rayStart = vec3add(eye.Origin, vec3scale(eye.Dir,tnear));
    vec3 rayStop = vec3add(eye.Origin, vec3scale(eye.Dir,tfar));
    // Transform from object space to texture coordinate space:
    rayStart = vec3scale( vec3add(rayStart, vec3new1(1.0)), .5);
    rayStop = vec3scale(vec3add(rayStop,vec3new1(1.0)),.5);

    // Perform the ray marching:
    vec3 pos = rayStart;
    vec3 step = vec3scale(vec3normalize(vec3dif(rayStop,rayStart)),stepSize);
    float travel = vec3length(vec3dif(rayStop, rayStart));
    float T = 1.0;
    vec3 Lo = vec3new1(0.0);
	vec3 normal_eye_fragment = vec3new1(0.0); //not used in plug
	fragment_color.a = 1.0;
	if(travel <= 0.0) fragment_color = vec4new4(.5,.5,.5,1.0);
	//(numSamples <= 0) fragment_color = vec4new4(.1,.5,.1,1.0);
	//numSamples = 0;
	fw_TexCoord[0] = pos; //vertex_model; //vec3(.2,.2,.5);
	fragment_color = vec4new1(1.0);
	//fragment_color = texture2D(fw_Texture_unit0,fw_TexCoord[0].st);
	/* PL_UG: texture_apply (fragment_color, normal_eye_fragment) */
	fragment_color = texture3D(fw_Texture_unit0,fw_TexCoord[0]);
	fragment_color_main = fragment_color;
	//fragment_color_main.a = 1.0;

    for (int i=0; i < numSamples; ++i) {
       // ...lighting and absorption stuff here...
		fragment_color = vec4new1(1.0);
		fw_TexCoord[0] = pos;
		/* PLUG: texture_apply (fragment_color, normal_eye_fragment) */
		fragment_color = texture3D(fw_Texture_unit0,fw_TexCoord[0]);
        //float density = texture3D(Density, pos).x * densityFactor;
		float density = fragment_color.a * densityFactor;

        if (density <= 0.0)
            continue;

        T *= 1.0-density*stepSize*Absorption;
		fragment_color_main = fragment_color;
		fragment_color_main.a = 1.0 - T;
        if (T <= 0.01) {
            break;
		}
		travel -= stepSize;
		if(travel <= 0.0) break;
		pos = vec3add(pos,step);

    } 
	gl_FragColor = fragment_color_main;
}

// PIPELINE - connects vertex to fragment
float area_of_triangle(vec3 Q, vec3 R, vec3 S){
	// https://en.wikibooks.org/wiki/GLSL_Programming/Rasterization
	vec3 QR = vec3dif(R,Q);
	vec3 QS = vec3dif(S,Q);
	vec3 cross = vec3cross(QR,QS);
	float d = vec3length(cross);
	return d * .5;
}
vec4 vec4triterp(vec3 alphas, vec4 *trivals){
	//given 3 alphas, and 3 vec4s, interpolate
	int i;
	vec4 ret = vec4new1(0.0);
	for(i=0;i<3;i++)
		ret = vec4add(ret,vec4scale(trivals[i],alphas.data[i]));
	return ret;
}
vec3 vec3triterp(vec3 alphas, vec3 *trivals){
	//given 3 alphas, and 3 vec3s, interpolate
	int i;
	vec3 ret = vec3new1(0.0);
	for(i=0;i<3;i++)
		ret = vec3add(ret,vec3scale(trivals[i],alphas.data[i]));
	return ret;
}
void saveImage_web3dit(struct textureTableIndexStruct *tti, char *fname);
#define isign(a) ( ( (a) < 0 )  ?  -1   : ( (a) > 0 ) ? 1 : 0 )
static unsigned char *imblob = NULL;
void cpu_drawtriangles(float *vertices, int nvertices){
	// this is a mini opengl rendering pipeline that runs shader code converted to run on the cpu aka cpu-shaders
	// https://www.opengl.org/wiki/Rendering_Pipeline_Overview
	// x no depth
	// x just a few triangles per frame, enough to exercise cpu-shaders

	int itri, jvert, imsize[2];
	vec2 vpmin = vec2from4swiz(fw_viewport,"xy");
	vec2 vpsize = vec2from4swiz(fw_viewport,"zw");
	vec2 vpmax = vec2add(vpmin,vpsize);
	imsize[0] = iceil(vpsize.x);
	imsize[1] = iceil(vpsize.y);

	//malloc a 'framebuffer' as a binary large object (blob)
	if(!imblob) imblob = malloc(4 * imsize[0] * imsize[1]);

	static int framecount = 0;
	framecount++;
	if(framecount == 1500){
		//glClear 'framebuffer' to background color black
		memset(imblob,0,4* imsize[0] * imsize[1]);

		sampler_count = 0;
		for(int itri=0;itri<nvertices/3;itri++){
			//builtins as array of 3
			vec4 gl_Vertexes[3];
			vec3 gl_FragCoords[3];
			vec4 gl_Positions[3];
			vec4 ndcs[3];
			//declare your varying here in arrays of 3
			vec4 castle_Colors[3];
			vec4 castle_vertex_eyes[3];
			vec3 vertex_models[3];

			for(int jvert=0;jvert<3;jvert++){
				float *point = &vertices[itri*3 + jvert];
				fw_Vertex = vec4new4(point[0],point[1],point[2],1.0f);
				gl_Vertexes[jvert] = fw_Vertex;
				main_vertex(); //run vertex cpu-shader
				//after vertex 
				//https://www.opengl.org/wiki/Vertex_Post-Processing
				//- accumulate triangle corners
				// https://www.opengl.org/sdk/docs/man/html/gl_FragCoord.xhtml
				vec4 ndc = vec4from4homog(gl_Position); //does perspective divide
				ndcs[jvert] = ndc;
				vec3 window;
				window.x = (vpsize.x *.5 * ndc.x) + ndc.x + (vpsize.x *.5);
				window.y = (vpsize.y *.5 * ndc.y) + ndc.y + (vpsize.y *.5);
				window.z = ((10000.0-.1)*.5 * ndc.z) + ((10000.0+.1) *.5);
				gl_FragCoords[jvert] = window;
				gl_Positions[jvert] = gl_Position;
				// copy per-vertex varyings by triangle corner - snapshot so we can over-write per-frag below
				castle_Colors[jvert] = castle_Color;
				castle_vertex_eyes[jvert] = castle_vertex_eye;
				vertex_models[jvert] = vertex_model;
			}
			//after 3 vertices / triangle:

			//-check ccw frontface vs cw backface for backface culling
			vec3 diff1 = vec3dif(gl_FragCoords[1],gl_FragCoords[0]);
			vec3 diff2 = vec3dif(gl_FragCoords[2],gl_FragCoords[0]);
			diff1.z = 0.0;
			diff2.z = 0.0;
			vec3 normal = vec3cross(diff1,diff2);
			if(normal.z > 0.0 ){
				//- make 2D rectangle around projected triangle
				vec2 rectmin = vec2min(vec2from3(gl_FragCoords[0]),vec2from3(gl_FragCoords[1]));
				rectmin = vec2min(rectmin,vec2from3(gl_FragCoords[2]));
				vec2 rectmax = vec2max(vec2from3(gl_FragCoords[0]),vec2from3(gl_FragCoords[1]));
				rectmax = vec2max(rectmax,vec2from3(gl_FragCoords[2]));
				//clip to viewport
				rectmin = vec2max(vpmin,rectmin);
				rectmax = vec2min(vpmax,rectmax);
				rectmin = vec2floor(rectmin);
				rectmax = vec2floor(rectmax);

				float area_total = area_of_triangle(gl_FragCoords[0],gl_FragCoords[1],gl_FragCoords[2]);
				for(int i=0;i<3;i++){
					printf("%d v[%f %f %f %f]p[%f %f %f %f]\n f[%f %f %f]ndc[%f %f %f %f]\n",
					i,gl_Vertexes[i].x,gl_Vertexes[i].y,gl_Vertexes[i].z,gl_Vertexes[i].w,
					gl_Positions[i].x,gl_Positions[i].y,gl_Positions[i].z,gl_Positions[i].w,
					gl_FragCoords[i].x,gl_FragCoords[i].y,gl_FragCoords[i].z,
					ndcs[i].x,ndcs[i].y,ndcs[i].z,ndcs[i].w
					);
				}

				//- iterate over rectangle doing each line, starting with line intersect triangle edge
				//- for each pixel interpolate varying from triangle corners
				printf("recmin = %f %f rectmax = %f %f\n",rectmin.x,rectmin.y,rectmax.x,rectmax.y);
				for(int irow = ifloor(rectmin.y); irow < ifloor(rectmax.y); irow++){
					//intersect row with triangle edges
					vec2 ix;
					int nx = 0;
					//find edges that cross horizontal row (max 2 edges)
					for(int iedge=0;iedge<3;iedge++){
						int iedge2 = (iedge + 1) % 3;
						if(isign(gl_FragCoords[iedge].y - (float)irow) != isign(gl_FragCoords[iedge2].y - (float)irow)){
							//two consecutive points are on opposite sides of this horizontal line
							//intersect row with edge
							vec3 bigdelta, delta;
							bigdelta = vec3dif(gl_FragCoords[iedge2], gl_FragCoords[iedge]);
							delta.y = gl_FragCoords[iedge2].y - (float)irow;
							delta.x = bigdelta.x * delta.y/bigdelta.y;
							delta.x += gl_FragCoords[iedge].x;
							nx++;
							if(nx == 1) ix.s = delta.x;
							if(nx == 2) ix.t = delta.x;
						}
					}
					//get min and max intersection
					//clip to viewport
					if(nx >= 2){
						int istartx, iendx;
						vec2 cx;
						cx.s = min(ix.s,ix.t);
						cx.t = max(ix.s,ix.t);
						cx.s = max(cx.s,rectmin.x);
						cx.t = min(cx.t,rectmax.x);
						cx = vec2floor(cx);
						istartx = ifloor(cx.s);
						iendx = ifloor(cx.t);
						
						for(int icol = istartx; icol < iendx; icol++){
							gl_FragCoord = vec2new2((float)icol,(float)irow);
							vec3 gl_FragCoord3 = vec3new3(gl_FragCoord.x,gl_FragCoord.y,0.0);
							//https://en.wikibooks.org/wiki/GLSL_Programming/Rasterization
							//compute alphas for interpolation
							vec3 alphas;
							alphas.r = area_of_triangle(gl_FragCoord3,gl_FragCoords[1],gl_FragCoords[2]);
							alphas.s = area_of_triangle(gl_FragCoord3,gl_FragCoords[2],gl_FragCoords[0]);
							alphas.t = area_of_triangle(gl_FragCoord3,gl_FragCoords[0],gl_FragCoords[1]);
							alphas = vec3scale(alphas,1.0/area_total);
							//interpolate varying and set here
							castle_Color = vec4triterp(alphas, castle_Colors);
							vertex_model = vec3triterp(alphas, vertex_models);
							castle_vertex_eye = vec4triterp(alphas,castle_vertex_eyes);

							main_fragment(); //run fragment cpu-shader
							//after fragment:
							//https://www.opengl.org/wiki/Per-Sample_Processing
							//- check depth vs blob depth
							//- save frag results to image blob
							//printf("frag x %f y %f color %f %f %f %f\n",gl_FragCoord.x,gl_FragCoord.y,
							//	gl_FragColor.r,gl_FragColor.g,gl_FragColor.b,gl_FragColor.a);
							//-  subtract viewport min from fragCoord to get blob coord
							int ix,iy;
							unsigned char *pixel;
							ix = ifloor(gl_FragCoord.x);
							iy = ifloor(gl_FragCoord.y);
							if(ix >=0 && ix < imsize[0] && iy >=0 && iy < imsize[1]){
								pixel = &imblob[iy*imsize[0]*4 + ix*4];
								for(int ij=0;ij<4;ij++){
									pixel[ij] = ifloor(gl_FragColor.data[ij] * 255.0);
								}
							}
						}
					}
				}
			}
		}
		//after all triangles and fragments:
		//- set blob as texture image to view aka glSwapBuffers, or dump
		if(1){
			textureTableIndexStruct_s ttis, *tti;
			printf("sampler_count from Sampler3D = %d",sampler_count);
			tti = &ttis;
			tti->channels = 4;
			tti->texdata = imblob;
			tti->x = imsize[0];
			tti->y = imsize[1];
			tti->z = 1;
			saveImage_web3dit(tti, "test_cpu_render_frame3.web3dit");
		}
	}

}


// <<<<< END SPECIFIC SHADER EXAMPLE ==================================
// <<<<<< END MIT =================

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
			if(1){
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

			} else {
				//CPU VERSION
				//Step 1: set the 3D texture
				if(node->voxels){
					struct X3D_Node *tmpN;
					POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->voxels,tmpN);
					tg->RenderFuncs.texturenode = (void*)tmpN;

					render_node(tmpN); //render_node(node->voxels); //still need render_texture to get it loaded
					textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tmpN);
					//GLint tdepth = GET_UNIFORM(myProg,"tex3dDepth");
					//GLUNIFORM1I(tdepth,tti->z);
					tex3dDepth = tti->z;

					//tex3dUseVertex - used in some vertex shaders to generate 3D texture coords automatically if set
					//GLint tex3dUseVertex = GET_UNIFORM(myProg,"tex3dUseVertex");
					//glUniform1i(tex3dUseVertex,0); 
					//tex3dUseVertex = 0;

					//repeatSTR used with texture2D emulation of texture3D
					//GLint repeatSTR = GET_UNIFORM(myProg,"repeatSTR");
					//glUniform1iv(repeatSTR,3,tti->repeatSTR);
					memcpy(repeatSTR,tti->repeatSTR,3*sizeof(int)); 

					//GLint magFilter = GET_UNIFORM(myProg,"magFilter");
					//glUniform1i(magFilter,tti->magFilter);
					magFilter = tti->magFilter;

				}

				//3.1 set uniforms: dimensions, focal length, fov (field of view), window size, modelview matrix
				//    set attributes vertices of triangles of bounding box
				// set box with vol.dimensions with triangles
				//GLint Vertices = GET_ATTRIB(myProg,"fw_Vertex");
				//GLint mvm = GET_UNIFORM(myProg,"fw_ModelViewMatrix"); //fw_ModelViewMatrix
				//GLint proj = GET_UNIFORM(myProg,"fw_ProjectionMatrix"); //fw_ProjectionMatrix
				//sendExplicitMatriciesToShader(mvm,proj,-1,NULL,-1);
				double modd[16],projd[16];
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modd);
				FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projd);

				matdouble2float4(fw_ModelViewMatrix,modd);
				matdouble2float4(fw_ProjectionMatrix,projd);

				//glEnableVertexAttribArray(Vertices);
				float *boxtris = (float*)node->_boxtris;
				//glVertexAttribPointer(Vertices, 3, GL_FLOAT, GL_FALSE, 0, boxtris);

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
				//GLint focal = GET_UNIFORM(myProg,"fw_FocalLength"); //fw_ModelViewMatrix
				//GLUNIFORM1F(focal,focalLength);
				fw_FocalLength = focalLength;
				//GLint vp = GET_UNIFORM(myProg,"fw_viewport");
				viewport[0] = iviewport[0]; //xmin
				viewport[1] = iviewport[1]; //ymin
				viewport[2] = iviewport[2]; //width
				viewport[3] = iviewport[3]; //height
				//GLUNIFORM4F(vp,viewport[0],viewport[1],viewport[2],viewport[3]);
				fw_viewport = vec4new4(viewport[0],viewport[1],viewport[2],viewport[3]);
				//GLint dim = GET_UNIFORM(myProg,"fw_dimensions");
				//GLUNIFORM3F(dim,node->dimensions.c[0],node->dimensions.c[1],node->dimensions.c[2]);
				fw_dimensions = vec3new3(node->dimensions.c[0],node->dimensions.c[1],node->dimensions.c[2]);
				//ray origin: the camera position 0,0,0 transformed into geometry local (box) coords
				//GLint orig = GET_UNIFORM(myProg,"fw_RayOrigin");
				float eyeLocal[3];
				double origind[3], eyeLocald[3];
				origind[0] = origind[1] = origind[2] = 0.0;
				double modelviewMatrix[16], mvmInverse[16];
				//GL_GET_MODELVIEWMATRIX
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
				matinverseAFFINE(mvmInverse,modelviewMatrix);
				transformAFFINEd(eyeLocald,origind,mvmInverse);
				for(int i=0;i<3;i++) eyeLocal[i] = eyeLocald[i];
				//GLUNIFORM3F(orig,eyeLocal[0],eyeLocal[1],eyeLocal[2]);
				fw_RayOrigin = vec3new3(eyeLocal[0],eyeLocal[1],eyeLocal[2]);
				//printf("rayOrigin= %f %f %f\n",eyeLocal[0],eyeLocal[1],eyeLocal[2]);

				//3.2 draw with shader
				//glDrawArrays(GL_TRIANGLES,0,36);
				cpu_drawtriangles(boxtris,36); //(float *vertices, nvertices)

				if(node->voxels){
					tg->RenderFuncs.textureStackTop = 0;
					tg->RenderFuncs.texturenode = NULL;
				}

			}

		}
	} //VF_Blend

}
