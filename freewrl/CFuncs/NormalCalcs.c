/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "Structs.h"
#include "headers.h"

extern int smooth_normals;


/* Assuming that norindexes set and that cindex is set */
void calc_poly_normals_flat(struct VRML_PolyRep *rep) 
{
	int i;
	float a[3],b[3], *v1,*v2,*v3;
	for(i=0; i<rep->ntri; i++) {
		v1 = rep->coord+3*rep->cindex[i*3+0];
		v2 = rep->coord+3*rep->cindex[i*3+1];
		v3 = rep->coord+3*rep->cindex[i*3+2];
		

		printf ("cpnf %d using cindex %d %d %d\n",
		i,rep->cindex[i*3],rep->cindex[i*3+1],rep->cindex[i*3+2]);
		printf ("	v1 %f %f %f\n",v1[0],v1[1],v1[2]);
		printf ("	v2 %f %f %f\n",v2[0],v2[1],v2[2]);
		printf ("	v3 %f %f %f\n",v3[0],v3[1],v3[2]);

		a[0] = v2[0]-v1[0];
		a[1] = v2[1]-v1[1];
		a[2] = v2[2]-v1[2];
		b[0] = v3[0]-v1[0];
		b[1] = v3[1]-v1[1];
		b[2] = v3[2]-v1[2];
		rep->normal[i*3+0] =
			a[1]*b[2] - b[1]*a[2];
		rep->normal[i*3+1] =
			-(a[0]*b[2] - b[0]*a[2]);
		rep->normal[i*3+2] =
			a[0]*b[1] - b[0]*a[1];
	}
}




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



float calc_vector_scalar_product(struct pt a, struct pt b)
{
	return (a.x*b.x) + (a.y*b.y) + (a.z*b.z);
}



