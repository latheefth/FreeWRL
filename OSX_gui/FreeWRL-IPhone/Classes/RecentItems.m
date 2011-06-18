//
//  RecentItems.m
//  FreeWRL
//
// 
// stores and retrieves "recently used" URLs.
//
//  Created by John Stewart on 11-06-16.
//  Copyright 2011 CRC Canada. All rights reserved.
//

#import "RecentItems.h"

NSString *Recent1 = nil;
NSString *Recent2 = nil;
NSFileHandle *myFile = nil;


@implementation RecentItems

- (id)init
{
    //NSLog (@"RecentItems init");
    self = [super init];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}

// make up the paths here
+ (NSString *) pathForDataFile
{
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *folder = [paths objectAtIndex:0];
    //NSLog(folder);  // output documents directory to debug console
    
    // get the current directory, as an absolute file name; create it if required
    folder = [folder stringByExpandingTildeInPath];
    if ([fileManager fileExistsAtPath: folder] == NO) {
        [fileManager createDirectoryAtPath: folder withIntermediateDirectories: YES attributes: nil error: nil];
    }
    
    return folder;
}

// read in the last recently used correct URLS. If no file, return pre-made strings.
+(void) initializeRecentStrings
{
    NSString* path = [[self pathForDataFile] stringByAppendingString:@"/FreeWRL_Recent_URLs"];
    //NSLog (@"opening file %@",path);
    
    
    NSError *error;
    NSString *stringFromFileAtPath = [NSString
                                      stringWithContentsOfFile: path
                                      encoding: NSUTF8StringEncoding
                                      error:&error];
    if (stringFromFileAtPath == nil) {
        NSLog(@"Error reading file at %@\n%@",
              path, [error localizedFailureReason]);
    } 
    //NSLog(@"Contents:%@", stringFromFileAtPath);
    
    // ok, the string will be one large string. Split it up around newline characters,
    // because thats how we wrote it.
    NSArray *chunks = [stringFromFileAtPath componentsSeparatedByString: @"\n"];
    
    // get the recent strings out of the list
    //NSLog (@" we have %d chunks",[chunks count]);
    
    if ([chunks count] > 0) {
        Recent1 = [chunks objectAtIndex:0];
    } else {
        Recent1 = [NSString stringWithCString:"http://freewrl.sf.net/test.wrl" encoding:NSUTF8StringEncoding];
    }
    
    NSLog (@"setting Recent1 to test directly");
    Recent1 = [NSString stringWithCString:"http://freewrl.sf.net/tests/53.wrl" encoding:NSUTF8StringEncoding];
    if ([chunks count] > 1) {
        Recent2 = [chunks objectAtIndex:1];
    } else {
        Recent2 = [NSString stringWithCString:"http://freewrl.sf.net/test2.wrl" encoding:NSUTF8StringEncoding];
    }
}


+ (NSString *)getRecent1
{
    if (Recent1 == nil) {
        [self initializeRecentStrings];
    }
    return Recent1;
}

+ (NSString *)getRecent2
{
    if (Recent2 == nil) {
        [self initializeRecentStrings];
    }
    return Recent2;
}

// write the most recent valid urls to our file.
+ (void) setMostRecent: (NSString *)Rec1 {

    BOOL bRes = NO;
    NSString* path = [[self pathForDataFile] stringByAppendingString:@"/FreeWRL_Recent_URLs"];
    //NSLog (@"opening file %@",path);
    
    // Is this a new file?
    if ([Rec1 isEqualToString:Recent1] || 
        [Rec1 isEqualToString:Recent2]) {
            //NSLog (@"setMostRecent, strings equal, not bothering to write");
            return;
        }
    
    //Move these down the stack
    Recent2 = Recent1;
    Recent1 = Rec1;
    
    if ( (bRes=[[NSFileManager defaultManager] createFileAtPath:path
                                                       contents:nil attributes:nil]) == YES) {
        //NSLog(@"createFileAtPath ok");
    }else {
        NSLog(@"createFileAtPath not ok");
        return;
    }
    
    myFile = [NSFileHandle fileHandleForWritingAtPath: path];
    if (myFile == nil) {
        NSLog(@"writing to file for logging failed");
    } else {
        [myFile truncateFileAtOffset: 0]; // ensure it is at the beginning
        [myFile writeData: [Recent1 dataUsingEncoding:NSUTF8StringEncoding]];
        [myFile writeData: [@"\n" dataUsingEncoding:NSUTF8StringEncoding]];
        [myFile writeData: [Recent2 dataUsingEncoding:NSUTF8StringEncoding]];
        [myFile closeFile];
    }



}

- (void)dealloc
{
    [super dealloc];
}

@end
