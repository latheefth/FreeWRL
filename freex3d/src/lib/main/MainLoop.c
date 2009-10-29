/*
  $Id$

  FreeWRL support library.
  Main loop : handle events, ...

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
#include <system.h>
#include <system_threads.h>
#include <system_js.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>
#include <threads.h>
#include <resources.h>

#include "../vrml_parser/Structs.h"
#include "headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../world_script/jsUtils.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "Snapshot.h"
#include "../scenegraph/LinearAlgebra.h"
#include "../scenegraph/Collision.h"

#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"
#include "../input/EAIHeaders.h"

#include "../scenegraph/Component_KeyDevice.h"	/* resolving implicit declarations */
#include "../opengl/Frustum.h"
#include "../input/InputFunctions.h"

#include "../opengl/OpenGL_Utils.h"

#ifdef AQUA
#include "../ui/aquaInt.h"
#endif

#include "MainLoop.h"


#ifdef NEW_SYNC_ROOT
#define IS_WORLD_LOADED ((root_res != NULL) && (root_res->status == ress_parsed))
#else
extern int isURLLoaded(void);	/* initial scene loaded? Robert Sim */
#endif

/* are we displayed, or iconic? */
static int onScreen = TRUE;

/* do we do event propagation, proximity calcs?? */
static int doEvents = FALSE;

#ifdef VERBOSE
static char debs[300];
#endif

char* PluginFullPath;

static int replaceWorld = FALSE;
static char  replace_name[FILENAME_MAX];

/* linewidth for lines and points - passed in on command line */
float gl_linewidth = 1.0;

/* what kind of file was just parsed? */
int currentFileVersion = 0;

/*
   we want to run initialize() from the calling thread. NOTE: if
   initialize creates VRML/X3D nodes, it will call the ProdCon methods
   to do this, and these methods will check to see if nodes, yada,
   yada, yada, until we run out of stack. So, we check to see if we
   are initializing; if so, don't worry about checking for new scripts
   any scripts to initialize here? we do it here, because we may just
   have created new scripts during  X3D/VRML parsing. Routing in the
   Display thread may have noted new scripts, but will ignore them
   until   we have told it that the scripts are initialized.  printf
   ("have scripts to initialize in EventLoop old %d new
   %d\n",max_script_found, max_script_found_and_initialized);
*/

#define INITIALIZE_ANY_SCRIPTS \
        if (max_script_found != max_script_found_and_initialized) { \
                int i; jsval retval; \
                for (i=max_script_found_and_initialized+1; i <= max_script_found; i++) { \
                        /* printf ("initializing script %d in thread %u\n",i,pthread_self());  */ \
                        JSCreateScriptContext(i); \
                        JSInitializeScriptAndFields(i); \
                        ACTUALRUNSCRIPT(i, "initialize()" ,&retval); \
                        ScriptControl[i]._initialized=TRUE; \
                        /* printf ("initialized script %d\n",i);*/  \
                } \
                max_script_found_and_initialized = max_script_found; \
        }

/* we bind bindable nodes on parse in this thread */
#define SEND_BIND_IF_REQUIRED(node) \
                if (node != NULL) { send_bind_to(X3D_NODE(node),1); node = NULL; }


int quitThread = FALSE;
char * keypress_string=NULL;            /* Robert Sim - command line key sequence */
int keypress_wait_for_settle = 100;     /* JAS - change keypress to wait, then do 1 per loop */
extern int viewer_initialized;

void Next_ViewPoint(void);              /*  switch to next viewpoint -*/
static void setup_viewpoint();
static void get_collisionoffset(double *x, double *y, double *z);

/* Sensor table. When clicked, we get back from getRayHit the fromnode,
        have to look up type and data in order to properly handle it */
struct SensStruct {
        struct X3D_Node *fromnode;
        struct X3D_Node *datanode;
        void (*interpptr)(void *, int, int, int);
};
struct SensStruct *SensorEvents = 0;
int num_SensorEvents = 0;

/* Viewport data */
GLint viewPort2[10];

/* screen width and height. */
int clipPlane = 0;

struct X3D_Node* CursorOverSensitive=NULL;      /*  is Cursor over a Sensitive node?*/
struct X3D_Node* oldCOS=NULL;                   /*  which node was cursor over before this node?*/
int NavigationMode=FALSE;               /*  are we navigating or sensing?*/
int ButDown[] = {FALSE,FALSE,FALSE,FALSE,FALSE};

int currentX, currentY;                 /*  current mouse position.*/
int lastMouseEvent = 0/*MapNotify*/;         /*  last event a mouse did; care about Button and Motion events only.*/
struct X3D_Node* lastPressedOver = NULL;/*  the sensitive node that the mouse was last buttonpressed over.*/
struct X3D_Node* lastOver = NULL;       /*  the sensitive node that the mouse was last moused over.*/
int lastOverButtonPressed = FALSE;      /*  catch the 1 to 0 transition for button presses and isOver in TouchSensors */

int maxbuffers = 1;                     /*  how many active indexes in bufferarray*/
int bufferarray[] = {GL_BACK,0};

/* current time and other time related stuff */
double TickTime;
double lastTime;
double BrowserStartTime;        /* start of calculating FPS     */
double BrowserFPS = 0.0;        /* calculated FPS               */
double BrowserSpeed = 0.0;      /* calculated movement speed    */

#undef PROFILE
#ifdef PROFILE
static double timeAA, timeA, timeB, timeC, timeD, timeE, timeF, xxf, oxf;
#endif

int trisThisLoop;

/* do we have some sensitive nodes in scene graph? */
int HaveSensitive = FALSE;

/* Function protos */
static void sendDescriptionToStatusBar(struct X3D_Node *CursorOverSensitive);
void do_keyPress(char kp, int type);
static void render_collisions(void);
static void render_pre(void);
static void render(void);
static void setup_projection(int pick, int x, int y);
void EventLoop(void);
static struct X3D_Node*  getRayHit(void);
static void get_hyperhit(void);
static void sendSensorEvents(struct X3D_Node *COS,int ev, int butStatus, int status);
static bool pluginRunning;
int isBrowserPlugin = FALSE;

#if defined(_MSC_VER)
const char *libFreeWRL_get_version()
{
 return "1.22.5"; /*Q. where do I get this function? 
		    A: the configure build process will create it automatically in internal_version.c.
		   */
}
#endif

/* stop the display thread. Used (when this comment was made) by the OSX Safari plugin; keeps
most things around, just stops display thread, when the user exits a world. */
static void stopDisplayThread()
{
	if (!TEST_NULL_THREAD(DispThrd)) {
		quitThread = TRUE;
		pthread_join(DispThrd,NULL);
		ZERO_THREAD(DispThrd);
	}
}

static double waitsec;

#if !defined(_WIN32)

static struct timeval mytime;

/* Doug Sandens windows function; lets make it static here for non-windows */
static double Time1970sec(void) {
        gettimeofday(&mytime, NULL);
        return (double) mytime.tv_sec + (double)mytime.tv_usec/1000000.0;
}

#else 

#ifdef STRANGE_FUNCTION_FROM_LIBFREEWRL_H

#include <windows.h>
__inline double Time1970sec()
{
   SYSTEMTIME mytimet; /*winNT and beyond */
   /* the windows getlocaltime has a granularity of 1ms at best. 
   There are a gazillion time functions in windows so I isolated it here in case I got it wrong*/
		/* win32 there are some higher performance timer functions (win95-vista)
		but a system might not support it - lpFrequency returns 0 if not supported
		BOOL QueryPerformanceFrequency( LARGE_INTEGER *lpFrequency );
		BOOL QueryPerformanceCounter( LARGE_INTEGER *lpPerformanceCount );
		*/

   GetLocalTime(&mytimet);
   return (double) mytimet.wHour*3600.0 + (double)mytimet.wMinute*60.0 + (double)mytimet.wSecond + (double)mytimet.wMilliseconds/1000.0;
}

#endif

#endif

