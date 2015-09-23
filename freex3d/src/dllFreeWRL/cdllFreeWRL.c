/* dllFreeWRL.cpp : Defines the exported functions for the DLL application.
  general notes:
  Your main program -or html page- defines a single process and main thread.
  If you want to have more than one instance of freewrl (separate window and content)
  in the same process, then you need to connect the 'context' to the thread 
  functionality. But your main program is all in one thread. So you can't just use
  your main thread to select a context.
  Here we'll use a pointer to iglobal as a context handle.

*/ 
#include "cdllFreeWRL.h"

#ifdef _MSC_VER
#include "stdafx.h"
#include <windows.h>
#include <WinUser.h>
#if _MSC_VER > 1700
#ifdef WINAPI_FAMILY
#include "winapifamily.h"
#endif
#endif
#endif
#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <wtypes.h>
#include <sys/types.h>
#include "system.h"

#include "libFreeWRL.h"
#include "ui/statusbar.h"
// a few function prototypes from around libfreewrl
void fwl_setConsole_writePrimitive(int ibool);
void statusbar_set_window_size(int width, int height);
int statusbar_handle_mouse(int mev, int butnum, int mouseX, int mouseY);
int getCursorStyle();
void *fwl_frontenditem_dequeue();
char* fwl_resitem_getURL(void *res);
int	fwl_resitem_getStatus(void *res);
int	fwl_resitem_getType(void *res);
void fwl_resitem_enqueuNextMulti(void *res);
void fwl_resitem_setLocalPath(void *res, char* path);
void fwl_resitem_enqueue(void *res);
int file2blob(void *res);
#ifdef SSR_SERVER
//SSR (Server-side rendering)
void SSRserver_enqueue_request_and_wait(void *fwctx, void *request);
#endif //SSR_SERVER

#include <malloc.h>
#include <stdlib.h>

// This is the constructor of a class that has been exported.
// see dllFreeWRL.h for the class definition
DLLFREEWRL_API void * dllFreeWRL_dllFreeWRL()
{
	/*STA -single threaded app- frontends -like web pages in a browser, .net forms, xaml apps-
	can have multiple freewrl instances in different sub-windows, all running 
	in the same frontend thread. But then we can't rely on thread-lookup
	to find which freewrl instance. But the frontend developer will have
	a pointer to each instance. Then we look up the freewrl instance from that,
	using this->globalcontexthandle, fwl_setCurrentHandle(), fwl_clearCurrentHandle(),
	for the frontend-thread-synchronized part (functions called from the STA), and then
	worker threads within libfreewrl can use thread lookup to get the global instance
	for the backend parts. No thread locking is needed in the frontend-thread-sync part 
	-like here in dllfreewrl.cpp- because the frontend developer will program against 
	one dllfreewrl instance at a time due to it being STA.
	If converting this cdllfreewrl C++ to flat C interface, then add an extra
	parameter void* fwglobal to all the functions, do fwl_init_instance in the constructor
	and have it return the gglobal as void *, and the frontend programmer will hold
	the fwglobal pointer between calls.
	*/
	//this->globalcontexthandle = 0;
	return fwl_init_instance(); //before setting any structs we need a struct allocated
}
//	handle - window handle or null
//		- if you have a window already created, you should pass in the handle, 
//		- else pass null and a window will be created for you
DLLFREEWRL_API void dllFreeWRL_onInit(void *fwctx, int width, int height, void* windowhandle, int bEai, int frontend_handles_display_thread)
{
	int ok;
	struct freewrl_params *params;
	//if( !fwl_setCurrentHandle(handle) ){
	//this->globalcontexthandle = fwl_init_instance(); //before setting any structs we need a struct allocated
	fwl_setCurrentHandle(fwctx, __FILE__, __LINE__);
	/* Before we parse the command line, setup the FreeWRL default parameters */
	params = (freewrl_params_t*) malloc( sizeof(freewrl_params_t));
	memset(params,0,sizeof(freewrl_params_t));
	/* Default values */
	params->width = width; //600;
	params->height = height; //400;
	//params->eai = bEai;
	params->fullscreen = 0;
	params->winToEmbedInto = (long)windowhandle;
	params->frontend_handles_display_thread = frontend_handles_display_thread;
	ok = fwl_initFreeWRL(params);
#ifndef FRONTEND_HANDLES_DISPLAY_THREAD
	if(ok)
		if(!frontend_handles_display_thread)
			fwl_initializeDisplayThread();
#endif
#ifdef STATUSBAR_HUD
	statusbar_set_window_size(width, height);
#else
	fwl_setScreenDim(width, height);
#endif
	fwl_clearCurrentHandle();
	return;
}
DLLFREEWRL_API void dllFreeWRL_setTempFolder(void *fwctx, char *tmpFolder)
{
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		fwl_tmpFileLocation(tmpFolder);
	}
	fwl_clearCurrentHandle();
}
DLLFREEWRL_API void dllFreeWRL_setFontFolder(void *fwctx,char *fontFolder)
{
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		fwl_fontFileLocation(fontFolder);
	}
	fwl_clearCurrentHandle();
}
DLLFREEWRL_API void * dllFreeWRL_dllFreeWRL1(int width, int height, void* windowhandle, int bEai)
{
	void *fwctx;
	fwctx = fwl_init_instance(); //before setting any structs we need a struct allocated
	dllFreeWRL_onInit(fwctx, width, height, windowhandle, bEai, FALSE);
	return fwctx;
}
DLLFREEWRL_API void *dllFreeWRL_dllFreeWRL2(char* scene_url, int width, int height, void* windowhandle, int bEai)
{
	void *fwctx;
	fwctx = fwl_init_instance(); //before setting any structs we need a struct allocated
	dllFreeWRL_onInit(fwctx, width, height, windowhandle, bEai, FALSE);
	dllFreeWRL_onLoad(fwctx,scene_url);
	return fwctx;
}

