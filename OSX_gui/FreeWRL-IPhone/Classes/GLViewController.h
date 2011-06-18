//
//  GLViewController.h
//  FreeWRL
//
//  Created by John Stewart on 11-02-27.
//  Copyright CRC Canada 2011. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "GLView.h"
#import "RecentItems.h"

#include "../../../freex3d/src/lib/libFreeWRL.h"

@interface GLViewController : UIViewController <GLViewDelegate, UIGestureRecognizerDelegate>
{
    UITextField *urlFieldText;
    
    IBOutlet UIButton *quitPressed;
    IBOutlet UILabel *StatusBar;
    
    IBOutlet UIButton *Recent2;
    IBOutlet UIButton *Recent1;
    IBOutlet UIButton *ViewPointPressed;
    
    IBOutlet UITextField *URLField;
    IBOutlet UIButton *ExamineModePressed;
    IBOutlet UIButton *WalkModePressed;
    //NSMutableDictionary *receivedData;

//@synthesize(nonatomic, retain) NSMutableData *receivedData;    
}

@end
