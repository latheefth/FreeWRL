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

#include "duktape/duktape.h"

typedef int indexT;

#include "JScript.h"
typedef struct pJScript{

#ifdef HAVE_JAVASCRIPT_SM
	JSRuntime *runtime;// = NULL;
	JSClass globalClass;
	jsval JSglobal_return_value;
#endif // HAVE_JAVASCRIPT
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
#ifdef HAVE_JAVASCRIPT_SM
		p->runtime = NULL;
		memcpy(&p->globalClass,&staticGlobalClass,sizeof(staticGlobalClass));
		t->JSglobal_return_val = &p->JSglobal_return_value;
#endif // HAVE_JAVASCRIPT
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

void show_stack(duk_context *ctx, char* comment)
{
	int itop = duk_get_top(ctx);
	if(comment) printf("%s top=%d\n",comment,itop);
	//printf("%10s%10s%10s\n","position","type","more");
	printf("%10s%10s\n","position","type");
	for(int i=0;i<itop;i++){
		int ipos = -(i+1);
		int t = duk_get_type(ctx, ipos);
		char *stype = NULL;
		switch(t){
			case DUK_TYPE_NUMBER: stype ="number"; break;
			case DUK_TYPE_STRING: stype ="string"; break;
			case DUK_TYPE_OBJECT: stype ="object"; break;
			case DUK_TYPE_NONE: stype ="none"; break;
			case DUK_TYPE_UNDEFINED: stype ="undefined"; break;
			case DUK_TYPE_BOOLEAN: stype ="boolean"; break;
			case DUK_TYPE_NULL: stype ="null"; break;
			case DUK_TYPE_POINTER: stype ="pointer"; break;
			default:
				stype = "unknown";
		}
		char * amore = "";
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
		printf("%10d%10s%10s\n",ipos,stype,amore);
	}
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

void push_typed_proxy(duk_context *ctx, const char *fwType, void *fwpointer)
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
	duk_put_prop_string(ctx,-2,"fwType");
	//add native pointer to this
	duk_push_pointer(ctx,fwpointer);
	duk_put_prop_string(ctx,-2,"fwField");
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
	nargs = duk_get_top(ctx);

	duk_push_current_function(ctx);
	rc = duk_get_prop_string(ctx, -1, "fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	if(rc == 1) printf("in constructor fwType=%s\n",fwType);
	duk_pop(ctx);

	show_stack(ctx,"in C constructor before push this");
	duk_push_this(ctx);
	//add native pointer to this
	void *fwpointer = malloc(sizeof(nativeStruct));

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
		push_typed_proxy(ctx, fwType, fwpointer);
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
	int rc;
	const char *fwType = NULL;
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
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
int push_duk_fieldvalueObject(duk_context *ctx, int itype, struct ScriptFieldDecl *field ){
	//we need an object with our c handlers and pointer to our script->field[i]
	push_typed_proxy(ctx, FIELDTYPES[itype], field);
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

int cget(duk_context *ctx) {
	int rc, nr;
	//show_stack(ctx,"in cget");
	const char *fwType = NULL;
	struct ScriptFieldDecl* parent = NULL;
	struct ScriptFieldDecl* field = NULL;

	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	//if(fwType) printf("fwType in cget=%s\n",fwType);


	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,0,"fwField");
	if(rc == 1) parent = duk_to_string(ctx,-1);
	duk_pop(ctx);

	nr = 0;
	if(!fwType || !parent) return nr;

	/* figure out what field on the parent the get is referring to */
	if(!strncmp(fwType,"MF",2) || !strncmp(fwType,"SF",2)){
		if(duk_is_number(ctx,-2)){
			//indexer
			int ikey = duk_get_int(ctx,-2);
			//int ikey = round(key);
			if(!strncmp(fwType,"MF",2)){
				//its an MF field type, and we have an index to it.
				if(ikey < parent->value.mfbool.n && ikey > -1){
					// valid index range - figure out what type the SF is and return the element
					int isize;
					int iSFtype = ScriptFieldDecl_getType(parent);
					//convert the parent's MF type to equivalent SF type for element
					iSFtype = mf2sf(iSFtype);
					isize = sizeofSF(iSFtype);
					if(isECMAtype(iSFtype)){
						union anyVrml *fieldvalue;
						fieldvalue = (union anyVrml*)((char*)(parent->value.mfbool.p)+ ikey*isize);
						nr = push_duk_fieldvalueECMA(ctx, iSFtype, fieldvalue);
					}else{
						field = parent->value.mfbool.p[ikey];
						nr = push_duk_fieldvalueObject(ctx, iSFtype, field);
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
	}else if(!strcmp(fwType,"Browser")){
	}else if(!strcmp(fwType,"Scene") || !strcmp(fwType,"ExecutionContext")){
	}
    return 1;
}
int cset(duk_context *ctx) {
	int rc;
	const char *fwType = NULL;
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	if(fwType) printf("fwType in cset=%s\n",fwType);

	show_stack(ctx,"in cset");
	//char *val = duk_require_string(ctx,-2);
	const char *val = duk_to_string(ctx,-2);
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
	printf(" val=%s \n",val);
    return 0;
}
int cdel(duk_context *ctx) {
	int rc;
	const char *fwType = NULL;
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	if(fwType) printf("fwType in cdel=%s\n",fwType);
	show_stack(ctx,"in cdel");
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


static char *DefaultScriptMethods = "function initialize() {}; " \
			" function shutdown() {}; " \
			" function eventsProcessed() {}; " \
			" TRUE=true; FALSE=false; " \
			" function print(x) {Browser.print(x)}; " \
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
	addCustomProxyType(ctx, iglobal, "Browser"); 
	//add x3d v3.3 ecmascript constants table if you can find it _anywhere_ - I see snippents of X3DConstants.(modes, field types, error codes) etc
	//	http://www.web3d.org/files/specifications/19777-1/V3.0/index.html
	//  - see language bindings > ecmascript > examples/tables etc


	//test
	duk_eval_string(ctx,"var myvec3 = new SFVec3f(1.0,2.0,3.0);");
	duk_pop(ctx);
	duk_eval_string(ctx,"print(myvec3.x.toString());");
	duk_pop(ctx);
	duk_eval_string(ctx,"myvec3.y = 45.0;");
	duk_pop(ctx);
	duk_eval_string(ctx,"print('sb45='+myvec3.y);");
	duk_pop(ctx);


	//* Global methods and defines (some redirecting to the Browser object ie print = Browser.println)
	//if (!ACTUALRUNSCRIPT(num,DefaultScriptMethods,&rval))
	//	cleanupDie(num,"runScript failed in VRML::newJS DefaultScriptMethods");
	if(0){
		/* I need these working, but print = Browser.print isn't hooked up, and bombs later*/
		show_stack(ctx,"\nbefore eval DefaultScriptMethods");
		duk_eval_string(ctx,DefaultScriptMethods);
		duk_pop(ctx);
	}
	show_stack(ctx,"done initializeContext - should be 1 object (global)");
	//duk_eval_string(ctx,"print('hi there');"); duk_pop(ctx);


	/* send this data over to the routing table functions. */
	CRoutes_js_new (num, JAVASCRIPT);
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
int jsActualrunScript(int num, char *script){
	printf("in jsActualrunScript\n");
	return 0;
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
int get_valueChanged_flag (int fptr, int actualscript){
	printf("in get_valueChanged_flag\n");
	return 0;
}
void resetScriptTouchedFlag(int actualscript, int fptr){
	printf("in get_valueChanged_flag\n");
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

int fwsetterNS(duk_context *ctx) {
	/* myfield = new SFVec3f(1,2,3); 
	 * if myfield is a property we set on the global object, and we've assigned this setter to it,
	 * we'll come in here. We can set the script->fields[i].valueChanged and .value.
	 */
	int nargs, rc;
	struct Shader_Script *script;
	struct ScriptFieldDecl *field = NULL;
	nargs = duk_get_top(ctx);

	/* retrieve key from nonstandard arg */
	//show_stack(ctx,"in fwsetterNS");
    nativeValue = duk_require_string(ctx, 0);
    //implicit key by setter C function //char *key = duk_require_string(ctx, 1);
	const char *key = duk_require_string(ctx,1); //"myprop";
	//printf("\nfwsetterNS, key=%s value=%s\n",key,nativeValue);

	/* retrieve field pointer from Cfunc */
	duk_push_current_function(ctx);
	rc = duk_get_prop_string(ctx, -1, "fwField");
	if(rc == 1){
		field = duk_to_pointer(ctx,-1);
		script = field->script;
		//printf("in fwsetterNS fwField=%x\n",field);
	}
	duk_pop(ctx); //Q. should this one be conditional on rc==1?
	duk_pop(ctx);

	/*we have the field, and even the key name. 
	  So we should be able to decide how to deal with the incoming set value type 
	  according to specs:
	  - convert incoming ecma primitive value type for SFBool, SFInt32, SFFloat, SFDouble, SFTime, SFString
	  - if it's one of our field-type-specific object/proxy-wrapper, copy the field values
	  - if it's something else, return error, unknown conversion
	  if succesful set valueChanged
	*/
	if(field){
		int imode, itype, isOK = FALSE;
		imode = ScriptFieldDecl_getMode(field);
		itype = ScriptFieldDecl_getType(field);
		if(duk_is_number(ctx,0)){
			double val = duk_require_number(ctx,0);
			isOK = TRUE;
			switch(itype){
				case FIELDTYPE_SFFloat:
					field->value.sffloat = val; break;
				case FIELDTYPE_SFTime:
					field->value.sftime = val; break;
				case FIELDTYPE_SFDouble:
					field->value.sfdouble = val; break;
				case FIELDTYPE_SFInt32:
					field->value.sfint32 = round(val); break;
				default:
					isOK = FALSE;
			}
		}else if(duk_is_boolean(ctx,0)){
			int ival = duk_require_boolean(ctx,0);
			isOK = TRUE;
			switch(itype){
			case FIELDTYPE_SFBool:
				field->value.sfbool = ival;
			default:
				isOK = FALSE;
			}
		}else if(duk_is_string(ctx,0)){
			const char* sval = duk_require_string(ctx,0);
			isOK = TRUE;
			switch(itype){
			case FIELDTYPE_SFString:
				field->value.sfstring->strptr = strdup(sval); //should strdup this?
				field->value.sfstring->len = strlen(sval);
			default:
				isOK = FALSE;
			}
		}else if(duk_is_object(ctx,0)){
			const char *fwType = NULL;
			rc = duk_get_prop_string(ctx,0,"fwType");
			if(rc == 1) fwType = duk_to_string(ctx,-1);
			duk_pop(ctx);
			if(rc && fwType){
				/* its one of our proxy field types. But is it the type we need?*/
				if(!strcmp(fwType,FIELDTYPES[itype])){
					/* same proxy type - attempt to copy it  */
					struct ScriptFieldDecl *other = NULL;
					rc = duk_get_prop_string(ctx,0,"fwPointer");
					if(rc == 1) other = duk_to_pointer(ctx,-1);
					if(other){
						/* copy one field to the other. I think it's just the anyVrml we need */
						field->value = other->value;
						//scriptFieldDecl_setFieldValue(field, union anyVrml v)
						field->valueSet = TRUE;
						isOK = TRUE;
					}
				}
			}
		}
		if(isOK){
			field->valueChanged = 1;
		}
	}
	return 0;
}
int push_duk_fieldvalue(duk_context *ctx, struct ScriptFieldDecl *field)
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
		int imode, itype, isOK = FALSE;
		imode = ScriptFieldDecl_getMode(field);
		itype = ScriptFieldDecl_getType(field);
		nr = 1;
		switch(itype){
        case FIELDTYPE_SFBool:
			duk_push_boolean(ctx,field->value.sfbool); break;
        case FIELDTYPE_SFFloat:
			duk_push_number(ctx,field->value.sffloat); break;
        case FIELDTYPE_SFTime:
			duk_push_number(ctx,field->value.sftime); break;
        case FIELDTYPE_SFDouble:
			duk_push_number(ctx,field->value.sfdouble); break;
        case FIELDTYPE_SFInt32:
			duk_push_int(ctx,field->value.sfint32); break;
        case FIELDTYPE_SFString:
			duk_push_string(ctx,field->value.sfstring->strptr); break;
		default:
			//we need an object with our c handlers and pointer to our script->field[i]
			push_typed_proxy(ctx, FIELDTYPES[itype], field);
			break;
		}
	}
	//show_stack(ctx,"in fwgetterNS at end");
    return nr;
}
int fwgetterNS(duk_context *ctx) {
	int nargs, rc, nr;
	struct Shader_Script *script;
	struct ScriptFieldDecl *field = NULL;
	nargs = duk_get_top(ctx);

	/* retrieve key from nonstandard arg */
	//show_stack(ctx,"in fwgetterNS at start");
	const char *key = duk_require_string(ctx,0);
	//printf("\nfwgetterNS key=%s\n",key);

	/* retrieve field pointer from Cfunc */
	duk_push_current_function(ctx);
	rc = duk_get_prop_string(ctx, -1, "fwField");
	if(rc == 1){
		field = duk_to_pointer(ctx,-1);
		script = field->script;
		//printf("in fwgetterNS fwField=%x\n",field);
	}
	duk_pop(ctx);
	duk_pop(ctx);

	if(0){
		duk_eval_string(ctx,"__script");
		script = (struct Shader_Script*)duk_require_pointer(ctx,-1);
		//printf("script pointer=%x",script);
	}
	//duk_push_string(ctx, nativeValue);
	if(0){
		/*we have the field, and even the key name. 
		  So we should be able to decide how to package the outgoing value type:
		  according to specs:
		  - return ecma primitive value type for SFBool, SFInt32, SFFloat, SFDouble, SFTime, SFString
		  - return our field-type-specific object/proxy-wrapper, pointing to our global.field, for the others.
		*/
		nr = 0;
		if(field){
			int imode, itype, isOK = FALSE;
			imode = ScriptFieldDecl_getMode(field);
			itype = ScriptFieldDecl_getType(field);
			nr = 1;
			switch(itype){
			case FIELDTYPE_SFBool:
				duk_push_boolean(ctx,field->value.sfbool); break;
			case FIELDTYPE_SFFloat:
				duk_push_number(ctx,field->value.sffloat); break;
			case FIELDTYPE_SFTime:
				duk_push_number(ctx,field->value.sftime); break;
			case FIELDTYPE_SFDouble:
				duk_push_number(ctx,field->value.sfdouble); break;
			case FIELDTYPE_SFInt32:
				duk_push_int(ctx,field->value.sfint32); break;
			case FIELDTYPE_SFString:
				duk_push_string(ctx,field->value.sfstring->strptr); break;
			default:
				//we need an object with our c handlers and pointer to our script->field[i]
				push_typed_proxy(ctx, FIELDTYPES[itype], field);
				break;
			}
		}
	}else{
		nr = push_duk_fieldvalue(ctx, field);
	}
	//show_stack(ctx,"in fwgetterNS at end");
    return nr;
}

void add_duk_global_property(duk_context *ctx, int iglobal, const char *fieldname, void *fieldptr){
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
	/* push getter */
	duk_push_c_function(ctx,fwgetterNS,1); //0 extra parameter is nonstandard (NS) key
	duk_push_pointer(ctx,fieldptr);
	duk_put_prop_string(ctx,-2,"fwField");
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

void InitScriptField2(struct CRscriptStruct *scriptcontrol, indexT kind, const char* fieldname, struct ScriptFieldDecl *field)
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
	add_duk_global_property(ctx,iglobal,fieldname,field);
	field->valueChanged = 0;

	return;
}





void JSInitializeScriptAndFields (int num) {
	/*  1. creates javascript-context twins of Script node dynamic/authored fields
		2. runs the script as written by the scene author, which has the effect of
			declaring all the author's functions (and checking author's syntax)
	*/
	struct Shader_Script *script;
	struct ScriptFieldDecl *field;
	int i,nfields, kind;
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
		InitScriptField2(scriptcontrol, kind, fieldname, field);
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
void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value){
	return;
}
#endif /*  defined(JAVASCRIPT_DUK) */