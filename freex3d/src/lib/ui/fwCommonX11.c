/*

  FreeWRL support library.
  X11 common functions.

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <config.h>

// OLD_IPHONE_AQUA #if !(defined(IPHONE) || defined(_ANDROID) || defined(AQUA))

#if !(defined(_ANDROID))

#include <system.h>
#include <display.h>
#include <internal.h>

#include <threads.h>

#include <libFreeWRL.h>

#include "ui/common.h"
#include  <X11/cursorfont.h>

static Cursor arrowc;
static Cursor sensorc;
static Cursor cursor;

#if KEEP_X11_INLIB

int win_height; /* window */
int win_width;
int fullscreen;
int shutterGlasses; /* shutter glasses, stereo enabled ? */
int quadbuff_stereo_mode; /* quad buffer enabled ? */

GLXContext GLcx;
long event_mask;
XEvent event;
Display *Xdpy;
int Xscreen;
Window Xroot_window;
Colormap colormap;
XVisualInfo *Xvi;
Window Xwin;
Window GLwin;
XSetWindowAttributes attr;
unsigned long mask = 0;
Atom WM_DELETE_WINDOW;

long event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
                    ButtonMotionMask | ButtonReleaseMask |
                    ExposureMask | StructureNotifyMask |
                    PointerMotionMask;

/**
 * X86 vmode : choose extended graphic mode
 */
#ifdef HAVE_XF86_VMODE

int oldx = 0, oldy = 0;
int vmode_nb_modes;
XF86VidModeModeInfo **vmode_modes = NULL;
int vmode_mode_selected = -1;

/**
 * quick sort comparison function to sort X modes
 */
static int mode_cmp(const void *pa,const void *pb)
{
    XF86VidModeModeInfo *a = *(XF86VidModeModeInfo**)pa;
    XF86VidModeModeInfo *b = *(XF86VidModeModeInfo**)pb;
    if(a->hdisplay > b->hdisplay) return -1;
    return b->vdisplay - a->vdisplay;
}

void fv_switch_to_mode(int i)
{
    if ((!vmode_modes) || (i<0)) {
	ERROR_MSG("fv_switch_to_mode: no valid mode available.\n");
	return;
    }

    vmode_mode_selected = i;

    win_width = vmode_modes[i]->hdisplay;
    win_height = vmode_modes[i]->vdisplay;
    TRACE_MSG("fv_switch_to_mode: mode selected: %d (%d,%d).\n",
	  vmode_mode_selected, win_width, win_height);
    XF86VidModeSwitchToMode(Xdpy, Xscreen, vmode_modes[i]);
    XF86VidModeSetViewPort(Xdpy, Xscreen, 0, 0);
}
#endif /* HAVE_XF86_VMODE */

/**
 *   fv_find_best_visual: use GLX to choose the X11 visual.
 */
XVisualInfo *fv_find_best_visual()
{
	XVisualInfo *vi = NULL;
#define DEFAULT_COMPONENT_WEIGHT 5

	/*
	 * If FreeWRL is to be configurable one day,
	 * we will improve this visual query.
	 * One possibility: glXGetConfig.
	 */
	static int attribs[100] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE,    DEFAULT_COMPONENT_WEIGHT,
		GLX_GREEN_SIZE,  DEFAULT_COMPONENT_WEIGHT,
		GLX_BLUE_SIZE,   DEFAULT_COMPONENT_WEIGHT,
		GLX_ALPHA_SIZE,  DEFAULT_COMPONENT_WEIGHT,
		GLX_DEPTH_SIZE,  DEFAULT_COMPONENT_WEIGHT,
		None
	};

	if (shutterGlasses) {
		/* FIXME: handle stereo visual creation */
#ifdef STEREOCOMMAND
		system(STEREOCOMMAND);
#endif
	}

	if ((shutterGlasses) && (quadbuff_stereo_mode == 0)) {
		TRACE_MSG("Warning: No quadbuffer stereo visual found !");
		TRACE_MSG("On SGI IRIX systems read 'man setmon' or 'man xsetmon'\n");
	}

	quadbuff_stereo_mode = 0;

	vi = glXChooseVisual(Xdpy, Xscreen, attribs);
	return vi;
}

