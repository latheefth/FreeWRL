/*******************************************************************************
 * FreeWRL Netscape Plugin, Copyright (c) 2001 CRC Canada, based on
 *
 * Simple LiveConnect Sample Plugin
 * Copyright (c) 1996 Netscape Communications. All rights reserved.
 *
 * and...
 *
 * XSwallow , by Caolan.McNamara@ul.ie
 *
 ******************************************************************************/


#include <stdio.h>
#include "npapi.h"
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>


#define IMPLEMENT_FreeWRL
#define NPDEBUG
#include "FreeWRL.h"
#define REPARENT_LOOPS 50
#define GIVEUP 4000


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/*******************************************************************************
 * Instance state information about the plugin.
 ******************************************************************************/

typedef struct _PluginInstance
{
	NPWindow*		fWindow;
	uint16			fMode;
	Window window;
	Display *display;
	uint32 x, y;
	uint32 width, height;
	Widget netscape_widget;
	XtIntervalId		swallowtimer;
	Window 			victim;
	int			fullsize;	/* window to swallow = win area */
	Widget 			resizewatch;
	int			resizeEvent;
	int			child_pid;
	char 			*fname;
	int			count; 		/* try to swallow count	*/
} PluginInstance;

int abortflag;

void resizeCB (Widget, PluginInstance *, XEvent *, Boolean *);
void DisplayJavaMessage(NPP instance, char* msg, int len);
void Redraw(Widget w, XtPointer closure, XEvent *event);
void abortswallow(int );
void abortswallowX(Widget, XtPointer , XEvent *);
void do_swallow (PluginInstance * );
void swallow_check(PluginInstance *);
int run_child (const char *,int ,int );


void resizeCB (Widget w, PluginInstance * data, XEvent * event, Boolean * cont) {
  Arg args[2];
  Widget temp;


  /* This will be "netscapeEmbed", go to "drawingArea" */
  temp = data->netscape_widget;
  while(strcmp(XtName(temp),"drawingArea")) {
    temp = XtParent(temp);
  }

  if (data->fullsize ==FALSE) {
    fprintf (stderr,"resizeCB, !!! fullsize is FALSE\n");
    XReparentWindow (data->display, data->victim, XtWindow (data->resizewatch), 0, 0);    XSync (data->display, FALSE);
  } else {
    XtSetArg (args[0], XtNwidth, (XtArgVal) &(data->width));
    XtSetArg (args[1], XtNheight, (XtArgVal) &(data->height));
    XtGetValues(temp,args,2);
    XResizeWindow (data->display, data->window, data->width, data->height);
    XResizeWindow (data->display, data->victim, data->width, data->height);
  }
}



void do_swallow (PluginInstance * This) {
  /*add a timer*/
  This->swallowtimer = XtAppAddTimeOut(XtDisplayToApplicationContext
	(This->display), 333, (XtTimerCallbackProc) swallow_check, 
	(XtPointer)This);
}


