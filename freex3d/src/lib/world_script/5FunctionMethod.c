/*
=INSERT_TEMPLATE_HERE=

$Id: jsVRMLClasses.c,v 1.35 2012/03/18 15:41:35 dug9 Exp $

???

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#define JS_VERSION 185
#define HAVE_JAVASCRIPT 1

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif
 
#include "FWTYPE.h"
 
#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#include "jsapi.h"

static FWType *FWTYPEs = NULL;
char *jsformats[] = {"d","d","s","o","o"}; //fix

//constructor
JSBool jsConstructor(FWType fwtype, JSObject *obj, JSContext *cx, uintN argc, jsval *argv) {
	FWNative fwn;
	int arg_scheme,i;
	FWval fwvals;
	JSPropertySpec *jsprops;

	if ((fwn = (FWNative) FWNativeNew()) == NULL) {
		printf( "NativeNew failed in %sconstructor.\n",fwtype->name);
		return JS_FALSE;
	}
	fwn->fwtype = fwtype;
	
	//>>Property definition on Ojbect
	//Q, can/should defineProperties be moved to JS_InitClass prototype initialization?
	jsprops = (struct JSPropertySpec *)malloc((fwtype->nprops+1)*sizeof(struct JSPropertySpec));
	memset(jsprops,0,(fwtype->nprops+1)*sizeof(struct JSPropertySpec));
	for(i=0;i<fwtype->nprops;i++){
		jsprops[i].name = fwtype->Properties[i].name;
		jsprops[i].tinyid = (int8)fwtype->Properties[i].index;
	}
	
	if (!JS_DefineProperties(cx, obj, jsprops)) {
		printf( "JS_DefineProperties failed in %sConstr.\n",fwtype->name);
		return JS_FALSE;
	}
	for(i=0;i<fwtype->nprops+1;i++)
		FREE_IF_NZ(jsprops[i]);
	FREE_IF_NZ(jsprops);
	//<<< Property Definition on Object

	if (!JS_SetPrivate(cx, obj, fwn)) {
		printf( "JS_SetPrivate failed in %sConstr.\n",fwtype->name);
		return JS_FALSE;
	}

	arg_scheme = -1;
	for(i=0;i<fwtype->nschemes;i++)
		if(argc == fwtype->constructorSchemes[i].argc) arg_scheme = i;
	if(arg_scheme == -1){
		printf( "Invalid number of arguments for %sConstr.\n",fwtype->name);
		return JS_FALSE;
	}
	fwvals = FWvalsNew(argc);
	for(i=0;i<argc;i++)
	{
		if (!JS_ConvertArguments(cx, 1, &argv[i], 
			jsformats[fwtype->constructorSchemes[arg_scheme].argtypes[i]], 
			&fwvals[i].ptr))	
		{
				//itype[fwtype->constructorSchemes.schemes[arg_scheme].argTypes[i].valtype]
			printf( "Invalid argument type %d for %sConstr.\n",i,fwtype->name);
			FREE_IF_NZ(fwn);
			return JS_FALSE;
		}
	}
	if(!fwtype->constr(fwvals,arg_scheme,argc,fwn)) {
		FREE_IF_NZ(fwvals);
		FREE_IF_NZ(fwn);
		return JS_FALSE;
	}
	//int SFColorConstr(FWval fwvals, int arg_scheme, int argc, FWNative ptr){
	fwn->valueChanged = 1;
	FREE_IF_NZ(fwvals);

#if JS_VERSION < 185
	*rval = OBJECT_TO_JSVAL(obj);
#else
	JS_SET_RVAL(cx,&argv[-2],OBJECT_TO_JSVAL(obj));
#endif
return JS_TRUE;
}

void generateClass(FWType fwtype,JSClass *jsclass);


JSBool
#if JS_VERSION < 185
cf(int index, JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
cf(int index, JSContext *cx, uintN argc, jsval *vp) {
#endif
	FWNative *ptr;
	JSClass jsclass;
	FWType fwtype = FWTYPEs[index];
#if JS_VERSION < 185
#else
	jsval hrval;
	JSObject *obj;
	jsval *argv, *rval = &hrval; //stack OK? or do we need heap?
	generateClass(fwtype,&jsclass);
    obj = JS_NewObject(cx,&jsclass,NULL,NULL);
    argv = JS_ARGV(cx,vp);
	//uintN argc = JS_ARGC(cx,vp);
#endif
	jsConstructor(fwtype, obj, cx, argc, argv);
#if JS_VERSION < 185
	*rval = OBJECT_TO_JSVAL(obj);
#else
	JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(obj));
#endif
	return JS_TRUE;
}

JSBool
#if JS_VERSION < 185
cf0( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return cf(0,cx,obj,argc,argv,rval);
}
cf1( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return cf(1,cx,obj,argc,argv,rval);
}
cf2( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return cf(2,cx,obj,argc,argv,rval);
}
cf3( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return cf(3,cx,obj,argc,argv,rval);
}
cf4( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return cf(4,cx,obj,argc,argv,rval);
}
cf5( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return cf(5,cx,obj,argc,argv,rval);
}

#else
cf0(JSContext *cx, uintN argc, jsval *vp) {
	return cf(0,cx,argc,vp);
}
cf1(JSContext *cx, uintN argc, jsval *vp) {
	return cf(1,cx,argc,vp);
}
cf2(JSContext *cx, uintN argc, jsval *vp) {
	return cf(2,cx,argc,vp);
}
cf3(JSContext *cx, uintN argc, jsval *vp) {
	return cf(3,cx,argc,vp);
}
cf4(JSContext *cx, uintN argc, jsval *vp) {
	return cf(4,cx,argc,vp);
}
cf5(JSContext *cx, uintN argc, jsval *vp) {
	return cf(5,cx,argc,vp);
}
#endif
JSNative constructorArray [] = {
	cf0, cf1, cf2, cf3, cf4, cf5,
};



//finalizer
void FWFinalize(JSContext *cx, JSObject *obj)
{
	FWNative ptr;
	if ((ptr = (FWNative)JS_GetPrivate(cx, obj)) == NULL) {
		return;
	} else {
		if(ptr->wasMalloced) FREE_IF_NZ(ptr->native);
		FREE_IF_NZ (ptr);
        }
}
//getter
JSBool
#if JS_VERSION < 185
FWgetter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
FWgetter(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	FWNative fwn;
	jsdouble d;
#if JS_VERSION >= 185
	jsval id;
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in SFColorGetProperty.\n");
		return JS_FALSE;
	}
#endif

	if ((fwn = (FWNative)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in GetProperty.\n");
		return JS_FALSE;
	}
	if (JSVAL_IS_INT(id)) {
		struct FWVAL fwval;
		int index = JSVAL_TO_INT(id);
		if(fwn->fwtype->setter(index,fwn,&fwval)){
			//0 = ival, 1=dval, 2=cval, 3=fwnval 4=vrmlval 5=ptr
			//there could/should be something in fwtype to say what type
			//the JS return type should be from getter 
			//especially when its a JSClass (vs js scalar)
			//the fwval.itype is more primitive / just for scalars
			switch(fwval.itype){
				case 0: //ival
					break;
				case 1: //dval
					d = fwval.dval;
					if (JS_NewNumberValue(cx, d, vp) == JS_FALSE) {
						printf(	"JS_NewDouble failed for %f in %sGetProperty.\n",
								d, fwn->fwtype->name);
						return JS_FALSE;
					}
					break;
				case 2: //cval
				case 3: //fwnval
				case 4: //vrmlval
				case 5: //ptr (void*)
					break;
			}
		}else{
			return JS_FALSE;
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

//setter
JSBool
#if JS_VERSION < 185
FWsetter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
FWsetter(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	FWNative fwn;
	jsval _val;
	struct FWVAL fwval;
#if JS_VERSION >= 185
	jsval id;
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in SFColorSetProperty.\n");
		return JS_FALSE;
	}
#endif

	if ((fwn = (FWNative)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in FWsetter.\n");
		return JS_FALSE;
	}
	#ifdef JSVRMLCLASSESVERBOSE
		printf("SFColorSetProperty: obj = %p, id = %d, valueChanged = %d\n",
			   obj, JSVAL_TO_INT(id), ptr->valueChanged);
	#endif


	if (JSVAL_IS_INT(id)) {
		JSType jstype;
		int index = JSVAL_TO_INT(id);
		//0 = ival, 1=dval, 2=cval, 3=fwnval 4=vrmlval 5=ptr
		switch(fwn->fwtype->Properties[index].type){
			case 0: 
			case 1:
				jstype = JSTYPE_NUMBER; break;
			case 2:
				jstype = JSTYPE_STRING; break;
			case 3:
			case 4:
			case 5:
				jstype = JSTYPE_OBJECT; break;
		}
		if (!JS_ConvertValue(cx, *vp, jstype, &_val)) {
			printf( "JS_ConvertValue failed in %sSetProperty.\n",fwn->fwtype->name);
			return JS_FALSE;
		}
		//0 = ival, 1=dval, 2=cval, 3=fwnval 4=vrmlval 5=ptr
		switch(fwn->fwtype->Properties[index].type){
			case 0: 
			case 1:
#if JS_VERSION < 185
				fwval.dval  =  *JSVAL_TO_DOUBLE(_val);
#else
				fwval.dval  =  JSVAL_TO_DOUBLE(_val);
#endif
				fwval.itype=1; 
				break;
			case 2:
				fwval.cval = JS_EncodeString(cx,JSVAL_TO_STRING(_val)); 
				fwval.itype=2; 
				break;
			case 3:
			case 4:
			case 5:
				jstype = JSTYPE_OBJECT; break;
		}

		fwn->valueChanged++;
		if(!fwn->fwtype->setter(index,fwn,fwval)){
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}

//function
JSBool
#if JS_VERSION < 185
SFColorRGBAGetHSV(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
SFColorRGBAGetHSV(JSContext *cx, uintN argc, jsval *vp) {
        JSObject *obj = JS_THIS_OBJECT(cx,vp);
        jsval *argv = JS_ARGV(cx,vp);
#endif


	JSObject *_arrayObj;
	jsdouble hue = 0, saturation = 0, value = 0;
	jsval _v;

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);
	/* do conversion here!!! */

	if ((_arrayObj = JS_NewArrayObject(cx, 0, NULL)) == NULL) {
		printf( "JS_NewArrayObject failed in SFColorRGBAGetHSV.\n");
		return JS_FALSE;
	}

	/* construct new double before conversion? */
	JS_NewNumberValue(cx,hue,&_v); /* was: 	_v = DOUBLE_TO_JSVAL(&hue); */
	if (!JS_SetElement(cx, _arrayObj, 0, &_v)) {
		printf( "JS_SetElement failed for hue in SFColorRGBAGetHSV.\n");
		return JS_FALSE;
	}
	JS_NewNumberValue(cx,saturation,&_v); /* was: _v = DOUBLE_TO_JSVAL(&saturation); */
	if (!JS_SetElement(cx, _arrayObj, 1, &_v)) {
		printf( "JS_SetElement failed for saturation in SFColorRGBAGetHSV.\n");
		return JS_FALSE;
	}
	JS_NewNumberValue(cx,value,&_v); /* was: _v = DOUBLE_TO_JSVAL(&value); */
	if (!JS_SetElement(cx, _arrayObj, 2, &_v)) {
		printf( "JS_SetElement failed for value in SFColorRGBAGetHSV.\n");
		return JS_FALSE;
	}
