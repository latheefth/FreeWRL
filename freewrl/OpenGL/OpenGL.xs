/* OpenGL.xs - note that generate.p is no longer used... */

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
int XF86V4=TRUE;
#else
/* fudge calls for compiler - gosh, perl is certainly fun. */
int XF86V4=FALSE;
struct fudge { int hdisplay; int vdisplay;};
struct fudge **modes;
struct fudge original_display;
#endif

Cursor arrowc;
Cursor sensorc;

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
#define CALLBACK void


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

	    XF86VidModeGetAllModeLines(dpy, screen, &modeNum, &modes);

	    bestMode = 0;
	
	    for (i=0; i < modeNum; i++)
	    {
		if ((modes[i]->hdisplay == w) && (modes[i]->vdisplay==h))
		{
			bestMode = i;
		}
	    }
	   original_display = *modes[0];

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

	    if (fullscreen == 1)
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
	    strncpy (renderer, glGetString(GL_RENDERER), 250);
	    /* printf ("%s\n",renderer); */
	}

int
glpXConnectionNumber(d=dpy)
	void *d
	CODE:
	{
		Display *dp = d;
		RETVAL = ConnectionNumber(dp);
	}
	OUTPUT:
	RETVAL

# If glpOpenWindow was used then glXSwapBuffers should be called
# without parameters (i.e. use the default parameters)

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
			default:
				EXTEND(sp,1);
				PUSHs(sv_2mortal(newSViv(event.type)));
				break;
		}
	}

