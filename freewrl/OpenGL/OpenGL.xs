/* OpenGL.xs - perl calling OpenGL C functions. Trying to reduce this to zero, JAS */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"


/* #ifdef AQUA  */
/* #include <gl.h> */
/* #include <glu.h> */
/* #include <glext.h> */
/* #else */
/* #include <GL/gl.h> */
/* #include <GL/glx.h> */
/* #include <GL/glu.h> */
/* #include <GL/glext.h> */
/* #endif */

#include <unistd.h>
#include <stdio.h>

#include "OpenGL_Utils.h"

#ifndef AQUA 
#include <X11/cursorfont.h>
#ifdef XF86V4
#include <X11/extensions/xf86vmode.h>
#endif
#include <X11/keysym.h>
#endif

#ifndef AQUA
Display *dpy;
XVisualInfo *vi;
Colormap cmap;
XSetWindowAttributes swa;
Window win;
GLXContext cx;
Window winDummy;
#endif
unsigned int width, height;

char renderer[256];	/* what device are we using? */
int screen;
int modeNum;
int bestMode;
int quadbuff_stereo_mode;
unsigned int borderDummy;
int glwinx, glwiny;
unsigned int glwinwidth, glwinheight, glwindepth;
int i;
int dpyWidth, dpyHeight;

#ifndef AQUA 
#ifdef XF86V4
XF86VidModeModeInfo **modes;
XF86VidModeModeInfo original_display;
int ihaveXF86V4=TRUE;
#else

/* fudge calls for compiler - gosh, perl is certainly fun. */
int ihaveXF86V4=FALSE;
struct fudge { int hdisplay; int vdisplay;};
struct fudge **modes;
struct fudge original_display;
#endif
#endif

#ifndef AQUA 
Cursor arrowc;
Cursor sensorc;
#endif


#define OPENGL_NOVIRT

#include "OpenGL.m"

static OpenGLVTab vtab;
OpenGLVTab *OpenGLVPtr;


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



/***************************************************************************/


#ifndef AQUA 
XVisualInfo *find_best_visual(int shutter,int *attributes,int len)
{
   XVisualInfo *vi=NULL;
   int attrib;
   int startattrib=0;
   int *attrib_mem=malloc(len*sizeof(int)+sizeof(default_attributes0));

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
/***************************************************************************/
#ifndef AQUA 
static Bool WaitForNotify(Display *d, XEvent *e, char *arg)
{
    return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
}
#endif



/***************************************************************************/
MODULE = VRML::OpenGL		PACKAGE = VRML::OpenGL
PROTOTYPES: DISABLE


#
# Raise the window
#

void
raise_me_please()
	CODE:
        {
#ifndef AQUA 
	  /* XMapRaised(dpy, win); 
	     EG : XRaiseWindow should be more appropriate */
	  XRaiseWindow(dpy, win);
#endif
	}


	
# cursor stuff JAS
void
arrow_cursor()
	CODE:
	{ 
#ifndef AQUA 
	XDefineCursor (dpy, win, arrowc);
#endif
	}

void
sensor_cursor()
	CODE:
	{
#ifndef AQUA 
	XDefineCursor (dpy, win, sensorc);
#endif
	}


# GL Render loop C functions (see OpenGL_Utils.c)
void
BackEndClearBuffer()

void 
BackEndLightsOff()

void 
BackEndHeadlightOff()

void
BackEndHeadlightOn()



#define NUM_ARG 9
void
glpcOpenWindow(x,y,w,h,pw,fullscreen,shutter,event_mask, wintitle, ...)
	int	x
	int	y
	int	w
	int	h
	int	pw
	int	fullscreen
	int	shutter
	long	event_mask
	char	*wintitle

	CODE:
	{
#ifndef AQUA 
		XColor  black; 
		Cursor  cursor;
		Pixmap  cursor_pixmap; 
	    	XEvent event;
	    	Window pwin=(Window)pw;
	    	int *attributes = default_attributes3;
	    	int number;
		int len=0;
		XTextProperty windowName;
		int NotRunningAsPlugin = 0;
		int tomozilla;
	   
	if(items>NUM_ARG+1){
		len=(items-NUM_ARG+1)* sizeof(int);
		attributes = (int *)malloc(len*sizeof(int));
		for(i=0;i<(items-NUM_ARG+1);i++) {
			attributes[i]=SvIV(ST(i+NUM_ARG+1));
		}
	}
	    

	NotRunningAsPlugin = strncmp ("pipe:",wintitle,5);

	/* JAS if (NotRunningAsPlugin) { */
	/* JAS 	printf ("NOT Running as plugin %d\n",NotRunningAsPlugin); */
	/* JAS } else { */
	/* JAS 	printf ("Running as plugin %d\n",NotRunningAsPlugin); */
	/* JAS } */


	/* get a connection */
	dpy = XOpenDisplay(0);
	if (!dpy) { fprintf(stderr, "No display!\n");exit(-1);}

	screen = DefaultScreen(dpy);

	if (ihaveXF86V4) {
		XF86VidModeGetAllModeLines(dpy, screen, &modeNum, &modes);

		bestMode = 0;
	
		for (i=0; i < modeNum; i++) {
			if ((modes[i]->hdisplay == w) && (modes[i]->vdisplay==h)) {
				bestMode = i;
			}
		}
		original_display = *modes[0];
	}

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

	if ((fullscreen == 1) && (ihaveXF86V4)) {
		XF86VidModeSwitchToMode(dpy, screen, modes[bestMode]);
		XF86VidModeSetViewPort(dpy, screen, 0, 0);
		dpyWidth = modes[bestMode]->hdisplay;
		dpyHeight = modes[bestMode]->vdisplay;
		swa.override_redirect = True;
	}

	XFree(modes);

	if(!pwin){pwin=RootWindow(dpy, vi->screen);}
		

	if (x>=0) {
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
				x, y, w, h, 0, vi->depth, InputOutput, 
				vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);

			/* create window and icon name */
			if (XStringListToTextProperty(&wintitle, 1, &windowName) == 0){
				fprintf(stderr,
					"XStringListToTextProperty failed for %s, windowName in glpcOpenWindow.\n",
					wintitle);
			} 
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


		XSetInputFocus(dpy, pwin, RevertToParent, CurrentTime);
		if (NotRunningAsPlugin) {
			/* just map us to the display */
			XMapWindow(dpy, win);
		} else {
			/* send the window id back to the plugin parent */
			sscanf (wintitle,"pipe:%d",&tomozilla);
			/* JAS printf ("mozilla pipe is %d\n",tomozilla); */
			write (tomozilla,&win,4);
			close (tomozilla);
		}


		if (event_mask & StructureNotifyMask) {
			XIfEvent(dpy, &event, WaitForNotify, (char*)win);
		}
	} else { 
		die("NO PBUFFER EXTENSION\n");
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

	/* what is the hardware 3d accel? */
	/* strncpy (renderer, (char *)glGetString(GL_RENDERER), 250); */
	/* printf ("%s\n",renderer); */

	/* glXGetConfig (dpy, vi, GLX_DEPTH_SIZE, &number); */
	   /* printf ("GLX depth size %d\n",number); */
#endif


	// For Vertex arrays - we always assume these are enabled.
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_NORMAL_ARRAY);
}

