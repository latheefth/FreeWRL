#import "FreeWRLView.h"
#import "MyWindowController.h"
#import "wdelegate.h"
#define AQUA
#include <OpenGL/gl.h>
#include <OpenGL/OpenGL.h>
#include "aquaInterface.h"
#include <string.h>
#include <unistd.h>


@implementation FreeWRLView

- (void) debugPrint: (char *) theString
{
	
//	if (theFile == NULL) {
//		theFile = fopen("/tmp/aqua_log", "w");
//		if (theFile == NULL) abort();
//		fileIsOpen = TRUE;
//	}
//	fprintf(theFile, "%s\n", theString);
//	fflush(theFile);
}

- (void) print:(id)sender {
	NSFileManager* myManager;
	myManager = [NSFileManager defaultManager];
	
	setPrintShot();
	
	while (TRUE) {
		usleep(10000);
		if ([myManager fileExistsAtPath: @"/tmp/FW_print_snap_tmp.png"]) {
			break;
		}
	}
	NSImage* myImage = [[NSImage alloc] initWithContentsOfFile:@"/tmp/FW_print_snap_tmp.png"];
	[myManager removeFileAtPath:@"/tmp/FW_print_snap_tmp.png" handler:NULL];
	NSSize mySize = [myImage size];
	NSRect theFrame;
	theFrame.origin.x = 0;
	theFrame.origin.y = 0;
	theFrame.size.width = mySize.width;
	theFrame.size.height = mySize.height;
	NSImageView *myview = [[NSImageView alloc] initWithFrame:theFrame];
	theFrame = [myview frame];
	[myview setImage:myImage];
	NSPrintInfo *info = [NSPrintInfo sharedPrintInfo];
    NSPrintOperation *printOp = [NSPrintOperation printOperationWithView:myview printInfo:info];
    [printOp setShowsPrintPanel:YES];
    [printOp runOperation]; 
}

