/*
 * Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart CRC Canada
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License
 * (file COPYING in the distribution) for conditions of use and
 * redistribution, EXCEPT on the files which belong under the
 * Mozilla public license.
 * 
 * $Id$
 * 
 */

#include "jsVRMLClasses.h"

JSBool
globalResolve(JSContext *cx, JSObject *obj, jsval id) 
{
	UNUSED(cx);
	UNUSED(obj);
	UNUSED(id);
	return JS_TRUE;
}

JSBool
doMFAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsval v;
	jsval myv;
	char *p, *pp;
	JSString *str, *sstr;
	int len = 0, ind = JSVAL_TO_INT(id);

	str = JS_ValueToString(cx, id);
	p = JS_GetStringBytes(str);
	if (verbose) {
		printf("\tdoMFAddProperty: ");
	}

	if (!strcmp(p, "length") || !strcmp(p, "constructor") ||
		!strcmp(p, "assign") || !strcmp(p, "__touched_flag")) {
		if (verbose) {
			printf("property \"%s\" is one of \"length\", \"constructor\", \"assign\", \"__touched_flag\". Do nothing.\n", p);
		}
		return JS_TRUE;
	}

	sstr = JS_ValueToString(cx, *vp);
	pp = JS_GetStringBytes(sstr);

	if (verbose) {
		printf("adding property %s, %s to object %u, ",
			   p, pp, (unsigned int) obj);
	}
	if (!JSVAL_IS_INT(id)){ 
		fprintf(stderr, "JSVAL_IS_INT failed for id in doMFAddProperty.\n");
		return JS_FALSE;
	}
	if (!JS_GetProperty(cx, obj, "length", &v)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in doMFAddProperty.\n");
		return JS_FALSE;
	}
	
	len = JSVAL_TO_INT(v);
	if (verbose) {
		printf("index = %d, length = %d\n", ind, len);
	}
	if (ind >= len) {
		len = ind + 1;
		v = INT_TO_JSVAL(len);
		if (!JS_SetProperty(cx, obj, "length", &v)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"length\" in doMFAddProperty.\n");
			return JS_FALSE;
		}
	}
	myv = INT_TO_JSVAL(1);
	if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in doMFAddProperty.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
doMFSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsval myv;
	JSString *str, *sstr;
	char *p, *pp;

	str = JS_ValueToString(cx, id);
	p = JS_GetStringBytes(str);

	sstr = JS_ValueToString(cx, *vp);
	pp = JS_GetStringBytes(str);

	if (verbose) {
		printf("\tsetting property %s for object %u, %s\n", pp, (unsigned int) obj, p);
	}
	if (JSVAL_IS_INT(id)) {
		myv = INT_TO_JSVAL(1);
		if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"__touched_flag\" in doMFSetProperty.\n");
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}


