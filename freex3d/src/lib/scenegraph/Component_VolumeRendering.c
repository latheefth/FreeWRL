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

#undef uniform
#undef varying
// VERTEX ==========================================

//bulitins
vec4 gl_Position;

//uniforms
mat4 fw_ModelViewMatrix;
mat4 fw_ProjectionMatrix;
float fw_FocalLength;
vec4 fw_viewport;
vec4 fw_UnlitColor;
//attributes
vec4 fw_Vertex;

/* PLUG-DECLARATIONS */

//varying
vec4 castle_vertex_eye;
vec4 castle_Color; 
vec3 vertex_model; 


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
#define sampler3D int
int ifloor(float v){
	int ret = (int)v;
	return ret;
}
int iceil(float v){
	int ret;
	ret = (int)(v + .99999f);
	return ret;
}
textureTableIndexStruct_s *getTableTableFromTextureNode(struct X3D_Node *textureNode);
vec4 texture3D(sampler3D tunit, vec3 tcoord){
	vec4 ret;
	struct X3D_Node *tnode;
	ttglobal tg = gglobal();
	tnode = tg->RenderFuncs.texturenode;

	ret = vec4new1(0.0);
	if(isTex3D(tnode)){
		textureTableIndexStruct_s *tti = getTableTableFromTextureNode(tnode);
		vec3 imcoord;
		imcoord.x = tcoord.x * tti->x;
		imcoord.y = tcoord.y * tti->y;
		imcoord.z = tcoord.z * tti->z;
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
			xyzweight[0] = imcoord.x - (float)xrange[0];
			xyzweight[1] = imcoord.y - (float)yrange[0];
			xyzweight[2] = imcoord.z - (float)zrange[0];
			float frgba[4];

			int i,j,k,l,ii,jj,kk;
			for(i=0;i<2;i++){
				ii = zrange[i];
				if(ii < 0 || ii >= imagesize[2]) printf("ouch image z %d\n",ii);
				else
				for(j=0;j<2;j++){
					jj = yrange[j];
					if(jj < 0 || jj >= imagesize[1]) printf("ouch image y %d\n",jj);
					else
					for(k=0;k<2;k++){
						unsigned int pixel;
						unsigned char* rgba;
						kk = xrange[k];
						if(kk < 0 || kk >= imagesize[0]) printf("ouch image x %d\n",kk);
						else{
							pixel = td[ii*imagesize[1]*imagesize[0] + jj*imagesize[0] + kk];
							//rgba = (unsigned int *)&pixel;
							samples[k][j][i] = pixel; //(int)rgba[3];
						}
					}
				}
			}
			//weight the 8 samples
			unsigned char *rgba = (unsigned char *)&samples[0][0][0];
			ret.r = rgba[0];
			ret.g = rgba[1];
			ret.b = rgba[2];
			ret.a = rgba[3];
			//for(i=0;i<4;i++) frgba = 0.0f;
			//for(i=0;i<3;i++){
			//	unsigned char *rgba;
			//	unsigned int pixel = samples[k][j][i];
			//	rgba = (unsigned char *)&pixel;
			//	for(l=0;i<4;l++){
			//		frgba[l] += (1.0f - xyzweight[i])*(float)(int)rgba[l]
			//	}
			//}

		} //td != NULL
	}
	return ret;
}
// FRAGMENT ==========================================

//varying
vec3 vertex_model;
vec4 castle_vertex_eye;
vec4 castle_Color;

//uniform
mat4 fw_ModelViewMatrix;
mat4 fw_ProjectionMatrix;
float fw_FocalLength;
vec4 fw_viewport;
vec3 fw_dimensions;
vec3 fw_RayOrigin;
sampler3D fw_Texture_unit0;

int tex3dDepth;
int repeatSTR[3];
int magFilter;

