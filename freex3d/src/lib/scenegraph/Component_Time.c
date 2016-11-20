/*


X3D Time Component

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

#include "../vrml_parser/Structs.h"
#include "../vrml_parser/CRoutes.h"
#include "../main/headers.h"
#include "../input/SensInterps.h"


void do_active_inactive_0 (
	int *act, 		/* pointer to are we active or not?	*/
	double *inittime,	/* pointer to nodes inittime		*/
	double *startt,		/* pointer to nodes startTime		*/
	double *stopt,		/* pointer to nodes stop time		*/
	int loop,		/* nodes loop field			*/
	double myDuration,	/* duration of cycle			*/
	double speed,		/* speed field				*/
	double elapsedTime   /* cumulative non-paused time */
);

/*
	Nov 2016 before:
	- verifying against NIST http://www.web3d.org/x3d/content/examples/ConformanceNist/Sensors/TimeSensor/index.html
	x win32 desktop fails 6 nist tests ie:
		stopeqstartlooptrue.x3d- freewrl doesn't animate when world is loaded, starts 5 seconds after
		stopgtstartloopfalse.x3d - freewrl wrong on startup - moves 4 seconds
	x time was 0 on startup, should be time since 1970 ie 1479495634.873 seconds 
	- android, uwp: NIST working properly, (android 1970, uwp something > 0 on startup)
	- created pause_resume.x3d example: NIST has no pause/resume examnple, 
		x and not web3d member so don't have full test suite) 
	- sample pause_resume.x3d works properly with vivaty, octaga
	x pause_resume has no effect in freewrl
	x looks like __inittime is set 0 on startup and never changed
	Nov 2016 CHANGES:
	0. fixed win32 desktop ticktime to secconds from 1970
	1. added pause resume snippet from do_audiotick / do_movietexturetick
	2. added __lasttime to help compute cumulative elapsedTime
	3. sent/marked elapsedTime events as per spec
	4. set __inittime to TickTime on startup
	Nov 2016 after:
	Nist tests: pass
	pause_rexume.x3d: pass
*/


/* void do_TimeSensorTick (struct X3D_TimeSensor *node) {*/
void do_TimeSensorTick ( void *ptr) {
	struct X3D_TimeSensor *node = (struct X3D_TimeSensor *)ptr;
	double duration;
	int oldstatus;
	double myFrac;
	double frac;

	/* are we not enabled */
	if (!node) return;

	if(node->__inittime == 0.0)
		node->__inittime = TickTime();

	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_TimeSensor, enabled));
	}
	if (!node->enabled) {
		if (node->isActive) {
			node->isActive=0;
			MARK_EVENT (ptr, offsetof(struct X3D_TimeSensor, isActive));
		}
		return;
	}

	/* can we possibly have started yet? */
	if(TickTime() < node->startTime) {
		return;
	}

	oldstatus = node->isActive;
	duration = node->cycleInterval;

	/* call common time sensor routine */
	/*
		printf ("cycleInterval %f \n",node->cycleInterval);

		uncomment the following to ensure that the gcc bug
		in calling doubles/floats in here is not causing us
		problems again...

		static int count = 0;
		if(count == 0){
			printf ("calling ");
			printf ("act %d ",node->isActive);
			printf ("initt %lf ",node->__inittime);
			printf ("startt %lf ",node->startTime);
			printf ("stopt %lf ",node->stopTime);
			printf ("loop %d ",node->loop);
			printf ("duration %f ",(float) duration);
			printf ("speed %f\n",(float) 1.0);
		}
		count++;
	*/
	//if(node->__inittime != 0.0)
	//	printf("TimeSensor.__inittime = %lf",node->__inittime);
	//if(0) do_active_inactive (
	//	&node->isActive, &node->__inittime, &node->startTime,
	//	&node->stopTime,node->loop,duration, 1.0);

	do_active_inactive_0 (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,node->loop,duration, 1.0,node->elapsedTime);

	/* MARK_SFNODE_INOUT_EVENT(node->metadata, node->__oldmetadata, offsetof (struct X3D_TimeSensor, metadata)) */

	/* now process if we have changed states */
	if (oldstatus != node->isActive) {
		if (node->isActive == 1) {
			/* force code below to generate event */
			node->__ctflag = 10.0;
			node->__lasttime = TickTime();
			node->elapsedTime = 0.0;
		}
		/* push @e, [$t, "isActive", node->{isActive}]; */
		MARK_EVENT (ptr, offsetof(struct X3D_TimeSensor, isActive));
	}


	if(node->isActive){
		if(node->pauseTime > node->startTime){
			if( node->resumeTime < node->pauseTime && !node->isPaused){
				node->isPaused = TRUE;
				MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_TimeSensor, isPaused));
			}else if(node->resumeTime > node->pauseTime && node->isPaused){
				node->isPaused = FALSE;
				node->__lasttime = TickTime();
				MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_TimeSensor, isPaused));
			}
		}
	}

	if(node->isActive == 1 && node->isPaused == FALSE) {
		/* set time field */
		node->time = TickTime();
		MARK_EVENT (ptr, offsetof(struct X3D_TimeSensor, time));
		node->elapsedTime += node->time - node->__lasttime;
		node->__lasttime = node->time; 
		/* calculate what fraction we should be */
 		//myTime = (TickTime() - node->startTime) / duration;
		myFrac = node->elapsedTime / duration;
		if (node->loop) {
			frac = myFrac - (int) myFrac;
		} else {
			frac = (myFrac > 1 ? 1 : myFrac);
		}

		#ifdef SEVERBOSE
		printf ("TimeSensor myFrac %f frac %f dur %f\n", myFrac,frac,duration);
		#endif

		/* cycleTime events once at start, and once every loop. */
		if (frac < node->__ctflag) {
			/* push @e, [$t, cycleTime, $TickTime]; */
			node->cycleTime = TickTime();
			MARK_EVENT (ptr, offsetof(struct X3D_TimeSensor, cycleTime));
		}
		node->__ctflag = frac;

		node->fraction_changed = (float) frac;
		MARK_EVENT (ptr, offsetof(struct X3D_TimeSensor, fraction_changed));

	}
}

