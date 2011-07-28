#import "FreeWRLAppDelegate.h"
#import "FWGLView.h"
#import "UrlDownloader.h"
#import "/FreeWRL/freewrl/freex3d/src/lib/libFreeWRL.h"

// ==================================




bool gettingURL = false;
NSPoint place;
float xcoor;
float ycoor;
int button;
BOOL mouseOverSensitive = false;
BOOL mouseDisplaySensitive = false;
NSRect myrect;
float curHeight;
NSMutableData *receivedData;

#define TOP_BAR_HEIGHT 0.0

// wait for loading until loopCount != 0, so things "can get set up"
int mainloopCount = 0;


// ===================================
// get the initial URL in, and load'er up!

@interface initializerURL : NSObject
+(void)firstMethod:(id)param;
@end

@implementation initializerURL
+(void)firstMethod:(id)param {
    
    char* cString;
    
    
    //NSLog (@"starting loading thread");
    
    //can not do any opengl calls in this thread here.
    //[[self openGLContext] makeCurrentContext];
    
    //NSLog (@"calling fwl_initializeRenderSceneUpdateScene");
    fwl_initializeRenderSceneUpdateScene();
    
    // has fwl_RenderSceneUpdateScene run at least once??
    
    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    // the user hit return, and we are flying...
    if (fileToOpen != nil) {
        cString = (char *)[fileToOpen UTF8String];
    } else {
        // no file specified; go here.
        cString = "/Applications/FreeWRL/blankScreen.wrl";
        //cString = "/FreeWRL/freewrl/freewrl/tests/13.wrl";
    }
    
    // wait for the main loop to go through at least once, 
    // so that we know things are initialized properly.
    while (mainloopCount == 0) {
        //NSLog (@"sleeping...");
        usleep(20);
    }
    
    
    //NSLog (@"initial url %s",cString);

    //NSLog (@"calling fwl_OSX_initializeParameters");
    
    fwl_OSX_initializeParameters((const char*)cString);
    //NSLog (@"finished calling fwl_OSX_initializeParameters");
    [pool drain];
    
    //NSLog (@"ending loading thread");
    
}
@end


@implementation FWGLView
 

// start of additions

// pixel format definition
+ (NSOpenGLPixelFormat*) basicPixelFormat
{
    NSOpenGLPixelFormatAttribute attributes [] = {
            NSOpenGLPFANoRecovery,
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAWindow,
            NSOpenGLPFAAccelerated,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFADepthSize, 24,
            NSOpenGLPFAStencilSize, 8,
            NSOpenGLPFAAccumSize, 0,
            0
    };
    return [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];
}


// ---------------------------------

// handles resizing of GL need context update and if the window dimensions change, a
// a window dimension update, reseting of viewport and an update of the projection matrix
- (void) resizeGL
{
	NSRect rectView = [self bounds];
	
	// ensure camera knows size changed
	if ((camera.viewHeight != rectView.size.height) ||
	    (camera.viewWidth != rectView.size.width)) {
		camera.viewHeight = rectView.size.height;
		camera.viewWidth = rectView.size.width;
        
        //NSLog (@"fwl_setScreenDim(%d, %d)",camera.viewWidth,camera.viewHeight);
        fwl_setScreenDim(camera.viewWidth,camera.viewHeight);
        //NSLog (@"finished resize GL");
	}
}


// ---------------------------------


// per-window timer function, basic time based animation preformed here
- (void)animationTimer:(NSTimer *)timer
{
    //NSLog (@"timer tick");
    // Just draw
    [self drawRect:[self bounds]];
}


#pragma mark ---- IB Actions ----

-(IBAction) animate: (id) sender
{
	fAnimate = 1 - fAnimate;
	if (fAnimate)
		[animateMenuItem setState: NSOnState];
	else 
		[animateMenuItem setState: NSOffState];
}

// ---------------------------------

-(IBAction) info: (id) sender
{
    // unsure if this is required anymore
    
//	fInfo = 1 - fInfo;
//	if (fInfo)
//		[infoMenuItem setState: NSOnState];
//	else
//		[infoMenuItem setState: NSOffState];
	[self setNeedsDisplay: YES];
}

#pragma mark ---- Method Overrides ----

#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#define MapNotify       19




#define SET_CURSOR_FOR_ME \
if (mouseOverSensitive != mouseDisplaySensitive) { \
if (mouseOverSensitive) { \
/*[[NSCursor disappearingItemCursor] push]; */ \
[[NSCursor pointingHandCursor] push]; \
} else { \
[NSCursor pop]; \
} \
mouseDisplaySensitive = mouseOverSensitive; \
}

