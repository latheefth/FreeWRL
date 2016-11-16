/*


Do Sensors and Interpolators in C, not in perl.

Interps are the "EventsProcessed" fields of interpolators.

*/


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

#include "SensInterps.h"



/* when we get a new sound source, what is the number for this? */
//int SoundSourceNumber = 0;
typedef struct pSensInterps{
	int stub;
}* ppSensInterps;
void *SensInterps_constructor(){
	void *v = MALLOCV(sizeof(struct pSensInterps));
	memset(v,0,sizeof(struct pSensInterps));
	return v;
}
void SensInterps_init(struct tSensInterps *t)
{
	//public
	//private
	t->prv = SensInterps_constructor();
	{
		//ppSensInterps p = (ppSensInterps)t->prv;
	}
}


///* function prototypes */
//void locateAudioSource (struct X3D_AudioClip *node);


/* time dependent sensor nodes- check/change activity state */
void do_active_inactive (
	int *act, 		/* pointer to are we active or not?	*/
	double *inittime,	/* pointer to nodes inittime		*/
	double *startt,		/* pointer to nodes startTime		*/
	double *stopt,		/* pointer to nodes stop time		*/
	int loop,		/* nodes loop field			*/
	double myDuration,	/* duration of cycle			*/
	double speed		/* speed field				*/
) {

	/* what we do now depends on whether we are active or not */
	/* gcc seemed to have problems mixing double* and floats in a function
	   call, so make them all doubles. Note duplicate printfs in 
	   do_active_inactive call - uncomment and make sure all are identical 
		printf ("called ");
		printf ("act %d ",*act);
		printf ("initt %lf ",*inittime);
		printf ("startt %lf ",*startt);
		printf ("stopt %lf ",*stopt);
		printf ("loop %d ",loop);
		printf ("myDuration %lf ",myDuration);
		printf ("speed %f\n",speed);
	*/


	if (*act == 1) {   /* active - should we stop? */
		#ifdef SEVERBOSE
		printf ("is active tick %f startt %f stopt %f\n",
				TickTime(), *startt, *stopt);
		#endif

		if (TickTime() > *stopt) {
			if (*startt >= *stopt) {
				/* cases 1 and 2 */
				if (!(loop)) {
					/*printf ("case 1 and 2, not loop md %f sp %f fabs %f\n",
							myDuration, speed, fabs(myDuration/speed));
					*/
					
					/* if (speed != 0) */
					if (! APPROX(speed, 0)) {
					    if (TickTime() >= (*startt +
							fabs(myDuration/speed))) {
						#ifdef SEVERBOSE
						printf ("stopping case x\n");
						printf ("TickTime() %f\n",TickTime());
						printf ("startt %f\n",*startt);
						printf ("myDuration %f\n",myDuration);
						printf ("speed %f\n",speed);
						#endif

						*act = 0;
						*stopt = TickTime();
					    }
					}
				}
			} else {
				#ifdef SEVERBOSE
				printf ("stopping case z\n");
				#endif

				*act = 0;
				*stopt = TickTime();
			}
		}
	}

	/* immediately process start events; as per spec.  */
	if (*act == 0) {   /* active - should we start? */
		/* printf ("is not active TickTime %f startt %f\n",TickTime(),*startt); */

		if (TickTime() >= *startt) {
			/* We just might need to start running */

			if (TickTime() >= *stopt) {
				/* lets look at the initial conditions; have not had a stoptime
				event (yet) */

				if (loop) {
					if (*startt >= *stopt) {
						/* VRML standards, table 4.2 case 2 */
						/* printf ("CASE 2\n"); */
						/* Umut Sezen's code: */
						if (!(*startt > 0)) *startt = TickTime();
						*act = 1;
					}
				} else if (*startt >= *stopt) {
					if (*startt > *inittime) {
						/* ie, we have an event */
						 /* printf ("case 1 here\n"); */
						/* we should be running VRML standards, table 4.2 case 1 */
						/* Umut Sezen's code: */
						if (!(*startt > 0)) *startt = TickTime();
						*act = 1;
					}
				}
			} else {
				/* printf ("case 3 here\n"); */
				/* we should be running -
				VRML standards, table 4.2 cases 1 and 2 and 3 */
				/* Umut Sezen's code: */
				if (!(*startt > 0)) *startt = TickTime();
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

	MARK_EVENT (node, offsetof (struct X3D_ScalarInterpolator, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed = (float) 0.0;
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


void do_OintNormal(void *node) {
	struct X3D_NormalInterpolator *px;
	int kin, kvin/* , counter */;
	struct SFVec3f *kVs;
	struct SFVec3f *valchanged;

	int thisone, prevone;	/* which keyValues we are interpolating between */
	int tmp;
	float interval;		/* where we are between 2 values */
	struct point_XYZ normalval;	/* different structures for normalization calls */
	int kpkv; /* keys per key value */
	int indx;
	int myKey;

	if (!node) return;
	px = (struct X3D_NormalInterpolator *) node;


	#ifdef SEVERBOSE
		printf ("debugging OintCoord keys %d kv %d vc %d\n",px->keyValue.n, px->key.n,px->value_changed.n);
	#endif

	MARK_EVENT (node, offsetof (struct X3D_NormalInterpolator, value_changed));

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
			FREE_IF_NZ (px->value_changed.p);
		}
		px->value_changed.n = kpkv;
		px->value_changed.p = MALLOC (struct SFVec3f*, sizeof (struct SFVec3f) * kpkv);
	}

	/* shortcut valchanged; have to put it here because might be reMALLOC'd */
	valchanged = px->value_changed.p;


	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		#ifdef SEVERBOSE
		printf ("no keys or keyValues yet\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			valchanged[indx].c[0] = (float) 0.0;
			valchanged[indx].c[1] = (float) 0.0;
			valchanged[indx].c[2] = (float) 0.0;
		}
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */


	#ifdef SEVERBOSE
		printf ("debugging, kpkv %d, px->value_changed.n %d\n", kpkv, px->value_changed.n);
		printf ("NormalInterpolator, kpkv %d\n",kpkv);
	#endif
	

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= px->key.p[0]) {
		#ifdef SEVERBOSE
		printf ("COINT out1\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			memcpy ((void *)&valchanged[indx],
				(void *)&kVs[indx], sizeof (struct SFColor));
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
			#ifdef SEVERBOSE
			printf ("	1 %d interval %f prev %f this %f final %f\n",1,interval,kVs[prevone].c[1],kVs[thisone].c[1],valchanged[indx].c[1]);
			#endif
		}
		#ifdef SEVERBOSE
		printf ("COINT out3 finished\n");
		#endif

	}

	/* if this is a NormalInterpolator... */
	for (indx = 0; indx < kpkv; indx++) {
		normalval.x = valchanged[indx].c[0];
		normalval.y = valchanged[indx].c[1];
		normalval.z = valchanged[indx].c[2];
		normalize_vector(&normalval);
		valchanged[indx].c[0] = (float) normalval.x;
		valchanged[indx].c[1] = (float) normalval.y;
		valchanged[indx].c[2] = (float) normalval.z;
	}
	#ifdef SEVERBOSE
	printf ("Done CoordinateInterpolator\n");
	#endif
}


void do_OintCoord(void *node) {
	struct X3D_CoordinateInterpolator *px;
	int kin, kvin/* , counter */;
	struct SFVec3f *kVs;
	struct SFVec3f *valchanged;

	int thisone, prevone;	/* which keyValues we are interpolating between */
	int tmp;
	float interval;		/* where we are between 2 values */
	int kpkv; /* keys per key value */
	int indx;
	int myKey;

	if (!node) return;
	px = (struct X3D_CoordinateInterpolator *) node;

#ifdef SEVERBOSE
        printf ("do_OintCoord, frac %f toGPU %d toCPU %d\n",px->set_fraction,px->_GPU_Routes_out, px->_CPU_Routes_out);
		printf ("debugging OintCoord keys %d kv %d vc %d\n",px->keyValue.n, px->key.n,px->value_changed.n);
	#endif

	MARK_EVENT (node, offsetof (struct X3D_CoordinateInterpolator, value_changed));

    // create the VBOs if required, for running on the GPU
    if (px->_GPU_Routes_out > 0) {
        if (px->_keyVBO==0) {
            glGenBuffers(1,(GLuint *)&px->_keyValueVBO);
            glGenBuffers(1,(GLuint *)&px->_keyVBO);
            FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,px->_keyValueVBO);
            printf ("genning buffer data for %d keyValues, total floats %d\n",px->keyValue.n, px->keyValue.n*3);
            glBufferData(GL_ARRAY_BUFFER,px->keyValue.n *sizeof(float)*3,px->keyValue.p, GL_STATIC_DRAW);
            
            FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,px->_keyVBO);
            glBufferData(GL_ARRAY_BUFFER,px->key.n *sizeof(float),px->key.p, GL_STATIC_DRAW);
            printf ("created VBOs for the CoordinateInterpolator, they are %d and %d\n",
                    px->_keyValueVBO, px->_keyVBO);
        }
    }
    

    
    if (px->_CPU_Routes_out == 0) {
        #ifdef SEVERBOSE
        printf ("do_OintCoord, no CPU routes out, no need to do this work\n");
        #endif
        return;
    }

    //MARK_EVENT (node, offsetof (struct X3D_CoordinateInterpolator, value_changed));

    
	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;
	kpkv = kvin/kin;

	/* do we need to (re)allocate the value changed array? */
	if (kpkv != px->value_changed.n) {
		#ifdef SEVERBOSE
		    printf ("refactor valuechanged array. n %d sizeof p %d\n",
			kpkv,sizeof (struct SFVec3f) * kpkv);
		#endif
		if (px->value_changed.n != 0) {
			FREE_IF_NZ (px->value_changed.p);
		}
		px->value_changed.n = kpkv;
		px->value_changed.p = MALLOC (struct SFVec3f*, sizeof (struct SFVec3f) * kpkv);
	}

	/* shortcut valchanged; have to put it here because might be reMALLOC'd */
	valchanged = px->value_changed.p;


	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		#ifdef SEVERBOSE
		printf ("no keys or keyValues yet\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			valchanged[indx].c[0] = (float) 0.0;
			valchanged[indx].c[1] = (float) 0.0;
			valchanged[indx].c[2] = (float) 0.0;
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
				(void *)&kVs[indx], sizeof (struct SFVec3f));
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
				sizeof (struct SFVec3f));
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
			#ifdef SEVERBOSE
			printf ("	1 %d interval %f prev %f this %f final %f\n",1,interval,kVs[prevone].c[1],kVs[thisone].c[1],valchanged[indx].c[1]);
			#endif
		}
		#ifdef SEVERBOSE
		printf ("COINT out3 finished\n");
		#endif

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
	int kpkv; /* keys per key value */
	int indx;
	int myKey;

	if (!node) return;
	px = (struct X3D_CoordinateInterpolator2D *) node;


	#ifdef SEVERBOSE
		printf ("debugging OintCoord keys %d kv %d vc %d\n",px->keyValue.n, px->key.n,px->value_changed.n);
	#endif

	MARK_EVENT (node, offsetof (struct X3D_CoordinateInterpolator2D, value_changed));

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
			FREE_IF_NZ (px->value_changed.p);
		}
		px->value_changed.n = kpkv;
		px->value_changed.p = MALLOC (struct SFVec2f*, sizeof (struct SFVec2f) * kpkv);
	}

	/* shortcut valchanged; have to put it here because might be reMALLOC'd */
	valchanged = px->value_changed.p;


	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		#ifdef SEVERBOSE
		printf ("no keys or keyValues yet\n");
		#endif

		for (indx = 0; indx < kpkv; indx++) {
			valchanged[indx].c[0] = (float) 0.0;
			valchanged[indx].c[1] = (float) 0.0;
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

	MARK_EVENT (node, offsetof (struct X3D_PositionInterpolator2D, value_changed));

	kin = px->key.n;
	kvin = px->keyValue.n;
	kVs = px->keyValue.p;

	#ifdef SEVERBOSE
		printf("do_Oint2: Position interp2D, node %u kin %d kvin %d set_fraction %f\n",
			   node, kin, kvin, px->set_fraction);
	#endif

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.c[0] = (float) 0.0;
		px->value_changed.c[1] = (float) 0.0;
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

/* PositionInterpolator, ColorInterpolator, GeoPositionInterpolator	*/
/* Called during the "events_processed" section of the event loop,	*/
/* so this is called ONLY when there is something required to do, thus	*/
/* there is no need to look at whether it is active or not		*/

/* GeoPositionInterpolator in the Component_Geospatial file */

/* ColorInterpolator == PositionIterpolator */
void do_ColorInterpolator (void *node) {
	struct X3D_ColorInterpolator *px;
	int kin, kvin, counter, tmp;
	struct SFColor *kVs; 

	if (!node) return;
	px = (struct X3D_ColorInterpolator *) node;

	kvin = px->keyValue.n;
	kVs = px->keyValue.p;
	kin = px->key.n;

	MARK_EVENT (node, offsetof (struct X3D_ColorInterpolator, value_changed)); 

	#ifdef SEVERBOSE
		printf("do_ColorInt: Position/Color interp, node %u kin %d kvin %d set_fraction %f\n",
			   node, kin, kvin, px->set_fraction);
	#endif

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.c[0] = (float) 0.0;
		px->value_changed.c[1] = (float) 0.0;
		px->value_changed.c[2] = (float) 0.0;
		return;
	}

	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed, (void *)&kVs[0], sizeof (struct SFColor));
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		memcpy ((void *)&px->value_changed, (void *)&kVs[kvin-1], sizeof (struct SFColor));
	} else {
		/* have to go through and find the key before */
		counter = find_key(kin,((float)(px->set_fraction)),px->key.p);
		for (tmp=0; tmp<3; tmp++) {
			px->value_changed.c[tmp] =
				(px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]) *
				(kVs[counter].c[tmp] - kVs[counter-1].c[tmp]) + kVs[counter-1].c[tmp];
		}
	}
	#ifdef SEVERBOSE
	printf ("Pos/Col, new value (%f %f %f)\n",
		px->value_changed.c[0],px->value_changed.c[1],px->value_changed.c[2]);
	#endif
}


