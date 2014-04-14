/*
=INSERT_TEMPLATE_HERE=

$Id$

???

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

#include <math.h>

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"

#include "LinearAlgebra.h"

#define DJ_KEEP_COMPILER_WARNING 0

/* Altenate implemetations available, should merge them eventually */
double *vecaddd(double *c, double* a, double *b)
{
	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1];
	c[2] = a[2] + b[2];
	return c;
}
float *vecadd3f(float *c, float *a, float *b)
{
	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1];
	c[2] = a[2] + b[2];
	return c;
}
double *vecdifd(double *c, double* a, double *b)
{
	c[0] = a[0] - b[0];
	c[1] = a[1] - b[1];
	c[2] = a[2] - b[2];
	return c;
}
float *vecdif3f(float *c, float *a, float *b)
{
	c[0] = a[0] - b[0];
	c[1] = a[1] - b[1];
	c[2] = a[2] - b[2];
	return c;
}
double *veccopyd(double *c, double *a)
{
	c[0] = a[0];
	c[1] = a[1];
	c[2] = a[2];
	return c;
}
float *veccopy3f(float *b, float *a)
{
	b[0] = a[0];
	b[1] = a[1];
	b[2] = a[2];
	return b;
}
double * veccrossd(double *c, double *a, double *b)
{
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
	return c;
}
double veclengthd( double *p )
{
	return sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
}
double vecdotd(double *a, double *b)
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
double* vecscaled(double* r, double* v, double s)
{
    r[0] = v[0] * s;
    r[1] = v[1] * s;
    r[2] = v[2] * s;
    return r;
}
/* returns vector length, too */
double vecnormald(double *r, double *v)
{
    double ret = sqrt(vecdotd(v,v));
    /* if(ret == 0.) return 0.; */
    if (APPROX(ret, 0)) return 0.;
    vecscaled(r,v,1./ret);
    return ret;
}

void veccross(struct point_XYZ *c, struct point_XYZ a, struct point_XYZ b)
{
    c->x = a.y * b.z - a.z * b.y;
    c->y = a.z * b.x - a.x * b.z;
    c->z = a.x * b.y - a.y * b.x;
}
float *veccross3f(float *c, float *a, float *b)
{
    c[0] = a[1]*b[2] - a[2]*b[1];
    c[1] = a[2]*b[0] - a[0]*b[2];
    c[2] = a[0]*b[1] - a[1]*b[0];
	return c;
}
float veclength( struct point_XYZ p )
{
    return (float) sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
}
float vecdot3f( float *a, float *b )
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
float *vecset3f(float *b, float x, float y, float z)
{
	b[0] = x; b[1] = y; b[2] = z;
	return b;
}
float veclength3f(float *a){
	return sqrt(vecdot3f(a, a));
}

double vecangle(struct point_XYZ* V1, struct point_XYZ* V2) {
    return acos((V1->x*V2->x + V1->y*V2->y +V1->z*V2->z) /
		sqrt( (V1->x*V1->x + V1->y*V1->y + V1->z*V1->z)*(V2->x*V2->x + V2->y*V2->y + V2->z*V2->z) )  );
};


float calc_angle_between_two_vectors(struct point_XYZ a, struct point_XYZ b)
{
    float length_a, length_b, scalar, temp;
    scalar = (float) calc_vector_scalar_product(a,b);
    length_a = calc_vector_length(a);
    length_b = calc_vector_length(b);

    /* printf("scalar: %f  length_a: %f  length_b: %f \n", scalar, length_a, length_b);*/

    /* if (scalar == 0) */
    if (APPROX(scalar, 0)) {
		return (float) (M_PI/2.0);
    }

    if ( (length_a <= 0)  || (length_b <= 0)){
	printf("Divide by 0 in calc_angle_between_two_vectors():  No can do! \n");
	return 0;
    }

    temp = scalar /(length_a * length_b);
    /*  printf("temp: %f", temp);*/

    /*acos() appears to be unable to handle 1 and -1  */
    /* fixed to handle border case where temp <=-1.0 for 0.39 JAS */
    if ((temp >= 1) || (temp <= -1)){
	if (temp < 0.0f) return 3.141526f;
	return 0.0f;
    }
    return (float) acos(temp);
}

/* returns vector length, too */
GLDOUBLE vecnormal(struct point_XYZ*r, struct point_XYZ* v)
{
    GLDOUBLE ret = sqrt(vecdot(v,v));
    /* if(ret == 0.) return 0.; */
    if (APPROX(ret, 0)) return 0.;
    vecscale(r,v,1./ret);
    return ret;
}
float vecnormsquared3f(float *a){
	return a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
}
float *vecscale3f(float *b, float *a, float scale){
	b[0] = a[0] * scale;
	b[1] = a[1] * scale;
	b[2] = a[2] * scale;
	return b;
}

