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

#ifdef AQUA
#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#endif
#include "SensInterps.h"

/* if a Sound {} can not be found... */
#define BADAUDIOSOURCE -9999

/* when we get a new sound source, what is the number for this? */
int SoundSourceNumber = 0;

/* function prototypes */
void locateAudioSource (struct X3D_AudioClip *node);

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
	int loop,		/* nodes loop field			*/
	float myDuration,	/* duration of cycle			*/
	float speed		/* speed field				*/
) {

	/* what we do now depends on whether we are active or not */

	if (*act == 1) {   /* active - should we stop? */
		#ifdef SEVERBOSE
		printf ("is active tick %f startt %f stopt %f\n",
				TickTime, *startt, *stopt);
		#endif

		if (TickTime > *stopt) {
			if (*startt >= *stopt) {
				/* cases 1 and 2 */
				if (!(loop)) {
					/* printf ("case 1 and 2, not loop md %f sp %f fabs %f\n",
							myDuration, speed, fabs(myDuration/speed));
					*/
					/* if (speed != 0) */
					if (! APPROX(speed, 0)) {
					    if (TickTime >= (*startt +
							fabs(myDuration/speed))) {
						#ifdef SEVERBOSE
						printf ("stopping case x\n");
						#endif

						*act = 0;
						*stopt = TickTime;
					    }
					}
				}
			} else {
				#ifdef SEVERBOSE
				printf ("stopping case z\n");
				#endif

				*act = 0;
				*stopt = TickTime;
			}
		}
	}

	/* immediately process start events; as per spec.  */
	if (*act == 0) {   /* active - should we start? */
		/* printf ("is not active TickTime %f startt %f\n",TickTime,*startt); */

		if (TickTime >= *startt) {
			/* We just might need to start running */

			if (TickTime >= *stopt) {
				/* lets look at the initial conditions; have not had a stoptime
				event (yet) */

				if (loop) {
					if (*startt >= *stopt) {
						/* VRML standards, table 4.2 case 2 */
						/* printf ("CASE 2\n"); */
						*startt = TickTime;
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
						*startt = TickTime;
						*act = 1;
					}
				}
			} else {
				/* printf ("case 3 here\n"); */
				/* we should be running -
				VRML standards, table 4.2 cases 1 and 2 and 3 */
				*startt = TickTime;
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
	struct X3D_ScalarInterpolator *px;
	int kin, kvin;
	float *kVs;
	int counter;

	if (!node) return;
	px = (struct X3D_ScalarInterpolator *) node;
	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;

	mark_event (node, offsetof (struct X3D_ScalarInterpolator, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed = 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */

	#ifdef SEVERBOSE
		printf ("ScalarInterpolator, kin %d kvin %d, vc %f\n",kin,kvin,px->value_changed);
	#endif

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= px->key.p[0]) {
		 px->value_changed = kVs[0];
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		 px->value_changed = kVs[kvin-1];
	} else {
		/* have to go through and find the key before */
		counter=find_key(kin,(float)(px->set_fraction),px->key.p);
		px->value_changed =
			(px->set_fraction - px->key.p[counter-1]) /
			(px->key.p[counter] - px->key.p[counter-1]) *
			(kVs[counter] - kVs[counter-1]) +
			kVs[counter-1];
	}
}


void do_OintCoord(void *node) {
	struct X3D_CoordinateInterpolator *px;
	int kin, kvin/* , counter */;
	struct SFColor *kVs;
	struct SFColor *valchanged;

	int thisone, prevone;	/* which keyValues we are interpolating between */
	int tmp;
	float interval;		/* where we are between 2 values */
	struct pt normalval;	/* different structures for normalization calls */
	int kpkv; /* keys per key value */
	int indx;
	int myKey;

	if (!node) return;
	px = (struct X3D_CoordinateInterpolator *) node;


	#ifdef SEVERBOSE
		printf ("debugging OintCoord keys %d kv %d vc %d\n",px->keyValue.n, px->key.n,px->value_changed.n);
	#endif

	mark_event (node, offsetof (struct X3D_CoordinateInterpolator, value_changed));

	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;
	kpkv = kvin/kin;

	/* do we need to (re)allocate the value changed array? */
	if (kpkv != px->value_changed.n) {
		#ifdef SEVERBOSE
		    printf ("refactor valuechanged array. n %d sizeof p %d\n",
			kpkv,sizeof (struct SFColor) * kpkv);
		#endif
		if (px->value_changed.n != 0) {
			free (px->value_changed.p);
		}
		px->value_changed.n = kpkv;
		px->value_changed.p =(struct SFColor*) malloc (sizeof (struct SFColor) * kpkv);
	}

	/* shortcut valchanged; have to put it here because might be remalloc'd */
	valchanged = px->value_changed.p;


	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		#ifdef SEVERBOSE
		printf ("no keys or keyValues yet\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			valchanged[indx].c[0] = 0.0;
			valchanged[indx].c[1] = 0.0;
			valchanged[indx].c[2] = 0.0;
		}
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */


	#ifdef SEVERBOSE
		printf ("debugging, kpkv %d, px->value_changed.n %d\n", kpkv, px->value_changed.n);
		printf ("CoordinateInterpolator, kpkv %d\n",kpkv);
	#endif
	

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= px->key.p[0]) {
		#ifdef SEVERBOSE
		printf ("COINT out1\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			memcpy ((void *)&valchanged[indx],
				(void *)&kVs[indx], sizeof (struct SFColor));
			/* JAS valchanged[indx].c[0] = kVs[indx].c[0]; */
			/* JAS valchanged[indx].c[1] = kVs[indx].c[1]; */
			/* JAS valchanged[indx].c[2] = kVs[indx].c[2]; */
		}
		#ifdef SEVERBOSE
		printf ("COINT out1 copied\n");
		#endif
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		#ifdef SEVERBOSE
		printf ("COINT out2\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			memcpy ((void *)&valchanged[indx],
				(void *)&kVs[kvin-kpkv+indx],
				sizeof (struct SFColor));
		}
		#ifdef SEVERBOSE
		printf ("COINT out2 finished\n");
		#endif
	} else {
		#ifdef SEVERBOSE
		printf ("COINT out3\n");
		#endif

		/* have to go through and find the key before */
		#ifdef SEVERBOSE
		printf ("indx=0, kin %d frac %f\n",kin,px->set_fraction);
		#endif

		myKey=find_key(kin,(float)(px->set_fraction),px->key.p);
		#ifdef SEVERBOSE
		printf ("working on key %d\n",myKey);
		#endif

		/* find the fraction between the 2 values */
		interval = (px->set_fraction - px->key.p[myKey-1]) /
				(px->key.p[myKey] - px->key.p[myKey-1]);

		for (indx = 0; indx < kpkv; indx++) {
			thisone = myKey * kpkv + indx;
			prevone = (myKey-1) * kpkv + indx;

			#ifdef SEVERBOSE
			if (thisone >= kvin) {
				printf ("CoordinateInterpolator error: thisone %d prevone %d indx %d kpkv %d kin %d kvin %d\n",thisone,prevone,
				indx,kpkv,kin,kvin);
			}
			#endif

			for (tmp=0; tmp<3; tmp++) {
				valchanged[indx].c[tmp] = kVs[prevone].c[tmp]  +
						interval * (kVs[thisone].c[tmp] -
							kVs[prevone].c[tmp]);
			}
		}
		#ifdef SEVERBOSE
		printf ("COINT out3 finished\n");
		#endif

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
	#ifdef SEVERBOSE
	printf ("Done CoordinateInterpolator\n");
	#endif

}
void do_OintCoord2D(void *node) {
	struct X3D_CoordinateInterpolator2D *px;
	int kin, kvin/* , counter */;
	struct SFVec2f *kVs;
	struct SFVec2f *valchanged;

	int thisone, prevone;	/* which keyValues we are interpolating between */
	int tmp;
	float interval;		/* where we are between 2 values */
	struct pt normalval;	/* different structures for normalization calls */
	int kpkv; /* keys per key value */
	int indx;
	int myKey;

	if (!node) return;
	px = (struct X3D_CoordinateInterpolator2D *) node;


	#ifdef SEVERBOSE
		printf ("debugging OintCoord keys %d kv %d vc %d\n",px->keyValue.n, px->key.n,px->value_changed.n);
	#endif

	mark_event (node, offsetof (struct X3D_CoordinateInterpolator2D, value_changed));

	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;
	kpkv = kvin/kin;

	/* do we need to (re)allocate the value changed array? */
	if (kpkv != px->value_changed.n) {
		#ifdef SEVERBOSE
		    printf ("refactor valuechanged array. n %d sizeof p %d\n",
			kpkv,sizeof (struct SFVec2f) * kpkv);
		#endif
		if (px->value_changed.n != 0) {
			free (px->value_changed.p);
		}
		px->value_changed.n = kpkv;
		px->value_changed.p =(struct SFVec2f*) malloc (sizeof (struct SFVec2f) * kpkv);
	}

	/* shortcut valchanged; have to put it here because might be remalloc'd */
	valchanged = px->value_changed.p;


	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		#ifdef SEVERBOSE
		printf ("no keys or keyValues yet\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			valchanged[indx].c[0] = 0.0;
			valchanged[indx].c[1] = 0.0;
		}
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */


	#ifdef SEVERBOSE
		printf ("debugging, kpkv %d, px->value_changed.n %d\n", kpkv, px->value_changed.n);
		printf ("CoordinateInterpolator2D, kpkv %d\n",kpkv);
	#endif
	

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= px->key.p[0]) {
		#ifdef SEVERBOSE
		printf ("COINT out1\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			memcpy ((void *)&valchanged[indx],
				(void *)&kVs[indx], sizeof (struct SFVec2f));
			/* JAS valchanged[indx].c[0] = kVs[indx].c[0]; */
			/* JAS valchanged[indx].c[1] = kVs[indx].c[1]; */
		}
		#ifdef SEVERBOSE
		printf ("COINT out1 copied\n");
		#endif
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		#ifdef SEVERBOSE
		printf ("COINT out2\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			memcpy ((void *)&valchanged[indx],
				(void *)&kVs[kvin-kpkv+indx],
				sizeof (struct SFVec2f));
		}
		#ifdef SEVERBOSE
		printf ("COINT out2 finished\n");
		#endif
	} else {
		#ifdef SEVERBOSE
		printf ("COINT out3\n");
		#endif

		/* have to go through and find the key before */
		#ifdef SEVERBOSE
		printf ("indx=0, kin %d frac %f\n",kin,px->set_fraction);
		#endif

		myKey=find_key(kin,(float)(px->set_fraction),px->key.p);
		#ifdef SEVERBOSE
		printf ("working on key %d\n",myKey);
		#endif

		/* find the fraction between the 2 values */
		interval = (px->set_fraction - px->key.p[myKey-1]) /
				(px->key.p[myKey] - px->key.p[myKey-1]);

		for (indx = 0; indx < kpkv; indx++) {
			thisone = myKey * kpkv + indx;
			prevone = (myKey-1) * kpkv + indx;

			#ifdef SEVERBOSE
			if (thisone >= kvin) {
				printf ("CoordinateInterpolator2D error: thisone %d prevone %d indx %d kpkv %d kin %d kvin %d\n",thisone,prevone,
				indx,kpkv,kin,kvin);
			}
			#endif

			for (tmp=0; tmp<2; tmp++) {
				valchanged[indx].c[tmp] = kVs[prevone].c[tmp]  +
						interval * (kVs[thisone].c[tmp] -
							kVs[prevone].c[tmp]);
			}
		}
		#ifdef SEVERBOSE
		printf ("COINT out3 finished\n");
		#endif

	}

	#ifdef SEVERBOSE
	printf ("Done CoordinateInterpolator2D\n");
	#endif
}

