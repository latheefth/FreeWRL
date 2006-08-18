/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/********************************************************************

Javascript C language binding.

*********************************************************************/


#include <math.h>
#include "headers.h"

#include "jsapi.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"

/* #define MAX_RUNTIME_BYTES 0x100000L*/
#define MAX_RUNTIME_BYTES 0x1000000L
/* #define STACK_CHUNK_SIZE 0x2000L*/
#define STACK_CHUNK_SIZE 0x20000L


/*
 * Global JS variables (from Brendan Eichs short embedding tutorial):
 *
 * JSRuntime       - 1 runtime per process
 * JSContext       - 1 CONTEXT per thread
 * global JSObject - 1 global object per CONTEXT
 *
 * struct JSClass {
 *     char *name;
 *     uint32 flags;
 * Mandatory non-null function pointer members:
 *     JSPropertyOp addProperty;
 *     JSPropertyOp delProperty;
 *     JSPropertyOp getProperty;
 *     JSPropertyOp setProperty;
 *     JSEnumerateOp enumerate;
 *     JSResolveOp resolve;
 *     JSConvertOp convert;
 *     JSFinalizeOp finalize;
 * Optionally non-null members start here:
 *     JSGetObjectOps getObjectOps;
 *     JSCheckAccessOp checkAccess;
 *     JSNative call;
 *     JSNative construct;
 *     JSXDRObjectOp xdrObject;
 *     JSHasInstanceOp hasInstance;
 *     prword spare[2];
 * };
 *
 * global JSClass  - populated by stubs
 *
 */

static JSRuntime *runtime;
static JSClass globalClass = {
	"global",
	0,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_EnumerateStub,
	globalResolve,
	JS_ConvertStub,
	JS_FinalizeStub
};


int JSMaxScript = 0;

char *DefaultScriptMethods = "function initialize() {}; function shutdown() {}; function eventsProcessed() {}; TRUE=true; FALSE=false; function print(x) {Browser.print(x)} ";

/* housekeeping routines */
void kill_javascript(void) {
}


void cleanupDie(uintptr_t num, const char *msg) {
	kill_javascript();
	freewrlDie(msg);
}

void JSMaxAlloc() {
	/* perform some reallocs on JavaScript database stuff for interfacing */
	uintptr_t count;

	JSMaxScript += 10;
	ScriptControl = (struct CRscriptStruct*)realloc (ScriptControl, sizeof (*ScriptControl) * JSMaxScript);
	scr_act = (uintptr_t *)realloc (scr_act, sizeof (*scr_act) * JSMaxScript);

	if ((ScriptControl == NULL) || (scr_act == 0)) {
		printf ("Can not allocate memory for more script indexes\n");
		exit(1);
	}

	/* mark these scripts inactive */
	for (count=JSMaxScript-10; count<JSMaxScript; count++) {
		scr_act[count]= FALSE;
		ScriptControl[count].thisScriptType = NOSCRIPT;
	}
}



