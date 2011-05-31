//
//  MyApp.m
//  FreeWRL
//
//  Created by Sarah Dumoulin on Fri Jan 30 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import "MyApp.h"
#import "libFreeWRL.h"

#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#define NONE 0
#define EXAMINE 1
#define WALK 2
#define EXFLY 3
#define FLY 4


@implementation MyApp

- (void) debugPrint: (char *) theString
{
	
//	if (theFile == NULL) {
//		theFile = fopen("/tmp/aqua_log2", "w");
//		if (theFile == NULL)
//			abort();
//		fileIsOpen = TRUE;
//	}

//	fprintf(theFile, "%s\n", theString);
//	fflush(theFile);
}

- (void) applicationWillFinishLaunching: (NSNotification*) aNotification
{
	wc = [[MyWindowController alloc] initWithFreeWRL: (FreeWRLView*) theView];
	[wc retain];
}

- (void) applicationDidFinishLaunching: (NSNotification*) aNotification
{
	[wc initScreen];
	freewrl = [wc getView];
	[freewrl setApp: self];
	[ConsoleDrawer setContentSize: NSMakeSize(50, 150)];
	[MessageDrawer setContentSize: NSMakeSize(22, 22)];
	[texPriority setState: NSOnState];
	colourPanel = [NSColorPanel sharedColorPanel];
	//[colourPanel setShowsAlpha: TRUE];
	//NSColor* defaultColour = [NSColor blackColor];
	//[colourPanel setColor: defaultColour];
}

- (IBAction) setEAI: (id) sender
{
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];
		[freewrl stopEai];
	} else {
		[sender setState: NSOnState];
		[freewrl startEai];
	}
}

- (IBAction) Coll: (id) sender {
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];
	} else {
		[sender setState: NSOnState];
	}
	ks = 'c';
	fwl_do_keyPress(ks, KeyPress);
}

- (void) toggleCollButton: (int) val {
	//printf("got to toggleColl button with int %d\n", val);
	if (val) {
		if ([Collision state] == NSOffState) {
			//printf("turning on");
			[Collision setState: NSOnState];
		}
	} else {
		if ([Collision state] == NSOnState) {
			//printf("turning off");
			[Collision setState: NSOffState];
		}
	}
}

- (IBAction) ExMd: (id) sender {
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];

	} else {
		[sender setState: NSOnState];
	}
	[FlyMode setState: NSOffState];
	[WalkMode setState: NSOffState];
	ks = 'e';
	fwl_do_keyPress(ks, KeyPress);
}

- (IBAction) FlyMd: (id) sender {
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];

	} else {
		[sender setState: NSOnState];

	}
	[WalkMode setState: NSOffState];
	[ExamineMode setState: NSOffState];
	ks = 'f';
	fwl_do_keyPress(ks, KeyPress);
}


- (IBAction) HeadL: (id) sender {
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];

	} else {
		[sender setState: NSOnState];

	}
	ks = 'h';
	fwl_do_keyPress(ks, KeyPress);
}

- (void) toggleHLButton: (int) val {
	if (val) {
		[Headlight setState: NSOnState];
	} else {
		[Headlight setState: NSOffState];
	}
}

- (void) toggleNavButton: (int) type {
	if (type == EXAMINE) {
		[WalkMode setState: NSOffState];
		[ExamineMode setState: NSOnState];
		[FlyMode setState: NSOffState];	
	} else if (type == FLY) {
		[WalkMode setState: NSOffState];
		[ExamineMode setState: NSOffState];
		[FlyMode setState: NSOnState];	
	} else if (type == WALK) {
		[WalkMode setState: NSOnState];
		[ExamineMode setState: NSOffState];
		[FlyMode setState: NSOffState];	
	}
}

- (IBAction) FirstView: (id) sender {
	fwl_First_ViewPoint();
}

- (IBAction) LastView: (id) sender {
	fwl_Last_ViewPoint();
}

- (IBAction) NextView: (id) sender {
	ks = 'v';
	fwl_do_keyPress(ks, KeyPress);
}

