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

#include "headers.h"

#include "jsapi.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"


#define FROM_SCRIPT 1
#define TO_SCRIPT 2
#define SCRIPT_TO_SCRIPT 3

/* scripting function protos PLACED HERE, not in headers.h,
   because these are shared only between this and JScript.c,
   and other modules dont require JavaScript headers */
void
cleanupDie(int num, char *msg);

void
setECMAtype(int num);

void
getMFStringtype(JSContext *cx, jsval *from, struct Multi_String *to);

int
get_touched_flag(int fptr, int actualscript);

void
getMultiElementtype(char *strp, struct Multi_Vec3f *tn, int eletype);

void
setMultiElementtype(int num);

void
Multimemcpy(void *tn, void *fn, int len);

void
CRoutes_Register(unsigned int from,
				 int fromoffset,
				 unsigned int to_count,
				 char *tonode_str,
				 int length,
				 void *intptr,
				 int scrdir,
				 int extra);

void
CRoutes_free(void);

void
mark_script(int num);

void
zero_scripts(void);

void
propagate_events(void);

void
sendScriptEventIn(int num);


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

typedef struct _CRnodeStruct {
	unsigned int node;
	unsigned int foffset;
} CRnodeStruct;

struct CRStruct {
	unsigned int	fromnode;
	unsigned int	fnptr;
/* 	unsigned int	tonode; */
/* 	unsigned int	tnptr; */
	unsigned int tonode_count;
	CRnodeStruct *tonodes;
	int	act;
	int	len;
	void	(*interpptr)(void *);
	int	direction_flag;	/* if non-zero indicates script in/out,
						   proto in/out */
	int	extra;		/* used to pass a parameter (eg, 1 = addChildren..) */
};

/* Routing table */
struct CRStruct *CRoutes;
static int CRoutes_Initiated = FALSE;
int CRoutes_Count;
int CRoutes_MAX;

/* Structure table */
struct CRjsStruct *JSglobs = 0; 	/* global objects and contexts for each script */
int *scr_act = 0;			/* this script has been sent an eventIn */
int scripts_active;		/* a script has been sent an eventIn */
int max_script_found = -1;	/* the maximum script number found */

/* Script name/type table */
struct CRjsnameStruct *JSparamnames = 0;
int jsnameindex = -1;
int MAXJSparamNames = 0;


int CRVerbose = 0;

/* global return value for getting the value of a variable within Javascript */
jsval global_return_val;


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
#define SFIMAGE	10

#define MFCOLOR 11
#define MFFLOAT 12
#define MFTIME 	13
#define MFINT32 14
#define MFSTRING 15
#define MFNODE	16
#define MFROTATION 17
#define MFVEC2F	18


#define FIELD_TYPE_STRING(f) ( \
	f == SFBOOL ? "SFBool" : ( \
	f == SFCOLOR ? "SFColor or SFVec3f" : ( \
	f == SFFLOAT ? "SFFloat" : ( \
	f == SFTIME ? "SFTime" : ( \
	f == SFINT32 ? "SFInt32" : ( \
	f == SFSTRING ? "SFString" : ( \
	f == SFNODE ? "SFNode" : ( \
	f == SFROTATION ? "SFRotation" : ( \
	f == SFVEC2F ? "SFVec2f" : ( \
	f == SFIMAGE ? "SFImage" : ( \
	f == MFCOLOR ? "MFColor or MFVec3f" : ( \
	f == MFFLOAT ? "MFFloat" : ( \
	f == MFTIME ? "MFTime" : ( \
	f == MFINT32 ? "MFInt32" : ( \
	f == MFSTRING ? "MFString" : ( \
	f == MFNODE ? "MFNode" : ( \
	f == MFROTATION ? "MFRotation" : ( \
	f == MFVEC2F ? "MFVec2f" : "unknown field type"))))))))))))))))))

/****************************************************************************/
/*									    */
/* get_touched_flag - see if this variable (can be a sub-field; see tests   */
/* 8.wrl for the DEF PI PositionInterpolator). return true if variable is   */
/* touched, and pointer to touched value is in global variable              */
/* global_return_val							    */
/*                                                                          */
/****************************************************************************/

