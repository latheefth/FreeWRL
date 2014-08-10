/* dllFreeWRL.cpp : Defines the exported functions for the DLL application.
  general notes:
  Your main program -or html page- defines a single process and main thread.
  If you want to have more than one instance of freewrl (separate window and content)
  in the same process, then you need to connect the 'context' to the thread 
  functionality. But your main program is all in one thread. So you can't just use
  your main thread to select a context.
  Here we'll use a pointer to iglobal as a context handle.

*/ 

#include "stdafx.h"
#include "dllFreeWRL.h"

#include <windows.h>
#if _MSC_VER > 1700
#include "winapifamily.h"
#endif
#include <stdio.h>
#include <stdarg.h>
#include <wtypes.h>
//#define CONSOLE _DEBUG
#ifdef CONSOLE
#undef CONSOLE
static HANDLE hStdErr = NULL;
static void
swInitConsole(void)
{
    BOOL ac = AllocConsole();
    if (ac)
        hStdErr = GetStdHandle(STD_ERROR_HANDLE);
}
void swDebugf(LPCSTR formatstring, ...)
{
    char buff[2500];
    DWORD cWritten;
	int nSize;
   memset(buff, 0, sizeof(buff));
   va_list args;
   va_start(args, formatstring);
   
   nSize = _vsnprintf( buff, sizeof(buff) - 1, formatstring, args); // C4996
    if (hStdErr == NULL)
        swInitConsole(); 
    //_vsnprintf(buff, BUFSIZE, fmt, ap);
    //OutputDebugStringA(buff);

    /* not C console - more low level windows SDK API */
    WriteConsoleA(hStdErr, buff, strlen(buff),&cWritten, NULL);
}
#else
//int ConsoleMesage(char *_Format,...);
//#define swDebugf ConsoleMessage
int swDebugf(LPCSTR formatstring, ...) {return 0;}
#endif

extern "C"
{
#include "libFreeWRL.h"
#include "ui/statusbar.h"
void initConsoleH(DWORD pid);
//char *strBackslash2fore(char *str);
void fwl_setConsole_writePrimitive(int ibool);
void statusbar_set_window_size(int width, int height);
void statusbar_handle_mouse(int mev, int butnum, int mouseX, int mouseY);
int getCursorStyle();
void *fwl_frontenditem_dequeue();
char* fwl_resitem_getURL(void *res);
int	fwl_resitem_getStatus(void *res);
int	fwl_resitem_getType(void *res);
void fwl_resitem_enqueuNextMulti(void *res);
void fwl_resitem_setLocalPath(void *res, char* path);
void fwl_resitem_enqueue(void *res);
int file2blob(void *res);
}

#include <malloc.h>
#include <WinUser.h>
#include <stdlib.h>

// This is an example of an exported variable
DLLFREEWRL_API int ndllFreeWRL=0;

// This is an example of an exported function.
DLLFREEWRL_API int fndllFreeWRL(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see dllFreeWRL.h for the class definition
CdllFreeWRL::CdllFreeWRL()
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
	this->globalcontexthandle = fwl_init_instance(); //before setting any structs we need a struct allocated
}
//	handle - window handle or null
//		- if you have a window already created, you should pass in the handle, 
//		- else pass null and a window will be created for you
void CdllFreeWRL::onInit(int width, int height, void* windowhandle, bool bEai, bool frontend_handles_display_thread)
{
	int ok;
	struct freewrl_params *params;
	//if( !fwl_setCurrentHandle(handle) ){
	//this->globalcontexthandle = fwl_init_instance(); //before setting any structs we need a struct allocated
	fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__);
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
	swDebugf("just before fwl_initFreeWRL\n");
	ok = fwl_initFreeWRL(params);
	//if(!ok){
		//ERROR_MSG("main: aborting during initialization.\n");
		//exit(1);
	//} 
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
	//printf("press key to continue..:");
	//getchar();
	fwl_clearCurrentHandle();
	return;
}
void CdllFreeWRL::setTempFolder(char *tmpFolder)
{
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		fwl_tmpFileLocation(tmpFolder);
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::setFontFolder(char *fontFolder)
{
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		fwl_fontFileLocation(fontFolder);
	}
	fwl_clearCurrentHandle();
}
CdllFreeWRL::CdllFreeWRL(int width, int height, void* windowhandle, bool bEai)
{
	this->globalcontexthandle = fwl_init_instance(); //before setting any structs we need a struct allocated
	this->onInit(width, height, windowhandle, bEai);
}
CdllFreeWRL::CdllFreeWRL(char* scene_url, int width, int height, void* windowhandle, bool bEai)
{
	this->globalcontexthandle = fwl_init_instance(); //before setting any structs we need a struct allocated
	this->onInit(width, height, windowhandle, bEai);
	this->onLoad(scene_url);
}

	//enum class KeyAction {KEYDOWN,KEYUP,KEYPRESS};
	//enum class MouseAction {MOUSEMOVE,MOUSEDOWN,MOUSEUP};
	//enum class MouseButton {LEFT,MIDDLE,RIGHT,NONE};
