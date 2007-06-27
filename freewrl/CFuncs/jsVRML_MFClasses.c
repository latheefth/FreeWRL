
/*
 * Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
 * 2007 John Stewart CRC Canada.
 *
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License
 * (file COPYING in the distribution) for conditions of use and
 * redistribution, EXCEPT on the files which belong under the
 * Mozilla public license.
 *
 * $Id$
 *
 */
#include "headers.h"
#include "jsVRMLClasses.h"


/********************************************************/
/*							*/
/* Third part - MF classes				*/
/*							*/
/********************************************************/

/* remove any private data from this datatype, and let the garbage collector handle the object */

void
JS_MY_Finalize(JSContext *cx, JSObject *obj)
{
	void *ptr;
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("finalizing %x\n",obj);
	#endif

	REMOVE_ROOT(cx,obj)

	if ((ptr = (void *)JS_GetPrivate(cx, obj)) != NULL) {
		FREE_IF_NZ(ptr);
	}
}

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
	
	ADD_ROOT(cx,obj)

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

	#ifdef JSVRMLCLASSESVERBOSE
		printf("MFColorConstr: obj = %u, %u args\n",
			   VERBOSE_OBJ obj, argc);
	#endif

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

	ADD_ROOT(cx,obj)

	if (!JS_DefineProperty(cx, obj, "length", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"length\" in MFFloatConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
						   JS_PropertyStub, JS_PropertyStub,
						   JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"__touched_flag\" in MFFloatConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	#ifdef JSVRMLCLASSESVERBOSE
		printf("MFFloatConstr: obj = %u, %u args\n",
			   VERBOSE_OBJ obj, argc);
	#endif
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToNumber(cx, argv[i], &_d)) {
			printf( "JS_ValueToNumber failed in MFFloatConstr.\n");
			return JS_FALSE;
		}

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
							  JS_PropertyStub, JS_PropertyStub,
							  JSPROP_ENUMERATE)) {
			printf( "JS_DefineElement failed for arg %u in MFFloatConstr.\n", i);
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
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("start of MFInt32ToString\n");
	#endif

	return doMFToString(cx, obj, "MFInt32", rval);
}

JSBool
MFInt32Assign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("start of MFInt32Assign\n");
	#endif

	return _standardMFAssign (cx, obj, argc, argv, rval, &MFInt32Class,"MFInt32Assign");
}


JSBool
MFInt32Constr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int32 _i;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("start of MFInt32Constr\n");
	#endif

	ADD_ROOT(cx,obj)

	if (!JS_DefineProperty(cx, obj, "length", v, JS_PropertyStub, JS_PropertyStub, JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"length\" in MFInt32Constr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v, JS_PropertyStub, JS_PropertyStub, JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"__touched_flag\" in MFInt32Constr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	#ifdef JSVRMLCLASSESVERBOSE
		printf("MFInt32Constr: obj = %u, %u args\n", VERBOSE_OBJ obj, argc);
	#endif

	/* any values here that we should add in? */
	for (i = 0; i < argc; i++) {
		if (!JS_ValueToInt32(cx, argv[i], &_i)) {
			printf( "JS_ValueToBoolean failed in MFInt32Constr.\n");
			return JS_FALSE;
		}
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("value at %d is %d\n",i,_i);
		#endif

		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
			  JS_PropertyStub, JS_PropertyStub, JSPROP_ENUMERATE)) {
			printf( "JS_DefineElement failed for arg %u in MFInt32Constr.\n", i);
			return JS_FALSE;
		}
	}
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("setting __touched_flag\n");
	#endif
	v = INT_TO_JSVAL(1);

	if (!JS_SetProperty(cx, obj, "__touched_flag", &v)) {
		printf( "JS_SetProperty failed for \"__touched_flag\" in doMFSetProperty.\n");
		return JS_FALSE;
	}

	*rval = OBJECT_TO_JSVAL(obj);

	return JS_TRUE;
}

JSBool
MFInt32AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("start of MFInt32AddProperty\n");
	#endif

	return doMFAddProperty(cx, obj, id, vp,"MFInt32AddProperty");
}

JSBool
MFInt32GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("start of MFInt32GetProperty\n");
	#endif

	return _standardMFGetProperty(cx, obj, id, vp,
			"_FreeWRL_Internal = 0", "MFInt32");
}

JSBool
MFInt32SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("start of MFInt32SetProperty\n");
	#endif

	return doMFSetProperty(cx, obj, id, vp,"MFInt32SetProperty");
}


JSBool
MFNodeToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	UNUSED(argc);
	UNUSED(argv);
	/* printf ("start of MFNODETOSTRING, obj %d\n",obj);*/
	return doMFToString(cx, obj, "MFNode", rval);
}

JSBool
MFNodeAssign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("start of MFNODEASSIGN, obj %d\n",obj);
	#endif

	return _standardMFAssign (cx, obj, argc, argv, rval, &MFNodeClass,"MFNodeAssign");
}

