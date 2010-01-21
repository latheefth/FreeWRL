/* FreeWRLController */

#import <Cocoa/Cocoa.h>
#import "FreeWRLView.h"

@interface FreeWRLController : NSObject
{

    IBOutlet NSButton *allowScripts;
    IBOutlet NSButton *eaiPort;
    IBOutlet NSTextField *eyeDistance;
    IBOutlet NSButton *fastMode;
    IBOutlet NSTextField *fileName;
    IBOutlet NSButton *fullScreen;
    IBOutlet NSTextField *geometry;
    IBOutlet NSTextField *geometryh;
    IBOutlet NSTextField *logFile;
    IBOutlet NSTextField *maxImages;
    IBOutlet NSTextField *saveDir;
    IBOutlet NSTextField *screenDistance;
    IBOutlet NSTextField *seqFile;
    IBOutlet NSButton *seqImages;
    IBOutlet NSTextField *seqTempFile;
    IBOutlet NSButton *shutterMode;
    IBOutlet NSTextField *snapFile;
    IBOutlet NSTextField *zbuffer;
	IBOutlet NSTextField *ConsoleMessage;
	IBOutlet NSWindow   *ConsoleWindow;
    FreeWRLView*  freewrlView;
    NSWindow* theWindow;
}
- (IBAction)chooseFile:(id)sender;
- (void) showMessage: (NSString*) message;
- (IBAction)start:(id)sender;
- (IBAction)dismiss: (id) sender;
@end