int get_touched_flag (int fptr, int actualscript) {
	char fullname[100];
	char tmethod[100];
	jsval v, retval, retval2;
	jsval interpobj;
	/* jsval touchedobj; */
	/* int tn; */
        JSString *strval; /* strings */
	char *strtouched;
	int intval = 0;
	int touched_function;


	int index, locindex;
	int len;
	int complex_name; /* a name with a period in it */
	char *myname;

	if (JSVerbose) 
		printf ("\nget_touched_flag, name %s script %d context %#x \n",JSparamnames[fptr].name,
				actualscript,JSglobs[actualscript].cx);

	myname = JSparamnames[fptr].name;
	len = strlen(myname);
	index = 0;
	interpobj = JSglobs[actualscript].glob;
	complex_name = (strstr(myname,".") != NULL);
	fullname[0] = 0;


	// if this is a complex name (ie, it is like a field of a field) then get the
	// first part. 
	if (complex_name) {
		// get first part, and convert it into a handle name.
		locindex = 0;
		while (*myname!='.') {
			tmethod[locindex] = *myname;
			locindex++; 
			myname++;
		}
		tmethod[locindex] = 0;
		myname++;

		//printf ("getting intermediate value by using %s\n",tmethod);
		 if (!JS_GetProperty((JSContext *) JSglobs[actualscript].cx, (JSObject *) interpobj,tmethod,&retval)) {
			printf ("cant get property for name %s\n",tmethod);
			return FALSE;
		} else {
               		strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, retval);
                	strtouched = JS_GetStringBytes(strval);
                	//printf ("interpobj %d and getproperty returns %s\n",retval,strtouched);
		}
		strcpy (fullname,strtouched);
		strcat (fullname,"_");
	}

	// now construct the varable name; it might have a prefix as found above.

	//printf ("before constructor, fullname is %s\n",fullname);
	strcat (fullname,myname);
	touched_function = FALSE;

	// Find out the method of getting the touched flag from this variable type

	/* Multi types */
	switch (JSparamnames[fptr].type) {
	case MFFLOAT: case MFTIME: case MFINT32: case MFCOLOR:
	case MFROTATION: case MFNODE: case MFVEC2F: 
	case MFSTRING: {
		strcpy (tmethod,"__touched_flag");
		complex_name = TRUE;
		break;
		}
	
	/* ECMAScriptNative types */
	case SFBOOL: case SFFLOAT: case SFTIME: case SFINT32: case SFSTRING: {
		if (complex_name) strcpy (tmethod,"_touched");
		else sprintf (tmethod, "_%s_touched",fullname);
		break;
		}

	case SFCOLOR:
	case SFNODE: case SFROTATION: case SFVEC2F: {
		if (complex_name) strcpy (tmethod,"__touched()");
		else sprintf (tmethod, "%s.__touched()",fullname);
		touched_function = TRUE;
		break;
		}
	default: {
		printf ("WARNING, this type (%d) not handled yet\n",
			JSparamnames[fptr].type);
		return FALSE;
		}
	}

	// get the property value, if we can
	//printf ("getting property for fullname %s\n",fullname);
	if (!JS_GetProperty((JSContext *) JSglobs[actualscript].cx, (JSObject *) interpobj ,fullname,&retval)) {
               	printf ("cant get property for %s\n",fullname);
		return FALSE;
        } else {
       	        strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, retval);
               	strtouched = JS_GetStringBytes(strval);
               	//printf ("and get of actual property %d returns %s\n",retval,strtouched);

		// this can be undefined, as the associated route is created if there is a DEF
		// node in the parameter list, and the function does not touch this node/field.
		// if this is the case, just ignore it.
		if (strcmp("undefined",strtouched)==0) {
			//printf ("abnormal return here\n");
			return FALSE;
		}

		// Save this value for later parsing
		global_return_val = retval;
	}


	// Now, for the Touched (and thus the return) value 
	if (touched_function) {
		//printf ("Function, have to run script\n");
	
		if (!ActualrunScript(actualscript, tmethod ,&retval)) 
			printf ("failed to get touched, line %s\n",tmethod);

       	        //strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, retval);
               	//strtouched = JS_GetStringBytes(strval);
               	//printf ("and get touched of function %d returns %s\n",retval,strtouched);


		if (JSVAL_IS_INT(retval)) {
			intval = JSVAL_TO_INT(retval);
			return (intval!=0);
		}
		return FALSE; // should never get here
	}


	// now, if this is a complex name, we get property relative to what was before;
	// if not (ie, this is a standard, simple, name, use the object as before
	if (complex_name) {
		interpobj = retval;
	}	

	//printf ("using touched method %s on %d %d\n",tmethod,JSglobs[actualscript].cx,interpobj);

	if (!JS_GetProperty((JSContext *) JSglobs[actualscript].cx, (JSObject *) interpobj ,tmethod,&retval2)) {
               	printf ("cant get property for %s\n",tmethod);
		return FALSE;
        } else {
       	        //strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, retval2);
               	//strtouched = JS_GetStringBytes(strval);
               	//printf ("and getproperty 3 %d returns %s\n",retval2,strtouched);

		if (JSVAL_IS_INT(retval2)) {
			intval = JSVAL_TO_INT(retval2);
		}

		// set it to 0 now.
		v = INT_TO_JSVAL(0);
		JS_SetProperty ((JSContext *) JSglobs[actualscript].cx, (JSObject *) interpobj, tmethod, &v);

		return (intval!=0);

	}
	return FALSE; // should never get here
}

