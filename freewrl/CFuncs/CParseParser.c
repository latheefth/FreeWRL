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

/* Constructor and destructor */

struct VRMLParser* newParser()
{
 struct VRMLParser* ret=malloc(sizeof(struct VRMLParser));
 ret->lexer=newLexer();
 assert(ret->lexer);

 return ret;
}

void deleteParser(struct VRMLParser* me)
{
 assert(me->lexer);
 deleteLexer(me->lexer);

 free(me);
}

/* Nodes and fields */
/* **************** */

/* Parses a nodeStatement */
BOOL parser_nodeStatement(struct VRMLParser* me, vrmlNodeT* ret)
{
 assert(me->lexer);

 /* A DEF-statement? */
 if(lexer_keyword(me->lexer, KW_DEF))
 {
  indexT ind;

  if(!lexer_nodeName(me->lexer, &ind))
   PARSE_ERROR("Expected nodeNameId after DEF!\n")
  assert(ind&USER_ID);
  ind&=~USER_ID;

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

  if(!lexer_nodeName(me->lexer, &ind))
   PARSE_ERROR("Expected nodeNameId after USE!\n")
  assert(ind&USER_ID);
  ind&=~USER_ID;

  if(!DEFedNodes || ind>=vector_size(DEFedNodes))
   PARSE_ERROR("Undefined nodeNameId referenced!\n")

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
 
 if(!lexer_node(me->lexer, &nodeType))
  return FALSE;

 if(!lexer_openCurly(me->lexer))
  PARSE_ERROR("Expected { after node-type id!")

 node=X3D_NODE(createNewX3DNode(nodeType));

 assert(node);
 while(parser_field(me, node));

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
 indexT field;

 assert(me->lexer);
 if(!lexer_fieldName(me->lexer, &field))
  return FALSE;

 switch(node->_nodeType)
 {

  /* Processes fields for one given node-type */
  #define PROC_FIELDS_BEGIN(type) \
   case NODE_##type: \
   { \
    struct X3D_##type* node2=(struct X3D_##type*)node; \
    switch(field) \
    {
  #define PROC_FIELDS_END \
     default: \
      PARSE_ERROR("Unsupported field for node!") \
    } \
   } \
   break;

  /* Allow field field of type fieldType and store it to node2->var. */
  #define PROC_FIELD(field, fieldType, var) \
   case FIELDNAMES_##field: \
    if(!parser_##fieldType##Value(me, (void*)&node2->var)) \
     PARSE_ERROR("Expected " #fieldType "Value!") \
    break;
  #define PROC_SFNODE_FIELD(field, var) \
   case FIELDNAMES_##field: \
    if(!parser_sfnodeValue(me, (void*)&node2->var)) \
     PARSE_ERROR("Expected sfnodeValue!") \
    add_parent(node2->var, node2); \
    break;
  #define PROC_MFNODE_FIELD(field, var) \
   case FIELDNAMES_##field: \
    if(!parser_mfnodeValue(me, (void*)&node2->var)) \
     PARSE_ERROR("Expected mfnodeValue!") \
    mfnode_add_parent(&node2->var, X3D_NODE(node2)); \
    break;

  /* Appearance node */
  PROC_FIELDS_BEGIN(Appearance)
   PROC_SFNODE_FIELD(material, material)
  PROC_FIELDS_END

  /* Coordinate node */
  PROC_FIELDS_BEGIN(Coordinate)
   PROC_FIELD(point, mfvec3f, point)
  PROC_FIELDS_END

  /* ElevationGrid node */
  /*
  PROC_FIELDS_BEGIN(ElevationGrid)
   PROC_FIELD(xDimension, sfint32, xDimension)
   PROC_FIELD(zDimension, sfint32, zDimension)
   PROC_FIELD(height, mffloat, height)
  PROC_FIELDS_END
  */

  /* Group node */
  PROC_FIELDS_BEGIN(Group)
   PROC_MFNODE_FIELD(children, children)
  PROC_FIELDS_END

  /* IndexedFaceSet node */
  PROC_FIELDS_BEGIN(IndexedFaceSet)
   PROC_SFNODE_FIELD(coord, coord)
   PROC_FIELD(coordIndex, mfint32, coordIndex)
   PROC_FIELD(solid, sfbool, solid)
  PROC_FIELDS_END

  /* Material node */
  PROC_FIELDS_BEGIN(Material)
   PROC_FIELD(diffuseColor, sfcolor, diffuseColor)
   PROC_FIELD(emissiveColor, sfcolor, emissiveColor)
   PROC_FIELD(transparency, sffloat, transparency)
  PROC_FIELDS_END

  /* Shape node */
  PROC_FIELDS_BEGIN(Shape)
   PROC_SFNODE_FIELD(appearance, appearance)
   PROC_SFNODE_FIELD(geometry, geometry)
  PROC_FIELDS_END

  /* Sphere node */
  PROC_FIELDS_BEGIN(Sphere)
   PROC_FIELD(radius, sffloat, radius)
  PROC_FIELDS_END

  /* Text node */
  PROC_FIELDS_BEGIN(Text)
   PROC_FIELD(string, mfstring, string)
  PROC_FIELDS_END

  /* Transform node */
  PROC_FIELDS_BEGIN(Transform)
   PROC_FIELD(translation, sfvec3f, translation)
   PROC_MFNODE_FIELD(children, children)
  PROC_FIELDS_END

  /* Unsupported node */
  default:
   parseError("Unsupported node!");
   return FALSE;

 }

 return TRUE;
}

/* MF* field values */
/* **************** */

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

PARSER_MFFIELD(color, Color)
PARSER_MFFIELD(float, Float)
PARSER_MFFIELD(int32, Int32)
PARSER_MFFIELD(node, Node)
PARSER_MFFIELD(rotation, Rotation)
PARSER_MFFIELD(string, String)
PARSER_MFFIELD(time, Time)
PARSER_MFFIELD(vec2f, Vec2f)
PARSER_MFFIELD(vec3f, Vec3f)

/* SF* field values */
/* **************** */

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
