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
pthread_t mythread;
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

#include "headers.h"

void Next_ViewPoint(void);		// switch to next viewpoint - 
void setup_viewpoint(int doBinding);
void get_collisionoffset(double *x, double *y, double *z);

/* Sensor table. When clicked, we get back from rayHit the fromnode, 
	have to look up type and data in order to properly handle it */
struct SensStruct {
	void *fromnode;
	int  datanode;
	void (*interpptr)(int, int, int);
};
struct SensStruct *SensorEvents = 0;
int num_SensorEvents = 0;

/* Viewport data */
GLint viewPort2[10];

/* screen width and height. */
int screenWidth=1;
int screenHeight=1;
double screenRatio=1.5;
int CursorOverSensitive=FALSE;		// is Cursor over a Sensitive node?
int oldCOS=FALSE;			// which node was cursor over before this node?
int NavigationMode=FALSE;		// are we navigating or sensing?
int ButDown[] = {FALSE,FALSE,FALSE,FALSE,FALSE};

int currentX, currentY;			// current mouse position.
int lastMouseEvent = MapNotify;		// last event a mouse did; care about Button and Motion events only.
int lastPressedOver = 0;		// the sensitive node that the mouse was last buttonpressed over.

int maxbuffers = 1;			// how many active indexes in bufferarray
int bufferarray[] = {GL_BACK,0};

/* current time and other time related stuff */
double TickTime;
double lastTime;
double BrowserStartTime; 	/* start of calculating FPS 	*/
double BrowserFPS = 0.0;	/* calculated FPS		*/

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
int  rayHit(void);
void get_hyperhit(void);
void sendSensorEvents(int COS,int ev, int status);

/* a simple routine to allow the front end to get our version */
char *getLibVersion() {
	return (FWVER);
}


