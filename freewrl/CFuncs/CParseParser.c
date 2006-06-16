/* Sourcecode for CParseParser.h */

#include <stdlib.h>
#include <assert.h>

#include "CParseParser.h"

#define PARSE_ERROR(msg) \
 { \
  parseError(msg); \
  return FALSE; \
 }

#define DEFMEM_INIT_SIZE	16

/* The DEF/USE memory. */
struct Vector* DEFedNodes=NULL;

/* ************************************************************************** */
/* Constructor and destructor */

struct VRMLParser* newParser(void* ptr, unsigned ofs)
{
 struct VRMLParser* ret=malloc(sizeof(struct VRMLParser));
 ret->lexer=newLexer();
 assert(ret->lexer);
 ret->ptr=ptr;
 ret->ofs=ofs;

 return ret;
}

void deleteParser(struct VRMLParser* me)
{
 assert(me->lexer);
 deleteLexer(me->lexer);

 free(me);
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

  break;
 }

 return lexer_eof(me->lexer);
}

/* ************************************************************************** */
/* Nodes and fields */

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
  if(!lexer_nodeName(me->lexer, &pre##NodeIndex, NULL)) \
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

  if(!DEFedNodes)
   DEFedNodes=newVector(struct X3D_Node*, DEFMEM_INIT_SIZE);
  assert(DEFedNodes);

  assert(ind<=vector_size(DEFedNodes));
  if(ind==vector_size(DEFedNodes))
   vector_pushBack(struct X3D_Node*, DEFedNodes, NULL);
  assert(ind<vector_size(DEFedNodes));

  if(!parser_node(me, &vector_get(struct X3D_Node*, DEFedNodes, ind)))
   PARSE_ERROR("Expected node in DEF statement!\n")

  *ret=vector_get(struct X3D_Node*, DEFedNodes, ind);
  return TRUE;
 }

 /* A USE-statement? */
 if(lexer_keyword(me->lexer, KW_USE))
 {
  indexT ind;

  if(!lexer_nodeName(me->lexer, NULL, &ind))
   PARSE_ERROR("Expected nodeNameId after USE!\n")
  assert(ind!=ID_UNDEFINED);

  assert(DEFedNodes && ind<vector_size(DEFedNodes));

  *ret=vector_get(struct X3D_Node*, DEFedNodes, ind);
  return TRUE;
 }

 /* Otherwise, simply a node. */
 return parser_node(me, ret);
}

/* Parses a node (node non-terminal) */
BOOL parser_node(struct VRMLParser* me, vrmlNodeT* ret)
{
 indexT nodeType;
 struct X3D_Node* node=NULL;

 assert(me->lexer);
 
 if(!lexer_node(me->lexer, &nodeType, NULL))
  return FALSE;
 assert(nodeType!=ID_UNDEFINED);

 if(!lexer_openCurly(me->lexer))
  PARSE_ERROR("Expected { after node-type id!")

 node=X3D_NODE(createNewX3DNode(nodeType));

 assert(node);
 while(parser_field(me, node) || parser_routeStatement(me));

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
 #define INIT_CODE_mfimage(var)
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
   assert(!ret->n && !ret->p); \
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
  assert(!ret->n && !ret->p); \
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
