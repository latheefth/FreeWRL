/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/



#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stddef.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>

/* get the definitions from the command line */
#include "vrmlconf.h"

void *freewrlMalloc(int line, char *file, size_t sz);
void *freewrlRealloc (int line, char *file, void *ptr, size_t size);
void *freewrlStrdup (int line, char *file, char *str);
#define MALLOC(sz) FWMALLOC (__LINE__,__FILE__,sz)
#define FWMALLOC(l,f,sz) freewrlMalloc(l,f,sz)
#define REALLOC(a,b) freewrlRealloc(__LINE__,__FILE__,a,b)
#define STRDUP(a) freewrlStrdup(__LINE__,__FILE__,a)
#ifdef DEBUG_MALLOC
	/* free a malloc'd pointer */
	void freewrlFree(int line, char *file, void *a);
	#define FREE_IF_NZ(a) if(a) {freewrlFree(__LINE__,__FILE__,a); a = 0;}

#else 
	#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}
#endif

#define ID_UNDEFINED -1

/* stop the display thread. Used (when this comment was made) by the OSX Safari plugin; keeps
most things around, just stops display thread, when the user exits a world. */
#define STOP_DISPLAY_THREAD \
        if (DispThrd != NULL) { \
                quitThread = TRUE; \
                pthread_join(DispThrd,NULL); \
                DispThrd = NULL; \
        }


/* vrmlconf.h should have the AQUA definition - so get the GL headers */
#ifdef AQUA
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

/* now get all of our structures */
#include "Structs.h"

typedef struct _CRnodeStruct {
        struct X3D_Node *routeToNode;
        unsigned int foffset;
} CRnodeStruct;

/* Size of static array */
#define ARR_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

/* Some stuff for routing */
#define FROM_SCRIPT 1
#define TO_SCRIPT 2
#define SCRIPT_TO_SCRIPT 3

/* Helper to get size of a struct's memer */
#define sizeof_member(str, var) \
 sizeof(((str*)NULL)->var)

/* C routes */
#define MAXJSVARIABLELENGTH 25	/* variable name length can be this long... */

struct CRStruct {
        struct X3D_Node*  routeFromNode;
        uintptr_t fnptr;
        unsigned int tonode_count;
        CRnodeStruct *tonodes;
        int     isActive;
        int     len;
        void    (*interpptr)(void *); /* pointer to an interpolator to run */
        int     direction_flag; /* if non-zero indicates script in/out,
                                                   proto in/out */
        int     extra;          /* used to pass a parameter (eg, 1 = addChildren..) */
};
struct CRjsnameStruct {
        int     	type;
        char    	name[MAXJSVARIABLELENGTH];
	uintptr_t 	eventInFunction;		/* compiled javascript function... if it is required */
};


extern struct CRjsnameStruct *JSparamnames;
extern struct CRStruct *CRoutes;
extern int jsnameindex;
extern int MAXJSparamNames;



/* To allow BOOL for boolean values */
#define BOOL	int

/* multi-threaded OpenGL contexts - works on OS X, kind of ok on Linux, but
   blows plugins out of the water, because of the XLib threaded call in FrontEnd
   not working that well... */
#ifdef AQUA
	#define DO_MULTI_OPENGL_THREADS
#endif

/* if we want to see our opengl errors, define this and recompile everything. */
#undef GLERRORS

/* display the BoundingBoxen */
#undef DISPLAYBOUNDINGBOX

/* rendering constants used in SceneGraph, etc. */
#define VF_Viewpoint 				0x0001
#define VF_Geom 				0x0002
#define VF_DirectionalLight			0x0004 
#define VF_Sensitive 				0x0008
#define VF_Blend 				0x0010
#define VF_Proximity 				0x0020
#define VF_Collision 				0x0040
#define VF_hasSensitiveChildren			0x0080
#define VF_hasVisibleChildren 			0x0100
#define VF_hasGeometryChildren 			0x0200
#define VF_hasBeenScannedForGeometryChildren	0x0400
#define VF_otherLight				0x0800 

/* compile simple nodes (eg, Cone, LineSet) into an internal format. Check out
   CompileC in VRMLRend.pm, and look for compile_* functions in code. General
   meshes are rendered using the PolyRep scheme, which also compiles into OpenGL 
   calls, using the PolyRep (and, stream_PolyRep) methodology */

int isShapeCompilerParsing(void);
void compile_polyrep(void *node, void *coord, void *color, void *normal, void *texCoord);
#define COMPILE_POLY_IF_REQUIRED(a,b,c,d) \
                if(!node->_intern || node->_change != ((struct X3D_PolyRep *)node->_intern)->_change) { \
                        compileNode ((void *)compile_polyrep, node, a,b,c,d); \
		} \
		if (!node->_intern) return;

#define COMPILE_IF_REQUIRED { struct X3D_Virt *v; \
	if (node->_ichange != node->_change) { \
	/* printf ("COMP %d %d\n",node->_ichange, node->_change); */ \
		v = *(struct X3D_Virt **)node; \
		if (v->compile) { \
			compileNode (v->compile, (void *)node, NULL, NULL, NULL, NULL); \
		} else {printf ("huh - have COMPIFREQD, but v->compile null for %s\n",stringNodeType(node->_nodeType));} \
		} \
		if (node->_ichange == 0) return; \
	}


/* convert a PROTO node (which will be a Group node) into a node. eg, for Materials  - this is a possible child
node for ANY node that takes something other than a Group */
#define POSSIBLE_PROTO_EXPANSION(inNode,outNode) \
	if (inNode == NULL) outNode = NULL; \
	else {if (X3D_NODE(inNode)->_nodeType == NODE_Group) { \
		if (X3D_GROUP(inNode)->children.n>0) { \
			outNode = X3D_GROUP(inNode)->children.p[0]; \
		} else outNode = NULL; \
	} else outNode = inNode; };

#define MARK_NODE_COMPILED node->_ichange = node->_change;
/* end of compile simple nodes code */

void startOfLoopNodeUpdates(void);
void OcclusionCulling (void);
void OcclusionStartofEventLoop(void);
extern int HaveSensitive;
void zeroVisibilityFlag(void);
void setField_fromJavascript (struct X3D_Node *ptr, char *field, char *value);
unsigned int setField_method2 (char *ptr);
void setField_javascriptEventOut(struct X3D_Node  *tn,unsigned int tptr, int fieldType, unsigned len, int extraData, uintptr_t mycx);

extern char *GL_VEN;
extern char *GL_VER;
extern char *GL_REN;

/* do we have GL Occlusion Culling? */
#ifdef AQUA
	#define OCCLUSION
        #define VISIBILITYOCCLUSION
        #define SHAPEOCCLUSION
        #define glGenQueries(a,b) glGenQueriesARB(a,b)
        #define glDeleteQueries(a,b) glDeleteQueriesARB(a,b)
