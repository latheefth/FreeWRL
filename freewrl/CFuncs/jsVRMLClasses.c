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


static JSBool
doMFToString(JSContext *cx, JSObject *obj, const char *className, jsval *rval)
{
    JSString *_str, *_tmpStr;
    jsval _v;
	char *_buff, *_tmp_valStr, *_tmp_buff;
	const char *_empty_array = "[]";
    int len = 0, i;
	size_t buff_size = 0, tmp_valStr_len = 0, tmp_buff_len = 0, className_len = 0;
	JSBool isString = JS_FALSE;

    if (!JS_GetProperty(cx, obj, "length", &_v)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in doMFToString for %s.\n",
				className);
        return JS_FALSE;
	}
    len = JSVAL_TO_INT(_v);

	if (len == 0) {
		_str = JS_NewStringCopyZ(cx, _empty_array);
		*rval = STRING_TO_JSVAL(_str);
		return JS_TRUE;
	}

	className_len = strlen(className);
	if (!strncmp(className, "MFString", className_len)) {
		isString = JS_TRUE;
	}

	buff_size = LARGESTRING;
	if ((_buff = (char *)
		 malloc(buff_size * sizeof(char))) == NULL) {
			fprintf(stderr,
					"malloc failed in doMFToString for %s.\n",
					className);
			return JS_FALSE;
	}
	memset(_buff, 0, buff_size);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, obj, i, &_v)) {
			fprintf(stderr,
					"JS_GetElement failed for %d in doMFToString for %s.\n",
						i, className);
			return JS_FALSE;
		}
		_tmpStr = JS_ValueToString(cx, _v);
		_tmp_valStr = JS_GetStringBytes(_tmpStr);
		tmp_valStr_len = strlen(_tmp_valStr) + 1;
		tmp_buff_len = strlen(_buff);

		if ((buff_size - (tmp_buff_len + 1)) < (tmp_valStr_len + 1)) {
			buff_size += LARGESTRING;
			if ((_buff =
				 (char *)
				 JS_realloc(cx, _buff, buff_size * sizeof(char *)))
				== NULL) {
				fprintf(stderr,
						"JS_realloc failed for %d in doMFToString for %s.\n",
						i, className);
				return JS_FALSE;
			}
		}

		if (len == 1) {
			if (isString) {
				sprintf(_buff, "[ \"%.*s\" ]", tmp_valStr_len, _tmp_valStr);
			} else {
				sprintf(_buff, "[ %.*s ]", tmp_valStr_len, _tmp_valStr);
			}
			break;
		}

		if ((_tmp_buff = (char *)
			 malloc((tmp_buff_len + 1) * sizeof(char))) == NULL) {
			fprintf(stderr,
					"malloc failed for %d in doMFToString for %s.\n",
					i, className);
			return JS_FALSE;
		}
		memset(_tmp_buff, 0, tmp_buff_len + 1);
		memmove(_tmp_buff, _buff, tmp_buff_len);
		memset(_buff, 0, buff_size);

		if (i == 0 && len > 1) {
			if (isString) {
				sprintf(_buff, "[ \"%.*s\"",
						tmp_valStr_len, _tmp_valStr);
			} else {
				sprintf(_buff, "[ %.*s",
						tmp_valStr_len, _tmp_valStr);
			}
		} else if (i == (len - 1)) {
			if (isString) {
				sprintf(_buff,
						"%.*s, \"%.*s\" ]",
						tmp_buff_len, _tmp_buff, tmp_valStr_len, _tmp_valStr);
			} else {
				sprintf(_buff,
						"%.*s, %.*s ]",
						tmp_buff_len, _tmp_buff, tmp_valStr_len, _tmp_valStr);
			}
		} else {
			if (isString) {
				sprintf(_buff,
						"%.*s, \"%.*s\"",
						tmp_buff_len, _tmp_buff, tmp_valStr_len, _tmp_valStr);
			} else {
				sprintf(_buff,
						"%.*s, %.*s",
						tmp_buff_len, _tmp_buff, tmp_valStr_len, _tmp_valStr);
			}
		}

		free(_tmp_buff);
    }

	_str = JS_NewStringCopyZ(cx, _buff);
	*rval = STRING_TO_JSVAL(_str);

	free(_buff);

    return JS_TRUE;
}


static JSBool
doMFAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *str, *sstr;
	jsval v;
	jsval myv;
	char *p, *pp;
	size_t p_len = 0;
	int len = 0, ind = JSVAL_TO_INT(id);

	if (JSVerbose) {
		printf("\tdoMFAddProperty: ");
	}

	str = JS_ValueToString(cx, id);
	p = JS_GetStringBytes(str);

	p_len = strlen(p);
	if (!strncmp(p, "length", p_len) || !strncmp(p, "toString", p_len) ||
		!strncmp(p, "__touched_flag", p_len) || !strncmp(p, "assign", p_len) ||
		!strncmp(p, "constructor", p_len)) {
		if (JSVerbose) {
			printf("property \"%s\" is one of \"length\", \"constructor\", \"assign\", \"__touched_flag\", \"toString\". Do nothing.\n", p);
		}
		return JS_TRUE;
	}

	if (JSVerbose) {
		sstr = JS_ValueToString(cx, *vp);
		pp = JS_GetStringBytes(sstr);

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
	if (ind >= len) {
		len = ind + 1;
		v = INT_TO_JSVAL(len);
		if (!JS_SetProperty(cx, obj, "length", &v)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"length\" in doMFAddProperty.\n");
			return JS_FALSE;
		}
	}
	if (JSVerbose) {
		printf("index = %d, length = %d\n", ind, len);
	}

	myv = INT_TO_JSVAL(1);
	if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		fprintf(stderr,
				"JS_SetProperty failed for \"__touched_flag\" in doMFAddProperty.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}


static JSBool
doMFSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str, *_sstr;
	char *_c, *_cc;
	jsval myv;
	jsint _index;

	if (JSVerbose) {
		_str = JS_ValueToString(cx, id);
		_c = JS_GetStringBytes(_str);

		_sstr = JS_ValueToString(cx, *vp);
		_cc = JS_GetStringBytes(_sstr);
		printf("\tdoMFSetProperty: obj = %u, id = %s, vp = %s\n",
			   (unsigned int) obj, _c, _cc);
	}

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		myv = INT_TO_JSVAL(1);
		if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"__touched_flag\" in doMFSetProperty.\n");
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}


