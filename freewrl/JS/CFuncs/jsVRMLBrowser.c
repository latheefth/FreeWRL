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
 */

#include "jsVRMLBrowser.h"

JSBool
VrmlBrowserInit(JSContext *context, JSObject *globalObj, BrowserNative *brow)
{
	JSObject *obj;
	/* BrowserNative *brow = (BrowserNative *) JS_malloc(context, sizeof(BrowserNative)); */
	/* brow->sv_js = newSVsv(sv_js); */ /* new duplicate of sv_js */
	/* brow->magic = BROWMAGIC; */

	if (verbose) {
		printf("VrmlBrowserInit\n");
	}

	/* why not JS_InitClass ??? */
	obj = JS_DefineObject(context,
						  globalObj,
						  "Browser", 
						  &Browser,
						  NULL,
						  JSPROP_ENUMERATE | JSPROP_PERMANENT);
	if (!JS_DefineFunctions(context, obj, BrowserFunctions)) {
		return JS_FALSE;
	}

	if (!JS_SetPrivate(context, obj, brow)) {
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
VrmlBrowserCreateVrmlFromString(JSContext *context, JSObject *obj,
								uintN argc, jsval *argv, jsval *rval)
{
	jsval v;
	BrowserNative *brow;
	char *_c, *_c_args = "SFString vrmlSyntax", *_c_format = "s";

	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr,
				"JS_GetPrivate failed in VrmlBrowserCreateVrmlFromString.\n");
		return JS_FALSE;
	}
	
	if (brow->magic != BROWMAGIC) {
		fprintf(stderr, "Wrong browser magic!\n");
		return JS_FALSE;
	}

	if (argc == 1 &&
		JS_ConvertArguments(context, argc, argv, _c_format, &_c)) {
		doPerlCallMethodVA(brow->sv_js, "browserCreateVrmlFromString", "s", _c);
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for createVrmlFromString(%s).\n",
				_c_args);
		return JS_FALSE;
	}

	if (!JS_GetProperty(context, obj, "__bret",  &v)) {
		fprintf(stderr,
				"JS_GetProperty failed in VrmlBrowserCreateVrmlFromString.\n");
	}
	*rval = v;
	return JS_TRUE;
}