/* Main eventloop for FreeWRL!!! */
void EventLoop() {

#if defined(TARGET_X11) || defined(TARGET_MOTIF)
        Cursor cursor;
#endif

        static int loop_count = 0;

#ifdef AQUA
        if (RUNNINGASPLUGIN) {
                cErr = aglSetCurrentContext(aqglobalContext);
                if (cErr == GL_FALSE) {
                        printf("set current context error!");
                }
        }

        /* window size changed by Safari? */
        if (PaneClipChanged) {
                eventLoopsetPaneClipRect( PaneClipnpx, PaneClipnpy, PaneClipfwWindow,
                        PaneClipct, PaneClipcb, PaneClipcr, PaneClipcl,
                        PaneClipwidth, PaneClipheight);
                PaneClipChanged = FALSE;
        }
#endif

#ifdef VERBOSE

#ifdef NEW_SYNC_ROOT
        printf ("start of MainLoop (parsing=%s) (url loaded=%s)\n", 
		BOOL_STR(isinputThreadParsing()), BOOL_STR(IS_WORLD_LOADED));
#else
        printf ("start of MainLoop (parsing=%s) (url loaded=%s)\n",
		BOOL_STR(isinputThreadParsing()), BOOL_STR(isURLLoaded()));
#endif

#endif

        /* should we do events, or maybe a parser is parsing? */
        doEvents = (!isinputThreadParsing()) && (!isTextureParsing()) && (!isShapeCompilerParsing()) && isInputThreadInitialized();

        /* Set the timestamp */
	TickTime = Time1970sec();

        /* any scripts to do?? */
        INITIALIZE_ANY_SCRIPTS;

        /* has the default background changed? */
        if (cc_changed) doglClearColor();

        OcclusionStartofEventLoop();
        startOfLoopNodeUpdates();

        /* First time through */
        if (loop_count == 0) {
                BrowserStartTime = TickTime;
                lastTime = TickTime;
                #ifdef PROFILE
                /* printf ("time setup for debugging\n"); */ 
                timeAA = timeA = timeB = timeC = timeD = timeE = timeF =0.0;
                #endif
        } else {
		/* calculate how much to wait so that we are running around 100fps. Adjust the constant
		   in the line below to raise/lower this frame rate */
               waitsec = 7000.0 + TickTime - lastTime;
               if (waitsec > 0.0) {
                       /* printf ("waiting %lf\n",waitsec); */
                       usleep((unsigned)waitsec);
		}

        }
        if (loop_count == 25) {

                BrowserFPS = 25.0 / (TickTime-BrowserStartTime);
                setMenuFps(BrowserFPS); /*  tell status bar to refresh, if it is displayed*/
                /* printf ("fps %f tris %d\n",BrowserFPS,trisThisLoop); */

		/* printf ("MainLoop, nearPlane %lf farPlane %lf\n",nearPlane, farPlane); */

                #ifdef PROFILE
                oxf = timeAA + timeA + timeB + timeC + timeD + timeE + timeF;
                if (oxf > 0.01) 
                printf ("fps %f times beg:%lf eve:%lf handle_tick:%lf render_pre:%lf do_first:%lf render:%lf ending:%lf\n",
                                BrowserFPS,
/*
                                timeAA,
                                timeA,timeB,
                                timeC, timeD,
                                timeE,timeF);
*/
                                timeAA/oxf*100.0,
                                timeA/oxf*100.0,timeB/oxf*100.0,
                                timeC/oxf*100.0, timeD/oxf*100.0,
                                timeE/oxf*100.0,timeF/oxf*100.0);

                #endif
                BrowserStartTime = TickTime;
                loop_count = 1;
        } else {
                loop_count++;
        }

        trisThisLoop = 0;

        #ifdef PROFILE
        gettimeofday(&mytime, NULL);
        xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
        timeAA = (double)timeAA +  (double)xxf - TickTime;
        #endif

        /* BrowserAction required? eg, anchors, etc */
        if (BrowserAction) {
                doBrowserAction ();
                BrowserAction = FALSE;  /* action complete */
        }

        if (replaceWorld) {
/*MBFILES          Anchor_ReplaceWorld(replace_name); */
		/* FIXME: implement reload fw_main_try_reload(); */
                replaceWorld= FALSE;
        }

        /* handle any events provided on the command line - Robert Sim */
        if (keypress_string && doEvents) {
                if (keypress_wait_for_settle > 0) {
                        keypress_wait_for_settle--;
                } else {
                        /* dont do the null... */
                        if (*keypress_string) {
                                /* printf ("handling key %c\n",*keypress_string); */
#if !defined( AQUA ) && !defined( WIN32 )  /*win32 - don't know whats it is suppsoed to do yet */

                                do_keyPress(*keypress_string,KeyPress);
#endif
                                keypress_string++;
                        } else {
                                keypress_string=NULL;
                        }
                }
        }

	/**
	 *   Merge of Bare X11 and Motif/X11 event handling ...
	 */
	/* REMARK: Do we want to process all pending events ? */

#if defined(TARGET_X11)
	/* We are running our own bare window */
	while (XPending(Xdpy)) {
	    XNextEvent(Xdpy, &event);
	    handle_Xevents(event);
	}
#endif

#if defined(TARGET_MOTIF)
	/* any updates to the menu buttons? Because of Linux threading
	   issues, we try to make all updates come from 1 thread */
	frontendUpdateButtons();
	
	/* do the Xt events here. */
	while (XtAppPending(Xtcx)!= 0) {
	    XAnyEvent *aev;
	    
	    XtAppNextEvent(Xtcx, &event);
	    
	    aev = &event.xany;
	    
#ifdef XEVENT_VERBOSE
	    XButtonEvent *bev;
	    XMotionEvent *mev;
	    
	    switch (event.type) {
	    case MotionNotify:
		mev = &event.xmotion;
		TRACE_MSG("mouse motion event: win=%u, state=%d\n",
			  mev->window, mev->state);
		break;
	    case ButtonPress:
	    case ButtonRelease:
		bev = &event.xbutton;
		TRACE_MSG("mouse button event: win=%u, state=%d\n",
			  bev->window, bev->state);
		break;
	    }
#endif //XEVENT_VERBOSE
	    /**
	     *   Quick hack to make events propagate
	     *   to the drawing area.... :)
	     */
	    if (aev->window == GLwin) {
		handle_Xevents(event);
	    } else {
		XtDispatchEvent (&event);
	    }
	}
#endif //defined(TARGET_MOTIF)

#if defined(TARGET_AQUA)
	/* Just guessing what would fit :P ... */
	/* handle_aqua(); */
	printf ("handling handle_aqua badly\n");
#endif

#if defined(TARGET_WIN32)
	/**
	 *   Win32 event loop
	 *   gives windows message handler a time slice and 
	 *   it calls handle_aqua and do_keypress from fwWindow32.c 
	 */
	doEventsWin32A(); 
#endif

#if 0 // was !defined( AQUA ) && !defined( WIN32 )
#ifndef HAVE_MOTIF
        while (XPending(Xdpy)) {
            XNextEvent(Xdpy, &event);
            handle_Xevents(event);
        }
#endif
#endif

        #ifdef PROFILE
        gettimeofday(&mytime, NULL);
        oxf = xxf;
        xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
        timeA = (double)timeA +  (double)xxf - oxf;
        #endif

        /* Viewer move viewpoint */
        handle_tick();

        #ifdef PROFILE
        gettimeofday(&mytime, NULL);
        oxf = xxf;
        xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
        timeB = (double)timeB +  (double)xxf - oxf;
        #endif

        /* setup Projection and activate ProximitySensors */
        if (onScreen) render_pre(); 

        #ifdef PROFILE
        gettimeofday(&mytime, NULL);
        oxf = xxf;
        xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
        timeC = (double)timeC +  (double)xxf - oxf;
        #endif


        /* first events (clock ticks, etc) if we have other things to do, yield */
        if (doEvents) do_first (); else sched_yield();

        #ifdef PROFILE
        gettimeofday(&mytime, NULL);
        oxf = xxf;
        xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
        timeD = (double)timeD +  (double)xxf - oxf;
        #endif

        /* actual rendering */
        if (onScreen)
			render();

        #ifdef PROFILE
        gettimeofday(&mytime, NULL);
        oxf = xxf;
        xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
        timeE = (double)timeE +  (double)xxf - oxf;
        #endif

        /* handle_mouse events if clicked on a sensitive node */
		/*printf("nav mode =%d sensitive= %d\n",NavigationMode, HaveSensitive); */
        if (!NavigationMode && HaveSensitive) {
                setup_projection(TRUE,currentX,currentY);
                setup_viewpoint();
                /* original: render_hier(rootNode,VF_Sensitive); */
                render_hier(rootNode,VF_Sensitive  | VF_Geom); 
                CursorOverSensitive = getRayHit();

                /* for nodes that use an "isOver" eventOut... */
                if (lastOver != CursorOverSensitive) {
                        #ifdef VERBOSE
                        printf ("%lf over changed, lastOver %u cursorOverSensitive %u, butDown1 %d\n",
				TickTime, (unsigned int) lastOver, (unsigned int) CursorOverSensitive,
				ButDown[1]);
                        #endif

                        if (ButDown[1]==0) {

                                /* ok, when the user releases a button, cursorOverSensitive WILL BE NULL
                                   until it gets sensed again. So, we use the lastOverButtonPressed flag to delay 
                                   sending this flag by one event loop loop. */
                                if (!lastOverButtonPressed) {
                                        sendSensorEvents(lastOver, overMark, 0, FALSE);
                                        sendSensorEvents(CursorOverSensitive, overMark, 0, TRUE);
                                        lastOver = CursorOverSensitive;
                                }
                                lastOverButtonPressed = FALSE;
                        } else {
                                lastOverButtonPressed = TRUE;
                        }

                }
                #ifdef VERBOSE
                if (CursorOverSensitive != NULL) 
			printf("COS %d (%s)\n",
			       (unsigned int) CursorOverSensitive,
			       stringNodeType(CursorOverSensitive->_nodeType));
                #endif

                /* did we have a click of button 1? */

                if (ButDown[1] && (lastPressedOver==NULL)) {
                        /* printf ("Not Navigation and 1 down\n"); */
                        /* send an event of ButtonPress and isOver=true */
                        lastPressedOver = CursorOverSensitive;
                        sendSensorEvents(lastPressedOver, ButtonPress, ButDown[1], TRUE);
                }

                if ((ButDown[1]==0) && lastPressedOver!=NULL) {
                        /* printf ("Not Navigation and 1 up\n");  */
                        /* send an event of ButtonRelease and isOver=true;
                           an isOver=false event will be sent below if required */
                        sendSensorEvents(lastPressedOver, ButtonRelease, ButDown[1], TRUE);
                        lastPressedOver = NULL;
                }

                if (lastMouseEvent == MotionNotify) {
                        /* printf ("Not Navigation and motion - going into sendSensorEvents\n"); */
                        /* TouchSensor hitPoint_changed needs to know if we are over a sensitive node or not */
                        sendSensorEvents(CursorOverSensitive,MotionNotify, ButDown[1], TRUE);

                        /* PlaneSensors, etc, take the last sensitive node pressed over, and a mouse movement */
                        sendSensorEvents(lastPressedOver,MotionNotify, ButDown[1], TRUE);
                }



                /* do we need to re-define cursor style?        */
                /* do we need to send an isOver event?          */
                if (CursorOverSensitive!= NULL) {
		    SENSOR_CURSOR;

                        /* is this a new node that we are now over?
                           don't change the node pointer if we are clicked down */
                        if ((lastPressedOver==NULL) && (CursorOverSensitive != oldCOS)) {
                                sendSensorEvents(oldCOS,MapNotify,ButDown[1], FALSE);
                                sendSensorEvents(CursorOverSensitive,MapNotify,ButDown[1], TRUE);
                                oldCOS=CursorOverSensitive;
                                sendDescriptionToStatusBar(CursorOverSensitive);
                        }

                } else {
                        /* hold off on cursor change if dragging a sensor */
                        if (lastPressedOver!=NULL) {
			    SENSOR_CURSOR;
                        } else {
			    ARROW_CURSOR;
                        }

                        /* were we over a sensitive node? */
                        if ((oldCOS!=NULL)  && (ButDown[1]==0)) {
                                sendSensorEvents(oldCOS,MapNotify,ButDown[1], FALSE);
                                /* remove any display on-screen */
                                sendDescriptionToStatusBar(NULL);
                                oldCOS=NULL;
                        }
                }

                /* do we have to change cursor? */
#if !defined( AQUA ) && !defined( WIN32 )


                if (cursor != curcursor) {
                        curcursor = cursor;
                        XDefineCursor (Xdpy, GLwin, cursor);
                }
#elif defined( WIN32 )
				/*win32 - dont know what goes here */
#else
                if (ccurse != ocurse) {
                        ocurse = ccurse;
                        setAquaCursor(ccurse);
                }
#endif
        }

        /* handle snapshots */
        if (doSnapshot) {
                Snapshot();
        }


        /* do OcclusionCulling, etc */
        OcclusionCulling();
        
        if (doEvents) {
                /* and just parsed nodes needing binding? */
                SEND_BIND_IF_REQUIRED(setViewpointBindInRender)
                SEND_BIND_IF_REQUIRED(setFogBindInRender)
                SEND_BIND_IF_REQUIRED(setBackgroundBindInRender)
                SEND_BIND_IF_REQUIRED(setNavigationBindInRender)


                /* handle ROUTES - at least the ones not generated in do_first() */
                propagate_events();

                /* Javascript events processed */
                process_eventsProcessed();

                /* EAI */
                handle_EAI();
		handle_MIDIEAI();
        }

        /* record the TickTime here, for rate setting. We don't do this earlier, as some
           nodes use the lastTime variable */
        lastTime = TickTime;


        #ifdef PROFILE
        gettimeofday(&mytime, NULL);
        oxf = xxf;
        xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
        timeF = (double)timeF +  (double)xxf - oxf;
        #endif

}

