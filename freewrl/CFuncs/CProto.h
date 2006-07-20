/* CProto.h - this is the object representing a PROTO definition and being
 * capable of instantiating it.
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

/* Updates destination pointers */
void protoFieldDecl_doDestinationUpdate(
 struct ProtoFieldDecl*, struct ProtoFieldDecl*, uint8_t*, uint8_t*, uint8_t*);

/* Sets this field's value (copy to destinations) */
void protoFieldDecl_setValue(struct ProtoFieldDecl*, union anyVrml*);

/* ************************************************************************** */
/* ****************************** ProtoDefinition *************************** */
/* ************************************************************************** */

/* The object */
struct ProtoDefinition
{
 struct X3D_Group* tree; /* The scene graph of the PROTO definition */
 struct Vector* iface; /* The ProtoFieldDecls making up the interface */
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
 indexT);

/* Copies a ProtoDefinition, so that we can afterwards fill in field values */
struct ProtoDefinition* protoDefinition_copy(struct ProtoDefinition*);

/* Extracts the scene graph out of a ProtoDefinition */
struct X3D_Group* protoDefinition_extractScene(struct ProtoDefinition*);

/* Updates interface-pointers to a given memory block */
void protoDefinition_doInterfaceUpdate(struct Vector*, struct Vector*,
 uint8_t*, uint8_t*, uint8_t*);

/* Does a recursively deep copy of a node-tree */
struct X3D_Node* protoDefinition_deepCopy(struct X3D_Node*,
 struct Vector*, struct Vector*);

#endif /* Once-check */