#else
	/* on Linux, test to see if we have this defined. */
	#ifdef HAVE_GL_QUERIES_ARB
		#define OCCLUSION
		#define VISIBILITYOCCLUSION
		#define SHAPEOCCLUSION
		#define glGenQueries(a,b) glGenQueriesARB(a,b)
		#define glDeleteQueries(a,b) glDeleteQueriesARB(a,b)
	#else 
		#undef OCCLUSION
		#undef VISIBILITYOCCLUSION
		#undef SHAPEOCCLUSION
		#define glGenQueries(a,b)
		#define glDeleteQueries(a,b)
	#endif
#endif

#define glIsQuery(a) glIsQueryARB(a)
#define glBeginQuery(a,b) glBeginQueryARB(a,b)
#define glEndQuery(a) glEndQueryARB(a)
#define glGetQueryiv(a,b,c) glGetQueryivARB(a,b,c)
#define glGetQueryObjectiv(a,b,c) glGetQueryObjectivARB(a,b,c)
#define glGetQueryObjectuiv(a,b,c) glGetQueryObjectuivARB(a,b,c)

extern int OccQuerySize;
extern int OccFailed;
extern int *OccCheckCount;
extern int *OccNodeRendered;
extern GLuint *OccQueries;
extern void * *OccNodes;
extern GLint *OccSamples;
int newOcclude(void);
void zeroOcclusion(void);
extern int QueryCount;

#ifdef OCCLUSION
#define OCCLUSIONTEST \
	/* a value of ZERO means that it HAS visible children - helps with initialization */ \
        if ((render_geom!=0) | (render_sensitive!=0)) { \
		/* printf ("node %d fl %x\n",node, node->_renderFlags & VF_hasVisibleChildren); */ \
                if ((node->_renderFlags & VF_hasVisibleChildren) == 0) { \
                        /* printf ("WOW - we do NOT need to do this transform %x!\n",(node->_renderFlags & VF_hasVisibleChildren)); */ \
                        return; \
                } \
        } 
#else
#define OCCLUSIONTEST
#endif



#define BEGINOCCLUSIONQUERY \
				if (render_geom) { \
                                if ((node->__OccludeNumber >=0) && (node->__OccludeNumber < QueryCount)) { \
					/* Occlude table is large enough now to add this entry */ \
					if (OccCheckCount[node->__OccludeNumber]<0) {\
                                        	glBeginQuery(GL_SAMPLES_PASSED,OccQueries[node->__OccludeNumber]); \
						/* printf ("beginning query for %d\n",node->__OccludeNumber); */ \
					}}} 

#define ENDOCCLUSIONQUERY \
			if (render_geom) { \
                        if ((node->__OccludeNumber >=0) && (node->__OccludeNumber < QueryCount)) { \
					if (OccCheckCount[node->__OccludeNumber]<0) {\
                                		glEndQuery(GL_SAMPLES_PASSED);   \
						/* printf ("ending query for %d\n",node->__OccludeNumber); */ \
					}}} 

/* bounding box calculations */
#define EXTENTTOBBOX    node->bboxSize.c[0] = node->EXTENT_MAX_X; \
                        node->bboxSize.c[1] = node->EXTENT_MAX_Y; \
                        node->bboxSize.c[2] = node->EXTENT_MAX_Z;

#define INITIALIZE_EXTENT        node->EXTENT_MAX_X = -10000.0; \
        node->EXTENT_MAX_Y = -10000.0; \
        node->EXTENT_MAX_Z = -10000.0; \
        node->EXTENT_MIN_X = 10000.0; \
        node->EXTENT_MIN_Y = 10000.0; \
        node->EXTENT_MIN_Z = 10000.0;

/********************************
	Verbosity
*********************************/
/* Parsing & Lexing */
#undef CPARSERVERBOSE 

/* Java Class invocation */
#undef JSVRMLCLASSESVERBOSE

/* child node parsing */
#undef CHILDVERBOSE

/* routing */
#undef CRVERBOSE

/* Javascript */
#undef JSVERBOSE

/* sensitive events */
#undef SEVERBOSE

/* Text nodes */
#undef TEXTVERBOSE

/* Texture processing */
#undef TEXVERBOSE

/* streaming from VRML to OpenGL internals. */
#undef STREAM_POLY_VERBOSE

/* collision */
#undef COLLISIONVERBOSE

/* Capabilities of x3dv and x3d */
#undef CAPABILITIESVERBOSE

/* External Authoring Interface */
extern int eaiverbose;

#ifndef AQUA
#include <GL/glu.h>
#else
#include <glu.h>
#include <CGLTypes.h>
#include <AGL/AGL.h>
#include "aquaInt.h"
extern CGLContextObj myglobalContext;
extern AGLContext aqglobalContext;
#endif

/* number of tesselated coordinates allowed */
#define TESS_MAX_COORDS  500

#define offset_of(p_type,field) ((unsigned int)(&(((p_type)NULL)->field)-NULL))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define UNUSED(v) ((void) v)
#define ISUSED(v) ((void) v)

#define BOOL_STRING(b) (b ? "TRUE" : "FALSE")

#ifdef M_PI
#define PI M_PI
#else
#define PI 3.141592653589793
#endif
/* return TRUE if numbers are very close */
#define APPROX(a,b) (fabs(a-b)<0.00000001)
/* defines for raycasting: */

#define NORMAL_VECTOR_LENGTH_TOLERANCE 0.00001
/* (test if the vector part of a rotation is normalized) */
#define IS_ROTATION_VEC_NOT_NORMAL(rot)        ( \
       fabs(1-sqrt(rot.r[0]*rot.r[0]+rot.r[1]*rot.r[1]+rot.r[2]*rot.r[2])) \
               >NORMAL_VECTOR_LENGTH_TOLERANCE \
)

/* from VRMLC.pm */
extern int sound_from_audioclip;
extern int have_texture;
extern int global_lineProperties;
extern int global_fillProperties;
extern int fullscreen;
extern float gl_linewidth;
extern int soundWarned;
extern int cur_hits;
extern struct pt hyper_r1,hyper_r2;

extern struct X3D_Text *lastTextNode;

