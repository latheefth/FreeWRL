/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*****************************************

SensInterps.c - do Sensors and Interpolators in C, not in perl.

Interps are the "EventsProcessed" fields of interpolators.

******************************************/


#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#ifdef AQUA 
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "Structs.h"
#include "headers.h"
#include "LinearAlgebra.h"


/* local routine, look for the appropriate key */
int find_key (int kin, float frac, float *keys) {
	int counter;

	for (counter=1; counter <= kin; counter++) {
		if (frac <keys[counter]) {
			return counter;
		}
	}
	return kin;	/* huh? not found! */
}


/* ScalarInterpolators - return only one float */
do_OintScalar (struct VRML_ScalarInterpolator *px) {
	/* ScalarInterpolator - store final value in px->value_changed */
	int kin = px->key.n;
	int kvin = px->keyValue.n;
	float * kVs = px->keyValue.p;
	int counter;

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0) || (kin>kvin)) {
		px->value_changed = 0.0;
		return;
	}

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= px->key.p[0]) {
		 px->value_changed = kVs[0];
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		 px->value_changed = kVs[kvin-1];
	} else {
		/* have to go through and find the key before */
		counter=find_key(kin,px->set_fraction,px->key.p);
		px->value_changed =
			(px->set_fraction - px->key.p[counter-1]) /
			(px->key.p[counter] - px->key.p[counter-1]) *
			(kVs[counter] - kVs[counter-1]) +
			kVs[counter-1];
	}
}


do_OintCoord(struct VRML_CoordinateInterpolator *px, int indx) {
	int kin = px->key.n;
	int kvin = px->keyValue.n;
	struct SFColor * kVs = px->keyValue.p;
	int thisone, prevone;	/* which keyValues we are interpolating between */
	int tmp;
	float interval;		/* where we are between 2 values */
	struct pt normalval;	/* different structures for normalization calls */

	int kpkv; /* keys per key value */

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0) || (kin>kvin)) {
		px->_this_value.c[0] = 0.0;
		px->_this_value.c[1] = 0.0;
		px->_this_value.c[2] = 0.0;
		return;
	}
	kpkv = kvin/kin;

	//printf ("CoordinateInterpolator, kpkv %d index %d ",kpkv,indx);

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= px->key.p[0]) {
		 px->_this_value.c[0] = kVs[0+indx].c[0];
		 px->_this_value.c[1] = kVs[0+indx].c[1];
		 px->_this_value.c[2] = kVs[0+indx].c[2];
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		 px->_this_value.c[0] = kVs[(kvin-1)*kpkv+indx].c[0];
		 px->_this_value.c[1] = kVs[(kvin-1)*kpkv+indx].c[1];
		 px->_this_value.c[2] = kVs[(kvin-1)*kpkv+indx].c[2];
	} else {
		/* have to go through and find the key before */
		/* we process, but only for first valueperkey */
		if (indx == 0) {
			//printf ("indx=0, kin %d frac %f\n",kin,px->set_fraction);
			px->_counter=find_key(kin,px->set_fraction,px->key.p);
		}

		/* find the fraction between the 2 values */
		thisone = px->_counter;
		interval = (px->set_fraction - px->key.p[thisone-1]) /
				(px->key.p[thisone] - px->key.p[thisone-1]);

		thisone = thisone * kpkv + indx;
		prevone = (px->_counter-1) * kpkv + indx;

		if (thisone >= kvin) {
			printf ("CoordinateInterpolator error: thisone %d prevone %d indx %d kpkv %d kin %d kvin %d\n",thisone,prevone,
			indx,kpkv,kin,kvin);
		}

		for (tmp=0; tmp<3; tmp++) {
			px->_this_value.c[tmp] = kVs[prevone].c[tmp]  +
					interval * (kVs[thisone].c[tmp] -
						kVs[prevone].c[tmp]);
		}

	}

	/* if this is a NormalInterpolator... */
        if (px->_type==1) {
		normalval.x = px->_this_value.c[0];
		normalval.y = px->_this_value.c[1];
		normalval.z = px->_this_value.c[2];
                normalize_vector(&normalval);
		px->_this_value.c[0] = normalval.x;
		px->_this_value.c[1] = normalval.y;
		px->_this_value.c[2] = normalval.z;
        }
}


/* PositionInterpolator, ColorInterpolator		 		*/
/* Called during the "events_processed" section of the event loop,	*/
/* so this is called ONLY when there is something required to do, thus	*/
/* there is no need to look at whether it is active or not		*/

