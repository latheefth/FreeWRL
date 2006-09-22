/* CProto.c - Sourcecode for CProto.h */
/* TODO:  Pointer updating could be made more efficient! */

#include <stdlib.h>
#include <assert.h>

#include "CProto.h"
#include "CParseGeneral.h"
#include "CParseLexer.h"

/* ************************************************************************** */
/* ******************************** OffsetPointer *************************** */
/* ************************************************************************** */

/* Constructor/destructor */
/* ********************** */

struct OffsetPointer* newOffsetPointer(struct X3D_Node* node, unsigned ofs)
{
 struct OffsetPointer* ret=malloc(sizeof(struct OffsetPointer));
 assert(ret);

 ret->node=node;
 ret->ofs=ofs;

 return ret;
}

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
 ret->dests=newVector(struct OffsetPointer*, 4);
 assert(ret->dests);
 return ret;
}

void deleteProtoFieldDecl(struct ProtoFieldDecl* me)
{
 size_t i;
 for(i=0; i!=vector_size(me->dests); ++i)
  deleteOffsetPointer(vector_get(struct OffsetPointer*, me->dests, i));
 deleteVector(struct OffsetPointer*, me->dests);
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
  vector_pushBack(void**, v,
   &vector_get(struct OffsetPointer*, me->dests, i)->node);
}

size_t protoFieldDecl_getLength(struct ProtoFieldDecl* me)
{
 #define SF_TYPE(ttype, type, stype) \
  case FIELDTYPE_##ttype: \
   return sizeof(vrml##stype##T);
 #define MF_TYPE(ttype, type, stype) \
  case FIELDTYPE_##ttype: \
   return 0;

 switch(me->type)
 {
  #include "VrmlTypeList.h"
#ifndef NDEBUG
  default:
   assert(FALSE);
#endif
 }

 #undef SF_TYPE
 #undef MF_TYPE

 assert(FALSE);
 return 0;
}

/* Routing to/from */

void protoFieldDecl_routeTo(struct ProtoFieldDecl* me,
 struct X3D_Node* node, unsigned ofs, int dir, struct VRMLParser* p)
{
 int i;
 size_t len=protoFieldDecl_getLength(me);
 assert(me->mode==PKW_exposedField || me->mode==PKW_eventIn);
 for(i=0; i!=vector_size(me->dests); ++i)
 {
  struct OffsetPointer* optr=vector_get(struct OffsetPointer*, me->dests, i);
  parser_registerRoute(p, node, ofs, optr->node, optr->ofs, len, dir);
 }
}

void protoFieldDecl_routeFrom(struct ProtoFieldDecl* me,
 struct X3D_Node* node, unsigned ofs, int dir, struct VRMLParser* p)
{
 int i;
 size_t len=protoFieldDecl_getLength(me);
 assert(me->mode==PKW_exposedField || me->mode==PKW_eventIn);
 for(i=0; i!=vector_size(me->dests); ++i)
 {
  struct OffsetPointer* optr=vector_get(struct OffsetPointer*, me->dests, i);
  parser_registerRoute(p, optr->node, optr->ofs, node, ofs, len, dir);
 }
}

/* setValue is at the end, because we need deep-copying there */
/* copy is at the end, too, because defaultVal needs to be deep-copied. */

/* ************************************************************************** */
/* ********************************** ProtoRoute **************************** */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

struct ProtoRoute* newProtoRoute(struct X3D_Node* from, int fromOfs,
 struct X3D_Node* to, int toOfs, size_t len, int dir)
{
 struct ProtoRoute* ret=malloc(sizeof(struct ProtoRoute));
 assert(ret);

 ret->from=from;
 ret->to=to;
 ret->fromOfs=fromOfs;
 ret->toOfs=toOfs;
 ret->len=len;

 return ret;
}

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

 ret->iface=newVector(struct ProtoFieldDecl*, 4);
 assert(ret->iface);

 ret->routes=newVector(struct ProtoRoute*, 4);
 assert(ret->routes);

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
  for(i=0; i!=vector_size(me->routes); ++i)
   deleteProtoRoute(vector_get(struct ProtoRoute*, me->routes, i));
  deleteVector(struct ProtoRoute*, me->routes);
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

 /* Copy routes */
 ret->routes=newVector(struct ProtoRoute*, vector_size(me->routes));
 assert(ret->routes);
 for(i=0; i!=vector_size(me->routes); ++i)
  vector_pushBack(struct ProtoRoute*, ret->routes,
   protoRoute_copy(vector_get(struct ProtoRoute*, me->routes, i)));

 /* Fill inner pointers */
 ret->innerPtrs=NULL;
 protoDefinition_fillInnerPtrs(ret);

 /* Copy the scene graph and fill the fields thereby */
 ret->tree=protoDefinition_deepCopy(me->tree, ret, NULL);
 /* Set reference */
 /* XXX:  Do we need the *original* reference? */
 ret->tree->__protoDef=ret;

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

 /* Register all routes */
 for(i=0; i!=vector_size(me->routes); ++i)
  protoRoute_register(vector_get(struct ProtoRoute*, me->routes, i));

 assert(ret->__protoDef);

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

 for(i=0; i!=vector_size(me->routes); ++i)
  protoRoute_addInnerPointersPointers(
   vector_get(struct ProtoRoute*, me->routes, i), me->innerPtrs);
}

