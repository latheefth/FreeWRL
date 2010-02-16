//
//  MyApp.h
//  FreeWRL
//
//  Created by Sarah Dumoulin on Fri Jan 30 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <ApplicationServices/ApplicationServices.h>
#import "MyWindowController.h"

@interface MyApp : NSDocumentController {
	FILE* theFile;
	bool fileIsOpen;
	id wc;
	char buff[128];
	char file_name[2048];
	char ks;
	id freewrl;
	float mycolour[3];
	NSColorPanel* colourPanel;
	IBOutlet NSWindow* freewrlWindow;
	IBOutlet id EAIMenuItem;
	IBOutlet id SeqMenuItem;
	IBOutlet NSTextView* ConsoleWindow;
	IBOutlet NSTextField* MessageLine;
	IBOutlet NSWindow* cWindow;
	IBOutlet NSTextView* cText;
	IBOutlet id Collision;
	IBOutlet id theView;
	IBOutlet id ConsoleDisp;
	IBOutlet id ExamineMode;
	IBOutlet id FlyMode;
	IBOutlet id Headlight;
	IBOutlet id MessageDisp;
	IBOutlet id WalkMode;
	IBOutlet id ConsoleDrawer;
	IBOutlet id ConsoleMI;
	IBOutlet id MessageDrawer;
	IBOutlet id sTex;
	IBOutlet id mTex;
	IBOutlet id bTex;
	IBOutlet id shapeThreadb;
	IBOutlet id texPriority;
	IBOutlet id expParserb;
}
 
- (void) applicationWillFinishLaunching:(NSNotification*)aNotification;
- (void) applicationDidFinishLaunching:(NSNotification*)aNotification;
- (void) setEAIFlag: (int) flag;
- (void) setSeqFlag: (int) flag;
- (IBAction) setScreen: (id) sender;
- (IBAction) openFile: (id) sender;
- (IBAction) closeFile: (id) sender;
- (IBAction) printFile: (id) sender;
//- (IBAction) dismiss: (id) sender;
- (IBAction) showAbout: (id) sender;
- (IBAction) Coll: (id) sender;
- (IBAction) ConDisp: (id) sender;
- (IBAction) MesgDisp: (id) sender;
- (IBAction) ExMd: (id) sender;
- (IBAction) FlyMd: (id) sender;
- (IBAction) HeadL: (id) sender;
- (IBAction) NextView: (id) sender;
- (IBAction) PrevView: (id) sender;
- (IBAction) WalkMd: (id) sender;
- (IBAction) FirstView: (id) sender;
- (IBAction) LastView: (id) sender;
- (IBAction) Reload: (id) sender;
- (IBAction) smallTextures: (id) sender;
- (IBAction) mediumTextures: (id) sender;
- (IBAction) bigTextures: (id) sender;
- (IBAction) prioritizeTextures: (id) sender;
- (IBAction) setStereo: (id) sender;

- (void) toggleHLButton: (int) val;
- (void) toggleCollButton: (int) val;
- (void) toggleNavButton: (int) type;
- (void) setMessage: (NSString*) str;
- (void) setStatusMessage: (NSString*) str;

@end
