/*


???

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>
#include <io_files.h>


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/fieldSet.h"
#include "../input/InputFunctions.h"
#include "../input/EAIHelpers.h"	/* for newASCIIString() */
#include "CParseParser.h"
#include "CParseLexer.h"
#include "CParse.h"

void lexer_handle_EXTERNPROTO(struct VRMLLexer *me);
//char *externProtoPointer = NULL; //not used
/* Pre- and suffix for exposed events. */
const char* EXPOSED_EVENT_IN_PRE="set_";
const char* EXPOSED_EVENT_OUT_SUF="_changed";

/* Tables of user-defined IDs */
#define USER_IDS_INIT_SIZE	16

/* Maximum id length (input buffer size) */
#define MAX_IDLEN	127
/* Start buffer length for strings */
#define INITIAL_STRINGLEN	256

/* Input data */
static int setLexerNextIn(struct VRMLLexer *);

#define LEXER_GETINPUT(c) \
 { \
  ASSERT(!me->curID); \
  if(!*me->nextIn) c=setLexerNextIn(me); \
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
 int i;

 struct VRMLLexer* ret=MALLOC(struct VRMLLexer *, sizeof(struct VRMLLexer));

 ret->nextIn=NULL;

 for (i=0; i<LEXER_INPUT_STACK_MAX; i++) ret->startOfStringPtr[i] = NULL;

 
 ret->curID=NULL;
 ret->isEof=TRUE;
 ret->lexerInputLevel = -1;
 
 /* Init id tables */
 ret->userNodeNames=newStack(struct Vector*);
 ret->userNodeTypesStack=newStack(int);
 stack_push(int, ret->userNodeTypesStack, 0);
 ret->userNodeTypesVec=newVector(char*, USER_IDS_INIT_SIZE);
 ret->user_initializeOnly=newVector(char*, USER_IDS_INIT_SIZE);
 ret->user_inputOutput=newVector(char*, USER_IDS_INIT_SIZE);
 ret->user_inputOnly=newVector(char*, USER_IDS_INIT_SIZE);
 ret->user_outputOnly=newVector(char*, USER_IDS_INIT_SIZE);
 lexer_scopeIn(ret);

#ifdef CPARSERVERBOSE
 printf("new lexer created, userNodeTypesVec is %p, user_initializeOnly is %p, user_inputOutput is %p, user_inputOnly is %p, user_outputOnly is %p\n", ret->userNodeTypesVec, ret->user_initializeOnly, ret->user_inputOutput, ret->user_inputOnly, ret->user_outputOnly); 
#endif 

 return ret;
}

void deleteLexer(struct VRMLLexer* me)
{
	#ifdef CPARSERVERBOSE
	printf ("deleteLexer called; deleting lexer %x %u\n",me,me);
	#endif
	 FREE_IF_NZ (me->curID);
 	FREE_IF_NZ (me);
 	//FREE_IF_NZ (p->externProtoPointer);
}


/* end of file on this "stack level" for lexer input */
static int setLexerNextIn(struct VRMLLexer *me) {
	int retval = EOF;

	#ifdef CPARSERVERBOSE
	printf ("set lexerNextIn called \n"); 
	#endif

	if (me->lexerInputLevel > 0) {
		#ifdef CPARSERVERBOSE
		printf ("setlexerNextIn, decrementing; deleting %x, setting nextIn to %x\n", me->startOfStringPtr[me->lexerInputLevel], me->oldNextIn[me->lexerInputLevel]); 
		#endif

		//FREE_IF_NZ (me->startOfStringPtr[me->lexerInputLevel]);
		if(me->startOfStringPtr[me->lexerInputLevel]){
			if(strlen(me->startOfStringPtr[me->lexerInputLevel])){
				FREE_IF_NZ(me->startOfStringPtr[me->lexerInputLevel]);
			}else{
				me->startOfStringPtr[me->lexerInputLevel] = NULL;
			}
		}
		me->nextIn = me->oldNextIn[me->lexerInputLevel];
		me->lexerInputLevel--;
		/* printf ("setlexerNextIn, level now is %d, and nextIn is :%s:\n",me->lexerInputLevel,me->nextIn); */
		retval = (int)*(me->nextIn++);
	}
	#ifdef CPARSERVERBOSE
	printf ("setLexerNextIn returning :%d:\n",retval);
	#endif

	return retval;
}


