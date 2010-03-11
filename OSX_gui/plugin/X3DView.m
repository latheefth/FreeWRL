#import "X3DView.h"

#import <WebKit/WebKit.h>
#include <OpenGL/gl.h>

@implementation X3DView
NSDictionary *_arguments;

static int freewrlInitialized = FALSE;
static int freewrlCurrentlyRunning = FALSE;
static X3DView* firstView = NULL;

- (void) mouseMoved: (NSEvent *) theEvent
{
	place = [theEvent locationInWindow];
	place = [self convertPoint: place fromView: nil];
	
	xcoor = (float) place.x;		
    ycoor = (float) place.y;
	button = 0;
	myrect = [self frame];
	curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
	setCurXY((int)xcoor,(int)ycoor);
	handle_aqua(MotionNotify, button, xcoor, ycoor);
	
}
- (void) mouseDown: (NSEvent *) theEvent
{
	place = [theEvent locationInWindow];
	place = [self convertPoint: place fromView: nil];
	xcoor = place.x;
	ycoor = place.y;
		
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
	setCurXY((int)xcoor,(int)ycoor);
	setButDown(button, TRUE);
	setLastMouseEvent(ButtonPress);
	handle_aqua(ButtonPress, button, xcoor, ycoor);
	
}
- (void) mouseDragged: (NSEvent *) theEvent
{
	
    place = [theEvent locationInWindow];
	place = [self convertPoint: place fromView: nil];
    if ([theEvent modifierFlags] & NSControlKeyMask)
    {
        button = 3;
    }
    else
    {
        button = 1;
    }
	//NSLog(@"Event is %@", theEvent);
	
    xcoor = (float) place.x;
    ycoor = (float) place.y;
	
	
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
	setCurXY((int)xcoor,(int)ycoor);
	setLastMouseEvent(MotionNotify);
	handle_aqua(MotionNotify, button, xcoor, ycoor);
}

- (void) mouseUp: (NSEvent *) theEvent
{
	place = [theEvent locationInWindow];
	place = [self convertPoint: place fromView: nil];
    if ([theEvent modifierFlags] & NSControlKeyMask)
    {
        button = 3;
    }
    else
    {
        button = 1;
    }
	
    xcoor = place.x;
    ycoor = place.y;
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
	setButDown(button, FALSE);
	setCurXY((int)xcoor,(int)ycoor);
	setLastMouseEvent(ButtonRelease);
	handle_aqua(ButtonRelease, button, xcoor, ycoor);
}

- (void) rightMouseDown: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
	place = [self convertPoint: place fromView: nil];
    button = 3;
    xcoor = place.x;
    ycoor = place.y;
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
	setCurXY((int)xcoor,(int)ycoor);
	setButDown(button, TRUE);
	setLastMouseEvent(ButtonPress);
	handle_aqua(ButtonPress, button, xcoor, ycoor);
}
- (void) rightMouseUp: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
	place = [self convertPoint: place fromView: nil];
    button = 3;
    xcoor = place.x;
    ycoor = place.y;
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
	setCurXY((int)xcoor,(int)ycoor);
	setButDown(button, FALSE);
	setLastMouseEvent(ButtonRelease);
	handle_aqua(ButtonRelease, button, xcoor, ycoor);
}
- (void) rightMouseDragged: (NSEvent *) theEvent
{
    place = [theEvent locationInWindow];
	place = [self convertPoint: place fromView: nil];
    button = 3;
    xcoor = place.x;
    ycoor = place.y;
    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
	setCurXY((int)xcoor,(int)ycoor);
	setLastMouseEvent(MotionNotify);
	handle_aqua(MotionNotify, button, xcoor, ycoor);
}
- (void) keyUp: (NSEvent*) theEvent
{
	NS_DURING
	NSString* character = [theEvent characters];
	char ks;
	ks = (char) [character characterAtIndex: 0];
	do_keyPress(ks, KeyRelease);
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
	do_keyPress(ks, KeyPress);
	NS_HANDLER
	return;
	NS_ENDHANDLER
}


