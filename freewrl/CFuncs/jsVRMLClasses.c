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

/********************************************************/
/*							*/
/* first part - standard helper functions		*/
/*							*/
/********************************************************/

static int JSVRMLClassesVerbose = 0;
void _get4f(double *ret, double *mat, int row);
void _set4f(double len, double *mat, int row);


/* try and print what an element is in case of error */
static void
printNodeType (JSContext *context, JSObject *myobj) {
	printf ("type was: ");
	if (JS_InstanceOf(context, myobj, &SFColorClass, NULL)) {
	printf ("SFColorClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &SFVec2fClass, NULL)) {
	printf ("SFVec2fClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &SFVec3fClass, NULL)) {
	printf ("SFVec3fClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &SFRotationClass, NULL)) {
	printf ("SFRotationClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &SFImageClass, NULL)) {
	printf ("SFImageClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &SFNodeClass, NULL)) {
	printf ("SFNodeClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFFloatClass, NULL)) {
	printf ("MFFloatClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFTimeClass, NULL)) {
	printf ("MFTimeClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFInt32Class, NULL)) {
	printf ("MFInt32Class\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFColorClass, NULL)) {
	printf ("MFColorClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFVec2fClass, NULL)) {
	printf ("MFVec2fClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFVec3fClass, NULL)) {
	printf ("MFVec3fClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFRotationClass, NULL)) {
	printf ("MFRotationClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFNodeClass, NULL)) {
	printf ("MFNodeClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &MFStringClass, NULL)) {
	printf ("MFStringClass\n");
	}
	else if (JS_InstanceOf(context, myobj, &VrmlMatrixClass, NULL)) {
	printf ("VrmlMatrixClass\n");
	}
/*	
	else if (JSVAL_IS_STRING(myobj)==TRUE) {
		printf ("Common String");
	}
	else if (JSVAL_IS_OBJECT(myobj)==TRUE) {
		printf ("It is an object");
	} 
	else if (JSVAL_IS_PRIMITIVE(myobj)==TRUE) {
		printf ("is a primitive");
	}
*/
	else printf ("Unknown");
	printf ("\n");
}

/* do a simple copy; from, to, and count */
static JSBool _simplecopyElements (JSContext *cx, 
		JSObject *fromObj,
		JSObject *toObj,
		int count,
		char *name) {
	int i;
	jsval val;

	for (i = 0; i < count; i++) {
		if (!JS_GetElement(cx, fromObj, (jsint) i, &val)) {
			printf( "failed in get %s index %d.\n",name, i);
			return JS_FALSE;
		}
		if (!JS_SetElement(cx, toObj, (jsint) i, &val)) {
			printf( "failed in set %s index %d.\n", name, i);
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}

/* make a standard assignment for MF variables */
JSBool _standardMFAssign(JSContext *cx, 
	JSObject *obj, 
	uintN argc, 
	jsval *argv, 
	jsval *rval,
	JSClass *myClass,
	char *name) {

	JSObject *_from_obj;
	jsval val, myv;
	int32 len;
	char *_id_str;

	if (!JS_InstanceOf(cx, obj, myClass, argv)) {
		printf("JS_InstanceOf failed in %s.\n",name);
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		printf("JS_ConvertArguments failed in %s.\n",name);
		return JS_FALSE;
	}
	
	if (!JS_InstanceOf(cx, _from_obj, myClass, argv)) {
		printf("JS_InstanceOf failed in %s.\n",name);
		return JS_FALSE;
	}

	myv = INT_TO_JSVAL(1);
	if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		printf("JS_SetProperty failed for \"__touched_flag\" in %s.\n",name);
		return JS_FALSE;
	}

	if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		printf("JS_GetProperty failed for \"length\" in %s.\n",name);
		return JS_FALSE;
	}
	
	if (!JS_SetProperty(cx, obj, "length", &val)) {
		printf("JS_SetProperty failed for \"length\" in %s\n",name);
		return JS_FALSE;
	}

	len = JSVAL_TO_INT(val);

	if (JSVRMLClassesVerbose) {
		printf("%s: obj = %u, id = \"%s\", from = %u, len = %d\n",name,
		(unsigned int) obj, _id_str, (unsigned int) _from_obj, len);
	}

	/* copyElements */
	*rval = OBJECT_TO_JSVAL(obj); 
	return _simplecopyElements(cx, _from_obj, obj, len,name);
}

/* standardized GetProperty for MF's */
JSBool 
_standardMFGetProperty(JSContext *cx, 
		JSObject *obj, 
		jsval id, 
		jsval *vp,
		char *makeNewElement,
		char *name) {

	JSObject *_obj;
	int32 _length, _index;
	jsval _length_val;

	// in case we need to run makeNewElement
	int newElemenLen;
	jsval newEle;
	
	if (JSVRMLClassesVerbose) printf ("_standardMFGetProperty starting\n");

	if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		printf( "JS_GetProperty failed for \"length\" in %s.\n",name);
		return JS_FALSE;
	}

	_length = JSVAL_TO_INT(_length_val);
	if (JSVRMLClassesVerbose) printf ("standarg get property, len %d\n",_length);

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		if (JSVRMLClassesVerbose) printf ("standard get property, index requested %d\n",_index);
	
		if (_index >= _length) {
			if (JSVRMLClassesVerbose) printf ("\n\nconstructing new object\n");
			// we were making this with C calls, but it would fail with a 
			// segfault; so, now, we run a script to do it.


			newElemenLen = strlen(makeNewElement);

			if (!JS_EvaluateScript(cx, obj, makeNewElement, newElemenLen,
				FNAME_STUB, LINENO_STUB, &newEle)) {
				printf("JS_EvaluateScript failed for \"%s\".\n", makeNewElement);
				return JS_FALSE;
			}
	
			*vp = OBJECT_TO_JSVAL(newEle);

			if (JSVRMLClassesVerbose) printf ("defining element %d now... is %d %x\n",_index,*vp,*vp);
			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
				JS_PropertyStub, JS_PropertyStub,
				JSPROP_ENUMERATE)) {
				printf( "JS_DefineElement failed in %s.\n",name);
				return JS_FALSE;
			}

			if (!doMFSetProperty(cx,obj,id,vp,name)) {
				printf ("wow, cant assign property\n");
			}
		}
		if (JSVRMLClassesVerbose) printf ("object already has this index\n");
		if (!JS_LookupElement(cx, obj, _index, vp)) {
			printf( "JS_LookupElement failed in %s.\n",name);
			return JS_FALSE;
		}
		if (*vp == JSVAL_VOID) {
			printf( "warning: %s: obj = %u, jsval = %d does not exist!\n",name,
				(unsigned int) obj, (int) _index);
			//return JS_FALSE;
		}
	}
	if (JSVRMLClassesVerbose) printf ("_standardMFGetProperty finishing; element is %d %x\n",*vp,*vp);

	return JS_TRUE;
}



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
		printf( "JS_GetProperty failed for \"length\" in doMFToString for %s.\n",
				className);
        return JS_FALSE;
	}
	len = JSVAL_TO_INT(_v);

	if (JSVRMLClassesVerbose) printf ("doMFToString, len %d\n",len);

	if (len == 0) {
		_str = JS_NewStringCopyZ(cx, _empty_array);
		*rval = STRING_TO_JSVAL(_str);
		if (JSVRMLClassesVerbose) printf ("doMFToString, len is zero, returning JS_TRUE, and %d\n",*rval);
		return JS_TRUE;
	}

	className_len = strlen(className);
	if (!strncmp(className, "MFString", className_len)) {
		isString = JS_TRUE;
	}

	buff_size = LARGESTRING;
	if ((_buff = (char *)
		 malloc(buff_size * sizeof(char))) == NULL) {
			printf( "malloc failed in doMFToString for %s.\n",
					className);
			return JS_FALSE;
	}
	memset(_buff, 0, buff_size);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, obj, i, &_v)) {
			printf("JS_GetElement failed for %d of %d in doMFToString for %s.\n",
				i, len,className);
			return JS_FALSE;
		}
		_tmpStr = JS_ValueToString(cx, _v);
		if (_tmpStr==NULL) {
			_tmp_valStr = "NULL";
		} else {
			_tmp_valStr = JS_GetStringBytes(_tmpStr);
		}
		if (JSVRMLClassesVerbose) printf ("doMFToString, element %d is %d, string %s\n",i,_v,_tmp_valStr);
		tmp_valStr_len = strlen(_tmp_valStr) + 1;
		tmp_buff_len = strlen(_buff);

		if ((buff_size - (tmp_buff_len + 1)) < (tmp_valStr_len + 1)) {
			buff_size += LARGESTRING;
			if ((_buff =
				 (char *)
				 JS_realloc(cx, _buff, buff_size * sizeof(char *)))
				== NULL) {
				printf(
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
			printf(
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
doMFAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp, char *name) {
	JSString *str, *sstr;
	jsval v;
	jsval myv;
	char *p, *pp;
	size_t p_len = 0;
	int len = 0, ind = JSVAL_TO_INT(id);

	if (JSVRMLClassesVerbose) 
		printf("\tdoMFAddProperty:%s ",name);
	

	str = JS_ValueToString(cx, id);
	p = JS_GetStringBytes(str);

	p_len = strlen(p);
	if (!strncmp(p, "length", p_len) ||
		!strncmp(p, "toString", p_len) ||
		!strncmp(p, "__touched_flag", p_len) || 
		!strncmp(p, "setTransform", p_len) ||
		!strncmp(p, "assign", p_len) ||
		!strncmp(p, "inverse", p_len) ||
		!strncmp(p, "transpose", p_len) ||
		!strncmp(p, "multLeft", p_len) ||
		!strncmp(p, "multRight", p_len) ||
		!strncmp(p, "multVecMatrix", p_len) ||
		!strncmp(p, "multMatrixVec", p_len) ||
		!strncmp(p, "constructor", p_len) ||
		!strncmp(p, "getTransform", p_len)) {
		if (JSVRMLClassesVerbose) {
			printf("property \"%s\" is one of the standard properties. Do nothing.\n", p);
		}
		return JS_TRUE;
	}

	//if (JSVRMLClassesVerbose) {
	//	printf ("(past quickreturn), context %d %x vp %d %x\n",cx,cx,*vp,*vp);
	//	
	//	sstr = JS_ValueToString(cx, *vp);
	//	printf ("past JS_ValueToString\n");
	//	if (JSVAL_IS_STRING(*vp)) {
	//		pp = JS_GetStringBytes(sstr);
	//
	//		printf("adding property %s, %s to object %u, \n",
	//			   p, pp, (unsigned int) obj);
	//	} else {
	//		printf ("OBJECT IS NOT A STRING!!! ERROR!!!\n");
	//	}
	//}
	if (!JSVAL_IS_INT(id)){ 
		printf( "JSVAL_IS_INT failed for id in doMFAddProperty.\n");
		return JS_FALSE;
	}
	if (!JS_GetProperty(cx, obj, "length", &v)) {
		printf( "JS_GetProperty failed for \"length\" in doMFAddProperty.\n");
		return JS_FALSE;
	}
	
	len = JSVAL_TO_INT(v);
	if (ind >= len) {
		len = ind + 1;
		v = INT_TO_JSVAL(len);
		if (!JS_SetProperty(cx, obj, "length", &v)) {
			printf(
					"JS_SetProperty failed for \"length\" in doMFAddProperty.\n");
			return JS_FALSE;
		}
	}
	if (JSVRMLClassesVerbose) {
		printf("index = %d, length = %d\n", ind, len);
	}

	myv = INT_TO_JSVAL(1);
	if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
		printf(
				"JS_SetProperty failed for \"__touched_flag\" in doMFAddProperty.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}


static JSBool
doMFSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp,char *name)
{
	JSString *_str, *_sstr;
	char *_c, *_cc;
	jsval myv;
	jsint _index;

	if (JSVRMLClassesVerbose) {
		printf ("doMFSetProperty, for vp %d %x\n",
				*vp,*vp);
		_str = JS_ValueToString(cx, id);
		_c = JS_GetStringBytes(_str);
		printf ("id is %s\n",_c);

		_sstr = JS_ValueToString(cx, *vp);
		printf ("looking up value for %d %x object %u\n",*vp,*vp,(unsigned int) obj);
		if (JSVRMLClassesVerbose) {
			_cc = JS_GetStringBytes(_sstr);
			printf("\tdoMFSetProperty:%s: obj = %u, id = %s, vp = %s\n",name,
			   (unsigned int) obj, _c, _cc);
		}
	}

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);

		myv = INT_TO_JSVAL(1);
		if (!JS_SetProperty(cx, obj, "__touched_flag", &myv)) {
			printf( "JS_SetProperty failed for \"__touched_flag\" in doMFSetProperty.\n");
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
		printf( "JS_GetProperty failed for \"Browser\" in getBrowser.\n");
		return JS_FALSE;
	}
	if (!JSVAL_IS_OBJECT(_b_val)) {
		printf( "\"Browser\" property is not an object in getBrowser.\n");
		return JS_FALSE;
	}
	
	if ((*brow = JS_GetPrivate(context, JSVAL_TO_OBJECT(_b_val))) == NULL) {
		printf( "JS_GetPrivate failed in getBrowser.\n");
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

	if (JSVRMLClassesVerbose) {
		printf("doMFStringUnquote: vp = \"%s\"\n", _buff);
	}

	if (memchr(_buff, '"', _buff_len) != NULL) {
		if ((_tmp_vpStr = (char *)
			 malloc(_buff_len * sizeof(char))) == NULL) {
			printf( "malloc failed in doMFStringUnquote.\n");
			return JS_FALSE;
		}

		memset(_tmp_vpStr, 0, _buff_len);

		for (i = 0; i <= (_buff_len-1); i++) {
			if (_buff[i] != '"' ||
				(i > 0 && _buff[i - 1] == '\\')) {
				_tmp_vpStr[j++] = _buff[i];
			}
		}
		if (JSVRMLClassesVerbose) printf ("new unquoted string %s\n",_tmp_vpStr);

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
		printf("JS_InitClass for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFColor);
	if (!JS_SetProperty(context, globalObj, "__SFColor_proto", &v)) {
		printf("JS_SetProperty for SFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;
	
	if ((proto_SFVec2f = JS_InitClass(context, globalObj, NULL, &SFVec2fClass,
			  SFVec2fConstr, INIT_ARGC, NULL,
			  SFVec2fFunctions, NULL, NULL)) == NULL) {
		printf("JS_InitClass for SFVec2fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFVec2f);
	if (!JS_SetProperty(context, globalObj, "__SFVec2f_proto", &v)) {
		printf("JS_SetProperty for SFVec2fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;
	
	if ((proto_SFVec3f = JS_InitClass(context, globalObj, NULL, &SFVec3fClass,
			  SFVec3fConstr, INIT_ARGC, NULL,
			  SFVec3fFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for SFVec3fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFVec3f);
	if (!JS_SetProperty(context, globalObj, "__SFVec3f_proto", &v)) {
		printf("JS_SetProperty for SFVec3fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_SFRotation = JS_InitClass(context, globalObj, NULL, &SFRotationClass,
			 SFRotationConstr, INIT_ARGC,
			 NULL, SFRotationFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for SFRotationClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFRotation);
	if (!JS_SetProperty(context, globalObj, "__SFRotation_proto", &v)) {
		printf( "JS_SetProperty for SFRotationClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_SFImage = JS_InitClass(context, globalObj, NULL, &SFImageClass,
			  SFImageConstr, INIT_ARGC, NULL,
			  SFImageFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for SFImageClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFImage);
	if (!JS_SetProperty(context, globalObj, "__SFImage_proto", &v)) {
		printf( "JS_SetProperty for SFImageClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_SFNode = JS_InitClass(context, globalObj, NULL, &SFNodeClass,
			 SFNodeConstr, INIT_ARGC_NODE, NULL,
			 SFNodeFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for SFNodeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_SFNode);
	if (!JS_SetProperty(context, globalObj, "__SFNode_proto", &v)) {
		printf("JS_SetProperty for SFNodeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFFloat = JS_InitClass(context, globalObj, NULL, &MFFloatClass,
			  MFFloatConstr, INIT_ARGC, NULL,
			  MFFloatFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFFloatClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFFloat);
	if (!JS_SetProperty(context, globalObj, "__MFFloat_proto", &v)) {
		printf( "JS_SetProperty for MFFloatClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFTime = JS_InitClass(context, globalObj, NULL, &MFTimeClass,
			 MFTimeConstr, INIT_ARGC, NULL,
			 MFTimeFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFTimeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFTime);
	if (!JS_SetProperty(context, globalObj, "__MFTime_proto", &v)) {
		printf( "JS_SetProperty for MFTimeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFInt32 = JS_InitClass(context, globalObj, NULL, &MFInt32Class,
			  MFInt32Constr, INIT_ARGC, NULL,
			  MFInt32Functions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFInt32Class failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFInt32);
	if (!JS_SetProperty(context, globalObj, "__MFInt32_proto", &v)) {
		printf( "JS_SetProperty for MFInt32Class failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFColor = JS_InitClass(context, globalObj, NULL, &MFColorClass,
			  MFColorConstr, INIT_ARGC, NULL,
			  MFColorFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFColor);
	if (!JS_SetProperty(context, globalObj, "__MFColor_proto", &v)) {
		printf( "JS_SetProperty for MFColorClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFVec2f = JS_InitClass(context, globalObj, NULL, &MFVec2fClass,
			  MFVec2fConstr, INIT_ARGC, NULL,
			  MFVec2fFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFVec2fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFVec2f);
	if (!JS_SetProperty(context, globalObj, "__MFVec2f_proto", &v)) {
		printf( "JS_SetProperty for MFVec2fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFVec3f = JS_InitClass(context, globalObj, NULL, &MFVec3fClass,
			  MFVec3fConstr, INIT_ARGC, NULL,
			  MFVec3fFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFVec3fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFVec3f);
	if (!JS_SetProperty(context, globalObj, "__MFVec3f_proto", &v)) {
		printf( "JS_SetProperty for MFVec3fClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFRotation = JS_InitClass(context, globalObj, NULL, &MFRotationClass,
			 MFRotationConstr, INIT_ARGC, NULL,
			 MFRotationFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFRotationClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFRotation);
	if (!JS_SetProperty(context, globalObj, "__MFRotation_proto", &v)) {
		printf( "JS_SetProperty for MFRotationClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;
	
	if ((proto_MFNode = JS_InitClass(context, globalObj, NULL, &MFNodeClass,
			 MFNodeConstr, INIT_ARGC, NULL,
			 MFNodeFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFNodeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFNode);
	if (!JS_SetProperty(context, globalObj, "__MFNode_proto", &v)) {
		printf( "JS_SetProperty for MFNodeClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;

	if ((proto_MFString = JS_InitClass(context, globalObj, NULL, &MFStringClass,
			   MFStringConstr, INIT_ARGC, NULL,
			   MFStringFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for MFStringClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_MFString);
	if (!JS_SetProperty(context, globalObj, "__MFString_proto", &v)) {
		printf( "JS_SetProperty for MFStringClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = 0;
	if ((proto_VrmlMatrix = JS_InitClass(context, globalObj, NULL, &VrmlMatrixClass,
			   VrmlMatrixConstr, INIT_ARGC, NULL,
			   VrmlMatrixFunctions, NULL, NULL)) == NULL) {
		printf( "JS_InitClass for VrmlMatrixClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}
	v = OBJECT_TO_JSVAL(proto_VrmlMatrix);
	if (!JS_SetProperty(context, globalObj, "__VrmlMatrix_proto", &v)) {
		printf( "JS_SetProperty for VrmlMatrixClass failed in loadVrmlClasses.\n");
		return JS_FALSE;
	}

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
			printf( "malloc failed in setECMANative.\n");
			return JS_FALSE;
		}
		//JAS - do NOT prepend/append double quotes to this string
		//JAS - only for the null terminator
		//JAS len += 3;
		len += 1;

		memset(_new_vp_c, 0, len);
		//JAS sprintf(_new_vp_c, "\"%.*s\"", len, _vp_c);
		sprintf(_new_vp_c, "%.*s", len, _vp_c);
		_newVpStr = JS_NewStringCopyZ(context, _new_vp_c);
		*vp = STRING_TO_JSVAL(_newVpStr);

		if (JSVRMLClassesVerbose) {
			printf("setECMANative: obj = %u, id = \"%s\", vp = %s\n",
				   (unsigned int) obj, _id_c, _new_vp_c);
		}
		free(_new_vp_c);
	} else {
		if (JSVRMLClassesVerbose) {
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
		printf( "malloc failed in setECMANative.\n");
		return JS_FALSE;
	}
	memset(_buff, 0, len);
	sprintf(_buff, "_%.*s_touched", len, _id_c);
	v = INT_TO_JSVAL(1);
	if (!JS_SetProperty(context, obj, _buff, &v)) {
		printf(
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

	if (JSVRMLClassesVerbose) {
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
			printf( "JS_ConvertValue failed in setAssignProperty.\n");
			return JS_FALSE;
		}
		if (!JS_GetProperty(cx, obj, _id_c, &_initVal)) {
			printf(
					"JS_GetProperty failed in setAssignProperty.\n");
			return JS_FALSE;
		}
		if (JSVRMLClassesVerbose) {
			printf("setAssignProperty: obj = %u, id = \"%s\", from = %ld, to = %ld\n",
				   (unsigned int) obj, _id_c, _newVal, _initVal);
		}
		_o = JSVAL_TO_OBJECT(_initVal);
		_argv[0] = _newVal;
		_argv[1] = id;
		if (!JS_CallFunctionName(cx, _o, "assign", _argc, _argv, vp)) {
			printf(
					"JS_CallFunctionName failed in setAssignProperty.\n");
			return JS_FALSE;
		}
	} else {
		if (JSVRMLClassesVerbose) {
			_str = JS_ValueToString(cx, id);
			_id_c = JS_GetStringBytes(_str);
			printf("setAssignProperty: obj = %u, id = \"%s\"\n",
				   (unsigned int) obj, _id_c);
		}
	}

	return JS_TRUE;
}



/********************************************************/
/*							*/
/* Second part - SF classes				*/
/*							*/
/********************************************************/


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
		printf( "JS_NewArrayObject failed in SFColorGetHSV.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_arrayObj);

	/* construct new double before conversion? */
	_v = DOUBLE_TO_JSVAL(&hue);
	if (!JS_SetElement(cx, _arrayObj, 0, &_v)) {
		printf( "JS_SetElement failed for hue in SFColorGetHSV.\n");
		return JS_FALSE;
	}
	_v = DOUBLE_TO_JSVAL(&saturation);
	if (!JS_SetElement(cx, _arrayObj, 1, &_v)) {
		printf( "JS_SetElement failed for saturation in SFColorGetHSV.\n");
		return JS_FALSE;
	}

	_v = DOUBLE_TO_JSVAL(&value);
	if (!JS_SetElement(cx, _arrayObj, 2, &_v)) {
		printf( "JS_SetElement failed for value in SFColorGetHSV.\n");
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
		printf( "JS_GetPrivate failed in SFColorToString.\n");
		return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "d d d",
							 &hue, &saturation, &value)) {
		printf( "JS_ConvertArguments failed in SFColorSetHSV.\n");
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
		printf( "JS_GetPrivate failed in SFColorToString.\n");
		return JS_FALSE;
	}

	memset(_buff, 0, STRING);
	sprintf(_buff, "%.9g %.9g %.9g",
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
		printf( "JS_GetPrivate failed for obj in SFColorAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFColorClass, argv)) {
		printf( "JS_InstanceOf failed for obj in SFColorAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		printf( "JS_ConvertArguments failed in SFColorAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFColorClass, argv)) {
		printf( "JS_InstanceOf failed for _from_obj in SFColorAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		printf( "JS_GetPrivate failed for _from_obj in SFColorAssign.\n");
        return JS_FALSE;
	}
	if (JSVRMLClassesVerbose) {
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
		printf( "JS_GetPrivate failed in SFSFColorTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVRMLClassesVerbose) {
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
		printf( "SFColorNativeNew failed in SFColorConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFColorProperties)) {
		printf( "JS_DefineProperties failed in SFColorConstr.\n");
		return JS_FALSE;
	}

	if (!JS_SetPrivate(cx, obj, ptr)) {
		printf( "JS_SetPrivate failed in SFColorConstr.\n");
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
		printf( "Invalid arguments for SFColorConstr.\n");
		return JS_FALSE;
	}
	if (JSVRMLClassesVerbose) {
		printf("SFColorConstr: obj = %u, %u args, %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	}
	*rval = OBJECT_TO_JSVAL(obj);

	return JS_TRUE;
}

void
SFColorFinalize(JSContext *cx, JSObject *obj)
{
	SFColorNative *ptr;

	if (JSVRMLClassesVerbose) {
		printf("SFColorFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFColorFinalize.\n");
		return;
	}
	SFColorNativeDelete(ptr);
}

JSBool
SFColorGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	SFColorNative *ptr;
	jsdouble d, *dp;

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFColorGetProperty.\n");
		return JS_FALSE;
	}
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			d = (ptr->v).c[0];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
						"JS_NewDouble failed for %f in SFColorGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break;
		case 1:
			d = (ptr->v).c[1];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
						"JS_NewDouble failed for %f in SFColorGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break;
		case 2:
			d = (ptr->v).c[2];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
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
		printf( "JS_GetPrivate failed in SFColorSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (JSVRMLClassesVerbose) {
		printf("SFColorSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}

	if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &_val)) {
		printf( "JS_ConvertValue failed in SFColorSetProperty.\n");
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
		printf( "JS_GetPrivate failed in SFImageToString.\n");
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
		printf( "JS_GetPrivate failed for obj in SFImageAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFImageClass, argv)) {
		printf( "JS_InstanceOf failed for obj in SFImageAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		printf( "JS_ConvertArguments failed in SFImageAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFImageClass, argv)) {
		printf( "JS_InstanceOf failed for _from_obj in SFImageAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		printf( "JS_GetPrivate failed for _from_obj in SFImageAssign.\n");
        return JS_FALSE;
	}
	if (JSVRMLClassesVerbose) {
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
		printf( "JS_GetPrivate failed in SFSFImageTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVRMLClassesVerbose) {
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
		printf( "SFImageNativeNew failed in SFImageConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFImageProperties)) {
		printf( "JS_DefineProperties failed in SFImageConstr.\n");
		return JS_FALSE;
	}

	if (!JS_SetPrivate(cx, obj, ptr)) {
		printf( "JS_SetPrivate failed in SFImageConstr.\n");
		return JS_FALSE;
	}
     	
	if (JSVRMLClassesVerbose) {
		printf("SFImageConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	*rval = OBJECT_TO_JSVAL(obj);
		
	return JS_TRUE;
}

void
SFImageFinalize(JSContext *cx, JSObject *obj)
{
	SFImageNative *ptr;

	if (JSVRMLClassesVerbose) {
		printf("SFImageFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFImageFinalize.\n");
		return;
	}
	SFImageNativeDelete(ptr);
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
		printf( "JS_GetPrivate failed in SFImageSetProperty.\n");
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
	if (JSVRMLClassesVerbose) printf ("SFNODETOSTRING\n");
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFNodeToString.\n");
		return JS_FALSE;
	}

	handle_len = strlen(ptr->handle) + 1;
	if ((_buff = (char *)
		 malloc(handle_len * sizeof(char))) == NULL) {
		printf( "malloc failed in SFNodeToString.\n");
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
	/* unsigned int toptr; */
printf ("start of SFNodeAssign\n");

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFNodeAssign.\n");
	    return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, obj, &SFNodeClass, argv)) {
		printf( "JS_InstanceOf failed for obj in SFNodeAssign.\n");
	    return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		printf( "JS_ConvertArguments failed in SFNodeAssign.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _from_obj, &SFNodeClass, argv)) {
		printf( "JS_InstanceOf failed for _from_obj in SFNodeAssign.\n");
	    return JS_FALSE;
	}
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		printf( "JS_GetPrivate failed for _from_obj in SFNodeAssign.\n");
	    return JS_FALSE;
	}
	if (JSVRMLClassesVerbose) {
		printf("SFNodeAssign: obj = %u, id = \"%s\", from = %u\n",
			   (unsigned int) obj, _id_str, (unsigned int) _from_obj);
	}

	/* printf ("SFNodeAssign calling SFNodeNativeAssign\n"); */
	if (strncmp("NODE",fptr->handle,4) == 0) {
		/* printf ("SFNodeAssign, handle is a NODE\n"); */
		/* try to get the node backend now */

		if ((globalObj = JS_GetGlobalObject(cx)) == NULL) {
			printf( "JS_GetGlobalObject failed in SFNodeAssign.\n");
			return JS_FALSE;
		}
		if (!getBrowser(cx, globalObj, &brow)) {
			printf( "getBrowser failed in SFNodeAssign.\n");
			return JS_FALSE;
		}

		doPerlCallMethodVA(brow->sv_js, "getNodeCNode", "s", fptr->handle);
	       if (!JS_GetProperty(cx, globalObj, "__ret",  &_rval)) {
        	        printf( "JS_GetProperty failed in VrmlBrowserGetVersion.\n");
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
		printf( "SFNodeNativeAssign failed in SFNodeAssign.\n");
	    return JS_FALSE;
	}

	*rval = OBJECT_TO_JSVAL(obj);

	printf ("end of SFNodeAssign\n");
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

	if (JSVRMLClassesVerbose) printf ("start of SFNodeTouched\n");

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFNodeTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVRMLClassesVerbose) {
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
	char *tmpptr, *xptr;
	size_t vrmlstring_len = 0, handle_len = 0;
	jsval _obj_val = OBJECT_TO_JSVAL(obj);
	jsval _rval = 0;
	JSString *_idStr; char *_id_c;

	/*
	 * ECMAScript scripting reference requires that a legal VRML
	 * string be a constructor argument.
	 */
	if (JSVRMLClassesVerbose) printf ("start of SFNodeConstr, obj %d argc %d\n",obj,argc);
	if (argc == 1 && JS_ConvertArguments(cx, argc, argv, "s",
							&_vrmlstr)) {

		vrmlstring_len = strlen(_vrmlstr) + 1;
		if (JSVRMLClassesVerbose) printf ("SFNodeConstr, argc==1, vrmlstr = %s\n",_vrmlstr);

		if ((ptr = SFNodeNativeNew(vrmlstring_len, handle_len)) == NULL) {
			printf( "SFNodeNativeNew failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_DefineProperties(cx, obj, SFNodeProperties)) {
			printf( "JS_DefineProperties failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_SetPrivate(cx, obj, ptr)) {
			printf( "JS_SetPrivate failed in SFNodeConstr.\n");
			return JS_FALSE;
		}

		/* copy this over, making sure we dont get hit by threading or
		 * memory problems */
		tmpptr = malloc ((vrmlstring_len+1)*sizeof(char));
		memmove(tmpptr, _vrmlstr, vrmlstring_len);
		xptr = ptr->vrmlstring;
		ptr->vrmlstring = tmpptr;
		free (xptr);
		

		if ((globalObj = JS_GetGlobalObject(cx)) == NULL) {
			printf( "JS_GetGlobalObject failed in SFNodeConstr.\n");
			return JS_FALSE;
		}

		if (!getBrowser(cx, globalObj, &brow)) {
			printf( "getBrowser failed in SFNodeConstr.\n");
			return JS_FALSE;
		}

		if (!JS_SetProperty(cx, globalObj, BROWSER_SFNODE, &_obj_val)) {
			printf(
					"JS_SetProperty failed for \"%s\" in SFNodeConstr.\n",
					BROWSER_SFNODE);
			return JS_FALSE;
		}

		doPerlCallMethodVA(brow->sv_js, "jspSFNodeConstr", "s", _vrmlstr);

		/* get the string and handle here */
	       if (!JS_GetProperty(cx, globalObj, "__ret",  &_rval)) {
        	        printf( "JS_GetProperty failed in VrmlBrowserGetVersion.\n");
        	        return JS_FALSE;
        	}
		_idStr = JS_ValueToString(cx, _rval);
		_id_c = JS_GetStringBytes(_idStr);
		if (JSVRMLClassesVerbose) printf ("RETURNED %s\n",_id_c);
		handle_len = strlen(_id_c) + 1;
		if (JSVRMLClassesVerbose) printf ("pointer handle is %x\n",ptr->handle);

		/* copy this over, making sure we dont get hit by threading or
		 * memory problems */
		tmpptr = malloc ((handle_len+1)*sizeof(char));
		memmove(tmpptr, _id_c, handle_len);
		xptr = ptr->handle;
		ptr->handle = tmpptr;
		free (xptr);
		




	} else if (argc == 2 && JS_ConvertArguments(cx, argc, argv, "s s",
			   &_vrmlstr, &_handle)) {
		vrmlstring_len = strlen(_vrmlstr) + 1;
		handle_len = strlen(_handle) + 1;

		if (JSVRMLClassesVerbose) printf ("SFNodeConstr, argc==2, vrmlstr = %s\n",_vrmlstr);
		if ((ptr = SFNodeNativeNew(vrmlstring_len, handle_len)) == NULL) {
			printf( "SFNodeNativeNew failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_DefineProperties(cx, obj, SFNodeProperties)) {
			printf( "JS_DefineProperties failed in SFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_SetPrivate(cx, obj, ptr)) {
			printf( "JS_SetPrivate failed in SFNodeConstr.\n");
			return JS_FALSE;
		}

		/* copy this over, making sure we dont get hit by threading or
		 * memory problems */
		tmpptr = malloc ((vrmlstring_len+1)*sizeof(char));
		memmove(tmpptr, _vrmlstr, vrmlstring_len);
		xptr = ptr->vrmlstring;
		ptr->vrmlstring = tmpptr;
		free (xptr);

		tmpptr = malloc ((handle_len+1)*sizeof(char));
		memmove(tmpptr, _handle, handle_len);
		xptr = ptr->handle;
		ptr->handle = tmpptr;
		free (xptr);
	} else {
		printf(
				"SFNodeConstr requires at least 1 string arg.\n");
		return JS_FALSE;
	}

	*rval = OBJECT_TO_JSVAL(obj);
	if (JSVRMLClassesVerbose) {
		printf("SFNodeConstr: obj = %u, argc = %u, vrmlstring=\"%s\", handle=\"%s\"\n",
			   (unsigned int) obj, argc, ptr->vrmlstring, ptr->handle);
	}
	return JS_TRUE;
}

void
SFNodeFinalize(JSContext *cx, JSObject *obj)
{
	SFNodeNative *ptr;

	if (JSVRMLClassesVerbose) {
		printf("SFNodeFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFNodeFinalize.\n");
		return;
	}
	SFNodeNativeDelete(ptr);
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
		printf( "JS_GetPrivate failed in SFNodeGetProperty.\n");
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
			printf( "JS_GetGlobalObject failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}

		if (!getBrowser(cx, globalObj, &brow)) {
			printf( "getBrowser failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}

		if ((_buff = (char *) malloc((id_len + STRING) * sizeof(char))) == NULL) {
			printf( "malloc failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}
		val_len = strlen(ptr->handle) + 1;
		sprintf(_buff, "NODE%.*s_%.*s", val_len, ptr->handle, id_len, _id_c);

		if (JSVRMLClassesVerbose) printf ("\n\n getting property for buff %s\n\n",_buff);
		if (!JS_SetProperty(cx, globalObj, _buff, vp)) {
			printf(
					"JS_SetProperty failed for \"%s\" in SFNodeGetProperty.\n",
					_buff);
			return JS_FALSE;
		}

		if (JSVRMLClassesVerbose) printf ("SFNodeGetProperty, getting the property for %s\n",ptr->handle);
		doPerlCallMethodVA(brow->sv_js, "jspSFNodeGetProperty", "ss", _id_c, ptr->handle);

		if (JSVRMLClassesVerbose) printf ("getting property for vuff %s\n",_buff);
		if (!JS_GetProperty(cx, globalObj, _buff, &_rval)) {
			printf ("failed; try for prepending a NODE to the front\n");
			printf(
					"JS_GetProperty failed in SFNodeGetProperty.\n");
			return JS_FALSE;
		}
		*vp = _rval;
		if (JSVRMLClassesVerbose) printf ("jsp, returnval %d, storing in %d\n",_rval,vp);
		if (JSVRMLClassesVerbose) printf ("jsp, is rv a string?\n");
		//if (JSVAL_IS_STRING(_rval)) printf ("yes!\n");
		free(_buff);
	}

	if (JSVRMLClassesVerbose &&
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

	if (JSVRMLClassesVerbose) {
		printf("SFNodeSetProperty: obj = %u, id = %s, vp = %s\n",
			   (unsigned int) obj, _id_c, _val_c);
	}

	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFNodeSetProperty.\n");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		ptr->touched++;
		val_len = strlen(_val_c) + 1;

		if (JSVRMLClassesVerbose) printf ("switching on %d\n",JSVAL_TO_INT(id));

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
		if (JSVRMLClassesVerbose) printf ("JS_IS_INT false\n");
		if ((globalObj = JS_GetGlobalObject(cx)) == NULL) {
			printf( "JS_GetGlobalObject failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}

		if (!getBrowser(cx, globalObj, &brow)) {
			printf( "getBrowser failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}

		if ((_buff = (char *) malloc((id_len + STRING) * sizeof(char))) == NULL) {
			printf( "malloc failed in SFNodeSetProperty.\n");
			return JS_FALSE;
		}
		val_len = strlen(ptr->handle) + 1;
		sprintf(_buff, "__node_%s", _id_c);

		// save this property in Javascript, because, we'll be getting it
		// via perl shortly.
		if (!JS_SetProperty(cx, globalObj, _buff, vp)) {
			printf(
					"JS_SetProperty failed for \"%s\" in SFNodeSetProperty.\n",
					_buff);
			return JS_FALSE;
		}

		// call the perl method to manipulate this node - we must do this
		// because we need the offsets that only perl (or, via a ROUTE) has.
		doPerlCallMethodVA(brow->sv_js, "jspSFNodeSetProperty", "ss", _id_c, ptr->handle);
		free(_buff);
	}

	return JS_TRUE;
}


/********************************************************************/

JSBool
SFRotationGetAxis(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_retObj;
	SFRotationNative *_rot;
	SFVec3fNative *_retNative;

	UNUSED(argc);
	UNUSED(argv);
	if (JSVRMLClassesVerbose) printf ("start of SFRotationGetAxis\n");

	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, NULL, NULL)) == NULL) {
		printf( "JS_ConstructObject failed in SFRotationGetAxis.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFRotationGetAxis.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		printf( "JS_GetPrivate failed for _retObj in SFRotationGetAxis.\n");
		return JS_FALSE;
	}

	(_retNative->v).c[0] = (_rot->v).r[0];
	(_retNative->v).c[1] = (_rot->v).r[1];
	(_retNative->v).c[2] = (_rot->v).r[2];

	if (JSVRMLClassesVerbose) {
		printf("SFRotationGetAxis: obj = %u, result = [%.9g, %.9g, %.9g]\n",
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
	if (JSVRMLClassesVerbose) printf ("start of SFRotationInverse\n");
	if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
		printf( "JS_GetPrototype failed in SFRotationInverse.\n");
		return JS_FALSE;
	}
	if ((_retObj = JS_ConstructObject(cx, &SFRotationClass, _proto, NULL)) == NULL) {
		printf( "JS_ConstructObject failed in SFRotationInverse.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFRotationInverse.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		printf( "JS_GetPrivate failed for _retObj in SFRotationInverse.\n");
		return JS_FALSE;
	}

	/* calculation correct? */
/* 	_retNative->v.r[0] = (_rot->v).r[0]; */
/* 	_retNative->v.r[1] = (_rot->v).r[1]; */
/* 	_retNative->v.r[2] = (_rot->v).r[2]; */
/* 	_retNative->v.r[3] = -(_rot->v).r[3]; */
	printf( "SFRotation's inverse function does nothing!\n");

	return JS_TRUE;
}

/* implement later */
JSBool
SFRotationMultiply(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_multObj, *_proto, *_retObj;
	SFRotationNative *_rot1, *_rot2, *_retNative;
	if (JSVRMLClassesVerbose) printf ("start of SFRotationMultiply\n");

	if (!JS_ConvertArguments(cx, argc, argv, "o", &_multObj)) {
		printf( "JS_ConvertArguments failed in SFRotationMultiply.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _multObj, &SFRotationClass, argv)) {
		printf( "JS_InstanceOf failed in SFRotationMultiply.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _multObj)) == NULL) {
		printf( "JS_GetPrototype failed in SFRotationMultiply.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFRotationClass, _proto, NULL)) == NULL) {
		printf( "JS_ConstructObject failed in SFRotationMultiply.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot1 = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFRotationMultiply.\n");
		return JS_FALSE;
	}

	if ((_rot2 = JS_GetPrivate(cx, _multObj)) == NULL) {
		printf( "JS_GetPrivate failed for _multObj in SFRotationMultiply.\n");
		return JS_FALSE;
	}

	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		printf( "JS_GetPrivate failed for _retObj in SFRotationMultiply.\n");
		return JS_FALSE;
	}

	printf( "SFRotation's multiply function does nothing!\n");

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

	if (JSVRMLClassesVerbose) printf ("start of SFRotationMultiVec\n");
	if (!JS_ConvertArguments(cx, argc, argv, "o", &_multObj)) {
		printf( "JS_ConvertArguments failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _multObj, &SFVec3fClass, argv)) {
		printf( "JS_InstanceOf failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	if ((_proto = JS_GetPrototype(cx, _multObj)) == NULL) {
		printf( "JS_GetPrototype failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}

	if ((_retObj = JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
		printf( "JS_ConstructObject failed in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	*rval = OBJECT_TO_JSVAL(_retObj);

	if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFRotationMultVec.\n");
		return JS_FALSE;
	}
	r.x = _rot->v.r[0];
	r.y = _rot->v.r[1];
	r.z = _rot->v.r[2];
	angle = _rot->v.r[3];

	if ((_vec = JS_GetPrivate(cx, _multObj)) == NULL) {
		printf( "JS_GetPrivate failed for_multObjin SFRotationMultVec.\n");
		return JS_FALSE;
	}
	v.x = _vec->v.c[0];
	v.y = _vec->v.c[1];
	v.z = _vec->v.c[2];
	if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
		printf( "JS_GetPrivate failed for _retObj in SFRotationMultVec.\n");
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

	if (JSVRMLClassesVerbose) printf ("start of SFRotationSetAxis\n");
	if (!JS_ConvertArguments(cx, argc, argv, "o", &_setAxisObj)) {
		printf( "JS_ConvertArguments failed in SFRotationSetAxis.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _setAxisObj, &SFVec3fClass, argv)) {
		printf( "JS_InstanceOf failed in SFRotationSetAxis.\n");
		return JS_FALSE;
	}

	if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFRotationSetAxis.\n");
		return JS_FALSE;
	}

	if ((_vec = JS_GetPrivate(cx, _setAxisObj)) == NULL) {
		printf( "JS_GetPrivate failed for _retObj in SFRotationSetAxis.\n");
		return JS_FALSE;
	}

	(_rot->v).r[0] = (_vec->v).c[0];
	(_rot->v).r[1] = (_vec->v).c[1];
	(_rot->v).r[2] = (_vec->v).c[2];

	*rval = OBJECT_TO_JSVAL(obj);

	if (JSVRMLClassesVerbose) {
		printf("SFRotationSetAxis: obj = %u, result = [%.9g, %.9g, %.9g, %.9g]\n",
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
	JSObject *_destObj, *_retObj, *_proto;
	SFRotationNative *_rot, *_dest, *_ret;
	Quaternion _quat, _quat_dest, _quat_ret;
	jsdouble t;

	if (JSVRMLClassesVerbose) printf ("start of SFRotationSlerp\n");
	if (!JS_ConvertArguments(cx, argc, argv, "o d", &_destObj, &t)) {
		printf( "JS_ConvertArguments failed in SFRotationSlerp.\n");
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _destObj, &SFRotationClass, argv)) {
		printf( "JS_InstanceOf failed in SFRotationSlerp.\n");
		return JS_FALSE;
	}

	/*
	 * From Annex C, C.6.7.4:
	 *
	 * For t = 0, return object's rotation.
	 * For t = 1, return 1st argument.
	 * For 0 < t < 1, compute slerp.
	 */
	if (APPROX(t, 0)) {
		*rval = OBJECT_TO_JSVAL(obj);
	} else if (APPROX(t, 1)) {
		*rval = OBJECT_TO_JSVAL(_destObj);
	} else {
		if ((_proto = JS_GetPrototype(cx, _destObj)) == NULL) {
			printf( "JS_GetPrototype failed in SFRotationSlerp.\n");
			return JS_FALSE;
		}

		if ((_retObj = JS_ConstructObject(cx, &SFRotationClass, _proto, NULL)) == NULL) {
			printf( "JS_ConstructObject failed in SFRotationSlerp.\n");
			return JS_FALSE;
		}
		/* root the object */
		*rval = OBJECT_TO_JSVAL(_retObj);

		if ((_rot = JS_GetPrivate(cx, obj)) == NULL) {
			printf( "JS_GetPrivate failed for obj in SFRotationSlerp.\n");
			return JS_FALSE;
		}

		if ((_dest = JS_GetPrivate(cx, _destObj)) == NULL) {
			printf( "JS_GetPrivate failed for _destObj in SFRotationSlerp.\n");
			return JS_FALSE;
		}

		if ((_ret = JS_GetPrivate(cx, _retObj)) == NULL) {
			printf( "JS_GetPrivate failed for _retObj in SFRotationSlerp.\n");
			return JS_FALSE;
		}

		vrmlrot_to_quaternion(&_quat,
							  (_rot->v).r[0],
							  (_rot->v).r[1],
							  (_rot->v).r[2],
							  (_rot->v).r[3]);

		vrmlrot_to_quaternion(&_quat_dest,
							  (_dest->v).r[0],
							  (_dest->v).r[1],
							  (_dest->v).r[2],
							  (_dest->v).r[3]);

		slerp(&_quat_ret, &_quat, &_quat_dest, t);
		quaternion_to_vrmlrot(&_quat_ret,
							  (double *) &(_ret->v).r[0],
							  (double *) &(_ret->v).r[1],
							  (double *) &(_ret->v).r[2],
							  (double *) &(_ret->v).r[3]);
	}

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
	if (JSVRMLClassesVerbose) printf ("start of SFRotationToString\n");
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFRotationToString.\n");
		return JS_FALSE;
	}

	memset(buff, 0, STRING);
	sprintf(buff, "%.9g %.9g %.9g %.9g",
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

	if (JSVRMLClassesVerbose) printf ("start of SFRotationAssign\n");
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFRotationAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFRotationClass, argv)) {
		printf( "JS_InstanceOf failed for obj in SFRotationAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		printf( "JS_ConvertArguments failed in SFRotationAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFRotationClass, argv)) {
		printf( "JS_InstanceOf failed for _from_obj in SFRotationAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		printf( "JS_GetPrivate failed for _from_obj in SFRotationAssign.\n");
        return JS_FALSE;
	}
	if (JSVRMLClassesVerbose) {
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
	if (JSVRMLClassesVerbose) printf ("start of SFRotationTouched\n");
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFRotationTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVRMLClassesVerbose) {
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

	if (JSVRMLClassesVerbose) printf ("start of SFRotationConstr\n");
	if ((ptr = SFRotationNativeNew()) == NULL) {
		printf( "SFRotationNativeNew failed in SFRotationConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFRotationProperties)) {
		printf( "JS_DefineProperties failed in SFRotationConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		printf( "JS_SetPrivate failed in SFRotationConstr.\n");
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
			printf(
					"JS_InstanceOf failed for _ob1 in SFRotationConstr.\n");
			return JS_FALSE;
		}

		if (!JS_InstanceOf(cx, _ob2, &SFVec3fClass, argv)) {
			printf(
					"JS_InstanceOf failed for _ob2 in SFRotationConstr.\n");
			return JS_FALSE;
		}
		if ((_vec = JS_GetPrivate(cx, _ob1)) == NULL) {
			printf(
					"JS_GetPrivate failed for _ob1 in SFRotationConstr.\n");
			return JS_FALSE;
		}
		v1.x = _vec->v.c[0];
		v1.y = _vec->v.c[1];
		v1.z = _vec->v.c[2];
		_vec = 0;

		if ((_vec = JS_GetPrivate(cx, _ob2)) == NULL) {
			printf(
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
			printf(
					"JS_InstanceOf failed for arg format \"o d\" in SFRotationConstr.\n");
			return JS_FALSE;
		}
		if ((_vec = JS_GetPrivate(cx, _ob1)) == NULL) {
			printf(
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
		printf( "Invalid arguments for SFRotationConstr.\n");
		return JS_FALSE;
	}

	if (JSVRMLClassesVerbose) {
		printf("SFRotationConstr: obj = %u, %u args, %f %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).r[0], (ptr->v).r[1], (ptr->v).r[2], (ptr->v).r[3]);
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

void
SFRotationFinalize(JSContext *cx, JSObject *obj)
{
	SFRotationNative *ptr;

	if (JSVRMLClassesVerbose) {
		printf("SFRotationFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFRotationFinalize.\n");
		return;
	}
	SFRotationNativeDelete(ptr);
}

JSBool 
SFRotationGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	SFRotationNative *ptr;
	jsdouble d, *dp;

	if (JSVRMLClassesVerbose) printf ("start of SFRotationGetProperty\n");
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFRotationGetProperty.\n");
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			d = (ptr->v).r[0];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
						"JS_NewDouble failed for %f in SFRotationGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 1:
			d = (ptr->v).r[1];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
						"JS_NewDouble failed for %f in SFRotationGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 2:
			d = (ptr->v).r[2];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
						"JS_NewDouble failed for %f in SFRotationGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 3:
			d = (ptr->v).r[3];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
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

	if (JSVRMLClassesVerbose) printf ("start of SFRotationSetProperty\n");
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFRotationSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (JSVRMLClassesVerbose) {
		printf("SFRotationSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}
	if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &myv)) {
		printf( "JS_ConvertValue failed in SFRotationSetProperty.\n");
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

/********************************************************************/

/* Generic SFVec2f routines that return a SFVec2f */
#define __2FADD		1
#define __2FDIVIDE 	2
#define __2FMULT	3
#define __2FSUBT	4
#define __2FDOT		5
#define __2FLENGTH	6
#define __2FNORMALIZE	8
JSBool SFVec2fGeneric( JSContext *cx, JSObject *obj,
		   uintN argc, jsval *argv, jsval *rval, int op) {

	JSObject *_paramObj, *_proto, *_retObj;
	SFVec2fNative *_vec1, *_vec2, *_retNative;
	jsdouble d=0.0; 
	jsdouble d0=0.0; 
	jsdouble d1=0.0;
	jsdouble *dp;
	struct pt v1, v2;


	/* parameters */
	int SFParam = FALSE;
	int numParam = FALSE;

	/* return values */
	int retSFVec2f = FALSE;
	int retNumeric = FALSE;

	/* is the "argv" parameter a string? */
	int param_isString;
	char *charString;
	jsdouble pars[3];
	JSString *_str;

	/* determine what kind of parameter to get */
	if ((op==__2FADD)||(op==__2FDOT)||(op==__2FSUBT))SFParam=TRUE;
	if ((op==__2FDIVIDE)||(op==__2FMULT))numParam=TRUE;

	/* determine the return value, if it is NOT a SFVec2f */
	if ((op==__2FDOT)||(op==__2FLENGTH)) retNumeric = TRUE;
	retSFVec2f = (!retNumeric);

	/* is the parameter a string, possibly gotten from the VRML/X3d
	 * side of things? */
	param_isString = JSVAL_IS_STRING (*argv);

	/* get the parameter */
	if ((SFParam) || (numParam)) {
		if (numParam) {
			if (!JSVAL_IS_NUMBER(argv[0])) {
				printf ("SFVec2f param error - number expected\n");
				return JS_FALSE;
			} 
			if (!JS_ValueToNumber(cx, argv[0], &d)) {
				printf("JS_ValueToNumber failed in SFVec2f.\n");
				return JS_FALSE;
			}
		} else {
			/* did this come in from VRML as a string, or did 
			 * it get created in javascript? */
			if (param_isString) {
				_str = JS_ValueToString(cx, *argv);
				charString = JS_GetStringBytes(_str);

				if (sscanf(charString, "%lf %lf",
							&(pars[0]), &(pars[1])) != 2) {
					printf ("conversion problem in SFVec2fGeneric\n");
					return JS_FALSE;
				}
				//printf ("past scan, %f %f %f\n",pars[0], pars[1]);
			} else {
				if (!JS_ConvertArguments(cx, argc, argv, "o", &_paramObj)) {
					printf( "JS_ConvertArguments failed in SFVec2f.\n");
					return JS_FALSE;
				}
				if (!JS_InstanceOf(cx, _paramObj, &SFVec2fClass, argv)) {
					printf( "SFVec2f - expected a SFVec2f parameter.\n");
					printNodeType (cx,_paramObj);
					return JS_FALSE;
				}
			
				if ((_vec2 = JS_GetPrivate(cx, _paramObj)) == NULL) {
					printf( "JS_GetPrivate failed for _paramObj in SFVec2f.\n");
					return JS_FALSE;
				}
				pars[0]= (_vec2->v).c[0];
				pars[1] = (_vec2->v).c[1];
			}
		}
	}

	/* get our values */
	if ((_vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFVec2fAdd.\n");
		return JS_FALSE;
	}

	/* do the operation */
	switch (op) {
		/* returning a SFVec2f */
		case __2FADD:
			d0 = (_vec1->v).c[0] + (_vec2->v).c[0];
			d1 = (_vec1->v).c[1] + (_vec2->v).c[1];
			break;
		case __2FDIVIDE:
			d0 = (_vec1->v).c[0] / d;
			d1 = (_vec1->v).c[1] / d;
			break;
		case __2FMULT:
			d0 = (_vec1->v).c[0] * d;
			d1 = (_vec1->v).c[1] * d;
			break;
		case __2FSUBT:
			d0 = (_vec1->v).c[0] - (_vec2->v).c[0];
			d1 = (_vec1->v).c[1] - (_vec2->v).c[1];
			break;
		case __2FDOT:
			v1.x = (_vec1->v).c[0]; v1.y=(_vec1->v).c[1];v1.z=0.0;
			v2.x = (_vec2->v).c[0]; v2.y=(_vec2->v).c[1];v2.z=0.0;
			d = vecdot (&v1, &v2);
			break;
		case __2FLENGTH:
			v1.x = (_vec1->v).c[0]; v1.y=(_vec1->v).c[1];v1.z=0.0;
			d = veclength(v1);
			break;
		case __2FNORMALIZE:
			v1.x = (_vec1->v).c[0]; v1.y=(_vec1->v).c[1];v1.z=0.0;
			vecnormal(&v1, &v1);
			d0 = v1.x; d1 = v1.y;
			break;
		default:
		return JS_FALSE;
	}

	/* set the return object */
	if (retSFVec2f) {
		if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
			printf( "JS_GetPrototype failed in SFVec2f.\n");
			return JS_FALSE;
		}
		if ((_retObj =
			JS_ConstructObject(cx, &SFVec2fClass, _proto, NULL)) == NULL) {
			printf( "JS_ConstructObject failed in SFVec2f.\n");
			return JS_FALSE;
		}
		*rval = OBJECT_TO_JSVAL(_retObj);
		if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
			printf( "JS_GetPrivate failed for _retObj in SFVec2f.\n");
			return JS_FALSE;
		}
		(_retNative->v).c[0] = d0;
		(_retNative->v).c[1] = d1;
	} else if (retNumeric) {
		if ((dp = JS_NewDouble(cx,d)) == NULL) {
			printf( "JS_NewDouble failed for %f in SFVec2f.\n",d);
			return JS_FALSE;
		}
		*rval = DOUBLE_TO_JSVAL(dp); 
	}

	if ((JSVRMLClassesVerbose) && (retSFVec2f)){
		printf("SFVec2fgeneric: obj = %u, result = [%.9g, %.9g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1]);
	}
	if ((JSVRMLClassesVerbose) && (retNumeric)){
		printf("SFVec2fgeneric: obj = %u, result = %.9g\n",
			   (unsigned int) obj, d);
	}

	return JS_TRUE;
}

JSBool
SFVec2fAdd(JSContext *cx, JSObject *obj,
		   uintN argc, jsval *argv, jsval *rval) {
	return SFVec2fGeneric(cx, obj, argc, argv, rval, __2FADD);
}

JSBool
SFVec2fDivide(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval) {
	return SFVec2fGeneric(cx, obj, argc, argv, rval, __2FDIVIDE);
}

JSBool
SFVec2fMultiply(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval) {
	return SFVec2fGeneric(cx, obj, argc, argv, rval, __2FMULT);
}

JSBool
SFVec2fSubtract(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval) {
	return SFVec2fGeneric(cx, obj, argc, argv, rval, __2FSUBT);
}

JSBool
SFVec2fDot(JSContext *cx, JSObject *obj,
		   uintN argc, jsval *argv, jsval *rval) {
	return SFVec2fGeneric(cx, obj, argc, argv, rval, __2FDOT);
}

JSBool
SFVec2fLength(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval) {
	return SFVec2fGeneric(cx, obj, argc, argv, rval, __2FLENGTH);
}

JSBool
SFVec2fNormalize(JSContext *cx, JSObject *obj,
			  uintN argc, jsval *argv, jsval *rval) {
	return SFVec2fGeneric(cx, obj, argc, argv, rval, __2FNORMALIZE);
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
		printf( "JS_GetPrivate failed in SFVec2fToString.\n");
		return JS_FALSE;
	}
    
	memset(buff, 0, STRING);
	sprintf(buff, "%.9g %.9g",
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
		printf( "JS_GetPrivate failed for obj in SFVec2fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFVec2fClass, argv)) {
		printf( "JS_InstanceOf failed for obj in SFVec2fAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		printf( "JS_ConvertArguments failed in SFVec2fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFVec2fClass, argv)) {
		printf( "JS_InstanceOf failed for _from_obj in SFVec2fAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		printf( "JS_GetPrivate failed for _from_obj in SFVec2fAssign.\n");
        return JS_FALSE;
	}
	if (JSVRMLClassesVerbose) {
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
		printf( "JS_GetPrivate failed in SFVec2fTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVRMLClassesVerbose) {
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
		printf( "SFVec2fNativeNew failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFVec2fProperties)) {
		printf( "JS_DefineProperties failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		printf( "JS_SetPrivate failed in SFVec2fConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).c[0] = 0.0;
		(ptr->v).c[1] = 0.0;
	} else {
		if (!JS_ConvertArguments(cx, argc, argv, "d d", &(pars[0]), &(pars[1]))) {
			printf( "JS_ConvertArguments failed in SFVec2fConstr.\n");
			return JS_FALSE;
		}
		(ptr->v).c[0] = pars[0];
		(ptr->v).c[1] = pars[1];
	}
	if (JSVRMLClassesVerbose) {
		printf("SFVec2fConstr: obj = %u, %u args, %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1]);
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}


void
SFVec2fFinalize(JSContext *cx, JSObject *obj)
{
	SFVec2fNative *ptr;

	if (JSVRMLClassesVerbose) {
		printf("SFColorFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFVec2fFinalize.\n");
		return;
	}
	SFVec2fNativeDelete(ptr);
}

JSBool 
SFVec2fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	SFVec2fNative *ptr;
	jsdouble d, *dp;

	if ((ptr = JS_GetPrivate(cx,obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFVec2fGetProperty.\n");
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			d = (ptr->v).c[0];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
						"JS_NewDouble failed for %f in SFVec2fGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 1:
			d = (ptr->v).c[1];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
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
		printf( "JS_GetPrivate failed in SFVec2fSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (JSVRMLClassesVerbose) {
		printf("SFVec2fSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}

	if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &myv)) {
		printf( "JS_ConvertValue failed in SFVec2fSetProperty.\n");
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


/********************************************************************/

/* Generic SFVec3f routines that return a SFVec3f */
#define __3FADD		1
#define __3FDIVIDE 	2
#define __3FMULT	3
#define __3FSUBT	4
#define __3FDOT		5
#define __3FLENGTH	6
#define __3FNORMALIZE	8
#define __3FNEGATE	7
#define __3FCROSS	9

JSBool SFVec3fGeneric( JSContext *cx, JSObject *obj,
		   uintN argc, jsval *argv, jsval *rval, int op) {
	JSObject *_paramObj, *_proto, *_retObj;
	SFVec3fNative *_vec1, *_vec2, *_retNative;
	jsdouble d=0.0;
	jsdouble d0=0.0;
	jsdouble d1=0.0;
	jsdouble d2=0.0;
	jsdouble *dp;
	struct pt v1, v2, ret;


	/* parameters */
	int SFParam = FALSE;
	int numParam = FALSE;

	/* return values */
	int retSFVec3f = FALSE;
	int retNumeric = FALSE;

	/* is the "argv" parameter a string? */
	int param_isString;
	char *charString;
	jsdouble pars[3];
	JSString *_str;

	/* determine what kind of parameter to get */
	if ((op==__3FADD)||(op==__3FDOT)||(op==__3FCROSS)||(op==__3FSUBT))SFParam=TRUE;
	if ((op==__3FDIVIDE)||(op==__3FMULT))numParam=TRUE;

	/* determine the return value, if it is NOT a SFVec3f */
	if ((op==__3FDOT)||(op==__3FLENGTH)) retNumeric = TRUE;
	retSFVec3f = (!retNumeric);

	/* is the parameter a string, possibly gotten from the VRML/X3d
	 * side of things? */
	param_isString = JSVAL_IS_STRING (*argv);

	/* get the parameter */
	if ((SFParam) || (numParam)) {
		if (numParam) {
			if (!JSVAL_IS_NUMBER(argv[0])) {
				printf ("SFVec3f param error - number expected\n");
				return JS_FALSE;
			} 
			if (!JS_ValueToNumber(cx, argv[0], &d)) {
				printf("JS_ValueToNumber failed in SFVec3f.\n");
				return JS_FALSE;
			}
		} else {
			/* did this come in from VRML as a string, or did 
			 * it get created in javascript? */
			if (param_isString) {
				_str = JS_ValueToString(cx, *argv);
				charString = JS_GetStringBytes(_str);

				if (sscanf(charString, "%lf %lf %lf",
							&(pars[0]), &(pars[1]), &(pars[2])) != 3) {
					printf ("conversion problem in SFVec3fGeneric\n");
					return JS_FALSE;
				}
				//printf ("past scan, %f %f %f\n",pars[0], pars[1],pars[2]);
			} else {
				if (!JS_ConvertArguments(cx, argc, argv, "o", &_paramObj)) {
					printf( "JS_ConvertArguments failed in SFVec3f.\n");
					return JS_FALSE;
				}
				if (!JS_InstanceOf(cx, _paramObj, &SFVec3fClass, argv)) {
					printf( "SFVec3f - expected a SFVec3f parameter.\n");
					printNodeType (cx,_paramObj);
	
					return JS_FALSE;
				}
			
				/* get the second object's data */
				if ((_vec2 = JS_GetPrivate(cx, _paramObj)) == NULL) {
					printf( "JS_GetPrivate failed for _paramObj in SFVec3f.\n");
					return JS_FALSE;
				}
				pars[0]= (_vec2->v).c[0];
				pars[1] = (_vec2->v).c[1];
				pars[2] = (_vec2->v).c[2];
			}
		}
	}

	/* get our values */
	if ((_vec1 = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed for obj in SFVec3fAdd.\n");
		return JS_FALSE;
	}

	/* do the operation */
	if (JSVRMLClassesVerbose) printf ("SFVec3f generic, vec2 %f %f %f\n",pars[0],pars[1],pars[2]);
	switch (op) {
		/* returning a SFVec3f */
		case __3FADD:
			d0 = (_vec1->v).c[0] + pars[0];
			d1 = (_vec1->v).c[1] + pars[1];
			d2 = (_vec1->v).c[2] + pars[2];
			break;
		case __3FDIVIDE:
			d0 = (_vec1->v).c[0] / d;
			d1 = (_vec1->v).c[1] / d;
			d2 = (_vec1->v).c[2] / d;
			break;
		case __3FMULT:
			d0 = (_vec1->v).c[0] * d;
			d1 = (_vec1->v).c[1] * d;
			d2 = (_vec1->v).c[2] * d;
			break;
		case __3FSUBT:
			d0 = (_vec1->v).c[0] - pars[0];
			d1 = (_vec1->v).c[1] - pars[1];
			d2 = (_vec1->v).c[2] - pars[2];
			break;
		case __3FDOT:
			v1.x = (_vec1->v).c[0]; v1.y=(_vec1->v).c[1];v1.z=(_vec1->v).c[2];
			v2.x = pars[0]; v2.y=pars[1];v2.z=pars[2];
			d = vecdot (&v1, &v2);
			break;
		case __3FCROSS:
			v1.x = (_vec1->v).c[0]; v1.y=(_vec1->v).c[1];v1.z=(_vec1->v).c[2];
			v2.x = pars[0]; v2.y=pars[1];v2.z=pars[2];
			veccross(&ret, v1, v2);
			d0 = ret.x;d1 = ret.y, d2 = ret.z;
			break;
		case __3FLENGTH:
			v1.x = (_vec1->v).c[0]; v1.y=(_vec1->v).c[1];v1.z=(_vec1->v).c[2];
			d = veclength(v1);
			break;
		case __3FNORMALIZE:
			v1.x = (_vec1->v).c[0]; v1.y=(_vec1->v).c[1];v1.z=(_vec1->v).c[2];
			vecnormal(&v1, &v1);
			d0 = v1.x; d1 = v1.y; d2 = v1.z;
			break;
		case __3FNEGATE:
			d0 = -(_vec1->v).c[0];
			d1 = -(_vec1->v).c[1];
			d2 = -(_vec1->v).c[2];
			break;
		default:
			printf ("woops... %d\n",op);
		return JS_FALSE;
	}

	if (JSVRMLClassesVerbose) printf ("past calcs\n");
	/* set the return object */
	if (retSFVec3f) {
		if (JSVRMLClassesVerbose) printf ("returning SFVec3f\n");
		if ((_proto = JS_GetPrototype(cx, obj)) == NULL) {
			printf( "JS_GetPrototype failed in SFVec3f.\n");
			return JS_FALSE;
		}
		if ((_retObj =
			JS_ConstructObject(cx, &SFVec3fClass, _proto, NULL)) == NULL) {
			printf( "JS_ConstructObject failed in SFVec3f.\n");
			return JS_FALSE;
		}
		*rval = OBJECT_TO_JSVAL(_retObj);
		if ((_retNative = JS_GetPrivate(cx, _retObj)) == NULL) {
			printf( "JS_GetPrivate failed for _retObj in SFVec3f.\n");
			return JS_FALSE;
		}
		(_retNative->v).c[0] = d0;
		(_retNative->v).c[1] = d1;
		(_retNative->v).c[2] = d2;
	} else if (retNumeric) {
		if ((dp = JS_NewDouble(cx,d)) == NULL) {
			printf( "JS_NewDouble failed for %f in SFVec3f.\n",d);
			return JS_FALSE;
		}
		*rval = DOUBLE_TO_JSVAL(dp); 
	}
	if ((JSVRMLClassesVerbose) && (retSFVec3f)){
		printf("SFVec3fgeneric: obj = %u, result = [%.9g, %.9g, %.9g]\n",
			   (unsigned int) obj,
			   (_retNative->v).c[0], (_retNative->v).c[1],
			   (_retNative->v).c[2]);
	}
	if ((JSVRMLClassesVerbose) && (retNumeric)){
		printf("SFVec2fgeneric: obj = %u, result = %.9g\n",
			   (unsigned int) obj, d);
	}
return JS_TRUE;
}

JSBool
SFVec3fAdd(JSContext *cx, JSObject *obj,
		   uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FADD);
}

JSBool
SFVec3fCross(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FCROSS);
}

JSBool
SFVec3fDivide(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FDIVIDE);
}

JSBool
SFVec3fDot(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FDOT);
}

JSBool
SFVec3fLength(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FLENGTH);
}


JSBool
SFVec3fMultiply(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FMULT);
}


JSBool
SFVec3fNegate(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FNEGATE);
}

JSBool
SFVec3fNormalize(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FNORMALIZE);
}

JSBool
SFVec3fSubtract(JSContext *cx, JSObject *obj,
			 uintN argc, jsval *argv, jsval *rval) {
	return SFVec3fGeneric(cx, obj, argc, argv, rval, __3FSUBT);
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
		printf( "JS_GetPrivate failed in SFVec3fToString.\n");
		return JS_FALSE;
	}

	memset(buff, 0, STRING);
	sprintf(buff, "%.9g %.9g %.9g",
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
		printf( "JS_GetPrivate failed for obj in SFVec3fAssign.\n");
        return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, obj, &SFVec3fClass, argv)) {
		printf( "JS_InstanceOf failed for obj in SFVec3fAssign.\n");
        return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		printf( "JS_ConvertArguments failed in SFVec3fAssign.\n");
		return JS_FALSE;
	}
    if (!JS_InstanceOf(cx, _from_obj, &SFVec3fClass, argv)) {
		printf( "JS_InstanceOf failed for _from_obj in SFVec3fAssign.\n");
        return JS_FALSE;
    }
	if ((fptr = JS_GetPrivate(cx, _from_obj)) == NULL) {
		printf( "JS_GetPrivate failed for _from_obj in SFVec3fAssign.\n");
        return JS_FALSE;
	}
	if (JSVRMLClassesVerbose) {
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
		printf( "JS_GetPrivate failed in SFVec3fTouched.\n");
		return JS_FALSE;
	}

    t = ptr->touched;
	ptr->touched = 0;
    if (JSVRMLClassesVerbose) {
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
		printf( "SFVec3fNativeNew failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperties(cx, obj, SFVec3fProperties)) {
		printf( "JS_DefineProperties failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}
	if (!JS_SetPrivate(cx, obj, ptr)) {
		printf( "JS_SetPrivate failed in SFVec3fConstr.\n");
		return JS_FALSE;
	}
     	
	if (argc == 0) {
		(ptr->v).c[0] = 0.0;
		(ptr->v).c[1] = 0.0;
		(ptr->v).c[2] = 0.0;
	} else {
		if (!JS_ConvertArguments(cx, argc, argv, "d d d",
				 &(pars[0]), &(pars[1]), &(pars[2]))) {
			printf( "JS_ConvertArguments failed in SFVec3fConstr.\n");
			return JS_FALSE;
		}
		(ptr->v).c[0] = pars[0];
		(ptr->v).c[1] = pars[1];
		(ptr->v).c[2] = pars[2];
	}
	if (JSVRMLClassesVerbose) {
		printf("SFVec3fConstr: obj = %u, %u args, %f %f %f\n",
			   (unsigned int) obj, argc,
			   (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

void
SFVec3fFinalize(JSContext *cx, JSObject *obj)
{
	SFVec3fNative *ptr;

	if (JSVRMLClassesVerbose) {
		printf("SFVec3fFinalize: obj = %u\n", (unsigned int) obj);
	}
	if ((ptr = JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFVec3fFinalize.\n");
		return;
	}
	SFVec3fNativeDelete(ptr);
}

JSBool 
SFVec3fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	SFVec3fNative *ptr;
	jsdouble d, *dp;

	if ((ptr = JS_GetPrivate(cx,obj)) == NULL) {
		printf( "JS_GetPrivate failed in SFVec3fGetProperty.\n");
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
			d = (ptr->v).c[0];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
						"JS_NewDouble failed for %f in SFVec3fGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 1:
			d = (ptr->v).c[1];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
						"JS_NewDouble failed for %f in SFVec3fGetProperty.\n",
						d);
				return JS_FALSE;
			}
			*vp = DOUBLE_TO_JSVAL(dp);
			break; 
		case 2:
			d = (ptr->v).c[2];
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf(
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
		printf( "JS_GetPrivate failed in SFVec3fSetProperty.\n");
		return JS_FALSE;
	}
	ptr->touched++;
	if (JSVRMLClassesVerbose) {
		printf("SFVec3fSetProperty: obj = %u, id = %d, touched = %d\n",
			   (unsigned int) obj, JSVAL_TO_INT(id), ptr->touched);
	}

	if (!JS_ConvertValue(cx, *vp, JSTYPE_NUMBER, &myv)) {
		printf( "JS_ConvertValue failed in SFVec3fSetProperty.\n");
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



/********************************************************/
/*							*/
/* Third part - MF classes				*/
/*							*/
/********************************************************/

JSBool
MFColorToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	UNUSED(argc);
	UNUSED(argv);
	return doMFToString(cx, obj, "MFColor", rval);
}

JSBool
MFColorAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFColorClass,"MFColorAssign");
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
		printf(
				"JS_DefineProperty failed for \"length\" in MFColorConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"__touched_flag\" in MFColorConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFColorConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			printf(
					"JS_ValueToObject failed in MFColorConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFColorClass, NULL)) {
			printf( "JS_InstanceOf failed in MFColorConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			printf(
					"JS_DefineElement failed for arg %u in MFColorConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFColorAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"MFColorAddProperty");
}

JSBool 
MFColorGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return _standardMFGetProperty(cx, obj, id, vp,  
			//&SFColorClass, proto_SFColor,
			"_FreeWRL_Internal = new SFColor()", "MFColor");
}

JSBool 
MFColorSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFSetProperty(cx, obj, id, vp,"MFColorSetProperty");
}

JSBool
MFFloatToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	UNUSED(argc);
	UNUSED(argv);
	return doMFToString(cx, obj, "MFFloat", rval);
}

JSBool
MFFloatAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFFloatClass,"MFFloatAssign");
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
		printf(
				"JS_DefineProperty failed for \"length\" in MFFloatConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"__touched_flag\" in MFFloatConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFFloatConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToNumber(cx, argv[i], &_d)) {
			printf(
					"JS_ValueToNumber failed in MFFloatConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			printf(
					"JS_DefineElement failed for arg %u in MFFloatConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFFloatAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"MFFloatAddProperty");
}

JSBool 
MFFloatGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return _standardMFGetProperty(cx, obj, id, vp,  
			//&MFFloatClass, proto_MFFloat,
			"_FreeWRL_Internal = 0.0", "MFFloat");
}

JSBool 
MFFloatSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFSetProperty(cx, obj, id, vp,"MFFloatSetProperty");
}

JSBool
MFInt32ToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	UNUSED(argc);
	UNUSED(argv);
	return doMFToString(cx, obj, "MFInt32", rval);
}

JSBool
MFInt32Assign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFInt32Class,"MFInt32Assign");
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
		printf(
				"JS_DefineProperty failed for \"length\" in MFInt32Constr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"__touched_flag\" in MFInt32Constr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFInt32Constr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToInt32(cx, argv[i], &_i)) {
			printf(
					"JS_ValueToBoolean failed in MFInt32Constr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			printf(
					"JS_DefineElement failed for arg %u in MFInt32Constr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFInt32AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"MFInt32AddProperty");
}

JSBool 
MFInt32GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return _standardMFGetProperty(cx, obj, id, vp,  
			//&MFInt32Class, proto_MFInt32,
			"_FreeWRL_Internal = 0", "MFInt32");
}

JSBool 
MFInt32SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFSetProperty(cx, obj, id, vp,"MFInt32SetProperty");
}

JSBool
MFNodeToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	UNUSED(argc);
	UNUSED(argv);
	//printf ("start of MFNODETOSTRING, obj %d\n",obj);
	return doMFToString(cx, obj, "MFNode", rval);
}

JSBool
MFNodeAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	if (JSVRMLClassesVerbose) printf ("start of MFNODEASSIGN, obj %d\n",obj);
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFNodeClass,"MFNodeAssign");
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
		printf(
				"JS_DefineProperty failed for \"length\" in MFNodeConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"__touched_flag\" in MFNodeConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFNodeConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			printf(
					"JS_ValueToObject failed in MFNodeConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFNodeClass, NULL)) {
			printf( "JS_InstanceOf failed in MFNodeConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			printf(
					"JS_DefineElement failed for arg %d in MFNodeConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFNodeAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	if (JSVRMLClassesVerbose) printf ("startof MFNODEADDPROPERTY\n");
	return doMFAddProperty(cx, obj, id, vp,"MFNodeAddProperty");
}

JSBool 
MFNodeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	if (JSVRMLClassesVerbose) printf ("startof MFNODEGETPROPERTY obj %d\n");
	return _standardMFGetProperty(cx, obj, id, vp,  
			//&SFNodeClass, proto_SFNode,
			"_FreeWRL_Internal = 0",
			"MFNode");
}

JSBool
MFNodeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	JSString *_str;
	JSObject *_obj;
	jsval _val;
	char *_c;
	int32 _index;

	printf ("start of MFNODESETPROPERTY obj %d\n",obj);

	if (JSVRMLClassesVerbose && JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		printf ("MFNodeSetProperty, setting %d for obj %d\n",_index,obj);
		if (JSVAL_IS_OBJECT(*vp)) {
			if (!JS_ValueToObject(cx, *vp, &_obj)) {
				printf(
						"JS_ValueToObject failed in MFNodeSetProperty.\n");
				return JS_FALSE;
			}
			if (!JS_GetProperty(cx, _obj, "__handle", &_val)) {
				printf(
						"JS_GetProperty failed in MFNodeSetProperty.\n");
				return JS_FALSE;
			}
			_str = JSVAL_TO_STRING(_val);
			_c = JS_GetStringBytes(_str);
			printf("MFNodeSetProperty: obj = %u, id = %d, SFNode object = %u, handle = \"%s\"\n",
				   (unsigned int) obj, _index, (unsigned int) _obj, _c);
		}
	}
	return doMFSetProperty(cx, obj, id, vp,"MFNodeSetProperty");
}



JSBool
MFTimeAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"MFTimeAddProperty");
}

JSBool 
MFTimeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return _standardMFGetProperty(cx, obj, id, vp,  
			//&MFTimeClass, proto_MFTime,
			 "_FreeWRL_Internal = 0.0",
			"MFTime");
}

JSBool 
MFTimeSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFSetProperty(cx, obj, id, vp,"MFTimeSetProperty");
}

JSBool
MFTimeToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	UNUSED(argc);
	UNUSED(argv);
	return doMFToString(cx, obj, "MFTime", rval);
}

JSBool
MFTimeConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	jsdouble _d;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"length\" in MFTimeConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"__touched_flag\" in MFTimeConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFTimeConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToNumber(cx, argv[i], &_d)) {
			printf(
					"JS_ValueToNumber failed in MFTimeConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			printf(
					"JS_DefineElement failed for arg %u in MFTimeConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFTimeAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFTimeClass,"MFTimeAssign");
}



JSBool
MFVec2fAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"MFVec2fAddProperty");
}

JSBool 
MFVec2fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return _standardMFGetProperty(cx, obj, id, vp,  
			//&SFVec2fClass, proto_SFVec2f,
			 "_FreeWRL_Internal = new SFVec2f()","MFVec2f");
			// "new SFVec2f()","MFVec2f");
}

JSBool 
MFVec2fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFSetProperty(cx, obj, id, vp,"MFVec2fSetProperty");
}

JSBool
MFVec2fToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	UNUSED(argc);
	UNUSED(argv);
	return doMFToString(cx, obj, "MFVec2f", rval);
}

JSBool
MFVec2fConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSObject *_obj;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);
 
	if (!JS_DefineProperty(cx, obj, "length", v,
				   JS_PropertyStub, JS_PropertyStub,
				   JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"length\" in MFVec2fConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
				   JS_PropertyStub, JS_PropertyStub,
				   JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"__touched_flag\" in MFVec2fConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFVec2fConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			printf( "JS_ValueToObject failed in MFVec2fConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFVec2fClass, NULL)) {
			printf( "JS_InstanceOf failed in MFVec2fConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
						  JS_PropertyStub, JS_PropertyStub,
						  JSPROP_ENUMERATE)) {
			printf( "JS_DefineElement failed for arg %d in MFVec2fConstr.\n", i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFVec2fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFVec2fClass,"MFVec2fAssign");
}

/* MFVec3f */
JSBool
MFVec3fAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"MFVec3fAddProperty");
}

JSBool 
MFVec3fGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return _standardMFGetProperty(cx, obj, id, vp,  
			//&SFVec3fClass, proto_SFVec3f,
			 "_FreeWRL_Internal = new SFVec3f()","MFVec3f");
}

JSBool 
MFVec3fSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFSetProperty(cx, obj, id, vp,"MFVec3fSetProperty");
}

JSBool
MFVec3fToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	UNUSED(argc);
	UNUSED(argv);
	//printf ("CALLED MFVec3fToString\n");
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
		printf(
				"JS_DefineProperty failed for \"length\" in MFVec3fConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"__touched_flag\" in MFVec3fConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFVec3fConstr: obj = %u, %u args\n", (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			printf( "JS_ValueToObject failed in MFVec3fConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFVec3fClass, NULL)) {
			printf( "JS_InstanceOf failed in MFVec3fConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
				  JS_PropertyStub, JS_PropertyStub,
				  /* getAssignProperty, setAssignProperty, */
				  JSPROP_ENUMERATE)) {
			printf( "JS_DefineElement failed for arg %d in MFVec3fConstr.\n", i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFVec3fAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFVec3fClass,"MFVec3fAssign");
}

/* VrmlMatrix */

/* get the matrix values into a double array */
static void _getmatrix (JSContext *cx, JSObject *obj, double *fl) {
	int32 _length;
	jsval _length_val;
	jsval val;
	int i;
	double d;

	if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		printf( "JS_GetProperty failed for \"length\" in _getmatrix.\n");
		_length = 0;
	} else {
		_length = JSVAL_TO_INT(_length_val);
	}

	if (JSVRMLClassesVerbose) printf ("_getmatrix, length %d\n",_length);

	if (_length>16) _length = 16;

	for (i = 0; i < _length; i++) {
		if (!JS_GetElement(cx, obj, (jsint) i, &val)) {
			printf( "failed in get of copyElements index %d.\n", i);
			fl[i] = 0.0;
		} else {
			if (!JS_ValueToNumber(cx, val, &d)) {
				printf ("this is not a mumber!\n");
				fl[i]=0.0;
			} else fl[i]=d;
		}
	}

	/* in case our matrix was short for some reason */
	for (i=_length; i < 16; i++) {
		fl[i]=0.0;
	}
}


JSBool
VrmlMatrixToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED(argc);
	UNUSED(argv);

	return doMFToString(cx, obj, "MFFloat", rval);
}

/* get rows; used for scale and rot in getTransform */
void _get4f(double *ret, double *mat, int row) {
	if (row == 0) {ret[0]=MAT00;ret[1]=MAT01;ret[2]=MAT02;ret[3]=MAT03;}
	if (row == 1) {ret[0]=MAT10;ret[1]=MAT11;ret[2]=MAT12;ret[3]=MAT13;}
	if (row == 2) {ret[0]=MAT20;ret[1]=MAT21;ret[2]=MAT22;ret[3]=MAT23;}
}

/* set rows; used for scale and rot in getTransform */
void _set4f(double len, double *mat, int row) {
	if (row == 0) {MAT00=MAT00/len;MAT01=MAT01/len;MAT02=MAT02/len;MAT03=MAT03/len;}
	if (row == 1) {MAT10=MAT10/len;MAT11=MAT11/len;MAT12=MAT12/len;MAT13=MAT13/len;}
	if (row == 2) {MAT20=MAT20/len;MAT21=MAT21/len;MAT22=MAT22/len;MAT23=MAT23/len;}
}

JSBool
VrmlMatrixgetTransform(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int i;
    	JSObject *transObj, *rotObj, *scaleObj;
	SFRotationNative *Rptr;
	SFVec3fNative *Vptr;

    	Quaternion quat;
    	double matrix[16];
    	double qu[4];
	double r0[4], r1[4], r2[4];
	double l0,l1,l2;

	/* some intermediate calculations */
	_getmatrix(cx,obj,matrix);
	/* get each row */
	_get4f(r0,matrix,0);
	_get4f(r1,matrix,1);
	_get4f(r2,matrix,2);
	/* get the length of each row */
	l0 = sqrt(r0[0]*r0[0] + r0[1]*r0[1] + r0[2]*r0[2] +r0[3]*r0[3]);
	l1 = sqrt(r1[0]*r1[0] + r1[1]*r1[1] + r1[2]*r1[2] +r1[3]*r1[3]);
	l2 = sqrt(r2[0]*r2[0] + r2[1]*r2[1] + r2[2]*r2[2] +r2[3]*r2[3]);

	if (argc == 1) {
		if (!JS_ConvertArguments(cx, argc, argv, "o", &transObj)) {
			printf ("getTransform, invalid parameters\n");
			return JS_FALSE;
		}
	}
	if (argc == 2) {
		if (!JS_ConvertArguments(cx, argc, argv, "o o", &transObj,
					&rotObj)) {
			printf ("getTransform, invalid parameters\n");
			return JS_FALSE;
		}
	}
	if (argc == 3) {
		if (!JS_ConvertArguments(cx, argc, argv, "o o o", 
					&transObj,&rotObj,&scaleObj)) {
			printf ("getTransform, invalid parameters\n");
			return JS_FALSE;
		}
	}

	/* translation */
	if ((argc>=1) && (!JSVAL_IS_NULL(argv[0]))) {
		if (!JS_InstanceOf(cx, transObj, &SFVec3fClass, NULL)) {
			printf ("VrmlMatrix:this is not a translation!\n");
			return JS_FALSE;
		}
		if ((Vptr = JS_GetPrivate(cx, transObj)) == NULL) {
			printf( "JS_GetPrivate failed.\n");
			return JS_FALSE;
		}
		(Vptr->v).c[0] = matrix[12];
		(Vptr->v).c[1] = matrix[13];
		(Vptr->v).c[2] = matrix[14];
	}

	/* rotation */
	if ((argc>=2) && (!JSVAL_IS_NULL(argv[1]))) {
		if (!JS_InstanceOf(cx, rotObj, &SFRotationClass, NULL)) {
			printf ("VrmlMatrix:this is not a rotation!\n");
			return JS_FALSE;
		}

		if ((Rptr = JS_GetPrivate(cx, rotObj)) == NULL) {
			printf( "JS_GetPrivate failed.\n");
			return JS_FALSE;
		}

		/* apply length to each row */
		_set4f(l0, matrix, 0);
		_set4f(l1, matrix, 1);
		_set4f(l2, matrix, 2);

		/* convert the matrix to a quaternion */
		matrix_to_quaternion (&quat, matrix);
		if (JSVRMLClassesVerbose) printf ("quaternion %f %f %f %f\n",quat.x,quat.y,quat.z,quat.w);
		
		/* convert the quaternion to a VRML rotation */
		quaternion_to_vrmlrot(&quat, &qu[0],&qu[1],&qu[2],&qu[3]);
		
		/* now copy the values over */
		for (i=0; i<4; i++) (Rptr->v).r[i] = qu[i];
	}

	/* scale */
	if ((argc>=3) && (!JSVAL_IS_NULL(argv[2]))) {
		if (!JS_InstanceOf(cx, scaleObj, &SFVec3fClass, NULL)) {
			printf ("VrmlMatrix:this is not a scale!\n");
			return JS_FALSE;
		}
		if ((Vptr = JS_GetPrivate(cx, scaleObj)) == NULL) {
			printf( "JS_GetPrivate failed.\n");
			return JS_FALSE;
		}
		(Vptr->v).c[0] = l0;
		(Vptr->v).c[1] = l1;
		(Vptr->v).c[2] = l2;
	}

	*rval = JSVAL_VOID;
	return JS_TRUE;
}


JSBool
VrmlMatrixsetTransform(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED (cx); UNUSED (obj); UNUSED (argc); UNUSED (argv); UNUSED (rval);
	printf ("VrmlMatrixsetTransform\n");
	return JS_TRUE;
}


JSBool
VrmlMatrixinverse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED (cx); UNUSED (obj); UNUSED (argc); UNUSED (argv); UNUSED (rval);
	printf ("VrmlMatrixinverse\n");
	return JS_TRUE;
}


JSBool
VrmlMatrixtranspose(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED (cx); UNUSED (obj); UNUSED (argc); UNUSED (argv); UNUSED (rval);
	printf ("VrmlMatrixtranspose\n");
	return JS_TRUE;
}


JSBool
VrmlMatrixmultLeft(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED (cx); UNUSED (obj); UNUSED (argc); UNUSED (argv); UNUSED (rval);
	printf ("VrmlMatrixmultLeft\n");
	return JS_TRUE;
}


JSBool
VrmlMatrixmultRight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED (cx); UNUSED (obj); UNUSED (argc); UNUSED (argv); UNUSED (rval);
	printf ("VrmlMatrixmultRight\n");
	return JS_TRUE;
}


JSBool
VrmlMatrixmultVecMatrix(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED (cx); UNUSED (obj); UNUSED (argc); UNUSED (argv); UNUSED (rval);
	printf ("VrmlMatrixmultVecMatrix\n");
	return JS_TRUE;
}


JSBool
VrmlMatrixmultMatrixVec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	UNUSED (cx); UNUSED (obj); UNUSED (argc); UNUSED (argv); UNUSED (rval);
	printf ("VrmlMatrixmultMatrixVec\n");
	return JS_TRUE;
}


JSBool
VrmlMatrixAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &VrmlMatrixClass,"VrmlMatrixAssign");
}

JSBool
VrmlMatrixConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsdouble _d;
	unsigned int i;
	jsval v = INT_TO_JSVAL(16);
	jsdouble d, *dp;

	if ((argc != 16) && (argc != 0)) {
		printf ("VrmlMatrixConstr - require either 16 or no values\n");
		return JS_FALSE;
	}

	if (!JS_DefineProperty(cx, obj, "length", v,
				   JS_PropertyStub, JS_PropertyStub,
				   JSPROP_PERMANENT)) {
		printf(
			"JS_DefineProperty failed for \"length\" in VrmlMatrixConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
				   JS_PropertyStub, JS_PropertyStub,
				   JSPROP_PERMANENT)) {
		printf(
			"JS_DefineProperty failed for \"__touched_flag\" in VrmlMatrixConstr.\n");
		return JS_FALSE;
	}

	if (argc == 16) {
		for (i = 0; i < 16; i++) {
			if (!JS_ValueToNumber(cx, argv[i], &_d)) {
				printf(
					"JS_ValueToNumber failed in VrmlMatrixConstr.\n");
				return JS_FALSE;
			}
	
			if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
						  JS_PropertyStub, JS_PropertyStub,
						  JSPROP_ENUMERATE)) {
				printf(
					"JS_DefineElement failed for arg %u in VrmlMatrixConstr.\n",
						i);
				return JS_FALSE;
			}
		}
	} else {
		/* make the identity matrix */
		for (i=0; i<16; i++) {
			if ((i==0) || (i==5) || (i==10) || (i==15)) { d = 1.0;
			} else { d = 0.0; }
  
			if ((dp = JS_NewDouble(cx, d)) == NULL) {
				printf ("problem creating id matrix\n");
				return JS_FALSE;
			}

			if (!JS_DefineElement(cx, obj, (jsint) i, 
					  DOUBLE_TO_JSVAL(dp),
					  JS_PropertyStub, JS_PropertyStub,
					  JSPROP_ENUMERATE)) {
				printf(
					"JS_DefineElement failed for arg %u in VrmlMatrixConstr.\n",
						i);
				return JS_FALSE;
			}
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
VrmlMatrixAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"VrmlMatrixAddProperty");
}

JSBool 
VrmlMatrixGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		printf(
				"JS_GetProperty failed for \"length\" in VrmlMatrixGetProperty.\n");
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
				printf(
						"JS_DefineElement failed in VrmlMatrixGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				printf(
						"JS_LookupElement failed in VrmlMatrixGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				printf(
						"VrmlMatrixGetProperty: obj = %u, jsval = %d does not exist!\n",
					   (unsigned int) obj, (int) _index);
				return JS_FALSE;
			}
		}
	}

	return JS_TRUE;
}

JSBool 
VrmlMatrixSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFSetProperty(cx, obj, id, vp,"VrmlMatrixSetProperty");
}

/* MFRotation */
JSBool
MFRotationAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"MFRotationAddProperty");
}

JSBool 
MFRotationGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return _standardMFGetProperty(cx, obj, id, vp,  
			//&SFRotationClass, proto_SFRotation,
			 "_FreeWRL_Internal = new SFRotation()","MFRotation");
}

JSBool 
MFRotationSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFSetProperty(cx, obj, id, vp,"MFRotationSetProperty");
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
		printf(
				"JS_DefineProperty failed for \"length\" in MFRotationConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"__touched_flag\" in MFRotationConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFRotationConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToObject(cx, argv[i], &_obj)) {
			printf(
					"JS_ValueToObject failed in MFRotationConstr.\n");
			return JS_FALSE;
		}
		if (!JS_InstanceOf(cx, _obj, &SFRotationClass, NULL)) {
			printf( "JS_InstanceOf failed in MFRotationConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			printf(
					"JS_DefineElement failed for arg %d in MFRotationConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFRotationAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFRotationClass,"MFRotationAssign");
}


JSBool
MFStringAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	/* unquote parts of vp string if necessary */
	if (JSVAL_IS_STRING(*vp)) {
		if (!doMFStringUnquote(cx, vp)) {
			printf(
				"doMFStringUnquote failed in MFStringAddProperty.\n");
			return JS_FALSE;
		}
	}
	return doMFAddProperty(cx, obj, id, vp,"MFStringAddProperty");
}

JSBool 
MFStringGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_str;
	int32 _length, _index;
    jsval _length_val;

    if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		printf(
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
				printf(
						"JS_DefineElement failed in MFStringGetProperty.\n");
				return JS_FALSE;
			}
		} else {
			if (!JS_LookupElement(cx, obj, _index, vp)) {
				printf(
						"JS_LookupElement failed in MFStringGetProperty.\n");
				return JS_FALSE;
			}
			if (*vp == JSVAL_VOID) {
				printf(
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
			printf(
				"doMFStringUnquote failed in MFStringSetProperty.\n");
			return JS_FALSE;
		}
	}
	return doMFSetProperty(cx, obj, id, vp,"MFStringSetProperty");
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
		printf(
				"JS_DefineProperty failed for \"length\" in MFStringConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf(
				"JS_DefineProperty failed for \"__touched_flag\" in MFStringConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	if (JSVRMLClassesVerbose) {
		printf("MFStringConstr: obj = %u, %u args\n",
			   (unsigned int) obj, argc);
	}
	for (i = 0; i < argc; i++) {
		if ((_str = JS_ValueToString(cx, argv[i])) == NULL) {
			printf(
					"JS_ValueToString failed in MFStringConstr.\n");
			return JS_FALSE;
		}
		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			printf(
					"JS_DefineElement failed for arg %d in MFStringConstr.\n",
					i);
			return JS_FALSE;
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFStringAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return _standardMFAssign (cx, obj, argc, argv, rval, &MFStringClass,"MFStringAssign");
}