/* set the lexer to parse from the string. Pushes the current string onto a lexer stack */
void lexer_fromString (struct VRMLLexer *me, char *str) {
         if (str!= NULL) me->isEof=(str[0]=='\0'); 
	else (me)->isEof=TRUE; 

	me->lexerInputLevel ++;
	me->startOfStringPtr[me->lexerInputLevel]=str; 
	me->oldNextIn[me->lexerInputLevel] = me->nextIn; /* save the old "nextIn" for popping back */
	me->nextIn=str;
	#ifdef CPARSERVERBOSE
	printf ("lexer_fromString, me %x %u\n",me,me);
	printf ("lexer_fromString, working on level %d\n",me->lexerInputLevel);
	printf ("lexer_fromString, passedin char pointer is %x %u\n",str,str);
	printf ("lexer_fromString, saving nextin as %x\n",me->oldNextIn[me->lexerInputLevel]);
	printf ("lexer_fromString, input is :%s:\n",str);
	#endif
}


void lexer_forceStringCleanup (struct VRMLLexer *me) {
	int i;
	for (i=1; i<me->lexerInputLevel; i++) {
		FREE_IF_NZ(me->startOfStringPtr[i]);
		me->startOfStringPtr[i] = NULL;
	}
	#ifdef CPARSERVERBOSE
	printf ("lexer_forceStringCleanup, level was %d\n",me->lexerInputLevel);
	#endif

	me->lexerInputLevel = -1;
	me->nextIn = NULL;
}

static void lexer_scopeOut_(Stack*);
void lexer_destroyIdStack(Stack* s)
{
 ASSERT(s);
 while(!stack_empty(s))
  lexer_scopeOut_(s);
 deleteStack(struct Vector*, s);
 s = NULL; /* JAS */
}


#ifdef OLDCODE
void lexer_destroyIdVector(struct Vector* v)
OLDCODE{
OLDCODE int i;
OLDCODE if (v==NULL) {
OLDCODE	ConsoleMessage("lexer_destroyIdVector - vector already NULL");
OLDCODE	return;
OLDCODE }
OLDCODE
OLDCODE ASSERT(v);
OLDCODE for(i=0; i!=vectorSize(v); ++i) {
OLDCODE  FREE_IF_NZ (vector_get(char*, v, i));
OLDCODE }
OLDCODE
OLDCODE deleteVector(char*, v);
OLDCODE}
#endif //OLDCODE

void lexer_destroyData(struct VRMLLexer* me)
{
 #define DESTROY_IDVEC(v) \
  if(v) { int i; for(i=0; i<vectorSize(v); i++) {FREE_IF_NZ (vector_get(char*, v, i));} deleteVector(char*,v);} \
   /* lexer_destroyIdVector(v); */ \
  v=NULL /* might be redundant */ ;
  
 /* User node names */
 if(me->userNodeNames)
  lexer_destroyIdStack(me->userNodeNames);
 me->userNodeNames=NULL;

 /* User node types */
 DESTROY_IDVEC(me->userNodeTypesVec)
 if(me->userNodeTypesStack) {
  	deleteStack(int, me->userNodeTypesStack);
	me->userNodeTypesStack = NULL; /* JAS */
 }

 /* User fields */
 DESTROY_IDVEC(me->user_initializeOnly)
 DESTROY_IDVEC(me->user_inputOutput)
 DESTROY_IDVEC(me->user_inputOnly)
 DESTROY_IDVEC(me->user_outputOnly)
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
 int i;
 ASSERT(!stack_empty(s));

 for(i=0; i!=vectorSize(stack_top(struct Vector*, s)); ++i)
  FREE_IF_NZ (vector_get(char*, stack_top(struct Vector*, s), i));
 deleteVector(char*, stack_top(struct Vector*, s));
 stack_pop(struct Vector*, s);
}

/* Scope in PROTOs and DEFed nodes */
void lexer_scopeIn(struct VRMLLexer* me)
{
/* printf ("lexer_scopeIn, not doing push anymore \n"); */
 lexer_scopeIn_(&me->userNodeNames);
  /* printf("lexer_scopeIn: push value %d onto userNodeTypesStack\n", vectorSize(me->userNodeTypesVec)); */
 /* Remember the number of PROTOs that were defined when we first entered this scope.  This is the 
    number of PROTOs that must be defined when we leave this scope.  Keep this number on the userNodeTypesStack */
 stack_push(int, me->userNodeTypesStack, vectorSize(me->userNodeTypesVec));
 /* Fields aren't scoped because they need to be accessible in two levels */
}

/* Scope out PROTOs and DEFed nodes */
void lexer_scopeOut(struct VRMLLexer* me)
{
/* printf ("lexer_scopeOut, not doing push anymore \n"); */
 lexer_scopeOut_(me->userNodeNames);
 /* lexer_scopeOut_PROTO();  */
 /* Fields aren't scoped because they need to be accessible in two levels */
}

