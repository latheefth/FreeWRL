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

#include "jsapi.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"

/* scripting function protos PLACED HERE, not in headers.h,
   because these are shared only between this and JScript.c,
   and other modules dont require JavaScript headers */
int ActualrunScript(int num, char *script, jsval *rval);
void cleanupDie(int num, char *msg);

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
		.... ??


	

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
struct CRjsnameStruct {
	int	type;
	char	name[MAXJSVARIABLELENGTH];
};


struct CRStruct {
	unsigned int	fromnode;
	unsigned int	fnptr;
	unsigned int	tonode;
	unsigned int	tnptr;
	int	act;
	int	len;
	void	(*interpptr)(void *);
	int	script;
};

/* Routing table */
struct CRStruct CRoutes[MAXROUTES];
static int CRoutes_Initiated = FALSE;
int CRoutes_Count;

/* Structure table */
struct CRjsStruct JSglobs[MAXSCRIPTS];
int scr_act[MAXSCRIPTS];	/* this script has been sent an eventIn */
int scripts_active;		/* a script has been sent an eventIn */
int max_script_found = -1;	/* the maximum script number found -no need to search MAXSCRIPTS */

/* Script name/type table */
struct CRjsnameStruct JSparamnames[MAXPARAMS];
int jsnameindex = -1;


int CRVerbose = 0;

#define SFUNKNOWN 0
#define SFBOOL 	1
#define SFCOLOR 2
#define SFFLOAT 3
#define SFTIME 	4
#define SFINT32 5
#define SFSTRING 6
#define SFNODE	7
#define SFROTATION 8
#define SFVEC2F	9

#define MFBOOL 	11
#define MFCOLOR 12
#define MFFLOAT 13
#define MFTIME 	14
#define MFINT32 15
#define MFSTRING 16
#define MFNODE	17
#define MFROTATION 18
#define MFVEC2F	19

/* sets a SFBool, SFFloat, SFTime, SFIint32, SFString in a script */
void setECMAtype (int num) {
	char scriptline[100];
	int fn, tn, fptr, tptr;
	int len;
	jsval retval;
	float fl;
	double dl;
	int il;
	int intval;

	fn = (int) CRoutes[num].fromnode;
	tn = (int) CRoutes[num].tonode;
	fptr = (int) CRoutes[num].fnptr;
	tptr = (int) CRoutes[num].tnptr;
	len = CRoutes[num].len;
	
	
	switch (JSparamnames[tptr].type) {
		case SFBOOL:	{	/* SFBool */
			memcpy ((void *) &intval,(void *)fn+fptr, len);
			if (intval == 1) sprintf (scriptline,"__tmp_arg_%s=true",JSparamnames[tptr].name);
			else sprintf (scriptline,"__tmp_arg_%s=false",JSparamnames[tptr].name);
			
			break;
		}

		case SFFLOAT:	{
			memcpy ((void *) &fl,(void *)fn+fptr, len);
			sprintf (scriptline,"__tmp_arg_%s=%f",
				JSparamnames[tptr].name,fl);
			break;
		}
		case SFTIME:	{
			memcpy ((void *) &dl,(void *)fn+fptr, len);
			sprintf (scriptline,"__tmp_arg_%s=%f",
				JSparamnames[tptr].name,dl);
			break;
		}
		case SFINT32:	{ /* SFInt32 */
			memcpy ((void *) &il,(void *)fn+fptr, len);
			sprintf (scriptline,"__tmp_arg_%s=%d",
				JSparamnames[tptr].name,il);
			break;
		}
		default: {	printf ("WARNING: SHOULD NOT BE HERE! %d\n",JSparamnames[tptr].type);
		}
	}

	/* set property */
	if (!ActualrunScript(tn, scriptline ,&retval)) 
		printf ("failed to set parameter, line %s\n",scriptline);

	/* ECMAScriptNative SF nodes require a touched=0 */
	sprintf (scriptline,"___tmp_arg_%s__touched=0", JSparamnames[tptr].name);
	if (!ActualrunScript(tn, scriptline ,&retval)) 
		printf ("failed to set parameter, line %s\n",scriptline);


	/* and set the value */
	sprintf (scriptline,"%s(__tmp_arg_%s,%f)",
		JSparamnames[tptr].name,JSparamnames[tptr].name,
		TickTime);
	if (!ActualrunScript(tn, scriptline ,&retval)) {
		printf ("failed to set parameter, line %s\n",scriptline);
	}
}




