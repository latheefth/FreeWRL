/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#define RUNNINGASPLUGIN (_fw_pipe != 0)
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <vrmlconf.h>

extern int screenWidth, screenHeight;
int fullscreen;

/* Petr Mikulik - IndexedLineSet line width */
float global_linewidth = 1.0;


#ifndef AQUA

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#endif

#ifdef LINUX
#include <GL/glext.h>
#endif

#ifndef AQUA
#include <X11/cursorfont.h>
#ifdef XF86V4
#include <X11/extensions/xf86vmode.h>
#endif
#include <X11/keysym.h>
#endif

#ifndef AQUA
Colormap cmap;
XSetWindowAttributes swa;
Window winDummy;
XVisualInfo *vi;
	Display *dpy;
	Window win;
	GLXContext cx;
#endif

extern int _fw_pipe, _fw_FD;
extern unsigned _fw_instance;

int screen;
int modeNum;
int bestMode;
int quadbuff_stereo_mode;
unsigned int borderDummy;
int glwinx, glwiny;
int i;
int dpyWidth, dpyHeight;

#ifndef AQUA
#ifdef XF86V4
XF86VidModeModeInfo **modes;
int oldx, oldy;
#else

/* fudge calls for compiler - gosh, perl is certainly fun. */
//struct fudge { int hdisplay; int vdisplay;};
//struct fudge **modes;
//struct fudge original_display;
#endif
#endif

#ifndef AQUA
Cursor arrowc;
Cursor sensorc;
#endif


#define OPENGL_NOVIRT
//JAS static OpenGLVTab vtab;
//JAS OpenGLVTab *OpenGLVPtr;


/*
static int default_attributes[] = { GLX_RGBA , GL_TRUE, GLX_DOUBLEBUFFER, GL_TRUE, None };
*/

/*
   from similar code in white_dune 8-)
   test for best visual you can get
   with best attribut list
   with maximal possible colorsize
   with maximal possible depth
 */

int legal_depth_list[] = { 32, 24, 16, 15, 8, 4, 1 };

#ifndef AQUA
int  default_attributes0[] =
   {
   GLX_DEPTH_SIZE,         24,		// JAS
   GLX_RED_SIZE,           8,
   GLX_DOUBLEBUFFER,       GL_TRUE,
#ifdef GLX_STEREO
   GLX_STEREO,             GL_TRUE,
#endif
   GLX_RGBA,               GL_TRUE,
   0
   };

int  default_attributes1[] =
   {
   GLX_DEPTH_SIZE,         16,
   GLX_RED_SIZE,           8,
   GLX_DOUBLEBUFFER,       GL_TRUE,
   GLX_RGBA,               GL_TRUE,
   0
   };

int  default_attributes2[] =
   {
   GLX_DEPTH_SIZE,         16,
   GLX_RED_SIZE,           8,
   GLX_RGBA,               GL_TRUE,
   0
   };

int  default_attributes3[] =
   {
   GLX_RGBA,               GL_TRUE,
   0
   };
#endif


int	shutter = 0; /* stereo shutter glasses */

// Function prototypes
#ifndef AQUA
XVisualInfo *find_best_visual(int shutter,int *attributes,int len);
#endif
void setGeometry (char *gstring);

static int xPos = 0;
static int yPos = 0;
#ifndef AQUA
void openMainWindow (Display *Disp, unsigned *Win,
		GLXContext *Cont) {

	int	pw = 0;
	long	event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
				ButtonMotionMask | ButtonReleaseMask |
				ExposureMask | StructureNotifyMask |
				PointerMotionMask;

	char	*wintitle =  "FreeWRL VRML/X3D Browser";


	XColor  black;
	Cursor  cursor;
	Pixmap  cursor_pixmap;
    	XEvent event;
    	Window pwin=(Window)pw;
    	int *attributes = default_attributes3;
    	int number;
	int len=0;
	XTextProperty windowName;

	int items=0; // jas

	//JAS if(items>NUM_ARG+1){
	//JAS 	len=(items-NUM_ARG+1)* sizeof(int);
	//JAS 	attributes = (int *)malloc(len*sizeof(int));
	//JAS 	for(i=0;i<(items-NUM_ARG+1);i++) {
	//JAS 		attributes[i]=SvIV(ST(i+NUM_ARG+1));
	//JAS 	}
	//JAS }


	/* get a connection */
	dpy = XOpenDisplay(0);
	if (!dpy) { fprintf(stderr, "No display!\n");exit(-1);}

	bestMode = -1;
	screen = DefaultScreen(dpy);
#ifdef XF86V4
	 	XF86VidModeGetAllModeLines(dpy, screen, &modeNum, &modes);

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
		XF86VidModeGetViewPort(dpy, DefaultScreen(dpy), &oldx, &oldy);
#endif

	vi = find_best_visual(shutter,attributes,len);
	if(!vi) { fprintf(stderr, "No visual!\n");exit(-1);}

	if ((shutter) && (quadbuff_stereo_mode==0)) {
		fprintf(stderr, "Warning: No quadbuffer stereo visual found !");
		fprintf(stderr, "On SGI IRIX systems read 'man setmon' or 'man xsetmon'\n");
	}

	/* create a GLX context */
	cx = glXCreateContext(dpy, vi, 0, GL_TRUE);

	if(!cx){fprintf(stderr, "No context!\n");exit(-1);}

	/* create a color map */
	cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
				   vi->visual, AllocNone);

	/* create a window */
	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = event_mask;