float *vecnormalize3f(float *b, float *a)
{
	float norm = veclength3f(a);
	if (APPROX(norm, 0.0f)){
		b[0] = 1.0f; b[1] = 0.0f; b[2] = 0.0f;
	}else{
		vecscale3f(b, a, 1.0f / norm);
	}
	return b;
}
/*will add functions here as needed. */

GLDOUBLE det3x3(GLDOUBLE* data)
{
    return -data[1]*data[10]*data[4] +data[0]*data[10]*data[5] -data[2]*data[5]*data[8] +data[1]*data[6]*data[8] +data[2]*data[4]*data[9] -data[0]*data[6]*data[9];
}
float det3f(float *a, float *b, float *c)
{
	float temp[3];
	return vecdot3f(a,veccross3f(temp,b, c));
}

struct point_XYZ* transform(struct point_XYZ* r, const struct point_XYZ* a, const GLDOUBLE* b)
{
    struct point_XYZ tmp; /* JAS*/

    if(r != a) { /*protect from self-assignments */
	r->x = b[0]*a->x +b[4]*a->y +b[8]*a->z +b[12];
	r->y = b[1]*a->x +b[5]*a->y +b[9]*a->z +b[13];
	r->z = b[2]*a->x +b[6]*a->y +b[10]*a->z +b[14];
    } else {
	/* JAS was  - struct point_XYZ tmp = {a->x,a->y,a->z};*/
	tmp.x = a->x; tmp.y = a->y; tmp.z = a->z;
	r->x = b[0]*tmp.x +b[4]*tmp.y +b[8]*tmp.z +b[12];
	r->y = b[1]*tmp.x +b[5]*tmp.y +b[9]*tmp.z +b[13];
	r->z = b[2]*tmp.x +b[6]*tmp.y +b[10]*tmp.z +b[14];
    }
    return r;
}

float* transformf(float* r, const float* a, const GLDOUBLE* b)
{
    float tmp[3];  /* JAS*/

    if(r != a) { /*protect from self-assignments */
	r[0] = (float) (b[0]*a[0] +b[4]*a[1] +b[8]*a[2] +b[12]);
	r[1] = (float) (b[1]*a[0] +b[5]*a[1] +b[9]*a[2] +b[13]);
	r[2] = (float) (b[2]*a[0] +b[6]*a[1] +b[10]*a[2] +b[14]);
    } else {
	tmp[0] =a[0]; tmp[1] = a[1]; tmp[2] = a[2]; /* JAS*/
	r[0] = (float) (b[0]*tmp[0] +b[4]*tmp[1] +b[8]*tmp[2] +b[12]);
	r[1] = (float) (b[1]*tmp[0] +b[5]*tmp[1] +b[9]*tmp[2] +b[13]);
	r[2] = (float) (b[2]*tmp[0] +b[6]*tmp[1] +b[10]*tmp[2] +b[14]);
    }
    return r;
}
/*transform point, but ignores translation.*/
struct point_XYZ* transform3x3(struct point_XYZ* r, const struct point_XYZ* a, const GLDOUBLE* b)
{
    struct point_XYZ tmp;

    if(r != a) { /*protect from self-assignments */
	r->x = b[0]*a->x +b[4]*a->y +b[8]*a->z;
	r->y = b[1]*a->x +b[5]*a->y +b[9]*a->z;
	r->z = b[2]*a->x +b[6]*a->y +b[10]*a->z;
    } else {
	/* JAS struct point_XYZ tmp = {a->x,a->y,a->z};*/
	tmp.x = a->x; tmp.y = a->y; tmp.z = a->z;
	r->x = b[0]*tmp.x +b[4]*tmp.y +b[8]*tmp.z;
	r->y = b[1]*tmp.x +b[5]*tmp.y +b[9]*tmp.z;
	r->z = b[2]*tmp.x +b[6]*tmp.y +b[10]*tmp.z;
    }
    return r;
}

struct point_XYZ* vecscale(struct point_XYZ* r, struct point_XYZ* v, GLDOUBLE s)
{
    r->x = v->x * s;
    r->y = v->y * s;
    r->z = v->z * s;
    return r;
}
double vecdot(struct point_XYZ* a, struct point_XYZ* b)
{
    return (a->x*b->x) + (a->y*b->y) + (a->z*b->z);
}


