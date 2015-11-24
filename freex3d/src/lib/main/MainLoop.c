/*

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
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>
#include <threads.h>
#if HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#if HAVE_TIME_H
# include <time.h>
#endif

#include <sys/stat.h>  // for mkdir


#include "../vrml_parser/Structs.h"
#include "../vrml_parser/CRoutes.h"
#include "headers.h"
#include "../vrml_parser/CParseGeneral.h"
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

#include "../opengl/LoadTextures.h"
#include "../opengl/OpenGL_Utils.h"
#include "../ui/statusbar.h"
#include "../ui/CursorDraw.h"
#include "../scenegraph/RenderFuncs.h"

#include "../ui/common.h"
#include "../io_files.h"

#include "ProdCon.h"

int getRayHitAndSetLookatTarget();
void transformMBB(GLDOUBLE *rMBBmin, GLDOUBLE *rMBBmax, GLDOUBLE *matTransform, GLDOUBLE* inMBBmin, GLDOUBLE* inMBBmax);

// for getting time of day
#if !defined(_MSC_VER)
#include <sys/time.h>
#endif

void (*newResetGeometry) (void) = NULL;

#ifdef WANT_OSC
	#define USE_OSC 1
#else
	#define USE_OSC 0
#endif

#if defined(_ANDROID )
void  setAquaCursor(int ctype) { };

#endif // _ANDROID

#include "MainLoop.h"

double TickTime()
{
	return gglobal()->Mainloop.TickTime;
}
double lastTime()
{
	return gglobal()->Mainloop.lastTime;
}
/* Sensor table. When clicked, we get back from getRayHit the fromnode,
        have to look up type and data in order to properly handle it */
struct SensStruct {
        struct X3D_Node *fromnode;
        struct X3D_Node *datanode;
        void (*interpptr)(void *, int, int, int);
};
struct Touch
{
	int buttonState[4]; /*none down=0, LMB =1, MMB=2, RMB=3*/
	//bool isDown; /* false = up, true = down */
	int mev; /* down/press=4, move/drag=6, up/release=5 */
	int ID;  /* for multitouch: 0-20, represents one finger drag. Recycle after an up */
	float angle; /*some multitouch -like smarttech- track the angle of the finger */
	int x;
	int y; //y-up
	float fx;
	float fy;
	int windex; //multi_window window index 0=default for regular freewrl
};

void pushviewport(Stack *vpstack, ivec4 vp){
	stack_push(ivec4,vpstack,vp);
}
void popviewport(Stack *vpstack){
	stack_pop(ivec4,vpstack);
}
int overlapviewports(ivec4 vp1, ivec4 vp2){
	//0 - outside, 1 - vp1 inside vp2 -1 vp2 inside vp1 2 overlapping
	int inside = 0;
	inside = vp1.X >= vp2.X && (vp1.X+vp1.W) <= (vp2.X+vp2.W) ? 1 : 0;
	if(!inside){
		inside = vp2.X >= vp1.X && (vp2.X+vp2.W) <= (vp1.X+vp1.W) ? -1 : 0;
	}
	if(!inside){
		inside = vp1.X > (vp2.X+vp2.W) || vp1.X > (vp1.X+vp1.W) || vp1.Y > (vp2.Y+vp2.H) || vp2.Y > (vp1.Y+vp1.H) ? 0 : 2;
	}
	return inside;
}
ivec4 intersectviewports(ivec4 vp1, ivec4 vp2){
	ivec4 vpo;
	vpo.X = max(vp1.X,vp2.X);
	vpo.W = min(vp1.X+vp1.W,vp2.X+vp2.W) - vpo.X;
	vpo.Y = max(vp1.Y,vp2.Y);
	vpo.H = min(vp1.Y+vp1.H,vp2.Y+vp2.H) - vpo.Y;
	//printf("olap [%d %d %d %d] ^ [%d %d %d %d] = [%d %d %d %d]\n",vp1.X,vp1.Y,vp1.W,vp1.H,vp2.X,vp2.Y,vp2.W,vp2.H,vpo.X,vpo.Y,vpo.W,vpo.H);
	return vpo;
}
int visibleviewport(ivec4 vp){
	int ok = vp.W > 0 && vp.H > 0;
	return ok;
}
int pointinsideviewport(ivec4 vp, ivec2 pt){
	int inside = TRUE;
	inside = inside && pt.X <= (vp.X + vp.W) && (pt.X >= vp.X);
	inside = inside && pt.Y <= (vp.Y + vp.H) && (pt.Y >= vp.Y);
	return inside;
}
int pointinsidecurrentviewport(Stack *vpstack, ivec2 pt){
	ivec4 vp = stack_top(ivec4,vpstack);
	return pointinsideviewport(vp,pt);
}
void intersectandpushviewport(Stack *vpstack, ivec4 childvp){
	ivec4 currentvp = stack_top(ivec4,vpstack);
	ivec4 olap = intersectviewports(childvp,currentvp);
	pushviewport(vpstack, olap); //I need to unconditionally push, because I will be unconditionally popping later
}
int currentviewportvisible(Stack *vpstack){
	ivec4 currentvp = stack_top(ivec4,vpstack);
	return visibleviewport(currentvp);
}
void setcurrentviewport(Stack *_vpstack){
	ivec4 vp = stack_top(ivec4,_vpstack);
	glViewport(vp.X,vp.Y,vp.W,vp.H);
}


/* eye can be computed automatically from vp (viewpoint)
	mono == vp
	stereo - move left and right from vp by half-eyebase
	front, top, right - use vp position and a primary direction
*/
//typedef struct eye {
//	float *viewport; //fraction of parent viewport left, width, bottom, height
//	void (*pick)(struct eye *e, float *ray); //pass in pickray (and tranform back to prior stage)
//	float pickray[6]; //store transformed pickray
//	void (*cursor)(struct eye *e, int *x, int *y); //return transformed cursor coords, in pixels
//	//BOOL sbh; //true if render statusbarhud at this stage, eye
//} eye;

/* contenttype abstracts scene, statusbarhud, and HMD (head-mounted display) textured-distortion-grid
	- each type has a prep and a render and some data, and a way to handle a pickray
*/
typedef struct contenttype {
	int itype; //0 scene, 1 statusbarHud, 2 texture grid
	void (*render)(); //struct stage *s);
	void (*cursor)(int *x, int *y); //return transformed cursor coords, in pixels
	float viewport[4]; //fraction of parent viewport left, width, bottom, height
	struct contenttype *next;
	void *data;
} contenttype;


typedef struct contenttype_scene {
	//int neyes; //1 mono vp, 2 stereo vp, 4 quadrant front,top,right,vp
	//eye eyes[7]; //full, left, front, top, right, vp
	int eyenumber;
} contenttype_scene;


/* stage wraps contenttype and opengl buffer (screen backbuffer or fbo)
	and allows chaining and forking of stages
	- so a single output window can be rendered from multiple stages
		working back from the window to the scene, the scene is rendered first, 
			then any menu stage
			then any distortion stage
	- and a single stage can be composed of multiple stages, 
		- covering the same viewport but clearing depth buffer between each stage
		- and/or covering sub-viewports / tiles
*/
typedef struct stage {
	unsigned int id; 
	unsigned int ibuffer; //fbo or backbuffer GL_UINT
	contenttype *content;
	contenttype contents[10];
	struct stage *sub_stages; //null terminated list of substages, use stage++ in loop
	float viewport[4]; //fraction of parent viewport left, width, bottom, height
	int want_statusbarHud;
} stage;

typedef struct targetwindow {
	//a target is a window. For example you could have an HMD as one target, 
	//and desktop screen window as another target, both rendered to on the same frame
	void *hwnd; //window handle
	BOOL swapbuf; //true if we should swapbuffer on the target
	ivec4 ivport; //fraction of pixel iviewport we are targeting left, width, bottom, height
	freewrl_params_t params; //will have gl context switching parameters
	stage stages[2]; //pre-allocation
	//stage *output_stages;
	int stages_initialized;
	stage *stage;
	struct targetwindow *next;
} targetwindow;

typedef struct pMainloop{
	//browser
	/* are we displayed, or iconic? */
	int onScreen;// = TRUE;

	/* do we do event propagation, proximity calcs?? */
	int doEvents;// = FALSE;

	#ifdef VERBOSE
	char debs[300];
	#endif

	char* PluginFullPath;
	//
	int num_SensorEvents;// = 0;

	/* Viewport data */
	GLint viewPort2[10];
	GLint viewpointScreenX[2], viewpointScreenY[2]; /*for stereo where we can adjust the viewpoint position on the screen */
	/* screen width and height. */
	struct X3D_Node* CursorOverSensitive;//=NULL;      /*  is Cursor over a Sensitive node?*/
	struct X3D_Node* oldCOS;//=NULL;                   /*  which node was cursor over before this node?*/
	int NavigationMode;//=FALSE;               /*  are we navigating or sensing?*/
	//int ButDown[20][8];// = {{FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE}};

	//int currentCursor;// = 0;
	int lastMouseEvent;// = 0/*MapNotify*/;         /*  last event a mouse did; care about Button and Motion events only.*/
	struct X3D_Node* lastPressedOver;// = NULL;/*  the sensitive node that the mouse was last buttonpressed over.*/
	struct X3D_Node* lastOver;// = NULL;       /*  the sensitive node that the mouse was last moused over.*/
	int lastOverButtonPressed;// = FALSE;      /*  catch the 1 to 0 transition for button presses and isOver in TouchSensors */

	int maxbuffers;// = 1;                     /*  how many active indexes in bufferarray*/
	int bufferarray[2];// = {GL_BACK,0};

	double BrowserStartTime;        /* start of calculating FPS     */
	double BrowserInitTime;		/* time of first frame */

	//int quitThread;// = FALSE;
	int keypress_wait_for_settle;// = 100;     /* JAS - change keypress to wait, then do 1 per loop */
	char * keypress_string;//=NULL;            /* Robert Sim - command line key sequence */

	struct SensStruct *SensorEvents;// = 0;

    unsigned int loop_count;// = 0;
    unsigned int slowloop_count;// = 0;
	//scene
	//window
	//2D_inputdevice
	int lastDeltax;// = 50;
	int lastDeltay;// = 50;
	int lastxx;
	int lastyy;
	int ntouch;// =0;
	int currentTouch;// = -1;
	struct Touch touchlist[20];
	int EMULATE_MULTITOUCH;// = 1;

	FILE* logfile;
	FILE* logerr;
	char* logfname;
	int logging;
	int keySensorMode;
	int draw_initialized;
	int keywait;
	char keywaitstring[25];
	int fps_sleep_remainder;
	double screenorientationmatrix[16];
	double viewtransformmatrix[16];
	double posorimatrix[16];
	double stereooffsetmatrix[2][16];
	targetwindow twindows[4];
	int windex; //current window index into twoindows array, valid during render()
	Stack *_vportstack;
}* ppMainloop;
void *Mainloop_constructor(){
	void *v = MALLOCV(sizeof(struct pMainloop));
	memset(v,0,sizeof(struct pMainloop));
	return v;
}
void Mainloop_init(struct tMainloop *t){
	//public
	/* linewidth for lines and points - passed in on command line */
	t->gl_linewidth= 1.0f;
	//t->TickTime;
	//t->lastTime;
	t->BrowserFPS = 100.0;        /* calculated FPS               */
	t->BrowserSpeed = 0.0;      /* calculated movement speed    */
	t->BrowserDescription = "libfreewrl opensource virtual reality player library";
	t->trisThisLoop = 0;

	/* what kind of file was just parsed? */
	t->currentFileVersion = 0;
	/* do we have some sensitive nodes in scene graph? */
	t->HaveSensitive = FALSE;
	//t->currentX[20];
	//t->currentY[20];                 /*  current mouse position.*/
	t->clipPlane = 0;

	t->tmpFileLocation = MALLOC(char *, 5);
	strcpy(t->tmpFileLocation,"/tmp");
	t->replaceWorldRequest = NULL;
	t->replaceWorldRequestMulti = NULL;
	//private
	t->prv = Mainloop_constructor();
	{
		ppMainloop p = (ppMainloop)t->prv;
		int i;
		//browser
		/* are we displayed, or iconic? */
		p->onScreen = TRUE;

		/* do we do event propagation, proximity calcs?? */
		p->doEvents = FALSE;

		#ifdef VERBOSE
		//static char debs[300];
		#endif

		//char* PluginFullPath;
		p->num_SensorEvents = 0;

		/* Viewport data */
		//p->viewPort2[10];

		/* screen width and height. */
		p->CursorOverSensitive=NULL;      /*  is Cursor over a Sensitive node?*/
		p->oldCOS=NULL;                   /*  which node was cursor over before this node?*/
		p->NavigationMode=FALSE;               /*  are we navigating or sensing?*/
		//p->ButDown[20][8] = {{FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE}}; nulls

		//p->currentCursor = 0;
		p->lastMouseEvent = 0/*MapNotify*/;         /*  last event a mouse did; care about Button and Motion events only.*/
		p->lastPressedOver = NULL;/*  the sensitive node that the mouse was last buttonpressed over.*/
		p->lastOver = NULL;       /*  the sensitive node that the mouse was last moused over.*/
		p->lastOverButtonPressed = FALSE;      /*  catch the 1 to 0 transition for button presses and isOver in TouchSensors */

		p->maxbuffers = 1;                     /*  how many active indexes in bufferarray*/
		p->bufferarray[0] = GL_BACK;
		p->bufferarray[1] = 0;
		/* current time and other time related stuff */
		//p->BrowserStartTime;        /* start of calculating FPS     */
		p->BrowserInitTime = 0.0; /* time of first frame */

		//p->quitThread = FALSE;
		p->keypress_wait_for_settle = 100;     /* JAS - change keypress to wait, then do 1 per loop */
		p->keypress_string=NULL;            /* Robert Sim - command line key sequence */

		p->SensorEvents = 0;

        p->loop_count = 0;
        p->slowloop_count = 0;

		//scene
		//window
		//2D_inputdevice
		p->lastDeltax = 50;
		p->lastDeltay = 50;
		//p->lastxx;
		//p->lastyy;
		p->ntouch =20;
		p->currentTouch = -1;
		//p->touchlist[20];
		p->EMULATE_MULTITOUCH = 0;
		for(i=0;i<p->ntouch;i++) p->touchlist[i].ID = -1;

		p->logfile = NULL;
		p->logerr = NULL;
		p->logfname = NULL;
		p->logging = 0;
		p->keySensorMode = 1; //by default on, so it works 'out of the gate' if Key or StringSensor in scene, then ESC to toggle off
		p->draw_initialized = FALSE;
		p->keywait = FALSE;
		p->keywaitstring[0] = (char)0;
		p->fps_sleep_remainder = 0;
		p->windex = 0;
		t->twindows = p->twindows;
		p->_vportstack = newStack(ivec4);
		t->_vportstack = (void *)p->_vportstack; //represents screen pixel area being drawn to
	}
}
void Mainloop_clear(struct tMainloop *t){
	FREE_IF_NZ(t->scene_name);
	FREE_IF_NZ(t->scene_suff);
	FREE_IF_NZ(t->replaceWorldRequest);
	FREE_IF_NZ(t->tmpFileLocation);
	{
		ppMainloop p = (ppMainloop)t->prv;
		FREE_IF_NZ(p->SensorEvents);
	}
}

//call hwnd_to_windex in frontend window creation and event handling,
//to convert to more convenient int index.
int hwnd_to_windex(void *hWnd){
	int i;
	targetwindow *targets;
	ttglobal tg = gglobal();

	targets = (targetwindow*)tg->Mainloop.twindows;
	for(i=0;i<4;i++){
		//the following line assume hwnd is never natively null or 0
		if(!targets[i].hwnd){
			//not found, create
			targets[i].hwnd = hWnd;
			targets[i].swapbuf = TRUE;
		}
		if(targets[i].hwnd == hWnd) return i;
	}
	return 0;
}

void fwl_getWindowSize(int *width, int *height){
	//call this one when in target rendering loop (and setScreenDim0() 
	// has been called with targetwindow-specific dimensions)
	//the libfreewrl rendering loop should have setScreenDim0 to the appropriate values
	ttglobal tg = gglobal();
	*width = tg->display.screenWidth;
	*height = tg->display.screenHeight;	
}
void fwl_getWindowSize1(int windex, int *width, int *height){
	//call this one when recieving window events, ie mouse events
	//windex: index (0-3, 0=default) of targetwindow the window event came in on
	ivec4 ivport;
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

	ivport = p->twindows[windex].ivport;
	*width = ivport.W;
	*height = ivport.H;	
}


#define LMB 1
#define MMB 2
#define RMB 3
struct Touch *currentTouch(){
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;
	//printf("currentTouch %d\n",p->currentTouch);
	return &p->touchlist[p->currentTouch];
}

//true statics:
int isBrowserPlugin = FALSE; //I can't think of a scenario where sharing this across instances would be a problem
void fwl_set_emulate_multitouch(int ion){
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
	p->EMULATE_MULTITOUCH = ion;
}
int fwl_get_emulate_multitouch(){
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
	return p->EMULATE_MULTITOUCH;
}

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
   ("have scripts to initialize in fwl_RenderSceneUpdateScene old %d new
   %d\n",max_script_found, max_script_found_and_initialized);
*/

/* we bind bindable nodes on parse in this thread */
#define SEND_BIND_IF_REQUIRED(node) \
                if (node != NULL) { /* ConsoleMessage ("sendBind in render"); */ send_bind_to(X3D_NODE(node),1); node = NULL; }



static void setup_viewpoint();
static void set_viewmatrix();

/* Function protos */
static void sendDescriptionToStatusBar(struct X3D_Node *CursorOverSensitive);
/* void fwl_do_keyPress(char kp, int type); Now in lib.h */
void render_collisions(int Viewer_type);
void slerp_viewpoint();
static void render_pre(void);
static void render(void);
static int setup_pickside(int x, int y);
static void setup_projection();
static void setup_pickray(int x, int y);
static struct X3D_Node*  getRayHit(void);
static void get_hyperhit(void);
static void sendSensorEvents(struct X3D_Node *COS,int ev, int butStatus, int status);
#if USE_OSC
void activate_OSCsensors();
#endif


/* libFreeWRL_get_version()

  Q. where do I get this function ?
  A: look in Makefile.am (vtempl will create it automatically in internal_version.c).

*/

/* stop the display thread. Used (when this comment was made) by the OSX Safari plugin; keeps
most things around, just stops display thread, when the user exits a world. */
static void stopDisplayThread()
{
	ttglobal tg = gglobal();
	if (!TEST_NULL_THREAD(tg->threads.DispThrd)) {
		//((ppMainloop)(tg->Mainloop.prv))->quitThread = TRUE;
		tg->threads.MainLoopQuit = max(1, tg->threads.MainLoopQuit); //make sure we don't go backwards in the quit process with a double 'q'
		//pthread_join(tg->threads.DispThrd,NULL);
		//ZERO_THREAD(tg->threads.DispThrd);
	}
}
#ifndef SIGTERM
#define SIGTERM SIG_TERM
#endif






#if !defined(_MSC_VER)

/* Doug Sandens windows function; lets make it static here for non-windows */
double Time1970sec(void) {
		struct timeval mytime;
        gettimeofday(&mytime, NULL);
        return (double) mytime.tv_sec + (double)mytime.tv_usec/1000000.0;
}


#endif


#define DJ_KEEP_COMPILER_WARNING 0
#if DJ_KEEP_COMPILER_WARNING
#define TI(_tv) gettimeofdat(&_tv)
#define TID(_tv) ((double)_tv.tv_sec + (double)_tv.tv_usec/1000000.0)
#endif


/* Main eventloop for FreeWRL!!! */
void fwl_do_keyPress0(int key, int type);
void handle0(const int mev, const unsigned int button, const float x, const float y);
void fwl_handle_aqua_multi(const int mev, const unsigned int button, int x, int y, int ID, int windex);

void fwl_RenderSceneUpdateScene0(double dtime);
void splitpath_local_suffix(const char *url, char **local_name, char **suff);
int vpGroupActive(struct X3D_ViewpointGroup *vp_parent);
void fwl_gotoCurrentViewPoint()
{
	struct tProdCon *t = &gglobal()->ProdCon;

	struct X3D_Node *cn;
	POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, vector_get(struct X3D_Node*, t->viewpointNodes, t->currboundvpno),cn);

	/* printf ("NVP, %d of %d, looking at %d\n",ind, totviewpointnodes, t->currboundvpno);
	printf ("looking at node :%s:\n",X3D_VIEWPOINT(cn)->description->strptr); */

	if (vpGroupActive((struct X3D_ViewpointGroup *) cn)) {
		t->setViewpointBindInRender = vector_get(struct X3D_Node*,t->viewpointNodes, t->currboundvpno);
		return;
	}
}

int fw_exit(int val)
{
	printf("exiting with value=%d hit Enter:",val);
	getchar();
	exit(val);
}

void view_update0(void){
	#if defined(STATUSBAR_HUD)
		/* status bar, if we have one */
		finishedWithGlobalShader();
		drawStatusBar();  // View update
		restoreGlobalShader();
	#endif
}


//static stage output_stages[2];
//static int noutputstages = 2;
//static contenttype contents[2];
/*
render_stage {
	content *data;
	//[set buffer ie if 1-buffer fbo technqiue]
	//push buffer viewport
	vport = childViewport(pvport,s->viewport);
	pushviewport(vportstack, vport);
	//[clear buffer]
	glClear(GL_DEPTH);
	data = s->data;
	while(data){ // scene [,sbh]
		//push content area viewport
		for view in views //for now, just vp, future [,side, top, front]
			push projection
			push / alter view matrix
			for eye in eyes
				[set buffer ie if 2-buffer fbo technique]
				push eye viewport
				push or alter view matrix
				render from last stage output to this stage output, applying stage-specific
				pop
			pop
		pop
		data = data->next;
	}
	popviewport(vportstack);
	setcurrentviewport(vportstack);
}
*/

