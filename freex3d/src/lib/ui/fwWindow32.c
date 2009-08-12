/*
  =INSERT_TEMPLATE_HERE=

  $Id$

  FreeWRL main window : win32 code.

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <float.h>

#include <main/headers.h>

#include "ui.h"
/* #include "../plugin/pluginUtils.h" */

#include <winuser.h>


void do_keyPress(const char kp, int type);


/* from Blender GHOST_SystemWin32.cpp: Key code values not found in winuser.h */
#ifndef VK_MINUS
#define VK_MINUS 0xBD
#endif // VK_MINUS
#ifndef VK_SEMICOLON
#define VK_SEMICOLON 0xBA
#endif // VK_SEMICOLON
#ifndef VK_PERIOD
#define VK_PERIOD 0xBE
#endif // VK_PERIOD
#ifndef VK_COMMA
#define VK_COMMA 0xBC
#endif // VK_COMMA
#ifndef VK_QUOTE
#define VK_QUOTE 0xDE
#endif // VK_QUOTE
#ifndef VK_BACK_QUOTE
#define VK_BACK_QUOTE 0xC0
#endif // VK_BACK_QUOTE
#ifndef VK_SLASH
#define VK_SLASH 0xBF
#endif // VK_SLASH
#ifndef VK_BACK_SLASH
#define VK_BACK_SLASH 0xDC
#endif // VK_BACK_SLASH
#ifndef VK_EQUALS
#define VK_EQUALS 0xBB
#endif // VK_EQUALS
#ifndef VK_OPEN_BRACKET
#define VK_OPEN_BRACKET 0xDB
#endif // VK_OPEN_BRACKET
#ifndef VK_CLOSE_BRACKET
#define VK_CLOSE_BRACKET 0xDD
#endif // VK_CLOSE_BRACKET
#ifndef VK_GR_LESS
#define VK_GR_LESS 0xE2
#endif // VK_GR_LESS



static int oldx = 0, oldy = 0;

/* #undef XTDEBUG */

/* static int screen; */
static int quadbuff_stereo_mode;

char *GL_VER = NULL;
char *GL_VEN = NULL;
char *GL_REN = NULL;

int button[5];
int mouseX, mouseY;

static short gcWheelDelta = 0;

extern int	shutterGlasses; /* stereo shutter glasses */

float myFps = 0.0;

void setMenuButton_collision(int val){}
void setMenuButton_texSize(int size){}
void setMenuButton_headlight(int val){}
void setMenuButton_navModes(int type){}

void setMessageBar()
{
}
GLfloat latitude, longitude, latinc, longinc; 
GLdouble radius; 
 

#define GLOBE    1 
#define CYLINDER 2 
#define CONE     3 

void pause()
{
    printf(":");
    getchar();
}

GLvoid resize( GLsizei width, GLsizei height ) 
{ 
    GLfloat aspect; 
 
    glViewport( 0, 0, width, height ); 
 
    aspect = (GLfloat) width / height; 
 
    glMatrixMode( GL_PROJECTION ); 
    glLoadIdentity(); 
    gluPerspective( 45.0, aspect, .3, 700.0); /* 3.0, 7.0 ); */
    glMatrixMode( GL_MODELVIEW ); 
}     

GLvoid createObjects() 
{ 
    GLUquadricObj *quadObj; 
 
    glNewList(GLOBE, GL_COMPILE); 
    quadObj = gluNewQuadric (); 
    gluQuadricDrawStyle (quadObj, GLU_LINE); 
    gluSphere (quadObj, 1.5, 16, 16); 
    glEndList(); 
 
    /*
      glNewList(CONE, GL_COMPILE); 
      quadObj = gluNewQuadric (); 
      gluQuadricDrawStyle (quadObj, GLU_FILL); 
      gluQuadricNormals (quadObj, GLU_SMOOTH); 
      gluCylinder(quadObj, 0.3, 0.0, 0.6, 15, 10); 
      glEndList(); 
    */
    glNewList(CYLINDER, GL_COMPILE); 
    glPushMatrix (); 
    glRotatef ((GLfloat)90.0, (GLfloat)1.0, (GLfloat)0.0, (GLfloat)0.0); 
    glTranslatef ((GLfloat)0.0, (GLfloat)0.0, (GLfloat)-1.0); 
    quadObj = gluNewQuadric (); 
    gluQuadricDrawStyle (quadObj, GLU_FILL); 
    gluQuadricNormals (quadObj, GLU_SMOOTH); 
    gluCylinder (quadObj, 0.3, 0.3, 0.6, 12, 2); 
    glPopMatrix (); 
    glEndList(); 
}

