#include "Collision.h"


#include <stdio.h>
#include <math.h>
#include "LinearAlgebra.h"
#include <malloc.h>

#define swap(x,y) {double k = x; x = y; y = k; }
#define FLOAT_TOLERANCE 0.00000001
#define MAX_POLYREP_DISP_RECURSION_COUNT 10

#ifdef DEBUGPTS
#define DEBUGPTSPRINT(x,y,z) printf(x,y,z)
#else
#define DEBUGPTSPRINT(x,y,z) {}
#endif

double closest_point_of_segment_to_y_axis(struct pt p1, struct pt p2) {
    /*the equation */
    double x12 = (p1.x - p2.x);
    double z12 = (p1.z - p2.z);
    double q = ( x12*x12 + z12*z12 );
    double i = ((q == 0.) ? 0.5 : (p1.x * x12 + p1.z * z12) / q);
    struct pt result;

     /*clamp result to constraints */
    if(i < 0) i = 0.;
    if(i > 1) i = 1.;
     
    return i;
     
}


/* [p1,p2[ is segment,  q1,q2 defines line */
/* ignores y coord. eg intersection is done on projection of segment and line on the y plane */
/* nowtice point p2 is NOT included, (for simplification elsewhere) */
int intersect_segment_with_line_on_yplane(struct pt* pk, struct pt p1, struct pt p2, struct pt q1, struct pt q2) {
    double k,l,quotient;

    /* p2 becomes offset */
    VECDIFF(p2,p1,p2);
    /* q2 becomes offset */
    VECDIFF(q2,q1,q2);
     
    quotient = ((-p2.z)*q2.x + p2.x*q2.z);
    if(quotient == 0.) return 0;

    k = (p1.z*q2.x - q1.z*q2.x - p1.x*q2.z + q1.x*q2.z)/quotient; 
    l = (p1.z*p2.x - p1.x*p2.z + p2.z*q1.x - p2.x*q1.z)/quotient;

    if((k >= 0.) && (k < 1.)) {
	vecscale(pk,&p2,k);
	VECADD(*pk,p1);
	return 1;
    } else
	return 0;
     
}

/*projects a point on the surface of the cylinder, in the direction of n. 
  returns TRUE if exists.
   */
int project_on_cylindersurface(struct pt* res, struct pt p, struct pt n,double r) {
    double k1,k2;
    switch(getk_intersect_line_with_ycylinder(&k1,&k2,r,p,n)) {
    case 0:
	return 0;
    case 1:
    case 2:
	vecscale(res,&n,k1);
	VECADD(*res,p);
	return 1;
    }
    return 0;
}


/*projects a point on the y="y" plane, in the direction of n. *
  n probably needs to be normal. */
struct pt project_on_yplane(struct pt p1, struct pt n,double y) {
    struct pt ret = {p1.x - (n.x*(p1.y-y))/n.y,y,(p1.z - (n.z*(p1.y-y))/n.y)};
    return ret;
}

/*projects a point on the plane tangent to the surface of the cylinder at point kn (the prolonged normal) 
  , in the direction of n. 
  n probably needs to be normal. */
struct pt project_on_cylindersurface_plane(struct pt p, struct pt n,double r) {
    struct pt pp = n;
    struct pt ret;
    pp.y = 0;
    vecnormal(&pp,&pp);
    vecscale(&pp,&pp,r);

    ret.x = p.x + (n.x*((n.x*((pp.x - p.x)) + n.z*((pp.z - p.z)))))/(n.x*n.x + n.z*n.z);
    ret.y = p.y + (n.y*((n.x*((pp.x - p.x)) + n.z*((pp.z - p.z)))))/(n.x*n.x + n.z*n.z);
    ret.z = p.z + (n.z*((n.x*((pp.x - p.x)) + n.z*((pp.z - p.z)))))/(n.x*n.x + n.z*n.z);

    return ret;
}

/*finds the intersection of the segment(pp1,pp2) with a cylinder on the y axis.
  returns the 0,1 or 2 values in the range [0..1]
 */
