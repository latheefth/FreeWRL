/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*
 * handle X events.
 */

#include <unistd.h>
#include <stdio.h>
#include "Bindable.h"
#include "Snapshot.h"
#include "EAIheaders.h"

#include "OpenGL_Utils.h"
#include "Viewer.h"
#include "Collision.h"
#include "SensInterps.h"

#ifndef AQUA
#include <X11/cursorfont.h>
#ifdef XF86V4
#include <X11/extensions/xf86vmode.h>
#endif
#include <X11/keysym.h>
#endif

#ifndef AQUA
Cursor arrowc;
Cursor sensorc;
Cursor curcursor;
#endif
#ifdef AQUA
#include <OpenGL.h>
CGLContextObj aqglobalContext;
#include <pthread.h>
pthread_t mythread = 0;
char* threadmsg;
#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#define MapNotify       19
#define SCURSE 1
#define ACURSE 0
int ccurse = ACURSE;
int ocurse = ACURSE;
#endif

int quitThread = 0;
char * keypress_string=NULL; 		/* Robert Sim - command line key sequence */
int keypress_wait_for_settle = 100;	/* JAS - change keypress to wait, then do 1 per loop */
extern int viewer_initialized;
#include "headers.h"

void Next_ViewPoint(void);		/*  switch to next viewpoint -*/
void setup_viewpoint(int doBinding);
void get_collisionoffset(double *x, double *y, double *z);

/* Sensor table. When clicked, we get back from rayHit the fromnode,
	have to look up type and data in order to properly handle it */
struct SensStruct {
	void *fromnode;
	void *datanode;
	void (*interpptr)(void *, int, int);
};
struct SensStruct *SensorEvents = 0;
int num_SensorEvents = 0;

/* Viewport data */
GLint viewPort2[10];

/* screen width and height. */
int screenWidth=1;
int screenHeight=1;
double nearPlane=0.1;
double farPlane=21000.0;
double screenRatio=1.5;
double fieldofview=45.0;


unsigned char * CursorOverSensitive=0;		/*  is Cursor over a Sensitive node?*/
unsigned char * oldCOS=0;			/*  which node was cursor over before this node?*/
int NavigationMode=FALSE;		/*  are we navigating or sensing?*/
int ButDown[] = {FALSE,FALSE,FALSE,FALSE,FALSE};

int currentX, currentY;			/*  current mouse position.*/
int lastMouseEvent = MapNotify;		/*  last event a mouse did; care about Button and Motion events only.*/
unsigned char * lastPressedOver = 0;		/*  the sensitive node that the mouse was last buttonpressed over.*/

int maxbuffers = 1;			/*  how many active indexes in bufferarray*/
int bufferarray[] = {GL_BACK,0};

/* current time and other time related stuff */
double TickTime;
double lastTime;
double BrowserStartTime; 	/* start of calculating FPS 	*/
double BrowserFPS = 0.0;	/* calculated FPS		*/

#ifdef PROFILE
static double timeA, timeB, timeC, timeD, timeE, timeF, xxf, oxf;
#endif


/* used for initializing (sometimes!) javascript initialize() */
int myMaxScript = -1;

/* do we have some sensitive nodes in scene graph? */
static int HaveSensitive = FALSE;


/* Function protos */
void do_keyPress(char kp, int type);
void render_collisions(void);
void render_pre(void);
void render(void);
void setup_projection(int pick, int x, int y);
void glPrintError(char *str);
void handle_Xevents(void);
void XEventStereo(void);
void handle_Xevents(void);
void EventLoop(void);
unsigned char*  rayHit(void);
void get_hyperhit(void);
void sendSensorEvents(unsigned char *COS,int ev, int status);

/* a simple routine to allow the front end to get our version */
char *getLibVersion() {
	return (FWVER);
}