void do_PositionInterpolator (void *node) {
	struct X3D_PositionInterpolator *px;
	int kin, kvin, counter, tmp;
	struct SFVec3f *kVs; 

	if (!node) return;
	px = (struct X3D_PositionInterpolator *) node;

	kvin = px->keyValue.n;
	kVs = px->keyValue.p;
	kin = px->key.n;

	MARK_EVENT (node, offsetof (struct X3D_PositionInterpolator, value_changed)); 

	#ifdef SEVERBOSE
		printf("do_PositionInt: Position/Vec3f interp, node %u kin %d kvin %d set_fraction %f\n",
			   node, kin, kvin, px->set_fraction);
	#endif

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.c[0] = (float) 0.0;
		px->value_changed.c[1] = (float) 0.0;
		px->value_changed.c[2] = (float) 0.0;
		return;
	}

	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */

	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed, (void *)&kVs[0], sizeof (struct SFVec3f));
	} else if (px->set_fraction >= px->key.p[kin-1]) {
		memcpy ((void *)&px->value_changed, (void *)&kVs[kvin-1], sizeof (struct SFVec3f));
	} else {
		/* have to go through and find the key before */
		counter = find_key(kin,((float)(px->set_fraction)),px->key.p);
		for (tmp=0; tmp<3; tmp++) {
			px->value_changed.c[tmp] =
				(px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]) *
				(kVs[counter].c[tmp] - kVs[counter-1].c[tmp]) + kVs[counter-1].c[tmp];
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
	float interval;		/* where we are between 2 values */
	// UNUSED?? int stzero;
	// UNUSED?? int endzero;	/* starting and/or ending angles zero? */

	Quaternion st, fin, final;
	double x,y,z,a;

	if (!node) return;
	px = (struct X3D_OrientationInterpolator *) node;
	kin = ((px->key).n);
	kvin = ((px->keyValue).n);
	kVs = ((px->keyValue).p);

	#ifdef SEVERBOSE
	printf ("starting do_Oint4; keyValue count %d and key count %d\n",
				kvin, kin);
	#endif


	MARK_EVENT (node, offsetof (struct X3D_OrientationInterpolator, value_changed));

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

/* fired at start of event loop for every Collision */
/* void do_CollisionTick(struct X3D_Collision *cx) {*/
void do_CollisionTick( void *ptr) {
	struct X3D_Collision *cx = (struct X3D_Collision *)ptr;
        if (cx->__hit == 3) {
                /* printf ("COLLISION at %f\n",TickTime()); */
                cx->collideTime = TickTime();
                MARK_EVENT (ptr, offsetof(struct X3D_Collision, collideTime));
        }
}


/* Audio AudioClip sensor code */
/* void do_AudioTick(struct X3D_AudioClip *node) {*/
void do_AudioTick(void *ptr) {
	struct X3D_AudioClip *node = (struct X3D_AudioClip *)ptr;
	int 	oldstatus;
	double pitch, duration; /* gcc and params - make all doubles to do_active_inactive */

	/* can we possibly have started yet? */
	if (!node) return;

	if(TickTime() < node->startTime) {
		return;
	}

	oldstatus = node->isActive;
	pitch = node->pitch;

	if(node->__sourceNumber < 0) return;
	///* is this audio wavelet initialized yet? */
	//if (node->__sourceNumber == -1) {
	//	locateAudioSource (node);
	//	/* printf ("do_AudioTick, node %d sn %d\n", node, node->__sourceNumber);  */
	//}

	///* is this audio ok? if so, the sourceNumber will range
	// * between 0 and infinity; if it is BADAUDIOSOURCE, bad source.
	// * check out locateAudioSource to find out reasons */
	//if (node->__sourceNumber == BADAUDIOSOURCE) return;

	/* call common time sensor routine */
	//duration = return_Duration(node->__sourceNumber);
	duration = return_Duration(node);
	do_active_inactive (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,node->loop,duration,
		pitch);

	if (oldstatus != node->isActive) {
		/* push @e, [$t, "isActive", node->{isActive}]; */
		MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_AudioClip, isActive));
		/* tell SoundEngine that this source has changed.  */
		//if (!SoundEngineStarted) {
		//	#ifdef SEVERBOSE
		//	printf ("SetAudioActive: initializing SoundEngine\n");
		//	#endif
		//	SoundEngineStarted = TRUE;
		//	SoundEngineInit();
		//}
		//if(haveSoundEngine())
		//	SetAudioActive (node->__sourceNumber,node->isActive);
	}
	
	if(node->isActive){
		if(node->pauseTime > node->startTime){
			if( node->resumeTime < node->pauseTime && !node->isPaused){
				node->isPaused = TRUE;
				MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_AudioClip, isPaused));
			}else if(node->resumeTime > node->pauseTime && node->isPaused){
				node->isPaused = FALSE;
				MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_AudioClip, isPaused));
			}
		}
	}

}



