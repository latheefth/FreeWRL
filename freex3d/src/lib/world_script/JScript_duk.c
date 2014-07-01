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

#include <config.h>
#if defined(JAVASCRIPT_DUK)
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../vrml_parser/CRoutes.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/EAIHelpers.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"

#include "JScript.h"
#include "FWTYPE.h"
#include "duktape/duktape.h"

typedef int indexT;

FWTYPE *fwtypesArray[30];  //true statics - they only need to be defined once per process
int FWTYPES_COUNT = 0;

void initVRMLBrowser(FWTYPE** typeArray, int *n);
void initFWTYPEs(){
	initVRMLBrowser(fwtypesArray, &FWTYPES_COUNT);
}
FWTYPE *getFWTYPE(int itype){
	int i;
	for(i=0;i<FWTYPES_COUNT;i++){
		if(itype == fwtypesArray[i]->itype)
			return fwtypesArray[i];
	}
	return NULL;
}
FWFunctionSpec *getFWFunc(FWTYPE *fwt,const char *key){
	int i = 0;
	FWFunctionSpec *fs = fwt->Functions;
	if(fs)
	while(fs[i].name){
		if(!strcmp(fs[i].name,key)){
			//found it - its a function, return functionSpec
			return &fs[i];
		}
		i++;
	}
	return NULL;
}
FWPropertySpec *getFWProp(FWTYPE *fwt,const char *key, int *index){
	int i = 0;
	FWPropertySpec *ps = fwt->Properties;
	*index = 0;
	if(ps)
	while(ps[i].name){
		if(!strcmp(ps[i].name,key)){
			//found it - its a property, return propertySpec
			(*index) = ps[i].index; //index can be any +- integer
			return &ps[i];
		}
		i++;
	}
	return NULL;
}


typedef struct pJScript{
	int ijunk;
}* ppJScript;


void *JScript_constructor(){
	void *v = malloc(sizeof(struct pJScript));
	memset(v,0,sizeof(struct pJScript));
	return v;
}
void JScript_init(struct tJScript *t){
	//public
	t->JSglobal_return_val = NULL;
	//private
	t->prv = JScript_constructor();
	{
		ppJScript p = (ppJScript)t->prv;
		//initialize statics
		if(!FWTYPES_COUNT) initFWTYPEs();
	}
}
//	ppJScript p = (ppJScript)gglobal()->JScript.prv;

//stubs the linker will be looking for
void jsVRMLBrowser_init(void *t){}
void jsUtils_init(void *t){}
void jsVRMLClasses_init(void *t){}


#include <stdio.h>
#include <memory.h>
static char buf[2048];
static int n = 0;
int fwwrite(const void *buffer, int size, int count, FILE *target);
int fwflush(FILE *target);
int fwwrite(const void *addon, int size, int count, FILE *target)
{
	if(target == stdout || target == stderr){
		memcpy(&buf[n], addon, size*count);
		n += size*count;
		return size*count;
	}else{
		return fwrite(addon,size,count,target);
	}
}
int fwflush(FILE *target)
{
	if(target == stdout || target == stderr){
		buf[n] = '\0';
		printf("%s", buf);
		n = 0;
		return 0;
	}else{
		return fflush(target);
	}

}

void show_stack0(duk_context *ctx, char* comment, int dig)
{
	int rc, itop = duk_get_top(ctx);
	if(comment) printf("%s top=%d\n",comment,itop);
	//printf("%10s%10s%10s\n","position","type","more");
	printf("%10s%10s\n","position","type");
	for(int i=0;i<itop;i++){
		int ipos = -(i+1);
		int t = duk_get_type(ctx, ipos);
		char *stype = NULL;
		const char * amore = "";
		switch(t){
			case DUK_TYPE_NUMBER: stype ="number"; break;
			case DUK_TYPE_STRING: stype ="string"; 
				if(dig) amore = duk_get_string(ctx,ipos);
				break;

			case DUK_TYPE_OBJECT: stype ="object"; 
				if(dig){
				rc = duk_get_prop_string(ctx, ipos, "fwType");
				if(rc == 1) amore = duk_to_string(ctx,-1);
				duk_pop(ctx);
				}
				break;
			case DUK_TYPE_NONE: stype ="none"; break;
			case DUK_TYPE_UNDEFINED: stype ="undefined"; break;
			case DUK_TYPE_BOOLEAN: stype ="boolean"; break;
			case DUK_TYPE_NULL: stype ="null"; break;
			case DUK_TYPE_POINTER: stype ="pointer"; break;
			default:
				stype = "unknown";
		}
		if(duk_is_function(ctx,ipos)){
			char *afunc = "";
			afunc = duk_is_c_function(ctx,ipos) ? "Cfunc" : afunc;
			afunc = duk_is_ecmascript_function(ctx,ipos) ? "jsfunc" : afunc;
			afunc = duk_is_bound_function(ctx,ipos) ? "boundfunc" : afunc;
			amore = afunc;
		}
		if(duk_is_nan(ctx,ipos)){
			amore = "NaN";
		}
		if(duk_is_object(ctx,ipos)){

		}
		printf("%10d%10s   %s\n",ipos,stype,amore);
	}
}
void show_stack(duk_context *ctx, char* comment)
{
	show_stack0(ctx, comment, 1);
}



//OBJECT VIRTUALIZATION / PROXY HELPERS: constructor, handler (has,get,set,deleteProp)

static struct {
	int nkey;
	char *key[100];
	char *val[100];
	char *arr[100];
} nativeStruct = {0};

void nativeSetS(const char *key, const char *val)
{
	//named property
	int kval = -1;
	for(int j=0;j<nativeStruct.nkey;j++)
		if(!strcmp(nativeStruct.key[j],key))	kval = j;
	if(kval < 0) {
		kval = nativeStruct.nkey;
		nativeStruct.key[kval] = strdup(key);
		nativeStruct.nkey++;
	}
	nativeStruct.val[kval] = strdup(val);
}
static char* nativeValue = NULL;




//struct Node_Scene {
//       int _nodeType; /* unique integer for each type */ 
//};
//struct Node_ExecutionContext {
//       int _nodeType; /* unique integer for each type */ 
//};
struct string_int{
	char *c;
	int i;
};

struct string_int lookup_fieldType[] = {
	{"Float", FIELDTYPE_SFFloat},
	{"Rotation", FIELDTYPE_SFRotation},
	{"Vec3f", FIELDTYPE_SFVec3f},
	{"Bool", FIELDTYPE_SFBool},
	{"Int32", FIELDTYPE_SFInt32},
	{"Node", FIELDTYPE_SFNode},
	{"Color", FIELDTYPE_SFColor},
	{"ColorRGBA", FIELDTYPE_SFColorRGBA},
	{"Time", FIELDTYPE_SFTime},
	{"String", FIELDTYPE_SFString},
	{"Vec2f", FIELDTYPE_SFVec2f},
	{"Image", FIELDTYPE_SFImage},
	{"Vec3d", FIELDTYPE_SFVec3d},
	{"Double", FIELDTYPE_SFDouble},
	{"Matrix3f", FIELDTYPE_SFMatrix3f},
	{"Matrix3d", FIELDTYPE_SFMatrix3d},
	{"Matrix4f", FIELDTYPE_SFMatrix4f},
	{"Matrix4d", FIELDTYPE_SFMatrix4d},
	{"Vec2d", FIELDTYPE_SFVec2d},
	{"Vec4f", FIELDTYPE_SFVec4f},
	{"Vec4d", FIELDTYPE_SFVec4d},
	{NULL,0}
};
//const char *stringFieldtypeType (int st); //in generatedcode
//const char *stringNodeType (int st);
int fwType2itype(const char *fwType){
	int isSF, isMF, ifield = -1;
	const char *suffix;
	isSF = !strncmp(fwType,"SF",2);
	isMF = !strncmp(fwType,"MF",2);
	if(isSF || isMF){
		suffix = &fwType[2]; //skip SF/MF part
		int i = 0;
		while(lookup_fieldType[i].c){
			if(!strcmp(suffix,lookup_fieldType[i].c)){
				ifield = lookup_fieldType[i].i;
				break;
			}
			i++;
		}
		if(ifield > -1 && isMF ) ifield++;
	}else{
		//browser and scene/executionContext shouldn't be going through cfwconstructor
		if(!strcmp(fwType,"Browser")) ifield = AUXTYPE_X3DBrowser;
		if(!strcmp(fwType,"X3DConstants")) ifield = AUXTYPE_X3DConstants;
	}
	return ifield;
}
void push_typed_proxy(duk_context *ctx, const char *fwType, int itype, void *fwpointer, int* valueChanged)
{
	/*  called by both the cfwconstructor (for new Proxy) and fwgetter (for referenced script->fields)
		1. please have the proxy object on the stack before calling
		   cfwconstructor: push_this (from the 'new')
		   fwgetter: push_object (fresh object)
		2. nativePtr
			cfwconstructor: malloc/construct a new field, set the values and give the pointer
			fwgetter: reference to script->field[i]
	*/
	int rc;

	//add fwtype to this
	//- if I have one C constructor, and many named js constructors
	//- I need to fetch the name of the constructor and use it here
	duk_push_string(ctx,fwType); //"SFColor");
	//show_stack(ctx,"just before putting prop string fwtype");
	duk_put_prop_string(ctx,-2,"fwType");
	//show_stack(ctx,"just after putting prop string fwtype");
	//add native pointer to this
	duk_push_pointer(ctx,fwpointer);
	duk_put_prop_string(ctx,-2,"fwField");
	duk_push_pointer(ctx,valueChanged);
	duk_put_prop_string(ctx,-2,"fwChanged");
	duk_push_int(ctx,itype);
	duk_put_prop_string(ctx,-2,"fwItype");


	duk_pop(ctx); //pop this

	duk_push_global_object(ctx); //could I just push an object, or push nothing? then how to get global->handler?
	int iglobal = duk_get_top(ctx) -1;
	rc = duk_get_prop_string(ctx,iglobal,"Proxy");
	//rc = duk_get_prop_string(ctx,iglobal,"this");
	duk_push_this(ctx);
	rc = duk_get_prop_string(ctx,iglobal,"handler");
	duk_new(ctx,2); /* [ global Proxy target handler ] -> [ global result ] */
	duk_remove(ctx,-2); //remove global so just proxy on stack
}

