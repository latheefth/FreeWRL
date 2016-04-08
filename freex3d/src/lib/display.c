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
#include "opengl/RasterFont.h"
#include "opengl/OpenGL_Utils.h"
#include "opengl/Textures.h"
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
	freewrl_params_t params; //pre-allocated
	s_renderer_capabilities_t rdr_caps;
	char myMenuStatus[MAXSTAT];
	int multi_window_capable;
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
		/*
		p->params.height = 0; // window
		p->params.width = 0;
		p->params.winToEmbedInto = INT_ID_UNDEFINED;
		p->params.fullscreen = FALSE;
		p->params.xpos = 0;
		p->params.ypos = 0;
		p->params.frontend_handles_display_thread = FALSE;
		*/
#if defined(ANGLEPROJECT) || defined(_ANDROID) || defined(QNX) || defined(IPHONE)
		p->multi_window_capable = 0; //single-window EGL/ANGLEPROJECT(GLES2)/MOBILE
#else
		p->multi_window_capable = 1;  //desktop opengl __linux__, _MSC_VER, TARGET_AQUA
#endif
		t->params = (void*)&p->params;
	}
}



/* simple display initialize for Android (and, probably, iPhones, too)
	Nov 2015: now used for desktop backend opengl initialization
*/

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

//void fv_swapbuffers(freewrl_params_t *d);
#ifdef WINRT
void fv_swapbuffers(freewrl_params_t *d){
	return;
}
#endif

//void fv_change_GLcontext(freewrl_params_t* d);
// each config needs to populate:
// ANGLEPROJECT(stub) and WIN32(wglSetContext) done in src/lib/ui/fwWindow32.c
//void fv_change_GLcontext(freewrl_params_t* d){
//	return; //stub for ANLGEPROJECT, EGL/GLES2, mobile which don't change context but need to link
//}
#ifdef WINRT
void fv_change_GLcontext(freewrl_params_t* d){
	//stub for non-desktop configs (they can't do multiple windows anyway)
}
#elif _MSC_VER
//win32 in fwWindow32.c
#elif __linux__  //LINUX
//void fv_change_GLcontext(freewrl_params_t* d){
//	glXMakeCurrent(d->display,d->surface,d->context); 
//}
#elif AQUA
void fv_change_GLcontext(freewrl_params_t* d){
	aglSetCurrentContext(d->context);
}
#else
void fv_change_GLcontext(freewrl_params_t* d){
	//stub for non-desktop configs (they can't do multiple windows anyway)
}
#endif

#if !defined (_ANDROID) && !defined(WINRT)
int fv_create_window_and_context(freewrl_params_t *params, freewrl_params_t *share);
//#if defined (__linux__)
//int fv_create_window_and_context(freewrl_params_t *params, freewrl_params_t *share){
// 	/* make the window, create the OpenGL context, share the context if necessary 
//		Nov 2015: linux desktop is still single windowed, with static GLXContext etc, no sharing
//		- to get sharing, you need to populate params during creation of window and gl context
//			d->display = Display *Xdpy;
//			d->surface = Drawable or ???
//			d->context = GLXContext GLcx;
//			so when the targetwindow changes, there's enough info to do glXMakeCurrent and glXSwapBuffers
//			- and when doing glCreateContext you have the previous window's GLXcontext to use as a shareList
//	*/
//
//	if (!fv_open_display()) {
//		printf("open_display failed\n");
//		return FALSE;
//	}
//
//	if (!fv_create_GLcontext()) {
//		printf("create_GLcontext failed\n");
//		return FALSE;
//	}
//	fv_bind_GLcontext();
//	return TRUE;
//}
//#endif //__linux__

#ifdef _MSC_VER 
int fv_create_window_and_context(freewrl_params_t *params, freewrl_params_t *share){
	if (!fv_create_main_window2(params,share)){ //0 /*argc*/, NULL /*argv*/)) {
		return FALSE;
	}
	return TRUE;
}
#endif //_MSC_VER
#ifdef AQUA
int fv_create_window_and_context(freewrl_params_t *params, freewrl_params_t *share){
 	/* make the window, create the OpenGL context, share the context if necessary 
		Nov 2015: OSX desktop is still single windowed, with static AGLcontext etc, no sharing
		- to get sharing, you need to populate params during creation of window and gl context
			d->display = (don't need)
			d->surface = (don't need)
			d->context = AGLContext
			so when the targetwindow changes, there's enough info to do aglSetCurrentContext and aglSwapBuffers
			- and when doing aglCreateContext you have the previous window's AGLContext to use as a share

	*/

	if (!fv_create_main_window(params)){ //0 /*argc*/, NULL /*argv*/)) {
		return FALSE;
	}
	fv_bind_GLcontext();
	return TRUE;
}
#endif

