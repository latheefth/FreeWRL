/* CProto.c - Sourcecode for CProto.h */
/* TODO:  Pointer updating could be made more efficient! */

#include <stdlib.h>
#include <assert.h>

#include "CProto.h"
#include "CParseGeneral.h"

/* ************************************************************************** */
/* ******************************* ProtoFieldDecl *************************** */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

/* Without default value (event) */
struct ProtoFieldDecl* newProtoFieldDecl(indexT mode, indexT type, indexT name)
{
 struct ProtoFieldDecl* ret=malloc(sizeof(struct ProtoFieldDecl));
 ret->mode=mode;
 ret->type=type;
 ret->name=name;
 ret->alreadySet=FALSE;
 ret->dests=newVector(void*, 4);
 assert(ret->dests);
 return ret;
}

void deleteProtoFieldDecl(struct ProtoFieldDecl* me)
{
 deleteVector(void*, me->dests);
 free(me);
}

/* Other members */
/* ************* */

/* Add destinations to innerPtrs vector */
void protoFieldDecl_addInnerPointersPointers(struct ProtoFieldDecl* me,
 struct Vector* v)
{
 size_t i;
 for(i=0; i!=vector_size(me->dests); ++i)
  vector_pushBack(void**, v, &vector_get(void*, me->dests, i));
}

/* setValue is at the end, because we need deep-copying there */
/* copy is at the end, too, because defaultVal needs to be deep-copied. */

/* ************************************************************************** */
/* ******************************** ProtoDefinition ************************* */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

struct ProtoDefinition* newProtoDefinition()
{
 /* Attention!  protoDefinition_copy also sets up data from raw malloc!  Don't
  * forget to mimic any changes to this method there, too!
  */

 struct ProtoDefinition* ret=malloc(sizeof(struct ProtoDefinition));
 assert(ret);
 ret->tree=createNewX3DNode(NODE_Group);
 assert(ret->tree);
 ret->tree->__isProto=TRUE;

 ret->iface=newVector(struct ProtoFieldDecl*, 4);
 assert(ret->iface);

 ret->innerPtrs=NULL;

 return ret;
}

void deleteProtoDefinition(struct ProtoDefinition* me)
{
 /* FIXME:  Not deep-destroying nodes!!! */
 /* If tree is NULL, it is already extracted! */
 if(me->tree)
  free(me->tree);

 {
  size_t i;
  for(i=0; i!=vector_size(me->iface); ++i)
   deleteProtoFieldDecl(vector_get(struct ProtoFieldDecl*, me->iface, i));
  deleteVector(struct ProtoDefinition*, me->iface);
 }

 if(me->innerPtrs)
  deleteVector(void**, me->innerPtrs);
 
 free(me);
}

/* Other members */
/* ************* */

/* Retrieve a field by its "name" */
struct ProtoFieldDecl* protoDefinition_getField(struct ProtoDefinition* me,
 indexT ind, indexT mode)
{
 /* TODO:  O(log(n)) by sorting */
 size_t i;
 for(i=0; i!=vector_size(me->iface); ++i)
 {
  struct ProtoFieldDecl* f=vector_get(struct ProtoFieldDecl*, me->iface, i);
  if(f->name==ind && f->mode==mode)
   return f;
 }

 return NULL;
}

/* Add a node */
void protoDefinition_addNode(struct ProtoDefinition* me, struct X3D_Node* node)
{
 assert(me);
 assert(me->tree);
 add_parent(node, me->tree);
 addToNode(me->tree, offsetof(struct X3D_Group, children), node);
}

/* Copies the PROTO */
struct ProtoDefinition* protoDefinition_copy(struct ProtoDefinition* me)
{
 struct ProtoDefinition* ret=malloc(sizeof(struct ProtoDefinition));
 size_t i;
 assert(ret);

 /* Copy interface */
 ret->iface=newVector(struct ProtoFieldDecl*, vector_size(me->iface));
 assert(ret->iface);
 for(i=0; i!=vector_size(me->iface); ++i)
  vector_pushBack(struct ProtoFieldDecl*, ret->iface,
   protoFieldDecl_copy(vector_get(struct ProtoFieldDecl*, me->iface, i)));

 /* Fill inner pointers */
 ret->innerPtrs=NULL;
 protoDefinition_fillInnerPtrs(ret);

 /* Copy the scene graph and fill the fields thereby */
 ret->tree=protoDefinition_deepCopy(me->tree, ret);
 ret->tree->__isProto=TRUE;