/* sets a SFBool, SFFloat, SFTime, SFIint32, SFString in a script */
void setECMAtype (int num) {
	char scriptline[100];
	int fn, fptr, tn, tptr;
	int len;
	jsval retval;
	float fl;
	double dl;
	int il;
	int intval = 0;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	fn = (int) CRoutes[num].fromnode;
	fptr = (int) CRoutes[num].fnptr;
/* 	tn = (int) CRoutes[num].tonode; */
/* 	tptr = (int) CRoutes[num].tnptr; */
	len = CRoutes[num].len;
	
	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		to_ptr = &(CRoutes[num].tonodes[to_counter]);
		tn = (int) to_ptr->node;
		tptr = (int) to_ptr->foffset;

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
		case SFNODE:
		case SFINT32:	{ /* SFInt32 */
			memcpy ((void *) &il,(void *)fn+fptr, len);
			sprintf (scriptline,"__tmp_arg_%s=%d",
					 JSparamnames[tptr].name,il);
			break;
		}
		default: {	printf("WARNING: SHOULD NOT BE HERE! %d\n",JSparamnames[tptr].type);
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
}


/****************************************************************/
/* a script is returning a MFString type; add this to the C	*/
/* children field						*/
/****************************************************************/

void getMFStringtype (JSContext *cx, jsval *from, struct Multi_String *to) {
	/* unsigned int newptr; */
	int oldlen, newlen;
	/* char *cptr; */
	/* void *newmal; */

	jsval _v;
	JSObject *obj;
	int i;
	char *valStr, *OldvalStr;
	SV **svptr;
	int myv;

	JSString *strval; /* strings */



	/* Multi_String def is struct Multi_String { int n; SV * *p; }; */

	/* oldlen = what was there in the first place */
	oldlen = to->n;
	svptr = to->p;
	newlen=0;

	if (!JS_ValueToObject(cx,from, &obj)) printf ("JS_ValueToObject failed in getMFStringtype\n");

	// printf ("getMFStringtype, object is %d\n",obj);
	if (!JS_GetProperty(cx, obj, "length", &_v)) {
		printf ("JS_GetProperty failed for \"length\" in getMFStringtype.\n");
        }

	newlen = JSVAL_TO_INT(_v);	

	// printf ("new len %d old len %d\n",newlen,oldlen);

	if (newlen > oldlen) {
		// printf ("MFString assignment, new string has more elements than old, cant do this yet\n");
		newlen = oldlen;
	}

	for (i = 0; i < newlen; i++) {
		// get the old string pointer
		OldvalStr = SvPV(svptr[i],PL_na);
		//printf ("old string at %d is %s len %d\n",i,OldvalStr,strlen(OldvalStr));

		// get the new string pointer
		if (!JS_GetElement(cx, obj, i, &_v)) {
			fprintf(stderr,
				"JS_GetElement failed for %d in getMFStringtype\n",i);
			return;
		}
		strval = JS_ValueToString(cx, _v);
		valStr = JS_GetStringBytes(strval);

		// printf ("new string %d is %s\n",i,valStr);

		// if the strings are different...
		if (strncmp(valStr,OldvalStr,strlen(valStr)) != 0) {
			if (OldvalStr!=NULL) free(OldvalStr);
			svptr[i]= newSVpvn(valStr,strlen(valStr));
		}
	}

	myv = INT_TO_JSVAL(1);
	if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
			"JS_SetProperty failed for \"__touched_flag\" in doMFAddProperty.\n");
	}
}


/************************************************************************/
/* a script is returning a MFNode type; add or remove this to the C	*/
/* children field							*/
/************************************************************************/

void getMFNodetype (char *strp, struct Multi_Node *par, int ar) {
	unsigned int newptr;
	int oldlen, newlen;
	char *cptr;
	void *newmal;
	unsigned int *tmpptr;

	unsigned int *remptr;
	unsigned int remchild;
	int num_removed;
	int counter;


	/*printf ("getMFNodetype, %s ar %d\n",strp,ar);
	printf ("getMFNodetype, parent %d has %d nodes currently\n",par,par->n); */

	/* oldlen = what was there in the first place */
	oldlen = par->n;
	newlen=0;

	/* this string will be in the form "[ CNode addr CNode addr....]" */
	/* count the numbers to add  or remove */
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
	cptr = strp; /* reset this pointer to the first number */

	if (ar != 0) {
		/* addChildren - now we know how many SFNodes are in this MFNode, lets malloc and add */
		newmal = malloc ((oldlen+newlen)*sizeof(unsigned int));
	
		if (newmal == 0) {
			printf ("cant malloc memory for addChildren");
			return;
		}
	
		/* copy the old stuff over */
		memcpy (newmal,par->p,oldlen*sizeof(unsigned int));
	
		/* set up the C structures for this new MFNode addition */
		free (par->p);
		par->p = newmal;
		par->n = oldlen+newlen;
	
		newmal += sizeof (unsigned int)*oldlen;
	
		while (sscanf (cptr,"%d", newmal) == 1) {
			/* skip past this number */
			while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
			while (*cptr == ' ') cptr++; /* skip spaces */
			newmal += sizeof (unsigned int);
		}

	} else {
		/* this is a removeChildren */

		/* go through the original array, and "zero" out children that match one of
		   the parameters */

		num_removed = 0;
		while (sscanf (cptr,"%d", &remchild) == 1) {
			/* skip past this number */
			while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
			while (*cptr == ' ') cptr++; /* skip spaces */

			remptr = par->p;
			for (counter = 0; counter < par->n; counter ++) {
				if (*remptr == remchild) {
					*remptr = 0;  /* "0" can not be a valid memory address */
					num_removed ++;
				}
				remptr ++;
			}
		}

		if (num_removed > 0) {
			newmal = malloc ((oldlen-num_removed)*sizeof(unsigned int));
			tmpptr = newmal;
			remptr = par->p;
			if (newmal == 0) {
				printf ("cant malloc memory for removeChildren");
				return;
			}

			/* go through and copy over anything that is not zero */
			for (counter = 0; counter < par->n; counter ++) {
				if (*remptr != 0) {
					*tmpptr = *remptr;
					tmpptr ++;
				}
				remptr ++;
			}

			free (par->p);
			par->p = newmal;
			par->n = oldlen - num_removed;
		}
	}
}


