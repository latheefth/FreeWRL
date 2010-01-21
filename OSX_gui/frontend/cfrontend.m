//
//  cfrontend.m
//  FreeWRL
//
//  Created by Sarah Dumoulin on Wed Jan 28 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import "cfrontend.h"

void displayThread() {

	//glpOpenGLInitialize();
	initialize_GL();
	
	new_tessellation();
	while (1) {
		EventLoop();
	}
}
