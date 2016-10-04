/*

  FreeWRL support library.

Purpose:
  Handle platform specific includes about windowing systems and OpenGL.
  Try to present a generic interface to the rest of FreeWRL library.

Data:

Functions:
  
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

#if 1 || defined(HAVE_FV_INLIB)
#define KEEP_FV_INLIB 1
#define KEEP_X11_INLIB 1
#else
#define KEEP_FV_INLIB 0
#define KEEP_X11_INLIB 0
#endif

#ifndef __LIBFREEWRL_DISPLAY_H__
#define __LIBFREEWRL_DISPLAY_H__

/* this is platform-dependent but there is no default so set one here */
#ifndef MAX_MULTITEXTURE
#define MAX_MULTITEXTURE 4
#endif

#ifdef GL_ES_VERSION_2_0
#define MAX_LIGHTS 6
#define STR_MAX_LIGHTS "\n#define MAX_LIGHTS 6\n "
#define MAX_LIGHT_STACK 8 //making this larger than MAX_LIGHTS means we can visit all the local lights and use the last one on the stack/most local (or 2 if headlight off)
#define HEADLIGHT_LIGHT (MAX_LIGHT_STACK-1)
#else
#define MAX_LIGHTS 8 //8 lights is 1152bytes transfered to GPU shader per shape draw - 11% of mainloop load on pentium class PC with card in old-style 32bit PCI expansion slot
#define STR_MAX_LIGHTS "\n#define MAX_LIGHTS 8\n "
#define MAX_LIGHT_STACK 8 //going down the transform stack, up to 7 local lights, with most-local as last
#define HEADLIGHT_LIGHT (MAX_LIGHT_STACK-1)
#endif

#ifdef AQUA // OLD_IPHONE_AQUA
OLD_IPHONE_AQUA /**
OLD_IPHONE_AQUA  * Specific platform : Mac
OLD_IPHONE_AQUA  */
OLD_IPHONE_AQUA 
OLD_IPHONE_AQUA #ifdef IPHONE
OLD_IPHONE_AQUA #include <OpenGLES/ES2/gl.h>
OLD_IPHONE_AQUA #include <OpenGLES/ES2/glext.h>
OLD_IPHONE_AQUA #include <OpenGLES/ES3/gl.h>
OLD_IPHONE_AQUA #include <OpenGLES/ES3/glext.h>
OLD_IPHONE_AQUA #else
OLD_IPHONE_AQUA 
OLD_IPHONE_AQUA #include <OpenGL/OpenGL.h>
OLD_IPHONE_AQUA #include <OpenGL/CGLTypes.h>
OLD_IPHONE_AQUA 
OLD_IPHONE_AQUA #include <AGL/AGL.h> 
OLD_IPHONE_AQUA #endif /* defined IPHONE */
#endif /* defined TARGET_AQUA OLD_IPHONE_AQUA */

#include <libFreeWRL.h>

/* Some Windows-specific stuff */
#if defined(_MSC_VER) && defined(HAVE_GLEW_H) && !defined(ANGLEPROJECT)
#define GLEW_NO_GLU 1
#include <GL/glew.h>
#ifdef GLEW_MX
GLEWContext * glewGetContext();
#endif
#define ERROR 0
#endif /* TARGET_WIN32 */

#if defined(__linux__) && !defined(_ANDROID) && !defined(ANDROIDNDK)
#  define GL_GLEXT_PROTOTYPES 1
#  include <GL/gl.h>
//JAS #  include <GL/glu.h>
//JAS #  include <GL/glext.h>
#include <../libtess/libtess2.h>

# include <GL/glx.h>
/* original bits that were here; the above was moved from linux-specific section below 
	 define GL_GLEXT_PROTOTYPES 1
	 include <GL/gl.h>
	 include <GL/glext.h>
	 include <GL/glx.h> */
#endif

#if defined (_ANDROID) || defined(ANDROIDNDK) || defined (QNX) || defined(ANGLEPROJECT)
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
//    #include <GLES3/gl3.h>
//	#include <GLES3/gl3ext.h>
#endif




/* generic - OpenGL ES 2.0 does not have doubles */
#ifdef GL_ES_VERSION_2_0
	#define GLDOUBLE double
	#define DOUBLE_MAX fmax
	#define DOUBLE_MIN fmin
#else
	#define GLDOUBLE GLdouble
	#define DOUBLE_MAX max
	#define DOUBLE_MIN min
#endif


/* face culling */
#define CULL_FACE(v) /* printf ("nodeSolid %d getAppearanceProperties()->cullFace %d GL_FALSE %d FALSE %d\n",v,getAppearanceProperties()->cullFace,GL_FALSE,FALSE); */ \
                if (v != getAppearanceProperties()->cullFace) {    \
                        getAppearanceProperties()->cullFace = v; \
                        if (getAppearanceProperties()->cullFace == TRUE) {glEnable(GL_CULL_FACE);}\
                        else { glDisable(GL_CULL_FACE);} \
                }
	#define CULL_FACE_INITIALIZE getAppearanceProperties()->cullFace=FALSE; glDisable(GL_CULL_FACE);

