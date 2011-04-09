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
    
    fwl_initializeRenderSceneUpdateScene();
    
    //fwl_OSX_initializeParameters("http://freewrl.sourceforge.net/test2pt.wrl");
   // fwl_OSX_initializeParameters( //"http://freewrl.sourceforge.net/JAS/SSID-Mar2011/staticCount500.x3d");
                             //"http://freewrl.sourceforge.net/test2.wrl");
    
    //fwl_OSX_initializeParameters("http://kins.net/smar/files/ryatkins/aeroboat.wrl");
	fwl_OSX_initializeParameters("http://freewrl.sourceforge.net/test3.wrl");
    //fwl_OSX_initializeParameters("http://dl.dropbox.com/u/17457/aeroboat.wrl");
    //NSLog (@"ending loading thread");
	
}
@end


@implementation GLViewController

bool frontEndGettingFile = false;
NSMutableData *receivedData;


/* from the headers.h file - for now */
#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#define MapNotify       19
/* .... */


#pragma mark -
#pragma mark View lifecycle

- (void)viewDidLoad {
    [super viewDidLoad];
	
   	UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapFrom:)];
    [tapGesture setDelegate:self];
	[self.view addGestureRecognizer:tapGesture];
	[tapGesture release];
        
    UIPinchGestureRecognizer *pinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinchFrom:)];
    [pinchGesture setDelegate:self];
    [self.view addGestureRecognizer:pinchGesture];
    [pinchGesture release];

    UIPanGestureRecognizer *panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanFrom:)];
    [panGesture setMaximumNumberOfTouches:1];
    //[panGesture setDelegate:self];
    [self.view addGestureRecognizer:panGesture];
    [panGesture release];      
    
    UIRotationGestureRecognizer *rotationGesture = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(handleRotationFrom:)];
    [self.view addGestureRecognizer:rotationGesture];
    [rotationGesture release];
    
}


-(void)setupView:(GLView*)view
{
	//NSLog (@"setupView");
	CGRect rect = view.bounds;
    //NSLog (@"initialize - size %d %d",(int)rect.size.width, (int)rect.size.height);
    
	fwl_setScreenDim((int)rect.size.width, (int)rect.size.height);
	fwl_display_initialize();
	
        // thread the getting of the file...
    [NSThread detachNewThreadSelector:@selector(aMethod:) toTarget:[MyObject class] withObject:nil];
    
    
	// NSLog (@"setupView complete");
}


- (void)drawView:(GLView*)view;
{
    //NSLog (@"drawing");
    if (frontEndWantsFileName() != nil) {
#define ccFUDGE_THIS_FOR_TESTING
#ifdef FUDGE_THIS_FOR_TESTING
        
#define MYSTRING \
"#VRML V2.0 utf8\n" \
"# 10vertices.wrl\n" \
        " Shape { appearance Appearance { material Material {} } geometry IndexedFaceSet { color Color {color [0 1 0]} colorPerVertex FALSE coord Coordinate { point [-1 0 0,0 0 0,1 0 0, 1 0.5 0,1 1 0,1 2 0, 0 2 0,-1 2 0,-1 1 0, -1 0.5 0]} coordIndex [0,1,2,3,4,5,6,7,8, 9,-1] solid FALSE } }"
        
        fwl_frontEndReturningData(MYSTRING, strlen(MYSTRING));
        
#else
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
#endif
    }

    RenderSceneUpdateScene();
	
}

#pragma mark -
#pragma mark Connections


-(void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    // This method is called when the server has determined that it
    // has enough information to create the NSURLResponse.
    
    // It can be called multiple times, for example in the case of a
    // redirect, so each time we reset the data.
    
    // receivedData is an instance variable declared elsewhere.
    NSLog (@"connection: didReceiveResponse called");
    [receivedData setLength:0];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    NSLog (@"connection: didReceiveData called");
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
    
    fwl_frontEndReturningData(myData,len);

    free (myData);
    frontEndGettingFile = false;
    
    // release the connection, and the data object
    [connection release];
    [receivedData release];
}



#pragma mark -
#pragma mark Responding to gestures


