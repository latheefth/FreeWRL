//
//  Plugin.m
//  WebKitMoviePlugIn
//
//  Created by Sarah Dumoulin on 10-01-27.
//  Copyright 2010 Communications Research Centre. All rights reserved.
//
#import "Plugin.h"

@implementation Plugin

/* initializer for our plug-in */
-(id) initWithArguments:(NSDictionary *)arguments {
	//NSLog(@"init with arguments %@", arguments);
	if ((self = [super init]) != nil) {
		[self setPluginArguments: arguments];
	}
	return self;
}


/* deallocation for our plug-in */
- (void) dealloc {
	//[self setPluginArguments: nil];
	[super dealloc];
}


/* WebPlugInViewFactory message handler for vending plug-in instances */
+ (NSView *)plugInViewWithArguments:(NSDictionary *)arguments {
    Plugin *thePlugin;
	thePlugin = [[[self alloc] initWithArguments:arguments] autorelease];
    return thePlugin;
}


/* accessors for pluginArguments */
- (NSDictionary *)pluginArguments {
    return [[pluginArguments retain] autorelease];
}

- (void)setPluginArguments:(NSDictionary *)value {
	//NSLog(@"setPluginArguments");
    if (pluginArguments != value) {
        [pluginArguments release];
        pluginArguments = [value copy];
		[self setArguments:pluginArguments];
    }
}

@end

