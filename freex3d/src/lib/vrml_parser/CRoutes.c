/*


???

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
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldGet.h"
#include "../world_script/fieldSet.h"
#include "CParseParser.h"
#include "CParseLexer.h"
#include "../input/SensInterps.h"
#include "../scenegraph/Component_ProgrammableShaders.h"
#include "../input/EAIHeaders.h"
#include "../input/EAIHelpers.h"		/* for verify_Uni_String */
#ifdef HAVE_OPENCL
#include "../opencl/OpenCL_Utils.h"
#endif //HAVE_OPENCL


#include "CRoutes.h"
//#define CRVERBOSE 1

/* static void Multimemcpy (struct X3D_Node *toNode, struct X3D_Node *fromNode, void *tn, void *fn, size_t multitype); */
static void sendScriptEventIn(int num);
static struct X3D_Node *returnSpecificTypeNode(int requestedType, int *offsetOfsetValue, int *offsetOfvalueChanged);

/* fix usage-before-definition for this function */
#ifdef HAVE_OPENCL
static bool canRouteOnGPUTo(struct X3D_Node *me);
#endif

///* we count times through the scenegraph; helps to break routing loops */
//static int thisIntTimeStamp = 1;

/* declared and defined in fieldGet.c(.h) , do not declare it here */
/* void setMFElementtype (int num); */

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
		GeoProximitySensor

	EventIn/EventOuts:
		ScalarInterpolator
		OrientationInterpolator
		ColorInterpolator
		PositionInterpolator
		GeoPositionInterpolator
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
		.... ??




	--------------------------------------------------------------------------
	C Routes are stored in a table with the following entries:
		Fromnode 	- the node that created an event address
		actual ptr	- pointer to the exact field within the address
		Tonode		- destination node address
		actual ptr	- pointer to the exact field within the address
		active		- True of False for each iteration
		length		- data field length
		interpptr	- pointer to an interpolator node, if this is one



	SCRIPTS handled like this:

		1) a call is made to        CRoutes_js_new (num,cx,glob,brow);
		   with the script number (0 on up), script context, script globals,
		   and browser data.

		2) Initialize called;


		3) scripts that have eventIns have the values copied over and
		   sent to the script by the routine "sendScriptEventIn".

		4) scripts that have eventOuts have the eventOut values copied over
		   and acted upon by the routine "gatherScriptEventOuts".


******************************************/
///* Routing table */
//struct CRStruct *_CRoutes;
//static int CRoutes_Initiated = FALSE;
//int CRoutes_Count;
//int CRoutes_MAX;



///* Structure table */
//struct CRscriptStruct *_ScriptControl = 0; 	/* global objects and contexts for each script */
//int *scr_act = 0;				/* this script has been sent an eventIn */
//int max_script_found = -1;			/* the maximum script number found */
//int max_script_found_and_initialized = -1;	/* the maximum script number found */

///* EAI needs the extra parameter, so we put it globally when a RegisteredListener is clicked. */
//int CRoutesExtra = 0;

/* global return value for getting the value of a variable within Javascript */
//jsval JSglobal_return_val;
//void *JSSFpointer;

/* ClockTick structure for processing all of the initevents - eg, TimeSensors */
struct FirstStruct {
	void *	tonode;
	void (*interpptr)(void *);
};



/* We buffer route registrations, JUST in case a registration comes from executing a route; eg,
from within a Javascript function invocation createVrmlFromURL call that was invoked by a routing
call */

struct CR_RegStruct {
		int adrem;
		struct X3D_Node *from;
		int fromoffset;
		struct X3D_Node *to;
		int toOfs;
		int fieldType;
		void *intptr;
		int scrdir;
		int extra;
#ifdef HAVE_OPENCL
    cl_kernel CL_Interpolator;
#endif //HAVE_OPENCL
};


//static struct Vector* routesToRegister = NULL;


/* if we get mark_events sent, before routing is established, save them and use them
   as soon as routing is here */
#define POSSIBLEINITIALROUTES 1000
//static int initialEventBeforeRoutesCount = 0;
//static int preRouteTableSize = 0;
struct initialRouteStruct {
	struct X3D_Node *from;
	size_t totalptr;
};
//static struct initialRouteStruct *preEvents = NULL;
//pthread_mutex_t  preRouteLock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_PREROUTETABLE                pthread_mutex_lock(&p->preRouteLock);
#define UNLOCK_PREROUTETABLE              pthread_mutex_unlock(&p->preRouteLock);

//pthread_mutex_t  insertRouteLock = PTHREAD_MUTEX_INITIALIZER;
#define MUTEX_LOCK_ROUTING_UPDATES                pthread_mutex_lock(&p->insertRouteLock);
#define MUTEX_FREE_LOCK_ROUTING_UPDATES		pthread_mutex_unlock(&p->insertRouteLock);




typedef struct pCRoutes{
	/* ClockTick structure and counter */
	struct FirstStruct *ClockEvents;// = NULL;
	int num_ClockEvents;// = 0;
	int size_ClockEvents;
	int CRoutes_Initiated;// = FALSE;
	int CRoutes_Count;
	int CRoutes_MAX;
	int initialEventBeforeRoutesCount;// = 0;
	int preRouteTableSize;// = 0;
	struct initialRouteStruct *preEvents;// = NULL;
	pthread_mutex_t  preRouteLock;// = PTHREAD_MUTEX_INITIALIZER;
	struct Vector* routesToRegister;// = NULL;
	pthread_mutex_t  insertRouteLock;// = PTHREAD_MUTEX_INITIALIZER;
	/* we count times through the scenegraph; helps to break routing loops */
	int thisIntTimeStamp;// = 1;
	/* Routing table */
	struct CRStruct *CRoutes;
	/* Structure table */
	struct CRscriptStruct *ScriptControl;// = 0; 	/* global objects and contexts for each script */
	int JSMaxScript;// = 0;
	/* Script name/type table */
	struct CRjsnameStruct *JSparamnames;// = NULL;


}* ppCRoutes;
void *CRoutes_constructor(){
	void *v = malloc(sizeof(struct pCRoutes));
	memset(v,0,sizeof(struct pCRoutes));
	return v;
}
void CRoutes_init(struct tCRoutes *t){
	//public
	/* EAI needs the extra parameter, so we put it globally when a RegisteredListener is clicked. */
	t->CRoutesExtra = 0;
	t->scr_act = 0;				/* this script has been sent an eventIn */
	t->max_script_found = -1;			/* the maximum script number found */
	t->max_script_found_and_initialized = -1;	/* the maximum script number found */
	t->jsnameindex = -1;
	t->MAXJSparamNames = 0;

	//private
	t->prv = CRoutes_constructor();
	{
		ppCRoutes p = (ppCRoutes)t->prv;
		/* ClockTick structure and counter */
		p->size_ClockEvents = 1; //pre-allocated size (will be power of 2)
		p->ClockEvents = MALLOC(struct FirstStruct*, p->size_ClockEvents * sizeof(struct FirstStruct));
		p->num_ClockEvents = 0;  
		p->CRoutes_Initiated = FALSE;
		//p->CRoutes_Count;
		//p->CRoutes_MAX;
		p->initialEventBeforeRoutesCount = 0;
		p->preRouteTableSize = 0;
		p->preEvents = NULL;
		//pthread_mutex_t  preRouteLock = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_init(&(p->preRouteLock), NULL);
		p->routesToRegister = NULL;
		//pthread_mutex_t  insertRouteLock = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_init(&(p->insertRouteLock), NULL);
		/* we count times through the scenegraph; helps to break routing loops */
		p->thisIntTimeStamp = 1;
		/* Routing table */
		//p->CRoutes;
		/* Structure table */
		p->ScriptControl = 0; 	/* global objects and contexts for each script */
		p->JSMaxScript = 0;
		/* Script name/type table */
		p->JSparamnames = NULL;

	}
}
//	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
struct CRStruct *getCRoutes()
{
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	return p->CRoutes;
}
int getCRouteCount(){
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	return p->CRoutes_Count;
}



/* a Script (JavaScript or CLASS) has given us an event, tell the system of this */
/* tell this node now needs to redraw  - but only if it is not a script to
   script route - see CRoutes_Register here, and check for the MALLOC in that code.
   You should see that the offset is zero, while in real nodes, the offset of user
   accessible fields is NEVER zero - check out CFuncs/Structs.h and look at any of
   the node types, eg, X3D_IndexedFaceSet  the first offset is for X3D_Virt :=)
*/

void markScriptResults(struct X3D_Node * tn, int tptr, int route, void * tonode) {
	ppCRoutes p;
	ttglobal tg = gglobal();
	p = (ppCRoutes)tg->CRoutes.prv;

	if (tptr != 0) {
		#ifdef CRVERBOSE
		printf ("markScriptResults: can update this node %p %d\n",tn,tptr); 
		#endif
		update_node(tn);
	#ifdef CRVERBOSE
	} else {
		printf ("markScriptResults: skipping this node %p %d flag %d\n",tn,tptr,p->CRoutes[route].direction_flag); 
	#endif
	}

	MARK_EVENT (p->CRoutes[route].routeFromNode,p->CRoutes[route].fnptr);

	/* run an interpolator, if one is attached. */
	if (p->CRoutes[route].interpptr != 0) {
		/* this is an interpolator, call it */
		tg->CRoutes.CRoutesExtra = p->CRoutes[route].extra; /* in case the interp requires it... */
		#ifdef CRVERBOSE 
		printf ("script propagate_events. index %d is an interpolator\n",route);
		#endif
		p->CRoutes[route].interpptr(tonode);
	}
}


void AddRemoveSFNodeFieldChild(
		struct X3D_Node *parent,
		struct X3D_Node **tn,  //target SFNode field
		struct X3D_Node *child,  //node to set,add or remove from parent
		int ar,  //0=set,1=add,2=remove
		char *file,
		int line) {

/*
ConsoleMessage ("AddRemoveSFNodeFieldChild called at %s:%d",file,line);
ConsoleMessage ("AddRemoveSFNodeFieldChild, parent %p, child to add offset %p, child to add %p ar %d",parent,tn,child,ar);
if (child!=NULL) ConsoleMessage ("AddRemoveSFNodeFieldChild, parent is a %s, child is a %s",stringNodeType(parent->_nodeType), stringNodeType(child->_nodeType));
if (*tn == NULL) ConsoleMessage ("toNode field is NULL"); else {ConsoleMessage ("tn field is ptr %p",*tn); ConsoleMessage ("and it ias %s",stringNodeType(X3D_NODE(*tn)->_nodeType));}
*/

	if ((parent==NULL) || (child == NULL)) {
		//printf ("Freewrl: AddRemoveSFNodeFieldChild, parent and/or child NULL\n");
		return;
	}


	/* mark the parent changed, eg, rootNode() will not be sorted if this is not marked */
	parent->_change ++;
	
	// Note that, with SFNodeFields, either a "set" or an "add" do the same thing, as 
	// we only have 1 child. MFNodes are different, but we keep the same calling conventions
	// as AddRemoveChildren for simplicity

	if ((ar == 0) || (ar == 1)) {
		#ifdef CRVERBOSE
		printf ("we have to perform a \"set_child\" on this field\n");
		# endif

		/* go to the old child, and tell them that they are no longer wanted here */
		if (*tn != NULL) remove_parent(*tn,parent);

		/* addChild - now lets add */
		*tn = child;
		ADD_PARENT(child,parent);
	} else {
		/* this is a removeChild - check to see if child is correct. We might have
		   a removeChild of NULL, for instance */

		if (child != NULL) {
			if(child == *tn){
				remove_parent(*tn,parent);
				*tn = NULL;
			} else {
				if ((*tn != NULL) && (child->referenceCount > 0)) {
				ConsoleMessage (".... ARSF, requested child to remove is %p %s ref %d as a child",child,stringNodeType(child->_nodeType),
					child->referenceCount);
				}
				
			}
		}
	}
	update_node(parent);
}

/****************************************************************/
/* Add or Remove a series of children				*/
/*								*/
/* pass in a pointer to a node, (see Structs.h for defn)	*/
/* a pointer to the actual field in that node,			*/
/*	a list of node pointers, in memory,			*/
/*	the length of this list, (ptr size, not bytes)		*/
/*	and a flag for add (1), remove (2) or replace (0) 	*/
/*								*/
/****************************************************************/
unsigned long upper_power_of_two(unsigned long v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;

}