- (IBAction) PrevView: (id) sender {
	fwl_Prev_ViewPoint();
}

- (IBAction) WalkMd: (id) sender {
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];
	} else {
		// Do nothing you can't "unselect" a view mode
		[sender setState: NSOnState];
	}
	[FlyMode setState: NSOffState];
	[ExamineMode setState: NSOffState];
	ks = 'w';
	fwl_do_keyPress(ks, KeyPress);
}

- (void) setMessage: (NSString*) message {
	//NSRange range;
	NSRect wFrame;
	NSPoint newPoint;
	wFrame = [freewrlWindow frame];
	//NSLog(@"Got origin %f %f size %f %f\n", wFrame.origin.x, wFrame.origin.y, wFrame.size.width, wFrame.size.height);
	newPoint.x = wFrame.origin.x;
	newPoint.y = wFrame.origin.y - 32;
	
	//NSLog(@"Should be setting point to %f %f\n", newPoint.x, newPoint.y);
	//NSLog(@"recieved:%s:", [message cString]);
	
	[cWindow setFrameTopLeftPoint: newPoint];

		[cWindow orderFront: nil];
	NSTextStorage *textStorage;
        NSAttributedString *amessage = [[NSAttributedString new] initWithString: message];
        textStorage = [cText textStorage];
        [textStorage beginEditing];
        [textStorage appendAttributedString:amessage];
        [textStorage endEditing];
	//range = NSMakeRange([[cText string] length], 0);
	//[cText replaceCharactersInRange: range withString: message];
	//range = NSMakeRange([[cText string] length], 0);
	//[cText replaceCharactersInRange: range withString: @"\n"];

}

- (void) openConsole {
	//if ([ConsoleDrawer state] == NSDrawerClosedState) {
	//	[ConsoleMI setState: NSOnState];
	//	[ConsoleDrawer open];
	//}
	[self ConDisp: ConsoleMI];
}

- (void) setStatusMessage: (NSString*) str {
	//printf("IN SET STATUS");
	[MessageLine setStringValue: str];
}

- (IBAction) Reload: (id) sender
{
	NSString *aFile = [freewrl getFileName];
	[aFile getCString: file_name maxLength: 2048 encoding: NSUTF8StringEncoding];
	//[wc hideWindow];
	fwl_replaceWorldNeeded(file_name);
	[wc showWindow];
}

- (IBAction) showAbout: (id) sender
{
	[NSApp orderFrontStandardAboutPanel: nil];
}

- (IBAction) setScreen: (id) sender
{
	switch ([sender tag]) {
		case 0:
			if ([sender state] == NSOnState) {
				[sender setState: NSOffState];
			} else {
				[sender setState: NSOnState];
				[sender setHeight: 480 width: 640];
			}
			break;
		case 1:
			if ([sender state] == NSOnState) {
				[sender setState: NSOffState];
			} else {
				[sender setState: NSOnState];
				[sender setHeight: 300 width: 400];
			}
			break;
		case 2:
			if ([sender state] == NSOnState) {
				[sender setState: NSOffState];
			} else {
				[sender setState: NSOnState];
				[sender setHeight: 200 width: 300];
			}
			break;
	}
}

- (void) checkRunning
{
}

- (IBAction) openFile: (id) sender
{
    int result;
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];
	[self debugPrint: "in openFile"];
    [oPanel setAllowsMultipleSelection:NO];
    result = [oPanel runModalForDirectory:NSHomeDirectory()
                    file:nil types:nil];
    if (result == NSOKButton) {
			NSArray *filesToOpen = [oPanel filenames];
			NSString *aFile = [filesToOpen objectAtIndex:0];
			[[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL: [NSURL fileURLWithPath: aFile]];
			[self debugPrint: "setFile 2"];
			[freewrl setFile: aFile];
			[aFile getCString: buff];
			//NSLog(@"Calling anchor_replaceworld1 ... ");
		if ([freewrl hasDoneInit]) {
				fwl_replaceWorldNeeded(buff);
		}
			[wc showWindow];
    }
}

