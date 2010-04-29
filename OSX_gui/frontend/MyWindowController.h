//
//  MyWindowController.h
//  FreeWRL
//
//  Created by Sarah Dumoulin on Fri Jan 30 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include </FreeWRL/freewrl/freex3d/src/lib/libFreeWRL.h>
#import "FreeWRLView.h"


@interface MyWindowController : NSObject {
	FILE* theFile;
	bool fileIsOpen;
	NSWindowController* myController;
	id theView;
	NSWindow* viewWindow;
	NSOpenGLContext* context;
}
- (id) init;
- (id) initWithFreeWRL: (NSView*) mview;
- (void) initScreen;
- (id) getView;
- (void) setWindow: (NSWindow*) vwindow;
- (void) hideWindow;
- (void) showWindow;

@end