/* Main eventloop for FreeWRL!!! */
void EventLoop() {
	int doEvents;
	int counter;


	#ifndef AQUA
	Cursor cursor;
	#endif

	static int loop_count = 0;
	struct timeval waittime;

	struct timeval mytime;
	struct timezone tz; /* unused see man gettimeofday */

	/* printf ("start of MainLoop\n");*/
	#ifdef PROFILEMARKER
	glTranslatef(1,1,1); glTranslatef (-1,-1,-1);
	#endif

	/* Set the timestamp */
	gettimeofday (&mytime,&tz);
	TickTime = (double) mytime.tv_sec + (double)mytime.tv_usec/1000000.0;

	/* First time through */
	if (loop_count == 0) {
		BrowserStartTime = TickTime;
		lastTime = TickTime;
		#ifdef PROFILE
		/* printf ("time setup for debugging\n"); */ 
		timeA = timeB = timeC = timeD = timeE = timeF =0.0;
		#endif
	} else {
		/*  rate limit ourselves to about 65fps.*/
		/* waittime.tv_usec = (TickTime - lastTime - 0.0120)*1000000.0;*/
		waittime.tv_usec = (TickTime - lastTime - 0.0153)*1000000.0;
		lastTime = TickTime;
		if (waittime.tv_usec < 0.0) {
			waittime.tv_usec = -waittime.tv_usec;
			/* printf ("waiting %d\n",(int)waittime.tv_usec);*/
			usleep((unsigned)waittime.tv_usec);
		}
	}
	if (loop_count == 25) {

		BrowserFPS = 25.0 / (TickTime-BrowserStartTime);
		update_status(); /*  tell status bar to refresh, if it is displayed*/
		/* printf ("fps %f\n",BrowserFPS);*/

		#ifdef PROFILE
		oxf = timeA + timeB + timeC + timeD + timeE + timeF;
		/* printf ("times %lf %lf %lf %lf %lf %lf\n",
				timeA,timeB,
				timeC, timeD,
				timeE,timeF); */
				/* timeA/oxf*100.0,timeB/oxf*100.0,*/
				/* timeC/oxf*100.0, timeD/oxf*100.0,*/
				/* timeE/oxf*100.0,timeF/oxf*100.0);*/
		#endif
		BrowserStartTime = TickTime;
		loop_count = 1;
	} else {
		loop_count++;
	}

	/* should we do events, or maybe Perl is parsing? */
	doEvents = (!isPerlParsing()) && (!isTextureParsing()) && isPerlinitialized();

	/* BrowserAction required? eg, anchors, etc */
	if (BrowserAction) {
		doBrowserAction ();
		BrowserAction = FALSE;	/* action complete */
	}

	#ifdef PROFILEMARKER
	glTranslatef(2,2,2); glTranslatef (-2,-2,-2);
	#endif

	/* handle any events provided on the command line - Robert Sim */
	if (keypress_string && doEvents) {
		if (keypress_wait_for_settle > 0) {
			keypress_wait_for_settle--;
		} else {
			/* dont do the null... */
			if (*keypress_string) {
				/* printf ("handling key %c\n",*keypress_string); */
				do_keyPress(*keypress_string,KeyPress);
				keypress_string++;
			} else {
				keypress_string=NULL;
			}
		}
	}

	/* Handle X events */
	handle_Xevents();


	#ifdef PROFILEMARKER
	glTranslatef(3,3,3); glTranslatef (-3,-3,-3);
	#endif


	#ifdef PROFILE
	gettimeofday (&mytime,&tz);
	xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	timeA = (double)timeA +  (double)xxf - TickTime;
	#endif

	/* Viewer move viewpoint */
	handle_tick();

	#ifdef PROFILE
	gettimeofday (&mytime,&tz);
	oxf = xxf;
	xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	timeB = (double)timeB +  (double)xxf - oxf;
	#endif

	/* setup Projection and activate ProximitySensors */
	render_pre();


	#ifdef PROFILEMARKER
	glTranslatef(4,4,4); glTranslatef (-4,-4,-4);
	#endif

	#ifdef PROFILE
	gettimeofday (&mytime,&tz);
	oxf = xxf;
	xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	timeC = (double)timeC +  (double)xxf - oxf;
	#endif


	/* first events (clock ticks, etc) if we have other things to do, yield */
	if (doEvents) do_first (); else sched_yield();


	#ifdef PROFILEMARKER
	glTranslatef(5,5,5); glTranslatef (-5,-5,-5);
	#endif

	#ifdef PROFILE
	gettimeofday (&mytime,&tz);
	oxf = xxf;
	xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	timeD = (double)timeD +  (double)xxf - oxf;
	#endif

	/* actual rendering */
	render();

	#ifdef PROFILEMARKER
	glTranslatef(6,6,6); glTranslatef (-6,-6,-6);
	#endif

	#ifdef PROFILE
	gettimeofday (&mytime,&tz);
	oxf = xxf;
	xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	timeE = (double)timeE +  (double)xxf - oxf;
	#endif

	/* handle_mouse events if clicked on a sensitive node */
	if (!NavigationMode && HaveSensitive) {
	/* if (!NavigationMode) {*/
		setup_projection(TRUE,currentX,currentY);
		setup_viewpoint(FALSE);
		render_hier(rootNode,VF_Sensitive);
		CursorOverSensitive = rayHit();

		/* did we have a click of button 1? */
		if (ButDown[1] && (lastPressedOver==0)) {
			/*  printf ("Not Navigation and 1 down\n");*/
			/* send an event of ButtonPress and isOver=true */
			lastPressedOver = CursorOverSensitive;
			sendSensorEvents(lastPressedOver, ButtonPress, TRUE);
		}

		if ((ButDown[1]==0) && lastPressedOver) {
			/*  printf ("Not Navigation and 1 up\n");*/
			/* send an event of ButtonRelease and isOver=true;
			   an isOver=false event will be sent below if required */
			sendSensorEvents(lastPressedOver, ButtonRelease, TRUE);
			lastPressedOver = 0;
		}

		if ((lastMouseEvent == MotionNotify) && ButDown[1]) {
			/*  printf ("Not Navigation and motion\n");*/
			sendSensorEvents(lastPressedOver,MotionNotify, TRUE);
		}



		/* do we need to re-define cursor style? 	*/
		/* do we need to send an isOver event?		*/
		if (CursorOverSensitive!= 0) {
#ifndef AQUA
			cursor= sensorc;
#else
		ccurse = SCURSE;
#endif

			/* is this a new node that we are now over?
			   don't change the node pointer if we are clicked down */
			if ((lastPressedOver==0) && (CursorOverSensitive != oldCOS)) {
				sendSensorEvents(oldCOS,MapNotify,FALSE);
				sendSensorEvents(CursorOverSensitive,MapNotify,TRUE);
				oldCOS=CursorOverSensitive;
			}

		} else {
			/* hold off on cursor change if dragging a sensor */
			if (lastPressedOver!=0) {
#ifndef AQUA
				cursor = sensorc;
#else
				ccurse = SCURSE;
#endif
			} else {
#ifndef AQUA
				cursor = arrowc;
#else
				ccurse = ACURSE;
#endif
			}

			/* were we over a sensitive node? */
			if (oldCOS!=0) {
				sendSensorEvents(oldCOS,MapNotify,FALSE);
				oldCOS=0;
			}
		}

		/* do we have to change cursor? */
#ifndef AQUA
		if (cursor != curcursor) {
			curcursor = cursor;
			XDefineCursor (dpy, win, cursor);
		}
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


	#ifdef PROFILEMARKER
	glTranslatef(7,7,7); glTranslatef (-7,-7,-7);
	#endif

	if (doEvents) {
		/* handle ROUTES - at least the ones not generated in do_first() */
		propagate_events();

		/* Javascript events processed */
		process_eventsProcessed();

		/* EAI */
		handle_EAI();


	#ifdef PROFILEMARKER
	glTranslatef(10,10,10); glTranslatef (-10,-10,-10);
	#endif

	}
	/* any new scripts in here, that maybe are not initialized yet?
	 * If they had some events already, they'll already be initialized;
	 * but don't do this if perl is parsing, because script init might
	 * reference VRML constructs NOT found yet */
	if (myMaxScript != max_script_found) {
		if (!isPerlParsing()) {
		    for (counter = myMaxScript; counter <= max_script_found; counter++) {
			initializeScript(counter, FALSE);
		    }
		    myMaxScript = max_script_found;
		}
	}

	#ifdef PROFILE
	gettimeofday (&mytime,&tz);
	oxf = xxf;
	xxf = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	timeF = (double)timeF +  (double)xxf - oxf;
	#endif

}



void handle_Xevents() {
#ifndef AQUA

	XEvent event, nextevent;
	char buf[10];
	KeySym ks;
	int count;

	while (XPending(dpy)) {

		XNextEvent(dpy,&event);
		lastMouseEvent=event.type;
		switch(event.type) {
			case ConfigureNotify:
				setScreenDim (event.xconfigure.width,event.xconfigure.height);
				break;
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
				buf[0]=(char)ks;buf[1]='\0';
				do_keyPress((char)ks,event.type);

				break;
			case ButtonPress:
			case ButtonRelease:
				/*  if a button is pressed, we should not change state,*/
				/*  so keep a record.*/
				if (event.xbutton.button>=5) break;  /* bounds check*/
				ButDown[event.xbutton.button] = (event.type == ButtonPress);

				/* if we are Not over a sensitive node, and we do NOT
				   already have a button down from a sensitive node... */
				if ((CursorOverSensitive==0) &&
						(lastPressedOver==0))  {
					NavigationMode=ButDown[1] || ButDown[3];
					handle (event.type,event.xbutton.button,
						(float) ((float)event.xbutton.x/screenWidth),
						(float) ((float)event.xbutton.y/screenHeight));
				}
                                break;
                        case MotionNotify:
				/*  do we have more motion notify events queued?*/
				if (XPending(dpy)) {
					XPeekEvent(dpy,&nextevent);
					if (nextevent.type==MotionNotify) { break;
					}
				}

				/*  save the current x and y positions for picking.*/
				currentX = event.xbutton.x;
				currentY = event.xbutton.y;

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
			case Expose:
				break;
			case MapNotify:
				set_now_mapped(TRUE);
				break;
			case UnmapNotify:
				set_now_mapped(FALSE);
				break;
			default:
				break;
		}
	}
#endif
}

/* get setup for rendering. */
void render_pre() {
	/* 1. Set up projection */
	setup_projection(FALSE,0,0);


	/* 2. Headlight, initialized here where we have the modelview matrix to Identity.
	FIXME: position of light sould actually be offset a little (towards the center)
	when in stereo mode. */
	fwLoadIdentity();

	if (get_headlight()) lightState(0,TRUE);


	/* 3. Viewpoint */
	setup_viewpoint(TRUE); 	/*  need this to render collisions correctly*/

	/* 4. Collisions */
	if (be_collision == 1) {
		render_collisions();
		setup_viewpoint(FALSE); /*  update viewer position after collision, to*/
				   /*  give accurate info to Proximity sensors.*/
	}

	/* 5. render hierarchy - proximity */
	render_hier((void *)rootNode, VF_Proximity);

	glPrintError("GLBackend::render_pre");
}

/* Render the scene */
void render() {
	int count;

	/*  profile*/
	/* double xx,yy,zz,aa,bb,cc,dd,ee,ff;*/
	/* struct timeval mytime;*/
	/* struct timezone tz; unused see man gettimeofday */

	/* set transparency flag */
	have_transparency = 0;

	for (count = 0; count < maxbuffers; count++) {


	#ifdef PROFILEMARKER
	glTranslatef(50,50,50); glTranslatef (-50,-50,-50);
	#endif


	/* gettimeofday (&mytime,&tz);*/
	/* aa = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;*/

		set_buffer((unsigned)bufferarray[count]);		/*  in Viewer.c*/
		glDrawBuffer((unsigned)bufferarray[count]);

		/*  turn lights off, and clear buffer bits*/
		BackEndClearBuffer();
		BackEndLightsOff();

		/*  turn light #0 off only if it is not a headlight.*/
		if (!get_headlight()) {
			lightState(0,FALSE);
		}


		#ifdef PROFILEMARKER
		glTranslatef(50,50,50); glTranslatef (-50,-50,-50);
		#endif


		/*  Correct Viewpoint, only needed when in stereo mode.*/
		if (maxbuffers > 1) setup_viewpoint(FALSE);


		#ifdef PROFILEMARKER
		glTranslatef(51,51,51); glTranslatef (-51,-51,-51);
		#endif

	/* gettimeofday (&mytime,&tz);*/
	/* bb = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;*/

		/*  Other lights*/
		glPrintError("XEvents::render, before render_hier");

		render_hier((void *)rootNode, VF_Lights);
		glPrintError("XEvents::render, render_hier(VF_Lights)");


		#ifdef PROFILEMARKER
		glTranslatef(52,52,52); glTranslatef (-52,-52,-52);
		#endif

	/* gettimeofday (&mytime,&tz);*/
	/* cc = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;*/

		/*  4. Nodes (not the blended ones)*/

		render_hier((void *)rootNode, VF_Geom);
		glPrintError("XEvents::render, render_hier(VF_Geom)");

		/*  5. Blended Nodes*/


		#ifdef PROFILEMARKER
		glTranslatef(53,53,53); glTranslatef (-53,-53,-53);
		#endif

	/* gettimeofday (&mytime,&tz);*/
	/* dd = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;*/

		if (have_transparency > 0) {
			/*  turn off writing to the depth buffer*/
			glDepthMask(FALSE);

			/*  render the blended nodes*/
			render_hier((void *)rootNode, VF_Geom | VF_Blend);

			/*  and turn writing to the depth buffer back on*/
			glDepthMask(TRUE);
			glPrintError("XEvents::render, render_hier(VF_Geom)");
		}

		#ifdef PROFILEMARKER
		glTranslatef(54,54,54); glTranslatef (-54,-54,-54);
		#endif



	/* gettimeofday (&mytime,&tz);*/
	/* ee = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;*/

	/* printf ("render %f %f %f %f\n",bb-aa, cc-bb, dd-cc, ee-dd);*/
	}
#ifndef AQUA
	glXSwapBuffers(dpy,win);
#else
	CGLError err = CGLFlushDrawable(aqglobalContext);
	updateContext();
#endif

	#ifdef PROFILEMARKER
	glTranslatef(55,55,55); glTranslatef (-55,-55,-55);
	#endif


	glPrintError("XEvents::render");
}



void
get_collisionoffset(double *x, double *y, double *z)
{
	struct pt res = CollisionInfo.Offset;

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
	    }
	}
}

