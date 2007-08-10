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

/* PROTOs definitions  */
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

/* PROTOs are scope by the parser in the PROTOs vector, and by the lexer in the userNodeTypesVec vector.  
   This is accomplished by keeping track of the number of PROTOs defined so far in a the userNodeTypesStack.  
   When a new scope is entered, the number of PROTOs defined up to this point is pushed onto the stack.  When
   we leave the scope the number of PROTOs now defined is compared to the top value of the stack, and the newest
   PROTOs are removed until the number of PROTOs defined equals the top value of the stack.

   Management of the userNodeTypesStack is accomplished by the lexer.  Therefore, scoping in PROTOs for the parser
   does nothing except to make sure that the PROTOs vector has been initialized. */ 
static void parser_scopeIn_PROTO()
{
 if(!PROTOs) {
  	PROTOs=newVector(struct ProtoDefinition*, DEFMEM_INIT_SIZE);
  }
}

static void parser_scopeOut_DEFUSE()
{
 indexT i;
 assert(!stack_empty(DEFedNodes));
 /* FIXME:  Can't delete individual nodes, as they might be referenced! */
 deleteVector(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes));
 stack_pop(struct Vector*, DEFedNodes);
}

/* Scoping out of PROTOs.  Check the difference between the number of PROTO definitions currently 
   available and the number of PROTO definitions available when we first entered this scope (this is
   the top value on the userNodeTypesVec stack). Remove all PROTO definitions from the PROTOs list that have 
   been added since we first entered this scope. */ 
static void parser_scopeOut_PROTO()
{
 indexT i;
 /* Do not delete the ProtoDefinitions, as they are referenced in the scene
  * graph!  TODO:  How to delete them properly? */

 vector_popBackN(struct ProtoDefinition*, PROTOs, lexer_getProtoPopCnt());
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
 /* As long as there are nodes, routes, or protos to be parsed keep doing so */
 while(TRUE)
 {
  /* Try nodeStatement */
  {
   vrmlNodeT node;
  /* This will parse a builtin node, including all nested ROUTES and PROTOs, and return
     a pointer to the node that was parsed. 
     If the node is a user-defined node (PROTO expansion) this will expand the PROTO (propagate
     all field values, and add all routes to the CRoute table), and returns a pointer to the
     root node of the scene graph for this PROTO */
#ifdef CPARSERVERBOSE
   printf("parser_vrmlScene: Try node\n");
#endif
   if(parser_nodeStatement(me, &node))
   {
    /* Add the node just parsed to the ROOT node for this scene */
    addToNode(me->ptr, me->ofs, node);
#ifdef CPARSERVERBOSE
    printf("parser_vrmlScene: node parsed\n");
#endif
    continue;
   }
  }

  /* Try routeStatement */
  /* Checks that the ROUTE statement is valid (i.e. that the referenced node and field combinations
   exist, and that they are compatible) and then adds the route to the CRoutes table of routes. */

#ifdef CPARSERVERBOSE
   printf("parser_vrmlScene: Try route\n");
#endif
  if(parser_routeStatement(me)) {
#ifdef CPARSERVERBOSE
    printf("parser_vrmlScene: route parsed\n");
#endif
    continue;
  }

  /* Try protoStatement */
   /* Add the PROTO name to the userNodeTypesVec list of names.  Create and fill in a new protoDefinition structure and add it to the PROTOs list.
      Goes through the interface declarations for the PROTO and adds each user-defined field name to the appropriate list of user-defined names (user_field,
      user_eventIn, user_eventOut, or user_exposedField), creates a new protoFieldDecl for the field and adds it to the iface vector of the ProtoDefinition,
      and, in the case of fields and exposedFields, gets the default value of the field and stores it in the protoFieldDecl.
      Parses the body of the PROTO.  Nodes are added to the scene graph for this PROTO.  Routes are parsed and a new ProtoRoute structure
      is created for each one and added to the routes vector of the ProtoDefinition.  PROTOs are recursively parsed!
    */
#ifdef CPARSERVERBOSE
   printf("parser_vrmlScene: Try proto\n");
#endif
  if(parser_protoStatement(me)) {
#ifdef CPARSERVERBOSE
   printf("parser_vrmlScene: PROTO parsed\n");
#endif
   continue;
  }

  break;
 }

 /* We were unable to keep parsing Nodes, ROUTEs, or PROTOs.  Check that this is indeed the end of the file.  If it isn't, 
    there is an error, so we return FALSE. */
 return lexer_eof(me->lexer);
}

/* ************************************************************************** */
/* Nodes and fields */

/* Parses an interface declaration and adds it to the PROTO or Script definition */
/* Adds the user-defined name to the appropriate user-defined name list (user_field, user_exposedField, user_eventIn, or user_eventOut)
   Creates a protoFieldDecl or scriptFieldDecl structure to hold field data.
   Parses and stores the default value of fields and exposedFields.
   Adds the protoFieldDecl or scriptFieldDecl to the list of fields in the ProtoDefinition or Script structure. */ 
BOOL parser_interfaceDeclaration(struct VRMLParser* me,
 struct ProtoDefinition* proto, struct Script* script)
{
 indexT mode;
 indexT type;
 indexT name;
 union anyVrml defaultVal;
 struct ProtoFieldDecl* pdecl=NULL;
 struct ScriptFieldDecl* sdecl=NULL;

 /* Either PROTO or Script interface! */
 assert((proto || script) && !(proto && script));

 /* lexer_protoFieldMode is #defined as lexer_specialID(me, r, NULL, PROTOKEYWORDS, PROTOKEYWORDS_COUNT, NULL) */
 /* Looks for the next token in the array PROTOKEYWORDS (eventIn, eventOut, exposedField, or field) and returns the 
    appropriate index in mode */
 if(!lexer_protoFieldMode(me->lexer, &mode))
  return FALSE;

 /* Script can not take exposedFields */
 if(script && mode==PKW_exposedField)
  PARSE_ERROR("Scripts must not have exposedFields!")
  
 /* lexer_fieldType is #defined as lexer_specialID(me, r, NULL, FIELDTYPES, FIELDTYPES_COUNT, NULL) */
 /* Looks for the next token in the array FIELDTYPES and returns the index in type */
 if(!lexer_fieldType(me->lexer, &type))
  PARSE_ERROR("Expected fieldType after proto-field keyword!")

 /* Add the next token (which is the user-defined name) to the appropriate vector.  
    For eventIns, this is user_eventIn.
    For eventOuts, this is user_eventOut.
    For exposedFields, this is user_exposedField.
    For fields, this is user_field.
 */
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

