/*
=INSERT_TEMPLATE_HERE=

$Id$

CProto ???

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeX3D.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/CScripts.h"
#include "CParseParser.h"
#include "CParseLexer.h"
#include "CProto.h"


/* TODO:  Pointer updating could be made more efficient! */

#undef CPROTOVERBOSE

#define PROTO_CAT(newString,strlen2) \
	{ memcpy(&newProtoText[newProtoTextIndex],newString,strlen2); newProtoTextIndex += strlen2; \
		newProtoText[newProtoTextIndex] = '\0'; }

#define STARTPROTOGROUP "Group{FreeWRL__protoDef"
#define PROTOGROUPNUMBER "%s %d children[ #PROTOGROUP\n"
#define ENDPROTOGROUP "]}#END PROTOGROUP\n"
#define VERIFY_OUTPUT_LEN(extra) \
		/* printf ("verify, cur textInd %d, malloclen %d, extra %d\n",newProtoTextIndex, newProtoTextMallocLen, extra); */ \
		if ((newProtoTextIndex + extra + 1024) > newProtoTextMallocLen) { \
		while ((newProtoTextIndex + extra + 1024) > newProtoTextMallocLen) { \
			newProtoTextMallocLen *=2 ; \
		} \
			/* printf ("verify, made REALLOC to %d\n",newProtoTextMallocLen); */ \
			newProtoText = REALLOC(newProtoText, newProtoTextMallocLen); \
		}

#define APPEND_SPACE PROTO_CAT (" ",1);
#define APPEND_THISID PROTO_CAT (thisID,strlen(thisID));
#define APPEND_NODE PROTO_CAT (stringNodeType(ele->isNODE),strlen(stringNodeType(ele->isNODE)));
#define APPEND_KEYWORD PROTO_CAT (stringKeywordType(ele->isKEYWORD),strlen(stringKeywordType(ele->isKEYWORD)));
#define APPEND_STRINGTOKEN PROTO_CAT ( ele->stringToken,strlen(ele->stringToken));
#define APPEND_TERMINALSYMBOL \
			{ char chars[3]; \
			chars[0] = (char) ele->terminalSymbol;\
			chars[1] = '\0';\
			PROTO_CAT (chars,1);\
			}
#define APPEND_ENDPROTOGROUP PROTO_CAT (ENDPROTOGROUP,strlen(ENDPROTOGROUP));
#define SOMETHING_IN_ISVALUE (strlen(newTl) > 0) 
#define APPEND_ISVALUE PROTO_CAT (newTl,strlen(newTl));
#define APPEND_STARTPROTOGROUP \
	sprintf (thisID, PROTOGROUPNUMBER, STARTPROTOGROUP, *thisProto); \
	PROTO_CAT(thisID, strlen(thisID))

#define APPEND_IF_NOT_OUTPUTONLY \
{ int coffset, ctype, ckind, field; \
	printf ("checking to see if node %s has an outputOnly field %s\n",stringNodeType(lastNode->isNODE), ele->stringToken); \
	findFieldInOFFSETS(NODE_OFFSETS[lastNode->isNODE], findFieldInFIELDNAMES(ele->stringToken), &coffset, &ctype, &ckind); \
	printf ("found coffset %d ctype %d ckind %d\n",coffset, ctype, ckind); \
	if (ckind != KW_outputOnly) { printf ("APPENDING\n"); APPEND_STRINGTOKEN } else printf ("NOT APPENDING\n"); \
}

#define APPEND_EDITED_STRINGTOKEN \
	/* if this is an outputOnly field, dont bother printing it. It SHOULD be followed by an \
	   KW_IS, but the IS value from the PROTO definition will be blank */ \
	if (lastKeyword != NULL) { \
		/* is this a KW_IS, or another keyword that needs a PROTO expansion specific ID? */ \
		if (lastKeyword->isKEYWORD != KW_IS) { \
			sprintf (thisID," %s%d_",FABRICATED_DEF_HEADER,(*thisProto)->protoDefNumber); \
			APPEND_THISID \
			APPEND_STRINGTOKEN \
		} \
		lastKeyword = NULL; \
	} else {APPEND_STRINGTOKEN}

/* internal sequence number for protos */
static  indexT latest_protoDefNumber =1;
static  indexT nextFabricatedDef=1;

/* ************************************************************************** */
/* ************************** ProtoElementPointer *************************** */
/* ************************************************************************** */

/* Constructor/destructor */
struct ProtoElementPointer* newProtoElementPointer(void) {
	struct ProtoElementPointer *ret=MALLOC(sizeof(struct ProtoElementPointer));
	ASSERT (ret);

	ret->stringToken = NULL;
	ret->isNODE = ID_UNDEFINED;
	ret->isKEYWORD = ID_UNDEFINED;
	ret->terminalSymbol = ID_UNDEFINED;	
	ret->fabricatedDef = ID_UNDEFINED;
	return ret;
}

/* in CFuncs/CProto.h #define deleteProtoElementPointer(me)  */

struct ProtoElementPointer *copyProtoElementPointer(struct ProtoElementPointer * me) {
	struct ProtoElementPointer *ret=MALLOC(sizeof(struct ProtoElementPointer));
	ASSERT (ret);