/* Similar to AudioClip, this is the Play, Pause, Stop, Resume code
*/
unsigned char *movietexture_get_frame_by_fraction(struct X3D_Node* node, float fraction, int *width, int *height, int *nchan);
void do_MovieTextureTick( void *ptr) {
	struct X3D_MovieTexture *node = (struct X3D_MovieTexture *)ptr;
	struct X3D_AudioClip *anode;
	int 	oldstatus;
	float 	frac;		/* which texture to display */
	//int 	highest,lowest;	/* selector variables		*/
	double myTime;
	double 	speed;
	double	duration;
	int tmpTrunc; 		/* used for timing for textures */

	//anode = (struct X3D_AudioClip *)node;
	//do_AudioTick(ptr);  //does play, pause, active, inactive part

	/* can we possibly have started yet? */
	if (!node) return;
	if(TickTime() < node->startTime) {
		return;
	}

//	duration = (highest - lowest)/30.0;
	//highest = node->__highest;
	//lowest = node->__lowest;
	duration = return_Duration(node);
	speed = node->speed;

	oldstatus = node->isActive;
	do_active_inactive (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,node->loop,duration,
		speed);

	if (oldstatus != node->isActive) {
		MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_MovieTexture, isActive));
	}

	if(node->isActive){
		if(node->pauseTime > node->startTime){
			if( node->resumeTime < node->pauseTime && !node->isPaused){
				node->isPaused = TRUE;
				MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_MovieTexture, isPaused));
			}else if(node->resumeTime > node->pauseTime && node->isPaused){
				node->isPaused = FALSE;
				MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_MovieTexture, isPaused));
			}
		}
	}

	if(node->isActive && node->isPaused == FALSE) {
		//frac = node->__ctex;

		///* sanity check - avoids divide by zero problems below */
		//if (node->__lowest >= node->__highest) {
		//	node->__lowest = node->__highest-1;
		//}
		/* calculate what fraction we should be */
 		myTime = (TickTime() - node->startTime) * speed/duration;
		tmpTrunc = (int) myTime;
		frac = myTime - (float)tmpTrunc;

		/* negative speed? */
		if (speed < 0) {
			frac = 1.0f + frac; /* frac will be *negative* */
		/* else if (speed == 0) */
		} else if (APPROX(speed, 0.0f)) {
			frac = 0.0f;
		}


		//Nov 16, 2016 the following works with MPEG_Utils_ffmpeg.c on non-audio mpeg (vts.mpg)
		// x not tested with audio
		unsigned char* texdata;
		int width,height,nchan;
		textureTableIndexStruct_s *tti;
		texdata = movietexture_get_frame_by_fraction(node, frac, &width, &height, &nchan);
		if(texdata){
			//printf("[%f %p]",frac,texdata);
			int thisTexture = node->__textureTableIndex;
			tti = getTableIndex(thisTexture);
			//printf("[ %d %p ]",thisTexture,tti);
			if(tti){
				tti->x = width;
				tti->y = height;
				tti->z = 1;
				tti->channels = nchan;
				static int once = 0;
				if(!once){
					//send it through textures.c once to get things like wrap set
					// textures.c likes to free texdata, so we'll deep copy
					tti->texdata = malloc(tti->x*tti->y*tti->channels);
					memcpy(tti->texdata,texdata,tti->x*tti->y*tti->channels);
					tti->status = TEX_NEEDSBINDING;
					once = 1;
				}else{
					tti->status = TEX_LOADED;
					glBindTexture(GL_TEXTURE_2D,tti->OpenGLTexture);
					//disable the mipmapping done on the once pass through textures.c above
					FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					//replace the texture data every frame when we are isActive and not paused
					if(nchan == 4)
						glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,texdata);
					if(nchan == 3)
						glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,texdata);
					glBindTexture(GL_TEXTURE_2D,0);
				}
			}else{
				printf("movietexture has no tti\n");
			}
		}

	}
}