/****************************************************************/
/* a script is returning a MFNode type; add this to the C	*/
/* children field						*/
/****************************************************************/

getMFNodetype (char *strp, struct Multi_Node *ch) {
	int newptr;
	int oldlen, newlen;
	char *cptr;
	void *newmal;


	/* printf ("getMFNodetype, %s\n",strp);
	printf ("getMFNodetype, parent has %d nodes currently\n",ch->n); */

	/* oldlen = what was there in the first place */
	oldlen = ch->n;
	newlen=0;

	/* this string will be in the form "[ CNode addr CNode addr....]" */
	/* count the numbers to add */
	if (*strp == '[') {
		strp++;
	}
	while (*strp == ' ') strp++; /* skip spaces */
	cptr = strp;

	while (sscanf (cptr,"%d",&newptr) == 1) {
		newlen++;
		/* skip past this number */
		while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
		while (*cptr == ' ') cptr++; /* skip spaces */
	}

	/* now we know how many SFNodes are in this MFNode, lets malloc and add */
	newmal = malloc ((oldlen+newlen)*sizeof(unsigned int));

	if (newmal == 0) {
		printf ("cant malloc memory for addChildren");
		return;
	}

	/* copy the old stuff over */
	memcpy (newmal,ch->p,oldlen*sizeof(unsigned int));

	/* set up the C structures for this new MFNode addition */
	free (ch->p);
	ch->p = newmal;
	ch->n = oldlen+newlen;

	newmal += sizeof (unsigned int)*oldlen;
	cptr = strp; /* reset this pointer to the first number */

	while (sscanf (cptr,"%d",newmal) == 1) {
		/* skip past this number */
		while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
		while (*cptr == ' ') cptr++; /* skip spaces */
		newmal += sizeof (unsigned int);
	}
}


/****************************************************************/
/* a script is returning a Multi-float type; copy this from 	*/
/* the script return string to the data structure within the	*/
/* freewrl C side of things.					*/
/*								*/
/* note - this cheats in that the code assumes that it is 	*/
/* a series of Multi_Vec3f's while in reality the structure	*/
/* of the multi structures is the same - so we "fudge" things	*/
/* to make this multi-purpose.					*/
/****************************************************************/

void getMultiElementtype (char *strp, struct Multi_Vec3f *tn, int eleperinex) {
	float *fl;
	int shouldfind;

	/* pass in a character string, a pointer to a Multi*float 
	   structure, and an indication of the number of elements per index;
	   eg, 3 = SFColor, 2 = SFVec2f, etc, etc */ 

	shouldfind = tn->n * eleperinex;
	fl = (float *) tn->p;

	if (*strp == '[') {
		strp++;
	}
	while (*strp == ' ') strp++; /* skip spaces */

	/* convert a series of numbers */
	while (sscanf (strp,"%f",fl) == 1) {
		fl ++;
		shouldfind --;
							
		/* increment past this number */
		while (isalnum(*strp) ||
			(*strp == '.') ||
			(*strp == ',') ||
			(*strp == '-')) strp++;
		while (*strp == ' ') strp++; /* skip spaces */

		if ((shouldfind == 0) && (*strp != ']')) {
			printf ("getMultiElementtype: string now is :%s: shouldfind %d\n",strp,shouldfind);
			return;
		}
	}
}


/****************************************************************/
/* sets a SFVec3f and SFColor in a script 			*/
/* sets a SFRotation and SFVec2fin a script 			*/
/*								*/
/* all *Native types have the same structure of the struct -	*/
/* we are just looking for the pointer, thus we can handle	*/
/* multi types here 						*/
/* sets a SFVec3f and SFColor in a script 			*/
/****************************************************************/