JSBool
loadVrmlClasses(JSContext *context, JSObject *globalObj)
{
	jsval v = 0;

	if ((proto_SFColor = JS_InitClass(context, globalObj, NULL, &SFColorClass,
									  SFColorConstr, INIT_ARGC, NULL,
									  SFColorFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFColor);
	if (!JS_SetProperty(context, globalObj, "__SFColor_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;
	
	if ((proto_SFVec2f = JS_InitClass(context, globalObj, NULL, &SFVec2fClass,
									  SFVec2fConstr, INIT_ARGC_ROT, NULL,
									  SFVec2fFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFVec2fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFVec2f);
	if (!JS_SetProperty(context, globalObj, "__SFVec2f_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for SFVec2fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;
	
	if ((proto_SFVec3f = JS_InitClass(context, globalObj, NULL, &SFVec3fClass,
									  SFVec3fConstr, INIT_ARGC, NULL,
									  SFVec3fFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFVec3fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFVec3f);
	if (!JS_SetProperty(context, globalObj, "__SFVec3f_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for SFVec3fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_SFRotation = JS_InitClass(context, globalObj, NULL, &SFRotationClass,
										 SFRotationConstr, 0,
										 NULL, SFRotationFunctions, NULL,
										 NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFRotationClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFRotation);
	if (!JS_SetProperty(context, globalObj, "__SFRotation_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for SFRotationClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;


	if ((proto_SFImage = JS_InitClass(context, globalObj, NULL, &SFImageClass,
									  SFImageConstr, INIT_ARGC_IMG, NULL,
									  SFImageFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFImageClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFImage);
	if (!JS_SetProperty(context, globalObj, "__SFImage_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for SFImageClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFColor = JS_InitClass(context, globalObj, NULL,
									  &MFColorClass, MFColorConstr, INIT_ARGC,
									  NULL, MFColorFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFColor);
	if (!JS_SetProperty(context, globalObj, "__MFColor_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFVec2f = JS_InitClass(context, globalObj, NULL,
									  &MFVec2fClass, MFVec2fConstr, INIT_ARGC,
									  NULL, MFVec2fFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFVec2fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFVec2f);
	if (!JS_SetProperty(context, globalObj, "__MFVec2f_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFVec2fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFVec3f = JS_InitClass(context, globalObj, NULL,
									  &MFVec3fClass, MFVec3fConstr, INIT_ARGC,
									  NULL, MFVec3fFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFVec3fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFVec3f);
	if (!JS_SetProperty(context, globalObj, "__MFVec3f_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFVec3fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFRotation = JS_InitClass(context, globalObj, NULL,
										 &MFRotationClass, MFRotationConstr, INIT_ARGC,
										 NULL, MFRotationFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFRotationClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFRotation);
	if (!JS_SetProperty(context, globalObj, "__MFRotation_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFRotationClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;
	
	if ((proto_MFNode = JS_InitClass(context, globalObj, NULL,
									 &MFNodeClass, MFNodeConstr, INIT_ARGC,
									 NULL, MFNodeFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFNodeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFNode);
	if (!JS_SetProperty(context, globalObj, "__MFNode_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFNodeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFString = JS_InitClass(context, globalObj, NULL,
									   &MFStringClass, MFStringConstr, INIT_ARGC,
									   NULL, MFStringFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFString failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFString);
	if (!JS_SetProperty(context, globalObj, "__MFString_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFStringClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_SFNode = JS_InitClass(context, globalObj, NULL,
									 &SFNodeClass, SFNodeConstr, INIT_ARGC,
									 NULL, SFNodeFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFNodeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFNode);
	if (!JS_SetProperty(context, globalObj, "__SFNode_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for SFNodeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	return JS_TRUE;
}


JSBool
setTouchable(JSContext *context, JSObject *obj, jsval id, jsval *vp)
{
	jsval v;
	char buffer[STRING];
	char *n = JS_GetStringBytes(JSVAL_TO_STRING(id));
	UNUSED(vp);
	
	if (verbose) {
		printf("SetTouchable %s\n", n);
	}

	memset(buffer, 0, STRING);
	sprintf(buffer, "_%s_touched", n);
	v = INT_TO_JSVAL(1);
	if (!JS_SetProperty(context, obj, buffer, &v)) {
		fprintf(stderr,
				"JS_SetProperty failed in SetTouchable.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}

JSBool
getAssignProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	char *c;
	UNUSED(cx);
	UNUSED(vp);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("getAssignProperty: obj = %u, id = %s\n",
				   (unsigned int) obj, c);
		}
	}
	return JS_TRUE;
}

/* a kind of hack to replace the use of JSPROP_ASSIGNHACK */
JSBool
setAssignProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsval _newVal, _initVal, _argv[1];
	JSObject *_o;
	JSString *_str;
	char *_c;

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (!JS_ConvertValue(cx, *vp, JSTYPE_OBJECT, &_newVal)) {
			fprintf(stderr, "JS_ConvertValue failed in setAssignProperty.\n");
			return JS_FALSE;
		}
		if (!JS_GetProperty(cx, obj, _c, &_initVal)) {
			fprintf(stderr,
					"JS_GetProperty failed in setAssignProperty.\n");
			return JS_FALSE;
		}
		if (verbose) {
			printf("setAssignProperty: obj = %u, id = %s, _newVal = %ld, _initVal = %ld\n",
				   (unsigned int) obj, _c, _newVal, _initVal);
		}
		_o = JSVAL_TO_OBJECT(_initVal);
		_argv[0] = _newVal;
		if (!JS_CallFunctionName(cx, _o, "assign", 1, _argv, vp)) {
			fprintf(stderr,
					"JS_CallFunctionName failed in setAssignProperty.\n");
			return JS_FALSE;
		}
	}

	return JS_TRUE;
}


JSBool
SFColorGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsdouble d, *dp;
	SFColorNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFColorGetProperty.\n");
		return JS_FALSE;
	}
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			d = (ptr->v).c[0];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFColorGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break;
		case 1:
			d = (ptr->v).c[1];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFColorGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break;
		case 2:
			d = (ptr->v).c[2];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFColorGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break;
		}
	}
	return JS_TRUE;
}

JSBool
SFColorSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsval _val;
	SFColorNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFColorSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (verbose) {
		printf("SFColorSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}

	if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &_val)) {
		fprintf(stderr, "JS_ConvertValue failed in SFColorSetProperty.\n");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			(ptr->v).c[0] = *JSVAL_TO_DOUBLE(_val);
			break;
		case 1:
			(ptr->v).c[1] = *JSVAL_TO_DOUBLE(_val);
			break;
		case 2:
			(ptr->v).c[2] = *JSVAL_TO_DOUBLE(_val);
			break;

		}
	}
	return JS_TRUE;
}


JSBool
SFColorToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *_str;
	char _buff[STRING];
    SFColorNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFColorToString.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFColorToString.\n");
        return JS_FALSE;
	}
    
	memset(_buff, 0, STRING);
	sprintf(_buff, "%f %f %f",
			(ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	_str = JS_NewStringCopyZ(cx, _buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}


JSBool
SFColorAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *o;
    SFColorNative *ptr, *fptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFColorAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFColorAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFColorAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &SFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in SFColorAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for o in SFColorAssign.\n");
        return JS_FALSE;
	}
	if (verbose) {
		printf("SFColorAssign: obj = %u, o = %u, %u args\n",
			   (unsigned int) obj, (unsigned int) o, argc);
	}

    SFColorNativeAssign(ptr, fptr);
    *rval = OBJECT_TO_JSVAL(obj); 

    return JS_TRUE;
}

JSBool
SFColorTouched(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int t;
    SFColorNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFSFColorTouched.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFColorTouched.\n");
        return JS_FALSE;
	}
    t = ptr->touched;
	ptr->touched = 0;
    if (verbose) {
		printf("SFColorTouched: touched = %d\n", t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFColorConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble pars[3];
	SFColorNative *ptr;

	if ((ptr = (SFColorNative *) SFColorNativeNew()) == NULL) {
		fprintf(stderr, "SFColorNativeNew failed in SFColorConstr.\n");
		return JS_FALSE;
	}
	UNUSED(rval);

	if (!JS_DefineProperties(cx, obj, SFColorProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFColorConstr.\n");
		return JS_FALSE;
	}

	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFColorConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).c[0] = 0;
		(ptr->v).c[1] = 0;
		(ptr->v).c[2] = 0;
	} else {
		if (!JS_ConvertArguments(cx, argc, argv, "d d d",
								 &(pars[0]), &(pars[1]), &(pars[2]))) {
			fprintf(stderr,
					"JS_ConvertArguments failed in SFColorConstr.\n");
			return JS_FALSE;
		}
		(ptr->v).c[0] = pars[0];
		(ptr->v).c[1] = pars[1];
		(ptr->v).c[2] = pars[2];
	}
	if (verbose) {
		printf("SFColorConstr: obj = %u, %d args, %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	}
		
	return JS_TRUE;
}

JSBool
SFColorFinalize(JSContext *cx, JSObject *obj)
{
	SFColorNative *ptr;

	if (verbose) {
		printf("SFColorFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFColorFinalize.\n");
		return JS_FALSE;
	}
	SFColorNativeDelete(ptr);
	return JS_TRUE;
}


JSBool
SFImageGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsint i;
	SFImageNative *ptr;
	char *c;
	JSString *_str;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFImageGetProperty.\n");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			i = (ptr->v).__x;
			*vp = INT_TO_JSVAL(i);
			break;
		case 1:
			i = (ptr->v).__y;
			*vp = INT_TO_JSVAL(i);
			break;
		case 2:
			i = (ptr->v).__depth;
			*vp = INT_TO_JSVAL(i);
			break;
		case 3:
			c = (ptr->v).__data;
			_str = JS_NewStringCopyZ(cx, c);
			*vp = STRING_TO_JSVAL(_str);
		case 4:
			i = (ptr->v).__texture;
			*vp = INT_TO_JSVAL(i);
			break;
		}
	}
	return JS_TRUE;
}

JSBool
SFImageSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsval myv;
	SFImageNative *ptr;
	JSString *_str;
	char *c;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFImageSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (verbose) {
		printf("SFImageSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}
	if (JSVAL_IS_NUMBER(*vp)) { /* succeeds for int and double */
		if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &myv)) {
			fprintf(stderr,
					"JS_ConvertValue failed for vp as JSTYPE_NUMBER in SFImageSetProperty.\n");
			return JS_FALSE;
		}
	} else if (JSVAL_IS_STRING(*vp)) {
		if (!JS_ConvertValue(cx, *vp, JSTYPE_STRING, &myv)) {
			fprintf(stderr,
					"JS_ConvertValue failed for vp as JSTYPE_STRING in SFImageSetProperty.\n");
			return JS_FALSE;
		}
	} else {
			fprintf(stderr,
					"Arg vp is not one of JSTYPE_NUMBER or JSTYPE_STRING.\n");
			return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			(ptr->v).__x = JSVAL_TO_INT(myv);
			break;
		case 1:
			(ptr->v).__y = JSVAL_TO_INT(myv);
			break;
		case 2:
			(ptr->v).__depth = JSVAL_TO_INT(myv);
			break;
		case 3:
			_str = JSVAL_TO_STRING(myv);
			c = JS_GetStringBytes(_str);
			strcpy((ptr->v).__data, c);
		case 4:
			(ptr->v).__texture = JSVAL_TO_INT(myv);
			break;
		}
	}
	return JS_TRUE;
}


JSBool
SFImageToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *_str;
	char buff[LARGESTRING];
    SFImageNative *ptr;
	UNUSED(argc);

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFImageToString.\n");
		return JS_FALSE;
	}

    if (!JS_InstanceOf(cx, obj, &SFImageClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFImageToString.\n");
        return JS_FALSE;
	}
    
	memset(buff, 0, LARGESTRING);
	sprintf(buff, "%d %d %d %s %d",
			(ptr->v).__x, (ptr->v).__y,
			(ptr->v).__depth, (ptr->v).__data, (ptr->v).__texture);
	_str = JS_NewStringCopyZ(cx, buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}

JSBool
SFImageAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *o;
    SFImageNative *ptr, *fptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFImageAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFImageClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFImageAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFImageAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &SFImageClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in SFImageAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for o in SFImageAssign.\n");
        return JS_FALSE;
	}
	if (verbose) {
		printf("SFImageAssign: obj = %u, o = %u, %u args\n",
			   (unsigned int) obj, (unsigned int) o, argc);
	}

    SFImageNativeAssign(ptr, fptr);
    *rval = OBJECT_TO_JSVAL(obj); 

    return JS_TRUE;
}

JSBool
SFImageTouched(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int t;
    SFImageNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFSFImageTouched.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFImageClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFImageTouched.\n");
        return JS_FALSE;
	}
    t = ptr->touched;
	ptr->touched = 0;
    if (verbose) {
		printf("SFImageTouched: touched = %d\n", t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFImageConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsint pars[4];
	char *cpars = 0;
	SFImageNative *ptr;

	if ((ptr = (SFImageNative *) SFImageNativeNew()) == NULL) {
		fprintf(stderr, "SFImageNativeNew failed in SFImageConstr.\n");
		return JS_FALSE;
	}
	UNUSED(rval);

	if (!JS_DefineProperties(cx, obj, SFImageProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFImageConstr.\n");
		return JS_FALSE;
	}

	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFImageConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).__x = 0;
		(ptr->v).__y = 0;
		(ptr->v).__depth = 0;
		(ptr->v).__data = "";
		(ptr->v).__texture = 0;
	} else {
		if (!JS_ConvertArguments(cx, argc, argv, "i i i s i",
								 &(pars[0]), &(pars[1]), &(pars[2]),
								 cpars, &(pars[3]))) {
			fprintf(stderr, "JS_ConvertArguments failed in SFImageConstr.\n");
			return JS_FALSE;
		}
		(ptr->v).__x = pars[0];
		(ptr->v).__y = pars[1];
		(ptr->v).__depth = pars[2];
		/* XXX change to allow memory reallocation */
		strcpy((ptr->v).__data, cpars);
		(ptr->v).__texture = pars[3];
	}
	if (verbose) {
		printf("SFImageConstr: obj = %u, %d args, %d %d %d %s %d\n",
			   (unsigned int) obj, argc,
			   (ptr->v).__x, (ptr->v).__y,
			   (ptr->v).__depth, (ptr->v).__data, (ptr->v).__texture);
	}
		
	return JS_TRUE;
}

JSBool
SFImageFinalize(JSContext *cx, JSObject *obj)
{
	SFImageNative *ptr;

	if (verbose) {
		printf("SFImageFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFImageFinalize.\n");
		return JS_FALSE;
	}
	SFImageNativeDelete(ptr);
	return JS_TRUE;
}


JSBool 
SFVec2fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsdouble d, *dp;
	SFVec2fNative *ptr;

	if ((ptr = JS_GetPrivate(cx,obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec2fGetProperty.\n");
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			d = (ptr->v).c[0];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFVec2fGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 1:
			d = (ptr->v).c[1];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFVec2fGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		}
	}
	return JS_TRUE;
}

JSBool 
SFVec2fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsval myv;
	SFVec2fNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec2fSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (verbose) {
		printf("SFVec2fSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}

	if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &myv)) {
		fprintf(stderr, "JS_ConvertValue failed in SFVec2fSetProperty.\n");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			(ptr->v).c[0] = *JSVAL_TO_DOUBLE(myv);
			break;
		case 1:
			(ptr->v).c[1] = *JSVAL_TO_DOUBLE(myv);
			break;
		case 2:
			(ptr->v).c[2] = *JSVAL_TO_DOUBLE(myv);
			break;
		}
	}
	return JS_TRUE;
}


JSBool
SFVec2fToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char buff[STRING];
    JSString *_str;
    SFVec2fNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec2fToString.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec2fToString.\n");
        return JS_FALSE;
	}
    
	memset(buff, 0, STRING);
	sprintf(buff, "%f %f", (ptr->v).c[0], (ptr->v).c[1]);
	_str = JS_NewStringCopyZ(cx, buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}

JSBool
SFVec2fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *o;
    SFVec2fNative *fptr, *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFVec2fAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in SFVec2fAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for o in SFVec2fAssign.\n");
        return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec2fAssign: obj = %u, o = %u, %u args\n",
			   (unsigned int) obj, (unsigned int) o, argc);
	}

    SFVec2fNativeAssign(ptr, fptr);
    *rval = OBJECT_TO_JSVAL(obj); 

    return JS_TRUE;
}

JSBool
SFVec2fTouched(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int t;
    SFVec2fNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec2fTouched.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec2fTouched.\n");
        return JS_FALSE;
	}
    t = ptr->touched;
	ptr->touched = 0;
    if (verbose) {
		printf("SFVec2fTouched: touched = %d\n", t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFVec2fSubtract(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *o, *proto, *ro;
	SFVec2fNative *vec1, *vec2, *res;
		
	if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec2fSubtract\n");
	}
			
	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, o, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in SFVec2fSubtract.\n");
		return JS_FALSE;
	}

	if ((proto = JS_GetPrototype(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec2fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	if ((vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for obj failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	if ((vec2 = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for o failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for ro failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);
	
	(res->v).c[0] = (vec1->v).c[0] - (vec2->v).c[0];
	(res->v).c[1] = (vec1->v).c[1] - (vec2->v).c[1];

	return JS_TRUE;
}


JSBool
SFVec2fNormalize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *ro, *proto;
	SFVec2fNative *vec, *res;
/* 	double xx; */
	struct pt v, ret;

	if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec2fNormalize\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "")) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec2fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	if ((vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	v.x = (vec->v).c[0];
	v.y = (vec->v).c[1];
	v.z = 0;
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for ro in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);

/* 	xx = sqrt((*vec1).v.c[0] * (*vec1).v.c[0] + */
/* 			  (*vec1).v.c[1] * (*vec1).v.c[1]); */
/* 	(*res).v.c[0] = (*vec1).v.c[0] / xx; */
/* 	(*res).v.c[1] = (*vec1).v.c[1] / xx; */
	vecnormal(&ret, &v);
	(res->v).c[0] = ret.x;
	(res->v).c[1] = ret.y;

	return JS_TRUE;
}
		

JSBool
SFVec2fAdd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *v2, *proto, *ro;
	SFVec2fNative *vec1, *vec2, *res;

	if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec2fAdd\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &v2)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, v2, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, v2)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec2fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if ((vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if ((vec2 = JS_GetPrivate(cx, v2)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for v2 in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for ro in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);
	(res->v).c[0] = (vec1->v).c[0] + (vec2->v).c[0];
	(res->v).c[1] = (vec1->v).c[1] + (vec2->v).c[1];

	return JS_TRUE;
}
		

JSBool
SFVec2fLength(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble result, *dp;
	JSObject *proto;
	SFVec2fNative *vec;
	struct pt v;

	if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec2fLength.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec2fLength\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "")) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fLength.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fLength.\n");
		return JS_FALSE;
	}
	if ((vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fLength.\n");
		return JS_FALSE;
	}
	v.x = (vec->v).c[0];
	v.y = (vec->v).c[1];
	v.z = 0;
/* 	result = sqrt((*vec).v.c[0] * (*vec).v.c[0] + */
/* 				  (*vec).v.c[1] * (*vec).v.c[1]); */
	result = (double) veclength(v);
	if ((dp = JS_NewDouble(cx, result)) == NULL) {
		fprintf(stderr, "JS_NewDouble failed for %f in SFVec2fLength.\n", result);
		return JS_FALSE;
	}
	*rval = DOUBLE_TO_JSVAL(dp); 

	return JS_TRUE;
}
		


JSBool
SFVec2fNegate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *ro, *proto;
	SFVec2fNative *vec, *res;

	if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec2fNegate.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec2fCross\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "")) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fNegate.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fNegate.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec2fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fNegate.\n");
		return JS_FALSE;
	}
	if ((vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fNegate.\n");
		return JS_FALSE;
	}
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for ro in SFVec2fNegate.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);
	(res->v).c[0] = -(vec->v).c[0];
	(res->v).c[1] = -(vec->v).c[1];

	return JS_TRUE;
}


JSBool
SFVec2fConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble pars[2];
	SFVec2fNative *ptr;
	/* void *p; */
	/* ptr = p; */

	if ((ptr = (SFVec2fNative *) SFVec2fNativeNew()) == NULL) {
		fprintf(stderr, "SFVec2fNativeNew failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}
	UNUSED(rval);

	if (!JS_DefineProperties(cx, obj, SFVec2fProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).c[0] = 0;
		(ptr->v).c[1] = 0;
	} else {
		if (!JS_ConvertArguments(cx, argc, argv, "d d",
								 &(pars[0]), &(pars[1]))) {
			fprintf(stderr, "JS_ConvertArguments failed in SFVec2fConstr.\n");
			return JS_FALSE;
		}
		(ptr->v).c[0] = pars[0];
		(ptr->v).c[1] = pars[1];
	}
	if (verbose) {
		printf("SFVec2fConstr: obj = %u, %d args, %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1]);
	}
	return JS_TRUE;
}


JSBool
SFVec2fFinalize(JSContext *cx, JSObject *obj)
{
	SFVec2fNative *ptr;

	if (verbose) {
		printf("SFColorFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec2fFinalize.\n");
		return JS_FALSE;
	}
	SFVec2fNativeDelete(ptr);
	return JS_TRUE;
}


JSBool 
SFVec3fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsdouble d, *dp;
	SFVec3fNative *ptr;

	if ((ptr = JS_GetPrivate(cx,obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec3fGetProperty.\n");
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			d = (ptr->v).c[0];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFVec3fGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 1:
			d = (ptr->v).c[1];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFVec3fGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 2:
			d = (ptr->v).c[2];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFVec3fGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		}
	}
	return JS_TRUE;
}

JSBool 
SFVec3fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsval myv;
	SFVec3fNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec3fSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (verbose) {
		printf("SFVec3fSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}

	if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &myv)) {
		fprintf(stderr, "JS_ConvertValue failed in SFVec3fSetProperty.\n");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			(ptr->v).c[0] = *JSVAL_TO_DOUBLE(myv);
			break;
		case 1:
			(ptr->v).c[1] = *JSVAL_TO_DOUBLE(myv);
			break;
		case 2:
			(ptr->v).c[2] = *JSVAL_TO_DOUBLE(myv);
			break;
		}
	}
	return JS_TRUE;
}


JSBool
SFVec3fToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char buff[STRING];
    JSString *_str;
    SFVec3fNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec3fToString.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fToString.\n");
        return JS_FALSE;
	}
	memset(buff, 0, STRING);
	sprintf(buff, "%f %f %f", (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	_str = JS_NewStringCopyZ(cx, buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}

JSBool
SFVec3fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *o;
    SFVec3fNative *fptr, *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFVec3fAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in SFVec3fAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for o in SFVec3fAssign.\n");
        return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec3fAssign: obj = %u, o = %u, %u args\n",
			   (unsigned int) obj, (unsigned int) o, argc);
	}

    SFVec3fNativeAssign(ptr, fptr);
    *rval = OBJECT_TO_JSVAL(obj); 

    return JS_TRUE;
}

JSBool
SFVec3fTouched(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int t;
    SFVec3fNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec3fTouched.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fTouched.\n");
        return JS_FALSE;
	}
    t = ptr->touched;
	ptr->touched = 0;
    if (verbose) {
		printf("SFVec3fTouched: touched = %d\n", t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFVec3fSubtract(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *o, *proto, *ro;
	SFVec3fNative *vec1, *vec2, *res;
		
	if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec3fSubtract\n");
	}
			
	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	
	if (!JS_InstanceOf(cx, o, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in SFVec3fSubtract.\n");
		return JS_FALSE;
	}

	if ((proto = JS_GetPrototype(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec3fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	if ((vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for obj failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	if ((vec2 = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for o failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for ro failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);
	
	(res->v).c[0] = (vec1->v).c[0] - (vec2->v).c[0];
	(res->v).c[1] = (vec1->v).c[1] - (vec2->v).c[1];
	(res->v).c[2] = (vec1->v).c[2] - (vec2->v).c[2];

	return JS_TRUE;
}


JSBool
SFVec3fNormalize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *ro, *proto;
	SFVec3fNative *vec, *res;
	struct pt v, ret;

	if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec3fNormalize\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "")) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec3fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	if ((vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	v.x = (vec->v).c[0];
	v.y = (vec->v).c[1];
	v.z = (vec->v).c[2];
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for ro in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);

	vecnormal(&ret, &v);
	(res->v).c[0] = ret.x;
	(res->v).c[1] = ret.y;
	(res->v).c[2] = ret.z;

	return JS_TRUE;
}
		

JSBool
SFVec3fAdd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *v2, *proto, *ro;
	SFVec3fNative *vec1, *vec2, *res;

	if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec3fAdd\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &v2)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, v2, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, v2)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec3fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if ((vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if ((vec2 = JS_GetPrivate(cx, v2)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for v2 in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for ro in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);
	(res->v).c[0] = (vec1->v).c[0] + (vec2->v).c[0];
	(res->v).c[1] = (vec1->v).c[1] + (vec2->v).c[1];
	(res->v).c[2] = (vec1->v).c[2] + (vec2->v).c[2];

	return JS_TRUE;
}
		

JSBool
SFVec3fLength(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble result, *dp;
	JSObject *proto;
	SFVec3fNative *vec;
	struct pt v;

	if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fLength.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec3fLength\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "")) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fLength.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fLength.\n");
		return JS_FALSE;
	}
	if ((vec = JS_GetPrivate(cx,obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fLength.\n");
		return JS_FALSE;
	}
	v.x = (vec->v).c[0];
	v.y = (vec->v).c[1];
	v.z = (vec->v).c[2];
/* 	result = sqrt((*vec1).v.c[0] * (*vec1).v.c[0] + */
/* 				  (*vec1).v.c[1] * (*vec1).v.c[1] + */
/* 				  (*vec1).v.c[2] * (*vec1).v.c[2]);  */
	result = (double) veclength(v);
	if ((dp = JS_NewDouble(cx, result)) == NULL) {
		fprintf(stderr, "JS_NewDouble failed for %f in SFVec3fLength.\n", result);
		return JS_FALSE;
	}
	*rval = DOUBLE_TO_JSVAL(dp); 

	return JS_TRUE;
}
		

JSBool
SFVec3fCross(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *v2, *proto, *ro;
	SFVec3fNative *vec, *res;
	struct pt c1, c2, ret;

	if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fCross.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec3fCross\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &v2)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fCross.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, v2, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fCross.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, v2)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fCross.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec3fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fCross.\n");
		return JS_FALSE;
	}
	if ((vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fCross.\n");
		return JS_FALSE;
	}
	c1.x = (vec->v).c[0]; 
	c1.y = (vec->v).c[1]; 
	c1.z = (vec->v).c[2]; 
	vec = 0;
	if ((vec = JS_GetPrivate(cx, v2)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for v2 in SFVec3fCross.\n");
		return JS_FALSE;
	}
	c2.x = (vec->v).c[0]; 
	c2.y = (vec->v).c[1]; 
	c2.z = (vec->v).c[2]; 
	vec = 0;
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for ro in SFVec3fCross.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);

	veccross(&ret, c1, c2);
	(res->v).c[0] = ret.x;
	(res->v).c[1] = ret.y;
	(res->v).c[2] = ret.z;

	return JS_TRUE;
}
		

JSBool
SFVec3fNegate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *ro, *proto;
	SFVec3fNative *vec, *res;

	if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fNegate.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFVec3fNegate\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "")) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fNegate.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fNegate.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec3fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fNegate.\n");
		return JS_FALSE;
	}
	if ((vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fNegate.\n");
		return JS_FALSE;
	}
	if ((res = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for ro in SFVec3fNegate.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(ro);
	(res->v).c[0] = -(vec->v).c[0];
	(res->v).c[1] = -(vec->v).c[1];
	(res->v).c[2] = -(vec->v).c[2];

	return JS_TRUE;
}

JSBool
SFVec3fConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble pars[3];
	SFVec3fNative *ptr;

	if ((ptr = (SFVec3fNative *) SFVec3fNativeNew()) == NULL) {
		fprintf(stderr, "SFVec3fNativeNew failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}
	UNUSED(rval);

	if (!JS_DefineProperties(cx, obj, SFVec3fProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).c[0] = 0;
		(ptr->v).c[1] = 0;
		(ptr->v).c[2] = 0;
	} else {
		if (!JS_ConvertArguments(cx, argc, argv, "d d d",
								 &(pars[0]), &(pars[1]), &(pars[2]))) {
			fprintf(stderr, "JS_ConvertArguments failed in SFVec3fConstr.\n");
			return JS_FALSE;
		}
		(ptr->v).c[0] = pars[0];
		(ptr->v).c[1] = pars[1];
		(ptr->v).c[2] = pars[2];
	}
	if (verbose) {
		printf("SFVec3fConstr: obj = %u, %d args, %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	}
	return JS_TRUE;
}

JSBool
SFVec3fFinalize(JSContext *cx, JSObject *obj)
{
	SFVec3fNative *ptr;

	if (verbose) {
		printf("SFVec3fFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec3fFinalize.\n");
		return JS_FALSE;
	}
	SFVec3fNativeDelete(ptr);
	return JS_TRUE;
}


JSBool 
SFRotationGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsdouble d;
	jsdouble *dp;
	SFRotationNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFRotationGetProperty.\n");
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			d = (ptr->v).r[0];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFRotationGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 1:
			d = (ptr->v).r[1];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFRotationGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 2:
			d = (ptr->v).r[2];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFRotationGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 3:
			d = (ptr->v).r[3];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				fprintf(stderr,
						"JS_NewDouble failed for %f in SFRotationGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break;
		}
	}
	return JS_TRUE;
}

JSBool 
SFRotationSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	jsval myv;
	SFRotationNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFRotationSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (verbose) {
		printf("SFRotationSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}
	if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &myv)) {
		fprintf(stderr, "JS_ConvertValue failed in SFRotationSetProperty.\n");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			(ptr->v).r[0] = *JSVAL_TO_DOUBLE(myv);
			break;
		case 1:
			(ptr->v).r[1] = *JSVAL_TO_DOUBLE(myv);
			break;
		case 2:
			(ptr->v).r[2] = *JSVAL_TO_DOUBLE(myv);
			break;
		case 3:
			(ptr->v).r[3] = *JSVAL_TO_DOUBLE(myv);
			break;
		}
	}
	return JS_TRUE;
}


JSBool
SFRotationToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char buff[STRING];
    JSString *_str;
    SFRotationNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFRotationToString.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationToString.\n");
        return JS_FALSE;
	}
    
	memset(buff, 0, STRING);
	sprintf(buff, "%f %f %f %f", (ptr->v).r[0], (ptr->v).r[1], (ptr->v).r[2], (ptr->v).r[3]);
	_str = JS_NewStringCopyZ(cx, buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}

JSBool
SFRotationAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *o;
    SFRotationNative *fptr, *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFRotationAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFRotationAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in SFRotationAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for o in SFRotationAssign.\n");
        return JS_FALSE;
	}
	if (verbose) {
		printf("SFRotationAssign: obj = %u, o = %u, %u args\n",
			   (unsigned int) obj, (unsigned int) o, argc);
	}

    SFRotationNativeAssign(ptr, fptr);
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}

JSBool
SFRotationTouched(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int t;
    SFRotationNative *ptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFRotationTouched.\n");
		return JS_FALSE;
	}
	UNUSED(argc);

    if (!JS_InstanceOf(cx, obj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationTouched.\n");
        return JS_FALSE;
	}
    t = ptr->touched;
	ptr->touched = 0;
    if (verbose) {
		printf("SFRotationTouched: touched = %d\n", t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFRotationMultVec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *o, *ro, *proto;
	SFRotationNative *rfrom;
	SFVec3fNative *vfrom, *vto;
/* 	double rl, vl, s, c; */
	float rl, vl, rlpt, s, c, angle;
/* 	float c1[3], c2[3]; */
	struct pt rotfrom, vecfrom, c1, c2;

	if (!JS_InstanceOf(cx, obj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFRotationMultVec\n");
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, o, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if ((proto = JS_GetPrototype(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if ((ro = JS_ConstructObject(cx, &SFVec3fClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if ((rfrom = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	rotfrom.x = rfrom->v.r[0];
	rotfrom.y = rfrom->v.r[1];
	rotfrom.z = rfrom->v.r[2];
	angle = rfrom->v.r[3];
	if ((vfrom = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for o in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	vecfrom.x = vfrom->v.c[0];
	vecfrom.y = vfrom->v.c[1];
	vecfrom.z = vfrom->v.c[2];
	if ((vto = JS_GetPrivate(cx, ro)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for ro in SFRotationMultVec.\n");
		return JS_FALSE;
	}


	
	rl = veclength(rotfrom);
	vl = veclength(vecfrom);
	rlpt = VECPT(rotfrom, vecfrom) / rl / vl;
	s = sin(angle);
	c = cos(angle);
	VECCP(rotfrom, vecfrom, c1);
	VECSCALE(c1, 1.0 / rl);
	VECCP(rotfrom, c1, c2);
	VECSCALE(c2, 1.0 / rl) ;
	vto->v.c[0] = vecfrom.x + s * c1.x + (1-c) * c2.x;
	vto->v.c[1] = vecfrom.y + s * c1.y + (1-c) * c2.y;
	vto->v.c[2] = vecfrom.z + s * c1.z + (1-c) * c2.z;
	
	*rval = OBJECT_TO_JSVAL(ro);
	return JS_TRUE;
}
		

JSBool
SFRotationInverse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *o, *proto;
	SFRotationNative *rfrom, *rto;
	UNUSED(argc);

	if (!JS_InstanceOf(cx, obj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationInverse.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFRotationInverse\n");
	}

	if ((proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFRotationInverse.\n");
		return JS_FALSE;
	}
	if ((o = JS_ConstructObject(cx, &SFRotationClass, proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFRotationInverse.\n");
		return JS_FALSE;
	}
	if ((rfrom = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationInverse.\n");
		return JS_FALSE;
	}
	if ((rto = JS_GetPrivate(cx, o)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for o in SFRotationInverse.\n");
		return JS_FALSE;
	}

	rto->v.r[0] = rfrom->v.r[0];
	rto->v.r[1] = rfrom->v.r[1];
	rto->v.r[2] = rfrom->v.r[2];
	rto->v.r[3] = -rfrom->v.r[3];

	*rval = OBJECT_TO_JSVAL(o);
	return JS_TRUE;
}
		

JSBool 
SFRotationConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble pars[4];
	JSObject *_ob1, *_ob2;
/* 	JSFunction *_f; */
	SFVec3fNative *vec;
	float v1len, v2len;
	double v12dp;
	struct pt v1, v2;
	SFRotationNative *ptr;

	if ((ptr = SFRotationNativeNew()) == NULL) {
		fprintf(stderr, "SFRotationNativeNew failed in SFRotationConstr.\n");
		return JS_FALSE;
	}
	UNUSED(rval);

	if (!JS_DefineProperties(cx, obj, SFRotationProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFRotationConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFRotationConstr.\n");
		return JS_FALSE;
	}

	if (argc == 0) {
		(ptr->v).r[0] = 0;
		(ptr->v).r[1] = 0;
		(ptr->v).r[2] = 1;
		(ptr->v).r[3] = 0;
	} else if (JS_ConvertArguments(cx, argc, argv, "d d d d",
							&(pars[0]), &(pars[1]), &(pars[2]), &(pars[3]))) {
		(ptr->v).r[0] = pars[0];
		(ptr->v).r[1] = pars[1];
		(ptr->v).r[2] = pars[2];
		(ptr->v).r[3] = pars[3];
	} else if (JS_ConvertArguments(cx, argc, argv, "o o", &_ob1, &_ob2)) {
		if (!JS_InstanceOf(cx, _ob1, &SFVec3fClass, argv)) {
			fprintf(stderr, "JS_InstanceOf failed for _ob1 in SFRotationConstr.\n");
			return JS_FALSE;
		}

		if (!JS_InstanceOf(cx, _ob2, &SFVec3fClass, argv)) {
			fprintf(stderr, "JS_InstanceOf failed for _ob2 in SFRotationConstr.\n");
			return JS_FALSE;
		}
		if ((vec = JS_GetPrivate(cx, _ob1)) == NULL) {
			fprintf(stderr, "JS_GetPrivate failed for _ob1 in SFRotationConstr.\n");
			return JS_FALSE;
		}
		v1.x = vec->v.c[0];
		v1.y = vec->v.c[1];
		v1.z = vec->v.c[2];
		vec = 0;

		if ((vec = JS_GetPrivate(cx, _ob2)) == NULL) {
			fprintf(stderr, "JS_GetPrivate failed for _ob2 in SFRotationConstr.\n");
			return JS_FALSE;
		}
		v2.x = vec->v.c[0];
		v2.y = vec->v.c[1];
		v2.z = vec->v.c[2];
		vec = 0;

		v1len = veclength(v1);
		v2len = veclength(v2);
		v12dp = vecdot(&v1, &v2);
		(ptr->v).r[0] = v1.y * v2.z - v2.y * v1.z;
		(ptr->v).r[1] = v1.z * v2.x - v2.z * v1.x;
		(ptr->v).r[2] = v1.x * v2.y - v2.x * v1.y;
		v12dp /= v1len * v2len;
		(ptr->v).r[3] = atan2(sqrt(1 - v12dp * v12dp), v12dp);
	} else if (JS_ConvertArguments(cx, argc, argv, "o d", &_ob1, &(pars[0]))) {
		if (!JS_InstanceOf(cx, _ob1, &SFVec3fClass, argv)) {
			fprintf(stderr, "JS_InstanceOf failed for _ob1 with arg format \"o d\" in SFRotationConstr.\n");
			return JS_FALSE;
		}
		if ((vec = JS_GetPrivate(cx, _ob1)) == NULL) {
			fprintf(stderr, "JS_GetPrivate failed for _ob1 with arg format \"o d\" in SFRotationConstr.\n");
			return JS_FALSE;
		}
		(ptr->v).r[0] = vec->v.c[0]; 
		(ptr->v).r[1] = vec->v.c[1]; 
		(ptr->v).r[2] = vec->v.c[2]; 
		(ptr->v).r[3] = pars[0];
		vec = 0;
/* 	} else if (JS_ConvertArguments(cx, argc, argv, "o f", &_ob1, &_f)) { */
/* 		if (!JS_InstanceOf(cx, _ob1, &SFVec3fClass, argv)) { */
/* 			fprintf(stderr, "JS_InstanceOf failed for _ob1 with arg format \"o f\" in SFRotationConstr.\n"); */
/* 			return JS_FALSE; */
/* 		} */
	} else {
		fprintf(stderr, "Invalid arguments for SFRotationConstr.\n");
		return JS_FALSE;
	}

	if (verbose) {
		printf("SFRotationConstr: obj = %u, %d args, %f %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).r[0], (ptr->v).r[1], (ptr->v).r[2], (ptr->v).r[3]);
	}
	return JS_TRUE;
}


JSBool
SFRotationFinalize(JSContext *cx, JSObject *obj)
{
	SFRotationNative *ptr;

	if (verbose) {
		printf("SFRotationFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFRotationFinalize.\n");
		return JS_FALSE;
	}
	SFRotationNativeDelete(ptr);
	return JS_TRUE;
}


JSBool
MFColorAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFColorAddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFColorSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFColorSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFColorConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	unsigned int i;
	char buf[STRING];
	jsval v = INT_TO_JSVAL(argc);
	UNUSED(rval);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT )) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFColorConstr.\n");
		return JS_FALSE;
	}
	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFColorConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFColorConstr: obj = %u, args: ", (unsigned int) obj);
	}
	for (i = 0; i < argc; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
		if (verbose) {
			printf("%s ", buf);
		}
		/* XXX Check type */
		if (!JS_DefineProperty(cx, obj, buf, argv[i],
							   JS_PropertyStub, JS_PropertyStub,
							   JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineProperty failed for \"%s\" in MFColorConstr.\n",
					buf);
			return JS_FALSE;
		}
	}
	if (verbose) {
		printf("\n");
	}
	return JS_TRUE;
}

JSBool
MFColorAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval val, myv;
    int len, i;
    JSObject *o;
	char buf[STRING];

	if (!JS_InstanceOf(cx, obj, &MFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFColorAssign.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("MFColorAssign: obj = %u, %u args\n", (unsigned int) obj, argc);
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFColorAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &MFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFColorAssign.\n");
        return JS_FALSE;
    }
	/* Now, we assign length properties from o to obj (XXX HERE) */
	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr, "JS_SetProperty failed for \"__touched_flag\" in MFColorAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, o, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFColorAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFColorAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val); /* XXX Assume int */

    for (i = 0; i < len; i++) {
		memset(buf, 0, STRING);
		sprintf(buf,"%d",i);
		if (!JS_GetProperty(cx, o, buf, &val)) {
			fprintf(stderr, "JS_GetProperty failed for \"%s\" in MFColorAssign.\n", buf);
			return JS_FALSE;
		}
		if (!JS_SetProperty(cx, obj, buf, &val)) {
			fprintf(stderr, "JS_SetProperty failed for \"%s\" in MFColorAssign.\n", buf);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}


JSBool
MFVec2fAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFVec2fAddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFVec2fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFVec2fSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFVec2fConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	unsigned int i;
	char buf[STRING];
	jsval v = INT_TO_JSVAL(argc);
	UNUSED(rval);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr, "JS_DefineProperty failed for \"length\" in MFVec2fConstr.\n");
        return JS_FALSE;
	}
	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFVec2fConstr.\n");
        return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFVec2fConstr: obj = %u, args: ", (unsigned int) obj);
	}
	for (i = 0; i < argc; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
		if (verbose) {
			printf("%s ", buf);
		}
		/* XXX Check type */
		if (!JS_DefineProperty(cx, obj, buf, argv[i],
							   JS_PropertyStub, JS_PropertyStub,
							   JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineProperty failed for \"%s\" in MFVec2fConstr.\n",
					buf);
			return JS_FALSE;
		}
	}
	if (verbose) {
		printf("\n");
	}
	return JS_TRUE;
}

JSBool
MFVec2fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval val, myv;
    int len, i;
    JSObject *o;
	char buf[STRING];

	if (!JS_InstanceOf(cx, obj, &MFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFVec2fAssign.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("MFVec2fAssign: obj = %u, %u args\n", (unsigned int) obj, argc);
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFVec2fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &MFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in MFVec2fAssign.\n");
        return JS_FALSE;
    }

	/* Now, we assign length properties from o to obj */
	/* XXX HERE */
	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr, "JS_SetProperty failed for \"__touched_flag\" in MFVec2fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, o, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFVec2fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFVec2fAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val); /* XXX Assume int */

    for (i = 0; i < len; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
	    if (!JS_GetProperty(cx, o, buf, &val)) {
			fprintf(stderr,
					"JS_GetProperty failed for \"%s\" in MFVec2fAssign.\n",
					buf);
			return JS_FALSE;
		}
	    if (!JS_SetProperty(cx, obj, buf, &val)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in MFVec2fAssign.\n",
					buf);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}


JSBool
MFVec3fAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFVec3fAddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFVec3fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFVec3fSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFVec3fConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	unsigned int i;
	char buf[STRING];
	jsval v = INT_TO_JSVAL(argc);
	UNUSED(rval);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr, "JS_DefineProperty failed for \"length\" in MFVec3fConstr.\n");
        return JS_FALSE;
	}
	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFVec3fConstr.\n");
        return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFVec3fConstr: obj = %u, args: ", (unsigned int) obj);
	}
	for (i = 0; i < argc; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
		if (verbose) {
			printf("%s ", buf);
		}
		/* XXX Check type */
		if (!JS_DefineProperty(cx, obj, buf, argv[i],
							   JS_PropertyStub, JS_PropertyStub,
							   JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineProperty failed for \"%s\" in MFVec3fConstr.\n",
					buf);
			return JS_FALSE;
		}
	}
	if (verbose) {
		printf("\n");
	}
	return JS_TRUE;
}

JSBool
MFVec3fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval val, myv;
    int len, i;
    JSObject *o;
	char buf[STRING];

	if (!JS_InstanceOf(cx, obj, &MFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFVec3fAssign.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("MFVec3fAssign: obj = %u, %u args\n", (unsigned int) obj, argc);
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFVec3fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &MFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in MFVec3fAssign.\n");
        return JS_FALSE;
    }

	/* Now, we assign length properties from o to obj */
	/* XXX HERE */
	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr, "JS_SetProperty failed for \"__touched_flag\" in MFVec3fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, o, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFVec3fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFVec3fAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val); /* XXX Assume int */

    for (i = 0; i < len; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
	    if (!JS_GetProperty(cx, o, buf, &val)) {
			fprintf(stderr,
					"JS_GetProperty failed for \"%s\" in MFVec3fAssign.\n",
					buf);
			return JS_FALSE;
		}
	    if (!JS_SetProperty(cx, obj, buf, &val)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in MFVec3fAssign.\n",
					buf);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}


JSBool
MFRotationAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFRotationAddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFRotationSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFRotationSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFRotationConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	unsigned int i;
	char buf[STRING];
	jsval v = INT_TO_JSVAL(argc);
	UNUSED(rval);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT )) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFRotationConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFRotationConstr.\n");
		return JS_FALSE;
	}

	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFRotationConstr: obj = %u, args: ", (unsigned int) obj);
	}
	for (i = 0; i < argc; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
		if (verbose) {
			printf("%s ", buf);
		}
		/* XXX Check type */
		if (!JS_DefineProperty(cx, obj, buf, argv[i],
							   JS_PropertyStub, JS_PropertyStub,
							   JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineProperty failed for \"%s\" in MFRotationConstr.\n",
					buf);
			return JS_FALSE;
		}
	}
	if (verbose) {
		printf("\n");
	}
	return JS_TRUE;
}

JSBool
MFRotationAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval val;
    jsval myv;
    int len;
    int i;
    JSObject *o;
	char buf[STRING];

    if (!JS_InstanceOf(cx, obj, &MFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in MFRotationAssign.\n");
        return JS_FALSE;
	}
    if (verbose) {
		printf("MFRotationAssign: obj = %u, %u args\n", (unsigned int) obj, argc);
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o",&o)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFRotationAssign.\n");
		return JS_FALSE;
	}
	
    if (!JS_InstanceOf(cx, o, &MFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for o in MFRotationAssign.\n");
        return JS_FALSE;
    }
	/* Now, we assign length properties from o to obj */
	/* XXX HERE */
	myv = INT_TO_JSVAL(1);

    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFRotationAssign.\n");
        return JS_FALSE;
	}

    if (!JS_GetProperty(cx, o, "length", &val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFRotationAssign.\n");
        return JS_FALSE;
	}

    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"length\" in MFRotationAssign.\n");
        return JS_FALSE;
	}

    len = JSVAL_TO_INT(val); /* XXX Assume int */
    for (i = 0; i < len; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
	    if (!JS_GetProperty(cx, o, buf, &val)) {
			fprintf(stderr,
					"JS_GetProperty failed for \"%s\" in MFRotationAssign.\n",
					buf);
			return JS_FALSE;
		}

	    if (!JS_SetProperty(cx, obj, buf, &val)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in MFRotationAssign.\n",
					buf);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}



JSBool
MFNodeAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFNodeAddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool
MFNodeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFNodeSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFNodeConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	unsigned int i;
	char buf[STRING];
	jsval v = INT_TO_JSVAL(argc);
	UNUSED(rval);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFNodeConstr.\n");
		return JS_FALSE;
	}
	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFNodeConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFNodeConstr: obj = %u, args: ", (unsigned int) obj);
	}
	for (i = 0; i < argc; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
		if (verbose) {
			printf("%s ", buf);
		}
		/* XXX Check type */
		if (!JS_DefineProperty(cx, obj, buf, argv[i],
							   JS_PropertyStub, JS_PropertyStub,
							   JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineProperty failed for \"%s\" in MFNodeConstr.\n",
					buf);
			return JS_FALSE;
		}
	}
	if (verbose) {
		printf("\n");
	}
	return JS_TRUE;
}

JSBool
MFNodeAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval val, myv;
    int len, i;
    JSObject *o;
	char buf[STRING];

    if (!JS_InstanceOf(cx, obj, &MFNodeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf for obj failed in MFNodeAssign.\n");
        return JS_FALSE;
	}
    if (verbose) {
		printf("MFNodeAssign: obj = %u, %u args\n", (unsigned int) obj, argc);
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o",&o)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFNodeAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &MFNodeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf for o failed in MFNodeAssign.\n");
        return JS_FALSE;
    }
	/* Now, we assign length properties from o to obj */
	/* XXX HERE */
	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFNodeAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, o, "length", &val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFNodeAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"length\" in MFNodeAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val); /* XXX Assume int */

	for (i = 0; i < len; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
		if (!JS_GetProperty(cx, o, buf, &val)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in MFNodeAssign.\n",
					buf);
			return JS_FALSE;
		}
		if (!JS_SetProperty(cx, obj, buf, &val)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in MFNodeAssign.\n",
					buf);
			return JS_FALSE;
		}
	}
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}


JSBool
MFStringAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFStringAddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}


