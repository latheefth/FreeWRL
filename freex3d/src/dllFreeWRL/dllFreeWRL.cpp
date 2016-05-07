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
void CdllFreeWRL::setDensityFactor(float density_factor)
{
	dllFreeWRL_setDensityFactor(this->globalcontexthandle, density_factor);
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

int CdllFreeWRL::onMouse(int mouseAction,int mouseButton,int x, int y)
{
	return dllFreeWRL_onMouse(this->globalcontexthandle, mouseAction,mouseButton, x, y);
}
int CdllFreeWRL::onTouch(int touchAction, unsigned int ID, int x, int y)
{
	return dllFreeWRL_onTouch(this->globalcontexthandle, touchAction, ID, x, y);
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
void CdllFreeWRL::resitem_setStatus(void *res, int status){
	dllFreeWRL_resitem_setStatus(this->globalcontexthandle, res, status);
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
void CdllFreeWRL::commandline(char *cmdline){
	dllFreeWRL_commandline(this->globalcontexthandle, cmdline);
}