/****************************************************************************

	Sensitive nodes

*****************************************************************************/
void do_TouchSensor ( void *ptr, int ev, int but1, int over) {

	struct X3D_TouchSensor *node = (struct X3D_TouchSensor *)ptr;
	struct point_XYZ normalval;	/* different structures for normalization calls */
	ttglobal tg;
	#ifdef SENSVERBOSE
	printf ("%lf: TS ",TickTime());
	if (ev==ButtonPress) printf ("ButtonPress ");
	else if (ev==ButtonRelease) printf ("ButtonRelease ");
	else if (ev==KeyPress) printf ("KeyPress ");
	else if (ev==KeyRelease) printf ("KeyRelease ");
	else if (ev==MotionNotify) printf ("%lf MotionNotify ");
	else printf ("ev %d ",ev);
	
	if (but1) printf ("but1 TRUE "); else printf ("but1 FALSE ");
	if (over) printf ("over TRUE "); else printf ("over FALSE ");
	printf ("\n");
	#endif


	/* if not enabled, do nothing */
	if (!node) return;
	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_TouchSensor, enabled));
	}
	if (!node->enabled) return;
	tg = gglobal();
	/* isOver state */
	if ((ev == overMark) && (over != node->isOver)) {
		#ifdef SENSVERBOSE
		printf ("TS %u, isOver changed %d\n",node, over);
		#endif
		node->isOver = over;
		MARK_EVENT (ptr, offsetof (struct X3D_TouchSensor, isOver));
	}

	/* active */
		/* button presses */
		if (ev == ButtonPress) {
			node->isActive=TRUE;
			MARK_EVENT (ptr, offsetof (struct X3D_TouchSensor, isActive));
			#ifdef SENSVERBOSE
			printf ("touchSens %u, butPress\n",node);
			#endif

			node->touchTime = TickTime();
			MARK_EVENT(ptr, offsetof (struct X3D_TouchSensor, touchTime));

		} else if (ev == ButtonRelease) {
			#ifdef SENSVERBOSE
			printf ("touchSens %u, butRelease\n",node);
			#endif
			node->isActive=FALSE;
			MARK_EVENT (ptr, offsetof (struct X3D_TouchSensor, isActive));
		}

		/* hitPoint and hitNormal */
		/* save the current hitPoint for determining if this changes between runs */
		memcpy ((void *) &node->_oldhitPoint, (void *) &tg->RenderFuncs.ray_save_posn,sizeof(struct SFColor));

		/* did the hitPoint change between runs? */
		if ((APPROX(node->_oldhitPoint.c[0],node->hitPoint_changed.c[0])!= TRUE) ||
			(APPROX(node->_oldhitPoint.c[1],node->hitPoint_changed.c[1])!= TRUE) ||
			(APPROX(node->_oldhitPoint.c[2],node->hitPoint_changed.c[2])!= TRUE)) {

			memcpy ((void *) &node->hitPoint_changed, (void *) &node->_oldhitPoint, sizeof(struct SFColor));
			MARK_EVENT(ptr, offsetof (struct X3D_TouchSensor, hitPoint_changed));
		}

		/* have to normalize normal; change it from SFColor to struct point_XYZ. */
		normalval.x = tg->RenderFuncs.hyp_save_norm[0];
		normalval.y = tg->RenderFuncs.hyp_save_norm[1];
		normalval.z = tg->RenderFuncs.hyp_save_norm[2];
		normalize_vector(&normalval);
		node->_oldhitNormal.c[0] = (float) normalval.x;
		node->_oldhitNormal.c[1] = (float) normalval.y;
		node->_oldhitNormal.c[2] = (float) normalval.z;

		/* did the hitNormal change between runs? */
		if ((APPROX(node->_oldhitNormal.c[0],node->hitNormal_changed.c[0])!= TRUE) ||
			(APPROX(node->_oldhitNormal.c[1],node->hitNormal_changed.c[1])!= TRUE) ||
			(APPROX(node->_oldhitNormal.c[2],node->hitNormal_changed.c[2])!= TRUE)) {

			memcpy ((void *) &node->hitNormal_changed, (void *) &node->_oldhitNormal, sizeof(struct SFColor));
			MARK_EVENT(ptr, offsetof (struct X3D_TouchSensor, hitNormal_changed));
		}
}


void do_LineSensor(void *ptr, int ev, int but1, int over) {
	/* There is no LineSensor node in the specs in April 2014. X3Dom guru Max Limper complained
		on X3DPublic about how PlaneSensor fails as an axis mover in the degenerate case of
		looking edge-on at the planeSensor. The solution we (dug9) came up with was LineSensor, 
		which also has a degenerate case (when looking end-on at the Line), but that degerate
		case is more normal for users - more intuitive.
		LineSensor is the same as PlaneSensor, except minPosition and maxPosition are floats,
		and LineSensor uses a SFVec3f .direction field to say which way the Line is oriented
		in local-sensor coordinates. In April 2014, Paulo added LineSensor to the perl generator,
		and dug9 implemented it here.
	*/
	struct X3D_LineSensor *node;
	float trackpoint[3], translation[3], xxx;
	//struct SFColor tr;
	//int tmp;
	ttglobal tg;
	UNUSED(over);
	node = (struct X3D_LineSensor *)ptr;
#ifdef SENSVERBOSE
	printf("%lf: TS ", TickTime());
	if (ev == ButtonPress) printf("ButtonPress ");
	else if (ev == ButtonRelease) printf("ButtonRelease ");
	else if (ev == KeyPress) printf("KeyPress ");
	else if (ev == KeyRelease) printf("KeyRelease ");
	else if (ev == MotionNotify) printf("%lf MotionNotify ");
	else printf("ev %d ", ev);

	if (but1) printf("but1 TRUE "); else printf("but1 FALSE ");
	if (over) printf("over TRUE "); else printf("over FALSE ");
	printf("\n");
#endif

	/* if not enabled, do nothing */
	if (!node) return;

	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node), offsetof(struct X3D_LineSensor, enabled));
	}
	if (!node->enabled) return;
	tg = gglobal();

	/* only do something when button pressed */
	/* if (!but1) return; */
	if (but1){
		//pre-calculate for Press and Move
		/* hyperhit saved in render_hypersensitive phase */
		// bearing in sensor-local coordinates: (A=posn,B=norm) 
		// B/norm is a point, so to get a direction vector: v = B - A
		float tt;
		float origin [] = { 0.0f, 0.0f, 0.0f };
		float footpoint2[3], footpoint1[3], v1[3]; //, temp[3], temp2[3];
		vecdif3f(v1, tg->RenderFuncs.hyp_save_norm, tg->RenderFuncs.hyp_save_posn);
		vecnormalize3f(v1, v1);
		if (!line_intersect_line_3f(tg->RenderFuncs.hyp_save_posn, v1,
			origin, node->direction.c, NULL, &tt, footpoint1, footpoint2)) 
			return; //no intersection, lines are parallel
		//footpoint1 - closest point of intersection on the A'B' bearing
		//footpoint2 - closest point of intersection on the Line (0,0,0)(LineSensor.direction)
		//tt is scale of unit vector from origin to footpoint2
		xxx = tt;
		veccopy3f(trackpoint,footpoint2); //unclamped intersection with Sensor geometry, for trackpoint
	}
	if ((ev == ButtonPress) && but1) {
		/* record the current position from the saved position */
#define LINESENSOR_FLOAT_OFFSET 1
#ifndef LINESENSOR_FLOAT_OFFSET
		struct SFColor op;
		veccopy3f(op.c, trackpoint);
		memcpy((void *)&node->_origPoint, (void *)&op,sizeof(struct SFColor));
		//	(void *)&tg->RenderFuncs.ray_save_posn, sizeof(struct SFColor));
#else
		node->_origPoint.c[0] = xxx;
		//in case we go from mousedown to mouseup without mousemove:
		if (node->autoOffset)
			node->_origPoint.c[1] = node->offset; 
		else
			node->_origPoint.c[1] = 0.0f;
#endif
		/* set isActive true */
		node->isActive = TRUE;
		MARK_EVENT(ptr, offsetof(struct X3D_LineSensor, isActive));

	}
	else if ((ev == MotionNotify) && (node->isActive) && but1) {
		float xxxoffset, xxxorigin;
		//float diroffset[3], nondiroffset[3];
		/* trackpoint changed */
		node->_oldtrackPoint.c[0] = trackpoint[0];
		node->_oldtrackPoint.c[1] = trackpoint[1];
		node->_oldtrackPoint.c[2] = trackpoint[2];
		
		if ((APPROX(node->_oldtrackPoint.c[0], node->trackPoint_changed.c[0]) != TRUE) ||
			(APPROX(node->_oldtrackPoint.c[1], node->trackPoint_changed.c[1]) != TRUE) ||
			(APPROX(node->_oldtrackPoint.c[2], node->trackPoint_changed.c[2]) != TRUE)) {

			memcpy((void *)&node->trackPoint_changed, (void *)&node->_oldtrackPoint, sizeof(struct SFColor));
			MARK_EVENT(ptr, offsetof(struct X3D_LineSensor, trackPoint_changed));

		}

		//clamp to min,max 
#ifdef LINESENSOR_FLOAT_OFFSET
		xxxoffset = node->offset;
		xxxorigin = node->_origPoint.c[0];
#else
		//in theory the user can set a non-autoOffset sfvec3f offset that's not along .direction
		//- we accomodate that below^, so here we just use the part going along .direction
		xxxoffset = vecdot3f(node->direction.c,node->offset.c); //xxxoffset - like web3d specs offset, except just along direction vector
		xxxorigin = vecdot3f(node->direction.c,node->_origPoint.c); //mouse-down origin
#endif
		//xxx before: unclamped position from line origin
		xxx -= xxxorigin; //xxx after: net drag/delta along line since mouse-down
		xxx += xxxoffset; //xxx after: cumulative position along line (from line 0) after any/all mousedown/drag sequences
		if (node->maxPosition >= node->minPosition) {
			if (xxx < node->minPosition) {
				xxx = node->minPosition;
			}
			else if (xxx > node->maxPosition) {
				xxx = node->maxPosition;
			}
		}
		//translation clamped to LineSensor.minPosition/.maxPosition
		vecscale3f(translation, node->direction.c, xxx);

#ifndef LINESENSOR_FLOAT_OFFSET		
		//^add on any non-autoOffset non-.direction offset 
		//a) part of offset going along direction
		vecscale3f(diroffset, node->direction.c, xxxoffset);
		//b) part of offset not going along direction
		vecdif3f(nondiroffset, node->offset.c, diroffset);
		//add non-direction part of offset
		vecadd3f(translation, translation, nondiroffset);
#endif

		node->_oldtranslation.c[0] = translation[0];
		node->_oldtranslation.c[1] = translation[1];
		node->_oldtranslation.c[2] = translation[2];

		if ((APPROX(node->_oldtranslation.c[0], node->translation_changed.c[0]) != TRUE) ||
			(APPROX(node->_oldtranslation.c[1], node->translation_changed.c[1]) != TRUE) ||
			(APPROX(node->_oldtranslation.c[2], node->translation_changed.c[2]) != TRUE)) {

			memcpy((void *)&node->translation_changed, (void *)&node->_oldtranslation, sizeof(struct SFColor));
			MARK_EVENT(ptr, offsetof(struct X3D_LineSensor, translation_changed));
		}
		//save current for use in mouse-up auto-offset
		node->_origPoint.c[1] = xxx;
	}
	else if (ev == ButtonRelease) {
		/* set isActive false */
		node->isActive = FALSE;
		MARK_EVENT(ptr, offsetof(struct X3D_LineSensor, isActive));

		/* autoOffset? */
		if (node->autoOffset) {
#ifdef LINESENSOR_FLOAT_OFFSET
			node->offset = node->_origPoint.c[1];
#else
			node->offset.c[0] = node->translation_changed.c[0];
			node->offset.c[1] = node->translation_changed.c[1];
			node->offset.c[2] = node->translation_changed.c[2];
#endif
			MARK_EVENT(ptr, offsetof(struct X3D_LineSensor, offset));
		}
	}

}