void render_collisions() {
	struct pt v;
	CollisionInfo.Offset.x = 0;
	CollisionInfo.Offset.y = 0;
	CollisionInfo.Offset.z = 0;
	CollisionInfo.Count = 0;
	CollisionInfo.Maximum2 = 0.;

	render_hier((void *)rootNode, VF_Collision);
	get_collisionoffset(&(v.x), &(v.y), &(v.z));
	increment_pos(&v);
}

void setup_viewpoint(int doBinding) {
	int i;
	unsigned int *setBindPtr;
	int render_flag;	/* is this a VF_Viewpoint, or a VF_Blend? */

	/* first, go through, and see if any viewpoints require binding. */
	/* some scripts just send a set_bind to a Viewpoint; if another  */
	/* viewpoint beforehand is bound, the set_bind will never get	 */
	/* seen if we leave this to the rendering stage	(grep for 	 */
	/* found_vp to see why	JAS					 */

	/* also, we can sort nodes for proper blending here, using the	*/
	/* doBinding flag also. JAS 					*/

	render_flag = VF_Viewpoint;

	if (doBinding & (!isPerlParsing())) {
		/* top of mainloop, we can tell the renderer to sort children */
		/* render_flag = VF_Viewpoint | VF_SortChildren;*/
		render_flag = VF_Viewpoint;

		for (i=0; i<totviewpointnodes; i++) {
			setBindPtr = (unsigned int *)(viewpointnodes[i]+
				offsetof (struct VRML_Viewpoint, set_bind));

			/* check the set_bind eventin to see if it is TRUE or FALSE */
			if (*setBindPtr < 100) {
				/* printf ("Found a vp to modify %d\n",viewpointnodes[i]);*/
				/* up_vector is reset after a bind */
				if (*setBindPtr==1) reset_upvector();

				bind_node ((void *)viewpointnodes[i],
					offsetof (struct VRML_Viewpoint,set_bind),
					offsetof (struct VRML_Viewpoint,isBound),
					&viewpoint_tos,&viewpoint_stack[0]);
			}
		}
	}

        fwMatrixMode(GL_MODELVIEW); /*  this should be assumed , here for safety.*/
        fwLoadIdentity();

        /* Make viewpoint, adds offset in stereo mode.*/
        /* FIXME: I think it also adds offset of left eye in mono mode.*/

        viewer_togl(fieldofview);

        render_hier((void *)rootNode, render_flag);
        glPrintError("XEvents::setup_viewpoint");
}



