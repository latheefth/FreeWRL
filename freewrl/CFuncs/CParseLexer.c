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

 /* is this an EXTERNPROTO? if so, handle it here */
 if (lexer_keyword(me,KW_EXTERNPROTO)) {
	lexer_handle_EXTERNPROTO(me);

	/* ok - we are replacing EXTERNPROTO with PROTO */
	me->curID = MALLOC (sizeof(char)*20);
	strcpy(me->curID,"PROTO");
 }
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
 /* printf ("lexer_defineID, curID %s\n",me->curID); */
 vector_pushBack(char*, vec, me->curID);
 me->curID=NULL;
 return TRUE;
}

/* A eventIn/eventOut terminal symbol */
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
  uarr=user_eventIn;
  arr=EVENT_IN;
  arrCnt=EVENT_IN_COUNT;
 } else
 {
  uarr=user_eventOut;
  arr=EVENT_OUT;
  arrCnt=EVENT_OUT_COUNT;
 }

 if(!lexer_setCurID(me))
  return FALSE;
 assert(me->curID);

 const char** userArr=&vector_get(const char*, uarr, 0);
 size_t userCnt=vector_size(uarr);

 if(rBO)
  *rBO=findRoutedFieldInARR(routedNode, me->curID, routedToFrom, arr, arrCnt,
   FALSE);
 if(rUO)
  *rUO=findRoutedFieldInARR(routedNode, me->curID, routedToFrom,
   userArr, userCnt, TRUE);
 if(!found)
  found=((rBO && *rBO!=ID_UNDEFINED) || (rUO && *rUO!=ID_UNDEFINED));

 /*printf("%p %u %d\n", rBO, *rBO, *rBO!=ID_UNDEFINED);*/

 userArr=&vector_get(const char*, user_exposedField, 0);
 userCnt=vector_size(user_exposedField);

 if(rBE)
  *rBE=findRoutedFieldInEXPOSED_FIELD(routedNode, me->curID, routedToFrom);
 if(rUE)
  *rUE=findRoutedFieldInARR(routedNode, me->curID, routedToFrom,
   userArr, userCnt, TRUE);
 if(!found)
  found=((rBE && *rBE!=ID_UNDEFINED) || (rUE && *rUE!=ID_UNDEFINED));

 if(found)
  FREE_IF_NZ(me->curID)

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

 const char** userArr=&vector_get(const char*, user_field, 0);
 size_t userCnt=vector_size(user_field);

 /*printf("%s\n", me->curID);*/

 if(retBO)
  *retBO=findFieldInFIELD(me->curID);
 if(retUO)
  *retUO=findFieldInARR(me->curID, userArr, userCnt);
 if(!found)
  found=((retBO && *retBO!=ID_UNDEFINED) || (retUO && *retUO!=ID_UNDEFINED));

 userArr=&vector_get(const char*, user_exposedField, 0);
 userCnt=vector_size(user_exposedField);
  
 if(retBE)
  *retBE=findFieldInEXPOSED_FIELD(me->curID);
 if(retUE)
  *retUE=findFieldInARR(me->curID, userArr, userCnt);
 if(!found)
  found=((retBE && *retBE!=ID_UNDEFINED) || (retUE && *retUE!=ID_UNDEFINED));

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
			proto = strstr (buffer,"PROTO"); \
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

void lexer_handle_EXTERNPROTO(struct VRMLLexer *me) {
	char *myName = NULL;
	indexT mode;
	indexT type;
	struct Multi_String url;
	int i;
	char *pound;
	char *savedCurInputURL;
	char *buffer;
	char firstBytes[4];
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
		

		if (getValidFileFromUrl (testname ,getInputURL(), &url, firstBytes)) {


                	buffer = readInputString(testname,"");
			embedEXTERNPROTO(me,myName,buffer,pound);
			return;
		} else {
			/* printf ("fileExists returns failure for %s\n",testname); */
		}

	}
	PARSE_ERROR ("Not Successful at getting EXTERNPROTO");
}