static int fv_catch_XLIB(Display *disp, XErrorEvent *err)
{
	static int XLIB_errors = 0;
	static char error_msg[4096];

	XGetErrorText(disp, err->error_code, error_msg, sizeof(error_msg));

	ERROR_MSG("FreeWRL caught an XLib error !\n"
		  "   Display:    %s (%p)\n"
		  "   Error code: %d\n"
		  "   Error msg:  %s\n"
		  "   Request:    %d\n",
		  XDisplayName(NULL), disp, err->error_code,
		  error_msg, err->request_code);

	XLIB_errors++;
	if (XLIB_errors > 20) {
		ERROR_MSG("FreeWRL - too many XLib errors (%d>20), exiting...\n", XLIB_errors);
		exit(0);
	}
	return 0;
}

int fv_create_colormap()
{
	colormap = XCreateColormap(Xdpy, RootWindow(Xdpy, Xvi->screen),Xvi->visual, AllocNone);
	return TRUE;
}

/* void setMenuFps(float fps) */
/* { */
/* 	myFps = fps; */
/* 	setMessageBar(); */
/* } */

void fv_resetGeometry()
{
#ifdef HAVE_XF86_VMODE
    int oldMode, i;

    if (fullscreen) {
	XF86VidModeGetAllModeLines(Xdpy, Xscreen, &vmode_nb_modes, &vmode_modes);
	oldMode = 0;

	for (i=0; i < vmode_nb_modes; i++) {
	    if ((vmode_modes[i]->hdisplay == oldx) && (vmode_modes[i]->vdisplay==oldy)) {
		oldMode = i;
		break;
	    }
	}

	XF86VidModeSwitchToMode(Xdpy, Xscreen, vmode_modes[oldMode]);
	XF86VidModeSetViewPort(Xdpy, Xscreen, 0, 0);
	XFlush(Xdpy);
    }
#endif /* HAVE_XF86_VMODE */
}

/*======== "VIRTUAL FUNCTIONS" ==============*/

/**
 *   fv_open_display: setup up X11, choose visual, create colomap and query fullscreen capabilities.
 */
int fv_open_display()
{
    char *display;

    fwl_thread_dump();

    /* Display */
    XInitThreads();

    display = getenv("DISPLAY");
    Xdpy = XOpenDisplay(display);
    if (!Xdpy) {
	ERROR_MSG("can't open display %s.\n", display);
	return FALSE;
    }

    /* start up a XLib error handler to catch issues with FreeWRL. There
       should not be any issues, but, if there are, we'll most likely just
       throw our hands up, and continue */
    XSetErrorHandler(fv_catch_XLIB);

    Xscreen = DefaultScreen(Xdpy);
    Xroot_window = RootWindow(Xdpy,Xscreen);

    /* Visual */

    Xvi = fv_find_best_visual();
    if(!Xvi) {
	    ERROR_MSG("FreeWRL can not find an appropriate visual from GLX\n");
	    return FALSE;
    }

    /* Fullscreen */

    if (fullscreen) {
#ifdef HAVE_XF86_VMODE
	    int i;
	    if (vmode_modes == NULL) {
		    if (XF86VidModeGetAllModeLines(Xdpy, Xscreen, &vmode_nb_modes, &vmode_modes) == 0) {
			    ERROR_MSG("can`t get mode lines through XF86VidModeGetAllModeLines.\n");
			    return FALSE;
		    }
		    qsort(vmode_modes, vmode_nb_modes, sizeof(XF86VidModeModeInfo*), mode_cmp);
	    }
	    for (i = 0; i < vmode_nb_modes; i++) {
		    if (vmode_modes[i]->hdisplay <= win_width && vmode_modes[i]->vdisplay <= win_height) {
			    fv_switch_to_mode(i);
			    break;
		    }
	    }
#endif
    }


    /* Color map */
    fv_create_colormap();

    /* Initialize cursors */
    loadCursors();

    return TRUE;
}

/*=== fv_create_main_window: in fwBareWindow.c or in fwMotifWindow.c */

/**
 *   fv_create_GLcontext: create the main OpenGL context.
 *                     TODO: finish implementation for Mac and Windows.
 */
