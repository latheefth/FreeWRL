/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

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

/*****************************************
C Routing Methodology:

Different nodes produce eventins/eventouts...

	EventOuts only:
		MovieTexture
		AudioClip
		TimeSensor
		TouchSensor
		PlaneSensor
		SphereSensor
		CylinderSensor
		VisibilitySensor
		ProximitySensor
	
	EventIn/EventOuts:
		ScalarInterpolator
		OrientationInterpolator
		ColorInterpolator
		PositionInterpolator
		NormalInterpolator
		CoordinateInterpolator
		Fog
		Background
		Viewpoint
		NavigationInfo
		Collision
	
	EventIns only:
		Almost everything else...


	Nodes with ClockTicks:
		MovieTexture, AudioClip, TimeSensor, 
		ProximitySensor, Collision, ...?

	Nodes that have the EventsProcessed method:
		ScalarInterpolator, OrientationInterpolator,
		ColorInterpolator, PositionInterpolator,
		NormalInterpolator,  (should be all the interpolators)

		SCRIPT

	

	So, in the event loop, (Events.pm, right now), the call to

		push @e, $_->get_firstevent($timestamp);

	Does all the nodes with clock ticks - ie, it starts off
	generating a series of events.

		push @ne,$_->events_processed($timestamp,$be);
	
	goes through the list of routes, copies the source event to
	the destination, and if the node is one of the EventIn/EventOut
	style, it then re-tells the routing table to do another route.
	The table is gone through until all events are done with.
	
		 

	--------------------------------------------------------------------------	
	C Routes are stored in a table with the following entries:
		Fromnode 	- the node that created an event address
		actual ptr	- pointer to the exact field within the address
		Tonode		- destination node address
		actual ptr	- pointer to the exact field within the address
		active		- True of False for each iteration
		length		- data field length
		interpptr	- pointer to an interpolator node, if this is one
******************************************/

struct CRStruct {
	unsigned int	fromnode;
	unsigned int	fnptr;
	unsigned int	tonode;
	unsigned int	tnptr;
	int	act;
	int	len;
	void	(*interpptr)(void *);
};

struct CRStruct CRoutes[MAXROUTES];

static int CRoutes_Initiated = FALSE;

int CRoutes_Count;

int CRVerbose = 0;

/* internal variable to copy a C structure's Multi* field */
void Multimemcpy (void *tn, void *fn, int len) {
	struct Multi_Vec3f *mv3ffn;
	struct Multi_Vec3f *mv3ftn;

	if (CRVerbose) printf ("MultiMemcpy to %x from %x len %d\n",tn,fn,len);

	if (len == -1) {
		/* this is a Multi_Vec3f */
		mv3ffn = fn;
		mv3ftn = tn;
		memcpy (mv3ftn->p,mv3ffn->p,sizeof(struct SFColor) * mv3ftn->n);
	} else {
		printf ("Multimemcpy, don't handle type %d yet\n",len);
	}
}

/********************************************************************

CRoutes_Register. 

Register a route in the routing table.

********************************************************************/


unsigned int CRoutes_Register (unsigned int from, int fromoffset, 
			unsigned int to, int tooffset,
			int length, void *intptr) {

	int insert_here;
	int shifter;


	/* first time through, create minimum and maximum for insertion sorts */
	if (!CRoutes_Initiated) {
		CRoutes[0].fromnode = 0;
		CRoutes[0].fnptr=0;
		CRoutes[0].act=FALSE;
		CRoutes[0].interpptr=0;
		CRoutes[1].fromnode =0x8FFFFFFF;
		CRoutes[1].fnptr = 0x8FFFFFFF;
		CRoutes[1].act=FALSE;
		CRoutes[1].interpptr=0;
		CRoutes_Count = 2;
		CRoutes_Initiated = TRUE;
	}

if (CRVerbose) printf ("CRoutes_Register from %x off %x to %x off %x len %d intptr %x \n",from, fromoffset,
		to,tooffset,length, intptr);

	insert_here = 1;

	/* go through the routing list, finding where to put it */
	while (from >= CRoutes[insert_here].fromnode) {
		if (CRVerbose) printf ("comparing %x to %x\n",from, CRoutes[insert_here].fromnode);
		insert_here++; 
	}

	/* hmmm - do we have a route from this node already? If so, go
	   through and put the offsets in order */
	while ((from==CRoutes[insert_here].fromnode) &&
		((from + fromoffset) < CRoutes[insert_here].fnptr)) { 
		if (CRVerbose) printf ("same fromnode, different offset\n");
		insert_here++;
	}

	if (CRVerbose) printf ("CRoutes, inserting at %d\n",insert_here);

	/* create the space for this entry. */
	for (shifter = CRoutes_Count; shifter > insert_here; shifter--) {
		memcpy ((void *)&CRoutes[shifter], (void *)&CRoutes[shifter-1],sizeof(struct CRStruct));
		if (CRVerbose) printf ("Copying from index %d to index %d\n",shifter, shifter-1);
	}

	/* and put it in */
	CRoutes[insert_here].fromnode = from;
	CRoutes[insert_here].fnptr = fromoffset;
	CRoutes[insert_here].act = FALSE;
	CRoutes[insert_here].tonode = to;
	CRoutes[insert_here].len = length;
	CRoutes[insert_here].tnptr = tooffset;	
	CRoutes[insert_here].interpptr = intptr;

	/* record that we have one more route, with upper limit checking... */
	if (CRoutes_Count >= (MAXROUTES-2)) {
		if (CRVerbose) printf ("Maximum number of routes exceeded\n");
	} else {
		CRoutes_Count ++;
	}

	for (shifter = 1; shifter < (CRoutes_Count-1); shifter ++) {
		if (CRVerbose) printf ("Route indx %d is (%x %x) to (%x %x) len %d\n",
			shifter, CRoutes[shifter].fromnode,
			CRoutes[shifter].fnptr, CRoutes[shifter].tonode,
			CRoutes[shifter].tnptr, CRoutes[shifter].len);
	}

}

