/* Sourcecode for CParseParser.h */

#include <stdlib.h>
#include <assert.h>

#include "CParseParser.h"
#include "CProto.h"
#include "CScripts.h"

#define PARSE_ERROR(msg) \
 { \
  parseError(msg); \
  PARSER_FINALLY \
  return FALSE; \
 }
#define PARSER_FINALLY

#define DEFMEM_INIT_SIZE	16

/* The DEF/USE memory. */
Stack* DEFedNodes=NULL;

/* Our PROTOs */
Stack* PROTOs=NULL;

/* Parsing a specific type */
BOOL (*PARSE_TYPE[])(struct VRMLParser*, void*)={
 &parser_sffloatValue_, &parser_mffloatValue,
 &parser_sfrotationValue, &parser_mfrotationValue,
 &parser_sfcolorValue, &parser_mfvec3fValue,
 &parser_sfboolValue, &parser_mfboolValue,
 &parser_sfint32Value_, &parser_mfint32Value,
 &parser_sfnodeValue, &parser_mfnodeValue,
 &parser_sfcolorValue, &parser_mfcolorValue,
 &parser_sfcolorrgbaValue, &parser_mfcolorrgbaValue,
 &parser_sftimeValue, &parser_mftimeValue,
 &parser_sfstringValue_, &parser_mfstringValue,
 &parser_sfvec2fValue, &parser_mfvec2fValue,
 &parser_sfimageValue,
 NULL
};

/* for error messages */
char fw_outline[2000];

/* Macro definitions used more than once for event processing */
/* ********************************************************** */

/* Use real size for those types? */
#define ROUTE_REAL_SIZE_sfbool	TRUE
#define ROUTE_REAL_SIZE_sfcolor	TRUE
#define ROUTE_REAL_SIZE_sffloat	TRUE
#define ROUTE_REAL_SIZE_sfimage	TRUE
#define ROUTE_REAL_SIZE_sfint32	TRUE
#define ROUTE_REAL_SIZE_sfnode	TRUE
#define ROUTE_REAL_SIZE_sfrotation	TRUE
#define ROUTE_REAL_SIZE_sfstring	TRUE
#define ROUTE_REAL_SIZE_sftime	TRUE
#define ROUTE_REAL_SIZE_sfvec2f	TRUE
#define ROUTE_REAL_SIZE_sfvec3f	TRUE
#define ROUTE_REAL_SIZE_mfbool	FALSE
#define ROUTE_REAL_SIZE_mfcolor	FALSE
#define ROUTE_REAL_SIZE_mfcolorrgba	FALSE
#define ROUTE_REAL_SIZE_mffloat	FALSE
#define ROUTE_REAL_SIZE_mfimage	FALSE
#define ROUTE_REAL_SIZE_mfint32	FALSE
#define ROUTE_REAL_SIZE_mfnode	FALSE
#define ROUTE_REAL_SIZE_mfrotation	FALSE
#define ROUTE_REAL_SIZE_mfstring	FALSE
#define ROUTE_REAL_SIZE_mftime	FALSE
#define ROUTE_REAL_SIZE_mfvec2f	FALSE
#define ROUTE_REAL_SIZE_mfvec3f	FALSE

/* General processing macros */
#define PROCESS_EVENT(constPre, destPre, node, field, type, var) \
 case constPre##_##field: \
  destPre##Len= \
   (ROUTE_REAL_SIZE_##type ? sizeof_member(struct X3D_##node, var) : 0); \
  destPre##Ofs=offsetof(struct X3D_##node, var); \
  break;
#define EVENT_BEGIN_NODE(fieldInd, ptr, node) \
 case NODE_##node: \
 { \
  struct X3D_##node* node2=(struct X3D_##node*)ptr; \
  switch(fieldInd) \
  {
#define EVENT_END_NODE(node,fieldString) \
  default: \
	/* printf ("node is a %s, event %s\n",stringNodeType(node2->_nodeType),fieldString);*/ \
   	/* PARSE_ERROR("Unsupported event for node!") */ \
	strcpy (fw_outline,"ERROR: Unsupported event ("); \
	strcat (fw_outline,fieldString); \
	strcat (fw_outline,") for node of type "); \
	strcat (fw_outline,stringNodeType(node2->_nodeType)); \
  	ConsoleMessage(fw_outline);  \
	fprintf (stderr,"%s\n",fw_outline); \
  	PARSER_FINALLY  \
  	return FALSE;  \
  } \
  break; \
 }
#define EVENT_NODE_DEFAULT \
 default: \
  PARSE_ERROR("Unsupported node!")


/* ************************************************************************** */
/* Constructor and destructor */

struct VRMLParser* newParser(void* ptr, unsigned ofs)
{
 struct VRMLParser* ret=MALLOC(sizeof(struct VRMLParser));
 ret->lexer=newLexer();
 assert(ret->lexer);
 ret->ptr=ptr;
 ret->ofs=ofs;
 ret->curPROTO=NULL;

 return ret;
}

void deleteParser(struct VRMLParser* me)
{
 assert(me->lexer);
 deleteLexer(me->lexer);

 FREE_IF_NZ (me);
}

static void parser_scopeOut_DEFUSE();
static void parser_scopeOut_PROTO();
void parser_destroyData()
{
 /* DEFed Nodes. */
 if(DEFedNodes)
 {
  while(!stack_empty(DEFedNodes))
   parser_scopeOut_DEFUSE();
  deleteStack(struct Vector*, DEFedNodes);
  DEFedNodes=NULL;
 }
 assert(!DEFedNodes);

 /* PROTOs */
 /* FIXME: PROTOs must not be stacked here!!! */
 if(PROTOs)
 {
  while(!stack_empty(PROTOs))
   parser_scopeOut_PROTO();
  deleteStack(struct Vector*, PROTOs);
  PROTOs=NULL;
 }
 assert(!PROTOs);

 lexer_destroyData();

  /* zero script count */
  zeroScriptHandles ();	
}

/* Scoping */

static void parser_scopeIn_DEFUSE()
{
 if(!DEFedNodes)
  DEFedNodes=newStack(struct Vector*);
 assert(DEFedNodes);
 stack_push(struct Vector*, DEFedNodes,
  newVector(struct X3D_Node*, DEFMEM_INIT_SIZE));
 assert(!stack_empty(DEFedNodes));
}
static void parser_scopeIn_PROTO()
{
 if(!PROTOs)
  PROTOs=newVector(struct ProtoDefinition*, DEFMEM_INIT_SIZE);
}

static void parser_scopeOut_DEFUSE()
{
 indexT i;
 assert(!stack_empty(DEFedNodes));
 /* FIXME:  Can't delete individual nodes, as they might be referenced! */
 deleteVector(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes));
 stack_pop(struct Vector*, DEFedNodes);
}

static void parser_scopeOut_PROTO()
{
 indexT i;
 /* Do not delete the ProtoDefinitions, as they are referenced in the scene
  * graph!  TODO:  How to delete them properly? */

 vector_popBackN(struct ProtoDefinition*, PROTOs, lexer_getProtoPopCnt());
 lexer_scopeOut_PROTO();
}

void parser_scopeIn()
{
 lexer_scopeIn();
 parser_scopeIn_DEFUSE();
 parser_scopeIn_PROTO();
}
void parser_scopeOut()
{
 parser_scopeOut_DEFUSE();
 parser_scopeOut_PROTO();
 lexer_scopeOut();
}

/* ************************************************************************** */
/* The start symbol */

BOOL parser_vrmlScene(struct VRMLParser* me)
{
 while(TRUE)
 {
  /* Try nodeStatement */
  {
   vrmlNodeT node;
   if(parser_nodeStatement(me, &node))
   {
    addToNode(me->ptr, me->ofs, node);
    continue;
   }
  }

  /* Try routeStatement */
  if(parser_routeStatement(me))
   continue;
  /* Try protoStatement */
  if(parser_protoStatement(me))
   continue;

  break;
 }

 return lexer_eof(me->lexer);
}