bool fv_create_GLcontext()
{
	int direct_rendering = TRUE;

	fwl_thread_dump();

#if defined(TARGET_X11) || defined(TARGET_MOTIF)

	GLcx = glXCreateContext(Xdpy, Xvi, NULL, direct_rendering);
	if (!GLcx) {
		ERROR_MSG("can't create OpenGL context.\n");
		return FALSE;
	}
	if (glXIsDirect(Xdpy, GLcx)) {
		TRACE_MSG("glX: direct rendering enabled\n");
	}
#endif
	return TRUE;
}
bool fv_create_GLcontext1(freewrl_params_t *share)
{
	GLXContext share_context;
	int direct_rendering = TRUE;
	share_context = NULL;
	if(share) share_context = share->context;

	fwl_thread_dump();

#if defined(TARGET_X11) || defined(TARGET_MOTIF)

	GLcx = glXCreateContext(Xdpy, Xvi, share_context, direct_rendering);
	if (!GLcx) {
		ERROR_MSG("can't create OpenGL context.\n");
		return FALSE;
	}
	if (glXIsDirect(Xdpy, GLcx)) {
		TRACE_MSG("glX: direct rendering enabled\n");
	}
#endif
	return TRUE;
}
/**
 *   fv_bind_GLcontext: attache the OpenGL context to the main window.
 *                   TODO: finish implementation for Mac and Windows.
 */
bool fv_bind_GLcontext()
{
	fwl_thread_dump();

#if defined(TARGET_X11) || defined(TARGET_MOTIF)
	if (!Xwin) {
		ERROR_MSG("window not initialized, can't initialize OpenGL context.\n");
		return FALSE;
	}
	if (!glXMakeCurrent(Xdpy, GLwin, GLcx)) {
/*
		ERROR_MSG("fv_bind_GLcontext: can't set OpenGL context for this thread %d (glXMakeCurrent: %s).\n", fw_thread_id(), GL_ERROR_MSG);
*/
		ERROR_MSG("fv_bind_GLcontext: can't set OpenGL context for this thread %d  , glGetError=%d).\n", fw_thread_id(), glGetError());
		return FALSE;
	}
#endif

// OLD_IPHONE_AQUA #if defined(TARGET_AQUA)
// OLD_IPHONE_AQUA 	return aglSetCurrentContext(aqglobalContext);
// OLD_IPHONE_AQUA #endif

	return TRUE;
}
#endif /* KEEP_FV_INLIB */

/**
 *  Initialize cursor types for X11
 *
 */
void loadCursors() {
	arrowc = XCreateFontCursor(Xdpy,XC_arrow);
	sensorc = XCreateFontCursor(Xdpy,XC_hand1);
}

/**
 * setCursor() declared as generic in common.h
 * specific X11 implementation
 */
void setCursor(int ccurse)
{
	switch (ccurse) {
	case SCURSE: cursor = sensorc; break;
	case ACURSE: cursor = arrowc; break;
	default:
		DEBUG_MSG("setCursor: invalid value for ccurse: %d\n", ccurse);
	}
	XDefineCursor(Xdpy, GLwin, cursor);
}

void setWindowTitle()
{
	XStoreName(Xdpy, Xwin, getWindowTitle());
	XSetIconName(Xdpy, Xwin, getWindowTitle());
}
int fv_create_window_and_context(freewrl_params_t *params, freewrl_params_t *share){
 	/* make the window, create the OpenGL context, share the context if necessary 
		Nov 2015: linux desktop is still single windowed, with static GLXContext etc, no sharing
		- to get sharing, you need to populate params during creation of window and gl context
			d->display = Display *Xdpy;
			d->surface = Drawable or ???
			d->context = GLXContext GLcx;
			so when the targetwindow changes, there's enough info to do glXMakeCurrent and glXSwapBuffers
			- and when doing glCreateContext you have the previous window's GLXcontext to use as a shareList
	*/


	if (!fv_open_display()) {
		printf("open_display failed\n");
		return FALSE;
	}

	if (!fv_create_GLcontext1(share)) {
		printf("create_GLcontext failed\n");
		return FALSE;
	}
	fv_create_main_window(params);

	fv_bind_GLcontext();
	//scrape parameters from statics
	params->context = GLcx;
	params->display = Xdpy;

	// JAS - this is actually a "Window" but store it as a pointer... 
	params->surface = (void*) GLwin;

	return TRUE;
}
// remember, Window {aka long unsigned int} is stored as a pointer, so 
// type cast it back again.

void fv_change_GLcontext(freewrl_params_t* d){
	glXMakeCurrent(d->display,
		(Window) d->surface,
		d->context); 
}
void fv_swapbuffers(freewrl_params_t* d){
	glXSwapBuffers(d->display,
		(Window) d->surface);
}