void setup_projection(int pick, int x, int y) {
	fwMatrixMode(GL_PROJECTION);
	glViewport(0,0,screenWidth,screenHeight);
	fwLoadIdentity();
	if(pick) {
		/* picking for mouse events */
		glGetIntegerv(GL_VIEWPORT,viewPort2);
		gluPickMatrix((float)x,(float)viewPort2[3]-y,
			(float)100,(float)100,viewPort2);
	}

        /* bounds check */
        if ((fieldofview <= 0.0) || (fieldofview > 180.0)) fieldofview=45.0;
        /* glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);  */
        gluPerspective(fieldofview, screenRatio, nearPlane, farPlane); 

        fwMatrixMode(GL_MODELVIEW);

	glPrintError("XEvents::setup_projection");
}

/* handle a keypress. "man freewrl" shows all the recognized keypresses */
void do_keyPress(const char kp, int type) {
	char comline[100];
	if (type == KeyPress) {
		switch (kp) {
			case 'e': { set_viewer_type (EXAMINE); break; }
			case 'w': { set_viewer_type (WALK); break; }
			case 'd': { set_viewer_type (FLY); break; }
			case 'f': { set_viewer_type (EXFLY); break; }
			case 'h': { toggle_headlight(); break;}
			case '/': { print_viewer(); break; }
			case '.': { display_status = !display_status; break; }
			case 'q': { if (!RUNNINGASPLUGIN) {
					  doQuit();
					break;
				    }
				  }
			case 'c': {be_collision = !be_collision; break; }
			case '?': {
					  if (strlen(ASCIITERM) < 50) {
						  strcpy (comline,ASCIITERM);
						  strcat (comline," -e man freewrl &");
						  freewrlSystem(comline);
					  }
				   break;
				  }
			case 'v': {Next_ViewPoint(); break;}
			case 's': {setSnapshot(); break;}
			default: {handle_key(kp);}

		}
	} else {
		handle_keyrelease(kp);
	}
}

