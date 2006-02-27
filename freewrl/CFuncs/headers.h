/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#ifndef __HEADERS_H__
#define __HEADERS_H__

#undef GLERRORS

/* free a malloc'd pointer */
#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}

/* rendering constants used in SceneGraph, etc. */
#define VF_Viewpoint 	0x0001
#define VF_Geom 	0x0002
#define VF_Lights	0x0004 
#define VF_Sensitive 	0x0008
#define VF_Blend 	0x0010
#define VF_Proximity 	0x0020
#define VF_Collision 	0x0040

#define VF_hasVisibleChildren 			0x0100
#define VF_removeHasVisibleChildren		0xFEFF
#define VF_hasGeometryChildren 			0x0200
#define VF_hasBeenScannedForGeometryChildren	0x0400

void OcclusionCulling (void);
void OcclusionStartofEventLoop(void);


#ifdef GL_VERSION_1_5
#ifdef GL_ARB_occlusion_query
#define OCCLUSION
#define VISIBILITYOCCLUSION
#undef TRANSFORMOCCLUSION
#undef SHAPEOCCLUSION
#undef STATICGROUPOCCLUSION
#endif
#endif

#ifdef OCCLUSION

#define glGenQueries(a,b) glGenQueriesARB(a,b)
#define glDeleteQueries(a,b) glDeleteQueriesARB(a,b)
#define glIsQuery(a) glIsQueryARB(a)
#define glBeginQuery(a,b) glBeginQueryARB(a,b)
#define glEndQuery(a) glEndQueryARB(a)
#define glGetQueryiv(a,b,c) glGetQueryivARB(a,b,c)
#define glGetQueryObjectiv(a,b,c) glGetQueryObjectivARB(a,b,c)
#define glGetQueryObjectuiv(a,b,c) glGetQueryObjectuivARB(a,b,c)

extern GLuint *OccQueries;
extern void * *OccNodes;
extern int *OccActive;
extern GLint *OccSamples;

extern int maxShapeFound;
extern int OccQuerySize;

#define BEGINOCCLUSIONQUERY \
				if (render_geom) { \
                                /* printf ("OcclusionQuery for %d type %s\n",node->__OccludeNumber,stringNodeType( \
                                                ((struct X3D_Box*) node->geometry)->_nodeType)); */ \
                                if (node->__OccludeNumber > maxShapeFound) maxShapeFound = node->__OccludeNumber; \
                                if ((node->__OccludeNumber >=0) && (node->__OccludeNumber < OccQuerySize)) { \
					OccActive[node->__OccludeNumber] = TRUE; \
					if (OccNodes[node->__OccludeNumber] == 0) { \
						OccNodes[node->__OccludeNumber] = node; \
					} \
/* printf ("beginning quert for %d %s\n",node->__OccludeNumber,stringNodeType(node->_nodeType)); */ \
                                        glBeginQuery(GL_SAMPLES_PASSED,OccQueries[node->__OccludeNumber]); \
                                } }
#define ENDOCCLUSIONQUERY \
			if (render_geom) { \
                        if ((node->__OccludeNumber >=0) && (node->__OccludeNumber < OccQuerySize)) { \
				/* printf ("ending query for %d (%s)\n",node->__OccludeNumber,stringNodeType(node->_nodeType)); */ \
                                glEndQuery(GL_SAMPLES_PASSED);   \
                        } }
#endif

/* bounding box calculations */
#define EXTENTTOBBOX 	node->bboxSize.c[0] = node->_extent[0]; \
			node->bboxSize.c[1] = node->_extent[1]; \
			node->bboxSize.c[2] = node->_extent[2];
/********************************
	Verbosity
*********************************/

/* Java Class invocation */
#undef JSVRMLCLASSVERBOSE

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

#include "Structs.h"

/* from the vrml.conf file... */
#include "vrmlconf.h"

#include "LinearAlgebra.h"

