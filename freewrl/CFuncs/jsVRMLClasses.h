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
 * Complex VRML nodes as Javascript classes.
 *
 */

#ifndef __jsVRMLClasses_h__
#define  __jsVRMLClasses_h__

#ifndef UNUSED
#define UNUSED(v) ((void) v)
#endif

#include <math.h>
#include "LinearAlgebra.h" /* FreeWRL math */
#include "quaternion.h" /* more math */

#ifndef __jsUtils_h__
#include "jsUtils.h"
#endif /* __jsUtils_h__ */

#ifndef __jsNative_h__
#include "jsNative.h"
#endif /* __jsNative_h__ */

#include "jsVRMLBrowser.h"

#define INIT_ARGC_NODE 1
#define INIT_ARGC 0

#define NULL_HANDLE "NULL_HANDLE"

#if 0
/* #define AVECLEN(x) (sqrt((x)[0]*(x)[0]+(x)[1]*(x)[1]+(x)[2]*(x)[2])) */
/* #define AVECPT(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2]) */
/* #define AVECSCALE(x,y) x[0] *= y;\ */
/* x[1] *= y;\ */
/* x[2] *= y; */
/* #define AVECCP(x,y,z) (z)[0]=(x)[1]*(y)[2]-(x)[2]*(y)[1]; \ */
/* (z)[1]=(x)[2]*(y)[0]-(x)[0]*(y)[2]; \ */
/* (z)[2]=(x)[0]*(y)[1]-(x)[1]*(y)[0]; */
#endif

/*
 * The following VRML field types don't need JS classes:
 * (ECMAScript native datatypes, see JS.pm):
 *
 * * SFBool
 * * SFFloat
 * * SFInt32
 * * SFString
 * * SFTime
 *
 * VRML field types that are implemented here as Javascript classes
 * are:
 *
 * * SFColor, MFColor
 * * MFFloat
 * * SFImage -- not supported currently
 * * MFInt32
 * * SFNode (special case - must be supported perl (see JS.pm), MFNode
 * * SFRotation, MFRotation
 * * MFString
 * * MFTime
 * * SFVec2f, MFVec2f
 * * SFVec3f, MFVec3f
 *
 * These (single value) fields have struct types defined elsewhere
 * (see Structs.h) that are stored by Javascript classes as private data.
 *
 * Some of the computations for SFVec3f, SFRotation are now defined
 * elsewhere (see LinearAlgebra.h) to avoid duplication.
 */



/* helper functions */

static JSBool
doMFToString(JSContext *cx,
				JSObject *obj,
				const char *className,
				jsval *rval);

static JSBool
doMFAddProperty(JSContext *cx,
				JSObject *obj,
				jsval id,
				jsval *vp, char *name);

static JSBool
doMFSetProperty(JSContext *cx,
				JSObject *obj,
				jsval id,
				jsval *vp, char *name);

static JSBool
getBrowser(JSContext *context,
		   JSObject *obj,
		   BrowserNative **brow);

static JSBool
doMFStringUnquote(JSContext *cx,
				  jsval *vp);


/* class functions */

JSBool
globalResolve(JSContext *cx,
			  JSObject *obj,
			  jsval id);

JSBool
loadVrmlClasses(JSContext *context,
				JSObject *globalObj);


JSBool
setECMANative(JSContext *cx,
			  JSObject *obj,
			  jsval id,
			  jsval *vp);


JSBool
getAssignProperty(JSContext *context,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);

JSBool
setAssignProperty(JSContext *context,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);