//builtin
vec2 gl_FragCoord;
vec4 gl_FragColor;


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
	float densityFactor = .01; //1.0; //5.0;
	float Absorption = 1.0;
	int numSamples = 7; //128;
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
	if(numSamples <= 0) fragment_color = vec4new4(.1,.5,.1,1.0);
	//numSamples = 0;
	fw_TexCoord[0] = vertex_model; //vec3(.2,.2,.5);
	fragment_color = vec4new1(1.0);
	//fragment_color = texture2D(fw_Texture_unit0,fw_TexCoord[0].st);
	/* PL_UG: texture_apply (fragment_color, normal_eye_fragment) */
	fragment_color = texture3D(fw_Texture_unit0,fw_TexCoord[0]);
	fragment_color_main = fragment_color;
	fragment_color_main.a = 1.0;

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
				//GPU VERSION
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
					struct X3D_Node *tmpN;
					POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->voxels,tmpN);
					tg->RenderFuncs.texturenode = (void*)tmpN;

					render_node(tmpN); //render_node(node->voxels);

					struct textureVertexInfo mtf = {boxtex,2,GL_FLOAT,0,NULL,NULL};
					textureDraw_start(&mtf);

					//if(0){
					//	textureTableIndexStruct_s *tti = getTableTableFromTextureNode(X3D_NODE(node->voxels));
					//	GLint texloc = GET_UNIFORM(myProg,"fw_Texture_unit0");
					//	glUniform1i ( texloc, tti->OpenGLTexture );
					//}
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
				double modelviewMatrix[16], mvmInverse[16];
				//GL_GET_MODELVIEWMATRIX
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
				if(1){
				matinverseAFFINE(mvmInverse,modelviewMatrix);
				transformAFFINEd(eyeLocald,origind,mvmInverse);
				}else if(1){
				transformAFFINEd(eyeLocald,origind,modelviewMatrix);
				}else {
					veccopyd(eyeLocald,origind);
				}
				for(int i=0;i<3;i++) eyeLocal[i] = eyeLocald[i];
				GLUNIFORM3F(orig,eyeLocal[0],eyeLocal[1],eyeLocal[2]);
				//printf("rayOrigin= %f %f %f\n",eyeLocal[0],eyeLocal[1],eyeLocal[2]);
				if(!once) printf("orig %d dim %d vp %d focal %d\n",
				orig, dim ,  vp, focal );

				//3.2 draw with shader
				glDrawArrays(GL_TRIANGLES,0,36);

				if(node->voxels){
					textureDraw_end();
				}
				once = 1;
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


			} else {
				//CPU VERSION
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
					struct X3D_Node *tmpN;
					POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->voxels,tmpN);
					tg->RenderFuncs.texturenode = (void*)tmpN;

					render_node(tmpN); //render_node(node->voxels);
					struct textureVertexInfo mtf = {boxtex,2,GL_FLOAT,0,NULL,NULL};
					textureDraw_start(&mtf);

					//if(0){
					//	textureTableIndexStruct_s *tti = getTableTableFromTextureNode(X3D_NODE(node->voxels));
					//	GLint texloc = GET_UNIFORM(myProg,"fw_Texture_unit0");
					//	glUniform1i ( texloc, tti->OpenGLTexture );
					//}
				}

				//3.1 set uniforms: dimensions, focal length, fov (field of view), window size, modelview matrix
				//    set attributes vertices of triangles of bounding box
				// set box with vol.dimensions with triangles
				GLint Vertices = GET_ATTRIB(myProg,"fw_Vertex");
				GLint mvm = GET_UNIFORM(myProg,"fw_ModelViewMatrix"); //fw_ModelViewMatrix
				GLint proj = GET_UNIFORM(myProg,"fw_ProjectionMatrix"); //fw_ProjectionMatrix
				sendExplicitMatriciesToShader(mvm,proj,-1,NULL,-1);
				double modd[16],projd[16];
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modd);
				FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projd);

				matdouble2float4(fw_ModelViewMatrix,modd);
				matdouble2float4(fw_ProjectionMatrix,projd);

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
				GLint focal = GET_UNIFORM(myProg,"fw_FocalLength"); //fw_ModelViewMatrix
				GLUNIFORM1F(focal,focalLength);
				fw_FocalLength = focalLength;
				GLint vp = GET_UNIFORM(myProg,"fw_viewport");
				viewport[0] = iviewport[0]; //xmin
				viewport[1] = iviewport[1]; //ymin
				viewport[2] = iviewport[2]; //width
				viewport[3] = iviewport[3]; //height
				GLUNIFORM4F(vp,viewport[0],viewport[1],viewport[2],viewport[3]);
				fw_viewport = vec4new4(viewport[0],viewport[1],viewport[2],viewport[3]);
				GLint dim = GET_UNIFORM(myProg,"fw_dimensions");
				GLUNIFORM3F(dim,node->dimensions.c[0],node->dimensions.c[1],node->dimensions.c[2]);
				fw_dimensions = vec3new3(node->dimensions.c[0],node->dimensions.c[1],node->dimensions.c[2]);
				//ray origin: the camera position 0,0,0 transformed into geometry local (box) coords
				GLint orig = GET_UNIFORM(myProg,"fw_RayOrigin");
				float eyeLocal[3];
				double origind[3], eyeLocald[3];
				origind[0] = origind[1] = origind[2] = 0.0;
				double modelviewMatrix[16], mvmInverse[16];
				//GL_GET_MODELVIEWMATRIX
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
				if(1){
				double testzero [3];
				matinverseAFFINE(mvmInverse,modelviewMatrix);
				transformAFFINEd(eyeLocald,origind,mvmInverse);
				transformAFFINEd(testzero,eyeLocald,modelviewMatrix);
				//printf("testzero= %lf %lf %lf\n",testzero[0],testzero[1],testzero[2]);
				}else if(0){
				transformAFFINEd(eyeLocald,origind,modelviewMatrix);
				}else {
					veccopyd(eyeLocald,origind);
				}
				for(int i=0;i<3;i++) eyeLocal[i] = eyeLocald[i];
				GLUNIFORM3F(orig,eyeLocal[0],eyeLocal[1],eyeLocal[2]);
				fw_RayOrigin = vec3new3(eyeLocal[0],eyeLocal[1],eyeLocal[2]);
				//printf("rayOrigin= %f %f %f\n",eyeLocal[0],eyeLocal[1],eyeLocal[2]);


				//3.2 draw with shader
				//glDrawArrays(GL_TRIANGLES,0,36);
				for(int i=0;i<36;i++){
					float *point = &boxtris[i*3];
					fw_Vertex = vec4new4(point[0],point[1],point[2],0.0f);
					main_vertex();
					gl_FragCoord = vec2from3(vec3from4homog(gl_Position));
					main_fragment();
				}

				if(node->voxels){
					textureDraw_end();
				}
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