JSBool
VrmlBrowserCreateVrmlFromURL(JSContext *context, JSObject *obj,
							 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj[2];
	JSString *_str[2];
	JSClass *_cls[2];
	jsval _v[2], _rval;
	BrowserNative *brow;
	char *_c,
		*_c_args = "MFString url, SFNode node, SFString event",
		*_costr[2],
		*_c_format = "o o s";

	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr,
				"JS_GetPrivate failed in VrmlBrowserCreateVrmlFromUrl.\n");
		return JS_FALSE;
	}
	
	if (brow->magic != BROWMAGIC) {
		fprintf(stderr, "Wrong browser magic!\n");
		return JS_FALSE;
	}

	if (JS_ConvertArguments(context,
							argc,
							argv,
							_c_format,
							&(_obj[0]), &(_obj[1]), &_c)) {
		if ((_cls[0] = JS_GetClass(_obj[0])) == NULL) {
			fprintf(stderr,
					"JS_GetClass failed for arg 0 in VrmlBrowserCreateVrmlFromUrl.\n");
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GetClass(_obj[1])) == NULL) {
			fprintf(stderr,
					"JS_GetClass failed for arg 1 in VrmlBrowserCreateVrmlFromUrl.\n");
			return JS_FALSE;
		}
		if (memcmp("MFString", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("SFNode", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			fprintf(stderr,
					"\nIncorrect arguments in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}

		if (!JS_CallFunctionName(context, _obj[0],
								 "toString", 0, NULL, &(_v[0]))) {
			fprintf(stderr,
					"JS_CallFunctionName failed in VrmlBrowserCreateVrmlFromUrl.\n");
			return JS_FALSE;
		}
		_str[0] = JS_ValueToString(context, _v[0]);
		_costr[0] = JS_GetStringBytes(_str[0]);

		if (!JS_GetProperty(context, _obj[1], "__handle", &(_v[1]))) {
			fprintf(stderr,
					"JS_GetProperty failed for \"__handle\" in VrmlBrowserCreateVrmlFromUrl.\n");
			return JS_FALSE;
		}
		_str[1] = JS_ValueToString(context, _v[1]);
		_costr[1] = JS_GetStringBytes(_str[1]);
		doPerlCallMethodVA(brow->sv_js,
						   "browserCreateVrmlFromUrl", "sss",
						   _costr[0], _costr[1], _c);
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for createVrmlFromUrl(%s).\n",
				_c_args);
		return JS_FALSE;
	}

	if (!JS_GetProperty(context, obj, "__bret",  &_rval)) {
		fprintf(stderr,
				"JS_GetProperty failed in VrmlBrowserCreateVrmlFromUrl.\n");
	}
	*rval = _rval;
	return JS_TRUE;
}


JSBool
VrmlBrowserSetDescription(JSContext *context, JSObject *obj,
						  uintN argc, jsval *argv, jsval *rval)
{
	jsval _v;
	char *_c, *_c_args = "SFString description", *_c_format = "s";
	BrowserNative *brow;
	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr,
				"JS_GetPrivate failed in VrmlBrowserSetDescription.\n");
		return JS_FALSE;
	}

	if (brow->magic != BROWMAGIC) {
		fprintf(stderr, "Wrong browser magic!\n");
		return JS_FALSE;
	}

	if (argc == 1 &&
		JS_ConvertArguments(context, argc, argv, _c_format, &_c)) {
		doPerlCallMethodVA(brow->sv_js, "browserSetDescription", "s", _c);

		if (!JS_GetProperty(context, obj, "__bret",  &_v)) {
			fprintf(stderr,
					"JS_GetProperty failed in VrmlBrowserSetDescription.\n");
		}
		*rval = _v;
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for setDescription(%s).\n",
				_c_args);
		return JS_FALSE;
	}
	return JS_TRUE;
}

		
JSBool
VrmlBrowserGetName(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval _rval;
/* 	unsigned int i; */
	BrowserNative *brow;
	UNUSED(argc);
	UNUSED(argv);

	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in VrmlBrowserGetName.\n");
		return JS_FALSE;
	}

	if (brow->magic != BROWMAGIC) {
		fprintf(stderr, "Wrong browser magic!\n");
		return JS_FALSE;
	}

	doPerlCallMethod(brow->sv_js, "browserGetName");

	if (!JS_GetProperty(context, obj, "__bret",  &_rval)) {
		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserGetName.\n");
	}
	
	*rval = _rval;
	return JS_TRUE;
}

		
#if FALSE		
/* JSBool */
/* VrmlBrowserLoadURL(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) */
/* { */
/* 	int count; */
/* 	SV *sv; */
/* 	jsval v; */
/* 	unsigned int i; */
/* 	BrowserNative *brow = JS_GetPrivate(context, obj); */
/* 	UNUSED(count); */
/* 	UNUSED(sv); */

/* 	if (!brow) { */
/* 		return JS_FALSE; */
/* 	} */
	
/* 	if (brow->magic != BROWMAGIC) { */
/* 		fprintf(stderr, "Wrong browser magic!\n"); */
/* 	} */
/* 	if (argc != 2) { */
/* 		fprintf(stderr, "Invalid number of arguments for browser method!\n"); */
/* 	} */
/* 	for (i = 0; i < argc; i++) { */
/* 		char buffer[SMALLSTRING]; */
/* 		sprintf(buffer,"__arg%d", i); */
/* 		JS_SetProperty(context, obj, buffer, argv+i); */
/* 	} */
/* 	if (verbose) { */
/* 		printf("Calling method with sv %u (%s)\n", (unsigned int) brow->sv_js, SvPV(brow->sv_js, PL_na)); */
/* 	} */

