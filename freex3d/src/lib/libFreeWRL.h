/*
  $Id$

  FreeWRL library API (public)

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


#ifndef __LIBFREEWRL_API_H__
#define __LIBFREEWRL_API_H__


/* for front ends that do not have these X-11-based defines */
#if defined(AQUA) || defined(_MSC_VER) || defined(_ANDROID)
#ifndef _MIMIC_X11_SCREEN_BUTTONS
        #define _MIMIC_X11_SCREEN_BUTTONS
                #define KeyPress        2
                #define KeyRelease      3
                #define ButtonPress     4
                #define ButtonRelease   5
                #define MotionNotify    6
                #define MapNotify       19
        #endif
#endif




#ifdef COMPILING_IPHONE_FRONT_END
	/* Ok, ok, ok. I know. Another definition. But, Objective-C gives lots of
 	   errors if the whole file is included, and also, we only need a couple of
	   definitions to keep the front end as separate from the library as possible... */

	void fwl_initializeRenderSceneUpdateScene(void);
	

#else /* COMPILING_IPHONE_FRONT_END */

/**
 * Version embedded
 */
const char *libFreeWRL_get_version();

/* for front ends to get the versions */
const char *fwl_libFreeWRL_get_version(); /* library version */
const char *fwl_freewrl_get_version();  /* UI version */

/**
 * Initialization
 */
typedef struct freewrl_params {
	/* Put here global parameters, parsed in main program
	   and needed to initialize libFreeWRL
	   example: width, height, fullscreen, multithreading, eai...
	*/
	int width;
	int height;
	int xpos;
	int ypos;
	long int winToEmbedInto;
	bool fullscreen;
	bool multithreading;
	bool eai;
	bool verbose;
	//int collision;	/* do collision detection? moved to x3d_viewer struct july 7, 2012*/

} freewrl_params_t;

extern freewrl_params_t fwl_params;

/* FreeWRL parameters */
/*
 * These have been subject to abuse when then were all fw_params
 * At at Fri Apr 29 09:38:26 BST 2011 I expect lots of compiler messages.
 */
/* extern freewrl_params_t fv_params; */
/* extern freewrl_params_t fwl_params; */
/* extern freewrl_params_t OSX_params; */
void *fwl_init_instance();
void fwl_initParams( freewrl_params_t *params) ;

void fwl_setp_width		(int foo);
void fwl_setp_height		(int foo);
void fwl_setp_winToEmbedInto	(long int);
void fwl_setp_fullscreen	(bool foo);
void fwl_setp_multithreading	(bool foo);
void fwl_setp_eai		(bool foo);
void fwl_setp_verbose		(bool foo);
//void fwl_setp_collision		(int foo);

int	fwl_getp_width		(void);
int	fwl_getp_height		(void);
long int fwl_getp_winToEmbedInto (void);
bool	fwl_getp_fullscreen	(void);
bool	fwl_getp_multithreading	(void);
bool	fwl_getp_eai		(void);
bool	fwl_getp_verbose	(void);
//int	fwl_getp_collision	(void);

bool fwl_initFreeWRL(freewrl_params_t *params);
void closeFreeWRL();
void terminateFreeWRL();

int fwl_parse_geometry_string(const char *geometry, int *out_width, int *out_height, 
			      int *out_xpos, int *out_ypos);

/**
 * General functions
 */
int ConsoleMessage(const char *fmt, ...); /* This does not belong here!! */
//#endif

/* void Anchor_ReplaceWorld(char *name); */
bool Anchor_ReplaceWorld();

#define VIEWER_NONE 0	  /* would have conflicted with another NONE definition */
#define VIEWER_EXAMINE 1
#define VIEWER_WALK 2
#define VIEWER_EXFLY 3
#define VIEWER_FLY 4
#define VIEWER_YAWPITCHZOOM 5
#define VIEWER_FLY2 6
#define VIEWER_TILT 7
#define VIEWER_TPLANE 8
#define VIEWER_RPLANE 9

