
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

#define VECSQ(a) VECPT(a,a)
#define VECPT(a,b) ((a).x*(b).x + (a).y*(b).y + (a).z*(b).z)
#define VECDIFF(a,b,c) {(c).x = (a).x-(b).x;(c).y = (a).y-(b).y;(c).z = (a).z-(b).z;}
#define VEC_FROM_CDIFF(a,b,r) {(r).x = (a).c[0]-(b).c[0];(r).y = (a).c[1]-(b).c[1];(r).z = (a).c[2]-(b).c[2];}
#define VECCP(a,b,c) {(c).x = (a).y*(b).z-(b).y*(a).z; (c).y = -((a).x*(b).z-(b).x*(a).z); (c).z = (a).x*(b).y-(b).x*(a).y;}
#define VECSCALE(a,c) {(a).x *= c; (a).y *= c; (a).z *= c;}

/*special case ; used in Extrusion.GenPolyRep and ElevationGrid.GenPolyRep: 
 *	Calc diff vec from 2 coordvecs which must be in the same field 	*/
#define VEC_FROM_COORDDIFF(f,a,g,b,v) {\
	(v).x= (f)[(a)*3]-(g)[(b)*3];	\
	(v).y= (f)[(a)*3+1]-(g)[(b)*3+1];	\
	(v).z= (f)[(a)*3+2]-(g)[(b)*3+2];	\
}

/* rotate a vector along one axis				*/
#define VECROTATE_X(c,angle) { \
	/*(c).x =  (c).x	*/ \
	  (c).y = 		  cos(angle) * (c).y 	- sin(angle) * (c).z; \
	  (c).z = 		  sin(angle) * (c).y 	+ cos(angle) * (c).z; \
	}
#define VECROTATE_Y(c,angle) { \
	  (c).x = cos(angle)*(c).x +			+ sin(angle) * (c).z; \
	/*(c).y = 				(c).y 	*/ \
	  (c).z = -sin(angle)*(c).x 			+ cos(angle) * (c).z; \
	}
#define VECROTATE_Z(c,angle) { \
	  (c).x = cos(angle)*(c).x - sin(angle) * (c).y;	\
	  (c).y = sin(angle)*(c).x + cos(angle) * (c).y; 	\
	/*(c).z = s						 (c).z; */ \
	}

#define MATRIX_ROTATION_X(angle,m) {\
	m[0][0]=1; m[0][1]=0; m[0][2]=0; \
	m[1][0]=0; m[1][1]=cos(angle); m[1][2]=- sin(angle); \
	m[2][0]=0; m[2][1]=sin(angle); m[2][2]=cos(angle); \
}
#define MATRIX_ROTATION_Y(angle,m) {\
	m[0][0]=cos(angle); m[0][1]=0; m[0][2]=sin(angle); \
	m[1][0]=0; m[1][1]=1; m[1][2]=0; \
	m[2][0]=-sin(angle); m[2][1]=0; m[2][2]=cos(angle); \
}
#define MATRIX_ROTATION_Z(angle,m) {\
	m[0][0]=cos(angle); m[0][1]=- sin(angle); m[0][2]=0; \
	m[1][0]=sin(angle); m[1][1]=cos(angle); m[1][2]=0; \
	m[2][0]=0; m[2][1]=0; m[2][2]=1; \
}

/* next matrix calculation comes from comp.graphics.algorithms FAQ	*/
/* the axis vector has to be normalized					*/
#define MATRIX_FROM_ROTATION(ro,m) { \
	struct { double x,y,z,w ; } __q; \
        double sinHalfTheta = sin(0.5*(ro.r[3]));\
        double xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;\
        __q.x = (ro.r[0])*sinHalfTheta;\
        __q.y = (ro.r[1])*sinHalfTheta;\
        __q.z = (ro.r[2])*sinHalfTheta;\
        __q.w = cos(0.5*(ro.r[3]));\
        xs = 2*__q.x;  ys = 2*__q.y;  zs = 2*__q.z;\
        wx = __q.w*xs; wy = __q.w*ys; wz = __q.w*zs;\
        xx = __q.x*xs; xy = __q.x*ys; xz = __q.x*zs;\
        yy = __q.y*ys; yz = __q.y*zs; zz = __q.z*zs;\
        m[0][0] = 1 - (yy + zz); m[0][1] = xy - wz;      m[0][2] = xz + wy;\
        m[1][0] = xy + wz;       m[1][1] = 1 - (xx + zz);m[1][2] = yz - wx;\
        m[2][0] = xz - wy;       m[2][1] = yz + wx;      m[2][2] = 1-(xx + yy);\
}

/* matrix multiplication */
#define VECMM(m,c) { \
	double ___x=(c).x,___y=(c).y,___z=(c).z; \
	(c).x= m[0][0]*___x + m[0][1]*___y + m[0][2]*___z; \
	(c).y= m[1][0]*___x + m[1][1]*___y + m[1][2]*___z; \
	(c).z= m[2][0]*___x + m[2][1]*___y + m[2][2]*___z; \
}

	
/* next define rotates vector c with rotation vector r and angle */
/*  after section 5.8 of the VRML`97 spec			 */

#define VECROTATE(rx,ry,rz,angle,nc) { \
	double ___x=(nc).x,___y=(nc).y,___z=(nc).z; \
	double ___c=cos(angle),  ___s=sin(angle), ___t=1-___c; \
	(nc).x=   (___t*((rx)*(rx))+___c)     *___x    \
	        + (___t*(rx)*(ry)  -___s*(rz))*___y    \
	        + (___t*(rx)*(rz)  +___s*(ry))*___z ;  \
	(nc).y=   (___t*(rx)*(ry)  +___s*(rz))*___x    \
	        + (___t*((ry)*(ry))+___c)     *___y    \
	        + (___t*(ry)*(rz)  -___s*(rx))*___z ;  \
	(nc).z=   (___t*(rx)*(rz)  -___s*(ry))*___x    \
	        + (___t*(ry)*(rz)  +___s*(rx))*___y    \
	        + (___t*((rz)*(rz))+___c)     *___z ;  \
	}


/*
#define VECROTATE(rx,ry,rz,angle,c) { \
	double ___c=cos(angle),  ___s=sin(angle), ___t=1-___c; \
	(c).x=   (___t*((rx)*(rx))+___c)     *(c).x    \
	       + (___t*(rx)*(ry)  +___s*(rz))*(c).y    \
	       + (___t*(rx)*(rz)  -___s*(ry))*(c).z ;  \
	(c).y=   (___t*(rx)*(ry)  -___s*(rz))*(c).x    \
	       + (___t*((ry)*(ry))+___c)     *(c).y    \
	       + (___t*(ry)*(rz)  +___s*(rx))*(c).z ;  \
	(c).z=   (___t*(rx)*(rz)  +___s*(ry))*(c).x    \
	       + (___t*(ry)*(rz)  -___s*(rx))*(c).y    \
	       + (___t*((rz)*(rz))+ ___c)    *(c).z ;  \
	}

*/
/* next define abbreviates VECROTATE with use of the SFRotation struct	*/
#define VECRROTATE(ro,c) VECROTATE((ro).r[0],(ro).r[1],(ro).r[2],(ro).r[3],c)	



#define HIT rayhit


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

float calc_vector_length( struct pt p );
void normalize_vector(struct pt *vec);

void fwnorprint (float *norm);


/* Triangulator extern defs - look in CFuncs/Tess.c */
extern struct VRML_PolyRep *global_tess_polyrep;
extern GLUtriangulatorObj *global_tessobj;
extern int global_IFS_Coords[];
extern int global_IFS_Coord_count;