#include <math.h> //for int = round(numeric)
int cfwconstructor(duk_context *ctx) {
	int rc, nargs;
	const char *fwType = NULL;
	int *valueChanged = NULL; //so called 'internal' variables inside the script context don't point to a valueChanged
	int itype = 0;
	nargs = duk_get_top(ctx);

	duk_push_current_function(ctx);
	rc = duk_get_prop_string(ctx, -1, "fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	if(rc == 1) itype = fwType2itype(fwType);
	if(rc == 1) printf("in constructor fwType=%s\n",fwType);
	duk_pop(ctx);

	//some things we don't allow scene authors to construct in their javascript
	if(!fwType) return 0; //not one of our types
	if(!strcmp(fwType,"X3DBrowser") || !strcmp(fwType,"X3DConstants")) return 0; //this are static singletons
	if(!strcmp(fwType,"X3DScene") || !strcmp(fwType,"X3DExecutionContext")) return 0; //Browser creates these internally


	show_stack(ctx,"in C constructor before push this");
	duk_push_this(ctx);
	//add native pointer to this
	void *fwpointer = malloc(sizeof(union anyVrml));

	if(0){
		show_stack(ctx,"in C constructor");

		//add fwtype to this
		//- if I have one C constructor, and many named js constructors
		//- I need to fetch the name of the constructor and use it here
		duk_push_string(ctx,fwType); //"SFColor");
		duk_put_prop_string(ctx,-2,"fwType");
		duk_push_pointer(ctx,fwpointer);
		duk_put_prop_string(ctx,-2,"fwPointer");
		duk_pop(ctx); //pop this

		duk_push_global_object(ctx); //could I just push an object, or push nothing? then how to get global->handler?
		if(1){
			int iglobal = duk_get_top(ctx) -1;
			rc = duk_get_prop_string(ctx,iglobal,"Proxy");
			//rc = duk_get_prop_string(ctx,iglobal,"this");
				duk_push_this(ctx);
			rc = duk_get_prop_string(ctx,iglobal,"handler");
			duk_new(ctx,2); /* [ global Proxy target handler ] -> [ global result ] */
			//duk_remove(ctx, -2); /* remove global -> [ result ] */
			if(0){
				duk_put_prop_string(ctx,iglobal,"proxy");
				duk_get_prop_string(ctx,-1,"proxy"); //get it from global
			}
		}else{
			duk_eval_string(ctx,"var proxy = new Proxy(this,handler);");
			duk_pop(ctx); //pop eval results
			duk_get_prop_string(ctx,-1,"proxy"); //get it from global
		}
		duk_remove(ctx,-2); //remove global so just proxy on stack
	}else{
		push_typed_proxy(ctx, fwType, itype, fwpointer, valueChanged);
	}
	//put return value -a proxy object- on stack for return
	//get the args passed to the constructor, and call the proxy properties with them
	//or call a native constructor for the type and pass in args
	//Q. in FWTypes, did I have an fw_vargs type?
	if(nargs == 3){
		const char *val;
		//here we go directly to my code - works
		val = duk_to_string(ctx,0);
		nativeSetS("x", val);
		val = duk_to_string(ctx,1);
		nativeSetS("y", val);
		val = duk_to_string(ctx,2);
		nativeSetS("z", val);
	}
	//for(int i=0;i<nargs;i++){
	//}
	return 1;
}
int chas(duk_context *ctx) {
	int rc, itype, *valueChanged;
	const char *fwType = NULL;
	union anyVrml *parent;

	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,0,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	//if(fwType) printf("fwType in cget=%s\n",fwType);
	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,0,"fwField");
	if(rc == 1) parent = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the changed flag */
	rc = duk_get_prop_string(ctx,0,"fwChanged");
	if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
	duk_pop(ctx);

	if(fwType) printf("fwType in chas=%s\n",fwType);

	show_stack(ctx,"in chas");
	duk_push_string(ctx, nativeValue);
    return 1;
}

int push_duk_fieldvalueECMA(duk_context *ctx, int itype, union anyVrml *fieldvalue)
{
	/*we have the field, and even the key name. 
	  So we should be able to decide how to package the outgoing value type:
	  according to specs:
	  - return ecma primitive value type for SFBool, SFInt32, SFFloat, SFDouble, SFTime, SFString
	  - return our field-type-specific object/proxy-wrapper, pointing to our global.field, for the others.
	*/
	int nr;
	int isOK = FALSE;
	nr = 1;
	switch(itype){
    case FIELDTYPE_SFBool:
		duk_push_boolean(ctx,fieldvalue->sfbool); break;
    case FIELDTYPE_SFFloat:
		duk_push_number(ctx,fieldvalue->sffloat); break;
    case FIELDTYPE_SFTime:
		duk_push_number(ctx,fieldvalue->sftime); break;
    case FIELDTYPE_SFDouble:
		duk_push_number(ctx,fieldvalue->sfdouble); break;
    case FIELDTYPE_SFInt32:
		duk_push_int(ctx,fieldvalue->sfint32); break;
    case FIELDTYPE_SFString:
		duk_push_string(ctx,fieldvalue->sfstring->strptr); break;
	default:
		nr = 0; 
		break;
	}
	//show_stack(ctx,"in fwgetterNS at end");
    return nr;
}
int push_duk_fieldvalueObject(duk_context *ctx, int itype, union anyVrml *field, int *valueChanged ){
	//we need an object with our c handlers and pointer to our script->field[i]
	push_typed_proxy(ctx, FIELDTYPES[itype], itype, field, valueChanged);
	return 1;
}
int isECMAtype(int itype){
	int isEcma;
	switch(itype){
    case FIELDTYPE_SFBool:
    case FIELDTYPE_SFFloat:
    case FIELDTYPE_SFTime:
    case FIELDTYPE_SFDouble:
    case FIELDTYPE_SFInt32:
    case FIELDTYPE_SFString:
		isEcma = TRUE;
	default:
		isEcma = FALSE;
	}
	return isEcma;
}
int mf2sf(int itype){
	//return convertToSFType(itype); //this is more reliable but bulky
	return itype -1;
}
/*we seem to be missing something in generated code/structs that would allow me to
  look up how big something is. I suspect it's ##MACRO-ized elsewhere.
*/
int sizeofSF(int itype){
	//goal get the offset for MF.p[i] in bytes
	int iz;
	switch(itype){
	case FIELDTYPE_SFFloat: iz = sizeof(float); break;
	case FIELDTYPE_SFRotation:	iz = sizeof(struct SFRotation); break;
	case FIELDTYPE_SFVec3f:	iz = 3*sizeof(struct SFVec3f);break;
	case FIELDTYPE_SFBool:	iz = sizeof(int); break;
	case FIELDTYPE_SFInt32:	iz = sizeof(int); break;
	case FIELDTYPE_SFNode:	iz = sizeof(void*); break;
	case FIELDTYPE_SFColor:	iz = sizeof(struct SFColor); break;
	case FIELDTYPE_SFColorRGBA:	iz = sizeof(struct SFColorRGBA); break;
	case FIELDTYPE_SFTime:	iz = sizeof(double); break;
	case FIELDTYPE_SFString: iz = sizeof(struct Uni_string *); break;
	case FIELDTYPE_SFVec2f:	iz = sizeof(struct SFVec2f); break;
	//case FIELDTYPE_SFImage:	iz = 
	case FIELDTYPE_SFVec3d:	iz = sizeof(struct SFVec3d); break;
	case FIELDTYPE_SFDouble: iz = sizeof(double); break;
	case FIELDTYPE_SFMatrix3f: iz = sizeof(struct SFMatrix3f); break;
	case FIELDTYPE_SFMatrix3d: iz = sizeof(struct SFMatrix3d); break;
	case FIELDTYPE_SFMatrix4f: iz = sizeof(struct SFMatrix4f); break;
	case FIELDTYPE_SFMatrix4d: iz = sizeof(struct SFMatrix4d); break;
	case FIELDTYPE_SFVec2d: iz = sizeof(struct SFVec2d); break;
	case FIELDTYPE_SFVec4f:	iz = sizeof(struct SFVec4f); break;
	case FIELDTYPE_SFVec4d:	iz = sizeof(struct SFVec4d); break;
	default:
		//ouch
		iz = sizeof(void*);
	}
	return iz;
}
int Browser_getSomething(double *dval,double A, double B, double C){
	*dval = A + B + C;
	return 1;
}