JSBool 
MFStringSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFStringSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFStringConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	unsigned int i;
	char buf[STRING];
	jsval v = INT_TO_JSVAL(argc);
	UNUSED(rval);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr, "JS_DefineProperty failed for \"length\" in MFStringConstr.\n");
		return JS_FALSE;
	}
	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFStringConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFStringConstr: obj = %u, args: ", (unsigned int) obj);
	}
	for (i = 0; i < argc; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
		if (verbose) {
			printf("%s ", buf);
		}
		/* XXX Check type */
		if (!JS_DefineProperty(cx, obj, buf, argv[i],
							   JS_PropertyStub, JS_PropertyStub,
							   JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineProperty failed for \"%s\" in MFStringConstr.\n",
					buf);
			return JS_FALSE;
		}
	}
	if (verbose) {
		printf("\n");
	}
	return JS_TRUE;
}

JSBool
MFStringAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval val, myv;
    int len, i;
    JSObject *o;
	char buf[STRING];

	if (!JS_InstanceOf(cx, obj, &MFStringClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFStringAssign.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("MFStringAssign: obj = %u, %u args\n", (unsigned int) obj, argc);
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &o)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFStringClass.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, o, &MFStringClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFStringAssign.\n");
        return JS_FALSE;
    }
	/* Now, we assign length properties from o to obj */
	/* XXX HERE */
	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFStringAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, o, "length", &val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFStringAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"length\" in MFStringAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val); /* XXX Assume int */

	for (i = 0; i < len; i++) {
		memset(buf, 0, STRING);
		sprintf(buf, "%d", i);
		if (!JS_GetProperty(cx, o, buf, &val)) {
			fprintf(stderr,
					"JS_GetProperty failed for \"%s\" in MFStringAssign.\n",
					buf);
			return JS_FALSE;
		}
		if (!JS_SetProperty(cx, obj, buf, &val)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in MFStringAssign.\n",
					buf);
			return JS_FALSE;
		}
	}
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}