 /* If we are parsing a PROTO, create a new  protoFieldDecl.
    If we are parsing a Script, create a new scriptFieldDecl. */
 if(proto) {
  pdecl=newProtoFieldDecl(mode, type, name);
 }
 else
  sdecl=newScriptFieldDecl(mode, type, name);

 
 /* If this is a field or an exposed field */ 
 if(mode==PKW_field || mode==PKW_exposedField)
 {
  /* Get the next token(s) from the lexer and store them in defaultVal as the appropriate type. 
    This is the default value for this field.  */
  assert(PARSE_TYPE[type]);
  if(!PARSE_TYPE[type](me, (void*)&defaultVal))
  {
   /* Invalid default value parsed.  Delete the proto or script declaration. */
   parseError("Expected default value for field!");
   if(pdecl) deleteProtoFieldDecl(pdecl);
   if(sdecl) deleteScriptFieldDecl(sdecl);
   return FALSE;
  }

  /* Store the default field value in the protoFieldDeclaration or scriptFieldDecl structure */
  if(proto) {
   pdecl->defaultVal=defaultVal;
  }
  else
  {
   assert(script);
   scriptFieldDecl_setFieldValue(sdecl, defaultVal);
  }
 }

 /* Add the new field declaration to the list of fields in the Proto or Script definition.
    For a PROTO, this means adding it to the iface vector of the ProtoDefinition. 
    For a Script, this means adding it to the fields vector of the ScriptDefinition. */ 
 if(proto)
 {
  /* protoDefinition_addIfaceField is #defined as vector_pushBack(struct ProtoFieldDecl*, (me)->iface, field) */
  /* Add the protoFieldDecl structure to the iface vector of the protoDefinition structure */
  protoDefinition_addIfaceField(proto, pdecl);
 } else
 {
  /* Add the scriptFieldDecl structure to the fields vector of the Script structure */
  assert(script);
  script_addField(script, sdecl);
 }

 return TRUE;
}

/* Parses a protoStatement */
/* Adds the PROTO name to the userNodeTypesVec list of names.  
   Creates a new protoDefinition structure and adds it to the PROTOs list.
   Goes through the interface declarations for the PROTO and adds each user-defined field name to the appropriate list of user-defined names (user_field, 
   user_eventIn, user_eventOut, or user_exposedField), creates a new protoFieldDecl for the field and adds it to the iface vector of the ProtoDefinition, 
   and, in the case of fields and exposedFields, gets the default value of the field and stores it in the protoFieldDecl. 
   Parses the body of the PROTO.  Nodes are added to the scene graph for this PROTO.  Routes are parsed and a new ProtoRoute structure
   is created for each one and added to the routes vector of the ProtoDefinition.  PROTOs are recursively parsed!
*/ 
BOOL parser_protoStatement(struct VRMLParser* me)
{
 indexT name;
 struct ProtoDefinition* obj;

 /* Really a PROTO? */
 if(!lexer_keyword(me->lexer, KW_PROTO))
  return FALSE;

 /* Our name */
 /* lexer_defineNodeType is #defined as lexer_defineID(me, ret, userNodeTypesVec, FALSE) */
 /* Add the PROTO name to the userNodeTypesVec list of names return the index of the name in the list in name */ 
 if(!lexer_defineNodeType(me->lexer, &name))
  PARSE_ERROR("Expected nodeTypeId after PROTO!\n")
 assert(name!=ID_UNDEFINED);

 /* Create a new blank ProtoDefinition structure to contain the data for this PROTO */
 obj=newProtoDefinition();

 /* If the PROTOs stack has not yet been created, create it */
 if(!PROTOs) {
  	parser_scopeIn_PROTO();
  }

 assert(PROTOs);
 /*  assert(name==vector_size(PROTOs)); */

 /* Add the empty ProtoDefinition structure we just created onto the PROTOs stack */
 vector_pushBack(struct ProtoDefinition*, PROTOs, obj);
 
 /* Now we want to fill in the information in the ProtoDefinition */

 /* Interface declarations */

 /* Make sure that the next token is a '['.  Skip over it. */
 if(!lexer_openSquare(me->lexer))
  PARSE_ERROR("Expected [ to start interface declaration!")

 /* Read the next line and parse it as an interface declaration. */
 /* Add the user-defined field name to the appropriate list of user-defined names (user_field, user_eventIn, user_eventOut, or user_exposedField).
    Create a new protoFieldDecl for this field and add it to the iface vector for the ProtoDefinition obj.
    For fields and exposedFields, get the default value of the field and store it in the protoFieldDecl. */
 while(parser_interfaceDeclaration(me, obj, NULL));

 /* Make sure that the next token is a ']'.  Skip over it. */
 if(!lexer_closeSquare(me->lexer))
  PARSE_ERROR("Expected ] after interface declaration!")

 /* PROTO body */
 /* Make sure that the next oken is a '{'.  Skip over it. */
 if(!lexer_openCurly(me->lexer))
  PARSE_ERROR("Expected { to start PROTO body!")

 /* Create a new vector of nodes and push it onto the DEFedNodes stack */
 /* This is the list of nodes defined for this scope */
 /* Also checks that the PROTOs vector exists, and creates it if it does not */
 parser_scopeIn();

 /* Parse body */
 {
  /* Store the previous currentProto for future reference, and set the PROTO we are now parsing to the currentProto */
  struct ProtoDefinition* oldCurPROTO=me->curPROTO;
  me->curPROTO=obj;
 
  /* While we are able to parse nodes, routes, and protos, keep doing so */
  while(TRUE)
  {
   {
    vrmlNodeT node;
    
    /* If we can successfully parse the next section of the file as a node statement, we do so and get a pointer 
       to the X3D_Node structure containing the values for the node. Then we add the node to the scene graph for
       this proto */
#ifdef CPARSERVERBOSE
    printf("protoStatement: try node .. \n");
#endif
    if(parser_nodeStatement(me, &node))
    {
     protoDefinition_addNode(obj, node);
#ifdef CPARSERVERBOSE
      printf("protoStatement: parsed node\n");
#endif
     continue;
    }
   }

   /*  Parse a ROUTE statement and add a new ProtoRoute structure to the routes vector of this ProtoDefinition */ 
#ifdef CPARSERVERBOSE
   printf("protoStatement: try ROUTE statement ...\n");
#endif
   if(parser_routeStatement(me)) {
#ifdef CPARSERVERBOSE
    printf("protoStatement: parsed route\n");
#endif
    continue;
   }

   /* Nested PROTO.  Parse the PROTO and add it to the PROTOs list */
#ifdef CPARSERVERBOSE
   printf("protoStatement: try protoStatement .. \n");
#endif
   /* A proto within a proto ....
      Parse the PROTO and add the ProtoDefinition to the PROTOs stack */
   if(parser_protoStatement(me)) {
#ifdef CPARSERVERBOSE
    printf("protoStatement: parsed proto\n");
#endif
    continue;
   }

   break;
  }

  /* We are done parsing this proto.  Set the curPROTO to the last proto we were parsing. */
  me->curPROTO=oldCurPROTO;
 }

 /* Takes the top DEFedNodes vector off of the stack.  The local scope now refers to the next vector in the DEFedNodes stack */
 parser_scopeOut();

 /* Make sure that the next token is a '}'.  Skip over it. */
 if(!lexer_closeCurly(me->lexer))
  PARSE_ERROR("Expected } after PROTO body!")

 return TRUE;
}