	if (me->stringToken != NULL) ret->stringToken = STRDUP(me->stringToken);
	else ret->stringToken = NULL;
	ret->isNODE = me->isNODE;
	ret->isKEYWORD = me->isKEYWORD;
	ret->terminalSymbol = me->terminalSymbol;	
	ret->fabricatedDef = me->fabricatedDef;	
	return ret;
}





/* ************************************************************************** */
/* ******************************** OffsetPointer *************************** */
/* ************************************************************************** */

/* Constructor/destructor */
/* ********************** */

struct OffsetPointer* newOffsetPointer(struct X3D_Node* node, unsigned ofs)
{
 struct OffsetPointer* ret=MALLOC(sizeof(struct OffsetPointer));
 /* printf("creating offsetpointer %p\n", ret); */
 ASSERT(ret);

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

 ret->scriptDests=newVector(struct ScriptFieldInstanceInfo*, 4);
 ASSERT(ret->scriptDests);
 return ret;
}

void deleteProtoFieldDecl(struct ProtoFieldDecl* me)
{
 FREE_IF_NZ(me);
}

/* Other members */
/* ************* */

/* Routing to/from */
void protoFieldDecl_routeTo(struct ProtoFieldDecl* me,
 struct X3D_Node* node, unsigned ofs, int dir, struct VRMLParser* p)
{
 int i;
 size_t len=returnRoutingElementLength(me->type);
 ASSERT(me->mode==PKW_inputOutput || me->mode==PKW_inputOnly);

 /* For each script field mapped to this proto field, add a route */
 for (i=0; i!=vector_size(me->scriptDests); ++i) {
	struct ScriptFieldInstanceInfo* sfield = vector_get(struct ScriptFieldInstanceInfo*, me->scriptDests, i);
	struct Shader_Script* toscript = sfield->script;
	struct ScriptFieldDecl* tosfield = sfield->decl;
	if (dir == FROM_SCRIPT) {
		dir = SCRIPT_TO_SCRIPT;
	} else if (dir == 0) {
		dir = TO_SCRIPT;
	}
  	/* printf("protoFieldDecl_routeTo: registering route from %p %u to script dest %p %u %d\n", node, ofs, toscript->num, scriptFieldDecl_getRoutingOffset(tosfield), dir); */
	/* parser_registerRoute, cast the toscript->num so as to get around compiler warnings, it IS NOT a pointer, though */
	parser_registerRoute(p, node, ofs, (struct X3D_Node *) toscript->num, scriptFieldDecl_getRoutingOffset(tosfield), len, dir);
 }
}

void protoFieldDecl_routeFrom(struct ProtoFieldDecl* me,
 struct X3D_Node* node, unsigned ofs, int dir, struct VRMLParser* p)
{
 int i;
 size_t len=returnRoutingElementLength(me->type);

 ASSERT(me->mode==PKW_inputOutput || me->mode==PKW_outputOnly);