unsigned char* rayHit() {
        double x,y,z;

        if(hpdist >= 0) {
                gluUnProject(hp.x,hp.y,hp.z,rh.modelMatrix,rh.projMatrix,viewport,&x,&y,&z);

                /* and save this globally */
                ray_save_posn.c[0] = x; ray_save_posn.c[1] = y; ray_save_posn.c[2] = z;

                return ((unsigned char*) rh.node);
        } else {
                return(0);
        }
}


/* set a node to be sensitive, and record info for this node */
void setSensitive(void *ptr,void *datanode,char *type) {
	struct VRML_Box *p;
	void (*myp)(unsigned *);

	/* printf ("set_sensitive ,ptr %d data %d type %s\n",ptr,datanode,type);*/

	if (strncmp("TouchSensor",type,10) == 0) { myp =  (void *)do_TouchSensor;
	} else if (strncmp("GeoTouchSensor",type,10) == 0) { myp = (void *)do_GeoTouchSensor;
	} else if (strncmp("PlaneSensor",type,10) == 0) { myp = (void *)do_PlaneSensor;
	} else if (strncmp("CylinderSensor",type,10) == 0) { myp = (void *)do_CylinderSensor;
	} else if (strncmp("SphereSensor",type,10) == 0) { myp = (void *)do_SphereSensor;
	} else if (strncmp("Anchor",type,10) == 0) { myp = (void *)do_Anchor;
	} else if (strncmp("ProximitySensor",type,10) == 0) { return; /* its time sensive only */

	} else {
		printf ("set_sensitive, unhandled type %s\n",type);
		return;
	}

	/* mark THIS node as sensitive. */
	p = ptr;
	p->_sens = TRUE;

 	/* and tell the rendering pass that there is a sensitive node down*/
 	 /* this branch */
/* 	update_renderFlag(p,VF_Sensitive);*/

	/* tell mainloop that we have to do a sensitive pass now */
	HaveSensitive = TRUE;

	/* record this sensor event for clicking purposes */
	SensorEvents = realloc(SensorEvents,sizeof (struct SensStruct) * (num_SensorEvents+1));
	if (SensorEvents == 0) {
		printf ("setSensitive: can not allocate memory\n");
		num_SensorEvents = 0;
	}

	if (datanode == 0) {
		printf ("setSensitive: datastructure is zero for type %s\n",type);
		return;
	}

	/* now, put the function pointer and data pointer into the structure entry */
	SensorEvents[num_SensorEvents].fromnode = ptr;
	SensorEvents[num_SensorEvents].datanode = datanode;
	SensorEvents[num_SensorEvents].interpptr = (void *)myp;

	num_SensorEvents++;
}