JSBool
SFNodeConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char *_id;
	UNUSED(rval);

	if (argc == 1 &&
		JS_ConvertArguments(cx, argc, argv, "s", &_id)) {
		if (!JS_DefineProperty(cx, obj, "__id", argv[0],
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
			fprintf(stderr,
					"JS_DefineProperty failed for \"__id\" in SFNodeConstr.\n");
			return JS_FALSE;
		}
	} else {
		fprintf(stderr,
				"JS_ConvertArguments failed in SFNodeConstr: format is \"s\".\n");
		return JS_TRUE;
	}

	if (verbose) {
		printf("SFNodeConstr: obj = %u, argc = %d, %s\n",
			   (unsigned int) obj, argc, _id);
	}
	return JS_TRUE;
}


JSBool
SFNodeFinalize(JSContext *cx, JSObject *obj)
{
	UNUSED(cx);
	UNUSED(obj);
	return JS_TRUE;
}

JSBool
SFNodeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *globalObj;
	BrowserNative *brow;
	jsval pv, v = OBJECT_TO_JSVAL(obj);

	globalObj = JS_GetGlobalObject(cx);
	if (globalObj == NULL) {
		fprintf(stderr, "JS_GetGlobalObject failed in SFNodeSetProperty.\n");
		return JS_FALSE;
	}
	if (!JS_GetProperty(cx, globalObj, "Browser", &pv)) {
		fprintf(stderr, "JS_GetProperty failed for \"Browser\" in SFNodeSetProperty.\n");
		return JS_FALSE;
	}
	if (!JSVAL_IS_OBJECT(pv)) {
		fprintf(stderr, "jsval pv is not an object in SFNodeSetProperty.\n");
		return JS_FALSE;
	}
	
	if ((brow = JS_GetPrivate(cx, JSVAL_TO_OBJECT(pv))) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFNodeSetProperty.\n");
		return JS_FALSE;
	}

	if (!JS_SetProperty(cx, globalObj, "__node", &v)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__node\" in SFNodeSetProperty.\n");
		return JS_FALSE;
	}
	if (!JS_SetProperty(cx, globalObj, "__prop", &id)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__prop\" in SFNodeSetProperty.\n");
		return JS_FALSE;
	}

	if (!JS_SetProperty(cx, globalObj, "__val", vp)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__val\" in SFNodeSetProperty.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFNodeSetProperty: obj = %u\n", (unsigned int) obj);
	}
	doPerlCallMethod(brow->sv_js, "nodeSetProperty");

	return JS_TRUE;
}

