/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/********************************************************************

Javascript C language binding.

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

#define MAX_RUNTIME_BYTES 0x100000L
#define STACK_CHUNK_SIZE 0x2000L


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


int JSVerbose = 0;
char *DefaultScriptMethods = "function initialize() {}; function shutdown() {}; function eventsProcessed() {}; TRUE=true; FALSE=false;";

/* housekeeping routines */

void JScleanup (int num) {
	UNUSED(num);
	//VRML::VRMLFunc::cleanupJS($this->{ScriptNum});
//	BrowserNative *brow = br;
//	JSContext *_context = cx;
//
//	if (brow) {
//		printf("\tfree browser internals!!!\n");
//		JS_free(_context, brow);
//		printf("\tbrowser internals freed!!!\n");
//	}
//
//	/* JS_DestroyContext(_context); */
//	/* printf("\tJS _context destroyed!!!\n"); */
//
//	JS_DestroyRuntime(runtime);
 //   JS_ShutDown();

}


void cleanupDie(int num, char *msg) {
	JScleanup(num);
	die(msg);
}

void JSInit(int num, SV *script) {
	jsval rval;
	JSContext *_context; 	/* these are set here */
	JSObject *_globalObj; 	/* these are set here */
	BrowserNative *br; 	/* these are set here */

	if (JSVerbose) printf("init:\n");

	/* more scripts than we can handle right now? */
	if (num >= MAXSCRIPTS) 
		cleanupDie (num,"Too many scripts; increase MAXSCRIPTS value and recompile\n");

	runtime = JS_NewRuntime(MAX_RUNTIME_BYTES);
	if (!runtime) die("JS_NewRuntime failed");
	
	if (JSVerbose) printf("\tJS runtime created,\n");
	
	
	_context = JS_NewContext(runtime, STACK_CHUNK_SIZE);
	if (!_context) die("JS_NewContext failed");
	
	if (JSVerbose) printf("\tJS context created,\n");
	
	
	_globalObj = JS_NewObject(_context, &globalClass, NULL, NULL);
	if (!_globalObj) die("JS_NewObject failed");
	
	if (JSVerbose) printf("\tJS global object created,\n");
	

	/* gets JS standard classes */
	if (!JS_InitStandardClasses(_context, _globalObj)) 
		die("JS_InitStandardClasses failed");
	
	if (JSVerbose) printf("\tJS standard classes initialized,\n");


	//if (JSVerbose) {
	//	reportWarningsOn();
	//} else {
	//	reportWarningsOff();
	//}
	
	JS_SetErrorReporter(_context, errorReporter);
	if (JSVerbose) printf("\tJS errror reporter set,\n");
	
	br = (BrowserNative *) JS_malloc(_context, sizeof(BrowserNative));
	br->sv_js = newSVsv(script); /* new duplicate of sv_js */
	br->magic = BROWMAGIC; /* needed ??? */

	/* for this script, here are the necessary data areas */
	JSglobs[num].cx = (unsigned int) _context;
	JSglobs[num].glob = (unsigned int) _globalObj;
	JSglobs[num].brow = (unsigned int) br;

	
	if (!loadVrmlClasses(_context, _globalObj)) 
		die("loadVrmlClasses failed");
	
	if (JSVerbose) printf("\tVRML classes loaded,\n");
	

	if (!VrmlBrowserInit(_context, _globalObj, br)) 
		die("VrmlBrowserInit failed");
	
	if (JSVerbose) printf("\tVRML Browser interface loaded,\n");

	/* now, run initial scripts */
	if (!ActualrunScript(num,DefaultScriptMethods,&rval)) 
		cleanupDie(num,"runScript failed in VRML::newJS DefaultScriptMethods");

	/* send this data over to the routing table functions. */
	CRoutes_js_new (num,(unsigned int)_context, (unsigned int)_globalObj,
		(unsigned int)br);
	

	//JAS printf ("calling javascript Initialize\n");
	//JAS /* and run the initialize function */
	//JAS if (!ActualrunScript(num, "initialize()",&rval)) {
	//JAS 	cleanupDie("runScript failed in VRML::JS::initialize",_context);
	//JAS }

	//JAS printf ("Now gathering events, with param now eq TRUE\n");
	//JAS gatherScriptEventOuts (num,TRUE);


	if (JSVerbose) printf("\tVRML browser initialized\n");
}

