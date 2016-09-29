/*


X3D Cubemap Texturing Component

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
#include "../scenegraph/Component_CubeMapTexturing.h"
#include "../input/EAIHelpers.h"
#include "../vrml_parser/CParseGeneral.h" /* for union anyVrml */

#ifndef HAVE_UINT32
# define uint32 uint32_t
#endif


/*

"CUBEMAP STANDARDS"?
- left to right is usually obvious - sky usually at top
a) for the single image -+-- layout its usually ovbious which way to shoot down and top images:
	-top of down image is contiguous with bottom of front
	-bottom of up image is contiguous with top of front
	https://msdn.microsoft.com/en-us/library/windows/desktop/bb204881(v=vs.85).aspx
	- direct3D cubic environment mapping
	- no talk of DDS, but shows +- layout on single uv image (relative to LHS object space axes, Y up)
		 +y
	-x  +z  +x  -z
		 -y

b) for otherwise piecewise cubemaps its a little less obvious, needs standards:

http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/env_texture.html#Textureorientation
Web3D has texture orientation
- Front is on the XY plane, in RHS
- Left is on the YZ plane
+x == Right
-x == Left
+y == Top
-y == Down
+z == Back
-z == Front
so to match DX ordering:
R,L,T,D,B,F
x no mention of which way is up on the the top and bottom images

http://wiki.simigon.com/wiki/index.php?title=Dds_cubemaps
- has instructions for generating via maya -> photoshop -> dds
- rotations of images: 4 sides are obvious, top at top
x up/down seem rotatated: 
- Up has Right at top (+x when looking from center)
x Down has Left at top (-x when looking from center)
file order: F,B,U,D,L,R
Assuming F==+x: in LHS system:
file order: +x,-x,+y,-y,+z,-z
- with top of Top against -z, top of bottom against +z

OpenGL Redbook p.441 has no diagram, but hints at the same face ordering as dds.

https://docs.unity3d.com/Manual/class-Cubemap.html
Unity uses Y up, and (unlike web3d) LHS
Right +x
Left -x
Top +Y
Bottom -Y
Front +z
Back -Z
Top of the bottom image is Left -x like simigon says about DDS cubemap
Top of the top image is front or back likely +z front

http://stackoverflow.com/questions/11685608/convention-of-faces-in-opengl-cubemapping
- mentions of renderman
- a LHS diagram for figuring opengl cube map
http://www.nvidia.com/object/cube_map_ogl_tutorial.html
- the refleciton pool architecture model images I'm using are from an nVidia oopengl cubemap tutorial


http://developer.amd.com/tools-and-sdks/archive/games-cgi/cubemapgen/
CCubeMapProcessor.cpp:
// D3D cube map face specification
//   mapping from 3D x,y,z cube map lookup coordinates 
//   to 2D within face u,v coordinates
//
//   --------------------> U direction 
//   |                   (within-face texture space)
//   |         _____
//   |        |     |
//   |        | +Y  |
//   |   _____|_____|_____ _____
//   |  |     |     |     |     |
//   |  | -X  | +Z  | +X  | -Z  |
//   |  |_____|_____|_____|_____|
//   |        |     |
//   |        | -Y  |
//   |        |_____|
//   |
//   v   V direction
//      (within-face texture space)
- that's an LHS (Left-Handed coordinate System)
- don't take the U,V as absolute in this diagram, but rather as directional hint 
- if +Y is top, -Y bottom, +Z front, -Z back, then its saying:
	top of the Top is against Back, and top of the Bottom is against Front.
x doesn't explain the order of faces in .dds file
* does harmonize with simigon above, which has (dug9-derived) file face order (LHS Z):
  _____ _____ _____ _____ _____ _____
 |     |     |     |     |     |     |
 | +X  | -X  | +Y  | -Y  | +Z  | -Z  |
 |_____|_____|_____|_____|_____|_____|


SUMMARY OF BEST GUESS OF DDS CUBEMAP LAYOUT:
a) LHS: +y is up, xy form RHS 2D axes, +z is LHS relative to xy
b) face order in file: (as per simigon derived diagram above):
	 +x (Right), -x (Left), +y (Top), -y(Bottom), +z(Front, in LHS), -z(Back, in LHS)
c) uv direction per face (as per CubeMapGen diagram above): 
	- x,z faces (+x/Right,-x/Left,+z/Front,-z/Back): top of image is up; 
	- +y(Top): top of Top is against -z(Back)
	- -y(Bottom): top of Bottom is against +z(Front)
This interpretation matches the 2nd diagram on this page:
https://msdn.microsoft.com/en-us/library/windows/desktop/bb204881(v=vs.85).aspx


CUBEMAP FORMAT FOR INTERNAL FREEWRL AND .WEB3DIT
http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/env_texture.html#Textureorientation
Similar to above for DDS summary, except:
1. using opengl/web3d RHS: when naming the faces by signed axis:-
	sign on z is reversed, -z is Front, +z is Back
2. order: +x,-x,+y,-y,+z,-z except +z,-z mean something different. So relative to DDS, we swap Front and Back
	Back is before Front in linear list

a) RHS with y-up, -z Front
b) face order in linear array:
	 +x (Right), -x (Left), +y (Top), -y(Bottom), +z(Back, in RHS), -z(Front, in RHS)
c) uv direction per face - (same as DDS)
	- x,z faces: top of image is up
	- +y(Top): top of Top is against +z(Back)
	- -y(Bottom): top of Bottom is against -z(Front)

OPENGL CUBETEXTURE CONVENTION
https://www.opengl.org/registry/doc/glspec21.20061201.pdf
section 3.8.6, p.170, table 3.19 shows how your 3D texture coordinate
is used in the sampler: 
Major Axis
Direction  Target                       sc  tc ma
+rx        TEXTURE CUBE MAP POSITIVE X -rz -ry rx
-rx        TEXTURE CUBE MAP NEGATIVE X  rz -ry rx
+ry        TEXTURE CUBE MAP POSITIVE Y  rx  rz ry
-ry        TEXTURE CUBE MAP NEGATIVE Y  rx -rz ry
+rz        TEXTURE CUBE MAP POSITIVE Z  rx -ry rz
-rz        TEXTURE CUBE MAP NEGATIVE Z -rx -ry rz
Take the +rx - the plus x face. If you're in the center of the cube
at the origin, looking down the +x axis, in a Y-up RHS system, shouldn't 
you see +z going to your right, and +y going up, in the 2D texture coordinate 
system? Opengl has them both negative.

Its a bit bizzarre and makes more sense
if thinking of texture rows as y-down like images, and xyz as LHS - 2 things 
that seem un-opengl-like. Someone said its using renderman convention for cubemaps.

There's no way to intercept the output of this table before its used in cubeSampler.

In freewrl we have been flipping texture rows to be y-down in Textures.c L.1432 in move_texture_to_opengl()
- and for ComposedTexture below we exchange left/right and front/back textures
- we still need to reflect one axis of our RHS reflection vector to get from our RHS to renderman LHS,
x hard to find a way to do that in the shader that works for all cubemap faces

http://www.3dcpptutorials.sk/index.php?id=24
- this developer shows there's a way to do it without flipping your textures y-down
-- 'just' re-arranging textures and rotating around 180
-- I tried with composed, and it worked by doing 3 things:
	a) don't flip y-down in textures.c L.1432
		if(0){
			//flip cubemap textures to be y-down following opengl specs table 3-19
	b) swap faces in pairs so right goes to (our count=1) GL_CUBEMAP_NEGATIVE_X instead of (our count=0) POSITIVE_X
				case 1: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->right,thistex); break;}
	c) in vertex shader reflect 2 axes of the reflection vector
		fw_TexCoord[0].yz = -fw_TexCoord[0].yz;
So could be rolled out for Composed, Generated, Image Cubemaps


OpenGL has defined constants for cubemap faces:
- GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT
- in numerical order +X, -X, +Y, -Y, +Z, -Z
  http://learnopengl.com/#!Advanced-OpenGL/Cubemaps
- assume (untested): uv directions are same as FREEWRL/WEB3DIT and DDS:
	- top of x,z faces: +y/up
	- top of Top: adjacent to Back
	- top of Bottom: adjacent to Front

SUMMARY OF CUBEMAP
I've confirmed my proper creation of dds cubemap with Gimp DDS using ATI's CubeMapGen utility
http://developer.amd.com/tools-and-sdks/archive/games-cgi/cubemapgen/
- CubeMapGen uses an axis numbering scheme that matches DX/DDS conventions

WORKING with DDS cubemap and -+-- tee layout single images Sept 12, 2016
Not done:
- figure out how/why other browsers (octaga, instantplayer, view3dscene) accept .dds, but show a spiral texture
- auto-detect 1x6, 2x3 (6x1, 3x2?) and 1x1 (earth/spherical texture) layouts 
	- InstantReality uses 1x1 spherical image map with single .png
	x tried 1x6 and 2x3 but other browsers aren't using those layouts either

GENERATEDCUBEMAP aka dynamic cubemap
a general algorithm: 7 more passes through scenegraph:
A search scenegraph for generatedcubemap locations
B foreach generatedcubemap location
	foreach 6 faces
		render scenegraph to fbo without generatedcubemap consumers
	convert fbo to cubemap
C render scenegraph normally with generatedcubemap consumers

Strange conciderations:
- if there are 2 generatedcubemap consumers side by side, in theory you should
	 have infinite re-renderings to get all the reflections back and forth between them
	 (we will ignore other generatedcubemap consumers when generating)
- if a generatedcubemap is DEF/USED, which location would you use
	(we will snapshot generatedcubemap locations (node,modelviewmatrix), sort by node, 
		and eliminated node duplicates. So don't DEF/USE in scene - you'll get only one)


dynamic cubemap links:
http://www.mbroecker.com/project_dynamic_cubemaps.html
http://richardssoftware.net/Home/Post/26
- DX
http://webglsamples.org/dynamic-cubemap/dynamic-cubemap.html
- webgl sample
http://stackoverflow.com/questions/4775798/rendering-dynamic-cube-maps-in-opengl-with-frame-buffer-objects
http://users.csc.calpoly.edu/~ssueda/teaching/CSC471/2016S/demos/khongton/index.html
http://math.hws.edu/graphicsbook/source/webgl/cube-camera.html
https://www.opengl.org/discussion_boards/showthread.php/156906-Dynamic-cubemap-FBO
https://github.com/WebGLSamples/WebGLSamples.github.io/tree/master/dynamic-cubemap


*/