/* defines for raycasting: */
#define XEQ (APPROX(t_r1.x,t_r2.x))
#define YEQ (APPROX(t_r1.y,t_r2.y))
#define ZEQ (APPROX(t_r1.z,t_r2.z))
/* xrat(a) = ratio to reach coordinate a on axis x */
#define XRAT(a) (((a)-t_r1.x)/(t_r2.x-t_r1.x))
#define YRAT(a) (((a)-t_r1.y)/(t_r2.y-t_r1.y))
#define ZRAT(a) (((a)-t_r1.z)/(t_r2.z-t_r1.z))
/* mratx(r) = x-coordinate gotten by multiplying by given ratio */
#define MRATX(a) (t_r1.x + (a)*(t_r2.x-t_r1.x))
#define MRATY(a) (t_r1.y + (a)*(t_r2.y-t_r1.y))
#define MRATZ(a) (t_r1.z + (a)*(t_r2.z-t_r1.z))
/* trat: test if a ratio is reasonable */
#undef TRAT
#define TRAT(a) 1
#undef TRAT
#define TRAT(a) ((a) > 0 && ((a) < hpdist || hpdist < 0))

/* POLYREP stuff */
#define POINT_FACES	16 /* give me a point, and it is in up to xx faces */

/* Function Prototypes */

void render_node(struct X3D_Node *node);

void rayhit(float rat, float cx,float cy,float cz, float nx,float ny,float nz,
float tx,float ty, char *descr) ;

void fwnorprint (float *norm);


/* not defined anywhere: */
/* void Extru_init_tex_cap_vals(); */


/* from the PNG examples */
unsigned char  *readpng_get_image(double display_exponent, int *pChannels,
		                       unsigned long *pRowbytes);

/* Used to determine in Group, etc, if a child is a DirectionalLight; do comparison with this */
void DirectionalLight_Rend(void *nod_);
#define DIRLIGHTCHILDREN(a) \
	if ((node->_renderFlags & VF_DirectionalLight)==VF_DirectionalLight)dirlightChildren(a);

#define DIRECTIONAL_LIGHT_OFF if ((node->_renderFlags & VF_DirectionalLight)==VF_DirectionalLight) { \
		lightState(savedlight+1,FALSE); curlight = savedlight; }
#define DIRECTIONAL_LIGHT_SAVE int savedlight = curlight;

void normalize_ifs_face (float *point_normal,
                         struct pt *facenormals,
                         int *pointfaces,
                        int mypoint,
                        int curpoly,
                        float creaseAngle);


void FW_rendertext(unsigned int numrows,struct Uni_String **ptr,char *directstring, unsigned int nl, double *length,
                double maxext, double spacing, double mysize, unsigned int fsparam,
                struct X3D_PolyRep *rp);


/* Triangulator extern defs - look in CFuncs/Tess.c */
extern struct X3D_PolyRep *global_tess_polyrep;
extern GLUtriangulatorObj *global_tessobj;
extern int global_IFS_Coords[];
extern int global_IFS_Coord_count;

/* do we have to do textures?? */
#define HAVETODOTEXTURES (texture_count != 0)

/* multitexture and single texture handling */
#define MAX_MULTITEXTURE 10

/* texture stuff - see code. Need array because of MultiTextures */
extern GLuint bound_textures[MAX_MULTITEXTURE];
extern int bound_texture_depths[MAX_MULTITEXTURE];
extern GLint maxTexelUnits;
extern int texture_count; 
extern int     *global_tcin;
extern int     global_tcin_count; 
extern void 	*global_tcin_lastParent;

extern void textureDraw_start(struct X3D_IndexedFaceSet *texC, GLfloat *tex);
extern void textureDraw_end(void);

extern void * this_textureTransform;  /* do we have some kind of textureTransform? */

extern int isTextureLoaded(int texno);
extern int displayDepth;
extern int display_status;

extern int _fw_pipe, _fw_FD;
extern int _fw_browser_plugin;
#define RUNNINGASPLUGIN (isBrowserPlugin)

#define RUNNINGONAMD64 (sizeof(void *) == 8)

/* appearance does material depending on last texture depth */
extern int last_texture_depth;
extern float last_transparency;


/* what is the max texture size as set by FreeWRL? */
extern GLint global_texSize;


/* Text node system fonts. On startup, freewrl checks to see where the fonts
 * are stored
 */
#define fp_name_len 256
extern char sys_fp[fp_name_len];


extern float AC_LastDuration[];

extern int SoundEngineStarted;

/* Material optimizations */
void do_shininess (float shininess);
void do_glMaterialfv (GLenum face, GLenum pname, GLfloat *param);

/* used to determine whether we have transparent materials. */
extern int have_transparency;


/* current time */
extern double TickTime;
extern double lastTime;

/* number of triangles this rendering loop */
extern int trisThisLoop;


/* Transform node optimizations */
int verify_rotate(GLfloat *params);
int verify_translate(GLfloat *params);
int verify_scale(GLfloat *params);

void mark_event (struct X3D_Node *from, unsigned int fromoffset);
void mark_event_check (struct X3D_Node *from, unsigned int fromoffset,char *fn, int line);

/* saved rayhit and hyperhit */
extern struct SFColor ray_save_posn, hyp_save_posn, hyp_save_norm;

/* set a node to be sensitive */
void setSensitive(struct X3D_Node *parent,struct X3D_Node *me);

/* bindable nodes */
extern GLint viewport[];
extern GLdouble fieldofview;
extern struct pt ViewerUpvector;
extern struct sNaviInfo naviinfo;


/* Sending events back to Browser (eg, Anchor) */
extern int BrowserAction;
extern struct X3D_Anchor *AnchorsAnchor;
extern uintptr_t _fw_instance;
int checkIfX3DVRMLFile(char *fn);
void Anchor_ReplaceWorld (char *fn);
void EAI_Anchor_Response (int resp);
extern int wantEAI;
struct Uni_String *newASCIIString(char *str);
void verify_Uni_String(struct  Uni_String *unis, char *str);

void *returnInterpolatorPointer (const char *x);

/* SAI code node interface return values  The meanings of
   these numbers can be found in the SAI java code */
#define X3DBoundedObject			1
#define X3DBounded2DObject 		 	2
#define X3DURLObject 			 	3
#define X3DAppearanceNode 			10
#define X3DAppearanceChildNode 			11
#define X3DMaterialNode 			12
#define X3DTextureNode 				13
#define X3DTexture2DNode 			14
#define X3DTexture3DNode 			15
#define X3DTextureTransformNode  		16
#define X3DTextureTransform2DNode 		17
#define X3DGeometryNode 			18
#define X3DTextNode 				19
#define X3DParametricGeometryNode 		20
#define X3DGeometricPropertyNode 		21
#define X3DColorNode				22
#define X3DCoordinateNode 			23
#define X3DNormalNode 				24
#define X3DTextureCoordinateNode 		25
#define X3DFontStyleNode 			26
#define X3DProtoInstance 			27
#define X3DChildNode 				28
#define X3DBindableNode 			29
#define X3DBackgroundNode 			30
#define X3DGroupingNode 			31
#define X3DShapeNode 				32
#define X3DInterpolatorNode 			33
#define X3DLightNode 				34
#define X3DScriptNode 				35
#define X3DSensorNode 				36
#define X3DEnvironmentalSensorNode 		37
#define X3DKeyDeviceSensorNode 			38
#define X3DNetworkSensorNode 			39
#define X3DPointingDeviceSensorNode 		40
#define X3DDragSensorNode 			41
#define X3DTouchSensorNode 			42
#define X3DSequencerNode  			43
#define X3DTimeDependentNode 			44
#define X3DSoundSourceNode 			45
#define X3DTriggerNode 				46
#define X3DInfoNode 				47