/*
 In response to a tap gesture
*/
- (void)handleTapFrom:(UITapGestureRecognizer *)recognizer {
	
	CGPoint loc = [recognizer locationInView:self.view];
	NSLog(@"TAP x:%f y:%f", loc.x, loc.y);
}


/*
 In response to a pinch gesture
*/
- (void)handlePinchFrom:(UIPinchGestureRecognizer *)recognizer {
	
    // location is only recorded once
	CGPoint loc = [recognizer locationInView:self.view];
    CGFloat scale = recognizer.scale;	    
    // crude calculation to get the distance by using the screen height and the scale
    CGFloat height = [[UIScreen mainScreen] bounds].size.height;
    CGFloat distance = (height / 5) * scale; 
    
    // FIRST TOUCH
    if (recognizer.state == UIGestureRecognizerStateBegan) {
        fwl_setButDown(3, 1);
        fwl_setLastMouseEvent(ButtonPress);        
        fwl_handle_aqua(ButtonPress,3,(int)loc.x,(int)distance);
        NSLog(@"==== pinch first touch ====");
        
    // END    
    } else if (recognizer.state == UIGestureRecognizerStateEnded || recognizer.state == UIGestureRecognizerStateCancelled) {        
        
        fwl_setButDown(3, 0);
        fwl_setLastMouseEvent(ButtonRelease);
        fwl_handle_aqua(ButtonRelease,3,(int)loc.x,(int)loc.y);       
        
    // CONTINUING
    } else if (recognizer.state == UIGestureRecognizerStateChanged) {        
        fwl_setButDown(3, 1);
        fwl_setLastMouseEvent(MotionNotify);       
                
        fwl_handle_aqua(MotionNotify,3,(int)loc.x,(int)distance);
        NSLog(@"SCALE y:%f h:%f scale:%f", loc.y, height, scale);
    }
}

/*
 In response to a pan gesture
*/
- (void)handlePanFrom:(UIPanGestureRecognizer *)recognizer {
    
	CGPoint loc = [recognizer locationInView:self.view];
    
    // Velocity could be used here
    //CGPoint velocity = [recognizer velocityInView:self.view];

    
    // FIRST TOUCH
    if (recognizer.state == UIGestureRecognizerStateBegan) {
        
        fwl_setButDown(1, 1);
        fwl_setLastMouseEvent(ButtonPress);    
        //fwl_setCurXY((int)loc.x, (int)loc.y);
        fwl_handle_aqua(ButtonPress,1,(int)loc.x,(int)loc.y);      
        
    // END    
    } else if (recognizer.state == UIGestureRecognizerStateEnded || recognizer.state == UIGestureRecognizerStateCancelled) {        
        
        fwl_setButDown(1, 0);
        fwl_setLastMouseEvent(ButtonRelease);
        //fwl_setCurXY((int)loc.x, (int)loc.y);
        fwl_handle_aqua(ButtonRelease,1,(int)loc.x,(int)loc.y);       
        
    // CONTINUING
    } else if (recognizer.state == UIGestureRecognizerStateChanged) {        
        fwl_setButDown(1, 1);
        fwl_setLastMouseEvent(MotionNotify);
        
        //fwl_setCurXY((int)loc.x, (int)loc.y);
        fwl_handle_aqua(MotionNotify,1,(int)loc.x,(int)loc.y);
        NSLog(@"PAN x:%f y:%f", loc.x, loc.y);
    }	
   
}


/*
 In response to a rotation gesture
*/
- (void)handleRotationFrom:(UIRotationGestureRecognizer *)recognizer {
	
	CGPoint loc = [recognizer locationInView:self.view];
    
    CGAffineTransform transform = CGAffineTransformMakeRotation([recognizer rotation]);
    
}

/*
 In response to a swipe gesture.
 */
- (void)handleSwipeFrom:(UISwipeGestureRecognizer *)recognizer {
    
        
    
	CGPoint loc = [recognizer locationInView:self.view];
	
	
    if (recognizer.direction == UISwipeGestureRecognizerDirectionLeft) {
        
    }
    else {
        
    }
	
}

#pragma mark -
#pragma mark Memory management

- (void)dealloc {
	// JASfinalizeRenderSceneUpdateScene();
    [super dealloc];
}


@end