#if !defined( AQUA ) && !defined( WIN32 )
void handle_Xevents(XEvent event) {

        XEvent nextevent;
        char buf[10];
        KeySym ks;
        int count;

        lastMouseEvent=event.type;

        #ifdef VERBOSE
        switch (event.type) {
                case ConfigureNotify: printf ("Event: ConfigureNotify\n"); break;
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

        switch(event.type) {
                #ifdef HAVE_NOTOOLKIT
                /* Motif, etc, usually handles this. */
                case ConfigureNotify:
                        setScreenDim (event.xconfigure.width,event.xconfigure.height);
                        break;
                #endif
                case KeyPress:
                case KeyRelease:
                        XLookupString(&event.xkey,buf,sizeof(buf),&ks,0);
                        /*  Map keypad keys in - thanks to Aubrey Jaffer.*/
                        switch(ks) {
                           /*  the non-keyboard arrow keys*/
                           case XK_Left: ks = XK_j; break;
                           case XK_Right: ks = XK_l; break;
                           case XK_Up: ks = XK_p; break;
                           case XK_Down: ks = XK_semicolon; break;
                           case XK_KP_0:
                           case XK_KP_Insert:
                                ks = XK_a; break;
                           case XK_KP_Decimal:
                           case XK_KP_Delete:
                                ks = XK_z; break;
                           case XK_KP_7:
                           case XK_KP_Home:
                                 ks = XK_7; break;
                           case XK_KP_9:
                           case XK_KP_Page_Up:
                                ks = XK_9; break;
                           case XK_KP_8:
                           case XK_KP_Up:
                                ks = XK_k; break;
                           case XK_KP_2:
                           case XK_KP_Down:
                                ks = XK_8; break;
                           case XK_KP_4:
                           case XK_KP_Left:
                                ks = XK_u; break;
                           case XK_KP_6:
                           case XK_KP_Right:
                                ks = XK_o; break;
                           case XK_Num_Lock: ks = XK_h; break;
                           default: break;
                           }

                        /* doubt that this is necessary */
                        buf[0]=(char)ks;buf[1]='\0';

                        do_keyPress((char)ks,event.type);
                        break;

                case ButtonPress:
                case ButtonRelease:
                        /* printf("got a button press or button release\n"); */
                        /*  if a button is pressed, we should not change state,*/
                        /*  so keep a record.*/
                        if (event.xbutton.button>=5) break;  /* bounds check*/
                        ButDown[event.xbutton.button] = (event.type == ButtonPress);

                        /* if we are Not over an enabled sensitive node, and we do NOT
                           already have a button down from a sensitive node... */
                        /* printf("cursoroversensitive is %u lastPressedOver %u\n", CursorOverSensitive,lastPressedOver); */
                        if ((CursorOverSensitive==NULL) && (lastPressedOver==NULL))  {
                                NavigationMode=ButDown[1] || ButDown[3];
                                handle (event.type,event.xbutton.button,
                                        (float) ((float)event.xbutton.x/screenWidth),
                                        (float) ((float)event.xbutton.y/screenHeight));
                        }
                        break;

                case MotionNotify:
                        /* printf("got a motion notify\n"); */
                        /*  do we have more motion notify events queued?*/
                        if (XPending(Xdpy)) {
                                XPeekEvent(Xdpy,&nextevent);
                                if (nextevent.type==MotionNotify) { break;
                                }
                        }

                        /*  save the current x and y positions for picking.*/
                        currentX = event.xbutton.x;
                        currentY = event.xbutton.y;
                        /* printf("navigationMode is %d\n", NavigationMode); */

                        if (NavigationMode) {
                                /*  find out what the first button down is*/
                                count = 0;
                                while ((count < 5) && (!ButDown[count])) count++;
                                if (count == 5) return; /*  no buttons down???*/

                                handle (event.type,(unsigned)count,
                                        (float)((float)event.xbutton.x/screenWidth),
                                        (float)((float)event.xbutton.y/screenHeight));
                        }
                        break;
        }
}
#endif

/* get setup for rendering. */
static void render_pre() {
        /* 1. Set up projection */
        setup_projection(FALSE,0,0);


        /* 2. Headlight, initialized here where we have the modelview matrix to Identity.
        FIXME: position of light sould actually be offset a little (towards the center)
        when in stereo mode. */
        FW_GL_LOAD_IDENTITY();

        /*printf("calling get headlight in render_pre\n"); */
        if (get_headlight()) lightState(HEADLIGHT_LIGHT,TRUE);


        /* 3. Viewpoint */
        setup_viewpoint();      /*  need this to render collisions correctly*/

        /* 4. Collisions */
        if (fw_params.collision == 1) {
                render_collisions();
                setup_viewpoint(); /*  update viewer position after collision, to*/
                                   /*  give accurate info to Proximity sensors.*/
        }

        /* 5. render hierarchy - proximity */
        if (doEvents) render_hier(rootNode, VF_Proximity);

        PRINT_GL_ERROR_IF_ANY("GLBackend::render_pre");
}
void setup_projection(int pick, int x, int y) 
{
	GLsizei screenwidth2 = screenWidth;
	GLdouble aspect2 = screenRatio;
	GLint xvp = 0;
	if(Viewer.sidebyside) 
	{
		screenwidth2 = (int)((screenwidth2 * .5)+.5);
		aspect2 = aspect2 * .5;
		if(Viewer.iside == 1) xvp = (GLint)screenwidth2;
	}

        #ifdef AQUA
        if (RUNNINGASPLUGIN) {
                aglSetCurrentContext(aqglobalContext);
        } else {
                CGLSetCurrentContext(myglobalContext);
        }
        #endif

        FW_GL_MATRIX_MODE(GL_PROJECTION);
		glViewport(xvp,clipPlane,screenwidth2,screenHeight);
        FW_GL_LOAD_IDENTITY();
        if(pick) {
                /* picking for mouse events */
                glGetIntegerv(GL_VIEWPORT,viewPort2);
                gluPickMatrix((float)x,(float)viewPort2[3]-y,
                        (float)100,(float)100,viewPort2);
        }

        /* bounds check */
        if ((fieldofview <= 0.0) || (fieldofview > 180.0)) fieldofview=45.0;
        /* glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);  */
        gluPerspective(fieldofview, aspect2, nearPlane, farPlane); 

        FW_GL_MATRIX_MODE(GL_MODELVIEW);

        PRINT_GL_ERROR_IF_ANY("XEvents::setup_projection");

}


/* Render the scene */
static void render() {
        int count;
	static double shuttertime;
	static int shutterside;
        /*  profile*/
        /* double xx,yy,zz,aa,bb,cc,dd,ee,ff;*/
        /* struct timeval mytime;*/
        /* struct timezone tz; unused see man gettimeofday */

        for (count = 0; count < maxbuffers; count++) {
                /*set_buffer((unsigned)bufferarray[count],count); */              /*  in Viewer.c*/
			    Viewer.buffer = (unsigned)bufferarray[count]; /*dug9 can I go directly or is there thread issues*/
				Viewer.iside = count;
				glDrawBuffer((unsigned)bufferarray[count]);

                /*  turn lights off, and clear buffer bits*/
				if(Viewer.isStereo)
				{
					if(Viewer.shutterGlasses == 2) /* flutter mode - like --shutter but no GL_STEREO so alternates */
					{
						if(TickTime - shuttertime > 2.0)
						{
							shuttertime = TickTime;
							if(shutterside > 0) shutterside = 0;
							else shutterside = 1;
						}
						if(count != shutterside) continue;
					}
					if(Viewer.haveAnaglyphShader)
						glUseProgram(Viewer.programs[Viewer.iprog[count]]);
					setup_projection(0, 0, 0);
					if(Viewer.sidebyside && count >0)
						BackEndClearBuffer(1);
					else
						BackEndClearBuffer(2);
					setup_viewpoint(); 
				}
				else
					BackEndClearBuffer(2);
                BackEndLightsOff();
				
                /*  turn light #0 off only if it is not a headlight.*/
                if (!get_headlight()) {
                        lightState(HEADLIGHT_LIGHT,FALSE);
                }

                /*  Correct Viewpoint, only needed when in stereo mode.*/
                /* if (maxbuffers > 1) setup_viewpoint(); i think this is done above now */
				

                /*  Other lights*/
                PRINT_GL_ERROR_IF_ANY("XEvents::render, before render_hier");

                render_hier(rootNode, VF_globalLight);
                PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_globalLight)");

                /*  4. Nodes (not the blended ones)*/
                render_hier(rootNode, VF_Geom);
                PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_Geom)");

                /*  5. Blended Nodes*/
                if (have_transparency) {
                        /*  turn off writing to the depth buffer*/
                        glDepthMask(FALSE);

                        /*  render the blended nodes*/
                        render_hier(rootNode, VF_Geom | VF_Blend);

                        /*  and turn writing to the depth buffer back on*/
                        glDepthMask(TRUE);
                        PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_Geom)");
                }
				if(Viewer.isStereo)
				{
					if(Viewer.haveAnaglyphShader)
					{
						if(count==0)
						{
						   glUseProgram(0);
						   glAccum(GL_LOAD,1.0); 
						}
						else if(count==1)
						{
							glUseProgram(0);
							glAccum(GL_ACCUM,1.0); 
							glAccum(GL_RETURN,1.0);
						}
					}
				}

        }
		if(Viewer.isStereo)
		{
			Viewer.iside = Viewer.dominantEye; /*is used later in picking to set the cursor pick box on the (left=0 or right=1) viewport*/
		}