#define DISABLE_CULL_FACE CULL_FACE(FALSE)
#define ENABLE_CULL_FACE CULL_FACE(TRUE)

#define GL_LIGHT_RADIUS                   0xBEEF /* smile - my definition */
#define GL_SPOT_BEAMWIDTH                   0xF00D /* smile - my definition */
#ifdef GL_ES_VERSION_2_0
#if !defined(PATH_MAX)
	#define PATH_MAX 5000
#endif

	/* as we now do our own matrix manipulation, we can change these; note that OpenGL-ES 2.0 does not
	   have these by default */
	#define GL_MODELVIEW                   0x1700
	#define GL_MODELVIEW_MATRIX            0x0BA6
	#define GL_PROJECTION                  0x1701
	#define GL_PROJECTION_MATRIX           0x0BA7
	#define GL_TEXTURE_MATRIX              0x0BA8

	/* same with material properties - we do our own, but need some constants, so... */
	#define GL_SHININESS                      0x1601
	#define GL_DIFFUSE                        0x1201
	#define GL_AMBIENT                        0x1200
	#define GL_SPECULAR                       0x1202
	#define GL_EMISSION                       0x1600
	#define GL_ENABLE_BIT				0x00002000

	#define GL_LIGHT_MODEL_COLOR_CONTROL		0x81F8
	#define GL_SEPARATE_SPECULAR_COLOR		0x81FA
	#define GL_LIGHT_MODEL_TWO_SIDE			0x0B52
	#define GL_LIGHT_MODEL_LOCAL_VIEWER		0x0B51
	#define GL_LIGHT_MODEL_AMBIENT			0x0B53
	#define GL_LIGHT0                         0x4000

	/* and, one buffer only, not stereo viewing */
	#define GL_BACK_LEFT			GL_BACK
	#define GL_BACK_RIGHT			GL_BACK
	#define GL_STEREO                         0x0C33

	/* we do not do occlusion queries yet; have to figure this one out */
	#define GL_QUERY_RESULT                   0x8866
	#define GL_QUERY_RESULT_AVAILABLE         0x8867
	#define GL_SAMPLES_PASSED                 0x8914

	/* and, we have shaders, but not OpenGL 2.0, so we just put these here */
	#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
	#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
	#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
	#define GL_RGBA8				0x8058
	#define GL_RGB8					0x8051
	#define GL_BGR                            0x80E0
	#define GL_RGB5					0x8050

#define GL_EDGE_FLAG_ARRAY			0x8079
#define GL_INDEX_ARRAY				0x8077
#define GL_FOG_COORD_ARRAY                GL_FOG_COORDINATE_ARRAY
#define GL_SECONDARY_COLOR_ARRAY          0x845E
	#define GL_LINE_STIPPLE                   0x0B24
	#define GL_VERTEX_ARRAY                   0x8074
	#define GL_NORMAL_ARRAY                   0x8075
	#define GL_TEXTURE_COORD_ARRAY            0x8078
	#define GL_COLOR_ARRAY                    0x8076
	#define GL_OBJECT_LINEAR                  0x2401
	#define GL_EYE_LINEAR                     0x2400
	#define GL_REFLECTION_MAP                 0x8512
	#define GL_SPHERE_MAP                     0x2402
	#define GL_NORMAL_MAP                     0x8511
	#define GL_S                              0x2000
	#define GL_TEXTURE_GEN_MODE               0x2500
	#define GL_T                              0x2001
	#define GL_TEXTURE_GEN_S                  0x0C60
	#define GL_TEXTURE_GEN_T                  0x0C61
	#define GL_TEXTURE_ENV                    0x2300
	#define GL_TEXTURE_ENV_MODE               0x2200
	#define GL_MODULATE                       0x2100
	#define GL_COMBINE                        0x8570
	#define GL_COMBINE_RGB                    0x8571
	#define GL_SOURCE0_RGB                    0x8580
	#define GL_OPERAND0_RGB                   0x8590
	#define GL_SOURCE1_RGB                    0x8581
	#define GL_OPERAND1_RGB                   0x8591
	#define GL_COMBINE_ALPHA                  0x8572
	#define GL_SOURCE0_ALPHA                  0x8588
	#define GL_OPERAND0_ALPHA                 0x8598
	#define GL_RGB_SCALE                      0x8573
	#define GL_ALPHA_SCALE                    0x0D1C
	#define GL_SOURCE1_ALPHA                  0x8589
	#define GL_OPERAND1_ALPHA                 0x8599
	#define GL_TEXTURE_GEN_S                  0x0C60
	#define GL_TEXTURE_GEN_T                  0x0C61
	#define GL_PREVIOUS                       0x8578
	#define GL_ADD                            0x0104
	#define GL_SUBTRACT                       0x84E7
	#define GL_DOT3_RGB                       0x86AE
	#define GL_ADD_SIGNED                     0x8574
	#define GL_CLAMP                          0x2900
	#define GL_CLAMP_TO_BORDER                0x812D
	#define GL_TEXTURE_WRAP_R                 0x8072
	#define GL_R                              0x2002
	#define GL_TEXTURE_GEN_R                  0x0C62
	#define GL_GENERATE_MIPMAP                0x8191
	#define GL_TEXTURE_PRIORITY               0x8066
	#define GL_TEXTURE_BORDER_COLOR           0x1004
	#define GL_TEXTURE_INTERNAL_FORMAT        0x1003
	#define GL_COMPRESSED_RGBA                0x84EE
	#define GL_TEXTURE_COMPRESSION_HINT       0x84EF
	#define GL_PROXY_TEXTURE_2D               0x8064
	#define GL_TEXTURE_WIDTH                  0x1000
	#define GL_TEXTURE_HEIGHT                 0x1001
	#define GL_POSITION                       0x1203
	#define GL_SPOT_DIRECTION                 0x1204

	#define GL_CONSTANT_ATTENUATION           0x1207
	#define GL_LINEAR_ATTENUATION             0x1208
	#define GL_QUADRATIC_ATTENUATION          0x1209
	#define GL_SPOT_CUTOFF                    0x1206
	#define GL_COMPILE                        0x1300
	#define GL_FLAT                           0x1D00
	#define GL_SMOOTH                         0x1D01
	#define GL_LIST_BIT                   0x00020000

	#define VERTEX_SHADER GL_VERTEX_SHADER
	#define FRAGMENT_SHADER GL_FRAGMENT_SHADER
	#define SHADER_SOURCE glShaderSource
	#define COMPILE_SHADER glCompileShader
	#define CREATE_PROGRAM glCreateProgram();
	#define CREATE_SHADER glCreateShader
	#define ATTACH_SHADER glAttachShader
	#define LINK_SHADER glLinkProgram
	#define DELETE_SHADER glDeleteShader
	#define DELETE_PROGRAM glDeleteProgram
	#define USE_SHADER(aaa) glUseProgram(aaa)
	#define GET_SHADER_INFO glGetShaderiv
	#define LINK_STATUS GL_LINK_STATUS
	#define COMPILE_STATUS GL_COMPILE_STATUS
	#define GET_UNIFORM(aaa,bbb) glGetUniformLocation(aaa,bbb)
	#define GET_ATTRIB(aaa,bbb) glGetAttribLocation(aaa,bbb)
	#define GLUNIFORM1I glUniform1i
	#define GLUNIFORM1F glUniform1f
	#define GLUNIFORM2F glUniform2f
	#define GLUNIFORM3F glUniform3f
	#define GLUNIFORM4F glUniform4f
	#define GLUNIFORM1IV glUniform1iv
	#define GLUNIFORM1FV glUniform1fv
	#define GLUNIFORM2FV glUniform2fv
	#define GLUNIFORM3FV glUniform3fv
	#define GLUNIFORM4FV glUniform4fv
	#define GLUNIFORMMATRIX4FV glUniformMatrix4fv
	#define GLUNIFORMMATRIX3FV glUniformMatrix3fv
