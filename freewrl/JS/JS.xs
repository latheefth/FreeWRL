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


/* #define MAX_RUNTIME_BYTES 0x100000UL */
#define MAX_RUNTIME_BYTES 0xF4240L
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

void
doPerlCallMethod(SV *sv, const char *methodName)
{
	int count = 0;
 #define PERL_NO_GET_CONTEXT
	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(sp);
	XPUSHs(sv);
	PUTBACK;
	count = perl_call_method(methodName, G_SCALAR);
	if (count && verbose) {
		printf("perl_call_method returned %.1g in doPerlCallMethod.\n", POPn);
	}
	PUTBACK;
	FREETMPS;
	LEAVE;
}

void
doPerlCallMethodVA(SV *sv, const char *methodName, const char *format, ...)
{
	va_list ap; /*will point to each unnamed argument in turn*/
	char *c;
	void *v;
	int count = 0;
	size_t len = 0;
	const char *p = format;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(sp);
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
	count = perl_call_method(methodName, G_SCALAR);
	if (count && verbose) {
		printf("perl_call_method returned %.1g in doPerlCallMethod.\n", POPn);
	}
	PUTBACK;
	FREETMPS;
	LEAVE;
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
#if FALSE
/* 	(ptr->v).__data = malloc(LARGESTRING * sizeof(char)); */
#endif
	return ptr;
}

void
SFImageNativeDelete(void *p)
{
	SFImageNative *ptr;
	if (p != NULL) {
		ptr = p;
#if FALSE
/* 		if ((ptr->v).__data != NULL) { */
/* 			free((ptr->v).__data); */
/* 		} */
#endif
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
#if FALSE
/* 	AV *a; */
/* 	SV **__data, **__x, **__y, **__depth, **__texture; */
/* 	STRLEN pl_na; */
/* 	size_t _len, _data_s; */
/* 	char *c; */

/* 	if (!SvROK(sv)) { */
/* 		(ptr->v).__x = 0; */
/* 		(ptr->v).__y = 0; */
/* 		(ptr->v).__depth = 0; */
/* 		(ptr->v).__data = ""; */
/* 		(ptr->v).__texture = 0; */
/* 	} else if (SvTYPE(SvRV(sv)) != SVt_PVAV) { */
/* 			fprintf(stderr, "SFImage without being arrayref in SFImageNativeSet.\n"); */
/* 			return; */
/* 	} else { */
/* 		a = (AV *) SvRV(sv); */

		/* __x */
/* 		__x = av_fetch(a, 0, 1); */
/* 		if (!__x) { */
/* 			fprintf(stderr, "SFImage __x is NULL in SFImageNativeSet.\n"); */
/* 			return; */
/* 		} */
/* 		(ptr->v).__x = SvNV(*__x); */

		/* __y */
/* 		__y = av_fetch(a, 1, 1); */
/* 		if (!__y) { */
/* 			fprintf(stderr, "SFImage __y is NULL in SFImageNativeSet.\n"); */
/* 			return; */
/* 		} */
/* 		(ptr->v).__y = SvNV(*__y); */

		/* __depth */
/* 		__depth = av_fetch(a, 2, 1); */
/* 		if (!__depth) { */
/* 			fprintf(stderr, "SFImage __depth is NULL in SFImageNativeSet.\n"); */
/* 			return; */
/* 		} */
/* 		(ptr->v).__depth = SvNV(*__depth); */

		/* Handle image data */
		/* __data = av_fetch(a, 4, 1); */
/* 		__data = av_fetch(a, 3, 1); */ /* ??? */
/* 		if (!__data) { */
/* 			fprintf(stderr, "SFImage __data is NULL in SFImageNativeSet.\n"); */
/* 			return; */
/* 		} */
/* 		c = SvPV(*__data, pl_na); */
/* 		_len = strlen(c); */
/* 		_data_s = sizeof((ptr->v).__data); */
/* 		if (_len * sizeof(char) > _data_s) { */
/* 			_data_s = (_len + 1) * sizeof(char); */
/* 			if (((ptr->v).__data = */
/* 				 (char *) realloc((ptr->v).__data, _data_s)) */
/* 				== NULL) { */
/* 				fprintf(stderr, */
/* 						"realloc failed in SFImageNativeSet.\n"); */
/* 				return; */
/* 			} */
/* 		} */
/* 		memset((ptr->v).__data, 0, _len + 1); */
/* 		memmove((ptr->v).__data, c, _len); */

		/* __texture */
/* 		__texture = av_fetch(a, 4, 1); */ /* ??? */
/* 		if (!__texture) { */
/* 			fprintf(stderr, "SFImage __texture is NULL in SFImageNativeSet.\n"); */
/* 			return; */
/* 		} */
/* 		(ptr->v).__texture = SvNV(*__texture); */
/* 	} */
#endif
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
	ptr->touched = 0;

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
		if (verbose) {
			printf("\tvec3f values: (%.1g, %.1g)\n",
				   (ptr->v).c[0],
				   (ptr->v).c[1]);
		}
	}
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
	ptr->touched = 0;

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
		if (verbose) {
			printf("\tvec3f values: (%.1g, %.1g, %.1g)\n",
				   (ptr->v).c[0],
				   (ptr->v).c[1],
				   (ptr->v).c[2]);
		}
	}
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
init(context, global, brow, sv_js)
	void *context
	void *global
	void *brow
	SV *sv_js