void AddRemoveChildren (
		struct X3D_Node *parent,
		struct Multi_Node *tn,
		struct X3D_Node * *nodelist,
		int len,
		int ar,
		char *file,
		int line) {
	int oldlen;
	void *newmal;
	struct X3D_Node * *remchild;
	struct X3D_Node * *remptr;
	struct X3D_Node * *tmpptr;
	int done;

	int counter, c2;
	#ifdef CRVERBOSE
	
	printf ("\n start of AddRemoveChildren; parent is a %s at %p\n",stringNodeType(parent->_nodeType),parent);
	printf ("AddRemove Children parent %p tn %p, len %d ar %d\n",parent,tn,len,ar);
	printf ("called at %s:%d\n",file,line);
	#endif

	/* if no elements, just return */
	if (len <=0) return;
	if ((parent==0) || (tn == 0)) {
		//printf ("Freewrl: AddRemoveChildren, parent and/or field NULL\n");
		return;
	}

	/* mark the parent changed, eg, rootNode() will not be sorted if this is not marked */
	parent->_change ++;


	oldlen = tn->n;
	#ifdef CRVERBOSE
	printf ("AddRemoveChildren, len %d, oldlen %d ar %d\n",len, oldlen, ar);
	#endif

	/* to do a "set_children", we remove the children, then do an add */
	if (ar == 0) {
		#ifdef CRVERBOSE
		printf ("we have to perform a \"set_children\" on this field\n");
		# endif

		/* make it so that we have 0 children */
		tn->n=0; 

		/* go through the children, and tell them that they are no longer wanted here */
		for (counter=0; counter < oldlen; counter ++) remove_parent(tn->p[counter],parent);

		/* now, totally free the old children array */
		if (oldlen > 0) {FREE_IF_NZ(tn->p);}

		/* now, make this into an addChildren */
		oldlen = 0;
		ar = 1;

	}


	if (ar == 1) {
		/* addChildren - now we know how many SFNodes are in this MFNode, lets MALLOC and add */
		unsigned long p2new, p2old;
		unsigned long old_len = (unsigned)(oldlen);
		unsigned long new_len = (unsigned)(oldlen+len);
		p2new = upper_power_of_two(new_len);
		p2old = upper_power_of_two(old_len);

		//if(upper_power_of_two(new_len) > upper_power_of_two(old_len))
		//if(1)
		if(p2new > p2old)
		{
			//realloc to next power-of-2 and copy over
			// the power-of-2 strategy means we 'anticipate' storage based on how much we've already used.
			// if we used 128 already, then we allocate another 128. If we've used 256 we allocate
			// another 256 - always doubling. That means wasted memory, but fewer reallocs, and
			// therefore less memory fragmentation than if we right-sized on each realloc.
			// (there was a dataset at http://r1.3crowd.com/blyon/opte/maps/raw/1069524880.3D.wrl 
			// that was very large and malloc failed not due to absolute out-of-memory, 
			// but rather due to fragmentation in AddRemoveChildren -reallocing for each 1 additional node-
			// causing malloc to return null after ~35000 of ~78000 children were added one at a time)
			unsigned long po2 = upper_power_of_two(new_len);
			/* first, set children to 0, in case render thread comes through here */
			tn->n = 0;
			#ifdef CRVERBOSE
			printf("[%d]{%u}",oldlen,upper_power_of_two(old_len));
			#endif
			//newmal = MALLOC (void *, (oldlen+len)*sizeof(struct X3D_Node *));
			newmal = MALLOC (void *, (po2)*sizeof(struct X3D_Node *));

			/* copy the old stuff over */
			if (oldlen > 0) memcpy (newmal,tn->p,oldlen*sizeof(void *));

			/* set up the C structures for this new MFNode addition */
			if(oldlen > 0) {
				FREE_IF_NZ (tn->p);
			}
			tn->n = oldlen;
			tn->p = newmal;
		}else{
			/*already alloced - just add to end*/
			newmal = tn->p;
			tn->n = oldlen;
		}

		/* copy the new stuff over - note, tmpptr changes what it points to */
		tmpptr  = offsetPointer_deref(struct X3D_Node * *,newmal, sizeof(struct X3D_Node *) * oldlen);

		/* tell each node in the nodelist that it has a new parent */
		for (counter = 0; counter < len; counter++) {
			#ifdef CRVERBOSE
			printf ("AddRemove, count %d of %d, node %p parent %p\n",counter, len,nodelist[counter],parent);
			#endif
			if (nodelist[counter] != NULL) {
				//add a new node to the children list
				*tmpptr = nodelist[counter];
				tmpptr ++;
				tn->n++;
				//add new parent to new node
				ADD_PARENT((void *)nodelist[counter],(void *)parent);
			} else {
				/* gosh, we are asking to add a NULL node pointer, lets just skip it... */
				printf ("AddRemoveChildren, Add, but new node is null; ignoring...\n");
			}
		}
		/*
		for (counter = 0; counter < tn->n; counter++) {
			printf ("AddRemoveChildren, checking, we have index %d node %p\n",counter,tn->p[counter]);
		}
		*/
	} else {
		int finalLength;
		int num_removed;

		/* this is a removeChildren */

		/* go through the original array, and "zero" out children that match one of
		   the parameters */

		num_removed = 0;
		remchild = nodelist;
		/* printf ("removing, len %d, tn->n %d\n",len,tn->n);  */
		for (c2 = 0; c2 < len; c2++) {
			remptr = (struct X3D_Node * *) tn->p;
			done = FALSE;

			for (counter = 0; counter < tn->n; counter ++) {
				#ifdef CRVERBOSE
				printf ("remove, comparing %p with %p\n",*remptr, *remchild); 
				#endif
				if ((*remptr == *remchild) && (!done)) {
					#ifdef CRVERBOSE
					printf ("Found it! removing this child from this parent\n");
					#endif

					remove_parent(X3D_NODE(*remchild),parent);
					*remptr = NULL;  /* "0" can not be a valid memory address */
					num_removed ++;
					done = TRUE; /* remove this child ONLY ONCE - in case it has been added
							more than once. */
				}
				remptr ++;
			}
			remchild ++;
		}


		finalLength = oldlen - num_removed;
		#ifdef CRVERBOSE
		printf ("final length is %d, we have %d in original array\n", finalLength, tn->n);
		remptr = (struct X3D_Node * *) tn->p;
		printf ("so, the original array, with zeroed elements is: \n");
		for (counter = 0; counter < tn->n; counter ++) {
			printf ("count %d of %d is %p\n",counter,tn->n, *remptr); 
			remptr ++;
		}
		#endif


		if (num_removed > 0) {
			if (finalLength > 0) {
				newmal = MALLOC (void *, finalLength*sizeof(struct X3D_Node * *));
				bzero (newmal, (size_t)(finalLength*sizeof(struct X3D_Node * *)));
				tmpptr = (struct X3D_Node * *) newmal;
				remptr = (struct X3D_Node * *) tn->p;

				/* go through and copy over anything that is not zero */
				for (counter = 0; counter < tn->n; counter ++) {
					/* printf ("count %d is %p\n",counter, *remptr); */
					if (*remptr != NULL) {
						*tmpptr = *remptr;
						/* printf ("now, tmpptr is %p\n",*tmpptr);  */
						tmpptr ++;
					}
					remptr ++;
				}
				/* printf ("done loops, now make data active \n"); */

				/* now, do the move of data */
				tn->n = 0;
				FREE_IF_NZ (tn->p);
				tn->p = newmal;
				tn->n = finalLength;
			} else {
				tn->n = 0;
				FREE_IF_NZ(tn->p);
			}

			#ifdef CRVERBOSE
			printf ("so, we have a final array length of %d\n",tn->n);
			for (counter =0; counter <tn->n; counter ++) {
				printf ("    element %d is %p\n",counter,tn->p[counter]);
			}
			#endif

		}

	}

	update_node(parent);
}


/* These events must be run first during the event loop, as they start an event cascade.
   Regsister them with add_first, then call them during the event loop with do_first.    */

void kill_clockEvents() { 
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	/* printf ("killing clckevents - was %d\n",num_ClockEvents); */
	p->num_ClockEvents = 0;
}

void add_first(struct X3D_Node * node) {
	void (*myp)(void *);
	int clocktype;
	int count;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	
	if (node == 0) {
		printf ("error in add_first; somehow the node datastructure is zero \n");
		return;
	}

	clocktype = node->_nodeType;
	/* printf ("add_first for %s\n",stringNodeType(clocktype)); */

	if (NODE_TimeSensor == clocktype) { myp =  do_TimeSensorTick;
	} else if (NODE_ProximitySensor == clocktype) { myp = do_ProximitySensorTick;
	} else if (NODE_Collision == clocktype) { myp = do_CollisionTick;
	} else if (NODE_MovieTexture == clocktype) { myp = do_MovieTextureTick;
	} else if (NODE_AudioClip == clocktype) { myp = do_AudioTick;
	} else if (NODE_VisibilitySensor == clocktype) { myp = do_VisibilitySensorTick;
	} else if (NODE_MovieTexture == clocktype) { myp = do_MovieTextureTick;
	} else if (NODE_GeoProximitySensor == clocktype) { myp = do_GeoProximitySensorTick;

	} else {
		/* printf ("this is not a type we need to add_first for %s\n",stringNodeType(clocktype)); */
		return;
	}

	if (p->num_ClockEvents + 1 > p->size_ClockEvents){
		//ATOMIC OPS - realloc in parsing thread (here) while display thread is in do_first() 
		//can cause do_first to bomb as it points to abandoned p->
		//we don't have mutexes here (yet)
		//so we break realloc into more atomic steps (and reduce frequency of reallocs with pre-allocated size_ )
		struct FirstStruct *old_ce, *ce;
		ce = MALLOC(struct FirstStruct *, sizeof (struct FirstStruct) * p->size_ClockEvents * 2);
		memcpy(ce, p->ClockEvents, sizeof (struct FirstStruct) * p->num_ClockEvents);
		p->size_ClockEvents *= 2; //power-of-two resizing means less memory fragmentation for large counts
		old_ce = p->ClockEvents;
		p->ClockEvents = ce;
		FREE_IF_NZ(old_ce);
	}
	//	p->ClockEvents = (struct FirstStruct *)REALLOC(p->ClockEvents, sizeof (struct FirstStruct) * (p->num_ClockEvents + 1));

	if (p->ClockEvents == 0) {
		printf ("can not allocate memory for add_first call\n");
		p->num_ClockEvents = 0;
	}

	/* does this event exist? */
	for (count=0; count < p->num_ClockEvents; count ++) {
		if (p->ClockEvents[count].tonode == node) {
			/* printf ("add_first, already have %d\n",node); */
			return;
		}	
	}

	/* is there a free slot to slide this into? see delete_first */
	for (count=0; count < p->num_ClockEvents; count ++) {
		if (p->ClockEvents[count].tonode == NULL) {
			/* printf ("add_first, already have %d\n",node); */
			p->ClockEvents[count].interpptr = myp;
			p->ClockEvents[count].tonode = node;
			return;
		}	
	}


	/* now, put the function pointer and data pointer into the structure entry */
	p->ClockEvents[p->num_ClockEvents].interpptr = myp;
	p->ClockEvents[p->num_ClockEvents].tonode = node;

	p->num_ClockEvents++;
}

/* go through, and delete this entry from the do_first list, if it exists */
void delete_first(struct X3D_Node *node) {
	int count;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	if (p->ClockEvents) {
		for (count=0; count < p->num_ClockEvents; count ++) {
			if (p->ClockEvents[count].tonode == node) {
				p->ClockEvents[count].tonode = NULL;
				return;
			}	
		}
	}
}






/********************************************************************

Register a route, but with fewer and more expressive parameters than
CRoutes_Register.  Currently a wrapper around that other function.

********************************************************************/

void CRoutes_RegisterSimple(
	struct X3D_Node* from, int fromOfs,
	struct X3D_Node* to, int toOfs,
	int type)  {
	 //printf ("CRoutes_RegisterSimple, registering a route of %s\n",stringFieldtypeType(type)); 

 	/* 10+1+3+1=15:  Number <5000000000, :, number <999, \0 */
 	void* interpolatorPointer;
 	int extraData = 0;
	int dir = 0;


	/* get direction flags here */
	switch (from->_nodeType) {
		case NODE_Script:
		case NODE_ComposedShader:
		case NODE_PackagedShader:
		//JAS case NODE_ShaderProgram: 
        case NODE_ProgramShader:
			dir  = dir | FROM_SCRIPT; break;
		default: {}
	}
	switch (to->_nodeType) {
		case NODE_Script:
		case NODE_ComposedShader:
		case NODE_PackagedShader:
		//JAS case NODE_ShaderProgram:
        case NODE_ProgramShader:
			dir  = dir | TO_SCRIPT; break;
		default: {}
	}

	/* check to ensure that we are not doing with a StaticGroup here */
	if (dir!=SCRIPT_TO_SCRIPT && dir!=TO_SCRIPT) {
		/* printf ("we are NOT sending to a script, checking for StaticGroup\n"); */
		if (to->_nodeType == NODE_StaticGroup) {
			ConsoleMessage ("ROUTE to a StaticGroup not allowed");
			return;
		}
	}
	/* check to ensure that we are not doing with a StaticGroup here */
	if (dir!=SCRIPT_TO_SCRIPT && dir!=FROM_SCRIPT) {
		/* printf ("we are NOT sending from a script, checking for StaticGroup\n"); */
		if (from->_nodeType == NODE_StaticGroup) {
			ConsoleMessage ("ROUTE from a StaticGroup not allowed");
			return;
		}
	}

	/* When routing to a script, to is not a node pointer! */
	if(dir!=SCRIPT_TO_SCRIPT && dir!=TO_SCRIPT)
		interpolatorPointer=returnInterpolatorPointer(stringNodeType(to->_nodeType));
	else
		interpolatorPointer=NULL;
	CRoutes_Register(1, from, fromOfs, to,toOfs, type, interpolatorPointer, dir, extraData);
}
 

/********************************************************************

Remove a route, but with fewer and more expressive parameters than
CRoutes_Register.  Currently a wrapper around that other function.

********************************************************************/

void CRoutes_RemoveSimple(
	struct X3D_Node* from, int fromOfs,
	struct X3D_Node* to, int toOfs,
	int type) {

 	/* 10+1+3+1=15:  Number <5000000000, :, number <999, \0 */
 	void* interpolatorPointer;
 	int extraData = 0;

  	interpolatorPointer=returnInterpolatorPointer(stringNodeType(to->_nodeType));

 	CRoutes_Register(0, from, fromOfs, to, toOfs, type, 
  		interpolatorPointer, 0, extraData);
}

/********************************************************************

CRoutes_Register.

Register a route in the routing table.

********************************************************************/

	
void CRoutes_Register(
		int adrem,
		struct X3D_Node *from,
		int fromoffset,
		struct X3D_Node *to,
		int toOfs,
		int type,
		void *intptr,
		int scrdir,
		int extra) {

	struct CR_RegStruct *newEntry;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

#ifdef HAVE_OPENCL
    cl_kernel CL_Interpolator = NULL;
        ConsoleMessage ("CRoutes_Register, being run on :%s:\n",__DATE__);

#endif //HAVE_OPENCL

    
/*
ConsoleMessage ("CRoutes_Register - adrem %d, from %p (%s) fromoffset %d to %p (%s) toOfs %d type %d intptr %p scrdir %d extra %d\n",
                        adrem, from,
                        stringNodeType(from->_nodeType),
                        fromoffset, to,
                        stringNodeType(to->_nodeType),
                        toOfs, type, intptr, scrdir, extra);
*/


	// do we have an Interpolator running on the GPU?
	if (from->_nodeType == NODE_CoordinateInterpolator) {

		int incr = adrem;
		struct X3D_CoordinateInterpolator *px = (struct X3D_CoordinateInterpolator *) from;
		if (incr == 0) incr = -1; // makes easy addition
        
		#ifdef HAVE_OPENCL
		if (to->_nodeType == NODE_Coordinate) {
	
			if (canRouteOnGPUTo(to) ) {
				ppOpenCL_Utils p;
				ttglobal tg = gglobal();
				p = (ppOpenCL_Utils)tg->OpenCL_Utils.prv;

				px ->_GPU_Routes_out += incr;

				if (tg->OpenCL_Utils.OpenCL_Initialized) { 
					printf ("OpenCL initialized in routes\n");
				} else {
					printf("OPENCL NOT INITIALIZED YET\n");
				}

				while (!tg->OpenCL_Utils.OpenCL_Initialized) {
					usleep (100000);
					printf ("sleeping, waiting for CL to be initialized\n");
				}


		
				CL_Interpolator = p->coordinateInterpolatorKernel;
			} else {
				printf ("CRoutes Register, have a CoordinateInterpolator to Coordinate, but dest node type not supported yet\n");
					px->_CPU_Routes_out+= incr;
			}
		} else {
            
			px->_CPU_Routes_out += incr;
		}
		#else
		px->_CPU_Routes_out += incr;
		#endif //HAVE_OPENCL
	}

/* Script to Script - we actually put a small node in, and route to/from this node so routing is a 2 step process */
	if (scrdir == SCRIPT_TO_SCRIPT) {
		struct X3D_Node *chptr;
		int set, changed;

		/* initialize stuff for compile checks */
		set = 0; changed = 0;

		chptr = returnSpecificTypeNode(type, &set, &changed);
		CRoutes_Register (adrem, from, fromoffset,chptr,set, type, 0, FROM_SCRIPT, extra);
		CRoutes_Register (adrem, chptr, changed, to, toOfs, type, 0, TO_SCRIPT, extra);
		return;
	}

	MUTEX_LOCK_ROUTING_UPDATES

	if (p->routesToRegister == NULL) {
		p->routesToRegister = newVector(struct CR_RegStruct *, 16);
	}


	newEntry = MALLOC(struct CR_RegStruct *, sizeof (struct CR_RegStruct));
	newEntry->adrem = adrem;
	newEntry->from = from;
	newEntry->fromoffset = fromoffset;
	newEntry->to = to;
	newEntry->toOfs = toOfs;
	newEntry->fieldType = type;
	newEntry->intptr = intptr;
	newEntry->scrdir = scrdir;
	newEntry->extra = extra;
	#ifdef HAVE_OPENCL
	newEntry->CL_Interpolator = CL_Interpolator;
	#endif

	vector_pushBack(struct CR_RegStruct *, p->routesToRegister, newEntry);

	MUTEX_FREE_LOCK_ROUTING_UPDATES

}