float calc_angle_between_two_vectors(struct pt a, struct pt b)
{
	float length_a, length_b, scalar, temp;
	scalar = calc_vector_scalar_product(a,b);
	length_a = calc_vector_length(a);
	length_b = calc_vector_length(b);

	/*printf("scalar: %f  length_a: %f  length_b: %f \n", scalar, length_a, length_b);*/
	
	if (scalar == 0){
		return PI/2;	
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

void fwnorprint (float *norm) {
		printf ("normals %f %f %f\n",norm[0],norm[1],norm[2]);
}

void normalize_ifs_face (float *point_normal,	
			 struct pt *facenormals,	
			 int *pointfaces,		
			int mypoint,
			int curpoly,
			float creaseAngle) {

	/* IndexedFaceSet (and possibly sometime, others)
	   normal generator 

	  Passed in:
		point_normal	- where to put the calculated normal
		facenormals	- normals of each face of a polygon
		pointfaces	- each point - which face(s) is it part of
		mypoint		- which point are we looking at
		curpoly		- which poly (face) we are working on
		creaseAngle	- creaseAngle of polygon
	*/
	int tmp_a;
	int tmp_b; 
	int facecount;
	float zz;
	struct pt temp;

	point_normal[0] = 0.0; point_normal[1] = 0.0; point_normal[2] = 0.0;

	/*
	printf ("normface poly %d point %d\n ",curpoly,mypoint);
	printf ("this point is in %d faces, these are: ", pointfaces[mypoint*POINT_FACES]);

	for (tmp_b=0; tmp_b<pointfaces[mypoint*POINT_FACES]; tmp_b++) {
		printf ("%d ",pointfaces[mypoint*POINT_FACES+tmp_b+1]);
	}
	printf ("\n");
	printf ("my normal is %f %f %f\n", facenormals[curpoly].x,
		facenormals[curpoly].y,facenormals[curpoly].z);
	*/

	/* short cut for a point in only 1 face */
	if (pointfaces[mypoint*POINT_FACES] == 1) {
		point_normal[0]=facenormals[curpoly].x;
		point_normal[1]=facenormals[curpoly].y;
		point_normal[2]=facenormals[curpoly].z;
		return;
	}

	/* ok, calculate normal */
	facecount = 0;
	for (tmp_b=0; tmp_b<pointfaces[mypoint*POINT_FACES]; tmp_b++) {
		tmp_a = pointfaces[mypoint*POINT_FACES+tmp_b+1];
		/* printf ("comparing myface %d to %d\n",curpoly,tmp_a); */
	
		if (curpoly == tmp_a) {
			zz = 0.0;
		} else {
			zz = calc_angle_between_two_vectors(facenormals[curpoly],facenormals[tmp_a] );
		}
		/* printf ("angle between faces is %f, creaseAngle is %f\n",zz,creaseAngle); */

		
		if (zz <= creaseAngle) {
			/* printf ("count this one in; adding %f %f %f\n",facenormals[tmp_a].x,facenormals[tmp_a].y,facenormals[tmp_a].z); */
			point_normal[0] += facenormals[tmp_a].x;
			point_normal[1] += facenormals[tmp_a].y;
			point_normal[2] += facenormals[tmp_a].z;
		/*
		} else {
			printf ("count this one out\n");
		*/
		}
	}
	temp.x = point_normal[0]; temp.y=point_normal[1]; temp.z=point_normal[2];	
	normalize_vector(&temp); 
	point_normal[0]=temp.x; point_normal[1]=temp.y; point_normal[2]=temp.z;

	/* printf ("normalized vector is %f %f %f\n",point_normal[0],
		point_normal[1], point_normal[2]); */
}
	

/* This fuction returns a code that determines whether edges   */
/* at this point need to smoothed over or not. This code is    */
/* number from -1 to 16. These correspond to 16 possible       */
/* ways the edges can be configured at this point.             */
/* This function uses the data in the adj struct to make       */
/* these determinations. Macros would have been nice for this  */
/* , but this system has enough already.                       */
/*                                                             */
/*  0       1      2      3       0-3 are corners              */
/* ---*      |    |      *---                                  */
/*    |      |    |      |                                     */
/*    |   ---*    *---   |   s means the edge is smoothed over */
/*                                                             */
/*   4      5         6          7                             */
/*    |      |       |          s       codes 4-11 are for     */
/*    |      |       |          s       sides where one edge   */
/* ---*   sss*    ---*---       s       may or may not have    */ 
/*    |      |               ---*---    to be smoothed over    */
/*    |      |                                                 */
/*                                                             */
/*    8      9      10          11                             */
/*  |       |     ---*---     ---*---     code 16 is for error */
/*  |       |        |           s                             */
/*  *---    *sss     |           s                             */
/*  |       |        |           s                             */
/*  |       |                                                  */
/*                                                             */
/*      12       13          14         15                     */
/*       s        |          |           s                     */
/*       s        |          |           s                     */
/*    ---*---  sss*sss    ---*---     sss*sss                  */
/*       s        |          |           s                     */
/*       s        |          |           s                     */
/*                                                             */
int find_edge_config_at_this_point(struct VRML_PolyRep *rep,
				struct VRML_Extrusion_Adj *adj,
				int index_pt, float creaseAngle){
	
	float angle_north_south, angle_east_west;

	/* find out if we are in the north_east corner */
	if (	(adj[index_pt].north_pt == -1)
		&&(adj[index_pt].east_pt  == -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  != -1)){
		return 0;		
	}
	
	/* find out if we are in the south_east corner */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  == -1)
		&&(adj[index_pt].south_pt == -1)
		&&(adj[index_pt].west_pt  != -1)){
		return 1;		
	}

	/* find out if we are in the south_west corner */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt == -1)
		&&(adj[index_pt].west_pt  == -1)){
		return 2;		
	}

	/* find out if we are in the north_west corner */
	if (	(adj[index_pt].north_pt == -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  == -1)){
		return 3;		
	}

	/* find out if we are the east edge */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  == -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  != -1)){

		angle_north_south = calc_angle_between_two_vectors(
				adj[index_pt].south_vec, adj[index_pt].north_vec);
		if (angle_north_south <= creaseAngle){return 4;}
		else {return 5;}
	}

	/* find out if we are the south edge */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt == -1)
		&&(adj[index_pt].west_pt  != -1)){

		angle_east_west = calc_angle_between_two_vectors(
				adj[index_pt].west_vec, adj[index_pt].east_vec);
		if (angle_east_west <= creaseAngle){return 6;}
		else {return 7;}
	}

	/* find out if we are the west edge */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  == -1)){

		angle_north_south = calc_angle_between_two_vectors(
				adj[index_pt].south_vec, adj[index_pt].north_vec);
		if (angle_north_south <= creaseAngle){return 8;}
		else {return 9;}
	}

	/* find out if we are the north edge */
	if (	(adj[index_pt].north_pt == -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  != -1)){

		angle_east_west = calc_angle_between_two_vectors(
				adj[index_pt].west_vec, adj[index_pt].east_vec);
		if (angle_east_west <= creaseAngle){return 10;}
		else {return 11;}
	}

	/* general case */
	if (	(adj[index_pt].north_pt != -1)
		&&(adj[index_pt].east_pt  != -1)
		&&(adj[index_pt].south_pt != -1)
		&&(adj[index_pt].west_pt  != -1)){

		angle_north_south = calc_angle_between_two_vectors(
				adj[index_pt].south_vec, adj[index_pt].north_vec);
		angle_east_west = calc_angle_between_two_vectors(
				adj[index_pt].west_vec, adj[index_pt].east_vec);
	
		if ( 	(angle_east_west <= creaseAngle)
			&&(angle_north_south <= creaseAngle)){return 14;}

		if ( 	(angle_east_west > creaseAngle)
			&&(angle_north_south <= creaseAngle)){return 12;}

		if ( 	(angle_east_west <= creaseAngle)
			&&(angle_north_south > creaseAngle)){return 13;}

		if ( 	(angle_east_west > creaseAngle)
			&&(angle_north_south > creaseAngle)){return 15;}
	}
