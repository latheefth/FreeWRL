/*******************************************************************
 Copyright (C) 2006 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#ifdef HAVE_MOTIF

#define ABOUT_FREEWRL "FreeWRL Version %s\n\
FreeWRL is a VRML/X3D Browser for OS X and Unix\n\
\nFreeWRL is maintained by:\nJohn A. Stewart and Sarah J. Dumoulin\n\
\nContact: freewrl-06@rogers.com\n\
Telephone: +1 613-998-2079\nhttp://www.crc.ca/FreeWRL\n\
\nThanks to the Open Source community for all the help received.\n\
Communications Research Centre\n\
Ottawa, Ontario, Canada.\nhttp://www.crc.ca"



#include <headers.h>
#include <vrmlconf.h>
#include <unistd.h>
#include "OpenGL_Utils.h"
#include "Viewer.h"


 #include <stdlib.h>
 #include <stdio.h>
 #include <unistd.h>
 #include <math.h>

 #include <Xm/MainW.h>
 #include <Xm/RowColumn.h>
 #include <Xm/PushB.h>
 #include <Xm/ToggleB.h>
 #include <Xm/CascadeB.h>
 #include <Xm/Frame.h>
 #include <Xm/TextF.h>
 #include <Xm/Label.h>
 #include <Xm/Separator.h>
 #include <Xm/PanedW.h>
 #include <Xm/Text.h>
 #include <Xm/ScrolledW.h>
 #include <Xm/FileSB.h>
extern WidgetClass glwDrawingAreaWidgetClass;

 #include <X11/keysym.h>
 #include <GL/gl.h>
 #include <GL/glu.h>
 #include <GL/glx.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>

/************************************************************************

Set window variables from FreeWRL 

************************************************************************/

/* because of threading issues in Linux, if we can only use 1 thread, we
   delay setting of info until this time. */
int colbut; int colbutChanged = FALSE;
int headbut; int headbutChanged = FALSE;
int fl, ex, wa; int navbutChanged = FALSE;
int msgChanged = FALSE;
char *consMsg = NULL; int consmsgChanged = FALSE;
int localtexpri = TRUE; /* mimics textures_take_priority in CFuncs/RenderFuncs.c */
int localshapepri = TRUE; /* mimics textures_take_priority in CFuncs/RenderFuncs.c */
int cparser = FALSE; /* do not use Daniel Kraft's C parser unless clicked */
#define MAXSTAT 200
char fpsstr[MAXSTAT+20];

static String defaultResources[200];
int MainWidgetRealized = FALSE;
int msgWindowOnscreen = FALSE;
int consWindowOnscreen = FALSE;

Widget freewrlTopWidget, mainw, menubar,menuworkwindow;
Widget menumessagewindow = NULL; /* tested against NULL in code */
Widget frame, freewrlDrawArea;
Widget headlightButton;
Widget collisionButton;
Widget walkButton, flyButton, examineButton;
Widget menumessageButton;
Widget consolemessageButton;
Widget consoleTextArea;
Widget consoleTextWidget;
Widget about_widget;
Widget newFileWidget;
Widget tex128_button, tex256_button, texFull_button, texturesFirstButton, shapeThreadButton;
Widget cParserButton;

Arg args[10];
Arg buttonArgs[10]; int buttonArgc = 0;
Arg textArgs[10]; int textArgc = 0;
XtAppContext freewrlXtAppContext;
extern char myMenuStatus[];
extern float myFps;

void createMenuBar(void);
void createDrawingFrame(void);


void myXtManageChild (int c, Widget child) {
	#ifdef XTDEBUG
	printf ("at %d, managing %d\n",c, child);
	#endif

	if (child != NULL) XtManageChild (child);
}

void createMotifMainWindow() {
	mainw = XmCreateMainWindow (freewrlTopWidget, "mainw", NULL, 0);
	myXtManageChild (29,mainw);

	/* Create a menu bar. */
	createMenuBar();

	/* Create a framed drawing area for OpenGL rendering. */
	createDrawingFrame();

	/* Set up the application's window layout. */
	XtVaSetValues(mainw, 
		XmNworkWindow, frame,
		XmNcommandWindow,  NULL,
		XmNmenuBar, menubar,
		XmNmessageWindow, menumessagewindow,
		NULL);


	XtRealizeWidget (freewrlTopWidget);
	MainWidgetRealized = TRUE;

	/* let the user ask for this one We do it here, again, because on Ubuntu,
	 * this pops up on startup. Maybe because it has scrollbars, or other internal
	 * widgets? */
	XtUnmanageChild(consoleTextWidget);


	Xwin = XtWindow (freewrlTopWidget);
}

