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

/* #define MAX_RUNTIME_BYTES 0x100000L*/
#define MAX_RUNTIME_BYTES 0x1000000L
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

static JSRuntime *runtime;
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

char *DefaultScriptMethods = "function initialize() {}; function shutdown() {}; function eventsProcessed() {}; TRUE=true; FALSE=false; function print(x) {Browser.print(x)} ";

/* housekeeping routines */
void kill_javascript(void) {
}


void cleanupDie(uintptr_t num, const char *msg) {
	kill_javascript();
	freewrlDie(msg);
}

void JSMaxAlloc() {
	/* perform some reallocs on JavaScript database stuff for interfacing */
	uintptr_t count;

	JSMaxScript += 10;
	ScriptControl = (struct CRscriptStruct*)realloc (ScriptControl, sizeof (*ScriptControl) * JSMaxScript);
	scr_act = (uintptr_t *)realloc (scr_act, sizeof (*scr_act) * JSMaxScript);

	if ((ScriptControl == NULL) || (scr_act == 0)) {
		printf ("Can not allocate memory for more script indexes\n");
		exit(1);
	}

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
	#ifdef JSVERBOSE 
	printf("init: script %d\n",num);
	#endif

	/* more scripts than we can handle right now? */
	if (num >= JSMaxScript)  {
		JSMaxAlloc();
	}


	runtime = JS_NewRuntime(MAX_RUNTIME_BYTES);
	if (!runtime) freewrlDie("JS_NewRuntime failed");

	#ifdef JSVERBOSE 
	printf("\tJS runtime created,\n");
	#endif


	_context = JS_NewContext(runtime, STACK_CHUNK_SIZE);
	if (!_context) freewrlDie("JS_NewContext failed");

	#ifdef JSVERBOSE 
	printf("\tJS context created,\n");
	#endif


	_globalObj = JS_NewObject(_context, &globalClass, NULL, NULL);
	if (!_globalObj) freewrlDie("JS_NewObject failed");

	#ifdef JSVERBOSE 
	printf("\tJS global object created,\n");
	#endif


	/* gets JS standard classes */
	if (!JS_InitStandardClasses(_context, _globalObj))
		freewrlDie("JS_InitStandardClasses failed");

	#ifdef JSVERBOSE 
	printf("\tJS standard classes initialized,\n");
	#endif



	/* #ifdef JSVERBOSE {*/
	/* 	reportWarningsOn();*/
	/* } else {*/
	/* 	reportWarningsOff();*/
	/* }*/

	JS_SetErrorReporter(_context, errorReporter);
	#ifdef JSVERBOSE 
	printf("\tJS errror reporter set,\n");
	#endif


	br = (BrowserNative *) JS_malloc(_context, sizeof(BrowserNative));

	/* for this script, here are the necessary data areas */
	ScriptControl[num].cx = (uintptr_t) _context;
	ScriptControl[num].glob = (uintptr_t) _globalObj;
	ScriptControl[num].brow = (uintptr_t) br;


	if (!loadVrmlClasses(_context, _globalObj))
		freewrlDie("loadVrmlClasses failed");

	#ifdef JSVERBOSE 
	printf("\tVRML classes loaded,\n");
	#endif



	if (!VrmlBrowserInit(_context, _globalObj, br))
		freewrlDie("VrmlBrowserInit failed");

	#ifdef JSVERBOSE 
	printf("\tVRML Browser interface loaded,\n");
	#endif


	/* now, run initial scripts */
	if (!ActualrunScript(num,DefaultScriptMethods,&rval))
		cleanupDie(num,"runScript failed in VRML::newJS DefaultScriptMethods");

	/* send this data over to the routing table functions. */
	CRoutes_js_new (num, JAVASCRIPT);

	#ifdef JSVERBOSE 
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

	#ifdef JSVERBOSE
		printf("ActualrunScript script %d cx %x \"%s\", \n",
			   num, _context, script);
	#endif
	len = strlen(script);
	if (!JS_EvaluateScript(_context, _globalObj, script, len,
						   FNAME_STUB, LINENO_STUB, rval)) {
		printf("JS_EvaluateScript failed for \"%s\".\n", script);
		return JS_FALSE;
	 }

	#ifdef JSVERBOSE 
	printf ("runscript passed\n");
	#endif

	return JS_TRUE;
}



/* run the script from within Javascript  */
int jsrrunScript(JSContext *_context, JSObject *_globalObj, char *script, jsval *rval) {

	size_t len;

	#ifdef JSVERBOSE
		printf("jsrrunScript script cx %x \"%s\", \n",
			   _context, script);
	#endif

	len = strlen(script);
	if (!JS_EvaluateScript(_context, _globalObj, script, len,
						   FNAME_STUB, LINENO_STUB, rval)) {
		printf("JS_EvaluateScript failed for \"%s\".\n", script);
		return JS_FALSE;
	 }

	#ifdef JSVERBOSE 
	printf ("runscript passed\n");
	#endif

	return JS_TRUE;
}


/* FROM VRMLC.pm */