GLvoid initializeGL(GLsizei width, GLsizei height) 
{ 
    GLfloat     maxObjectSize, aspect; 
    GLdouble    near_plane, far_plane; 
 
    glClearIndex( (GLfloat)BLACK_INDEX); 
    glClearDepth( 1.0 ); 
 
    glEnable(GL_DEPTH_TEST); 
 
    glMatrixMode( GL_PROJECTION ); 
    glLoadIdentity(); /*inserted by doug*/

    aspect = (GLfloat) width / height; 
    gluPerspective( 45.0, aspect, .3, 700.0 ); 
    glMatrixMode( GL_MODELVIEW ); 
 
    near_plane = 3.0; 
    far_plane = 7.0; 
    maxObjectSize = 3.0F; 
    radius = near_plane + maxObjectSize/2.0; 
 
    latitude = 0.0F; 
    longitude = 0.0F; 
    latinc = 6.0F; 
    longinc = 2.5F; 
 
    createObjects(); 
} 
 
void polarView(GLdouble radius, GLdouble twist, GLdouble latitude, 
	       GLdouble longitude) 
{ 
    glTranslated(0.0, 0.0, -radius); 
    glRotated(-twist, 0.0, 0.0, 1.0); 
    glRotated(-latitude, 1.0, 0.0, 0.0); 
    glRotated(longitude, 0.0, 0.0, 1.0);      
 
} 
 
GLvoid drawScene(GLvoid) 
{ 
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); 
 
    glPushMatrix(); 
 
    latitude += latinc; 
    longitude += longinc; 
 
    polarView( radius, 0, latitude, longitude ); 
 
    glIndexi(RED_INDEX); 
    glCallList(CONE); 
 
    glIndexi(BLUE_INDEX); 
    glCallList(GLOBE); 
 
    glIndexi(GREEN_INDEX); 
    glPushMatrix(); 
    glTranslatef(0.8F, -0.65F, 0.0F); 
    glRotatef(30.0F, 1.0F, 0.5F, 1.0F); 
    glCallList(CYLINDER); 
    glPopMatrix(); 
 
    glPopMatrix(); 

    SwapBuffers(wglGetCurrentDC());
} 

void setMenuStatus(char *stat) {
    /*strncpy (myMenuStatus, stat, MAXSTAT);*/
    setMessageBar();
}

void setMenuFps (float fps) {
    myFps = fps;
    setMessageBar();
}