/*	printf("angle_north_south: %f angle_east_west: %f \n", angle_north_south, angle_east_west);
        printf("%i   north_vec:  %f  %f  %f \n", index_pt, adj[index_pt].north_vec.x,  
				adj[index_pt].north_vec.y, adj[index_pt].north_vec.z);
        printf("%i   south_vec:  %f  %f  %f \n", index_pt, adj[index_pt].south_vec.x,  
				adj[index_pt].south_vec.y, adj[index_pt].south_vec.z);

        printf("%i   north_pt: %i\n", index_pt, adj[index_pt].north_pt);
        printf("%i   east_pt: %i\n", index_pt, adj[index_pt].east_pt);
        printf("%i   south_pt: %i\n", index_pt, adj[index_pt].south_pt);
        printf("%i   west_pt: %i\n", index_pt, adj[index_pt].west_pt);
        printf("%i   north_east_pt: %i\n", index_pt, adj[index_pt].north_east_pt);
        printf("%i   south_east_pt: %i\n", index_pt, adj[index_pt].south_east_pt);
        printf("%i   south_west_pt: %i\n", index_pt, adj[index_pt].south_west_pt);
        printf("%i   north_west_pt: %i\n", index_pt, adj[index_pt].north_west_pt);
        printf("-----------------------------------------------\n");
*/
	return 16;
}




