
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


/*******************************************************************

	X3D Interpolation Component

*********************************************************************/


#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>

#include "../vrml_parser/Structs.h"
#include "../input/InputFunctions.h"
#include "../opengl/LoadTextures.h"        /* for finding a texture url in a multi url */


#include "../main/headers.h"
#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/RenderFuncs.h"

#include "../x3d_parser/Bindable.h"
#include "../scenegraph/LinearAlgebra.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/sounds.h"
#include "../vrml_parser/CRoutes.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"            /* for finding a texture url in a multi url */

#include "../input/SensInterps.h"


// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html
// see also SenseInterps.c for other interpolator componenent implementations

void do_EaseInEaseOut(void *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#EaseInEaseOut
}
void do_SplinePositionInterpolator(void *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#SplinePositionInterpolator
}
void do_SplinePositionInterpolator2D(void *node){
}
void do_SplineScalarInterpolator(void *node){
}
void do_SquadOrientationInterpolator(void *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#SquadOrientationInterpolator
	// Squad research:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb281656(v=vs.85).aspx
	// - Squad interp using slerps
	//   Slerp(Slerp(q1, c, t), Slerp(a, b, t), 2t(1 - t))
	// https://theory.org/software/qfa/writeup/node12.html
	// - Also squad via slerp
	//   squad(b0, S1, S2, b3, u) = slerp( slerp(b0,b3,u), slerp(S1, S2, u), 2u(1-u))
	// http://run.usc.edu/cs520-s13/assign2/p245-shoemake.pdf
	// - SHOE paper refered to by web3d specs, 'un-readable jibberish' (dug9)
	// http://web.mit.edu/2.998/www/QuaternionReport1.pdf  
	// - page 51 explains SHOE, 
	// qinterp = Squad(qi, qi+1,si,si+1,h) = Slerp( Slerp(qi,qi+1,h), Slerp(si,si+1,h), 2h(1-h))
	//		where si = qi * exp( - (log(inv(qi)*qi+1) + log(inv(qi)*qi-1)) / 4 )
	//		and qi+1 means q[i+1] and qi-1 means q[i-1] ie that's an indexer, not a quat + scalar
	//	p. 15 shows log(q) if q=[cost,sint*v] then 
	//			log(q) = [0, sint*v] (non-unit)
	//			exp(q) = [cost, sint*v] (puts cost back in scalar part, inverting log)
	//	July 19, 2016 - code below copied from orientationInterpolator and node caste changed, but no other changes
	//  - feel free to implement squad in here


	struct X3D_SquadOrientationInterpolator *px;
	int kin, kvin;
	struct SFRotation *kVs;
	int counter;
	float interval;		/* where we are between 2 values */
	// UNUSED?? int stzero;
	// UNUSED?? int endzero;	/* starting and/or ending angles zero? */

	Quaternion st, fin, final;
	double x,y,z,a;

	if (!node) return;
	px = (struct X3D_SquadOrientationInterpolator *) node;
	kin = ((px->key).n);
	kvin = ((px->keyValue).n);
	kVs = ((px->keyValue).p);

	#ifdef SEVERBOSE
	printf ("starting do_Oint4; keyValue count %d and key count %d\n",
				kvin, kin);
	#endif


	MARK_EVENT (node, offsetof (struct X3D_SquadOrientationInterpolator, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.c[0] = (float) 0.0;
		px->value_changed.c[1] = (float) 0.0;
		px->value_changed.c[2] = (float) 0.0;
		px->value_changed.c[3] = (float) 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */


	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[0], sizeof (struct SFRotation));
	} else if (px->set_fraction >= ((px->key).p[kin-1])) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[kvin-1], sizeof (struct SFRotation));
	} else {
		counter = find_key(kin,(float)(px->set_fraction),px->key.p);
		interval = (px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]);

		
		/* are either the starting or ending angles zero? */
		// unused? stzero = APPROX(kVs[counter-1].c[3],0.0);
		// unused? endzero = APPROX(kVs[counter].c[3],0.0);
		#ifdef SEVERBOSE
			printf ("counter %d interval %f\n",counter,interval);
			printf ("angles %f %f %f %f, %f %f %f %f\n",
				kVs[counter-1].c[0],
				kVs[counter-1].c[1],
				kVs[counter-1].c[2],
				kVs[counter-1].c[3],
				kVs[counter].c[0],
				kVs[counter].c[1],
				kVs[counter].c[2],
				kVs[counter].c[3]);
		#endif
		vrmlrot_to_quaternion (&st, kVs[counter-1].c[0],
                                kVs[counter-1].c[1], kVs[counter-1].c[2], kVs[counter-1].c[3]);
		vrmlrot_to_quaternion (&fin,kVs[counter].c[0],
                                kVs[counter].c[1], kVs[counter].c[2], kVs[counter].c[3]);

		quaternion_slerp(&final, &st, &fin, (double)interval);
		quaternion_to_vrmlrot(&final,&x, &y, &z, &a);
		px->value_changed.c[0] = (float) x;
		px->value_changed.c[1] = (float) y;
		px->value_changed.c[2] = (float) z;
		px->value_changed.c[3] = (float) a;

		#ifdef SEVERBOSE
		printf ("Oint, new angle %f %f %f %f\n",px->value_changed.c[0],
			px->value_changed.c[1],px->value_changed.c[2], px->value_changed.c[3]);
		#endif
	}

}