/************************************************************************

Callbacks to handle button presses, etc.

************************************************************************/

/* Callbacks */
void aboutFreeWRLpopUp (Widget w, XtPointer data, XtPointer callData) { 
	myXtManageChild(2,about_widget);
}
/* quit selected */
void quitMenuBar (Widget w, XtPointer data, XtPointer callData) { 
	doQuit();
}

void reloadFile (Widget w, XtPointer data, XtPointer callData) {
	/* ConsoleMessage ("reloading %s", BrowserFullPath); */
	/* Anchor - this is just a "replace world" call */
	Anchor_ReplaceWorld (BrowserFullPath);
}
void ViewpointFirst (Widget w, XtPointer data, XtPointer callData) {First_ViewPoint();}
void ViewpointLast (Widget w, XtPointer data, XtPointer callData) {Last_ViewPoint();}
void ViewpointNext (Widget w, XtPointer data, XtPointer callData) {Next_ViewPoint();}
void ViewpointPrev (Widget w, XtPointer data, XtPointer callData) {Prev_ViewPoint();}

void Tex128(Widget w, XtPointer data, XtPointer callData) {setTexSize(-128);};
void Tex256(Widget w, XtPointer data, XtPointer callData) {setTexSize(-256);};
void TexFull(Widget w, XtPointer data, XtPointer callData) {setTexSize(0);};
void texturesFirst(Widget w, XtPointer data, XtPointer callData) {
	/* default is set in CFuncs/RenderFuncs to be TRUE; we need to be in sync */
	localtexpri = !localtexpri;
	setTextures_take_priority (localtexpri);
}
void cParserCallback(Widget w, XtPointer data, XtPointer callData) {
	/* default is set in CFuncs/RenderFuncs to be TRUE; we need to be in sync */
	cparser = !cparser;
	setUseCParser (cparser);
}
void shapeMaker(Widget w, XtPointer data, XtPointer callData) {
	/* default is set in CFuncs/RenderFuncs to be TRUE; we need to be in sync */
	localshapepri = !localshapepri;
	setUseShapeThreadIfPossible (localshapepri);
}


/* do we want a message window displaying fps, viewpoint, etc? */
void toggleMessagebar (Widget w, XtPointer data, XtPointer callData) {
	msgWindowOnscreen = !msgWindowOnscreen; /* keep track of state */
	XmToggleButtonSetState (menumessageButton,msgWindowOnscreen,FALSE); /* display blip if on */
	if (msgWindowOnscreen) myXtManageChild (3,menumessagewindow); /* display (or not) message window */
	else XtUnmanageChild (menumessagewindow);
}

/* do we want a console window displaying errors, etc? */
void toggleConsolebar (Widget w, XtPointer data, XtPointer callData) {
	consWindowOnscreen = !consWindowOnscreen; /* keep track of state */
	XmToggleButtonSetState (consolemessageButton,consWindowOnscreen,FALSE); /* display blip if on */
	if (consWindowOnscreen) myXtManageChild (30,consoleTextWidget); /* display (or not) console window */
	else XtUnmanageChild (consoleTextWidget);
}

void WalkMode (Widget w, XtPointer data, XtPointer callData) {set_viewer_type (WALK); }
void ExamineMode (Widget w, XtPointer data, XtPointer callData) {set_viewer_type (EXAMINE);}
void FlyMode (Widget w, XtPointer data, XtPointer callData) {set_viewer_type (FLY); }
void Headlight (Widget w, XtPointer data, XtPointer callData) {toggle_headlight(); }
void Collision (Widget w, XtPointer data, XtPointer callData) {be_collision = !be_collision; }
void ViewpointStraighten (Widget w, XtPointer data, XtPointer callData) {printf ("not yet implemented\n"); }