void CRoutes_js_new (uintptr_t num,int scriptType);
extern int max_script_found;
extern int max_script_found_and_initialized;
void getMFNodetype (char *strp, struct Multi_Node *ch, struct X3D_Node *par, int ar);
void AddRemoveChildren (struct X3D_Node *parent, struct Multi_Node *tn, uintptr_t *nodelist, int len, int ar);

void update_node(struct X3D_Node *ptr);
void update_renderFlag(struct X3D_Node *ptr, int flag);

int JSparamIndex (char *name, char *type);

/* setting script eventIns from routing table or EAI */
void Set_one_MultiElementtype (uintptr_t tn, uintptr_t tptr, void *fn, unsigned len);
void set_one_ECMAtype (uintptr_t tonode, int toname, int dataType, void *Data, unsigned datalen);
void mark_script (uintptr_t num);


/* structure for rayhits */
struct currayhit {
	struct X3D_Node *node; /* What node hit at that distance? */
	GLdouble modelMatrix[16]; /* What the matrices were at that node */
	GLdouble projMatrix[16];
};



struct CRscriptStruct {
	/* type */
	int thisScriptType;

	/* Javascript parameters */
	uintptr_t	cx;			/* JSContext		*/
	uintptr_t	glob;			/* JSGlobals		*/
	uintptr_t	eventsProcessed; 	/* eventsProcessed() compiled function parameter*/

	/* Java .CLASS parameters */
	unsigned int 	_initialized; 	/* has initialize been sent? */
	int listen_fd, send_fd;		/* socket descriptors */
	char NodeID[20];		/* combo Perl NODEXXX, and CNODE */

};
void JSMaxAlloc(void);
void cleanupDie(uintptr_t num, const char *msg);
void shutdown_EAI(void);
uintptr_t EAI_GetNode(const char *str);
unsigned int EAI_GetViewpoint(const char *str);
void EAI_killBindables (void);
void EAI_GetType (uintptr_t cNode,  char *ctmp, char *dtmp, uintptr_t *cNodePtr, uintptr_t *fieldOffset,
                        uintptr_t *dataLen, uintptr_t *typeString,  unsigned int *scripttype, int *accessType);


void setScriptECMAtype(uintptr_t);
int get_touched_flag(uintptr_t fptr, uintptr_t actualscript);
void getMultiElementtype(char *strp, struct Multi_Vec3f *tn, int eletype);
void setScriptMultiElementtype(uintptr_t);
void Parser_scanStringValueToMem(struct X3D_Node *ptr, int coffset, int ctype, char *value);
void Multimemcpy(void *tn, void *fn, int len);
void CRoutes_RegisterSimple(struct X3D_Node* from, int fromOfs,
 struct X3D_Node* to, int toOfs, int len, int dir);
void CRoutes_Register(int adrem,        struct X3D_Node *from,
                                 int fromoffset,
                                 unsigned int to_count,
                                 char *tonode_str,
                                 int length,
                                 void *intptr,
                                 int scrdir,
                                 int extra);
void CRoutes_free(void);
void propagate_events(void);
int getRoutesCount(void);
void getSpecificRoute (int routeNo, uintptr_t *fromNode, int *fromOffset, 
                uintptr_t *toNode, int *toOffset);
void sendScriptEventIn(uintptr_t num);
void getField_ToJavascript (int num, int fromoffset);
void add_first(struct X3D_Node * node);
void registerTexture(struct X3D_Node * node);
void registerMIDINode(struct X3D_Node *node);
int checkNode(struct X3D_Node *node, char *fn, int line);

void do_first(void);
void process_eventsProcessed(void);

void getEAI_MFStringtype (struct Multi_String *from, struct Multi_String *to);


extern struct CRscriptStruct *ScriptControl; /* Script invocation parameters */
extern uintptr_t *scr_act;    /* script active array - defined in CRoutes.c */
extern int *thisScriptType;    /* what kind of script this is - in CRoutes.c */
extern int JSMaxScript;  /* defined in JSscipts.c; maximum size of script arrays */

void update_status(char* msg);
void kill_status();

/* menubar stuff */
void frontendUpdateButtons(void); /* used only if we are not able to multi-thread OpenGL */
void setMenuButton_collision (int val) ;
void setMenuButton_headlight (int val) ;
void setMenuButton_navModes (int type) ;
void setConsoleMessage(char *stat) ;
void setMenuStatus(char *stat) ;
void setMenuFps (float fps) ;
void setMenuButton_texSize (int size);
extern int textures_take_priority;
void setTextures_take_priority (int x);
extern int useExperimentalParser;
void setUseCParser (int x);
extern int useShapeThreadIfPossible;
void setUseShapeThreadIfPossible(int x);

int convert_typetoInt (const char *type);	/* convert a string, eg "SFBOOL" to type, eg SFBOOL */

extern double BrowserFPS;
extern double BrowserSpeed;
void render_polyrep(void *node);

extern int CRoutesExtra;		/* let EAI see param of routing table - Listener data. */

/* types of scripts. */
#define NOSCRIPT 	0
#define JAVASCRIPT	1
#define SHADERSCRIPT	4

/* printf is defined by perl; causes segfault in threaded freewrl */
#ifdef printf
#undef printf
#endif
#ifdef die
#undef die
#endif


/* types to tell the Perl thread what to handle */
#define FROMSTRING 	1
#define	FROMURL		2
#define INLINE		3
#define ZEROBINDABLES   8   /* get rid of Perl datastructures */
#define FROMCREATENODE	13  /* create a node by just giving its node type */
#define FROMCREATEPROTO	14  /* create a node by just giving its node type */
#define UPDATEPROTOD	16  /* update a PROTO definition */
#define GETPROTOD	17  /* update a PROTO definition */