 /* For each script field mapped to this proto field, add a route */
 for (i=0; i!=vector_size(me->scriptDests); ++i) {
	struct ScriptFieldInstanceInfo* sfield = vector_get(struct ScriptFieldInstanceInfo*, me->scriptDests, i);
	struct Shader_Script* fromscript = sfield->script;
	struct ScriptFieldDecl* fromsfield = sfield->decl;
	if (dir == TO_SCRIPT) {
		dir = SCRIPT_TO_SCRIPT;
	} else if (dir == 0) {
		dir = FROM_SCRIPT;
	}
	/* parser_registerRoute, cast the fromscript->num to get around compiler warnings, but it is NOT a pointer, though */
	parser_registerRoute(p, (struct X3D_Node *) fromscript->num, scriptFieldDecl_getRoutingOffset(fromsfield), node, ofs, len, dir);
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
 ASSERT(ret);

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
 ASSERT(ret);

 /* printf("creating new ProtoDefinition %u\n", ret);  */

 ret->iface=newVector(struct ProtoFieldDecl*, 4);
 ASSERT(ret->iface);

 /* proto bodies are tokenized to help IS and routing to/from PROTOS */
 ret->deconstructedProtoBody=newVector(struct ProtoElementPointer*, 128);
 ASSERT(ret->deconstructedProtoBody);

 ret->protoDefNumber = latest_protoDefNumber++;
 ret->estimatedBodyLen = 0;

 return ret;
}

void deleteProtoDefinition(struct ProtoDefinition* me) {
	size_t i;

	for(i=0; i!=vector_size(me->iface); ++i)
		deleteProtoFieldDecl(vector_get(struct ProtoFieldDecl*, me->iface, i));
	deleteVector(struct ProtoDefinition*, me->iface);

	for(i=0; i!=vector_size(me->deconstructedProtoBody); ++i) {
		struct ProtoElementPointer* ele;
		ele = vector_get(struct ProtoElementPointer*, me->deconstructedProtoBody, i);
		FREE_IF_NZ(ele->stringToken);
		FREE_IF_NZ(ele);
	}
	deleteVector(struct ProtoRoute*, me->deconstructedProtoBody);
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

/* Copies the PROTO */
struct ProtoDefinition* protoDefinition_copy(struct VRMLLexer* lex, struct ProtoDefinition* me)
{
	struct ProtoDefinition* ret=MALLOC(sizeof(struct ProtoDefinition));
	size_t i;

	ASSERT(ret);

	/* printf("protoDefinition_copy: copying %u to %u\n", me, ret);  */
	/* Copy interface */
	ret->iface=newVector(struct ProtoFieldDecl*, vector_size(me->iface));
	ASSERT(ret->iface);
	for(i=0; i!=vector_size(me->iface); ++i)
		vector_pushBack(struct ProtoFieldDecl*, ret->iface,
	protoFieldDecl_copy(lex, vector_get(struct ProtoFieldDecl*, me->iface, i)));

	/* copy the deconsctructed PROTO body */
	ret->deconstructedProtoBody = newVector (struct ProtoElementPointer* , vector_size(me->deconstructedProtoBody));
	ASSERT (ret->deconstructedProtoBody);
	for(i=0; i!=vector_size(me->deconstructedProtoBody); ++i) {
		vector_pushBack(struct ProtoElementPointer *, ret->deconstructedProtoBody, copyProtoElementPointer(vector_get(struct ProtoElementPointer*, me->deconstructedProtoBody, i)));
	}
	
 	ret->estimatedBodyLen = me->estimatedBodyLen;
	ret->protoDefNumber = latest_protoDefNumber++;

	return ret;
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
#define DEEPCOPY_sfvec2d(l,v, i, h) v
#define DEEPCOPY_mfvec2d(l,v, i, h) v
#define DEEPCOPY_sfvec3f(l,v, i, h) v
#define DEEPCOPY_sfvec3d(l,v, i, h) v
#define DEEPCOPY_sfvec4d(l,v, i, h) v
#define DEEPCOPY_mfvec4d(l,v, i, h) v
#define DEEPCOPY_sfimage(l, v, i, h) v
#define DEEPCOPY_sfdouble(l, v, i, h) v
#define DEEPCOPY_sfmatrix3f(l, v, i, h) v
#define DEEPCOPY_sfmatrix4f(l, v, i, h) v
#define DEEPCOPY_mfmatrix3f(l, v, i, h) v
#define DEEPCOPY_mfmatrix4f(l, v, i, h) v
#define DEEPCOPY_sfmatrix3d(l, v, i, h) v
#define DEEPCOPY_sfmatrix4d(l, v, i, h) v
#define DEEPCOPY_mfmatrix3d(l, v, i, h) v
#define DEEPCOPY_mfmatrix4d(l, v, i, h) v


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
DEEPCOPY_MFVALUE(lex, double, Double)
DEEPCOPY_MFVALUE(lex, vec2f, Vec2f)
DEEPCOPY_MFVALUE(lex, vec3f, Vec3f)
DEEPCOPY_MFVALUE(lex, vec3d, Vec3d)

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
	UNUSED(node2); UNUSED(ret2); 

  #define END_NODE(n) \
    break; \
   }


  /* Copying of fields depending on type */

  #define FIELD(n, field, type, var) \
   ret2->var=DEEPCOPY_##type(lex, node2->var, new, hash); \
   UNUSED(ret2);

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
	struct Shader_Script* old_script;
	struct Shader_Script* new_script;
	struct X3D_Script* ret2 = (struct X3D_Script*) ret;
	struct X3D_Script* node2 = (struct X3D_Script*) node;
	int i, j, k;

 	old_script = node2->__scriptObj; 
	ret2->__scriptObj = new_Shader_Script(node);

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

/* a bunch of "OLDCODE" was here */
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



 ASSERT(!me->alreadySet);
 me->alreadySet=TRUE;

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
  /* printf ("protoFieldDecl_copy: copying field string for field... %s\n",me->fieldString);  */
 if (me->fieldString != NULL) ret->fieldString = STRDUP(me->fieldString);

 ret->mode=me->mode;
 ret->type=me->type;
 ret->name=me->name;
 /* printf ("copied mode %s type %s and name %d\n",stringPROTOKeywordType(ret->mode)
	, stringFieldtypeType(ret->type), ret->name);



 printf ("protoFieldDecl_copy, copied fieldString for proto field\n"); */

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
 ASSERT(ret);

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

 ASSERT(!pointerHash_get(me, o));

 if(!me->data[pos])
  me->data[pos]=newVector(struct PointerHashEntry, 4);
 
 entry.original=o;
 entry.copy=c;

 vector_pushBack(struct PointerHashEntry, me->data[pos], entry);
}


struct NestedProtoField* newNestedProtoField(struct ProtoFieldDecl* origField, struct ProtoFieldDecl* localField)
{
 struct NestedProtoField* ret = MALLOC(sizeof(struct NestedProtoField));
 ASSERT(ret);

 /* printf("creating nested field %p with values %p %p %p\n", ret, origField, localField, origProto); */

 ret->origField=origField;
 ret->localField = localField;

