/*


Javascript C language binding.

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


#include <config.h>
#if !(defined(JAVASCRIPT_STUB) || defined(JAVASCRIPT_DUK))
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>

#include "../vrml_parser/Structs.h"
#include "../vrml_parser/CRoutes.h"
#include "../opengl/OpenGL_Utils.h"
#include "../main/headers.h"
#include "../main/ProdCon.h"
#include "../scenegraph/RenderFuncs.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CProto.h"
#include "../vrml_parser/CParse.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../x3d_parser/Bindable.h"
#include "../input/EAIHeaders.h"	/* for implicit declarations */


#include "JScript.h"
#include "CScripts.h"
#include "fieldSet.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"
#include "jsVRMLBrowser.h"

#ifdef HAVE_JAVASCRIPT

#define X3DBROWSER 1



#ifndef X3DBROWSER
#if JS_VERSION < 185
#define SetPropertyStub JS_PropertyStub
#else
#define SetPropertyStub JS_StrictPropertyStub
#endif
#endif // ndef X3DBROWSER

#ifdef X3DBROWSER
JSBool
#if JS_VERSION < 185
BrowserGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);
#else
BrowserGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp);
#endif
JSBool
#if JS_VERSION < 185
BrowserSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);
#else
BrowserSetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp);
#endif

#endif
int jsrrunScript(JSContext *_context, JSObject *_globalObj, char *script, jsval *rval);


//Q. is this a true sharable static?
static JSClass Browser = {
    "Browser",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
#ifdef X3DBROWSER
    BrowserGetProperty, 
	BrowserSetProperty,
#else
	JS_PropertyStub,
	SetPropertyStub,
#endif

    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static JSBool doVRMLRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, const char *browserFunc); 

static JSFunctionSpec (BrowserFunctions)[] = {
	{"getName", VrmlBrowserGetName, 0},
	{"getVersion", VrmlBrowserGetVersion, 0},
	{"getCurrentSpeed", VrmlBrowserGetCurrentSpeed, 0},
	{"getCurrentFrameRate", VrmlBrowserGetCurrentFrameRate, 0},
	{"getWorldURL", VrmlBrowserGetWorldURL, 0},
	{"replaceWorld", VrmlBrowserReplaceWorld, 0},
	{"loadURL", VrmlBrowserLoadURL, 0},
	{"setDescription", VrmlBrowserSetDescription, 0},
	{"createVrmlFromString", VrmlBrowserCreateVrmlFromString, 0},
	{"createVrmlFromURL", VrmlBrowserCreateVrmlFromURL, 0},
	{"createX3DFromString", VrmlBrowserCreateX3DFromString, 0},
	{"createX3DFromURL", VrmlBrowserCreateVrmlFromURL, 0},
	{"addRoute", VrmlBrowserAddRoute, 0},
	{"deleteRoute", VrmlBrowserDeleteRoute, 0},
	{"print", VrmlBrowserPrint, 0},
	{"println", VrmlBrowserPrintln, 0},
#ifdef X3DBROWSER
	//{"replaceWorld", X3dBrowserReplaceWorld, 0},  //conflicts - X3DScene vs MFNode parameter - could detect?
	//{"createX3DFromString", X3dBrowserCreateX3DFromString, 0}, //conflicts but above verion shouldn't be above, or could detect?
	//{"createX3DFromURL", X3dBrowserCreateVrmlFromURL, 0}, //conflicts but above version shouldn't be above, or could detect?
	//{importDocument, X3dBrowserImportDocument, 0), //not sure we need/want this, what does it do?
	//{getRenderingProperty, X3dGetRenderingProperty, 0},
	//{addBrowserListener, X3dAddBrowserListener, 0},
	//{removeBrowserListener, X3dRemoveBrowserListener, 0},
#endif
	{0}
};
#ifdef X3DBROWSER

/* ProfileInfo, ProfileInfoArray, ComponentInfo, ComponentInfoArray
   I decided to do these as thin getter wrappers on the bits and pieces defined
   in Structs.h, GeneratedCode.c and capabilitiesHandler.c
   The Array types return the info type wrapper with private native member == index into
   the native array.
*/
//ComonentInfo{
//String name;
//Numeric level;
//String Title;
//String providerUrl;
//}
int capabilitiesHandler_getTableLength(int* table);
int capabilitiesHandler_getComponentLevel(int *table, int comp);
int capabilitiesHandler_getProfileLevel(int prof);
const int *capabilitiesHandler_getProfileComponent(int prof);
const int *capabilitiesHandler_getCapabilitiesTable();
typedef struct intTableIndex{
	int* table;
	int index;
} *IntTableIndex;

JSBool
#if JS_VERSION < 185
ComponentInfoGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ComponentInfoGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	IntTableIndex ptr;
	int _index, *_table, _nameIndex;
	jsval rval;
	jsval id;

	UNUSED(rval); // compiler warning mitigation

#if JS_VERSION >= 185
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in ComponentInfoGetProperty.\n");
		return JS_FALSE;
	}
#endif
	if ((ptr = (IntTableIndex)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in ExecutionContextGetProperty.\n");
		return JS_FALSE;
	}
	_index = ptr->index;
	_table = ptr->table;
//extern const char *COMPONENTS[];
//extern const int COMPONENTS_COUNT;

    if (JSVAL_IS_INT(id)) 
	{
		int index = JSVAL_TO_INT(id);
		switch(index){
			case 0://name
			case 1://Title
				_nameIndex = _table[2*_index];
#if JS_VERSION < 185
			*rval = STRING_TO_JSVAL(COMPONENTS[_index]);
#else
			JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,COMPONENTS[_nameIndex])));
#endif
				break;
			case 2://level
				{
				int level = capabilitiesHandler_getComponentLevel(_table,_index);
#if JS_VERSION < 185
			*rval = INT_TO_JSVAL(lev);
#else
			JS_SET_RVAL(cx,vp,INT_TO_JSVAL(level));
#endif
				}
				break;
			case 3://providerUrl
#if JS_VERSION < 185
				*rval = STRING_TO_JSVAL("freewrl.sourceforge.net");
#else
				JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,"freewrl.sourceforge.net")));
#endif
				break;
		}
	}
	return JS_TRUE;
}
JSBool
#if JS_VERSION < 185
ComponentInfoSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ComponentInfoSetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	//can I, should I force it to read-only this way?
	return JS_FALSE;
}
void
ComponentInfoFinalize(JSContext *cx, JSObject *obj)
{
	IntTableIndex ptr;
	if ((ptr = (IntTableIndex)JS_GetPrivate(cx, obj)) == NULL) {
		return;
	} else {
		FREE_IF_NZ (ptr);
	}
}


static JSClass ComponentInfoClass = {
    "ComponentInfo",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    ComponentInfoGetProperty, 
	ComponentInfoSetProperty, 
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    ComponentInfoFinalize //JS_FinalizeStub
};

static JSPropertySpec (ComponentInfoProperties)[] = {
	//executionContext
	{"name", 0, JSPROP_ENUMERATE},  //"Core"
	{"Title", 1, JSPROP_ENUMERATE}, //"Core"
	{"level", 2, JSPROP_ENUMERATE},  //4
	{"providerUrl", 3, JSPROP_ENUMERATE}, //"freewrl.sourceforge.net"
	{0}
};


//ComponentInfoArray{
//numeric length;
//ComponentInfo [integer index];
//}

JSBool
#if JS_VERSION < 185
ComponentInfoArrayGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ComponentInfoArrayGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	int *_table;
	jsval rval;
	jsval id;

	UNUSED(rval); // compiler warning mitigation

#if JS_VERSION >= 185
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in ComponentInfoArrayGetProperty.\n");
		return JS_FALSE;
	}
#endif
	if ((_table = (int *)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in ProfileInfoGetProperty.\n");
		return JS_FALSE;
	}

    if (JSVAL_IS_INT(id)) 
	{
		int index = JSVAL_TO_INT(id);
		if(index == -1){
//extern const char *COMPONENTS[];
//extern const int COMPONENTS_COUNT;

			int _length = capabilitiesHandler_getTableLength(_table); //COMPONENTS_COUNT;
#if JS_VERSION < 185
			*rval = INT_TO_JSVAL(_length);
#else
			JS_SET_RVAL(cx,vp,INT_TO_JSVAL(_length));
#endif
		}else if(index > -1 && index < COMPONENTS_COUNT )
		{
			JSObject *_obj;
			IntTableIndex tableindex = malloc(sizeof(struct intTableIndex));
			//int* _index = malloc(sizeof(int));
			_obj = JS_NewObject(cx,&ComponentInfoClass,NULL,obj);
			tableindex->index = index;
			tableindex->table = _table;
			if (!JS_DefineProperties(cx, _obj, ComponentInfoProperties)) {
				printf( "JS_DefineProperties failed in ComponentInfoProperties.\n");
				return JS_FALSE;
			}

			if (!JS_SetPrivate(cx, _obj, (void*)tableindex)) {
				printf( "JS_SetPrivate failed in ComponentInfoArray.\n");
				return JS_FALSE;
			}

#if JS_VERSION < 185
			*rval = OBJECT_TO_JSVAL(_obj);
#else
			JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif

		}
	}
	return JS_TRUE;
}
JSBool
#if JS_VERSION < 185
ComponentInfoArraySetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ComponentInfoArraySetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	//can I, should I force it to read-only this way?
	return JS_FALSE;
}