/* run the script from within C */
int ActualrunScript(int num, char *script, jsval *rval) {
	size_t len;
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) JSglobs[num].cx;
	_globalObj = (JSObject *)JSglobs[num].glob;


	if (JSVerbose) 
		printf("ActualrunScript script %d cx %x \"%s\", \n",
			   num, (unsigned int) _context, script);
	
	len = strlen(script);
	if (!JS_EvaluateScript(_context, _globalObj, script, len,
						   FNAME_STUB, LINENO_STUB, rval)) {
		fprintf(stderr, "JS_EvaluateScript failed for \"%s\".\n", script);
		return JS_FALSE;
	 }

	if (JSVerbose) printf ("runscript passed\n");
	return JS_TRUE;
}

/* perl wants us to run the script- do so, and return return values */
int JSrunScript(int num, char *script, SV *rstr, SV *rnum) {
	JSString *strval;
	jsval rval;
	jsdouble dval = -1.0;
	char *strp;
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) JSglobs[num].cx;
	_globalObj = (JSObject *)JSglobs[num].glob;

	if (!ActualrunScript(num,script,&rval))
		return JS_FALSE;

	strval = JS_ValueToString(_context, rval);
	strp = JS_GetStringBytes(strval);
	sv_setpv(rstr, strp);
	if (JSVerbose) {
		printf("strp=\"%s\", ", strp);
	}

	if (!JS_ValueToNumber(_context, rval, &dval)) {
		fprintf(stderr, "JS_ValueToNumber failed.\n");
		return JS_FALSE;
	}
	if (JSVerbose) {
		printf("dval=%.4g\n", dval);
	}
	sv_setnv(rnum, dval);
	return JS_TRUE;
}




int JSaddGlobalAssignProperty(int num, char *name, char *str) {
	jsval _rval = INT_TO_JSVAL(0);
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) JSglobs[num].cx;
	_globalObj = (JSObject *)JSglobs[num].glob;


	if (JSVerbose) {
		printf("addGlobalAssignProperty: name \"%s\", evaluate script \"%s\"\n",
			   name, str);
	}
	if (!JS_EvaluateScript(_context, _globalObj, str, strlen(str),
						   FNAME_STUB, LINENO_STUB, &_rval)) {
		fprintf(stderr,
				"JS_EvaluateScript failed for \"%s\" in addGlobalAssignProperty.\n",
				str);
		return JS_FALSE;
	}
	if (!JS_DefineProperty(_context, _globalObj, name, _rval,
						   getAssignProperty, setAssignProperty,
						   0 | JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"%s\" in addGlobalAssignProperty.\n",
				name);
		return JS_FALSE;
	}
	return JS_TRUE;
}



int JSaddSFNodeProperty(int num, char *nodeName, char *name, char *str) {
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) JSglobs[num].cx;
	_globalObj = (JSObject *)JSglobs[num].glob;

	JSObject *_obj;
	jsval _val, _rval = INT_TO_JSVAL(0);

	if (JSVerbose) {
		printf("addSFNodeProperty: name \"%s\", node name \"%s\", evaluate script \"%s\"\n",
			   name, nodeName, str);
	}
	if (!JS_GetProperty(_context, _globalObj, nodeName, &_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"%s\" in addSFNodeProperty.\n",
				nodeName);
		return JS_FALSE;
	}
	_obj = JSVAL_TO_OBJECT(_val);

	if (!JS_EvaluateScript(_context, _obj, str, strlen(str),
						   FNAME_STUB, LINENO_STUB, &_rval)) {
		fprintf(stderr,
				"JS_EvaluateScript failed for \"%s\" in addSFNodeProperty.\n",
				str);
		return JS_FALSE;
	}
	if (!JS_DefineProperty(_context, _obj, name, _rval,
						   NULL, NULL,
						   0 | JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"%s\" in addSFNodeProperty.\n",
				name);
		return JS_FALSE;
	}
	return JS_TRUE;
}

int JSaddGlobalECMANativeProperty(int num, char *name) {
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) JSglobs[num].cx;
	_globalObj = (JSObject *)JSglobs[num].glob;

	char buffer[STRING];
	jsval _val, rval = INT_TO_JSVAL(0);

	if (JSVerbose) {
		printf("addGlobalECMANativeProperty: name \"%s\"\n", name);
	}

	if (!JS_DefineProperty(_context, _globalObj, name, rval,
						   NULL, setECMANative,
						   0 | JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"%s\" in addGlobalECMANativeProperty.\n",
				name);
		return JS_FALSE;
	}

	memset(buffer, 0, STRING);
	sprintf(buffer, "_%s_touched", name);
	_val = INT_TO_JSVAL(1);
	if (!JS_SetProperty(_context, _globalObj, buffer, &_val)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"%s\" in addGlobalECMANativeProperty.\n",
				buffer);
		return JS_FALSE;
	}
	return JS_TRUE;
}



