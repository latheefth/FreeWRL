//
//  FreeWRLAppDelegate.h
//  FreeWRL
//
//  Created by John Stewart on 11-02-27.
//  Copyright CRC Canada 2011. All rights reserved.
//

#import <UIKit/UIKit.h>

@class GLView;

@interface FreeWRLAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    GLView *glView;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet GLView *glView;

@end