int fwval_duk_push(duk_context *ctx, FWval fwretval){
	//converts engine-agnostic FWVAL return value to duk engine specific return values and pushes them onto the duk value stack
	int nr = 1;
	switch(fwretval->itype){
	case 'B':
		duk_push_boolean(ctx,fwretval->_boolean); break;
	case 'I':
		duk_push_int(ctx,fwretval->_integer); break;
	case 'N':
		duk_push_number(ctx,fwretval->_numeric); break;
	case 'S':
		duk_push_string(ctx,fwretval->_string); break;
	case 'W':
		duk_push_pointer(ctx,fwretval->_web3dval.native);
		duk_push_int(ctx,fwretval->_web3dval.fieldType);
		duk_put_prop_string(ctx,-2,"fwItype");
		break;
	case '0':
	default:
		nr = 0; break;
	}
	return nr;
}

int cfunction(duk_context *ctx) {
	int rc, nr, itype, *valueChanged;
	//show_stack(ctx,"in cget");
	const char *fwType = NULL;
	const char *fwFunc = NULL;
	union anyVrml* parent = NULL;
	union anyVrml* field = NULL;
	FWTYPE *fwt;
	FWFunctionSpec *fs;

	int nargs = duk_get_top(ctx);
	show_stack0(ctx,"in cfuction",0);
	duk_push_current_function(ctx);
	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,-1,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,-1,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,-1,"fwField");
	if(rc == 1) parent = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the changed flag */
	rc = duk_get_prop_string(ctx,-1,"fwChanged");
	if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the name of the function called */
	rc = duk_get_prop_string(ctx,-1,"fwFunc");
	if(rc == 1) fwFunc = duk_to_string(ctx,-1);
	duk_pop(ctx);
	duk_pop(ctx); //durrent function

	printf("fwFunc=%s, fwType=%s\n",fwFunc,fwType);
	nr = 0;
	int i;
	fwt = getFWTYPE(itype);
	//check functions - if its a function push the type's specfic function
	fs = getFWFunc(fwt,fwFunc);
	if(fs){
		FWVAL fwretval;
		int nUsable,nNeeded;
		nUsable = fs->arglist.iVarArgStartsAt > -1 ? nargs : fs->arglist.nfixedArg;
		nNeeded = max(nUsable,fs->arglist.nfixedArg);
		FWval pars = malloc(nNeeded*sizeof(FWVAL));
		//QC and genericization of incoming parameters
		for(i=0;i<nUsable;i++){
			char ctype;
			if(i < fs->arglist.nfixedArg) 
				ctype = fs->arglist.argtypes[i];
			else 
				ctype = fs->arglist.argtypes[fs->arglist.iVarArgStartsAt];
			pars[i].itype = ctype;
			switch(ctype){
			case 'B': pars[i]._boolean = duk_get_boolean(ctx,i); break;
			case 'I': pars[i]._integer = duk_get_int(ctx,i); break;
			case 'N': pars[i]._numeric = duk_get_number(ctx,i); break;
			case 'S': pars[i]._string = duk_get_string(ctx,i); break;
			case 'F': //flexi-string idea - allow either String or MFString (no such thing as SFString from ecma - it uses String for that)
				if(duk_is_pointer(ctx,i)){
					void *ptr = duk_get_pointer(ctx,i); 
					pars[i]._web3dval.native = ptr;
					pars[i]._web3dval.fieldType = FIELDTYPE_MFString; //type of the incoming arg[i]
					pars[i].itype = 'W';
				}else if(duk_is_string(ctx,i)){
					pars[i]._string = duk_get_string(ctx,i); 
					pars[i].itype = 'S';
				}
				break; 
			case 'W': {
				void *ptr = duk_get_pointer(ctx,i); 
				pars[i]._web3dval.native = ptr;
				pars[i]._web3dval.fieldType = FIELDTYPE_SFNode; //type of the incoming arg[i]
				}
				break;
			case 'O': break; //object pointer ie to js function callback object
			}
		}
		
		for(i=nUsable;i<nNeeded;i++){
			//fill
			char ctype = fs->arglist.argtypes[i];
			pars[i].itype = ctype;
			switch(ctype){
			case 'B': pars[i]._boolean = FALSE; break;
			case 'I': pars[i]._integer = 0; break;
			case 'N': pars[i]._numeric = 0.0; break;
			case 'S': pars[i]._string = NULL; break;
			case 'F': pars[i]._string = NULL; pars[i].itype = 'S'; break;
			case 'W': pars[i]._web3dval.fieldType = FIELDTYPE_SFNode; pars[i]._web3dval.native = NULL; break;
			case 'O': pars[i]._jsobject = NULL; break; 
			}
		}
		//the object function call, using engine-agnostic parameters
		nr = fs->call(fwt,parent,nNeeded,pars,&fwretval);
		if(nr){
			nr = fwval_duk_push(ctx,&fwretval);
		}
		free(pars);
	}
	return nr;
}
int cget(duk_context *ctx) {
	int rc, nr, itype, *valueChanged;
	//show_stack(ctx,"in cget");
	const char *fwType = NULL;
	union anyVrml* parent = NULL;
	union anyVrml* field = NULL;

	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,0,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,0,"fwField");
	if(rc == 1) parent = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the changed flag */
	rc = duk_get_prop_string(ctx,0,"fwChanged");
	if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	show_stack0(ctx,"in cget",0);

	nr = 0;
	if(!fwType ) return nr;
	if(itype < 1000){
		//itype is in FIELDTYPE_ range
		/* figure out what field on the parent the get is referring to */
		//if(!strncmp(fwType,"MF",2) || !strncmp(fwType,"SF",2)){
		if(!parent) return nr;
		if(duk_is_number(ctx,-2)){
			//indexer
			int ikey = duk_get_int(ctx,-2);
			//int ikey = round(key);
			if(!strncmp(fwType,"MF",2)){
				//its an MF field type, and we have an index to it.
				if(ikey < parent->mfbool.n && ikey > -1){
					// valid index range - figure out what type the SF is and return the element
					int isize;
					int iSFtype;
					//convert the parent's MF type to equivalent SF type for element
					iSFtype = mf2sf(itype);
					isize = sizeofSF(iSFtype);
					if(isECMAtype(iSFtype)){
						union anyVrml *fieldvalue;
						fieldvalue = (union anyVrml*)((char*)(parent->mfbool.p)+ ikey*isize);
						nr = push_duk_fieldvalueECMA(ctx, iSFtype, fieldvalue);
					}else{
						field = (union anyVrml*)&parent->mfbool.p[ikey];
						nr = push_duk_fieldvalueObject(ctx, iSFtype, field, valueChanged);
					}
				}
			}
		}else{
			//named property
			int kval = -1;
			char *val = NULL;
			const char *key = duk_require_string(ctx,-2);
			printf("key=%s\n",key);

			for(int j=0;j<nativeStruct.nkey;j++)
				if(!strcmp(nativeStruct.key[j],key)) kval = j;
			if(kval > -1) val = nativeStruct.val[kval];
			else val = "None";
			duk_push_string(ctx,val);
		}
	}else{ //itype < 1000
		//itype is in AUXTYPE_ range
		FWTYPE *fwt = getFWTYPE(itype);
		const char *key = duk_require_string(ctx,-2);
		printf("key=%s\n",key);
		//check numeric indexer
		if(duk_is_number(ctx,-2)){
			//indexer
			int ikey = duk_get_int(ctx,-2);
			if(fwt->takesIndexer){
				FWVAL fwretval;
				nr = fwt->Getter(ikey,parent,&fwretval);
				if(nr){
					nr = fwval_duk_push(ctx,&fwretval);
				}
			}
		}
		//check functions - if its a function push the type's specfic function
		FWFunctionSpec *fw = getFWFunc(fwt,key);
		if(fw){
			//its a function
			duk_push_c_function(ctx,cfunction,DUK_VARARGS);
			duk_push_pointer(ctx,parent);
			duk_put_prop_string(ctx,-2,"fwField");
			duk_push_pointer(ctx,valueChanged);
			duk_put_prop_string(ctx,-2,"fwChanged");
			duk_push_int(ctx,itype);
			duk_put_prop_string(ctx,-2,"fwItype");
			duk_push_string(ctx,fwType);
			duk_put_prop_string(ctx,-2,"fwType");
			duk_push_string(ctx,key);
			duk_put_prop_string(ctx,-2,"fwFunc");
			nr = 1;
		}else{
			//check properties - if a property, call the type-specific getter
			if(fwt->Properties){
				int index;
				FWPropertySpec *ps = getFWProp(fwt,key,&index);
				if(ps){
					FWVAL fwretval;
					nr = fwt->Getter(index,parent,&fwretval);
					if(nr){
						nr = fwval_duk_push(ctx,&fwretval);
					}
				}
			}
		}
	}
    return nr;
}
int cset(duk_context *ctx) {
	int rc, itype, *valueChanged;
	const char *fwType = NULL;
	union anyVrml *parent;

	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,0,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,0,"fwField");
	if(rc == 1) parent = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the changed flag */
	rc = duk_get_prop_string(ctx,0,"fwChanged");
	if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
	duk_pop(ctx);


	show_stack0(ctx,"in cset",0);
	//char *val = duk_require_string(ctx,-2);
	const char *val = duk_to_string(ctx,-2);
	const char *key = duk_require_string(ctx,-3);
	printf("key=%s val=%s\n",key,val);

	if(itype < 1000){
		if(duk_is_number(ctx,-3)){
			//indexer
			double key = duk_require_number(ctx,-3);
			int ikey = round(key);
			printf("index =%d\n",ikey);
			nativeStruct.arr[ikey] = strdup(val);
		}else{
			//named property
			int kval = -1;
			const char *key = duk_require_string(ctx,-3);
			for(int j=0;j<nativeStruct.nkey;j++)
				if(!strcmp(nativeStruct.key[j],key))	kval = j;
			if(kval < 0) {
				kval = nativeStruct.nkey;
				nativeStruct.key[kval] = strdup(key);
				nativeStruct.nkey++;
			}
			nativeStruct.val[kval] = strdup(val);

			printf("key =%s\n",key);
		}
	}else{
		//itype is in AUXTYPE_ range
		FWTYPE *fwt = getFWTYPE(itype);
		//check numeric indexer
		if(duk_is_number(ctx,-2)){
			//indexer
			int ikey = duk_get_int(ctx,-2);
			if(fwt->takesIndexer){
				FWVAL fwsetval;
				fwsetval.itype = fwt->takesIndexer;
				switch(fwt->takesIndexer){
					case '0':break;
					case 'I': fwsetval._integer = duk_get_int(ctx,-2); break;
					case 'N': fwsetval._numeric = duk_get_number(ctx,-2); break;
					case 'B': fwsetval._boolean = duk_get_boolean(ctx,-2); break;
					case 'S': fwsetval._string = duk_get_string(ctx,-2); break;
					case 'W': fwsetval._web3dval.native = duk_get_pointer(ctx,-2); fwsetval._web3dval.fieldType = itype; break;
					case 'P': fwsetval._pointer.native = duk_get_pointer(ctx,-2); fwsetval._pointer.fieldType = itype; break;
				}
				fwt->Setter(ikey,parent,&fwsetval);
			}
		}else{
			//check properties - if a property, call the type-specific getter
			if(fwt->Properties){
				int index;
				FWPropertySpec *ps = getFWProp(fwt,key,&index);
				if(ps){
					FWVAL fwsetval;
					fwsetval.itype = ps->type;
					switch(ps->type){
					case '0':break;
					case 'I': fwsetval._integer = duk_get_int(ctx,-2); break;
					case 'N': fwsetval._numeric = duk_get_number(ctx,-2); break;
					case 'B': fwsetval._boolean = duk_get_boolean(ctx,-2); break;
					case 'S': fwsetval._string = duk_get_string(ctx,-2); break;
					case 'W': fwsetval._web3dval.native = duk_get_pointer(ctx,-2); fwsetval._web3dval.fieldType = itype; break;
					case 'P': fwsetval._pointer.native = duk_get_pointer(ctx,-2); fwsetval._pointer.fieldType = itype; break;
					}
					fwt->Setter(index,parent,&fwsetval);
				}
			}
		}
	}
    return 0;
}
int cdel(duk_context *ctx) {
	int rc, itype, *valueChanged;
	const char *fwType = NULL;
	union anyVrml *parent;

	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,0,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	//if(fwType) printf("fwType in cget=%s\n",fwType);
	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,0,"fwField");
	if(rc == 1) parent = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the changed flag */
	rc = duk_get_prop_string(ctx,0,"fwChanged");
	if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
	duk_pop(ctx);

	if(fwType) printf("fwType in cdel=%s\n",fwType);
	show_stack0(ctx,"in cdel",0);
	duk_push_string(ctx, nativeValue);
    return 1;
}