void
glpXQueryPointer(d=dpy,w=win)
	void *	d
	GLXDrawable	w
	PPCODE:
	{
		int x,y,rx,ry;
		Window r,c;
		unsigned int m;
		XQueryPointer(d,w,&r,&c,&rx,&ry,&x,&y,&m);
		EXTEND(sp,3);
		PUSHs(sv_2mortal(newSViv(x)));
		PUSHs(sv_2mortal(newSViv(y)));
		PUSHs(sv_2mortal(newSViv(m)));
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
glpClipPlane(p,a,b,c,d)
	GLenum	p
	GLdouble	a
	GLdouble	b
	GLdouble	c
	GLdouble	d
	CODE:
	{
		GLdouble e[4];
		e[0]=a;e[1]=b;e[2]=c;e[3]=d;
		glClipPlane(p,e);
	}

void
glpGetClipPlane(plane)
	GLenum	plane
	PPCODE:
	{
	    GLdouble equation[4];
	    glGetClipPlane(plane,equation);
	    EXTEND(sp,4);
	    PUSHs(sv_2mortal(newSVnv(equation[0])));
	    PUSHs(sv_2mortal(newSVnv(equation[1])));
	    PUSHs(sv_2mortal(newSVnv(equation[2])));
	    PUSHs(sv_2mortal(newSVnv(equation[3])));
	}


void
glpLoadMatrixd(m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,ma,mb,mc,md,me,mf)
	GLdouble	m0
	GLdouble	m1
	GLdouble	m2
	GLdouble	m3
	GLdouble	m4
	GLdouble	m5
	GLdouble	m6
	GLdouble	m7
	GLdouble	m8
	GLdouble	m9
	GLdouble	ma
	GLdouble	mb
	GLdouble	mc
	GLdouble	md
	GLdouble	me
	GLdouble	mf
	CODE:
	{
		GLdouble m[16];
		m[0]= m0; m[1]= m1; m[2]= m2; m[3]= m3;
		m[4]= m4; m[5]= m5; m[6]= m6; m[7]= m7;
		m[8]= m8; m[9]= m9; m[10]=ma; m[11]=mb;
		m[12]=mc; m[13]=md; m[14]=me; m[15]=mf;
		glLoadMatrixd(m);
	}

void
glpMultMatrixd(m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,ma,mb,mc,md,me,mf)
	GLdouble	m0
	GLdouble	m1
	GLdouble	m2
	GLdouble	m3
	GLdouble	m4
	GLdouble	m5
	GLdouble	m6
	GLdouble	m7
	GLdouble	m8
	GLdouble	m9
	GLdouble	ma
	GLdouble	mb
	GLdouble	mc
	GLdouble	md
	GLdouble	me
	GLdouble	mf
	CODE:
	{
		GLdouble m[16];
		m[0]= m0; m[1]= m1; m[2]= m2; m[3]= m3;
		m[4]= m4; m[5]= m5; m[6]= m6; m[7]= m7;
		m[8]= m8; m[9]= m9; m[10]=ma; m[11]=mb;
		m[12]=mc; m[13]=md; m[14]=me; m[15]=mf;
		glMultMatrixd(m);
	}

void
glpLoadMatrixf(m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,ma,mb,mc,md,me,mf)
	GLfloat	m0
	GLfloat	m1
	GLfloat	m2
	GLfloat	m3
	GLfloat	m4
	GLfloat	m5
	GLfloat	m6
	GLfloat	m7
	GLfloat	m8
	GLfloat	m9
	GLfloat	ma
	GLfloat	mb
	GLfloat	mc
	GLfloat	md
	GLfloat	me
	GLfloat	mf
	CODE:
	{
		GLfloat m[16];
		m[0]= m0; m[1]= m1; m[2]= m2; m[3]= m3;
		m[4]= m4; m[5]= m5; m[6]= m6; m[7]= m7;
		m[8]= m8; m[9]= m9; m[10]=ma; m[11]=mb;
		m[12]=mc; m[13]=md; m[14]=me; m[15]=mf;
		glLoadMatrixf(m);
	}


void
glpMultMatrixf(m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,ma,mb,mc,md,me,mf)
	GLfloat	m0
	GLfloat	m1
	GLfloat	m2
	GLfloat	m3
	GLfloat	m4
	GLfloat	m5
	GLfloat	m6
	GLfloat	m7
	GLfloat	m8
	GLfloat	m9
	GLfloat	ma
	GLfloat	mb
	GLfloat	mc
	GLfloat	md
	GLfloat	me
	GLfloat	mf
	CODE:
	{
		GLfloat m[16];
		m[0]= m0; m[1]= m1; m[2]= m2; m[3]= m3;
		m[4]= m4; m[5]= m5; m[6]= m6; m[7]= m7;
		m[8]= m8; m[9]= m9; m[10]=ma; m[11]=mb;
		m[12]=mc; m[13]=md; m[14]=me; m[15]=mf;
		glMultMatrixf(m);
	}


#
# Here are the glu ones that have been done so far:
#


void
gluOrtho2D(left,right,bottom,top)
	GLdouble	left
	GLdouble	right
	GLdouble	bottom
	GLdouble	top

# void
# gluPerspective(fovy,aspect,zNear,zFar)
# 	GLdouble	fovy
# 	GLdouble	aspect
# 	GLdouble	zNear
# 	GLdouble	zFar

void
gluLookAt(eyex,eyey,eyez,centerx,centery,centerz,upx,upy,upz)
	GLdouble	eyex
	GLdouble	eyey
	GLdouble	eyez
	GLdouble	centerx
	GLdouble	centery
	GLdouble	centerz
	GLdouble	upx
	GLdouble	upy
	GLdouble	upz


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




#
#  Some of the following XSUBS functions have 1 or more pointers as arguments
#  The type was changed to "char *" for each of these so you can
#  use perl's pack() routine to pass the required data to the C function.
#  See examples/texture for an example using glTexImage2D().
#  Be sure to pack your data properly (see the man page for the 
#  function) there is no sanity check here.  Use a glpFunctionName() or
#  a non-vector equivalent wherever possible
#
#  and now the gl XSUBs that were generated: 
#
##################################################



void
glClearIndex(c)
	GLfloat	c

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
glIndexMask(mask)
	GLuint	mask

void
glColorMask(red,green,blue,alpha)
	GLboolean	red
	GLboolean	green
	GLboolean	blue
	GLboolean	alpha

void
glAlphaFunc(func,ref)
	GLenum	func
	GLclampf	ref

void
glBlendFunc(sfactor,dfactor)
	GLenum	sfactor
	GLenum	dfactor

void
glLogicOp(opcode)
	GLenum	opcode

void
glCullFace(mode)
	GLenum	mode

void
glFrontFace(mode)
	GLenum	mode

void
glPointSize(size)
	GLfloat	size

void
glLineWidth(width)
	GLfloat	width

void
glLineStipple(factor,pattern)
	GLint	factor
	GLushort	pattern

void
glPolygonMode(face,mode)
	GLenum	face
	GLenum	mode

void
glPolygonStipple(mask)
	char *	mask
	CODE:
	{
	   glPolygonStipple((GLubyte *)mask);
	}

void
glGetPolygonStipple(mask)
	char *	mask
	CODE:
	{
	   glGetPolygonStipple((GLubyte *)mask);
	}

void
glEdgeFlag(flag)
	GLboolean	flag

void
glEdgeFlagv(flag)
	char *	flag
	CODE:
	{
	   glEdgeFlagv((GLboolean *)flag);
	}

void
glScissor(x,y,width,height)
	GLint	x
	GLint	y
	GLsizei	width
	GLsizei	height

void
glClipPlane(plane,equation)
	GLenum	plane
	char *	equation
	CODE:
	{
	   glClipPlane(plane,(GLdouble *)equation);
	}

void
glGetClipPlane(plane,equation)
	GLenum	plane
	char *	equation
	CODE:
	{
	   glGetClipPlane(plane,(GLdouble *)equation);
	}

void
glDrawBuffer(mode)
	GLenum	mode

void
glReadBuffer(mode)
	GLenum	mode

void
glEnable(cap)
	GLenum	cap

void
glDisable(cap)
	GLenum	cap

GLboolean
glIsEnabled(cap)
	GLenum	cap

void
glGetBooleanv(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetBooleanv(pname,(GLboolean *)params);
	}

void
glGetDoublev(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetDoublev(pname,(GLdouble *)params);
	}

void
glGetFloatv(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetFloatv(pname,(GLfloat *)params);
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

GLenum
glGetError()

const GLubyte*
glGetString(name)
	GLenum	name

void
glFinish()

void
glFlush()

void
glHint(target,mode)
	GLenum	target
	GLenum	mode

void
glClearDepth(depth)
	GLclampd	depth

void
glDepthFunc(func)
	GLenum	func

void
glDepthMask(flag)
	GLboolean	flag

void
glDepthRange(near_val,far_val)
	GLclampd	near_val
	GLclampd	far_val

void
glClearAccum(red,green,blue,alpha)
	GLfloat	red
	GLfloat	green
	GLfloat	blue
	GLfloat	alpha

void
glAccum(op,value)
	GLenum	op
	GLfloat	value

void
glMatrixMode(mode)
	GLenum	mode

void
glOrtho(left,right,bottom,top,near_val,far_val)
	GLdouble	left
	GLdouble	right
	GLdouble	bottom
	GLdouble	top
	GLdouble	near_val
	GLdouble	far_val

void
glFrustum(left,right,bottom,top,near_val,far_val)
	GLdouble	left
	GLdouble	right
	GLdouble	bottom
	GLdouble	top
	GLdouble	near_val
	GLdouble	far_val

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
glLoadMatrixd(m)
	char *	m
	CODE:
	{
	   glLoadMatrixd((GLdouble *)m);
	}

void
glLoadMatrixf(m)
	char *	m
	CODE:
	{
	   glLoadMatrixf((GLfloat *)m);
	}

void
glMultMatrixd(m)
	char *	m
	CODE:
	{
	   glMultMatrixd((GLdouble *)m);
	}

void
glMultMatrixf(m)
	char *	m
	CODE:
	{
	   glMultMatrixf((GLfloat *)m);
	}

void
glRotated(angle,x,y,z)
	GLdouble	angle
	GLdouble	x
	GLdouble	y
	GLdouble	z

void
glRotatef(angle,x,y,z)
	GLfloat	angle
	GLfloat	x
	GLfloat	y
	GLfloat	z

void
glScaled(x,y,z)
	GLdouble	x
	GLdouble	y
	GLdouble	z

void
glScalef(x,y,z)
	GLfloat	x
	GLfloat	y
	GLfloat	z

void
glTranslated(x,y,z)
	GLdouble	x
	GLdouble	y
	GLdouble	z

void
glTranslatef(x,y,z)
	GLfloat	x
	GLfloat	y
	GLfloat	z

GLboolean
glIsList(list)
	GLuint	list

void
glDeleteLists(list,range)
	GLuint	list
	GLsizei	range

void
glGenTextures(n,textures)
	GLsizei	n
	GLuint *	textures

void
glBindTexture(target,texture)
	GLenum target
	GLuint texture

void
glDeleteTextures(n,textures)
	GLsizei	n
	GLuint *	textures

GLuint
glGenLists(range)
	GLsizei	range

void
glNewList(list,mode)
	GLuint	list
	GLenum	mode

void
glEndList()

void
glCallList(list)
	GLuint	list

void
glCallLists(n,type,lists)
	GLsizei	n
	GLenum	type
	char *	lists
	CODE:
	{
	   glCallLists(n,type,(GLvoid *)lists);
	}

void
glListBase(base)
	GLuint	base

void
glBegin(mode)
	GLenum	mode

void
glEnd()

void
glVertex2d(x,y)
	GLdouble	x
	GLdouble	y

void
glVertex2f(x,y)
	GLfloat	x
	GLfloat	y

void
glVertex2i(x,y)
	GLint	x
	GLint	y

void
glVertex2s(x,y)
	GLshort	x
	GLshort	y

void
glVertex3d(x,y,z)
	GLdouble	x
	GLdouble	y
	GLdouble	z

void
glVertex3f(x,y,z)
	GLfloat	x
	GLfloat	y
	GLfloat	z

void
glVertex3i(x,y,z)
	GLint	x
	GLint	y
	GLint	z

void
glVertex3s(x,y,z)
	GLshort	x
	GLshort	y
	GLshort	z

void
glVertex4d(x,y,z,w)
	GLdouble	x
	GLdouble	y
	GLdouble	z
	GLdouble	w

void
glVertex4f(x,y,z,w)
	GLfloat	x
	GLfloat	y
	GLfloat	z
	GLfloat	w

void
glVertex4i(x,y,z,w)
	GLint	x
	GLint	y
	GLint	z
	GLint	w

void
glVertex4s(x,y,z,w)
	GLshort	x
	GLshort	y
	GLshort	z
	GLshort	w

void
glVertex2dv(v)
	char *	v
	CODE:
	{
	   glVertex2dv((GLdouble *)v);
	}

void
glVertex2fv(v)
	char *	v
	CODE:
	{
	   glVertex2fv((GLfloat *)v);
	}

void
glVertex2iv(v)
	char *	v
	CODE:
	{
	   glVertex2iv((GLint *)v);
	}

void
glVertex2sv(v)
	char *	v
	CODE:
	{
	   glVertex2sv((GLshort *)v);
	}

void
glVertex3dv(v)
	char *	v
	CODE:
	{
	   glVertex3dv((GLdouble *)v);
	}

void
glVertex3fv(v)
	char *	v
	CODE:
	{
	   glVertex3fv((GLfloat *)v);
	}

void
glVertex3iv(v)
	char *	v
	CODE:
	{
	   glVertex3iv((GLint *)v);
	}

void
glVertex3sv(v)
	char *	v
	CODE:
	{
	   glVertex3sv((GLshort *)v);
	}

void
glVertex4dv(v)
	char *	v
	CODE:
	{
	   glVertex4dv((GLdouble *)v);
	}

void
glVertex4fv(v)
	char *	v
	CODE:
	{
	   glVertex4fv((GLfloat *)v);
	}

void
glVertex4iv(v)
	char *	v
	CODE:
	{
	   glVertex4iv((GLint *)v);
	}

void
glVertex4sv(v)
	char *	v
	CODE:
	{
	   glVertex4sv((GLshort *)v);
	}

void
glNormal3b(nx,ny,nz)
	GLbyte	nx
	GLbyte	ny
	GLbyte	nz

void
glNormal3d(nx,ny,nz)
	GLdouble	nx
	GLdouble	ny
	GLdouble	nz

void
glNormal3f(nx,ny,nz)
	GLfloat	nx
	GLfloat	ny
	GLfloat	nz

void
glNormal3i(nx,ny,nz)
	GLint	nx
	GLint	ny
	GLint	nz

void
glNormal3s(nx,ny,nz)
	GLshort	nx
	GLshort	ny
	GLshort	nz

void
glNormal3bv(v)
	char *	v
	CODE:
	{
	   glNormal3bv((GLbyte *)v);
	}

void
glNormal3dv(v)
	char *	v
	CODE:
	{
	   glNormal3dv((GLdouble *)v);
	}

void
glNormal3fv(v)
	char *	v
	CODE:
	{
	   glNormal3fv((GLfloat *)v);
	}

void
glNormal3iv(v)
	char *	v
	CODE:
	{
	   glNormal3iv((GLint *)v);
	}

void
glNormal3sv(v)
	char *	v
	CODE:
	{
	   glNormal3sv((GLshort *)v);
	}

void
glIndexd(c)
	GLdouble	c

void
glIndexf(c)
	GLfloat	c

void
glIndexi(c)
	GLint	c

void
glIndexs(c)
	GLshort	c

void
glIndexdv(c)
	char *	c
	CODE:
	{
	   glIndexdv((GLdouble *)c);
	}

void
glIndexfv(c)
	char *	c
	CODE:
	{
	   glIndexfv((GLfloat *)c);
	}

void
glIndexiv(c)
	char *	c
	CODE:
	{
	   glIndexiv((GLint *)c);
	}

void
glIndexsv(c)
	char *	c
	CODE:
	{
	   glIndexsv((GLshort *)c);
	}

void
glColor3b(red,green,blue)
	GLbyte	red
	GLbyte	green
	GLbyte	blue

void
glColor3d(red,green,blue)
	GLdouble	red
	GLdouble	green
	GLdouble	blue

void
glColor3f(red,green,blue)
	GLfloat	red
	GLfloat	green
	GLfloat	blue

void
glColor3i(red,green,blue)
	GLint	red
	GLint	green
	GLint	blue

void
glColor3s(red,green,blue)
	GLshort	red
	GLshort	green
	GLshort	blue

void
glColor3ub(red,green,blue)
	GLubyte	red
	GLubyte	green
	GLubyte	blue

void
glColor3ui(red,green,blue)
	GLuint	red
	GLuint	green
	GLuint	blue

void
glColor3us(red,green,blue)
	GLushort	red
	GLushort	green
	GLushort	blue

void
glColor4b(red,green,blue,alpha)
	GLbyte	red
	GLbyte	green
	GLbyte	blue
	GLbyte	alpha

void
glColor4d(red,green,blue,alpha)
	GLdouble	red
	GLdouble	green
	GLdouble	blue
	GLdouble	alpha

void
glColor4f(red,green,blue,alpha)
	GLfloat	red
	GLfloat	green
	GLfloat	blue
	GLfloat	alpha

void
glColor4i(red,green,blue,alpha)
	GLint	red
	GLint	green
	GLint	blue
	GLint	alpha

void
glColor4s(red,green,blue,alpha)
	GLshort	red
	GLshort	green
	GLshort	blue
	GLshort	alpha

void
glColor4ub(red,green,blue,alpha)
	GLubyte	red
	GLubyte	green
	GLubyte	blue
	GLubyte	alpha

void
glColor4ui(red,green,blue,alpha)
	GLuint	red
	GLuint	green
	GLuint	blue
	GLuint	alpha

void
glColor4us(red,green,blue,alpha)
	GLushort	red
	GLushort	green
	GLushort	blue
	GLushort	alpha

void
glColor3bv(v)
	char *	v
	CODE:
	{
	   glColor3bv((GLbyte *)v);
	}

void
glColor3dv(v)
	char *	v
	CODE:
	{
	   glColor3dv((GLdouble *)v);
	}

void
glColor3fv(v)
	char *	v
	CODE:
	{
	   glColor3fv((GLfloat *)v);
	}

void
glColor3iv(v)
	char *	v
	CODE:
	{
	   glColor3iv((GLint *)v);
	}

void
glColor3sv(v)
	char *	v
	CODE:
	{
	   glColor3sv((GLshort *)v);
	}

void
glColor3ubv(v)
	char *	v
	CODE:
	{
	   glColor3ubv((GLubyte *)v);
	}

void
glColor3uiv(v)
	char *	v
	CODE:
	{
	   glColor3uiv((GLuint *)v);
	}

void
glColor3usv(v)
	char *	v
	CODE:
	{
	   glColor3usv((GLushort *)v);
	}

void
glColor4bv(v)
	char *	v
	CODE:
	{
	   glColor4bv((GLbyte *)v);
	}

void
glColor4dv(v)
	char *	v
	CODE:
	{
	   glColor4dv((GLdouble *)v);
	}

void
glColor4fv(v)
	char *	v
	CODE:
	{
	   glColor4fv((GLfloat *)v);
	}

void
glColor4iv(v)
	char *	v
	CODE:
	{
	   glColor4iv((GLint *)v);
	}

void
glColor4sv(v)
	char *	v
	CODE:
	{
	   glColor4sv((GLshort *)v);
	}

void
glColor4ubv(v)
	char *	v
	CODE:
	{
	   glColor4ubv((GLubyte *)v);
	}

void
glColor4uiv(v)
	char *	v
	CODE:
	{
	   glColor4uiv((GLuint *)v);
	}

void
glColor4usv(v)
	char *	v
	CODE:
	{
	   glColor4usv((GLushort *)v);
	}

void
glTexCoord1d(s)
	GLdouble	s

void
glTexCoord1f(s)
	GLfloat	s

void
glTexCoord1i(s)
	GLint	s

void
glTexCoord1s(s)
	GLshort	s

void
glTexCoord2d(s,t)
	GLdouble	s
	GLdouble	t

void
glTexCoord2f(s,t)
	GLfloat	s
	GLfloat	t

void
glTexCoord2i(s,t)
	GLint	s
	GLint	t

void
glTexCoord2s(s,t)
	GLshort	s
	GLshort	t

void
glTexCoord3d(s,t,r)
	GLdouble	s
	GLdouble	t
	GLdouble	r

void
glTexCoord3f(s,t,r)
	GLfloat	s
	GLfloat	t
	GLfloat	r

void
glTexCoord3i(s,t,r)
	GLint	s
	GLint	t
	GLint	r

void
glTexCoord3s(s,t,r)
	GLshort	s
	GLshort	t
	GLshort	r

void
glTexCoord4d(s,t,r,q)
	GLdouble	s
	GLdouble	t
	GLdouble	r
	GLdouble	q

void
glTexCoord4f(s,t,r,q)
	GLfloat	s
	GLfloat	t
	GLfloat	r
	GLfloat	q

void
glTexCoord4i(s,t,r,q)
	GLint	s
	GLint	t
	GLint	r
	GLint	q

void
glTexCoord4s(s,t,r,q)
	GLshort	s
	GLshort	t
	GLshort	r
	GLshort	q

void
glTexCoord1dv(v)
	char *	v
	CODE:
	{
	   glTexCoord1dv((GLdouble *)v);
	}

void
glTexCoord1fv(v)
	char *	v
	CODE:
	{
	   glTexCoord1fv((GLfloat *)v);
	}

void
glTexCoord1iv(v)
	char *	v
	CODE:
	{
	   glTexCoord1iv((GLint *)v);
	}

void
glTexCoord1sv(v)
	char *	v
	CODE:
	{
	   glTexCoord1sv((GLshort *)v);
	}

void
glTexCoord2dv(v)
	char *	v
	CODE:
	{
	   glTexCoord2dv((GLdouble *)v);
	}

void
glTexCoord2fv(v)
	char *	v
	CODE:
	{
	   glTexCoord2fv((GLfloat *)v);
	}

void
glTexCoord2iv(v)
	char *	v
	CODE:
	{
	   glTexCoord2iv((GLint *)v);
	}

void
glTexCoord2sv(v)
	char *	v
	CODE:
	{
	   glTexCoord2sv((GLshort *)v);
	}

void
glTexCoord3dv(v)
	char *	v
	CODE:
	{
	   glTexCoord3dv((GLdouble *)v);
	}

void
glTexCoord3fv(v)
	char *	v
	CODE:
	{
	   glTexCoord3fv((GLfloat *)v);
	}

void
glTexCoord3iv(v)
	char *	v
	CODE:
	{
	   glTexCoord3iv((GLint *)v);
	}

void
glTexCoord3sv(v)
	char *	v
	CODE:
	{
	   glTexCoord3sv((GLshort *)v);
	}

void
glTexCoord4dv(v)
	char *	v
	CODE:
	{
	   glTexCoord4dv((GLdouble *)v);
	}

void
glTexCoord4fv(v)
	char *	v
	CODE:
	{
	   glTexCoord4fv((GLfloat *)v);
	}

void
glTexCoord4iv(v)
	char *	v
	CODE:
	{
	   glTexCoord4iv((GLint *)v);
	}

void
glTexCoord4sv(v)
	char *	v
	CODE:
	{
	   glTexCoord4sv((GLshort *)v);
	}

void
glRasterPos2d(x,y)
	GLdouble	x
	GLdouble	y

void
glRasterPos2f(x,y)
	GLfloat	x
	GLfloat	y

void
glRasterPos2i(x,y)
	GLint	x
	GLint	y

void
glRasterPos2s(x,y)
	GLshort	x
	GLshort	y

void
glRasterPos3d(x,y,z)
	GLdouble	x
	GLdouble	y
	GLdouble	z

void
glRasterPos3f(x,y,z)
	GLfloat	x
	GLfloat	y
	GLfloat	z

void
glRasterPos3i(x,y,z)
	GLint	x
	GLint	y
	GLint	z

void
glRasterPos3s(x,y,z)
	GLshort	x
	GLshort	y
	GLshort	z

void
glRasterPos4d(x,y,z,w)
	GLdouble	x
	GLdouble	y
	GLdouble	z
	GLdouble	w

void
glRasterPos4f(x,y,z,w)
	GLfloat	x
	GLfloat	y
	GLfloat	z
	GLfloat	w

void
glRasterPos4i(x,y,z,w)
	GLint	x
	GLint	y
	GLint	z
	GLint	w

void
glRasterPos4s(x,y,z,w)
	GLshort	x
	GLshort	y
	GLshort	z
	GLshort	w

void
glRasterPos2dv(v)
	char *	v
	CODE:
	{
	   glRasterPos2dv((GLdouble *)v);
	}

void
glRasterPos2fv(v)
	char *	v
	CODE:
	{
	   glRasterPos2fv((GLfloat *)v);
	}

void
glRasterPos2iv(v)
	char *	v
	CODE:
	{
	   glRasterPos2iv((GLint *)v);
	}

void
glRasterPos2sv(v)
	char *	v
	CODE:
	{
	   glRasterPos2sv((GLshort *)v);
	}

void
glRasterPos3dv(v)
	char *	v
	CODE:
	{
	   glRasterPos3dv((GLdouble *)v);
	}

void
glRasterPos3fv(v)
	char *	v
	CODE:
	{
	   glRasterPos3fv((GLfloat *)v);
	}

void
glRasterPos3iv(v)
	char *	v
	CODE:
	{
	   glRasterPos3iv((GLint *)v);
	}

void
glRasterPos3sv(v)
	char *	v
	CODE:
	{
	   glRasterPos3sv((GLshort *)v);
	}

void
glRasterPos4dv(v)
	char *	v
	CODE:
	{
	   glRasterPos4dv((GLdouble *)v);
	}

void
glRasterPos4fv(v)
	char *	v
	CODE:
	{
	   glRasterPos4fv((GLfloat *)v);
	}

void
glRasterPos4iv(v)
	char *	v
	CODE:
	{
	   glRasterPos4iv((GLint *)v);
	}

void
glRasterPos4sv(v)
	char *	v
	CODE:
	{
	   glRasterPos4sv((GLshort *)v);
	}

void
glRectd(x1,y1,x2,y2)
	GLdouble	x1
	GLdouble	y1
	GLdouble	x2
	GLdouble	y2

void
glRectf(x1,y1,x2,y2)
	GLfloat	x1
	GLfloat	y1
	GLfloat	x2
	GLfloat	y2

void
glRecti(x1,y1,x2,y2)
	GLint	x1
	GLint	y1
	GLint	x2
	GLint	y2

void
glRects(x1,y1,x2,y2)
	GLshort	x1
	GLshort	y1
	GLshort	x2
	GLshort	y2

void
glRectdv(v1,v2)
	char *	v1
	char *	v2
	CODE:
	{
	   glRectdv((GLdouble *)v1,(GLdouble *)v2);
	}

void
glRectfv(v1,v2)
	char *	v1
	char *	v2
	CODE:
	{
	   glRectfv((GLfloat *)v1,(GLfloat *)v2);
	}

void
glRectiv(v1,v2)
	char *	v1
	char *	v2
	CODE:
	{
	   glRectiv((GLint *)v1,(GLint *)v2);
	}

void
glRectsv(v1,v2)
	char *	v1
	char *	v2
	CODE:
	{
	   glRectsv((GLshort *)v1,(GLshort *)v2);
	}

void
glShadeModel(mode)
	GLenum	mode

void
glLightf(light,pname,param)
	GLenum	light
	GLenum	pname
	GLfloat	param

void
glLighti(light,pname,param)
	GLenum	light
	GLenum	pname
	GLint	param

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
glLightiv(light,pname,params)
	GLenum	light
	GLenum	pname
	char *	params
	CODE:
	{
	   glLightiv(light,pname,(GLint *)params);
	}

void
glGetLightfv(light,pname,params)
	GLenum	light
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetLightfv(light,pname,(GLfloat *)params);
	}

void
glGetLightiv(light,pname,params)
	GLenum	light
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetLightiv(light,pname,(GLint *)params);
	}

