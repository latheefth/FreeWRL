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

}

#include <malloc.h>
#include <WinUser.h>
#include <stdlib.h>
static char *fontPath = NULL; //once per process for the DLL should do it
void setFontPath()
{
	/* deployed system (with intalled fonts) - use system fonts  
	we plan to use a professional installer to install the fonts to %windir%\Fonts directory 
	where all the system fonts already are.
	Then in this program we will get the %windir%\Fonts directory, and set it as temporary
	environment variable for InputFunctions.C > makeFontsDirectory() to fetch.
	*/
	static char *fdir;
	char *syspath;
	if(fontPath == NULL){
		syspath = getenv("windir");
		printf("windir path=[%s]\n",syspath);
		fdir = (char *)malloc(1024); 
		strcpy(fdir,"FREEWRL_FONTS_DIR=");
		strcat(fdir,syspath);
		strcat(fdir,"\\Fonts");
		_putenv( fdir );
		fontPath = fdir;
	}
}

// This is an example of an exported variable
DLLFREEWRL_API int ndllFreeWRL=0;

// This is an example of an exported function.
DLLFREEWRL_API int fndllFreeWRL(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see dllFreeWRL.h for the class definition
//	handle - window handle or null
//		- if you have a window already created, you should pass in the handle, 
//		- else pass null and a window will be created for you
CdllFreeWRL::CdllFreeWRL()
{
	this->globalcontexthandle = 0;
}
void CdllFreeWRL::onInit(int width, int height, void* windowhandle, bool bEai)
{
	struct freewrl_params *params;
	//if( !fwl_setCurrentHandle(handle) ){
	this->globalcontexthandle = fwl_init_instance(); //before setting any structs we need a struct allocated
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
	params->frontend_handles_display_thread = false;
	swDebugf("just before fwl_initFreeWRL\n");
	fwl_ConsoleSetup(MC_DEF_AQUA , MC_TARGET_AQUA , MC_HAVE_MOTIF , MC_TARGET_MOTIF , MC_MSC_HAVE_VER , 0);
#ifdef CONSOLE
	fwl_setConsole_writePrimitive( 1 );
	DWORD pid = GetCurrentProcessId() ;
	initConsoleH(pid);
	swDebugf("after fwl_initFreeWRL\n");
#endif
	if (!fwl_initFreeWRL(params)) {
		//ERROR_MSG("main: aborting during initialization.\n");
		//exit(1);
	}
	setFontPath();
	//printf("press key to continue..:");
	//getchar();
	fwl_clearCurrentHandle();
	return;
}
CdllFreeWRL::CdllFreeWRL(int width, int height, void* windowhandle, bool bEai)
{
	this->onInit(width, height, windowhandle, bEai);
}
CdllFreeWRL::CdllFreeWRL(char* scene_url, int width, int height, void* windowhandle, bool bEai)
{
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
	//while(!this->globalcontexthandle) Sleep(50);
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
    
	/* when finished: */
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

//void __stdcall CdllFreeWRL::setProcessingAICommandsCallback(OnProcessingAICommands func)
//{
//	fwl_setCallBack(func, FWL_CB_ONAICOMMANDSPROCESSING);
//	return;
//}