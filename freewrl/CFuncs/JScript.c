/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/********************************************************************

Javascript C language binding.

*********************************************************************/


#include <math.h>
#include "headers.h"

#include "jsapi.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"

#include "CParseGeneral.h"

/* MAX_RUNTIME_BYTES controls when garbage collection takes place. */
#define MAX_RUNTIME_BYTES 0x100000L
/*   #define MAX_RUNTIME_BYTES 0x1000000L */
/* #define STACK_CHUNK_SIZE 0x2000L*/
#define STACK_CHUNK_SIZE 0x20000L

/*
 * Global JS variables (from Brendan Eichs short embedding tutorial):
 *
 * JSRuntime       - 1 runtime per process
 * JSContext       - 1 CONTEXT per thread
 * global JSObject - 1 global object per CONTEXT
 *
 * struct JSClass {
 *     char *name;
 *     uint32 flags;
 * Mandatory non-null function pointer members:
 *     JSPropertyOp addProperty;
 *     JSPropertyOp delProperty;
 *     JSPropertyOp getProperty;
 *     JSPropertyOp setProperty;
 *     JSEnumerateOp enumerate;
 *     JSResolveOp resolve;
 *     JSConvertOp convert;
 *     JSFinalizeOp finalize;
 * Optionally non-null members start here:
 *     JSGetObjectOps getObjectOps;
 *     JSCheckAccessOp checkAccess;
 *     JSNative call;
 *     JSNative construct;
 *     JSXDRObjectOp xdrObject;
 *     JSHasInstanceOp hasInstance;
 *     prword spare[2];
 * };
 *
 * global JSClass  - populated by stubs
 *
 */


char *DefaultScriptMethods = "function initialize() {}; " \
			" function shutdown() {}; " \
			" function eventsProcessed() {}; " \
			" TRUE=true; FALSE=false; " \
			" function print(x) {Browser.print(x)}; " \	
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
			" function addRoute(a,b,c,d) {Browser.addRoute(a,b,c,d)}; "\
			" function deleteRoute(a,b,c,d) {Browser.deleteRoute(a,b,c,d)}; "\
			" function getMidiDeviceList() {return Browser.getMidiDeviceList())}; "\
			" function getMidiDeviceInfo(x) {return Browser.getMidiDeviceInfo(x))}; "\
			"";

static JSRuntime *runtime = NULL;
static JSClass globalClass = {
	"global",
	0,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_EnumerateStub,
	globalResolve,
	JS_ConvertStub,
	JS_FinalizeStub
};


int JSMaxScript = 0;


/* housekeeping routines */
void kill_javascript(void) {
/*printf ("calling kill_javascript()\n");*/
return;
	JS_DestroyRuntime(runtime);
	runtime = NULL;
	JSMaxScript = 0;
	FREE_IF_NZ (ScriptControl)
}


void cleanupDie(uintptr_t num, const char *msg) {
	kill_javascript();
	freewrlDie(msg);
}

void JSMaxAlloc() {
	/* perform some REALLOCs on JavaScript database stuff for interfacing */
	uintptr_t count;

	JSMaxScript += 10;
	ScriptControl = (struct CRscriptStruct*)REALLOC (ScriptControl, sizeof (*ScriptControl) * JSMaxScript);
	scr_act = (uintptr_t *)REALLOC (scr_act, sizeof (*scr_act) * JSMaxScript);

	/* mark these scripts inactive */
	for (count=JSMaxScript-10; count<JSMaxScript; count++) {
		scr_act[count]= FALSE;
		ScriptControl[count].thisScriptType = NOSCRIPT;
	}
}



