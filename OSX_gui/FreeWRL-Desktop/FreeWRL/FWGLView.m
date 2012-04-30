#import "FreeWRLAppDelegate.h"
#import "FWGLView.h"
#import "UrlDownloader.h"
#import "../../../freex3d/src/lib/libFreeWRL.h"

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
//int mainloopCount = 0;


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
        //cString = "/FreeWRL/freewrl/freewrl/tests/16.wrl";
    }
    
    while ([FreeWRLAppDelegate applicationHasLaunched]) {
        //NSSLog (@"applicationHasLaunched false, sleeping...");
        usleep(20);

        
    }
    // wait for the main loop to go through at least once, 
    // so that we know things are initialized properly.
//    while (mainloopCount == 0) {
  //      //NSLog (@"sleeping...");
    //    usleep(20);
   // }
    
    
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
	
    fwl_setScreenDim(rectView.size.width,rectView.size.height);
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
        //NSLog (@"drawRect mainloopCount %d",mainloopCount);
        
        // ensure that the app delegate for loading is called first....
        //URLSystemRunning = (mainloopCount > 100);
        
        
        //NSLog (@"FRONT END WANTS FILENAME");
        if (!gettingURL) {
            if ([FreeWRLAppDelegate applicationHasLaunched]) {
            gettingURL = true;
            NSString *myString = [[NSString alloc] initWithUTF8String:fwg_frontEndWantsFileName()];
            //NSLog (@"string from lib is %@ as a string %s",myString,fwg_frontEndWantsFileName());
          
            //NSLog (@"going to add to queue");
            [FreeWRLAppDelegate newDoURL:myString opFlag:&gettingURL];
            }
        }

    }
    
    [[self openGLContext] makeCurrentContext];
    

	// setup viewport and prespective
	[self resizeGL]; // forces projection matrix update (does test for size changes)
    
    fwl_RenderSceneUpdateScene();

    //mainloopCount ++;
    
    
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
    
#define BUFFSIZE 2048
#define MAX_ARGC 200
   // NSLog (@"awakeFromNib");
    int mi;