void render_stage(stage *stagei,double dtime){
	contenttype *content;
	stage *ss;
	ttglobal tg = gglobal();

	if(stagei->ibuffer == 0){
		//rendering to normal backbuffer, use current viewport
		ivec4 ivport = stack_top(ivec4,tg->Mainloop._vportstack);
		pushviewport(tg->Mainloop._vportstack,ivport);
	}else{
		//rendering to fbo
	}
	if(0) setcurrentviewport(tg->Mainloop._vportstack);

	//do the sub-stages first, like you do layers in layersets
	ss = stagei->sub_stages;
	while(ss){
		render_stage(ss, dtime);
		ss++;
	}
	//then render over top the current layer/stage
	content = stagei->content;
	while(content){

		content->render(); //stagei); //for contenttype_scene should iterate over eyes
		content = content->next;
	}
	popviewport(tg->Mainloop._vportstack);
	//setcurrentviewport(tg->display._vportstack);
}
static float fullviewport[4] = {0.0f, 1.0f, 0.0f, 1.0f};
void setup_stagesNORMAL(){
	targetwindow *twindows, *t;
	stage *stages;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;

	twindows = p->twindows;
	t = twindows;
	while(t){
		contenttype *content;
		stage *stagei;
		int noutputstages = 1; //one screen
		stages = t->stages;
		t->stage = &stages[0];
		//for(i=0;i<noutputstages;i++){
			stagei = t->stage;// &stages[i];
			content = &stagei->contents[0];
			stagei->content = content;
			//stagei->content->prep = fwl_RenderSceneUpdateScene0;
			content->render = render;
			stagei->ibuffer = 0;
			//stagei->nsub = 0;
			stagei->sub_stages = NULL;
			stagei->content->itype = 0;

			content = &stagei->contents[1];
			stagei->content->next = content;
			content->render = view_update0; //sbh
			/*
			stagei->neyes = 1;
			for(i=0;i<stagei->neyes;i++){
				eye *eyei = &stagei->eyes[i];
				eyei->cursor = NULL;
				eyei->pick = NULL;
				//eyei->sbh = FALSE;
				//eyei->ibuffer = 0;
				eyei->viewport = fullviewport;
			}
			*/
		//}
		t = t->next;
	}
}
static int stages_initialized = 0;
void fwl_RenderSceneUpdateSceneSTAGES() {
	double dtime;
	int noutputstages;
	targetwindow *t;
	stage *stagei;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;

	if(!stages_initialized){
		setup_stagesNORMAL();
		stages_initialized = 1;
	}

	dtime = Time1970sec();
	fwl_RenderSceneUpdateScene0(dtime);

	t = p->twindows;
	noutputstages = 1;
	stagei = t->stage;
	//for(i=0;i<noutputstages;i++){
	//	stage *stagei = &output_stages[i];
		if ( p->onScreen)
			render_stage(stagei,dtime);
	//}
}
void fwl_RenderSceneUpdateSceneNORMAL() {
	double dtime;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;

	dtime = Time1970sec();
	fwl_RenderSceneUpdateScene0(dtime);
	/* actual rendering */
	if ( p->onScreen) {
		render();
	}

}

//viewport stuff - see Component_Layering.c
ivec4 childViewport(ivec4 parentViewport, float *clipBoundary); 

//#ifdef MULTI_WINDOW
/* MULTI_WINDOW is for desktop configurations where 2 or more windows are rendered to.
	- desktop (windows, linux, mac) only, because it relies on changing the opengl context, 
		and those are desktop-platform-specific calls. GLES2 (opengl es 2) 
		doesn't have those functions - assuming a single window - so can't do multi-window.
	- an example multi-window configuration: 
		1) HMD (head mounted display + 2) supervisors screen
		- they would go through different stages, rendered to different size windows, different menuing
		- and different pickrays -a mouse for supervisor, orientation sensor for HMD 
	Design Option: instead of #ifdef here, all configs could supply 
		fv_swapbuffers() and fwChangeGlContext() functions in the front-end modules,
		including android/mobile/GLES2 which would stub or implement as appropriate
*/


/*
for targetwindow in targets //supervisor screen, HMD
	for stage in stages
		[set buffer ie if 1-buffer fbo technqiue]
		[clear buffer]
		push buffer viewport
		for content in contents // scene [,sbh]
			push content area viewport
			for view in views //for now, just vp, future [,side, top, front]
				push projection
				push / alter view matrix
				for eye in eyes
					[set buffer ie if 2-buffer fbo technique]
					push eye viewport
					push or alter view matrix
					render from last stage output to this stage output, applying stage-specific
					pop
				pop
			pop
		pop
	get final buffer, or swapbuffers	
something similar for pickrays, except pick() instead of render()
- or pickrays (3 per target: [left, right, forehead/mono]),  updated on same pass once per frame

*/


void targetwindow_set_params(int itargetwindow, freewrl_params_t* params){
	targetwindow *twindows, *t;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;
	
	twindows = p->twindows;
	twindows[itargetwindow].params = *params;
	if(itargetwindow > 0){
		twindows[itargetwindow -1].next = &twindows[itargetwindow];
	}
	if(0){
		t = twindows;
		while(t){
			printf("windex=%d t->next = %ld\n",itargetwindow,t->next);
			t=t->next;
		}
		printf("hows that?\n");
	}
}
freewrl_params_t* targetwindow_get_params(int itargetwindow){
	targetwindow *twindows;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;
	
	twindows = p->twindows;
	return &twindows[itargetwindow].params;
}

void fwl_setScreenDim1(int wi, int he, int itargetwindow){
	targetwindow *twindows;
	ivec4 window_rect;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;

	window_rect.X = 0;
	window_rect.Y = 0;
	window_rect.W = wi;
	window_rect.H = he;

	twindows = p->twindows;
	twindows[itargetwindow].ivport = window_rect;
	//the rest is initialized in the target rendering loop, via fwl_setScreenDim(w,h)
}

static int targets_initialized = 0;
float defaultClipBoundary [] = {0.0f, 1.0f, 0.0f, 1.0f}; 
void initialize_targets_simple(){
	stage *stagei;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;

	targetwindow *t = p->twindows;

	if(!t->stages_initialized){
		setup_stagesNORMAL();
		t->stages_initialized = 1;
	}


	//t->next = NULL;
	while(t){
		stagei = t->stage;
			//stage *stagei = &stages[0];
			//stagei->content = &contents[0];
			//stagei->content->prep = fwl_RenderSceneUpdateScene0;
			stagei->content->render = render;
			memcpy(stagei->viewport,defaultClipBoundary,4*sizeof(float));
			stagei->sub_stages = NULL;
		//t->stage = stagei;
		//t->ivport = defaultClipBoundary;
		t->swapbuf = TRUE;
		t=t->next;
	}
	tg->Mainloop.targets_initialized = 1;
}
void fwl_setScreenDim0(int wi, int he);
void fwl_RenderSceneUpdateSceneTARGETWINDOWS() {
	double dtime;
	targetwindow *t, *twindows;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;

	if(!tg->Mainloop.targets_initialized)
		initialize_targets_simple();

	dtime = Time1970sec();
	fwl_RenderSceneUpdateScene0(dtime);

	twindows = p->twindows;
	t = twindows;
	p->windex = -1;
	while(t) { 
		//a targetwindow might be a supervisor's screen, or HMD
		freewrl_params_t *dp;
		Stack *vportstack;
		stage *s;

		p->windex++;
		fwl_setScreenDim0(t->ivport.W, t->ivport.H);
		dp = (freewrl_params_t*)tg->display.params;
		if(t->params.context != dp->context){
			tg->display.params = (void*)&t->params;
			fv_change_GLcontext((freewrl_params_t*)tg->display.params);
			//printf("%ld %ld %ld\n",t->params.display,t->params.context,t->params.surface);
		}
		doglClearColor();

		vportstack = (Stack *)tg->Mainloop._vportstack;
		pushviewport(vportstack,t->ivport);

		s = t->stage;
		//while(s){

			render_stage(s,dtime);
			//s = s->next;
			/*
			content *data;
			//[set buffer ie if 1-buffer fbo technqiue]
			//push buffer viewport
			vport = childViewport(pvport,s->viewport);
			pushviewport(vportstack, vport);
			//[clear buffer]
			glClear(GL_DEPTH);
			data = s->data;
			while(data){ // scene [,sbh]
				//push content area viewport
				for view in views //for now, just vp, future [,side, top, front]
					push projection
					push / alter view matrix
					for eye in eyes
						[set buffer ie if 2-buffer fbo technique]
						push eye viewport
						push or alter view matrix
						render from last stage output to this stage output, applying stage-specific
						pop
					pop
				pop
				data = data->next;
			}
			popviewport(vportstack);
			setcurrentviewport(vportstack);
			*/
		//}
		//get final buffer, or swapbuffers	
		popviewport(vportstack);
		//setcurrentviewport(vportstack);
		if(t->swapbuf) { FW_GL_SWAPBUFFERS }
		t = t->next;
	}
	p->windex = 0;
}
void (*fwl_RenderSceneUpdateScenePTR)() = fwl_RenderSceneUpdateSceneTARGETWINDOWS;
//#else //MULTI_WINDOW
////void (*fwl_RenderSceneUpdateScenePTR)() = fwl_RenderSceneUpdateSceneNORMAL;
//void (*fwl_RenderSceneUpdateScenePTR)() = fwl_RenderSceneUpdateSceneSTAGES;
//#endif //MULTI_WINDOW

/*rendersceneupdatescene overridden with SnapshotRegressionTesting.c  
	fwl_RenderSceneUpdateSceneTESTING during regression testing runs
*/
void fwl_RenderSceneUpdateScene(void){

	fwl_RenderSceneUpdateScenePTR();
}
void setup_picking();
void setup_projection();
void fwl_RenderSceneUpdateScene0(double dtime) {
	//Nov 2015 change: just viewport-independent, once-per-frame-scene-updates here
	//-functionality relying on a viewport -setup_projection(), setup_picking()- has been 
	//	moved to render() which is now called outside this function
	//  this will allow quadrant displays and multiple windows to update the scene once per frame here,
	//  then render to many viewports/windows, pickray from any viewport/window
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;

	/* HAd an issue with Anaglyph rendering on Android; the cursorDraw routine caused the MODELVIEW matrix
	to have the Identity matrix loaded, which caused near/far plane calculations to be dinked.
	 should be set     FW_GL_MATRIX_MODE(GL_MODELVIEW);
		FW_GL_LOAD_IDENTITY(); DO NOT LOAD IDENTITY HERE, ELSE near/Far planes screwed up.
	 if you want to see what happened, load identity matrix here! (uncomment above line)
	*/

	PRINT_GL_ERROR_IF_ANY("start of renderSceneUpdateScene");

	DEBUG_RENDER("start of MainLoop (parsing=%s) (url loaded=%s)\n",
	BOOL_STR(fwl_isinputThreadParsing()), BOOL_STR(resource_is_root_loaded()));

	/* should we do events, or maybe a parser is parsing? */
	p->doEvents = (!fwl_isinputThreadParsing()) && (!fwl_isTextureParsing()) && fwl_isInputThreadInitialized();
	/* First time through */
	if (p->loop_count == 0) {
		p->BrowserStartTime = dtime; //Time1970sec();
		tg->Mainloop.TickTime = p->BrowserStartTime;
		tg->Mainloop.lastTime = tg->Mainloop.TickTime - 0.01; /* might as well not invoke the usleep below */
		if(p->BrowserInitTime == 0.0)
			p->BrowserInitTime = dtime;
	} else {
		/* NOTE: front ends now sync with the monitor, meaning, this sleep is no longer needed unless
			something goes totally wrong.
			Perhaps could be moved up a level, since mobile controls in frontend, but npapi and activex plugins also need displaythread  */
#ifndef FRONTEND_HANDLES_DISPLAY_THREAD
		if(!((freewrl_params_t*)(tg->display.params))->frontend_handles_display_thread){
			/* 	some users report their device overheats if frame rate is a zillion, so this will limit it to a target number
				statusbarHud options has an option to set.
				we see how long it took to do the last loop; now that the frame rate is synced to the
				vertical retrace of the screens, we should not get more than 60-70fps. We calculate the
				time here, if it is more than 200fps, we sleep for 1/100th of a second - we should NOT
				need this, but in case something goes pear-shaped (british expression, there!) we do not
				consume thousands of frames per second 
				frames-per-second = FPS = 1/time-per-frame[s];  [s] means seconds, [ms] millisec [us] microseconds [f] frames
				target_time_per_frame[s] = 1[f]/target_FPS[f/s];
				suggested_wait_time[s] = target_time_per_frame[s] - elapsed_time_since_last_frame[s];
										= 1[f]/target_FPS[f/s]    - elapsed_time_since_last_frame[s];
				if suggested_wait_time < 0 then we can't keep up, no wait time

			*/
			double elapsed_time_per_frame, suggested_wait_time, target_time_per_frame, kludgefactor;
			int wait_time_micro_sec, target_frames_per_second;
			kludgefactor = 2.0; //2 works on win8.1 with intel i5
			target_frames_per_second = fwl_get_target_fps();
			elapsed_time_per_frame = TickTime() - lastTime();
			if(target_frames_per_second > 0)
				target_time_per_frame = 1.0/(double)target_frames_per_second;
			else
				target_time_per_frame = 1.0/30.0;
			suggested_wait_time = target_time_per_frame - elapsed_time_per_frame;
			suggested_wait_time *= kludgefactor;

			wait_time_micro_sec = (int)(suggested_wait_time * 1000000.0);
			if(wait_time_micro_sec > 1)
				usleep(wait_time_micro_sec);
		}
#endif /* FRONTEND_HANDLES_DISPLAY_THREAD */
	}

	// Set the timestamp
	tg->Mainloop.lastTime = tg->Mainloop.TickTime;
	tg->Mainloop.TickTime = dtime; //Time1970sec();

	#if !defined(FRONTEND_DOES_SNAPSHOTS)
	// handle snapshots
	if (tg->Snapshot.doSnapshot) {
		Snapshot();
	}
	#endif //FRONTEND_DOES_SNAPSHOTS

	OcclusionCulling();

	// any scripts to do??
#ifdef _MSC_VER
	if(p->doEvents)
#endif /* _MSC_VER */

	#ifdef HAVE_JAVASCRIPT
		initializeAnyScripts();
	#endif


	// BrowserAction required? eg, anchors, etc
#ifndef DISABLER
	if (tg->RenderFuncs.BrowserAction) {
		tg->RenderFuncs.BrowserAction = doBrowserAction ();
	}
#endif

	//doglClearColor();

	OcclusionStartofRenderSceneUpdateScene();

	startOfLoopNodeUpdates();

	if (p->loop_count == 25) {
		tg->Mainloop.BrowserFPS = 25.0 / (TickTime()-p->BrowserStartTime);
		setMenuFps((float)tg->Mainloop.BrowserFPS); /*  tell status bar to refresh, if it is displayed*/
		// printf ("fps %f tris %d, rootnode children %d \n",p->BrowserFPS,p->trisThisLoop, X3D_GROUP(rootNode)->children.n);
		//ConsoleMessage("fps %f tris %d\n",tg->Mainloop.BrowserFPS,tg->Mainloop.trisThisLoop);
		//printf ("MainLoop, nearPlane %lf farPlane %lf\n",Viewer.nearPlane, Viewer.farPlane);
		p->BrowserStartTime = TickTime();
		p->loop_count = 1;
	} else {
		p->loop_count++;
	}

	tg->Mainloop.trisThisLoop = 0;

	if(p->slowloop_count == 1009) p->slowloop_count = 0 ;
	#if USE_OSC
	if ((p->slowloop_count % 256) == 0) {
		/* activate_picksensors() ; */
		/*
		printf("slowloop_count = %d at T=%lf : lastMouseEvent=%d , MotionNotify=%d\n",
			p->slowloop_count, TickTime(), p->lastMouseEvent, MotionNotify) ;
		*/
		activate_OSCsensors() ;
	} else {
		/* deactivate_picksensors() ; */
	}
	#endif /* USE_OSC */

	p->slowloop_count++ ;

	// handle any events provided on the command line - Robert Sim 
	if (p->keypress_string && p->doEvents) {
		if (p->keypress_wait_for_settle > 0) {
			p->keypress_wait_for_settle--;
		} else {
			// dont do the null... 
			if (*p->keypress_string) {
				// printf ("handling key %c\n",*p->keypress_string); 
#if !defined( AQUA ) && !defined( _MSC_VER )  /*win32 - don't know whats it is suppsoed to do yet */
				DEBUG_XEV("CMD LINE GEN EVENT: %c\n", *p->keypress_string);
				fwl_do_keyPress(*p->keypress_string,KeyPress);
#endif /* NOT AQUA and NOT WIN32 */
				p->keypress_string++;
			} else {
				p->keypress_string=NULL;
			}
		}
	}

#if KEEP_X11_INLIB
	/**
	 *   Merge of Bare X11 and Motif/X11 event handling ...
	 */
	/* REMARK: Do we want to process all pending events ? */

#if defined(TARGET_X11)
	/* We are running our own bare window */
	while (XPending(Xdpy)) {
		XNextEvent(Xdpy, &event);
		DEBUG_XEV("EVENT through XNextEvent\n");
		handle_Xevents(event);
	}
#endif /* TARGET_X11 */


	PRINT_GL_ERROR_IF_ANY("before xtdispatch");
#if defined(TARGET_MOTIF)
	/* any updates to the menu buttons? Because of Linux threading
		issues, we try to make all updates come from 1 thread */
	frontendUpdateButtons();

	/* do the Xt events here. */
	while (XtAppPending(Xtcx)!= 0) {
		XtAppNextEvent(Xtcx, &event);
#ifdef XEVENT_VERBOSE
		XButtonEvent *bev;
		XMotionEvent *mev;
		switch (event.type) {
			case MotionNotify:
			mev = &event.xmotion;
			TRACE_MSG("mouse motion event: win=%u, state=%d\n",mev->window, mev->state);
		break;
		case ButtonPress:
		case ButtonRelease:
		bev = &event.xbutton;
		TRACE_MSG("mouse button event: win=%u, state=%d\n",bev->window, bev->state);
		break;
	}
#endif /* XEVENT_VERBOSE */

		DEBUG_XEV("EVENT through XtDispatchEvent\n");
		XtDispatchEvent (&event);
	}

#endif /* TARGET_MOTIF */
#endif /* KEEP_X11_INLIB */


	/* Viewer move viewpoint */
	handle_tick();

	PRINT_GL_ERROR_IF_ANY("after handle_tick")
	/* setup Projection and activate ProximitySensors */
	if (p->onScreen)
	{
		render_pre();
		//slerp_viewpoint(); //moved inside render_pre > setup_viewpoint

	}

	if (p->doEvents) {
		/* and just parsed nodes needing binding? */
		SEND_BIND_IF_REQUIRED(tg->ProdCon.setViewpointBindInRender)
		SEND_BIND_IF_REQUIRED(tg->ProdCon.setFogBindInRender)
		SEND_BIND_IF_REQUIRED(tg->ProdCon.setBackgroundBindInRender)
		SEND_BIND_IF_REQUIRED(tg->ProdCon.setNavigationBindInRender)

		/* handle ROUTES - at least the ones not generated in do_first() */
		do_first(); //propagate events called from do_first

		/* Javascript events processed */
		process_eventsProcessed();

		#if !defined(EXCLUDE_EAI)
		// the fwlio_SCK* funcs to get data into the system, and calls the fwl_EAI*
		// funcs to give the data to the EAI,nd the fwl_MIDI* funcs for MIDI
		//
		// Actions are now separate so that file IO is not tightly coupled
		// via shared buffers and file descriptors etc. 'The core' now calls
		// Although the MIDI code and the EAI code are basically the same
		// and one could compress them into a loop, for the moment keep
		// them seperate to serve as a example for any extensions...
		// handle_EAI(); 
		{
		int socketVerbose = fwlio_RxTx_control(CHANNEL_EAI, RxTx_GET_VERBOSITY)  ;
		if ( socketVerbose <= 1 || (socketVerbose > 1 && ((p->slowloop_count % 256) == 0)) ) {
			if(fwlio_RxTx_control(CHANNEL_EAI, RxTx_REFRESH) == 0) {
				/* Nothing to be done, maybe not even running */
				if ( socketVerbose > 1 ) {
					printf("%s:%d Nothing to be done\n",__FILE__,__LINE__) ;
				}
			} else {
				if ( socketVerbose > 1 ) {
					printf("%s:%d Test RxTx_PENDING\n",__FILE__,__LINE__) ;
				}
				if(fwlio_RxTx_control(CHANNEL_EAI, RxTx_PENDING) > 0) {
					char *tempEAIdata;
					if ( socketVerbose != 0 ) {
						printf("%s:%d Something pending\n",__FILE__,__LINE__) ;
					}
					tempEAIdata = fwlio_RxTx_getbuffer(CHANNEL_EAI) ;
					if(tempEAIdata != (char *)NULL) {
						char * replyData;
						int EAI_StillToDo;
						if ( socketVerbose != 0 ) {
							printf("%s:%d Something for EAI to do with buffer addr %p\n",__FILE__,__LINE__,tempEAIdata ) ;
						}
						// Every incoming command has a reply,
						// and the reply is synchronous.
						replyData = fwl_EAI_handleBuffer(tempEAIdata);
						FREE(tempEAIdata) ;
						EAI_StillToDo = 1;
						do {
							if(replyData != NULL && strlen(replyData) != 0) {
								fwlio_RxTx_sendbuffer(__FILE__,__LINE__,CHANNEL_EAI, replyData) ;
								FREE(replyData) ;
								// Note: fwlio_RxTx_sendbuffer() can also be called async
								// due to a listener trigger within routing, but it is
								// is up to that caller to clean out its own buffers.
							}
							EAI_StillToDo = fwl_EAI_allDone();
							if(EAI_StillToDo) {
								if ( socketVerbose != 0 ) {
									printf("%s:%d Something still in EAI buffer? %d\n",__FILE__,__LINE__,EAI_StillToDo ) ;
								}
								replyData = fwl_EAI_handleRest();
							}
						} while(EAI_StillToDo) ;
					} //temEAIdata
				} //fwlio PENDING
			} //fwlio REFRESH
		} //socketverbose
		}
		#endif //EXCLUDE_EAI
	} //doEvents

#ifdef RENDERVERBOSE
	printf("RENDER STEP----------\n");
#endif

	/* ensure depth mask turned on here */
	FW_GL_DEPTHMASK(GL_TRUE);
	PRINT_GL_ERROR_IF_ANY("after depth")

}