void do_OintPos2D(void *node) {
/* PositionInterpolator2D				 		*/
/* Called during the "events_processed" section of the event loop,	*/
/* so this is called ONLY when there is something required to do, thus	*/
/* there is no need to look at whether it is active or not		*/

	struct X3D_PositionInterpolator2D *px;
	int kin, kvin, counter, tmp;
	struct SFVec2f *kVs;

	if (!node) return;
	px = (struct X3D_PositionInterpolator2D *) node;

	mark_event (node, offsetof (struct X3D_PositionInterpolator2D, value_changed));

	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;

	#ifdef SEVERBOSE
		printf("do_Oint2: Position interp2D, node %u kin %d kvin %d set_fraction %f\n",
			   node, kin, kvin, px->set_fraction);
	#endif

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.c[0] = 0.0;
		px->value_changed.c[1] = 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */


	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[0], sizeof (struct SFVec2f));
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[kvin-1], sizeof (struct SFVec2f));
	} else {
		/* have to go through and find the key before */
		counter = find_key(kin,((float)(px->set_fraction)),px->key.p);
		for (tmp=0; tmp<2; tmp++) {
			px->value_changed.c[tmp] =
				(px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]) *
				(kVs[counter].c[tmp] -
					kVs[counter-1].c[tmp]) +
				kVs[counter-1].c[tmp];
		}
	}
	#ifdef SEVERBOSE
	printf ("Pos/Col, new value (%f %f)\n",
		px->value_changed.c[0],px->value_changed.c[1]);
	#endif
}