- (IBAction) printFile: (id) sender
{
	//NSLog(@"Got to print file");
}
- (BOOL) application: (NSApplication*) app openFile: (NSString *) fileName
{
	[self debugPrint: "in app open file"];
	if (![freewrl getOptions]) {
		[[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL: [NSURL fileURLWithPath: fileName]];
		[self debugPrint: "setFile1"];
		[freewrl setFile: fileName];
		[fileName getCString: buff];
		if([freewrl hasDoneInit])
			replaceWorldNeeded(buff);
		[wc showWindow];
	}
	return TRUE;
}

- (void) setEAIFlag: (int) flag
{	
	if (flag) {
		[EAIMenuItem setState: NSOnState];
	} else {
		[EAIMenuItem setState: NSOffState];
	}
}

- (void) setSeqFlag: (int) flag
{
	if (flag) {
		[SeqMenuItem setState: NSOnState];
	} else {
		[SeqMenuItem setState: NSOffState];
	}
}

- (IBAction) saveSeq: (id) sender
{
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];
		//[freewrl stopEai];
	} else {
		[sender setState: NSOnState];
		//[freewrl fwl_init_SnapSeq];
	}
}

- (IBAction) closeFile: (id) sender
{
	[wc hideWindow];
}

- (IBAction) MesgDisp: (id) sender {
	[MessageDrawer setContentSize: NSMakeSize(22, 22)];
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];
		[MessageDrawer close: sender];
	} else {
		[sender setState: NSOnState];
		[MessageDrawer open: sender];
	} 
}

- (IBAction) ConDisp: (id) sender {
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];
		[ConsoleDrawer close: sender];
	} else {
		[sender setState: NSOnState];
		[ConsoleDrawer open: sender];
	}
}

- (IBAction) bigTextures: (id) sender {
	// JAS setTexSize(0);
	[bTex setState: NSOnState];
	[sTex setState: NSOffState];
	[mTex setState: NSOffState];
}

- (IBAction) mediumTextures: (id) sender {
	// JAS setTexSize(-512);
	[bTex setState: NSOffState];
	[sTex setState: NSOffState];
	[mTex setState: NSOnState];
}

- (IBAction) smallTextures: (id) sender {
	// JAS setTexSize(-256);
	[bTex setState: NSOffState];
	[sTex setState: NSOnState];
	[mTex setState: NSOffState];	
}

- (IBAction) shapeThread: (id) sender {
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];
		// obsolete - JAS setUseShapeThreadIfPossible(0);
	} else if ([sender state] == NSOffState) {
		[sender setState: NSOnState];
		// obsolete - JAS setUseShapeThreadIfPossible(1);
	}
}

- (void) changeColor: (id) sender {
	NSColor* theColour = NULL;
#ifdef USE_DOUBLE
	CGFloat myred, mygreen, myblue, myalpha;
#else
	float myred, mygreen, myblue, myalpha;
#endif
	
	theColour = [sender color];
	[theColour getRed: &myred green: &mygreen blue: &myblue alpha: &myalpha];
	mycolour[0] = (float) myred;
	mycolour[1] = (float) mygreen;
	mycolour[2] = (float) myblue;
	mycolour[3] = (float) myalpha;
	setglClearColor(mycolour);
}

- (IBAction) prioritizeTextures: (id) sender {
	if ([sender state] == NSOnState) {
		[sender setState: NSOffState];
		setTextures_take_priority(0);
	} else if ([sender state] == NSOffState) {
		[sender setState: NSOnState];
		setTextures_take_priority(1);
	}
}



- (void) setTexSize: (int) size {
	if (size <= 256) {
		[bTex setState: NSOffState];
		[sTex setState: NSOnState];
		[mTex setState: NSOffState];		
	} else if (size <= 512) {
		[bTex setState: NSOffState];
		[sTex setState: NSOffState];
		[mTex setState: NSOnState];
	} else {
		[bTex setState: NSOnState];
		[sTex setState: NSOffState];
		[mTex setState: NSOffState];
	}
}

- (void) setOpenFlag
{
}

@end
