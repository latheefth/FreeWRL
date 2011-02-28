//
//  GLViewController.m
//  FW_testing
//
//  Created by John Stewart on 11-02-15.
//  Copyright CRC Canada 2011. All rights reserved.
//

#import "GLViewController.h"
#import "ConstantsAndMacros.h"
@implementation GLViewController

-(void)setupView:(GLView*)view
{
	//NSLog (@"setupView");
	CGRect rect = view.bounds;
	//NSLog (@"initialize - size %d %d",(int)rect.size.width, (int)rect.size.height);
	setScreenDim((int)rect.size.width, (int)rect.size.height);
	display_initialize();
	OSX_initializeParameters("wrong path entered");
	setMono(); /* no shutter glasses... */
	initializeRenderSceneUpdateScene();
	
	//NSLog (@"setupView complete");
}

- (void)drawView:(GLView*)view;
{
	
	RenderSceneUpdateScene();
	
}

- (void)dealloc {
	// JASfinalizeRenderSceneUpdateScene();
    [super dealloc];
}
@end

