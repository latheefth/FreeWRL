#include "LinearAlgebra.h"


#include <stdio.h>
/* Altenate implemetations available, should merge them eventually */

void calc_vector_product(struct pt a, struct pt b, struct pt *c )
{
	c->x = a.y * b.z - a.z * b.y;
	c->y = a.z * b.x - a.x * b.z;
	c->z = a.x * b.y - a.y * b.x;
}

float calc_vector_length( struct pt p )
{
	return sqrt(p.x*p.x + p.y*p.y + p.z*p.z); 
}

float calc_angle_between_two_vectors(struct pt a, struct pt b)
{
	float length_a, length_b, scalar, temp;
	scalar = calc_vector_scalar_product(a,b);
	length_a = calc_vector_length(a);
	length_b = calc_vector_length(b);

	/*printf("scalar: %f  length_a: %f  length_b: %f \n", scalar, length_a, length_b);*/
	
	if (scalar == 0){
		return M_PI/2;	
	}

	if ( (length_a <= 0)  || (length_b <= 0)){
		printf("Divide by 0 in calc_angle_between_two_vectors():  No can do! \n");
		return 0;
	}
	
	temp = scalar /(length_a * length_b);
	/*printf("temp: %f", temp);*/

	/*acos() appears to be unable to handle 1 and -1  */
	if ((temp >= 1) || (temp <= -1)){
		return 0;
	}
	return acos(temp);
}

void normalize_vector(struct pt *vec)
{
	float vector_length;

	vector_length = calc_vector_length(*vec);
	
	vec->x = vec->x / vector_length;
	vec->y = vec->y / vector_length;
	vec->z = vec->z / vector_length;
}



/*will add functions here as needed. */

GLdouble det3x3(GLdouble* data) 
{
    return -data[1]*data[10]*data[4] +data[0]*data[10]*data[5] -data[2]*data[5]*data[8] +data[1]*data[6]*data[8] +data[2]*data[4]*data[9] -data[0]*data[6]*data[9];
}

struct pt* vecscale(struct pt* r, struct pt* v, GLdouble s)
{
    r->x = v->x * s;
    r->y = v->y * s;
    r->z = v->z * s;
    return r;
}

double vecdot(struct pt* a, struct pt* b)
{
	return (a->x*b->x) + (a->y*b->y) + (a->z*b->z);
}


/* returns 0 if p1 is closest, 1 if p2 is closest, and a fraction if the closest point is in between */
/* To get the closest point, use pclose = retval*p1 + (1-retval)p2; */
/* y1 must be smaller than y2 */
/*double closest_point_of_segment_to_y_axis_segment(double y1, double y2, struct pt p1, struct pt p2) {
     double imin = (y1- p1.y) / (p2.y - p1.y);
     double imax = (y2- p1.y) / (p2.y - p1.y);

     double x21 = (p2.x - p1.x);
     double z21 = (p2.z - p1.z);
     double i = (p2.x * x21 + p2.z * z21) /
	          ( x21*x21 + z21*z21 );
     return max(min(i,imax),imin);
     
     }*/

double closest_point_of_segment_to_y_axis_segment(double y1, double y2, struct pt p1, struct pt p2) {
     /*cylinder constraints (to be between y1 and y2) */
     double imin = (y1- p1.y) / (p2.y - p1.y);
     double imax = (y2- p1.y) / (p2.y - p1.y);

     /*the equation */
     double x12 = (p1.x - p2.x);
     double z12 = (p1.z - p2.z);
     double q = ( x12*x12 + z12*z12 );
     double i = ((q == 0.) ? 0 : (p1.x * x12 + p1.z * z12) / q);
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



#ifdef COMMENT
GLdouble vecangle(GLdouble* V1, GLdouble* V2) {
    return acos((V1[0]*V2[0] + V1[1]*V2[1] +V1[2]*V2[2]) /
		sqrt( (V1[0]*V1[0] + V1[1]*V1[1] + V1[2]*V1[2])*(V2[0]*V2[0] + V2[1]*V2[1] + V2[2]*V2[2]) )  );
};

GLdouble* veccross(GLdouble* r, GLdouble* v1, GLdouble* v2)
{
    /*check against self-assignment. */
    if(r != v1) {
	if(r != v2) {
	    r[0] = v1[1]*v2[2] - v1[2]*v2[1];
	    r[1] = v1[2]*v2[0] - v1[0]*v2[2];
	    r[2] = v1[0]*v2[1] - v1[1]*v2[0];
	} else { /* r == v2 */
	    GLdouble v2c[3] = {v2[0],v2[1],v2[2]};
	    r[0] = v1[1]*v2c[2] - v1[2]*v2c[1];
	    r[1] = v1[2]*v2c[0] - v1[0]*v2c[2];
	    r[2] = v1[0]*v2c[1] - v1[1]*v2c[0];
	}
    } else { /* r == v1 */
	GLdouble v1c[3] = {v1[0],v1[1],v1[2]};
	r[0] = v1c[1]*v2[2] - v1c[2]*v2[1];
	r[1] = v1c[2]*v2[0] - v1c[0]*v2[2];
	r[2] = v1c[0]*v2[1] - v1c[1]*v2[0];
    }
    return r;
}


GLdouble* vecadd(GLdouble* r, GLdouble* v, GLdouble *v2)
{
    r[0] = v[0] + v2[0];
    r[1] = v[1] + v2[1];
    r[2] = v[2] + v2[2];
    return r;
}

GLdouble* vecdiff(GLdouble* r, GLdouble* v, GLdouble *v2)
{
    r[0] = v[0] - v2[0];
    r[1] = v[1] - v2[1];
    r[2] = v[2] - v2[2];
    return r;
}

/* returns vector length, too */
GLdouble vecnormal(GLdouble*r, GLdouble* v)
{
    GLdouble ret = sqrt(dot(v,v));
    scalarmul(r,v,ret);
    return ret;
}

#endif