/* returns 0 if p1 is closest, 1 if p2 is closest, and a fraction if the closest point is in between */
/* To get the closest point, use pclose = retval*p1 + (1-retval)p2; */
/* y1 must be smaller than y2 */
/*double closest_point_of_segment_to_y_axis_segment(double y1, double y2, struct point_XYZ p1, struct point_XYZ p2) {
  double imin = (y1- p1.y) / (p2.y - p1.y);
  double imax = (y2- p1.y) / (p2.y - p1.y);

  double x21 = (p2.x - p1.x);
  double z21 = (p2.z - p1.z);
  double i = (p2.x * x21 + p2.z * z21) /
  ( x21*x21 + z21*z21 );
  return max(min(i,imax),imin);

  }*/

double closest_point_of_segment_to_y_axis_segment(double y1, double y2, struct point_XYZ p1, struct point_XYZ p2) {
    /*cylinder constraints (to be between y1 and y2) */
    double imin = (y1- p1.y) / (p2.y - p1.y);
    double imax = (y2- p1.y) / (p2.y - p1.y);

    /*the equation */
    double x12 = (p1.x - p2.x);
    double z12 = (p1.z - p2.z);
    double q = ( x12*x12 + z12*z12 );

    /* double i = ((q == 0.) ? 0 : (p1.x * x12 + p1.z * z12) / q); */
    double i = ((APPROX(q, 0)) ? 0 : (p1.x * x12 + p1.z * z12) / q);

    printf("imin=%f, imax=%f => ",imin,imax);

    if(imin > imax) {
	double tmp = imax;
	imax = imin;
	imin = tmp;
    }

    /*clamp constraints to segment*/
    if(imin < 0) imin = 0;
    if(imax > 1) imax = 1;

    printf("imin=%f, imax=%f\n",imin,imax);

    /*clamp result to constraints */
    if(i < imin) i = imin;
    if(i > imax) i = imax;
    return i;

}
BOOL line_intersect_line_3f(float *p1, float *v1, float *p2, float *v2, float *t, float *s, float *x1, float *x2)
{
	//from Graphics Gems I, p.304 http://inis.jinr.ru/sl/vol1/CMC/Graphics_Gems_1,ed_A.Glassner.pdf
	//L1: P1 + V1*t
	//L2: P2 + V2*s
	//t and s are at the footpoint/point of closest passing
	//t = det(P2-P1,V2,V1xV2) / |V1 x V2|**2
	//s = det(P2-P1,V1,V1xV2) / |V1 x V2|**2
	float t1[3], t2[3], cross[3]; //temp intermediate variables
	float crosslength2, det, ss, tt;
	veccross3f(cross, v1, v2);
	crosslength2 = vecnormsquared3f(cross);
	if (APPROX(crosslength2, 0.0f)) return FALSE; //lines are parallel, no intersection
	crosslength2 = 1.0f / crosslength2;
	tt = det3f(vecdif3f(t1, p2, p1), v2, cross) * crosslength2;
	ss = det3f(vecdif3f(t1, p2, p1), v1, cross) * crosslength2;
	if (x1)
		vecadd3f(x1, p1, vecscale3f(t1, v1, tt));
	if (x2)
		vecadd3f(x2, p2, vecscale3f(t1, v2, ss));
	if (t)
		*t = tt;
	if (s)
		*s = ss;
	return TRUE; //success we have 2 footpoints.
}

BOOL line_intersect_planed_3f(float *p, float *v, float *N, float d, float *pi, float *t)
{
	//from graphics gems I, p.391 http://inis.jinr.ru/sl/vol1/CMC/Graphics_Gems_1,ed_A.Glassner.pdf
	// V dot N = d = const for points on a plane, or N dot P + d = 0
	// line/ray P1 + v1*t = P2 (intersection point)
	// combining t = -(d + N dot P1)/(N dot v1)
	float t1[3], t2[3], nd, tt;
	nd = vecdot3f(N, v);
	if (APPROX(nd, 0.0f)) return FALSE;
	tt = -(d + vecdot3f(N, p)) / nd;
	vecadd3f(t2, p, vecscale3f(t1, v, tt));
	if (t) *t = tt;
	if (pi) veccopy3f(pi, t2);
	return TRUE;
}
BOOL line_intersect_plane_3f(float *p, float *v, float *N, float *pp, float *pi, float *t)
{
	float d;
	d = vecdot3f(N, pp);
	return line_intersect_planed_3f(p, v, N, d, pi, t);
}

