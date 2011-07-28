//
//  FreeWRLAppDelegate.m
//  FreeWRL
//
//  Created by John Stewart on 11-07-20.
//  Copyright 2011 CRC Canada. All rights reserved.
//

#import "FreeWRLAppDelegate.h"
#import "UrlDownloader.h"

static NSString * OperationsChangedContext = @"OperationsChangedContext";
NSOperationQueue * _queue;
bool *opFlagPtr;


@implementation FreeWRLAppDelegate

-(void)addOperation:(UrlDownloader *)operation {
    
    [_queue addOperation:operation];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {

    _queue = [[NSOperationQueue alloc] init];
    // Set to 1 to serialize operations. Comment out for parallel operations.
    // [_queue setMaxConcurrentOperationCount:1];
    
    [_queue addObserver:self
             forKeyPath:@"operations"
                options:0
                context:&OperationsChangedContext];
}

- (void)dealloc
{
    [_queue removeObserver:self forKeyPath:@"operations"];
    [_queue release];
    [super dealloc];
}



+(void)newDoURL:(NSString *)url opFlag:(bool *)opFlag
{
    //NSLog (@"starting newDoURL");
    opFlagPtr = opFlag;
    
    
    UrlDownloader * operation =
    [UrlDownloader urlDownloaderWithUrlString:url opFlag:opFlag];
    [_queue addOperation:operation];
    
}


- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
    if (context == &OperationsChangedContext)
    {
        //NSLog(@"Queue size: %u", [[_queue operations] count]);
    }
    else
    {
        [super observeValueForKeyPath:keyPath
                             ofObject:object
                               change:change
                              context:context];
    }
}


- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
	return YES;
}

@end