void
glLightModelf(pname,param)
	GLenum	pname
	GLfloat	param

void
glLightModeli(pname,param)
	GLenum	pname
	GLint	param

void
glLightModelfv(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glLightModelfv(pname,(GLfloat *)params);
	}

void
glLightModeliv(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glLightModeliv(pname,(GLint *)params);
	}

void
glMaterialf(face,pname,param)
	GLenum	face
	GLenum	pname
	GLfloat	param

void
glMateriali(face,pname,param)
	GLenum	face
	GLenum	pname
	GLint	param

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
glMaterialiv(face,pname,params)
	GLenum	face
	GLenum	pname
	char *	params
	CODE:
	{
	   glMaterialiv(face,pname,(GLint *)params);
	}

void
glGetMaterialfv(face,pname,params)
	GLenum	face
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetMaterialfv(face,pname,(GLfloat *)params);
	}

void
glGetMaterialiv(face,pname,params)
	GLenum	face
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetMaterialiv(face,pname,(GLint *)params);
	}

void
glColorMaterial(face,mode)
	GLenum	face
	GLenum	mode

void
glPixelZoom(xfactor,yfactor)
	GLfloat	xfactor
	GLfloat	yfactor

void
glPixelStoref(pname,param)
	GLenum	pname
	GLfloat	param