/* void JSInit(uintptr_t num, SV *script) { */
void JSInit(uintptr_t num) {
	jsval rval;
	JSContext *_context; 	/* these are set here */
	JSObject *_globalObj; 	/* these are set here */
	BrowserNative *br; 	/* these are set here */
	#ifdef JSVERBOSE 
	printf("init: script %d\n",num);
	#endif

	/* more scripts than we can handle right now? */
	if (num >= JSMaxScript)  {
		JSMaxAlloc();
	}


	runtime = JS_NewRuntime(MAX_RUNTIME_BYTES);
	if (!runtime) freewrlDie("JS_NewRuntime failed");

	#ifdef JSVERBOSE 
	printf("\tJS runtime created,\n");
	#endif


	_context = JS_NewContext(runtime, STACK_CHUNK_SIZE);
	if (!_context) freewrlDie("JS_NewContext failed");

	#ifdef JSVERBOSE 
	printf("\tJS context created,\n");
	#endif


	_globalObj = JS_NewObject(_context, &globalClass, NULL, NULL);
	if (!_globalObj) freewrlDie("JS_NewObject failed");

	#ifdef JSVERBOSE 
	printf("\tJS global object created,\n");
	#endif


	/* gets JS standard classes */
	if (!JS_InitStandardClasses(_context, _globalObj))
		freewrlDie("JS_InitStandardClasses failed");

	#ifdef JSVERBOSE 
	printf("\tJS standard classes initialized,\n");
	#endif



	/* #ifdef JSVERBOSE {*/
	/* 	reportWarningsOn();*/
	/* } else {*/
	/* 	reportWarningsOff();*/
	/* }*/

	JS_SetErrorReporter(_context, errorReporter);
	#ifdef JSVERBOSE 
	printf("\tJS errror reporter set,\n");
	#endif


	br = (BrowserNative *) JS_malloc(_context, sizeof(BrowserNative));

	/* for this script, here are the necessary data areas */
	ScriptControl[num].cx = (uintptr_t) _context;
	ScriptControl[num].glob = (uintptr_t) _globalObj;
	ScriptControl[num].brow = (uintptr_t) br;


	if (!loadVrmlClasses(_context, _globalObj))
		freewrlDie("loadVrmlClasses failed");

	#ifdef JSVERBOSE 
	printf("\tVRML classes loaded,\n");
	#endif



	if (!VrmlBrowserInit(_context, _globalObj, br))
		freewrlDie("VrmlBrowserInit failed");

	#ifdef JSVERBOSE 
	printf("\tVRML Browser interface loaded,\n");
	#endif


	/* now, run initial scripts */
	if (!ActualrunScript(num,DefaultScriptMethods,&rval))
		cleanupDie(num,"runScript failed in VRML::newJS DefaultScriptMethods");

	/* send this data over to the routing table functions. */
	CRoutes_js_new (num, JAVASCRIPT);

	#ifdef JSVERBOSE 
	printf("\tVRML browser initialized\n");
	#endif
}

/* run the script from within C */
int ActualrunScript(uintptr_t num, char *script, jsval *rval) {
	size_t len;
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[num].cx;
	_globalObj = (JSObject *)ScriptControl[num].glob;

	#ifdef JSVERBOSE
		printf("ActualrunScript script %d cx %x \"%s\", \n",
			   num, _context, script);
	#endif
	len = strlen(script);
	if (!JS_EvaluateScript(_context, _globalObj, script, len,
						   FNAME_STUB, LINENO_STUB, rval)) {
		printf("JS_EvaluateScript failed for \"%s\".\n", script);
		return JS_FALSE;
	 }

	#ifdef JSVERBOSE 
	printf ("runscript passed\n");
	#endif

	return JS_TRUE;
}



/* run the script from within Javascript  */
int jsrrunScript(JSContext *_context, JSObject *_globalObj, char *script, jsval *rval) {

	size_t len;

	#ifdef JSVERBOSE
		printf("jsrrunScript script cx %x \"%s\", \n",
			   _context, script);
	#endif

	len = strlen(script);
	if (!JS_EvaluateScript(_context, _globalObj, script, len,
						   FNAME_STUB, LINENO_STUB, rval)) {
		printf("JS_EvaluateScript failed for \"%s\".\n", script);
		return JS_FALSE;
	 }

	#ifdef JSVERBOSE 
	printf ("runscript passed\n");
	#endif

	return JS_TRUE;
}

/* perl wants us to run the script- do so, and return return values */
int JSrunScript(uintptr_t num, char *script, SV *rstr, SV *rnum) {
	JSString *strval;
	jsval rval;
	jsdouble dval = -1.0;
	char *strp;
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[num].cx;
	_globalObj = (JSObject *)ScriptControl[num].glob;

	/* printf("JSrunScript - context %d %x  obj %d %x\n",_context,_context, _globalObj,_globalObj); */

	if (!ActualrunScript(num,script,&rval))
		return JS_FALSE;

	strval = JS_ValueToString(_context, rval);
	strp = JS_GetStringBytes(strval);
	sv_setpv(rstr, strp);
	#ifdef JSVERBOSE 
		printf("strp=\"%s\", ", strp);
	#endif

	if (!JS_ValueToNumber(_context, rval, &dval)) {
		printf("JS_ValueToNumber failed.\n");
		return JS_FALSE;
	}
	#ifdef JSVERBOSE 
		printf("dval=%.4g\n", dval);
	#endif

	sv_setnv(rnum, dval);
	return JS_TRUE;
}