/* This function answers with a number from 1 thru 4. It determines */
/* quadrant the triangle belongs to with reference to this point.   */
/*   4th          1st                                               */
/*       +---+---+      The triangle we are looking for is one of   */
/*       |\  |\  |      the 8.                                      */
/*       | \ | \ |                                                  */
/*       |  \|  \|                                                  */
/*       +---*---+                                                  */
/*       |\  |\  |                                                  */
/*       | \ | \ |                                                  */
/*       |  \|  \|                                                  */
/*   3rd +---+---+ 2nd                                              */
/*                                                                  */
int find_the_quadrant_of_this_triangle(struct VRML_PolyRep *rep,
				struct VRML_Extrusion_Adj *adj, 
				int index_pt, int a_pt, int b_pt){

	/*first group of cases, diagonal goes from south_east to north_west*/
	/*case where the triangle is in the first quadrant*/
	if ( 	((adj[index_pt].north_pt == a_pt ) && (adj[index_pt].east_pt == b_pt))
		||((adj[index_pt].north_pt == b_pt ) && (adj[index_pt].east_pt == a_pt))	){
		return 1;
	}
		
	/*case where the triangle is in the third quadrant*/
	if ( 	((adj[index_pt].south_pt == a_pt ) && (adj[index_pt].west_pt == b_pt))
		||((adj[index_pt].south_pt == b_pt ) && (adj[index_pt].west_pt == a_pt))	){
		return 3;
	}

	/*case where the triangle is in the second quadrant*/
	if ( 	((adj[index_pt].east_pt == a_pt ) && (adj[index_pt].south_east_pt == b_pt))
		||((adj[index_pt].east_pt == b_pt ) && (adj[index_pt].south_east_pt == a_pt))	
		||((adj[index_pt].south_pt == a_pt ) && (adj[index_pt].south_east_pt == b_pt))	
		||((adj[index_pt].south_pt == b_pt ) && (adj[index_pt].south_east_pt == a_pt))	){
		return 2;
	}

	/*case where the triangle is in the second quadrant*/
	if ( 	((adj[index_pt].west_pt == a_pt ) && (adj[index_pt].north_west_pt == b_pt))
		||((adj[index_pt].west_pt == b_pt ) && (adj[index_pt].north_west_pt == a_pt))	
		||((adj[index_pt].north_pt == a_pt ) && (adj[index_pt].north_west_pt == b_pt))	
		||((adj[index_pt].north_pt == b_pt ) && (adj[index_pt].north_west_pt == a_pt))	){
		return 4;
	}



	/*second group of cases, diagonal goes from south_west to north_east*/

	/*case where the triangle is in the first quadrant*/
	if ( 	((adj[index_pt].east_pt == a_pt ) && (adj[index_pt].north_east_pt == b_pt))
		||((adj[index_pt].east_pt == b_pt ) && (adj[index_pt].north_east_pt == a_pt))	
		||((adj[index_pt].north_pt == a_pt ) && (adj[index_pt].north_east_pt == b_pt))	
		||((adj[index_pt].north_pt == b_pt ) && (adj[index_pt].north_east_pt == a_pt))	){
		return 1;
	}

	/*case where the triangle is in the second quadrant*/
	if ( 	((adj[index_pt].south_pt == a_pt ) && (adj[index_pt].east_pt == b_pt))
		||((adj[index_pt].south_pt == b_pt ) && (adj[index_pt].east_pt == a_pt))	){
		return 2;
	}
		
	/*case where the triangle is in the third quadrant*/
	if ( 	((adj[index_pt].west_pt == a_pt ) && (adj[index_pt].south_west_pt == b_pt))
		||((adj[index_pt].west_pt == b_pt ) && (adj[index_pt].south_west_pt == a_pt))	
		||((adj[index_pt].south_pt == a_pt ) && (adj[index_pt].south_west_pt == b_pt))	
		||((adj[index_pt].south_pt == b_pt ) && (adj[index_pt].south_west_pt == a_pt))	){
		return 3;
	}

	/*case where the triangle is in the fourth quadrant*/
	if ( 	((adj[index_pt].north_pt == a_pt ) && (adj[index_pt].west_pt == b_pt))
		||((adj[index_pt].north_pt == b_pt ) && (adj[index_pt].west_pt == a_pt))	){
		return 4;
	}



			
        printf("%i   south_pt: %i\n", index_pt, adj[index_pt].south_pt);
        printf("%i   north_pt: %i\n", index_pt, adj[index_pt].north_pt);
        printf("%i   east_pt: %i\n", index_pt, adj[index_pt].east_pt);
        printf("%i   west_pt: %i\n", index_pt, adj[index_pt].west_pt);
        printf("%i   north_east_pt: %i\n", index_pt, adj[index_pt].north_east_pt);
        printf("%i   south_east_pt: %i\n", index_pt, adj[index_pt].south_east_pt);
        printf("%i   south_west_pt: %i\n", index_pt, adj[index_pt].south_west_pt);
        printf("%i   north_west_pt: %i\n", index_pt, adj[index_pt].north_west_pt);
        printf("-----------------------------------------------\n");
        printf("index_pt: %i  a_pt: %i  b_pt: %i \n", index_pt, a_pt, b_pt);
        printf("-----------------------------------------------\n");
	

	return 0;
}




