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
 struct ScriptFieldDecl* ret=malloc(sizeof(struct ScriptFieldDecl));
 assert(ret);

 ret->fieldDecl=newFieldDecl(mod, type, name);

 /* Stringify */
 ret->kind=PROTOKEYWORDS[mod];
 ret->type=FIELDTYPES[type];
 ret->name=fieldDecl_getStringName(ret->fieldDecl);

 ret->value=NULL;

 return ret;
}

void deleteScriptFieldDecl(struct ScriptFieldDecl* me)
{
 if(me->value)
  free(me->value);
 
 deleteFieldDecl(me->fieldDecl);
 free(me);
}

/* Other members */
/* ************* */

/* Get "offset" data for routing */
int scriptFieldDecl_getRoutingOffset(struct ScriptFieldDecl* me)
{
 return JSparamIndex(me->name, me->type);
}

/* ************************************************************************** */
/* ********************************** Script ******************************** */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

/* Next handle to be assinged */
static uintptr_t handleCnt=0;

struct Script* newScript()
{
 struct Script* ret=malloc(sizeof(struct Script));
 assert(ret);

 ret->num=(handleCnt++);
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
 
 free(me);
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
 InitScriptField(me->num, field->kind, field->type, field->name, "");
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

BOOL script_initCodeFromUri(struct Script* me, const char* uri)
{
 size_t i;
 
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

  if(!*v && *u==':')
   return script_initCode(me, u+1);
 }

 /* Other protocols not supported */
 return FALSE;
}

BOOL script_initCodeFromMFUri(struct Script* me, const struct Multi_String* s)
{
 size_t i;
 int xx;
 for(i=0; i!=s->n; ++i)
  if(script_initCodeFromUri(me, SvPV(s->p[i], xx)))
   return TRUE;

 return FALSE;
}