- (void) mouseMoved: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
    xcoor = place.x;
    
      
    ycoor = place.y;
    button = 0;
    myrect = [self frame];
    curHeight = myrect.size.height;
    
   
   
    ycoor = curHeight - place.y;
    //NSLog (@"mouse moved, place.y %f", place.y);
    
    fwl_setCurXY((int)xcoor,(int)ycoor);
    //NSLog(@"sending motion notify with %f %f\n", xcoor, ycoor);
    fwl_setLastMouseEvent(ButtonPress);
    fwl_handle_aqua(MotionNotify, button, xcoor, ycoor);
        
    
    
    SET_CURSOR_FOR_ME
}

- (void) mouseDown: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
    xcoor = place.x;
    ycoor = place.y + TOP_BAR_HEIGHT;
    
    
    myrect = [self frame];
    curHeight = myrect.size.height;
    if ([theEvent modifierFlags] & NSControlKeyMask)
    {
        button = 3;
    }
    else
    {
        button = 1;
    }
    ycoor = curHeight - place.y;
    fwl_setCurXY((int)xcoor,(int)ycoor);
    fwl_setButDown(button, TRUE);
    fwl_setLastMouseEvent(ButtonPress);
    fwl_handle_aqua(ButtonPress, button, xcoor, ycoor);
    
    SET_CURSOR_FOR_ME
    
}
- (void) mouseDragged: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
    if ([theEvent modifierFlags] & NSControlKeyMask)
    {
        button = 3;
    }
    else
    {
        button = 1;
    }
    xcoor = place.x;
    ycoor = place.y + TOP_BAR_HEIGHT;
        
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
    //      NSLog(@"xcoor %f ycoor %f\n", xcoor, ycoor);
    //NSLog(@"sending motion notify with %f %f\n", xcoor, ycoor);
    fwl_setCurXY((int)xcoor,(int)ycoor);
    fwl_setLastMouseEvent(MotionNotify);
    fwl_handle_aqua(MotionNotify, button, xcoor, ycoor);
}

- (void) mouseUp: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
    if ([theEvent modifierFlags] & NSControlKeyMask)
    {
        button = 3;
    }
    else
    {
        button = 1;
    }
    
    xcoor = place.x;
    ycoor = place.y + TOP_BAR_HEIGHT;
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
    fwl_setButDown(button, FALSE);
    fwl_setCurXY((int)xcoor,(int)ycoor);
    fwl_setLastMouseEvent(ButtonRelease);
    fwl_handle_aqua(ButtonRelease, button, xcoor, ycoor);
    
    SET_CURSOR_FOR_ME
}

- (void) rightMouseDown: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
    button = 3;
    xcoor = place.x;
    ycoor = place.y + TOP_BAR_HEIGHT;
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
    fwl_setCurXY((int)xcoor,(int)ycoor);
    fwl_setButDown(button, TRUE);
    fwl_setLastMouseEvent(ButtonPress);
    fwl_handle_aqua(ButtonPress, button, xcoor, ycoor);
}
- (void) rightMouseUp: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
    button = 3;
    xcoor = place.x;
    ycoor = place.y + TOP_BAR_HEIGHT;
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
    fwl_setCurXY((int)xcoor,(int)ycoor);
    fwl_setButDown(button, FALSE);
    fwl_setLastMouseEvent(ButtonRelease);
    fwl_handle_aqua(ButtonRelease, button, xcoor, ycoor);
}
- (void) rightMouseDragged: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
    button = 3;
    xcoor = place.x;
    ycoor = place.y + TOP_BAR_HEIGHT;
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
    fwl_setCurXY((int)xcoor,(int)ycoor);
    fwl_setLastMouseEvent(MotionNotify);
    fwl_handle_aqua(MotionNotify, button, xcoor, ycoor);
}
- (void) keyUp: (NSEvent*) theEvent
{
    NS_DURING
    NSString* character = [theEvent characters];
    char ks;
    ks = (char) [character characterAtIndex: 0];
    fwl_do_keyPress(ks, KeyRelease);
    NS_HANDLER
    return;
    NS_ENDHANDLER
}
- (void) keyDown: (NSEvent*) theEvent
{
    NS_DURING
    NSString* character = [theEvent characters];
    char ks;
    ks = (char) [character characterAtIndex: 0];
    //NSLog(@"got char down: ll%cll\n", ks);
    fwl_do_keyPress(ks, KeyPress);
    NS_HANDLER
    return;
    NS_ENDHANDLER
}




// ---------------------------------

- (void) drawRect:(NSRect)rect
{		
    
    if (fwg_frontEndWantsFileName() != nil) {
        //NSLog (@"FRONT END WANTS FILENAME");
        if (!gettingURL) {
            gettingURL = true;
            NSString *myString = [[NSString alloc] initWithUTF8String:fwg_frontEndWantsFileName()];
            //NSLog (@"string from lib is %@ as a string %s",myString,fwg_frontEndWantsFileName());
          
            //NSLog (@"going to add to queue");
            [FreeWRLAppDelegate newDoURL:myString opFlag:&gettingURL];
           
        }

    }
    
    [[self openGLContext] makeCurrentContext];
    

	// setup viewport and prespective
	[self resizeGL]; // forces projection matrix update (does test for size changes)
    
    fwl_RenderSceneUpdateScene();

    mainloopCount ++;
    
    
    [[self openGLContext] flushBuffer];
}