/* 	{ */
/* 		dSP; */
/* 		ENTER; */
/* 		SAVETMPS; */
/* 		PUSHMARK(sp); */
/* 		XPUSHs(brow->sv_js); */
/* 		PUTBACK; */
/* 		count = perl_call_method("brow_loadURL",  G_SCALAR); */
/* 		if (count) { */
/* 			if (verbose) printf("Got return %f\n", POPn); */
/* 		} */
/* 		PUTBACK; */
/* 		FREETMPS; */
/* 		LEAVE; */
/* 	} */

/* 	if (!JS_GetProperty(context, obj, "__bret", &v)) { */
/* 		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserLoadURL.\n"); */
		/* exit(1); */
/* 	} */
	
/* 	*rval = v; */
/* 	return JS_TRUE; */
/* } */
#endif /* FALSE */

		
#if FALSE		
/* JSBool */
/* VrmlBrowserReplaceWorld(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) */
/* { */
/* 	int count; */
/* 	SV *sv; */
/* 	jsval v; */
/* 	unsigned int i; */
/* 	BrowserNative *brow = JS_GetPrivate(context, obj); */
/* 	UNUSED(count); */
/* 	UNUSED(sv); */

/* 	if (!brow) { */
/* 		return JS_FALSE; */
/* 	} */
	
/* 	if (brow->magic != BROWMAGIC) { */
/* 		fprintf(stderr, "Wrong browser magic!\n"); */
/* 	} */
/* 	if (argc != 1) { */
/* 		fprintf(stderr, "Invalid number of arguments for browser method!\n"); */
/* 	} */
/* 	for (i = 0; i < argc; i++) { */
/* 		char buffer[SMALLSTRING]; */
/* 		sprintf(buffer,"__arg%d", i); */
/* 		JS_SetProperty(context, obj, buffer, argv+i); */
/* 	} */
/* 	if (verbose) { */
/* 		printf("Calling method with sv %u (%s)\n", (unsigned int) brow->sv_js, SvPV(brow->sv_js, PL_na)); */
/* 	} */

/* 	{ */
/* 		dSP; */
/* 		ENTER; */
/* 		SAVETMPS; */
/* 		PUSHMARK(sp); */
/* 		XPUSHs(brow->sv_js); */
/* 		PUTBACK; */
/* 		count = perl_call_method("brow_replaceWorld",  G_SCALAR); */
/* 		if (count) { */
/* 			if (verbose) printf("Got return %f\n", POPn); */
/* 		} */
/* 		PUTBACK; */
/* 		FREETMPS; */
/* 		LEAVE; */
/* 	} */


/* 	if (!JS_GetProperty(context, obj, "__bret",  &v)) { */
/* 		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserReplaceWorld.\n"); */
		/* exit(1); */
/* 	} */
/* 	*rval = v; */
/* 	return JS_TRUE; */
/* } */
#endif /* FALSE */

	
#if FALSE		
/* JSBool */
/* VrmlBrowserGetCurrentSpeed(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) */
/* { */
/* 	int count; */
/* 	SV *sv; */
/* 	jsval v; */
/* 	unsigned int i; */
/* 	BrowserNative *brow = JS_GetPrivate(context, obj); */
/* 	UNUSED(count); */
/* 	UNUSED(sv); */

/* 	if (!brow) { */
/* 		return JS_FALSE; */
/* 	} */
	
/* 	if (brow->magic != BROWMAGIC) { */
/* 		fprintf(stderr, "Wrong browser magic!\n"); */
/* 	} */
/* 	if (argc != 0) { */
/* 		fprintf(stderr, "Invalid number of arguments for browser method!\n"); */
/* 	} */
/* 	for (i = 0; i < argc; i++) { */
/* 		char buffer[SMALLSTRING]; */
/* 		sprintf(buffer,"__arg%d", i); */
/* 		JS_SetProperty(context, obj, buffer, argv+i); */
/* 	} */
/* 	if (verbose) { */
/* 		printf("Calling method with sv %u (%s)\n", (unsigned int) brow->sv_js, SvPV(brow->sv_js, PL_na)); */
/* 	} */