#ifndef AQUA
#include <GL/glu.h>
#else
#include <glu.h>
#include <CGLTypes.h>
#include "aquaInt.h"
extern CGLContextObj aqglobalContext;
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

/* convert a nodeType to a string */
char *stringNodeType (int type);

/* POLYREP stuff */
#define POINT_FACES	16 /* give me a point, and it is in up to xx faces */

/* Function Prototypes */

void render_node(void *node);

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
#define DIRECTIONAL_LIGHT_OFF if (node->has_light) lightState(savedlight+1,FALSE); curlight = savedlight;
#define DIRECTIONAL_LIGHT_SAVE int savedlight = curlight;

void normalize_ifs_face (float *point_normal,
                         struct pt *facenormals,
                         int *pointfaces,
                        int mypoint,
                        int curpoly,
                        float creaseAngle);


void FW_rendertext(unsigned int numrows,SV **ptr,char *directstring, unsigned int nl, double *length,
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
extern int texture_count; 
extern int     *global_tcin;
extern int     global_tcin_count; 
extern void textureDraw_start(struct X3D_IndexedFaceSet *texC, GLfloat *tex);
extern void textureDraw_end(void);

extern void * this_textureTransform;  /* do we have some kind of textureTransform? */

extern int isTextureLoaded(int texno);


extern int _fw_pipe, _fw_FD;
#define RUNNINGASPLUGIN (_fw_pipe != 0)

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


/* Transform node optimizations */
int verify_rotate(GLfloat *params);
int verify_translate(GLfloat *params);
int verify_scale(GLfloat *params);

/* C routes */
#define MAXJSVARIABLELENGTH 25	/* variable name length can be this long... */

void mark_event (void *from, unsigned int fromoffset);

/* saved rayhit and hyperhit */
extern struct SFColor ray_save_posn, hyp_save_posn, hyp_save_norm;

/* set a node to be sensitive */
void setSensitive(void *ptr,void *datanode,char *type);

/* bindable nodes */
extern GLint viewport[];
extern GLdouble fieldofview;
extern int found_vp;
extern struct pt ViewerUpvector;
extern struct sNaviInfo naviinfo;


/* Sending events back to Browser (eg, Anchor) */
extern int BrowserAction;
extern struct X3D_Anchor *AnchorsAnchor;

/* Scripting Routing interfaces */

#define SFUNKNOWN 0
#define SFBOOL 	1
#define SFCOLOR 2
#define SFFLOAT 3
#define SFTIME 	4
#define SFINT32 5
#define SFSTRING 6
#define SFNODE	7
#define SFROTATION 8
#define SFVEC2F	9
#define SFIMAGE	10

#define MFCOLOR 11
#define MFFLOAT 12
#define MFTIME 	13
#define MFINT32 14
#define MFSTRING 15
#define MFNODE	16
#define MFROTATION 17
#define MFVEC2F	18
#define MFVEC3F 19
#define SFVEC3F 20


#define FIELD_TYPE_STRING(f) ( \
	f == SFBOOL ? "SFBool" : ( \
	f == SFCOLOR ? "SFColor" : ( \
	f == SFVEC3F ? "SFVec3f" : ( \
	f == SFFLOAT ? "SFFloat" : ( \
	f == SFTIME ? "SFTime" : ( \
	f == SFINT32 ? "SFInt32" : ( \
	f == SFSTRING ? "SFString" : ( \
	f == SFNODE ? "SFNode" : ( \
	f == SFROTATION ? "SFRotation" : ( \
	f == SFVEC2F ? "SFVec2f" : ( \
	f == SFIMAGE ? "SFImage" : ( \
	f == MFCOLOR ? "MFColor" : ( \
	f == MFVEC3F ? "MFVec3f" : ( \
	f == MFFLOAT ? "MFFloat" : ( \
	f == MFTIME ? "MFTime" : ( \
	f == MFINT32 ? "MFInt32" : ( \
	f == MFSTRING ? "MFString" : ( \
	f == MFNODE ? "MFNode" : ( \
	f == MFROTATION ? "MFRotation" : ( \
	f == MFVEC2F ? "MFVec2f" : ( \
	f == MFVEC3F ? "MFVec3f" : ( \
	f == MFROTATION ? "MFRotation" : ( \
	f == SFVEC2F ? "SFVec2f" : "unknown field type")))))))))))))))))))))))


