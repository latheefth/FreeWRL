/* Sourcecode for CParseParser.h */

#include <stdlib.h>
#include <assert.h>

#include "CParseParser.h"
#include "CProto.h"

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
   (ROUTE_REAL_SIZE_##type ? sizeof_member(struct X3D_##node, var) : -1); \
  destPre##Ofs=offsetof(struct X3D_##node, var); \
  break;
#define EVENT_BEGIN_NODE(fieldInd, ptr, node) \
 case NODE_##node: \
 { \
  struct X3D_##node* node2=(struct X3D_##node*)ptr; \
  switch(fieldInd) \
  {
#define EVENT_END_NODE(node) \
  default: \
   PARSE_ERROR("Unsupported event for node!") \
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
 struct VRMLParser* ret=malloc(sizeof(struct VRMLParser));
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

 free(me);
}

static void parser_scopeOut_DEFUSE();
static void parser_scopeOut_PROTO();
void parser_destroyData()
{
 lexer_destroyData();

 /* DEFed Nodes. */
 if(DEFedNodes)
 {
  while(!stack_empty(DEFedNodes))
   parser_scopeOut_DEFUSE();
  deleteStack(DEFedNodes);
  DEFedNodes=NULL;
 }
 assert(!DEFedNodes);

 /* PROTOs */
 if(PROTOs)
 {
  while(!stack_empty(PROTOs))
   parser_scopeOut_PROTO();
  deleteStack(PROTOs);
  PROTOs=NULL;
 }
 assert(!PROTOs);
}

/* Scoping */

static void parser_scopeIn_DEFUSE()
{
 if(!DEFedNodes)
  DEFedNodes=newStack();
 stack_push(DEFedNodes, newVector(struct X3D_Node*, DEFMEM_INIT_SIZE));
}
static void parser_scopeIn_PROTO()
{
 if(!PROTOs)
  PROTOs=newStack();
 stack_push(PROTOs, newVector(struct ProtoDefinition*, DEFMEM_INIT_SIZE));
}

static void parser_scopeOut_DEFUSE()
{
 indexT i;
 assert(!stack_empty(DEFedNodes));
 /* FIXME:  Can't delete individual nodes, as they might be referenced! */
 deleteVector(struct X3D_Node*, stack_top(DEFedNodes));
 stack_pop(DEFedNodes);
}

static void parser_scopeOut_PROTO()
{
 indexT i;
 /* Do not delete the ProtoDefinitions, as they are referenced in the scene
  * graph!  TODO:  How to delte them properly? */
 deleteVector(struct ProtoDefinition*, stack_top(PROTOs));
 stack_pop(PROTOs);
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
 struct ProtoDefinition* proto)
{
 indexT mode;
 indexT type;
 indexT name;
 struct ProtoFieldDecl* decl=NULL;

 if(!lexer_protoFieldMode(me->lexer, &mode))
  return FALSE;
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

 decl=newProtoFieldDecl(mode, type, name);
 if(mode==PKW_field || mode==PKW_exposedField)
 {
  assert(PARSE_TYPE[type]);
  if(!PARSE_TYPE[type](me, (void*)&decl->defaultVal))
  {
   parseError("Expected default value for field!");
   deleteProtoFieldDecl(decl);
   return FALSE;
  }
 }

 protoDefinition_addIfaceField(proto, decl);
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
 if(!PROTOs || stack_empty(PROTOs))
  parser_scopeIn_PROTO();
 assert(PROTOs);
 assert(!stack_empty(PROTOs));
 assert(name==vector_size(stack_top(PROTOs)));
 vector_pushBack(struct ProtoDefinition*, stack_top(PROTOs), obj);

 /* Interface declarations */
 if(!lexer_openSquare(me->lexer))
  PARSE_ERROR("Expected [ to start interface declaration!")
 while(parser_interfaceDeclaration(me, obj));
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
 indexT fromFieldO;
 indexT fromFieldE;
 indexT fromUFieldO;
 indexT fromUFieldE;
 int fromOfs;
 int fromLen;
 struct ProtoFieldDecl* fromField=NULL;

 indexT toNodeIndex;
 struct X3D_Node* toNode;
 struct ProtoDefinition* toProto=NULL;
 indexT toFieldO;
 indexT toFieldE;
 indexT toUFieldO;
 indexT toUFieldE;
 int toOfs;
 int toLen;
 struct ProtoFieldDecl* toField=NULL;

 assert(me->lexer);
 lexer_skip(me->lexer);

 /* Is this a routeStatement? */
 if(!lexer_keyword(me->lexer, KW_ROUTE))
  return FALSE;

 /* Parse the elements. */
 #define ROUTE_PARSE_NODEFIELD(pre, eventType) \
  /* Target-node */ \
  if(!lexer_nodeName(me->lexer, &pre##NodeIndex)) \
   PARSE_ERROR("Expected node-name in ROUTE-statement!") \
  assert(DEFedNodes && !stack_empty(DEFedNodes) && \
   pre##NodeIndex<vector_size(stack_top(DEFedNodes))); \
  pre##Node=vector_get(struct X3D_Node*, stack_top(DEFedNodes), \
   pre##NodeIndex); \
  if(pre##Node->_nodeType==NODE_Group) \
   pre##Proto=X3D_GROUP(pre##Node)->__protoDef; \
  else \
   assert(!pre##Proto); \
  /* Seperating '.' */ \
  if(!lexer_point(me->lexer)) \
   PARSE_ERROR("Expected . after node-name!") \
  /* Field, user/built-in depending on whether node is a PROTO instance */ \
  if(!pre##Proto) \
  { \
   if(!lexer_event##eventType(me->lexer, \
    &pre##FieldO, &pre##FieldE, NULL, NULL)) \
    PARSE_ERROR("Expected built-in event" #eventType " after .!") \
  } else \
  { \
   assert(pre##Proto); \
   if(lexer_event##eventType(me->lexer, \
    NULL, NULL, &pre##UFieldO, &pre##UFieldE)) \
   { \
    if(pre##UFieldO!=ID_UNDEFINED) \
     pre##Field=protoDefinition_getField(pre##Proto, pre##UFieldO, \
      PKW_event##eventType); \
    else \
    { \
     assert(pre##UFieldE!=ID_UNDEFINED); \
     pre##Field=protoDefinition_getField(pre##Proto, pre##UFieldE, \
      PKW_exposedField); \
    } \
    if(!pre##Field) \
     PARSE_ERROR("Event-field invalid for this PROTO!") \
   } \
  }

 ROUTE_PARSE_NODEFIELD(from, Out)
 if(!lexer_keyword(me->lexer, KW_TO))
  PARSE_ERROR("Expected TO in ROUTE-statement!")
 ROUTE_PARSE_NODEFIELD(to, In)

 /* Now, do the really hard macro work... */
 /* ************************************* */

 /* Ignore the fields. */
 #define FIELD(n, f, t, v)

 #define END_NODE(n) \
  EVENT_END_NODE(n)
 
 /* Process from eventOut */
 if(!fromField)
  if(fromFieldE!=ID_UNDEFINED)
   switch(fromNode->_nodeType)
   {
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
  else if(fromFieldO!=ID_UNDEFINED)
  {
   switch(fromNode->_nodeType)
   {
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

 /* Process to eventIn */
 if(!toField)
  if(toFieldE!=ID_UNDEFINED)
   switch(toNode->_nodeType)
   {
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
  else if(toFieldO!=ID_UNDEFINED)
  {
   switch(toNode->_nodeType)
   {
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
 if(fromField)
  fromLen=protoFieldDecl_getLength(fromField);
 if(toField)
  toLen=protoFieldDecl_getLength(toField);

 /* FIXME:  Not a really safe check for types in ROUTE! */
 if(fromLen!=toLen)
  PARSE_ERROR("Types mismatch in ROUTE!")

 /* Finally, register the route. */
 /* **************************** */

 /* Built-in to built-in */
 if(!fromField && !toField)
  parser_registerRoute(me, fromNode, fromOfs, toNode, toOfs, toLen);
 /* Built-in to user-def */
 else if(!fromField && toField)
  protoFieldDecl_routeTo(toField, fromNode, fromOfs, me);
 /* user-def to built-in */
 else if(fromField && !toField)
  protoFieldDecl_routeFrom(fromField, toNode, toOfs, me);
 /* user-def to user-def */
 else
  PARSE_ERROR("Routing from user-event to user-event is currently unsupported!")

 return TRUE;
}

/* Register a ROUTE here */
void parser_registerRoute(struct VRMLParser* me,
 struct X3D_Node* fromNode, unsigned fromOfs,
 struct X3D_Node* toNode, unsigned toOfs,
 size_t len)
{
 if(me->curPROTO)
 {
  protoDefinition_addRoute(me->curPROTO,
   newProtoRoute(fromNode, fromOfs, toNode, toOfs, len));
 } else
  CRoutes_RegisterSimple(fromNode, fromOfs, toNode, toOfs, len);
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

  assert(ind<=vector_size(stack_top(DEFedNodes)));
  if(ind==vector_size(stack_top(DEFedNodes)))
   vector_pushBack(struct X3D_Node*, stack_top(DEFedNodes), NULL);
  assert(ind<vector_size(stack_top(DEFedNodes)));

  if(!parser_node(me, &vector_get(struct X3D_Node*,
   stack_top(DEFedNodes), ind)))
   PARSE_ERROR("Expected node in DEF statement!\n")

  *ret=vector_get(struct X3D_Node*, stack_top(DEFedNodes), ind);
  return TRUE;
 }

 /* A USE-statement? */
 if(lexer_keyword(me->lexer, KW_USE))
 {
  indexT ind;

  if(!lexer_nodeName(me->lexer, &ind))
   PARSE_ERROR("Expected nodeNameId after USE!\n")
  assert(ind!=ID_UNDEFINED);

  assert(DEFedNodes && !stack_empty(DEFedNodes) &&
   ind<vector_size(stack_top(DEFedNodes)));

  *ret=vector_get(struct X3D_Node*, stack_top(DEFedNodes), ind);
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
  node=X3D_NODE(createNewX3DNode(nodeTypeB));
  assert(node);
  while(parser_field(me, node) ||
   parser_routeStatement(me) || parser_protoStatement(me));
 }
 
 /* Proto */
 else
 {
  /* The copy of our ProtoDefinition; here are the fields filled in. */
  struct ProtoDefinition* protoCopy;

  assert(nodeTypeU!=ID_UNDEFINED);
  assert(PROTOs);
  assert(!stack_empty(PROTOs));
  assert(nodeTypeU<vector_size(stack_top(PROTOs)));

  protoCopy=protoDefinition_copy(vector_get(struct ProtoDefinition*,
   stack_top(PROTOs), nodeTypeU));
  while(parser_protoField(me, protoCopy) ||
   parser_routeStatement(me) || parser_protoStatement(me));
  node=protoDefinition_extractScene(protoCopy);
  assert(node);

  /* Can't delete ProtoDefinition, it is referenced! */
  /*deleteProtoDefinition(protoCopy);*/
 }
 assert(node);

 if(!lexer_closeCurly(me->lexer))
  parseError("Expected } after fields of node!");

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
  if(!parser_fieldValue(me, newOffsetPointer(&val, 0), field->type))
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
 indexT type)
{
 #define PARSER_FINALLY \
  deleteOffsetPointer(ret);

 /* If we are inside a PROTO, IS is possible */
 if(me->curPROTO && lexer_keyword(me->lexer, KW_IS))
 {
  indexT fieldO, fieldE;
  struct ProtoFieldDecl* pField=NULL;

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
 #define PROCESS_FIELD(exposed, node, field, fieldType, var) \
  case exposed##FIELD_##field: \
   if(!parser_fieldValue(me, \
    newOffsetPointer(node2, offsetof(struct X3D_##node, var)), \
    FTIND_##fieldType)) \
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
    PROCESS_FIELD(EXPOSED_, node, field, fieldType, var)

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
    PROCESS_FIELD(, node, field, fieldType, var)

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
 BOOL isIn;	/* EventIn?  Otherwise EventOut, of course */
 indexT evO, evE;
 indexT pevO, pevE;
 struct ProtoFieldDecl* pfield=NULL;
 unsigned myOfs;
 size_t myLen;

 /* Check and gain information */
 /* ************************** */

 /* We should be in a PROTO */
 if(!me->curPROTO)
  return FALSE;

 /* There should be really either an eventIn or an eventOut... */
 if(lexer_eventIn(me->lexer, &evO, &evE, NULL, NULL))
  isIn=TRUE;
 else if(lexer_eventOut(me->lexer, &evO, &evE, NULL, NULL))
  isIn=FALSE;
 else
  return FALSE;
 
 /* Then, there should be 'IS' */
 if(!lexer_keyword(me->lexer, KW_IS))
  PARSE_ERROR("Expected \'IS\' after event used in node\'s field-section!")

 /* And finally the PROTO's event to be linked */
 if(isIn)
 {
  if(!lexer_eventIn(me->lexer, NULL, NULL, &pevO, &pevE))
   PARSE_ERROR("Need user-eventIn after IS!")
 } else
  if(!lexer_eventOut(me->lexer, NULL, NULL, &pevO, &pevE))
   PARSE_ERROR("Need user-eventOut after IS!")

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
 #define END_NODE(n) \
  EVENT_END_NODE(n)

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

  #define BEGIN_NODE(n) \
   EVENT_BEGIN_NODE(evO, ptr, n)
  #define EXPOSED_FIELD(n, f, t, v)
  
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

  /* eventOut */
  else
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
   ret->p=malloc(sizeof(vrml##type##T)); \
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
    parseError("Expected ] before end of MF-Value!"); \
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

 *ret=malloc(sizeof(struct Multi_Int32));
 assert(*ret);

 (*ret)->n=3+width*height;
 (*ret)->p=malloc(sizeof(*(*ret)->p)*(*ret)->n);
 assert((*ret)->p);
 (*ret)->p[0]=width;
 (*ret)->p[1]=height;
 (*ret)->p[2]=depth;

 for(ptr=(*ret)->p+3; ptr!=(*ret)->p+(*ret)->n; ++ptr)
  if(!lexer_int32(me->lexer, ptr))
  {
   free((*ret)->p);
   (*ret)->p=NULL;
   (*ret)->n=0;
   free(*ret);
   *ret=NULL;
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