static JSClass ComponentInfoArrayClass = {
    "ComponentInfoArray",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    ComponentInfoArrayGetProperty, 
	ComponentInfoArraySetProperty, 
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static JSPropertySpec (ComponentInfoArrayProperties)[] = {
	{"length", -1, JSPROP_READONLY | JSPROP_SHARED | JSPROP_PERMANENT}, //JSPROP_ENUMERATE},
	{0}
};

//ProfileInfo{
//String name;
//Numeric level;
//String Title;
//String providerUrl;
//ComonentInfoArray components;
//}

JSBool
#if JS_VERSION < 185
ProfileInfoGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ProfileInfoGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	int *ptr;
	int _index;
	jsval rval;
	jsval id;

	UNUSED(rval); // compiler warning mitigation

#if JS_VERSION >= 185
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in ProfileInfoGetProperty.\n");
		return JS_FALSE;
	}
#endif
	if ((ptr = (int *)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in ProfileInfoGetProperty.\n");
		return JS_FALSE;
	}
	_index = *ptr;
//extern const char *PROFILES[];
//extern const int PROFILES_COUNT;

    if (JSVAL_IS_INT(id)) 
	{
		int index = JSVAL_TO_INT(id);
		switch(index){
			case 0://name
			case 1://Title
#if JS_VERSION < 185
			*rval = STRING_TO_JSVAL(COMPONENTS[_index]);
#else
			JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,PROFILES[_index])));
#endif
				break;
			case 2://level
				{
				int level = capabilitiesHandler_getProfileLevel(_index);
#if JS_VERSION < 185
			*rval = INT_TO_JSVAL(lev);
#else
			JS_SET_RVAL(cx,vp,INT_TO_JSVAL(level));
#endif
				}
				break;
			case 3://providerUrl
#if JS_VERSION < 185
				*rval = STRING_TO_JSVAL("freewrl.sourceforge.net");
#else
				JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,"freewrl.sourceforge.net")));
#endif
				break;
			case 4://components ComponentInfoArray
				{
					const int *_table = capabilitiesHandler_getProfileComponent(_index);
					JSObject *_obj;
					//malloc private not needed
					_obj = JS_NewObject(cx,&ComponentInfoArrayClass,NULL,obj);
					if (!JS_DefineProperties(cx, _obj, ComponentInfoArrayProperties)) {
						printf( "JS_DefineProperties failed in ComponentInfoArrayProperties.\n");
						return JS_FALSE;
					}
				
					if (!JS_SetPrivate(cx, _obj, (void*)_table)) {
						printf( "JS_SetPrivate failed in ComponentInfoArray.\n");
						return JS_FALSE;
					}
#if JS_VERSION < 185
					*rval = OBJECT_TO_JSVAL(_obj);
#else
					JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif
				}
				break;
		}
	}
	return JS_TRUE;
}
JSBool
#if JS_VERSION < 185
ProfileInfoSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ProfileInfoSetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	//can I, should I force it to read-only this way?
	return JS_FALSE;
}

static JSClass ProfileInfoClass = {
    "ProfileInfo",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    ProfileInfoGetProperty, 
	ProfileInfoSetProperty, 
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static JSPropertySpec (ProfileInfoProperties)[] = {
	//executionContext
	{"name", 0, JSPROP_ENUMERATE},
	{"Title", 1, JSPROP_ENUMERATE},
	{"level", 2, JSPROP_ENUMERATE},
	{"providerUrl", 3, JSPROP_ENUMERATE},
	{"components", 4, JSPROP_ENUMERATE},
	{0}
};

//ProfileInfoArray{
//numeric length;
//ProfileInfo [integer index];
//}

JSBool
#if JS_VERSION < 185
ProfileInfoArrayGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ProfileInfoArrayGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	jsval rval;
	jsval id;

	UNUSED(rval); // compiler warning mitigation

#if JS_VERSION >= 185
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in ProfileInfoArrayGetProperty.\n");
		return JS_FALSE;
	}
#endif

    if (JSVAL_IS_INT(id)) 
	{
		int index = JSVAL_TO_INT(id);
		if(index == -1){
			int _length = PROFILES_COUNT;
#if JS_VERSION < 185
			*rval = INT_TO_JSVAL(_length);
#else
			JS_SET_RVAL(cx,vp,INT_TO_JSVAL(_length));
#endif
		}else
		//if(index < getNumberOfProfiles() )
		{
			JSObject *_obj;
			int* _index = malloc(sizeof(int));
			_obj = JS_NewObject(cx,&ProfileInfoClass,NULL,obj);
			*_index = index;
			if (!JS_DefineProperties(cx, _obj, ProfileInfoProperties)) {
				printf( "JS_DefineProperties failed in ProfileInfoProperties.\n");
				return JS_FALSE;
			}

			if (!JS_SetPrivate(cx, _obj, (void*)_index)) {
				printf( "JS_SetPrivate failed in ProfileInfoArray.\n");
				return JS_FALSE;
			}

#if JS_VERSION < 185
			*rval = OBJECT_TO_JSVAL(_obj);
#else
			JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif

		}
	}
	return JS_TRUE;
}
JSBool
#if JS_VERSION < 185
ProfileInfoArraySetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ProfileInfoArraySetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	//can I, should I force it to read-only this way?
	return JS_FALSE;
}


static JSClass ProfileInfoArrayClass = {
    "ProfileInfo",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    ProfileInfoArrayGetProperty, 
	ProfileInfoArraySetProperty, 
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static JSPropertySpec (ProfileInfoArrayProperties)[] = {
	{"length", -1, JSPROP_READONLY | JSPROP_SHARED | JSPROP_PERMANENT}, //JSPROP_ENUMERATE},
	{0}
};




//X3DRoute{
//SFNode sourceNode;
//String sourceField;
//SFNode destinationNode;
//String destinationField;
//}
struct CRStruct *getCRoutes();
int getCRouteCount();

JSBool
#if JS_VERSION < 185
X3DRouteGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
X3DRouteGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	int *ptr;
	int _index;
	JSString *_str;
	jsval rval;
	struct X3D_Node *fromNode, *toNode;
	int fromOffset, toOffset;
	const char *fieldname;
	jsval id;

	UNUSED(rval); // compiler warning mitigation

#if JS_VERSION >= 185
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in ProfileInfoGetProperty.\n");
		return JS_FALSE;
	}
#endif
	if ((ptr = (int *)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in ProfileInfoGetProperty.\n");
		return JS_FALSE;
	}
	_index = *ptr;
	//routes = getCRoutes();
	//route = routes[_index];
	getSpecificRoute (_index,&fromNode, &fromOffset, &toNode, &toOffset);
	//fromName = parser_getNameFromNode(fromNode);
	//toName   = parser_getNameFromNode(toNode);

		//fprintf (fp, " %p %s.%s TO %p %s.%s \n",fromNode,fromName,
		//	findFIELDNAMESfromNodeOffset0(fromNode,fromOffset),
		//	toNode,toName,
		//	findFIELDNAMESfromNodeOffset0(toNode,toOffset)
		//	);

    if (JSVAL_IS_INT(id)) 
	{
		int index = JSVAL_TO_INT(id);
		switch(index){
			case 0://sourceNode
			case 2://destinationNode
				//route.routeFromNode
				{
					JSObject *_obj;
					SFNodeNative *sfnn = malloc(sizeof(SFNodeNative));
					memset(sfnn,0,sizeof(SFNodeNative)); //I don't know if I'm supposed to set something else dug9 aug5,2013
					if(index==0)
						sfnn->handle = fromNode;
					if(index==2)
						sfnn->handle = toNode;
					
					_obj = JS_NewObject(cx,&SFNodeClass,NULL,obj);
					if (!JS_DefineProperties(cx, _obj, SFNodeProperties)) {
						printf( "JS_DefineProperties failed in Route sourceNode.\n");
						return JS_FALSE;
					}
					if (!JS_DefineFunctions(cx, _obj, SFNodeFunctions)) {
						printf( "JS_DefineFunctions failed in Route sourceNode.\n");
						return JS_FALSE;
					}

					if (!JS_SetPrivate(cx, _obj, (void*)sfnn)) {
						printf( "JS_SetPrivate failed in Route sourceNode.\n");
						return JS_FALSE;
					}

#if JS_VERSION < 185
					*rval = OBJECT_TO_JSVAL(_obj);
#else
					JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif
				}
				break;

			case 1://sourceField
				fieldname = findFIELDNAMESfromNodeOffset0(fromNode,fromOffset);
				_str = JS_NewStringCopyZ(cx,fieldname);
#if JS_VERSION < 185
				*rval = STRING_TO_JSVAL(_str);
#else
				JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(_str));
#endif
				break;
			case 3://destinationField
				fieldname = findFIELDNAMESfromNodeOffset0(toNode,toOffset);
				_str = JS_NewStringCopyZ(cx,fieldname);
#if JS_VERSION < 185
				*rval = STRING_TO_JSVAL(_str);
#else
				JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(_str));
#endif

				break;
		}
	}
	return JS_TRUE;
}
JSBool
#if JS_VERSION < 185
X3DRouteSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
X3DRouteSetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	//can I, should I force it to read-only this way?
	return JS_FALSE;
}

static JSClass X3DRouteClass = {
    "X3DRoute",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    X3DRouteGetProperty, 
	X3DRouteSetProperty, 
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static JSPropertySpec (X3DRouteProperties)[] = {
	//executionContext
	{"sourceNode", 0, JSPROP_ENUMERATE},
	{"sourceField", 1, JSPROP_ENUMERATE},
	{"destinationNode", 2, JSPROP_ENUMERATE},
	{"destinationField", 3, JSPROP_ENUMERATE},
	{0}
};


//ProfileInfoArray{
//numeric length;
//ProfileInfo [integer index];
//}

JSBool
#if JS_VERSION < 185
RouteArrayGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
RouteArrayGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	jsval rval;
	jsval id;

	UNUSED(rval); //compiler warning mitigation

#if JS_VERSION >= 185
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in RouteArrayGetProperty.\n");
		return JS_FALSE;
	}