/* file selection dialog box, ok button pressed */
void fileSelectPressed (Widget w, XtPointer data, XmFileSelectionBoxCallbackStruct *callData) {
	char *newfile;

	/* get the filename */
	XmStringGetLtoR(callData->value, 
		XmSTRING_DEFAULT_CHARSET, &newfile);

	/* Anchor - this is just a "replace world" call */
	setFullPath (newfile);
	Anchor_ReplaceWorld (BrowserFullPath);
	XtUnmanageChild(w);
}

/* file selection dialog box cancel button - just get rid of widget */
void unManageMe (Widget widget, XtPointer client_data, 
            XmFileSelectionBoxCallbackStruct *selection) {
		XtUnmanageChild(widget);}


/* new file popup - user wants to load a new file */ 
void newFilePopup(Widget cascade_button, char *text, XmPushButtonCallbackStruct *cbs) {   
	myXtManageChild(4,newFileWidget);
	XtPopup(XtParent(newFileWidget), XtGrabNone); 
}
 

/* resize, configure events */
void GLArearesize (Widget w, XtPointer data, XtPointer callData) {
	XmDrawingAreaCallbackStruct *cd = (XmDrawingAreaCallbackStruct *) callData;
	Dimension width, height;

	XtVaGetValues (w, XmNwidth, &width, XmNheight, &height, NULL);
	setScreenDim (width,height);
}

/* Mouse, keyboard input when focus is in OpenGL window. */
void GLAreainput (Widget w, XtPointer data, XtPointer callData) {
	XmDrawingAreaCallbackStruct *cd = (XmDrawingAreaCallbackStruct *) callData;
	char buf[1];
	KeySym keysym;
	int rc;

	#ifdef XTDEBUG
	printEvent(*(cd->event));
	#endif

	handle_Xevents(*(cd->event));
}


/* remove this button from this SelectionBox widget */

void removeWidgetFromSelect (Widget parent, 
	#if NeedWidePrototypes
		unsigned int 
	#else
		unsigned char
	#endif
	button) {

	Widget tmp;

	tmp = XmSelectionBoxGetChild(parent, button);
	if (tmp == NULL) {
		printf ("hmmm - button does not exist\n");
	} else {
		XtUnmanageChild(tmp);
	}
}

/* start up the browser, and point it to www.crc.ca/FreeWRL */
void freewrlHomePopup (Widget w, XtPointer data, XtPointer callData) { 
#define MAXLINE 2000
	char *browser;
	char sysline[MAXLINE];

	browser = getenv("BROWSER");
	if (browser) {
		if (strlen(browser)>(MAXLINE-30)) return;
		strcpy (sysline, browser);
	} else
		strcpy (sysline, BROWSER);
	strcat (sysline, " http://www.crc.ca/FreeWRL &");
	freewrlSystem (sysline);
}


#ifdef XTDEBUG
 /* for debugging... */
 printEvent (XEvent event) {
 
 	switch (event.type) {
 		case KeyPress: printf ("KeyPress"); break;
 		case KeyRelease: printf ("KeyRelease"); break;
 		case ButtonPress: printf ("ButtonPress"); break;
 		case ButtonRelease: printf ("ButtonRelease"); break;
 		case MotionNotify: printf ("MotionNotify"); break;
 		case EnterNotify: printf ("EnterNotify"); break;
 		case LeaveNotify: printf ("LeaveNotify"); break;
 		case FocusIn: printf ("FocusIn"); break;
 		case FocusOut: printf ("FocusOut"); break;
 		case KeymapNotify: printf ("KeymapNotify"); break;
 		case Expose: printf ("Expose"); break;
 		case GraphicsExpose: printf ("GraphicsExpose"); break;
 		case NoExpose: printf ("NoExpose"); break;
 		case VisibilityNotify: printf ("VisibilityNotify"); break;
 		case CreateNotify: printf ("CreateNotify"); break;
 		case DestroyNotify: printf ("DestroyNotify"); break;
 		case UnmapNotify: printf ("UnmapNotify"); break;
 		case MapNotify: printf ("MapNotify"); break;
 		case MapRequest: printf ("MapRequest"); break;
 		case ReparentNotify: printf ("ReparentNotify"); break;
 		case ConfigureNotify: printf ("ConfigureNotify"); break;
 		case ConfigureRequest: printf ("ConfigureRequest"); break;
 		case GravityNotify: printf ("GravityNotify"); break;
 		case ResizeRequest: printf ("ResizeRequest"); break;
 		case CirculateNotify: printf ("CirculateNotify"); break;
 		case CirculateRequest: printf ("CirculateRequest"); break;
 		case PropertyNotify: printf ("PropertyNotify"); break;
 		case SelectionClear: printf ("SelectionClear"); break;
 		case SelectionRequest: printf ("SelectionRequest"); break;
 		case SelectionNotify: printf ("SelectionNotify"); break;
 		case ColormapNotify: printf ("ColormapNotify"); break;
 		case ClientMessage: printf ("ClientMessage"); break;
 		case MappingNotify: printf ("MappingNotify"); break;
 		default :printf ("Event out of range - %d",event.type);
 	}
 
 	printf ("\n");
 }