void setMultiElementtype (int num) {
	char scriptline[100];
	int fn, tn, fptr, tptr;
	int len;
	jsval retval;
	float fourl[4];
	SFVec3fNative *_privPtr; 

	JSContext *_context;
	JSObject *_globalObj, *_sfvec3fObj;

	fn = (int) CRoutes[num].fromnode;
	tn = (int) CRoutes[num].tonode;
	fptr = (int) CRoutes[num].fnptr;
	tptr = (int) CRoutes[num].tnptr;
	len = CRoutes[num].len;
	
	if (CRVerbose) {
		printf ("got a script event! index %d type %d\n",num,CRoutes[num].script);
		printf ("	from %x from ptr %x\n	to %x toptr %x\n",fn,fptr,tn,tptr);
		printf ("	data length %d\n",len);
		printf ("setMultiElementtype here tn %d tptr %d len %d\n",tn, tptr,len);
	}
	/* get context and global object for this script */
	_context = (JSContext *) JSglobs[tn].cx;
	_globalObj = (JSObject *)JSglobs[tn].glob;


	/* make up the name */
	sprintf (scriptline,"__tmp_arg_%s", JSparamnames[tptr].name);

	if (CRVerbose) printf ("script %d line %s\n",tn, scriptline);

	if (!JS_GetProperty(_context,_globalObj,scriptline,&retval)) 
		printf ("JS_GetProperty failed in jsSFVec3fSet.\n");

	if (!JSVAL_IS_OBJECT(retval)) 
		printf ("jsSFVec3fSet - not an object\n");

	_sfvec3fObj = JSVAL_TO_OBJECT(retval);

	if ((_privPtr = JS_GetPrivate(_context, _sfvec3fObj)) == NULL) 
		printf("JS_GetPrivate failed in jsSFVec3fSet.\n");

	/* copy over the data from the perl/C VRML side into the script. */
	memcpy ((void *) &_privPtr->v,(void *)fn+fptr, len);

	// required?? JAS _privPtr->touched = 0;


	/* now, runscript to tell it that it has been touched */
	sprintf (scriptline,"__tmp_arg_%s.__touched()", JSparamnames[tptr].name);
	if (!ActualrunScript(tn, scriptline ,&retval)) 
		printf ("failed to set parameter, line %s\n",scriptline);

	/* and run the function */
	sprintf (scriptline,"%s(__tmp_arg_%s,%f)",
		JSparamnames[tptr].name,JSparamnames[tptr].name,
		TickTime);
	if (!ActualrunScript(tn, scriptline ,&retval)) {
		printf ("failed to set parameter, line %s\n",scriptline);
	}
}

/* internal variable to copy a C structure's Multi* field */
void Multimemcpy (void *tn, void *fn, int len) {
	struct Multi_Vec3f *mv3ffn;
	struct Multi_Vec3f *mv3ftn;


	if (len == -1) {
		/* this is a Multi_Vec3f */
		mv3ffn = fn;
		mv3ftn = tn;
		if (CRVerbose) printf ("MultiMemcpy to %x from %x lenf %d lent %d\n",tn,fn,mv3ftn->n,mv3ftn->n);
		memcpy (mv3ftn->p,mv3ffn->p,sizeof(struct SFColor) * mv3ftn->n);
	} else {
		printf ("WARNING: Multimemcpy, don't handle type %d yet\n",len);
	}
}


/*******************************************************************

CRoutes_js_new;

Register a new script for future routing

********************************************************************/

void CRoutes_js_new (int num,unsigned int cx, unsigned int glob, unsigned int brow) {
	jsval retval;

	/* too many scripts? */
	if (num >=MAXSCRIPTS) {
		printf ("WARNING: too many scripts - recompile with larger MAXSCRIPTS\n");
	} else {
		if (num > max_script_found) max_script_found = num;
	}
}


/********************************************************************

JSparamIndex. 

stores ascii names with types (see code for type equivalences).

********************************************************************/

