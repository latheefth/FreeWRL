/* CProto.c - Sourcecode for CProto.h */

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
 return ret;
}

void deleteProtoFieldDecl(struct ProtoFieldDecl* me)
{
 free(me);
}

/* ************************************************************************** */
/* ******************************** ProtoDefinition ************************* */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

struct ProtoDefinition* newProtoDefinition()
{
 struct ProtoDefinition* ret=malloc(sizeof(struct ProtoDefinition));
 assert(ret);
 ret->tree=createNewX3DNode(NODE_Group);
 assert(ret->tree);
 ret->tree->__isProto=TRUE;

 ret->iface=newVector(struct ProtoFieldDecl*, 4);
 assert(ret->iface);

 return ret;
}

void deleteProtoDefinition(struct ProtoDefinition* me)
{
 /* FIXME:  Not deep-destroying nodes!!! */
 free(me->tree);

 {
  size_t i;
  for(i=0; i!=vector_size(me->iface); ++i)
   deleteProtoFieldDecl(vector_get(struct ProtoFieldDecl*, me->iface, i));
  deleteVector(struct ProtoDefinition*, me->iface);
 }
 
 free(me);
}

/* Other members */
/* ************* */

/* Add a node */
void protoDefinition_addNode(struct ProtoDefinition* me, struct X3D_Node* node)
{
 assert(me);
 assert(me->tree);
 add_parent(node, me->tree);
 addToNode(me->tree, offsetof(struct X3D_Group, children), node);
}

/* Instantiates the PROTO */
struct X3D_Group* protoDefinition_instantiate(struct ProtoDefinition* me)
{
 struct X3D_Group* ret=protoDefinition_deepCopy(me->tree);
 ret->__isProto=TRUE;
 return ret;
}

/* Deep copying */
/* ************ */

/* Deepcopies sf */
#define DEEPCOPY_sfbool(v) v
#define DEEPCOPY_sfcolor(v) v
#define DEEPCOPY_sfcolorrgba(v) v
#define DEEPCOPY_sffloat(v) v
#define DEEPCOPY_sfimage(v) deepcopy_sfimage(v)
#define DEEPCOPY_sfint32(v) v
#define DEEPCOPY_sfnode(v) protoDefinition_deepCopy(v)
#define DEEPCOPY_sfrotation(v) v
#define DEEPCOPY_sfstring(v) deepcopy_sfstring(v)
#define DEEPCOPY_sftime(v) v
#define DEEPCOPY_sfvec2f(v) v
#define DEEPCOPY_sfvec3f(v) v
static struct Multi_Int32 DEEPCOPY_mfint32(struct Multi_Int32);
static vrmlImageT deepcopy_sfimage(vrmlImageT img)
{
 vrmlImageT ret=malloc(sizeof(*img));
 *ret=DEEPCOPY_mfint32(*img);
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
 static struct Multi_##stype DEEPCOPY_mf##type(struct Multi_##stype src) \
 { \
  int i; \
  struct Multi_##stype dest; \
  dest.n=src.n; \
  dest.p=malloc(sizeof(src.p[0])*src.n); \
  for(i=0; i!=src.n; ++i) \
   dest.p[i]=DEEPCOPY_sf##type(src.p[i]); \
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

/* Nodes */
struct X3D_Node* protoDefinition_deepCopy(struct X3D_Node* node)
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
    struct X3D_##n* ret2=(struct X3D_##n*)ret;
  #define END_NODE(n) \
    break; \
   }

  /* Copying of fields depending on type */

  #define FIELD(n, field, type, var) \
   ret2->var=DEEPCOPY_##type(node2->var);

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