/* Deep copying */
/* ************ */

/* Deepcopies sf */
#define DEEPCOPY_sfbool(v, i, h) v
#define DEEPCOPY_sfcolor(v, i, h) v
#define DEEPCOPY_sfcolorrgba(v, i, h) v
#define DEEPCOPY_sffloat(v, i, h) v
#define DEEPCOPY_sfimage(v, i, h) deepcopy_sfimage(v, i, h)
#define DEEPCOPY_sfint32(v, i, h) v
#define DEEPCOPY_sfnode(v, i, h) protoDefinition_deepCopy(v, i, h)
#define DEEPCOPY_sfrotation(v, i, h) v
#define DEEPCOPY_sfstring(v, i, h) deepcopy_sfstring(v)
#define DEEPCOPY_sftime(v, i, h) v
#define DEEPCOPY_sfvec2f(v, i, h) v
#define DEEPCOPY_sfvec3f(v, i, h) v
static struct Multi_Int32 DEEPCOPY_mfint32(struct Multi_Int32,
 struct ProtoDefinition*, struct PointerHash*);
static vrmlImageT deepcopy_sfimage(vrmlImageT img,
 struct ProtoDefinition* new, struct PointerHash* hash)
{
 vrmlImageT ret=malloc(sizeof(*img));
 *ret=DEEPCOPY_mfint32(*img, new, hash);
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
  struct ProtoDefinition* new, struct PointerHash* hash) \
 { \
  int i; \
  struct Multi_##stype dest; \
  dest.n=src.n; \
  dest.p=malloc(sizeof(src.p[0])*src.n); \
  for(i=0; i!=src.n; ++i) \
   dest.p[i]=DEEPCOPY_sf##type(src.p[i], new, hash); \
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
 struct ProtoDefinition* new, struct PointerHash* hash)
{
 struct X3D_Node* ret;
 BOOL myHash=(!hash);

 /* If we get nothing, what can we return? */
 if(!node) return NULL;

 /* Check if we've already copied this node */
 if(hash)
 {
  ret=pointerHash_get(hash, node);
  if(ret)
   return ret;
 }

 if(!hash)
  hash=newPointerHash();

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
   ret2->var=DEEPCOPY_##type(node2->var, new, hash);

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

 if(myHash)
  deletePointerHash(hash);

 /* Add pointer pair to hash */
 if(!myHash)
  pointerHash_add(hash, node, ret);

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
    *offsetPointer_deref(vrml##ttype##T*, \
     vector_get(struct OffsetPointer*, me->dests, 0))=val->type; \
    break;
  #define MF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: \
    *offsetPointer_deref(struct Multi_##ttype*, \
     vector_get(struct OffsetPointer*, me->dests, 0))=val->type; \
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
     *offsetPointer_deref(vrml##ttype##T*, \
      vector_get(struct OffsetPointer*, me->dests, i))= \
      DEEPCOPY_##type(val->type, NULL, NULL); \
     break;
   #define MF_TYPE(fttype, type, ttype) \
    case FIELDTYPE_##fttype: \
     *offsetPointer_deref(struct Multi_##ttype*, \
      vector_get(struct OffsetPointer*, me->dests, i))= \
      DEEPCOPY_##type(val->type, NULL, NULL); \
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
    ret->defaultVal.type=DEEPCOPY_##type(me->defaultVal.type, NULL, NULL); \
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

/* ************************************************************************** */
/* ******************************* PointerHash ****************************** */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

struct PointerHash* newPointerHash()
{
 struct PointerHash* ret=malloc(sizeof(struct PointerHash));
 size_t i;
 assert(ret);

 for(i=0; i!=POINTER_HASH_SIZE; ++i)
  ret->data[i]=NULL;

 return ret;
}

void deletePointerHash(struct PointerHash* me)
{
 size_t i;
 for(i=0; i!=POINTER_HASH_SIZE; ++i)
  if(me->data[i])
   deleteVector(struct PointerHashEntry, me->data[i]);
 free(me);
}

/* Query the hash */
struct X3D_Node* pointerHash_get(struct PointerHash* me, struct X3D_Node* o)
{
 size_t pos=((unsigned long)o)%POINTER_HASH_SIZE;
 size_t i;

 if(!me->data[pos])
  return NULL;

 for(i=0; i!=vector_size(me->data[pos]); ++i)
 {
  struct PointerHashEntry* entry=
   &vector_get(struct PointerHashEntry, me->data[pos], i);
  if(entry->original==o)
   return entry->copy;
 }

 return NULL;
}

/* Add to the hash */
void pointerHash_add(struct PointerHash* me,
 struct X3D_Node* o, struct X3D_Node* c)
{
 size_t pos=((unsigned long)o)%POINTER_HASH_SIZE;
 struct PointerHashEntry entry;

 assert(!pointerHash_get(me, o));

 if(!me->data[pos])
  me->data[pos]=newVector(struct PointerHashEntry, 4);
 
 entry.original=o;
 entry.copy=c;

 vector_pushBack(struct PointerHashEntry, me->data[pos], entry);
}