BOOL line_intersect_cylinder_3f(float *p, float *v, float radius, float *pi)
{
	//from rendray_Cylinder
	//intersects arbitrary ray (p,v) with cylinder of radius, origiin 0,0,0 and axis 0,1,0
	//april 2014: NOT TESTED, NOT USED - just hacked in and compiled
    //    if((!XEQ) && (!ZEQ)) {
	float t2[3], pp[3];
	float dx = v[0]; 
	float dz = v[2];
	float a = dx*dx + dz*dz;
	float b = 2.0f*(dx * p[0] + dz * p[2]);
	float c = p[0] * p[0] + p[2] * p[2] - radius*radius;
	float und;
	if (APPROX(a, 0.0f))return FALSE;
	b /= a; c /= a;
	und = b*b - 4*c;
	if(und > 0) { /* HITS the infinite cylinder */
		float t2[3];
		float sol1 = (-b+(float) sqrt(und))/2;
		float sol2 = (-b-(float) sqrt(und))/2;
		float sol = sol2;// sol1 < sol2 ? sol1 : sol2; //take the one closest to p (but should these be abs? what about direction 
		vecadd3f(pp, p, vecscale3f(t2, v, sol));
		if (pi) veccopy3f(pi, pp);
		return TRUE;
	}
     // }
	return FALSE;
}

struct point_XYZ* vecadd(struct point_XYZ* r, struct point_XYZ* v, struct point_XYZ* v2)
{
    r->x = v->x + v2->x;
    r->y = v->y + v2->y;
    r->z = v->z + v2->z;
    return r;
}

struct point_XYZ* vecdiff(struct point_XYZ* r, struct point_XYZ* v, struct point_XYZ* v2)
{
    r->x = v->x - v2->x;
    r->y = v->y - v2->y;
    r->z = v->z - v2->z;
    return r;
}

/*i,j,n will form an orthogonal vector space */
void make_orthogonal_vector_space(struct point_XYZ* i, struct point_XYZ* j, struct point_XYZ n) {
    /* optimal axis finding algorithm. the solution isn't unique.*/
    /*  each of these three calculations doesn't work (or works poorly)*/
    /*  in certain distinct cases. (gives zero vectors when two axes are 0)*/
    /*  selecting the calculations according to smallest axis avoids this problem.*/
    /*  (the two remaining axis are thus far from zero, if n is normal)*/
    if(fabs(n.x) <= fabs(n.y) && fabs(n.x) <= fabs(n.z)) { /* x smallest*/
	i->x = 0;
	i->y = n.z;
	i->z = -n.y;
	vecnormal(i,i);
	j->x = n.y*n.y + n.z*n.z;
	j->y = (-n.x)*n.y;
	j->z = (-n.x)*n.z;
    } else if(fabs(n.y) <= fabs(n.x) && fabs(n.y) <= fabs(n.z)) { /* y smallest*/
	i->x = -n.z;
	i->y = 0;
	i->z = n.x;
	vecnormal(i,i);
	j->x = (-n.x)*n.y;
	j->y = n.x*n.x + n.z*n.z;
	j->z = (-n.y)*n.z;
    } else { /* z smallest*/
	i->x = n.y;
	i->y = -n.x;
	i->z = 0;
	vecnormal(i,i);
	j->x = (-n.x)*n.z;
	j->y = (-n.y)*n.z;
	j->z =  n.x*n.x + n.y*n.y;
    }
}


GLDOUBLE* mattranspose(GLDOUBLE* res, GLDOUBLE* mm)
{
	GLDOUBLE mcpy[16];
	int i, j;
	GLDOUBLE *m;

	m = mm;
	if(res == m) {
		memcpy(mcpy,m,sizeof(GLDOUBLE)*16);
		m = mcpy;
	}

	for (i = 0; i < 4; i++) {
		//for (j = i + 1; j < 4; j++) {
		//	res[i*4+j] = m[j*4+i];
		//}
		for (j = 0; j < 4; j++) {
			res[i*4+j] = m[j*4+i];
		}
	}
	return res;
}