int getk_intersect_segment_with_ycylinder(double* k1, double* k2, double r, struct pt pp1, struct pt pp2) {
    double b,a,sqrdelta,delta;
    int res = 0;

    /* pp2 becomes offset */
    VECDIFF(pp2,pp1,pp2);
     
    /*solves (pp1+ k pp2) . (pp1 + k pp2) = r^2 */
    a = 2*(pp2.x*pp2.x + pp2.z*pp2.z);
    b = -2*(pp1.x*pp2.x + pp1.z*pp2.z);
    delta = (4*((pp1.x*pp2.x + pp1.z*pp2.z)*(pp1.x*pp2.x + pp1.z*pp2.z)) - 
	     4*((pp2.x*pp2.x + pp2.z*pp2.z))*((pp1.x*pp1.x + pp1.z*pp1.z - r*r)));
    if(delta < 0.) return 0;
    sqrdelta = sqrt(delta);
    /* keep the ks that are in the segment */
    *k1 = (b+sqrdelta)/a;
    *k2 = (b-sqrdelta)/a;

    if(*k1 >= 0. && *k1 <= 1.) res++;
    if(*k2 >= 0. && *k2 <= 1.) res++;
    if(res == 1 && (*k1 < 0. || *k1 > 1.)) swap(*k1,*k2);
    if(res == 2 && sqrdelta == 0.) res = 1;

    return res;
}

struct pt weighted_sum(struct pt p1, struct pt p2, double k) {
    struct pt ret = {p1.x*(1-k)+p2.x*k, 
		     p1.y*(1-k)+p2.y*k, 
		     p1.z*(1-k)+p2.z*k};
    return ret;
}

/*finds the intersection of the line pp1 + k n with a cylinder on the y axis.
  returns the 0,1 or 2 values.
 */
int getk_intersect_line_with_ycylinder(double* k1, double* k2, double r, struct pt pp1, struct pt n) {
    double b,a,sqrdelta,delta;
    int res = 0;

    /*solves (pp1+ k n) . (pp1 + k n) = r^2 */
    a = 2*(n.x*n.x + n.z*n.z);
/*    if(a == 0.) {
	//degenerate case.
	//it fits our objective to simply specify a random normal.
	n.x = 1;
	n.y = 0;
	n.z = 0;
	a = 2;
	}*/
    b = -2*(pp1.x*n.x + pp1.z*n.z);
    delta = (4*((pp1.x*n.x + pp1.z*n.z)*(pp1.x*n.x + pp1.z*n.z)) - 
	     4*((n.x*n.x + n.z*n.z))*((pp1.x*pp1.x + pp1.z*pp1.z - r*r)));
    if(delta < 0.) return 0;
    sqrdelta = sqrt(delta);

    *k1 = (b+sqrdelta)/a;
    if(sqrdelta == 0.) return 1;

    *k2 = (b-sqrdelta)/a;
    return 2;
}



/*used by get_poly_normal_disp to clip the polygon on the cylinder caps, called twice*/
int helper_poly_clip_cap(struct pt* clippedpoly, int clippedpolynum, const struct pt* p, int num, double r, struct pt n, double y)
{
    struct pt* ppoly;
    int allin = 1;
    int i;

    ppoly = (struct pt*) malloc(sizeof(struct pt) * num);

 
    /*sqush poly on cylinder cap plane.*/
    for(i= 0; i < num; i++) {
	ppoly[i] = project_on_yplane(p[i],n,y);
    }

    /*find points of poly hitting cylinder cap*/
    for(i= 0; i < num; i++) {
	if(ppoly[i].x*ppoly[i].x + ppoly[i].z*ppoly[i].z > r*r) {
	    allin = 0;
	} else {
	    DEBUGPTSPRINT("intersect_point_cap(%f)= %d\n",y,clippedpolynum);
	    clippedpoly[clippedpolynum++] = ppoly[i]; 
	}
    }

    if(!allin) {
	static const struct pt zero = {0,0,0};
	int numdessect = 0;
	struct pt dessect[2];
	double k1,k2;
	int nsect;


	/*find intersections of poly with cylinder cap edge*/
	for(i=0; i <num; i++) {
	    nsect = getk_intersect_segment_with_ycylinder(&k1,&k2,r,ppoly[i],ppoly[(i+1)%num]);
	    switch(nsect) {
	    case 2:
		DEBUGPTSPRINT("intersect_segment_cap(%f)_2= %d\n",y,clippedpolynum);
		clippedpoly[clippedpolynum++] = weighted_sum(ppoly[i],ppoly[(i+1)%num],k2);
	    case 1: 
		DEBUGPTSPRINT("intersect_segment_cap(%f)_1= %d\n",y,clippedpolynum);
		clippedpoly[clippedpolynum++] = weighted_sum(ppoly[i],ppoly[(i+1)%num],k1);
	    case 0: break;
	    }
	    /*find points of poly intersecting descending line on poly*/
	    if((numdessect != 2) && intersect_segment_with_line_on_yplane(&dessect[numdessect],ppoly[i],ppoly[(i+1)%num],zero,n)) {
		numdessect++;
	    }
	}
	/*find intersections of descending segment too.
	  these will point out maximum and minimum in cylinder cap edge that is inside triangle */
	if(numdessect == 2) {
	    nsect = getk_intersect_segment_with_ycylinder(&k1,&k2,r,dessect[0],dessect[1]);
	    switch(nsect) {
	    case 2:
		DEBUGPTSPRINT("intersect_descending_segment_cap(%f)_2= %d\n",y,clippedpolynum);
		clippedpoly[clippedpolynum++] = weighted_sum(dessect[0],dessect[1],k2);
	    case 1: 
		DEBUGPTSPRINT("intersect_descending_segment_cap(%f)_1= %d\n",y,clippedpolynum);
		clippedpoly[clippedpolynum++] = weighted_sum(dessect[0],dessect[1],k1);
	    case 0: break;
	    }
	} 
    }

    free(ppoly);

    return clippedpolynum;
}


