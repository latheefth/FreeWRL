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

int SEVerbose = 0;

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
		if (SEVerbose) printf ("is active\n"); 

		if (tick > *stopt) {
			if (*startt >= *stopt) { 
				/* cases 1 and 2 */
				if (!(loop)) {
					if (speed != 0) {
					    if (tick >= (*startt + 
							fabs(myDuration/speed))) {
						if (SEVerbose) printf ("stopping case x\n");
			printf ("tick %f startt %f myD %f speed %f fabs %f\n",
					tick, *startt, myDuration, speed, fabs(myDuration/speed));
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
				if (SEVerbose) printf ("stopping case z\n");
				*act = 0;
				*stopt = tick;
			}
		}
	}

	/* immediately process start events; as per spec.  */
	if (*act == 0) {   /* active - should we start? */
		/* if (SEVerbose) printf ("is not active tick %f startt %f\n",tick,*startt); */

		if (tick >= *startt) {
			/* We just might need to start running */

			if (tick >= *stopt) {
				/* lets look at the initial conditions; have not had a stoptime
				event (yet) */

				if (loop) {
					if (*startt >= *stopt) {
						/* VRML standards, table 4.2 case 2 */
						/* if (SEVerbose) printf ("CASE 2\n"); */
						*startt = tick;
						*act = 1;
					}
				} else if (*startt >= *stopt) {
					if (*startt > *inittime) { 
						/* ie, we have an event */
						 /* if (SEVerbose) printf ("case 1 here\n"); */
						/*
						we should be running 
						VRML standards, table 4.2 case 1 
						*/
						*startt = tick;
						*act = 1;
					}
				}
			} else {
				/* if (SEVerbose) printf ("case 3 here\n"); */
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
void do_OintScalar (void *node) {
	/* ScalarInterpolator - store final value in px->value_changed */
	struct VRML_ScalarInterpolator *px;
	int kin, kvin;
	float *kVs;
	int counter;

	px = (struct VRML_ScalarInterpolator *) node;
	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;

	mark_event ((unsigned int) px, offsetof (struct VRML_ScalarInterpolator, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed = 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */

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


void do_OintCoord(void *node) {
	struct VRML_CoordinateInterpolator *px;
	int kin, kvin, counter;
	struct SFColor *kVs;
	struct SFColor *valchanged;

	int thisone, prevone;	/* which keyValues we are interpolating between */
	int tmp;
	float interval;		/* where we are between 2 values */
	struct pt normalval;	/* different structures for normalization calls */
	int kpkv; /* keys per key value */
	int indx;
	int myKey;
	
	px = (struct VRML_CoordinateInterpolator *) node;


	if (SEVerbose) 
		printf ("debugging OintCoord keys %d kv %d vc %d\n",px->keyValue.n, px->key.n,px->value_changed.n);
	
	mark_event ((unsigned int) px, offsetof (struct VRML_CoordinateInterpolator, value_changed));

	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;
	kpkv = kvin/kin;



	/* do we need to (re)allocate the value changed array? */
	if (kpkv != px->value_changed.n) {
		/*printf ("refactor valuechanged array. n %d sizeof p %d\n",
			kpkv,sizeof (struct SFColor) * kpkv); */
		if (px->value_changed.n != 0) {
			free (px->value_changed.p);
		}
		px->value_changed.n = kpkv;
		px->value_changed.p = malloc (sizeof (struct SFColor) * kpkv);
	}

	/* shortcut valchanged; have to put it here because might be remalloc'd */
	valchanged = px->value_changed.p;


	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		if (SEVerbose) printf ("no keys or keyValues yet\n");
		for (indx = 0; indx < kpkv; indx++) {
			valchanged[indx].c[0] = 0.0;
			valchanged[indx].c[1] = 0.0;
			valchanged[indx].c[2] = 0.0;
		}		
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */


	if (SEVerbose) {
		printf ("debugging, kpkv %d, px->value_changed.n %d\n", kpkv, px->value_changed.n);	
		printf ("CoordinateInterpolator, kpkv %d index %d\n",kpkv,indx);
	}

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= px->key.p[0]) {
		if (SEVerbose) printf ("COINT out1\n");
		for (indx = 0; indx < kpkv; indx++) {
			memcpy ((void *)&valchanged[indx], 
				(void *)&kVs[indx], sizeof (struct SFColor));
			//JAS valchanged[indx].c[0] = kVs[indx].c[0];
			//JAS valchanged[indx].c[1] = kVs[indx].c[1];
			//JAS valchanged[indx].c[2] = kVs[indx].c[2];
		}
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		if (SEVerbose) printf ("COINT out1\n");
		for (indx = 0; indx < kpkv; indx++) {
			memcpy ((void *)&valchanged[indx], 
				(void *)&kVs[(kvin-1)*kpkv+indx], 
				sizeof (struct SFColor));
			//JAS valchanged[indx].c[0] = kVs[(kvin-1)*kpkv+indx].c[0];
			//JAS valchanged[indx].c[1] = kVs[(kvin-1)*kpkv+indx].c[1];
			//JAS valchanged[indx].c[2] = kVs[(kvin-1)*kpkv+indx].c[2];
		}
	} else {
		if (SEVerbose) printf ("COINT out1\n");
		/* have to go through and find the key before */
		if (SEVerbose) printf ("indx=0, kin %d frac %f\n",kin,px->set_fraction);

		myKey=find_key(kin,px->set_fraction,px->key.p);
		if (SEVerbose) printf ("working on key %d\n",myKey);

		/* find the fraction between the 2 values */
		interval = (px->set_fraction - px->key.p[myKey-1]) /
				(px->key.p[myKey] - px->key.p[myKey-1]);

		for (indx = 0; indx < kpkv; indx++) {
			thisone = myKey * kpkv + indx;
			prevone = (myKey-1) * kpkv + indx;


			if (thisone >= kvin) {
				if (SEVerbose) printf ("CoordinateInterpolator error: thisone %d prevone %d indx %d kpkv %d kin %d kvin %d\n",thisone,prevone,
				indx,kpkv,kin,kvin);
			}

			for (tmp=0; tmp<3; tmp++) {
				valchanged[indx].c[tmp] = kVs[prevone].c[tmp]  +
						interval * (kVs[thisone].c[tmp] -
							kVs[prevone].c[tmp]);
			}
		}

	}

	/* if this is a NormalInterpolator... */
        if (px->_type==1) {
		for (indx = 0; indx < kpkv; indx++) {
			normalval.x = valchanged[indx].c[0];
			normalval.y = valchanged[indx].c[1];
			normalval.z = valchanged[indx].c[2];
			normalize_vector(&normalval);
			valchanged[indx].c[0] = normalval.x;
			valchanged[indx].c[1] = normalval.y;
			valchanged[indx].c[2] = normalval.z;
		}
        }
	if (SEVerbose) printf ("Done CoordinateInterpolator\n");
}


/* PositionInterpolator, ColorInterpolator		 		*/
/* Called during the "events_processed" section of the event loop,	*/
/* so this is called ONLY when there is something required to do, thus	*/
/* there is no need to look at whether it is active or not		*/

void do_Oint3 (void *node) {
	/* PositionInterpolator - store final value in px->value_changed */
	struct VRML_PositionInterpolator *px;
	int kin, kvin, counter, tmp;
	struct SFColor *kVs;

	px = (struct VRML_PositionInterpolator *) node;

	mark_event ((unsigned int) px, offsetof (struct VRML_PositionInterpolator, value_changed));

	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.c[0] = 0.0;
		px->value_changed.c[1] = 0.0;
		px->value_changed.c[2] = 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */


	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed, 
				(void *)&kVs[0], sizeof (struct SFColor));
		 //JAS px->value_changed.c[0] = kVs[0].c[0];
		 //JAS px->value_changed.c[1] = kVs[0].c[1];
		 //JAS px->value_changed.c[2] = kVs[0].c[2];
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		memcpy ((void *)&px->value_changed, 
				(void *)&kVs[kvin-1], sizeof (struct SFColor));
		 //JAS px->value_changed.c[0] = kVs[kvin-1].c[0];
		 //JAS px->value_changed.c[1] = kVs[kvin-1].c[1];
		 //JAS px->value_changed.c[2] = kVs[kvin-1].c[2];
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

void do_Oint4 (void *node) {
	struct VRML_OrientationInterpolator *px;
	int kin, kvin;
	struct SFRotation *kVs;
	int counter;
	int tmp;
	float testangle;	/* temporary variable 	*/
	float oldangle;		/* keyValue where we are moving from */
	float newangle;		/* keyValue where we are moving to */
	float interval;		/* where we are between 2 values */

	px = (struct VRML_OrientationInterpolator *) node;
	kin = ((px->key).n);
	kvin = ((px->keyValue).n);
	kVs = ((px->keyValue).p);

if (SEVerbose) printf ("starting do_Oint4\n");

	mark_event ((unsigned int) px, offsetof (struct VRML_OrientationInterpolator, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.r[0] = 0.0;
		px->value_changed.r[1] = 0.0;
		px->value_changed.r[2] = 0.0;
		px->value_changed.r[3] = 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */


	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed, 
				(void *)&kVs[0], sizeof (struct SFRotation));
		 //JAS px->value_changed.r[0] = kVs[0].r[0];
		 //JAS px->value_changed.r[1] = kVs[0].r[1];
		 //JAS px->value_changed.r[2] = kVs[0].r[2];
		 //JAS px->value_changed.r[3] = kVs[0].r[3];
	} else if (px->set_fraction >= ((px->key).p[kin-1])) {
		memcpy ((void *)&px->value_changed, 
				(void *)&kVs[kvin-1], sizeof (struct SFRotation));
		 //JAS px->value_changed.r[0] = kVs[kvin-1].r[0];
		 //JAS px->value_changed.r[1] = kVs[kvin-1].r[1];
		 //JAS px->value_changed.r[2] = kVs[kvin-1].r[2];
		 //JAS px->value_changed.r[3] = kVs[kvin-1].r[3];
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
		if (SEVerbose) printf ("Oint, new angle %f %f %f %f\n",px->value_changed.r[0],
			px->value_changed.r[1],px->value_changed.r[2], px->value_changed.r[3]);
	}
}


/* Audio AudioClip sensor code */
void do_AudioTick(struct VRML_AudioClip *node, double tick) {
	int 	oldstatus;	

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
		mark_event ((unsigned int) node, offsetof(struct VRML_AudioClip, isActive));
		/* tell SoundEngine that this source has changed.  */
	        if (!SoundEngineStarted) {
        	        if (SEVerbose) printf ("SetAudioActive: initializing SoundEngine\n"); 
                	SoundEngineStarted = TRUE;
                	SoundEngineInit();
		}
        	SetAudioActive (node->__sourceNumber,node->isActive);
	}
}


void do_TimeSensorTick (struct VRML_TimeSensor *node, double tick) {
	double myDuration;
	int oldstatus;
	double myTime;
	double frac;

	/* are we not enabled */
	if (!node->enabled) {
		if (node->isActive) {
			node->isActive=0;
			mark_event ((unsigned int) node, offsetof(struct VRML_TimeSensor, isActive));
		}
		return;
	}

	/* can we possibly have started yet? */
	if(tick < node->startTime) {
		return;
	}

	oldstatus = node->isActive;
	myDuration = node->cycleInterval;

	/* call common time sensor routine */
	do_active_inactive (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,tick,node->loop,node->cycleTime, 1.0);


	/* now process if we have changed states */
	if (oldstatus != node->isActive) {
		if (node->isActive == 1) {
			/* force code below to generate event */
			node->__ctflag = 10.0;
		}

		/* push @e, [$t, "isActive", node->{isActive}]; */
		mark_event ((unsigned int) node, offsetof(struct VRML_TimeSensor, isActive));
	}


	if(node->isActive == 1) {
		/* calculate what fraction we should be */
 		myTime = (tick - node->startTime) / myDuration;

		if (node->loop) {
			frac = myTime - (int) myTime;
		} else {
			frac = (myTime > 1 ? 1 : myTime);
		}

		if (SEVerbose) printf ("TimeSensor myTime %f frac %f dur %f\n",
				myTime,frac,myDuration);

		/* cycleTime events once at start, and once every loop. */
		if (frac < node->__ctflag) {
			/* push @e, [$t, cycleTime, $tick]; */
			node->cycleTime = tick;
			mark_event ((unsigned int) node, offsetof(struct VRML_TimeSensor, cycleTime));
		}
		node->__ctflag = frac;
	
		/* time  and fraction_changed events */
		/* push @e, [$t, "time", $tick];
		push @e, [$t, fraction_changed, $frac]; */
		node->fraction_changed = frac;
		mark_event ((unsigned int) node, offsetof(struct VRML_TimeSensor, fraction_changed));
	}
}





/* Audio MovieTexture code */
void do_MovieTextureTick(struct VRML_MovieTexture *node, double tick) {
	int 	oldstatus;	
	float 	frac;		/* which texture to display */
	int 	highest,lowest;	/* selector variables		*/
	double myDuration;
	double myTime;
	float 	speed;
	float	duration;

	/* can we possibly have started yet? */
	if(tick < node->startTime) {
		return;
	}

	oldstatus = node->isActive;
	highest = node->__texture1_;
	lowest = node->__texture0_;
	duration = (highest - lowest)/30.0;
	speed = node->speed;


	/* call common time sensor routine */
	do_active_inactive (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,tick,node->loop,duration,speed);
	

	/* what we do now depends on whether we are active or not */
	if (oldstatus != node->isActive) {
		/* push @e, [$t, "isActive", node->{isActive}]; */
		mark_event ((unsigned int) node, offsetof(struct VRML_MovieTexture, isActive));
	}

	if(node->isActive == 1) {
		frac = node->__ctex;

		/* sanity check - avoids divide by zero problems below */
		if (lowest >= highest) {
			lowest = highest-1;
		}	
		/* calculate what fraction we should be */
 		myTime = (tick - node->startTime) * speed/duration;

		frac = myTime - truncf((float)myTime);
	
		/* negative speed? */
		if (speed < 0) {
			frac = 1+frac; /* frac will be *negative* */
		} else if (speed == 0) {
			frac = 0;
		}
	
		/* frac will tell us what texture frame we should apply... */
		frac = truncf(frac*(highest-lowest+1)+lowest);
	
		/* verify parameters */
		if (frac < lowest){ 
			frac = lowest;
		}
		if (frac > highest){ 
			frac = highest;
		}
	
		if (node->__ctex != frac) {
			node->__ctex = frac;

			/* force a change to re-render this node */
			update_node(node);
		}
	}
}

void do_TouchSensor (struct VRML_TouchSensor *node, char *ev, double tick, int over) {
	int len;

	/* TouchSensor - handle only a PRESS or RELEASE - should handle hitPoint,hitNormal */

	/* $f->{isOver} = $over;
	$f->{hitPoint_changed} = $pos;
	$f->{hitNormal_changed} = $norm;
	$f->{isActive} = 1;
	$f->{touchTime} = $time;
	*/

	/* if not enabled, do nothing */
	if (!node->enabled) return;

	/* isOver state */
	if (over != node->isOver) {
		node->isOver = over;
		mark_event ((unsigned int) node, 
			offsetof (struct VRML_TouchSensor, isOver));
	}


	/* active */
	if (over) {
		len = strlen(ev);
		if (len == strlen("PRESS")) {
			printf ("got a push\n");
			node->isActive=1;
			mark_event ((unsigned int) node, 
				offsetof (struct VRML_TouchSensor, isActive));

			node->touchTime = tick;
			mark_event((unsigned int) node,
				offsetof (struct VRML_TouchSensor, touchTime));

		} else if (len == strlen("RELEASE")) { 
			printf ("got a RELEASE\n");
			node->isActive=0;
			mark_event ((unsigned int) node, 
				offsetof (struct VRML_TouchSensor, isActive));
		}
	}
}

void do_PlaneSensor (struct VRML_PlaneSensor *node, char *ev, double tick, int over) {
	int len;
	float mult, nx, ny;
	struct SFColor tr;
	int tmp;



	len = strlen(ev);

	if (len == strlen("PRESS")) {
		/* record the current position from the saved position */
		memcpy ((void *) &node->_origPoint,
			(void *) &ray_save_posn,sizeof(struct SFColor));

		/* set isActive true */
		node->isActive=1;
		mark_event ((unsigned int) node, 
			offsetof (struct VRML_PlaneSensor, isActive));

	} else if (len == strlen("DRAG")) {
		/* hyperhit saved in render_hypersensitive phase */
		mult = (node->_origPoint.c[2] - hyp_save_posn.c[2]) /
			(hyp_save_norm.c[2]-hyp_save_posn.c[2]);
		nx = hyp_save_posn.c[0] + mult * (hyp_save_norm.c[0] - hyp_save_posn.c[0]);
		ny = hyp_save_posn.c[1] + mult * (hyp_save_norm.c[1] - hyp_save_posn.c[1]);

		if (SEVerbose) printf ("now, mult %f nx %f ny %f op %f %f %f\n",mult,nx,ny,
			node->_origPoint.c[0],node->_origPoint.c[1],
			node->_origPoint.c[2]);

		/* trackpoint changed */
		node->trackPoint_changed.c[0] = nx;
		node->trackPoint_changed.c[1] = ny;
		node->trackPoint_changed.c[2] = node->_origPoint.c[2];

		mark_event ((unsigned int) node,
			offsetof (struct VRML_PlaneSensor, trackPoint_changed));

		/* clamp translation to max/min position */
		tr.c[0] = nx - node->_origPoint.c[0] + node->offset.c[0];
		tr.c[1] = ny - node->_origPoint.c[1] + node->offset.c[1];
		tr.c[2] = node->offset.c[2];

		for (tmp=0; tmp<2; tmp++) {
			if (node->maxPosition.c[tmp] >= node->minPosition.c[tmp]) {
				if (tr.c[tmp] < node->minPosition.c[tmp]) {
					tr.c[tmp] = node->minPosition.c[tmp];
				} else if (tr.c[tmp] > node->maxPosition.c[tmp]) {
					tr.c[tmp] = node->maxPosition.c[tmp];
				}
			}
		}

		node->translation_changed.c[0] = tr.c[0];
		node->translation_changed.c[1] = tr.c[1];
		node->translation_changed.c[2] = tr.c[2];
		
		if (SEVerbose) printf ("TRC %f %f %f\n",node->translation_changed.c[0],
			node->translation_changed.c[1],node->translation_changed.c[2]);	

		/* and send this event */
		mark_event ((unsigned int) node,
			offsetof (struct VRML_PlaneSensor, translation_changed));



	} else if (len == strlen("RELEASE")) {
		/* set isActive false */
		node->isActive=0;
		mark_event ((unsigned int) node, 
			offsetof (struct VRML_PlaneSensor, isActive));

		/* autoOffset? */
		if (node->autoOffset) {
			node->offset.c[0] = node->translation_changed.c[0];
			node->offset.c[1] = node->translation_changed.c[1];
			node->offset.c[2] = node->translation_changed.c[2];

			mark_event ((unsigned int) node,
				offsetof (struct VRML_PlaneSensor, translation_changed));
		}
	}
}


void do_CylinderSensor (struct VRML_CylinderSensor *node, char *ev, double tick, int over) {
/* not implemented */
}


void do_SphereSensor (struct VRML_SphereSensor *node, char *ev, double tick, int over) {

/*__mouse__ => sub {
	my($t, $f, $time, $moved, $button, $over, $pos,
	   $norm, $texc) = @_;
	# print "PS_MOUSE: $moved $button $over @$pos @$norm\n";
	if ($button eq "PRESS") {
		$t->{OrigPoint} = $pos;
		$t->{Radius} = $pos->[0] ** 2 +
			$pos->[1] ** 2 + $pos->[2] ** 2;
		$f->{isActive} = 1;
	} elsif ($button eq "RELEASE") {
		undef $t->{OrigPoint};
		$t->{isActive} = 0;
		if ($f->{autoOffset}) {
			$f->{offset} = $f->{rotation_changed};
		}
	} elsif ($button eq "DRAG") {
		# 1. get the point on the plane
		my $op = $t->{OrigPoint};
		my $r = $t->{Radius};
		my $of = $f->{offset};

		my $tr1sq =
			$pos->[0]**2 +
			$pos->[1]**2 +
			$pos->[2]**2;
		my $tr2sq =
			$norm->[0]**2 +
			$norm->[1]**2 +
			$norm->[2]**2;
		my $tr1tr2 =
			$pos->[0]*$norm->[0] +
			$pos->[1]*$norm->[1] +
			$pos->[2]*$norm->[2];
		my @d = map {
			$norm->[$_] - $pos->[$_]
		} 0..2;
		my $dlen =
			$d[0]**2 +
			$d[1]**2 +
			$d[2]**2;

		my $a = $dlen;
		my $b = 2*($d[0]*$pos->[0] +
				   $d[1]*$pos->[1] +
				   $d[2]*$pos->[2]);
		my $c = $tr1sq - $r*$r;
		$b /= $a;
		$c /= $a;

		my $und = $b*$b - 4*$c;
		if ($und >= 0) {
			my $sol;
			if ($b >= 0) {
				$sol = (-$b + sqrt($und)) / 2;
			} else {
				$sol = (-$b - sqrt($und)) / 2;
			}
			my @r = map {
				$pos->[$_] + $sol * ($norm->[$_] - $pos->[$_])
			} 0..2;
			# Ok, now we have the two vectors op
			# and r, find out the rotation to take 
			# one to the other.
			my @cp = (
					  $r[1] * $op->[2] - $op->[1] * $r[2],
					  $r[2] * $op->[0] - $op->[2] * $r[0],
					  $r[0] * $op->[1] - $op->[0] * $r[1],
					 );
			my @dot = (
					   $r[0] * $op->[0],
					   $r[1] * $op->[1],
					   $r[2] * $op->[2]
					  );
			my $an = atan2((my $cl =
							$cp[0]**2 +
							$cp[1]**2 +
							$cp[2]**2),
						   $dot[0]**2 +
						   $dot[1]**2 +
						   $dot[2]**2);
			for (@cp) {
				$_ /= $cl;
			}
			$f->{trackPoint_changed} = [@r];
				
			# print "QNEW: @cp, $an (R: $r)\n";
			my $q =
				VRML::Quaternion->new_vrmlrot(@cp, -$an);
			# print "QNEW2: @$of\n";
			my $q2 =
				VRML::Quaternion->new_vrmlrot(@$of);

			$f->{rotation_changed} =
				$q->multiply($q2)->to_vrmlrot();
		}
	}
}
}*/
}
