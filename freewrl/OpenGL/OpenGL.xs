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


#define NUM_ARG 8

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
int	render_frame = 0;	/* do we render, or do we sleep? */
int	now_mapped = 1;		/* are we on screen, or minimized? */

#define OPENGL_NOVIRT
#include "OpenGL.m"

static OpenGLVTab vtab;
OpenGLVTab *OpenGLVPtr;

static int default_attributes[] = { GLX_RGBA, /*GLX_DOUBLEBUFFER,*/  None };
static Bool WaitForNotify(Display *d, XEvent *e, char *arg) {
    return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
}
/* Mesa hack */
#undef CALLBACK
#define CALLBACK GLvoid


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
	render_frame = 1;
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
BackEndRender1()
	CODE:
	{
	render_frame = 0;

        glDisable(GL_LIGHT0); /* Put them all off first */
        glDisable(GL_LIGHT1);
        glDisable(GL_LIGHT2);
        glDisable(GL_LIGHT3);
        glDisable(GL_LIGHT4);
        glDisable(GL_LIGHT5);
        glDisable(GL_LIGHT6);
        glDisable(GL_LIGHT7);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

void
glpcOpenWindow(x,y,w,h,pw,fullscreen,event_mask, wintitle, ...)
	int	x
	int	y
	int	w
	int	h
	int	pw
	int	fullscreen
	long	event_mask
	char	*wintitle

	CODE:
	{
		XColor  black; 
		Cursor  cursor;
		Pixmap  cursor_pixmap; 
	    XEvent event;
	    Window pwin=(Window)pw;
	    int *attributes = default_attributes;

	    int number;

	    if(items>NUM_ARG){
	        int i;
	        attributes = (int *)malloc((items-NUM_ARG+1)* sizeof(int));
	        for(i=NUM_ARG;i<items;i++) {
	            attributes[i-NUM_ARG]=SvIV(ST(i));
	        }
	        attributes[items-NUM_ARG]=None;
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

	    /* get an appropriate visual */
	    vi = glXChooseVisual(dpy, screen, attributes);
	    if(!vi) { fprintf(stderr, "No visual!\n");exit(-1);}

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
	    strncpy (renderer, (char *) glGetString(GL_RENDERER), 250);
	    /* printf ("%s\n",renderer); */


	    /* and make it so that we render 1 frame, at least */
	    render_frame = 1;
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
		render_frame = 1;

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

# We have scanned up to here for unused functions. JAS.

void
gluPerspective(fovy,aspect,zNear,zFar)
	GLdouble	fovy
	GLdouble	aspect
	GLdouble	zNear
	GLdouble	zFar

GLint
gluProject(objx,objy,objz,modelMatrix,projMatrix,viewport,winx,winy,winz)
	GLdouble	objx
	GLdouble	objy
	GLdouble	objz
	char *	modelMatrix
	char *	projMatrix
	char *	viewport
	char *	winx
	char *	winy
	char *	winz
	CODE:
	{
	   gluProject(objx,objy,objz,(GLdouble *)modelMatrix,(GLdouble *)projMatrix,(GLint *)viewport,(GLdouble *)winx,(GLdouble *)winy,(GLdouble *)winz);
	}

GLint
gluUnProject(winx,winy,winz,modelMatrix,projMatrix,viewport,objx,objy,objz)
	GLdouble	winx
	GLdouble	winy
	GLdouble	winz
	char *	modelMatrix
	char *	projMatrix
	char *	viewport
	char *	objx
	char *	objy
	char *	objz
	CODE:
	{
	   gluUnProject(winx,winy,winz,(GLdouble *)modelMatrix,(GLdouble *)projMatrix,(GLint *)viewport,(GLdouble *)objx,(GLdouble *)objy,(GLdouble *)objz);
	}

const GLubyte*
gluErrorString(errorCode)
	GLenum	errorCode

GLint
gluScaleImage(format,widthin,heightin,typein,datain,widthout,heightout,typeout,dataout)
	GLenum	format
	GLint	widthin
	GLint	heightin
	GLenum	typein
	char *	datain
	GLint	widthout
	GLint	heightout
	GLenum	typeout
	char *	dataout
	CODE:
	{
	   gluScaleImage(format,widthin,heightin,typein,(void *)datain,widthout,heightout,typeout,(void *)dataout);
	}

GLint
gluBuild1DMipmaps(target,components,width,format,type,data)
	GLenum	target
	GLint	components
	GLint	width
	GLenum	format
	GLenum	type
	char *	data
	CODE:
	{
	   gluBuild1DMipmaps(target,components,width,format,type,(void *)data);
	}

GLint
gluBuild2DMipmaps(target,components,width,height,format,type,data)
	GLenum	target
	GLint	components
	GLint	width
	GLint	height
	GLenum	format
	GLenum	type
	char *	data
	CODE:
	{
	   gluBuild2DMipmaps(target,components,width,height,format,type,(void *)data);
	}

GLUquadricObj*
gluNewQuadric()

void
gluQuadricCallback(qobj,which,fn)
	char *	qobj
	GLenum	which
	char *	fn
	CODE:
	{
	   gluQuadricCallback((GLUquadricObj *)qobj,which,(CALLBACK *)fn);
	}

GLUnurbsObj*
gluNewNurbsRenderer()

GLUtriangulatorObj*
gluNewTess()

void
gluTessCallback(tobj,which,fn)
	char *	tobj
	GLenum	which
	char *	fn
	CODE:
	{
	   gluTessCallback((GLUtriangulatorObj *)tobj,which,(CALLBACK *)fn);
	}

void
gluDeleteTess(tobj)
	char *	tobj
	CODE:
	{
	   gluDeleteTess((GLUtriangulatorObj *)tobj);
	}

void
gluBeginPolygon(tobj)
	char *	tobj
	CODE:
	{
	   gluBeginPolygon((GLUtriangulatorObj *)tobj);
	}

void
gluEndPolygon(tobj)
	char *	tobj
	CODE:
	{
	   gluEndPolygon((GLUtriangulatorObj *)tobj);
	}

void
gluNextContour(tobj,type)
	char *	tobj
	GLenum	type
	CODE:
	{
	   gluNextContour((GLUtriangulatorObj *)tobj,type);
	}

void
gluTessVertex(tobj,v,data)
	char *	tobj
	char *	v
	char *	data
	CODE:
	{
	   gluTessVertex((GLUtriangulatorObj *)tobj,(GLdouble *)v,(void *)data);
	}

const GLubyte*
gluGetString(name)
	GLenum	name

XVisualInfo*
glXChooseVisual(dpy,screen,attribList)
	char *	dpy
	int	screen
	char *	attribList
	CODE:
	{
	   glXChooseVisual((Display *)dpy,screen,(int *)attribList);
	}

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

Bool
glXMakeCurrent(dpy,drawable,ctx)
	char *	dpy
	GLXDrawable	drawable
	GLXContext	ctx
	CODE:
	{
	   glXMakeCurrent((Display *)dpy,drawable,ctx);
	}

GLXPixmap
glXCreateGLXPixmap(dpy,visual,pixmap)
	char *	dpy
	char *	visual
	Pixmap	pixmap
	CODE:
	{
	   glXCreateGLXPixmap((Display *)dpy,(XVisualInfo *)visual,pixmap);
	}

void
glXDestroyGLXPixmap(dpy,pixmap)
	char *	dpy
	GLXPixmap	pixmap
	CODE:
	{
	   glXDestroyGLXPixmap((Display *)dpy,pixmap);
	}

Bool
glXQueryExtension(dpy,errorb,event)
	char *	dpy
	char *	errorb
	char *	event
	CODE:
	{
	   glXQueryExtension((Display *)dpy,(int *)errorb,(int *)event);
	}

Bool
glXQueryVersion(dpy,maj,min)
	char *	dpy
	char *	maj
	char *	min
	CODE:
	{
	   glXQueryVersion((Display *)dpy,(int *)maj,(int *)min);
	}

Bool
glXIsDirect(dpy,ctx)
	char *	dpy
	GLXContext	ctx
	CODE:
	{
	   glXIsDirect((Display *)dpy,ctx);
	}

int
glXGetConfig(dpy,visual,attrib,value)
	char *	dpy
	char *	visual
	int	attrib
	char *	value
	CODE:
	{
	   glXGetConfig((Display *)dpy,(XVisualInfo *)visual,attrib,(int *)value);
	}

GLXContext
glXGetCurrentContext()

GLXDrawable
glXGetCurrentDrawable()

void
glXWaitGL()

void
glXWaitX()

void
glXUseXFont(font,first,count,list)
	Font	font
	int	first
	int	count
	int	list

BOOT:
 {
	OpenGLVPtr = &vtab;
#define VFUNC(type,name,mem,args) (vtab.mem = name);
#include "OpenGL.vf"
#undef VFUNC
	sv_setiv(perl_get_sv("VRML::OpenGLVPtr",1),(IV)OpenGLVPtr);
 }