 return ret;
}



/* for backtracking to see if this field expansion has a proto expansion in it */
/* I know, I know - going backwards through input, but what can happen is that:


#VRML V2.0 utf8
PROTO StandardAppearance [ ] { Appearance { material Material { } } } #end of proto standardAppearance
PROTO Test [ field MFNode children [ ] ] { Transform { children IS children } } #end of protoTest

See the PROTO expansion in the "children" field of the Test proto invocation in the following:
Test { children Shape { appearance StandardAppearance { } geometry Sphere { } } } #end of test expansion 

the "children" field expansion gets:
Shape { appearance Group{FreeWRL__protoDef 5980080 children[ #PROTOGROUP
Appearance {material Material {}}]}#END PROTOGROUP
 geometry Sphere { } }

And, the whole thing becomes:
Group{FreeWRL__protoDef 5975760 children[ #PROTOGROUP
Transform {children  Shape { appearance Group{FreeWRL__protoDef 5980080 children[ #PROTOGROUP
Appearance {material Material {}}]}#END PROTOGROUP
 geometry Sphere { } } }]}#END PROTOGROUP

*/

void removeProtoFieldFromThis(char *inputCopy) {
	char *cp;
	int closeBrackCount = 0;
	int inName = FALSE;

	/* printf ("removeProtoFieldFromThis, have :%s:\n",inputCopy); */
	
	cp = inputCopy + strlen(inputCopy);
	while (cp > inputCopy) {
		/* printf ("cp at 1 :%s:\n",cp); */
		cp --;
		if (*cp == '}') closeBrackCount++;
		if (*cp == '{') { closeBrackCount--; *cp = '\0'; }

		if (closeBrackCount == 0) {
			if (*cp <= ' ') {
				if (inName) {
					/* printf ("inName, cp :%s: are we finished? \n",cp); */
					*cp = '\0';
					return;
				} else {
					*cp = '\0'; 
				}
			} else {
				inName = TRUE;
				/* printf ("worrying at :%s:\n",inputCopy); */
			}
		}		
	}
}


/* go through a proto invocation, and get the invocation fields (if any) */
void getProtoInvocationFields(struct VRMLParser *me, struct ProtoDefinition *thisProto) {
	char *copyPointer;
	char *initCP;
	char tmp;
	char *inputCopy = NULL;

	struct ProtoFieldDecl* pdecl = NULL;
	union anyVrml thisVal;

	#ifdef CPROTOVERBOSE
	printf ("start of getProtoInvocationFields, lexer %u nextIn :%s:\n",me->lexer, me->lexer->nextIn);
	#endif

	while (!lexer_closeCurly(me->lexer)) {

		lexer_setCurID(me->lexer);
		#ifdef CPROTOVERBOSE
		printf ("getProtoInvocationFields, id is %s\n",me->lexer->curID);
		#endif

		if (me->lexer->curID != NULL) {
			#ifdef CPROTOVERBOSE
			printf ("getProtoInvocationFields, curID != NULL\n");
			#endif

			pdecl = getProtoFieldDeclaration(me->lexer, thisProto,me->lexer->curID);
			if (pdecl != NULL) {
				#ifdef CPROTOVERBOSE
				printf ("getProtoInvocationFields, we have pdecl original as %s\n",pdecl->fieldString);
				#endif

	
				/* get a link to the beginning of this field */
				FREE_IF_NZ(me->lexer->curID);
				copyPointer = me->lexer->nextIn;
				initCP = me->lexer->startOfStringPtr;
				inputCopy = STRDUP(me->lexer->nextIn);
				/* printf ("inputCopy is %s\n",inputCopy); */

				#ifdef CPROTOVERBOSE
				printf ("getProtoInvocationField, going to parse type of %d mode (%s), curID %s, starting at :%s:\n",pdecl->type, stringPROTOKeywordType(pdecl->mode),
						me->lexer->curID, me->lexer->nextIn);
				#endif

				if (pdecl->mode != PKW_outputOnly) {

					lexer_setCurID(me->lexer);
					#ifdef CPROTOVERBOSE
					printf ("getProtoInvocationField, checking if this is an IS :%s:\n",me->lexer->curID);
					#endif

					if(me->curPROTO && lexer_keyword(me->lexer, KW_IS)) {
						struct ProtoFieldDecl* pexp;

						#ifdef CPROTOVERBOSE
						printf ("getProtoInvocationField, FOUND IS on protoInvocationFields; have to replace \n");
						#endif


						FREE_IF_NZ(me->lexer->curID);
						lexer_setCurID(me->lexer);
						#ifdef CPROTOVERBOSE
						printf ("getProtoInvocationField, going to try and find :%s: in curPROTO\n",me->lexer->curID);
						#endif

						pexp = getProtoFieldDeclaration(me->lexer,me->curPROTO,me->lexer->curID);
						if (pexp != NULL) {
							#ifdef CPROTOVERBOSE
							printf ("getProtoInvocationField, attaching :%s: to :%s:\n",pexp->fieldString, me->lexer->nextIn);
							#endif

							concatAndGiveToLexer(me->lexer, pexp->fieldString, me->lexer->nextIn);
						}
						FREE_IF_NZ(me->lexer->curID);
					}

					/* printf ("getPRotoInvocationFields, before parseType, nextIn %s\n",me->lexer->nextIn); */
					if (!parseType(me, pdecl->type, &thisVal)) {
						#ifdef CPROTOVERBOSE
						printf ("getProtoInvocationField, parsing error on field value in proto expansion\n");
						#endif

					} else {
						#ifdef CPROTOVERBOSE
						printf ("getProtoInvocationField, parsed field; nextin was :%s:, now :%s:\n",copyPointer, me->lexer->nextIn);
						#endif

					}
	
					/* was this possibly a proto expansion? */
					/* printf ("getProtoInvocationField, initCP %u startOfStringPtr %u\n",initCP, me->lexer->startOfStringPtr); */

					if (initCP != me->lexer->startOfStringPtr) {
						char *a1;
						char *a2;
						/* we had a proto expansion of a field here... */
							
						/* printf ("we are probably missing %s off of front\n", inputCopy);
						printf ("currently, lexer is %u\n",me->lexer); */

						a1 = strstr(me->lexer->startOfStringPtr,ENDPROTOGROUP);
						if (a1 != NULL) {
							/* printf ("a1 before adding len %s\n",a1); */
							a1 = a1+strlen(ENDPROTOGROUP);
		
							/* printf ("a1 is :%s:\n",a1);  */
							/* ok, where is that string in the original? */
							a2 = strstr (inputCopy,a1);

							if (a2 != NULL) {
								/* printf ("in inputcopy, we found :%s: here :%s:\n", a1, a2); */
								*a2 = '\0';
								/* printf ("should remove the last field of:%s:\n",inputCopy); */
								/* go back, and try and remove the PROTO name and parameters from this field */
								removeProtoFieldFromThis(inputCopy);

							} else inputCopy[0] = '\0';

						} else inputCopy[0] = '\0';

						/* printf ("so, inputCopy is :%s:\n",inputCopy); */


						copyPointer = me->lexer->startOfStringPtr;
					} else {
						inputCopy[0] = '\0';
					}

					/* copy over the new value */
					initCP = (char *) (me->lexer->nextIn);
					tmp = *initCP; *initCP = '\0';
					FREE_IF_NZ(pdecl->fieldString); 
					pdecl->fieldString = MALLOC (3 + strlen(inputCopy) + strlen(copyPointer));
					strcpy(pdecl->fieldString,inputCopy);
					strcat (pdecl->fieldString, " ");
					strcat (pdecl->fieldString,copyPointer);
					*initCP = tmp;
					#ifdef CPROTOVERBOSE
					printf ("getProtoInvocationFields, just copied :%s: remainder :%s:\n",pdecl->fieldString,me->lexer->nextIn);
					#endif
				} else {
					#ifdef CPROTOVERBOSE
					printf ("getProtoInvocationField, skipped parsing this type of %s\n",stringPROTOKeywordType(pdecl->type));
					#endif

				}
			}
		}
		#ifdef CPROTOVERBOSE
		printf ("after pt, curID %s\n",me->lexer->curID);
		#endif

		lexer_skip (me);
		FREE_IF_NZ (inputCopy);
	}

	#ifdef CPROTOVERBOSE
	printf ("end of getProtoInvocationFields\n");
	#endif

}

/* find the proto field declare for a particular proto */
struct ProtoFieldDecl* getProtoFieldDeclaration(struct VRMLLexer *me, struct ProtoDefinition *thisProto, char *thisID)
{
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


/* take an ascii string apart, and tokenize this so that we can extract
	IS fields, and route to/from properly */
void tokenizeProtoBody(struct ProtoDefinition *me, char *pb) {
	struct VRMLLexer *lex;
	vrmlInt32T tmp32;
	vrmlFloatT tmpfloat;
	vrmlStringT tmpstring;
	struct ProtoElementPointer* ele;
	struct ProtoElementPointer* tE;
	struct ProtoElementPointer* tD;
	int toPush;
	indexT i;
	indexT ct = 0;

	/* remove spaces at start of string, to help to see if string is empty */
	while ((*pb != '\0') && (*pb <= ' ')) pb++;

	/* record this body length to help us with MALLOCing when expanding PROTO */
	me->estimatedBodyLen = strlen(pb) * 2;

	lex = newLexer();
	lexer_fromString(lex,pb);

	while (lex->isEof == FALSE) {
		ele = newProtoElementPointer();
		toPush = TRUE; /* only put this new element on Vector if it is successful */

		if (lexer_setCurID(lex)) {
			if ((ele->isKEYWORD = findFieldInKEYWORDS(lex->curID)) == ID_UNDEFINED)  {
				if ((ele->isNODE = findFieldInNODES(lex->curID)) == ID_UNDEFINED)  {
					ele->stringToken = lex->curID;
					lex->curID = NULL;
				}
			}
			FREE_IF_NZ(lex->curID);

			/* ok, if this is an IS, go back to the controlling NODE, and ENSURE that
			   it has a DEF to enable it to accept external ROUTE requests */
			if (ele->isKEYWORD == KW_IS) {
				i = vector_size(me->deconstructedProtoBody) -1; /* remember, 0->(x-1), not 1->x */
				/* printf ("tokenizeProtoBody, found an IS, lets go back... we currently have %d\n",i); */
				if (i>0) {
					do {
                				tE = vector_get(struct ProtoElementPointer*, me->deconstructedProtoBody, i);
                				ASSERT(tE);
						i--;
					} while ((i>0) && (tE->isNODE == ID_UNDEFINED));

					/* ok, we should be at the NODE for this IS */

					if (tE->isNODE != ID_UNDEFINED) {
						/* printf ("node for IS is an %s\n",stringNodeType(tE->isNODE)); */
						tD = NULL;
						i--; 		/* back up to where the DEF should be */
						if (i>=0) {
							tD = vector_get(struct ProtoElementPointer*, me->deconstructedProtoBody, i);
							/* printf ("backed up, have NO %d KW %d te %d str %s\n",
								tD->isNODE, tD->isKEYWORD, tD->terminalSymbol, tD->stringToken); */

							ASSERT (tD);
							if (tD->isKEYWORD != KW_DEF) {
								tD = NULL; /* not a DEF, we have to make a special note of this */
							}

						}

						/* if we DID NOT find a DEF back in the stack, tell this NODE to put a DEF in */
						if (tD == NULL) {
							ASSIGN_UNIQUE_ID(tE)
						}
					}
				}
			}
				
		} else if (lexer_point(lex)) { ele->terminalSymbol = (indexT) '.';
		} else if (lexer_openCurly(lex)) { ele->terminalSymbol = (indexT) '{';
		} else if (lexer_closeCurly(lex)) { ele->terminalSymbol = (indexT) '}';
		} else if (lexer_openSquare(lex)) { ele->terminalSymbol = (indexT) '[';
		} else if (lexer_closeSquare(lex)) { ele->terminalSymbol = (indexT) ']';
		} else if (lexer_colon(lex)) { ele->terminalSymbol = (indexT) ':';

		} else if (lexer_string(lex,&tmpstring)) { 
			/* must put the double quotes back on */
			ele->stringToken = MALLOC (tmpstring->len + 3);
			sprintf (ele->stringToken, "\"%s\"",tmpstring->strptr);
		} else {
			/* printf ("probably a number, scan along until it is done. :%s:\n",lex->nextIn); */
			if ((*lex->nextIn == '-') || (*lex->nextIn >= '0') && (*lex->nextIn <= '9')) {
				uintptr_t ip; uintptr_t fp; char *cur;
				int ignore;

				/* see which of float, int32 gobbles up more of the string */
				cur = lex->nextIn;

				ignore = lexer_float(lex,&tmpfloat);
				fp = (uintptr_t) lex->nextIn;
				/* put the next in pointer back to the beginning of the number */
				lex->nextIn = cur;


				ignore = lexer_int32(lex,&tmp32);
				ip = (uintptr_t) lex->nextIn;

				/* put the next in pointer back to the beginning of the number */
				lex->nextIn = cur;

				ele->stringToken = MALLOC (10);
				ASSERT (ele->stringToken);

				/* printf ("so we read in from :%s:\n",lex->nextIn); */

				/* now, really scan depending on the type - which one got us further? */
				/* note that if they are the same, we choose the INT because of expansion
				   problems if we write an int as a float... (eg, coordinates in an IFS) */
				if (ip >= fp) {
					/* printf ("this is an int\n"); */
					ignore = lexer_int32(lex,&tmp32);
					sprintf (ele->stringToken,"%d",tmp32);
				} else {
					/* printf ("this is a float\n"); */
					ignore = lexer_float(lex,&tmpfloat); 
					sprintf (ele->stringToken,"%f",tmpfloat);
				}

			
			} else {
				if (*lex->nextIn != '\0') ConsoleMessage ("lexer_setCurID failed on char :%d:\n",*lex->nextIn);
				lex->nextIn++;
				toPush = FALSE;
			}
		}


		/* printf ("newprotoele %d, NODE %d KW %d ts %d st %s\n",ct, ele->isNODE, ele->isKEYWORD, ele->terminalSymbol, ele->stringToken); */
		ct ++;

		/* push this element on the vector for the PROTO */
		if (toPush) vector_pushBack(struct ProtoElementPointer*, me->deconstructedProtoBody, ele);
	}
	deleteLexer(lex);
}



char *protoExpand (struct VRMLParser *me, indexT nodeTypeU, struct ProtoDefinition **thisProto) {
	char *newProtoText;
	int newProtoTextMallocLen;
	int newProtoTextIndex = 0;
	char thisID[1000];
	indexT i;
	indexT protoElementCount;
	struct ProtoElementPointer* ele;
	struct ProtoElementPointer* tempEle;
	
	struct ProtoElementPointer* lastKeyword = NULL;
	struct ProtoElementPointer* lastNode = NULL;

	#ifdef CPROTOVERBOSE
	printf ("start of protoExpand\n");
	#endif

	*thisProto = protoDefinition_copy(me->lexer, vector_get(struct ProtoDefinition*, me->PROTOs, nodeTypeU));
	#ifdef CPROTOVERBOSE
	printf ("expanding proto, "); 
	printf ("thisProto %u, me->curPROTO %u\n",(*thisProto),me->curPROTO);
	#endif


	newProtoTextMallocLen = (*thisProto)->estimatedBodyLen * 2 + strlen(PROTOGROUPNUMBER) +strlen(STARTPROTOGROUP) + strlen (ENDPROTOGROUP) + 10;
	newProtoText = MALLOC(newProtoTextMallocLen);

	APPEND_STARTPROTOGROUP

	/* printf ("copying proto fields here\n"); */
	/* copy the proto fields, so that we have either the defined field, or the field at invocation */

	#ifdef CPROTOVERBOSE
	printf ("\n\ngetProtoInvocationFields being called\n");
	#endif

	getProtoInvocationFields(me,(*thisProto));

	/* go through each part of this deconstructedProtoBody, and see what needs doing... */
	protoElementCount = vector_size((*thisProto)->deconstructedProtoBody);

	#ifdef CPROTOVERBOSE
		printf ("protoExpand - protoEpementCount %d\n",protoElementCount);
	#endif
	
	i = 0;
	while (i < protoElementCount) {
		/* get the current element */
		ele = vector_get(struct ProtoElementPointer*, (*thisProto)->deconstructedProtoBody, i);
		ASSERT(ele);
		#ifdef XXX
		PROTO_CAT ( "# at A\n");
		#endif

		#ifdef CPROTOVERBOSE
		printf ("\nPROTO - ele %d of %d;  is %u isNODE %d isKEYWORD %d ts %d st %s\n",i, protoElementCount, ele, ele->isNODE, ele->isKEYWORD, ele->terminalSymbol, ele->stringToken); 
		#endif

		/* this is a NODE, eg, "SphereSensor". If we need this DEFined because it contains an IS, then make the DEF */
		if (ele->isNODE != ID_UNDEFINED) {
			/* keep this entry around, to see if there is an outputOnly ISd field */
			lastNode = ele; 

			/* possibly this is a synthetic DEF for possible external IS routing */
			if (ele->fabricatedDef != ID_UNDEFINED) {
			#ifdef XXX
			PROTO_CAT ( "# at B\n");
			printf ("newProtoText :%s:\n",newProtoText);
			#endif
				sprintf (thisID,"DEF %s%d_",FABRICATED_DEF_HEADER,ele->fabricatedDef);
				APPEND_THISID
				APPEND_SPACE
			}
			#ifdef XXX
			PROTO_CAT ( "# at C\n");
			printf ("newProtoText :%s:\n",newProtoText);
			#endif
			APPEND_NODE
			APPEND_SPACE

		/* Maybe this is a KEYWORD, like "USE" or "DEFINE". if this is an "IS" DO NOT print it out */
		} else if (ele->isKEYWORD != ID_UNDEFINED) {
			if ((ele->isKEYWORD == KW_DEF) ||
			    (ele->isKEYWORD == KW_USE) ||
			    (ele->isKEYWORD == KW_IS) ||
			    (ele->isKEYWORD == KW_ROUTE) ||
			    (ele->isKEYWORD == KW_TO))
				lastKeyword = ele;

			if (ele->isKEYWORD != KW_IS) { 
				#ifdef XXX
				PROTO_CAT ( "# at D\n");
				printf ("newProtoText :%s:\n",newProtoText);
				#endif
				APPEND_KEYWORD APPEND_SPACE 
			}

		/* Hmmm - maybe this is a "{" or something like that */
		} else if (ele->terminalSymbol != ID_UNDEFINED) {
			#ifdef XXX
			PROTO_CAT ( "# at E\n");
			printf ("newProtoText :%s:\n",newProtoText);
			#endif

			APPEND_TERMINALSYMBOL

		/* nope, this is a fieldname, DEF name, or string, or something equivalent */
		} else if (ele->stringToken != NULL) {
			/* ok we have a name. Is the NEXT token an IS? If not, just continue */
			/* printf ("we have a normal stringToken - :%s:\n",ele->stringToken); */

			if (i<(protoElementCount-2)) {
				tempEle = vector_get(struct ProtoElementPointer*, (*thisProto)->deconstructedProtoBody, i+1);
				if ((tempEle != NULL) && (tempEle->isKEYWORD == KW_IS)) {
					int tl =100;
					char *newTl = MALLOC(100);
					newTl[0] = '\0';

					/* printf ("next element is an IS \n"); */
					tempEle = vector_get(struct ProtoElementPointer*, (*thisProto)->deconstructedProtoBody, i+2);
					/* printf ("ok, so IS of :%s: is :%s:\n",ele->stringToken, tempEle->stringToken); */

					replaceProtoField(me->lexer, *thisProto, tempEle->stringToken,&newTl,&tl);
					 /* printf ("IS replacement is len %d, str :%s:\n",strlen(newTl), newTl);  */

					/* is there actually a value for this field?? */
					if SOMETHING_IN_ISVALUE {
						#ifdef XXX
						PROTO_CAT ( "# at F\n");
						printf ("newProtoText :%s:\n",newProtoText);
						#endif

						VERIFY_OUTPUT_LEN(strlen(newTl))
						APPEND_STRINGTOKEN
						APPEND_SPACE
						APPEND_ISVALUE

					} else if (lastNode->isNODE == NODE_Script) {
						#ifdef XXX
						PROTO_CAT ( "# at G\n");
						printf ("newProtoText :%s:\n",newProtoText);
						#endif

						/* Script nodes NEED the fieldname, even if it is blank, so... */
						APPEND_STRINGTOKEN
						APPEND_SPACE
					}

					i+=2; /* skip the IS and the field */
					FREE_IF_NZ(newTl);
				} else { 
					#ifdef XXX
					PROTO_CAT ( "# at H\n");
					printf ("newProtoText :%s:\n",newProtoText);
					#endif

					APPEND_EDITED_STRINGTOKEN
				}

			} else { 
				#ifdef XXX
				PROTO_CAT ( "# at I\n");
				printf ("newProtoText :%s:\n",newProtoText);
				#endif
				APPEND_EDITED_STRINGTOKEN
			}

			APPEND_SPACE
		} else {
			/* this is a blank proto... */
			/* ConsoleMessage ("PROTO EXPANSION, vector element %d, can not expand\n",i); */
		}

		/* possible overflow condition */
		VERIFY_OUTPUT_LEN(128)

		/* go to the next token */
		i++;
	}

	APPEND_ENDPROTOGROUP

	#ifdef CPROTOVERBOSE
	printf ("so, newProtoText %s\n",newProtoText);
	#endif

	return newProtoText;
}

/* for resolving ROUTEs to/from PROTOS... */
/* this is a PROTO; go through and find the node, and fill in the correct curID so that parsing can
   continue. XXX - only works with one IS routed field; if a PROTO has more than one IS, then this will fail. */
BOOL resolveProtoNodeField(struct VRMLParser *me, struct ProtoDefinition *Proto, struct X3D_Node **Node) {
	indexT i,j;
	indexT ret;
	struct ProtoElementPointer *ele;
	char thisID[200];

	#ifdef CPROTOVERBOSE
	printf ("resolveProtoNodeField, looking for field %s\n",me->lexer->curID);
	#endif

	for(i=0; i!=vector_size(Proto->deconstructedProtoBody); ++i) {
		ele = vector_get(struct ProtoElementPointer*, Proto->deconstructedProtoBody, i);
		ASSERT(ele);

		/* printf ("ele %d is %u isNODE %d isKEYWORD %d ts %d st %s\n",i, ele, ele->isNODE, ele->isKEYWORD, ele->terminalSymbol, ele->stringToken); */

		/* is this an KW_IS? */
		if (ele->isKEYWORD == KW_IS) {
			/* printf ("resolveProtoNodeField, found an IS\n"); */
			if (i<(vector_size(Proto->deconstructedProtoBody)-1)) {
				/* look for a match with this field */
				j = i+1;
				ele = vector_get(struct ProtoElementPointer*, Proto->deconstructedProtoBody, j);
				/* printf ("resolveProtoNodeField next ele %d is %u isNODE %d isKEYWORD %d ts %d st %s\n",j, ele, ele->isNODE, ele->isKEYWORD, ele->terminalSymbol, ele->stringToken); */
				/* printf ("resolveProtoNodeField comparing %s to %s\n",me->lexer->curID, ele->stringToken); */
				if (strcmp(me->lexer->curID,ele->stringToken) == NULL) {
					/* printf ("resolveProtoNodeField ID MATCH!!\n"); */
					/* go back 2 (the word before the IS) and get the real field name */
					j = i-1;
					ele = vector_get(struct ProtoElementPointer*, Proto->deconstructedProtoBody, j);
					#ifdef CPROTOVERBOSE
					printf ("resolveProtoNodeField REAL FIELD IS %s\n",ele->stringToken);
					#endif

					if (ele->stringToken != NULL) {
						FREE_IF_NZ(me->lexer->curID);
						me->lexer->curID = STRDUP(ele->stringToken);
					}

					#ifdef CPROTOVERBOSE
					printf ("resolveProtoNodeField ok, now going back and finding the DEF name for the node for this field \n");
					#endif

					while (j>0) {
						j--;
						ele = vector_get(struct ProtoElementPointer*, Proto->deconstructedProtoBody, j);
						if ((ele->fabricatedDef != ID_UNDEFINED) || (ele->isKEYWORD == KW_DEF)) {
							if (ele->fabricatedDef != ID_UNDEFINED) {
								/* printf ("FOUND A FABRICATED DEF HERE\n"); */
								sprintf (thisID,"%s%d_",FABRICATED_DEF_HEADER,ele->fabricatedDef);
							} else {
								/* printf ("FOUND A KW_DEF HERE\n"); */
								ele = vector_get(struct ProtoElementPointer*, Proto->deconstructedProtoBody, j+1);
								/* printf ("name is %s\n",ele->stringToken); */
								sprintf (thisID,"%s%d_",FABRICATED_DEF_HEADER,Proto->protoDefNumber);
								strcat (thisID, ele->stringToken);
							}

							/* ok, now we have a DEF name, lets look it up. */
							#ifdef CPROTOVERBOSE
							printf ("resolveProtoNodeField looking up :%s:\n",thisID);
							#endif

							lexer_specialID_string(me->lexer, NULL, &ret, NULL, 
								0, stack_top(struct Vector*, me->lexer->userNodeNames), thisID);
							#ifdef CPROTOVERBOSE
							printf ("resolveProtoNodeField after lookup, ret %d\n",ret);
							#endif


        						/* If we're USEing it, it has to already be defined. */
        						ASSERT(ret!=ID_UNDEFINED);

        						/* It also has to be in the DEFedNodes stack */
        						ASSERT(me->DEFedNodes && !stack_empty(me->DEFedNodes) &&
        							ret<vector_size(stack_top(struct Vector*, me->DEFedNodes)));

        						/* Get a pointer to the X3D_Node structure for this DEFed node and return it in ret */
        						*Node = vector_get(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes), ret);
							#ifdef CPROTOVERBOSE
							printf ("RETURNING; so, now the node of the DEF is %u, type %s\n",*Node, stringNodeType((*Node)->_nodeType));
							#endif


							return TRUE;


						}
					}
				} else {
					#ifdef CPROTOVERBOSE
					printf ("resolveProtoNodeField, no match, maybe next one?\n");
					#endif
				}
			}
		}
	}
	/* no luck here */
	#ifdef CPROTOVERBOSE
	printf  ("could not find encompassing DEF for PROTO field %s of node %s\n", me->lexer->curID,thisID);
	#endif

	ConsoleMessage ("could not find encompassing DEF for PROTO field %s", me->lexer->curID);
	return FALSE;
}