#endif

    if (JSVAL_IS_INT(id)) 
	{
		int index = JSVAL_TO_INT(id);
		if(index == -1){
			int _length = getCRouteCount();
#if JS_VERSION < 185
			*rval = INT_TO_JSVAL(_length);
#else
			JS_SET_RVAL(cx,vp,INT_TO_JSVAL(_length));
#endif
		}else
		//if(index < getNumberOfProfiles() )
		{
			JSObject *_obj;
			int* _index = malloc(sizeof(int));
			_obj = JS_NewObject(cx,&X3DRouteClass,NULL,obj);
			*_index = index;
			if (!JS_DefineProperties(cx, _obj, X3DRouteProperties)) {
				printf( "JS_DefineProperties failed in RouteArray.\n");
				return JS_FALSE;
			}

			if (!JS_SetPrivate(cx, _obj, (void*)_index)) {
				printf( "JS_SetPrivate failed in RouteArray.\n");
				return JS_FALSE;
			}

#if JS_VERSION < 185
			*rval = OBJECT_TO_JSVAL(_obj);
#else
			JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif

		}
	}
	return JS_TRUE;
}
JSBool
#if JS_VERSION < 185
RouteArraySetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
RouteArraySetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	//can I, should I force it to read-only this way?
	return JS_FALSE;
}


static JSClass RouteArrayClass = {
    "RouteArray",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    RouteArrayGetProperty, 
	RouteArraySetProperty, 
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static JSPropertySpec (RouteArrayProperties)[] = {
	{"length", -1, JSPROP_READONLY | JSPROP_SHARED | JSPROP_PERMANENT}, //JSPROP_ENUMERATE},
	{0}
};




static JSFunctionSpec (ExecutionContextFunctions)[] = {
	//executionContext
	//{"addRoute", X3DExecutionContext_addRoute, 0},
	//{"deleteRoute", X3DExecutionContext_deleteRoute, 0},
	//{"createNode", X3DExecutionContext_createNode, 0},
	//{"createProto", X3DExecutionContext_createProto, 0},
	//{"getImportedNode", X3DExecutionContext_getImportedNode, 0},
	//{"updateImportedNode", X3DExecutionContext_updateImportedNode, 0},
	//{"removeImportedNode", X3DExecutionContext_removeImportedNode, 0},
	//{"getNamedNode", X3DExecutionContext_getNamedNode, 0},
	//{"updateNamedNode", X3DExecutionContext_updateNamedNode, 0},
	//{"removeNamedNode", X3DExecutionContext_removeNamedNode, 0},
	////scene
	//{"setMetaData", X3DScene_setMetaData, 0},
	//{"getMetaData", X3DScene_getMetaData, 0},
	//{"getExportedNode", X3DScene_getExportedNode, 0},
	//{"updateExportedNode", X3DScene_updateExportedNode, 0},
	//{"removeExportedNode", X3DScene_removeExportedNode, 0},
	{0}
};


static JSPropertySpec (ExecutionContextProperties)[] = {
	//executionContext
	{"specificationVersion", 0, JSPROP_ENUMERATE},
	{"encoding", 1, JSPROP_ENUMERATE},
	{"profile", 2, JSPROP_ENUMERATE},
	{"components", 3, JSPROP_ENUMERATE},
	{"worldURL", 4, JSPROP_ENUMERATE},
	{"rootNodes", 5, JSPROP_ENUMERATE},
	{"protos", 6, JSPROP_ENUMERATE},
	{"externprotos", 7, JSPROP_ENUMERATE},
	{"routes", 8, JSPROP_ENUMERATE},
	//scene
	//{"specificationVersion", 9, JSPROP_ENUMERATE}, //already done for executionContext above
	{"isScene", 9, JSPROP_ENUMERATE}, //else protoInstance. extra beyond specs - I think flux has it.
	{0}
};

//typedef struct _ExecutionContextNative {
//	struct X3D_Node *handle;
//} ExecutionContextNative;
typedef struct X3D_Node * ExecutionContextNative;

JSBool
#if JS_VERSION < 185
ExecutionContextGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ExecutionContextGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	ExecutionContextNative *ptr;
	JSString *_str;
	jsval rval;
	jsval id;

	UNUSED(rval); //compiler warning mitigation

#if JS_VERSION >= 185
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in ExecutionContextGetProperty.\n");
		return JS_FALSE;
	}
#endif

	if ((ptr = (ExecutionContextNative *)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in ExecutionContextGetProperty.\n");
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0: //specificationVersion string readonly
			{
				char cs[100];
				sprintf(cs,"{%d,%d,%d}",inputFileVersion[0],inputFileVersion[1],inputFileVersion[2]);
				_str = JS_NewStringCopyZ(cx,cs);
			}
#if JS_VERSION < 185
			*rval = STRING_TO_JSVAL(_str);
#else
			JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(_str));
#endif
			break;
		case 1: //encoding string readonly
			//Valid values are "ASCII", "VRML", "XML", "BINARY", "SCRIPTED", "BIFS", "NONE" 
			_str = JS_NewStringCopyZ(cx, "not filled in yet sb. VRML or XML or .."); 
#if JS_VERSION < 185
			*rval = STRING_TO_JSVAL(_str);
#else
			JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(_str));
#endif
			break;
		case 2: //profile ProfileInfo readonly
			{
				int index = gglobal()->Mainloop.scene_profile;

				JSObject *_obj;
				int* _index = malloc(sizeof(int));
				_obj = JS_NewObject(cx,&ProfileInfoClass,NULL,obj);
				*_index = index;
				if (!JS_DefineProperties(cx, _obj, ProfileInfoProperties)) {
					printf( "JS_DefineProperties failed in ExecutionContextProfileInfoProperties.\n");
					return JS_FALSE;
				}

				if (!JS_SetPrivate(cx, _obj, (void*)_index)) {
					printf( "JS_SetPrivate failed in ExecutionContextProfileInfoArray.\n");
					return JS_FALSE;
				}

#if JS_VERSION < 185
				*rval = OBJECT_TO_JSVAL(_obj);
#else
				JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif
			}
			break;
		case 3: //components ComponentInfoArray readonly
			{
				JSObject *_obj;
				//int ncomp;
				const int *_table = gglobal()->Mainloop.scene_components; //capabilitiesHandler_getProfileComponent(_index);
				//ncomp = capabilitiesHandler_getTableLength(_table);
				//malloc private not needed
				_obj = JS_NewObject(cx,&ComponentInfoArrayClass,NULL,obj);
				if (!JS_DefineProperties(cx, _obj, ComponentInfoArrayProperties)) {
					printf( "JS_DefineProperties failed in ExecutionContext_ComponentInfoArrayProperties.\n");
					return JS_FALSE;
				}
			
				if (!JS_SetPrivate(cx, _obj, (void*)_table)) {
					printf( "JS_SetPrivate failed in ExecutionContext_ComponentInfoArray.\n");
					return JS_FALSE;
				}
#if JS_VERSION < 185
				*rval = OBJECT_TO_JSVAL(_obj);
#else
				JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif
			}
			break;
		case 4: //worldURL string readonly
			_str = JS_NewStringCopyZ(cx, gglobal()->Mainloop.url);
#if JS_VERSION < 185
			*rval = STRING_TO_JSVAL(_str);
#else
			JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(_str));
#endif
			break;
		case 5: //rootNodes MFNode (readonly if !isScene, else rw)
			{
				JSObject *_obj;
				//MFNodeNative *mfn;

				//struct X3D_Group* scene = (struct X3D_Group*)(struct X3D_Node*)ptr; //->handle;
				return JS_FALSE;

				//scene->children;
				//somehow return children as an MFNode
				//mfn = malloc(sizeof(MFNodeNative));
				_obj = JS_NewObject(cx,&MFNodeClass,NULL,obj);

				//mfn->handle = (struct X3D_Node*)rootNode(); //change this to (Script)._executionContext when brotos working fully
				//if (!JS_DefineProperties(cx, _obj, MFNodeProperties)) {
				//	printf( "JS_DefineProperties failed in SFRotationConstr.\n");
				//	return JS_FALSE;
				//}
				if (!JS_DefineFunctions(cx, _obj, MFNodeFunctions)) {
					printf( "JS_DefineProperties failed in SFRotationConstr.\n");
					return JS_FALSE;
				}
//OUCH NEEDS WORK i DON'T KNOW WHAT I'M DOING
				//if (!JS_SetPrivate(cx, _obj, &scene->children)) {
				//	printf( "JS_SetPrivate failed in ExecutionContext.\n");
				//	return JS_FALSE;
				//}

#if JS_VERSION < 185
				*rval = OBJECT_TO_JSVAL(_obj);
#else
				JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif
			}
			break;
		case 6: //protos protoDeclarationArray  rw
		case 7: //externprotos externProtoDeclarationArray rw
			return JS_FALSE;
		case 8: //routes RouteArray readonly
			{
				JSObject *_obj;
				//malloc private not needed
				_obj = JS_NewObject(cx,&RouteArrayClass,NULL,obj);
				if (!JS_DefineProperties(cx, _obj, RouteArrayProperties)) {
					printf( "JS_DefineProperties failed in ExecutionContext_X3DRouteArrayProperties.\n");
					return JS_FALSE;
				}
					//if (!JS_SetPrivate(cx, _obj, (void*)_table)) {
				//	printf( "JS_SetPrivate failed in ExecutionContext_X3DRouteArray.\n");
				//	return JS_FALSE;
				//}
#if JS_VERSION < 185
				*rval = OBJECT_TO_JSVAL(_obj);
#else
				JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif
			}
			break;
		case 9: //isScene readonly (extra to specs)
			//once brotos are working then the main scene broto will need a flag to say it's a scene
			JS_SET_RVAL(cx,vp,BOOLEAN_TO_JSVAL(JS_TRUE));
			break;
		}
	}
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
ExecutionContextSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
ExecutionContextSetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	//can I, should I force it to read-only this way?
	return JS_FALSE;
}
static JSClass ExecutionContextClass = {
    "ExecutionContext",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    ExecutionContextGetProperty, 
	ExecutionContextSetProperty,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};