/* Main eventloop for FreeWRL!!! */
void EventLoop() {


	#ifndef AQUA
	Cursor cursor;
	#endif

	static int loop_count = 0;
	struct timeval waittime;

	struct timeval mytime;
	struct timezone tz; /* unused see man gettimeofday */

	/* Set the timestamp */
	gettimeofday (&mytime,&tz);
	TickTime = (double) mytime.tv_sec + (double)mytime.tv_usec/1000000.0;

	/* First time through */
	if (loop_count == 0) {
		BrowserStartTime = TickTime;
		lastTime = TickTime;
	} else {
		// rate limit ourselves to about 65fps. 
		waittime.tv_usec = (TickTime - lastTime - 0.0120)*1000000.0; 
		lastTime = TickTime;
		if (waittime.tv_usec < 0.0) {
			waittime.tv_usec = -waittime.tv_usec;
			//printf ("waiting %d\n",(int)waittime.tv_usec);
			usleep((unsigned)waittime.tv_usec);
		}
	}
	if (loop_count == 25) {
		BrowserFPS = 25.0 / (TickTime-BrowserStartTime);
		update_status(); // tell status bar to refresh, if it is displayed 
		BrowserStartTime = TickTime; 
		loop_count = 1;
	} else {
		loop_count++;
	}

	/* BrowserAction required? eg, anchors, etc */
	if (BrowserAction) {
		doBrowserAction ();
		BrowserAction = FALSE;	/* action complete */
	}

	/* Handle X events */
	handle_Xevents();

	/* Viewer move viewpoint */
	handle_tick();

	/* setup Projection and activate ProximitySensors */
	render_pre();

	/* first events (clock ticks, etc) */
	do_first ();
	
	/* actual rendering */
	render();

	/* handle_mouse events if clicked on a sensitive node */
	if (!NavigationMode) {
		setup_projection(TRUE,currentX,currentY);
		setup_viewpoint(FALSE);
		render_hier((void *)rootNode,VF_Sensitive);
		CursorOverSensitive = rayHit();

		/* did we have a click of button 1? */
		if (ButDown[1] && (!lastPressedOver)) {
			// printf ("Not Navigation and 1 down\n");
			/* send an event of ButtonPress and isOver=true */
			lastPressedOver = CursorOverSensitive;
			sendSensorEvents(lastPressedOver, ButtonPress, TRUE);
		}

		if ((!ButDown[1]) && lastPressedOver) {
			// printf ("Not Navigation and 1 up\n");
			/* send an event of ButtonRelease and isOver=true;
			   an isOver=false event will be sent below if required */
			sendSensorEvents(lastPressedOver, ButtonRelease, TRUE);
			lastPressedOver = 0;
		}

		if ((lastMouseEvent == MotionNotify) && ButDown[1]) {
			// printf ("Not Navigation and motion\n");
			sendSensorEvents(lastPressedOver,MotionNotify, TRUE);
		}



		/* do we need to re-define cursor style? 	*/
		/* do we need to send an isOver event?		*/
		if (CursorOverSensitive) {
#ifndef AQUA
			cursor= sensorc;
#else
		ccurse = SCURSE;
#endif

			/* is this a new node that we are now over? 
			   don't change the node pointer if we are clicked down */
			if ((!lastPressedOver) && (CursorOverSensitive != oldCOS)) {
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
			if (oldCOS) {
				sendSensorEvents(oldCOS,MapNotify,FALSE);
				oldCOS=FALSE;
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

	/* handle ROUTES - at least the ones not generated in do_first() */
	propagate_events();

	/* Javascript events processed */
	process_eventsProcessed();

	/* EAI */
	handle_EAI();
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
				screenWidth = event.xconfigure.width;
				screenHeight = event.xconfigure.height;
				if (screenHeight != 0) screenRatio = (double) screenWidth/(double) screenHeight;
				else screenRatio =  screenWidth;
				break;
			case KeyPress:
			case KeyRelease:
				XLookupString(&event.xkey,buf,sizeof(buf),&ks,0);
				// Map keypad keys in - thanks to Aubrey Jaffer.
				switch(ks) {
				   // the non-keyboard arrow keys
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
				// if a button is pressed, we should not change state,
				// so keep a record.
				if (event.xbutton.button>5) break;  //bounds check
				ButDown[event.xbutton.button] = (event.type == ButtonPress);

				/* if we are Not over a sensitive node, and we do NOT
				   already have a button down from a sensitive node... */
				if ((!CursorOverSensitive) && 
						(!lastPressedOver))  {
					NavigationMode=ButDown[1] || ButDown[3];
					handle (event.type,event.xbutton.button,
						(float) ((float)event.xbutton.x/screenWidth),
						(float) ((float)event.xbutton.y/screenHeight));
				}
                                break;
                        case MotionNotify:
				// do we have more motion notify events queued?
				if (XPending(dpy)) {
					XPeekEvent(dpy,&nextevent);
					if (nextevent.type==MotionNotify) { break;
					}
				}

				// save the current x and y positions for picking.
				currentX = event.xbutton.x;
				currentY = event.xbutton.y;

				if (NavigationMode) {
					// find out what the first button down is
					count = 0;
					while ((count < 5) && (!ButDown[count])) count++;
					if (count == 5) return; // no buttons down???

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
	glLoadIdentity();

	if (get_headlight()) BackEndHeadlightOn();


	/* 3. Viewpoint */
	setup_viewpoint(TRUE); 	// need this to render collisions correctly

	/* 4. Collisions */
	if (be_collision == 1) {
		render_collisions();
		setup_viewpoint(FALSE); // update viewer position after collision, to 
				   // give accurate info to Proximity sensors.
	}

	/* 5. render hierarchy - proximity */
	render_hier((void *)rootNode, VF_Proximity);

	glPrintError("GLBackend::render_pre");
}

/* Render the scene */
void render() {
	int count;

	/* set transparency flag */
	have_transparency = 0;

	for (count = 0; count < maxbuffers; count++) {
		set_buffer((unsigned)bufferarray[count]);		// in Viewer.c
		glDrawBuffer((unsigned)bufferarray[count]);

		// turn lights off, and clear buffer bits
		BackEndClearBuffer();
		BackEndLightsOff();

		// turn light #0 off only if it is not a headlight.
		if (!get_headlight()) {
			BackEndHeadlightOff();
		}

		// Correct Viewpoint, only needed when in stereo mode.
		if (maxbuffers > 1) setup_viewpoint(FALSE);

		// Other lights
		glPrintError("XEvents::render, before render_hier");

		render_hier((void *)rootNode, VF_Lights);
		glPrintError("XEvents::render, render_hier(VF_Lights)");

		// 4. Nodes (not the blended ones)

		render_hier((void *)rootNode, VF_Geom);
		glPrintError("XEvents::render, render_hier(VF_Geom)");

		// 5. Blended Nodes 

		if (have_transparency > 0) {
			render_hier((void *)rootNode, VF_Geom | VF_Blend);
			glPrintError("XEvents::render, render_hier(VF_Geom)");
		}
	}
#ifndef AQUA
	glXSwapBuffers(dpy,win);
#else
	CGLError err = CGLFlushDrawable(aqglobalContext);
	updateContext();
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
		render_flag = VF_Viewpoint | VF_Blend;

		for (i=0; i<totviewpointnodes; i++) {
			setBindPtr = (unsigned int *)(viewpointnodes[i]+
				offsetof (struct VRML_Viewpoint, set_bind));

			/* check the set_bind eventin to see if it is TRUE or FALSE */
			if (*setBindPtr < 100) {
				//printf ("Found a vp to modify %d\n",viewpointnodes[i]);
				/* up_vector is reset after a bind */
				if (*setBindPtr==1) reset_upvector();

				bind_node ((void *)viewpointnodes[i],
					offsetof (struct VRML_Viewpoint,set_bind),
					offsetof (struct VRML_Viewpoint,isBound),
					&viewpoint_tos,&viewpoint_stack[0]);
			}
		}
	}
	
        glMatrixMode(GL_MODELVIEW); // this should be assumed , here for safety.
        glLoadIdentity();

        //Make viewpoint, adds offset in stereo mode.
        //FIXME: I think it also adds offset of left eye in mono mode.

        viewer_togl(fieldofview);

        render_hier((void *)rootNode, render_flag);
        glPrintError("XEvents::setup_viewpoint");
}



void setup_projection(int pick, int x, int y) {
	glMatrixMode(GL_PROJECTION);
	
	glViewport(0,0,screenWidth,screenHeight);
	glLoadIdentity();
	if(pick) {
		/* picking for mouse events */
		glGetIntegerv(GL_VIEWPORT,viewPort2);
		//gluPickMatrix(x,viewPort2[3]-y,3,3,viewPort2);
		gluPickMatrix((float)x,(float)viewPort2[3]-y,
			(float)100,(float)100,viewPort2);
	}

        /* bounds check */
        if ((fieldofview <= 0.0) || (fieldofview > 180.0)) fieldofview=45.0;

        gluPerspective(fieldofview, screenRatio, 0.1, 21000.0);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
        glMatrixMode(GL_MODELVIEW);

	glPrintError("XEvents::setup_projection");
}

/* handle a keypress. "man freewrl" shows all the recognized keypresses */
void do_keyPress(const char kp, int type) {
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
#ifndef AQUA
					if (wantEAI) shutdown_EAI();
#endif
					exit(0);
					break;
				    }
				  }
			case 'c': {be_collision = !be_collision; break; }
			case '?': {system ("xterm -e man freewrl &"); break;}
			case 'v': {Next_ViewPoint(); break;}
			case 's': {setSnapshot(); break;}
			default: {handle_key(kp);}

		}
	} else {
		handle_keyrelease(kp);
	}
}

int rayHit() {
        double x,y,z;

        if(hpdist >= 0) {
                gluUnProject(hp.x,hp.y,hp.z,rh.modelMatrix,rh.projMatrix,viewport,&x,&y,&z);

                /* and save this globally */
                ray_save_posn.c[0] = x; ray_save_posn.c[1] = y; ray_save_posn.c[2] = z;

                return ((int)rh.node);
        } else {
                return(0);
        }
}


/* set a node to be sensitive, and record info for this node */
void setSensitive(void *ptr,int datanode,char *type) {
	struct VRML_Box *p;
	void (*myp)(unsigned *);

	//printf ("set_sensitive ,ptr %d data %d type %s\n",ptr,datanode,type);
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

	/* mark this node as sensitive. */
	p = ptr;
	p->_sens = TRUE;

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
void sendSensorEvents(int COS,int ev, int status) {
	int count;

	/* printf ("sio, COS %d ev %d status %d\n",COS,ev,status); */
	if (!COS) return;

	for (count = 0; count < num_SensorEvents; count++) {
		if ((int) SensorEvents[count].fromnode == COS) { 
	
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


			SensorEvents[count].interpptr(SensorEvents[count].datanode,
                                                                ev,status);
			return;
		}
	}
}


/* If we have a sensitive node, that is clicked and moved, get the posn
   for use later								*/
void get_hyperhit() {
	double x1,y1,z1,x2,y2,z2,x3,y3,z3;
	GLdouble projMatrix[16];

	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(r1.x, r1.y, r1.z, rhhyper.modelMatrix,
		projMatrix, viewport, &x1, &y1, &z1);
	gluUnProject(r2.x, r2.y, r2.z, rhhyper.modelMatrix,
		projMatrix, viewport, &x2, &y2, &z2);
	gluUnProject(hp.x, hp.y, hp.z, rh.modelMatrix,
		projMatrix,viewport, &x3, &y3, &z3);
		
	//printf ("get_hyperhit in VRMLC %f %f %f, %f %f %f, %f %f %f\n",
	//	x1,y1,z1,x2,y2,z2,x3,y3,z3);
	
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
        int err;
        while((err = glGetError()) != GL_NO_ERROR)
                fprintf(stderr,"OpenGL Error: \"%s\" in %s\n", gluErrorString((unsigned)err),str);
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

void setBrowserURL(char* file) {
        int count;
        count = strlen(file);
        if (BrowserURL != NULL) {
                free (BrowserURL);
        }
        BrowserURL = malloc (count + 1);
        strcpy(BrowserURL, file);
}

void initFreewrl() {
        threadmsg = "event loop";
        pthread_create(&mythread, NULL, (void *) aqDisplayThread, (void*) threadmsg);
        initializePerlThread("/usr/bin/perl");
        while (!isPerlinitialized()) {
                usleep(50);
        }
        initializeTextureThread();
        while (!isTextureinitialized()) {
                usleep(50);
        }

        int tmp = 0;
        perlParse(FROMURL, BrowserURL, TRUE, FALSE, rootNode, offsetof(struct VRML_Group, children), &tmp);
}

void aqDisplayThread() {
        glpOpenGLInitialize();
        new_tessellation();
        while (1) {
                EventLoop();
        }
}

void setButDown(int button, int value) {
        ButDown[button] = value;
}

void setScreenDim(int wi, int he) {
        screenWidth = wi;
        screenHeight = he;
        if (screenHeight != 0) screenRatio = (double) screenWidth/(double) screenHeight;
        else screenRatio =  screenWidth;
}

void setSnapSeq() {
        snapsequence = TRUE;
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
#endif

/* quit key pressed, or Plugin sends SIGQUIT */
void doQuit(void) {
#ifndef AQUA
	resetGeometry();
#endif
	shutdown_EAI();
	exit(0);
}
