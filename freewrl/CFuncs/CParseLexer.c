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
struct Vector* userNodeNames=NULL;

/* Maximum string/id length (input buffer size) */
#define MAX_STRINGLEN	1023
#define MAX_IDLEN	127

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
 struct VRMLLexer* ret=malloc(sizeof(struct VRMLLexer));

 ret->nextIn=NULL;
 ret->curID=NULL;
 ret->isEof=TRUE;

 return ret;
}

void deleteLexer(struct VRMLLexer* me)
{
 if(me->curID)
  free(me->curID);
 free(me);
}

/* Sets curID of lexer */
/* FIXME:  ID not as in VRML grammar! */
#define IS_ID_FIRST(c) \
 (c=='_' || (c>='a' && c<='z') || (c>='A' && c<='Z'))
#define IS_ID_REST(c) \
 (IS_ID_FIRST(c) || (c>='0' && c<='9'))
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
 me->curID=malloc(sizeof(char)*(cur-buf+1));

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
  free(me->curID);
  me->curID=NULL;
  return TRUE;
 }

 return FALSE;
}

/* Lexes an ID (node type, field name...) depending on args. */
BOOL lexer_specialID(struct VRMLLexer* me, indexT* retB, indexT* retU,
 const char** builtIn, const indexT builtInCount,
 struct Vector** user)
{
 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

 if(lexer_specialID_string(me, retB, retU, builtIn, builtInCount, user,
  me->curID))
 {
  free(me->curID);
  me->curID=NULL;
  return TRUE;
 }

 return FALSE;
}
BOOL lexer_specialID_string(struct VRMLLexer* me, indexT* retB, indexT* retU,
 const char** builtIn, const indexT builtInCount,
 struct Vector** user, const char* str)
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
 if(!user || !*user)
  return found;

 /* Already defined user id? */
 for(i=0; i!=vector_size(*user); ++i)
  if(!strcmp(str, vector_get(char*, *user, i)))
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
BOOL lexer_defineID(struct VRMLLexer* me, indexT* ret, struct Vector** vec)
{
 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

 /* Initialize user list, if it is not yet created. */
 assert(vec);
 if(!*vec)
  *vec=newVector(char*, USER_IDS_INIT_SIZE);
 assert(*vec);

 /* Define the id */
 *ret=vector_size(*vec);
 vector_pushBack(char*, *vec, me->curID);
 me->curID=NULL;
 return TRUE;
}

/* A eventIn terminal symbol */
BOOL lexer_eventIn(struct VRMLLexer* me,
 indexT* rBO, indexT* rBE, indexT* rUO, indexT* rUE)
{
 BOOL found=FALSE;

 if(lexer_specialID(me, rBO, rUO, EVENT_IN, EVENT_IN_COUNT, NULL))
  found=TRUE;
 if(rBE) *rBE=ID_UNDEFINED;
 if(rUE) *rUE=ID_UNDEFINED;
 
 /* Exposed field with set_ prefix? */
 if(!lexer_setCurID(me))
  return FALSE;
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
   free(me->curID);
   me->curID=NULL;
   found=TRUE;
  }
 }
noExposedPre:
 
 return found;
}

/* A eventOut terminal symbol */
BOOL lexer_eventOut(struct VRMLLexer* me,
 indexT* rBO, indexT* rBE, indexT* rUO, indexT* rUE)
{
 BOOL found=FALSE;

 if(lexer_specialID(me, rBO, rUO, EVENT_OUT, EVENT_OUT_COUNT, NULL))
  found=TRUE;
 if(rBE) *rBE=ID_UNDEFINED;
 if(rUE) *rUE=ID_UNDEFINED;

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
   free(me->curID);
   me->curID=NULL;
   found=TRUE;
  } else /* Wrong, revert the chop-off */
   curId[1]=*begSuf;
 }
noExposedSuf:

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
  FIELD, FIELD_COUNT, NULL, me->curID))
  found=TRUE;
 if(lexer_specialID_string(me, retBE, retUE,
  EXPOSED_FIELD, EXPOSED_FIELD_COUNT, NULL, me->curID))
  found=TRUE;

 if(found)
 {
  free(me->curID);
  me->curID=NULL;
 }

 return found;
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
 char buf[MAX_STRINGLEN+1];
 char* cur=buf;

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

 /* Main processing loop */
 while(cur!=buf+MAX_STRINGLEN)
 {
  assert(cur<buf+MAX_STRINGLEN);
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
    *cur=(char)c;
    ++cur;

  }
 }
 parseError("String buffer length hit!");
breakStringLoop:
 /* No unget, because c is closing quote */
 assert(cur<=buf+MAX_STRINGLEN);
 *cur=0;

 *ret=EAI_newSVpv(buf);
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