/* void do_PlaneSensor (struct X3D_PlaneSensor *node, int ev, int over) {*/
void do_PlaneSensor ( void *ptr, int ev, int but1, int over) {
	struct X3D_PlaneSensor *node;
	float mult, nx, ny, trackpoint[3], *posn;
	struct SFColor tr;
	int tmp, imethod;
	ttglobal tg;
	UNUSED(over);
	node = (struct X3D_PlaneSensor *)ptr;
#ifdef SENSVERBOSE
	ConsoleMessage("%lf: TS ",TickTime());
	if (ev==ButtonPress) ConsoleMessage("ButtonPress ");
	else if (ev==ButtonRelease) ConsoleMessage("ButtonRelease ");
	else if (ev==KeyPress) ConsoleMessage("KeyPress ");
	else if (ev==KeyRelease) ConsoleMessage("KeyRelease ");
	else if (ev==MotionNotify) ConsoleMessage("MotionNotify ");
	else ConsoleMessage("ev %d ",ev);
	
	if (but1) ConsoleMessage("but1 TRUE "); else ConsoleMessage("but1 FALSE ");
	if (over) ConsoleMessage("over TRUE "); else ConsoleMessage("over FALSE ");
	ConsoleMessage ("\n");
#endif

	/* if not enabled, do nothing */
	if (!node) return;

	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_PlaneSensor, enabled));
	}
	if (!node->enabled) return;
	tg = gglobal();

	imethod = 1; //0 = old pre-April-2014, 1=April 2014
	/* only do something when button pressed */
	/* if (!but1) return; */
	if (but1){
		float v[3], t1[3];
		float N[3] = { 0.0f, 0.0f, 1.0f }; //plane normal, in plane-local
		float NS[3]; //plane normal, in sensor-local after axisRotation
		//bearing (A,B) in sensor-local
		// A=posn, B=norm - norm is a point. To get a direction vector v = (B - A)
		//ConsoleMessage("hsp = %f %f %f \n", tg->RenderFuncs.hyp_save_posn[0], tg->RenderFuncs.hyp_save_posn[1], tg->RenderFuncs.hyp_save_posn[2]);
		vecnormalize3f(v, vecdif3f(t1, tg->RenderFuncs.hyp_save_norm, tg->RenderFuncs.hyp_save_posn));
		//rotate plane normal N, in plane-local to plane normal NS in sensor-local using axisRotation
		axisangle_rotate3f(NS,N, node->axisRotation.c);
		//a plane P dot N = d = const, for any point P on plane. Our plane is in plane-local coords, 
		// so we could use P={0,0,0} and P dot N = d = 0
		posn = tg->RenderFuncs.hyp_save_posn;
		if (!line_intersect_planed_3f(posn, v, NS, 0.0f, trackpoint, NULL))
			return; //looking at plane edge-on / parallel, no intersection
		axisangle_rotate3f(trackpoint, trackpoint, node->axisRotation.c);
	}

	if ((ev==ButtonPress) && but1) {
		/* record the current position from the saved position */
		struct SFColor op;
		float *posn;
		posn = tg->RenderFuncs.hyp_save_posn;

		veccopy3f(op.c, trackpoint);
		if (imethod==1)
			memcpy((void *)&node->_origPoint, (void *)&op,sizeof(struct SFColor));
		if (imethod==0)
			memcpy ((void *) &node->_origPoint,
				(void *) posn,sizeof(struct SFColor));

		/* set isActive true */
		node->isActive=TRUE;
		MARK_EVENT (ptr, offsetof (struct X3D_PlaneSensor, isActive));

	} else if ((ev==MotionNotify) && (node->isActive) && but1) {
		/* hyperhit saved in render_hypersensitive phase */
		if (imethod==0){
			//this is ray intersect plane code, for plane Z=0
			mult = (node->_origPoint.c[2] - tg->RenderFuncs.hyp_save_posn[2]) /
				(tg->RenderFuncs.hyp_save_norm[2] - tg->RenderFuncs.hyp_save_posn[2]);
			nx = tg->RenderFuncs.hyp_save_posn[0] + mult * (tg->RenderFuncs.hyp_save_norm[0] - tg->RenderFuncs.hyp_save_posn[0]);
			ny = tg->RenderFuncs.hyp_save_posn[1] + mult * (tg->RenderFuncs.hyp_save_norm[1] - tg->RenderFuncs.hyp_save_posn[1]);
		}
		if (imethod==1){
			nx = trackpoint[0]; ny = trackpoint[1];
		}
		#ifdef SEVERBOSE
		ConsoleMessage ("now, mult %f nx %f ny %f op %f %f %f\n",mult,nx,ny,
			node->_origPoint.c[0],node->_origPoint.c[1],
			node->_origPoint.c[2]);
		#endif

		/* trackpoint changed */
		if (imethod == 0){
			node->_oldtrackPoint.c[0] = nx;
			node->_oldtrackPoint.c[1] = ny;
			node->_oldtrackPoint.c[2] = node->_origPoint.c[2];
		}
		if (imethod == 1){
			veccopy3f(node->_oldtrackPoint.c, trackpoint);
		}
		/*printf(">%f %f %f\n",nx,ny,node->_oldtrackPoint.c[2]); */
		if ((APPROX(node->_oldtrackPoint.c[0],node->trackPoint_changed.c[0])!= TRUE) ||
			(APPROX(node->_oldtrackPoint.c[1],node->trackPoint_changed.c[1])!= TRUE) ||
			(APPROX(node->_oldtrackPoint.c[2],node->trackPoint_changed.c[2])!= TRUE)) {
			
			memcpy ((void *) &node->trackPoint_changed, (void *) &node->_oldtrackPoint, sizeof(struct SFColor));
			MARK_EVENT(ptr, offsetof (struct X3D_PlaneSensor, trackPoint_changed));

		}

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

		node->_oldtranslation.c[0] = tr.c[0];
		node->_oldtranslation.c[1] = tr.c[1];
		node->_oldtranslation.c[2] = tr.c[2];

		if ((APPROX(node->_oldtranslation.c[0],node->translation_changed.c[0])!= TRUE) ||
			(APPROX(node->_oldtranslation.c[1],node->translation_changed.c[1])!= TRUE) ||
			(APPROX(node->_oldtranslation.c[2],node->translation_changed.c[2])!= TRUE)) {

			memcpy ((void *) &node->translation_changed, (void *) &node->_oldtranslation, sizeof(struct SFColor));
			MARK_EVENT(ptr, offsetof (struct X3D_PlaneSensor, translation_changed));
		}

	} else if (ev==ButtonRelease) {
		/* set isActive false */
		node->isActive=FALSE;
		MARK_EVENT (ptr, offsetof (struct X3D_PlaneSensor, isActive));

		/* autoOffset? */
		if (node->autoOffset) {
			node->offset.c[0] = node->translation_changed.c[0];
			node->offset.c[1] = node->translation_changed.c[1];
			node->offset.c[2] = node->translation_changed.c[2];

			MARK_EVENT (ptr, offsetof (struct X3D_PlaneSensor, offset));
		}
	}

}