extern void *rootNode;
extern int isPerlParsing(void);
extern int isURLLoaded(void);	/* initial scene loaded? Robert Sim */
extern int isTextureParsing(void);
extern void loadInline(struct X3D_Inline *node);
extern void loadTextureNode(struct X3D_Node *node,  void *param);
extern void loadMovieTexture(struct X3D_MovieTexture *node,  void *param);
extern void loadMultiTexture(struct X3D_MultiTexture *node);
extern void loadBackgroundTextures (struct X3D_Background *node);
extern void loadTextureBackgroundTextures (struct X3D_TextureBackground *node);
extern GLfloat boxtex[], boxnorms[], BackgroundVert[];
extern GLfloat Backtex[], Backnorms[];

extern void new_tessellation(void);
extern void initializePerlThread(void);
extern void setGeometry (const char *optarg);
extern void setWantEAI(int flag);
extern void setPluginPipe(const char *optarg);
extern void setPluginFD(const char *optarg);
extern void setPluginInstance(const char *optarg);

/* shutter glasses, stereo view  from Mufti@rus */
extern void setShutter (void);
#ifndef AQUA
extern int shutterGlasses;
#endif
extern void setScreenDist (const char *optArg);
extern void setStereoParameter (const char *optArg);
extern void setEyeDist (const char *optArg);

extern int isPerlinitialized(void);

#ifdef HAVE_MOTIF
	#define ISDISPLAYINITIALIZED isMotifDisplayInitialized()
	#define GET_GLWIN getMotifWindowedGLwin (&GLwin);
	#define OPEN_TOOLKIT_MAINWINDOW openMotifMainWindow (argc, argv);
	#define CREATE_TOOLKIT_MAIN_WINDOW createMotifMainWindow();
	int isMotifDisplayInitialized(void);
	void getMotifWindowedGLwin (Window *);
	void openMotifMainWindow (int argc, char ** argv);
	void createMotifMainWindow(void);
#else

	#ifdef HAVE_GTK2
		#define ISDISPLAYINITIALIZED isGtkDisplayInitialized()
		#define GET_GLWIN getGtkWindowedGLwin (&GLwin);
		#define OPEN_TOOLKIT_MAINWINDOW openGtkMainWindow (argc, argv);
		#define CREATE_TOOLKIT_MAIN_WINDOW createGtkMainWindow();
		int isGtkDisplayInitialized(void);
		void getGtkWindowedGLwin (Window *);
		void openGtkMainWindow (int argc, char ** argv);
		void createGtkMainWindow(void);

	#else
		#define HAVE_NOTOOLKIT
		#define ISDISPLAYINITIALIZED TRUE
		#define GET_GLWIN getBareWindowedGLwin (&GLwin);
		#define OPEN_TOOLKIT_MAINWINDOW openBareMainWindow (argc, argv);
		#define CREATE_TOOLKIT_MAIN_WINDOW createBareMainWindow();
	#endif
#endif

extern char *BrowserName, *BrowserFullPath, *cacheFileName, *PluginFullPath; 
extern char *getInputURL(void);
extern char *keypress_string;
extern char *lastReadFile; 		/* name last file read in */
extern int be_collision;		/* toggle collision detection - defined in VRMLC.pm */
extern int  lightingOn;			/* state of GL_LIGHTING */
extern int cullFace;			/* state of GL_CULL_FACE */
extern int colorMaterialEnabled;	/* state of GL_COLOR_MATERIAL */
extern double hpdist;			/* in VRMLC.pm */
extern struct pt hp;			/* in VRMLC.pm */
extern void *hypersensitive; 		/* in VRMLC.pm */
extern int hyperhit;			/* in VRMLC.pm */
extern struct pt r1, r2;		/* in VRMLC.pm */
extern struct sCollisionInfo CollisionInfo;
extern struct currayhit rayHit,rayph,rayHitHyper;
extern GLint smooth_normals;

extern void xs_init(void);

extern int navi_tos;
extern void initializeTextureThread(void);
extern int isTextureinitialized(void);
extern int fileExists(char *fname, char *firstBytes, int GetIt);
extern int checkNetworkFile(char *fn);
extern void checkAndAllocMemTables(int *texture_num, int increment);
extern void   storeMPGFrameData(int latest_texture_number, int h_size, int v_size,
        int mt_repeatS, int mt_repeatT, char *Image);
void mpg_main(char *filename, int *x,int *y,int *depth,int *frameCount,void **ptr);
void makeAbsoluteFileName(char *filename, char *pspath,char *thisurl);
int getValidFileFromUrl (char *filename, char *path, struct Multi_String *inurl, char *firstBytes);
void removeFilenameFromPath (char *path);


void create_EAI(void);
int EAI_CreateVrml(const char *tp, const char *inputstring, uintptr_t *retarr, int retarrsize);
void EAI_Route(char cmnd, const char *tf);
void EAI_replaceWorld(const char *inputstring);

void render_hier(struct X3D_Node *p, int rwhat);
void handle_EAI(void);
void handle_aqua(const int mev, const unsigned int button, int x, int y);


extern int screenWidth, screenHeight;

/* Unix front end height/width */
#ifndef AQUA
extern int feWidth, feHeight;
#endif

/* SD AQUA FUNCTIONS */
#ifdef AQUA
extern int getOffset();
extern void initGL();
extern void setButDown(int button, int value);
extern void setCurXY(int x, int y);
extern void do_keyPress (char ch, int ev);
extern void setLastMouseEvent(int etype);
extern void initFreewrl();
extern void aqDisplayThread();
#endif
extern void setSnapSeq();
extern void setEAIport(int pnum);
extern void setTexSize(int num);
extern void setKeyString(const char *str);
extern void setNoCollision();
extern void setSnapGif();
extern void setLineWidth(float lwidth);
extern void closeFreewrl();
extern void setSeqFile(const char* file);
extern void setSnapFile(const char* file);
extern void setMaxImages(int max);
extern void setBrowserFullPath(const char *str);
extern void setSeqTemp(const char* file);
extern void setFullPath(const char *str);
extern void setInstance(uintptr_t instance);
extern void setScreenDim(int w, int h);

extern char *getLibVersion();
extern void doQuit(void);
extern void doBrowserAction ();


extern char *myPerlInstallDir;

/* for Extents and BoundingBoxen */
#define EXTENT_MAX_X _extent[0]
#define EXTENT_MIN_X _extent[1]
#define EXTENT_MAX_Y _extent[2]
#define EXTENT_MIN_Y _extent[3]
#define EXTENT_MAX_Z _extent[4]
#define EXTENT_MIN_Z _extent[5]
void setExtent (float maxx, float minx, float maxy, float miny, float maxz, float minz, struct X3D_Node *this_);
void recordDistance(struct X3D_Transform *nod);
void propagateExtent (struct X3D_Node *this_);

#ifdef DISPLAYBOUNDINGBOX
void BoundingBox(struct X3D_Node* node);
#define BOUNDINGBOX BoundingBox ((struct X3D_Node *)node);
#else
#define BOUNDINGBOX
#endif