void JSInit(uintptr_t num) {
	jsval rval;
	JSContext *_context; 	/* these are set here */
	JSObject *_globalObj; 	/* these are set here */
	BrowserNative *br; 	/* these are set here */
	#ifdef JAVASCRIPTVERBOSE 
	printf("init: script %d\n",num);
	#endif

	/* more scripts than we can handle right now? */
	if (num >= JSMaxScript)  {
		JSMaxAlloc();
	}


	/* is this the first time through? */
	if (runtime == NULL) {
		runtime = JS_NewRuntime(MAX_RUNTIME_BYTES);
		if (!runtime) freewrlDie("JS_NewRuntime failed");

		#ifdef JAVASCRIPTVERBOSE 
		printf("\tJS runtime created,\n");
		#endif
	}


	_context = JS_NewContext(runtime, STACK_CHUNK_SIZE);
	if (!_context) freewrlDie("JS_NewContext failed");

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tJS context created,\n");
	#endif


	_globalObj = JS_NewObject(_context, &globalClass, NULL, NULL);
	if (!_globalObj) freewrlDie("JS_NewObject failed");

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tJS global object created,\n");
	#endif


	/* gets JS standard classes */
	if (!JS_InitStandardClasses(_context, _globalObj))
		freewrlDie("JS_InitStandardClasses failed");

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tJS standard classes initialized,\n");
	#endif



	#ifdef JAVASCRIPTVERBOSE 
	 	reportWarningsOn();
	#endif

	JS_SetErrorReporter(_context, errorReporter);
	#ifdef JAVASCRIPTVERBOSE 
	printf("\tJS errror reporter set,\n");
	#endif


	br = (BrowserNative *) JS_malloc(_context, sizeof(BrowserNative));

	/* for this script, here are the necessary data areas */
	ScriptControl[num].cx = (uintptr_t) _context;
	ScriptControl[num].glob = (uintptr_t) _globalObj;


	if (!loadVrmlClasses(_context, _globalObj))
		freewrlDie("loadVrmlClasses failed");

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tVRML classes loaded,\n");
	#endif



	if (!VrmlBrowserInit(_context, _globalObj, br))
		freewrlDie("VrmlBrowserInit failed");

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tVRML Browser interface loaded,\n");
	#endif

	if (!ActualrunScript(num,DefaultScriptMethods,&rval))
		cleanupDie(num,"runScript failed in VRML::newJS DefaultScriptMethods");

	/* send this data over to the routing table functions. */
	CRoutes_js_new (num, JAVASCRIPT);

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tVRML browser initialized\n");
	#endif
}

/* run the script from within C */
int ActualrunScript(uintptr_t num, char *script, jsval *rval) {
	size_t len;
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[num].cx;
	_globalObj = (JSObject *)ScriptControl[num].glob;

	#ifdef JAVASCRIPTVERBOSE
		printf("ActualrunScript script %d cx %x \"%s\", \n",
			   num, _context, script);
	#endif
	len = strlen(script);
	if (!JS_EvaluateScript(_context, _globalObj, script, len,
						   FNAME_STUB, LINENO_STUB, rval)) {
		printf("JS_EvaluateScript failed for \"%s\".\n", script);
		return JS_FALSE;
	 }

	#ifdef JAVASCRIPTVERBOSE 
	printf ("runscript passed\n");
	#endif

	return JS_TRUE;
}



/* run the script from within Javascript  */
int jsrrunScript(JSContext *_context, JSObject *_globalObj, char *script, jsval *rval) {

	size_t len;

	#ifdef JAVASCRIPTVERBOSE
		printf("jsrrunScript script cx %x \"%s\", \n",
			   _context, script);
	#endif

	len = strlen(script);
	if (!JS_EvaluateScript(_context, _globalObj, script, len,
						   FNAME_STUB, LINENO_STUB, rval)) {
		printf("JS_EvaluateScript failed for \"%s\".\n", script);
		return JS_FALSE;
	 }

	#ifdef JAVASCRIPTVERBOSE 
	printf ("runscript passed\n");
	#endif

	return JS_TRUE;
}


/* FROM VRMLC.pm */