void print_routes_ready_to_register(FILE* fp)
{
	int numRoutes;
	int count;
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	int fromOffset;
	int toOffset;
	char *fromName;
	char *toName;
	struct CR_RegStruct *entry;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	if(p->routesToRegister == NULL) return;
	numRoutes = vectorSize(p->routesToRegister);
	fprintf(fp,"Number of Routes Ready to Register %d\n",numRoutes);
	if (numRoutes < 1) {
		return;
	}

	for (count = 0; count < (numRoutes); count++) {
		entry = vector_get(struct CR_RegStruct *, p->routesToRegister, count);
		fromNode = entry->from;
		fromOffset = entry->fromoffset;
		toNode = entry->to;
		toOffset = entry->toOfs;
		fromName = parser_getNameFromNode(fromNode);
		toName   = parser_getNameFromNode(toNode);
		fprintf (fp, " %p %s.%s TO %p %s.%s \n",fromNode,fromName,
			findFIELDNAMESfromNodeOffset0(fromNode,fromOffset),
			toNode,toName,
			findFIELDNAMESfromNodeOffset0(toNode,toOffset)
			);
	}
}


static void actually_do_CRoutes_Register() {
	int insert_here, shifter;
	CRnodeStruct *to_ptr = NULL;
	size_t toof;		/* used to help determine duplicate routes */
	struct X3D_Node *toN;
	indexT ind;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	if (p->routesToRegister == NULL) return; /* should never get here, but... */

#ifdef CRVERBOSE
	printf ("actually_do_CRoutes_Register, vector size %d\n",vectorSize(p->routesToRegister));
#endif

	for (ind=0; ind<vectorSize(p->routesToRegister); ind++ ) {
		struct CR_RegStruct *newEntry;

		newEntry = vector_get(struct CR_RegStruct *, p->routesToRegister, ind);

#ifdef CRVERBOSE  
		printf ("CRoutes_Register adrem %d from %u ",newEntry->adrem, newEntry->from);
		//if (newEntry->from > JSMaxScript) printf ("(%s) ",stringNodeType(X3D_NODE(newEntry->from->_nodeType)));

		printf ("off %u to %u intptr %p\n",
				newEntry->fromoffset, newEntry->to, newEntry->intptr);
		printf ("CRoutes_Register, CRoutes_Count is %d\n",p->CRoutes_Count);
#endif

		/* first time through, create minimum and maximum for insertion sorts */
		if (!p->CRoutes_Initiated) {
			/* allocate the CRoutes structure */
			p->CRoutes_MAX = 25; /* arbitrary number; max 25 routes to start off with */
			p->CRoutes = MALLOC (struct CRStruct *, sizeof (*p->CRoutes) * p->CRoutes_MAX);
	
			p->CRoutes[0].routeFromNode = X3D_NODE(0);
			p->CRoutes[0].fnptr = 0;
			p->CRoutes[0].tonode_count = 0;
			p->CRoutes[0].tonodes = NULL;
			p->CRoutes[0].isActive = FALSE;
			p->CRoutes[0].interpptr = 0;
			p->CRoutes[0].intTimeStamp = 0;
			p->CRoutes[1].routeFromNode = X3D_NODE(-1);
			p->CRoutes[1].fnptr = 0x8FFFFFFF;
			p->CRoutes[1].tonode_count = 0;
			p->CRoutes[1].tonodes = NULL;
			p->CRoutes[1].isActive = FALSE;
			p->CRoutes[1].interpptr = 0;
			p->CRoutes[1].intTimeStamp = 0;
			p->CRoutes_Count = 2;
			p->CRoutes_Initiated = TRUE;

			#ifdef HAVE_OPENCL
			p->CRoutes[0].CL_Interpolator = NULL;
			p->CRoutes[1].CL_Interpolator = NULL;
			#endif
		}
	
		insert_here = 1;
	
		/* go through the routing list, finding where to put it */
		while (newEntry->from > p->CRoutes[insert_here].routeFromNode) {
			#ifdef CRVERBOSE 
				printf ("comparing %u to %u\n",newEntry->from, p->CRoutes[insert_here].routeFromNode);
			#endif
			insert_here++;
		}
	
		/* hmmm - do we have a route from this node already? If so, go
		   through and put the offsets in order */
		while ((newEntry->from == p->CRoutes[insert_here].routeFromNode) &&
			(newEntry->fromoffset > p->CRoutes[insert_here].fnptr)) {
			#ifdef CRVERBOSE 
				printf ("same routeFromNode, different offset\n");
			#endif
			insert_here++;
		}
	
	
		/* Quick check to verify that we don't have a duplicate route here
		   OR to delete a route... */
	
		#ifdef CRVERBOSE
		printf ("ok, CRoutes_Register - is this a duplicate? comparing from (%d %d), fnptr (%d %d) intptr (%d %d) and tonodes %d\n",
			p->CRoutes[insert_here].routeFromNode, newEntry->from,
			p->CRoutes[insert_here].fnptr, newEntry->fromoffset,
			p->CRoutes[insert_here].interpptr, newEntry->intptr,
			p->CRoutes[insert_here].tonodes);
		#endif
	
		if ((p->CRoutes[insert_here].routeFromNode==newEntry->from) &&
			(p->CRoutes[insert_here].fnptr==newEntry->fromoffset) &&
			(p->CRoutes[insert_here].interpptr==newEntry->intptr) &&
			(p->CRoutes[insert_here].tonodes!=0)) {
	
			/* possible duplicate route */
			toN = newEntry->to; 
			toof = newEntry->toOfs;

			if ((toN == (p->CRoutes[insert_here].tonodes)->routeToNode) &&
				(toof == (p->CRoutes[insert_here].tonodes)->foffset)) {
				/* this IS a duplicate, now, what to do? */
	
				#ifdef CRVERBOSE
				printf ("duplicate route; maybe this is a remove? \n");
				#endif
	
				/* is this an add? */
				if (newEntry->adrem == 1) {
					#ifdef CRVERBOSE
						printf ("definite duplicate, returning\n");
					#endif
					continue; //return;
				} else {
					/* this is a remove */
	
					for (shifter = insert_here; shifter < p->CRoutes_Count; shifter++) {
					#ifdef CRVERBOSE 
						printf ("copying from %d to %d\n",shifter, shifter-1);
					#endif
						memcpy ((void *)&p->CRoutes[shifter],
							(void *)&p->CRoutes[shifter+1],
							sizeof (struct CRStruct));
					}
					p->CRoutes_Count --;
					#ifdef CRVERBOSE 
						printf ("routing table now %d\n",p->CRoutes_Count);
						for (shifter = 0; shifter < p->CRoutes_Count; shifter ++) {
							printf ("%d: %u %u %u\n",shifter, p->CRoutes[shifter].routeFromNode, p->CRoutes[shifter].fnptr,
								p->CRoutes[shifter].interpptr);
						}
					#endif
	
					/* return; */
				}
			}
		}
	
		/* this is an Add; removes should be handled above. */
		if (newEntry->adrem == 1)  {
			#ifdef CRVERBOSE 
				printf ("CRoutes, inserting at %d\n",insert_here);
			#endif
		
			/* create the space for this entry. */
			for (shifter = p->CRoutes_Count; shifter > insert_here; shifter--) {
				memcpy ((void *)&p->CRoutes[shifter], (void *)&p->CRoutes[shifter-1],sizeof(struct CRStruct));
				#ifdef CRVERBOSE 
					printf ("Copying from index %d to index %d\n",shifter, shifter-1);
				#endif
			}
		
		
			/* and put it in */
			p->CRoutes[insert_here].routeFromNode = newEntry->from;
			p->CRoutes[insert_here].fnptr = newEntry->fromoffset;
			p->CRoutes[insert_here].isActive = FALSE;
			p->CRoutes[insert_here].tonode_count = 0;
			p->CRoutes[insert_here].tonodes = NULL;
			p->CRoutes[insert_here].len = returnRoutingElementLength(newEntry->fieldType);
			p->CRoutes[insert_here].interpptr = (void (*)(void*))newEntry->intptr;
			p->CRoutes[insert_here].direction_flag = newEntry->scrdir;
			p->CRoutes[insert_here].extra = newEntry->extra;
			p->CRoutes[insert_here].intTimeStamp = 0;
			#ifdef HAVE_OPENCL
			p->CRoutes[insert_here].CL_Interpolator = newEntry->CL_Interpolator;
			#endif

		
			if ((p->CRoutes[insert_here].tonodes =
				 MALLOC(CRnodeStruct *, sizeof(CRnodeStruct))) == NULL) {
				fprintf(stderr, "CRoutes_Register: calloc failed to allocate memory.\n");
			} else {
				p->CRoutes[insert_here].tonode_count = 1;
				/* printf ("inserting route, to %u, offset %d\n",newEntry->to, newEntry->toOfs); */
		
				to_ptr = &(p->CRoutes[insert_here].tonodes[0]);
				to_ptr->routeToNode = newEntry->to;
				to_ptr->foffset = newEntry->toOfs;
			}
		
			/* record that we have one more route, with upper limit checking... */
			if (p->CRoutes_Count >= (p->CRoutes_MAX-2)) {
				/* printf("WARNING: expanding routing table\n");  */
				p->CRoutes_MAX += 50; /* arbitrary expansion number */
				p->CRoutes =(struct CRStruct *) REALLOC (p->CRoutes, sizeof (*p->CRoutes) * p->CRoutes_MAX);
			}
		
			p->CRoutes_Count ++;
		
	#ifdef CRVERBOSE 
				printf ("routing table now %d\n",p->CRoutes_Count);
				for (shifter = 0; shifter < p->CRoutes_Count; shifter ++) {
					printf ("%d: from: %p offset: %u Interpolator %p direction %d, len %d extra %d : ",shifter,
						p->CRoutes[shifter].routeFromNode, p->CRoutes[shifter].fnptr,
						p->CRoutes[shifter].interpptr, p->CRoutes[shifter].direction_flag, p->CRoutes[shifter].len, p->CRoutes[shifter].extra);
					for (insert_here = 0; insert_here < p->CRoutes[shifter].tonode_count; insert_here++) {
						printf (" to: %p %u",p->CRoutes[shifter].tonodes[insert_here].routeToNode,
									p->CRoutes[shifter].tonodes[insert_here].foffset);
					}
					printf ("\n");
				}
	#endif
		FREE_IF_NZ(newEntry);
		}
	}
	FREE_IF_NZ(p->routesToRegister);

}

#ifdef DEBUG_VALIDNODE
/* only if DEBUG_VALIDNODE is defined; helps us find memory/routing problems */
void mark_event_check (struct X3D_Node *from, int totalptr, char *fn, int line) {
	printf ("mark_event_check: at %s:%d\n",fn,line);
	if (X3D_NODE_CHECK(from)) {
		#ifdef CRVERBOSE
		printf ("mark_event_check, routing from a %s\n",stringNodeType(from->_nodeType));
		#endif
	} else {
		printf ("mark_event_check, not a real node %d\n",from);
	}
	mark_event(from,totalptr);
	printf ("mark_event_check: finished at %s:%d\n",fn,line); 
}
#endif
	
/********************************************************************

mark_event - something has generated an eventOut; record the node
data structure pointer, and the offset. Mark all relevant entries
in the routing table that this node/offset triggered an event.

********************************************************************/

void mark_event (struct X3D_Node *from, int totalptr) {
	int findit;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	if(from == 0) return;
	/*if(totalptr == 0) return; */

	X3D_NODE_CHECK(from);

	/* maybe this MARK_EVENT is coming in during initial node startup, before routing is registered? */
	if (!p->CRoutes_Initiated) {
		LOCK_PREROUTETABLE
		/* printf ("routes not registered yet; lets save this one for a bit...\n"); */
		if (p->initialEventBeforeRoutesCount >= p->preRouteTableSize) {
			p->preRouteTableSize += POSSIBLEINITIALROUTES;
			p->preEvents=REALLOC (p->preEvents,
				sizeof (struct initialRouteStruct) * p->preRouteTableSize);
		}
		p->preEvents[p->initialEventBeforeRoutesCount].from = from;
		p->preEvents[p->initialEventBeforeRoutesCount].totalptr = totalptr;
		p->initialEventBeforeRoutesCount++;
		UNLOCK_PREROUTETABLE

		return;  /* no routes registered yet */
	}

	findit = 1;

	#ifdef CRVERBOSE 
		printf ("\nmark_event, from %s (%u) fromoffset %u\n", stringNodeType(from->_nodeType),from, totalptr);
	#endif

	/* events in the routing table are sorted by routeFromNode. Find
	   out if we have at least one route from this node */
	while (from > p->CRoutes[findit].routeFromNode) {
		#ifdef CRVERBOSE
		printf ("mark_event, skipping past %x %x, index %d\n",from, p->CRoutes[findit].routeFromNode, findit);
		#endif
		findit ++;
	}

	/* while we have an eventOut from this NODE/OFFSET, mark it as
	   active. If no event from this NODE/OFFSET, ignore it */
	while ((from == p->CRoutes[findit].routeFromNode) &&
		(totalptr != p->CRoutes[findit].fnptr)) findit ++;

	/* did we find the exact entry? */
	#ifdef CRVERBOSE 
 		printf ("ep, (%#x %#x) (%#x %#x) at %d \n",
			from,p->CRoutes[findit].routeFromNode, totalptr,
			p->CRoutes[findit].fnptr,findit); 
	#endif

	/* if we did, signal it to the CEvents loop  - maybe more than one ROUTE,
	   eg, a time sensor goes to multiple interpolators */
	while ((from == p->CRoutes[findit].routeFromNode) &&
		(totalptr == p->CRoutes[findit].fnptr)) {
		#ifdef CRVERBOSE
			printf ("found event at %d\n",findit);
		#endif
		if (p->CRoutes[findit].intTimeStamp!=p->thisIntTimeStamp) {
			p->CRoutes[findit].isActive=TRUE;
			p->CRoutes[findit].intTimeStamp=p->thisIntTimeStamp;
		}

#ifdef CRVERBOSE
		else printf ("routing loop broken, findit %d\n",findit);
#endif

		findit ++;
	}
	#ifdef CRVERBOSE
		printf ("done mark_event\n");
	#endif
}

