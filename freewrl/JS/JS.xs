/*
 * Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License
 * (file COPYING in the distribution) for conditions of use and
 * redistribution, EXCEPT on the files which belong under the
 * Mozilla public license.
 * 
 * $Id$
 * 
 * A substantial amount of code has been adapted from the embedding
 * tutorials from the SpiderMonkey web pages
 * (http://www.mozilla.org/js/spidermonkey/)
 * and from js/src/js.c, which is the sample application included with
 * the javascript engine.
 *
 */

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>


#ifndef __jsUtils_h__
#include "jsUtils.h" /* misc helper C functions and globals */
#endif

#ifndef __jsVRMLBrowser_h__
#include "jsVRMLBrowser.h" /* VRML browser script interface implementation */
#endif

#include "jsVRMLClasses.h" /* VRML field type implementation */


#define MAX_RUNTIME_BYTES 0x100000L
#define STACK_CHUNK_SIZE 0x2000L

JSBool verbose = 0;


/*
 * Global JS variables (from Brendan Eich's short embedding tutorial):
 *
 * JSRuntime       - 1 runtime per process
 * JSContext       - 1 context per thread
 * global JSObject - 1 global object per context
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

/*
 * See perldoc perlapi, perlcall, perlembed, perlguts for how this all
 * works.
 */
void
doPerlCallMethod(SV *sv, const char *methodName)
{
	int count = 0;
	SV *retSV;

 #define PERL_NO_GET_CONTEXT

	dSP; /* local copy of stack pointer (don't leave home without it) */
	ENTER;
	SAVETMPS;
	PUSHMARK(SP); /* keep track of the stack pointer */
	XPUSHs(sv); /* push package ref to the stack */
	PUTBACK;
	count = call_method(methodName, G_SCALAR);

	SPAGAIN; /* refresh local copy of the stack pointer */
	
	if (count > 1) {
		fprintf(stderr,
				"doPerlCallMethod: call_method returned in list context - shouldn't happen here!\n");
	}

	PUTBACK;
	FREETMPS;
	LEAVE;
}

void
doPerlCallMethodVA(SV *sv, const char *methodName, const char *format, ...)
{
	va_list ap; /* will point to each unnamed argument in turn */
	char *c;
	void *v;
	int count = 0;
	size_t len = 0;
	const char *p = format;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv);

	va_start(ap, format); /* point to first element after format*/
	while(*p) {
		switch (*p++) {
		case 's':
			c = va_arg(ap, char *);
			len = strlen(c);
			c[len] = 0;
			XPUSHs(sv_2mortal(newSVpv(c, len)));
			break;
		case 'p':
			v = va_arg(ap, void *);
			XPUSHs(sv_2mortal(newSViv((IV) v)));
			break;
		default:
			fprintf(stderr, "doPerlCallMethodVA: argument type not supported!\n");
			break;
		}
	}
	va_end(ap);

	PUTBACK;
	count = call_method(methodName, G_SCALAR);

	SPAGAIN;
	

if (count > 1) {
		fprintf(stderr,
				"doPerlCallMethodVA: call_method returned in list context - shouldn't happen here!\n");
	}

	PUTBACK;
	FREETMPS;
	LEAVE;
}