void CRoutes_js_new (int num,int scriptType);
extern int max_script_found;
void getMFNodetype (char *strp, struct Multi_Node *ch, struct X3D_Box *par, int ar);

void update_node(void *ptr);
void update_renderFlag(void *ptr, int flag);

int JSparamIndex (char *name, char *type);

/* setting script eventIns from routing table or EAI */
void Set_one_MultiElementtype (int tn, int tptr, void *fn, unsigned len);
void set_one_ECMAtype (unsigned long int tonode, int toname, int dataType, void *Data, unsigned datalen);
void mark_script (unsigned long int num);


/* structure for rayhits */
struct currayhit {
	void *node; /* What node hit at that distance? */
	GLdouble modelMatrix[16]; /* What the matrices were at that node */
	GLdouble projMatrix[16];
};



struct CRscriptStruct {
	/* type */
	int thisScriptType;

	/* Javascript parameters */
	unsigned long int	cx;	/* JSContext		*/
	unsigned long int	glob;	/* JSGlobals		*/
	unsigned long int	brow;	/* BrowserIntern	*/

	/* Java .CLASS parameters */
	unsigned int 	_initialized; 	/* has initialize been sent? */
	int listen_fd, send_fd;		/* socket descriptors */
	char NodeID[20];		/* combo Perl NODEXXX, and CNODE */

};
void JSMaxAlloc(void);
void cleanupDie(int num, const char *msg);
void shutdown_EAI(void);
unsigned int EAI_GetNode(const char *str);
unsigned int EAI_GetViewpoint(const char *str);
void EAI_killBindables (void);
void EAI_GetType(unsigned int nodenum, const char *fieldname, const char *direction,
        int *nodeptr,
        int *dataoffset,
        int *datalen,
        int *nodetype,
        int *scripttype);

void setECMAtype(int num);
int get_touched_flag(int fptr, unsigned long int actualscript);
void getMultiElementtype(char *strp, struct Multi_Vec3f *tn, int eletype);
void setMultiElementtype(int num);
void Multimemcpy(void *tn, void *fn, int len);
void CRoutes_Register(int adrem,        void *from,
                                 int fromoffset,
                                 unsigned int to_count,
                                 char *tonode_str,
                                 int length,
                                 void *intptr,
                                 int scrdir,
                                 int extra);
void CRoutes_free(void);
void propagate_events(void);
void sendScriptEventIn(int num);
void add_first(char *clocktype,void * node);
void do_first(void);
void process_eventsProcessed(void);

void getEAI_MFStringtype (struct Multi_String *from, struct Multi_String *to);


extern struct CRscriptStruct *ScriptControl; /* Script invocation parameters */
extern unsigned long int *scr_act;    /* script active array - defined in CRoutes.c */
extern int *thisScriptType;    /* what kind of script this is - in CRoutes.c */
extern int JSMaxScript;  /* defined in JSscipts.c; maximum size of script arrays */
void render_status(void); /* status bar */
void update_status(void); 	/* update status bar */
void viewer_type_status(int x);		/* tell status bar what kind of viewer */
void viewpoint_name_status(char *str); /* tell status bar name of current vp 	*/
int convert_typetoInt (const char *type);	/* convert a string, eg "SFBOOL" to type, eg SFBOOL */

extern double BrowserFPS;
void render_polyrep(void *node);

extern int CRoutesExtra;		/* let EAI see param of routing table - Listener data. */

unsigned EAI_do_ExtraMemory (int size,SV *data,char *type);