/* perl wants a value returned. return return values */
 int JSGetProperty(uintptr_t num, char *script, SV *rstr) {
 	JSString *strval;
 	jsval rval;
 	char *strp;
 	JSContext *_context;
 	JSObject *_globalObj;

 	/* get context and global object for this script */
 	_context = (JSContext *) ScriptControl[num].cx;
 	_globalObj = (JSObject *)ScriptControl[num].glob;

	#ifdef JSVERBOSE
 		printf ("start of JSGetProperty, cx %d script %s\n",_context,script);
	#endif

 	if (!JS_GetProperty(_context, _globalObj, script, &rval)) {
 		printf("JSGetProperty verify failed for %s in SFNodeSetProperty.\n", script);
 		return JS_FALSE;
 	}

 	strval = JS_ValueToString(_context, rval);
 	strp = JS_GetStringBytes(strval);
 	sv_setpv(rstr, strp);
 	#ifdef JSVERBOSE 
 		printf("JSGetProperty strp=:%s:\n", strp);
 	#endif

 	return JS_TRUE;
 }


int JSaddGlobalAssignProperty(uintptr_t num, char *name, char *str) {
	jsval _rval = INT_TO_JSVAL(0);
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[num].cx;
	_globalObj = (JSObject *)ScriptControl[num].glob;
#define JSVERBOSE


	#ifdef JSVERBOSE 
		printf("addGlobalAssignProperty: cx: %x obj %x name \"%s\", evaluate script \"%s\"\n",
			   _context, _globalObj, name, str);
	#endif

	if (!JS_EvaluateScript(_context, _globalObj, str, strlen(str),
						   FNAME_STUB, LINENO_STUB, &_rval)) {
		printf("JS_EvaluateScript failed for \"%s\" in addGlobalAssignProperty.\n",
				str);
		return JS_FALSE;
	}
	if (!JS_DefineProperty(_context, _globalObj, name, _rval,
						   getAssignProperty, setAssignProperty,
						   0 | JSPROP_PERMANENT)) {
		printf("JS_DefineProperty failed for \"%s\" in addGlobalAssignProperty.\n",
				name);
		return JS_FALSE;
	}
	return JS_TRUE;
}


int JSaddSFNodeProperty(uintptr_t num, char *nodeName, char *name, char *str) {
	JSContext *_context;
	JSObject *_globalObj;
	JSObject *_obj;
	jsval _val, _rval = INT_TO_JSVAL(0);

	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[num].cx;
	_globalObj = (JSObject *)ScriptControl[num].glob;


	#ifdef JSVERBOSE 
		printf("addSFNodeProperty: name \"%s\", node name \"%s\", evaluate script \"%s\"\n",
			   name, nodeName, str);
	#endif

	if (!JS_GetProperty(_context, _globalObj, nodeName, &_val)) {
		printf("JS_GetProperty failed for \"%s\" in addSFNodeProperty.\n",
				nodeName);
		return JS_FALSE;
	}
	_obj = JSVAL_TO_OBJECT(_val);

	if (!JS_EvaluateScript(_context, _obj, str, strlen(str),
						   FNAME_STUB, LINENO_STUB, &_rval)) {
		printf("JS_EvaluateScript failed for \"%s\" in addSFNodeProperty.\n",
				str);
		return JS_FALSE;
	}
	if (!JS_DefineProperty(_context, _obj, name, _rval,
						   NULL, NULL,
						   0 | JSPROP_PERMANENT)) {
		printf("JS_DefineProperty failed for \"%s\" in addSFNodeProperty.\n",
				name);
		return JS_FALSE;
	}
	return JS_TRUE;
}

