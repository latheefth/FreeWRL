// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DLLFREEWRL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DLLFREEWRL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifdef __cplusplus
extern "C" { 
#endif

#ifdef _MSC_VER
#ifdef DLLFREEWRL_EXPORTS
#define DLLFREEWRL_API __declspec(dllexport)
#else
#define DLLFREEWRL_API __declspec(dllimport)
#endif
#else
#define DLLFREEWRL_API
#endif


DLLFREEWRL_API void * dllFreeWRL_dllFreeWRL();
DLLFREEWRL_API void * dllFreeWRL_dllFreeWRL1(int width, int height, void* windowhandle, int bEai);
DLLFREEWRL_API void * dllFreeWRL_dllFreeWRL2(char *scene_url, int width, int height, void* windowhandle, int bEai);
// TODO: add your methods here.
enum KeyAction {KEYDOWN=2,KEYUP=3,KEYPRESS=1};
//#define KeyChar         1  //KeyPress
//#define KeyPress        2  //KeyDown
//#define KeyRelease      3  //KeyUp

enum MouseAction {MOUSEMOVE=6,MOUSEDOWN=4,MOUSEUP=5};
//	mev = ButtonPress; //4 down
//	mev = ButtonRelease; //3 up
//	mev = MotionNotify; //6 move
enum MouseButton {LEFT=1,MIDDLE=2,RIGHT=3,NONE=0}; 		
/* butnum=1 left butnum=3 right (butnum=2 middle, not used by freewrl) */

DLLFREEWRL_API void dllFreeWRL_setDensityFactor(void *fwctx, float density_factor);
DLLFREEWRL_API void dllFreeWRL_onInit(void *fwctx, int width, int height, void* windowhandle, int bEai, int frontend_handles_display_thread);
DLLFREEWRL_API void dllFreeWRL_onLoad(void *fwctx, char* scene_url);
DLLFREEWRL_API void dllFreeWRL_onResize(void *fwctx, int width, int height);
DLLFREEWRL_API int dllFreeWRL_onMouse(void *fwctx, int mouseAction,int mouseButton,int x, int y);
DLLFREEWRL_API int dllFreeWRL_onTouch(void *fwctx, int touchAction, unsigned int ID, int x, int y);
DLLFREEWRL_API void dllFreeWRL_onKey(void *fwctx, int keyAction,int keyValue);
DLLFREEWRL_API void dllFreeWRL_onDraw(void *fwctx); //use when FRONTEND_HANDLES_DISPLAY_THREAD
DLLFREEWRL_API void dllFreeWRL_onClose(void *fwctx);
DLLFREEWRL_API void dllFreeWRL_print(void *fwctx, char *str);
DLLFREEWRL_API void dllFreeWRL_setTempFolder(void *fwctx, char *tmpFolder);
DLLFREEWRL_API void dllFreeWRL_setFontFolder(void *fwctx, char *fontFolder);
DLLFREEWRL_API int dllFreeWRL_getUpdatedCursorStyle(void *fwctx);
DLLFREEWRL_API void* dllFreeWRL_frontenditem_dequeue(void *fwctx);
DLLFREEWRL_API char* dllFreeWRL_resitem_getURL(void *fwctx, void *res);
DLLFREEWRL_API int dllFreeWRL_resitem_getStatus(void *fwctx, void *res);
DLLFREEWRL_API void dllFreeWRL_resitem_setStatus(void *fwctx, void *res, int status);
DLLFREEWRL_API int dllFreeWRL_resitem_getType(void *fwctx, void *res);
DLLFREEWRL_API int dllFreeWRL_resitem_getMediaType(void *fwctx, void *res);
DLLFREEWRL_API void dllFreeWRL_resitem_enqueuNextMulti(void *fwctx, void *res);
DLLFREEWRL_API void dllFreeWRL_resitem_setLocalPath(void *fwctx, void *res, char* path);
DLLFREEWRL_API void dllFreeWRL_resitem_enqueue(void *fwctx, void *res);
DLLFREEWRL_API void dllFreeWRL_resitem_load(void *fwctx, void *res);
#ifdef SSR_SERVER
DLLFREEWRL_API void dllFreeWRL_SSRserver_enqueue_request_and_wait(void *fwctx, void *request);
#endif //SSR_SERVER
DLLFREEWRL_API void dllFreeWRL_commandline(void *fwctx, char *cmdline);

#ifdef __cplusplus
}
#endif