/* types of scripts. */
#define NOSCRIPT 	0
#define JAVASCRIPT	1
#define	CLASSSCRIPT	2
#define	PERLSCRIPT	3
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
#define CALLMETHOD	4   /* Javascript... 	*/
#define CALLMETHODVA    5   /* Javascript... 	*/
#define EAIGETNODE      6   /* EAI getNode      	*/
#define EAIGETTYPE	7   /* EAI getType	*/
#define ZEROBINDABLES   8   /* get rid of Perl datastructures */
#define EAIROUTE	9   /* EAI add/del route */
#define EAIGETVALUE	10  /* get a value of a node */
#define EAIGETTYPENAME	11  /* get the type name for a  node */
#define EAIGETVIEWPOINT	12  /* get a Viewpoint BackEnd CNode */




extern void *rootNode;
extern int isPerlParsing(void);
extern int isURLLoaded(void);	/* initial scene loaded? Robert Sim */
extern int isTextureParsing(void);
extern void loadInline(struct X3D_Inline *node);
extern void loadImageTexture(struct X3D_ImageTexture *node,  void *param);
extern void loadPixelTexture(struct X3D_PixelTexture *node,  void *param);
extern void loadMovieTexture(struct X3D_MovieTexture *node,  void *param);
extern void loadMultiTexture(struct X3D_MultiTexture *node);
extern void loadBackgroundTextures (struct X3D_Background *node);
extern void loadTextureBackgroundTextures (struct X3D_TextureBackground *node);
extern GLfloat boxtex[], boxnorms[], BackgroundVert[];
extern GLfloat Backtex[], Backnorms[];

extern void new_tessellation(void);
extern void initializePerlThread(const char *perlpath);
extern PerlInterpreter *my_perl;
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
extern char *BrowserName, *BrowserVersion, *BrowserURL, *BrowserFullPath; /* defined in VRMLC.pm */
extern char *lastReadFile; 		/* name last file read in */
extern int display_status;		/* toggle status bar - defined in VRMLC.pm */
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
extern struct currayhit rh,rph,rhhyper;
extern int smooth_normals;

extern void xs_init(void);

extern int navi_tos;
extern void initializeTextureThread(void);
extern int isTextureinitialized(void);
extern int fileExists(char *fname, char *firstBytes, int GetIt);
extern int checkNetworkFile(char *fn);
extern void checkAndAllocMemTables(int *texture_num, int increment);
extern void   storeMPGFrameData(int latest_texture_number, int h_size, int v_size,
        int mt_repeatS, int mt_repeatT, char *Image);
void mpg_main(const char *filename, int *x,int *y,int *depth,int *frameCount,void **ptr);
void makeAbsoluteFileName(char *filename, char *pspath,char *thisurl);


void create_EAI(void);
int EAI_CreateVrml(const char *tp, const char *inputstring, unsigned *retarr, int retarrsize);
void EAI_Route(char cmnd, const char *tf);
void EAI_replaceWorld(const char *inputstring);

void render_hier(void *p, int rwhat);
void handle_EAI(void);

extern int screenWidth, screenHeight;

/* SD AQUA FUNCTIONS */
#ifdef AQUA
extern int getOffset();
extern void initGL();
extern void setButDown(int button, int value);
extern void setCurXY(int x, int y);
extern void setLastMouseEvent(int etype);
extern void initFreewrl();
extern void aqDisplayThread();
#endif
extern void setSnapSeq();
extern void setEAIport(int pnum);
extern void setNoStatus();
extern void setFast();
extern void setKeyString(const char *str);
extern void setNoCollision();
extern void setSnapGif();
extern void setLineWidth(float lwidth);
extern void closeFreewrl();
extern void setSeqFile(const char* file);
extern void setSnapFile(const char* file);
extern void setMaxImages(int max);
extern void setBrowserURL(const char *str);
extern void setSeqTemp(const char* file);
extern void setFullPath(const char *str);
extern void setScreenDim(int w, int h);

extern char *getLibVersion();
extern void doQuit(void);
extern void doBrowserAction ();


