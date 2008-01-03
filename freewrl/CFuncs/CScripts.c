/*******************************************************************
  Copyright (C) 2007 Daniel Kraft,  John Stewart, CRC Canada.
  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
  See the GNU Library General Public License (file COPYING in the distribution)
  for conditions of use and redistribution.
 *********************************************************************/


/* Sourcecode for CScripts.h */

#include "CScripts.h"

#include "jsapi.h"
#include "jsUtils.h"
#include "headers.h"

#include <assert.h>
#include <stdlib.h>

#undef CPARSERVERBOSE

/* JavaScript-"protocols" */
const char* JS_PROTOCOLS[]={
 "javascript",
 "ecmascript",
 "vrmlscript"};

/* ************************************************************************** */
/* ****************************** ScriptFieldDecl *************************** */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

struct ScriptFieldDecl* newScriptFieldDecl(struct VRMLLexer* me, indexT mod, indexT type, indexT name)
{
 struct ScriptFieldDecl* ret=MALLOC(sizeof(struct ScriptFieldDecl));

 assert(ret);

 assert(mod!=PKW_inputOutput);

 ret->fieldDecl=newFieldDecl(mod, type, name);
 assert(ret->fieldDecl);

 /* Stringify */
 ret->name=fieldDecl_getStringName(me, ret->fieldDecl);
 ret->type=FIELDTYPES[type];
 ret->ISname = NULL;

 /* Field's value not yet initialized! */
 ret->valueSet=(mod!=PKW_initializeOnly);
 /* value is set later on */

 #ifdef CPARSERVERBOSE
 printf ("newScriptFieldDecl, returning name %s, type %s, mode %s\n",ret->name, ret->type,PROTOKEYWORDS[ret->fieldDecl->mode]); 
 #endif

 return ret;
}

/* Create a new ScriptFieldInstanceInfo structure to hold information about script fields that are destinations for IS statements in PROTOs */
struct ScriptFieldInstanceInfo* newScriptFieldInstanceInfo(struct ScriptFieldDecl* dec, struct Script* script) {
	struct ScriptFieldInstanceInfo* ret = MALLOC(sizeof(struct ScriptFieldInstanceInfo));
	
	assert(ret);

	ret->decl = dec;
	ret->script = script;

	#ifdef CPARSERVERBOSE
	printf("creating new scriptfieldinstanceinfo with decl %p script %p\n", dec, script); 
	#endif

	return(ret);
}

/* Copy a ScriptFieldInstanceInfo structure to a new structure */
struct ScriptFieldInstanceInfo* scriptFieldInstanceInfo_copy(struct ScriptFieldInstanceInfo* me) {
	struct ScriptFieldInstanceInfo* ret = MALLOC(sizeof(struct ScriptFieldInstanceInfo));

	#ifdef CPARSERVERBOSE
	printf("copying instanceinfo %p (%p %p) to %p\n", me, me->decl, me->script, ret);
	#endif

	
	assert(ret);

	ret->decl = me->decl;
	ret->script = me->script;

	return ret;
}

struct ScriptFieldDecl* scriptFieldDecl_copy(struct VRMLLexer* lex, struct ScriptFieldDecl* me) 
{
	struct ScriptFieldDecl* ret = MALLOC(sizeof (struct ScriptFieldDecl));
	assert(ret);

	#ifdef CPARSERVERBOSE
	printf("copying script field decl %p to %p\n", me, ret);
	#endif


	ret->fieldDecl = fieldDecl_copy(me->fieldDecl);
	assert(ret->fieldDecl);	

	ret->name = fieldDecl_getStringName(lex, ret->fieldDecl);
	ret->type = me->type;
	ret->ISname = me->ISname;
	
	ret->valueSet=((ret->fieldDecl->mode)!=PKW_initializeOnly);
	return ret;
}

void deleteScriptFieldDecl(struct ScriptFieldDecl* me)
{
 deleteFieldDecl(me->fieldDecl);
 FREE_IF_NZ (me);
}

/* Other members */
/* ************* */

/* Sets script field value */
void scriptFieldDecl_setFieldValue(struct ScriptFieldDecl* me, union anyVrml v)
{
 assert(me->fieldDecl->mode==PKW_initializeOnly); 
 /* assert(!me->valueSet); */


 me->value=v;

 me->valueSet=TRUE;
}

/* Get "offset" data for routing */
int scriptFieldDecl_getRoutingOffset(struct ScriptFieldDecl* me)
{
 return JSparamIndex((char *)me->name, (char *)me->type);
}

/* Initialize JSField */
void scriptFieldDecl_jsFieldInit(struct ScriptFieldDecl* me, uintptr_t num) {
	#ifdef CPARSERVERBOSE
	printf ("scriptFieldDecl_jsFieldInit mode %d\n",me->fieldDecl->mode);
	#endif

	assert(me->valueSet);
 	InitScriptFieldC(num, me->fieldDecl->mode, me->fieldDecl->type, me->name, me->value);
}

/* ************************************************************************** */
/* ********************************** Script ******************************** */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

/* Next handle to be assinged */
static uintptr_t handleCnt=0;