static JSPropertySpec (BrowserProperties)[] = {
	{"name", 0, JSPROP_ENUMERATE},
	{"version", 1, JSPROP_ENUMERATE},
	{"currentSpeed", 2, JSPROP_ENUMERATE},
	{"currentFrameRate", 3, JSPROP_ENUMERATE},
	{"description", 4, JSPROP_ENUMERATE},
	{"supportedComponents", 5, JSPROP_ENUMERATE},
	{"supportedProfiles", 6, JSPROP_ENUMERATE},
	{"currentScene", 7, JSPROP_ENUMERATE},
	{0}
};

JSBool
#if JS_VERSION < 185
BrowserGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
BrowserGetProperty(JSContext *cx, JSObject *obj, jsid iid, jsval *vp)
#endif
{
	BrowserNative *ptr;
	jsdouble d;
	JSString *_str;
	jsval rval;
	jsval id;

	UNUSED(rval); // compiler warning mitigation

#if JS_VERSION >= 185
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in BrowserGetProperty.\n");
		return JS_FALSE;
	}
#endif

	//right now we don't need/use the ptr to BrowserNative which is a stub struct, 
	//because browser is conceptually a global static singleton 
	//(or more precisely 1:1 with a gglobal[i] 'browser instance' for things like framerate, 
	// and 1:1 with static for unchanging things like browser version, components and profiles supported), 
	//and in practice all the bits and pieces are scattered throughout freewrl
	//but for fun we'll get it:
	if ((ptr = (BrowserNative *)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in BrowserGetProperty.\n");
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0: //name
			_str = JS_NewStringCopyZ(cx,BrowserName);
#if JS_VERSION < 185
			*rval = STRING_TO_JSVAL(_str);
#else
			JS_SET_RVAL(cx,vp,STRING_TO_JSVAL(_str));
#endif
			break;
		case 1: //version
			_str = JS_NewStringCopyZ(cx, libFreeWRL_get_version());
#if JS_VERSION < 185
			*rval = STRING_TO_JSVAL(_str);
#else
			JS_SET_RVAL(context,vp,STRING_TO_JSVAL(_str));
#endif
			break;
		case 2: //currentSpeed
			/* get the variable updated */
			getCurrentSpeed();
			d = gglobal()->Mainloop.BrowserSpeed;
			if (JS_NewNumberValue(cx, d, vp) == JS_FALSE) {
				printf("JS_NewDouble failed for %f in BrowserGetProperty.\n",d);
				return JS_FALSE;
			}
			break;
		case 3: //currentFrameRate
			d = gglobal()->Mainloop.BrowserFPS;
			if (JS_NewNumberValue(cx, d, vp) == JS_FALSE) {
				printf("JS_NewDouble failed for %f in BrowserGetProperty.\n",d);
				return JS_FALSE;
			}
			break;
		case 4: //description
			_str = JS_NewStringCopyZ(cx, get_status());
#if JS_VERSION < 185
			*rval = STRING_TO_JSVAL(_str);
#else
			JS_SET_RVAL(context,vp,STRING_TO_JSVAL(_str));
#endif
			break;
		case 5: //supportedComponents
			{
				JSObject *_obj;
				//malloc private not needed
				_obj = JS_NewObject(cx,&ComponentInfoArrayClass,NULL,obj);
				if (!JS_DefineProperties(cx, _obj, ComponentInfoArrayProperties)) {
					printf( "JS_DefineProperties failed in ComponentInfoArrayProperties.\n");
					return JS_FALSE;
				}
				//if (!JS_DefineFunctions(cx, _obj, ProfileInfoArrayFunctions)) {
				//	printf( "JS_DefineProperties failed in ExecutionContextFunctions.\n");
				//	return JS_FALSE;
				//}
				if (!JS_SetPrivate(cx, _obj, (void*)capabilitiesHandler_getCapabilitiesTable())) {
					printf( "JS_SetPrivate failed in ExecutionContext.\n");
					return JS_FALSE;
				}

#if JS_VERSION < 185
				*rval = OBJECT_TO_JSVAL(_obj);
#else
				JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif

			}
			break;

		case 6: //supportedProfiles
			{
				JSObject *_obj;
				//malloc private not needed
				_obj = JS_NewObject(cx,&ProfileInfoArrayClass,NULL,obj);
				if (!JS_DefineProperties(cx, _obj, ProfileInfoArrayProperties)) {
					printf( "JS_DefineProperties failed in ExecutionContextProperties.\n");
					return JS_FALSE;
				}
				//if (!JS_DefineFunctions(cx, _obj, ProfileInfoArrayFunctions)) {
				//	printf( "JS_DefineProperties failed in ExecutionContextFunctions.\n");
				//	return JS_FALSE;
				//}
				//set private not needed
				//if (!JS_SetPrivate(cx, _obj, ec)) {
				//	printf( "JS_SetPrivate failed in ExecutionContext.\n");
				//	return JS_FALSE;
				//}

#if JS_VERSION < 185
				*rval = OBJECT_TO_JSVAL(_obj);
#else
				JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif

			}
			break;
		case 7: //currentScene
#ifdef sceneIsBroto  
			//someday soon I hope, the ScriptNode._executionContext might be working, 
			//and give you the currentScene native pointer to put as PRIVATE in BrowserNative
#else
			//in theory it's rootNode() in
			//struct X3D_Group *rootNode()
			//H: I have to return an ExecutionContextNative here with its guts set to our rootNode or ???
			{
				JSObject *_obj;
				ExecutionContextNative ec = malloc(sizeof(ExecutionContextNative));
				_obj = JS_NewObject(cx,&ExecutionContextClass,NULL,obj);

				//ec->handle = (struct X3D_Node*)rootNode(); //change this to (Script)._executionContext when brotos working fully
				ec = (struct X3D_Node*)rootNode(); //change this to (Script)._executionContext when brotos working fully
				if (!JS_DefineProperties(cx, _obj, ExecutionContextProperties)) {
					printf( "JS_DefineProperties failed in ExecutionContextProperties.\n");
					return JS_FALSE;
				}
				if (!JS_DefineFunctions(cx, _obj, ExecutionContextFunctions)) {
					printf( "JS_DefineProperties failed in ExecutionContextFunctions.\n");
					return JS_FALSE;
				}

				if (!JS_SetPrivate(cx, _obj, ec)) {
					printf( "JS_SetPrivate failed in ExecutionContext.\n");
					return JS_FALSE;
				}

#if JS_VERSION < 185
				*rval = OBJECT_TO_JSVAL(_obj);
#else
				JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(_obj));
#endif

			}

#endif
		}
	}
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
BrowserSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
BrowserSetProperty(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp)
#endif
{
	BrowserNative *ptr;
	jsval _val;
	JSString *ss;
	char *cs;
#if JS_VERSION >= 185
	jsval id;
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("JS_IdToValue failed in BrowserSetProperty.\n");
		return JS_FALSE;
	}
#endif

	if ((ptr = (BrowserNative *)JS_GetPrivate(cx, obj)) == NULL) {
		printf( "JS_GetPrivate failed in BrowserSetProperty.\n");
		return JS_FALSE;
	}
	//ptr->valueChanged++;

	if (!JS_ConvertValue(cx, *vp, JSTYPE_STRING, &_val)) {
		printf( "JS_ConvertValue failed in BrowserSetProperty.\n");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		switch (JSVAL_TO_INT(id)) {
		case 0:
		case 1:
		case 2:
		case 3:
			return JS_FALSE;
		case 4: //description
			ss = JS_ValueToString(cx, _val);
			cs = JS_EncodeString(cx, ss);
			update_status(cs);  //script node setting the statusbar text
			break;
		case 5:
		case 6:
		case 7:
			return JS_FALSE;

		}
	}
	return JS_TRUE;
}


#endif


///* for setting field values to the output of a CreateVrml style of call */
///* it is kept at zero, unless it has been used. Then it is reset to zero */
//jsval JSCreate_global_return_val;
typedef struct pjsVRMLBrowser{
	int ijunk;
#ifdef HAVE_JAVASCRIPT
	jsval JSCreate_global_return_val;
#endif // HAVE_JAVASCRIPT

}* ppjsVRMLBrowser;
void *jsVRMLBrowser_constructor(){
	void *v = malloc(sizeof(struct pjsVRMLBrowser));
	memset(v,0,sizeof(struct pjsVRMLBrowser));
	return v;
}
void jsVRMLBrowser_init(struct tjsVRMLBrowser *t){
	//public
	//private
	t->prv = jsVRMLBrowser_constructor();
	{
		ppjsVRMLBrowser p = (ppjsVRMLBrowser)t->prv;
		/* Script name/type table */
#ifdef HAVE_JAVASCRIPT
		t->JSCreate_global_return_val = &p->JSCreate_global_return_val;
#endif // HAVE_JAVASCRIPT
	}

}
//	ppjsVRMLBrowser p = (ppjsVRMLBrowser)gglobal()->jsVRMLBrowser.prv;
/* we add/remove routes with this call */
void jsRegisterRoute(
	struct X3D_Node* from, int fromOfs,
	struct X3D_Node* to, int toOfs,
	int len, const char *adrem) {
	int ad;

	if (strcmp("addRoute",adrem) == 0) 
		ad = 1;
	else ad = 0;

 	CRoutes_Register(ad, from, fromOfs, to, toOfs , len, 
 		 returnInterpolatorPointer(stringNodeType(to->_nodeType)), 0, 0);
}
 