void setup_picking(){
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;


	/* handle_mouse events if clicked on a sensitive node */
	//printf("nav mode =%d sensitive= %d\n",p->NavigationMode, tg->Mainloop.HaveSensitive);
	if (!p->NavigationMode && tg->Mainloop.HaveSensitive && !Viewer()->LookatMode && !tg->Mainloop.SHIFT) {
		//p->currentCursor = 0;
		int x,yup;
		struct Touch *touch = currentTouch();
		x = touch->x;
		yup = touch->y;
		if(setup_pickside(x,yup)){
			setup_projection();
			setup_pickray(x,yup);
			//setup_viewpoint();
			set_viewmatrix();
			render_hier(rootNode(),VF_Sensitive  | VF_Geom);
			p->CursorOverSensitive = getRayHit();

			/* for nodes that use an "isOver" eventOut... */
			if (p->lastOver != p->CursorOverSensitive) {
				#ifdef VERBOSE
					printf ("%lf over changed, p->lastOver %u p->cursorOverSensitive %u, p->butDown1 %d\n",
						TickTime(), (unsigned int) p->lastOver, (unsigned int) p->CursorOverSensitive,
						p->ButDown[p->currentCursor][1]);
				#endif
				//if (p->ButDown[p->currentCursor][1]==0) {
				if (touch->buttonState[LMB]==0) {

					/* ok, when the user releases a button, cursorOverSensitive WILL BE NULL
						until it gets sensed again. So, we use the lastOverButtonPressed flag to delay
						sending this flag by one event loop loop. */
					if (!p->lastOverButtonPressed) {
						sendSensorEvents(p->lastOver, overMark, 0, FALSE);
						sendSensorEvents(p->CursorOverSensitive, overMark, 0, TRUE);
						p->lastOver = p->CursorOverSensitive;
					}
					p->lastOverButtonPressed = FALSE;
				} else {
					p->lastOverButtonPressed = TRUE;
				}
			}
			#ifdef VERBOSE
			if (p->CursorOverSensitive != NULL)
				printf("COS %d (%s)\n", (unsigned int) p->CursorOverSensitive, stringNodeType(p->CursorOverSensitive->_nodeType));
			#endif /* VERBOSE */

			/* did we have a click of button 1? */
			//if (p->ButDown[p->currentCursor][1] && (p->lastPressedOver==NULL)) {
			if (touch->buttonState[LMB] && (p->lastPressedOver==NULL)) {
				/* printf ("Not Navigation and 1 down\n"); */
				/* send an event of ButtonPress and isOver=true */
				p->lastPressedOver = p->CursorOverSensitive;
				sendSensorEvents(p->lastPressedOver, ButtonPress, touch->buttonState[LMB], TRUE); //p->ButDown[p->currentCursor][1], TRUE);
			}
			//if ((p->ButDown[p->currentCursor][1]==0) && p->lastPressedOver!=NULL) {
			if ((touch->buttonState[LMB]==0) && p->lastPressedOver!=NULL) {
				/* printf ("Not Navigation and 1 up\n");  */
				/* send an event of ButtonRelease and isOver=true;
					an isOver=false event will be sent below if required */
				sendSensorEvents(p->lastPressedOver, ButtonRelease, touch->buttonState[LMB], TRUE); //p->ButDown[p->currentCursor][1], TRUE);
				p->lastPressedOver = NULL;
			}
			if (p->lastMouseEvent == MotionNotify) {
				/* printf ("Not Navigation and motion - going into sendSensorEvents\n"); */
				/* TouchSensor hitPoint_changed needs to know if we are over a sensitive node or not */
				sendSensorEvents(p->CursorOverSensitive,MotionNotify, touch->buttonState[LMB], TRUE); //p->ButDown[p->currentCursor][1], TRUE);

				/* PlaneSensors, etc, take the last sensitive node pressed over, and a mouse movement */
				sendSensorEvents(p->lastPressedOver,MotionNotify, touch->buttonState[LMB], TRUE); //p->ButDown[p->currentCursor][1], TRUE);
				p->lastMouseEvent = 0 ;
			}

			/* do we need to re-define cursor style? */
			/* do we need to send an isOver event? */
			if (p->CursorOverSensitive!= NULL) {
				setSensorCursor();

				/* is this a new node that we are now over?
					don't change the node pointer if we are clicked down */
				if ((p->lastPressedOver==NULL) && (p->CursorOverSensitive != p->oldCOS)) {
					//sendSensorEvents(p->oldCOS,MapNotify,p->ButDown[p->currentCursor][1], FALSE);
					sendSensorEvents(p->oldCOS,MapNotify,touch->buttonState[LMB], FALSE);
					//sendSensorEvents(p->CursorOverSensitive,MapNotify,p->ButDown[p->currentCursor][1], TRUE);
					sendSensorEvents(p->CursorOverSensitive,MapNotify,touch->buttonState[LMB], TRUE);
					 p->oldCOS=p->CursorOverSensitive;
					sendDescriptionToStatusBar(p->CursorOverSensitive);
				}
			} else {
				/* hold off on cursor change if dragging a sensor */
				if (p->lastPressedOver!=NULL) {
					setSensorCursor();
				} else {
					setArrowCursor();
				}
				/* were we over a sensitive node? */
				//if ((p->oldCOS!=NULL)  && (p->ButDown[p->currentCursor][1]==0)) {
				if ((p->oldCOS!=NULL)  && (touch->buttonState[LMB]==0)) {
					sendSensorEvents(p->oldCOS,MapNotify, touch->buttonState[LMB], FALSE); //p->ButDown[p->currentCursor][1], FALSE);
					/* remove any display on-screen */
					sendDescriptionToStatusBar(NULL);
					p->oldCOS=NULL;
				}
			}
		}
	} /* (!NavigationMode && HaveSensitive) */
	else if(Viewer()->LookatMode){
		//pick a target object to travel to
		if(Viewer()->LookatMode < 3)
			setLookatCursor();
		if(Viewer()->LookatMode == 2){
			//p->currentCursor = 0;
			int x, yup;
			struct Touch * touch = currentTouch();
			x = touch->x;
			yup = touch->y;
			if(setup_pickside(x,yup)){ //tg->Mainloop.currentX[p->currentCursor],tg->Mainloop.currentY[p->currentCursor])){
				setup_projection();
				setup_pickray(x,yup); //tg->Mainloop.currentX[p->currentCursor],tg->Mainloop.currentY[p->currentCursor]);
				setup_viewpoint();
				set_viewmatrix();
				render_hier(rootNode(),VF_Sensitive  | VF_Geom);
				getRayHitAndSetLookatTarget();
			}
		}
		if(Viewer()->LookatMode == 0) ///> 2)
			setArrowCursor();
	}else{
		//normal or navigation mode
		setArrowCursor();
	}

}


void (*handlePTR)(const int mev, const unsigned int button, const float x, const float y) = handle0;
void handle(const int mev, const unsigned int button, const float x, const float y)
{
	handlePTR(mev, button, x, y);
}



#if !defined( AQUA ) && !defined( _MSC_VER ) &&!defined (_ANDROID)
//XK_ constants from /usr/include/X11/keysymdef.h
#define PHOME_KEY XK_Home //80
#define PPGDN_KEY XK_Page_Down //86
#define PLEFT_KEY XK_Left //106
#define PEND_KEY XK_End //87
#define PUP_KEY XK_Up //112
#define PRIGHT_KEY XK_Right //108
#define PPGUP_KEY XK_Page_Up //85
#define PDOWN_KEY XK_Down //59
#define PF1_KEY  XK_F1 //0xFFBE
//OLDCODE #define PF2_KEY  XK_F2 //0xFFBF
//OLDCODE #define PF3_KEY  XK_F3 //0XFFC0
//OLDCODE #define PF4_KEY  XK_F4 //0XFFC1
//OLDCODE #define PF5_KEY  XK_F5 //0XFFC2
//OLDCODE #define PF6_KEY  XK_F6 //0XFFC3
//OLDCODE #define PF7_KEY  XK_F7 //0XFFC4
//OLDCODE #define PF8_KEY  XK_F8 //0XFFC5
//OLDCODE #define PF9_KEY  XK_F9 //0XFFC6
//OLDCODE #define PF10_KEY XK_F10 //0XFFC7
//OLDCODE #define PF11_KEY XK_F11 //0XFFC8
#define PF12_KEY XK_F12 //0XFFC9
#define PALT_KEY XK_Alt_L //0XFFE9 //left, and 0XFFEA   //0XFFE7
#define PALT_KEYR XK_Alt_R //0XFFE9 //left, and 0XFFEA   //0XFFE7
#define PCTL_KEY XK_Control_L //0XFFE3 //left, and 0XFFE4 on right
#define PCTL_KEYR XK_Control_R //0XFFE3 //left, and 0XFFE4 on right
#define PSFT_KEY XK_Shift_L //0XFFE1 //left, and 0XFFE2 on right
#define PSFT_KEYR XK_Shift_R //0XFFE1 //left, and 0XFFE2 on right
#define PDEL_KEY XK_Delete //0XFF9F //on numpad, and 0XFFFF near Insert //0x08
//OLDCODE #define PRTN_KEY XK_Return //XK_KP_Enter //0xff0d 13
#define PNUM0 XK_KP_Insert    //XK_KP_0
#define PNUM1 XK_KP_End       //XK_KP_1
#define PNUM2 XK_KP_Down      //XK_KP_2
#define PNUM3 XK_KP_Page_Down //XK_KP_3
#define PNUM4 XK_KP_Left      //XK_KP_4
#define PNUM5 XK_KP_Begin     //XK_KP_5
#define PNUM6 XK_KP_Right     //XK_KP_6
#define PNUM7 XK_KP_Home      //XK_KP_7
#define PNUM8 XK_KP_Up        //XK_KP_8
#define PNUM9 XK_KP_Page_Up   //XK_KP_9
#define PNUMDEC XK_KP_Delete //XK_KP_Decimal

//OLDCODE #define KEYPRESS 1
//OLDCODE #define KEYDOWN 2
//OLDCODE #define KEYUP	3

///* from http://www.web3d.org/x3d/specifications/ISO-IEC-19775-1.2-X3D-AbstractSpecification/index.html
//section 21.4.1
//Key Value
//Home 13
//End 14
//PGUP 15
//PGDN 16
//UP 17
//DOWN 18
//LEFT 19
//RIGHT 20
//F1-F12  1 to 12
//ALT,CTRL,SHIFT true/false
//*/
//#define F1_KEY  1
//#define F2_KEY  2
//#define F3_KEY  3
//#define F4_KEY  4
//#define F5_KEY  5
//#define F6_KEY  6
//#define F7_KEY  7
//#define F8_KEY  8
//#define F9_KEY  9
//#define F10_KEY 10
//#define F11_KEY 11
//#define F12_KEY 12
//#define HOME_KEY 13
//#define END_KEY  14
//#define PGUP_KEY 15
//#define PGDN_KEY 16
//#define UP_KEY   17
//#define DOWN_KEY 18
//#define LEFT_KEY 19
//#define RIGHT_KEY 20
//#define ALT_KEY	30 /* not available on OSX */
//#define CTL_KEY 31 /* not available on OSX */
//#define SFT_KEY 32 /* not available on OSX */
//#define DEL_KEY 0XFFFF /* problem: I'm insterting this back into the translated char stream so 0XFFFF too high to clash with a latin? */
//#define RTN_KEY 13  //what about 10 newline?


int platform2web3dActionKeyLINUX(int platformKey)
{
	int key;

	key = 0; //platformKey;
	if(platformKey >= PF1_KEY && platformKey <= PF12_KEY)
		key = platformKey - PF1_KEY + F1_KEY;
	else
		switch(platformKey)
		{
		case PHOME_KEY:
			key = HOME_KEY; break;
		case PEND_KEY:
			key = END_KEY; break;
		case PPGDN_KEY:
			key = PGDN_KEY; break;
		case PPGUP_KEY:
			key = PGUP_KEY; break;
		case PUP_KEY:
			key = UP_KEY; break;
		case PDOWN_KEY:
			key = DOWN_KEY; break;
		case PLEFT_KEY:
			key = LEFT_KEY; break;
		case PRIGHT_KEY:
			key = RIGHT_KEY; break;
		case PDEL_KEY:
			key = DEL_KEY; break;
		case PALT_KEY:
		case PALT_KEYR:
			key = ALT_KEY; break;
		case PCTL_KEY:
		case PCTL_KEYR:
			key = CTL_KEY; break;
		case PSFT_KEY:
		case PSFT_KEYR:
			key = SFT_KEY; break;
		case PNUM0:
			key = NUM0; break;
		case PNUM1:
			key = NUM1; break;
		case PNUM2:
			key = NUM2; break;
		case PNUM3:
			key = NUM3; break;
		case PNUM4:
			key = NUM4; break;
		case PNUM5:
			key = NUM5; break;
		case PNUM6:
			key = NUM6; break;
		case PNUM7:
			key = NUM7; break;
		case PNUM8:
			key = NUM8; break;
		case PNUM9:
			key = NUM9; break;
		case PNUMDEC:
			key = NUMDEC; break;
		default:
			key = 0;
		}
	return key;
}


void handle_Xevents(XEvent event) {

	XEvent nextevent;
	char buf[10];
	KeySym ks, ksraw, ksupper, kslower;
	KeySym *keysym;

	int keysyms_per_keycode_return;

	//int count;
	int actionKey;
	int cursorStyle;
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;
	p->lastMouseEvent=event.type;

#ifdef VERBOSE
	switch (event.type) {
		case ConfigureNotify: printf ("Event: ConfigureNotify\n"); break;
		case ClientMessage: printf ("Event: ClientMessage\n"); break;
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
//#ifdef HAVE_NOTOOLKIT
		/* Motif, etc, usually handles this. */
		case ConfigureNotify:
			/*  printf("%s,%d ConfigureNotify  %d %d\n",__FILE__,__LINE__,event.xconfigure.width,event.xconfigure.height); */
//#ifdef STATUSBAR_HUD
//			statusbar_set_window_size(event.xconfigure.width,event.xconfigure.height);
//#else
			fwl_setScreenDim (event.xconfigure.width,event.xconfigure.height);
//#endif
			break;
//#endif
		case ClientMessage:
			if (event.xclient.data.l[0] == WM_DELETE_WINDOW && !RUNNINGASPLUGIN) {
				#ifdef VERBOSE
				printf("---XClient sent wmDeleteMessage, quitting freewrl\n");
				#endif
				fwl_doQuit();
			}
			break;
		case KeyPress:
		case KeyRelease:
			XLookupString(&event.xkey,buf,sizeof(buf),&ks,0);
			///*  Map keypad keys in - thanks to Aubrey Jaffer.*/
			//if(0) switch(ks) {
			//	/*  the non-keyboard arrow keys*/
			//	case XK_Left: ks = XK_j; break;
			//	case XK_Right: ks = XK_l; break;
			//	case XK_Up: ks = XK_p; break;
			//	case XK_Down: ks = XK_semicolon; break;
			//	case XK_KP_0:
			//	case XK_KP_Insert:
			//		ks = XK_a; break;
			//	case XK_KP_Decimal:
			//	case XK_KP_Delete:
			//		ks = XK_z; break;
			//	case XK_KP_7:
			//	case XK_KP_Home:
			//		ks = XK_7; break;
			//	case XK_KP_9:
			//	case XK_KP_Page_Up:
			//		ks = XK_9; break;
			//	case XK_KP_8:
			//	case XK_KP_Up:
			//		ks = XK_k; break;
			//	case XK_KP_2:
			//	case XK_KP_Down:
			//		ks = XK_8; break;
			//	case XK_KP_4:
			//	case XK_KP_Left:
			//		ks = XK_u; break;
			//	case XK_KP_6:
			//	case XK_KP_Right:
			//		ks = XK_o; break;
			//	case XK_Num_Lock: ks = XK_h; break;
			//		default: break;
			//}

			/* doubt that this is necessary */
			buf[0]=(char)ks;buf[1]='\0';

			DEBUG_XEV("Key type = %s\n", (event.type == KeyPress ? "KEY PRESS" : "KEY  RELEASE"));
			//fwl_do_keyPress((char)ks,event.type);
			//ksraw = (char)buf[0];


			// deprecated:  ksraw = XKeycodeToKeysym(event.xkey.display, event.xkey.keycode, 0);

			keysym = XGetKeyboardMapping(event.xkey.display,
				event.xkey.keycode, 1, &keysyms_per_keycode_return);
			ksraw = *keysym;
			XFree(keysym);

			XConvertCase(ksraw,&kslower,&ksupper);

			ksraw = ksupper;
			if(event.type == KeyRelease && !IsModifierKey(ks)
				&& !IsFunctionKey(ks) && !IsMiscFunctionKey(ks) && !IsCursorKey(ks)){
				fwl_do_rawKeyPress((int)ks,1);
				//printf("ks=%c %d %o %x\n",ks,(int)ks,(int)ks,(int)ks);
			}
			//printf("ksraw=%c %d %o %x\n",ksraw,(int)ksraw,(int)ksraw,(int)ksraw);
			actionKey = platform2web3dActionKeyLINUX(ksraw);
			if(actionKey)
				fwl_do_rawKeyPress(actionKey,event.type+10);
			else
				fwl_do_rawKeyPress(ksraw,event.type);
			break;

		case ButtonPress:
		case ButtonRelease:
#ifdef STATUSBAR_HUD
			cursorStyle = statusbar_handle_mouse(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y);
#else
			cursorStyle = fwl_handle_aqua(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y);
#endif
			setCursor(cursorStyle);
			//if(0){
			//	/* printf("got a button press or button release\n"); */
			//	/*  if a button is pressed, we should not change state,*/
			//	/*  so keep a record.*/
			//	if(handleStatusbarHud(event.type, &tg->Mainloop.clipPlane))break;
			//	if (event.xbutton.button>=5) break;  /* bounds check*/
			//	p->ButDown[p->currentCursor][event.xbutton.button] = (event.type == ButtonPress);

			//	/* if we are Not over an enabled sensitive node, and we do NOT
			//		already have a button down from a sensitive node... */
			//	/* printf("cursoroversensitive is %u lastPressedOver %u\n", p->CursorOverSensitive,p->lastPressedOver); */
			//	if ((p->CursorOverSensitive==NULL) && (p->lastPressedOver==NULL))  {
			//			p->NavigationMode=p->ButDown[p->currentCursor][1] || p->ButDown[p->currentCursor][3];
			//			handle (event.type,event.xbutton.button,
			//					(float) ((float)event.xbutton.x/tg->display.screenWidth),
			//					(float) ((float)event.xbutton.y/tg->display.screenHeight));
			//	}
			//}
			break;

		case MotionNotify:
#if KEEP_X11_INLIB
			/* printf("got a motion notify\n"); */
			/*  do we have more motion notify events queued?*/
			if (XPending(Xdpy)) {
					XPeekEvent(Xdpy,&nextevent);
					if (nextevent.type==MotionNotify) { break;
					}
			}
#endif /* KEEP_X11_INLIB */
#ifdef STATUSBAR_HUD
			cursorStyle = statusbar_handle_mouse(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y);
#else
			cursorStyle = fwl_handle_aqua(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y);
#endif
			setCursor(cursorStyle);
			//if(0){

			//	/*  save the current x and y positions for picking.*/
			//	tg->Mainloop.currentX[p->currentCursor] = event.xbutton.x;
			//	tg->Mainloop.currentY[p->currentCursor] = event.xbutton.y;
			//	/* printf("navigationMode is %d\n", NavigationMode); */
			//	if(handleStatusbarHud(6, &tg->Mainloop.clipPlane))break;
			//	if (p->NavigationMode) {
			//			/*  find out what the first button down is*/
			//			count = 0;
			//			while ((count < 5) && (!p->ButDown[p->currentCursor][count])) count++;
			//			if (count == 5) return; /*  no buttons down???*/

			//			handle (event.type,(unsigned)count,
			//					(float)((float)event.xbutton.x/tg->display.screenWidth),
			//					(float)((float)event.xbutton.y/tg->display.screenHeight));
			//	}
			//}
			break;
	}
}
#endif

/* get setup for rendering. */
#ifdef DJTRACK_PICKSENSORS
void do_pickSensors();
int enabled_picksensors();
#endif
void SSR_test_cumulative_pose();
static void render_pre() {
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;

        /* 1. Set up projection */
        // Nov 2015 moved render(): setup_projection(); //FALSE,0,0);


        /* 2. Headlight, initialized here where we have the modelview matrix to Identity.
        FIXME: position of light sould actually be offset a little (towards the center)
        when in stereo mode. */

        if (fwl_get_headlight()) {
		setLightState(HEADLIGHT_LIGHT,TRUE);
		setLightType(HEADLIGHT_LIGHT,2); // DirectionalLight
	}


        ///* 3. Viewpoint */
        //setup_viewpoint();      
		/*  need this to render collisions correctly 
				x Oct 2015 change: rely on last frame's results for this frames collision*/

#ifdef SSR_SERVER
		//just for a diagnostic test of transforms - replaces modelview matrix with one formed from cumQuat,cumTrans
		if(0){
			static double toggleTime = 0.0;
			static int runTest = 0;
			double dtime;
			dtime = TickTime();
			if(dtime - toggleTime > 5.0){
				//alternate between ordinary view and test view every 5 seconds, to visually compare
				runTest = 1 - runTest;
				toggleTime = dtime;
			}
			if(runTest) SSR_test_cumulative_pose();
		}
#endif


        /* 4. Collisions */
        if (fwl_getCollision() == 1) {
			profile_start("collision");
                render_collisions(Viewer()->type);
				profile_end("collision");
                // setup_viewpoint(); //see 5 lines below
        }

		/* 3. Viewpoint */
		/*  unconditionally update viewer position after collision, to*/
		/*  give accurate info to Proximity sensors.*/
		setup_viewpoint(); //Oct 2015: now this is the only setup_viewpoint per frame (set_viewmatrix() does shortcut)

        /* 5. render hierarchy - proximity */
        if (p->doEvents)
		{
			profile_start("hier_prox");
			render_hier(rootNode(), VF_Proximity);
			profile_end("hier_prox");
#ifdef DJTRACK_PICKSENSORS
			{
				/* find pickingSensors, record their world transform and picktargets */
				save_viewpoint2world();
				render_hier(rootNode(), VF_PickingSensor | VF_Other);
				if( enabled_picksensors() )
				{
					/* find picktargets, transform to world and do pick test and save results */
					render_hier(rootNode(), VF_inPickableGroup | VF_Other );
					/* record results of picks to picksensor node fields and event outs*/
					do_pickSensors();
				}
			}
#endif
		}

		//drawStatusBar();
		PRINT_GL_ERROR_IF_ANY("GLBackend::render_pre");
}
ivec2 ivec2_init(int x, int y);
int pointinsideviewport(ivec4 vp, ivec2 pt);
static int setup_pickside(int x, int y){
	/* Oct 2015 idea: change which stereo side the pickray is working on, 
	   based on which stereo side the mouse is in
	   - only makes a difference for updown and sidebyside
	   - analgyph and quadbuffer use the whole screen, so can use either
	   -- there's now an explicit userPrefferedPickSide (versus always using right)
	*/
	int sideleft, sideright, iside, userPreferredPickSide, ieither;
	ivec4 vportleft, vportright, vport, vportscene;
	ivec2 pt;
	Stack *vportstack;
	X3D_Viewer *viewer;

	ttglobal tg = gglobal();
	viewer = Viewer();
	userPreferredPickSide = viewer->dominantEye; //0= left, 1= right
	ieither = viewer->eitherDominantEye;

	//pt = ivec2_init(x,tg->display.screenHeight - y);
	pt = ivec2_init(x,y);
	vportstack = (Stack*)tg->Mainloop._vportstack;
	vport = stack_top(ivec4,vportstack); //should be same as stack bottom, only one on stack here
	vportscene = vport;
	vportscene.Y = vport.Y + tg->Mainloop.clipPlane;
	vportscene.H = vport.H - tg->Mainloop.clipPlane;

	vportleft = vportscene;
	vportright = vportscene;
	if(viewer->isStereo)
	{
		if (viewer->sidebyside){
			vportleft.W /= 2;
			vportright.W /=2;
			vportright.X = vportleft.X + vportleft.W;
		}
		if(viewer->updown) { //overunder
			vportright.H /= 2;
			vportleft.H /=2;
			//vportright.Y = vportleft.Y + vportright.H;
			vportleft.Y = vportright.Y + vportright.H;
		}
		//analgyph and quadbuffer use full window
	}
	sideleft = sideright=0;
	sideleft = pointinsideviewport(vportleft,pt);
	sideright = pointinsideviewport(vportright,pt);;
	if(sideleft && sideright) 
		iside = userPreferredPickSide; //analgyph, quadbuffer
	else 
		iside = sideleft? 0 : sideright ? 1 : 0;
	if(!ieither) iside = userPreferredPickSide;
	Viewer()->iside = iside;
	return sideleft || sideright; //if the mouse is outside graphics window, stop tracking it
}