- (void) createReleasePool
{
	pool = [[NSAutoreleasePool alloc] init];
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (BOOL) needsPanelToBecomeKey
{
    return YES;
}

- (void) mouseMoved: (NSEvent *) theEvent
{
		char dmesg[128];
	place = [theEvent locationInWindow];
	xcoor = place.x;
	        
		sprintf(dmesg, "mouse moved %f %f", place.x, place.y);
		[self debugPrint: dmesg];
		
    ycoor = place.y;
	button = 0;
	myrect = [self frame];
	curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
	setCurXY((int)xcoor,(int)ycoor);
	handle_aqua(MotionNotify, button, xcoor, ycoor);
	
}
- (void) mouseDown: (NSEvent *) theEvent
{
	char dmesg[128];
	place = [theEvent locationInWindow];
	xcoor = place.x;
	ycoor = place.y;

		sprintf(dmesg, "mouse down %f %f", place.x, place.y);
		[self debugPrint: dmesg];
		
		
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
	char dmesg[128];
	
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
    ycoor = place.y;
	
			sprintf(dmesg, "mouse dragged %f %f", place.x, place.y);
		[self debugPrint: dmesg];

    myrect = [self frame];
    curHeight = myrect.size.height;
    ycoor = curHeight - place.y;
//	NSLog(@"xcoor %f ycoor %f\n", xcoor, ycoor);
	setCurXY((int)xcoor,(int)ycoor);
	setLastMouseEvent(MotionNotify);
	handle_aqua(MotionNotify, button, xcoor, ycoor);
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
		//NSLog(@"got char down: ll%cll\n", ks);
		do_keyPress(ks, KeyPress);
        NS_HANDLER
        return;
        NS_ENDHANDLER
}

// awake from nib is called when the window opens
- (void)awakeFromNib
{
    NSPoint mouseSpot;
	char dmesg[2048];
	setUseShapeThreadIfPossible(0);
	messageFlag = FALSE;
	//setUseCParser(0);
    mouseSpot.x = 8;
    mouseSpot.y = 8;
	theWindow = NULL;
	wantEAI = FALSE;
	fileIsOpen = FALSE;
	isPlugin = FALSE;
	switchSeq = FALSE;
	switchEAI = FALSE;
	open_opt = FALSE;
	haveFileOnCommandLine = FALSE;
	fps = 0.0;
	statString = @"NONE";
    NSProcessInfo* PInfo = [NSProcessInfo processInfo];
    NSArray* args = [PInfo arguments];
    [args retain];
    
    //NSString* path = [[NSBundle mainBundle] pathForResource: @"cross" ofType: @"gif"];
    //NSImage* crossImage = [[[NSImage alloc] initWithContentsOfFile: path] retain];
    //crossCursor = [[[NSCursor alloc] initWithImage: crossImage hotSpot: mouseSpot] retain];    
	crossCursor = [[NSCursor pointingHandCursor] retain];
	arrowCursor = [[NSCursor arrowCursor] retain];
	[self addCursorRect:[self visibleRect] cursor:arrowCursor];
	currentCursor = arrowCursor;
    options = NO;
	fullscreen = FALSE;

	
	[self debugPrint: "before if"];
	
	// If we started from the command line, parse arguments
    if ([args count] > 1)
    {
        NSRange range;
        range.location = 1;
        range.length = 3;
        
		sprintf(dmesg, "args count %d", [args count]);
		[self debugPrint: dmesg];
		
        fileToOpen = [args objectAtIndex:1];
		
		[fileToOpen retain];
        if ([fileToOpen compare: @"psn" options:0 range:range] == NSOrderedSame)
        {
            options = NO;
        }
        else
        {
			char buff[2048]; //JAS - was 512, but some long filenames tend to give us problems.
            options = YES;
			open_opt = TRUE;
			int mi;
			int myi;

			for (mi = 1; mi < [args count]; mi++) {
				[self debugPrint: "     ... "];
				[self debugPrint: "getting next"];
				[[args objectAtIndex:mi] getCString: buff];
				sprintf(dmesg, "arg at %d is %s count is %d", mi, buff, [args count]);
				[self debugPrint: dmesg];
				if ((buff == NULL) || ([args objectAtIndex:mi] == NULL) || (!(strcmp(buff, "(null)")))){
					[self debugPrint: "null argument"];
					break;
				}
				else if (([[args objectAtIndex:mi] hasSuffix: @".wrl"]) || ([[args objectAtIndex:mi] hasSuffix: @".x3d"]) || ([[args objectAtIndex:mi] hasSuffix: @".X3D"]) || ([[args objectAtIndex:mi] hasSuffix: @".x3dv"]) || ([[args objectAtIndex:mi] hasSuffix: @".X3DV"]) ){
					fileToOpen = [args objectAtIndex:mi];
					[fileToOpen getCString: buff];
					[self debugPrint: "arg is filename"];
					[self debugPrint: buff];
					
					// does this name have a prefix? if not, prepend the current working directory
					if(!([fileToOpen hasPrefix: @"/"])) {
						[self debugPrint: "getting getwd"];
						char *mywd = getwd(NULL);
						[self debugPrint: "getwd returns:"];
						[self debugPrint: mywd];
						int len = strlen(mywd);
						sprintf(dmesg, "getwd is %d long",(int)strlen(mywd));
						[self debugPrint: dmesg];
						
						char totalbuf[2048];
						[fileToOpen getCString: buff];
						
						// is this a file WITHOUT a url/uri on the front? 
						if (!checkNetworkFile(buff)) {
						
							// make up a path, using the cwd and the file name
							sprintf(totalbuf,mywd);
							strcat(totalbuf,"/");
							strcat(totalbuf,buff);
							len = strlen(totalbuf);
							sprintf (dmesg, "C string for file is :%s:",totalbuf);
							[self debugPrint: dmesg];
							fileToOpen= [NSString stringWithCString: totalbuf length: len];
							[fileToOpen retain];
						}
					}
				}
				else if ([[args objectAtIndex:mi] hasPrefix: @"HD:"]) {
					tempFile = [args objectAtIndex:(mi)];
					fileToOpen = [tempFile substringFromIndex: 2];
					[fileToOpen getCString: buff];
					[self debugPrint: "have HD: about to print before swap"];
					[self debugPrint: buff];
					for (myi = 0; myi < 2048; myi++) { // JAS - was 512, now 2048
						if (buff[myi] == ':')
								buff[myi] = '/';
					}
					int len = strlen(buff);
					fileToOpen = [NSString stringWithCString: buff length: len];
					[fileToOpen retain];
					[fileToOpen getCString: buff];
					[self debugPrint: buff];

				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"eai"]) {
					wantEAI = TRUE;
					switchEAI = TRUE;
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"eaiverbose"]) {
					setEaiVerbose();
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"geom"]) {
					char geo[256];
					int xval, yval;
					NSString* geomString = [args objectAtIndex:(mi+1)];
					[geomString getCString: geo]; 
					sscanf(geo, "%dx%d", &xval, &yval);
					sprintf(dmesg, "setting height to %d, width to %d\n", yval, xval);
					[self debugPrint: dmesg];
					[self setHeight: yval width: xval];
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"big"]) {
					[self setHeight: 600 width: 800];
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"seq"]) {
					[self debugPrint: "got seq image flag"];
					[self setSeqImages];
					switchSeq = TRUE;
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"nostatus"]) {
					[self debugPrint: "got no status"];
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"nocollision"]) {
					setNoCollision();
					exit(0);
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"version"]) {
					aquaPrintVersion();
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"--gif"]) {
					setSnapGif();
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"maximg"]) {
					int maxImages = [[args objectAtIndex:(mi+1)] intValue];
					setMaxImages(maxImages);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"linewidth"]) {
					float lwidth = [[args objectAtIndex:(mi+1)] floatValue];
					setLineWidth(lwidth);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"best"]) {
					setTexSize(-256);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"keypress"]) {
					NSString* keyString = [args objectAtIndex:(mi+1)];
					char kString[528];
					[keyString getCString: kString];
					setKeyString(kString);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"seqb"]) {
					NSString* seqFile = [args objectAtIndex:(mi+1)];
					char sFile[528];
					[seqFile getCString: sFile];
					setSeqFile(sFile);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"snapb"]) {
					NSString* snapFile = [args objectAtIndex:(mi+1)];
					char snFile[528];
					[snapFile getCString: snFile];
					setSnapFile(snFile);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"seqtmp"]) {
					NSString* seqtFile = [args objectAtIndex:(mi+1)];
					char stFile[528];
					[seqtFile getCString: stFile];
					//JAS 1.22.2 removes this option setSeqTemp(stFile);
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"plugin"]) {
					childfd = [[args objectAtIndex:(mi+1)] intValue];
					isPlugin = TRUE;
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"eaiport"]) {
					int eai_portnum = [[args objectAtIndex:(mi+1)] intValue];
					[self setEAIport: eai_portnum];
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"fd"]) {
					fw_pipe = [[args objectAtIndex:(mi+1)] intValue];
					isPlugin = TRUE;
					sprintf(dmesg, "pipe: %u\n", fw_pipe);
					[self debugPrint: dmesg];

				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"instance"]) {
					instance = [[args objectAtIndex:(mi+1)] intValue];
					isPlugin = TRUE;
					sprintf(dmesg, "instance: %u\n", instance);
					[self debugPrint: dmesg];
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"fullscreen"]) {
					fullscreen = TRUE;
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"shutter"]) {
					setShutter();
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"anaglyph"]) {
					char anaglyph[256];
					NSString* anaString = [args objectAtIndex:(mi+1)];
					[anaString getCString: anaglyph];
					setAnaglyphParameter(anaglyph);
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"sidebyside"]) {
					setSideBySide();
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"eyedist"]) {
					char eyeDist[256];
					NSString* eyeString = [args objectAtIndex:(mi+1)];
					[eyeString getCString: eyeDist];
					setEyeDist(eyeDist);
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"screendist"]) {
					char screenDist[256];
					NSString* screenString = [args objectAtIndex:(mi+1)];
					[screenString getCString: screenDist];
					setScreenDist(screenDist);
				}
				else if ([[args objectAtIndex:mi] hasSuffix: @"stereo"]) {
					char stereo[256];
					NSString* stereoString = [args objectAtIndex:(mi+1)];
					[stereoString getCString: stereo];
					setStereoParameter(stereo);
				}
			}
        }
    }
	[self debugPrint: "out of if"];
}