GLDOUBLE* matinverse(GLDOUBLE* res, GLDOUBLE* mm)
{
    double Deta;
    GLDOUBLE mcpy[16];
	GLDOUBLE *m;

	m = mm;
    if(res == m) {
	memcpy(mcpy,m,sizeof(GLDOUBLE)*16);
	m = mcpy;
    }

    Deta = det3x3(m);
	Deta = 1.0 / Deta;

    res[0] = (-m[9]*m[6] +m[5]*m[10])*Deta;
    res[4] = (+m[8]*m[6] -m[4]*m[10])*Deta;
    res[8] = (-m[8]*m[5] +m[4]*m[9])*Deta;
    res[12] = ( m[12]*m[9]*m[6] -m[8]*m[13]*m[6] -m[12]*m[5]*m[10] +m[4]*m[13]*m[10] +m[8]*m[5]*m[14] -m[4]*m[9]*m[14])*Deta;

    res[1] = (+m[9]*m[2] -m[1]*m[10])*Deta;
    res[5] = (-m[8]*m[2] +m[0]*m[10])*Deta;
    res[9] = (+m[8]*m[1] -m[0]*m[9])*Deta;
    res[13] = (-m[12]*m[9]*m[2] +m[8]*m[13]*m[2] +m[12]*m[1]*m[10] -m[0]*m[13]*m[10] -m[8]*m[1]*m[14] +m[0]*m[9]*m[14])*Deta;

    res[2] = (-m[5]*m[2] +m[1]*m[6])*Deta;
    res[6] = (+m[4]*m[2] -m[0]*m[6])*Deta;
    res[10] = (-m[4]*m[1] +m[0]*m[5])*Deta;
    res[14] = ( m[12]*m[5]*m[2] -m[4]*m[13]*m[2] -m[12]*m[1]*m[6] +m[0]*m[13]*m[6] +m[4]*m[1]*m[14] -m[0]*m[5]*m[14])*Deta;

    res[3] = (+m[5]*m[2]*m[11] -m[1]*m[6]*m[11])*Deta;
    res[7] = (-m[4]*m[2]*m[11] +m[0]*m[6]*m[11])*Deta;
    res[11] = (+m[4]*m[1]*m[11] -m[0]*m[5]*m[11])*Deta;
    res[15] = (-m[8]*m[5]*m[2] +m[4]*m[9]*m[2] +m[8]*m[1]*m[6] -m[0]*m[9]*m[6] -m[4]*m[1]*m[10] +m[0]*m[5]*m[10])*Deta;

    return res;
}

struct point_XYZ* polynormal(struct point_XYZ* r, struct point_XYZ* p1, struct point_XYZ* p2, struct point_XYZ* p3) {
    struct point_XYZ v1;
    struct point_XYZ v2;
    VECDIFF(*p2,*p1,v1);
    VECDIFF(*p3,*p1,v2);
    veccross(r,v1,v2);
    vecnormal(r,r);
    return r;
}

/*simple wrapper for now. optimize later */
struct point_XYZ* polynormalf(struct point_XYZ* r, float* p1, float* p2, float* p3) {
    struct point_XYZ pp[3];
    pp[0].x = p1[0];
    pp[0].y = p1[1];
    pp[0].z = p1[2];
    pp[1].x = p2[0];
    pp[1].y = p2[1];
    pp[1].z = p2[2];
    pp[2].x = p3[0];
    pp[2].y = p3[1];
    pp[2].z = p3[2];
    return polynormal(r,pp+0,pp+1,pp+2);
}


GLDOUBLE* matrotate(GLDOUBLE* Result, double Theta, double x, double y, double z)
{
    GLDOUBLE CosTheta = cos(Theta);
    GLDOUBLE SinTheta = sin(Theta);

    Result[0] = x*x + (y*y+z*z)*CosTheta;
    Result[1] = x*y - x*y*CosTheta + z*SinTheta;
    Result[2] = x*z - x*z*CosTheta - y*SinTheta;
    Result[4] = x*y - x*y*CosTheta - z*SinTheta;
    Result[5] = y*y + (x*x+z*z)*CosTheta;
    Result[6] = z*y - z*y*CosTheta + x*SinTheta;
    Result[8] = z*x - z*x*CosTheta + y*SinTheta;
    Result[9] = z*y - z*y*CosTheta - x*SinTheta;
    Result[10]= z*z + (x*x+y*y)*CosTheta;
    Result[3] = 0;
    Result[7] = 0;
    Result[11] = 0;
    Result[12] = 0;
    Result[13] = 0;
    Result[14] = 0;
    Result[15] = 1;

    return Result;
}

GLDOUBLE* mattranslate(GLDOUBLE* r, double dx, double dy, double dz)
{
    r[0] = r[5] = r[10] = r[15] = 1;
    r[1] = r[2] = r[3] = r[4] =
	r[6] = r[7] = r[8] = r[9] =
	r[11] = 0;
    r[12] = dx;
    r[13] = dy;
    r[14] = dz;
    return r;
}

