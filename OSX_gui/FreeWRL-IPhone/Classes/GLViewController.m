//
//  GLViewController.m
//  FW_testing
//
//  Created by John Stewart on 11-02-15.
//  Copyright CRC Canada 2011. All rights reserved.
//

#import "GLViewController.h"
#import "ConstantsAndMacros.h"

@interface MyObject : NSObject
+(void)aMethod:(id)param;
@end

@implementation MyObject
+(void)aMethod:(id)param {
    //NSLog (@"starting loading thread");
    
    initializeRenderSceneUpdateScene();
    
    //OSX_initializeParameters("http://freewrl.sourceforge.net/test2pt.wrl");
    OSX_initializeParameters("http://freewrl.sourceforge.net/test2.wrl");
    
    // simple cone OSX_initializeParameters("http://freewrl.sourceforge.net/test.wrl");

    
    //NSLog (@"ending loading thread");
}
@end


@implementation GLViewController

bool frontEndGettingFile = false;
NSMutableData *receivedData;

-(void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    // This method is called when the server has determined that it
    // has enough information to create the NSURLResponse.
    
    // It can be called multiple times, for example in the case of a
    // redirect, so each time we reset the data.
    
    // receivedData is an instance variable declared elsewhere.
    //NSLog (@"connection: didReceiveResponse called");
    [receivedData setLength:0];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    //NSLog (@"connection: didReceiveData called");
    // Append the new data to receivedData.
    // receivedData is an instance variable declared elsewhere.
    [receivedData appendData:data];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    // release the connection, and the data object
    [connection release];
    // receivedData is declared as a method instance elsewhere
    [receivedData release];
    
    // inform the user
    NSLog(@"Connection failed! Error - %@ %@",
          [error localizedDescription],
          [[error userInfo] objectForKey:NSURLErrorFailingURLStringErrorKey]);
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    int len;
    unsigned char *myData;
    
    // do something with the data
    // receivedData is declared as a method instance elsewhere
    NSLog(@"At connection: didFinishLoading  Received %d bytes of data",[receivedData length]);
     
    len = [receivedData length];
    myData = malloc (len);
    [receivedData getBytes:myData];
    
    //NSString *s = [[NSString alloc] initWithData:receivedData encoding:NSASCIIStringEncoding];
    //NSLog(@"at A: string is: %@", s);
    //const char* cString = [s UTF8String]; 
    //[s release];
    
    frontEndReturningData(myData,len);

    free (myData);
    frontEndGettingFile = false;


    // release the connection, and the data object
    [connection release];
    [receivedData release];
}




-(void)setupView:(GLView*)view
{
	//NSLog (@"setupView");
	CGRect rect = view.bounds;
      //NSLog (@"initialize - size %d %d",(int)rect.size.width, (int)rect.size.height);
    
	setScreenDim((int)rect.size.width, (int)rect.size.height);
	display_initialize();

    // thread the getting of the file...
    [NSThread detachNewThreadSelector:@selector(aMethod:) toTarget:[MyObject class] withObject:nil];
    
    
	// NSLog (@"setupView complete");
}

- (void)drawView:(GLView*)view;
{
    if (frontEndWantsFileName() != nil) {
        if (!frontEndGettingFile) {
            frontEndGettingFile = true;
        
        // Create the request. Get the file name from the FreeWRL library.
        NSString *myString = [[NSString alloc] initWithUTF8String:frontEndWantsFileName()];
        NSURLRequest *theRequest=
            [NSURLRequest requestWithURL:[NSURL URLWithString: myString ]
                             cachePolicy:NSURLRequestUseProtocolCachePolicy
                         timeoutInterval:60.0];

            
            // create the connection with the request
        // and start loading the data
        NSURLConnection *theConnection=[[NSURLConnection alloc] initWithRequest:theRequest delegate:self];

        if (theConnection) {
            // Create the NSMutableData to hold the received data.
            // receivedData is an instance variable declared elsewhere.
            receivedData = [[NSMutableData data] retain];
         
        } else {
            // Inform the user that the connection failed.
        }
        }
    }

    RenderSceneUpdateScene();
	
}

- (void)dealloc {
	// JASfinalizeRenderSceneUpdateScene();
    [super dealloc];
}
@end