#if JS_VERSION < 185
	*rval = OBJECT_TO_JSVAL(_arrayObj);
#else
	JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_arrayObj));
#endif

    return JS_TRUE;
}
JSBool
#if JS_VERSION < 185
f0(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(0,cx,obj,argc,argv,rval);
}
f1(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(1,cx,obj,argc,argv,rval);
}
f2(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(2,cx,obj,argc,argv,rval);
}
f3(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(3,cx,obj,argc,argv,rval);
}
f4(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(4,cx,obj,argc,argv,rval);
}
f5(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(5,cx,obj,argc,argv,rval);
}
f6(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(6,cx,obj,argc,argv,rval);
}
f7(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(7,cx,obj,argc,argv,rval);
}
f8(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(8,cx,obj,argc,argv,rval);
}
f9(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return function(9,cx,obj,argc,argv,rval);
}
#else
f0(JSContext *cx, uintN argc, jsval *vp) {
	return function(0,cx,argc,vp);
}
f1(JSContext *cx, uintN argc, jsval *vp) {
	return function(1,cx,argc,vp);
}
f2(JSContext *cx, uintN argc, jsval *vp) {
	return function(2,cx,argc,vp);
}
f3(JSContext *cx, uintN argc, jsval *vp) {
	return function(3,cx,argc,vp);
}
f4(JSContext *cx, uintN argc, jsval *vp) {
	return function(4,cx,argc,vp);
}
f5(JSContext *cx, uintN argc, jsval *vp) {
	return function(5,cx,argc,vp);
}
f6(JSContext *cx, uintN argc, jsval *vp) {
	return function(6,cx,argc,vp);
}
f7(JSContext *cx, uintN argc, jsval *vp) {
	return function(7,cx,argc,vp);
}
f8(JSContext *cx, uintN argc, jsval *vp) {
	return function(8,cx,argc,vp);
}
f9(JSContext *cx, uintN argc, jsval *vp) {
	return function(9,cx,argc,vp);
}
#endif
JSNative functionArray [] = {
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9,
};