GLDOUBLE* matmultiply(GLDOUBLE* r, GLDOUBLE* mm , GLDOUBLE* nn)
{
    GLDOUBLE tm[16],tn[16];
	GLDOUBLE *m, *n;
	int i,j,k;
    /* prevent self-multiplication problems.*/
	m = mm;
	n = nn;
    if(r == m) {
	memcpy(tm,m,sizeof(GLDOUBLE)*16);
	m = tm;
    }
    if(r == n) {
	memcpy(tn,n,sizeof(GLDOUBLE)*16);
	n = tn;
    }
	/* assume 4x4 homgenous transform */
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
		{
			r[i*4+j] = 0.0;
			for(k=0;k<4;k++)
				r[i*4+j] += m[i*4+k]*n[k*4+j];
		}
	return r;
	/* this method ignors the perspectives */
    r[0] = m[0]*n[0]+m[4]*n[1]+m[8]*n[2];
    r[4] = m[0]*n[4]+m[4]*n[5]+m[8]*n[6];
    r[8] = m[0]*n[8]+m[4]*n[9]+m[8]*n[10];
    r[12]= m[0]*n[12]+m[4]*n[13]+m[8]*n[14]+m[12];

    r[1] = m[1]*n[0]+m[5]*n[1]+m[9]*n[2];
    r[5] = m[1]*n[4]+m[5]*n[5]+m[9]*n[6];
    r[9] = m[1]*n[8]+m[5]*n[9]+m[9]*n[10];
    r[13]= m[1]*n[12]+m[5]*n[13]+m[9]*n[14]+m[13];

    r[2] = m[2]*n[0]+m[6]*n[1]+m[10]*n[2];
    r[6] = m[2]*n[4]+m[6]*n[5]+m[10]*n[6];
    r[10]= m[2]*n[8]+m[6]*n[9]+m[10]*n[10];
    r[14]= m[2]*n[12]+m[6]*n[13]+m[10]*n[14]+m[14];

    return r;
}
float *axisangle_rotate3f(float* b, float *a, float *axisangle)
{
	/*	http://en.wikipedia.org/wiki/Axis%E2%80%93angle_representation
	uses Rodrigues formula axisangle (axis,angle)
	somewhat expensive, so if tranforming many points with the same rotation, 
		it might be more efficient to use another method (like axisangle -> matrix, then matrix transforms)
	b = a*cos(angle) + (axis cross a)*sin(theta) + axis*(axis dot a)*(1 - cos(theta))
	*/
	float cosine, sine, cross[3], dot, theta, *axis, t1[3], t2[3],t3[3],t4[3];
	theta = axisangle[3];
	axis = axisangle;
	cosine = cos(theta);
	sine = sin(theta);
	veccross3f(cross,axis, a);
	dot = vecdot3f(axis, a);
	vecadd3f(b,vecscale3f(t1, a, cosine), vecadd3f(t2, vecscale3f(t3, cross, sine), vecscale3f(t4, axis, dot*(1.0f - cosine))));
	return b;
}
void rotate_v2v_axisAngled(double* axis, double* angle, double *orig, double *result)
{
    double cvl;
	double dv[3], iv[3], cv[3];

	/* step 1 get sin of angle between 2 vectors using cross product rule: ||u x v|| = ||u||*||v||*sin(theta) */
    vecnormald(dv,orig); /*normalizes vector to unit length U -> u^ (length 1) */
    vecnormald(iv,result);

    veccrossd(cv,dv,iv); /*the axis of rotation cv = dv X iv*/
    cvl = vecnormald(cv,cv); /* cvl = ||u x v|| / ||u^||*||v^|| = ||u x v|| = sin(theta)*/
	veccopyd(axis,cv);

    /* if(cvl == 0) { */
    if(APPROX(cvl, 0)) {
		cv[2] = 1.0;
	}
	/* step 2 get cos of angle between 2 vectors using dot product rule: u dot v = ||u||*||v||*cos(theta) or cos(theta) = u dot v/( ||u|| ||v||) 
	   or, since U,V already unit length from being normalized, cos(theta) = u dot v */
    *angle = atan2(cvl,vecdotd(dv,iv)); /*the angle theta = arctan(rise/run) = atan2(sin_theta,cos_theta) in radians*/
}
/*puts dv back on iv - return the 4x4 rotation matrix that will rotate vector dv onto iv*/
double matrotate2v(GLDOUBLE* res, struct point_XYZ iv/*original*/, struct point_XYZ dv/*result*/) {
    struct point_XYZ cv;
    double cvl,a;

	/* step 1 get sin of angle between 2 vectors using cross product rule: ||u x v|| = ||u||*||v||*sin(theta) */
    vecnormal(&dv,&dv); /*normalizes vector to unit length U -> u^ (length 1) */
    vecnormal(&iv,&iv);

    veccross(&cv,dv,iv); /*the axis of rotation cv = dv X iv*/
    cvl = vecnormal(&cv,&cv); /* cvl = ||u x v|| / ||u^||*||v^|| = ||u x v|| = sin(theta)*/
    /* if(cvl == 0) { */
    if(APPROX(cvl, 0)) {
		cv.z = 1;
	}
	/* step 2 get cos of angle between 2 vectors using dot product rule: u dot v = ||u||*||v||*cos(theta) or cos(theta) = u dot v/( ||u|| ||v||) 
	   or, since U,V already unit length from being normalized, cos(theta) = u dot v */
    a = atan2(cvl,vecdot(&dv,&iv)); /*the angle theta = arctan(rise/run) = atan2(sin_theta,cos_theta) in radians*/
    /* step 3 convert rotation angle around unit directional vector of rotation into an equivalent rotation matrix 4x4 */
    matrotate(res,a,cv.x,cv.y,cv.z);
    return a;
}