//experimental _B mark event for brotos, to stop cycling node.exposed to/from proto.exposed
void mark_event_B (struct X3D_Node *lastFrom, int lastptr, struct X3D_Node *from, int totalptr) {
	int findit;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	if(from == 0) return;
	/*if(totalptr == 0) return; */

	X3D_NODE_CHECK(from);

	/* maybe this MARK_EVENT is coming in during initial node startup, before routing is registered? */
	if (!p->CRoutes_Initiated) {
		LOCK_PREROUTETABLE
		/* printf ("routes not registered yet; lets save this one for a bit...\n"); */
		if (p->initialEventBeforeRoutesCount >= p->preRouteTableSize) {
			p->preRouteTableSize += POSSIBLEINITIALROUTES;
			p->preEvents=REALLOC (p->preEvents,
				sizeof (struct initialRouteStruct) * p->preRouteTableSize);
		}
		p->preEvents[p->initialEventBeforeRoutesCount].from = from;
		p->preEvents[p->initialEventBeforeRoutesCount].totalptr = totalptr;
		p->initialEventBeforeRoutesCount++;
		UNLOCK_PREROUTETABLE

		return;  /* no routes registered yet */
	}

	findit = 1;

	#ifdef CRVERBOSE 
		printf ("\nmark_event, from %s (%u) fromoffset %u\n", stringNodeType(from->_nodeType),from, totalptr);
	#endif

	/* events in the routing table are sorted by routeFromNode. Find
	   out if we have at least one route from this node */
	while (from > p->CRoutes[findit].routeFromNode) {
		#ifdef CRVERBOSE
		printf ("mark_event, skipping past %x %x, index %d\n",from, p->CRoutes[findit].routeFromNode, findit);
		#endif
		findit ++;
	}

	/* while we have an eventOut from this NODE/OFFSET, mark it as
	   active. If no event from this NODE/OFFSET, ignore it */
	while ((from == p->CRoutes[findit].routeFromNode) &&
		(totalptr != p->CRoutes[findit].fnptr)) findit ++;

	/* did we find the exact entry? */
	#ifdef CRVERBOSE 
 		printf ("ep, (%#x %#x) (%#x %#x) at %d \n",
			from,p->CRoutes[findit].routeFromNode, totalptr,
			p->CRoutes[findit].fnptr,findit); 
	#endif

	/* if we did, signal it to the CEvents loop  - maybe more than one ROUTE,
	   eg, a time sensor goes to multiple interpolators */
	while ((from == p->CRoutes[findit].routeFromNode) &&
		(totalptr == p->CRoutes[findit].fnptr)) {
		BOOL isCycle = 0;
		#ifdef CRVERBOSE
			printf ("found event at %d\n",findit);
		#endif
		isCycle = (p->CRoutes[findit].tonodes[0].routeToNode == lastFrom && 
					p->CRoutes[findit].tonodes[0].foffset == lastptr);
		if(!isCycle)
		if (p->CRoutes[findit].intTimeStamp!=p->thisIntTimeStamp) {
			p->CRoutes[findit].isActive=TRUE;
			p->CRoutes[findit].intTimeStamp=p->thisIntTimeStamp;
		}

#ifdef CRVERBOSE
		else printf ("routing loop broken, findit %d\n",findit);
#endif

		findit ++;
	}
	#ifdef CRVERBOSE
		printf ("done mark_event\n");
	#endif
}

struct CRscriptStruct *getScriptControl()
{
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	return p->ScriptControl;
}
void setScriptControl(struct CRscriptStruct *ScriptControl)
{
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	p->ScriptControl = ScriptControl;
}
struct CRscriptStruct *getScriptControlIndex(int actualscript)
{
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	return &p->ScriptControl[actualscript];
}
int isScriptControlOK(int actualscript)
{
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	return p->ScriptControl[actualscript].scriptOK;
}
int isScriptControlInitialized(int actualscript)
{
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	return p->ScriptControl[actualscript]._initialized;
}
void initializeAnyScripts()
{
/*
   we want to run initialize() from the calling thread. NOTE: if
   initialize creates VRML/X3D nodes, it will call the ProdCon methods
   to do this, and these methods will check to see if nodes, yada,
   yada, yada, until we run out of stack. So, we check to see if we
   are initializing; if so, don't worry about checking for new scripts
   any scripts to initialize here? we do it here, because we may just
   have created new scripts during  X3D/VRML parsing. Routing in the
   Display thread may have noted new scripts, but will ignore them
   until   we have told it that the scripts are initialized.  printf
   ("have scripts to initialize in fwl_RenderSceneUpdateScene old %d new
   %d\n",max_script_found, max_script_found_and_initialized);
*/

//#define INITIALIZE_ANY_SCRIPTS 
#ifdef HAVE_JAVASCRIPT
	ttglobal tg = (ttglobal)gglobal();
	if( tg->CRoutes.max_script_found != tg->CRoutes.max_script_found_and_initialized) 
	{ 
		struct CRscriptStruct *ScriptControl = getScriptControl(); 
		int i; //jsval retval; 
		for (i=tg->CRoutes.max_script_found_and_initialized+1; i <= tg->CRoutes.max_script_found; i++) 
		{ 
			/* printf ("initializing script %d in thread %u\n",i,pthread_self());  */ 
			JSCreateScriptContext(i); 
			JSInitializeScriptAndFields(i); 
			if (ScriptControl[i].scriptOK) 
				jsActualrunScript(i, "initialize()");
				//ACTUALRUNSCRIPT(i, "initialize()" ,&retval); 
			 /* printf ("initialized script %d\n",i);*/  
		} 
		tg->CRoutes.max_script_found_and_initialized = tg->CRoutes.max_script_found; 
	}
#endif /* HAVE_JAVASCRIPT */
}

/*******************************************************************

CRoutes_js_new;

Register a new script for future routing

********************************************************************/

void CRoutes_js_new (int num, int scriptType) {
	/* record whether this is a javascript, class invocation, ... */
	ttglobal tg = gglobal();
	ppCRoutes p = (ppCRoutes)tg->CRoutes.prv;
	p->ScriptControl[num].thisScriptType = scriptType;

	/* compare with a intptr_t, because we need to compare to -1 */
	if (num > tg->CRoutes.max_script_found) tg->CRoutes.max_script_found = num;
}



#ifdef HAVE_JAVASCRIPT
/********************************************************************

mark_script - indicate that this script has had an eventIn
zero_scripts - reset all script indicators

********************************************************************/
void mark_script (int num) {
	ttglobal tg = gglobal();

	#ifdef CRVERBOSE 
		printf ("mark_script - script %d has been invoked\n",num);
	#endif
	tg->CRoutes.scr_act[num]= TRUE;
}


/********************************************************************

gatherScriptEventOuts - at least one script has been triggered; get the
eventOuts for this script

********************************************************************/

static void gatherScriptEventOuts(void) {
	int route;
	size_t fptr;
	size_t tptr;
	size_t len;
 	struct X3D_Node* tn;
	//OLDCODE struct X3D_Node* fn;

	int fromalready=FALSE;	 /* we have already got the from value string */
	int touched_flag=FALSE;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;
	ppCRoutes p;
	ttglobal tg = gglobal();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	p = (ppCRoutes)tg->CRoutes.prv;

	/* NOTE - parts of things in here might need to be wrapped by BeginRequest ??? */

	/* go through all routes, looking for this script as an eventOut */

	/* do we have any routes yet? - we can gather events before any routes are made */
	if (!p->CRoutes_Initiated) return;

	/* go from beginning to end in the routing table */
	route=1;
	while (route < (p->CRoutes_Count-1)) {
		#ifdef CRVERBOSE
		printf ("gather, routing %d is %s\n",route,
			stringNodeType(X3D_NODE(p->CRoutes[route].routeFromNode)->_nodeType));
		#endif

	if (X3D_NODE(p->CRoutes[route].routeFromNode)->_nodeType == NODE_Script) {
		struct X3D_Script *mys = X3D_SCRIPT(p->CRoutes[route].routeFromNode);
		struct Shader_Script *sp = (struct Shader_Script *) mys->__scriptObj;
		int actualscript = sp->num;

		/* printf ("gatherEvents, found a script at element %d, it is script number %d and node %u\n",
			route, actualscript,mys);  */
		/* this script initialized yet? We make sure that on initialization that the Parse Thread
		   does the initialization, once it is finished parsing. */
		//if (!p->ScriptControl[actualscript]._initialized) {
		if(!isScriptControlInitialized(actualscript)){

			/* printf ("waiting for initializing script %d at %s:%d\n",actualscript, __FILE__,__LINE__); */
			return;
		}

		if (actualscript > tg->CRoutes.max_script_found_and_initialized) {
			/* printf ("gatherScriptEventOut, waiting for script %d to become initialized\n"); */
			return;
		}

		//if (!p->ScriptControl[actualscript].scriptOK) {
		if (!isScriptControlOK(actualscript)){

			/* printf ("gatherScriptEventOuts - script initialized but not OK\n"); */
			return;
		}
		
		/* is this the same from node/field as before? */
		if ((p->CRoutes[route].routeFromNode == p->CRoutes[route-1].routeFromNode) &&
			(p->CRoutes[route].fnptr == p->CRoutes[route-1].fnptr) &&
			(route > 1)) {
			fromalready=TRUE;
		} else {
			/* printf ("different from, have to get value\n"); */
			fromalready=FALSE;
		}

		fptr = p->CRoutes[route].fnptr;
		//OLDCODE fn = p->CRoutes[route].routeFromNode;
		len = p->CRoutes[route].len;

		#ifdef CRVERBOSE
			printf ("\ngatherSentEvents, script %d from %s type %d len %d\n",actualscript, JSparamnames[fptr].name,
				JSparamnames[fptr].type, len);
		#endif

		/* now, set the actual properties - switch as documented above */
		if (!fromalready) {
			#ifdef CRVERBOSE 
				printf ("Not found yet, getting touched flag fptr %d script %d \n",fptr,actualscript);
			#endif
			touched_flag = get_valueChanged_flag((int)fptr,actualscript);
		}

		if (touched_flag!= 0) {
			/* get some easy to use pointers */
			for (to_counter = 0; to_counter < p->CRoutes[route].tonode_count; to_counter++) {
				to_ptr = &(p->CRoutes[route].tonodes[to_counter]);
				tn = to_ptr->routeToNode;
				tptr = to_ptr->foffset;

				#ifdef CRVERBOSE 
					printf ("%s script %d VALUE CHANGED! copy value and update %p\n",JSparamnames[fptr].name,actualscript,tn);
				#endif

				/* eventOuts go to VRML data structures */
				js_setField_javascriptEventOut(tn,(unsigned int) tptr,JSparamnames[fptr].type, (int) len, p->CRoutes[route].extra,
					actualscript);
					//p->ScriptControl[actualscript].cx);

				/* tell this node now needs to redraw */
				markScriptResults(tn, (int) tptr, route, to_ptr->routeToNode);

				#ifdef CRVERBOSE 
					printf ("%s script %d has successfully updated  %u\n",JSparamnames[fptr].name,actualscript,tn);
				#endif

			}
		}

		/* unset the touched flag */
		resetScriptTouchedFlag ((int) actualscript, (int) fptr);

		/* 
#if defined(JS_THREADSAFE)
		JS_BeginRequest(p->ScriptControl[actualscript].cx);
#endif
		REMOVE_ROOT(p->ScriptControl[actualscript].cx,global_return_val); 
#if defined(JS_THREADSAFE)
		JS_EndRequest(p->ScriptControl[actualscript].cx);
#endif
		*/
	}
	route ++;
	}

	#ifdef CRVERBOSE 
		printf ("%f finished  gatherScriptEventOuts loop\n",TickTime());
	#endif
}

