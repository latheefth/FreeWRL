/* Sourcecode for CParseLexer.h */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h> /* Using pow() to parse floats */

#include "CParseLexer.h"

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
  --me->nextIn; \
 }

/* Constructor and destructor */

struct VRMLLexer* newLexer()
{
 struct VRMLLexer* ret=malloc(sizeof(struct VRMLLexer));

 ret->nextIn=NULL;
 ret->curID=NULL;

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
BOOL lexer_specialID(struct VRMLLexer* me, indexT* ret,
 const char** builtIn, const indexT builtInCount,
 struct Vector** user)
{
 indexT i;
 
 if(!lexer_setCurID(me)) return FALSE;
 assert(me->curID);

 /* Try as built-in */
 for(i=0; i!=builtInCount; ++i)
  if(!strcmp(me->curID, builtIn[i]))
  {
   *ret=i;
   free(me->curID);
   me->curID=NULL;
   return TRUE;
  }

 /* Return if no user list is requested. */
 if(!user)
  return FALSE;
 
 /* Initialize user list, if it is not yet created. */
 if(!*user)
  *user=newVector(char*, USER_IDS_INIT_SIZE);
 assert(*user);

 /* Already defined user id? */
 for(i=0; i!=vector_size(*user); ++i)
  if(!strcmp(me->curID, vector_get(char*, *user, i)))
  {
   *ret=i|USER_ID;
   free(me->curID);
   me->curID=NULL;
   return TRUE;
  }

 /* No, just defined it. */
 vector_pushBack(char*, *user, me->curID);
 me->curID=NULL;
 *ret=(vector_size(*user)-1)|USER_ID;
 return TRUE;
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
 if(c!=op)
 {
  LEXER_UNGETINPUT(c)
  return FALSE;
 }

 return TRUE;
}
