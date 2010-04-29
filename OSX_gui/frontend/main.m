//
//  main.m
//  FreeWRL
//
//  Created by Sarah Dumoulin on Thu Jan 15 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MyApp.h"

int main(int argc, const char *argv[])
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	MyApp* theApp = [[MyApp alloc] init];

	[theApp retain];

	[NSApplication sharedApplication];


	if ([NSBundle loadNibNamed:@"MainMenu" owner:NSApp]) {

		[NSApp setDelegate: theApp];

		[pool release];

		[NSApp run];
	}
	return NSApplicationMain(argc, argv);
}