static JSBool
getBrowser(JSContext *context, JSObject *obj, BrowserNative **brow)
{
	jsval _b_val;

	if (!JS_GetProperty(context, obj, "Browser", &_b_val)) {
		fprintf(stderr, "JS_GetProperty failed for \"Browser\" in getBrowser.\n");
		return JS_FALSE;
	}
	if (!JSVAL_IS_OBJECT(_b_val)) {
		fprintf(stderr, "\"Browser\" property is not an object in getBrowser.\n");
		return JS_FALSE;
	}
	
	if ((*brow = JS_GetPrivate(context, JSVAL_TO_OBJECT(_b_val))) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in getBrowser.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}


static JSBool
doMFStringUnquote(JSContext *cx, jsval *vp)
{
	JSString *_str, *_vpStr;
	char *_buff, *_tmp_vpStr;
	size_t _buff_len;
	unsigned int i, j = 0;

	_str = JS_ValueToString(cx, *vp);
	_buff = JS_GetStringBytes(_str);
	_buff_len = strlen(_buff) + 1;

	if (JSVerbose) {
		printf("doMFStringUnquote: vp = \"%s\"\n", _buff);
	}

	if (memchr(_buff, '"', _buff_len) != NULL) {
		if ((_tmp_vpStr = (char *)
			 malloc(_buff_len * sizeof(char))) == NULL) {
			fprintf(stderr, "malloc failed in doMFStringUnquote.\n");
			return JS_FALSE;
		}

		memset(_tmp_vpStr, 0, _buff_len);

		for (i = 0; i <= _buff_len; i++) {
			if (_buff[i] != '"' ||
				(i > 0 && _buff[i - 1] == '\\')) {
				_tmp_vpStr[j++] = _buff[i];
			}
		}

		_vpStr = JS_NewStringCopyZ(cx, _tmp_vpStr);
		*vp = STRING_TO_JSVAL(_vpStr);

		free(_tmp_vpStr);
	}

	return JS_TRUE;
}



JSBool
globalResolve(JSContext *cx, JSObject *obj, jsval id) 
{
	UNUSED(cx);
	UNUSED(obj);
	UNUSED(id);
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
									  SFVec2fConstr, INIT_ARGC, NULL,
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
										 SFRotationConstr, INIT_ARGC,
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
									  SFImageConstr, INIT_ARGC, NULL,
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

	if ((proto_SFNode = JS_InitClass(context, globalObj, NULL, &SFNodeClass,
									 SFNodeConstr, INIT_ARGC_NODE, NULL,
									 SFNodeFunctions, NULL, NULL)) == NULL) {
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

	if ((proto_MFFloat = JS_InitClass(context, globalObj, NULL, &MFFloatClass,
									  MFFloatConstr, INIT_ARGC, NULL,
									  MFFloatFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFFloatClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFFloat);
	if (!JS_SetProperty(context, globalObj, "__MFFloat_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFFloatClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFTime = JS_InitClass(context, globalObj, NULL, &MFTimeClass,
									 MFTimeConstr, INIT_ARGC, NULL,
									 MFTimeFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFTimeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFTime);
	if (!JS_SetProperty(context, globalObj, "__MFTime_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFTimeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFInt32 = JS_InitClass(context, globalObj, NULL, &MFInt32Class,
									  MFInt32Constr, INIT_ARGC, NULL,
									  MFInt32Functions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFInt32Class failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFInt32);
	if (!JS_SetProperty(context, globalObj, "__MFInt32_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFInt32Class failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFColor = JS_InitClass(context, globalObj, NULL, &MFColorClass,
									  MFColorConstr, INIT_ARGC, NULL,
									  MFColorFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFColor);
	if (!JS_SetProperty(context, globalObj, "__MFColor_proto", &v)) {
		fprintf(stderr,
				"JS_SetProperty for MFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFVec2f = JS_InitClass(context, globalObj, NULL, &MFVec2fClass,
									  MFVec2fConstr, INIT_ARGC, NULL,
									  MFVec2fFunctions, NULL, NULL)) == NULL) {
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

	if ((proto_MFVec3f = JS_InitClass(context, globalObj, NULL, &MFVec3fClass,
									  MFVec3fConstr, INIT_ARGC, NULL,
									  MFVec3fFunctions, NULL, NULL)) == NULL) {
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

	if ((proto_MFRotation = JS_InitClass(context, globalObj, NULL, &MFRotationClass,
										 MFRotationConstr, INIT_ARGC, NULL,
										 MFRotationFunctions, NULL, NULL)) == NULL) {
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
	
	if ((proto_MFNode = JS_InitClass(context, globalObj, NULL, &MFNodeClass,
									 MFNodeConstr, INIT_ARGC, NULL,
									 MFNodeFunctions, NULL, NULL)) == NULL) {
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

	if ((proto_MFString = JS_InitClass(context, globalObj, NULL, &MFStringClass,
									   MFStringConstr, INIT_ARGC, NULL,
									   MFStringFunctions, NULL, NULL)) == NULL) {
		fprintf(stderr,
				"JS_InitClass for MFStringClass failed in loadVrmlClasses.\n");
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
	JSString *_idStr, *_vpStr, *_newVpStr;
	JSBool ret = JS_TRUE;
	jsval v;
	char *_id_c, *_vp_c, *_new_vp_c, *_buff;
	const size_t touched_len = 10;
	size_t len = 0;

	_idStr = JS_ValueToString(context, id);
	_id_c = JS_GetStringBytes(_idStr);
	
	if (JSVAL_IS_STRING(*vp)) {
		_vpStr = JS_ValueToString(context, *vp);
		_vp_c = JS_GetStringBytes(_vpStr);

		len = strlen(_vp_c);
		/* len + 3 for '\0' and "..." */
		if ((_new_vp_c = (char *) malloc((len + 3) * sizeof(char))) == NULL) {
			fprintf(stderr, "malloc failed in setECMANative.\n");
			return JS_FALSE;
		}
		len += 3;

		memset(_new_vp_c, 0, len);
		sprintf(_new_vp_c, "\"%.*s\"", len, _vp_c);
		_newVpStr = JS_NewStringCopyZ(context, _new_vp_c);
		*vp = STRING_TO_JSVAL(_newVpStr);

		if (JSVerbose) {
			printf("setECMANative: obj = %u, id = \"%s\", vp = %s\n",
				   (unsigned int) obj, _id_c, _new_vp_c);
		}
		free(_new_vp_c);
	} else {
		if (JSVerbose) {
			_vpStr = JS_ValueToString(context, *vp);
			_vp_c = JS_GetStringBytes(_vpStr);
			printf("setECMANative: obj = %u, id = \"%s\", vp = %s\n",
				   (unsigned int) obj, _id_c, _vp_c);
		}
	}

	len = strlen(_id_c);
	if (len + touched_len >= STRING) {
		len += SMALLSTRING;
	} else {
		len = STRING;
	}
	if ((_buff = (char *) malloc(len * sizeof(char))) == NULL) {
		fprintf(stderr, "malloc failed in setECMANative.\n");
		return JS_FALSE;
	}
	memset(_buff, 0, len);
	sprintf(_buff, "_%.*s_touched", len, _id_c);
	v = INT_TO_JSVAL(1);
	if (!JS_SetProperty(context, obj, _buff, &v)) {
		fprintf(stderr,
				"JS_SetProperty failed in setECMANative.\n");
		ret = JS_FALSE;
	}
	free(_buff);

	return ret;
}


/* used mostly for debugging */
JSBool
getAssignProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_idStr, *_vpStr;
	char *_id_c, *_vp_c;

	if (JSVerbose) {
		_idStr = JS_ValueToString(cx, id);
		_id_c = JS_GetStringBytes(_idStr);

		_vpStr = JS_ValueToString(cx, *vp);
		_vp_c = JS_GetStringBytes(_vpStr);

		printf("getAssignProperty: obj = %u, id = \"%s\", vp = %s\n",
			   (unsigned int) obj, _id_c, _vp_c);
	}
	return JS_TRUE;
}


/* a kind of hack to replace the use of JSPROP_ASSIGNHACK */
JSBool
setAssignProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *_o;
	JSString *_str;
	const uintN _argc = 2;
	jsval _newVal, _initVal, _argv[_argc];
	char *_id_c;

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_id_c = JS_GetStringBytes(_str);
		if (!JS_ConvertValue(cx, *vp, JSTYPE_OBJECT, &_newVal)) {
			fprintf(stderr, "JS_ConvertValue failed in setAssignProperty.\n");
			return JS_FALSE;
		}
		if (!JS_GetProperty(cx, obj, _id_c, &_initVal)) {
			fprintf(stderr,
					"JS_GetProperty failed in setAssignProperty.\n");
			return JS_FALSE;
		}
		if (JSVerbose) {
			printf("setAssignProperty: obj = %u, id = \"%s\", from = %ld, to = %ld\n",
				   (unsigned int) obj, _id_c, _newVal, _initVal);
		}
		_o = JSVAL_TO_OBJECT(_initVal);
		_argv[0] = _newVal;
		_argv[1] = id;
		if (!JS_CallFunctionName(cx, _o, "assign", _argc, _argv, vp)) {
			fprintf(stderr,
					"JS_CallFunctionName failed in setAssignProperty.\n");
			return JS_FALSE;
		}
	} else {
		if (JSVerbose) {
			_str = JS_ValueToString(cx, id);
			_id_c = JS_GetStringBytes(_str);
			printf("setAssignProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _id_c);
		}
	}

	return JS_TRUE;
}



/* implement later */
JSBool
SFColorGetHSV(JSContext *cx, JSObject *obj,
				uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_arrayObj;
/* 	SFColorNative *ptr; */
	jsdouble hue = 0, saturation = 0, value = 0;
	jsval _v;

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);
	/* do conversion here!!! */

	if ((_arrayObj = JS_NewArrayObject(cx, 0, NULL)) == NULL) {
		fprintf(stderr, "JS_NewArrayObject failed in SFColorGetHSV.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_arrayObj);

	/* construct new double before conversion? */
	_v = DOUBLE_TO_JSVAL(&hue);
	if (!JS_SetElement(cx, _arrayObj, 0, &_v)) {
		fprintf(stderr, "JS_SetElement failed for hue in SFColorGetHSV.\n");
		return JS_FALSE;
	}
	_v = DOUBLE_TO_JSVAL(&saturation);
	if (!JS_SetElement(cx, _arrayObj, 1, &_v)) {
		fprintf(stderr, "JS_SetElement failed for saturation in SFColorGetHSV.\n");
		return JS_FALSE;
	}

	_v = DOUBLE_TO_JSVAL(&value);
	if (!JS_SetElement(cx, _arrayObj, 2, &_v)) {
		fprintf(stderr, "JS_SetElement failed for value in SFColorGetHSV.\n");
		return JS_FALSE;
	}

    return JS_TRUE;
}

/* implement later */
JSBool
SFColorSetHSV(JSContext *cx, JSObject *obj,
				uintN argc, jsval *argv, jsval *rval)
{
    SFColorNative *ptr;
	jsdouble hue, saturation, value;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFColorToString.\n");
		return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "d d d",
							 &hue, &saturation, &value)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFColorSetHSV.\n");
		return JS_FALSE;
	}

	/* do conversion here!!! */

	*rval = OBJECT_TO_JSVAL(obj);

    return JS_TRUE;
}

JSBool
SFColorToString(JSContext *cx, JSObject *obj,
				uintN argc, jsval *argv, jsval *rval)
{
    SFColorNative *ptr;
    JSString *_str;
	char _buff[STRING];

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFColorToString.\n");
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
SFColorAssign(JSContext *cx, JSObject *obj,
			   uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    SFColorNative *ptr, *fptr;
    char *_id_str;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFColorAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFColorAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFColorAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _from_obj in SFColorAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _from_obj in SFColorAssign.\n");
        return JS_FALSE;
	}
	if (JSVerbose) {
		printf("SFColorAssign: obj = %u, id = \"%s\", from = %u\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj);
	}

    SFColorNativeAssign(ptr, fptr);
    *rval = OBJECT_TO_JSVAL(obj); 

    return JS_TRUE;
}

JSBool
SFColorTouched(JSContext *cx, JSObject *obj,
			   uintN argc, jsval *argv, jsval *rval)
{
    SFColorNative *ptr;
    int t;

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFSFColorTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVerbose) {
		printf("SFColorTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}

JSBool
SFColorConstr(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
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
	if (JSVerbose) {
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

	if (JSVerbose) {
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
	if (JSVerbose) {
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
SFImageToString(JSContext *cx, JSObject *obj,
				uintN argc, jsval *argv, jsval *rval)
{
    SFImageNative *ptr;
    JSString *_str;
	char buff[LARGESTRING];

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFImageToString.\n");
		return JS_FALSE;
	}

	memset(buff, 0, LARGESTRING);
	_str = JS_NewStringCopyZ(cx, buff);
    *rval = STRING_TO_JSVAL(_str);

    return JS_TRUE;
}

JSBool
SFImageAssign(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    SFImageNative *ptr, *fptr;
    char *_id_str;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFImageAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFImageClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFImageAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFImageAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFImageClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _from_obj in SFImageAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _from_obj in SFImageAssign.\n");
        return JS_FALSE;
	}
	if (JSVerbose) {
		printf("SFImageAssign: obj = %u, id = \"%s\", from = %u\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj);
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
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFSFImageTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVerbose) {
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
     	
	if (JSVerbose) {
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

	if (JSVerbose) {
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
SFImageGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	UNUSED(cx);
	UNUSED(obj);
	UNUSED(id);
	UNUSED(vp);

	return JS_TRUE;
}

JSBool
SFImageSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	SFImageNative *ptr;
	UNUSED(id);
	UNUSED(vp);

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFImageSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	return JS_TRUE;
}



/* returns the handle - either "NODExx" or (hopefully) a string rep of the pointer to the node in memory */
JSBool
SFNodeToString(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
    JSString *_str;
    SFNodeNative *ptr;
	char *_buff;
	size_t handle_len = 0;

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFNodeToString.\n");
		return JS_FALSE;
	}

	handle_len = strlen(ptr->handle) + 1;
	if ((_buff = (char *)
		 malloc(handle_len * sizeof(char))) == NULL) {
		fprintf(stderr, "malloc failed in SFNodeToString.\n");
		return JS_FALSE;
	}

	memset(_buff, 0, handle_len);
	sprintf(_buff, "%.*s", handle_len, ptr->handle);
	_str = JS_NewStringCopyZ(cx, _buff);
    *rval = STRING_TO_JSVAL(_str);

	free(_buff);

    return JS_TRUE;
}

JSBool
SFNodeAssign(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_from_obj, *globalObj;
	BrowserNative *brow;
	SFNodeNative *fptr, *ptr;
	char *_id_str;
	jsval _rval;
	JSString *strval;
	char *strtouched;
	unsigned int toptr;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFNodeAssign.\n");
	    return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, obj, &SFNodeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFNodeAssign.\n");
	    return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFNodeAssign.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _from_obj, &SFNodeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _from_obj in SFNodeAssign.\n");
	    return JS_FALSE;
	}
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _from_obj in SFNodeAssign.\n");
	    return JS_FALSE;
	}
	if (JSVerbose) {
		printf("SFNodeAssign: obj = %u, id = \"%s\", from = %u\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj);
	}

	//printf ("SFNodeAssign calling SFNodeNativeAssign\n");
	if (strncmp("NODE",fptr->handle,4) == 0) {
		//printf ("SFNodeAssign, handle is a NODE\n");
		/* try to get the node backend now */

		if ((globalObj = JS_GetGlobalObject(cx)) == NULL) {
			fprintf(stderr, "JS_GetGlobalObject failed in SFNodeAssign.\n");
			return JS_FALSE;
		}
		if (!getBrowser(cx, globalObj, &brow)) {
			fprintf(stderr, "getBrowser failed in SFNodeConstr.\n");
			return JS_FALSE;
		}

		doPerlCallMethodVA(brow->sv_js, "getNodeCNode", "s", fptr->handle);
	       if (!JS_GetProperty(cx, globalObj, "__ret",  &_rval)) {
        	        fprintf(stderr, "JS_GetProperty failed in VrmlBrowserGetVersion.\n");
        	        return JS_FALSE;
        	}

		strval = JS_ValueToString(cx, _rval);
		strtouched = JS_GetStringBytes(strval);

		if (fptr->handle) free (fptr->handle);
		fptr->handle = malloc (strlen(strtouched)+1);
		strncpy (fptr->handle,strtouched,strlen(strtouched));
	}
	
	/* assign this internally */
	if (!SFNodeNativeAssign(ptr, fptr)) {
		fprintf(stderr, "SFNodeNativeAssign failed in SFNodeAssign.\n");
	    return JS_FALSE;
	}

	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
SFNodeTouched(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
    SFNodeNative *ptr;
    int t;

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFNodeTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVerbose) {
		printf("SFNodeTouched: obj = %u, touched = %d\n",
			   (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}

JSBool
SFNodeConstr(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *globalObj;
	BrowserNative *brow;
    SFNodeNative *ptr;
	char *_vrmlstr, *_handle;
	size_t vrmlstring_len = 0, handle_len = 0;
	jsval _obj_val = OBJECT_TO_JSVAL(obj);

	/*
	 * ECMAScript scripting reference requires that a legal VRML
	 * string be a constructor argument.
	 */
	if (argc == 1 && JS_ConvertArguments(cx, argc, argv, "s",
							&_vrmlstr)) {

		vrmlstring_len = strlen(_vrmlstr) + 1;
		handle_len = strlen(NULL_HANDLE) + 1;
		//printf ("SFNodeConstr, argc==1, vrmlstr = %s\n",_vrmlstr);

		if ((ptr = SFNodeNativeNew(vrmlstring_len, handle_len)) == NULL) {
			fprintf(stderr, "SFNodeNativeNew failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_DefineProperties(cx, obj, SFNodeProperties)) {
			fprintf(stderr, "JS_DefineProperties failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_SetPrivate(cx, obj, ptr)) {
			fprintf(stderr, "JS_SetPrivate failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		memset(ptr->vrmlstring, 0, vrmlstring_len);
		memmove(ptr->vrmlstring, _vrmlstr, vrmlstring_len);

		memset(ptr->handle, 0, handle_len);
		memmove(ptr->handle, NULL_HANDLE, handle_len);

		if ((globalObj = JS_GetGlobalObject(cx)) == NULL) {
			fprintf(stderr, "JS_GetGlobalObject failed in SFNodeConstr.\n");
			return JS_FALSE;
		}

		if (!getBrowser(cx, globalObj, &brow)) {
			fprintf(stderr, "getBrowser failed in SFNodeConstr.\n");
			return JS_FALSE;
		}

		if (!JS_SetProperty(cx, globalObj, BROWSER_SFNODE, &_obj_val)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in SFNodeConstr.\n",
					BROWSER_SFNODE);
			return JS_FALSE;
		}

		doPerlCallMethodVA(brow->sv_js, "jspSFNodeConstr", "s", _vrmlstr);
	} else if (argc == 2 && JS_ConvertArguments(cx, argc, argv, "s s",
								   &_vrmlstr, &_handle)) {
		vrmlstring_len = strlen(_vrmlstr) + 1;
		handle_len = strlen(_handle) + 1;

		//printf ("SFNodeConstr, argc==2, vrmlstr = %s\n",_vrmlstr);
		if ((ptr = SFNodeNativeNew(vrmlstring_len, handle_len)) == NULL) {
			fprintf(stderr, "SFNodeNativeNew failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_DefineProperties(cx, obj, SFNodeProperties)) {
			fprintf(stderr, "JS_DefineProperties failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_SetPrivate(cx, obj, ptr)) {
			fprintf(stderr, "JS_SetPrivate failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		memset(ptr->vrmlstring, 0, vrmlstring_len);
		memmove(ptr->vrmlstring, _vrmlstr, vrmlstring_len);

		memset(ptr->handle, 0, handle_len);
		memmove(ptr->handle, _handle, handle_len);
	} else {
		fprintf(stderr,
				"SFNodeConstr requires at least 1 string arg.\n");
		return JS_FALSE;
	}

	*rval = OBJECT_TO_JSVAL(obj);
	if (JSVerbose) {
		printf("SFNodeConstr: obj = %u, argc = %u, vrmlstring=\"%s\", handle=\"%s\"\n",
			   (unsigned int) obj, argc, ptr->vrmlstring, ptr->handle);
	}
	return JS_TRUE;
}

JSBool
SFNodeFinalize(JSContext *cx, JSObject *obj)
{
	SFNodeNative *ptr;

	if (JSVerbose) {
		printf("SFNodeFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFNodeFinalize.\n");
		return JS_FALSE;
	}
	SFNodeNativeDelete(ptr);
	return JS_TRUE;
}

JSBool 
SFNodeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *globalObj;
	JSString *_str, *_idStr, *_valStr;
	BrowserNative *brow;
	SFNodeNative *ptr;
	jsval _rval = 0;
	char *_id_c, *_val_c, *_buff;
	size_t id_len = 0, val_len = 0;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFNodeGetProperty.\n");
		return JS_FALSE;
	}

	_idStr = JS_ValueToString(cx, id);
	_id_c = JS_GetStringBytes(_idStr);
	id_len = strlen(_id_c) + 1;
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			_str = JS_NewStringCopyZ(cx, ptr->vrmlstring);
			*vp = STRING_TO_JSVAL(_str);
			break; 
		case 1:
			_str = JS_NewStringCopyZ(cx, ptr->handle);
			*vp = STRING_TO_JSVAL(_str);
			break; 
		}
	} else if (JSVAL_IS_PRIMITIVE(*vp)) {
		if ((globalObj = JS_GetGlobalObject(cx)) == NULL) {
			fprintf(stderr, "JS_GetGlobalObject failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}

		if (!getBrowser(cx, globalObj, &brow)) {
			fprintf(stderr, "getBrowser failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}

		if ((_buff = (char *) malloc((id_len + STRING) * sizeof(char))) == NULL) {
			fprintf(stderr, "malloc failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}
		val_len = strlen(ptr->handle) + 1;
		sprintf(_buff, "%.*s_%.*s", val_len, ptr->handle, id_len, _id_c);

		if (!JS_SetProperty(cx, globalObj, _buff, vp)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in SFNodeGetProperty.\n",
					_buff);
			return JS_FALSE;
		}

		printf ("SFNodeGetProperty, getting the property for %s\n",ptr->handle);
		//doPerlCallMethodVA(brow->sv_js, "jspSFNodeGetProperty", "ss", _id_c, ptr->handle);

		if (!JS_GetProperty(cx, globalObj, _buff, &_rval)) {
			fprintf(stderr,
					"JS_GetProperty failed in SFNodeGetProperty.\n");
			return JS_FALSE;
		}
		*vp = _rval;
		free(_buff);
	}

	if (JSVerbose &&
		strncmp(_id_c, "toString", 8) != 0 &&
		strncmp(_id_c, "assign", 6) != 0 &&
		strncmp(_id_c, "__touched", 9) != 0) {
		_valStr = JS_ValueToString(cx, *vp);
		_val_c = JS_GetStringBytes(_valStr);

		printf("SFNodeGetProperty: obj = %u, id = %s, vp = %s\n",
			   (unsigned int) obj, _id_c, _val_c);
	}

	return JS_TRUE;
}

JSBool
SFNodeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *globalObj;
	JSString *_idStr, *_valStr;
	BrowserNative *brow;
	SFNodeNative *ptr;
	char *_id_c, *_val_c, *_buff;
	size_t id_len = 0, val_len = 0;

	_idStr = JS_ValueToString(cx, id);
	_id_c = JS_GetStringBytes(_idStr);
	id_len = strlen(_id_c) + 1;

	_valStr = JS_ValueToString(cx, *vp);
	_val_c = JS_GetStringBytes(_valStr);

	if (JSVerbose) {
		printf("SFNodeSetProperty: obj = %u, id = %s, vp = %s\n",
			   (unsigned int) obj, _id_c, _val_c);
	}

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFNodeSetProperty.\n");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		printf ("JS_IS_INT true\n");
		ptr->touched++;
		val_len = strlen(_val_c) + 1;

		if (JSVerbose) printf ("switching on %d\n",JSVAL_TO_INT(id));

		switch (JSVAL_TO_INT(id)) {
		case 0:
			if ((strlen(ptr->vrmlstring) + 1) > val_len) {
				ptr->vrmlstring =
					(char *) realloc(ptr->vrmlstring, val_len * sizeof(char));
			}
			memset(ptr->vrmlstring, 0, val_len);
			memmove(ptr->vrmlstring, _val_c, val_len);
			break;
		case 1:
			if ((strlen(ptr->handle) + 1) > val_len) {
				ptr->handle =
					(char *) realloc(ptr->handle, val_len * sizeof(char));
			}
			memset(ptr->handle, 0, val_len);
			memmove(ptr->handle, _val_c, val_len);
			break;
		}
	} else {
		if (JSVerbose) printf ("JS_IS_INT false\n");
		if ((globalObj = JS_GetGlobalObject(cx)) == NULL) {
			fprintf(stderr, "JS_GetGlobalObject failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}

		if (!getBrowser(cx, globalObj, &brow)) {
			fprintf(stderr, "getBrowser failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}

		if ((_buff = (char *) malloc((id_len + STRING) * sizeof(char))) == NULL) {
			fprintf(stderr, "malloc failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}
		val_len = strlen(ptr->handle) + 1;
		sprintf(_buff, "%.*s_%.*s", val_len, ptr->handle, id_len, _id_c);

		if (!JS_SetProperty(cx, globalObj, _buff, vp)) {
			fprintf(stderr,
					"JS_SetProperty failed for \"%s\" in SFNodeSetProperty.\n",
					_buff);
			return JS_FALSE;
		}

		free(_buff);
	}

	return JS_TRUE;
}



JSBool
SFRotationGetAxis(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_retObj;
	SFRotationNative *_rot;
	SFVec3fNative *_retNative;

	UNUSED(argc);
	UNUSED(argv);
	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, NULL, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFRotationGetAxis.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationGetAxis.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFRotationGetAxis.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = (_rot->v).r[0];
	(_retNative->v).c[1] = (_rot->v).r[1];
	(_retNative->v).c[2] = (_rot->v).r[2];

	if (JSVerbose) {
		printf("SFRotationGetAxis: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

/* implement later */
JSBool
SFRotationInverse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_retObj, *_proto;
	SFRotationNative *_rot, *_retNative;

	UNUSED(argc);
	UNUSED(argv);
	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFRotationInverse.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFRotationClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFRotationInverse.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationInverse.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFRotationInverse.\n");
		return JS_FALSE;
	}

	/* calculation correct? */
/* 	_retNative->v.r[0] = (_rot->v).r[0]; */
/* 	_retNative->v.r[1] = (_rot->v).r[1]; */
/* 	_retNative->v.r[2] = (_rot->v).r[2]; */
/* 	_retNative->v.r[3] = -(_rot->v).r[3]; */
	fprintf(stderr, "SFRotation's inverse function does nothing!\n");

	return JS_TRUE;
}

/* implement later */
JSBool
SFRotationMultiply(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_multObj, *_proto, *_retObj;
	SFRotationNative *_rot1, *_rot2, *_retNative;

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_multObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFRotationMultiply.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _multObj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationMultiply.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _multObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFRotationMultiply.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFRotationClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFRotationMultiply.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationMultiply.\n");
		return JS_FALSE;
	}

	if ((_rot2 = JS_GetPrivate(cx, _multObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _multObj in SFRotationMultiply.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFRotationMultiply.\n");
		return JS_FALSE;
	}

	fprintf(stderr, "SFRotation's multiply function does nothing!\n");

	return JS_TRUE;
}

JSBool
SFRotationMultVec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_multObj, *_retObj, *_proto;
	SFRotationNative *_rot;
	SFVec3fNative *_vec, *_retNative;
	float rl, vl, rlpt, s, c, angle;
	struct pt r, v, c1, c2;

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_multObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _multObj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _multObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	r.x = _rot->v.r[0];
	r.y = _rot->v.r[1];
	r.z = _rot->v.r[2];
	angle = _rot->v.r[3];

	if ((_vec = JS_GetPrivate(cx, _multObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for_multObjin SFRotationMultVec.\n");
		return JS_FALSE;
	}
	v.x = _vec->v.c[0];
	v.y = _vec->v.c[1];
	v.z = _vec->v.c[2];
	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	
	rl = veclength(r);
	vl = veclength(v);
	rlpt = VECPT(r, v) / rl / vl;
	s = sin(angle);
	c = cos(angle);
	VECCP(r, v, c1);
	VECSCALE(c1, 1.0 / rl);
	VECCP(r, c1, c2);
	VECSCALE(c2, 1.0 / rl) ;
	_retNative->v.c[0] = v.x + s * c1.x + (1-c) * c2.x;
	_retNative->v.c[1] = v.y + s * c1.y + (1-c) * c2.y;
	_retNative->v.c[2] = v.z + s * c1.z + (1-c) * c2.z;
	
	return JS_TRUE;
}

JSBool
SFRotationSetAxis(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_setAxisObj;
	SFRotationNative *_rot;
	SFVec3fNative *_vec;

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_setAxisObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFRotationSetAxis.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _setAxisObj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationSetAxis.\n");
		return JS_FALSE;
	}

	if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationSetAxis.\n");
		return JS_FALSE;
	}

	if ((_vec = JS_GetPrivate(cx, _setAxisObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFRotationSetAxis.\n");
		return JS_FALSE;
	}

	(_rot->v).r[0] = (_vec->v).c[0];
	(_rot->v).r[1] = (_vec->v).c[1];
	(_rot->v).r[2] = (_vec->v).c[2];

	*rval = OBJECT_TO_JSVAL(obj);

	if (JSVerbose) {
		printf("SFRotationSetAxis: obj = %u, result = [%.4g, %.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_rot->v).r[0],
			   (_rot->v).r[1],
			   (_rot->v).r[2],
			   (_rot->v).r[3]);
	}

	return JS_TRUE;
}

/* implement later */
JSBool
SFRotationSlerp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_slerpObj, *_retObj, *_proto;
	SFRotationNative *_rot1, *_rot2;
	jsdouble d;

	if (!JS_ConvertArguments(cx, argc, argv, "o d", &_slerpObj, &d)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFRotationSlerp.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _slerpObj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFRotationSlerp.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _slerpObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFRotationSlerp.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFRotationClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFRotationSlerp.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationSlerp.\n");
		return JS_FALSE;
	}

	if ((_rot2 = JS_GetPrivate(cx, _slerpObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _slerpObj in SFRotationSlerp.\n");
		return JS_FALSE;
	}

	fprintf(stderr, "SFRotation's slerp function does nothing!\n");

	return JS_TRUE;
}

JSBool
SFRotationToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    SFRotationNative *ptr;
    JSString *_str;
	char buff[STRING];

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFRotationToString.\n");
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
    JSObject *_from_obj;
    SFRotationNative *fptr, *ptr;
    char *_id_str;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFRotationAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFRotationAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFRotationAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _from_obj in SFRotationAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _from_obj in SFRotationAssign.\n");
        return JS_FALSE;
	}
	if (JSVerbose) {
		printf("SFRotationAssign: obj = %u, id = \"%s\", from = %u\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj);
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

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFRotationTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVerbose) {
		printf("SFRotationTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}

JSBool 
SFRotationConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SFVec3fNative *_vec;
	SFRotationNative *ptr;
	JSObject *_ob1, *_ob2;
	jsdouble pars[4];
	float v1len, v2len;
	double v12dp;
	struct pt v1, v2;

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
	} else if (argc == 2 && JS_ConvertArguments(cx, argc, argv, "o o",
												&_ob1, &_ob2)) {
		if (!JS_InstanceOf(cx, _ob1, &SFVec3fClass, argv)) {
			fprintf(stderr,
					"JS_InstanceOf failed for _ob1 in SFRotationConstr.\n");
			return JS_FALSE;
		}

		if (!JS_InstanceOf(cx, _ob2, &SFVec3fClass, argv)) {
			fprintf(stderr,
					"JS_InstanceOf failed for _ob2 in SFRotationConstr.\n");
			return JS_FALSE;
		}
		if ((_vec = JS_GetPrivate(cx, _ob1)) == NULL) {
			fprintf(stderr,
					"JS_GetPrivate failed for _ob1 in SFRotationConstr.\n");
			return JS_FALSE;
		}
		v1.x = _vec->v.c[0];
		v1.y = _vec->v.c[1];
		v1.z = _vec->v.c[2];
		_vec = 0;

		if ((_vec = JS_GetPrivate(cx, _ob2)) == NULL) {
			fprintf(stderr,
					"JS_GetPrivate failed for _ob2 in SFRotationConstr.\n");
			return JS_FALSE;
		}
		v2.x = _vec->v.c[0];
		v2.y = _vec->v.c[1];
		v2.z = _vec->v.c[2];
		_vec = 0;

		v1len = veclength(v1);
		v2len = veclength(v2);
		v12dp = vecdot(&v1, &v2);
		(ptr->v).r[0] = v1.y * v2.z - v2.y * v1.z;
		(ptr->v).r[1] = v1.z * v2.x - v2.z * v1.x;
		(ptr->v).r[2] = v1.x * v2.y - v2.x * v1.y;
		v12dp /= v1len * v2len;
		(ptr->v).r[3] = atan2(sqrt(1 - v12dp * v12dp), v12dp);
	} else if (argc == 2 && JS_ConvertArguments(cx, argc, argv, "o d",
					&_ob1, &(pars[0]))) {
		if (!JS_InstanceOf(cx, _ob1, &SFVec3fClass, argv)) {
			fprintf(stderr,
					"JS_InstanceOf failed for arg format \"o d\" in SFRotationConstr.\n");
			return JS_FALSE;
		}
		if ((_vec = JS_GetPrivate(cx, _ob1)) == NULL) {
			fprintf(stderr,
					"JS_GetPrivate failed for arg format \"o d\" in SFRotationConstr.\n");
			return JS_FALSE;
		}
		(ptr->v).r[0] = _vec->v.c[0]; 
		(ptr->v).r[1] = _vec->v.c[1]; 
		(ptr->v).r[2] = _vec->v.c[2]; 
		(ptr->v).r[3] = pars[0];
	} else if (argc == 4 && JS_ConvertArguments(cx, argc, argv, "d d d d",
												&(pars[0]), &(pars[1]),
												&(pars[2]), &(pars[3]))) {
		(ptr->v).r[0] = pars[0];
		(ptr->v).r[1] = pars[1];
		(ptr->v).r[2] = pars[2];
		(ptr->v).r[3] = pars[3];
	} else {
		fprintf(stderr, "Invalid arguments for SFRotationConstr.\n");
		return JS_FALSE;
	}

	if (JSVerbose) {
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

	if (JSVerbose) {
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
	if (JSVerbose) {
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
SFVec2fAdd(JSContext *cx, JSObject *obj,
		   uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_addObj, *_proto, *_retObj;
	SFVec2fNative *_vec1, *_vec2, *_retNative;

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_addObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _addObj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _addObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}

	if ((_retObj =
		 JS_ConstructObject(cx, &SFVec2fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fAdd.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fAdd.\n");
		return JS_FALSE;
	}

	if ((_vec2 = JS_GetPrivate(cx, _addObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _addObj in SFVec2fAdd.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec2fAdd.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = (_vec1->v).c[0] + (_vec2->v).c[0];
	(_retNative->v).c[1] = (_vec1->v).c[1] + (_vec2->v).c[1];

	if (JSVerbose) {
		printf("SFVec2fAdd: obj = %u, result = [%.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1]);
	}

	return JS_TRUE;
}

JSBool
SFVec2fDivide(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_proto, *_retObj;
	SFVec2fNative *_vec, *_retNative;
	jsdouble d;

	if (!JS_ConvertArguments(cx, argc, argv, "d", &d)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fDivide.\n");
		return JS_FALSE;
	}

	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fDivide.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFVec2fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fDivide.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fDivide.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec2fDivide.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = (_vec->v).c[0] / d;
	(_retNative->v).c[1] = (_vec->v).c[1] / d;

	if (JSVerbose) {
		printf("SFVec2fDivide: obj = %u, result = [%.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1]);
	}

	return JS_TRUE;
}

JSBool
SFVec2fDot(JSContext *cx, JSObject *obj,
		   uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_dotObj, *_proto, *_retObj;
	SFVec2fNative *_vec, *_retNative;
	struct pt v, ret;
			
	if (!JS_ConvertArguments(cx, argc, argv, "o", &_dotObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fDot.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _dotObj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _dotObj in SFVec2fDot.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _dotObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fDot.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec2fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fDot.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fDot.\n");
		return JS_FALSE;
	}
	v.x = (_vec->v).c[0];
	v.y = (_vec->v).c[1];
	v.z = 0;

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec2fDot.\n");
		return JS_FALSE;
	}

	vecdot(&ret, &v);
	(_retNative->v).c[0] = ret.x;
	(_retNative->v).c[1] = ret.y;

	if (JSVerbose) {
		printf("SFVec2fDot: obj = %u, result = [%.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1]);
	}

	return JS_TRUE;
}

JSBool
SFVec2fLength(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
	SFVec2fNative *_vec;
	jsdouble result, *dp;
	struct pt v;

	UNUSED(argc);
	UNUSED(argv);
	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fLength.\n");
		return JS_FALSE;
	}
	v.x = (_vec->v).c[0];
	v.y = (_vec->v).c[1];
	v.z = 0;

	result = (double) veclength(v);
	if ((dp = JS_NewDouble(cx, result)) == NULL) {
		fprintf(stderr, "JS_NewDouble failed for %f in SFVec2fLength.\n", result);
		return JS_FALSE;
	}
	*rval = DOUBLE_TO_JSVAL(dp); 

	if (JSVerbose) {
		printf("SFVec2fLength: obj = %u, result = %.4g\n",
			   (unsigned int) obj, *dp);
	}

	return JS_TRUE;
}

JSBool
SFVec2fMultiply(JSContext *cx, JSObject *obj,
				uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_proto, *_retObj;
	SFVec2fNative *_vec, *_retNative;
	jsdouble d;

	if (!JS_ConvertArguments(cx, argc, argv, "d", &d)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fMultiply.\n");
		return JS_FALSE;
	}

	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fMultiply.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFVec2fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fMultiply.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fMultiply.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec2fMultiply.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = (_vec->v).c[0] * d;
	(_retNative->v).c[1] = (_vec->v).c[1] * d;

	if (JSVerbose) {
		printf("SFVec2fMultiply: obj = %u, result = [%.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1]);
	}

	return JS_TRUE;
}

JSBool
SFVec2fNegate(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
	SFVec2fNative *_vec, *_retNative;
	JSObject *_retObj, *_proto;

	UNUSED(argc);
	UNUSED(argv);
	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fNegate.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFVec2fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fNegate.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fNegate.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec2fNegate.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = -(_vec->v).c[0];
	(_retNative->v).c[1] = -(_vec->v).c[1];

	if (JSVerbose) {
		printf("SFVec2fNegate: obj = %u, result = [%.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1]);
	}

	return JS_TRUE;
}

JSBool
SFVec2fNormalize(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_retObj, *_proto;
	SFVec2fNative *_vec, *_retNative;
	struct pt v, ret;

	UNUSED(argc);
	UNUSED(argv);
	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFVec2fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fNormalize.\n");
		return JS_FALSE;
	}
	v.x = (_vec->v).c[0];
	v.y = (_vec->v).c[1];
	v.z = 0;

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec2fNormalize.\n");
		return JS_FALSE;
	}

	vecnormal(&ret, &v);
	(_retNative->v).c[0] = ret.x;
	(_retNative->v).c[1] = ret.y;

	if (JSVerbose) {
		printf("SFVec2fNormalize: obj = %u, result = [%.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1]);
	}

	return JS_TRUE;
}

JSBool
SFVec2fSubtract(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_subtObj, *_proto, *_retObj;
	SFVec2fNative *_vec1, *_vec2, *_retNative;
			
	if (!JS_ConvertArguments(cx, argc, argv, "o", &_subtObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _subtObj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _subtObj in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _subtObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec2fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for obj failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}

	if ((_vec2 = JS_GetPrivate(cx, _subtObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for _subtObj failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for _retObj failed in SFVec2fSubtract.\n");
		return JS_FALSE;
	}
	
	(_retNative->v).c[0] = (_vec1->v).c[0] - (_vec2->v).c[0];
	(_retNative->v).c[1] = (_vec1->v).c[1] - (_vec2->v).c[1];

	if (JSVerbose) {
		printf("SFVec2fSubtract: obj = %u, result = [%.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1]);
	}

	return JS_TRUE;
}

JSBool
SFVec2fToString(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
    SFVec2fNative *ptr;
    JSString *_str;
	char buff[STRING];

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec2fToString.\n");
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
SFVec2fAssign(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    SFVec2fNative *fptr, *ptr;
    char *_id_str;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec2fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFVec2fAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec2fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _from_obj in SFVec2fAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _from_obj in SFVec2fAssign.\n");
        return JS_FALSE;
	}
	if (JSVerbose) {
		printf("SFVec2fAssign: obj = %u, id = \"%s\", from = %u\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj);
	}

    SFVec2fNativeAssign(ptr, fptr);
    *rval = OBJECT_TO_JSVAL(obj); 

    return JS_TRUE;
}

JSBool
SFVec2fTouched(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
{
    SFVec2fNative *ptr;
    int t;

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec2fTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVerbose) {
		printf("SFVec2fTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}

JSBool
SFVec2fConstr(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval)
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
	if (JSVerbose) {
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

	if (JSVerbose) {
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
	if (JSVerbose) {
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
SFVec3fAdd(JSContext *cx, JSObject *obj,
		   uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_addObj, *_proto, *_retObj;
	SFVec3fNative *_vec1, *_vec2, *_retNative;

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_addObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _addObj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _addObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fAdd.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fAdd.\n");
		return JS_FALSE;
	}

	if ((_vec2 = JS_GetPrivate(cx, _addObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _addObj in SFVec3fAdd.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec3fAdd.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = (_vec1->v).c[0] + (_vec2->v).c[0];
	(_retNative->v).c[1] = (_vec1->v).c[1] + (_vec2->v).c[1];
	(_retNative->v).c[2] = (_vec1->v).c[2] + (_vec2->v).c[2];

	if (JSVerbose) {
		printf("SFVec3fAdd: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

JSBool
SFVec3fCross(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_crossObj, *_proto, *_retObj;
	SFVec3fNative *_vec1, *_vec2, *_retNative;
	struct pt v1, v2, ret;

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_crossObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fCross.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _crossObj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in SFVec3fCross.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _crossObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fCross.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fCross.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fCross.\n");
		return JS_FALSE;
	}
	v1.x = (_vec1->v).c[0]; 
	v1.y = (_vec1->v).c[1]; 
	v1.z = (_vec1->v).c[2]; 

	if ((_vec2 = JS_GetPrivate(cx, _crossObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _crossObj in SFVec3fCross.\n");
		return JS_FALSE;
	}
	v2.x = (_vec2->v).c[0]; 
	v2.y = (_vec2->v).c[1]; 
	v2.z = (_vec2->v).c[2]; 

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec3fCross.\n");
		return JS_FALSE;
	}

	veccross(&ret, v1, v2);
	(_retNative->v).c[0] = ret.x;
	(_retNative->v).c[1] = ret.y;
	(_retNative->v).c[2] = ret.z;

	if (JSVerbose) {
		printf("SFVec3fCross: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

JSBool
SFVec3fDivide(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_proto, *_retObj;
	SFVec3fNative *_vec, *_retNative;
	jsdouble d;

	if (!JS_ConvertArguments(cx, argc, argv, "d", &d)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fDivide.\n");
		return JS_FALSE;
	}

	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fDivide.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fDivide.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fDivide.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec3fDivide.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = (_vec->v).c[0] / d;
	(_retNative->v).c[1] = (_vec->v).c[1] / d;
	(_retNative->v).c[2] = (_vec->v).c[2] / d;

	if (JSVerbose) {
		printf("SFVec3fDivide: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

JSBool
SFVec3fDot(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_dotObj, *_proto, *_retObj;
	SFVec3fNative *_vec, *_retNative;
	struct pt v, ret;

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_dotObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fDot.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _dotObj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _dotObj in SFVec3fDot.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _dotObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fDot.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fDot.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fDot.\n");
		return JS_FALSE;
	}
	v.x = (_vec->v).c[0];
	v.y = (_vec->v).c[1];
	v.z = (_vec->v).c[2];

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec3fDot.\n");
		return JS_FALSE;
	}

	vecdot(&ret, &v);
	(_retNative->v).c[0] = ret.x;
	(_retNative->v).c[1] = ret.y;
	(_retNative->v).c[2] = ret.z;

	if (JSVerbose) {
		printf("SFVec3fDot: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

JSBool
SFVec3fLength(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	SFVec3fNative *_vec;
	jsdouble result, *dp;
	struct pt v;

	UNUSED(argc);
	UNUSED(argv);
	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fLength.\n");
		return JS_FALSE;
	}
	v.x = (_vec->v).c[0];
	v.y = (_vec->v).c[1];
	v.z = (_vec->v).c[2];

	result = (double) veclength(v);
	if ((dp = JS_NewDouble(cx, result)) == NULL) {
		fprintf(stderr, "JS_NewDouble failed for %f in SFVec3fLength.\n", result);
		return JS_FALSE;
	}
	*rval = DOUBLE_TO_JSVAL(dp); 

	if (JSVerbose) {
		printf("SFVec3fLength: obj = %u, result = %.4g\n",
			   (unsigned int) obj, *dp);
	}

	return JS_TRUE;
}

JSBool
SFVec3fMultiply(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_proto, *_retObj;
	SFVec3fNative *_vec, *_retNative;
	jsdouble d;

	if (!JS_ConvertArguments(cx, argc, argv, "d", &d)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fMultiply.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fMultiply.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fMultiply.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fMultiply.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec3fMultiply.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = (_vec->v).c[0] * d;
	(_retNative->v).c[1] = (_vec->v).c[1] * d;
	(_retNative->v).c[2] = (_vec->v).c[2] * d;

	if (JSVerbose) {
		printf("SFVec3fMultiply: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

JSBool
SFVec3fNegate(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	SFVec3fNative *_vec, *_retNative;
	JSObject *_retObj, *_proto;

	UNUSED(argc);
	UNUSED(argv);
	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fNegate.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fNegate.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fNegate.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec3fNegate.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = -(_vec->v).c[0];
	(_retNative->v).c[1] = -(_vec->v).c[1];
	(_retNative->v).c[2] = -(_vec->v).c[2];

	if (JSVerbose) {
		printf("SFVec3fNegate: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

JSBool
SFVec3fNormalize(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	SFVec3fNative *_vec, *_retNative;
	JSObject *_retObj, *_proto;
	struct pt v, ret;

	UNUSED(argc);
	UNUSED(argv);
	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fNormalize.\n");
		return JS_FALSE;
	}
	v.x = (_vec->v).c[0];
	v.y = (_vec->v).c[1];
	v.z = (_vec->v).c[2];

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _retObj in SFVec3fNormalize.\n");
		return JS_FALSE;
	}

	vecnormal(&ret, &v);
	(_retNative->v).c[0] = ret.x;
	(_retNative->v).c[1] = ret.y;
	(_retNative->v).c[2] = ret.z;

	if (JSVerbose) {
		printf("SFVec3fNormalize: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

JSBool
SFVec3fSubtract(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_subtObj, *_proto, *_retObj;
	SFVec3fNative *_vec1, *_vec2, *_retNative;

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_subtObj)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _subtObj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _subtObj in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _subtObj)) == NULL) {
		fprintf(stderr, "JS_GetPrototype failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		fprintf(stderr, "JS_ConstructObject failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for obj failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}

	if ((_vec2 = JS_GetPrivate(cx, _subtObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for _subtObj failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate for _retObj failed in SFVec3fSubtract.\n");
		return JS_FALSE;
	}
	
	(_retNative->v).c[0] = (_vec1->v).c[0] - (_vec2->v).c[0];
	(_retNative->v).c[1] = (_vec1->v).c[1] - (_vec2->v).c[1];
	(_retNative->v).c[2] = (_vec1->v).c[2] - (_vec2->v).c[2];

	if (JSVerbose) {
		printf("SFVec3fSubtract: obj = %u, result = [%.4g, %.4g, %.4g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0],
			   (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}

	return JS_TRUE;
}

JSBool
SFVec3fToString(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
    SFVec3fNative *ptr;
    JSString *_str;
	char buff[STRING];

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec3fToString.\n");
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
SFVec3fAssign(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    SFVec3fNative *fptr, *ptr;
    char *_id_str;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for obj in SFVec3fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for obj in SFVec3fAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		fprintf(stderr, "JS_ConvertArguments failed in SFVec3fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed for _from_obj in SFVec3fAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed for _from_obj in SFVec3fAssign.\n");
        return JS_FALSE;
	}
	if (JSVerbose) {
		printf("SFVec3fAssign: obj = %u, id = \"%s\", from = %u\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj);
	}

    SFVec3fNativeAssign(ptr, fptr);
    *rval = OBJECT_TO_JSVAL(obj); 

    return JS_TRUE;
}

JSBool
SFVec3fTouched(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
{
    SFVec3fNative *ptr;
    int t;

	UNUSED(argc);
	UNUSED(argv);
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		fprintf(stderr, "JS_GetPrivate failed in SFVec3fTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVerbose) {
		printf("SFVec3fTouched: obj = %u, touched = %d\n", (unsigned int) obj, t);
	}
    *rval = INT_TO_JSVAL(t);
    return JS_TRUE;
}

JSBool
SFVec3fConstr(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval)
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
	if (JSVerbose) {
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

	if (JSVerbose) {
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
SFVec3fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	SFVec3fNative *ptr;
	jsdouble d, *dp;

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
	if (JSVerbose) {
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
MFColorToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFColor", rval);
}

JSBool
MFColorAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int32 len, i;
    char *_id_str;

	if (!JS_InstanceOf(cx, obj, &MFColorClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFColorAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
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

	if (JSVerbose) {
		printf("MFColorAssign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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

	if (JSVerbose) {
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
MFColorAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFColorAddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFColorGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *_obj;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFColorGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		if (_index >= _length) {
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
	}

	return JS_TRUE;
}

JSBool 
MFColorSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFColorSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}



JSBool
MFFloatToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFFloat", rval);
}

JSBool
MFFloatAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int32 len, i;
    char *_id_str;

	if (!JS_InstanceOf(cx, obj, &MFFloatClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFFloatAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		fprintf(stderr, "JS_ConvertArguments failed in MFFloatAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &MFFloatClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFFloatAssign.\n");
        return JS_FALSE;
    }

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

	if (JSVerbose) {
		printf("MFFloatAssign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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

	if (JSVerbose) {
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
MFFloatAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFFloatAddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFFloatGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFFloatGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		if (_index >= _length) {
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
	}

	return JS_TRUE;
}

JSBool 
MFFloatSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFFloatSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}



JSBool
MFInt32ToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFInt32", rval);
}

JSBool
MFInt32Assign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj;
    jsval val, myv;
    int32 len, i;
    char *_id_str;

	if (!JS_InstanceOf(cx, obj, &MFInt32Class, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFInt32Assign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
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

	if (JSVerbose) {
		printf("MFInt32Assign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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

	if (JSVerbose) {
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
MFInt32AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFInt32AddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFInt32GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFInt32GetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		if (_index >= _length) {
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
	}

	return JS_TRUE;
}

JSBool 
MFInt32SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFInt32SetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}



JSBool
MFNodeToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFNode", rval);
}

JSBool
MFNodeAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *_from_obj, *globalObj;
	BrowserNative *brow;
    jsval val, myv;
    int32 len, i;
    char *_id_str;

	if (!JS_InstanceOf(cx, obj, &MFNodeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFNodeAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
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

	if (JSVerbose) {
		printf("MFNodeAssign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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
	if ((globalObj = JS_GetGlobalObject(cx)) == NULL) {
		fprintf(stderr, "JS_GetGlobalObject failed in MFNodeSetProperty.\n");
		return JS_FALSE;
	}
	if (!getBrowser(cx, globalObj, &brow)) {
		fprintf(stderr, "getBrowser failed in MFNodeConstr.\n");
		return JS_FALSE;
	}

    *rval = OBJECT_TO_JSVAL(obj);

    return JS_TRUE;
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

	if (JSVerbose) {
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
MFNodeAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFNodeAddProperty: obj = %u\n", (unsigned int) obj);
	}

	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFNodeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	int32 _index;

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				fprintf(stderr,
						"JS_LookupElement failed in MFNodeGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				fprintf(stderr,
						"MFNodeGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				*vp = INT_TO_JSVAL(0);
			}
	}
	return JS_TRUE;
}

JSBool
MFNodeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	JSObject *_obj;
	jsval _val;
	char *_c;
	int32 _index;

	if (JSVerbose && JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (JSVAL_IS_OBJECT(*vp)) {
			if (!JS_ValueToObject(cx, *vp, &_obj)) {
				fprintf(stderr,
						"JS_ValueToObject failed in MFNodeSetProperty.\n");
				return JS_FALSE;
			}
			if (!JS_GetProperty(cx, _obj, "__handle", &_val)) {
				fprintf(stderr,
						"JS_GetProperty failed in MFNodeSetProperty.\n");
				return JS_FALSE;
			}
			_str = JSVAL_TO_STRING(_val);
			_c = JS_GetStringBytes(_str);
			printf("MFNodeSetProperty: obj = %u, id = %d, SFNode object = %u, handle = \"%s\"\n",
				   (unsigned int) obj, _index, (unsigned int) _obj, _c);
		}
	}
	return doMFSetProperty(cx, obj, id, vp);
}



JSBool
MFTimeAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFTimeAddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFTimeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFTimeGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		if (_index >= _length) {
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
	}

	return JS_TRUE;
}

JSBool 
MFTimeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFTimeSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}

JSBool
MFTimeToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFTime", rval);
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

	if (JSVerbose) {
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
    int32 len, i;
    char *_id_str;

	if (JSVerbose) {
		printf("MFTimeAssign: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}

	if (!JS_InstanceOf(cx, obj, &MFTimeClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFTimeAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
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

	if (JSVerbose) {
		printf("MFTimeAssign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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
MFVec2fAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFVec2fAddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFVec2fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *_obj;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFVec2fGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		if (_index >= _length) {
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
	}

	return JS_TRUE;
}

JSBool 
MFVec2fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFVec2fSetProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFSetProperty(cx, obj, id, vp);
}

JSBool
MFVec2fToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFVec2f", rval);
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

	if (JSVerbose) {
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
    int32 len, i;
    char *_id_str;

	if (!JS_InstanceOf(cx, obj, &MFVec2fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFVec2fAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
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

	if (JSVerbose) {
		printf("MFVec2fAssign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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
	if (JSVerbose) {
		printf("MFVec3fAddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFVec3fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *_obj;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFVec3fGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		if (_index >= _length) {
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
	}

	return JS_TRUE;
}

JSBool 
MFVec3fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFVec3fSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}

JSBool
MFVec3fToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFVec3f", rval);
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

	if (JSVerbose) {
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
							  /* getAssignProperty, setAssignProperty, */
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
    int32 len, i;
    char *_id_str;

	if (!JS_InstanceOf(cx, obj, &MFVec3fClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFVec3fAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
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

	if (JSVerbose) {
		printf("MFVec3fAssign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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
	if (JSVerbose) {
		printf("MFRotationAddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFRotationGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *_obj;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFRotationGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		if (_index >= _length) {
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
	}

	return JS_TRUE;
}

JSBool 
MFRotationSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (JSVerbose) {
		printf("MFRotationSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}

JSBool
MFRotationToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFRotation", rval);
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

	if (JSVerbose) {
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
    int32 len, i;
    char *_id_str;

	if (!JS_InstanceOf(cx, obj, &MFRotationClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFRotationAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
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

	if (JSVerbose) {
		printf("MFRotationAssign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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
MFStringAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	/* unquote parts of vp string if necessary */
	if (JSVAL_IS_STRING(*vp)) {
		if (!doMFStringUnquote(cx, vp)) {
			fprintf(stderr,
				"doMFStringUnquote failed in MFStringAddProperty.\n");
			return JS_FALSE;
		}
	}
	if (JSVerbose) {
		printf("MFStringAddProperty: obj = %u\n", (unsigned int) obj);
	}
	return doMFAddProperty(cx, obj, id, vp);
}

JSBool 
MFStringGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		fprintf(stderr,
				"JS_GetProperty failed for \"length\" in MFStringGetProperty.\n");
        return JS_FALSE;
	}
	_length = JSVAL_TO_INT(_length_val);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		if (_index >= _length) {
			_str = JS_NewStringCopyZ(cx, "");
			*vp = STRING_TO_JSVAL(_str);
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
	}

	return JS_TRUE;
}

JSBool 
MFStringSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	/* unquote parts of vp string if necessary */
	if (JSVAL_IS_STRING(*vp)) {
		if (!doMFStringUnquote(cx, vp)) {
			fprintf(stderr,
				"doMFStringUnquote failed in MFStringSetProperty.\n");
			return JS_FALSE;
		}
	}
	if (JSVerbose) {
		printf("MFStringSetProperty:\n");
	}
	return doMFSetProperty(cx, obj, id, vp);
}

JSBool
MFStringToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFString", rval);
}

JSBool
MFStringConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;
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

	if (JSVerbose) {
		printf("MFStringConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if ((_str = JS_ValueToString(cx, argv[i])) == NULL) {
			fprintf(stderr,
					"JS_ValueToString failed in MFStringConstr.\n");
			return JS_FALSE;
		}
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
    int32 len, i;
    char *_id_str;

	if (!JS_InstanceOf(cx, obj, &MFStringClass, argv)) {
		fprintf(stderr, "JS_InstanceOf failed in MFStringAssign.\n");
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
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

	if (JSVerbose) {
		printf("MFStringAssign: obj = %u, id = \"%s\", from = %u, len = %d\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

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
