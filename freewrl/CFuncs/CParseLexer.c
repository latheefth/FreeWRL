/* Sourcecode for CParseLexer.h */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h> /* Using pow() to parse floats */

#include "CParseLexer.h"

/* Pre- and suffix for exposed events. */
const char* EXPOSED_EVENT_IN_PRE="set_";
const char* EXPOSED_EVENT_OUT_SUF="_changed";

/* Tables of user-defined IDs */
#define USER_IDS_INIT_SIZE	16
Stack* userNodeNames=NULL;
Stack* userNodeTypesStack=NULL;
struct Vector* userNodeTypesVec=NULL;
struct Vector* user_field=NULL;
struct Vector* user_exposedField=NULL;
struct Vector* user_eventIn=NULL;
struct Vector* user_eventOut=NULL;

/* Maximum id length (input buffer size) */
#define MAX_IDLEN	127
/* Start buffer length for strings */
#define INITIAL_STRINGLEN	256

/* Input data */
#define LEXER_GETINPUT(c) \
 { \
  assert(!me->curID); \
  if(!*me->nextIn) c=EOF; \
  else c=(int)*(me->nextIn++); \
 }
#define LEXER_UNGETINPUT(c) \
 if(c!=EOF) \
 { \
  --(me->nextIn); \
 }

/* Check for eof */
#define CHECK_EOF(var) \
 if((var)==EOF) \
 { \
  me->isEof=TRUE; \
  return FALSE; \
 }

/* Constructor and destructor */

struct VRMLLexer* newLexer()
{
 struct VRMLLexer* ret=MALLOC(sizeof(struct VRMLLexer));

 ret->nextIn=NULL;
 ret->curID=NULL;
 ret->isEof=TRUE;
 
 /* Init id tables */
 userNodeNames=newStack(struct Vector*);
 userNodeTypesStack=newStack(size_t);
 stack_push(size_t, userNodeTypesStack, 0);
 userNodeTypesVec=newVector(char*, USER_IDS_INIT_SIZE);
 user_field=newVector(char*, USER_IDS_INIT_SIZE);
 user_exposedField=newVector(char*, USER_IDS_INIT_SIZE);
 user_eventIn=newVector(char*, USER_IDS_INIT_SIZE);
 user_eventOut=newVector(char*, USER_IDS_INIT_SIZE);
 lexer_scopeIn();

 return ret;
}

void deleteLexer(struct VRMLLexer* me)
{
 FREE_IF_NZ (me->curID);
 FREE_IF_NZ (me);
}

static void lexer_scopeOut_(Stack*);
void lexer_destroyIdStack(Stack* s)
{
 assert(s);
 while(!stack_empty(s))
  lexer_scopeOut_(s);
 deleteStack(struct Vector*, s);
}
void lexer_destroyIdVector(struct Vector* v)
{
 size_t i;
 assert(v);
 for(i=0; i!=vector_size(v); ++i)
  FREE_IF_NZ (vector_get(char*, v, i));
 deleteVector(char*, v);
}

void lexer_destroyData()
{
 #define DESTROY_IDVEC(v) \
  if(v) \
   lexer_destroyIdVector(v); \
  v=NULL;
  
 /* User node names */
 if(userNodeNames)
  lexer_destroyIdStack(userNodeNames);
 userNodeNames=NULL;

 /* User node types */
 DESTROY_IDVEC(userNodeTypesVec)
 if(userNodeTypesStack)
  deleteStack(size_t, userNodeTypesStack);

 /* User fields */
 DESTROY_IDVEC(user_field)
 DESTROY_IDVEC(user_exposedField)
 DESTROY_IDVEC(user_eventIn)
 DESTROY_IDVEC(user_eventOut)
}

/* Scope in and scope out for IDs */

static void lexer_scopeIn_(Stack** s)
{
 if(!*s)
  *s=newStack(struct Vector*);
 stack_push(struct Vector*, *s, newVector(char*, USER_IDS_INIT_SIZE));
}

static void lexer_scopeOut_(Stack* s)
{
 indexT i;
 assert(!stack_empty(s));

 for(i=0; i!=vector_size(stack_top(struct Vector*, s)); ++i)
  FREE_IF_NZ (vector_get(char*, stack_top(struct Vector*, s), i));
 deleteVector(char*, stack_top(struct Vector*, s));
 stack_pop(struct Vector*, s);
}

