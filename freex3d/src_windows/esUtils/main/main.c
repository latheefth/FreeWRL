//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//
#include "esUtil.h"
#include <config.h>
#include <system.h>
#include <libFreeWRL.h>
#include <stdlib.h>
#include <stdio.h>

void *gglobal();
void frontend_dequeue_get_enqueue(void *tg);
static void *globalcontext;
static int more = 1;

void fwDraw ( ESContext *esContext )
{
	if(more){
		//borrowed from desktop.c loop
		frontend_dequeue_get_enqueue(globalcontext); //this is non-blocking (returns immediately) if queue empty
		more = fwl_draw();

		//Aug 2015 if eglSwapbuffers is a null function, you'll see black
		//it can be caused by using AMD libEGL/libGLES2 emulators on an Intel desktop machine, or something like that
		//if so use another libEGL/libGLES2 emulator kit from someone/somewhere else
		//in windows, we use angleproject, which when built gives us fwEGL.lib/dll and fwGLES2.lib/.dll
		eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );
	}else{
		esQuit(esContext);
	}
}
void fwOnKey( ESContext* esContext, unsigned char c, int updown, int ishift)
{
	//after 'q' to exit, the keyup for q will come in after gglobal already freed
	if(more)
		fwl_do_keyPress(c, updown); 
}
void statusbar_set_window_size(int width, int height);
int statusbar_handle_mouse(int mev, int butnum, int mouseX, int mouseY);
int fwl_handle_mouse(int mev, int butnum, int mouseX, int mouseY, int windex);
void fwl_setScreenDim(int wi, int he);
void fwOnMouse( ESContext* esContext, int mev, int button, int ix, int iy)
{
	if(more){
		fwl_handle_mouse(mev,button,ix,iy,0); 
	}
}

void fwOnResize( ESContext* esContext, int screenWidth, int screenHeight)
{

//#ifdef STATUSBAR_HUD
//	statusbar_set_window_size(screenWidth,screenHeight);
//#else
//	fwl_setScreenDim(screenWidth,screenHeight);
//#endif
	fwl_setScreenDim(screenWidth,screenHeight);

}
char *getWindowTitle();
void setWindowTitleFE()
{
	char * title;
	title = getWindowTitle();
	//SetWindowText(ghWnd,getWindowTitle()); //window_title);

}
static char* sceneUrl = NULL;
int fwInit ( ESContext *esContext )
{
	freewrl_params_t *fv_params = NULL;
    fv_params = calloc(1, sizeof(freewrl_params_t));

    /* Default values */
    fv_params->width = esContext->width; //600;
    fv_params->height = esContext->height; //400;
    //fv_params->eai = FALSE;
    fv_params->fullscreen = FALSE;
    fv_params->winToEmbedInto = INT_ID_UNDEFINED;
    fv_params->verbose = FALSE;
	fv_params->frontend_handles_display_thread = TRUE;
	if(!fwl_initFreeWRL(fv_params)) return FALSE;

	//borrowed from desktop.c > fwl_startFreeWRL
	// - we don't want to start its _displayThread here, instead we'll call the guts 
	//   of displaythread elsewhere.
	globalcontext = gglobal();
	if(sceneUrl)
		fwl_resource_push_single_request(sceneUrl); 
	fwl_setCurrentHandle(globalcontext, __FILE__, __LINE__);


	return 1;
}
ESContext *staticContext;
char *frontend_pick_file(void)
{
	return esUtil_pick_file(staticContext);
}
char *frontend_pick_URL(void)
{
	return esUtil_pick_URL(staticContext);
}
void fwl_setOrientation(int);
void setDisplayed(int);
int main ( int argc, char *argv[] )
{
   GLboolean retval;
   ESContext esContext;
   //UserData  userData;
   void *userData;


   if(argc > 1)
		sceneUrl = argv[1];
	else{
		sceneUrl = NULL;
		//sceneUrl = "http://dug9.users.sourceforge.net/web3d/tests/1.x3d";

	}
   esInitContext ( &esContext );
   staticContext = &esContext;
   userData = fwl_init_instance(); //before setting any structs we need a struct allocated
   esContext.userData = userData;//&userData;

   //Blackberry PlayBook 1024x600 (7") 170 PPI
   //equivalent 7" on Acer x203w desktop monitor: 600x352
   retval = esCreateWindow ( &esContext, "winGLES2", 600, 352, ES_WINDOW_RGB | ES_WINDOW_DEPTH | ES_WINDOW_STENCIL );
   //retval = esCreateWindow ( &esContext, "winGLES2", 1024, 600, ES_WINDOW_RGB | ES_WINDOW_DEPTH | ES_WINDOW_STENCIL );
   fwl_setOrientation (0); //int: 0, 90, 180, 270
   setDisplayed(1); //0=not 1=displayed
   if( retval == GL_FALSE ) printf("ouch - esCreateWindow returns false\n");
   //printf("%s\n",glGetString(GL_VERSION));
   //printf("%s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
   
   if ( !fwInit ( &esContext ) )
      return 0;
 	fwOnResize( &esContext, 600, 352);

   //esRegisterUpdateFunc ( &esContext, fwUpdate );
   esRegisterDrawFunc ( &esContext, fwDraw );
   esRegisterKeyFunc ( &esContext, fwOnKey );
   esRegisterMouseFunc ( &esContext, fwOnMouse );
   esRegisterResizeFunc( &esContext, fwOnResize );
   
   esMainLoop ( &esContext );

}