int JSaddGlobalECMANativeProperty(uintptr_t num, char *name) {
	JSContext *_context;
	JSObject *_globalObj;

	char buffer[STRING];
	jsval _val, rval = INT_TO_JSVAL(0);

	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[num].cx;
	_globalObj = (JSObject *)ScriptControl[num].glob;

	#ifdef JSVERBOSE 
		printf("addGlobalECMANativeProperty: name \"%s\"\n", name);
	#endif
	

	if (!JS_DefineProperty(_context, _globalObj, name, rval,
						   NULL, setECMANative,
						   0 | JSPROP_PERMANENT)) {
		printf("JS_DefineProperty failed for \"%s\" in addGlobalECMANativeProperty.\n",
				name);
		return JS_FALSE;
	}

	memset(buffer, 0, STRING);
	sprintf(buffer, "_%s_touched", name);
	_val = INT_TO_JSVAL(0);

	if (!JS_SetProperty(_context, _globalObj, buffer, &_val)) {
		printf("JS_SetProperty failed for \"%s\" in addGlobalECMANativeProperty.\n",
				buffer);
		return JS_FALSE;
	}
	return JS_TRUE;
}

#undef JSVERBOSE


/* FROM VRMLC.pm */

void *
SFNodeNativeNew(size_t vrmlstring_len, size_t handle_len)
{
	SFNodeNative *ptr;
	ptr = (SFNodeNative *) malloc(sizeof(*ptr));

	/* printf ("SFNodeNativeNew; string len %d handle_len %d\n",vrmlstring_len,handle_len);*/

	if (ptr == NULL) {
		return NULL;
	}
	ptr->vrmlstring = (char *) malloc(vrmlstring_len * sizeof(char));
	if (ptr->vrmlstring == NULL) {
		printf("malloc failed in SFNodeNativeNew.\n");
		return NULL;
	}
	ptr->handle = (char *) malloc(handle_len * sizeof(char));
	if (ptr->handle == NULL) {
		printf("malloc failed in SFNodeNativeNew.\n");
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFNodeNativeDelete(void *p)
{
	SFNodeNative *ptr;
	if (p != NULL) {
		ptr = (SFNodeNative *)p;
		if (ptr->vrmlstring != NULL) {
			free(ptr->vrmlstring);
		}
		if (ptr->handle != NULL) {
			free(ptr->handle);
		}
		free(ptr);
	}
}

/* assign this internally to the Javascript engine environment */
int
SFNodeNativeAssign(void *top, void *fromp)
{
	size_t to_vrmlstring_len = 0, to_handle_len = 0,
		from_vrmlstring_len = 0, from_handle_len = 0;
	SFNodeNative *to = (SFNodeNative *)top;
	SFNodeNative *from = (SFNodeNative *)fromp;

	/* printf ("SFNodeNativeAssign, assigning from vrmlstring %s handle %s to vrmlstring %s handle %s\n",*/
	/* 	from->vrmlstring,from->handle,to->vrmlstring,to->handle);*/

	to->touched++;

	to_vrmlstring_len = strlen(to->vrmlstring) + 1;
	to_handle_len = strlen(to->handle) + 1;

	from_vrmlstring_len = strlen(from->vrmlstring) + 1;
	from_handle_len = strlen(from->handle) + 1;

	/* printf ("lengths: %d %d, %d %d\n",from_handle_len,from_vrmlstring_len, to_handle_len,to_vrmlstring_len);*/


	if (from_vrmlstring_len > to_vrmlstring_len) {
		to->vrmlstring = (char *) realloc(to->vrmlstring,
										  from_vrmlstring_len * sizeof(char));
		if (to->vrmlstring == NULL) {
			printf("realloc failed in SFNodeNativeAssign.\n");
			return JS_FALSE;
		}
	}
	memset(to->vrmlstring, 0, from_vrmlstring_len);
	memmove(to->vrmlstring, from->vrmlstring, from_vrmlstring_len);

	if (from_handle_len > to_handle_len) {
		to->handle = (char *) realloc(to->handle,
									  from_handle_len * sizeof(char));
		if (to->handle == NULL) {
			printf("realloc failed in SFNodeNativeAssign.\n");
			return JS_FALSE;
		}
	}
	memset(to->handle, 0, from_handle_len);
	memmove(to->handle, from->handle, from_handle_len);

	return JS_TRUE;
}


void *
SFColorRGBANativeNew()
{
	SFColorRGBANative *ptr;
	ptr = (SFColorRGBANative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFColorRGBANativeDelete(void *p)
{
	SFColorRGBANative *ptr;
	if (p != NULL) {
		ptr = (SFColorRGBANative *)p;
		free(ptr);
	}
}

void
SFColorRGBANativeAssign(void *top, void *fromp)
{
	SFColorRGBANative *to = (SFColorRGBANative *)top;
	SFColorRGBANative *from = (SFColorRGBANative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFColorNativeNew()
{
	SFColorNative *ptr;
	ptr = (SFColorNative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFColorNativeDelete(void *p)
{
	SFColorNative *ptr;
	if (p != NULL) {
		ptr = (SFColorNative *)p;
		free(ptr);
	}
}

void
SFColorNativeAssign(void *top, void *fromp)
{
	SFColorNative *to = (SFColorNative *)top;
	SFColorNative *from = (SFColorNative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFImageNativeNew()
{
	SFImageNative *ptr;
	ptr =(SFImageNative *) malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFImageNativeDelete(void *p)
{
	SFImageNative *ptr;
	if (p != NULL) {
		ptr = (SFImageNative *)p;
		free(ptr);
	}
}

void
SFImageNativeAssign(void *top, void *fromp)
{
	SFImageNative *to = (SFImageNative *)top;
	/* SFImageNative *from = fromp; */
	UNUSED(fromp);

	to->touched++;
/* 	(to->v) = (from->v); */
}

void *
SFRotationNativeNew()
{
	SFRotationNative *ptr;
	ptr = (SFRotationNative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFRotationNativeDelete(void *p)
{
	SFRotationNative *ptr;
	if (p != NULL) {
		ptr = (SFRotationNative *)p;
		free(ptr);
	}
}

void
SFRotationNativeAssign(void *top, void *fromp)
{
	SFRotationNative *to = (SFRotationNative *)top;
	SFRotationNative *from = (SFRotationNative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFVec2fNativeNew()
{
	SFVec2fNative *ptr;
	ptr = (SFVec2fNative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFVec2fNativeDelete(void *p)
{
	SFVec2fNative *ptr;
	if (p != NULL) {
		ptr = (SFVec2fNative *)p;
		free(ptr);
	}
}

void
SFVec2fNativeAssign(void *top, void *fromp)
{
	SFVec2fNative *to = (SFVec2fNative *)top;
	SFVec2fNative *from = (SFVec2fNative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFVec3fNativeNew()
{
	SFVec3fNative *ptr;
	ptr = (SFVec3fNative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFVec3fNativeDelete(void *p)
{
	SFVec3fNative *ptr;
	if (p != NULL) {
		ptr = (SFVec3fNative *)p;
		free(ptr);
	}
}

void
SFVec3fNativeAssign(void *top, void *fromp)
{
	SFVec3fNative *to = (SFVec3fNative *)top;
	SFVec3fNative *from = (SFVec3fNative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}


/*********************
temporary for transitioning from perl to C

	kind: "eventOut" "eventIn" "field"
	type: "SFFloat" ,,, etc.

----------------------------------------------
ECMA fields:

when kind != "field", value is ignored.
expected value formats when kind == "field":

	SFInt32:	"333"
	SFFloat:	"3.14"
	SFBool:		"true" or "false"
	SFTime:		"50000.000"
	SFString:	""note the quotes needed around the string ""

----------------------------------------------
NON ECMA fields:
	when kind = "eventIn" value is expected to be ""
	when kind = "eventOut" value is expected to be the default; eg "0,0,0" (see VRMLFields.pm:VRMLFieldInit)
	when kind = "field", value is a string representation:

	SFRotation:	
	SFVec3f:	
	SFColor:	
	SFColorRGBA:	
	FreeWRLPTR:	
	SFNode:	
	SFVec2f:	a string like "1.0,0.0,2.0" - correct number of values, with 
			commas between values. It can optionally have "[" and "]" at the beginning
			and end of the string.

	MFFloat:	
	MFRotation:	
	MFVec3f:	
	MFBool:	
	MFInt32:	
	MFNode:	
	MFColor:	
	MFColorRGBA:	
	MFTime:	
	MFVec2f		A string, like the SFColors above. The code will split it up into the correct
			number of values; eg "[1,2,3,4,5,6]" will be 6 MFFloats, but 3 MFVec2Fs.	

	SFImage:	
	MFString:	
	


*/

void InitScriptField(int num,char *kind,char *type,char *field,char *value) {
	jsval rval;
	int ikind, itype;
	char *smallfield = NULL;
	char mynewname[400];
	char *cptr;
	int rows, commas;
	int elements;
	int count;
	char *sftype;

	itype = convert_typetoInt(type);
	ikind = findFieldInKEYWORDS(kind);

	/*
	printf ("InitScriptField, %d, kind %s (%d)type %s (%d) field %s value %s\n",
			num,kind,ikind,type,itype,field,value);
	*/

	/* input check */
	if ((ikind != KW_eventIn) && (ikind != KW_eventOut) && (ikind != KW_field)) {
		ConsoleMessage ("invalid kind for script field: %s\n",kind);
		return;	
	}

	if (itype == SFUNKNOWN) {
		ConsoleMessage ("invalid type for script field: %s\n",type);
		return;	
	}

	/* ok, lets handle the types here */
	switch (itype) {
		/* ECMA types */
		case SFBOOL:
		case SFFLOAT:
		case SFTIME:
		case SFINT32:
		case SFIMAGE:
		case SFSTRING: {
			/* do not care about eventIns */
			if (ikind != KW_eventIn)  {
				JSaddGlobalECMANativeProperty(num, field);

				if (ikind == KW_field) {
					smallfield = malloc (strlen(value) + strlen(field) + 10);
					sprintf (smallfield,"%s=%s\n",field,value);
					if (!ActualrunScript(num,smallfield,&rval))
						printf ("huh???\n");
				}
			}
			break;
		}
		/* non ECMA types */
		default: {
			/* first, do we need to make a new name up? */
			if (ikind == KW_eventIn) {
				sprintf (mynewname,"__tmp_arg_%s",field);
			} else strcpy(mynewname,field);

			/* SIMPLE MANIPULATION if the string comes in with "[" and/or "]", strip them off. */
			if ((cptr = strchr(value, '[')) != NULL) *cptr = ' ';
			if ((cptr = strchr(value, ']')) != NULL) *cptr = ' ';
			commas = countCommas (value) + 1;
			rows = returnElementRowSize (itype);
			elements = commas/rows;
			sftype = strdup(type);
			if (sftype[0] = 'M') sftype[0] = 'S';
			/* printf ("commas %d, rows %d, elements %d sftype %s\n",commas,rows,elements, sftype); */

			switch (itype) {
				case MFCOLOR:
				case MFCOLORRGBA:
				case MFVEC2F:
				case MFVEC3F:
				case MFROTATION: 
				case FREEWRLPTR:
				case SFNODE:
				case SFVEC2F:
				case SFCOLOR:
				case SFCOLORRGBA:
				case SFROTATION:
				case MFNODE:
				case MFTIME:
				case MFINT32:
				case MFFLOAT: 
				case SFVEC3F: {
					smallfield = malloc (strlen(type) + strlen(value) + 
						(elements*15) + 100);

					/* start the string */
					smallfield[0] = '\0';
					sprintf (smallfield,"new %s(",type);

					/* is this an MF with SF types? */
					if (rows>1) {
						/* is there any value here, at all? */
						if (elements > 0) {
							for (count = 0; count < commas; count++) {
								/* printf ("count %d mod %d\n",count, count % rows); */
	
								/* start of this element? */
								if ((count%rows) == 0) {
									strcat (smallfield,"new ");
									strcat (smallfield, sftype);
									strcat (smallfield, "(");
								}

								/* copy the value over */
								cptr = strchr(value,',');
								if (cptr != NULL) *cptr = '\0';
								strcat (smallfield, value);
								if (cptr != NULL) {
									value = cptr;
									value ++; 
								}
	
								/* end of this element? */
								if (((count+1)%rows) == 0) {
									strcat (smallfield,")");
									/* if we are not at the end, apply a comma */
									if (count <(commas-1)) strcat (smallfield,",");
								}
								else strcat (smallfield,",");
							}
						}

					} else strcat (smallfield, value);

					strcat (smallfield,")");
					JSaddGlobalAssignProperty (num,mynewname,smallfield);
					break;
				}
	
				default: {
					printf ("unhandled itype, in InitScriptField %s\n",type);
					return;
				}
			}
		}
	}

	FREE_IF_NZ (smallfield);

}