static int lookup_xxyyzz_face_from_count [] = {0,1,2,3,4,5}; // {1,0,2,3,5,4}; //swaps left-right front-back faces

/* testing */
//OLDCODE #define CUBE_MAP_SIZE 256

#ifndef GL_EXT_texture_cube_map
//OLDCODE # define GL_NORMAL_MAP_EXT                   0x8511
//OLDCODE # define GL_REFLECTION_MAP_EXT               0x8512
//OLDCODE # define GL_TEXTURE_CUBE_MAP_EXT             0x8513
//OLDCODE # define GL_TEXTURE_BINDING_CUBE_MAP_EXT     0x8514
# define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT  0x8515
//OLDCODE # define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT  0x8516
//OLDCODE # define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT  0x8517
//OLDCODE # define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT  0x8518
//OLDCODE # define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT  0x8519
//OLDCODE # define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT  0x851A
//OLDCODE # define GL_PROXY_TEXTURE_CUBE_MAP_EXT       0x851B
//OLDCODE # define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT    0x851C
#endif


/****************************************************************************
 *
 * ComposedCubeMapTextures
 *
 ****************************************************************************/

void render_ComposedCubeMapTexture (struct X3D_ComposedCubeMapTexture *node) {
	int count, iface;
	struct X3D_Node *thistex = 0;

        //printf ("render_ComposedCubeMapTexture\n");
	for (count=0; count<6; count++) {

		/* set up the appearanceProperties to indicate a CubeMap */
		getAppearanceProperties()->cubeFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+count;
        //printf ("set cubeFace to %d in rcm\n",getAppearanceProperties()->cubeFace);
		/* go through these, right left, top, bottom, front, back, */
		//                     +x,   -x,  +y,     -y,   +z,   -z    //LHS system
		//                                              -z,   +z    //RHS system
		// we appear to be swapping left/right front/back
		iface = lookup_xxyyzz_face_from_count[count];
		switch (iface) {
			case 0: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->right,thistex); break;}
			case 1: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->left,thistex);    break;}

			case 2: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->top,thistex);  break;}
			case 3: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->bottom,thistex);   break;}

			case 4: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->front,thistex);   break;}
			case 5: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->back,thistex);  break;}
		}
        //printf ("rcm, thistex %p, type %s\n",thistex,stringNodeType(thistex->_nodeType));
		if (thistex != NULL) {
			/* we have an image specified for this face */
			/* the X3D spec says that a X3DTextureNode has to be one of... */
			if ((thistex->_nodeType == NODE_ImageTexture) ||
			    (thistex->_nodeType == NODE_PixelTexture) ||
			    (thistex->_nodeType == NODE_MovieTexture) ||
			    (thistex->_nodeType == NODE_MultiTexture)) {

				gglobal()->RenderFuncs.textureStackTop = 0;
				/* render the proper texture */
				render_node((void *)thistex);
			} 
		}
	}
    
    /* set this back for "normal" textures. */
     getAppearanceProperties()->cubeFace = 0;
}



/* is this a DDS file? If so, get it, and subdivide it. Ignore MIPMAPS for now */
/* see: http://www.mindcontrol.org/~hplus/graphics/dds-info/MyDDS.cpp */
/* see: http://msdn.microsoft.com/en-us/library/bb943991.aspx/ */
// 2016: https://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx

/* DDS readstuff */
/* DDS loader written by Jon Watte 2002 */
/* Permission granted to use freely, as long as Jon Watte */
/* is held harmless for all possible damages resulting from */
/* your use or failure to use this code. */
/* No warranty is expressed or implied. Use at your own risk, */
/* or not at all. */

#if !defined( mydds_h )
#define mydds_h

//  little-endian, of course
#define DDS_MAGIC 0x20534444


//  DDS_header.dwFlags
#define DDSD_CAPS                   0x00000001 
#define DDSD_HEIGHT                 0x00000002 
#define DDSD_WIDTH                  0x00000004 
#define DDSD_PITCH                  0x00000008 
#define DDSD_PIXELFORMAT            0x00001000 
#define DDSD_MIPMAPCOUNT            0x00020000 
#define DDSD_LINEARSIZE             0x00080000 
#define DDSD_DEPTH                  0x00800000 