//c-side helper adds the generic handler to global, for use when creating each proxy
void addHandler(duk_context *ctx){
	int iglobal, ihandler, rc;
	iglobal = duk_get_top(ctx) -1;

	duk_push_object(ctx);
	duk_put_prop_string(ctx, iglobal, "handler");

	duk_get_prop_string(ctx,iglobal,"handler"); //get handler from global
	if(1) ihandler = duk_get_top(ctx) -1; //+ve
	else ihandler = -2; //-ve
	duk_push_c_function(ctx,chas,2);
	rc = duk_put_prop_string(ctx, ihandler, "has");
	printf("rc=%d",rc);
	duk_push_c_function(ctx,cget,3);
	duk_put_prop_string(ctx, ihandler, "get");
	duk_push_c_function(ctx,cset,4);
	duk_put_prop_string(ctx, ihandler, "set");
	duk_push_c_function(ctx,cdel,2);
	duk_put_prop_string(ctx, ihandler, "del");
	duk_pop(ctx); //pop handler off stack

}
void addCustomProxyType(duk_context *ctx, int iglobal, const char *typeName)
{
	duk_push_c_function(ctx,cfwconstructor,DUK_VARARGS);
	//put fname=SFVec3f on c_function, so in constructor we can tell what we are trying to construct
	duk_push_string(ctx,typeName);
	duk_put_prop_string(ctx,-2,"fwType");
	//put SFVec3f = c_fuction on global
	duk_put_prop_string(ctx,iglobal,typeName);
}
//void add_duk_global_property(duk_context *ctx, int iglobal, const char *fieldname, int itype, int mode, const char *ctype, void *fieldptr /*anyVrml*/, int *valueChanged);
void add_duk_global_property(duk_context *ctx, int iglobal, int itype, const char *fieldname, const char *ctype, void *fieldptr /*anyVrml*/, int *valueChanged, struct X3D_Node *node, int ifield );

static char *DefaultScriptMethodsA = "function initialize() {}; " \
			" function shutdown() {}; " \
			" function eventsProcessed() {}; " \
			" TRUE=true; FALSE=false; " \
			"";