void
glPixelStorei(pname,param)
	GLenum	pname
	GLint	param

void
glPixelTransferf(pname,param)
	GLenum	pname
	GLfloat	param

void
glPixelTransferi(pname,param)
	GLenum	pname
	GLint	param

void
glPixelMapfv(map,mapsize,values)
	GLenum	map
	GLint	mapsize
	char *	values
	CODE:
	{
	   glPixelMapfv(map,mapsize,(GLfloat *)values);
	}

void
glPixelMapuiv(map,mapsize,values)
	GLenum	map
	GLint	mapsize
	char *	values
	CODE:
	{
	   glPixelMapuiv(map,mapsize,(GLuint *)values);
	}

void
glPixelMapusv(map,mapsize,values)
	GLenum	map
	GLint	mapsize
	char *	values
	CODE:
	{
	   glPixelMapusv(map,mapsize,(GLushort *)values);
	}

void
glGetPixelMapfv(map,values)
	GLenum	map
	char *	values
	CODE:
	{
	   glGetPixelMapfv(map,(GLfloat *)values);
	}

void
glGetPixelMapuiv(map,values)
	GLenum	map
	char *	values
	CODE:
	{
	   glGetPixelMapuiv(map,(GLuint *)values);
	}

void
glGetPixelMapusv(map,values)
	GLenum	map
	char *	values
	CODE:
	{
	   glGetPixelMapusv(map,(GLushort *)values);
	}