/* used in loadURL*/
void conCat (char *out, char *in) {

	while (strlen (in) > 0) {
		strcat (out," :loadURLStringBreak:");
		while (*out != '\0') out++;

		if (*in == '[') in++;
		while ((*in != '\0') && (*in == ' ')) in++;
		if (*in == '"') {
			in++;
			/* printf ("have the initial quote string here is %s\n",in); */
			while (*in != '"') { *out = *in; out++; in++; }
			*out = '\0';
			/* printf ("found string is :%s:\n",tfilename); */
		}

		/* skip along to the start of the next name */
		if (*in == '"') in++;
		if (*in == ',') in++;
		if (*in == ']') in++; /* this allows us to leave */
	}
}



void createLoadUrlString(char *out, int outLen, char *url, char *param) {
	int commacount1;
	int commacount2;
	char *tptr;

	/* mimic the EAI loadURL, java code is:
        // send along sizes of the Strings
        SysString = "" + url.length + " " + parameter.length;
                
        for (count=0; count<url.length; count++) {
                SysString = SysString + " :loadURLStringBreak:" + url[count];
        }       

        for (count=0; count<parameter.length; count++) {
                SysString = SysString + " :loadURLStringBreak:" + parameter[count];
        }
	*/

	/* find out how many elements there are */

	commacount1 = 0; commacount2 = 0;
	tptr = url; while (*tptr != '\0') { if (*tptr == '"') commacount1 ++; tptr++; }
	tptr = param; while (*tptr != '\0') { if (*tptr == '"') commacount2 ++; tptr++; }
	commacount1 = commacount1 / 2;
	commacount2 = commacount2 / 2;

	if ((	strlen(url) +
		strlen(param) +
		(commacount1 * strlen (" :loadURLStringBreak:")) +
		(commacount2 * strlen (" :loadURLStringBreak:"))) > (outLen - 20)) {
		printf ("createLoadUrlString, string too long\n");
		return;
	}

	sprintf (out,"%d %d",commacount1,commacount2);
	
	/* go to the end of this string */
	while (*out != '\0') out++;

	/* go through the elements and find which (if any) url exists */	
	conCat (out,url);
	while (*out != '\0') out++;
	conCat (out,param);
}


