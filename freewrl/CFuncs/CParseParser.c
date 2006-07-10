/* Sourcecode for CParseParser.h */

#include <stdlib.h>
#include <assert.h>

#include "CParseParser.h"
#include "CProto.h"

#define PARSE_ERROR(msg) \
 { \
  parseError(msg); \
  return FALSE; \
 }

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
 &parser_sfvec2fValue, &parser_sfvec2fValue,
 &parser_sfimageValue,
 NULL
};

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
 for(i=0; i!=vector_size(stack_top(PROTOs)); ++i)
  deleteProtoDefinition(vector_get(struct ProtoDefinition*,
   stack_top(PROTOs), i));
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
 if(!lexer_defineField(me->lexer, &name))
  PARSE_ERROR("Expected field-name ID after field type!")

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
 indexT fromFieldO;
 indexT fromFieldE;
 int fromOfs;
 int fromLen;

 indexT toNodeIndex;
 struct X3D_Node* toNode;
 indexT toFieldO;
 indexT toFieldE;
 int toOfs;
 int toLen;

 assert(me->lexer);
 lexer_skip(me->lexer);

 /* Is this a routeStatement? */
 if(!lexer_keyword(me->lexer, KW_ROUTE))
  return FALSE;

 /* Parse the elements. */
 #define ROUTE_PARSE_NODEFIELD(pre, eventType) \
  if(!lexer_nodeName(me->lexer, &pre##NodeIndex)) \
   PARSE_ERROR("Expected node-name in ROUTE-statement!") \
  assert(DEFedNodes && pre##NodeIndex<vector_size(DEFedNodes)); \
  pre##Node=vector_get(struct X3D_Node*, DEFedNodes, pre##NodeIndex); \
  if(!lexer_point(me->lexer)) \
   PARSE_ERROR("Expected . after node-name!") \
  if(!lexer_event##eventType(me->lexer, \
   &pre##FieldO, &pre##FieldE, NULL, NULL)) \
   PARSE_ERROR("Expected event" #eventType " after .!")

 ROUTE_PARSE_NODEFIELD(from, Out)
 if(!lexer_keyword(me->lexer, KW_TO))
  PARSE_ERROR("Expected TO in ROUTE-statement!")
 ROUTE_PARSE_NODEFIELD(to, In)

 /* Now, do the really hard macro work... */
 /* ************************************* */

 /* Redirect routing sizes for all the types. */
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

 /* Ignore the fields. */
 #define FIELD(n, f, t, v)

 /* General processing macros */
 #define PROCESS_EVENT(constPre, destPre, node, field, type, var) \
  case constPre##_##field: \
   destPre##Len=(ROUTE_REAL_SIZE_##type ? sizeof(node2->var) : -1); \
   destPre##Ofs=offsetof(struct X3D_##node, var); \
   break;
 #define EVENT_BEGIN_NODE(fieldInd, ptr, node) \
  case NODE_##node: \
  { \
   struct X3D_##node* node2=(struct X3D_##node*)ptr; \
   switch(fieldInd) \
   {
 #define END_NODE(node) \
   default: \
    PARSE_ERROR("Unsupported event for node!") \
   } \
   break; \
  }
 #define EVENT_NODE_DEFAULT \
  default: \
   PARSE_ERROR("Unsupported node!")
 
 /* Process from eventOut */
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
 else
 {
  assert(fromFieldO!=ID_UNDEFINED);
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
 else
 {
  assert(toFieldO!=ID_UNDEFINED);
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
 
 /* FIXME:  Not a really safe check for types in ROUTE! */
 if(fromLen!=toLen)
  PARSE_ERROR("Types mismatch in ROUTE!")

 /* Finally, register the route. */
 /* **************************** */

 {
  /* 10+1+3+1=15:  Number <5000000000, :, number < 999, \0 */
  char tonode_str[15];
  snprintf(tonode_str, 15, "%lu:%d", toNode, toOfs);

  CRoutes_Register(1, fromNode, fromOfs, 1, tonode_str, toLen, 
   returnInterpolatorPointer(stringNodeType(toNode->_nodeType)), 0, 0);
 }
 
 return TRUE;
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

  assert(DEFedNodes && ind<vector_size(stack_top(DEFedNodes)));

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
 } else
 {
  assert(nodeTypeU!=ID_UNDEFINED);
  assert(PROTOs);
  assert(!stack_empty(PROTOs));
  assert(nodeTypeU<vector_size(stack_top(PROTOs)));
  node=protoDefinition_instantiate(vector_get(struct ProtoDefinition*,
   stack_top(PROTOs), nodeTypeU));
  assert(node);
 }
 assert(node);

 if(!lexer_closeCurly(me->lexer))
  parseError("Expected } after fields of node!");

 *ret=node;
 return TRUE;
}

/* add_parent for Multi_Node */
void mfnode_add_parent(struct Multi_Node* node, struct X3D_Node* parent)
{
 int i;
 for(i=0; i!=node->n; ++i)
  add_parent(node->p[i], parent);
}

/* Parses a field and sets it in node */
BOOL parser_field(struct VRMLParser* me, struct X3D_Node* node)
{
 indexT fieldO;
 indexT fieldE;

 assert(me->lexer);
 if(!lexer_field(me->lexer, &fieldO, &fieldE, NULL, NULL))
  return FALSE;

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
 
 /* Process a field (either exposed or ordinary) generally */
 #define PROCESS_FIELD(exposed, node, field, fieldType, var) \
  case exposed##FIELD_##field: \
   if(!parser_##fieldType##Value(me, (void*)&node2->var)) \
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