static char *DefaultScriptMethodsB = " function print(x) {Browser.print(x)}; " \
			" function println(x) {Browser.println(x)}; " \
			" function getName() {return Browser.getName()}; "\
			" function getVersion() {return Browser.getVersion()}; "\
			" function getCurrentSpeed() {return Browser.getCurrentSpeed()}; "\
			" function getCurrentFrameRate() {return Browser.getCurrentFrameRate()}; "\
			" function getWorldURL() {return Browser.getWorldURL()}; "\
			" function replaceWorld(x) {Browser.replaceWorld(x)}; "\
			" function loadURL(x,y) {Browser.loadURL(x,y)}; "\
			" function setDescription(x) {Browser.setDescription(x)}; "\
			" function createVrmlFromString(x) {Browser.createVrmlFromString(x)}; "\
			" function createVrmlFromURL(x,y,z) {Browser.createVrmlFromURL(x,y,z)}; "\
			" function createX3DFromString(x) {Browser.createX3DFromString(x)}; "\
			" function createX3DFromURL(x,y,z) {Browser.createX3DFromURL(x,y,z)}; "\
			" function addRoute(a,b,c,d) {Browser.addRoute(a,b,c,d)}; "\
			" function deleteRoute(a,b,c,d) {Browser.deleteRoute(a,b,c,d)}; "
			"";

/*add x3d v3.3 ecmascript X3DConstants table 
// http://www.web3d.org/files/specifications/19777-1/V3.0/index.html
// http://www.web3d.org/files/specifications/19777-1/V3.0/Part1/functions.html
// 7.9.11
*/


/* www.duktape.org javascript engine used here */
//A DUK helper function
static char *eval_string_defineAccessor = "\
function defineAccessor(obj, key, set, get) { \
    Object.defineProperty(obj, key, { \
        enumerable: true, configurable: true, \
        set: set, get: get \
    }); \
}";


//void JSCreateScriptContext(int num){return;}
/* create the script context for this script. This is called from the thread
   that handles script calling in the fwl_RenderSceneUpdateScene */
void JSCreateScriptContext(int num) {
	int iglobal, rc;
	//jsval rval;
	duk_context *ctx; 	/* these are set here */
	struct Shader_Script *script;
	//JSObject *_globalObj; 	/* these are set here */
	//BrowserNative *br; 	/* these are set here */
	ppJScript p = (ppJScript)gglobal()->JScript.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();
	script = ScriptControl[num].script;

	//CREATE CONTEXT
	//_context = JS_NewContext(p->runtime, STACK_CHUNK_SIZE);
	//if (!_context) freewrlDie("JS_NewContext failed");
	//JS_SetErrorReporter(_context, errorReporter);
	ctx = duk_create_heap_default();

	//ADD STANDARD JS GLOBAL OBJECT/CLASSES
	//_globalObj = JS_NewObject(_context, &p->globalClass, NULL, NULL);
	/* gets JS standard classes */
	//if (!JS_InitStandardClasses(_context, _globalObj))
	//	freewrlDie("JS_InitStandardClasses failed");
    duk_push_global_object(ctx);
	iglobal = duk_get_top(ctx) -1;

	//SAVE OUR CONTEXT IN OUR PROGRAM'S SCRIPT NODE FOR LATER RE-USE
	//ScriptControl[num].cx =  _context;
	//ScriptControl[num].glob =  _globalObj;
	ScriptControl[num].cx =  ctx;
	ScriptControl[num].glob =  (void *)malloc(sizeof(int)); 
	*((int *)ScriptControl[num].glob) = iglobal; //we'll be careful not to pop our global for this context (till context cleanup)

	//ADD DUK HELPER PROPS AND FUNCTIONS
	duk_push_pointer(ctx,script);
	duk_put_prop_string(ctx,iglobal,"__script");

	duk_push_string(ctx,eval_string_defineAccessor);
	duk_eval(ctx);
	//printf("result is: %s\n", duk_get_string(ctx, -1));
	duk_pop(ctx);

	//ADD CUSTOM TYPES
	//* VRML Browser
	//br = (BrowserNative *) JS_malloc(_context, sizeof(BrowserNative));
	//if (!VrmlBrowserInit(_context, _globalObj, br))
	//	freewrlDie("VrmlBrowserInit failed");

	//* VRML field types - SF and MF
	//if (!loadVrmlClasses(_context, _globalObj))
	//	freewrlDie("loadVrmlClasses failed");

	addHandler(ctx); //add helper called handler, to global object
	//from GeneratedCode.c const char *FIELDTYPES[]; const int FIELDTYPES_COUNT;
	for(int i=0;i<FIELDTYPES_COUNT;i++)
		addCustomProxyType(ctx, iglobal, FIELDTYPES[i]); //adds proxy constructor function (called typeName in js), and proxy handlers
	show_stack(ctx,"before adding Browser");
	add_duk_global_property(ctx, iglobal, AUXTYPE_X3DBrowser, "Browser", "X3DBrowser", NULL, NULL,NULL,0);
	//add_duk_global_property(ctx, iglobal, AUXTYPE_X3DBrowser, "Browser", "X3DBrowser", p->Instance->Browser, NULL,(struct X3D_Node*)p->Instance,2);
	//addCustomProxyType(ctx, iglobal, "Browser"); 
	//add x3d X3DConstants table 
	//addCustomProxyType(ctx,iglobal,"X3DConstants");
	add_duk_global_property(ctx, iglobal,AUXTYPE_X3DConstants,"X3DConstants", "X3DConstants", NULL, NULL, NULL,0);
	//add_duk_global_property(ctx, iglobal,AUXTYPE_X3DConstants,"X3DConstants", "X3DConstants", p->Instance->X3DConstants, NULL, (struct X3D_Node*) p->Instance,3);


	//test
	if(0){
	duk_eval_string(ctx,"Browser.description = 'funny description happened on the way to ..';");
	duk_pop(ctx);
	duk_eval_string(ctx,"print('hi from print');");
	duk_pop(ctx);
	duk_eval_string(ctx,"print(Browser.version);");
	duk_pop(ctx);
	duk_eval_string(ctx,"Browser.println('hi from brwsr.println');");
	duk_pop(ctx);
	duk_eval_string(ctx,"Browser.println(Browser.description);");
	duk_pop(ctx);

	}


	if(0){
	duk_eval_string(ctx,"var myvec3 = new SFVec3f(1.0,2.0,3.0);");
	duk_pop(ctx);
	duk_eval_string(ctx,"print(myvec3.x.toString());");
	duk_pop(ctx);
	duk_eval_string(ctx,"myvec3.y = 45.0;");
	duk_pop(ctx);
	duk_eval_string(ctx,"print('sb45='+myvec3.y);");
	duk_pop(ctx);
	}


	//* Global methods and defines (some redirecting to the Browser object ie print = Browser.println)
	//if (!ACTUALRUNSCRIPT(num,DefaultScriptMethods,&rval))
	//	cleanupDie(num,"runScript failed in VRML::newJS DefaultScriptMethods");
	if(1){
		/* I need these working, but print = Browser.print isn't hooked up, and bombs later*/
		show_stack(ctx,"\nbefore eval DefaultScriptMethods");
		duk_eval_string(ctx,DefaultScriptMethodsA);
		duk_pop(ctx);
	}
	show_stack(ctx,"done initializeContext - should be 1 object (global)");
	//duk_eval_string(ctx,"print('hi there');"); duk_pop(ctx);


	/* send this data over to the routing table functions. */
	CRoutes_js_new (num, JAVASCRIPT);
	return;
}
int get_valueChanged_flag (int fptr, int actualscript){
	struct Shader_Script *script;
	struct ScriptFieldDecl *field;
	struct CRscriptStruct *scriptcontrol, *ScriptControlArr = getScriptControl();
	scriptcontrol = &ScriptControlArr[actualscript];
	script = scriptcontrol->script;
	field = Shader_Script_getScriptField(script, fptr);
	return field->valueChanged;
	//printf("in get_valueChanged_flag\n");
}
void resetScriptTouchedFlag(int actualscript, int fptr){
	struct Shader_Script *script;
	struct ScriptFieldDecl *field;
	struct CRscriptStruct *scriptcontrol, *ScriptControlArr = getScriptControl();
	scriptcontrol = &ScriptControlArr[actualscript];
	script = scriptcontrol->script;
	field = Shader_Script_getScriptField(script, fptr);
	field->valueChanged = 0;
	//printf("in get_valueChanged_flag\n");
	return;
}


/* fwsetterNS, fwgetterNS are for our Script node dynamic fields, or
   more precisely, for the javascript property we create on the js>context>global object,
   one global>property for each script field, with the name of the script field on the property
*/

