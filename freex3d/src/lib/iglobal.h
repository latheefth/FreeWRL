/*
The globals have 'private' and 'public' facilities
1. if the variables are used only in one source file, they go 
    in a ppFileName struct in Filename.c
2. if the variables are used in other source files (via extern) then either:
2a. add getter and setter functions and keep private, or
2b. add directly to iiglobal struct in iglobal.h, in the struct tFileName sub-struct
2bi. in this case if its a pointer to a complex type, put as void* in iglobal and 
     add casting to the code (so iglobal.h doesn't have to #include a bunch of headers)
 
Variable initialization:
1. for private: in a //private section in FileName_init(..) p->variable = const value
2. for public: in a //public section in FileName_init(..) t->variable = const value
 
Variable use:
1. for private: ppFileName p = (ppFileName)gglobal()->FileName.prv;
-- p->variable = ...
2. for public:  gglobal()->FileName.variable = ...

*/

#ifndef INSTANCEGLOBAL
//#include "display.h" //for opengl_utils.h which is for rdr_caps
//#include "opengl/OpenGL_Utils.h"  //for rdr_caps
#include "list.h"
#ifdef DISABLER
#include "dbl_list.h"
#endif
#include <system.h>
//#include <libFreeWRL.h>
#include <pthread.h>
#include <threads.h> //for threads
//#define GLenum int
//#define GLuint unsigned int
//#include "vrml_parser/Structs.h" //for SFColor
//#include "x3d_parser/X3DParser.h" //for PARENTSTACKSIZE
//#include "ui/common.h" // for ppcommon



