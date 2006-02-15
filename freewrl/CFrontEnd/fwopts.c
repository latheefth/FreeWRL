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
extern int fullscreen;

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#ifdef LINUX
#include <GL/glext.h>
#endif

#include <X11/cursorfont.h>
#ifdef XF86V4
#include <X11/extensions/xf86vmode.h>
#endif
#include <X11/keysym.h>

static Colormap cmap;
static XSetWindowAttributes swa;
extern XVisualInfo *Xvi;

extern int _fw_pipe, _fw_FD;
extern unsigned _fw_instance;

static int screen;
static int modeNum;
static int bestMode;
static int quadbuff_stereo_mode;
/*
static int i;
*/

#ifdef XF86V4
XF86VidModeModeInfo **modes;
static int oldx, oldy;
#else

/* fudge calls for compiler - gosh, perl is certainly fun. */
//struct fudge { int hdisplay; int vdisplay;};
//struct fudge **modes;
//struct fudge original_display;
#endif

extern Cursor arrowc;
extern Cursor sensorc;

#define OPENGL_NOVIRT
//JAS static OpenGLVTab vtab;
//JAS OpenGLVTab *OpenGLVPtr;


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
   GLX_DEPTH_SIZE,         24,		// JAS
   GLX_RED_SIZE,           8,
   GLX_DOUBLEBUFFER,       GL_TRUE,
#ifdef GLX_STEREO
   GLX_STEREO,             GL_TRUE,
#endif
   GLX_RGBA,               GL_TRUE,
   0
   };

static int  default_attributes1[] =
   {
   GLX_DEPTH_SIZE,         16,
   GLX_RED_SIZE,           8,
   GLX_DOUBLEBUFFER,       GL_TRUE,
   GLX_RGBA,               GL_TRUE,
   0
   };

static int  default_attributes2[] =
   {
   GLX_DEPTH_SIZE,         16,
   GLX_RED_SIZE,           8,
   GLX_RGBA,               GL_TRUE,
   0
   };

static int  default_attributes3[] =
   {
   GLX_RGBA,               GL_TRUE,
   0
   };


extern int	shutterGlasses; /* stereo shutter glasses */

// Function prototypes
XVisualInfo *find_best_visual(int shutter,int *attributes,int len);
void setGeometry (char *gstring);

static int xPos = 0;
static int yPos = 0;

extern Display *Xdpy;
extern Window Xwin;
extern GLXContext GLcx;

void openMainWindow () {

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

	int XdpyWidth, XdpyHeight;

	/* get a connection */
	Xdpy = XOpenDisplay(0);
	if (!Xdpy) { fprintf(stderr, "No display!\n");exit(-1);}

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

	Xvi = find_best_visual(shutterGlasses,attributes,len);
	if(!Xvi) { fprintf(stderr, "No visual!\n");exit(-1);}

	if ((shutterGlasses) && (quadbuff_stereo_mode==0)) {
		fprintf(stderr, "Warning: No quadbuffer stereo visual found !");
		fprintf(stderr, "On SGI IRIX systems read 'man setmon' or 'man xsetmon'\n");
	}

	/* create a GLX context */
	GLcx = glXCreateContext(Xdpy, Xvi, 0, GL_FALSE);

	if(!GLcx){fprintf(stderr, "No context!\n");exit(-1);}

	/* create a color map */
	cmap = XCreateColormap(Xdpy, RootWindow(Xdpy, Xvi->screen),
				   Xvi->visual, AllocNone);

	/* create a window */
	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = event_mask;
#ifdef XF86V4
	if (fullscreen == 1) {
	 	XF86VidModeSwitchToMode(Xdpy, screen, modes[bestMode]);
	 	XF86VidModeSetViewPort(Xdpy, screen, 0, 0);
	 	XdpyWidth = modes[bestMode]->hdisplay;
	 	XdpyHeight = modes[bestMode]->vdisplay;
	 	swa.override_redirect = True;
	}

	XFree(modes);
#endif

	if(!pwin){pwin=RootWindow(Xdpy, Xvi->screen);}


	if (screenWidth>=0) {
		XTextProperty textpro;
		if (fullscreen == 1) {
			Xwin = XCreateWindow(Xdpy, pwin,
				0, 0, XdpyWidth, XdpyHeight,
				0, Xvi->depth, InputOutput, Xvi->visual,
				CWBorderPixel| CWOverrideRedirect |
				CWColormap | CWEventMask, &swa);

			cursor_pixmap = XCreatePixmap(Xdpy, Xwin ,1, 1, 1);
			black.pixel = WhitePixel(Xdpy, DefaultScreen(Xdpy));
			XQueryColor(Xdpy, DefaultColormap(Xdpy, DefaultScreen(Xdpy)), &black);
			cursor = XCreatePixmapCursor(Xdpy, cursor_pixmap, cursor_pixmap, &black, &black, 0, 0);
			XDefineCursor(Xdpy, Xwin, cursor);

		} else {
			Xwin = XCreateWindow(Xdpy, pwin,
				xPos, yPos, screenWidth, screenHeight, 0, Xvi->depth, InputOutput,
				Xvi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);

			/* create window and icon name */
			if (XStringListToTextProperty(&wintitle, 1, &windowName) == 0){
				fprintf(stderr,
					"XStringListToTextProperty failed for %s, windowName in glpcOpenWindow.\n",
					wintitle);
			}
			XSetWMName(Xdpy, Xwin, &windowName);
			XSetWMIconName(Xdpy, Xwin, &windowName);
		}

		glXMakeCurrent(Xdpy, Xwin, GLcx);
		glFlush();
		if(!Xwin) {
			fprintf(stderr, "No Window\n");
			exit(-1);
		}

		if (!RUNNINGASPLUGIN) {
			/* just map us to the display */
			XMapWindow(Xdpy, Xwin);
			XSetInputFocus(Xdpy, pwin, RevertToParent, CurrentTime);
		} else {
			/* send the window id back to the plugin parent */
			write (_fw_pipe,&Xwin,4);
			close (_fw_pipe);
		}


		//JAS if (event_mask & StructureNotifyMask) {
		//JAS 	XIfEvent(Xdpy, &event, WaitForNotify, (char*)win);
		//JAS }
		// Alberto Dubuc:
		XMoveWindow(Xdpy,Xwin,xPos,yPos);
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
		arrowc = XCreateFontCursor (Xdpy, XC_left_ptr);
		sensorc = XCreateFontCursor (Xdpy, XC_diamond_cross);
	}

	/* connect the context to the window */
	if(!glXMakeCurrent(Xdpy, Xwin, GLcx)) {
		fprintf(stderr, "Non current\n");
		exit(-1);
	}

	printf ("VEndor: %s, Renderer: %s\n",glGetString(GL_VENDOR),
		glGetString(GL_RENDERER));
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

void setGeometry (char *gstring) {
	int c;
	c = sscanf(gstring,"%dx%d+%d+%d",&screenWidth,&screenHeight,&xPos,&yPos);
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