/* stack_top(int, me->userNodeTypesStack) returns the number of PROTOs that were defined before
   we reached the local scope.  To scope out any added names, we take off names added to the vector
   userNodeTypesVec since the local scope started.  i.e. we keep removing the newest PROTO name 
   from userNodeTypesVec until the size of this vector is the same as the number popped off of the top of
   the userNodeTypesStack.  Afterwards, pop off the top value of the userNodeTypesStack, to complete
   the scopeOut  */
void lexer_scopeOut_PROTO(struct VRMLLexer* me)
{
 /* printf("lexer_scopeOut_PROTO: userNodeTypesVec has %d PROTO IDs top of userNodeTypesStack is %d\n", vectorSize(me->userNodeTypesVec), stack_top(int, userNodeTypesStack)); */
 while(vectorSize(me->userNodeTypesVec)>stack_top(int, me->userNodeTypesStack))
 {
  /* Free the last element added to the vector */
  FREE_IF_NZ (vector_back(char*, me->userNodeTypesVec));
  /* Decrement the number of items in the vector */
  /* printf("	popping item off of userNodeTypesVec\n"); */
  vector_popBack(char*, me->userNodeTypesVec);
 }
 /* Take off the top value of userNodeTypesStack */
 /* printf("	popped items off of userNodeTypesVec, now take top item off of userNodeTypesStack\n"); */
 stack_pop(int, me->userNodeTypesStack);
}

/* Sets curID of lexer */
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
  ASSERT(cur<buf+MAX_IDLEN);
  *cur=c;
  ++cur;
  
  LEXER_GETINPUT(c)
  if(!IS_ID_REST(c))
   goto breakIdLoop;
 }
 parseError("ID buffer length hit!");
breakIdLoop:
 LEXER_UNGETINPUT(c)
 ASSERT(cur<=buf+MAX_IDLEN);
 *cur=0;

 ASSERT(strlen(buf)==(cur-buf));
 ASSERT(!me->curID);
 me->curID=MALLOC(char *, sizeof(char)*(cur-buf+1));

 strcpy(me->curID, buf);

 /* is this an EXTERNPROTO? if so, handle it here */
 if(!usingBrotos())
 if (lexer_keyword(me,KW_EXTERNPROTO) )
        lexer_handle_EXTERNPROTO(me);

 #ifdef CPARSERVERBOSE
 printf ("lexer_setCurID, got %s\n",me->curID); 
 #endif

 return TRUE;
}

/* Lexes a keyword */
BOOL lexer_keyword(struct VRMLLexer* me, int kw) {
	if(!lexer_setCurID(me)) return FALSE;
	ASSERT(me->curID);

	if(!strcmp(me->curID, KEYWORDS[kw])) {
		FREE_IF_NZ (me->curID);
		return TRUE;
	}
	return FALSE;
}

/* Finds the index of a given string */
int lexer_string2id(const char* str, const struct Vector* v)
{

/* printf ("lexer_string2id looking for %s vector %u\n",str,v); */
 int i;
 for(i=0; i!=vectorSize(v); ++i) {
	/* printf ("lexer_string2id, comparing %s to %s\n",str,vector_get(const char*, v, i)); */
  if(!strcmp(str, vector_get(const char*, v, i)))
   return i;
}
 return ID_UNDEFINED;
}

/* Lexes an ID (node type, field name...) depending on args. */
/* Basically, just calls lexer_specialID_string with the same args plus the current token */
/* Checks for an ID (the next lexer token) in the builtin array of IDs passed in builtin and/or in the array of user defined
   IDs passed in user.  Returns the index to the ID in retB (if found in the built in list) or retU (if found in the 
   user defined list) if it is found.  */

BOOL lexer_specialID(struct VRMLLexer* me, int* retB, int* retU,
 const char** builtIn, const int builtInCount, struct Vector* user)
{
	/* Get the next token */
	if(!lexer_setCurID(me))
		 return FALSE;
	ASSERT(me->curID);

	#ifdef CPARSERVERBOSE
	printf("lexer_specialID looking for %s\n", me->curID);
	#endif

	if(lexer_specialID_string(me, retB, retU, builtIn, builtInCount, user, me->curID)) {
	    FREE_IF_NZ (me->curID);
	    return TRUE;
	}

	return FALSE;
}

/* Checks for the ID passed in str in the builtin array of IDs passed in builtin and/or in the array of user defined
   IDs passed in user.  Returns the index to the ID in retB (if found in the built in list) or retU (if found in the 
   user defined list) if it is found.  */
BOOL lexer_specialID_string(struct VRMLLexer* me, int* retB, int* retU,
 const char** builtIn, const int builtInCount,
 struct Vector* user, const char* str)
{
 int i;
 BOOL found=FALSE;
 int ind;

 #ifdef CPARSERVERBOSE
 printf ("lexer_specialID_string, builtInCount %d, builtIn %u\n",builtInCount, builtIn); 
 #endif

