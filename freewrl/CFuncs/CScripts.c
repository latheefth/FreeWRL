/* Sourcecode for CScripts.h */

#include "CScripts.h"

#include "jsapi.h"
#include "jsUtils.h"
#include "headers.h"

#include <assert.h>
#include <stdlib.h>

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

struct ScriptFieldDecl* newScriptFieldDecl(indexT mod, indexT type, indexT name)
{
 struct ScriptFieldDecl* ret=MALLOC(sizeof(struct ScriptFieldDecl));
 assert(ret);

 assert(mod!=PKW_exposedField);

 ret->fieldDecl=newFieldDecl(mod, type, name);
 assert(ret->fieldDecl);

 /* Stringify */
 ret->name=fieldDecl_getStringName(ret->fieldDecl);
 ret->type=FIELDTYPES[type];

 /* Field's value not yet initialized! */
 ret->valueSet=(mod!=PKW_field);
 /* value is set later on */

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
 assert(me->fieldDecl->mode==PKW_field);
 assert(!me->valueSet);

 me->value=v;

 me->valueSet=TRUE;
}

/* Get "offset" data for routing */
int scriptFieldDecl_getRoutingOffset(struct ScriptFieldDecl* me)
{
 return JSparamIndex(me->name, me->type);
}

/* Initialize JSField */
void scriptFieldDecl_jsFieldInit(struct ScriptFieldDecl* me, uintptr_t num)
{
 assert(me->valueSet);
 InitScriptFieldC(num, me->fieldDecl->mode, me->fieldDecl->type,
  me->name, me->value);
}

/* ************************************************************************** */
/* ********************************** Script ******************************** */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

/* Next handle to be assinged */
static uintptr_t handleCnt=0;

uintptr_t nextScriptHandle (void) {uintptr_t retval; retval = handleCnt; handleCnt++; return retval;}

/* on a reload, zero script counts */
void zeroScriptHandles (void) {handleCnt = 0;}

struct Script* newScript()
{
 struct Script* ret=MALLOC(sizeof(struct Script));
 assert(ret);

 ret->num=nextScriptHandle();
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
 vector_pushBack(struct ScriptFieldDecl*, me->fields, field);
 scriptFieldDecl_jsFieldInit(field, me->num);
}

BOOL script_initCode(struct Script* me, const char* code)
{
 jsval jsret;
 assert(!me->loaded);

 if(!ActualrunScript(me->num, code, &jsret))
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

 /* printf ("script_initCodeFromUri, uri is %s\n",uri); */
 filename = (char *)MALLOC(1000);

 /* get the current parent */
 mypath = STRDUP(getInputURL());

 /* and strip off the file name, leaving any path */
 removeFilenameFromPath (mypath);

 /* add the two together */
 makeAbsoluteFileName(filename,mypath,uri);

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