void *
SFNodeNativeNew()
{
	SFNodeNative *ptr;
	ptr = (SFNodeNative *) MALLOC(sizeof(*ptr));

	/* printf ("SFNodeNativeNew; string len %d handle_len %d\n",vrmlstring_len,handle_len);*/

	ptr->handle = 0;
	ptr->touched = 0;
	ptr->X3DString = NULL;
	return ptr;
}

void
SFNodeNativeDelete(void *p)
{
	SFNodeNative *ptr;
	if (p != NULL) {
		ptr = (SFNodeNative *)p;
		FREE_IF_NZ (ptr->X3DString);
		FREE_IF_NZ (ptr);
	}
}

/* assign this internally to the Javascript engine environment */
int
SFNodeNativeAssign(void *top, void *fromp)
{
	SFNodeNative *to = (SFNodeNative *)top;
	SFNodeNative *from = (SFNodeNative *)fromp;

	/* indicate that this was touched; and copy contents over */
	to->touched++;

	if (from != NULL) {
		to->handle = from->handle;
		to->X3DString = strdup(from->X3DString);

		#ifdef JAVASCRIPTVERBOSE
		printf ("SFNodeNativeAssign, copied %d to %d, handle %d, string %s\n", from, to, to->handle, to->X3DString);
		#endif
	} else {
		to->handle = 0;
		to->X3DString = strdup("from a NULL assignment");
	}

	return JS_TRUE;
}


void *
SFColorRGBANativeNew()
{
	SFColorRGBANative *ptr;
	ptr = (SFColorRGBANative *)MALLOC(sizeof(*ptr));
	ptr->touched = 0;
	return ptr;
}

void
SFColorRGBANativeDelete(void *p)
{
	SFColorRGBANative *ptr;
	if (p != NULL) {
		ptr = (SFColorRGBANative *)p;
		FREE_IF_NZ (ptr);
	}
}