 /* Have to be looking in either the builtin and/or the user defined lists */
 if(!retB && !retU)
  return FALSE;

 if(retB) *retB=ID_UNDEFINED;
 if(retU) *retU=ID_UNDEFINED;

 /* Try as built-in */
 /* Look for the ID in the passed built in array.  If it is found, return the index to the ID in retB */
  for(i=0; i!=builtInCount; ++i) {
	/* printf ("lexer_specialID_string, comparing :%s: and :%s:\n",str,builtIn[i]); */
  if(!strcmp(str, builtIn[i])) {
#ifdef CPARSERVERBOSE
   printf("found ID %s matches %s, return retB %d\n", str, builtIn[i], i);
#endif
	/* is this a PROTOKEYWORD? If so, change any possible depreciated tags to new ones */
	if (builtIn == PROTOKEYWORDS) {
		switch (i) {
			case PKW_eventIn: i = PKW_inputOnly; break;
			case PKW_eventOut: i= PKW_outputOnly; break;
			case PKW_exposedField: i= PKW_inputOutput; break;
			case PKW_field: i= PKW_initializeOnly; break;
			default : { /* do nothing - already in new format */ }
		}
		#ifdef CPARSERVERBOSE
   		printf("CONVERTED - found ID %s matches %s, return retB %d\n", str, builtIn[i], i);
		#endif
	}

   if(retB) {
    *retB=i;
    found=TRUE;
   }
   break;
  }
}

 /* Return if no user list is requested or it is empty */
 if(!user)
  return found;

/*
 for(i=0; i!=vectorSize(user); ++i) { 
	printf ("special_ID_string, user %d is %s\n",
		i, vector_get(char*, user, i));
}
*/


 /* Already defined user id? */
 /* Look for the ID in the passed user array.  If it is found, return the index to the ID in retU */
 /* JAS - COUNT DOWN from last entered to first; this makes duplicates use the latest entry. */
 /* use an index to see when we go from 0 to -1; int can not do this, as it is unsigned */
 /* was : for(i=0; i!=vectorSize(user); ++i) { */


for (ind= (int)vectorSize(user)-1; ind>=0; ind--) {

	/* convert an int into an int */
	i = (int) ind;

	/* printf ("lexer sp, ele %d\n",i);
	printf ("lexer_specialID_string, part II, comparing :%s: and :%s: lengths %d and %d\n", 
	str, vector_get(char*, user, i), 
	strlen(str), strlen(vector_get(char*, user, i)));  */

  if(!strcmp(str, vector_get(char*, user, i))) {
   #ifdef CPARSERVERBOSE
   printf("found ID %s matches %s, return retU %d\n", str, vector_get(char*, user, i), i);
   #endif

   if(retU) {
    *retU=i;
    found=TRUE;
   }
   break;
  }
 }

 return found;
}


/* Lexes and defines an ID */
/* Adds the ID to the passed vector of IDs (unless it is already present) */
/* Note that we only check for duplicate IDs if multi is TRUE */
BOOL lexer_defineID(struct VRMLLexer* me, int* ret, struct Vector* vec, BOOL multi) {

	/* Get the next token */
	if(!lexer_setCurID(me))
		return FALSE;
	ASSERT(me->curID);

	#ifdef CPARSERVERBOSE
	printf ("lexer_defineID, VRMLLexer %u Vector %u\n",me,vec); 
	if (multi) printf ("Multi SET\n"); else printf ("no Mlti set\n");
	#endif

	/* User list should be created */
	ASSERT(vec);


	/* If multiple definition possible? Look if the ID's already there */
	if(multi) {
		int i;
		for(i=0; i!=vectorSize(vec); ++i) {

		#ifdef CPARSERVERBOSE
		printf ("lexer_defineID, comparing %s to %s\n",me->curID, vector_get(const char*, vec, i));
		#endif

		if(!strcmp(me->curID, vector_get(const char*, vec, i))) {
			FREE_IF_NZ (me->curID);
			*ret=i;
			return TRUE;
		}
		}
	} else {
		/* is this already defined? */
		if (gglobal()->internalc.global_strictParsing) {
			int i;
			for(i=0; i!=vectorSize(vec); ++i) {
				if(!strcmp(me->curID, vector_get(const char*, vec, i))) {
					ConsoleMessage ("warning, duplicate ID (%s at %u), using last DEF",me->curID,i);
				}	
			}
		}

	}

	/* Define the id */
	/* Add this ID to the passed vector of IDs */
	*ret=vectorSize(vec);
	#ifdef CPARSERVERBOSE
		printf("lexer_defineID: adding %s to vector %p\n", me->curID, vec);
	#endif
	/* save the curID on the stack... */
	vector_pushBack(char*, vec, me->curID);

	/* set curID to NULL to indicate that we have used this id */
	me->curID=NULL;

	return TRUE;
}