//  DDS_header.sPixelFormat.dwFlags
#define DDPF_ALPHAPIXELS            0x00000001 
#define DDPF_FOURCC                 0x00000004 
#define DDPF_INDEXED                0x00000020 
#define DDPF_RGB                    0x00000040 

//  DDS_header.sCaps.dwCaps1
#define DDSCAPS_COMPLEX             0x00000008 
#define DDSCAPS_TEXTURE             0x00001000 
#define DDSCAPS_MIPMAP              0x00400000 

//  DDS_header.sCaps.dwCaps2
#define DDSCAPS2_CUBEMAP            0x00000200 
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400 
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800 
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000 
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000 
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000 
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000 
#define DDSCAPS2_VOLUME             0x00200000 

/* old way - use 4-char string and cast later, not a good idea 
#define D3DFMT_DXT1     "1TXD"    //  DXT1 compression texture format 
#define D3DFMT_DXT2     "2TXD"    //  DXT2 compression texture format 
#define D3DFMT_DXT3     "3TXD"    //  DXT3 compression texture format 
#define D3DFMT_DXT4     "4TXD"    //  DXT4 compression texture format 
#define D3DFMT_DXT5     "5TXD"    //  DXT5 compression texture format 
*/
/* new way - use actual four-byte unsigned integer value */
#define D3DFMT_DXT1	0x31545844
#define D3DFMT_DXT2	0x32545844
#define D3DFMT_DXT3	0x33545844
#define D3DFMT_DXT4	0x34545844
#define D3DFMT_DXT5	0x35545844


#define PF_IS_DXT1(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == (unsigned int) D3DFMT_DXT1))

#define PF_IS_DXT3(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == (unsigned int) D3DFMT_DXT3))

#define PF_IS_DXT5(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == (unsigned int) D3DFMT_DXT5))

#define PF_IS_BGRA8(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
   (pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 32) && \
   (pf.dwRBitMask == 0xff0000) && \
   (pf.dwGBitMask == 0xff00) && \
   (pf.dwBBitMask == 0xff) && \
   (pf.dwAlphaBitMask == 0xff000000U))

#define PF_IS_RGB8(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
  !(pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 24) && \
   (pf.dwRBitMask == 0xff) && \
   (pf.dwGBitMask == 0xff00) && \
   (pf.dwBBitMask == 0xff0000))

#define PF_IS_BGR8(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
  !(pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 24) && \
   (pf.dwRBitMask == 0xff0000) && \
   (pf.dwGBitMask == 0xff00) && \
   (pf.dwBBitMask == 0xff))

#define PF_IS_BGR5A1(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
   (pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 16) && \
   (pf.dwRBitMask == 0x00007c00) && \
   (pf.dwGBitMask == 0x000003e0) && \
   (pf.dwBBitMask == 0x0000001f) && \
   (pf.dwAlphaBitMask == 0x00008000))

#define PF_IS_BGR565(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
  !(pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 16) && \
   (pf.dwRBitMask == 0x0000f800) && \
   (pf.dwGBitMask == 0x000007e0) && \
   (pf.dwBBitMask == 0x0000001f))

#define PF_IS_INDEX8(pf) \
  ((pf.dwFlags & DDPF_INDEXED) && \
   (pf.dwRGBBitCount == 8))

#define PF_IS_VOLUME(pf) \
  ((pf.dwFlags & DDSD_DEPTH))
  //&& 
  // (pf.sCaps.dwCaps1 & DDSCAPS_COMPLEX) && 
  // (pf.sCaps.dwCaps1 & DDSCAPS2_VOLUME)) 



union DDS_header {
  struct {
    unsigned int    dwMagic;
    unsigned int    dwSize;
    unsigned int    dwFlags;
    unsigned int    dwHeight;
    unsigned int    dwWidth;
    unsigned int    dwPitchOrLinearSize;
    unsigned int    dwDepth;
    unsigned int    dwMipMapCount;
    unsigned int    dwReserved1[ 11 ];

    //  DDPIXELFORMAT
    struct {
      unsigned int    dwSize;
      unsigned int    dwFlags;
      unsigned int    dwFourCC;
      unsigned int    dwRGBBitCount;
      unsigned int    dwRBitMask;
      unsigned int    dwGBitMask;
      unsigned int    dwBBitMask;
      unsigned int    dwAlphaBitMask;
    }               sPixelFormat;

    //  DDCAPS2
    struct {
      unsigned int    dwCaps1;
      unsigned int    dwCaps2;
      unsigned int    dwDDSX;
      unsigned int    dwReserved;
    }               sCaps;
    unsigned int    dwReserved2;
  }; //JASdefStruct; // put "name" in here to get rid of compiler warning
char data[ 128 ];
};

#endif  //  mydds_h


struct DdsLoadInfo {
  bool compressed;
  bool swap;
  bool palette;
  unsigned int divSize;
  unsigned int blockBytes;
  GLenum internalFormat;
  GLenum externalFormat;
  GLenum type;
};

struct DdsLoadInfo loadInfoDXT1 = {
  true, false, false, 4, 8, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
};
struct DdsLoadInfo loadInfoDXT3 = {
  true, false, false, 4, 16, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
};
struct DdsLoadInfo loadInfoDXT5 = {
  true, false, false, 4, 16, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
};

#if defined (GL_BGRA)

	struct DdsLoadInfo loadInfoBGRA8 = {
	  false, false, false, 1, 4, GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE
	};
	struct DdsLoadInfo loadInfoBGR5A1 = {
	  false, true, false, 1, 2, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV
	};
	struct DdsLoadInfo loadInfoIndex8 = {
	  false, false, true, 1, 1, GL_RGB8, GL_BGRA, GL_UNSIGNED_BYTE
	};
#endif //BGRA textures supported