typedef struct iiglobal //InstanceGlobal
{
	struct tdisplay{
		void *params; //freewrl_params_t
		int _global_gl_err; //GLenum
		bool display_initialized;// = FALSE;
		int screenWidth;// = 0; /* screen */
		int screenHeight;// = 0;
		char *window_title;// = NULL;
		int shutterGlasses;// = 0; /* stereo shutter glasses */
		void *rdr_caps; //s_renderer_capabilities_t
		void *prv;
	}display;
	struct tinternalc {
		bool global_strictParsing;// = FALSE;
		bool global_plugin_print;// = FALSE;
		bool global_occlusion_disable;// = FALSE;
		unsigned user_request_texture_size;// = 0;
		bool global_print_opengl_errors;// = FALSE;
		bool global_trace_threads;// = FALSE;
		void *prv;
	} internalc;
	//struct tio_http {
	//	void *prv;
	//} io_http;
	struct tresources {
		//resource_item_t *root_res; // = NULL;
		void *root_res;
		void *prv;
	} resources;
	struct tthreads {
        pthread_t disposeThread;
		pthread_t mainThread; /* main (default) thread */
		pthread_t DispThrd; /*DEF_THREAD(DispThrd); display thread */
		pthread_t PCthread; /* DEF_THREAD(PCthread)parser thread */
		pthread_t loadThread; /* DEF_THREAD(pthread_t loadThread)texture thread */
		/* Synchronize / exclusion root_res and below */
		pthread_mutex_t mutex_resource_tree; // = PTHREAD_MUTEX_INITIALIZER;

		/* Synchronize / exclusion : resource queue for parser */
		pthread_mutex_t mutex_resource_list; // = PTHREAD_MUTEX_INITIALIZER;
		pthread_cond_t resource_list_condition; // = PTHREAD_COND_INITIALIZER;

		pthread_mutex_t mutex_frontend_list; // = PTHREAD_MUTEX_INITIALIZER;

		/* Synchronize / exclusion (main<=>texture) */
		pthread_mutex_t mutex_texture_list; // = PTHREAD_MUTEX_INITIALIZER;
		pthread_cond_t texture_list_condition; // = PTHREAD_COND_INITIALIZER;
		BOOL ResourceThreadRunning;
		BOOL TextureThreadRunning;
		BOOL ResourceThreadWaiting;
		BOOL TextureThreadWaiting;
		int MainLoopQuit;
		int flushing;
		void *prv;
	} threads;
    
	struct tSnapshot {
		bool doSnapshot;
		bool doPrintshot;
		int snapGoodCount;
		void *prv;
	} Snapshot;
	struct tEAI_C_CommonFunctions {
		int eaiverbose;// = FALSE;
		void *prv;
	} EAI_C_CommonFunctions;
	struct tEAIEventsIn{
		void *prv;
	} EAIEventsIn;
	struct tEAIHelpers{
		char *outBuffer;
		int outBufferLen;
		void *prv;
	} EAIHelpers;
	struct tEAICore{
		/* EAI input buffer */
		char *EAIbuffer;
		int EAIbufcount;				/* pointer into buffer*/
		int EAIbufpos;
		int EAIbufsize;				/* current size in bytes of input buffer*/
		char *EAIListenerData; /* this is the location for getting Listenered data back again.*/
		void *prv;
	} EAICore;
	struct tSensInterps{
		void *prv;
	} SensInterps;
	struct tConsoleMessage{
		int consMsgCount;
		int Console_writeToHud;
		void *prv;
	} ConsoleMessage;
	struct tMainloop{
		float gl_linewidth;
		/* what kind of file was just parsed? */
		int currentFileVersion;
		double TickTime;
		double lastTime;
		double BrowserFPS;// = 100.0;        /* calculated FPS               */
		double BrowserSpeed;// = 0.0;      /* calculated movement speed    */
		const char *BrowserDescription;
		int HaveSensitive;// = FALSE;
		int AllowNavDrag;
		int trisThisLoop;
		int clipPlane;// = 0;
		int SHIFT; //state of shift key up = 0, down = 1
		int CTRL; //state of ctrl key up = 0, down = 1
		//int currentX[20], currentY[20];                 /*  current mouse position.*/
		void *prv;
		char *tmpFileLocation;
		char *url;
		char *scene_name; //null or take from url
		char *scene_suff; //null or wrl or x3d
		int  scene_profile; //from parser (or capabilities handler) used in js scene.profile
		int *scene_components;
		char *replaceWorldRequest;
		void *replaceWorldRequestMulti; //will be struct multi-string
		void *_vportstack; //Stack for viewports
		void *_stagestack; //stack for stage ID
		void *_framebufferstack; //stack for backbuffers, usually GL_BACK, or can be FBO
		int screenOrientation2;
		int pickray_x;
		int pickray_y;
	} Mainloop;
	struct tProdCon{
		struct Vector *viewpointNodes;// = NULL;
		int currboundvpno;//=0;
		/* bind nodes in display loop, NOT in parsing threadthread */
		struct X3D_Node *setViewpointBindInRender;// = NULL;
		struct X3D_Node *setFogBindInRender;// = NULL;
		struct X3D_Node *setBackgroundBindInRender;// = NULL;
		struct X3D_Node *setNavigationBindInRender;// = NULL;
		void *savedParser; //struct VRMLParser* savedParser;
#ifdef DISABLER		
#ifdef FRONTEND_GETS_FILES
		void (*_frontEndOnResourceRequiredListener)(char *);
#endif
		void (*_frontEndOnX3DFileLoadedListener)(char *);
#endif		
		void *prv;
	} ProdCon;
       #if defined (INCLUDE_NON_WEB3D_FORMATS)
	struct tColladaParser{
		void *prv;
	}ColladaParser;
       #endif //INCLUDE_NON_WEB3D_FORMATS

	#if defined (INCLUDE_STL_FILES)
	struct tSTLHandler {
		void *prv;
	}STLHandler;
	#endif // INCLUDE_STL_FILES


	struct tFrustum{
		int OccFailed;//. = FALSE;
		void *prv;
	} Frustum;
	struct tLoadTextures{
		/* is the texture thread up and running yet? */
		//int TextureThreadInitialized;// = FALSE;
		void *prv;
	}LoadTextures;
	struct tOpenGL_Utils{
		/* is this 24 bit depth? 16? 8?? Assume 24, unless set on opening */
		int displayDepth;// = 24;
		//static float cc_red = 0.0f, cc_green = 0.0f, cc_blue = 0.0f, cc_alpha = 1.0f;
		int cc_changed;// = FALSE;
		void *prv;
	}OpenGL_Utils;

#ifdef HAVE_OPENCL
        struct tOpenCL_Utils{
                bool OpenCL_Initialized; // = FALSE;
                bool OpenCL_OK; // = FALSE
                void *prv;
        }OpenCL_Utils;
#endif //HAVE_OPENCL

#ifdef STATUSBAR_STD
	struct tRasterFont{
		void *prv;
	}RasterFont;
#endif
	struct tRenderTextures{
		//struct multiTexParams textureParameterStack[MAX_MULTITEXTURE];
		void *textureParameterStack;
		void *prv;
	}RenderTextures;
	struct tTextures{
		/* for texture remapping in TextureCoordinate nodes */
		//GLuint	*global_tcin;
		unsigned int *global_tcin;
		int	global_tcin_count;
		void 	*global_tcin_lastParent;
		//GLuint defaultBlankTexture;
		unsigned int defaultBlankTexture;
		void *prv;
	}Textures;
	struct tPluginSocket{
		void *prv;
	}PluginSocket;
	struct tpluginUtils{
		void *prv;
	}pluginUtils;
	struct tcollision{
		void *prv;
	}collision;
	struct tComponent_EnvironSensor{
		void *prv;
	}Component_EnvironSensor;
	struct tComponent_Geometry3D{
		void *prv;
	}Component_Geometry3D;
	struct tComponent_Geospatial{
		void *prv;
	}Component_Geospatial;
	struct tComponent_HAnim{
		void *prv;
	}Component_HAnim;
	struct tComponent_Layering{
		void *prv;
	}Component_Layering;
	struct tComponent_Layout{
		void *prv;
	}Component_Layout;
	struct tComponent_NURBS{
		void *prv;
	}Component_NURBS;
	struct tComponent_ParticleSystems{
		void *prv;
	}Component_ParticleSystems;
	struct tComponent_RigidBodyPhysics{
		void *prv;
	}Component_RigidBodyPhysics;
	struct tComponent_Followers{
		void *prv;
	}Component_Followers;
	struct tComponent_KeyDevice{
		void *prv;
	}Component_KeyDevice;

#ifdef OLDCODE
iOLDCODE	struct tComponent_Networking{
iOLDCODE		void *ReWireNamenames;
iOLDCODE		int ReWireNametableSize;
iOLDCODE		void *ReWireDevices;
iOLDCODE		int ReWireDevicetableSize;
iOLDCODE		void *prv;
iOLDCODE	}Component_Networking;
#endif // OLDCODE

#ifdef DJTRACK_PICKSENSORS
	struct tComponent_Picking{
		void *prv;
	}Component_Picking;
#endif
	struct tComponent_Shape{
		void *prv;
	}Component_Shape;
	struct tComponent_Sound{
		int sound_from_audioclip;// = 0;
		/* is the sound engine started yet? */
		int SoundEngineStarted;// = FALSE;
		void *prv;
	}Component_Sound;
	struct tComponent_Text{
		void *prv;
	}Component_Text;
	struct tComponent_VRML1{
		void *prv;
	}Component_VRML1;
	struct tRenderFuncs{
		#ifdef OLDCODE
		OLDCODE char *OSX_last_world_url_for_reload;
		OLDCODE char *OSX_replace_world_from_console;
		#endif //OLDCODE

		/* Any action for the Browser to do? */
		int BrowserAction;// = FALSE;
		double hitPointDist; /* distance in ray: 0 = r1, 1 = r2, 2 = 2*r2-r1... */
		/* used to save rayhit and hyperhit for later use by C functions */
		//struct SFColor hyp_save_posn, hyp_save_norm, ray_save_posn;
		float hyp_save_posn[3];
		float hyp_save_norm[3];
		float ray_save_posn[3]; //getRayHit() > last intersection of pickray/bearing with geometry, transformed into the coordinates of the geometry
		void *hypersensitive;//= 0; 
		int hyperhit;// = 0;
		//struct point_XYZ hp;
		void *hp;
		void *rayHit;
		//void *rayHitHyper;
		//struct point_XYZ t_r1,t_r2,t_r3; /* transformed ray */
		//void *t_r123; /* transformed ray */
		int	lightingOn;		/* do we need to restore lighting in Shape? */
		int	have_transparency;//=FALSE;/* did any Shape have transparent material? */
		/* material node usage depends on texture depth; if rgb (depth1) we blend color field
		   and diffusecolor with texture, else, we dont bother with material colors */
		int last_texture_type;// = NOTEXTURE;
		/* texture stuff - see code. Need array because of MultiTextures */
		//GLuint boundTextureStack[10];//MAX_MULTITEXTURE];
		unsigned int boundTextureStack[10];//MAX_MULTITEXTURE];
		int textureStackTop;
		void *prv;
	}RenderFuncs;
	struct tStreamPoly{
		void *prv;
	}StreamPoly;
	struct tTess{
		int *global_IFS_Coords;
		int global_IFS_Coord_count;//=0;
		//GLUtriangulatorObj *global_tessobj;
		void *global_tessobj;
		void *prv;
	}Tess;
	struct tViewer{
		void *prv;
	}Viewer;
	struct tstatusbar{
		void *prv;
	}statusbar;
	struct tCParse{
		void* globalParser;
		void *prv;
	}CParse;
	struct tCParseParser{
		void *prv;
	}CParseParser;
	struct tCProto{
		void *prv;
	}CProto;
	struct tCRoutes{
		/* EAI needs the extra parameter, so we put it globally when a RegisteredListener is clicked. */
		int CRoutesExtra;// = 0;
		//jsval JSglobal_return_val;
		void *JSSFpointer;
		int *scr_act;// = 0;				/* this script has been sent an eventIn */
		int max_script_found;// = -1;			/* the maximum script number found */
		int max_script_found_and_initialized;// = -1;	/* the maximum script number found */
		int jsnameindex; //= -1;
		int MAXJSparamNames;// = 0;

		void *prv;
	}CRoutes;
	struct tCScripts{
		void *prv;
	}CScripts;
	struct tJScript{
		void * JSglobal_return_val;
		void *prv;
	}JScript;
	struct tjsUtils{
		void *prv;
	}jsUtils;
	struct tjsVRMLBrowser{
		/* for setting field values to the output of a CreateVrml style of call */
		/* it is kept at zero, unless it has been used. Then it is reset to zero */
		void * JSCreate_global_return_val;
		void *prv;
	}jsVRMLBrowser;
	struct tjsVRMLClasses{
		void *prv;
	}jsVRMLClasses;
	struct tBindable{
		//struct sNaviInfo naviinfo;
  //      struct Vector *background_stack;
  //      struct Vector *viewpoint_stack;
  //      struct Vector *navigation_stack;
  //      struct Vector *fog_stack;
		void *naviinfo;
        //void *background_stack;
        //void *viewpoint_stack;
        //void *navigation_stack;
        //void *fog_stack;
		int activeLayer;
		void *bstacks;
		void *prv;
	}Bindable;
	struct tX3DParser{
		int parentIndex;// = -1;
		//struct X3D_Node *parentStack[PARENTSTACKSIZE];
		char *CDATA_Text;// = NULL;
		int CDATA_Text_curlen;// = 0;
		void *prv;
	}X3DParser;
	struct tX3DProtoScript{
		void *prv;
	}X3DProtoScript;
	struct tcommon{
		void *prv;
	}common;
	struct tCursorDraw{
		void *prv;
	}CursorDraw;
#ifdef DISABLER	
#if defined(WRAP_MALLOC) || defined(DEBUG_MALLOC)
    pthread_mutex_t __memTableGlobalLock;
    bool __memTable_CheckInit;
    bool __memTable_ShouldRegisterAllocation;
    dbl_list_t *__memTable;
#endif
#endif
} * ttglobal;
#define INSTANCEGLOBAL 1
#endif
ttglobal  iglobal_constructor(); 
void iglobal_destructor(ttglobal);
//void set_thread2global(ttglobal fwl, pthread_t any , char *desc);
void resetGGlobal();
ttglobal gglobal(); //gets based on threadID, errors out if no threadID
//ppcommon gglobal_common(); // lets the front end get the myMenuStatus without hassle. dug9 Mar2014: poll for the values with get_status, get_... in common.c
ttglobal gglobal0(); //will return null if thread not yet initialized
//ttglobal gglobalH(void *handle); //use window handle
//ttglobal gglobalH0(void *handle); //test if window handle is in the table yet
