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

char FPSstring[10];

JSBool
VrmlBrowserInit(JSContext *context, JSObject *globalObj, BrowserNative *brow)
{
	JSObject *obj;
	/* BrowserNative *brow = (BrowserNative *) JS_malloc(context, sizeof(BrowserNative)); */
	/* brow->sv_js = newSVsv(sv_js); */ /* new duplicate of sv_js */
	/* brow->magic = BROWMAGIC; */

	if (JSVerbose) {
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
		fprintf(stderr,
				"JS_DefineFunctions failed in VrmlBrowserInit.\n");
		return JS_FALSE;
	}

	if (!JS_SetPrivate(context, obj, brow)) {
		fprintf(stderr,
				"JS_SetPrivate failed in VrmlBrowserInit.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
VrmlBrowserGetName(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;

	_str = JS_NewString(context,BrowserName,strlen(BrowserName));
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


/* get the string stored in global BrowserVersion into a jsObject */
JSBool
VrmlBrowserGetVersion(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;

	_str = JS_NewString(context,BrowserVersion,strlen(BrowserVersion));
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


JSBool
VrmlBrowserGetCurrentSpeed(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;

	/* 0.0 is a valid return for this one. */
	_str = JS_NewString(context,"0.0",strlen("0.0"));
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


JSBool
VrmlBrowserGetCurrentFrameRate(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;

	sprintf (FPSstring,"%6.2f",BrowserFPS);
	_str = JS_NewString(context,FPSstring,strlen(FPSstring));
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


JSBool
VrmlBrowserGetWorldURL(JSContext *context, JSObject *obj,
					   uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;

	_str = JS_NewString(context,BrowserURL,strlen(BrowserURL));
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


JSBool
VrmlBrowserReplaceWorld(JSContext *context, JSObject *obj,
						uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj;
	JSString *_str;
	JSClass *_cls;
	jsval _v, _rval = INT_TO_JSVAL(0);
	BrowserNative *brow;
	char *_c_args = "MFNode nodes",
		*_costr,
		*_c_format = "o";

	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr,
				"JS_GetPrivate failed in VrmlBrowserReplaceWorld.\n");
		return JS_FALSE;
	}
	
	if (brow->magic != BROWMAGIC) {
		fprintf(stderr, "Wrong browser magic!\n");
		return JS_FALSE;
	}

	if (JS_ConvertArguments(context, argc, argv, _c_format, &_obj)) {
		if ((_cls = JS_GetClass(_obj)) == NULL) {
			fprintf(stderr,
					"JS_GetClass failed in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}

		if (memcmp("MFNode", _cls->name, strlen(_cls->name)) != 0) {
			fprintf(stderr,
					"\nIncorrect argument in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}
		if (!JS_GetProperty(context, _obj, "__handle", &_v)) {
			fprintf(stderr,
					"JS_GetProperty failed for \"__handle\" in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}
		_str = JS_ValueToString(context, _v);
		_costr = JS_GetStringBytes(_str);

		doPerlCallMethodVA(brow->sv_js,
						   "jspBrowserReplaceWorld",
						   "s",
						   _costr);
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for replaceWorld(%s).\n",
				_c_args);
		return JS_FALSE;
	}
	*rval = _rval;

	return JS_TRUE;
}


JSBool
VrmlBrowserLoadURL(JSContext *context, JSObject *obj,
				   uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj[2];
	JSString *_str[2];
	JSClass *_cls[2];
	jsval _rval = INT_TO_JSVAL(0);
	BrowserNative *brow;
	char *_c_args = "MFString url, MFString parameter",
		*_costr[2],
		*_c_format = "o o";

	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr,
				"JS_GetPrivate failed in VrmlBrowserLoadURL.\n");
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
							&(_obj[0]), &(_obj[1]))) {
		if ((_cls[0] = JS_GetClass(_obj[0])) == NULL) {
			fprintf(stderr,
					"JS_GetClass failed for arg 0 in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GetClass(_obj[1])) == NULL) {
			fprintf(stderr,
					"JS_GetClass failed for arg 1 in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		if (memcmp("MFString", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("MFString", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			fprintf(stderr,
					"\nIncorrect arguments in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		_str[0] = JS_ValueToString(context, argv[0]);
		_costr[0] = JS_GetStringBytes(_str[0]);

		_str[1] = JS_ValueToString(context, argv[1]);
		_costr[1] = JS_GetStringBytes(_str[1]);
		doPerlCallMethodVA(brow->sv_js,
						   "jspBrowserLoadURL", "ss",
						   _costr[0], _costr[1]);
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for loadURL(%s).\n",
				_c_args);
		return JS_FALSE;
	}
	*rval = _rval;

	return JS_TRUE;
}


JSBool
VrmlBrowserSetDescription(JSContext *context, JSObject *obj,
						  uintN argc, jsval *argv, jsval *rval)
{
	jsval _rval = INT_TO_JSVAL(0);
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
		doPerlCallMethodVA(brow->sv_js, "jspBrowserSetDescription", "s", _c);
		*rval = _rval;
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for setDescription(%s).\n",
				_c_args);
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
VrmlBrowserCreateVrmlFromString(JSContext *context, JSObject *obj,
								uintN argc, jsval *argv, jsval *rval)
{
	jsval _rval;
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
		if (JSVerbose) {
			printf("VrmlBrowserCreateVrmlFromString: obj = %u, str = \"%s\"\n",
				   (unsigned int) obj, _c);
		}

		doPerlCallMethodVA(brow->sv_js, "jspBrowserCreateVrmlFromString", "s", _c);
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for createVrmlFromString(%s).\n",
				_c_args);
		return JS_FALSE;
	}

	if (!JS_GetProperty(context, obj, BROWSER_RETVAL,  &_rval)) {
		fprintf(stderr,
				"JS_GetProperty failed in VrmlBrowserCreateVrmlFromString.\n");
		return JS_FALSE;
	}
	*rval = _rval;

	return JS_TRUE;
}


JSBool
VrmlBrowserCreateVrmlFromURL(JSContext *context, JSObject *obj,
							 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj[2];
	JSString *_str[2];
	JSClass *_cls[2];
	jsval _v, _rval = INT_TO_JSVAL(0);
	BrowserNative *brow;
	char *_c,
		*_c_args = "MFString url, SFNode node, SFString event",
		*_costr[2],
		*_c_format = "o o s";

	if ((brow = JS_GetPrivate(context, obj)) == NULL) {
		fprintf(stderr,
				"JS_GetPrivate failed in VrmlBrowserCreateVrmlFromURL.\n");
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
					"JS_GetClass failed for arg 0 in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GetClass(_obj[1])) == NULL) {
			fprintf(stderr,
					"JS_GetClass failed for arg 1 in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}
		if (memcmp("MFString", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("SFNode", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			fprintf(stderr,
					"\nIncorrect arguments in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}

		_str[0] = JS_ValueToString(context, argv[0]);
		_costr[0] = JS_GetStringBytes(_str[0]);

		if (!JS_GetProperty(context, _obj[1], "__handle", &_v)) {
			fprintf(stderr,
					"JS_GetProperty failed for \"__handle\" in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}
		_str[1] = JS_ValueToString(context, _v);
		_costr[1] = JS_GetStringBytes(_str[1]);
		doPerlCallMethodVA(brow->sv_js,
						   "jspBrowserCreateVrmlFromURL", "sss",
						   _costr[0], _costr[1], _c);
	} else {
		fprintf(stderr,
				"\nIncorrect argument format for createVrmlFromURL(%s).\n",
				_c_args);
		return JS_FALSE;
	}
	*rval = _rval;

	return JS_TRUE;
}


JSBool
VrmlBrowserAddRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval _rval = INT_TO_JSVAL(0);
	if (!doVRMLRoute(context, obj, argc, argv,
					 "VrmlBrowserAddRoute", "jspBrowserAddRoute", "addRoute")) {
		fprintf(stderr, "doVRMLRoute failed in VrmlBrowserAddRoute.\n");
		return JS_FALSE;
	}
	*rval = _rval;
	return JS_TRUE;
}


JSBool
VrmlBrowserDeleteRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval _rval = INT_TO_JSVAL(0);
	if (!doVRMLRoute(context, obj, argc, argv,
					 "VrmlBrowserDeleteRoute", "jspBrowserDeleteRoute", "deleteRoute")) {
		fprintf(stderr, "doVRMLRoute failed in VrmlBrowserDeleteRoute.\n");
		return JS_FALSE;
	}
	*rval = _rval;
	return JS_TRUE;
}


static JSBool
doVRMLRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv,
			const char *callingFunc, const char *perlBrowserFunc, const char *browserFunc)
{
	jsval _v[2];
	BrowserNative *brow;
	JSObject *_obj[2];
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
