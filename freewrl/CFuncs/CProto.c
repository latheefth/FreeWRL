/*******************************************************************
  Copyright (C) 2007 Daniel Kraft,  John Stewart, CRC Canada.
  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
  See the GNU Library General Public License (file COPYING in the distribution)
  for conditions of use and redistribution.
 *********************************************************************/


/* CProto.c - Sourcecode for CProto.h */
/* TODO:  Pointer updating could be made more efficient! */

#include <stdlib.h>
#include <assert.h>

#include "CProto.h"
#include "CParseGeneral.h"
#include "CParseParser.h"
#include "CParseLexer.h"

/* internal sequence number for protos */
static  indexT latest_protoDefNumber =1;


/* ************************************************************************** */
/* ******************************** OffsetPointer *************************** */
/* ************************************************************************** */

/* Constructor/destructor */
/* ********************** */

struct OffsetPointer* newOffsetPointer(struct X3D_Node* node, unsigned ofs)
{
 struct OffsetPointer* ret=MALLOC(sizeof(struct OffsetPointer));
 /* printf("creating offsetpointer %p\n", ret); */
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
 struct ProtoFieldDecl* ret=MALLOC(sizeof(struct ProtoFieldDecl));
  /* printf("creating ProtoFieldDecl %p\n", ret);  */
 ret->mode=mode;
 ret->type=type;
 ret->name=name;
 ret->alreadySet=FALSE;
 ret->fieldString = NULL;
 ret->dests=newVector(struct OffsetPointer*, 4);
 assert(ret->dests);
 ret->scriptDests=newVector(struct ScriptFieldInstanceInfo*, 4);
 assert(ret->scriptDests);
 return ret;
}