void lexer_scopeIn()
{
 lexer_scopeIn_(&userNodeNames);
 stack_push(size_t, userNodeTypesStack, vector_size(userNodeTypesVec));
 /* Fields aren't scoped because they need to be accessible in two levels */
}
void lexer_scopeOut()
{
 lexer_scopeOut_(userNodeNames);
 /* PROTO out-scoping is done by parser */
 /*lexer_scopeOut_PROTO();*/
 /* Fields aren't scoped because they need to be accessible in two levels */
}
void lexer_scopeOut_PROTO()
{
 while(vector_size(userNodeTypesVec)>stack_top(size_t, userNodeTypesStack))
 {
  FREE_IF_NZ (vector_back(char*, userNodeTypesVec));
  vector_popBack(char*, userNodeTypesVec);
 }
 stack_pop(size_t, userNodeTypesStack);
}

/* Sets curID of lexer */
#define IS_ID_REST(c) \
 (c>0x20 && c!=0x22 && c!=0x23 && c!=0x27 && c!=0x2C && c!=0x2E && c!=0x5B && \
  c!=0x5C && c!=0x5D && c!=0x7B && c!=0x7D && c!=0x7F)
#define IS_ID_FIRST(c) \
 (IS_ID_REST(c) && (c<0x30 || c>0x39) && c!=0x2B && c!=0x2D)
BOOL lexer_setCurID(struct VRMLLexer* me)
{
 int c;
 char buf[MAX_IDLEN+1];
 char* cur=buf;

 /* If it is already set, simply return. */
 if(me->curID)
  return TRUE;

 lexer_skip(me);

 /* Is it really an ID? */
 LEXER_GETINPUT(c)
 CHECK_EOF(c)
 if(!IS_ID_FIRST(c))
 {
  LEXER_UNGETINPUT(c)
  return FALSE;
 }

 /* Main loop. */
 while(cur!=buf+MAX_IDLEN)
 {
  assert(cur<buf+MAX_IDLEN);
  *cur=c;
  ++cur;
  
  LEXER_GETINPUT(c)
  if(!IS_ID_REST(c))
   goto breakIdLoop;
 }
 parseError("ID buffer length hit!");
breakIdLoop:
 LEXER_UNGETINPUT(c)
 assert(cur<=buf+MAX_IDLEN);
 *cur=0;

 assert(strlen(buf)==(cur-buf));
 assert(!me->curID);
 me->curID=MALLOC(sizeof(char)*(cur-buf+1));

 strcpy(me->curID, buf);
 return TRUE;
}

/* Lexes a keyword */
BOOL lexer_keyword(struct VRMLLexer* me, indexT kw)
{
 if(!lexer_setCurID(me)) return FALSE;
 assert(me->curID);

 if(!strcmp(me->curID, KEYWORDS[kw]))
 {
  FREE_IF_NZ (me->curID);
  return TRUE;
 }

 return FALSE;
}

/* Finds the index of a given string */
indexT lexer_string2id(const char* str, const struct Vector* v)
{
 indexT i;
 for(i=0; i!=vector_size(v); ++i)
  if(!strcmp(str, vector_get(const char*, v, i)))
   return i;
 return ID_UNDEFINED;
}

/* Lexes an ID (node type, field name...) depending on args. */
BOOL lexer_specialID(struct VRMLLexer* me, indexT* retB, indexT* retU,
 const char** builtIn, const indexT builtInCount, struct Vector* user)
{
 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

 if(lexer_specialID_string(me, retB, retU, builtIn, builtInCount, user,
  me->curID))
 {
  FREE_IF_NZ (me->curID);
  return TRUE;
 }

 return FALSE;
}
BOOL lexer_specialID_string(struct VRMLLexer* me, indexT* retB, indexT* retU,
 const char** builtIn, const indexT builtInCount,
 struct Vector* user, const char* str)
{
 indexT i;
 BOOL found=FALSE;

 if(!retB && !retU)
  return FALSE;

 if(retB) *retB=ID_UNDEFINED;
 if(retU) *retU=ID_UNDEFINED;

 /* Try as built-in */
 for(i=0; i!=builtInCount; ++i)
  if(!strcmp(str, builtIn[i]))
  {
   if(retB)
   {
    *retB=i;
    found=TRUE;
   }
   break;
  }

 /* Return if no user list is requested or it is empty */
 if(!user)
  return found;