struct DdsLoadInfo loadInfoRGB8 = {
  false, false, false, 1, 3, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE
};
struct DdsLoadInfo loadInfoBGR8 = {
  false, false, false, 1, 3, GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE
};
struct DdsLoadInfo loadInfoBGR565 = {
  false, true, false, 1, 2, GL_RGB5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5
};
unsigned int GetLowestBitPos(unsigned int value)
{
   assert(value != 0); // handled separately

   unsigned int pos = 0;
   while (!(value & 1))
   {
      value >>= 1;
      ++pos;
	  if(pos == 32) break;
   }
   return pos;
}
int textureIsDDS(textureTableIndexStruct_s* this_tex, char *filename) {
	FILE *file;
	char *buffer, *bdata, *bdata2;
	char sniffbuf[20];
	unsigned long fileLen;
	union DDS_header hdr;
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int z = 0;
	unsigned int rshift[4]; //to go with color bitmask
	int nchan, idoFrontBackSwap;
	unsigned int mipMapCount = 0;
	unsigned int size,xSize, ySize,zSize;

	struct DdsLoadInfo * li;
	size_t xx;

	UNUSED(xx); // compiler warning mitigation
	xSize=ySize=zSize=0;
	li = NULL;

	//printf ("textureIsDDS... node %s, file %s\n",
	//	stringNodeType(this_tex->scenegraphNode->_nodeType), filename);

	/* read in file */
	file = fopen(filename,"rb");
	if (!file) 
		return FALSE;

	//sniff header
	xx=fread(sniffbuf, 4, 1, file);
	fclose(file);
	if(strncmp(sniffbuf,"DDS ",4)){
		//not DDS file
		//sniffbuf[5] = '\0';
		//printf("sniff header = %s\n",sniffbuf);
		return FALSE;
	}
	file = fopen(filename,"rb");
	/* have file, read in data */


	/* get file length */
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);

	/* llocate memory */
	buffer=MALLOC(char *, fileLen+1);
	if (!buffer) {
		fclose(file);
		return FALSE;
	}

	/* read file */
	xx=fread(buffer, fileLen, 1, file);
	fclose(file);

	/* check to see if this could be a valid DDS file */
	if (fileLen < sizeof(hdr)) 
		return FALSE;

	/* look at the header, see what kind of a DDS file it might be */
	memcpy( &hdr, buffer, sizeof(hdr));

	/* does this start off with "DDS " an so on ?? */
	if ((hdr.dwMagic == DDS_MAGIC) && (hdr.dwSize == 124) &&
		(hdr.dwFlags & DDSD_PIXELFORMAT) && (hdr.dwFlags & DDSD_CAPS)) {
		printf ("matched :DDS :\n");

		
		printf ("dwFlags %x, DDSD_PIXELFORMAT %x, DDSD_CAPS %x\n",hdr.dwFlags, DDSD_PIXELFORMAT, DDSD_CAPS);
			xSize = hdr.dwWidth;
			ySize = hdr.dwHeight;
		printf ("size %d, %d\n",xSize, ySize);
		

		/*
			assert( !(xSize & (xSize-1)) );
			assert( !(ySize & (ySize-1)) );
		*/

		
		printf ("looking to see what it is...\n");
		printf ("DDPF_FOURCC dwFlags %x mask %x, final %x\n",hdr.sPixelFormat.dwFlags,DDPF_FOURCC,hdr.sPixelFormat.dwFlags & DDPF_FOURCC);

		printf ("if it is a dwFourCC, %x and %x\n", hdr.sPixelFormat.dwFourCC ,D3DFMT_DXT1);

		printf ("dwFlags %x\n",hdr.sPixelFormat.dwFlags);
		printf ("dwRGBBitCount %d\n",hdr.sPixelFormat.dwRGBBitCount); //24 for normal RGB
		printf ("dwRBitMask %x\n",hdr.sPixelFormat.dwRBitMask);
		printf ("dwGBitMask %x\n",hdr.sPixelFormat.dwGBitMask);
		printf ("dwBBitMask %x\n",hdr.sPixelFormat.dwBBitMask);
		printf ("dwAlphaBitMask %x\n",hdr.sPixelFormat.dwAlphaBitMask);
		printf ("dwFlags and DDPF_ALPHAPIXELS... %x\n",DDPF_ALPHAPIXELS & hdr.sPixelFormat.dwFlags);
		printf ("dwflags & DDPF_RGB %x\n,",hdr.sPixelFormat.dwFlags & DDPF_RGB);

		printf ("dwFlags and DEPTH %x\n",hdr.dwFlags & DDSD_DEPTH);
		printf ("dwCaps1 and complex %x\n",   (hdr.sCaps.dwCaps1 & DDSCAPS_COMPLEX));
		printf ("dwCaps1 and VOLUME %x\n", (hdr.sCaps.dwCaps1 & DDSCAPS2_VOLUME));
		
		//rshift[0] = GetLowestBitPos(hdr.sPixelFormat.dwRBitMask);
		//rshift[1] = GetLowestBitPos(hdr.sPixelFormat.dwGBitMask);
		//rshift[2] = GetLowestBitPos(hdr.sPixelFormat.dwBBitMask);
		//rshift[3] = GetLowestBitPos(hdr.sPixelFormat.dwAlphaBitMask);
		bdata = NULL;
		if(hdr.sPixelFormat.dwFlags & DDPF_FOURCC){
			if( PF_IS_DXT1( hdr.sPixelFormat ) ) {
				li = &loadInfoDXT1;
			}
			else if( PF_IS_DXT3( hdr.sPixelFormat ) ) {
				li = &loadInfoDXT3;
			}
			else if( PF_IS_DXT5( hdr.sPixelFormat ) ) {
				li = &loadInfoDXT5;
			}
  
			#if defined (GL_BGRA)
			else if( PF_IS_BGRA8( hdr.sPixelFormat ) ) {
				li = &loadInfoBGRA8;
			}
			else if( PF_IS_BGR5A1( hdr.sPixelFormat ) ) {
				li = &loadInfoBGR5A1;
			}
			else if( PF_IS_INDEX8( hdr.sPixelFormat ) ) {
				li = &loadInfoIndex8;
			}
			#endif

			else if( PF_IS_RGB8( hdr.sPixelFormat ) ) {
				li = &loadInfoRGB8;
			}
			else if( PF_IS_BGR8( hdr.sPixelFormat ) ) {
				li = &loadInfoBGR8;
			}
			else if( PF_IS_BGR565( hdr.sPixelFormat ) ) {
				li = &loadInfoBGR565;
			}
			//else {
			//	ConsoleMessage("CubeMap li failure\n");
			//	return FALSE;
			//}
		}else{
			//no FOURCC
			bdata = &buffer[sizeof(union DDS_header)];
			//bdata = &hdr.data[0];
		}
		//fixme: do cube maps later
		//fixme: do 3d later
		x = xSize = hdr.dwWidth;
		y = ySize = hdr.dwHeight;
		z = zSize = 1;
		idoFrontBackSwap = 0;
		if( PF_IS_VOLUME(hdr) )
			z = zSize = hdr.dwDepth;
		if( hdr.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP){
			int facecount = 0;
			if(hdr.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) facecount++;
			if(hdr.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) facecount++;
			if(hdr.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY) facecount++;
			if(hdr.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) facecount++;
			if(hdr.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) facecount++;
			if(hdr.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) facecount++;
			z = zSize = facecount;
			if(z==6)
				idoFrontBackSwap = 1;
		}
		nchan = 3;
		if(DDPF_ALPHAPIXELS & hdr.sPixelFormat.dwFlags) nchan = 4;
		//if(li == NULL)
		//	return FALSE;
		//if(!hdr.dwFlags & DDSD_MIPMAPCOUNT){
		if(bdata){
			//simple, convert to rgba and set tti
			int ipix,jpix,i,j,k,bpp, ir, ig, ib;
			char * rgbablob = malloc(x*y*z *4);
			bpp = hdr.sPixelFormat.dwRGBBitCount / 8;
			ir = 0; ig = 1; ib = 2; //if incoming is BGR order
			if(hdr.sPixelFormat.dwRBitMask > hdr.sPixelFormat.dwBBitMask){
				//if incoming is RGB order
				ir = 2;
				ib = 0;
				printf("BGR\n");
			}
			//printf("bitmasks R %d G %d B %d\n",hdr.sPixelFormat.dwRBitMask,hdr.sPixelFormat.dwGBitMask,hdr.sPixelFormat.dwBBitMask);
			//printf("bpp=%d x %d y %d z %d\n",bpp, x,y,z);
			for(i=0;i<z;i++){
				for(j=0;j<y;j++){
					for(k=0;k<x;k++){
						unsigned char *pixel,*rgba;
						int ii;
						ii = idoFrontBackSwap && i == 4? 5 : i; //swap Front and Back faces for opengl order
						ii = idoFrontBackSwap && i == 5? 4 : i;
						ii = i;
						ipix = (i*y +j)*x +k;     //top down, for input image
						jpix = (ii*y +(y-1-j))*x + k; //bottom up, for ouput texture
						pixel = &bdata[ipix * bpp];
						rgba = &rgbablob[jpix *4];
						//freewrl target format: RGBA
						//swizzle if incoming is BGRA
						rgba[3] = 255;
						rgba[0] = pixel[ir];
						rgba[1] = pixel[ig];
						rgba[2] = pixel[ib];
						if(nchan == 4)
							rgba[3] = pixel[3];
						if(0){
							static int once = 0;
							if(!once){
								printf("pixel R=%x G=%x B=%x A=%x\n",rgba[0],rgba[1],rgba[2],rgba[3]);
								//once = 1;
							}
						}

					}
				}
			}
			this_tex->channels = nchan;
			this_tex->x = x;
			this_tex->y = y;
			this_tex->z = z;
			this_tex->texdata = rgbablob;
			return TRUE;
		}else{
			return FALSE;
		}

		mipMapCount = (hdr.dwFlags & DDSD_MIPMAPCOUNT) ? hdr.dwMipMapCount : 1;
		//printf ("mipMapCount %d\n",mipMapCount);

		if( li->compressed ) {
			printf ("compressed\n");
			/*
			size_t size = max( li->divSize, x )/li->divSize * max( li->divSize, y )/li->divSize * li->blockBytes;
			assert( size == hdr.dwPitchOrLinearSize );
			assert( hdr.dwFlags & DDSD_LINEARSIZE );
			unsigned char * data = (unsigned char *)malloc( size );
			if( !data ) {
				goto failure;
			}
			format = cFormat = li->internalFormat;
			for( unsigned int ix = 0; ix < mipMapCount; ++ix ) {
				fread( data, 1, size, f );
				glCompressedTexImage2D( GL_TEXTURE_2D, ix, li->internalFormat, x, y, 0, size, data );
				gl->updateError();
				x = (x+1)>>1;
				y = (y+1)>>1;
				size = max( li->divSize, x )/li->divSize * max( li->divSize, y )/li->divSize * li->blockBytes;
			}
			free( data );
			*/
		} else if( li->palette ) {
			printf ("palette\n");
			/*
			//  currently, we unpack palette into BGRA
			//  I'm not sure we always get pitch...
			assert( hdr.dwFlags & DDSD_PITCH );
			assert( hdr.sPixelFormat.dwRGBBitCount == 8 );
			size_t size = hdr.dwPitchOrLinearSize * ySize;
			//  And I'm even less sure we don't get padding on the smaller MIP levels...
			assert( size == x * y * li->blockBytes );
			format = li->externalFormat;
			cFormat = li->internalFormat;
			unsigned char * data = (unsigned char *)malloc( size );
			unsigned int palette[ 256 ];
			unsigned int * unpacked = (unsigned int *)malloc( size*sizeof( unsigned int ) );
			fread( palette, 4, 256, f );
			for( unsigned int ix = 0; ix < mipMapCount; ++ix ) {
				fread( data, 1, size, f );
				for( unsigned int zz = 0; zz < size; ++zz ) {
				unpacked[ zz ] = palette[ data[ zz ] ];
				}
				glPixelStorei( GL_UNPACK_ROW_LENGTH, y );
				glTexImage2D( GL_TEXTURE_2D, ix, li->internalFormat, x, y, 0, li->externalFormat, li->type, unpacked );
				gl->updateError();
				x = (x+1)>>1;
				y = (y+1)>>1;
				size = x * y * li->blockBytes;
			}
			free( data );
			free( unpacked );
			*/  
		} else {
			if( li->swap ) {
			printf ("swap\n");

			/*
			glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_TRUE );
			*/
			}
			size = x * y * li->blockBytes;

			printf ("size is %d\n",size);
			/*
			format = li->externalFormat;
			cFormat = li->internalFormat;
			unsigned char * data = (unsigned char *)malloc( size );
			//fixme: how are MIP maps stored for 24-bit if pitch != ySize*3 ?
			for( unsigned int ix = 0; ix < mipMapCount; ++ix ) {
				fread( data, 1, size, f );
				glPixelStorei( GL_UNPACK_ROW_LENGTH, y );
				glTexImage2D( GL_TEXTURE_2D, ix, li->internalFormat, x, y, 0, li->externalFormat, li->type, data );
				gl->updateError();
				x = (x+1)>>1;
				y = (y+1)>>1;
				size = x * y * li->blockBytes;
			}
			free( data );
			glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
			gl->updateError();
			*/
		}
		/*
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1 );
		gl->updateError();

		return true;

		failure:
		return false;
		}
		*/

	} else {
		printf ("put in the dummy file here, and call it quits\n");
	}
	FREE_IF_NZ(buffer);
	return FALSE;
}