uintptr_t nextScriptHandle (void) {uintptr_t retval; retval = handleCnt; handleCnt++; return retval;}

/* copy a Script node in a proto. */
struct X3D_Script * protoScript_copy (struct X3D_Script *me) {
	struct X3D_Script* ret;

	ret = createNewX3DNode(NODE_Script);
	ret->__parenturl = me->__parenturl;
	ret->directOutput = me->directOutput;
	ret->mustEvaluate = me->mustEvaluate;
	ret->url = me->url;
	ret->__scriptObj = me->__scriptObj;

	/* script handle gets updated in registerScriptInPROTO */
	/* ((struct Script *) (ret->__scriptObj))->num = nextScriptHandle(); */

	return ret;
	
}

/* on a reload, zero script counts */
void zeroScriptHandles (void) {handleCnt = 0;}

struct Script* newScript(void)
{
 struct Script* ret=MALLOC(sizeof(struct Script));
 assert(ret);

 ret->num=nextScriptHandle();
 	#ifdef CPARSERVERBOSE
	printf("newScript: created new script with num %d\n", ret->num);
	#endif
 ret->loaded=FALSE;

 ret->fields=newVector(struct ScriptFieldDecl*, 4);

 JSInit(ret->num);

 return ret;
}

void deleteScript(struct Script* me)
{
 size_t i;
 for(i=0; i!=vector_size(me->fields); ++i)
  deleteScriptFieldDecl(vector_get(struct ScriptFieldDecl*, me->fields, i));
 deleteVector(struct ScriptFieldDecl*, me->fields);
 
 FREE_IF_NZ (me);
 /* FIXME:  JS-handle is not freed! */
}

/* Other members */
/* ************* */

struct ScriptFieldDecl* script_getField(struct Script* me, indexT n, indexT mod)
{
 size_t i;
 for(i=0; i!=vector_size(me->fields); ++i)
 {
  struct ScriptFieldDecl* curField=
   vector_get(struct ScriptFieldDecl*, me->fields, i);
  if(scriptFieldDecl_isField(curField, n, mod))
   return curField;
 }

 return NULL;
}

void script_addField(struct Script* me, struct ScriptFieldDecl* field)
{
 #ifdef CPARSERVERBOSE
 printf ("script_addField: adding field %p to script %d (pointer %p)\n",field,me->num,me); 
 #endif 

 vector_pushBack(struct ScriptFieldDecl*, me->fields, field);
 scriptFieldDecl_jsFieldInit(field, me->num);
}

BOOL script_initCode(struct Script* me, const char* code)
{
 jsval jsret;
 assert(!me->loaded);

 if(!ACTUALRUNSCRIPT(me->num, (char *)code, &jsret))
  return FALSE;

 me->loaded=TRUE;
 return TRUE;
}

/* get the script from this SFString. First checks to see if the string
   contains the script; if not, it goes and tries to see if the SFString 
   contains a file that (hopefully) contains the script */

BOOL script_initCodeFromUri(struct Script* me, const char* uri)
{
 size_t i;
 char *filename = NULL;
 char *buffer = NULL;
 char *mypath = NULL;
 char firstBytes[4]; /* not used here, but required for function call */
 int rv;

 /* strip off whitespace at the beginning JAS */
 while ((*uri<= ' ') && (*uri>0)) uri++;

 /* Try javascript protocol */
 for(i=0; i!=ARR_SIZE(JS_PROTOCOLS); ++i)
 {
  const char* u=uri;
  const char* v=JS_PROTOCOLS[i];

  while(*u && *v && *u==*v)
  {
   ++u;
   ++v;
  }

  /* Is this a simple "javascript:" "ecmascript:" or "vrmlscript:" uri? JAS*/
  if(!*v && *u==':')
   return script_initCode(me, u+1);
 }

 /* Not a valid script in this SFString. Lets see if this
    is this a possible file that we have to get? */

 #ifdef CPARSERVERBOSE
 printf ("script_initCodeFromUri, uri is %s\n",uri); 
 #endif

 filename = (char *)MALLOC(1000);

 /* get the current parent */
 mypath = STRDUP(getInputURL());

 /* and strip off the file name, leaving any path */
 removeFilenameFromPath (mypath);

 /* add the two together */
 makeAbsoluteFileName(filename,mypath,(char *)uri);

 /* and see if it exists. If it does, try running script_initCode() on it */
 if (fileExists(filename,firstBytes,TRUE)) {
	buffer = readInputString(filename,"");
	rv = script_initCode(me,buffer);
	FREE_IF_NZ (filename);
	FREE_IF_NZ (buffer);
	FREE_IF_NZ (mypath);
	return rv;
 }
 FREE_IF_NZ (filename);
 FREE_IF_NZ (buffer);
 FREE_IF_NZ (mypath);


 /* Other protocols not supported */
 return FALSE;
}

BOOL script_initCodeFromMFUri(struct Script* me, const struct Multi_String* s)
{
 size_t i;
 for(i=0; i!=s->n; ++i)
  if(script_initCodeFromUri(me, s->p[i]->strptr))
   return TRUE;

 return FALSE;
}