void freewrlDie (const char *format);
char * readInputString(char *fn, char *parent);
char * sanitizeInputString(char *instr);

extern double nearPlane, farPlane, screenRatio;

/* children stuff moved out of VRMLRend.pm and VRMLC.pm for v1.08 */

extern int render_sensitive,render_vp,render_light,render_proximity,curlight,verbose,render_blend,render_geom,render_collision;

extern void XEventStereo();


/* Java CLASS invocation */
int newJavaClass(int scriptInvocationNumber,char * nodestr,char *node);
int initJavaClass(int scriptno);

int SAI_IntRetCommand (char cmnd, const char *fn);
char * SAI_StrRetCommand (char cmnd, const char *fn);
char *EAI_GetTypeName (unsigned int uretval);
char* EAI_GetValue(unsigned int nodenum, const char *fieldname, const char *nodename);
void setCLASStype (uintptr_t num);
void sendCLASSEvent(uintptr_t fn, int scriptno, char *fieldName, int type, int len);
void processClassEvents(int scriptno, int startEntry, int endEntry);
char *processThisClassEvent (void *fn, int startEntry, int endEntry, char *buf);
int ScanValtoBuffer(int *len, int type, char *buf, void *memptr, int buflen);
void getCLASSMultNumType (char *buf, int bufSize,
	struct Multi_Vec3f *tn,
	struct X3D_Node *parent,
	int eletype, int addChild);

void fwGetDoublev (int ty, double *mat);
void fwMatrixMode (int mode);
void fwXformPush(struct X3D_Transform *me);
void fwXformPop(struct X3D_Transform *me);
void fwLoadIdentity (void);
void invalidateCurMat(void);
void doBrowserAction (void);
void add_parent(struct X3D_Node *node_, struct X3D_Node *parent_,char *file, int line);
void remove_parent(struct X3D_Node *node_, struct X3D_Node *parent_);
void EAI_readNewWorld(char *inputstring);
void addToNode (void *rc,  int offs, struct X3D_Node *newNode);
void make_indexedfaceset(struct X3D_IndexedFaceSet *this_);

void render_LoadSensor(struct X3D_LoadSensor *this);

void render_Text (struct X3D_Text * this_);
#define rendray_Text render_ray_polyrep
void make_Text (struct X3D_Text * this_);
void collide_Text (struct X3D_Text * this_);
void render_TextureCoordinateGenerator(struct X3D_TextureCoordinateGenerator *this);
void render_TextureCoordinate(struct X3D_TextureCoordinate *this);

/* Component Grouping */
void prep_Transform (struct X3D_Transform *this_);
void fin_Transform (struct X3D_Transform *this_);
void child_Transform (struct X3D_Transform *this_);
void prep_Group (struct X3D_Group *this_);
void child_Group (struct X3D_Group *this_);
void child_StaticGroup (struct X3D_StaticGroup *this_);
void child_Switch (struct X3D_Switch *this_);

void changed_Group (struct X3D_Group *this_);
void changed_StaticGroup (struct X3D_StaticGroup *this_);
void changed_Transform (struct X3D_Transform *this_);

/* Environmental Sensor nodes */
void proximity_ProximitySensor (struct X3D_ProximitySensor *this_);
void child_VisibilitySensor (struct X3D_VisibilitySensor *this_);

/* Navigation Component */
void prep_Billboard (struct X3D_Billboard *this_);
void changed_Billboard (struct X3D_Billboard *this_);
void prep_Viewpoint(struct X3D_Viewpoint *node);
void child_Billboard (struct X3D_Billboard *this_);
void child_LOD (struct X3D_LOD *this_);
void fin_Billboard (struct X3D_Billboard *this_);
void child_Collision (struct X3D_Collision *this_);
void changed_Collision (struct X3D_Collision *this_);


/* HAnim Component */
void prep_HAnimJoint (struct X3D_HAnimJoint *this);
void prep_HAnimSite (struct X3D_HAnimSite *this);

void child_HAnimHumanoid(struct X3D_HAnimHumanoid *this_); 
void child_HAnimJoint(struct X3D_HAnimJoint *this_); 
void child_HAnimSegment(struct X3D_HAnimSegment *this_); 
void child_HAnimSite(struct X3D_HAnimSite *this_); 

void render_HAnimHumanoid (struct X3D_HAnimHumanoid *node);
void render_HAnimJoint (struct X3D_HAnimJoint * node);

void fin_HAnimSite (struct X3D_HAnimSite *this_);
void fin_HAnimJoint (struct X3D_HAnimJoint *this_);

void changed_HAnimSite (struct X3D_HAnimSite *this_);



/* Sound Component */
void render_Sound (struct X3D_Sound *this_);
void render_AudioControl (struct X3D_AudioControl *this_);
void render_AudioClip (struct X3D_AudioClip *this_);

/* Texturing Component */
void render_PixelTexture (struct X3D_PixelTexture *this_);
void render_ImageTexture (struct X3D_ImageTexture *this_);
void render_MultiTexture (struct X3D_MultiTexture *this_);
void render_MovieTexture (struct X3D_MovieTexture *this_);

/* Shape Component */
void render_Appearance (struct X3D_Appearance *this_);
void render_FillProperties (struct X3D_FillProperties *this_);
void render_LineProperties (struct X3D_LineProperties *this_);
void render_Material (struct X3D_Material *this_);
void render_Shape (struct X3D_Shape *this_);
void child_Shape (struct X3D_Shape *this_);
void child_Appearance (struct X3D_Appearance *this_);

/* Geometry3D nodes */
void render_Box (struct X3D_Box *this);
void compile_Box (struct X3D_Box *this);
void collide_Box (struct X3D_Box *this);
void render_Cone (struct X3D_Cone *this);
void compile_Cone (struct X3D_Cone *this);
void collide_Cone (struct X3D_Cone *this);
void render_Cylinder (struct X3D_Cylinder *this);
void compile_Cylinder (struct X3D_Cylinder *this);
void collide_Cylinder (struct X3D_Cylinder *this);
void render_ElevationGrid (struct X3D_ElevationGrid *this);
#define rendray_ElevationGrid  render_ray_polyrep
#define collide_ElevationGrid collide_IndexedFaceSet
void render_Extrusion (struct X3D_Extrusion *this);
void collide_Extrusion (struct X3D_Extrusion *this);
#define rendray_Extrusion render_ray_polyrep
void render_IndexedFaceSet (struct X3D_IndexedFaceSet *this);
void collide_IndexedFaceSet (struct X3D_IndexedFaceSet *this);
#define rendray_IndexedFaceSet render_ray_polyrep 
void render_IndexedFaceSet (struct X3D_IndexedFaceSet *this);
void render_Sphere (struct X3D_Sphere *this);
void compile_Sphere (struct X3D_Sphere *this);
void collide_Sphere (struct X3D_Sphere *this);
void make_Extrusion (struct X3D_Extrusion *this);
#define make_IndexedFaceSet make_indexedfaceset
#define make_ElevationGrid make_indexedfaceset
#define rendray_ElevationGrid render_ray_polyrep
void rendray_Box (struct X3D_Box *this_);
void rendray_Sphere (struct X3D_Sphere *this_);
void rendray_Cylinder (struct X3D_Cylinder *this_);
void rendray_Cone (struct X3D_Cone *this_);