/****************************************************************************
 *
 * ImageCubeMapTextures
 * notes - we make 6 PixelTextures, and actually put the data for each face 
 * into these pixelTextures. 
 *
 * yes, maybe there is a better way; this way mimics the ComposedCubeMap
 * method, and makes rendering both ImageCubeMap and ComposedCubeMapTextures
 * the same, in terms of scene-graph traversal.
 *
 * look carefully at the call to unpackImageCubeMap...
 *
 ****************************************************************************/
 void add_node_to_broto_context(struct X3D_Proto *currentContext,struct X3D_Node *node);

void compile_ImageCubeMapTexture (struct X3D_ImageCubeMapTexture *node) {
	if (node->__subTextures.n == 0) {
		int i;

		/* printf ("changed_ImageCubeMapTexture - creating sub-textures\n"); */
		FREE_IF_NZ(node->__subTextures.p); /* should be NULL, checking */
		node->__subTextures.p = MALLOC(struct X3D_Node  **,  6 * sizeof (struct X3D_PixelTexture *));
		for (i=0; i<6; i++) {
			struct X3D_PixelTexture *pt;
			//struct textureTableIndexStruct *tti;
			pt = (struct X3D_PixelTexture *)createNewX3DNode(NODE_PixelTexture);
			node->__subTextures.p[i] = X3D_NODE(pt);
			if(node->_executionContext)
				add_node_to_broto_context(X3D_PROTO(node->_executionContext),X3D_NODE(node->__subTextures.p[i]));
			//tti = getTableIndex(pt->__textureTableIndex);
			//tti->status = TEX_NEEDSBINDING; //I found I didn't need - yet
		}
		node->__subTextures.n=6;
	}

	/* tell the whole system to re-create the data for these sub-children */
	node->__regenSubTextures = TRUE;
	MARK_NODE_COMPILED
}


void render_ImageCubeMapTexture (struct X3D_ImageCubeMapTexture *node) {
	int count, iface;

	COMPILE_IF_REQUIRED

	/* do we have to split this CubeMap raw data apart? */
	if (node->__regenSubTextures) {
		/* Yes! Get the image data from the file, and split it apart */
		loadTextureNode(X3D_NODE(node),NULL);
	} else {
		/* we have the 6 faces from the image, just go through and render them as a cube */
		if (node->__subTextures.n == 0) return; /* not generated yet - see changed_ImageCubeMapTexture */

		for (count=0; count<6; count++) {

			/* set up the appearanceProperties to indicate a CubeMap */
			getAppearanceProperties()->cubeFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+count;

			/* go through these, back, front, top, bottom, right left */
			iface = lookup_xxyyzz_face_from_count[count];
			render_node(node->__subTextures.p[iface]);
		}
	}
    /* Finished rendering CubeMap, set it back for normal textures */
    getAppearanceProperties()->cubeFace = 0; 

}