/* A eventIn/eventOut terminal symbol */
/* Looks for the current token in builtin and/or user defined name arrays depending on the 
   requested return values and the eventtype (in or out)
   If looking through EVENT_IN, EVENT_OUT, or EXPOSED_FIELD, checks to see if the current token
   is valid with either set_ or _changed stripped from it 
   If rBO is non-null, then search through EVENT_IN or EVENT_OUT and return the index of the event (if found) in rBO
   If rBE is non-null, then search through EXPOSED_FIELD and return the index of the event (if found) in rBE
   If rUO is non-null, then search through user_inputOnly or user_outputOnly and return the index of the event (if found) in rUO
   if rUE is non-null, then search through user_inputOutput and return the index of the event (if found) in rUE */ 

BOOL lexer_event(struct VRMLLexer* me,
 struct X3D_Node* routedNode,
 int* rBO, int* rBE, int* rUO, int* rUE,
 int routedToFrom)
{
 BOOL found=FALSE;

 struct Vector* uarr;
 const char** arr;
 int arrCnt;
 const char** userArr;
 int userCnt;

 if(routedToFrom==ROUTED_FIELD_EVENT_IN)
 {
  /* If we are looking for an eventIn we need to look through the EVENT_IN array and the user_inputOnly vector */
  uarr=me->user_inputOnly;
  arr=EVENT_IN;
  arrCnt=EVENT_IN_COUNT;
 } else
 {
  /* If we are looking for an eventOut we need to look through the EVENT_OUT array and the user_outputOnly vector */
  uarr=me->user_outputOnly;
  arr=EVENT_OUT;
  arrCnt=EVENT_OUT_COUNT;
 }

 /* Get the next token  - if this is a PROTO expansion, this will be non-NULL */
 if (me->curID == NULL) 
 if(!lexer_setCurID(me)) {
  return FALSE;
 }

 ASSERT(me->curID);

#ifdef CPARSERVERBOSE
 printf("lexer_event: looking for %s\n", me->curID);
#endif

 /* Get a pointer to the data in the vector of user defined event names */
 userArr=&vector_get(const char*, uarr, 0);
 userCnt=vectorSize(uarr);

 /* Strip off set_ or _changed from current token.  Then look through the EVENT_IN/EVENT_OUT array for the eventname (current token).  
    If it is found, return the index of the eventname. Also looks through fields of the routedNode to check if fieldname is valid for that node 
    (but doesn't seem to do anything if not valid ... ) */
 if(rBO)
  *rBO=findRoutedFieldInARR(routedNode, me->curID, routedToFrom, arr, arrCnt,
   FALSE);

 /* Strip off set_ or _changed from current token.  Then look through the user_inputOnly/user_outputOnly array for the eventname (current token).  
    If it is found, return the index of the eventname.  */
 if(rUO)
  *rUO=findRoutedFieldInARR(routedNode, me->curID, routedToFrom,
   userArr, userCnt, TRUE);

 /* Set the found flag to TRUE if the eventname was found in either the EVENT_IN/EVENT_OUT or user_inputOnly/user_outputOnly arrays */ 
 if(!found)
  found=((rBO && *rBO!=ID_UNDEFINED) || (rUO && *rUO!=ID_UNDEFINED));

#ifdef CPARSERVERBOSE
 if (rBO && *rBO != ID_UNDEFINED)
	printf("lexer_event: found in EVENT_IN/EVENT_OUT\n");

 if (rUO && *rUO != ID_UNDEFINED)
	printf("lexer_event: found in user_inputOnly/user_outputOnly\n");
#endif

 /* Get a pointer to the event names in the vector of user defined exposed fields */
 userArr=&vector_get(const char*, me->user_inputOutput, 0);
 userCnt=vectorSize(me->user_inputOutput);

 /* findRoutedFieldInEXPOSED_FIELD calls findRoutedFieldInARR(node, field, fromTo, EXPOSED_FIELD, EXPOSED_FIELD_COUNT, 0) */
 /* Strip off set_ or _changed from current token.  Then look through the EXPOSED_FIELD array for the eventname (current token). 
    If it is found, return the index of the eventname.  Also looks through fields of the routedNode to check if fieldname is valid for that node
    (but doesn't seem to do anything if not valid ... ) */ 
 if(rBE)
  *rBE=findRoutedFieldInEXPOSED_FIELD(routedNode, me->curID, routedToFrom);

 /* Strip off set_ or _changed from current token.  Then look through the user_inputOutput array for the eventname (current token). 
    If it is found, return the index of the eventname.  */ 
 if(rUE)
  *rUE=findRoutedFieldInARR(routedNode, me->curID, routedToFrom,
   userArr, userCnt, TRUE);

 /* Set the found flag to TRUE if the eventname was found in either the EXPOSED_FIELD or user_inputOutput arrays */ 
 if(!found)
  found=((rBE && *rBE!=ID_UNDEFINED) || (rUE && *rUE!=ID_UNDEFINED));

#ifdef CPARSERVERBOSE
 if (rBE && *rBE != ID_UNDEFINED)
	printf("lexer_event: found in EXPOSED_FIELD\n");

 if (rUE && *rUE != ID_UNDEFINED)
	printf("lexer_event: found in user_inputOutput\n");
#endif

 if(found) {
     FREE_IF_NZ(me->curID);
	}

 return found;
}