/* PositionInterpolator, ColorInterpolator		 		*/
/* Called during the "events_processed" section of the event loop,	*/
/* so this is called ONLY when there is something required to do, thus	*/
/* there is no need to look at whether it is active or not		*/

void do_Oint3 (void *node) {
	/* PositionInterpolator - store final value in px->value_changed */
	struct X3D_PositionInterpolator *px;
	int kin, kvin, counter, tmp;
	struct SFColor *kVs;

	if (!node) return;
	px = (struct X3D_PositionInterpolator *) node;

	mark_event (node, offsetof (struct X3D_PositionInterpolator, value_changed));

	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;

	#ifdef SEVERBOSE
		printf("do_Oint3: Position/Color interp, node %u kin %d kvin %d set_fraction %f\n",
			   node, kin, kvin, px->set_fraction);
	#endif

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
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[kvin-1], sizeof (struct SFColor));
	} else {
		/* have to go through and find the key before */
		counter = find_key(kin,((float)(px->set_fraction)),px->key.p);
		for (tmp=0; tmp<3; tmp++) {
			px->value_changed.c[tmp] =
				(px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]) *
				(kVs[counter].c[tmp] -
					kVs[counter-1].c[tmp]) +
				kVs[counter-1].c[tmp];
		}
	}
	#ifdef SEVERBOSE
	printf ("Pos/Col, new value (%f %f %f)\n",
		px->value_changed.c[0],px->value_changed.c[1],px->value_changed.c[2]);
	#endif
}

/* OrientationInterpolator				 		*/
/* Called during the "events_processed" section of the event loop,	*/
/* so this is called ONLY when there is something required to do, thus	*/
/* there is no need to look at whether it is active or not		*/

