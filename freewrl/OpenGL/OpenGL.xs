/* OpenGL.xs - perl calling OpenGL C functions. Trying to reduce this to zero, JAS */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/cursorfont.h>


#ifdef XF86V4
#include <X11/extensions/xf86vmode.h>
#endif

#include <X11/keysym.h>

Display *dpy;
XVisualInfo *vi;
Colormap cmap;
XSetWindowAttributes swa;
Window win;
GLXContext cx;
unsigned int width, height;

char renderer[256];	/* what device are we using? */
int screen;
int modeNum;
int bestMode;
int quadbuff_stereo_mode;
Window winDummy;
unsigned int borderDummy;
int glwinx, glwiny;
unsigned int glwinwidth, glwinheight, glwindepth;
int i;
int dpyWidth, dpyHeight;

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

Cursor arrowc;
Cursor sensorc;
int	render_frame = 5;	/* do we render, or do we sleep? */
int	now_mapped = 1;		/* are we on screen, or minimized? */

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

int  default_attributes0[] = 
   {
   GLX_DEPTH_SIZE,         16,
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








/***************************************************************************/

XVisualInfo *find_best_visual(int shutter,int *attributes,int len) {
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

/***************************************************************************/
static Bool WaitForNotify(Display *d, XEvent *e, char *arg) {
    return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
}


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
	  /* XMapRaised(dpy, win); 
	     EG : XRaiseWindow should be more appropriate */
	  XRaiseWindow(dpy, win);
	}

# should we render?
void
set_render_frame()
	CODE:
	{
	render_frame = 5; /* render a couple of frames to let events propagate */
	}

void
BackEndSleep()
	CODE:
	{
	usleep(100);
	}

int
get_render_frame()
	CODE:
	{
		RETVAL = (render_frame && now_mapped);
	}
	OUTPUT:
	RETVAL

void 
dec_render_frame()
	CODE:
	{
	if (render_frame > 0) render_frame--;
	}
	
# cursor stuff JAS
void
arrow_cursor()
	CODE:
	{ 
	XDefineCursor (dpy, win, arrowc);
	}

void
sensor_cursor()
	CODE:
	{
	XDefineCursor (dpy, win, sensorc);
	}


# GL Render loop C functions
void
BackEndClearBuffer()
	CODE:
	{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

void 
BackEndLightsOff()
	CODE:
	{
        glDisable(GL_LIGHT1); /* Put them all off first (except headlight)*/
        glDisable(GL_LIGHT2);
        glDisable(GL_LIGHT3);
        glDisable(GL_LIGHT4);
        glDisable(GL_LIGHT5);
        glDisable(GL_LIGHT6);
        glDisable(GL_LIGHT7);
	}

void 
BackEndHeadlightOff()
	CODE:
	{
	glDisable(GL_LIGHT0); /* headlight off (or other, if no headlight) */
	}


void
BackEndHeadlightOn()
	CODE:
	{
	    float pos[]={0.0, 0.0, 1.0, 0.0};
	    float s[]={1.0,1.0,1.0,1.0};
            glEnable(GL_LIGHT0);
            glLightfv(GL_LIGHT0,GL_POSITION, pos);
            glLightfv(GL_LIGHT0,GL_AMBIENT, s);
            glLightfv(GL_LIGHT0,GL_DIFFUSE, s);
            glLightfv(GL_LIGHT0,GL_SPECULAR, s);
	}

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
		XColor  black; 
		Cursor  cursor;
		Pixmap  cursor_pixmap; 
	    XEvent event;
	    Window pwin=(Window)pw;
	    int *attributes = default_attributes3;
	    int number;
            int len=0;

	   
	    if(items>NUM_ARG+1){
	       len=(items-NUM_ARG+1)* sizeof(int);
	       attributes = (int *)malloc(len*sizeof(int));
	       for(i=0;i<(items-NUM_ARG+1);i++) {
	          attributes[i]=SvIV(ST(i+NUM_ARG+1));
	       }
	    }
	    

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

	    if ((fullscreen == 1) && (ihaveXF86V4))
	    {
	    	XF86VidModeSwitchToMode(dpy, screen, modes[bestMode]);
	    	XF86VidModeSetViewPort(dpy, screen, 0, 0);
	    	dpyWidth = modes[bestMode]->hdisplay;
	    	dpyHeight = modes[bestMode]->vdisplay;
	    	swa.override_redirect = True;
	    }

	    XFree(modes);

	    if(!pwin){pwin=RootWindow(dpy, vi->screen);}
		
	    if(x>=0) {
	    	    XTextProperty textpro;
		    if (fullscreen == 1)
		    {
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
			
		    }
		    else
		    {
			win = XCreateWindow(dpy, pwin, x, y, w, h, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);

			/* create window name */
			XStoreName(dpy,win,wintitle);

		    }
	    	    glXMakeCurrent(dpy, win, cx);
	    	    glFlush();
		    XSetInputFocus(dpy, pwin, RevertToParent, CurrentTime);
		    if(!win) {
			fprintf(stderr, "No Window\n");
			exit(-1);
		    }
		    XMapWindow(dpy, win);
		    if(event_mask & StructureNotifyMask) {
			XIfEvent(dpy, &event, WaitForNotify, (char*)win);
		    }
	    } else { 
		    die("NO PBUFFER EXTENSION\n");
	    }
	    /* clear the buffer */
	    glClearColor(0,0,0,1);

	    /* Create Cursors */
	    if (fullscreen == 1)
	    {
		arrowc = cursor;
		sensorc = cursor;
	    }
	    else
	    {
	    	arrowc = XCreateFontCursor (dpy, XC_left_ptr);
	    	sensorc = XCreateFontCursor (dpy, XC_diamond_cross);
	    }

	    /* connect the context to the window */
	    if(!glXMakeCurrent(dpy, win, cx)) {
	        fprintf(stderr, "Non current\n");
	        exit(-1);
	    }
	
	    /* what is the hardware 3d accel? */
	    strncpy (renderer, (char *)glGetString(GL_RENDERER), 250);
	    /* printf ("%s\n",renderer); */

	    /* and make it so that we render 1 frame, at least */
	    render_frame = 5;
	}


void
glXSwapBuffers(d=dpy,w=win)
	void *	d
	GLXDrawable	w
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


		/* must render now */
		render_frame = 5;

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
				now_mapped = 1;
				/* printf ("now mapped\n"); */
				break;
			case UnmapNotify:
				now_mapped = 0;
				/* printf ("Now unmapped\n"); */
				break;
			default:
				EXTEND(sp,1);
				PUSHs(sv_2mortal(newSViv(event.type)));
				break;
		}
	}


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


