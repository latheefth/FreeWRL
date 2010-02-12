//
//  Plugin.h
//  WebKitMoviePlugIn
//
//  Created by Sarah Dumoulin on 10-01-27.
//  Copyright 2010 Communications Research Centre. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import "X3DView.h"


/* our main plug-in class definition. */
@interface Plugin : X3DView <WebPlugInViewFactory> {
	NSDictionary *pluginArguments;
}

/* WebPlugInViewFactory message handler for vending plug-in instances */
+ (NSView *)plugInViewWithArguments:(NSDictionary *)arguments;

/* storage management message handlers */
-(id) initWithArguments:(NSDictionary *)arguments;
- (void) dealloc;

/* accessors for pluginArguments */
- (NSDictionary *)pluginArguments;
- (void)setPluginArguments:(NSDictionary *)value;

@end
