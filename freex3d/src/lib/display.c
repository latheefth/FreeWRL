/*

  FreeWRL support library.
  Display (X11/Motif or OSX/Aqua) initialization.

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
#include <system_threads.h>
#include <internal.h>
#include <display.h>
#include <threads.h>
#include <libFreeWRL.h>

#include "vrml_parser/Structs.h"
#include "opengl/Textures.h"
#include "opengl/RasterFont.h"
#include "opengl/OpenGL_Utils.h"
//JAS #include "scenegraph/Collision.h"

#include "ui/common.h"

#if defined(FREEWRL_PLUGIN) && (defined(TARGET_X11) || defined(TARGET_MOTIF))
#include "plugin/pluginUtils.h"
#endif


#if defined (TARGET_AQUA)
/* display part specific to Mac */

#ifndef IPHONE

/* for handling Safari window changes at the top of the display event loop */
int PaneClipnpx;
int PaneClipnpy;

int PaneClipct;
int PaneClipcb;
int PaneClipcr;
int PaneClipcl;
int PaneClipwidth;
int PaneClipheight;
int PaneClipChanged = FALSE;
#endif
#endif

//static Stack *_vpstack = NULL; //ivec4 in y-down pixel coords - viewport stack used for clipping drawing

typedef struct ivec4 {int X; int Y; int W; int H;} ivec4;
typedef struct ivec2 {int X; int Y;} ivec2;
ivec4 ivec4_init(int x, int y, int w, int h){
	ivec4 ret;
	ret.X = x, ret.Y = y;  ret.W = w; ret.H = h;
	return ret;
}

ivec2 ivec2_init(int x, int y){
	ivec2 ret;
	ret.X = x, ret.Y = y; 
	return ret;
}

#define MAXSTAT 200
typedef struct pdisplay{
	freewrl_params_t params;
	s_renderer_capabilities_t rdr_caps;
	char myMenuStatus[MAXSTAT];
}* ppdisplay;
void *display_constructor(){
	void *v = MALLOCV(sizeof(struct pdisplay));
	memset(v,0,sizeof(struct pdisplay));
	return v;
}
void display_init(struct tdisplay* t) 
{
	//public
	//freewrl_params_t p = d->params;

	t->display_initialized = FALSE;

	t->screenWidth = 0; /* screen */
	t->screenHeight = 0;
	t->window_title = NULL;

	t->shutterGlasses = 0; /* stereo shutter glasses */
	t->prv = display_constructor();
	{
		ppdisplay p = (ppdisplay)t->prv;
		memset(&p->rdr_caps,0,sizeof(s_renderer_capabilities_t));
		t->rdr_caps = &p->rdr_caps;
		p->params.height = 0; /* window */
		p->params.width = 0;
		p->params.winToEmbedInto = INT_ID_UNDEFINED;
		p->params.fullscreen = FALSE;
		p->params.xpos = 0;
		p->params.ypos = 0;
		p->params.frontend_handles_display_thread = FALSE;
		t->params = &p->params;
	}
}



#if KEEP_FV_INLIB

#if defined (_ANDROID)

/* simple display initialize for Android (and, probably, iPhones, too) */

int fv_display_initialize()
{
    struct tdisplay* d = &gglobal()->display;
#ifdef HAVE_OPENCL
	struct tOpenCL_Utils *cl = &gglobal()->OpenCL_Utils;
#endif //HAVE_OPENCL

    
    //printf ("fv_display_initialize called\n");
    if (d->display_initialized) {
        //ConsoleMessage ("fv_display_initialized re-called for a second time");
        return TRUE;
    }
    
    if (!fwl_initialize_GL()) {
        return FALSE;
    }
    
    /* Display full initialized :P cool ! */
    d->display_initialized = TRUE;
   
#ifdef HAVE_OPENCL

    if (!cl->OpenCL_Initialized) {
	printf ("doing fwl_OpenCL_startup here in fv_display_inintialize\n");
	fwl_OpenCL_startup(cl);
    }

#endif

    PRINT_GL_ERROR_IF_ANY ("end of fv_display_initialize");
    
    return TRUE;
}
#else
/**
 *  fv_display_initialize: takes care of all the initialization process, 
 *                      creates the display thread and wait for it to complete
 *                      the OpenGL initialization and the Window creation.
 */
