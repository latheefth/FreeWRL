#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/glu.h>
#include "libFreeWRL.h"
extern void initGL();
void fwl_handle_aqua(const int mev, const unsigned int button, int x, int y);
void fwl_setCurXY(int x, int y);
extern void fwl_setButDown(int button, int value);
extern void fwl_setCurXY(int x, int y);
extern void fwl_do_keyPress (char ch, int ev);
extern void fwl_setLastMouseEvent(int etype);
extern void fwl_setScreenDim(int wi, int he);
void fwl_OSX_initializeParameters(const char* initialURL);
void kill_oldWorld(int kill_EAI, int kill_JavaScript, char *file, int line);
void stopRenderingLoop();
extern int ocurse;


#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6

/* file for debugging purposes */
FILE* theFile; 
bool fileIsOpen;

@interface X3DView : NSView {
  @private
	NSOpenGLContext* oglContext;
	NSOpenGLPixelFormat* oglPixelFormat;
	NSTimer* timer;
	float xcoor;
	NSApplication* myApp;
    float ycoor;
    int button;
    NSPoint place;
	NSRect myrect;
	float curHeight;
	int isNotFirst;
	NSImage* stopImage;
}
- (void)setArguments:(NSDictionary *)arguments;
- (void) mouseDown: (NSEvent *) theEvent;
- (void) mouseDragged: (NSEvent *) theEvent;
- (void) mouseUp: (NSEvent *) theEvent;
- (void) rightMouseDown: (NSEvent *) theEvent;
- (void) rightMouseUp: (NSEvent *) theEvent;
- (void) rightMouseDragged: (NSEvent *) theEvent;
- (void) keyUp: (NSEvent *) theEvent;
- (void) keyDown: (NSEvent *) theEvent;

+ (NSOpenGLPixelFormat*)defaultPixelFormat;

@end