void setStereoBufferStyle(int);


/**
 * General variables
 */

#define RUNNINGASPLUGIN (isBrowserPlugin)

extern char *BrowserFullPath;

extern int _fw_pipe, _fw_FD;
extern int _fw_browser_plugin;
extern int isBrowserPlugin;
extern uintptr_t _fw_instance;
//extern char *keypress_string;

#ifdef HAVE_LIBCURL
extern int with_libcurl;
#endif

#endif /* COMPILING_IPHONE_FRONT_END */

/* ** NEW DJ ** */

void fwl_set_strictParsing(bool flag);
void fwl_set_plugin_print(bool flag);
void fwl_set_occlusion_disable(bool flag);
void fwl_set_print_opengl_errors(bool flag);
void fwl_set_trace_threads(bool flag);
void fwl_set_texture_size(unsigned int texture_size);
void fwl_set_glClearColor (float red , float green , float blue , float alpha);
void fwl_thread_dump(void);
int fwg_get_unread_message_count(void);
char *fwg_get_last_message(int whichOne);
void fwl_set_logfile(char *);
void fwl_set_nameTest(char *);

#if defined(_ANDROID)
void DROIDDEBUG( const char*pFmtStr, ...);
void PRINTF_ALL( const char*pFmtStr, ...);
#endif


/* ** REPLACE DJ ** */
/* Try to replace the compile-time options in ConsoleMessage with run-time options */
#ifdef AQUA
	#define MC_DEF_AQUA 1
#else
	#define MC_DEF_AQUA 0
#endif

#ifdef TARGET_AQUA
	#define MC_TARGET_AQUA 1
#else
	#define MC_TARGET_AQUA 0
#endif

#ifdef HAVE_MOTIF
	#define MC_HAVE_MOTIF 1
#else
	#define MC_HAVE_MOTIF 0
#endif

#ifdef TARGET_MOTIF
	#define MC_TARGET_MOTIF 1
#else
	#define MC_TARGET_MOTIF 0
#endif

#ifdef _MSC_VER
	#define MC_MSC_HAVE_VER 1
#else
	#define MC_MSC_HAVE_VER 0
#endif

/*
#ifdef NEW_CONSOLEMESSAGE_VERSION
#ifdef OLD_CONSOLEMESSAGE_VERSION
#ifdef HAVE_VSCPRINTF
*/

void fwl_ConsoleSetup(int setDefAqua , int setTargetAqua , int setHaveMotif , int setTargetMotif , int setHaveMscVer, int setTargetAndroid);
int fwl_StringConsoleMessage(char* message);

void fwl_init_SnapGif(void);
void fwl_init_PrintShot();
void fwl_set_SnapFile(const char* file);
void fwl_set_SnapTmp(const char* file);
void fwl_init_SnapSeq(); /* Was in main/headers.h */
void fwl_toggleSnapshot();
void fwl_set_LineWidth(float lwidth);
void fwl_set_KeyString(const char *str);
void fwl_set_SeqFile(const char* file);
void fwl_set_MaxImages(int max); 
void fwl_setCurXY(int x, int y);
void fwl_do_keyPress(char kp, int type);
void fwl_doQuit();
void fwl_doQuitInstance();
void fwl_set_viewer_type(const int type);
void fwl_set_modeRecord();
void fwl_set_modeFixture();
void fwl_set_modePlayback();

#define CHANNEL_EAI 0
#define CHANNEL_MIDI 1

#define RxTx_STOP 0		/* Shutdown */
#define RxTx_START 1	/* Start */
#define RxTx_REFRESH 2	/* Read any pending input into PRIVATE buffer */
#define RxTx_EMPTY 4	/* Empty the private buffer */
#define RxTx_MOREVERBOSE 8
#define RxTx_SILENT 16
#define RxTx_SINK 32	/* Just throw away any future input */
#define RxTx_PENDING 64
#define RxTx_STOP_IF_DISCONNECT 128
#define RxTx_STATE 32768
#define RxTx_GET_VERBOSITY 65536