void do_Oint4 (void *node) {
	struct X3D_OrientationInterpolator *px;
	int kin, kvin;
	struct SFRotation *kVs;
	int counter;
	int tmp;
	float testangle;	/* temporary variable 	*/
	float oldangle;		/* keyValue where we are moving from */
	float newangle;		/* keyValue where we are moving to */
	float interval;		/* where we are between 2 values */
	int stzero, endzero;	/* starting and/or ending angles zero? */

	if (!node) return;
	px = (struct X3D_OrientationInterpolator *) node;
	kin = ((px->key).n);
	kvin = ((px->keyValue).n);
	kVs = ((px->keyValue).p);

	#ifdef SEVERBOSE
	printf ("starting do_Oint4; keyValue count %d and key count %d\n",
				kvin, kin);
	#endif


	mark_event (node, offsetof (struct X3D_OrientationInterpolator, value_changed));

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
	} else if (px->set_fraction >= ((px->key).p[kin-1])) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[kvin-1], sizeof (struct SFRotation));
	} else {
		counter = find_key(kin,(float)(px->set_fraction),px->key.p);
		interval = (px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]);

		/* are either the starting or ending angles zero? */
		stzero = APPROX(kVs[counter-1].r[3],0.0);
		endzero = APPROX(kVs[counter].r[3],0.0);

		#ifdef SEVERBOSE
			printf ("counter %d interval %f\n",counter,interval);
			printf ("angles %f %f %f %f, %f %f %f %f\n",
				kVs[counter-1].r[0],
				kVs[counter-1].r[1],
				kVs[counter-1].r[2],
				kVs[counter-1].r[3],
				kVs[counter].r[0],
				kVs[counter].r[1],
				kVs[counter].r[2],
				kVs[counter].r[3]);
		#endif

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
		/* printf ("newangle %f oldangle %f testangle %f\n",*/
		/* 		newangle,oldangle, testangle);*/

		/* make it so we smoothly transition */
		if (fabs(testangle) > PI) {
			if (fabs(testangle) > (PI*2)) {
				if (testangle>0.0) { oldangle += PI*4;
				} else { newangle += PI*4; }
			} else {
				if (testangle>0.0) { oldangle += PI*2;
				} else { newangle += PI*2; }
			}
			/* printf ("NOW newangle %f oldangle %f testangle %f\n",*/
				/* 	newangle,oldangle, testangle);*/

		}

		/* ok, now, some people write rotations like 0 0 1 0, 1 0 0 0.3
		 * meaning that the first is "zero". We should not interpret
		 * the axes between these, only the angles */

		if (stzero || endzero) {
			/* printf ("have zero angle somewhere...\n");*/
			if (stzero) {
				/* starting angle zero, use the next axis */
				for (tmp = 0; tmp <= 2; tmp++) {
					px->value_changed.r[tmp] =
						kVs[counter].r[tmp];
				}
			} else {
				/* ending angle zero, use the first axis */
				for (tmp = 0; tmp <= 2; tmp++) {
					px->value_changed.r[tmp] =
						kVs[counter-1].r[tmp];
				}
			}
			/* printf ("axis now %f %f %f\n",px->value_changed.r[0],*/
			/* 		px->value_changed.r[1],*/
			/* 		px->value_changed.r[2]);*/
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
		#ifdef SEVERBOSE
		printf ("Oint, new angle %f %f %f %f\n",px->value_changed.r[0],
			px->value_changed.r[1],px->value_changed.r[2], px->value_changed.r[3]);
		#endif
	}
}

/* GeoPositionInterpolator				 		*/
/* Called during the "events_processed" section of the event loop,	*/
/* so this is called ONLY when there is something required to do, thus	*/
/* there is no need to look at whether it is active or not		*/

void do_GeoOint (void *node) {
	struct X3D_GeoPositionInterpolator *px;
	UNUSED(node);
	UNUSED(px);
}



/* fired at start of event loop for every Collision */
/* void do_CollisionTick(struct X3D_Collision *cx) {*/
void do_CollisionTick( void *ptr) {
	struct X3D_Collision *cx = (struct X3D_Collision *)ptr;
        if (cx->__hit == 3) {
                /* printf ("COLLISION at %f\n",TickTime); */
                cx->collideTime = TickTime;
                mark_event (ptr, offsetof(struct X3D_Collision, collideTime));
        }
}



/* Audio AudioClip sensor code */
/* void do_AudioTick(struct X3D_AudioClip *node) {*/
void do_AudioTick(void *ptr) {
	struct X3D_AudioClip *node = (struct X3D_AudioClip *)ptr;
	int 	oldstatus;

	/* can we possibly have started yet? */
	if (!node) return;

	if(TickTime < node->startTime) {
		return;
	}

	oldstatus = node->isActive;

	/* is this audio wavelet initialized yet? */
	if (node->__sourceNumber == -1) {
		locateAudioSource (node);
		/* printf ("do_AudioTick, node %d sn %d\n", node, node->__sourceNumber); */
	}

	/* is this audio ok? if so, the sourceNumber will range
	 * between 0 and infinity; if it is BADAUDIOSOURCE, bad source.
	 * check out locateAudioSource to find out reasons */
	if (node->__sourceNumber == BADAUDIOSOURCE) return;

	/* call common time sensor routine */
	do_active_inactive (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,node->loop,(float)(return_Duration(node->__sourceNumber)),
		((float)node->pitch));


	if (oldstatus != node->isActive) {
		/* push @e, [$t, "isActive", node->{isActive}]; */
		mark_event (node, offsetof(struct X3D_AudioClip, isActive));
		/* tell SoundEngine that this source has changed.  */
	        if (!SoundEngineStarted) {
        	        #ifdef SEVERBOSE
			printf ("SetAudioActive: initializing SoundEngine\n");
			#endif
                	SoundEngineStarted = TRUE;
                	SoundEngineInit();
		}
        	SetAudioActive (node->__sourceNumber,node->isActive);
	}
}



