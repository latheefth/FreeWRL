/*******************************************************************
 Copyright (C) 2006 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#define ABOUT_FREEWRL "FreeWRL is a VRML/X3D Browser for OS X and Unix\n\
\nFreeWRL is maintained by:\nJohn A. Stewart and Sarah J. Dumoulin\n\
\nThanks to the Open Source community for all the help received.\n\
Communications Research Canada\n\
Ottawa, Ontario, Canada.\nhttp://www.crc.ca"

#include <headers.h>
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

 #include <GL/GLwDrawA.h> 
 #include <X11/keysym.h>
 #include <GL/gl.h>
 #include <GL/glu.h>
 #include <GL/glx.h>

int xPos = 0; 
int yPos = 0;
static int screen;
static int quadbuff_stereo_mode;


/*
   from similar code in white_dune 8-)
   test for best visual you can get
   with best attribut list
   with maximal possible colorsize
   with maximal possible depth
 */

static int legal_depth_list[] = { 32, 24, 16, 15, 8, 4, 1 };

static int  default_attributes0[] =
   {
   GLX_DEPTH_SIZE,	 24,		// JAS
   GLX_RED_SIZE,	   8,
   GLX_DOUBLEBUFFER,       GL_TRUE,
#ifdef GLX_STEREO
   GLX_STEREO,	     GL_TRUE,
#endif
   GLX_RGBA,	       GL_TRUE,
   0
   };

static int  default_attributes1[] =
   {
   GLX_DEPTH_SIZE,	 16,
   GLX_RED_SIZE,	   8,
   GLX_DOUBLEBUFFER,       GL_TRUE,
   GLX_RGBA,	       GL_TRUE,
   0
   };

static int  default_attributes2[] =
   {
   GLX_DEPTH_SIZE,	 16,
   GLX_RED_SIZE,	   8,
   GLX_RGBA,	       GL_TRUE,
   0
   };

static int  default_attributes3[] =
   {
   GLX_RGBA,	       GL_TRUE,
   0
   };


extern int	shutterGlasses; /* stereo shutter glasses */

static String defaultResources[] = {
	"*title: FreeWRL VRML X3D Browser ",
	"*freewrlDrawArea*width: 600", "*freewrlDrawArea*height: 400", NULL
};


int MainWidgetRealized = FALSE;
int msgWindowOnscreen = FALSE;
int consWindowOnscreen = FALSE;

XtAppContext freewrlXtAppContext;
Widget freewrlTopWidget, mainw, menubar,menuworkwindow;
Widget menumessagewindow = NULL;
Widget frame, freewrlDrawArea;
Widget headlightButton;
Widget collisionButton;
Widget walkButton, flyButton, examineButton;
Widget menumessageButton;
Widget consolemessageButton;
Widget consoleTextArea, consoleTextWidget;
Widget about_widget;
Widget newFileWidget;

Arg args[10];
Arg buttonArgs[10]; int buttonArgc = 0;
Arg textArgs[10]; int textArgc = 0;

#define MAXSTAT 200
float myFps = 0.0;
char myMenuStatus[MAXSTAT];

void handle_Xevents(XEvent event);
XVisualInfo *find_best_visual(int shutter,int *attributes,int len);


/* Set window variables from FreeWRL */
void setMenuButton_collision (int val) {
	XmToggleButtonSetState (collisionButton,val,FALSE);
}
void setMenuButton_headlight (int val) {
	XmToggleButtonSetState (headlightButton,val,FALSE);
}
void setMenuButton_navModes (int type) {
	int fl, ex, wa;
	fl = FALSE; ex = FALSE; wa = FALSE;
	switch(type) {
	case NONE: break;
	case EXAMINE: ex = TRUE; break;
	case WALK: wa = TRUE; break;
	case FLY: fl = TRUE; break;
	default: break;
	}
	XmToggleButtonSetState (walkButton,wa,FALSE);
	XmToggleButtonSetState (flyButton,fl,FALSE);
	XmToggleButtonSetState (examineButton,ex,FALSE);
}

void setMessageBar() {
	char fpsstr[MAXSTAT+20];
	
	if (menumessagewindow != NULL) {


		/* make up new line to display */
		if (strlen(myMenuStatus) == 0) {
			strcat (myMenuStatus, "NONE");
		}
		if (isPerlParsing() || isTextureParsing() || (!isPerlinitialized())) {
			sprintf (fpsstr, "(Loading...)  speed: %4.1f", myFps);
		} else {
			sprintf (fpsstr,"fps: %4.1f Viewpoint: %s",myFps,myMenuStatus);
		}
		XmTextSetString(menumessagewindow,fpsstr);
	}

}
void setMenuStatus(char *stat) {
	strncpy (myMenuStatus, stat, MAXSTAT);
	setMessageBar();
}
void setMenuFps (float fps) {
	myFps = fps;
	setMessageBar();
}