int JSparamIndex (char *name, char *type) {
	int len;
	int ty;
	int ctr;

	/* first, convert the type to an integer value */
	if (strncmp("SFBool",type,7) == 0) ty = SFBOOL;
	else if (strncmp ("SFColor",type,7) == 0) ty = SFCOLOR;
	else if (strncmp ("SFVec3f",type,7) == 0) ty = SFCOLOR; /*Colors and Vec3fs are same */
	else if (strncmp ("SFFloat",type,7) == 0) ty = SFFLOAT;
	else if (strncmp ("SFTime",type,6) == 0) ty = SFTIME;
	else if (strncmp ("SFInt32",type,6) == 0) ty = SFINT32;
	else if (strncmp ("SFString",type,6) == 0) ty = SFSTRING;
	else if (strncmp ("SFNode",type,6) == 0) ty = SFNODE;
	else if (strncmp ("SFVec2f",type,6) == 0) ty = SFVEC2F;
	else if (strncmp ("SFRotation",type,6) == 0) ty = SFROTATION;
	else if (strncmp("MFBool",type,7) == 0) ty = MFBOOL;
	else if (strncmp ("MFColor",type,7) == 0) ty = MFCOLOR;
	else if (strncmp ("MFVec3f",type,7) == 0) ty = MFCOLOR; /*Colors and Vec3fs are same */
	else if (strncmp ("MFFloat",type,7) == 0) ty = MFFLOAT;
	else if (strncmp ("MFTime",type,6) == 0) ty = MFTIME;
	else if (strncmp ("MFInt32",type,6) == 0) ty = MFINT32;
	else if (strncmp ("MFString",type,6) == 0) ty = MFSTRING;
	else if (strncmp ("MFNode",type,6) == 0) ty = MFNODE;
	else if (strncmp ("MFVec2f",type,6) == 0) ty = MFVEC2F;
	else if (strncmp ("MFRotation",type,6) == 0) ty = MFROTATION;

	else {
		printf ("WARNING: JSparamIndex, cant match type %s\n",type);
		ty = SFUNKNOWN;
	}

	len = strlen(name);

	/* is this a duplicate name and type? types have to be same,
	   name lengths have to be the same, and the strings have to be the same.
	*/
	for (ctr=0; ctr<=jsnameindex; ctr++) {
		if (ty==JSparamnames[ctr].type) {
			if ((strlen(JSparamnames[ctr].name) == len) && 
				(strncmp(name,JSparamnames[ctr].name,len)==0)) {
				return ctr;
			}
		}
	}
	
	/* nope, not duplicate */		

	jsnameindex ++;

	/* ok, we got a name and a type */
	if (jsnameindex >= MAXPARAMS) {
		printf ("WARNING: too many Javascripts - recompile with larger MAXPARAMS\n");
		jsnameindex = 0;
		return 0; /* oh well! */
	}

	if (len > MAXJSVARIABLELENGTH-2) len = MAXJSVARIABLELENGTH-2;	/* concatenate names to this length */
	strncpy (JSparamnames[jsnameindex].name,name,len);
	JSparamnames[jsnameindex].name[len+1] = 0; /* make sure terminated */
	JSparamnames[jsnameindex].type = ty;
	return jsnameindex;
}

/********************************************************************

CRoutes_Register. 

Register a route in the routing table.

********************************************************************/