void swallow_check (PluginInstance * This)
    {
    Arg args[2];
    Widget temp;
    int i, k, l, j, m, FoundIt = FALSE;
    int width, height;
    char *windowname;
    unsigned int number_of_subkids=0, number_of_subkids2=0, number_of_kids=0, number_of_kids2=0,
      number_of_subsubkids=0, number_of_subsubkids2=0;
    Window root, parent, *children = NULL, *subchildren = NULL, *subchildren2 = NULL,
     *subsubchildren = NULL, *children2 = NULL, *subsubchildren2 = NULL;

    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    Window *win = NULL;
    Window *win2 = NULL;
    Atom _XA_WM_CLIENT_LEADER;
    XWMHints *leader_change;
    char FreeWRLName[30];

    This->swallowtimer = -1;
    sprintf (FreeWRLName,"fw%d",This->child_pid);

    if ((This->count < GIVEUP) && (abortflag == 0)) {
        This->count++;

        if (children != (Window *) NULL)
          XFree (children);
    
        /* find the number of children on the root window */
        if (0 != XQueryTree (This->display, RootWindowOfScreen (XtScreen 
    		(This->netscape_widget)), &root,
                    &parent, &children, &number_of_kids)) 
    
        /* go through the children, and see if one matches our FreeWRL win */
        for (i = 0; i < number_of_kids; i++) {
          if (0 != XFetchName (This->display, children[i], &windowname)) {
            if (!strncmp (windowname, FreeWRLName, strlen (FreeWRLName))) {
              /* Found it!!! */
              FoundIt = TRUE;
              This->victim = children[i];
            }
            XFree (windowname);
          }
    
          /* nope, go through the sub-children */
          if (FoundIt == FALSE) {
            if (subchildren != (Window *) NULL)
              XFree (subchildren);
            if (0 != XQueryTree (This->display, children[i], &root, &parent, 
    		&subchildren, &number_of_subkids))
              for (k = 0; k < number_of_subkids; k++) {
                if (0 != XFetchName (This->display, subchildren[k], &windowname)) {
                  if (!strncmp (windowname, FreeWRLName, strlen (FreeWRLName))) {
                    FoundIt = TRUE;
                    This->victim = subchildren[k];
                  }
                  XFree (windowname);
                }
                if (FoundIt == FALSE) {
                  if (subsubchildren != (Window *) NULL)
                    XFree (subsubchildren);
                  if (0 != XQueryTree (This->display, subchildren[k], &root,
                              &parent, &subsubchildren, &number_of_subsubkids))
                  for (l = 0; l < number_of_subsubkids; l++) {
                    if (0 != XFetchName (This->display, subsubchildren[l], &windowname)) {
                      if (!strncmp (windowname, FreeWRLName, strlen (FreeWRLName))) {
                        FoundIt = TRUE;
                        This->victim = subsubchildren[l];
                      }
                      XFree (windowname);
                    }
                  }
                }
              }
            }
          }
          /* still in the for loop... */
          if (FoundIt == TRUE) {
            /*search up the current tree to add a resize event handler */
            temp = XtWindowToWidget (This->display, This->window);

            /* tree is:
		
		netscape-communicator
		Navigator
		form	-------- While loop stops on this one.
		mainForm
		viewParent
		scrollerForm
		pane
		scroller
		drawingArea
		form
		pane
		scroller
		drawingArea
		form
		pane
		scroller
		drawingArea
		netscapeEmbed
	    */
		
            while (strcmp (XtName (temp), "form")) {
              temp = XtParent (temp);
              if (!(strcmp (XtName (temp), "scroller"))) {
                XtSetArg (args[0], XtNwidth, (XtArgVal) & width);
                XtSetArg (args[1], XtNheight, (XtArgVal) & height);
                XtGetValues (temp, args, 2);
                if ((width == This->width) && (height == This->height))
                  This->fullsize = TRUE;
              }
              if (!(strcmp (XtName (XtParent (temp)), "drawingArea"))) {
                temp = XtParent (temp);
	      }
            }

            /* remember - temp now points to "form" - see above tree */
	    if (This->fullsize!=TRUE)
              fprintf (stderr,"Surprise!!! This is not fullsize! \n");

            This->resizewatch = temp;
            This->resizeEvent = TRUE;

	    /* when netscape is resized, the "form" will be resized, and */
	    /* we'll get a notification via this event handler		 */
            XtAddEventHandler (This->resizewatch, StructureNotifyMask, 
    		False, (XtEventHandler) resizeCB, (XtPointer) This);

	    /* make it the right size for us; redundant if fullsize */
            XResizeWindow (This->display, This->victim, This->width, This->height);

            XSync (This->display, FALSE);

            /* still have to figure the following couple of lines out. */
            _XA_WM_CLIENT_LEADER = XInternAtom (This->display, "WM_CLIENT_LEADER", False);

            if (XGetWindowProperty ( /* Get value from property		*/

		This->display, 		/* Server connection		*/
		This->victim, 		/* Window ID, NOT Widget ID	*/
		_XA_WM_CLIENT_LEADER, 	/* Atom id'ing property name	*/
		0,			/* Offset (32 bit units to read	*/
                                        
		sizeof (Window), 	/* # of 32 bit units to read	*/
		False, 			/* delete value after read?	*/
		AnyPropertyType,	/* Requested type		*/
		&type_ret, 		/* Atom iding type actually fnd	*/
		&fmt_ret, 		/* Fmt of elements - 8,16,32	*/
		&nitems_ret, 		/* Number of items returned	*/
		&bytes_after_ret,	/* Bytes left in property	*/
		(unsigned char **) &win)/* Returend property value	*/
		 == Success) 
		if (nitems_ret > 0)
		fprintf (stderr,
			"FreeWRL Plugin: send in bug- WM_CLIENT_LEADER = %ld\n",
			nitems_ret);
		
            if (win != (Window *) NULL)
                XFree (win);
    
            XSync (This->display, FALSE);
            XWithdrawWindow (This->display, This->victim, XScreenNumberOfScreen 
    		(XtScreen (This->netscape_widget)));

            XSync (This->display, FALSE);

            XMapWindow (This->display, This->window);

            XResizeWindow (This->display, This->window, This->width, This->height);

            XSync (This->display, FALSE);

	    /* huh? One should do this... */
            for (j = 0; j < REPARENT_LOOPS; j++) {
              /* * more bloody serious dodginess */ /**/
              XReparentWindow (This->display, This->victim, This->window, 0, 0);
              XSync (This->display, FALSE);
            }
    
            XMapWindow (This->display, This->victim);

            XSync (This->display, FALSE);
 
            if (children != (Window *) NULL)
              XFree (children);
            if (subchildren != (Window *) NULL)
              XFree (subchildren);
            if (subsubchildren != (Window *) NULL)
              XFree (subsubchildren);
          } else {
            This->swallowtimer = XtAppAddTimeOut (XtDisplayToApplicationContext 
    		(This->display), 333, (XtTimerCallbackProc) swallow_check, 
    		(XtPointer) This);
       }
    } else {
	/* cant run */
      fprintf (stderr,"FreeWRL invocation can not be found\n");
    }
}