 /* Already defined user id? */
 for(i=0; i!=vector_size(user); ++i)
  if(!strcmp(str, vector_get(char*, user, i)))
  {
   if(retU)
   {
    *retU=i;
    found=TRUE;
   }
   break;
  }
 
 return found;
}

/* Lexes and defines an ID */
BOOL lexer_defineID(struct VRMLLexer* me, indexT* ret, struct Vector* vec,
 BOOL multi)
{
 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

 /* User list should be created */
 assert(vec);

 /* Look if the ID's already there */
 if(multi)
 {
  size_t i;
  for(i=0; i!=vector_size(vec); ++i)
   if(!strcmp(me->curID, vector_get(const char*, vec, i)))
   {
    FREE_IF_NZ (me->curID);
    *ret=i;
    return TRUE;
   }
 }

 /* Define the id */
 *ret=vector_size(vec);
 vector_pushBack(char*, vec, me->curID);
 me->curID=NULL;
 return TRUE;
}

/* A eventIn terminal symbol */
BOOL lexer_eventIn(struct VRMLLexer* me,
 indexT* rBO, indexT* rBE, indexT* rUO, indexT* rUE)
{
 BOOL found=FALSE;

 if(lexer_specialID(me, rBO, rUO, EVENT_IN, EVENT_IN_COUNT,
  user_eventIn))
  found=TRUE;
 if(rBE) *rBE=ID_UNDEFINED;
 if(rUE) *rUE=ID_UNDEFINED;

 /* XXX:  If not-exposed result was found, don't even try exposed...  Should be
  * ok according to spec.  Otherwise problem with already freed curID, and
  * parsing an 'absolutetly new' one! */
 if(found)
  return TRUE;
 
 /* Exposed field with set_ prefix? */
 if(!lexer_setCurID(me))
  return found;
 assert(me->curID);
 {
  const char* id=me->curID;
  const char* pre=EXPOSED_EVENT_IN_PRE;

  while(*id && *pre)
  {
   /* If we encounter a mismatch, we don't have the prefix! */
   if(*id!=*pre)
    goto noExposedPre;
   ++id;
   ++pre;
  }

  /* If id and not prefix is at end, it can't have pre as prefix! */
  if(*pre)
  {
   assert(!*id);
   goto noExposedPre;
  }

  /* Now, pre is the string with prefix cut off. */
  if(lexer_specialID_string(me, rBE, rUE,
   EXPOSED_FIELD, EXPOSED_FIELD_COUNT, NULL, id))
  {
   FREE_IF_NZ (me->curID);
   found=TRUE;
  }
 }
noExposedPre:

 /* Try without prefix if not found */
 if(!found)
 {
  if(lexer_specialID(me, rBE, rUE, EXPOSED_FIELD, EXPOSED_FIELD_COUNT,
   user_exposedField))
   found=TRUE;
 }
 
 return found;
}

/* A eventOut terminal symbol */
BOOL lexer_eventOut(struct VRMLLexer* me,
 indexT* rBO, indexT* rBE, indexT* rUO, indexT* rUE)
{
 BOOL found=FALSE;

 if(lexer_specialID(me, rBO, rUO, EVENT_OUT, EVENT_OUT_COUNT,
  user_eventOut))
  found=TRUE;
 if(rBE) *rBE=ID_UNDEFINED;
 if(rUE) *rUE=ID_UNDEFINED;

 /* XXX:  See lexer_eventIn */
 if(found)
  return TRUE;

 /* Exposed field with _changed suffix? */
 if(!lexer_setCurID(me))
  return found;
 assert(me->curID);
 {
  char* begId=me->curID;
  char* curId=begId+strlen(begId);
  const char* begSuf=EXPOSED_EVENT_OUT_SUF;
  const char* curSuf=begSuf+strlen(begSuf);

  while(curId>=begId && curSuf>=begSuf)
  {
   /* Mismatch? */
   if(*curId!=*curSuf)
    goto noExposedSuf;
   --curId;
   --curSuf;
  }
  
  /* If only id reached "end", it can't be suffixed by suf! */
  if(curSuf>=begSuf)
  {
   assert(curId<begId);
   goto noExposedSuf;
  }

  /* Otherwise, chop off the suffix!  At least temporarily... */
  assert(curId[1]==*begSuf);
  curId[1]=0;

  if(lexer_specialID_string(me, rBE, rUE,
   EXPOSED_FIELD, EXPOSED_FIELD_COUNT, NULL, begId))
  {
   FREE_IF_NZ (me->curID);
   found=TRUE;
  } else /* Wrong, revert the chop-off */
   curId[1]=*begSuf;
 }
noExposedSuf:

 /* Try without suffix if not found */
 if(!found)
 {
  if(lexer_specialID(me, rBE, rUE, EXPOSED_FIELD, EXPOSED_FIELD_COUNT,
   user_exposedField))
   found=TRUE;
 }

 return found;
}

