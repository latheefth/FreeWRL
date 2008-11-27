/*
=INSERT_TEMPLATE_HERE=

$Id$

*/

#ifndef __FREEX3D_JS_VRML_BROWSER_H__
#define __FREEX3D_JS_VRML_BROWSER_H__


#ifndef UNUSED
#define UNUSED(v) ((void) v)
#endif

extern char *BrowserName; /* defined in VRMLC.pm */
extern double BrowserFPS;				/* defined in VRMLC.pm */

#define BROWMAGIC 12345

JSBool VrmlBrowserInit(JSContext *context, JSObject *globalObj,	BrowserNative *brow );


JSBool
VrmlBrowserGetName(JSContext *cx,
				   JSObject *obj,
				   uintN argc,
				   jsval *argv,
				   jsval *rval);


JSBool
VrmlBrowserGetVersion(JSContext *cx,
					  JSObject *obj,
					  uintN argc,
					  jsval *argv,
					  jsval *rval);


JSBool
VrmlBrowserGetCurrentSpeed(JSContext *cx,
					   JSObject *obj,
					   uintN argc,
					   jsval *argv,
					   jsval *rval);


JSBool
VrmlBrowserGetCurrentFrameRate(JSContext *cx,
						   JSObject *obj,
						   uintN argc,
						   jsval *argv,
						   jsval *rval);


JSBool
VrmlBrowserGetWorldURL(JSContext *cx,
					   JSObject *obj,
					   uintN argc,
					   jsval *argv,
					   jsval *rval);


JSBool
VrmlBrowserReplaceWorld(JSContext *cx,
					JSObject *obj,
					uintN argc,
					jsval *argv,
					jsval *rval);


JSBool
VrmlBrowserLoadURL(JSContext *cx,
				   JSObject *obj,
				   uintN argc,
				   jsval *argv,
				   jsval *rval);


JSBool
VrmlBrowserSetDescription(JSContext *cx,
						  JSObject *obj,
						  uintN argc,
						  jsval *argv,
						  jsval *rval);


JSBool
VrmlBrowserCreateVrmlFromString(JSContext *cx,
						  JSObject *obj,
								uintN argc,
								jsval *argv,
								jsval *rval);


JSBool
VrmlBrowserCreateVrmlFromURL(JSContext *cx,
							 JSObject *obj,
							 uintN argc,
							 jsval *argv,
							 jsval *rval);


JSBool
VrmlBrowserAddRoute(JSContext *cx,
					JSObject *obj,
					uintN argc,
					jsval *argv,
					jsval *rval);


JSBool
VrmlBrowserPrint(JSContext *cx,
					   JSObject *obj,
					   uintN argc,
					   jsval *argv,
					   jsval *rval);

JSBool
VrmlBrowserDeleteRoute(JSContext *cx,
					   JSObject *obj,
					   uintN argc,
					   jsval *argv,
					   jsval *rval);

JSBool
VrmlBrowserGetMidiDeviceList(JSContext *cx,
					   JSObject *obj,
					   uintN argc,
					   jsval *argv,
					   jsval *rval);

JSBool
VrmlBrowserGetMidiDeviceInfo(JSContext *cx,
					   JSObject *obj,
					   uintN argc,
					   jsval *argv,
					   jsval *rval);



static JSClass Browser = {
	"Browser",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};



#endif /* __FREEX3D_JS_VRML_BROWSER_H__ */