/* 	{ */
/* 		dSP; */
/* 		ENTER; */
/* 		SAVETMPS; */
/* 		PUSHMARK(sp); */
/* 		XPUSHs(brow->sv_js); */
/* 		PUTBACK; */
/* 		count = perl_call_method("brow_getCurrentSpeed",  G_SCALAR); */
/* 		if (count) { */
/* 			if (verbose) printf("Got return %f\n", POPn); */
/* 		} */
/* 		PUTBACK; */
/* 		FREETMPS; */
/* 		LEAVE; */
/* 	} */


/* 	if (!JS_GetProperty(context, obj, "__bret",  &v)) { */
/* 		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserGetCurrentSpeed.\n"); */
		/* exit(1); */
/* 	} */

/* 	*rval = v; */
/* 	return JS_TRUE; */
/* } */
#endif /* FALSE */

		
JSBool
VrmlBrowserGetVersion(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int count;
	jsval v;
	unsigned int i;
	BrowserNative *brow;
	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in VrmlBrowserGetVersion.\n");
		return JS_FALSE;
	}
	
	UNUSED(count);

	if (brow->magic != BROWMAGIC) {
		fprintf(stderr, "Wrong browser magic!\n");
		return JS_FALSE;
	}
	if (argc != 0) {
		fprintf(stderr, "Invalid number of arguments for browser method!\n");
		return JS_FALSE;
	}
	for (i = 0; i < argc; i++) {
		char buffer[SMALLSTRING];
		sprintf(buffer,"__arg%d", i);
		JS_SetProperty(context, obj, buffer, argv+i);
	}

	doPerlCallMethod(brow->sv_js, "browserGetVersion");

	if (!JS_GetProperty(context, obj, "__bret",  &v)) {
		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserGetVersion.\n");
	}
	*rval = v;
	return JS_TRUE;
}

