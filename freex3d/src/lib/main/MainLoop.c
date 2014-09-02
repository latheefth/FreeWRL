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

#include "ProdCon.h"

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
	int button; /*none down=0, LMB =1, MMB=2, RMB=3*/
	bool isDown; /* false = up, true = down */
	int mev; /* down/press=4, move/drag=6, up/release=5 */
	int ID;  /* for multitouch: 0-20, represents one finger drag. Recycle after an up */
	float angle; /*some multitouch -like smarttech- track the angle of the finger */
	int x;
	int y;
};
struct keypressTuple{
	int key;
	int type;
};
struct mouseTuple{
	int mev;
	unsigned int button;
	float x;
	float y;
	int ix;
	int iy;
	int ID;
};
struct playbackRecord {
	int frame;
	double dtime;
	//should we use more general Touch instead of mouse-specific?
	int *mousetuples; //x,y,button chord
	int mouseCount; //# mouse tuples
	char *keystrokes;
	int keyCount;
};
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
	int ButDown[20][8];// = {{FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE}};

	int currentCursor;// = 0;
	int lastMouseEvent;// = 0/*MapNotify*/;         /*  last event a mouse did; care about Button and Motion events only.*/
	struct X3D_Node* lastPressedOver;// = NULL;/*  the sensitive node that the mouse was last buttonpressed over.*/
	struct X3D_Node* lastOver;// = NULL;       /*  the sensitive node that the mouse was last moused over.*/
	int lastOverButtonPressed;// = FALSE;      /*  catch the 1 to 0 transition for button presses and isOver in TouchSensors */

	int maxbuffers;// = 1;                     /*  how many active indexes in bufferarray*/
	int bufferarray[2];// = {GL_BACK,0};

	double BrowserStartTime;        /* start of calculating FPS     */

	//int quitThread;// = FALSE;
	int keypress_wait_for_settle;// = 100;     /* JAS - change keypress to wait, then do 1 per loop */
	char * keypress_string;//=NULL;            /* Robert Sim - command line key sequence */

	struct SensStruct *SensorEvents;// = 0;

    unsigned int loop_count;// = 0;
    unsigned int slowloop_count;// = 0;
	double waitsec;

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
	FILE* recordingFile;
	char* recordingFName;
	int modeRecord;
	int modeFixture;
	int modePlayback;
	int fwplayOpened;
	char *nameTest;
	int frameNum; //for Record, Playback - frame# =0 after scene loaded
	struct playbackRecord* playback;
	int playbackCount;
	struct keypressTuple keypressQueue[50]; //for Record,Playback where keypresses are applied just once per frame for consistency
	int keypressQueueCount;
	struct mouseTuple mouseQueue[50];
	int mouseQueueCount;
	FILE* logfile;
	FILE* logerr;
	char* logfname;
	int logging;
	int keySensorMode;
	int draw_initialized;
}* ppMainloop;
void *Mainloop_constructor(){
	void *v = malloc(sizeof(struct pMainloop));
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

		p->currentCursor = 0;
		p->lastMouseEvent = 0/*MapNotify*/;         /*  last event a mouse did; care about Button and Motion events only.*/
		p->lastPressedOver = NULL;/*  the sensitive node that the mouse was last buttonpressed over.*/
		p->lastOver = NULL;       /*  the sensitive node that the mouse was last moused over.*/
		p->lastOverButtonPressed = FALSE;      /*  catch the 1 to 0 transition for button presses and isOver in TouchSensors */

		p->maxbuffers = 1;                     /*  how many active indexes in bufferarray*/
		p->bufferarray[0] = GL_BACK;
		p->bufferarray[1] = 0;
		/* current time and other time related stuff */
		//p->BrowserStartTime;        /* start of calculating FPS     */

		//p->quitThread = FALSE;
		p->keypress_wait_for_settle = 100;     /* JAS - change keypress to wait, then do 1 per loop */
		p->keypress_string=NULL;            /* Robert Sim - command line key sequence */

		p->SensorEvents = 0;

        p->loop_count = 0;
        p->slowloop_count = 0;
		//p->waitsec;

		//scene
		//window
		//2D_inputdevice
		p->lastDeltax = 50;
		p->lastDeltay = 50;
		//p->lastxx;
		//p->lastyy;
		p->ntouch =0;
		p->currentTouch = -1;
		//p->touchlist[20];
		p->EMULATE_MULTITOUCH = 0;
		p->recordingFile = NULL;
		p->recordingFName = NULL;
		p->modeRecord = FALSE;
		p->modeFixture = FALSE;
		p->modePlayback = FALSE;
		p->nameTest = NULL;
		p->frameNum = 0;
		p->playbackCount = 0;
		p->playback = NULL;
		p->fwplayOpened = 0;
		p->keypressQueueCount=0;
		p->mouseQueueCount=0;
		p->logfile = NULL;
		p->logerr = NULL;
		p->logfname = NULL;
		p->logging = 0;
		p->keySensorMode = 1; //by default on, so it works 'out of the gate' if Key or StringSensor in scene, then ESC to toggle off
		p->draw_initialized = FALSE;
	}
}

//true statics:
int isBrowserPlugin = FALSE; //I can't think of a scenario where sharing this across instances would be a problem
///* are we displayed, or iconic? */
//static int onScreen = TRUE;
//
//
///* do we do event propagation, proximity calcs?? */
//static int doEvents = FALSE;
//
//#ifdef VERBOSE
//static char debs[300];
//#endif
//
//char* PluginFullPath;
//
///* linewidth for lines and points - passed in on command line */
//float gl_linewidth = 1.0f;
//
///* what kind of file was just parsed? */
//int currentFileVersion = 0;

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

/* Function protos */
static void sendDescriptionToStatusBar(struct X3D_Node *CursorOverSensitive);
/* void fwl_do_keyPress(char kp, int type); Now in lib.h */
void render_collisions(int Viewer_type);
void slerp_viewpoint();
static void render_pre(void);
static void render(void);
static void setup_projection(int pick, int x, int y);
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





#ifdef OLDSTOPCODE
// stops the Texture loading thread - will either pthread_cancel or will send SIGUSR2 to
// the thread, depending on platform.

static void stopLoadThread()
{
	ttglobal tg = gglobal();
	if (!TEST_NULL_THREAD(tg->threads.loadThread)) {

		#if defined(HAVE_PTHREAD_CANCEL)
			//pthread_cancel(tg->threads.loadThread);
	 	#else

		{
			int status;
			char me[200];
			sprintf(me,"faking pthread cancel on thread %p",tg->threads.loadThread);
			//ConsoleMessage(me);
			if ((status = pthread_kill(tg->threads.loadThread, SIGUSR2)) != 0) {
				ConsoleMessage("issue stopping thread");
			}
		}
		#endif //HAVE_PTHREAD_CANCEL

		pthread_join(tg->threads.loadThread,NULL);
		ZERO_THREAD(tg->threads.loadThread);
	}
}


// stops the source parsing thread - will either pthread_cancel or will send SIGUSR2 to
// the thread, depending on platform.

static void stopPCThread()
{
	ttglobal tg = gglobal();

	if (!TEST_NULL_THREAD(tg->threads.PCthread)) {
		#if defined(HAVE_PTHREAD_CANCEL)
			//pthread_cancel(tg->threads.PCthread);
	 	#else

		{
			int status;
			char me[200];
			sprintf(me,"faking pthread cancel on thread %p",tg->threads.PCthread);
			//ConsoleMessage(me);
			if ((status = pthread_kill(tg->threads.PCthread, SIGUSR2)) != 0) {
				ConsoleMessage("issue stopping thread");
			}
		}
	#endif //HAVE_PTHREAD_CANCEL

		pthread_join(tg->threads.PCthread,NULL);
		ZERO_THREAD(tg->threads.PCthread);
	}
}
#endif
//static double waitsec;

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

int dequeueKeyPress(ppMainloop p,int *key, int *type){
	if(p->keypressQueueCount > 0){
		int i;
		p->keypressQueueCount--;
		*key = p->keypressQueue[0].key;
		*type = p->keypressQueue[0].type;
		for(i=0;i<p->keypressQueueCount;i++){
			p->keypressQueue[i].key = p->keypressQueue[i+1].key;
			p->keypressQueue[i].type = p->keypressQueue[i+1].type;
		}
		return 1;
	}
	return 0;
}
int dequeueMouse(ppMainloop p, int *mev, unsigned int *button, float *x, float *y){
	if(p->mouseQueueCount > 0){
		int i;
		p->mouseQueueCount--;
		*mev = p->mouseQueue[0].mev;
		*button = p->mouseQueue[0].button;
		*x = p->mouseQueue[0].x;
		*y = p->mouseQueue[0].y;
		for(i=0;i<p->mouseQueueCount;i++){
			p->mouseQueue[i].mev = p->mouseQueue[i+1].mev;
			p->mouseQueue[i].button = p->mouseQueue[i+1].button;
			p->mouseQueue[i].x = p->mouseQueue[i+1].x;
			p->mouseQueue[i].y = p->mouseQueue[i+1].y;
		}
		return 1;
	}
	return 0;
}
int dequeueMouseMulti(ppMainloop p, int *mev, unsigned int *button, int *ix, int *iy, int *ID){
	if(p->mouseQueueCount > 0){
		int i;
		p->mouseQueueCount--;
		*mev = p->mouseQueue[0].mev;
		*button = p->mouseQueue[0].button;
		*ix = p->mouseQueue[0].ix;
		*iy = p->mouseQueue[0].iy;
		*ID = p->mouseQueue[0].ID;
		for(i=0;i<p->mouseQueueCount;i++){
			p->mouseQueue[i].mev = p->mouseQueue[i+1].mev;
			p->mouseQueue[i].button = p->mouseQueue[i+1].button;
			p->mouseQueue[i].ix = p->mouseQueue[i+1].ix;
			p->mouseQueue[i].iy = p->mouseQueue[i+1].iy;
			p->mouseQueue[i].ID = p->mouseQueue[i+1].ID;
		}
		return 1;
	}
	return 0;
}

/* Main eventloop for FreeWRL!!! */
void fwl_do_keyPress0(int key, int type);
void handle0(const int mev, const unsigned int button, const float x, const float y);
void fwl_handle_aqua_multi(const int mev, const unsigned int button, int x, int y, int ID);
void fwl_handle_aqua_multi0(const int mev, const unsigned int button, int x, int y, int ID);

#if !defined(FRONTEND_DOES_SNAPSHOTS)
void fwl_RenderSceneUpdateScene0(double dtime);
void set_snapshotModeTesting(int value);
int isSnapshotModeTesting();
void splitpath_local_suffix(const char *url, char **local_name, char **suff);
#endif //FRONTEND_DOES_SNAPSHOTS



int fw_exit(int val)
{
	printf("exiting with value=%d hit Enter:",val);
	getchar();
	exit(val);
}