#endif

/* File pulldown menu */
void createFilePulldown () {
	Widget menupane, btn, cascade;

	XmString mask;
	int ac;
	Arg args[10];
	   
	/* Create the FileSelectionDialog */     
	ac = 0;
	mask  = XmStringCreateLocalized("*.wrl");
	XtSetArg(args[ac], XmNdirMask, mask); ac++;

	/* newFileWidget = XmCreateFileSelectionDialog(menubar, "select", args, 1); */
	newFileWidget = XmCreateFileSelectionDialog(mainw, "select", args, 1);        

	XtAddCallback(newFileWidget, XmNokCallback, fileSelectPressed, NULL);
	XtAddCallback(newFileWidget, XmNcancelCallback, unManageMe, NULL);
	/* delete buttons not wanted */
	removeWidgetFromSelect(newFileWidget,XmDIALOG_HELP_BUTTON);
	XtUnmanageChild(newFileWidget);


	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);
		btn = XmCreatePushButton (menupane, "Reload", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, reloadFile, NULL);
		myXtManageChild (5,btn);
		btn = XmCreatePushButton (menupane, "New...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, newFilePopup, NULL);
		myXtManageChild (6,btn);

		btn = XmCreatePushButton (menupane, "Quit", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, quitMenuBar, NULL);
		myXtManageChild (7,btn);
	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "File", args, 1);
	myXtManageChild (8,cascade);
}