//yes, global. for speed optimizations.

double maxdisp;

struct pt get_poly_normal_disp(double y1, double y2, double r, struct pt* p, int num, struct pt n) {
    int i;
    double polydisp;
    struct pt result;

    struct pt* clippedpoly;
    int clippedpolynum = 0;
    static const struct pt zero = {0,0,0};

    maxdisp = -1E90;
    
#ifdef DEBUGFACEMASK
    printf("facemask = %d, debugsurface = %d\n",facemask,debugsurface);
    if((facemask & (1 <<debugsurface++)) ) return zero;
#endif

    /*allocate data */
    clippedpoly = (struct pt*) malloc(sizeof(struct pt) * (num*5 + 4));

    /*if normal not specified, calculate it */
    if(n.x == 0 && n.y == 0 && n.z == 0) {
	struct pt v1,v2;
	/*get normal to poly*/
	VECDIFF(p[0],p[1],v1);
	VECDIFF(p[0],p[2],v2);
	veccross(&n,v1,v2);

	vecnormal(&n,&n); 
    }

    for(i = 0; i < num; i++) {
	if(project_on_cylindersurface(&clippedpoly[clippedpolynum],weighted_sum(p[i],p[(i+1)%num],closest_point_of_segment_to_y_axis(p[i],p[(i+1)%num])),n,r) &&
	   clippedpoly[clippedpolynum].y < y2 && 
	   clippedpoly[clippedpolynum].y > y1 ) {

	    clippedpolynum++;
	}
    }

    /* clip polygon on top and bottom cap */
    if(n.y!= 0.) {
	clippedpolynum = helper_poly_clip_cap(clippedpoly, clippedpolynum, p, num, r, n, y1);
	clippedpolynum = helper_poly_clip_cap(clippedpoly, clippedpolynum, p, num, r, n, y2);
    }

    /*find intersections of poly with cylinder side*/
    if(n.y != 1. && n.y != -1.) { /*n.y == +/-1 makes n.x == n.z == 0, wich does div's by 0, besides making no sense at all. */
	
	int numdessect3d = 0;
	struct pt dessect3d[2];
	double k1,k2;
	int nsect;


	for(i=0; i <num; i++) {
	    /*find points of poly intersecting descending line on poly, (non-projected)*/
	    if((numdessect3d != 2) && intersect_segment_with_line_on_yplane(&dessect3d[numdessect3d],p[i],p[(i+1)%num],zero,n)) {
		numdessect3d++;
	    }
	}

	if(numdessect3d == 2) {
	    dessect3d[0] = project_on_cylindersurface_plane(dessect3d[0],n,r);
	    dessect3d[1] = project_on_cylindersurface_plane(dessect3d[1],n,r);

	    /*only do/correct points if dessect3d line is somewhere inside the cylinder */
	    if((dessect3d[0].y <= y2 || dessect3d[1].y <= y2) && (dessect3d[0].y >= y1 || dessect3d[1].y >= y1)) {
		if(dessect3d[0].y > y2) dessect3d[0].y = y2;
		if(dessect3d[0].y < y1) dessect3d[0].y = y1;
		if(dessect3d[1].y > y2) dessect3d[1].y = y2;
		if(dessect3d[1].y < y1) dessect3d[1].y = y1;

		clippedpoly[clippedpolynum++] = dessect3d[0];
		clippedpoly[clippedpolynum++] = dessect3d[1];
	    }

	} 
	{ /*find intersections on cylinder of polygon points projected on surface */
	    struct pt sect;
	    for(i = 0; i < num; i++) {
		nsect = getk_intersect_line_with_ycylinder(&k1, &k2, r, p[i], n);
		if(nsect == 0) continue;

		/*sect = p[i] + k1 n*/
		vecscale(&sect,&n,k1);
		VECADD(sect,p[i]);
		    
		if(sect.y > y1 && sect.y < y2) {
		    clippedpoly[clippedpolynum++] = sect;
		}
	    } 
	}

    }

#ifdef DEBUGPTS
    for(i=0; i < clippedpolynum; i++) {
	debugpts.push_back(clippedpoly[i]);
    }
#endif