JSBool
MFNodeConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	ADD_ROOT(cx,obj)

	if (!JS_DefineProperty(cx, obj, "length", v,
		   JS_PropertyStub, JS_PropertyStub, JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"length\" in MFNodeConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v,
		   JS_PropertyStub, JS_PropertyStub, JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"__touched_flag\" in MFNodeConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	#ifdef JSVRMLCLASSESVERBOSE
		printf("MFNodeConstr: obj = %u, %u args\n", VERBOSE_OBJ obj, argc);
	#endif

	for (i = 0; i < argc; i++) {
		if (JSVAL_IS_OBJECT(argv[i])) {

			if (!JS_ValueToObject(cx, argv[i], &_obj)) {
				printf( "JS_ValueToObject failed in MFNodeConstr.\n");
				return JS_FALSE;
			}
			if (!JS_InstanceOf(cx, _obj, &SFNodeClass, NULL)) {
				printf( "JS_InstanceOf failed in MFNodeConstr.\n");
				return JS_FALSE;
			}
	
			if (!JS_DefineElement(cx, obj, (jsint) i, argv[i],
								  JS_PropertyStub, JS_PropertyStub,
								  JSPROP_ENUMERATE)) {
				printf( "JS_DefineElement failed for arg %d in MFNodeConstr.\n", i);
				return JS_FALSE;
			}
		} else {
			/* if a NULL is passed in, eg, we have a script with an MFNode eventOut, and
			   nothing sets it, we have a NULL here. Lets just ignore it */
			/* hmmm - this is not an object - lets see... */
			#ifdef JSVRMLCLASSESVERBOSE
			if (JSVAL_IS_NULL(argv[i])) { printf ("MFNodeConstr - its a NULL\n");}
			if (JSVAL_IS_INT(argv[i])) { printf ("MFNodeConstr - its a INT\n");}
			if (JSVAL_IS_STRING(argv[i])) { printf ("MFNodeConstr - its a STRING\n");}
			#endif
		}
	}
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

JSBool
MFNodeAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("startof MFNODEADDPROPERTY\n");
	#endif
	return doMFAddProperty(cx, obj, id, vp,"MFNodeAddProperty");
}

JSBool
MFNodeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("startof MFNODEGETPROPERTY obj %d\n");
	#endif
	return _standardMFGetProperty(cx, obj, id, vp,
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

	/* printf ("start of MFNODESETPROPERTY obj %d\n",obj); */

	#ifdef JSVRMLCLASSESVERBOSE
	if (JSVAL_IS_INT(id)) {
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
				   VERBOSE_OBJ obj, _index, VERBOSE_OBJ _obj, _c);
		}
	}
	#endif
	return doMFSetProperty(cx, obj, id, vp,"MFNodeSetProperty");
}



JSBool
MFTimeAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return doMFAddProperty(cx, obj, id, vp,"MFTimeAddProperty");
}

JSBool
MFTimeGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return _standardMFGetProperty(cx, obj, id, vp,
			/* &MFTimeClass, proto_MFTime,*/
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

	ADD_ROOT(cx,obj)

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

	#ifdef JSVRMLCLASSESVERBOSE
		printf("MFTimeConstr: obj = %u, %u args\n",
			   VERBOSE_OBJ obj, argc);
	#endif
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
			 "_FreeWRL_Internal = new SFVec2f()","MFVec2f");
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

	ADD_ROOT(cx,obj)

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

	#ifdef JSVRMLCLASSESVERBOSE
		printf("MFVec2fConstr: obj = %u, %u args\n",
			   VERBOSE_OBJ obj, argc);
	#endif

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
	/* printf ("CALLED MFVec3fToString\n");*/
	return doMFToString(cx, obj, "MFVec3f", rval);
}