void
SFColorRGBANativeAssign(void *top, void *fromp)
{
	SFColorRGBANative *to = (SFColorRGBANative *)top;
	SFColorRGBANative *from = (SFColorRGBANative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFColorNativeNew()
{
	SFColorNative *ptr;
	ptr = (SFColorNative *)MALLOC(sizeof(*ptr));
	ptr->touched = 0;
	return ptr;
}

void
SFColorNativeDelete(void *p)
{
	SFColorNative *ptr;
	if (p != NULL) {
		ptr = (SFColorNative *)p;
		FREE_IF_NZ (ptr);
	}
}

void
SFColorNativeAssign(void *top, void *fromp)
{
	SFColorNative *to = (SFColorNative *)top;
	SFColorNative *from = (SFColorNative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFImageNativeNew()
{
	SFImageNative *ptr;
	ptr =(SFImageNative *) MALLOC(sizeof(*ptr));
	ptr->touched = 0;
	return ptr;
}

void
SFImageNativeDelete(void *p)
{
	SFImageNative *ptr;
	if (p != NULL) {
		ptr = (SFImageNative *)p;
		FREE_IF_NZ (ptr);
	}
}

void
SFImageNativeAssign(void *top, void *fromp)
{
	SFImageNative *to = (SFImageNative *)top;
	/* SFImageNative *from = fromp; */
	UNUSED(fromp);

	to->touched++;
/* 	(to->v) = (from->v); */
}

void *
SFRotationNativeNew()
{
	SFRotationNative *ptr;
	ptr = (SFRotationNative *)MALLOC(sizeof(*ptr));
	ptr->touched = 0;
	return ptr;
}

void
SFRotationNativeDelete(void *p)
{
	SFRotationNative *ptr;
	if (p != NULL) {
		ptr = (SFRotationNative *)p;
		FREE_IF_NZ (ptr);
	}
}

void
SFRotationNativeAssign(void *top, void *fromp)
{
	SFRotationNative *to = (SFRotationNative *)top;
	SFRotationNative *from = (SFRotationNative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFVec2fNativeNew()
{
	SFVec2fNative *ptr;
	ptr = (SFVec2fNative *)MALLOC(sizeof(*ptr));
	ptr->touched = 0;
	return ptr;
}

void
SFVec2fNativeDelete(void *p)
{
	SFVec2fNative *ptr;
	if (p != NULL) {
		ptr = (SFVec2fNative *)p;
		FREE_IF_NZ (ptr);
	}
}

void
SFVec2fNativeAssign(void *top, void *fromp)
{
	SFVec2fNative *to = (SFVec2fNative *)top;
	SFVec2fNative *from = (SFVec2fNative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}

void *
SFVec3fNativeNew()
{
	SFVec3fNative *ptr;
	ptr = (SFVec3fNative *)MALLOC(sizeof(*ptr));
	ptr->touched = 0;
	return ptr;
}

void
SFVec3fNativeDelete(void *p)
{
	SFVec3fNative *ptr;
	if (p != NULL) {
		ptr = (SFVec3fNative *)p;
		FREE_IF_NZ (ptr);
	}
}

void
SFVec3fNativeAssign(void *top, void *fromp)
{
	SFVec3fNative *to = (SFVec3fNative *)top;
	SFVec3fNative *from = (SFVec3fNative *)fromp;
	to->touched++;
	(to->v) = (from->v);
}


/* A new version of InitScriptField which takes "nicer" arguments; currently a
 * simple and restricted wrapper, but it could replace it soon? */
/* Parameters:
	num:		Script number. Starts at 0. 
	kind:		One of PKW_eventIn, PKW_eventOut, PKW_field
	type:		One of the FIELDTYPE_ defines, eg, FIELDTYPE_MFFloat
	field:		the field name as found in the VRML/X3D file. eg "set_myField"
		
*/

void InitScriptFieldC(int num, indexT kind, indexT type, char* field, union anyVrml value) {
	jsval rval;
	char *smallfield = NULL;
	char mynewname[400];
	char thisValue[100];
	int rows, elements;
	char *sftype = NULL;

	int haveMulti;
	int MFhasECMAtype;
	int rowCount, eleCount;

	int tlen;
	struct Multi_Int32*     vrmlImagePtr;
	float *FloatPtr;
	uintptr_t  *VoidPtr;
	int *IntPtr;
	double *DoublePtr;
	struct Uni_String **SVPtr;
	int *iptr; int SFImage_depth; int SFImage_wid; int SFImage_hei;


	uintptr_t defaultVoid[] = {0,0};
	float defaultFloat[] = {0.0,0.0,0.0,0.0};
	int defaultInt[] = {0,0,0,0};
	double defaultDouble[] = {0.0, 0.0};
	struct Uni_String *sptr[1];

	 #ifdef JAVASCRIPTVERBOSE
	printf ("\nInitScriptFieldC, num %d, kind %d type %d field %s value %d\n", num,kind,type,field,value);
	#endif
	

        /* input check */
	if (kind == X3DACCESSOR_inputOnly) kind = PKW_eventIn;
	else if (kind == X3DACCESSOR_outputOnly) kind = PKW_eventOut;
	else if (kind == X3DACCESSOR_inputOutput) kind = PKW_field;

        if ((kind != PKW_eventIn) && (kind != PKW_eventOut) && (kind != PKW_field)) {
                ConsoleMessage ("InitScriptField: invalid kind for script: %d\n",kind);
                return;
        }

        if (type >= FIELDTYPES_COUNT) {
                ConsoleMessage ("InitScriptField: invalid type for script: %d\n",type);
                return;
        }


	/* ok, lets handle the types here */
	switch (type) {
		/* ECMA types */
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFFloat:
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFImage:
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFString: {
			/* do not care about eventIns */
			if (kind != PKW_eventIn)  {
				JSaddGlobalECMANativeProperty(num, field);

				if (kind == PKW_field) {
					if (type == FIELDTYPE_SFImage) {
							vrmlImagePtr = &(value.sfimage);

					} else if  (type == FIELDTYPE_SFString) {
						tlen = strlen(value.sfstring->strptr) + 20;
					} else {
						tlen = strlen(field) + 20;
					}
					smallfield = MALLOC (tlen);
					smallfield[0] = '\0';

					switch (type) {
						case FIELDTYPE_SFFloat: sprintf (smallfield,"%s=%f\n",field,value.sffloat);break;
						case FIELDTYPE_SFTime: sprintf (smallfield,"%s=%f\n",field,value.sftime);break;
						case FIELDTYPE_SFInt32: sprintf (smallfield,"%s=%d\n",field,value.sfint32); break;
						case FIELDTYPE_SFBool: 
							if (value.sfbool == 1) sprintf (smallfield,"%s=true",field);
							else sprintf (smallfield,"%s=false",field);
							break;
						case FIELDTYPE_SFImage: 
							iptr = vrmlImagePtr->p;
                					SFImage_wid = *iptr; iptr++;
              						SFImage_hei = *iptr; iptr++;
                					SFImage_depth = *iptr; iptr++;
printf ("image wid %d hei %d depth %d\n",SFImage_wid, SFImage_hei, SFImage_depth);

							break;
						case FIELDTYPE_SFString:  
							sprintf (smallfield,"%s=\"%s\"\n",field,value.sfstring->strptr); break;
					}
					if (!ActualrunScript(num,smallfield,&rval))
						printf ("huh??? Field initialization script failed %s\n",smallfield);
				}
			}
			break;
		}
		/* non ECMA types */
		default: {
			/* first, do we need to make a new name up? */
			if (kind == PKW_eventIn) {
				sprintf (mynewname,"__tmp_arg_%s",field);
			} else strcpy(mynewname,field);


			/* get an appropriate pointer - we either point to the initialization value
			   in the script header, or we point to some data here that are default values */
			
			/* does this MF type have an ECMA type as a single element? */
			switch (type) {
				case FIELDTYPE_MFString:
				case FIELDTYPE_MFTime:
				case FIELDTYPE_MFBool:
				case FIELDTYPE_MFInt32:
				case FIELDTYPE_MFNode:
				case FIELDTYPE_MFFloat: 
					MFhasECMAtype = TRUE;
					break;
				default: {
					MFhasECMAtype = FALSE;
				}
			}

			elements=0;
			IntPtr = NULL;
			FloatPtr = NULL;
			DoublePtr = NULL;
			SVPtr = NULL;
			VoidPtr = NULL;
			if (kind == PKW_field) {
				switch (type) {
					case FIELDTYPE_SFNode:
						VoidPtr = (uintptr_t *) (&(value.sfnode)); elements = 1;
						break;
					case FIELDTYPE_MFColor:
						FloatPtr = (float *) value.mfcolor.p; elements = value.mfcolor.n;
						break;
					case FIELDTYPE_MFColorRGBA:
						FloatPtr = (float *) value.mfcolorrgba.p; elements = value.mfcolorrgba.n;
						break;
					case FIELDTYPE_MFVec2f:
						FloatPtr = (float *) value.mfvec2f.p; elements = value.mfvec2f.n;
						break;
					case FIELDTYPE_MFVec3f:
						FloatPtr = (float *) value.mfvec3f.p; elements = value.mfvec3f.n;
						break;
					case FIELDTYPE_MFRotation: 
						FloatPtr = (float *) value.mfrotation.p; elements = value.mfrotation.n;
						break;
					case FIELDTYPE_SFVec2f:
						FloatPtr = (float *) value.sfvec2f.c; elements = 1;
						break;
					case FIELDTYPE_SFColor:
						FloatPtr = value.sfcolor.c; elements = 1;
						break;
					case FIELDTYPE_SFColorRGBA:
						FloatPtr = value.sfcolorrgba.r; elements = 1;
						break;
					case FIELDTYPE_SFRotation:
						FloatPtr = value.sfrotation.r; elements = 1;
						break;
					case FIELDTYPE_SFVec3f: 
						FloatPtr = value.sfvec3f.c; elements =1;
						break;
					case FIELDTYPE_MFString:
						SVPtr = value.mfstring.p; elements = value.mfstring.n;
						break;
					case FIELDTYPE_MFTime:
						DoublePtr = value.mftime.p; elements = value.mftime.n;
						break;
					case FIELDTYPE_MFBool:
						IntPtr = value.mfbool.p; elements = value.mfbool.n;
						break;
					case FIELDTYPE_MFInt32:
						IntPtr = value.mfint32.p; elements = value.mfint32.n;
						break;
					case FIELDTYPE_MFNode:
						VoidPtr = ((uintptr_t*)(value.mfnode.p)); elements = value.mfnode.n;
						break;
					case FIELDTYPE_MFFloat: 
						FloatPtr = value.mffloat.p; elements = value.mffloat.n;
						break;
					default: {
						printf ("unhandled type, in InitScriptField %d\n",type);
						return;
					}
				}

			} else {


				/* make up a default pointer */
				elements = 1;
				switch (type) {
					/* Void types */
					case FIELDTYPE_SFNode:
					case FIELDTYPE_MFNode:
						VoidPtr = defaultVoid; 
						break;

					/* Float types */
					case FIELDTYPE_MFColor:
					case FIELDTYPE_MFColorRGBA:
					case FIELDTYPE_MFVec2f:
					case FIELDTYPE_MFVec3f:
					case FIELDTYPE_MFRotation: 
					case FIELDTYPE_SFVec2f:
					case FIELDTYPE_SFColor:
					case FIELDTYPE_SFColorRGBA:
					case FIELDTYPE_SFRotation:
					case FIELDTYPE_SFVec3f: 
					case FIELDTYPE_MFFloat: 
						FloatPtr = defaultFloat;
						break;

					/* Int types */
					case FIELDTYPE_MFBool:
					case FIELDTYPE_MFInt32:
						IntPtr = defaultInt;
						break;

					/* String types */
					case FIELDTYPE_SFString:
					case FIELDTYPE_MFString:
						sptr[0] = newASCIIString("");
						SVPtr = sptr;
						break;

					/* Double types */
					case FIELDTYPE_MFTime:
					case FIELDTYPE_SFTime:
						DoublePtr = defaultDouble;
						break;
						
					default: {
						printf ("unhandled type, in InitScriptField %d\n",type);
						return;
					}
				}

			}

			rows = returnElementRowSize (type);

			#ifdef JAVASCRIPTVERBOSE
			printf ("in fieldSet, we have ElementRowSize %d and individual elements %d\n",rows,elements);
			#endif

			smallfield = MALLOC ( (elements*15) + 100);

			/* what is the equivalent SF for this MF?? */
			if (type != convertToSFType(type)) haveMulti = TRUE;
			 else haveMulti = FALSE;
			
			/* the sftype is the SF form of either the MF or SF */
			sftype = strdup(FIELDTYPES[convertToSFType(type)]);

			/* SFStrings are Strings */
			if (strncmp(sftype,"SFString",8)==0) strcpy (sftype,"String");

			/* start the string */
			smallfield[0] = '\0';

			/* is this an MF variable, with SFs in it? */
			if (haveMulti) {
				strcat (smallfield, "new ");
				strcat (smallfield, FIELDTYPES[type]);
				strcat (smallfield, "(");
			}

			/* loop through, and put values in */
			for (eleCount=0; eleCount<elements; eleCount++) {
				/* ECMA native types can just be passed in... */
				if (!MFhasECMAtype) {
					strcat (smallfield, "new ");
					strcat (smallfield, sftype);
					strcat (smallfield, "(");
				}

				/* go through the SF type; SFints will have 1; SFVec3f's will have 3, etc */
				for (rowCount=0; rowCount<rows; rowCount++ ) {
					if (IntPtr != NULL) {
						sprintf (thisValue,"%d",*IntPtr); IntPtr++;
					} else if (FloatPtr != NULL) {
						sprintf (thisValue,"%f",*FloatPtr); FloatPtr++;
					} else if (DoublePtr != NULL) {
						sprintf (thisValue,"%f",*DoublePtr); DoublePtr++;
					} else if (FloatPtr != NULL) {
						sprintf (thisValue,"%d",*FloatPtr); FloatPtr++;
					} else if (SVPtr != NULL) {
						sptr[0] = *SVPtr; SVPtr++;
						sprintf (thisValue,"\"%s\"",sptr[0]->strptr);
					} else { /* must be a Void */
						sprintf (thisValue,"%d",*VoidPtr); VoidPtr++;
					}
					strcat (smallfield, thisValue);
					if (rowCount < (rows-1)) strcat (smallfield,",");
				}

				if (!MFhasECMAtype) strcat (smallfield, ")");
				if (eleCount < (elements-1)) strcat (smallfield,",");

			}

			if (haveMulti) {
				strcat (smallfield,")");
			}
				
			/* Warp factor 5, Dr Sulu... */
			#ifdef JAVASCRIPTVERBOSE 
			printf ("JScript, for newname %s, sending %s\n",mynewname,smallfield); 
			#endif

			JSaddGlobalAssignProperty (num,mynewname,smallfield);
		}
	}

	FREE_IF_NZ (smallfield);
	FREE_IF_NZ (sftype);

}

int JSaddGlobalECMANativeProperty(uintptr_t num, char *name) {
	JSContext *_context;
	JSObject *_globalObj;

	char buffer[STRING];
	jsval _val, rval = INT_TO_JSVAL(0);

	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[num].cx;
	_globalObj = (JSObject *)ScriptControl[num].glob;

	#ifdef  JAVASCRIPTVERBOSE
		printf("addGlobalECMANativeProperty: name \"%s\"\n", name);
	#endif
	

	if (!JS_DefineProperty(_context, _globalObj, name, rval,
						   NULL, setECMANative,
						   0 | JSPROP_PERMANENT)) {
		printf("JS_DefineProperty failed for \"%s\" in addGlobalECMANativeProperty.\n",
				name);
		return JS_FALSE;
	}

	memset(buffer, 0, STRING);
	sprintf(buffer, "_%s_touched", name);
	_val = INT_TO_JSVAL(0);

	if (!JS_SetProperty(_context, _globalObj, buffer, &_val)) {
		printf("JS_SetProperty failed for \"%s\" in addGlobalECMANativeProperty.\n",
				buffer);
		return JS_FALSE;
	}
	return JS_TRUE;
}

int JSaddGlobalAssignProperty(uintptr_t num, char *name, char *str) {
	jsval _rval = INT_TO_JSVAL(0);
	JSContext *_context;
	JSObject *_globalObj;

	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[num].cx;
	_globalObj = (JSObject *)ScriptControl[num].glob;
	#ifdef JAVASCRIPTVERBOSE 
		printf("addGlobalAssignProperty: cx: %d obj %d name \"%s\", evaluate script \"%s\"\n",
			   _context, _globalObj, name, str);
	#endif

	if (!JS_EvaluateScript(_context, _globalObj, str, strlen(str),
						   FNAME_STUB, LINENO_STUB, &_rval)) {
		printf("JS_EvaluateScript failed for \"%s\" in addGlobalAssignProperty.\n",
				str);
		return JS_FALSE;
	}
	if (!JS_DefineProperty(_context, _globalObj, name, _rval,
						   getAssignProperty, setAssignProperty,
						   0 | JSPROP_PERMANENT)) {
		printf("JS_DefineProperty failed for \"%s\" in addGlobalAssignProperty.\n",
				name);
		return JS_FALSE;
	}
	return JS_TRUE;
}