    /*here we find mimimum displacement possible */
    polydisp = vecdot(&p[0],&n);

    /*calculate farthest point from the "n" plane passing through the origin */
    for(i = 0; i < clippedpolynum; i++) {
	double disp = vecdot(&clippedpoly[i],&n) - polydisp;
	if(disp > maxdisp) maxdisp = disp;
    }
    if(maxdisp >= 0.) {
	vecscale(&result,&n,maxdisp);
    } else
	result = zero;

    /*free alloc'd data */
    free(clippedpoly);

    return result;
}
  
/*used by get_line_normal_disp to clip the polygon on the cylinder caps, called twice*/
int helper_line_clip_cap(struct pt* clippedpoly, int clippedpolynum, struct pt p1, struct pt p2, double r, struct pt n, double y)
{
    struct pt ppoly[2];
    int allin = 1;
    int i;

    /*sqush poly on cylinder cap plane.*/
    ppoly[0] = project_on_yplane(p1,n,y);
    ppoly[1] = project_on_yplane(p2,n,y);

    /*find points of poly hitting cylinder cap*/
    for(i= 0; i < 2; i++) {
	if(ppoly[i].x*ppoly[i].x + ppoly[i].z*ppoly[i].z > r*r) {
	    allin = 0;
	} else {
	    clippedpoly[clippedpolynum++] = ppoly[i]; 
	}
    }

    if(!allin) {
	static const struct pt zero = {0,0,0};
	int numdessect = 0;
	struct pt dessect;
	double k1,k2;
	int nsect;


	/*find intersections of line with cylinder cap edge*/
	nsect = getk_intersect_segment_with_ycylinder(&k1,&k2,r,ppoly[0],ppoly[1]);
	switch(nsect) {
	case 2:
	    clippedpoly[clippedpolynum++] = weighted_sum(ppoly[0],ppoly[1],k2);
	case 1: 
	    clippedpoly[clippedpolynum++] = weighted_sum(ppoly[0],ppoly[1],k1);
	case 0: break;
	}
	/*find intersections of descending segment too.
	  these will point out maximum and minimum in cylinder cap edge that is inside triangle */
	if(intersect_segment_with_line_on_yplane(&dessect,ppoly[0],ppoly[1],zero,n)) {
	    if(dessect.x*dessect.x + dessect.z*dessect.z < r*r) {
		    
		clippedpoly[clippedpolynum++] = dessect;
	    }
	} /*else { //else one last case not treated above 
	    if(n.y == -1. || n.y == 1.) {
		struct pt c = {0,y,0};
		clippedpoly[clippedpolynum++] = c;
	    }
	       
	}*/
    }

    return clippedpolynum;
}

struct pt get_line_normal_disp(double y1, double y2, double r, struct pt p1, struct pt p2, struct pt n) {
    int i;
    double maxdisp = 0;
    double polydisp;
    struct pt p[2] = {p1,p2};
    int num = 2;
    struct pt result;

    struct pt clippedpoly[14];
    int clippedpolynum = 0;
    static const struct pt zero = {0,0,0};

    /*project closest point on cylindersurface ???needed???*/
/*     if(project_on_cylindersurface(&clippedpoly[clippedpolynum],weighted_sum(p1,p2,closest_point_of_segment_to_y_axis(p1,p2)),n,r) &&
       clippedpoly[clippedpolynum].y < y2 && 
       clippedpoly[clippedpolynum].y > y1 ) {

       clippedpolynum++;
       }*/

