

#define AQUA

#import "FreeWRLView.h"
#import "MyWindowController.h"
#import "wdelegate.h"
#include <OpenGL/gl.h>
#include <OpenGL/OpenGL.h>
#include "aquaInterface.h"
#include <string.h>
#include <unistd.h>



@implementation FreeWRLView

BOOL mouseOverSensitive = false;
BOOL mouseDisplaySensitive = false;


- (void) debugPrint: (char *) theString
{
	/*if (theFile == NULL) {
		theFile = fopen("/tmp/aqua_log", "w");
		if (theFile == NULL) abort();
		fileIsOpen = TRUE;
	}
	fprintf(theFile, "%s\n", theString);
	fflush(theFile);
	 */
}

- (void) print:(id)sender {
	NSFileManager* myManager;
	myManager = [NSFileManager defaultManager];
	
	fwl_init_PrintShot();
	
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
		//char dmesg[128];
	place = [theEvent locationInWindow];
	xcoor = place.x;
	        
		
		 //sprintf(dmesg, "mouse moved %f %f self %p", place.x, place.y,self);
		//[self debugPrint: dmesg];
		 
		
    ycoor = place.y;
	button = 0;
	myrect = [self frame];
	curHeight = myrect.size.height;
	
    ycoor = curHeight - place.y;
	fwl_setCurXY((int)xcoor,(int)ycoor);
	//NSLog(@"sending motion notify with %f %f\n", xcoor, ycoor);
	fwl_handle_aqua(MotionNotify, button, xcoor, ycoor);
	
	SET_CURSOR_FOR_ME	
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
	fwl_setCurXY((int)xcoor,(int)ycoor);
	fwl_setButDown(button, TRUE);
	fwl_setLastMouseEvent(ButtonPress);
	fwl_handle_aqua(ButtonPress, button, xcoor, ycoor);
	
	SET_CURSOR_FOR_ME

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
    ycoor = place.y;
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
    ycoor = place.y;
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
    ycoor = place.y;
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
    ycoor = place.y;
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

// awake from nib is called when the window opens
- (void)awakeFromNib
{
    NSPoint mouseSpot;
	char dmesg[2048];

	messageFlag = FALSE;
    mouseSpot.x = 8;
    mouseSpot.y = 8;
	theWindow = NULL;
	wantEAI = FALSE;
	fileIsOpen = FALSE;
	isPlugin = FALSE;
	switchSeq = FALSE;
	switchEAI = FALSE;
	open_opt = FALSE;
	initFinished = FALSE;
	haveFileOnCommandLine = FALSE;
	fps = 0.0;
	statString = @"NONE";
    NSProcessInfo* PInfo = [NSProcessInfo processInfo];
    NSArray* args = [PInfo arguments];
    [args retain];
    
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
					fwl_init_SnapGif();
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"maximg"]) {
					int maxImages = [[args objectAtIndex:(mi+1)] intValue];
					fwl_set_MaxImages(maxImages);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"linewidth"]) {
					float lwidth = [[args objectAtIndex:(mi+1)] floatValue];
					fwl_set_LineWidth(lwidth);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"best"]) {
					// JAS setTexSize(-256);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"keypress"]) {
					NSString* keyString = [args objectAtIndex:(mi+1)];
					char kString[528];
					[keyString getCString: kString];
					fwl_set_KeyString(kString);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"seqb"]) {
					NSString* seqFile = [args objectAtIndex:(mi+1)];
					char sFile[528];
					[seqFile getCString: sFile];
					fwl_set_SeqFile(sFile);
				}
				else if ([[args objectAtIndex: mi] hasSuffix: @"snapb"]) {
					NSString* snapFile = [args objectAtIndex:(mi+1)];
					char snFile[528];
					[snapFile getCString: snFile];
					fwl_set_SnapFile(snFile);
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
	
	fwpassObjects(self, context, [NSCursor resizeLeftCursor], [NSCursor resizeRightCursor]);
	initGL();

		
	/* create the display thread. */
	[self debugPrint: [fileToOpen cString]];
	usleep(100);

	if (haveFileOnCommandLine)
			fwl_OSX_initializeParameters([fileToOpen cString]);
	else 
			fwl_OSX_initializeParameters("/Applications/FreeWRL/blankScreen.wrl");
	initFinished = TRUE;
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
- (BOOL) hasDoneInit {
	return initFinished;
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
	fwl_askForRefreshOK();
	while (!fwl_checkRefresh()) {
		usleep(10);
	}
	[currentContext update];
	fwl_resetRefresh();
	NSSize mySize = [self frame].size;
	
	fwl_setScreenDim((int) mySize.width, (int) mySize.height);
}


- (void) setFile: (NSString*) filePassed
{
	[self debugPrint:"in set file"];
    if (!options)
    {
		haveFileOnCommandLine = TRUE;
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
	fwl_init_SnapSeq();
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
	fwl_setScreenDim((int) width, (int) height);
}

- (void) fwl_set_SeqFile: (NSString*) seqFile
{
	fwl_set_SeqFile([seqFile cString]);
}

- (void) setTempSeqFile: (NSString*) seqTempFile
{
	// JAS - 1.22.2 removes this option setSeqTemp([seqTempFile cString]);
}

- (void) fwl_set_SnapFile: (NSString*) snapFile
{
	fwl_set_SnapFile([snapFile cString]);
}

- (void) fwl_set_MaxImages: (int) maxImg
{
	fwl_set_MaxImages(maxImg);
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


- (void) resetCursorRects {
	//char dmesg[128];
	
	//sprintf (dmesg, "starting resetCursorRects, %p",self);
	//[self debugPrint: dmesg];

	[super resetCursorRects];
	
	// set the cursor that is seen at the start of the program....
	[self addCursorRect:[self visibleRect] cursor:[NSCursor crosshairCursor]];
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


/* JAS - try pushing/popping curso in the event loop */
- (void) setCrossCursor
{
	//[self debugPrint: "setting sensitive cursor"];
	
	mouseOverSensitive = true;
}

-(void) setArrowCursor
{
	//[self debugPrint: "setting normal pokey cursor"];
	
	mouseOverSensitive = false;
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
