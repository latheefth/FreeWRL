#include "LinearAlgebra.h"


#include <stdio.h>
/* Altenate implemetations available, should merge them eventually */

void veccross(struct pt *c, struct pt a, struct pt b)
{
    c->x = a.y * b.z - a.z * b.y;
    c->y = a.z * b.x - a.x * b.z;
    c->z = a.x * b.y - a.y * b.x;
}

float veclength( struct pt p )
{
    return sqrt(p.x*p.x + p.y*p.y + p.z*p.z); 
}


double vecangle(struct pt* V1, struct pt* V2) {
    return acos((V1->x*V2->x + V1->y*V2->y +V1->z*V2->z) /
		sqrt( (V1->x*V1->x + V1->y*V1->y + V1->z*V1->z)*(V2->x*V2->x + V2->y*V2->y + V2->z*V2->z) )  );
};


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

/* returns vector length, too */
GLdouble vecnormal(struct pt*r, struct pt* v)
{
    GLdouble ret = sqrt(vecdot(v,v));
    if(ret == 0.) return 0.;
    vecscale(r,v,1./ret);
    return ret;
}


/*will add functions here as needed. */

GLdouble det3x3(GLdouble* data) 
{
    return -data[1]*data[10]*data[4] +data[0]*data[10]*data[5] -data[2]*data[5]*data[8] +data[1]*data[6]*data[8] +data[2]*data[4]*data[9] -data[0]*data[6]*data[9];
}

struct pt* transform(struct pt* r, const struct pt* a, const GLdouble* b)
{
    if(r != a) { /*protect from self-assignments */
	r->x = b[0]*a->x +b[4]*a->y +b[8]*a->z +b[12];
	r->y = b[1]*a->x +b[5]*a->y +b[9]*a->z +b[13];
	r->z = b[2]*a->x +b[6]*a->y +b[10]*a->z +b[14];
    } else {
	struct pt tmp = {a->x,a->y,a->z};
	r->x = b[0]*tmp.x +b[4]*tmp.y +b[8]*tmp.z +b[12];
	r->y = b[1]*tmp.x +b[5]*tmp.y +b[9]*tmp.z +b[13];
	r->z = b[2]*tmp.x +b[6]*tmp.y +b[10]*tmp.z +b[14];
    }
    return r;
}
/*transform point, but ignores translation.*/
struct pt* transform3x3(struct pt* r, const struct pt* a, const GLdouble* b)
{
    if(r != a) { /*protect from self-assignments */
	r->x = b[0]*a->x +b[4]*a->y +b[8]*a->z;
	r->y = b[1]*a->x +b[5]*a->y +b[9]*a->z;
	r->z = b[2]*a->x +b[6]*a->y +b[10]*a->z;
    } else {
	struct pt tmp = {a->x,a->y,a->z};
	r->x = b[0]*tmp.x +b[4]*tmp.y +b[8]*tmp.z;
	r->y = b[1]*tmp.x +b[5]*tmp.y +b[9]*tmp.z;
	r->z = b[2]*tmp.x +b[6]*tmp.y +b[10]*tmp.z;
    }
    return r;
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

struct pt* vecadd(struct pt* r, struct pt* v, struct pt* v2)
{
    r->x = v->x + v2->x;
    r->y = v->y + v2->y;
    r->z = v->z + v2->z;
    return r;
}

struct pt* vecdiff(struct pt* r, struct pt* v, struct pt* v2)
{
    r->x = v->x - v2->x;
    r->y = v->y - v2->y;
    r->z = v->z - v2->z;
    return r;
}

/*i,j,n will form an orthogonal vector space */
void make_orthogonal_vector_space(struct pt* i, struct pt* j, struct pt n) {
    //optimal axis finding algorithm. the solution isn't unique. 
    // each of these three calculations doesn't work (or works poorly) 
    // in certain distinct cases. (gives zero vectors when two axes are 0)
    // selecting the calculations according to smallest axis avoids this problem.
    // (the two remaining axis are thus far from zero, if n is normal)
    if(n.x < n.y && n.x < n.z) { //x smallest
	i->x = 0;
	i->y = n.z;
	i->z = -n.y;
	vecnormal(i,i);
	j->x = n.y*n.y + n.z*n.z;
	j->y = (-n.x)*n.y;
	j->z = (-n.x)*n.z;
    } else if(n.y < n.x && n.y < n.z) { //y smallest
	i->x = -n.z;
	i->y = 0;
	i->z = n.x;
	vecnormal(i,i);
	j->x = (-n.x)*n.y;
	j->y = n.x*n.x + n.z*n.z;
	j->z = (-n.y)*n.z;
    } else { //z smallest
	i->x = n.y;
	i->y = -n.x;
	i->z = 0;
	vecnormal(i,i);
	j->x = (-n.x)*n.z;
	j->y = (-n.y)*n.z;
	j->z =  n.x*n.x + n.y*n.y;
    }
}


GLdouble* Inverse(GLdouble* res, GLdouble* m)
{
    double Deta;

    Deta = det3x3(m);

    res[0] = (-m[9]*m[6] +m[5]*m[10])/Deta;
    res[4] = (+m[8]*m[6] -m[4]*m[10])/Deta;
    res[8] = (-m[8]*m[5] +m[4]*m[9])/Deta;
    res[12] = ( m[12]*m[9]*m[6] -m[8]*m[13]*m[6] -m[12]*m[5]*m[10] +m[4]*m[13]*m[10] +m[8]*m[5]*m[14] -m[4]*m[9]*m[14])/Deta;

    res[1] = (+m[9]*m[2] -m[1]*m[10])/Deta;
    res[5] = (-m[8]*m[2] +m[0]*m[10])/Deta;
    res[9] = (+m[8]*m[1] -m[0]*m[9])/Deta;
    res[13] = (-m[12]*m[9]*m[2] +m[8]*m[13]*m[2] +m[12]*m[1]*m[10] -m[0]*m[13]*m[10] -m[8]*m[1]*m[14] +m[0]*m[9]*m[14])/Deta;

    res[2] = (-m[5]*m[2] +m[1]*m[6])/Deta;
    res[6] = (+m[4]*m[2] -m[0]*m[6])/Deta;
    res[10] = (-m[4]*m[1] +m[0]*m[5])/Deta;
    res[14] = ( m[12]*m[5]*m[2] -m[4]*m[13]*m[2] -m[12]*m[1]*m[6] +m[0]*m[13]*m[6] +m[4]*m[1]*m[14] -m[0]*m[5]*m[14])/Deta;

    res[3] = (+m[5]*m[2]*m[11] -m[1]*m[6]*m[11])/Deta;
    res[7] = (-m[4]*m[2]*m[11] +m[0]*m[6]*m[11])/Deta;
    res[11] = (+m[4]*m[1]*m[11] -m[0]*m[5]*m[11])/Deta;
    res[15] = (-m[8]*m[5]*m[2] +m[4]*m[9]*m[2] +m[8]*m[1]*m[6] -m[0]*m[9]*m[6] -m[4]*m[1]*m[10] +m[0]*m[5]*m[10])/Deta;

    return res;
}

struct pt* polynormal(struct pt* r, struct pt* p1, struct pt* p2, struct pt* p3) {
    struct pt v1;
    struct pt v2;
    VECDIFF(*p2,*p1,v1);
    VECDIFF(*p3,*p1,v2);
    veccross(r,v1,v2);
    vecnormal(r,r);
    return r;
}

#ifdef COMMENT
/*fast crossproduct using references, that checks for auto-assignments */
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



#endif