void generateClass(FWType fwtype,JSClass *jsclass){
	jsclass->name = fwtype->name; //*
	jsclass->flags = JSCLASS_HAS_PRIVATE;
	jsclass->addProperty = JS_PropertyStub;
	jsclass->delProperty = JS_PropertyStub;
	jsclass->getProperty = FWgetter; //*
	jsclass->setProperty = FWsetter; //*
	jsclass->enumerate = JS_EnumerateStub;
	jsclass->resolve = JS_ResolveStub;
	jsclass->convert = JS_ConvertStub;
	jsclass->finalize = FWFinalize; //*
}
JSFunctionSpec *generateFunctionSpec(FWType fwtype){
	int i;
	JSFunctionSpec *fspec;
	fspec = (JSFunctionSpec*)malloc(sizeof(JSFunctionSpec)*fwtype->nfuncs+1);
	memset(fspec,0,sizeof(JSFunctionSpec)*fwtype->nfuncs+1);
	for(i=0;i<fwtype->nfuncs;i++){
		fspec[i].call = functionArray[i]; //f0 to f9
		fspec[i].name = fwtype->Functions[i].name;
	}
	return fspec;
}
#define INIT_ARGC 0

struct Stuff{
	JSContext *context;
	JSObject *globalObj;
};
#ifdef __cplusplus
extern "C" 
#endif
void *initializeContext(){ 
	return malloc(4);
}
#ifdef __cplusplus
extern "C" 
#endif
int initializeClassInContext(int iclass, void *opaque, FWType fwtype) {
	JSObject *myProto;
	jsval v;
	JSClass jsclass;  //on the local stack OK?
	JSFunctionSpec *fspec; //malloced but freed in here OK?
	JSContext *context;
	JSObject *globalObj;
	struct Stuff* stuff = (struct Stuff*)opaque;
	context = stuff->context;
	globalObj = stuff->globalObj;

	/* v = 0; */
	generateClass(fwtype,&jsclass);
	fspec = generateFunctionSpec(fwtype); //SHOULD BE POINTER**?
	if (( myProto = JS_InitClass(context, globalObj, NULL, &jsclass,
		  constructorArray[iclass], INIT_ARGC, NULL,
		  fspec, NULL, NULL)) == NULL) {
		printf("JS_InitClass for %s failed in loadVrmlClasses.\n",fwtype->name);
		return 0;
	}
	FREE_IF_NZ(fspec);
	v = OBJECT_TO_JSVAL(myProto);
	if (!JS_SetProperty(context, globalObj, fwtype->name, &v)) {
		printf("JS_SetProperty for %s failed in loadVrmlClasses.\n",fwtype->name);
		return 0;
	}
	return	1;
}



//#endif /* HAVE_JAVASCRIPT */