int run_child (const char *filename,int width,int height) {
  int childPID;
  int parentPID;
  int number;
  int i,j;
  char    geom[20];
  char    fname[256];
  char    childname[30];
  char    *paramline[15];         /* parameter line */

  parentPID = getpid();
  childPID = fork ();
  if (childPID == -1) {
      fprintf(stderr,"FreeWRL: Fork for plugin failed: %s\n", strerror (errno));
      childPID = 0;
  } else if (childPID == 0) {
    pid_t mine = getpid();
    if  (setpgid(mine,mine) < 0)
      fprintf(stderr,"FreeWRL child group set failed\n");
    else {
      /* freewrl cachefile -geom wwxhh -best -netscape PID */
      paramline[0] = "nice";
      paramline[1] = "freewrl";
      paramline[2] = fname;
      paramline[3] = "-geom";
      paramline[4] = geom;
      paramline[5] = "-best";
      paramline[6] = "-netscape";
      paramline[7] = childname;
      paramline[8] = NULL;
      sprintf (fname,"%s",filename);
      sprintf (geom,"%dx%d",width, height);
      sprintf (childname,"fw%d",mine);
      execvp (paramline[0], (char* const *) paramline);
    }
    fprintf(stderr,"FreeWRL Plugin: Couldnt run FreeWRL \n");
  } else {
    return(childPID);
  }
  return(-1); /*keep compiler happy*/
}

char*
NPP_GetMIMEDescription(void)
{
	return("x-world/x-vrml:wrl:FreeWRL VRML Browser");
}

