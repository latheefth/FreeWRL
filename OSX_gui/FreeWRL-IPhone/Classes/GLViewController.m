//
//  GLViewController.m
//  FW_testing
//
//  Created by John Stewart on 11-02-15.
//  Copyright CRC Canada 2011. All rights reserved.
//

#import "GLViewController.h"
#import "ConstantsAndMacros.h"

NSString *initialURL = nil;
UITextField *urlFieldText = nil;


@interface MyObject : NSObject
+(void)aMethod:(id)param;
@end

@implementation MyObject
+(void)aMethod:(id)param {
    //NSLog (@"starting loading thread");
    
   
    
    fwl_initializeRenderSceneUpdateScene();
    
    // the user hit return, and we are flying...
    if (initialURL != nil) {
        //const char* cString = [initialURL UTF8String]; 
        //NSLog (@"initial url %s",cString);
        
        fwl_OSX_initializeParameters(
            [initialURL UTF8String]);
          
        
        
    }
    //fwl_OSX_initializeParameters("http://freewrl.sourceforge.net/test2pt.wrl");
   // fwl_OSX_initializeParameters( //"http://freewrl.sourceforge.net/JAS/SSID-Mar2011/staticCount500.x3d");
                             //"http://freewrl.sourceforge.net/test2.wrl");
    
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
    
    // JAS - trying auto rotation sensing for landscape,portrait, etc.
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(receivedRotate:) name: UIDeviceOrientationDidChangeNotification object: nil];
    
    // global data area
    NSLog (@"calling fwl_init_instance()");
    fwl_init_instance();
}


-(void)setupView:(GLView*)view
{
//#ifdef TRY_SCREEN_RESOLUTION_1
    /* trying resolution first way */
    {
        
        int w = 320;
        int h = 480;
        
        float ver = [[[UIDevice currentDevice] systemVersion] floatValue];
        // You can't detect screen resolutions in pre 3.2 devices, but they are all 320x480
        NSLog (@"UIDevice version %f",ver);
        if (ver >= 3.2f)
        {
            UIScreen* mainscr = [UIScreen mainScreen];
            w = mainscr.currentMode.size.width;
            h = mainscr.currentMode.size.height;
            NSLog (@"UIDevice try 1 size w %d,  h %d",w,h);
            fwl_setScreenDim(w,h); 
        }
        
    }
//#endif /* TRY_SCREEN_RESOLUTION_1 */
    
    /* trying resolution second way */
    
    float scaleFac = [[UIScreen mainScreen] scale];
    NSLog (@"mainScreen scale factor %f",scaleFac);
    
    CGRect screenRect = [[UIScreen mainScreen] bounds];
    CGFloat screenWidth = screenRect.size.width;
    CGFloat screenHeight = screenRect.size.height;
    NSLog (@"Screen resolution, 2nd way, %f x %f",screenWidth, screenHeight);
    //fwl_setScreenDim((int)screenWidth, (int)screenHeight); 
        
    
    
//#ifdef TRY_SCREEN_RESOLUTION_3   
	//NSLog (@"setupView");
	CGRect rect = view.bounds;
    NSLog (@"Third way - size %d %d",(int)rect.size.width, (int)rect.size.height);
	//fwl_setScreenDim((int)rect.size.width, (int)rect.size.height);    
	//fwl_setScreenDim((int)rect.size.height, (int)rect.size.width);
//#endif /* TRY_SCREEN_RESOLUTION_3 */
    
    
	fv_display_initialize();
	
        // thread the getting of the file...
    //[NSThread detachNewThreadSelector:@selector(aMethod:) toTarget:[MyObject class] withObject:nil];
    
    
    // show the URL text field, and the keyboard
    [URLField becomeFirstResponder];
    
	// NSLog (@"setupView complete");
}


- (void)drawView:(GLView*)view;
{
    //NSLog (@"drawing");
    if (frontEndWantsFileName() != nil) {
#define FUDGE_THIS_FOR_TESTING
#ifdef FUDGE_THIS_FOR_TESTING
        
#define MYSTRING \
"#VRML V2.0 utf8\n" \
"NavigationInfo {type \"EXAMINE\"}\n" \
"#Background {skyAngle        [ 1.07 1.45 1.52 1.57 ]skyColor        [ 0.00 0.00 0.30 0.00 0.00 0.80 0.45 0.70 0.80 0.70 0.50 0.00 1.00 0.00 0.00 ] groundAngle     1.57 groundColor     [ 0.0 0.0 0.0, 0.0 0.7 0.0 ]}" \
        " Shape { appearance Appearance { material Material {} } geometry Box {}  }"
        
        fwg_frontEndReturningData(MYSTRING, strlen(MYSTRING));
        
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
            [StatusBar setText:@"Loading..."];

        } else {
            [StatusBar setText:@"URL Not valid"];

            // Inform the user that the connection failed.
        }
        }