/* Geometry2D nodes */
void render_Arc2D (struct X3D_Arc2D *this_);
void compile_Arc2D (struct X3D_Arc2D *this_);
void render_ArcClose2D (struct X3D_ArcClose2D *this_);
void compile_ArcClose2D (struct X3D_ArcClose2D *this_);
void render_Circle2D (struct X3D_Circle2D *this_);
void compile_Circle2D (struct X3D_Circle2D *this_);
void render_Disk2D (struct X3D_Disk2D *this_);
void compile_Disk2D (struct X3D_Disk2D *this_);
void render_Polyline2D (struct X3D_Polyline2D *this_);
void render_Polypoint2D (struct X3D_Polypoint2D *this_);
void render_Rectangle2D (struct X3D_Rectangle2D *this_);
void compile_Rectangle2D (struct X3D_Rectangle2D *this_);
void render_TriangleSet2D (struct X3D_TriangleSet2D *this_);
void compile_TriangleSet2D (struct X3D_TriangleSet2D *this_);
void collide_Disk2D (struct X3D_Disk2D *this_);
void collide_Rectangle2D (struct X3D_Rectangle2D *this_);
void collide_TriangleSet2D (struct X3D_TriangleSet2D *this_);

/* Component Rendering nodes */
#define rendray_IndexedTriangleSet render_ray_polyrep
#define rendray_IndexedTriangleFanSet render_ray_polyrep
#define rendray_IndexedTriangleStripSet render_ray_polyrep
#define rendray_TriangleSet render_ray_polyrep
#define rendray_TriangleFanSet render_ray_polyrep
#define rendray_TriangleStripSet render_ray_polyrep
void render_IndexedTriangleFanSet (struct X3D_IndexedTriangleFanSet *this_); 
void render_IndexedTriangleSet (struct X3D_IndexedTriangleSet *this_); 
void render_IndexedTriangleStripSet (struct X3D_IndexedTriangleStripSet *this_); 
void render_TriangleFanSet (struct X3D_TriangleFanSet *this_); 
void render_TriangleStripSet (struct X3D_TriangleStripSet *this_); 
void render_TriangleSet (struct X3D_TriangleSet *this_); 
void render_LineSet (struct X3D_LineSet *this_); 
void render_IndexedLineSet (struct X3D_IndexedLineSet *this_); 
void render_PointSet (struct X3D_PointSet *this_); 
#define collide_IndexedTriangleFanSet  collide_IndexedFaceSet
#define collide_IndexedTriangleSet  collide_IndexedFaceSet
#define collide_IndexedTriangleStripSet  collide_IndexedFaceSet
#define collide_TriangleFanSet  collide_IndexedFaceSet
#define collide_TriangleSet  collide_IndexedFaceSet
#define collide_TriangleStripSet  collide_IndexedFaceSet
#define make_IndexedTriangleFanSet  make_indexedfaceset
#define make_IndexedTriangleSet  make_indexedfaceset
#define make_IndexedTriangleStripSet  make_indexedfaceset
#define make_TriangleFanSet  make_indexedfaceset
#define make_TriangleSet  make_indexedfaceset
#define make_TriangleStripSet  make_indexedfaceset
void compile_LineSet (struct X3D_LineSet *this_); 
void compile_IndexedLineSet (struct X3D_IndexedLineSet *this_); 

/* Component Lighting Nodes */
void render_DirectionalLight (struct X3D_DirectionalLight *this_);
void prep_SpotLight (struct X3D_SpotLight *this_);
void prep_PointLight (struct X3D_PointLight *this_);

/* Geospatial nodes */
void render_GeoElevationGrid (struct X3D_GeoElevationGrid *this_);
#define rendray_GeoElevationGrid render_ray_polyrep
void collide_GeoElevationGrid (struct X3D_GeoElevationGrid *this_);
void make_GeoElevationGrid (struct X3D_GeoElevationGrid *this_);
void prep_GeoViewpoint(struct X3D_GeoViewpoint *node);
void fin_GeoLocation (struct X3D_GeoLocation *this_);
void changed_GeoLocation (struct X3D_GeoLocation *this_);
void child_GeoLOD (struct X3D_GeoLOD *this_);
void child_GeoLocation (struct X3D_GeoLocation *this_);

/* Networking Component */
void child_Anchor (struct X3D_Anchor *this_);
void child_Inline (struct X3D_Inline *this_);
void changed_Inline (struct X3D_Inline *this_);
void child_InlineLoadControl (struct X3D_InlineLoadControl *this_);
void changed_InlineLoadControl (struct X3D_InlineLoadControl *this_);
void changed_Anchor (struct X3D_Anchor *this_);

/* KeyDevice Component */
void killKeySensorNodeList(void);
void addNodeToKeySensorList(struct X3D_Node* node);
int KeySensorNodePresent(void);
void sendKeyToKeySensor(const char key, int upDown);

void prep_MidiControl (struct X3D_MidiControl *node);
void do_MidiControl (void *node);
void MIDIRegisterMIDI(char *str);
/* ReWire device/controller  table */
struct ReWireDeviceStruct {
        struct X3D_MidiControl* node;   /* pointer to the node that controls this */
        int encodedDeviceName;          /* index into ReWireNamenames */
        int bus;                        /* which MIDI bus this is */
        int channel;                    /* which MIDI channel on this bus it is */
        int encodedControllerName;      /* index into ReWireNamenames */
        int controller;                 /* controller number */
        int cmin;                       /* minimum value for this controller */
        int cmax;                       /* maximum value for this controller */
        int ctype;                      /* controller type TYPE OF FADER control - not used currently */
};

/* ReWireName table */
struct ReWireNamenameStruct {
        char *name;
};

extern struct ReWireNamenameStruct *ReWireNamenames;
extern int ReWireNametableSize;
extern int MAXReWireNameNames;
extern struct ReWireDeviceStruct *ReWireDevices;
extern int ReWireDevicetableSize;
extern int MAXReWireDevices;


