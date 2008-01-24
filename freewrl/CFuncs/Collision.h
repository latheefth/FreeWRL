/* $Id$
 *
 * Copyright (C) 2002 Nicolas Coderre CRC Canada
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License (file COPYING in the distribution)
 * for conditions of use and redistribution.
 */
#ifndef COLLISIONH
#define COLLISIONH


#include <stdio.h>
#include <math.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#if defined(__APPLE__)
#include <sys/malloc.h>
#endif

#include "headers.h"
#include "LinearAlgebra.h"
#include "Structs.h"

/* Collision detection results structure*/
struct sCollisionInfo {
    struct pt Offset;
    int Count;
    double Maximum2; /*squared. so we only need to root once */
};

typedef int prflags;
#define PR_DOUBLESIDED 0x01
#define PR_FRONTFACING 0x02 /* overrides effect of doublesided. */
#define PR_BACKFACING 0x04 /* overrides effect of doublesided, all normals are reversed. */
#define PR_NOSTEPING 0x08 /* gnores stepping. used internally */


/*uncomment this to enable the scene exporting functions */
/*#define DEBUG_SCENE_EXPORT*/

double
closest_point_of_segment_to_y_axis(struct pt p1,
								   struct pt p2);
double
closest_point_of_segment_to_origin(struct pt p1,
								   struct pt p2);

struct pt
closest_point_of_plane_to_origin(struct pt b,
								 struct pt n);

int
intersect_segment_with_line_on_yplane(struct pt* pk,
									  struct pt p1,
									  struct pt p2,
									  struct pt q1,
									  struct pt q2);

int
getk_intersect_line_with_ycylinder(double* k1,
								   double* k2,
								   double r,
								   struct pt pp1,
								   struct pt n);

int
project_on_cylindersurface(struct pt* res,
						   struct pt p,
						   struct pt n,
						   double r);

int
getk_intersect_line_with_sphere(double* k1,
								double* k2,
								double r,
								struct pt pp1,
								struct pt n);

int
project_on_spheresurface(struct pt* res,
						 struct pt p,
						 struct pt n,
						 double r);

struct pt
project_on_yplane(struct pt p1,
				  struct pt n,
				  double y);

struct pt
project_on_cylindersurface_plane(struct pt p,
								 struct pt n,
								 double r);

int
perpendicular_line_passing_inside_poly(struct pt a,
									   struct pt* p,
									   int num);

int
getk_intersect_segment_with_ycylinder(double* k1,
									  double* k2,
									  double r,
									  struct pt pp1,
									  struct pt pp2);

int
helper_poly_clip_cap(struct pt* clippedpoly,
					 int clippedpolynum,
					 const struct pt* p,
					 int num,
					 double r,
					 struct pt n,
					 double y,
					 int stepping);

struct pt
polyrep_disp_rec(double y1,
				 double y2,
				 double ystep,
				 double r,
				 struct X3D_PolyRep* pr,
				 struct pt* n,
				 struct pt dispsum,
				 prflags flags);

struct pt
planar_polyrep_disp_rec(double y1,
						double y2,
						double ystep,
						double r,
						struct X3D_PolyRep* pr,
						struct pt n,
						struct pt dispsum,
						prflags flags);

int
helper_line_clip_cap(struct pt* clippedpoly,
					 int clippedpolynum,
					 struct pt p1,
					 struct pt p2,
					 double r,
					 struct pt n,
					 double y,
					 int stepping);

/*accumulator function, for displacements. */
void accumulate_disp(struct sCollisionInfo* ci, struct pt add);

/*returns (1-k)p1 + k p2 */
struct pt weighted_sum(struct pt p1, struct pt p2, double k);

/*feed a poly, and stats of a cylinder, it returns the displacement in the direction of the
  normal of the poly that is needed for them not to intersect any more.*/
struct pt get_poly_normal_disp(double y1, double y2, double r, struct pt* p, int num, struct pt n);

/*feed a poly, and stats of a cylinder, it returns the vertical displacement that is needed for them not to intersect any more,
  if this displacement is less than the height of the cylinder (y2-y1).*/
struct pt get_poly_step_disp(double y1, double y2, double r, struct pt* p, int num, struct pt n);

/*feed a poly, and stats of a cylinder, it returns the displacement in the direction of the
  normal of the poly that is needed for them not to intersect any more, or vertically if contact point below ystep*/
struct pt get_poly_disp(double y1, double y2, double ystep, double r, struct pt* p, int num, struct pt n);