- (Boolean) getFullscreen {
	return fullscreen;
}

- (void) initBrowser
{

	
	[viewWindow makeKeyAndOrderFront: nil];
	[self debugPrint: "in init browser - going to fwpassObjects"];
	context = [NSOpenGLContext currentContext];
	
	fwpassObjects(self, context, crossCursor, [NSCursor arrowCursor]);
	initGL();

		
	/* create the display thread. */
	[self debugPrint: [fileToOpen cString]];

	if (haveFileOnCommandLine)
			OSX_initializeParameters([fileToOpen cString]);
	else 
		OSX_initializeParameters("/Applications/FreeWRL/blankScreen.wrl");
		//OSX_initializeParameters("/FreeWRL/freewrl/freewrl/tests/1.wrl");
	
	/* do we require EAI? */
	if (wantEAI) {
		create_EAI();
		setWantEAI(1);
	} else {
		setWantEAI(0);
	}
	
    // Tell the window to track all mouse events
    NSWindow* myWindow = [self window];
	if (haveFileOnCommandLine) [myWindow setTitle: fileToOpen];
    [myWindow setAcceptsMouseMovedEvents: YES];
	
	// create a delegate
	mydelegate = [[wdelegate alloc] init];
	[mydelegate retain];
	[myWindow setDelegate: mydelegate];
	[mydelegate setView: self];
    options = NO;
	[self doResize];
}

