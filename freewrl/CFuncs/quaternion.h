/******************************************************************************
 Copyright (C) 1998 Tuomas J. Lukka, 2003 John Stewart, Ayla Khan, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*******************************************************************************/

/*
 * $Id$
 *
 */

#ifndef __QUATERNION_H__
#define __QUATERNION_H__

/*
 * #include "EXTERN.h"
 * #include "perl.h"
 * #include "XSUB.h"
 */
#include "headers.h"
#include "Structs.h"
#include "LinearAlgebra.h"

#include <GL/gl.h>
#include <math.h>

typedef struct quaternion {
	double w;
	double x;
	double y;
	double z;
} Quaternion;

void
vrmlrot_to_quaternion(
					  Quaternion *quat,
					  const double x,
					  const double y,
					  const double z,
					  const double a
					  );

void
quaternion_to_vrmlrot(
					  const Quaternion *quat,
					  double *x,
					  double *y,
					  double *z,
					  double *a
					  );

void
conjugate(Quaternion *quat);

void
inverse(
		Quaternion *ret,
		const Quaternion *quat
		);

double
norm(const Quaternion *quat);

void
normalize(Quaternion *quat);

void
add(
	Quaternion *ret,
	const Quaternion *q1,
	const Quaternion *q2
	);

void
multiply(
		 Quaternion *ret,
		 const Quaternion *q1,
		 const Quaternion *q2
		 );

void
scalar_multiply(
				Quaternion *quat,
				const double s
				);

void
rotation(
			struct pt *ret,
			const Quaternion *quat,
			const struct pt *v
			);

void
togl(Quaternion *quat);

/* void */
/* slerp(); */

#endif /* __QUATERNION_H__ */