DLLFREEWRL_API void dllFreeWRL_onLoad(void *fwctx, char* scene_url)
{
	char * url;
	url = _strdup(scene_url);
	if(fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		fwl_replaceWorldNeeded(url);
	}
	fwl_clearCurrentHandle();
}


DLLFREEWRL_API void dllFreeWRL_onResize(void *fwctx, int width,int height){
	if(fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
#ifdef STATUSBAR_HUD
		statusbar_set_window_size(width,height);
#else
		fwl_setScreenDim(width,height);
#endif
	}
	fwl_clearCurrentHandle();
}

DLLFREEWRL_API int dllFreeWRL_onMouse(void *fwctx, int mouseAction,int mouseButton,int x, int y){

	/*void fwl_handle_aqua(const int mev, const unsigned int button, int x, int y);*/
	/* butnum=1 left butnum=3 right (butnum=2 middle, not used by freewrl) */
	int cursorStyle = 0;
	if(fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
#ifdef STATUSBAR_HUD
		cursorStyle = statusbar_handle_mouse(mouseAction,mouseButton,x,y);
#else
		cursorStyle = fwl_handle_aqua(mouseAction,mouseButton,x,y); 
#endif
	}
	fwl_clearCurrentHandle();
	return cursorStyle;
}
DLLFREEWRL_API void dllFreeWRL_onKey(void *fwctx, int keyAction,int keyValue){
	int kp = keyValue;
	int ka = keyAction;
	if(fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		switch(keyAction)
		{
		case KEYDOWN:
			if(kp & 1 << 30) 
				break; //ignor - its an auto-repeat
		case KEYUP: 
			switch (kp) 
			{ 
				case VK_OEM_1:
					kp = ';'; //could be : or ; but tolower won't lowercase it, but returns same character if it can't
					break;
				default:
					break;
			}
			fwl_do_keyPress(kp, ka); 
			break; 

		case KEYPRESS: //WM_CHAR:
			fwl_do_keyPress(kp,ka);
			break;
		}
	}
	fwl_clearCurrentHandle();
}
DLLFREEWRL_API void dllFreeWRL_onClose(void *fwctx)
{
    
	/* when finished: as of early 2014 dug9 changed the _displayThread so now fwl_doQuit() is asynchronous meaning
	   it returns here immediately, but it takes a while for libfreewrl to finish parking threads, deleting resources
   */
	if(fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		//fwl_doQuit();
		fwl_doQuitAndWait();
	}
	fwl_clearCurrentHandle();
}
DLLFREEWRL_API void dllFreeWRL_print(void *fwctx, char *str)
{
	if(fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
	}
	fwl_clearCurrentHandle();
}
DLLFREEWRL_API void dllFreeWRL_onDraw(void *fwctx)
{
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		int more = fwl_draw();
	}
	fwl_clearCurrentHandle();
}