#endif
    }

    fwl_RenderSceneUpdateScene();
	
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
    //NSLog (@"connection: didReceiveResponse called");
    [StatusBar setText:@"receiving..."];
    [receivedData setLength:0];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    NSLog (@"connection: didReceiveData called");
    [StatusBar setText:@"data..."];
    
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
    //NSLog(@"Connection failed! Error - %@ %@",
      //    [error localizedDescription],
        //  [[error userInfo] objectForKey:NSURLErrorFailingURLStringErrorKey]);
    
    [StatusBar setText:@"URL Invalid"];

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
    
    [StatusBar setText:@"Data -> FreeWRL"];
    urlFieldText.hidden=YES;
    
    fwg_frontEndReturningData(myData,len);

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
        fwl_handle_aqua(ButtonPress,1,(int)loc.x,(int)loc.y);      
        
    // END    
    } else if (recognizer.state == UIGestureRecognizerStateEnded || recognizer.state == UIGestureRecognizerStateCancelled) {        
        
        fwl_setButDown(1, 0);
        fwl_setLastMouseEvent(ButtonRelease);
        fwl_handle_aqua(ButtonRelease,1,(int)loc.x,(int)loc.y);       
        
    // CONTINUING
    } else if (recognizer.state == UIGestureRecognizerStateChanged) {        
        fwl_setButDown(1, 1);
        fwl_setLastMouseEvent(MotionNotify);
        
        fwl_handle_aqua(MotionNotify,1,(int)loc.x,(int)loc.y);
        //NSLog(@"PAN x:%f y:%f", loc.x, loc.y);
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


-(void) receivedRotate: (NSNotification*) notification
{
    UIDeviceOrientation interfaceOrientation = [[UIDevice currentDevice] orientation];
    //NSLog (@"recievedRotate");
    if(interfaceOrientation == UIInterfaceOrientationLandscapeLeft)
    {
        fwl_setOrientation(90);
        
    } else if (interfaceOrientation == UIInterfaceOrientationLandscapeRight)
    { 
        fwl_setOrientation(270);
    } else if (interfaceOrientation == UIInterfaceOrientationPortrait)
    {
        fwl_setOrientation(0);
    } else if (interfaceOrientation == UIInterfaceOrientationPortraitUpsideDown)
    {
        fwl_setOrientation(180);
    } else {
        //NSLog (@"rotate not handled!");
    }
    
}


- (BOOL)acceptsFirstResponder {
    return YES;

}
- (IBAction)ViewPointPressed:(id)sender {
    NSLog (@"VP");
    fwl_Next_ViewPoint();
    
}

- (IBAction)WalkModePressed:(id)sender {
    NSLog (@"Wakl");
    fwl_set_viewer_type(VIEWER_WALK);
}
- (IBAction)ExamineModePressed:(id)sender {
    NSLog (@"Examine");
    fwl_set_viewer_type(VIEWER_EXAMINE);
}





- (IBAction)URLField:(id)sender {
    //UITextField inField;
    //inField = (UITextField*)sender;
    NSLog (@"URLField text");
    [sender resignFirstResponder];
    
    urlFieldText = (UITextField*) sender;
    [urlFieldText retain];
    
    initialURL = [[NSString stringWithString:((UITextField *)sender).text] retain];
    
     [StatusBar setText:@"Resolving..."];
    
    [NSThread detachNewThreadSelector:@selector(aMethod:) toTarget:[MyObject class] withObject:nil];

}


/* stop landscape/portrait, etc...
-(void) viewWillDisappear: (BOOL) animated{
    [[NSNotificationCenter defaultCenter] removeObserver: self];
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
}
 */


#pragma mark -
#pragma mark Memory management

- (void)dealloc {
	// JASfinalizeRenderSceneUpdateScene();
    [ViewPointPressed release];
    [WalkModePressed release];
    [ExamineModePressed release];
    [URLField release];
    [urlFieldText release];
    [StatusBar release];
    [super dealloc];
}


@end