extern char *myPerlInstallDir;

void freewrlDie (const char *format);
char * readInputString(char *fn, char *parent);
char * sanitizeInputString(char *instr);
void BoundingBox(struct SFColor bbc,struct SFColor bbs);
void setExtent (float x, float y, float z, struct X3D_Box *this_);
void recordDistance(struct X3D_Transform *nod);
void propagateExtent (float x, float y, float z, struct X3D_Box *this_);

extern double nearPlane, farPlane, screenRatio;

/* children stuff moved out of VRMLRend.pm and VRMLC.pm for v1.08 */

extern int render_sensitive,render_vp,render_light,render_proximity,curlight,verbose,render_blend,render_geom,render_collision;

extern void XEventStereo();


/* Java CLASS invocation */
int newJavaClass(int scriptInvocationNumber,char * nodestr,char *node);
int initJavaClass(int scriptno);

char *EAI_GetTypeName (unsigned int uretval);
char* EAI_GetValue(unsigned int nodenum, const char *fieldname, const char *nodename);
void setCLASStype (int num);
void sendCLASSEvent(int fn, int scriptno, char *fieldName, int type, int len);
void processClassEvents(int scriptno, int startEntry, int endEntry);
char *processThisClassEvent (void *fn, int startEntry, int endEntry, char *buf);
int ScanValtoBuffer(int *len, int type, char *buf, void *memptr, int buflen);
void getCLASSMultNumType (char *buf, int bufSize,
	struct Multi_Vec3f *tn,
	struct X3D_Box *parent,
	int eletype, int addChild);

void fwGetDoublev (int ty, double *mat);
void fwMatrixMode (int mode);
void fwXformPush(struct X3D_Transform *me);
void fwXformPop(struct X3D_Transform *me);
void fwLoadIdentity (void);
void invalidateCurMat(void);
void doBrowserAction (void);
void add_parent(void *node_, void *parent_);
void remove_parent(void *node_, void *parent_);
void EAI_readNewWorld(char *inputstring);
void addToNode (void *rc,  void *newNode);
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
void collide_Box (struct X3D_Box *this);
void render_Cone (struct X3D_Cone *this);
void collide_Cone (struct X3D_Cone *this);
void render_Cylinder (struct X3D_Cylinder *this);
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
void render_ArcClose2D (struct X3D_ArcClose2D *this_);
void render_Circle2D (struct X3D_Circle2D *this_);
void render_Disk2D (struct X3D_Disk2D *this_);
void render_Polyline2D (struct X3D_Polyline2D *this_);
void render_Polypoint2D (struct X3D_Polypoint2D *this_);
void render_Rectangle2D (struct X3D_Rectangle2D *this_);
void render_TriangleSet2D (struct X3D_TriangleSet2D *this_);
void collide_Disk2D (struct X3D_Disk2D *this_);
void collide_Rectangle2D (struct X3D_Rectangle2D *this_);
void collide_TriangleSet2D (struct X3D_TriangleSet2D *this_);

/* Rendering nodes */
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

/* Component Lighting Nodes */
void render_DirectionalLight (struct X3D_DirectionalLight *this_);
void light_SpotLight (struct X3D_SpotLight *this_);
void light_PointLight (struct X3D_PointLight *this_);

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


#define NODE_ADD_PARENT(a) add_parent(a,ptr)
#define NODE_REMOVE_PARENT(a) add_parent(a,ptr)


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
int freewrlSystem (const char *string);

int perlParse(unsigned type, char *inp, int bind, int returnifbusy,
                        void *ptr, unsigned ofs, int *complete,
                        int zeroBind);


int ConsoleMessage(const char *fmt, ...);
extern int consMsgCount;

void outOfMemory(const char *message);
void initializeScript(int num,int evIn);

void killErrantChildren(void);

void kill_routing(void);
void kill_bindables(void);
void kill_javascript(void);
void kill_oldWorld(int a, int b, int c);


#endif /* __HEADERS_H__ */