static bool m_fullscreen = false;
static bool dualmonitor = false;
static RECT smallrect;
bool EnableFullscreen(int w, int h, int bpp)
{
#if 0
    /* adapted from http://www.gamedev.net/community/forums/topic.asp?topic_id=418397 */
    /* normally I pass in bpp=32. If you set debugit=true below and do a run, you'll see the modes available */
    /* CDS_FULLSCREEN
       for dual-monitor the author warns to disable nVidia's nView (they hook 
       into changedisplaysettings but not changedisplaysettingsex)
       one idea: don't do the ex on single monitors - set dualmonitor=false above.
       Find out the name of the device this window - is on (this is for multi-monitor setups) 
    */
    MONITORINFOEX monInfo;
    LONG ret;
    bool ok;
    DWORD style, exstyle;
    bool debugit;
    DEVMODE dmode;
    bool foundMode;
    int i;

    HMONITOR hMonitor = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTOPRIMARY);
    memset(&monInfo, 0, sizeof(MONITORINFOEX));
    monInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, (LPMONITORINFO)&monInfo);

    /* Find the requested device mode */
    foundMode = false;
    memset(&dmode, 0, sizeof(DEVMODE));
    dmode.dmSize = sizeof(DEVMODE);
    debugit = false;
    for(i=0 ; EnumDisplaySettings(monInfo.szDevice, i, &dmode) && !foundMode ; ++i)
    {
        foundMode = (dmode.dmPelsWidth==(DWORD)w) && 
	    (dmode.dmPelsHeight==(DWORD)h) &&
	    (dmode.dmBitsPerPel==(DWORD)bpp);
	if(debugit)
	    ConsoleMessage("found w=%d h=%d bpp=%d\n",(int)dmode.dmPelsWidth, (int)dmode.dmPelsHeight, (int)dmode.dmBitsPerPel);
    }
    if(!foundMode || debugit )
    {
	ConsoleMessage("error: suitable display mode for w=%d h=%d bpp=%d not found\n",w,h,bpp);
        GetWindowRect(ghWnd, &smallrect);
	ConsoleMessage("window rect l=%d t=%d r=%d b=%d\n",smallrect.top,smallrect.left,smallrect.right,smallrect.bottom);
	return false;
    }
    dmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    /* If we're switching from a windowed mode to this fullscreen
       mode, save some information about the window so that it can
       be restored when switching back to windowed mode
    */
    style=0, exstyle=0;
    if(!m_fullscreen)
    {
        /* Save the current window position/size */
        GetWindowRect(ghWnd, &smallrect);

        /* Save the window style and set it for fullscreen mode */
        style = GetWindowLongPtr(ghWnd, GWL_STYLE);
        exstyle = GetWindowLongPtr(ghWnd, GWL_EXSTYLE);
        SetWindowLongPtr(ghWnd, GWL_STYLE, style & (~WS_OVERLAPPEDWINDOW));
        SetWindowLongPtr(ghWnd, GWL_EXSTYLE, exstyle | WS_EX_APPWINDOW | WS_EX_TOPMOST);
    }

    // Attempt to change the resolution
    if(dualmonitor)
    {
	ret = ChangeDisplaySettingsEx(monInfo.szDevice, &dmode, NULL, CDS_FULLSCREEN, NULL);
    }else{
	ret = ChangeDisplaySettings(&dmode, CDS_FULLSCREEN);
    }
    //LONG ret = ChangeDisplaySettings(&dmode, CDS_FULLSCREEN);
    ok = (ret == DISP_CHANGE_SUCCESSFUL);
    if(ok) m_fullscreen = true;

    /* If all was good resize & reposition the window
       to match the new resolution on the correct monitor
    */
    if(ok)
    {
        /* We need to call GetMonitorInfo() again becase
        // details may have changed with the resolution
	*/
        GetMonitorInfo(hMonitor, (LPMONITORINFO)&monInfo);

        /* Set the window's size and position so
        // that it covers the entire screen
	*/
        SetWindowPos(ghWnd, NULL, monInfo.rcMonitor.left, monInfo.rcMonitor.top, (int)w, (int)h,
                     SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_NOZORDER);
    }

    /* If the attempt failed and we weren't already
    // in fullscreen mode, restore the window styles
    */
    else if(!m_fullscreen)
    {
        SetWindowLongPtr(ghWnd, GWL_STYLE, style);
        SetWindowLongPtr(ghWnd, GWL_EXSTYLE, exstyle);
    }
    return ok;
#endif
    return FALSE;
}

void DisableFullscreen()
{
#if 0
    if(m_fullscreen) {

        /* Find out the name of the device this window
           is on (this is for multi-monitor setups) */
        MONITORINFOEX monInfo;
	DWORD style,exstyle;
        HMONITOR hMonitor = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTOPRIMARY);
        memset(&monInfo, 0, sizeof(MONITORINFOEX));
        monInfo.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(hMonitor, (LPMONITORINFO)&monInfo);

        /* Restore the display resolution */
        ChangeDisplaySettingsEx(monInfo.szDevice, NULL, NULL, 0, NULL);
        /*ChangeDisplaySettings(NULL, 0);*/

        m_fullscreen = false;

        /* Restore the window styles */
        style = GetWindowLongPtr(ghWnd, GWL_STYLE);
        exstyle = GetWindowLongPtr(ghWnd, GWL_EXSTYLE);
        SetWindowLongPtr(ghWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowLongPtr(ghWnd, GWL_EXSTYLE, exstyle & (~(WS_EX_APPWINDOW | WS_EX_TOPMOST)));

        /* Restore the window size/position  */
        /*SetPosition(m_windowedX, m_windowedY);*/
        /* SetSize(m_windowedWidth, m_windowedHeight); */
	SetWindowPos(ghWnd ,       /* handle to window */
		     HWND_TOPMOST,  /* placement-order handle */
		     smallrect.left,     /* horizontal position */
		     smallrect.top,      /* vertical position */
		     smallrect.right - smallrect.left,  /* width */
		     smallrect.bottom - smallrect.top, /* height */
		     SWP_SHOWWINDOW /* window-positioning options); */
	    );
    }
#endif
}