static BOOL gatherScriptEventOut_B(union anyVrml* any, struct Shader_Script *shader, 
			int JSparamNameIndex, int type, int extra, int len) {
	//dug9 this version stores the value back in the script field instead of toNode.
	//also, doesn't do 'new parents' in here if value is an SFNode or MFNode.
	//int route;
	//size_t fptr;
	//size_t tptr;
	//size_t len;
 //	struct X3D_Node* tn;
	//struct X3D_Node* fn;
	//struct anyVrml* any;

	//int fromalready=FALSE;	 /* we have already got the from value string */
	int touched_flag=FALSE;
	int actualscript;
	//unsigned int to_counter;
	//CRnodeStruct *to_ptr = NULL;
	ppCRoutes p;
	ttglobal tg = gglobal();
	
	#ifdef CRVERBOSE
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	#endif

	p = (ppCRoutes)tg->CRoutes.prv;

	/* NOTE - parts of things in here might need to be wrapped by BeginRequest ??? */

	/* go through all routes, looking for this script as an eventOut */

	/* do we have any routes yet? - we can gather events before any routes are made */
	//if (!p->CRoutes_Initiated) return;

	/* go from beginning to end in the routing table */
	//route=1;
	//while (route < (p->CRoutes_Count-1)) {
	//	#ifdef CRVERBOSE
	//	printf ("gather, routing %d is %s\n",route,
	//		stringNodeType(X3D_NODE(p->CRoutes[route].routeFromNode)->_nodeType));
	//	#endif

	//if (X3D_NODE(p->CRoutes[route].routeFromNode)->_nodeType == NODE_Script) {
		//struct X3D_Script *mys = X3D_SCRIPT(p->CRoutes[route].routeFromNode);
		//struct Shader_Script *sp = (struct Shader_Script *) mys->__scriptObj;
		actualscript = shader->num;

		/* printf ("gatherEvents, found a script at element %d, it is script number %d and node %u\n",
			route, actualscript,mys);  */
		/* this script initialized yet? We make sure that on initialization that the Parse Thread
		   does the initialization, once it is finished parsing. */
		//if (!p->ScriptControl[actualscript]._initialized) {
		if(!isScriptControlInitialized(actualscript)){
			/* printf ("waiting for initializing script %d at %s:%d\n",actualscript, __FILE__,__LINE__); */
			return FALSE;
		}

		if (actualscript > tg->CRoutes.max_script_found_and_initialized) {
			/* printf ("gatherScriptEventOut, waiting for script %d to become initialized\n"); */
			return FALSE;
		}

		//if (!p->ScriptControl[actualscript].scriptOK) {
		if(!isScriptControlOK(actualscript)){
			/* printf ("gatherScriptEventOuts - script initialized but not OK\n"); */
			return FALSE;
		}
		
		/* is this the same from node/field as before? */
		//if ((p->CRoutes[route].routeFromNode == p->CRoutes[route-1].routeFromNode) &&
		//	(p->CRoutes[route].fnptr == p->CRoutes[route-1].fnptr) &&
		//	(route > 1)) {
		//	fromalready=TRUE;
		//} else {
		//	/* printf ("different from, have to get value\n"); */
		//	fromalready=FALSE;
		//}

		//fptr = p->CRoutes[route].fnptr;
		//fn = p->CRoutes[route].routeFromNode;
		//len = p->CRoutes[route].len;

		#ifdef CRVERBOSE
			//printf ("\ngatherSentEvents, script %d from %s type %d len %d\n",actualscript, JSparamnames[fptr].name,
			//	JSparamnames[fptr].type, len);
		#endif

		/* now, set the actual properties - switch as documented above */
		//if (!fromalready) {
			#ifdef CRVERBOSE 
				//printf ("Not found yet, getting touched flag fptr %d script %d \n",fptr,actualscript);
			#endif
			touched_flag = get_valueChanged_flag((int)JSparamNameIndex,actualscript);
		//}

		if (touched_flag!= 0) {
			/* get some easy to use pointers */
			//for (to_counter = 0; to_counter < p->CRoutes[route].tonode_count; to_counter++) {
			//	to_ptr = &(p->CRoutes[route].tonodes[to_counter]);
			//	tn = to_ptr->routeToNode;
			//	tptr = to_ptr->foffset;

				#ifdef CRVERBOSE 
					//printf ("%s script %d VALUE CHANGED! copy value and update %p\n",JSparamnames[fptr].name,actualscript,tn);
				#endif

				/* eventOuts go to VRML data structures */
				
				js_setField_javascriptEventOut_B(any,type, len, extra, 
					actualscript);
					//p->ScriptControl[actualscript].cx);
			//	void setField_javascriptEventOut_B(union anyVrml* any,  
			//int fieldType, unsigned len, int extraData, JSContext *scriptContext

				/* tell this node now needs to redraw */
				//markScriptResults(tn, (int) tptr, route, to_ptr->routeToNode);
				//MARK_EVENT (tn,tptr);

				#ifdef CRVERBOSE 
					//printf ("%s script %d has successfully updated  %u\n",JSparamnames[fptr].name,actualscript,tn);
				#endif

			//}
			/* unset the touched flag */
			resetScriptTouchedFlag ((int) actualscript, (int) JSparamNameIndex);
			return TRUE;

		}

		///* unset the touched flag */
		//resetScriptTouchedFlag ((int) actualscript, (int) fromOffset);

		/* 
#if defined(JS_THREADSAFE)
		JS_BeginRequest(p->ScriptControl[actualscript].cx);
#endif
		REMOVE_ROOT(p->ScriptControl[actualscript].cx,global_return_val); 
#if defined(JS_THREADSAFE)
		JS_EndRequest(p->ScriptControl[actualscript].cx);
#endif
		*/
	//}
	//route ++;
	//}

	#ifdef CRVERBOSE 
		printf ("%f finished  gatherScriptEventOuts loop\n",TickTime());
	#endif
	return FALSE;
}

void JSparamnamesShutdown(){
	ttglobal tg = gglobal();
	ppCRoutes p = (ppCRoutes)tg->CRoutes.prv;
	/* Script name/type table */
	FREE_IF_NZ(p->JSparamnames);
	tg->CRoutes.jsnameindex = -1;
	tg->CRoutes.MAXJSparamNames = 0;
}

void kill_javascript(void) {
	int i;
	ttglobal tg = gglobal();
	ppCRoutes p = (ppCRoutes)tg->CRoutes.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* printf ("calling kill_javascript()\n"); */
	zeroScriptHandles();
	if (jsIsRunning() != 0) {
		for (i=0; i<=tg->CRoutes.max_script_found_and_initialized; i++) {
			/* printf ("kill_javascript, looking at %d\n",i); */
			if (ScriptControl[i].cx != 0) {
				JSDeleteScriptContext(i);
			}
		}
	}
	p->JSMaxScript = 0;
	tg->CRoutes.max_script_found = -1;
	tg->CRoutes.max_script_found_and_initialized = -1;
	jsShutdown();
	JSparamnamesShutdown();
	FREE_IF_NZ (ScriptControl);
	setScriptControl(NULL);
	FREE_IF_NZ(tg->CRoutes.scr_act);


}

void cleanupDie(int num, const char *msg) {
	kill_javascript();
	freewrlDie(msg);
}

void JSMaxAlloc() {
	/* perform some REALLOCs on JavaScript database stuff for interfacing */
	int count;
	ttglobal tg = gglobal();
	ppCRoutes p = (ppCRoutes)tg->CRoutes.prv;
	/* printf ("start of JSMaxAlloc, JSMaxScript %d\n",JSMaxScript); */
	struct CRscriptStruct *ScriptControl = getScriptControl();

	p->JSMaxScript += 10;
	setScriptControl( (struct CRscriptStruct*)REALLOC (ScriptControl, sizeof (*ScriptControl) * p->JSMaxScript));
	ScriptControl = getScriptControl();
	tg->CRoutes.scr_act = (int *)REALLOC (tg->CRoutes.scr_act, sizeof (*tg->CRoutes.scr_act) * p->JSMaxScript);

	/* mark these scripts inactive */
	for (count=p->JSMaxScript-10; count<p->JSMaxScript; count++) {
		tg->CRoutes.scr_act[count]= FALSE;
		ScriptControl[count].thisScriptType = NOSCRIPT;
		ScriptControl[count].eventsProcessed = NULL;
		ScriptControl[count].cx = 0;
		ScriptControl[count].glob = 0;
		ScriptControl[count]._initialized = FALSE;
		ScriptControl[count].scriptOK = FALSE;
		ScriptControl[count].scriptText = NULL;
		ScriptControl[count].paramList = NULL;
	}
}

/* set up table entry for this new script */
void JSInit(int num) {
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	#ifdef JAVASCRIPTVERBOSE 
	printf("JSinit: script %d\n",num);
	#endif

	/* more scripts than we can handle right now? */
	if (num >= p->JSMaxScript)  {
		JSMaxAlloc();
	}
}
int jsActualrunScript(int num, char *script);
void JSInitializeScriptAndFields (int num) {
        struct ScriptParamList *thisEntry;
        struct ScriptParamList *nextEntry;
	//jsval rval;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* printf ("JSInitializeScriptAndFields script %d, thread %u\n",num,pthread_self());   */
	/* run through paramList, and run the script */
	/* printf ("JSInitializeScriptAndFields, running through params and main script\n");  */
	if (num >= p->JSMaxScript)  {
		ConsoleMessage ("JSInitializeScriptAndFields: warning, script %d initialization out of order",num);
		return;
	}
	/* run through fields in order of entry in the X3D file */
        thisEntry = ScriptControl[num].paramList;
        while (thisEntry != NULL) {
		/* printf ("script field is %s\n",thisEntry->field);  */
		InitScriptField(num, thisEntry->kind, thisEntry->type, thisEntry->field, thisEntry->value);

		/* get the next block; free the current name, current block, and make current = next */
		nextEntry = thisEntry->next;
		FREE_IF_NZ (thisEntry->field);
		FREE_IF_NZ (thisEntry);
		thisEntry = nextEntry;
	}
	
	/* we have freed each element, set list to NULL in case anyone else comes along */
	ScriptControl[num].paramList = NULL;

	if (!jsActualrunScript(num, ScriptControl[num].scriptText)) {
		ConsoleMessage ("JSInitializeScriptAndFields, script failure\n");
		ScriptControl[num].scriptOK = FALSE;
		ScriptControl[num]._initialized = TRUE;
		return;
	}
	FREE_IF_NZ(ScriptControl[num].scriptText);
	ScriptControl[num]._initialized = TRUE;
	ScriptControl[num].scriptOK = TRUE;

}

/* A new version of InitScriptField which takes "nicer" arguments; currently a
 * simple and restricted wrapper, but it could replace it soon? */
/* Parameters:
	num:		Script number. Starts at 0. 
	kind:		One of PKW_initializeOnly PKW_outputOnly PKW_inputOutput PKW_inputOnly
	type:		One of the FIELDTYPE_ defines, eg, FIELDTYPE_MFFloat
	field:		the field name as found in the VRML/X3D file. eg "set_myField"
		
*/

/* save this field from the parser; initialize it when the fwl_RenderSceneUpdateScene wants to initialize it */
void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value) {
	struct ScriptParamList **nextInsert;
	struct ScriptParamList *newEntry;
	struct CRscriptStruct *ScriptControl = getScriptControl();
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	if (num >= p->JSMaxScript)  {
		ConsoleMessage ("JSSaveScriptText: warning, script %d initialization out of order",num);
		return;
	}

	/* generate a new ScriptParamList entry */
	/* note that this is a linked list, and we put things on at the end. The END MUST
	   have NULL termination */
	nextInsert = &(ScriptControl[num].paramList);
	while (*nextInsert != NULL) {
		nextInsert = &(*nextInsert)->next;
	}

	/* create a new entry and link it in */
	newEntry = MALLOC (struct ScriptParamList *, sizeof (struct ScriptParamList));
	*nextInsert = newEntry;
	
	/* initialize the new entry */
	newEntry->next = NULL;
	newEntry->kind = kind;
	newEntry->type = type;
	newEntry->field = STRDUP(field);
	newEntry->value = value;
}
#endif /* HAVE_JAVASCRIPT */
/* Save the text, so that when the script is initialized in the fwl_RenderSceneUpdateScene thread, it will be there */
void SaveScriptText(int num, const char *text) {
	ttglobal tg = gglobal();
	ppCRoutes p = (ppCRoutes)tg->CRoutes.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* printf ("SaveScriptText, num %d, thread %u saving :%s:\n",num, pthread_self(),text); */
	if (num >= p->JSMaxScript)  {
		ConsoleMessage ("SaveScriptText: warning, script %d initialization out of order",num);
		return;
	}
	FREE_IF_NZ(ScriptControl[num].scriptText);
	ScriptControl[num].scriptText = STRDUP(text);
/* NOTE - seems possible that a script could be overwritten; if so then fix eventsProcessed */
	//jsClearScriptControlEntries(&ScriptControl[num]);
	jsClearScriptControlEntries(num);

	if (((int)num) > tg->CRoutes.max_script_found) tg->CRoutes.max_script_found = num;
	/* printf ("SaveScriptText, for script %d scriptText %s\n",text);
	printf ("SaveScriptText, max_script_found now %d\n",max_script_found); */
}


struct CRjsnameStruct *getJSparamnames()
{
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	return p->JSparamnames;
}
void setJSparamnames(struct CRjsnameStruct *JSparamnames)
{
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
	p->JSparamnames = JSparamnames;
}

/********************************************************************

JSparamIndex.

stores ascii names with types (see code for type equivalences).

********************************************************************/

int JSparamIndex (const char *name, const char *type) {
	size_t len;
	int ty;
	int ctr;
	ttglobal tg = gglobal();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	#ifdef CRVERBOSE
	printf ("start of JSparamIndex, name %s, type %s\n",name,type);
	printf ("start of JSparamIndex, lengths name %d, type %d\n",
			strlen(name),strlen(type)); 
	#endif

	ty = findFieldInFIELDTYPES(type);

	#ifdef CRVERBOSE
	printf ("JSparamIndex, type %d, %s\n",ty,type); 
	#endif

	len = strlen(name);

	/* is this a duplicate name and type? types have to be same,
	   name lengths have to be the same, and the strings have to be the same.
	*/
	for (ctr=0; ctr<=tg->CRoutes.jsnameindex; ctr++) {
		if (ty==JSparamnames[ctr].type) {
			if ((strlen(JSparamnames[ctr].name) == len) &&
				(strncmp(name,JSparamnames[ctr].name,len)==0)) {
				#ifdef CRVERBOSE
				printf ("JSparamIndex, duplicate, returning %d\n",ctr);
				#endif

				return ctr;
			}
		}
	}

	/* nope, not duplicate */

	tg->CRoutes.jsnameindex ++;

	/* ok, we got a name and a type */
	if (tg->CRoutes.jsnameindex >= tg->CRoutes.MAXJSparamNames) {
		/* oooh! not enough room at the table */
		tg->CRoutes.MAXJSparamNames += 100; /* arbitrary number */
		setJSparamnames( (struct CRjsnameStruct*)REALLOC (JSparamnames, sizeof(*JSparamnames) * tg->CRoutes.MAXJSparamNames));
		JSparamnames = getJSparamnames();
	}

	if (len > MAXJSVARIABLELENGTH-2) len = MAXJSVARIABLELENGTH-2;	/* concatenate names to this length */
	strncpy (JSparamnames[tg->CRoutes.jsnameindex].name,name,len);
	JSparamnames[tg->CRoutes.jsnameindex].name[len] = 0; /* make sure terminated */
	JSparamnames[tg->CRoutes.jsnameindex].type = ty;
	JSparamnames[tg->CRoutes.jsnameindex].eventInFunction = NULL;
	#ifdef CRVERBOSE
	printf ("JSparamIndex, returning %d\n",tg->JScript.jsnameindex); 
	#endif

	return tg->CRoutes.jsnameindex;
}





/* we have a Script/Shader at routing table element %d, send events to it */
static void sendScriptEventIn(int num) {
	int to_counter;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;
    
	CRnodeStruct *to_ptr = NULL;


	#ifdef CRVERBOSE
	  printf("----BEGIN-------\nsendScriptEventIn, num %d direction %d\n",num,
		p->CRoutes[num].direction_flag);
	#endif


	/* script value: 1: this is a from script route
			 2: this is a to script route
			 (3 = SCRIPT_TO_SCRIPT - this gets changed in to a FROM and a TO;
			 check for SCRIPT_TO_SCRIPT in this file */

	if (p->CRoutes[num].direction_flag == TO_SCRIPT) {
		for (to_counter = 0; to_counter < p->CRoutes[num].tonode_count; to_counter++) {
			
            to_ptr = &(p->CRoutes[num].tonodes[to_counter]);
            
			if (to_ptr->routeToNode->_nodeType == NODE_Script) {
                #ifdef HAVE_JAVASCRIPT
                struct Shader_Script *myObj;
                
				/* this script initialized yet? We make sure that on initialization that the Parse Thread
				   does the initialization, once it is finished parsing. */

				/* get the value from the VRML structure, in order to propagate it to a script */
				myObj = X3D_SCRIPT(to_ptr->routeToNode)->__scriptObj;

				#ifdef CRVERBOSE
				printf ("myScriptNumber is %d\n",myObj->num);
				#endif


				/* is the script ok and initialized? */
				//if ((!p->ScriptControl[myObj->num]._initialized) || (!p->ScriptControl[myObj->num].scriptOK)) {
				if((!isScriptControlInitialized(myObj->num)) ||(!isScriptControlOK(myObj->num))){
					/* printf ("waiting for initializing script %d at %s:%d\n",(uintptr_t)to_ptr->routeToNode, __FILE__,__LINE__); */
					return;
				}

				/* mark that this script has been active SCRIPTS ARE INTEGER NUMBERS */
				mark_script(myObj->num);
				getField_ToJavascript(num,to_ptr->foffset);
                #endif /* HAVE_JAVASCRIPT */
			} else {
				getField_ToShader(to_ptr->routeToNode, num);
			}
			

		}
	} else {
		#ifdef CRVERBOSE 
			printf ("not a TO_SCRIPT value, ignoring this entry\n");
		#endif
	}
	#ifdef CRVERBOSE 
		printf("-----END-----\n");
	#endif

}