#define PLUGIN_NAME		"FreeWRL VRML Browser"
#define PLUGIN_DESCRIPTION	"Implements VRML for Netscape"

NPError
NPP_GetValue(void *future, NPPVariable variable, void *value)
{
    NPError err = NPERR_NO_ERROR;
    if (variable == NPPVpluginNameString)
		*((char **)value) = PLUGIN_NAME;
    else if (variable == NPPVpluginDescriptionString)
		*((char **)value) = PLUGIN_DESCRIPTION;
    else
		err = NPERR_GENERIC_ERROR;

    return err;
}

/*******************************************************************************
 * General Plug-in Calls
 ******************************************************************************/

/*
** NPP_Initialize is called when your DLL is being loaded to do any
** DLL-specific initialization.
*/
NPError
NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}

/*
** We'll keep a global execution environment around to make our life
** simpler.
*/
JRIEnv* env;

/*
** NPP_GetJavaClass is called during initialization to ask your plugin
** what its associated Java class is. If you don't have one, just return
** NULL. Otherwise, use the javah-generated "use_" function to both
** initialize your class and return it. If you can't find your class, an
** error will be signalled by "use_" and will cause the Navigator to
** complain to the user.
*/
jref
NPP_GetJavaClass(void)
{
	struct java_lang_Class* myClass;
	env = NPN_GetJavaEnv();
	if (env == NULL)
		return NULL;		/* Java disabled */

    myClass = use_FreeWRL(env);

	if (myClass == NULL) {
		/*
		** If our class doesn't exist (the user hasn't installed it) then
		** don't allow any of the Java stuff to happen.
		*/
		env = NULL;
	}
	return myClass;
}

/*
** NPP_Shutdown is called when your DLL is being unloaded to do any
** DLL-specific shut-down. You should be a good citizen and declare that
** you're not using your java class any more. This allows java to unload
** it, freeing up memory.
*/
void
NPP_Shutdown(void)
{
	if (env)
		unuse_FreeWRL(env);
}

/*
** This function is a utility routine that calls back into Java to print
** messages to the Java Console and to stdout (via the native method,
** native_FreeWRL_printToStdout, defined below).  Sure, it's not a very
** interesting use of Java, but it gets the point across.
*/
void
DisplayJavaMessage(NPP instance, char* msg, int len)
{
	jref str, javaPeer;

	if (!env) {
		/* Java failed to initialize, so do nothing. */
		return;
	}

	if (len == -1) 
		len = strlen(msg);

	/*
    ** Use the JRI (see jri.h) to create a Java string from the input
    ** message:
    */
	str = JRI_NewStringUTF(env, msg, len);

	/*
    ** Use the NPN_GetJavaPeer operation to get the Java instance that
    ** corresponds to our plug-in (an instance of the FreeWRL class):
    */
	javaPeer = NPN_GetJavaPeer(instance);
	
	/*
    ** Finally, call our plug-in's big "feature" -- the 'doit' method,
    ** passing the execution environment, the object, and the java
    ** string:
    */
	FreeWRL_doit(env, javaPeer, str);
}