/* ProximitySensor code for ClockTick */
/* void do_ProximitySensorTick(struct X3D_ProximitySensor *node) {*/
void do_ProximitySensorTick( void *ptr) {
	struct X3D_ProximitySensor *node = (struct X3D_ProximitySensor *)ptr;
	/* are we enabled? */
	if (!node) return;
	if (!node->enabled) return;

	/* make sure scenegraph rendering knows we exist */
	if ((node->_renderFlags & VF_Proximity) != VF_Proximity)
		update_renderFlag(node,VF_Proximity);

	/* did we get a signal? */
	if (node->__hit) {
		if (!node->isActive) {
			#ifdef SEVERBOSE
			printf ("PROX - initial defaults\n");
			#endif

			node->isActive = 1;
			node->enterTime = TickTime;
			mark_event (ptr, offsetof(struct X3D_ProximitySensor, isActive));
			mark_event (ptr, offsetof(struct X3D_ProximitySensor, enterTime));

		}

		/* now, has anything changed? */
		if (memcmp ((void *) &node->position_changed,(void *) &node->__t1,sizeof(struct SFColor))) {
			#ifdef SEVERBOSE
			printf ("PROX - position changed!!! \n");
			#endif

			memcpy ((void *) &node->position_changed,
				(void *) &node->__t1,sizeof(struct SFColor));
			mark_event (ptr, offsetof(struct X3D_ProximitySensor, position_changed));
		}
		if (memcmp ((void *) &node->orientation_changed, (void *) &node->__t2,sizeof(struct SFRotation))) {
			#ifdef SEVERBOSE
			printf  ("PROX - orientation changed!!!\n ");
			#endif

			memcpy ((void *) &node->orientation_changed,
				(void *) &node->__t2,sizeof(struct SFRotation));
			mark_event (ptr, offsetof(struct X3D_ProximitySensor, orientation_changed));
		}
	} else {
		if (node->isActive) {
			#ifdef SEVERBOSE
			printf ("PROX - stopping\n");
			#endif

			node->isActive = 0;
			node->exitTime = TickTime;
			mark_event (ptr, offsetof(struct X3D_ProximitySensor, isActive));

			mark_event (ptr, offsetof(struct X3D_ProximitySensor, exitTime));
		}
	}
	node->__hit=FALSE;
}


/* Audio MovieTexture code */
/* void do_MovieTextureTick(struct X3D_MovieTexture *node) {*/
void do_MovieTextureTick( void *ptr) {
	struct X3D_MovieTexture *node = (struct X3D_MovieTexture *)ptr;
	int 	oldstatus;
	float 	frac;		/* which texture to display */
	int 	highest,lowest;	/* selector variables		*/
	/* double myDuration; */
	double myTime;
	float 	speed;
	float	duration;

	int tmpTrunc; 		/* used for timing for textures */

	/* can we possibly have started yet? */
	if (!node) return;
	if(TickTime < node->startTime) {
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
		&node->stopTime,node->loop,(float)duration,(float)speed);


	/* what we do now depends on whether we are active or not */
	if (oldstatus != node->isActive) {
		/* push @e, [$t, "isActive", node->{isActive}]; */
		mark_event (ptr, offsetof(struct X3D_MovieTexture, isActive));
	}

	if(node->isActive == 1) {
		frac = node->__ctex;

		/* sanity check - avoids divide by zero problems below */
		if (lowest >= highest) {
			lowest = highest-1;
		}
		/* calculate what fraction we should be */
 		myTime = (TickTime - node->startTime) * speed/duration;

		tmpTrunc = (int) myTime;
		frac = myTime - (float)tmpTrunc;

		/* negative speed? */
		if (speed < 0) {
			frac = 1+frac; /* frac will be *negative* */
		/* else if (speed == 0) */
		} else if (APPROX(speed, 0)) {
			frac = 0;
		}

		/* frac will tell us what texture frame we should apply... */
		/* code changed by Alberto Dubuc to compile on Solaris 8 */
		tmpTrunc = (int) (frac*(highest-lowest+1)+lowest);
		frac = (float) tmpTrunc;

		/* verify parameters */
		if (frac < lowest){
			frac = lowest;
		}
		if (frac > highest){
			frac = highest;
		}

		/* if (node->__ctex != frac) */
		if (! APPROX(node->__ctex, frac)) {
			node->__ctex = (int)frac;

			/* force a change to re-render this node */
			update_node(node);
		}
	}
}


/****************************************************************************

	Sensitive nodes

*****************************************************************************/
/* void do_GeoTouchSensor (struct X3D_GeoTouchSensor *node, int ev, int over) {*/
void do_GeoTouchSensor ( void *ptr, int ev, int over) {

struct X3D_GeoTouchSensor *node = (struct X3D_GeoTouchSensor *)ptr;
UNUSED(node);
UNUSED(ev);
UNUSED(over);

};


/* void do_TouchSensor (struct X3D_TouchSensor *node, int ev, int over) {*/
void do_TouchSensor ( void *ptr, int ev, int over) {

	struct X3D_TouchSensor *node;

    	node = (struct X3D_TouchSensor *)ptr;
	struct pt normalval;	/* different structures for normalization calls */

	/* if not enabled, do nothing */
	if (!node) return;
	if (!node->enabled) return;

	/* isOver state */
	if (over != node->isOver) {
		node->isOver = over;
		mark_event (ptr, offsetof (struct X3D_TouchSensor, isOver));
	}

	/* active */
	if (over) {

		/* button presses */
		if (ev == ButtonPress) {
			node->isActive=1;
			mark_event (ptr, offsetof (struct X3D_TouchSensor, isActive));

			node->touchTime = TickTime;
			mark_event(ptr, offsetof (struct X3D_TouchSensor, touchTime));

		} else if (ev == ButtonRelease) {
			node->isActive=0;
			mark_event (ptr,
				offsetof (struct X3D_TouchSensor, isActive));
		}

		/* hitPoint and hitNormal */
		memcpy ((void *) &node->hitPoint_changed,
				(void *) &ray_save_posn,sizeof(struct SFColor));
		mark_event(ptr, offsetof (struct X3D_TouchSensor, hitPoint_changed));

		/* have to normalize normal; change it from SFColor to struct pt. */
		normalval.x = hyp_save_norm.c[0];
		normalval.y = hyp_save_norm.c[1];
		normalval.z = hyp_save_norm.c[2];
		normalize_vector(&normalval);
		node->hitNormal_changed.c[0] = normalval.x;
		node->hitNormal_changed.c[1] = normalval.y;
		node->hitNormal_changed.c[2] = normalval.z;
		mark_event(ptr, offsetof (struct X3D_TouchSensor, hitNormal_changed));
	}
}

