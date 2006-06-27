/* CProto.h - this is the object representing a PROTO definition and being
 * capable of instantiating it.
 */

#ifndef CPROTO_H
#define CPROTO_H

#include "headers.h"

/* The object */
struct ProtoDefinition
{
 struct X3D_Group* tree; /* The scene graph of the PROTO definition */
};

/* Constructor and destructor */
struct ProtoDefinition* newProtoDefinition();
void deleteProtoDefinition(struct ProtoDefinition*);

/* Add a node to the virtual group node */
void protoDefinition_addNode(struct ProtoDefinition*, struct X3D_Node*);

/* Instantiates the PROTO */
struct X3D_Group* protoDefinition_instantiate(struct ProtoDefinition*);

#endif /* Once-check */