/* + (NSView *)plugInViewWithArguments:(NSDictionary *)arguments
{
    MovieView *movieView = [[[self alloc] init] autorelease];
	[movieView setArguments:arguments];
    return movieView;
} */


- (void)setArguments:(NSDictionary *)arguments
{
	//NSLog(@"Called set arguments with %@", arguments);
    if (arguments != _arguments) {
		[_arguments release];
		_arguments = [arguments copy];
    }
	
}

- (void)dealloc
{
    [super dealloc];
}


+ (NSOpenGLPixelFormat*)defaultPixelFormat {
  NSOpenGLPixelFormatAttribute attribs[] = {0};
  return [[(NSOpenGLPixelFormat*)[NSOpenGLPixelFormat alloc] initWithAttributes:attribs] autorelease];
}

- (id)initWithFrame:(NSRect)frame {
	NSOpenGLPixelFormat* fmt;
	NSOpenGLPixelFormatAttribute attribs[] =
	{
		NSOpenGLPFAWindow,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFADepthSize,
		(NSOpenGLPixelFormatAttribute) 16,
		(NSOpenGLPixelFormatAttribute) nil
		
	};
	fmt = [[[NSOpenGLPixelFormat alloc] initWithAttributes: (NSOpenGLPixelFormatAttribute*) attribs] autorelease];
	oglPixelFormat = [fmt retain];
	
    self = [super initWithFrame:frame];	

	
	return self;
}


- (void) drawRect: (NSRect) bounds
{
	if (isNotFirst) {
		//NSLog(@"is not first drawRect");
		//[stopImage drawAtPoint:NSZeroPoint fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1];
	} else  {
		CGLContextObj cglContext;
		cglContext = CGLGetCurrentContext();
	}
}

- (void) doResize
{
	//NSLog(@"do resize");
	NSOpenGLContext* currentContext;
	NSSize mySize = [self frame].size;
	setScreenDim((int) mySize.width, (int) mySize.height);

	currentContext = [NSOpenGLContext currentContext];
	[currentContext setView:self];
	[currentContext update];
	
}

- (void) setFrame: (NSRect) frameRect {
	//NSLog(@"In set frame");
	[super setFrame: frameRect];
	if (!isNotFirst)
		[self doResize];
}