#endif

/* OLD_IPHONE_AQUA
   OLD_IPHONE_AQUA #if defined (_MSC_VER) || defined (TARGET_AQUA) || defined(IPHONE) || defined(_ANDROID) || defined(ANDROIDNDK) || defined(QNX) */
#if defined (_MSC_VER) || defined(_ANDROID) || defined(ANDROIDNDK) || defined(QNX)  /* not win32, ie linux */
	#include <libtess2.h>
#endif // linux spefcific for now

/* Main initialization function */
/* int display_initialize(); */
#define IS_DISPLAY_INITIALIZED (gglobal()->display.display_initialized==TRUE)

/**
 * Sort of "virtual" functions
 *
 * TARGET_X11    : ui/fwBareWindow.c
 * TARGET_MOTIF  : ui/fwMotifWindow.c
 * TARGET_WIN32  : ui/fwWindow32.c
 */

/* are we doing Vertex Buffer Objects? (VBOs) for OpenGL? */
//#define VERTEX_VBO 0
//#define NORMAL_VBO 1
//#define TEXTURE_VBO 2
//#define INDEX_VBO 3
//#define COLOR_VBO 4
//#define FOG_VBO 5
//#define VBO_COUNT 6
#define VERTEX_VBO 0
#define NORMAL_VBO 1
#define INDEX_VBO 2
#define COLOR_VBO 3
#define FOG_VBO 4
#define TEXTURE_VBO0 5
#define TEXTURE_VBO1 6
#define TEXTURE_VBO2 7
#define TEXTURE_VBO3 8
#define VBO_COUNT 9


void fv_setScreenDim(int wi, int he);

int fv_open_display();
int fv_display_initialize(void);
int fv_display_initialize_desktop(void);
int fv_create_main_window(freewrl_params_t *d); //int argc, char *argv[]);
int fv_create_main_window2(freewrl_params_t *d, freewrl_params_t *share); 
bool fv_create_GLcontext();
bool fv_bind_GLcontext();
void fv_swapbuffers(freewrl_params_t *d);
int fv_create_window_and_context(freewrl_params_t *params, freewrl_params_t *share);
void fv_change_GLcontext(freewrl_params_t* d);
/* end of "virtual" functions */

/* OpenGL renderer capabilities */