int fv_display_initialize()
{
	struct tdisplay* d;
	ppdisplay p;
	ttglobal tg = gglobal();
	d = &tg->display;
	p = (ppdisplay)tg->display.prv;

#ifdef HAVE_OPENCL
	struct tOpenCL_Utils *cl = &gglobal()->OpenCL_Utils;
#endif //HAVE_OPENCL

	if (d->display_initialized) return TRUE;

	//memset(&d->rdr_caps, 0, sizeof(d->rdr_caps));

	/* FreeWRL parameters */
	//d->fullscreen = fwl_getp_fullscreen();
	//d->width = fwl_getp_width();
	//d->height = fwl_getp_height();
	//d->winToEmbedInto = fwl_getp_winToEmbedInto();

 	/* make the window, get the OpenGL context */
// #if !defined(_MSC_VER) && !defined(_ANDROID) && !defined(QNX) && !defined(IPHONE)
#if defined (__linux__)

	if (!fv_open_display()) {
		printf("open_display failed\n");
		return FALSE;
	}

	if (!fv_create_GLcontext()) {
		printf("create_GLcontext failed\n");
		return FALSE;
	}

 #endif //!MSC_VER && ! any OpenGL ES 2.0 device

	if (0 != d->screenWidth)  p->params.width  = d->screenWidth;
	if (0 != d->screenHeight) p->params.height = d->screenHeight;
	fv_setScreenDim(p->params.width,p->params.height); /* recompute screenRatio */

	//snprintf(window_title, sizeof(window_title), "FreeWRL");

	if (!fv_create_main_window(d->params)){ //0 /*argc*/, NULL /*argv*/)) {
	//if (!fv_create_main_window((freewrl_params_t *)d)){ //0 /*argc*/, NULL /*argv*/)) {
		return FALSE;
	}

	setWindowTitle0();

#if ! ( defined(_MSC_VER) || defined(FRONTEND_HANDLES_DISPLAY_THREAD) )
	
	fv_bind_GLcontext();
#endif


	if (!fwl_initialize_GL()) {
		printf("initialize_GL failed\n");
		return FALSE;
	}

        /* lets make sure everything is sync'd up */
#if defined(TARGET_X11) || defined(TARGET_MOTIF)
        XFlush(Xdpy);
#endif

        /* initialize default font 
           
           TODO: this may be a configuration option (config file or command line)
         */
        //rf_xfont_init("fixed");

	/* Display full initialized :P cool ! */
	d->display_initialized = TRUE;
	gglobal()->display.display_initialized = TRUE;

	DEBUG_MSG("FreeWRL: running as a plugin: %s\n", BOOL_STR(isBrowserPlugin));

    PRINT_GL_ERROR_IF_ANY ("end of fv_display_initialize");
    
#if !(defined(TARGET_AQUA) || defined(_MSC_VER) || defined(_ANDROID))
        
	if (RUNNINGASPLUGIN) {
#if defined(FREEWRL_PLUGIN) && (defined(TARGET_X11) || defined(TARGET_MOTIF))
		sendXwinToPlugin();
#endif
	} else {
		XMapWindow(Xdpy, Xwin);
	}
#endif /* IPHONE */

#ifdef HAVE_OPENCL

    if (!cl->OpenCL_Initialized) {
	printf ("doing fwl_OpenCL_startup here in fv_display_inintialize\n");
	fwl_OpenCL_startup(cl);
    }

#endif
	return TRUE;
}
#endif //ANDROID


/**
 *   fv_setGeometry_from_cmdline: scan command line arguments (X11 convention), to
 *                             set up the window dimensions.
 */
int fwl_parse_geometry_string(const char *geometry, int *out_width, int *out_height, 
			      int *out_xpos, int *out_ypos)
{
	int width, height, xpos, ypos;
	int c;

	width = height = xpos = ypos = 0;

	c = sscanf(geometry, "%dx%d+%d+%d", 
		   &width, &height, &xpos, &ypos);

	if (out_width) *out_width = width;
	if (out_height) *out_height = height;
	if (out_xpos) *out_xpos = xpos;
	if (out_ypos) *out_ypos = ypos;

	if (c > 0)
		return TRUE;
	return FALSE;
}

void fv_setScreenDim(int wi, int he) { fwl_setScreenDim(wi,he); }
#endif /* KEEP_FV_INLIB */

/**
 *   fwl_setScreenDim: set internal variables for screen sizes, and calculate frustum
 */
void fwl_setScreenDim(int wi, int he)
{
	ttglobal tg = gglobal();

    tg->display.screenWidth = wi;  //width of the whole opengl surface in pixels
    tg->display.screenHeight = he; //height of the whole opengl surface in pixels
    /* printf("%s,%d fwl_setScreenDim(int %d, int %d)\n",__FILE__,__LINE__,wi,he); */

}
double display_screenRatio(){
	ttglobal tg = gglobal();
	double ratio = 1.5;
	if (tg->display.screenHeight != 0) ratio = (double) tg->display.screenWidth/(double) tg->display.screenHeight;
	return ratio;
}
void fwl_setClipPlane(int height)
{
	//this should be 2 numbers, one for top and bottom
	//right now the statusbarHud -which shares the opengl window with the scene- takes 16 pixels on the bottom
	gglobal()->Mainloop.clipPlane = height; 
}
/**
 *   resize_GL: when the window is resized we have to update the GL viewport.
 */
