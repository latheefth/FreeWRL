//
//  MyWindowController.m
//  FreeWRL
//
//  Created by Sarah Dumoulin on Fri Jan 30 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import "MyWindowController.h"
#import <OpenGL/gl.h>
#import <OpenGL/Opengl.h>
#import <CGLTypes.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreFoundation/CFNumber.h>
#import <ApplicationServices/ApplicationServices.h>

@implementation MyWindowController

- (void) debugPrint: (char *) theString
{
	
//	if (theFile == NULL) {
	//	NSLog(@"REOPENING");
//		theFile = fopen("/Users/sarah/win_log", "w");
//		if (theFile == NULL)
//			abort();
//		fileIsOpen = TRUE;
//	}
//
//	fprintf(theFile, "%s\n", theString);
//	fflush(theFile);
}

- (NSOpenGLContext*) getContext {
	return context;
}
- (id) init 
{
	return [self initWithFreeWRL: nil];
}
- (id) initWithFreeWRL: (NSView*) view
{
	if (self = [super init]) {
		NSOpenGLPixelFormat* fmt;

		
		theView = (FreeWRLView*) view;
		Boolean fullscreen = [theView getFullscreen];
		myController = [[[NSWindowController alloc] initWithWindowNibName:@"MainWindow" owner:self] retain];
		//[myController window];
		if (!fullscreen) {
	
			GLuint attribs[] = 
			{
			NSOpenGLPFANoRecovery,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAWindow,
			NSOpenGLPFAAccelerated,
			NSOpenGLPFAColorSize, 24,
			NSOpenGLPFAAlphaSize, 8,
			NSOpenGLPFADepthSize, 24,
			NSOpenGLPFAStencilSize, 8,
			NSOpenGLPFAAccumSize, 0,
			0
			};	
		
			fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: (NSOpenGLPixelFormatAttribute*) attribs]; 
			context = [[[NSOpenGLContext alloc] initWithFormat: fmt shareContext: nil] retain];
			[fmt release];
			fmt = nil;
			[context makeCurrentContext];

			// JAS - will this fix vertical tearing?
			CGLContextObj cglContext;
			long newSwapInterval;
			cglContext = CGLGetCurrentContext();
			newSwapInterval = 1;
			CGLSetParameter(cglContext, kCGLCPSwapInterval, &newSwapInterval);
		
			[context setView: theView];
		} else {
			GLuint attribs[] = 
			{
			// NSOpenGLPFANoRecovery,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAFullScreen,
			NSOpenGLPFAScreenMask, CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay),
			NSOpenGLPFAAccelerated,
			NSOpenGLPFAColorSize, 24,
			// NSOpenGLPFAAlphaSize, 8,
			NSOpenGLPFADepthSize, 16,
			// NSOpenGLPFAStencilSize, 8,
			// NSOpenGLPFAAccumSize, 0,
			0
			};
			fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: (NSOpenGLPixelFormatAttribute*) attribs]; 
			context = [[[NSOpenGLContext alloc] initWithFormat: fmt shareContext: nil] retain];
			[fmt release];
			fmt = nil;
			if (context == nil) {
				NSLog(@"Failed to create fullscreen context");
				return nil;
			}
			
			CGDisplayErr err = CGCaptureAllDisplays();
			if (err != CGDisplayNoErr) {
				[context release];
				context = nil;
				return nil;
			}
			
			[context setFullScreen];
			[context makeCurrentContext];
			
			CGLContextObj cglContext;
			long newSwapInterval;
			cglContext = CGLGetCurrentContext();
			newSwapInterval = 1;
			CGLSetParameter(cglContext, kCGLCPSwapInterval, &newSwapInterval);
		

			[theView setHeight: CGDisplayPixelsHigh(kCGDirectMainDisplay) width: CGDisplayPixelsWide(kCGDirectMainDisplay)];
			
			long swapInterval;
			CGLGetParameter(cglContext, kCGLCPSwapInterval, &swapInterval);
			NSLog(@"Swap interval is now %d\n", swapInterval);
			
			CFNumberRef number; 
			// CFBoolean booleanValue; 
			//Boolean gui;
			long mode, refresh, ioflags; 
 
			CFDictionaryRef currentMode = CGDisplayCurrentMode (kCGDirectMainDisplay);
 
			number = CFDictionaryGetValue (currentMode, kCGDisplayMode); 
			CFNumberGetValue (number, kCFNumberLongType, &mode); 
 
			number = CFDictionaryGetValue (currentMode, kCGDisplayRefreshRate); 
			CFNumberGetValue (number, kCFNumberLongType, &refresh); 
 
			//booleanValue = CFDictionaryGetValue (currentMode, kCGDisplayModeUsableForDesktopGUI); 
			//gui = CFBooleanGetValue (booleanValue); 
 
			number = CFDictionaryGetValue (currentMode, kCGDisplayIOFlags); 
			CFNumberGetValue (number, kCFNumberLongType, &ioflags); 
			//NSLog(@"got mode %ld, refresh %ld, ioflags %ld, gui %d\n", mode, refresh, ioflags, gui);
		}
		char buff[128];
		sprintf(buff, "Before set view view is %p\n", theView);
		[self debugPrint: buff];
		[theView setController: self];

	}
	return self;
}

- (void) initScreen
{
	[self debugPrint: "in wc init screen"];
	[theView initScreen];
}

- (id) getView 
{
	return theView;
}

- (void) setWindow: (NSWindow*) vwindow
{
	viewWindow = vwindow;
}

- (void) hideWindow
{
	[viewWindow orderOut: nil];
}

- (void) showWindow
{
	[viewWindow makeKeyAndOrderFront: nil];
}
@end