/* we have a sensor event changed, look up event and do it */
/* note, ProximitySensor events are handled during tick, as they are time-sensitive only */
void sendSensorEvents(unsigned char * COS,int ev, int status) {
	int count;

	/* printf ("sio, COS %d ev %d status %d\n",COS,ev,status); */
	if (COS==0) return;

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


			SensorEvents[count].interpptr(SensorEvents[count].datanode, ev,status);
			return;
		}
	}
}


/* If we have a sensitive node, that is clicked and moved, get the posn
   for use later								*/
void get_hyperhit() {
	double x1,y1,z1,x2,y2,z2,x3,y3,z3;
	GLdouble projMatrix[16];

	fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(r1.x, r1.y, r1.z, rhhyper.modelMatrix,
		projMatrix, viewport, &x1, &y1, &z1);
	gluUnProject(r2.x, r2.y, r2.z, rhhyper.modelMatrix,
		projMatrix, viewport, &x2, &y2, &z2);
	gluUnProject(hp.x, hp.y, hp.z, rh.modelMatrix,
		projMatrix,viewport, &x3, &y3, &z3);

	/* printf ("get_hyperhit in VRMLC %f %f %f, %f %f %f, %f %f %f\n",*/
	/* 	x1,y1,z1,x2,y2,z2,x3,y3,z3);*/

	/* and save this globally */
	hyp_save_posn.c[0] = x1; hyp_save_posn.c[1] = y1; hyp_save_posn.c[2] = z1;
	hyp_save_norm.c[0] = x2; hyp_save_norm.c[1] = y2; hyp_save_norm.c[2] = z2;
	ray_save_posn.c[0] = x3; ray_save_posn.c[1] = y3; ray_save_posn.c[2] = z3;
}