/*
** NPP_New is called when your plugin is instantiated (i.e. when an EMBED
** tag appears on a page).
*/
NPError 
NPP_New(NPMIMEType pluginType,
	NPP instance,
	uint16 mode,
	int16 argc,
	char* argn[],
	char* argv[],
	NPSavedData* saved)
{
	NPError result = NPERR_NO_ERROR;
	PluginInstance* This;
	char factString[60];

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
		
	instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));
	
	This = (PluginInstance*) instance->pdata;

	if (This == NULL)
	    return NPERR_OUT_OF_MEMORY_ERROR;

	This->fWindow = NULL;
	This->fMode = mode;
	This->window = 0;
	This->netscape_widget = NULL;
	This->swallowtimer = -1;
	This->victim = 0;
	This->fullsize = FALSE;
	This->resizeEvent = FALSE;
	This->child_pid = -1;
	This->fname = NULL;
	This->count = 0;

	/* Show off some of that Java functionality: */
	if (env) {
		jint v;

		/*
		** Call the DisplayJavaMessage utility function to cause Java to
		** write to the console and to stdout:
		*/
		DisplayJavaMessage(instance, "Hello world from npsimple!", -1); 

		/*
		** Also test out that fancy factorial method. It's a static
		** method, so we'll need to use the class object in order to call
		** it:
		*/
		v = FreeWRL_fact(env, class_FreeWRL(env), 10);
		sprintf(factString, "my favorite function returned %d\n", v);
		DisplayJavaMessage( instance, factString, -1 );
	}
	return result;
}


NPError
NPP_Destroy (NPP instance, NPSavedData ** save) {
  PluginInstance *This;

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  This = (PluginInstance *) instance->pdata;

  DisplayJavaMessage(instance, "Calling NPP_Destroy.", -1);

  if (This != NULL) {
    if (This->swallowtimer!= -1 ) 
      XtRemoveTimeOut(This->swallowtimer);   

    if (This->resizeEvent) {
      XtRemoveEventHandler (This->resizewatch, StructureNotifyMask, False, 
		(XtEventHandler) resizeCB, (XtPointer) This);
    }

    /*kill child*/
    if (This->child_pid != -1) {
      kill(This->child_pid*-1,SIGQUIT);
    }

    if (This->fname != NULL) {
      free(This->fname);
    }

		
    NPN_MemFree (instance->pdata);
    instance->pdata = NULL;
  }
  return NPERR_NO_ERROR;
}


NPError
NPP_SetWindow (NPP instance, NPWindow * window) {
  static int t;
  PluginInstance *This;

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  if (window == NULL)
    return NPERR_NO_ERROR;

  This = (PluginInstance *) instance->pdata;

  if (t == 0)
    This->window = (Window) window->window;


  This->x = window->x;
  This->y = window->y;
  This->width = window->width;
  This->height = window->height;
  This->display = ((NPSetWindowCallbackStruct *) window->ws_info)->display;

  /* the app wasnt run yet */
  if (This->window != (Window) window->window)
    fprintf (stderr, "FreeWRL Plugin: this should not be happening\n");
  else {

    /* initialize */
    This->window = (Window) window->window;
    This->netscape_widget = XtWindowToWidget (This->display, This->window);

    /* add an event handler for the "starting freewrl" window, on exposure */
    XtAddEventHandler(This->netscape_widget, ExposureMask, FALSE, 
		(XtEventHandler)Redraw, This);

    /* this one is the click to abort 
	XtAddEventHandler(This->netscape_widget, ButtonPress, FALSE, 
		(XtEventHandler)abortswallowX, This); */

    /* put a simple "starting FreeWRL" on the screen */
    Redraw(This->netscape_widget, (XtPointer)This, NULL);
  }
  return NPERR_NO_ERROR;
}


NPError 
NPP_NewStream(NPP instance,
	      NPMIMEType type,
	      NPStream *stream, 
	      NPBool seekable,
	      uint16 *stype)
{
	PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	DisplayJavaMessage(instance, "Calling NPP_NewStream.", -1); 

	/* Lets tell netscape to save this to a file */
	*stype=NP_ASFILEONLY;

	return NPERR_NO_ERROR;
}


/* PLUGIN DEVELOPERS:
 *	These next 2 functions are directly relevant in a plug-in which
 *	handles the data in a streaming manner. If you want zero bytes
 *	because no buffer space is YET available, return 0. As long as
 *	the stream has not been written to the plugin, Navigator will
 *	continue trying to send bytes.  If the plugin doesn't want them,
 *	just return some large number from NPP_WriteReady(), and
 *	ignore them in NPP_Write().  For a NP_ASFILE stream, they are
 *	still called but can safely be ignored using this strategy.
 */

