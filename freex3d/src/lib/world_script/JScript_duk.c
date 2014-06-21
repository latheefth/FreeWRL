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
	show_stack(ctx,"in C constructor");

	//add fwtype to this
	//- if I have one C constructor, and many named js constructors
	//- I need to fetch the name of the constructor and use it here
	duk_push_string(ctx,fwType); //"SFColor");
	duk_put_prop_string(ctx,-2,"fwType");
	//add native pointer to this
	void *fwpointer = malloc(sizeof(nativeStruct));
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


int cget(duk_context *ctx) {
	int rc;
	show_stack(ctx,"in cget");
	const char *fwType = NULL;
	rc = duk_get_prop_string(ctx,0,"fwType");
	if(rc == 1) fwType = duk_to_string(ctx,-1);
	duk_pop(ctx);
	if(fwType) printf("fwType in cget=%s\n",fwType);
	if(duk_is_number(ctx,-2)){
		//indexer
		double key = duk_require_number(ctx,-2);
		int ikey = round(key);
		printf("index =%d\n",ikey);
		duk_push_string(ctx,nativeStruct.arr[ikey]);
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

//void JSCreateScriptContext(int num){return;}
/* create the script context for this script. This is called from the thread
   that handles script calling in the fwl_RenderSceneUpdateScene */
void JSCreateScriptContext(int num) {
	int iglobal, rc;
	//jsval rval;
	duk_context *ctx; 	/* these are set here */
	//JSObject *_globalObj; 	/* these are set here */
	//BrowserNative *br; 	/* these are set here */
	ppJScript p = (ppJScript)gglobal()->JScript.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();


	//CREATE CONTEXT
	//_context = JS_NewContext(p->runtime, STACK_CHUNK_SIZE);
	//if (!_context) freewrlDie("JS_NewContext failed");
	//JS_SetErrorReporter(_context, errorReporter);
	ctx = duk_create_heap_default();
	iglobal = duk_get_top(ctx) -1;

	//SAVE OUR CONTEXT IN OUR PROGRAM'S SCRIPT NODE FOR LATER RE-USE
	//ScriptControl[num].cx =  _context;
	//ScriptControl[num].glob =  _globalObj;
	ScriptControl[num].cx =  ctx;
	ScriptControl[num].glob =  (void *)malloc(sizeof(int)); 
	*((int *)ScriptControl[num].glob) = iglobal; //we'll be careful not to pop our global for this context (till context cleanup)

	//ADD STANDARD JS GLOBAL OBJECT/CLASSES
	//_globalObj = JS_NewObject(_context, &p->globalClass, NULL, NULL);
	/* gets JS standard classes */
	//if (!JS_InitStandardClasses(_context, _globalObj))
	//	freewrlDie("JS_InitStandardClasses failed");
    duk_push_global_object(ctx);
	iglobal = duk_get_top(ctx) -1;

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
	show_stack(ctx,"\nbefore eval DefaultScriptMethods");
	duk_eval_string(ctx,DefaultScriptMethods);
	duk_pop(ctx);
	show_stack(ctx,"done initializeContext - should be 1 object (global)");


	/* send this data over to the routing table functions. */
	CRoutes_js_new (num, JAVASCRIPT);
	return;
}

void process_eventsProcessed(){
	printf("in process_eventsProcessed\n");
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
void InitScriptField(int num, indexT kind, indexT type, const char* field, union anyVrml value)
{
	/* 
	*/
	printf("in InitScriptField\n");
	return;
}




void jsClearScriptControlEntries(int num){
	printf("in jsClearScriptControlEntries\n");
	return;
}

#endif /*  defined(JAVASCRIPT_DUK) */