unsigned int CRoutes_Register (unsigned int from, int fromoffset, 
			unsigned int to, int tooffset,
			int length, void *intptr, int script) {

	int insert_here;
	int shifter;
	int count;


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


		/* and mark all scripts inactive */
		for (count=0; count<MAXSCRIPTS; count++) 
			scr_act[count]= FALSE;
		scripts_active = FALSE;
	}

	if (CRVerbose) printf ("CRoutes_Register from %x off %x to %x off %x len %d intptr %x \n",from, fromoffset,
		to,tooffset,length, intptr);

	insert_here = 1;

	/* go through the routing list, finding where to put it */
	while (from > CRoutes[insert_here].fromnode) {
		if (CRVerbose) printf ("comparing %x to %x\n",from, CRoutes[insert_here].fromnode);
		insert_here++; 
	}

	/* hmmm - do we have a route from this node already? If so, go
	   through and put the offsets in order */
	while ((from==CRoutes[insert_here].fromnode) &&
		((from + fromoffset) > CRoutes[insert_here].fnptr)) { 
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
	CRoutes[insert_here].script = script;

	/* record that we have one more route, with upper limit checking... */
	if (CRoutes_Count >= (MAXROUTES-2)) {
		printf ("WARNING: Maximum number of routes exceeded\n");
	} else {
		CRoutes_Count ++;
	}
	if (CRVerbose) for (shifter = 1; shifter < (CRoutes_Count-1); shifter ++) {
			printf ("Route indx %d is (%x %x) to (%x %x) len %d\n",
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

mark_script - indicate that this script has had an eventIn
zero_scripts - reset all script indicators

********************************************************************/
void mark_script (int num) {

	if (CRVerbose) printf ("mark_script - script %d has been invoked\n",num); 
	scr_act[num]= TRUE;
	scripts_active = TRUE;
}


void zero_scripts () {
	/* mark all scripts inactive */
	int count;

	for (count = 0; count < MAXSCRIPTS; count ++)
		scr_act[count] = FALSE;
	scripts_active = FALSE;
}


/********************************************************************

gatherScriptEventOuts - at least one script has been triggered; get the
eventOuts for this script

FIXME XXXXX =  can we do this without the string conversions?

********************************************************************/

void gatherScriptEventOuts(int script, int ignore) {
	int route;	
	char scriptline[100];
	jsval retval;
	int fn, tn, fptr, tptr;
	int len;
	float fl[0];	/* return float values */
	double tval;
	int ival;
	jsval touched;		/* was this really touched? */

        JSString *strval; /* strings */
        char *strp;
	char *strtouched;
	int fromalready;	 /* we have already got the from value string */

	fromalready=FALSE; 


	/* go through all routes, looking for this script as an eventOut */

	/* do we have any routes yet? - we can gather events before any routes are made */
	if (!CRoutes_Initiated) return;

	/* routing table is ordered, so we can walk up to this script */
	route=1;
	while (CRoutes[route].fromnode<script) route++;
	
	while (CRoutes[route].fromnode == script) {

		/* is this the same from node/field as before? */
		if ((CRoutes[route].fromnode == CRoutes[route-1].fromnode) &&
			(CRoutes[route].fnptr == CRoutes[route-1].fnptr) &&
			(route > 1)) {
			fromalready=TRUE;
		} else {
			/* printf ("different from, have to get value\n"); */
			fromalready=FALSE;
		}
		
		fn = CRoutes[route].fromnode;	
		fptr = CRoutes[route].fnptr;
		len = CRoutes[route].len;
	
		/* in Ayla's Perl code, the following happened:
			MF* - run __touched_flag

			SFBool, SFFloat, SFTime, SFInt32, SFString-
				this is her $ECMASCriptNative; run _name_touched
				and _name_touched=0

			else, run _name.__touched()
		*/

		/* now, set the actual properties - switch as documented above */
		if (!fromalready) {
			switch (JSparamnames[fptr].type) {
			case MFBOOL:
			case MFFLOAT:
			case MFTIME:
			case MFINT32:
			case MFSTRING: {
				sprintf (scriptline,"_%s__touched_flag",JSparamnames[fptr].name);
				if (!ActualrunScript(script, scriptline ,&touched))
					printf ("WARNING: failed to set parameter, line %s\n",scriptline);
				
				sprintf (scriptline,"_%s__touched_flag=0",JSparamnames[fptr].name);
				if (!ActualrunScript(script, scriptline ,&retval)) 
					printf ("WARNING: failed to set parameter, line %s\n",scriptline);
				
				break;
				}
			
			case SFBOOL:
			case SFFLOAT:
			case SFTIME:
			case SFINT32:
			case SFSTRING: {
				sprintf (scriptline,"_%s_touched",JSparamnames[fptr].name);
				if (!ActualrunScript(script, scriptline ,&touched)) 
					printf ("WARNING: failed to set parameter, line %s\n",scriptline);
				
				sprintf (scriptline,"_%s_touched=0",JSparamnames[fptr].name);
				if (!ActualrunScript(script, scriptline ,&retval)) 
					printf ("WARNING: failed to set parameter, line %s\n",scriptline);
				
				break;
				}

			case MFCOLOR:
			case MFROTATION: 
			case MFVEC2F:
			case MFNODE: {
				sprintf (scriptline,"%s.__touched_flag",JSparamnames[fptr].name);
				if (!ActualrunScript(script, scriptline ,&touched)) 
					printf ("WARNING: failed to set parameter, line %s\n",scriptline);

				sprintf (scriptline,"%s.__touched_flag=0",JSparamnames[fptr].name);
				if (!ActualrunScript(script, scriptline ,&retval)) 
					printf ("WARNING: failed to set parameter, line %s\n",scriptline);

				break;
				}	

			case SFCOLOR: 
			case SFNODE:
			case SFROTATION:
			case SFVEC2F: {
				sprintf (scriptline,"%s.__touched()",JSparamnames[fptr].name);
				if (!ActualrunScript(script, scriptline ,&touched)) 
					printf ("WARNING: failed to set parameter, line %s\n",scriptline);
				break;
				}
			default: {
				printf ("WARNING, this type (%d) not handled yet\n",
					JSparamnames[fptr].type);
				}
			}

			strval = JS_ValueToString((JSContext *)JSglobs[script].cx, touched);
			strtouched = JS_GetStringBytes(strval);
			if (JSVerbose ) printf ("touched string is %s\n",strtouched);
			if (*strtouched!='0') {
				/* we did, so get the value */
				if (!ActualrunScript(script, JSparamnames[fptr].name ,&retval)) {
					printf ("WARNING: Failed to get value, line %s\n",scriptline);
				}
				strval = JS_ValueToString((JSContext *)JSglobs[script].cx, retval);
			        strp = JS_GetStringBytes(strval);
				if (JSVerbose) printf ("retval string is %s\n",strp);
			}
		}


		if (*strtouched!='0') {
			/* get some easy to use pointers */
			tn = (int) CRoutes[route].tonode;
			tptr = (int) CRoutes[route].tnptr;

			/* eventOuts go to VRML data structures */

			switch (JSparamnames[fptr].type) {
				case SFBOOL:	{	/* SFBool */
						/* printf ("we have a boolean, copy value over string is %s\n",strp); */
						if (strncmp(strp,"true",4)==0) {
							ival = 1;
						} else {
							/* printf ("ASSUMED TO BE FALSE\n"); */
							ival = 0;
						}	
						memcpy ((void *)tn+tptr, (void *)&ival,len);
						break;
					}

				case SFTIME: {
						sscanf (strp,"%f",&tval);
						//printf ("SFTime conversion numbers %f\n",tval);
						memcpy ((void *)tn+tptr, (void *)&tval,len);
						break;
				}
				case SFINT32: {
						sscanf (strp,"%d",&ival);
						//printf ("SFTime conversion numbers %f\n",ival);
						memcpy ((void *)tn+tptr, (void *)&ival,len);
						break;
				}
				case SFFLOAT: {
						sscanf (strp,"%f",&fl);
						memcpy ((void *)tn+tptr, (void *)&fl,len);
						break;
				}

				case SFVEC2F: {	/* SFVec2f */
						sscanf (strp,"%f %f",&fl[0],&fl[1]);
						//printf ("conversion numbers %f %f\n",fl[0],fl[1]);
						memcpy ((void *)tn+tptr, (void *)fl,len);
						break;
				}

				case SFCOLOR: {	/* SFColor */
						sscanf (strp,"%f %f %f",&fl[0],&fl[1],&fl[2]);
						//printf ("conversion numbers %f %f %f\n",fl[0],fl[1],fl[2]);
						memcpy ((void *)tn+tptr, (void *)fl,len);
						break;
				}

				case SFROTATION: {
						sscanf (strp,"%f %f %f %f",&fl[0],&fl[1],&fl[2],&fl[3]);
						//printf ("conversion numbers %f %f %f %f\n",fl[0],fl[1],fl[2],fl[3]);
						memcpy ((void *)tn+tptr, (void *)fl,len);
						break;
				}


				/* a series of Floats... */
				case MFCOLOR:
					{
						getMultiElementtype (strp, tn+tptr,3);
						break;
				}
				case MFFLOAT:
					{
						getMultiElementtype (strp, tn+tptr,1);
						break;
				}
				case MFROTATION:
					{
						getMultiElementtype (strp, tn+tptr,4);
						break;
				}
				case MFVEC2F: 
					{
						getMultiElementtype (strp, tn+tptr,2);
						break;
				}

				case MFNODE:
					{	getMFNodetype (strp,tn+tptr);
						break;
					}
				case MFTIME:
				case MFINT32:
				default: {	printf ("WARNING: unhandled from type %d\n",JSparamnames[fptr].type);
					}
			}

			/* tell this node now needs to redraw */
			update_node(CRoutes[route].tonode);
		} 
		route++;
	}
}

/********************************************************************

sendScriptEventIn.

this sends events to scripts that have eventIns defined.

********************************************************************/

void sendScriptEventIn(int num) {
	/* script value: 1: this is a from script route
			 2: this is a to script route
			 3: this is a from script to a script route */

	if (CRoutes[num].script == 2) {
		/* get the value from the VRML structure, in order to propagate it to a script */

		/* mark that this script has been active */
		mark_script(CRoutes[num].tonode);

		/* set the parameter */
		/* see comments in gatherScriptEventOuts to see exact formats */

		switch (JSparamnames[CRoutes[num].tnptr].type) {
			case SFBOOL:	
			case SFFLOAT:
			case SFTIME:
			case SFINT32:
			case SFSTRING: {
					setECMAtype(num);
					break;
				}
			case SFCOLOR: 
			case SFVEC2F:
			case SFROTATION: {
					setMultiElementtype(num);
					break;
				}
			case SFNODE:
			case MFBOOL:
			case MFCOLOR:
			case MFFLOAT:
			case MFTIME:
			case MFINT32:
			case MFSTRING:
			case MFNODE:
			case MFROTATION: {
					printf ("WARNING: entry set in sendScriptEventIn, but no code yet for type %d\n",
						JSparamnames[CRoutes[num].tnptr].type);
					break;
				}
			default : {printf ("WARNING: sendScriptEventIn value %d not handled yet\n",
					JSparamnames[CRoutes[num].tnptr].type);}
		}
	} else {
		printf ("WARNING: sendScriptEventIn, don't handle %d yet\n",CRoutes[num].script);
	}
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


	if (CRVerbose) printf ("\npropagate_events start\n");

	do {
		/* set all script flags to false - no triggers */
		zero_scripts();

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

				if (CRoutes[counter].script != 0) {
					/* scripts are a bit complex, so break this out */
					sendScriptEventIn(counter);
					if (scripts_active) havinterp = TRUE;
				} else {

					/* copy the value over */
					if (CRoutes[counter].len>0) {
						/* simple, fixed length copy */
						memcpy (CRoutes[counter].tonode + CRoutes[counter].tnptr, 
							CRoutes[counter].fromnode + CRoutes[counter].fnptr,
							CRoutes[counter].len);
					} else {
						/* this is a Multi*node, do a specialized copy */
						Multimemcpy (CRoutes[counter].tonode + CRoutes[counter].tnptr, 
							CRoutes[counter].fromnode + CRoutes[counter].fnptr,
							CRoutes[counter].len);
					}

					/* is this an interpolator? if so call the code to do it */
					if (CRoutes[counter].interpptr != 0) {
						/* this is an interpolator, call it */
						havinterp = TRUE;
						if (CRVerbose) printf ("propagate_events. index %d is an interpolator\n",counter);
						CRoutes[counter].interpptr(CRoutes[counter].tonode);
					} else {	
						/* just an eventIn node. signal to the reciever to update */
						update_node(CRoutes[counter].tonode);
					}
				}
			}
		}

		/* run gatherScriptEventOuts for each active script */
		if (scripts_active) {
			for (counter =0; counter <= max_script_found; counter++) {
				gatherScriptEventOuts (counter,TRUE);
			}
		}
		
	} while (havinterp==TRUE);

	if (CRVerbose) printf ("done propagate_events\n\n");
}