/* textures - we have got a png (jpeg, etc) file with a cubemap in it; eg, see:
	http://en.wikipedia.org/wiki/Cube_mapping
*/
/* images are stored in an image as 3 "rows", 4 "columns", we pick the data out of these columns */
static int offsets[]={
  /*y,x,   with y-up    */
	1,2,	/* right 	*/
	1,0,	/* left 	*/
	2,1,	/* top		*/
	0,1,	/* bottom	*/
	1,1,	/* front	*/
	1,3};	/* back		*/
//if assuming the offsets order represents +x,-x,+y,-y,+z,-z then this is LHS (left handed system)
/* or:
	----    Top     --      --
	Left    Front   Right   Back
	----    Down    --      --
*/

/* fill in the 6 PixelTextures from the data in the texture 
	this is for when you have a single .png image with 6 sub-patches
*/
void unpackImageCubeMap (textureTableIndexStruct_s* me) {
	int size;
	int count;

	struct X3D_ImageCubeMapTexture *node = (struct X3D_ImageCubeMapTexture *)me->scenegraphNode;

	if (node == NULL) { 
		ERROR_MSG("problem unpacking single image ImageCubeMap\n");
		return; 
	}

	if (node->_nodeType != NODE_ImageCubeMapTexture) {
		ERROR_MSG("internal error - expected ImageCubeMapTexture here");
		return;
	}

	/* expect the cube map to be in a 4:3 ratio */
	/* printf ("size %dx%d, data %p\n",me->x, me->y, me->texdata); */
	if ((me->x * 3) != (me->y*4)) {
		ERROR_MSG ("expect an ImageCubeMap to be in a 4:3 ratio");
		return;
	}

	/* ok, we have, probably, a cube map in the image data. Extract the data and go nuts */
	size = me->x / 4;


	if (node->__subTextures.n != 6) {
		ERROR_MSG("unpackImageCubeMap, there should be 6 PixelTexture nodes here\n");
		return;
	}
	/* go through each face, and send the data to the relevant PixelTexture */
	/* order: right left, top, bottom, back, front */
	for (count=0; count <6; count++) {
		int x,y;
		uint32 val;
		uint32 *tex = (uint32 *) me->texdata;
		struct X3D_PixelTexture *pt = X3D_PIXELTEXTURE(node->__subTextures.p[count]);
		int xSubIndex, ySubIndex;
		int index;

		ySubIndex=offsets[count*2]*size; xSubIndex=offsets[count*2+1]*size;

		/* create the MFInt32 array for this face in the PixelTexture */
		FREE_IF_NZ(pt->image.p);
		pt->image.n = size*size+3;
		pt->image.p = MALLOC(int *, pt->image.n * sizeof (int));
		pt->image.p[0] = size;
		pt->image.p[1] = size;
		pt->image.p[2] = 4; /* this last one is for RGBA nchannels/components = 4 */
		index = 3;

		for (y=ySubIndex; y<ySubIndex+size; y++) {
			for (x=xSubIndex; x<xSubIndex+size; x++) {
				int ipix;
				unsigned char *rgba;
				ipix = y*me->x + x; //pixel in big image
				if(0){
					/* remember, this will be in ARGB format, make into RGBA */
					val = tex[ipix];
					pt->image.p[index] = ((val & 0xffffff) << 8) | ((val & 0xff000000) >> 24); 
				}else{
					rgba = (unsigned char *)&tex[ipix];
					//convert to host-endian red-high int
					pt->image.p[index] = (rgba[0] << 24) + (rgba[1] << 16) + (rgba[2] << 8) + (rgba[3] << 0);
				}
				/* printf ("was %x, now %x\n",tex[x*me->x+y], pt->image.p[index]); */
				index ++;
			}

		}
	}

	/* we are now locked-n-loaded */
	node->__regenSubTextures = FALSE;

	/* get rid of the original texture data now */
	FREE_IF_NZ(me->texdata);
}


void unpackImageCubeMap6 (textureTableIndexStruct_s* me) {
	//for .DDS and .web3dit that are in cubemap format ie 6 contiguous images in tti->teximage
	// incoming order of images: +x,-x,+y,-y,+z,-z (or R,L,F,B,T,D ?) */
	int size;
	int count;

	struct X3D_ImageCubeMapTexture *node = (struct X3D_ImageCubeMapTexture *)me->scenegraphNode;

	if (node == NULL) { 
		ERROR_MSG("problem unpacking single image ImageCubeMap\n");
		return; 
	}

	if (node->_nodeType != NODE_ImageCubeMapTexture) {
		ERROR_MSG("internal error - expected ImageCubeMapTexture here");
		return;
	}


	if (node->__subTextures.n != 6) {
		ERROR_MSG("unpackImageCubeMap, there should be 6 PixelTexture nodes here\n");
		return;
	}
	/* go through each face, and send the data to the relevant PixelTexture */
	/* (jas declared target) order: right left, top, bottom, back, front */
	// (dug9 incoming order from dds/.web3dit cubemap texture, RHS: +x,-x,+y,-y,+z,-z
	// this should be same as opengl/web3d order
	uint32 imlookup[] = {0,1,2,3,4,5}; //dug9 lookup order that experimentally seems to work
	for (count=0; count <6; count++) {
		int x,y,i,j,k;
		uint32 val, ioff;
		uint32 *tex;
		struct X3D_PixelTexture *pt = X3D_PIXELTEXTURE(node->__subTextures.p[count]);

		/* create the MFInt32 array for this face in the PixelTexture */
		FREE_IF_NZ(pt->image.p);
		pt->image.n = me->x*me->y+3;
		pt->image.p = MALLOC(int *, pt->image.n * sizeof (uint32));
		pt->image.p[0] = me->x;
		pt->image.p[1] = me->y;
		pt->image.p[2] = 4; /* this last one is for RGBA */
		ioff = imlookup[count] * me->x * me->y;
		//we are in char rgba order, but we need to convert to endian-specific uint32
		// which is what texture_load_from_pixelTexture() will be expecting
		//in imageIsDDS() image reader, we already flipped from top-down image to bottom-up texture order
		// which pixeltexture is expecting
		tex = (uint32 *) me->texdata;
		tex = &tex[ioff];
		for(j=0;j<me->y;j++){
			for(i=0;i<me->x;i++){
				int ipix,jpix;
				uint32 pixint;
				unsigned char* rgba;

				ipix = j*me->x + i;  //image row same as image row out
				//jpix = (me->y-1 -j)*me->x + i;  //flip image vertically - no, pixeltexture is bottom-up like incoming
				rgba = (unsigned char*)&tex[ipix];
				pixint = (rgba[0] << 24) + (rgba[1] << 16) + (rgba[2] << 8) + rgba[3];
				pt->image.p[ipix+3] = pixint;
			}
		}
	}

	/* we are now locked-n-loaded */
	node->__regenSubTextures = FALSE;

	/* get rid of the original texture data now */
	FREE_IF_NZ(me->texdata);
}