    /* clip line on top and bottom cap */
    if(n.y!= 0.) {
	clippedpolynum = helper_line_clip_cap(clippedpoly, clippedpolynum, p1, p2, r, n, y1);
	clippedpolynum = helper_line_clip_cap(clippedpoly, clippedpolynum, p1, p2, r, n, y2);
    }

    /*find intersections of line with cylinder side*/
    if(n.y != 1. && n.y != -1.) { /*n.y == +/-1 makes n.x == n.z == 0, wich does div's by 0, besides making no sense at all. */
	
	struct pt dessect3d;
	double k1,k2;
	int nsect;


	/*find points of poly intersecting descending line on poly, (non-projected)*/
	if(intersect_segment_with_line_on_yplane(&dessect3d,p[0],p[1],zero,n)) {
	    dessect3d = project_on_cylindersurface_plane(dessect3d,n,r);

	    if(dessect3d.y < y2 && 
	       dessect3d.y > y1)
		clippedpoly[clippedpolynum++] = dessect3d;

	} 
	{ /*find intersections on cylinder of polygon points projected on surface */
	    struct pt sect;
	    for(i = 0; i < num; i++) {
		nsect = getk_intersect_line_with_ycylinder(&k1, &k2, r, p[i], n);
		if(nsect == 0) continue;

		/*sect = p[i] + k1 n*/
		vecscale(&sect,&n,k1);
		VECADD(sect,p[i]);
		    
		if(sect.y > y1 && sect.y < y2) {
		    clippedpoly[clippedpolynum++] = sect;
		}
	    } 
	}

    }

#ifdef DEBUGPTS_COMMENT
    for(i=0; i < clippedpolynum; i++) {
	debugpts.push_back(clippedpoly[i]);
    }
#endif

    /*here we find mimimum displacement possible */
    polydisp = vecdot(&p[0],&n);

    /*calculate farthest point from the "n" plane passing through the origin */
    for(i = 0; i < clippedpolynum; i++) {
	double disp = vecdot(&clippedpoly[i],&n) - polydisp;
	if(disp > maxdisp) maxdisp = disp;
    }
    vecscale(&result,&n,maxdisp);

    return result;
}
  

struct pt get_point_normal_disp(double y1, double y2, double r, struct pt p1, struct pt n) {
    double y;
    struct pt result;
    struct pt cp;
    static const struct pt zero = {0,0,0};

    /*select relevant cap*/
    y = (n.y > 0.) ? y2 : y1;

    /*check if intersect cap*/
    if(n.y != 0) {
	cp = project_on_yplane(p1,n,y);
	if(cp.x*cp.x + cp.z*cp.z <= r*r) {
	    VECDIFF(cp,p1,result);
	    return result;
	}
    }

    /*find intersections of point with cylinder side*/
    if(n.y != 1. && n.y != -1.) { /*n.y == +/-1 makes n.x == n.z == 0, wich does div's by 0, besides making no sense at all. */
	int nsect;
	double k1,k2;
	/*find pos of the point projected on surface of cylinder*/
	nsect = getk_intersect_line_with_ycylinder(&k1, &k2, r, p1, n);
	if(nsect != 0) {
	    /*sect = p1 + k1 n*/
	    vecscale(&result,&n,k1);
	    cp = result;
	    VECADD(cp,p1);

	    if(cp.y > y1 && cp.y < y2) {
		return result;
	    }
	}
	 
    }

    return zero;
}
  

struct pt box_disp(double y1, double y2, double r,struct pt p0, struct pt i, struct pt j, struct pt k) {
    struct pt p[8] = {p0,p0,p0,p0,p0,p0,p0,p0};
    struct pt n[6];
    struct pt mindispv = {0,0,0};
    double mindisp = 1E99;
    struct pt middle;
    /*draw this up, you will understand: */
    static const int faces[6][4] = {
	{1,7,2,0},
	{2,6,3,0},
	{3,5,1,0},
	{5,3,6,4},
	{7,1,5,4},
	{6,2,7,4}
    };
    int ci;

    /*compute points of box*/
    VECADD(p[1],i);
    VECADD(p[2],j);
    VECADD(p[3],k);
    VECADD(p[4],i); VECADD(p[4],j); VECADD(p[4],k); //p[4]= i+j+k
    VECADD(p[5],k); VECADD(p[5],i); //p[6]= k+i
    VECADD(p[6],j); VECADD(p[6],k); //p[5]= j+k
    VECADD(p[7],i); VECADD(p[7],j); //p[7]= i+j
     