/********************************************************************

propagate_events.

Go through the event table, until the table is "active free". Some
nodes have eventins/eventouts - have to do the table multiple times
in this case.

********************************************************************/

//#ifdef CRVERBOSE
char * BOOL_STRING(int inp) {if (inp)return "true "; else return "false ";}
//#endif
void propagate_events_A() {
	int havinterp;
	int counter;
	int to_counter;
	CRnodeStruct *to_ptr = NULL;
	ppCRoutes p;
	ttglobal tg = gglobal();
	p = (ppCRoutes)tg->CRoutes.prv;

		#ifdef CRVERBOSE
		printf ("\npropagate_events start\n");
		#endif

	/* increment the "timestamp" for this entry */
	p->thisIntTimeStamp ++; 

	do {
		havinterp=FALSE; /* assume no interpolators triggered */

		for (counter = 1; counter < p->CRoutes_Count-1; counter++) {
			for (to_counter = 0; to_counter < p->CRoutes[counter].tonode_count; to_counter++) {
				to_ptr = &(p->CRoutes[counter].tonodes[to_counter]);
				if (to_ptr == NULL) {
					printf("WARNING: tonode at %u is NULL in propagate_events.\n",
							to_counter);
					continue;
				}

				#ifdef CRVERBOSE
					printf("propagate_events: counter %d to_counter %u act %s from %u off %u to %u off %u oint %u dir %d\n",
						   counter, to_counter, BOOL_STRING(p->CRoutes[counter].isActive),
						   p->CRoutes[counter].routeFromNode, p->CRoutes[counter].fnptr,
						   to_ptr->routeToNode, to_ptr->foffset, p->CRoutes[counter].interpptr,
							p->CRoutes[counter].direction_flag);
				#endif

				if (p->CRoutes[counter].isActive == TRUE) {
					/* first thing, set this to FALSE */
					p->CRoutes[counter].isActive = FALSE;
						#ifdef CRVERBOSE
						printf("event %p %u len %d sent something", p->CRoutes[counter].routeFromNode, p->CRoutes[counter].fnptr,p->CRoutes[counter].len);
						if (p->CRoutes[counter].fnptr < 20)
						{
							struct CRjsnameStruct *JSparamnames = getJSparamnames();
							printf (" (script param: %s)",JSparamnames[p->CRoutes[counter].fnptr].name);
						}else {
							printf (" (nodeType %s)",stringNodeType(X3D_NODE(p->CRoutes[counter].routeFromNode)->_nodeType));
						}
						printf ("\n");
						#endif
					/* to get routing to/from exposedFields, lets
					 * mark this to/offset as an event */

					#ifdef HAVE_OPENCL
					ConsoleMessage (" - JAS - bringing this event back into the fray\n");
					ConsoleMessage (" as leaving it out gives us routing problems for, eg, MFRotation.wrl\n");
					ConsoleMessage (" but leaving it in is a problem for CL routing\n");
					#endif //HAVE_OPENCL

					MARK_EVENT (to_ptr->routeToNode, to_ptr->foffset);
					//printf(",");
					if (p->CRoutes[counter].direction_flag != 0) {
						/* scripts are a bit complex, so break this out */
						sendScriptEventIn(counter);
						havinterp = TRUE;
					} else {
						/* copy the value over */

						#ifdef HAVE_OPENCL
/*
                         printf ("CRoutes, wondering if the clInterpolator is here...%p toNode %s interp %p\n",
                                p->CRoutes[counter].CL_Interpolator, stringNodeType(to_ptr->routeToNode->_nodeType),
                                p->CRoutes[counter].interpptr);
 */

						if (p->CRoutes[counter].CL_Interpolator != NULL) {
							void runOpenCLInterpolator(struct CRStruct *route, struct X3D_Node * toNode, int toOffset);

							runOpenCLInterpolator(&p->CRoutes[counter], to_ptr->routeToNode, to_ptr->foffset);
						} else
						#endif // HAVE_OPENCL

						if (p->CRoutes[counter].len > 0) {
						/* simple, fixed length copy */
							memcpy( offsetPointer_deref(void *,to_ptr->routeToNode ,to_ptr->foffset),
								offsetPointer_deref(void *,p->CRoutes[counter].routeFromNode , p->CRoutes[counter].fnptr),
								(unsigned)p->CRoutes[counter].len);
						} else {
							/* this is a Multi*node, do a specialized copy. eg, Tiny3D EAI test will
							   trigger this */
							#ifdef CRVERBOSE
							printf ("in croutes, mmc len is %d\n",p->CRoutes[counter].len);
							#endif
							Multimemcpy (
								X3D_NODE(to_ptr->routeToNode),
								X3D_NODE(p->CRoutes[counter].routeFromNode),
								offsetPointer_deref(void *, to_ptr->routeToNode, to_ptr->foffset),
								offsetPointer_deref(void *, p->CRoutes[counter].routeFromNode, 
									p->CRoutes[counter].fnptr), p->CRoutes[counter].len);
						}

						/* is this an interpolator? if so call the code to do it */
						if (p->CRoutes[counter].interpptr != 0) {
							/* this is an interpolator, call it */
							havinterp = TRUE;
								#ifdef CRVERBOSE
								printf("propagate_events: index %d is an interpolator\n",
									   counter);
								#endif

							/* copy over this "extra" data, EAI "advise" calls need this */
							tg->CRoutes.CRoutesExtra = p->CRoutes[counter].extra;
							p->CRoutes[counter].interpptr((void *)(to_ptr->routeToNode));
						} else {
							bool doItOnTheCPU = FALSE;

							if (p->CRoutes[counter].routeFromNode->_nodeType == NODE_CoordinateInterpolator) {
								if (X3D_COORDINATEINTERPOLATOR(p->CRoutes[counter].routeFromNode)->_CPU_Routes_out != 0) {
									doItOnTheCPU = TRUE;
								}
							}else {

								doItOnTheCPU = TRUE;
							}


							if (doItOnTheCPU) {
								#ifdef CRVERBOSE
								printf ("doing this route on the CPU (from a %s)\n",stringNodeType(p->CRoutes[counter].routeFromNode->_nodeType));
								#endif


								/* just an eventIn node. signal to the reciever to update */
								MARK_EVENT(to_ptr->routeToNode, to_ptr->foffset);

								/* make sure that this is pointing to a real node,
								 * not to a block of memory created by
								 * EAI - extra memory - if it has an offset of
								 * zero, it is most certainly made. */
								if ((to_ptr->foffset) != 0) {
									update_node(to_ptr->routeToNode);
								}
							} else {
								#ifdef CRVERBOSE
                        				       printf ("yep! doing this on the GPU!\n");
                                				#endif
                            				}

						}
					}
				}
			}
		}

		#ifdef HAVE_JAVASCRIPT
		/* run gatherScriptEventOuts for each active script */
		gatherScriptEventOuts();
		#endif

	} while (havinterp==TRUE);

	#ifdef HAVE_JAVASCRIPT
	/* now, go through and clean up all of the scripts */
	for (counter =0; counter <= tg->CRoutes.max_script_found_and_initialized; counter++) {
		if (tg->CRoutes.scr_act[counter]) {
			tg->CRoutes.scr_act[counter] = FALSE;
			js_cleanup_script_context(counter);
			//CLEANUP_JAVASCRIPT(p->ScriptControl[counter].cx);
		}
	}	
	#endif /* HAVE_JAVASCRIPT */
	//printf(" & ");
	#ifdef CRVERBOSE
	printf ("done propagate_events\n\n");
	#endif
}


/*
	new strategy, to reduce combinations and permuations of to/from types
	from n x n to n + n (where n=3 (builtin, script, proto): 3x3=9 -> 3+3=6)
	after the introduction of PROTO instances with interfaces different
	than script and builtin. 
	Steps:
	A. get anyVrml* of the from end - using switch/case/if/else of 3 items
	B. get anyVrml* of the to end - ditto of 3 items
	C. memcpy/shallow_copy of anyVrml for everyone
	D. touchup special target nodes like scripts and sensors
*/
union anyVrml* get_anyVrml(struct X3D_Node* node, int offset, int *type, int *mode)
{
	union anyVrml* fromAny;
	struct X3D_Node* fromNode;
	int fromMode, fromType, fromOffset;

	fromType = INT_ID_UNDEFINED;
	fromMode = INT_ID_UNDEFINED;
	fromOffset = offset;
	fromNode = node;

	switch(node->_nodeType)
	{
		case NODE_ShaderProgram:
		case NODE_ComposedShader:
		case NODE_PackagedShader:
		case NODE_Script:
			{
				struct Shader_Script* shader = NULL;
				struct ScriptFieldDecl* sfield;
				switch(fromNode->_nodeType) 
				{ 
					case NODE_Script:         shader =(struct Shader_Script *)(X3D_SCRIPT(fromNode)->__scriptObj); break;
					case NODE_ComposedShader: shader =(struct Shader_Script *)(X3D_COMPOSEDSHADER(fromNode)->_shaderUserDefinedFields); break;
					case NODE_ShaderProgram:  shader =(struct Shader_Script *)(X3D_SHADERPROGRAM(fromNode)->_shaderUserDefinedFields); break;
					case NODE_PackagedShader: shader =(struct Shader_Script *)(X3D_PACKAGEDSHADER(fromNode)->_shaderUserDefinedFields); break;
				}
				sfield= vector_get(struct ScriptFieldDecl*, shader->fields, fromOffset);
				fromAny = &sfield->value;
				fromType = sfield->fieldDecl->fieldType;
				fromMode = sfield->fieldDecl->PKWmode;

			}
			break;
		case NODE_Proto:
			{
				struct ProtoFieldDecl* pfield;
				struct X3D_Proto* pnode = (struct X3D_Proto*)fromNode;
				struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
				pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, fromOffset);
				fromAny = &pfield->defaultVal;
				fromType = pfield->type;
				fromMode = pfield->mode;
			}
			break;
		default: //builtin
			{
				const int * offsets;

				fromAny = (union anyVrml*)offsetPointer_deref(void *,fromNode , fromOffset);
				//I wish we had stored fromType when registering the route
				offsets = NODE_OFFSETS[fromNode->_nodeType];
				while(*offsets > -1)
				{
					//printf("%d %d %d %d %d\n",offsets[0],offsets[1],offsets[2],offsets[3],offsets[4]);
					if(offsets[1]==fromOffset) 
					{
						fromType = offsets[2];
						fromMode = PKW_from_KW(offsets[3]);
						break;
					}
					offsets += 5;
				}
			}
			break;
	}
	*mode = fromMode;
	*type = fromType;
	return fromAny;
}

void cleanFieldIfManaged(int type,int mode,int isPublic, struct X3D_Node* parent, int offset)
{
	//there should be a shallow_clean_field(type,toAny) that releases old mallocs 
	//  in UniString,MF p*, unlinks and/or killNodes
	//cleanFieldIfManaged()
	//  1. is toField a valueHolding field (inputOutput,initializeOnly)?
	//  2. if yes, is toField a node field (SFNode, MFNode)?
	//  3. if yes, is there something in toField now?
	//  4. if yes, get it, remove toNode as parent, refcount-- (let killNode in startofloopnodeupdates garbage collect it)
	//  5. if it was an MFNode, release the p* array
	//int isManagedField; //managed in the unlink_node sense, see unlink_node() killNode() policy
	//isManagedField = isPublic && (type == FIELDTYPE_SFNode || type == FIELDTYPE_MFNode);
	//isManagedField = isManagedField && (mode == PKW_initializeOnly || mode == PKW_inputOutput);
	//if(isManagedField)
	if(isManagedField(mode,type,isPublic))
	{
		int n,k,haveSomething,fromType,fromMode;
		struct X3D_Node **plist, *sfn;
		union anyVrml* any;
		any = get_anyVrml(parent,offset,&fromType,&fromMode);
		haveSomething = (type==FIELDTYPE_SFNode && any->sfnode) || (type==FIELDTYPE_MFNode && any->mfnode.n);
		haveSomething = haveSomething && parent;
		if(haveSomething){
			if(type==FIELDTYPE_SFNode){
				plist = &any->sfnode;
				n = 1;
			}else{
				plist = any->mfnode.p;
				n = any->mfnode.n;
			}
			for(k=0;k<n;k++)
			{
				sfn = plist[k];
				remove_parent(sfn,parent);
				//remove parent should return a bool if found, so we know if we can/should decrement referenceCount
				sfn->referenceCount--;
			}
			if(type==FIELDTYPE_MFNode) {
				FREE_IF_NZ(plist);
			}
		}
	}
}


void add_mfparents(struct X3D_Node* newParent, union anyVrml* mfnode, int mftype)
{
	int i;
	if(mftype != FIELDTYPE_MFNode) return;
	for(i=0;i<mfnode->mfnode.n;i++)
	{
		add_parent(mfnode->mfnode.p[i],newParent,__FILE__,__LINE__);
	}
					//case FIELDTYPE_SFNode:
					//	{
					//		if(source->sfnode){ 
					//			memcpy(dest,source,isize);
					//			add_parent(dest->sfnode,parent,__FILE__,__LINE__);
					//		}else{
					//			dest->sfnode = NULL;
					//		}
					//	}
					//	break;
}

//char *findFIELDNAMESfromNodeOffset0(struct X3D_Node *node, int offset);
char *findFIELDNAMES0(struct X3D_Node *node, int offset);


const char *stringMode(int pkwmode, int cute){
	const char **strmode;
	const char *cutemode[] = {"init","in","out","inOut" };
	const char *fullmode[] = {"initializeOnly","inputOnly","outputOnly","inputOutput"};
	strmode = fullmode;
	if(cute) strmode = cutemode;

	switch(pkwmode)
	{
		case PKW_initializeOnly:
		  return strmode[0];
		case PKW_inputOutput:
		  return strmode[3];
		case PKW_inputOnly:
		  return strmode[1];
		case PKW_outputOnly:
			return strmode[2];
		default:
			break;
	}
	return "_udef_"; /* gets rid of compile time warnings */
}
void print_field_value(FILE *fp, int typeIndex, union anyVrml* value);

