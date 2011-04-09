/* FreeWRLView */
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <CoreFoundation/CoreFoundation.h>
#import "wdelegate.h"
#import <AppKit/AppKit.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include "MyApp.h"
#include <tcpd.h>

#include <main.h>
#include <list.h>
#include <resources.h>
#include <io_http.h>

#define DEFAULT_TIME_INTERVAL 	0.0001
#define IPADDR "127.0.0.1"
#define PORTNUM 6452
#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6

//Function Prototypes for code in the FreeWRL side of things. Used to be in
//headers.h - JAS
void fwl_handle_aqua(const int mev, const unsigned int button, int x, int y);
void fwl_setCurXY(int x, int y);
extern int getOffset();
extern void initGL();
extern void fwl_setButDown(int button, int value);
extern void fwl_setCurXY(int x, int y);
extern void fwl_do_keyPress (char ch, int ev);
extern void fwl_setLastMouseEvent(int etype);
extern void aqDisplayThread();
void setUseShapeThreadIfPossible(int x);
void setNoCollision();
bool notFinished();
void setPaneClipRect(int npx, int npy, WindowPtr fwWindow, int ct, int cb, int cr, int cl, int width, int height);
void createContext(CGrafPtr grafPtr);
void setIsPlugin();
void sendPluginFD(int fd);
void aquaPrintVersion();
void setPluginPath(char* path);
void setEaiVerbose();
void fwl_init_SnapGif();
void fwl_set_MaxImages(int);
void fwl_set_KeyString(const char* kstring);
void fwl_init_SnapSeq();
void setEAIport(int pnum);
int isShapeCompilerParsing(void);
int isinputThreadParsing(void);
int isTextureParsing(void);
void setWantEAI(int flag);
void fwl_set_SeqFile(const char* file);
int isInputThreadInitialized(void);


#include "/FreeWRL/freewrl/freex3d/src/lib/libFreeWRL.h"
//some from #include "display.h"
void fwl_setScreenDim(int wi, int he);



@interface FreeWRLView : NSView
{
	NSAutoreleasePool* pool;
    NSOpenGLContext* context;
	NSString* statString;
	double fps;
	NSRect myrect;
    float xcoor;
	NSApplication* myApp;
    float ycoor;
    int button;
    NSPoint place;
    char xstring[1];
	char tbuff[128];
    NSCursor* crossCursor;
	NSCursor* arrowCursor;
	NSCursor* currentCursor;
    NSString*	fileToOpen;
	NSString*   tempString;
	NSString*   tempFile;
    float curHeight;
    NSWindow* controlWindow;
	bool fullscreen;
    NSWindow* viewWindow;
	void* theWindow;
	wdelegate* mydelegate;
    bool options;
	BOOL open_opt;
    id controller;
	int wantEAI;
	NSView* view;
	int fw_pipe;
	int childfd;
	unsigned instance;
	bool isPlugin;
	bool fileIsOpen;
	bool switchEAI;
	bool switchSeq;
	FILE* theFile;
	NSString* theMessage;
	bool messageFlag;
	id theApp;
	int haveFileOnCommandLine;
	int initFinished;
}

- (void) setApp: (id) app;
- (void) doResize;
- (void) debugPrint: (char *) theString;
- (void) set_fast;
- (void) initScreen;
- (void) mouseDown: (NSEvent *) theEvent;
- (void) mouseDragged: (NSEvent *) theEvent;
- (void) mouseUp: (NSEvent *) theEvent;
- (void) rightMouseDown: (NSEvent *) theEvent;
- (void) rightMouseUp: (NSEvent *) theEvent;
- (void) rightMouseDragged: (NSEvent *) theEvent;
- (void) keyUp: (NSEvent *) theEvent;
- (void) keyDown: (NSEvent *) theEvent;
- (BOOL) acceptsFirstResponder;
- (BOOL) needsPanelToBecomeKey;
- (void) setFile: (NSString*) filePassed;
- (void) setView: (NSView*) theView;
- (void) setCrossCursor;
- (void) setArrowCursor;
- (void) setWinPtr: (void*) win;
- (void) setHeight: (int) height width: (int) width;
- (void) setSeqImages;
- (void) setEAIport: (int) portnum;
- (void) setWindow;
- (void) stopEai;
- (void) startEai;
- (void) initBrowser;
- (BOOL) getOpenOpt;
- (BOOL) hasDoneInit;
- (bool) getOptions;
- (Boolean) getFullscreen;
- (void) setController:(id) passedController;
- (void) setHeadlightButton: (int) val;
- (void) setCollisionButton: (int) val;
- (void) setStatusMess: (char*) stat;
- (void) setStatusFps: (double) fps;
- (void) setConsoleMess: (char*) str;
- (void) setNavMode: (int) type;
- (void) createReleasePool;
- (void) print: (id) sender;
- (NSString*) getFileName;
@end