DLLFREEWRL_API int dllFreeWRL_getUpdatedCursorStyle(void *fwctx)
{
	int cstyle = 0;
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		cstyle = getCursorStyle();
	}
	fwl_clearCurrentHandle();
	return cstyle;
}
#if !defined(NULL) 
#define NULL (char*)0
#endif
DLLFREEWRL_API void* dllFreeWRL_frontenditem_dequeue(void *fwctx)
{
	void *item = NULL;
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		item = fwl_frontenditem_dequeue();
	}
	fwl_clearCurrentHandle();
	return item;
}
DLLFREEWRL_API char* dllFreeWRL_resitem_getURL(void *fwctx, void *res){
	char *url = NULL;
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		url = fwl_resitem_getURL(res);
	}
	fwl_clearCurrentHandle();
	return url;
}
DLLFREEWRL_API int dllFreeWRL_resitem_getStatus(void *fwctx, void *res){
	int status;
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		status = fwl_resitem_getStatus(res);
	}
	fwl_clearCurrentHandle();
	return status;
}
DLLFREEWRL_API int dllFreeWRL_resitem_getType(void *fwctx, void *res){
	int status;
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		status = fwl_resitem_getType(res);
	}
	fwl_clearCurrentHandle();
	return status;
}
DLLFREEWRL_API void dllFreeWRL_resitem_enqueuNextMulti(void *fwctx, void *res){
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		fwl_resitem_enqueuNextMulti(res);
	}
	fwl_clearCurrentHandle();
}
DLLFREEWRL_API void dllFreeWRL_resitem_setLocalPath(void *fwctx, void *res, char* path){
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		fwl_resitem_setLocalPath(res,path);
	}
	fwl_clearCurrentHandle();
}
DLLFREEWRL_API void dllFreeWRL_resitem_load(void *fwctx, void *res){
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		if (file2blob(res))
			fwl_resitem_enqueue(res);
	}
	fwl_clearCurrentHandle();
}
DLLFREEWRL_API void dllFreeWRL_resitem_enqueue(void *fwctx, void *res){
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		fwl_resitem_enqueue(res);
	}
	fwl_clearCurrentHandle();
}

#ifdef SSR_SERVER
DLLFREEWRL_API void dllFreeWRL_SSRserver_enqueue_request_and_wait(void *fwctx, void *request){
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		SSRserver_enqueue_request_and_wait(fwctx, request);
	}
	fwl_clearCurrentHandle();
}
#endif //SSR_SERVER

DLLFREEWRL_API void dllFreeWRL_commandline(void *fwctx, char *cmdline){
	if (fwl_setCurrentHandle(fwctx, __FILE__, __LINE__)){
		fwl_commandline(cmdline);
	}
	fwl_clearCurrentHandle();
}

	//void commandv(char *cmd, int argc, void **argv, int *type); //type 0=int, 1=float, 2=double, 3=char*
	//void command(char *cmd, void *arg, int type); //type 0=int, 1=float, 2=double, 3=char*
	//void keyvalue(char *key, void *value, int type); //type 0=int, 1=float, 2=double, 3=char*