#if defined( AQUA )
	if (RUNNINGASPLUGIN) {
	    aglSetCurrentContext(aqglobalContext);
	    aglSwapBuffers(aqglobalContext);
	} else {
	    CGLError err = CGLFlushDrawable(myglobalContext);
	    if (err != kCGLNoError) printf ("CGLFlushDrawable error %d\n",err);
	    updateContext();
	}

#else

#if defined( WIN32 )

	SwapBuffers(wglGetCurrentDC());

#else

	glXSwapBuffers(Xdpy,GLwin);

#endif
#endif

        PRINT_GL_ERROR_IF_ANY("XEvents::render");
}



static void
get_collisionoffset(double *x, double *y, double *z)
{
        struct point_XYZ res = CollisionInfo.Offset;

        /* uses mean direction, with maximum distance */
        if (CollisionInfo.Count == 0) {
            *x = *y = *z = 0;
        } else {
            if (APPROX(vecnormal(&res, &res),0.0)) {
                        *x = *y = *z = 0;
            } else {
                        vecscale(&res, &res, sqrt(CollisionInfo.Maximum2));
                        *x = res.x;
                        *y = res.y;
                        *z = res.z;
                         /* printf ("get_collisionoffset, %lf %lf %lf\n",*x, *y, *z);  */
            }
        }
}