#define TRY_MAINLOOP_STUFF_HERE 1
#ifdef TRY_MAINLOOP_STUFF_HERE
//XK_ constants from /usr/include/X11/keysymdef.h
#define PHOME_KEY XK_Home //80
#define PPGDN_KEY XK_Page_Down //86
#define PLEFT_KEY XK_Left //106
#define PEND_KEY XK_End //87
#define PUP_KEY XK_Up //112
#define PRIGHT_KEY XK_Right //108
#define PPGUP_KEY XK_Page_Up //85
#define PDOWN_KEY XK_Down //59
#define PF1_KEY  XK_F1 //0xFFBE
#define PF12_KEY XK_F12 //0XFFC9
#define PALT_KEY XK_Alt_L //0XFFE9 //left, and 0XFFEA   //0XFFE7
#define PALT_KEYR XK_Alt_R //0XFFE9 //left, and 0XFFEA   //0XFFE7
#define PCTL_KEY XK_Control_L //0XFFE3 //left, and 0XFFE4 on right
#define PCTL_KEYR XK_Control_R //0XFFE3 //left, and 0XFFE4 on right
#define PSFT_KEY XK_Shift_L //0XFFE1 //left, and 0XFFE2 on right
#define PSFT_KEYR XK_Shift_R //0XFFE1 //left, and 0XFFE2 on right
#define PDEL_KEY XK_Delete //0XFF9F //on numpad, and 0XFFFF near Insert //0x08
#define PNUM0 XK_KP_Insert    //XK_KP_0
#define PNUM1 XK_KP_End       //XK_KP_1
#define PNUM2 XK_KP_Down      //XK_KP_2
#define PNUM3 XK_KP_Page_Down //XK_KP_3
#define PNUM4 XK_KP_Left      //XK_KP_4
#define PNUM5 XK_KP_Begin     //XK_KP_5
#define PNUM6 XK_KP_Right     //XK_KP_6
#define PNUM7 XK_KP_Home      //XK_KP_7
#define PNUM8 XK_KP_Up        //XK_KP_8
#define PNUM9 XK_KP_Page_Up   //XK_KP_9
#define PNUMDEC XK_KP_Delete //XK_KP_Decimal

///* from http://www.web3d.org/x3d/specifications/ISO-IEC-19775-1.2-X3D-AbstractSpecification/index.html
//section 21.4.1
//Key Value
//Home 13
//End 14
//PGUP 15
//PGDN 16
//UP 17
//DOWN 18
//LEFT 19
//RIGHT 20
//F1-F12  1 to 12
//ALT,CTRL,SHIFT true/false
//*/
//#define F1_KEY  1
//#define F2_KEY  2
//#define F3_KEY  3
//#define F4_KEY  4
//#define F5_KEY  5
//#define F6_KEY  6
//#define F7_KEY  7
//#define F8_KEY  8
//#define F9_KEY  9
//#define F10_KEY 10
//#define F11_KEY 11
//#define F12_KEY 12
//#define HOME_KEY 13
//#define END_KEY  14
//#define PGUP_KEY 15
//#define PGDN_KEY 16
//#define UP_KEY   17
//#define DOWN_KEY 18
//#define LEFT_KEY 19
//#define RIGHT_KEY 20
//#define ALT_KEY	30 /* not available on OSX */
//#define CTL_KEY 31 /* not available on OSX */
//#define SFT_KEY 32 /* not available on OSX */
//#define DEL_KEY 0XFFFF /* problem: I'm insterting this back into the translated char stream so 0XFFFF too high to clash with a latin? */
//#define RTN_KEY 13  //what about 10 newline?


int platform2web3dActionKeyLINUX(int platformKey)
{
	int key;

	key = 0; //platformKey;
	if(platformKey >= PF1_KEY && platformKey <= PF12_KEY)
		key = platformKey - PF1_KEY + F1_KEY;
	else
		switch(platformKey)
		{
		case PHOME_KEY:
			key = HOME_KEY; break;
		case PEND_KEY:
			key = END_KEY; break;
		case PPGDN_KEY:
			key = PGDN_KEY; break;
		case PPGUP_KEY:
			key = PGUP_KEY; break;
		case PUP_KEY:
			key = UP_KEY; break;
		case PDOWN_KEY:
			key = DOWN_KEY; break;
		case PLEFT_KEY:
			key = LEFT_KEY; break;
		case PRIGHT_KEY:
			key = RIGHT_KEY; break;
		case PDEL_KEY:
			key = DEL_KEY; break;
		case PALT_KEY:
		case PALT_KEYR:
			key = ALT_KEY; break;
		case PCTL_KEY:
		case PCTL_KEYR:
			key = CTL_KEY; break;
		case PSFT_KEY:
		case PSFT_KEYR:
			key = SFT_KEY; break;
		case PNUM0:
			key = NUM0; break;
		case PNUM1:
			key = NUM1; break;
		case PNUM2:
			key = NUM2; break;
		case PNUM3:
			key = NUM3; break;
		case PNUM4:
			key = NUM4; break;
		case PNUM5:
			key = NUM5; break;
		case PNUM6:
			key = NUM6; break;
		case PNUM7:
			key = NUM7; break;
		case PNUM8:
			key = NUM8; break;
		case PNUM9:
			key = NUM9; break;
		case PNUMDEC:
			key = NUMDEC; break;
		default:
			key = 0;
		}
	return key;
}