/****************************************************************/
/* a script is returning a Multi-number type; copy this from 	*/
/* the script return string to the data structure within the	*/
/* freewrl C side of things.					*/
/*								*/
/* note - this cheats in that the code assumes that it is 	*/
/* a series of Multi_Vec3f's while in reality the structure	*/
/* of the multi structures is the same - so we "fudge" things	*/
/* to make this multi-purpose.					*/
/* eletype switches depending on:				*/
/* 	0: MFINT32						*/
/* 	1: MFFLOAT						*/
/* 	2: MFVEC2F						*/
/* 	3: MFCOLOR						*/
/* 	4: MFROTATION						*/
/*	5: MFTIME						*/
/****************************************************************/

void getMultNumType (JSContext *cx, struct Multi_Vec3f *tn, int eletype) {
	float *fl;
	int *il;
	double *dl;

	float f2, f3, f4;
/* 	int shouldfind; */
	jsval mainElement/* , subElement */;
	int len/* , len2 */;
	int i/* , j */;
	JSString *_tmpStr;
	char *strp;
	int elesize;

	/* get size of each element, used for mallocing memory */
	if (eletype == 0) elesize = sizeof (int);		// integer
	else if (eletype == 5) elesize = sizeof (double);	// doubles.
	else elesize = sizeof (float)*eletype;			// 1, 2, 3 or 4 floats per element.

	/* rough check of return value */
	if (!JSVAL_IS_OBJECT(global_return_val)) {
		if (JSVerbose) printf ("getMultNumType - did not get an object\n");
		return;
	}

	//printf ("getmultielementtypestart, tn %d %#x dest has  %d size %d\n",tn,tn,eletype, elesize);

	if (!JS_GetProperty(cx, global_return_val, "length", &mainElement)) {
		printf ("JS_GetProperty failed for \"length\" in getMultNumType\n");
		return;
	}
	len = JSVAL_TO_INT(mainElement);
	//printf ("getmuiltie length of grv is %d old len is %d\n",len,tn->n);

	/* do we have to realloc memory? */
	if (len != tn->n) {
		/* yep... */
			// printf ("old pointer %d\n",tn->p);
		if (tn->p != NULL) free (tn->p);
		tn->p = malloc (elesize*len);
		if (tn->p == NULL) {
			printf ("can not malloc memory in getMultNumType\n");
			return;
		}
		tn->n = len;
	}

	/* set these three up, but we only use one of them */
	fl = (float *) tn->p;
	il = (int *) tn->p;
	dl = (double *) tn->p;

	/* go through each element of the main array. */
	for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, global_return_val, i, &mainElement)) {
			printf ("JS_GetElement failed for %d in getMultNumType\n",i);
			return;
		}

                _tmpStr = JS_ValueToString(cx, mainElement);
		strp = JS_GetStringBytes(_tmpStr);
                //printf ("sub element %d is %s as a string\n",i,strp);

		switch (eletype) {
		case 0: { sscanf(strp,"%d",il); il++; break;}
		case 1: { sscanf(strp,"%f",fl); fl++; break;}
		case 2: { sscanf (strp,"%f %f",fl,&f2);
			fl++; *fl=f2; fl++; break;}
		case 3: { sscanf (strp,"%f %f %f",fl,&f2,&f3);
			fl++; *fl=f2; fl++; *fl=f3; fl++; break;}
		case 4: { sscanf (strp,"%f %f %f %f",fl,&f2,&f3,&f4);
			fl++; *fl=f2; fl++; *fl=f3; fl++; *fl=f4; fl++; break;}
		case 5: {sscanf (strp,"%lf",dl); dl++; break;}

		default : {printf ("getMultNumType unhandled eletype: %d\n",
				eletype);
			   return;
			}
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
	int fn, fptr, tn, tptr;
	int len;
	jsval retval;
	/* float fourl[4]; */
	SFVec3fNative *_privPtr; 
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	JSContext *_context;
	JSObject *_globalObj, *_sfvec3fObj;

	fn = (int) CRoutes[num].fromnode;
	fptr = (int) CRoutes[num].fnptr;
/* 	tn = (int) CRoutes[num].tonode; */
/* 	tptr = (int) CRoutes[num].tnptr; */
	len = CRoutes[num].len;
	
	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		to_ptr = &(CRoutes[num].tonodes[to_counter]);
		tn = (int) to_ptr->node;
		tptr = (int) to_ptr->foffset;

		if (CRVerbose) {
			printf ("got a script event! index %d type %d\n",
					num, CRoutes[num].direction_flag);
			printf ("\tfrom %#x from ptr %#x\n\tto %#x toptr %#x\n",fn,fptr,tn,tptr);
			printf ("\tdata length %d\n",len);
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

		_privPtr->touched = 0;


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
}

/* internal variable to copy a C structure's Multi* field */
void Multimemcpy (void *tn, void *fn, int len) {
	struct Multi_Vec3f *mv3ffn;
	struct Multi_Vec3f *mv3ftn;


	if (len == -1) {
		/* this is a Multi_Vec3f */
		mv3ffn = fn;
		mv3ftn = tn;
/* 		if (CRVerbose) printf ("MultiMemcpy to %#x from %#x lenf %d lent %d\n",tn,fn,mv3ftn->n,mv3ftn->n); */
		if (CRVerbose) printf ("MultiMemcpy to %u from %u lenf %d lent %d\n",
							   tn, fn, mv3ftn->n, mv3ftn->n);
		memcpy (mv3ftn->p,mv3ffn->p,sizeof(struct SFColor) * mv3ftn->n);
	} else {
		printf("WARNING: Multimemcpy, don't handle type %d yet\n", len);
	}
}


/*******************************************************************

CRoutes_js_new;

Register a new script for future routing

********************************************************************/

void CRoutes_js_new (int num,unsigned int cx, unsigned int glob, unsigned int brow) {
	/* jsval retval; */
	UNUSED(cx);
	UNUSED(glob);
	UNUSED(brow);

	int count;

	/* more scripts than we can handle right now? */
	if (num >= JSMaxScript)  {
		JSMaxAlloc();
	}

	if (num > max_script_found) max_script_found = num;
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
		printf("WARNING: JSparamIndex, cant match type %s\n",type);
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
	if (jsnameindex >= MAXJSparamNames) {
		/* oooh! not enough room at the table */
		MAXJSparamNames += 100; /* arbitrary number */
		JSparamnames = realloc (JSparamnames, sizeof(*JSparamnames) * MAXJSparamNames);
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


void
/* CRoutes_Register(unsigned int from, int fromoffset, unsigned int to, int tooffset, int length, void *intptr, int scrdir, unsigned int is_count, char *is_str, int extra) */
/* CRoutes_Register(unsigned int from, int fromoffset, unsigned int to, int tooffset, int length, void *intptr, int scrdir, int extra) */

CRoutes_Register(unsigned int from, int fromoffset, unsigned int to_count, char *tonode_str,
				 int length, void *intptr, int scrdir, int extra)
{
	int insert_here, shifter, count;
	char *buffer;
	const char *token = " ";
	CRnodeStruct *to_ptr = NULL;
	unsigned int to_counter;


	/* first time through, create minimum and maximum for insertion sorts */
	if (!CRoutes_Initiated) {
		/* allocate the CRoutes structure */
		CRoutes_MAX = 25; /* arbitrary number; max 25 routes to start off with */
		CRoutes = malloc (sizeof (*CRoutes) * CRoutes_MAX);




		CRoutes[0].fromnode = 0;
		CRoutes[0].fnptr = 0;
		CRoutes[0].tonode_count = 0;
		CRoutes[0].tonodes = NULL;
		CRoutes[0].act = FALSE;
		CRoutes[0].interpptr = 0;
		CRoutes[1].fromnode = 0x8FFFFFFF;
		CRoutes[1].fnptr = 0x8FFFFFFF;
		CRoutes[1].tonode_count = 0;
		CRoutes[1].tonodes = NULL;
		CRoutes[1].act = FALSE;
		CRoutes[1].interpptr = 0;
		CRoutes_Count = 2;
		CRoutes_Initiated = TRUE;

		/* and mark all scripts inactive */
		scripts_active = FALSE;
	}

	if (CRVerbose) 
		printf ("CRoutes_Register from %u off %u to %u %s len %d intptr %u\n",
				from, fromoffset, to_count, tonode_str, length, intptr);

	insert_here = 1;

	/* go through the routing list, finding where to put it */
	while (from > CRoutes[insert_here].fromnode) {
		if (CRVerbose) printf ("comparing %u to %u\n",from, CRoutes[insert_here].fromnode);
		insert_here++; 
	}

	/* hmmm - do we have a route from this node already? If so, go
	   through and put the offsets in order */
	while ((from == CRoutes[insert_here].fromnode) &&
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
/* 	CRoutes[insert_here].tonode = to; */
/* 	CRoutes[insert_here].tnptr = tooffset;	 */
	CRoutes[insert_here].tonode_count = 0;
	CRoutes[insert_here].tonodes = NULL;
	CRoutes[insert_here].len = length;
	CRoutes[insert_here].interpptr = intptr;
	CRoutes[insert_here].direction_flag = scrdir;
	CRoutes[insert_here].extra = extra;

	if (to_count > 0) {
		if ((CRoutes[insert_here].tonodes =
			 (CRnodeStruct *) calloc(to_count, sizeof(CRnodeStruct))) == NULL) {
			fprintf(stderr, "CRoutes_Register: calloc failed to allocate memory.\n");
		} else {
			CRoutes[insert_here].tonode_count = to_count;
			if (CRVerbose)
				printf("CRoutes at %d to nodes: %s\n",
					   insert_here, tonode_str);

			if ((buffer = strtok(tonode_str, token)) != NULL) {
				/* printf("\t%s\n", buffer); */
				to_ptr = &(CRoutes[insert_here].tonodes[0]);
				if (sscanf(buffer, "%u:%u",
						   &(to_ptr->node), &(to_ptr->foffset)) == 2) {
					if (CRVerbose) printf("\tsscanf returned: %u, %u\n",
										  to_ptr->node, to_ptr->foffset);
				}


				/* condition statement changed */
				buffer = strtok(NULL, token);
				for (to_counter = 1;

					// JAS - bounds check compile failure (to_counter < to_count) && ((buffer = strtok(NULL, token)) != NULL);

					 ((to_counter < to_count) && (buffer != NULL));
					 to_counter++) {
					to_ptr = &(CRoutes[insert_here].tonodes[to_counter]);
					if (sscanf(buffer, "%u:%u",
							   &(to_ptr->node), &(to_ptr->foffset)) == 2) {
						if (CRVerbose) printf("\tsscanf returned: %u, %u\n",
											  to_ptr->node, to_ptr->foffset);
					}
					buffer = strtok(NULL, token);
				}
			}
		}
	}

	/* record that we have one more route, with upper limit checking... */
	if (CRoutes_Count >= (CRoutes_MAX-2)) {
		//printf("WARNING: expanding routing table\n");
		CRoutes_MAX += 50; /* arbitrary expansion number */
		CRoutes = realloc (CRoutes, sizeof (*CRoutes) * CRoutes_MAX);
	}


	CRoutes_Count ++;

 	/*if (CRVerbose)  
 		for (shifter = 1; shifter < (CRoutes_Count-1); shifter ++) { 
 			printf ("Route indx %d is (%#x %#x) to (%#x %#x) len %d\n", 
 			shifter, CRoutes[shifter].fromnode, 
 			CRoutes[shifter].fnptr, CRoutes[shifter].tonode, 
 			CRoutes[shifter].tnptr, CRoutes[shifter].len); 
 		} 
	*/
}

void
CRoutes_free()
{
	int i;
	for (i = 0; i < CRoutes_MAX; i++) {
/* 		if (CRoutes[i].is != NULL) { */
/* 			free(CRoutes[i].is); */
		if (CRoutes[i].tonodes != NULL) {
			free(CRoutes[i].tonodes);
		}
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

	if (CRVerbose)
		/* printf ("mark_event, from %#x fromoffset %#x\n",from,totalptr); */
		printf ("\nmark_event, from %u fromoffset %u\n", from, totalptr);

	/* events in the routing table are sorted by fromnode. Find
	   out if we have at least one route from this node */
	while (from > CRoutes[findit].fromnode) findit ++;

	/* while we have an eventOut from this NODE/OFFSET, mark it as 
	   active. If no event from this NODE/OFFSET, ignore it */
	while ((from == CRoutes[findit].fromnode) &&
		(totalptr != CRoutes[findit].fnptr)) findit ++;

	/* did we find the exact entry? */
	if (CRVerbose)
/* 		printf ("ep, (%#x %#x) (%#x %#x) at %d \n",from,CRoutes[findit].fromnode, totalptr,CRoutes[findit].fnptr,findit); */
		printf ("ep, (%u %u) (%u %u) at %d \n",from,CRoutes[findit].fromnode, totalptr,CRoutes[findit].fnptr,findit);

	/* if we did, signal it to the CEvents loop  - maybe more than one ROUTE,
	   eg, a time sensor goes to multiple interpolators */
	while ((from == CRoutes[findit].fromnode) && 
		(totalptr == CRoutes[findit].fnptr)) {
		if (CRVerbose) 
			printf ("found event at %d\n",findit);
		CRoutes[findit].act=TRUE;
		findit ++;
	}
	if (CRVerbose) 
		printf ("done mark_event\n");
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

	// JAS - now done on realloc for (count = 0; count < MAXSCRIPTS; count ++)
	// JAS - now done on realloc 	scr_act[count] = FALSE;
	scripts_active = FALSE;
}


/********************************************************************

gatherScriptEventOuts - at least one script has been triggered; get the
eventOuts for this script

FIXME XXXXX =  can we do this without the string conversions?

********************************************************************/

void gatherScriptEventOuts(int actualscript, int ignore) {
	int route;	
	/* char scriptline[100]; */
	/* jsval retval; */
	int fn, tn, fptr, tptr;
	int len;
	float fl[0];	/* return float values */
	double tval;
	int ival;
	/* jsval touched; */		/* was this really touched? */

        JSString *strval; /* strings */
        char *strp;
	/* char *strtouched; */
	int fromalready=FALSE;	 /* we have already got the from value string */
	int touched_flag;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	UNUSED(ignore);

	/* go through all routes, looking for this script as an eventOut */

	/* do we have any routes yet? - we can gather events before any routes are made */
	if (!CRoutes_Initiated) return;

	/* routing table is ordered, so we can walk up to this script */
	route=1;
	while (CRoutes[route].fromnode<actualscript) route++;
	while (CRoutes[route].fromnode == actualscript) {
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

		if (CRVerbose) 
			printf ("\ngatherSentEvents, from %s type %d len %d\n",JSparamnames[fptr].name,
				JSparamnames[fptr].type, len);	

		/* in Ayla's Perl code, the following happened:
			MF* - run __touched_flag

			SFBool, SFFloat, SFTime, SFInt32, SFString-
				this is her $ECMASCriptNative; run _name_touched
				and _name_touched=0

			else, run _name.__touched()
		*/

		/* now, set the actual properties - switch as documented above */
		if (!fromalready) {
			if (CRVerbose) printf ("Not found yet, getting touched flag\n");
			touched_flag = get_touched_flag(fptr,actualscript);

			if (touched_flag) {
				/* we did, so get the value */
				strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, global_return_val);
			        strp = JS_GetStringBytes(strval);

				if (JSVerbose) 
					printf ("retval string is %s\n",strp);
			}
		}


		if (touched_flag) {
			/* get some easy to use pointers */
			for (to_counter = 0; to_counter < CRoutes[route].tonode_count; to_counter++) {
				to_ptr = &(CRoutes[route].tonodes[to_counter]);
				tn = (int) to_ptr->node;
				tptr = (int) to_ptr->foffset;

				if (JSVerbose) printf ("VALUE CHANGED! copy value and update %d\n",tn);

				if (JSVerbose) printf (" -- string from javascript is %s\n",strp);
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
					if (!JS_ValueToNumber((JSContext *)JSglobs[actualscript].cx, 
										  global_return_val,&tval)) tval=0.0;

					//printf ("SFTime conversion numbers %f from string %s\n",tval,strp);
					//printf ("copying to %#x offset %#x len %d\n",tn, tptr,len);
					memcpy ((void *)tn+tptr, (void *)&tval,len);
					break;
				}
				case SFNODE:
				case SFINT32: {
					sscanf (strp,"%d",&ival);
					//printf ("SFInt, SFNode conversion number %d\n",ival);
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
				case MFCOLOR: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, tn+tptr,3); break;}
				case MFFLOAT: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, tn+tptr,1); break;}
				case MFROTATION: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, tn+tptr,4); break;}
				case MFVEC2F: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, tn+tptr,2); break;}
				case MFNODE: {getMFNodetype (strp,tn+tptr,CRoutes[route].extra); break;}
				case MFSTRING: {
					getMFStringtype ((JSContext *) JSglobs[actualscript].cx,
									 global_return_val,tn+tptr); 
					break;
				}

				case MFINT32: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, tn+tptr,0); break;}
				case MFTIME: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, tn+tptr,5); break;}

				default: {	printf("WARNING: unhandled from type %s\n", FIELD_TYPE_STRING(JSparamnames[fptr].type));
				printf (" -- string from javascript is %s\n",strp);
				}
				}

				/* tell this node now needs to redraw */
				update_node(tn);
				mark_event (tn,tptr);
				//mark_event (CRoutes[route].fromnode,CRoutes[route].fnptr);

				/* run an interpolator, if one is attached. */
				if (CRoutes[route].interpptr != 0) {
					/* this is an interpolator, call it */
					if (CRVerbose) printf ("script propagate_events. index %d is an interpolator\n",route);
					CRoutes[route].interpptr(to_ptr->node);
				}
			}
		}
		route++;
	}
}

