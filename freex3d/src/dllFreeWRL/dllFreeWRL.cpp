/* dllFreeWRL.cpp : Defines the exported functions for the DLL application.
  general notes:
  Your main program -or html page- defines a single process and main thread.
  If you want to have more than one instance of freewrl (separate window and content)
  in the same process, then you need to connect the 'context' to the thread 
  functionality. But your main program is all in one thread. So you can't just use
  your main thread to select a context.
  Here we'll use a pointer to iglobal as a context handle.

*/ 



#include "dllFreeWRL.h"
#define DELEGATE_TO_C 1
#ifdef DELEGATE_TO_C
#include "cdllFreeWRL.h"
#endif

// This is the constructor of a class that has been exported.
// see dllFreeWRL.h for the class definition

#ifdef DELEGATE_TO_C
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
	this->globalcontexthandle = dllFreeWRL_dllFreeWRL(); //before setting any structs we need a struct allocated
}
//	handle - window handle or null
//		- if you have a window already created, you should pass in the handle, 
//		- else pass null and a window will be created for you
void CdllFreeWRL::onInit(int width, int height, void* windowhandle, bool bEai, bool frontend_handles_display_thread)
{
	int BEai, FEHDT;
	FEHDT = frontend_handles_display_thread ? 1 : 0;
	BEai = bEai ? 1 : 0;
	dllFreeWRL_onInit(this->globalcontexthandle, width, height, windowhandle, BEai, FEHDT);
	return;
}
void CdllFreeWRL::setTempFolder(char *tmpFolder)
{
	dllFreeWRL_setTempFolder(this->globalcontexthandle, tmpFolder);
}
void CdllFreeWRL::setFontFolder(char *fontFolder)
{
	dllFreeWRL_setFontFolder(this->globalcontexthandle, fontFolder);
}
CdllFreeWRL::CdllFreeWRL(int width, int height, void* windowhandle, bool bEai)
{
	int BEai;
	BEai = bEai ? 1 : 0;
	this->globalcontexthandle = dllFreeWRL_dllFreeWRL1(width, height, windowhandle, BEai);
}
CdllFreeWRL::CdllFreeWRL(char* scene_url, int width, int height, void* windowhandle, bool bEai)
{
	int BEai;
	BEai = bEai ? 1 : 0;
	this->globalcontexthandle = dllFreeWRL_dllFreeWRL2(scene_url, width, height, windowhandle, BEai);
}

void CdllFreeWRL::onLoad(char* scene_url)
{
	dllFreeWRL_onLoad(this->globalcontexthandle, scene_url);
}


void CdllFreeWRL::onResize(int width,int height){
	dllFreeWRL_onResize(this->globalcontexthandle, width, height);
}

void CdllFreeWRL::onMouse(int mouseAction,int mouseButton,int x, int y)
{
	dllFreeWRL_onMouse(this->globalcontexthandle, mouseAction,mouseButton, x, y);
}
void CdllFreeWRL::onKey(int keyAction,int keyValue)
{
	dllFreeWRL_onKey(this->globalcontexthandle, keyAction, keyValue);
}
void CdllFreeWRL::onClose()
{
    dllFreeWRL_onClose(this->globalcontexthandle);
}
void CdllFreeWRL::print(char *str)
{
	dllFreeWRL_print(this->globalcontexthandle, str);
}
void CdllFreeWRL::onDraw()
{
	dllFreeWRL_onDraw(this->globalcontexthandle); 
}

int CdllFreeWRL::getUpdatedCursorStyle()
{
	return dllFreeWRL_getUpdatedCursorStyle(this->globalcontexthandle);
}
void* CdllFreeWRL::frontenditem_dequeue()
{
	return dllFreeWRL_frontenditem_dequeue(this->globalcontexthandle);
}
char* CdllFreeWRL::resitem_getURL(void *res)
{
	return dllFreeWRL_resitem_getURL(this->globalcontexthandle, res);
}
int CdllFreeWRL::resitem_getStatus(void *res)
{
	return dllFreeWRL_resitem_getStatus(this->globalcontexthandle, res);
}
int CdllFreeWRL::resitem_getType(void *res)
{
	return dllFreeWRL_resitem_getType(this->globalcontexthandle, res);
}
void CdllFreeWRL::resitem_enqueuNextMulti(void *res){
	dllFreeWRL_resitem_enqueuNextMulti(this->globalcontexthandle, res);
}
void CdllFreeWRL::resitem_setLocalPath(void *res, char* path)
{
	dllFreeWRL_resitem_setLocalPath(this->globalcontexthandle, res, path);
}
void CdllFreeWRL::resitem_load(void *res)
{
	dllFreeWRL_resitem_load(this->globalcontexthandle, res);
}
void CdllFreeWRL::resitem_enqueue(void *res){
	dllFreeWRL_resitem_enqueue(this->globalcontexthandle, res);
}


#else //DELEGATE_TO_C


#ifdef _MSC_VER
#include "stdafx.h"
#include <windows.h>
#include <WinUser.h>
#if _MSC_VER > 1700
#include "winapifamily.h"
#endif
#endif

#include <stdio.h>
#include <stdarg.h>
#include <wtypes.h>

extern "C"
{
#include "libFreeWRL.h"
#include "ui/statusbar.h"
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
#include <stdlib.h>


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

void CdllFreeWRL::onLoad(char* scene_url)
{
	char * url;
	url = _strdup(scene_url);
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		fwl_replaceWorldNeeded(url);
	}
	fwl_clearCurrentHandle();

}


void CdllFreeWRL::onResize(int width,int height){
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
#ifdef STATUSBAR_HUD
		statusbar_set_window_size(width,height);
#else
		fwl_setScreenDim(width,height);
#endif
	}
	fwl_clearCurrentHandle();
}

void CdllFreeWRL::onMouse(int mouseAction,int mouseButton,int x, int y){

	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
#ifdef STATUSBAR_HUD
		statusbar_handle_mouse(mouseAction,mouseButton,x,y);
#else
		fwl_handle_aqua(mouseAction,mouseButton,x,y); 
#endif
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::onKey(int keyAction,int keyValue){

	int kp = keyValue;
	int ka = keyAction;
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
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
void CdllFreeWRL::onClose()
{
    
	/* when finished: as of early 2014 dug9 changed the _displayThread so now fwl_doQuit() is asynchronous meaning
	   it returns here immediately, but it takes a while for libfreewrl to finish parking threads, deleting resources
   */
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		fwl_doQuit();
	}
	fwl_clearCurrentHandle();
}
void CdllFreeWRL::print(char *str)
{
	if(fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
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
#if !defined(nullptr) 
#define nullptr (char*)0
#endif
void* CdllFreeWRL::frontenditem_dequeue()
{
	void *item = nullptr;
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		item = fwl_frontenditem_dequeue();
	}
	fwl_clearCurrentHandle();
	return item;
}
char* CdllFreeWRL::resitem_getURL(void *res)
{
	char *url = nullptr;
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		url = fwl_resitem_getURL(res);
	}
	fwl_clearCurrentHandle();
	return url;
}
int CdllFreeWRL::resitem_getStatus(void *res)
{
	int status;
	if (fwl_setCurrentHandle(this->globalcontexthandle, __FILE__, __LINE__)){
		status = fwl_resitem_getStatus(res);
	}
	fwl_clearCurrentHandle();
	return status;
}
int CdllFreeWRL::resitem_getType(void *res)
{
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



#endif //DELEGATE_TO_C