JSBool
MFVec3fConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	ADD_ROOT(cx,obj)

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

	#ifdef JSVRMLCLASSESVERBOSE
		printf("MFVec3fConstr: obj = %u, %u args\n", VERBOSE_OBJ obj, argc);
	#endif	
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

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("_getmatrix, length %d\n",_length);
	#endif


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
		if ((Vptr = (SFVec3fNative *)JS_GetPrivate(cx, transObj)) == NULL) {
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

		if ((Rptr = (SFRotationNative*)JS_GetPrivate(cx, rotObj)) == NULL) {
			printf( "JS_GetPrivate failed.\n");
			return JS_FALSE;
		}

		/* apply length to each row */
		_set4f(l0, matrix, 0);
		_set4f(l1, matrix, 1);
		_set4f(l2, matrix, 2);

		/* convert the matrix to a quaternion */
		matrix_to_quaternion (&quat, matrix);
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("quaternion %f %f %f %f\n",quat.x,quat.y,quat.z,quat.w);
		#endif

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
		if ((Vptr = (SFVec3fNative*)JS_GetPrivate(cx, scaleObj)) == NULL) {
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

	ADD_ROOT(cx,obj)

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
					   VERBOSE_OBJ obj, (int) _index);
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

	ADD_ROOT(cx,obj)

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

	#ifdef JSVRMLCLASSESVERBOSE
		printf("MFRotationConstr: obj = %u, %u args\n",
			   VERBOSE_OBJ obj, argc);
	#endif
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

/* MFStrings */
JSBool
MFStringAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	#ifdef JSVRMLCLASSESVERBOSE
	printf("MFStringAddProperty: vp = %u\n", VERBOSE_OBJ obj);
                if (JSVAL_IS_STRING(*vp)==TRUE) {
		printf("	is a common string :%s:\n",
                        JS_GetStringBytes(JS_ValueToString(cx, *vp)));
                }
                if (JSVAL_IS_OBJECT(*vp)==TRUE) {
                        printf ("       parameter is an object\n");
                }
                if (JSVAL_IS_PRIMITIVE(*vp)==TRUE) {
                        printf ("       parameter is a primitive\n");
                }
		if (JSVAL_IS_NULL(*vp)) { printf ("	- its a NULL\n");}
		if (JSVAL_IS_INT(*vp)) { printf ("	- its a INT %d\n",JSVAL_TO_INT(*vp));}

		printf("MFStringAddProperty: id = %u\n", VERBOSE_OBJ obj);
                if (JSVAL_IS_STRING(id)==TRUE) {
		printf("	is a common string :%s:\n",
                        JS_GetStringBytes(JS_ValueToString(cx, id)));
                }
                if (JSVAL_IS_OBJECT(id)==TRUE) {
                        printf ("       parameter is an object\n");
                }
                if (JSVAL_IS_PRIMITIVE(id)==TRUE) {
                        printf ("       parameter is a primitive\n");
                }
		if (JSVAL_IS_NULL(id)) { printf ("	- its a NULL\n");}
		if (JSVAL_IS_INT(id)) { printf ("	- its a INT %d\n",JSVAL_TO_INT(id));}

	#endif


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

	#ifdef JSVRMLCLASSESVERBOSE
	printf("MFStringGetProperty: obj = %u\n", VERBOSE_OBJ obj);
	#endif

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
					   VERBOSE_OBJ obj, (int) _index);
				return JS_FALSE;
			}
		}
	}

	return JS_TRUE;
}

JSBool
MFStringSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	#ifdef JSVRMLCLASSESVERBOSE
	printf("MFStringSetProperty: obj = %u\n", VERBOSE_OBJ obj);
	#endif

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
	#ifdef JSVRMLCLASSESVERBOSE
	printf("MFStringToString: obj = %u, %u args\n", VERBOSE_OBJ obj, argc);
	#endif


	return doMFToString(cx, obj, "MFString", rval);
}

JSBool
MFStringConstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;
	unsigned int i;
	jsval v = INT_TO_JSVAL(argc);

	ADD_ROOT(cx,obj)

	#ifdef JSVRMLCLASSESVERBOSE
	printf("MFStringConstr: obj = %u, %u args\n", VERBOSE_OBJ obj, argc);
	#endif

	if (!JS_DefineProperty(cx, obj, "length", v, JS_PropertyStub, JS_PropertyStub, JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"length\" in MFStringConstr.\n");
		return JS_FALSE;
	}

	v = INT_TO_JSVAL(0);
	if (!JS_DefineProperty(cx, obj, "__touched_flag", v, JS_PropertyStub, JS_PropertyStub, JSPROP_PERMANENT)) {
		printf( "JS_DefineProperty failed for \"__touched_flag\" in MFStringConstr.\n");
		return JS_FALSE;
	}
	if (!argv) {
		return JS_TRUE;
	}

	for (i = 0; i < argc; i++) {
		#ifdef JSVRMLCLASSESVERBOSE
	  	printf ("MFStringConstr: argv %d is a ...\n",i);

		if (JSVAL_IS_STRING(argv[i])==TRUE) {
        	        printf ("	parameter %d , a Common String, is",i);
			_str = JS_ValueToString(cx, argv[i]);
			printf (JS_GetStringBytes(_str));
			printf ("\n");
		
	        }                                          
		if (JSVAL_IS_OBJECT(argv[i])==TRUE) {   
	                printf ("	parameter %d is an object\n",i);
	        }                       
		if (JSVAL_IS_PRIMITIVE(argv[i])==TRUE) {
        	        printf ("	parameter %d is a primitive\n",i);
        	}

		if ((_str = JS_ValueToString(cx, argv[i])) == NULL) {
			printf( "JS_ValueToString failed in MFStringConstr.\n");
			return JS_FALSE;
		}
		#endif

	
		if (!JS_DefineElement(cx, obj, (jsint) i, argv[i], JS_PropertyStub, JS_PropertyStub, JSPROP_ENUMERATE)) {
			printf( "JS_DefineElement failed for arg %d in MFStringConstr.\n", i);
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