void
glpOpenGLInitialize()



#ifndef AQUA 
void
glXSwapBuffers(d=dpy,w=win)
	void *	d
	GLXDrawable  w
	CODE:
	{
	    glXSwapBuffers(d,w);
	}


int
XPending(d=dpy)
	void *	d
	CODE:
	{
		RETVAL = XPending(d);
	}
	OUTPUT:
	RETVAL

void
glpXNextEvent(d=dpy)
	void *	d
	PPCODE:
	{
		XEvent event;
		char buf[10];
		KeySym ks;
		XNextEvent(d,&event);
		switch(event.type) {
			case ConfigureNotify:
				EXTEND(sp,3);
				PUSHs(sv_2mortal(newSViv(event.type)));
				PUSHs(sv_2mortal(newSViv(event.xconfigure.width)));
				PUSHs(sv_2mortal(newSViv(event.xconfigure.height)));				
				break;
			case KeyPress:
			case KeyRelease:
				EXTEND(sp,3);
				PUSHs(sv_2mortal(newSViv(event.type)));
				XLookupString(&event.xkey,buf,sizeof(buf),&ks,0);
				// Map keypad keys in - thanks to Aubrey Jaffer.
				// JAS - num lock had an effect, so added 
				// more definitions.
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
				PUSHs(sv_2mortal(newSVpv(buf,1)));
				PUSHs(sv_2mortal(newSViv(event.xkey.state)));
				break;
			case ButtonPress:
			case ButtonRelease:
				EXTEND(sp,7);
				PUSHs(sv_2mortal(newSViv(event.type)));
				PUSHs(sv_2mortal(newSViv(event.xbutton.button)));
				PUSHs(sv_2mortal(newSViv(event.xbutton.x)));
				PUSHs(sv_2mortal(newSViv(event.xbutton.y)));
				PUSHs(sv_2mortal(newSViv(event.xbutton.x_root)));
				PUSHs(sv_2mortal(newSViv(event.xbutton.y_root)));
				PUSHs(sv_2mortal(newSViv(event.xbutton.state)));
				break;
			case MotionNotify:
				EXTEND(sp,4);
				PUSHs(sv_2mortal(newSViv(event.type)));
				PUSHs(sv_2mortal(newSViv(event.xmotion.state)));
				PUSHs(sv_2mortal(newSViv(event.xmotion.x)));
				PUSHs(sv_2mortal(newSViv(event.xmotion.y)));
				break;
			case Expose:
				break;
			case MapNotify:
				/* now_mapped = 1; */
				set_now_mapped(TRUE);
				/* printf ("now mapped\n"); */
				break;
			case UnmapNotify:
				/* now_mapped = 0; */
				set_now_mapped(FALSE);
				/* printf ("Now unmapped\n"); */
				break;
			default:
				EXTEND(sp,1);
				PUSHs(sv_2mortal(newSViv(event.type)));
				break;
		}
}

