//
//  GLViewController.h
//  FreeWRL
//
//  Created by John Stewart on 11-02-27.
//  Copyright CRC Canada 2011. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "GLView.h"

/* get library definitions; do not get the rest of the
   stuff in there, as there's lots of stuff that is hard
   to compile here. */

//#define COMPILING_IPHONE_FRONT_END
#include "../../../freex3d/src/lib/libFreeWRL.h"
//#undef COMPILING_IPHONE_FRONT_END


@interface GLViewController : UIViewController <GLViewDelegate>
{

    //NSMutableDictionary *receivedData;

//@synthesize(nonatomic, retain) NSMutableData *receivedData;    
}

@end