static void render_collisions() {
        struct point_XYZ v;
        CollisionInfo.Offset.x = 0;
        CollisionInfo.Offset.y = 0;
        CollisionInfo.Offset.z = 0;
        CollisionInfo.Count = 0;
        CollisionInfo.Maximum2 = 0.;

        render_hier(rootNode, VF_Collision);
        get_collisionoffset(&(v.x), &(v.y), &(v.z));

	/* if (!APPROX(v.x,0.0) || !APPROX(v.y,0.0) || !APPROX(v.z,0.0)) {
		printf ("MainLoop, rendercollisions, offset %f %f %f\n",v.x,v.y,v.z);
	} */

        increment_pos(&v);
}


static void setup_viewpoint() {
	
	/* GLdouble projMatrix[16]; */

        FW_GL_MATRIX_MODE(GL_MODELVIEW); /*  this should be assumed , here for safety.*/
        FW_GL_LOAD_IDENTITY();

        viewer_togl(fieldofview);
        render_hier(rootNode, VF_Viewpoint);
        PRINT_GL_ERROR_IF_ANY("XEvents::setup_viewpoint");

	/*
	fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	printf ("\n");
	printf ("setup_viewpoint, proj  %lf %lf %lf\n",projMatrix[12],projMatrix[13],projMatrix[14]);
	fwGetDoublev(GL_MODELVIEW_MATRIX, projMatrix);
	printf ("setup_viewpoint, model %lf %lf %lf\n",projMatrix[12],projMatrix[13],projMatrix[14]);
	printf ("setup_viewpoint, currentPos %lf %lf %lf\n",        Viewer.currentPosInModel.x, 
	        Viewer.currentPosInModel.y ,
	        Viewer.currentPosInModel.z);
	*/

}


void sendKeyToKeySensor(const char key, int upDown);
/* handle a keypress. "man freewrl" shows all the recognized keypresses */
void do_keyPress(const char kp, int type) {
        /* does this X3D file have a KeyDevice node? if so, send it to it */
        /*printf("%c%d\n",kp,type);*/
        if (KeySensorNodePresent()) {
                sendKeyToKeySensor(kp,type);
        } else {
                if (type == KeyPress) {
                        switch (kp) {
                                case 'e': { set_viewer_type (EXAMINE); break; }
                                case 'w': { set_viewer_type (WALK); break; }
                                case 'd': { set_viewer_type (FLY); break; }
                                case 'f': { set_viewer_type (EXFLY); break; }
                                case 'h': { toggle_headlight(); break;}
                                case '/': { print_viewer(); break; }
                                case 'q': { if (!RUNNINGASPLUGIN) {
                                                  doQuit();
                                            }
                                            break;
                                          }
                                case 'c': {fw_params.collision = !fw_params.collision; 
                                                setMenuButton_collision(fw_params.collision); break; }
                                case 'v': {Next_ViewPoint(); break;}
                                case 'b': {Prev_ViewPoint(); break;}
                                case 's': {setSnapshot(); break;}
                                default: {handle_key(kp);}
        
                        }
                } else {
                        handle_keyrelease(kp);
                }
        }
}

struct X3D_Node* getRayHit() {
        double x,y,z;
        int i;

        if(hpdist >= 0) {
                gluUnProject(hp.x,hp.y,hp.z,rayHit.modelMatrix,rayHit.projMatrix,viewport,&x,&y,&z);

                /* and save this globally */
                ray_save_posn.c[0] = x; ray_save_posn.c[1] = y; ray_save_posn.c[2] = z;

                /* we POSSIBLY are over a sensitive node - lets go through the sensitive list, and see
                   if it exists */

                /* is the sensitive node not NULL? */
                if (rayHit.node == NULL) return NULL;
        
                /*
                printf ("rayhit, we are over a node, have node %u (%s), posn %lf %lf %lf",
                        rayHit.node,stringNodeType(rayHit.node->_nodeType), x,y,z);
                printf (" dist %f ",rayHit.node->_dist);
                */

                for (i=0; i<num_SensorEvents; i++) {
                        if (SensorEvents[i].fromnode == rayHit.node) {
                                /* printf ("found this node to be sensitive - returning %u\n",rayHit.node); */
                                return ((struct X3D_Node*) rayHit.node);
                        }
                }
        }

        /* no rayhit, or, node was "close" (scenegraph-wise) to a sensitive node, but is not one itself */
        return(NULL);
}


/* set a node to be sensitive, and record info for this node */
void setSensitive(struct X3D_Node *parentNode, struct X3D_Node *datanode) {
        void (*myp)(unsigned *);

        switch (datanode->_nodeType) {
                /* sibling sensitive nodes - we have a parent node, and we use it! */
                case NODE_TouchSensor: myp = (void *)do_TouchSensor; break;
                case NODE_GeoTouchSensor: myp = (void *)do_GeoTouchSensor; break;
                case NODE_PlaneSensor: myp = (void *)do_PlaneSensor; break;
                case NODE_CylinderSensor: myp = (void *)do_CylinderSensor; break;
                case NODE_SphereSensor: myp = (void *)do_SphereSensor; break;
                case NODE_ProximitySensor: /* it is time sensitive only, NOT render sensitive */ return; break;
                case NODE_GeoProximitySensor: /* it is time sensitive only, NOT render sensitive */ return; break;

                /* Anchor is a special case, as it has children, so this is the "parent" node. */
                case NODE_Anchor: myp = (void *)do_Anchor; parentNode = datanode; break;
                default: return;
        }
        /* printf ("set_sensitive ,parentNode %d  type %s data %d type %s\n",parentNode,
                        stringNodeType(parentNode->_nodeType),datanode,stringNodeType (datanode->_nodeType));  */

        /* record this sensor event for clicking purposes */
        SensorEvents = REALLOC(SensorEvents,sizeof (struct SensStruct) * (num_SensorEvents+1));

        if (datanode == 0) {
                printf ("setSensitive: datastructure is zero for type %s\n",stringNodeType(datanode->_nodeType));
                return;
        }

        /* now, put the function pointer and data pointer into the structure entry */
        SensorEvents[num_SensorEvents].fromnode = parentNode;
        SensorEvents[num_SensorEvents].datanode = datanode;
        SensorEvents[num_SensorEvents].interpptr = (void *)myp;

        /* printf ("saved it in num_SensorEvents %d\n",num_SensorEvents); */
        num_SensorEvents++;
}

/* we have a sensor event changed, look up event and do it */
/* note, (Geo)ProximitySensor events are handled during tick, as they are time-sensitive only */
static void sendSensorEvents(struct X3D_Node* COS,int ev, int butStatus, int status) {
        int count;

        /* if we are not calling a valid node, dont do anything! */
        if (COS==NULL) return;

        for (count = 0; count < num_SensorEvents; count++) {
                if (SensorEvents[count].fromnode == COS) {
                        /* should we set/use hypersensitive mode? */
                        if (ev==ButtonPress) {
                                hypersensitive = SensorEvents[count].fromnode;
                                hyperhit = 0;
                        } else if (ev==ButtonRelease) {
                                hypersensitive = 0;
                                hyperhit = 0;
                        } else if (ev==MotionNotify) {
                                get_hyperhit();
                        }


                        SensorEvents[count].interpptr(SensorEvents[count].datanode, ev,butStatus, status);
                        /* return; do not do this, incase more than 1 node uses this, eg,
                                an Anchor with a child of TouchSensor */
                }
        }
}