CODE:
{
    JSContext *cx;
	BrowserNative *br;
	JSObject *glob;

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
	
	cx = JS_NewContext(runtime, STACK_CHUNK_SIZE);
	if (!cx) {
		die("JS_NewContext failed");
	}
	context = cx;
	if (verbose) {
		printf("\tJS context created,\n");
	}
	
	glob = JS_NewObject(context, &globalClass, NULL, NULL);
	if (!glob) {
		die("JS_NewObject failed");
	}
	global = glob;
	if (verbose) {
		printf("\tJS global object created,\n");
	}

	/* gets JS standard classes */
	if (!JS_InitStandardClasses(context, global)) {
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
	
	JS_SetErrorReporter(context, errorReporter);
	if (verbose) {
		printf("\tJS errror reporter set,\n");
	}

	br = (BrowserNative *) JS_malloc(context, sizeof(BrowserNative));
	br->sv_js = newSVsv(sv_js); /* new duplicate of sv_js */
	br->magic = BROWMAGIC; /* needed ??? */
	brow = br;
	
	if (!loadVrmlClasses(cx, glob)) {
	/* if (!LoadVrmlClasses(context, global)) { */
		die("loadVrmlClasses failed");
	}
	if (verbose) {
		printf("\tVRML classes loaded,\n");
	}

	if (!VrmlBrowserInit(cx, glob, br)) {
		die("VrmlBrowserInit failed");
	}
	if (verbose) {
		printf("\tVRML browser initialized\n");
	}
}
OUTPUT:
context
global
brow
sv_js

void
cleanupJS(cx, br)
	void *cx
	void *br
CODE:
{
	BrowserNative *brow = br;
	JSContext *context = cx;

	if (brow) {
		printf("\tfree browser internals!!!\n");
		JS_free(context, brow);
		printf("\tbrowser internals freed!!!\n");
	}

	/* JS_DestroyContext(context); */
	/* printf("\tJS context destroyed!!!\n"); */

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
	jsval rval;
	JSString *strval;
	jsdouble dval = -1.0;
	char *strp;
	size_t len;
	JSObject *_obj = obj;
	JSContext *context = cx;

	if (verbose) {
		printf("runScript: \"%s\", ", script);
	}
	len = strlen(script);
	if (!JS_EvaluateScript(context, _obj, script, len,
						   FNAME_STUB, LINENO_STUB, &rval)) {
		fprintf(stderr, "JS_EvaluateScript failed for \"%s\".\n", script);
		RETVAL = JS_FALSE;
		return;
	}
	strval = JS_ValueToString(context, rval);
	strp = JS_GetStringBytes(strval);
	sv_setpv(rstr, strp);
	if (verbose) {
		printf("strp=\"%s\", ", strp);
	}

	if (!JS_ValueToNumber(context, rval, &dval)) {
		fprintf(stderr, "JS_ValueToNumber failed.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if (verbose) {
		printf("dval=%.4g\n", dval);
	}
	sv_setnv(rnum, dval);
	//cx = context;
	//obj = _obj;

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
	jsval v;
	void *privateData;
	JSContext *_cx;
	JSObject *_obj, *_o;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFColorSet: obj = %u, name = %s\n", (unsigned int) _obj, name);
	}
	if (!JS_GetProperty(_cx, _obj, name, &v)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFColorSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if (!JSVAL_IS_OBJECT(v)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFColorSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_o = JSVAL_TO_OBJECT(v);

	if ((privateData = JS_GetPrivate(_cx, _o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFColorSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFColorNativeSet(privateData, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj
sv


JSBool
jsSFImageSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	jsval v;
	void *privateData;
	JSContext *_cx;
	JSObject *_obj, *_o;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFImageSet: obj = %u, name = %s\n", (unsigned int) _obj, name);
	}
	if (!JS_GetProperty(_cx, _obj, name, &v)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFImageSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if (!JSVAL_IS_OBJECT(v)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFImageSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_o = JSVAL_TO_OBJECT(v);

	if ((privateData = JS_GetPrivate(_cx, _o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFColorSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFImageNativeSet(privateData, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj
sv


JSBool
jsSFVec2fSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	jsval v;
	void *privateData;
	JSContext *_cx;
	JSObject *_obj, *_o;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFVec2fSet: obj = %u, name = %s\n", (unsigned int) _obj, name);
	}
	if(!JS_GetProperty(_cx, _obj, name, &v)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFVec2fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if(!JSVAL_IS_OBJECT(v)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFVec2fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_o = JSVAL_TO_OBJECT(v);

	if ((privateData = JS_GetPrivate(_cx, _o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFVec2fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFVec2fNativeSet(privateData, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj
sv


JSBool
jsSFVec3fSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	jsval v;
	void *privateData;
	JSContext *_cx;
	JSObject *_obj, *_o;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFVec3fSet: obj = %u, name = %s\n", (unsigned int) _obj, name);
	}
	if(!JS_GetProperty(_cx, _obj, name, &v)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFVec3fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if(!JSVAL_IS_OBJECT(v)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFVec3fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_o = JSVAL_TO_OBJECT(v);

	if ((privateData = JS_GetPrivate(_cx, _o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFVec3fSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFVec3fNativeSet(privateData, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj
sv


JSBool
jsSFRotationSet(cx, obj, name, sv)
	void *cx
	void *obj
	char *name
	SV *sv
CODE:
{
	jsval v;
	void *privateData;
	JSContext *_cx;
	JSObject *_obj, *_o;

	_cx = cx;
	_obj = obj;
	if (verbose) {
		printf("jsSFRotationSet: obj = %u, name = %s\n", (unsigned int) _obj, name);
	}
	if (!JS_GetProperty(_cx, _obj, name, &v)) {
		fprintf(stderr, "JS_GetProperty failed in jsSFRotationSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	if (!JSVAL_IS_OBJECT(v)) {
		fprintf(stderr, "JSVAL_IS_OBJECT failed in jsSFRotationSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	_o = JSVAL_TO_OBJECT(v);

	if ((privateData = JS_GetPrivate(_cx, _o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in jsSFRotationSet.\n");
		RETVAL = JS_FALSE;
		return;
	}
	SFRotationNativeSet(privateData, sv);
	RETVAL = JS_TRUE;
}
OUTPUT:
RETVAL
cx
obj
sv


JSBool
addAssignProperty(cx, glob, name, str)
	void *cx
	void *glob
	char *name
	char *str
CODE:
{
	JSContext *context;
	JSObject *globalObj;
	jsval _rval = INT_TO_JSVAL(0);

	context = cx;
	globalObj = glob;

	if (verbose) {
		printf("addAssignProperty: name = \"%s\", evaluate script = \"%s\"\n",
			   name, str);
	}
	if (!JS_EvaluateScript(context, globalObj, str, strlen(str),
						   FNAME_STUB, LINENO_STUB, &_rval)) {
		fprintf(stderr,
				"JS_EvaluateScript failed for \"%s\" in addAssignProperty.\n",
				str);
		RETVAL = JS_FALSE;
		return;
	}
	if (!JS_DefineProperty(context, globalObj,
						   /* name, _rval, getAssignProperty, setAssignProperty, */
						   name, _rval, JS_PropertyStub, setAssignProperty,
						   0 | JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"%s\" in addAssignProperty.\n",
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
addECMANativeProperty(cx, glob, name)
	void *cx
	void *glob
	char *name
CODE:
{
	JSContext *context;
	JSObject *globalObj;
	char buffer[STRING];
	jsval v, rval = INT_TO_JSVAL(0);

	context = cx;
	globalObj = glob;
	if (verbose) {
		printf("addECMANativeProperty: name = \"%s\"\n", name);
	}

	if (!JS_DefineProperty(context,
						   globalObj,
						   name,
						   rval,
						   NULL,
						   setECMANative,
						   0 | JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"%s\" in addECMANativeProperty.\n",
				name);
		RETVAL = JS_FALSE;
		return;
	}

	memset(buffer, 0, STRING);
	sprintf(buffer, "_%s_touched", name);
	v = INT_TO_JSVAL(1);
	if (!JS_SetProperty(context, globalObj, buffer, &v)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"%s\" in addECMANativeProperty.\n",
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