void *
SFNodeNativeNew(size_t vrmlstring_len, size_t handle_len)
{
	SFNodeNative *ptr;
	ptr = (SFNodeNative *) malloc(sizeof(*ptr));
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

JSBool
SFNodeNativeAssign(void *top, void *fromp)
{
	size_t to_vrmlstring_len = 0, to_handle_len = 0,
		from_vrmlstring_len = 0, from_handle_len = 0;
	SFNodeNative *to = top;
	SFNodeNative *from = fromp;
	to->touched++;

	to_vrmlstring_len = strlen(to->vrmlstring) + 1;
	to_handle_len = strlen(to->handle) + 1;

	from_vrmlstring_len = strlen(from->vrmlstring) + 1;
	from_handle_len = strlen(from->handle) + 1;

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

void
SFColorNativeSet(void *p, SV *sv)
{
	SFColorNative *ptr = p;
	AV *a;
	SV **b;
	int i;

	if (verbose) {
		printf("SFColorNativeSet\n");
	}

	if (!SvROK(sv)) {
		(ptr->v).c[0] = 0.0;
		(ptr->v).c[1] = 0.0;
		(ptr->v).c[2] = 0.0;
	} else {
		if (SvTYPE(SvRV(sv)) != SVt_PVAV) {
			fprintf(stderr, "SFColor without being arrayref in SFColorNativeSet.\n");
			return;
		}
		a = (AV *) SvRV(sv);
		for (i = 0; i < 3; i++) {
			b = av_fetch(a, i, 1); /* LVal for easiness */
			if (!b) {
				fprintf(stderr, "SFColor b is NULL in SFColorNativeSet.\n");
				return;
			}
			(ptr->v).c[i] = SvNV(*b);
		}
	}
	ptr->touched = 0;
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
	SFImageNative *from = fromp;
	to->touched++;
/* 	(to->v) = (from->v); */
}

void
SFImageNativeSet(void *p, SV *sv)
{
	SFImageNative *ptr = p;
	UNUSED(sv);

	ptr->touched = 0;
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

void
SFRotationNativeSet(void *p, SV *sv)
{
	SFRotationNative *ptr = p;
	AV *a;
	SV **b;
	int i;

	if (verbose) {
		printf("SFRotationNativeSet\n");
	}
	if (!SvROK(sv)) {
		(ptr->v).r[0] = 0.0;
		(ptr->v).r[1] = 1.0;
		(ptr->v).r[2] = 0.0;
		(ptr->v).r[3] = 0.0;
	} else {
		if (SvTYPE(SvRV(sv)) != SVt_PVAV) {
			fprintf(stderr, "SFRotation without being arrayref in SFRotationNativeSet.\n");
			return;
		}
		a = (AV *) SvRV(sv);
		for (i = 0; i < 4; i++) {
			b = av_fetch(a, i, 1); /* LVal for easiness */
			if (!b) {
				fprintf(stderr, "SFRotation b is NULL in SFRotationNativeSet.\n");
				return;
			}
			(ptr->v).r[i] = SvNV(*b);
		}
	}
	ptr->touched = 0;
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

void
SFVec2fNativeSet(void *p, SV *sv)
{
	AV *a;
	SV **b;
	int i;
	SFVec2fNative *ptr = p;

	if (verbose) {
		printf("SFVec2fNativeSet\n");
	}

	if (!SvROK(sv)) {
		(ptr->v).c[0] = 0.0;
		(ptr->v).c[1] = 0.0;
	} else if (SvTYPE(SvRV(sv)) != SVt_PVAV) {
			fprintf(stderr, "SFVec2f without being arrayref in SFVec2fNativeSet.\n");
			return;
	} else {
		a = (AV *) SvRV(sv);
		for (i = 0; i < 2; i++) {
			b = av_fetch(a, i, 1); /* LVal for easiness */
			if (!b) {
				fprintf(stderr, "SFVec2f b is NULL in SFVec2fNativeSet.\n");
				return;
			}
			(ptr->v).c[i] = SvNV(*b);
		}
	}
	ptr->touched = 0;
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

void
SFVec3fNativeSet(void *p, SV *sv)
{
	AV *a;
	SV **b;
	int i;
	SFVec3fNative *ptr = p;

	if (verbose) {
		printf("SFVec3fNativeSet\n");
	}

	if (!SvROK(sv)) {
		(ptr->v).c[0] = 0.0;
		(ptr->v).c[1] = 0.0;
		(ptr->v).c[2] = 0.0;
	} else if (SvTYPE(SvRV(sv)) != SVt_PVAV) {
			fprintf(stderr, "SFVec3f without being arrayref in SFVec3fNativeSet.\n");
			return;
	} else {
		a = (AV *) SvRV(sv);
		for (i = 0; i < 3; i++) {
			b = av_fetch(a, i, 1); /* LVal for easiness */
			if (!b) {
				fprintf(stderr, "SFVec3f b is NULL in SFVec3fNativeSet.\n");
				return;
			}
			(ptr->v).c[i] = SvNV(*b);
		}
	}
	ptr->touched = 0;
}



MODULE = VRML::JS	PACKAGE = VRML::JS
PROTOTYPES: ENABLE


void
setVerbose(v)
	JSBool v;
CODE:
{
	verbose = v;
}


## worry about garbage collection here ???
void
init(cx, glob, brow, sv_js)
	void *cx
	void *glob
	void *brow
	SV *sv_js
CODE:
{
    JSContext *_context;
	JSObject *_globalObj;
	BrowserNative *br;

	if (verbose) {
		printf("init:\n");
	}

	runtime = JS_NewRuntime(MAX_RUNTIME_BYTES);
	if (!runtime) {
		die("JS_NewRuntime failed");
	}
	if (verbose) {
		printf("\tJS runtime created,\n");
	}
	
	_context = JS_NewContext(runtime, STACK_CHUNK_SIZE);
	if (!_context) {
		die("JS_NewContext failed");
	}
	cx = _context;
	if (verbose) {
		printf("\tJS context created,\n");
	}
	
	_globalObj = JS_NewObject(_context, &globalClass, NULL, NULL);
	if (!_globalObj) {
		die("JS_NewObject failed");
	}
	glob = _globalObj;
	if (verbose) {
		printf("\tJS global object created,\n");
	}

	/* gets JS standard classes */
	if (!JS_InitStandardClasses(_context, _globalObj)) {
		die("JS_InitStandardClasses failed");
	}
	if (verbose) {
		printf("\tJS standard classes initialized,\n");
	}

	if (verbose) {
		reportWarningsOn();
	} else {
		reportWarningsOff();
	}
	
	JS_SetErrorReporter(_context, errorReporter);
	if (verbose) {
		printf("\tJS errror reporter set,\n");
	}

	br = (BrowserNative *) JS_malloc(_context, sizeof(BrowserNative));
	br->sv_js = newSVsv(sv_js); /* new duplicate of sv_js */
	br->magic = BROWMAGIC; /* needed ??? */
	brow = br;
	
	if (!loadVrmlClasses(_context, _globalObj)) {
		die("loadVrmlClasses failed");
	}
	if (verbose) {
		printf("\tVRML classes loaded,\n");
	}

	if (!VrmlBrowserInit(_context, _globalObj, br)) {
		die("VrmlBrowserInit failed");
	}
	if (verbose) {
		printf("\tVRML browser initialized\n");
	}
}
OUTPUT:
cx
glob
brow
sv_js


void
cleanupJS(cx, br)
	void *cx
	void *br
CODE:
{
	BrowserNative *brow = br;
	JSContext *_context = cx;

	if (brow) {
		printf("\tfree browser internals!!!\n");
		JS_free(_context, brow);
		printf("\tbrowser internals freed!!!\n");
	}

	/* JS_DestroyContext(_context); */
	/* printf("\tJS _context destroyed!!!\n"); */

	JS_DestroyRuntime(runtime);
    JS_ShutDown();
}


JSBool
runScript(cx, obj, script, rstr, rnum)
	void *cx
	void *obj
	char *script
	SV *rstr
	SV *rnum
CODE:
{
	JSObject *_obj = obj;
	JSContext *_context = cx;
	JSString *strval;
	jsval rval;
	jsdouble dval = -1.0;
	char *strp;
	size_t len;

	if (verbose) {
		printf("runScript \"%s\", ", script);
	}
	len = strlen(script);
	if (!JS_EvaluateScript(_context, _obj, script, len,
						   FNAME_STUB, LINENO_STUB, &rval)) {
		fprintf(stderr, "JS_EvaluateScript failed for \"%s\".\n", script);
		RETVAL = JS_FALSE;
		return;
	}
	strval = JS_ValueToString(_context, rval);
	strp = JS_GetStringBytes(strval);
	sv_setpv(rstr, strp);
	if (verbose) {
		printf("strp=\"%s\", ", strp);
	}

	if (!JS_ValueToNumber(_context, rval, &dval)) {
		fprintf(stderr, "JS_ValueToNumber failed.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if (verbose) {
		printf("dval=%.4g\n", dval);
	}
	sv_setnv(rnum, dval);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj
rstr
rnum



JSBool
jsSFColorSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	JSContext *_cx;
	JSObject *_obj, *_sfcolObj;
	jsval _val;
	void *_privPtr;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFColorSet: obj %u, name %s\n", (unsigned int) _obj, name);
	}
	if (!JS_GetProperty(_cx, _obj, name, &_val)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFColorSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if (!JSVAL_IS_OBJECT(_val)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFColorSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_sfcolObj = JSVAL_TO_OBJECT(_val);

	if ((_privPtr = JS_GetPrivate(_cx, _sfcolObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFColorSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFColorNativeSet(_privPtr, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj


JSBool
jsSFImageSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	JSContext *_cx;
	JSObject *_obj, *_sfimObj;
	jsval _val;
	void *_privPtr;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFImageSet: obj %u, name %s\n", (unsigned int) _obj, name);
	}
	if (!JS_GetProperty(_cx, _obj, name, &_val)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFImageSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if (!JSVAL_IS_OBJECT(_val)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFImageSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_sfimObj = JSVAL_TO_OBJECT(_val);

	if ((_privPtr = JS_GetPrivate(_cx, _sfimObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFColorSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFImageNativeSet(_privPtr, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj


JSBool
jsSFVec2fSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	JSContext *_cx;
	JSObject *_obj, *_sfvec2fObj;
	jsval _val;
	void *_privPtr;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFVec2fSet: obj %u, name %s\n", (unsigned int) _obj, name);
	}
	if(!JS_GetProperty(_cx, _obj, name, &_val)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFVec2fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if(!JSVAL_IS_OBJECT(_val)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFVec2fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_sfvec2fObj = JSVAL_TO_OBJECT(_val);

	if ((_privPtr = JS_GetPrivate(_cx, _sfvec2fObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFVec2fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFVec2fNativeSet(_privPtr, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj


JSBool
jsSFVec3fSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	JSContext *_cx;
	JSObject *_obj, *_sfvec3fObj;
	jsval _val;
	void *_privPtr;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFVec3fSet: obj %u, name %s\n", (unsigned int) _obj, name);
	}
	if(!JS_GetProperty(_cx, _obj, name, &_val)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFVec3fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if(!JSVAL_IS_OBJECT(_val)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFVec3fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_sfvec3fObj = JSVAL_TO_OBJECT(_val);

	if ((_privPtr = JS_GetPrivate(_cx, _sfvec3fObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFVec3fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFVec3fNativeSet(_privPtr, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj


JSBool
jsSFRotationSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	JSContext *_cx;
	JSObject *_obj, *_sfrotObj;
	jsval _val;
	void *_privPtr;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFRotationSet: obj %u, name %s\n", (unsigned int) _obj, name);
	}
	if (!JS_GetProperty(_cx, _obj, name, &_val)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFRotationSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if (!JSVAL_IS_OBJECT(_val)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFRotationSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_sfrotObj = JSVAL_TO_OBJECT(_val);

	if ((_privPtr = JS_GetPrivate(_cx, _sfrotObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFRotationSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFRotationNativeSet(_privPtr, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj


JSBool
addSFNodeProperty(cx, glob, nodeName, name, str)
	void *cx
	void *glob
	char *nodeName
	char *name
	char *str
CODE:
{
	JSContext *_context;
	JSObject *_globalObj, *_obj;
	jsval _val, _rval = INT_TO_JSVAL(0);

	_context = cx;
	_globalObj = glob;

	if (verbose) {
		printf("addSFNodeProperty: name \"%s\", node name \"%s\", evaluate script \"%s\"\n",
			   name, nodeName, str);
	}
	if (!JS_GetProperty(_context, _globalObj, nodeName, &_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"%s\" in addSFNodeProperty.\n",
				nodeName);
		RETVAL = JS_FALSE;
		return;
	}
	_obj = JSVAL_TO_OBJECT(_val);

	if (!JS_EvaluateScript(_context, _obj, str, strlen(str),
						   FNAME_STUB, LINENO_STUB, &_rval)) {
		fprintf(stderr,
				"JS_EvaluateScript failed for \"%s\" in addSFNodeProperty.\n",
				str);
		RETVAL = JS_FALSE;
		return;
	}
	if (!JS_DefineProperty(_context, _obj, name, _rval,
						   NULL, NULL,
						   0 | JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"%s\" in addSFNodeProperty.\n",
				name);
		RETVAL = JS_FALSE;
		return;
	}
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
glob


JSBool
addGlobalAssignProperty(cx, glob, name, str)
	void *cx
	void *glob
	char *name
	char *str
CODE:
{
	JSContext *_context;
	JSObject *_globalObj;
	jsval _rval = INT_TO_JSVAL(0);

	_context = cx;
	_globalObj = glob;

	if (verbose) {
		printf("addGlobalAssignProperty: name \"%s\", evaluate script \"%s\"\n",
			   name, str);
	}
	if (!JS_EvaluateScript(_context, _globalObj, str, strlen(str),
						   FNAME_STUB, LINENO_STUB, &_rval)) {
		fprintf(stderr,
				"JS_EvaluateScript failed for \"%s\" in addGlobalAssignProperty.\n",
				str);
		RETVAL = JS_FALSE;
		return;
	}
	if (!JS_DefineProperty(_context, _globalObj, name, _rval,
						   getAssignProperty, setAssignProperty,
						   0 | JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"%s\" in addGlobalAssignProperty.\n",
				name);
		RETVAL = JS_FALSE;
		return;
	}
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
glob


JSBool
addGlobalECMANativeProperty(cx, glob, name)
	void *cx
	void *glob
	char *name
CODE:
{
	JSContext *_context;
	JSObject *_globalObj;
	char buffer[STRING];
	jsval _val, rval = INT_TO_JSVAL(0);

	_context = cx;
	_globalObj = glob;
	if (verbose) {
		printf("addGlobalECMANativeProperty: name \"%s\"\n", name);
	}

	if (!JS_DefineProperty(_context, _globalObj, name, rval,
						   NULL, setECMANative,
						   0 | JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"%s\" in addGlobalECMANativeProperty.\n",
				name);
		RETVAL = JS_FALSE;
		return;
	}

	memset(buffer, 0, STRING);
	sprintf(buffer, "_%s_touched", name);
	_val = INT_TO_JSVAL(1);
	if (!JS_SetProperty(_context, _globalObj, buffer, &_val)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"%s\" in addGlobalECMANativeProperty.\n",
				buffer);
		RETVAL = JS_FALSE;
		return;
	}
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
glob
