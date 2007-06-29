/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "headers.h"
#include <math.h>


#include "jsapi.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"

#include "SensInterps.h"

/* old perl - eg, IRIX 6.5, perl 5.6.0 */
#ifndef STRUCT_SV
#define STRUCT_SV sv
#endif

/* defines for getting touched flags and exact Javascript pointers */

/****************************** ECMA types ******************************************/
/* where we have a Native structure to go along with it */
#define GETJSPTR_TYPE_A(thistype) \
			 case FIELDTYPE_##thistype:  {  \
				thistype##Native *ptr; \
        			if ((ptr = (thistype##Native *)JS_GetPrivate(cx, (JSObject *)retval)) == NULL) { \
                			printf( "JS_GetPrivate failed in get_touched_flag\n"); \
                			return JS_FALSE; \
				} \
				JSSFpointer = (uintptr_t *)ptr; /* save this for quick extraction of values */ \
				touched = ptr->touched; \
				break; \
			} 

#define RESET_TOUCHED_TYPE_A(thistype) \
                case FIELDTYPE_##thistype: { \
                        ((thistype##Native *)JSSFpointer)->touched = 0; \
                        break; \
                }       

#define GETJSPTR_TYPE_MF_A(thisMFtype,thisSFtype) \
	case FIELDTYPE_##thisMFtype: { \
		thisSFtype##Native *ptr; \
		jsval mainElement; \
		int len; \
		int i; \
		if (!JS_GetProperty(cx, (JSObject *)JSglobal_return_val, "length", &mainElement)) { \
			printf ("JS_GetProperty failed for \"length\" in get_touched_flag\n"); \
			return; \
		} \
		len = JSVAL_TO_INT(mainElement); \
		/* go through each element of the main array. */ \
		for (i = 0; i < len; i++) { \
			if (!JS_GetElement(cx, (JSObject *)JSglobal_return_val, i, &mainElement)) { \
				printf ("JS_GetElement failed for %d in get_touched_flag\n",i); \
				return FALSE; \
			} \
			if ((ptr = (thisSFtype##Native *)JS_GetPrivate(cx, (JSObject *)mainElement)) == NULL) { \
				printf( "JS_GetPrivate failed for obj in setField_javascriptEventOut.\n"); \
				return FALSE; \
			} \
			if (ptr->touched > 0) touched = TRUE; /* did this element change? */ \
			/* printf ("touched flag for element %d is %d\n",i,ptr->touched); */ \
		} \
		break; \
	} 

#define RESET_TOUCHED_TYPE_MF_A(thisMFtype,thisSFtype) \
	case FIELDTYPE_##thisMFtype: { \
		thisSFtype##Native *ptr; \
		jsval mainElement; \
		int len; \
		int i; \
		JSContext *cx; \
		cx = ScriptControl[actualscript].cx; \
		if (!JS_GetProperty(cx, (JSObject *)JSglobal_return_val, "length", &mainElement)) { \
			printf ("JS_GetProperty failed for \"length\" in get_touched_flag\n"); \
			break; \
		} \
		len = JSVAL_TO_INT(mainElement); \
		/* go through each element of the main array. */ \
		for (i = 0; i < len; i++) { \
			if (!JS_GetElement(cx, (JSObject *)JSglobal_return_val, i, &mainElement)) { \
				printf ("JS_GetElement failed for %d in get_touched_flag\n",i); \
				break; \
			} \
			if ((ptr = (thisSFtype##Native *)JS_GetPrivate(cx, (JSObject *)mainElement)) == NULL) { \
				printf( "JS_GetPrivate failed for obj in setField_javascriptEventOut.\n"); \
				break; \
			} \
			ptr->touched = 0; \
		} \
		break; \
	} 

/****************************** ECMA types ******************************************/
/* where we have to keep a table of old values, because we do not have "private" data */
#define FIND_IN_ECMA_TABLE(whatToFind) \
			while ((i < maxECMAVal) && (!found)) { \
				if (ECMAValues[i].JS_address = whatToFind) {\
					/* printf ("found value at %d\n",i); */\
					found = TRUE;\
				}\
			}

/* "Bool" might be already declared - we DO NOT want it to be declared as an "int" */
#define savedBool Bool
#ifdef Bool
#undef Bool
#endif

#define JS_ValueToBool JS_ValueToInt32
#define JS_ValueToFloat JS_ValueToNumber
#define JS_ValueToTime JS_ValueToNumber
#define FIND_TOUCHED_FOR_ECMA(thistype,ctype) \
				i=0;  found = FALSE; \
				FIND_IN_ECMA_TABLE(JSglobal_return_val) \
				if (found) { \
					if (!JS_ValueTo##thistype(cx, JSglobal_return_val, &temp)) { printf ("wow - ECMA problems\n"); }  \
					ECMAValues[i].touched = (temp != ECMAValues[i].old##thistype); \
					ECMAValues[i].old##thistype = (ctype) temp; \
					touched = ECMAValues[i].touched;\
				} else {\
					/* printf ("new ecma value\n"); */\
					maxECMAVal++;\
					ECMAValues[i].JS_address = JSglobal_return_val;\
					ECMAValues[i].touched = 1;\
					if (!JS_ValueTo##thistype(cx, JSglobal_return_val, &temp)) {\
						printf ("wow - ECMA problems\n");\
					}\
					ECMAValues[i].old##thistype = (ctype) temp; \
					touched = TRUE;\
				}

#define GET_ECMA_TOUCHED(thistype,ctype) \
	case FIELDTYPE_SF##thistype: {	int i; int found; ctype temp; \
			FIND_TOUCHED_FOR_ECMA(thistype,ctype) \
				break;\
			}

#define GET_ECMA_MF_TOUCHED(thistype,ctype) \
	case FIELDTYPE_MF##thistype: {	int i; int found; ctype temp; \
		i=0;  found = FALSE; \
		jsval mainElement; \
		int len; \
		if (!JS_GetProperty(cx, (JSObject *)JSglobal_return_val, "length", &mainElement)) { \
			printf ("JS_GetProperty failed for \"length\" in get_touched_flag\n"); \
			return; \
		} \
		len = JSVAL_TO_INT(mainElement); \
		/* go through each element of the main array. */ \
		for (i = 0; i < len; i++) { \
			if (!JS_GetElement(cx, (JSObject *)JSglobal_return_val, i, &mainElement)) { \
				printf ("JS_GetElement failed for %d in get_touched_flag\n",i); \
				return FALSE; \
			} \
			/*printf ("main element is %x for index %d\n",mainElement,i); \
			if (JSVAL_IS_NUMBER(mainElement)) printf ("	it is a NUMBER\n"); \
			if (JSVAL_IS_DOUBLE(mainElement)) printf ("	it is a DOUBLE\n"); \
			if (JSVAL_IS_INT(mainElement)) printf ("	it is a INT\n"); \
			if (JSVAL_IS_STRING(mainElement)) printf ("	it is a STRING\n"); \
			if (JSVAL_IS_OBJECT(mainElement)) printf ("	it is a OBJECT\n"); \
			if (JSVAL_IS_PRIMITIVE(mainElement)) printf ("	it is a PRIMITIVE\n"); \
			*/ \
			FIND_IN_ECMA_TABLE(mainElement) \
			if (found) { \
				if (!JS_ValueTo##thistype(cx, mainElement, &temp)) { printf ("wow - ECMA problems\n"); }  \
				ECMAValues[i].touched = (temp != ECMAValues[i].old##thistype); \
				ECMAValues[i].old##thistype = (ctype) temp; \
				touched = ECMAValues[i].touched;\
			} else {\
				/* printf ("GET_ECMA_MF_TOUCHED new ecma value\n"); */ \
				maxECMAVal++;\
				ECMAValues[i].JS_address = mainElement;\
				ECMAValues[i].touched = 1;\
				if (!JS_ValueTo##thistype(cx, mainElement, &temp)) {\
					printf ("wow - ECMA problems\n");\
				}\
				ECMAValues[i].old##thistype = (ctype) temp; \
				touched = TRUE;\
			}\
		} \
		break; \
	}

#define RESET_ECMA_MF_TOUCHED(thistype) \
	case FIELDTYPE_##thistype: {	int i; int found; \
		i=0;  found = FALSE; \
		jsval mainElement; \
		int len; \
		if (!JS_GetProperty((JSContext *) ScriptControl[actualscript].cx, (JSObject *)JSglobal_return_val, "length", &mainElement)) { \
			printf ("JS_GetProperty failed for \"length\" in reset_touched_flag\n"); \
		} \
		len = JSVAL_TO_INT(mainElement); \
		/* go through each element of the main array. */ \
		for (i = 0; i < len; i++) { \
			if (!JS_GetElement((JSContext *) ScriptControl[actualscript].cx, (JSObject *)JSglobal_return_val, i, &mainElement)) { \
				printf ("JS_GetElement failed for %d in get_touched_flag\n",i); \
			} \
			FIND_IN_ECMA_TABLE(mainElement) \
			if (found) { \
				ECMAValues[i].touched = 0;\
			} else {\
				printf ("RESET_ECMA_MF_TOUCHED new ecma value - huh?\n"); \
			}\
		} \
		break; \
	}

#define GET_ECMA_TOUCHED_String \
	case FIELDTYPE_SFString: {	JSString *temp; int i; int found; \
				i=0;  found = FALSE; \
				FIND_IN_ECMA_TABLE(JSglobal_return_val) \
		if (!JSVAL_IS_STRING(JSglobal_return_val)) printf ("is NOT a string\n"); else printf ("IS a string\n"); \
				if (found) { \
					temp = JS_ValueToString(cx, JSglobal_return_val); \
					ECMAValues[i].touched = (temp != ECMAValues[i].oldString); \
					ECMAValues[i].oldString = temp; \
					touched = ECMAValues[i].touched;\
				} else {\
					/* printf ("new ecma value\n"); */\
					maxECMAVal++;\
					ECMAValues[i].JS_address = JSglobal_return_val;\
					ECMAValues[i].touched = 1;\
					touched = TRUE;\
				}\
				break;\
			}

#define RESET_TOUCHED_TYPE_ECMA(thistype) \
	case FIELDTYPE_##thistype: { int i; int found; \
				i=0;  found = FALSE;  \
				FIND_IN_ECMA_TABLE(JSglobal_return_val) \
				if (found) { \
					ECMAValues[i].touched = 0; \
				} else { \
					printf ("did not find value for reset_touched\n"); \
				} \
				break; \
			}
/* in case Bool was defined above, restore the value */
#define Bool savedBool


void AddRemoveChildren (struct X3D_Box *parent, struct Multi_Node *tn, uintptr_t *nodelist, int len, int ar);
void setMFElementtype (uintptr_t num);

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
/* Routing table */
struct CRStruct *CRoutes;
static int CRoutes_Initiated = FALSE;
int CRoutes_Count;
int CRoutes_MAX;

/* Structure table */
struct CRscriptStruct *ScriptControl = 0; 	/* global objects and contexts for each script */
uintptr_t *scr_act = 0;			/* this script has been sent an eventIn */
int max_script_found = -1;	/* the maximum script number found */

/* Script name/type table */
struct CRjsnameStruct *JSparamnames = 0;
int jsnameindex = -1;
int MAXJSparamNames = 0;

/* EAI needs the extra parameter, so we put it globally when a RegisteredListener is clicked. */
int CRoutesExtra = 0;

/* global return value for getting the value of a variable within Javascript */
jsval global_return_val;
jsval JSglobal_return_val;
uintptr_t *JSSFpointer;

/* ClockTick structure for processing all of the initevents - eg, TimeSensors */
struct FirstStruct {
	void *	tonode;
	void (*interpptr)(void *);
};

/* ClockTick structure and counter */
struct FirstStruct *ClockEvents = 0;
int num_ClockEvents = 0;



/* a Script (JavaScript or CLASS) has given us an event, tell the system of this */
/* tell this node now needs to redraw  - but only if it is not a script to
   script route - see CRoutes_Register here, and check for the MALLOC in that code.
   You should see that the offset is zero, while in real nodes, the offset of user
   accessible fields is NEVER zero - check out CFuncs/Structs.h and look at any of
   the node types, eg, X3D_IndexedFaceSet  the first offset is for X3D_Virt :=)
*/

void markScriptResults(void * tn, int tptr, int route, void * tonode) {
	if (tptr != 0) {
		#ifdef CRVERBOSE
		printf ("markScriptResults: can update this node %d %d\n",tn,tptr); 
		#endif
		update_node(tn);
	#ifdef CRVERBOSE
	} else {
		printf ("markScriptResults: skipping this node %d %d flag %d\n",tn,tptr,CRoutes[route].direction_flag); 
	#endif
	}

	mark_event (CRoutes[route].fromnode,CRoutes[route].fnptr);

	/* run an interpolator, if one is attached. */
	if (CRoutes[route].interpptr != 0) {
		/* this is an interpolator, call it */
		CRoutesExtra = CRoutes[route].extra; /* in case the interp requires it... */
		#ifdef CRVERBOSE 
		printf ("script propagate_events. index %d is an interpolator\n",route);
		#endif
		CRoutes[route].interpptr(tonode);
	}
}


/* call initialize on this script. called for script eventins and eventouts */
void initializeScript(uintptr_t num,int evIn) {
	jsval retval;
	int counter;
	uintptr_t tn;
	CRnodeStruct *to_ptr = NULL;


	/* printf ("initializeScript script, table element %d evin %d\n",num,evIn); */

	/* is this an event in? If so, num is a routing table entry */
	if (evIn) {
	    for (counter = 0; counter < CRoutes[num].tonode_count; counter++) {
		to_ptr = &(CRoutes[num].tonodes[counter]);
		tn = (uintptr_t) to_ptr->node;
		/* printf ("initializeScript, tn %d\n",tn); */

		if (!(ScriptControl[tn]._initialized)) {
                                        ActualrunScript(tn, "initialize()" ,&retval);
                                        ScriptControl[tn]._initialized=TRUE;
		}
	    }
	} else {
		/* bounds check */
		if ((num <0) || (num>max_script_found)) return;

		/* this script initialized yet? */
		if (!(ScriptControl[num]._initialized)) {
                                        ActualrunScript(num, "initialize()" ,&retval);
                                        ScriptControl[num]._initialized=TRUE;
		}
	}
}

/********************************************************************************/
/*									    	*/
/* get_touched_flag - see if this variable (can be a sub-field; see tests   	*/
/* 8.wrl for the DEF PI PositionInterpolator). return true if variable is   	*/
/* touched, and pointer to touched value is in global variable              	*/
/* JSglobal_return_val, AND possibly:						*/
/*	uintptr_t *JSSFpointer for SF non-ECMA nodes.				*/
/* 										*/
/* the way touched, and, the actual values work is as follows:			*/
/*										*/
/* keep track of the old value in a table, and compare:				*/
/* FIELDTYPE_SFInt32								*/
/* FIELDTYPE_SFBool								*/
/* FIELDTYPE_SFFloat								*/
/* FIELDTYPE_SFTime								*/
/*										*/
/* check the "touched" flag for non-zero in the private area:			*/
/* FIELDTYPE_SFRotation								*/
/* FIELDTYPE_SFNode								*/
/* FIELDTYPE_SFVec2f								*/
/* FIELDTYPE_SFVec3f								*/
/* FIELDTYPE_SFImage								*/
/* FIELDTYPE_SFColor								*/
/* FIELDTYPE_SFColorRGBA							*/
/*										*/
/* go through all elements, and find if at least one SF has been touched:	*/
/* FIELDTYPE_MFRotation								*/
/* FIELDTYPE_MFNode								*/
/* FIELDTYPE_MFVec2f								*/
/* FIELDTYPE_MFVec3f								*/
/* FIELDTYPE_MFColor								*/
/* FIELDTYPE_MFColorRGBA							*/

/* FIELDTYPE_SFString	*/
/* FIELDTYPE_MFString	*/

/* still to do:	*/
/* FIELDTYPE_MFFloat	*/
/* FIELDTYPE_MFBool	*/
/* FIELDTYPE_MFInt32	*/
/* FIELDTYPE_MFTime	*/
/* FIELDTYPE_FreeWRLPTR	*/
/*                                                                          */
/****************************************************************************/
struct ECMAValueStruct {
	jsval	JS_address;
	int	touched;
	int 	oldInt32;
	JSString *oldString;
	double	oldFloat;
	double	oldTime;
	int 	oldBool;
};

struct ECMAValueStruct ECMAValues[10];
int maxECMAVal = 0;


int get_touched_flag (uintptr_t fptr, uintptr_t actualscript) {
	JSContext *cx;
	jsval interpobj;
	char *fullname;
	jsval retval;
	int touched;

	touched = FALSE;
	interpobj = ScriptControl[actualscript].glob;
	cx = (JSContext *) ScriptControl[actualscript].cx;
	fullname = JSparamnames[fptr].name;

	if (!JS_GetProperty(cx, (JSObject *) interpobj ,fullname,&retval)) {
               	printf ("cant get property for %s\n",fullname);
		return FALSE;
        } else {
		JSglobal_return_val = retval; /* save this for extraction of values, and for resetting touched pointers */
		/* ADD_ROOT (cx,JSglobal_return_val); */

		#ifdef CRVERBOSE
		printf("get_touched_flag: node type: %s\n",FIELDTYPES[JSparamnames[fptr].type]);
		#endif

		switch (JSparamnames[fptr].type) {
			GETJSPTR_TYPE_A(SFRotation)
			GETJSPTR_TYPE_A(SFNode)
			GETJSPTR_TYPE_A(SFVec2f)
			GETJSPTR_TYPE_A(SFVec3f)
			GETJSPTR_TYPE_A(SFImage)
			GETJSPTR_TYPE_A(SFColor)
			GETJSPTR_TYPE_A(SFColorRGBA)

			GETJSPTR_TYPE_MF_A(MFRotation,SFRotation)
			GETJSPTR_TYPE_MF_A(MFNode,SFNode)
			GETJSPTR_TYPE_MF_A(MFVec2f,SFVec2f)
			GETJSPTR_TYPE_MF_A(MFVec3f,SFVec3f)
			/* GETJSPTR_TYPE_MF_A(MFImage,SFImage) */
			GETJSPTR_TYPE_MF_A(MFColor,SFColor)
			GETJSPTR_TYPE_MF_A(MFColorRGBA,SFColorRGBA)
			
			GET_ECMA_MF_TOUCHED(Int32,int) /* note ,no SF for these ones */
			GET_ECMA_MF_TOUCHED(Bool,int) /* note ,no SF for these ones */
			GET_ECMA_MF_TOUCHED(Time,double) /* note ,no SF for these ones */
			GET_ECMA_MF_TOUCHED(Float,double) /* note ,no SF for these ones */

			GET_ECMA_TOUCHED(Int32,int) /* note ,no SF for these ones */
			GET_ECMA_TOUCHED(Bool,int) /* note ,no SF for these ones */
			GET_ECMA_TOUCHED(Float,double) /* note ,no SF for these ones */
			GET_ECMA_TOUCHED(Time,double) /* note ,no SF for these ones */
			GET_ECMA_TOUCHED_String 

			
			default: {printf ("not handled yet in get_touched_flag %s\n",FIELDTYPES[JSparamnames[fptr].type]);
			}
		}
	}
	return touched;
}

#ifdef OLDGETTOUCHED
int old_get_touched_flag (uintptr_t fptr, uintptr_t actualscript) {
	JSContext *mycx;
	char fullname[100];
	char tmethod[100];
	jsval v, retval, retval2;
	jsval interpobj;
        JSString *strval; /* strings */
	char *strtouched;
	int intval = 0;
	int touched_Multi;
	int touched_function;
	int touched;

	int index, locindex;
	int len;
	int complex_name; /* a name with a period in it */
	char *myname;

	/* used for finding touched flag in multi nodes */
	jsval vp;
	jsval tval;
	jsint jlen;
	jsval _length_val;
	int count;

	/* first - lets see if we can use a newer way of doing this */
	switch (JSparamnames[fptr].type) {
	/* possible field types 
FIELDTYPE_SFFloat
FIELDTYPE_MFFloat
FIELDTYPE_MFRotation
FIELDTYPE_MFVec3f
FIELDTYPE_SFBool
FIELDTYPE_MFBool
FIELDTYPE_SFInt32
FIELDTYPE_MFInt32
FIELDTYPE_MFNode
FIELDTYPE_MFColor
FIELDTYPE_MFColorRGBA
FIELDTYPE_SFTime
FIELDTYPE_MFTime
FIELDTYPE_SFString
FIELDTYPE_MFString
FIELDTYPE_MFVec2f
FIELDTYPE_FreeWRLPTR

		case FIELDTYPE_SFRotation: 
		case FIELDTYPE_SFNode:
		case FIELDTYPE_SFVec2f:
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFImage:
		case FIELDTYPE_SFColor:
		case FIELDTYPE_SFColorRGBA:

			return new_get_touched_flag(fptr,actualscript);
		default: {}
	}

	*/
	mycx = (JSContext *) ScriptControl[actualscript].cx;
	myname = JSparamnames[fptr].name;
	#ifdef CRVERBOSE 
		printf ("\nget_touched_flag, name %s script %d context %#x \n",myname,
				actualscript,mycx);
	#endif

	/* should never get into this if */
	if (ScriptControl[actualscript].thisScriptType != JAVASCRIPT) {
		printf ("gettouched, not a javascript\n");
		return 0;
	}

	len = strlen(myname);
	index = 0;
	interpobj = ScriptControl[actualscript].glob;
	complex_name = (strstr(myname,".") != NULL);
	fullname[0] = 0;


	/* if this is a complex name (ie, it is like a field of a field) then get the
	 first part. */
	if (complex_name) {
		/* get first part, and convert it into a handle name. */
		locindex = 0;
		while (*myname!='.') {
			tmethod[locindex] = *myname;
			locindex++;
			myname++;
		}
		tmethod[locindex] = 0;
		myname++;

		/* printf ("getting intermediate value by using %s\n",tmethod); */
		 if (!JS_GetProperty(mycx, (JSObject *) interpobj,tmethod,&retval)) {
			printf ("cant get property for name %s\n",tmethod);
			return FALSE;
		} else {
               		strval = JS_ValueToString(mycx, retval);
                	strtouched = JS_GetStringBytes(strval);
                	/* printf ("interpobj %d and getproperty returns %s\n",retval,strtouched); */
		}
		strcpy (fullname,strtouched);
		strcat (fullname,"_");
	}

	/* now construct the varable name; it might have a prefix as found above. */

	/* printf ("get_touched_flag, before constructor, fullname is %s\n",fullname); */
	strcat (fullname,myname);
	touched_function = FALSE;
	touched_Multi = FALSE;



	/* Find out the method of getting the touched flag from this variable type */

	/* Multi types */
	switch (JSparamnames[fptr].type) {
	case FIELDTYPE_MFFloat:
	case FIELDTYPE_MFTime:
	case FIELDTYPE_MFInt32:
	case FIELDTYPE_MFString:
		strcpy (tmethod,"__touched_flag");
		complex_name = TRUE;
		break;

	case FIELDTYPE_MFColor:
	case FIELDTYPE_MFRotation:
	case FIELDTYPE_MFNode:
	case FIELDTYPE_MFVec2f:
	case FIELDTYPE_MFVec3f:
		strcpy (tmethod,"__touched_flag");
		touched_Multi = TRUE;
		complex_name = TRUE;
		break;

	/* ECMAScriptNative types */
	case FIELDTYPE_SFBool:
	case FIELDTYPE_SFFloat:
	case FIELDTYPE_SFTime:
	case FIELDTYPE_SFInt32:
	case FIELDTYPE_SFString:
	case FIELDTYPE_SFImage:
		if (complex_name) strcpy (tmethod,"_touched");
		else sprintf (tmethod, "_%s_touched",fullname);
		break;

	case FIELDTYPE_SFColor:
	case FIELDTYPE_SFVec3f:	
	case FIELDTYPE_SFNode:
	case FIELDTYPE_SFRotation:
	case FIELDTYPE_SFVec2f:
		if (complex_name) strcpy (tmethod,"__touched()");
		else sprintf (tmethod, "%s.__touched()",fullname);
		touched_function = TRUE;
		break;
	default: {
		printf ("WARNING, this type (%d) not handled yet\n",
			JSparamnames[fptr].type);
		return FALSE;
		}
	}

	/* get the property value, if we can */
	#ifdef CRVERBOSE 
	printf ("getting property for fullname %s\n",fullname);
	#endif
	if (!JS_GetProperty(mycx, (JSObject *) interpobj ,fullname,&retval)) {
               	printf ("cant get property for %s\n",fullname);
		return FALSE;
        } else {
		ADD_ROOT(mycx,retval);
	printf ("got property for fullname %s\n",fullname);
       	        strval = JS_ValueToString(mycx, retval);
	printf ("got strval for fullname %s\n",fullname);
		if (strval == NULL) {
			/* there was a javascript error here */
			printf ("possible javascript error found in get_touched while running routes\n");
			return FALSE;
		}
               	strtouched = JS_GetStringBytes(strval);
			printf ("and get of actual property %d returns %s\n",retval,strtouched); 
               	#ifdef CRVERBOSE  
			printf ("and get of actual property %d returns %s\n",retval,strtouched); 
		#endif

		/* this can be undefined, as the associated route is created if there is a DEF
		 node in the parameter list, and the function does not touch this node/field.
		 if this is the case, just ignore it. */
		if (strcmp("undefined",strtouched)==0) {
			#ifdef CRVERBOSE 
				printf ("abnormal return here\n");
			#endif
			return FALSE;
		}

		/* Save this value for later parsing */
		global_return_val = retval;
	}


	/*  Now, for the Touched (and thus the return) value */
	if (touched_function) {
		#ifdef CRVERBOSE 
			printf ("Function, have to run script\n"); 
		#endif

		if (!ActualrunScript(actualscript, tmethod ,&retval))
			printf ("failed to get touched, line %s\n",tmethod);

		if (JSVAL_IS_INT(retval)) {
			intval = JSVAL_TO_INT(retval);
			return (intval!=0);
		}
		return FALSE; /* should never get here */
	}


	/*  now, if this is a complex name, we get property relative to what was before;
	 if not (ie, this is a standard, simple, name, use the object as before */
	if (complex_name) {
		interpobj = retval;
	}

	/* Multi types, go through each element, and find the touched flag. grep for
	 touched in CFuncs/jsVRMLClasses.c to see what we are really trying to find.
	 We can also look at the base node; it *may* have a touched flag. */

	if (touched_Multi) {

		touched = 0;

		/* lets try the MF touched flag. If this is 0, then go
		 through each element. */
		if (!JS_GetProperty(mycx, (JSObject *) interpobj, "__touched_flag", &_length_val)) {
				fprintf(stderr, "JS_GetProperty failed for \"__touched_flag\" in here.\n");
				            return JS_FALSE;
            	}


		jlen = JSVAL_TO_INT(_length_val);
		#ifdef CRVERBOSE 
			printf ("touched_Multi: jlen %d\n",jlen);
		#endif
		if (jlen>0) {
			/* yeah! We don't have to go through each element!
			set it to "0" for next time. */
			v = INT_TO_JSVAL(0);
			JS_SetProperty (mycx, (JSObject *)  interpobj,
					"__touched_flag", &v);
			return TRUE;
		}


		if (!JS_GetProperty(mycx, (JSObject *) interpobj, "length", &_length_val)) {
				printf("JS_GetProperty failed for \"length\" in here.\n");
				            return JS_FALSE;
            	}
		jlen = JSVAL_TO_INT(_length_val);
		#ifdef CRVERBOSE 
		printf ("length of object %d is %d\n",interpobj,jlen); 
		#endif



		/* go through each element of the MF* and look for the touched flag. */
		for (count = 0; count < jlen; count ++) {
			if (!JS_GetElement(mycx, (JSObject *) interpobj,
				count, &vp)) { printf ("cant get element %d\n",count);
			} else {
				#ifdef CRVERBOSE 
				printf ("first element %d is %d\n",count,vp); 
 				#endif
				switch (JSparamnames[fptr].type) {
				  case FIELDTYPE_MFVec3f:
				case FIELDTYPE_MFColor:
					if (!(SFColorTouched( mycx, (JSObject *)vp, 0, 0, &tval)))
						printf ("cant get touched for MFColor/MFVec3f\n");

				     	break;
				  case FIELDTYPE_MFRotation: {
					if (!(SFRotationTouched( mycx, (JSObject *)vp, 0, 0, &tval)))
						printf ("cant get touched for MFRotation\n");
				     	break;
					}
				  case FIELDTYPE_MFNode: {
					if (!(SFNodeTouched( mycx, (JSObject *)vp, 0, 0, &tval)))
						printf ("cant get touched for MFNode\n");
				     	break;
					}
				  case FIELDTYPE_MFVec2f: {
					if (!(SFVec2fTouched( mycx, (JSObject *)vp, 0, 0, &tval)))
						printf ("cant get touched for MFVec2f\n");
				     	break;
					}
				  default: printf ("WARNING, touched touched_Multi case problem for scripts\n");
				}
				touched += JSVAL_TO_INT(tval);
				#ifdef CRVERBOSE 
				printf ("touched for %d is %d\n",count, JSVAL_TO_INT(tval)); 
				#endif
			}
		}
		return (touched != 0);
	}

	#ifdef CRVERBOSE 
	printf ("using touched method %s on %d %d\n",tmethod,ScriptControl[actualscript].cx,interpobj); 
	#endif

	if (!JS_GetProperty(mycx, (JSObject *) interpobj ,tmethod,&retval2)) {
              	printf ("cant get property for %s\n",tmethod);
		return FALSE;
        } else {
		 
       	        strval = JS_ValueToString((JSContext *)ScriptControl[actualscript].cx, retval2);
               	strtouched = JS_GetStringBytes(strval);
               	#ifdef CRVERBOSE 
		printf ("and getproperty 3 %d returns %s\n",retval2,strtouched);
		#endif
		

		if (JSVAL_IS_INT(retval2)) {
			intval = JSVAL_TO_INT(retval2);
		}

		#ifdef CRVERBOSE 
		printf ("and here, 3, going to compare %d to 0\n",intval);
		#endif

		/*  set it to 0 now. */
		v = INT_TO_JSVAL(0);
		JS_SetProperty (mycx, (JSObject *) interpobj, tmethod, &v);
		return (intval!=0);

	}
	return FALSE; /*  should never get here */
}
#endif

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

void AddRemoveChildren (
		struct X3D_Box *parent,
		struct Multi_Node *tn,
		uintptr_t *nodelist,
		int len,
		int ar) {
	int oldlen;
	void *newmal;
	int num_removed;
	uintptr_t *remchild;
	uintptr_t *remptr;
	uintptr_t *tmpptr;

	int counter, c2;

	/* printf ("AddRemove Children parent %d tn %d, len %d\n",parent,tn,len); */
	/* if no elements, just return */
	if (len <=0) return;
	if ((parent==0) || (tn == 0)) {
		printf ("Freewrl: AddRemoveChildren, parent and/or field NULL\n");
		return;
	}

	oldlen = tn->n;
	/* printf ("AddRemoveChildren, len %d, oldlen %d ar %d\n",len, oldlen, ar);  */

	/* to do a "set_children", we remove the children, then do an add */
	if (ar == 0) {
		#ifdef CRVERBOSE
		printf ("we have to perform a \"set_children\" on this field\n");
		# endif

		/* make it so that we have 0 children */
		tn->n=0; 
		if (oldlen > 0) FREE_IF_NZ(tn->p);

		/* now, make this into an addChildren */
		oldlen = 0;
		ar = 1;
	}

	if (ar == 1) {
		/* addChildren - now we know how many SFNodes are in this MFNode, lets MALLOC and add */

		/* first, set children to 0, in case render thread comes through here */
		tn->n = 0;

		newmal = MALLOC ((oldlen+len)*sizeof(void *));

		/* copy the old stuff over */
		if (oldlen > 0) memcpy (newmal,tn->p,oldlen*sizeof(void *));

		/* set up the C structures for this new MFNode addition */
		FREE_IF_NZ (tn->p);
		/* JAS tn->p = &newmal; */
		tn->p = newmal;

		/* copy the new stuff over - note, newmal changes 
		what it points to */

		newmal = (void *) (newmal + sizeof (void *) * oldlen);
		memcpy(newmal,nodelist,sizeof(void *) * len);

		/* tell each node in the nodelist that it has a new parent */
		for (counter = 0; counter < len; counter++) {
			add_parent((void *)nodelist[counter],(void *)parent);
		}

		/* and, set the new length */
		tn->n = len+oldlen;

	} else {
		/* this is a removeChildren */

		/* go through the original array, and "zero" out children that match one of
		   the parameters */

		num_removed = 0;
		remchild = nodelist;
		for (c2 = 0; c2 < len; c2++) {
			remptr = (uintptr_t*) tn->p;
			for (counter = 0; counter < tn->n; counter ++) {
				/* printf ("remove, comparing %d with %d\n",*remptr, *remchild); */
				if (*remptr == *remchild) {
					*remptr = 0;  /* "0" can not be a valid memory address */
					num_removed ++;
				}
				remptr ++;
			}
			remchild ++;
		}

		/* printf ("end of finding, num_removed is %d\n",num_removed);  */

		if (num_removed > 0) {
			/* printf ("MALLOCing size of %d\n",(oldlen-num_removed)*sizeof(void *)); */
			newmal = MALLOC ((oldlen-num_removed)*sizeof(void *));
			tmpptr = newmal;
			remptr = (uintptr_t*) tn->p;

			/* go through and copy over anything that is not zero */
			for (counter = 0; counter < tn->n; counter ++) {
				/* printf ("count %d is %d\n",counter, *remptr); */
				if (*remptr != 0) {
					*tmpptr = *remptr;
					/* printf ("now, tmpptr %d is %d\n",tmpptr,*tmpptr); */
					remove_parent((void *)*remptr,(void *)tn);
					tmpptr ++;
				}
				remptr ++;
			}
			/* printf ("done loops, now make data active \n"); */

			/* now, do the move of data */
			tn->n = 0;
			FREE_IF_NZ (tn->p);
			tn->p = newmal;
			tn->n = oldlen - num_removed;
		}
	}
}

/****************************************************************/
/* a CLASS is returning a Multi-number type; copy this from 	*/
/* the CLASS to the data structure within the freewrl C side	*/
/* of things.							*/
/*								*/
/* note - this cheats in that the code assumes that it is 	*/
/* a series of Multi_Vec3f's while in reality the structure	*/
/* of the multi structures is the same - so we "fudge" things	*/
/* to make this multi-purpose.					*/
/* eletype switches depending on:				*/
/* what the sub clen does in VRMLFields.pm;			*/
/*  "String" {return -13;} 					*/
/*  "Float" {return -14;}        				*/
/*  "Rotation" {return -15;}     				*/
/*  "Int32" {return -16;}        				*/
/*  "Color" {return -1;}        				*/
/*  "Vec2f" {return -18;}        				*/
/*  "Vec3f" {return -19;}         				*/
/*  "Node" {return -10;}         				*/
/****************************************************************/

void getCLASSMultNumType (char *buf, int bufSize,
			  struct Multi_Vec3f *tn,
			  struct X3D_Box *parent,
			  int eletype, int addChild) {
	int len;
	int elesize;




	/* get size of each element, used for MALLOCing memory */
	switch (eletype) {
	  case -13: elesize = sizeof (char); break;	/* string   */
	  case -14:
	  case FIELDTYPE_MFFloat:
	    elesize = sizeof (float); break;	        /* Float    */
	  case -15: elesize = sizeof(float)*4; break;	/* Rotation */
	  case -16: elesize = sizeof(int); break;	/* Integer  */

	  case -1:
	  case -17:
	  case -19:
	    elesize = sizeof(float)*3;
	    break;	/* SFColor, SFVec3f */
	  case -18: elesize = sizeof(float)*2; break;	/* SFVec2f */
		case -10: elesize = sizeof(int); break;
		default: {printf ("getCLASSMulNumType - unknown type %d\n",eletype); return;}
	}

	len = bufSize / elesize;  /* convert Bytes into whatever */

	#ifdef CRVERBOSE
		printf("getCLASSMultNumType: bufSize:%d, eletype:%d, allocated: %d, elesize: %d.\n",
	       bufSize,eletype, tn->n, elesize);
	#endif

	/* now, we either replace the whole data, or we add or remove it if
	 * this is a Node type. (eg, add/remove child) */

	if (eletype != -10) {
		/* do we have to REALLOC memory? */
		if (len != tn->n) {
			/* yep... */
		        /* printf ("old pointer %d\n",tn->p); */
			tn->n = 0;	/* gets around possible mem problem */
			FREE_IF_NZ (tn->p);
			tn->p =(struct SFColor *)MALLOC ((unsigned)(elesize*len));
		}

		/* copy memory over */
		memcpy (tn->p, buf, bufSize);

		/* and, tell the scene graph how many elements there are in here */
		tn->n = len;
	} else {
		/* this is a Node type, so we need to add/remove children */
		AddRemoveChildren (parent, (struct Multi_Node*)tn, (uintptr_t*)buf, len, addChild);
	}
}



/* These events must be run first during the event loop, as they start an event cascade.
   Regsister them with add_first, then call them during the event loop with do_first.    */

void kill_clockEvents() { 
	/* printf ("killing clckevents - was %d\n",num_ClockEvents); */
	num_ClockEvents = 0;
}

void add_first(void * node) {
	void (*myp)(void *);
	struct X3D_Box * tmp;
	int clocktype;
	int count;
	
	if (node == 0) {
		printf ("error in add_first; somehow the node datastructure is zero \n");
		return;
	}

	tmp = (struct X3D_Box*) node;
	clocktype = tmp->_nodeType;
	/* printf ("add_first for %s\n",stringNodeType(clocktype)); */

	if (NODE_TimeSensor == clocktype) { myp =  do_TimeSensorTick;
	} else if (NODE_ProximitySensor == clocktype) { myp = do_ProximitySensorTick;
	} else if (NODE_Collision == clocktype) { myp = do_CollisionTick;
	} else if (NODE_MovieTexture == clocktype) { myp = do_MovieTextureTick;
	} else if (NODE_AudioClip == clocktype) { myp = do_AudioTick;
	} else if (NODE_VisibilitySensor == clocktype) { myp = do_VisibilitySensorTick;
	} else if (NODE_MovieTexture == clocktype) { myp = do_MovieTextureTick;

	} else {
		/* printf ("this is not a type we need to add_first for %s\n",stringNodeType(clocktype)); */
		return;
	}

	ClockEvents = (struct FirstStruct *)REALLOC(ClockEvents,sizeof (struct FirstStruct) * (num_ClockEvents+1));
	if (ClockEvents == 0) {
		printf ("can not allocate memory for add_first call\n");
		num_ClockEvents = 0;
	}

	/* does this event exist? */
	for (count=0; count <num_ClockEvents; count ++) {
		if (ClockEvents[count].tonode == node) {
			/* printf ("add_first, already have %d\n",node); */
			return;
		}	
	}


	/* now, put the function pointer and data pointer into the structure entry */
	ClockEvents[num_ClockEvents].interpptr = myp;
	ClockEvents[num_ClockEvents].tonode = node;

	num_ClockEvents++;
}



/*******************************************************************

CRoutes_js_new;

Register a new script for future routing

********************************************************************/

void CRoutes_js_new (uintptr_t num, int scriptType) {

	/* printf ("start of CRoutes_js_new, ScriptControl %d\n",ScriptControl);  */

	/* record whether this is a javascript, class invocation, ... */
	ScriptControl[num].thisScriptType = scriptType;

	/* if it is a script (class or javascript), make sure we know that it is not
	 * initialized yet; because of threading, we have to wait until
	 * the creating (perl) function is finished, otherwise a
	 * potential deadlock situation occurs, if the initialize
	 * tries to get something via perl...
	 */

	ScriptControl[num]._initialized = FALSE;

	/* compare with a intptr_t, because we need to compare to -1 */
	if ((intptr_t)num > max_script_found) max_script_found = (intptr_t)num;
	/* printf ("returning from CRoutes_js_new - num %d max_script_found %d\n",num,max_script_found); */

}


/********************************************************************

JSparamIndex.

stores ascii names with types (see code for type equivalences).

********************************************************************/

int JSparamIndex (char *name, char *type) {
	unsigned len;
	int ty;
	int ctr;

	/*
	printf ("start of JSparamIndex, name %s, type %s\n",name,type);
	printf ("start of JSparamIndex, lengths name %d, type %d\n",
			strlen(name),strlen(type)); 
	*/


	ty = findFieldInFIELDTYPES(type);

	/* printf ("JSParamIndex, type %d, %s\n",ty,type);  */
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
	if (jsnameindex >= MAXJSparamNames) {
		/* oooh! not enough room at the table */
		MAXJSparamNames += 100; /* arbitrary number */
		JSparamnames = (struct CRjsnameStruct*)REALLOC (JSparamnames, sizeof(*JSparamnames) * MAXJSparamNames);
	}

	if (len > MAXJSVARIABLELENGTH-2) len = MAXJSVARIABLELENGTH-2;	/* concatenate names to this length */
	strncpy (JSparamnames[jsnameindex].name,name,len);
	JSparamnames[jsnameindex].name[len] = 0; /* make sure terminated */
	JSparamnames[jsnameindex].type = ty;
	/* printf ("JSparamNameIndex, returning %d\n",jsnameindex);  */
	return jsnameindex;
}

/********************************************************************

Register a route, but with fewer and more expressive parameters than
CRoutes_Register.  Currently a wrapper around that other function.

********************************************************************/

void CRoutes_RegisterSimple(
	struct X3D_Node* from, int fromOfs,
	struct X3D_Node* to, int toOfs,
	int len, int dir)
{
 /* 10+1+3+1=15:  Number <5000000000, :, number <999, \0 */
 char tonode_str[15];
 void* interpolatorPointer;
 snprintf(tonode_str, 15, "%lu:%d", to, toOfs);

 /* When routing to a script, to is not a node pointer! */
 if(dir!=SCRIPT_TO_SCRIPT && dir!=TO_SCRIPT)
  interpolatorPointer=returnInterpolatorPointer(stringNodeType(to->_nodeType));
 else
  interpolatorPointer=NULL;

 CRoutes_Register(1, from, fromOfs, 1, tonode_str, len, 
  interpolatorPointer, dir, 0);
}
 

/********************************************************************

CRoutes_Register.

Register a route in the routing table.

********************************************************************/

void CRoutes_Register(
		int adrem,
		void *from,
		int fromoffset,
		unsigned int to_count,
		char *tonode_str,
		int length,
		void *intptr,
		int scrdir,
		int extra) {
	int insert_here, shifter;
	char *buffer;
	const char *token = " ";
	CRnodeStruct *to_ptr = NULL;
	unsigned int to_counter;
	struct Multi_Node *Mchptr;
	void * chptr;
	int rv;				/* temp for sscanf rets */

	char buf[20];
	long unsigned int toof;		/* used to help determine duplicate routes */
	long unsigned int toN;

	/* is this a script to script route??? */
	/* if so, we need an intermediate location for memory, as the values must
	   be placed somewhere FROM the script node, to be found when sending TO
	   the other script */
	if (scrdir == SCRIPT_TO_SCRIPT) {
		if (length <= 0) {
			/* this is of an unknown length - most likely a MF* field */

			/* So, this is a Multi_Node, MALLOC it... */
			chptr = MALLOC (sizeof(struct Multi_Node));
			Mchptr = (struct Multi_Node *)chptr; 

			#ifdef CRVERBOSE 
				printf ("hmmm - script to script, len %d ptr %d %x\n",
				length,chptr,chptr);
			#endif

			Mchptr->n = 0; /* make it 0 nodes long */
			Mchptr->p = 0; /* it has no memory MALLOCd here */
			
		} else {
			/* this is just a block of memory, eg, it will hold an "SFInt32" */
			chptr = MALLOC (sizeof (char) * length);
		}
		sprintf (buf,"%d:0",chptr);
		CRoutes_Register (adrem, from, fromoffset,1,buf, length, 0, FROM_SCRIPT, extra);
		CRoutes_Register (adrem, chptr, 0, to_count, tonode_str,length, 0, TO_SCRIPT, extra);
		return;
	}

	/* first time through, create minimum and maximum for insertion sorts */
	if (!CRoutes_Initiated) {
		/* allocate the CRoutes structure */
		CRoutes_MAX = 25; /* arbitrary number; max 25 routes to start off with */
		CRoutes = (struct CRStruct *)MALLOC (sizeof (*CRoutes) * CRoutes_MAX);

		CRoutes[0].fromnode = 0;
		CRoutes[0].fnptr = 0;
		CRoutes[0].tonode_count = 0;
		CRoutes[0].tonodes = NULL;
		CRoutes[0].isActive = FALSE;
		CRoutes[0].interpptr = 0;
		CRoutes[1].fromnode = (char *) -1;
		CRoutes[1].fnptr = 0x8FFFFFFF;
		CRoutes[1].tonode_count = 0;
		CRoutes[1].tonodes = NULL;
		CRoutes[1].isActive = FALSE;
		CRoutes[1].interpptr = 0;
		CRoutes_Count = 2;
		CRoutes_Initiated = TRUE;
	}

	#ifdef CRVERBOSE  
		printf ("\n\nCRoutes_Register adrem %d from %u off %u to %u %s len %d intptr %u\n",
				adrem, from, fromoffset, to_count, tonode_str, length, intptr);
		printf ("CRoutes_Register, CRoutes_Count is %d\n",CRoutes_Count);
	#endif

	insert_here = 1;

	/* go through the routing list, finding where to put it */
	while (from > CRoutes[insert_here].fromnode) {
		#ifdef CRVERBOSE 
			printf ("comparing %u to %u\n",from, CRoutes[insert_here].fromnode);
		#endif
		insert_here++;
	}

	/* hmmm - do we have a route from this node already? If so, go
	   through and put the offsets in order */
	while ((from == CRoutes[insert_here].fromnode) &&
		(fromoffset > CRoutes[insert_here].fnptr)) {
		#ifdef CRVERBOSE 
			printf ("same fromnode, different offset\n");
		#endif
		insert_here++;
	}


	/* Quick check to verify that we don't have a duplicate route here
	   OR to delete a route... */

	#ifdef CRVERBOSE
	printf ("ok, CRoutes_Register - is this a duplicate? comparing from (%d %d), fnptr (%d %d) intptr (%d %d) and tonodes %d\n",
		CRoutes[insert_here].fromnode, from,
		CRoutes[insert_here].fnptr, fromoffset,
		CRoutes[insert_here].interpptr, intptr,
		CRoutes[insert_here].tonodes);
	#endif

	if ((CRoutes[insert_here].fromnode==from) &&
		(CRoutes[insert_here].fnptr==(unsigned)fromoffset) &&
		(CRoutes[insert_here].interpptr==intptr) &&
		(CRoutes[insert_here].tonodes!=0)) {

		/* possible duplicate route */
		rv=sscanf (tonode_str, "%u:%u", &toN,&toof);
		/* printf ("from tonode_str %s we have %u %u\n",tonode_str, toN, toof); */

		if ((toN == ((long unsigned)(CRoutes[insert_here].tonodes)->node)) &&
			(toof == (CRoutes[insert_here].tonodes)->foffset)) {
			/* this IS a duplicate, now, what to do? */

			#ifdef CRVERBOSE
			printf ("duplicate route; maybe this is a remove? \n");
			#endif

			/* is this an add? */
			if (adrem == 1) {
				#ifdef CRVERBOSE
					printf ("definite duplicate, returning\n");
				#endif
				return;
			} else {
				/* this is a remove */

				for (shifter = insert_here; shifter < CRoutes_Count; shifter++) {
				#ifdef CRVERBOSE 
					printf ("copying from %d to %d\n",shifter, shifter-1);
				#endif
					memcpy ((void *)&CRoutes[shifter],
						(void *)&CRoutes[shifter+1],
						sizeof (struct CRStruct));
				}
				CRoutes_Count --;
				#ifdef CRVERBOSE 
					printf ("routing table now %d\n",CRoutes_Count);
					for (shifter = 0; shifter < CRoutes_Count; shifter ++) {
						printf ("%d %d %d\n",CRoutes[shifter].fromnode, CRoutes[shifter].fnptr,
							CRoutes[shifter].interpptr);
					}
				#endif

				return;
			}
		}
	}

	/* is this a removeRoute? if so, its not found, and we SHOULD return here */
	if (adrem != 1) return;
	#ifdef CRVERBOSE 
		printf ("CRoutes, inserting at %d\n",insert_here);
	#endif

	/* create the space for this entry. */
	for (shifter = CRoutes_Count; shifter > insert_here; shifter--) {
		memcpy ((void *)&CRoutes[shifter], (void *)&CRoutes[shifter-1],sizeof(struct CRStruct));
		#ifdef CRVERBOSE 
			printf ("Copying from index %d to index %d\n",shifter, shifter-1);
		#endif
	}


	/* and put it in */
	CRoutes[insert_here].fromnode = from;
	CRoutes[insert_here].fnptr = fromoffset;
	CRoutes[insert_here].isActive = FALSE;
	CRoutes[insert_here].tonode_count = 0;
	CRoutes[insert_here].tonodes = NULL;
	CRoutes[insert_here].len = length;
	CRoutes[insert_here].interpptr = (void (*)(void*))intptr;
	CRoutes[insert_here].direction_flag = scrdir;
	CRoutes[insert_here].extra = extra;

	if (to_count > 0) {
		if ((CRoutes[insert_here].tonodes =
			 (CRnodeStruct *) calloc(to_count, sizeof(CRnodeStruct))) == NULL) {
			fprintf(stderr, "CRoutes_Register: calloc failed to allocate memory.\n");
		} else {
			CRoutes[insert_here].tonode_count = to_count;
			#ifdef CRVERBOSE
				printf("CRoutes at %d to nodes: %s\n",
					   insert_here, tonode_str);
			#endif

			if ((buffer = strtok(tonode_str, token)) != NULL) {
				/* printf("\t%s\n", buffer); */
				to_ptr = &(CRoutes[insert_here].tonodes[0]);
				if (sscanf(buffer, "%u:%u",
						   &(to_ptr->node), &(to_ptr->foffset)) == 2) {
					#ifdef CRVERBOSE 
						printf("\tsscanf returned: %u, %u\n",
						  to_ptr->node, to_ptr->foffset);
					#endif
				}


				/* condition statement changed */
				buffer = strtok(NULL, token);
				for (to_counter = 1;
					 ((to_counter < to_count) && (buffer != NULL));
					 to_counter++) {
					to_ptr = &(CRoutes[insert_here].tonodes[to_counter]);
					if (sscanf(buffer, "%u:%u",
							   &(to_ptr->node), &(to_ptr->foffset)) == 2) {
						#ifdef CRVERBOSE 
							printf("\tsscanf returned: %u, %u\n",
								  to_ptr->node, to_ptr->foffset);
						#endif
					}
					buffer = strtok(NULL, token);
				}
			}
		}
	}

	/* record that we have one more route, with upper limit checking... */
	if (CRoutes_Count >= (CRoutes_MAX-2)) {
		/* printf("WARNING: expanding routing table\n"); */
		CRoutes_MAX += 50; /* arbitrary expansion number */
		CRoutes =(struct CRStruct *) REALLOC (CRoutes, sizeof (*CRoutes) * CRoutes_MAX);
	}

	CRoutes_Count ++;

	#ifdef CRVERBOSE 
		printf ("routing table now %d\n",CRoutes_Count);
		for (shifter = 0; shifter < CRoutes_Count; shifter ++) {
			printf ("%d %d %d : ",CRoutes[shifter].fromnode, CRoutes[shifter].fnptr,
				CRoutes[shifter].interpptr);
			for (insert_here = 0; insert_here < CRoutes[shifter].tonode_count; insert_here++) {
				printf (" to: %d %d",CRoutes[shifter].tonodes[insert_here].node,
							CRoutes[shifter].tonodes[insert_here].foffset);
			}
			printf ("\n");
		}
	#endif
}

/********************************************************************

mark_event - something has generated an eventOut; record the node
data structure pointer, and the offset. Mark all relevant entries
in the routing table that this node/offset triggered an event.

********************************************************************/

void mark_event (void *from, unsigned int totalptr) {
	int findit;

	if (!CRoutes_Initiated) return;  /* no routes registered yet */

	findit = 1;

	#ifdef CRVERBOSE 
		printf ("\nmark_event, from %u fromoffset %u\n", from, totalptr);
	#endif

	/* events in the routing table are sorted by fromnode. Find
	   out if we have at least one route from this node */
	while (from > CRoutes[findit].fromnode) findit ++;

	/* while we have an eventOut from this NODE/OFFSET, mark it as
	   active. If no event from this NODE/OFFSET, ignore it */
	while ((from == CRoutes[findit].fromnode) &&
		(totalptr != CRoutes[findit].fnptr)) findit ++;

	/* did we find the exact entry? */
	#ifdef CRVERBOSE 
 		printf ("ep, (%#x %#x) (%#x %#x) at %d \n",
			from,CRoutes[findit].fromnode, totalptr,
			CRoutes[findit].fnptr,findit); 
	#endif

	/* if we did, signal it to the CEvents loop  - maybe more than one ROUTE,
	   eg, a time sensor goes to multiple interpolators */
	while ((from == CRoutes[findit].fromnode) &&
		(totalptr == CRoutes[findit].fnptr)) {
		#ifdef CRVERBOSE
			printf ("found event at %d\n",findit);
		#endif
		CRoutes[findit].isActive=TRUE;
		findit ++;
	}
	#ifdef CRVERBOSE
		printf ("done mark_event\n");
	#endif
}

/********************************************************************

mark_script - indicate that this script has had an eventIn
zero_scripts - reset all script indicators

********************************************************************/
void mark_script (uintptr_t num) {

	#ifdef CRVERBOSE 
		printf ("mark_script - script %d has been invoked\n",num);
	#endif
	scr_act[num]= TRUE;
}


/********************************************************************

gatherScriptEventOuts - at least one script has been triggered; get the
eventOuts for this script

FIXME XXXXX =  can we do this without the string conversions?

********************************************************************/

void gatherScriptEventOuts(uintptr_t actualscript) {
	int route;
	unsigned int fptr;
	unsigned int tptr;
	unsigned len;
 	void * tn;
	void * fn;

	/* temp for sscanf retvals */

	int fromalready=FALSE;	 /* we have already got the from value string */
	int touched_flag=FALSE;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	/* go through all routes, looking for this script as an eventOut */

	/* do we have any routes yet? - we can gather events before any routes are made */
	if (!CRoutes_Initiated) return;

	/* this script initialized yet? */
	/* JAS - events are running already if (!isinputThreadParsing()) */
	initializeScript(actualscript, FALSE);

	/* routing table is ordered, so we can walk up to this script */
	route=1;


	#ifdef CRVERBOSE
	printf ("routing table looking, looking at %x and %x\n",(uintptr_t)(CRoutes[route].fromnode), actualscript); 
	#endif

	while ((uintptr_t)(CRoutes[route].fromnode) < actualscript) route++;
	while ((uintptr_t)(CRoutes[route].fromnode) == actualscript) {
		/* is this the same from node/field as before? */
		if ((CRoutes[route].fromnode == CRoutes[route-1].fromnode) &&
			(CRoutes[route].fnptr == CRoutes[route-1].fnptr) &&
			(route > 1)) {
			fromalready=TRUE;
		} else {
			/* printf ("different from, have to get value\n"); */
			fromalready=FALSE;
		}

		fptr = CRoutes[route].fnptr;
		fn = CRoutes[route].fromnode;
		len = CRoutes[route].len;

		#ifdef CRVERBOSE
			printf ("\ngatherSentEvents, from %s type %d len %d\n",JSparamnames[fptr].name,
				JSparamnames[fptr].type, len);
		#endif

		/* in Ayla's Perl code, the following happened:
			MF* - run __touched_flag

			SFBool, SFFloat, SFTime, SFInt32, SFString-
				this is her $ECMASCriptNative; run _name_touched
				and _name_touched=0

			else, run _name.__touched()
		*/

		/* now, set the actual properties - switch as documented above */
		if (!fromalready) {
			#ifdef CRVERBOSE 
				printf ("Not found yet, getting touched flag fptr %d script %d \n",fptr,actualscript);
			#endif
			touched_flag = get_touched_flag(fptr,actualscript);
		}

		if (touched_flag) {
			/* get some easy to use pointers */
			for (to_counter = 0; to_counter < CRoutes[route].tonode_count; to_counter++) {
				to_ptr = &(CRoutes[route].tonodes[to_counter]);
				tn = to_ptr->node;
				tptr = to_ptr->foffset;
				#ifdef CRVERBOSE 
					printf ("%s script %d VALUE CHANGED! copy value and update %d\n",JSparamnames[fptr].name,actualscript,tn);
				#endif
				/* eventOuts go to VRML data structures */
				setField_javascriptEventOut((struct X3D_Node *)tn,tptr,JSparamnames[fptr].type, len, 
					CRoutes[route].extra, ScriptControl[actualscript].cx);

				/* tell this node now needs to redraw */
				markScriptResults(tn, tptr, route, to_ptr->node);
			}
		}
		route++;

		/* unset the touched flag */
		switch (JSparamnames[fptr].type) {
			RESET_TOUCHED_TYPE_A(SFRotation)
			RESET_TOUCHED_TYPE_A(SFNode)
			RESET_TOUCHED_TYPE_A(SFVec2f)
			RESET_TOUCHED_TYPE_A(SFVec3f)
			RESET_TOUCHED_TYPE_A(SFImage)
			RESET_TOUCHED_TYPE_A(SFColor)
			RESET_TOUCHED_TYPE_A(SFColorRGBA)
			RESET_TOUCHED_TYPE_MF_A(MFRotation,SFRotation)
			RESET_TOUCHED_TYPE_MF_A(MFNode,SFNode)
			RESET_TOUCHED_TYPE_MF_A(MFVec2f,SFVec2f)
			RESET_TOUCHED_TYPE_MF_A(MFVec3f,SFVec3f)
			/* RESET_TOUCHED_TYPE_MF_A(MFImage,SFImage) */
			RESET_TOUCHED_TYPE_MF_A(MFColor,SFColor)
			RESET_TOUCHED_TYPE_MF_A(MFColorRGBA,SFColorRGBA)

			RESET_TOUCHED_TYPE_ECMA (SFInt32)
			RESET_TOUCHED_TYPE_ECMA (SFBool)
			RESET_TOUCHED_TYPE_ECMA (SFFloat)
			RESET_TOUCHED_TYPE_ECMA (SFTime)
			RESET_TOUCHED_TYPE_ECMA (SFString)
			RESET_ECMA_MF_TOUCHED(MFInt32)
			RESET_ECMA_MF_TOUCHED(MFBool) 
			RESET_ECMA_MF_TOUCHED(MFFloat) 
			RESET_ECMA_MF_TOUCHED(MFTime) 
			
			
			default: {printf ("can not reset touched_flag for %s\n",FIELDTYPES[JSparamnames[fptr].type]);
			}
		}
		
		/* REMOVE_ROOT(ScriptControl[actualscript].cx,global_return_val); */
	}

	#ifdef CRVERBOSE 
		printf ("%f finished  gatherScriptEventOuts loop\n",TickTime);
	#endif
}



void sendScriptEventIn(uintptr_t num) {
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	#ifdef CRVERBOSE
	  printf("----BEGIN-------\nsendScriptEventIn, num %d direction %d\n",num,
		CRoutes[num].direction_flag);
	#endif

	/* script value: 1: this is a from script route
			 2: this is a to script route
			 (3 = SCRIPT_TO_SCRIPT - this gets changed in to a FROM and a TO;
			 check for SCRIPT_TO_SCRIPT in this file */

	if (CRoutes[num].direction_flag == TO_SCRIPT) {
		for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
			to_ptr = &(CRoutes[num].tonodes[to_counter]);

			/* get the value from the VRML structure, in order to propagate it to a script */

			/* mark that this script has been active SCRIPTS ARE INTEGER NUMBERS */
			mark_script((uintptr_t) to_ptr->node);
			getField_ToJavascript(num,to_ptr->foffset);

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
void propagate_events() {
	int havinterp;
	int counter;
	int to_counter;
	CRnodeStruct *to_ptr = NULL;

		#ifdef CRVERBOSE
		printf ("\npropagate_events start\n");
		#endif

	do {
		havinterp=FALSE; /* assume no interpolators triggered */

		for (counter = 1; counter < CRoutes_Count-1; counter++) {
			for (to_counter = 0; to_counter < CRoutes[counter].tonode_count; to_counter++) {
				to_ptr = &(CRoutes[counter].tonodes[to_counter]);
				if (to_ptr == NULL) {
					printf("WARNING: tonode at %u is NULL in propagate_events.\n",
							to_counter);
					continue;
				}

				#ifdef CRVERBOSE
					printf("propagate_events: counter %d to_counter %u act %s from %u off %u to %u off %u oint %u dir %d\n",
						   counter, to_counter, BOOL_STRING(CRoutes[counter].act),
						   CRoutes[counter].fromnode, CRoutes[counter].fnptr,
						   to_ptr->node, to_ptr->foffset, CRoutes[counter].interpptr,
							CRoutes[counter].direction_flag);
				#endif

				if (CRoutes[counter].isActive == TRUE) {
						#ifdef CRVERBOSE
						printf("event %u %u sent something\n", CRoutes[counter].fromnode, CRoutes[counter].fnptr);
						#endif

					/* to get routing to/from exposedFields, lets
					 * mark this to/offset as an event */
					mark_event (to_ptr->node, to_ptr->foffset);

					if (CRoutes[counter].direction_flag != 0) {
						/* scripts are a bit complex, so break this out */
						sendScriptEventIn(counter);
						havinterp = TRUE;
					} else {

						/* copy the value over */
						if (CRoutes[counter].len > 0) {
						/* simple, fixed length copy */

							memcpy((void *)(to_ptr->node + to_ptr->foffset),
								   (void *)(CRoutes[counter].fromnode + CRoutes[counter].fnptr),
								   (unsigned)CRoutes[counter].len);
						} else {
							/* this is a Multi*node, do a specialized copy. eg, Tiny3D EAI test will
							   trigger this */
							#ifdef CRVERBOSE
							printf ("in croutes, mmc len is %d\n",CRoutes[counter].len);
							#endif

							Multimemcpy ((void *)(to_ptr->node + to_ptr->foffset),
								 (void *)(CRoutes[counter].fromnode + CRoutes[counter].fnptr),
								 CRoutes[counter].len);
						}

						/* is this an interpolator? if so call the code to do it */
						if (CRoutes[counter].interpptr != 0) {
							/* this is an interpolator, call it */
							havinterp = TRUE;
								#ifdef CRVERBOSE
								printf("propagate_events: index %d is an interpolator\n",
									   counter);
								#endif

							/* copy over this "extra" data, EAI "advise" calls need this */
							CRoutesExtra = CRoutes[counter].extra;
							CRoutes[counter].interpptr((void *)(to_ptr->node));
						} else {
							/* just an eventIn node. signal to the reciever to update */
							mark_event(to_ptr->node, to_ptr->foffset);

							/* make sure that this is pointing to a real node,
							 * not to a block of memory created by
							 * EAI - extra memory - if it has an offset of
							 * zero, it is most certainly made. */
							if ((to_ptr->foffset) != 0)
								update_node((void *)to_ptr->node);
						}
					}
				}
			}

			if (CRoutes[counter].isActive == TRUE) {
				/* we have this event found */
				CRoutes[counter].isActive = FALSE;
			}

		}

		/* run gatherScriptEventOuts for each active script */
		for (counter =0; counter <= max_script_found; counter++) {
/*
printf ("msf %d c %d\n",max_script_found, counter);
printf ("script type %d\n",ScriptControl[counter].thisScriptType);
*/

			gatherScriptEventOuts (counter);

		}

	} while (havinterp==TRUE);

		#ifdef CRVERBOSE
		printf ("done propagate_events\n\n");
		#endif
}


/********************************************************************

process_eventsProcessed()

According to the spec, all scripts can have an eventsProcessed
function - see section C.4.3 of the spec.

********************************************************************/
void process_eventsProcessed() {

	int counter;
	jsval retval;

	for (counter = 0; counter <= max_script_found; counter++) {
                if (!ActualrunScript(counter, "eventsProcessed()" ,&retval))
                        printf ("failed to run eventsProcessed for script %d\n",counter);
	}
}

/*******************************************************************

do_first()


Call the sensor nodes to get the results of the clock ticks; this is
the first thing in the event loop.

********************************************************************/

void do_first() {
	int counter;

	/* go through the array; add_first will NOT add a null pointer
	   to either field, so we don't need to bounds check here */

	for (counter =0; counter < num_ClockEvents; counter ++) {
		ClockEvents[counter].interpptr(ClockEvents[counter].tonode);
	}

	/* now, propagate these events */
	propagate_events();
}


/*******************************************************************

Interface to allow EAI/SAI to get routing information.

********************************************************************/

int getRoutesCount(void) {
	return CRoutes_Count;
}
/*
struct CRStruct {
	void *	fromnode;
	uintptr_t fnptr;
	unsigned int tonode_count;
	CRnodeStruct *tonodes;
	int	act;
	int	len;
	void	(*interpptr)(void *); 
	int	direction_flag;f
*/
void getSpecificRoute (int routeNo, uintptr_t *fromNode, int *fromOffset, 
		uintptr_t *toNode, int *toOffset) {
        CRnodeStruct *to_ptr = NULL;


	if ((routeNo <1) || (routeNo >= CRoutes_Count)) {
		*fromNode = 0; *fromOffset = 0; *toNode = 0; *toOffset = 0;
	}
/*
	printf ("getSpecificRoute, fromNode %d fromPtr %d tonode_count %d\n",
		CRoutes[routeNo].fromnode, CRoutes[routeNo].fnptr, CRoutes[routeNo].tonode_count);
*/
		*fromNode = CRoutes[routeNo].fromnode;
		*fromOffset = CRoutes[routeNo].fnptr;
	/* there is not a case where tonode_count != 1 for a valid route... */
	if (CRoutes[routeNo].tonode_count != 1) {
		printf ("huh? tonode count %d\n",CRoutes[routeNo].tonode_count);
		*toNode = 0; *toOffset = 0;
		return;
	}

	/* get the first toNode,toOffset */
        to_ptr = &(CRoutes[routeNo].tonodes[0]);
        *toNode = to_ptr->node;
	*toOffset = to_ptr->foffset;


	

}
/*******************************************************************

kill_routing()

Stop routing, remove structure. Used for ReplaceWorld style calls.

********************************************************************/

void kill_routing (void) {
        if (CRoutes_Initiated) {
                CRoutes_Count = 0;
                CRoutes_MAX = 0;
                FREE_IF_NZ (CRoutes);
                CRoutes_Initiated = FALSE;
        }
}


/* internal variable to copy a C structure's Multi* field */
void Multimemcpy (void *tn, void *fn, int multitype) {
	unsigned int structlen;
	unsigned int fromcount, tocount;
	void *fromptr, *toptr;

	struct Multi_Vec3f *mv3ffn, *mv3ftn;

	#ifdef CRVERBOSE 
		printf ("Multimemcpy, copying structures %d %d type %d\n",tn,fn,multitype); 
	#endif

	/* copy a complex (eg, a MF* node) node from one to the other
	   the following types are currently found in VRMLNodes.pm -

		 -1  is a Multi_Color 
		 -10 is a Multi_Node
		 -12 is a SFImage
		 -13 is a Multi_String
		 -14 is a Multi_Float
		 -15 is a Multi_Rotation
		 -16 is a Multi_Int32
		 -18 is a Multi_Vec2f
		 -19 is a Multi_Vec3f
	*/

	/* Multi_XXX nodes always consist of a count then a pointer - see
	   Structs.h */

	/* making the input pointers into a (any) structure helps deciphering params */
	mv3ffn = (struct Multi_Vec3f *)fn;
	mv3ftn = (struct Multi_Vec3f *)tn;

	/* so, get the from memory pointer, and the to memory pointer from the structs */
	fromptr = (void *)mv3ffn->p;

	/* and the from and to sizes */
	fromcount = mv3ffn->n;
	tocount = mv3ftn->n;
	#ifdef CRVERBOSE 
		printf ("Multimemcpy, fromcount %d\n",fromcount);
	#endif

	/* get the structure length */
	switch (multitype) {
		case -1: {structlen = sizeof (struct SFColor); break; }
		case -10: {structlen = sizeof (unsigned int); break; }
		case -12: {structlen = sizeof (unsigned int); break; } 
		case -13: {structlen = sizeof (unsigned int); break; }
		case -14: {structlen = sizeof (float); break; }
		case -15: {structlen = sizeof (struct SFRotation); break;}
		case -16: {structlen = sizeof (int); break;}
		case -18: {structlen = sizeof (struct SFVec2f); break;}
		case -19: {structlen = sizeof (struct SFColor); break;} /* This is actually SFVec3f - but no struct of this type */
		default: {
			/* this is MOST LIKELY for an EAI handle_Listener call 
			printf("WARNING: Multimemcpy, don't handle type %d yet\n", multitype); */
			structlen=0;
			return;
		}
	}


	/* free the old data, if there is old data... */
	FREE_IF_NZ (mv3ftn->p);

	/* MALLOC the toptr */
	mv3ftn->p = (struct SFColor *)MALLOC (structlen*fromcount);
	toptr = (void *)mv3ftn->p;

	/* tell the recipient how many elements are here */
	mv3ftn->n = fromcount;

	#ifdef CRVERBOSE 
		printf ("Multimemcpy, fromcount %d tocount %d fromptr %d toptr %d\n",fromcount,tocount,fromptr,toptr); 
	#endif

	/* and do the copy of the data */
	memcpy (toptr,fromptr,structlen * fromcount);
}