/* void do_Anchor (struct X3D_Anchor *node, int ev, int over) {*/
void do_Anchor ( void *ptr, int ev, int but1, int over) {
	struct X3D_Anchor *node = (struct X3D_Anchor *)ptr;
	UNUSED(over);
	UNUSED(but1);

	if (!node) return;
	/* try button release, so that we dont get worlds flashing past if 
	   the user keeps the finger down. :-) if (ev==ButtonPress) { */
	if (ev==ButtonRelease) {
		ttglobal tg = gglobal();
		/* no parameters in url field? */
		if (node->url.n < 1) return;
		setAnchorsAnchor( node );
		#ifdef OLDCODE
		OLDCODE FREE_IF_NZ(tg->RenderFuncs.OSX_replace_world_from_console);
		#endif // OLDCODE

		tg->RenderFuncs.BrowserAction = TRUE;
	}
}


void do_CylinderSensor ( void *ptr, int ev, int but1, int over) {
	struct X3D_CylinderSensor *node = (struct X3D_CylinderSensor *)ptr;
	double rot, radius, ang, length;
	double det, pos, neg, temp;
	double acute_angle, disk_angle, height;
	float Y[3] = { 0.0f, 1.0f, 0.0f }, ZERO[3] = { 0.0f, 0.0f, 0.0f };
	float as[3], bs[3], v[3], rps[3]; 

	int imethod;
	Quaternion bv, dir1, dir2, tempV;
	GLDOUBLE modelMatrix[16];
	ttglobal tg;

	UNUSED(over);
	
	/* if not enabled, do nothing */
	if (!node) return;
	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_CylinderSensor, enabled));
	}
	if (!node->enabled) return;

	/* only do something if the button is pressed */
	if (!but1) return;
	tg = gglobal();
	imethod = 1;
	if (imethod == 1){
		/*precompute some values for mouse-down, mouse-move*/
		//convert all almost-sensor-local points into sensor-local 
		//(the axisRotation never gets applied in the modelview transform stack - if that changes in the future, then don't need these)
		axisangle_rotate3f(as, tg->RenderFuncs.hyp_save_posn, node->axisRotation.c);
		axisangle_rotate3f(bs, tg->RenderFuncs.hyp_save_norm, node->axisRotation.c);
		vecnormalize3f(v, vecdif3f(v, bs, as));
		axisangle_rotate3f(rps,tg->RenderFuncs.ray_save_posn, node->axisRotation.c);

	}
	if (ev==ButtonPress) {
		/* record the current position from the saved position */
		if (imethod == 0){
			memcpy((void *)&node->_origPoint,
				(void *)&tg->RenderFuncs.ray_save_posn, sizeof(struct SFColor));
		}else{
			/* on mouse-down we have to decide which sensor geometry to use: disk or cylinder, as per specs
				http://www.web3d.org/files/specifications/19775-1/V3.3/Part01/components/pointingsensor.html#CylinderSensor
				and that's determined by the angle between the bearing and the sensor Y axis, in sensor-local coords
				The bearing (A,B) where A=hyp_posn, B=hyp_norm and both are points in sensor-local coordinates
				To get a direction vector v = B - A
			*/
			struct SFColor origPoint;
			/*ray_save_posn is the intersection with scene geometry, in sensor-local coordinates, for cylinder*/
			float dot, rs[3];
			dot = vecdot3f(v, Y);
			acute_angle = acos(dot);
			ang = min(acute_angle,PI - acute_angle);
			veccopy3f(rs, rps); //rps: ray_posn (intersection with scene geometry) in sensor-local
			height = rs[1];
			rs[1] = 0.0f;
			radius = veclength3f(rs); //radius of ray_posn from cylinder axis, for scaling the 'feel' of the rotations to what the user clicked
			vecnormalize3f(rs, rs);
			if (ang < node->diskAngle){
				//use end cap disks
				disk_angle = -atan2(rs[2], rs[0]);
			}else{
				//use cylinder wall
				float travelled, cylpoint[3], axispoint[3], dif[3];
				line_intersect_line_3f(as, v, ZERO, Y, NULL, NULL, cylpoint, axispoint);
				travelled = veclength3f(vecdif3f(dif, cylpoint, axispoint)); //travelled: closest distance of our bearing from cylinder axis
				if (det3f(v, dif, Y) > 0.0f) travelled = -travelled; //which side of cylinder axis is our bearing on? v x dif will point a different direction (up or down) depending on which side, so dot with Y to get a sign
				disk_angle = travelled / (2.0f * PI * radius) * (2.0f * PI); //don't need the 2PI except to show how we converted to radians: travelled is a fraction of circumference, and circumference is 2PI
			}
			node->_radius = (float)radius; //store for later use on mouse-moves
			//origPoint - we get to store whatever we need later mouse-moves. 
			origPoint.c[0] = (float)disk_angle;
			origPoint.c[1] = (float)-height; //Q. why -height? don't know but it works
			memcpy((void *)&node->_origPoint,(void *)&origPoint, sizeof(struct SFColor));
		}
		/* set isActive true */
		node->isActive=TRUE;
		MARK_EVENT (ptr, offsetof (struct X3D_CylinderSensor, isActive));

    	/* record the current Radius */
		if (imethod == 0)
		{
			node->_radius = tg->RenderFuncs.ray_save_posn[0] * tg->RenderFuncs.ray_save_posn[0] +
				tg->RenderFuncs.ray_save_posn[1] * tg->RenderFuncs.ray_save_posn[1] +
				tg->RenderFuncs.ray_save_posn[2] * tg->RenderFuncs.ray_save_posn[2];

			FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);
			/*
			printf ("Cur Matrix: \n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n",
			modelMatrix[0],  modelMatrix[4],  modelMatrix[ 8],  modelMatrix[12],
			modelMatrix[1],  modelMatrix[5],  modelMatrix[ 9],  modelMatrix[13],
			modelMatrix[2],  modelMatrix[6],  modelMatrix[10],  modelMatrix[14],
			modelMatrix[3],  modelMatrix[7],  modelMatrix[11],  modelMatrix[15]);
			*/

			/* find the bearing vector in the local coordinate system */
			pos = neg = 0.0;
			temp = modelMatrix[1] * modelMatrix[6] * modelMatrix[8];
			if (temp >= 0.0) pos += temp; else neg += temp;
			temp = -modelMatrix[2] * modelMatrix[5] * modelMatrix[8];
			if (temp >= 0.0) pos += temp; else neg += temp;
			temp = -modelMatrix[0] * modelMatrix[6] * modelMatrix[9];
			if (temp >= 0.0) pos += temp; else neg += temp;
			temp = modelMatrix[2] * modelMatrix[4] * modelMatrix[9];
			if (temp >= 0.0) pos += temp; else neg += temp;
			temp = modelMatrix[0] * modelMatrix[5] * modelMatrix[10];
			if (temp >= 0.0) pos += temp; else neg += temp;
			temp = -modelMatrix[1] * modelMatrix[4] * modelMatrix[10];
			if (temp >= 0.0) pos += temp; else neg += temp;
			det = pos + neg;
			det = 1.0 / det;

			bv.w = 0;/* set to 0 to ensure vector is normalised correctly */
			bv.x = (modelMatrix[4] * modelMatrix[9] - modelMatrix[5] * modelMatrix[8]) * det;
			bv.y = -(modelMatrix[0] * modelMatrix[9] - modelMatrix[1] * modelMatrix[8]) * det;
			bv.z = (modelMatrix[0] * modelMatrix[5] - modelMatrix[1] * modelMatrix[4]) * det;

			quaternion_normalize(&bv);
			ang = acos(bv.y);
			if (ang > (M_PI / 2)) { ang = M_PI - ang; }
		}
		if (ang < node->diskAngle) {
			node->_dlchange=TRUE; //use disk sensor geometry
		} else {
			node->_dlchange=FALSE; //use cylinder sensor geometry
		}


	}else 
	if ((ev == MotionNotify) && (node->isActive)) {

		if (imethod==0)
			memcpy((void *)&node->_oldtrackPoint, (void *)&tg->RenderFuncs.ray_save_posn, sizeof(struct SFColor));
		if (imethod == 1)
			veccopy3f(node->_oldtrackPoint.c, rps); //I'm using ray_posn, which is intersection with sensitized scene geometry. Should I be using the bearing intersect sensor_geometry?
		if ((APPROX(node->_oldtrackPoint.c[0], node->trackPoint_changed.c[0]) != TRUE) ||
			(APPROX(node->_oldtrackPoint.c[1], node->trackPoint_changed.c[1]) != TRUE) ||
			(APPROX(node->_oldtrackPoint.c[2], node->trackPoint_changed.c[2]) != TRUE)) {

			memcpy((void *)&node->trackPoint_changed, (void *)&node->_oldtrackPoint, sizeof(struct SFColor));
			MARK_EVENT(ptr, offsetof(struct X3D_CylinderSensor, trackPoint_changed));
		}

		if (imethod==0)
		{
			dir1.w = 0;
			dir1.x = tg->RenderFuncs.ray_save_posn[0];
			dir1.y = 0;
			dir1.z = tg->RenderFuncs.ray_save_posn[2];

			if (node->_dlchange) {
				radius = 1.0;  //disk
			}
			else {
				/* get the radius */
				radius = (dir1.x * dir1.x + dir1.y * dir1.y + dir1.z * dir1.z); //2D cylinder radius**2
			}

			quaternion_normalize(&dir1);
			dir2.w = 0;
			dir2.x = node->_origPoint.c[0];
			dir2.y = 0;
			dir2.z = node->_origPoint.c[2];

			quaternion_normalize(&dir2);

			tempV.w = 0;
			tempV.x = dir2.y * dir1.z - dir2.z * dir1.y;
			tempV.y = dir2.z * dir1.x - dir2.x * dir1.z;
			tempV.z = dir2.x * dir1.y - dir2.y * dir1.x;
			quaternion_normalize(&tempV);

			length = tempV.x * tempV.x + tempV.y * tempV.y + tempV.z * tempV.z;
			if (APPROX(length, 0.0)) { return; }

			/* Find the angle of the dot product */
			rot = radius * acos((dir1.x*dir2.x + dir1.y*dir2.y + dir1.z*dir2.z));

			if (APPROX(tempV.y, -1.0)) rot = -rot;
		}
		if (imethod == 1)
		{
			//compute delta rotation from drag
			//a plane P dot N = d = const, for any point P on plane. Our plane is in plane-local coords, 
			// so we could use P={0,0,0} and P dot N = d = 0
			float diskpoint[3], orig_diskangle, height;
			height = node->_origPoint.c[1];
			radius = node->_radius;
			orig_diskangle = node->_origPoint.c[0];
			if (node->_dlchange == TRUE) {
				//disk
				line_intersect_planed_3f(as, v, Y, height, diskpoint, NULL);
				vecnormalize3f(diskpoint, diskpoint);
				//for cylinder compute angle from intersection on cylinder of radius
				disk_angle = -atan2(diskpoint[2], diskpoint[0]);
				//printf("D");
			}else {
				float cylpoint[3]; //pi1[3], 
				//cylinder wall
				//ray-intersect-cylinder is too hard for us, a quadratic (but is done in rendray_Cylinder)
				//if (line_intersect_cylinder_3f(as, v, radius, cylpoint)){ //didn't work - wrong sol1,sol2 or ???
				//	//on the cylinder
				//	disk_angle = -atan2(cylpoint[2], cylpoint[0]);
				//	printf("C");
				//
				//off the cylinder (and well this works as good as the line_interesect_cylinder
				//we want a drag off the cylinder to keep working even when mouse isn't over cylinder
				//on the cylinder
				//basically we try and do a linear drag perpendicular to both our bearing and the cylinder 
				//axis, and convert that linear distance from cylinder axis from distance into rotations
				float travelled, axispoint[3], dif[3];
				radius = 1.0f;
				line_intersect_line_3f(as, v, ZERO, Y, NULL, NULL, cylpoint, axispoint);
				//cylpoint - closest point of approach of our bearing, on the bearing
				//axispoint - ditto, on the cyl axis
				//dif = cylpoint - axispoint //vector perpendicular to axis - our 'travel' from the axis
				travelled = veclength3f(vecdif3f(dif, cylpoint, axispoint));
				if (det3f(v, dif, Y) > 0.0f) travelled = -travelled; // v x dif will be up or down the cyl axis, depending on which side of the axis we are on
				disk_angle = travelled / (2.0f * PI * radius) * (2.0f * PI); //convert from distance to radians using ratio of circumference
				//printf("V");
			}
			rot = disk_angle - orig_diskangle;
		}

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

		node->_oldrotation.c[0] = (float) 0;
		node->_oldrotation.c[1] = (float) 1;
		node->_oldrotation.c[2] = (float) 0;
		node->_oldrotation.c[3] = (float) rot;

		if ((APPROX(node->_oldrotation.c[0],node->rotation_changed.c[0])!= TRUE) ||
			(APPROX(node->_oldrotation.c[1],node->rotation_changed.c[1])!= TRUE) ||
			(APPROX(node->_oldrotation.c[2],node->rotation_changed.c[2])!= TRUE) ||
			(APPROX(node->_oldrotation.c[3],node->rotation_changed.c[3])!= TRUE)) {

			memcpy ((void *) &node->rotation_changed, (void *) &node->_oldrotation, sizeof(struct SFRotation));
			MARK_EVENT(ptr, offsetof (struct X3D_CylinderSensor, rotation_changed));
		}


	} else if (ev==ButtonRelease) {
		/* set isActive false */
		node->isActive=FALSE;
		MARK_EVENT (ptr, offsetof (struct X3D_CylinderSensor, isActive));
		/* save auto offset of rotation */
		if (node->autoOffset) {
			memcpy ((void *) &node->offset,
				(void *) &node->rotation_changed.c[3],
				sizeof (float));

		MARK_EVENT (ptr, offsetof (struct X3D_CylinderSensor, rotation_changed));
		}
	}
}