void
glBitmap(width,height,xorig,yorig,xmove,ymove,bitmap)
	GLsizei	width
	GLsizei	height
	GLfloat	xorig
	GLfloat	yorig
	GLfloat	xmove
	GLfloat	ymove
	char *	bitmap
	CODE:
	{
	   glBitmap(width,height,xorig,yorig,xmove,ymove,(GLubyte *)bitmap);
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

void
glDrawPixels(width,height,format,type,pixels)
	GLsizei	width
	GLsizei	height
	GLenum	format
	GLenum	type
	char *	pixels
	CODE:
	{
	   glDrawPixels(width,height,format,type,(GLvoid *)pixels);
	}

void
glCopyPixels(x,y,width,height,type)
	GLint	x
	GLint	y
	GLsizei	width
	GLsizei	height
	GLenum	type

void
glStencilFunc(func,ref,mask)
	GLenum	func
	GLint	ref
	GLuint	mask

void
glStencilMask(mask)
	GLuint	mask

void
glStencilOp(fail,zfail,zpass)
	GLenum	fail
	GLenum	zfail
	GLenum	zpass

void
glClearStencil(s)
	GLint	s

void
glTexGend(coord,pname,param)
	GLenum	coord
	GLenum	pname
	GLdouble	param

void
glTexGenf(coord,pname,param)
	GLenum	coord
	GLenum	pname
	GLfloat	param

void
glTexGeni(coord,pname,param)
	GLenum	coord
	GLenum	pname
	GLint	param

void
glTexGendv(coord,pname,params)
	GLenum	coord
	GLenum	pname
	char *	params
	CODE:
	{
	   glTexGendv(coord,pname,(GLdouble *)params);
	}

void
glTexGenfv(coord,pname,params)
	GLenum	coord
	GLenum	pname
	char *	params
	CODE:
	{
	   glTexGenfv(coord,pname,(GLfloat *)params);
	}

void
glTexGeniv(coord,pname,params)
	GLenum	coord
	GLenum	pname
	char *	params
	CODE:
	{
	   glTexGeniv(coord,pname,(GLint *)params);
	}

void
glGetTexGendv(coord,pname,params)
	GLenum	coord
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexGendv(coord,pname,(GLdouble *)params);
	}