/* Navigate pulldown menu */
void createNavigatePulldown() {
	Widget cascade, btn, menupane;

	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);
	
		/* Viewpoints */
		btn = XmCreatePushButton (menupane, "First Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointFirst, NULL);
		myXtManageChild (30,btn);
		btn = XmCreatePushButton (menupane, "Next Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointNext, NULL);
		myXtManageChild (9,btn);
		btn = XmCreatePushButton (menupane, "Prev Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointPrev, NULL);
		myXtManageChild (10,btn);
		btn = XmCreatePushButton (menupane, "Last Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointLast, NULL);
		myXtManageChild (31,btn);


		/* Navigation Mode Selection */
		myXtManageChild(11,XmCreateSeparator (menupane, "sep1", NULL, 0));

		walkButton = XtCreateManagedWidget("Walk Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (walkButton, XmNvalueChangedCallback, (XtCallbackProc)WalkMode, NULL);
		myXtManageChild (12,walkButton);

		examineButton = XtCreateManagedWidget("Examine Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (examineButton, XmNvalueChangedCallback, (XtCallbackProc)ExamineMode, NULL);
		myXtManageChild (13,examineButton);

		flyButton = XtCreateManagedWidget("Fly Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (flyButton, XmNvalueChangedCallback, (XtCallbackProc)FlyMode, NULL);
		myXtManageChild (14,flyButton);

		/* Headlight, Collision */
		myXtManageChild(15,XmCreateSeparator (menupane, "sep1", NULL, 0));

		headlightButton = XtCreateManagedWidget("Headlight",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(headlightButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)Headlight, NULL);
		myXtManageChild (16,headlightButton);

		collisionButton = XtCreateManagedWidget("Collision",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(collisionButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)Collision, NULL);
		myXtManageChild (17,collisionButton);
	
		/* Straighten */
		/* BUTTON NOT WORKING - so make insensitive */
		XtSetArg (buttonArgs[buttonArgc], XmNsensitive, FALSE); 
		myXtManageChild(18,XmCreateSeparator (menupane, "sep1", NULL, 0));
		btn = XmCreatePushButton (menupane, "Straighten", buttonArgs, buttonArgc+1); /* NOTE THE +1 here for sensitive */
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointStraighten, NULL);
		myXtManageChild (19,btn);

		consolemessageButton = XtCreateManagedWidget("Console Display",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(consolemessageButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)toggleConsolebar, NULL);
		myXtManageChild (21,consolemessageButton);
		menumessageButton = XtCreateManagedWidget("Message Display",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(menumessageButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)toggleMessagebar, NULL);
		myXtManageChild (20,menumessageButton);
	
	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "Navigate", args, 1);
	myXtManageChild (22,cascade);
}

/* Preferences pulldown menu */
void createPreferencesPulldown() {
	Widget cascade, btn, menupane;

	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);

		/* texture size on loading */	
		myXtManageChild(11,XmCreateSeparator (menupane, "sep1", NULL, 0));

		tex128_button = XtCreateManagedWidget("128x128 Textures", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (tex128_button, XmNvalueChangedCallback, (XtCallbackProc)Tex128, NULL);
		myXtManageChild (12,tex128_button);

		tex256_button = XtCreateManagedWidget("256x256 Textures", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (tex256_button, XmNvalueChangedCallback, (XtCallbackProc)Tex256, NULL);
		myXtManageChild (13,tex256_button);

		texFull_button = XtCreateManagedWidget("Fullsize Textures", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (texFull_button, XmNvalueChangedCallback, (XtCallbackProc)TexFull, NULL);
		myXtManageChild (14,texFull_button);

		/* texture, shape compiling  */
		myXtManageChild(15,XmCreateSeparator (menupane, "sep1", NULL, 0));

		texturesFirstButton = XtCreateManagedWidget("Textures take priority",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(texturesFirstButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)texturesFirst, NULL);
		myXtManageChild (16,texturesFirstButton);
	        XmToggleButtonSetState (texturesFirstButton, localtexpri, FALSE);

		/* texture, shape compiling  */
		myXtManageChild(15,XmCreateSeparator (menupane, "sep1", NULL, 0));

		cParserButton = XtCreateManagedWidget("Experimental Parser",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(cParserButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)cParserCallback, NULL);
		myXtManageChild (16,cParserButton);
	        XmToggleButtonSetState (cParserButton, cparser, FALSE);


		/* what things can we NOT do if we dont have threads? */
		#ifndef DO_MULTI_OPENGL_THREADS
		XtSetArg (buttonArgs[buttonArgc], XmNsensitive, FALSE);  buttonArgc++;
		#endif
		shapeThreadButton = XtCreateManagedWidget("Shape maker uses thread",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(shapeThreadButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)shapeMaker, NULL);
		#ifndef DO_MULTI_OPENGL_THREADS
		buttonArgc--;
		#endif
		myXtManageChild (17,shapeThreadButton);

		#ifdef DO_MULTI_OPENGL_THREADS
	        XmToggleButtonSetState (shapeThreadButton, localshapepri, FALSE);
		#endif
	
	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "Preferences", args, 1);
	myXtManageChild (22,cascade);
}

void createHelpPulldown() {
	Widget btn, menupane, cascade;
	XmString diastring;
	int ac;
	Arg args[10];
	char ns[2000];


	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);

		/* Helpity stuff */
		ac = 0;

		sprintf (ns,ABOUT_FREEWRL,getLibVersion());
		diastring = XmStringCreateLocalized(ns);
		XtSetArg(args[ac], XmNmessageString, diastring); ac++;
		XtSetArg(args[ac], XmNmessageAlignment,XmALIGNMENT_CENTER); ac++;
		about_widget = XmCreateInformationDialog(menubar, "about", args, ac);        
		XmStringFree(diastring);
		XtAddCallback(about_widget, XmNokCallback, unManageMe, NULL);
		removeWidgetFromSelect (about_widget, XmDIALOG_CANCEL_BUTTON);
		/*
		 causes segfault on Core3 removeWidgetFromSelect (about_widget, XmDIALOG_HELP_BUTTON);
		*/


		btn = XmCreatePushButton (menupane, "About FreeWRL...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)aboutFreeWRLpopUp, NULL);
		myXtManageChild (23,btn);
		btn = XmCreatePushButton (menupane, "FreeWRL Homepage...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)freewrlHomePopup, NULL);
		myXtManageChild (24,btn);

	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "Help", args, 1);
	myXtManageChild (25,cascade);
}

/**********************************/
void createMenuBar(void) {
	Arg menuArgs[10]; int menuArgc = 0;
	Widget cascade, btn, menupane, navModeWidget;
	Widget walkButton, flyButton, examineButton;


	/* create the menu bar */
	menuArgc = 0;
	XtSetArg(menuArgs[menuArgc], XmNscrolledWindowChildType, XmMENU_BAR); menuArgc++;
	menubar = XmCreateMenuBar (mainw, "menubar", menuArgs, menuArgc);
	myXtManageChild (26,menubar);

	menumessagewindow = 
		XtVaCreateWidget ("Message:", xmTextFieldWidgetClass, mainw,
		XmNeditable, False,
		XmNmaxLength, 200,
		NULL);

	/* generic toggle button resources */
	XtSetArg(buttonArgs[buttonArgc], XmCVisibleWhenOff, TRUE); buttonArgc++;
	XtSetArg(buttonArgs[buttonArgc],XmNindicatorType,XmN_OF_MANY); buttonArgc++;

	if (!RUNNINGASPLUGIN) createFilePulldown();
	createNavigatePulldown();
	createPreferencesPulldown();
	createHelpPulldown();

}

/**********************************************************************************/
/*
	create a frame for FreeWRL, and for messages
*/
void createDrawingFrame(void) {
	/* frame holds everything here */
        frame = XtVaCreateManagedWidget("form", xmPanedWindowWidgetClass, mainw, NULL);
	consoleTextWidget = XtVaCreateManagedWidget ("console text widget", xmScrolledWindowWidgetClass, frame,
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNworkWindow, consoleTextArea,
	NULL);
	consoleTextArea = XtVaCreateManagedWidget ("console text area ", xmTextWidgetClass, consoleTextWidget,
	XmNrows, 5,
	XmNcolumns, 0,
	XmNeditable, False,
	XmNeditMode, XmMULTI_LINE_EDIT,
	NULL);

	/* create the FreeWRL OpenGL drawing area, and map it. */
	freewrlDrawArea = XtVaCreateManagedWidget ("freewrlDrawArea", glwDrawingAreaWidgetClass,
			frame, "visualInfo", Xvi, 
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
			NULL);

	XtAddCallback (freewrlDrawArea, XmNresizeCallback, GLArearesize, NULL);
	XtAddCallback (freewrlDrawArea, XmNinputCallback, GLAreainput, NULL);

	myXtManageChild(27,freewrlDrawArea);

	/* let the user ask for this one */
	XtUnmanageChild(consoleTextWidget);

}

void openMotifMainWindow (int argc, char **argv) {
	String dummyargc[] = { " ", " "};
	Arg initArgs[10]; int initArgc = 0;
	argc = 0;

	#ifdef DO_MULTI_OPENGL_THREADS
	XInitThreads();
	#endif

	XtSetArg(initArgs[initArgc],XmNlabelString,XmStringCreate("FreeWRL VRML X3D Browser",XmSTRING_DEFAULT_CHARSET)); initArgc++;
	XtSetArg(initArgs[initArgc],XmNheight, screenHeight); initArgc++;
	XtSetArg(initArgs[initArgc],XmNwidth, screenWidth); initArgc++;
	freewrlTopWidget = XtAppInitialize (&freewrlXtAppContext, "FreeWRL", NULL, 0, 
		&argc, dummyargc, NULL, initArgs, initArgc);

	Xdpy = XtDisplay (freewrlTopWidget);
	/* do not bother mapping this, if we are a plugin. */
	/* NOTE: have problems with AMD64 firefox plugin right now; just keep mapped*/
	if (!RUNNINGONAMD64) {
		if (RUNNINGASPLUGIN) {
			XtSetMappedWhenManaged (freewrlTopWidget,False);
		}
	}
}


void setConsoleMessage (char *str) {

	/* is the consoleTextWidget created yet?? */
	if (ISDISPLAYINITIALIZED != TRUE) {
		printf ("ConsoleMessage: %s\n",str);
	} else {
		/* make sure console window is on screen */
		if (!consWindowOnscreen) {
			consWindowOnscreen = TRUE;
			myXtManageChild (1,consoleTextWidget); /* display console window */
			XmToggleButtonSetState (consolemessageButton,consWindowOnscreen,FALSE); /* display blip if on */
		}
		
		/* put the text here */
		#ifdef DO_MULTI_OPENGL_THREADS
			XmTextInsert (consoleTextArea, strlen(XmTextGetString(consoleTextArea)),str);
		#else
			if (consMsg != NULL) free (consMsg);
			consMsg = strdup(str);
			consmsgChanged = TRUE;
		#endif
	}
}



void frontendUpdateButtons() {
	if (colbutChanged) {
		XmToggleButtonSetState (collisionButton,colbut,FALSE);
		colbutChanged = FALSE;
	}
	if (headbutChanged) {
		XmToggleButtonSetState (collisionButton,headbut,FALSE);
		headbutChanged = FALSE;
	}
	if (navbutChanged) {
			XmToggleButtonSetState (walkButton,wa,FALSE);
			XmToggleButtonSetState (flyButton,fl,FALSE);
			XmToggleButtonSetState (examineButton,ex,FALSE);
			navbutChanged = FALSE;
	}
	if (msgChanged) {
			XmTextSetString(menumessagewindow,fpsstr);
			msgChanged = FALSE;
	}
	if (consmsgChanged) {
			XmTextInsert (consoleTextArea, strlen(XmTextGetString(consoleTextArea)),consMsg);
			consmsgChanged = FALSE;
	}
}

void setMenuButton_collision (int val) {
		#ifdef DO_MULTI_OPENGL_THREADS
			XmToggleButtonSetState (collisionButton,val,FALSE);
		#else
			colbut = val;
			colbutChanged = TRUE;
		
		#endif
}
void setMenuButton_headlight (int val) {
		#ifdef DO_MULTI_OPENGL_THREADS
			XmToggleButtonSetState (headlightButton,val,FALSE);
		#else
			headbut = val;
			headbutChanged = TRUE;
		
		#endif
}
void setMenuButton_navModes (int type) {
		fl = FALSE; ex = FALSE; wa = FALSE;
		switch(type) {
			case NONE: break;
			case EXAMINE: ex = TRUE; break;
			case WALK: wa = TRUE; break;
			case FLY: fl = TRUE; break;
			default: break;
		}
		#ifdef DO_MULTI_OPENGL_THREADS
			XmToggleButtonSetState (walkButton,wa,FALSE);
			XmToggleButtonSetState (flyButton,fl,FALSE);
			XmToggleButtonSetState (examineButton,ex,FALSE);
		#else
			navbutChanged = TRUE;
		#endif
}

void setMenuButton_texSize (int size) {
	int val;
	/* this is called from the texture thread, so there is not a threading problem here */
	val = FALSE;
	/* set all thread buttons to FALSE */
	XmToggleButtonSetState (tex128_button, val, FALSE);
	XmToggleButtonSetState (tex256_button, val, FALSE);
	XmToggleButtonSetState (texFull_button, val, FALSE);
	val = TRUE;
	if (size <= 128) {XmToggleButtonSetState (tex128_button, val, FALSE);
	} else if (size <=256) {XmToggleButtonSetState (tex256_button, val, FALSE);
	} else {XmToggleButtonSetState (texFull_button, val, FALSE); }
}

void setMessageBar() {
	
	if (menumessagewindow != NULL) {
		/* make up new line to display */
		if (strlen(myMenuStatus) == 0) {
			strcat (myMenuStatus, "NONE");
		}
		if (isShapeCompilerParsing() || isinputThreadParsing() || isTextureParsing() || (!isInputThreadInitialized())) {
			sprintf (fpsstr, "(Loading...)  speed: %4.1f", myFps);
		} else {
			sprintf (fpsstr,"fps: %4.1f Viewpoint: %s",myFps,myMenuStatus);
		}
		#ifdef DO_MULTI_OPENGL_THREADS
			XmTextSetString(menumessagewindow,fpsstr);
		#else
			msgChanged = TRUE;
		#endif
	}

}

void  getMotifWindowedGLwin(Window *win) {
        *win = XtWindow(freewrlDrawArea);
}

int isMotifDisplayInitialized (void) {
        return  (MainWidgetRealized);
}               
  
#endif
