//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

// ESUtil.c
//
//    A utility library for OpenGL ES.  This library provides a
//    basic common framework for the example applications in the
//    OpenGL ES 2.0 Programming Guide.
//

///
//  Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include "esUtil.h"
#include "esUtil_win.h"

/* Errors / GetError return values */
struct error_string
{
	char *str;
	EGLint error;
};
static struct error_string error_strings[] = {
	{"EGL_SUCCESS",	0x3000},
	{"EGL_NOT_INITIALIZED",		0x3001},
	{"EGL_BAD_ACCESS",			0x3002},
	{"EGL_BAD_ALLOC",			0x3003},
	{"EGL_BAD_ATTRIBUTE",		0x3004},
	{"EGL_BAD_CONFIG",			0x3005},
	{"EGL_BAD_CONTEXT",			0x3006},
	{"EGL_BAD_CURRENT_SURFACE",		0x3007},
	{"EGL_BAD_DISPLAY",			0x3008},
	{"EGL_BAD_MATCH",			0x3009},
	{"EGL_BAD_NATIVE_PIXMAP",		0x300A},
	{"EGL_BAD_NATIVE_WINDOW",		0x300B},
	{"EGL_BAD_PARAMETER",		0x300C},
	{"EGL_BAD_SURFACE",			0x300D},
	{"EGL_CONTEXT_LOST",		0x300E},	/* EGL 1.1 - IMG_power_management */
	{"SUCCESS", 0x0000}
};

char* ESUTIL_API esErrorString ( EGLint error )
{
	//char * str;
	int i;
	for(i=0;;i++){
		if(error_strings[i].error == error) return error_strings[i].str;
		if(error_strings[i].error == 0x0000) break;
	}
	return 0;
}

///
// CreateEGLContext()
//
//    Creates an EGL rendering context and all associated elements
//
EGLBoolean CreateEGLContext ( EGLNativeWindowType hWnd, EGLDisplay* eglDisplay,
                              EGLContext* eglContext, EGLSurface* eglSurface,
                              EGLint attribList[])
{
   EGLint numConfigs;
   EGLint majorVersion;
   EGLint minorVersion;
   EGLDisplay display;
   EGLContext context;
   EGLSurface surface;
   EGLConfig config;
   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };

   // Get Display
   
   display = eglGetDisplay(GetDC(hWnd));
   if ( display == EGL_NO_DISPLAY )
      display = eglGetDisplay(EGL_DEFAULT_DISPLAY); //win32 goes in here
   if ( display == EGL_NO_DISPLAY )
   {
	   printf("Ouch - EGL_NO_DISPLAY\n");
      return EGL_FALSE;
   }
   // Initialize EGL
   if ( !eglInitialize(display, &majorVersion, &minorVersion) )
   {
	   printf("Ouch no eglInitialize\n");
      return EGL_FALSE;
   }
   // Get configs
   if ( !eglGetConfigs(display, NULL, 0, &numConfigs) )
   {
      return EGL_FALSE;
	  printf("Ouch no eglGetConfigs\n");
   }

   // Choose config
   if ( !eglChooseConfig(display, attribList, &config, 1, &numConfigs) )
   {
      return EGL_FALSE;
   }

   // Create a surface
   surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)hWnd, NULL);
   if ( surface == EGL_NO_SURFACE )
   {
      return EGL_FALSE;
   }

   // Create a GL context
   context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );
   if ( context == EGL_NO_CONTEXT )
   {
      return EGL_FALSE;
   }   
   
   // Make the context current
   if ( !eglMakeCurrent(display, surface, surface, context) )
   {
      return EGL_FALSE;
   }
   
   *eglDisplay = display;
   *eglSurface = surface;
   *eglContext = context;
   return EGL_TRUE;
} 

//////////////////////////////////////////////////////////////////
//
//  Public Functions
//
//

///
//  esInitContext()
//
//      Initialize ES utility context.  This must be called before calling any other
//      functions.
//
void ESUTIL_API esInitContext ( ESContext *esContext )
{
   if ( esContext != NULL )
   {
      memset( esContext, 0, sizeof( ESContext) );
   }
}