//extern "C"{
//int fv_display_initialize(void);
//}

void CdllFreeWRL::onLoad(char* scene_url)
{
	//char * url;
	url = _strdup(scene_url);
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		//url = strBackslash2fore(url);
		//swDebugf("onLoad have url=[%s]\n",url);
		fwl_replaceWorldNeeded(url);
		//swDebugf("onLoad after push_single_request url=[%s]\n",url);
	}
	fwl_clearCurrentHandle();

}


void CdllFreeWRL::onResize(int width,int height){
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		//swDebugf("onResize before\n");
#ifdef STATUSBAR_HUD
		statusbar_set_window_size(width,height);
#else
		fwl_setScreenDim(width,height);
#endif
		//swDebugf("onResize after\n");
	}
	fwl_clearCurrentHandle();
}
//#define KeyChar         1
//#if defined(AQUA) || defined(WIN32)
//#define KeyPress        2
//#define KeyRelease      3
//#define ButtonPress     4
//#define ButtonRelease   5
//#define MotionNotify    6
//#define MapNotify       19
//#endif

void CdllFreeWRL::onMouse(int mouseAction,int mouseButton,int x, int y){

	/*void fwl_handle_aqua(const int mev, const unsigned int button, int x, int y);*/
	/* butnum=1 left butnum=3 right (butnum=2 middle, not used by freewrl) */
	//fwl_handle_aqua(mev,butnum,mouseX,mouseY); 
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		//swDebugf("onMouse before\n");
#ifdef STATUSBAR_HUD
		statusbar_handle_mouse(mouseAction,mouseButton,x,y);
#else
		fwl_handle_aqua(mouseAction,mouseButton,x,y); 
#endif
		//swDebugf("onMouse after\n");
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::onKey(int keyAction,int keyValue){

	int kp = keyValue;
	int ka = keyAction;
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		//swDebugf("onKey before\n");
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
		//swDebugf("onKey after\n");
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::onClose()
{
    
	/* when finished: as of early 2014 dug9 changed the _displayThread so now fwl_doQuit() is asynchronous meaning
	   it returns here immediately, but it takes a while for libfreewrl to finish parking threads, deleting resources
	   
	   */
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		//swDebugf("onClose before -fwl_doQuitInstance being called\n");
		//fwl_doQuitInstance();
		fwl_doQuit();
		//swDebugf("onClose after\n");
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::print(char *str)
{
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		//swDebugf(str);
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::onDraw()
{
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		int more = fwl_draw();
	}
	fwl_clearCurrentHandle();
}

int CdllFreeWRL::getUpdatedCursorStyle()
{
	int cstyle = 0;
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		cstyle = getCursorStyle();
	}
	fwl_clearCurrentHandle();
	return cstyle;
}

void* CdllFreeWRL::frontenditem_dequeue()
{
	void *item = nullptr;
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		item = fwl_frontenditem_dequeue();
	}
	fwl_clearCurrentHandle();
	return item;
}
char* CdllFreeWRL::resitem_getURL(void *res){
	char *url = nullptr;
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		url = fwl_resitem_getURL(res);
	}
	fwl_clearCurrentHandle();
	return url;
}
int CdllFreeWRL::resitem_getStatus(void *res){
	int status;
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		status = fwl_resitem_getStatus(res);
	}
	fwl_clearCurrentHandle();
	return status;
}
int CdllFreeWRL::resitem_getType(void *res){
	int status;
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		status = fwl_resitem_getType(res);
	}
	fwl_clearCurrentHandle();
	return status;
}
void CdllFreeWRL::resitem_enqueuNextMulti(void *res){
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		fwl_resitem_enqueuNextMulti(res);
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::resitem_setLocalPath(void *res, char* path){
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		fwl_resitem_setLocalPath(res,path);
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::resitem_load(void *res){
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		if (file2blob(res))
			fwl_resitem_enqueue(res);
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::resitem_enqueue(void *res){
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		fwl_resitem_enqueue(res);
	}
	fwl_clearCurrentHandle();
}

//void __stdcall CdllFreeWRL::setProcessingAICommandsCallback(OnProcessingAICommands func)
//{
//	fwl_setCallBack(func, FWL_CB_ONAICOMMANDSPROCESSING);
//	return;
//}