    /*compute normals, in case of perfectly orthogonal box, a shortcut exists*/
    veccross(&n[3],j,i);
    veccross(&n[4],k,j);
    veccross(&n[5],i,k);
    vecnormal(&n[3],&n[3]);
    vecnormal(&n[4],&n[4]);
    vecnormal(&n[5],&n[5]);
    vecscale(&n[0],&n[3],-1.);
    vecscale(&n[1],&n[4],-1.);
    vecscale(&n[2],&n[5],-1.);

    /*what it says : middle of box */
    middle = weighted_sum(p[0],p[4],.5);

    for(ci = 0; ci < 6; ci++) {
	/*only clip faces "facing" origin */
	if(vecdot(&n[ci],&middle) > 0.) {
	    struct pt pts[4] = { p[faces[ci][0]],
				 p[faces[ci][1]],
				 p[faces[ci][2]],
				 p[faces[ci][3]] };
	    struct pt dispv = get_poly_normal_disp(y1,y2,r,pts,4,n[ci]);
	    double disp = vecdot(&dispv,&dispv);
	       
	    /*get minimal displacement*/
	    if(disp < mindisp) {
		mindisp = disp;
		mindispv = dispv;
	    }
	       
	}
    }
     
    return mindispv;
     
}

/*gives false positives. */
int fast_ycylinder_box_intersect(double y1, double y2, double r,struct pt pcenter, double xs, double ys, double zs) {
    double y = pcenter.y < 0 ? y1 : y2;
     
    double lefteq = sqrt(y*y + r*r) + sqrt(xs*xs + ys*ys + zs*zs);
    return lefteq*lefteq > vecdot(&pcenter,&pcenter);
}


/*gives false positives. */
int fast_ycylinder_cone_intersect(double y1, double y2, double r,struct pt pcenter, double halfheight, double baseradius) {
    double y = pcenter.y < 0 ? y1 : y2;
     
    double lefteq = sqrt(y*y + r*r) + sqrt(halfheight*halfheight + baseradius*baseradius);
    return lefteq*lefteq > vecdot(&pcenter,&pcenter);
}



/*algorithm is approximative */
/*basically, it does collision with a triangle on a plane that passes through the origin.*/
struct pt cone_disp(double y1, double y2, double r, struct pt base, struct pt top, double baseradius) {

    struct pt i; //cone axis vector
    double h; //height of cone
    struct pt tmp;
    struct pt bn; //direction from cone to cylinder
    struct pt side; //side of base in direction of origin
    struct pt normalbase; //collision normal of base (points upwards)
    struct pt normalside; //collision normal of side (points inside)
    struct pt normaltop; //collision normal of top (points down)
    struct pt mindispv= {0,0,0};
    double mindisp = 1E99;

    /*find closest point of cone base to origin. */

    vecscale(&bn,&base,-1);

    VECDIFF(top,base,i);
    vecscale(&tmp,&i,- vecdot(&i,&bn)/vecdot(&i,&i));
    VECADD(bn,tmp);
    if(vecnormal(&bn,&bn) == 0.) {
	/* origin is aligned with cone axis */
	/* must use different algorithm to find side */
	struct pt tmpn = i;
	struct pt tmpj;
	vecnormal(&tmpn,&tmpn);
	make_orthogonal_vector_space(&bn,&tmpj,tmpn);
    }
    vecscale(&side,&bn,baseradius);
    VECADD(side,base);

    //find normals ;
    h = vecnormal(&i,&i);
    normalbase = i;
    vecscale(&normaltop,&normalbase,-1);
    vecscale(&i,&i,-baseradius);
    vecscale(&normalside,&bn,-h);
    VECADD(normalside,i);
    vecnormal(&normalside,&normalside);

    {
	/*get minimal displacement*/
	struct pt dispv;
	double disp;

	dispv = get_line_normal_disp(y1,y2,r,top,side,normalside);
	disp = vecdot(&dispv,&dispv);
	if(disp < mindisp)
	    mindispv = dispv, mindisp = disp;

	dispv = get_line_normal_disp(y1,y2,r,base,side,normalbase);
	disp = vecdot(&dispv,&dispv);
	if(disp < mindisp)
	    mindispv = dispv, mindisp = disp;

	dispv = get_point_normal_disp(y1,y2,r,top,normaltop);
	disp = vecdot(&dispv,&dispv);
	/*i don't like "disp !=0." there should be a different condition for
	 * non applicability.*/
	if(disp != 0. && disp < mindisp) 
	    mindispv = dispv, mindisp = disp;
    }