void *
SFNodeNativeNew()
{
	SFNodeNative *ptr;
	ptr = (SFNodeNative *) malloc(sizeof(*ptr));

	/* printf ("SFNodeNativeNew; string len %d handle_len %d\n",vrmlstring_len,handle_len);*/

	if (ptr == NULL) {
		return NULL;
	}
	ptr->handle = 0;
	ptr->touched = 0;
	return ptr;
}

void
SFNodeNativeDelete(void *p)
{
	SFNodeNative *ptr;
	if (p != NULL) {
		ptr = (SFNodeNative *)p;
		FREE_IF_NZ (ptr->X3DString);
		free(ptr);
	}
}

/* assign this internally to the Javascript engine environment */
int
SFNodeNativeAssign(void *top, void *fromp)
{
	size_t to_vrmlstring_len = 0, to_handle_len = 0,
		from_vrmlstring_len = 0, from_handle_len = 0;
	SFNodeNative *to = (SFNodeNative *)top;
	SFNodeNative *from = (SFNodeNative *)fromp;

	/* indicate that this was touched; and copy contents over */
	to->touched++;
	to->handle = from->handle;
	to->X3DString = strdup(from->X3DString);

	#ifdef JSVERBOSE
	printf ("SFNodeNativeAssign, copied %d to %d, handle %d, string %s\n", from, to, to->handle, to->X3DString);
	#endif

	return JS_TRUE;
}