- (bool) getOptions {
	return options;
}

- (void) set_fast
{
    glShadeModel(GL_FLAT);
}

- (void) drawRect: (NSRect) rect
{	
}

- (void) doResize
{	
	NSOpenGLContext* currentContext;
	currentContext = [NSOpenGLContext currentContext];
	[currentContext setView:self];
	[currentContext update];
	NSSize mySize = [self frame].size;
	
	setScreenDim((int) mySize.width, (int) mySize.height);
}


- (void) setFile: (NSString*) filePassed
{
    if (!options)
    {
        fileToOpen = filePassed;
		[fileToOpen retain];
    }
}

- (BOOL) getOpenOpt
{
	char dmesg[128];

	sprintf(dmesg, "open opt is %d", open_opt);
	[self debugPrint: dmesg];

	if (open_opt) {
		open_opt = FALSE;
		return FALSE;
	} else {
		return TRUE;
	}
}

- (void) setSeqImages
{
	setSnapSeq();
}

- (void) setEAIport: (int) portNum
{
	setEAIport(portNum);
}	

- (void) startEai
{
		wantEAI = TRUE;
}

- (void) stopEai
{
	wantEAI = FALSE;
}

- (void) setHeight: (int) height width: (int) width
{
    NSSize size;
    NSWindow* myWindow = [self window];
    size.width = (float) width;
    size.height = (float) height;
    [myWindow setContentSize: size];
    [self setFrameSize: size];
	setScreenDim((int) width, (int) height);
}

- (void) setSeqFile: (NSString*) seqFile
{
	setSeqFile([seqFile cString]);
}

- (void) setTempSeqFile: (NSString*) seqTempFile
{
	// JAS - 1.22.2 removes this option setSeqTemp([seqTempFile cString]);
}

- (void) setSnapFile: (NSString*) snapFile
{
	setSnapFile([snapFile cString]);
}

- (void) setMaxImages: (int) maxImg
{
	setMaxImages(maxImg);
}

- (void) initScreen
{
    NSArray* windowsArray = [NSApp windows];
    int loop, max;
    max = [windowsArray count];
	
	[self debugPrint: "in initScreen"];
    for (loop = 0; loop < max; loop++)
    {
        NSWindow* curWindow = [windowsArray objectAtIndex: loop];
        NSString* curTitle = [curWindow title];
        if ([curTitle compare: @"FreeWRL"] == NSOrderedSame)
        {
            viewWindow = curWindow;
            [viewWindow retain];
			[self setWindow];
        }
    }
    if (options)
    {
		[self debugPrint: "in options"];
		[viewWindow makeKeyAndOrderFront: nil];
		[NSApp activateIgnoringOtherApps: YES];
		haveFileOnCommandLine = TRUE;
		//[self initBrowser];
    }
	[self initBrowser];
}
- (void) setController:(id) passedController
{
	[self debugPrint: "in setController"];
	controller = passedController;
}

