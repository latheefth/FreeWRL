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
NSOperationQueue * _queue = nil;
bool *opFlagPtr;
static bool appRunningNow = false;


@implementation FreeWRLAppDelegate

-(void)addOperation:(UrlDownloader *)operation {
    //NSLog (@"Delegate: addOperation");
    
    [_queue addOperation:operation];
}

+(bool)applicationHasLaunched
{
    return appRunningNow;
}


- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    //NSLog (@"Delegate: applicationDidFinishLaunching queue %p",_queue);

        _queue = [[NSOperationQueue alloc] init];
    // Set to 1 to serialize operations. Comment out for parallel operations.
    // [_queue setMaxConcurrentOperationCount:1];
    
    
    [_queue addObserver:self
             forKeyPath:@"operations"
                options:0
                context:&OperationsChangedContext];
    appRunningNow = true;
    
}

- (void)dealloc
{
    //NSLog (@"Delegate: dealloc");
    [_queue removeObserver:self forKeyPath:@"operations"];
    [_queue release];
    [super dealloc];
}



+(void)newDoURL:(NSString *)url opFlag:(bool *)opFlag
{
   //NSLog (@"Delegate: starting newDoURL queue %p",_queue);
        
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
    //NSLog (@"Delegate: observeValueForKeyPath");
    if (context == &OperationsChangedContext)
    {
        //NSLog(@"Delegate: Queue size: %u", [[_queue operations] count]);
    }
    else
    {
        //NSLog (@"Delegate: observeValueForKeyPath - having to super");
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