BOOL bSetupPixelFormat(HDC hdc) 
{ 
    PIXELFORMATDESCRIPTOR pfd, *ppfd; 
    int pixelformat; 
 
    ppfd = &pfd; 
 
    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR); 
    ppfd->nVersion = 1; 
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |  
	PFD_DOUBLEBUFFER; 
    ppfd->dwLayerMask = PFD_MAIN_PLANE; 
    ppfd->iPixelType = PFD_TYPE_COLORINDEX; 
    ppfd->cColorBits = 8; 
    ppfd->cDepthBits = 16; 
    ppfd->cAccumBits = 0; 
    ppfd->cStencilBits = 0; 
 
    pixelformat = ChoosePixelFormat(hdc, ppfd); 
 
    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 ) 
    { 
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK); 
        return FALSE; 
    } 
 
    if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE) 
    { 
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK); 
        return FALSE; 
    } 
 
    return TRUE; 
} 

void createContext32()
{
    RECT rect; 
    GLenum err;
    BOOL bb;

    printf("starting createcontext32\n");

    ghDC = GetDC(ghWnd); 
    printf("got hdc\n");
    if (!bSetupPixelFormat(ghDC))
	printf("ouch - bSetupPixelFormat failed\n");
    ghRC = wglCreateContext(ghDC); 
    printf("created context\n");

    bb = wglMakeCurrent(ghDC, ghRC); 
    printf("made current %u\n", bb);

    GetClientRect(ghWnd, &rect); 

    err = glewInit();
    if (GLEW_OK != err)
    {
	/* Problem: glewInit failed, something is seriously wrong. */
	printf("Error: %s\n", glewGetErrorString(err));
	 
    }
    printf( "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    printf("before initializeGL\n");
    initializeGL(rect.right, rect.bottom); 
    printf("after initializeGl\n");

}

void createGLContext(void)
{
    /* createContext32(); */
    /* save this info for later use */
    GL_REN = (char *)glGetString(GL_RENDERER);
    GL_VER = (char *)glGetString(GL_VERSION);
    GL_VEN = (char *)glGetString(GL_VENDOR);

    /* Set up the OpenGL state. This'll get overwritten later... */
    glClearDepth (1.0);
    glClearColor (0.0, 0.0, 1.0, 0.0);
    FW_GL_MATRIX_MODE (GL_PROJECTION);
    glFrustum (-1.0, 1.0, -1.0, 1.0, 1.0, 20);
    FW_GL_MATRIX_MODE (GL_MODELVIEW);
}

LRESULT CALLBACK PopupWndProc( 
    HWND hWnd, 
    UINT msg, 
    WPARAM wParam,
    LPARAM lParam )
{
    PAINTSTRUCT ps;
    LONG lRet = 1; 
    RECT rect; 
    char kp;
    int mev;
    int butnum;
    /*
      short cmd;
      WORD uDevice;
      int dwKeys;
      printf("msg=%ld ",(long)msg);
      if( wParam ) printf(" %ld ",(long)wParam);
      cmd  = GET_APPCOMMAND_LPARAM(lParam);
      uDevice = GET_DEVICE_LPARAM(lParam);
      dwKeys = GET_KEYSTATE_LPARAM(lParam);
      printf(" %ld %ld %ld",(long)cmd, (long)uDevice, (long) dwKeys);
      printf("\n");
    */
    mev = 0;
    butnum = 0;
    ghWnd = hWnd;
    switch( msg ) {

    case WM_CREATE: 
	printf("wm_create\n");
		   
	ghDC = GetDC(hWnd); 
	if (!bSetupPixelFormat(ghDC)) 
	    PostQuitMessage (0); 
	printf("WM_Create happening now\n");
	ghRC = wglCreateContext(ghDC); 
	wglMakeCurrent(ghDC, ghRC); 
	GetClientRect(hWnd, &rect); 
	screenWidth = rect.right; /*used in mainloop render_pre setup_projection*/
	screenHeight = rect.bottom;
	/* should it be r-l,b-t for w,h? No - getClientRect() returns 0,0,width,height in rect*/
	initializeGL(rect.right, rect.bottom); 
			
	break; 
 
    case WM_SIZE: 
	GetClientRect(hWnd, &rect); 
	screenWidth = rect.right; /*used in mainloop render_pre setup_projection*/
	screenHeight = rect.bottom;
	resize(rect.right, rect.bottom); 
	break; 

    case WM_DISPLAYCHANGE:
	/*triggred when the display mode is changed ie changedisplaysettings window <> fullscreen */
	ghDC = GetDC(hWnd); 
	if (!bSetupPixelFormat(ghDC)) 
	    PostQuitMessage (0); 
	printf("WM_DISPLAYCHANGE happening now\n");
	ghRC = wglCreateContext(ghDC); 
	wglMakeCurrent(ghDC, ghRC); 
	GetClientRect(hWnd, &rect); 
	screenWidth = rect.right; /*used in mainloop render_pre setup_projection*/
	screenHeight = rect.bottom;
	/* should it be r-l,b-t for w,h? No - getClientRect() returns 0,0,width,height in rect*/
	initializeGL(rect.right, rect.bottom); 
	break; 

    case WM_CLOSE: 
	if (ghRC) 
	    wglDeleteContext(ghRC); 
	ghDC = GetDC(hWnd); 
	if (ghDC) 
	    ReleaseDC(hWnd, ghDC); 
	ghRC = 0; 
	ghDC = 0; 
	 
	DestroyWindow (hWnd);
	doQuit();
	break; 



/**************************************************************\
 *     WM_PAINT:                                                *
\**************************************************************/

    case WM_PAINT:
		  
	ghDC = BeginPaint( hWnd, &ps );
	/*TextOut( ghDC, 10, 10, "Hello, Windows!", 13 ); */
	EndPaint( hWnd, &ps );
	break;

/**************************************************************\
 *     WM_COMMAND:                                              *
\**************************************************************/

    case WM_COMMAND:
	/*
	  switch( wParam ) {
	  case IDM_ABOUT:
	  DialogBox( ghInstance, "AboutDlg", hWnd, (DLGPROC)
	  AboutDlgProc );
	  break;
	  }
	*/
	break;

/**************************************************************\
 *     WM_DESTROY: PostQuitMessage() is called                  *
\**************************************************************/

    case WM_DESTROY: 
	if (ghRC) 
	    wglDeleteContext(ghRC); 
	ghDC = GetDC(hWnd); 
	if (ghDC) 
	    ReleaseDC(hWnd, ghDC); 
	 
	PostQuitMessage (0); 
	break; 

    case WM_KEYDOWN: 
	kp = (char)tolower(wParam);
	switch (wParam) { 
	case VK_LEFT: 
	    kp = 'j';
	    break; 
	case VK_RIGHT: 
	    kp = 'l';
	    break; 
	case VK_UP: 
	    kp = 'p';
	    break; 
	case VK_DOWN: 
	    kp = ';';
	    break; 
	case -70:
	    kp = ';';
	case VK_OPEN_BRACKET:
	    printf("[");
	    /* width, height, bpp of monitor */
	    EnableFullscreen(1680,1050,32);
	    break;
	case VK_CLOSE_BRACKET:
	    printf("]");
	    DisableFullscreen();
	    break;
	} 
	do_keyPress(kp, KeyPress); 
	break; /* FIXME: michel */

	/* Mouse events, processed */
    case WM_LBUTTONDOWN:
	button[0] = TRUE;
	butnum = 1;
	mev = ButtonPress;
	break;
    case WM_MBUTTONDOWN:
	button[1] = TRUE;
	butnum = 2;
	mev = ButtonPress;
	break;
    case WM_RBUTTONDOWN:
	button[2] = TRUE;
	butnum = 3;
	mev = ButtonPress;
	break;
    case WM_LBUTTONUP:
	button[0] = FALSE;
	butnum = 1;
	mev = ButtonRelease;
	break;
    case WM_MBUTTONUP:
	button[1] = FALSE;
	butnum = 2;
	mev = ButtonRelease;
	break;
    case WM_RBUTTONUP:
	button[2] = FALSE;
	butnum = 3;
	mev = ButtonRelease;
	break;
    case WM_MOUSEMOVE:
    {
	POINTS pz;
	pz= MAKEPOINTS(lParam); 
	mouseX = pz.x;
	mouseY = pz.y;
    } 
    mev = MotionNotify;
    break;
    case WM_MOUSEWHEEL:
	/* The WM_MOUSEWHEEL message is sent to the focus window 
	 * when the mouse wheel is rotated. The DefWindowProc 
	 * function propagates the message to the window's parent.
	 * There should be no internal forwarding of the message, 
	 * since DefWindowProc propagates it up the parent chain 
	 * until it finds a window that processes it.
	 */
	if(!(wParam & (MK_SHIFT | MK_CONTROL))) {
	    /* gcWheelDelta -= (short) HIWORD(wParam); windows snippet */
	    gcWheelDelta = (short) HIWORD(wParam);
	    mev = MotionNotify;
	    break;
	}

	/* falls through to default ? */

/**************************************************************\
 *     Let the default window proc handle all other messages    *
\**************************************************************/

    default:
	return( DefWindowProc( hWnd, msg, wParam, lParam ));
    }
    if(mev)
    {
	/*void handle_aqua(const int mev, const unsigned int button, int x, int y);*/
	/* butnum=1 left butnum=3 right (butnum=2 middle, not used by freewrl) */
	handle_aqua(mev,butnum,mouseX,mouseY); /* ,gcWheelDelta); */
    }
    return 0;
}

int createWindow32()
{
    HINSTANCE hInstance; 
    WNDCLASS wc;
    MSG msg;

    int nCmdShow = SW_SHOW;
    printf("starting createWindow32\n");
    /* I suspect hInstance should be get() and passed in from the console program not get() in the dll, but .lib maybe ok */
    hInstance = (HANDLE)GetModuleHandle(NULL); 
    window_title = "FreeWRL";

/*  Blender Ghost
    WNDCLASS wc;
    wc.style= CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc= s_wndProc;
    wc.cbClsExtra= 0;
    wc.cbWndExtra= 0;
    wc.hInstance= ::GetModuleHandle(0);
    wc.hIcon = ::LoadIcon(wc.hInstance, "APPICON");
  		
    if (!wc.hIcon) {
    ::LoadIcon(NULL, IDI_APPLICATION);
    }
    wc.hCursor = ::LoadCursor(0, IDC_ARROW);
    wc.hbrBackground= (HBRUSH)::GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName= GHOST_WindowWin32::getWindowClassName();
    
    // Use RegisterClassEx for setting small icon
    if (::RegisterClass(&wc) == 0) {
    success = GHOST_kFailure;
    }
*/
    wc.lpszClassName = "FreeWrlAppClass";
    wc.lpfnWndProc = PopupWndProc; //MainWndProc;
    wc.style = CS_VREDRAW | CS_HREDRAW; /* 0 CS_OWNDC |  */
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW+1 );
    wc.lpszMenuName = 0; /* "GenericAppMenu"; */
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    RegisterClass( &wc );


    ghWnd = CreateWindowEx( WS_EX_APPWINDOW, "FreeWrlAppClass", "freeWrl win32 rev 0.0", 
			    /* ghWnd = CreateWindow( "GenericAppClass", "Generic Application", */
			    WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
			    CW_USEDEFAULT, 
			    CW_USEDEFAULT, 
			    win_width, 
			    win_height, 
			    NULL, 
			    NULL, 
			    hInstance, 
			    NULL); 
    /* make sure window was created */ 
    if (!ghWnd) 
        return FALSE; 

    printf("made a window\n");
   
    ShowWindow( ghWnd, SW_SHOW); /* SW_SHOWNORMAL); /*nCmdShow );*/
    printf("showed window\n");


    UpdateWindow(ghWnd); 
    printf("updated window - leaving createwindow\n");
   
    return TRUE;

}