#define ORIG_X node->_origPoint.c[0]
#define ORIG_Y node->_origPoint.c[1]
#define ORIG_Z node->_origPoint.c[2]
#define NORM_ORIG_X node->_origNormalizedPoint.c[0]
#define NORM_ORIG_Y node->_origNormalizedPoint.c[1]
#define NORM_ORIG_Z node->_origNormalizedPoint.c[2]
#define CUR_X  tg->RenderFuncs.ray_save_posn[0]
#define CUR_Y  tg->RenderFuncs.ray_save_posn[1]
#define CUR_Z  tg->RenderFuncs.ray_save_posn[2]
#define NORM_CUR_X normalizedCurrentPoint.c[0]
#define NORM_CUR_Y normalizedCurrentPoint.c[1]
#define NORM_CUR_Z normalizedCurrentPoint.c[2]
#define RADIUS node->_radius

/********************************************************************************/
/*										*/
/* do the guts of a SphereSensor.... this has been changed considerably in Apr	*/
/* 2009 because the original, fast methods created by Tuomas Lukka failed in 	*/
/* a boundary area (HUD, small transform scale, close to viewer) and I could 	*/
/* not understand what *exactly* Tuomas' code did - I guess I don't have a 	*/
/* doctorate in math like he does! I went to the old linear algebra text and	*/
/* created a simple but inelegant solution from that. J.A. Stewart.		*/
/*										*/
/********************************************************************************/
void do_SphereSensor ( void *ptr, int ev, int but1, int over) {
	struct X3D_SphereSensor *node = (struct X3D_SphereSensor *)ptr;
/*
	int tmp;
	float tr1sq, tr2sq, tr1tr2;
	struct SFColor dee, arr, cp, dot;
	float deelen, aay, bee, cee, und, sol, cl, an;
	Quaternion q, q2, q_r;
	double s1,s2,s3,s4;
*/
	ttglobal tg;
	UNUSED(over);

	/* if not enabled, do nothing */
	if (!node) 
		return;
	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_SphereSensor, enabled));
	}
	if (!node->enabled) 
		return;

	/* only do something if button1 is pressed */
	if (!but1) return;
	tg = gglobal();
	if (ev==ButtonPress) {
		/* record the current position from the saved position */
		ORIG_X = CUR_X;
		ORIG_Y = CUR_Y;
		ORIG_Z = CUR_Z;

		/* record the current Radius */
		RADIUS = (float) sqrt(CUR_X * CUR_X + CUR_Y * CUR_Y + CUR_Z * CUR_Z);

		if (APPROX(RADIUS,0.0)) {
			printf ("warning, RADIUS %lf == 0, can not compute\n",RADIUS);
			return;
		}

		/* save the initial norm here */
		NORM_ORIG_X = CUR_X / RADIUS;
		NORM_ORIG_Y = CUR_Y / RADIUS;
		NORM_ORIG_Z = CUR_Z / RADIUS;

		/* norm(offset) ideally this would be done once during parsing 
		  of crazy SFRotation ie '1 1 -5 .6' in 10.wrl/10.x3d 
		  I might be getting rounding errors from repeated normalization */
		vrmlrot_normalize(node->offset.c); 

		/* set isActive true */
		node->isActive=TRUE;
		MARK_EVENT (ptr, offsetof (struct X3D_SphereSensor, isActive));

	} else if (ev==ButtonRelease) {
		/* set isActive false */
		node->isActive=FALSE;
		MARK_EVENT (ptr, offsetof (struct X3D_SphereSensor, isActive));

		if (node->autoOffset) {
			memcpy ((void *) &node->offset,
				(void *) &node->rotation_changed,
				sizeof (struct SFRotation));
		}
	} else if ((ev==MotionNotify) && (node->isActive)) {
		
		double dotProd;
		double newRad;
		struct SFColor normalizedCurrentPoint;
		struct point_XYZ newA;

		/* record the current Radius */
		newRad = sqrt(CUR_X * CUR_X + CUR_Y * CUR_Y + CUR_Z * CUR_Z);

		/* bounds check... */
		if (APPROX(newRad,0.0)) {
			printf ("warning, newRad %lf == 0, can not compute\n",newRad);
			return;
		}
		RADIUS = (float) newRad;

		/* save the current norm here */
		NORM_CUR_X = CUR_X / RADIUS;
		NORM_CUR_Y = CUR_Y / RADIUS;
		NORM_CUR_Z = CUR_Z / RADIUS;

		/* find the cross-product between the initial and current points */
		newA.x = ORIG_Y * CUR_Z - ORIG_Z * CUR_Y;
		newA.y = ORIG_Z * CUR_X - ORIG_X * CUR_Z;
		newA.z = ORIG_X * CUR_Y - ORIG_Y * CUR_X;
		normalize_vector(&newA);

		/* clamp the angle to |a| < 1.0 */
		/* remember A dot B = |A|*|B|*cos(theta_between) or theta_between = acos(A dot B/|A|*|B| ) */
		dotProd = NORM_ORIG_X * NORM_CUR_X + NORM_ORIG_Y * NORM_CUR_Y + NORM_ORIG_Z * NORM_CUR_Z;
		if (dotProd > 1.0) 
			dotProd = 1.0;
		if (dotProd < -1.0) 
			dotProd = -1.0;
		dotProd = acos(dotProd);

		/* have axis-angle now */
		/*
		printf ("newRotation  a %lf - rot -- %lf %lf %lf %lf\n",
			dotProd, newA.x,newA.y,newA.z,dotProd);
		*/
		if(node->autoOffset)
		{
/*
			if(0)
			{
				//Aug 1, 2010 experimental code - stale date: Sept 1
				struct SFRotation temp, temp2;
				temp.c[0] = newA.x;
				temp.c[1] = newA.y;
				temp.c[2] = newA.z;
				temp.c[3] = dotProd;
				vrmlrot_multiply(temp2.c, node->offset.c, temp.c);
				newA.x = temp2.c[0];
				newA.y = temp2.c[1];
				newA.z = temp2.c[2];
				dotProd = temp2.c[3];
			}
			if(1)
			{
*/
				/* copied from the javascript SFRotationMultiply */
				Quaternion q1, q2, qret;
				/* convert both rotations into quaternions */
				vrmlrot_to_quaternion(&q1, (double) newA.x, 
					(double) newA.y, (double) newA.z, (double) dotProd);
				vrmlrot_to_quaternion(&q2, (double) node->offset.c[0], 
					(double) node->offset.c[1], (double) node->offset.c[2], (double) node->offset.c[3]);
				/* multiply them */
				quaternion_multiply(&qret,&q1,&q2);
				/* and return the resultant, as a vrml rotation */
				quaternion_to_vrmlrot(&qret, &newA.x, &newA.y, &newA.z, &dotProd);
			/*}*/
		}


		node->rotation_changed.c[0] = (float) newA.x;
		node->rotation_changed.c[1] = (float) newA.y;
		node->rotation_changed.c[2] = (float) newA.z;
		node->rotation_changed.c[3] = (float) dotProd; //acos(dotProd); done above
		MARK_EVENT (ptr, offsetof (struct X3D_SphereSensor, rotation_changed));

		node->trackPoint_changed.c[0] = NORM_CUR_X;
		node->trackPoint_changed.c[1] = NORM_CUR_Y;
		node->trackPoint_changed.c[2] = NORM_CUR_Z;
		MARK_EVENT (ptr, offsetof (struct X3D_SphereSensor, trackPoint_changed));
	}
}