typedef struct s_shader_capabilities{
	GLint compiledOK;
	GLuint myShaderProgram;

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

	GLint myPointSize;
    
	// do we need to send down light information?
	bool  haveLightInShader; 

	GLint lightcount;
	//GLint lightType;
	GLint lightType[MAX_LIGHTS];
	GLint lightAmbient[MAX_LIGHTS];
	GLint lightDiffuse[MAX_LIGHTS];
	GLint lightSpecular[MAX_LIGHTS];
	GLint lightPosition[MAX_LIGHTS];
	GLint lightSpotDir[MAX_LIGHTS];
	GLint lightAtten[MAX_LIGHTS];
	//GLint lightConstAtten[MAX_LIGHTS];
	//GLint lightLinAtten[MAX_LIGHTS];
	//GLint lightQuadAtten[MAX_LIGHTS];
	GLint lightSpotCutoffAngle[MAX_LIGHTS];
	GLint lightSpotBeamWidth[MAX_LIGHTS];
	//GLint lightRadius;
	GLint lightRadius[MAX_LIGHTS];

	GLint ModelViewMatrix;
	GLint ProjectionMatrix;
	GLint NormalMatrix;
	GLint ModelViewInverseMatrix;
	GLint TextureMatrix[MAX_MULTITEXTURE];
	GLint Vertices;
	GLint Normals;
	GLint Colours;
	GLint TexCoords[MAX_MULTITEXTURE];
	GLint FogCoords; //Aug 2016

	GLint TextureUnit[MAX_MULTITEXTURE];
	GLint TextureMode[MAX_MULTITEXTURE];
	GLint TextureSource[MAX_MULTITEXTURE];
	GLint TextureFunction[MAX_MULTITEXTURE];
	GLint textureCount;
	GLint multitextureColor;

	/* texture3D */
	GLint tex3dDepth; //int nz or 3rd dimension, needed in shader for texture2D emulation of texture3D
	GLint tex3dUseVertex; //bool flag when no 3D texture coords supplied, vertex shader should use vertex
	GLint repeatSTR;
	GLint magFilter;

	/* fill properties */
	GLint hatchColour;
	GLint hatchPercent;
	GLint hatchScale;
	GLint filledBool;
	GLint hatchedBool;
	GLint algorithm;
    
	/* TextureCoordinateGenerator type */
	GLint texCoordGenType;

	GLint fogColor;  //Aug 2016
	GLint fogvisibilityRange;
	GLint fogScale;
	GLint fogType;
	GLint fogHaveCoords;

	GLint clipplanes; //Sept 2016
	GLint nclipplanes;

/* attributes - reduce redundant state chage calls on GPU */
/*
	need to ensure that all calls to glEnableVertexAttribArray
	are tagged for redundancy - eg, in statusbarHud.c, etc.
	before trying to reduce the glEnableVertexAttribArray and
	glDisableVertexAttribArray calls.

	bool vertexAttribEnabled;
	bool texCoordAttribEnabled;
	bool colourAttribEnabled;
	bool normalAttribEnabled;
*/
	
} s_shader_capabilities_t;

typedef struct {

	const char *renderer; /* replace GL_REN */
	const char *version;
	const char *vendor;
	const char *extensions;
	float versionf;
	bool have_GL_VERSION_1_1;
	bool have_GL_VERSION_1_2;
	bool have_GL_VERSION_1_3;
	bool have_GL_VERSION_1_4;
	bool have_GL_VERSION_1_5;
	bool have_GL_VERSION_2_0;
	bool have_GL_VERSION_2_1;
	bool have_GL_VERSION_3_0;

	bool av_multitexture; /* Multi textures available ? */
	bool av_npot_texture; /* Non power of 2 textures available ? */
	bool av_texture_rect; /* Rectangle textures available ? */
	bool av_occlusion_q;  /* Occlusion query available ? */
	
	int texture_units;
	int runtime_max_texture_size;
	int system_max_texture_size;
	float anisotropicDegree;
    
    GLboolean quadBuffer;        /* does platform support quadbuffer? */

} s_renderer_capabilities_t;

// JAS extern s_renderer_capabilities_t rdr_caps;

bool initialize_rdr_caps();
void initialize_rdr_functions();
void rdr_caps_dump(s_renderer_capabilities_t *rdr_caps);


#ifdef TARGET_AQUA /* OLD_IPHONE_AQUA */
OLD_IPHONE_AQUA #ifndef IPHONE
OLD_IPHONE_AQUA 
OLD_IPHONE_AQUA extern int ccurse;
OLD_IPHONE_AQUA extern int ocurse;
OLD_IPHONE_AQUA 
OLD_IPHONE_AQUA //#define SCURSE 1
OLD_IPHONE_AQUA //#define ACURSE 0
OLD_IPHONE_AQUA 
OLD_IPHONE_AQUA /* for handling Safari window changes at the top of the display event loop */
OLD_IPHONE_AQUA extern int PaneClipnpx;
OLD_IPHONE_AQUA extern int PaneClipnpy;
OLD_IPHONE_AQUA 
OLD_IPHONE_AQUA extern int PaneClipct;
OLD_IPHONE_AQUA extern int PaneClipcb;
OLD_IPHONE_AQUA extern int PaneClipcr;
OLD_IPHONE_AQUA extern int PaneClipcl;
OLD_IPHONE_AQUA extern int PaneClipwidth;
OLD_IPHONE_AQUA extern int PaneClipheight;
OLD_IPHONE_AQUA extern int PaneClipChanged;
OLD_IPHONE_AQUA 
OLD_IPHONE_AQUA #include "OpenGL/glu.h"
OLD_IPHONE_AQUA #endif
#endif /* OLD_IPHONE_AQUA TARGET_AQUA */

