//
//  FreeWRLAppDelegate.m
//  FreeWRL
//
//  Created by John Stewart on 11-02-27.
//  Copyright CRC Canada 2011. All rights reserved.
//

#import "FreeWRLAppDelegate.h"
#import "GLView.h"

@implementation FreeWRLAppDelegate

@synthesize window;
@synthesize glView;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
    
    // change the statusbar
    //[application setStatusBarStyle:UIStatusBarStyleBlackOpaque];
    [application setStatusBarHidden:YES];
    
	glView.animationInterval = 1.0 / kRenderingFrequency;
	[glView startAnimation];
}


- (void)applicationWillResignActive:(UIApplication *)application {
	glView.animationInterval = 1.0 / kInactiveRenderingFrequency;
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
	glView.animationInterval = 1.0 / 60.0;
}


- (void)dealloc {
	[window release];
	[glView release];
	[super dealloc];
}

@end
