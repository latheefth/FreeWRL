/******************************************************************************
 Copyright (C) 1998 Tuomas J. Lukka, 2003 John Stewart, Ayla Khan, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
 ******************************************************************************/

/*
 * $Id$
 *
 * Quaternion math ported from Perl to C
 * (originally in Quaternion.pm)
 *
 * VRML rotation representation:
 *
 *  axis (x, y, z) and angle (radians), default is unit vector = (0, 0, 1)
 *  and angle = 0 (see VRML97 spec. clauses 4.4.5 'Standard units and
 *  coordinate system', 5.8 'SFRotation and MFRotation')
 *
 * Quaternion representation:
 *
 * q = w + xi + yj + zk or q = (w, v) where w is a scalar and
 * v = (x, y, z) is a vector
 *
 * Quaternion addition:
 *
 * q1 + q2 = (w1 + w2, v1 + v2)
 *         = (w1 + w2) + (x1 + x2)i + (y1 + y2)j + (z1 + z2)k
 *
 * Quaternion multiplication
 *  (let the dot product of v1 and v2 be v1 dp v2):
 *
 * q1q2 = (w1, v1) dp (w2, v2)
 *      = (w1w2 - v1 dp v2, w1v2 + w2v1 + v1 x v2)
 * q1q2 != q2q1 (not commutative)
 *
 *
 * norm(q) = || q || = sqrt(w^2 + x^2 + y^2 + z^2) is q's magnitude
 * normalize a quaternion: q' = q / || q ||
 *
 * conjugate of q = q* = (w, -v)
 * inverse of q = q^-1 = q* / || q ||
 * unit quaternion: || q || = 1, w^2 + x^2 + y^2 + z^2 = 1, q^-1 = q*
 *
 * Identity quaternions: q = (1, (0, 0, 0)) is the multiplication
 *  identity and q = (0, (0, 0, 0)) is the addition identity
 *
 * References:
 *
 * * www.gamedev.net/reference/programming/features/qpowers/
 * * www.gamasutra.com/features/19980703/quaternions_01.htm
 * * mathworld.wolfram.com/
 * * skal.planet-d.net/demo/matrixfaq.htm
 */


#include "quaternion.h"


/*
 * VRML rotation (axis, angle) to quaternion (q = (w, v)):
 *
 * To simplify the math, the rotation vector needs to be normalized.
 *
 * q.w = cos(angle / 2);
 * q.x = (axis.x / || axis ||) * sin(angle / 2)
 * q.y = (axis.y / || axis ||) * sin(angle / 2)
 * q.z = (axis.z / || axis ||) * sin(angle / 2)
 *
 * Normalize quaternion: q /= ||q ||
 */

void
vrmlrot_to_quaternion(Quaternion *quat, const double x, const double y, const double z, const double a)
{
	double s;
	double scale = sqrt((x * x) + (y * y) + (z * z));

	/* no rotation - use (multiplication ???) identity quaternion */
	if (APPROX(scale, 0)) {
		quat->w = 1;
		quat->x = 0;
		quat->y = 0;
		quat->z = 0;

	} else {
		s = sin(a/2);
		/* normalize rotation axis to convert VRML rotation to quaternion */
		quat->w = cos(a / 2);
		quat->x = s * (x / scale);
		quat->y = s * (y / scale);
		quat->z = s * (z / scale);
		normalize(quat);
	}
}

/*
 * Quaternion (q = (w, v)) to VRML rotation (axis, angle):
 *
 * angle = 2 * acos(q.w)
 * axis.x = q.x / scale
 * axis.y = q.y / scale
 * axis.z = q.z / scale
 *
 * unless scale = 0, in which case, we'll use the default VRML
 * rotation
 *
 * One can use either scale = sqrt(q.x^2 + q.y^2 + q.z^2) or
 * scale = sin(acos(q.w)).
 * Since we are using unit quaternions, 1 = w^2 + x^2 + y^2 + z^2.
 * Also, acos(x) = asin(sqrt(1 - x^2)) (for x >= 0, but since we don't
 * actually compute asin(sqrt(1 - x^2)) let's not worry about it).
 * scale = sin(acos(q.w)) = sin(asin(sqrt(1 - q.w^2)))
 *       = sqrt(1 - q.w^2) = sqrt(q.x^2 + q.y^2 + q.z^2)
 */
void
quaternion_to_vrmlrot(const Quaternion *quat, double *x, double *y, double *z, double *a)
{
	double scale = sqrt(VECSQ(*quat));
	if (APPROX(scale, 0)) {
		*x = 0;
		*y = 0;
		*z = 1;
		*a = 0;
	} else {
		*x = quat->x / scale;
		*y = quat->y / scale;
		*z = quat->z / scale;
		*a = 2 * acos(quat->w);
	}
}

void
conjugate(Quaternion *quat)
{
	quat->x *= -1;
	quat->y *= -1;
	quat->z *= -1;
}

void
inverse(Quaternion *ret, const Quaternion *quat)
{
/* 	double n = norm(quat); */

	set(ret, quat);
	conjugate(ret);

	/* unit quaternion, so take conjugate */
	normalize(ret);
/* 	printf("Quaternion inverse: ret = {%f, %f, %f, %f}, quat = {%f, %f, %f, %f}\n", */
/* 		   ret->w, ret->x, ret->y, ret->z, quat->w, quat->x, quat->y, quat->z); */
}

double
norm(const Quaternion *quat)
{
	return(sqrt(
				quat->w * quat->w +
				quat->x * quat->x +
				quat->y * quat->y +
				quat->z * quat->z
				));
}