do_Oint3 (struct VRML_PositionInterpolator *px) {
	/* PositionInterpolator - store final value in px->value_changed */
	int kin = px->key.n;
	int kvin = px->keyValue.n;
	struct SFColor *kVs = px->keyValue.p;
	int counter;
	int tmp;

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0) || (kin>kvin)) {
		px->value_changed.c[0] = 0.0;
		px->value_changed.c[1] = 0.0;
		px->value_changed.c[2] = 0.0;
		return;
	}

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		 px->value_changed.c[0] = kVs[0].c[0];
		 px->value_changed.c[1] = kVs[0].c[1];
		 px->value_changed.c[2] = kVs[0].c[2];
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		 px->value_changed.c[0] = kVs[kvin-1].c[0];
		 px->value_changed.c[1] = kVs[kvin-1].c[1];
		 px->value_changed.c[2] = kVs[kvin-1].c[2];
	} else {
		/* have to go through and find the key before */
		counter = find_key(kin,px->set_fraction,px->key.p);
		for (tmp=0; tmp<3; tmp++) {
			px->value_changed.c[tmp] =
				(px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]) *
				(kVs[counter].c[tmp] - 
					kVs[counter-1].c[tmp]) +
				kVs[counter-1].c[tmp];
		}
	}
}

/* OrientationInterpolator				 		*/
/* Called during the "events_processed" section of the event loop,	*/
/* so this is called ONLY when there is something required to do, thus	*/
/* there is no need to look at whether it is active or not		*/

do_Oint4 (struct VRML_OrientationInterpolator *px) {
	int kin = ((px->key).n);
	int kvin = ((px->keyValue).n);
	struct SFRotation *kVs = ((px->keyValue).p);
	int counter;
	int tmp;
	float testangle;	/* temporary variable 	*/
	float oldangle;		/* keyValue where we are moving from */
	float newangle;		/* keyValue where we are moving to */
	float interval;		/* where we are between 2 values */

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0) || (kin>kvin)) {
		px->value_changed.r[0] = 0.0;
		px->value_changed.r[1] = 0.0;
		px->value_changed.r[2] = 0.0;
		px->value_changed.r[3] = 0.0;
		return;
	}

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		 px->value_changed.r[0] = kVs[0].r[0];
		 px->value_changed.r[1] = kVs[0].r[1];
		 px->value_changed.r[2] = kVs[0].r[2];
		 px->value_changed.r[3] = kVs[0].r[3];
	} else if (px->set_fraction >= ((px->key).p[kin-1])) {
		 px->value_changed.r[0] = kVs[kvin-1].r[0];
		 px->value_changed.r[1] = kVs[kvin-1].r[1];
		 px->value_changed.r[2] = kVs[kvin-1].r[2];
		 px->value_changed.r[3] = kVs[kvin-1].r[3];
	} else {
		counter = find_key(kin,px->set_fraction,px->key.p);
		interval = (px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]);

		/* are there any -1s in there? */
		testangle = 0.0;
		for (tmp=0; tmp<3; tmp++) {
			testangle+= kVs[counter].r[tmp]*kVs[counter-1].r[tmp];
		}

		/* do the angles */
		if (testangle >= 0.0) {
			for (tmp=0; tmp<3; tmp++) {
				px->value_changed.r[tmp] = kVs[counter-1].r[tmp]  +
					interval * (kVs[counter].r[tmp] -
						kVs[counter-1].r[tmp]);
			}
			 newangle = kVs[counter].r[3];
		} else {
			for (tmp=0; tmp<3; tmp++) {
				px->value_changed.r[tmp] = kVs[counter-1].r[tmp]  +
					interval * (-kVs[counter].r[tmp] -
						kVs[counter-1].r[tmp]);
			}
			newangle = -kVs[counter].r[3];
		}
		oldangle = kVs[counter-1].r[3];
		testangle = newangle-oldangle;

					
		if (fabs(testangle) > PI) {
			if (testangle>0.0) {
				oldangle += PI*2;
			} else {
				newangle += PI*2;
			}
		}


		/* now that we have angles straight (hah!) bounds check result */
		newangle = oldangle + interval*(newangle-oldangle);
		if (newangle> PI*2) {
			newangle -= PI*2;
		} else {
			if (newangle<PI*2) {
				newangle += PI*2;
			}
		}

		px->value_changed.r[3]=newangle;
	}
}