/**
 * Specific platform : Linux / UNIX
 */
#if defined(TARGET_X11) || defined(TARGET_MOTIF)

/**
 * X11 common: weither we use Motif or not
 */

# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/keysym.h>


extern GLXContext GLcx;

extern XEvent event;
extern long event_mask;
extern Display *Xdpy;
extern int Xscreen;
extern Window Xroot_window;
extern XVisualInfo *Xvi;
extern Colormap colormap;
extern Window Xwin;
extern Window GLwin;
extern XSetWindowAttributes attr;
extern unsigned long mask;
extern Atom WM_DELETE_WINDOW;

void handle_Xevents(XEvent event);

# ifdef HAVE_XF86_VMODE
#  include <X11/extensions/xf86vmode.h>
extern int vmode_nb_modes;
extern XF86VidModeModeInfo **vmode_modes;
extern int vmode_mode_selected;
# endif /* HAVE_XF86_VMODE */

# if defined(TARGET_MOTIF)

/**
 * Motif
 */
# include <X11/Intrinsic.h>
# include <Xm/Xm.h>

extern XtAppContext Xtcx;

void getMotifWindowedGLwin(Window *win);
# define GET_GLWIN getMotifWindowedGLwin(&GLwin)

# else /* defined(TARGET_MOTIF) */

/**
 * Only X11, no Motif
 */
# define GET_GLWIN getBareWindowedGLwin(&GLwin)

# endif /* defined(TARGET_MOTIF) */

#endif /* defined(TARGET_X11) || defined(TARGET_MOTIF) */

/**
 * General : all systems
 */

#if defined (FW_DEBUG)

	#if defined(_ANDROID)
		#define PRINT_GL_ERROR_IF_ANY(_where) { \
			GLenum _global_gl_err = glGetError(); \
			while (_global_gl_err != GL_NO_ERROR) { \
				if (_global_gl_err == GL_INVALID_ENUM) {DROIDDEBUG ("GL_INVALID_ENUM"); } \
				else if (_global_gl_err == GL_INVALID_VALUE) {DROIDDEBUG("GL_INVALID_VALUE"); } \
				else if (_global_gl_err == GL_INVALID_OPERATION) {DROIDDEBUG("GL_INVALID_OPERATION"); } \
				else if (_global_gl_err == GL_OUT_OF_MEMORY) {DROIDDEBUG("GL_OUT_OF_MEMORY"); } \
				else DROIDDEBUG("unknown error"); \
				DROIDDEBUG(" here: %s (%s:%d)\n", _where,__FILE__,__LINE__); \
				_global_gl_err = glGetError(); \
			} \
		} 

	#else
		#define PRINT_GL_ERROR_IF_ANY(_where) { \
			GLenum _global_gl_err = glGetError(); \
			while (_global_gl_err != GL_NO_ERROR) { \
				if (_global_gl_err == GL_INVALID_ENUM) {printf ("GL_INVALID_ENUM"); } \
				else if (_global_gl_err == GL_INVALID_VALUE) {printf ("GL_INVALID_VALUE"); } \
				else if (_global_gl_err == GL_INVALID_OPERATION) {printf ("GL_INVALID_OPERATION"); } \
				else if (_global_gl_err == GL_OUT_OF_MEMORY) {printf ("GL_OUT_OF_MEMORY"); } \
				else printf ("unknown error"); \
				printf(" here: %s (%s:%d)\n", _where,__FILE__,__LINE__); \
				_global_gl_err = glGetError(); \
			} \
		} 
	#endif

#else // FW_DEBUG
	#define PRINT_GL_ERROR_IF_ANY(_where) /* do nothing */
#endif

#define GL_ERROR_MSG (\
	(glGetError() == GL_NO_ERROR)?"":\
		(glGetError() == GL_INVALID_ENUM)?"GL_INVALID_ENUM":\
		(glGetError() == GL_INVALID_VALUE)?"GL_INVALID_VALUE":\
		(glGetError() == GL_INVALID_OPERATION)?"GL_INVALID_OPERATION":\
		(glGetError() == GL_OUT_OF_MEMORY)?"GL_OUT_OF_MEMORY":\
		"unknown GL_ERROR")

void resetGeometry();
/* void setScreenDim(int wi, int he); */