void *
SFColorRGBANativeNew()
{
	SFColorRGBANative *ptr;
	ptr = (SFColorRGBANative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFColorRGBANativeDelete(void *p)
{
	SFColorRGBANative *ptr;
	if (p != NULL) {
		ptr = (SFColorRGBANative *)p;
		free(ptr);
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
	ptr = (SFColorNative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFColorNativeDelete(void *p)
{
	SFColorNative *ptr;
	if (p != NULL) {
		ptr = (SFColorNative *)p;
		free(ptr);
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
	ptr =(SFImageNative *) malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFImageNativeDelete(void *p)
{
	SFImageNative *ptr;
	if (p != NULL) {
		ptr = (SFImageNative *)p;
		free(ptr);
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
	ptr = (SFRotationNative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFRotationNativeDelete(void *p)
{
	SFRotationNative *ptr;
	if (p != NULL) {
		ptr = (SFRotationNative *)p;
		free(ptr);
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
	ptr = (SFVec2fNative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFVec2fNativeDelete(void *p)
{
	SFVec2fNative *ptr;
	if (p != NULL) {
		ptr = (SFVec2fNative *)p;
		free(ptr);
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
	ptr = (SFVec3fNative *)malloc(sizeof(*ptr));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->touched = 0;
	return ptr;
}

void
SFVec3fNativeDelete(void *p)
{
	SFVec3fNative *ptr;
	if (p != NULL) {
		ptr = (SFVec3fNative *)p;
		free(ptr);
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
	int rows, columns, elements;
	int count;
	char *sftype;

	int haveMulti;
	int MFhasECMAtype;
	int rowCount, eleCount;

	int tlen;
	struct Multi_Int32*     vrmlImagePtr;
	struct Multi_Int32    Int32Ptr;
	float *FloatPtr;
	uintptr_t  *VoidPtr;
	int *IntPtr;
	double *DoublePtr;
	struct Uni_String **SVPtr;
	int *iptr; int SFImage_depth; int SFImage_wid; int SFImage_hei;



	/* printf ("InitScriptFieldC, num %d, kind %d type %d field %s value %d\n",
		num,kind,type,field,value);
	*/

        /* input check */
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
							vrmlImagePtr = value.sfimage;
							tlen = (vrmlImagePtr->n) *10;
printf ("image has %d elements\n",vrmlImagePtr->n);

					} else if  (type == FIELDTYPE_SFString) {
						tlen = strlen(value.sfstring->strptr) + 20;
					} else {
						tlen = strlen(field) + 20;
					}
					smallfield = malloc (tlen);
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


			/* get an appropriate pointer */
			MFhasECMAtype = FALSE;
			elements=0;
			IntPtr = NULL;
			FloatPtr = NULL;
			DoublePtr = NULL;
			SVPtr = NULL;
			VoidPtr = NULL;
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
					MFhasECMAtype = TRUE;
					break;
				case FIELDTYPE_MFTime:
					DoublePtr = value.mftime.p; elements = value.mftime.n;
					MFhasECMAtype = TRUE;
					
					break;
				case FIELDTYPE_MFBool:
					IntPtr = value.mfbool.p; elements = value.mfbool.n;
					MFhasECMAtype = TRUE;
					break;
				case FIELDTYPE_MFInt32:
					IntPtr = value.mfint32.p; elements = value.mfint32.n;
					MFhasECMAtype = TRUE;
					break;
				case FIELDTYPE_MFNode:
					VoidPtr = ((uintptr_t*)(value.mfnode.p)); elements = value.mfnode.n;
					MFhasECMAtype = TRUE;
					break;
				case FIELDTYPE_MFFloat: 
					FloatPtr = value.mffloat.p; elements = value.mffloat.n;
					MFhasECMAtype = TRUE;
					break;
				default: {
					printf ("unhandled type, in InitScriptField %d\n",type);
					return;
				}
			}

			rows = returnElementRowSize (mapFieldTypeToInernaltype(type));

			/* printf ("in fieldSet, we have ElementRowSize %d and individual elements %d\n",rows,elements); */

			smallfield = malloc ( (elements*15) + 100);

			/* what is the equivalent SF for this MF?? */
			sftype = strdup (FIELDTYPES[type]);
			if (sftype[0] == 'M') { sftype[0] = 'S'; haveMulti = TRUE;
			} else { haveMulti = FALSE; }

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
			#ifdef JSVERBOSE
			printf ("JScript, sending %s\n",smallfield); 
			#endif

			JSaddGlobalAssignProperty (num,mynewname,smallfield);
		}
	}

	FREE_IF_NZ (smallfield);

}


/* this is for JS/JS.pm - scripts in X3D. Remove once perl is gone */
void InitScriptField(int num,char *kind,char *type,char *field,char *value) {
	jsval rval;
	int ikind, itype;
	char *smallfield = NULL;
	char mynewname[400];
	char *cptr;
	int rows, commas;
	int elements;
	int count;
	char *sftype;

	itype = convert_typetoInt(type);
	ikind = findFieldInKEYWORDS(kind);

	/*
	printf ("InitScriptField, %d, kind %s (%d)type %s (%d) field %s value %s\n",
			num,kind,ikind,type,itype,field,value);
	*/

	/* input check */
	if ((ikind != KW_eventIn) && (ikind != KW_eventOut) && (ikind != KW_field)) {
		ConsoleMessage ("invalid kind for script field: %s\n",kind);
		return;	
	}

	if (itype == SFUNKNOWN) {
		ConsoleMessage ("invalid type for script field: %s\n",type);
		return;	
	}

	/* ok, lets handle the types here */
	switch (itype) {
		/* ECMA types */
		case SFBOOL:
		case SFFLOAT:
		case SFTIME:
		case SFINT32:
		case SFIMAGE:
		case SFSTRING: {
			/* do not care about eventIns */
			if (ikind != KW_eventIn)  {
				JSaddGlobalECMANativeProperty(num, field);

				if (ikind == KW_field) {
					smallfield = malloc (strlen(value) + strlen(field) + 10);
					sprintf (smallfield,"%s=%s\n",field,value);
					if (!ActualrunScript(num,smallfield,&rval))
						printf ("huh???\n");
				}
			}
			break;
		}
		/* non ECMA types */
		default: {
			/* first, do we need to make a new name up? */
			if (ikind == KW_eventIn) {
				sprintf (mynewname,"__tmp_arg_%s",field);
			} else strcpy(mynewname,field);

			/* SIMPLE MANIPULATION if the string comes in with "[" and/or "]", strip them off. */
			if ((cptr = strchr(value, '[')) != NULL) *cptr = ' ';
			if ((cptr = strchr(value, ']')) != NULL) *cptr = ' ';
			commas = countCommas (value) + 1;
			rows = returnElementRowSize (itype);
			elements = commas/rows;
			sftype = strdup(type);
			if (sftype[0] = 'M') sftype[0] = 'S';
			/* printf ("commas %d, rows %d, elements %d sftype %s\n",commas,rows,elements, sftype); */

			switch (itype) {
				case MFCOLOR:
				case MFCOLORRGBA:
				case MFVEC2F:
				case MFVEC3F:
				case MFROTATION: 
				case FREEWRLPTR:
				case SFNODE:
				case SFVEC2F:
				case SFCOLOR:
				case SFCOLORRGBA:
				case SFROTATION:
				case MFNODE:
				case MFTIME:
				case MFINT32:
				case MFFLOAT: 
				case SFVEC3F: {
					smallfield = malloc (strlen(type) + strlen(value) + 
						(elements*15) + 100);

					/* start the string */
					smallfield[0] = '\0';
					sprintf (smallfield,"new %s(",type);

					/* is this an MF with SF types? */
					if (rows>1) {
						/* is there any value here, at all? */
						if (elements > 0) {
							for (count = 0; count < commas; count++) {
								/* printf ("count %d mod %d\n",count, count % rows); */
	
								/* start of this element? */
								if ((count%rows) == 0) {
									strcat (smallfield,"new ");
									strcat (smallfield, sftype);
									strcat (smallfield, "(");
								}

								/* copy the value over */
								cptr = strchr(value,',');
								if (cptr != NULL) *cptr = '\0';
								strcat (smallfield, value);
								if (cptr != NULL) {
									value = cptr;
									value ++; 
								}
	
								/* end of this element? */
								if (((count+1)%rows) == 0) {
									strcat (smallfield,")");
									/* if we are not at the end, apply a comma */
									if (count <(commas-1)) strcat (smallfield,",");
								}
								else strcat (smallfield,",");
							}
						}

					} else strcat (smallfield, value);

					strcat (smallfield,")");
					JSaddGlobalAssignProperty (num,mynewname,smallfield);
					break;
				}
	
				default: {
					printf ("unhandled itype, in InitScriptField %s\n",type);
					return;
				}
			}
		}
	}

	FREE_IF_NZ (smallfield);
}
