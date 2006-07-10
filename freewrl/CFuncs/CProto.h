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
 /* Default value, if exposedField or field */
 union
 {
  vrmlVec3fT sfVec3f;
 } defaultVal;
};

/* Constructor and destructor */
struct ProtoFieldDecl* newProtoFieldDecl(indexT, indexT, indexT);
void deleteProtoFieldDecl(struct ProtoFieldDecl*);

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

/* Instantiates the PROTO */
struct X3D_Group* protoDefinition_instantiate(struct ProtoDefinition*);

/* Does a recursively deep copy of a node-tree */
struct X3D_Node* protoDefinition_deepCopy(struct X3D_Node*);

#endif /* Once-check */