/****
 * hacked from a graphics gem
 * Returned value:
 *   TRUE   if input matrix is nonsingular
 *   FALSE  otherwise
 *
 ***/

BOOL matrix3x3_inverse_float(float *inn, float *outt)
{
	
    float    det_1;
    float    pos, /* neg, */ temp;
	float *in[3], *out[3];

/*#define ACCUMULATE    \
//    if (temp >= 0.0)  \
//        pos += temp;  \
//    else              \
        neg += temp;
*/

#define ACCUMULATE pos += temp;

//#define PRECISION_LIMIT 1.0e-7 //(1.0e-15)
	in[0] = &inn[0];
	in[1] = &inn[3];
	in[2] = &inn[6];
	out[0] = &outt[0];
	out[1] = &outt[3];
	out[2] = &outt[6];

    /*
     * Calculate the determinant of submatrix A and determine if the
     * the matrix is singular as limited by the double precision
     * floating-point data representation.
     */
    pos = 0.0f; //neg = 0.0;
    temp =  in[0][0] * in[1][1] * in[2][2];
    ACCUMULATE
    temp =  in[0][1] * in[1][2] * in[2][0];
    ACCUMULATE
    temp =  in[0][2] * in[1][0] * in[2][1];
    ACCUMULATE
    temp = -in[0][2] * in[1][1] * in[2][0];
    ACCUMULATE
    temp = -in[0][1] * in[1][0] * in[2][2];
    ACCUMULATE
    temp = -in[0][0] * in[1][2] * in[2][1];
    ACCUMULATE
    det_1 = pos; // + neg;

    /* Is the submatrix A singular? */
    //if ((det_1 == 0.0) || (abs(det_1 / (pos - neg)) < PRECISION_LIMIT)) {
	if(APPROX(det_1,0.0f)){

        /* Matrix M has no inverse */
        fprintf (stderr, "affine_matrix4_inverse: singular matrix\n");
        return FALSE;
    }

    else {

        /* Calculate inverse(A) = adj(A) / det(A) */
        det_1 = 1.0f / det_1;
        out[0][0] =  (in[1][1] * in[2][2] - in[1][2] * in[2][1] ) * det_1;
        out[1][0] = -(in[1][0] * in[2][2] - in[1][2] * in[2][0] ) * det_1;
        out[2][0] =  (in[1][0] * in[2][1] - in[1][1] * in[2][0] ) * det_1;
        out[0][1] = -(in[0][1] * in[2][2] - in[0][2] * in[2][1] ) * det_1;
        out[1][1] =  (in[0][0] * in[2][2] - in[0][2] * in[2][0] ) * det_1;
        out[2][1] = -(in[0][0] * in[2][1] - in[0][1] * in[2][0] ) * det_1;
        out[0][2] =  (in[0][1] * in[1][2] - in[0][2] * in[1][1] ) * det_1;
        out[1][2] = -(in[0][0] * in[1][2] - in[0][2] * in[1][0] ) * det_1;
        out[2][2] =  (in[0][0] * in[1][1] - in[0][1] * in[1][0] ) * det_1;

        return TRUE;
    }
}


#ifdef COMMENT
/*fast crossproduct using references, that checks for auto-assignments */
GLDOUBLE* veccross(GLDOUBLE* r, GLDOUBLE* v1, GLDOUBLE* v2)
{
    /*check against self-assignment. */
    if(r != v1) {
	if(r != v2) {
	    r[0] = v1[1]*v2[2] - v1[2]*v2[1];
	    r[1] = v1[2]*v2[0] - v1[0]*v2[2];
	    r[2] = v1[0]*v2[1] - v1[1]*v2[0];
	} else { /* r == v2 */
	    GLDOUBLE v2c[3] = {v2[0],v2[1],v2[2]};
	    r[0] = v1[1]*v2c[2] - v1[2]*v2c[1];
	    r[1] = v1[2]*v2c[0] - v1[0]*v2c[2];
	    r[2] = v1[0]*v2c[1] - v1[1]*v2c[0];
	}
    } else { /* r == v1 */
	GLDOUBLE v1c[3] = {v1[0],v1[1],v1[2]};
	r[0] = v1c[1]*v2[2] - v1c[2]*v2[1];
	r[1] = v1c[2]*v2[0] - v1c[0]*v2[2];
	r[2] = v1c[0]*v2[1] - v1c[1]*v2[0];
    }
    return r;
}



