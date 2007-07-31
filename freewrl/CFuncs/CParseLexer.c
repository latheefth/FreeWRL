/* Sourcecode for CParseLexer.h */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h> /* Using pow() to parse floats */

#include "CParseLexer.h"

void lexer_handle_EXTERNPROTO(struct VRMLLexer *me);
char *externProtoPointer = NULL;

/* Pre- and suffix for exposed events. */
const char* EXPOSED_EVENT_IN_PRE="set_";
const char* EXPOSED_EVENT_OUT_SUF="_changed";

/* Tables of user-defined IDs */
#define USER_IDS_INIT_SIZE	16
/* DEFed node names stack */
Stack* userNodeNames=NULL;
/* PROTO names stack */
Stack* userNodeTypesStack=NULL;
/* PROTO names list */
/* FIXME - should this be a global type? */
struct Vector* userNodeTypesVec=NULL;
/* User defined fields */
struct Vector* user_field=NULL;
/* User defined exposed fields */
struct Vector* user_exposedField=NULL;
/* User defined eventIns */
struct Vector* user_eventIn=NULL;
/* User defined eventOuts */
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
 /* printf("pushed on value 0 to userNodeTypesStack\n"); */
 userNodeTypesVec=newVector(char*, USER_IDS_INIT_SIZE);
 user_field=newVector(char*, USER_IDS_INIT_SIZE);
 user_exposedField=newVector(char*, USER_IDS_INIT_SIZE);
 user_eventIn=newVector(char*, USER_IDS_INIT_SIZE);
 user_eventOut=newVector(char*, USER_IDS_INIT_SIZE);
 lexer_scopeIn();

 /* printf("new lexer created, userNodeTypesVec is %p, user_field is %p, user_exposedField is %p, user_eventIn is %p, user_eventOut is %p\n", userNodeTypesVec, user_field, user_exposedField, user_eventIn, user_eventOut); */

 return ret;
}

void deleteLexer(struct VRMLLexer* me)
{
 FREE_IF_NZ (me->curID);
 FREE_IF_NZ (me);
 FREE_IF_NZ (externProtoPointer);
}

