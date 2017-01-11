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

/* To do list July 2014
- runQueuedDirectOutputs() - is there a way to flag a Script Node so this isn't a double loop over all scripts and fields?
- cfwconstructor - fwtype could be extended to articulate allowed AUXTYPEs and FIELDTYPEs for a given W or P
*/


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
#define FIELDTYPE_MFImage	43 
typedef int indexT;

#ifdef DEBUG_MALLOC
#define malloc(A) MALLOCV(A)
#define free(A) FREE_IF_NZ(A)
#define realloc(A,B) REALLOC(A,B)
#endif

FWTYPE *fwtypesArray[60];  //true statics - they only need to be defined once per process, we have about 50 types as of july 2014
int FWTYPES_COUNT = 0;

void initVRMLBrowser(FWTYPE** typeArray, int *n);
void initVRMLFields(FWTYPE** typeArray, int *n);
void initFWTYPEs(){
	initVRMLBrowser(fwtypesArray, &FWTYPES_COUNT);
	initVRMLFields(fwtypesArray, &FWTYPES_COUNT);
}
FWTYPE *getFWTYPE(int itype){
	int i;
	for(i=0;i<FWTYPES_COUNT;i++){
		if(itype == fwtypesArray[i]->itype)
			return fwtypesArray[i];
	}
	return NULL;
}
#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

