#ifndef __HEADERS_H__
#define __HEADERS_H__
 	 
#include "Structs.h"

#include "LinearAlgebra.h"
#include "constants.h"

#ifndef AQUA
#include <GL/glu.h>
#else
#include <glu.h>
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

#define BOOL_STRING(b) (b ? "TRUE" : "FALSE")


#ifdef M_PI
#define PI M_PI
#else
#define PI 3.141592653589793
#endif

/* how many steps for dividing a shape into - eg spheres, cones... */
#define horiz_div 20
#define vert_div 20

/* Faster trig macros (thanks for Robin Williams) */
/* fixed code, thanks to Etienne Grossmann */

/*
   (t_aa,  t_ab)  constants for the rotation params 
   (t_sa,  t_ca)  this 2D point will be rotated by UP_TRIG1
   (t_sa1, t_ca1) temp vars
*/
#define DECL_TRIG1 float t_aa, t_ab, t_sa, t_ca, t_sa1, t_ca1;
#define INIT_TRIG1(div) t_aa = sin(PI/(div)); t_aa *= 2*t_aa; t_ab = -sin(2*PI/(div));
#define START_TRIG1 t_sa = 0; t_ca = -1;
#define UP_TRIG1 t_sa1 = t_sa; t_sa -= t_sa*t_aa - t_ca * t_ab; t_ca -= t_ca * t_aa + t_sa1 * t_ab;
#define SIN1 t_sa
#define COS1 t_ca


#define DECL_TRIG2 float t2_aa, t2_ab, t2_sa, t2_ca, t2_sa1, t2_ca1;
#define INIT_TRIG2(div) t2_aa = sin(PI/(div)); t2_aa *= 2*t2_aa; t2_ab = -sin(2*PI/(div));
/* Define starting point of horizontal rotations */
#define START_TRIG2 t2_sa = -1; t2_ca = 0;
/* #define START_TRIG2 t2_sa = 0; t2_ca = -1; */
#define UP_TRIG2 t2_sa1 = t2_sa; t2_sa -= t2_sa*t2_aa - t2_ca * t2_ab; t2_ca -= t2_ca * t2_aa + t2_sa1 * t2_ab;
#define SIN2 t2_sa
#define COS2 t2_ca



/* defines for raycasting: */
#define APPROX(a,b) (fabs(a-b)<0.00000001)
#define NORMAL_VECTOR_LENGTH_TOLERANCE 0.00001
/* (test if the vector part of a rotation is normalized) */
#define IS_ROTATION_VEC_NOT_NORMAL(rot)        ( \
       fabs(1-sqrt(rot.r[0]*rot.r[0]+rot.r[1]*rot.r[1]+rot.r[2]*rot.r[2])) \
               >NORMAL_VECTOR_LENGTH_TOLERANCE \
)

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


void normalize_ifs_face (float *point_normal,
                         struct pt *facenormals,
                         int *pointfaces,
                        int mypoint,
                        int curpoly,
                        float creaseAngle);


void FW_rendertext(int numrows,SV **ptr,char *directstring, int nl, float *length,
                float maxext, float spacing, float mysize, unsigned int fsparam,
                struct VRML_PolyRep *rp);


/* Triangulator extern defs - look in CFuncs/Tess.c */
extern struct VRML_PolyRep *global_tess_polyrep;
extern GLUtriangulatorObj *global_tessobj;
extern int global_IFS_Coords[];
extern int global_IFS_Coord_count;

/* do we have to do textures?? */
#define HAVETODOTEXTURES (last_bound_texture != 0)

/* appearance does material depending on last texture depth */
extern int last_texture_depth;


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

/* Some drivers give a GL error if doing a glIsEnabled when creating
   display lists, so we just use an already created variable. */
extern GLuint last_bound_texture;

/* current time */
extern double TickTime;


/* Transform node optimizations */
int verify_rotate(GLfloat *params);
int verify_translate(GLfloat *params);

/* C routes */
#define MAXJSVARIABLELENGTH 25	/* variable name length can be this long... */

void mark_event (unsigned int from, unsigned int fromoffset);

/* saved rayhit and hyperhit */
extern struct SFColor ray_save_posn, hyp_save_posn, hyp_save_norm;


/* bindable nodes */
extern GLint viewport[];
extern GLdouble fieldofview;
extern int found_vp;
extern struct pt ViewerUpvector;
extern struct sNaviInfo naviinfo;


/* Sending events back to Browser (eg, Anchor) */
extern int BrowserAction;
extern char * BrowserActionString;

/* Scripting Routing interfaces */
void CRoutes_js_new (int num,unsigned int cx, unsigned int glob, unsigned int brow);
void gatherScriptEventOuts(int script, int ignore);
void getMFNodetype (char *strp, struct Multi_Node *ch, int ar);

void update_node(void *ptr);

extern int CRVerbose, JSVerbose;

int JSparamIndex (char *name, char *type);

struct CRjsStruct {
	unsigned int	cx;	/* JSContext		*/
	unsigned int	glob;	/* JSGlobals		*/
	unsigned int	brow;	/* BrowserIntern	*/
};
void JSMaxAlloc();

extern struct CRjsStruct *JSglobs; /* Javascript invocation parameters */
extern int *scr_act;    /* script active array - defined in CRoutes.c */
extern int JSMaxScript;  /* defined in JSscipts.c; maximum size of script arrays */
void render_status(); /* status bar */
void update_status(); 	/* update status bar */
void viewer_type_status(int x);		/* tell status bar what kind of viewer */
//void viewpoint_name_status(char *str); /* tell status bar name of current vp 	*/
extern double BrowserFPS;
void render_polyrep(void *node,
        int npoints, struct SFColor *points,
        int ncolors, struct SFColor *colors,
        int nnormals, struct SFColor *normals,
        int ntexcoords, struct SFVec2f *texcoords);




#endif /* __HEADERS_H__ */
