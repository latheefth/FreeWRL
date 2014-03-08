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
void finalizeRenderSceneUpdateScene();
#include <stdlib.h>
#include <stdio.h>

	void updateConsoleStatus(); //poll Model & update UI(View)

	/* status bar, if we have one */
	void finishedWithGlobalShader();
	void drawStatusBar();  // UI/View 
	void restoreGlobalShader();


void fwDraw ( ESContext *esContext )
//void fwUpdate ( ESContext *esContext, float delta )
{
	fwl_RenderSceneUpdateScene();
	//updateButtonStatus(); //poll Model & update UI(View)
	updateConsoleStatus(); //poll Model & update UI(View)
	//checkFileLoadRequest();
	/* status bar, if we have one */
	finishedWithGlobalShader();
	drawStatusBar();  // UI/View 
	restoreGlobalShader();

	//drawStatusBarFE();
	eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );

}
void fwOnKey( ESContext* esContext, unsigned char c, int updown, int ishift)
{
	//int updown;
	//printf("c=%c updown=%d ishift=%d\n",c,updown,ishift);
	//updown=1;
	fwl_do_keyPress(c, updown); 

}
//int handleStatusbarHudFE(int mev, int mx, int my); //int* clipplane)
void fwOnMouse( ESContext* esContext, int mev, int button, int ix, int iy)
{
	//printf("mev=%d button=%d\n",mev,button);
	//if(!handleStatusbarHudFE(mev,ix,iy))
		fwl_handle_aqua(mev,button,ix,iy); 
}
//void statusbarOnResize(int screenWidth, int screenHeight);
void fwOnResize( ESContext* esContext, int screenWidth, int screenHeight)
{
	//statusbarOnResize(screenWidth,screenHeight);
	//resize_GL(rect.right, rect.bottom); 
	fwl_setScreenDim(screenWidth,screenHeight);
}
char *getWindowTitle();
void setWindowTitleFE()
{
	char * title;
	title = getWindowTitle();
	//SetWindowText(ghWnd,getWindowTitle()); //window_title);

}
//void initStaticStatusbar();
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
    //fv_params->collision = 1; // if you set it, you need to update ui button with a call
	//setMenuButton_collision(fv_params->collision);
	if(!fwl_initFreeWRL(fv_params)) return FALSE;
	//fwl_registerFunction("Replace","setWindowTitle",setWindowTitleFE);
	/* Hmm. display_initialize is really a frontend function. The frontend should call it before calling _displayThread */
	/* Initialize display */
	//initStaticStatusbar(); //call early for console_message

	if (!fv_display_initialize()) {
		printf("initFreeWRL: error in display initialization.\n");
		return(0);
	}
	//fwl_init_SideBySide(); //works but needs base adjustment

	//fwl_startFreeWRL("");
	//fwl_startFreeWRL("../../../../tests/4.wrl");
	//fwl_startFreeWRL("../../../../tests/6.wrl");
	//fwl_startFreeWRL("../../../../tests/11.wrl");
	//fwl_startFreeWRL("../../../../tests/15.wrl");
	//fwl_startFreeWRL("../../../../tests/16.wrl");
	//fwl_startFreeWRL("../../../../tests/20.wrl");
	//fwl_startFreeWRL("../../../../tests/51.wrl");
	//fwl_startFreeWRL("../../../../tests/ProgrammableShaders/models/flutter2-ProgramShader.x3d");
	//fwl_startFreeWRL("../../../../tests/ProgrammableShaders/models/teapot-Toon.wrl");
	//fwl_startFreeWRL("../../../../tests/ProgrammableShaders/models/TwoCylinders.wrl");
	//fwl_startFreeWRL("../../../../placeholders/townsite/townsite_withHud.x3d");
	//fwl_startFreeWRL("../../../../tests/Roelofs/43487/43487-galaxies.wrl");
	//fwl_startFreeWRL("../../../../tests/21.x3d");
	fwl_startFreeWRL("../../../../tests/1.x3d");
	//fwl_startFreeWRL("../../../../placeholders/townsite/townsite.x3d");
	//fwl_startFreeWRL("../../../../tests/shaders/Door248_PlutoMod.x3d");
	//fwl_startFreeWRL("../../../../tests/1_IFS.x3d");
	//fwl_startFreeWRL("../../../../tests/blender/cone_lamp4.x3d");
	//fwl_startFreeWRL("../../../../tests/blender/cone.x3d");
	//fwl_startFreeWRL("../../../../tests/Dave_exposedField/jtest.wrl");
	//fwl_startFreeWRL("../../../../tests/shaders/oneTexOneMaterial.x3d");
	//fwl_startFreeWRL("http://dug9.users.sourceforge.net/web3d/tests/1.x3d");
	//fwl_startFreeWRL("http://dug9.users.sourceforge.net/web3d/tests/2.x3d");
	//fwl_startFreeWRL("http://dug9.users.sourceforge.net/web3d/tests/49.x3d");
	//fwl_startFreeWRL("http://dug9.users.sourceforge.net/gravity.x3d");
	//fwl_startFreeWRL("http://dug9.users.sourceforge.net/web3d/townsite/townsite.x3d");
	//fwl_startFreeWRL("http://dug9.users.sourceforge.net/web3d/townsite/townsite_withHudfw.x3d");
	//fwl_startFreeWRL("../../../../tests/Roelofs/Roelofs_concaveFace_JASversion.wrl");
	//fwl_startFreeWRL("../../../../tests/Roelofs/Roelofs_concaveFace.x3d");
	//fwl_startFreeWRL("../../../../freewrl/Generic/winGLES2/HudIcons.wrl");
	fwl_initializeRenderSceneUpdateScene();
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
   printf("%s\n",glGetString(GL_VERSION));
   printf("%s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
   
   if ( !fwInit ( &esContext ) )
      return 0;
   	//statusbarOnResize(600,352);
	fwl_setScreenDim(600,352);

   //esRegisterUpdateFunc ( &esContext, fwUpdate );
   esRegisterDrawFunc ( &esContext, fwDraw );
   esRegisterKeyFunc ( &esContext, fwOnKey );
   esRegisterMouseFunc ( &esContext, fwOnMouse );
   esRegisterResizeFunc( &esContext, fwOnResize );
   
   esMainLoop ( &esContext );
   finalizeRenderSceneUpdateScene();

}