int fwsetterNS(duk_context *ctx) {
	/* myfield = new SFVec3f(1,2,3); 
	 * if myfield is a property we set on the global object, and we've assigned this setter to it,
	 * we'll come in here. We can set the *valueChanged and value on the LHS object, by copying, as per specs
	 * terminology: LHS: left hand side of equation (ie myfield) RHS: right hand side of equation (ie result of new SFVec3f() )
	 */
	int nargs, nr;
	int rc, itype, *valueChanged;
	const char *fwType = NULL;
	union anyVrml *field;
	nargs = duk_get_top(ctx);

	/* retrieve key from nonstandard arg */
	//show_stack(ctx,"in fwsetterNS");
   // nativeValue = duk_require_string(ctx, 0);
    //implicit key by setter C function //char *key = duk_require_string(ctx, 1);
	const char *key = duk_require_string(ctx,1); //"myprop";
	//printf("\nfwsetterNS, key=%s value=%s\n",key,nativeValue);

	/* get details of LHS object */
	/* retrieve field pointer from Cfunc */
	duk_push_current_function(ctx);
	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,-1,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,-1,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,-1,"fwField");
	if(rc == 1) field = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the changed flag */
	rc = duk_get_prop_string(ctx,-1,"fwChanged");
	if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	duk_pop(ctx); //pop current function

	/*we have the field, and even the key name. 
	  So we should be able to decide how to deal with the incoming set value type 
	  according to specs:
	  - convert incoming ecma primitive value type for SFBool, SFInt32, SFFloat, SFDouble, SFTime, SFString
	  - if it's one of our field-type-specific object/proxy-wrapper, copy the field values
	  - if it's something else, return error, unknown conversion
	  if succesful set valueChanged
	*/
	int isOK = FALSE;
	double val;
	int ival;
	const char* sval;
	/* get details of RHS ... then copy by value to LHS */
	int RHS_duk_type = duk_get_type(ctx, 0);
	switch(RHS_duk_type){
	case DUK_TYPE_NUMBER: 
		val = duk_require_number(ctx,0);
		isOK = TRUE;
		switch(itype){
			case FIELDTYPE_SFFloat:
				field->sffloat = val; break;
			case FIELDTYPE_SFTime:
				field->sftime = val; break;
			case FIELDTYPE_SFDouble:
				field->sfdouble = val; break;
			case FIELDTYPE_SFInt32:
				field->sfint32 = round(val); break;
			default:
				isOK = FALSE;
		}
		break;
	case DUK_TYPE_BOOLEAN: 
		ival = duk_require_boolean(ctx,0);
		isOK = TRUE;
		switch(itype){
		case FIELDTYPE_SFBool:
			field->sfbool = ival;
		default:
			isOK = FALSE;
		}
		break;
	case DUK_TYPE_STRING:
		sval = duk_require_string(ctx,0);
		isOK = TRUE;
		switch(itype){
		case FIELDTYPE_SFString:
			field->sfstring->strptr = strdup(sval); //should strdup this?
			field->sfstring->len = strlen(sval);
		default:
			isOK = FALSE;
		}
		break;
	case DUK_TYPE_OBJECT:
	{
		int itypeRHS;
		const char *fwTypeRHS = NULL;
		union anyVrml *fieldRHS = NULL;
		rc = duk_get_prop_string(ctx,0,"fwType");
		if(rc == 1) fwType = duk_to_string(ctx,-1);
		duk_pop(ctx);
		rc = duk_get_prop_string(ctx,0,"fwItype");
		if(rc == 1) itypeRHS = duk_to_int(ctx,-1);
		duk_pop(ctx);
		rc = duk_get_prop_string(ctx,0,"fwField");
		if(rc == 1) fieldRHS = duk_to_pointer(ctx,-1);
		duk_pop(ctx);
		/*we don't need the RHS fwChanged=valueChanged* because we are only changing the LHS*/

		if(fieldRHS){
			/* its one of our proxy field types. But is it the type we need?*/
			if(!strcmp(fwType,fwTypeRHS)){ //or if(itype == itypeRHS)
				/* same proxy type - attempt to copy it's value from LHS to RHS  */
				/* copy one anyVrml field to the other by value. 
					Q. what about the p* from complex fields? deep copy or just the pointer?
				*/
				*field = *fieldRHS;
				//*valueChanged = TRUE;
				isOK = TRUE;
			}
		}
	}
		break;
	case DUK_TYPE_NONE: 
	case DUK_TYPE_UNDEFINED: 
	case DUK_TYPE_NULL: 
		/* are we attempting to null out the field? we aren't allowed to change its type (to undefined) */
		memset(field,0,sizeof(union anyVrml));
		isOK = TRUE;
		break;
	case DUK_TYPE_POINTER: 
		/* don't know what this would be for if anything */
	default:
		break;
	}
	if(isOK){
		*valueChanged = TRUE; /*LHS valueChanged*/
	}
	return 0;
}

void push_typed_proxy_fwgetter(duk_context *ctx, const char *fwType, int itype, int mode, const char* fieldname, void *fwpointer,  int* valueChanged)
{
	/*  called by fwgetter (for referenced script->fields)
		1. push_object (fresh object)
		2. fwpointer: reference to script->field[i]->anyvrml
	*/
	int rc;

	//add fwtype to this
	//- if I have one C constructor, and many named js constructors
	//- I need to fetch the name of the constructor and use it here

	//duk_pop(ctx); //pop this

	//duk_push_global_object(ctx); //could I just push an object, or push nothing? then how to get global->handler?
	//int iglobal = duk_get_top(ctx) -1;
	//rc = duk_get_prop_string(ctx,iglobal,"Proxy");
	duk_eval_string(ctx,"Proxy");
	//rc = duk_get_prop_string(ctx,iglobal,"this");
	//duk_push_this(ctx);
	duk_push_object(ctx);
	duk_push_string(ctx,fwType); //"SFColor");
	//show_stack(ctx,"just before putting prop string fwtype");
	duk_put_prop_string(ctx,-2,"fwType");
	//show_stack(ctx,"just after putting prop string fwtype");
	//add native pointer to this
	duk_push_pointer(ctx,fwpointer);
	duk_put_prop_string(ctx,-2,"fwField");
	duk_push_pointer(ctx,valueChanged);
	duk_put_prop_string(ctx,-2,"fwChanged");
	duk_push_int(ctx,itype);
	duk_put_prop_string(ctx,-2,"fwItype");
	duk_push_int(ctx,mode);
	duk_put_prop_string(ctx,-2,"fwMode");
	duk_push_string(ctx,fieldname); //myscriptfield1
	duk_put_prop_string(ctx,-2,"fwName");

	//rc = duk_get_prop_string(ctx,iglobal,"handler");
	duk_eval_string(ctx,"handler");
	duk_new(ctx,2); /* [ global Proxy target handler ] -> [ global result ] */
	//duk_remove(ctx,-2); //remove global so just proxy on stack
}


int push_duk_fieldvalue(duk_context *ctx, int itype, int mode, const char* fieldname, union anyVrml *field, int *valueChanged)
{
	/*we have the field, and even the key name. 
	  So we should be able to decide how to package the outgoing value type:
	  according to specs:
	  - return ecma primitive value type for SFBool, SFInt32, SFFloat, SFDouble, SFTime, SFString
	  - return our field-type-specific object/proxy-wrapper, pointing to our global.field, for the others.
	*/
	int nr;
	nr = 0;
	if(field){
		int isOK = FALSE;
		nr = 1;
		switch(itype){
        case FIELDTYPE_SFBool:
			duk_push_boolean(ctx,field->sfbool); break;
        case FIELDTYPE_SFFloat:
			duk_push_number(ctx,field->sffloat); break;
        case FIELDTYPE_SFTime:
			duk_push_number(ctx,field->sftime); break;
        case FIELDTYPE_SFDouble:
			duk_push_number(ctx,field->sfdouble); break;
        case FIELDTYPE_SFInt32:
			duk_push_int(ctx,field->sfint32); break;
        case FIELDTYPE_SFString:
			duk_push_string(ctx,field->sfstring->strptr); break;
		default:
			//we need an object with our c handlers and pointer to our script->field[i]
			push_typed_proxy_fwgetter(ctx, FIELDTYPES[itype], itype, mode, fieldname, field,  valueChanged);
			break;
		}
	}
	//show_stack(ctx,"in fwgetterNS at end");
    return nr;
}