int32 STREAMBUFSIZE = 0X0FFFFFFF; /* If we are reading from a file in NPAsFile
								   * mode so we can take any size stream in our
								   * write call (since we ignore it) */

int32 
NPP_WriteReady(NPP instance, NPStream *stream)
{
	PluginInstance* This;
	if (instance != NULL)
		This = (PluginInstance*) instance->pdata;

	DisplayJavaMessage(instance, "Calling NPP_WriteReady.", -1); 

	/* Number of bytes ready to accept in NPP_Write() */
	return STREAMBUFSIZE;
}


int32 
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
	if (instance != NULL)
	{
		PluginInstance* This = (PluginInstance*) instance->pdata;
	}

	DisplayJavaMessage(instance, (char*)buffer, len); 

	return len;		/* The number of bytes accepted */
}


NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
	PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
	This = (PluginInstance*) instance->pdata;

	DisplayJavaMessage(instance, "Calling NPP_DestroyStream.", -1); 

	return NPERR_NO_ERROR;
}


void  NPP_StreamAsFile (NPP instance, NPStream * stream, const char *fname)
    {
    PluginInstance *This;
    if (instance != NULL)
        This = (PluginInstance *) instance->pdata;

    /*ready to roll*/
    /*but wait until we have a window before we do the bizz*/

    abortflag=0;

    if (This->netscape_widget != NULL) {
	This->child_pid = run_child (fname,
			This->width,This->height);
	if (This->child_pid == -1)
		fprintf(stderr,"Attempt to run FreeWRL failed\n");
			
	else {
		setpgid(This->child_pid,This->child_pid);
		do_swallow (This); /* swallowtimer will be set away from -1*/
	}
    } else {
	This->swallowtimer = -2; /*inform setwindow to run it instead*/
	This->fname = (char *) malloc((strlen(fname) +1) *sizeof(char *));
	strcpy(This->fname,fname);
    }
  }


void 
NPP_StreamAsFileSAVE(NPP instance, NPStream *stream, const char* fname)
{
	PluginInstance* This;

	int	child;
	pid_t	mypid;
	char	filename[256];
	char	geom[20];
	char	PIDstring[20];
	char	eot[] = "\0";

	char 	*paramline[15]; 	/* parameter line */

	/* freewrl cachefile -geom wwxhh -best -netscape PID */
	paramline[0] = "nice";
	paramline[1] = "freewrl";
	paramline[2] = filename;
	paramline[3] = "-geom";
	paramline[4] = geom;
	paramline[5] = "-best";
	paramline[6] = "-netscape";
	paramline[7] = PIDstring;
	paramline[8] = NULL;

	if (instance != NULL)
		This = (PluginInstance*) instance->pdata;
	
	sprintf (filename,"%s",fname);
	sprintf (geom,"%dx%d",This->width, This->height);
	sprintf (PIDstring,"%d",mypid);

	child = fork();
	if (child == -1) {
		fprintf (stderr, "FreeWRL Plugin: Fork failed: %s\n",
			strerror(errno));
		child = 0;
	} else if (child == 0) {
                mypid = getpid();
                if  (setpgid(mypid,mypid) < 0)
                        fprintf(stderr,"child group set failed\n");
                else {
			This->swallowtimer = 
				XtAppAddTimeOut(XtDisplayToApplicationContext(This->display), 3000, 
				(XtTimerCallbackProc) swallow_check, (XtPointer)This);

			DisplayJavaMessage(instance, 
				"Calling NPP_StreamAsFile.", -1); 
			/* execvp (paramline[0], (char* const *) paramline); */
		}


		/* now, down here is where we should be getting the window... */
		/* should never get here... */
        	fprintf(stderr,"FreeWRL Plugin failed to execvp freewrl\n");
        } 
}