void
glGetTexGenfv(coord,pname,params)
	GLenum	coord
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexGenfv(coord,pname,(GLfloat *)params);
	}

void
glGetTexGeniv(coord,pname,params)
	GLenum	coord
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexGeniv(coord,pname,(GLint *)params);
	}

void
glTexEnvf(target,pname,param)
	GLenum	target
	GLenum	pname
	GLfloat	param

void
glTexEnvi(target,pname,param)
	GLenum	target
	GLenum	pname
	GLint	param

void
glTexEnvfv(target,pname,params)
	GLenum	target
	GLenum	pname
	char *	params
	CODE:
	{
	   glTexEnvfv(target,pname,(GLfloat *)params);
	}

void
glTexEnviv(target,pname,params)
	GLenum	target
	GLenum	pname
	char *	params
	CODE:
	{
	   glTexEnviv(target,pname,(GLint *)params);
	}

void
glGetTexEnvfv(target,pname,params)
	GLenum	target
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexEnvfv(target,pname,(GLfloat *)params);
	}

void
glGetTexEnviv(target,pname,params)
	GLenum	target
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexEnviv(target,pname,(GLint *)params);
	}

void
glTexParameterf(target,pname,param)
	GLenum	target
	GLenum	pname
	GLfloat	param

void
glTexParameteri(target,pname,param)
	GLenum	target
	GLenum	pname
	GLint	param