/* GLSL variables */
/* Versions 1.5 and above have shaders */
#ifdef GL_VERSION_2_0
	#define VERTEX_SHADER GL_VERTEX_SHADER
	#define FRAGMENT_SHADER GL_FRAGMENT_SHADER
	#define SHADER_SOURCE glShaderSource
	#define COMPILE_SHADER glCompileShader
	#define CREATE_PROGRAM glCreateProgram();
	#define CREATE_SHADER glCreateShader
	#define ATTACH_SHADER glAttachShader
	#define LINK_SHADER glLinkProgram
	#define DELETE_SHADER glDeleteShader
	#define DELETE_PROGRAM glDeleteProgram
	#define USE_SHADER(aaa) glUseProgram(aaa)
	#define GET_SHADER_INFO glGetShaderiv
	#define LINK_STATUS GL_LINK_STATUS
	#define COMPILE_STATUS GL_COMPILE_STATUS
	#define GET_UNIFORM(aaa,bbb) glGetUniformLocation(aaa,bbb)
	#define GET_ATTRIB(aaa,bbb) glGetAttribLocation(aaa,bbb)
	#define GLUNIFORM1I glUniform1i
	#define GLUNIFORM1F glUniform1f
	#define GLUNIFORM2F glUniform2f
	#define GLUNIFORM3F glUniform3f
	#define GLUNIFORM4F glUniform4f
	#define GLUNIFORM1IV glUniform1iv
	#define GLUNIFORM1FV glUniform1fv
	#define GLUNIFORM2FV glUniform2fv
	#define GLUNIFORM3FV glUniform3fv
	#define GLUNIFORM4FV glUniform4fv
	#define GLUNIFORMMATRIX4FV glUniformMatrix4fv
	#define GLUNIFORMMATRIX3FV glUniformMatrix3fv

#else
#ifdef GL_VERSION_1_5
	#define VERTEX_SHADER GL_VERTEX_SHADER_ARB
	#define FRAGMENT_SHADER GL_FRAGMENT_SHADER_ARB
	#define SHADER_SOURCE glShaderSourceARB
	#define COMPILE_SHADER glCompileShaderARB
	#define CREATE_PROGRAM glCreateProgramObjectARB();
	#define CREATE_SHADER glCreateShaderARB
	#define ATTACH_SHADER glAttachObjectARB
	#define LINK_SHADER glLinkProgramARB
	#define DELETE_SHADER glDeleteShaderARB
	#define DELETE_PROGRAM glDeleteProgramARB
	#define USE_SHADER(aaa) glUseProgramObjectARB(aaa)
	#define CREATE_SHADER glCreateShaderObjectARB
	#define GET_SHADER_INFO glGetObjectParameterivARB
	#define LINK_STATUS GL_OBJECT_LINK_STATUS_ARB
	#define COMPILE_STATUS GL_OBJECT_COMPILE_STATUS_ARB
	#define GET_UNIFORM(aaa,bbb) glGetUniformLocationARB(aaa,bbb)
	#define GET_ATTRIB(aaa,bbb) glGetAttribLocationARB(aaa,bbb)
	#define GLUNIFORM1F glUniform1fARB
	#define GLUNIFORM1I glUniform1iARB
	#define GLUNIFORM2F glUniform2fARB
	#define GLUNIFORM3F glUniform3fARB
	#define GLUNIFORM4F glUniform4fARB
	#define GLUNIFORM1IV glUniform1ivARB
	#define GLUNIFORM1FV glUniform1fvARB
	#define GLUNIFORM2FV glUniform2fvARB
	#define GLUNIFORM3FV glUniform3fvARB
	#define GLUNIFORM4FV glUniform4fvARB
	#define GLUNIFORMMATRIX4FV glUniformMatrix4fvARB
	#define GLUNIFORMMATRIX3FV glUniformMatrix3fvARB
#endif
#endif

/* OpenGL-2.x and OpenGL-3.x "desktop" systems calls */
	/****************************************************************/
	/* First - any platform specifics to do? 			*/
	/****************************************************************/

	#if defined(_MSC_VER) 
		void fwMessageLoop();
		#define FW_GL_SWAPBUFFERS fv_swapbuffers(gglobal()->display.params); //SwapBuffers(wglGetCurrentDC());
	#endif

#if KEEP_X11_INLIB
	#if defined (TARGET_X11) || defined (TARGET_MOTIF)
		#define FW_GL_SWAPBUFFERS fv_swapbuffers(gglobal()->display.params);
	#endif
#endif
	