/* The first few functions do I/O */
int	fwlio_RxTx_control(int channel, int action);
char *	fwlio_RxTx_getbuffer(int channel) ;
void	fwlio_RxTx_sendbuffer(char *fromFile, int fromLine, int channel, char *str);
char *	fwlio_RxTx_waitfor(int channel, char *str);

void	fwl_init_EaiVerbose();
void	fwl_EAI_clearListenerNode(void);
char *	fwl_EAI_handleBuffer(char *tempEAIdata);
int	fwl_EAI_allDone();
char *	fwl_EAI_handleRest();
char *	fwl_MIDI_handleBuffer(char *tempEAIdata);

void fwl_set_ScreenDist(const char *optArg);
void fwl_init_StereoDefaults(void); //don't need to call now March 2012
void fwl_set_EyeDist(const char *optArg);
void fwl_init_Shutter(void);
void fwl_init_SideBySide(void);
void fwl_set_AnaglyphParameter(const char *optArg);
void fwl_set_StereoParameter(const char *optArg);

// JAS obsolete void fwl_askForRefreshOK();

/* DISPLAY THREAD */
void fwl_initializeDisplayThread();
bool checkNetworkFile(const char *fn);
#define fwl_checkNetworkFile(a) checkNetworkFile(a)

/* PARSER THREAD */
void fwl_initialize_parser();
void fwl_initializeInputParseThread();
int fwl_isinputThreadParsing();
int fwl_isInputThreadInitialized();

/* TEXTURE THREAD */
void fwl_initializeTextureThread();
int fwl_isTextureinitialized();

/* PARSER THREAD */
int fwl_isTextureParsing();

void fwl_Next_ViewPoint(void);
void fwl_Prev_ViewPoint(void);
void fwl_First_ViewPoint(void);
void fwl_Last_ViewPoint(void);
void fwl_gotoViewpoint (char *findThisOne);

void fwl_startFreeWRL(const char *url);
/* distinguish instances from window event handler using the window handle */
int fwl_setCurrentHandle(void *handle, char*, int);
void fwl_clearCurrentHandle();
void *fwl_getCurrentHandle(char *, int);

void fwl_resource_push_single_request(const char *request);
void fwl_OSX_initializeParameters(const char* initialURL);
void fwl_resource_push_single_request_IE_main_scene(const char *request);

void fwg_frontEndReturningData(unsigned char* fileData,int length,int width,int height,bool hasAlpha);

void fwg_frontEndReturningLocalFile(char *localfile, int iret);
void fwl_RenderSceneUpdateScene(void);
void fwl_setScreenDim(int wi, int he);
bool fwl_initialize_GL(void);
void fwl_setLastMouseEvent(int etype);
void fwl_handle_aqua(const int mev, const unsigned int button, int x, int y);

/* JAS - moving OSX front end into 2011 code workings - these may change. */
void fwl_replaceWorldNeeded(char* str);
int fwl_pollPromptForURL(); /* poll from front end / UI in loop */
int fwl_pollPromptForFile();
void fwl_setPromptForURL(int state);
void fwl_setPromptForFile(int state);

char *fwg_frontEndWantsFileName(void);
int fv_display_initialize(void);
void fwl_initializeRenderSceneUpdateScene(void);
void fwl_setButDown(int button, int value);


/* IS - moving from main/headers.h to here for use in front-ends (bin/main.c calls some of these) */
void setMenuButton_collision (int val);
void setMenuButton_headlight (int val);
void setMenuButton_navModes (int type);
void setMenuButton_texSize (int size);
int fwl_get_headlight();
char* fwl_getNavModeStr();
int fwl_getNavMode();
int	fwl_getCollision();
void fwl_setCollision(int state);
int fwl_getAnaglyphSide(int whichSide);

#endif /* __LIBFREEWRL_API_H__ */
