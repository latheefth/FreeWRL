/*******************************************************************
 Copyright (C) 2005 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/


/* put a string on the console. This is fork'd by FreeWRL for error messages,
 * because when running within an HTML browser, error messages to the command
 * line get lost. 
 */


#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/Sme.h>

#define MESG "something strange happened with\nthe FreeWRL console..."
#define WINTITLE "FreeWRL X3D/VRML Console:"
#define MINTITLE "FreeWRL Console:"
#define MAXLINE 2000

/* Define an application context */
XtAppContext app_context;

/* Define the widgets */
Widget top, mainBox, messageText, dismissButton;

char inLine[MAXLINE];

/* what to do when the user hits the dismiss button */
void dismiss_proc (Widget w, XtPointer client_data, XtPointer call_data) {
	XtDestroyApplicationContext(app_context);
	exit (0);
}

int main(int argc, char **argv) {

	if (argc > 1) {
		strncpy (inLine,argv[1],strlen(argv[1]));
	} else {
		strncpy (inLine,MESG,strlen(MESG));
	}

	/* Create the application shell widget */
	top = XtVaAppInitialize(&app_context, "XViewfile", NULL, 0, &argc, argv, NULL, 
		XtNtitle,WINTITLE,
		XtNiconName,MINTITLE,
		NULL);

	/* Create a box to hold everything */
	mainBox = XtVaCreateManagedWidget("mainBox", boxWidgetClass, top, 
			NULL);

	/* Create a read only, scrollable text widget, to display the text */
	messageText = XtVaCreateManagedWidget("messageText", asciiTextWidgetClass,
		mainBox,
		XtNheight, 100,
		XtNwidth, 400,
		//XtNtype, XawAsciiFile,
		XtNtype, XawAsciiString,
		XtNscrollVertical, XawtextScrollAlways,
		XtNscrollHorizontal, XawtextScrollWhenNeeded,
		XtNstring, inLine,
		XtNlength, strlen(inLine),
		//XtNstring, argv[1],
		//XtNlength, strlen(argv[1]),
		NULL);


	/* Create a file button and file menu, with open & dismiss selections */
	dismissButton = XtVaCreateManagedWidget("dismiss_button", commandWidgetClass,
	       mainBox, XtNlabel, "Dismiss", NULL);


	/* create window and icon name */
	//if (XStringListToTextProperty(&wintitle, 1, &windowName) == 0){
	//	printf(
	//	"XStringListToTextProperty failed for %s, windowName in glpcOpenWindow.\n",
	//	wintitle);
	//} 

	/* Tie in the callbacks */
	XtAddCallback(dismissButton,XtNcallback, dismiss_proc, NULL);
	XtRealizeWidget(top);
	XtAppMainLoop(app_context);
}

