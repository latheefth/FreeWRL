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


/* returns the audio duration, unscaled by pitch */
float return_Duration (int indx) {
	float retval;

	if (indx < 0)  retval = 1.0;
	else if (indx > 50) retval = 1.0;
	else retval = AC_LastDuration[indx];
	return retval;
}

/* time dependent sensor nodes- check/change activity state */
void do_active_inactive (
	int *act, 		/* pointer to are we active or not?	*/
	double *inittime,	/* pointer to nodes inittime		*/
	double *startt,		/* pointer to nodes startTime		*/
	double *stopt,		/* pointer to nodes stop time		*/
	double tick,		/* time tick				*/
	int loop,		/* nodes loop field			*/
	float myDuration,	/* duration of cycle			*/
	float speed		/* speed field				*/
) {

	/* what we do now depends on whether we are active or not */

	if (*act == 1) {   /* active - should we stop? */
		/* printf ("is active\n"); */

		if (tick > *stopt) {
			if (*startt >= *stopt) { 
				/* cases 1 and 2 */
				if (!(loop)) {
					if (speed != 0) {
					    if (tick >= (*startt + 
							abs(myDuration/speed))) {
						/* printf ("stopping case x\n"); */
						*act = 0;
						*stopt = tick;
					    }
					}
				/*
				} else {
				#	print "stopping case y\n";
				#	node->{isActive} = 0;
				#	node->{stopTime} = $tick;
				*/
				}
			} else {
				/* printf ("stopping case z\n");  */
				*act = 0;
				*stopt = tick;
			}
		}
	}

	/* immediately process start events; as per spec.  */
	if (*act == 0) {   /* active - should we start? */
		/* printf ("is not active tick %f startt %f\n",tick,*startt); */

		if (tick >= *startt) {
			/* We just might need to start running */

			if (tick >= *stopt) {
				/* lets look at the initial conditions; have not had a stoptime
				event (yet) */

				if (loop) {
					if (*startt >= *stopt) {
						/* VRML standards, table 4.2 case 2 */
						/* printf ("CASE 2\n"); */
						*startt = tick;
						*act = 1;
					}
				} else if (*startt >= *stopt) {
					if (*startt > *inittime) { 
						/* ie, we have an event */
						 /* printf ("case 1 here\n"); */
						/*
						we should be running 
						VRML standards, table 4.2 case 1 
						*/
						*startt = tick;
						*act = 1;
					}
				}
			} else {
				/* printf ("case 3 here\n"); */
				/* we should be running -  
				VRML standards, table 4.2 cases 1 and 2 and 3 */
				*startt = tick;
				*act = 1;
			}
		}
	}
}


/* Interpolators - local routine, look for the appropriate key */
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

	/* printf ("CoordinateInterpolator, kpkv %d index %d ",kpkv,indx); */

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


/* Audio AudioClip sensor code */
void do_AudioTick(struct VRML_AudioClip *node, double tick,int *doevent) {
	int 	oldstatus;	

	/* assume no event from this node */
	*doevent = 0;

	/* can we possibly have started yet? */
	if(tick < node->startTime) {
		return;
	}

	oldstatus = node->isActive;

	/* call common time sensor routine */
	do_active_inactive (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,tick,node->loop,return_Duration(node->__sourceNumber),
		node->pitch);
	

	if (oldstatus != node->isActive) {
		/* push @e, [$t, "isActive", node->{isActive}]; */
		*doevent = 1;
		/* tell SoundEngine that this source has changed.  */
	        if (!SoundEngineStarted) {
        	        /* printf ("SetAudioActive: initializing SoundEngine\n"); */
                	SoundEngineStarted = TRUE;
                	SoundEngineInit();
		}
        	SetAudioActive (node->__sourceNumber,node->isActive);
	}

	if(node->isActive == 1) {
		/* VRML::OpenGL::set_render_frame(); */
	}
}


void do_TimeSensorTick (struct VRML_TimeSensor *node, double tick, int *doActevent, 
		int *doCtevent, int *doTsevent, double *retfrac) {
	double myDuration;
	int oldstatus;
	double myTime;
	double frac;

	/* assume no event from this node */
	*doActevent = 0;
	*doCtevent = 0;
	*doTsevent = 0;

	/* can we possibly have started yet? */
	if(tick < node->startTime) {
		return;
	}

	oldstatus = node->isActive;
	myDuration = node->cycleInterval;

	/* call common time sensor routine */
	do_active_inactive (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,tick,node->loop,1.0, 1.0);


	/* now process if we have changed states */
	if (oldstatus != node->isActive) {
		if (node->isActive == 1) {
			/* force code below to generate event */
			node->__ctflag = 10.0;
		}

		/* push @e, [$t, "isActive", node->{isActive}]; */
		*doActevent = 1;
	}


	if(node->isActive == 1) {
			/* calculate what fraction we should be */
	 		myTime = (tick - node->startTime) / myDuration;

			if (node->loop) {
				frac = myTime - (int) myTime;
			} else {
				frac = (myTime > 1 ? 1 : myTime);
			}

			/* cycleTime events once at start, and once every loop. */
			if (frac < node->__ctflag) {
				*doCtevent = 1;
                               	/* push @e, [$t, cycleTime, $tick]; */
			}
			node->__ctflag = frac;
	
			/* time  and fraction_changed events */
			*doTsevent = 1;
			*retfrac = frac;
			/* push @e, [$t, "time", $tick];
			push @e, [$t, fraction_changed, $frac]; */
	}
}