#endif


# used by VRML::GLBackEnd::snapshot

void
glReadPixels(x,y,width,height,format,type,pixels)
	GLint	x
	GLint	y
	GLsizei	width
	GLsizei	height
	GLenum	format
	GLenum	type
	char *	pixels
	CODE:
	{
	   glReadPixels(x,y,width,height,format,type,(GLvoid *)pixels);
	}

#ifndef AQUA 
int
glpRasterFont(name,base,number,d=dpy)
	char *name
	int base
	int number
	void *d
	CODE:
	{
		XFontStruct *fi;
		int lb;
		fi = XLoadQueryFont(d,name);
		if(fi == NULL) {
			die("No font %s found",name);
		}
		lb = glGenLists(number);
		if(lb == 0) {
			die("No display lists left for font %s (need %d)",name,number);
		}
		glXUseXFont(fi->fid, base, number, lb);
		RETVAL=lb;
	}
	OUTPUT:
	RETVAL

#endif

void
glpPrintString(base,str)
	int base
	char *str
	CODE:
	{
		glPushAttrib(GL_LIST_BIT);
		glListBase(base);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,(GLubyte*)str);
		glPopAttrib();
	}

#
# The following XSUBS were done by hand
# These are perl-ized versions of the corresponding opengl function
# The reason is that the API with respect to 
# arguments and/or return value differs from the C equivalent
# These functions are more elegant to call and provide better error checking
# than the equivalent counter-part that needs pointer arguments
#

# used by VRML::GLBackEnd::setup_projection

void
glupPickMatrix(x,y,width,height,vp1,vp2,vp3,vp4)
	GLdouble	x
	GLdouble	y
	GLdouble	width
	GLdouble	height
	GLint vp1
	GLint vp2
	GLint vp3
	GLint vp4
	CODE:
	{
		GLint vp[4];
		vp[0] = vp1; vp[1]=vp2; vp[2]=vp3; vp[3]=vp4;
		gluPickMatrix(x,y,width,height,vp);
	}



# used by VRML::NodeType::init_image and VRML::NodeType::init_(movie|pixel)_image

int
glGenTexture()
	CODE:
	{ 
	GLuint texture;	
	glGenTextures(1, &texture);
	RETVAL = texture;
	}
	OUTPUT:
	RETVAL



# used by VRML::GLBackEnd::setup_projection

void
glGetIntegerv(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetIntegerv(pname,(GLint *)params);
	}


# used by VRML::GLBackEnd::render

void
glDrawBuffer(mode)
	GLenum	mode 

# used by VRML::GLBackEnd::setup_projection and VRML::GLBackEnd::setup_viewpoint

void
glMatrixMode(mode)
	GLenum	mode

# used by VRML::GLBackEnd::setup_projection

void
glViewport(x,y,width,height)
	GLint	x
	GLint	y
	GLsizei	width
	GLsizei	height


# used by VRML::GLBackEnd::setup_projection and VRML::GLBackEnd::setup_viewpoint

void
glLoadIdentity()


# used by VRML::GLBackEnd::set_fast, which is called by freewrl.PL

void
glShadeModel(mode)
	GLenum	mode


# used by VRML::GLBackEnd::snapshot

void
glPixelStorei(pname,param)
	GLenum	pname
	GLint	param



#ifndef AQUA 
void
glXDestroyContext()
	CODE:
	{
	  if (ihaveXF86V4) {
	   /* had a problem with this killing some older video cards display */
	   if (strcmp(renderer,"Mesa X11") != 0) {
	     /* printf ("destroying context\n"); */
  	     XF86VidModeSwitchToMode((Display*) dpy, DefaultScreen((Display*)dpy), &original_display);
	     XF86VidModeSetViewPort((Display*) dpy, DefaultScreen((Display*)dpy), 0, 0);
	     glXDestroyContext((Display *)dpy,cx);
	   }
	  } else {
	     glXDestroyContext((Display *)dpy,cx);
	  }
	}

#endif

void
glPrintError(str)
	char *str
	CODE:
	{
	int err; 
	while((err = glGetError()) != GL_NO_ERROR) 
		fprintf(stderr,"OpenGL Error: \"%s\" in %s\n", gluErrorString(err),str); 
	}




BOOT:
 {
	OpenGLVPtr = &vtab;
#define VFUNC(type,name,mem,args) (vtab.mem = name);
#include "OpenGL.vf"
#undef VFUNC
	sv_setiv(perl_get_sv("VRML::OpenGLVPtr",1),(IV)OpenGLVPtr);
 }