//convenience wrappers to get details for built-in fields and -on script and protoInstance- dynamic fields
//ifield: for script/proto if >= 1000 then ifield-1000 is an index into dynamic fields array ie script->fields[i], else its treated as builtin ie to get URL on script
//		  for builtin fields index 0,1,2 into fields (offset = index*5, index = offset/5)
void getFieldFromNodeAndName(struct X3D_Node* node,const char *fieldname, int *type, int *kind, int *iifield, union anyVrml **value){
	*type = 0;
	*kind = 0;
	*iifield = -1;
	*value = NULL;
	if(node->_nodeType == NODE_Script) 
	{
		int k;
		struct Vector *sfields;
		struct ScriptFieldDecl *sfield;
		struct FieldDecl *fdecl;
		struct Shader_Script *sp;
		struct CRjsnameStruct *JSparamnames = getJSparamnames();
		struct X3D_Script *snode;

		snode = (struct X3D_Script*)node;
		sp = (struct Shader_Script *)snode->__scriptObj;
		sfields = sp->fields;
		//fprintf(fp,"sp->fields->n = %d\n",sp->fields->n);
		for(k=0;k<sfields->n;k++)
		{
			char *fieldName;
			sfield = vector_get(struct ScriptFieldDecl *,sfields,k);
			//if(sfield->ASCIIvalue) printf("Ascii value=%s\n",sfield->ASCIIvalue);
			fdecl = sfield->fieldDecl;
			fieldName = fieldDecl_getShaderScriptName(fdecl);
			if(!strcmp(fieldName,fieldname)){
				*type = fdecl->fieldType;
				*kind = fdecl->PKWmode;
				*value = &(sfield->value);
				*iifield = k + 1000; //1000 is sentinal value for dynamic/user fields on script/proto
				return;
			}
		}
	}else if(node->_nodeType == NODE_Proto) {
		int k, mode;
		struct Vector* usernames[4];
		const char **userArr;
		struct ProtoFieldDecl* pfield;
		struct X3D_Proto* pnode = (struct X3D_Proto*)node;
		struct VRMLLexer* lexer;
		struct VRMLParser *globalParser;
		struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
		if(pstruct){
			globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;
			lexer = (struct VRMLLexer*)globalParser->lexer;
			usernames[0] = lexer->user_initializeOnly;
			usernames[1] = lexer->user_inputOnly;
			usernames[2] = lexer->user_outputOnly;
			usernames[3] = lexer->user_inputOutput;
			if(pstruct->iface)
			for(k=0; k!=vectorSize(pstruct->iface); ++k)
			{
				const char *fieldName;
				pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, k);
				mode = pfield->mode;
				#define X3DMODE(val)  ((val) % 4)
				userArr =&vector_get(const char*, usernames[X3DMODE(mode)], 0);
				fieldName = userArr[pfield->name];
				if(!strcmp(fieldName,fieldname)){
					*type = pfield->type;
					*kind = pfield->mode;
					if(pfield->mode == PKW_initializeOnly || pfield->mode == PKW_inputOutput)
						*value = &(pfield->defaultVal);
					*iifield = k + 1000; //1000 is sentinal value for dynamic/user fields on script/proto
					return;
				}
			}
		}
	}
	//builtins on non-script, non-proto nodes (and also builtin fields like url on Script)
	{
		typedef struct field_info{
			int nameIndex;
			int offset;
			int typeIndex;
			int ioType;
			int version;
		} *finfo;

		finfo offsets;
		finfo field;
		int ifield;
		offsets = (finfo)NODE_OFFSETS[node->_nodeType];
		ifield = 0;
		field = &offsets[ifield];
		while( field->nameIndex > -1) //<< generalized for scripts and builtins?
		{
			if(!strcmp(FIELDNAMES[field->nameIndex],fieldname)){
				*type = field->typeIndex;
				*kind = field->ioType;
				*iifield = ifield; //1000 is sentinal value for dynamic/user fields on script/proto
				*value = (union anyVrml*)&((char*)node)[field->offset];
				return;
			}
			ifield++;
			field = &offsets[ifield];
		}
	}
}
void getFieldFromNodeAndIndex(struct X3D_Node* node, int iifield, const char **fieldname, int *type, int *kind, union anyVrml **value){
	int ifield = iifield % 1000;
	*type = 0;
	*kind = 0;
	*fieldname = NULL;
	*value = NULL;
	if(node->_nodeType == NODE_Script ) 
	{
		int k;
		struct Vector *sfields;
		struct ScriptFieldDecl *sfield;
		struct FieldDecl *fdecl;
		struct Shader_Script *sp;
		struct CRjsnameStruct *JSparamnames = getJSparamnames();
		struct X3D_Script *snode;

		snode = (struct X3D_Script*)node;

		//sp = *(struct Shader_Script **)&((char*)node)[field->offset];
		sp = (struct Shader_Script *)snode->__scriptObj;
		sfields = sp->fields;
		//fprintf(fp,"sp->fields->n = %d\n",sp->fields->n);
		k = ifield;
		if(k > -1 && k < sfields->n)
		{
			sfield = vector_get(struct ScriptFieldDecl *,sfields,k);
			//if(sfield->ASCIIvalue) printf("Ascii value=%s\n",sfield->ASCIIvalue);
			fdecl = sfield->fieldDecl;
			*fieldname = fieldDecl_getShaderScriptName(fdecl);
			*type = fdecl->fieldType;
			*kind = fdecl->PKWmode;
			*value = &(sfield->value);
		}
		return;
	}else if(node->_nodeType == NODE_Proto ) {
		int k, mode;
		struct Vector* usernames[4];
		const char **userArr;
		struct ProtoFieldDecl* pfield;
		struct X3D_Proto* pnode = (struct X3D_Proto*)node;
		struct VRMLLexer* lexer;
		struct VRMLParser *globalParser;
		struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
		if(pstruct){
			globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;
			lexer = (struct VRMLLexer*)globalParser->lexer;
			usernames[0] = lexer->user_initializeOnly;
			usernames[1] = lexer->user_inputOnly;
			usernames[2] = lexer->user_outputOnly;
			usernames[3] = lexer->user_inputOutput;
			if(pstruct->iface){
				k = ifield;
				if(k > -1 && k < vectorSize(pstruct->iface))
				{
					pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, k);
					mode = pfield->mode;
					#define X3DMODE(val)  ((val) % 4)
					userArr =&vector_get(const char*, usernames[X3DMODE(mode)], 0);
					*fieldname = userArr[pfield->name];
					*type = pfield->type;
					*kind = pfield->mode;
					if(pfield->mode == PKW_initializeOnly || pfield->mode == PKW_inputOutput)
						*value = &(pfield->defaultVal);
				}
			}
		}
		return;
	}
	//builtins on non-script, non-proto nodes (and also builtin fields like url on Script)
	{
		typedef struct field_info{
			int nameIndex;
			int offset;
			int typeIndex;
			int ioType;
			int version;
		} *finfo;

		finfo offsets;
		finfo field;
		offsets = (finfo)NODE_OFFSETS[node->_nodeType];
		field = &offsets[iifield];
		*fieldname = FIELDNAMES[field->nameIndex];
		*type = field->typeIndex;
		*kind = field->ioType;
		*value = (union anyVrml*)&((char*)node)[field->offset];
		return;
	}
}



int fwgetterNS(duk_context *ctx) {
	int nargs, nr;
	int rc, itype, mode, *valueChanged;
	const char *fwName = NULL;
	const char *fwType = NULL;
	union anyVrml *field;

	nargs = duk_get_top(ctx);

	/* retrieve key from nonstandard arg */
	//show_stack(ctx,"in fwgetterNS at start");
	const char *fieldname = duk_require_string(ctx,0);
	//printf("\nfwgetterNS key=%s\n",key);

	/* retrieve field pointer from Cfunc */
	duk_push_current_function(ctx);
	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,-1,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,-1,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,-1,"fwMode");
	if(rc==1) mode = duk_get_int(ctx,-1);
	duk_pop(ctx);
	rc = duk_get_prop_string(ctx,-1,"fwName"); //s.b. same as key/fieldname
	if(rc == 1) fwName = duk_to_string(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,-1,"fwField");
	if(rc == 1) field = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the changed flag */
	rc = duk_get_prop_string(ctx,-1,"fwChanged");
	if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	duk_pop(ctx); //pop current function

	if(0){
		duk_eval_string(ctx,"__script");
		struct Shader_Script *script = (struct Shader_Script*)duk_require_pointer(ctx,-1);
		//printf("script pointer=%x",script);
	}
	if(itype == AUXTYPE_X3DBrowser || itype == AUXTYPE_X3DConstants){
		//duk_push_object(ctx); //proxy object on which get/set handlers will be applied
		push_typed_proxy_fwgetter(ctx, fwType, itype, PKW_initializeOnly, fieldname, NULL, NULL);
		nr = 1;
	}else{
		nr = push_duk_fieldvalue(ctx, itype, mode, fieldname, field,  valueChanged);
	}
	//show_stack(ctx,"in fwgetterNS at end");
    return nr;
}