void targetwindow_set_params(int itargetwindow, freewrl_params_t* params);
freewrl_params_t* targetwindow_get_params(int itargetwindow);
/**
 *  fv_display_initialize_desktop: 
 *		creates window
 *		creates opengl context, associates with window
 *		sets sharing if multi-window
 *      calls fv_display_initialize() for the backend generic OpenGL initialization 
 */
int fv_display_initialize_desktop(){
	int nwindows;
	struct tdisplay* d;
	freewrl_params_t *dp;
	ppdisplay p;
	ttglobal tg = gglobal();
	d = &tg->display;
	p = (ppdisplay)tg->display.prv;

	dp = (freewrl_params_t*)d->params;
	if(dp->frontend_handles_display_thread){
		//all configs are technically frontend handles display thread now, 
		// as seen by the backend (not including desktop.c which is a frontend)
		// the flag frontend_handles_display_thread here really means 
		// frontend_handles_window_creation_and_opengl_context_creation
		// for example: winGLES2.exe which uses an EGL kit for window/glcontext
		return fv_display_initialize(); //display_initialize now really means initialize generic backend opengl
	}

	nwindows = 1; //1 is normal freewrl, 2 or 3 is freaky 2,3 windowed freewrl for experiments, search targetwindow and windex
	if(!p->multi_window_capable) nwindows = 1;
 	/* make the window, get the OpenGL context */
	if(!fv_create_window_and_context(dp, NULL)){
		return FALSE;
	}
	d->display_initialized = fwl_initialize_GL();
	targetwindow_set_params(0,dp);
	if(nwindows > 1){
		//2nd fun window! to challenge us!
		freewrl_params_t *p0;
		dp->winToEmbedInto = -1;
		p0 = targetwindow_get_params(0);
		if(!fv_create_window_and_context(dp,p0)){
			return FALSE;
		}
		targetwindow_set_params(1,dp); 
		fwl_initialize_GL(); //has context-specific initializations -like GL_BLEND- so repeat per-context
	}
	if(nwindows > 2){
		freewrl_params_t *p1;
		dp->winToEmbedInto = -1;
		p1 = targetwindow_get_params(1);
		if(!fv_create_window_and_context(dp, p1)){
			return FALSE;
		}
		targetwindow_set_params(2,dp); 
		fwl_initialize_GL();
	}
	setWindowTitle0();

        /* lets make sure everything is sync'd up */
#if defined(TARGET_X11) || defined(TARGET_MOTIF)
        XFlush(Xdpy);
#endif

	gglobal()->display.display_initialized = d->display_initialized;

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
	return TRUE;
}
#endif //!_ANDORID


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

void fwl_setScreenDim1(int wi, int he, int windex);
void fwl_setScreenDim0(int wi, int he)
{
	//this one just sets the tg->display.screenWidth and is called in the targetwindow rendering loop
	//this allows legacy code use of display.screenWidth etc to work normally in the render functions
	ttglobal tg = gglobal();

    tg->display.screenWidth = wi;  //width of the whole opengl surface in pixels
    tg->display.screenHeight = he; //height of the whole opengl surface in pixels
}
/**
 *   fwl_setScreenDim: set internal variables for screen sizes, and calculate frustum
 */
void fwl_setScreenDim(int wi, int he)
{
	//this one is called from platform-specific window event handling code, and
	//by default assumes there's only one window, windex=0
	//and sets a windowtarget-specific viewport as well as tg.display.screenwidth/height 
	fwl_setScreenDim0(wi,he);
	fwl_setScreenDim1(wi,he,0);
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
	if (p->rdr_caps.version)
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
        if (0 == p->rdr_caps.version)
        {
            //Try define version with 3.x api
            GLint major = 0, minor = 0;
			#if defined(GL_MAJOR_VERSION) && defined(GL_MINOR_VERSION)
            FW_GL_GETINTEGERV(GL_MAJOR_VERSION, &major);
            FW_GL_GETINTEGERV(GL_MINOR_VERSION, &minor);
			#else
			major = 2;
			minor = 1;
			#endif
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
	if (p->rdr_caps.extensions){
		p->rdr_caps.av_multitexture = (strstr(p->rdr_caps.extensions, "GL_ARB_multitexture") != 0);

		/* Occlusion Queries */
		p->rdr_caps.av_occlusion_q = ((strstr(p->rdr_caps.extensions, "GL_ARB_occlusion_query") != 0) ||
			(strstr(p->rdr_caps.extensions, "GL_EXT_occlusion_query_boolean") != 0) ||
			p->rdr_caps.have_GL_VERSION_3_0);


		/* Non-power-of-two textures */
		p->rdr_caps.av_npot_texture = (strstr(p->rdr_caps.extensions, "GL_ARB_texture_non_power_of_two") != 0);

		/* Texture rectangle (x != y) */
		p->rdr_caps.av_texture_rect = (strstr(p->rdr_caps.extensions, "GL_ARB_texture_rectangle") != 0);
	}
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
	if (p->rdr_caps.renderer)
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