void openMainWindow (int argc, char **argv)
{
    GLenum err;

    /*
      int argcc;
      char * argvv[1];
      argcc = 1;
      argvv[0] = "freewrlg";
      glutInit(&argcc, argvv);
      glutCreateWindow("GLEW Test");
      create_main_window_win32();
    */
    createWindow32();
    printf("after createwindow32\n");
}

int create_main_window_win32()
{
    createWindow32();
    printf("after createwindow32 in create_main_window_win32\n");
    return TRUE;
}

int doEventsWin32A()
{
    static int eventcount = 0;
    MSG msg;
    do
    {
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE) 
        { 
            if (GetMessage(&msg, NULL, 0, 0) ) 
            { 
                TranslateMessage(&msg);
                DispatchMessage(&msg); 
            } else { 
                return TRUE; 
            } 

        } 
	eventcount++;
    }while(0); /*eventcount < 1000);*/
    eventcount = 0;
    return FALSE;
}

int getEventsWin32(int* ButDown,int len,int* currentX,int* currentY)
{
    int res;
    int i;
    res = doEventsWin32A();
    for(i=0;i<len;i++)
	ButDown[i] = button[i];
    (*currentX) = mouseX;
    (*currentY) = mouseY;
    return TRUE;
	
}

int startMessageLoop()
{
    while(1)
	if(doEventsWin32A()) return TRUE;
    drawScene();
    return FALSE;
}

void resetGeometry()
{
}