void calc_poly_normals_extrusion(struct VRML_PolyRep *rep, 
			struct VRML_Extrusion_Adj *adj,
			int nspi, int nsec, int ntri, int nctri,
			float crease_angle) 
{

/* Each point in the coord array now has a corresponding entry in */
/* the adj array which specifies which points are adjacent to a   */
/* given point.  Values of -1 in the adj indicate that the point  */
/* being considered is on an edge of the extrusion.(i.e. there is */
/* no adjacent point)  In this case, only the points present are  */
/* considered */

	int   i, j, p[3], edge_config, quadrant; 
	float vector_length;
	struct pt cap_vec1, cap_vec2, cap_normal;	

        for(i=0; i < (nsec * nspi) ; i++) {

                if ( adj[i].north_pt != -1 ){
			adj[i].north_vec.x = 
				(rep->coord[ adj[i].north_pt *3+0] - rep->coord[i*3+0]);
                        adj[i].north_vec.y = 
				(rep->coord[ adj[i].north_pt *3+1] - rep->coord[i*3+1]);
                        adj[i].north_vec.z = 
				(rep->coord[ adj[i].north_pt *3+2] - rep->coord[i*3+2]);
                }
                else {
                        adj[i].north_vec.x = 0;
			adj[i].north_vec.y = 0;
			adj[i].north_vec.z = 0;
                }
/*
		printf("%i  next_layer vector: %f %f %f   \n",i, adj[i].north_vec.x, 
			adj[i].north_vec.y, adj[i].north_vec.z );
*/


                if ( adj[i].south_pt != -1 ){
                        adj[i].south_vec.x =
                                (rep->coord[ adj[i].south_pt *3+0] - rep->coord[i*3+0]);
                        adj[i].south_vec.y =
                                (rep->coord[ adj[i].south_pt *3+1] - rep->coord[i*3+1]);
                        adj[i].south_vec.z =
                                (rep->coord[ adj[i].south_pt *3+2] - rep->coord[i*3+2]);
                }
                else {
                        adj[i].south_vec.x = 0;
			adj[i].south_vec.y = 0;
			adj[i].south_vec.z = 0;
                }
/*
                printf("%i  prev_layer vector: %f %f %f   \n",i, adj[i].south_vec.x,
                        adj[i].south_vec.y, adj[i].south_vec.z );
*/


                if ( adj[i].east_pt != -1 ){
                        adj[i].east_vec.x =
                                (rep->coord[ adj[i].east_pt *3+0] - rep->coord[i*3+0]);
                        adj[i].east_vec.y =
                                (rep->coord[ adj[i].east_pt *3+1] - rep->coord[i*3+1]);
                        adj[i].east_vec.z =
                                (rep->coord[ adj[i].east_pt *3+2] - rep->coord[i*3+2]);
                }
                else {
                        adj[i].east_vec.x = 0;
			adj[i].east_vec.y = 0;
			adj[i].east_vec.z = 0;
                }
/*
                printf("%i  next_cross vector: %f %f %f   \n",i, adj[i].east_vec.x,
                        adj[i].east_vec.y, adj[i].east_vec.z );
*/


                if ( adj[i].west_pt != -1 ){
                        adj[i].west_vec.x =
                                (rep->coord[ adj[i].west_pt *3+0] - rep->coord[i*3+0]);
                        adj[i].west_vec.y =
                                (rep->coord[ adj[i].west_pt *3+1] - rep->coord[i*3+1]);
                        adj[i].west_vec.z =
                                (rep->coord[ adj[i].west_pt *3+2] - rep->coord[i*3+2]);
                }
                else {
                        adj[i].west_vec.x = 0;
			adj[i].west_vec.y = 0;
			adj[i].west_vec.z = 0;
                }
/*
                printf("%i  prev_cross vector: %f %f %f   \n",i, adj[i].west_vec.x,
                        adj[i].west_vec.y, adj[i].west_vec.z );
*/
	}/*for*/


	/*Calculate the diag-normals*/
        for(i=0; i < (nsec * nspi) ; i++) {
		if((adj[i].north_pt != -1) && (adj[i].east_pt != -1)){

			calc_vector_product(adj[i].north_vec, adj[i].east_vec, 
							&adj[i].first_quad_diag_vec);

			vector_length = calc_vector_length( adj[i].first_quad_diag_vec);

			adj[i].first_quad_diag_vec.x = (adj[i].first_quad_diag_vec.x / vector_length) ;
			adj[i].first_quad_diag_vec.y = (adj[i].first_quad_diag_vec.y / vector_length) ;
			adj[i].first_quad_diag_vec.z = (adj[i].first_quad_diag_vec.z / vector_length) ;

		}				
		else{
			adj[i].first_quad_diag_vec.x = 0;
			adj[i].first_quad_diag_vec.y = 0;
			adj[i].first_quad_diag_vec.z = 0;
		}

/*		printf("%i   first_quad_diag_vec:  %f  %f  %f  \n",i, adj[i].first_quad_diag_vec.x, 
			adj[i].first_quad_diag_vec.y, adj[i].first_quad_diag_vec.z);
*/
		if((adj[i].east_pt != -1) && (adj[i].south_pt != -1)){

			calc_vector_product(adj[i].east_vec, adj[i].south_vec,
							&adj[i].second_quad_diag_vec);

			vector_length = calc_vector_length( adj[i].second_quad_diag_vec);

			adj[i].second_quad_diag_vec.x = (adj[i].second_quad_diag_vec.x / vector_length);
			adj[i].second_quad_diag_vec.y = (adj[i].second_quad_diag_vec.y / vector_length);
			adj[i].second_quad_diag_vec.z = (adj[i].second_quad_diag_vec.z / vector_length);
		}				
		else{
			adj[i].second_quad_diag_vec.x = 0;
			adj[i].second_quad_diag_vec.y = 0;
			adj[i].second_quad_diag_vec.z = 0;
		}

/*		printf("%i   second_quad_diag_vec:  %f  %f  %f  \n",i, adj[i].second_quad_diag_vec.x, 
			adj[i].second_quad_diag_vec.y, adj[i].second_quad_diag_vec.z);
*/
		if((adj[i].south_pt != -1) && (adj[i].west_pt != -1)){

			calc_vector_product(adj[i].south_vec, adj[i].west_vec, 
									&adj[i].third_quad_diag_vec);

			vector_length = calc_vector_length( adj[i].third_quad_diag_vec);

			adj[i].third_quad_diag_vec.x = (adj[i].third_quad_diag_vec.x / vector_length);
			adj[i].third_quad_diag_vec.y = (adj[i].third_quad_diag_vec.y / vector_length);
			adj[i].third_quad_diag_vec.z = (adj[i].third_quad_diag_vec.z / vector_length);
		}				
		else{
			adj[i].third_quad_diag_vec.x = 0;
			adj[i].third_quad_diag_vec.y = 0;
			adj[i].third_quad_diag_vec.z = 0;
		}

/*		printf("%i   third_quad_diag_vec:  %f  %f  %f  \n",i, adj[i].third_quad_diag_vec.x, 
			adj[i].third_quad_diag_vec.y, adj[i].third_quad_diag_vec.z);
*/
		if((adj[i].west_pt != -1) && (adj[i].north_pt != -1)){

			calc_vector_product(adj[i].west_vec, adj[i].north_vec,
									&adj[i].fourth_quad_diag_vec);

			vector_length = calc_vector_length( adj[i].fourth_quad_diag_vec);

			adj[i].fourth_quad_diag_vec.x = (adj[i].fourth_quad_diag_vec.x / vector_length);
			adj[i].fourth_quad_diag_vec.y = (adj[i].fourth_quad_diag_vec.y / vector_length);
			adj[i].fourth_quad_diag_vec.z = (adj[i].fourth_quad_diag_vec.z / vector_length);
		}				
		else{
			adj[i].fourth_quad_diag_vec.x = 0;
			adj[i].fourth_quad_diag_vec.y = 0;
			adj[i].fourth_quad_diag_vec.z = 0;
		}

/*		printf("%i   fourth_quad_diag_vec:  %f  %f  %f  \n",i, adj[i].fourth_quad_diag_vec.x, 
			adj[i].fourth_quad_diag_vec.y, adj[i].fourth_quad_diag_vec.z);
*/
	}/*for*/


	for (i=0; i<(nsec*nspi); i++ ){
		adj[i].cumul_normal_vec.x = 	(adj[i].first_quad_diag_vec.x
						+ adj[i].second_quad_diag_vec.x
						+ adj[i].third_quad_diag_vec.x
						+ adj[i].fourth_quad_diag_vec.x) ;

		adj[i].cumul_normal_vec.y = 	(adj[i].first_quad_diag_vec.y
						+ adj[i].second_quad_diag_vec.y
						+ adj[i].third_quad_diag_vec.y
						+ adj[i].fourth_quad_diag_vec.y) ;
 
		adj[i].cumul_normal_vec.z = 	(adj[i].first_quad_diag_vec.z
						+ adj[i].second_quad_diag_vec.z
						+ adj[i].third_quad_diag_vec.z
						+ adj[i].fourth_quad_diag_vec.z) ;
		normalize_vector(&adj[i].cumul_normal_vec); 

		adj[i].north_edge_vec.x = adj[i].fourth_quad_diag_vec.x + adj[i].first_quad_diag_vec.x;	
		adj[i].north_edge_vec.y = adj[i].fourth_quad_diag_vec.y + adj[i].first_quad_diag_vec.y;	
		adj[i].north_edge_vec.z = adj[i].fourth_quad_diag_vec.z + adj[i].first_quad_diag_vec.z;	
		normalize_vector(&adj[i].north_edge_vec); 

		adj[i].east_edge_vec.x = adj[i].first_quad_diag_vec.x + adj[i].second_quad_diag_vec.x;	
		adj[i].east_edge_vec.y = adj[i].first_quad_diag_vec.y + adj[i].second_quad_diag_vec.y;	
		adj[i].east_edge_vec.z = adj[i].first_quad_diag_vec.z + adj[i].second_quad_diag_vec.z;	
		normalize_vector(&adj[i].east_edge_vec); 

		adj[i].south_edge_vec.x = adj[i].second_quad_diag_vec.x + adj[i].third_quad_diag_vec.x;	
		adj[i].south_edge_vec.y = adj[i].second_quad_diag_vec.y + adj[i].third_quad_diag_vec.y;	
		adj[i].south_edge_vec.z = adj[i].second_quad_diag_vec.z + adj[i].third_quad_diag_vec.z;	
		normalize_vector(&adj[i].south_edge_vec); 

		adj[i].west_edge_vec.x = adj[i].third_quad_diag_vec.x + adj[i].fourth_quad_diag_vec.x;	
		adj[i].west_edge_vec.y = adj[i].third_quad_diag_vec.y + adj[i].fourth_quad_diag_vec.y;	
		adj[i].west_edge_vec.z = adj[i].third_quad_diag_vec.z + adj[i].fourth_quad_diag_vec.z;	
		normalize_vector(&adj[i].west_edge_vec); 

/*		printf("%i  %f  %f  %f   \n ",i ,adj[i].cumul_normal_vec.x, 
				adj[i].cumul_normal_vec.y, adj[i].cumul_normal_vec.z );*/
	}/*for*/


	for (i=0; i<ntri; i++){
		/* Get indexes to the three points */	
		p[0] = rep->cindex[i*3+0];
		p[1] = rep->cindex[i*3+1];
		p[2] = rep->cindex[i*3+2];
		
		/* specify the normal indexes */
		rep->norindex[i*3+0] = i*3+0;  
		rep->norindex[i*3+1] = i*3+1;  
		rep->norindex[i*3+2] = i*3+2;

		/* specify the normals */

		/* Insert decision code right here */
		
		for (j=0; j<3; j++){
			
			edge_config = find_edge_config_at_this_point(rep, adj, p[j], crease_angle);
			switch(j){
				case(0):
					quadrant = find_the_quadrant_of_this_triangle(rep, adj, p[j], p[1], p[2]);
					break;
				case(1):
					quadrant = find_the_quadrant_of_this_triangle(rep, adj, p[j], p[0], p[2]);
					break;
				case(2):
					quadrant = find_the_quadrant_of_this_triangle(rep, adj, p[j], p[0], p[1]);
					break;
				default:
					printf("Error in calc_poly_normals_extrusion(): loop error \n");
					break; 
			}
			/*printf("edge_config %i ", edge_config);*/	

			switch(edge_config){

				case(0):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].third_quad_diag_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].third_quad_diag_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].third_quad_diag_vec.z;
					break;

				case(1):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].fourth_quad_diag_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].fourth_quad_diag_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].fourth_quad_diag_vec.z;
					break;

				case(2):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].first_quad_diag_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].first_quad_diag_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].first_quad_diag_vec.z;
					break;

				case(3):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].second_quad_diag_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].second_quad_diag_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].second_quad_diag_vec.z;
					break;

				case(4):
					switch(quadrant){
					case(3):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].south_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].south_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].south_edge_vec.z;
						break;
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].north_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].north_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].north_edge_vec.z;
						break;
					case(1):
					case(2):
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 4\n");
						break;
					}/*switch quadrant*/
					break;
					
				case(5):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].west_edge_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].west_edge_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].west_edge_vec.z;
					break;
					
				case(6):
					switch(quadrant){
					case(1):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].east_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].east_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].east_edge_vec.z;
						break;
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].west_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].west_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].west_edge_vec.z;
						break;
					case(2):
					case(3):
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 6\n");
						break;
					}/*switch quadrant*/
					break;

				case(7):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].north_edge_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].north_edge_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].north_edge_vec.z;
					break;

				case(8):
					switch(quadrant){
					case(1):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].north_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].north_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].north_edge_vec.z;
						break;
					case(2):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].south_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].south_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].south_edge_vec.z;
						break;
					case(3):
					case(4):
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 8\n");

						break;
					}/*switch quadrant*/
					break;

				case(9):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].east_edge_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].east_edge_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].east_edge_vec.z;
					break;

				case(10):
					switch(quadrant){
					case(2):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].east_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].east_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].east_edge_vec.z;
						break;
					case(3):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].west_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].west_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].west_edge_vec.z;
						break;
					case(1):
					case(4):
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 10\n");
						break;
					}/*switch quadrant*/
					break;

				case(11):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].south_edge_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].south_edge_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].south_edge_vec.z;
					break;

				case(12):
					switch(quadrant){
					case(1):
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].north_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].north_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].north_edge_vec.z;
						break;
					case(2):
					case(3):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].south_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].south_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].south_edge_vec.z;
						break;
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 12\n");
						break;
					}/*switch quadrant*/
					break;
				
				case(13):
					switch(quadrant){
					case(1):
					case(2):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].east_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].east_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].east_edge_vec.z;
						break;
					case(3):
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].west_edge_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].west_edge_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].west_edge_vec.z;
						break;
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 13\n");
						break;
					}/*switch quadrant*/
					break;

				case(14):
					switch(quadrant){
					case(1):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].first_quad_diag_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].first_quad_diag_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].first_quad_diag_vec.z;
						break;
					case(2):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].second_quad_diag_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].second_quad_diag_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].second_quad_diag_vec.z;
						break;
					case(3):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].third_quad_diag_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].third_quad_diag_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].third_quad_diag_vec.z;
						break;
					case(4):
						rep->normal[i*9+j*3+0] = adj[ p[j] ].fourth_quad_diag_vec.x;
						rep->normal[i*9+j*3+1] = adj[ p[j] ].fourth_quad_diag_vec.y;
						rep->normal[i*9+j*3+2] = adj[ p[j] ].fourth_quad_diag_vec.z;
						break;
					default:	
						printf("Error in calc_poly_normals_extrusion(): quadrant error at case 14\n");
						break;
					}/*switch quadrant*/
					break;

				case(15):
					rep->normal[i*9+j*3+0] = adj[ p[j] ].cumul_normal_vec.x;
					rep->normal[i*9+j*3+1] = adj[ p[j] ].cumul_normal_vec.y;
					rep->normal[i*9+j*3+2] = adj[ p[j] ].cumul_normal_vec.z;
					break;

				default:
					printf("Error in calc_poly_normals_extrusion(): unknown case error\n");
					break;
			}/*switch edge_config*/	

		}/*for j */