void deleteProtoFieldDecl(struct ProtoFieldDecl* me)
{
 size_t i;
 for(i=0; i!=vector_size(me->dests); ++i)
  deleteOffsetPointer(vector_get(struct OffsetPointer*, me->dests, i));
 deleteVector(struct OffsetPointer*, me->dests);
 FREE_IF_NZ(me);
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

/* Routing to/from */
void protoFieldDecl_routeTo(struct ProtoFieldDecl* me,
 struct X3D_Node* node, unsigned ofs, int dir, struct VRMLParser* p)
{
 int i;
 size_t len=returnRoutingElementLength(me->type);
 assert(me->mode==PKW_inputOutput || me->mode==PKW_inputOnly);

 /* For each destination mapped to this proto field, add a route */
 for(i=0; i!=vector_size(me->dests); ++i)
 {
  struct OffsetPointer* optr=vector_get(struct OffsetPointer*, me->dests, i);
  /* printf("protoFieldDecl_routeTo: registering route from %p %u to dest %p %u dir is %d\n", node, ofs, optr->node, optr->ofs, dir); */
  parser_registerRoute(p, node, ofs, optr->node, optr->ofs, len, dir);
 }

 /* For each script field mapped to this proto field, add a route */
 for (i=0; i!=vector_size(me->scriptDests); ++i) {
	struct ScriptFieldInstanceInfo* sfield = vector_get(struct ScriptFieldInstanceInfo*, me->scriptDests, i);
	struct Script* toscript = sfield->script;
	struct ScriptFieldDecl* tosfield = sfield->decl;
	if (dir == FROM_SCRIPT) {
		dir = SCRIPT_TO_SCRIPT;
	} else if (dir == 0) {
		dir = TO_SCRIPT;
	}
  	/* printf("protoFieldDecl_routeTo: registering route from %p %u to script dest %p %u %d\n", node, ofs, toscript->num, scriptFieldDecl_getRoutingOffset(tosfield), dir); */
	parser_registerRoute(p, node, ofs, toscript->num, scriptFieldDecl_getRoutingOffset(tosfield), len, dir);
 }
}

void protoFieldDecl_routeFrom(struct ProtoFieldDecl* me,
 struct X3D_Node* node, unsigned ofs, int dir, struct VRMLParser* p)
{
 int i;
 size_t len=returnRoutingElementLength(me->type);

 assert(me->mode==PKW_inputOutput || me->mode==PKW_outputOnly);

 /* For each destination mapped to this proto field, add a route */
 for(i=0; i!=vector_size(me->dests); ++i)
 {
  struct OffsetPointer* optr=vector_get(struct OffsetPointer*, me->dests, i);
  parser_registerRoute(p, optr->node, optr->ofs, node, ofs, len, dir);
  /* printf("protoFieldDecl_routeFrom: registering route from dest %p %u to %p %u dir is %d\n", optr->node, optr->ofs, node, ofs, dir); */
 }

 /* For each script field mapped to this proto field, add a route */
 for (i=0; i!=vector_size(me->scriptDests); ++i) {
	struct ScriptFieldInstanceInfo* sfield = vector_get(struct ScriptFieldInstanceInfo*, me->scriptDests, i);
	struct Script* fromscript = sfield->script;
	struct ScriptFieldDecl* fromsfield = sfield->decl;
	if (dir == TO_SCRIPT) {
		dir = SCRIPT_TO_SCRIPT;
	} else if (dir == 0) {
		dir = FROM_SCRIPT;
	}
	parser_registerRoute(p, fromscript->num, scriptFieldDecl_getRoutingOffset(fromsfield), node, ofs, len, dir);
  	/* printf("protoFieldDecl_routeFrom: registering route from script dest %p %u to %p %u %d\n", fromscript->num, scriptFieldDecl_getRoutingOffset(fromsfield), node, ofs,  dir); */
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
 struct ProtoRoute* ret=MALLOC(sizeof(struct ProtoRoute));
 assert(ret);

 /* printf("creating new proto route from %p %u to %p %u dir %d\n", from, fromOfs, to, toOfs, dir); */

 ret->from=from;
 ret->to=to;
 ret->fromOfs=fromOfs;
 ret->toOfs=toOfs;
 ret->len=len;
 ret->dir=dir;

 return ret;
}

/* ************************************************************************** */
/* ******************************** ProtoDefinition ************************* */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

struct ProtoDefinition* newProtoDefinition()
{
 /* Attention!  protoDefinition_copy also sets up data from raw MALLOC!  Don't
  * forget to mimic any changes to this method there, too!
  */

 struct ProtoDefinition* ret=MALLOC(sizeof(struct ProtoDefinition));
 assert(ret);
 ret->tree=createNewX3DNode(NODE_Group);
 assert(ret->tree);

 /* printf("creating new ProtoDefinition %u\n", ret);  */

 ret->iface=newVector(struct ProtoFieldDecl*, 4);
 assert(ret->iface);

 ret->routes=newVector(struct ProtoRoute*, 4);
 assert(ret->routes);

 ret->nestedProtoFields = newVector(struct NestedProtoField*, 4);
 assert(ret->nestedProtoFields);
 
 ret->innerPtrs=NULL;

 ret->protoBody = NULL; /* string copy of the proto body */

 ret->protoDefNumber = latest_protoDefNumber++;

 return ret;
}

void deleteProtoDefinition(struct ProtoDefinition* me)
{
 /* FIXME:  Not deep-destroying nodes!!! */
 /* If tree is NULL, it is already extracted! */
  FREE_IF_NZ (me->tree);
  FREE_IF_NZ (me->protoBody);

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
 
 FREE_IF_NZ (me);
}

/* Other members */
/* ************* */

/* Retrieve a field by its "name" */
struct ProtoFieldDecl* protoDefinition_getField(struct ProtoDefinition* me,
 indexT ind, indexT mode)
{
 /* TODO:  O(log(n)) by sorting */
 size_t i;
/* printf ("protoDefinition_getField; sizeof iface %d\n",vector_size(me->iface)); */
 for(i=0; i!=vector_size(me->iface); ++i)
 {
  struct ProtoFieldDecl* f=vector_get(struct ProtoFieldDecl*, me->iface, i);
  if(f->name==ind && f->mode==mode) {
   /* printf ("protoDefinition_getField, comparing %d %d and %d %d\n", f->name, ind, f->mode, mode); */
   return f;
  }
 }

 return NULL;
}

/* Add a node */
void protoDefinition_addNode(struct ProtoDefinition* me, struct X3D_Node* node)
{
 assert(me);
 assert(me->tree);
 ADD_PARENT(node, X3D_NODE(me->tree));
 addToNode(me->tree, offsetof(struct X3D_Group, children), node);
}

/* Copies the PROTO */
struct ProtoDefinition* protoDefinition_copy(struct VRMLLexer* lex, struct ProtoDefinition* me)
{
 struct ProtoDefinition* ret=MALLOC(sizeof(struct ProtoDefinition));
 size_t i;
 assert(ret);

	/* printf ("coping protoBody...\n");  */
	ret->protoBody = STRDUP (me->protoBody);
	/* printf ("copied proto body as %s\n",ret->protoBody); */

	/* printf("protoDefinition_copy: copying %u to %u\n", me, ret);  */
 /* Copy interface */
 ret->iface=newVector(struct ProtoFieldDecl*, vector_size(me->iface));
 assert(ret->iface);
 for(i=0; i!=vector_size(me->iface); ++i)
  vector_pushBack(struct ProtoFieldDecl*, ret->iface,
   protoFieldDecl_copy(lex, vector_get(struct ProtoFieldDecl*, me->iface, i)));

 /* Copy routes */
 ret->routes=newVector(struct ProtoRoute*, vector_size(me->routes));
 assert(ret->routes);
 for(i=0; i!=vector_size(me->routes); ++i) {
  struct ProtoRoute* theRoute = vector_get(struct ProtoRoute*, me->routes, i);
  /* printf("copying proto route from %p %u to %p %u dir %d\n", theRoute->from, theRoute->fromOfs, theRoute->to, theRoute->toOfs, theRoute->dir); */
  vector_pushBack(struct ProtoRoute*, ret->routes,
   protoRoute_copy(vector_get(struct ProtoRoute*, me->routes, i)));
 }

 /* Fill inner pointers */
 ret->innerPtrs=NULL;
 protoDefinition_fillInnerPtrs(ret);

 ret->nestedProtoFields = newVector(struct NestedProtoField*, 4);
 assert(ret->nestedProtoFields);

 /* Copy the scene graph and fill the fields thereby */
 ret->tree=protoDefinition_deepCopy(lex, me->tree, ret, NULL);
 /* Set reference */
 /* XXX:  Do we need the *original* reference? */
 ret->tree->__protoDef=ret;

 /* JAS - call a function to ensure that the parents are filled in properly -
    sometimes the reverse links are required, especially when propagating sensitive info 
    back up the tree */
 /* printf ("going to call checkParentLink\n"); */

 checkParentLink(ret->tree,NULL);

 ret->protoDefNumber = latest_protoDefNumber++;

 /* printf ("finished calling checkParentLink, tree is %u\n",ret->tree);  */

 return ret;
}

/* Extracts the scene graph */
/* Checks that every field and exposed field has had it's value propagated to all of it's destinations.
   (i.e. propogates the value of fields using the default value.  Fields where values were specified will
   already have had that value parsed and propagagted to all dests.)
   Adds all of the routes in the route vector for this ProtoDefinition to the CRoutes table.
   Gets the scene graph for this protoDefinition and returns it. */
struct X3D_Group* protoDefinition_extractScene(struct VRMLLexer* lex, struct ProtoDefinition* me)
{
 size_t i;
 size_t j;
 struct X3D_Group* ret=me->tree;
 struct NestedProtoField* nestedField;
 struct OffsetPointer* toCopy;
 struct OffsetPointer* copy;
 assert(ret);

	#ifdef CPROTOVERBOSE
 	printf("protoDefinition_extractScene: Extracting scene for protodef %u protoBody :%s:\n", me,me->protoBody);
	#endif

 me->tree=NULL;

 /* First check if there are any nested proto fields.  If there are, we need to go through the dests list for the original field and
    copy the offset pointers into the dests list for the local field. We also need to copy over the scriptDests list for the original field
    into the scriptDests list for the local field */
 for (i=0; i<vector_size(me->nestedProtoFields); i++) {
	nestedField = vector_get(struct NestedProtoField*, me->nestedProtoFields, i);
   	struct ProtoFieldDecl* origField;
	struct ProtoFieldDecl* localField;
	origField = nestedField->origField;
	localField = nestedField->localField;
	for (j=0; j<vector_size(origField->dests); j++) {
		toCopy = vector_get(struct OffsetPointer*, origField->dests, j);
		copy = newOffsetPointer(toCopy->node, toCopy->ofs);
		vector_pushBack(struct OffsetPointer*, localField->dests, copy);
	}
	for (j=0; j < vector_size(origField->scriptDests); j++) {
		struct ScriptFieldInstanceInfo* sCopy = vector_get(struct ScriptFieldInstanceInfo*, origField->scriptDests, j);
		struct ScriptFieldInstanceInfo* snew = newScriptFieldInstanceInfo(sCopy->decl, sCopy->script);
		vector_pushBack(struct ScriptFieldInstanceInfo*, localField->scriptDests, snew);
	}
  }
	
 /* Finish all fields now */
 /* If we haven't already done so, call protoFieldDecl_setValue for this field.
    This will only happen if no value for the field was parsed (i.e. a default value must be used).
    This will go through the dests vector for this field, and set the value of each dest to the default value.
     (Note that the first element of the dests vector is actually assigned the value of "val".  While
     subsequent elements of the dests vector have a DEEPCOPY of the "val" assigned (if appropriate). */
 for(i=0; i!=vector_size(me->iface); ++i) {
   struct ProtoFieldDecl* pField;
   pField = vector_get(struct ProtoFieldDecl*, me->iface, i);
   protoFieldDecl_finish(lex, pField);
  }

 /* Register all routes */
 /* This is #defined as  CRoutes_RegisterSimple((me)->from, (me)->fromOfs, (me)->to, (me)->toOfs, (me)->len, (me)->dir) */
 /* Goes through the list of routes for this proto (the routes vector) and adds each one
    to the CRoutes table */
 for(i=0; i!=vector_size(me->routes); ++i) {
  struct ProtoRoute* theRoute;
  theRoute = vector_get(struct ProtoRoute*, me->routes, i);
  /* printf("extract_scene: register route from %p %u to %p %u (dir is %d)\n", theRoute->from, theRoute->fromOfs, theRoute->to, theRoute->toOfs, theRoute->dir); */
  protoRoute_register(vector_get(struct ProtoRoute*, me->routes, i));
 }

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
#define DEEPCOPY_sfbool(l, v, i, h) v
#define DEEPCOPY_sfcolor(l,v, i, h) v
#define DEEPCOPY_sfcolorrgba(l,v, i, h) v
#define DEEPCOPY_sffloat(l,v, i, h) v
#define DEEPCOPY_sfint32(l,v, i, h) v
#define DEEPCOPY_sfnode(l,v, i, h) protoDefinition_deepCopy(l,v, i, h)
#define DEEPCOPY_sfrotation(l,v, i, h) v
#define DEEPCOPY_sfstring(l,v, i, h) deepcopy_sfstring(l, v)
#define DEEPCOPY_sftime(l,v, i, h) v
#define DEEPCOPY_sfvec2f(l,v, i, h) v
#define DEEPCOPY_sfvec3f(l,v, i, h) v

#ifdef OLDCODE
/* JAS sfimages have changed structure types */
#define DEEPCOPY_sfimage(l, v, i, h) deepcopy_sfimage(l, v, i, h)

static struct Multi_Int32 DEEPCOPY_mfint32(struct VRMLLexer*, struct Multi_Int32,
 struct ProtoDefinition*, struct PointerHash*);
static vrmlImageT deepcopy_sfimage(struct VRMLLexer* lex, vrmlImageT img,
 struct ProtoDefinition* new, struct PointerHash* hash)
{
 vrmlImageT ret=MALLOC(sizeof(*img));
 *ret=DEEPCOPY_mfint32(lex, *img, new, hash);
 return ret;
}
#else

#define DEEPCOPY_sfimage(l, v, i, h) v

#endif
static vrmlStringT deepcopy_sfstring(struct VRMLLexer* lex, vrmlStringT str)
{
 return newASCIIString (str->strptr);
}

/* Deepcopies a mf* */
#define DEEPCOPY_MFVALUE(lex, type, stype) \
 static struct Multi_##stype DEEPCOPY_mf##type(struct VRMLLexer* lex, struct Multi_##stype src, \
  struct ProtoDefinition* new, struct PointerHash* hash) \
 { \
  int i; \
  struct Multi_##stype dest; \
  dest.n=src.n; \
  dest.p=MALLOC(sizeof(src.p[0])*src.n); \
  for(i=0; i!=src.n; ++i) \
   dest.p[i]=DEEPCOPY_sf##type(lex, src.p[i], new, hash); \
  if(new) \
   protoDefinition_doPtrUpdate(new, \
    (uint8_t*)src.p, (uint8_t*)(src.p+src.n), (uint8_t*)dest.p); \
  return dest; \
 }
DEEPCOPY_MFVALUE(lex, bool, Bool)
DEEPCOPY_MFVALUE(lex, color, Color)
DEEPCOPY_MFVALUE(lex, colorrgba, ColorRGBA)
DEEPCOPY_MFVALUE(lex, float, Float)
DEEPCOPY_MFVALUE(lex, int32, Int32)
DEEPCOPY_MFVALUE(lex, node, Node)
DEEPCOPY_MFVALUE(lex, rotation, Rotation)
DEEPCOPY_MFVALUE(lex, string, String)
DEEPCOPY_MFVALUE(lex, time, Time)
DEEPCOPY_MFVALUE(lex, vec2f, Vec2f)
DEEPCOPY_MFVALUE(lex, vec3f, Vec3f)

/* ************************************************************************** */

/* Nodes; may be used to update the interface-pointers, too. */
struct X3D_Node* protoDefinition_deepCopy(struct VRMLLexer* lex, struct X3D_Node* node,
 struct ProtoDefinition* new, struct PointerHash* hash)
{
 struct X3D_Node* ret;
 BOOL myHash=(!hash);

  /* printf("doing a deepcopy of proto with root node %p\n", node); */

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
 /* printf ("CProto deepcopy, created a node %u of type %s\n",ret, stringNodeType(ret->_nodeType)); */

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
   ret2->var=DEEPCOPY_##type(lex, node2->var, new, hash);

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
 if (node->_nodeType == NODE_Script) {
  /* If this is a script node, create a new context for the script */
	struct Script* old_script;
	struct Script* new_script;
	struct X3D_Script* ret2 = (struct X3D_Script*) ret;
	struct X3D_Script* node2 = (struct X3D_Script*) node;
	int i, j, k;

 	old_script = node2->__scriptObj; 
	ret2->__scriptObj = newScript();

	new_script = ret2->__scriptObj;

	/* Init the code for the new script */
	script_initCodeFromMFUri(X3D_SCRIPT(ret2)->__scriptObj, &X3D_SCRIPT(ret2)->url);

	/* Add in the fields defined for the old script into the new script */
	for (i = 0; i !=  vector_size(old_script->fields); ++i) {
		struct ScriptFieldDecl* sfield = vector_get(struct ScriptFieldDecl*, old_script->fields, i);
		struct ScriptFieldDecl* newfield = scriptFieldDecl_copy(lex, sfield);
		if (sfield->fieldDecl->mode == PKW_initializeOnly) {
			scriptFieldDecl_setFieldValue(newfield, sfield->value);
		}
		script_addField(new_script, newfield);

		/* Update the pointers in the proto expansion's field interface list for each field interface that has script destinations 
		   so that the script destinations point to the new script */
        	for (k=0; k != vector_size(new->iface); ++k) {
        	        struct ProtoFieldDecl* newdecl = vector_get(struct ProtoFieldDecl*, new->iface, k);
        	        for (j=0; j!= vector_size(newdecl->scriptDests); j++) {
        	                struct ScriptFieldInstanceInfo* sfieldinfo = vector_get(struct ScriptFieldInstanceInfo*, newdecl->scriptDests, j);
        	                if (sfieldinfo->script == old_script) {
        	                        sfieldinfo->script = new_script;
        	                }
				if (sfieldinfo->decl == sfield) {
					sfieldinfo->decl = newfield;
				}
        	        }
        	}
	}
	for (i = 0; i != vector_size(new->routes); i++) {
		struct ProtoRoute* curRoute = vector_get(struct ProtoRoute*, new->routes, i);
		if (curRoute->dir == TO_SCRIPT) {
			if (curRoute->to == old_script->num) {
				curRoute->to = new_script->num;
			}
		} else if (curRoute->dir == FROM_SCRIPT) {
			if (curRoute->from == old_script->num) {
				curRoute->from = new_script->num;
		}
	}
    }
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
void protoFieldDecl_setValue(struct VRMLLexer* lex, struct ProtoFieldDecl* me, union anyVrml* val)
{
 size_t i;
 struct OffsetPointer* myptr;



 assert(!me->alreadySet);
 me->alreadySet=TRUE;

 /* If there are no targets, destroy the value */
 if(vector_empty(me->dests) && vector_empty(me->scriptDests))
 {
  /* FIXME:  Free (destroy) value!!! */
  return;
 }

 /* Otherwise, assign first target to val */
/* 
    printf("got myptr %p\n", myptr); \ 
    printf("setting node %p offset %u\n", myptr->node, myptr->ofs); \
*/
 if (!vector_empty(me->dests)) {
 switch(me->type)
 {
  #define SF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: \
    myptr = vector_get(struct OffsetPointer*, me->dests, 0); \
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

 /* copy it to the others */
 for(i=0; i!=vector_size(me->dests); ++i)
 {
  switch(me->type)
  {
   #define SF_TYPE(fttype, type, ttype) \
    case FIELDTYPE_##fttype: \
    myptr = vector_get(struct OffsetPointer*, me->dests, i); \
     *offsetPointer_deref(vrml##ttype##T*, \
      vector_get(struct OffsetPointer*, me->dests, i))= \
      DEEPCOPY_##type(lex, val->type, NULL, NULL); \
     break;
   #define MF_TYPE(fttype, type, ttype) \
    case FIELDTYPE_##fttype: \
     *offsetPointer_deref(struct Multi_##ttype*, \
      vector_get(struct OffsetPointer*, me->dests, i))= \
      DEEPCOPY_##type(lex, val->type, NULL, NULL); \
     break;
   #include "VrmlTypeList.h"
   #undef SF_TYPE
   #undef MF_TYPE

   default:
    parseError("Error: Unsupported type for PROTO field!!!");
  }
  }
 }
if (!vector_empty(me->scriptDests)) {

 /* and copy it to the others */
 for(i=0; i!=vector_size(me->scriptDests); ++i)
 {
	struct ScriptFieldInstanceInfo* sfield = vector_get(struct ScriptFieldInstanceInfo*, me->scriptDests, i);
	scriptFieldDecl_setFieldValue(sfield->decl, *val);
	script_addField(sfield->script, sfield->decl);
 }
}
}

/* Copies a fieldDeclaration */
struct ProtoFieldDecl* protoFieldDecl_copy(struct VRMLLexer* lex, struct ProtoFieldDecl* me)
{
 struct ProtoFieldDecl* ret=newProtoFieldDecl(me->mode, me->type, me->name);
 size_t i;
 ret->alreadySet=FALSE;

 /* copy over the fieldString */
 /* printf ("copying field string for field... %s\n",me->fieldString); */
 if (me->fieldString != NULL) ret->fieldString = STRDUP(me->fieldString);

 ret->mode=me->mode;
 ret->type=me->type;
 ret->name=me->name;
 /* printf ("copied mode %s type %s and name %d\n",stringPROTOKeywordType(ret->mode)
	, stringFieldtypeType(ret->type), ret->name);

 printf ("protoFieldDecl_copy, copied fieldString for proto field\n"); */

 /* Copy destination pointers */
 for(i=0; i!=vector_size(me->dests); ++i) {
  vector_pushBack(struct OffsetPointer*, ret->dests,
   offsetPointer_copy(vector_get(struct OffsetPointer*, me->dests, i)));
  }

  /* Copy scriptfield dests */
  for (i=0; i!=vector_size(me->scriptDests); ++i) {
	vector_pushBack(struct ScriptFieldInstanceInfo*, ret->scriptDests, scriptFieldInstanceInfo_copy(vector_get(struct ScriptFieldInstanceInfo*, me->scriptDests, i)));
   	struct ScriptFieldInstanceInfo* temp;
   	struct ScriptFieldInstanceInfo* temp2;
   	temp = vector_get(struct ScriptFieldInstanceInfo*, me->scriptDests, i);
   	temp2 = vector_get(struct ScriptFieldInstanceInfo*, ret->scriptDests, i);
  }

 /* Copy default value */
 switch(me->type)
 {
  #define SF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: \
    ret->defaultVal.type=DEEPCOPY_##type(lex, me->defaultVal.type, NULL, NULL); \
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
 struct PointerHash* ret=MALLOC(sizeof(struct PointerHash));
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
 FREE_IF_NZ (me);
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


struct NestedProtoField* newNestedProtoField(struct ProtoFieldDecl* origField, struct ProtoFieldDecl* localField)
{
 struct NestedProtoField* ret = MALLOC(sizeof(struct NestedProtoField));
 assert(ret);

 /* printf("creating nested field %p with values %p %p %p\n", ret, origField, localField, origProto); */

 ret->origField=origField;
 ret->localField = localField;

 return ret;
}


/* go through a proto invocation, and get the invocation fields (if any) */
void getProtoInvocationFields(struct VRMLParser *me, struct ProtoDefinition *thisProto) {
	int c;
	char *cp;
	char *initCP;
	char tmp;

	struct ProtoFieldDecl* pdecl;
	union anyVrml thisVal;

	#ifdef CPROTOVERBOSE
	printf ("start of getProtoInvocationFields, nextIn :%s:\n",me->lexer->nextIn);
	#endif

	while (!lexer_closeCurly(me->lexer)) {

		lexer_setCurID(me->lexer);
		#ifdef CPROTOVERBOSE
		printf ("getProtoInvocationFields, id is %s\n",me->lexer->curID);
		#endif

		if (me->lexer->curID != NULL) {
			pdecl = getProtoFieldDeclaration(me->lexer, thisProto,me->lexer->curID);
			if (pdecl != NULL) {
				#ifdef CPROTOVERBOSE
				printf ("in getProtoInvocationFields, we have pdecl original as %s\n",pdecl->fieldString);
				#endif

	
				/* get a link to the beginning of this field */
				FREE_IF_NZ(me->lexer->curID);
				cp = me->lexer->nextIn;
				initCP = me->lexer->startOfStringPtr;

				#ifdef CPROTOVERBOSE
				printf ("going to parse type of %d mode (%s), curID %s, starting at :%s:\n",pdecl->type, stringPROTOKeywordType(pdecl->mode),
						me->lexer->curID, me->lexer->nextIn);
				#endif

				if (pdecl->mode != PKW_outputOnly) {

/* 					lexer_skip(me); */
					lexer_setCurID(me->lexer);
					#ifdef CPROTOVERBOSE
					printf ("checking if this is an IS :%s:\n",me->lexer->curID);
					#endif

					if(me->curPROTO && lexer_keyword(me->lexer, KW_IS)) {
						struct ProtoFieldDecl* pexp;

						#ifdef CPROTOVERBOSE
						printf ("FOUND IS on protoInvocationFields; have to replace \n");
						#endif


						FREE_IF_NZ(me->lexer->curID);
						lexer_setCurID(me->lexer);
						#ifdef CPROTOVERBOSE
						printf ("going to try and find :%s: in curPROTO\n",me->lexer->curID);
						#endif

						pexp = getProtoFieldDeclaration(me->lexer,me->curPROTO,me->lexer->curID);
						if (pexp != NULL) {
							#ifdef CPROTOVERBOSE
							printf ("attaching :%s: to :%s:\n",pexp->fieldString, me->lexer->nextIn);
							#endif

							concatAndGiveToLexer(me->lexer, pexp->fieldString, me->lexer->nextIn);
						}
						FREE_IF_NZ(me->lexer->curID);
					}

					if (!parseType(me, pdecl->type, &thisVal)) {
						#ifdef CPROTOVERBOSE
						printf ("parsing error on field value in proto expansion\n");
						#endif

					} else {
						#ifdef CPROTOVERBOSE
						printf ("parsed field; nextin was %u, now %u\n",cp, me->lexer->nextIn);
						#endif

					}
	
					/* was this possibly a proto expansion? */
					if (initCP != me->lexer->startOfStringPtr) cp = me->lexer->startOfStringPtr;

					/* copy over the new value */
					initCP = (char *) (me->lexer->nextIn);
					tmp = *initCP; *initCP = '\0';
					FREE_IF_NZ(pdecl->fieldString); pdecl->fieldString = strdup (cp);
					*initCP = tmp;
					/* printf ("getProtoInvocationFields, just copied :%s:\n",pdecl->fieldString); */
				} else {
					#ifdef CPROTOVERBOSE
					printf ("skipped parsing this type of %s\n",stringPROTOKeywordType(pdecl->type));
					#endif

				}
			}
		}
		#ifdef CPROTOVERBOSE
		printf ("after pt, curID %s\n",me->lexer->curID);
		#endif

		lexer_skip (me);
	}

}


/* find the proto field declare for a particular proto */
struct ProtoFieldDecl* getProtoFieldDeclaration(struct VRMLLexer *me, struct ProtoDefinition *thisProto, char *thisID) {
	indexT retUO;
	const char** userArr;
	size_t userCnt;
	struct ProtoFieldDecl* ret = NULL;

	#ifdef CPROTOVERBOSE
	printf ("getProtoFieldDeclaration, for field :%s:\n",thisID);
	#endif

	userArr=&vector_get(const char*, me->user_initializeOnly, 0);
	userCnt=vector_size(me->user_initializeOnly);
	retUO=findFieldInARR(thisID, userArr, userCnt);
	if (retUO != ID_UNDEFINED) 
		ret=protoDefinition_getField(thisProto,retUO,PKW_initializeOnly);
	else {
		userArr=&vector_get(const char*, me->user_inputOutput, 0);
		userCnt=vector_size(me->user_inputOutput);
		retUO=findFieldInARR(thisID, userArr, userCnt);
		if (retUO != ID_UNDEFINED) 
			ret=protoDefinition_getField(thisProto,retUO,PKW_inputOutput);
		else {
			userArr=&vector_get(const char*, me->user_inputOnly, 0);
			userCnt=vector_size(me->user_inputOnly);
			retUO=findFieldInARR(thisID, userArr, userCnt);
			if (retUO != ID_UNDEFINED) 
				ret=protoDefinition_getField(thisProto,retUO,PKW_inputOnly);
			else {
				userArr=&vector_get(const char*, me->user_outputOnly, 0);
				userCnt=vector_size(me->user_outputOnly);
				retUO=findFieldInARR(thisID, userArr, userCnt);
				if (retUO != ID_UNDEFINED) ret=protoDefinition_getField(thisProto,retUO,PKW_outputOnly);
			}
		}
	}	

	#ifdef CPROTOVERBOSE
	printf ("getProtoFieldDeclaration, ret is %u\n",ret);
	#endif

	return ret;
}