/* If we have a sensitive node, that is clicked and moved, get the posn
   for use later                                                                */
static void get_hyperhit() {
        double x1,y1,z1,x2,y2,z2,x3,y3,z3;
        GLdouble projMatrix[16];

        fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
        gluUnProject(r1.x, r1.y, r1.z, rayHitHyper.modelMatrix,
                projMatrix, viewport, &x1, &y1, &z1);
        gluUnProject(r2.x, r2.y, r2.z, rayHitHyper.modelMatrix,
                projMatrix, viewport, &x2, &y2, &z2);
        gluUnProject(hp.x, hp.y, hp.z, rayHit.modelMatrix,
                projMatrix,viewport, &x3, &y3, &z3);

        /* printf ("get_hyperhit in VRMLC %f %f %f, %f %f %f, %f %f %f\n",*/
        /*      x1,y1,z1,x2,y2,z2,x3,y3,z3);*/

        /* and save this globally */
        hyp_save_posn.c[0] = x1; hyp_save_posn.c[1] = y1; hyp_save_posn.c[2] = z1;
        hyp_save_norm.c[0] = x2; hyp_save_norm.c[1] = y2; hyp_save_norm.c[2] = z2;
        ray_save_posn.c[0] = x3; ray_save_posn.c[1] = y3; ray_save_posn.c[2] = z3;
}

/* set stereo buffers, if required */
void setStereoBufferStyle(int itype) /*setXEventStereo()*/
{
	if(itype==0)
	{
		/* quad buffer crystal eyes style */
		bufferarray[0]=GL_BACK_LEFT;
		bufferarray[1]=GL_BACK_RIGHT;
		maxbuffers=2;
	}
	else if(itype==1)
	{
		/*sidebyside and anaglyph type*/
		bufferarray[0]=GL_BACK;
		bufferarray[1]=GL_BACK;
		maxbuffers=2;
	}
}

/* go to the first viewpoint */
void First_ViewPoint() {
        if (totviewpointnodes>=1) {

                /* whew, we have other vp nodes */
                /*
                if (currboundvpno != 0) {
                */
                        /* have to do some work */
                        send_bind_to(X3D_NODE(viewpointnodes[currboundvpno]),0);
                        currboundvpno = 0;
                        send_bind_to(X3D_NODE(viewpointnodes[currboundvpno]),1);
                /*
                }
                */
        }
}
/* go to the first viewpoint */
void Last_ViewPoint() {
        if (totviewpointnodes>=1) {
                /* whew, we have other vp nodes */
                /*
                if (currboundvpno != (totviewpointnodes-1)) {
                */
                        /* have to do some work */
                        send_bind_to(X3D_NODE(viewpointnodes[currboundvpno]),0);
                        currboundvpno = totviewpointnodes-1;
                        send_bind_to(X3D_NODE(viewpointnodes[currboundvpno]),1);
                /*
                }
                */
        }
}
/* go to the previous viewpoint */
void Prev_ViewPoint() {
        if (totviewpointnodes>=1) {
                /* whew, we have other vp nodes */
                send_bind_to(X3D_NODE(viewpointnodes[currboundvpno]),0);
                currboundvpno--;
                if (currboundvpno<0) currboundvpno=totviewpointnodes-1;
                send_bind_to(X3D_NODE(viewpointnodes[currboundvpno]),1);
        }
}

/* go to the next viewpoint */
void Next_ViewPoint() {
        if (totviewpointnodes>=1) {
                /* whew, we have other vp nodes */
                send_bind_to(X3D_NODE(viewpointnodes[currboundvpno]),0);
                currboundvpno++;
                if (currboundvpno>=totviewpointnodes) currboundvpno=0;
                send_bind_to(X3D_NODE(viewpointnodes[currboundvpno]),1);
        }
}

/* OSX plugin is telling us the id to refer to */
void setInstance (uintptr_t instance) {
        /* printf ("setInstance, setting to %u\n",instance); */
        _fw_instance = instance;
}

/* osx Safari plugin is telling us where the initial file is */
void setFullPath(const char* file) 
{
    if (!fw_params.collision) {
        char ks = 'c';
        do_keyPress(ks, KeyPress);
    }

    /* remove a FILE:// or file:// off of the front */
    file = stripLocalFileName ((char *)file);
    FREE_IF_NZ (BrowserFullPath);
    BrowserFullPath = STRDUP((char *) file);
    /* ConsoleMessage ("setBrowserFullPath is %s (%d)",BrowserFullPath,strlen(BrowserFullPath));  */
}


/* handle all the displaying and event loop stuff. */
void _displayThread()
{
/* #ifdef AQUA */
/*     if (RUNNINGASPLUGIN) { */
/*         aglSetCurrentContext(aqglobalContext); */
/*         glpOpenGLInitialize(); */
/*         new_tessellation(); */
/*         set_viewer_type(EXAMINE); */
/*     } else { */
/*         glpOpenGLInitialize(); */
/*         new_tessellation(); */
/*     } */
/* #else */
/*     /\* make the window, get the OpenGL context *\/ */
/*     openMainWindow(0, NULL); */
/*     createGLContext(); */
/*     glpOpenGLInitialize();  */
/*     new_tessellation(); */
/* #endif */

	ENTER_THREAD("display");

	/* Initialize display */
	if (!display_initialize()) {
		ERROR_MSG("initFreeWRL: error in display initialization.\n");
		exit(1);
	}

	/* Context has been created,
	   make it current to this thread */
	bind_GLcontext();

	new_tessellation();
	
	set_viewer_type(EXAMINE);
	
	viewer_postGLinit_init();
    
    /* loop and loop, and loop... */
    while (!quitThread) {
        
        /* FreeWRL SceneGraph */
        EventLoop();
        
#if 0 /* was HAVE_MOTIF */

        /* X11 Windowing calls */
        
        /* any updates to the menu buttons? Because of Linux threading
           issues, we try to make all updates come from 1 thread */
        frontendUpdateButtons();
              
        /* do the Xt events here. */
        while (XtAppPending(Xtcx)!= 0) {
		XButtonEvent *bev;
		XMotionEvent *mev;

            XtAppNextEvent(Xtcx, &event);
	    switch (event.type) {
	    case MotionNotify:
		    mev = &event.xmotion;
		    TRACE_MSG("mouse motion event: win=%u, state=%d\n",
			      mev->window, mev->state);
		    break;
	    case ButtonPress:
	    case ButtonRelease:
		    bev = &event.xbutton;
		    TRACE_MSG("mouse button event: win=%u, state=%d\n",
			      bev->window, bev->state);
		    break;
	    }
            XtDispatchEvent (&event);
        }
#endif
    }
    
#ifndef AQUA
    if (fullscreen) resetGeometry();
#endif
}

#ifdef AQUA
void initGL() {
        /* printf ("initGL called\n"); */
        if (RUNNINGASPLUGIN) {
                //aqglobalContext = aglGetCurrentContext();
        /* printf ("initGL - runningasplugin...\n"); */
                pluginRunning = TRUE;
                aglSetCurrentContext(aqglobalContext);
        } else {
                myglobalContext = CGLGetCurrentContext();
        }
        /* printf ("initGL call finished\n"); */
}

int getOffset() {
        return offsetof(struct X3D_Group, children);
}

void setCurXY(int cx, int cy) {
	/* printf ("setCurXY, have %d %d\n",currentX,currentY); */
        currentX = cx;
        currentY = cy;
}

void setButDown(int button, int value) {
        ButDown[button] = value;
}

#endif


void setLastMouseEvent(int etype) {
        lastMouseEvent = etype;
}

void initialize_parser()
{
/*         threadmsg = "event loop"; */
        quitThread = FALSE;


/* MB: display thread init: if (TEST_NULL_THREAD(DispThrd)) { */

	/* create the root node */
	if (rootNode == NULL) {
		rootNode = createNewX3DNode (NODE_Group);	
		/*remove this node from the deleting list*/
		doNotRegisterThisNodeForDestroy(rootNode);
	}

/*         } */
}

void setSnapSeq() {
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */
        snapsequence = TRUE;
#endif
}