/*
		rep->normal[i*9+0] = adj[ p[0] ].cumul_normal_vec.x;
		rep->normal[i*9+1] = adj[ p[0] ].cumul_normal_vec.y;
		rep->normal[i*9+2] = adj[ p[0] ].cumul_normal_vec.z;
		  
		rep->normal[i*9+3] = adj[ p[1] ].cumul_normal_vec.x;
		rep->normal[i*9+4] = adj[ p[1] ].cumul_normal_vec.y;
		rep->normal[i*9+5] = adj[ p[1] ].cumul_normal_vec.z;
		
		rep->normal[i*9+6] = adj[ p[2] ].cumul_normal_vec.x;
		rep->normal[i*9+7] = adj[ p[2] ].cumul_normal_vec.y;
		rep->normal[i*9+8] = adj[ p[2] ].cumul_normal_vec.z;
*/

	}/*for i */


	/*calculate the normals for the endcaps*/
	for (i=ntri; i < (nctri+ntri) ; i++){
		p[0] = rep->cindex[i*3+0];
		p[1] = rep->cindex[i*3+1];
		p[2] = rep->cindex[i*3+2];

		cap_vec1.x = rep->coord[p[1]*3+0] - rep->coord[p[0]*3+0];
		cap_vec1.y = rep->coord[p[1]*3+1] - rep->coord[p[0]*3+1];
		cap_vec1.z = rep->coord[p[1]*3+2] - rep->coord[p[0]*3+2];

		cap_vec2.x = rep->coord[p[2]*3+0] - rep->coord[p[0]*3+0];
		cap_vec2.y = rep->coord[p[2]*3+1] - rep->coord[p[0]*3+1];
		cap_vec2.z = rep->coord[p[2]*3+2] - rep->coord[p[0]*3+2];

		calc_vector_product(cap_vec1, cap_vec2, &cap_normal);

		/* specify the normal indexes */
		rep->norindex[i*3+0] = i*3+0;  
		rep->norindex[i*3+1] = i*3+1;  
		rep->norindex[i*3+2] = i*3+2;

		/* specify the normals */
		rep->normal[i*9+0] = cap_normal.x;
		rep->normal[i*9+1] = cap_normal.y;
		rep->normal[i*9+2] = cap_normal.z;
		  
		rep->normal[i*9+3] = cap_normal.x;
		rep->normal[i*9+4] = cap_normal.y;
		rep->normal[i*9+5] = cap_normal.z;
		
		rep->normal[i*9+6] = cap_normal.x;
		rep->normal[i*9+7] = cap_normal.y;
		rep->normal[i*9+8] = cap_normal.z;
	}/*for*/


}/*calc_poly_normals_extrusion*/


void initialize_smooth_normals() {

	/* first complex face - are we running in fast or good mode... */
	if (smooth_normals == -1) {
		/* Needs to be initialized */
		glGetIntegerv (GL_SHADE_MODEL, &smooth_normals);
		smooth_normals = smooth_normals == GL_SMOOTH;
	}
}