void setup_projection()
{
	GLDOUBLE fieldofview2;
	GLint xvp;
	GLint scissorxl,scissorxr;
	Stack *vportstack;
	ivec4 vport;
	ppMainloop p;
	X3D_Viewer *viewer;
	ttglobal tg = gglobal();
	GLsizei screenwidth2; // = tg->display.screenWidth;
	GLsizei screenheight, bottom, top;
	static int counter = 0;
	GLDOUBLE aspect2; // = tg->display.screenRatio;
	p = (ppMainloop)tg->Mainloop.prv;
	viewer = Viewer();
	vportstack = (Stack*)tg->Mainloop._vportstack;
	vport = stack_top(ivec4,vportstack); //should be same as stack bottom, only one on stack here

	screenwidth2 = vport.W; //tg->display.screenWidth
	xvp = vport.X;
	top = vport.Y + vport.H; //or .H - .Y?  CHANGE OF MEANING used to be 0 at top of screen, now its more like screenHeight
	bottom = vport.Y + tg->Mainloop.clipPlane;
	screenheight = top - bottom; //tg->display.screenHeight - bottom;
	PRINT_GL_ERROR_IF_ANY("XEvents::start of setup_projection");

	scissorxl = xvp;
	scissorxr = xvp + screenwidth2;
	fieldofview2 = viewer->fieldofview;

	aspect2 = (double)(scissorxr - scissorxl)/(double)(screenheight);

	if(viewer->type==VIEWER_SPHERICAL)
		fieldofview2*=viewer->fovZoom;
	if(viewer->isStereo)
	{
		GLint xl,xr;
		xl = xvp;
		xr = xvp + screenwidth2;

		if (viewer->sidebyside){
			GLint iexpand;
			bool expand;
			double expansion;
			//its just sidebyside that needs the screen distance adjusted to be slightly less than human eyebase
			//(the others can center their viewpoints in the viewports, and center the viewports on the screen)
			//assume: the viewpoint is centered in the viewport
			//there are 2 viewports, one for left and one for right
			//so if you want to spread the screen eyebase out,
			//you need to expand the viewport(s) horizontally by 2x
			// in the direction you want it to move
			//for example to move the left viewpoint left, you expand the left viewport
			//on the left side by 2x (and move the right side of the right viewport to the right)
			//to move the left viewpoint right, move the right side of the left viewport
			//to the right by 2x.
			//except in sidebyside, that would cause an over-write in the middle, and changes
			//to aspect2 ratio can change the field of view
			//so for sidebyside, we make the viewports normal screenwidth2 wide and
			//use scissor test to crop to the viewports
			expand = viewer->screendist > .5f;
			expansion = viewer->screendist - .5;
			expansion = fabs(expansion);
			iexpand = (GLint)(expansion * screenwidth2);

			xr -= screenwidth2/4;
			xl -= screenwidth2/4;
			scissorxr = xvp + screenwidth2/2;
			if(viewer->iside ==1)
			{
				xl += screenwidth2/2;
				xr += screenwidth2/2;
				scissorxl += screenwidth2/2;
				scissorxr += screenwidth2/2;
			}
			if(expand)
			{
				if(viewer->iside ==1)
					xr = xr + iexpand;
				else
					xl = xl - iexpand;
			}else{
				if(viewer->iside ==1)
					xl = xl - iexpand;
				else
					xr = xr + iexpand;
			}

		}
		if(viewer->updown) //overunder
		{
			//if there's statusabarHud statusbar to be drawn, reserve space in both viewports
			screenheight = vport.H; // tg->display.screenHeight;
			screenheight /= 2;
			if (viewer->iside == 0){
				bottom += screenheight;
			}else{
				top -= screenheight; //+=
			}
			screenheight -= tg->Mainloop.clipPlane;
			scissorxl = xl;
			scissorxr = xr;
			counter++;
			if(counter == 100)
				printf("in setup_projection\n");

		}
		aspect2 = (double)(xr - xl)/(double)(screenheight);
		xvp = xl;
		screenwidth2 = xr-xl;
	}

	FW_GL_MATRIX_MODE(GL_PROJECTION);
	/* >>> statusbar hud */
	if(tg->Mainloop.clipPlane != 0 || viewer->updown || viewer->sidebyside)
	{   
		/* scissor used to prevent mainloop from glClear()ing the wrong stereo side, and the statusbar area
		 which is updated only every 10-25 loops */
		//FW_GL_SCISSOR(0,tg->Mainloop.clipPlane,tg->display.screenWidth,tg->display.screenHeight);
		FW_GL_SCISSOR(scissorxl,bottom,scissorxr-scissorxl,screenheight);
		glEnable(GL_SCISSOR_TEST);
	}
	/* <<< statusbar hud */
	// side-by-side eyebase fiducials (see fiducialDraw())
	p->viewpointScreenX[viewer->iside] = xvp + screenwidth2/2;
	p->viewpointScreenY[viewer->iside] = top; //yup now //tg->display.screenHeight - top; //fiducial draw still using top-down Y
	if (viewer->updown){
        FW_GL_VIEWPORT(xvp - screenwidth2 / 2, bottom, screenwidth2 * 2, screenheight);
    }
    else{
        FW_GL_VIEWPORT(xvp, bottom, screenwidth2, screenheight);
    }
	FW_GL_LOAD_IDENTITY();

	/* ortho projection or perspective projection? */
	if (Viewer()->ortho) {
		double minX, maxX, minY, maxY;
		double numerator;

		minX = viewer->orthoField[0];
		minY = viewer->orthoField[1];
		maxX = viewer->orthoField[2];
		maxY = viewer->orthoField[3];

		if (tg->display.screenHeight != 0) {
			numerator = (maxY - minY) * ((float) tg->display.screenWidth) / ((float) tg->display.screenHeight);
			maxX = numerator/2.0f;
			minX = -(numerator/2.0f);
		}

		FW_GL_ORTHO (minX, maxX, minY, maxY,
			viewer->nearPlane,viewer->farPlane);

	} else {
		/* bounds check */
		if ((fieldofview2 <= 0.0) || (fieldofview2 > 180.0))
			fieldofview2=45.0;
		/* glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);  */
        //printf ("Before FW_GLU_PERSPECTIVE, np %f fp %f\n",viewer->nearPlane, viewer->farPlane);
		FW_GLU_PERSPECTIVE(fieldofview2, aspect2, viewer->nearPlane,viewer->farPlane);
	}
	FW_GL_MATRIX_MODE(GL_MODELVIEW);
	PRINT_GL_ERROR_IF_ANY("XEvents::setup_projection");

}

void setup_pickray(int x, int y)
{
	ttglobal tg = gglobal();
	if(tg->RenderFuncs.usingAffinePickmatrix){
		//feature-AFFINE_GLU_UNPROJECT
		//NEW WAY: leaves proj matrix as normal, and creates a separate affine PICKMATRIX that when multiplied with modelview,
		// will point down the pickray (see above for OLD WAY)
		// method: uproject 2 points along the ray, one on nearside of frustum (window z = 0) 
		//	one on farside of frustum (window z = 1)
		// then the first one is A, second one is B
		// create a translation matrix to get from 0,0,0 to A T
		// create a rotation matrix R to get from A toward B
		// pickmatrix = R * T
		double mvident[16], pickMatrix[16], pmi[16], proj[16], R1[16], R2[16], R3[16], T[16];
		int viewport[4];
		double A[3], B[3], C[3], a[3], b[3];
		double yaw, pitch, yy,xx;
		loadIdentityMatrix(mvident);
		FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, proj);
		FW_GL_GETINTEGERV(GL_VIEWPORT,viewport);
		//yy = (float)viewport[3]  -y + bottom +top;
		//glu_unproject will subtract the viewport from the x,y, if they're all in y-up screen coords
		//yy = (float)(tg->display.screenHeight - y); //y-up - bottom
		yy = (float)y; //yup
		xx = (float)x;
		//printf("vp = %d %d %d %d\n",viewport[0],viewport[1],viewport[2],viewport[3]);
		//printf("yy %lf vp3 %d y %d vp1 %d sh %d\n",
		//	yy, viewport[3], y, viewport[1], tg->display.screenHeight);
		//nearside point
		a[0] = xx; a[1] = yy;  a[2] = 0.0;
		FW_GLU_UNPROJECT(a[0], a[1], a[2], mvident, proj, viewport,
				&A[0],&A[1],&A[2]);
		mattranslate(T,A[0],A[1],A[2]);
		//farside point
		b[0] = xx; b[1] = yy;  b[2] = 1.0;
		FW_GLU_UNPROJECT(b[0], b[1], b[2], mvident, proj, viewport,
				&B[0],&B[1],&B[2]);
		vecdifd(C,B,A);
		vecnormald(C,C);
		if(0) printf("Cdif %f %f %f\n",C[0],C[1],C[2]);
		//if(1){
		//	double hypotenuse = sqrt(C[0]*C[0] + C[2]*C[2]);
		//	yaw = asin(C[0]/hypotenuse); 
		//	hypotenuse = sqrt(C[1]*C[1] + C[2]*C[2]);
		//	pitch = asin(C[1]/hypotenuse); 
		//	if(1) printf("asin yaw=%f pitch=%f\n",yaw,pitch);
		//}
		yaw = atan2(C[0],-C[2]);
		matrixFromAxisAngle4d(R1, -yaw, 0.0, 1.0, 0.0);
		if(1){
			transformAFFINEd(C,C,R1);
			if(0) printf("Yawed Cdif %f %f %f\n",C[0],C[1],C[2]);
			pitch = atan2(C[1],-C[2]);
		}else{
			double hypotenuse = sqrt(C[0]*C[0] + C[2]*C[2]);
			pitch = atan2(C[1],hypotenuse);
		}
		if(0) printf("atan2 yaw=%f pitch=%f\n",yaw,pitch);

		pitch = -pitch;
		if(0) printf("[yaw=%f pitch=%f\n",yaw,pitch);
		if(0){
			matrotate(R1, -pitch, 1.0, 0.0, 0.0);
			matrotate(R2, -yaw, 0.0, 1.0, 0.0);
		}else{
			matrixFromAxisAngle4d(R1, pitch, 1.0, 0.0, 0.0);
			if(0) printmatrix2(R1,"pure R1");
			matrixFromAxisAngle4d(R2, yaw, 0.0, 1.0, 0.0);
			if(0) printmatrix2(R2,"pure R2");
		}
		matmultiplyAFFINE(R3,R1,R2);
		if(0) printmatrix2(R3,"R3=R1*R2");
		if(1){
			matmultiplyAFFINE(pickMatrix,R3, T); 
			matinverseAFFINE(pmi,pickMatrix);
			//matinverseFULL(pmi,pickMatrix); //don't need extra FLOPS 
		}else{
			//direct hacking of matrix, can save a few FLOPs
			R3[12] = A[0]; 
			R3[13] = A[1]; 
			R3[14] = A[2];
			matcopy(pickMatrix,R3);
			matinverseAFFINE(pmi,pickMatrix); //,R3);
			if(0)printmatrix2(R3,"R3[12]=A");
		}
		if(0) printmatrix2(pmi,"inverted");
		setPickrayMatrix(0,pickMatrix); //using pickmatrix in upd_ray and get_hyper
		setPickrayMatrix(1,pmi); //if using pickmatrix_inverse in upd_ray and get_hyper
		if(0){
			//Test: transform A,B and they should come out 0,0,x
			double rA[3], rB[3];
			transformAFFINEd(rA,A,pmi);
			transformAFFINEd(rB,B,pmi);
			printf(" A %f %f %f  B %f %f %f \n",A[0],A[1],A[2],B[0],B[1],B[2]);
			printf("rA %f %f %f rB %f %f %f \n",rA[0],rA[1],rA[2],rB[0],rB[1],rB[2]);
		}
	}
}


/* Render the scene */
static void render()
{
	int count;
	static double shuttertime;
	static int shutterside;

	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

	setup_projection();
	setup_picking();

	for (count = 0; count < p->maxbuffers; count++) {

		Viewer()->buffer = (unsigned)p->bufferarray[count];
		Viewer()->iside = count;

		/*  turn lights off, and clear buffer bits*/
		if(Viewer()->isStereo)
		{

			if(Viewer()->shutterGlasses == 2) /* flutter mode - like --shutter but no GL_STEREO so alternates */
			{
				if(TickTime() - shuttertime > 2.0)
				{
					shuttertime = TickTime();
					if(shutterside > 0) shutterside = 0;
					else shutterside = 1;
				}
				if(count != shutterside) continue;
			}
			if(Viewer()->anaglyph)
			{
				//set the channels for backbuffer clearing
				if(count == 0)
					Viewer_anaglyph_clearSides(); //clear all channels
				else
					Viewer_anaglyph_setSide(count); //clear just the channels we're going to draw to
			}
			setup_projection(); //scissor test in here
			BackEndClearBuffer(2);
			if(Viewer()->anaglyph)
				Viewer_anaglyph_setSide(count); //set the channels for scenegraph drawing
			//setup_viewpoint();
			set_viewmatrix();
		}
		else
			BackEndClearBuffer(2);
		//BackEndLightsOff();
		clearLightTable();//turns all lights off- will turn them on for VF_globalLight and scope-wise for non-global in VF_geom


		/*  turn light #0 off only if it is not a headlight.*/
		if (!fwl_get_headlight()) {
			setLightState(HEADLIGHT_LIGHT,FALSE);
			setLightType(HEADLIGHT_LIGHT,2); // DirectionalLight
		}

		/*  Other lights*/
		PRINT_GL_ERROR_IF_ANY("XEvents::render, before render_hier");

		render_hier(rootNode(), VF_globalLight);
		PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_globalLight)");

		/*  4. Nodes (not the blended ones)*/
		profile_start("hier_geom");
		render_hier(rootNode(), VF_Geom);
		profile_end("hier_geom");
		PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_Geom)");

		/*  5. Blended Nodes*/
		if (tg->RenderFuncs.have_transparency) {
			/*  render the blended nodes*/
			render_hier(rootNode(), VF_Geom | VF_Blend);
			PRINT_GL_ERROR_IF_ANY("XEvents::render, render_hier(VF_Geom)");
		}

		if (Viewer()->isStereo) {
#ifndef DISABLER
			if (Viewer()->sidebyside){
				//cursorDraw(1, p->viewpointScreenX[count], p->viewpointScreenY[count], 0.0f); //draw a fiducial mark where centre of viewpoint is
				fiducialDraw(1,p->viewpointScreenX[count],p->viewpointScreenY[count],0.0f); //draw a fiducial mark where centre of viewpoint is
			}
#endif
			if (Viewer()->anaglyph)
				glColorMask(1,1,1,1); /*restore, for statusbarHud etc*/
		}
		glDisable(GL_SCISSOR_TEST);
	} /* for loop */


	if(p->EMULATE_MULTITOUCH) {
		int i, screenWidth, screenHeight;
		fwl_getWindowSize(&screenWidth,&screenHeight);
		for(i=0;i<p->ntouch;i++){
			if(p->touchlist[i].ID > -1)
				if(p->touchlist[i].windex == p->windex)
				{
					int x,y;
					x = (int)(p->touchlist[i].fx * (float)screenWidth);
					y = (int)(p->touchlist[i].fy * (float)screenHeight);
					//printf("i %d windex %d x %d y %d ID %d\n",i,p->windex,x,y,p->touchlist[i].ID);
					cursorDraw(p->touchlist[i].ID,x,y,p->touchlist[i].angle);
				}
		}
    }
}

static int currentViewerLandPort = 0;
static int rotatingCCW = FALSE;
static double currentViewerAngle = 0.0;
static double requestedViewerAngle = 0.0;

static void setup_viewpoint() {
/*
	 Computes view part of modelview matrix and leaves it in modelview.
	 You would call this before traversing the scenegraph to scene nodes
	  with render() or render_hier().
	 The view part includes:
	 a) screen orientation ie on mobile devices landscape vs portrait (this function)
	 b) stereovision +- 1/2 base offset (viewer_togl)
	 c) viewpoint slerping interpolation (viewer_togl)
	 d) .Pos and .Quat of viewpoint from: (viewer_togl)
	        1) .position and .orientation specified in scenefile
	        2) cumulative navigation away from initial bound pose
	        3) gravity and collision (bumping and wall penetration) adjustments
	 e) transform stack between scene root and currently bound viewpoint (render_hier(rootnode,VF_Viewpoint))

*/
	int isStereo, iside;
	double viewmatrix[16];
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

	FW_GL_MATRIX_MODE(GL_MODELVIEW); /*  this should be assumed , here for safety.*/
	FW_GL_LOAD_IDENTITY();

	// has a change happened?
	if (Viewer()->screenOrientation != currentViewerLandPort) {
		// 4 possible values; 0, 90, 180, 270
		//
		rotatingCCW = FALSE; // assume, unless told otherwise
		switch (currentViewerLandPort) {
			case 0: {
				rotatingCCW= (Viewer()->screenOrientation == 270);
				break;
			}
			case 90: {
				rotatingCCW = (Viewer()->screenOrientation == 0);
				break;
			}
			case 180: {
				rotatingCCW = (Viewer()->screenOrientation != 270);
				break;
			}
			case 270: {
				rotatingCCW = (Viewer()->screenOrientation != 0);
				break;
			}
		}
		currentViewerLandPort = Viewer()->screenOrientation;
		requestedViewerAngle = (double)Viewer()->screenOrientation;
	}

	if (!(APPROX(currentViewerAngle,requestedViewerAngle))) {
		if (rotatingCCW) {
			//printf ("ccw, cva %lf req %lf\n",currentViewerAngle, requestedViewerAngle);
			currentViewerAngle -= 10.0;
			if (currentViewerAngle < -5.0) currentViewerAngle = 360.0;
		} else {
			//printf ("cw, cva %lf req %lf\n",currentViewerAngle, requestedViewerAngle);
			currentViewerAngle +=10.0;
			if (currentViewerAngle > 365.0) currentViewerAngle = 0.0;
		}
	}
	FW_GL_ROTATE_D (currentViewerAngle,0.0,0.0,1.0);

		fw_glGetDoublev(GL_MODELVIEW_MATRIX, p->screenorientationmatrix);
		isStereo = Viewer()->isStereo;
		iside = Viewer()->iside;
		Viewer()->isStereo = 1;
		Viewer()->iside = 0;
		FW_GL_LOAD_IDENTITY();
		set_stereo_offset0();
		fw_glGetDoublev(GL_MODELVIEW_MATRIX, p->stereooffsetmatrix[0]);
		Viewer()->iside = 1;
		FW_GL_LOAD_IDENTITY();
		set_stereo_offset0();
		fw_glGetDoublev(GL_MODELVIEW_MATRIX, p->stereooffsetmatrix[1]);
		Viewer()->isStereo = 0;
		FW_GL_LOAD_IDENTITY();

	viewer_togl(Viewer()->fieldofview);
		fw_glGetDoublev(GL_MODELVIEW_MATRIX, p->posorimatrix);
		FW_GL_LOAD_IDENTITY();

	profile_start("vp_hier");
	render_hier(rootNode(), VF_Viewpoint);
	profile_end("vp_hier");
	PRINT_GL_ERROR_IF_ANY("XEvents::setup_viewpoint");

		Viewer()->isStereo = isStereo;
		Viewer()->iside = iside;
		fw_glGetDoublev(GL_MODELVIEW_MATRIX, p->viewtransformmatrix);
		matcopy(viewmatrix,p->screenorientationmatrix);
		if(isStereo)
			matmultiplyAFFINE(viewmatrix,p->stereooffsetmatrix[iside],viewmatrix);
		matmultiplyAFFINE(viewmatrix,p->posorimatrix,viewmatrix); 
		matmultiplyAFFINE(viewmatrix,p->viewtransformmatrix,viewmatrix); 
		fw_glSetDoublev(GL_MODELVIEW_MATRIX, viewmatrix);
		slerp_viewpoint(); //just starting block
		fw_glGetDoublev(GL_MODELVIEW_MATRIX, p->viewtransformmatrix);

}

static void set_viewmatrix() {
	//if we already computed view matrix earlier in the frame via setup_viewpoint,
	//and theoretically it hasn't changed since, 
	//and just want to make sure its set, this is shorter than re-doing setup_viewpoint()
	double viewmatrix[16];
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

	FW_GL_MATRIX_MODE(GL_MODELVIEW); /*  this should be assumed , here for safety.*/

	matcopy(viewmatrix,p->screenorientationmatrix);
	if(Viewer()->isStereo)
		matmultiplyAFFINE(viewmatrix,p->stereooffsetmatrix[Viewer()->iside],viewmatrix);
	matmultiplyAFFINE(viewmatrix,p->posorimatrix,viewmatrix); 
	matmultiplyAFFINE(viewmatrix,p->viewtransformmatrix,viewmatrix); 
	fw_glSetDoublev(GL_MODELVIEW_MATRIX, viewmatrix);
}

char *nameLogFileFolderNORMAL(char *logfilename, int size){
	strcat(logfilename,"freewrl_tmp");
	fw_mkdir(logfilename);
	strcat(logfilename,"/");
	strcat(logfilename,"logfile");
	return logfilename;
}
char * (*nameLogFileFolderPTR)(char *logfilename, int size) = nameLogFileFolderNORMAL;

void toggleLogfile()
{
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;
	if(p->logging){
		fclose(p->logfile);
		//fclose(p->logerr);
		p->logging = 0;
#ifdef _MSC_VER
		freopen("CON","w",stdout);
#else
		//JAS - this does nothing, correct?
		// freopen("/dev/tty", "w", stdout);
#endif
		//save p->logfname for reopening
		printf("logging off\n");
	}else{
		char *mode = "a+";
		if(p->logfname == NULL){
			char logfilename[1000];
			mode = "w";
			logfilename[0] = '\0';
			nameLogFileFolderPTR(logfilename, 1000);
			strcat(logfilename,".log");
			p->logfname = STRDUP(logfilename);
		}
		printf("logging to %s\n",p->logfname);
		p->logfile = freopen(p->logfname, mode, stdout );
		//p->logerr = freopen(p->logfname, mode, stderr );
		p->logging = 1;
	}
}
#if defined(_MSC_VER)
#define strncasecmp _strnicmp
#endif
void fwl_set_logfile(char *lname){
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;
	if (strncasecmp(lname, "-", 1) == 0) {
	    printf("FreeWRL: output to stdout/stderr\n");
	} else {
		p->logfname = STRDUP(lname);
		toggleLogfile();
	}
}