    return mindispv;
}


/*algorithm is approximative */
/*basically, it does collision with a rectangle on a plane that passes through the origin.*/
struct pt cylinder_disp(double y1, double y2, double r, struct pt base, struct pt top, double baseradius) {

    struct pt i; //cone axis vector
    double h; //height of cone
    struct pt tmp;
    struct pt bn; //direction from cone to cylinder
    struct pt sidetop; //side of top in direction of origin
    struct pt sidebase; //side of base in direction of origin
    struct pt normalbase; //collision normal of base (points upwards)
    struct pt normalside; //collision normal of side (points inside)
    struct pt normaltop; //collision normal of top (points down)
    struct pt mindispv= {0,0,0};
    double mindisp = 1E99;

    /*find closest point of cone base to origin. */

    vecscale(&bn,&base,-1);

    VECDIFF(top,base,i);
    vecscale(&tmp,&i,- vecdot(&i,&bn)/vecdot(&i,&i));
    VECADD(bn,tmp);
    if(vecnormal(&bn,&bn) == 0.) {
	/* origin is aligned with cone axis */
	/* must use different algorithm to find side */
	struct pt tmpn = i;
	struct pt tmpj;
	vecnormal(&tmpn,&tmpn);
	make_orthogonal_vector_space(&bn,&tmpj,tmpn);
    }
    vecscale(&sidebase,&bn,baseradius);
    sidetop = top; VECADD(sidetop,sidebase)
		       VECADD(sidebase,base);

    //find normals ;
    h = vecnormal(&i,&i);
    normalbase = i;
    vecscale(&normaltop,&normalbase,-1);
    vecscale(&normalside,&bn,-1);

    {
	/*get minimal displacement*/
	struct pt dispv;
	double disp;

	dispv = get_line_normal_disp(y1,y2,r,sidetop,sidebase,normalside);
//	  printf("dispv_side (%f,%f,%f)\n",dispv.x,dispv.y,dispv.z);
	disp = vecdot(&dispv,&dispv);
	if(disp < mindisp)
	    mindispv = dispv, mindisp = disp;

	dispv = get_line_normal_disp(y1,y2,r,base,sidebase,normalbase);
//	  printf("dispv_base (%f,%f,%f)\n",dispv.x,dispv.y,dispv.z);
	disp = vecdot(&dispv,&dispv);
	if(disp < mindisp)
	    mindispv = dispv, mindisp = disp;

	dispv = get_line_normal_disp(y1,y2,r,top,sidetop,normaltop);
//	  printf("dispv_top (%f,%f,%f)\n",dispv.x,dispv.y,dispv.z);
	disp = vecdot(&dispv,&dispv);
	if( disp < mindisp) 
	    mindispv = dispv, mindisp = disp;
    }

    return mindispv;
}


struct pt polyrep_disp_rec(double y1, double y2, double r, struct VRML_PolyRep* pr, struct pt* n, struct pt dispsum) {
    struct pt p[3];
    double mindisp = 1E99;
    struct pt mindispv = {0,0,0};
    double disp;
    struct pt dispv;
    static int recursion_count = 0;
    int nextrec = 0;
    int i;

    for(i = 0; i < pr->ntri; i++) {
	p[0].x = pr->coord[pr->cindex[i*3]*3]    +dispsum.x;
	p[0].y = pr->coord[pr->cindex[i*3]*3+1]  +dispsum.y;
	p[0].z = pr->coord[pr->cindex[i*3]*3+2]  +dispsum.z;

	if(vecdot(&n[i],&p[0]) > 0) {
	    p[1].x = pr->coord[pr->cindex[i*3+1]*3]    +dispsum.x;
	    p[1].y = pr->coord[pr->cindex[i*3+1]*3+1]  +dispsum.y;
	    p[1].z = pr->coord[pr->cindex[i*3+1]*3+2]  +dispsum.z;
	    p[2].x = pr->coord[pr->cindex[i*3+2]*3]    +dispsum.x;
	    p[2].y = pr->coord[pr->cindex[i*3+2]*3+1]  +dispsum.y;
	    p[2].z = pr->coord[pr->cindex[i*3+2]*3+2]  +dispsum.z;
	    
	    dispv = get_poly_normal_disp(y1,y2,r, p, 3, n[i]);
	    disp = maxdisp; /*global variable. was calculated inside poly_normal_disp already. */

#ifdef DEBUGPTS
    printf("polyd: (%f,%f,%f) |%f|\n",dispv.x,dispv.y,dispv.z,disp);
#endif
	    
	    
	    if((disp > FLOAT_TOLERANCE) && (disp < mindisp)) {
		mindisp = disp;
		mindispv = dispv;
		nextrec = 1;
	    }
	}
	
    }
#ifdef DEBUGPTS
    printf("adding correction: (%f,%f,%f) |%f|\n",mindispv.x,mindispv.y,mindispv.z,mindisp);
#endif
    VECADD(dispsum,mindispv);
    if(nextrec && mindisp > FLOAT_TOLERANCE && recursion_count++ < MAX_POLYREP_DISP_RECURSION_COUNT) {
	return polyrep_disp_rec(y1, y2, r, pr, n, dispsum);
    } else /*end condition satisfied */
    {
#ifdef DEBUGPTS
	printf("recursion_count = %d\n",recursion_count);
#endif
	recursion_count = 0;
	return dispsum;
    }

}


