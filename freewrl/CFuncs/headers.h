
/* number of tesselated coordinates allowed */
#define TESS_MAX_COORDS  500

#define offset_of(p_type,field) ((unsigned int)(&(((p_type)NULL)->field)-NULL))

#ifdef M_PI
#define PI M_PI
#else
#define PI 3.141592653589793
#endif

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
void render_polyrep(void *node, 
	int npoints, struct SFColor *points,
	int ncolors, struct SFColor *colors,
	int nnormals, struct SFColor *normals,
	int ntexcoords, struct SFVec2f *texcoords
	);
void regen_polyrep(void *node) ;

void rayhit(float rat, float cx,float cy,float cz, float nx,float ny,float nz,
float tx,float ty, char *descr) ;

void fwnorprint (float *norm);


void Elev_Tri (
        int vertex_ind,
        int this_face,
        int A,
        int D,
        int E,
        int NONORMALS,
        struct VRML_PolyRep *this_Elev,
        struct pt *facenormals,
        int *pointfaces,
	int ccw);

void Extru_check_normal (
        struct pt *facenormals,
        int this_face,
        float direction,
        struct VRML_PolyRep  *rep_,
	int ccw);

void Extru_tex(
	int vertex_ind,
	int tci_ct,
	int A,
	int B,
	int C,
	struct VRML_PolyRep *this_Elev,
	int ccw,
	int tcindexsize);

void Extru_init_tex_cap_vals();

void Extru_ST_map(
        int triind_start,
        int start, 
        int end,
	float *Vals,
	int nsec,
        struct VRML_PolyRep *this_Extru,
	int tcoordsize);

/* from the PNG examples */
unsigned char  *readpng_get_image(double display_exponent, int *pChannels,
		                       unsigned long *pRowbytes);




void normalize_vector(struct pt *vec);

void normalize_ifs_face (float *point_normal,
                         struct pt *facenormals,
                         int *pointfaces,
                        int mypoint,
                        int curpoly,
                        float creaseAngle);

void render_ray_polyrep(void *node,
        int npoints, struct SFColor *points);


void FW_rendertext(int n,SV **p,int nl, float *length, 
       float maxext, float spacing, float mysize, 
       unsigned int fsparam, struct VRML_PolyRep *rep_);
	




/* Triangulator extern defs - look in CFuncs/Tess.c */
extern struct VRML_PolyRep *global_tess_polyrep;
extern GLUtriangulatorObj *global_tessobj;
extern int global_IFS_Coords[];
extern int global_IFS_Coord_count;

/* do we have to do textures?? */
//#define HAVETODOTEXTURES  (glIsEnabled(GL_TEXTURE_2D))
// Rendering order changes created problems, so we assume textures all the
// time

#define HAVETODOTEXTURES glIsEnabled(GL_TEXTURE_2D)
#define BEHAVETODOTEXTURES  (1 || glIsEnabled(GL_TEXTURE_2D))

/* appearance does material depending on last texture depth */
extern int last_texture_depth;

/* does the sound come from an audioclip or from a movietexture?
int sound_from_audioclip;


/* what is the max texture size as set by FreeWRL? */
extern GLint global_texSize;


/* Text node system fonts. On startup, freewrl checks to see where the fonts
 * are stored
 */
#define fp_name_len 256
extern char sys_fp[fp_name_len];


/* Sound stuff */
float SoundSourceInit (int sourcenum, int loop, float pitch,
			float start_time, float stop_time, char *url);