void closeFreewrl() {
        struct Multi_Node* tn;
        struct X3D_Group* rn;
        printf ("closeFreewrl called\n"); 

        #ifdef AQUA
        pluginRunning = FALSE;
        kill_clockEvents();
        EAI_killBindables();
        kill_bindables();
        kill_routing();
        kill_status();
        kill_openGLTextures();
        kill_javascript();
	kill_shadowFileTable();

        #endif
        /* kill any remaining children */
        rn = (struct X3D_Group*) rootNode;
        tn =  &(rn->children);
        tn->n = 0;
        quitThread = TRUE;
        viewer_initialized = FALSE;

        if (!RUNNINGASPLUGIN) {
                set_viewer_type (EXAMINE);
        }
        glFlush();
        glFinish();
        screenWidth = screenHeight = 1;
        clipPlane = 0;
        /* printf ("closeFreewrl call finished\n"); */
}

void setEAIport(int pnum) {
        EAIport = pnum;
}

void setWantEAI(int flag) {
        EAIwanted = TRUE;
}

void setLineWidth(float lwidth) {
        gl_linewidth = lwidth;
}

void setUseShapeThreadIfPossible(int x) {
#ifdef DO_MULTI_OPENGL_THREADS
	useShapeThreadIfPossible = x;
#endif
}

void setTextures_take_priority (int x) {
        textures_take_priority = x;
}

/* set the opengl_has_textureSize. Expect a number that is 0 - use max, or negative. eg,
   -512 hopefully sets to size 512x512; this will be bounds checked in the texture
   thread */
void setTexSize(int requestedsize) {
        opengl_has_textureSize = requestedsize;
}

void setNoCollision() {
        fw_params.collision = 0;
        setMenuButton_collision(fw_params.collision);
}

void setKeyString(const char* kstring)
{
    keypress_string = strdup(kstring);
}

void setSeqFile(const char* file)
{
#if defined(DOSNAPSEQUENCE)
    /* need to re-implement this for OSX generating QTVR */
    snapseqB = strdup(file);
    printf("snapseqB is %s\n", snapseqB);
#else
    WARN_MSG("Call to setSeqFile when Snapshot Sequence not compiled in.\n");
#endif
}

void setSnapFile(const char* file)
{
    snapsnapB = strdup(file);
    TRACE_MSG("snapsnapB set to %s\n", snapsnapB);
}

void setMaxImages(int max)
{
#if defined(DOSNAPSEQUENCE)
    /* need to re-implement this for OSX generating QTVR */
    if (max <=0)
	max = 100;
    maxSnapImages = max;
#else
    WARN_MSG("Call to setMaxImages when Snapshot Sequence not compiled in.\n");
#endif
}

void setSnapTmp(const char* file)
{
    seqtmp = strdup(file);
    TRACE_MSG("seqtmp set to %s\n", seqtmp);
}

/* if we had an exit(EXIT_FAILURE) anywhere in this C code - it means
   a memory error. So... print out a standard message to the
   console. */
void outOfMemory(const char *msg) {
        ConsoleMessage ("FreeWRL has encountered a memory allocation problem\n"\
                        "and is exiting.\nPlease email this file to freewrl-09@rogers.com\n -- %s--",msg);
        usleep(10 * 1000);
        exit(EXIT_FAILURE);
}

/* quit key pressed, or Plugin sends SIGQUIT */
void doQuit()
{
    stopDisplayThread();

    kill_oldWorld(TRUE,TRUE,TRUE,__FILE__,__LINE__);

    /* set geometry to normal size from fullscreen */
#ifndef AQUA
    resetGeometry();
#endif

    /* kill any remaining children */
    killErrantChildren();
    
#ifdef DEBUG_MALLOC
    void scanMallocTableOnQuit(void);
    scanMallocTableOnQuit();
#endif

    exit(EXIT_SUCCESS);
}

void freewrlDie (const char *format) {
        ConsoleMessage ("Catastrophic error: %s\n",format);
        doQuit();
}

#if defined(AQUA) || defined(WIN32)

/* MIMIC what happens in handle_Xevents, but without the X events */
void handle_aqua(const int mev, const unsigned int button, int x, int y) {
        int count;

         /* printf ("handle_aqua in MainLoop; but %d x %d y %d screenWidth %d screenHeight %d",
                button, x,y,screenWidth,screenHeight); 
        if (mev == ButtonPress) printf ("ButtonPress\n");
        else if (mev == ButtonRelease) printf ("ButtonRelease\n");
        else if (mev == MotionNotify) printf ("MotionNotify\n");
        else printf ("event %d\n",mev);  */

        /* save this one... This allows Sensors to get mouse movements if required. */
        lastMouseEvent = mev;

        if ((mev == ButtonPress) || (mev == ButtonRelease)) {
                /* record which button is down */
                ButDown[button] = (mev == ButtonPress);

                /* if we are Not over an enabled sensitive node, and we do NOT already have a 
                   button down from a sensitive node... */

                if ((CursorOverSensitive ==NULL) && (lastPressedOver ==NULL)) {
                        NavigationMode=ButDown[1] || ButDown[3];
                        handle(mev, button, (float) ((float)x/screenWidth), (float) ((float)y/screenHeight));
                }
        }

        if (mev == MotionNotify) {

                /* save the current x and y positions for picking. */
                currentX = x;
                currentY = y;

                if (NavigationMode) {
                        /* find out what the first button down is */
                        count = 0;
                        while ((count < 5) && (!ButDown[count])) count++;
                        if (count == 5) return; /* no buttons down???*/

                        handle (mev, (unsigned) count, (float) ((float)x/screenWidth), (float) ((float)y/screenHeight));
                }
        }
}
#endif
#ifdef AQUA
void setIsPlugin() {

        RUNNINGASPLUGIN = TRUE;
        setUseShapeThreadIfPossible(0);
                
        // Save local working directory
        /*
        {
        FILE* tmpfile;
        char tmppath[512];
        system("pwd > /tmp/freewrl_filename");
        tmpfile = fopen("/tmp/freewrl_filename", "r");
        
        if (tmpfile) {
                fgets(tmppath, 512, tmpfile);
        }
        BrowserFullPath = STRDUP(tmppath);      
ConsoleMessage ("setIsPlugin, BrowserFullPath :%s:");
        fclose(tmpfile);
        //system("rm /tmp/freewrl_filename");   
        tmpfile = fopen("/tmp/after", "w");
        if (tmpfile) {
                fprintf(tmpfile, "%s\n", BrowserFullPath);
        }
        fclose(tmpfile);
        }
        */
        
}
void createContext(CGrafPtr grafPtr) {
        AGLPixelFormat  fmt;
        GLboolean      mkc, ok;
        const GLint    attribWindow[]   = {AGL_RGBA, AGL_DOUBLEBUFFER, AGL_NO_RECOVERY, AGL_ALL_RENDERERS, AGL_ACCELERATED, AGL_DEPTH_SIZE, 24, AGL_STENCIL_SIZE, 8, AGL_NONE};
        AGLDrawable             aglWin;

        /* printf ("createContext called\n"); */
        if (aqglobalContext) {
                /* printf ("FreeWRL: createContext already made\n"); */
        /* printf ("FreeWRL: createContext already made\n");  */
                aglUpdateContext(aqglobalContext);
                return;
        }

        gGDevice = GetMainDevice();
        fmt = aglChoosePixelFormat(&gGDevice, 1, attribWindow);

        if ((fmt == NULL) || (aglGetError() != AGL_NO_ERROR)) {
                printf("FreeWRL: aglChoosePixelFormat failed!\n");
        }

        aqglobalContext = aglCreateContext(fmt, nil);
        if ((aqglobalContext == nil) || (aglGetError() != AGL_NO_ERROR)) {
                printf("FreeWRL: aglCreateContext failed!\n");
        }

        aglWin = (AGLDrawable)grafPtr;
        ok = aglSetDrawable(aqglobalContext, aglWin);

        if ((!ok) || (aglGetError() != AGL_NO_ERROR)) {
                if (aglGetError() == AGL_BAD_ALLOC) {
                        printf("FreeWRL: Not enough VRAM to initialize the draw context.\n");
                } else {
                        printf("FreeWRL: OGL_InitDrawContext: aglSetDrawable failed!\n");
                }
        }

        mkc = aglSetCurrentContext(aqglobalContext);
        if ((mkc == 0) || (aglGetError() != AGL_NO_ERROR)) {
                printf("FreeWRL: aglSetCurrentContext failed!\n");
        }

        aglDestroyPixelFormat(fmt);

        //sprintf(debs, "Created context: %p", aqglobalContext);
        //debug_print(debs);

        pluginRunning = TRUE;
        /* printf ("createContext call finished\n"); */
}