int unload_broto(struct X3D_Proto* node);
struct X3D_Proto *hasContext(struct X3D_Node* node);
void fwl_clearWorld(){
	//clear the scene to empty (and do cleanup on old scene);
	int done = 0;
	ttglobal tg = gglobal();
	if(usingBrotos()){
		struct X3D_Node *rn = rootNode();
		if(hasContext(rn)){
			unload_broto(X3D_PROTO(rn));
			printf("unloaded scene as broto\n");
			done = 1;
		}
	}
	if(!done){
		tg->Mainloop.replaceWorldRequest = NULL;
		tg->threads.flushing = 1;
	}
	return;
}

void sendKeyToKeySensor(const char key, int upDown);
/* handle a keypress. "man freewrl" shows all the recognized keypresses */


#define KEYDOWN 2
#define KEYUP 3
#ifdef AQUA
#define KEYPRESS 2
#define isAQUA 1
#else
#define KEYPRESS 1
#define isAQUA 0
#endif
char lookup_fly_key(int key);
//#endif
void dump_scenegraph(int method);
void fwl_do_keyPress0(int key, int type) {
	int lkp;
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

	/* does this X3D file have a KeyDevice node? if so, send it to it */
	//printf("fwl_do_keyPress: %c%d\n",kp,type);
	if(key == 27 && type == 1)
	{
		//ESC key to toggle back to freewrl command use of keyboard
		p->keySensorMode = 1 - p->keySensorMode; //toggle
	}
	if (p->keySensorMode && KeySensorNodePresent()) {
		sendKeyToKeySensor(key,type); //some keysensor test files show no opengl graphics, so we need a logfile
	} else {
		int handled = isAQUA;
		if(p->keywait){
			if(type == KEYPRESS){
				//key,value commands
				//example: hit spacebar, then at the : prompt type keychord,yawz so it looks on the console:
				//:keychord,yawz
				//then press enter. Then if you use the arrow keys <> should turn left right, and ^v should go back/forth
				//here's a little hack so you can set any (pre-programmed) value from the keyboard in freewrl
				//by specifying key,value pair
				//to get the commandline, hit spacebar
				//then type the key, then the value, then hit Enter.
				//don't make mistakes typing - there's no backspace handling yet
				int len = strlen(p->keywaitstring);
				lkp = key;
				len = min(24,len); //dimensioned to 25
				if(lkp == '\r'){
					fwl_commandline(p->keywaitstring);
					p->keywait = FALSE;
					p->keywaitstring[0] = '\0';
					ConsoleMessage("%c",'\n');
				}else{
					ConsoleMessage("%c",lkp);
					if(lkp == '\b' && len){
						p->keywaitstring[len-1] = '\0';
					}else{
						p->keywaitstring[len] = lkp;
						p->keywaitstring[len+1] = '\0';
					}
				}
			}
			handled = TRUE;
			return;
		}

		if(type == KEYPRESS)
		{
			lkp = key;
			//normal key
			//if(kp>='A' && kp <='Z') lkp = tolower(kp);
			switch (lkp) {
				case 'n': {  fwl_clearWorld(); break; }
				case 'e': { fwl_set_viewer_type (VIEWER_EXAMINE); break; }
				case 'w': { fwl_set_viewer_type (VIEWER_WALK); break; }
				case 'd': { fwl_set_viewer_type (VIEWER_FLY); break; }
				case 'f': { fwl_set_viewer_type (VIEWER_EXFLY); break; }
				case 'y': { fwl_set_viewer_type (VIEWER_SPHERICAL); break; }
				case 't': { fwl_set_viewer_type(VIEWER_TURNTABLE); break; }
				case 'm': { fwl_set_viewer_type(VIEWER_LOOKAT); break; }
				case 'g': { fwl_set_viewer_type(VIEWER_EXPLORE); break; }
				case 'h': { fwl_toggle_headlight(); break; }
				case '/': { print_viewer(); break; }
				//case '\\': { dump_scenegraph(); break; }
				case '\\': { dump_scenegraph(1); break; }
				case '|': { dump_scenegraph(2); break; }
				case '=': { dump_scenegraph(3); break; }
				case '+': { dump_scenegraph(4); break; }
				case '-': { dump_scenegraph(5); break; }
				case '`': { toggleLogfile(); break; }
				case '$': resource_tree_dump(0, (resource_item_t*)tg->resources.root_res); break;
				case '*': resource_tree_list_files(0, (resource_item_t*)tg->resources.root_res); break;
				case 'q': { if (!RUNNINGASPLUGIN) {
							fwl_doQuit();
							}
							break;
						}
				case 'c': { toggle_collision(); break;}
				case 'v': {fwl_Next_ViewPoint(); break;}
				case 'b': {fwl_Prev_ViewPoint(); break;}
				case '.': {profile_print_all(); break;}
				case ' ': p->keywait = TRUE; ConsoleMessage("\n%c",':'); p->keywaitstring[0] = '\0'; break;

#if !defined(FRONTEND_DOES_SNAPSHOTS)
				case 's': {fwl_toggleSnapshot(); break;}
				case 'x': {Snapshot(); break;} /* thanks to luis dias mas dec16,09 */
#endif //FRONTEND_DOES_SNAPSHOTS

				default:
					printf("didn't handle key=[%c][%d] type=%d\n",lkp,(int)lkp,type);
					handled = 0;
					break;
			}
		}
		if(!handled) {
			char kp;
			if(type/10 == 0){
				kp = (char)key; //normal keyboard key
			}else{
				kp = lookup_fly_key(key); //actionKey possibly numpad or arrows, convert to a/z
				if(!kp){
					//not a fly key - is it SHIFT or CTRL?  //feature-EXPLORE
					int keystate = type % 10 == KEYDOWN ? 1 : 0;
					switch(key){
						case CTL_KEY:
							tg->Mainloop.CTRL = keystate; break;
						case SFT_KEY:
							tg->Mainloop.SHIFT = keystate; break;
						default:
						break;
					}
					//printf("CTRL=%d SHIFT=%d\n",tg->Mainloop.CTRL,tg->Mainloop.SHIFT);
				}
			}
			if(kp){
				if(tg->Mainloop.SHIFT){
					if(type%10 == KEYDOWN && (key == LEFT_KEY || key == RIGHT_KEY)){
						int ichord;
						//shift arrow left or right changes keychord
						ichord = viewer_getKeyChord();
						if(key == LEFT_KEY) ichord--;
						if(key == RIGHT_KEY) ichord++;
						viewer_setKeyChord(ichord);
					}
				}else{
					double keytime = Time1970sec();
					if(type%10 == KEYDOWN)
						handle_key(kp,keytime);  //keydown for fly
					if(type%10 == KEYUP)
						handle_keyrelease(kp,keytime); //keyup for fly
				}
			}
		}
	}
}
int fwl_getShift(){
	ttglobal tg = gglobal();
	return tg->Mainloop.SHIFT;
}
void fwl_setShift(int ishift){
	ttglobal tg = gglobal();
	tg->Mainloop.SHIFT = ishift;
}

int fwl_getCtrl(){
	ttglobal tg = gglobal();
	return tg->Mainloop.CTRL;
}


int platform2web3dActionKey(int platformKey);

void (*fwl_do_rawKeyPressPTR)(int key, int type) = fwl_do_keyPress0;
void fwl_do_rawKeyPress(int key, int type) {
	fwl_do_rawKeyPressPTR(key,type);
}

void fwl_do_keyPress(char kp, int type) {
	//call this from AQUA, ANDROID, QNX, IPHONE as always
	//it will do the old-style action-key lookup
	//(desktop win32 and linux can get finer tuning on the keyboard
	// with per-platform platform2web3dActionKeyLINUX and WIN32 functions
	int actionKey=0;
	int key = (int) kp;
	if (type != KEYPRESS) //May 2014 added this if (I think just the raw keys would need virtual key lookup, I have a problem with '.')
		actionKey = platform2web3dActionKey(key);
	if(actionKey)
		fwl_do_rawKeyPress(actionKey,type+10);
	else
		fwl_do_rawKeyPress(key,type);
}


/* go to a viewpoint, hopefully it is one that is in our current list */
void fwl_gotoViewpoint (char *findThisOne) {
	int i;
	int whichnode = -1;
	struct tProdCon *t = &gglobal()->ProdCon;

	/* did we have a "#viewpoint" here? */
	if (findThisOne != NULL) {
		for (i=0; i<vectorSize(t->viewpointNodes); i++) {
			switch ((vector_get(struct X3D_Node*, t->viewpointNodes,i)->_nodeType)) {
				case NODE_Viewpoint:
					if (strcmp(findThisOne,
						X3D_VIEWPOINT(vector_get(struct X3D_Node *,t->viewpointNodes,i))->description->strptr) == 0) {
						whichnode = i;
					}
					break;


				case NODE_GeoViewpoint:
					if (strcmp(findThisOne,
						X3D_GEOVIEWPOINT(vector_get(struct X3D_Node *,t->viewpointNodes,i))->description->strptr) == 0) {
						whichnode = i;
					}
					break;

				case NODE_OrthoViewpoint:
					if (strcmp(findThisOne,
						X3D_ORTHOVIEWPOINT(vector_get(struct X3D_Node *,t->viewpointNodes,i))->description->strptr) == 0) {
						whichnode = i;
					}
					break;


			}
		}


		/* were we successful at finding this one? */
		if (whichnode != -1) {
			/* set the initial viewpoint for this file */
			t->setViewpointBindInRender = vector_get(struct X3D_Node *,t->viewpointNodes,whichnode);
		}
    	}
}

void setup_viewpoint_slerp(double *center, double pivot_radius, double vp_radius);

int getRayHitAndSetLookatTarget() {
	/* called from mainloop for LOOKAT navigation:
		- take mousexy and treat it like a pickray, similar to, or borrowing VF_Sensitive code
		- get the closest shape node* along the pickray and its modelview matrix (similar to sensitive, except all and only shape nodes)
		- get the center and size of the picked shape node, and send the viewpoint to it
		- return to normal navigation
	*/
    double pivot_radius, vp_radius; //x,y,z,
    int i;
	//ppMainloop p;
	ttglobal tg = gglobal();
	//p = (ppMainloop)tg->Mainloop.prv;

    if(tg->RenderFuncs.hitPointDist >= 0) {
		struct X3D_Node * node;
		struct currayhit * rh = (struct currayhit *)tg->RenderFuncs.rayHit;

        /* is the sensitive node not NULL? */
        if (rh->hitNode == NULL) {
			Viewer()->LookatMode = 0; //give up, turn off lookat cursor
		}else{
			//GLDOUBLE matTarget[16];
			double center[3], radius; //pos[3], 
			vp_radius = 10.0;
			if(Viewer()->type == VIEWER_LOOKAT){
				//use the center of the object, and its radius
				GLDOUBLE smin[3], smax[3], shapeMBBmin[3], shapeMBBmax[3];
				double viewerdist;
				//double dradius, pos[3], distance;
				node = rh->hitNode;
				for(i=0;i<3;i++)
				{
					shapeMBBmin[i] = node->_extent[i*2 + 1];
					shapeMBBmax[i] = node->_extent[i*2];
				}
				transformMBB(smin,smax,rh->modelMatrix,shapeMBBmin,shapeMBBmax); //transform shape's MBB into eye space
				radius = 0.0;
				for(i=0;i<3;i++){
					center[i] = (smax[i] + smin[i])*.5;
					radius = max(radius,(max(fabs(smax[i]-center[i]),fabs(smin[i]-center[i]))));
				}
				viewerdist = Viewer()->Dist;
				vp_radius = max(viewerdist, radius + 5.0);
				//distance = veclengthd(center);
				//distance = (distance - dradius)/distance;
				//radius = distance;
				pivot_radius = 0.0;
				//vp_radius = dradius;

			} else if(Viewer()->type == VIEWER_EXPLORE){
				//use the pickpoint (think of a large, continuous geospatial terrain shape,
				// and you want to examine a specific geographic point on that shape)
				pointxyz2double(center,tg->RenderFuncs.hp);
				transformAFFINEd(center,center,getPickrayMatrix(0));
				pivot_radius = 0.0;
				vp_radius = .8 * veclengthd(center);
			}
			Viewer()->LookatMode = 3; //go to viewpiont transition mode
			setup_viewpoint_slerp(center,pivot_radius,vp_radius);
		}
    }
    return Viewer()->LookatMode;
}

struct X3D_Node* getRayHit() {
        double x,y,z;
        int i;
		ppMainloop p;
		ttglobal tg = gglobal();
		p = (ppMainloop)tg->Mainloop.prv;

        if(tg->RenderFuncs.hitPointDist >= 0) {
			struct currayhit * rh = (struct currayhit *)tg->RenderFuncs.rayHit;
			if (rh->hitNode == NULL) return NULL;  //this prevents unnecessary matrix inversion non-singularity

			if(!tg->RenderFuncs.usingAffinePickmatrix){
				struct point_XYZ *hp = (struct point_XYZ*)tg->RenderFuncs.hp;
				FW_GLU_UNPROJECT(hp->x,hp->y,hp->z,rh->modelMatrix,rh->projMatrix,viewport,&x,&y,&z);
			}
			if(tg->RenderFuncs.usingAffinePickmatrix){
				GLDOUBLE mvp[16], mvpi[16];
				GLDOUBLE *pickMatrix = getPickrayMatrix(0);
				GLDOUBLE *pickMatrixi = getPickrayMatrix(1);
				//struct point_XYZ r11 = {0.0,0.0,1.0}; //note viewpoint/avatar Z=1 behind the viewer, to match the glu_unproject method WinZ = -1
				struct point_XYZ tp; //note viewpoint/avatar Z=1 behind the viewer, to match the glu_unproject method WinZ = -1

				if(0){
					//pickMatrix is inverted in setup_pickray
					matmultiplyAFFINE(mvp,rh->modelMatrix,pickMatrixi);
					matinverseAFFINE(mvpi,mvp);
				}else{
					//pickMatrix is not inverted in setup_pickray
					double mvi[16];
					matinverseAFFINE(mvi,rh->modelMatrix);
					matmultiplyAFFINE(mvpi,pickMatrix,mvi);
				}
		
				transform(&tp,tg->RenderFuncs.hp,mvpi);
				x = tp.x; y = tp.y, z = tp.z;
			}
            /* and save this globally */
            tg->RenderFuncs.ray_save_posn[0] = (float) x; tg->RenderFuncs.ray_save_posn[1] = (float) y; tg->RenderFuncs.ray_save_posn[2] = (float) z;

            /* we POSSIBLY are over a sensitive node - lets go through the sensitive list, and see
                if it exists */

            /* is the sensitive node not NULL? */
            if (rh->hitNode == NULL) return NULL;


			/*
            printf ("rayhit, we are over a node, have node %p (%s), posn %lf %lf %lf",
				rh->hitNode, stringNodeType(rh->hitNode->_nodeType), x, y, z);
			printf(" dist %f \n", rh->hitNode->_dist);
			*/


            for (i=0; i<p->num_SensorEvents; i++) {
                    if (p->SensorEvents[i].fromnode == rh->hitNode) {
                            /* printf ("found this node to be sensitive - returning %u\n",rayHit.hitNode); */
                            return ((struct X3D_Node*) rh->hitNode);
                    }
            }
        }

        /* no rayhit, or, node was "close" (scenegraph-wise) to a sensitive node, but is not one itself */
        return(NULL);
}


/* set a node to be sensitive, and record info for this node */
void setSensitive(struct X3D_Node *parentNode, struct X3D_Node *datanode) {
        void (*myp)(unsigned *);
	int i;
		ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;

        switch (datanode->_nodeType) {
                /* sibling sensitive nodes - we have a parent node, and we use it! */
                case NODE_TouchSensor: myp = (void *)do_TouchSensor; break;
                case NODE_GeoTouchSensor: myp = (void *)do_GeoTouchSensor; break;
				case NODE_LineSensor: myp = (void *)do_LineSensor; break;
                case NODE_PlaneSensor: myp = (void *)do_PlaneSensor; break;
                case NODE_CylinderSensor: myp = (void *)do_CylinderSensor; break;
                case NODE_SphereSensor: myp = (void *)do_SphereSensor; break;
                case NODE_ProximitySensor: /* it is time sensitive only, NOT render sensitive */ return; break;
                case NODE_GeoProximitySensor: /* it is time sensitive only, NOT render sensitive */ return; break;

                /* Anchor is a special case, as it has children, so this is the "parent" node. */
                case NODE_Anchor: myp = (void *)do_Anchor; parentNode = datanode; break;
                default: return;
        }
        /* printf ("setSensitive parentNode %p  type %s data %p type %s\n",parentNode,
                        stringNodeType(parentNode->_nodeType),datanode,stringNodeType (datanode->_nodeType)); */

	/* is this node already here? */
	/* why would it be duplicate? When we parse, we add children to a temp group, then we
	   pass things over to a rootNode; we could possibly have this duplicated */
	for (i=0; i<p->num_SensorEvents; i++) {
		if ((p->SensorEvents[i].fromnode == parentNode) &&
		    (p->SensorEvents[i].datanode == datanode) &&
		    (p->SensorEvents[i].interpptr == (void *)myp)) {
			/* printf ("setSensitive, duplicate, returning\n"); */
			return;
		}
	}

        if (datanode == 0) {
                printf ("setSensitive: datastructure is zero for type %s\n",stringNodeType(datanode->_nodeType));
                return;
        }

        /* record this sensor event for clicking purposes */
        p->SensorEvents = REALLOC(p->SensorEvents,sizeof (struct SensStruct) * (p->num_SensorEvents+1));

        /* now, put the function pointer and data pointer into the structure entry */
        p->SensorEvents[p->num_SensorEvents].fromnode = parentNode;
        p->SensorEvents[p->num_SensorEvents].datanode = datanode;
        p->SensorEvents[p->num_SensorEvents].interpptr = (void *)myp;

        /* printf ("saved it in num_SensorEvents %d\n",p->num_SensorEvents);  */
        p->num_SensorEvents++;
}

/* we have a sensor event changed, look up event and do it */
/* note, (Geo)ProximitySensor events are handled during tick, as they are time-sensitive only */
static void sendSensorEvents(struct X3D_Node* COS,int ev, int butStatus, int status) {
        int count;
		int butStatus2;
		ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;

        /* if we are not calling a valid node, dont do anything! */
        if (COS==NULL) return;

        for (count = 0; count < p->num_SensorEvents; count++) {
                if (p->SensorEvents[count].fromnode == COS) {
						butStatus2 = butStatus;
                        /* should we set/use hypersensitive mode? */
                        if (ev==ButtonPress) {
                                gglobal()->RenderFuncs.hypersensitive = p->SensorEvents[count].fromnode;
                                gglobal()->RenderFuncs.hyperhit = 0;
                        } else if (ev==ButtonRelease) {
                                gglobal()->RenderFuncs.hypersensitive = 0;
                                gglobal()->RenderFuncs.hyperhit = 0;
								butStatus2 = 1;
                        } else if (ev==MotionNotify) {
                                get_hyperhit();
                        }


                        p->SensorEvents[count].interpptr(p->SensorEvents[count].datanode, ev,butStatus2, status);
                        /* return; do not do this, incase more than 1 node uses this, eg,
                                an Anchor with a child of TouchSensor */
                }
        }
}