/* Event Utilities Component */
void do_BooleanFilter (void *node);
void do_BooleanSequencer (void *node);
void do_BooleanToggle (void *node);
void do_BooleanTrigger (void *node);
void do_IntegerSequencer (void *node);
void do_IntegerTrigger (void *node);
void do_TimeTrigger (void *node);


#define ADD_PARENT(a,b) add_parent(a,b,__FILE__,__LINE__)
#define NODE_ADD_PARENT(a) ADD_PARENT(a,X3D_NODE(ptr))
#define NODE_REMOVE_PARENT(a) ADD_PARENT(a,X3D_NODE(ptr))


/* OpenGL state cache */
/* solid shapes, GL_CULL_FACE can be enabled if a shape is Solid */ 
#define CULL_FACE(v) /* printf ("nodeSolid %d cullFace %d GL_FALSE %d FALSE %d\n",v,cullFace,GL_FALSE,FALSE); */ \
		if (v != cullFace) {	\
			cullFace = v; \
			if (cullFace == 1) glEnable(GL_CULL_FACE);\
			else glDisable(GL_CULL_FACE);\
		}
#define DISABLE_CULL_FACE CULL_FACE(0)
#define ENABLE_CULL_FACE CULL_FACE(1)
#define CULL_FACE_INITIALIZE cullFace=0; glDisable(GL_CULL_FACE);

#define LIGHTING_ON if (!lightingOn) {lightingOn=TRUE;glEnable(GL_LIGHTING);}
#define LIGHTING_OFF if(lightingOn) {lightingOn=FALSE;glDisable(GL_LIGHTING);}
#define LIGHTING_INITIALIZE lightingOn=TRUE; glEnable(GL_LIGHTING);

#define COLOR_MATERIAL_ON if (colorMaterialEnabled == GL_FALSE) {colorMaterialEnabled=GL_TRUE;glEnable(GL_COLOR_MATERIAL);}
#define COLOR_MATERIAL_OFF if (colorMaterialEnabled == GL_TRUE) {colorMaterialEnabled=GL_FALSE;glDisable(GL_COLOR_MATERIAL);}
#define COLOR_MATERIAL_INITIALIZE colorMaterialEnabled = GL_FALSE; glDisable(GL_COLOR_MATERIAL);

void zeroAllBindables(void);
void Next_ViewPoint(void);
void Prev_ViewPoint(void);
void First_ViewPoint(void);
void Last_ViewPoint(void);

int freewrlSystem (const char *string);

int inputParse(unsigned type, char *inp, int bind, int returnifbusy,
                        void *ptr, unsigned ofs, int *complete,
                        int zeroBind);
void compileNode (void (*nodefn)(void *, void *, void *, void *, void *), void *node, void *a, void *b, void *c, void *d);
void destroyCParserData();

void getMovieTextureOpenGLFrames(int *highest, int *lowest,int myIndex);

int ConsoleMessage(const char *fmt, ...);
void closeConsoleMessage(void);
extern int consMsgCount;

void outOfMemory(const char *message);

void killErrantChildren(void);

void kill_routing(void);
void kill_bindables(void);
void kill_javascript(void);
void kill_oldWorld(int a, int b, int c);
void kill_clockEvents(void);
void kill_openGLTextures(void);
void kill_X3DDefs(void);
extern int currentFileVersion;

int findFieldInFIELDNAMES(const char *field);
int findFieldInFIELD(const char* field);
int findFieldInFIELDTYPES(const char *field);
int findFieldInX3DACCESSORS(const char *field);
int findFieldInEXPOSED_FIELD(const char* field);
int findFieldInEVENT_IN(const char* field);
int findFieldInEVENT_OUT(const char* field);
int findFieldInX3DSPECIALKEYWORDS(const char *field);

/* Values for fromTo */
#define ROUTED_FIELD_EVENT_OUT 0
#define ROUTED_FIELD_EVENT_IN  1
int findRoutedFieldInFIELDNAMES(struct X3D_Node *node, const char *field, int fromTo);
int findRoutedFieldInEXPOSED_FIELD(struct X3D_Node*, const char*, int);
int findRoutedFieldInEVENT_IN(struct X3D_Node*, const char*, int);
int findRoutedFieldInEVENT_OUT(struct X3D_Node*, const char*, int);
int findFieldInNODES(const char *node);
int findFieldInCOMPONENTS(const char *node);
int findFieldInPROFILESS(const char *node);
int findFieldInALLFIELDNAMES(const char *field);
void findFieldInOFFSETS(const int *nodeOffsetPtr, const int field, int *coffset, int *ctype, int *ckind);
char *findFIELDNAMESfromNodeOffset(struct X3D_Node *node, int offset);
int findFieldInKEYWORDS(const char *field);
int findFieldInPROTOKEYWORDS(const char *field);
int countCommas (char *instr);
void sortChildren (struct Multi_Node ch);
void dirlightChildren(struct Multi_Node ch);
void normalChildren(struct Multi_Node ch);
void checkParentLink (struct X3D_Node *node,struct X3D_Node *parent);

/* background colour */
void setglClearColor (float *val); 
void doglClearColor(void);
extern int cc_changed;

char *findPathToFreeWRLFile(char *lfn);

int mapFieldTypeToInernaltype (indexT kwIndex);
void finishEventLoop();
void resetEventLoop();

/* MIDI stuff... */
void ReWireRegisterMIDI (char *str);
void ReWireMIDIControl(char *str);


/* META data, component, profile  stuff */
void handleMetaDataStringString(struct Uni_String *val1,struct Uni_String *val2);
void handleProfile(int myp);
void handleComponent(int com, int lev);
void handleExport (char *node, char *as);
void handleImport (char *nodeName,char *nodeImport, char *as);


/* free memory */
void registerX3DNode(struct X3D_Node * node);

void doNotRegisterThisNodeForDestroy(struct X3D_Node * nodePtr);

#ifdef AQUA
Boolean notFinished();
void disposeContext();
void setPaneClipRect(int npx, int npy, WindowPtr fwWindow, int ct, int cb, int cr, int cl, int width, int height);
void createContext(CGrafPtr grafPtr);
void setIsPlugin();
void sendPluginFD(int fd);
void aquaPrintVersion();
void setPluginPath(char* path);
#endif
void setEaiVerbose();
void replaceWorldNeeded(char* str);

/* X3D C parser */
int X3DParse (struct X3D_Group *parent, char *inputstring);
void *createNewX3DNode (int nt);

/* this is set by OSX, or to FALSE if on Linux. */
#ifdef AQUA
extern Boolean isBrowserPlugin;
#else
extern int isBrowserPlugin;
#endif


#endif /* __HEADERS_H__ */
