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
	//NSLog([NSString stringWithString:@"hi1 from FreeWRL main.m"]);
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	//NSLog([NSString stringWithString:@"hi2 from FreeWRL main.m"]);
	MyApp* theApp = [[MyApp alloc] init];
	//NSLog([NSString stringWithString:@"hi3 from FreeWRL main.m"]);

	[theApp retain];
	//NSLog([NSString stringWithString:@"hi4 from FreeWRL main.m"]);

	[NSApplication sharedApplication];
	//NSLog([NSString stringWithString:@"hi5 from FreeWRL main.m"]);


	if ([NSBundle loadNibNamed:@"MainMenu" owner:NSApp]) {
		//NSLog([NSString stringWithString:@"hi6 from FreeWRL main.m"]);

		[NSApp setDelegate: theApp];
		//NSLog([NSString stringWithString:@"hi7 from FreeWRL main.m"]);

		[pool release];
		//NSLog([NSString stringWithString:@"hi8 from FreeWRL main.m"]);

		[NSApp run];
	}
	//NSLog([NSString stringWithString:@"Going to run NSApplicationMain in FreeWRL main.m"]);
	return NSApplicationMain(argc, argv);
}
