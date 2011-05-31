/* FreeWRLView */

#define AQUA

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



#import <libFreeWRL.h>

/* the following should be imported from libFreeWRL.h, but they are not scoped?? */
void fwl_replaceWorldNeeded(char* str);
void fwl_setCurXY(int x, int y);
void fwl_setButDown(int button, int value);


/* for front ends that do not have these X-11-based defines */
#if defined(AQUA) || defined(WIN32) || defined(_ANDROID)
#ifndef _MIMIC_X11_SCREEN_BUTTONS
#define _MIMIC_X11_SCREEN_BUTTONS
#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#define MapNotify       19
#endif
#endif

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