GLvoid resize_GL(GLsizei width, GLsizei height)
{ 
    FW_GL_VIEWPORT( 0, 0, width, height ); 
	printf("resize_GL\n");
}

void fwl_updateScreenDim(int wi, int he)
{
	fwl_setScreenDim(wi, he);

	resize_GL(wi, he);
}



/**
 * On all platforms, when we don't have GLEW, we simulate it.
 * In any case we setup the rdr_capabilities struct.
 */
bool initialize_rdr_caps()
{
	//s_renderer_capabilities_t *rdr_caps;
	/* Max texture size */
	GLint tmp;  /* ensures that we pass pointers of same size across all platforms */
	ppdisplay p = (ppdisplay)gglobal()->display.prv;
	
#if defined(HAVE_GLEW_H) && !defined(ANGLEPROJECT)
	/* Initialize GLEW */
	{
	GLenum err;
	err = glewInit();
	if (GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		ERROR_MSG("GLEW initialization error: %s\n", glewGetErrorString(err));
		return FALSE;
	}
	TRACE_MSG("GLEW initialization: version %s\n", glewGetString(GLEW_VERSION));
	}
#endif

	/* OpenGL is initialized, context is created,
	   get some info, for later use ...*/
        p->rdr_caps.renderer   = (char *) FW_GL_GETSTRING(GL_RENDERER);
        p->rdr_caps.version    = (char *) FW_GL_GETSTRING(GL_VERSION);
        p->rdr_caps.vendor     = (char *) FW_GL_GETSTRING(GL_VENDOR);
	p->rdr_caps.extensions = (char *) FW_GL_GETSTRING(GL_EXTENSIONS);
    FW_GL_GETBOOLEANV(GL_STEREO,&(p->rdr_caps.quadBuffer));
    //if (rdr_caps.quadBuffer) ConsoleMessage("INIT HAVE QUADBUFFER"); else ConsoleMessage("INIT_ NO QUADBUFFER");
    ConsoleMessage("openGL version %s\n",p->rdr_caps.version);

	/* rdr_caps.version = "1.5.7"; //"1.4.1"; //for testing */
    p->rdr_caps.versionf = (float) atof(p->rdr_caps.version); 
    if (p->rdr_caps.versionf == 0) // can't parse output of GL_VERSION, generally in case it is smth. like "OpenGL ES 3.0 V@66.0 AU@ (CL@)". probably 3.x or bigger.
	{
        const char *openGLPrefix = "OpenGL ES ";
        if (NULL != p->rdr_caps.version && strstr(p->rdr_caps.version, openGLPrefix))
        {
            char version[256], *versionPTR;
            sprintf(version, "%s", p->rdr_caps.version);
            versionPTR = version + strlen(openGLPrefix);
            p->rdr_caps.versionf = (float) atof(versionPTR);
            //free(version);
        }
#if defined(GL_ES_VERSION_2_0) && !defined(ANGLEPROJECT)
        if (0 == rdr_caps.version)
        {
            //Try define version with 3.x api
            GLint major = 0, minor = 0;
            FW_GL_GETINTEGERV(GL_MAJOR_VERSION, &major);
            FW_GL_GETINTEGERV(GL_MINOR_VERSION, &minor);
            char *version;
            asprintf(&version, "%d.%d", major, minor);
            p->rdr_caps.version = version;
            p->rdr_caps.versionf = (float) atof(p->rdr_caps.version);
            free(version);
        }
#endif
	}
	/* atof technique: http://www.opengl.org/resources/faq/technical/extensions.htm */
    p->rdr_caps.have_GL_VERSION_1_1 = p->rdr_caps.versionf >= 1.1f;
    p->rdr_caps.have_GL_VERSION_1_2 = p->rdr_caps.versionf >= 1.2f;
    p->rdr_caps.have_GL_VERSION_1_3 = p->rdr_caps.versionf >= 1.3f;
    p->rdr_caps.have_GL_VERSION_1_4 = p->rdr_caps.versionf >= 1.4f;
    p->rdr_caps.have_GL_VERSION_1_5 = p->rdr_caps.versionf >= 1.5f;
    p->rdr_caps.have_GL_VERSION_2_0 = p->rdr_caps.versionf >= 2.0f;
    p->rdr_caps.have_GL_VERSION_2_1 = p->rdr_caps.versionf >= 2.1f;
    p->rdr_caps.have_GL_VERSION_3_0 = p->rdr_caps.versionf >= 3.0f;


	/* Initialize renderer capabilities without GLEW */

	/* Multitexturing */
	p->rdr_caps.av_multitexture = (strstr (p->rdr_caps.extensions, "GL_ARB_multitexture")!=0);

	/* Occlusion Queries */
	p->rdr_caps.av_occlusion_q = ((strstr (p->rdr_caps.extensions, "GL_ARB_occlusion_query") !=0) ||
                             (strstr(p->rdr_caps.extensions, "GL_EXT_occlusion_query_boolean") != 0) ||
                             p->rdr_caps.have_GL_VERSION_3_0);


	/* Non-power-of-two textures */
	p->rdr_caps.av_npot_texture = (strstr (p->rdr_caps.extensions, "GL_ARB_texture_non_power_of_two") !=0);

	/* Texture rectangle (x != y) */
	p->rdr_caps.av_texture_rect = (strstr (p->rdr_caps.extensions, "GL_ARB_texture_rectangle") !=0);

	/* if we are doing our own shading, force the powers of 2, because otherwise mipmaps are not possible. */
	p->rdr_caps.av_npot_texture=FALSE;

	/* attempting multi-texture */
	p->rdr_caps.av_multitexture = 1;

	FW_GL_GETINTEGERV(GL_MAX_TEXTURE_SIZE, &tmp);
	p->rdr_caps.runtime_max_texture_size = (int) tmp;
	p->rdr_caps.system_max_texture_size = (int) tmp;

	// GL_MAX_TEXTURE_UNITS is for fixed function, and should be deprecated.
	// use GL_MAX_TEXTURE_IMAGE_UNITS now, according to the OpenGL.org wiki


	#if defined (GL_MAX_TEXTURE_IMAGE_UNITS)
	FW_GL_GETINTEGERV(GL_MAX_TEXTURE_IMAGE_UNITS, &tmp);
	#else
	FW_GL_GETINTEGERV(GL_MAX_TEXTURE_UNITS, &tmp);
	#endif

	p->rdr_caps.texture_units = (int) tmp;

	/* max supported texturing anisotropicDegree- can be changed in TextureProperties */
#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
	FW_GL_GETFLOATV (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &p->rdr_caps.anisotropicDegree);
#endif
	/* User settings in environment */

	//ConsoleMessage ("Environment set texture size: %d", gglobal()->internalc.user_request_texture_size);
	if (gglobal()->internalc.user_request_texture_size > 0) {
		DEBUG_MSG("Environment set texture size: %d", gglobal()->internalc.user_request_texture_size);
		p->rdr_caps.runtime_max_texture_size = gglobal()->internalc.user_request_texture_size;
	}

	/* Special drivers settings */
	if (
	strstr(p->rdr_caps.renderer, "Intel GMA 9") != NULL ||
	strstr(p->rdr_caps.renderer, "Intel(R) 9") != NULL ||
	strstr(p->rdr_caps.renderer, "i915") != NULL ||
	strstr(p->rdr_caps.renderer, "NVIDIA GeForce2") != NULL
	) {
		if (p->rdr_caps.runtime_max_texture_size > 1024) p->rdr_caps.runtime_max_texture_size = 1024;
	}

	/* print some debug infos */
	rdr_caps_dump(&p->rdr_caps);

	//make this the renderer caps for this thread.
	//memcpy(&gglobal()->display.rdr_caps,&rdr_caps,sizeof(rdr_caps));
	return TRUE;
}

