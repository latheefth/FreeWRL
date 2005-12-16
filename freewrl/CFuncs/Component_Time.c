/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Time Component

*********************************************************************/

#include "headers.h"

/* void do_TimeSensorTick (struct X3D_TimeSensor *node) {*/
void do_TimeSensorTick ( void *ptr) {
	struct X3D_TimeSensor *node = (struct X3D_TimeSensor *)ptr;
	double myDuration;
	int oldstatus;
	double myTime;
	double frac;

	/* are we not enabled */
	if (!node) return;

	if (!node->enabled) {
		if (node->isActive) {
			node->isActive=0;
			mark_event (ptr, offsetof(struct X3D_TimeSensor, isActive));
		}
		return;
	}

	/* can we possibly have started yet? */
	if(TickTime < node->startTime) {
		return;
	}

	oldstatus = node->isActive;
	myDuration = node->cycleInterval;

	/* call common time sensor routine */
	do_active_inactive (
		&node->isActive, &node->__inittime, &node->startTime,
		&node->stopTime,node->loop,(float)(node->cycleInterval), (float)1.0);


	/* now process if we have changed states */
	if (oldstatus != node->isActive) {
		if (node->isActive == 1) {
			/* force code below to generate event */
			node->__ctflag = 10.0;
		}

		/* push @e, [$t, "isActive", node->{isActive}]; */
		mark_event (ptr, offsetof(struct X3D_TimeSensor, isActive));
	}


	if(node->isActive == 1) {
		/* set time field */
		node->time = TickTime;
		mark_event (ptr,
				offsetof(struct X3D_TimeSensor, time));

		/* calculate what fraction we should be */
 		myTime = (TickTime - node->startTime) / myDuration;

		if (node->loop) {
			frac = myTime - (int) myTime;
		} else {
			frac = (myTime > 1 ? 1 : myTime);
		}

		#ifdef SEVERBOSE
		printf ("TimeSensor myTime %f frac %f dur %f\n", myTime,frac,myDuration);
		#endif

		/* cycleTime events once at start, and once every loop. */
		if (frac < node->__ctflag) {
			/* push @e, [$t, cycleTime, $TickTime]; */
			node->cycleTime = TickTime;
			mark_event (ptr, offsetof(struct X3D_TimeSensor, cycleTime));
		}
		node->__ctflag = frac;

		/* time  and fraction_changed events */
		/* push @e, [$t, "time", $TickTime];
		push @e, [$t, fraction_changed, $frac]; */
		node->fraction_changed = frac;
		mark_event (ptr, offsetof(struct X3D_TimeSensor, fraction_changed));

	}
}