/* Lexes a fieldId terminal symbol */
BOOL lexer_field(struct VRMLLexer* me,
 indexT* retBO, indexT* retBE, indexT* retUO, indexT* retUE)
{
 BOOL found=FALSE;

 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

 if(lexer_specialID_string(me, retBO, retUO,
  FIELD, FIELD_COUNT, user_field, me->curID))
  found=TRUE;
 if(lexer_specialID_string(me, retBE, retUE,
  EXPOSED_FIELD, EXPOSED_FIELD_COUNT, user_exposedField, me->curID))
  found=TRUE;

 if(found)
 {
  FREE_IF_NZ (me->curID);
 }

 return found;
}

/* Conversion of user field name to char* */
const char* lexer_stringUser_fieldName(indexT name, indexT mode)
{
 switch(mode)
 {
  case PKW_field:
   return lexer_stringUser_field(name);
  case PKW_exposedField:
   return lexer_stringUser_exposedField(name);
  case PKW_eventIn:
   return lexer_stringUser_eventIn(name);
  case PKW_eventOut:
   return lexer_stringUser_eventOut(name);
 }
 assert(FALSE);
}

/* Skip whitespace and comments. */
void lexer_skip(struct VRMLLexer* me)
{
 int c;

 if(me->curID) return;

 while(TRUE)
 {
  LEXER_GETINPUT(c)
  switch(c)
  {
   
   /* Whitespace:  Simply ignore. */
   case ' ':
   case '\n':
   case '\r':
   case '\t':
   case ',':
    break;

   /* Comment:  Ignore until end of line. */
   case '#':
    do
     LEXER_GETINPUT(c)
    while(c!='\n' && c!=EOF);
    break;

   /* Everything else:  Unget and return. */
   default:
    LEXER_UNGETINPUT(c)
    return;
   
  }
 }
}

/* Input the basic literals */
/* ************************ */

/* Processes sign for int32 and float */
/* FIXME:  Eats up "sign" for some wrong input (like -x) */
#define NUMBER_PROCESS_SIGN_GENERAL(addCheck) \
 { \
  neg=(c=='-'); \
  if(c=='-' || c=='+') \
  { \
   LEXER_GETINPUT(c) \
   if(!(c>='0' && c<='9') addCheck) \
   { \
    LEXER_UNGETINPUT(c) \
    return FALSE; \
   } \
  } \
 }
#define NUMBER_PROCESS_SIGN_INT \
 NUMBER_PROCESS_SIGN_GENERAL()
#define NUMBER_PROCESS_SIGN_FLOAT \
 NUMBER_PROCESS_SIGN_GENERAL(&& c!='.')

/* Changes sign of return value if neg is set and returns. */
#define RETURN_NUMBER_WITH_SIGN \
 { \
  if(neg) \
   *ret=-*ret; \
  return TRUE; \
 }

BOOL lexer_int32(struct VRMLLexer* me, vrmlInt32T* ret)
{
 int c;
 BOOL neg;
 
 if(me->curID) return FALSE;
 lexer_skip(me);

 /* Check if it is really a number */
 LEXER_GETINPUT(c)
 CHECK_EOF(c)
 if(c!='-' && c!='+' && !(c>='0' && c<='9'))
 {
  LEXER_UNGETINPUT(c)
  return FALSE;
 }

 /* Process sign. */
 NUMBER_PROCESS_SIGN_INT

 /* Initialize return value. */
 *ret=0;

 /* Hex constant?  Otherwise simply skip the useless 0 */
 if(c=='0')
 {
  LEXER_GETINPUT(c)
  if(c=='x')
  {
   while(TRUE)
   {
    LEXER_GETINPUT(c)
    *ret*=0x10;
    if(c>='0' && c<='9')
     *ret+=c-'0';
    else if(c>='A' && c<='F')
     *ret+=10+(c-'A');
    else if(c>='a' && c<='f')
     *ret+=10+(c-'a');
    else
     break;
   }
   LEXER_UNGETINPUT(c)
   assert(!(*ret%0x10));
   *ret/=0x10;

   RETURN_NUMBER_WITH_SIGN
  }
 }

 /* Main processing loop. */
 while(TRUE)
 {
  if(!(c>='0' && c<='9'))
   break;
  *ret*=10;
  *ret+=c-'0';

  LEXER_GETINPUT(c)
 }
 LEXER_UNGETINPUT(c)

 RETURN_NUMBER_WITH_SIGN
}