/* POINTING DEVICE SENSOR CONMPONENT
http://www.web3d.org/files/specifications/19775-1/V3.3/Part01/components/pointingsensor.html
- this describes how the pointing device sensors:
 TouchSensor, DragSensors: CylinderSensor, SphereSensor, PlaneSensor, [LineSensor]
 work as seen by the user:
 - dragsensor - the parent's descendent geometry is used only to identify and activate the dragsensor node. 
	After that it's the dragsensor's geometry (Cylinder/Sphere/Plane/Line) (not the parent's descendent geometry) 
	that's intersected with the pointing device bearing/ray to generate trackpoint_changed 
	and translation_ or rotation_changed events in sensor-local coordinates
- touchsensor - generates events in touchsensor-local coordinates

Terminology ([] denotes alternative design not yet implemented April 2014):
<node>-local - coordinates relative to a given node
modelview matrix - transforms coords from the current node-local into the current viewpoint-local
proj matrix - projects points from viewpoint-local 3D to 2D normalized screen viewport (with Z in 0 to 1 range)
pick-proj matrix - special projection matrix that aligns the mousexy (pickpoint) on the viewport center
	- formed in setup_pickray(pick=TRUE,,)
view matrix - the view part of the modelview matrix, generated during setup_viewpoint()
model matrix - the model part of the world modelview matrix, generated during
	traversing the scenegraph in render_node()
world - coordinates at the root node level ie with no transforms applied
	viewport-local -[proj] - viewpoint-local - [view] - world - [model] - node-local
modelview matrix: combined model * view matrices
place - position in the scenegraph transform hierarchy (with DEF/USE, a node can be in multiple places)
geometry_model: the model part of the modelview matrix at a geometry node's place
sensor_model: the model part of the modelview matrix at a sensor node's place
bearing: 2 points (A,B) defining a ray in 3D space going from A in the direction of B
	1. For 2D screen-based pointing devices	like mouse, A is the viewpoint position 0,0,0 
		in viewpoint-local, and for B mousexy is converted in setup_pickray(pick=TRUE,,) 
		to a special pick-proj matrix that centers the viewport on the mousexy, so B is 0,0,-1 in
			pick-viewport-local coordinates	(glu_unproject produces pick-viewport-local)
		[using normal projection matrix for the bearing, then B= mousex,mousey,-1 in 
		normal viewport-local coordinates, and glu_unproject is used to get this bearing
		into world coordinates for the remainder of PointingDeviceSensor activity]
	[2.for a 3D pointing device, it would have its own A,B in world. And then that world bearing 
		can be transformed into geometry-local and sensor-local and intersections with 
		shape-geometry and sensor-geometry calculated. dug9 Sept 2, 2014: FLY keyboard is 6DOF/3D, and not in world]
hit: an intersection between the bearing and geometry that is closer to A (supercedes previous cloest)

As seen by the developer, here's how I (dug9 Apr 2014) think they should work internally, on each frame 
 0. during parsing, when adding a parent to a node, check the node type and if a sensor type, 
		create a SensorEvent (by calling setSensitive(node,parent)) and set the SensorEvent[i].datanode=sensor
		and SensorEvent[i].fromNode = parent
 1. in startofloopnodeupdates() for each sensor node, flag its immediate Group/Transform parent 
	with VF_Sensitive [the sensor's ID]	so when traversing the scenegraph, this 'sensitivity' 
	can be pushed onto a state stack to affect descendent geometry nodes
 2. from the scene root, traverse to the viewpoint to get the view part of the modelview matrix
	-Set up 2 points in viewpoint space: A=0,0,0 (the viewpoint) and B=(mouse x, mouse y, z=-1) to represent the bearing
	-apply a special pick-projection matrix, in setup_pickray(pick=TRUE,,), so that glu_unproject 
		is with respect to the mouse-xy bearing B point being at the center of the viewport 
		[use normal projection, and minimize the use of glu_unproject by transforming viewport-local bearing
		to world during setup, and use world for bearing for all subsequent PointingDeviceSensor activities
		dug9 Sept 2014: or keep in avatar coords, and set up a avatar2pickray affine transform]
 3. from the scene root, in in render_hier(VF_Sensitive) pass, tranverse the scenegraph looking for sensitive 
	group/transform parent nodes, accumulating the model part of the modelview matrix as normal.
	When we find a sensitive parent: 
	a) save the current best-hit parent-node* to a C local variable, and set the current best node* to the 
		current parent	[push the parent's sensor node ID onto a sensor_stack]
	b) save the current hit modelview matrix to a C local variable, and set it to the parent's modelview matrix 
		[snapshot the current modelview matrix, and push onto a sensor_model stack]
	When we find descendent geometry to a sensitive (grand)parent:
	a) use glu_unproject with the pick-proj and current modelview matrix to re-transform the bearing 
		into geometry-local space [invert the cumulative model matrix, and use it to transform 
		world bearing (A,B) into geometry-local]
		-so we have a bearing (A',B') in geometry-local (this requires fewer math ops than 
		transforming all geometry vertices into pick-viewport-local [bearing-world])
	b) intersect the infinite line passing through A' and B' 
		with the geometry to get a list of intersection points n,xyz[n] in geometry-local space, or for 
		well-known geometry sphere,box,cylinder,rectangle2D... compute and test intersections one by one
	c) for each intersection point:
		- filter the intersection point to elliminate if in the -B direction from A (behind the viewpoint)
		- transform using modelview and pick-proj matrix into pick-viewport-local coordinates 
		  [transform using model into bearing-world coordinates]
		- compare the remaining intersection point by distance from A, and choose it over 
			the current one if closer to A (the other is 'occluded') to produce a new hit
	d) if there was a new hit, record some details needed by the particular type of sensor node, (but don't
		generate events yet - we have to search for even closer hits):
		- sensor's parent (Group/Transform) node* [+ Sensor node* ]
		- sensor's parent modelview and pick-proj matrices [sensor's parent model matrix]
		- hit point on decendent geometry surface, in pick-viewport-local [sensor-local]
			(the geometry node may be in descendant transform groups from the sensor's parent, so needs to be 
			transformed up to the Sensor-local system at some point, using 
			the sensor's modelview and pick-proj matrices [sensor_model matrix] we saved)
		- TouchSensor: 
			a) texture coordinates at the hitpoint
			b) normal to surface at hitpoint [transformed into sensor-local system]
  4. once finished searching for sensor nodes, and back in mainloop at the root level, if there 
		was a hit, and the hit is on sensitive geometry, call a function to look up the sensor node* given
		the parent node* in SensorEvents array, and generate events from the sensor node

What's not shown above: 
6. what happens when 2+ sensor nodes apply to the same geometry:
	for example, parent [ sensor1, group [ sensor 2, shape ] ]
	the specs say it's the sensor that's the nearest co-descendent (sibling or higher) to the geometry [sensor2,shape]
	(it doesn't say what happens in a tie ie 2 different sensor nodes are siblings) [sensor1,sensor2,shape]
	(it doesn't say what happens if you DEF a sensornode and USE it in various places)
7. what happens on each frame during a Drag:
	the successful dragsensor node is 'chosen' on the mouse-down
	on mouse-move: intersections with other occluding geometry, and geometry sensitive to other sensor nodes, are ignored, 
	- but intersection with the same dragsensor's geometry are updated for trackpoint_changed eventout
	- but because typically you move the geometry you are dragging, it and the dragsensor are 
		moved on each drag/mousemove, so you can't intersect the bearing with the updated sensor geometry position
		because that would produce net zero movement. Rather you need to 'freeze' the pose of the dragsensor geometry in the scene
		on mousdown, then on each mousemove/drag frame, intersect the new pointer bearing with 
		the original/frozen/mousedown dragsensor geometry pose. Then on mouse-up you unfreeze so next mousedown 
		you are starting fresh on the translated sensor geometry.
8. where things are stored
	The details for the hit are stored in a global hitCursor struct [struct array]
	render_node() has some C local variables which get pushed onto the C call stack when recursing
	[render_node() uses an explicit stack for sensor_model and sensor_parent_node* to apply to descendent geometry]
9. any special conditions for touch devices: none [multiple bearings processed on the same pass, for multi-touch, 
	PlaneSensor.rotation_changed, PlaneSensor.scale_changed, LineSensor.scale_changed events added to specs]

More implemnetation details - these may change, a snapshot April 2014
Keywords:
	Sensitive - all pointingdevice sensors
	HyperSensitive - drag sensors in the middle of a drag
Storage types:
struct currayhit {
	struct X3D_Node *hitNode; // What node hit at that distance? 
	- It's the parent Group or Transform to the Sensor node (the sensor node* gets looked up from parent node*
		in SensorEvents[] array, later in sendSensorEvents())
	GLDOUBLE modelMatrix[16]; // What the matrices were at that node
	- a snapshot of modelview at the sensornode or more precisely it's immediate parent Group or Transform
	- it's the whole modelview matrix
	GLDOUBLE projMatrix[16]; 
	- snapshot of the pick-projection matrix at the same spot
	-- it will include the pick-specific projection matrix aligned to the mousexy in setup_pickray(pick=TRUE,,)
};
global variables:
	struct point_XYZ r1 = {0,0,-1},r2 = {0,0,0},r3 = {0,1,0}; 
		pick-viewport-local axes: r1- along pick-proj axis, r2 viewpoint, r3 y-up axis in case needed
	hyp_save_posn, t_r2 - A - (viewpoint 0,0,0 transformed by modelviewMatrix.inverse() to geometry-local space)
	hyp_save_norm, t_r1 - B - bearing point (viewport 0,0,-1 used with pick-proj bearing-specific projection matrix)
		- norm is not a direction vector, its a point. To get a direction vector: v = (B - A) = (norm - posn)
	ray_save_posn - intersection with scene geometry, in sensor-local coordinates 
		- used in do_CyclinderSensor, do_SphereSensor for computing a radius  on mouse-down
	t_r3 - viewport y-up in case needed
call stacks:
resource thread > parsing > setParent > setSensitive
mainloop > setup_projection > glu_pick
mainloop > render_hier(VF_Sensitive) > render_Node() > upd_ray(), (node)->rendray_<geom> > rayhit()
mainloop > sendSensorEvent > get_hyperHit(), .interptr()={do_TouchSensor / do_<Drag>Sensor /..}
funcion-specific variables:
rayhit()
	Renderfuncs.hp.xyz - current closest hit, in bearing-local system
	RenderFuncs.hitPointDist - current distance to closest hit from viewpoint 0,0,0 to geometry intersection (in viewpoint scale)
	rayHit - snapshot of sensor's modelview matrix to go with closest-hit-so-far
	rayHitHyper - "
render_node(): - a few variables acting as a stack by using automatic/local C variables in recursive calling
	int srg - saves current renderstate.render_geom
	int sch - saves current count of the hits from a single geometry node before trying to to intersect another geometry
	struct currayhit *srh - saves the current best hit rayph on the call stack
	rayph.modelMatrix- sensor-local snapshot of modelview matrix 
	rayph.viewMatrix - " proj "
		-- this could have been a stack, with a push for each direct parent of a pointingdevice sensor
SIBLING_SENSITIVE(node) - macro that checks if a node is VF_Sensitive and if so, sets up a parent vector
	n->_renderFlags = n->_renderFlags  | VF_Sensitive; - for each parent n, sets the parent sensitive
		(which indirectly sensitizes the siblings)
geometry nodes have virt->rendray_<shapenodetype>() functions called unconditionally on VF_Sensitive pass
	- ie rendray_Box, rendray_Sphere, rendray_Circle2D
	- called whether or not they are sensitized, to find an intersection and determine if they a) occlude 
			or b) succeed other sensitized geometry along the bearing
upd_ray() - keeps bearing A,B transformed to geometry-local A',B' - called after a push or pop to the modelview matrix stack.
rayhit() - accepts intersection of A',B' line with geometry in geometry-local space, 
	- filters if behind viewpoint,
	- sees if it's closer to A' than current, if so 
		- transforms hit xyz from geometry-local into bearing-local (pick-viewport-local) [model] and stores as hp.xyz
		- snapshots the sensor-local modelview and stores as .rayHit, for possible use in get_hyperhit()
get_hyperhit() for DragSensors: transforms the current-frame bearing from bearing-local 
		into the sensor-local coordinate system of the winning sensor, in a generic way that works 
		for all the DragSensor types in their do_<Drag>Sensor function. 
		- ray_save_posn, hyp_save_posn, hyp_save_norm
sendSensorEvents(node,event,button,status) - called from mainloop if there was a any pointing sensor hits
	- looks up the sensor node* from the parent node*
	- calls the do_<sensortype> function ie do_PlaneSensor(,,,)
do_TouchSensor, do_<dragSensorType>: do_CylinderSensor, do_SphereSensor, do_PlaneSensor, [do_LineSensor not in specs, fw exclusive]
	- compute output events and update any carryover sensor state variables
		- these are called from mainloop ie mainloop > sendSensorEvents > do_PlaneSensor
		- because its not called from the scenegraph where the SensorNode is, and because 
			the Sensor evenouts need to be in sensor-local coordinates, the inbound variables
			need to already be in sensor-local coordinates ie posn,norm are in sensor-local


What I think we could do better:
1. world bearing coords:
	Benefits of converting mousexy into world bearing(A,B) once per frame before sensitive pass:
	a) 3D PointingDevices generate bearing(A,B) in world coordinates
		dug9 Aug31, 2014: not really. FLY keyboard is 6DOF/3D, and works in current-viewpoint space, 
			adding relative motions on each tick. If I had a 3D pointing device, I would use it the same way
	b) separate PickingComponent (not yet fully implemented) works in world coordinates
	- so these two can share code if PointingDevice bearing from mouse is in world
	c) minimizes the use of glu_unproject (a compound convenience function with expensive 
		matrix inverse)
		dug9 Aug31, 2014: to do a ray_intersect_geometry you need to transform 2 points into local geometry
			-and that requires a matrix inverse- or you need to transform all the geometry points into
			a stable pickray coordinate system (like avatar collision) - requiring the transform of  lots of points
			IDEA: when compile_transform do the inverse of the transform too - 2 matrix stacks 
			- then for VF_Sensitive you'd matrix multiply down both stacks, use the inverse for transforming
				pick ray points into local, and use modelview (or model if you want global) to transform
				the near-side intersection back into stable pickray coordinates for eventual comparative 
				distance sorting
			- Q. would this be faster than inverting modelview (or model) at each shape (to transform 2 ray points to
				local) or transforming all shape to viewpoint space to intersect there (like collision)? I ask because
				I'm thinking of implementing web3d.org LOOKAT navigation type, which allows you to click on any shape
				(and then your viewpoint transitions to EXAMINE distance near that shape). Unlike Sensitive -which
				sensitize a few nodes- LOOKAT would allow any shape to be picked, so testing (transforming or inverting)
				needs to be done at more shapes. For LOOKAT I'll use the collision approach (transform all shape
				points to avatar/viewpoint space) and it already has a linesegment(A,B)_intersect_shape, 
				with distance sorting for wall penetration detection and correction.
	How:
	after setup_viewpoint and before render_hier(VF_Sensitive), transform the mouse bearing (A,B) 
	from viewpoint to world coordinates using the view matrix. 
	dug9 Aug31, 2014: keep in mind points along the pickray need to be sorted along the pickray 
			(all but the closest are occluded), and that might be simpler math in viewpoint coordinates (distance=z)
			versus world coordinate (A,B) which requires a dot product distance=(C-A)dot(B-A)
2. normal proj matrix (vs glu_pick modified proj matrix)
	Benefits of using normal proj matrix for PointingDeviceSensor:
	a) allows multiple bearings to be processed on the same pass for future multi-touch devices (no 
		competition for the bearing-specific pick-proj matrix)
	b) allows VF_Sensitive pass to be combined with other passes for optimization, if using
		the same transform matrices
	c) allows setup_viewpoint(), setup_pickray() and setup_projection() to be computed once for multiple passes
	d) allows using world coordinates for the bearing because then bearing doesn't depend 
		on a special proj matrix which only glu_project and glu_unproject can make use of,
		and those functions transform to/from viewport space
	How:
	in setup_projection(pick=FALSE,,), and glu_unproject with the mousexy to get B in 
	viewpoint-local, then use view to transform A,B from viewpoint-local to world. View is 
	computed with setup_viewpoint()
	
	dug9 Aug31 2014: extent/Minimum Bounding Boxes (MBB) - if you transform the object shape MBB
		directly into pickray-aligned coordinates in one step with pickray-aligned modelview, then exclusion can be done
		with simple x < max type culling tests. Then once that test gets a hit you can do more expensive
		operations. That's the benefit of pickray-aligned (modified) modelview: faster culling. Otherwise using
		ordinary modelview or model, you have to do another matrix multiply on 8 points to get that MBB into
		pickray-aligned space, or do a more general ray-intersect-unaligned-box which has more math.
	dug9 Sept 2, 2014: if GLU_PROJECT, GLU_UNPROJECT are being used now to transform, then there is already
		a matrix concatonation: modelview and projection. So substituting a pickray-aligned affine
		matrix for the proj matrix won't add matrix multiplies, and since the combined would now be 
		affine, further ops can also be affine, reducing FLOPs. 

3. explicit sensor stack:
	Benefits of an explicit sensor stack in render_node():
	a) code will be easier to read (vs. current 'shuffling' on recusion)
	b) minimizes C call stack size and minimizes memory fragmentation from malloc/free
		by using a pre-allocaed (or infrequently realloced) heap stack
	How: 
	when visiting a VF_sensitive parent (Group/Transform), push parent's modelview [model] transform
	onto a special sensor_viewmodel [sensor_model] stack that works like modelview matrix push and pop stack,
	and push the parents node* onto a sensor_parent stack (and pop them both going back)
3. minimize use of glu_unproject
	glu_unproject is a compound convenience function doing an expensive 4x4 matrix inversion
	Benefits of avoiding glu_unproject:
	a) allows speed optimization by computing inverse once, and using it many times for a given
		modelview [model] matrix
	b) avoids reliance on any projection matrix
	dug9 Sept 2, 2014: 
	  c) without projection matrix, affine matrix ops can be used which cut FLOPs in half
	How:
	a) implement: 
		FW_GL_GETDOUBLEV(GL_MODEL_MATRIX_INVERSE, modelMatrix);
		FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX_INVSERSE, modelviewMatrix);
		and during modelview_push and _pop clear a state flag. Then when we call the above
		we would test if !flag invert & set flag, else: re-use
	b) for bearing, use world coordinates, which elliminates the dependency on pick-bearing-specific projmatrix
		so we don't rely on glu_project/glu_unproject which are the only functions that can use the projMatrix.

	we need the following transform capabilities:
	a) bearing: transform A,B bearing from PointingDevice-local to world. If pointing device is:
		- 2D mouse use pick-proj [normal proj] and view matrix
		- 3D, leave in world coords, or do some uniform scale to world
	b) sensor hits: 
		- transform bearing from bearing-local (pick-viewport-local [world]) to geometry-local
		- transform geometry-local hit xyz and surface normal vector to sensor-local 
			for generating events, DragSensor: for intersecting with sensor geometry,
				using modelview+pick-proj [using model]
	c) DragSensor: snapshot/freeze the Sensor_model transform on mousedown 
		for use during a drag/mousemove
	d) Neither/Non-Sensitized geom nodes: transform geometry xyz hits to bearing-local
		using modelview+pick-proj [model]
	[viewport coordinates are only needed once per frame to project the mouse into bearing-world]

Update May 2015 - dug9
	- we've been using AffinePickMatrix method since fall 2014, 8 months, and with LOOKAT and EXPLORE navigation modes
		and its been working fine.
	- clarifications of some points above:
		- we do our ray-geometry intersections in geometry-local coordinates. That requires us to 
			transform (a copy of) the pick ray into geometry-local as we move down the transform stack in render_node()
			we do that in upd_ray(), and it requires a matrix inverse. Currently we call upd_ray() during descent, and
			also during ascent in render_node(). It could be a stack instead, pushed on the way down and popped on the way back 
			to save an inverse. 
		- We recompute upd_ray() on each vf_sensitive recursion into render_node. But in theory it should/could be just when 
			we've chnaged the modelview transform by passing through a transform (or geotransform or group) node.
			I'm not sure the best place/way to detect that. Perhaps just before ->children(node).
					bool pushed_ray = false
					if(node.type == transform type) pushed_ray = true
					if(pushed_ray) upd_ray_and_push()
					node->children(node)
					if(pushed_ray) pop_ray()
		- then if/when we have an intersection/hit point, we transform it back into bearing-local space for comparison
			with the best-hit so far on the frame (which is closest/not occluded). This transform uses no inverse.
		- One might argue whether it would be easier / somehow better to transform all geometry points 
			into ray/bearing local space instead, requiring no inverse, and simplifying some of the intersection math.
			For drawing, this transform is done on the gpu. I don't know which is better. 
			OpenCL might help, transforming in parallel, analogous to drawing.
		- Or for each transform when compile_transform the 4x4 from translation, rotation, scale, also compute its inverse and
			store with the transform. Then when updating modelview during scengraph traversing, also multiply the inverses 
			to get inverse(modelview). 49 FLOPS in an AFFINE inverse (ie inverting cumulative modelview at each level), 
			vs. 36 in AFFINE matrix multiply. You save 13 FLOPS per transform level during VFSensitve pass, but need to
			pre-compute the inverses, likely once for most, in compile_transform.
		- we have to intersect _all_ geometry, not just sensitive. That's because non-sensitive geometry can occlude.
			we do an extent/MBB (minimum bounding box) intersection test first, but we need the upd_ray() with inverse to do that.
		- LOOKAT and EXPLORE use this intersecting all geometry to find the closest point of all geometry along the ray, 
			even when no sensitive nodes in the scene. They only do it on passes where someone has indicated they 
			want to pick a new location (for viewpoint slerping to) (versus sensitive nodes in scene which cause 
			us to always be checking for hits)
		- for dragsensor nodes, on mouse-down we want to save/snapshot the modelview matrix from viewpoint to sensor-local
			- then use this sensor-local transform to place the sensor-geometry ie cylinder, sphere, plane [,line]
				for intersecting as we move/drag with mousedown
			- since we don't know if or which dragsensor hit will be the winner (on a frame) when visiting a sensor,
				we save it if its the best-so-far and over-write with any subsequent better hits
		- if there are multiple sensors in the same scengraph branch, the one closest to the hit emits the events
			- there could/should be a stack for 'current sensor' pushed when descending, and popping on the way back
	- An alternative to a cluttered render_node() function would be to change virt->children(node) functions
		to accept a function pointer ie children(node,func). Same with normal_children(node,func).
		Then use separate render_node(node), sensor_node(node), and collision_node(node) functions. 
		They are done on separate passes now anyway.
		On the other hand, someone may find a way to combine passes for better efficiency/reduced transform FLOPs per frame.
	- Multitouch - there is currently nothing in the specs. But if there was, it might apply to (modfied / special) touch 
		and drag sensors. And for us that might mean simulataneously or iterating over a list of touches.
*/

/*	get_hyperhit()
	If we have successfully picked a DragSensor sensitive node, and we are on mousedown
	or mousemove(drag) events:
   - transform the bearing/pick-ray from bearing-local^  to sensor-local coordinates
   - in a way that is generic for all DragSensor nodes in their do_<Drag>Sensor function
   - so they can intersect the bearing/pick-ray with their sensor geometry (Cylinder,Sphere,Plane[,Line])
   - and emit events in sensor-local coordinates
   - ^bearing-local: currently == pick-viewport-local 
   -- unproject is used because to go from geometry-local to bearing-local, because
		it's convenient, and includes the pick-viewport in the transform - see setup_pickray(pick=TRUE,,) for details
	  But it may be overkill if bearing-local is made to == world, for compatibility with 3D pointing devices
*/
static void get_hyperhit() {
    double x1,y1,z1,x2,y2,z2,x3,y3,z3;
    GLDOUBLE projMatrix[16];
	struct currayhit *rhh, *rh;
	ttglobal tg = gglobal();
	rhh = (struct currayhit *)tg->RenderFuncs.rayHitHyper;
	rh = (struct currayhit *)tg->RenderFuncs.rayHit;

	/*
	printf ("hy %.2f %.2f %.2f, %.2f %.2f %.2f, %.2f %.2f %.2f\n",
		r1.x, r1.y, r1.z, r2.x, r2.y, r2.z, 
		tg->RenderFuncs.hp.x, tg->RenderFuncs.hp.y, tg->RenderFuncs.hp.z);
	*/

	if(!tg->RenderFuncs.usingAffinePickmatrix){
		struct point_XYZ *hp = (struct point_XYZ *)tg->RenderFuncs.hp;
		//FLOPS 588 double: 3x glu_unproject 196
		FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projMatrix);
		//FLOPs 588 double: 3 x glu_unproject 196
		FW_GLU_UNPROJECT(r1.x, r1.y, r1.z, rhh->modelMatrix,
				projMatrix, viewport, &x1, &y1, &z1);
		FW_GLU_UNPROJECT(r2.x, r2.y, r2.z, rhh->modelMatrix,
				projMatrix, viewport, &x2, &y2, &z2);
		FW_GLU_UNPROJECT(hp->x, hp->y, hp->z, rh->modelMatrix,
				projMatrix,viewport, &x3, &y3, &z3);
		if(0) printf("OLD ");
	}
	if(tg->RenderFuncs.usingAffinePickmatrix){
		//feature-AFFINE_GLU_UNPROJECT
		//FLOPs	112 double:	matmultiplyAFFINE 36, matinverseAFFINE 49, transform (affine) 3x9 =27
		GLDOUBLE mvp[16], mvpi[16];
		GLDOUBLE *pickMatrix = getPickrayMatrix(0);
		GLDOUBLE *pickMatrixi = getPickrayMatrix(1);
		struct point_XYZ r11 = {0.0,0.0,1.0}; //note viewpoint/avatar Z=1 behind the viewer, to match the glu_unproject method WinZ = -1
		struct point_XYZ tp; //note viewpoint/avatar Z=1 behind the viewer, to match the glu_unproject method WinZ = -1

		if(0){
			//pickMatrix is inverted in setup_pickray
			matmultiplyAFFINE(mvp,rhh->modelMatrix,pickMatrixi);
			matinverseAFFINE(mvpi,mvp);
		}else{
			//pickMatrix is not inverted in setup_pickray
			double mvi[16];
			matinverseAFFINE(mvi,rhh->modelMatrix);
			matmultiplyAFFINE(mvpi,pickMatrix,mvi);
		}
		
		transform(&tp,&r11,mvpi);
		x1 = tp.x; y1 = tp.y; z1 = tp.z;
		transform(&tp,&r2,mvpi);
		x2 = tp.x; y2 = tp.y; z2 = tp.z;
		if(0){
			//pickMatrix is inverted in setup_pickray
			matmultiplyAFFINE(mvp,rh->modelMatrix,pickMatrix);
			matinverseAFFINE(mvpi,mvp);
		}else{
			//pickMatrix is not inverted in setup_pickray
			double mvi[16];
			matinverseAFFINE(mvi,rh->modelMatrix);
			matmultiplyAFFINE(mvpi,pickMatrix,mvi);
		}

		transform(&tp,tg->RenderFuncs.hp,mvpi);
		x3 = tp.x; y3 = tp.y; z3 = tp.z;
		if(0) printf("NEW ");
	}
	
    if(0) printf ("get_hyper %f %f %f, %f %f %f, %f %f %f\n",
        x1,y1,z1,x2,y2,z2,x3,y3,z3); 
	
    /* and save this globally */
    tg->RenderFuncs.hyp_save_posn[0] = (float) x1; tg->RenderFuncs.hyp_save_posn[1] = (float) y1; tg->RenderFuncs.hyp_save_posn[2] = (float) z1;
    tg->RenderFuncs.hyp_save_norm[0] = (float) x2; tg->RenderFuncs.hyp_save_norm[1] = (float) y2; tg->RenderFuncs.hyp_save_norm[2] = (float) z2;
    tg->RenderFuncs.ray_save_posn[0] = (float) x3; tg->RenderFuncs.ray_save_posn[1] = (float) y3; tg->RenderFuncs.ray_save_posn[2] = (float) z3;
}