#ifndef FW_GL_SWAPBUFFERS
#define FW_GL_SWAPBUFFERS  /* nothing */
#endif


	/****************************************************************/
	/* Second - things that might be specific to one platform;	*/
	/*	this is the "catch for other OS" here 			*/
	/****************************************************************/
        /* nothing here */

	/****************************************************************/
	/* Third - common across all platforms				*/
	/****************************************************************/


	/* GLU replacement - needs local matrix stacks, plus more code */
	#define FW_GLU_PERSPECTIVE(aaa,bbb,ccc,ddd) fw_gluPerspective(aaa,bbb,ccc,ddd)
	#define FW_GLU_UNPROJECT(aaa, bbb, ccc, ddd, eee, fff, ggg, hhh, iii) fw_gluUnProject(aaa, bbb, ccc, ddd, eee, fff, ggg, hhh, iii)
	#define FW_GLU_PROJECT(aaa, bbb, ccc, ddd, eee, fff, ggg, hhh, iii) fw_gluProject(aaa, bbb, ccc, ddd, eee, fff, ggg, hhh, iii)
	#define FW_GLU_PICK_MATRIX(aaa, bbb, ccc, ddd, eee) fw_gluPickMatrix(aaa, bbb, ccc, ddd, eee)

	/* GLU replacement -these still need doing */
	#define FW_GLU_DELETETESS(aaa) gluDeleteTess(aaa)
	#define FW_GLU_NEW_TESS gluNewTess
	#define FW_GLU_END_POLYGON(aaa) gluEndPolygon(aaa)
	#define FW_GLU_BEGIN_POLYGON(aaa) gluBeginPolygon(aaa)
	#define FW_GLU_TESS_VERTEX(aaa, bbb, ccc) gluTessVertex(aaa, bbb, ccc)
	#define FW_GLU_TESS_CALLBACK(aaa, bbb, ccc) gluTessCallback(aaa,bbb,ccc);
	#define FW_GLU_NEXT_CONTOUR(aaa, bbb) gluNextContour(aaa,bbb)


	#define FW_GL_GETDOUBLEV(aaa,bbb) fw_glGetDoublev(aaa,bbb);
	#define FW_GL_SETDOUBLEV(aaa,bbb) fw_glSetDoublev(aaa,bbb);
	#define FW_GL_LOAD_IDENTITY fw_glLoadIdentity
	#define FW_GL_POP_MATRIX() fw_glPopMatrix()
	#define FW_GL_PUSH_MATRIX() fw_glPushMatrix()

	#define FW_GL_TRANSLATE_F(xxx,yyy,zzz) fw_glTranslatef(xxx,yyy,zzz)
	#define FW_GL_TRANSLATE_D(xxx,yyy,zzz) fw_glTranslated(xxx,yyy,zzz)
	#define FW_GL_ROTATE_F(aaa,xxx,yyy,zzz) fw_glRotatef(aaa,xxx,yyy,zzz)
	#define FW_GL_ROTATE_D(aaa,xxx,yyy,zzz) fw_glRotated(aaa,xxx,yyy,zzz)
	#define FW_GL_ROTATE_RADIANS(aaa,xxx,yyy,zzz) fw_glRotateRad(aaa,xxx,yyy,zzz)
	#define FW_GL_SCALE_F(xxx,yyy,zzz) fw_glScalef(xxx,yyy,zzz)
	#define FW_GL_SCALE_D(xxx,yyy,zzz) fw_glScaled(xxx,yyy,zzz)
        #define FW_GL_PUSH_ATTRIB(aaa) glPushAttrib(aaa); 
	#define FW_GL_POP_ATTRIB() glPopAttrib();
	#define FW_GL_MATRIX_MODE(aaa) fw_glMatrixMode(aaa)
	#define FW_GL_ORTHO(aaa,bbb,ccc,ddd,eee,fff) fw_Ortho(aaa,bbb,ccc,ddd,eee,fff);


	/* geometry rendering - varies on whether we are using appearance shaders, etc */
	#define FW_VERTEX_POINTER_TYPE 44354
	#define FW_NORMAL_POINTER_TYPE 5434
	#define FW_FOG_POINTER_TYPE 33888 //?? how geenerate these numbers
	#define FW_COLOR_POINTER_TYPE 12453
	#define FW_TEXCOORD_POINTER_TYPE 67655
	//void sendAttribToGPU(int myType, int dataSize, int dataType, int normalized, int stride, float *pointer, int texID, char *file, int line);
	//                           datasize, dataType, stride, pointer
	#define FW_GL_VERTEX_POINTER(aaa, bbb, ccc, ddd) {sendAttribToGPU(FW_VERTEX_POINTER_TYPE, aaa, bbb, GL_FALSE, ccc, ddd,0,__FILE__,__LINE__); }
	#define FW_GL_COLOR_POINTER(aaa, bbb, ccc, ddd) {sendAttribToGPU(FW_COLOR_POINTER_TYPE, aaa, bbb, GL_FALSE, ccc, ddd,0,__FILE__,__LINE__); }
	#define FW_GL_NORMAL_POINTER(aaa, bbb, ccc) {sendAttribToGPU(FW_NORMAL_POINTER_TYPE, 0, aaa, GL_FALSE, bbb, ccc,0,__FILE__,__LINE__); }
	#define FW_GL_FOG_POINTER(aaa, bbb, ccc) {sendAttribToGPU(FW_FOG_POINTER_TYPE, 0, aaa, GL_FALSE, bbb, ccc,0,__FILE__,__LINE__); }
	#define FW_GL_TEXCOORD_POINTER(aaa, bbb, ccc, ddd, eee) {sendAttribToGPU(FW_TEXCOORD_POINTER_TYPE, aaa, bbb, GL_FALSE, ccc, ddd,eee,__FILE__,__LINE__); }
	#define FW_GL_BINDBUFFER(xxx,yyy) {sendBindBufferToGPU(xxx,yyy,__FILE__,__LINE__); }



	#define FW_GL_VIEWPORT(aaa,bbb,ccc,ddd) glViewport(aaa,bbb,ccc,ddd);
	#define FW_GL_CLEAR_COLOR(aaa,bbb,ccc,ddd) glClearColor(aaa,bbb,ccc,ddd);
	#define FW_GL_DEPTHMASK(aaa) glDepthMask(aaa);
        #define FW_GL_SCISSOR(aaa,bbb,ccc,ddd) glScissor(aaa,bbb,ccc,ddd); 
	#define FW_GL_WINDOWPOS2I(aaa,bbb) glWindowPos2i(aaa,bbb);
	#define FW_GL_FLUSH glFlush
	#define FW_GL_RASTERPOS2I(aaa,bbb) glRasterPos2i(aaa,bbb); 
        #define FW_GL_LIGHTMODELI(aaa,bbb) glLightModeli(aaa,bbb); 
        #define FW_GL_LIGHTMODELFV(aaa,bbb) glLightModelfv(aaa,bbb); 
	#define FW_GL_BLENDFUNC(aaa,bbb) glBlendFunc(aaa,bbb);
	#define FW_GL_LIGHTFV(aaa,bbb,ccc) fwglLightfv(aaa,bbb,ccc);
	#define FW_GL_LIGHTF(aaa,bbb,ccc) fwglLightf(aaa,bbb,ccc);
	#define FW_GL_CLEAR(zzz) glClear(zzz); 
	#define FW_GL_DEPTHFUNC(zzz) glDepthFunc(zzz); 
	#define FW_GL_SHADEMODEL(aaa) glShadeModel(aaa);  
	#define FW_GL_PIXELSTOREI(aaa,bbb) glPixelStorei(aaa,bbb);