static void lexer_scopeOut_(Stack*);
void lexer_destroyIdStack(Stack* s)
{
 assert(s);
 while(!stack_empty(s))
  lexer_scopeOut_(s);
 deleteStack(struct Vector*, s);
 s = NULL; /* JAS */
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
 if(userNodeTypesStack) {
	/* printf("deleting userNodeTypesStack\n"); */
  	deleteStack(size_t, userNodeTypesStack);
	userNodeTypesStack = NULL; /* JAS */
 }

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

/* How many PROTOs were defined before we reached this scope?  Remember that number by pushing
   it onto the userNodeTypesStack */
void lexer_scopeIn()
{
 lexer_scopeIn_(&userNodeNames);
  /* printf("push value %d onto userNodeTypesStack\n", vector_size(userNodeTypesVec)); */
 stack_push(size_t, userNodeTypesStack, vector_size(userNodeTypesVec));
 /* Fields aren't scoped because they need to be accessible in two levels */
}
void lexer_scopeOut()
{
 lexer_scopeOut_(userNodeNames);
 /* PROTO out-scoping is done by parser */
 /* lexer_scopeOut_PROTO(); */
 /* Fields aren't scoped because they need to be accessible in two levels */
}

/* stack_top(size_t, userNodeTypesStack) returns the number of PROTOs that were defined before
   we reached the local scope.  To scope out any added names, we take off names added to the vector
   userNodeTypesVec since the local scope started.  i.e. we keep removing the newest PROTO name 
   from userNodeTypesVec until the size of this vector is the same as the number popped off of the top of
   the userNodeTypesStack */
void lexer_scopeOut_PROTO()
{
 /* printf("lexer_scopeOut_PROTO: while %d is > than %d\n", vector_size(userNodeTypesVec), stack_top(size_t, userNodeTypesStack)); */
 while(vector_size(userNodeTypesVec)>stack_top(size_t, userNodeTypesStack))
 {
  /* Free the last element added to the vector */
  FREE_IF_NZ (vector_back(char*, userNodeTypesVec));
  /* Decrement the number of items in the vector */
  vector_popBack(char*, userNodeTypesVec);
 }
 /* Take off the top value of userNodeTypesStack */
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

 /* is this an EXTERNPROTO? if so, handle it here */
 if (lexer_keyword(me,KW_EXTERNPROTO))
	lexer_handle_EXTERNPROTO(me);

 /* JAS printf ("lexer_setCurID, got %s\n",me->curID); */
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
/* Basically, just calls lexer_specialID_string with the same args plus the current token */
/* Checks for an ID (the next lexer token) in the builtin array of IDs passed in builtin and/or in the array of user defined
   IDs passed in user.  Returns the index to the ID in retB (if found in the built in list) or retU (if found in the 
   user defined list) if it is found.  */
BOOL lexer_specialID(struct VRMLLexer* me, indexT* retB, indexT* retU,
 const char** builtIn, const indexT builtInCount, struct Vector* user)
{
 /* Get the next token */
 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

  /* printf("lexer_specialID looking for %s\n", me->curID); */

 if(lexer_specialID_string(me, retB, retU, builtIn, builtInCount, user,
  me->curID))
 {
  FREE_IF_NZ (me->curID);
  return TRUE;
 }

 return FALSE;
}

/* Checks for the ID passed in str in the builtin array of IDs passed in builtin and/or in the array of user defined
   IDs passed in user.  Returns the index to the ID in retB (if found in the built in list) or retU (if found in the 
   user defined list) if it is found.  */
BOOL lexer_specialID_string(struct VRMLLexer* me, indexT* retB, indexT* retU,
 const char** builtIn, const indexT builtInCount,
 struct Vector* user, const char* str)
{
 indexT i;
 BOOL found=FALSE;

 /* Have to be looking in either the builtin and/or the user defined lists */
 if(!retB && !retU)
  return FALSE;

 if(retB) *retB=ID_UNDEFINED;
 if(retU) *retU=ID_UNDEFINED;

 /* Try as built-in */
 /* Look for the ID in the passed built in array.  If it is found, return the index to the ID in retB */
 for(i=0; i!=builtInCount; ++i)
  if(!strcmp(str, builtIn[i]))
  {
   /* printf("found ID %s matches %s, return retB %d\n", str, builtIn[i], i); */
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
 /* Look for the ID in the passed user array.  If it is found, return the index to the ID in retU */
 for(i=0; i!=vector_size(user); ++i)
  if(!strcmp(str, vector_get(char*, user, i)))
  {
   /* printf("found ID %s matches %s, return retU %d\n", str, vector_get(char*, user, i), i); */
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
/* Adds the ID to the passed vector of IDs (unless it is already present) */
/* Note that we only check for duplicate IDs if multi is TRUE */
BOOL lexer_defineID(struct VRMLLexer* me, indexT* ret, struct Vector* vec,
 BOOL multi)
{
 
 /* Get the next token */
 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

 /* User list should be created */
 assert(vec);

 /* If multiple definition possible? Look if the ID's already there */
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
 /* Add this ID to the passed vector of IDs */
 *ret=vector_size(vec);
 /* printf("lexer_defineID: adding %s to vector %p\n", me->curID, vec); */
 vector_pushBack(char*, vec, me->curID);
 me->curID=NULL;
 return TRUE;
}

/* A eventIn/eventOut terminal symbol */
/* Looks for the current token in builtin and/or user defined name arrays depending on the requested return values and the eventtype (in or out)
   If looking through EVENT_IN, EVENT_OUT, or EXPOSED_FIELD, checks to see if the current token is valid with either set_ or _changed stripped from it 
   If rBO is non-null, then search through EVENT_IN or EVENT_OUT and return the index of the event (if found) in rBO
   If rBE is non-null, then search through EXPOSED_FIELD and return the index of the event (if found) in rBE
   If rUO is non-null, then search through user_eventIn or user_eventOut and return the index of the event (if found) in rUO
   if rUE is non-null, then search through user_exposedField and return the index of the event (if found) in rUE */ 

BOOL lexer_event(struct VRMLLexer* me,
 struct X3D_Node* routedNode,
 indexT* rBO, indexT* rBE, indexT* rUO, indexT* rUE,
 int routedToFrom)
{
 BOOL found=FALSE;

 struct Vector* uarr;
 const char** arr;
 size_t arrCnt;

 if(routedToFrom==ROUTED_FIELD_EVENT_IN)
 {
  /* If we are looking for an eventIn we need to look through the EVENT_IN array and the user_eventIn vector */
  uarr=user_eventIn;
  arr=EVENT_IN;
  arrCnt=EVENT_IN_COUNT;
 } else
 {
  /* If we are looking for an eventOut we need to look through the EVENT_OUT array and the user_eventOut vector */
  uarr=user_eventOut;
  arr=EVENT_OUT;
  arrCnt=EVENT_OUT_COUNT;
 }

 /* Get the next token */
 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

  /* printf("Looking for %s\n", me->curID); */

 /* Get a pointer to the data in the vector of user defined event names */
 const char** userArr=&vector_get(const char*, uarr, 0);
 size_t userCnt=vector_size(uarr);

 /* Strip off set_ or _changed from current token.  Then look through the EVENT_IN/EVENT_OUT array for the eventname (current token).  
    If it is found, return the index of the eventname. Also looks through fields of the routedNode to check if fieldname is valid for that node 
    (but doesn't seem to do anything if not valid ... ) */
 if(rBO)
  *rBO=findRoutedFieldInARR(routedNode, me->curID, routedToFrom, arr, arrCnt,
   FALSE);

 /* Strip off set_ or _changed from current token.  Then look through the user_eventIn/user_eventOut array for the eventname (current token).  
    If it is found, return the index of the eventname.  */
 if(rUO)
  *rUO=findRoutedFieldInARR(routedNode, me->curID, routedToFrom,
   userArr, userCnt, TRUE);

 /* Set the found flag to TRUE if the eventname was found in either the EVENT_IN/EVENT_OUT or user_eventIn/user_eventOut arrays */ 
 if(!found)
  found=((rBO && *rBO!=ID_UNDEFINED) || (rUO && *rUO!=ID_UNDEFINED));

/* 
 if (rBO && *rBO != ID_UNDEFINED)
	printf("lexer_event: found in EVENT_IN/EVENT_OUT\n");

 if (rUO && *rUO != ID_UNDEFINED)
	printf("lexer_event: found in user_eventIn/user_eventOut\n");
*/

 /*printf("%p %u %d\n", rBO, *rBO, *rBO!=ID_UNDEFINED);*/

 /* Get a pointer to the event names in the vector of user defined exposed fields */
 userArr=&vector_get(const char*, user_exposedField, 0);
 userCnt=vector_size(user_exposedField);

 /* findRoutedFieldInEXPOSED_FIELD calls findRoutedFieldInARR(node, field, fromTo, EXPOSED_FIELD, EXPOSED_FIELD_COUNT, 0) */
 /* Strip off set_ or _changed from current token.  Then look through the EXPOSED_FIELD array for the eventname (current token). 
    If it is found, return the index of the eventname.  Also looks through fields of the routedNode to check if fieldname is valid for that node
    (but doesn't seem to do anything if not valid ... ) */ 
 if(rBE)
  *rBE=findRoutedFieldInEXPOSED_FIELD(routedNode, me->curID, routedToFrom);

 /* Strip off set_ or _changed from current token.  Then look through the user_exposedField array for the eventname (current token). 
    If it is found, return the index of the eventname.  */ 
 if(rUE)
  *rUE=findRoutedFieldInARR(routedNode, me->curID, routedToFrom,
   userArr, userCnt, TRUE);

 /* Set the found flag to TRUE if the eventname was found in either the EXPOSED_FIELD or user_exposedField arrays */ 
 if(!found)
  found=((rBE && *rBE!=ID_UNDEFINED) || (rUE && *rUE!=ID_UNDEFINED));

/* 
 if (rBE && *rBE != ID_UNDEFINED)
	printf("lexer_event: found in EXPOSED_FIELD\n");

 if (rUE && *rUE != ID_UNDEFINED)
	printf("lexer_event: found in user_exposedField\n");
*/

 if(found)
  FREE_IF_NZ(me->curID)

  /* printf("lexer_event: found is now %d\n", found); */

 return found;
}

/* Lexes a fieldId terminal symbol */
/* If retBO isn't null, checks for the field in the FIELDNAMES array */
/* If retBE isn't null, checks for the field in the EXPOSED_FIELD array */
/* if retUO isn't null, checks for the field in the user_field vector */
/* if retUE isn't null, checks for the field in the user_exposedField vector */
/* returns the index of the field in the corresponding ret value if found */
BOOL lexer_field(struct VRMLLexer* me,
 indexT* retBO, indexT* retBE, indexT* retUO, indexT* retUE)
{
 BOOL found=FALSE;

  /* Get next token */
 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

  /* Get a pointer to the entries in the user_field vector */
 const char** userArr=&vector_get(const char*, user_field, 0);
 size_t userCnt=vector_size(user_field);

 /* printf("looking for %s\n", me->curID); */
 /* findFieldInFIELD is #defined to findFieldInARR(field, FIELDNAMES, FIELDNAMES_COUNT) */
 /* look through the FIELDNAMES array for the fieldname (current token).  If it is found, return the index of the fieldname */
 if(retBO)
  *retBO=findFieldInFIELD(me->curID);

 /* look through the fieldnames from the user_field names vector for the fieldname (current token).  If it is found, return the index 
   of the fieldname */
 if(retUO)
  *retUO=findFieldInARR(me->curID, userArr, userCnt);

  /* Set the found flag to TRUE if the fieldname was found in either FIELDNAMES or user_field */
 if(!found)
  found=((retBO && *retBO!=ID_UNDEFINED) || (retUO && *retUO!=ID_UNDEFINED));

  /* Get a pointer to the entries in the user_exposedField vector */
 userArr=&vector_get(const char*, user_exposedField, 0);
 userCnt=vector_size(user_exposedField);
  
  /* findFieldInEXPOSED_FIELD #defined to findFieldInARR(field, EXPOSED_FIELD, EXPOSED_FIELD_COUNT) */
  /* look through the EXPOSED_FIELD array for the fieldname (current token).  If it is found, return the index of the fieldname.  */
 if(retBE)
  *retBE=findFieldInEXPOSED_FIELD(me->curID);

 /* look through the fieldnames from the user_exposedField names vector for the fieldname (current token).  If it is found, return the
    index of the fieldname */
 if(retUE)
  *retUE=findFieldInARR(me->curID, userArr, userCnt);

 /* Set the found flag to TRUE if the fieldname was found in either EXPOSED_FIELD or user_exposedField */
 if(!found)
  found=((retBE && *retBE!=ID_UNDEFINED) || (retUE && *retUE!=ID_UNDEFINED));

/* 
 if (retBO && *retBO != ID_UNDEFINED) 
   printf("lexer_field: found field in FIELDNAMES\n");
 if (retUO && *retUO != ID_UNDEFINED) 
   printf("lexer_field: found field in user_field\n");
 if (retBE && *retBE != ID_UNDEFINED) 
   printf("lexer_field: found field in EXPOSED_FIELD\n");
 if (retUE && *retUE != ID_UNDEFINED) 
   printf("lexer_field: found field in user_exposedField\n");
*/

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
    do {
     LEXER_GETINPUT(c)
	/* printf ("lexer, found comment, current char %d:%c:\n",c,c); */
	/* for those files created by ith VRML97 plugin for LightWave3D v6 from NewTek, Inc
	   we have added the \r check. JAS */
    } while(c!='\n' && c!= '\r' && c!=EOF);
	
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
	/* JAS - I hate doing this, but Lightwave exporter SOMETIMES seems to put double dots
	   in a VRML file. This catches them...  see
	   http://neptune.gsfc.nasa.gov/osb/aquarius/animations/vrml.php */
  else if (c=='.') {
	/*printf ("double dots\n");*/
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

/* EXTERNPROTO HANDLING */
/************************/
#define PARSE_ERROR(msg) \
 { \
  parseError(msg); \
  return; \
 }

#define parser_sfstringValue(me, ret) \
 lexer_string(me, ret)

#define FIND_PROTO_IN_proto_BUFFER \
		do { \
			proto = strstr (proto,"PROTO"); \
			if (proto == NULL) \
				PARSE_ERROR ("EXTERNPROTO does not contain a PROTO!"); \
			if (*(proto-1) != 'N') { \
				break; \
			} \
		} while (1==1);

/* the following is cribbed from CParseParser for MFStrings, but we pass a VRMLLexer, not a VRMLParser */
int lexer_EXTERNPROTO_mfstringValue(struct VRMLLexer* me, struct Multi_String* ret) {
	struct Vector* vec=NULL;
	char fw_outline[2000];

	/* Just a single value? */
	if(!lexer_openSquare(me)) {
		ret->p=MALLOC(sizeof(vrmlStringT));
		if(!parser_sfstringValue(me, (void*)ret->p))
			return FALSE;
		ret->n=1;
		return TRUE;
	}

	/* Otherwise, a real vector */
	vec=newVector(vrmlStringT, 128);
	while(!lexer_closeSquare(me)) {
		vrmlStringT val;
		if(!parser_sfstringValue (me, &val)) {
			/* parseError("Expected ] before end of MF-Value!"); */
			strcpy (fw_outline,"ERROR:Expected \"]\" before end of EXTERNPROTO URL value, found \"");
			if (me->curID != ((void *)0))
				strcat (fw_outline, me->curID);
			else
				strcat (fw_outline, "(EOF)");
			strcat (fw_outline,"\" ");
			ConsoleMessage(fw_outline);
			fprintf (stderr,"%s\n",fw_outline);
			break;
		}

		vector_pushBack(vrmlStringT, vec, val);
	}

	ret->n=vector_size(vec);
	ret->p=vector_releaseData(vrmlStringT, vec);

	deleteVector(vrmlStringT, vec);
	return TRUE;
}

/* isolate the PROTO that we want from the just read in EXTERNPROTO string */
void embedEXTERNPROTO(struct VRMLLexer *me, char *myName, char *buffer, char *pound) {
	char *cp;
	char *externProto;
	char *proto;
	int curlscount;
	int foundBracket;
	int str1len, str2len;

	/* step 1. Remove comments, so that we do not locate the requested PROTO in comments. */
	cp = buffer;

	while (*cp != '\0') {
		if (*cp == '#') {
			do {
				*cp = ' ';
				cp++;
				/* printf ("lexer, found comment, current char %d:%c:\n",c,c); */
				/* for those files created by ith VRML97 plugin for LightWave3D v6 from NewTek, Inc
				   we have added the \r check. JAS */
			} while((*cp!='\n') && (*cp!= '\r') && (*cp!='\0'));
		} else {
			cp++;
		}
	}

	/* find the requested name, or find the first PROTO here */
	if (pound != NULL) {
		pound++;
		/* printf ("looking for ID %s\n",pound); */
		proto=buffer;

		do {
			FIND_PROTO_IN_proto_BUFFER

			/* is this the PROTO we are looking for? */
			proto += sizeof ("PROTO");
			while ((*proto <= ' ') && (*proto != '\0')) proto++;
			/* printf ("found PROTO at %s\n",proto); */
		} while (strncmp(pound,proto,sizeof(pound)) != 0);
	} else {
		/* no name requested; find the first PROTO that is not an EXTERNPROTO */
		proto = buffer;
		FIND_PROTO_IN_proto_BUFFER
		/* printf ("found PROTO at %s\n",proto); */
	}

	/* go to the first '[' of the proto */
	cp = strchr(proto,'[');
	if (cp != NULL) proto = cp;

	/* now, isolate this PROTO from the rest ... count the curly braces */
	cp = proto;
	curlscount = 0;
	foundBracket = FALSE;
	do {
		if (*cp == '{') {curlscount++; foundBracket = TRUE;}
		if (*cp == '}') curlscount--;
		cp++;
		if (*cp == '\0')
			PARSE_ERROR ("brackets missing in EXTERNPROTO");

	} while (!foundBracket || (curlscount > 0));
	*cp = '\0';

	/* now, insert this PROTO text INTO the stream */
	cp = externProtoPointer; /* keep a handle on this */

	externProtoPointer = MALLOC (sizeof (char) * (strlen (me->nextIn)+strlen (proto)+strlen(myName) +4));
	strcpy (externProtoPointer,myName);
	strcat (externProtoPointer," ");
	strcat (externProtoPointer,proto);
	strcat (externProtoPointer,me->nextIn);
	lexer_fromString(me,externProtoPointer);

	FREE_IF_NZ(cp); /* free, now that we have copied all we need from (possibly) this */


}

/* the curID is EXTERNPROTO. Replace the EXTERNPROTO with the actual PROTO string read in from
   an external file */

lexer_handle_EXTERNPROTO(struct VRMLLexer *me) {
	char *myName = NULL;
	indexT mode;
	indexT type;
	struct Multi_String url;
	int i;
	char *pound;
	char *savedCurInputURL;
	char *buffer;
	char emptyString[100];
	char *testname;

	testname = (char *)MALLOC (1000);

	/* expect the EXTERNPROTO proto name */
	if (lexer_setCurID(me)) {
		/* printf ("next token is %s\n",me->curID); */
		myName = STRDUP(me->curID);
		FREE_IF_NZ(me->curID);
	} else {
		PARSE_ERROR ("EXTERNPROTO - expected a PROTO name\n");
	}

	/* go through and save the parameters and types. */

	if (!lexer_openSquare(me))
		PARSE_ERROR ("EXTERNPROTO - expected a '['");


	/* XXX - we should save these mode/type/name pairs, and compare them to the
	  ones in the EXTERNPROTO definition. But, for now, we don't */

	/* get the Name/Type value pairs and save them */
	while (lexer_protoFieldMode(me, &mode)) {
		/* printf ("mode is %d\n",mode); */

		if(!lexer_fieldType(me, &type))
			PARSE_ERROR("Expected fieldType after proto-field keyword!")

		/* printf ("type is %d\n",type); */


		if (lexer_setCurID(me)) {
			/* printf ("param name is %s\n",me->curID); */
			FREE_IF_NZ(me->curID);
		} else {
			PARSE_ERROR ("EXTERNPROTO - expected a PROTO name\n");
		}
	}

	/* now, check for closed square */
	if (!lexer_closeSquare(me))
		PARSE_ERROR ("EXTERNPROTO - expected a ']'");

	/* get the URL string */
	if (!lexer_EXTERNPROTO_mfstringValue(me,&url)) {
		PARSE_ERROR ("EXTERNPROTO - problem reading URL string");
	}

	for (i=0; i< url.n; i++) {
		/* printf ("trying url %s\n",(url.p[i])->strptr); */
		pound = strchr((url.p[i])->strptr,'#');
		if (pound != NULL) {
			/* we take the pound character off, BUT USE this variable later */
			*pound = '\0';
		}


		if (getValidFileFromUrl (testname ,getInputURL(), &url, emptyString)) {


			buffer = readInputString(testname,"");
			FREE_IF_NZ(testname)
			embedEXTERNPROTO(me,myName,buffer,pound);

			/* ok - we are replacing EXTERNPROTO with PROTO */
			me->curID = MALLOC (sizeof(char)*20);
			strcpy(me->curID,"PROTO");
			return;
		} else {
			/* printf ("fileExists returns failure for %s\n",testname); */
		}

	}

	FREE_IF_NZ(testname)

	/* print up an error message, then get the next token */
	strcpy (emptyString, "Not Successful at getting EXTERNPROTO \"");
	if (strlen(myName) > 100) myName[100] = '\0';
	strcat (emptyString,myName);
	strcat (emptyString,"\"");
	ConsoleMessage("Parse error: %s ", emptyString); fprintf(stderr, "%s\n",emptyString);

	/* so, lets continue. Maybe this EXTERNPROTO is never referenced?? */
	lexer_setCurID(me);
	/* printf ("so, curID is :%s: and rest is :%s:\n",me->curID, me->nextIn); */
	return;
}