/* void do_PlaneSensor (struct X3D_PlaneSensor *node, int ev, int over) {*/
void do_PlaneSensor ( void *ptr, int ev, int over) {
	struct X3D_PlaneSensor *node = (struct X3D_PlaneSensor *)ptr;
	float mult, nx, ny;
	struct SFColor tr;
	int tmp;

	UNUSED(over);

	if (!node) return;

	if (ev==ButtonPress) {
		/* record the current position from the saved position */
		memcpy ((void *) &node->_origPoint,
			(void *) &ray_save_posn,sizeof(struct SFColor));

		/* set isActive true */
		node->isActive=1;
		mark_event (ptr, offsetof (struct X3D_PlaneSensor, isActive));

	} else if ((ev==MotionNotify) && (node->isActive==1)) {
		/* hyperhit saved in render_hypersensitive phase */
		mult = (node->_origPoint.c[2] - hyp_save_posn.c[2]) /
			(hyp_save_norm.c[2]-hyp_save_posn.c[2]);
		nx = hyp_save_posn.c[0] + mult * (hyp_save_norm.c[0] - hyp_save_posn.c[0]);
		ny = hyp_save_posn.c[1] + mult * (hyp_save_norm.c[1] - hyp_save_posn.c[1]);

		#ifdef SEVERBOSE
		printf ("now, mult %f nx %f ny %f op %f %f %f\n",mult,nx,ny,
			node->_origPoint.c[0],node->_origPoint.c[1],
			node->_origPoint.c[2]);
		#endif

		/* trackpoint changed */
		node->trackPoint_changed.c[0] = nx;
		node->trackPoint_changed.c[1] = ny;
		node->trackPoint_changed.c[2] = node->_origPoint.c[2];

		mark_event (ptr, offsetof (struct X3D_PlaneSensor, trackPoint_changed));

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

		#ifdef SEVERBOSE
		printf ("TRC %f %f %f\n",node->translation_changed.c[0],
			node->translation_changed.c[1],node->translation_changed.c[2]);
		#endif

		/* and send this event */
		mark_event (ptr, offsetof (struct X3D_PlaneSensor, translation_changed));



	} else if (ev==ButtonRelease) {
		/* set isActive false */
		node->isActive=0;
		mark_event (ptr, offsetof (struct X3D_PlaneSensor, isActive));

		/* autoOffset? */
		if (node->autoOffset) {
			node->offset.c[0] = node->translation_changed.c[0];
			node->offset.c[1] = node->translation_changed.c[1];
			node->offset.c[2] = node->translation_changed.c[2];

			mark_event (ptr, offsetof (struct X3D_PlaneSensor, translation_changed));
		}
	}
}


/* void do_Anchor (struct X3D_Anchor *node, int ev, int over) {*/
void do_Anchor ( void *ptr, int ev, int over) {
	struct X3D_Anchor *node = (struct X3D_Anchor *)ptr;
	UNUSED(over);

	if (!node) return;
	if (ev==ButtonPress) {
		/* no parameters in url field? */
		if (node->url.n < 1) return;
		AnchorsAnchor = node;
		BrowserAction = TRUE;
	}
}