void
glPolygonOffsetEXT(factor,bias)
	GLfloat factor
	GLfloat bias
	CODE:
	{
/* Commented out -- a bug report said this stopped the program from running somewhere :( */
/*
		#ifdef GL_EXT_polygon_offset
			extern void glPolygonOffsetEXT(GLfloat factor, GLfloat units);
			glPolygonOffsetEXT(factor,bias);
		#endif
*/
	}



void
glPolygonMode(face,mode)
        GLenum  face
        GLenum  mode


void
glClearColor(red,green,blue,alpha)
	GLclampf	red
	GLclampf	green
	GLclampf	blue
	GLclampf	alpha

void
glClear(mask)
	GLbitfield	mask


void
glBlendFunc(sfactor,dfactor)
	GLenum	sfactor
	GLenum	dfactor

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



void
glEnable(cap)
	GLenum	cap

void
glDisable(cap)
	GLenum	cap


void
glGetDoublev(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetDoublev(pname,(GLdouble *)params);
	}


void
glGetIntegerv(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetIntegerv(pname,(GLint *)params);
	}

void
glPushAttrib(mask)
	GLbitfield	mask

void
glPopAttrib()

GLint
glRenderMode(mode)
	GLenum	mode


void
glHint(target,mode)
	GLenum	target
	GLenum	mode


void
glDepthFunc(func)
	GLenum	func

void
glDrawBuffer(mode)
	GLenum	mode 

void
glMatrixMode(mode)
	GLenum	mode

void
glViewport(x,y,width,height)
	GLint	x
	GLint	y
	GLsizei	width
	GLsizei	height

void
glPushMatrix()

void
glPopMatrix()

void
glLoadIdentity()


void
glMultMatrixd(m)
	char *	m
	CODE:
	{
	   glMultMatrixd((GLdouble *)m);
	}



void
glRotatef(angle,x,y,z)
	GLfloat	angle
	GLfloat	x
	GLfloat	y
	GLfloat	z


void
glTranslatef(x,y,z)
	GLfloat	x
	GLfloat	y
	GLfloat	z




void
glShadeModel(mode)
	GLenum	mode

void
glLightfv(light,pname,params)
	GLenum	light
	GLenum	pname
	char *	params
	CODE:
	{
	   glLightfv(light,pname,(GLfloat *)params);
	}




void
glLightModeli(pname,param)
	GLenum	pname
	GLint	param


void
glMaterialf(face,pname,param)
	GLenum	face
	GLenum	pname
	GLfloat	param

void
glMaterialfv(face,pname,params)
	GLenum	face
	GLenum	pname
	char *	params
	CODE:
	{
	   glMaterialfv(face,pname,(GLfloat *)params);
	}


void
glPixelStorei(pname,param)
	GLenum	pname
	GLint	param


void
glSelectBuffer(size,buffer)
	GLsizei	size
	char *	buffer
	CODE:
	{
	   glSelectBuffer(size,(GLuint *)buffer);
	}

void
gluPerspective(fovy,aspect,zNear,zFar)
	GLdouble	fovy
	GLdouble	aspect
	GLdouble	zNear
	GLdouble	zFar


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