#define BUFSIZE 2048
    char buff[BUFSIZE]; 
    char opt[BUFSIZE];
    bool argLookedAt[MAX_ARGC];
    unsigned long argc;
    
    for (mi=0; mi<MAX_ARGC; mi++) argLookedAt[mi] = false;

    
	// start animation timer
	timer = [NSTimer timerWithTimeInterval:(1.0f/60.0f) target:self selector:@selector(animationTimer:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSEventTrackingRunLoopMode]; // ensure timer fires during resize
    
    // lets go through and see what arguments abound
    NSProcessInfo* PInfo = [NSProcessInfo processInfo];
    NSArray* args = [PInfo arguments];
    [args retain];
    argc = [args count];
    if (argc > MAX_ARGC) argc = MAX_ARGC;
    
    
    //NSLog (@"checking for args");
    for (mi = 1; mi < argc; mi++) {
        [[args objectAtIndex:mi] getCString:buff maxLength:BUFSIZE-1 encoding:NSUTF8StringEncoding];
        if (mi <(argc-1)) {
            [[args objectAtIndex:mi+1] getCString:opt maxLength:BUFSIZE-1 encoding:NSUTF8StringEncoding];
        } else {
            opt[0] = '\0'; // no more arguments possible for this command line argument
        }

        //NSLog (@"arg at %d is %s count is %ld", mi, buff, [args count]);
        //fprintf (stderr,"FreeWRL arguments at %d is %s count is %ld\n", mi, buff, [args count]);
        
        // null argument - ignore if one found
        if ((argLookedAt[mi]) || (buff == NULL) || ([args objectAtIndex:mi] == NULL) || (!(strcmp(buff, "(null)")))){
            break;
        }

        // found a file name (possibly)
        else if (([[args objectAtIndex:mi] hasSuffix: @".wrl"]) || 
                 ([[args objectAtIndex:mi] hasSuffix: @".WRL"]) || 
                 ([[args objectAtIndex:mi] hasSuffix: @".x3d"]) || 
                 ([[args objectAtIndex:mi] hasSuffix: @".X3D"]) || 
                 ([[args objectAtIndex:mi] hasSuffix: @".x3dv"]) || 
                 ([[args objectAtIndex:mi] hasSuffix: @".X3DV"]) ){
            fileToOpen = [args objectAtIndex:mi];
            [fileToOpen getCString: buff maxLength:sizeof(buff)-1 encoding:NSUTF8StringEncoding];
            
            // does this name have a prefix? if not, prepend the current working directory
            if(!([fileToOpen hasPrefix: @"/"])) {
                char *mywd = getwd(NULL);
                //int len = strlen(mywd);
                
                char totalbuf[2048];
                [fileToOpen getCString: buff maxLength:sizeof(buff)-1 encoding:NSUTF8StringEncoding];
                
                // is this a file WITHOUT a url/uri on the front? 
                if (!fwl_checkNetworkFile(buff)) {
                    
                    // make up a path, using the cwd and the file name
                    strcpy(totalbuf,mywd);
                    strcat(totalbuf,"/");
                    strcat(totalbuf,buff);
                    fileToOpen= [NSString stringWithCString: totalbuf encoding:NSUTF8StringEncoding];
                    [fileToOpen retain];
                }
            }
        } else {
            /* Command line options - from the USE web page, Oct 2011
             --version
             --fullscreen
             --big
             --geo[metry] geom
             --eai host:port
             --server
             --sig
             --shutter
             --anaglyph LR
             --sidebyside 
             --eyedist number
             --screendist number
             --stereo number
             */
            if ([[args objectAtIndex:mi] isEqualTo: @"--version"]) {
                //NSLog (@"FreeWRL UI Version %s, Library Version %s",fwl_freewrl_get_version(), fwl_libFreeWRL_get_version()); 
                //printf ("FreeWRL UI Version %s, Library Version %s\n",fwl_freewrl_get_version(), fwl_libFreeWRL_get_version());
                
            } else if ([[args objectAtIndex:mi] isEqualTo: @"--fullscreen"]) {
                NSLog (@"command line argument :%s: ignored in this version",buff);
                
            } else if ([[args objectAtIndex:mi] isEqualTo: @"--big"]) {
            } else if (([[args objectAtIndex:mi] isEqualTo: @"--geo"]) ||
                       ([[args objectAtIndex:mi] isEqualTo: @"--geom"]) ||
                       ([[args objectAtIndex:mi] isEqualTo: @"--geometry"])) {
                argLookedAt[mi+1] = true; // next argument already peeked at
                NSLog (@"command line argument :%s: ignored in this version",buff);
                
            } else if ([[args objectAtIndex:mi] isEqualTo: @"--eai"]) {
                argLookedAt[mi+1] = true; // next argument already peeked at
                NSLog (@"command line argument :%s: ignored in this version",buff);

            } else if ([[args objectAtIndex:mi] isEqualTo: @"--server"]) {
                NSLog (@"command line argument :%s: ignored in this version",buff);

            } else if ([[args objectAtIndex:mi] isEqualTo: @"--sig"]) {
                NSLog (@"command line argument :%s: ignored in this version",buff);

            } else if ([[args objectAtIndex:mi] isEqualTo: @"--shutter"]) {
                fwl_init_Shutter();

            } else if ([[args objectAtIndex:mi] isEqualTo: @"--keypress"]) {
                fwl_set_KeyString(opt);
                argLookedAt[mi+1] = true; // next argument already peeked at
                

            } else if ([[args objectAtIndex:mi] isEqualTo: @"--anaglyph"]) {
                fwl_set_AnaglyphParameter(opt);
                argLookedAt[mi+1] = true; // next argument already peeked at
                
            } else if ([[args objectAtIndex:mi] isEqualTo: @"--sidebyside"]) {
                fwl_init_SideBySide();

            } else if ([[args objectAtIndex:mi] isEqualTo: @"--eyedist"]) {
                fwl_set_EyeDist(optarg);
                argLookedAt[mi+1] = true; // next argument already peeked at
                
            } else if ([[args objectAtIndex:mi] isEqualTo: @"--screendist"]) {
                argLookedAt[mi+1] = true; // next argument already peeked at  
                fwl_set_ScreenDist(opt);
                
            } else if ([[args objectAtIndex:mi] isEqualTo: @"--stereo"]) {
                argLookedAt[mi+1] = true; // next argument already peeked at
                fwl_set_StereoParameter(opt);
               
            } else {
                NSLog (@"unknown command line argument, :%s:",buff);
            }
        }

    }
    //NSLog (@"arg checking finished");
    

}


@end