/* set stereo buffers, if required */
void XEventStereo() {
	bufferarray[0]=GL_BACK_LEFT;
	bufferarray[1]=GL_BACK_RIGHT;
	maxbuffers=2;
}

/* if we had an opengl error... */
void glPrintError(char *str) {
#ifdef GLERRORS
        int err;
        while((err = glGetError()) != GL_NO_ERROR)
                fprintf(stderr,"OpenGL Error: \"%s\" in %s\n", gluErrorString((unsigned)err),str);

#endif
        }

/* go to the next viewpoint */
void Next_ViewPoint() {
	if (totviewpointnodes>=2) {
		/* whew, we have other vp nodes */
		send_bind_to(VIEWPOINT,(void *)viewpointnodes[currboundvpno],0);
		currboundvpno++;
		if (currboundvpno>=totviewpointnodes) currboundvpno=0;
		send_bind_to(VIEWPOINT,(void *)viewpointnodes[currboundvpno],1);
	}
}

/* set internal variables for screen sizes, and calculate frustum */
void setScreenDim(int wi, int he) {
        screenWidth = wi;
        screenHeight = he;
        if (screenHeight != 0) screenRatio = (double) screenWidth/(double) screenHeight;
        else screenRatio =  screenWidth;

	calculateFrustumCone();
}

void setBrowserURL(char* file) {
        int count;
        count = strlen(file);
        if (BrowserURL != NULL) {
                free (BrowserURL);
        }
        BrowserURL = malloc (count + 1);
        strcpy(BrowserURL, file);
}

void setFullPath(char* file) {
	int count;
	count = strlen(file);
	if (BrowserFullPath != NULL) {
		free (BrowserFullPath);
	}
	BrowserFullPath = malloc ((strlen(file)+1) * sizeof (char));
	strcpy(BrowserFullPath, file);
}