#endif


/*
 * apply a scale to the matrix given. Assumes matrix is valid... 
 *
 */
	
void scale_to_matrix (double *mat, struct point_XYZ *scale) {
/* copy the definitions from quaternion.h... */
#define MAT00 mat[0]
#define MAT01 mat[1]
#define MAT02 mat[2]
#if DJ_KEEP_COMPILER_WARNING
#define MAT03 mat[3]
#endif
#define MAT10 mat[4]
#define MAT11 mat[5]
#define MAT12 mat[6]
#if DJ_KEEP_COMPILER_WARNING
#define MAT13 mat[7]
#endif
#define MAT20 mat[8]
#define MAT21 mat[9]
#define MAT22 mat[10]
#if DJ_KEEP_COMPILER_WARNING
#define MAT23 mat[11]
#define MAT30 mat[12]
#define MAT31 mat[13]
#define MAT32 mat[14]
#define MAT33 mat[15]
#endif

	MAT00 *=scale->x;
	MAT01 *=scale->x;
	MAT02 *=scale->x;
	MAT10 *=scale->y;
	MAT11 *=scale->y;
	MAT12 *=scale->y;
	MAT20 *=scale->z;
	MAT21 *=scale->z;
	MAT22 *=scale->z;
}

static double identity[] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };

void loadIdentityMatrix (double *mat) {
        memcpy((void *)mat, (void *)identity, sizeof(double)*16);
}

#ifdef OLDCODE
OLDCODEvoid point_XYZ_slerp(struct point_XYZ *ret, struct point_XYZ *p1, struct point_XYZ *p2, const double t)
OLDCODE{
OLDCODE	//not tested as of July16,2011
OLDCODE	//goal start slow, speed up in the middle, and slow down when stopping 
OLDCODE	// (like a sine or cosine wave)
OLDCODE	//let omega = t*pi 
OLDCODE	//then cos omega goes from 1 to -1 natively
OLDCODE	//we want scale0 to go from 1 to 0
OLDCODE	//scale0 = .5(1+cos(t*pi)) should be in the 1 to 0 range,
OLDCODE	//and be 'fastest' in the middle ie at pi/2 
OLDCODE	//then scale1 = 1 - scale0
OLDCODE	double scale0, scale1, omega;
OLDCODE
OLDCODE	/* calculate coefficients */
OLDCODE	if ( t > .05 || t < .95 ) {
OLDCODE		/* standard case (SLERP) */
OLDCODE		omega = t*PI;
OLDCODE		scale0 = 0.5*(1.0 + cos(omega));
OLDCODE		scale1 = 1.0 - scale0;
OLDCODE	} else {
OLDCODE		/* p1 & p2 are very close, so do linear interpolation */
OLDCODE		scale0 = 1.0 - t;
OLDCODE		scale1 = t;
OLDCODE	}
OLDCODE	ret->x = scale0 * p1->x + scale1 * p2->x;
OLDCODE	ret->y = scale0 * p1->y + scale1 * p2->y;
OLDCODE	ret->z = scale0 * p1->z + scale1 * p2->z;
OLDCODE}
#endif //OLDCODE

void general_slerp(double *ret, double *p1, double *p2, int size, const double t)
{
	//not tested as of July16,2011
	//goal start slow, speed up in the middle, and slow down when stopping 
	// (like a sine or cosine wave)
	//let omega = t*pi 
	//then cos omega goes from 1 to -1 natively
	//we want scale0 to go from 1 to 0
	//scale0 = .5(1+cos(t*pi)) should be in the 1 to 0 range,
	//and be 'fastest' in the middle ie at pi/2 
	//then scale1 = 1 - scale0
	double scale0, scale1, omega;
	int i;
	/* calculate coefficients */
	if (0) {
	//if ( t > .05 || t < .95 ) {
		/* standard case (SLERP) */
		omega = t*PI;
		scale0 = 0.5*(1.0 + cos(omega));
		scale1 = 1.0 - scale0;
	} else {
		/* p1 & p2 are very close, so do linear interpolation */
		scale0 = 1.0 - t;
		scale1 = t;
	}
	for(i=0;i<size;i++)
		ret[i] = scale0 * p1[i] + scale1 * p2[i];
}