#if !defined(FRONTEND_DOES_SNAPSHOTS)
int fw_mkdir(char* path);
void fwl_RenderSceneUpdateScene() {
	double dtime;
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;

	dtime = Time1970sec();
	if((p->modeRecord || p->modeFixture || p->modePlayback)) //commandline --record/-R and --playback/-P, for automated testing
	{
		//functional testing support options May 2013
		//records frame#, dtime, keyboard, mouse to an ASCII .fwplay file for playback
		//to record, run a scene file with -R or --record option
		//copy the .fwplay between platforms
		//before starting refactoring, run scenes with -F or --fixture option,
		//  and hit the 'x' key to save a snapshot one or more times per fixture run
		//after each refactoring step, run scenes with -P or --playback option,
		//  and (with perl script) do a file compare(fixture_snapshot,playback_snapshot)
		//
		//on the command line use:
		//-R to just record the .fwplay file
		//-F to play recording and save as fixture
		//-P to play recording and save as playback
		//-R -F to record and save as fixture in one step
		//command line long option equivalents: -R --record, -F --fixture, -P --playback
		int key;
		int type;
		int mev,ix,iy,ID;
		unsigned int button;
		float x,y;
		char buff[1000], keystrokes[200], mouseStr[1000];
		int namingMethod;
		char *folder;
		char sceneName[1000];
		//naming method for related files (and folders)
		//0=default: recording.fwplay, fixture.bmp playback.bmp - will overwrite for each scene
		//1=folders: 1_wrl/recording.fwplay, 1_wrl/fixture/17.bmp, 1_wrl/playback/17.bmp
		//2=flattened: 1_wrl.fwplay, 1_wrl_fixture_17.bmp, 1_wrl_playback_17.bmp (17 is frame#)
		//3=groupfolders: /tests, /recordings/*.fwplay, /fixtures/1_wrl_17.bmp /playbacks/1_wrl_17.bmp
		//4=groupfolders: /tests, /recordings/*.fwplay, /fixtures/1_wrl_17.bmp /playbacks/1_wrl_17.bmp
		//  - 4 same as 3, except done to harmonize with linux/aqua naming approach:
		//  - fwl_set_SnapFile(path = {"fixture" | "playback" }); to set mytmp
		//  -
		folder = NULL;
		namingMethod = 4;
		//if(p->frameNum == 1){
		if(!p->fwplayOpened){
			char recordingName[1000];
			int j,k;
			p->fwplayOpened = 1;
			recordingName[0] = '\0';
			sceneName[0] = '\0';
			if(tg->Mainloop.scene_name){
				strcat(sceneName,tg->Mainloop.scene_name);
				if(tg->Mainloop.scene_suff){
					strcat(sceneName,".");
					strcat(sceneName,tg->Mainloop.scene_suff);
				}
			}
			if(namingMethod==3 || namingMethod==4){
				strcpy(recordingName,"recording");
				fw_mkdir(recordingName);
				strcat(recordingName,"/");
			}
			if(namingMethod>0){
				if(p->nameTest){
					strcat(recordingName,p->nameTest);
				}else{
					strcat(recordingName,tg->Mainloop.scene_name);
					k = strlen(recordingName);
					if(k){
						//1.wrl -> 1_wrl
						j = strlen(tg->Mainloop.scene_suff);
						if(j){
							strcat(recordingName,"_");
							strcat(recordingName,tg->Mainloop.scene_suff);
						}
					}
				}
			}
			if(namingMethod==1){
				fw_mkdir(recordingName);
				strcat(recordingName,"/recording"); //recording.fwplay a generic name, in case there's no scene name
			}
			if(namingMethod==0)
				strcat(recordingName,"recording");
			strcat(recordingName,".fwplay"); //1_wrl.fwplay
			p->recordingFName = strdup(recordingName);

			if(p->modeFixture  || p->modePlayback){
				if(!p->modeRecord){
					p->recordingFile = fopen(p->recordingFName, "r");
					if(p->recordingFile == NULL){
						printf("ouch recording file %s not found\n", p->recordingFName);
						fw_exit(1);
					}
					if( fgets(buff, 1000, p->recordingFile) != NULL){
						char window_widthxheight[100], equals[50];
						int width, height;
						//window_wxh = 600,400
						if( sscanf(buff,"%s %s %d, %d\n",window_widthxheight,equals, &width,&height) == 4) {
							if(width != tg->display.screenWidth || height != tg->display.screenHeight){
								if(1){ //right now all we can do is passively complain
									printf("Ouch - the test playback window size is different than recording:\n");
									printf("recording %d x %d playback %d x %d\n",width,height,
										tg->display.screenWidth,tg->display.screenHeight);
									printf("hit Enter:");
									getchar();
								}
								//if(0){
								//	fwl_setScreenDim(width,height); //this doesn't actively set the window size except before window is created
								//}
							}
						}
					}
					if( fgets(buff, 1000, p->recordingFile) != NULL){
						char scenefile[100], equals[50];
						//scenefile = 1.wrl
						if( sscanf(buff,"%s %s %s \n",scenefile,equals, sceneName) == 3) {
							if(!tg->Mainloop.scene_name){
								char* suff = NULL;
								char* local_name = NULL;
								char* url = NULL;
								if(strlen(sceneName)) url = strdup(sceneName);
								if(url){
									splitpath_local_suffix(url, &local_name, &suff);
									gglobal()->Mainloop.url = url;
									gglobal()->Mainloop.scene_name = local_name;
									gglobal()->Mainloop.scene_suff = suff;
									fwl_resource_push_single_request(url);
								}
							}
						}
					}
				}
			}
		}
		p->doEvents = (!fwl_isinputThreadParsing()) && (!fwl_isTextureParsing()) && fwl_isInputThreadInitialized();
		//printf("frame %d doevents=%d\n",p->frameNum,p->doEvents);
		if(!p->doEvents)
			return; //for Record and Playback, don't start doing things until scene and textures are loaded
		if(p->modeRecord)
			if(dtime - tg->Mainloop.TickTime < .5) return; //slow down frame rate to 2fps to reduce empty meaningless records
		p->frameNum++; //for record, frame relative to when scene is loaded

		if(p->modeRecord){
			int i;
			char temp[1000];
			if(p->frameNum == 1){
				p->recordingFile = fopen(p->recordingFName, "w");
				if(p->recordingFile == NULL){
					printf("ouch recording file %s not found\n", p->recordingFName);
					fw_exit(1);
				}
				//put in a header record, passively showing window widthxheight
				fprintf(p->recordingFile,"window_wxh = %d, %d \n",tg->display.screenWidth,tg->display.screenHeight);
				fprintf(p->recordingFile,"scenefile = %s \n",tg->Mainloop.url); //sceneName);
			}
			strcpy(keystrokes,"\"");
			while(dequeueKeyPress(p,&key,&type)){
				sprintf(temp,"%d,%d,",key,type);
				strcat(keystrokes,temp);
			}
			strcat(keystrokes,"\"");
			strcpy(mouseStr,"\"");
			i = 0;
			if(0){
				while(dequeueMouse(p,&mev, &button, &x, &y)){
					sprintf(temp,"%d,%d,%.6f,%.6f;",mev,button,x,y);
					strcat(mouseStr,temp);
					i++;
				}
			}
			if(1){
				while(dequeueMouseMulti(p,&mev, &button, &ix, &iy, &ID)){
					sprintf(temp,"%d,%d,%d,%d,%d;",mev,button,ix,iy,ID);
					strcat(mouseStr,temp);
					i++;
				}
			}
			strcat(mouseStr,"\"");
			fprintf(p->recordingFile,"%d %.6lf %s %s\n",p->frameNum,dtime,keystrokes,mouseStr);
			//in case we are -R -F together,
			//we need to round dtime for -F like it will be coming out of .fwplay for -P
			sprintf(temp,"%.6lf",dtime);
			sscanf(temp,"%lf",&dtime);
			//folder = "fixture";
			folder = NULL;
		}
		if(p->modeFixture  || p->modePlayback){
			if(!p->modeRecord){
				/*
				if(p->frameNum == 1){
					p->recordingFile = fopen(p->recordingFName, "r");
					if(p->recordingFile == NULL){
						printf("ouch recording file %s not found\n", p->recordingFName);
						exit(1);
					}
					if( fgets(buff, 1000, p->recordingFile) != NULL){
						char window_widthxheight[100], equals[50];
						int width, height;
						//window_wxh = 600,400
						if( sscanf(buff,"%s %s %d, %d\n",&window_widthxheight,&equals, &width,&height) == 4) {
							if(width != tg->display.screenWidth || height != tg->display.screenHeight){
								printf("Ouch - the test playback window size is different than recording:\n");
								printf("recording %d x %d playback %d x %d\n",width,height,
									tg->display.screenWidth,tg->display.screenHeight);
								printf("hit Enter:");
								getchar();
							}
						}
					}
					if( fgets(buff, 1000, p->recordingFile) != NULL){
						char scenefile[100], equals[50];
						//scenefile = 1.wrl
						if( sscanf(buff,"%s %s %s \n",&scenefile,&equals, &sceneName) == 3) {
						}
					}
				}
				*/
				// playback[i] = {iframe, dtime, keystrokes or NULL, mouse (xy,button sequence) or NULL, snapshot URL or NULL, scenegraph_dump URL or NULL, ?other?}
				if( fgets( buff, 1000, p->recordingFile ) != NULL ) {
					if(sscanf(buff,"%d %lf %s %s\n",&p->frameNum,&dtime,keystrokes,mouseStr) == 4){ //,snapshotURL,scenegraphURL) == 6){
						if(0) printf("%d %lf %s %s\n",p->frameNum,dtime,keystrokes,mouseStr);
					}
				}
			}
			if(p->modeFixture)  folder = "fixture";
			if(p->modePlayback) folder = "playback";
		}
		//for all 3 - read the keyboard string and the mouse string
		if(p->modeRecord || p->modeFixture || p->modePlayback){
			if(strlen(keystrokes)>2){ // "x,1," == 6
				char *next,*curr;
				//count the number of ','
				//for(i=0,n=0;i<strlen(keystrokes);i++) if(keystrokes[i] == ',') n++; //(strlen(keystrokes) -2)/4;
				//n /= 2; //each keystroke has 2 commas: (char),(type),
				curr = &keystrokes[1]; //skip leading "
				while(curr && strlen(curr)>1){
					//for(i=0;i<n;i++){
					//ii = i*4 +1;
					//sscanf(&keystrokes[ii],"%d,%d",&key,&type);
					sscanf(curr,"%d",&key);
					next = strchr(curr,',');
					curr = &next[1];
					sscanf(curr,"%d",&type);
					next = strchr(curr,',');
					curr = &next[1];
					if(p->modeFixture || p->modePlayback){
						//we will catch the snapshot keybaord command and prepare the
						//snapshot filename and folder/directory for fixture and playback
						if(key == 'x'){
							//prepare snapshot folder(scene/ + fixture ||playback)
							// and file name(frame#)
							char snapfile[5];
#ifdef _MSC_VER
							char *suff = ".bmp";
#else
							char *suff = ".snap";
#endif
							sprintf(snapfile,"%d",p->frameNum);
							if(namingMethod == 0){
								//default: recording.bmp, playback.bmp
								char snappath[100];
								strcpy(snappath,folder);
								strcat(snappath,suff);
								fwl_set_SnapFile(snappath);
							}
							if(namingMethod==1){
								//nested folder approach
								//1=folders: 1_wrl/recording.fwplay, 1_wrl/fixture/17.bmp, 1_wrl/playback/17.bmp
								int k,j;
								char snappath[100];
								strcpy(snappath,tg->Mainloop.scene_name);
								k = strlen(snappath);
								if(k){
									//1.wrl -> 1_wrl
									j = strlen(tg->Mainloop.scene_suff);
									if(j){
										strcat(snappath,"_");
										strcat(snappath,tg->Mainloop.scene_suff);
									}
								}
								strcat(snappath,"/");
								strcat(snappath,folder);
								fw_mkdir(snappath); //1_wrl/fixture
								//fwl_set_SnapTmp(snappath); //sets the folder for snaps
								strcat(snappath,"/");
								strcat(snappath,snapfile);
								strcat(snappath,suff); //".bmp");
								//fwl_set_SnapFile(snapfile);
								fwl_set_SnapFile(snappath); //1_wrl/fixture/17.bmp
							}
							if(namingMethod == 2){
								//flattened filename approach with '_'
								//if snapshot 'x' is on frame 17, and fixture,
								//   then 1_wrl_fixture_17.snap or .bmp
								char snappath[100];
								int j, k;
								strcpy(snappath,tg->Mainloop.scene_name);
								k = strlen(snappath);
								if(k){
									j= strlen(tg->Mainloop.scene_suff);
									if(j){
										strcat(snappath,"_");
										strcat(snappath,tg->Mainloop.scene_suff);
									}
									strcat(snappath,"_");
								}
								strcat(snappath,folder);
								strcat(snappath,"_");
								strcat(snappath,snapfile);
								strcat(snappath,suff); //".bmp");
								fwl_set_SnapFile(snappath);
							}
							if(namingMethod == 3){
								//group folder
								//if snapshot 'x' is on frame 17, and fixture,
								//   then fixture/1_wrl_17.snap or .bmp
								char snappath[100];
								int j, k;
								strcpy(snappath,folder);
								fw_mkdir(snappath); // /fixture
								strcat(snappath,"/");
								strcat(snappath,tg->Mainloop.scene_name); // /fixture/1
								k = strlen(tg->Mainloop.scene_name);
								if(k){
									j= strlen(tg->Mainloop.scene_suff);
									if(j){
										strcat(snappath,"_");
										strcat(snappath,tg->Mainloop.scene_suff);
									}
									strcat(snappath,"_");
								}
								strcat(snappath,snapfile);
								strcat(snappath,suff); //".bmp");
								fwl_set_SnapFile(snappath);  //  /fixture/1_wrl_17.bmp
							}
							if(namingMethod == 4){
								//group folder
								//if snapshot 'x' is the first one .0001, and fixture,
								//   then fixture/1_wrl.0001.rgb or .bmp
								char snappath[100];
								char *sep = "_"; // "." or "_" or "/"
								set_snapshotModeTesting(TRUE);
								//if(isSnapshotModeTesting())
								//	printf("testing\n");
								//else
								//	printf("not testing\n");
								strcpy(snappath,folder);
								fw_mkdir(snappath); // /fixture
								fwl_set_SnapTmp(snappath);

								snappath[0] = '\0';
								if(p->nameTest){
									strcat(snappath,p->nameTest);
								}else{
									if(tg->Mainloop.scene_name){
										strcat(snappath,tg->Mainloop.scene_name); // /fixture/1
										if(tg->Mainloop.scene_suff)
										{
											strcat(snappath,sep); // "." or "_");
											strcat(snappath,tg->Mainloop.scene_suff);
										}
									}
								}
								fwl_set_SnapFile(snappath);  //  /fixture/1_wrl.001.bmp

							}
						}
					}
					fwl_do_keyPress0(key, type);
				}
			}
			if(strlen(mouseStr)>2){
				int i,ii,len;
				int mev;
				unsigned int button;
				float x,y;
				len = strlen(mouseStr);
				ii=1;
				do{
					for(i=ii;i<len;i++)
						if(mouseStr[i] == ';') break;
					if(0){
					sscanf(&mouseStr[ii],"%d,%d,%f,%f;",&mev,&button,&x,&y);
					handle0(mev,button,x,y);
					}
					if(1){
					sscanf(&mouseStr[ii],"%d,%d,%d,%d,%d;",&mev,&button,&ix,&iy,&ID);
					fwl_handle_aqua_multi0(mev,button,ix,iy,ID);
					}
					//printf("%d,%d,%f,%f;",mev,button,x,y);
					ii=i+1;
				}while(ii<len-1);
			}
		}
	}
	fwl_RenderSceneUpdateScene0(dtime);
}
void fwl_RenderSceneUpdateScene0(double dtime) {

#else //FRONTEND_DOES_SNAPSHOTS

void fwl_RenderSceneUpdateScene() {
		double dtime = Time1970sec();

#endif //FRONTEND_DOES_SNAPSHOTS

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
        } else {
		/* NOTE: front ends now sync with the monitor, meaning, this sleep is no longer needed unless
		   something goes totally wrong */
#ifndef FRONTEND_HANDLES_DISPLAY_THREAD
			if(0) if(!tg->display.params.frontend_handles_display_thread){
				/* we see how long it took to do the last loop; now that the frame rate is synced to the
				   vertical retrace of the screens, we should not get more than 60-70fps. We calculate the
				   time here, if it is more than 200fps, we sleep for 1/100th of a second - we should NOT
				   need this, but in case something goes pear-shaped (british expression, there!) we do not
				   consume thousands of frames per second */

				p->waitsec = TickTime() - lastTime();
				if (p->waitsec < 0.005) {
					usleep(10000);
				}
			}
#endif /* FRONTEND_HANDLES_DISPLAY_THREAD */
        }

        /* Set the timestamp */
		tg->Mainloop.lastTime = tg->Mainloop.TickTime;
		tg->Mainloop.TickTime = dtime; //Time1970sec();

        /* any scripts to do?? */
#ifdef _MSC_VER
		if(p->doEvents)
#endif /* _MSC_VER */

	#ifdef HAVE_JAVASCRIPT
		initializeAnyScripts();
	#endif


        /* BrowserAction required? eg, anchors, etc */
        if (tg->RenderFuncs.BrowserAction) {
                tg->RenderFuncs.BrowserAction = doBrowserAction ();
        }

        /* has the default background changed? */
        if (tg->OpenGL_Utils.cc_changed) doglClearColor();

        OcclusionStartofRenderSceneUpdateScene();
        startOfLoopNodeUpdates();

		if (p->loop_count == 25) {
                tg->Mainloop.BrowserFPS = 25.0 / (TickTime()-p->BrowserStartTime);
                setMenuFps((float)tg->Mainloop.BrowserFPS); /*  tell status bar to refresh, if it is displayed*/
                /* printf ("fps %f tris %d, rootnode children %d \n",p->BrowserFPS,p->trisThisLoop, X3D_GROUP(rootNode)->children.n);  */

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

        /* handle any events provided on the command line - Robert Sim */
        if (p->keypress_string && p->doEvents) {
                if (p->keypress_wait_for_settle > 0) {
                        p->keypress_wait_for_settle--;
                } else {
                        /* dont do the null... */
                        if (*p->keypress_string) {
                                /* printf ("handling key %c\n",*p->keypress_string); */
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
		TRACE_MSG("mouse motion event: win=%u, state=%d\n",
			  mev->window, mev->state);
		break;
	    case ButtonPress:
	    case ButtonRelease:
		bev = &event.xbutton;
		TRACE_MSG("mouse button event: win=%u, state=%d\n",
			  bev->window, bev->state);
		break;
	    }
#endif /* XEVENT_VERBOSE */

	    DEBUG_XEV("EVENT through XtDispatchEvent\n");
	    XtDispatchEvent (&event);
	}

#endif /* TARGET_MOTIF */
#endif /* KEEP_X11_INLIB */

//#if defined(_MSC_VER)
//	/**
//	 *   Win32 event loop
//	 *   gives windows message handler a time slice and
//	 *   it calls fwl_handle_aqua and do_keypress from fwWindow32.c
//	 */
//	doEventsWin32A();
//#endif /* _MSC_VER */

        /* Viewer move viewpoint */
        handle_tick();

    PRINT_GL_ERROR_IF_ANY("after handle_tick")

        /* setup Projection and activate ProximitySensors */
        if (p->onScreen)
		{
			render_pre();
			slerp_viewpoint();
		}

#ifdef RENDERVERBOSE
    printf("RENDER STEP----------\n");
#endif

        /* first events (clock ticks, etc) if we have other things to do, yield */
        if (p->doEvents) do_first (); //else sched_yield();

	/* ensure depth mask turned on here */
	FW_GL_DEPTHMASK(GL_TRUE);

    PRINT_GL_ERROR_IF_ANY("after depth")
        /* actual rendering */
        if (p->onScreen) {
		render();
	}

        /* handle_mouse events if clicked on a sensitive node */
	 //printf("nav mode =%d sensitive= %d\n",p->NavigationMode, tg->Mainloop.HaveSensitive);
        if (!p->NavigationMode && tg->Mainloop.HaveSensitive) {
                p->currentCursor = 0;
                setup_projection(TRUE,tg->Mainloop.currentX[p->currentCursor],tg->Mainloop.currentY[p->currentCursor]);
                setup_viewpoint();
                render_hier(rootNode(),VF_Sensitive  | VF_Geom);
                p->CursorOverSensitive = getRayHit();

                /* for nodes that use an "isOver" eventOut... */
                if (p->lastOver != p->CursorOverSensitive) {
                        #ifdef VERBOSE
			  printf ("%lf over changed, p->lastOver %u p->cursorOverSensitive %u, p->butDown1 %d\n",
				TickTime(), (unsigned int) p->lastOver, (unsigned int) p->CursorOverSensitive,
				p->ButDown[p->currentCursor][1]);
                        #endif

                        if (p->ButDown[p->currentCursor][1]==0) {

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
			printf("COS %d (%s)\n",
			       (unsigned int) p->CursorOverSensitive,
			       stringNodeType(p->CursorOverSensitive->_nodeType));
                #endif /* VERBOSE */

                /* did we have a click of button 1? */

                if (p->ButDown[p->currentCursor][1] && (p->lastPressedOver==NULL)) {
                        /* printf ("Not Navigation and 1 down\n"); */
                        /* send an event of ButtonPress and isOver=true */
                        p->lastPressedOver = p->CursorOverSensitive;
                        sendSensorEvents(p->lastPressedOver, ButtonPress, p->ButDown[p->currentCursor][1], TRUE);
                }

                if ((p->ButDown[p->currentCursor][1]==0) && p->lastPressedOver!=NULL) {
                        /* printf ("Not Navigation and 1 up\n");  */
                        /* send an event of ButtonRelease and isOver=true;
                           an isOver=false event will be sent below if required */
                        sendSensorEvents(p->lastPressedOver, ButtonRelease, p->ButDown[p->currentCursor][1], TRUE);
                        p->lastPressedOver = NULL;
                }

                if (p->lastMouseEvent == MotionNotify) {
                        /* printf ("Not Navigation and motion - going into sendSensorEvents\n"); */
                        /* TouchSensor hitPoint_changed needs to know if we are over a sensitive node or not */
                        sendSensorEvents(p->CursorOverSensitive,MotionNotify, p->ButDown[p->currentCursor][1], TRUE);

                        /* PlaneSensors, etc, take the last sensitive node pressed over, and a mouse movement */
                        sendSensorEvents(p->lastPressedOver,MotionNotify, p->ButDown[p->currentCursor][1], TRUE);
                	p->lastMouseEvent = 0 ;
                }



                /* do we need to re-define cursor style?        */
                /* do we need to send an isOver event?          */
                if (p->CursorOverSensitive!= NULL) {
					setSensorCursor();

                        /* is this a new node that we are now over?
                           don't change the node pointer if we are clicked down */
                        if ((p->lastPressedOver==NULL) && (p->CursorOverSensitive != p->oldCOS)) {
                                sendSensorEvents(p->oldCOS,MapNotify,p->ButDown[p->currentCursor][1], FALSE);
                                sendSensorEvents(p->CursorOverSensitive,MapNotify,p->ButDown[p->currentCursor][1], TRUE);
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
                        if ((p->oldCOS!=NULL)  && (p->ButDown[p->currentCursor][1]==0)) {
                                sendSensorEvents(p->oldCOS,MapNotify,p->ButDown[p->currentCursor][1], FALSE);
                                /* remove any display on-screen */
                                sendDescriptionToStatusBar(NULL);
                                p->oldCOS=NULL;
                        }
                }

        } /* (!NavigationMode && HaveSensitive) */
		else
			setArrowCursor();


	#if !defined(FRONTEND_DOES_SNAPSHOTS)
        /* handle snapshots */
        if (tg->Snapshot.doSnapshot) {
                Snapshot();
        }
	#endif //FRONTEND_DOES_SNAPSHOTS

        /* do OcclusionCulling, etc */
        OcclusionCulling();

        if (p->doEvents) {
                /* and just parsed nodes needing binding? */
                SEND_BIND_IF_REQUIRED(tg->ProdCon.setViewpointBindInRender)
                SEND_BIND_IF_REQUIRED(tg->ProdCon.setFogBindInRender)
                SEND_BIND_IF_REQUIRED(tg->ProdCon.setBackgroundBindInRender)
                SEND_BIND_IF_REQUIRED(tg->ProdCon.setNavigationBindInRender)


                /* handle ROUTES - at least the ones not generated in do_first() */
                propagate_events();

                /* Javascript events processed */
                process_eventsProcessed();

		#if !defined(EXCLUDE_EAI)
		/*
		 * Actions are now separate so that file IO is not tightly coupled
		 * via shared buffers and file descriptors etc. 'The core' now calls
		 * the fwlio_SCK* funcs to get data into the system, and calls the fwl_EAI*
		 * funcs to give the data to the EAI,nd the fwl_MIDI* funcs for MIDI
		 *
		 * Although the MIDI code and the EAI code are basically the same
		 * and one could compress them into a loop, for the moment keep
		 * them seperate to serve as a example for any extensions...
		 */

                /* handle_EAI(); */
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
						/*
						 * Every incoming command has a reply,
						 * and the reply is synchronous.
						 */
						replyData = fwl_EAI_handleBuffer(tempEAIdata);
						free(tempEAIdata) ;
						EAI_StillToDo = 1;
						do {
							if(replyData != NULL && strlen(replyData) != 0) {
								fwlio_RxTx_sendbuffer(__FILE__,__LINE__,CHANNEL_EAI, replyData) ;
								free(replyData) ;
								/*
								 * Note: fwlio_RxTx_sendbuffer() can also be called async
								 * due to a listener trigger within routing, but it is
								 * is up to that caller to clean out its own buffers.
								 */
							}
							EAI_StillToDo = fwl_EAI_allDone();
							if(EAI_StillToDo) {
								if ( socketVerbose != 0 ) {
									printf("%s:%d Something still in EAI buffer? %d\n",__FILE__,__LINE__,EAI_StillToDo ) ;
								}
								replyData = fwl_EAI_handleRest();
							}
						} while(EAI_StillToDo) ;
					}
				}
			}
#ifdef OLDCODE
OLDCODE			/* handle_MIDI(); */
OLDCODE			//socketVerbose = fwlio_RxTx_control(CHANNEL_MIDI, RxTx_GET_VERBOSITY)  ;
OLDCODE			if(fwlio_RxTx_control(CHANNEL_MIDI, RxTx_REFRESH) == 0) {
OLDCODE				/* Nothing to be done, maybe not even running */
OLDCODE				if ( socketVerbose > 1 ) {
OLDCODE					printf("%s:%d Nothing to be done\n",__FILE__,__LINE__) ;
OLDCODE				}
OLDCODE			} else {
OLDCODE				if ( socketVerbose > 1 ) {
OLDCODE					printf("%s:%d Test RxTx_PENDING\n",__FILE__,__LINE__) ;
OLDCODE				}
OLDCODE				if(fwlio_RxTx_control(CHANNEL_MIDI, RxTx_PENDING) > 0) {
OLDCODE					char *tempMIDIdata;
OLDCODE					if ( socketVerbose != 0 ) {
OLDCODE						printf("%s:%d Something pending\n",__FILE__,__LINE__) ;
OLDCODE					}
OLDCODE					tempMIDIdata = fwlio_RxTx_getbuffer(CHANNEL_MIDI) ;
OLDCODE					if(tempMIDIdata != (char *)NULL) {
OLDCODE						char * replyData;
OLDCODE						int EAI_StillToDo;
OLDCODE						if ( socketVerbose != 0 ) {
OLDCODE							printf("%s:%d Something for MIDI to do with buffer addr %p\n",__FILE__,__LINE__,tempMIDIdata ) ;
OLDCODE						}
OLDCODE						replyData = fwl_MIDI_handleBuffer(tempMIDIdata);
OLDCODE						free(tempMIDIdata) ;
OLDCODE						EAI_StillToDo = 1;
OLDCODE						do {
OLDCODE							if(replyData != NULL && strlen(replyData) != 0) {
OLDCODE								fwlio_RxTx_sendbuffer(__FILE__,__LINE__,CHANNEL_MIDI, replyData) ;
OLDCODE								free(replyData) ;
OLDCODE							}
OLDCODE							EAI_StillToDo = fwl_EAI_allDone();
OLDCODE							if(EAI_StillToDo) {
OLDCODE								if ( socketVerbose != 0 ) {
OLDCODE									printf("%s:%d Something still in EAI buffer? %d\n",__FILE__,__LINE__,EAI_StillToDo ) ;
OLDCODE								}
OLDCODE								replyData = fwl_EAI_handleRest();
OLDCODE							}
OLDCODE						} while(EAI_StillToDo) ;
OLDCODE					}
OLDCODE				}
OLDCODE			}
#endif //OLDCODE
		}
		}
  		#endif //EXCLUDE_EAI
          }
  }