void
glTexParameterfv(target,pname,params)
	GLenum	target
	GLenum	pname
	char *	params
	CODE:
	{
	   glTexParameterfv(target,pname,(GLfloat *)params);
	}

void
glTexParameteriv(target,pname,params)
	GLenum	target
	GLenum	pname
	char *	params
	CODE:
	{
	   glTexParameteriv(target,pname,(GLint *)params);
	}

void
glGetTexParameterfv(target,pname,params)
	GLenum	target
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexParameterfv(target,pname,(GLfloat *)params);
	}

void
glGetTexParameteriv(target,pname,params)
	GLenum	target
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexParameteriv(target,pname,(GLint *)params);
	}

void
glGetTexLevelParameterfv(target,level,pname,params)
	GLenum	target
	GLint	level
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexLevelParameterfv(target,level,pname,(GLfloat *)params);
	}

void
glGetTexLevelParameteriv(target,level,pname,params)
	GLenum	target
	GLint	level
	GLenum	pname
	char *	params
	CODE:
	{
	   glGetTexLevelParameteriv(target,level,pname,(GLint *)params);
	}

void
glTexImage1D(target,level,internalFormat,width,border,format,type,pixels)
	GLenum	target
	GLint	level
	GLint	internalFormat
	GLsizei	width
	GLint	border
	GLenum	format
	GLenum	type
	char *	pixels
	CODE:
	{
	   glTexImage1D(target,level,internalFormat,width,border,format,type,(GLvoid *)pixels);
	}