/* ************************************************************************** */
/* Nodes and fields */

/* Parses an interface declaration and adds it to the PROTO definition */
BOOL parser_interfaceDeclaration(struct VRMLParser* me,
 struct ProtoDefinition* proto, struct Script* script)
{
 indexT mode;
 indexT type;
 indexT name;
 union anyVrml defaultVal;
 struct ProtoFieldDecl* pdecl=NULL;
 struct ScriptFieldDecl* sdecl=NULL;
 int haveISinPROTO = FALSE;			/* did we find an IS in a Script declaration? */
 int i;
 struct ProtoFieldDecl* jaspdecl=NULL;
 int foundIt = FALSE;
 char *nm;

 /* Either PROTO or Script interface! */
 assert((proto || script) && !(proto && script));

 if(!lexer_protoFieldMode(me->lexer, &mode))
  return FALSE;

 /* Script may not take exposedFields */
 if(script && mode==PKW_exposedField)
  PARSE_ERROR("Scripts must not have exposedFields!")
  
 if(!lexer_fieldType(me->lexer, &type))
  PARSE_ERROR("Expected fieldType after proto-field keyword!")
 switch(mode)
 {
  #define LEX_DEFINE_FIELDID(suff) \
   case PKW_##suff: \
    if(!lexer_define_##suff(me->lexer, &name)) \
     PARSE_ERROR("Expected fieldNameId after field type!") \
    break;
  LEX_DEFINE_FIELDID(field)
  LEX_DEFINE_FIELDID(exposedField)
  LEX_DEFINE_FIELDID(eventIn)
  LEX_DEFINE_FIELDID(eventOut)
#ifndef NDEBUG
  default:
   assert(FALSE);
#endif
 }

 if(proto) {
  pdecl=newProtoFieldDecl(mode, type, name);
 } else {
	/* get the name and type */
	haveISinPROTO = (me->curPROTO && lexer_keyword(me->lexer, KW_IS));

  	sdecl=newScriptFieldDecl(mode, type, name);

	/* JAS are we running as a proto, and do we have an "IS" */
 	if(haveISinPROTO) {

		/* skip to the name, and find it */
		FREE_IF_NZ(me->lexer->curID)

		lexer_setCurID(me->lexer);
		/* printf ("next id is %s\n",me->lexer->curID);
		printf ("PROTO field size is %d\n",protoDefinition_getFieldCount(me->curPROTO)); */
		i=0; while (i<protoDefinition_getFieldCount(me->curPROTO)) {
			jaspdecl = protoDefinition_getFieldByNum(me->curPROTO,i);
			nm = protoFieldDecl_getStringName(jaspdecl);
			/* printf ("name at index %d is %s comparing to %s\n",i,nm,me->lexer->curID); */
			if (nm != NULL) {
			if (strcmp(me->lexer->curID,nm) == 0) {
				/* printf ("FOUND NAME IN TABLE \n");
				printf ("index type is %d\n",protoFieldDecl_getType(jaspdecl));
				printf ("index accesstype is %d\n",protoFieldDecl_getAccessType(jaspdecl));  */
				defaultVal = protoFieldDecl_getDefaultValue(jaspdecl);

				sdecl->ISname= strdup(me->lexer->curID);
				i = protoDefinition_getFieldCount(me->curPROTO); /* end loop */

				foundIt = TRUE;
				FREE_IF_NZ(me->lexer->curID);
			}
			}
			i++;
		}
		if (!foundIt) {
			parseError ("error in finding IS for field in Script interface declaration");
			haveISinPROTO = FALSE;
			FREE_IF_NZ(sdecl->ISname);
		}
	}

 }
  
 if (!haveISinPROTO) {
 	if(mode==PKW_field || mode==PKW_exposedField) {
 	 assert(PARSE_TYPE[type]);
 	 if(!PARSE_TYPE[type](me, (void*)&defaultVal))
 	 {
 	  parseError("Expected default value for field!");
 	  if(pdecl) deleteProtoFieldDecl(pdecl);
 	  if(sdecl) deleteScriptFieldDecl(sdecl);
 	  return FALSE;
 	 }
	
	  if(proto)
	   pdecl->defaultVal=defaultVal;
	  else
	  {
	   assert(script);
	   scriptFieldDecl_setFieldValue(sdecl, defaultVal);
	  }
	 }
 } else {
	assert(script);
	scriptFieldDecl_setFieldValue(sdecl,defaultVal);
 }

 if(proto)
 {
  protoDefinition_addIfaceField(proto, pdecl);
 } else
 {
  assert(script);
  script_addField(script, sdecl);
 }

 return TRUE;
}

/* Parses a protoStatement */
BOOL parser_protoStatement(struct VRMLParser* me)
{
 indexT name;
 struct ProtoDefinition* obj;

 /* Really a PROTO? */
 if(!lexer_keyword(me->lexer, KW_PROTO))
  return FALSE;

 /* Our name */
 if(!lexer_defineNodeType(me->lexer, &name))
  PARSE_ERROR("Expected nodeTypeId after PROTO!\n")
 assert(name!=ID_UNDEFINED);

 /* Create the object */
 obj=newProtoDefinition();
 if(!PROTOs)
  parser_scopeIn_PROTO();
 assert(PROTOs);
 assert(name==vector_size(PROTOs));
 vector_pushBack(struct ProtoDefinition*, PROTOs,
  obj);

 /* Interface declarations */
 if(!lexer_openSquare(me->lexer))
  PARSE_ERROR("Expected [ to start interface declaration!")
 while(parser_interfaceDeclaration(me, obj, NULL));
 if(!lexer_closeSquare(me->lexer))
  PARSE_ERROR("Expected ] after interface declaration!")

 /* PROTO body */
 if(!lexer_openCurly(me->lexer))
  PARSE_ERROR("Expected { to start PROTO body!")
 parser_scopeIn();
 /* Parse body */
 {
  struct ProtoDefinition* oldCurPROTO=me->curPROTO;
  me->curPROTO=obj;
  while(TRUE)
  {
   {
    vrmlNodeT node;
    if(parser_nodeStatement(me, &node))
    {
     protoDefinition_addNode(obj, node);
     continue;
    }
   }

   if(parser_routeStatement(me))
    continue;
   if(parser_protoStatement(me))
    continue;

   break;
  }
  me->curPROTO=oldCurPROTO;
 }
 parser_scopeOut();
 if(!lexer_closeCurly(me->lexer))
  PARSE_ERROR("Expected } after PROTO body!")

 return TRUE;
}

/* Parses a routeStatement */
BOOL parser_routeStatement(struct VRMLParser* me)
{
 indexT fromNodeIndex;
 struct X3D_Node* fromNode;
 struct ProtoDefinition* fromProto=NULL;
 struct Script* fromScript=NULL;
 indexT fromFieldO;
 indexT fromFieldE;
 indexT fromUFieldO;
 indexT fromUFieldE;
 int fromOfs;
 int fromLen;
 struct ProtoFieldDecl* fromProtoField=NULL;
 struct ScriptFieldDecl* fromScriptField=NULL;

 indexT toNodeIndex;
 struct X3D_Node* toNode;
 struct ProtoDefinition* toProto=NULL;
 struct Script* toScript=NULL;
 indexT toFieldO;
 indexT toFieldE;
 indexT toUFieldO;
 indexT toUFieldE;
 int toOfs;
 int toLen;
 struct ProtoFieldDecl* toProtoField=NULL;
 struct ScriptFieldDecl* toScriptField=NULL;
int temp, tempFE, tempFO, tempTE, tempTO;

 int routingDir;

 assert(me->lexer);
 lexer_skip(me->lexer);

 /* Is this a routeStatement? */
 if(!lexer_keyword(me->lexer, KW_ROUTE))
  return FALSE;

 /* Parse the elements. */
 #define ROUTE_PARSE_NODEFIELD(pre, eventType) \
  /* Target-node */ \
\
  if(!lexer_nodeName(me->lexer, &pre##NodeIndex)) { \
	strcpy (fw_outline,"ERROR:ROUTE: Expected a valid DEF name; found \""); \
	if (me->lexer->curID != NULL) strcat (fw_outline, me->lexer->curID); else strcat (fw_outline, "(EOF)"); \
	strcat (fw_outline,"\" "); \
  	ConsoleMessage(fw_outline);  \
	fprintf (stderr,"%s\n",fw_outline); \
  	PARSER_FINALLY  \
  	return FALSE; \
  } \
  assert(DEFedNodes && !stack_empty(DEFedNodes) && \
   pre##NodeIndex<vector_size(stack_top(struct Vector*, DEFedNodes))); \
  pre##Node=vector_get(struct X3D_Node*, \
   stack_top(struct Vector*, DEFedNodes), \
   pre##NodeIndex); \
  if (pre##Node == NULL) { \
	/* we had a bracket underflow, from what I can see. JAS */ \
	strcpy (fw_outline,"ERROR:ROUTE: no DEF name found - check scoping and \"}\"s"); \
  	ConsoleMessage(fw_outline);  \
	fprintf (stderr,"%s\n",fw_outline); \
  	PARSER_FINALLY  \
  	return FALSE; \
 } else { \
  /* PROTO/Script? */ \
  	switch(pre##Node->_nodeType) \
  	{ \
  	 case NODE_Group: \
  	  pre##Proto=X3D_GROUP(pre##Node)->__protoDef; \
  	  assert(pre##Proto); \
  	  break; \
  	 case NODE_Script: \
  	  pre##Script=((struct X3D_Script*)pre##Node)->__scriptObj; \
  	  assert(pre##Script); \
  	  break; \
  	} \
  } \
  assert(!(pre##Proto && pre##Script)); \
  /* Seperating '.' */ \
  if(!lexer_point(me->lexer)) {\
   	/* PARSE_ERROR("Expected . after node-name!")*/ \
	/* try to make a better error message. */ \
	strcpy (fw_outline,"ERROR:ROUTE: Expected \".\" after the NODE name, found \""); \
	if (me->lexer->curID != NULL) strcat (fw_outline, me->lexer->curID); else strcat (fw_outline, "(EOF)"); \
	strcat (fw_outline,"\" "); \
  	ConsoleMessage(fw_outline);  \
	fprintf (stderr,"%s\n",fw_outline); \
  	PARSER_FINALLY  \
  	return FALSE;  \
  } \
  /* Field, user/built-in depending on whether node is a PROTO instance */ \
  if(!pre##Proto && !pre##Script) \
  { \
   if(!lexer_event##eventType(me->lexer, pre##Node, \
    &pre##FieldO, &pre##FieldE, NULL, NULL))  {\
        /*PARSE_ERROR("Expected built-in event" #eventType " after .!") */ \
	/* try to make a better error message. */ \
	strcpy (fw_outline,"ERROR:ROUTE: Expected event" #eventType " field after \".\" found \""); \
	if (me->lexer->curID != NULL) strcat (fw_outline, me->lexer->curID); else strcat (fw_outline, "(EOF)"); \
	strcat (fw_outline,"\" "); \
  	ConsoleMessage(fw_outline);  \
	fprintf (stderr,"%s\n",fw_outline); \
  	PARSER_FINALLY  \
  	return FALSE;  \
    } \
  } else \
  { \
   assert(pre##Proto || pre##Script); \
   if(lexer_event##eventType(me->lexer, pre##Node, \
    NULL, NULL, &pre##UFieldO, &pre##UFieldE)) \
   { \
    if(pre##UFieldO!=ID_UNDEFINED) \
    { \
     if(pre##Proto) \
     { \
      assert(!pre##Script); \
      pre##ProtoField=protoDefinition_getField(pre##Proto, pre##UFieldO, \
       PKW_event##eventType); \
     } else \
     { \
      assert(pre##Script && !pre##Proto); \
      pre##ScriptField=script_getField(pre##Script, pre##UFieldO, \
       PKW_event##eventType); \
     } \
    } else \
    { \
     if(pre##Proto) \
     { \
      assert(!pre##Script); \
      assert(pre##UFieldE!=ID_UNDEFINED); \
      pre##ProtoField=protoDefinition_getField(pre##Proto, pre##UFieldE, \
       PKW_exposedField); \
     } else \
     { \
      assert(pre##Script && !pre##Proto); \
      pre##ScriptField=script_getField(pre##Script, pre##UFieldE, \
       PKW_exposedField); \
     } \
    } \
    if((pre##Proto && !pre##ProtoField) || (pre##Script && !pre##ScriptField)) \
     PARSE_ERROR("Event-field invalid for this PROTO/Script!") \
   } else \
    PARSE_ERROR("Expected event" #eventType "!") \
  } \
  /* Process script routing */ \
  if(pre##Script) \
  { \
   assert(pre##ScriptField); \
   pre##Node=pre##Script->num; \
   pre##Ofs=scriptFieldDecl_getRoutingOffset(pre##ScriptField); \
  } 

 ROUTE_PARSE_NODEFIELD(from, Out)
 if(!lexer_keyword(me->lexer, KW_TO)) {
	/* try to make a better error message. */
	strcpy (fw_outline,"ERROR:ROUTE: Expected \"TO\" found \"");
	if (me->lexer->curID != NULL) strcat (fw_outline, me->lexer->curID); else strcat (fw_outline, "(EOF)");
	strcat (fw_outline,"\" ");
	if (fromNode != NULL) { strcat (fw_outline, " from type:"); strcat (fw_outline, stringNodeType(fromNode->_nodeType)); strcat (fw_outline, " "); }
	if (fromFieldE != ID_UNDEFINED) { strcat (fw_outline, ":"); strcat (fw_outline, EXPOSED_FIELD[fromFieldE]); strcat (fw_outline, " "); }
	if (fromFieldO != ID_UNDEFINED) { strcat (fw_outline, ":"); strcat (fw_outline, EVENT_OUT[fromFieldO]); strcat (fw_outline, " "); }

        /* PARSE_ERROR("Expected TO in ROUTE-statement!") */
  	ConsoleMessage(fw_outline); 
	fprintf (stderr,"%s\n",fw_outline);
  	PARSER_FINALLY 
  	return FALSE; 
 }
 ROUTE_PARSE_NODEFIELD(to, In)

 /* Now, do the really hard macro work... */
 /* ************************************* */

 /* Ignore the fields. */
 #define FIELD(n, f, t, v)

 #define END_NODE(n) EVENT_END_NODE(n,XFIELD[fromFieldE])

 /* potentially rename the Event_From and Event_To */

 /* REASON: we had a problem with (eg) set_scale in tests/27.wrl. 
    set_scale is a valid field for an Extrusion, so, it was "found". Unfortunately,
    set_scale should be changed to "scale" for an event into a Transform...

    The following code attempts to do this transformation. John Stewart - March 22 2007 */

  /* BTW - Daniel, YES this is SLOW!! It does a lot of string comparisons. But, it does
    seem to work, and it is only SLOW during ROUTE parsing, which (hopefully) there are not
    thousands of. Code efficiency changes more than welcome, from anyone. ;-) */

 tempFE=ID_UNDEFINED; tempFO=ID_UNDEFINED; tempTE=ID_UNDEFINED; tempTO=ID_UNDEFINED;
 if(!fromProtoField && !fromScriptField) {
	/* fromFieldE = Daniel's code thinks this is from an exposedField */
	/* fromFieldO = Daniel's code thinks this is from an eventOut */

	/* first, convert this to a FIELDNAME table index. */
  	if(fromFieldE!=ID_UNDEFINED) {
		/* printf ("step1a, node type %s fromFieldE %d %s\n",stringNodeType(fromNode->_nodeType),fromFieldE,EXPOSED_FIELD[fromFieldE]); */
		tempFE = findRoutedFieldInFIELDNAMES(fromNode,EXPOSED_FIELD[fromFieldE],0);
		if (tempFE != ID_UNDEFINED) tempFE = findFieldInEXPOSED_FIELD(FIELDNAMES[tempFE]);
	}
	if (fromFieldO != ID_UNDEFINED) {
		/* printf ("step2a, node type %s fromFieldO %d %s\n",stringNodeType(fromNode->_nodeType),fromFieldO,EVENT_OUT[fromFieldO]); */
		tempFO = findRoutedFieldInFIELDNAMES(fromNode,EVENT_OUT[fromFieldO],0);
		if (tempFO != ID_UNDEFINED) {
			 tempFO = findFieldInEVENT_OUT(FIELDNAMES[tempFO]);
		}  
		if (tempFO == ID_UNDEFINED) {
			/* hmmm - maybe this is NOW just an exposedField? */
			temp = findRoutedFieldInFIELDNAMES(toNode,EVENT_OUT[toFieldO],0);
			if (temp != ID_UNDEFINED) tempFE = findFieldInEXPOSED_FIELD(FIELDNAMES[temp]);
		}
	}
 }
 if(!toProtoField && !toScriptField) {
	/* toFieldE = Daniel's code thinks this is from an exposedField */
	/* toFieldO = Daniel's code thinks this is from an eventIn */

  	if(toFieldE!=ID_UNDEFINED) {
		/* printf ("step3a, node type %s toFieldE %d %s\n",stringNodeType(toNode->_nodeType),toFieldE,EXPOSED_FIELD[toFieldE]); */
		tempTE = findRoutedFieldInFIELDNAMES(toNode,EXPOSED_FIELD[toFieldE],1);
		if (tempTE != ID_UNDEFINED) tempTE = findFieldInEXPOSED_FIELD(FIELDNAMES[tempTE]);

	}
  	if(toFieldO!=ID_UNDEFINED) {
		/* printf ("step4a, node type %s toFieldO %d %s\n",stringNodeType(toNode->_nodeType),toFieldO,EVENT_IN[toFieldO]); */
		tempTO = findRoutedFieldInFIELDNAMES(toNode,EVENT_IN[toFieldO],1);
		if (tempTO != ID_UNDEFINED) {
			tempTO = findFieldInEVENT_IN(FIELDNAMES[tempTO]);
		}  
		if (tempTO == ID_UNDEFINED) {
			/* hmmm - maybe this is NOW just an exposedField? */
			temp = findRoutedFieldInFIELDNAMES(toNode,EVENT_IN[toFieldO],1);
			if (temp != ID_UNDEFINED) tempTE = findFieldInEXPOSED_FIELD(FIELDNAMES[temp]);
		}
	}
}
/*
printf ("so, before routing we have: ");
if (tempFE != ID_UNDEFINED) {printf ("from EXPOSED_FIELD %s ",EXPOSED_FIELD[tempFE]);}
if (tempFO != ID_UNDEFINED) {printf ("from EVENT_OUT %s ",EVENT_OUT[tempFO]);}
if (tempTE != ID_UNDEFINED) {printf ("to EXPOSED_FIELD %s ",EXPOSED_FIELD[tempTE]);}
if (tempTO != ID_UNDEFINED) {printf ("to EVENT_IN %s ",EVENT_IN[tempTO]);}
printf ("\n\n");
*/

  /* so, lets try and assign what we think we have now... */
  fromFieldE = tempFE;
  fromFieldO = tempFO;
  toFieldE = tempTE;
  toFieldO = tempTO;

 #undef END_NODE 
 #define END_NODE(n) EVENT_END_NODE(n,EXPOSED_FIELD[fromFieldE])

 /* Process from eventOut */
 if(!fromProtoField && !fromScriptField)
  if(fromFieldE!=ID_UNDEFINED) {
   switch(fromNode->_nodeType) {
    #define EVENT_IN(n, f, t, v)
    #define EVENT_OUT(n, f, t, v)
    #define EXPOSED_FIELD(node, field, type, var) \
     PROCESS_EVENT(EXPOSED_FIELD, from, node, field, type, var)
    #define BEGIN_NODE(node) \
     EVENT_BEGIN_NODE(fromFieldE, fromNode, node)
    #include "NodeFields.h"
    #undef EVENT_IN
    #undef EVENT_OUT
    #undef EXPOSED_FIELD
    #undef BEGIN_NODE
    EVENT_NODE_DEFAULT
   }

 #undef END_NODE 
 #define END_NODE(n) EVENT_END_NODE(n,EVENT_OUT[fromFieldO])

  } else if(fromFieldO!=ID_UNDEFINED) {
   switch(fromNode->_nodeType) {
    #define EVENT_IN(n, f, t, v)
    #define EXPOSED_FIELD(n, f, t, v)
    #define EVENT_OUT(node, field, type, var) \
     PROCESS_EVENT(EVENT_OUT, from, node, field, type, var)
    #define BEGIN_NODE(node) \
     EVENT_BEGIN_NODE(fromFieldO, fromNode, node)
    #include "NodeFields.h"
    #undef EVENT_IN
    #undef EVENT_OUT
    #undef EXPOSED_FIELD
    #undef BEGIN_NODE
    EVENT_NODE_DEFAULT
   }
  }


 #undef END_NODE 
 #define END_NODE(n) EVENT_END_NODE(n,EXPOSED_FIELD[toFieldE])

 /* Process to eventIn */
 if(!toProtoField && !toScriptField)
  if(toFieldE!=ID_UNDEFINED) {
   switch(toNode->_nodeType) {
    #define EVENT_IN(n, f, t, v)
    #define EVENT_OUT(n, f, t, v)
    #define EXPOSED_FIELD(node, field, type, var) \
     PROCESS_EVENT(EXPOSED_FIELD, to, node, field, type, var)
    #define BEGIN_NODE(node) \
     EVENT_BEGIN_NODE(toFieldE, toNode, node)
    #include "NodeFields.h"
    #undef EVENT_IN
    #undef EVENT_OUT
    #undef EXPOSED_FIELD
    #undef BEGIN_NODE
    EVENT_NODE_DEFAULT
   }

 #undef END_NODE 
 #define END_NODE(n) EVENT_END_NODE(n,EVENT_IN[toFieldO])

  } else if(toFieldO!=ID_UNDEFINED) {
   switch(toNode->_nodeType) {
    #define EVENT_OUT(n, f, t, v)
    #define EXPOSED_FIELD(n, f, t, v)
    #define EVENT_IN(node, field, type, var) \
     PROCESS_EVENT(EVENT_IN, to, node, field, type, var)
    #define BEGIN_NODE(node) \
     EVENT_BEGIN_NODE(toFieldO, toNode, node)
    #include "NodeFields.h"
    #undef EVENT_IN
    #undef EVENT_OUT
    #undef EXPOSED_FIELD
    #undef BEGIN_NODE
    EVENT_NODE_DEFAULT
   }
  }

 /* Clean up. */
 #undef FIELD
 #undef END_NODE

 /* Update length for fields */
 if(fromProtoField)
  fromLen=protoFieldDecl_getLength(fromProtoField);
 else if(fromScriptField)
  fromLen=scriptFieldDecl_getLength(fromScriptField);
 if(toProtoField)
  toLen=protoFieldDecl_getLength(toProtoField);
 else if(toScriptField)
  toLen=scriptFieldDecl_getLength(toScriptField);

 /* FIXME:  Not a really safe check for types in ROUTE! */
 /* JAS - made message better. Should compare types, not lengths. */
 if(fromLen!=toLen) {
	/* try to make a better error message. */
	strcpy (fw_outline,"ERROR:Types mismatch in ROUTE: ");
	if (fromNode != NULL) { strcat (fw_outline, " from type:"); strcat (fw_outline, stringNodeType(fromNode->_nodeType)); strcat (fw_outline, " "); }
	if (fromFieldE != ID_UNDEFINED) { strcat (fw_outline, ":"); strcat (fw_outline, EXPOSED_FIELD[fromFieldE]); strcat (fw_outline, " "); }
	if (fromFieldO != ID_UNDEFINED) { strcat (fw_outline, ":"); strcat (fw_outline, EVENT_OUT[fromFieldO]); strcat (fw_outline, " "); }

	if (toNode != NULL) { strcat (fw_outline, " to type:"); strcat (fw_outline, stringNodeType(toNode->_nodeType)); strcat (fw_outline, " "); }
	if (toFieldE != ID_UNDEFINED) { strcat (fw_outline, ":"); strcat (fw_outline, EXPOSED_FIELD[toFieldE]); strcat (fw_outline, " "); }
	if (toFieldO != ID_UNDEFINED) { strcat (fw_outline, ":"); strcat (fw_outline, EVENT_IN[toFieldO]); strcat (fw_outline, " "); }

  	/* PARSE_ERROR(fw_outline) */
  	ConsoleMessage(fw_outline); 
	fprintf (stderr,"%s\n",fw_outline);
  	PARSER_FINALLY 
  	return FALSE; 
  }

 /* Finally, register the route. */
 /* **************************** */

 /* Calculate dir parameter */
 if(fromScript && toScript)
  routingDir=SCRIPT_TO_SCRIPT;
 else if(fromScript)
 {
  assert(!toScript);
  routingDir=FROM_SCRIPT;
 } else if(toScript)
 {
  assert(!fromScript);
  routingDir=TO_SCRIPT;
 } else
 {
  assert(!fromScript && !toScript);
  routingDir=0;
 }

 /* Built-in to built-in */
 if(!fromProtoField && !toProtoField)
  parser_registerRoute(me, fromNode, fromOfs, toNode, toOfs, toLen, routingDir);
 /* Built-in to user-def */
 else if(!fromProtoField && toProtoField)
  protoFieldDecl_routeTo(toProtoField, fromNode, fromOfs, routingDir, me);
 /* User-def to built-in */
 else if(fromProtoField && !toProtoField)
  protoFieldDecl_routeFrom(fromProtoField, toNode, toOfs, routingDir, me);
 /* User-def to user-def */
 else
  PARSE_ERROR("Routing from user-event to user-event is currently unsupported!")

 return TRUE;
}

/* Register a ROUTE here */
void parser_registerRoute(struct VRMLParser* me,
 struct X3D_Node* fromNode, unsigned fromOfs,
 struct X3D_Node* toNode, unsigned toOfs,
 size_t len, int dir)
{
 assert(me);
 if(me->curPROTO)
 {
  protoDefinition_addRoute(me->curPROTO,
   newProtoRoute(fromNode, fromOfs, toNode, toOfs, len, dir));
 } else
  CRoutes_RegisterSimple(fromNode, fromOfs, toNode, toOfs, len, dir);
}

/* Parses a nodeStatement */
BOOL parser_nodeStatement(struct VRMLParser* me, vrmlNodeT* ret)
{
 assert(me->lexer);

 /* A DEF-statement? */
 if(lexer_keyword(me->lexer, KW_DEF))
 {
  indexT ind;

  if(!lexer_defineNodeName(me->lexer, &ind))
   PARSE_ERROR("Expected nodeNameId after DEF!\n")
  assert(ind!=ID_UNDEFINED);

  if(!DEFedNodes || stack_empty(DEFedNodes))
   parser_scopeIn_DEFUSE();
  assert(DEFedNodes);
  assert(!stack_empty(DEFedNodes));

  assert(ind<=vector_size(stack_top(struct Vector*, DEFedNodes)));
  if(ind==vector_size(stack_top(struct Vector*, DEFedNodes)))
   vector_pushBack(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes),
    NULL);
  assert(ind<vector_size(stack_top(struct Vector*, DEFedNodes)));

  vrmlNodeT node;
  if(!parser_node(me, &node)) {
   	/* PARSE_ERROR("Expected node in DEF statement!\n") */
	/* try to make a better error message. */
	strcpy (fw_outline,"ERROR:Expected an X3D node in a DEF statement, got \"");
	if (me->lexer->curID != NULL) strcat (fw_outline, me->lexer->curID); else strcat (fw_outline, "(EOF)");
	strcat (fw_outline,"\" ");
  	ConsoleMessage(fw_outline); 
	fprintf (stderr,"%s\n",fw_outline);
  	PARSER_FINALLY 
  	return FALSE; 
}
  vector_get(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes), ind)=node;

  /*
  if(!parser_node(me, &vector_get(struct X3D_Node*,
   stack_top(struct Vector*, DEFedNodes), ind)))
   PARSE_ERROR("Expected node in DEF statement!\n")
  */

  *ret=vector_get(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes), ind);
  return TRUE;
 }

 /* A USE-statement? */
 if(lexer_keyword(me->lexer, KW_USE))
 {
  indexT ind;

  if(!lexer_nodeName(me->lexer, &ind)) {
   PARSE_ERROR("Expected nodeNameId after USE!\n")
	}
  assert(ind!=ID_UNDEFINED);

  assert(DEFedNodes && !stack_empty(DEFedNodes) &&
   ind<vector_size(stack_top(struct Vector*, DEFedNodes)));

  *ret=vector_get(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes), ind);
  return TRUE;
 }

 /* Otherwise, simply a node. */
 return parser_node(me, ret);
}

/* Parses a node (node non-terminal) */
BOOL parser_node(struct VRMLParser* me, vrmlNodeT* ret)
{
 indexT nodeTypeB, nodeTypeU;
 struct X3D_Node* node=NULL;

 assert(me->lexer);
 
 if(!lexer_node(me->lexer, &nodeTypeB, &nodeTypeU))
  return FALSE;

 if(!lexer_openCurly(me->lexer))
  PARSE_ERROR("Expected { after node-type id!")

 /* Built-in node */
 if(nodeTypeB!=ID_UNDEFINED)
 {
  struct Script* script=NULL;
 
  node=X3D_NODE(createNewX3DNode(nodeTypeB));

  assert(node);
  /* Node specific initialization */
  parser_specificInitNode(node);

  /* Set curScript for Script-nodes */
  if(node->_nodeType==NODE_Script)
   script=X3D_SCRIPT(node)->__scriptObj;

  while(TRUE)
  {
   if(parser_field(me, node)) continue;
   if(parser_routeStatement(me)) continue;
   if(parser_protoStatement(me)) continue;
   if(script && parser_interfaceDeclaration(me, NULL, script)) continue;
   break;
  }

  /* Init code for Scripts. if this script is within a PROTO, initiate it when the PROTO
     is instantiated. */
  if(script) {
	if (!me->curPROTO)  {
		/* printf ("parser_node, proto %d, going to init script\n",me->curPROTO); */
	   	script_initCodeFromMFUri(script, &X3D_SCRIPT(node)->url);
	} else {
		/* printf ("in parser_node, have script and me->curPROTO not null\n"); */
		vector_pushBack(struct X3D_Script*, me->curPROTO->scripts, node);
		/* printf ("added this script %d to the scripts field of this proto\n",node); */
	}
   }
 }
 
 /* Proto */
 else
 {
  /* The copy of our ProtoDefinition; here are the fields filled in. */
  struct ProtoDefinition* protoCopy;
	int i;

  assert(nodeTypeU!=ID_UNDEFINED);
  assert(PROTOs);
  assert(nodeTypeU<vector_size(PROTOs));

  protoCopy=protoDefinition_copy(vector_get(struct ProtoDefinition*,
   PROTOs, nodeTypeU));

  while(parser_protoField(me, protoCopy) ||
   parser_routeStatement(me) || parser_protoStatement(me));

  node=protoDefinition_extractScene(protoCopy);

  /* do the script interface and initialization */
  /* printf ("extracting scripts in protoDefinition_extractScene size %d\n",vector_size(protoCopy->scripts)); */
  for(i=0; i!=vector_size(protoCopy->scripts); ++i) {
        struct X3D_Script *scr;
        scr = (struct Script *)vector_get(struct X3D_Script*, protoCopy->scripts,i);
        /* printf ("HAVE Scropt = script %d, script number %d\n",scr,((struct Script *) (scr->__scriptObj))->num); */
        registerScriptInPROTO((struct X3D_Node *)scr,protoCopy);
  }


  assert(node);

  /* Can't delete ProtoDefinition, it is referenced! */
  /*deleteProtoDefinition(protoCopy);*/
 }

 assert(node);

 if(!lexer_closeCurly(me->lexer)) {
  	/* parseError("Expected } after fields of node!"); */
	/* try to make a better error message. */
	strcpy (fw_outline,"ERROR: Expected \"}\" after fields of a node; found \"");
	if (me->lexer->curID != NULL) strcat (fw_outline, me->lexer->curID); else strcat (fw_outline, "(EOF)");
	strcat (fw_outline,"\" ");
  	ConsoleMessage(fw_outline); 
	fprintf (stderr,"%s\n",fw_outline);
  	PARSER_FINALLY 
  	return FALSE; 
 }

 *ret=node;

 return TRUE;
}

/* Parses a field assignment of a PROTOtyped node */
BOOL parser_protoField(struct VRMLParser* me, struct ProtoDefinition* p)
{
 indexT fieldO, fieldE;
 struct ProtoFieldDecl* field=NULL;

 if(!lexer_field(me->lexer, NULL, NULL, &fieldO, &fieldE))
  return FALSE;

 if(fieldO!=ID_UNDEFINED)
 {
  field=protoDefinition_getField(p, fieldO, PKW_field);
  if(!field)
   PARSE_ERROR("Field is not part of PROTO's interface!")
  assert(field->mode==PKW_field);
 } else
 {
  assert(fieldE!=ID_UNDEFINED);
  field=protoDefinition_getField(p, fieldE, PKW_exposedField);
  if(!field)
   PARSE_ERROR("Field is not part of PROTO's interface!")
  assert(field->mode==PKW_exposedField);
 }
 assert(field);

 /* Parse the value */
 {
  union anyVrml val;
  if(!parser_fieldValue(me, newOffsetPointer(&val, 0), field->type,
   ID_UNDEFINED))
   PARSE_ERROR("Expected value of field after fieldId!")
  protoFieldDecl_setValue(field, &val);
 }

 return TRUE;
}

/* add_parent for Multi_Node */
void mfnode_add_parent(struct Multi_Node* node, struct X3D_Node* parent)
{
 int i;
 for(i=0; i!=node->n; ++i)
  add_parent(node->p[i], parent);
}

/* Parses a field value (literally or IS) */
BOOL parser_fieldValue(struct VRMLParser* me, struct OffsetPointer* ret,
 indexT type, indexT origFieldE)
{
 #undef PARSER_FINALLY
 #define PARSER_FINALLY \
  deleteOffsetPointer(ret);

 /* If we are inside a PROTO, IS is possible */
 if(me->curPROTO && lexer_keyword(me->lexer, KW_IS))
 {
  indexT fieldO, fieldE;
  struct ProtoFieldDecl* pField=NULL;

  /* Try event */
  if(origFieldE!=ID_UNDEFINED &&
   parser_fieldEventAfterISPart(me, ret->node, TRUE, TRUE, ID_UNDEFINED,
    origFieldE))
   return TRUE;

  if(!lexer_field(me->lexer, NULL, NULL, &fieldO, &fieldE))
   PARSE_ERROR("Expected fieldId after IS!")

  if(fieldO!=ID_UNDEFINED)
  {
   pField=protoDefinition_getField(me->curPROTO, fieldO, PKW_field);
   if(!pField)
    PARSE_ERROR("IS source is no field of current PROTO!")
   assert(pField->mode==PKW_field);
  } else
  {
   assert(fieldE!=ID_UNDEFINED);
   pField=protoDefinition_getField(me->curPROTO, fieldE, PKW_exposedField);
   if(!pField)
    PARSE_ERROR("IS source is no field of current PROTO!")
   assert(pField->mode==PKW_exposedField);
  }
  assert(pField);

  /* Check type */
  if(pField->type!=type)
   PARSE_ERROR("Types mismatch for PROTO field!")

  /* Don't set field for now but register us as users of this PROTO field */
  protoFieldDecl_addDestinationOptr(pField, ret);

  return TRUE;
 }

 /* Otherwise */
 {
  void* directRet=offsetPointer_deref(void*, ret);
  PARSER_FINALLY
  return PARSE_TYPE[type](me, directRet);
 }

 #undef PARSER_FINALLY
 #define PARSER_FINALLY
}

/* Specific initialization of node fields */
void parser_specificInitNode(struct X3D_Node* n)
{
 switch(n->_nodeType)
 {

  #define NODE_SPECIFIC_INIT(type, code) \
   case NODE_##type: \
   { \
    struct X3D_##type* node=(struct X3D_##type*)n; \
    code \
   }

  /* Scripts get a script object associated to them */
  NODE_SPECIFIC_INIT(Script,
   node->__scriptObj=newScript();
   )

 }
}

/* ************************************************************************** */
/* Built-in fields */

/* Parses a built-in field and sets it in node */
BOOL parser_field(struct VRMLParser* me, struct X3D_Node* node)
{
 indexT fieldO;
 indexT fieldE;

 assert(me->lexer);
 if(!lexer_field(me->lexer, &fieldO, &fieldE, NULL, NULL))
  return parser_fieldEvent(me, node);

 /* Ignore all events */
 #define EVENT_IN(n, f, t, v)
 #define EVENT_OUT(n, f, t, v)

 /* End of node is the same for fields and exposedFields */
 #define END_NODE(type) \
   } \
  } \
  break;

 /* The init codes used. */
 #define INIT_CODE_sfnode(var) \
  add_parent(node2->var, node2);
 #define INIT_CODE_mfnode(var) \
  mfnode_add_parent(&node2->var, X3D_NODE(node2));
 #define INIT_CODE_sfbool(var)
 #define INIT_CODE_sfcolor(var)
 #define INIT_CODE_sfcolorrgba(var)
 #define INIT_CODE_sffloat(var)
 #define INIT_CODE_sfimage(var)
 #define INIT_CODE_sfint32(var)
 #define INIT_CODE_sfrotation(var)
 #define INIT_CODE_sfstring(var)
 #define INIT_CODE_sftime(var)
 #define INIT_CODE_sfvec2f(var)
 #define INIT_CODE_sfvec3f(var)
 #define INIT_CODE_mfbool(var)
 #define INIT_CODE_mfcolor(var)
 #define INIT_CODE_mfcolorrgba(var)
 #define INIT_CODE_mffloat(var)
 #define INIT_CODE_mfint32(var)
 #define INIT_CODE_mfrotation(var)
 #define INIT_CODE_mfstring(var)
 #define INIT_CODE_mftime(var)
 #define INIT_CODE_mfvec2f(var)
 #define INIT_CODE_mfvec3f(var)

 /* The field type indices */
 #define FTIND_sfnode	FIELDTYPE_SFNode
 #define FTIND_mfnode	FIELDTYPE_MFNode
 #define FTIND_sfbool	FIELDTYPE_SFBool
 #define FTIND_sfcolor	FIELDTYPE_SFColor
 #define FTIND_sfcolorrgba	FIELDTYPE_SFColorRGBA
 #define FTIND_sffloat	FIELDTYPE_SFFloat
 #define FTIND_sfimage	FIELDTYPE_SFImage
 #define FTIND_sfint32	FIELDTYPE_SFInt32
 #define FTIND_sfrotation	FIELDTYPE_SFRotation
 #define FTIND_sfstring	FIELDTYPE_SFString
 #define FTIND_sftime	FIELDTYPE_SFTime
 #define FTIND_sfvec2f	FIELDTYPE_SFVec2f
 #define FTIND_sfvec3f	FIELDTYPE_SFVec3f
 #define FTIND_mfbool	FIELDTYPE_MFBool
 #define FTIND_mfcolor	FIELDTYPE_MFColor
 #define FTIND_mfcolorrgba	FIELDTYPE_MFColorRGBA
 #define FTIND_mffloat	FIELDTYPE_MFFloat
 #define FTIND_mfint32	FIELDTYPE_MFInt32
 #define FTIND_mfrotation	FIELDTYPE_MFRotation
 #define FTIND_mfstring	FIELDTYPE_MFString
 #define FTIND_mftime	FIELDTYPE_MFTime
 #define FTIND_mfvec2f	FIELDTYPE_MFVec2f
 #define FTIND_mfvec3f	FIELDTYPE_MFVec3f
 
 /* Process a field (either exposed or ordinary) generally */
 #define PROCESS_FIELD(exposed, node, field, fieldType, var, fe) \
  case exposed##FIELD_##field: \
   if(!parser_fieldValue(me, \
    newOffsetPointer(X3D_NODE(node2), offsetof(struct X3D_##node, var)), \
    FTIND_##fieldType, fe)) \
    PARSE_ERROR("Expected " #fieldType "Value!") \
   INIT_CODE_##fieldType(var) \
   return TRUE;
 
 /* Default action if node is not encountered in list of known nodes */
 #define NODE_DEFAULT \
  default: \
   PARSE_ERROR("Unsupported node!")

 /* Try exposed field */
 /* ***************** */
 if(fieldE!=ID_UNDEFINED)
  switch(node->_nodeType)
  {

   /* Processes exposed fields for node */
   #define BEGIN_NODE(type) \
    case NODE_##type: \
    { \
     struct X3D_##type* node2=(struct X3D_##type*)node; \
     switch(fieldE) \
     {

   /* Process exposed fields */
   #define EXPOSED_FIELD(node, field, fieldType, var) \
    PROCESS_FIELD(EXPOSED_, node, field, fieldType, var, fieldE)

   /* Ignore just fields */
   #define FIELD(n, f, t, v)

   /* Process it */
   #include "NodeFields.h"

   /* Undef the field-specific macros */
   #undef BEGIN_NODE
   #undef FIELD
   #undef EXPOSED_FIELD

   NODE_DEFAULT

  }

 /* Ordnary field */
 /* ************* */
 if(fieldO!=ID_UNDEFINED)
  switch(node->_nodeType)
  {

   /* Processes ordinary fields for node */
   #define BEGIN_NODE(type) \
    case NODE_##type: \
    { \
     struct X3D_##type* node2=(struct X3D_##type*)node; \
     switch(fieldO) \
     {

   /* Process fields */
   #define FIELD(node, field, fieldType, var) \
    PROCESS_FIELD(, node, field, fieldType, var, ID_UNDEFINED)

   /* Ignore exposed fields */
   #define EXPOSED_FIELD(n, f, t, v)

   /* Process it */
   #include "NodeFields.h"

   /* Undef the field-specific macros */
   #undef BEGIN_NODE
   #undef FIELD
   #undef EXPOSED_FIELD

   NODE_DEFAULT
      
  }

 /* Clean up */
 #undef END_NODE
 #undef EVENT_IN
 #undef EVENT_OUT

 /* If field was found, return TRUE; would have happened! */
 PARSE_ERROR("Unsupported field for node!")
}

/* Parses a built-in field with IS in PROTO */
BOOL parser_fieldEvent(struct VRMLParser* me, struct X3D_Node* ptr)
{
 BOOL isIn=FALSE, isOut=FALSE;
 indexT evO, evE;

 /* We should be in a PROTO */
 if(!me->curPROTO)
  return FALSE;

 /* There should be really an eventIn or an eventOut...
  * In case of exposedField, both is true, but evE should be the same (and is
  * therefore not overridden). */
 if(lexer_eventIn(me->lexer, ptr, &evO, &evE, NULL, NULL))
 {
  isIn=TRUE;
  /* When exposedField, out is allowed, too */
  isOut=(evE!=ID_UNDEFINED);
 } else if(lexer_eventOut(me->lexer, ptr, &evO, &evE, NULL, NULL))
  isOut=TRUE;

 if(!isIn && !isOut)
  return FALSE;

#ifndef NDEBUG
 if(isIn && isOut)
  assert(evE!=ID_UNDEFINED);
#endif

 /* Then, there should be 'IS' */
 if(!lexer_keyword(me->lexer, KW_IS))
  return FALSE;
  
 return parser_fieldEventAfterISPart(me, ptr, isIn, isOut, evO, evE);
}

/* Parses only the after-IS-part of a PROTO event */
BOOL parser_fieldEventAfterISPart(struct VRMLParser* me, struct X3D_Node* ptr,
 BOOL isIn, BOOL isOut, indexT evO, indexT evE)
{
 indexT pevO, pevE;
 struct ProtoFieldDecl* pfield=NULL;
 unsigned myOfs;
 size_t myLen;
 BOOL pevFound=FALSE;

 /* Check and gain information */
 /* ************************** */

 /* We should be in a PROTO */
 if(!me->curPROTO)
  return FALSE;

 /* And finally the PROTO's event to be linked */
 if(isIn && lexer_eventIn(me->lexer, ptr, NULL, NULL, &pevO, &pevE))
  pevFound=TRUE;
 else if(isOut && lexer_eventOut(me->lexer, ptr, NULL, NULL, &pevO, &pevE))
  pevFound=TRUE;
 if(!pevFound)
  return FALSE;

 /* Now, retrieve the ProtoFieldDecl. */
 if(pevE!=ID_UNDEFINED)
 {
  pfield=protoDefinition_getField(me->curPROTO, pevE, PKW_exposedField);
  if(!pfield)
   PARSE_ERROR("This exposedField is not member of current PROTO!")
 } else if(pevO!=ID_UNDEFINED)
 {
  assert(!pfield);
  pfield=protoDefinition_getField(me->curPROTO, pevO,
   isIn ? PKW_eventIn : PKW_eventOut);
  if(!pfield)
   PARSE_ERROR("This event is not member of current PROTO!")
 }
 assert(pfield);

 /* Register the link by some macros */
 /* ******************************** */

 /* Ignore fields */
 #define FIELD(n, f, t, v)

 /* Basics */
 #define END_NODE(n) EVENT_END_NODE(n,EXPOSED_FIELD[evE])

 /* exposedField */
 if(evE!=ID_UNDEFINED)
 {
  #define EVENT_IN(n, f, t, v)
  #define EVENT_OUT(n, f, t, v)
  #define EXPOSED_FIELD(n, f, t, v) \
   PROCESS_EVENT(EXPOSED_FIELD, my, n, f, t, v)
  #define BEGIN_NODE(n) \
   EVENT_BEGIN_NODE(evE, ptr, n)

  switch(ptr->_nodeType)
  {
   #include "NodeFields.h"
   EVENT_NODE_DEFAULT
  }

  #undef EVENT_IN
  #undef EVENT_OUT
  #undef EXPOSED_FIELD
  #undef BEGIN_NODE
 } else
 {
  assert(evO!=ID_UNDEFINED);
  assert((isIn || isOut) && !(isIn && isOut));

  #define BEGIN_NODE(n) \
   EVENT_BEGIN_NODE(evO, ptr, n)
  #define EXPOSED_FIELD(n, f, t, v)

 #undef END_NODE
 #define END_NODE(n) EVENT_END_NODE(n,EVENT_IN[evO])

  
  /* eventIn */
  if(isIn)
  {
   #define EVENT_OUT(n, f, t, v)
   #define EVENT_IN(n, f, t, v) \
    PROCESS_EVENT(EVENT_IN, my, n, f, t, v)

   switch(ptr->_nodeType)
   {
    #include "NodeFields.h"
    EVENT_NODE_DEFAULT
   }

   #undef EVENT_IN
   #undef EVENT_OUT
  }


 #undef END_NODE
 #define END_NODE(n) EVENT_END_NODE(n,EVENT_OUT[evO])

  /* eventOut */
  else if(isOut)
  {
   #define EVENT_IN(n, f, t, v)
   #define EVENT_OUT(n, f, t, v) \
    PROCESS_EVENT(EVENT_OUT, my, n, f, t, v)

   switch(ptr->_nodeType)
   {
    #include "NodeFields.h"
    EVENT_NODE_DEFAULT
   }

   #undef EVENT_IN
   #undef EVENT_OUT
  }

  #undef BEGIN_NODE
  #undef EXPOSED_FIELD
 }
 
 /* Clean up */
 #undef FIELD
 #undef END_NODE

 /* Link it */
 protoFieldDecl_addDestination(pfield, ptr, myOfs);

 return TRUE;
}

/* ************************************************************************** */
/* MF* field values */

/* Parse a MF* field */
#define PARSER_MFFIELD(name, type) \
 BOOL parser_mf##name##Value(struct VRMLParser* me, \
  struct Multi_##type* ret) \
 { \
  struct Vector* vec=NULL; \
  \
  /* Just a single value? */ \
  if(!lexer_openSquare(me->lexer)) \
  { \
   ret->p=MALLOC(sizeof(vrml##type##T)); \
   if(!parser_sf##name##Value(me, (void*)ret->p)) \
    return FALSE; \
   ret->n=1; \
   return TRUE; \
  } \
  \
  /* Otherwise, a real vector */ \
  vec=newVector(vrml##type##T, 128); \
  while(!lexer_closeSquare(me->lexer)) \
  { \
   vrml##type##T val; \
   \
   if(!parser_sf##name##Value(me, &val)) \
   { \
        /* parseError("Expected ] before end of MF-Value!"); */ \
	strcpy (fw_outline,"ERROR:Expected \"]\" before end of MF-Value, found \""); \
	if (me->lexer->curID != NULL) strcat (fw_outline, me->lexer->curID); else strcat (fw_outline, "(EOF)"); \
	strcat (fw_outline,"\" "); \
  	ConsoleMessage(fw_outline); \
	fprintf (stderr,"%s\n",fw_outline);\
    break; \
   } \
   vector_pushBack(vrml##type##T, vec, val); \
  } \
  \
  ret->n=vector_size(vec); \
  ret->p=vector_releaseData(vrml##type##T, vec); \
  \
  deleteVector(vrml##type##T, vec); \
  return TRUE; \
 } 

PARSER_MFFIELD(bool, Bool)
PARSER_MFFIELD(color, Color)
PARSER_MFFIELD(colorrgba, ColorRGBA)
PARSER_MFFIELD(float, Float)
PARSER_MFFIELD(int32, Int32)
PARSER_MFFIELD(node, Node)
PARSER_MFFIELD(rotation, Rotation)
PARSER_MFFIELD(string, String)
PARSER_MFFIELD(time, Time)
PARSER_MFFIELD(vec2f, Vec2f)
PARSER_MFFIELD(vec3f, Vec3f)

/* ************************************************************************** */
/* SF* field values */

/* Parses a fixed-size vector-field of floats (SFColor, SFRotation, SFVecXf) */
#define PARSER_FIXED_VEC(name, type, cnt, dest) \
 BOOL parser_sf##name##Value(struct VRMLParser* me, vrml##type##T* ret) \
 { \
  int i; \
  assert(me->lexer); \
  for(i=0; i!=cnt; ++i) {\
   if(!parser_sffloatValue(me, ret->dest+i)) \
    return FALSE; \
	/* printf ("PARSER_FIXED_VEC, just read in %f\n",*(ret->dest+i)); */\
	} \
  return TRUE; \
 }

BOOL parser_sffloatValue_(struct VRMLParser* me, vrmlFloatT* ret)
{
 return lexer_float(me->lexer, ret);
}
BOOL parser_sfint32Value_(struct VRMLParser* me, vrmlInt32T* ret)
{
 return lexer_int32(me->lexer, ret);
}
BOOL parser_sfstringValue_(struct VRMLParser* me, vrmlStringT* ret)
{
 return lexer_string(me->lexer, ret);
}

BOOL parser_sfboolValue(struct VRMLParser* me, vrmlBoolT* ret)
{
 if(lexer_keyword(me->lexer, KW_TRUE))
 {
  *ret=TRUE;
  return TRUE;
 }
 if(lexer_keyword(me->lexer, KW_FALSE))
 {
  *ret=FALSE;
  return TRUE;
 }

 return FALSE;
}

PARSER_FIXED_VEC(color, Color, 3, c)
PARSER_FIXED_VEC(colorrgba, ColorRGBA, 4, r)

/* JAS this code assumes that the ret points to a SFInt_32 type, and just
fills in the values. */
 
BOOL parser_sfimageValue(struct VRMLParser* me, vrmlImageT* ret)
{
 vrmlInt32T width, height, depth;
 vrmlInt32T* ptr;
 
 if(!lexer_int32(me->lexer, &width))
  return FALSE;
 if(!lexer_int32(me->lexer, &height))
  return FALSE;
 if(!lexer_int32(me->lexer, &depth))
  return FALSE;


 ret->n=3+width*height;
 ret->p=MALLOC(sizeof(int) * ret->n);
 ret->p[0]=width;
 ret->p[1]=height;
 ret->p[2]=depth;

 for(ptr=ret->p+3; ptr!=ret->p+ret->n; ++ptr)
  if(!lexer_int32(me->lexer, ptr))
  {
   FREE_IF_NZ(ret->p);
   ret->n=0;
   return FALSE;
  }

 return TRUE;
}

BOOL parser_sfnodeValue(struct VRMLParser* me, vrmlNodeT* ret)
{
 assert(me->lexer);
 if(lexer_keyword(me->lexer, KW_NULL))
 {
  *ret=NULL;
  return TRUE;
 }

 return parser_nodeStatement(me, ret);
}

PARSER_FIXED_VEC(rotation, Rotation, 4, r)

BOOL parser_sftimeValue(struct VRMLParser* me, vrmlTimeT* ret)
{
 vrmlFloatT f;

 assert(me->lexer);
 if(!lexer_float(me->lexer, &f))
  return FALSE;

 *ret=(vrmlTimeT)f;
 return TRUE;
}

PARSER_FIXED_VEC(vec2f, Vec2f, 2, c)