void setConsoleMessage (char *str) {
	/* make sure console window is on screen */
	if (!consWindowOnscreen) {
		consWindowOnscreen = TRUE;
		XtManageChild (consoleTextWidget); /* display console window */
		XmToggleButtonSetState (consolemessageButton,consWindowOnscreen,FALSE); /* display blip if on */
	}
		
	/* put the text here */
	XmTextInsert (consoleTextArea, strlen(XmTextGetString(consoleTextArea)),str);
}

/* Callbacks */
void aboutFreeWRLpopUp (Widget w, XtPointer data, XtPointer callData) { 
	XtManageChild(about_widget);
}
void freewrlHomePopup (Widget w, XtPointer data, XtPointer callData) { 
	printf ("freewrl hp pressed\n");
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
void ViewpointNext (Widget w, XtPointer data, XtPointer callData) {Next_ViewPoint();}
void ViewpointPrev (Widget w, XtPointer data, XtPointer callData) {Prev_ViewPoint();}

/* do we want a message window displaying fps, viewpoint, etc? */
void toggleMessagebar (Widget w, XtPointer data, XtPointer callData) {
	msgWindowOnscreen = !msgWindowOnscreen; /* keep track of state */
	XmToggleButtonSetState (menumessageButton,msgWindowOnscreen,FALSE); /* display blip if on */
	if (msgWindowOnscreen) XtManageChild (menumessagewindow); /* display (or not) message window */
	else XtUnmanageChild (menumessagewindow);
}

/* do we want a console window displaying errors, etc? */
void toggleConsolebar (Widget w, XtPointer data, XtPointer callData) {
	consWindowOnscreen = !consWindowOnscreen; /* keep track of state */
	XmToggleButtonSetState (consolemessageButton,consWindowOnscreen,FALSE); /* display blip if on */
	if (consWindowOnscreen) XtManageChild (consoleTextWidget); /* display (or not) console window */
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
	XtManageChild(newFileWidget);
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
	newFileWidget = XmCreateFileSelectionDialog(menubar, "select", args, 1);        
	XtAddCallback(newFileWidget, XmNokCallback, fileSelectPressed, NULL);
	XtAddCallback(newFileWidget, XmNcancelCallback, unManageMe, NULL);
	/* delete buttons not wanted */
	XtUnmanageChild ((Widget) XmSelectionBoxGetChild(newFileWidget, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(newFileWidget);


	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);
		btn = XmCreatePushButton (menupane, "Reload", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, reloadFile, NULL);
		XtManageChild (btn);
		btn = XmCreatePushButton (menupane, "New...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, newFilePopup, NULL);
		XtManageChild (btn);

		btn = XmCreatePushButton (menupane, "Quit", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, quitMenuBar, NULL);
		XtManageChild (btn);
	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "File", args, 1);
	XtManageChild (cascade);
}

/* Navigate pulldown menu */
void createNavigatePulldown() {
	Widget cascade, btn, menupane;

	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);
	
		/* Viewpoints */
		btn = XmCreatePushButton (menupane, "Next Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointNext, NULL);
		XtManageChild (btn);
		btn = XmCreatePushButton (menupane, "Prev Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointPrev, NULL);
		XtManageChild (btn);


		/* Navigation Mode Selection */
		XtManageChild(XmCreateSeparator (menupane, "sep1", NULL, 0));

		walkButton = XtCreateManagedWidget("Walk Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (walkButton, XmNvalueChangedCallback, (XtCallbackProc)WalkMode, NULL);
		XtManageChild (walkButton);

		examineButton = XtCreateManagedWidget("Examine Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (examineButton, XmNvalueChangedCallback, (XtCallbackProc)ExamineMode, NULL);
		XtManageChild (examineButton);

		flyButton = XtCreateManagedWidget("Fly Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (flyButton, XmNvalueChangedCallback, (XtCallbackProc)FlyMode, NULL);
		XtManageChild (flyButton);

		/* Headlight, Collision */
		XtManageChild(XmCreateSeparator (menupane, "sep1", NULL, 0));

		headlightButton = XtCreateManagedWidget("Headlight",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(headlightButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)Headlight, NULL);
		XtManageChild (headlightButton);

		collisionButton = XtCreateManagedWidget("Collision",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(collisionButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)Collision, NULL);
		XtManageChild (collisionButton);
	
		/* Straighten */
		XtManageChild(XmCreateSeparator (menupane, "sep1", NULL, 0));
		//btn = XmCreatePushButton (menupane, "Straighten", NULL, 0);
		//XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointStraighten, NULL);
		//XtManageChild (btn);
		menumessageButton = XtCreateManagedWidget("Message Display",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(menumessageButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)toggleMessagebar, NULL);
		XtManageChild (menumessageButton);
		consolemessageButton = XtCreateManagedWidget("Console Display",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(consolemessageButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)toggleConsolebar, NULL);
		XtManageChild (consolemessageButton);
	
	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "Navigate", args, 1);
	XtManageChild (cascade);
}

void createHelpPulldown() {
	Widget btn, menupane, cascade;
	XmString diastring;
	int ac;
	Arg args[10];


	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);

		/* Helpity stuff */
		ac = 0;
		diastring = XmStringCreateLocalized(ABOUT_FREEWRL);
		XtSetArg(args[ac], XmNmessageString, diastring); ac++;
		about_widget = XmCreateInformationDialog(menubar, "about", args, ac);        
		XmStringFree(diastring);
		XtAddCallback(about_widget, XmNokCallback, unManageMe, NULL);
		XtUnmanageChild ((Widget) XmSelectionBoxGetChild(about_widget, XmDIALOG_CANCEL_BUTTON));
		//XtUnmanageChild ((Widget) XmSelectionBoxGetChild(about_widget, XmDIALOG_HELP_BUTTON));



		btn = XmCreatePushButton (menupane, "About FreeWRL...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)aboutFreeWRLpopUp, NULL);
		XtManageChild (btn);
		btn = XmCreatePushButton (menupane, "FreeWRL Homepage...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)freewrlHomePopup, NULL);
		XtManageChild (btn);

	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "Help", args, 1);
	XtManageChild (cascade);
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
	XtManageChild (menubar);

	//menuArgc = 0;
	//XtSetArg(menuArgs[menuArgc], XmNscrolledWindowChildType, XmMESSAGE_WINDOW); menuArgc++;
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

	/* let the user ask for this one */
	XtUnmanageChild(consoleTextWidget);


	/* create the FreeWRL OpenGL drawing area, and map it. */
	freewrlDrawArea = XtVaCreateManagedWidget ("freewrlDrawArea", glwDrawingAreaWidgetClass,
			frame, GLwNvisualInfo, Xvi, 
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
			NULL);

	XtAddCallback (freewrlDrawArea, XmNresizeCallback, GLArearesize, NULL);
	XtAddCallback (freewrlDrawArea, XmNinputCallback, GLAreainput, NULL);

	XtManageChild(freewrlDrawArea);
}



void getVisual(void) {
	int *attributes = default_attributes3;
	int len=0;

	Xvi = find_best_visual(shutterGlasses,attributes,len);
	if(!Xvi) { fprintf(stderr, "No visual!\n");exit(-1);}

	if ((shutterGlasses) && (quadbuff_stereo_mode==0)) {
		fprintf(stderr, "Warning: No quadbuffer stereo visual found !");
		fprintf(stderr, "On SGI IRIX systems read 'man setmon' or 'man xsetmon'\n");
	}
}

void createGLContext(void) {
	/* create a GLX context */
	#ifdef DO_TWO_OPENGL_THREADS
	GLcx = glXCreateContext(Xdpy, Xvi, 0, GL_FALSE);
	#else
	GLcx = glXCreateContext(Xdpy, Xvi, 0, GL_TRUE);
	#endif

	if (GLcx == NULL)
	      XtAppError (freewrlXtAppContext, "could not create rendering context");

	/* we have to wait until the main widget is realized to get the GLwin */
	while (MainWidgetRealized == FALSE) {
		printf ("MainWidgetRealized = FALSE, sleeping...\n");
		sleep (1);
	}

	/* get window id for later calls - we use more window refs than widget refs */
	GLwin = XtWindow(freewrlDrawArea);
	XtManageChild(freewrlDrawArea);

	glXMakeCurrent (Xdpy, GLwin,  GLcx);

	/* Set up the OpenGL state. This'll get overwritten later... */
	glClearDepth (1.0);
	glClearColor (0.0, 0.0, 1.0, 0.0);
	glMatrixMode (GL_PROJECTION);
	glFrustum (-1.0, 1.0, -1.0, 1.0, 1.0, 20);
	glMatrixMode (GL_MODELVIEW);
}


void openMainWindow (int argc, char **argv) {
	int bestMode;
	argc = 0;

	#ifdef DO_TWO_OPENGL_THREADS
	XInitThreads();
	#endif

	/* zero status stuff */
	myMenuStatus[0] = '\0';

	freewrlTopWidget = XtAppInitialize (&freewrlXtAppContext, "FreeWRL", NULL, 0, &argc, argv, defaultResources, NULL, 0);

	Xdpy = XtDisplay (freewrlTopWidget);
	Xwin = XtWindow (freewrlTopWidget);
	bestMode = -1;
	screen = DefaultScreen(Xdpy);

	#ifdef XF86V4
		XF86VidModeGetAllModeLines(Xdpy, screen, &modeNum, &modes);

		bestMode = 0;
		for (i=0; i < modeNum; i++) {
			if ((modes[i]->hdisplay == screenWidth) && (modes[i]->vdisplay==screenHeight)) {
				bestMode = i;
				break;
			}
		}
		/* There is no mode equivalent to the geometry specified */
		if (bestMode == -1) {
			fullscreen = 0;
			printf("No video mode for geometry %d x %d found.  Please use the --geo flag to specify an appropriate geometry, or add the required video mode\n", screenWidth, screenHeight);
		}
		XF86VidModeGetViewPort(Xdpy, DefaultScreen(Xdpy), &oldx, &oldy);
	#endif

	/* Find an OpenGL-capable RGB visual with depth buffer. */
	getVisual();

	mainw = XmCreateMainWindow (freewrlTopWidget, "mainw", NULL, 0);
	XtManageChild (mainw);

	/* Create a menu bar. */
	createMenuBar();


	/* Create a framed drawing area for OpenGL rendering. */
	createDrawingFrame();

	/* Set up the application's window layout. */
/*
	XmMainWindowSetAreas (mainw, menubar, NULL, NULL, NULL, frame);
*/

	XtVaSetValues(mainw, 
		XmNworkWindow, frame,
		XmNcommandWindow,  NULL,
		XmNmenuBar, menubar,
		XmNmessageWindow, menumessagewindow,
		NULL);

	XtRealizeWidget (freewrlTopWidget);
	MainWidgetRealized = TRUE;
}

void setGeometry (const char *gstring) {
	int c;
	c = sscanf(gstring,"%dx%d+%d+%d",&screenWidth,&screenHeight,&xPos,&yPos);
}

XVisualInfo *find_best_visual(int shutter,int *attributes,int len)
{
   XVisualInfo *vi=NULL;
   int attrib;
   int startattrib=0;
   int *attrib_mem;

   attrib_mem=(int *)malloc(len*sizeof(int)+sizeof(default_attributes0));


   quadbuff_stereo_mode=0;
   if (!shutter)
      startattrib=1;
   else
      {
#     ifdef STEREOCOMMAND
      system(STEREOCOMMAND);
#     endif
      }
   for (attrib=startattrib;attrib<2;attrib++) {
      int idepth;
      for (idepth=0;idepth<sizeof(legal_depth_list)/sizeof(int);idepth++) {
	 int redsize;
	 for (redsize=8;redsize>=4;redsize--) {
	    int i;
	    int* attribs_pointer=default_attributes0;
	    int  attribs_size=sizeof(default_attributes0)/sizeof(int);
	    if (attrib==1) {
	       attribs_pointer=default_attributes1;
	       attribs_size=sizeof(default_attributes1)/sizeof(int);
	    }
	    if (attrib==2) {
	       attribs_pointer=default_attributes2;
	       attribs_size=sizeof(default_attributes2)/sizeof(int);
	    }
	    if (attrib==3) {
	       attribs_pointer=default_attributes3;
	       attribs_size=sizeof(default_attributes3)/sizeof(int);
	    }
	    attribs_pointer[1]=legal_depth_list[idepth];
	    if ((attrib==0) || (attrib==1))
	       attribs_pointer[3]=redsize;

	    for (i=0;i<len;i++)
	       attrib_mem[i]=attributes[i];
	    for (i=0;i<attribs_size;i++)
	       attrib_mem[i+len]=attribs_pointer[i];

      	    /* get an appropriate visual */
	    vi = glXChooseVisual(Xdpy, screen, attrib_mem);
	    if (vi) {
	       if (attrib==0) {
		  quadbuff_stereo_mode=1;
	       }
	    free(attrib_mem);
	    return vi;
	    }
	 }
      }
   }
   free(attrib_mem);
   return(NULL);
}


void resetGeometry() {
#ifdef XF86V4
		XF86VidModeModeInfo info;
		int oldMode;

	if (fullscreen) {
	 	XF86VidModeGetAllModeLines(Xdpy, screen, &modeNum, &modes);
 		oldMode = 0;

 		for (i=0; i < modeNum; i++) {
 			if ((modes[i]->hdisplay == oldx) && (modes[i]->vdisplay==oldy)) {
 				oldMode = i;
				break;
 			}
 		}

	 	XF86VidModeSwitchToMode(Xdpy, screen, modes[oldMode]);
	 	XF86VidModeSetViewPort(Xdpy, screen, 0, 0);
		XFlush(Xdpy);
	}

#endif
}