/* Lexes a fieldId terminal symbol */
/* If retBO isn't null, checks for the field in the FIELDNAMES array */
/* If retBE isn't null, checks for the field in the EXPOSED_FIELD array */
/* if retUO isn't null, checks for the field in the user_initializeOnly vector */
/* if retUE isn't null, checks for the field in the user_inputOutput vector */
/* returns the index of the field in the corresponding ret value if found */
BOOL lexer_field(struct VRMLLexer* me,
 int* retBO, int* retBE, int* retUO, int* retUE)
{
 const char** userArr;
 int userCnt;

 BOOL found=FALSE;

  /* Get next token */
 if(!lexer_setCurID(me))
  return FALSE;
 ASSERT(me->curID);

  /* Get a pointer to the entries in the user_initializeOnly vector */
 userArr=&vector_get(const char*, me->user_initializeOnly, 0);
 userCnt=vectorSize(me->user_initializeOnly);

#ifdef CPARSERVERBOSE
 printf("lexer_field: looking for %s\n", me->curID);
#endif
 /* findFieldInFIELD is #defined to findFieldInARR(field, FIELDNAMES, FIELDNAMES_COUNT) */
 /* look through the FIELDNAMES array for the fieldname (current token).  If it is found, return the index of the fieldname */
 if(retBO)
  *retBO=findFieldInFIELD(me->curID);

 /* look through the fieldnames from the user_initializeOnly names vector for the fieldname (current token).  If it is found, return the index 
   of the fieldname */
 if(retUO)
  *retUO=findFieldInARR(me->curID, userArr, userCnt);

  /* Set the found flag to TRUE if the fieldname was found in either FIELDNAMES or user_initializeOnly */
 if(!found)
  found=((retBO && *retBO!=ID_UNDEFINED) || (retUO && *retUO!=ID_UNDEFINED));

  /* Get a pointer to the entries in the user_inputOutput vector */
 userArr=&vector_get(const char*, me->user_inputOutput, 0);
 userCnt=vectorSize(me->user_inputOutput);
  
  /* findFieldInEXPOSED_FIELD #defined to findFieldInARR(field, EXPOSED_FIELD, EXPOSED_FIELD_COUNT) */
  /* look through the EXPOSED_FIELD array for the fieldname (current token).  If it is found, return the index of the fieldname.  */
 if(retBE)
  *retBE=findFieldInEXPOSED_FIELD(me->curID);

 /* look through the fieldnames from the user_inputOutput names vector for the fieldname (current token).  If it is found, return the
    index of the fieldname */
 if(retUE)
  *retUE=findFieldInARR(me->curID, userArr, userCnt);

 /* Set the found flag to TRUE if the fieldname was found in either EXPOSED_FIELD or user_inputOutput */
 if(!found)
  found=((retBE && *retBE!=ID_UNDEFINED) || (retUE && *retUE!=ID_UNDEFINED));

#ifdef CPARSERVERBOSE
 if (retBO && *retBO != ID_UNDEFINED) 
   printf("lexer_field: found field in FIELDNAMES\n");
 if (retUO && *retUO != ID_UNDEFINED) 
   printf("lexer_field: found field in me->user_initializeOnly\n");
 if (retBE && *retBE != ID_UNDEFINED) 
   printf("lexer_field: found field in EXPOSED_FIELD\n");
 if (retUE && *retUE != ID_UNDEFINED) 
   printf("lexer_field: found field in me->user_inputOutput\n");
#endif

 if(found)
 {
  FREE_IF_NZ (me->curID);
 }

 return found;
}



