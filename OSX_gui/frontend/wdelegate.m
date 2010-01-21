//
//  wdelegate.m
//  FreeWRL
//
//  Created by Sarah Dumoulin on Tue Apr 06 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import "wdelegate.h"
#import "FreeWRLView.h"

@implementation wdelegate
- (void) windowWillClose: (NSNotification*) aNotification
{
	doQuit();
	[NSApp terminate: self];
}

- (void) setView: (id) view
{
	freewrl = view;
}

- (void) windowDidResize: (NSNotification *) aNotification
{
	[freewrl doResize];

}

- (void) setArrow
{
	[freewrl addCursorRect:[freewrl visibleRect] cursor:[NSCursor arrowCursor]];
}

- (void) setCross
{
	NSLog(@"In set cross");
	[freewrl addCursorRect:[freewrl visibleRect] cursor:[NSCursor pointingHandCursor]];
}


@end