void propagate_events_B() {
	int havinterp;
	int counter;
	int to_counter;

	union anyVrml *fromAny, *toAny; //dug9
	struct X3D_Node *fromNode, *toNode, *lastFromNode;
	int fromOffset, toOffset, lastFromOffset, last_markme;
#ifdef HAVE_JAVASCRIPT
    int markme;
#endif
    
	int len, isize, type, sftype, isMF, extra, itime, nRoutesDone, modeFrom, modeTo, debugRoutes;

	CRnodeStruct *to_ptr = NULL;
	ppCRoutes p;
	ttglobal tg = gglobal();
	p = (ppCRoutes)tg->CRoutes.prv;

		#ifdef CRVERBOSE
		printf ("\npropagate_events start\n");
		#endif
	nRoutesDone = 0; //debug diagnosis
	type = INT_ID_UNDEFINED;

	/* increment the "timestamp" for this entry */
	p->thisIntTimeStamp ++; 
	lastFromOffset = -1; //used for from script
	lastFromNode = NULL; // "
	last_markme = FALSE; // "
	//#ifdef CRVERBOSE
	debugRoutes = 0;
	if(debugRoutes)printf("current time=%d\n",p->thisIntTimeStamp);
	//#endif
	do {
		havinterp=FALSE; /* assume no interpolators triggered */

		for (counter = 1; counter < p->CRoutes_Count-1; counter++) {
			//dug9 >> fromAny
			//JAS union anyVrml tempAny;
			fromNode = p->CRoutes[counter].routeFromNode;
			fromOffset = p->CRoutes[counter].fnptr;
			extra = p->CRoutes[counter].extra;
			//len = p->CRoutes[counter].len; //this has -ve sentinal values - we need +ve
			itime = p->CRoutes[counter].intTimeStamp;
			switch(fromNode->_nodeType)
			{
				case NODE_ShaderProgram:
				case NODE_ComposedShader:
				case NODE_PackagedShader:
				case NODE_Script:
					{
						//JAS struct X3D_Script* scr = (struct X3D_Script*)fromNode;
						struct Shader_Script* shader = NULL;
						struct ScriptFieldDecl* sfield;
						switch(fromNode->_nodeType) 
						{ 
							case NODE_Script:         shader =(struct Shader_Script *)(X3D_SCRIPT(fromNode)->__scriptObj); break;
							case NODE_ComposedShader: shader =(struct Shader_Script *)(X3D_COMPOSEDSHADER(fromNode)->_shaderUserDefinedFields); break;
							case NODE_ShaderProgram:  shader =(struct Shader_Script *)(X3D_SHADERPROGRAM(fromNode)->_shaderUserDefinedFields); break;
							case NODE_PackagedShader: shader =(struct Shader_Script *)(X3D_PACKAGEDSHADER(fromNode)->_shaderUserDefinedFields); break;
						}
						sfield= vector_get(struct ScriptFieldDecl*, shader->fields, fromOffset);
						fromAny = &sfield->value;

						type = sfield->fieldDecl->fieldType;
						isMF = type % 2;
						sftype = type - isMF;
						//from EAI_C_CommonFunctions.c
						isize = returnElementLength(sftype) * returnElementRowSize(sftype);
						if(isMF) len = sizeof(int) + sizeof(void*);
						else len = isize;
						modeFrom = sfield->fieldDecl->PKWmode;

#ifdef HAVE_JAVASCRIPT
						if(fromNode->_nodeType == NODE_Script){
							//continue; //let the gatherScriptEventOuts(); copy directly toNode.
							//there's an expensive operation in here, and the route fanout doesn't work
							//so we'll check if this is the same fromNode/fromOffset as the last loop and skip 
							markme = last_markme;
							if(!(fromNode==lastFromNode && fromOffset==lastFromOffset)){
								//gatherScriptEventOut_B copies from javascript to the script field ->value
								int JSparamNameIndex = sfield->fieldDecl->JSparamNameIndex;
								markme = gatherScriptEventOut_B(fromAny,shader,JSparamNameIndex,type,extra,len);
							}
							if(markme){
								if (p->CRoutes[counter].intTimeStamp!=p->thisIntTimeStamp) {
									p->CRoutes[counter].isActive=TRUE;
									p->CRoutes[counter].intTimeStamp=p->thisIntTimeStamp;
								}
							}
							last_markme = markme;
						}

#endif //HAVE_JAVASCRIPT

					}
					break;
				case NODE_Proto:
					{
						struct ProtoFieldDecl* pfield;
						struct X3D_Proto* pnode = (struct X3D_Proto*)fromNode;
						struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
						pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, fromOffset);
						fromAny = &pfield->defaultVal;
						type = pfield->type;
						modeFrom = pfield->mode;
					}
					break;
				default: //builtin
					{
						const int * offsets;
                    
						fromAny = (union anyVrml*)offsetPointer_deref(void *,fromNode , fromOffset);
						//I wish we had stored fromType when registering the route
						offsets = NODE_OFFSETS[fromNode->_nodeType];
						while(*offsets > -1)
						{
							//printf("%d %d %d %d %d\n",offsets[0],offsets[1],offsets[2],offsets[3],offsets[4]);
							if(offsets[1]==fromOffset) 
							{
								type = offsets[2];
								modeFrom = PKW_from_KW(offsets[3]);
								break;
							}
							offsets += 5;
						}
					}
					break;
			}

			
			isMF = type % 2;
			sftype = type - isMF;
			//from EAI_C_CommonFunctions.c
			isize = returnElementLength(sftype) * returnElementRowSize(sftype);
			if(isMF) len = sizeof(int) + sizeof(void*);
			else len = isize;
			


			for (to_counter = 0; to_counter < p->CRoutes[counter].tonode_count; to_counter++) {
				modeTo = PKW_inputOnly;
				to_ptr = &(p->CRoutes[counter].tonodes[to_counter]);
				if (to_ptr == NULL) {
					printf("WARNING: tonode at %u is NULL in propagate_events.\n",
							to_counter);
					continue;
				}

				#ifdef CRVERBOSE
					printf("propagate_events: counter %d to_counter %u act %s from %u off %u to %u off %u oint %u dir %d\n",
						   counter, to_counter, BOOL_STRING(p->CRoutes[counter].isActive),
						   p->CRoutes[counter].routeFromNode, p->CRoutes[counter].fnptr,
						   to_ptr->routeToNode, to_ptr->foffset, p->CRoutes[counter].interpptr,
							p->CRoutes[counter].direction_flag);
				#endif

				if (p->CRoutes[counter].isActive == TRUE) {
					/* first thing, set this to FALSE */
					p->CRoutes[counter].isActive = FALSE;
					/* to get routing to/from exposedFields, lets
					 * mark this to/offset as an event */
					//MARK_EVENT (to_ptr->routeToNode, to_ptr->foffset);


					//dug9 >> toAny
					toNode = to_ptr->routeToNode; //p->CRoutes[counter].routeFromNode;
					toOffset = to_ptr->foffset; //p->CRoutes[counter].fnptr;
					//MARK_EVENT(toNode, toOffset);

					switch(toNode->_nodeType)
					{
						case NODE_ShaderProgram:
						case NODE_ComposedShader:
						case NODE_PackagedShader:
						case NODE_Script:
							{
								struct Shader_Script* shader = NULL;
								struct ScriptFieldDecl* sfield;
								switch(toNode->_nodeType) 
								{ 
  									case NODE_Script:         shader =(struct Shader_Script *)(X3D_SCRIPT(toNode)->__scriptObj); break;
  									case NODE_ComposedShader: shader =(struct Shader_Script *)(X3D_COMPOSEDSHADER(toNode)->_shaderUserDefinedFields); break;
  									case NODE_ShaderProgram:  shader =(struct Shader_Script *)(X3D_SHADERPROGRAM(toNode)->_shaderUserDefinedFields); break;
  									case NODE_PackagedShader: shader =(struct Shader_Script *)(X3D_PACKAGEDSHADER(toNode)->_shaderUserDefinedFields); break;
								}
								sfield= vector_get(struct ScriptFieldDecl*, shader->fields, toOffset);
								toAny = &sfield->value;
								modeTo = sfield->fieldDecl->PKWmode;
							}
							break;
						case NODE_Proto:
							{
								struct ProtoFieldDecl* pfield;
								struct X3D_Proto* pnode = (struct X3D_Proto*)toNode;
								struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
								pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, toOffset);
								toAny = &pfield->defaultVal;
								modeTo = pfield->mode;
							}
							break;
						default: //builtin
							toAny = (union anyVrml*)offsetPointer_deref(void *,toNode , toOffset);
							//I wish we stored toMode when registering the route
							{
								const int *offsets = NODE_OFFSETS[toNode->_nodeType];
								while(*offsets > -1)
								{
									//printf("%d %d %d %d %d\n",offsets[0],offsets[1],offsets[2],offsets[3],offsets[4]);
									if(offsets[1]==fromOffset) 
									{
										modeTo = PKW_from_KW(offsets[3]);
										break;
									}
									offsets += 5;
								}
							}
							break;
					}

					//we now have from and to as *anyVrml, so lets copy
					//there should be a shallow_clean_field(type,toAny) that releases old mallocs 
					//  in UniString,MF p*, unlinks and/or killNodes
					//clean_field()
					//  1. is toField a valueHolding field (inputOutput,initializeOnly)?
					//  2. if yes, is toField a node field (SFNode, MFNode)?
					//  3. if yes, is there something in toField now?
					//  4. if yes, get it, remove toNode as parent, refcount-- (let killNode in startofloopnodeupdates garbage collect it)
					//  5. if it was an MFNode, release the p* array
					cleanFieldIfManaged(type,modeTo,1,toNode,toOffset); //see unlink_node/killNode policy
					shallow_copy_field(type,fromAny,toAny);
					//if(isMF && sftype == FIELDTYPE_SFNode)
					//	add_mfparents(toNode,toAny,type);
					registerParentIfManagedField(type,modeTo,1, toAny, toNode); //see unlink_node/killNode policy
					//OK we copied. 
					//if(extra == 1 || extra == -1)
					mark_event_B(fromNode,fromOffset, toNode, toOffset);
					//MARK_EVENT(toNode, toOffset);

					//#ifdef CRVERBOSE
					if(debugRoutes){
						char *fromName, *toName, *fromFieldName, *toFieldName, *fromModeName, *toModeName, *fromNodeType, *toNodeType;
						char fromNameP[100], toNameP[100];
						sprintf(fromNameP,"%p",fromNode);
						sprintf(toNameP,"%p",toNode);
						fromName = parser_getNameFromNode(fromNode);
						if(!fromName) fromName = &fromNameP[0];
						toName   = parser_getNameFromNode(toNode);
						if(!toName) toName = &toNameP[0];
						fromFieldName = findFIELDNAMES0(fromNode,fromOffset);
						toFieldName = findFIELDNAMES0(toNode,toOffset);
						if(!toName) toName = &toNameP[0];
						fromNodeType = (char *)stringNodeType(fromNode->_nodeType);
						if(fromNode->_nodeType == NODE_Proto)
							fromNodeType = ((struct ProtoDefinition*)(X3D_PROTO(fromNode)->__protoDef))->protoName;
						toNodeType = (char *)stringNodeType(toNode->_nodeType);
						if(toNode->_nodeType == NODE_Proto)
							toNodeType = ((struct ProtoDefinition*)(X3D_PROTO(toNode)->__protoDef))->protoName;
						fromModeName = (char *)stringMode(modeFrom,1);
						toModeName = (char *)stringMode(modeTo, 1);
						printf(" %s %s.%s %s TO %s %s.%s %s %d ",fromNodeType,fromName,fromFieldName,fromModeName,
							toNodeType,toName,toFieldName,toModeName,itime);
						print_field_value(stdout,type,toAny);
						printf("\n");

					}
					//#endif
					nRoutesDone++;
					//Some target node types need special processing ie sensors and scripts
					switch(toNode->_nodeType)
					{
						case NODE_Script:
							{
#ifdef HAVE_JAVASCRIPT
								//OLDCODE struct X3D_Script* scr = (struct X3D_Script*)toNode;
								struct Shader_Script* shader;
								struct ScriptFieldDecl* sfield;
								shader =(struct Shader_Script *)(X3D_SCRIPT(toNode)->__scriptObj);
								//{
								//	int kk;
								//	struct CRjsnameStruct *JSparamnames; // = getJSparamnames();
								//	JSObject *eventInFunction;
								//	JSparamnames = getJSparamnames();

								//	for(kk=0;kk<shader->fields->n;kk++)
								//	{
								//		sfield= vector_get(struct ScriptFieldDecl*, shader->fields, kk);
								//		//printf("sfield[%d]=%d",kk,sfield->fieldDecl->JSparamNameIndex);
								//		eventInFunction = JSparamnames[sfield->fieldDecl->JSparamNameIndex].eventInFunction;
								//		//printf(" func= %d\n",eventInFunction);
								//	}

								//}
								sfield= vector_get(struct ScriptFieldDecl*, shader->fields, toOffset);

								//if (p->ScriptControl[shader->num]._initialized && p->ScriptControl[shader->num].scriptOK) 
								if(isScriptControlInitialized(shader->num) && isScriptControlOK(shader->num))
								{
									int JSparamNameIndex = sfield->fieldDecl->JSparamNameIndex;
									/* mark that this script has been active SCRIPTS ARE INTEGER NUMBERS */
									mark_script(shader->num);
									if(isMF){ 
										// note the casting of parameter 4, the toAny type
										getField_ToJavascript_B(shader->num, JSparamNameIndex, type, (union anyVrml* ) toAny->mfnode.p, toAny->mfnode.n); //mfp->p, mfp->n);
									} else {
										getField_ToJavascript_B(shader->num, JSparamNameIndex, type, toAny, len);
									}
								}else{
									/* printf ("waiting for initializing script %d at %s:%d\n",(uintptr_t)to_ptr->routeToNode, __FILE__,__LINE__); */
								}
								havinterp = TRUE;
#endif //HAVE_JAVASCRIPT
							}
							break;
						case NODE_ShaderProgram:
						case NODE_ComposedShader:
						case NODE_PackagedShader:
							{
								struct Shader_Script* shader = NULL;
								switch(toNode->_nodeType) 
								{ 
  									case NODE_ComposedShader: shader =(struct Shader_Script *)(X3D_COMPOSEDSHADER(toNode)->_shaderUserDefinedFields); break;
  									case NODE_ShaderProgram:  shader =(struct Shader_Script *)(X3D_SHADERPROGRAM(toNode)->_shaderUserDefinedFields); break;
  									case NODE_PackagedShader: shader =(struct Shader_Script *)(X3D_PACKAGEDSHADER(toNode)->_shaderUserDefinedFields); break;
								}
								// note, "shader" can not be NULL here...
								// otherwise we'd never be here in this switch
								getField_ToShader(toNode, shader->num);
								havinterp = TRUE;
							}
							break;
						default:
							havinterp = FALSE;
							break;
					}
					if (p->CRoutes[counter].interpptr != 0) 
					{
						/* this is an interpolator, call it */
						havinterp = TRUE;
						#ifdef CRVERBOSE
						printf("propagate_events: index %d is an interpolator\n",counter);
						#endif
						/* copy over this "extra" data, EAI "advise" calls need this */
						tg->CRoutes.CRoutesExtra = p->CRoutes[counter].extra;
						p->CRoutes[counter].interpptr((void *)(toNode));
					} else {
						/* just an eventIn node. signal to the reciever to update */
						//marked above 
						//MARK_EVENT(toNode, toOffset);

						/* make sure that this is pointing to a real node,
						 * not to a block of memory created by
						 * EAI - extra memory - if it has an offset of
						 * zero, it is most certainly made. 
						 * dug9,feb2013: with new Broto code I changed Proto and Script offset 
						 * to field index (0 to nfield-1) (instead of offset=JSparamNameIndex)
						 */
						//dug9 if(toOffset != 0)
							update_node(toNode);
					}
				} //if isActive
			} //for(to_counter)
			lastFromNode = fromNode;
			lastFromOffset = fromOffset;
		} //for(counter)

		#ifdef HAVE_JAVASCRIPT
		/* run gatherScriptEventOuts for each active script */
		//gatherScriptEventOuts();
		#endif
	} while (havinterp==TRUE);

	#ifdef HAVE_JAVASCRIPT
	/* now, go through and clean up all of the scripts */
	for (counter =0; counter <= tg->CRoutes.max_script_found_and_initialized; counter++) {
		if (tg->CRoutes.scr_act[counter]) {
			tg->CRoutes.scr_act[counter] = FALSE;
			js_cleanup_script_context(counter);
			//CLEANUP_JAVASCRIPT(p->ScriptControl[counter].cx);
		}
	}	
	#endif /* HAVE_JAVASCRIPT */
	if(debugRoutes){
		printf(" *\n");
		if(nRoutesDone)
			getchar();
	}
	#ifdef CRVERBOSE
	printf ("done propagate_events\n\n");
	#endif
}
/* BOOL usingBrotos(); - moved to CParseParser.h */
void propagate_events()
{
	if( usingBrotos() )
		propagate_events_B();
	else
		propagate_events_A();
}