/* void do_CylinderSensor (struct X3D_CylinderSensor *node, int ev, int over) {*/
void do_CylinderSensor ( void *ptr, int ev, int over) {
	struct X3D_CylinderSensor *node = (struct X3D_CylinderSensor *)ptr;
	float rot, radius, ang, length;
	double det, pos, neg, temp;
	Quaternion bv, dir1, dir2, tempV;
	GLdouble modelMatrix[16];

	UNUSED(over);

	if (!node) return;

	if (ev==ButtonPress) {
		/* record the current position from the saved position */
    		memcpy ((void *) &node->_origPoint,
			(void *) &ray_save_posn,sizeof(struct SFColor));

		/* set isActive true */
		node->isActive=1;
		mark_event (ptr, offsetof (struct X3D_CylinderSensor, isActive));

    		/* record the current Radius */
		node->_radius = ray_save_posn.c[0] * ray_save_posn.c[0] +
				ray_save_posn.c[1] * ray_save_posn.c[1] +
				ray_save_posn.c[2] * ray_save_posn.c[2];

        	fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
     		/*
     		printf ("Cur Matrix: \n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n",
               		modelMatrix[0],  modelMatrix[4],  modelMatrix[ 8],  modelMatrix[12],
               		modelMatrix[1],  modelMatrix[5],  modelMatrix[ 9],  modelMatrix[13],
               		modelMatrix[2],  modelMatrix[6],  modelMatrix[10],  modelMatrix[14],
               		modelMatrix[3],  modelMatrix[7],  modelMatrix[11],  modelMatrix[15]);
		*/

		/* find the bearing vector in the local coordinate system */
        	pos = neg = 0.0;
		temp =  modelMatrix[1] * modelMatrix[6] * modelMatrix[8];
        	if(temp >= 0.0) pos += temp; else neg += temp;
		temp = -modelMatrix[2] * modelMatrix[5] * modelMatrix[8];
        	if(temp >= 0.0) pos += temp; else neg += temp;
		temp = -modelMatrix[0] * modelMatrix[6] * modelMatrix[9];
        	if(temp >= 0.0) pos += temp; else neg += temp;
		temp =  modelMatrix[2] * modelMatrix[4] * modelMatrix[9];
		if(temp >= 0.0) pos += temp; else neg += temp;
		temp =  modelMatrix[0] * modelMatrix[5] * modelMatrix[10];
		if(temp >= 0.0) pos += temp; else neg += temp;
		temp = -modelMatrix[1] * modelMatrix[4] * modelMatrix[10];
       	 	if(temp >= 0.0) pos += temp; else neg += temp;
		det = pos + neg;
        	det = 1.0 / det;

		bv.w = 0;/* set to 0 to ensure vector is normalised correctly */
        	bv.x = (modelMatrix[4] * modelMatrix[9] - modelMatrix[5] * modelMatrix[8]) * det;
        	bv.y = -(modelMatrix[0] * modelMatrix[9] - modelMatrix[1] * modelMatrix[8]) * det;
        	bv.z = (modelMatrix[0] * modelMatrix[5] - modelMatrix[1] * modelMatrix[4]) * det;

		normalize(&bv);
		ang = acos(bv.y);
        	if (ang > (M_PI/2)) { ang = M_PI - ang; }

        	if (ang < node->diskAngle) {
			node->_dlchange=1;
        	} else {
			node->_dlchange=0;
        	}

	} else if (ev==MotionNotify) {

		memcpy ((void *) &node->trackPoint_changed,
			(void *) &ray_save_posn,sizeof(struct SFColor));

		mark_event (ptr, offsetof (struct X3D_CylinderSensor, trackPoint_changed));

		dir1.w=0;
  		dir1.x=ray_save_posn.c[0];
  		dir1.y=0;
  		dir1.z=ray_save_posn.c[2];

        	if (node->_dlchange) {
            		radius = 1.0;
		} else {
			/* get the radius */
            		radius = (dir1.x * dir1.x + dir1.y * dir1.y + dir1.z * dir1.z);
		}

        	normalize(&dir1);
        	dir2.w=0;
        	dir2.x=node->_origPoint.c[0];
		dir2.y=0;
  		dir2.z=node->_origPoint.c[2];

		normalize(&dir2);

    		tempV.w = 0;
    		tempV.x = dir2.y * dir1.z - dir2.z * dir1.y;
    		tempV.y = dir2.z * dir1.x - dir2.x * dir1.z;
    		tempV.z = dir2.x * dir1.y - dir2.y * dir1.x;
		normalize(&tempV);

        	length = tempV.x * tempV.x + tempV.y * tempV.y + tempV.z * tempV.z;
        	if (APPROX(length,0.0)) { return; }

		/* Find the angle of the dot product */
        	rot = radius * acos((dir1.x*dir2.x+dir1.y*dir2.y+dir1.z*dir2.z)) ;

		if (APPROX(tempV.y,-1.0)) rot = -rot;

        	if (node->autoOffset) {
            	rot = node->offset + rot;
		}
        	if (node->minAngle < node->maxAngle) {
            		if (rot < node->minAngle) {
                		rot = node->minAngle;
            		} else if (rot > node->maxAngle) {
                		rot = node->maxAngle;
            		}
        	}

		node->rotation_changed.r[0] = 0;
		node->rotation_changed.r[1] = 1;
		node->rotation_changed.r[2] = 0;
		node->rotation_changed.r[3] = rot;

		mark_event (ptr, offsetof (struct X3D_CylinderSensor, rotation_changed));

	} else if (ev==ButtonRelease) {
		/* set isActive false */
		node->isActive=0;
		mark_event (ptr, offsetof (struct X3D_CylinderSensor, isActive));
		/* save auto offset of rotation */
		if (node->autoOffset) {
			memcpy ((void *) &node->offset,
				(void *) &node->rotation_changed.r[3],
				sizeof (float));

		mark_event (ptr, offsetof (struct X3D_CylinderSensor, rotation_changed));
		}
	}
}