#ifdef DEBUGPTS
void printpolyrep(struct VRML_PolyRep pr, int npoints) {
    int i;
    printf("VRML_PolyRep makepolyrep() {\n");
    printf("static int cindex[%d];\nstatic float coord[%d];\n",pr.ntri*3,npoints*3);
    printf(" int cindext[%d] = {",pr.ntri*3);
    for(i=0; i < pr.ntri*3-1; i++)
	printf("%d,",pr.cindex[i]);
    printf("%d};\n",pr.cindex[i]);

    printf(" float coordt[%d] = {",npoints*3);
    for(i=0; i < npoints*3-1; i++)
	printf("%f,",pr.coord[i]);
    printf("%f};\n",pr.coord[i]);
    
    printf("VRML_PolyRep pr = {0,%d,%d,cindex,coord,NULL,NULL,NULL,NULL,NULL,NULL};\n",pr.ntri,pr.alloc_tri);
    printf("memcpy(cindex,cindext,sizeof(cindex));\n");
    printf("memcpy(coord,coordt,sizeof(coord));\n");
    printf("return pr; }\n");
    
};

void printmatrix(GLdouble* mat) {
    int i;
    printf("void getmatrix(GLdouble* mat) {\n");
    for(i = 0; i< 16; i++) {
	printf("mat[%d] = %f;\n",i,mat[i]);
    }
    printf("}\n");

}
#endif

struct pt polyrep_disp(double y1, double y2, double r, struct VRML_PolyRep pr, GLdouble* mat) {
    float* newc;
    struct pt* normals; 
    struct pt res ={0,0,0};
    int i;
    

    /*transform all points to viewer space */
    newc = (float*)malloc(pr.ntri*9*sizeof(float));
    for(i = 0; i < pr.ntri*3; i++) {
	/*an example why NOT to have different working and storing data sizes */
	/*I shoudl write a specialized transform function*/
	struct pt p1 = {pr.coord[pr.cindex[i]*3],
			pr.coord[pr.cindex[i]*3+1],
			pr.coord[pr.cindex[i]*3+2]};
	struct pt r;
	transform(&r,&p1,mat);
	newc[pr.cindex[i]*3] = r.x;
	newc[pr.cindex[i]*3+1] = r.y;
	newc[pr.cindex[i]*3+2] = r.z;
    }
    pr.coord = newc; /*remember, coords are only replaced in our local copy of PolyRep */

    /*pre-calculate face normals */
    normals = (struct pt*)malloc(pr.ntri*sizeof(struct pt));
    for(i = 0; i < pr.ntri; i++) {
	/*yeah, I know. should write specialized funcion instead.*/
	struct pt p1 = {pr.coord[pr.cindex[i*3]*3],
			pr.coord[pr.cindex[i*3]*3+1],
			pr.coord[pr.cindex[i*3]*3+2]};
	struct pt p3 = {pr.coord[pr.cindex[i*3+1]*3],
			pr.coord[pr.cindex[i*3+1]*3+1],
			pr.coord[pr.cindex[i*3+1]*3+2]};
	struct pt p2 = {pr.coord[pr.cindex[i*3+2]*3],
			pr.coord[pr.cindex[i*3+2]*3+1],
			pr.coord[pr.cindex[i*3+2]*3+2]};
	polynormal(&normals[i],&p1,&p2,&p3);
    }
    
    
    res = polyrep_disp_rec(y1,y2,r,&pr,normals,res);

    
    /*free! */
    free(newc);
    free(normals);
    
    return res;
    
}




