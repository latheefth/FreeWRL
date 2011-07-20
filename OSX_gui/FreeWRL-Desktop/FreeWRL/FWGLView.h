//
// File:		FWGLView.h
//

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>

// this is the inital file to open. We keep it global so that the loading thread
// can easily see it.
NSString*   fileToOpen;


typedef struct {
   GLdouble x,y,z;
} recVec;

typedef struct {
	recVec viewPos; // View position
	recVec viewDir; // View direction vector
	recVec viewUp; // View up direction
	recVec rotPoint; // Point to rotate about
	GLdouble aperture; // pContextInfo->camera aperture
	GLint viewWidth, viewHeight; // current window/screen height and width
} recCamera;

@interface FWGLView : NSOpenGLView
{
    
	NSTimer* timer;
 
    bool fAnimate;
	IBOutlet NSMenuItem * animateMenuItem;
    bool fInfo;    
    
    recCamera camera;
}

+ (NSOpenGLPixelFormat*) basicPixelFormat;

- (void) resizeGL;


- (void)animationTimer:(NSTimer *)timer;

-(IBAction) animate: (id) sender;
-(IBAction) info: (id) sender;

- (void)keyDown:(NSEvent *)theEvent;

- (void) mouseDown:(NSEvent *)theEvent;
- (void) rightMouseDown:(NSEvent *)theEvent;
- (void) mouseUp:(NSEvent *)theEvent;
- (void) rightMouseUp:(NSEvent *)theEvent;
- (void) mouseDragged:(NSEvent *)theEvent;
- (void) rightMouseDragged:(NSEvent *)theEvent;

- (void) drawRect:(NSRect)rect;

- (void) prepareOpenGL;
- (void) update;		// moved or resized

- (BOOL) acceptsFirstResponder;
- (BOOL) becomeFirstResponder;
- (BOOL) resignFirstResponder;

- (id) initWithFrame: (NSRect) frameRect;
- (void) awakeFromNib;

@end