/* set stereo buffers, if required */
void setStereoBufferStyle(int itype) /*setXEventStereo()*/
{
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
	if(itype==0)
	{
		/* quad buffer crystal eyes style */
		p->bufferarray[0]=GL_BACK_LEFT;
		p->bufferarray[1]=GL_BACK_RIGHT;
		p->maxbuffers=2;
	}
	else if(itype==1)
	{
		/*sidebyside and anaglyph type*/
		p->bufferarray[0]=GL_BACK;
		p->bufferarray[1]=GL_BACK;
		p->maxbuffers=2;
	}
	printf("maxbuffers=%d\n",p->maxbuffers);
}

/* go to the first viewpoint */
/* ok, is this ViewpointGroup active or not? */
int vpGroupActive(struct X3D_ViewpointGroup *vp_parent) {

	/* ok, if this is not a ViewpointGroup, we are ok */
	if (vp_parent->_nodeType != NODE_ViewpointGroup) return TRUE;

	if (vp_parent->__proxNode != NULL) {
	        /* if size == 0,,0,0 we always do the render */
	        if ((APPROX(0.0,vp_parent->size.c[0])) && (APPROX(0.0,vp_parent->size.c[1])) && (APPROX(0.0,vp_parent->size.c[2]))) {
	                printf ("size is zero\n");
	                return TRUE;
	        }

		return X3D_PROXIMITYSENSOR(vp_parent->__proxNode)->isActive;
	}
	return TRUE;
}

/* find if there is another valid viewpoint */
static int moreThanOneValidViewpoint( void) {
	int count;
	struct tProdCon *t = &gglobal()->ProdCon;

	if (vectorSize(t->viewpointNodes)<=1) return FALSE;

	for (count=0; count < vectorSize(t->viewpointNodes); count++) {
		if (count != t->currboundvpno) {
			struct Vector *me = vector_get(struct X3D_Node*, t->viewpointNodes,count)->_parentVector;

			/* ok, we have a viewpoint; is its parent a ViewpointGroup? */
			if (me != NULL) {

			    if (vectorSize(me) > 0) {
				struct X3D_Node * vp_parent;

				POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, vector_get( struct X3D_Node *,
					vector_get(struct X3D_Node *,t->viewpointNodes,count)->_parentVector, 0),
					vp_parent);
				/* printf ("parent found, it is a %s\n",stringNodeType(vp_parent->_nodeType)); */

				/* sigh, find if the ViewpointGroup is active or not */
				return vpGroupActive((struct X3D_ViewpointGroup *)vp_parent);
			   }
			}
		}
	}
	return FALSE;
}


/* go to the last viewpoint */
void fwl_Last_ViewPoint() {
	if (moreThanOneValidViewpoint()) {

		int vp_to_go_to;
		int ind;
		struct tProdCon *t = &gglobal()->ProdCon;

		/* go to the next viewpoint. Possibly, quite possibly, we might
		   have to skip one or more if they are in a ViewpointGroup that is
		   out of proxy */
		vp_to_go_to = vectorSize(t->viewpointNodes);
		for (ind = 0; ind < vectorSize(t->viewpointNodes); ind--) {
			struct X3D_Node *cn;

			vp_to_go_to--;
                	if (vp_to_go_to<0) vp_to_go_to=vectorSize(t->viewpointNodes)-1;
			POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, vector_get(struct X3D_Node*, t->viewpointNodes,vp_to_go_to),cn);

			/* printf ("NVP, %d of %d, looking at %d\n",ind, totviewpointnodes,vp_to_go_to);
			printf ("looking at node :%s:\n",X3D_VIEWPOINT(cn)->description->strptr); */

			if (vpGroupActive((struct X3D_ViewpointGroup *) cn)) {
				if(0){
					/* whew, we have other vp nodes */
					send_bind_to(vector_get(struct X3D_Node*, t->viewpointNodes,t->currboundvpno),0);
					t->currboundvpno = vp_to_go_to;
					if (t->currboundvpno>=vectorSize(t->viewpointNodes)) t->currboundvpno=0;
					send_bind_to(vector_get(struct X3D_Node*, t->viewpointNodes,t->currboundvpno),1);

				}else{
					/* dug9 - using the display-thread-synchronous gotoViewpoint style
						to help order-senstive slerp_viewpoint() process */
					/* set the initial viewpoint for this file */
					t->setViewpointBindInRender = vector_get(struct X3D_Node*,
						t->viewpointNodes,vp_to_go_to);
					t->currboundvpno = vp_to_go_to;
					if (t->currboundvpno>=vectorSize(t->viewpointNodes)) t->currboundvpno=0;
				}
			return;
			}
		}
        }
}



/* go to the first viewpoint */
void fwl_First_ViewPoint() {
	if (moreThanOneValidViewpoint()) {

		int vp_to_go_to;
		int ind;
		struct tProdCon *t = &gglobal()->ProdCon;

		/* go to the next viewpoint. Possibly, quite possibly, we might
		   have to skip one or more if they are in a ViewpointGroup that is
		   out of proxy */
		vp_to_go_to = -1;
		for (ind = 0; ind < vectorSize(t->viewpointNodes); ind++) {
			struct X3D_Node *cn;

			vp_to_go_to++;
                	if (vp_to_go_to<0) vp_to_go_to=vectorSize(t->viewpointNodes)-1;
			POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, vector_get(
				struct X3D_Node* , t->viewpointNodes,vp_to_go_to),cn);

			/* printf ("NVP, %d of %d, looking at %d\n",ind, totviewpointnodes,vp_to_go_to);
			printf ("looking at node :%s:\n",X3D_VIEWPOINT(cn)->description->strptr); */

			if (vpGroupActive((struct X3D_ViewpointGroup *) cn)) {
				if(0){
                	/* whew, we have other vp nodes */
                	send_bind_to(vector_get(struct X3D_Node*,t->viewpointNodes,t->currboundvpno),0);
                	t->currboundvpno = vp_to_go_to;
                	if (t->currboundvpno>=vectorSize(t->viewpointNodes)) t->currboundvpno=0;
                	send_bind_to(vector_get(struct X3D_Node*,t->viewpointNodes,t->currboundvpno),1);
				}else{
					/* dug9 - using the display-thread-synchronous gotoViewpoint style
						to help order-senstive slerp_viewpoint() process */
					/* set the initial viewpoint for this file */
					t->setViewpointBindInRender = vector_get(struct X3D_Node*,t->viewpointNodes,vp_to_go_to);
                	t->currboundvpno = vp_to_go_to;
                	if (t->currboundvpno>=vectorSize(t->viewpointNodes)) t->currboundvpno=0;

				}

			return;
			}
		}
        }
}
/* go to the next viewpoint */
void fwl_Prev_ViewPoint() {
	if (moreThanOneValidViewpoint()) {

		int vp_to_go_to;
		int ind;
		struct tProdCon *t = &gglobal()->ProdCon;

		/* go to the next viewpoint. Possibly, quite possibly, we might
		   have to skip one or more if they are in a ViewpointGroup that is
		   out of proxy */
		vp_to_go_to = t->currboundvpno;
		for (ind = 0; ind < vectorSize(t->viewpointNodes); ind--) {
			struct X3D_Node *cn;

			vp_to_go_to--;
                	if (vp_to_go_to<0) vp_to_go_to=vectorSize(t->viewpointNodes)-1;
			POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, vector_get(struct X3D_Node*, t->viewpointNodes,vp_to_go_to),cn);

			/* printf ("NVP, %d of %d, looking at %d\n",ind, totviewpointnodes,vp_to_go_to);
			printf ("looking at node :%s:\n",X3D_VIEWPOINT(cn)->description->strptr); */

			if (vpGroupActive((struct X3D_ViewpointGroup *) cn)) {

				if(0){
                	/* whew, we have other vp nodes */
                	send_bind_to(vector_get(struct X3D_Node*,t->viewpointNodes,t->currboundvpno),0);
                	t->currboundvpno = vp_to_go_to;
                	if (t->currboundvpno>=vectorSize(t->viewpointNodes)) t->currboundvpno=0;
                	send_bind_to(vector_get(struct X3D_Node*,t->viewpointNodes,t->currboundvpno),1);
				}else{
					/* dug9 - using the display-thread-synchronous gotoViewpoint style
						to help order-senstive slerp_viewpoint() process */
					/* set the initial viewpoint for this file */
					t->setViewpointBindInRender = vector_get(struct X3D_Node*,
						t->viewpointNodes,vp_to_go_to);
                	t->currboundvpno = vp_to_go_to;
                	if (t->currboundvpno>=vectorSize(t->viewpointNodes)) t->currboundvpno=0;
				}


			return;
			}
		}
        }
}

/* go to the next viewpoint */
void fwl_Next_ViewPoint() {
	if (moreThanOneValidViewpoint()) {

		int vp_to_go_to;
		int ind;
		struct tProdCon *t = &gglobal()->ProdCon;

		/* go to the next viewpoint. Possibly, quite possibly, we might
		   have to skip one or more if they are in a ViewpointGroup that is
		   out of proxy */
		vp_to_go_to = t->currboundvpno;
		for (ind = 0; ind < vectorSize(t->viewpointNodes); ind++) {
			struct X3D_Node *cn;

			vp_to_go_to++;
                	if (vp_to_go_to>=vectorSize(t->viewpointNodes)) vp_to_go_to=0;
			POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, vector_get(
				struct X3D_Node*, t->viewpointNodes,vp_to_go_to),cn);

			/* printf ("NVP, %d of %d, looking at %d\n",ind, totviewpointnodes,vp_to_go_to);
			printf ("looking at node :%s:\n",X3D_VIEWPOINT(cn)->description->strptr); */

			if (vpGroupActive((struct X3D_ViewpointGroup *) cn)) {
                		/* whew, we have other vp nodes */
				/* dug9 - using the display-thread-synchronous gotoViewpoint style
					to help order-senstive slerp_viewpoint() process */
				/* set the initial viewpoint for this file */
				t->setViewpointBindInRender = vector_get(
					struct X3D_Node*,t->viewpointNodes,vp_to_go_to);
                		t->currboundvpno = vp_to_go_to;
                		if (t->currboundvpno>=vectorSize(t->viewpointNodes)) t->currboundvpno=0;

			return;
			}
		}
        }
}

/* initialization for the OpenGL render, event processing sequence. Should be done in threat that has the OpenGL context */
void fwl_initializeRenderSceneUpdateScene() {

#ifndef AQUA
	ttglobal tg = gglobal();
#endif

	/*
	ConsoleMessage("fwl_initializeRenderSceneUpdateScene start\n");
	if (rootNode()==NULL) {
		ConsoleMessage("fwl_initializeRenderSceneUpdateScene rootNode NULL\n");
	} else {
		ConsoleMessage("fwl_initializeRenderSceneUpdateScene rootNode %d children \n",rootNode()->children.n);
	}
	*/
	new_tessellation();
	fwl_set_viewer_type(VIEWER_EXAMINE);
	viewer_postGLinit_init();

#ifndef AQUA
	if( ((freewrl_params_t*)(tg->display.params))->fullscreen && newResetGeometry != NULL) newResetGeometry();
#endif

}

/* phases to shutdown:
- stop mainthread from rendering - someone presses 'q'. If we are in here, we aren't rendering
A. worker threads > tell them to flush and stop
B. check if both worker threads have stopped
- exit loop
C. delete instance data
- let the display thread die a peaceful death
*/


int workers_waiting(){
	BOOL waiting;
	ttglobal tg = gglobal();
	waiting = tg->threads.ResourceThreadWaiting && tg->threads.TextureThreadWaiting;
	return waiting;
}
void workers_stop()
{
	resitem_queue_exit();
	texitem_queue_exit();
}
int workers_running(){
	BOOL more;
	ttglobal tg = gglobal();
	more = tg->threads.ResourceThreadRunning || tg->threads.TextureThreadRunning;
	return more;
}

int isSceneLoaded()
{
	//have all the resoruces been loaded and parsed and the scene is stable?
	//some other web3d browseers have a way to tell, and you can delay rendering till all resources are loaded
	//freewrl -after 2014 rework by dug9- has been reworked to be 'lazy loading' meaning it might not 
	//request a resource until it visits a node that needs it, perhaps several times - see load_inline() (extern proto is similar)

	//need: in our case we want SSR (server-side rendering) to loop normally until the scene
	//is (lazy?) loaded and parsed and ready, then go into a render-one-frame-for-each-client-request mode
	//how do we tell? this may change if we have a more reliable cycle for resources.
	//for now we'll check if our worker threads are waiting, and frontend has no res items in its possession (not downloading one)
	//		p->doEvents = (!fwl_isinputThreadParsing()) && (!fwl_isTextureParsing()) && fwl_isInputThreadInitialized();

	int ret;
	double dtime;
	//ppProdCon p;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;
	//p = (ppProdCon) tg->ProdCon.prv;
	//ret = 0;
	//ret = workers_waiting() && !p->frontend_list_to_get;
	//ret = ret && tg->Mainloop.
	//printf("[%d %d %p]",tg->threads.ResourceThreadWaiting,tg->threads.TextureThreadWaiting,p->frontend_list_to_get);
	ret = (!fwl_isinputThreadParsing()) && (!fwl_isTextureParsing()) && fwl_isInputThreadInitialized();
	ret = ret && workers_waiting();
	//curtime = TickTime();
	dtime = tg->Mainloop.TickTime - p->BrowserInitTime;
	ret = ret && (dtime > 10.0); //wait 10 seconds
	return ret;
}

void end_of_run_tests(){
	//miscalaneous malloc, buffer, resource cleanup testing at end of run
	//press Enter on console after viewing results
	if(1){
		int i, notfreed, notfreedt;
		//see if there are any opengl buffers not freed
		notfreed = 0;
		notfreedt = 0;
		for(i=0;i<100000;i++){
			if(glIsBuffer(i)) {notfreed++; printf("b%d ",i);}
			if(glIsTexture(i)) {notfreedt++; printf("t%d ",i);}
		}
		printf("\ngl buffers not freed = %d\n",notfreed);
		printf("gl textures not freed = %d\n",notfreedt);
		getchar();
	}
}

void finalizeRenderSceneUpdateScene() {
	//C. delete instance data
	struct X3D_Node* rn;
	ttglobal tg = gglobal();
	printf ("finalizeRenderSceneUpdateScene\n");

	/* set geometry to normal size from fullscreen */
#ifndef AQUA
	if (newResetGeometry != NULL) newResetGeometry();
#endif
	/* kill any remaining children processes like sound processes or consoles */
	killErrantChildren();
	/* tested on win32 console program July9,2011 seems OK */
	rn = rootNode();
	if(rn)
		deleteVector(struct X3D_Node*,rn->_parentVector); //perhaps unlink first
	freeMallocedNodeFields(rn);
	FREE_IF_NZ(rn);
	setRootNode(NULL);
#ifdef DEBUG_MALLOC
	end_of_run_tests(); //with glew mx, we get the glew context from tg, so have to do the glIsBuffer, glIsTexture before deleting tg
#endif
	iglobal_destructor(tg);
#ifdef DEBUG_MALLOC
	void scanMallocTableOnQuit(void);
	scanMallocTableOnQuit();
#endif

}


int checkReplaceWorldRequest(){
	ttglobal tg = gglobal();
	if (tg->Mainloop.replaceWorldRequest || tg->Mainloop.replaceWorldRequestMulti){
		tg->threads.flushing = 1;
	}
	return tg->threads.flushing;
}
static int exitRequest = 0; //static because we want to exit the process, not just a freewrl instance (I think).
int checkExitRequest(){
	return exitRequest;
}

int checkQuitRequest(){
	ttglobal tg = gglobal();
	if (tg->threads.MainLoopQuit == 1){
		tg->threads.flushing = 1;
	}
	return tg->threads.MainLoopQuit;
}
void doReplaceWorldRequest()
{
	resource_item_t *res,*resm;
	char * req;

	ttglobal tg = gglobal();

	req = tg->Mainloop.replaceWorldRequest;
	tg->Mainloop.replaceWorldRequest = NULL;
	if (req){
		//kill_oldWorldB(__FILE__,__LINE__);
		res = resource_create_single(req);
		//send_resource_to_parser_async(res);
		resitem_enqueue(ml_new(res));
		FREE_IF_NZ(req);
	}
	resm = (resource_item_t *)tg->Mainloop.replaceWorldRequestMulti;
	if (resm){
		tg->Mainloop.replaceWorldRequestMulti = NULL;
		//kill_oldWorldB(__FILE__, __LINE__);
		resm->new_root = true;
		gglobal()->resources.root_res = (void*)resm;
		//send_resource_to_parser_async(resm);
		resitem_enqueue(ml_new(resm));
	}
	tg->threads.flushing = 0;
}
static int(*view_initialize)() = NULL;
static void(*view_update)() = NULL;
//
//EGL/GLES2 winGLES2.exe with KEEP_FV_INLIB sets frontend_handles_display_thread=true, 
// then calls fv_display_initialize() which only creates window in backend if false
#if defined(_ANDROID) || defined(WINRT)
int view_initialize0(void){
	/* Initialize display - View initialize*/
	if (!fv_display_initialize()) {
		ERROR_MSG("initFreeWRL: error in display initialization.\n");
		return FALSE; //exit(1);
	}
	return TRUE;
}
#else
int view_initialize0(void){
	/* Initialize display - View initialize*/
	if (!fv_display_initialize_desktop()) {
		ERROR_MSG("initFreeWRL: error in display initialization.\n");
		return FALSE; //exit(1);
	}
	return TRUE;
}
#endif //ANDROID


void killNodes();

/* fwl_draw() call from frontend when frontend_handles_display_thread */
int fwl_draw()
{
	int more;
	ppMainloop p;
	ttglobal tg = gglobal();
	fwl_setCurrentHandle(tg, __FILE__, __LINE__);
	p = (ppMainloop)tg->Mainloop.prv;

	more = TRUE; //FALSE;
	if (!p->draw_initialized){
		more = FALSE;
		view_initialize = view_initialize0; //defined above, with ifdefs
//		view_update = view_update0; //defined above with ifdefs
		if (view_initialize)
			more = view_initialize();

		if (more){
			fwl_initializeRenderSceneUpdateScene();  //Model initialize
		}
		p->draw_initialized = TRUE;
	}
	if(more) //more = TRUE;
	switch (tg->threads.MainLoopQuit){
	case 0:
	case 1:
		//PRINTF("event loop\n");
		switch (tg->threads.flushing)
		{
		case 0:
			profile_end("frontend");
			profile_start("mainloop");
			//model: udate yourself
			fwl_RenderSceneUpdateScene(); //Model update
			profile_end("mainloop");
			profile_start("frontend");

			//view: poll model and update yourself >>
			if (view_update) view_update();

			//if (!tg->display.params.frontend_handles_display_thread){
			//	/* swap the rendering area */
			//	FW_GL_SWAPBUFFERS;
			//}
			PRINT_GL_ERROR_IF_ANY("XEvents::render");
			checkReplaceWorldRequest(); //will set flushing=1
			checkQuitRequest(); //will set flushing=1
			break;
		case 1:
			if (workers_waiting()) //one way to tell if workers finished flushing is if their queues are empty, and they are not busy
			{
                //if (!tg->Mainloop.replaceWorldRequest || tg->threads.MainLoopQuit) //attn Disabler
				//kill_oldWorldB(__FILE__, __LINE__); //cleans up old scene while leaving gglobal intact ready to load new scene
				reset_Browser(); //rename
				tg->threads.flushing = 0;
				if (tg->threads.MainLoopQuit)
					tg->threads.MainLoopQuit++; //quiting takes priority over replacing
				else
					doReplaceWorldRequest();
			}
		}
		break;
	case 2:
		//tell worker threads to stop gracefully
		workers_stop();
		//killNodes(); //deallocates nodes MarkForDisposed
		//killed above kill_oldWorldB(__FILE__,__LINE__);
		tg->threads.MainLoopQuit++;
		break;
	case 3:
		//check if worker threads have exited
		more = workers_running();
        if (more == 0)
        {
			//moved from desktop.c for disabler
            finalizeRenderSceneUpdateScene();
        }
		break;
	}
	return more;
}


//#endif /* FRONTEND_HANDLES_DISPLAY_THREAD */


void fwl_setLastMouseEvent(int etype) {
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
	//printf ("fwl_setLastMouseEvent called\n");
        p->lastMouseEvent = etype;
}

void fwl_initialize_parser()
{
	/* create the root node */
	if (rootNode() == NULL) {
		if(usingBrotos())
			setRootNode( createNewX3DNode (NODE_Proto) );
		else
			setRootNode( createNewX3DNode (NODE_Group) );
		/*remove this node from the deleting list*/
		doNotRegisterThisNodeForDestroy(X3D_NODE(rootNode()));
	}
}

void fwl_init_SnapSeq() {
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */
        set_snapsequence(TRUE);
#endif
}


void fwl_set_LineWidth(float lwidth) {
        gglobal()->Mainloop.gl_linewidth = lwidth;
}

void fwl_set_KeyString(const char* kstring)
{
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
    p->keypress_string = STRDUP(kstring);
}



/* if we had an exit(EXIT_FAILURE) anywhere in this C code - it means
   a memory error. So... print out a standard message to the
   console. */
void outOfMemory(const char *msg) {
        ConsoleMessage ("FreeWRL has encountered a memory allocation problem\n"\
                        "and is exiting. -- %s--",msg);
        usleep(10 * 1000);
        exit(EXIT_FAILURE);
}

void _disposeThread(void *globalcontext);

/* quit key pressed, or Plugin sends SIGQUIT */
void fwl_doQuitInstance(void *tg_remote)
{
    ttglobal tg = gglobal();
    if (tg_remote == tg)
    {
        fwl_doQuit();
        fwl_draw();
        workers_stop();
        fwl_clearCurrentHandle();
#ifdef DISABLER
        pthread_create(&tg->threads.disposeThread, NULL, (void *(*)(void *))&_disposeThread, tg);
#endif
    }
}

void __iglobal_destructor(ttglobal tg);

