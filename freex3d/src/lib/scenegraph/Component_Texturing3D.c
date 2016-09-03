/*


X3D Texturing3D Component

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
http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/texture3D.html

Texturing3D > Volumetric Image Formats
http://www.volumesoffun.com/voldat-format/
https://graphics.stanford.edu/data/voldata/
http://paulbourke.net/dataformats/pvl/
http://www.ncbi.nlm.nih.gov/pmc/articles/PMC2954506/#!po=59.5238
https://www.blender.org/manual/render/blender_render/textures/types/volume/voxel_data.html

http://paulbourke.net/dataformats/volumetric/
-Simplest 3d texture file if you write your own images
http://www.web3d.org/x3d/content/examples/Basic/VolumeRendering/
- Volumetric Rendering Samples use nrrd format
- http://teem.sourceforge.net/index.html
-- Nrrd lib LGPL

What is Texturing3D used for? a few links:
https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
- difference between composed slices and volume image
http://docs.nvidia.com/gameworks/content/gameworkslibrary/graphicssamples/opengl_samples/texturearrayterrainsample.htm
- an application for rendering terrain



Fuzzy Design:
- load various ways into a buffer representing contiguous voxels, with width, height, depth available
- somewhere in geometry rendering - polyrep stuff? - give it 3D or 4D-homogenous texture coordinates 
	instead of regular 2D. Hint: look at per-vertex RGBA floats in polyrep?
- somewhere in sending geom to shader, send the 3D/4D texture coords (and/or Matrix?)
- in shader, detect if its 3D texture, and use 3D texture lookup 

Q. is Texture3D available in GLES2?
	https://www.khronos.org/opengles/sdk/docs/reference_cards/OpenGL-ES-2_0-Reference-card.pdf
	- at bottom shows texture2D etc sampler functions
	instead of  vec3 texture2D(sampler2D,coord2D) 
	it would be vec4 texture3D(sampler3D sampler, vec3 coord) ?

GLES2 texture3D: first edition doesn't have sampler3D/texture3D
https://www.khronos.org/registry/gles/extensions/OES/OES_texture_3D.txt
- there's a proposed extension
-Q.how to test for it at runtime?
-A. worst case: frag shader has a compile error:
-- if I put Texture3D uniform, and sampler3D(texture3d,vec3(coords2D,0.0)) then:
	- desktop windows opengl frag shader compiles
	x uwp windows (using angle over dx/hlsl) frag shader doesn't compile
	x android frag shader doesn't compile
http://stackoverflow.com/questions/14150941/webgl-glsl-emulate-texture3d
http://stackoverflow.com/questions/16938422/3d-texture-emulation-in-shader-subpixel-related
- hackers emulating with texture2D tiles and lerps
https://android.googlesource.com/platform/external/chromium_org/third_party/angle/+/0027fa9%5E!/
- looks like chromium's ANGLE^ emulates cube as 6 2Ds as of 2014
  ^(gles2 over DX/HLSL for webgl in windows chrome browser) 
Michalis: "Note that Castle Game Engine does not support 3D textures in
	OpenGLES now (so we don't use OES_texture_3D), but only on desktop
	OpenGL now (available through EXT_texture3D, or in standard OpenGL >=
	1.2). But that's simply due to my lack of time to patch necessary
	things:) From the first glance, using OES_texture_3D to achieve the
	same functionality on OpenGLES should be easy."
	https://github.com/castle-engine/demo-models > texturing_advanced/tex3d_*

Example use: if you have 3 images for terrain - white for mountain peaks, brown for mountain sides, green for valleys
- then combine them into a nxmx3 volume image?
- then you can use the terrain height to compute an R value ?
- cube sampler should in theory interpolate between the 3 layers?

Design:
Texture coordinates:
I think the geometry vertices, scaled into the range 0-1 on each axis, 
could/should serve as default 3D texture coordinates.
algorithm:
a) interpolate through varying the vertex xyz you want as usual
b) transform xyz into 0-1 range 
	-would need to know bounding box, or have pre-computed, or assume -1 to 1, or in textureTransform3D
Option: convert xyz vertex to 0-1 range in vertex shader, 
	and do varying interpolation in 0-1 range for varying textureCoords3D aka tc3d
c) sample the texture
A. if have texture3D/sampler3D/EXT_texture3D/OES_texture3D: 
	sample directly tc3d.xyz (0-1): frag = sampler3D(texture3D, vec3 tc3d.xyz)
B. else emulate texture3D/sampler3D with 2D tiled texture:
	single texture2D: sample the texture at xy[z] on the 2D image plane, z rounded 'above' and 'below'
	Q. how many z layers, and does it have to be same as rows & cols?
	nx,ny,nz = size of image in pixels
	nz == number of z levels
	nzr = sqrt(nz) so if 256 z levels, it would be sqrt(256) = 16. 16x16 tiles in 2D plane
	x,y,z - 0-1 range 3D image texture coords from tc3d.xyz
	vec2 xyi = xy[i] = f(xyz) = [x/nzr + (i / nzr), y/nzr + (i % nzr)]
	vec2 xyj = xy[j] ... "
	where
	 i = floor(z*nz)
	 j = ceil(z*nz)
	fxyi = sampler2D(texture2D,xyi)
	fxyj = sampler2D(texture2D,xyj)
	lerp between layers xy[i],xy[j]: frag = lerp(fxyi,fxyj,(z*nz-i)/(j-i))
	Note: i,j don't need to be ints, nothing above needs to be int, can all be float


*/

void render_PixelTexture3D (struct X3D_PixelTexture3D *node) {
	loadTextureNode(X3D_NODE(node),NULL);
	gglobal()->RenderFuncs.textureStackTop=1; /* not multitexture - should have saved to boundTextureStack[0] */
}

void render_ImageTexture3D (struct X3D_ImageTexture3D *node) {
	/* printf ("render_ImageTexture, global Transparency %f\n",getAppearanceProperties()->transparency); */
	loadTextureNode(X3D_NODE(node),NULL);
	gglobal()->RenderFuncs.textureStackTop=1; /* not multitexture - should have saved to boundTextureStack[0] */
}

void render_ComposedTexture3D (struct X3D_ComposedTexture3D *node) {
	/* printf ("render_ComposedTexture, global Transparency %f\n",getAppearanceProperties()->transparency); */
	loadTextureNode(X3D_NODE(node),NULL);
	loadMultiTexture(node);
	gglobal()->RenderFuncs.textureStackTop=1; /* not multitexture - should have saved to boundTextureStack[0] */
}
