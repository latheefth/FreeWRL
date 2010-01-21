//
//  CustomClass.m
//  FreeWRL
//
//  Created by Sarah Dumoulin on Tue Feb 04 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import "CustomClass.h"
#include <OpenGL/gl.h>

@implementation CustomClass
- (NSOpenGLPixelFormat*) getFormat
{
 	GLuint attribs[] = 
	{
		NSOpenGLPFANoRecovery,
		NSOpenGLPFAWindow,
                //NSOpenGLPFAFullScreen,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFAAccumSize, 0,
		0
	};

	NSOpenGLPixelFormat* fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: (NSOpenGLPixelFormatAttribute*) attribs]; 
        return fmt;
}
- (void) setView: (NSView*) view
{
    myView = view;
    [view retain];
}
    
- (float) getHeight 
{
    NSRect rect;
    rect = [myView frame];
    return rect.size.width;
}
    
- (float) getWidth
{
    NSRect rect;
    rect = [myView frame];
    return rect.size.height;
}
@end