/*******************************************************************

do_first()


Call the sensor nodes to get the results of the clock ticks; this is
the first thing in the event loop.

********************************************************************/
void printStatsEvents(){
	ConsoleMessage("%25s %d\n","ClockEvent count", ((ppCRoutes)gglobal()->CRoutes.prv)->num_ClockEvents);
}
void do_first() {
	int counter, ne;
	struct FirstStruct ce;
	/* go through the array; add_first will NOT add a null pointer
	   to either field, so we don't need to bounds check here */
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	ne = p->num_ClockEvents;
	for (counter =0; counter < ne; counter ++) {
		ce = p->ClockEvents[counter]; 
		if (ce.tonode)
			ce.interpptr(ce.tonode);
	}
	//for (counter = 0; counter < p->num_ClockEvents; counter++) {
	//	if (p->ClockEvents[counter].tonode)
	//		p->ClockEvents[counter].interpptr(p->ClockEvents[counter].tonode);
	//}

	/* now, propagate these events */
	propagate_events();

	/* any new routes waiting in the wings for buffering to happen? */
	/* Note - rTr will be incremented by either parsing (in which case,
	   events are not run, correct?? or by a script within a route,
	   which will be this thread, or by EAI, which will also be this
	   thread, so the following should be pretty thread_safe */ 

	if (p->routesToRegister != NULL) {
		MUTEX_LOCK_ROUTING_UPDATES
		actually_do_CRoutes_Register();
		MUTEX_FREE_LOCK_ROUTING_UPDATES
	}

	/* any mark_events kicking around, waiting for someone to come in and tell us off?? */
	/* CRoutes_Inititated should be set here, as it would have been created in 
	   actually_do_CRoutes_Register */
	if (p->preEvents != NULL) {
		if (p->CRoutes_Initiated) {
		LOCK_PREROUTETABLE

		#ifdef CRVERBOSE
		printf ("doing preEvents, we have %d events \n",p->initialEventBeforeRoutesCount);
		#endif

		for (counter = 0; counter < p->initialEventBeforeRoutesCount; counter ++) {
			MARK_EVENT(p->preEvents[counter].from, p->preEvents[counter].totalptr);
		}
		p->initialEventBeforeRoutesCount = 0;
		p->preRouteTableSize = 0;
		FREE_IF_NZ(p->preEvents);
		UNLOCK_PREROUTETABLE
		}
	}
}


/*******************************************************************

Interface to allow EAI/SAI to get routing information.

********************************************************************/

int getRoutesCount(void) {
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	return p->CRoutes_Count;
}

void getSpecificRoute (int routeNo, struct X3D_Node **fromNode, int *fromOffset, 
		struct X3D_Node **toNode, int *toOffset) {
        CRnodeStruct *to_ptr = NULL;
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;


	if ((routeNo <1) || (routeNo >= p->CRoutes_Count)) {
		*fromNode = NULL; *fromOffset = 0; *toNode = NULL; *toOffset = 0;
	}
/*
	printf ("getSpecificRoute, fromNode %d fromPtr %d tonode_count %d\n",
		CRoutes[routeNo].routeFromNode, CRoutes[routeNo].fnptr, CRoutes[routeNo].tonode_count);
*/
		*fromNode = p->CRoutes[routeNo].routeFromNode;
		*fromOffset = p->CRoutes[routeNo].fnptr;
	/* there is not a case where tonode_count != 1 for a valid route... */
	if (p->CRoutes[routeNo].tonode_count != 1) {
		printf ("huh? tonode count %d\n",p->CRoutes[routeNo].tonode_count);
		*toNode = 0; *toOffset = 0;
		return;
	}

	/* get the first toNode,toOffset */
        to_ptr = &(p->CRoutes[routeNo].tonodes[0]);
        *toNode = to_ptr->routeToNode;
	*toOffset = to_ptr->foffset;


	

}
/*******************************************************************

kill_routing()

Stop routing, remove structure. Used for ReplaceWorld style calls.

********************************************************************/

void kill_routing (void) {
	ppCRoutes p = (ppCRoutes)gglobal()->CRoutes.prv;

	//ConsoleMessage ("kill_routing called\n");

        if (p->CRoutes_Initiated) {
                p->CRoutes_Initiated = FALSE;
                p->CRoutes_Count = 0;
                p->CRoutes_MAX = 0;
                FREE_IF_NZ (p->CRoutes);
        }
}


/* internal variable to copy a C structure's Multi* field */
void Multimemcpy (struct X3D_Node *toNode, struct X3D_Node *fromNode, void *tn, void *fn, size_t multitype) {
	size_t structlen;
	int fromcount;

	#ifdef CRVERBOSE
	int tocount;
	#endif

	void *fromptr, *toptr;

	struct Multi_Vec3f *mv3ffn, *mv3ftn;

	#ifdef CRVERBOSE 
		printf ("Multimemcpy, copying structures from %p (%s) to %p (%s)  %p %p type %d\n",
			fromNode, stringNodeType(fromNode->_nodeType),
			toNode, stringNodeType(toNode->_nodeType),
			
			tn,fn,multitype); 
	#endif

	/* copy a complex (eg, a MF* node) node from one to the other
	   grep for the ROUTING_SF and ROUTING_MF defines to see them all. */

	/* Multi_XXX nodes always consist of a count then a pointer - see
	   Structs.h */

	/* making the input pointers into a (any) structure helps deciphering params */
	mv3ffn = (struct Multi_Vec3f *)fn;
	mv3ftn = (struct Multi_Vec3f *)tn;

	/* so, get the from memory pointer, and the to memory pointer from the structs */
	fromptr = (void *)mv3ffn->p;

	/* and the from and to sizes */
	fromcount = mv3ffn->n;
	//printf("fn = %u value *fn = %u fromcount = %u\n",(unsigned int)fn, *(unsigned int *)fn, (unsigned int) fromcount);

	#ifdef CRVERBOSE 
		tocount = mv3ftn->n;
		printf ("Multimemcpy, fromcount %d\n",fromcount);
	#endif

	/* get the structure length */
	switch (multitype) {
		case ROUTING_SFNODE: structlen = sizeof (void *); break;
		case ROUTING_MFNODE: structlen = sizeof (void *); break;
		case ROUTING_SFIMAGE: structlen = sizeof (void *); break;
		case ROUTING_MFSTRING: structlen = sizeof (void *); break;
		case ROUTING_MFFLOAT: structlen = sizeof (float); break;
		case ROUTING_MFROTATION: structlen = sizeof (struct SFRotation); break;
		case ROUTING_MFINT32: structlen = sizeof (int); break;
		case ROUTING_MFCOLOR: structlen = sizeof (struct SFColor); break;
		case ROUTING_MFVEC2F: structlen = sizeof (struct SFVec2f); break;
		case ROUTING_MFVEC3F: structlen = sizeof (struct SFColor); break; /* This is actually SFVec3f - but no struct of this type */
		case ROUTING_MFVEC3D: structlen = sizeof (struct SFVec3d); break;
		case ROUTING_MFDOUBLE: structlen = sizeof (double); break;
		case ROUTING_MFMATRIX4F: structlen = sizeof (struct SFMatrix4f); break;
		case ROUTING_MFMATRIX4D: structlen = sizeof (struct SFMatrix4d); break;
		case ROUTING_MFVEC2D: structlen = sizeof (struct SFVec2d); break;
		case ROUTING_MFVEC4F: structlen = sizeof (struct SFVec4f); break;
		case ROUTING_MFVEC4D: structlen = sizeof (struct SFVec4d); break;
		case ROUTING_MFMATRIX3F: structlen = sizeof (struct SFMatrix3f); break;
		case ROUTING_MFMATRIX3D: structlen = sizeof (struct SFMatrix3d); break;

		case ROUTING_SFSTRING: { 
			/* SFStrings are "special" */
			/* remember:
				struct Uni_String {
				        int len;
				        char * strptr;
				        int touched;
			};
			*/
			struct Uni_String *fStr; 
			struct Uni_String *tStr;

			/* get the CONTENTS of the fn and tn pointers */
			memcpy (&fStr,fn,sizeof (void *));
			memcpy (&tStr,tn,sizeof (void *));


			/* printf ("copying over a SFString in Multi from %u to %u\n",fStr, tStr);
			printf ("string was :%s:\n",tStr->strptr); */
			verify_Uni_String(tStr, fStr->strptr); 
			/* printf ("string is :%s:\n",tStr->strptr); */
			return; /* we have done the needed stuff here */
			break;
		}
		default: {
			 /* this is MOST LIKELY for an EAI handle_Listener call - if not, it is a ROUTING problem... */
			/* printf("WARNING: Multimemcpy, don't handle type %d yet\n", multitype);  */
			structlen=0;
			return;
		}
	}


	if(multitype==ROUTING_SFNODE){
		/* and do the copy of the data */
		memcpy (tn,fn,structlen);
		//*(unsigned int)toptr = (unsigned int)fromcount;
		//memcpy(toptr,&fromcount,structlen);
		//printf("tn=%u *tn=%u\n",tn,*(unsigned int *)tn);
	}else{
		int nele = fromcount;
		FREE_IF_NZ (mv3ftn->p);
		/* MALLOC the toptr */
		if( multitype == ROUTING_MFNODE ) nele = (int) upper_power_of_two(nele);
		mv3ftn->p = MALLOC (struct SFVec3f *, structlen*nele); //fromcount);
		toptr = (void *)mv3ftn->p;

		/* tell the recipient how many elements are here */
		mv3ftn->n = fromcount;

		#ifdef CRVERBOSE 
			printf ("Multimemcpy, fromcount %d tocount %d fromptr %p toptr %p\n",fromcount,tocount,fromptr,toptr); 
		#endif

		/* and do the copy of the data */
		memcpy (toptr,fromptr,structlen * fromcount);
	}
	/* is this an MFNode or SFNode? */
	{
	//ppEAICore p = (ppEAICore)gglobal()->EAICore.prv;
	if (toNode != (struct X3D_Node*) gglobal()->EAICore.EAIListenerData) {
		if (multitype==ROUTING_SFNODE) {
			unsigned int fnvalue;
			unsigned int *fnlocation;
			struct X3D_Node *sfnodeptr;
			fnlocation = (unsigned int*)fn;
			fnvalue= *fnlocation;
			sfnodeptr = (struct X3D_Node*)fnvalue;
#ifdef CRVERBOSE
			printf ("got a ROUTING_SFNODE, adding %u to %u\n",(unsigned int) fn, (unsigned int) toNode);
#endif
			ADD_PARENT(X3D_NODE(sfnodeptr),toNode);
		}
		if (multitype==ROUTING_MFNODE) {
			int count;
			struct X3D_Node **arrptr = (struct X3D_Node **)mv3ffn->p;

			#ifdef CRVERBOSE
			printf ("fromcount %d tocount %d\n",fromcount, tocount);
			printf ("ROUTING - have to add parents... \n");
			#endif

			for (count = 0; count < mv3ffn->n; count++) {
				#ifdef CRVERBOSE
				printf ("node in place %d is %u ",count,arrptr[count]);
				printf ("%s ",stringNodeType(arrptr[count]->_nodeType));
				printf ("\n");
				#endif

				ADD_PARENT(arrptr[count],toNode);
			}
		}
	}
	}
}


/*********************************************************************************************/

static struct X3D_Node *returnSpecificTypeNode(int requestedType, int *offsetOfsetValue, int *offsetOfvalueChanged) {
	struct X3D_Node *rv;

	rv = NULL;
	switch  (requestedType) {
                 #define SF_TYPE(fttype, type, ttype) \
                        case FIELDTYPE_##fttype: \
			rv = createNewX3DNode(NODE_Metadata##fttype); \
			*offsetOfsetValue = (int) offsetof (struct X3D_Metadata##fttype, setValue); \
			*offsetOfvalueChanged = (int) offsetof (struct X3D_Metadata##fttype, valueChanged); \
			break; 

                        #define MF_TYPE(fttype, type, ttype) \
                                SF_TYPE(fttype, type, ttype)

                        #include "VrmlTypeList.h"

                        #undef SF_TYPE
                        #undef MF_TYPE
			default: {
				printf ("returnSpecific, not found %d\n",requestedType);
			}
	}
	return rv;
}

#ifdef HAVE_OPENCL
static bool canRouteOnGPUTo(struct X3D_Node *me) {
    int i;
    
    if (me == NULL) return FALSE;
    printf ("canRouteOnGPUTo = %s\n",stringNodeType(me->_nodeType));
    for (i=0; i< vectorSize(me->_parentVector); i++) {
        struct X3D_Node *par = vector_get(struct X3D_Node *,me->_parentVector,i);
        printf ("parent %d is a %s\n",i,stringNodeType(par->_nodeType));
        switch (par->_nodeType) {
            case NODE_TriangleSet :
            case NODE_IndexedTriangleSet:
                return TRUE;
                break;
            default: return FALSE;
        }
    }
    
    return TRUE;
    
}
#endif //HAVE_OPENCL