//#include <libFreeWRL.h>
void handle_Xevents(XEvent event) {

	XEvent nextevent;
	char buf[10];
	KeySym ks, ksraw, ksupper, kslower;
	KeySym *keysym;

	int keysyms_per_keycode_return;

	//int count;
	int actionKey, windex;
	int cursorStyle;
	//ppMainloop p;
	//ttglobal tg = gglobal();
	//p = (ppMainloop)tg->Mainloop.prv;
	//p->lastMouseEvent=event.type;

#ifdef VERBOSE
	switch (event.type) {
		case ConfigureNotify: printf ("Event: ConfigureNotify\n"); break;
		case ClientMessage: printf ("Event: ClientMessage\n"); break;
		case KeyPress: printf ("Event: KeyPress\n"); break;
		case KeyRelease: printf ("Event: KeyRelease\n"); break;
		case ButtonPress: printf ("Event: ButtonPress\n"); break;
		case ButtonRelease: printf ("Event: ButtonRelease\n"); break;
		case MotionNotify: printf ("Event: MotionNotify\n"); break;
		case MapNotify: printf ("Event: MapNotify\n"); break;
		case UnmapNotify: printf ("Event: *****UnmapNotify\n"); break;
		default: printf ("event, unknown %d\n", event.type);
	}
#endif
	// window is Window {aka long unsigned int} but we store it as a void *, so...
	windex = fwl_hwnd_to_windex( (void *)event.xany.window); //sets it if doesn't exist	

	switch(event.type) {
//#ifdef HAVE_NOTOOLKIT
		/* Motif, etc, usually handles this. */
		case ConfigureNotify:
			/*  printf("%s,%d ConfigureNotify  %d %d\n",__FILE__,__LINE__,event.xconfigure.width,event.xconfigure.height); */
//#ifdef STATUSBAR_HUD
//			statusbar_set_window_size(event.xconfigure.width,event.xconfigure.height);
//#else
			fwl_setScreenDim1 (event.xconfigure.width,event.xconfigure.height,windex);
//#endif
			break;
//#endif
		case ClientMessage:
			if (event.xclient.data.l[0] == WM_DELETE_WINDOW && !RUNNINGASPLUGIN) {
				#ifdef VERBOSE
				printf("---XClient sent wmDeleteMessage, quitting freewrl\n");
				#endif
				fwl_doQuit();
			}
			break;
		case KeyPress:
		case KeyRelease:
			XLookupString(&event.xkey,buf,sizeof(buf),&ks,0);
			///*  Map keypad keys in - thanks to Aubrey Jaffer.*/
			//if(0) switch(ks) {
			//	/*  the non-keyboard arrow keys*/
			//	case XK_Left: ks = XK_j; break;
			//	case XK_Right: ks = XK_l; break;
			//	case XK_Up: ks = XK_p; break;
			//	case XK_Down: ks = XK_semicolon; break;
			//	case XK_KP_0:
			//	case XK_KP_Insert:
			//		ks = XK_a; break;
			//	case XK_KP_Decimal:
			//	case XK_KP_Delete:
			//		ks = XK_z; break;
			//	case XK_KP_7:
			//	case XK_KP_Home:
			//		ks = XK_7; break;
			//	case XK_KP_9:
			//	case XK_KP_Page_Up:
			//		ks = XK_9; break;
			//	case XK_KP_8:
			//	case XK_KP_Up:
			//		ks = XK_k; break;
			//	case XK_KP_2:
			//	case XK_KP_Down:
			//		ks = XK_8; break;
			//	case XK_KP_4:
			//	case XK_KP_Left:
			//		ks = XK_u; break;
			//	case XK_KP_6:
			//	case XK_KP_Right:
			//		ks = XK_o; break;
			//	case XK_Num_Lock: ks = XK_h; break;
			//		default: break;
			//}

			/* doubt that this is necessary */
			buf[0]=(char)ks;buf[1]='\0';

			DEBUG_XEV("Key type = %s\n", (event.type == KeyPress ? "KEY PRESS" : "KEY  RELEASE"));
			//fwl_do_keyPress((char)ks,event.type);
			//ksraw = (char)buf[0];


			// deprecated:  ksraw = XKeycodeToKeysym(event.xkey.display, event.xkey.keycode, 0);

			keysym = XGetKeyboardMapping(event.xkey.display,
				event.xkey.keycode, 1, &keysyms_per_keycode_return);
			ksraw = *keysym;
			XFree(keysym);

			XConvertCase(ksraw,&kslower,&ksupper);

			ksraw = ksupper;
			if(event.type == KeyRelease && !IsModifierKey(ks)
				&& !IsFunctionKey(ks) && !IsMiscFunctionKey(ks) && !IsCursorKey(ks)){
				fwl_do_rawKeyPress((int)ks,1);
				//printf("ks=%c %d %o %x\n",ks,(int)ks,(int)ks,(int)ks);
			}
			//printf("ksraw=%c %d %o %x\n",ksraw,(int)ksraw,(int)ksraw,(int)ksraw);
			actionKey = platform2web3dActionKeyLINUX(ksraw);
			if(actionKey)
				fwl_do_rawKeyPress(actionKey,event.type+10);
			else
				fwl_do_rawKeyPress(ksraw,event.type);
			break;

		case ButtonPress:
		case ButtonRelease:
			cursorStyle = fwl_handle_mouse(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y,windex);
			setCursor(cursorStyle);
			//if(0){
			//	/* printf("got a button press or button release\n"); */
			//	/*  if a button is pressed, we should not change state,*/
			//	/*  so keep a record.*/
			//	if(handleStatusbarHud(event.type, &tg->Mainloop.clipPlane))break;
			//	if (event.xbutton.button>=5) break;  /* bounds check*/
			//	p->ButDown[p->currentCursor][event.xbutton.button] = (event.type == ButtonPress);

			//	/* if we are Not over an enabled sensitive node, and we do NOT
			//		already have a button down from a sensitive node... */
			//	/* printf("cursoroversensitive is %u lastPressedOver %u\n", p->CursorOverSensitive,p->lastPressedOver); */
			//	if ((p->CursorOverSensitive==NULL) && (p->lastPressedOver==NULL))  {
			//			p->NavigationMode=p->ButDown[p->currentCursor][1] || p->ButDown[p->currentCursor][3];
			//			handle (event.type,event.xbutton.button,
			//					(float) ((float)event.xbutton.x/tg->display.screenWidth),
			//					(float) ((float)event.xbutton.y/tg->display.screenHeight));
			//	}
			//}
			break;

		case MotionNotify:
#if KEEP_X11_INLIB
			/* printf("got a motion notify\n"); */
			/*  do we have more motion notify events queued?*/
			if (XPending(Xdpy)) {
					XPeekEvent(Xdpy,&nextevent);
					if (nextevent.type==MotionNotify) { break;
					}
			}
#endif /* KEEP_X11_INLIB */
			cursorStyle = fwl_handle_mouse(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y,windex);
			setCursor(cursorStyle);
			//if(0){

			//	/*  save the current x and y positions for picking.*/
			//	tg->Mainloop.currentX[p->currentCursor] = event.xbutton.x;
			//	tg->Mainloop.currentY[p->currentCursor] = event.xbutton.y;
			//	/* printf("navigationMode is %d\n", NavigationMode); */
			//	if(handleStatusbarHud(6, &tg->Mainloop.clipPlane))break;
			//	if (p->NavigationMode) {
			//			/*  find out what the first button down is*/
			//			count = 0;
			//			while ((count < 5) && (!p->ButDown[p->currentCursor][count])) count++;
			//			if (count == 5) return; /*  no buttons down???*/

			//			handle (event.type,(unsigned)count,
			//					(float)((float)event.xbutton.x/tg->display.screenWidth),
			//					(float)((float)event.xbutton.y/tg->display.screenHeight));
			//	}
			//}
			break;
	}
}
#endif //TRY_MAINLOOP_STUFF_HERE


#endif /* IPHONE */