void initialize_rdr_functions()
{
	/**
	 * WARNING:
	 *
	 * Linux OpenGL driver (Mesa or ATI or NVIDIA) and Windows driver
	 * will not initialize function pointers. So we use GLEW or we
	 * initialize them ourself.
	 *
	 * OSX 10.4 : same as above.
	 * OSX 10.5 and OSX 10.6 : Apple driver will initialize functions.
	 */

	
}

void rdr_caps_dump(s_renderer_capabilities_t *rdr_caps)
{
#ifdef VERBOSE
	{
		char *p, *pp;
		p = pp = STRDUP(rdr_caps->extensions);
		while (*pp != '\0') {
			if (*pp == ' ') *pp = '\n';
			pp++;
		}
		DEBUG_MSG ("OpenGL extensions : %s\n", p);
		FREE(p);
	}
#endif //VERBOSE

	DEBUG_MSG ("Multitexture support: %s\n", BOOL_STR(rdr_caps->av_multitexture));
	DEBUG_MSG ("Occlusion support:    %s\n", BOOL_STR(rdr_caps->av_occlusion_q));
	DEBUG_MSG ("Max texture size      %d\n", rdr_caps->runtime_max_texture_size);
	DEBUG_MSG ("Max texture size      %d\n", rdr_caps->system_max_texture_size);
	DEBUG_MSG ("Texture units         %d\n", rdr_caps->texture_units);
}