FWFunctionSpec *getFWFunc(FWTYPE *fwt,const char *key){
	int i = 0;
	FWFunctionSpec *fs = fwt->Functions;
	if(fs)
	while(fs[i].name){
		if(!strcasecmp(fs[i].name,key)){
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
		if(!strcasecmp(ps[i].name,key)){
			//found it - its a property, return propertySpec
			(*index) = ps[i].index; //index can be any +- integer
			return &ps[i];
		}
		i++;
	}
	return NULL;
}
int len_properties(FWPropertySpec *ps){
	int len = 0;
	if(ps) while(ps[len].name) len++;
	return len;
}
int len_functions(FWFunctionSpec *fs){
	int len = 0;
	if(fs) while(fs[len].name) len++;
	return len;
}
int fwiterator_generic(int index, FWTYPE *fwt, void *pointer, const char **name, int *lastProp, int *jndex, char *type, char *readOnly){
	//start iterating by passing -1 for index. When you get -1 back, you are done.
	//FWPointer is for SFNode: it will have an instance-specific result from its custom iterator
	//next property
	int lenp, lenf, ifindex;
	FWPropertySpec *ps;
	FWIterator iterator;
	FWFunctionSpec *fs;
	(*jndex) = 0;
	ps = fwt->Properties;
	iterator = fwt->iterator;
	if(ps){
		index ++;
		lenp = len_properties(ps);
		if(index < lenp){
			(*name) = ps[index].name;
			(*jndex) = ps[index].index;
			(*lastProp) = index;
			(*type) = ps[index].type;
			(*readOnly) = ps[index].readOnly;
			return index;
		}
	}else if(iterator){
		int iret = iterator(index, fwt, pointer, name, lastProp, jndex, type, readOnly);
		if(iret > -1) return iret;
		index++; //for functions below
	}else{
		index++; //may not have properties (or iterator) like SFFloat, which has a valueOf function
	}
	//next function
	fs = fwt->Functions;
	lenf = len_functions(fs);
	ifindex = index - 1 - (*lastProp);
	if(ifindex < lenf){
		(*name) = fs[ifindex].name;
		(*type) = 'f';
		(*readOnly) = 'T';
		return index;
	}
	return -1;
}

int fwhas_generic(FWTYPE *fwt, void *pointer, const char *key, int *jndex, char *type, char *readOnly){
	char *name;
	int lastProp, isSet, index = -1;
	lastProp = -1;
	isSet = FALSE;
	
	while( (index = fwiterator_generic(index,fwt,pointer,&name, &lastProp, jndex, type, readOnly)) > -1){
		if(!strcasecmp(name,key)){
			//found it
			return TRUE;
		}
	}
	if(strlen(key)>4 && !strncmp(key,"set_",4))
		isSet = TRUE;

	if(isSet){
		char* key2 = &key[4];
		while( (index = fwiterator_generic(index,fwt,pointer,&name, &lastProp, jndex, type, readOnly)) > -1){
			if(!strcasecmp(name,key2)){
				//found it
				return TRUE;
			}
		}
	}
	return FALSE;
}



typedef struct pJScript{
	int ijunk;
}* ppJScript;


void *JScript_constructor(){
	void *v = MALLOCV(sizeof(struct pJScript));
	memset(v,0,sizeof(struct pJScript));
	return v;
}
void JScript_init(struct tJScript *t){
	//public
	t->JSglobal_return_val = NULL;
	//private
	t->prv = JScript_constructor();
	{
		//ppJScript p = (ppJScript)t->prv;
		//initialize statics
		if(!FWTYPES_COUNT) initFWTYPEs();
	}
}
//	ppJScript p = (ppJScript)gglobal()->JScript.prv;

//stubs the linker will be looking for
void jsVRMLBrowser_init(void *t){}
void jsUtils_init(void *t){}
void jsVRMLClasses_init(void *t){}




//==============ENGINE-AGNOSTIC HELPER CODE (could be extracted to other module) ====================


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
char * itype2string(int itype){
	int i = 0;
	while(lookup_fieldType[i].c){
		if(lookup_fieldType[i].i == itype) return lookup_fieldType[i].c;
		i++;
	}
	return NULL;
}

int getFieldFromNodeAndName(struct X3D_Node* node,const char *fieldname, int *type, int *kind, int *iifield, union anyVrml **value);


int get_valueChanged_flag (int fptr, int actualscript){
	char *fullname;
	union anyVrml* value;
	int type, kind, ifield, found;
	struct X3D_Node *node;
	struct Shader_Script *script;
	struct ScriptFieldDecl *field;
	struct CRscriptStruct *scriptcontrol; //, *ScriptControlArr = getScriptControl();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	scriptcontrol = getScriptControlIndex(actualscript); //&ScriptControlArr[actualscript];
	script = scriptcontrol->script;
	node = script->ShaderScriptNode;
	fullname = JSparamnames[fptr].name;
	found = getFieldFromNodeAndName(node,fullname,&type,&kind,&ifield,&value);
	if(found){
		field = Shader_Script_getScriptField(script, ifield);
		gglobal()->JScript.JSglobal_return_val = (void *)&field->value;
		return field->valueChanged;
	}
	gglobal()->JScript.JSglobal_return_val = NULL;
	return 0;
}
void resetScriptTouchedFlag(int actualscript, int fptr){
	char *fullname;
	union anyVrml* value;
	int type, kind, ifield, found;
	struct X3D_Node *node;
	struct Shader_Script *script;
	struct ScriptFieldDecl *field;
	struct CRscriptStruct *scriptcontrol; // *ScriptControlArr = getScriptControl();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	scriptcontrol = getScriptControlIndex(actualscript); //&ScriptControlArr[actualscript];
	script = scriptcontrol->script;
	node = script->ShaderScriptNode;
	fullname = JSparamnames[fptr].name;
	found = getFieldFromNodeAndName(node,fullname,&type,&kind,&ifield,&value);
	if(found){
		field = Shader_Script_getScriptField(script, ifield);
		field->valueChanged = 0;
	}
	//printf("in get_valueChanged_flag\n");
	return;
}

//const char *stringFieldtypeType (int st); //in generatedcode
//const char *stringNodeType (int st);
int fwType2itype(const char *fwType){
	int i, isSF, isMF, ifield = -1;
	const char *suffix;
	isSF = !strncmp(fwType,"SF",2);
	isMF = !strncmp(fwType,"MF",2);
	if(isSF || isMF){
		suffix = &fwType[2]; //skip SF/MF part
		i = 0;
		while(lookup_fieldType[i].c){
			if(!strcasecmp(suffix,lookup_fieldType[i].c)){
				ifield = lookup_fieldType[i].i;
				break;
			}
			i++;
		}
		if(ifield > -1 && isMF ) ifield++;
	}else{
		//browser and scene/executionContext shouldn't be going through fwconstructor
		if(!strcasecmp(fwType,"Browser")) ifield = AUXTYPE_X3DBrowser;
		if(!strcasecmp(fwType,"X3DConstants")) ifield = AUXTYPE_X3DConstants;
	}
	return ifield;
}
void freeField(int itype, void* any){
	if(isSForMFType(itype) == 0){
		//if(itype == FIELDTYPE_SFString){
		//	struct Uni_String *sf = (struct Uni_String*)any;
		//	if(sf) free(sf->strptr);
		//	free(sf);
		//}
		free(any); //SF
	}else if(isSForMFType(itype) == 1){
		//MF
		struct Multi_Any* mf = (struct Multi_Any*)any;
		//if(itype == FIELDTYPE_MFString){
		//	int i;
		//	struct Multi_String *ms = (struct Multi_String*)mf;
		//	for(i=0;i<ms->n;i++){
		//		struct Uni_String *sf = ms->p[i];
		//		if(sf) free(sf->strptr);
		//		free(sf);
		//	}
		//}
		free(mf->p);  //if bombs, it could be because I'm not deep copying or medium_copy_field() everywhere I should
		free(mf);
	}
}

#include <math.h> //for int = round(numeric)
unsigned long upper_power_of_two(unsigned long v);
void deleteMallocedFieldValue(int type,union anyVrml *fieldPtr);
void medium_copy_field0(int itype, void* source, void* dest)
{
	/* medium-deep copies field up to and including pointer: doesn't deep copy *(SFNode*) or *(SFString*), 
		- SFString treated analogous to const char * 
		- malloc your starting type outside
	*/
	
	int i, sfsize,sformf;
	int sftype, isMF;
	struct Multi_Any *mfs,*mfd;

	sformf = isSForMFType(itype);
	if(sformf < 0){
		printf("bad type in medium_copy_field0\n");
		return;
	}
	isMF = sformf == 1; 
	sftype = type2SF(itype);
	//from EAI_C_CommonFunctions.c
	sfsize = sizeofSF(sftype); //returnElementLength(sftype) * returnElementRowSize(sftype);
	if(isMF)
	{
		int nele;
		char *ps, *pd;
		mfs = (struct Multi_Any*)source;
		mfd = (struct Multi_Any*)dest;
		//we need to malloc and do more copying
		deleteMallocedFieldValue(itype,dest);
		nele = mfs->n;
		if( sftype == FIELDTYPE_SFNode ) nele = (int) upper_power_of_two(nele); //upper power of 2 is a convention for children[] to solve a realloc memory fragmentation issue during parsing of extremely large and flat files
		mfd->p = malloc(sfsize*nele);
		mfd->n = mfs->n;
		ps = (char *)mfs->p;
		pd = (char *)mfd->p;
		for(i=0;i<mfs->n;i++)
		{
			medium_copy_field0(sftype,(union anyVrml*)ps,(union anyVrml*)pd);
			ps += sfsize;
			pd += sfsize;
		}

	}else{ 
		//isSF
		memcpy(dest,source,sfsize);
	}
} //return medium_copy_field
void medium_copy_field(int itype, void* source, void** dest){
	//void *myDestination = NULL;
	//medium_copy_field(itype,source,&myDestination);
	// it will malloc the size
	(*dest) = malloc(sizeofSForMF(itype));
	memset((*dest),0,sizeofSForMF(itype));
	medium_copy_field0(itype,source,(*dest));
}


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


//==============START OF DUKTAPE-SPECIFIC CODE====================
#include "duktape/duktape.h"

const char *duk_type_to_string(int duktype){
	const char* r = NULL;
	switch(duktype){
	case DUK_TYPE_NUMBER: r = "DUK_TYPE_NUMBER"; break;
	case DUK_TYPE_BOOLEAN:  r = "DUK_TYPE_BOOLEAN"; break;
	case DUK_TYPE_STRING:  r = "DUK_TYPE_STRING"; break;
	case DUK_TYPE_OBJECT: r =  "DUK_TYPE_OBJECT"; break;
	case DUK_TYPE_NONE:  r = "DUK_TYPE_NONE"; break;
	case DUK_TYPE_UNDEFINED:  r =  "DUK_TYPE_UNDEFINED"; break;
	case DUK_TYPE_NULL:  r =  "DUK_TYPE_NULL"; break;
	case DUK_TYPE_POINTER:  r = "DUK_TYPE_POINTER"; break;
	default:
		r = "UNKNOWN_TYPE";
		break;
	}
	return r;
}

void show_stack(duk_context *ctx, char* comment)
{
	int i, itop = duk_get_top(ctx);
	if(comment) printf("%s top=%d\n",comment,itop);
	//printf("%10s%10s%10s\n","position","type","more");
	printf("%10s%10s\n","position","type");
	for(i=0;i<itop;i++){
		int ipos = -(i+1);
		int t = duk_get_type(ctx, ipos);
		char *stype = NULL;
		const char * amore = "";
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

//Object virtualization via proxy objects: constructor, handlers (has,ownKeys,enumerate,get,set,deleteProp), finalizer

// >> PROXY CACHING FUNCTIONS 
// 2017 - lazy proxies have been too lazy, we created a new "Proxy" on ever fetch
// x and that meant if(ground == ground) would always be false if ground is a proxy
// x in js there's no proxy trap (function overload) just for the binary == scenario, 
// x and no way to override binary == operator
// - can do if(ground.valueOf() == ground.valueOf()) and over-ride valueOf (working now Jan 2017), 
//   x but that's unconventional syntax
// - duktape creator Sami says try caching your proxies
// - then (ground == ground) still won't be comparing x3d node addresses, 
//   but will return true because the proxy instances will be the same
// - that means per-context/ctx caching
// - and since we don't have a way to hook into javascript scope push and pop
//   we need to rely on finalizer for a place to remove a proxy from our cache / lookup table
//   Jan 11, 2017 proxy caching is working, now ground==ground and scenarios like 
//   val[1] == ground are true if they are supposed to be the same node
//   still some wasteful re-generation of proxies 
//   but within the scope of the == operator, its working, which is better than before caching
static Stack * proxycaches = NULL;
typedef struct cache_table_entry {
	duk_context *ctx;
	Stack *cache;
} cache_entry;
typedef struct proxy_cache_entry {
	struct X3D_Node *node;
	// native proxy
	// js proxy
	void *jsproxy;
} proxy_entry;

cache_entry * lookup_ctx_proxy_cache(duk_context *ctx){
	int i;
	cache_entry *ret = NULL;
	if(proxycaches == NULL){
		proxycaches = newStack(cache_entry *); //* so can NULL if/when script node deleted, without needing to pack
	}
	for(i=0;i<vectorSize(proxycaches);i++){
		cache_entry * ce = vector_get(cache_entry*,proxycaches,i);
		if(ce->ctx == ctx){
			ret = ce;
			break;	
		}
	}
	if(ret == NULL){
		cache_entry *ce = MALLOC(cache_entry*,sizeof(cache_entry));
		stack_push(cache_entry*,proxycaches,ce);
		ce->ctx = ctx;
		ce->cache = newStack(proxy_entry*); //* so can NULL in cfinalizer without needing to pack table
		ret = ce;
	}
	return ret;
}
proxy_entry *lookup_ctx_proxycache_entry_by_nodeptr(duk_context *ctx, struct X3D_Node *node){
	proxy_entry *ret = NULL;
	cache_entry* cache = lookup_ctx_proxy_cache(ctx);
	if(cache){
		int i;
		for(i=0;i<vectorSize(cache->cache);i++){
			proxy_entry *pe = vector_get(proxy_entry*,cache->cache,i);
			if(pe && pe->node == node){
				ret = pe;
			}
		}
	}
	return ret;
}
proxy_entry *add_ctx_proxycache_entry(duk_context *ctx, struct X3D_Node *node, void *jsproxy){
	int i;
	//assume we already verified it doesn't exist
	proxy_entry *ret = NULL;
	cache_entry *cache = lookup_ctx_proxy_cache(ctx);
	if(cache){
		proxy_entry *pe = MALLOC(proxy_entry*,sizeof(proxy_entry));
		pe->node = node;
		pe->jsproxy = jsproxy;
		int i, itarget;
		itarget = -1;
		for(i=0;i<vectorSize(cache->cache);i++){
			proxy_entry *pe0 = vector_get(proxy_entry*,cache->cache,i);
			if(pe0 == NULL){
				itarget = i; 
				vector_set(proxy_entry*,cache->cache,i,pe);
				ret = pe;
				break;
			}
		}
		if(itarget == -1){
			stack_push(proxy_entry*,cache->cache,pe);
			ret = pe;
		}
		if(0){
			printf("cache after add proxy\n");
			for(i=0;i<vectorSize(cache->cache);i++){
				proxy_entry *pe0 = vector_get(proxy_entry*,cache->cache,i);
				if(pe0)
					printf("%d %x %x\n",i,pe0->node,pe0->jsproxy);
				else
					printf("%d NULL\n",i);
			}
		}
	}
	return ret;
}
void remove_ctx_proxycache_entry_by_nodeptr(duk_context *ctx, struct X3D_Node *node){
	int i;
	//Q. is it dangerous / should we always remove by jsproxy* ?
	proxy_entry *ret = NULL;
	cache_entry *cache = lookup_ctx_proxy_cache(ctx);
	if(cache){
		int i;
		for(i=0;i<vectorSize(cache->cache);i++){
			proxy_entry *pe0 = vector_get(proxy_entry*,cache->cache,i);
			if(pe0 && pe0->node == node){
				vector_set(proxy_entry*,cache->cache,i,NULL);
				FREE_IF_NZ(pe0);
				break;
			}
		}
		if(0){
			printf("after cache clean\n");
			for(i=0;i<vectorSize(cache->cache);i++){
				proxy_entry *pe0 = vector_get(proxy_entry*,cache->cache,i);
				if(pe0)
					printf("%d %x %x\n",i,pe0->node,pe0->jsproxy);
				else
					printf("%d NULL\n",i);
			}
		}
	}
}
void remove_ctx_proxycache_entry_by_jsproxy(duk_context *ctx, void *jsproxy){
	int i;
	proxy_entry *ret = NULL;
	cache_entry *cache = lookup_ctx_proxy_cache(ctx);
	if(cache){
		int i;
		for(i=0;i<vectorSize(cache->cache);i++){
			proxy_entry *pe0 = vector_get(proxy_entry*,cache->cache,i);
			if(pe0 && pe0->jsproxy == jsproxy){
				vector_set(proxy_entry*,cache->cache,i,NULL);
				FREE_IF_NZ(pe0);
				break;
			}
		}
	}
}
//<< PROXY CACHING FUNCTIONS

int cfinalizer(duk_context *ctx){
	int rc, itype, igc;
	void *fwpointer = NULL;
	itype = igc = -1;
	rc = duk_get_prop_string(ctx,0,"fwItype");
	if(rc == 1) itype = duk_to_int(ctx,-1);
	duk_pop(ctx); //get prop string result
	rc = duk_get_prop_string(ctx,0,"fwGC");
	if(rc == 1) igc = duk_to_boolean(ctx,-1);
	duk_pop(ctx); //get prop string result
	rc = duk_get_prop_string(ctx,0,"fwField");
	if(rc == 1) fwpointer = duk_to_pointer(ctx,-1);
	duk_pop(ctx); //get prop string result


	//printf("hi from finalizer, itype=%s igc=%d p=%p\n",itype2string(itype),igc,fwpointer);
	if(itype == FIELDTYPE_SFNode && fwpointer){
		//2017 remove proxy from context cache
		//
		//a) lookup context cache
		//b) lookup node's proxy in context cache
		//c) remove
		struct X3D_Node *node = *(struct X3D_Node**)fwpointer;
		remove_ctx_proxycache_entry_by_nodeptr(ctx, node);
	}
	if(igc > 0 && itype > -1 && fwpointer){
		if(itype < AUXTYPE_X3DConstants){
			//FIELDS
			freeField(itype,fwpointer);   
		}else{
			//AUXTYPES
			free(fwpointer);
		}
	}
	return 0;
}

static int doingFinalizer = 1;
void push_typed_proxy(duk_context *ctx, int itype, void *fwpointer, int* valueChanged)
{
	//like push_typed_proxy2 except push this instead of push obj
	//int rc;
	proxy_entry *pe = NULL;
	if(itype == FIELDTYPE_SFNode){
		struct X3D_Node* node = *(struct X3D_Node**)fwpointer;
		//printf("pushtyped nodetype %d\n",node->_nodeType);
		pe = lookup_ctx_proxycache_entry_by_nodeptr(ctx, node);
	}
	if(pe){
		duk_push_heapptr(ctx, pe->jsproxy);
	}else{
		//show_stack(ctx,"push_typed_proxy start");
		duk_eval_string(ctx,"Proxy");
		duk_push_this(ctx);  //this
		duk_push_pointer(ctx,fwpointer);
		duk_put_prop_string(ctx,-2,"fwField");
		duk_push_pointer(ctx,valueChanged);
		duk_put_prop_string(ctx,-2,"fwChanged");
		duk_push_int(ctx,itype);
		duk_put_prop_string(ctx,-2,"fwItype");
		if(doingFinalizer){
			duk_push_boolean(ctx,TRUE);
			duk_put_prop_string(ctx,-2,"fwGC");
		}
		duk_eval_string(ctx,"handler");
		//show_stack(ctx,"push_typed_proxy should have Proxy, this, handler");

		duk_new(ctx,2); /* [ global Proxy target handler ] -> [ global result ] */
		//show_stack(ctx,"push_typed_proxy after new, proxy obj should be result???");

		if(doingFinalizer){
			//push_typed_proxy is called by constructor, that mallocs (via fwtype->constructor) and should GC
			//
			//Duktape.fin(a, function (x) {
			//       try {
			//           print('finalizer, foo ->', x.foo);
			//       } catch (e) {
			//           print('WARNING: finalizer failed (ignoring): ' + e);
			//       }
			//   });
			if(itype == FIELDTYPE_SFNode){
				struct X3D_Node* node = *(struct X3D_Node**)fwpointer;
				void *jsproxy = duk_get_heapptr(ctx, -1);
				add_ctx_proxycache_entry(ctx, node, jsproxy);
			}
			duk_eval_string(ctx,"Duktape.fin");
			duk_dup(ctx, -2); //copy the proxy object
			duk_push_c_function(ctx,cfinalizer,1);
			duk_pcall(ctx,2);
			duk_pop(ctx); //pop Duktape.fin result
		}
	}
}

int push_typed_proxy2(duk_context *ctx, int itype, int kind, void *fwpointer, int* valueChanged, char doGC)
{
	/*  like fwgetter version, except with no fieldname or mode, for temp proxies
		nativePtr
	*/
	//int rc;
	proxy_entry *pe = NULL;
	int idogc = doGC ? TRUE : FALSE;
	if(itype == FIELDTYPE_SFNode){
		struct X3D_Node* node = *(struct X3D_Node**)fwpointer;
		//printf("pushtyped2 nodetype %d\n",node->_nodeType);
		pe = lookup_ctx_proxycache_entry_by_nodeptr(ctx, node);
	}
	if(pe){
		duk_push_heapptr(ctx, pe->jsproxy);
	}else{
		duk_eval_string(ctx,"Proxy");
		duk_push_object(ctx);
		duk_push_pointer(ctx,fwpointer);
		duk_put_prop_string(ctx,-2,"fwField");
		duk_push_pointer(ctx,valueChanged);
		duk_put_prop_string(ctx,-2,"fwChanged");
		duk_push_int(ctx,itype);
		duk_put_prop_string(ctx,-2,"fwItype");
		duk_push_int(ctx,kind);
		duk_put_prop_string(ctx,-2,"fwKind");

		if(doingFinalizer) { // && idogc){
			duk_push_boolean(ctx,idogc);
			duk_put_prop_string(ctx,-2,"fwGC");
		}

		duk_eval_string(ctx,"handler");
		duk_new(ctx,2); /* [ global Proxy target handler ] -> [ global result ] */
		if(doingFinalizer) { // && idogc){
			//push_typed_proxy2 _refers_ to script->field[i]->anyVrml (its caller fwgetter doesn't malloc) and should not GC its pointer
			//
			//Duktape.fin(a, function (x) {
			//       try {
			//           print('finalizer, foo ->', x.foo);
			//       } catch (e) {
			//           print('WARNING: finalizer failed (ignoring): ' + e);
			//       }
			//   });
			if(itype == FIELDTYPE_SFNode){
				struct X3D_Node* node = *(struct X3D_Node**)fwpointer;
				void *jsproxy = duk_get_heapptr(ctx, -1);
				add_ctx_proxycache_entry(ctx, node, jsproxy);
			}

			duk_eval_string(ctx,"Duktape.fin");
			duk_dup(ctx, -2); //copy the proxy object
			duk_push_c_function(ctx,cfinalizer,1);
			duk_pcall(ctx,2);
			duk_pop(ctx); //pop Duktape.fin result
		}
	}

	return 1;
}



void convert_duk_to_fwvals(duk_context *ctx, int nargs, int istack, struct ArgListType arglist, FWval *args, int *argc){
	int nUsable,nNeeded, i, ii;
	FWval pars;
	//struct Uni_String *uni;
	nUsable = arglist.iVarArgStartsAt > -1 ? nargs : arglist.nfixedArg;
	nNeeded = max(nUsable,arglist.nfixedArg);
	pars = malloc(nNeeded*sizeof(FWVAL));
	(*args) = pars;
	//QC and genericization of incoming parameters
	(*argc) = nNeeded;
	for(i=0;i<nUsable;i++){
		//const char* str;
		char ctype;
		ii = istack + i;
		if(i < arglist.nfixedArg) 
			ctype = arglist.argtypes[i];
		else 
			ctype = arglist.argtypes[arglist.iVarArgStartsAt];
		pars[i].itype = ctype;
		if( duk_is_object(ctx, ii)){
			int rc, isPrimitive;
			//if the script goes myField = new String('hi'); then it comes in here as an object (versus myField = 'hi'; which is a string)
			rc = duk_get_prop_string(ctx,ii,"fwItype");
			duk_pop(ctx);
			isPrimitive = rc == 0;
			if(isPrimitive){
				//void duk_to_primitive(duk_context *ctx, duk_idx_t index, duk_int_t hint); DUK_HINT_NONE
				//http://www.duktape.org/api.html#duk_to_primitive
				duk_to_primitive(ctx,ii,DUK_HINT_NONE);
			}
		}
		switch(ctype){
		case 'B': {
			int bb = duk_get_boolean(ctx,ii); //duk_to_boolean(ctx,ii);
			pars[i]._boolean = bb; // duk_to_boolean(ctx,ii); 
			}
			break;
		case 'I': pars[i]._integer = duk_to_int(ctx,ii); break;
		case 'F': pars[i]._numeric = duk_to_number(ctx,ii); break;
		case 'D': pars[i]._numeric = duk_to_number(ctx,ii); break;
		case 'S': pars[i]._string = duk_to_string(ctx,ii); break;
		case 'Z': //flexi-string idea - allow either String or MFString (no such thing as SFString from ecma - it uses String for that)
			if(duk_is_string(ctx,ii)){
				pars[i]._string = duk_get_string(ctx,ii); 
				pars[i].itype = 'S';
				break;
			}
			if(!duk_is_object(ctx,i))
				break;
			//else fall through to W
		case 'W': {
				int rc, isOK, itypeRHS = -1;
				union anyVrml *fieldRHS = NULL;
				rc = duk_get_prop_string(ctx,ii,"fwItype");
				if(rc == 1){
					itypeRHS = duk_to_int(ctx,-1);
				}
				duk_pop(ctx);
				rc = duk_get_prop_string(ctx,ii,"fwField");
				if(rc == 1) fieldRHS = duk_to_pointer(ctx,-1);
				duk_pop(ctx);
				/*we don't need the RHS fwChanged=valueChanged* because we are only changing the LHS*/
				isOK = FALSE;
				if(fieldRHS != NULL && itypeRHS > -1){
					// its one of our proxy field types. But is it the type we need?
					//medium_copy_field(itypeRHS,fieldRHS,&pars[i]._web3dval.native); //medium copy - copies p[] in MF types but not deep copy *(p[i]) if p[i] is pointer type ie SFNode* or Uni_String*
					pars[i]._web3dval.native = fieldRHS;
					pars[i]._web3dval.fieldType = itypeRHS;
					pars[i].itype = 'W';
					// see below *valueChanged = TRUE;
					isOK = TRUE;
				}
			}
			break;
		case 'P': {
				int rc, isOK, itypeRHS = -1;
				union anyVrml *fieldRHS = NULL;
				rc = duk_get_prop_string(ctx,ii,"fwItype");
				if(rc == 1){
					//printf(duk_type_to_string(duk_get_type(ctx, -1)));
					itypeRHS = duk_to_int(ctx,-1);
				}
				duk_pop(ctx);
				rc = duk_get_prop_string(ctx,ii,"fwField");
				if(rc == 1) fieldRHS = duk_to_pointer(ctx,-1);
				duk_pop(ctx);
				/*we don't need the RHS fwChanged=valueChanged* because we are only changing the LHS*/
				isOK = FALSE;
				if(fieldRHS != NULL && itypeRHS >= AUXTYPE_X3DConstants){
					/* its one of our auxiliary types - Browser, X3DConstants, ProfileInfo, ComponentInfo, X3DRoute ...*/
					pars[i]._pointer.native = fieldRHS;
					pars[i]._pointer.fieldType = itypeRHS;
					pars[i].itype = 'P';
					// see below *valueChanged = TRUE;
					isOK = TRUE;
				}
			}
			break;

		case 'O': break; //object pointer ie to js function callback object
		}
	}
		
	for(i=nUsable;i<nNeeded;i++){
		//fill
		char ctype = arglist.argtypes[i];
		pars[i].itype = ctype;
		switch(ctype){
		case 'B': pars[i]._boolean = FALSE; break;
		case 'I': pars[i]._integer = 0; break;
		case 'F': pars[i]._numeric = 0.0; break;
		case 'D': pars[i]._numeric = 0.0; break;
		case 'S': pars[i]._string = NULL; break;
		case 'Z': pars[i]._string = NULL; pars[i].itype = 'S'; break;
		case 'W': 
			pars[i]._web3dval.fieldType = FIELDTYPE_SFNode; 
			pars[i]._web3dval.native = NULL; break;
		//case 'P': 
		//	pars[i]._web3dval.fieldType = FIELDTYPE_SFNode; //I don't have a good default value - do I need an AUXTYPE_NULL?
		//	pars[i]._web3dval.native = NULL; break;
		case 'O': 
			pars[i]._jsobject = NULL; break; 
		default:
			pars[i].itype = '0';
		}
	}
}


int cfwconstructor(duk_context *ctx) {
	int i, j, rc, nargs, argc, ifound;
	FWTYPE *fwt;
	FWval args;
	void *fwpointer;
	int *valueChanged = NULL; //so called 'internal' variables inside the script context don't point to a valueChanged
	int itype = -1;
	nargs = duk_get_top(ctx);

	//show_stack(ctx,"cfwconstructor start");

	duk_push_current_function(ctx);
	rc = duk_get_prop_string(ctx,-1,"fwItype");
	if(rc == 1) itype = duk_to_int(ctx,-1);
	duk_pop(ctx); //get prop string result
	duk_pop(ctx); //current function

	//show_stack(ctx,"cfwconstructor after push and pop current function");

	if(itype < 0) return 0; //no itype means it's not one of ours
	fwt = getFWTYPE(itype);
	if(!fwt->Constructor) return 0; ///AUXTYPE_s not constructable (except route?)

	//find the contructor that matches the args best
	i = 0;
	ifound = -1;
	while(fwt->ConstructorArgs[i].nfixedArg > -1){
		int nfixed = fwt->ConstructorArgs[i].nfixedArg;
		int ivarsa = fwt->ConstructorArgs[i].iVarArgStartsAt;
		char *neededTypes = fwt->ConstructorArgs[i].argtypes;
		int fill = fwt->ConstructorArgs[i].fillMissingFixedWithZero == 'T';
		if( nargs == nfixed || (ivarsa > -1 && nargs >= nfixed ) || (ivarsa > -1 && fill)){ 
			//nargs is a match
			int allOK = TRUE;
			//check each narg for compatible type
			for(j=0;j<nargs;j++){
				char neededType;
				int isOK, RHS_duk_type = duk_get_type(ctx, j);
				isOK = FALSE;
				neededType = j >= nfixed ? neededTypes[ivarsa] : neededTypes[j]; //if you have varargs you specify one more type than the fixed requires
				// for example MFColor nfixed=0 (you can have 0 to infinity args), ivarsa=0 (varargs start at index 0), neededTypes="W" the first and subsequent varargs are of type 'W'
				//printf("duktype %s\n",duk_type_to_string(RHS_duk_type));
				switch(RHS_duk_type){
				case DUK_TYPE_NUMBER: 
					if(neededType =='F' || neededType =='D' || neededType =='I') isOK = TRUE;
					break;
				case DUK_TYPE_BOOLEAN: 
					if(neededType =='B') isOK = TRUE;
					break;
				case DUK_TYPE_STRING:
					if(neededType =='S' || neededType =='Z') isOK = TRUE;
					break;
				case DUK_TYPE_OBJECT:
					if(neededType =='W' || neededType =='P'){
						int rc, itypeRHS = -1;
						union anyVrml *fieldRHS = NULL;
						rc = duk_get_prop_string(ctx,j,"fwItype");
						if(rc == 1){
							//printf(duk_type_to_string(duk_get_type(ctx, -1)));
							itypeRHS = duk_to_int(ctx,-1);
						}
						duk_pop(ctx);
						rc = duk_get_prop_string(ctx,j,"fwField");
						if(rc == 1) fieldRHS = duk_to_pointer(ctx,-1);
						duk_pop(ctx);
						//we don't need the RHS fwChanged=valueChanged* because we are only changing the LHS

						if(fieldRHS != NULL && itypeRHS > -1){
							//in theory, we could make sure somehow that we had the right kind of 'W' : add a FIELDTYPE_ / AUXTYPE_ array in arglist struct
							isOK = TRUE;
						}
					}
					break;
				case DUK_TYPE_NONE: 
				case DUK_TYPE_UNDEFINED: 
				case DUK_TYPE_NULL: 
					// are we attempting to null out the field? we aren't allowed to change its type (to undefined) 
				case DUK_TYPE_POINTER: 
					// don't know what this would be for if anything 
				default:
					isOK = FALSE;
					break;
				}
				allOK = allOK && isOK;
			}
			if(fill)
				for(j=nargs;j<nfixed;j++){
					allOK = allOK && 1;
				}
			if(allOK){
				ifound = i;
				break;
			}
		}
		i++;
	}
	if(ifound < 0){
		//printf("matching constructor not found, you have %d args for %s\n",nargs,fwt->name);
		//Jan 2016 if you're in here, and your Script did new MFString(number,string) -heterogenous call args-
		//.. then I think the problem is the nested loops above are inside-out. MFString constructor
		//.. should be able to handle heterogenous args, and if constructor.args was the inner loop and
		//.. call args the outer loop, allOK would be true:
		//.. it would find the (only) constructor is a match.
		//.. don't have time to code-review, try and test this theory thoroughly today
		//.. temproary fix in your script: new MFString(number.toString(),string) to make args homogenous.
		//return 0;
		i = 0; //take the first one
	}
	args = NULL;
	convert_duk_to_fwvals(ctx, nargs, 0, fwt->ConstructorArgs[i], &args, &argc);
	if(fwt->ConstructorArgs[ifound].fillMissingFixedWithZero == 'T' && nargs < fwt->ConstructorArgs[ifound].nfixedArg){
		int nfixed = fwt->ConstructorArgs[ifound].nfixedArg;
		//int ivarsa = fwt->ConstructorArgs[ifound].iVarArgStartsAt;
		char *neededTypes = fwt->ConstructorArgs[ifound].argtypes;
		//int fill = fwt->ConstructorArgs[ifound].fillMissingFixedWithZero == 'T';
		args = realloc(args,nfixed * sizeof(FWVAL));
		for(j=nargs;j<nfixed;j++){
			switch(neededTypes[j]){
			case 'B':
				args[j]._boolean = FALSE; break;
			case 'I':
				args[j]._integer = 0; break;
			case 'F':
				args[j]._numeric = 0.0; break;
			case 'D':
				args[j]._numeric = 0.0; break;
			case 'S':
				args[j]._string = ""; break;
			case 'W':
			case 'P':
				break;
			}
		}
		argc = nfixed;
	}

	fwpointer = fwt->Constructor(fwt,argc,args);
	free(args);
	push_typed_proxy(ctx,itype, fwpointer, valueChanged);

	return 1;
}
int chas(duk_context *ctx) {
	int rc, itype, *valueChanged;
	const char *key;
	int nr, index;
	char type, readOnly;
	FWTYPE *fwt;
	union anyVrml *parent = NULL;

	itype = 0;
	/* get type of parent object for this property*/
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
	key = duk_require_string(ctx,-1);
	//printf("key=%s\n",key);

	nr = 1;
	fwt = getFWTYPE(itype);
	if(fwhas_generic(fwt,parent,key,&index,&type,&readOnly)){
		duk_push_true(ctx);
	}else{
		duk_push_false(ctx);
	}
	//isFunc = type == 'f';
	//show_stack(ctx,"in chas");

    return nr;
}
int cownKeys(duk_context *ctx) {
	int rc, itype, *valueChanged, arr_idx;
	void *parent = NULL;
	int i;
	char *fieldname;
	int lastProp, jndex; //isFunc, 
	char type, readOnly;
	//FWTYPE *getFWTYPE(int itype)
	FWTYPE *fwt;
	itype = -1;

	/* get type of parent object for this property*/
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

	arr_idx = duk_push_array(ctx);
	if(itype < 0 || (itype < AUXTYPE_X3DConstants && parent == NULL))
		return 1; //return empty array
	i = -1;
	fwt = getFWTYPE(itype);
	//fwiterator_generic(int index, FWTYPE *fwt, FWPointer *pointer, char **name, int *lastProp, int *jndex)
	while( (i = fwiterator_generic(i,fwt,parent,&fieldname,&lastProp,&jndex,&type,&readOnly)) > -1 ){
		duk_push_string(ctx, fieldname);
		duk_put_prop_index(ctx, arr_idx, i);
	}
	//show_stack(ctx,"in cownKeys");
    return 1;
}
int cenumerate(duk_context *ctx) {
	int rc, itype, *valueChanged;
	union anyVrml *parent = NULL;
	int i;
	char *fieldname;
	int lastProp, jndex; //isFunc, 
	char type, readOnly;
	FWTYPE *fwt;
	int arr_idx;

	itype =0;
	/* get type of parent object for this property*/
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

	arr_idx = duk_push_array(ctx);
	i = -1;
	fwt = getFWTYPE(itype);
	//fwiterator_generic(int index, FWTYPE *fwt, FWPointer *pointer, char **name, int *lastProp, int *jndex)
	while( (i = fwiterator_generic(i,fwt,parent,&fieldname,&lastProp,&jndex,&type,&readOnly)) > -1 ){
		//isFunc = i > lastProp;
		duk_push_string(ctx, fieldname);
		duk_put_prop_index(ctx, arr_idx, i);
	}
	//show_stack(ctx,"in cenumerate");
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
	//int isOK = FALSE;
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

static int SCALARS_ARE_PRIMITIVES = TRUE;
/* SCALARS_ARE_PRIMITIVES
   the ecmascript ! operator invokes ToBoolean() which always returns true when the argument is an object
   http://www.ecma-international.org/ecma-262/5.1/#sec-11.4.9
   http://www.ecma-international.org/ecma-262/5.1/#sec-9.2
   the web3d.org ecmascript specs say all fields shall have getType(), isReadable(), isWritable() functions.
   if I have: Script {
	field SFBool enabled TRUE 
	url "ecmascript: function initialize(){
		var A = !enabled;			//A returns false if enabled is a primitive and its value is true, 
									//but A always returns false if enabled is a proxy object
		var B = enabled.getType();	//eval fails with 'type error, not an object' if enabled is a primitive,
									//but B returns X3DConstants.SFBool if enabled is a proxy object
	Because there are some goodies either way, and I'm not sure what the specs intend, I've made it configurable for now,
	although comparisons with vivaty are closer to SCALARS_ARE_PRIMITIVES = TRUE (some scenes fail with FALSE).
*/
int fwval_duk_push(duk_context *ctx, FWval fwretval, int *valueChanged){
	//converts engine-agnostic FWVAL return value to duk engine specific return values and pushes them onto the duk value stack
	int nr = 1;
	switch(fwretval->itype){
	
	case 'B':
		duk_push_boolean(ctx,fwretval->_boolean); break;
	case 'I':
		duk_push_int(ctx,fwretval->_integer); break;
	case 'F':
		duk_push_number(ctx,fwretval->_numeric); break;
	case 'D':
		duk_push_number(ctx,fwretval->_numeric); break;
	case 'S':
		duk_push_string(ctx,fwretval->_string); break;
	
	case 'W':
		if(SCALARS_ARE_PRIMITIVES){
			//for pointers to web3d field types
			switch(fwretval->_web3dval.fieldType){
			case FIELDTYPE_SFBool:
				duk_push_boolean(ctx,fwretval->_web3dval.anyvrml->sfbool); break;
			case FIELDTYPE_SFInt32:
				duk_push_int(ctx,fwretval->_web3dval.anyvrml->sfint32); break;
			case FIELDTYPE_SFFloat:
				duk_push_number(ctx,(double)fwretval->_web3dval.anyvrml->sffloat); break;
			case FIELDTYPE_SFDouble:
			case FIELDTYPE_SFTime:
				duk_push_number(ctx,fwretval->_web3dval.anyvrml->sfdouble); break;
			case FIELDTYPE_SFString:
				duk_push_string(ctx,fwretval->_web3dval.anyvrml->sfstring->strptr); break;
			default:
				push_typed_proxy2(ctx,fwretval->_web3dval.fieldType,fwretval->_web3dval.kind,fwretval->_web3dval.native,valueChanged,fwretval->_web3dval.gc);
			}
		}else{
			//SCALARS_ARE_PROXY_OBJECTS
			push_typed_proxy2(ctx,fwretval->_web3dval.fieldType,fwretval->_web3dval.kind,fwretval->_web3dval.native,valueChanged,fwretval->_web3dval.gc);
		}
		break;
	case 'X':
		duk_push_pointer(ctx,fwretval->_jsobject);
		break;
	case 'P':
		//for web3d auxiliary types Browser, X3DFieldDefinitionArray, X3DRoute ...
		push_typed_proxy2(ctx,fwretval->_pointer.fieldType,fwretval->_pointer.kind,fwretval->_pointer.native,valueChanged,fwretval->_pointer.gc);
		break;
	case '0':
	default:
		nr = 0; break;
	}
	return nr;
}

int ctypefunction(duk_context *ctx) {
	int rc, nr, itype, kind, nargs;
	const char *fwFunc = NULL;
	//union anyVrml* field = NULL;
	//FWTYPE *fwt;

	itype = -1;
	kind = -1;
	nargs = duk_get_top(ctx);
	//show_stack(ctx,"in cfuction");
	duk_push_current_function(ctx);
	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,-1,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	/*get the PKW_inputOutput read/write mode for the parent field*/
	rc = duk_get_prop_string(ctx,-1,"fwKind");
	if(rc==1) kind = duk_get_int(ctx,-1);
	duk_pop(ctx);
	/* get the name of the function called */
	rc = duk_get_prop_string(ctx,-1,"fwFunc");
	if(rc == 1) fwFunc = duk_to_string(ctx,-1);
	duk_pop(ctx);
	duk_pop(ctx); //durrent function
	nr = 0;
	if(!strcasecmp(fwFunc,"getType")){
		duk_push_int(ctx,itype);
		nr = 1;
	}
	if(!strcmp(fwFunc,"isReadable")){
		int isreadable = TRUE;
		if(kind > -1)
			isreadable = isreadable && (kind == PKW_inputOutput || kind == PKW_initializeOnly);
		if(isreadable) duk_push_true(ctx);
		else duk_push_false(ctx);
		nr = 1;
	}
	if(!strcmp(fwFunc,"isWritable")){
		int iswritable = TRUE;
		if(kind > -1)
			iswritable = iswritable && (kind == PKW_inputOutput || kind == PKW_outputOnly);
		if(iswritable) duk_push_true(ctx);
		else duk_push_false(ctx);
		nr = 1;
	}
	return nr;
}
int cfunction(duk_context *ctx) {
	int rc, nr, itype, nargs, *valueChanged = NULL;
	const char *fwFunc = NULL;
	union anyVrml* parent = NULL;
	//union anyVrml* field = NULL;
	FWTYPE *fwt;
	FWFunctionSpec *fs;

	itype = 0;
	nargs = duk_get_top(ctx);
	//show_stack(ctx,"in cfuction");
	duk_push_current_function(ctx);
	/* get type of parent object for this property*/
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

	nr = 0;

	fwt = getFWTYPE(itype);
	//check functions - if its a function push the type's specfic function
	fs = getFWFunc(fwt,fwFunc);
	if(fs){
		FWval pars;
		int argc;
		FWVAL fwretval;
		struct X3D_Node *scriptnode;
		void *ec = NULL;
		convert_duk_to_fwvals(ctx, nargs, 0, fs->arglist, &pars, &argc);
		//the object function call, using engine-agnostic parameters
		
		//>>just SFNode function getNodeName needs to know the script node context (it can't use its own - it may be an IMPORT)
		duk_eval_string(ctx,"__script");
		scriptnode = (struct X3D_Node*) duk_to_pointer(ctx,-1);
		duk_pop(ctx);
		if(scriptnode)
			ec = (void *)scriptnode->_executionContext;
		//<<
		nr = fs->call(fwt,ec,parent,argc,pars,&fwretval);
		if(nr){
			nr = fwval_duk_push(ctx,&fwretval,valueChanged);
			if(nr && !strcasecmp(fwFunc,"toString")){
				if(fwretval.itype == 'S' && fwretval._string){
					//printf("gcing toString string %s\n",fwretval._string);
					free(fwretval._string);  //if this bombs take it out and toString strings won't be gcd. There's nothing set up to gc _string in general
				}
			}
		}else{
			if(valueChanged) *valueChanged = TRUE;
		}
		free(pars);
	}
	return nr;
}
int cget(duk_context *ctx) {
	int rc, nr, itype, kind, *valueChanged = NULL;
	//show_stack(ctx,"in cget");
	union anyVrml* parent = NULL;
	//union anyVrml* field = NULL;

	/* get type of parent object for this property*/
	itype = -1;
	kind = -1;
	rc = duk_get_prop_string(ctx,0,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	/* get the kind of parent field PKW_inputOutput etc*/
	rc = duk_get_prop_string(ctx,0,"fwKind");
	if(rc==1) kind = duk_get_int(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the parent object */
	rc = duk_get_prop_string(ctx,0,"fwField");
	if(rc == 1) parent = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	/* get the pointer to the changed flag */
	rc = duk_get_prop_string(ctx,0,"fwChanged");
	if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
	duk_pop(ctx);
	//show_stack(ctx,"in cget");

	nr = 0;
	//printf("indexer is%s\n",duk_type_to_string(duk_get_type(ctx,-2)));
	switch(duk_get_type(ctx,-2)){
	case DUK_TYPE_NUMBER:{
		//int ikey = duk_get_int(ctx,-2);
		//printf("key=[%d]",ikey);
		}
		break;
	default: {
		const char *key = duk_require_string(ctx,-2);
		//printf("key=%s \n",key);
		if(!strcmp(key,"fwItype")){
			//someone else is asking a proxy for its fwItype (for example LHS = RHSProxy) the LHS Setter may want the RHS's fwItype
			duk_push_int(ctx,itype);
			nr = 1;
			return nr;
		}
		if(!strcmp(key,"fwGC")){
			//someone else is asking a proxy for its fwGC (for example LHS = RHSProxy) 
			//if there's no fwGC already on it, then the answer is FALSE
			duk_push_boolean(ctx,FALSE);
			nr = 1;
			return nr;
		}
		if(!strcmp(key,"fwField")){
			//someone is asking a proxy for its fwField
			duk_push_pointer(ctx,parent);
			nr = 1;
			return nr;
		}
		if(!strcasecmp(key,"getType") || !strcmp(key,"isReadable") || !strcmp(key,"isWritable")){
			//its a function all auxtypes and fieldtypes share
			duk_push_c_function(ctx,ctypefunction,DUK_VARARGS);
			duk_push_int(ctx,itype);
			duk_put_prop_string(ctx,-2,"fwItype");
			duk_push_int(ctx,kind);
			duk_put_prop_string(ctx,-2,"fwKind");
			duk_push_string(ctx,key);
			duk_put_prop_string(ctx,-2,"fwFunc");
			nr = 1;
			return nr;
		}
		}
		break;
	}


	if(itype > -1){
		//itype is in AUXTYPE_ range
		const char *key = NULL;// = duk_require_string(ctx,-2);
		FWTYPE *fwt = getFWTYPE(itype);
		int jndex, found;
		char type, readOnly;
		found = 0;

		//check numeric indexer
		if(duk_is_number(ctx,-2)){
			//indexer
			int index = duk_get_int(ctx,-2);
			if(fwt->takesIndexer){
				type = fwt->takesIndexer;
				readOnly = fwt->indexerReadOnly;
				jndex = index;
				found = 1;
			}else{
				//script is attempting to iterate over/get properties by number to get value - good luck
				const char *name;
				int lastProp;
				index = fwiterator_generic(index -1,fwt,parent,&name,&lastProp,&jndex,&type,&readOnly);
				if(index > -1) found = 1;
			}
		}else{
			//check properties - if a property, call the type-specific setter
			//int lastProp;
			key = duk_get_string(ctx,-2);
			found = fwhas_generic(fwt,parent,key,&jndex,&type,&readOnly);
			if(!found){
				static int once = 0;
				if(!once)
					ConsoleMessage("type %s has no property or function %s - please check your typing\n",fwt->name,key);
				once = 1;
			}
		}
		if(found && type=='f'){
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
				duk_push_string(ctx,key);
				duk_put_prop_string(ctx,-2,"fwFunc");
				nr = 1;
			}
		}else if(found && fwt->Getter){
			FWVAL fwretval;
			struct X3D_Node *scriptnode;
			void *ec = NULL;
			//>>just SFNode function getNodeName needs to know the script node context (it can't use its own - it may be an IMPORT)
			duk_eval_string(ctx,"__script");
			scriptnode = (struct X3D_Node*) duk_to_pointer(ctx,-1);
			duk_pop(ctx);
			if(scriptnode)
				ec = (void *)scriptnode->_executionContext;
			//<<

			nr = fwt->Getter(fwt,jndex,ec,parent,&fwretval);
			if(nr){
				nr = fwval_duk_push(ctx,&fwretval,valueChanged);
			}
		}
	}
    return nr;
}
int cset(duk_context *ctx) {
	int rc, itype, *valueChanged = NULL; 
	union anyVrml *parent = NULL;
	itype = -1;
	/* get type of parent object for this property*/
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


	switch(duk_get_type(ctx,-3)){
	case DUK_TYPE_NUMBER:{
		//int ikey = duk_get_int(ctx,-3);
		//printf("key=[%d] ",ikey);
		}
		break;
	default: {
		//const char *key = duk_require_string(ctx,-3);
		//printf("key=%s ",key);
		}
		break;
	}
	switch(duk_get_type(ctx,-2)){
	case DUK_TYPE_NUMBER:{
		int ival = duk_get_int(ctx,-2);
		//printf("val=[%d]\n",ival);
		}
		break;
	case DUK_TYPE_STRING:{
		const char *cval = duk_get_string(ctx,-2);
		//printf("val=%s\n",cval);
		}
		break;
	default: 
		//printf("val is object\n");
		break;
	}


	if(itype > -1) {
		//itype is in FIELDTYPE_ and AUXTYPE_ range
		const char* key;
		FWTYPE *fwt = getFWTYPE(itype);
		int jndex, found;
		char type, readOnly;
		//check numeric indexer
		if(duk_is_number(ctx,-3) && fwt->takesIndexer){
			//indexer
			jndex = duk_get_int(ctx,-3);
			type = fwt->takesIndexer;
			readOnly = fwt->indexerReadOnly;
			found = 1;
		}else{
			//check properties - if a property, call the type-specific setter
			//int lastProp;
			key = duk_get_string(ctx,-3);
			found = fwhas_generic(fwt,parent,key,&jndex,&type,&readOnly) && (type != 'f');
		}
		if(found && (readOnly != 'T') && fwt->Setter){
			FWval fwsetval = NULL;
			struct ArgListType arglist;
			int argc;
			arglist.argtypes = &type;
			arglist.fillMissingFixedWithZero = 0;
			arglist.nfixedArg = 1;
			arglist.iVarArgStartsAt = -1;
			convert_duk_to_fwvals(ctx, 1, -2, arglist, &fwsetval, &argc);
			if(argc == 1){
				struct X3D_Node *scriptnode;
				void *ec = NULL;
				//>>just SFNode function getNodeName needs to know the script node context (it can't use its own - it may be an IMPORT)
				duk_eval_string(ctx,"__script");
				scriptnode = (struct X3D_Node*) duk_to_pointer(ctx,-1);
				duk_pop(ctx);
				if(scriptnode)
					ec = (void *)scriptnode->_executionContext;
				//<<

				fwt->Setter(fwt,jndex,ec,parent,fwsetval);
				if(valueChanged)
					(*valueChanged) = 1;
			}
			free(fwsetval);
		}
	}
    return 0;
}
int cdel(duk_context *ctx) {
	int rc, itype, *valueChanged;
	union anyVrml *parent;

	/* get type of parent object for this property*/
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

	show_stack(ctx,"in cdel");
	//duk_push_string(ctx, nativeValue);
    return 1;
}

//c-side helper adds the generic handler to global, for use when creating each proxy
void addHandler(duk_context *ctx){
	int iglobal, ihandler; // , rc;
	iglobal = duk_get_top(ctx) -1;

	duk_push_object(ctx);
	duk_put_prop_string(ctx, iglobal, "handler");

	duk_get_prop_string(ctx,iglobal,"handler"); //get handler from global
	ihandler = duk_get_top(ctx) -1; //+ve
	duk_push_c_function(ctx,chas,2);
	duk_put_prop_string(ctx, ihandler, "has");
	duk_push_c_function(ctx,cownKeys,1);
	duk_put_prop_string(ctx, ihandler, "ownKeys");
	duk_push_c_function(ctx,cenumerate,1);
	duk_put_prop_string(ctx, ihandler, "enumerate");
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
	int itype;
	duk_push_c_function(ctx,cfwconstructor,DUK_VARARGS);
	//put fname=SFVec3f on c_function, so in constructor we can tell what we are trying to construct
	itype = fwType2itype(typeName);
	duk_push_int(ctx,itype);
	duk_put_prop_string(ctx,-2,"fwItype");
	//put SFVec3f = c_fuction on global
	duk_put_prop_string(ctx,iglobal,typeName);
}
void add_duk_global_property(duk_context *ctx, int itype, const char *fieldname, int *valueChanged, struct X3D_Node *node);


/* www.duktape.org javascript engine used here */
//A DUK helper function
static char *eval_string_defineAccessor = "\
function defineAccessor(obj, key, set, get) { \
    Object.defineProperty(obj, key, { \
        enumerable: true, configurable: true, \
        set: set, get: get \
    }); \
}";



/* create the script context for this script. This is called from the thread
   that handles script calling in the fwl_RenderSceneUpdateScene */
void JSCreateScriptContext(int num) {
	int i, iglobal; // , rc;
	//jsval rval;
	duk_context *ctx; 	/* these are set here */
	struct Shader_Script *script;
	struct X3D_Node *scriptnode;
	//JSObject *_globalObj; 	/* these are set here */
	//BrowserNative *br; 	/* these are set here */
	//ppJScript p = (ppJScript)gglobal()->JScript.prv;
	struct CRscriptStruct *ScriptControl; // = getScriptControl();

	ScriptControl = getScriptControlIndex(num);
	script = ScriptControl->script;
	scriptnode = script->ShaderScriptNode;
	//CREATE CONTEXT
	ctx = duk_create_heap_default();

	//ADD STANDARD JS GLOBAL OBJECT/CLASSES
    duk_push_global_object(ctx);
	iglobal = duk_get_top(ctx) -1;

	//SAVE OUR CONTEXT IN OUR PROGRAM'S SCRIPT NODE FOR LATER RE-USE
	ScriptControl->cx =  ctx;
	//ScriptControl[num].glob =  (void *)malloc(sizeof(int)); 
	//*((int *)ScriptControl[num].glob) = iglobal; //we'll be careful not to pop our global for this context (till context cleanup)
	((int *)&ScriptControl->glob)[0] = iglobal; //we'll be careful not to pop our global for this context (till context cleanup)

	//ADD HELPER PROPS AND FUNCTIONS
	duk_push_pointer(ctx,scriptnode); //I don't think we need to know the script this way, but in the future, you might
	duk_put_prop_string(ctx,iglobal,"__script"); //oct 2014 the future arrived. sfnode.getNodeName needs the DEFnames from the broto context, and script seems to know its broto context

	duk_push_string(ctx,eval_string_defineAccessor);
	duk_eval(ctx);
	duk_pop(ctx);

	//ADD CUSTOM TYPES - Browser, X3DConstants, web3d field types 
	addHandler(ctx); //add helper called handler, to global object
	//add types that can be newed ie var a = new SFVec3f();
	//  they will have a non-null constructor function
	//  generally, it's all our SF and MF field types
	for(i=0;i<FWTYPES_COUNT;i++)
		if(fwtypesArray[i]->Constructor)
			addCustomProxyType(ctx,iglobal,fwtypesArray[i]->name);
	//show_stack(ctx,"before adding Browser");
	//add static singltons on global object ie global.Browser global.X3DConstants
	add_duk_global_property(ctx, AUXTYPE_X3DBrowser, "Browser", NULL, NULL);
	add_duk_global_property(ctx, AUXTYPE_X3DConstants,"X3DConstants", NULL, NULL);
	//add Global methods and defines for VMRL/X3D (some redirecting to the Browser object ie print = Browser.println)
	duk_eval_string(ctx,DefaultScriptMethodsA);
	duk_pop(ctx);
	duk_eval_string(ctx,DefaultScriptMethodsB);
	duk_pop(ctx);

	/* send this data over to the routing table functions. */
	CRoutes_js_new (num, JAVASCRIPT);

	//tests, if something is broken these tests might help
	if(1){
		void *scriptnode;
		struct X3D_Node *snode;
		//duk_eval_string(ctx,"print('this.__script='+this.__script);"); //checks the NodeScript availability
		//duk_pop(ctx);
		duk_eval_string(ctx,"__script");
		scriptnode = duk_to_pointer(ctx,-1);
		duk_pop(ctx);
		snode = (struct X3D_Node *)scriptnode;
		//printf("script node = %p",scriptnode);
	}
	if(0){
		duk_eval_string(ctx,"print(Object.keys(Browser));"); //invokes ownKeys
		duk_pop(ctx);
		duk_eval_string(ctx,"print(Object.getOwnPropertyNames(Browser));"); //invokes ownKeys
		duk_pop(ctx);
		duk_eval_string(ctx,"for (k in Browser) {print(k);}"); //invokes enumerate
		duk_pop(ctx);
		duk_eval_string(ctx,"if('println' in Browser) print('have println'); else print('no println');"); //invokes has
		duk_pop(ctx);
		duk_eval_string(ctx,"print('X3DConstants.outputOnly='); print(X3DConstants.outputOnly);"); //invokes custom iterator in generic has
		duk_pop(ctx);
		duk_eval_string(ctx,"print(Object.keys(X3DConstants));"); //invokes custom iterator in ownKeys
		duk_pop(ctx);
	}
	if(0){
		duk_eval_string(ctx,"Browser.println('hi from brwsr.println');");
		duk_pop(ctx);
		duk_eval_string(ctx,"Browser.description = 'funny description happened on the way to ..';");
		duk_pop(ctx);
		duk_eval_string(ctx,"Browser.println(Browser.description);");
		duk_pop(ctx);
		duk_eval_string(ctx,"print('hi from print');");
		duk_pop(ctx);
		duk_eval_string(ctx,"print(Browser.version);");
		duk_pop(ctx);

	}
	if(0){
		duk_eval_string(ctx,"print('Browser.supportedComponents.length = ');");duk_pop(ctx);
		duk_eval_string(ctx,"print(Browser.supportedComponents.length);"); duk_pop(ctx);
		duk_eval_string(ctx,"for(var i=0;i<Browser.supportedComponents.length;i++) {print(Browser.supportedComponents[i].name + ' '+Browser.supportedComponents[i].level);}"); duk_pop(ctx);
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

	return;
}



/* fwsetterNS, fwgetterNS are for our Script node dynamic fields, or
   more precisely, for the javascript property we create on the js>context>global object,
   one global>property for each script field, with the name of the script field on the property
*/

int SFNode_Setter0(FWType fwt, int index, void *ec, void * fwn, FWval fwval, int isCurrentScriptNode);
int fwsetterNS(duk_context *ctx) {
	/* myfield = new SFVec3f(1,2,3); 
	 * if myfield is a property we set on the global object, and we've assigned this setter to it,
	 * we'll come in here. We can set the *valueChanged and value on the LHS object, by copying, as per specs
	 * terminology: LHS: left hand side of equation (ie myfield) RHS: right hand side of equation (ie result of new SFVec3f() )
	 * if we come in here for AUXTYPES we should not write - AUXTYPE_X3DBrowser, and AUXTYPE_X3DConstants are static singletons
	 */
	int nargs; // , nr;
	int rc, itype, *valueChanged;
	//union anyVrml *field;
	const char *key;
	struct X3D_Node* parent = NULL;
	nargs = duk_get_top(ctx);

	/* retrieve key from nonstandard arg */
	//show_stack(ctx,"in fwsetterNS");
   // nativeValue = duk_require_string(ctx, 0);
    //implicit key by setter C function //char *key = duk_require_string(ctx, 1);
	key = duk_require_string(ctx,1); //"myprop";
	//printf("\nfwsetterNS, key=%s value=%s\n",key,nativeValue);

	itype = -1;
	/* get details of LHS object */
	/* retrieve field pointer from Cfunc */
	duk_push_current_function(ctx);
	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,-1,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	if(itype > -1 && itype < AUXTYPE_X3DConstants){
		//our script fields
		rc = duk_get_prop_string(ctx,-1,"fwNode");
		if(rc==1) parent = duk_to_pointer(ctx,-1);
		duk_pop(ctx);
		/* get the pointer to the changed flag */
		rc = duk_get_prop_string(ctx,-1,"fwChanged");
		if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
		duk_pop(ctx);
	}
	duk_pop(ctx); //pop current function

	if(itype > -1 && itype < AUXTYPE_X3DConstants){
		//code borrowed from cget and modified to not set setEventIn on self (auto-eventing this script)
		//const char* key;
		FWTYPE *fwt = getFWTYPE(FIELDTYPE_SFNode);
		int jndex, found;
		char type, readOnly;
		//check properties - if a property, call the type-specific setter
		//int lastProp;
		union anyVrml any;
		any.sfnode = parent;

		found = fwhas_generic(fwt,&any,key,&jndex,&type,&readOnly) && (type != 'f');
		if(found){
			FWval fwsetval = NULL;
			struct ArgListType arglist;
			int argc;
			arglist.argtypes = &type;
			arglist.fillMissingFixedWithZero = 0;
			arglist.nfixedArg = 1;
			arglist.iVarArgStartsAt = -1;
			convert_duk_to_fwvals(ctx, 1, -2, arglist, &fwsetval, &argc);
			if(argc == 1){
				struct X3D_Node *scriptnode;
				void *ec = NULL;
				//>>just SFNode function getNodeName needs to know the script node context (it can't use its own - it may be an IMPORT)
				duk_eval_string(ctx,"__script");
				scriptnode = (struct X3D_Node*) duk_to_pointer(ctx,-1);
				duk_pop(ctx);
				if(scriptnode)
					ec = (void *)scriptnode->_executionContext;
				//<<

				SFNode_Setter0(fwt,jndex,ec,&any,fwsetval,TRUE);
				//if(valueChanged)
				//	(*valueChanged) = 1; //DONE IN SFNODE_SETTER0
			}
			free(fwsetval);
		}
	}
	return 0;
}

void push_typed_proxy_fwgetter(duk_context *ctx, int itype, int mode, const char* fieldname, void *fwpointer,  int* valueChanged)
{
	/*  called by fwgetter (for referenced script->fields)
		1. push_object (fresh object)
		2. fwpointer: reference to script->field[i]->anyvrml
	*/
	//int rc;
	proxy_entry *pe = NULL;
	if(itype == FIELDTYPE_SFNode){
		struct X3D_Node* node = *(struct X3D_Node**)fwpointer;
		printf("pushtyped2 nodetype %d\n",node->_nodeType);
		pe = lookup_ctx_proxycache_entry_by_nodeptr(ctx, node);
	}
	if(pe){
		duk_push_heapptr(ctx,pe->jsproxy);
	}else{

		duk_eval_string(ctx,"Proxy");
		duk_push_object(ctx);
		duk_push_pointer(ctx,fwpointer);
		duk_put_prop_string(ctx,-2,"fwField");
		duk_push_pointer(ctx,valueChanged);
		duk_put_prop_string(ctx,-2,"fwChanged");
		duk_push_int(ctx,itype);
		duk_put_prop_string(ctx,-2,"fwItype");
		duk_eval_string(ctx,"handler");
		duk_new(ctx,2); /* [ global Proxy target handler ] -> [ global result ] */
	
		//2017 >
		if(doingFinalizer) { // && idogc){
			//push_typed_proxy2 _refers_ to script->field[i]->anyVrml (its caller fwgetter doesn't malloc) and should not GC its pointer
			//
			//Duktape.fin(a, function (x) {
			//       try {
			//           print('finalizer, foo ->', x.foo);
			//       } catch (e) {
			//           print('WARNING: finalizer failed (ignoring): ' + e);
			//       }
			//   });
			if(itype == FIELDTYPE_SFNode){
				struct X3D_Node* node = *(struct X3D_Node**)fwpointer;
				void *jsproxy = duk_get_heapptr(ctx, -1);
				add_ctx_proxycache_entry(ctx, node, jsproxy);
			}
			duk_eval_string(ctx,"Duktape.fin");
			duk_dup(ctx, -2); //copy the proxy object
			duk_push_c_function(ctx,cfinalizer,1);
			duk_pcall(ctx,2);
			duk_pop(ctx); //pop Duktape.fin result
		}
	}

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
		//int isOK = FALSE;
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
			if(0){
				if(itype == FIELDTYPE_SFNode){
				//test to compare anyVrml.SFNode with X3D_Node*
				//typedef struct X3D_Node*	vrmlNodeT;

				struct X3D_Node *anode;
				//anode = (struct X3D_Node *)(field); //WRONG (but how? H0: struct/union word alignment WRONG H1: off by a pointer * or & RIGHT)
				//anode = (struct X3D_Node *)&(field); //WRONG
				//anode = (struct X3D_Node *)&(*field); //WRONG
				//anode = (struct X3D_Node *)(*field); //I think this is numerically RIGHT, but syntactically awkward/WRONG for compilers
				(memcpy(&anode,field,sizeof(void *))); //RIGHT, works, same as above line: the contents of struct anyVrml is a pointer
				printf("anode._nodeType=%d ",anode->_nodeType); 
				printf("anyvrml.sfnode._nodetype=%d\n",field->sfnode->_nodeType);
				anode = field->sfnode; //RIGHT
				printf("anode = anyvrml.sfnode ._nodetype=%d\n",anode->_nodeType);
				printf("same?\n");
				}
			}
			push_typed_proxy_fwgetter(ctx, itype, mode, fieldname, field,  valueChanged);
			break;
		}
	}
	//show_stack(ctx,"in fwgetterNS at end");
    return nr;
}


int fwgetter0(duk_context *ctx,void *parent,int itype, char *key, int *valueChanged){
	//uses fwtype SFNode's getter
	FWTYPE *fwt = getFWTYPE(itype);
	int jndex, found, nr;
	char type, readOnly;
	nr = 0;
	//check properties - if a property, call the type-specific setter
	found = fwhas_generic(fwt,parent,key,&jndex,&type,&readOnly); //SFNode_Iterator
	if(found && fwt->Getter){
		FWVAL fwretval;
		struct X3D_Node *scriptnode;
		void *ec = NULL;
		//>>just SFNode function getNodeName needs to know the script node context (it can't use its own - it may be an IMPORT)
		duk_eval_string(ctx,"__script");
		scriptnode = (struct X3D_Node*) duk_to_pointer(ctx,-1);
		duk_pop(ctx);
		if(scriptnode)
			ec = (void *)scriptnode->_executionContext;
		//<<

		nr = fwt->Getter(fwt,jndex,ec,parent,&fwretval); //SFNode_Getter
		if(nr){
			nr = fwval_duk_push(ctx,&fwretval,valueChanged);
		}
	}
	return nr;
}
int fwgetterNS(duk_context *ctx) {
	/* when we initializeContext we assign 2 kinds of properties to the global object in the context:
		1. FIELDTYPES: our Script Node's dynamic (scene-authored) fields (or more precisely, their js proxys), with features:
			- has valueChanged, getName, getMode (getType is part of all FIELDTYPEs and AUXTYPEs)
			- reference when getting if non-primitive, deep copy when setting
		2. AUXTYPES: a) Browser (AUXTYPE_X3DBrowser) b) X3DConstants (AUXTYPE_X3DConstants)
			- reference when getting, never set (these two are static singletons)
	*/
	int nargs, nr;
	int rc, itype, *valueChanged = NULL;
	//const char *fwName = NULL;
	const char *fieldname;
	struct X3D_Node *thisScriptNode = NULL;
	//union anyVrml *field;

	nargs = duk_get_top(ctx);
	itype = 0;
	/* retrieve key from nonstandard arg */
	//show_stack(ctx,"in fwgetterNS at start");
	fieldname = duk_require_string(ctx,0);
	//printf("\nfwgetterNS key=%s\n",key);

	/* retrieve field pointer from Cfunc */
	duk_push_current_function(ctx);
	/* get type of parent object for this property*/
	rc = duk_get_prop_string(ctx,-1,"fwItype");
	if(rc==1) itype = duk_get_int(ctx,-1);
	duk_pop(ctx);
	if(itype < AUXTYPE_X3DConstants){
		//our script fields
		rc = duk_get_prop_string(ctx,-1,"fwNode");
		if(rc==1) thisScriptNode = duk_to_pointer(ctx,-1);
		duk_pop(ctx);
		/* get the pointer to the changed flag */
		rc = duk_get_prop_string(ctx,-1,"fwChanged");
		if(rc == 1) valueChanged = duk_to_pointer(ctx,-1);
		duk_pop(ctx);
	}
	duk_pop(ctx); //pop current function


	nr = 0;
	if(itype < AUXTYPE_X3DConstants){
		//our script fields
		union anyVrml any;
		any.sfnode = thisScriptNode;
		nr = fwgetter0(ctx,&any,FIELDTYPE_SFNode,fieldname,valueChanged);
	}else{
		//X3DBrowser, X3DConstants
		push_typed_proxy_fwgetter(ctx, itype, PKW_initializeOnly, fieldname, NULL, NULL);
		nr = 1;
	}
    return nr;
}

void add_duk_global_property(duk_context *ctx, int itype, const char *fieldname, int *valueChanged, struct X3D_Node *node ){
	//int rc;
	//char *str;

	duk_eval_string(ctx, "defineAccessor"); //defineAccessor(obj,propName,setter,getter)
	/* push object */
	duk_eval_string(ctx,"this"); //global object
	/* push key */
	duk_push_string(ctx,fieldname); //"myScriptFieldName"
	/* push setter */
	duk_push_c_function(ctx,fwsetterNS,2); //1 extra parameter is nonstandard (NS) key
	if(itype < AUXTYPE_X3DConstants){
		duk_push_pointer(ctx,valueChanged);
		duk_put_prop_string(ctx,-2,"fwChanged");
		duk_push_pointer(ctx,node);
		duk_put_prop_string(ctx,-2,"fwNode");
	}
	duk_push_int(ctx,itype);
	duk_put_prop_string(ctx,-2,"fwItype");
	/* push getter */
	duk_push_c_function(ctx,fwgetterNS,1); //0 extra parameter is nonstandard (NS) key
	if(itype < AUXTYPE_X3DConstants){
		duk_push_pointer(ctx,node);
		duk_put_prop_string(ctx,-2,"fwNode");
		duk_push_pointer(ctx,valueChanged);
		duk_put_prop_string(ctx,-2,"fwChanged");
	}
	duk_push_int(ctx,itype);
	duk_put_prop_string(ctx,-2,"fwItype");

	duk_call(ctx, 4);
	duk_pop(ctx);
}

void InitScriptField2(struct CRscriptStruct *scriptcontrol, int itype, const char* fieldname, int *valueChanged, struct X3D_Node* parent)
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
	//int iglobal;
	//printf("in InitScriptField\n");

	// create twin property
	ctx = scriptcontrol->cx;
	//iglobal = *(int*)scriptcontrol->glob; 
	add_duk_global_property(ctx,itype,fieldname, valueChanged,parent);

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
	struct CRscriptStruct *scriptcontrol; //*ScriptControlArray, 
	//ScriptControlArray = getScriptControl();
	scriptcontrol = getScriptControlIndex(num); //&ScriptControlArray[num];


	/* run through fields in order of entry in the X3D file */
	script = scriptcontrol->script;
	//printf("adding fields from script %p\n",script);
	nfields = Shader_Script_getScriptFieldCount(script);
	for(i=0;i<nfields;i++){
		field = Shader_Script_getScriptField(script,i);
		fieldname = ScriptFieldDecl_getName(field);
		kind = ScriptFieldDecl_getMode(field);
		itype = ScriptFieldDecl_getType(field);
		if (kind != PKW_inputOnly) { //we'll hook input events to the author's functions elsewhere
			//everything else -fields, eventOuts- needs a strict property twin created on the global object
			field->valueChanged = 0;
			InitScriptField2(scriptcontrol, itype, fieldname, &field->valueChanged, script->ShaderScriptNode);
		}
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
	int len, rc, iret;
	duk_context *ctx;
	int iglobal;
	struct CRscriptStruct *ScriptControl; // = getScriptControl();
	//printf("in jsActualrunScript\n");

	ScriptControl = getScriptControlIndex(num);
	/* get context and global object for this script */
	ctx = (duk_context *)ScriptControl->cx;
	//iglobal = *((int *)ScriptControl[num].glob);
	iglobal = ((int *)&ScriptControl->glob)[0];

	//CLEANUP_JAVASCRIPT(_context)

	len = (int) strlen(script);
	iret = TRUE;
	if(0){
		rc=0;
		//this will do a popup abort, with no diagnostic message
		duk_eval_string(ctx, script);
		if(rc<0){
			printf ("ActualrunScript - JS_EvaluateScript failed for %s", script);
			printf ("\n");
			ConsoleMessage ("ActualrunScript - JS_EvaluateScript failed for %s", script);
			iret = FALSE;
		}
		duk_pop(ctx); //pop result which we don't use
	}else{
		//this shows the diagnostic message, and allows the program to continue running with the script not run
		duk_push_string(ctx, script);
		if (duk_peval(ctx) != 0) {
			ConsoleMessage("eval failed: %s\n", duk_safe_to_string(ctx, -1));
			iret = FALSE;
		} else if(0) {
			printf("result is: %s\n", duk_safe_to_string(ctx, -1));
		}
		duk_pop(ctx); //pop result which we don't use
	}


	return iret;
}
void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value){
	return;
}
static int duk_once = 0;
void process_eventsProcessed(){
	duk_context *ctx;
	int rc, counter;
	struct CRscriptStruct *scriptcontrol;
	ttglobal tg;
	ppJScript p;

	//if(!duk_once) printf("in process_eventsProcessed\n");
	//call function eventsProcessed () {

	duk_once++;

	tg = gglobal();
	p = (ppJScript)tg->JScript.prv;
	for (counter = 0; counter <= tg->CRoutes.max_script_found_and_initialized; counter++) {
		scriptcontrol = getScriptControlIndex(counter);
		if(scriptcontrol){
			//if (scriptcontrol->eventsProcessed == NULL) {
			//	//compile function - duktape doesn't have this
			//	scriptcontrol->eventsProcessed = ???
			//}
			ctx = scriptcontrol->cx;
			if(scriptcontrol->thisScriptType != NOSCRIPT && ctx){
				duk_eval_string(ctx,"eventsProcessed"); //gets the evenin function on the stack
				//push double TickTime(); as arg
				duk_push_number(ctx,TickTime());
				rc = duk_pcall(ctx, 1);
				if (rc != DUK_EXEC_SUCCESS) {
				  printf("error: '%s' happened in js function %s called from process_eventsProcessed\n", duk_to_string(ctx, -1),"eventsProcessed");
				}
				duk_pop(ctx); //pop undefined that results from void myfunc(){}
			}
		}
	}

	return;
}
void js_cleanup_script_context(int counter){
	//printf("in js_cleanup_script_context\n");
	return;
}
void js_setField_javascriptEventOut_B(union anyVrml* any, int fieldType, unsigned len, int extraData, int actualscript){
	//I think in here there is nothing to do for brotos, because the job of _B was to copy values out of javascript and
	//into script fields, and the _B broto approach to routing would then do routing from the script fields.
	//here in the duk / proxy method, we are already doing the setting of script fields directly.
	//printf("in js_setField_javascriptEventOut_B\n");
	return;
}

void setField_javascriptEventOut(struct X3D_Node *tn,unsigned int tptr,  int fieldType, unsigned len, int extraData) {
	//this proxy method already writes to the script field, so there's nothing to update in javascript
	//- can just copy anyVrml from script field to endpoint on Route 
	// (Brotos don't come in this function)
	char *memptr;
	char *fromptr;
	//int datasize;
	ttglobal tg = gglobal();

	/* set up a pointer to where to put this stuff */
	memptr = offsetPointer_deref(char *, tn, tptr);
	//the from -our current script field value- is coming in through JSglobal_return_val 
	fromptr = tg->JScript.JSglobal_return_val;
	
	medium_copy_field0(fieldType,fromptr,memptr); //will copy p data in MF
	return;
}
void js_setField_javascriptEventOut(struct X3D_Node *tn,unsigned int tptr,  int fieldType, unsigned len, int extraData, int actualscript) {
	struct CRscriptStruct *scriptcontrol;

	scriptcontrol = getScriptControlIndex(actualscript);
	setField_javascriptEventOut(tn,tptr,fieldType, len, extraData);
}




void set_one_ECMAtype (int tonode, int toname, int dataType, void *Data, int datalen) {
	//char scriptline[100];
	//FWVAL newval;
	duk_context *ctx;
	int obj, rc;
	struct CRscriptStruct *ScriptControl; // = getScriptControl();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	//printf("in set_one_ECMAtype\n");

	#ifdef SETFIELDVERBOSE
	printf ("set_one_ECMAtype, to %d namepointer %d, fieldname %s, datatype %d length %d\n",
		tonode,toname,JSparamnames[toname].name,dataType,datalen);
	#endif

	/* get context and global object for this script */
	ScriptControl = getScriptControlIndex(tonode);
	ctx =  (duk_context *)ScriptControl->cx;
	//ctx =  (duk_context *)ScriptControl[tonode].cx;
	//obj = *(int*)ScriptControl[tonode].glob; //don't need
	//obj = ((int*)&ScriptControl[tonode].glob)[0]; //don't need
	obj = ((int*)&ScriptControl->glob)[0]; //don't need


	//get function by name
	duk_eval_string(ctx,JSparamnames[toname].name); //gets the evenin function on the stack

	//push ecma value as arg
	{
		int rc;
		FWVAL fwval;
		fwval._web3dval.native = Data;
		fwval._web3dval.fieldType = dataType;
		fwval._web3dval.gc = 0;
		fwval.itype = 'W';
		rc = fwval_duk_push(ctx, &fwval, NULL);
		//if(rc == 1) OK
	}
	//push double TickTime(); as arg
	duk_push_number(ctx,TickTime());
	//run function
	rc = duk_pcall(ctx, 2);  /* [ ... func 2 3 ] -> [ 5 ] */
	if (rc != DUK_EXEC_SUCCESS) {
	  printf("error: '%s' happened in js function %s called from set_one_ECMAType\n", duk_to_string(ctx, -1),JSparamnames[toname].name);
	}

	duk_pop(ctx); //pop undefined that results from void myfunc(){}
	//printf("end ecma\n");
}



/*  setScriptECMAtype called by getField_ToJavascript for
        case FIELDTYPE_SFBool:
        case FIELDTYPE_SFFloat:
        case FIELDTYPE_SFTime:
        case FIELDTYPE_SFDouble:
        case FIELDTYPE_SFInt32:
        case FIELDTYPE_SFString:
*/

void setScriptECMAtype (int num) {
	void *fn;
	int tptr;
	int len;
	int to_counter;
	CRnodeStruct *to_ptr = NULL;
	struct CRStruct *CRoutes = getCRoutes();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	//printf("in setScriptECMAtype\n");
	fn = offsetPointer_deref(void *, CRoutes[num].routeFromNode, CRoutes[num].fnptr);
	len = CRoutes[num].len;

	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
                struct Shader_Script *myObj;

		to_ptr = &(CRoutes[num].tonodes[to_counter]);
                myObj = X3D_SCRIPT(to_ptr->routeToNode)->__scriptObj;
		/* printf ("setScriptECMAtype, myScriptNumber is %d\n",myObj->num); */
		tptr = to_ptr->foffset;
		set_one_ECMAtype (myObj->num, tptr, JSparamnames[tptr].type, fn,len);
	}
}

void set_one_MultiElementType (int tonode, int tnfield, void *Data, int dataLen){
	//tonode - script array num
	//tnfield - integer index into jsparamname[] array
	//void* Data - pointer to anyVrml of the from node
	//datalen - size of anyVrml to memcpy
	//FWVAL newval;
	duk_context *ctx;
	int obj, rc;
	int itype;
	void *datacopy;
	struct CRscriptStruct *ScriptControl; // = getScriptControl();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	ScriptControl = getScriptControlIndex(tonode);
	//ctx =  (duk_context *)ScriptControl[tonode].cx;
	ctx =  (duk_context *)ScriptControl->cx;
	//obj = *(int*)ScriptControl[tonode].glob;
	//obj = ((int*)&ScriptControl[tonode].glob)[0];
	obj = ((int*)&ScriptControl->glob)[0];
	
	//printf("in set_one_MultiElementType\n");
	//get function by name
	duk_eval_string(ctx,JSparamnames[tnfield].name); //gets the evenin function on the stack
	itype = JSparamnames[tnfield].type;
	//medium copy
	datacopy = NULL;
	medium_copy_field(itype,Data,&datacopy);
	push_typed_proxy2(ctx,itype,PKW_inputOutput,datacopy,NULL,'T');
	duk_push_number(ctx,TickTime());
	//duk_call(ctx,2);
	rc = duk_pcall(ctx, 2);  /* [ ... func 2 3 ] -> [ 5 ] */
	if (rc != DUK_EXEC_SUCCESS) {
	  printf("error: '%s' happened in js function %s called from set_one_Multi_ElementType\n", duk_to_string(ctx, -1),JSparamnames[tnfield].name);
	}
	//show_stack(ctx,"after calling isOver");
	duk_pop(ctx); //pop undefined that results from void myfunc(){}
	return;
}
void set_one_MFElementType(int tonode, int toname, int dataType, void *Data, int datalen){
	//tonode - script array num
	//tnfield - integer index into jsparamname[] array
	//void* Data - MF.p
	//datalen - MF.n
	//FWVAL newval;
	duk_context *ctx;
	int obj;
	int itype;
	union anyVrml *any;
	void *datacopy = NULL;
	//char *source = (char *)Data - sizeof(int); //backup so we get the whole MF including .n
	struct Multi_Any maData;
	char *source;
	struct CRscriptStruct *ScriptControl; // = getScriptControl();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	ScriptControl = getScriptControlIndex(tonode);
	//ctx =  (duk_context *)ScriptControl[tonode].cx;
	ctx =  (duk_context *)ScriptControl->cx;
	//obj = *(int*)ScriptControl[tonode].glob;
	//obj = ((int*)&ScriptControl[tonode].glob)[0];
	obj = ((int*)&ScriptControl->glob)[0];
	
	//printf("in set_one_MFElementType\n");
	//get function by name
	duk_eval_string(ctx,JSparamnames[toname].name); //gets the evenin function on the stack
	itype = dataType; //JSparamnames[toname].type;
	//medium copy
	maData.n = datalen;
	maData.p = Data;
	source = (char *)&maData;
	any = (void*)source;

	medium_copy_field(itype,source,&datacopy);
	any = datacopy;
	push_typed_proxy2(ctx,itype,PKW_inputOutput,datacopy,NULL,'T');
	duk_push_number(ctx,TickTime());
	duk_call(ctx,2);
	//show_stack(ctx,"after calling isOver");
	duk_pop(ctx); //pop undefined that results from void myfunc(){}
	return;
}
int jsIsRunning(){
	//printf("in jsIsRunning\n");
	return 1;
}
void JSDeleteScriptContext(int num){
	struct CRscriptStruct *ScriptControl;
	//printf("in JSDeleteScriptContext\n");
	ScriptControl = getScriptControlIndex(num);
	duk_destroy_heap(ScriptControl->cx);
	return;
}
void jsShutdown(){
	//printf("in jsShutdown\n");
	return;
}
void jsClearScriptControlEntries(int num){
	//printf("in jsClearScriptControlEntries\n");
	return;
}
/* run the script from within Javascript  */
/*
int jsrrunScript(duk_context *ctx, char *script, FWval retval) {
	double val;
	int ival, itype, isOK;
	const char *cval;
	duk_eval_string(ctx,script);
	int RHS_duk_type = duk_get_type(ctx, -1);
	isOK = FALSE;
	switch(RHS_duk_type){
	case DUK_TYPE_NUMBER: 
		retval->_numeric = duk_require_number(ctx,-1);
		retval->itype = 'D';
		isOK = TRUE;
		break;
	case DUK_TYPE_BOOLEAN: 
		retval->_boolean = duk_require_boolean(ctx,-1);
		retval->itype = 'B';
		isOK = TRUE;
		break;
	case DUK_TYPE_STRING:
		retval->_string = duk_require_string(ctx,-1);
		retval->itype = 'S';
		isOK = TRUE;
		break;
	case DUK_TYPE_OBJECT:
	{
		int rc, itypeRHS = -1;
		union anyVrml *fieldRHS = NULL;
		rc = duk_get_prop_string(ctx,-1,"fwItype");
		if(rc == 1){
			//printf(duk_type_to_string(duk_get_type(ctx, -1)));
			itypeRHS = duk_to_int(ctx,-1);
		}
		duk_pop(ctx);
		rc = duk_get_prop_string(ctx,-1,"fwField");
		if(rc == 1) fieldRHS = duk_to_pointer(ctx,-1);
		duk_pop(ctx);
		//we don't need the RHS fwChanged=valueChanged* because we are only changing the LHS

		if(fieldRHS != NULL && itypeRHS > -1){
			retval->_web3dval.native = fieldRHS; //shallow copy - won't copy p[] in MF types
			retval->_web3dval.fieldType = itypeRHS;
			isOK = TRUE;
		}
	}
		break;
	case DUK_TYPE_NONE: 
	case DUK_TYPE_UNDEFINED: 
	case DUK_TYPE_NULL: 
		// are we attempting to null out the field? we aren't allowed to change its type (to undefined) 
	case DUK_TYPE_POINTER: 
		// don't know what this would be for if anything 
	default:
		isOK = FALSE;
		break;
	}
	duk_pop(ctx); //the duk_eval_string result;
	return isOK; //we leave results on stack
}
*/
int isScriptControlOK(int actualscript);
int isScriptControlInitialized(int actualscript);
void getField_ToJavascript_B(int shader_num, int fieldOffset, int type, union anyVrml *any, int len);
int runQueuedDirectOutputs()
{
	/*
	http://www.web3d.org/files/specifications/19775-1/V3.3/Part01/components/scripting.html#directoutputs
	http://www.web3d.org/files/specifications/19775-1/V3.3/Part01/components/scripting.html#Accessingfieldsandevents

	Interpretation: The reason the SAI specs say to queue directOutputs in an event queue, 
	is because external SAIs are running in a different thread: the rendering thread could be
	using a node just when you want to write to it from the SAI thread.
	I'll assume here we are working on the internal/javascript/ecmascript SAI, and that it is
	synchronous with the rendering thread, so it can safely write to nodes without queuing.

	So our effort here is just to make it convenient to write to eventIn/inputOnly 
	(or the eventIn/inputOnly part of exposedField/inputOutput fields).

	Writing to builtin nodes from a script is already implemented in freewrl by directly writing 
	the fields immediately during the script. However writing to another script wassn't working properly July 8, 2014.
	The following proposed algo was the result of analyzing the behaviour of other vrml/x3d browsers. 

	DIRECTOUTPUT ALGO:
	When writing to another script node from the current script:
	a) write unconditionally to the other script->field->value, including to field/initializeOnly and eventIn/inputOnly
	b) set a valueSet flag on the field (like valueChanged for output) and the valueChanged flag
	c) set the node _changed or isActive flag to trigger updates
	d) either 
	i) have a stack of queues of script nodes changed and process after each script function OR
	ii) like gatherScriptEventOuts() have a spot in the routing loop to look at the valueSet flag 
	    for script fields and if valueSet then if the field is inputOnly/eventIn or exposedField/inputOutput 
		take the field->value and pass it the the eventIn function (with the current/same timestamp).

	It's this d) ii) we are implementing here.
	*/
	ttglobal tg = gglobal();
	struct Shader_Script *script;
	struct ScriptFieldDecl *field;
	int i,num,kind, itype;
	const char *fieldname;
	static int doneOnce = 0;
	int moreAction;
	struct CRscriptStruct *scriptcontrol; //*ScriptControlArray, 
	//ScriptControlArray = getScriptControl();
	
	if(!doneOnce){
		//	printf("in runQueuedDirectOutputs\n");
		printf("duktape javascript engine version %ld\n", DUK_VERSION);
		doneOnce++;
	}
	moreAction = FALSE;
	for(num=0;num< tg->CRoutes.max_script_found_and_initialized;num++){
		scriptcontrol = getScriptControlIndex(num); //&ScriptControlArray[num];
		if(scriptcontrol)
		{
			script = scriptcontrol->script;
			if(scriptcontrol->thisScriptType != NOSCRIPT && script){
				if(isScriptControlInitialized(script->num) && isScriptControlOK(script->num)){
					int nfields = Shader_Script_getScriptFieldCount(script);
					for(i=0;i<nfields;i++){
						field = Shader_Script_getScriptField(script,i);
						fieldname = ScriptFieldDecl_getName(field);
						kind = ScriptFieldDecl_getMode(field);
						itype = ScriptFieldDecl_getType(field);
						if(field->eventInSet){
							if( (kind == PKW_inputOnly || kind == PKW_inputOutput)){
								int isMF, sftype, len, isize;
								int JSparamNameIndex = field->fieldDecl->JSparamNameIndex;
								mark_script(script->num);
								//run script eventIn function with field->value and tickTime
								isMF = itype % 2; //WRONG - use a function to lookup
								sftype = itype - isMF;
								//from EAI_C_CommonFunctions.c
								isize = returnElementLength(sftype) * returnElementRowSize(sftype);
								if(isMF) len = sizeof(int) + sizeof(void*);
								else len = isize;

								field->eventInSet = FALSE;
								getField_ToJavascript_B(script->num, JSparamNameIndex, itype, &field->value, len);
								//printf("+eventInSet and input kind=%d value=%f\n",kind,field->value.sffloat);
								moreAction = TRUE;
							}else{
								//printf("-eventInSet but not input kind=%d value=%f\n",kind,field->value.sffloat);
								field->eventInSet = FALSE;
							}
						}
					}
				}
			}
		}
	}
	return moreAction; //IF TRUE will make routing do another loop on the same timestamp
}


#endif /*  defined(JAVASCRIPT_DUK) */