// ---------------------------------

// set initial OpenGL state (current context is set)
// called after context is created
- (void) prepareOpenGL
{
    GLint swapInt = 1;
    
    //NSLog(@"calling fwl_init_instance");
    fwl_init_instance();

    //NSLog (@"calling fv_display_initialize");
    
    fv_display_initialize();

    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval]; // set to vbl sync

    
    //NSLog (@"starting thread to load in new file here");
    [NSThread detachNewThreadSelector:@selector(firstMethod:) toTarget:[initializerURL class] withObject:nil];


}
// ---------------------------------

- (void) update // window resizes, moves and display changes (resize, depth and display config change)
{
	[super update];
}

// ---------------------------------

-(id) initWithFrame: (NSRect) frameRect
{
    //NSLog(@"initWithFrame");
    NSOpenGLPixelFormat * pf = [FWGLView basicPixelFormat];
    
	self = [super initWithFrame: frameRect pixelFormat: pf];

    // allow tracking of certain things, like MotionNotify, etc.
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc] 
                    initWithRect:[self bounds] 
                    options: (NSTrackingMouseEnteredAndExited |
                            NSTrackingMouseMoved |
                            NSTrackingCursorUpdate |
                            NSTrackingActiveAlways)
                    owner:self userInfo:nil];
    
    [self addTrackingArea:trackingArea];

    return self;
}

-(void)mouseEntered:(NSEvent *)theEvent {
    
    //NSLog(@"mouse entered");
    
}

-(void)mouseExited:(NSEvent *)theEvent {
    
    //NSLog(@"mouse exited");
    
}


// ---------------------------------

- (BOOL)acceptsFirstResponder
{
  return YES;
}

// ---------------------------------

- (BOOL)becomeFirstResponder
{
  return  YES;
}

// ---------------------------------

- (BOOL)resignFirstResponder
{
  return YES;
}

// ---------------------------------

- (void) awakeFromNib
{
    //NSLog (@"awakeFromNib");
    int mi;
    char buff[2048]; 
    
	// start animation timer
	timer = [NSTimer timerWithTimeInterval:(1.0f/60.0f) target:self selector:@selector(animationTimer:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSEventTrackingRunLoopMode]; // ensure timer fires during resize
    
    // lets go through and see what arguments abound
    NSProcessInfo* PInfo = [NSProcessInfo processInfo];
    NSArray* args = [PInfo arguments];
    [args retain];
    
    NSLog (@"checking for args");
    for (mi = 1; mi < [args count]; mi++) {
        [[args objectAtIndex:mi] getCString:buff maxLength:sizeof(buff)-1 encoding:NSUTF8StringEncoding];
        NSLog (@"arg at %d is %s count is %d", mi, buff, [args count]);
        
        // null argument - ignore if one found
        if ((buff == NULL) || ([args objectAtIndex:mi] == NULL) || (!(strcmp(buff, "(null)")))){
            break;
        }

        // found a file name (possibly)
        else if (([[args objectAtIndex:mi] hasSuffix: @".wrl"]) || ([[args objectAtIndex:mi] hasSuffix: @".x3d"]) || ([[args objectAtIndex:mi] hasSuffix: @".X3D"]) || ([[args objectAtIndex:mi] hasSuffix: @".x3dv"]) || ([[args objectAtIndex:mi] hasSuffix: @".X3DV"]) ){
            fileToOpen = [args objectAtIndex:mi];
            [fileToOpen getCString: buff maxLength:sizeof(buff)-1 encoding:NSUTF8StringEncoding];
            
            // does this name have a prefix? if not, prepend the current working directory
            if(!([fileToOpen hasPrefix: @"/"])) {
                char *mywd = getwd(NULL);
                int len = strlen(mywd);
                
                char totalbuf[2048];
                [fileToOpen getCString: buff maxLength:sizeof(buff)-1 encoding:NSUTF8StringEncoding];
                
                // is this a file WITHOUT a url/uri on the front? 
                if (!checkNetworkFile(buff)) {
                    
                    // make up a path, using the cwd and the file name
                    strcpy(totalbuf,mywd);
                    strcat(totalbuf,"/");
                    strcat(totalbuf,buff);
                    fileToOpen= [NSString stringWithCString: totalbuf encoding:NSUTF8StringEncoding];
                    [fileToOpen retain];
                }
            }
        } else {
            
            
        }

    }
    //NSLog (@"arg checking finished");
    

}


@end
