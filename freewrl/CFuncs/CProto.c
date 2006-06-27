/* CProto.c - Sourcecode for CProto.h */

#include <stdlib.h>
#include <assert.h>

#include "CProto.h"

/* Constructor and destructor */
/* ************************** */

struct ProtoDefinition* newProtoDefinition()
{
 struct ProtoDefinition* ret=malloc(sizeof(struct ProtoDefinition));
 assert(ret);
 ret->tree=createNewX3DNode(NODE_Group);
 assert(ret->tree);
 ret->tree->__isProto=TRUE;

 return ret;
}

void deleteProtoDefinition(struct ProtoDefinition* me)
{
 /* FIXME:  Not deep-destroying nodes!!! */
 free(me->tree);
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
 /* FIXME:  Deepcopy PROTO on instantiation! */
 return me->tree;
}