void
normalize(Quaternion *quat)
{
	double n = norm(quat);
	if (APPROX(n, 1)) {
		return;
	}
	quat->w /= n;
	quat->x /= n;
	quat->y /= n;
	quat->z /= n;
}

void
add(Quaternion *ret, const Quaternion *q1, const Quaternion *q2)
{
	ret->w = q1->w + q2->w;
	ret->x = q1->x + q2->x;
	ret->y = q1->y + q2->y;
	ret->z = q1->z + q2->z;
}

void
multiply(Quaternion *ret, const Quaternion *q1, const Quaternion *q2)
{
	ret->w = (q1->w * q2->w) - (q1->x * q2->x) - (q1->y * q2->y) - (q1->z * q2->z);
	ret->x = (q1->w * q2->x) + (q1->x * q2->w) + (q1->y * q2->z) - (q1->z * q2->y);
	ret->y = (q1->w * q2->y) + (q1->y * q2->w) - (q1->x * q2->z) + (q1->z * q2->x);
	ret->z = (q1->w * q2->z) + (q1->z * q2->w) + (q1->x * q2->y) - (q1->y * q2->x);
/* 	printf("Quaternion multiply: ret = {%f, %f, %f, %f}, q1 = {%f, %f, %f, %f}, q2 = {%f, %f, %f, %f}\n", ret->w, ret->x, ret->y, ret->z, q1->w, q1->x, q1->y, q1->z, q2->w, q2->x, q2->y, q2->z); */
}

void
scalar_multiply(Quaternion *quat, double s)
{
	quat->w *= s;
	quat->x *= s;
	quat->y *= s;
	quat->z *= s;
}

/*
 * Rotate vector v by unit quaternion q:
 *
 * v' = q q_v q^-1, where q_v = [0, v]
 */
void
rotation(struct pt *ret, const Quaternion *quat, const struct pt *v)
{
	Quaternion q_v = {0, v->x, v->y, v->z}, q_i, q_r1, q_r2;

	inverse(&q_i, quat);
	multiply(&q_r1, &q_v, &q_i);
	multiply(&q_r2, quat, &q_r1);

	ret->x = q_r2.x;
	ret->y = q_r2.y;
	ret->z = q_r2.z;
/* 	printf("Quaternion rotation: ret = {%f, %f, %f}, quat = {%f, %f, %f, %f}, v = {%f, %f, %f}\n", ret->x, ret->y, ret->z, quat->w, quat->x, quat->y, quat->z, v->x, v->y, v->z); */
}


void
togl(Quaternion *quat)
{
	if (APPROX(fabs(quat->w), 1)) { return; }

	if (quat->w > 1) { normalize(quat); }

	/* get the angle, but turn us around 180 degrees */
	//printf ("togl: setting rotation %f %f %f %f\n",(2 * (acos(quat->w) / PI * 180)),-quat->x, -quat->y, -quat->z);
	glRotated((2 * (acos(quat->w) / PI * 180)), -quat->x, -quat->y, -quat->z);
}

void
set(Quaternion *ret, const Quaternion *quat)
{
	ret->w = quat->w;
	ret->x = quat->x;
	ret->y = quat->y;
	ret->z = quat->z;
}

/* unused - JAS.
double abssq(Quaternion *quat) {
	return (quat->w * quat->w) +
	       (quat->x * quat->x) +
	       (quat->y * quat->y) +
	       (quat->z * quat->z);
}

void
quat_invert (Quaternion *quat) {
	double absq;

	absq = abssq(quat);

	quat->w *=  1/absq;
	quat->x *= -1/absq;
	quat->y *= -1/absq;
	quat->z *= -1/absq;
}
*/
	
/*
 * Code from www.gamasutra.com/features/19980703/quaternions_01.htm,
 * Listing 5.
 *
 * SLERP(p, q, t) = [p sin((1 - t)a) + q sin(ta)] / sin(a)
 *
 * where a is the arc angle, quaternions pq = cos(q) and 0 <= t <= 1
 */
void
slerp(Quaternion *ret, const Quaternion *q1, const Quaternion *q2, const double t)
{
	double omega, cosom, sinom, scale0, scale1, q2_array[4];

	cosom =
		q1->x * q2->x +
		q1->y * q2->y +
		q1->z * q2->z +
		q1->w * q2->w;

	if (cosom < 0.0) {
		cosom = -cosom;
		q2_array[0] = -q2->x;
		q2_array[1] = -q2->y;
		q2_array[2] = -q2->z;
		q2_array[3] = -q2->w;
	} else {
		q2_array[0] = q2->x;
		q2_array[1] = q2->y;
		q2_array[2] = q2->z;
		q2_array[3] = q2->w;
	}

	/* calculate coefficients */
	if ((1.0 - cosom) > DELTA) {
		/* standard case (SLERP) */
		omega = acos(cosom);
		sinom = sin(omega);
		scale0 = sin((1.0 - t) * omega) / sinom;
		scale1 = sin(t * omega) / sinom;
	} else {
		/* q1 & q2 are very close, so do linear interpolation */
		scale0 = 1.0 - t;
		scale1 = t;
	}
	ret->x = scale0 * q1->x + scale1 * q2_array[0];
	ret->y = scale0 * q1->y + scale1 * q2_array[1];
	ret->z = scale0 * q1->z + scale1 * q2_array[2];
	ret->w = scale0 * q1->w + scale1 * q2_array[3];
}
