//
//  FreeWRLAppDelegate.h
//  FreeWRL
//
//  Created by John Stewart on 11-07-20.
//  Copyright 2011 CRC Canada. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "UrlDownloader.h"


@interface FreeWRLAppDelegate : NSObject
{
}


- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context;

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication;
+(void) newDoURL:(NSString *)url 
          opFlag:(bool *)opFlag;

-(void)dealloc;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;


-(void)addOperation:(UrlDownloader *)operation;

@end



//@interface FreeWRLAppDelegate : NSObject <NSApplicationDelegate> {
//@private
//    NSWindow *window;
//}

//@property (assign) IBOutlet NSWindow *window;
//
//@end