/* not implemented */
JSBool
SFColorGetHSV(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

/* not implemented */
JSBool
SFColorSetHSV(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFColorToString(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
SFColorAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFColorTouched(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool 
SFColorConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

void
SFColorFinalize(JSContext *cx,
				JSObject *obj);

JSBool
SFColorGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool
SFColorSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



JSBool
SFImageToString(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
SFImageAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFImageTouched(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool 
SFImageConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

void 
SFImageFinalize(JSContext *cx,
				JSObject *obj);

JSBool
SFImageGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool
SFImageSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



JSBool
SFNodeToString(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
SFNodeAssign(JSContext *cx, JSObject *obj,
			 uintN argc,
			 jsval *argv,
			 jsval *rval);

JSBool
SFNodeTouched(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFNodeConstr(JSContext *cx,
			 JSObject *obj,
			 uintN argc,
			 jsval *argv,
			 jsval *rval);

void
SFNodeFinalize(JSContext *cx,
			   JSObject *obj);

JSBool
SFNodeGetProperty(JSContext *cx,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);

JSBool
SFNodeSetProperty(JSContext *cx,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);



JSBool
SFRotationGetAxis(JSContext *cx,
				  JSObject *obj,
				  uintN argc,
				  jsval *argv,
				  jsval *rval);

/* not implemented */
JSBool
SFRotationInverse(JSContext *cx,
				  JSObject *obj,
				  uintN argc,
				  jsval *argv,
				  jsval *rval);

/* not implemented */
JSBool
SFRotationMultiply(JSContext *cx,
				   JSObject *obj,
				   uintN argc,
				   jsval *argv,
				   jsval *rval);
JSBool
SFRotationMultVec(JSContext *cx,
				  JSObject *obj,
				  uintN argc,
				  jsval *argv,
				  jsval *rval);

JSBool
SFRotationSetAxis(JSContext *cx,
				  JSObject *obj,
				  uintN argc,
				  jsval *argv,
				  jsval *rval);

JSBool
SFRotationSlerp(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
SFRotationToString(JSContext *cx,
				   JSObject *obj,
				   uintN argc,
				   jsval *argv,
				   jsval *rval);

JSBool
SFRotationAssign(JSContext *cx,
				 JSObject *obj,
				 uintN argc,
				 jsval *argv,
				 jsval *rval);

JSBool
SFRotationTouched(JSContext *cx,
				  JSObject *obj,
				  uintN argc,
				  jsval *argv,
				  jsval *rval);

JSBool 
SFRotationConstr(JSContext *cx,
				 JSObject *obj,
				 uintN argc,
				 jsval *argv,
				 jsval *rval);

void 
SFRotationFinalize(JSContext *cx,
				   JSObject *obj);

JSBool 
SFRotationGetProperty(JSContext *cx,
					  JSObject *obj,
					  jsval id,
					  jsval *vp);

JSBool 
SFRotationSetProperty(JSContext *cx,
					  JSObject *obj,
					  jsval id,
					  jsval *vp);



JSBool
SFVec2fAdd(JSContext *cx,
		   JSObject *obj,
		   uintN argc,
		   jsval *argv,
		   jsval *rval);

JSBool
SFVec2fDivide(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFVec2fDot(JSContext *cx,
		   JSObject *obj,
		   uintN argc,
		   jsval *argv,
		   jsval *rval);

JSBool
SFVec2fLength(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFVec2fMultiply(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

//JSBool
//SFVec2fNegate(JSContext *cx,
//			  JSObject *obj,
//			  uintN argc,
//			  jsval *argv,
//			  jsval *rval);

JSBool
SFVec2fNormalize(JSContext *cx,
				 JSObject *obj,
				 uintN argc,
				 jsval *argv,
				 jsval *rval);

JSBool
SFVec2fSubtract(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
SFVec2fToString(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
SFVec2fAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFVec2fTouched(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
SFVec2fConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

void
SFVec2fFinalize(JSContext *cx,
				JSObject *obj);

JSBool 
SFVec2fGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
SFVec2fSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



JSBool
SFVec3fAdd(JSContext *cx,
		   JSObject *obj,
		   uintN argc,
		   jsval *argv,
		   jsval *rval);

JSBool
SFVec3fCross(JSContext *cx,
			 JSObject *obj,
			 uintN argc,
			 jsval *argv,
			 jsval *rval);

JSBool
SFVec3fDivide(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFVec3fDot(JSContext *cx,
		   JSObject *obj,
		   uintN argc,
		   jsval *argv,
		   jsval *rval);

JSBool
SFVec3fLength(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFVec3fMultiply(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
SFVec3fNegate(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFVec3fNormalize(JSContext *cx,
				 JSObject *obj,
				 uintN argc,
				 jsval *argv,
				 jsval *rval);

JSBool
SFVec3fSubtract(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
SFVec3fToString(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
SFVec3fAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
SFVec3fTouched(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
SFVec3fConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

void
SFVec3fFinalize(JSContext *cx,
				JSObject *obj);

JSBool 
SFVec3fGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
SFVec3fSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



JSBool
MFColorToString(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
MFColorAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFColorConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFColorAddProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFColorGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFColorSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



JSBool
MFFloatToString(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
MFFloatAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFFloatConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFFloatAddProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFFloatGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFFloatSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



JSBool
MFInt32ToString(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
MFInt32Assign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFInt32Constr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFInt32AddProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFInt32GetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFInt32SetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



JSBool
MFNodeToString(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
MFNodeAssign(JSContext *cx,
			 JSObject *obj,
			 uintN argc,
			 jsval *argv,
			 jsval *rval);

JSBool
MFNodeConstr(JSContext *cx,
			 JSObject *obj,
			 uintN argc,
			 jsval *argv,
			 jsval *rval);

JSBool
MFNodeAddProperty(JSContext *cx,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);

JSBool 
MFNodeGetProperty(JSContext *cx,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);

JSBool 
MFNodeSetProperty(JSContext *cx,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);



JSBool
MFRotationToString(JSContext *cx,
				   JSObject *obj,
				   uintN argc,
				   jsval *argv,
				   jsval *rval);

JSBool
MFRotationAssign(JSContext *cx,
				 JSObject *obj,
				 uintN argc,
				 jsval *argv,
				 jsval *rval);

JSBool
MFRotationConstr(JSContext *cx,
				 JSObject *obj,
				 uintN argc,
				 jsval *argv,
				 jsval *rval);

JSBool 
MFRotationGetProperty(JSContext *cx,
					  JSObject *obj,
					  jsval id,
					  jsval *vp);

JSBool 
MFRotationSetProperty(JSContext *cx,
					  JSObject *obj,
					  jsval id,
					  jsval *vp);

JSBool
MFRotationAddProperty(JSContext *cx,
					  JSObject *obj,
					  jsval id,
					  jsval *vp);



JSBool
MFStringToString(JSContext *cx,
				 JSObject *obj,
				 uintN argc,
				 jsval *argv,
				 jsval *rval);

JSBool
MFStringAssign(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
MFStringConstr(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool 
MFStringGetProperty(JSContext *cx,
					JSObject *obj,
					jsval id,
					jsval *vp);

JSBool 
MFStringSetProperty(JSContext *cx,
					JSObject *obj,
					jsval id,
					jsval *vp);


JSBool
MFStringAddProperty(JSContext *cx,
					JSObject *obj,
					jsval id,
					jsval *vp);



JSBool
MFTimeToString(JSContext *cx,
			   JSObject *obj,
			   uintN argc,
			   jsval *argv,
			   jsval *rval);

JSBool
MFTimeAssign(JSContext *cx,
			 JSObject *obj,
			 uintN argc,
			 jsval *argv,
			 jsval *rval);

JSBool
MFTimeConstr(JSContext *cx,
			 JSObject *obj,
			 uintN argc,
			 jsval *argv,
			 jsval *rval);

JSBool
MFTimeAddProperty(JSContext *cx,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);

JSBool 
MFTimeGetProperty(JSContext *cx,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);

JSBool 
MFTimeSetProperty(JSContext *cx,
				  JSObject *obj,
				  jsval id,
				  jsval *vp);



JSBool
MFVec2fToString(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
MFVec2fAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFVec2fConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFVec2fAddProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFVec2fGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFVec2fSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



JSBool
MFVec3fToString(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
MFVec3fAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFVec3fConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
MFVec3fAddProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFVec3fGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
MFVec3fSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool
VrmlMatrixToString(JSContext *cx,
				JSObject *obj,
				uintN argc,
				jsval *argv,
				jsval *rval);

JSBool
VrmlMatrixAssign(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);


JSBool VrmlMatrixsetTransform(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool VrmlMatrixgetTransform(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool VrmlMatrixinverse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool VrmlMatrixtranspose(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool VrmlMatrixmultLeft(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool VrmlMatrixmultRight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool VrmlMatrixmultVecMatrix(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool VrmlMatrixmultMatrixVec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool
VrmlMatrixConstr(JSContext *cx,
			  JSObject *obj,
			  uintN argc,
			  jsval *argv,
			  jsval *rval);

JSBool
VrmlMatrixAddProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
VrmlMatrixGetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);

JSBool 
VrmlMatrixSetProperty(JSContext *cx,
				   JSObject *obj,
				   jsval id,
				   jsval *vp);



/*
 * VRML Node types as JS classes:
 */


static JSObject *proto_SFColor;

static JSClass SFColorClass = {
	"SFColor",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFColorGetProperty,
	SFColorSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	SFColorFinalize
};

static JSPropertySpec (SFColorProperties)[] = {
	{"r", 0, JSPROP_ENUMERATE},
	{"g", 1, JSPROP_ENUMERATE},
	{"b", 2, JSPROP_ENUMERATE},
	{0}
};

static JSFunctionSpec (SFColorFunctions)[] = {
	{"getHSV", SFColorGetHSV, 0},
	{"setHSV", SFColorSetHSV, 0},
	{"toString", SFColorToString, 0},
	{"assign", SFColorAssign, 0},
	{"__touched", SFColorTouched, 0},
	{0}
};


static JSObject *proto_SFImage;

static JSClass SFImageClass = {
	"SFImage",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFImageGetProperty,
	SFImageSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	SFImageFinalize
};

static JSPropertySpec (SFImageProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{"comp", 2, JSPROP_ENUMERATE},
	{"array", 3, JSPROP_ENUMERATE},
	{0}
};

static JSFunctionSpec (SFImageFunctions)[] = {
	{"toString", SFImageToString, 0},
	{"assign", SFImageAssign, 0},
	{"__touched", SFImageTouched, 0},
	{0}
};


static JSObject *proto_SFNode;

static JSClass SFNodeClass = {
	"SFNode",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFNodeGetProperty,
	SFNodeSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	SFNodeFinalize
};

static JSPropertySpec (SFNodeProperties)[] = {
	{"__handle", 1, JSPROP_ENUMERATE},
	{"__vrmlstring", 0, JSPROP_ENUMERATE},
	{0}
};

static JSFunctionSpec (SFNodeFunctions)[] = {
	{"toString", SFNodeToString, 0},
	{"assign", SFNodeAssign, 0},
	{"__touched", SFNodeTouched, 0},
	{0}
};


static JSObject *proto_SFRotation;

static JSClass SFRotationClass = {
	"SFRotation",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFRotationGetProperty,
	SFRotationSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	SFRotationFinalize
};

static JSPropertySpec (SFRotationProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{"z", 2, JSPROP_ENUMERATE},
	{"angle",3, JSPROP_ENUMERATE},
	{0}
};

static JSFunctionSpec (SFRotationFunctions)[] = {
	{"getAxis", SFRotationGetAxis, 0},
	{"inverse", SFRotationInverse, 0},
	{"multiply", SFRotationMultiply, 0},
	{"multVec", SFRotationMultVec, 0},
	{"setAxis", SFRotationSetAxis, 0},
	{"slerp", SFRotationSlerp, 0},
	{"toString", SFRotationToString, 0},
	{"assign", SFRotationAssign, 0},
	{"__touched", SFRotationTouched, 0},
	{0}
};


static JSObject *proto_SFVec2f;

static JSClass SFVec2fClass = {
	"SFVec2f",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFVec2fGetProperty,
	SFVec2fSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	SFVec2fFinalize
};

static JSPropertySpec (SFVec2fProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{0}
};

static JSFunctionSpec (SFVec2fFunctions)[] = {
	{"add", SFVec2fAdd, 0},
	{"divide", SFVec2fDivide, 0},
	{"dot", SFVec2fDot, 0},
	{"length", SFVec2fLength, 0},
	{"multiply", SFVec2fMultiply, 0},
	//{"negate", SFVec2fNegate, 0},
	{"normalize", SFVec2fNormalize, 0},
	{"subtract", SFVec2fSubtract, 0},
	{"toString", SFVec2fToString, 0},
	{"assign", SFVec2fAssign, 0},
	{"__touched", SFVec2fTouched, 0},
	{0}
};


static JSObject *proto_SFVec3f;

static JSClass SFVec3fClass = {
	"SFVec3f",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFVec3fGetProperty,
	SFVec3fSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	SFVec3fFinalize
};

static JSPropertySpec (SFVec3fProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{"z", 2, JSPROP_ENUMERATE},
	{0}
};

static JSFunctionSpec (SFVec3fFunctions)[] = {
	{"add", SFVec3fAdd, 0},
	{"cross", SFVec3fCross, 0},
	{"divide", SFVec3fDivide, 0},
	{"dot", SFVec3fDot, 0},
	{"length", SFVec3fLength, 0},
	{"multiply", SFVec3fMultiply, 0},
	{"negate", SFVec3fNegate, 0},
	{"normalize", SFVec3fNormalize, 0},
	{"subtract", SFVec3fSubtract, 0},
	{"toString", SFVec3fToString, 0},
	{"assign", SFVec3fAssign, 0},
	{"__touched", SFVec3fTouched, 0},
	{0}
};


static JSObject *proto_MFColor;

static JSClass MFColorClass = {
	"MFColor",
	JSCLASS_HAS_PRIVATE,
	MFColorAddProperty,
	JS_PropertyStub,
	MFColorGetProperty,
	MFColorSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (MFColorFunctions)[] = {
	{"toString", MFColorToString, 0},
	{"assign", MFColorAssign, 0},
	{0}
};


static JSObject *proto_MFFloat;

static JSClass MFFloatClass = {
	"MFFloat",
	JSCLASS_HAS_PRIVATE,
	MFFloatAddProperty,
	JS_PropertyStub,
	MFFloatGetProperty,
	MFFloatSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (MFFloatFunctions)[] = {
	{"toString", MFFloatToString, 0},
	{"assign", MFFloatAssign, 0},
	{0}
};


static JSObject *proto_MFInt32;

static JSClass MFInt32Class = {
	"MFInt32",
	JSCLASS_HAS_PRIVATE,
	MFInt32AddProperty,
	JS_PropertyStub,
	MFInt32GetProperty,
	MFInt32SetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (MFInt32Functions)[] = {
	{"toString", MFInt32ToString, 0},
	{"assign", MFInt32Assign, 0},
	{0}
};


static JSObject *proto_MFNode;

static JSClass MFNodeClass = {
	"MFNode",
	JSCLASS_HAS_PRIVATE,
	MFNodeAddProperty,
	JS_PropertyStub,
	MFNodeGetProperty,
	MFNodeSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (MFNodeFunctions)[] = {
	{"toString", MFNodeToString, 0},
	{"assign", MFNodeAssign, 0},
	{0}
};


static JSObject *proto_MFRotation;

static JSClass MFRotationClass = {
	"MFRotation",
	JSCLASS_HAS_PRIVATE,
	MFRotationAddProperty,
	JS_PropertyStub,
	MFRotationGetProperty,
	MFRotationSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (MFRotationFunctions)[] = {
	{"toString", MFRotationToString, 0},
	{"assign", MFRotationAssign, 0},
	{0}
};


static JSObject *proto_MFString;

static JSClass MFStringClass = {
	"MFString",
	JSCLASS_HAS_PRIVATE,
	MFStringAddProperty,
	JS_PropertyStub,
	MFStringGetProperty,
	MFStringSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (MFStringFunctions)[] = {
	{"toString", MFStringToString, 0},
	{"assign", MFStringAssign, 0},
	{0}
};


static JSObject *proto_MFTime;

static JSClass MFTimeClass = {
	"MFTime",
	JSCLASS_HAS_PRIVATE,
	MFTimeAddProperty,
	JS_PropertyStub,
	MFTimeGetProperty,
	MFTimeSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

/* static JSPropertySpec (MFTimeProperties)[] = { */
/* 	{"length", 0, JSPROP_ENUMERATE|JSPROP_PERMANENT}, */
/* 	{0} */
/* }; */

static JSFunctionSpec (MFTimeFunctions)[] = {
	{"toString", MFTimeToString, 0},
	{"assign", MFTimeAssign, 0},
	{0}
};


static JSObject *proto_MFVec2f;

static JSClass MFVec2fClass = {
	"MFVec2f",
	JSCLASS_HAS_PRIVATE,
	MFVec2fAddProperty,
	JS_PropertyStub,
	MFVec2fGetProperty,
	MFVec2fSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (MFVec2fFunctions)[] = {
	{"toString", MFVec2fToString, 0},
	{"assign", MFVec2fAssign, 0},
	{0}
};


static JSObject *proto_MFVec3f;

static JSClass MFVec3fClass = {
	"MFVec3f",
	JSCLASS_HAS_PRIVATE,
	MFVec3fAddProperty,
	JS_PropertyStub,
	MFVec3fGetProperty,
	MFVec3fSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (MFVec3fFunctions)[] = {
	{"toString", MFVec3fToString, 0},
	{"assign", MFVec3fAssign, 0},
	{0}
};

/* VrmlMatrix - JAS */
static JSObject *proto_VrmlMatrix;

static JSClass VrmlMatrixClass = {
	"VrmlMatrix",
	JSCLASS_HAS_PRIVATE,
	VrmlMatrixAddProperty,
	JS_PropertyStub,
	VrmlMatrixGetProperty,
	VrmlMatrixSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSFunctionSpec (VrmlMatrixFunctions)[] = {
	{"toString", VrmlMatrixToString, 0},
	{"assign", VrmlMatrixAssign, 0},
	{"getTransform", VrmlMatrixgetTransform, 0},
	{"setTransform", VrmlMatrixsetTransform, 0},
	{"inverse", VrmlMatrixinverse, 0},
	{"transpose", VrmlMatrixtranspose, 0},
	{"multLeft", VrmlMatrixmultLeft, 0},
	{"multRight", VrmlMatrixmultRight, 0},
	{"multVecMatrix", VrmlMatrixmultVecMatrix, 0},
	{"multMatrixVec", VrmlMatrixmultMatrixVec, 0},
	{0}
};

#endif /*  __jsVRMLClasses_h__ */