 return ret;
}

/* Extracts the scene graph */
struct X3D_Group* protoDefinition_extractScene(struct ProtoDefinition* me)
{
 size_t i;
 struct X3D_Group* ret=me->tree;
 assert(ret);
 me->tree=NULL;

 /* Finish all fields now */
 for(i=0; i!=vector_size(me->iface); ++i)
  protoFieldDecl_finish(vector_get(struct ProtoFieldDecl*, me->iface, i));
 
 return ret;
}

/* Update pointers; only some pointer-arithmetic here */
void protoDefinition_doPtrUpdate(struct ProtoDefinition* me,
 uint8_t* beg, uint8_t* end, uint8_t* newPos)
{
 size_t i;
 assert(me->innerPtrs);
 for(i=0; i!=vector_size(me->innerPtrs); ++i)
 {
  uint8_t** curPtr=vector_get(uint8_t**, me->innerPtrs, i);
  if(*curPtr>=beg && *curPtr<end)
   *curPtr=newPos+(*curPtr-beg);
 }
}

/* Fills the innerPtrs field */
void protoDefinition_fillInnerPtrs(struct ProtoDefinition* me)
{
 size_t i;

 assert(!me->innerPtrs);
 me->innerPtrs=newVector(void**, 8);

 for(i=0; i!=vector_size(me->iface); ++i)
  protoFieldDecl_addInnerPointersPointers(
   vector_get(struct ProtoFieldDecl*, me->iface, i), me->innerPtrs);
}

/* Deep copying */
/* ************ */

/* Deepcopies sf */
#define DEEPCOPY_sfbool(v, i) v
#define DEEPCOPY_sfcolor(v, i) v
#define DEEPCOPY_sfcolorrgba(v, i) v
#define DEEPCOPY_sffloat(v, i) v
#define DEEPCOPY_sfimage(v, i) deepcopy_sfimage(v, i)
#define DEEPCOPY_sfint32(v, i) v
#define DEEPCOPY_sfnode(v, i) protoDefinition_deepCopy(v, i)
#define DEEPCOPY_sfrotation(v, i) v
#define DEEPCOPY_sfstring(v, i) deepcopy_sfstring(v)
#define DEEPCOPY_sftime(v, i) v
#define DEEPCOPY_sfvec2f(v, i) v
#define DEEPCOPY_sfvec3f(v, i) v
static struct Multi_Int32 DEEPCOPY_mfint32(struct Multi_Int32,
 struct ProtoDefinition*);
static vrmlImageT deepcopy_sfimage(vrmlImageT img,
 struct ProtoDefinition* new)
{
 vrmlImageT ret=malloc(sizeof(*img));
 *ret=DEEPCOPY_mfint32(*img, new);
 return ret;
}
static vrmlStringT deepcopy_sfstring(vrmlStringT str)
{
 char* cstr;
 STRLEN len;

 cstr=SvPV(str, len);
 return EAI_newSVpv(cstr);
}

/* Deepcopies a mf* */
#define DEEPCOPY_MFVALUE(type, stype) \
 static struct Multi_##stype DEEPCOPY_mf##type(struct Multi_##stype src, \
  struct ProtoDefinition* new) \
 { \
  int i; \
  struct Multi_##stype dest; \
  dest.n=src.n; \
  dest.p=malloc(sizeof(src.p[0])*src.n); \
  for(i=0; i!=src.n; ++i) \
   dest.p[i]=DEEPCOPY_sf##type(src.p[i], new); \
  if(new) \
   protoDefinition_doPtrUpdate(new, \
    (uint8_t*)src.p, (uint8_t*)(src.p+src.n), (uint8_t*)dest.p); \
  return dest; \
 }
DEEPCOPY_MFVALUE(bool, Bool)
DEEPCOPY_MFVALUE(color, Color)
DEEPCOPY_MFVALUE(colorrgba, ColorRGBA)
DEEPCOPY_MFVALUE(float, Float)
DEEPCOPY_MFVALUE(int32, Int32)
DEEPCOPY_MFVALUE(node, Node)
DEEPCOPY_MFVALUE(rotation, Rotation)
DEEPCOPY_MFVALUE(string, String)
DEEPCOPY_MFVALUE(time, Time)
DEEPCOPY_MFVALUE(vec2f, Vec2f)
DEEPCOPY_MFVALUE(vec3f, Vec3f)