#ifdef XF86V4
	if (fullscreen == 1) {
	 	XF86VidModeSwitchToMode(dpy, screen, modes[bestMode]);
	 	XF86VidModeSetViewPort(dpy, screen, 0, 0);
	 	dpyWidth = modes[bestMode]->hdisplay;
	 	dpyHeight = modes[bestMode]->vdisplay;
	 	swa.override_redirect = True;
	}

	XFree(modes);
#endif

	if(!pwin){pwin=RootWindow(dpy, vi->screen);}


	if (screenWidth>=0) {
		XTextProperty textpro;
		if (fullscreen == 1) {
			win = XCreateWindow(dpy, pwin,
				0, 0, dpyWidth, dpyHeight,
				0, vi->depth, InputOutput, vi->visual,
				CWBorderPixel| CWOverrideRedirect |
				CWColormap | CWEventMask, &swa);

			cursor_pixmap = XCreatePixmap(dpy, win ,1, 1, 1);
			black.pixel = WhitePixel(dpy, DefaultScreen(dpy));
			XQueryColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &black);
			cursor = XCreatePixmapCursor(dpy, cursor_pixmap, cursor_pixmap, &black, &black, 0, 0);
			XDefineCursor(dpy, win, cursor);

		} else {
			win = XCreateWindow(dpy, pwin,
				xPos, yPos, screenWidth, screenHeight, 0, vi->depth, InputOutput,
				vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);

			/* create window and icon name */
			if (XStringListToTextProperty(&wintitle, 1, &windowName) == 0){
				fprintf(stderr,
					"XStringListToTextProperty failed for %s, windowName in glpcOpenWindow.\n",
					wintitle);
			}
			XSetWMName(dpy, win, &windowName);
			XSetWMIconName(dpy, win, &windowName);
		}

		glXMakeCurrent(dpy, win, cx);
		glFlush();
		if(!win) {
			fprintf(stderr, "No Window\n");
			exit(-1);
		}

		if (!RUNNINGASPLUGIN) {
			/* just map us to the display */
			XMapWindow(dpy, win);
			XSetInputFocus(dpy, pwin, RevertToParent, CurrentTime);
		} else {
			/* send the window id back to the plugin parent */
			write (_fw_pipe,&win,4);
			close (_fw_pipe);
		}


		//JAS if (event_mask & StructureNotifyMask) {
		//JAS 	XIfEvent(dpy, &event, WaitForNotify, (char*)win);
		//JAS }
		// Alberto Dubuc:
		XMoveWindow(dpy,win,xPos,yPos);
	} else {
		printf ("NO PBUFFER EXTENSION\n");
		exit(1);
	}

	/* clear the buffer */
	glClearColor(0,0,0,1);

	/* Create Cursors */
	if (fullscreen == 1) {
		arrowc = cursor;
		sensorc = cursor;
	} else {
		arrowc = XCreateFontCursor (dpy, XC_left_ptr);
		sensorc = XCreateFontCursor (dpy, XC_diamond_cross);
	}

	/* connect the context to the window */
	if(!glXMakeCurrent(dpy, win, cx)) {
		fprintf(stderr, "Non current\n");
		exit(-1);
	}


	// For Vertex arrays - we always assume these are enabled.
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_NORMAL_ARRAY);

	// Set Line Width from the command line parameter
	// for GL_LINES and GL_LINE_STRIP and switch their anti-aliasing
	glLineWidth (global_linewidth);
	glPointSize(global_linewidth);
	glEnable(GL_LINE_SMOOTH);

	// return Display and window
	Disp = dpy;
	*Win =  win;
	*Cont = cx;

	//printf ("VEndor: %s, Renderer: %s\n",glGetString(GL_VENDOR),
	//	glGetString(GL_RENDERER));
}
#endif


#ifndef AQUA
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
            vi = glXChooseVisual(dpy, screen, attrib_mem);
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
#endif


void setGeometry (char *gstring) {
	int c;
	c = sscanf(gstring,"%dx%d+%d+%d",&screenWidth,&screenHeight,&xPos,&yPos);
}

void resetGeometry() {
#ifdef XF86V4
		XF86VidModeModeInfo info;
		int oldMode;

	if (fullscreen) {
	 	XF86VidModeGetAllModeLines(dpy, screen, &modeNum, &modes);
 		oldMode = 0;

 		for (i=0; i < modeNum; i++) {
 			if ((modes[i]->hdisplay == oldx) && (modes[i]->vdisplay==oldy)) {
 				oldMode = i;
				break;
 			}
 		}

	 	XF86VidModeSwitchToMode(dpy, screen, modes[oldMode]);
	 	XF86VidModeSetViewPort(dpy, screen, 0, 0);
		XFlush(dpy);
	}

#endif
}

/* handle setting shutter from parameters */
void setShutter (void) {
	shutter = 1;
}