void queueMouseMulti(ppMainloop p, const int mev, const unsigned int button, const int ix, const int iy, int ID){
	if(p->mouseQueueCount < 50){
		p->mouseQueue[p->mouseQueueCount].mev = mev;
		p->mouseQueue[p->mouseQueueCount].button = button;
		p->mouseQueue[p->mouseQueueCount].ix = ix;
		p->mouseQueue[p->mouseQueueCount].iy = iy;
		p->mouseQueue[p->mouseQueueCount].ID = ID;
		p->mouseQueueCount++;
	}
}
void queueMouse(ppMainloop p, const int mev, const unsigned int button, const float x, const float y){
	if(p->mouseQueueCount < 50){
		p->mouseQueue[p->mouseQueueCount].mev = mev;
		p->mouseQueue[p->mouseQueueCount].button = button;
		p->mouseQueue[p->mouseQueueCount].x = x;
		p->mouseQueue[p->mouseQueueCount].y = y;
		p->mouseQueueCount++;
	}
}

void handle(const int mev, const unsigned int button, const float x, const float y)
{
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

	if(0)
	if(p->modeRecord || p->modeFixture || p->modePlayback){
		if(p->modeRecord){
			queueMouse(p,mev,button,x,y);
		}
		//else ignor so test isn't ruined by random mouse movement during playback
		return;
	}
	handle0(mev, button, x, y);
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

	int count;
	int actionKey;
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
#ifdef STATUSBAR_HUD
			statusbar_set_window_size(event.xconfigure.width,event.xconfigure.height);
#else
			fwl_setScreenDim (event.xconfigure.width,event.xconfigure.height);
#endif
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
			statusbar_handle_mouse(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y);
#else
			fwl_handle_aqua(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y);
#endif
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
			statusbar_handle_mouse(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y);
#else
			fwl_handle_aqua(event.type,event.xbutton.button,event.xbutton.x,event.xbutton.y);
#endif
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

static void render_pre() {
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;

        /* 1. Set up projection */
        setup_projection(FALSE,0,0);


        /* 2. Headlight, initialized here where we have the modelview matrix to Identity.
        FIXME: position of light sould actually be offset a little (towards the center)
        when in stereo mode. */

        if (fwl_get_headlight()) {
		setLightState(HEADLIGHT_LIGHT,TRUE);
		setLightType(HEADLIGHT_LIGHT,2); // DirectionalLight
	}


        /* 3. Viewpoint */
        setup_viewpoint();      /*  need this to render collisions correctly*/

        /* 4. Collisions */
        if (fwl_getCollision() == 1) {
			profile_start("collision");
                render_collisions(Viewer()->type);
				profile_end("collision");
                setup_viewpoint(); /*  update viewer position after collision, to*/
                                   /*  give accurate info to Proximity sensors.*/
        }

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
void setup_projection(int pick, int x, int y)
{
	GLDOUBLE fieldofview2;
	GLint xvp = 0;
	GLint scissorxl,scissorxr;
	ppMainloop p;
	X3D_Viewer *viewer;
	ttglobal tg = gglobal();
	GLsizei screenwidth2 = tg->display.screenWidth;
	GLsizei screenheight, bottom, top;
	GLDOUBLE aspect2 = tg->display.screenRatio;
	p = (ppMainloop)tg->Mainloop.prv;
	viewer = Viewer();

	PRINT_GL_ERROR_IF_ANY("XEvents::start of setup_projection");

	scissorxl = 0;
	scissorxr = screenwidth2;
	fieldofview2 = viewer->fieldofview;
	bottom = tg->Mainloop.clipPlane;
	top = 0;
	screenheight = tg->display.screenHeight - bottom;
	if(viewer->type==VIEWER_YAWPITCHZOOM)
		fieldofview2*=viewer->fovZoom;
	if(viewer->isStereo)
	{
		GLint xl,xr;
		xl = 0;
		xr = screenwidth2;

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
			scissorxr = screenwidth2/2;
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
			screenheight = tg->display.screenHeight;
			screenheight /= 2;
			if (viewer->iside == 0){
				bottom += screenheight;
			}else{
				top += screenheight;
			}
			screenheight -= tg->Mainloop.clipPlane;
			scissorxl = xl;
			scissorxr = xr;
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
	p->viewpointScreenX[viewer->iside] = xvp + screenwidth2/2;
	p->viewpointScreenY[viewer->iside] = top;
	if (viewer->updown){
        FW_GL_VIEWPORT(xvp - screenwidth2 / 2, bottom, screenwidth2 * 2, screenheight);
    }
    else{
        FW_GL_VIEWPORT(xvp, bottom, screenwidth2, screenheight);
    }

	FW_GL_LOAD_IDENTITY();
	if(pick) {
		/* picking for mouse events */
		FW_GL_GETINTEGERV(GL_VIEWPORT,p->viewPort2);
		//FW_GLU_PICK_MATRIX((float)x,(float)p->viewPort2[3]-y + bottom, (float)100,(float)100,p->viewPort2);
		FW_GLU_PICK_MATRIX((float)x,(float)p->viewPort2[3]  -y + bottom +top, (float)100,(float)100,p->viewPort2);
	}

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

/* Render the scene */
static void render()
{
//#if defined(FREEWRL_SHUTTER_GLASSES) || defined(FREEWRL_STEREO_RENDERING)
    int count;
	static double shuttertime;
	static int shutterside;
//#endif

	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

//#if defined(FREEWRL_SHUTTER_GLASSES) || defined(FREEWRL_STEREO_RENDERING)
	/*  profile*/
    /* double xx,yy,zz,aa,bb,cc,dd,ee,ff;*/

	for (count = 0; count < p->maxbuffers; count++) {

        /*set_buffer((unsigned)bufferarray[count],count); */              /*  in Viewer.c*/

		Viewer()->buffer = (unsigned)p->bufferarray[count];
		Viewer()->iside = count;
#ifdef OLDCODE
OLDCODE#ifdef HAVE_GLEW_H //#ifndef GLES2
OLDCODE		FW_GL_DRAWBUFFER((unsigned)p->bufferarray[count]);
OLDCODE#endif
#endif //OLDCODE

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
			if(Viewer()->anaglyph) //haveAnaglyphShader)
			{
				//set the channels for backbuffer clearing
				if(count == 0)
					Viewer_anaglyph_clearSides(); //clear all channels
				else
					Viewer_anaglyph_setSide(count); //clear just the channels we're going to draw to
			}
			setup_projection(0, 0, 0); //scissor test in here
			BackEndClearBuffer(2);
			if(Viewer()->anaglyph)
				Viewer_anaglyph_setSide(count); //set the channels for scenegraph drawing
			setup_viewpoint();
		}
		else
			BackEndClearBuffer(2);
		//BackEndLightsOff();
		clearLightTable();//turns all lights off- will turn them on for VF_globalLight and scope-wise for non-global in VF_geom

//#else
//
//	BackEndClearBuffer(2); // no stereo, no shutter glasses: simple clear
//
//#endif // SHUTTER GLASSES or STEREO

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

//#if defined(FREEWRL_SHUTTER_GLASSES) || defined(FREEWRL_STEREO_RENDERING)
		if (Viewer()->isStereo) {

			if (Viewer()->sidebyside){
				//cursorDraw(1, p->viewpointScreenX[count], p->viewpointScreenY[count], 0.0f); //draw a fiducial mark where centre of viewpoint is
				fiducialDraw(1,p->viewpointScreenX[count],p->viewpointScreenY[count],0.0f); //draw a fiducial mark where centre of viewpoint is
			}
			if (Viewer()->anaglyph)
				glColorMask(1,1,1,1); /*restore, for statusbarHud etc*/
		}
		glDisable(GL_SCISSOR_TEST);
	} /* for loop */

	if (Viewer()->isStereo) {
		Viewer()->iside = Viewer()->dominantEye; /*is used later in picking to set the cursor pick box on the (left=0 or right=1) viewport*/
	}

//#endif

	if(p->EMULATE_MULTITOUCH) {
        int i;

		for(i=0;i<20;i++)
			if(p->touchlist[i].isDown > 0)
				cursorDraw(p->touchlist[i].ID,p->touchlist[i].x,p->touchlist[i].y,p->touchlist[i].angle);
    }
}

static int currentViewerLandPort = 0;
static int rotatingCCW = FALSE;
static double currentViewerAngle = 0.0;
static double requestedViewerAngle = 0.0;

static void setup_viewpoint() {


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




        viewer_togl(Viewer()->fieldofview);
		profile_start("vp_hier");
        render_hier(rootNode(), VF_Viewpoint);
		profile_end("vp_hier");
        PRINT_GL_ERROR_IF_ANY("XEvents::setup_viewpoint");

	/*
	{ GLDOUBLE projMatrix[16];
	fw_glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	printf ("\n");
	printf ("setup_viewpoint, proj  %lf %lf %lf\n",projMatrix[12],projMatrix[13],projMatrix[14]);
	fw_glGetDoublev(GL_MODELVIEW_MATRIX, projMatrix);
	printf ("setup_viewpoint, model %lf %lf %lf\n",projMatrix[12],projMatrix[13],projMatrix[14]);
	printf ("setup_viewpoint, currentPos %lf %lf %lf\n",        Viewer.currentPosInModel.x,
	        Viewer.currentPosInModel.y ,
	        Viewer.currentPosInModel.z);
	}
	*/


}
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
			if(p->modePlayback || p->modeFixture){
				if(p->modePlayback)
					strcat(logfilename,"playback");
				else
					strcat(logfilename,"fixture");
				fw_mkdir(logfilename);
				strcat(logfilename,"/");
				if(p->nameTest){
					//  /fixture/test1.log
					strcat(logfilename,p->nameTest);
				}else if(tg->Mainloop.scene_name){
					//  /fixture/1_wrl.log
					strcat(logfilename,tg->Mainloop.scene_name);
					if(tg->Mainloop.scene_suff){
						strcat(logfilename,"_");
						strcat(logfilename,tg->Mainloop.scene_suff);
					}
				}
			}else{
				strcat(logfilename,"freewrl_tmp");
				fw_mkdir(logfilename);
				strcat(logfilename,"/");
				strcat(logfilename,"logfile");
			}
			strcat(logfilename,".log");
			p->logfname = strdup(logfilename);
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
		p->logfname = strdup(lname);
		toggleLogfile();
	 //   printf ("FreeWRL: redirect stdout and stderr to %s\n", logFileName);
	 //   fp = freopen(logFileName, "a", stdout);
	 //   if (NULL == fp) {
		//WARN_MSG("WARNING: Unable to reopen stdout to %s\n", logFileName) ;
	 //   }
	 //   fp = freopen(logFileName, "a", stderr);
	 //   if (NULL == fp) {
		//WARN_MSG("WARNING: Unable to reopen stderr to %s\n", logFileName) ;
	 //   }
	}

}

#define Boolean int

/* Return DEFed name from its node, or NULL if not found */
int isNodeDEFedYet(struct X3D_Node *node, Stack *DEFedNodes)
{
	int ind;
	if(DEFedNodes == NULL) return 0;
	for (ind=0; ind < DEFedNodes->n; ind++) {
		/* did we find this index? */
		if (vector_get(struct X3D_Node*, DEFedNodes, ind) == node) {
			return 1;
		}
	}
	return 0;
}

char * dontRecurseList [] = {
	"_sortedChildren",
	NULL,
};
int doRecurse(const char *fieldname){
	int dont, j;
	dont = 0;
	j=0;
	while(dontRecurseList[j] != NULL)
	{
		dont = dont || !strcmp(dontRecurseList[j],fieldname);
		j++;
	}
	return dont == 0 ? 1 : 0;
}
void print_field_value(FILE *fp, int typeIndex, union anyVrml* value)
{
	int i;
	switch(typeIndex)
	{
		case FIELDTYPE_FreeWRLPTR:
		{
			fprintf(fp," %p \n",(void *)value);
			break;
		}
		case FIELDTYPE_SFNode:
		{
			fprintf(fp," %p \n",(void *)value);
			break;
		}
		case FIELDTYPE_MFNode:
		{
			int j;
			struct Multi_Node* mfnode;
			mfnode = (struct Multi_Node*)value;
			fprintf(fp,"{ ");
			for(j=0;j<mfnode->n;j++)
				fprintf(fp," %p, ",mfnode->p[j]);
			break;
		}
		case FIELDTYPE_SFString:
		{
			struct Uni_String** sfstring = (struct Uni_String**)value;
			fprintf (fp," %s ",(*sfstring)->strptr);
			break;
		}
		case FIELDTYPE_MFString:
		{
			struct Multi_String* mfstring = (struct Multi_String*)value;
			fprintf (fp," { ");
			for (i=0; i<mfstring->n; i++) { fprintf (fp,"%s, ",mfstring->p[i]->strptr); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFFloat:
		{
			float *flt = (float*)value;
			fprintf(fp," %4.3f ",*flt);
			break;
		}
		case FIELDTYPE_MFFloat:
		{
			struct Multi_Float *mffloat = (struct Multi_Float*)value;
			fprintf (fp,"{ ");
			for (i=0; i<mffloat->n; i++) { fprintf (fp," %4.3f,",mffloat->p[i]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFDouble:
		{
			double *sftime = (double*)value;
			fprintf (fp,"%4.3f",*sftime);
			break;
		}
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFDouble:
		{
			struct Multi_Double *mfdouble = (struct Multi_Double*)value;
			fprintf (fp,"{");
			for (i=0; i<mfdouble->n; i++) { fprintf (fp," %4.3f,",mfdouble->p[i]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFBool:
		{
			int *sfint32 = (int*)(value);
			fprintf (fp," \t%d\n",*sfint32);
			break;
		}
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFBool:
		{
			struct Multi_Int32 *mfint32 = (struct Multi_Int32*)value;
			fprintf (fp,"{");
			for (i=0; i<mfint32->n; i++) { fprintf (fp," %d,",mfint32->p[i]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec2f:
		{
			struct SFVec2f * sfvec2f = (struct SFVec2f *)value;
            for (i=0; i<2; i++) { fprintf (fp,"%4.3f  ",sfvec2f->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec2f:
		{
			struct Multi_Vec2f *mfvec2f = (struct Multi_Vec2f*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec2f->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f],",mfvec2f->p[i].c[0], mfvec2f->p[i].c[1]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec2d:
		{
			struct SFVec2d * sfvec2d = (struct SFVec2d *)value;
			for (i=0; i<2; i++) { fprintf (fp,"%4.3f,  ",sfvec2d->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec2d:
		{
			struct Multi_Vec2d *mfvec2d = (struct Multi_Vec2d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec2d->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f], ",mfvec2d->p[i].c[0], mfvec2d->p[i].c[1]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:
		{
			struct SFVec3f * sfvec3f = (struct SFVec3f *)value;
			for (i=0; i<3; i++) { fprintf (fp,"%4.3f  ",sfvec3f->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFColor:
		{
			struct Multi_Vec3f *mfvec3f = (struct Multi_Vec3f*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec3f->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f, %4.3f],",mfvec3f->p[i].c[0], mfvec3f->p[i].c[1],mfvec3f->p[i].c[2]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec3d:
		{
			struct SFVec3d * sfvec3d = (struct SFVec3d *)value;
			for (i=0; i<3; i++) { fprintf (fp,"%4.3f  ",sfvec3d->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec3d:
		{
			struct Multi_Vec3d *mfvec3d = (struct Multi_Vec3d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec3d->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f, %4.3f],",mfvec3d->p[i].c[0], mfvec3d->p[i].c[1],mfvec3d->p[i].c[2]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec4f:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:
		{
			struct SFRotation * sfrot = (struct SFRotation *)value;
			for (i=0; i<4; i++) { fprintf (fp,"%4.3f  ",sfrot->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec4f:
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFRotation:
		{
			struct Multi_ColorRGBA *mfrgba = (struct Multi_ColorRGBA*)value;
			fprintf (fp,"{");
			for (i=0; i<mfrgba->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f]\n",mfrgba->p[i].c[0], mfrgba->p[i].c[1],mfrgba->p[i].c[2],mfrgba->p[i].c[3]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec4d:
		{
			struct SFVec4d * sfvec4d = (struct SFVec4d *)value;
			for (i=0; i<4; i++) { fprintf (fp,"%4.3f  ",sfvec4d->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec4d:
		{
			struct Multi_Vec4d *mfvec4d = (struct Multi_Vec4d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec4d->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f],",mfvec4d->p[i].c[0], mfvec4d->p[i].c[1],mfvec4d->p[i].c[2],mfvec4d->p[i].c[3]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFMatrix3f:
		{
			struct SFMatrix3f *sfmat3f = (struct SFMatrix3f*)value;
			fprintf (fp," [%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat3f->c[0],sfmat3f->c[1],sfmat3f->c[2],
			sfmat3f->c[3],sfmat3f->c[4],sfmat3f->c[5],
			sfmat3f->c[6],sfmat3f->c[7],sfmat3f->c[8]);
			break;
		}
		case FIELDTYPE_MFMatrix3f:
		{
			struct Multi_Matrix3f *mfmat3f = (struct Multi_Matrix3f*)value;
			fprintf (fp,"{");
			for (i=0; i<mfmat3f->n; i++) {
				fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ],",
				mfmat3f->p[i].c[0],mfmat3f->p[i].c[1],mfmat3f->p[i].c[2],
				mfmat3f->p[i].c[3],mfmat3f->p[i].c[4],mfmat3f->p[i].c[5],
				mfmat3f->p[i].c[6],mfmat3f->p[i].c[7],mfmat3f->p[i].c[8]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFMatrix3d:
		{
			struct SFMatrix3d *sfmat3d = (struct SFMatrix3d*)value;
			fprintf (fp," [%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]",
			sfmat3d->c[0],sfmat3d->c[1],sfmat3d->c[2],
			sfmat3d->c[3],sfmat3d->c[4],sfmat3d->c[5],
			sfmat3d->c[6],sfmat3d->c[7],sfmat3d->c[8]);
			break;
		}
		case FIELDTYPE_MFMatrix3d:
		{
			struct Multi_Matrix3d *mfmat3d = (struct Multi_Matrix3d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfmat3d->n; i++) {
				fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat3d->p[i].c[0],mfmat3d->p[i].c[1],mfmat3d->p[i].c[2],
				mfmat3d->p[i].c[3],mfmat3d->p[i].c[4],mfmat3d->p[i].c[5],
				mfmat3d->p[i].c[6],mfmat3d->p[i].c[7],mfmat3d->p[i].c[8]); }
			break;
		}
		case FIELDTYPE_SFMatrix4f:
		{
			struct SFMatrix4f *sfmat4f = (struct SFMatrix4f*)value;
			fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat4f->c[0],sfmat4f->c[1],sfmat4f->c[2],sfmat4f->c[3],
			sfmat4f->c[4],sfmat4f->c[5],sfmat4f->c[6],sfmat4f->c[7],
			sfmat4f->c[8],sfmat4f->c[9],sfmat4f->c[10],sfmat4f->c[11],
			sfmat4f->c[12],sfmat4f->c[13],sfmat4f->c[14],sfmat4f->c[15]);
			break;
		}
		case FIELDTYPE_MFMatrix4f:
		{
			struct Multi_Matrix4f *mfmat4f = (struct Multi_Matrix4f*)value;
			fprintf (fp,"{");
			for (i=0; i<mfmat4f->n; i++) {
				fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ],",
				mfmat4f->p[i].c[0],mfmat4f->p[i].c[1],mfmat4f->p[i].c[2],mfmat4f->p[i].c[3],
				mfmat4f->p[i].c[4],mfmat4f->p[i].c[5],mfmat4f->p[i].c[6],mfmat4f->p[i].c[7],
				mfmat4f->p[i].c[8],mfmat4f->p[i].c[9],mfmat4f->p[i].c[10],mfmat4f->p[i].c[11],
				mfmat4f->p[i].c[12],mfmat4f->p[i].c[13],mfmat4f->p[i].c[14],mfmat4f->p[i].c[15]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFMatrix4d:
		{
			struct SFMatrix4d *sfmat4d = (struct SFMatrix4d*)value;
			fprintf (fp," [%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]",
			sfmat4d->c[0],sfmat4d->c[1],sfmat4d->c[2],sfmat4d->c[3],
			sfmat4d->c[4],sfmat4d->c[5],sfmat4d->c[6],sfmat4d->c[7],
			sfmat4d->c[8],sfmat4d->c[9],sfmat4d->c[10],sfmat4d->c[11],
			sfmat4d->c[12],sfmat4d->c[13],sfmat4d->c[14],sfmat4d->c[15]);
			break;
		}
		case FIELDTYPE_MFMatrix4d:	break;
		{
			struct Multi_Matrix4d *mfmat4d = (struct Multi_Matrix4d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfmat4d->n; i++) {
				fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ],",
				mfmat4d->p[i].c[0],mfmat4d->p[i].c[1],mfmat4d->p[i].c[2],mfmat4d->p[i].c[3],
				mfmat4d->p[i].c[4],mfmat4d->p[i].c[5],mfmat4d->p[i].c[6],mfmat4d->p[i].c[7],
				mfmat4d->p[i].c[8],mfmat4d->p[i].c[9],mfmat4d->p[i].c[10],mfmat4d->p[i].c[11],
				mfmat4d->p[i].c[12],mfmat4d->p[i].c[13],mfmat4d->p[i].c[14],mfmat4d->p[i].c[15]); }
			fprintf(fp,"}");
			break;
		}

		case FIELDTYPE_SFImage:
		{
			fprintf(fp," %p ",(void *)value); //no SFImage struct defined
			break;
		}
	}
} //return print_field
void dump_scene(FILE *fp, int level, struct X3D_Node* node);
void dump_scene2(FILE *fp, int level, struct X3D_Node* node, int recurse, Stack *DEFedNodes);
// print_field is used by dump_scene2() to pretty-print a single field.
// recurses into dump_scene2 for SFNode and MFNodes to print them in detail.
void print_field(FILE *fp,int level, int typeIndex, const char* fieldName, union anyVrml* value, Stack* DEFedNodes)
{
	int lc, i;
	#define spacer	for (lc=0; lc<level; lc++) fprintf (fp," ");

	switch(typeIndex)
	{
		case FIELDTYPE_FreeWRLPTR:
		{
			fprintf(fp," %p \n",(void *)value);
			break;
		}
		case FIELDTYPE_SFNode:
		{
			int dore;
			struct X3D_Node** sfnode = (struct X3D_Node**)value;
			dore = doRecurse(fieldName);
			fprintf (fp,":\n"); dump_scene2(fp,level+1,*sfnode,dore,DEFedNodes);
			break;
		}
		case FIELDTYPE_MFNode:
		{
			int j, dore;
			struct Multi_Node* mfnode;
			dore = doRecurse(fieldName);
			mfnode = (struct Multi_Node*)value;
			fprintf(fp,":\n");
			for(j=0;j<mfnode->n;j++)
				dump_scene2(fp,level+1,mfnode->p[j],dore,DEFedNodes);
			break;
		}
		case FIELDTYPE_SFString:
		{
			struct Uni_String** sfstring = (struct Uni_String**)value;
			fprintf (fp," \t%s\n",(*sfstring)->strptr);
			break;
		}
		case FIELDTYPE_MFString:
		{
			struct Multi_String* mfstring = (struct Multi_String*)value;
			fprintf (fp," : \n");
			for (i=0; i<mfstring->n; i++) { spacer fprintf (fp,"			%d: \t%s\n",i,mfstring->p[i]->strptr); }
			break;
		}
		case FIELDTYPE_SFFloat:
		{
			float *flt = (float*)value;
			fprintf (fp," \t%4.3f\n",*flt);
			break;
		}
		case FIELDTYPE_MFFloat:
		{
			struct Multi_Float *mffloat = (struct Multi_Float*)value;
			fprintf (fp," :\n");
			for (i=0; i<mffloat->n; i++) { spacer fprintf (fp,"			%d: \t%4.3f\n",i,mffloat->p[i]); }
			break;
		}
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFDouble:
		{
			double *sftime = (double*)value;
			fprintf (fp," \t%4.3f\n",*sftime);
			break;
		}
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFDouble:
		{
			struct Multi_Double *mfdouble = (struct Multi_Double*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfdouble->n; i++) { spacer fprintf (fp,"			%d: \t%4.3f\n",i,mfdouble->p[i]); }
			break;
		}
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFBool:
		{
			int *sfint32 = (int*)(value);
			fprintf (fp," \t%d\n",*sfint32);
			break;
		}
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFBool:
		{
			struct Multi_Int32 *mfint32 = (struct Multi_Int32*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfint32->n; i++) { spacer fprintf (fp,"			%d: \t%d\n",i,mfint32->p[i]); }
			break;
		}
		case FIELDTYPE_SFVec2f:
		{
			struct SFVec2f * sfvec2f = (struct SFVec2f *)value;
			fprintf (fp,": \t");
			for (i=0; i<2; i++) { fprintf (fp,"%4.3f  ",sfvec2f->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec2f:
		{
			struct Multi_Vec2f *mfvec2f = (struct Multi_Vec2f*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec2f->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f]\n",i,mfvec2f->p[i].c[0], mfvec2f->p[i].c[1]); }
			break;
		}
		case FIELDTYPE_SFVec2d:
		{
			struct SFVec2d * sfvec2d = (struct SFVec2d *)value;
			fprintf (fp,": \t");
			for (i=0; i<2; i++) { fprintf (fp,"%4.3f  ",sfvec2d->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec2d:
		{
			struct Multi_Vec2d *mfvec2d = (struct Multi_Vec2d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec2d->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f]\n",i,mfvec2d->p[i].c[0], mfvec2d->p[i].c[1]); }
			break;
		}
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:
		{
			struct SFVec3f * sfvec3f = (struct SFVec3f *)value;
			fprintf (fp,": \t");
			for (i=0; i<3; i++) { fprintf (fp,"%4.3f  ",sfvec3f->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFColor:
		{
			struct Multi_Vec3f *mfvec3f = (struct Multi_Vec3f*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec3f->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f]\n",i,mfvec3f->p[i].c[0], mfvec3f->p[i].c[1],mfvec3f->p[i].c[2]); }
			break;
		}
		case FIELDTYPE_SFVec3d:
		{
			struct SFVec3d * sfvec3d = (struct SFVec3d *)value;
			fprintf (fp,": \t");
			for (i=0; i<3; i++) { fprintf (fp,"%4.3f  ",sfvec3d->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec3d:
		{
			struct Multi_Vec3d *mfvec3d = (struct Multi_Vec3d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec3d->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f]\n",i,mfvec3d->p[i].c[0], mfvec3d->p[i].c[1],mfvec3d->p[i].c[2]); }
			break;
		}
		case FIELDTYPE_SFVec4f:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:
		{
			struct SFRotation * sfrot = (struct SFRotation *)value;
			fprintf (fp,": \t");
			for (i=0; i<4; i++) { fprintf (fp,"%4.3f  ",sfrot->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec4f:
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFRotation:
		{
			struct Multi_ColorRGBA *mfrgba = (struct Multi_ColorRGBA*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfrgba->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f]\n",i,mfrgba->p[i].c[0], mfrgba->p[i].c[1],mfrgba->p[i].c[2],mfrgba->p[i].c[3]); }
			break;
		}
		case FIELDTYPE_SFVec4d:
		{
			struct SFVec4d * sfvec4d = (struct SFVec4d *)value;
			fprintf (fp,": \t");
			for (i=0; i<4; i++) { fprintf (fp,"%4.3f  ",sfvec4d->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec4d:
		{
			struct Multi_Vec4d *mfvec4d = (struct Multi_Vec4d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec4d->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f]\n",i,mfvec4d->p[i].c[0], mfvec4d->p[i].c[1],mfvec4d->p[i].c[2],mfvec4d->p[i].c[3]); }
			break;
		}
		case FIELDTYPE_SFMatrix3f:
		{
			struct SFMatrix3f *sfmat3f = (struct SFMatrix3f*)value;
			spacer fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat3f->c[0],sfmat3f->c[1],sfmat3f->c[2],
			sfmat3f->c[3],sfmat3f->c[4],sfmat3f->c[5],
			sfmat3f->c[6],sfmat3f->c[7],sfmat3f->c[8]);
			break;
		}
		case FIELDTYPE_MFMatrix3f:
		{
			struct Multi_Matrix3f *mfmat3f = (struct Multi_Matrix3f*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfmat3f->n; i++) {
				spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat3f->p[i].c[0],mfmat3f->p[i].c[1],mfmat3f->p[i].c[2],
				mfmat3f->p[i].c[3],mfmat3f->p[i].c[4],mfmat3f->p[i].c[5],
				mfmat3f->p[i].c[6],mfmat3f->p[i].c[7],mfmat3f->p[i].c[8]); }
			break;
		}
		case FIELDTYPE_SFMatrix3d:
		{
			struct SFMatrix3d *sfmat3d = (struct SFMatrix3d*)value;
			spacer fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat3d->c[0],sfmat3d->c[1],sfmat3d->c[2],
			sfmat3d->c[3],sfmat3d->c[4],sfmat3d->c[5],
			sfmat3d->c[6],sfmat3d->c[7],sfmat3d->c[8]);
			break;
		}
		case FIELDTYPE_MFMatrix3d:
		{
			struct Multi_Matrix3d *mfmat3d = (struct Multi_Matrix3d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfmat3d->n; i++) {
				spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat3d->p[i].c[0],mfmat3d->p[i].c[1],mfmat3d->p[i].c[2],
				mfmat3d->p[i].c[3],mfmat3d->p[i].c[4],mfmat3d->p[i].c[5],
				mfmat3d->p[i].c[6],mfmat3d->p[i].c[7],mfmat3d->p[i].c[8]); }
			break;
		}
		case FIELDTYPE_SFMatrix4f:
		{
			struct SFMatrix4f *sfmat4f = (struct SFMatrix4f*)value;
			fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat4f->c[0],sfmat4f->c[1],sfmat4f->c[2],sfmat4f->c[3],
			sfmat4f->c[4],sfmat4f->c[5],sfmat4f->c[6],sfmat4f->c[7],
			sfmat4f->c[8],sfmat4f->c[9],sfmat4f->c[10],sfmat4f->c[11],
			sfmat4f->c[12],sfmat4f->c[13],sfmat4f->c[14],sfmat4f->c[15]);
			break;
		}
		case FIELDTYPE_MFMatrix4f:
		{
			struct Multi_Matrix4f *mfmat4f = (struct Multi_Matrix4f*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfmat4f->n; i++) {
				spacer
				fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat4f->p[i].c[0],mfmat4f->p[i].c[1],mfmat4f->p[i].c[2],mfmat4f->p[i].c[3],
				mfmat4f->p[i].c[4],mfmat4f->p[i].c[5],mfmat4f->p[i].c[6],mfmat4f->p[i].c[7],
				mfmat4f->p[i].c[8],mfmat4f->p[i].c[9],mfmat4f->p[i].c[10],mfmat4f->p[i].c[11],
				mfmat4f->p[i].c[12],mfmat4f->p[i].c[13],mfmat4f->p[i].c[14],mfmat4f->p[i].c[15]); }
			break;
		}
		case FIELDTYPE_SFMatrix4d:
		{
			struct SFMatrix4d *sfmat4d = (struct SFMatrix4d*)value;
			fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat4d->c[0],sfmat4d->c[1],sfmat4d->c[2],sfmat4d->c[3],
			sfmat4d->c[4],sfmat4d->c[5],sfmat4d->c[6],sfmat4d->c[7],
			sfmat4d->c[8],sfmat4d->c[9],sfmat4d->c[10],sfmat4d->c[11],
			sfmat4d->c[12],sfmat4d->c[13],sfmat4d->c[14],sfmat4d->c[15]);
			break;
		}
		case FIELDTYPE_MFMatrix4d:	break;
		{
			struct Multi_Matrix4d *mfmat4d = (struct Multi_Matrix4d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfmat4d->n; i++) {
				spacer
				fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat4d->p[i].c[0],mfmat4d->p[i].c[1],mfmat4d->p[i].c[2],mfmat4d->p[i].c[3],
				mfmat4d->p[i].c[4],mfmat4d->p[i].c[5],mfmat4d->p[i].c[6],mfmat4d->p[i].c[7],
				mfmat4d->p[i].c[8],mfmat4d->p[i].c[9],mfmat4d->p[i].c[10],mfmat4d->p[i].c[11],
				mfmat4d->p[i].c[12],mfmat4d->p[i].c[13],mfmat4d->p[i].c[14],mfmat4d->p[i].c[15]); }
			break;
		}

		case FIELDTYPE_SFImage:
		{
			fprintf(fp," %p \n",(void *)value); //no SFImage struct defined
			break;
		}
	}
} //return print_field

/*
dump_scene2() is like dump_scene() - a way to printf all the nodes and their fields,
when you hit a key on the keyboard ie '|'
and recurse if a field is an SFNode or MFNode, tabbing in and out to show the recursion level
- except dump_scene2 iterates over fields in a generic way to get all fields
- could be used as an example for deep copying binary nodes
- shows script/user fields and built-in fields
*/
void dump_scene2(FILE *fp, int level, struct X3D_Node* node, int recurse, Stack *DEFedNodes) {
	#define spacer	for (lc=0; lc<level; lc++) fprintf (fp," ");
	int lc;
	int isDefed;
	char *nodeName;
	//(int) FIELDNAMES_children, (int) offsetof (struct X3D_Group, children),  (int) FIELDTYPE_MFNode, (int) KW_inputOutput, (int) (SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33),
	typedef struct field_info{
		int nameIndex;
		int offset;
		int typeIndex;
		int ioType;
		int version;
	} *finfo;
	finfo offsets;
	finfo field;
	int ifield;

	#ifdef FW_DEBUG
		Boolean allFields;
		if (fileno(fp) == fileno(stdout)) { allFields = TRUE; } else { allFields = FALSE; }
	#else
		Boolean allFields = FALSE;
	#endif
	/* See vi +/double_conditional codegen/VRMLC.pm */
	if (node==NULL) return;

	fflush(fp);
	if (level == 0) fprintf (fp,"starting dump_scene2\n");
	nodeName = parser_getNameFromNode(node) ;
	isDefed = isNodeDEFedYet(node,DEFedNodes);
	spacer fprintf (fp,"L%d: node (%p) (",level,node);
	if(nodeName != NULL) {
		if(isDefed)
			fprintf(fp,"USE %s",nodeName);
		else
			fprintf(fp,"DEF %s",nodeName);
	}
	fprintf(fp,") type %s\n",stringNodeType(node->_nodeType));
	//fprintf(fp,"recurse=%d ",recurse);
	if(recurse && !isDefed)
	{
		vector_pushBack(struct X3D_Node*, DEFedNodes, node);
		offsets = (finfo)NODE_OFFSETS[node->_nodeType];
		ifield = 0;
		field = &offsets[ifield];
		while( field->nameIndex > -1) //<< generalized for scripts and builtins?
		{
			int privat;
			privat = FIELDNAMES[field->nameIndex][0] == '_';
			privat = privat && strcmp(FIELDNAMES[field->nameIndex],"__scriptObj");
			privat = privat && strcmp(FIELDNAMES[field->nameIndex],"__protoDef");
			if(allFields || !privat)
			{
				spacer
				fprintf(fp," %s",FIELDNAMES[field->nameIndex]); //[0]]);
				fprintf(fp," (%s)",FIELDTYPES[field->typeIndex]); //field[2]]);
				if(node->_nodeType == NODE_Script && !strcmp(FIELDNAMES[field->nameIndex],"__scriptObj") )
				{
					int k;
					struct Vector *sfields;
					struct ScriptFieldDecl *sfield;
					struct FieldDecl *fdecl;
					struct Shader_Script *sp;
					struct CRjsnameStruct *JSparamnames = getJSparamnames();

					sp = *(struct Shader_Script **)&((char*)node)[field->offset];
					fprintf(fp,"loaded = %d\n",sp->loaded);
					sfields = sp->fields;
					//fprintf(fp,"sp->fields->n = %d\n",sp->fields->n);
					for(k=0;k<sfields->n;k++)
					{
						char *fieldName;
						sfield = vector_get(struct ScriptFieldDecl *,sfields,k);
						//if(sfield->ASCIIvalue) printf("Ascii value=%s\n",sfield->ASCIIvalue);
						fdecl = sfield->fieldDecl;
						fieldName = fieldDecl_getShaderScriptName(fdecl);
						fprintf(fp,"  %s",fieldName);
						//fprintf(fp," (%s)",FIELDTYPES[field->typeIndex]); //field[2]]);
						fprintf(fp," (%s)", stringFieldtypeType(fdecl->fieldType)); //fdecl->fieldType)
						fprintf(fp," %s ",stringPROTOKeywordType(fdecl->PKWmode));

						if(fdecl->PKWmode == PKW_initializeOnly)
							print_field(fp,level,fdecl->fieldType,fieldName,&(sfield->value),DEFedNodes);
						else
							fprintf(fp,"\n");
					}
					level--;
				}
				else if(node->_nodeType == NODE_Proto && !strcmp(FIELDNAMES[field->nameIndex],"__protoDef") )
				{
					int k, mode;
					struct Vector* usernames[4];
					const char **userArr;
					struct ProtoFieldDecl* pfield;
					struct X3D_Proto* pnode = (struct X3D_Proto*)node;
					struct VRMLLexer* lexer;
					struct VRMLParser *globalParser;
					struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
					if(pstruct){
						globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;
						lexer = (struct VRMLLexer*)globalParser->lexer;
						usernames[0] = lexer->user_initializeOnly;
						usernames[1] = lexer->user_inputOnly;
						usernames[2] = lexer->user_outputOnly;
						usernames[3] = lexer->user_inputOutput;
						fprintf(fp," user fields:\n");
						level++;
						if(pstruct->iface)
						for(k=0; k!=vectorSize(pstruct->iface); ++k)
						{
							const char *fieldName;
							pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, k);
							mode = pfield->mode;
							#define X3DMODE(val)  ((val) % 4)
							userArr =&vector_get(const char*, usernames[X3DMODE(mode)], 0);
							fieldName = userArr[pfield->name];
							spacer
							fprintf(fp," %p ",(void*)pfield);
							fprintf(fp,"  %s",fieldName);
							fprintf(fp," (%s)", stringFieldtypeType(pfield->type)); //fdecl->fieldType)
							fprintf(fp," %s ",stringPROTOKeywordType(pfield->mode));

							if(pfield->mode == PKW_initializeOnly || pfield->mode == PKW_inputOutput)
								print_field(fp,level,pfield->type,fieldName,&(pfield->defaultVal),DEFedNodes);
							else
								fprintf(fp,"\n");
						}
						level--;
					}
				}else{
					union anyVrml* any_except_PTR = (union anyVrml*)&((char*)node)[field->offset];
					print_field(fp,level,field->typeIndex,FIELDNAMES[field->nameIndex],any_except_PTR,DEFedNodes);
				}
			}
			ifield++;
			field = &offsets[ifield];
		}
	}
	fflush(fp) ;
	spacer fprintf (fp,"L%d end\n",level);
	if (level == 0) fprintf (fp,"ending dump_scene2\n");
}

/* deep_copy2() - experimental keyboard reachable deepcopy function */
void deep_copy2(int iopt, char* defname)
{
	struct X3D_Node* node;
	char *name2;
	node = NULL;
	ConsoleMessage("in deep_copy2 - for copying a node and its fields\n");
	ConsoleMessage("got iopt=%d defname=%s\n",iopt,defname);
	if(iopt == 0) return;
	if(iopt == 1)
	{
		node = parser_getNodeFromName(defname);
	}
	if(iopt == 2)
	{
		node = (struct X3D_Node*)rootNode();
	}
	if(iopt == 3)
	{
		sscanf(defname,"%p",&node);
	}
	if( checkNode(node, NULL, 0) )
	{
		name2 = parser_getNameFromNode(node);
		if(name2 != NULL)
			ConsoleMessage("You entered %s\n",name2);
		else
			ConsoleMessage("Node exists!\n");
	}else{
		ConsoleMessage("Node does not exist.\n");
	}
}

void print_DEFed_node_names_and_pointers(FILE* fp)
{
	int ind,j,jj,nstack,nvector;
	char * name;
	struct X3D_Node * node;
	struct Vector *curNameStackTop;
	struct Vector *curNodeStackTop;
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;

	fprintf(fp,"DEFedNodes ");
	if(globalParser->DEFedNodes == NULL)
	{
		fprintf(fp," NULL\n");
		return;
	}
	nstack = globalParser->lexer->userNodeNames->n;
	fprintf(fp," lexer namespace vectors = %d\n",nstack);
	for(j=0;j<nstack;j++)
	{
		curNameStackTop = vector_get(struct Vector *, globalParser->lexer->userNodeNames,j);
		curNodeStackTop = vector_get(struct Vector *, globalParser->DEFedNodes,j);
		if(curNameStackTop && curNodeStackTop)
		{
			nvector = vectorSize(curNodeStackTop);
			for(jj=0;jj<j;jj++) fprintf(fp,"  ");
			fprintf(fp,"vector %d name count = %d\n",j,nvector);
			for (ind=0; ind < nvector; ind++)
			{
				for(jj=0;jj<j;jj++) fprintf(fp,"  ");
				node = vector_get(struct X3D_Node*,curNodeStackTop, ind);
				name = vector_get(char *,curNameStackTop, ind);
				fprintf (fp,"L%d: node (%p) name (%s) \n",jj,node,name);
			}
		}
	}
}
char *findFIELDNAMESfromNodeOffset0(struct X3D_Node *node, int offset)
{
	if( node->_nodeType != NODE_Script)
	{
		if( node->_nodeType == NODE_Proto )
		{
			int mode;
			struct Vector* usernames[4];
			char **userArr;
			struct ProtoFieldDecl* pfield;
			struct X3D_Proto* pnode = (struct X3D_Proto*)node;
			struct VRMLLexer* lexer;
			struct VRMLParser *globalParser;
			struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
			if(pstruct){
				globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;
				lexer = (struct VRMLLexer*)globalParser->lexer;
				usernames[0] = lexer->user_initializeOnly;
				usernames[1] = lexer->user_inputOnly;
				usernames[2] = lexer->user_outputOnly;
				usernames[3] = lexer->user_inputOutput;
				if(pstruct->iface) {
				    if(offset < vectorSize(pstruct->iface))
				    {
					//JAS const char *fieldName;
					pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, offset);
					mode = pfield->mode;
					#define X3DMODE(val)  ((val) % 4)
					userArr = (char **)&vector_get(const char*, usernames[X3DMODE(mode)], 0);
					return userArr[pfield->name];
				    } else return NULL;
				}
			}else return NULL;
		}
		else
		  //return (char *)FIELDNAMES[NODE_OFFSETS[node->_nodeType][offset*5]];
		  return (char *)findFIELDNAMESfromNodeOffset(node,offset);
	}
  #ifdef HAVE_JAVASCRIPT
	{
		struct Vector* fields;
		struct ScriptFieldDecl* curField;

		struct Shader_Script *myObj = X3D_SCRIPT(node)->__scriptObj;
		struct CRjsnameStruct *JSparamnames = getJSparamnames();

		fields = myObj->fields;
		curField = vector_get(struct ScriptFieldDecl*, fields, offset);
		return fieldDecl_getShaderScriptName(curField->fieldDecl);
	}
  #else
	return "script";
  #endif

}
char *findFIELDNAMES0(struct X3D_Node *node, int offset)
{
	return findFIELDNAMESfromNodeOffset0(node,offset);
}

void print_routes_ready_to_register(FILE* fp);
void print_routes(FILE* fp)
{
	int numRoutes;
	int count;
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	int fromOffset;
	int toOffset;
	char *fromName;
	char *toName;

	print_routes_ready_to_register(fp);
	numRoutes = getRoutesCount();
	fprintf(fp,"Number of Routes %d\n",numRoutes-2);
	if (numRoutes < 2) {
		return;
	}

	/* remember, in the routing table, the first and last entres are invalid, so skip them */
	for (count = 1; count < (numRoutes-1); count++) {
		getSpecificRoute (count,&fromNode, &fromOffset, &toNode, &toOffset);
		fromName = parser_getNameFromNode(fromNode);
		toName   = parser_getNameFromNode(toNode);

		fprintf (fp, " %p %s.%s TO %p %s.%s \n",fromNode,fromName,
			findFIELDNAMESfromNodeOffset0(fromNode,fromOffset),
			toNode,toName,
			findFIELDNAMESfromNodeOffset0(toNode,toOffset)
			);
	}
}
static struct consoleMenuState
{
	int active;
	void (*f)(void*,char*);
	char buf[100];
	int len;
	char *dfault;
	void *yourData;
} ConsoleMenuState;
int consoleMenuActive()
{
	return ConsoleMenuState.active;
}
#ifdef _MSC_VER
#define KEYPRESS 1
#else
#define KEYDOWN 2
#endif

void addMenuChar(kp,type)
{
	char str[100];
	void (*callback)(void*,char*);
	void *yourData;
#ifdef _MSC_VER
	if(type == KEYPRESS) {
#else
	if(type == KEYDOWN) {
#endif
	if((kp == '\n') || (kp == '\r'))
	{
		ConsoleMessage("\n");
		if(ConsoleMenuState.len == 0)
			strcpy(str,ConsoleMenuState.dfault);
		else
			strcpy(str,ConsoleMenuState.buf);
		callback = ConsoleMenuState.f;
		yourData = ConsoleMenuState.yourData;
		ConsoleMenuState.active = 0;
		ConsoleMenuState.len = 0;
		ConsoleMenuState.buf[0]= '\0';
		ConsoleMenuState.dfault = NULL;
		ConsoleMenuState.f = (void*)NULL;
		callback(yourData,str);
	}else{
		ConsoleMessage("%c",kp);
		ConsoleMenuState.buf[ConsoleMenuState.len] = kp;
		ConsoleMenuState.len++;
		ConsoleMenuState.buf[ConsoleMenuState.len] = '\0';
	}
	}
}
void setConsoleMenu(void *yourData, char *prompt, void (*callback), char* dfault)
{
	ConsoleMenuState.f = callback;
	ConsoleMenuState.len = 0;
	ConsoleMenuState.buf[0] = '\0';
	ConsoleMenuState.active = TRUE;
	ConsoleMenuState.dfault = dfault;
	ConsoleMenuState.yourData = yourData;
	ConsoleMessage(prompt);
	ConsoleMessage("[%s]:",dfault);
}
void deep_copy_defname(void *myData, char *defname)
{
	int iopt;
	ConsoleMessage("you entered defname: %s\n",defname);
	memcpy(&iopt,myData,4);
	deep_copy2(iopt,defname);
	free(myData);
}
void deep_copy_option(void* yourData, char *opt)
{
	int iopt;
	ConsoleMessage("you chose option %s\n",opt);
	sscanf(opt,"%d",&iopt);
	if(iopt == 0) return;
	if(iopt == 1 || iopt == 3)
	{
		void* myData = malloc(4); //could store in gglobal->mainloop or wherever, then don't free in deep_copy_defname
		memcpy(myData,&iopt,4);
		setConsoleMenu(myData,"Enter DEFname or node address:", deep_copy_defname, "");
	}
	if(iopt == 2)
		deep_copy2(iopt, NULL);
}
void dump_scenegraph(int method)
{
//#ifdef FW_DEBUG
	if(method == 1) // '\\'
		dump_scene(stdout, 0, (struct X3D_Node*) rootNode());
	else if(method == 2) // '|'
	{
		Stack * DEFedNodes = newVector(struct X3D_Node*, 2);
		dump_scene2(stdout, 0, (struct X3D_Node*) rootNode(),1,DEFedNodes);
		deleteVector(struct X3D_Node*,DEFedNodes);
	}
	else if(method == 3) // '='
	{
		print_DEFed_node_names_and_pointers(stdout);
	}
	else if(method == 4) // '+'
	{
		print_routes(stdout);
	}
	else if(method == 5) // '-'
	{
		//ConsoleMenuState.active = 1; //deep_copy2();
		setConsoleMenu(NULL,"0. Exit 1.DEFname 2.ROOTNODE 3.node address", deep_copy_option, "0");
	}
//#endif
}

void fwl_clearWorld(){
	//clear the scene to empty (and do cleanup on old scene);
	ttglobal tg = gglobal();
	tg->Mainloop.replaceWorldRequest = NULL;
	tg->threads.flushing = 1;
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
		if(type == KEYPRESS)
		{
			lkp = key;
			//if(kp>='A' && kp <='Z') lkp = tolower(kp);
			switch (lkp) {
				case 'n': {  fwl_clearWorld(); break; }
				case 'e': { fwl_set_viewer_type (VIEWER_EXAMINE); break; }
				case 'w': { fwl_set_viewer_type (VIEWER_WALK); break; }
				case 'd': { fwl_set_viewer_type (VIEWER_FLY); break; }
				case 'f': { fwl_set_viewer_type (VIEWER_EXFLY); break; }
				case 'y': { fwl_set_viewer_type (VIEWER_YAWPITCHZOOM); break; }
				case 't': { fwl_set_viewer_type(VIEWER_TURNTABLE); break; }
				case 'h': { fwl_toggle_headlight(); break; }
				case '/': { print_viewer(); break; }
				//case '\\': { dump_scenegraph(); break; }
				case '\\': { dump_scenegraph(1); break; }
				case '|': { dump_scenegraph(2); break; }
				case '=': { dump_scenegraph(3); break; }
				case '+': { dump_scenegraph(4); break; }
				case '-': { dump_scenegraph(5); break; }
				case '`': { toggleLogfile(); break; }

				case '$': resource_tree_dump(0, tg->resources.root_res); break;
				case '*': resource_tree_list_files(0, tg->resources.root_res); break;
				case 'q': { if (!RUNNINGASPLUGIN) {
							fwl_doQuit();
							}
							break;
						}
				case 'c': { toggle_collision(); break;}
				case 'v': {fwl_Next_ViewPoint(); break;}
				case 'b': {fwl_Prev_ViewPoint(); break;}
				case '.': {profile_print_all(); break;}

#if !defined(FRONTEND_DOES_SNAPSHOTS)
				case 's': {fwl_toggleSnapshot(); break;}
				case 'x': {Snapshot(); break;} /* thanks to luis dias mas dec16,09 */
#endif //FRONTEND_DOES_SNAPSHOTS

				default:
					handled = 0;
					break;
			}
		}
		if(!handled) {
			char kp;
			if(type/10 == 0)
				kp = (char)key; //normal keyboard key
			else
				kp = lookup_fly_key(key); //actionKey possibly numpad or arrows, convert to a/z
			if(kp){
				double keytime = Time1970sec();
				if(type%10 == KEYDOWN)
					handle_key(kp,keytime);  //keydown for fly
				if(type%10 == KEYUP)
					handle_keyrelease(kp,keytime); //keyup for fly
			}
		}
	}
}
void queueKeyPress(ppMainloop p, int key, int type){
	if(p->keypressQueueCount < 50){
		p->keypressQueue[p->keypressQueueCount].key = key;
		p->keypressQueue[p->keypressQueueCount].type = type;
		p->keypressQueueCount++;
	}
}
int platform2web3dActionKey(int platformKey);
//int isWeb3dDeleteKey(int web3dkey);
//void fwl_do_rawKeyPress_OLD(int key, int type) {
//	ppMainloop p;
//	ttglobal tg = gglobal();
//	p = (ppMainloop)tg->Mainloop.prv;
//
//	//for testing mode -R --record:
//	//we need to translate non-ascii keys before saving to ascii file
//	//so the .fwplay file can be replayed on any system (the action and control keys
//	//will be already in web3d format)
//	if(type>1){ //just the raw keys (the fully translated keys are already in ascii form)
//		int actionKey = platform2web3dActionKey(key);
//		if(actionKey){
//			key = actionKey;
//			type += 10; //pre-tranlated raw keys will have type 12 or 13
//		}
//	}
//
//	if(p->modeRecord){
//		queueKeyPress(p,key,type);
//	}else{
//		fwl_do_keyPress0(key,type);
//	}
//	if(type==13 && isWeb3dDeleteKey(key))
//	{
//		//StringSensor likes DEL as a single char int the char stream,
//		//but OSes usually only do the raw key so
//		//here we add a DEL to the stream.
//		type = 1;
//		if(p->modeRecord){
//			queueKeyPress(p,key,type);
//		}else{
//			fwl_do_keyPress0(key,type);
//		}
//	}
//}
void fwl_do_rawKeyPress(int key, int type) {
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

	if(p->modeRecord){
		queueKeyPress(p,key,type);
	}else{
		fwl_do_keyPress0(key,type);
	}
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

struct X3D_Node* getRayHit() {
        double x,y,z;
        int i;
		ppMainloop p;
		ttglobal tg = gglobal();
		p = (ppMainloop)tg->Mainloop.prv;

        if(tg->RenderFuncs.hitPointDist >= 0) {
			struct currayhit * rh = (struct currayhit *)tg->RenderFuncs.rayHit;
                FW_GLU_UNPROJECT(tg->RenderFuncs.hp.x,tg->RenderFuncs.hp.y,tg->RenderFuncs.hp.z,rh->modelMatrix,rh->projMatrix,viewport,&x,&y,&z);

                /* and save this globally */
                tg->RenderFuncs.ray_save_posn.c[0] = (float) x; tg->RenderFuncs.ray_save_posn.c[1] = (float) y; tg->RenderFuncs.ray_save_posn.c[2] = (float) z;

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
	- formed in setup_projection(pick=TRUE,,)
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
		in viewpoint-local, and for B mousexy is converted in setup_projection(pick=TRUE,,) 
		to a special pick-proj matrix that centers the viewport on the mousexy, so B is 0,0,-1 in
			pick-viewport-local coordinates	(glu_unproject produces pick-viewport-local)
		[using normal projection matrix for the bearing, then B= mousex,mousey,-1 in 
		normal viewport-local coordinates, and glu_unproject is used to get this bearing
		into world coordinates for the remainder of PointingDeviceSensor activity]
	[2.for a 3D pointing device, it would have its own A,B in world. And then that world bearing 
		can be transformed into geometry-local and sensor-local and intersections with 
		shape-geometry and sensor-geometry calculated.]
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
	-apply a special pick-projection matrix, in setup_projection(pick=TRUE,,), so that glu_unproject 
		is with respect to the mouse-xy bearing B point being at the center of the viewport 
		[use normal projection, and minimize the use of glu_unproject by transforming viewport-local bearing
		to world during setup, and use world for bearing for all subsequent PointingDeviceSensor activities]
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
		- compare the remaining intersection point by distance from A, and chose it over 
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
	the specs say it's the sensor that's the nearest co-descendent (sibling or higher) to the geometry
	(it doesn't say what happens in a tie ie 2 different sensor nodes are siblings)
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
	-- it will include the pick-specific projection matrix aligned to the mousexy in setup_projection(pick=TRUE,,)
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
	c) allows setup_viewpoint() and setup_projection() to be computed once for multiple passes
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
		it's convenient, and includes the pick-viewport in the transform - see setup_projection(pick=TRUE,,) for details
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

        FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projMatrix);
        FW_GLU_UNPROJECT(r1.x, r1.y, r1.z, rhh->modelMatrix,
                projMatrix, viewport, &x1, &y1, &z1);
        FW_GLU_UNPROJECT(r2.x, r2.y, r2.z, rhh->modelMatrix,
                projMatrix, viewport, &x2, &y2, &z2);
        FW_GLU_UNPROJECT(tg->RenderFuncs.hp.x, tg->RenderFuncs.hp.y, tg->RenderFuncs.hp.z, rh->modelMatrix,
                projMatrix,viewport, &x3, &y3, &z3);

        /* printf ("get_hyperhit in VRMLC %f %f %f, %f %f %f, %f %f %f\n",
            x1,y1,z1,x2,y2,z2,x3,y3,z3); */

        /* and save this globally */
        tg->RenderFuncs.hyp_save_posn.c[0] = (float) x1; tg->RenderFuncs.hyp_save_posn.c[1] = (float) y1; tg->RenderFuncs.hyp_save_posn.c[2] = (float) z1;
        tg->RenderFuncs.hyp_save_norm.c[0] = (float) x2; tg->RenderFuncs.hyp_save_norm.c[1] = (float) y2; tg->RenderFuncs.hyp_save_norm.c[2] = (float) z2;
        tg->RenderFuncs.ray_save_posn.c[0] = (float) x3; tg->RenderFuncs.ray_save_posn.c[1] = (float) y3; tg->RenderFuncs.ray_save_posn.c[2] = (float) z3;
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

#if KEEP_X11_INLIB
	/* Hmm. display_initialize is really a frontend function. The frontend should call it before calling fwl_initializeRenderSceneUpdateScene */
	/* Initialize display */
	//if (!fv_display_initialize()) {
	//       ERROR_MSG("initFreeWRL: error in display initialization.\n");
	//       exit(1);
	//}
#endif /* KEEP_X11_INLIB */

	new_tessellation();

	fwl_set_viewer_type(VIEWER_EXAMINE);

	viewer_postGLinit_init();

#ifndef AQUA
	if (tg->display.params.fullscreen && newResetGeometry != NULL) newResetGeometry();
	#endif

	/* printf ("fwl_initializeRenderSceneUpdateScene finish\n"); */
	// on OSX, this function is not called by the thread that holds the OpenGL
	// context. Unsure if only Windows can do this one, but for now,
	// do NOT do this on OSX.
//#ifndef TARGET_AQUA
//	drawStatusBar(); //just to get it initialized
//#endif
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

void finalizeRenderSceneUpdateScene() {
	//C. delete instance data
	ttglobal tg = gglobal();
	printf ("finalizeRenderSceneUpdateScene\n");

	/* set geometry to normal size from fullscreen */
#ifndef AQUA
	if (newResetGeometry != NULL) newResetGeometry();
#endif
	/* kill any remaining children processes like sound processes or consoles */
	killErrantChildren();
#ifdef DEBUG_MALLOC
	void scanMallocTableOnQuit(void);
	scanMallocTableOnQuit();
#endif
	/* tested on win32 console program July9,2011 seems OK */
	iglobal_destructor(tg);
}


/* iphone front end handles the displayThread internally */
//#ifndef FRONTEND_HANDLES_DISPLAY_THREAD


int checkReplaceWorldRequest(){
	ttglobal tg = gglobal();
	if (tg->Mainloop.replaceWorldRequest || tg->Mainloop.replaceWorldRequestMulti){
		tg->threads.flushing = 1;
	}
	return tg->threads.flushing;
}
static int exitRequest = 0; //static because we want to exit the process, not just a freewrl instance (I think).
int checkExitRequest(){
	if(!exitRequest){
		ttglobal tg = gglobal();
		if (tg->threads.MainLoopQuit == 1){
			tg->threads.flushing = 1;
			exitRequest = 1;
		}
	}
	return exitRequest;
}
void doReplaceWorldRequest()
{
	resource_item_t *res,*resm;
	char * req;

	ttglobal tg = gglobal();

	req = tg->Mainloop.replaceWorldRequest;
	tg->Mainloop.replaceWorldRequest = NULL;
	if (req){
		kill_oldWorld(TRUE, TRUE, __FILE__, __LINE__);
		res = resource_create_single(req);
		//send_resource_to_parser_async(res);
		resitem_enqueue(ml_new(res));
	}
	resm = (resource_item_t *)tg->Mainloop.replaceWorldRequestMulti;
	if (resm){
		tg->Mainloop.replaceWorldRequestMulti = NULL;
		kill_oldWorld(TRUE, TRUE, __FILE__, __LINE__);
		resm->new_root = true;
		gglobal()->resources.root_res = resm;
		//send_resource_to_parser_async(resm);
		resitem_enqueue(ml_new(resm));
	}
	tg->threads.flushing = 0;
}
static int(*view_initialize)() = NULL;
static void(*view_update)() = NULL;
#if KEEP_FV_INLIB
int view_initialize0(void){
	/* Initialize display - View initialize*/
	if (!fv_display_initialize()) {
		ERROR_MSG("initFreeWRL: error in display initialization.\n");
		return FALSE; //exit(1);
	}
	return TRUE;
}
#endif /* KEEP_FV_INLIB */

#ifdef _MSC_VER
void updateCursorStyle0(int cstyle);
void updateViewCursorStyle(int cstyle)
{
	updateCursorStyle0(cstyle);
}
#else
/* Status variables */
/* cursors are a 'shared resource' meanng you only need one cursor for n windows,
not per-instance cursors (except multi-touch multi-cursors)
However cursor style choice could/should be per-window/instance
*/

void updateViewCursorStyle(int cstyle)
{
#if !defined (_ANDROID)
	/* ANDROID - no cursor style right now */
	setCursor(cstyle);
#endif //ANDROID
}
#endif

void view_update0(void){
	#ifdef _MSC_VER
		fwMessageLoop(); //message pump
	#endif
	#if defined(STATUSBAR_HUD)
		/* status bar, if we have one */
		finishedWithGlobalShader();
		drawStatusBar();  // View update
		restoreGlobalShader();
	#endif
	updateViewCursorStyle(getCursorStyle()); /* in fwWindow32 where cursors are loaded */
}
void killNodes();

/* fwl_draw() call from frontend when frontend_handles_display_thread */
int fwl_draw()
{
	int more;
	ppMainloop p;
	ttglobal tg = gglobal();
	fwl_setCurrentHandle(tg, __FILE__, __LINE__);
	p = (ppMainloop)tg->Mainloop.prv;

	if (!p->draw_initialized){
		view_initialize = view_initialize0; //defined above, with ifdefs
		view_update = view_update0; //defined above with ifdefs
		if (view_initialize)
			more = view_initialize();

		if (more){
			fwl_initializeRenderSceneUpdateScene();  //Model initialize
		}
		p->draw_initialized = TRUE;
	}
	more = TRUE;
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
			checkExitRequest(); //will set flushing=1
			break;
		case 1:
			if (workers_waiting()) //one way to tell if workers finished flushing is if their queues are empty, and they are not busy
			{
				kill_oldWorld(TRUE, TRUE, __FILE__, __LINE__); //does a MarkForDispose on nodes, wipes out binding stacks and route table, javascript
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
		killNodes(); //deallocates nodes MarkForDisposed
		tg->threads.MainLoopQuit++;
		break;
	case 3:
		//check if worker threads have exited
		more = workers_running();
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
	/* JAS
		if (gglobal() == NULL) ConsoleMessage ("fwl_initialize_parser, gglobal() NULL");
		if ((gglobal()->Mainloop.prv) == NULL) ConsoleMessage ("fwl_initialize_parser, gglobal()->Mainloop.prv NULL");
	*/

   //     ((ppMainloop)(gglobal()->Mainloop.prv))->quitThread = FALSE;

	/* create the root node */
	if (rootNode() == NULL) {
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
    p->keypress_string = strdup(kstring);
}

void fwl_set_modeRecord()
{
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
    p->modeRecord = TRUE;
}
void fwl_set_modeFixture()
{
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
    p->modeFixture = TRUE;
}
void fwl_set_modePlayback()
{
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
    p->modePlayback = TRUE;
}
void fwl_set_nameTest(char *nameTest)
{
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
    p->nameTest = strdup(nameTest);
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

#ifdef OLDCODE
void fwl_doQuitInstance()
{
#if !defined(FRONTEND_HANDLES_DISPLAY_THREAD)
	if(!gglobal()->display.params.frontend_handles_display_thread)
    	stopDisplayThread();
#endif
    kill_oldWorld(TRUE,TRUE,__FILE__,__LINE__); //must be done from this thread
	stopLoadThread();
	stopPCThread();

	/* set geometry to normal size from fullscreen */
#ifndef AQUA
    if (newResetGeometry != NULL) newResetGeometry();
#endif
    /* kill any remaining children */
    killErrantChildren();
#ifdef DEBUG_MALLOC
    void scanMallocTableOnQuit(void);
    scanMallocTableOnQuit();
#endif
	/* tested on win32 console program July9,2011 seems OK */
	iglobal_destructor(gglobal());
}
#endif
//OLDCODE #endif //ANDROID


/* quit key pressed, or Plugin sends SIGQUIT */
void fwl_doQuit()
{
	ttglobal tg = gglobal();
//OLDCODE #if defined(_ANDROID)
//OLDCODE 	fwl_Android_doQuitInstance();
//OLDCODE #else //ANDROID
	//fwl_doQuitInstance();
//OLDCODE #endif //ANDROID
    //exit(EXIT_SUCCESS);
	tg->threads.MainLoopQuit = max(1,tg->threads.MainLoopQuit); //make sure we don't go backwards in the quit process with a double 'q'
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
//int iglobal_instance_count();
//void fwl_closeGlobals()
//{
//	//"last one out shut off the lights"
//	//when there are no freewrl iglobal instances left, then call this to shut
//	//down anything that's of per-process / per-application / static-global-shared
//	//dug9 - not used yet as of Aug 3, 2011
//	//if you call from the application main thread / message pump ie on_key > doQuit
//	//then in theory there should be a way to iterate through all
//	//instances, quitting each one in a nice way, say on freewrlDie or
//	//(non-existant yet) doQuitAll or doQuitInstanceOrAllIfNoneLeft
//	//for i = 1 to iglobal_instance_count
//	//  set instance through window handle or index (no function yet to
//	//       get window handle by index, or set instance by index )
//	//  fwl_doQuitInstance
//	//then call fwl_closeGlobals
//	if(iglobal_instance_count() == 0)
//	{
//		close_internetHandles();
//		//console window?
//	}
//}
void freewrlDie (const char *format) {
        ConsoleMessage ("Catastrophic error: %s\n",format);
        fwl_doQuit();
}

//int ntouch =0;
//int currentTouch = -1;
/* MIMIC what happens in handle_Xevents, but without the X events */
void fwl_handle_aqua_multi0(const int mev, const unsigned int button, int x, int y, int ID);
void fwl_handle_aqua_multi(const int mev, const unsigned int button, int x, int y, int ID)
{
	ppMainloop p;
	ttglobal tg = gglobal();
	p = (ppMainloop)tg->Mainloop.prv;

	if(p->modeRecord || p->modeFixture || p->modePlayback){
		if(p->modeRecord){
			queueMouseMulti(p,mev,button,x,y,ID);
		}
		//else ignor so test isn't ruined by random mouse movement during playback
		return;
	}
	fwl_handle_aqua_multi0(mev, button, x, y, ID);
}

void fwl_handle_aqua_multi0(const int mev, const unsigned int button, int x, int y, int ID) {
        int count;
		ppMainloop p;
		ttglobal tg = gglobal();
		p = (ppMainloop)tg->Mainloop.prv;

  /* printf ("fwl_handle_aqua in MainLoop; but %d x %d y %d screenWidth %d screenHeight %d",
                button, x,y,tg->display.screenWidth,tg->display.screenHeight);
        if (mev == ButtonPress) printf ("ButtonPress\n");
        else if (mev == ButtonRelease) printf ("ButtonRelease\n");
        else if (mev == MotionNotify) printf ("MotionNotify\n");
        else printf ("event %d\n",mev); */

        /* save this one... This allows Sensors to get mouse movements if required. */
        p->lastMouseEvent = mev;
        /* save the current x and y positions for picking. */
		tg->Mainloop.currentX[p->currentCursor] = x;
		tg->Mainloop.currentY[p->currentCursor] = y;
		p->touchlist[ID].x = x;
		p->touchlist[ID].y = y;
		p->touchlist[ID].button = button;
		p->touchlist[ID].isDown = (mev == ButtonPress || mev == MotionNotify);
		p->touchlist[ID].ID = ID; /*will come in handy if we change from array[] to accordian list*/
		p->touchlist[ID].mev = mev;
		p->touchlist[ID].angle = 0.0f;
		p->currentTouch = ID;


		//if( handleStatusbarHud(mev, &tg->Mainloop.clipPlane) )return; /* statusbarHud options screen should swallow mouse clicks */

        if ((mev == ButtonPress) || (mev == ButtonRelease)) {
                /* record which button is down */
                p->ButDown[p->currentCursor][button] = (mev == ButtonPress);
                /* if we are Not over an enabled sensitive node, and we do NOT already have a
                   button down from a sensitive node... */

                if ((p->CursorOverSensitive ==NULL) && (p->lastPressedOver ==NULL)) {
                        p->NavigationMode=p->ButDown[p->currentCursor][1] || p->ButDown[p->currentCursor][3];
                        handle(mev, button, (float) ((float)x/tg->display.screenWidth), (float) ((float)y/tg->display.screenHeight));
                }
        }

        if (mev == MotionNotify) {
                /* save the current x and y positions for picking. */
                // above currentX[currentCursor] = x;
                //currentY[currentCursor] = y;

                if (p->NavigationMode) {
                        /* find out what the first button down is */
                        count = 0;
                        while ((count < 8) && (!p->ButDown[p->currentCursor][count])) count++;
                        if (count == 8) return; /* no buttons down???*/

                        handle (mev, (unsigned) count, (float) ((float)x/tg->display.screenWidth), (float) ((float)y/tg->display.screenHeight));
                }
        }
}
//int lastDeltax = 50;
//int lastDeltay = 50;
//int lastxx;
//int lastyy;
void emulate_multitouch(const int mev, const unsigned int button, int x, int y)
{
	/* goal: when MMB draw a slave cursor pinned to last_distance,last_angle from real cursor
		Note: if using a RMB+LMB = MMB chord with 2 button mice, you need to emulate in your code
			and pass in button 2 here, after releasing your single button first ie:
			fwl_handle_aqua(ButtonRelease, 1, x, y);
			fwl_handle_aqua(ButtonRelease, 3, x, y);
	*/
	if( button == 2 )
	{
		ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
		if( mev == ButtonPress )
		{
			p->lastxx = x - p->lastDeltax;
			p->lastyy = y - p->lastDeltay;
		}else if(mev == MotionNotify || mev == ButtonRelease ){
			p->lastDeltax = x - p->lastxx;
			p->lastDeltay = y - p->lastyy;
		}
		fwl_handle_aqua_multi(mev, 1, x, y, 0);
		fwl_handle_aqua_multi(mev, 1, p->lastxx, p->lastyy, 1);
	}else{
		/* normal, no need to emulate if there's no MMB or LMB+RMB */
		fwl_handle_aqua_multi(mev,button,x,y,0);
	}
}
/* old function should still work, with single mouse and ID=0 */
void fwl_handle_aqua(const int mev, const unsigned int button, int x, int y) {
    ttglobal tg = gglobal();

	/* printf ("fwl_handle_aqua, type %d, screen wid:%d height:%d, orig x,y %d %d\n",
            mev,tg->display.screenWidth, tg->display.screenHeight,x,y); */

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
					x = tg->display.screenHeight-x;

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
					x = tg->display.screenWidth - oy;
					y = tg->display.screenHeight - ox;
					break;
				default:{}
			}

		// these make sense for examine navigation
		} else if (Viewer()->type == VIEWER_EXAMINE) {
			switch (Viewer()->screenOrientation) {
				case 0:
					break;
				case 90:
					x = tg->display.screenWidth - oy;
					y = ox;
					break;
				case 180:
					x = tg->display.screenWidth -x;
					y = tg->display.screenHeight -y;
					break;
				case 270:
					// nope x = tg->display.screenWidth - oy;
					// nope y = tg->display.screenHeight - ox;

					x = tg->display.screenHeight - oy;
					y = tg->display.screenWidth - ox;

					//printf ("resulting in x %d  y %d\n",x,y);
					break;
				default:{}
			}

		}
	}

	#endif

	if(((ppMainloop)(tg->Mainloop.prv))->EMULATE_MULTITOUCH)
		emulate_multitouch(mev,button,x, y);
	else
	{
		fwl_handle_aqua_multi(mev,button,x,y,0);

		//updateCursorStyle();
	}
}

//#endif

void fwl_setCurXY(int cx, int cy) {
	ttglobal tg = gglobal();
	ppMainloop p = (ppMainloop)tg->Mainloop.prv;
	/* printf ("fwl_setCurXY, have %d %d\n",p->currentX[p->currentCursor],p->currentY[p->currentCursor]); */
        tg->Mainloop.currentX[p->currentCursor] = cx;
        tg->Mainloop.currentY[p->currentCursor] = cy;
}

void fwl_setButDown(int button, int value) {
	ppMainloop p = (ppMainloop)gglobal()->Mainloop.prv;
	/* printf ("fwl_setButDown called\n"); */
        p->ButDown[p->currentCursor][button] = value;
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
		for (i=0; i<rootNode()->children.n; i++) {
			markForDispose(rootNode()->children.p[i], TRUE);
		}

		/* stop rendering. This should be done when the new resource is loaded, and new_root is set,
		but lets do it here just to make sure */
		rootNode()->children.n = 0; // no children, but _sortedChildren not made;
		rootNode()->_change ++; // force the rootNode()->_sortedChildren to be made
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
	if(!tg->display.params.frontend_handles_display_thread)
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
		sendSensorEvents(p->oldCOS,MapNotify,p->ButDown[p->currentCursor][1], FALSE);
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

	struct X3D_Group *rn = rootNode();
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

	struct X3D_Group *rn = rootNode();
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