void _disposeThread(void *globalcontext)
{
	int more;
    ttglobal tg = globalcontext;
    fwl_setCurrentHandle(tg, __FILE__, __LINE__);
    more = 0;
    while((more = workers_running()) && more > 0)
    {
        usleep(100);
    }
    if (more == 0)
    {
        markForDispose(rootNode(), TRUE);
        killNodes(); //deallocates nodes MarkForDisposed
        
        
        finalizeRenderSceneUpdateScene();
#ifdef DISABLER
#if defined(WRAP_MALLOC) || defined(DEBUG_MALLOC)
        freewrlFreeAllRegisteredAllocations();
        freewrlDisposeMemTable();
#endif
        __iglobal_destructor(tg);
#endif
    }
}

void fwl_doQuit()
{
	ttglobal tg = gglobal();
	tg->threads.MainLoopQuit = max(1,tg->threads.MainLoopQuit); //make sure we don't go backwards in the quit process with a double 'q'
}

void fwl_doQuitAndWait(){
	pthread_t displaythread;
	ttglobal tg = gglobal();
	displaythread = tg->threads.DispThrd;
	fwl_doQuit();
	pthread_join(displaythread,NULL);

}
// tmp files are on a per-invocation basis on Android, and possibly other locations.
// note that the "tempnam" function will accept NULL as the directory on many platforms,
// so this function does not really need to be called on many platforms.
void fwl_tmpFileLocation(char *tmpFileLocation) {
	ttglobal tg;
	if (tmpFileLocation == NULL) return;
	tg = gglobal();
	FREE_IF_NZ(tg->Mainloop.tmpFileLocation);
	tg->Mainloop.tmpFileLocation = MALLOC(char *,strlen(tmpFileLocation)+1);
	strcpy(tg->Mainloop.tmpFileLocation,tmpFileLocation);
}

void close_internetHandles();
void freewrlDie (const char *format) {
        ConsoleMessage ("Catastrophic error: %s\n",format);
        fwl_doQuit();
}
void fwl_handle_aqua_multiNORMAL(const int mev, const unsigned int button, int x, int y, int ID, int windex) {
	int count;
	int screenWidth, screenHeight;
	struct Touch *touch;
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;
	if(0){
		printf ("fwl_handle_aqua in MainLoop; mev %d but %d x %d y %d ID %d ",
				mev, button, x,y,ID);
		if (mev == ButtonPress) printf ("ButtonPress\n");
		else if (mev == ButtonRelease) printf ("ButtonRelease\n");
		else if (mev == MotionNotify) printf ("MotionNotify\n");
		else printf ("event %d\n",mev); 
	}
	/* save this one... This allows Sensors to get mouse movements if required. */
	p->lastMouseEvent = mev;
	fwl_getWindowSize1(windex,&screenWidth,&screenHeight);

	/* save the current x and y positions for picking. */
	touch = &p->touchlist[ID];
	touch->x = x;
	touch->y = y;
	touch->fx = (float)(x) / (float)screenWidth;
	touch->fy = (float)(y) / (float)screenHeight;
	touch->windex = windex;
	touch->buttonState[button] = mev == ButtonPress;
	touch->ID = ID; /*will come in handy if we change from array[] to accordian list*/
	touch->mev = mev;
	touch->angle = 0.0f;
	p->currentTouch = ID;

	if ((mev == ButtonPress) || (mev == ButtonRelease)) {
		/* if we are Not over an enabled sensitive node, and we do NOT already have a
			button down from a sensitive node... */

		if (((p->CursorOverSensitive ==NULL) && (p->lastPressedOver ==NULL)) || Viewer()->LookatMode || tg->Mainloop.SHIFT) {
			p->NavigationMode = touch->buttonState[LMB] || touch->buttonState[RMB];
			handle(mev, button, (float) ((float)x/screenWidth), (float) ((float)y/screenHeight));
		}
	}

	if (mev == MotionNotify) {
		if (p->NavigationMode) {
			/* find out what the first button down is */
			count = 0;
			while ((count < 4) && (!touch->buttonState[count])) count++;
			if (count == 4) return; /* no buttons down???*/

			handle (mev, (unsigned) count, (float) ((float)x/screenWidth), (float) ((float)y/screenHeight));
		}
	}
}
void (*fwl_handle_aqua_multiPTR)(const int mev, const unsigned int button, int x, int y, int ID, int windex) = fwl_handle_aqua_multiNORMAL;
void fwl_handle_aqua_multi(const int mev, const unsigned int button, int x, int y, int ID, int windex)
{
	fwl_handle_aqua_multiPTR(mev, button, x, y, ID, windex);
}


void emulate_multitouch(const int mev, const unsigned int button, int x, int y, int windex)
{
	/* CREATE/DELETE a touch with RMB down 
	   GRAB/MOVE a touch with LMB down and drag
	   ID=0 reserved for 'normal' cursor
	*/
    int i,ifound,ID,screenWidth,screenHeight;
	struct Touch *touch;
	float fx, fy;
	static int buttons[4] = {0,0,0,0};
	static int idone = 0;
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;
	
	if(!idone){
		printf("Use RMB (right mouse button) to create and delete touches\n");
		printf("Use LMB to drag touches (+- 5 pixel selection window)\n");
		idone = 1;
	}
	//convert to 0-1 float, in case window is resized
	fwl_getWindowSize1(windex,&screenWidth,&screenHeight);
	fx = (float)(x)/(float)screenWidth;
	fy = (float)(y)/(float)screenHeight;
	buttons[button] = mev == ButtonPress;
	ifound = 0;
	ID = -1;
	touch = NULL;

	for(i=0;i<p->ntouch;i++){
		touch = &p->touchlist[i];
		if(touch->ID > -1){
			if(touch->windex == windex)
			if((fabs(fx - touch->fx) < .03f) && (fabs(fy - touch->fy) < .03f)){
				ifound = 1;
				ID = i;
				break;
			}
		}
	}

	if( mev == ButtonPress && button == RMB )
	{
		//if near an existing one, delete
		if(ifound && touch){
			fwl_handle_aqua_multiNORMAL(ButtonRelease,LMB,x,y,ID,windex);
			//delete
			touch->ID = -1;
			printf("delete ID=%d windex=%d\n",ID,windex);
		}
		//else create
		if(!ifound){
			//create!
			for(i=0;i<p->ntouch;i++){
				touch = &p->touchlist[i];
				if(touch->ID < 0) {
					fwl_handle_aqua_multiNORMAL(mev, LMB, x, y, i,windex);
					printf("create ID=%d windex=%d\n",i,windex);
					break;
				}
			}
		}
	}else if( mev == MotionNotify && buttons[LMB])	{
		//if near an existing one, grab it and move it
		if(ifound){
			fwl_handle_aqua_multiNORMAL(MotionNotify,0,x,y,ID,windex);
			//printf("drag ID=%d \n",ID);
		}
		if(0) if(!ifound){
			/* normal, no need to emulate */
			fwl_handle_aqua_multiNORMAL(mev,button,x,y,0,windex);
		}
	}
}
/* old function should still work, with single mouse and ID=0 */
int fwl_handle_aqua1(const int mev, const unsigned int button, int x, int y, int windex) {
	int yup, screenWidth, screenHeight;
    ttglobal tg = gglobal();

	/* printf ("fwl_handle_aqua, type %d, screen wid:%d height:%d, orig x,y %d %d\n",
            mev,tg->display.screenWidth, tg->display.screenHeight,x,y); */
	fwl_getWindowSize1(windex,&screenWidth,&screenHeight);

	// do we have to worry about screen orientations (think mobile devices)
	#if defined (IPHONE) || defined (_ANDROID)
	{

        // iPhone - with home button on bottom, in portrait mode,
        // top left hand corner is x=0, y=0;
        // bottom left, 0, 468)
        // while standard opengl is (0,0) in lower left hand corner...
		int ox = x;
		int oy = y;

		// these make sense for walk navigation
		if (Viewer()->type == VIEWER_WALK) {
			switch (Viewer()->screenOrientation) {
				case 0:
					x = screenHeight-x;

					break;
				case 90:
					x = oy;
					y = ox;
					break;
				case 180:
					x = x;
					y = -y;
					break;
				case 270:
					x = screenWidth - oy;
					y = screenHeight - ox;
					break;
				default:{}
			}

		// these make sense for examine navigation
		} else if (Viewer()->type == VIEWER_EXAMINE) {
			switch (Viewer()->screenOrientation) {
				case 0:
					break;
				case 90:
					x = screenWidth - oy;
					y = ox;
					break;
				case 180:
					x = screenWidth -x;
					y = screenHeight -y;
					break;
				case 270:
					// nope x = tg->display.screenWidth - oy;
					// nope y = tg->display.screenHeight - ox;

					x = screenHeight - oy;
					y = screenWidth - ox;

					//printf ("resulting in x %d  y %d\n",x,y);
					break;
				default:{}
			}

		}
	}

	#endif

	//Nov. 2015 changed freewrl mouse from y-down to y-up from here on down:
	//all y-up now: sesnsor/picking, explore, statusbarHud, handle0 > all navigations, emulate_multitouch, sidebyside fiducials
	yup = screenHeight -y;
	if(((ppMainloop)(tg->Mainloop.prv))->EMULATE_MULTITOUCH){
		emulate_multitouch(mev,button,x, yup,windex);
	}else{
		fwl_handle_aqua_multi(mev,button,x,yup,0,windex);
	}
	return getCursorStyle();
}
int fwl_handle_aqua(const int mev, const unsigned int button, int x, int y){
	return fwl_handle_aqua1(mev,button,x,y,0);
}
//#endif

void fwl_setCurXY(int cx, int cy) {
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;
	/* printf ("fwl_setCurXY, have %d %d\n",p->currentX[p->currentCursor],p->currentY[p->currentCursor]); */
	p->touchlist[p->currentTouch].x = cx;
	p->touchlist[p->currentTouch].y = cy;
        //tg->Mainloop.currentX[p->currentCursor] = cx;
        //tg->Mainloop.currentY[p->currentCursor] = cy;
}

void fwl_setButDown(int button, int value) {
	//BUTTON NONE,LMB,MMB,RMB 0,1,2,3
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
	/* printf ("fwl_setButDown called\n"); */
    //    p->ButDown[p->currentCursor][button] = value;
	p->touchlist[p->currentTouch].buttonState[button] = value;
}


/* mobile devices - set screen orientation */
/* "0" is "normal" orientation; degrees clockwise; note that face up and face down not
   coded; assume only landscape/portrait style orientations */

void fwl_setOrientation (int orient) {
	switch (orient) {
		case 0:
		case 90:
		case 180:
		case 270:
			{
			Viewer()->screenOrientation = orient;
			break;
		}
		default: {
			ConsoleMessage ("invalid orientation %d\n",orient);
			Viewer()->screenOrientation = 0;
		}
	}
}



void setIsPlugin() {

        RUNNINGASPLUGIN = TRUE;

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

#ifdef AQUA

int aquaPrintVersion() {
	printf ("FreeWRL version: %s\n", libFreeWRL_get_version());
	exit(EXIT_SUCCESS);
}

#endif

/* if we are visible, draw the OpenGL stuff, if not, don not bother */
void setDisplayed (int state) {
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;

        #ifdef VERBOSE
        if (state) printf ("WE ARE DISPLAYED\n");
        else printf ("we are now iconic\n");
        #endif
        p->onScreen = state;
}

void fwl_init_EaiVerbose() {
        //eaiverbose = TRUE;
#if !defined(EXCLUDE_EAI)
	gglobal()->EAI_C_CommonFunctions.eaiverbose = TRUE;
	fwlio_RxTx_control(CHANNEL_EAI, RxTx_MOREVERBOSE); /* RxTx_SILENT */
#endif

}

#if defined (_ANDROID)

void fwl_Android_replaceWorldNeeded() {
	int i;
	#ifndef AQUA
        char mystring[20];
	#endif
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;

	/* get rid of sensor events */
	resetSensorEvents();

	/* make the root_res equal NULL - this throws away all old resource info */
	gglobal()->resources.root_res = NULL;
	Android_reset_viewer_to_defaults();

        struct tProdCon *t = &gglobal()->ProdCon;

	// if we have a bound vp; if the old world did not have a vp, there will be nothing to send_bind_to
	if (vectorSize(t->viewpointNodes) > t->currboundvpno) {
		send_bind_to(vector_get(struct X3D_Node*, t->viewpointNodes,t->currboundvpno),0);
	}

	if (rootNode() != NULL) {

		/* mark all rootNode children for Dispose */
		for (i=0; i<proto->__children.n; i++) {
			markForDispose(proto->__children.p[i], TRUE);
		}

		/* stop rendering. This should be done when the new resource is loaded, and new_root is set,
		but lets do it here just to make sure */
		proto->__children.n = 0; // no children, but _sortedChildren not made;
		proto->_change ++; // force the rootNode()->_sortedChildren to be made
	}

	/* close the Console Message system, if required. */
	closeConsoleMessage();

	/* occlusion testing - zero total count, but keep MALLOC'd memory around */
	zeroOcclusion();

	/* clock events - stop them from ticking */
	kill_clockEvents();

	/* kill DEFS, handles */
	//if we do this here, we have a problem, as the parser is already killed and cleaned up.
	//EAI_killBindables();

	kill_bindables();
	killKeySensorNodeList();

	/* stop routing */
	kill_routing();

	/* tell the statusbar that it needs to reinitialize */
	//kill_status();
	setMenuStatus(NULL);

	/* any user defined Shader nodes - ComposedShader, PackagedShader, ProgramShader?? */
	kill_userDefinedShaders();

	/* free scripts */
	#ifdef HAVE_JAVASCRIPT
	kill_javascript();
	#endif

	#if !defined(EXCLUDE_EAI)
	/* free EAI */
	if (kill_EAI) {
	       	/* shutdown_EAI(); */
		fwlio_RxTx_control(CHANNEL_EAI, RxTx_STOP) ;
	}
	#endif //EXCLUDE_EAI

	#ifndef AQUA
		sprintf (mystring, "QUIT");
		Sound_toserver(mystring);
	#endif

	/* reset any VRML Parser data */
	if (globalParser != NULL) {
		parser_destroyData(globalParser);
		//globalParser = NULL;
		gglobal()->CParse.globalParser = NULL;
	}

	kill_X3DDefs();

	/* tell statusbar that we have none */
	viewer_default();
	setMenuStatus("NONE");
}
#endif


#if !defined(_ANDROID)

// JAS - Do not know if these are still required.

/* called from the standalone OSX front end and the OSX plugin */
char *strBackslash2fore(char *);
void fwl_replaceWorldNeeded(char* str)
{
	ConsoleMessage("file to load: %s\n",str);
    FREE_IF_NZ(gglobal()->Mainloop.replaceWorldRequest);
	gglobal()->Mainloop.replaceWorldRequest = strBackslash2fore(STRDUP(str));
}
void fwl_replaceWorldNeededRes(resource_item_t *multiResWithParent){
	gglobal()->Mainloop.replaceWorldRequestMulti = (void*)(multiResWithParent);
}


void fwl_reload()
{
	ConsoleMessage("fwl_reload called");
}

#endif //NOT _ANDROID


/* OSX the Plugin is telling the displayThread to stop and clean everything up */
void stopRenderingLoop(void) {
	ttglobal tg = gglobal();
	//printf ("stopRenderingLoop called\n");

#if !defined(FRONTEND_HANDLES_DISPLAY_THREAD)
	if(!((freewrl_params_t*)(tg->display.params))->frontend_handles_display_thread)
    	stopDisplayThread();
#endif

    	//killErrantChildren();
	/* lets do an equivalent to replaceWorldNeeded, but with NULL for the new world */

        setAnchorsAnchor( NULL );
        tg->RenderFuncs.BrowserAction = TRUE;
	#ifdef OLDCODE
        OLDCODE FREE_IF_NZ(tg->RenderFuncs.OSX_replace_world_from_console);
	#endif //OLDCODE
	// printf ("stopRenderingLoop finished\n");
}


/* send the description to the statusbar line */
void sendDescriptionToStatusBar(struct X3D_Node *CursorOverSensitive) {
        int tmp;
        char *ns;
		ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;

		if (CursorOverSensitive == NULL) update_status(NULL);
        else {

                ns = NULL;
                for (tmp=0; tmp<p->num_SensorEvents; tmp++) {
                        if (p->SensorEvents[tmp].fromnode == CursorOverSensitive) {
                                switch (p->SensorEvents[tmp].datanode->_nodeType) {
                                        case NODE_Anchor: ns = ((struct X3D_Anchor *)p->SensorEvents[tmp].datanode)->description->strptr; break;
										case NODE_LineSensor: ns = ((struct X3D_LineSensor *)p->SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_PlaneSensor: ns = ((struct X3D_PlaneSensor *)p->SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_SphereSensor: ns = ((struct X3D_SphereSensor *)p->SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_TouchSensor: ns = ((struct X3D_TouchSensor *)p->SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_GeoTouchSensor: ns = ((struct X3D_GeoTouchSensor *)p->SensorEvents[tmp].datanode)->description->strptr; break;
                                        case NODE_CylinderSensor: ns = ((struct X3D_CylinderSensor *)p->SensorEvents[tmp].datanode)->description->strptr; break;
                                        default: {printf ("sendDesc; unknown node type %d\n",p->SensorEvents[tmp].datanode->_nodeType);}
                                }
                                /* if there is no description, put the node type on the screen */
                                if (ns == NULL) {ns = "(over sensitive)";}
                                else if (ns[0] == '\0') ns = (char *)stringNodeType(p->SensorEvents[tmp].datanode->_nodeType);

                                /* send this string to the screen */
								update_status(ns);
                        }
                }
        }
}


/* We have a new file to load, lets get rid of the old world sensor events, and run with it */
void resetSensorEvents(void) {
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;

	if (p->oldCOS != NULL)
		sendSensorEvents(p->oldCOS,MapNotify,p->touchlist[p->currentTouch].buttonState[LMB], FALSE);
		//sendSensorEvents(p->oldCOS,MapNotify,p->ButDown[p->currentCursor][1], FALSE);
    /* remove any display on-screen */
    sendDescriptionToStatusBar(NULL);
	p->CursorOverSensitive=NULL;

	p->oldCOS=NULL;
	p->lastMouseEvent = 0;
	p->lastPressedOver = NULL;
	p->lastOver = NULL;
	FREE_IF_NZ(p->SensorEvents);
	p->num_SensorEvents = 0;
	gglobal()->RenderFuncs.hypersensitive = NULL;
	gglobal()->RenderFuncs.hyperhit = 0;

}

#if defined (_ANDROID) || defined (AQUA)

struct X3D_IndexedLineSet *fwl_makeRootBoundingBox() {
	struct X3D_Node *shape, *app, *mat, *ils = NULL;
	struct X3D_Node *bbCoord = NULL;

	struct X3D_Group *rn = rootNode(); //attn Disabler, rootNode() is now always X3D_Proto
        float emis[] = {0.8, 1.0, 0.6};
        float myp[] = {
            -2.0, 1.0, 1.0,
            2.0, 1.0, 1.0,
            2.0, 1.0, -1.0,
            -2.0, 1.0, -1.0,
            -2.0, -1.0, 1.0,
            2.0, -1.0, 1.0,
            2.0, -1.0, -1.0,
            -2.0, -1.0, -1.0
        };
        int myci[] = {
            0, 1, 2, 3, 0, -1,
            4, 5, 6, 7, 4, -1,
            0, 4, -1,
            1, 5, -1,
            2, 6, -1,
            3, 7, -1

        };

	if (rn == NULL) return NULL;

	if (rn->children.n > 0) {
		shape = createNewX3DNode(NODE_Shape);
		app = createNewX3DNode(NODE_Appearance);
		mat = createNewX3DNode(NODE_Material);
		ils = createNewX3DNode(NODE_IndexedLineSet);
		bbCoord = createNewX3DNode(NODE_Coordinate);
		//ConsoleMessage ("adding shape to rootNode");

		memcpy(X3D_MATERIAL(mat)->emissiveColor.c,emis,sizeof(float) * 3);
		X3D_INDEXEDLINESET(ils)->coordIndex.p = MALLOC (int *, sizeof(int) * 24);
		X3D_INDEXEDLINESET(ils)->coordIndex.n = 24;
		memcpy(X3D_INDEXEDLINESET(ils)->coordIndex.p, myci, sizeof(int) * 24);

		X3D_COORDINATE(bbCoord)->point.p = MALLOC( struct SFVec3f *, sizeof(struct SFVec3f) * 8);
		X3D_COORDINATE(bbCoord)->point.n = 8;
		memcpy(X3D_COORDINATE(bbCoord)->point.p, myp, sizeof (struct SFVec3f) * 8);

		// MFNode field manipulation
		AddRemoveChildren(X3D_NODE(rootNode()),
			offsetPointer_deref(void *,rootNode(),
			offsetof(struct X3D_Group, children)),
			&shape,1,1,__FILE__,__LINE__);

		// SFNode manipulation
		X3D_SHAPE(shape)->appearance = app;
		ADD_PARENT(app,shape);

		X3D_SHAPE(shape)->geometry = ils;

		// we break the back link, so that this IndexedLineSet does not affect the
		// bounding box. Try this with the 1.wrl test, with a Transform, translation in
		// it, and see the difference
		//ADD_PARENT(ils,shape);

		X3D_INDEXEDLINESET(ils)->coord = bbCoord;
		ADD_PARENT(ils,bbCoord);

		X3D_APPEARANCE(app)->material = mat;
		ADD_PARENT(mat,app);

		return X3D_INDEXEDLINESET(ils);
	}
	return NULL;
}

void fwl_update_boundingBox(struct X3D_IndexedLineSet* node) {

	struct X3D_Group *rn = rootNode(); //attn Disabler, rootNode() is now always X3D_Proto
	struct SFVec3f newbbc[8];

	if (node==NULL) return;
	if (rn != NULL) {
		// x coordinate
		newbbc[1].c[0] = newbbc[2].c[0]= newbbc[5].c[0] = newbbc[6].c[0]=rn->EXTENT_MAX_X;
		newbbc[0].c[0] = newbbc[3].c[0]= newbbc[4].c[0] = newbbc[7].c[0]=rn->EXTENT_MIN_X;

		// y coordinate
		newbbc[0].c[1] = newbbc[1].c[1] = newbbc[2].c[1] = newbbc[3].c[1]=rn->EXTENT_MAX_Y;
		newbbc[4].c[1] = newbbc[5].c[1] = newbbc[6].c[1] = newbbc[7].c[1]=rn->EXTENT_MIN_Y;

		// z coordinate
		newbbc[0].c[2] = newbbc[1].c[2] = newbbc[4].c[2] = newbbc[5].c[2]=rn->EXTENT_MAX_Z;
		newbbc[2].c[2] = newbbc[3].c[2] = newbbc[6].c[2] = newbbc[7].c[2]=rn->EXTENT_MIN_Z;

		memcpy(X3D_COORDINATE(node->coord)->point.p, newbbc, sizeof (struct SFVec3f) * 8);

		node->_change++;
	}
}
#endif // _ANDROID
