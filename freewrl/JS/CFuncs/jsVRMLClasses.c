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

#include "jsVRMLClasses.h"

JSBool
globalResolve(JSContext *cx, JSObject *obj, jsval id) 
{
	UNUSED(cx);
	UNUSED(obj);
	UNUSED(id);
	return JS_TRUE;
}

/* change to support using elements ??? */
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


/* change to support using elements ??? */
JSBool
doMFSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_sstr;
	char *_cc;
	jsval myv;
	jsint _index;

	_sstr = JS_ValueToString(cx, *vp);
	_cc = JS_GetStringBytes(_sstr);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("\tdoMFSetProperty: obj = %u, id = %d, vp = %s\n",
				   (unsigned int) obj, _index, _cc);
		}

		myv = INT_TO_JSVAL(1);
		if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"__touched_flag\" in doMFSetProperty.\n");
			return JS_FALSE;
		}
	} else {
		printf("\tdoMFSetProperty: obj = %u, id = ???, vp = %s\n",
			   (unsigned int) obj, _cc);
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

	if ((proto_MFBool = JS_InitClass(context, globalObj, NULL,
									  &MFBoolClass, MFBoolConstr, INIT_ARGC,
									  NULL, MFBoolFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFBool);
	if (!JS_SetProperty(context, globalObj, "__MFBool_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFBoolClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFFloat = JS_InitClass(context, globalObj, NULL,
									  &MFFloatClass, MFFloatConstr, INIT_ARGC,
									  NULL, MFFloatFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFFloat);
	if (!JS_SetProperty(context, globalObj, "__MFFloat_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFFloatClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFTime = JS_InitClass(context, globalObj, NULL,
									  &MFTimeClass, MFTimeConstr, INIT_ARGC,
									  NULL, MFTimeFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFTime);
	if (!JS_SetProperty(context, globalObj, "__MFTime_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFTimeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFInt32 = JS_InitClass(context, globalObj, NULL,
									  &MFInt32Class, MFInt32Constr, INIT_ARGC,
									  NULL, MFInt32Functions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFInt32);
	if (!JS_SetProperty(context, globalObj, "__MFInt32_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFInt32Class failed in loadVrmlClasses.\n");
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
				"JS_InitClass for MFString failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFString);
	if (!JS_SetProperty(context, globalObj, "__MFString_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFStringClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	return JS_TRUE;
}


JSBool
setECMANative(JSContext *context, JSObject *obj, jsval id, jsval *vp)
{
	jsval v;
	char buffer[STRING];
	char *n = JS_GetStringBytes(JSVAL_TO_STRING(id));
	UNUSED(vp);
	
	if (verbose) {
		printf("setECMANative %s\n", n);
	}

	memset(buffer, 0, STRING);
	sprintf(buffer, "_%s_touched", n);
	v = INT_TO_JSVAL(1);
	if (!JS_SetProperty(context, obj, buffer, &v)) {
		fprintf(stderr,
				"JS_SetProperty failed in setECMANative.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}

#if FALSE

/* JSBool */
/* getAssignProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) */
/* { */
/* 	JSString *_str; */
/* 	char *c; */
/* 	UNUSED(cx); */
/* 	UNUSED(vp); */

/* 	if (JSVAL_IS_STRING(id)) { */
/* 		_str = JSVAL_TO_STRING(id); */
/* 		c = JS_GetStringBytes(_str); */
/* 		if (verbose) { */
/* 			printf("getAssignProperty: obj = %u, id = %s\n", */
/* 				   (unsigned int) obj, c); */
/* 		} */
/* 	} */
/* 	return JS_TRUE; */
/* } */

#endif /* FALSE */

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
	SFColorNative *ptr;
	jsdouble d, *dp;

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
	SFColorNative *ptr;
	jsval _val;

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
    SFColorNative *ptr;
    JSString *_str;
	char _buff[STRING];

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
	sprintf(_buff, "%.4g %.4g %.4g",
			(ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	_str = JS_NewStringCopyZ(cx, _buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}


JSBool
SFColorAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    SFColorNative *ptr, *fptr;
    JSObject *o;

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
    SFColorNative *ptr;
    int t;

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
		printf("SFColorTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFColorConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SFColorNative *ptr;
	jsdouble pars[3];

	if ((ptr = (SFColorNative *) SFColorNativeNew()) == NULL) {
		fprintf(stderr, "SFColorNativeNew failed in SFColorConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFColorProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFColorConstr.\n");
		return JS_FALSE;
	}

	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFColorConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).c[0] = 0.0;
		(ptr->v).c[1] = 0.0;
		(ptr->v).c[2] = 0.0;
	} else if (JS_ConvertArguments(cx, argc, argv, "d d d",
									&(pars[0]), &(pars[1]), &(pars[2]))) {
		(ptr->v).c[0] = pars[0];
		(ptr->v).c[1] = pars[1];
		(ptr->v).c[2] = pars[2];
	} else {
		fprintf(stderr, "Invalid arguments for SFColorConstr.\n");
		return JS_FALSE;
	}
	if (verbose) {
		printf("SFColorConstr: obj = %u, %u args, %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	}
	*rval = OBJECT_TO_JSVAL(obj);

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
	UNUSED(cx);
	UNUSED(obj);
	UNUSED(id);
	UNUSED(vp);
#if FALSE
/* 	SFImageNative *ptr; */
/* 	jsint i; */
/* 	JSString *_str; */

/* 	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) { */
/* 		fprintf(stderr, "JS_GetPrivate failed in SFImageGetProperty.\n"); */
/* 		return JS_FALSE; */
/* 	} */

/* 	if (JSVAL_IS_INT(id)) { */
/* 		switch (JSVAL_TO_INT(id)) { */
/* 		case 0: */
/* 			i = (ptr->v).__x; */
/* 			*vp = INT_TO_JSVAL(i); */
/* 			break; */
/* 		case 1: */
/* 			i = (ptr->v).__y; */
/* 			*vp = INT_TO_JSVAL(i); */
/* 			break; */
/* 		case 2: */
/* 			i = (ptr->v).__depth; */
/* 			*vp = INT_TO_JSVAL(i); */
/* 			break; */
/* 		case 3: */
/* 			_str = JS_NewStringCopyZ(cx, (ptr->v).__data); */
/* 			*vp = STRING_TO_JSVAL(_str); */
/* 		case 4: */
/* 			i = (ptr->v).__texture; */
/* 			*vp = INT_TO_JSVAL(i); */
/* 			break; */
/* 		} */
/* 	} */
#endif
	return JS_TRUE;
}

JSBool
SFImageSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	SFImageNative *ptr;
	UNUSED(id);
	UNUSED(vp);
#if FALSE
/* 	JSString *_str; */
/* 	jsval myv; */
/* 	char *c; */
/* 	size_t _len, _data_s; */
#endif

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFImageSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
#if FALSE
/* 	if (verbose) { */
/* 		printf("SFImageSetProperty: obj = %u, id = %d, touched = %d\n", */
/* 			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched); */
/* 	} */
/* 	if (JSVAL_IS_NUMBER(*vp)) { */ /* succeeds for int and double */
/* 		if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &myv)) { */
/* 			fprintf(stderr, */
/* 					"JS_ConvertValue failed for vp as JSTYPE_NUMBER in SFImageSetProperty.\n"); */
/* 			return JS_FALSE; */
/* 		} */
/* 	} else if (JSVAL_IS_STRING(*vp)) { */
/* 		if (!JS_ConvertValue(cx, *vp, JSTYPE_STRING, &myv)) { */
/* 			fprintf(stderr, */
/* 					"JS_ConvertValue failed for vp as JSTYPE_STRING in SFImageSetProperty.\n"); */
/* 			return JS_FALSE; */
/* 		} */
/* 	} else { */
/* 			fprintf(stderr, */
/* 					"Arg vp is not one of JSTYPE_NUMBER or JSTYPE_STRING.\n"); */
/* 			return JS_FALSE; */
/* 	} */

/* 	if (JSVAL_IS_INT(id)) { */
/* 		switch (JSVAL_TO_INT(id)) { */
/* 		case 0: */
/* 			(ptr->v).__x = JSVAL_TO_INT(myv); */
/* 			break; */
/* 		case 1: */
/* 			(ptr->v).__y = JSVAL_TO_INT(myv); */
/* 			break; */
/* 		case 2: */
/* 			(ptr->v).__depth = JSVAL_TO_INT(myv); */
/* 			break; */
/* 		case 3: */
/* 			_str = JS_ValueToString(cx, myv); */
/* 			c = JS_GetStringBytes(_str); */

/* 			_len = strlen(c); */
/* 			_data_s = sizeof((ptr->v).__data); */
/* 			if (_len * sizeof(char) > _data_s) { */
/* 				_data_s = (_len + 1) * sizeof(char); */
/* 				if (((ptr->v).__data = */
/* 					 (char *) realloc((ptr->v).__data, _data_s)) */
/* 					== NULL) { */
/* 					fprintf(stderr, */
/* 						"realloc failed in SFImageSetProperty.\n"); */
/* 					return JS_FALSE; */
/* 				} */
/* 			} */
/* 			memset((ptr->v).__data, 0, _len + 1); */
/* 			memmove((ptr->v).__data, c, _len); */
/* 		case 4: */
/* 			(ptr->v).__texture = JSVAL_TO_INT(myv); */
/* 			break; */
/* 		} */
/* 	} */
#endif
	return JS_TRUE;
}


JSBool
SFImageToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    SFImageNative *ptr;
    JSString *_str;
	char buff[LARGESTRING];
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
#if FALSE
/* 	sprintf(buff, "%d %d %d %s %d", */
/* 			(ptr->v).__x, (ptr->v).__y, */
/* 			(ptr->v).__depth, (ptr->v).__data, (ptr->v).__texture); */
#endif
	_str = JS_NewStringCopyZ(cx, buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}

JSBool
SFImageAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    SFImageNative *ptr, *fptr;
    JSObject *o;

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
    SFImageNative *ptr;
    int t;
	UNUSED(argc);

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFSFImageTouched.\n");
		return JS_FALSE;
	}

    if (!JS_InstanceOf(cx, obj, &SFImageClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFImageTouched.\n");
        return JS_FALSE;
	}
    t = ptr->touched;
	ptr->touched = 0;
    if (verbose) {
		printf("SFImageTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFImageConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SFImageNative *ptr;
	UNUSED(argv);
#if FALSE
/* 	jsint pars[4]; */
/* 	char *cpars = 0; */
/* 	size_t _len = 0, _data_s = 0; */
#endif

	if ((ptr = (SFImageNative *) SFImageNativeNew()) == NULL) {
		fprintf(stderr, "SFImageNativeNew failed in SFImageConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFImageProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFImageConstr.\n");
		return JS_FALSE;
	}

	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFImageConstr.\n");
		return JS_FALSE;
	}
     	
#if FALSE
/* 	if (argc == 0) { */
/* 		(ptr->v).__x = 0; */
/* 		(ptr->v).__y = 0; */
/* 		(ptr->v).__depth = 0; */
/* 		(ptr->v).__data = ""; */
/* 		(ptr->v).__texture = 0; */
/* 	} else { */
/* 		if (!JS_ConvertArguments(cx, argc, argv, "i i i s i", */
/* 								 &(pars[0]), &(pars[1]), &(pars[2]), */
/* 								 cpars, &(pars[3]))) { */
/* 			fprintf(stderr, "JS_ConvertArguments failed in SFImageConstr.\n"); */
/* 			return JS_FALSE; */
/* 		} */
/* 		(ptr->v).__x = pars[0]; */
/* 		(ptr->v).__y = pars[1]; */
/* 		(ptr->v).__depth = pars[2]; */

/* 		_len = strlen(cpars); */
/* 		_data_s = sizeof((ptr->v).__data); */
/* 		if (_len * sizeof(char) > _data_s) { */
/* 			_data_s = (_len + 1) * sizeof(char); */
/* 			if (((ptr->v).__data = */
/* 				 (char *) realloc((ptr->v).__data, _data_s)) */
/* 				== NULL) { */
/* 				fprintf(stderr, */
/* 						"realloc failed in SFImageConstr.\n"); */
/* 				return JS_FALSE; */
/* 			} */
/* 		} */
/* 		memset((ptr->v).__data, 0, _len + 1); */
/* 		memmove((ptr->v).__data, cpars, _len); */

/* 		(ptr->v).__texture = pars[3]; */
/* 	} */
#endif
	if (verbose) {
#if FALSE
/* 		printf("SFImageConstr: obj = %u, %u args, %d %d %d %s %d\n", */
/* 			   (unsigned int) obj, argc, */
/* 			   (ptr->v).__x, (ptr->v).__y, */
/* 			   (ptr->v).__depth, (ptr->v).__data, (ptr->v).__texture); */
#endif
		printf("SFImageConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	*rval = OBJECT_TO_JSVAL(obj);
		
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
	SFVec2fNative *ptr;
	jsdouble d, *dp;

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
	SFVec2fNative *ptr;
	jsval myv;

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
    SFVec2fNative *ptr;
    JSString *_str;
	char buff[STRING];

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
	sprintf(buff, "%.4g %.4g",
			(ptr->v).c[0], (ptr->v).c[1]);
	_str = JS_NewStringCopyZ(cx, buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}

JSBool
SFVec2fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    SFVec2fNative *fptr, *ptr;
    JSObject *o;

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
    SFVec2fNative *ptr;
    int t;

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
		printf("SFVec2fTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFVec2fSubtract(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SFVec2fNative *vec1, *vec2, *res;
	JSObject *o, *proto, *ro;
		
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
	SFVec2fNative *vec, *res;
	JSObject *ro, *proto;
	struct pt v, ret;
/* 	double xx; */

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
	SFVec2fNative *vec1, *vec2, *res;
	JSObject *v2, *proto, *ro;

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
	SFVec2fNative *vec;
	JSObject *proto;
	jsdouble result, *dp;
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
	SFVec2fNative *vec, *res;
	JSObject *ro, *proto;

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
	SFVec2fNative *ptr;
	jsdouble pars[2];

	if ((ptr = (SFVec2fNative *) SFVec2fNativeNew()) == NULL) {
		fprintf(stderr, "SFVec2fNativeNew failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFVec2fProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).c[0] = 0.0;
		(ptr->v).c[1] = 0.0;
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
		printf("SFVec2fConstr: obj = %u, %u args, %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1]);
	}
	*rval = OBJECT_TO_JSVAL(obj);
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
	SFVec3fNative *ptr;
	jsdouble d, *dp;

	if (verbose) {
		printf("SFVec3fSetProperty: obj = %u, id = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id));
	}

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
	SFVec3fNative *ptr;
	jsval myv;

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
    SFVec3fNative *ptr;
    JSString *_str;
	char buff[STRING];

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
	sprintf(buff, "%.4g %.4g %.4g",
			(ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	_str = JS_NewStringCopyZ(cx, buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}

JSBool
SFVec3fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    SFVec3fNative *fptr, *ptr;
    JSObject *o;

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
    SFVec3fNative *ptr;
    int t;

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
		printf("SFVec3fTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFVec3fSubtract(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SFVec3fNative *vec1, *vec2, *res;
	JSObject *o, *proto, *ro;
		
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
	SFVec3fNative *vec, *res;
	JSObject *ro, *proto;
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
	SFVec3fNative *vec1, *vec2, *res;
	JSObject *v2, *proto, *ro;

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
	SFVec3fNative *vec;
	JSObject *proto;
	jsdouble result, *dp;
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
	SFVec3fNative *vec, *res;
	JSObject *v2, *proto, *ro;
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
	SFVec3fNative *vec, *res;
	JSObject *ro, *proto;

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
	SFVec3fNative *ptr;
	jsdouble pars[3];

	if ((ptr = (SFVec3fNative *) SFVec3fNativeNew()) == NULL) {
		fprintf(stderr, "SFVec3fNativeNew failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFVec3fProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).c[0] = 0.0;
		(ptr->v).c[1] = 0.0;
		(ptr->v).c[2] = 0.0;
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
		printf("SFVec3fConstr: obj = %u, %u args, %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	}
	*rval = OBJECT_TO_JSVAL(obj);
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
	SFRotationNative *ptr;
	jsdouble d, *dp;

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
	SFRotationNative *ptr;
	jsval myv;

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
    SFRotationNative *ptr;
    JSString *_str;
	char buff[STRING];

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
	sprintf(buff, "%.4g %.4g %.4g %.4g",
			(ptr->v).r[0], (ptr->v).r[1], (ptr->v).r[2], (ptr->v).r[3]);
	_str = JS_NewStringCopyZ(cx, buff);
	    
    *rval = STRING_TO_JSVAL(_str);
    return JS_TRUE;
}

JSBool
SFRotationAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    SFRotationNative *fptr, *ptr;
    JSObject *o;

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
    SFRotationNative *ptr;
    int t;

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
		printf("SFRotationTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}


JSBool
SFRotationMultVec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SFRotationNative *rfrom;
	SFVec3fNative *vfrom, *vto;
	JSObject *o, *ro, *proto;
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
	SFRotationNative *rfrom, *rto;
	JSObject *o, *proto;
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
	SFVec3fNative *vec;
	SFRotationNative *ptr;
	JSObject *_ob1, *_ob2;
	jsdouble pars[4];
	float v1len, v2len;
	double v12dp;
	struct pt v1, v2;
/* 	JSFunction *_f; */

	if ((ptr = SFRotationNativeNew()) == NULL) {
		fprintf(stderr, "SFRotationNativeNew failed in SFRotationConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFRotationProperties)) {
		fprintf(stderr, "JS_DefineProperties failed in SFRotationConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		fprintf(stderr, "JS_SetPrivate failed in SFRotationConstr.\n");
		return JS_FALSE;
	}

	if (argc == 0) {
		(ptr->v).r[0] = 0.0;
		(ptr->v).r[1] = 0.0;
		(ptr->v).r[2] = 1.0;
		(ptr->v).r[3] = 0.0;
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
		printf("SFRotationConstr: obj = %u, %u args, %f %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).r[0], (ptr->v).r[1], (ptr->v).r[2], (ptr->v).r[3]);
	}
	*rval = OBJECT_TO_JSVAL(obj);
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
SFNodeConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char *_c_str;

	if (argc == 1 &&
		JS_ConvertArguments(cx, argc, argv, "s", &_c_str)) {
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
			   (unsigned int) obj, argc, _c_str);
	}
	*rval = OBJECT_TO_JSVAL(obj);
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
SFNodeAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if (verbose) {
		printf("SFNodeAssign: obj = %u, argc = %u\n",
			   (unsigned int) obj, argc);
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
SFNodeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *globalObj;
	JSString *_idStr;
	BrowserNative *brow;
	char *_id_c;
	jsval _b_val, _obj_val = OBJECT_TO_JSVAL(obj);

	globalObj = JS_GetGlobalObject(cx);
	if (globalObj == NULL) {
		fprintf(stderr, "JS_GetGlobalObject failed in SFNodeSetProperty.\n");
		return JS_FALSE;
	}
	if (!JS_GetProperty(cx, globalObj, "Browser", &_b_val)) {
		fprintf(stderr, "JS_GetProperty failed for \"Browser\" in SFNodeSetProperty.\n");
		return JS_FALSE;
	}
	if (!JSVAL_IS_OBJECT(_b_val)) {
		fprintf(stderr, "\"Browser\" property is not an object in SFNodeSetProperty.\n");
		return JS_FALSE;
	}
	
	if ((brow = JS_GetPrivate(cx, JSVAL_TO_OBJECT(_b_val))) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFNodeSetProperty.\n");
		return JS_FALSE;
	}

	if (!JS_SetProperty(cx, globalObj, "__node", &_obj_val)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__node\" in SFNodeSetProperty.\n");
		return JS_FALSE;
	}

	_idStr = JS_ValueToString(cx, id);
	_id_c = JS_GetStringBytes(_idStr);

	if (!JS_SetProperty(cx, globalObj, _id_c, vp)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"%s\" in SFNodeSetProperty.\n",
				_id_c);
		return JS_FALSE;
	}

	doPerlCallMethodVA(brow->sv_js, "nodeSetProperty", "s", _id_c);
	if (verbose) {
		printf("SFNodeSetProperty: obj = %u, id = %s\n",
			   (unsigned int) obj, _id_c);
	}

	return JS_TRUE;
}



JSBool
MFBoolAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFBoolAddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFBoolGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFBoolGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFBoolGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFBoolGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			*vp = BOOLEAN_TO_JSVAL(JS_FALSE);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFBoolGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFBoolGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFBoolGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFBoolGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
}

JSBool 
MFBoolSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFBoolSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFBoolConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSBool _b;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFBoolConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFBoolConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFBoolConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToBoolean(cx, argv[i], &_b)) {
			fprintf(stderr,
					"JS_ValueToBoolean failed in MFBoolConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %u in MFBoolConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFBoolAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    unsigned int len, i;

	if (verbose) {
		printf("MFBoolAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFBoolClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFBoolAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFBoolAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFBoolClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFBoolAssign.\n");
        return JS_FALSE;
    }

	/* Now, we assign length properties from o to obj (XXX HERE) */
	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFBoolAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFBoolAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFBoolAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val); /* XXX Assume int */

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFBoolAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFBoolAssign.\n", i);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}



JSBool
MFFloatAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFFloatAddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFFloatGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFFloatGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFFloatGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFFloatGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			*vp = DOUBLE_TO_JSVAL(0.0);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFFloatGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFFloatGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFFloatGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFFloatGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
}

JSBool 
MFFloatSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFFloatSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFFloatConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble _d;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFFloatConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFFloatConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFFloatConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToNumber(cx, argv[i], &_d)) {
			fprintf(stderr,
					"JS_ValueToNumber failed in MFFloatConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %u in MFFloatConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFFloatAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int len, i;

	if (verbose) {
		printf("MFFloatAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFFloatClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFFloatAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFFloatAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFFloatClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFFloatAssign.\n");
        return JS_FALSE;
    }

	/* Now, we assign length properties from o to obj (XXX HERE) */
	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFFloatAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFFloatAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFFloatAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val); /* XXX Assume int */

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFFloatAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFFloatAssign.\n", i);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}



JSBool
MFTimeAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFTimeAddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFTimeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFTimeGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFTimeGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFTimeGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			*vp = DOUBLE_TO_JSVAL(0.0);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFTimeGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFTimeGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFTimeGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFTimeGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
}

JSBool 
MFTimeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFTimeSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFTimeConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble _d;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFTimeConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFTimeConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFTimeConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToNumber(cx, argv[i], &_d)) {
			fprintf(stderr,
					"JS_ValueToNumber failed in MFTimeConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %u in MFTimeConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFTimeAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    unsigned int len, i;

	if (verbose) {
		printf("MFTimeAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFTimeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFTimeAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFTimeAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFTimeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFTimeAssign.\n");
        return JS_FALSE;
    }

	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFTimeAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFTimeAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFTimeAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFTimeAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFTimeAssign.\n", i);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}



JSBool
MFInt32AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFInt32AddProperty:\n");
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFInt32GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFInt32GetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFInt32GetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFInt32GetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			*vp = INT_TO_JSVAL(0);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFInt32GetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFInt32GetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFInt32GetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFInt32GetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
}

JSBool 
MFInt32SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFInt32SetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}


JSBool
MFInt32Constr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int32 _i;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFInt32Constr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"__touched_flag\" in MFInt32Constr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (verbose) {
		printf("MFInt32Constr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToInt32(cx, argv[i], &_i)) {
			fprintf(stderr,
					"JS_ValueToBoolean failed in MFInt32Constr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %u in MFInt32Constr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFInt32Assign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    unsigned int len, i;

	if (verbose) {
		printf("MFInt32Assign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFInt32Class, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFInt32Assign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFInt32Assign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFInt32Class, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFInt32Assign.\n");
        return JS_FALSE;
    }

	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFInt32Assign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFInt32Assign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFInt32Assign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFInt32Assign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFInt32Assign.\n", i);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
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
MFColorGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	JSObject *_obj;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFColorGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFColorGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFColorGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			if ((_obj =
				 JS_ConstructObject(cx, &SFColorClass, proto_SFColor, NULL))
				== NULL) {
				fprintf(stderr,
						"JS_ConstructObject failed in MFColorGetProperty.\n");
				return JS_FALSE;
			}
			*vp = OBJECT_TO_JSVAL(_obj);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFColorGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFColorGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFColorGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFColorGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
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
	JSObject *_obj;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
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
		printf("MFColorConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			fprintf(stderr,
					"JS_ValueToObject failed in MFColorConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFColorClass, NULL)) {
			fprintf(stderr, "JS_InstanceOf failed in MFColorConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %u in MFColorConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFColorAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    unsigned int len, i;

	if (verbose) {
		printf("MFColorAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFColorAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFColorAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFColorAssign.\n");
        return JS_FALSE;
    }

	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFColorAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFColorAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFColorAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFColorAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, (jsint) i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFColorAssign.\n", i);
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
MFVec2fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	JSObject *_obj;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFVec2fGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFVec2fGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFVec2fGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			if ((_obj =
				 JS_ConstructObject(cx, &SFVec2fClass, proto_SFVec2f, NULL))
				== NULL) {
				fprintf(stderr,
						"JS_ConstructObject failed in MFVec2fGetProperty.\n");
				return JS_FALSE;
			}
			*vp = OBJECT_TO_JSVAL(_obj);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFVec2fGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFVec2fGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFVec2fGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFVec2fGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
}

JSBool 
MFVec2fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (verbose) {
		printf("MFVec2fSetProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFSetProperty(cx, obj, id, vp);
}

JSBool
MFVec2fConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);
 
	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFVec2fConstr.\n");
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
		printf("MFVec2fConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			fprintf(stderr,
					"JS_ValueToObject failed in MFVec2fConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFVec2fClass, NULL)) {
			fprintf(stderr, "JS_InstanceOf failed in MFVec2fConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %d in MFVec2fConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFVec2fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int len, i;

	if (verbose) {
		printf("MFVec2fAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFVec2fAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFVec2fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFVec2fAssign.\n");
        return JS_FALSE;
    }

	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFVec2fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFVec2fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFVec2fAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFVec2fAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFVec2fAssign.\n", i);
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
MFVec3fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	JSObject *_obj;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFVec3fGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFVec3fGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFVec3fGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			if ((_obj =
				 JS_ConstructObject(cx, &SFVec3fClass, proto_SFVec3f, NULL))
				== NULL) {
				fprintf(stderr,
						"JS_ConstructObject failed in MFVec3fGetProperty.\n");
				return JS_FALSE;
			}
			*vp = OBJECT_TO_JSVAL(_obj);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFVec3fGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFVec3fGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFVec3fGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFVec3fGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
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
	JSObject *_obj;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFVec3fConstr.\n");
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
		printf("MFVec3fConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			fprintf(stderr,
					"JS_ValueToObject failed in MFVec3fConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFVec3fClass, NULL)) {
			fprintf(stderr, "JS_InstanceOf failed in MFVec3fConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %d in MFVec3fConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFVec3fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int len, i;

	if (verbose) {
		printf("MFVec3fAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFVec3fAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFVec3fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFVec3fAssign.\n");
        return JS_FALSE;
    }

	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFVec3fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFVec3fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFVec3fAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFVec3fAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFVec3fAssign.\n", i);
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
MFRotationGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	JSObject *_obj;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFRotationGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFRotationGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFRotationGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			if ((_obj =
				 JS_ConstructObject(cx, &SFRotationClass, proto_SFRotation, NULL))
				== NULL) {
				fprintf(stderr,
						"JS_ConstructObject failed in MFRotationGetProperty.\n");
				return JS_FALSE;
			}
			*vp = OBJECT_TO_JSVAL(_obj);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFRotationGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFRotationGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFRotationGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFRotationGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
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
	JSObject *_obj;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
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
		printf("MFRotationConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			fprintf(stderr,
					"JS_ValueToObject failed in MFRotationConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFRotationClass, NULL)) {
			fprintf(stderr, "JS_InstanceOf failed in MFRotationConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %d in MFRotationConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFRotationAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int len, i;

	if (verbose) {
		printf("MFRotationAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFRotationAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFRotationAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFRotationAssign.\n");
        return JS_FALSE;
    }

	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFRotationAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFRotationAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"length\" in MFRotationAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFRotationAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFRotationAssign.\n", i);
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
MFNodeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	JSObject *_obj;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFNodeGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFNodeGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFNodeGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			if ((_obj =
				 JS_ConstructObject(cx, &SFNodeClass, proto_SFNode, NULL))
				== NULL) {
				fprintf(stderr,
						"JS_ConstructObject failed in MFNodeGetProperty.\n");
				return JS_FALSE;
			}
			*vp = OBJECT_TO_JSVAL(_obj);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFNodeGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFNodeGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFNodeGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFNodeGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
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
	JSObject *_obj;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

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
		printf("MFNodeConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			fprintf(stderr,
					"JS_ValueToObject failed in MFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFNodeClass, NULL)) {
			fprintf(stderr, "JS_InstanceOf failed in MFNodeConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %d in MFNodeConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFNodeAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int len, i;

	if (verbose) {
		printf("MFNodeAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFNodeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFNodeAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFNodeAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFNodeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFNodeAssign.\n");
        return JS_FALSE;
    }

	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFNodeAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFNodeAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFNodeAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFNodeAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFNodeAssign.\n", i);
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
MFStringGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str, *_sstr;
	char *_c;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFStringGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_c = JS_GetStringBytes(_str);
		if (verbose) {
			printf("MFStringGetProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _c);
		}
	} else if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (verbose) {
			printf("MFStringGetProperty: obj = %u, id = %d\n",
				   (unsigned int) obj, _index);
		}
		if (_index >= _length) { /* fill in new objects here ??? */
			_sstr = JS_NewStringCopyZ(cx, "");
			*vp = STRING_TO_JSVAL(_sstr);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				fprintf(stderr,
						"JS_DefineElement failed in MFStringGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFStringGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFStringGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	} else {
		if (verbose) {
			fprintf(stderr,
					"Unknown property of %u in MFStringGetProperty.\n",
				   (unsigned int) obj);
		}
		*vp = INT_TO_JSVAL(0);
		return JS_FALSE;
	}
	return JS_TRUE;
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
	JSString *_str;
	char *_c;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		fprintf(stderr,
				"JS_DefineProperty failed for \"length\" in MFStringConstr.\n");
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
		printf("MFStringConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if ((_str = JS_ValueToString(cx, argv[i])) == NULL) {
			fprintf(stderr,
					"JS_ValueToString failed in MFStringConstr.\n");
			return JS_FALSE;
		}
		_c = JS_GetStringBytes(_str);
		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			fprintf(stderr,
					"JS_DefineElement failed for arg %d in MFStringConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFStringAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int len, i;

	if (verbose) {
		printf("MFStringAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFStringClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFStringAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_from_obj)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFStringAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFStringClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFStringAssign.\n");
        return JS_FALSE;
    }

	myv = INT_TO_JSVAL(1);
    if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in MFStringAssign.\n");
        return JS_FALSE;
	}
    if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		fprintf(stderr, "JS_GetProperty failed for \"length\" in MFStringAssign.\n");
        return JS_FALSE;
	}
    if (!JS_SetProperty(cx, obj, "length", &val)) {
		fprintf(stderr, "JS_SetProperty failed for \"length\" in MFStringAssign.\n");
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(val);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, _from_obj, i, &val)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in MFStringAssign.\n", i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, obj, i, &val)) {
			fprintf(stderr,
					"JS_SetElement failed for %d in MFStringAssign.\n", i);
			return JS_FALSE;
		}
    }
    *rval = OBJECT_TO_JSVAL(obj); 
    return JS_TRUE;
}