JSBool
VrmlBrowserInit(JSContext *context, JSObject *globalObj, BrowserNative *brow)
{
	JSObject *obj;
	ttglobal tg = gglobal();
	*(jsval *)tg->jsVRMLBrowser.JSCreate_global_return_val = INT_TO_JSVAL(0);

	#ifdef JSVERBOSE
		printf("VrmlBrowserInit\n");
	#endif

	obj = JS_DefineObject(context, globalObj, "Browser", &Browser, NULL, 
			JSPROP_ENUMERATE | JSPROP_PERMANENT);
	if (!JS_DefineFunctions(context, obj, BrowserFunctions)) {
		printf( "JS_DefineFunctions failed in VrmlBrowserInit.\n");
		return JS_FALSE;
	}
#ifdef X3DBROWSER

	if (!JS_DefineProperties(context, obj, BrowserProperties)) {
		printf( "JS_DefineProperties failed in VrmlBrowserInit.\n");
		return JS_FALSE;
	}
#endif
	if (!JS_SetPrivate(context, obj, brow)) {
		printf( "JS_SetPrivate failed in VrmlBrowserInit.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
VrmlBrowserGetName(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
VrmlBrowserGetName(JSContext *context, uintN argc, jsval *vp) {
	JSObject *obj = JS_THIS_OBJECT(context,vp);
	jsval *argv = JS_ARGV(context,vp);
#endif
	JSString *_str;

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	_str = JS_NewStringCopyZ(context,BrowserName);
#if JS_VERSION < 185
	*rval = STRING_TO_JSVAL(_str);
#else
	JS_SET_RVAL(context,vp,STRING_TO_JSVAL(_str));
#endif
	return JS_TRUE;
}


/* get the string stored in FWVER into a jsObject */
JSBool
#if JS_VERSION < 185
VrmlBrowserGetVersion(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
VrmlBrowserGetVersion(JSContext *context, uintN argc, jsval *vp) {
	JSObject *obj = JS_THIS_OBJECT(context,vp);
	jsval *argv = JS_ARGV(context,vp);
#endif
	JSString *_str;

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	_str = JS_NewStringCopyZ(context, libFreeWRL_get_version());
#if JS_VERSION < 185
	*rval = STRING_TO_JSVAL(_str);
#else
	JS_SET_RVAL(context,vp,STRING_TO_JSVAL(_str));
#endif
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
VrmlBrowserGetCurrentSpeed(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
VrmlBrowserGetCurrentSpeed(JSContext *context, uintN argc, jsval *vp) {
	JSObject *obj = JS_THIS_OBJECT(context,vp);
	jsval *argv = JS_ARGV(context,vp);
#endif
	JSString *_str;
	char string[1000];

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	/* get the variable updated */
	getCurrentSpeed();
	sprintf (string,"%f",gglobal()->Mainloop.BrowserSpeed);
	_str = JS_NewStringCopyZ(context,string);
#if JS_VERSION < 185
        *rval = STRING_TO_JSVAL(_str);
#else
        JS_SET_RVAL(context,vp,STRING_TO_JSVAL(_str));
#endif
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
VrmlBrowserGetCurrentFrameRate(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
VrmlBrowserGetCurrentFrameRate(JSContext *context, uintN argc, jsval *vp) {
	JSObject *obj = JS_THIS_OBJECT(context,vp);
	jsval *argv = JS_ARGV(context,vp);
#endif
	JSString *_str;
	char FPSstring[1000];

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	sprintf (FPSstring,"%6.2f",gglobal()->Mainloop.BrowserFPS);
	_str = JS_NewStringCopyZ(context,FPSstring);
#if JS_VERSION < 185
        *rval = STRING_TO_JSVAL(_str);
#else
        JS_SET_RVAL(context,vp,STRING_TO_JSVAL(_str));
#endif
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
VrmlBrowserGetWorldURL(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
VrmlBrowserGetWorldURL(JSContext *context, uintN argc, jsval *vp) {
        JSObject *obj = JS_THIS_OBJECT(context,vp);
        jsval *argv = JS_ARGV(context,vp);
#endif
	JSString *_str;

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	_str = JS_NewStringCopyZ(context,BrowserFullPath);
#if JS_VERSION < 185
        *rval = STRING_TO_JSVAL(_str);
#else
        JS_SET_RVAL(context,vp,STRING_TO_JSVAL(_str));
#endif
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
VrmlBrowserReplaceWorld(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
VrmlBrowserReplaceWorld(JSContext *context, uintN argc, jsval *vp) {
        jsval *argv = JS_ARGV(context,vp);
#endif
	JSObject *_obj;
	JSString *_str;
	JSClass *_cls;
	jsval _rval = INT_TO_JSVAL(0);
	char *_c_args = "MFNode nodes",
		*_costr,
		*_c_format = "o";
	char *tptr;

	if (JS_ConvertArguments(context, argc, argv, _c_format, &_obj)) {
		if ((_cls = JS_GET_CLASS(context, _obj)) == NULL) {
			printf("JS_GetClass failed in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}

		if (memcmp("MFNode", _cls->name, strlen(_cls->name)) != 0) {
			printf( "\nIncorrect argument in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}
		_str = JS_ValueToString(context, argv[0]);
#if JS_VERSION < 185
		_costr = JS_GetStringBytes(_str);
#else
		_costr = JS_EncodeString(context,_str);
#endif
		/* sanitize string, for the EAI_RW call (see EAI_RW code) */
		tptr = _costr;
		while (*tptr != '\0') {
			if(*tptr == '[') *tptr = ' ';
			if(*tptr == ']') *tptr = ' ';
			if(*tptr == ',') *tptr = ' ';
			tptr++;
		}
		EAI_RW(_costr);
#if JS_VERSION >= 185
		JS_free(context,_costr);
#endif
	} else {
		printf( "\nIncorrect argument format for replaceWorld(%s).\n", _c_args);
		return JS_FALSE;
	}
#if JS_VERSION < 185
	*rval = _rval;
#else
	JS_SET_RVAL(context,vp,_rval);
#endif

	return JS_TRUE;
}
struct X3D_Anchor* get_EAIEventsIn_AnchorNode();
JSBool
#if JS_VERSION < 185
VrmlBrowserLoadURL(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
VrmlBrowserLoadURL(JSContext *context, uintN argc, jsval *vp) {
        jsval *argv = JS_ARGV(context,vp);
#endif
	JSObject *_obj[2];
	JSString *_str[2];
	JSClass *_cls[2];
	char *_c_args = "MFString url, MFString parameter",
		*_costr[2],
		*_c_format = "o o";
	#define myBufSize 2000
	char myBuf[myBufSize];

	if (JS_ConvertArguments(context, argc, argv, _c_format, &(_obj[0]), &(_obj[1]))) {
		if ((_cls[0] = JS_GET_CLASS(context, _obj[0])) == NULL) {
			printf( "JS_GetClass failed for arg 0 in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GET_CLASS(context, _obj[1])) == NULL) {
			printf( "JS_GetClass failed for arg 1 in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		if (memcmp("MFString", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("MFString", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			printf( "\nIncorrect arguments in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		_str[0] = JS_ValueToString(context, argv[0]);
#if JS_VERSION < 185
		_costr[0] = JS_GetStringBytes(_str[0]);
#else
		_costr[0] = JS_EncodeString(context,_str[0]);
#endif

		_str[1] = JS_ValueToString(context, argv[1]);
#if JS_VERSION < 185
		_costr[1] = JS_GetStringBytes(_str[1]);
#else
		_costr[1] = JS_EncodeString(context,_str[1]);
#endif

		/* we use the EAI code for this - so reformat this for the EAI format */
		{
			//extern struct X3D_Anchor EAI_AnchorNode;  /* win32 C doesnt like new declarations in the middle of executables - start a new scope {} and put dec at top */

			/* make up the URL from what we currently know */
			createLoadUrlString(myBuf,myBufSize,_costr[0], _costr[1]);
			createLoadURL(myBuf);

			/* now tell the fwl_RenderSceneUpdateScene that BrowserAction is requested... */
			setAnchorsAnchor( get_EAIEventsIn_AnchorNode()); //&gglobal()->EAIEventsIn.EAI_AnchorNode;
		}
		gglobal()->RenderFuncs.BrowserAction = TRUE;

#if JS_VERSION >= 185
		JS_free(context,_costr[0]);
		JS_free(context,_costr[1]);
#endif
	} else {
		printf( "\nIncorrect argument format for loadURL(%s).\n", _c_args);
		return JS_FALSE;
	}
#if JS_VERSION < 185
	*rval = INT_TO_JSVAL(0);
#else
	JS_SET_RVAL(context,vp,JSVAL_ZERO);
#endif

	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
VrmlBrowserSetDescription(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *_c_format = "s";
#else
VrmlBrowserSetDescription(JSContext *context, uintN argc, jsval *vp) {
        jsval *argv = JS_ARGV(context,vp);
	JSString *js_c;
	char *_c_format = "S";
#endif
	char *_c, *_c_args = "SFString description";

	UNUSED(_c); // compiler warning mitigation

	if (argc == 1 &&
#if JS_VERSION < 185
		JS_ConvertArguments(context, argc, argv, _c_format, &_c)) {
#else
		JS_ConvertArguments(context, argc, argv, _c_format, &js_c)) {
			/* _c = JS_EncodeString(context,js_c);
			...why encode the string when we just have to JS_free it later? */
#endif

		/* we do not do anything with the description. If we ever wanted to, it is in _c */
#if JS_VERSION < 185
		*rval = INT_TO_JSVAL(0);
#else
		JS_SET_RVAL(context,vp,JSVAL_ZERO);
#endif
	} else {
		printf( "\nIncorrect argument format for setDescription(%s).\n", _c_args);
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
VrmlBrowserCreateVrmlFromString(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *_c_format = "s";
#else
VrmlBrowserCreateVrmlFromString(JSContext *context, uintN argc, jsval *vp) {
        JSObject *obj = JS_THIS_OBJECT(context,vp);
        jsval *argv = JS_ARGV(context,vp);
	jsval _my_rval;
	jsval *rval = &_my_rval;
	char *_c_format = "S";
	JSString *js_c;
#endif
	char *_c, *_c_args = "SFString vrmlSyntax";

	/* for the return of the nodes */
	struct X3D_Group *retGroup;
	char *xstr; 
	char *tmpstr;
	char *separator;
	int ra;
	int count;
	int wantedsize;
	int MallocdSize;
	ttglobal tg = gglobal();
	struct VRMLParser *globalParser = (struct VRMLParser *)tg->CParse.globalParser;
	
	UNUSED(ra); //compiler warning mitigation

	/* make this a default value */
	*rval = INT_TO_JSVAL(0);

	if (argc == 1 &&
#if JS_VERSION < 185
		JS_ConvertArguments(context, argc, argv, _c_format, &_c)) {
#else
		JS_ConvertArguments(context, argc, argv, _c_format, &js_c)) {
			_c = JS_EncodeString(context,js_c);
#endif
		#ifdef JSVERBOSE
			printf("VrmlBrowserCreateVrmlFromString: obj = %u, str = \"%s\"\n",
				   obj, _c);
		#endif

		/* do the call to make the VRML code  - create a new browser just for this string */
		gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
		retGroup = createNewX3DNode(NODE_Group);
		ra = EAI_CreateVrml("String",_c,X3D_NODE(retGroup),retGroup);
		globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */


		/* and, make a string that we can use to create the javascript object */
		MallocdSize = 200;
		xstr = MALLOC (char *, MallocdSize);
		strcpy (xstr,"new MFNode(");
		separator = " ";
		for (count=0; count<retGroup->children.n; count ++) {
			tmpstr = MALLOC(char *, strlen(_c) + 100);
			sprintf (tmpstr,"%s new SFNode('%s','%p')",separator, _c, (void*) retGroup->children.p[count]);
			// +1 to account for the ")" being added later ...
			wantedsize = (int) (strlen(tmpstr) + strlen(xstr) + 1);
			// sometimes wantedsize is borderline so alloc some more if it's equal
			if (wantedsize >= MallocdSize) {
				MallocdSize = wantedsize +200;
				xstr = REALLOC (xstr,MallocdSize);
			}
			
			
			strncat (xstr,tmpstr,strlen(tmpstr));
			FREE_IF_NZ (tmpstr);
			separator = ", ";
		}
		strcat (xstr,")");
		markForDispose(X3D_NODE(retGroup),FALSE);

#if JS_VERSION >= 185
		JS_free(context,_c);
#endif
		
		#ifdef JSVERBOSE
		printf ("running runscript on :%s:\n",xstr);
		#endif

		/* create this value NOTE: rval is set here. */
		jsrrunScript(context, obj, xstr, rval);
		FREE_IF_NZ (xstr);

	} else {
		printf("\nIncorrect argument format for createVrmlFromString(%s).\n", _c_args);
		return JS_FALSE;
	}

	/* save this value, in case we need it */
#if JS_VERSION < 185
	tg->jsVRMLBrowser.JSCreate_global_return_val = *rval;
#else
	JS_SET_RVAL(context,vp,*rval);
#endif
	return JS_TRUE;
}

JSBool
#if JS_VERSION < 185
VrmlBrowserCreateX3DFromString(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *_c_format = "s";
#else
VrmlBrowserCreateX3DFromString(JSContext *context, uintN argc, jsval *vp) {
        JSObject *obj = JS_THIS_OBJECT(context,vp);
        jsval *argv = JS_ARGV(context,vp);
	jsval _my_rval;
	jsval *rval = &_my_rval;
	char *_c_format = "S";
	JSString *js_c;
#endif
	char *_c, *_c_args = "SFString x3dSyntax"; //x3d

	/* for the return of the nodes */
	struct X3D_Group *retGroup;
	char *xstr; 
	char *tmpstr;
	char *separator;
	int ra;
	int count;
	int wantedsize;
	int MallocdSize;
	//ttglobal tg = gglobal();
	//struct VRMLParser *globalParser = (struct VRMLParser *)tg->CParse.globalParser;
	
	UNUSED(ra); //compiler warning mitigation

	/* make this a default value */
	*rval = INT_TO_JSVAL(0);

	if (argc == 1 &&
#if JS_VERSION < 185
		JS_ConvertArguments(context, argc, argv, _c_format, &_c)) {
#else
		JS_ConvertArguments(context, argc, argv, _c_format, &js_c)) {
			_c = JS_EncodeString(context,js_c);
#endif
		#ifdef JSVERBOSE
			printf("VrmlBrowserCreateVrmlFromString: obj = %u, str = \"%s\"\n",
				   obj, _c);
		#endif

		/* do the call to make the VRML code  - create a new browser just for this string */
		//gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
		retGroup = createNewX3DNode(NODE_Group);
		ra = EAI_CreateX3d("String",_c,X3D_NODE(retGroup),retGroup);
		//globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */


		/* and, make a string that we can use to create the javascript object */
		MallocdSize = 200;
		xstr = MALLOC (char *, MallocdSize);
		strcpy (xstr,"new MFNode(");
		separator = " ";
		for (count=0; count<retGroup->children.n; count ++) {
			tmpstr = MALLOC(char *, strlen(_c) + 100);
			sprintf (tmpstr,"%s new SFNode('%s','%p')",separator, _c, (void*) retGroup->children.p[count]);
			wantedsize = (int) (strlen(tmpstr) + strlen(xstr));
			if (wantedsize > MallocdSize) {
				MallocdSize = wantedsize +200;
				xstr = REALLOC (xstr,MallocdSize);
			}
			
			
			strncat (xstr,tmpstr,strlen(tmpstr));
			FREE_IF_NZ (tmpstr);
			separator = ", ";
		}
		strcat (xstr,")");
		markForDispose(X3D_NODE(retGroup),FALSE);

#if JS_VERSION >= 185
		JS_free(context,_c);
#endif
		
		#ifdef JSVERBOSE
		printf ("running runscript on :%s:\n",xstr);
		#endif

		/* create this value NOTE: rval is set here. */
		jsrrunScript(context, obj, xstr, rval);
		FREE_IF_NZ (xstr);

	} else {
		printf("\nIncorrect argument format for createVrmlFromString(%s).\n", _c_args);
		return JS_FALSE;
	}

	/* save this value, in case we need it */
#if JS_VERSION < 185
	*(jsval*)(gglobal()->jsVRMLBrowser.JSCreate_global_return_val) = *rval;
#else
	JS_SET_RVAL(context,vp,*rval);
#endif
	return JS_TRUE;
}


JSBool
#if JS_VERSION < 185
VrmlBrowserCreateVrmlFromURL(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
#else
VrmlBrowserCreateVrmlFromURL(JSContext *context, uintN argc, jsval *vp) {
        jsval *argv = JS_ARGV(context,vp);
	jsval _my_rval;
	jsval *rval = &_my_rval;
#endif
	JSString *_str[2];
	JSClass *_cls[2];
	SFNodeNative *oldPtr;
	char *fieldStr,
		*_costr0;
	struct X3D_Node *myptr;
	#define myFileSizeLimit 4000

/* DJ Tue May  4 21:25:15 BST 2010 Old stuff, no longer applicable
	int count;
	int offset;
	int fromtype;
	int xxx;
	int myField;
	char *address;
	struct X3D_Group *subtree;
*/
	resource_item_t *res = NULL;
	int fieldInt;
	int offs;
	int type;
	int accessType;
	struct Multi_String url;


	#ifdef JSVERBOSE
	printf ("JS start of createVrmlFromURL\n");
	#endif

	/* rval is always zero, so lets just set it */
#if JS_VERSION < 185
	*rval = INT_TO_JSVAL(0);
#else
	*rval = JSVAL_ZERO;
#endif

	/* first parameter - expect a MFString Object here */
	if (JSVAL_IS_OBJECT(argv[0])) {
		if ((_cls[0] = JS_GET_CLASS(context, JSVAL_TO_OBJECT(argv[0]))) == NULL) {
                        printf( "JS_GetClass failed for arg 0 in VrmlBrowserLoadURL.\n");
                        return JS_FALSE;
                }
	} else {
		printf ("VrmlBrowserCreateVrmlFromURL - expect first parameter to be an object\n");
		return JS_FALSE;
	}

	/* second parameter - expect a SFNode Object here */
	if (JSVAL_IS_OBJECT(argv[1])) {
		if ((_cls[1] = JS_GET_CLASS(context, JSVAL_TO_OBJECT(argv[1]))) == NULL) {
                        printf( "JS_GetClass failed for arg 1 in VrmlBrowserLoadURL.\n");
                        return JS_FALSE;
                }
	} else {
		printf ("VrmlBrowserCreateVrmlFromURL - expect first parameter to be an object\n");
		return JS_FALSE;
	}

	#ifdef JSVERBOSE
	printf ("JS createVrml - step 2\n");
	printf ("JS create - we should havve a MFString and SFNode, have :%s: :%s:\n",(_cls[0])->name, (_cls[1])->name);
	#endif

	/* make sure these 2 objects are really MFString and SFNode */
	if (memcmp("MFString", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
		memcmp("SFNode", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
		printf( "Incorrect arguments in VrmlBrowserLoadURL.\n");
		return JS_FALSE;
	}

	/* third parameter should be a string */
	if (JSVAL_IS_STRING(argv[2])) {
		_str[1] = JSVAL_TO_STRING(argv[2]);
#if JS_VERSION < 185
		fieldStr = JS_GetStringBytes(_str[1]);
#else
		fieldStr = JS_EncodeString(context,_str[1]);
#endif
		#ifdef JSVERBOSE
		printf ("field string is :%s:\n",fieldStr); 
		#endif
	 } else {
		printf ("Expected a string in createVrmlFromURL\n");
		return JS_FALSE;
	}

	#ifdef JSVERBOSE
	printf ("passed object type tests\n");
	#endif

	/* get the URL listing as a string */
	_str[0] = JS_ValueToString(context, argv[0]);
#if JS_VERSION < 185
	_costr0 = JS_GetStringBytes(_str[0]);
#else
	_costr0 = JS_EncodeString(context,_str[0]);
#endif


	#ifdef JSVERBOSE
	printf ("URL string is %s\n",_costr0);
	#endif


	/* get a pointer to the SFNode structure, in order to properly place the new string */
	if ((oldPtr = (SFNodeNative *)JS_GetPrivate(context, JSVAL_TO_OBJECT(argv[1]))) == NULL) {
		printf( "JS_GetPrivate failed in VrmlBrowserLoadURL for SFNode parameter.\n");
#if JS_VERSION >= 185
		JS_free(context,_costr0);
		JS_free(context,fieldStr);
#endif
		return JS_FALSE;
	}
	myptr = X3D_NODE(oldPtr->handle);
	if (myptr == NULL) {
		printf ("CreateVrmlFromURL, internal error - SFNodeNative memory pointer is NULL\n");
#if JS_VERSION >= 185
		JS_free(context,_costr0);
		JS_free(context,fieldStr);
#endif
		return JS_FALSE;
	}


	#ifdef JSVERBOSE
	printf ("SFNode handle %d, old X3DString %s\n",oldPtr->handle, oldPtr->X3DString);
	printf ("myptr %d\n",myptr);
	printf ("points to a %s\n",stringNodeType(myptr->_nodeType));
	#endif


	/* bounds checks */
	if (sizeof (_costr0) > (myFileSizeLimit-200)) {
		printf ("VrmlBrowserCreateVrmlFromURL, url too long...\n");
#if JS_VERSION >= 185
		JS_free(context,_costr0);
		JS_free(context,fieldStr);
#endif
		return JS_FALSE;
	}

	/* ok - here we have:
		_costr0		: the url string array; eg: [ "vrml.wrl" ]
		opldPtr		: pointer to a SFNode, with oldPtr->handle as C memory location. 
		fielsStr	: the field to send this to, eg: addChildren
	*/
	
	url.n = 0;
	url.p = NULL;
		
	/* parse the string, put it into the "url" struct defined here */
	Parser_scanStringValueToMem(X3D_NODE(&url),0,FIELDTYPE_MFString, _costr0, FALSE);

	/* find a file name that exists. If not, return JS_FALSE */
	res = resource_create_multi(&url);
	res->whereToPlaceData = myptr;


	/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
	fieldInt = findRoutedFieldInFIELDNAMES (myptr, fieldStr, TRUE);

	if (fieldInt >=0) { 
		findFieldInOFFSETS(myptr->_nodeType, fieldInt, &offs, &type, &accessType);
	} else {
		ConsoleMessage ("Can not find field :%s: in nodeType :%s:",fieldStr,stringNodeType(myptr->_nodeType));
#if JS_VERSION >= 185
		JS_free(context,_costr0);
		JS_free(context,fieldStr);
#endif
		return JS_FALSE;
	}

	/* printf ("type of field %s, accessType %s\n",stringFieldtypeType(type),stringKeywordType(accessType)); */
	res->offsetFromWhereToPlaceData = offs;
	parser_process_res_VRML_X3D(res);
	//send_resource_to_parser(res);
	//resource_wait(res);
	//
	//if (res->status == ress_parsed) {
	//	/* Cool :) */
	//}

	MARK_EVENT(myptr,offs);
#if JS_VERSION >= 185
	JS_SET_RVAL(context,vp,*rval);
	JS_free(context,fieldStr);
	JS_free(context,_costr0);
#endif
	return JS_TRUE;
}

JSBool
#if JS_VERSION < 185
VrmlBrowserAddRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	jsval _rval = INT_TO_JSVAL(0);
#else
VrmlBrowserAddRoute(JSContext *context, uintN argc, jsval *vp) {
        JSObject *obj = JS_THIS_OBJECT(context,vp);
        jsval *argv = JS_ARGV(context,vp);
#endif
	if (!doVRMLRoute(context, obj, argc, argv, "addRoute")) {
		printf( "doVRMLRoute failed in VrmlBrowserAddRoute.\n");
		return JS_FALSE;
	}
#if JS_VERSION < 185
	*rval = _rval;
#else
	JS_SET_RVAL(context,vp,JSVAL_ZERO);
#endif
	return JS_TRUE;
}

JSBool
#if JS_VERSION < 185
VrmlBrowserPrint(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	jsval _rval = INT_TO_JSVAL(0);
#else
VrmlBrowserPrint(JSContext *context, uintN argc, jsval *vp) {
        JSObject *obj = JS_THIS_OBJECT(context,vp);
        jsval *argv = JS_ARGV(context,vp);
#endif
	int count;
	JSString *_str;
	char *_id_c;

	UNUSED (context); UNUSED(obj);
	/* printf ("FreeWRL:javascript: "); */
	for (count=0; count < argc; count++) {
		if (JSVAL_IS_STRING(argv[count])) {
			_str = JSVAL_TO_STRING(argv[count]);
#if JS_VERSION < 185
			_id_c = JS_GetStringBytes(_str);
#else
			_id_c = JS_EncodeString(context,_str);
#endif
			#if defined(AQUA) || defined(_MSC_VER)
			ConsoleMessage(_id_c); /* statusbar hud */
			gglobal()->ConsoleMessage.consMsgCount = 0; /* reset the "Maximum" count */
			#else
				#ifdef HAVE_NOTOOLKIT 
					printf ("%s", _id_c);
				#else
					printf ("%s\n", _id_c);
					ConsoleMessage(_id_c); /* statusbar hud */
					gglobal()->ConsoleMessage.consMsgCount = 0; /* reset the "Maximum" count */
				#endif
			#endif
#if JS_VERSION >= 185
			JS_free(context,_id_c);
#endif
		} else {
	/*		printf ("unknown arg type %d\n",count); */
		}
	}
	/* the \n should be done with println below, or in javascript print("\n"); 
	  except web3d V3 specs don't have Browser.println so print will do \n like the old days*/
	#if defined(AQUA)  || defined(_MSC_VER)
	ConsoleMessage("\n"); /* statusbar hud */
	gglobal()->ConsoleMessage.consMsgCount = 0; /* reset the "Maximum" count */
	#elif !defined(_MSC_VER)
		#ifdef HAVE_NOTOOLKIT
			printf ("\n");
		#endif
	#endif
#if JS_VERSION < 185
	*rval = _rval;
#else
	JS_SET_RVAL(context,vp,JSVAL_ZERO);
#endif
	return JS_TRUE;
}

JSBool
#if JS_VERSION < 185
VrmlBrowserPrintln(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {	
    VrmlBrowserPrint(context,obj,argc,argv,rval);
#else
VrmlBrowserPrintln(JSContext *context, uintN argc, jsval *vp) {
	/* note, vp holds rval, since it is set in here we should be good */
	VrmlBrowserPrint(context,argc,vp); 
#endif
	#if defined(AQUA) || defined(_MSC_VER)
		//ConsoleMessage("\n"); /* statusbar hud */
		gglobal()->ConsoleMessage.consMsgCount = 0; /* reset the "Maximum" count */
	#else
		#ifdef HAVE_NOTOOLKIT
			printf ("\n");
		#endif
	#endif
	return JS_TRUE;
}

JSBool
#if JS_VERSION < 185
VrmlBrowserDeleteRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	jsval _rval = INT_TO_JSVAL(0);
#else
VrmlBrowserDeleteRoute(JSContext *context, uintN argc, jsval *vp) {
        JSObject *obj = JS_THIS_OBJECT(context,vp);
        jsval *argv = JS_ARGV(context,vp);
#endif
	if (!doVRMLRoute(context, obj, argc, argv, "deleteRoute")) {
		printf( "doVRMLRoute failed in VrmlBrowserDeleteRoute.\n");
		return JS_FALSE;
	}
#if JS_VERSION < 185
	*rval = _rval;
#else
	JS_SET_RVAL(context,vp,JSVAL_ZERO);
#endif
	return JS_TRUE;
}

/****************************************************************************************/


/****************************************************************************************************/

/* internal to add/remove a ROUTE */
static JSBool doVRMLRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, const char *callingFunc) {
	JSObject *fromNodeObj, *toNodeObj;
	SFNodeNative *fromNative, *toNative;
	JSClass *_cls[2];
	char 
		*fromFieldString, *toFieldString,
		*_c_args =
		"SFNode fromNode, SFString fromEventOut, SFNode toNode, SFString toEventIn",
#if JS_VERSION < 185
		*_c_format = "o s o s";
#else
		*_c_format = "oSoS";
	JSString *fromFieldStringJS, *toFieldStringJS;
#endif
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	int fromOfs, toOfs, len;
	int fromtype, totype;
	int xxx;
	int myField;

	/* first, are there 4 arguments? */
	if (argc != 4) {
		printf ("Problem with script - add/delete route command needs 4 parameters\n");
		return JS_FALSE;
	}

	/* get the arguments, and ensure that they are obj, string, obj, string */
	if (JS_ConvertArguments(context, argc, argv, _c_format,
#if JS_VERSION < 185
				&fromNodeObj, &fromFieldString, &toNodeObj, &toFieldString)) {
#else
				&fromNodeObj, &fromFieldStringJS, &toNodeObj, &toFieldStringJS)) {
		fromFieldString = JS_EncodeString(context,fromFieldStringJS);
		toFieldString = JS_EncodeString(context,toFieldStringJS);
#endif
		if ((_cls[0] = JS_GET_CLASS(context, fromNodeObj)) == NULL) {
			printf("JS_GetClass failed for arg 0 in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GET_CLASS(context, toNodeObj)) == NULL) {
			printf("JS_GetClass failed for arg 2 in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}

		/* make sure these are both SFNodes */
		if (memcmp("SFNode", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("SFNode", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			printf("\nArguments 0 and 2 must be SFNode in doVRMLRoute called from %s(%s): %s\n",
					callingFunc, _c_args, callingFunc);
			return JS_FALSE;
		}

		/* get the "private" data for these nodes. It will consist of a SFNodeNative structure */
		if ((fromNative = (SFNodeNative *)JS_GetPrivate(context, fromNodeObj)) == NULL) {
			printf ("problem getting native props\n");
			return JS_FALSE;
		}
		if ((toNative = (SFNodeNative *)JS_GetPrivate(context, toNodeObj)) == NULL) {
			printf ("problem getting native props\n");
			return JS_FALSE;
		}
		/* get the "handle" for the actual memory pointer */
		fromNode = X3D_NODE(fromNative->handle);
		toNode = X3D_NODE(toNative->handle);

		#ifdef JSVERBOSE
		printf ("routing from a node of type %s to a node of type %s\n",
			stringNodeType(fromNode->_nodeType), 
			stringNodeType(toNode->_nodeType));
		#endif	

		/* From field */
		/* try finding it, maybe with a "set_" or "changed" removed */
		myField = findRoutedFieldInFIELDNAMES(fromNode,fromFieldString,0);
		if (myField == -1) 
			myField = findRoutedFieldInFIELDNAMES(fromNode,fromFieldString,1);

		/* find offsets, etc */
       		findFieldInOFFSETS(fromNode->_nodeType, myField, &fromOfs, &fromtype, &xxx);

		/* To field */
		/* try finding it, maybe with a "set_" or "changed" removed */
		myField = findRoutedFieldInFIELDNAMES(toNode,toFieldString,0);
		if (myField == -1) 
			myField = findRoutedFieldInFIELDNAMES(toNode,toFieldString,1);

		/* find offsets, etc */
       		findFieldInOFFSETS(toNode->_nodeType, myField, &toOfs, &totype, &xxx);

		/* do we have a mismatch here? */
		if (fromtype != totype) {
			printf ("Javascript routing problem - can not route from %s to %s\n",
				stringNodeType(fromNode->_nodeType), 
				stringNodeType(toNode->_nodeType));
			return JS_FALSE;
		}

		len = returnRoutingElementLength(totype);

		jsRegisterRoute(fromNode, fromOfs, toNode, toOfs, len,callingFunc);

#if JS_VERSION >= 185
		JS_free(context,fromFieldString);
		JS_free(context,toFieldString);
#endif
	} else {
		printf( "\nIncorrect argument format for %s(%s).\n",
				callingFunc, _c_args);
		return JS_FALSE;
	}

	return JS_TRUE;
}

//dug9 - first look at x3dbrowser and x3dscene/executionContext
#ifdef X3DBROWSER
/* The Browser's supportedComponents and supportedProfiles are statically defined 
   in 'bits and pieces' in generatedCode.c and capabilitiesHandler.c and Structs.h.
   The Scene/ExecutionContext Profile and Components should be created during parsing
   (as of Aug 3, 2013 the parser calls handleProfile() or handleComponent() which
    just complains with printfs if freewrl can't handle the scene, and doesn't save them)

	For the browser's supportedComponents and supportedProfiles, we'll have
	indexable arrays, and on getting an index, we'll construct a throwaway JS object.
*/




#endif
/*
ComonentInfo{
String name;
Numeric level;
String Title;
String providerUrl;
}

ComponentInfoArray{
numeric length;
ComponentInfo [integer index];
}


ProfileInfo{
String name;
Numeric level;
String Title;
String providerUrl;
ComonentInfoArray components;
}
ProfileInfoArray{
numeric length;
ProfileInfo [integer index];
}

X3DFieldDefinition{
//properties
String name;
numeric accessType;  //e.g.. inputOnly
numeric dataType; //e.g. SFBool
}

FieldDefinitionArray{
numeric length;
X3DFieldDefinition [integer index];
}


ProtoDeclaration{
//properties
String name;
FieldDefinitionArray fields;
Boolean isExternProto;
//functions
SFNode newInstance();
}

ExternProtoDeclaration : ProtoDeclaration {
//properties
MFString urls;
numeric loadState;
//functions
void loadNow();
}

ProtoDeclarationArray{
numeric length;
X3DProtoDeclaration [integer index];
}

ExternProtoDeclarationArray{
numeric length;
X3DExternProtoDeclaration [integer index];
}

Route{
}
RouteArray{
numeric length;
Route [integer index];
}


ExecutionContext{
//properties
String specificationVersion;
String encoding;
ProfileInfo profile;
ComponentInfoArray components;
String worldURL;
MFNode rootNodes; //R + writable except in protoInstance
ProtoDeclarationArray protos; //RW
ExternProtoDeclarationArray externprotos; //RW
RouteArray routes;
//functions
X3DRoute addRoute(SFNode fromNode, String fromReadableField, SFNode toNode, String toWriteableField);
void deleteRoute(X3DRoute route);
SFNode createNode(String x3dsyntax);
SFNode createProto(String x3dsyntax);
SFNode getImportedNode(String defname, String);
void updateImportedNode(String defname, String);
void removeImportedNode(String defname);
SFNode getNamedNode(String defname):
void updateNamedNode(String defname, SFNode);
void removeNamedNode(String defname);
}

Scene : ExecutionContext{
//properties
String specificationVersion;
//functions
void setMetaData(String name, String value);
String getMetaData(String name);
SFNode getExportedNode(String defname);
void updateExportedNode(String defname, SFNode node);
void removeExportedNode(String defname);
}

//just createX3DFromString, createX3DFromURL and replaceWorld differ in signature between VRML and X3D browser classes
X3DBrowser{
//properties
String name;
String version;
numeric currentSpeed;
numeric currentFrameRate;
String description; //R/W
CompnentInfoArray supportedComponents;
ProfileInfoArray supportedProfiles;
//functions
X3DScene currentScene;  //since X3DScene : X3DExecutionContext, use Scene w/flag
void replaceWorld(X3DScene);
X3DScene createX3DFromString(String x3dsyntax);
X3DScene createX3DFromURL(MFString url, String callbackFunctionName, Object cbContextObject);
void loadURL(MFString url, MFString parameter);
X3DScene importDocument(DOMNode domNodeObject);
void getRenderingProperty(String propertyName);
void print(Object or String);
void println(Object or String);
}
*/

#endif /* HAVE_JAVASCRIPT */
#endif /* !(defined(JAVASCRIPT_STUB) || defined(JAVASCRIPT_DUK) */