/* ************************************************************************** */

/* Nodes; may be used to update the interface-pointers, too. */
struct X3D_Node* protoDefinition_deepCopy(struct X3D_Node* node,
 struct ProtoDefinition* new)
{
 struct X3D_Node* ret;

 /* If we get nothing, what can we return? */
 if(!node) return NULL;

 /* Create it */
 ret=createNewX3DNode(node->_nodeType);

 /* Copy the fields using the NodeFields.h file */
 switch(node->_nodeType)
 {

  #define BEGIN_NODE(n) \
   case NODE_##n: \
   { \
    struct X3D_##n* node2=(struct X3D_##n*)node; \
    struct X3D_##n* ret2=(struct X3D_##n*)ret; \
    if(new) \
     protoDefinition_doPtrUpdate(new, \
      (uint8_t*)node2, ((uint8_t*)node2)+sizeof(struct X3D_##n), \
      (uint8_t*)ret2);
  #define END_NODE(n) \
    break; \
   }

  /* Copying of fields depending on type */

  #define FIELD(n, field, type, var) \
   ret2->var=DEEPCOPY_##type(node2->var, new);

  #define EVENT_IN(n, f, t, v)
  #define EVENT_OUT(n, f, t, v)
  #define EXPOSED_FIELD(n, field, type, var) \
   FIELD(n, field, type, var)

  #include "NodeFields.h"

  #undef BEGIN_NODE
  #undef END_NODE
  #undef EVENT_IN
  #undef EVENT_OUT
  #undef EXOSED_FIELD
  #undef FIELD

  default:
   fprintf(stderr, "Nodetype %lu unsupported for deep-copy...\n",
    node->_nodeType);
   break;

 }

 return ret;
}

/* ************************************************************************** */

/* Set a field's value */
void protoFieldDecl_setValue(struct ProtoFieldDecl* me, union anyVrml* val)
{
 size_t i;

 assert(!me->alreadySet);
 me->alreadySet=TRUE;

 /* If there are no targets, destroy the value */
 if(vector_empty(me->dests))
 {
  /* FIXME:  Free (destroy) value!!! */
  return;
 }

 /* Otherwise, assign first target to val */
 switch(me->type)
 {
  #define SF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: \
    *vector_get(vrml##ttype##T*, me->dests, 0)=val->type; \
    break;
  #define MF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: \
    *vector_get(struct Multi_##ttype*, me->dests, 0)=val->type; \
    break;
  #include "VrmlTypeList.h"
  #undef SF_TYPE
  #undef MF_TYPE

  default:
   parseError("Error: Unsupported type for PROTO field!");
 }

 /* and copy it to the others */
 for(i=1; i!=vector_size(me->dests); ++i)
 {
  switch(me->type)
  {
   #define SF_TYPE(fttype, type, ttype) \
    case FIELDTYPE_##fttype: \
     *vector_get(vrml##ttype##T*, me->dests, i)= \
      DEEPCOPY_##type(val->type, NULL); \
     break;
   #define MF_TYPE(fttype, type, ttype) \
    case FIELDTYPE_##fttype: \
     *vector_get(struct Multi_##ttype*, me->dests, i)= \
      DEEPCOPY_##type(val->type, NULL); \
     break;
   #include "VrmlTypeList.h"
   #undef SF_TYPE
   #undef MF_TYPE

   default:
    parseError("Error: Unsupported type for PROTO field!!!");
  }
 }
}

/* Copies a fieldDeclaration */
struct ProtoFieldDecl* protoFieldDecl_copy(struct ProtoFieldDecl* me)
{
 struct ProtoFieldDecl* ret=newProtoFieldDecl(me->mode, me->type, me->name);
 size_t i;
 ret->alreadySet=FALSE;

 /* Copy destination pointers */
 for(i=0; i!=vector_size(me->dests); ++i)
  vector_pushBack(void*, ret->dests, vector_get(void*, me->dests, i));

 /* Copy default value */
 switch(me->type)
 {
  #define SF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: \
    ret->defaultVal.type=DEEPCOPY_##type(me->defaultVal.type, NULL); \
    break;
  #define MF_TYPE(fttype, type, ttype) \
   SF_TYPE(fttype, type, ttype)
  #include "VrmlTypeList.h"
  #undef SF_TYPE
  #undef MF_TYPE

  default:
   parseError("Unsupported type in defaultValue!");
 }

 return ret;
}