#ifdef AQUA
void initGL() {
        aqglobalContext = CGLGetCurrentContext();
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
}

int getOffset() {
        return offsetof(struct VRML_Group, children);
}

void setCurXY(int cx, int cy) {
        currentX = cx;
        currentY = cy;
}

void setLastMouseEvent(int etype) {
        lastMouseEvent = etype;
}


void initFreewrl() {
        threadmsg = "event loop";
	quitThread = 0;
	if (mythread <= 0) {
        	pthread_create(&mythread, NULL, (void *) aqDisplayThread, (void*) threadmsg);
        	initializePerlThread("/usr/bin/perl");
        	while (!isPerlinitialized()) {
        	        usleep(50);
        	}
        	initializeTextureThread();
        	while (!isTextureinitialized()) {
        	        usleep(50);
        	}
	}
        int tmp = 0;
        perlParse(FROMURL, BrowserURL, TRUE, FALSE, rootNode, offsetof(struct VRML_Group, children), &tmp, TRUE);
}

void aqDisplayThread() {
        glpOpenGLInitialize();
        new_tessellation();
        while (1) {
		while (!quitThread) {
                	EventLoop();
		}
        }
}

void setButDown(int button, int value) {
        ButDown[button] = value;
}

#endif

void setSnapSeq() {
        snapsequence = TRUE;
}

void setEAIport(int pnum) {
        EAIport = pnum;
}

void setWantEAI(int flag) {
	EAIwanted = TRUE;
}

void setNoStatus() {
        display_status = 0;
}

void setSeqFile(char* file) {
        int count;
        count = strlen(file);
        if (count > 500) count = 500;
        snapseqB = malloc (count+1);
        strcpy(snapseqB, file);
        printf("snapseqB is %s\n", snapseqB);
}

void setSnapFile(char* file) {
        int count;
        count = strlen(file);
        if (count > 500) count = 500;
        snapsnapB = malloc(count + 1);
        strcpy(snapsnapB, file);
        printf("snapsnapB is %s\n", snapsnapB);
}

void closeFreewrl() {
        struct Multi_Node* tn;
        struct VRML_Group* rn;
        int i;
        /* kill any remaining children */
        /* printf ("doQuit - calling exit(0)\n"); */
        rn = (struct VRML_Group*) rootNode;
        tn =  &(rn->children);
        tn->n = 0;
        quitThread = 1;
        if (fognodes) free (fognodes);
        if (backgroundnodes) free (backgroundnodes);
        if (navnodes) free (navnodes);
        if (viewpointnodes) free (viewpointnodes);
        fognodes = NULL;
        backgroundnodes = NULL;
        navnodes = NULL;
        viewpointnodes = NULL;
        totviewpointnodes = 0;
        totfognodes = 0;
        totnavnodes = 0;
        totbacknodes = 0;
        viewer_initialized = FALSE;
        set_viewer_type (EXAMINE);
}

void setMaxImages(int max) {
        if (max <=0)
                max = 100;
        maxSnapImages = max;
}
void setSeqTemp(char* file) {
        int count;
        count = strlen(file);
        if (count > 500) count = 500;
        seqtmp = malloc(count + 1);
        strcpy(seqtmp, file);
        printf("seqtmp is %s\n", seqtmp);
}

/* if we had an exit(1) anywhere in this C code - it means
   a memory error. So... print out a standard message to the
   console. */
void outOfMemory(char *msg) {
	ConsoleMessage ("FreeWRL has encountered a memory allocation problem\n"\
			"and is exiting.\n -- %s--",msg);
	exit(1);
}

/* quit key pressed, or Plugin sends SIGQUIT */
void doQuit(void) {
#ifndef AQUA
	char mystring[20];

	/* printf ("doQuit - got the quit signal in thread %d\n",pthread_self()); */
	sprintf (mystring, "QUIT");
	Sound_toserver(mystring);
	resetGeometry();
	if (wantEAI) shutdown_EAI();
#endif

	/* kill any remaining children */
	killErrantChildren();
	/* printf ("doQuit - calling exit(0)\n"); */
	exit(0);
}

void freewrlDie (const char *format) {
	ConsoleMessage ("Catastrophic error: %s\n",format);
	doQuit();
}