/****************************************************************************
 *
 * GeneratedCubeMapTextures
 *
 ****************************************************************************/
 #include "RenderFuncs.h"
 typedef struct pComponent_CubeMapTexturing{
	Stack * gencube_stack;
}* ppComponent_CubeMapTexturing;
void *Component_CubeMapTexturing_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_CubeMapTexturing));
	memset(v,0,sizeof(struct pComponent_CubeMapTexturing));
	return v;
}
void Component_CubeMapTexturing_init(struct tComponent_CubeMapTexturing *t){
	//public
	//private
	t->prv = Component_CubeMapTexturing_constructor();
	{
		ppComponent_CubeMapTexturing p = (ppComponent_CubeMapTexturing)t->prv;
		p->gencube_stack = newStack(usehit);
	}
}
void Component_CubeMapTexturing_clear(struct tComponent_CubeMapTexturing *t){
	//public
	//private
	{
		ppComponent_CubeMapTexturing p = (ppComponent_CubeMapTexturing)t->prv;
		deleteVector(usehit,p->gencube_stack);
	}
}
//ppComponent_CubeMapTexturing p = (ppComponent_CubeMapTexturing)gglobal()->Component_CubeMapTexturing.prv;

 // uni_string update ["NONE"|"NEXT_FRAME_ONLY"|"ALWAYS"]
 // int size
void compile_GeneratedCubeMapTexture (struct X3D_GeneratedCubeMapTexture *node) {
	if (node->__subTextures.n == 0) {
		int i;
		struct textureTableIndexStruct *tti;

		/* printf ("changed_ImageCubeMapTexture - creating sub-textures\n"); */
		FREE_IF_NZ(node->__subTextures.p); /* should be NULL, checking */
		node->__subTextures.p = MALLOC(struct X3D_Node  **,  6 * sizeof (struct X3D_PixelTexture *));
		for (i=0; i<6; i++) {
			struct X3D_PixelTexture *pt;
			pt = (struct X3D_PixelTexture *)createNewX3DNode(NODE_PixelTexture);
			node->__subTextures.p[i] = X3D_NODE(pt);
			if(node->_executionContext)
				add_node_to_broto_context(X3D_PROTO(node->_executionContext),X3D_NODE(node->__subTextures.p[i]));
			//tti = getTableIndex(pt->__textureTableIndex);
			//tti->status = TEX_NEEDSBINDING; //I found I didn't need - yet
			//tti->z = 6;

		}
		node->__subTextures.n=6;
		tti = getTableIndex(node->__textureTableIndex);
		tti->status = TEX_NEEDSBINDING; //I found I didn't need - yet
		//tti->z = 6;
	}

	/* tell the whole system to re-create the data for these sub-children */
	node->__regenSubTextures = TRUE;

	MARK_NODE_COMPILED
}
void render_GeneratedCubeMapTexture (struct X3D_GeneratedCubeMapTexture *node) {

	if(!strcmp(node->update->strptr,"ALWAYS") || !strcmp(node->update->strptr,"NEXT_FRAME_ONLY")){
		ttrenderstate rs;
		rs = renderstate();
		if(rs->render_geom && !rs->render_cube){
			//add (node,modelviewmatrix) for next frame
			//programmer: please clear the gencube stack once per frame
			int i, isAdded;
			usehit uhit;
			double modelviewMatrix[16];
			ppComponent_CubeMapTexturing p = (ppComponent_CubeMapTexturing)gglobal()->Component_CubeMapTexturing.prv;	
			//check if already added, only add once for simplification
			isAdded = FALSE;
			for(i=0;i<vectorSize(p->gencube_stack);i++){
				uhit = vector_get(usehit,p->gencube_stack,i);
				if(uhit.node == X3D_NODE(node)){
					 isAdded = TRUE;
					 break;
				}
			}
			if(!isAdded){
				//GL_GET_MODELVIEWMATRIX
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
				//strip viewmatrix - will happen when we invert one of the USEUSE pair, and multiply
				usehit uhit;
				uhit.node = X3D_NODE(node);
				memcpy(uhit.mvm,modelviewMatrix,16*sizeof(double)); //deep copy
				vector_pushBack(usehit,p->gencube_stack,uhit);  //fat elements do another deep copy
			}
		}
	}
	//render what we have now
	int count, iface;

	COMPILE_IF_REQUIRED

	/* do we have to split this CubeMap raw data apart? */
	if (node->__regenSubTextures) {
		/* Yes! Get the image data from the file, and split it apart */
		loadTextureNode(X3D_NODE(node),NULL);
	} 
	//else 
	{
		/* we have the 6 faces from the image, just go through and render them as a cube */
		if (node->__subTextures.n == 0) return; /* not generated yet - see changed_ImageCubeMapTexture */

		for (count=0; count<6; count++) {

			/* set up the appearanceProperties to indicate a CubeMap */
			getAppearanceProperties()->cubeFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+count;

			/* go through these, back, front, top, bottom, right left */
			iface = lookup_xxyyzz_face_from_count[count];
			render_node(node->__subTextures.p[iface]);
		}
	}
    /* Finished rendering CubeMap, set it back for normal textures */
    getAppearanceProperties()->cubeFace = 0; 

}
//Stack *getGenCubeList(){
//	ppComponent_CubeMapTexturing p = (ppComponent_CubeMapTexturing)gglobal()->Component_CubeMapTexturing.prv;	
//	return p->gencube_stack;
//}
void pushnset_framebuffer(int ibuffer);
void popnset_framebuffer();