- (void)lockFocus {
	BOOL widthIsPercent = FALSE;
	BOOL heightIsPercent = FALSE;
	CGLContextObj cglContext;
	if (!freewrlInitialized && !freewrlCurrentlyRunning && !isNotFirst) {
		
		//NSLog(@"not init, not running");
		long newSwapInterval;
		
		freewrlInitialized = TRUE;
		
		oglContext = [[[NSOpenGLContext alloc] initWithFormat: oglPixelFormat shareContext: nil] retain];
		
		[oglContext makeCurrentContext];
		
		cglContext = CGLGetCurrentContext();
		CGLSetCurrentContext(cglContext);
		
		newSwapInterval = 1;
		CGLSetParameter(cglContext, kCGLCPSwapInterval, ((const GLint *) &newSwapInterval));
		
		firstView = self;
		initGL();
	}

	if (!freewrlCurrentlyRunning &&  !isNotFirst) {
				//NSLog(@"not running");
	    freewrlCurrentlyRunning = TRUE;		
		
		NSEnumerator *enumerator = [_arguments keyEnumerator];
		id key;
		
		/* while ( key = [enumerator nextObject] ) {
			printf( "%s => %s\n",
				   [[key description] UTF8String],
				   [[[_arguments objectForKey: key] description] UTF8String] );
		} */
		
		// lets get the URL out of here.
		NSDictionary *webPluginAttributesObj = [_arguments objectForKey:WebPlugInAttributesKey];
		NSString *URLString = [webPluginAttributesObj objectForKey:@"src"];
		int urlStringLen = [URLString length];
		
		//for some reason this does not work... if ([URLString length] != 0) {
		if (urlStringLen != 0) {
			NSURL *baseURL = [_arguments objectForKey:WebPlugInBaseURLKey];
			NSURL *URL = [NSURL URLWithString:URLString relativeToURL:baseURL];
			
			//printf ("URL is %s\n",[[URL absoluteString] UTF8String]);
			OSX_initializeParameters([[URL absoluteString] UTF8String]);
		} else {
			// woah nelly! the url does not exist in the embed tag! lets just start something
			//printf("Oops ... no URL passed?");
			OSX_initializeParameters("/Applications/FreeWRL/blankScreen.wrl");
		}
		
		// set the width/height of the window
		int hei = 200; int wid = 200;
		NSString *heightString = [webPluginAttributesObj objectForKey:@"height"];
		if ([heightString hasSuffix: @"%"]) {
			heightIsPercent = TRUE;
		} 
		if (heightString != nil) {
			sscanf ([heightString UTF8String], "%d",&hei);
		}
		NSString *widthString = [webPluginAttributesObj objectForKey:@"width"];
		if ([widthString hasSuffix: @"%"]) {
			widthIsPercent = TRUE;
		} 
		if (widthString != nil) {
			sscanf ([widthString UTF8String], "%d",&wid);
		}
		NSSize mySize = [self frame].size;
		if (heightIsPercent) {
			hei = hei/100 * mySize.height;
		}
		if (widthIsPercent) {
			wid = wid/100 * mySize.width;
		}
		setScreenDim(wid,hei);
		
		[oglContext setView: self];
	}
	[[self window] makeFirstResponder:self];
    [super lockFocus];
}


- (void)webPlugInInitialize
{
	//printf ("webPlugInInitialize\n");
	NSBundle *bundle = [NSBundle bundleForClass:[self class]];
	NSString* imageName = [bundle pathForResource:@"simple" ofType:@"png"];
	//NSLog(@"imageName is %@\n", imageName);
	stopImage = [[NSImage alloc] initWithContentsOfFile:imageName];
	isNotFirst = FALSE;
	if (stopImage == nil) {
		NSLog(@"Couldn't get image");
	}
}

- (void)webPlugInStart
{
	//printf ("webPlugInStart, I am %u\n", self);
	if (freewrlCurrentlyRunning && (self != firstView) && (firstView != NULL)) {
		//printf("already RUNNING\n");
		isNotFirst = TRUE;
		NSRect myFrame = [self frame];
		
		//NSLog(@"Frame is %d %d %f %f\n", myFrame.origin.x, myFrame.origin.y, myFrame.size.width, myFrame.size.height);
		myFrame.origin.x = 0;
		myFrame.origin.y = 0;
		myFrame.size.width = 500;
		myFrame.size.height = 240;
		NSImageView* theView = [[NSImageView alloc] initWithFrame: myFrame];
		//[theView setImageFrameStyle:NSImageFramePhoto];
		[theView setImageAlignment:NSImageAlignCenter];
		//[theView setImageScaling:NSScaleProportionally];
		[theView setImage: stopImage];
		[self addSubview:theView];
		[theView setNeedsDisplay: YES];
		
	}
}

- (void)webPlugInStop
{
	//printf("webPluginStop\n");

}

- (void)webPlugInDestroy
{
	//printf("webplugin destroy\n");
	if (!isNotFirst) {
		freewrlCurrentlyRunning = FALSE;
		freewrlInitialized = FALSE;
		stopRenderingLoop();
		firstView = NULL;
	}
}

- (void)webPlugInIsSelected
{

}

- (BOOL) acceptsFirstResponder
{
	return YES;
} 

- (BOOL) becomeFirstResponder
{
	return  YES;
} 

@end