/* Parses a routeStatement */
/* Checks that the ROUTE statement is valid (i.e. that the referenced node and field combinations  
   exist, and that they are compatible) and then adds the route to either the CRoutes table of routes, or adds a new ProtoRoute structure to the vector 
   ProtoDefinition->routes if we are parsing a PROTO */
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
  /* Look for the current token in the userNodeNames vector (DEFed names) */ \
  if(!lexer_nodeName(me->lexer, &pre##NodeIndex)) { \
       /* The current token is not a valid DEFed name.  Error. */ \
	strcpy (fw_outline,"ERROR:ROUTE: Expected a valid DEF name; found \""); \
	if (me->lexer->curID != NULL) strcat (fw_outline, me->lexer->curID); else strcat (fw_outline, "(EOF)"); \
	strcat (fw_outline,"\" "); \
  	ConsoleMessage(fw_outline);  \
	fprintf (stderr,"%s\n",fw_outline); \
  	PARSER_FINALLY  \
  	return FALSE; \
  } \
  /* Check that there are DEFedNodes in the DEFedNodes vector, and that the index given for this node is valid */ \
  assert(DEFedNodes && !stack_empty(DEFedNodes) && \
   pre##NodeIndex<vector_size(stack_top(struct Vector*, DEFedNodes))); \
  /* Get the X3D_Node structure for the DEFed node we just looked up in the userNodeNames list */ \
  pre##Node=vector_get(struct X3D_Node*, \
   stack_top(struct Vector*, DEFedNodes), \
   pre##NodeIndex); \
  /* We were'nt able to get the X3D_Node structure for the DEFed node.  Error. */ \
  if (pre##Node == NULL) { \
	/* we had a bracket underflow, from what I can see. JAS */ \
	strcpy (fw_outline,"ERROR:ROUTE: no DEF name found - check scoping and \"}\"s"); \
  	ConsoleMessage(fw_outline);  \
	fprintf (stderr,"%s\n",fw_outline); \
  	PARSER_FINALLY  \
  	return FALSE; \
 } else { \
  /* PROTO/Script? */ \
  /* We got the node structure for the DEFed node. If this is a Group or a Script node do some more processing */ \
  	switch(pre##Node->_nodeType) \
  	{ \
  	 case NODE_Group: \
          /* Get a pointer to the protoDefinition for this group node */ \
  	  pre##Proto=X3D_GROUP(pre##Node)->__protoDef; \
  	  assert(pre##Proto); \
  	  break; \
         /* Get a pointer to the Script structure for this script node */ \
  	 case NODE_Script: \
  	  pre##Script=((struct X3D_Script*)pre##Node)->__scriptObj; \
  	  assert(pre##Script); \
  	  break; \
  	} \
  } \
  /* Can't be both a PROTO and a Script node */ \
  assert(!(pre##Proto && pre##Script)); \
  /* The next character has to be a '.' - skip over it */ \
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
   /* This is a builtin DEFed node.  */ \
   /* Look for the next token (field of the DEFed node) in the builtin EVENT_IN/EVENT_OUT or EXPOSED_FIELD arrays */ \
   if(!lexer_event##eventType(me->lexer, pre##Node, \
    &pre##FieldO, &pre##FieldE, NULL, NULL))  {\
        /* event not found in any built in array.  Error. */ \
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
   /* This is a user defined node type */ \
   /* Look for the next token (field of the DEFed node) in the user_eventIn/user_eventOut, user_exposedField arrays */ \
   if(lexer_event##eventType(me->lexer, pre##Node, \
    NULL, NULL, &pre##UFieldO, &pre##UFieldE)) \
   { \
    if(pre##UFieldO!=ID_UNDEFINED) \
    { \
     /* We found the event in user_eventIn or user_eventOut */ \
     if(pre##Proto) \
     { \
      assert(!pre##Script); \
      /* If this is a PROTO get the ProtoFieldDecl for this event */ \
      pre##ProtoField=protoDefinition_getField(pre##Proto, pre##UFieldO, \
       PKW_event##eventType); \
     } else \
     { \
      assert(pre##Script && !pre##Proto); \
      /* If this is a Script get the scriptFieldDecl for this event */ \
      pre##ScriptField=script_getField(pre##Script, pre##UFieldO, \
       PKW_event##eventType); \
     } \
    } else \
    { \
     /* We found the event in user_exposedField */ \
     if(pre##Proto) \
     { \
      assert(!pre##Script); \
      assert(pre##UFieldE!=ID_UNDEFINED); \
      /* If this is a PROTO get the ProtoFieldDecl for this event */ \
      pre##ProtoField=protoDefinition_getField(pre##Proto, pre##UFieldE, \
       PKW_exposedField); \
     } else \
     { \
      assert(pre##Script && !pre##Proto); \
      /* If this is a Script get the scriptFieldDecl for this event */ \
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

 /* Parse the first part of a routing statement: DEFEDNODE.event by locating the node DEFEDNODE in either the builtin or user-defined name arrays
    and locating the event in the builtin or user-defined event name arrays */
 ROUTE_PARSE_NODEFIELD(from, Out)
 /* Next token has to be "TO" */
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
/* Parse the second part of a routing statement: DEFEDNODE.event by locating the node DEFEDNODE in either the builtin or user-defined name arrays 
   and locating the event in the builtin or user-defined event name arrays */
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

	/* If the field was found in the EXPOSED_FIELDS array */
  	if(fromFieldE!=ID_UNDEFINED) {
		/* printf ("step1a, node type %s fromFieldE %d %s\n",stringNodeType(fromNode->_nodeType),fromFieldE,EXPOSED_FIELD[fromFieldE]); */

        	/* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
		tempFE = findRoutedFieldInFIELDNAMES(fromNode,EXPOSED_FIELD[fromFieldE],0);

        	/* Field was found in FIELDNAMES, and this is a valid field?  Then, try to find the index of the event in EXPOSEDFIELD */
		if (tempFE != ID_UNDEFINED) tempFE = findFieldInEXPOSED_FIELD(FIELDNAMES[tempFE]);
	}
        /* If the from event was found in the EVENT_OUT array */
	if (fromFieldO != ID_UNDEFINED) {
		/* printf ("step2a, node type %s fromFieldO %d %s\n",stringNodeType(fromNode->_nodeType),fromFieldO,EVENT_OUT[fromFieldO]); */

        	/* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
		tempFO = findRoutedFieldInFIELDNAMES(fromNode,EVENT_OUT[fromFieldO],0);

  		/* Field was found in FIELDNAMES, and this is a valid field?  Then find it in EVENT_OUT */
		if (tempFO != ID_UNDEFINED) {
			 tempFO = findFieldInEVENT_OUT(FIELDNAMES[tempFO]);
		}  

                /* We didn't find the event in EVENT_OUT?  Look for it in EXPOSED_FIELD */
		if (tempFO == ID_UNDEFINED) {

        		/* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
			temp = findRoutedFieldInFIELDNAMES(toNode,EVENT_OUT[toFieldO],0);

  			/* Field found in FIELDNAMES and it is a valid field?  Then find it in EXPOSED_FIELD */
			if (temp != ID_UNDEFINED) tempFE = findFieldInEXPOSED_FIELD(FIELDNAMES[temp]);
		}
	}
 }
 if(!toProtoField && !toScriptField) {
	/* toFieldE = Daniel's code thinks this is from an exposedField */
	/* toFieldO = Daniel's code thinks this is from an eventIn */

	/* If the field was found in the EXPOSED_FIELDS array */
  	if(toFieldE!=ID_UNDEFINED) {
		/* printf ("step3a, node type %s toFieldE %d %s\n",stringNodeType(toNode->_nodeType),toFieldE,EXPOSED_FIELD[toFieldE]); */

        	/* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
		tempTE = findRoutedFieldInFIELDNAMES(toNode,EXPOSED_FIELD[toFieldE],1);

        	/* Field was found in FIELDNAMES, and this is a valid field?  Then, try to find the index of the event in EXPOSEDFIELD */
		if (tempTE != ID_UNDEFINED) tempTE = findFieldInEXPOSED_FIELD(FIELDNAMES[tempTE]);
	}


        /* If the from event was found in the EVENT_IN array */
  	if(toFieldO!=ID_UNDEFINED) {
		/* printf ("step4a, node type %s toFieldO %d %s\n",stringNodeType(toNode->_nodeType),toFieldO,EVENT_IN[toFieldO]); */

        	/* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
		tempTO = findRoutedFieldInFIELDNAMES(toNode,EVENT_IN[toFieldO],1);

  		/* Field was found in FIELDNAMES, and this is a valid field?  Then find it in EVENT_IN */
		if (tempTO != ID_UNDEFINED) {
			tempTO = findFieldInEVENT_IN(FIELDNAMES[tempTO]);
		}  

                /* We didn't find the event in EVENT_IN?  Look for it in EXPOSED_FIELD */
		if (tempTO == ID_UNDEFINED) {
        		/* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
			temp = findRoutedFieldInFIELDNAMES(toNode,EVENT_IN[toFieldO],1);

  			/* Field found in FIELDNAMES and it is a valid field?  Then find it in EXPOSED_FIELD */
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
 /* Get the offset to the eventOut in the fromNode and store it in fromOfs */
 /* Get the size of the eventOut type and store it in fromLen */
 if(!fromProtoField && !fromScriptField)
  /* If the from field is an exposed field */
  if(fromFieldE!=ID_UNDEFINED) {
   /* Get the offset and size of this field in the fromNode */
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
  /* If the from field is an event out */
  } else if(fromFieldO!=ID_UNDEFINED) {
   /* Get the offset and size of this field in the fromNode */
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
 /* Get the offset to the eventIn in the toNode and store it in toOfs */
 /* Get the size of the eventOut type and store it in fromLen */
 if(!toProtoField && !toScriptField)
  /* If this is an exposed field */
  if(toFieldE!=ID_UNDEFINED) {
   /* get the offset and size of this field in the toNode */
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

  /* If this is an eventIn */
  } else if(toFieldO!=ID_UNDEFINED) {
   /* Get the offset and size of this field in the toNode */
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

 /* Get size information for user defined fields */
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
 /* We can only ROUTE between two equivalent fields.  If the size of one field value is different from the size of the other, we have problems (i.e. can't route SFInt to MFNode) */
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
  /* If we are in a PROTO add a new ProtoRoute structure to the vector ProtoDefinition->routes */
  /* Otherwise, add the ROUTE to the routing table CRoutes */
  parser_registerRoute(me, fromNode, fromOfs, toNode, toOfs, toLen, routingDir);

 /* Built-in to user-def */
 else if(!fromProtoField && toProtoField)
  /* For each member of the dests vector for this protoFieldDecl call parser_registerRoute for that destination node and offset */
  /* i.e. for every statement field IS user_field for the user_field defined in protoFieldDecl, register a route 
     to the node and field where the IS statement occurred */
  protoFieldDecl_routeTo(toProtoField, fromNode, fromOfs, routingDir, me);
 /* User-def to built-in */
 else if(fromProtoField && !toProtoField)
  /* For each member of the dests vector for this protoFieldDecl call parser_registerRoute for that destination node and offset */
  /* i.e. for every statement field IS user_field for the user_field defined in protoFieldDecl, register a route from the node and
     field where the IS statement occurred */
  protoFieldDecl_routeFrom(fromProtoField, toNode, toOfs, routingDir, me);
 /* User-def to user-def */
 else
  PARSE_ERROR("Routing from user-event to user-event is currently unsupported!")

 return TRUE;
}

/* Register a ROUTE here */
/* If we are in a PROTO add a new ProtoRoute structure to the vector ProtoDefinition->routes */
/* Otherwise, add the ROUTE to the routing table CRoutes */
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
/* If the statement starts with DEF, and has not previously been defined, we add it to the userNodeNames and DEFedNodes vectors.  We parse the node
   and return a pointer to the node in ret.
   If the statement is a USE statement, we look up the user-defined node name in the userNodeTypesVec vector and retrieve a pointer to the node from 
   the DEFedNodes vector and return it in ret.
   Otherwise, this is just a regular node statement. We parse the node into a X3D_Node structure of the appropriate type, and return a pointer to 
   this structure in ret. 
*/ 
BOOL parser_nodeStatement(struct VRMLParser* me, vrmlNodeT* ret)
{
 assert(me->lexer);

 /* A DEF-statement? */
 if(lexer_keyword(me->lexer, KW_DEF))
 {
  indexT ind;

  /* lexer_defineNodeName is #defined as lexer_defineID(me, ret, stack_top(struct Vector*, userNodeNames), TRUE) */
  /* Checks if this node already exists in the userNodeNames vector.  If it doesn't, adds it. */
  if(!lexer_defineNodeName(me->lexer, &ind))
   PARSE_ERROR("Expected nodeNameId after DEF!\n")
  assert(ind!=ID_UNDEFINED);

  /* If the DEFedNodes stack has not already been created.  If not, create new stack and add an X3D_Nodes vector to that stack */ 
  if(!DEFedNodes || stack_empty(DEFedNodes))
   parser_scopeIn_DEFUSE();
  assert(DEFedNodes);
  assert(!stack_empty(DEFedNodes));

  /* Did we just add the name to the userNodeNames vector?  If so, then the node hasn't yet been added to the DEFedNodes vector, so add it */
  assert(ind<=vector_size(stack_top(struct Vector*, DEFedNodes)));
  if(ind==vector_size(stack_top(struct Vector*, DEFedNodes))) {
   vector_pushBack(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes),
    NULL);
  }
  assert(ind<vector_size(stack_top(struct Vector*, DEFedNodes)));

  vrmlNodeT node;

  /* Parse this node.  Create an X3D_Node structure of the appropriate type for this node and fill in the values for the fields
     specified.  Add any routes to the CRoutes table. Add any PROTOs to the PROTOs vector */
#ifdef CPARSERVERBOSE
  printf("parser_nodeStatement: parsing DEFed node \n");
#endif
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
#ifdef CPARSERVERBOSE
  printf("parser_nodeStatement: DEFed node successfully parsed\n");
#endif

  /* Set the top memmber of the DEFed nodes stack to this node */
  vector_get(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes), ind)=node;
#ifdef CPARSERVERBOSE
    printf("parser_nodeStatement: adding DEFed node (pointer %p) to DEFedNodes vector\n", node);  
#endif

  /*
  if(!parser_node(me, &vector_get(struct X3D_Node*,
   stack_top(struct Vector*, DEFedNodes), ind)))
   PARSE_ERROR("Expected node in DEF statement!\n")
  */

  /* Return a pointer to the node in the variable ret */
  *ret=vector_get(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes), ind);
  return TRUE;
 }

 /* A USE-statement? */
 if(lexer_keyword(me->lexer, KW_USE))
 {
  indexT ind;

  /* lexer_nodeName is #defined as lexer_specialID(me, NULL, ret, NULL, 0, stack_top(struct Vector*, userNodeNames)) */
  /* Look for the nodename in list of user-defined node names (userNodeNames) and return the index in ret */
  if(!lexer_nodeName(me->lexer, &ind)) {
   PARSE_ERROR("Expected nodeNameId after USE!\n")
  }
#ifdef CPARSERVERBOSE
   printf("parser_nodeStatement: parsing USE\n");
#endif

  /* If we're USEing it, it has to already be defined. */
  assert(ind!=ID_UNDEFINED);

  /* It also has to be in the DEFedNodes stack */
  assert(DEFedNodes && !stack_empty(DEFedNodes) &&
   ind<vector_size(stack_top(struct Vector*, DEFedNodes)));

  /* Get a pointer to the X3D_Node structure for this DEFed node and return it in ret */
  *ret=vector_get(struct X3D_Node*, stack_top(struct Vector*, DEFedNodes), ind);
  return TRUE;
 }

 /* Otherwise, simply a node. */
 return parser_node(me, ret);
}

/* Parses a node (node non-terminal) */
/* Looks up the node type on the builtin NODES list and the userNodeNames list.  
   If this is a builtin node type, creates a new X3D_Node structure of the appropriate type for the node, and then parses the statements for that node.  
   	For each field statement, gets the value for that field and stores it in the X3D_Node structure.  
   	For each ROUTE statement, adds the route to the CRoutes table.
   	For each PROTO statement, adds the PROTO definition to te PROTOs list. 
	Return a pointer to the X3D_Node structure that holds the information for this node.
   If this is a user-defined node type (i.e. a PROTO expansion), complete the proto expansion.  
	For each field in the ProtoDefinition either parse and propagate the specified value for this field, or 
        propagate the default value of the field.  (i.e. copy the appropriate value into every node/field combination in
        the dests list.)
	For each route in the routes list of the ProtoDefinition, add the route to the CRoutes table.
	Return a pointer to the X3D_Node structure that is the scenegraph for this PROTO.
*/
BOOL parser_node(struct VRMLParser* me, vrmlNodeT* ret)
{
 indexT nodeTypeB, nodeTypeU;
 struct X3D_Node* node=NULL;

 assert(me->lexer);
 
  /* lexer_node( ... ) #defined to lexer_specialID(me, r1, r2, NODES, NODES_COUNT, userNodeTypesVec) where userNodeTypesVec is a list of PROTO defs */
  /* this will get the next token (which will be the node type) and search the NODES array for it.  If it is found in the NODES array nodeTypeB will be set to 
     the index of type in the NODES array.  If it is not in NODES, the list of user-defined nodes will be searched for the type.  If it is found in the user-defined 
     list nodeTypeU will be set to the index of the type in userNodeTypesVec.  A return value of FALSE indicates that the node type wasn't found in either list */
 if(!lexer_node(me->lexer, &nodeTypeB, &nodeTypeU))
  return FALSE;

  /* Checks that the next non-whitespace non-comment character is '{' and skips it. */
 if(!lexer_openCurly(me->lexer))
  PARSE_ERROR("Expected { after node-type id!")

 /* Built-in node */
 /* Node was found in NODES array */
 if(nodeTypeB!=ID_UNDEFINED)
 {
#ifdef CPARSERVERBOSE
  printf("parser_node: parsing builtin node\n");
#endif
  struct Script* script=NULL;
 
  /* Get malloced struct of appropriate X3D_Node type with default values filled in */
  node=X3D_NODE(createNewX3DNode(nodeTypeB));
  assert(node);

  /* Node specific initialization */
  /* From what I can tell, this only does something for Script nodes.  It sets node->__scriptObj to newScript() */
  parser_specificInitNode(node);

  /* Set curScript for Script-nodes */
  if(node->_nodeType==NODE_Script)
   script=X3D_SCRIPT(node)->__scriptObj;

  /* As long as the lexer is returning field statements, ROUTE statements, or PROTO statements continue parsing node */
  while(TRUE)
  {
   /* Try to parse the next statement as a field.  For normal "field value" statements (i.e. position 1 0 1) this gets the value of the field from the lexer (next token(s)
      to be processed) and stores it as the appropriate type in the node.
      For IS statements (i.e. position IS mypos) this adds the node-field combo (as an offsetPointer) to the dests list for the protoFieldDecl associated with the user
      defined field (in the given case, this would be mypos).  */
#ifdef CPARSERVERBOSE
   printf("parser_node: try parsing field ... \n");
#endif
   if(parser_field(me, node)) {
#ifdef CPARSERVERBOSE
        printf("parser_node: field parsed\n");
#endif
	continue;
   }

   /* Try to parse the next statement as a ROUTE (i.e. statement starts with ROUTE).  This checks that the ROUTE statement is valid (i.e. that the referenced node and field combinations  
      exist, and that they are compatible) and then adds the route to either the CRoutes table of routes, or adds a new ProtoRoute structure to the vector 
      ProtoDefinition->routes if we are parsing a PROTO */
#ifdef CPARSERVERBOSE
   printf("parser_node: try parsing ROUTE ... \n");
#endif
   if(parser_routeStatement(me))  {
#ifdef CPARSERVERBOSE
	printf("parser_node: ROUTE parsed\n");
#endif
	continue;
   }

   /* Try to parse the next statement as a PROTO (i.e. statement starts with PROTO).  */
   /* Add the PROTO name to the userNodeTypesVec list of names.  Create and fill in a new protoDefinition structure and add it to the PROTOs list.
      Goes through the interface declarations for the PROTO and adds each user-defined field name to the appropriate list of user-defined names (user_field, 
      user_eventIn, user_eventOut, or user_exposedField), creates a new protoFieldDecl for the field and adds it to the iface vector of the ProtoDefinition, 
      and, in the case of fields and exposedFields, gets the default value of the field and stores it in the protoFieldDecl. 
      Parses the body of the PROTO.  Nodes are added to the scene graph for this PROTO.  Routes are parsed and a new ProtoRoute structure
      is created for each one and added to the routes vector of the ProtoDefinition.  PROTOs are recursively parsed!
    */
#ifdef CPARSERVERBOSE
   printf("parser_node: try parsing PROTO ... \n");
#endif
   if(parser_protoStatement(me)) {
#ifdef CPARSERVERBOSE
	printf("parser_node: PROTO parsed\n");
#endif
	continue;
   }

   if(script && parser_interfaceDeclaration(me, NULL, script)) continue;
   break;
  }

  /* Init code for Scripts */
  if(script)
   script_initCodeFromMFUri(script, &X3D_SCRIPT(node)->url);
 }
 
 /* The node name was located in userNodeTypesVec (list of defined PROTOs), therefore this is an attempt to instantiate a PROTO */
 else
 {
#ifdef CPARSERVERBOSE
  printf("parser_node: parsing user defined node\n");
#endif
  struct ProtoDefinition* protoCopy;
  struct ProtoDefinition* origProto;

  /* If this is a PROTO instantiation, then there must be at least one PROTO defined.  Also, the index retrieved for
     this PROTO must be valid. */
  assert(nodeTypeU!=ID_UNDEFINED);
  assert(PROTOs);
  assert(nodeTypeU<vector_size(PROTOs));

  /* Fetch the PROTO for this user-defined name from the PROTOs list, and copy the protoDefinition into the structure protoCopy */
  protoCopy=protoDefinition_copy(vector_get(struct ProtoDefinition*,
   PROTOs, nodeTypeU));
  origProto = vector_get(struct ProtoDefinition*, PROTOs, nodeTypeU);

#ifdef CPARSERVERBOSE
  printf("parser_node: expanding proto\n");
#endif

  /* Parse all fields, routes, and PROTOs */
  /* Fields are parsed by setting the value of all destinations for this field to the value specified in the field statement */ 
  /* Routes are parsed by adding them to the CRoutes table */
  /* PROTOs are parsed by creating a new protoDefinition structure and adding it to the PROTOs vector, and adding the name to the 
     userNodeTypesVec vector  */

  /* FIXME - there shouldn't be parser_protoStatement here ... should there?  */
  while(parser_protoField(me, protoCopy, origProto) ||
   parser_routeStatement(me) || parser_protoStatement(me));

  /* Gets the scene tree for this protoDefinition and returns a pointer to it. 
     Also propogates the value of each field in the proto for which no non-default
     value has been specified (that is, copies the default value into each node/field 
     pair in the dests list), and puts all routes in the protoDefinition's route vector
     into the CRoutes table */
  node=protoDefinition_extractScene(protoCopy);
  /* dump_scene(0, node); */
  assert(node);

  /* Can't delete ProtoDefinition, it is referenced! */
  /*deleteProtoDefinition(protoCopy);*/
 }

 /* We must have a node that we've parsed at this point. */
 assert(node);

 /* Check that the node is closed by a '}', and skip this token */
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

 /* Return the parsed node */
 *ret=node;
 return TRUE;
}

/* Parses a field assignment of a PROTOtyped node */
/* Fetches the protoFieldDecl for the named field from the iface list of this ProtoDefinition. */
/* Then fetches the value for this field from the lexer and assigns it to every node/field combination
   registered as a destination for this proto field (i.e. every entry in that field's dests vector). */
BOOL parser_protoField(struct VRMLParser* me, struct ProtoDefinition* p, struct ProtoDefinition* op)
{
 indexT fieldO, fieldE;
 struct ProtoFieldDecl* field=NULL;

#ifdef CPARSERVERBOSE
  printf("parser_protoField: look for field in user_field and user_exposedField\n");
#endif
 /* Checks for the field name in user_field and user_exposedField */
 if(!lexer_field(me->lexer, NULL, NULL, &fieldO, &fieldE)) {
  return FALSE;
  }

 /* If this field was found in user_field */
 if(fieldO!=ID_UNDEFINED)
 {
  /* Get the protoFieldDecl for this field from the iface list for this PROTO */
  field=protoDefinition_getField(p, fieldO, PKW_field);

  /* This field is not defined for this PROTO.  Error. */
  if(!field)
   PARSE_ERROR("Field is not part of PROTO's interface!")

  assert(field->mode==PKW_field);
 } else
 {
  /* If this field wasn't found in user_field, it must be found in user_exposedField */
  assert(fieldE!=ID_UNDEFINED);

  /* Get the protoFieldDecl for this field from the iface list for this PROTO */
  field=protoDefinition_getField(p, fieldE, PKW_exposedField);

  /* This exposedField is not defined for this PROTO.  Error. */
  if(!field)
   PARSE_ERROR("Field is not part of PROTO's interface!")
  assert(field->mode==PKW_exposedField);
 }

 /* We must have retrieved the field */
 assert(field);

 /* Parse the value */
 {
  union anyVrml val;
  /* Get the value for this field and store it in the union anyVrml as the correct type.
     (In essence, the anyVrml union acts as a fake node here.  The function fills in the value 
     as if passed a node pointer with an offset of 0 - placing the value into the anyVrml union. */
   /*** FIXME:  WE ARE STORING val (local variable) with offset 0 in the dests array ***/
   /* We need to store offsetpointer(p->tree, ) */
  if(!parser_fieldValue(me, newOffsetPointer(&val, 0), field->type, ID_UNDEFINED, TRUE, p, op, field))
   PARSE_ERROR("Expected value of field after fieldId!")

  /* Go throught the dests vector for this field, and set the value of each dest to this value.  
     (Note that the first element of the dests vector is actually assigned the value of "val".  While
     subsequent elements of the dests vector have a DEEPCOPY of the "val" assigned (if appropriate). */
  /* printf("set value %f\n", val.sffloat); */
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
/* Gets the actual value of a field and stores it in a node, or, for an IS statement, adds this node and field as a destination to the appropriate protoFieldDecl */
/* Passed pointer to the parser, an offsetPointer structure pointing to the current node and an offset to the field being parsed, type of the event value (i.e. MFString) index in FIELDTYPES, */
/* index of the field in the FIELDNAMES (or equivalent) array */
BOOL parser_fieldValue(struct VRMLParser* me, struct OffsetPointer* ret,
 indexT type, indexT origFieldE, BOOL protoExpansion, struct ProtoDefinition* pdef, struct ProtoDefinition* opdef, struct ProtoFieldDecl* origField)
{
 #undef PARSER_FINALLY
 #define PARSER_FINALLY \
  deleteOffsetPointer(ret);

 /* If we are inside a PROTO, IS is possible */
 if(me->curPROTO && lexer_keyword(me->lexer, KW_IS))
 {
  indexT fieldO, fieldE;
  struct ProtoFieldDecl* pField=NULL;

#ifdef CPARSERVERBOSE
  printf("parser_fieldValue: this is an IS statement\n");
#endif
  /* If the field was found in the built in arrays of fields, then try to see if this is followed by a valid IS statement */
  /* Check that the part after IS consists of a valid user-defined eventIn/eventOut/exposed field and then add an Offset_Pointer struct (pointer to the node with the IS statement
    and an offset to the field that uses the IS) to the dests list of the fieldDeclaration structure of the proto */
  /* If this is valid, then we've parsed the field, so return */
  if(origFieldE!=ID_UNDEFINED &&
   parser_fieldEventAfterISPart(me, ret->node, TRUE, TRUE, ID_UNDEFINED,
    origFieldE))
   return TRUE;

  /* Must be a user-defined fieldname */
  /* Check that the field exists in either user_exposedFields or user_fields */
  if(!lexer_field(me->lexer, NULL, NULL, &fieldO, &fieldE))
   PARSE_ERROR("Expected fieldId after IS!")

  /* If the field was found in user_fields */
  if(fieldO!=ID_UNDEFINED)
  {
   /* Get the protoFieldDeclaration for the field at index fieldO */
   pField=protoDefinition_getField(me->curPROTO, fieldO, PKW_field);
   if(!pField)
    PARSE_ERROR("IS source is no field of current PROTO!")
   assert(pField->mode==PKW_field);
  } else
  /* If the field was found in user_exposedFields */
  {
   assert(fieldE!=ID_UNDEFINED);
   /* Get the protoFieldDeclaration for the exposedField at index fieldO */
   pField=protoDefinition_getField(me->curPROTO, fieldE, PKW_exposedField);
   if(!pField)
    PARSE_ERROR("IS source is no field of current PROTO!")
   assert(pField->mode==PKW_exposedField);
  }
  assert(pField);

  /* Check type */
  /* Check that the field type (i.e. SFInt) is the same as the field type in the protoFieldDeclaration */
  if(pField->type!=type)
   PARSE_ERROR("Types mismatch for PROTO field!")

  /* Don't set field for now but register us as users of this PROTO field */
  /* protoFieldDecl_addDestinationOptr is #defined as vector_pushBack(struct OffsetPointer*, me->dests, optr) */
  /* Add a the offset pointer for this field to the dests vector for the protoFieldDecl pField */
  if (!protoExpansion) {
  	protoFieldDecl_addDestinationOptr(pField, ret);
  } else {
       /* This is an IS statement within a proto expansion in a proto statement. (i.e. during a PROTO definition there is an instance
	  where a different PROTO is instantiated.  The instantiated PROTO has a statement: userdefinedfield IS anotheruserdefinedfield)
          We return the default value of "anotheruserdefinedfield" so that it can be used as the value for userdefinedfield
    	  in the protoexpansion.  

	  We also need to make the dests list for anotheruserdefinedfield point to all
          of the same places as userdefinedfield did. We do this when the scene for the proto is extracted after all fields have been parsed by
	  going through the list of nestedProtoFields and mapping their dests to valid dests in the proto expansion.  This nested field is
	  added to the nestedProtoFields list below. */

 struct NestedProtoField* nestedField;
 nestedField = newNestedProtoField(opdef, origField, pField);
 vector_pushBack(struct NestedProtoField*, pdef->nestedProtoFields, nestedField);

 switch(pField->type)
 {
  #define SF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: \
   *((vrml##ttype##T*)((char*)(ret->node))) = (pField->defaultVal).type;  \
    break; 

  #define MF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: \
    *((struct Multi_##ttype*)((char*)(ret->node))) = (pField->defaultVal).type; \
    break; 

  #include "VrmlTypeList.h"
  #undef SF_TYPE
  #undef MF_TYPE


  }
 }

  return TRUE;
 }

 /* Otherwise this is not an IS statement */
 {
  /* Get a pointer to the actual field */
  void* directRet=offsetPointer_deref(void*, ret);
  PARSER_FINALLY
 
  /* Get the actual value from the file (next token from lexer) and store it as the appropriate type in the node */
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

 /* Ask the lexer to find the field (next lexer token) in either the FIELDNAMES array or the EXPOSED_FIELD array. The 
    index of the field in the array is returned in fieldO (if found in FIELDNAMES) or fieldE (if found in EXPOSED_FIELD).  
    If the fieldname is found in neither array, lexer_field will return FALSE. */
 if(!lexer_field(me->lexer, &fieldO, &fieldE, NULL, NULL))
  /* If lexer_field does return false, this is an eventIn/eventOut IS user_definedField statement.  Add a Offset_Pointer structure to the dests list for the protoFieldDecl for 
     the user defined field.  The Offset_Pointer structure contains a pointer to the node currently being parsed along with an offset that references the field that is linked to the
     user defined field. 

     i.e. for a statement "rotation IS myrot" the protoFieldDecl for myrot is retrieved, and an Offset_Pointer structure is added to the dests list which contains a pointer to the current 
     node and the offset for the "rotation" field in that node.  

    If we've done all this, then we've parsed the field statement completely, and we return. */ 
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
 /* For a normal "field value" (i.e. position 1 0 1) statement gets the actual value of the field from the file (next token(s) to be processed) and stores it in the node
    For an IS statement, adds this node-field combo as a destination to the appropriate protoFieldDecl */
 #define PROCESS_FIELD(exposed, node, field, fieldType, var, fe) \
  case exposed##FIELD_##field: \
   if(!parser_fieldValue(me, \
    newOffsetPointer(X3D_NODE(node2), offsetof(struct X3D_##node, var)), \
    FTIND_##fieldType, fe, FALSE, NULL, NULL, NULL)) \
    PARSE_ERROR("Expected " #fieldType "Value!") \
   INIT_CODE_##fieldType(var) \
   return TRUE;
 
 /* Default action if node is not encountered in list of known nodes */
 #define NODE_DEFAULT \
  default: \
   PARSE_ERROR("Unsupported node!")

 /* Field was found in EXPOSED_FIELD list.  Parse value or IS statement */
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

 /* Field was found in FIELDNAMES list.  Parse value or IS statement */
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
 /* lexer_eventIn(me, node, a, b, c, d)  is #defined to lexer_event(me, node, a, b, c, d, ROUTED_FIELD_EVENT_IN) */
 /* look through the EVENT_IN array and the EXPOSED_FIELD array for the event name (current token).  Returns the index of the event name in evO if found in the 
    EVENT_IN array, or in evE if found in the EXPOSED_FIELD array.  Returns false if the event was found in neither array.  */
 if(lexer_eventIn(me->lexer, ptr, &evO, &evE, NULL, NULL))
 {
  isIn=TRUE;
  /* When exposedField, out is allowed, too */
  /* i.e. if this event was located in the EXPOSED_FIELD array isOut is also set to TRUE */
  isOut=(evE!=ID_UNDEFINED);

  /* lexer_eventIn(me, node, a, b, c, d) is #defined to lexer_event(me, node, a, b, c, d, ROUTED_FIELD_EVENT_OUT) */
  /* look through the EVENT_OUT array and the EXPOSED_FIELD array for the event name (current token).  Returns the index of the event name in evO if found in the 
    EVENT_OUT  array, or in evE if found in the EXPOSED_FIELD array.  Returns false if the event was found in neither array.  */
  /* This should really be lexer_eventOut(me->lexer, ptr, &evO, NULL, NULL, NULL) as the EXPOSED_FIELD array was already searched above. */
 } else if(lexer_eventOut(me->lexer, ptr, &evO, &evE, NULL, NULL))
  isOut=TRUE;

 /* event name was not found in EVENT_IN or EVENT_OUT or EXPOSED_FIELD.  Return FALSE. */
 if(!isIn && !isOut)
  return FALSE;

#ifndef NDEBUG
 if(isIn && isOut)
  assert(evE!=ID_UNDEFINED);
#endif

 /* Check that the next token in the lexer is "IS" */
 if(!lexer_keyword(me->lexer, KW_IS))
  return FALSE;
  
 /* Check that the part after IS consists of a valid user-defined eventIn/eventOut/exposed field and then add an Offset_Pointer struct (pointer to the node with the IS statement
    and an offset to the field that uses the IS) to the dests list of the fieldDeclaration structure of the proto */
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
 if(!me->curPROTO) {
  return FALSE; 
 }

 /* And finally the PROTO's event to be linked */
 /* lexer_eventIn is #defined to lexer_event(me, node, a, b, c, d, ROUTED_FIELD_EVENT_IN) */
 /* Look for the event name (current token) in the user_eventIn and user_exposedField vectors.  Return the index of the event name in pevO if it was found in the
    user_eventIn vector or in pevE if it was found in the user_exposedField vector. */
 if(isIn && lexer_eventIn(me->lexer, ptr, NULL, NULL, &pevO, &pevE)) {
  pevFound=TRUE;

#ifdef CPARSERVERBOSE
  if (pevO != ID_UNDEFINED)
    printf("parser_fieldEventAfterISPart: found field in user_eventIn\n");

  if (pevE != ID_UNDEFINED) 
    printf("parser_fieldEventAfterISPart: found field in user_exposedField\n");
#endif
  }

 /* lexer_event in is #defined to lexer_event(me, node, a, b, c, d, ROUTED_FIELD_EVENT_OUT) */
 /* Look for the event name (current token) in the user_eventOut and user_exposedField vectors.  Return the index of the event name in pevO if it was found in the
    user_eventOut vector or in pevE if it was found in the user_exposedField vector. */
 else {
  	/* printf("parser_fieldEventAfterISPart: looking for event in user_eventOut and user_exposedField\n"); */
	if(isOut && lexer_eventOut(me->lexer, ptr, NULL, NULL, &pevO, &pevE)) {

  		pevFound=TRUE;

#ifdef CPARSERVERBOSE
 		if (pevO != ID_UNDEFINED) 
 			  printf("parser_fieldEventAfterISPart: found field in user_eventout\n");
#endif
	}
 }

 /* If the event name was not found in the searched vectors return FALSE */
 if(!pevFound) {
#ifdef CPARSERVERBOSE
  printf("parser_fieldEventAfterISPart: field name not found in user_eventIn, user_eventOut or user_exposedField\n");
#endif
  return FALSE;
  }

 /* Now, retrieve the ProtoFieldDecl. */
 /* If the event name is an exposedField */
 if(pevE!=ID_UNDEFINED)
 {
  /* Search through the current PROTO definition for an exposed field that matches the field at index pevE in the user_exposedField array. */
  pfield=protoDefinition_getField(me->curPROTO, pevE, PKW_exposedField);
  if(!pfield)
   PARSE_ERROR("This exposedField is not member of current PROTO!")
 } else if(pevO!=ID_UNDEFINED) /* if the event is and eventOut or an eventIn */
 {
  assert(!pfield);
 
  /* Search through the current PROTO definition for an eventIn or eventOut that matches the field at index pevO in the user_eventIn or user_eventOut array. */
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
 /* i.e. the passed index into the EXPOSED_FIELD array is valid */
 if(evE!=ID_UNDEFINED)
 {
  #define EVENT_IN(n, f, t, v)
  #define EVENT_OUT(n, f, t, v)
  #define EXPOSED_FIELD(n, f, t, v) \
   PROCESS_EVENT(EXPOSED_FIELD, my, n, f, t, v)
  #define BEGIN_NODE(n) \
   EVENT_BEGIN_NODE(evE, ptr, n)

  /* All of this seems to just get the offset of the event evE in the node referenced by ptr and stores it in myOfs */
  /* This is an exposed_field */
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

 /* Otherwise, the passed index into the EVENT_IN or EVENT_OUT arrays must be valid */
 {
  assert(evO!=ID_UNDEFINED);
  assert((isIn || isOut) && !(isIn && isOut));

  #define BEGIN_NODE(n) \
   EVENT_BEGIN_NODE(evO, ptr, n)
  #define EXPOSED_FIELD(n, f, t, v)

 #undef END_NODE
 #define END_NODE(n) EVENT_END_NODE(n,EVENT_IN[evO])

  
  /* All of this seems to just get the offset of the event evO in the node referenced by ptr and stores it in myOfs */
  /* This is an eventIn */
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

  /* All of this seems to just get the offset of the event evO in the node referenced by ptr and stores it in myOfs */
  /* This is an eventOut */
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
 /* protoFeildDecl_addDestination(me, optr)  is #defined to vector_pushBack(struct OffsetPointer*, me->dests, optr) */
 /* Adds a new OffsetPointer structure (with node ptr and offset myOfs) to the dests vector list for the protofieldDeclaration pfield. */ 
  printf("adding node %p ofs %u to pfield %p\n", ptr, myOfs, pfield);
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
   printf("mallocing %d bytes\n", sizeof(vrml##type##T)); \
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
  for(i=0; i!=cnt; ++i) \
   if(!parser_sffloatValue(me, ret->dest+i)) \
    return FALSE; \
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

#ifdef OLDCODE
/* JAS this returns a pointer to a Multi_Int32 */
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

 *ret=MALLOC(sizeof(struct Multi_Int32));
 assert(*ret);

 (*ret)->n=3+width*height;
 (*ret)->p=MALLOC(sizeof(*(*ret)->p)*(*ret)->n);
 assert((*ret)->p);
 (*ret)->p[0]=width;
 (*ret)->p[1]=height;
 (*ret)->p[2]=depth;

 for(ptr=(*ret)->p+3; ptr!=(*ret)->p+(*ret)->n; ++ptr)
  if(!lexer_int32(me->lexer, ptr))
  {
   FREE_IF_NZ((*ret)->p);
   (*ret)->n=0;
   FREE_IF_NZ(*ret);
   return FALSE;
  }

 return TRUE;
}
#else
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

#endif

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