/* Conversion of user field name to char* */
const char* lexer_stringUser_fieldName(struct VRMLLexer* me, int name, int mode)
{
	#define OOB_RETURN_VAL "__UNDEFINED__"

 switch(mode)
 {
  case PKW_initializeOnly:
   if (name>vectorSize(me->user_initializeOnly)) return OOB_RETURN_VAL;
   return lexer_stringUser_initializeOnly(me, name);
  case PKW_inputOutput:
   if (name>vectorSize(me->user_inputOutput)) return OOB_RETURN_VAL;
   return lexer_stringUser_inputOutput(me, name);
  case PKW_inputOnly:
   if (name>vectorSize(me->user_inputOnly)) return OOB_RETURN_VAL;
   return lexer_stringUser_inputOnly(me, name);
  case PKW_outputOnly:
   if (name>vectorSize(me->user_outputOnly)) return OOB_RETURN_VAL;
   return lexer_stringUser_outputOnly(me, name);
 }
 ASSERT(FALSE);
 return OOB_RETURN_VAL; /* gets rid of compile time warnings */
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
/* win 32 complains not enough parameters for _general but what should go for int? && true / && 1*/
#ifdef _MSC_VER
#define NUMBER_PROCESS_SIGN_INT \
 NUMBER_PROCESS_SIGN_GENERAL(&& TRUE)
#else
#define NUMBER_PROCESS_SIGN_INT \
 NUMBER_PROCESS_SIGN_GENERAL()
#endif
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
   ASSERT(!(*ret%0x10));
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
 decimalFact=(float) 0.1;
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
  *ret*=(float)(pow(10, exp));
 }
 LEXER_UNGETINPUT(c)

 RETURN_NUMBER_WITH_SIGN
}


BOOL lexer_double(struct VRMLLexer* me, vrmlDoubleT* ret)
{
 int c;

 BOOL neg;
 BOOL afterPoint;	/* Already after decimal point? */
 double decimalFact;	/* Factor next decimal digit is multiplied with */

 if(me->curID) return FALSE;
 lexer_skip(me);

 /* Really a double? */
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
  *ret*=(pow(10, exp));
 }
 LEXER_UNGETINPUT(c)

 RETURN_NUMBER_WITH_SIGN
}