/********************************************************************

sendScriptEventIn.

this sends events to scripts that have eventIns defined.

********************************************************************/

void sendScriptEventIn(int num) {
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	/* script value: 1: this is a from script route
			 2: this is a to script route
			 3: this is a from script to a script route */

	/* if (CRoutes[num].direction_flag == 2) { */
	if (CRoutes[num].direction_flag == TO_SCRIPT) {
		for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
			to_ptr = &(CRoutes[num].tonodes[to_counter]);

			/* get the value from the VRML structure, in order to propagate it to a script */

			/* mark that this script has been active */
			mark_script(to_ptr->node);

			/* set the parameter */
			/* see comments in gatherScriptEventOuts to see exact formats */

			switch (JSparamnames[to_ptr->foffset].type) {
			case SFBOOL:	
			case SFFLOAT:
			case SFTIME:
			case SFINT32:
			case SFNODE:
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
			case MFCOLOR:
			case MFFLOAT:
			case MFTIME:
			case MFINT32:
			case MFSTRING:
			case MFNODE:
			case MFROTATION: {
				printf("WARNING: entry set in sendScriptEventIn, but no code yet for type %s.\n", FIELD_TYPE_STRING(JSparamnames[to_ptr->foffset].type));
				break;
			}
			default : {
				printf("WARNING: sendScriptEventIn type %s not handled yet\n", FIELD_TYPE_STRING(JSparamnames[to_ptr->foffset].type));
			}
			}
		}
	} else {
		printf("WARNING: sendScriptEventIn, don't handle %d yet\n",CRoutes[num].direction_flag);
	}
}