#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
	#define FW_GL_FOGFV(aaa, bbb) glFogfv(aaa, bbb)
	#define FW_GL_FOGF(aaa, bbb) glFogf(aaa, bbb)
	#define FW_GL_FOGI(aaa, bbb) glFogi(aaa, bbb)
	#define FW_GL_BEGIN_QUERY(aaa, bbb) glBeginQuery(aaa, bbb)
	#define FW_GL_END_QUERY(aaa) glEndQuery(aaa)
	#define FW_GL_LINE_STIPPLE(aaa, bbb) glLineStipple(aaa, bbb)
	#define FW_GL_VERTEX3D(aaa, bbb, ccc) glVertex3d(aaa, bbb, ccc)


	//#define SET_TEXTURE_UNIT(aaa) { glActiveTexture(GL_TEXTURE0+aaa); glClientActiveTexture(GL_TEXTURE0+aaa); }
	
	#define FW_GL_GETSTRING(aaa) glGetString(aaa)
	#define FW_GL_DELETETEXTURES(aaa,bbb) glDeleteTextures(aaa,bbb);
	#define FW_GL_GETINTEGERV(aaa,bbb) glGetIntegerv(aaa,bbb);
	#define FW_GL_GETFLOATV(aaa,bbb) glGetFloatv(aaa,bbb);


	#define FW_GL_FRONTFACE(aaa) glFrontFace(aaa);
	#define FW_GL_GENLISTS(aaa) glGenLists(aaa)
	#define FW_GL_GENTEXTURES(aaa,bbb) glGenTextures(aaa,bbb)
	#define FW_GL_GETBOOLEANV(aaa,bbb) glGetBooleanv(aaa,bbb)
	#define FW_GL_NEWLIST(aaa,bbb) glNewList(aaa,bbb)
	#define FW_GL_NORMAL3F(aaa,bbb,ccc) glNormal3f(aaa,bbb,ccc)

	#define FW_GL_READPIXELS(aaa,bbb,ccc,ddd,eee,fff,ggg) glReadPixels(aaa,bbb,ccc,ddd,eee,fff,ggg) 
	#define FW_GL_TEXIMAGE2D(aaa,bbb,ccc,ddd,eee,fff,ggg,hhh,iii) glTexImage2D(aaa,bbb,ccc,ddd,eee,fff,ggg,hhh,iii)
	#define FW_GL_TEXPARAMETERF(aaa,bbb,ccc) glTexParameterf(aaa,bbb,ccc)
	#define FW_GL_TEXPARAMETERI(aaa,bbb,ccc) glTexParameteri(aaa,bbb,ccc)
	#define FW_GL_TEXPARAMETERFV(aaa,bbb,ccc) glTexParameterfv(aaa,bbb,ccc)
        #define FW_GL_GETQUERYOBJECTIV(aaa,bbb,ccc) glGetQueryObjectiv(aaa,bbb,ccc)
		
	/*apr 6 2010 checkout win32 was missing the following macros */
	#define FW_GL_DRAWBUFFER(aaa) glDrawBuffer(aaa)
	#define FW_GL_ENDLIST() glEndList()
	#define FW_GL_BITMAP(aaa,bbb,ccc,ddd,eee,fff,ggg) glBitmap(aaa,bbb,ccc,ddd,eee,fff,ggg)
	#define FW_GL_CALLLISTS(aaa,bbb,ccc) glCallLists(aaa,bbb,ccc)
	#define FW_GL_LISTBASE(aaa) glListBase(aaa)
	#define FW_GL_DRAWPIXELS(aaa,bbb,ccc,ddd,eee) glDrawPixels(aaa,bbb,ccc,ddd,eee)
	
#endif /* __LIBFREEWRL_DISPLAY_H__ */