/* FROM VRMLC.pm */

void *
SFNodeNativeNew(size_t vrmlstring_len, size_t handle_len)
{
	SFNodeNative *ptr;
	ptr = (SFNodeNative *) malloc(sizeof(*ptr));

	//printf ("SFNodeNativeNew; string len %d handle_len %d\n",vrmlstring_len,handle_len);

	if (ptr == NULL) {
		return NULL;
	}
	ptr->vrmlstring = (char *) malloc(vrmlstring_len * sizeof(char));
	if (ptr->vrmlstring == NULL) {
		fprintf(stderr, "malloc failed in SFNodeNativeNew.\n");
		return NULL;
	}
	ptr->handle = (char *) malloc(handle_len * sizeof(char));
	if (ptr->handle == NULL) {
		fprintf(stderr, "malloc failed in SFNodeNativeNew.\n");
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
		ptr = p;
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
	SFNodeNative *to = top;
	SFNodeNative *from = fromp;

	//printf ("SFNodeNativeAssign, assigning from vrmlstring %s handle %s to vrmlstring %s handle %s\n",
	//	from->vrmlstring,from->handle,to->vrmlstring,to->handle);

	to->touched++;

	to_vrmlstring_len = strlen(to->vrmlstring) + 1;
	to_handle_len = strlen(to->handle) + 1;

	from_vrmlstring_len = strlen(from->vrmlstring) + 1;
	from_handle_len = strlen(from->handle) + 1;

	//printf ("lengths: %d %d, %d %d\n",from_handle_len,from_vrmlstring_len, to_handle_len,to_vrmlstring_len);


	if (from_vrmlstring_len > to_vrmlstring_len) {
		to->vrmlstring = (char *) realloc(to->vrmlstring,
										  from_vrmlstring_len * sizeof(char));
		if (to->vrmlstring == NULL) {
			fprintf(stderr, "realloc failed in SFNodeNativeAssign.\n");
			return JS_FALSE;
		}
	}
	memset(to->vrmlstring, 0, from_vrmlstring_len);
	memmove(to->vrmlstring, from->vrmlstring, from_vrmlstring_len);

	if (from_handle_len > to_handle_len) {
		to->handle = (char *) realloc(to->handle,
									  from_handle_len * sizeof(char));
		if (to->handle == NULL) {
			fprintf(stderr, "realloc failed in SFNodeNativeAssign.\n");
			return JS_FALSE;
		}
	}
	memset(to->handle, 0, from_handle_len);
	memmove(to->handle, from->handle, from_handle_len);

	return JS_TRUE;
}


void *
SFColorNativeNew()
{
	SFColorNative *ptr;
	ptr = malloc(sizeof(*ptr));
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
		ptr = p;
		free(ptr);
	}
}

void
SFColorNativeAssign(void *top, void *fromp)
{
	SFColorNative *to = top;
	SFColorNative *from = fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFImageNativeNew()
{
	SFImageNative *ptr;
	ptr = malloc(sizeof(*ptr));
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
		ptr = p;
		free(ptr);
	}
}

void
SFImageNativeAssign(void *top, void *fromp)
{
	SFImageNative *to = top;
	/* SFImageNative *from = fromp; */
	UNUSED(fromp);

	to->touched++;
/* 	(to->v) = (from->v); */
}

void *
SFRotationNativeNew()
{
	SFRotationNative *ptr;
	ptr = malloc(sizeof(*ptr));
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
		ptr = p;
		free(ptr);
	}
}

void
SFRotationNativeAssign(void *top, void *fromp)
{
	SFRotationNative *to = top;
	SFRotationNative *from = fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFVec2fNativeNew()
{
	SFVec2fNative *ptr;
	ptr = malloc(sizeof(*ptr));
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
		ptr = p;
		free(ptr);
	}
}

void
SFVec2fNativeAssign(void *top, void *fromp)
{
	SFVec2fNative *to = top;
	SFVec2fNative *from = fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFVec3fNativeNew()
{
	SFVec3fNative *ptr;
	ptr = malloc(sizeof(*ptr));
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
		ptr = p;
		free(ptr);
	}
}

void
SFVec3fNativeAssign(void *top, void *fromp)
{
	SFVec3fNative *to = top;
	SFVec3fNative *from = fromp;
	to->touched++;
	(to->v) = (from->v);
}