/*feed a poly, and radius of a sphere, it returns the displacement in the direction of the
  normal of the poly that is needed for them not to intersect any more.*/
struct pt get_poly_normal_disp_with_sphere(double r, struct pt* p, int num, struct pt n);
/*feed a poly, and radius of a sphere, it returns the minimum displacement and
  the direction that is needed for them not to intersect any more.*/
struct pt get_poly_min_disp_with_sphere(double r, struct pt* p, int num, struct pt n);

/*feed a line and a normal, and stats of a cylinder, it returns the displacement in the direction of the
  normal that is needed for them not to intersect any more.*/
struct pt get_line_normal_disp(double y1, double y2, double r, struct pt p1, struct pt p2, struct pt n);

/*feed a line and a normal, and stats of a cylinder, it returns the vertical displacement
  that is needed for them not to intersect any more.*/
struct pt get_line_step_disp(double y1, double y2, double r, struct pt p1, struct pt p2, struct pt n);

/*feed a line and a normal, and stats of a cylinder, it returns the displacement in the direction of the
  normal, or the vertical displacement(in case of stepping) that is needed for them not to intersect any more.*/
struct pt get_line_disp(double y1, double y2, double ystep, double r, struct pt p1, struct pt p2, struct pt n);

/*feed a point and a normal, and stats of a cylinder, it returns the displacement in the direction of the
  normal that is needed for them not to intersect any more.*/
struct pt get_point_normal_disp(double y1, double y2, double r, struct pt p1, struct pt n);

/*feed a point and a normal, and stats of a cylinder, it returns the displacement in the direction of the
  normal, or the vertical displacement(in case of stepping) that is needed for them not to intersect any more.*/
struct pt get_point_disp(double y1, double y2, double ystep, double r, struct pt p1, struct pt n);

/*feed a box (a corner, and the three vertice sides) and the stats of a cylinder, it returns the
  displacement of the box that is needed for them not to intersect any more, with optionnal stepping displacement */
struct pt box_disp(double y1, double y2, double ystep, double r,struct pt p0, struct pt i, struct pt j, struct pt k);

/*fast test to see if a box intersects a y-cylinder.
 * gives false positives */
int fast_ycylinder_box_intersect(double y1, double y2, double r,struct pt pcenter, double xs, double ys, double zs);


/*fast test to see if the min/max of a polyrep structure (IndexedFaceSet, eg)  intersects a y-cylinder.
 * gives false positives */
int fast_ycylinder_polyrep_intersect(double y1, double y2, double r,struct pt pcenter, double scale, struct X3D_PolyRep *pr);

/*fast test to see if a cone intersects a y-cylinder. */
/*gives false positives. */
int fast_ycylinder_cone_intersect(double y1, double y2, double r,struct pt pcenter, double halfheight, double baseradius);

/* fast test to see if a sphere intersects a y-cylinder.
   specify sphere center, and a point on it's surface
  gives false positives. */
int fast_ycylinder_sphere_intersect(double y1, double y2, double r,struct pt pcenter, struct pt psurface);


/*algorithm is approximative */
/*basically, it does collision with a triangle on a plane that passes through the origin.*/
struct pt cone_disp(double y1, double y2, double ydisp, double r, struct pt base, struct pt top, double baseradius);

/*algorithm is approximative */
/*basically, it does collision with a rectangle on a plane that passes through the origin.*/
struct pt cylinder_disp(double y1, double y2, double ydisp, double r, struct pt base, struct pt top, double baseradius);

/*uses sphere displacement, and a cylinder for stepping */
struct pt polyrep_disp(double y1, double y2, double ydisp, double r, struct X3D_PolyRep pr, GLdouble* mat, prflags flags);

/*displacement when the polyrep structure is all in the same plane
  if normal is zero, it will be calculated form the first triangle*/
struct pt planar_polyrep_disp(double y1, double y2, double ydisp, double r, struct X3D_PolyRep pr, GLdouble* mat, prflags flags, struct pt n);

struct pt elevationgrid_disp( double y1, double y2, double ydisp, double r, struct X3D_PolyRep pr,
			      int xdim, int zdim, double xs, double zs, GLdouble* mat, prflags flags);

/* functions VERY usefull for debugging purposes
   Use these inside FreeWRL to export a scene to
   the debugging programs. */
#ifdef DEBUG_SCENE_EXPORT
void printpolyrep(struct X3D_PolyRep pr, int npoints);

void printmatrix(GLdouble* mat);
#endif

#endif




