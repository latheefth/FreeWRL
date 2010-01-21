//
//  CustomClass.h
//  FreeWRL
//
//  Created by Sarah Dumoulin on Tue Feb 04 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>


@interface CustomClass : NSObject {
    NSView* myView;
}
- (NSOpenGLPixelFormat*) getFormat;
- (float) getHeight;
- (float) getWidth;
- (void) setView: (NSView *) view; 

@end
