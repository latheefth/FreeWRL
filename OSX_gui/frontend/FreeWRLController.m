#import "FreeWRLController.h"

@implementation FreeWRLController

- (IBAction)chooseFile:(id)sender
{
    int result;
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];

    [oPanel setAllowsMultipleSelection:NO];
    result = [oPanel runModalForDirectory:NSHomeDirectory()
                    file:nil types:nil];
    if (result == NSOKButton) {
        NSArray *filesToOpen = [oPanel filenames];
        NSString *aFile = [filesToOpen objectAtIndex:0];
        [fileName setStringValue: aFile];
    }
}

- (IBAction)start:(id)sender
{
    NSWindow* myWindow;
    // Button values
    int fastState;
    int shutterState;
    int fullScreenState;
    int seqImagesState;
    int scriptsState;
    
    // Text field values
    NSString* fileToOpen;
    int eaiValue;
    int zbufferDepthValue;
    int geometryWidth;
    int geometryHeight;
    NSString* saveDirectoryValue;
    NSString* seqFileValue;
    NSString* snapFileValue;
    NSString* seqTempFileValue;
    float     eyeDistValue;
    float     screenDistValue;
    int	      maxImagesValue;
    
    // Get Button States
    shutterState = [shutterMode state];
    fastState = [fastMode state];
    fullScreenState = [fullScreen state];
    seqImagesState = [seqImages state];
    scriptsState = [allowScripts state];
    
    // Get textfield values
    fileToOpen = [fileName stringValue];
    eaiValue = [eaiPort state];
    zbufferDepthValue = [zbuffer intValue];
    geometryWidth = [geometry intValue];
    geometryHeight = [geometryh intValue];
    saveDirectoryValue = [saveDir stringValue];
    seqFileValue = [seqFile stringValue];
    snapFileValue = [snapFile stringValue];
    seqTempFileValue = [seqTempFile stringValue];
    eyeDistValue = [eyeDistance floatValue];
    screenDistValue = [screenDistance floatValue];
    maxImagesValue = [maxImages intValue];
    
    // !! ASK ABOUT DO PERL SCRIPTS !!
        
    // Open file
    [freewrlView setFile: fileToOpen]; 
    //NSLog(@"Should have set file to open");
	
    if (fullScreenState == NSOnState)
    {
        [freewrlView fullScreen];
    }
    
    
    if (eaiValue == NSOnState)
    {
		//NSLog(@"should call startEAI");
         [freewrlView startEai];
    }
    
    if (seqImagesState == NSOnState)
    {
        [freewrlView setSeqImages];
    }

    if ((geometryHeight != 0) && (geometryWidth != 0))
    {
        [freewrlView setHeight: geometryHeight width: geometryWidth];
    }
        
    if (maxImagesValue != 0)
    {
        [freewrlView fwl_set_MaxImages: maxImagesValue];
    }
    
    if ([seqFileValue length] != 0)
    {
        [freewrlView fwl_set_SeqFile: seqFileValue];
        [freewrlView setSeqImages];
    }
    
    if ([snapFileValue length] != 0)
    {
        [freewrlView fwl_set_SnapFile: snapFileValue];
    }
    
    if ([seqTempFileValue length] != 0)
    {
        [freewrlView setTempSeqFile: seqTempFileValue];
    }
    
	// Start browser
    [freewrlView initBrowser];

    // Close options window
    myWindow = [sender window];
    [myWindow close];
}


- (IBAction)dismiss:(id)sender
{
	NSWindow* myWindow;
	
	myWindow = [sender window];
    [myWindow orderOut:nil];
}

- (void) setFreeWRLView:(FreeWRLView*) view
{
    freewrlView = view;
	[freewrlView setController: self];
}

- (void) tryit
{
    //NSLog(@"Got to tryit");
}

- (void) setWindow: (NSWindow*) passedWindow
{
    theWindow = passedWindow;
}

- (void) showMessage: (NSString*) message
{
	//NSLog(@"ConsoleMessage: %@", ConsoleMessage);
	//NSLog(@"ConsoleWindow: %@", ConsoleWindow);
	[ConsoleMessage setStringValue: message];
	[ConsoleWindow makeKeyAndOrderFront:nil];
}

@end
