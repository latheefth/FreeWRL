/*
  $Id$

  FreeWRL support library.

Purpose:
  Common UI for all platforms.

Data:
  Handle internal FreeWRL library variables related to UI.

Functions:
  Update internal FreeWRL library variables related to UI.
  NO PLATFORM SPECIFIC CODE HERE. ALL GENERIC CODE.

*/

#ifndef __LIBFREEWRL_UI_COMMON_H__
#define __LIBFREEWRL_UI_COMMON_H__


/* Generic declarations */

#define SCURSE 1
#define ACURSE 0

#define SENSOR_CURSOR ccurse = SCURSE
#define ARROW_CURSOR  ccurse = ACURSE

/* Status variables */

extern int ccurse;
extern int ocurse;



/* Status update functions */

void setMenuFps(float fps);
void setMenuStatus(char *stat); 
void setMenuStatusVP(char *stat); 
char* getMenuStatus();
void setMessageBar();


/* Generic (virtual) update functions */

void setCursor();
void setArrowCursor();
void setSensorCursor();
void setWindowTitle0();
void setWindowTitle();
char *getMessageBar();
char *getWindowTitle();
void updateCursorStyle();

#ifdef _MSC_VER
#define snprintf _snprintf
#endif


/* from http://www.web3d.org/files/specifications/19775-1/V3.2/Part01/components/keyboard.html#KeySensor
This needs to be included where the platform-specific key event handler is, so a
platform-specific int platform2web3dActionKeyPLATFORM_NAME(int platformKey)
function can refer to them, to send in web3d key equivalents, or at least FW neutral keys.
If a platform key, after lookup, is in this list, then call:
fwl_do_rawKeypress(actionKey,updown+10);
section 21.4.1 
Key Value
Home 13
End 14
PGUP 15
PGDN 16
UP 17
DOWN 18
LEFT 19
RIGHT 20
F1-F12  1 to 12
ALT,CTRL,SHIFT true/false
*/
#define F1_KEY  1
#define F2_KEY  2
#define F3_KEY  3
#define F4_KEY  4
#define F5_KEY  5
#define F6_KEY  6
#define F7_KEY  7
#define F8_KEY  8
#define F9_KEY  9
#define F10_KEY 10
#define F11_KEY 11
#define F12_KEY 12
#define HOME_KEY 13
#define END_KEY  14
#define PGUP_KEY 15
#define PGDN_KEY 16
#define UP_KEY   17
#define DOWN_KEY 18
#define LEFT_KEY 19
#define RIGHT_KEY 20
#define ALT_KEY	30 /* not available on OSX */
#define CTL_KEY 31 /* not available on OSX */
#define SFT_KEY 32 /* not available on OSX */
#define DEL_KEY 0XFFFF /* problem: I'm insterting this back into the translated char stream so 0XFFFF too high to clash with a latin? */
#define RTN_KEY 13  //what about 10 newline?
#define NUM0  40
#define NUM1  41
#define NUM2  42
#define NUM3  43
#define NUM4  44
#define NUM5  45
#define NUM6  46
#define NUM7  47
#define NUM8  48
#define NUM9  49
#define NUMDEC 50



#endif /* __LIBFREEWRL_UI_COMMON_H__ */