void add_duk_global_property(duk_context *ctx, int iglobal, int itype, const char *fieldname, const char *ctype, void *fieldptr /*anyVrml*/, int *valueChanged, struct X3D_Node *node, int ifield ){
	int rc;
	char *str;
	//show_stack(ctx,"starting add_duk_global_property");

	duk_eval_string(ctx, "defineAccessor"); //defineAccessor(obj,propName,setter,getter)
	//show_stack(ctx,"after eval");
	/* push object */
	duk_eval_string(ctx,"this"); //global object
	//show_stack(ctx,"myobj?");
	/* push key */
	duk_push_string(ctx,fieldname); //"myprop");
	/* push setter */
	duk_push_c_function(ctx,fwsetterNS,2); //1 extra parameter is nonstandard (NS) key
	duk_push_pointer(ctx,fieldptr);
	duk_put_prop_string(ctx,-2,"fwField");
	duk_push_pointer(ctx,valueChanged);
	duk_put_prop_string(ctx,-2,"fwChanged");
	duk_push_int(ctx,itype);
	duk_put_prop_string(ctx,-2,"fwItype");
	duk_push_string(ctx,ctype);
	duk_put_prop_string(ctx,-2,"fwType");
	duk_push_pointer(ctx,node);
	duk_put_prop_string(ctx,-2,"fwNode");
	duk_push_int(ctx,ifield); 
	duk_put_prop_string(ctx,-2,"fwIfield");
	/* push getter */
	duk_push_c_function(ctx,fwgetterNS,1); //0 extra parameter is nonstandard (NS) key
	duk_push_pointer(ctx,fieldptr);
	duk_put_prop_string(ctx,-2,"fwField");
	duk_push_pointer(ctx,valueChanged);
	duk_put_prop_string(ctx,-2,"fwChanged");
	duk_push_int(ctx,itype);
	duk_put_prop_string(ctx,-2,"fwItype");
	duk_push_string(ctx,ctype);
	duk_put_prop_string(ctx,-2,"fwType");
	duk_push_pointer(ctx,node);
	duk_put_prop_string(ctx,-2,"fwNode");
	duk_push_int(ctx,ifield); 
	duk_put_prop_string(ctx,-2,"fwIfield");

	//show_stack(ctx,"C");
	duk_call(ctx, 4);
	duk_pop(ctx);
	//show_stack(ctx,"D");
	if(0){
		//test evals:
		char teststring[1000];
		strcpy(teststring,fieldname);
		strcat(teststring," = 'halleluha!';");
		//duk_eval_string(ctx,"myprop = 'halleluha!';");
		duk_eval_string(ctx,teststring);
		duk_pop(ctx);
		show_stack(ctx,"before print");
		duk_eval_string(ctx,"print('hi there');"); duk_pop(ctx);
		strcpy(teststring,"print(");
		strcat(teststring,fieldname);
		strcat(teststring,".toString());");
		//duk_eval_string(ctx,"print(myprop.toString());");
		duk_eval_string(ctx,teststring);
		duk_pop(ctx);
	}
	//show_stack(ctx,"E");
}

void InitScriptField2(struct CRscriptStruct *scriptcontrol, indexT kind, int itype, const char* fieldname, union anyVrml *fieldvalue, int *valueChanged, struct X3D_Node* parent, int ifield)
{
	/* Creates a javascript-context twin of a Script node for fields of type:
	 *  field/initializeOnly, eventOut/outputOnly, and the field/eventOut part of exposedField/inputOutput
	 *  (not for eventIn/inputOnly, which linked elsewhere to scene author's javascript functions)
	 * puts the twin as a property on the context's global object
	 * should make the property 'strict' meaning the property can't be deleted by the script during execution
	 * but get/set should work normally on the property
	 * a set should cause a valueChanged flag to be set somewhere, so gatherScriptEventOuts 
	 *   can route from eventOut/outputOnly or the eventOut part of exposedField/inputOutput
	 * InitScriptField2 version: instead of jsNative, hook back into Script_Node->fields[i] for get/set storage
	*/
	duk_context *ctx;
	int iglobal;
	printf("in InitScriptField\n");

	if (kind == PKW_inputOnly) return; //we'll hook input events to the author's functions elsewhere
	//everything else -fields, eventOuts- needs a strict property twin created on the global object
	// create twin property
	ctx = scriptcontrol->cx;
	iglobal = *(int*)scriptcontrol->glob; 
	add_duk_global_property(ctx,iglobal,itype,fieldname, FIELDNAMES[itype], fieldvalue,valueChanged,parent,ifield);
	*valueChanged = 0;

	return;
}

void JSInitializeScriptAndFields (int num) {
	/*  1. creates javascript-context twins of Script node dynamic/authored fields
		2. runs the script as written by the scene author, which has the effect of
			declaring all the author's functions (and checking author's syntax)
	*/
	struct Shader_Script *script;
	struct ScriptFieldDecl *field;
	int i,nfields, kind, itype;
	const char *fieldname;
	struct CRscriptStruct *ScriptControlArray, *scriptcontrol;
	ScriptControlArray = getScriptControl();
	scriptcontrol = &ScriptControlArray[num];


	/* run through fields in order of entry in the X3D file */
	script = scriptcontrol->script;
	printf("adding fields from script %x\n",script);
	nfields = Shader_Script_getScriptFieldCount(script);
	for(i=0;i<nfields;i++){
		field = Shader_Script_getScriptField(script,i);
		fieldname = ScriptFieldDecl_getName(field);
		kind = ScriptFieldDecl_getMode(field);
		itype = ScriptFieldDecl_getType(field);
		InitScriptField2(scriptcontrol, kind, itype, fieldname, &field->value, &field->valueChanged, script->ShaderScriptNode, i+1000);
	}
	
	if (!jsActualrunScript(num, scriptcontrol->scriptText)) {
		ConsoleMessage ("JSInitializeScriptAndFields, script failure\n");
		scriptcontrol->scriptOK = FALSE;
		scriptcontrol->_initialized = TRUE;
		return;
	}
	FREE_IF_NZ(scriptcontrol->scriptText);
	scriptcontrol->_initialized = TRUE;
	scriptcontrol->scriptOK = TRUE;

	return;
}

int jsActualrunScript(int num, char *script){
	int len, rc;
	duk_context *ctx;
	int iglobal;
	struct CRscriptStruct *ScriptControl = getScriptControl();
	printf("in jsActualrunScript\n");


	/* get context and global object for this script */
	ctx = (duk_context *)ScriptControl[num].cx;
	iglobal = *((int *)ScriptControl[num].glob);

	//CLEANUP_JAVASCRIPT(_context)

	len = (int) strlen(script);
	rc=0;
	duk_eval_string(ctx, script);
	if(rc<0){
		printf ("ActualrunScript - JS_EvaluateScript failed for %s", script);
		printf ("\n");
		ConsoleMessage ("ActualrunScript - JS_EvaluateScript failed for %s", script);
		return FALSE;
	}
	duk_pop(ctx);
	return TRUE;
}
void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value){
	return;
}
static int duk_once = 0;
void process_eventsProcessed(){
	if(!duk_once) printf("in process_eventsProcessed\n");
	duk_once++;
	return;
}
void js_cleanup_script_context(int counter){
	printf("in js_cleanup_script_context\n");
	return;
}
void js_setField_javascriptEventOut_B(union anyVrml* any, int fieldType, unsigned len, int extraData, int actualscript){
	printf("in js_setField_javascriptEventOut_B\n");
	return;
}
void js_setField_javascriptEventOut(struct X3D_Node *tn,unsigned int tptr,  int fieldType, unsigned len, int extraData, int actualscript){
	printf("in js_setField_javascriptEventOut\n");
	return;
}

void setScriptECMAtype(int num){
	printf("in setScriptECMAtype\n");
	return;
}
void set_one_ECMAtype (int tonode, int toname, int dataType, void *Data, int datalen){
	printf("in set_one_ECMAtype\n");
	return;
}
void set_one_MultiElementType (int tonode, int tnfield, void *Data, int dataLen){
	printf("in set_one_MultiElementType\n");
	return;
}
void set_one_MFElementType(int tonode, int toname, int dataType, void *Data, int datalen){
	printf("in set_one_MFElementType\n");
	return;
}
int jsIsRunning(){
	printf("in jsIsRunning\n");
	return 1;
}
void JSDeleteScriptContext(int num){
	printf("in JSDeleteScriptContext\n");
	return;
}
void jsShutdown(){
	printf("in jsShutdown\n");
	return;
}
void jsClearScriptControlEntries(int num){
	printf("in jsClearScriptControlEntries\n");
	return;
}
#endif /*  defined(JAVASCRIPT_DUK) */