/********************************************************************

propagate_events.

Go through the event table, until the table is "active free". Some
nodes have eventins/eventouts - have to do the table multiple times
in this case.

********************************************************************/
void propagate_events() {
	int counter, havinterp;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	/* int mvcompCount, mvcompSize; */
	/* struct Multi_Vec3f *mv3fptr; */

	if (CRVerbose) 
		printf ("\npropagate_events start\n");

	do {
		/* set all script flags to false - no triggers */
		zero_scripts();

		havinterp=FALSE; /* assume no interpolators triggered */

		for (counter = 1; counter < CRoutes_Count-1; counter++) {
			for (to_counter = 0; to_counter < CRoutes[counter].tonode_count; to_counter++) {
				to_ptr = &(CRoutes[counter].tonodes[to_counter]);
				if (to_ptr == NULL) {
					printf("WARNING: tonode at %u is NULL in propagate_events.\n",
							to_counter);
					continue;
				}
/* 			if (CRVerbose) printf ("propagate_events, counter %d from %#x off %#x to %#x off %#x oint %#x\n", */
/* 				counter,CRoutes[counter].fromnode,CRoutes[counter].fnptr, */
/* 				CRoutes[counter].tonode,CRoutes[counter].tnptr, */
/* 				CRoutes[counter].interpptr); */

				if (CRVerbose)
					/* printf("propagate_events: counter %d to_counter %u from %#x off %#x to %#x off %#x oint %#x\n", */
					printf("propagate_events: counter %d to_counter %u act %s from %u off %u to %u off %u oint %u\n",
						   counter, to_counter, BOOL_STRING(CRoutes[counter].act),
						   CRoutes[counter].fromnode, CRoutes[counter].fnptr,
						   to_ptr->node, to_ptr->foffset, CRoutes[counter].interpptr);

				if (CRoutes[counter].act == TRUE) {
					if (CRVerbose)
						/* printf("event %#x %#x sent something\n", CRoutes[counter].fromnode, CRoutes[counter].fnptr); */
						printf("event %u %u sent something\n", CRoutes[counter].fromnode, CRoutes[counter].fnptr);

					/* we have this event found */
/* 					CRoutes[counter].act = FALSE; */

					if (CRoutes[counter].direction_flag != 0) {
						/* scripts are a bit complex, so break this out */
						sendScriptEventIn(counter);
						//gatherScriptEventOuts (counter,TRUE);
						if (scripts_active) havinterp = TRUE;
					} else {

						/* copy the value over */
						if (CRoutes[counter].len > 0) {
						/* simple, fixed length copy */

/* 						memcpy (CRoutes[counter].tonode + CRoutes[counter].tnptr,  */
/* 							CRoutes[counter].fromnode + CRoutes[counter].fnptr, */
/* 							CRoutes[counter].len); */
							memcpy(to_ptr->node + to_ptr->foffset,
								   CRoutes[counter].fromnode + CRoutes[counter].fnptr,
								   CRoutes[counter].len);
						} else {
							/* this is a Multi*node, do a specialized copy */

/* 						Multimemcpy (CRoutes[counter].tonode + CRoutes[counter].tnptr,  */
/* 							CRoutes[counter].fromnode + CRoutes[counter].fnptr, */
/* 							CRoutes[counter].len); */
							Multimemcpy (to_ptr->node + to_ptr->foffset,
										 CRoutes[counter].fromnode + CRoutes[counter].fnptr,
										 CRoutes[counter].len);
						}

						/* is this an interpolator? if so call the code to do it */
						if (CRoutes[counter].interpptr != 0) {
							/* this is an interpolator, call it */
							havinterp = TRUE;
							if (CRVerbose)
								printf("propagate_events: index %d is an interpolator\n",
									   counter);
/* 						CRoutes[counter].interpptr(CRoutes[counter].tonode); */
							CRoutes[counter].interpptr(to_ptr->node);
						} else {	
							/* just an eventIn node. signal to the reciever to update */
/* 						update_node(CRoutes[counter].tonode); */
							update_node(to_ptr->node);
							/* try */
							mark_event(to_ptr->node, to_ptr->foffset);
/* printf("propagate_events: update_node %u\n", to_ptr->node); */
						}
					}
				}
			}

			if (CRoutes[counter].act == TRUE) {
				/* we have this event found */
				CRoutes[counter].act = FALSE;
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