void
glTexImage2D(target,level,internalFormat,width,height,border,format,type,pixels)
	GLenum	target
	GLint	level
	GLint	internalFormat
	GLsizei	width
	GLsizei	height
	GLint	border
	GLenum	format
	GLenum	type
	char *	pixels
	CODE:
	{
	   glTexImage2D(target,level,internalFormat,width,height,border,format,type,(GLvoid *)pixels);
	}

void
glGetTexImage(target,level,format,type,pixels)
	GLenum	target
	GLint	level
	GLenum	format
	GLenum	type
	char *	pixels
	CODE:
	{
	   glGetTexImage(target,level,format,type,(GLvoid *)pixels);
	}

void
glMap1d(target,u1,u2,stride,order,points)
	GLenum	target
	GLdouble	u1
	GLdouble	u2
	GLint	stride
	GLint	order
	char *	points
	CODE:
	{
	   glMap1d(target,u1,u2,stride,order,(GLdouble *)points);
	}

void
glMap1f(target,u1,u2,stride,order,points)
	GLenum	target
	GLfloat	u1
	GLfloat	u2
	GLint	stride
	GLint	order
	char *	points
	CODE:
	{
	   glMap1f(target,u1,u2,stride,order,(GLfloat *)points);
	}

void
glMap2d(target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points)
	GLenum	target
	GLdouble	u1
	GLdouble	u2
	GLint	ustride
	GLint	uorder
	GLdouble	v1
	GLdouble	v2
	GLint	vstride
	GLint	vorder
	char *	points
	CODE:
	{
	   glMap2d(target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,(GLdouble *)points);
	}

void
glMap2f(target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points)
	GLenum	target
	GLfloat	u1
	GLfloat	u2
	GLint	ustride
	GLint	uorder
	GLfloat	v1
	GLfloat	v2
	GLint	vstride
	GLint	vorder
	char *	points
	CODE:
	{
	   glMap2f(target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,(GLfloat *)points);
	}

void
glGetMapdv(target,query,v)
	GLenum	target
	GLenum	query
	char *	v
	CODE:
	{
	   glGetMapdv(target,query,(GLdouble *)v);
	}

void
glGetMapfv(target,query,v)
	GLenum	target
	GLenum	query
	char *	v
	CODE:
	{
	   glGetMapfv(target,query,(GLfloat *)v);
	}

void
glGetMapiv(target,query,v)
	GLenum	target
	GLenum	query
	char *	v
	CODE:
	{
	   glGetMapiv(target,query,(GLint *)v);
	}

void
glEvalCoord1d(u)
	GLdouble	u

void
glEvalCoord1f(u)
	GLfloat	u

void
glEvalCoord1dv(u)
	char *	u
	CODE:
	{
	   glEvalCoord1dv((GLdouble *)u);
	}

void
glEvalCoord1fv(u)
	char *	u
	CODE:
	{
	   glEvalCoord1fv((GLfloat *)u);
	}

void
glEvalCoord2d(u,v)
	GLdouble	u
	GLdouble	v

void
glEvalCoord2f(u,v)
	GLfloat	u
	GLfloat	v

void
glEvalCoord2dv(u)
	char *	u
	CODE:
	{
	   glEvalCoord2dv((GLdouble *)u);
	}

void
glEvalCoord2fv(u)
	char *	u
	CODE:
	{
	   glEvalCoord2fv((GLfloat *)u);
	}

void
glMapGrid1d(un,u1,u2)
	GLint	un
	GLdouble	u1
	GLdouble	u2

void
glMapGrid1f(un,u1,u2)
	GLint	un
	GLfloat	u1
	GLfloat	u2

void
glMapGrid2d(un,u1,u2,vn,v1,v2)
	GLint	un
	GLdouble	u1
	GLdouble	u2
	GLint	vn
	GLdouble	v1
	GLdouble	v2

void
glMapGrid2f(un,u1,u2,vn,v1,v2)
	GLint	un
	GLfloat	u1
	GLfloat	u2
	GLint	vn
	GLfloat	v1
	GLfloat	v2

void
glEvalPoint1(i)
	GLint	i

void
glEvalPoint2(i,j)
	GLint	i
	GLint	j

void
glEvalMesh1(mode,i1,i2)
	GLenum	mode
	GLint	i1
	GLint	i2

void
glEvalMesh2(mode,i1,i2,j1,j2)
	GLenum	mode
	GLint	i1
	GLint	i2
	GLint	j1
	GLint	j2

void
glFogf(pname,param)
	GLenum	pname
	GLfloat	param

void
glFogi(pname,param)
	GLenum	pname
	GLint	param

void
glFogfv(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glFogfv(pname,(GLfloat *)params);
	}

void
glFogiv(pname,params)
	GLenum	pname
	char *	params
	CODE:
	{
	   glFogiv(pname,(GLint *)params);
	}

void
glFeedbackBuffer(size,type,buffer)
	GLsizei	size
	GLenum	type
	char *	buffer
	CODE:
	{
	   glFeedbackBuffer(size,type,(GLfloat *)buffer);
	}

void
glPassThrough(token)
	GLfloat	token

void
glSelectBuffer(size,buffer)
	GLsizei	size
	char *	buffer
	CODE:
	{
	   glSelectBuffer(size,(GLuint *)buffer);
	}

void
glInitNames()

void
glLoadName(name)
	GLuint	name

void
glPushName(name)
	GLuint	name

void
glPopName()

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
	  if (XF86V4) {
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
