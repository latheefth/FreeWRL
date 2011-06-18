//
//  RecentItems.h
//  FreeWRL
// 
// stores and retrieves "recently used" URLs.
//
//  Created by John Stewart on 11-06-16.
//  Copyright 2011 CRC Canada. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface RecentItems : NSObject {
@private
    
}

+ (NSString *)getRecent1;
+ (NSString *)getRecent2;
+ (void) setMostRecent: (NSString *)Rec1;

@end