- (void) setWindow
{
    [controller setWindow: viewWindow];
}

- (void) setView: (NSView*) passedView
{
	view = passedView;
}

- (void) setCrossCursor
{
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	
	[crossCursor set];	
	currentCursor = crossCursor;
	[currentCursor retain];
	[[self window] invalidateCursorRectsForView:self];
	[viewWindow makeKeyWindow];
	
	[[self window] makeKeyWindow];
	[[self window] display];
	[self setNeedsDisplay:YES];
	[mypool release];
}
- (void) resetCursorRects {
	[super resetCursorRects];
	//[[self window] invalidateCursorRectsForView:self];
	[self addCursorRect:[self visibleRect] cursor:currentCursor];
	if (currentCursor != nil && currentCursor != [NSCursor arrowCursor]) {
	      //NSLog(@"pointing cursor");
		[self addCursorRect:[self visibleRect] cursor: crossCursor];
	} else {
			//NSLog(@"arrow cursor");
	      [self addCursorRect:[self visibleRect] cursor:arrowCursor];
	}
}

- (void) setApp: (id) app
{
	theApp = app;
	if (switchEAI) {
			[theApp setEAIFlag: 1];
	}
	if (switchSeq) {
			[theApp setSeqFlag: 1];
	}
	if (messageFlag) {
		[theApp setMessage: theMessage];
		messageFlag = FALSE;
	}
}

- (void) setArrowCursor
{
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	[arrowCursor set];
	currentCursor = arrowCursor;

	[[self window] invalidateCursorRectsForView:self];
	[viewWindow makeKeyWindow];

	[[self window] makeKeyWindow];
	[[self window] display];
	[self setNeedsDisplay:YES];
	[mypool release];
}

- (void) setWinPtr: (void*) win
{
	[self debugPrint: "in setWinPtr"];
	theWindow = win;
}

- (void) setHeadlightButton: (int) val {
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	[theApp toggleHLButton: val];
	[mypool release];
}

- (void) setCollisionButton: (int) val {
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	[theApp toggleCollButton: val];
	[mypool release];
}

- (void) setStatusMess: (char*) stat {
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	if (isinputThreadParsing() || isTextureParsing() || (!isInputThreadInitialized())) {
		[theApp setStatusMessage: [NSString stringWithFormat: @"Loading ..."]];
	} else {
		statString = [[NSString alloc] initWithCString: stat encoding: [NSString defaultCStringEncoding]];
		[theApp setStatusMessage: [NSString stringWithFormat: @"fps: %1.1f   Viewpoint: %@", fps, statString]];
	}
	[mypool release];
}

- (void) setStatusFps: (double) myfps {
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	if (isinputThreadParsing() || isTextureParsing() || (!isInputThreadInitialized())) {
		[theApp setStatusMessage: [NSString stringWithFormat: @"Loading ..."]];
	} else {
		fps = myfps;
		[theApp setStatusMessage: [NSString stringWithFormat: @"fps: %1.1f   Viewpoint: %@", fps, statString]];
	}
	[mypool release];
}

- (void) setTexSize: (int) size {
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	[theApp setTexSize: size];
	[mypool release];
}

- (void) setNavMode: (int) type {
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	[theApp toggleNavButton: type];
	[mypool release];
}

- (void) setConsoleMess: (char*) msg {
	NSAutoreleasePool* mypool = [[NSAutoreleasePool alloc] init];
	if (theApp == NULL) {
		messageFlag = TRUE;
		theMessage = [NSString stringWithFormat: @"%s", msg];
		[theMessage retain];
	} else {
		[theApp setMessage: [NSString stringWithFormat: @"%s", msg]];
	}
	[mypool release];
}

- (NSString*) getFileName {
	return fileToOpen;
}


@end