#ifdef GL_DEPTH_COMPONENT32
#define FW_GL_DEPTH_COMPONENT GL_DEPTH_COMPONENT32
#else
#define FW_GL_DEPTH_COMPONENT GL_DEPTH_COMPONENT16
#endif
int create_cubmapfbo(int isize){
	int useMip;
	GLint itexturebuffer,ibuffer, idepthbuffer;

	glGenTextures(1, &itexturebuffer);
		//bind to set some parameters
		glBindTexture(GL_TEXTURE_2D, itexturebuffer);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		useMip = 0;
		if(useMip){
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap generation included in OpenGL v1.4
		}else{
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		}
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, isize, isize, 0, GL_RGBA , GL_UNSIGNED_BYTE, 0);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		//unbind - will rebind during render to reset width, height as needed
		glBindTexture(GL_TEXTURE_2D, 0); 

	glGenFramebuffers(1, &ibuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, ibuffer);

	// create a renderbuffer object to store depth info
	// NOTE: A depth renderable image should be attached the FBO for depth test.
	// If we don't attach a depth renderable image to the FBO, then
	// the rendering output will be corrupted because of missing depth test.
	// If you also need stencil test for your rendering, then you must
	// attach additional image to the stencil attachement point, too.
	glGenRenderbuffers(1, &idepthbuffer);
		//bind to set some parameters
		glBindRenderbuffer(GL_RENDERBUFFER, idepthbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, FW_GL_DEPTH_COMPONENT, isize,isize);
		//unbind
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// attach a texture to FBO color attachement point
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, itexturebuffer, 0);

	// attach a renderbuffer to depth attachment point
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, idepthbuffer);
	//unbind framebuffer till render
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return ibuffer;
}
static struct {
double angle;
double x;
double y;
double z;
} sideangle[6] = {
{ 90.0,0.0,1.0,0.0}, //+x
{-90.0,0.0,1.0,0.0}, //-x
{ 90.0,1.0,0.0,0.0}, //+y
{-90.0,1.0,0.0,0.0}, //-y
{  0.0,0.0,1.0,0.0}, //+z (lhs)
{180.0,0.0,1.0,0.0}, //-z
};
void saveImage_web3dit(struct textureTableIndexStruct *tti, char *fname);
void generate_GeneratedCubeMapTextures(){
	//call from mainloop once per frame:
	//foreach cubemaptexture location in cubgen list
	//  foreach 6 sides
	//    set viewpoint pose
	//    render scene to fbo
	//  convert fbo to regular cubemap texture
	//clear cubegen list
	Stack *gencube_stack;
	ttglobal tg = gglobal();
	ppComponent_CubeMapTexturing p = (ppComponent_CubeMapTexturing)tg->Component_CubeMapTexturing.prv;	

	gencube_stack = p->gencube_stack;
	if(vectorSize(gencube_stack)){
		int i, j, n;

		n = vectorSize(gencube_stack);
		for(i=0;i<n;i++){
			usehit uhit;
			int isize;
			double modelviewmatrix[16];
			textureTableIndexStruct_s* tti;
			struct X3D_GeneratedCubeMapTexture * node;

			uhit = vector_get(usehit,gencube_stack,i);
			node = (struct X3D_GeneratedCubeMapTexture*)uhit.node;
			isize = node->size;
			memcpy(modelviewmatrix,uhit.mvm,16*sizeof(double));
			tti = getTableIndex(node->__textureTableIndex);
			//set size of tile
			if(tti->ifbobuffer == 0){
				// https://www.opengl.org/wiki/Framebuffer_Object
				glGenFramebuffers(1, &tti->ifbobuffer);
				pushnset_framebuffer(tti->ifbobuffer); //binds framebuffer. we push here, in case higher up we are already rendering the whole scene to an fbo

				glGenRenderbuffers(1, &tti->idepthbuffer);
				glBindRenderbuffer(GL_RENDERBUFFER, tti->idepthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, FW_GL_DEPTH_COMPONENT, isize,isize);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tti->idepthbuffer);

				for(j=0;j<node->__subTextures.n;j++){  //should be 6
					textureTableIndexStruct_s* ttip;
					struct X3D_PixelTexture * nodep;
					nodep = (struct X3D_PixelTexture *)node->__subTextures.p[j];
					ttip = getTableIndex(nodep->__textureTableIndex);
					glGenTextures(1,&ttip->OpenGLTexture);
					glBindTexture(GL_TEXTURE_2D, ttip->OpenGLTexture);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, isize, isize, 0, GL_RGBA , GL_UNSIGNED_BYTE, 0);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+j, GL_TEXTURE_2D, ttip->OpenGLTexture, 0);
				}
				popnset_framebuffer(tti->ifbobuffer);
			}
			pushnset_framebuffer(tti->ifbobuffer); //binds framebuffer. we push here, in case higher up we are already rendering the whole scene to an fbo

			if(isize != tti->x ){
				//size change 
			}
			//create fbo or fbo tiles collection for generatedcubemap
			for(j=0;j<node->__subTextures.n;j++){  //should be 6
				textureTableIndexStruct_s* ttip;
				struct X3D_PixelTexture * nodep;

				nodep = (struct X3D_PixelTexture *)node->__subTextures.p[j];
				ttip = getTableIndex(nodep->__textureTableIndex);
				//glBindTexture(GL_TEXTURE_2D, ttip->OpenGLTexture);
				//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ttip->OpenGLTexture, 0);
				glClearColor(1.0f,0.0f,0.0f,1.0f);
				FW_GL_CLEAR(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				//set viewpoint matrix for side
				//setup_projection(); 
				FW_GL_MATRIX_MODE(GL_PROJECTION);
				FW_GL_LOAD_IDENTITY();
				fw_gluPerspective_2(0.0,90.0, 1.0, 1.0,10000.0);
				FW_GL_MATRIX_MODE(GL_MODELVIEW);
				FW_GL_LOAD_IDENTITY();
				fw_glSetDoublev(GL_MODELVIEW_MATRIX, modelviewmatrix);
				fw_glRotated(sideangle[j].angle,sideangle[j].x,sideangle[j].y,sideangle[j].z);


				clearLightTable();//turns all lights off- will turn them on for VF_globalLight and scope-wise for non-global in VF_geom

				/*  turn light #0 off only if it is not a headlight.*/
				if (!fwl_get_headlight()) {
					setLightState(HEADLIGHT_LIGHT,FALSE);
					setLightType(HEADLIGHT_LIGHT,2); // DirectionalLight
				}

				/*  Other lights*/
				PRINT_GL_ERROR_IF_ANY("XEvents::render, before render_hier");

				render_hier(rootNode(), VF_globalLight );
				PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_globalLight)");
				render_hier(rootNode(), VF_Other );

				/*  4. Nodes (not the blended ones)*/
				profile_start("hier_geom");
				render_hier(rootNode(), VF_Geom | VF_Cube);
				profile_end("hier_geom");
				PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_Geom)");

				/*  5. Blended Nodes*/
				if (tg->RenderFuncs.have_transparency) {
					/*  render the blended nodes*/
					render_hier(rootNode(), VF_Geom | VF_Blend | VF_Cube);
					PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_Geom)");
				}

				//if you can figure out how to use regular texture in cubemap, then there may be a shortcut
				//for now, we'll pull the fbo pixels back into cpu space and put them in pixeltexture
				GLuint pixelType = GL_RGBA;
				int bytesPerPixel = 4;
				if(!ttip->texdata || ttip->x != isize){
					FREE_IF_NZ(ttip->texdata);
					ttip->texdata = MALLOC (GLvoid *, bytesPerPixel*isize*isize*sizeof(char));
				}

				/* grab the data */
				FW_GL_PIXELSTOREI (GL_UNPACK_ALIGNMENT, 1);
				FW_GL_PIXELSTOREI (GL_PACK_ALIGNMENT, 1);
	
				if(1){
					//color pixels green, to see if they show up
					for(int k=0;k<isize*isize;k++)
						ttip->texdata[k*4 + 1] = 255;
				}
				FW_GL_READPIXELS (0,0,isize,isize,pixelType,GL_UNSIGNED_BYTE, ttip->texdata);
				if(0){
					//color pixels green, to see if they show up
					for(int k=0;k<isize*isize;k++)
						ttip->texdata[k*4 + 1] = 255;
				}
				ttip->x = isize;
				ttip->y = isize;
				ttip->z = 1;
				ttip->hasAlpha = 1;
				ttip->channels = 4;
				ttip->status = TEX_NEEDSBINDING;
				if(0){
					//write out tti as web3dit image file for diagnostic viewing
					//void saveImage_web3dit(struct textureTableIndexStruct *tti, char *fname)
					static int framecount = 0;
					framecount++;
					if(framecount == 50){
						char namebuf[100];
						sprintf(namebuf,"%s%d.web3dit","cubemapface_",j);
						saveImage_web3dit(ttip, namebuf);
					}
				}
			}
			popnset_framebuffer();
			//compile_generatedcubemaptexture // convert to opengl
		}
		//clear cubegen list
		gencube_stack->n = 0;
	}
}