///
//  esCreateWindow()
//
//      title - name for title bar of window
//      width - width of window to create
//      height - height of window to create
//      flags  - bitwise or of window creation flags 
//          ES_WINDOW_ALPHA       - specifies that the framebuffer should have alpha
//          ES_WINDOW_DEPTH       - specifies that a depth buffer should be created
//          ES_WINDOW_STENCIL     - specifies that a stencil buffer should be created
//          ES_WINDOW_MULTISAMPLE - specifies that a multi-sample buffer should be created
//
GLboolean ESUTIL_API esCreateWindow ( ESContext *esContext, const char* title, GLint width, GLint height, GLuint flags )
{
   //EGLint attribList[] =
   //{
   //    EGL_RED_SIZE,       5,
   //    EGL_GREEN_SIZE,     6,
   //    EGL_BLUE_SIZE,      5,
   //    EGL_ALPHA_SIZE,     (flags & ES_WINDOW_ALPHA) ? 8 : EGL_DONT_CARE,
   //    EGL_DEPTH_SIZE,     (flags & ES_WINDOW_DEPTH) ? 8 : EGL_DONT_CARE,
   //    EGL_STENCIL_SIZE,   (flags & ES_WINDOW_STENCIL) ? 8 : EGL_DONT_CARE,
   //    EGL_SAMPLE_BUFFERS, (flags & ES_WINDOW_MULTISAMPLE) ? 1 : 0,
   //    EGL_NONE
   //};

	//modified
   EGLint attribList[] =
   {
       EGL_RED_SIZE,       8,
       EGL_GREEN_SIZE,     8,
       EGL_BLUE_SIZE,      8,
       EGL_ALPHA_SIZE,     (flags & ES_WINDOW_ALPHA) ? 8 : EGL_DONT_CARE,
       EGL_DEPTH_SIZE,     (flags & ES_WINDOW_DEPTH) ? 24 : EGL_DONT_CARE,
       EGL_STENCIL_SIZE,   (flags & ES_WINDOW_STENCIL) ? 8 : EGL_DONT_CARE,
       EGL_SAMPLE_BUFFERS, (flags & ES_WINDOW_MULTISAMPLE) ? 1 : 0,
       EGL_NONE
   };
   
   if ( esContext == NULL )
   {
      return GL_FALSE;
   }

   esContext->width = width;
   esContext->height = height;

   if ( !WinCreate ( esContext, title) )
   {
      return GL_FALSE;
   }

   if ( !CreateEGLContext ( esContext->hWnd,
                            &esContext->eglDisplay,
                            &esContext->eglContext,
                            &esContext->eglSurface,
                            attribList) )
   {
	  printf("Ouch CreateEGLContext returns FALSE\n");
      return GL_FALSE;
   }
   
   return GL_TRUE;
}

///
//  esMainLoop()
//
//    Start the main loop for the OpenGL ES application
//
void ESUTIL_API esMainLoop ( ESContext *esContext )
{
   WinLoop ( esContext );
}


///
//  esRegisterDrawFunc()
//
void ESUTIL_API esRegisterDrawFunc ( ESContext *esContext, void (ESCALLBACK *drawFunc) (ESContext* ) )
{
   esContext->drawFunc = drawFunc;
}


///
//  esRegisterUpdateFunc()
//
void ESUTIL_API esRegisterUpdateFunc ( ESContext *esContext, void (ESCALLBACK *updateFunc) ( ESContext*, float ) )
{
   esContext->updateFunc = updateFunc;
}


///
//  esRegisterKeyFunc()
//
void ESUTIL_API esRegisterKeyFunc ( ESContext *esContext,
                                    void (ESCALLBACK *keyFunc) (ESContext*, unsigned char, int, int ) )
{
   esContext->keyFunc = keyFunc;
}
void ESUTIL_API esRegisterMouseFunc ( ESContext *esContext,
                                    void (ESCALLBACK *mouseFunc) (ESContext*, int, int, int, int ) )
{
   esContext->mouseFunc = mouseFunc;
}
void ESUTIL_API esRegisterResizeFunc ( ESContext *esContext,
                                    void (ESCALLBACK *resizeFunc) (ESContext*, int, int ) )
{
   esContext->resizeFunc = resizeFunc;
}


///
// esLogMessage()
//
//    Log an error message to the debug output for the platform
//
void ESUTIL_API esLogMessage ( const char *formatStr, ... )
{
    va_list params;
    char buf[BUFSIZ];

    va_start ( params, formatStr );
    vsprintf_s ( buf, sizeof(buf),  formatStr, params );
    
    printf ( "%s", buf );
    
    va_end ( params );
}

char* ESUTIL_API esUtil_pick_file(ESContext *esContext)
{
	return esPickFile(esContext->hWnd);
}
char * ESUTIL_API esUtil_pick_URL(ESContext *esContext)
{
	return esPickURL(esContext->hWnd);
}


///
// esLoadTGA()
//
//    Loads a 24-bit TGA image from a file
//
//char* ESUTIL_API esLoadTGA ( char *fileName, int *width, int *height )
//{
//   char *buffer;
//
//   if ( WinTGALoad ( fileName, &buffer, width, height ) )
//   {
//      return buffer;
//   }
//
//   return NULL;
//}