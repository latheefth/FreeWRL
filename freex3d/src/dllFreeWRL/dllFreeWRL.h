// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DLLFREEWRL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DLLFREEWRL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef _MSC_VER
#ifdef DLLFREEWRL_EXPORTS
#define DLLFREEWRL_API __declspec(dllexport)
#else
#define DLLFREEWRL_API __declspec(dllimport)
#endif
#else
#define DLLFREEWRL_API
#endif

// This class is exported from the dllFreeWRL.dll
class DLLFREEWRL_API CdllFreeWRL {
public:
	CdllFreeWRL();
	CdllFreeWRL(int width, int height, void* windowhandle=0, bool bEai = false);
	CdllFreeWRL(char *scene_url, int width, int height, void* windowhandle=0, bool bEai = false);
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

	void setDensityFactor(float density_factor);
	void onInit(int width, int height, void* windowhandle=0, bool bEai = false, bool frontend_handles_display_thread = false);
	void onLoad(char* scene_url);
    void onResize(int width, int height);
    int onMouse(int mouseAction,int mouseButton,int x, int y);
    void onKey(int keyAction,int keyValue);
	void onDraw(); //use when FRONTEND_HANDLES_DISPLAY_THREAD
	void onClose();
	void print(char *str);
	void setTempFolder(char *tmpFolder);
	void setFontFolder(char *fontFolder);
	int getUpdatedCursorStyle();
	void* frontenditem_dequeue();
	char* resitem_getURL(void *res);
	int resitem_getStatus(void *res);
	int resitem_getType(void *res);
	void resitem_enqueuNextMulti(void *res);
	void resitem_setLocalPath(void *res, char* path);
	void resitem_enqueue(void *res);
	void resitem_load(void *res);
	void commandline(char *cmdline);
	
private:
	void *globalcontexthandle;
};

