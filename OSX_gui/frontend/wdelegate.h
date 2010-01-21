//
//  wdelegate.h
//  FreeWRL
//
//  Created by Sarah Dumoulin on Tue Apr 06 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface wdelegate : NSObject {
	id freewrl;
}

- (void) windowWillClose: (NSNotification*) aNotification;
- (void) setView: (id) view;
- (void) windowDidResize: (NSNotification*) aNotification;
- (void) setArrow;
- (void) setCross;

@end