BOOL lexer_string(struct VRMLLexer* me, vrmlStringT* ret)
{
 int c;
 char* buf;
 int bufLen=INITIAL_STRINGLEN;
 int cur=0;

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
 buf=MALLOC(char *, sizeof(*buf)*bufLen);
 ASSERT(buf);

 /* Main processing loop */
 while(TRUE)
 {
  /* Buffer resize needed?  Extra space for closing 0. */
  if(cur+1==bufLen)
  {
   bufLen*=2;
   buf=REALLOC(buf, sizeof(*buf)*bufLen);
  }
  ASSERT(cur+1<bufLen);

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
 ASSERT(cur<bufLen);
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

if (me->curID) {
	ConsoleMessage ("lexer_operator: did not expect to find a text string  - it is \"%s\" - as I am looking for a \'%c\'\n",me->curID,op);
	FREE_IF_NZ(me->curID);
	/* printf ("NEXTIN is :%s:\n",me->nextIn); */
 }

 lexer_skip(me);

 LEXER_GETINPUT(c);
 CHECK_EOF(c);
 /* printf ("lxer_opr, got %d\n",c); */

 if(c!=op)
 {
     LEXER_UNGETINPUT(c);
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
                ret->p=MALLOC(vrmlStringT *, sizeof(vrmlStringT));
                if(!lexer_sfstringValue(me, (void*)ret->p))
                        return FALSE;
                ret->n=1;
                return TRUE;
        }

        /* Otherwise, a real vector */
        vec=newVector(vrmlStringT, 128);
        while(!lexer_closeSquare(me)) {
                vrmlStringT val;
                if(!lexer_sfstringValue (me, &val)) {
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

        ret->n=vectorSize(vec);
        ret->p=vector_releaseData(vrmlStringT, vec);

        deleteVector(vrmlStringT, vec);
        return TRUE;
}

/* isolate the PROTO that we want from the just read in EXTERNPROTO string */
void embedEXTERNPROTO(struct VRMLLexer *me, const char *myName, char *buffer, char *pound) {
        char *cp;
        char *externProtoPointer;
        char *proto;
        int curlscount;
        int foundBracket;

        /* step 1. Remove comments, so that we do not locate the requested PROTO in comments. */
        cp = buffer;

        while (*cp != '\0') {
                if (*cp == '#') {
                        do {
                                *cp = ' ';
                                cp++;
                                /* printf ("lexer, found comment, current char %d:%c:\n",cp,cp);  */
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
                /* printf ("looking for ID %s\n",pound);  */
                proto=buffer;

                do {
                        FIND_PROTO_IN_proto_BUFFER

                        /* is this the PROTO we are looking for? */
                        proto += sizeof ("PROTO");
                        while ((*proto <= ' ') && (*proto != '\0')) proto++;
                        /* printf ("found PROTO at %s\n",proto);  */
                } while (strncmp(pound,proto,strlen(pound)) != 0);
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


        externProtoPointer = MALLOC (char *, sizeof (char) * (strlen (proto)+strlen(myName) +40));
	externProtoPointer[0]='\0';
	strcat (externProtoPointer, "PROTO ");
        strcat (externProtoPointer,myName);
        strcat (externProtoPointer," ");
        strcat (externProtoPointer,proto);

/* printf ("pushing protoexp :%s:\n",externProtoPointer); */

	/* push it on to the lexer input string stack */
	lexer_fromString(me,externProtoPointer);
}

/* the curID is EXTERNPROTO. Replace the EXTERNPROTO with the actual PROTO string read in from
   an external file */

void lexer_handle_EXTERNPROTO(struct VRMLLexer *me) {
#ifdef HAD_RESOURCE_LOAD
	char *myName = NULL;
#endif
	int mode;
	int type;
	struct Multi_String url;

	resource_item_t *res;

	/* expect the EXTERNPROTO proto name */
	if (lexer_setCurID(me)) {
			/* printf ("next token is %s\n",me->curID); */
#ifdef HAD_RESOURCE_LOAD
			myName = STRDUP(me->curID);
#endif
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

	res = resource_create_multi(&url);
    
    FREE_IF_NZ(url.p);
    url.p = NULL;
    url.n = 0;
	resource_identify(gglobal()->resources.root_res, res);

	if (res->type != rest_invalid) {
#ifdef HAD_RESOURCE_LOAD
		if (resource_fetch(res)) {
			unsigned char *buffer;
			char *pound;
 			pound = strchr(res->URLrequest, '#'); 
			if (resource_load(res)) {
				s_list_t *l;
				openned_file_t *of;
				l = res->openned_files;
				of = ml_elem(l);
				buffer = of->fileData;
 				embedEXTERNPROTO(me, myName, (char *)buffer, pound); 
			}
		}
#else
		res->status = ress_failed;
		printf("externProto not currently supported\n");
#endif
	}
	
	if (res->status == ress_loaded) {
		/* ok - we are replacing EXTERNPROTO with PROTO */
		res->status = ress_parsed;
		res->complete = TRUE;
	} else {
		/* failure, FIXME: remove res from root_res... */
		/* 		resource_destroy(res); */
	}

	/* so, lets continue. Maybe this EXTERNPROTO is never referenced?? */
	lexer_setCurID(me);
	/* printf ("so, curID is :%s: and rest is :%s:\n",me->curID, me->nextIn); */
	return;
}


/* recursively skip to the closing curly bracket - ignoring all that comes between. */
void skipToEndOfOpenCurly(struct VRMLLexer *me, int level) {
	int curlyCount = 1;
	vrmlStringT tmpstring;

	#ifdef CPARSELEXERVERBOSE
	if (level == 0) printf ("start of skipToEndOfOpenCurly, have :%s:\n",me->nextIn);
	#endif

	while ((curlyCount > 0) && (*me->nextIn != '\0')) {
		lexer_skip(me);
		#ifdef CPARSELEXERVERBOSE
		printf ("cc %d, looking at :%c:\n",curlyCount,*me->nextIn);
		#endif

		if (*me->nextIn == '{') curlyCount++;
		else if (*me->nextIn == '}') curlyCount--;
		if (lexer_string(me,&tmpstring)) {
			#ifdef CPARSELEXERVERBOSE
			printf ("after string, :%s:\n",me->nextIn);
			printf ("and string :%s:\n",tmpstring->strptr);
			#endif

			FREE_IF_NZ(tmpstring->strptr); /* throw it away */
		} else {
			me->nextIn++;
		}
	}

	#ifdef CPARSELEXERVERBOSE
	if (level == 0) printf ("returning from skipToEndOfOpenCurly nextIn :%s:\n",me->nextIn);
	#endif
}

/* concat 2 strings, and tell the lexer to scan from this new string */
void concatAndGiveToLexer(struct VRMLLexer *me, const char *str_a, const char *str_b) {
	char *newstring;
	int len_a=0;
	int len_b=0;
	if (str_a != NULL) len_a = (int) strlen(str_a);
	if (str_b != NULL) len_b = (int) strlen(str_b);

	if ((len_a == 0) & (len_b == 0)) {
		printf ("concatAndGiveToLexer, no input!\n");
		return;
	}

	newstring = MALLOC(char *, sizeof (char) * (len_a + len_b +10));
	newstring[0] = '\0';
	if (len_a != 0) strcat (newstring,str_a);
	if (len_b != 0) strcat (newstring,str_b);

	/* printf ("concatAndGiveToLexer, sending in :%s:\n",newstring); */
	lexer_fromString(me,newstring);
}