void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
	DisplayJavaMessage(instance, "Calling NPP_Print.", -1); 

	if(printInfo == NULL)
		return;

	if (instance != NULL) {
		PluginInstance* This = (PluginInstance*) instance->pdata;
	
		if (printInfo->mode == NP_FULL) {
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin would like to take over
		     *	printing completely when it is in full-screen mode,
		     *	set printInfo->pluginPrinted to TRUE and print your
		     *	plugin as you see fit.  If your plugin wants Netscape
		     *	to handle printing in this case, set
		     *	printInfo->pluginPrinted to FALSE (the default) and
		     *	do nothing.  If you do want to handle printing
		     *	yourself, printOne is true if the print button
		     *	(as opposed to the print menu) was clicked.
		     *	On the Macintosh, platformPrint is a THPrint; on
		     *	Windows, platformPrint is a structure
		     *	(defined in npapi.h) containing the printer name, port,
		     *	etc.
		     */

			void* platformPrint =
				printInfo->print.fullPrint.platformPrint;
			NPBool printOne =
				printInfo->print.fullPrint.printOne;
			
			/* Do the default*/
			printInfo->print.fullPrint.pluginPrinted = FALSE;
		} else {	/* If not fullscreen, we must be embedded */
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin is embedded, or is full-screen
		     *	but you returned false in pluginPrinted above, NPP_Print
		     *	will be called with mode == NP_EMBED.  The NPWindow
		     *	in the printInfo gives the location and dimensions of
		     *	the embedded plugin on the printed page.  On the
		     *	Macintosh, platformPrint is the printer port; on
		     *	Windows, platformPrint is the handle to the printing
		     *	device context.
		     */

			NPWindow* printWindow =
				&(printInfo->print.embedPrint.window);
			void* platformPrint =
				printInfo->print.embedPrint.platformPrint;
		}
	}
}

/*******************************************************************************
 * Define the Java native methods
 ******************************************************************************/

/* public native printToStdout(Ljava/lang/String;)V */
JRI_PUBLIC_API(void)
native_FreeWRL_printToStdout(JRIEnv* env, struct FreeWRL* self,
							struct java_lang_String * s)
{
    const char* chars = JRI_GetStringUTFChars(env, s);
    printf(chars);		/* cross-platform UI! */
}

/*******************************************************************************
// NPP_URLNotify:
// Notifies the instance of the completion of a URL request. 
// 
// NPP_URLNotify is called when Netscape completes a NPN_GetURLNotify or
// NPN_PostURLNotify request, to inform the plug-in that the request,
// identified by url, has completed for the reason specified by reason. The most
// common reason code is NPRES_DONE, indicating simply that the request
// completed normally. Other possible reason codes are NPRES_USER_BREAK,
// indicating that the request was halted due to a user action (for example,
// clicking the "Stop" button), and NPRES_NETWORK_ERR, indicating that the
// request could not be completed (for example, because the URL could not be
// found). The complete list of reason codes is found in npapi.h. 
// 
// The parameter notifyData is the same plug-in-private value passed as an
// argument to the corresponding NPN_GetURLNotify or NPN_PostURLNotify
// call, and can be used by your plug-in to uniquely identify the request. 
 ******************************************************************************/

void
NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
}

void
Redraw(Widget w, XtPointer closure, XEvent *event)
{
	PluginInstance* This = (PluginInstance*)closure;
	GC gc;
	XGCValues gcv;
	const char* text = "Starting FreeWRL";

	XtVaGetValues(w, XtNbackground, &gcv.background,
				  XtNforeground, &gcv.foreground, 0);
	gc = XCreateGC(This->display, This->window, 
				   GCForeground|GCBackground, &gcv);
	XDrawRectangle(This->display, This->window, gc, 
				   0, 0, This->width-1, This->height-1);

	XDrawString(This->display, This->window, gc, 
				This->width/2 - 100, This->height/2,
				text, strlen(text));
}

