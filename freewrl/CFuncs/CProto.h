/* CProto.h - this is the object representing a PROTO definition and being
 * capable of instantiating it.
 * 
 * We keep a vector of pointers to all that pointers which point to "inner
 * memory" and need therefore be updated when copying.  Such pointers include
 * field-destinations and parts of ROUTEs.  Those pointers are then simply
 * copied, their new positions put in the new vector, and afterwards are all
 * pointers there updated.
 */

#ifndef CPROTO_H
#define CPROTO_H

#include "headers.h"

#include "CParseGeneral.h"
#include "Vector.h"

/* ************************************************************************** */
/* ********************************* ProtoFieldDecl ************************* */
/* ************************************************************************** */

/* The object */
struct ProtoFieldDecl
{
 indexT mode; /* field, exposedField, eventIn, eventOut */
 indexT type; /* field type */
 indexT name; /* field "name" (its lexer-index) */
 BOOL alreadySet; /* Has the value already been set? */
 /* This is the list of desination pointers for this field */
 struct Vector* dests;
 /* Default value, if exposedField or field */
 union anyVrml defaultVal;
};

/* Constructor and destructor */
struct ProtoFieldDecl* newProtoFieldDecl(indexT, indexT, indexT);
void deleteProtoFieldDecl(struct ProtoFieldDecl*);

/* Copies */
struct ProtoFieldDecl* protoFieldDecl_copy(struct ProtoFieldDecl*);

/* Add a destination this field's value must be assigned to */
#define protoFieldDecl_addDestination(me, d) \
 vector_pushBack(void*, me->dests, d)

/* Sets this field's value (copy to destinations) */
void protoFieldDecl_setValue(struct ProtoFieldDecl*, union anyVrml*);

/* Finish this field - if value is not yet set, use default. */
#define protoFieldDecl_finish(me) \
 if(!me->alreadySet) \
  protoFieldDecl_setValue(me, &me->defaultVal)

/* Add inner pointers' pointers to the vector */
void protoFieldDecl_addInnerPointersPointers(struct ProtoFieldDecl*,
 struct Vector*);

/* ************************************************************************** */
/* ******************************* ProtoRoute ******************************* */
/* ************************************************************************** */

/* A ROUTE defined inside a PROTO block. */
struct ProtoRoute
{
 struct X3D_Node* from;
 struct X3D_Node* to;
 int fromOfs;
 int toOfs;
 int len;
};

/* Constructor and destructor */
struct ProtoRoute* newProtoRoute(struct X3D_Node*, int, struct X3D_Node*, int,
 int);
#define protoRoute_copy(me) \
 newProtoRoute((me)->from, (me)->fromOfs, (me)->to, (me)->toOfs, (me)->len)
#define deleteProtoRoute(me) \
 free(me)

/* Register this route */
#define protoRoute_register(me) \
 CRoutes_RegisterSimple((me)->from, (me)->fromOfs, (me)->to, (me)->toOfs, \
 (me)->len)

/* Add this one's inner pointers to the vector */
#define protoRoute_addInnerPointersPointers(me, vec) \
 { \
  vector_pushBack(void**, vec, &(me)->from); \
  vector_pushBack(void**, vec, &(me)->to); \
 }

/* ************************************************************************** */
/* ****************************** ProtoDefinition *************************** */
/* ************************************************************************** */

/* The object */
struct ProtoDefinition
{
 struct X3D_Group* tree; /* The scene graph of the PROTO definition */
 struct Vector* iface; /* The ProtoFieldDecls making up the interface */
 struct Vector* routes; /* Inner ROUTEs */
 struct Vector* innerPtrs; /* Pointers to pointers which need to be updated */
};

/* Constructor and destructor */
struct ProtoDefinition* newProtoDefinition();
void deleteProtoDefinition(struct ProtoDefinition*);

/* Add a node to the virtual group node */
void protoDefinition_addNode(struct ProtoDefinition*, struct X3D_Node*);

/* Adds a field declaration to the interface */
#define protoDefinition_addIfaceField(me, field) \
 vector_pushBack(struct ProtoFieldDecl*, (me)->iface, field)

/* Retrieves a field declaration of this PROTO */
struct ProtoFieldDecl* protoDefinition_getField(struct ProtoDefinition*, 
 indexT, indexT);

/* Copies a ProtoDefinition, so that we can afterwards fill in field values */
struct ProtoDefinition* protoDefinition_copy(struct ProtoDefinition*);

/* Extracts the scene graph out of a ProtoDefinition */
struct X3D_Group* protoDefinition_extractScene(struct ProtoDefinition*);

/* Fills the innerPtrs field */
void protoDefinition_fillInnerPtrs(struct ProtoDefinition*);

/* Updates interface-pointers to a given memory block */
void protoDefinition_doPtrUpdate(struct ProtoDefinition*,
 uint8_t*, uint8_t*, uint8_t*);

/* Does a recursively deep copy of a node-tree */
struct X3D_Node* protoDefinition_deepCopy(struct X3D_Node*,
 struct ProtoDefinition*);

/* Adds an inner route */
#define protoDefinition_addRoute(me, r) \
 vector_pushBack(struct ProtoRoute*, (me)->routes, r)

#endif /* Once-check */