/* void do_SphereSensor (struct X3D_SphereSensor *node, int ev, int over) {*/
void do_SphereSensor ( void *ptr, int ev, int over) {
	struct X3D_SphereSensor *node = (struct X3D_SphereSensor *)ptr;
	int tmp;
	float tr1sq, tr2sq, tr1tr2;
	struct SFColor dee, arr, cp, dot;
	float deelen, aay, bee, cee, und, sol, cl, an;
	Quaternion q, q2, q_r;
	double s1,s2,s3,s4;

	UNUSED(over);

	if (!node) return;

	if (ev==ButtonPress) {
		/* record the current position from the saved position */
		memcpy ((void *) &node->_origPoint,
			(void *) &ray_save_posn,sizeof(struct SFColor));

		/* record the current Radius */
		node->_radius = ray_save_posn.c[0] * ray_save_posn.c[0] +
			ray_save_posn.c[1] * ray_save_posn.c[1] +
			ray_save_posn.c[2] * ray_save_posn.c[2];

		/* set isActive true */
		node->isActive=1;
		mark_event (ptr, offsetof (struct X3D_SphereSensor, isActive));

	} else if (ev==ButtonRelease) {
		/* set isActive false */
		node->isActive=0;
		mark_event (ptr, offsetof (struct X3D_SphereSensor, isActive));

		if (node->autoOffset) {
			memcpy ((void *) &node->offset,
				(void *) &node->rotation_changed,
				sizeof (struct SFRotation));
		}
	} else if (ev==MotionNotify) {
		/* 1. get the point on the plane */

		tr1sq = hyp_save_posn.c[0] * hyp_save_posn.c[0] +
			hyp_save_posn.c[1] * hyp_save_posn.c[1] +
                        hyp_save_posn.c[2] * hyp_save_posn.c[2];

		tr2sq = hyp_save_norm.c[0] * hyp_save_norm.c[0] +
			hyp_save_norm.c[1] * hyp_save_norm.c[1] +
                        hyp_save_norm.c[2] * hyp_save_norm.c[2];

		tr1tr2 = hyp_save_posn.c[0] * hyp_save_norm.c[0] +
			 hyp_save_posn.c[1] * hyp_save_norm.c[1] +
			 hyp_save_posn.c[2] * hyp_save_norm.c[2];

		for (tmp=0; tmp<3; tmp++) {
			dee.c[tmp] = hyp_save_norm.c[tmp] - hyp_save_posn.c[tmp];
		}

		deelen = dee.c[0]*dee.c[0] + dee.c[1]*dee.c[1] + dee.c[2]*dee.c[2];

		aay = deelen;
		bee = 2*(dee.c[0]*hyp_save_posn.c[0] +
			 dee.c[1]*hyp_save_posn.c[1] +
			 dee.c[2]*hyp_save_posn.c[2]);
		cee = tr1sq - node->_radius * node->_radius;
		bee = bee/aay;
		cee = cee/aay;

		und = bee*bee - 4.0*cee;

		if (und >= 0.0) {
			if (bee >= 0.0)  {
				sol = (-bee + sqrt(und)) / 2.0;
			} else {
				sol = (-bee - sqrt(und)) / 2.0;
			}

			for (tmp = 0; tmp < 3; tmp++) {
				arr.c[tmp] = hyp_save_posn.c[tmp] +
					sol * (hyp_save_norm.c[tmp] - hyp_save_posn.c[tmp]);
			}

			/* Ok, now we have the two vectors _origPoint
			and arr, find out the rotation to take
			one to the other. */

			cp.c[0] = arr.c[1] * node->_origPoint.c[2] -
				node->_origPoint.c[1] * arr.c[2];
			cp.c[1] = arr.c[2] * node->_origPoint.c[0] -
				node->_origPoint.c[2] * arr.c[0];
			cp.c[2] = arr.c[0] * node->_origPoint.c[1] -
				node->_origPoint.c[0] * arr.c[1];

			dot.c[0] = arr.c[0] * node->_origPoint.c[0];
			dot.c[1] = arr.c[1] * node->_origPoint.c[1];
			dot.c[2] = arr.c[2] * node->_origPoint.c[2];

			cl = cp.c[0]*cp.c[0] + cp.c[1]*cp.c[1] + cp.c[2]*cp.c[2];
			an = atan2(cl,  dot.c[0]*dot.c[0] + dot.c[1]*dot.c[1] +
						dot.c[2]*dot.c[2]);

			for (tmp=0; tmp<3;tmp++) {
				cp.c[tmp] = cp.c[tmp]/cl;
			}
			memcpy ((void *)&node->trackPoint_changed,
				(void *)&arr, sizeof (struct SFColor));
			mark_event (ptr, offsetof (struct X3D_SphereSensor, trackPoint_changed));

			vrmlrot_to_quaternion(&q, cp.c[0], cp.c[1], cp.c[2], -an);
			vrmlrot_to_quaternion(&q2,
								  (node->offset).r[0],
								  (node->offset).r[1],
								  (node->offset).r[2],
								  (node->offset).r[3]);
			multiply(&q_r, &q, &q2);

			/* calculate VRML rotation; note we have pointers to doubles, but rotation is float */
			quaternion_to_vrmlrot(&q_r,&s1,&s2,&s3,&s4);
			node->rotation_changed.r[0] = s1;
			node->rotation_changed.r[1] = s2;
			node->rotation_changed.r[2] = s3;
			node->rotation_changed.r[3] = s4;

			mark_event (ptr, offsetof (struct X3D_SphereSensor, rotation_changed));
		}
	}
}

void locateAudioSource (struct X3D_AudioClip *node) {
	STRLEN xx;
	int count;
	char *filename;
	char *mypath;
	char *slashindex;
	char *thisurl;
	char firstBytes[4]; /* not used here, but required for function call */

	node->__sourceNumber = SoundSourceNumber;
	SoundSourceNumber++;

	filename = (char*)malloc(1000);

	/* lets make up the path and save it, and make it the global path */
	count = strlen(SvPV(node->__parenturl,xx));
	mypath = (char *)malloc ((sizeof(char)* count)+1);

	if ((!filename) || (!mypath)) {
		outOfMemory ("locateAudioSource:can not malloc for filename\n");
	}

	/* copy the parent path over */
	strcpy (mypath,SvPV(node->__parenturl,xx));

	/* and strip off the file name, leaving any path */
	slashindex = (char *)rindex(mypath,'/');
	if (slashindex != NULL) {
		slashindex ++; /* leave the slash on */
		*slashindex = 0;
	 } else {mypath[0] = 0;}

	/* try the first url, up to the last */
	count = 0;
	while (count < (node->url).n) {
		thisurl = SvPV((node->url).p[count],xx);

		/* check to make sure we don't overflow */
		if ((strlen(thisurl)+strlen(mypath)) > 900) break;

		/* put the path and the file name together */
		makeAbsoluteFileName(filename,mypath,thisurl);

		if (fileExists(filename,firstBytes,TRUE)) { break; }
		count ++;
	}
	if (count == (node->url).n) {
		/* well, no file found */
		printf ("Audio: could not find audio file\n");
		free (filename);
		node->__sourceNumber = BADAUDIOSOURCE;
	} else {
		/* save local file in the structure, so that it can
		   be initialized later */
		node->__localFileName = (void *) filename;
	}
	free (mypath);
}