BOOL lexer_float(struct VRMLLexer* me, vrmlFloatT* ret)
{
 int c;

 BOOL neg;
 BOOL afterPoint;	/* Already after decimal point? */
 float decimalFact;	/* Factor next decimal digit is multiplied with */

 if(me->curID) return FALSE;
 lexer_skip(me);

 /* Really a float? */
 LEXER_GETINPUT(c)
 CHECK_EOF(c)
 if(c!='-' && c!='+' && c!='.' && !(c>='0' && c<='9'))
 {
  LEXER_UNGETINPUT(c)
  return FALSE;
 }

 /* Process sign. */
 NUMBER_PROCESS_SIGN_FLOAT
 
 /* Main processing loop. */
 *ret=0;
 afterPoint=FALSE;
 decimalFact=.1;
 while(TRUE)
 {
  if(c=='.' && !afterPoint)
   afterPoint=TRUE;
  else if(c>='0' && c<='9')
   if(afterPoint)
   {
    *ret+=decimalFact*(c-'0');
    decimalFact/=10;
   } else
   {
    *ret*=10;
    *ret+=c-'0';
   }
  else
   break;

  LEXER_GETINPUT(c)
 }
 /* No unget, because c is needed later. */

 /* Exponential factor? */
 if(c=='e' || c=='E')
 {
  BOOL negExp;
  int exp;

  LEXER_GETINPUT(c)
  negExp=(c=='-');
  /* FIXME:  Wrong for things like 1e-. */
  if(c=='-' || c=='+')
   LEXER_GETINPUT(c)

  exp=0;
  while(TRUE)
  {
   if(c>='0' && c<='9')
   {
    exp*=10;
    exp+=c-'0';
   } else
    break;

   LEXER_GETINPUT(c)
  }

  if(negExp)
   exp=-exp;
  *ret*=pow(10, exp);
 }
 LEXER_UNGETINPUT(c)

 RETURN_NUMBER_WITH_SIGN
}

BOOL lexer_string(struct VRMLLexer* me, vrmlStringT* ret)
{
 int c;
 char* buf;
 size_t bufLen=INITIAL_STRINGLEN;
 size_t cur=0;

 if(me->curID) return FALSE;
 lexer_skip(me);

 /* Really a string? */
 LEXER_GETINPUT(c)
 CHECK_EOF(c)
 if(c!='\"')
 {
  LEXER_UNGETINPUT(c)
  return FALSE;
 }

 /* Set up buffer */
 buf=MALLOC(sizeof(*buf)*bufLen);
 assert(buf);

 /* Main processing loop */
 while(TRUE)
 {
  /* Buffer resize needed?  Extra space for closing 0. */
  if(cur+1==bufLen)
  {
   bufLen*=2;
   buf=REALLOC(buf, sizeof(*buf)*bufLen);
  }
  assert(cur+1<bufLen);

  LEXER_GETINPUT(c)
  switch(c)
  {

   /* End of string */
   case EOF:
    parseError("String literal not closed at all!");
   case '\"':
    goto breakStringLoop;

   /* Copy character */
   case '\\':
    LEXER_GETINPUT(c)
    if(c==EOF)
    {
     parseError("String literal not closed at all!");
     goto breakStringLoop;
    }
   default:
    buf[cur]=(char)c;
    ++cur;

  }
 }
breakStringLoop:
 /* No unget, because c is closing quote */
 assert(cur<bufLen);
 buf[cur]=0;

 *ret=newASCIIString(buf);
 FREE_IF_NZ (buf);
 return TRUE;
}

/* Operator check */
/* ************** */

BOOL lexer_operator(struct VRMLLexer* me, char op)
{
 int c;

 if(me->curID) return FALSE;
 lexer_skip(me);

 LEXER_GETINPUT(c)
 CHECK_EOF(c)
 if(c!=op)
 {
  LEXER_UNGETINPUT(c)
  return FALSE;
 }

 return TRUE;
}