/********************************************************************

mark_event - something has generated an eventOut; record the node
data structure pointer, and the offset. Mark all relevant entries
in the routing table that this node/offset triggered an event.

********************************************************************/

void mark_event (unsigned int from, unsigned int totalptr) {
	int findit;

	if (!CRoutes_Initiated) return;  /* no routes registered yet */

	findit = 1;

	if (CRVerbose) printf ("mark_event, from %x fromoffset %x\n",from,totalptr);

	/* events in the routing table are sorted by fromnode. Find
	   out if we have at least one route from this node */
	while (from > CRoutes[findit].fromnode) findit ++;

	/* while we have an eventOut from this NODE/OFFSET, mark it as 
	   active. If no event from this NODE/OFFSET, ignore it */
	while ((from == CRoutes[findit].fromnode) &&
		(totalptr != CRoutes[findit].fnptr)) findit ++;

	/* did we find the exact entry? */
	if (CRVerbose) printf ("ep, (%x %x) (%x %x) at %d \n",from,CRoutes[findit].fromnode,
		totalptr,CRoutes[findit].fnptr,findit);

	/* if we did, signal it to the CEvents loop  - maybe more than one ROUTE,
	   eg, a time sensor goes to multiple interpolators */
	while ((from == CRoutes[findit].fromnode) && 
		(totalptr == CRoutes[findit].fnptr)) {
		if (CRVerbose) printf ("found it at %d\n",findit);
		CRoutes[findit].act=TRUE;
		findit ++;
	}
	if (CRVerbose) printf ("done mark_event\n");
}


/********************************************************************

propagate_events.

Go through the event table, until the table is "active free". Some
nodes have eventins/eventouts - have to do the table multiple times
in this case.

********************************************************************/

void propagate_events() {
	int counter;
	int havinterp;
	int mvcompCount, mvcompSize;
	struct Multi_Vec3f *mv3fptr;


	if (CRVerbose) printf ("propagate_events\n");


	do {
		havinterp=FALSE; /* assume no interpolators triggered */

		for (counter = 1; counter < CRoutes_Count-1; counter++) {
			if (CRVerbose) printf ("propagate_events, counter %d from %x off %x to %x off %x oint %x\n",
				counter,CRoutes[counter].fromnode,CRoutes[counter].fnptr,
				CRoutes[counter].tonode,CRoutes[counter].tnptr,
				CRoutes[counter].interpptr);


			if (CRoutes[counter].act == TRUE) {
				if (CRVerbose) printf ("event %x %x sent something\n",CRoutes[counter].fromnode,
					CRoutes[counter].fnptr);

				/* we have this event found */
				CRoutes[counter].act = FALSE;

				/* copy the value over */
				if (CRoutes[counter].len>0) {
					memcpy (CRoutes[counter].tonode + CRoutes[counter].tnptr, 
						CRoutes[counter].fromnode + CRoutes[counter].fnptr,
						CRoutes[counter].len);
				} else {
					/* this is a Multi*node, do a specialized copy */
					Multimemcpy (CRoutes[counter].tonode + CRoutes[counter].tnptr, 
						CRoutes[counter].fromnode + CRoutes[counter].fnptr,
						CRoutes[counter].len);
				}

				/* is this an interpolator? if so call the code to
				   do it */
				if (CRoutes[counter].interpptr != 0) {
					/* this is an interpolator, call it */
					havinterp = TRUE;
					if (CRVerbose) printf ("propagate_events. index %d is an interpolator\n",counter);
					CRoutes[counter].interpptr(CRoutes[counter].tonode);
				} 
				else {	
					/* just an eventIn node. signal to the reciever to update */
					update_node(CRoutes[counter].tonode);
				}

				/* if this is an interpolator type, run it to generate more
				   events */
			}
		}
	} while (havinterp==TRUE);

	if (CRVerbose) printf ("done propagate_events\n");
}