void setPaneClipRect(int npx, int npy, WindowPtr fwWindow, int ct, int cb, int cr, int cl, int width, int height) {
        /* record these items so that they get handled in the display thread */
        PaneClipnpx = npx;
        PaneClipnpy = npy;
        PaneClipfwWindow = fwWindow;
        PaneClipct = ct;
        PaneClipcb = cb;
        PaneClipcr = cr;
        PaneClipcl = cl;
        PaneClipwidth = width;
        PaneClipheight = height;

        PaneClipChanged = TRUE;
}

void eventLoopsetPaneClipRect(int npx, int npy, WindowPtr fwWindow, int ct, int cb, int cr, int cl, int width, int height) {
        GLint   bufferRect[4];
        Rect    r;
        int     x,y;
        int     clipHeight;
        int     windowHeight;

        #ifdef VERBOSE
        sprintf(debs, "eventLoopPaneClipRect npx %d npy %d ct %d cb %d cr %d cl %d width %d height %d\n", 
                npx, npy, ct, cb, cr, cl, width, height);
        debug_print(debs);
        #endif

        if (aqglobalContext == nil) return;

        if (!pluginRunning) return;

        cErr = aglSetCurrentContext(aqglobalContext);
        if (cErr == GL_FALSE) {
                printf("FreeWRL: EventLoopPaneClipRect: set current context error!\n");
                return;
        }

        x = npx;


        #ifdef VERBOSE
        debug_print("get window bounds");
        #endif

        GetWindowBounds(fwWindow, kWindowContentRgn, &r);               // get size of actual Mac window

        windowHeight = r.bottom - r.top;

        #ifdef VERBOSE
        sprintf (debs,"window from getWindowBounds, t %d b %d l %d r %d\n",r.top,r.bottom,r.left,r.right);
        debug_print(debs);
        #endif

        clipPlane = cb - npy;
        y = windowHeight - npy - clipPlane;

        clipHeight = cb - ct;

        bufferRect[0] = x;
        bufferRect[1] = y;
        bufferRect[2] = cr - x;
        bufferRect[3] = clipHeight;

        #ifdef VERBOSE
        sprintf (debs,"setting bufferRect  to %d %d %d %d\n",bufferRect[0],bufferRect[1],bufferRect[2],bufferRect[3]);
        debug_print(debs);
        sprintf (debs,"but, screen width/height is %d %d\n",width,height); debug_print(debs);
        #endif

        if ((width != bufferRect[2]) || (height != bufferRect[3])) {
                #ifdef VERBOSE
                debug_print("DIFFERENCE IN SIZES! choosing the largest \n");
                #endif

                if (bufferRect[2] > width) width = bufferRect[2];
                if (bufferRect[3] > height) height = bufferRect[3];
        } else {
                setScreenDim(width, height);

                #ifdef VERBOSE
                debug_print("calling agl buffer rect ... ");
                #endif

                aglSetInteger (aqglobalContext, AGL_BUFFER_RECT, bufferRect);
        }


        /* ok to here... */
        aglEnable (aqglobalContext, AGL_BUFFER_RECT);
        clipPlane = y - npy;
        clipPlane += cb - height;

        clipPlane -= (r.bottom - cb);
        clipPlane += r.top;

        #ifdef VERBOSE
        sprintf(debs, "leaving set clip - set cp to %d\n", clipPlane);
        debug_print(debs);
        #endif
}

/* make a disposeContext but without some of the node destroys. */
void Safari_disposeContext() {
        /* printf ("Safari_disposeContext called\n"); */


        stopDisplayThread();

        cErr = aglSetCurrentContext(nil);
        if (cErr == GL_FALSE) {
                printf("FreeWRL: set current context error!\n");
        }
        cErr = aglSetDrawable(aqglobalContext, nil);
        if (cErr == GL_FALSE) {
                printf("FreeWRL: set current context error!\n");
        }
        cErr = aglDestroyContext(aqglobalContext);
        if (cErr == GL_FALSE) {
                printf("FreeWRL: set current context error!\n");
        }
        aqglobalContext = nil;
        /* printf ("Safari_disposeContext call finished\n"); */
}

/* older code - is this called from the front end? keep it around until
verified that it is no longer required: */

void disposeContext() {
        //debug_print("called dispose context");
        //sprintf(debs, "context is currently %p\n", aqglobalContext);
        //debug_print(debs);

        stopDisplayThread();

        kill_X3DDefs();
        closeFreewrl();

        cErr = aglSetCurrentContext(nil);
        if (cErr == GL_FALSE) {
                printf("FreeWRL: set current context error!\n");
        }
        cErr = aglSetDrawable(aqglobalContext, nil);
        if (cErr == GL_FALSE) {
                printf("FreeWRL: set current context error!\n");
        }
        cErr = aglDestroyContext(aqglobalContext);
        if (cErr == GL_FALSE) {
                printf("FreeWRL: set current context error!\n");
        }
        aqglobalContext = nil;
}

void sendPluginFD(int fd) {
        /* printf ("sendPluginFD, FreeWRL received %d\n",fd); */
        _fw_browser_plugin = fd;
}
void aquaPrintVersion() {
        printf ("FreeWRL version: %s\n", libFreeWRL_get_version()); 
        exit(EXIT_SUCCESS);
}
void setPluginPath(char* path) {
        FREE_IF_NZ(PluginFullPath);
        PluginFullPath = strdup(path);
}


#endif

/* if we are visible, draw the OpenGL stuff, if not, don not bother */
void setDisplayed (int state) {
        #ifdef VERBOSE
        if (state) printf ("WE ARE DISPLAYED\n");
        else printf ("we are now iconic\n");
        #endif
        onScreen = state;
}

void setEaiVerbose() {
        eaiverbose = TRUE;
}
        
/* called from the standalone OSX front end */
void replaceWorldNeeded(char* str)
{
    strncpy(replace_name, str, FILENAME_MAX);
    replaceWorld = TRUE; 
}


/* send the description to the statusbar line */
void sendDescriptionToStatusBar(struct X3D_Node *CursorOverSensitive) {
        int tmp;
        char *ns;

        if (CursorOverSensitive == NULL) update_status ("");
        else {

                ns = NULL;
                for (tmp=0; tmp<num_SensorEvents; tmp++) {
                        if (SensorEvents[tmp].fromnode == CursorOverSensitive) {
                                switch (SensorEvents[tmp].datanode->_nodeType) {
                                        case NODE_Anchor: ns = ((struct X3D_Anchor *)SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_PlaneSensor: ns = ((struct X3D_PlaneSensor *)SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_SphereSensor: ns = ((struct X3D_SphereSensor *)SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_TouchSensor: ns = ((struct X3D_TouchSensor *)SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_GeoTouchSensor: ns = ((struct X3D_GeoTouchSensor *)SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_CylinderSensor: ns = ((struct X3D_CylinderSensor *)SensorEvents[tmp].datanode)->description->strptr; break;
				default: {printf ("sendDesc; unknown node type %d\n",SensorEvents[tmp].datanode->_nodeType);}
                                }
                                /* if there is no description, put the node type on the screen */
                                if (ns == NULL) {ns = "(over sensitive)";}
                                else if (ns[0] == '\0') ns = (char *)stringNodeType(SensorEvents[tmp].datanode->_nodeType);
        
                                /* send this string to the screen */
                                update_status(ns);
                        }
                }
        }
}