JSBool
doVRMLRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv,
			const char *callingFunc, const char *perlBrowserFunc, const char *browserFunc)
{
	jsval _v[2];
	BrowserNative *brow;
	JSObject *_obj[4];
	JSClass *_cls[2];
	JSString *_str[2];
	char *_route,
		*_cstr[2],
		*_costr[2],
		*_c_args =
		"SFNode fromNode, SFString fromEventOut, SFNode toNode, SFString toEventIn",
		*_c_format = "o s o s";
	size_t len;


	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr,
				"JS_GetPrivate failed in doVRMLRoute called from %s.\n",
				callingFunc);
		return JS_FALSE;
	}
	if (brow->magic != BROWMAGIC) {
		fprintf(stderr,
				"Wrong browser magic doVRMLRoute called from %s!\n",
				callingFunc);
		return JS_FALSE;
	}

	if (JS_ConvertArguments(context,
							argc,
							argv,
							_c_format,
							&(_obj[0]), &(_cstr[0]), &(_obj[1]), &(_cstr[1]))) {
		if ((_cls[0] = JS_GetClass(_obj[0])) == NULL) {
			fprintf(stderr,
					"JS_GetClass failed for arg 0 in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GetClass(_obj[1])) == NULL) {
			fprintf(stderr,
					"JS_GetClass failed for arg 2 in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		if (memcmp("SFNode", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("SFNode", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			fprintf(stderr,
					"\nArguments 0 and 2 must be SFNode in doVRMLRoute called from %s(%s): %s\n",
					browserFunc, _c_args, callingFunc);
			return JS_FALSE;
		}

		if (!JS_GetProperty(context, _obj[0], "__handle", &(_v[0]))) {
			fprintf(stderr,
					"JS_GetProperty failed for arg 0 and \"__handle\" in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		_str[0] = JS_ValueToString(context, _v[0]);
		_costr[0] = JS_GetStringBytes(_str[0]);

		if (!JS_GetProperty(context, _obj[1], "__handle", &(_v[1]))) {
			fprintf(stderr,
					"JS_GetProperty failed for arg 2 and \"__handle\" in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		_str[1] = JS_ValueToString(context, _v[1]);
		_costr[1] = JS_GetStringBytes(_str[1]);

		len = strlen(_costr[0]) + strlen(_cstr[0]) +
			strlen(_costr[1]) + strlen(_cstr[1]) + 7;
		_route = JS_malloc(context, len * sizeof(char *));
		sprintf(_route, "%s %s %s %s",
				_costr[0], _cstr[0],
				_costr[1], _cstr[1]);

		doPerlCallMethodVA(brow->sv_js, perlBrowserFunc, "s", _route);
		JS_free(context, _route);
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for %s(%s).\n",
				callingFunc, _c_args);
		return JS_FALSE;
	}

	return JS_TRUE;
}

	
JSBool
VrmlBrowserAddRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval _v;
	if (!doVRMLRoute(context, obj, argc, argv,
					 "VrmlBrowserAddRoute", "browserAddRoute", "addRoute")) {
		fprintf(stderr, "doVRMLRoute failed in VrmlBrowserAddRoute.\n");
		return JS_FALSE;
	}
	if (!JS_GetProperty(context, obj, "__bret",  &_v)) {
		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserAddRoute.\n");
	}
	*rval = _v;
	return JS_TRUE;
}

JSBool
VrmlBrowserDeleteRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval _v;
	if (!doVRMLRoute(context, obj, argc, argv,
					 "VrmlBrowserDeleteRoute", "browserDeleteRoute", "deleteRoute")) {
		fprintf(stderr, "doVRMLRoute failed in VrmlBrowserDeleteRoute.\n");
		return JS_FALSE;
	}
	if (!JS_GetProperty(context, obj, "__bret",  &_v)) {
		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserDeleteRoute.\n");
		return JS_FALSE;
	}
	*rval = _v;
	return JS_TRUE;
}

	
JSBool
VrmlBrowserGetCurrentFrameRate(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int count;
	jsval v;
	unsigned int i;
	BrowserNative *brow;
	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in VrmlBrowserGetCurrentFrameRate.\n");
		return JS_FALSE;
	}
	
	UNUSED(count);
	
	if (brow->magic != BROWMAGIC) {
		fprintf(stderr, "Wrong browser magic!\n");
		return JS_FALSE;
	}
	if (argc != 0) {
		fprintf(stderr, "Invalid number of arguments for browser method!\n");
		return JS_FALSE;
	}
	for (i = 0; i < argc; i++) {
		char buffer[SMALLSTRING];
		sprintf(buffer,"__arg%d", i);
		JS_SetProperty(context, obj, buffer, argv+i);
	}

	doPerlCallMethod(brow->sv_js, "browserGetCurrentFrameRate");

	if (!JS_GetProperty(context, obj, "__bret",  &v)) {
		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserGetCurrentFrameRate.\n");
	}
	*rval = v;
	return JS_TRUE;
}

	
JSBool
VrmlBrowserGetWorldURL(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int count;
	jsval v;
	unsigned int i;
	BrowserNative *brow;
	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in VrmlBrowserGetWorldURL.\n");
		return JS_FALSE;
	}
	
	UNUSED(count);

	if (brow->magic != BROWMAGIC) {
		fprintf(stderr, "Wrong browser magic!\n");
		return JS_FALSE;
	}

	if (argc != 0) {
		fprintf(stderr, "Invalid number of arguments for browser method!\n");
		return JS_FALSE;
	}
	for (i = 0; i < argc; i++) {
		char buffer[SMALLSTRING];
		sprintf(buffer,"__arg%d", i);
		JS_SetProperty(context, obj, buffer, argv+i);
	}

	doPerlCallMethod(brow->sv_js, "browserGetWorldURL");

	if (!JS_GetProperty(context, obj, "__bret",  &v)) {
		fprintf(stderr, "JS_GetProperty failed in VrmlBrowserGetWorldURL.\n");
	}

	*rval = v;
	return JS_TRUE;
}

