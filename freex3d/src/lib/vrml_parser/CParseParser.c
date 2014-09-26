/*


  ???

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldSet.h"
#include "../input/EAIHeaders.h"	/* resolving implicit declarations */
#include "../input/EAIHelpers.h"	/* resolving implicit declarations */
#include "CParseParser.h"
#include "CParseLexer.h"
#include "CProto.h"
#include "CParse.h"
#include "CRoutes.h"			/* for upper_power_of_two */

#define PARSE_ERROR(msg) \
 { \
  CPARSE_ERROR_CURID(msg); \
  FREE_IF_NZ(me->lexer->curID); \
  PARSER_FINALLY; \
 }
#define PARSER_FINALLY

#define DEFMEM_INIT_SIZE        16

#define DJ_KEEP_COMPILER_WARNING 0

typedef struct pCParseParser{
	char fw_outline[2000];
	int foundInputErrors;// = 0;
	int useBrotos; //use binary protos rather than text protos

}* ppCParseParser;
void *CParseParser_constructor(){
	void *v = malloc(sizeof(struct pCParseParser));
	memset(v,0,sizeof(struct pCParseParser));
	return v;
}
void CParseParser_init(struct tCParseParser *t){
	//public
	//private
	t->prv = CParseParser_constructor();
	{
		ppCParseParser p = (ppCParseParser)t->prv;
		p->foundInputErrors = 0;
		p->useBrotos = 3; //0= none/old-way, non-zero =wrl parsing broto only rendering, DEF/IS/script tables are in new proto 3=EXTERNPROTO is broto wrapper
	}
}
	//ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
BOOL usingBrotos()
{
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
	return (BOOL)p->useBrotos;
}
//static int foundInputErrors = 0;
void resetParseSuccessfullyFlag(void) { 
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
	p->foundInputErrors = 0;
}
int parsedSuccessfully(void) {
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
	return p->foundInputErrors == 0;
}

/* Parsing a specific type */
/* NOTE! We have to keep the order of these function calls the same
   as the FIELDTYPE names, created from the @VRML::Fields = qw/ in
   VRMLFields.pm (which writes the FIELDTYPE* defines in 
   CFuncs/Structs.h. Currently (September, 2008) this is the list:
   SFFloat
   MFFloat
   SFRotation
   MFRotation
   SFVec3f
   MFVec3f
   SFBool
   MFBool
   SFInt32
   MFInt32
   SFNode
   MFNode
   SFColor
   MFColor
   SFColorRGBA
   MFColorRGBA
   SFTime
   MFTime
   SFString
   MFString
   SFVec2f
   MFVec2f
   SFImage
   FreeWRLPTR
   SFVec3d
   MFVec3d
   SFDouble
   MFDouble
   SFMatrix3f
   MFMatrix3f
   SFMatrix3d
   MFMatrix3d
   SFMatrix4f
   MFMatrix4f
   SFMatrix4d
   MFMatrix4d
   SFVec2d
   MFVec2d
   SFVec4f
   MFVec4f
   SFVec4d
   MFVec4d
*/

/* Parses nodes, fields and other statements. */
static BOOL parser_routeStatement(struct VRMLParser*);
static BOOL parser_componentStatement(struct VRMLParser*);
static BOOL parser_exportStatement(struct VRMLParser*);
static BOOL parser_importStatement(struct VRMLParser*);
static BOOL parser_metaStatement(struct VRMLParser*);
static BOOL parser_profileStatement(struct VRMLParser*);

static BOOL parser_protoStatement(struct VRMLParser*);
static BOOL parser_nodeStatement(struct VRMLParser*, vrmlNodeT*);
static BOOL parser_node(struct VRMLParser*, vrmlNodeT*, int);
static BOOL parser_field(struct VRMLParser*, struct X3D_Node*);



static BOOL parser_sffloatValue_ (struct VRMLParser *, void *);
static BOOL parser_sfint32Value_ (struct VRMLParser *, void *);
static BOOL parser_sftimeValue (struct VRMLParser *, void *);
static BOOL parser_sfboolValue (struct VRMLParser *, void *);
static BOOL parser_sfnodeValue (struct VRMLParser *, void *);
static BOOL parser_sfrotationValue (struct VRMLParser *, void *);
static BOOL parser_sfcolorValue (struct VRMLParser *, void *);
static BOOL parser_sfcolorrgbaValue (struct VRMLParser *, void *);
static BOOL parser_sfmatrix3fValue (struct VRMLParser *, void *);
static BOOL parser_sfmatrix4fValue (struct VRMLParser *, void *);
static BOOL parser_sfvec2fValue (struct VRMLParser *, void *);
static BOOL parser_sfvec4fValue (struct VRMLParser *, void *);
static BOOL parser_sfvec2dValue (struct VRMLParser *, void *);
static BOOL parser_sfvec3dValue (struct VRMLParser *, void *);
static BOOL parser_sfvec4dValue (struct VRMLParser *, void *);
static BOOL parser_sfmatrix3dValue (struct VRMLParser *, void *);
static BOOL parser_sfmatrix4dValue (struct VRMLParser *, void *);
static BOOL parser_mfboolValue(struct VRMLParser*, void*);
static BOOL parser_mfcolorValue(struct VRMLParser*, void*);
static BOOL parser_mfcolorrgbaValue(struct VRMLParser*, void*);
static BOOL parser_mffloatValue(struct VRMLParser*, void*);
static BOOL parser_mfint32Value(struct VRMLParser*, void*);
static BOOL parser_mfnodeValue(struct VRMLParser*, void*);
static BOOL parser_mfrotationValue(struct VRMLParser*, void*);
static BOOL parser_mfstringValue(struct VRMLParser*, void*);
static BOOL parser_mftimeValue(struct VRMLParser*, void*);
static BOOL parser_mfvec2fValue(struct VRMLParser*, void*);
static BOOL parser_mfvec3fValue(struct VRMLParser*, void*);
static BOOL parser_mfvec3dValue(struct VRMLParser*, void*);
static BOOL parser_sfstringValue_(struct VRMLParser*, void*);
static BOOL parser_sfimageValue(struct VRMLParser*, void*);




#define parser_sfvec3fValue(me, ret) \
 parser_sfcolorValue(me, ret)


/* for those types not parsed yet, call this to print an error message */
static BOOL parser_fieldTypeNotParsedYet(struct VRMLParser* me, void* ret);


BOOL (*PARSE_TYPE[])(struct VRMLParser*, void*)={
    &parser_sffloatValue_, &parser_mffloatValue,
    &parser_sfrotationValue, &parser_mfrotationValue,
    &parser_sfcolorValue, &parser_mfvec3fValue,
    &parser_sfboolValue, &parser_mfboolValue,
    &parser_sfint32Value_, &parser_mfint32Value,
    &parser_sfnodeValue, &parser_mfnodeValue,
    &parser_sfcolorValue, &parser_mfcolorValue,
    &parser_sfcolorrgbaValue, &parser_mfcolorrgbaValue,
    &parser_sftimeValue, &parser_mftimeValue,
    &parser_sfstringValue_, &parser_mfstringValue,
    &parser_sfvec2fValue, &parser_mfvec2fValue,
    &parser_sfimageValue,  /* SFImage */ 
    &parser_fieldTypeNotParsedYet, /* FreeWRLPTR 23 */
    &parser_sfvec3dValue, &parser_mfvec3dValue,
    &parser_sftimeValue, &parser_mftimeValue,
    &parser_sfmatrix3fValue, &parser_fieldTypeNotParsedYet, /* Matrix3f */
    &parser_sfmatrix3dValue, &parser_fieldTypeNotParsedYet, /* Matrix3d */
    &parser_sfmatrix4fValue, &parser_fieldTypeNotParsedYet, /* Matrix4f */
    &parser_sfmatrix4dValue, &parser_fieldTypeNotParsedYet, /* Matrix4d */
    &parser_sfvec2dValue, &parser_fieldTypeNotParsedYet, /* Vec2d */
    &parser_sfvec4fValue, &parser_fieldTypeNotParsedYet, /* Vec4f */
    &parser_sfvec4dValue, &parser_fieldTypeNotParsedYet, /* Vec4d */
    &parser_fieldTypeNotParsedYet, /* FreeWRLThread */
};

/* for error messages */
//char fw_outline[2000];


/* General processing macros */
#define PROCESS_EVENT(constPre, destPre, node, field, type, var, realType) \
 case constPre##_##field: \
  destPre##Ofs=(int) offsetof(struct X3D_##node, var); \
  destPre##Type = realType; \
  break;

#define EVENT_BEGIN_NODE(fieldInd, ptr, node) \
 case NODE_##node: \
 { \
  switch(fieldInd) \
  {

#define EVENT_END_NODE(myn,fieldString) \
  default: \
printf ("EVENT_END_NODE no %s at %s:%d\n",fieldString,__FILE__,__LINE__); \
	CPARSE_ERROR_FIELDSTRING("ERROR: Unsupported event ",fieldString); \
        PARSER_FINALLY;  \
        return FALSE;  \
  } \
  break; \
 }

#define EVENT_NODE_DEFAULT \
 default: \
  PARSE_ERROR("Parser - PROCESS_EVENT: Unsupported node!")

/************************************************************************************************/
/* parse an SF/MF; return the parsed value in the defaultVal field */
BOOL parseType(struct VRMLParser* me, int type,   union anyVrml *defaultVal) {
    ASSERT(PARSE_TYPE[type]);
    
    //ConsoleMessage ("parseType, type %d, dfv %p",type,defaultVal);
    if (type == ID_UNDEFINED) return false;
    
    return PARSE_TYPE[type](me, (void*)defaultVal);
}


/* put the string value of the PROTO field into the input stream */
void replaceProtoField(struct VRMLLexer *me, struct ProtoDefinition *thisProto, char *thisID, char **outTextPtr, size_t *outSize) {
    struct ProtoFieldDecl* pdecl=NULL;
    /* find the ascii name, and try and find it's protodefinition by type */
#ifdef CPARSERVERBOSE
    printf ("start of replaceProtoField for id %s\n",thisID);
#endif

    /* BTW - I'm not sure which array to look in, so I end up looking in them all */
    pdecl = getProtoFieldDeclaration(me,thisProto,thisID);
#ifdef CPARSERVERBOSE
    printf ("replaceProtoField, so, pdecl is %u\n",pdecl);
#endif

    if (pdecl!=NULL) {
        if (pdecl->fieldString!=NULL) {
#ifdef CPARSERVERBOSE
            printf ("PROTO FIELD VALUE IS :xxx: %s :xxx:\n",pdecl->fieldString);
            printf ("see if we need to realloc: outSize %d, curlen %d, newlen %d\n",*outSize, strlen(*outTextPtr), strlen (pdecl->fieldString));
#endif

            if (strlen(pdecl->fieldString) > *outSize-5) {
#ifdef CPARSERVERBOSE
                printf ("replaceProtoField, reallocing outTextPtr\n");
#endif

                *outSize = (int) strlen(pdecl->fieldString) + 5;
                *outTextPtr = REALLOC(*outTextPtr,*outSize);
            }
        
            strcat (*outTextPtr,pdecl->fieldString);
        } else {
#ifdef CPARSERVERBOSE
            printf ("PROTO FIELD VALUE is NULL, just skip\n");
#endif
        
        }
    }
#ifdef CPARSERVERBOSE
    printf ("replaceProtoField returning for id %s\n",thisID);
#endif
        
}


/* ************************************************************************** */
/* Constructor and destructor */

struct VRMLParser* newParser(void* ptr, unsigned ofs, int parsingX3DfromXML) {
    struct VRMLParser* ret=MALLOC(struct VRMLParser *, sizeof(struct VRMLParser));
    ret->lexer=newLexer();
    ASSERT(ret->lexer);
    ret->ptr=ptr;
    ret->ofs=ofs;
    ret->curPROTO=NULL;
    ret->DEFedNodes = NULL;
    ret->PROTOs = NULL;
    ret->parsingX3DfromXML = parsingX3DfromXML;
	ret->brotoDEFedNodes = NULL;
    return ret;
}

struct VRMLParser* reuseParser(void* ptr, unsigned ofs) {
    struct VRMLParser* ret;
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;

    /* keep the defined nodes around, etc */
    ret = globalParser;

    /* keep the old lexer around, so that the ASSERTs do not get confused with sizes of stacks, etc
       if (ret->lexer != NULL) deleteLexer(ret->lexer);
       ret->lexer=newLexer();
    */
    ASSERT(ret->lexer);
    ret->ptr=ptr;
    ret->ofs=ofs;
/* We now need to keep the PROTOS and DEFS around 
   ret->curPROTO=NULL;
   ret->DEFedNodes = NULL;
   ret->PROTOs = NULL;
*/ 

    return ret;
}

void deleteParser(struct VRMLParser* me)
{
    ASSERT(me->lexer);
    deleteLexer(me->lexer);

    FREE_IF_NZ (me);
}

static void parser_scopeOut_DEFUSE();
static void parser_scopeOut_PROTO();
void parser_destroyData(struct VRMLParser* me)
{

    /* printf ("\nCParser: parser_destroyData, destroyCParserData: , destroying data, me->DEFedNodes %u\n",me->DEFedNodes); */

    /* DEFed Nodes. */
    if(me->DEFedNodes)
    {
        while(!stack_empty(me->DEFedNodes))
            parser_scopeOut_DEFUSE(me);
        deleteStack(struct Vector*, me->DEFedNodes);
        me->DEFedNodes=NULL;
    }
    ASSERT(!me->DEFedNodes);

    /* PROTOs */
    /* FIXME: PROTOs must not be stacked here!!! */
    if(me->PROTOs)
    {
        while(!stack_empty(me->PROTOs))
            parser_scopeOut_PROTO(me);
        deleteStack(struct Vector*, me->PROTOs);
        me->PROTOs=NULL;
    }
    ASSERT(!me->PROTOs);

    lexer_destroyData(me->lexer);

    /* zero script count */
    zeroScriptHandles ();       
}

/* Scoping */

static void parser_scopeIn_DEFUSE(struct VRMLParser* me)
{
    if(!me->DEFedNodes)
        me->DEFedNodes=newStack(struct Vector*);

    ASSERT(me->DEFedNodes);
    stack_push(struct Vector*, me->DEFedNodes,
               newVector(struct X3D_Node*, DEFMEM_INIT_SIZE));
    ASSERT(!stack_empty(me->DEFedNodes));
}

/* PROTOs are scope by the parser in the PROTOs vector, and by the lexer in the userNodeTypesVec vector.  
   This is accomplished by keeping track of the number of PROTOs defined so far in a the userNodeTypesStack.  
   When a new scope is entered, the number of PROTOs defined up to this point is pushed onto the stack.  When
   we leave the scope the number of PROTOs now defined is compared to the top value of the stack, and the newest
   PROTOs are removed until the number of PROTOs defined equals the top value of the stack.

   Management of the userNodeTypesStack is accomplished by the lexer.  Therefore, scoping in PROTOs for the parser
   does nothing except to make sure that the PROTOs vector has been initialized. */ 
static void parser_scopeIn_PROTO(struct VRMLParser* me)
{
    if(!me->PROTOs) {
        me->PROTOs=newVector(struct ProtoDefinition*, DEFMEM_INIT_SIZE);
    }
}

static void parser_scopeOut_DEFUSE(struct VRMLParser* me)
{
    ASSERT(!stack_empty(me->DEFedNodes));
    /* FIXME:  Can't delete individual nodes, as they might be referenced! */
    deleteVector(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes));
    stack_pop(struct Vector*, me->DEFedNodes);
}

/* Scoping out of PROTOs.  Check the difference between the number of PROTO definitions currently 
   available and the number of PROTO definitions available when we first entered this scope (this is
   the top value on the userNodeTypesVec stack). Remove all PROTO definitions from the PROTOs list that have 
   been added since we first entered this scope. */ 
static void parser_scopeOut_PROTO(struct VRMLParser* me)
{
    /* Do not delete the ProtoDefinitions, as they are referenced in the scene
     * graph!  TODO:  How to delete them properly? */

    vector_popBackN(struct ProtoDefinition*, me->PROTOs, lexer_getProtoPopCnt(me->lexer));
    lexer_scopeOut_PROTO(me->lexer);
}

void parser_scopeIn(struct VRMLParser* me)
{
    lexer_scopeIn(me->lexer);
    parser_scopeIn_DEFUSE(me);
    parser_scopeIn_PROTO(me);
}
void parser_scopeOut(struct VRMLParser* me)
{
    parser_scopeOut_DEFUSE(me);
    parser_scopeOut_PROTO(me);
    lexer_scopeOut(me->lexer);
}
/* save your spot in the lexer stream so if you're in the wrong handler
   you can backup and let something else try to handle it*/
#define STRDUP_IF_NZ(_ptr) ((_ptr) ? STRDUP(_ptr) : NULL)
#define DECLAREUP char *saveNextIn, *saveCurID = NULL;
#define SAVEUP { FREE_IF_NZ(saveCurID); \
	saveCurID = STRDUP_IF_NZ(me->lexer->curID); \
	saveNextIn = me->lexer->nextIn;}
#define BACKUP 	{FREE_IF_NZ(me->lexer->curID); \
	me->lexer->curID = saveCurID; \
	me->lexer->nextIn = saveNextIn; }
#define FREEUP {FREE_IF_NZ(saveCurID);}

static BOOL parser_brotoStatement(struct VRMLParser* me);

void parse_proto_body(struct VRMLParser* me)
{
	DECLAREUP

	/* As long as there are nodes, routes, or protos to be parsed keep doing so */
    while(TRUE)
    {
        /* Try nodeStatement */
		SAVEUP
        {
            vrmlNodeT node;
            /* This will parse a builtin node, including all nested ROUTES and PROTOs, and return
               a pointer to the node that was parsed. 
               If the node is a user-defined node (PROTO expansion) this will expand the PROTO (propagate
               all field values, and add all routes to the CRoute table), and returns a pointer to the
               root node of the scene graph for this PROTO */
#ifdef CPARSERVERBOSE
            printf("parser_vrmlScene: Try node\n");
#endif
            if(parser_nodeStatement(me, &node))
            {
                /* Add the node just parsed to the ROOT node for this scene */
                if (node != NULL) 
                	AddRemoveChildren(me->ptr,  offsetPointer_deref(void *,me->ptr,me->ofs), &node, 1, 1,__FILE__,__LINE__);
#ifdef CPARSERVERBOSE
                printf("parser_vrmlScene: node parsed\n");
#endif
				//printf("pp.children.n=%d\n",pp->children.n);
                continue;
            }
        }
		BACKUP
        /* Try routeStatement */
        /* Checks that the ROUTE statement is valid (i.e. that the referenced node and field combinations
           exist, and that they are compatible) and then adds the route to the CRoutes table of routes. */

#ifdef CPARSERVERBOSE
        printf("parser_vrmlScene: Try route\n");
#endif


        /* try ROUTE, COMPONENT, EXPORT, IMPORT, META, PROFILE statements here */
        BLOCK_STATEMENT(parser_vrmlScene)
		BACKUP
            /* Try protoStatement */
            /* Add the PROTO name to the userNodeTypesVec list of names.  Create and fill in a new protoDefinition structure and add it to the PROTOs list.
               Goes through the interface declarations for the PROTO and adds each user-defined field name to the appropriate list of user-defined names (user_initializeOnly,
               user_inputOnly, Out, or user_inputOutput), creates a new protoFieldDecl for the field and adds it to the iface vector of the ProtoDefinition,
               and, in the case of fields and inputOutputs, gets the default value of the field and stores it in the protoFieldDecl.
               Parses the body of the PROTO.  Nodes are added to the scene graph for this PROTO.  Routes are parsed and a new ProtoRoute structure
               is created for each one and added to the routes vector of the ProtoDefinition.  PROTOs are recursively parsed!
            */
#ifdef CPARSERVERBOSE
            printf("parser_vrmlScene: Try proto\n");
#endif
        if(parser_protoStatement(me)) {
#ifdef CPARSERVERBOSE
            printf("parser_vrmlScene: PROTO parsed\n");
#endif
            continue;
        }
		BACKUP
        if(parser_brotoStatement(me)) {
#ifdef CPARSERVERBOSE
            printf("parser_vrmlScene: BROTO parsed\n");
#endif
            continue;
        }
		
        break;
    }
}

void broto_store_DEF(struct X3D_Proto* proto,struct X3D_Node* node, char *name);
static BOOL parser_node_A(struct VRMLParser* me, vrmlNodeT* ret, int ind);
static BOOL parser_node_B(struct VRMLParser* me, vrmlNodeT* ret, int ind);

static BOOL parser_node(struct VRMLParser* me, vrmlNodeT* node, int ival)
{
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
	if(p->useBrotos)
		return parser_node_B(me,node,ival);
	else
		return parser_node_A(me,node,ival);
}

/* ************************************************************************** */
/* The start symbol */

BOOL parser_vrmlScene_A(struct VRMLParser* me)
{
    /* As long as there are nodes, routes, or protos to be parsed keep doing so */
    while(TRUE)
    {
        /* Try nodeStatement */
        {
            vrmlNodeT node;
            /* This will parse a builtin node, including all nested ROUTES and PROTOs, and return
               a pointer to the node that was parsed. 
               If the node is a user-defined node (PROTO expansion) this will expand the PROTO (propagate
               all field values, and add all routes to the CRoute table), and returns a pointer to the
               root node of the scene graph for this PROTO */
#ifdef CPARSERVERBOSE
            printf("parser_vrmlScene: Try node\n");
#endif
            if(parser_nodeStatement(me, &node))
            {
                /* Add the node just parsed to the ROOT node for this scene */
		if (node != NULL) 
                	AddRemoveChildren(me->ptr,  offsetPointer_deref(void *,me->ptr,me->ofs), &node, 1, 1,__FILE__,__LINE__);
#ifdef CPARSERVERBOSE
                printf("parser_vrmlScene: node parsed\n");
#endif
                continue;
            }
        }

        /* Try routeStatement */
        /* Checks that the ROUTE statement is valid (i.e. that the referenced node and field combinations
           exist, and that they are compatible) and then adds the route to the CRoutes table of routes. */

#ifdef CPARSERVERBOSE
        printf("parser_vrmlScene: Try route\n");
#endif


        /* try ROUTE, COMPONENT, EXPORT, IMPORT, META, PROFILE statements here */
        BLOCK_STATEMENT(parser_vrmlScene)

            /* Try protoStatement */
            /* Add the PROTO name to the userNodeTypesVec list of names.  Create and fill in a new protoDefinition structure and add it to the PROTOs list.
               Goes through the interface declarations for the PROTO and adds each user-defined field name to the appropriate list of user-defined names (user_initializeOnly,
               user_inputOnly, Out, or user_inputOutput), creates a new protoFieldDecl for the field and adds it to the iface vector of the ProtoDefinition,
               and, in the case of fields and inputOutputs, gets the default value of the field and stores it in the protoFieldDecl.
               Parses the body of the PROTO.  Nodes are added to the scene graph for this PROTO.  Routes are parsed and a new ProtoRoute structure
               is created for each one and added to the routes vector of the ProtoDefinition.  PROTOs are recursively parsed!
            */
#ifdef CPARSERVERBOSE
            printf("parser_vrmlScene: Try proto\n");
#endif
        if(parser_protoStatement(me)) {
#ifdef CPARSERVERBOSE
            printf("parser_vrmlScene: PROTO parsed\n");
#endif
            continue;
        }

        break;
    }

    /* We were unable to keep parsing Nodes, ROUTEs, or PROTOs.  Check that this is indeed the end of the file.  If it isn't, 
       there is an error, so we return FALSE. */

    return lexer_eof(me->lexer);
}
/* ************************************************************************** */
/* The start symbol */

BOOL parser_vrmlScene_B(struct VRMLParser* me)
{
	parse_proto_body(me);

    /* We were unable to keep parsing Nodes, ROUTEs, or PROTOs.  Check that this is indeed the end of the file.  If it isn't, 
       there is an error, so we return FALSE. */

    return lexer_eof(me->lexer);
}
BOOL parser_vrmlScene(struct VRMLParser* me)
{
	//ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
	if(usingBrotos() && X3D_NODE(me->ptr)->_nodeType == NODE_Proto || X3D_NODE(me->ptr)->_nodeType == NODE_Inline) //p->useBrotos)
		return parser_vrmlScene_B(me);
	else
		return parser_vrmlScene_A(me);
}
/* ************************************************************************** */
/* Nodes and fields */

/* Parses an interface declaration and adds it to the PROTO or Script definition */
/* Adds the user-defined name to the appropriate user-defined name list (user_initializeOnly, user_inputOutput, user_inputOnly, or Out)
   Creates a protoFieldDecl or scriptFieldDecl structure to hold field data.
   Parses and stores the default value of fields and inputOutputs.
   Adds the protoFieldDecl or scriptFieldDecl to the list of fields in the ProtoDefinition or Script structure. */ 
static BOOL parser_interfaceDeclaration(struct VRMLParser* me, struct ProtoDefinition* proto, struct Shader_Script* script) {
    int mode;
    int type;
    int name;
	int externproto;
    union anyVrml defaultVal;
	DECLAREUP
    struct ProtoFieldDecl* pdecl=NULL;
    struct ProtoFieldDecl* pField=NULL;
    struct ScriptFieldDecl* sdecl=NULL;
    char *startOfField = NULL;
    int startOfFieldLexerLevel = INT_ID_UNDEFINED;


#ifdef CPARSERVERBOSE
    printf ("start of parser_interfaceDeclaration\n");
#endif

	bzero (&defaultVal, sizeof (union anyVrml));

    /* Either PROTO or Script interface! */
    ASSERT((proto || script) && !(proto && script));

    /* lexer_protoFieldMode is #defined as 
	   lexer_specialID(me, r, NULL, PROTOKEYWORDS, PROTOKEYWORDS_COUNT, NULL) */
    /* Looks for the next token in the array PROTOKEYWORDS (inputOnly, outputOnly, inputOutput, field) 
	   and returns the appropriate index in mode */
	SAVEUP
    if(!lexer_protoFieldMode(me->lexer, &mode)) {
#ifdef CPARSERVERBOSE
        printf ("parser_interfaceDeclaration, not lexer_protoFieldMode, returning\n");
#endif
		BACKUP
        return FALSE;
    }

    /* Script can not take inputOutputs */
	if(0) //if(version == VRML2 OR LESS)
    if (script != NULL) {
		if(script->ShaderScriptNode->_nodeType==NODE_Script && mode==PKW_inputOutput)
		{
			PARSE_ERROR("Scripts must not have inputOutputs!")
			//printf("dug9: maybe scripts can have inputOutputs\n");
		}
    }
  
    /* lexer_fieldType is #defined as lexer_specialID(me, r, NULL, FIELDTYPES, FIELDTYPES_COUNT, NULL) */
    /* Looks for the next token in the array FIELDTYPES and returns the index in type */
    if(!lexer_fieldType(me->lexer, &type))
		PARSE_ERROR("Expected fieldType after proto-field keyword!")

#ifdef CPARSERVERBOSE
    printf ("parser_interfaceDeclaration, switching on mode %s\n",PROTOKEYWORDS[mode]);
#endif


    switch(mode)
    {
#define LEX_DEFINE_FIELDID(suff) \
   case PKW_##suff: \
    if(!lexer_define_##suff(me->lexer, &name)) \
     PARSE_ERROR("Expected fieldNameId after field type!") \
    break;

        LEX_DEFINE_FIELDID(initializeOnly)
            LEX_DEFINE_FIELDID(inputOnly)
            LEX_DEFINE_FIELDID(outputOnly)
            LEX_DEFINE_FIELDID(inputOutput)



#ifndef NDEBUG
   default:
        ASSERT(FALSE);
#endif
    }

    /* If we are parsing a PROTO, create a new  protoFieldDecl.
       If we are parsing a Script, create a new scriptFieldDecl. */
	externproto = FALSE;
    if(proto) {
#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, calling newProtoFieldDecl\n");
#endif

		pdecl=newProtoFieldDecl(mode, type, name);
		pdecl->cname = strdup(protoFieldDecl_getStringName(me->lexer, pdecl));
		//pdecl->fieldString = STRDUP(lexer_stringUser_fieldName(me->lexer, name, mode));
		externproto = proto->isExtern;
#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, finished calling newProtoFieldDecl\n");
#endif
    } else {
#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, calling newScriptFieldDecl\n");
#endif
		//lexer_stringUser_fieldName(me,name,mod)
		sdecl=newScriptFieldDecl(me->lexer, mode, type, name);
		//sdecl=newScriptFieldDecl(lexer_stringUser_fieldName(me->lexer,name,mod), mode, type, name);
		//sdecl->fieldString = STRDUP(lexer_stringUser_fieldName(me->lexer, name, mode));

    }

 
    /* If this is a field or an exposed field */ 
    if((mode==PKW_initializeOnly || mode==PKW_inputOutput)  ) { 
#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, mode==PKW_initializeOnly || mode==PKW_inputOutput\n");
#endif
		if(!externproto){

			/* Get the next token(s) from the lexer and store them in defaultVal as the appropriate type. 
				This is the default value for this field.  */
			if (script && lexer_keyword(me->lexer, KW_IS)) {
				int fieldE;
				int fieldO;
				struct ScriptFieldInstanceInfo* sfield;

				/* Find the proto field that this field is mapped to */
				if(!lexer_field(me->lexer, NULL, NULL, &fieldO, &fieldE))
					PARSE_ERROR("Expected fieldId after IS!")

				if(fieldO!=ID_UNDEFINED)
				{
					/* Get the protoFieldDeclaration for the field at index fieldO */
					pField=protoDefinition_getField(me->curPROTO, fieldO, PKW_initializeOnly);
					if(!pField)
						PARSE_ERROR("IS source is no field of current PROTO!")
					ASSERT(pField->mode==PKW_initializeOnly);
				} else {
					/* If the field was found in user_inputOutputs */
					ASSERT(fieldE!=ID_UNDEFINED);
					/* Get the protoFieldDeclaration for the inputOutput at index fieldO */
					pField=protoDefinition_getField(me->curPROTO, fieldE, PKW_inputOutput);
					if(!pField)
						PARSE_ERROR("IS source is no field of current PROTO!")
					ASSERT(pField->mode==PKW_inputOutput);
				}
        
				if (pField) {
					/* Add this scriptfielddecl to the list of script fields mapped to this proto field */
					sfield = newScriptFieldInstanceInfo(sdecl, script);
					vector_pushBack(struct ScriptFieldInstanceInfo*, pField->scriptDests, sfield);
					defaultVal = pField->defaultVal;
				}

			} else {
				/* else proto or script but not KW_IS */
				startOfField = (char *)me->lexer->nextIn;
				startOfFieldLexerLevel = me->lexer->lexerInputLevel;

				/* set the defaultVal to something - we might have a problem if the parser expects this to be
				a MF*, and there is "garbage" in there, as it will expect to free it. */
				bzero (&defaultVal, sizeof (union anyVrml));

				if (!parseType(me, type, &defaultVal)) {
					/* Invalid default value parsed.  Delete the proto or script declaration. */
					CPARSE_ERROR_CURID("Expected default value for field!");
					if(pdecl) deleteProtoFieldDecl(pdecl);
					if(sdecl) deleteScriptFieldDecl(sdecl);
					FREEUP
					return FALSE;
				}
			}
		}
		/* Store the default field value in the protoFieldDeclaration or scriptFieldDecl structure */
		if(proto) {
			pdecl->defaultVal=defaultVal;
		}
		else
		{
			ASSERT(script);
			scriptFieldDecl_setFieldValue(sdecl, defaultVal);
		}
    } else {
#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, NOT mode==PKW_initializeOnly || mode==PKW_inputOutput\n");
#endif

		/* If this is a Script inputOnly/outputOnly IS statement */
		if (script && lexer_keyword(me->lexer, KW_IS)) {
			int evE, evO;
			struct ScriptFieldInstanceInfo* sfield;
			BOOL isIn = FALSE, isOut = FALSE;

#ifdef CPARSERVERBOSE
			printf ("parser_interfaceDeclaration, got IS\n");
#endif
        
			/* Get the inputOnly or outputOnly that this field IS */
			if (mode == PKW_inputOnly) {
				if (lexer_inputOnly(me->lexer, NULL, NULL, NULL, &evO, &evE)) {
					isIn = TRUE;
					isOut = (evE != ID_UNDEFINED);
				}
			} else {
				if (lexer_outputOnly(me->lexer, NULL, NULL, NULL, &evO, &evE)) {
					isOut = TRUE;
				}
			}

			/* Check that the event was found somewhere ... */
			if (!isIn && !isOut)  {
#ifdef CPARSERVERBOSE
				printf ("parser_interfaceDeclaration, NOT isIn Nor isOut\n");
#endif
				FREEUP
				return FALSE;
			}


			/* Get the Proto field definition for the field that this IS */
			pField = protoDefinition_getField(me->curPROTO, evO, isIn ? PKW_inputOnly: PKW_outputOnly); /* can handle inputOnly, outputOnly */
        
			ASSERT(pField);

			/* Add this script as a destination for this proto field */
			sfield = newScriptFieldInstanceInfo(sdecl, script);
			if (pField) vector_pushBack(struct ScriptFieldInstanceInfo*, pField->scriptDests, sfield);
		}
    }

    /* Add the new field declaration to the list of fields in the Proto or Script definition.
       For a PROTO, this means adding it to the iface vector of the ProtoDefinition. 
       For a Script, this means adding it to the fields vector of the ScriptDefinition. */ 
    if(proto) {
		/* protoDefinition_addIfaceField is #defined as vector_pushBack(struct ProtoFieldDecl*, (me)->iface, field) */
		/* Add the protoFieldDecl structure to the iface vector of the protoDefinition structure */

		/* copy the ASCII text over and save it as part of the field */
		if (startOfField != NULL) {
			if (startOfFieldLexerLevel == me->lexer->lexerInputLevel) {

				size_t sz = (size_t) ((me->lexer->nextIn)-startOfField);
				/* printf ("non-recursive PROTO interface copy, string size is %d\n", sz); */

				FREE_IF_NZ(pdecl->fieldString);
				pdecl->fieldString = MALLOC (char *, sz + 2);
				strncpy(pdecl->fieldString,startOfField,sz);
				pdecl->fieldString[sz]='\0';
			} else {
				int i;
				size_t sz;
				char *curStrPtr;

				/* we had a PROTO field come in here; this gets difficult as we have to get different
				   lexer levels, not do simple "from here to here" string math */

				/* this is what happens:
						me->lexerInputLevel ++;
						me->startOfStringPtr[me->lexerInputLevel]=str;
						me->oldNextIn[me->lexerInputLevel] = me->nextIn; 
						me->nextIn=str;
				*/

				/* the "original" level contains a PROTO call; we skip this one, but the
				   following code will show how to get it if you wish 
				sz = (size_t) (me->lexer->oldNextIn[startOfFieldLexerLevel+1] - startOfField);
				printf ("complex recursive copy of PROTO invocation fields\n");
				printf ("for level %d, size is %d\n",startOfFieldLexerLevel, sz);
				*/

				/* we start off with a string of zero length */
				sz = 0;

				/* we go through any intermediate layers */
				for (i=startOfFieldLexerLevel+1; i<me->lexer->lexerInputLevel; i++) {
					printf ("CAUTION: unverified code in recursive PROTO invocations in classic VRML parser\n");
					printf ("level %d\n",i);
					printf ("size of this level, %d\n",(int) (me->lexer->oldNextIn[i+1] - me->lexer->startOfStringPtr[i]));
					sz += (size_t) (me->lexer->oldNextIn[i+1] - me->lexer->startOfStringPtr[i]);

				}
				/* printf ("final level, size %d\n",(int)(me->lexer->nextIn - me->lexer->startOfStringPtr[me->lexer->lexerInputLevel])); */
				sz += (size_t)(me->lexer->nextIn - me->lexer->startOfStringPtr[me->lexer->lexerInputLevel]);

				/* for good luck, and for the trailing null... */
				sz += 2;

				/* now, copy over the "stuff" */

				FREE_IF_NZ(pdecl->fieldString);
				pdecl->fieldString = MALLOC(char *, sz);
				curStrPtr = pdecl->fieldString;

				/* now, copy the actual strings... */
				/* first layer */
				/* if we copy this, we will get the PROTO invocation, so we skip this one
				sz = (size_t) (me->lexer->oldNextIn[startOfFieldLexerLevel+1] - startOfField);
				strncpy(curStrPtr, startOfField, sz);
				curStrPtr += sz;
				curStrPtr[1] = '\0';
				printf ("first layer, we have %d len in copied string\n",strlen(pdecl->fieldString));
				printf ("and, it results in :%s:\n",pdecl->fieldString);
				*/

				/* and, the intermediate layers... */
				for (i=startOfFieldLexerLevel+1; i<me->lexer->lexerInputLevel; i++) {
					sz = (size_t) (me->lexer->oldNextIn[i+1] - me->lexer->startOfStringPtr[i]);
					strncpy(curStrPtr,me->lexer->startOfStringPtr[i],sz);
					curStrPtr += sz;
				}

				/* and the final level */
				sz = (size_t)(me->lexer->nextIn - me->lexer->startOfStringPtr[me->lexer->lexerInputLevel]);
				strncpy(curStrPtr,me->lexer->startOfStringPtr[me->lexer->lexerInputLevel],sz);
				curStrPtr += sz;

				/* trailing null */
				//curStrPtr ++; dug9 dec6,2012 I found this was letting 1 char of junk into the string
				*curStrPtr = '\0';
			}
		}
		#ifdef CPARSERVERBOSE
		printf ("pdecl->fieldString is :%s:\n",pdecl->fieldString);
		#endif

		protoDefinition_addIfaceField(proto, pdecl);
	} else {
        /* Add the scriptFieldDecl structure to the fields vector of the Script structure */
        ASSERT(script);
        script_addField(script, sdecl);
    }

	#ifdef CPARSERVERBOSE
	printf ("end of parser_interfaceDeclaration\n");
	#endif
	FREEUP
    return TRUE;
}

//#include "broto2.h"


/* Parses a protoStatement */
/* Adds the PROTO name to the userNodeTypesVec list of names.  
   Creates a new protoDefinition structure and adds it to the PROTOs list.
   Goes through the interface declarations for the PROTO and adds each user-defined field name to the appropriate list of user-defined names (user_initializeOnly, 
   user_inputOnly, user_outputOnly, or user_inputOutput), creates a new protoFieldDecl for the field and adds it to the iface vector of the ProtoDefinition, 
   and, in the case of fields and inputOutputs, gets the default value of the field and stores it in the protoFieldDecl. 
   Parses the body of the PROTO.  Nodes are added to the scene graph for this PROTO.  Routes are parsed and a new ProtoRoute structure
   is created for each one and added to the routes vector of the ProtoDefinition.  PROTOs are recursively parsed!
*/ 
static BOOL parser_protoStatement(struct VRMLParser* me)
{
    int name;
    struct ProtoDefinition* obj;
    char *startOfBody;
    char *endOfBody;
    char *initCP;
	DECLAREUP
    uintptr_t bodyLen;

    /* Really a PROTO? */
	SAVEUP
    if(!lexer_keyword(me->lexer, KW_PROTO))
	{
		BACKUP
        return FALSE;
	}

    /* Our name */
    /* lexer_defineNodeType is #defined as lexer_defineID(me, ret, userNodeTypesVec, FALSE) */
    /* Add the PROTO name to the userNodeTypesVec list of names return the index of the name in the list in name */ 
    if(!lexer_defineNodeType(me->lexer, &name))
        PARSE_ERROR("Expected nodeTypeId after PROTO!\n")
            ASSERT(name!=ID_UNDEFINED);

    /* Create a new blank ProtoDefinition structure to contain the data for this PROTO */
    obj=newProtoDefinition();

    /* save the name, if we can get it - it will be the last name on the list, because we will have JUST parsed it. */
    if (vectorSize(me->lexer->userNodeTypesVec) != ID_UNDEFINED) {
	obj->protoName = STRDUP(vector_get(const char*, me->lexer->userNodeTypesVec, vectorSize(me->lexer->userNodeTypesVec)-1));
    } else {
	printf ("warning - have proto but no name, so just copying a default string in\n");
	obj->protoName = STRDUP("noProtoNameDefined");
    }

	#ifdef CPARSERVERBOSE
	printf ("parser_protoStatement, working on proto :%s:\n",obj->protoName);
	#endif

    /* If the PROTOs stack has not yet been created, create it */
    if(!me->PROTOs) {
        parser_scopeIn_PROTO(me);
    }

    ASSERT(me->PROTOs);
    /*  ASSERT(name==vectorSize(me->PROTOs)); */

    /* Add the empty ProtoDefinition structure we just created onto the PROTOs stack */
    vector_pushBack(struct ProtoDefinition*, me->PROTOs, obj);
 
    /* Now we want to fill in the information in the ProtoDefinition */

    /* Interface declarations */

    /* Make sure that the next token is a '['.  Skip over it. */
    if(!lexer_openSquare(me->lexer))
        PARSE_ERROR("Expected [ to start interface declaration!")

            /* Read the next line and parse it as an interface declaration. */
            /* Add the user-defined field name to the appropriate list of user-defined names (user_initializeOnly, user_inputOnly, Out, or user_inputOutput).
               Create a new protoFieldDecl for this field and add it to the iface vector for the ProtoDefinition obj.
               For fields and inputOutputs, get the default value of the field and store it in the protoFieldDecl. */
            while(parser_interfaceDeclaration(me, obj, NULL));

    /* Make sure that the next token is a ']'.  Skip over it. */
    if(!lexer_closeSquare(me->lexer))
        PARSE_ERROR("Expected ] after interface declaration!")

            /* PROTO body */
            /* Make sure that the next oken is a '{'.  Skip over it. */
            if(!lexer_openCurly(me->lexer))
                PARSE_ERROR("Expected { to start PROTO body!")

     /* record the start of this proto body - keep the text around */
    startOfBody = (char *) me->lexer->nextIn;
    initCP = (char *) me->lexer->startOfStringPtr[me->lexer->lexerInputLevel];

    /* Create a new vector of nodes and push it onto the DEFedNodes stack */
    /* This is the list of nodes defined for this scope */
    /* Also checks that the PROTOs vector exists, and creates it if it does not */
    parser_scopeIn(me);

    /* Parse body */
    {
        /* Store the previous currentProto for future reference, and set the PROTO we are now parsing to the currentProto */
        struct ProtoDefinition* oldCurPROTO=me->curPROTO;
        char *protoBody;

#ifdef CPARSERVERBOSE
        printf ("about to parse PROTO body; curPROTO = %u, new proto def %u\n",oldCurPROTO,obj);
#endif

        me->curPROTO=obj;

        /* just go to the end of the proto body */
        skipToEndOfOpenCurly(me->lexer,0);

        /* end of the proto body */
        /* printf ("parsing proto , obj %u me->curPROTO %u ocp %u\n",obj, me->curPROTO,oldCurPROTO); */
        endOfBody = (char *) me->lexer->nextIn;
        
        /* has a proto expansion come through here yet? */
        if (me->lexer->startOfStringPtr[me->lexer->lexerInputLevel] != initCP) { 
#ifdef CPARSERVERBOSE
#endif
            printf ("parsing proto, body changed!\n");

            startOfBody = (char *) me->lexer->startOfStringPtr[me->lexer->lexerInputLevel]; }

        bodyLen = endOfBody - startOfBody;
#ifdef CPARSERVERBOSE
        printf ("PROTO has a body of %u len\n",bodyLen);
#endif

        /* copy this proto body */
        protoBody = MALLOC (char *, bodyLen+10);
        strncpy (protoBody, startOfBody, bodyLen);

        if (bodyLen>0) {
            /* printf ("copied this PROTO, but lets see if we need to remove a curly brace... :%c:\n",
               protoBody[bodyLen-1]); */
            if (protoBody[bodyLen-1] == '}') {
                bodyLen--;
            }
        }
        protoBody[bodyLen] = '\0';

#ifdef CPARSERVERBOSE
        printf ("*** proto body for proto %u is :%s:\n",me, protoBody); 
#endif

        tokenizeProtoBody(me->curPROTO, protoBody);
        /* FREE_IF_NZ(protoBody); */

        /* We are done parsing this proto.  Set the curPROTO to the last proto we were parsing. */
        me->curPROTO=oldCurPROTO;
    }

    /* Takes the top DEFedNodes vector off of the stack.  The local scope now refers to the next vector in the DEFedNodes stack */
    parser_scopeOut(me);

    /* Make sure that the next token is a '}'.  Skip over it. */
#ifdef CPARSERVERBOSE
    printf ("calling lexer_closeCurly at A\n");
printf ("parser_protoStatement, FINISHED proto :%s:\n",obj->protoName);
#endif
	FREEUP
    return TRUE;
}

//#include "broto3.h"


static BOOL parser_componentStatement(struct VRMLParser* me) {
    int myComponent = INT_ID_UNDEFINED;
    int myLevel = INT_ID_UNDEFINED;

#if DJ_KEEP_COMPILER_WARNING
#define COMPSTRINGSIZE 20
#endif

    ASSERT(me->lexer);
    lexer_skip(me->lexer);

    /* Is this a COMPONENT statement? */
    if(!lexer_keyword(me->lexer, KW_COMPONENT)) return FALSE;

#ifdef CPARSERVERBOSE
    printf ("parser_componentStatement...\n");
#endif

    if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this IS a COMPONENT statement */
    ASSERT(me->lexer->curID);

    myComponent = findFieldInCOMPONENTS(me->lexer->curID);
    if (myComponent == ID_UNDEFINED) {
        CPARSE_ERROR_CURID("Invalid COMPONENT name ");
        return TRUE;
    }
                
#ifdef CPARSERVERBOSE
    printf ("my COMPONENT is %s\n",COMPONENTS[myComponent]);
#endif

    /* now, we are finished with this COMPONENT */
    FREE_IF_NZ(me->lexer->curID);

    /* we should have a ":" then an integer supportLevel */
    if (!lexer_colon(me->lexer)) {
        CPARSE_ERROR_CURID("expected colon in COMPONENT statement")
            return TRUE;
    }

    if (!parser_sfint32Value(me,&myLevel)) {
        CPARSE_ERROR_CURID("expected supportLevel")
            return TRUE;
    }   
    handleComponent(myComponent,myLevel);

    return TRUE;
}


void handleExport (char *node, char *as) {
	/* handle export statements. as will be either a string pointer, or NULL */
	
	#ifdef CAPABILITIESVERBOSE
	printf ("handleExport: node :%s: ",node);
	if (as != NULL) printf (" AS :%s: ",node);
	printf ("\n");
	#endif
}

struct X3D_Context {
	int protoFlags;
	void * DEFnames;
	void * IS;
	void * ROUTES;
	void * externProtoDeclares;
	void * protoDeclares;
	void * IMPORTS;
	void * EXPORTS;
	void * scripts;
	struct X3D_Context* parentContext;
	//struct X3D_Node *__parentProto;
};


struct X3D_Context *hasContext(struct X3D_Node* node){

	struct  X3D_Context * context = NULL;
	/*
	if(node)
		switch(node->_nodeType){
			case NODE_Group:
				context = offsetPointer_deref(void*, node,  offsetof(struct X3D_Group,__context));
				break;
			case NODE_Transform:
				context = offsetPointer_deref(void*, node,  offsetof(struct X3D_Transform,__context));
				break;
			case NODE_Proto:
				context = offsetPointer_deref(void*, node,  offsetof(struct X3D_Proto,__context));
				break;
			case NODE_Inline:  //Q. do I need this in here? Saw code in x3dparser.
				context = offsetPointer_deref(void*, node,  offsetof(struct X3D_Inline,__context));
				break;
			case NODE_GeoLOD:  //Q. do I need this in here?
				context = offsetPointer_deref(void*, node, offsetof(struct X3D_GeoLOD,__context));
				break;
		}
	*/
	return context;
}

void handleExport_B (void *nodeptr, char *node, char *as) {
	/* handle export statements. as will be either a string pointer, or NULL */
	if(usingBrotos() && nodeptr && hasContext(nodeptr)){
		struct X3D_Context *context = hasContext(nodeptr);
		//context-> hasContext(nodeptr);
	}
	#ifdef CAPABILITIESVERBOSE
	printf ("handleExport: node :%s: ",node);
	if (as != NULL) printf (" AS :%s: ",node);
	printf ("\n");
	#endif
}


void handleImport (char *nodeName,char *nodeImport, char *as) {
	/* handle Import statements. as will be either a string pointer, or NULL */
	
	#ifdef CAPABILITIESVERBOSE
	printf ("handleImport: inlineNodeName :%s: nodeToImport :%s:",nodeName, nodeImport);
	if (as != NULL) printf (" AS :%s: ",as);
	printf ("\n");
	#endif
}

static BOOL parser_exportStatement(struct VRMLParser* me) {
    char *nodeToExport = NULL;
    char *alias = NULL; 

    ASSERT(me->lexer);
    lexer_skip(me->lexer);

    /* Is this a EXPORT statement? */
    if(!lexer_keyword(me->lexer, KW_EXPORT)) return FALSE;

#ifdef CPARSERVERBOSE
    printf ("parser_exportStatement...\n");
#endif

    if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this IS an EXPORT statement... */
    ASSERT(me->lexer->curID);

    /* save this, and find the next token... */
    nodeToExport = me->lexer->curID;
    me->lexer->curID = NULL;

    if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this Is an EXPORT statement...*/
    ASSERT(me->lexer->curID);

    /* do we have an "AS" statement? */
    if (strcmp("AS",me->lexer->curID) == 0) {
        FREE_IF_NZ(me->lexer->curID);
        if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this Is an EXPORT statement...*/
        ASSERT(me->lexer->curID);
        alias = me->lexer->curID;
    }

    /* do the EXPORT */
	if(usingBrotos())
		handleExport_B(me->ptr,nodeToExport, alias);
	else
		handleExport(nodeToExport, alias);

    /* free things up, only as required */
    FREE_IF_NZ(nodeToExport);
    if (alias != NULL) {FREE_IF_NZ(me->lexer->curID);}
    return TRUE;
}

static BOOL parser_importStatement(struct VRMLParser* me) {
    char *inlineNodeName = NULL;
    char *alias = NULL; 
    char *nodeToImport = NULL;

    ASSERT(me->lexer);
    lexer_skip(me->lexer);

    /* Is this a IMPORT statement? */
    if(!lexer_keyword(me->lexer, KW_IMPORT)) return FALSE;

#ifdef CPARSERVERBOSE
    printf ("parser_importStatement...\n");
#endif

    if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this IS an IMPORT statement... */
    ASSERT(me->lexer->curID);

    /* save this, and find the next token... */
    inlineNodeName = STRDUP(me->lexer->curID);
    FREE_IF_NZ(me->lexer->curID);

    /* we should have a "." then an integer supportLevel */
    if (!lexer_point(me->lexer)) {
        CPARSE_ERROR_CURID("expected period in IMPORT statement")
            return TRUE;
    }

    if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this IS an IMPORT statement... */
    ASSERT(me->lexer->curID);

    /* ok, now, we should have the nodeToImport name... */
    nodeToImport = STRDUP(me->lexer->curID);
    FREE_IF_NZ(me->lexer->curID);

    /* get the next token */
    if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this Is an IMPORT statement...*/
    ASSERT(me->lexer->curID);

    /* do we have an "AS" statement? */
    if (strcmp("AS",me->lexer->curID) == 0) {
        FREE_IF_NZ(me->lexer->curID);
        if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this Is an IMPORT statement...*/
        ASSERT(me->lexer->curID);
        alias = STRDUP(me->lexer->curID);
        FREE_IF_NZ(me->lexer->curID);
    }

    /* do the IMPORT */
    handleImport(inlineNodeName, nodeToImport, alias);

    FREE_IF_NZ (inlineNodeName);
    FREE_IF_NZ (nodeToImport);
    FREE_IF_NZ (alias);
    return TRUE;
}
static BOOL parser_metaStatement(struct VRMLParser* me) {
    vrmlStringT val1, val2;

    ASSERT(me->lexer);
    lexer_skip(me->lexer);

    /* Is this a META statement? */
    if(!lexer_keyword(me->lexer, KW_META)) return FALSE;

#ifdef CPARSERVERBOSE
    printf ("parser_metaStatement...\n");
#endif

    /* META lines have 2 strings */

    /* Otherwise, a real vector */
    val1=NULL; val2 = NULL;

    if(!parser_sfstringValue (me, &val1)) {
        CPARSE_ERROR_CURID("Expected a string after a META keyword")
            }

    if(!parser_sfstringValue (me, &val2)) {
        CPARSE_ERROR_CURID("Expected a string after a META keyword")
            }

    if ((val1 != NULL) && (val2 != NULL)) { handleMetaDataStringString(val1,val2); }

    /* cleanup */
    if (val1 != NULL) {FREE_IF_NZ(val1->strptr); FREE_IF_NZ(val1);}
    if (val2 != NULL) {FREE_IF_NZ(val2->strptr); FREE_IF_NZ(val2);}
    return TRUE;
}

static BOOL parser_profileStatement(struct VRMLParser* me) {
    int myProfile = INT_ID_UNDEFINED;

    ASSERT(me->lexer);
    lexer_skip(me->lexer);

    /* Is this a PROFILE statement? */
    if(!lexer_keyword(me->lexer, KW_PROFILE)) return FALSE;

#ifdef CPARSERVERBOSE
    printf ("parser_profileStatement...\n");
#endif

    if(!lexer_setCurID(me->lexer)) return TRUE; /* true, because this IS an PROFILE statement... */
    ASSERT(me->lexer->curID);

    myProfile = findFieldInPROFILES(me->lexer->curID);

    if (myProfile != ID_UNDEFINED) {
        handleProfile(myProfile);
    } else {
        CPARSE_ERROR_CURID("Expected a profile after a PROFILE keyword")
            return TRUE;
    }
                
    /* XXX FIXME - do something with the Profile statement */
#ifdef CPARSERVERBOSE
    printf ("my profile is %d\n",myProfile);
#endif
    /* now, we are finished with this PROFILE */
    FREE_IF_NZ(me->lexer->curID);
    return TRUE;
}
//#include "broto4.h"



/* Parses a routeStatement */
/* Checks that the ROUTE statement is valid (i.e. that the referenced node and field combinations  
   exist, and that they are compatible) and then adds the route to either the CRoutes table of routes, or adds a new ProtoRoute structure to the vector 
   ProtoDefinition->routes if we are parsing a PROTO */

static BOOL parser_routeStatement_A(struct VRMLParser* me)
{
    int fromNodeIndex;
    struct X3D_Node* fromNode;
    struct ProtoDefinition* fromProto;
    int fromFieldO;
    int fromFieldE;
    int fromUFieldO;
    int fromUFieldE;
    int fromOfs;
    int fromType;
    struct ScriptFieldDecl* fromScriptField;

    int toNodeIndex;
    struct X3D_Node* toNode;
    struct ProtoDefinition* toProto;
    int toFieldO;
    int toFieldE;
    int toUFieldO;
    int toUFieldE;
    int toOfs;
    int toType;
    struct ScriptFieldDecl* toScriptField;
    int temp, tempFE, tempFO, tempTE, tempTO;
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;

    fromOfs = 0;
    fromType = 0;
   toOfs = 0;
   toType = 0;
    fromFieldE = ID_UNDEFINED; fromFieldO = ID_UNDEFINED; toFieldE = ID_UNDEFINED; toFieldO = ID_UNDEFINED;
    toNode = NULL; fromNode = NULL; toProto=NULL; fromProto=NULL;
    fromScriptField=NULL; toScriptField=NULL;

    ASSERT(me->lexer);
    lexer_skip(me->lexer);

    /* Is this a routeStatement? */
    if(!lexer_keyword(me->lexer, KW_ROUTE))
        return FALSE;

    /* Parse the elements. */
#define ROUTE_PARSE_NODEFIELD(pre, eventType) \
  /* Target-node */ \
\
  /* Look for the current token in the userNodeNames vector (DEFed names) */ \
  if(!lexer_nodeName(me->lexer, &pre##NodeIndex)) { \
       /* The current token is not a valid DEFed name.  Error. */ \
        CPARSE_ERROR_CURID("ERROR:ROUTE: Expected a valid DEF name; found \""); \
        PARSER_FINALLY;  \
        return FALSE; \
  } \
  /* Check that there are DEFedNodes in the DEFedNodes vector, and that the index given for this node is valid */ \
  ASSERT(me->DEFedNodes && !stack_empty(me->DEFedNodes) && \
   pre##NodeIndex<vectorSize(stack_top(struct Vector*, me->DEFedNodes))); \
  /* Get the X3D_Node structure for the DEFed node we just looked up in the userNodeNames list */ \
  pre##Node=vector_get(struct X3D_Node*, \
   stack_top(struct Vector*, me->DEFedNodes), \
   pre##NodeIndex); \
  /* We were'nt able to get the X3D_Node structure for the DEFed node.  Error. */ \
  if (pre##Node == NULL) { \
        /* we had a bracket underflow, from what I can see. JAS */ \
        CPARSE_ERROR_CURID("ERROR:ROUTE: no DEF name found - check scoping and \"}\"s"); \
        PARSER_FINALLY;  \
        return FALSE; \
 } \
 \
  /* PROTO? */ \
  /* We got the node structure for the DEFed node. If this is a Group or a Script node do some more processing */ \
        switch(pre##Node->_nodeType) \
        { \
         case NODE_Group: { \
          /* Get a pointer to the protoDefinition for this group node */ \
          pre##Proto=getVRMLprotoDefinition(X3D_GROUP(pre##Node)); \
          /* printf ("routing found protoGroup of %u\n",pre##Proto); */ \
	  } \
          break; \
        } \
  \
  \
  /* The next character has to be a '.' - skip over it */ \
  if(!lexer_point(me->lexer)) {\
        CPARSE_ERROR_CURID("ERROR:ROUTE: Expected \".\" after the NODE name") \
        PARSER_FINALLY;  \
        return FALSE;  \
  } \
  /* if this is a PROTO, then lets look through the tokenizedProtoBody for the REAL NODE.FIELD! */ \
  if (pre##Proto) { \
        lexer_setCurID(me->lexer); \
        /* printf ("have a proto, field is %s have to resolve this to a real node.field \n",me->lexer->curID); */ \
        if (!resolveProtoNodeField(me, pre##Proto, me->lexer->curID, &pre##Node)) return FALSE; \
	/* we KNOW that the field name is going to be "value" - see PROTO code, especially return from protoExpand() */ \
	FREE_IF_NZ(me->lexer->curID); \
	if (KW_##eventType == KW_outputOnly) me->lexer->curID = STRDUP("valueChanged"); \
	else me->lexer->curID = STRDUP("setValue"); \
	/* printf ("PROTO ROUTING TO FIELD %s in CPARSER\n",me->lexer->curID); */ \
        pre##Proto = NULL; /* forget about this being a PROTO because PROTO is expanded */ \
  } \
  \
  /* Field, user/built-in depending on whether node is a Script instance */ \
	switch (pre##Node->_nodeType) { \
		case NODE_Script: \
		case NODE_ComposedShader: \
		case NODE_ShaderProgram : \
		case NODE_PackagedShader: \
   			/* SCRIPT - This is a user defined node type */ \
   			/* Look for the next token (field of the DEFed node) in the user_inputOnly/user_outputOnly, user_inputOutput arrays */ \
   			if(lexer_##eventType(me->lexer, pre##Node, NULL, NULL, &pre##UFieldO, &pre##UFieldE)) { \
    				if(pre##UFieldO!=ID_UNDEFINED) { \
     					/* We found the event in user_inputOnly or Out */ \
      					/* If this is a Script get the scriptFieldDecl for this event */ \
					switch (pre##Node->_nodeType) { \
      					case NODE_Script: pre##ScriptField=script_getField((struct Shader_Script*) (X3D_SCRIPT(pre##Node)->__scriptObj), pre##UFieldO, PKW_##eventType); break;\
      					case NODE_ComposedShader: pre##ScriptField=script_getField((struct Shader_Script *)(X3D_COMPOSEDSHADER(pre##Node)->_shaderUserDefinedFields), pre##UFieldO, PKW_##eventType); break;\
      					case NODE_ShaderProgram: pre##ScriptField=script_getField((struct Shader_Script *)(X3D_SHADERPROGRAM(pre##Node)->_shaderUserDefinedFields), pre##UFieldO, PKW_##eventType); break;\
      					case NODE_PackagedShader: pre##ScriptField=script_getField((struct Shader_Script *)(X3D_PACKAGEDSHADER(pre##Node)->_shaderUserDefinedFields), pre##UFieldO, PKW_##eventType); break;\
					} \
    				} else { \
    	 				/* We found the event in user_inputOutput */ \
      					/* If this is a Script get the scriptFieldDecl for this event */ \
					switch (pre##Node->_nodeType) { \
      					case NODE_Script: pre##ScriptField=script_getField((struct Shader_Script *)(X3D_SCRIPT(pre##Node)->__scriptObj), pre##UFieldE, PKW_inputOutput); break; \
      					case NODE_ComposedShader: pre##ScriptField=script_getField((struct Shader_Script *)(X3D_COMPOSEDSHADER(pre##Node)->_shaderUserDefinedFields), pre##UFieldE, PKW_inputOutput); break; \
      					case NODE_ShaderProgram: pre##ScriptField=script_getField((struct Shader_Script *)(X3D_SHADERPROGRAM(pre##Node)->_shaderUserDefinedFields), pre##UFieldE, PKW_inputOutput); break; \
      					case NODE_PackagedShader: pre##ScriptField=script_getField((struct Shader_Script *)(X3D_PACKAGEDSHADER(pre##Node)->_shaderUserDefinedFields), pre##UFieldE, PKW_inputOutput); break; \
					} \
    				} \
    				if(pre##Node->_nodeType==NODE_Script && !pre##ScriptField) \
     					PARSE_ERROR("Event-field invalid for this PROTO/Script!") \
   			} else { \
    				PARSE_ERROR("Expected an event of type :" #eventType ": ") \
  			} \
   			ASSERT(pre##ScriptField); \
   			pre##Ofs=scriptFieldDecl_getRoutingOffset(pre##ScriptField); \
		break; \
\
		default: \
\
   		/* This is a builtin DEFed node.  */ \
   		/* Look for the next token (field of the DEFed node) in the builtin EVENT_IN/EVENT_OUT or EXPOSED_FIELD arrays */ \
   		if(!lexer_##eventType(me->lexer, pre##Node, &pre##FieldO, &pre##FieldE, NULL, NULL))  {\
        		/* event not found in any built in array.  Error. */ \
        		CPARSE_ERROR_CURID("ERROR:Expected built-in event " #eventType " after .") \
        		PARSER_FINALLY;  \
        		return FALSE;  \
    		} \
	} /* end of case */ 




    /* Parse the first part of a routing statement: DEFEDNODE.event by locating the node DEFEDNODE in either the builtin or user-defined name arrays
       and locating the event in the builtin or user-defined event name arrays */
    ROUTE_PARSE_NODEFIELD(from, outputOnly);
      //   printf ("ROUTE_PARSE_NODEFIELD, found fromFieldE %d fromFieldO %d\n",fromFieldE, fromFieldO);
        /*   printf ("fromNode %u fromProto %u fromScript %u fromNodeIndex %d\n",fromNode, fromScriptField, fromProto, fromNodeIndex); */

        /* Next token has to be "TO" */
        if(!lexer_keyword(me->lexer, KW_TO)) {
            /* try to make a better error message. */
            strcpy (p->fw_outline,"ERROR:ROUTE: Expected \"TO\" found \"");
            if (me->lexer->curID != NULL) strcat (p->fw_outline, me->lexer->curID); else strcat (p->fw_outline, "(EOF)");
            strcat (p->fw_outline,"\" ");
            if (fromNode != NULL) { strcat (p->fw_outline, " from type:"); strcat (p->fw_outline, stringNodeType(fromNode->_nodeType)); strcat (p->fw_outline, " "); }
            if (fromFieldE != ID_UNDEFINED) { strcat (p->fw_outline, ":"); strcat (p->fw_outline, EXPOSED_FIELD[fromFieldE]); strcat (p->fw_outline, " "); }
            if (fromFieldO != ID_UNDEFINED) { strcat (p->fw_outline, ":"); strcat (p->fw_outline, EVENT_OUT[fromFieldO]); strcat (p->fw_outline, " "); }

            /* PARSE_ERROR("Expected TO in ROUTE-statement!") */
            CPARSE_ERROR_CURID(p->fw_outline); 
            PARSER_FINALLY; 
	    return FALSE; 
        }
/* Parse the second part of a routing statement: DEFEDNODE.event by locating the node DEFEDNODE in either the builtin or user-defined name arrays 
   and locating the event in the builtin or user-defined event name arrays */
	ROUTE_PARSE_NODEFIELD(to, inputOnly);

	 //printf ("ROUTE_PARSE_NODEFIELD, found toFieldE %d toFieldO %d\n",toFieldE, toFieldO);
		/* printf ("toNode %u toProto %u toScript %u toNodeIndex %d\n",toNode, toScriptField, toProto, toNodeIndex); */

        /* Now, do the really hard macro work... */
        /* ************************************* */

        /* Ignore the fields. */
#define FIELD(n, f, t, v, realType)

#ifdef OLDCODE
OLDCODE #define END_NODE(n) EVENT_END_NODE(n,XFIELD[fromFieldE])
#endif //OLDCODE

        /* potentially rename the Event_From and Event_To */

        /* REASON: we had a problem with (eg) set_scale in tests/27.wrl. 
           set_scale is a valid field for an Extrusion, so, it was "found". Unfortunately,
           set_scale should be changed to "scale" for an event into a Transform...

           The following code attempts to do this transformation. John Stewart - March 22 2007 */

        /* BTW - Daniel, YES this is SLOW!! It does a lot of string comparisons. But, it does
           seem to work, and it is only SLOW during ROUTE parsing, which (hopefully) there are not
           thousands of. Code efficiency changes more than welcome, from anyone. ;-) */

        tempFE=ID_UNDEFINED; tempFO=ID_UNDEFINED; tempTE=ID_UNDEFINED; tempTO=ID_UNDEFINED;

#ifdef CPARSERVERBOSE
        printf ("fromScriptField %d fromFieldE %d fromFieldO %d\n",fromScriptField,fromFieldE,fromFieldO);
        printf ("toScriptField %d toFieldE %d toFieldO %d\n",toScriptField,toFieldE,toFieldO);
#endif

        if(!fromScriptField) {
            /* fromFieldE = Daniel's code thinks this is from an inputOutput */
            /* fromFieldO = Daniel's code thinks this is from an outputOnly */

            /* If the from event was found in the EVENT_OUT array */
            /* if we have an EVENT_OUT, we have to ensure that the inputOutput is NOT used (eg, IndexedFaceSet has set_x and x fields */
            if (fromFieldO != ID_UNDEFINED) {
#ifdef CPARSERVERBOSE
                printf ("step1a, node type %s fromFieldO %d %s\n",stringNodeType(fromNode->_nodeType),fromFieldO,EVENT_OUT[fromFieldO]);
#endif

                /* ensure that the inputOutput is NOT going to be used */
                fromFieldE = ID_UNDEFINED;

                /* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
                tempFO = findRoutedFieldInFIELDNAMES(fromNode,EVENT_OUT[fromFieldO],0);

                /* Field was found in FIELDNAMES, and this is a valid field?  Then find it in EVENT_OUT */
                if (tempFO != ID_UNDEFINED) {
                    tempFO = findFieldInEVENT_OUT(FIELDNAMES[tempFO]);
                }  

                /* We didn't find the event in EVENT_OUT?  Look for it in EXPOSED_FIELD */
                if (tempFO == ID_UNDEFINED) {

                    /* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
                    temp = findRoutedFieldInFIELDNAMES(fromNode,EVENT_OUT[fromFieldO],0);

                    /* Field found in FIELDNAMES and it is a valid field?  Then find it in EXPOSED_FIELD */
                    if (temp != ID_UNDEFINED) tempFE = findFieldInEXPOSED_FIELD(FIELDNAMES[temp]);
                }
            } else if(fromFieldE!=ID_UNDEFINED) {
                /* If the field was found in the EXPOSED_FIELDS array */
#ifdef CPARSERVERBOSE
                printf ("step2a, node type %s fromFieldE %d %s\n",stringNodeType(fromNode->_nodeType),fromFieldE,EXPOSED_FIELD[fromFieldE]);
#endif

                /* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
                tempFE = findRoutedFieldInFIELDNAMES(fromNode,EXPOSED_FIELD[fromFieldE],0);

                /* Field was found in FIELDNAMES, and this is a valid field?  Then, try to find the index of the event in EXPOSEDFIELD */
                if (tempFE != ID_UNDEFINED) tempFE = findFieldInEXPOSED_FIELD(FIELDNAMES[tempFE]);
            }
        }


        if(!toScriptField) {
            /* toFieldE = Daniel's code thinks this is from an inputOutput */
            /* toFieldO = Daniel's code thinks this is from an inputOnly */

            /* If the from event was found in the EVENT_IN array */
            /* if both are defined, use the inputOnly - eg, IndexedFaceSet might have set_coordIndex and coordIndex */
            if(toFieldO!=ID_UNDEFINED) {
#ifdef CPARSERVERBOSE
                printf ("step3a, node type %s toFieldO %d %s\n",stringNodeType(toNode->_nodeType),toFieldO,EVENT_IN[toFieldO]);
#endif
                
                /* ensure that we will NOT use the inputOutput, if one exists... */
                toFieldE = ID_UNDEFINED;

                /* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
                tempTO = findRoutedFieldInFIELDNAMES(toNode,EVENT_IN[toFieldO],1);

                /* Field was found in FIELDNAMES, and this is a valid field?  Then find it in EVENT_IN */
                if (tempTO != ID_UNDEFINED) {
                    tempTO = findFieldInEVENT_IN(FIELDNAMES[tempTO]);
                }  

                /* We didn't find the event in EVENT_IN?  Look for it in EXPOSED_FIELD */
                if (tempTO == ID_UNDEFINED) {
                    /* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
                    temp = findRoutedFieldInFIELDNAMES(toNode,EVENT_IN[toFieldO],1);

                    /* Field found in FIELDNAMES and it is a valid field?  Then find it in EXPOSED_FIELD */
                    if (temp != ID_UNDEFINED) tempTE = findFieldInEXPOSED_FIELD(FIELDNAMES[temp]);
                }
            }
            /* If the field was found in the EXPOSED_FIELDS array */
            else if(toFieldE!=ID_UNDEFINED) {
#ifdef CPARSERVERBOSE
                printf ("step4a, node type %s toFieldE %d %s\n",stringNodeType(toNode->_nodeType),toFieldE,EXPOSED_FIELD[toFieldE]);
#endif

                /* Try to find the index of the event in FIELDNAMES, and check that this is a valid event for the node fromNode */
                tempTE = findRoutedFieldInFIELDNAMES(toNode,EXPOSED_FIELD[toFieldE],1);

                /* Field was found in FIELDNAMES, and this is a valid field?  Then, try to find the index of the event in EXPOSEDFIELD */
                if (tempTE != ID_UNDEFINED) tempTE = findFieldInEXPOSED_FIELD(FIELDNAMES[tempTE]);
            }


        }
#ifdef CPARSERVERBOSE
        printf ("so, before routing we have: ");
        if (tempFE != ID_UNDEFINED) {printf ("from EXPOSED_FIELD %s ",EXPOSED_FIELD[tempFE]);}
        if (tempFO != ID_UNDEFINED) {printf ("from EVENT_OUT %s ",EVENT_OUT[tempFO]);}
        if (tempTE != ID_UNDEFINED) {printf ("to EXPOSED_FIELD %s ",EXPOSED_FIELD[tempTE]);}
        if (tempTO != ID_UNDEFINED) {printf ("to EVENT_IN %s ",EVENT_IN[tempTO]);}
        printf ("\n\n");
#endif

        /* so, lets try and assign what we think we have now... */
        fromFieldE = tempFE;
        fromFieldO = tempFO;
        toFieldE = tempTE;
        toFieldO = tempTO;

#ifdef OLDCODE
OLDCODE #undef END_NODE 
#endif //OLDCODE

#define END_NODE(n) EVENT_END_NODE(n,EXPOSED_FIELD[fromFieldE])

        /* Process from outputOnly */
        /* Get the offset to the outputOnly in the fromNode and store it in fromOfs */
        /* Get the type of the outputOnly type and store it in fromType */
        if(!fromScriptField) {
            /* If the from field is an exposed field */
            if(fromFieldE!=ID_UNDEFINED) {
                /* Get the offset and size of this field in the fromNode */
                switch(fromNode->_nodeType) {
#define EVENT_IN(n, f, t, v, realType)
#define EVENT_OUT(n, f, t, v, realType)
#define EXPOSED_FIELD(node, field, type, var, realType) \
     PROCESS_EVENT(EXPOSED_FIELD, from, node, field, type, var, realType)
#define BEGIN_NODE(node) \
     EVENT_BEGIN_NODE(fromFieldE, fromNode, node)
#include "NodeFields.h"
#undef EVENT_IN
#undef EVENT_OUT
#undef EXPOSED_FIELD
#undef BEGIN_NODE
                    EVENT_NODE_DEFAULT;
                        }

#undef END_NODE 
#define END_NODE(n) EVENT_END_NODE(n,EVENT_OUT[fromFieldO])
                /* If the from field is an event out */
            } else if(fromFieldO!=ID_UNDEFINED) {
                /* Get the offset and size of this field in the fromNode */
                switch(fromNode->_nodeType) {
#define EVENT_IN(n, f, t, v, realType)
#define EXPOSED_FIELD(n, f, t, v, realType)
#define EVENT_OUT(node, field, type, var, realType) \
     PROCESS_EVENT(EVENT_OUT, from, node, field, type, var, realType)
#define BEGIN_NODE(node) \
     EVENT_BEGIN_NODE(fromFieldO, fromNode, node)
#include "NodeFields.h"
#undef EVENT_IN
#undef EVENT_OUT
#undef EXPOSED_FIELD
#undef BEGIN_NODE
                    EVENT_NODE_DEFAULT;
                        }
            }
	}


#undef END_NODE 
#define END_NODE(n) EVENT_END_NODE(n,EXPOSED_FIELD[toFieldE])

        /* Process to inputOnly */
        /* Get the offset to the inputOnly in the toNode and store it in toOfs */
        /* Get the size of the outputOnly type and store it in fromType */
        if(!toScriptField) {
            /* If this is an exposed field */
            if(toFieldE!=ID_UNDEFINED) {
                /* get the offset and size of this field in the toNode */
                switch(toNode->_nodeType) {
#define EVENT_IN(n, f, t, v, realType)
#define EVENT_OUT(n, f, t, v, realType)
#define EXPOSED_FIELD(node, field, type, var, realType) \
     PROCESS_EVENT(EXPOSED_FIELD, to, node, field, type, var, realType)
#define BEGIN_NODE(node) \
     EVENT_BEGIN_NODE(toFieldE, toNode, node)
#include "NodeFields.h"
#undef EVENT_IN
#undef EVENT_OUT
#undef EXPOSED_FIELD
#undef BEGIN_NODE
                    EVENT_NODE_DEFAULT;
                        }

#undef END_NODE 
#define END_NODE(n) EVENT_END_NODE(n,EVENT_IN[toFieldO])

                /* If this is an inputOnly */
            } else if(toFieldO!=ID_UNDEFINED) {
                /* Get the offset and size of this field in the toNode */
                switch(toNode->_nodeType) {
#define EVENT_OUT(n, f, t, v, realType)
#define EXPOSED_FIELD(n, f, t, v, realType)
#define EVENT_IN(node, field, type, var, realType) \
     PROCESS_EVENT(EVENT_IN, to, node, field, type, var, realType)
#define BEGIN_NODE(node) \
     EVENT_BEGIN_NODE(toFieldO, toNode, node)
#include "NodeFields.h"
#undef EVENT_IN
#undef EVENT_OUT
#undef EXPOSED_FIELD
#undef BEGIN_NODE
                    EVENT_NODE_DEFAULT;
                        }
            }
	}

        /* Clean up. */
#undef FIELD
#undef END_NODE

        /* set the toType and fromType from the PROTO/Script info, if appropriate */
        if(fromScriptField) fromType=fieldDecl_getType(fromScriptField->fieldDecl);
        if(toScriptField) toType=fieldDecl_getType(toScriptField->fieldDecl);

	/* 
	printf ("fromScriptField %d, toScriptField %d\n",fromScriptField, toScriptField);
	printf ("fromType %d, toType %d\n",fromType,toType);
	printf ("types, %s and %s\n",stringFieldtypeType(fromType), stringFieldtypeType(toType));
	*/

    /* We can only ROUTE between two equivalent fields.  If the size of one field value is different from the size of the other, we have problems (i.e. can't route SFInt to MFNode) */
    if(fromType!=toType) {
        /* try to make a better error message. */
        strcpy (p->fw_outline,"ERROR:Types mismatch in ROUTE: ");
        if (fromNode != NULL) { strcat (p->fw_outline, " from type:"); strcat (p->fw_outline, stringNodeType(fromNode->_nodeType)); strcat (p->fw_outline, " "); }
        if (fromFieldE != ID_UNDEFINED) { strcat (p->fw_outline, ":"); strcat (p->fw_outline, EXPOSED_FIELD[fromFieldE]); strcat (p->fw_outline, " "); }
        if (fromFieldO != ID_UNDEFINED) { strcat (p->fw_outline, ":"); strcat (p->fw_outline, EVENT_OUT[fromFieldO]); strcat (p->fw_outline, " "); }

        if (toNode != NULL) { strcat (p->fw_outline, " to type:"); strcat (p->fw_outline, stringNodeType(toNode->_nodeType)); strcat (p->fw_outline, " "); }
        if (toFieldE != ID_UNDEFINED) { strcat (p->fw_outline, ":"); strcat (p->fw_outline, EXPOSED_FIELD[toFieldE]); strcat (p->fw_outline, " "); }
        if (toFieldO != ID_UNDEFINED) { strcat (p->fw_outline, ":"); strcat (p->fw_outline, EVENT_IN[toFieldO]); strcat (p->fw_outline, " "); }

        /* PARSE_ERROR(fw_outline) */
        CPARSE_ERROR_CURID(p->fw_outline); 
        PARSER_FINALLY; 
		return FALSE; 
    }

    /* Finally, register the route. */
    /* **************************** */

    /* Built-in to built-in */
	parser_registerRoute(me, fromNode, fromOfs, toNode, toOfs, toType); //old way direct registration

    return TRUE;
}
//#include "broto5.h"
static BOOL parser_routeStatement_B(struct VRMLParser* me);
static BOOL parser_routeStatement_A(struct VRMLParser* me);

static BOOL parser_routeStatement(struct VRMLParser* me)
{
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
	if(p->useBrotos)
		return parser_routeStatement_B(me);
	else
		return parser_routeStatement_A(me);
}

/* Register a ROUTE here */
/* If we are in a PROTO add a new ProtoRoute structure to the vector ProtoDefinition->routes */
/* Otherwise, add the ROUTE to the routing table CRoutes */
void parser_registerRoute(struct VRMLParser* me,
                          struct X3D_Node* fromNode, int fromOfs,
                          struct X3D_Node* toNode, int toOfs,
                          int ft)
{
    ASSERT(me);
	if ((fromOfs == ID_UNDEFINED) || (toOfs == ID_UNDEFINED)) {
		ConsoleMessage ("problem registering route - either fromField or toField invalid");
	} else {
        	CRoutes_RegisterSimple(fromNode, fromOfs, toNode, toOfs, ft);
	}
}
//#include "broto6.h"

/* parse a DEF statement. Return a pointer to a vrmlNodeT */
static vrmlNodeT parse_KW_DEF(struct VRMLParser *me) {
    int ind = ID_UNDEFINED;
    vrmlNodeT node;
	char* name;

    /* lexer_defineNodeName is #defined as lexer_defineID(me, ret, stack_top(struct Vector*, userNodeNames), TRUE) */
    /* Checks if this node already exists in the userNodeNames vector.  If it doesn't, adds it. */
    if(!lexer_defineNodeName(me->lexer, &ind))
        PARSE_ERROR("Expected nodeNameId after DEF!\n")
            ASSERT(ind!=ID_UNDEFINED);


    /* If the DEFedNodes stack has not already been created.  If not, create new stack and add an X3D_Nodes vector to that stack */ 
    if(!me->DEFedNodes || stack_empty(me->DEFedNodes)) {
        /* printf ("parsing KW_DEF, creating new Vectors...\n"); */
        parser_scopeIn_DEFUSE(me);
    }
    ASSERT(me->DEFedNodes);
    ASSERT(!stack_empty(me->DEFedNodes));

    /* Did we just add the name to the userNodeNames vector?  If so, then the node hasn't yet been 
	   added to the DEFedNodes vector, so add it */
    ASSERT(ind<=vectorSize(stack_top(struct Vector*, me->DEFedNodes)));
    if(ind==vectorSize(stack_top(struct Vector*, me->DEFedNodes))) {
        vector_pushBack(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes), NULL);
    }
    ASSERT(ind<vectorSize(stack_top(struct Vector*, me->DEFedNodes)));


    /* Parse this node.  Create an X3D_Node structure of the appropriate type for this node 
	   and fill in the values for the fields specified.  
	   Add any routes to the CRoutes table. Add any PROTOs to the PROTOs vector */
#ifdef CPARSERVERBOSE
    printf("parser_KW_DEF: parsing DEFed node \n");
#endif
    if(!parser_node(me, &node,ind)) {
        /* PARSE_ERROR("Expected node in DEF statement!\n") */
        /* try to make a better error message. */
        CPARSE_ERROR_CURID("ERROR:Expected an X3D node in a DEF statement, got \"");
        PARSER_FINALLY; 
	return NULL; 
    }
#ifdef CPARSERVERBOSE
    printf("parser_KW_DEF: DEFed node successfully parsed\n");
#endif

    /* Return a pointer to the node in the variable ret */
	{
		ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
		//name = vector_get(char *,me->lexer->userNodeNames, ind);
		if(p->useBrotos){
			name = vector_get(char*, stack_top(struct Vector*, me->lexer->userNodeNames), ind);
			broto_store_DEF((struct X3D_Proto*)(me->ptr),node, name);
		}
	}

	return (vrmlNodeT) vector_get(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes), ind);
}



/* parse a USE statement. Return a pointer to a vrmlNodeT */
static vrmlNodeT parse_KW_USE(struct VRMLParser *me) {
    int ind;

    /* lexer_nodeName is #defined as 
	lexer_specialID(me, NULL, ret, NULL, 0, stack_top(struct Vector*, userNodeNames)) */
    /* Look for the nodename in list of user-defined node names (userNodeNames) and return the index in ret */
    if(!lexer_nodeName(me->lexer, &ind)) {
        CPARSE_ERROR_CURID("ERROR:Expected valid DEF name after USE; found: ");
	FREE_IF_NZ(me->lexer->curID);
	return NULL;
    }
#ifdef CPARSERVERBOSE
    printf("parser_KW_USE: parsing USE\n");
#endif

    /* If we're USEing it, it has to already be defined. */
    ASSERT(ind!=ID_UNDEFINED);

    /* It also has to be in the DEFedNodes stack */
    ASSERT(me->DEFedNodes && !stack_empty(me->DEFedNodes) &&
           ind<vectorSize(stack_top(struct Vector*, me->DEFedNodes)));

    #ifdef CPARSERVERBOSE
    printf ("parser_KW_USE, returning vector %u\n", vector_get(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes), ind));
    #endif

    /* Get a pointer to the X3D_Node structure for this DEFed node and return it in ret */
    return (vrmlNodeT) vector_get(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes), ind);
}


 
/* Parses a node (node non-terminal) */
/* Looks up the node type on the builtin NODES list and the userNodeNames list.  
   If this is a builtin node type, creates a new X3D_Node structure of the appropriate type for the node, and then parses the statements for that node.  
   For each field statement, gets the value for that field and stores it in the X3D_Node structure.  
   For each ROUTE statement, adds the route to the CRoutes table.
   For each PROTO statement, adds the PROTO definition to te PROTOs list. 
   Return a pointer to the X3D_Node structure that holds the information for this node.
   If this is a user-defined node type (i.e. a PROTO expansion), complete the proto expansion.  
   For each field in the ProtoDefinition either parse and propagate the specified value for this field, or 
   propagate the default value of the field.  (i.e. copy the appropriate value into every node/field combination in
   the dests list.)
   For each route in the routes list of the ProtoDefinition, add the route to the CRoutes table.
   Return a pointer to the X3D_Node structure that is the scenegraph for this PROTO.
*/
/* Specific initialization of node fields */
void parser_specificInitNode_A(struct X3D_Node* n, struct VRMLParser* me);
static BOOL parser_node_A(struct VRMLParser* me, vrmlNodeT* ret, int ind) {
    int nodeTypeB, nodeTypeU;
    struct X3D_Node* node=NULL;
    struct ProtoDefinition *thisProto = NULL;
        
    ASSERT(me->lexer);
    *ret=node; /* set this to NULL, for now... if this is a real node, it will return a node pointer */
         
    /* lexer_node( ... ) #defined to lexer_specialID(me, r1, r2, NODES, NODES_COUNT, userNodeTypesVec) where userNodeTypesVec is a list of PROTO defs */
    /* this will get the next token (which will be the node type) and search the NODES array for it.  If it is found in the NODES array nodeTypeB will be set to 
       the index of type in the NODES array.  If it is not in NODES, the list of user-defined nodes will be searched for the type.  If it is found in the user-defined 
       list nodeTypeU will be set to the index of the type in userNodeTypesVec.  A return value of FALSE indicates that the node type wasn't found in either list */

#ifdef CPARSERVERBOSE
    printf ("parser_node START, curID :%s: nextIn :%s:\n",me->lexer->curID, me->lexer->nextIn);
#endif


#define XBLOCK_STATEMENT(LOCATION) \
   if(parser_routeStatement(me))  { \
        return TRUE; \
   } \
 \
  if (parser_componentStatement(me)) { \
        return TRUE; \
  } \
 \
  if (parser_exportStatement(me)) { \
        return TRUE; \
  } \
 \
  if (parser_importStatement(me)) { \
        return TRUE; \
  } \
 \
  if (parser_metaStatement(me)) { \
        return TRUE; \
  } \
 \
  if (parser_profileStatement(me)) { \
        return TRUE; \
  }

    XBLOCK_STATEMENT(ddd)


        if(!lexer_node(me->lexer, &nodeTypeB, &nodeTypeU)) {
#ifdef CPARSERVERBOSE
            printf ("parser_node, not lexed - is this one of those special nodes?\n");
#endif
            return FALSE;
        }
        
    /* printf ("after lexer_node, at this point, me->lexer->curID :%s:\n",me->lexer->curID); */
    /* could this be a proto expansion?? */

    /* Checks that the next non-whitespace non-comment character is '{' and skips it. */
    if(!lexer_openCurly(me->lexer))
        PARSE_ERROR("Expected { after node-type id!")

#ifdef CPARSERVERBOSE
            printf ("parser_node: have nodeTypeB %d nodeTypeU %d\n",nodeTypeB, nodeTypeU);
#endif

    if (nodeTypeU != ID_UNDEFINED) {
        /* The node name was located in userNodeTypesVec (list of defined PROTOs), therefore this is an attempt to instantiate a PROTO */
        /* expand this PROTO, put the code right in line, and let the parser go over it as if there was never a proto here... */
        
        char *newProtoText = NULL;
        
#ifdef CPARSERVERBOSE
        printf ("nodeTypeB = ID_UNDEFINED, but nodeTypeU is %d\n",nodeTypeU);
#endif
                
        /* If this is a PROTO instantiation, then there must be at least one PROTO defined.  Also, the index retrieved for
           this PROTO must be valid. */
        ASSERT(nodeTypeU!=ID_UNDEFINED);
        ASSERT(me->PROTOs);
        ASSERT(nodeTypeU<vectorSize(me->PROTOs));
                
        if (me->curPROTO == NULL) {
	    int newProtoTextLen = 0;

            /* printf ("curPROTO = NULL: before protoExpand, current stream :%s:\n",me->lexer->nextIn); */
                
            /* find and expand the PROTO definition */
            newProtoText = protoExpand(me, nodeTypeU,&thisProto,&newProtoTextLen);
                
            /* printf ("curPROTO = NULL: past protoExpand\n"); */
	    parser_fromString(me,newProtoText);
                
            /* printf ("curPROTO = NULL: past insertProtoExpansionIntoStream\n"); */
            /* and, now, lets get the id for the lexer */
            if(!lexer_node(me->lexer, &nodeTypeB, &nodeTypeU)) {
                printf ("after protoexpand lexer_node, is this one of those special ones? \n");
                FREE_IF_NZ(newProtoText);
                return FALSE;
            }
                

            /* printf ("curPROTO = NULL: after possible proto expansion, me->lexer->curID :%s: ",me->lexer->curID);
               printf ("curPROTO = NULL: and remainder, me->lexer->nextIn :%s:\n",me->lexer->nextIn); */

            if(!lexer_openCurly(me->lexer))
                PARSE_ERROR("Expected { after node-type id!")

                
                    } else {
                        /* Checks that the next non-whitespace non-comment character is an openCurly and skips it. */
                        if(!lexer_openCurly(me->lexer)) {
                            PARSE_ERROR("Expected after node-type id!")
                                } else {
                                    /* printf ("curPROTO != NULL; lets skip these fields\n"); */
                                    skipToEndOfOpenCurly(me->lexer,0);
                                    /* printf ("after skipToEndOfOpenCurly, we have :%s:\n",me->lexer->nextIn); */
                                }
                    }
        /* JAS - now done by stacked lexer FREE_IF_NZ(newProtoText); */
    }

    /* Built-in node */
    /* Node was found in NODES array */
    if(nodeTypeB!=ID_UNDEFINED) {
#ifdef CPARSERVERBOSE
        printf("parser_node: parsing builtin node\n");
#endif

	#ifdef HAVE_JAVASCRIPT
        struct Shader_Script* script=NULL;
	#endif

        struct Shader_Script* shader=NULL;
                 
        /* Get malloced struct of appropriate X3D_Node type with default values filled in */
        node=X3D_NODE(createNewX3DNode((int)nodeTypeB));
        ASSERT(node);
                
        /* if ind != ID_UNDEFINED, we have the first node of a DEF. Save this node pointer, in case
           some code uses it. eg: DEF xx Transform {children Script {field yy USE xx}} */
        if (ind != ID_UNDEFINED) {
            /* Set the top memmber of the DEFed nodes stack to this node */
            vector_get(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes), ind)=node;
#ifdef CPARSERVERBOSE
            printf("parser_node: adding DEFed node (pointer %p) to DEFedNodes vector\n", node);  
#endif
        }
                
        /* Node specific initialization */
        /* From what I can tell, this only does something for Script nodes.  It sets node->__scriptObj to new_Shader_Script() */
        parser_specificInitNode_A(node, me);
                
        /* Set flag for Shaders/Scripts - these ones can have any number of fields */
	switch (node->_nodeType) {
		#ifdef HAVE_JAVASCRIPT
		case NODE_Script: script=X3D_SCRIPT(node)->__scriptObj; break;
		#endif

		case NODE_ShaderProgram: shader=(struct Shader_Script *)(X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields); break;
		case NODE_PackagedShader: shader=(struct Shader_Script *)(X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields); break;
		case NODE_ComposedShader: shader=(struct Shader_Script *)(X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields); break;
		default: {}
	}
        
        /* As long as the lexer is returning field statements, ROUTE statements, or PROTO statements continue parsing node */
        while(TRUE)
        {
            /* Try to parse the next statement as a field.  For normal "field value" statements (i.e. position 1 0 1) this gets the value of the field from the lexer (next token(s)
               to be processed) and stores it as the appropriate type in the node.
               For IS statements (i.e. position IS mypos) this adds the node-field combo (as an offsetPointer) to the dests list for the protoFieldDecl associated with the user
               defined field (in the given case, this would be mypos).  */
#ifdef CPARSERVERBOSE
            printf("parser_node: try parsing field ... \n");
#endif
            if(parser_field(me, node)) {
#ifdef CPARSERVERBOSE
                printf("parser_node: field parsed\n");
#endif
                continue;
            }
                        
            /* Try to parse the next statement as a ROUTE (i.e. statement starts with ROUTE).  This checks that the ROUTE statement is valid (i.e. that the referenced node and field combinations  
               exist, and that they are compatible) and then adds the route to either the CRoutes table of routes, or adds a new ProtoRoute structure to the vector 
               ProtoDefinition->routes if we are parsing a PROTO */
#ifdef CPARSERVERBOSE
            printf("parser_node: try parsing ROUTE ... \n");
#endif
                        
            /* try ROUTE, COMPONENT, EXPORT, IMPORT, META, PROFILE statements here */
            BLOCK_STATEMENT(parser_node);
                        
                /* Try to parse the next statement as a PROTO (i.e. statement starts with PROTO).  */
                /* Add the PROTO name to the userNodeTypesVec list of names.  Create and fill in a new protoDefinition structure and add it to the PROTOs list.
                   Goes through the interface declarations for the PROTO and adds each user-defined field name to the appropriate list of user-defined names (user_initializeOnly, 
                   user_inputOnly, Out, or user_inputOutput), creates a new protoFieldDecl for the field and adds it to the iface vector of the ProtoDefinition, 
                   and, in the case of fields and inputOutputs, gets the default value of the field and stores it in the protoFieldDecl. 
                   Parses the body of the PROTO.  Nodes are added to the scene graph for this PROTO.  Routes are parsed and a new ProtoRoute structure
                   is created for each one and added to the routes vector of the ProtoDefinition.  PROTOs are recursively parsed!
                */
#ifdef CPARSERVERBOSE
                printf("parser_node: try parsing PROTO ... \n");
#endif
            if(parser_protoStatement(me)) {
#ifdef CPARSERVERBOSE
                printf("parser_node: PROTO parsed\n");
#endif
                continue;
            }
#ifdef CPARSERVERBOSE
            printf("parser_node: try parsing Script or Shader field\n");
#endif

	    #ifdef HAVE_JAVASCRIPT
            if(script && parser_interfaceDeclaration(me, NULL, script)) {
#ifdef CPARSERVERBOSE
                printf("parser_node: SCRIPT field parsed\n");
#endif
                continue;
            }
	    #endif

            if(shader && parser_interfaceDeclaration(me, NULL, shader)) {
#ifdef CPARSERVERBOSE
                printf("parser_node: Shader field parsed\n");
#endif
                continue;
            }
            break;
        }
                
	#ifdef HAVE_JAVASCRIPT
        /* Init code for Scripts */
        if(script) {
#ifdef CPARSERVERBOSE
            printf("parser_node: try parsing SCRIPT url\n");
#endif
            script_initCodeFromMFUri(script, &X3D_SCRIPT(node)->url);
#ifdef CPARSERVERBOSE
            printf("parser_node: SCRIPT url parsed\n");
#endif
        }
	#endif /* HAVE_JAVASCRIPT */
                
        /* We must have a node that we've parsed at this point. */
        ASSERT(node);
    }
        
    /* Check that the node is closed by a '}', and skip this token */
#ifdef CPARSERVERBOSE
    printf ("calling lexer_closeCurly at B\n");
#endif
        
    if(!lexer_closeCurly(me->lexer)) {
        CPARSE_ERROR_CURID("ERROR: Expected a closing brace after fields of a node;")
            PARSER_FINALLY;
        return FALSE; 
    }
        
    /* Return the parsed node */

    #ifdef CPARSERVERBOSE
    printf ("returning at end of parser_node, ret %u\n",node);
    if (node != NULL) printf ("and, node type is %s\n",stringNodeType(node->_nodeType));
    #endif
    *ret=node;
    return TRUE;
}


static BOOL parser_nodeStatement(struct VRMLParser* me, vrmlNodeT* ret)
{
    ASSERT(me->lexer);
	
    /* A DEF-statement? */
    if(lexer_keyword(me->lexer, KW_DEF)) {
		//vrmlNodeT * node = ret;
		//*node  = parse_KW_DEF(me);
		*ret  = parse_KW_DEF(me);
        return TRUE;
    }

    /* A USE-statement? */
    if(lexer_keyword(me->lexer, KW_USE)) {
        *ret= parse_KW_USE(me);
        return TRUE;
    }

	/* Otherwise, simply a node. */
    return parser_node(me, ret, ID_UNDEFINED);
}
//#include "broto7.h"



/* add_parent for Multi_Node */
void mfnode_add_parent(struct Multi_Node* node, struct X3D_Node* parent)
{
    int i;
    for(i=0; i!=node->n; ++i) {
        ADD_PARENT(node->p[i], parent);
    }
}


/* Parses a field value (literally or IS) */
/* Gets the actual value of a field and stores it in a node, or, for an IS statement, adds this node and field as a destination to the appropriate protoFieldDecl */
/* Passed pointer to the parser, an offsetPointer structure pointing to the current node and an offset to the field being parsed, type of the event value (i.e. MFString) index in FIELDTYPES, */
/* index of the field in the FIELDNAMES (or equivalent) array */
/* Parses a field value of a certain type (literally or IS) */

static BOOL parser_fieldValue(struct VRMLParser* me, struct X3D_Node *node, int offs,
                       int type, int origFieldE, BOOL protoExpansion, struct ProtoDefinition* pdef, struct ProtoFieldDecl* origField)
{
#undef PARSER_FINALLY
#define PARSER_FINALLY

#ifdef CPARSERVERBOSE
    printf ("start of parser_fieldValue\n");
    printf ("me->curPROTO = %u\n",me->curPROTO);
#endif

    {
#ifdef CPARSERVERBOSE
        printf ("parser_fieldValue, not an IS\n");
#endif
        /* Get a pointer to the actual field */
#define myOffsetPointer_deref(t, me) \
 ((t)(((char*)(node))+offs))

        void* directRet=myOffsetPointer_deref(void*, ret);

        /* we could print out a type, as shown below for the first element of a Multi_Color:
           { struct Multi_Color * mc;
           mc = (struct Multi_Color *) directRet;
           printf ("directret n is %d\n",mc->n);
        
           printf ("directret orig is %u, %f %f %f\n",
           mc->p,
           mc->p[0].c[0],
           mc->p[0].c[1],
           mc->p[0].c[2]);
           }
        */

        PARSER_FINALLY;
	#ifdef CPARSERVERBOSE
	printf ("parser_fieldValue, me %u, directRet %u\n",me,directRet);
	#endif
 
            /* Get the actual value from the file (next token from lexer) and store it as the appropriate type in the node */
            return PARSE_TYPE[type](me, directRet);
    }

#undef PARSER_FINALLY
#define PARSER_FINALLY
}

/* Specific initialization of node fields */
void parser_specificInitNode_A(struct X3D_Node* n, struct VRMLParser* me)
{
    switch(n->_nodeType)
    {

#define NODE_SPECIFIC_INIT(type, code) \
   case NODE_##type: \
   { \
    struct X3D_##type* node=(struct X3D_##type*)n; \
    code \
	break; \
   }

        /* Scripts get a script object associated to them */
	#ifdef HAVE_JAVASCRIPT
        NODE_SPECIFIC_INIT(Script, node->__scriptObj=new_Shader_Script(X3D_NODE(node));)
	#endif

        NODE_SPECIFIC_INIT(ShaderProgram, node->_shaderUserDefinedFields=X3D_NODE(new_Shader_Script(X3D_NODE(node)));)
        NODE_SPECIFIC_INIT(PackagedShader, node->_shaderUserDefinedFields=X3D_NODE(new_Shader_Script(X3D_NODE(node)));)
        NODE_SPECIFIC_INIT(ComposedShader, node->_shaderUserDefinedFields=X3D_NODE(new_Shader_Script(X3D_NODE(node)));)

            }
}


/* Specific initialization of node fields */
void parser_specificInitNode_B(struct X3D_Node* n, struct VRMLParser* me)
{
    switch(n->_nodeType)
    {

#define NODE_SPECIFIC_INIT(type, code) \
   case NODE_##type: \
   { \
    struct X3D_##type* node=(struct X3D_##type*)n; \
    code \
	break; \
   }

        /* Scripts get a script object associated to them */
	#ifdef HAVE_JAVASCRIPT
        NODE_SPECIFIC_INIT(Script, node->__scriptObj=new_Shader_ScriptB(X3D_NODE(node));)
	#endif

        NODE_SPECIFIC_INIT(ShaderProgram, node->_shaderUserDefinedFields=X3D_NODE(new_Shader_ScriptB(X3D_NODE(node)));)
        NODE_SPECIFIC_INIT(PackagedShader, node->_shaderUserDefinedFields=X3D_NODE(new_Shader_ScriptB(X3D_NODE(node)));)
        NODE_SPECIFIC_INIT(ComposedShader, node->_shaderUserDefinedFields=X3D_NODE(new_Shader_ScriptB(X3D_NODE(node)));)

            }
}
//#include "broto8.h"

/* ************************************************************************** */
/* Built-in fields */
/* Parses a built-in field and sets it in node */
static BOOL parser_field_A(struct VRMLParser* me, struct X3D_Node* node)
{
    int fieldO;
    int fieldE;

    ASSERT(me->lexer);

    /* printf ("start of parser_field, me->lexer->nextIn :%s:\n",me->lexer->nextIn);  */

    /* Ask the lexer to find the field (next lexer token) in either the FIELDNAMES array or the EXPOSED_FIELD array. The 
       index of the field in the array is returned in fieldO (if found in FIELDNAMES) or fieldE (if found in EXPOSED_FIELD).  
       If the fieldname is found in neither array, lexer_field will return FALSE. */
    if(!lexer_field(me->lexer, &fieldO, &fieldE, NULL, NULL))
        /* If lexer_field does return false, this is an inputOnly/outputOnly IS user_definedField statement.  Add a Offset_Pointer structure to the dests list for the protoFieldDecl for 
           the user defined field.  The Offset_Pointer structure contains a pointer to the node currently being parsed along with an offset that references the field that is linked to the
           user defined field. 

           i.e. for a statement "rotation IS myrot" the protoFieldDecl for myrot is retrieved, and an Offset_Pointer structure is added to the dests list which contains a pointer to the current 
           node and the offset for the "rotation" field in that node.  

           If we've done all this, then we've parsed the field statement completely, and we return. */ 
	return FALSE;

    /* Ignore all events */
#define EVENT_IN(n, f, t, v, realType)
#define EVENT_OUT(n, f, t, v, realType)

    /* End of node is the same for fields and inputOutputs */
#define END_NODE(type) \
   } \
  } \
  break;

/* The init codes used. */
#define INIT_CODE_sfnode(var) \
  ADD_PARENT(node2->var, X3D_NODE(node2));
#define INIT_CODE_mfnode(var) \
  mfnode_add_parent(&node2->var, X3D_NODE(node2));
#define INIT_CODE_sfbool(var)
#define INIT_CODE_sfcolor(var)
#define INIT_CODE_sfcolorrgba(var)
#define INIT_CODE_sffloat(var)
#define INIT_CODE_sfimage(var)
#define INIT_CODE_sfint32(var)
#define INIT_CODE_sfrotation(var)
#define INIT_CODE_sfstring(var)
#define INIT_CODE_sftime(var)
#define INIT_CODE_sfvec2f(var)
#define INIT_CODE_sfvec3f(var)
#define INIT_CODE_sfvec3d(var)
#define INIT_CODE_mfbool(var)
#define INIT_CODE_mfcolor(var)
#define INIT_CODE_mfcolorrgba(var)
#define INIT_CODE_mffloat(var)
#define INIT_CODE_mfint32(var)
#define INIT_CODE_mfrotation(var)
#define INIT_CODE_mfstring(var)
#define INIT_CODE_mftime(var)
#define INIT_CODE_mfvec2f(var)
#define INIT_CODE_mfvec3f(var)
#define INIT_CODE_mfvec3d(var)
#define INIT_CODE_sfdouble(var)
#define INIT_CODE_mfdouble(var)
#define INIT_CODE_sfvec4d(var)
#define INIT_CODE_mfmatrix3f(var)
#define INIT_CODE_mfmatrix4f(var)

#define INIT_CODE_mfmatrix3d(var)
#define INIT_CODE_mfmatrix4d(var)
#define INIT_CODE_mfvec2d(var)
#define INIT_CODE_mfvec4d(var)
#define INIT_CODE_mfvec4f(var)
#define INIT_CODE_sfmatrix3d(var)
#define INIT_CODE_sfmatrix3f(var)
#define INIT_CODE_sfmatrix4d(var)
#define INIT_CODE_sfmatrix4f(var)
#define INIT_CODE_sfvec2d(var)
#define INIT_CODE_sfvec4f(var)


/* The field type indices */
#define FTIND_sfnode    FIELDTYPE_SFNode
#define FTIND_sfbool    FIELDTYPE_SFBool
#define FTIND_sfcolor   FIELDTYPE_SFColor
#define FTIND_sfcolorrgba       FIELDTYPE_SFColorRGBA
#define FTIND_sffloat   FIELDTYPE_SFFloat
#define FTIND_sfimage   FIELDTYPE_SFImage
#define FTIND_sfint32   FIELDTYPE_SFInt32
#define FTIND_sfrotation        FIELDTYPE_SFRotation
#define FTIND_sfstring  FIELDTYPE_SFString
#define FTIND_sftime    FIELDTYPE_SFTime
#define FTIND_sfdouble  FIELDTYPE_SFDouble
#define FTIND_sfvec2f   FIELDTYPE_SFVec2f
#define FTIND_sfvec2d   FIELDTYPE_SFVec2d
#define FTIND_sfvec3f   FIELDTYPE_SFVec3f
#define FTIND_sfvec3d   FIELDTYPE_SFVec3d
#define FTIND_sfvec4f	FIELDTYPE_SFVec4f
#define FTIND_sfvec4d	FIELDTYPE_SFVec4d
#define FTIND_sfmatrix3f FIELDTYPE_SFMatrix3f
#define FTIND_sfmatrix4f FIELDTYPE_SFMatrix4f
#define FTIND_sfmatrix3d FIELDTYPE_SFMatrix3d
#define FTIND_sfmatrix4d FIELDTYPE_SFMatrix4d

#define FTIND_mfnode    FIELDTYPE_MFNode
#define FTIND_mfbool    FIELDTYPE_MFBool
#define FTIND_mfcolor   FIELDTYPE_MFColor
#define FTIND_mfcolorrgba       FIELDTYPE_MFColorRGBA
#define FTIND_mffloat   FIELDTYPE_MFFloat
#define FTIND_mfint32   FIELDTYPE_MFInt32
#define FTIND_mfrotation        FIELDTYPE_MFRotation
#define FTIND_mfstring  FIELDTYPE_MFString
#define FTIND_mftime    FIELDTYPE_MFTime
#define FTIND_mfvec2f   FIELDTYPE_MFVec2f
#define FTIND_mfvec2d   FIELDTYPE_MFVec2d
#define FTIND_mfvec3f   FIELDTYPE_MFVec3f
#define FTIND_mfvec3d   FIELDTYPE_MFVec3d
#define FTIND_mfvec4d   FIELDTYPE_MFVec4d
#define FTIND_mfvec4f   FIELDTYPE_MFVec4f
#define FTIND_mfdouble  FIELDTYPE_MFDouble
#define FTIND_mfmatrix3f FIELDTYPE_MFMatrix3f
#define FTIND_mfmatrix4f FIELDTYPE_MFMatrix4f
#define FTIND_mfmatrix3d FIELDTYPE_MFMatrix3d
#define FTIND_mfmatrix4d FIELDTYPE_MFMatrix4d
 
/* Process a field (either exposed or ordinary) generally */
/* For a normal "field value" (i.e. position 1 0 1) statement gets the actual value of the field from the file (next token(s) to be processed) and stores it in the node
   For an IS statement, adds this node-field combo as a destination to the appropriate protoFieldDecl */
#define PROCESS_FIELD(exposed, node, field, fieldType, var, fe) \
  case exposed##FIELD_##field: \
   if(!parser_fieldValue(me, \
    X3D_NODE(node2), (int) offsetof(struct X3D_##node, var), \
    FTIND_##fieldType, fe, FALSE, NULL, NULL)) {\
    printf ("error in parser_fieldValue by call 2\n"); \
        PARSE_ERROR("Expected " #fieldType " Value for a fieldtype!") }\
   INIT_CODE_##fieldType(var) \
   return TRUE;
 
/* Default action if node is not encountered in list of known nodes */
#define NODE_DEFAULT \
  default: \
   PARSE_ERROR("Parser PROCESS_FIELD, Unsupported node!")

/* printf ("at XXX, fieldE = %d, fieldO = %d nodeType %s\n",fieldE, fieldO,stringNodeType (node->_nodeType));
   if (fieldE!=ID_UNDEFINED) printf (".... field is %s\n",EXPOSED_FIELD[fieldE]);
   if (fieldO!=ID_UNDEFINED) printf (".... field is %s\n",FIELD[fieldO]);  */

/* Field was found in EXPOSED_FIELD list.  Parse value or IS statement */
if(fieldE!=ID_UNDEFINED)
    switch(node->_nodeType)
{

/* Processes exposed fields for node */
#define BEGIN_NODE(type) \
    case NODE_##type: \
    { \
     struct X3D_##type* node2=(struct X3D_##type*)node; \
	/* printf ("at YYY, in case for node %s\n",stringNodeType(NODE_##type)); */ \
     UNUSED(node2); /* for compiler warning reductions */ \
     switch(fieldE) \
     {

/* Process exposed fields */
#define EXPOSED_FIELD(node, field, fieldType, var, realType) \
    PROCESS_FIELD(EXPOSED_, node, field, fieldType, var, fieldE)

/* Ignore just fields */
#define FIELD(n, f, t, v, realType)

/* Process it */
#include "NodeFields.h"

/* Undef the field-specific macros */
#undef BEGIN_NODE
#undef FIELD
#undef EXPOSED_FIELD

NODE_DEFAULT

}

/* Field was found in FIELDS list.  Parse value or IS statement */
if(fieldO!=ID_UNDEFINED)
    switch(node->_nodeType)
{

    /* Processes ordinary fields for node */
#define BEGIN_NODE(type) \
    case NODE_##type: \
    { \
     struct X3D_##type* node2=(struct X3D_##type*)node; \
     UNUSED(node2); /* for compiler warning reductions */ \
     switch(fieldO) \
     {

         /* Process fields */
#define FIELD(node, field, fieldType, var, realType) \
    PROCESS_FIELD(, node, field, fieldType, var, ID_UNDEFINED)

         /* Ignore exposed fields */
#define EXPOSED_FIELD(n, f, t, v, realType)

         /* Process it */
#include "NodeFields.h"

         /* Undef the field-specific macros */
#undef BEGIN_NODE
#undef FIELD
#undef EXPOSED_FIELD

         NODE_DEFAULT
      
             }

/* Clean up */
#undef END_NODE
#undef EVENT_IN
#undef EVENT_OUT

/* If field was found, return TRUE; would have happened! */
PARSE_ERROR("Unsupported field for node!")
	return FALSE;
    }

/* Parses a fieldvalue for a built-in field and sets it in node */
static BOOL parser_field_B(struct VRMLParser* me, struct X3D_Node* node)
{
    int fieldO;
    int fieldE;
	//BOOL retval;
	DECLAREUP
    ASSERT(me->lexer);

    /* printf ("start of parser_field, me->lexer->nextIn :%s:\n",me->lexer->nextIn);  */

    /* Ask the lexer to find the field (next lexer token) in either the FIELDNAMES array or 
	   the EXPOSED_FIELD array. The index of the field in the array is returned in fieldO 
	   (if found in FIELDNAMES) or fieldE (if found in EXPOSED_FIELD).  
       If the fieldname is found in neither array, lexer_field will return FALSE. */
	SAVEUP

    if(!lexer_field(me->lexer, &fieldO, &fieldE, NULL, NULL))
	{
        /* If lexer_field does return false, this is an inputOnly/outputOnly IS user_definedField statement.  
		   Add a Offset_Pointer structure to the dests list for the protoFieldDecl for 
           the user defined field.  The Offset_Pointer structure contains a pointer to the node currently 
		   being parsed along with an offset that references the field that is linked to the
           user defined field. 

           i.e. for a statement "rotation IS myrot" the protoFieldDecl for myrot is retrieved, 
		   and an Offset_Pointer structure is added to the dests list which contains a pointer 
		   to the current node and the offset for the "rotation" field in that node.  

           If we've done all this, then we've parsed the field statement completely, and we return. */ 
		BACKUP
		return FALSE;
	}
	FREEUP
    /* Ignore all events */
#define EVENT_IN(n, f, t, v, realType)
#define EVENT_OUT(n, f, t, v, realType)

    /* End of node is the same for fields and inputOutputs */
#define END_NODE(type) \
   } \
  } \
  break;

/* The field type indices */
#define FTIND_sfnode    FIELDTYPE_SFNode
#define FTIND_sfbool    FIELDTYPE_SFBool
#define FTIND_sfcolor   FIELDTYPE_SFColor
#define FTIND_sfcolorrgba       FIELDTYPE_SFColorRGBA
#define FTIND_sffloat   FIELDTYPE_SFFloat
#define FTIND_sfimage   FIELDTYPE_SFImage
#define FTIND_sfint32   FIELDTYPE_SFInt32
#define FTIND_sfrotation        FIELDTYPE_SFRotation
#define FTIND_sfstring  FIELDTYPE_SFString
#define FTIND_sftime    FIELDTYPE_SFTime
#define FTIND_sfdouble  FIELDTYPE_SFDouble
#define FTIND_sfvec2f   FIELDTYPE_SFVec2f
#define FTIND_sfvec2d   FIELDTYPE_SFVec2d
#define FTIND_sfvec3f   FIELDTYPE_SFVec3f
#define FTIND_sfvec3d   FIELDTYPE_SFVec3d
#define FTIND_sfvec4f	FIELDTYPE_SFVec4f
#define FTIND_sfvec4d	FIELDTYPE_SFVec4d
#define FTIND_sfmatrix3f FIELDTYPE_SFMatrix3f
#define FTIND_sfmatrix4f FIELDTYPE_SFMatrix4f
#define FTIND_sfmatrix3d FIELDTYPE_SFMatrix3d
#define FTIND_sfmatrix4d FIELDTYPE_SFMatrix4d

#define FTIND_mfnode    FIELDTYPE_MFNode
#define FTIND_mfbool    FIELDTYPE_MFBool
#define FTIND_mfcolor   FIELDTYPE_MFColor
#define FTIND_mfcolorrgba       FIELDTYPE_MFColorRGBA
#define FTIND_mffloat   FIELDTYPE_MFFloat
#define FTIND_mfint32   FIELDTYPE_MFInt32
#define FTIND_mfrotation        FIELDTYPE_MFRotation
#define FTIND_mfstring  FIELDTYPE_MFString
#define FTIND_mftime    FIELDTYPE_MFTime
#define FTIND_mfvec2f   FIELDTYPE_MFVec2f
#define FTIND_mfvec2d   FIELDTYPE_MFVec2d
#define FTIND_mfvec3f   FIELDTYPE_MFVec3f
#define FTIND_mfvec3d   FIELDTYPE_MFVec3d
#define FTIND_mfvec4d   FIELDTYPE_MFVec4d
#define FTIND_mfvec4f   FIELDTYPE_MFVec4f
#define FTIND_mfdouble  FIELDTYPE_MFDouble
#define FTIND_mfmatrix3f FIELDTYPE_MFMatrix3f
#define FTIND_mfmatrix4f FIELDTYPE_MFMatrix4f
#define FTIND_mfmatrix3d FIELDTYPE_MFMatrix3d
#define FTIND_mfmatrix4d FIELDTYPE_MFMatrix4d
 
/* Process a field (either exposed or ordinary) generally */
/* For a normal "field value" (i.e. position 1 0 1) statement gets the actual value of the field 
   from the file (next token(s) to be processed) and stores it in the node
   For an IS statement, adds this node-field combo as a destination to the appropriate protoFieldDecl */
#define PROCESS_FIELD_B(exposed, node, field, fieldType, var, fe) \
  case exposed##FIELD_##field: \
   if(!parser_fieldValue(me, \
    X3D_NODE(node2), (int) offsetof(struct X3D_##node, var), \
    FTIND_##fieldType, fe, FALSE, NULL, NULL)) {\
    printf ("error in parser_fieldValue by call 2\n"); \
        PARSE_ERROR("Expected " #fieldType " Value for a fieldtype!") }\
	INIT_CODE_##fieldType(var) \
   return TRUE;
 
   //INIT_CODE_##fieldType(var) \  we're doing this add_parent during instancing as of feb 2013
   //return TRUE;

/* Default action if node is not encountered in list of known nodes */
#define NODE_DEFAULT_B \
  default: \
   PARSE_ERROR("Parser PROCESS_FIELD_B, Unsupported node!")

/* printf ("at XXX, fieldE = %d, fieldO = %d nodeType %s\n",fieldE, fieldO,stringNodeType (node->_nodeType));
   if (fieldE!=ID_UNDEFINED) printf (".... field is %s\n",EXPOSED_FIELD[fieldE]);
   if (fieldO!=ID_UNDEFINED) printf (".... field is %s\n",FIELD[fieldO]);  */

/* Field was found in EXPOSED_FIELD list.  Parse value or IS statement */
if(fieldE!=ID_UNDEFINED)
    switch(node->_nodeType)
{

/* Processes exposed fields for node */
#define BEGIN_NODE(type) \
    case NODE_##type: \
    { \
     struct X3D_##type* node2=(struct X3D_##type*)node; \
	/* printf ("at YYY, in case for node %s\n",stringNodeType(NODE_##type)); */ \
     UNUSED(node2); /* for compiler warning reductions */ \
     switch(fieldE) \
     {

/* Process exposed fields */
#define EXPOSED_FIELD(node, field, fieldType, var, realType) \
    PROCESS_FIELD_B(EXPOSED_, node, field, fieldType, var, fieldE)

/* Ignore just fields */
#define FIELD(n, f, t, v, realType)

/* Process it */
#include "NodeFields.h"

/* Undef the field-specific macros */
#undef BEGIN_NODE
#undef FIELD
#undef EXPOSED_FIELD

NODE_DEFAULT_B

}

/* Field was found in FIELDS list.  Parse value or IS statement */
if(fieldO!=ID_UNDEFINED)
    switch(node->_nodeType)
{

    /* Processes ordinary fields for node */
#define BEGIN_NODE(type) \
    case NODE_##type: \
    { \
     struct X3D_##type* node2=(struct X3D_##type*)node; \
     UNUSED(node2); /* for compiler warning reductions */ \
     switch(fieldO) \
     {

         /* Process fields */
#define FIELD(node, field, fieldType, var, realType) \
    PROCESS_FIELD_B(, node, field, fieldType, var, ID_UNDEFINED)

         /* Ignore exposed fields */
#define EXPOSED_FIELD(n, f, t, v, realType)

         /* Process it */
#include "NodeFields.h"

         /* Undef the field-specific macros */
#undef BEGIN_NODE
#undef FIELD
#undef EXPOSED_FIELD

         NODE_DEFAULT_B
      
             }

/* Clean up */
#undef END_NODE
#undef EVENT_IN
#undef EVENT_OUT

/* If field was found, return TRUE; would have happened! */
PARSE_ERROR("Unsupported field for node!")
	return FALSE;
    }


static BOOL parser_field(struct VRMLParser* me, struct X3D_Node* node)
{
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;
	if(p->useBrotos)
		return parser_field_B(me,node);
	else
		return parser_field_A(me,node);
}


/* ************************************************************************** */
/* MF* field values */

/* take a USE field, and stuff it into a Multi*type field  - see parser_mf routines below */

static void stuffDEFUSE(struct Multi_Node *outMF, vrmlNodeT in, int type) {
    /* printf ("stuff_it_in, got vrmlT vector successfully - it is a type of %s\n",stringNodeType(in->_nodeType));
       printf ("stuff_it_in, ret is %d\n",out); */

    /* convert, say, a X3D_something to a struct Multi_Node { int n; int  *p; }; */
    switch (type) {
        /* convert the node pointer into the "p" field of a Multi_MFNode */
    case FIELDTYPE_MFNode:
        /*struct Multi_Node { int n; void * *p; };*/
        outMF->n=1;
        outMF->p=MALLOC(void *, sizeof(struct X3D_Node*));
        outMF->p[0] = in;
        break;

    case FIELDTYPE_MFFloat:
    case FIELDTYPE_MFRotation:
    case FIELDTYPE_MFVec3f:
    case FIELDTYPE_MFBool:
    case FIELDTYPE_MFInt32:
    case FIELDTYPE_MFColor:
    case FIELDTYPE_MFColorRGBA:
    case FIELDTYPE_MFTime:
    case FIELDTYPE_MFDouble:
    case FIELDTYPE_MFString:
    case FIELDTYPE_MFVec2f:
    { size_t localSize;
    localSize =  returnRoutingElementLength(convertToSFType(type)); /* converts MF to equiv SF type */
    /* struct Multi_Float { int n; float  *p; }; */
    /* treat these all the same, as the data type is same size */
    outMF->n=1;
    outMF->p=MALLOC(void *, localSize);
    memcpy (&outMF->p[0], &in, localSize);
    break;
    }
    default: {
        ConsoleMessage("VRML Parser; stuffDEFUSE, unhandled type");
    }
    }
}


/* if we expect to find a MF field, but the user only puts a SF Field, we make up the MF field with
   1 entry, and copy the data over */
static void stuffSFintoMF(struct Multi_Node *outMF, vrmlNodeT *inSF, int type) {
    int rsz;
    int elelen;
    int i;

    /* printf ("stuffSFintoMF, got vrmlT vector successfully - it is a type of %s\n",FIELDTYPES[type]);  */

    rsz = returnElementRowSize(type);
    elelen = returnElementLength(type);

	/* printf ("stuffSFintoMF - rowsize %d length %d\n",rsz,elelen); */

    /* convert, say, a X3D_something to a struct Multi_Node { int n; int  *p; }; */
        /* convert the node pointer into the "p" field of a Multi_MFNode */

        /* struct Multi_Float { int n; float  *p; }; */
        /* struct Multi_Vec3f { int n; struct SFColor  *p; }; */
        /* treat these all the same, as the data type is same size */

        /* is the "old" size something other than 1? */
        /* I am not sure when this would ever happen, but one never knows... */

	/* free up old memory here */
	for (i=0; i<outMF->n; i++) {
		if (type == FIELDTYPE_MFString) {
			struct Uni_String *m = (struct Uni_String *)outMF->p[i];
			/* printf ("freeing string :%s:\n",m->strptr); */
			FREE_IF_NZ(m->strptr);
		}

	}

        if (outMF->n != 1) {
	    /* printf ("deleting pointer %p\n",outMF->p); */
            FREE_IF_NZ(outMF->p);
            outMF->n=1;
            outMF->p=MALLOC(void *, rsz * elelen);
        }

        /* { float *ptr; ptr = (float *) in; for (n=0; n<rsz; n++) { printf ("float %d is %f\n",n,*ptr); ptr++; } }  */

        memcpy (outMF->p, inSF, rsz * elelen); 
}

/* Parse a MF* field */
#define PARSER_MFFIELD(name, type) \
 static BOOL parser_mf##name##Value(struct VRMLParser* me, void *ret) { \
  struct Vector* vec; \
  vrmlNodeT RCX; \
  struct Multi_##type *rv; \
  RCX = NULL; \
  vec = NULL; \
  \
   /* printf ("start of a mfield parse for type %s curID :%s: me %u lexer %u\n",FIELDTYPES[FIELDTYPE_MF##type], me->lexer->curID,me,me->lexer); */ \
   /*  printf ("      str :%s:\n",me->lexer->startOfStringPtr[me->lexer->lexerInputLevel]);  */ \
   /* if (me->lexer->curID != NULL) printf ("parser_MF, have %s\n",me->lexer->curID); else printf("parser_MF, NULL\n"); */ \
\
 if (!(me->parsingX3DfromXML)) { \
          /* is this a USE statement? */ \
         if(lexer_keyword(me->lexer, KW_USE)) { \
                /* printf ("parser_MF, got a USE!\n"); */ \
                /* Get a pointer to the X3D_Node structure for this DEFed node and return it in ret */ \
                RCX=parse_KW_USE(me); \
                if (RCX == NULL) return FALSE; \
                /* so, we have a Multi_XX return val. (see Structs.h), have to get the info into a vrmlNodeT */ \
                stuffDEFUSE(ret, RCX, FIELDTYPE_MF##type); \
                return TRUE; \
         } \
         \
         else if (lexer_keyword(me->lexer, KW_DEF)) { \
                /* printf ("parser_MF, got the DEF!\n"); */ \
                /* Get a pointer to the X3D_Node structure for this DEFed node and return it in ret */ \
                RCX=parse_KW_DEF(me); \
                if (RCX == NULL) return FALSE; \
                \
                /* so, we have a Multi_XX return val. (see Structs.h), have to get the info into a vrmlNodeT */ \
                stuffDEFUSE(ret, RCX, FIELDTYPE_MF##type); \
                return TRUE; \
        } \
 }\
\
/* printf ("step 2... curID :%s:\n", me->lexer->curID); */ \
/* possibly a SFNodeish type value?? */ \
if (me->lexer->curID != NULL) { \
        /* printf ("parser_MF, curID was not null (it is %s) me %u lexer %u... lets just parse node\n",me->lexer->curID,me,me->lexer); */ \
        if (!parser_node(me, &RCX, ID_UNDEFINED)) { \
                return FALSE; \
        } \
        if (RCX == NULL) return FALSE; \
        /* so, we have a Multi_XX return val. (see Structs.h), have to get the info into a vrmlNodeT */ \
        stuffDEFUSE(ret, RCX, FIELDTYPE_MF##type); \
        return TRUE; \
 } \
/* Just a single value? */ \
/* NOTE: the XML parser will ALWAYS give this without the brackets */ \
if((!lexer_openSquare(me->lexer)) && (!(me->parsingX3DfromXML))) { \
        vrml##type##T RCXRet; \
        /* printf ("parser_MF, not an opensquare, lets just parse node\n");  */ \
        if(!parser_sf##name##Value(me, &RCXRet)) { \
                return FALSE; \
        } \
        /* printf ("after sf parse rcx %u\n",RCXRet); */ \
        /* RCX is the return value, if this value IN THE VRML FILE IS ZERO, then this valid parse will fail... */ \
        /* so it is commented out if (RCX == NULL) return FALSE; */ \
        /* so, we have a Multi_XX return val. (see Structs.h), have to get the info into a vrmlNodeT */ \
        stuffSFintoMF(ret, (vrmlNodeT *)&RCXRet, FIELDTYPE_MF##type); \
        return TRUE; \
} \
\
  /* Otherwise, a real vector */ \
  /* printf ("parser_MF, this is a real vector:%s:\n",me->lexer->nextIn); */ \
  vec=newVector(vrml##type##T, 128); \
  if (!me->parsingX3DfromXML) { \
        while(!lexer_closeSquare(me->lexer)) { \
                vrml##type##T val; \
                if(!parser_sf##name##Value(me, &val)) { \
                        CPARSE_ERROR_CURID("ERROR:Expected \"]\" before end of MF-Value") \
                         break; \
                } \
                vector_pushBack(vrml##type##T, vec, val); \
        } \
  } else { \
        lexer_skip(me->lexer); \
        while(*me->lexer->nextIn != '\0') { \
                vrml##type##T val; \
                if(!parser_sf##name##Value(me, &val)) { \
                        CPARSE_ERROR_CURID("ERROR:Expected \"]\" before end of MF-Value") \
                         break; \
                } \
                vector_pushBack(vrml##type##T, vec, val); \
                lexer_skip(me->lexer); \
        } \
  }\
  rv = (struct Multi_##type*) ret; \
  rv->n=vectorSize(vec); \
  rv->p=vector_releaseData(vrml##type##T, vec); \
  \
  deleteVector(vrml##type##T, vec); \
  return TRUE; \
 } 

    PARSER_MFFIELD(bool, Bool)
    PARSER_MFFIELD(color, Color)
    PARSER_MFFIELD(colorrgba, ColorRGBA)
    PARSER_MFFIELD(float, Float)
    PARSER_MFFIELD(int32, Int32)
    PARSER_MFFIELD(node, Node)
    PARSER_MFFIELD(rotation, Rotation)
    PARSER_MFFIELD(string, String)
    PARSER_MFFIELD(time, Time)
    PARSER_MFFIELD(vec2f, Vec2f)
    PARSER_MFFIELD(vec3f, Vec3f)
    PARSER_MFFIELD(vec3d, Vec3d)

/* ************************************************************************** */
/* SF* field values */

/* Parses a fixed-size vector-field of floats (SFColor, SFRotation, SFVecXf) */
#define PARSER_FIXED_VEC(name, type, cnt) \
 BOOL parser_sf##name##Value(struct VRMLParser* me, void* ret) \
 { \
  int i; \
  vrml##type##T *rv; \
  ASSERT(me->lexer); \
  rv = (vrml##type##T *) ret; \
  for(i=0; i!=cnt; ++i) {\
   if(!parser_sffloatValue(me, rv->c+i)) \
    return FALSE; \
  }\
  return TRUE; \
 }

/* Parses a fixed-size vector-field of floats (SFColor, SFRotation, SFVecXf) */
#define PARSER_FIXED_DOUBLE_VEC(name, type, cnt) \
 BOOL parser_sf##name##Value(struct VRMLParser* me, void* ret) \
 { \
  int i; \
  vrml##type##T *rv; \
  ASSERT(me->lexer); \
  rv = (vrml##type##T *) ret; \
  for(i=0; i!=cnt; ++i) {\
   if(!parser_sfdoubleValue_(me, rv->c+i)) \
    return FALSE; \
  }\
  return TRUE; \
 }

    BOOL parser_sfdoubleValue_(struct VRMLParser* me, vrmlDoubleT* ret)
    {
        return lexer_double(me->lexer, ret);
    }
static BOOL parser_sffloatValue_(struct VRMLParser* me, void* ret)
    {   
	vrmlFloatT *rf;
	rf = (vrmlFloatT*)ret;
	return lexer_float(me->lexer, rf);
    }
static BOOL parser_sfint32Value_(struct VRMLParser* me, void* ret)
    {
	vrmlInt32T* rf;
	rf = (vrmlInt32T*)ret;
        return lexer_int32(me->lexer, rf);
    }



static BOOL set_X3Dstring(struct VRMLLexer* me, vrmlStringT* ret) {
    /* printf ("lexer_X3DString, setting string to be :%s:\n",me->startOfStringPtr[me->lexerInputLevel]); */
    *ret=newASCIIString((char *)me->startOfStringPtr[me->lexerInputLevel]);
    return TRUE;
}

static BOOL parser_sfstringValue_(struct VRMLParser* me, void* ret) {
    vrmlStringT* rv;

    rv = (vrmlStringT*)ret;

    /* are we parsing the "classic VRML" formatted string? Ie, one with
       starting and finishing quotes? */
    if (!me->parsingX3DfromXML) return lexer_string(me->lexer, rv);

    else return set_X3Dstring(me->lexer, rv);
        
    return TRUE;
}

static BOOL parser_sfboolValue(struct VRMLParser* me, void* ret) {
    vrmlBoolT *rv;

    rv = (vrmlBoolT*)ret;

    /* are we in the VRML (x3dv) parser? */
    if (!me->parsingX3DfromXML) {
        if(lexer_keyword(me->lexer, KW_TRUE)) {
            *rv=TRUE;
            return TRUE;
        }
        if(lexer_keyword(me->lexer, KW_FALSE)) {
            *rv=FALSE;
            return TRUE;
        }
        return FALSE;
    }
    /* possibly, this is from the XML Parser */
    if (!strcmp(me->lexer->startOfStringPtr[me->lexer->lexerInputLevel],"true")) {
        *rv = TRUE;
        return TRUE;
    }
    if (!strcmp(me->lexer->startOfStringPtr[me->lexer->lexerInputLevel],"false")) {
        *rv = FALSE;
        return TRUE;
    }

    /* possibly this is from the XML parser, but there is a case problem */
    if (!gglobal()->internalc.global_strictParsing && (!strcmp(me->lexer->startOfStringPtr[me->lexer->lexerInputLevel],"TRUE"))) {
	CPARSE_ERROR_CURID("found upper case TRUE in XML file - should be lower case");
        *rv = TRUE;
        return TRUE;
    }
    if (!gglobal()->internalc.global_strictParsing && (!strcmp(me->lexer->startOfStringPtr[me->lexer->lexerInputLevel],"FALSE"))) {
	CPARSE_ERROR_CURID ("found upper case FALSE in XML file - should be lower case");
        *rv = FALSE;
        return TRUE;
    }


        
    /* Noperooni - this was from X3D, but did not parse */
    *rv = FALSE;
    return FALSE;
}

    PARSER_FIXED_VEC(color, Color, 3)
    PARSER_FIXED_VEC(colorrgba, ColorRGBA, 4)
    PARSER_FIXED_VEC(matrix3f, Matrix3f, 9)
    PARSER_FIXED_VEC(matrix4f, Matrix4f, 16)
    PARSER_FIXED_VEC(vec2f, Vec2f, 2)
    PARSER_FIXED_VEC(vec4f, Vec4f, 4)
    PARSER_FIXED_VEC(rotation, Rotation, 4)
    PARSER_FIXED_DOUBLE_VEC(vec2d, Vec2d, 2)
    PARSER_FIXED_DOUBLE_VEC(vec3d, Vec3d, 3)
    PARSER_FIXED_DOUBLE_VEC(vec4d, Vec4d, 4)
    PARSER_FIXED_DOUBLE_VEC(matrix3d, Matrix3d, 9)
    PARSER_FIXED_DOUBLE_VEC(matrix4d, Matrix4d, 16)

/* JAS this code assumes that the ret points to a SFInt_32 type, and just
   fills in the values. */
 
    static BOOL parser_sfimageValue(struct VRMLParser* me, void* ret)
    {
	vrmlImageT *rv;
        vrmlInt32T width, height, depth;
        vrmlInt32T* ptr;

	rv = (vrmlImageT*) ret;
 
        if(!lexer_int32(me->lexer, &width))
            return FALSE;
        if(!lexer_int32(me->lexer, &height))
            return FALSE;
        if(!lexer_int32(me->lexer, &depth))
            return FALSE;


        rv->n=3+width*height;
        rv->p=MALLOC(int *, sizeof(int) * rv->n);
        rv->p[0]=width;
        rv->p[1]=height;
        rv->p[2]=depth;

        for(ptr=rv->p+3; ptr!=rv->p+rv->n; ++ptr)
            if(!lexer_int32(me->lexer, ptr))
            {
                FREE_IF_NZ(rv->p);
                rv->n=0;
                return FALSE;
            }

        return TRUE;
    }


static BOOL parser_sfnodeValue(struct VRMLParser* me, void* ret) {
    uintptr_t tmp;
    vrmlNodeT* rv;

    ASSERT(me->lexer);
    rv = (vrmlNodeT*)ret;

    if(lexer_keyword(me->lexer, KW_NULL)) {
        *rv=NULL;
        return TRUE;
    }

    /* are we parsing from a proto expansion? */
    if (!me->parsingX3DfromXML) {
        return parser_nodeStatement(me, rv);
    } else {
        /* expect something like a number (memory pointer) to be here */
        if (sscanf(me->lexer->startOfStringPtr[me->lexer->lexerInputLevel], "%u",  &tmp) != 1) {
            CPARSE_ERROR_FIELDSTRING ("error finding SFNode id on line :%s:",me->lexer->startOfStringPtr[me->lexer->lexerInputLevel]);
            *rv=NULL;
            return FALSE;
        }
        *rv = (vrmlNodeT)tmp;
    }
    return TRUE;
}


static BOOL parser_sftimeValue(struct VRMLParser* me, void* ret)
    {
	vrmlTimeT *rv;
	rv = (vrmlTimeT*)ret;
        return lexer_double(me->lexer, rv);
    }


static BOOL parser_fieldTypeNotParsedYet(struct VRMLParser* me, void* ret) {
    CPARSE_ERROR_CURID ("received a request to parse a type not supported yet");
    return FALSE;
}


/* prettyprint this error */
	#define OUTLINELEN 	800
	#define FROMSRC		140
void cParseErrorCurID(struct VRMLParser *me, char *str) {
	char fw_outline[OUTLINELEN];
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;

	if (strlen(str) > FROMSRC) str[FROMSRC] = '\0';
	strcpy(fw_outline,str);
	if (me->lexer->curID != ((void *)0)) {
		strcat (fw_outline, "; current token :");
		strcat (fw_outline, me->lexer->curID); 
		strcat (fw_outline, ": ");
	}
	if (me->lexer->nextIn != NULL) {
		strcat (fw_outline," at: \"");
		strncat(fw_outline,me->lexer->nextIn,FROMSRC);
		if (strlen(me->lexer->nextIn) > FROMSRC)
			strcat (fw_outline,"...");
		strcat (fw_outline,"\"");
	}

	p->foundInputErrors++;
	ConsoleMessage(fw_outline); 
}

void cParseErrorFieldString(struct VRMLParser *me, char *str, const char *str2) {

	char fw_outline[OUTLINELEN];
	int str2len = (int) strlen(str2);
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;

	if (strlen(str) > FROMSRC) str[FROMSRC] = '\0';
	strcpy(fw_outline,str);
	strcat (fw_outline," (");
	strncat (fw_outline,str2,str2len);
	strcat (fw_outline, ") ");
	if (me->lexer->curID != ((void *)0)) strcat (fw_outline, me->lexer->curID); 
	if (me->lexer->nextIn != NULL) {
		strcat (fw_outline," at: \"");
		strncat(fw_outline,me->lexer->nextIn,FROMSRC);
		if (strlen(me->lexer->nextIn) > FROMSRC)
			strcat (fw_outline,"...");
		strcat (fw_outline,"\"");
	}

	p->foundInputErrors++;
	ConsoleMessage(fw_outline); 
}

//
//
// ******************** BROTO section *********************************************************
//
//

/*
Sept 2014
X3D_Proto is now being used for non-node entities as well as ProtoInstance:
a) protoInstance PI
b) protoDeclare PD
c) externProtoInstance EPI
d) externProtoDeclare EPD
e) sceneInstance SI (rootNodes parent, deep)
f) protoLibrary PL (scene declare, shallow)

WRL Parsing now uses the same code for both scene and protoBody, recursing as deep as needed,
and uses a 'depth' flag for deciding whether to instance what its parsing (ie for scene) or not
(protoDeclares, shallow).

deep - instancing a live scene, so nodes are registered, routes are registered, scripts are registered, PIs are deep copied
shallow - we are in a protoDeclare -perhaps in its body- so we just copy the interface of any contained PIs for routing,
	we do not register nodes, scripts, or do any binding

executionContext EC: scene or protoBody (according to specs), basically a name context, and where routes are supposed to be

X3D_Proto fields
	void * __DEFnames;	//besides giving def names to the parser, it also saves them in a Vector per-EC
	void * __IS;		//the IS keyword details are parsed to this, per EC, and a function generates browser ROUTES if deep
	void * __ROUTES;	//ROUTES are saved here per EC, as well as registered with browser when deep
	void * __afterPound;	//for EPD, if url is myfile.wrl#Geom then Geom is the afterPound, and is the name of the desired PD in the proto library
	void * __externProtoDeclares;	//besides giving EPD type names back to parser, the EPDs are stored here per-EC
	void * __loadResource;	//for network types, like EPD, this is the resource its monitoring that's downloading the PL
	int __loadstatus;	//for network types like EPD, helps EPD advance step-wise on each visit, as the resource is loaded and parsed
	struct X3D_Node *__parentProto; //parent EC, when instancing
	void * __protoDeclares;	//besides giving PD type name back to parser, the parsed PD is stored here per-EC
	void * __protoDef;	//struct for holding user fields, as used for Script, Proto
	int __protoFlags;	//char [4] 0) deep=1, shallow=0 1) 1 means useBrotos=0 old way 2) 0-declare 1-instance 2-scene 3) extern=1, else 0
	struct X3D_Node *__prototype;	//for PI, EPI: will be PD, EPD, so when deep_copying it can get the body
	void * __scripts;	//stores script nodes parsed here per EC as well as registering with browser when instancing
	void * __typename;	//for PD,EPD (and PI, EPI): besides giving PD user-defined type name (vs builtin type) back to parser, its stored here
	struct Multi_String __url;	//for network types like EPD - the parsed URL for downloading
	void * _parentResource; //for network types, like EPD, this is a resource_item_t * of the main scene, to get its absolute URL in resource_identify
	
	//familiar fields:
	struct Multi_Node _children;	//same use as children[] field in Group/Transform, except hidden as _children for some scenes t85.wrl that name a user field as children
	struct Multi_Node _sortedChildren;  
	struct Multi_Node addChildren;
	struct X3D_Node *metadata;
	struct Multi_Node removeChildren;
	struct SFVec3f bboxCenter;
	struct SFVec3f bboxSize;
*/



/* Parses a node (node non-terminal) */
/* Looks up the node type on the builtin NODES list and the userNodeNames list.  
   If this is a builtin node type, creates a new X3D_Node structure of the appropriate type for the node, 
   and then parses the statements for that node.  
   For each field statement, gets the value for that field and stores it in the X3D_Node structure.  
   For each ROUTE statement, adds the route to the CRoutes table.
   For each PROTO statement, adds the PROTO definition to the PROTOs list. 
   Return a pointer to the X3D_Node structure that holds the information for this node.
   If this is a user-defined node type (i.e. a PROTO expansion/instance), complete the proto expansion/instance  
   For each field in the ProtoDefinition either parse and propagate the specified value for this field, or 
   propagate the default value of the field.  (i.e. copy the appropriate value into every node/field combination in
   the dests list.)
   For each route in the routes list of the ProtoDefinition, add the route to the CRoutes table.
   Return a pointer to the X3D_Node structure that is the scenegraph for this PROTO.
*/
struct X3D_Proto *brotoInstance(struct X3D_Proto* proto, BOOL ideep);
static BOOL parser_field_user(struct VRMLParser* me, struct X3D_Node *node);
static BOOL parser_interfaceDeclarationB(struct VRMLParser* me, struct ProtoDefinition* proto, struct Shader_Script* script);
void deep_copy_broto_body2(struct X3D_Proto** proto, struct X3D_Proto** dest);
void initialize_one_script(struct Shader_Script* ss, const struct Multi_String *url);
static BOOL parser_externbrotoStatement(struct VRMLParser* me);
static BOOL parser_node_B(struct VRMLParser* me, vrmlNodeT* ret, int ind) {
	int nodeTypeB, nodeTypeU, isBroto;
	struct X3D_Node* node=NULL;
	struct X3D_Proto *currentContext;
	char pflagdepth;
	struct ProtoDefinition *thisProto = NULL;
	#ifdef HAVE_JAVASCRIPT
		struct Shader_Script* script=NULL;
	#endif
	struct Shader_Script* shader=NULL;
	struct X3D_Node* what_am_I = X3D_NODE(me->ptr);
	currentContext = (struct X3D_Proto*)me->ptr;
	pflagdepth = ciflag_get(currentContext->__protoFlags,0); //((char *)(&currentContext->__protoFlags))[0];

	DECLAREUP

	ASSERT(me->lexer);
	*ret=node; /* set this to NULL, for now... if this is a real node, it will return a node pointer */

	/* lexer_node( ... ) #defined to lexer_specialID(me, r1, r2, NODES, NODES_COUNT, userNodeTypesVec) where userNodeTypesVec is a list of PROTO defs */
	/* this will get the next token (which will be the node type) and search the NODES array for it.  If it is found in the NODES array nodeTypeB will be set to 
		the index of type in the NODES array.  If it is not in NODES, the list of user-defined nodes will be searched for the type.  If it is found in the user-defined 
		list nodeTypeU will be set to the index of the type in userNodeTypesVec.  A return value of FALSE indicates that the node type wasn't found in either list */

#ifdef CPARSERVERBOSE
	printf ("parser_node START, curID :%s: nextIn :%s:\n",me->lexer->curID, me->lexer->nextIn);
#endif


//#define XBLOCK_STATEMENT_B(LOCATION) 
	if(parser_routeStatement(me))  {
		return TRUE;
	}

	if (parser_componentStatement(me)) {
		return TRUE;
	}

	if (parser_exportStatement(me)) {
		return TRUE;
	}

	if (parser_importStatement(me)) {
		return TRUE;
	}

	if (parser_metaStatement(me)) {
		return TRUE;
	}

	if (parser_profileStatement(me)) {
		return TRUE;
	}
	if(parser_brotoStatement(me)) {
		return TRUE;
	}
	if(usingBrotos()) 
	if(parser_externbrotoStatement(me)) {
		return TRUE;
	}

	//if(parser_protoStatement(me)) { 
	//      return TRUE; 
	//} 

	//XBLOCK_STATEMENT_B(ddd)



	if(!lexer_node(me->lexer, &nodeTypeB, &nodeTypeU)) {
#ifdef CPARSERVERBOSE
		printf ("parser_node, not lexed - is this one of those special nodes?\n");
#endif
		return FALSE;
	}

	/* printf ("after lexer_node, at this point, me->lexer->curID :%s:\n",me->lexer->curID); */
	/* could this be a proto expansion?? */

	/* Checks that the next non-whitespace non-comment character is '{' and skips it. */
	if(!lexer_openCurly(me->lexer))
		PARSE_ERROR("Expected { after node-type id!")

#ifdef CPARSERVERBOSE
	printf ("parser_node: have nodeTypeB %d nodeTypeU %d\n",nodeTypeB, nodeTypeU);
#endif
	isBroto = FALSE;
	if (nodeTypeU != ID_UNDEFINED) {
		/* The node name was located in userNodeTypesVec (list of defined PROTOs), 
			therefore this is an attempt to instantiate a PROTO */
		/* expand this PROTO, put the code right in line, and let the parser
			go over it as if there was never a proto here... */
		struct X3D_Proto *proto; //, *currentContext;
		char *protoname = vector_get(char*, me->lexer->userNodeTypesVec, nodeTypeU);
		//currentContext = (struct X3D_Proto*)me->ptr;
		//BOOL isAvailableBroto(char *pname, struct X3D_Proto* currentContext, struct X3D_Proto **proto);
		//struct X3D_Proto *shallowBrotoInstance(X3D_Proto* proto);
		if( isAvailableBroto(protoname, currentContext , &proto))
		{
			/* its a binary proto, new in 2013 */
			int idepth = 0; //if its old brotos (2013) don't do depth until sceneInstance. If 2014 broto2, don't do depth here if we're in a protoDeclare or externProtoDeclare
			if(usingBrotos() ) idepth = pflagdepth == 1; //2014 broto2: if we're parsing a scene (or Inline) then deepcopy proto to instance it, else shallow
			node=X3D_NODE(brotoInstance(proto,idepth));
			//moved below, for all nodes if(idepth) add_parent(node,X3D_NODE(currentContext),__FILE__,__LINE__); //helps propagate VF_Sensitive to parent of proto, if proto's 1st node is sensor
			isBroto = TRUE;
			ASSERT(node);
			if (ind != ID_UNDEFINED) {
				/* Set the top memmber of the DEFed nodes stack to this node */
				vector_get(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes), ind)=node;
#ifdef CPARSERVERBOSE
				printf("parser_node: adding DEFed node (pointer %p) to DEFedNodes vector\n", node);  
#endif
			}
		}else{
			/* its a text proto */
			char *newProtoText = NULL;
	        
#ifdef CPARSERVERBOSE
			printf ("nodeTypeB = ID_UNDEFINED, but nodeTypeU is %d\n",nodeTypeU);
#endif
			/* If this is a PROTO instantiation, then there must be at least one PROTO defined.  
				Also, the index retrieved for  this PROTO must be valid. */
			ASSERT(nodeTypeU!=ID_UNDEFINED);
			ASSERT(me->PROTOs);
			ASSERT(nodeTypeU<vectorSize(me->PROTOs));

			if (me->curPROTO == NULL) {
			int newProtoTextLen = 0;

				/* printf ("curPROTO = NULL: before protoExpand, current stream :%s:\n",me->lexer->nextIn); */

				/* find and expand the PROTO definition */
				newProtoText = protoExpand(me, nodeTypeU,&thisProto,&newProtoTextLen);

				/* printf ("curPROTO = NULL: past protoExpand\n"); */
				parser_fromString(me,newProtoText);

				/* printf ("curPROTO = NULL: past insertProtoExpansionIntoStream\n"); */
				/* and, now, lets get the id for the lexer */
				if(!lexer_node(me->lexer, &nodeTypeB, &nodeTypeU)) {
					printf ("after protoexpand lexer_node, is this one of those special ones? \n");
					FREE_IF_NZ(newProtoText);
					return FALSE;
				}


				/* printf ("curPROTO = NULL: after possible proto expansion, me->lexer->curID :%s: ",me->lexer->curID);
					printf ("curPROTO = NULL: and remainder, me->lexer->nextIn :%s:\n",me->lexer->nextIn); */

				if(!lexer_openCurly(me->lexer))
					PARSE_ERROR("Expected { after node-type id!")
				} else {
					/* Checks that the next non-whitespace non-comment character is an openCurly and skips it. */
					if(!lexer_openCurly(me->lexer)) {
						PARSE_ERROR("Expected after node-type id!")
					} else {
						/* printf ("curPROTO != NULL; lets skip these fields\n"); */
						skipToEndOfOpenCurly(me->lexer,0);
						/* printf ("after skipToEndOfOpenCurly, we have :%s:\n",me->lexer->nextIn); */
					}
				}
			/* JAS - now done by stacked lexer FREE_IF_NZ(newProtoText); */
		}
	}

	/* Built-in node */
	/* Node was found in NODES array */
	if(nodeTypeB!=ID_UNDEFINED) {
#ifdef CPARSERVERBOSE
		 printf("parser_node: parsing builtin node\n");
#endif

	//#ifdef HAVE_JAVASCRIPT
 //       struct Shader_Script* script=NULL;
	//#endif

		//struct Shader_Script* shader=NULL;

		/* Get malloced struct of appropriate X3D_Node type with default values filled in */
		if(pflagdepth){
			node=X3D_NODE(createNewX3DNode((int)nodeTypeB)); //registers node types like sensors, textures in tables for scene
			if(node->_nodeType == NODE_Inline){
				if(X3D_NODE(me->ptr)->_nodeType != NODE_Inline && X3D_NODE(me->ptr)->_nodeType != NODE_Proto)
					printf("ouch trying to caste a %d nodetype to inline or proto\n",X3D_NODE(me->ptr)->_nodeType);
				X3D_INLINE(node)->__parentProto = me->ptr;
			}
		}else
			node=X3D_NODE(createNewX3DNode0((int)nodeTypeB)); //doesn't register node types in tables, for protoDeclare
		ASSERT(node);

		/* if ind != ID_UNDEFINED, we have the first node of a DEF. Save this node pointer, in case
			some code uses it. eg: DEF xx Transform {children Script {field yy USE xx}} */
		if (ind != ID_UNDEFINED) {
			/* Set the top memmber of the DEFed nodes stack to this node */
			vector_get(struct X3D_Node*, stack_top(struct Vector*, me->DEFedNodes), ind)=node;
#ifdef CPARSERVERBOSE
			printf("parser_node: adding DEFed node (pointer %p) to DEFedNodes vector\n", node);  
#endif
		}

		/* Node specific initialization */
		/* From what I can tell, this only does something for Script nodes.  It sets node->__scriptObj to new_Shader_Script() */
		parser_specificInitNode_B(node, me);

		/* Set flag for Shaders/Scripts - these ones can have any number of fields */
		switch (node->_nodeType) {
			#ifdef HAVE_JAVASCRIPT
			case NODE_Script: script=X3D_SCRIPT(node)->__scriptObj; break;
			#endif
			case NODE_ShaderProgram: shader=(struct Shader_Script *)(X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields); break;
			case NODE_PackagedShader: shader=(struct Shader_Script *)(X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields); break;
			case NODE_ComposedShader: shader=(struct Shader_Script *)(X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields); break;
			default: {}
		}
	} /*endif nodetypeB*/

	/* As long as the lexer is returning field statements, ROUTE statements, 
		or PROTO statements continue parsing node */

	/*	to get IS handling in the main scene parsing code including for Script user fields
		I split up the script interface parser so it only creates the field, then
		BACKUPs the lexer and lets field_user and found_IS take a crack at it
	*/
	if( (nodeTypeB!=ID_UNDEFINED) || isBroto)
	{
		#define SPLIT_SCRIPTUSERFIELD_CREATION_FROM_VALUEPARSING 1
		while(TRUE)
		{
			/* Try to parse the next statement as a field.  For normal "field value" statements 
				(i.e. position 1 0 1) this gets the value of the field from the lexer (next token(s)
				to be processed) and stores it as the appropriate type in the node.
				For IS statements (i.e. position IS mypos) this adds the node-field combo 
				(as an offsetPointer) to the dests list for the protoFieldDecl associated with the user
				defined field (in the given case, this would be mypos).  */
#ifdef CPARSERVERBOSE
			printf("parser_node: try parsing field ... \n");
#endif
			/* check for IS - can be any mode, and builtin or user field on builtin node or usernode/protoInstance */
			if( found_IS_field(me,node) ){
				continue;
			}

			if(SPLIT_SCRIPTUSERFIELD_CREATION_FROM_VALUEPARSING)
			if(parser_field_user(me,node)) {
				continue;
			}

			/*check for builtin field value on builtin node or usernode/protoInstance*/
			if(parser_field(me, node)) {
#ifdef CPARSERVERBOSE
				printf("parser_node: field parsed\n");
#endif
				continue;
			}


			/* Try to parse the next statement as a ROUTE (i.e. statement starts with ROUTE).  This checks that the ROUTE statement is valid (i.e. that the referenced node and field combinations  
				exist, and that they are compatible) and then adds the route to either the CRoutes table of routes, or adds a new ProtoRoute structure to the vector 
				ProtoDefinition->routes if we are parsing a PROTO */
#ifdef CPARSERVERBOSE
			printf("parser_node: try parsing ROUTE ... \n");
#endif

			/* try ROUTE, COMPONENT, EXPORT, IMPORT, META, PROFILE statements here */
			SAVEUP
			BLOCK_STATEMENT(parser_node);

			/* Try to parse the next statement as a PROTO (i.e. statement starts with PROTO).  */
			/* Add the PROTO name to the userNodeTypesVec list of names.  Create and fill in a new protoDefinition structure and add it to the PROTOs list.
				Goes through the interface declarations for the PROTO and adds each user-defined field name to the appropriate list of user-defined names (user_initializeOnly, 
				user_inputOnly, Out, or user_inputOutput), creates a new protoFieldDecl for the field and adds it to the iface vector of the ProtoDefinition, 
				and, in the case of fields and inputOutputs, gets the default value of the field and stores it in the protoFieldDecl. 
				Parses the body of the PROTO.  Nodes are added to the scene graph for this PROTO.  Routes are parsed and a new ProtoRoute structure
				is created for each one and added to the routes vector of the ProtoDefinition.  PROTOs are recursively parsed!
			*/
#ifdef CPARSERVERBOSE
			printf("parser_node: try parsing PROTO ... \n");
#endif
			BACKUP

			if(parser_protoStatement(me)) {
#ifdef CPARSERVERBOSE
				printf("parser_node: PROTO parsed\n");
#endif
				continue;
			}

			if(parser_brotoStatement(me)) {
#ifdef CPARSERVERBOSE
				printf("parser_vrmlScene: BROTO parsed\n");
#endif
				continue;
			}

#ifdef CPARSERVERBOSE
			printf("parser_node: try parsing Script or Shader field\n");
#endif

			#ifdef HAVE_JAVASCRIPT
			/* check for user field declaration on builtin node of type script or shaderprogram
				'mode fieldtype fieldname <fieldvalue>'
				and create the field
				aside: protoDeclares and protoInstances handled elsewhere:
					- protoInstance 'fieldname fieldvalue' are handled like builtins in found_IS and parser_field
					- protoDeclare  'mode fieldtype fieldname <fieldvalue>' are handled in parser_protostatement
			*/
			if(SPLIT_SCRIPTUSERFIELD_CREATION_FROM_VALUEPARSING) //this one just adds the field, and leave it to others to parse the 'fieldname fieldvalue'
			if(script && parser_interfaceDeclarationB(me, NULL, script)){
				continue;
			}
			if(!SPLIT_SCRIPTUSERFIELD_CREATION_FROM_VALUEPARSING)
            if(script && parser_interfaceDeclaration(me, NULL, script)) {
#ifdef CPARSERVERBOSE
				printf("parser_node: SCRIPT field parsed\n");
#endif
				continue;
			}

			#endif

			if(shader && parser_interfaceDeclaration(me, NULL, shader)) {
#ifdef CPARSERVERBOSE
				printf("parser_node: Shader field parsed\n");
#endif
				continue;
			}

			break;
		}

		#ifdef HAVE_JAVASCRIPT
		/* Init code for Scripts */
		if(script) {
#ifdef CPARSERVERBOSE
			printf("parser_node: try parsing SCRIPT url\n");
#endif
			if(pflagdepth) //broto1: do this later in sceneInstance broto2: do it during instancing here and brotoInstance
				//script_initCodeFromMFUri(script, &X3D_SCRIPT(node)->url);
				initialize_one_script(script,&X3D_SCRIPT(node)->url);
#ifdef CPARSERVERBOSE
			printf("parser_node: SCRIPT url parsed\n");
#endif
		} /* nodetypeB or brotoInstance */
		#endif /* HAVE_JAVASCRIPT */

		if(isBroto && pflagdepth){
			//copying the body _after_ the protoInstance field values have been parsed 
			//allows ISd fields in body nodes to get the pkw_initializeOnly/inputOutput value
			//from the protoInstance interface
			deep_copy_broto_body2(&X3D_PROTO(X3D_PROTO(node)->__prototype),&X3D_PROTO(node));
		}
		// if(pflagdepth) add_parent(node,parent??,__FILE__,__LINE__); //helps propagate VF_Sensitive to parent of proto, if proto's 1st node is sensor. We do in macro PROCESS_FIELD_B

		/* We must have a node that we've parsed at this point. */
		ASSERT(node);
	}

	/* Check that the node is closed by a '}', and skip this token */
#ifdef CPARSERVERBOSE
	printf ("calling lexer_closeCurly at B\n");
#endif

	if(!lexer_closeCurly(me->lexer)) {
		CPARSE_ERROR_CURID("ERROR: Expected a closing brace after fields of a node;")
		PARSER_FINALLY;
		return FALSE; 
	}

	/* Return the parsed node */

	#ifdef CPARSERVERBOSE
	printf ("returning at end of parser_node, ret %u\n",node);
	if (node != NULL) printf ("and, node type is %s\n",stringNodeType(node->_nodeType));
	#endif
	*ret=node;
	return TRUE;
}


//#include "broto1.h"
/*  mode fieldtype fieldname <fieldValue>
    Goal: just create the field and let other functions parse fieldvalue
		read the first 3 tokens, Quality Assure them.
		create the field, iniitalize to bzero
		add the field to proto/script field list
		if mode is field (not event) backup lexer to fieldname
		exit 
		(and other functions will parse and set fieldValue)
*/
static BOOL parser_interfaceDeclarationB(struct VRMLParser* me, struct ProtoDefinition* proto, struct Shader_Script* script) {
    int mode;
    int type;
    int name;
	DECLAREUP
    union anyVrml defaultVal;
    struct ProtoFieldDecl* pdecl=NULL;
    //struct ProtoFieldDecl* pField=NULL;
    struct ScriptFieldDecl* sdecl=NULL;
    //char *startOfField = NULL;
    //int startOfFieldLexerLevel = INT_ID_UNDEFINED;


#ifdef CPARSERVERBOSE
    printf ("start of parser_interfaceDeclaration\n");
#endif


    /* Either PROTO or Script interface! */
    ASSERT((proto || script) && !(proto && script));

    /* lexer_protoFieldMode is #defined as 
	   lexer_specialID(me, r, NULL, PROTOKEYWORDS, PROTOKEYWORDS_COUNT, NULL) */
    /* Looks for the next token in the array PROTOKEYWORDS (inputOnly, outputOnly, inputOutput, field) 
	   and returns the appropriate index in mode */
	SAVEUP
    if(!lexer_protoFieldMode(me->lexer, &mode)) {
#ifdef CPARSERVERBOSE
        printf ("parser_interfaceDeclaration, not lexer_protoFieldMode, returning\n");
#endif
		BACKUP
        return FALSE;
    }

    /* Script can not take inputOutputs */
   if(0) // change in post-DUK era - we allow inputoutputs now
    if (script != NULL) {
		if(script->ShaderScriptNode->_nodeType==NODE_Script && mode==PKW_inputOutput)
		{
			PARSE_ERROR("Scripts must not have inputOutputs!")
			//printf("dug9: maybe scripts can have inputOutputs\n");
		}
    }
  
    /* lexer_fieldType is #defined as lexer_specialID(me, r, NULL, FIELDTYPES, FIELDTYPES_COUNT, NULL) */
    /* Looks for the next token in the array FIELDTYPES and returns the index in type */
    if(!lexer_fieldType(me->lexer, &type))
		PARSE_ERROR("Expected fieldType after proto-field keyword!")

#ifdef CPARSERVERBOSE
    printf ("parser_interfaceDeclaration, switching on mode %s\n",PROTOKEYWORDS[mode]);
#endif

	//save fieldname - if field (not event) we'll back up here when we exit
	//so something else can parse IS or fieldValue
	SAVEUP 

    switch(mode)
    {
#define LEX_DEFINE_FIELDID(suff) \
   case PKW_##suff: \
    if(!lexer_define_##suff(me->lexer, &name)) \
     PARSE_ERROR("Expected fieldNameId after field type!") \
    break;

        LEX_DEFINE_FIELDID(initializeOnly)
            LEX_DEFINE_FIELDID(inputOnly)
            LEX_DEFINE_FIELDID(outputOnly)
            LEX_DEFINE_FIELDID(inputOutput)



#ifndef NDEBUG
   default:
        ASSERT(FALSE);
#endif
    }

    /* If we are parsing a PROTO, create a new  protoFieldDecl.
       If we are parsing a Script, create a new scriptFieldDecl. */
    if(proto) {
#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, calling newProtoFieldDecl\n");
#endif

		pdecl=newProtoFieldDecl(mode, type, name);
		//pdecl->fieldString = STRDUP(lexer_stringUser_fieldName(me->lexer, name, mode));

#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, finished calling newProtoFieldDecl\n");
#endif
    } else {
#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, calling newScriptFieldDecl\n");
#endif
		//lexer_stringUser_fieldName(me,name,mod)
		sdecl=newScriptFieldDecl(me->lexer, mode, type, name);
		//sdecl=newScriptFieldDecl(lexer_stringUser_fieldName(me->lexer,name,mod), mode, type, name);
		//sdecl->fieldString = STRDUP(lexer_stringUser_fieldName(me->lexer, name, mode));

    }

 
    /* If this is a field or an exposed field give it a bzero default value*/ 
    if(mode==PKW_initializeOnly || mode==PKW_inputOutput) { 
#ifdef CPARSERVERBOSE
		printf ("parser_interfaceDeclaration, mode==PKW_initializeOnly || mode==PKW_inputOutput\n");
#endif
		bzero (&defaultVal, sizeof (union anyVrml));

		/* Store the default field value in the protoFieldDeclaration or scriptFieldDecl structure */
		if(proto) {
			pdecl->defaultVal=defaultVal;
		}
		else
		{
			ASSERT(script);
			//defaultVal.sfcolor.c[1] = .333;
			scriptFieldDecl_setFieldValue(sdecl, defaultVal);
		}
    } 

    /* Add the new field declaration to the list of fields in the Proto or Script definition.
       For a PROTO, this means adding it to the iface vector of the ProtoDefinition. 
       For a Script, this means adding it to the fields vector of the ScriptDefinition. */ 
    if(proto) {
		/* protoDefinition_addIfaceField is #defined as vector_pushBack(struct ProtoFieldDecl*, (me)->iface, field) */
		/* Add the protoFieldDecl structure to the iface vector of the protoDefinition structure */
		protoDefinition_addIfaceField(proto, pdecl);
	} else {
        /* Add the scriptFieldDecl structure to the fields vector of the Script structure */
        ASSERT(script);
		vector_pushBack(struct ScriptFieldDecl*, script->fields, sdecl);
        //script_addField(script, sdecl); //this also registered the field. We'll do that during sceneInstance
    }

	#ifdef CPARSERVERBOSE
	printf ("end of parser_interfaceDeclaration\n");
	#endif
	//if(mode == PKW_initializeOnly || mode == PKW_inputOutput)
		BACKUP //backup so something else parses 'fieldname <fieldvalue>' or 'fieldname IS protofieldname'
		//if it's just fieldname -it's an event with no IS- parser_field_user will detect and just swallow the fieldname
	//else
	//{
	//	FREE_IF_NZ(me->lexer->curID); //event - swallow fieldname
	//}
    return TRUE;
}

BOOL find_anyfield_by_name(struct VRMLLexer* lexer, struct X3D_Node* node, union anyVrml **anyptr, int *imode, int *itype, char* nodeFieldName, int *isource, void **fdecl, int *ifield);
void scriptFieldDecl_jsFieldInit(struct ScriptFieldDecl* me, int num);

static BOOL parser_field_user(struct VRMLParser* me, struct X3D_Node *node) {
    int mode;
    int type;
	//int user;
	int source;
	int ifield;
	char *nodeFieldName;
	DECLAREUP
	//struct ProtoDefinition* proto;
	//struct Shader_Script* shader;
	union anyVrml *targetVal;
	void *fdecl;
    //struct ProtoFieldDecl* pdecl=NULL;
    //struct ProtoFieldDecl* pField=NULL;
    //struct ScriptFieldDecl* sdecl=NULL;


#ifdef CPARSERVERBOSE
    printf ("start of parser_field_user\n");
#endif


    /* Either PROTO or Script interface! */
    //ASSERT((proto || script) && !(proto && script));

	//get the fieldname
	SAVEUP //save the lexer spot so if it's not a 'fieldname <fieldValue>' we can backup
	/* get nodeFieldName */
	if(!lexer_setCurID(me->lexer)) return FALSE;
	ASSERT(me->lexer->curID);
	nodeFieldName = STRDUP(me->lexer->curID);
	//BACKUP;
	FREE_IF_NZ(me->lexer->curID);

	//retrieve field mode, type
	targetVal = NULL;
	if(!find_anyfield_by_name(me->lexer,node,&targetVal,&mode,&type,nodeFieldName,&source,&fdecl,&ifield)){
		BACKUP
		return FALSE; //couldn't find field in user or builtin fields anywhere
	}
	if(source < 1){
		BACKUP
		return FALSE; //we don't want builtins -handled elsewhere- just user fields
	}

    /* If this is a field or an exposed field */ 
    if(mode==PKW_initializeOnly || mode==PKW_inputOutput) { 
#ifdef CPARSERVERBOSE
		printf ("parser_field_user, mode==PKW_initializeOnly || mode==PKW_inputOutput\n");
#endif

		/* set the defaultVal to something - we might have a problem if the parser expects this to be
		a MF*, and there is "garbage" in there, as it will expect to free it. */
		//bzero (&defaultVal, sizeof (union anyVrml)); //default val pulled from existing field default

		if (!parseType(me, type, targetVal)) {
			/* Invalid default value parsed.  Delete the proto or script declaration. */
			CPARSE_ERROR_CURID("Expected default value for field!");
			//if(pdecl) deleteProtoFieldDecl(pdecl);
			//if(sdecl) deleteScriptFieldDecl(sdecl);
			return FALSE;
		}
		if(source==3){
			//externProtoDeclares don't set an initial value, yet externProtoInstances copy the declare fields, even if junk or null
			// (versus regular protoDeclares which must set initial field values on inputOutput/initializeOnly)
			//so the alreadySet flag is to say that the EPI had a value set here, and when 
			//filling out a late-ariving externprotodefinition EPD, only EPI fields that were initialzed are copied to the child PI
			//-see load_externProtoInstance() 
			struct X3D_Proto *pnode = X3D_PROTO(node);
			//I could filter and just do extern proto, but lazy, so do any proto
			struct ProtoDefinition *pd = pnode->__protoDef;
			struct ProtoFieldDecl * pf = protoDefinition_getFieldByNum(pd, ifield);
			pf->alreadySet = 1;
		}
		if(source==1)
		{
			//((struct ScriptFieldDecl*) fdecl)->valueSet=TRUE;
			//((struct ScriptFieldDecl*) fdecl)->value = *targetVal;
			//X3D_SCRIPT(node)->__scriptObj
			//if(0) //don't do this during parsing, do it during scene instancing.
			//scriptFieldDecl_jsFieldInit(
			//	(struct ScriptFieldDecl*) fdecl, 
			//	((struct Shader_Script*)X3D_SCRIPT(node)->__scriptObj)->num);
		}
		//printf("n=%f\n",targetVal->sfcolor.c[0]);
		///* Store the default field value in the protoFieldDeclaration or scriptFieldDecl structure */
		//if(proto) {
		//	pdecl->defaultVal=defaultVal;
		//}
		//else
		//{
		//	ASSERT(shader);
		//	scriptFieldDecl_setFieldValue(sdecl, defaultVal);
		//}
    } 
	#ifdef CPARSERVERBOSE
	printf ("end of parser_user_field\n");
	#endif
	FREEUP
    return TRUE;
}

/* PROTO keyword handling. 
   Like PROTO above: parses ProtoDeclare Interface
   Unlike PROTO above: parses the ProtoDeclare Body like a mini-scene, 
   through re-entrant/recursive call to the same function that parses the main scene
   So a SceneDeclare is-a ProtoDeclare and vice versa.
*/
static BOOL parser_brotoStatement(struct VRMLParser* me)
{
    int name;
    struct ProtoDefinition* obj;
    //char *startOfBody;
    //char *endOfBody;
    //char *initCP;
    //uintptr_t bodyLen;
	struct X3D_Proto *proto, *parent;
	void *ptr;
	DECLAREUP
	unsigned int ofs;


    /* Really a PROTO? */
	SAVEUP
    if(!lexer_keyword(me->lexer, KW_PROTO)) //KW_BROTO))
	{
		BACKUP
        return FALSE;
	}
	
    /* Our name */
    /* lexer_defineNodeType is #defined as lexer_defineID(me, ret, userNodeTypesVec, FALSE) */
    /* Add the PROTO name to the userNodeTypesVec list of names return the index of the name in the list in name */ 
    if(!lexer_defineNodeType(me->lexer, &name))
        PARSE_ERROR("Expected nodeTypeId after PROTO!\n")
    ASSERT(name!=ID_UNDEFINED);

    /* Create a new blank ProtoDefinition structure to contain the data for this PROTO */
    obj=newProtoDefinition();

    /* save the name, if we can get it - it will be the last name on the list, because we will have JUST parsed it. */
    if (vectorSize(me->lexer->userNodeTypesVec) != ID_UNDEFINED) {
	obj->protoName = STRDUP(vector_get(const char*, me->lexer->userNodeTypesVec, vectorSize(me->lexer->userNodeTypesVec)-1));
    } else {
	printf ("warning - have proto but no name, so just copying a default string in\n");
	obj->protoName = STRDUP("noProtoNameDefined");
    }

	#ifdef CPARSERVERBOSE
	printf ("parser_protoStatement, working on proto :%s:\n",obj->protoName);
	#endif

    /* If the PROTOs stack has not yet been created, create it */
    if(!me->PROTOs) {
        parser_scopeIn_PROTO(me);
    }

    ASSERT(me->PROTOs);
    /*  ASSERT(name==vectorSize(me->PROTOs)); */

    /* Add the empty ProtoDefinition structure we just created onto the PROTOs stack */
    vector_pushBack(struct ProtoDefinition*, me->PROTOs, obj);
 
    /* Now we want to fill in the information in the ProtoDefinition */

    /* Interface declarations */

    /* Make sure that the next token is a '['.  Skip over it. */
    if(!lexer_openSquare(me->lexer))
        PARSE_ERROR("Expected [ to start interface declaration!")

            /* Read the next line and parse it as an interface declaration. */
            /* Add the user-defined field name to the appropriate list of user-defined names (user_initializeOnly, user_inputOnly, Out, or user_inputOutput).
               Create a new protoFieldDecl for this field and add it to the iface vector for the ProtoDefinition obj.
               For fields and inputOutputs, get the default value of the field and store it in the protoFieldDecl. */
            while(parser_interfaceDeclaration(me, obj, NULL));

    /* Make sure that the next token is a ']'.  Skip over it. */
    if(!lexer_closeSquare(me->lexer))
        PARSE_ERROR("Expected ] after interface declaration!")

	//pseudocode:
	//proto = new Proto() //off scenegraph storage please
	//proto.__protoDef = obj
	//contextParent.declared_protos.add(proto);
	//parser_proto_body(proto)
	//return NULL; //no scenegraph node created, or more precisely: nothing to link in to parent's children
	
	//create a ProtoDeclare
    proto = createNewX3DNode0(NODE_Proto);
	//add it to the current context's list of declared protos
	if(X3D_NODE(me->ptr)->_nodeType != NODE_Proto && X3D_NODE(me->ptr)->_nodeType != NODE_Inline )
		printf("ouch trying to caste node type %d to proto\n",X3D_NODE(me->ptr)->_nodeType);
	parent = (struct X3D_Proto*)me->ptr;
	if(parent->__protoDeclares == NULL)
		parent->__protoDeclares = newVector(struct X3D_Proto*,4);
	vector_pushBack(struct X3D_Proto*,parent->__protoDeclares,proto);


	proto->__parentProto = X3D_NODE(parent); //me->ptr; //link back to parent proto, for isAvailableProto search
	proto->__protoFlags = parent->__protoFlags;
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,0); //((char*)(&proto->__protoFlags))[0] = 0; //shallow instancing of protoInstances inside a protoDeclare 
	///[1] leave parent's the oldway flag if set
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,2); //((char*)(&proto->__protoFlags))[2] = 0; //this is a protoDeclare we are parsing
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,3); //((char*)(&proto->__protoFlags))[3] = 0; //not an externProtoDeclare
	//set ProtoDefinition *obj
	proto->__protoDef = obj;
	proto->__prototype = X3D_NODE(proto); //point to self, so shallow and deep instances will inherit this value
	proto->__typename = strdup(obj->protoName);

    /* PROTO body */
    /* Make sure that the next oken is a '{'.  Skip over it. */
    if(!lexer_openCurly(me->lexer))
        PARSE_ERROR("Expected { to start PROTO body!")

     /* record the start of this proto body - keep the text around */
    //startOfBody = (char *) me->lexer->nextIn;
    //initCP = (char *) me->lexer->startOfStringPtr[me->lexer->lexerInputLevel];

    /* Create a new vector of nodes and push it onto the DEFedNodes stack */
    /* This is the list of nodes defined for this scope */
    /* Also checks that the PROTOs vector exists, and creates it if it does not */
    parser_scopeIn(me);

    /* Parse body */
    {

#ifdef CPARSERVERBOSE
        printf ("about to parse PROTO body; new proto def %p\n",obj);
#endif

        //me->curPROTO=obj;
		ptr = me->ptr;
		ofs = me->ofs; //Q. does this change? Or are we always ofs of children in Proto? H: parseFromString different
		me->ptr = proto;
		me->ofs = offsetof(struct X3D_Proto, __children);
		parse_proto_body(me);
		me->ptr = ptr;
		me->ofs = ofs;

        /* We are done parsing this proto.  Set the curPROTO to the last proto we were parsing. */
       // me->curPROTO=oldCurPROTO;
    }
    if(!lexer_closeCurly(me->lexer))
		PARSE_ERROR("Expected } to end PROTO body!")

    /* Takes the top DEFedNodes vector off of the stack.  The local scope now refers to the next vector in the DEFedNodes stack */
    parser_scopeOut(me);

    /* Make sure that the next token is a '}'.  Skip over it. */
#ifdef CPARSERVERBOSE
    printf ("calling lexer_closeCurly at A\n");
printf ("parser_protoStatement, FINISHED proto :%s:\n",obj->protoName);
#endif
	FREEUP
    return TRUE;
}

/* EXTERNPROTO keyword handling. 
   Like PROTO above: parses ExternProtoDeclare Interface as a X3D_Proto
   - with __url and resource for fetching the definition
*/
#define LOAD_INITIAL_STATE 0
static BOOL parser_externbrotoStatement(struct VRMLParser* me)
{
    int name;
    struct ProtoDefinition* obj;
    //char *startOfBody;
    //char *endOfBody;
    //char *initCP;
    //uintptr_t bodyLen;
	struct X3D_Proto *proto, *parent;
	void *ptr;
	DECLAREUP
	unsigned int ofs;


    /* Really a EXTERNPROTO? */
	SAVEUP
    if(!lexer_keyword(me->lexer, KW_EXTERNPROTO)) 
	{
		BACKUP
        return FALSE;
	}
	
    /* Our name */
    /* lexer_defineNodeType is #defined as lexer_defineID(me, ret, userNodeTypesVec, FALSE) */
    /* Add the EXTERNPROTO name to the userNodeTypesVec list of names return the index of the name in the list in name */ 
    if(!lexer_defineNodeType(me->lexer, &name))
        PARSE_ERROR("Expected nodeTypeId after EXTERNPROTO!\n")
    ASSERT(name!=ID_UNDEFINED);

    /* Create a new blank ProtoDefinition structure to contain the data for this EXTERNPROTO */
    obj=newProtoDefinition();
	obj->isExtern = TRUE;
    /* save the name, if we can get it - it will be the last name on the list, because we will have JUST parsed it. */
    if (vectorSize(me->lexer->userNodeTypesVec) != ID_UNDEFINED) {
	obj->protoName = STRDUP(vector_get(const char*, me->lexer->userNodeTypesVec, vectorSize(me->lexer->userNodeTypesVec)-1));
    } else {
	printf ("warning - have proto but no name, so just copying a default string in\n");
	obj->protoName = STRDUP("noProtoNameDefined");
    }

	#ifdef CPARSERVERBOSE
	printf ("parser_protoStatement, working on proto :%s:\n",obj->protoName);
	#endif

    /* If the PROTOs stack has not yet been created, create it */
    if(!me->PROTOs) {
        parser_scopeIn_PROTO(me);
    }

    ASSERT(me->PROTOs);
    /*  ASSERT(name==vectorSize(me->PROTOs)); */

    /* Add the empty ProtoDefinition structure we just created onto the PROTOs stack */
    vector_pushBack(struct ProtoDefinition*, me->PROTOs, obj);
 
    /* Now we want to fill in the information in the ProtoDefinition */

    /* Interface declarations */

    /* Make sure that the next token is a '['.  Skip over it. */
    if(!lexer_openSquare(me->lexer))
        PARSE_ERROR("Expected [ to start interface declaration!")

            /* Read the next line and parse it as an interface declaration. */
            /* Add the user-defined field name to the appropriate list of user-defined names (user_initializeOnly, user_inputOnly, Out, or user_inputOutput).
               Create a new protoFieldDecl for this field and add it to the iface vector for the ProtoDefinition obj.
               For fields and inputOutputs, get the default value of the field and store it in the protoFieldDecl. */
            while(parser_interfaceDeclaration(me, obj, NULL));

    /* Make sure that the next token is a ']'.  Skip over it. */
    if(!lexer_closeSquare(me->lexer))
        PARSE_ERROR("Expected ] after interface declaration!")

	//pseudocode:
	//proto = new Proto() //off scenegraph storage please
	//proto.__protoDef = obj
	//contextParent.declared_protos.add(proto);
	//parser_proto_body(proto)
	//return NULL; //no scenegraph node created, or more precisely: nothing to link in to parent's children
	
	//create a ProtoDeclare
    proto = createNewX3DNode0(NODE_Proto);
	//add it to the current context's list of declared protos
	parent = (struct X3D_Proto*)me->ptr;
	if(parent->__externProtoDeclares == NULL)
		parent->__externProtoDeclares = newVector(struct X3D_Proto*,4);
	vector_pushBack(struct X3D_Proto*,parent->__externProtoDeclares,proto);


	proto->__parentProto = X3D_NODE(parent); //me->ptr; //link back to parent proto, for isAvailableProto search
	proto->__protoFlags = parent->__protoFlags;
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,0); //((char*)(&proto->__protoFlags))[0] = 0; //shallow instancing of protoInstances inside a protoDeclare 
	///[1] leave parent's the oldway flag if set
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,2); //((char*)(&proto->__protoFlags))[2] = 0; //this is a protoDeclare we are parsing
	proto->__protoFlags = ciflag_set(proto->__protoFlags,1,3); //((char*)(&proto->__protoFlags))[3] = 1; //an externProtoDeclare
	//set ProtoDefinition *obj
	proto->__protoDef = obj;
	proto->__prototype = X3D_NODE(proto); //point to self, so shallow and deep instances will inherit this value
	proto->__typename = (void *)strdup(obj->protoName);

	/* EXTERNPROTO url */
	{
		struct Multi_String url;
		unsigned char *buffer;
		char *pound;
		resource_item_t *res;

		/* get the URL string */
		if (!parser_mfstringValue(me,&proto->url)) {
			PARSE_ERROR ("EXTERNPROTO - problem reading URL string");
		}
		proto->__loadstatus = LOAD_INITIAL_STATE;
		/*the rest is done in load_externProto during rendering*/
	}

	FREEUP
    return TRUE;
}

/*Q. could/should brotoRoutes resolve to pointers early during parsing (as they are now) 
	or late (by storing char* DEFNode, char* fieldname)? 
	- late might help with Inline IMPORT/EXPORT, where routes are declared
		before the nodes appear.
*/
struct brotoRoute
{
	struct X3D_Node* fromNode;
	int fromOfs;
	struct X3D_Node* toNode;
	int toOfs;
	int ft;
};
void broto_store_route(struct X3D_Proto* proto,
                          struct X3D_Node* fromNode, int fromOfs,
                          struct X3D_Node* toNode, int toOfs,
                          int ft)
{
	Stack* routes;
	struct brotoRoute* route;
	//struct X3D_Proto* protoDeclare = (struct X3D_Proto*)me->ptr;
    ASSERT(proto);
	if ((fromOfs == ID_UNDEFINED) || (toOfs == ID_UNDEFINED)) {
		ConsoleMessage ("problem registering route - either fromField or toField invalid");
		return;
	}

	route = MALLOC(struct brotoRoute*,sizeof(struct brotoRoute));
	route->fromNode = fromNode;
	route->fromOfs = fromOfs;
	route->toNode = toNode;
	route->toOfs = toOfs;
	route->ft = ft;

	routes = proto->__ROUTES;
	if( routes == NULL){
		routes = newStack(struct brotoRoute *);
		proto->__ROUTES = routes;
	}
	stack_push(struct brotoRoute *, routes, route);
	return;
}

//BOOL route_parse_nodefield(pre, eventType)
//used by parser_routeStatement:
BOOL route_parse_nodefield(struct VRMLParser* me, int *NodeIndex, struct X3D_Node** Node, int KW_eventType, 
						   int *Ofs, int *fieldType, struct ScriptFieldDecl** ScriptField)
{
	int PKW_eventType = PKW_outputOnly;
	char *cerror1;

	int mode;
	int type;
	int source;
	int ifield; //, iprotofield;
	char *nodeFieldName;
	//DECLAREUP
	int foundField;
	union anyVrml *fieldPtr;
	void *fdecl = NULL;

	*Ofs = 0;
	//Node = NULL; 
	*ScriptField=NULL; 
	cerror1 = "";

	if(KW_eventType == KW_outputOnly){
		PKW_eventType = PKW_outputOnly;
		cerror1 = "Expected an event of type : outputOnly :";
	}else if(KW_eventType == KW_inputOnly)  {
		PKW_eventType = PKW_inputOnly;
		cerror1 = "Expected an event of type : inputOnly :";
	}

	/* Target-node */ 

	/* Look for the current token in the userNodeNames vector (DEFed names) */ 
	if(!lexer_nodeName(me->lexer, NodeIndex)) { 
		/* The current token is not a valid DEFed name.  Error. */ 
		CPARSE_ERROR_CURID("ERROR:ROUTE: Expected a valid DEF name; found \""); 
		PARSER_FINALLY;  
		return FALSE; 
	} 


	/* Check that there are DEFedNodes in the DEFedNodes vector, and that the index given for this node is valid */ 
	ASSERT(me->DEFedNodes && !stack_empty(me->DEFedNodes) && 
	NodeIndex<vectorSize(stack_top(struct Vector*, me->DEFedNodes))); 
	/* Get the X3D_Node structure for the DEFed node we just looked up in the userNodeNames list */ 
	*Node=vector_get(struct X3D_Node*, 
	stack_top(struct Vector*, me->DEFedNodes), 
	*NodeIndex); 
	/* We were'nt able to get the X3D_Node structure for the DEFed node.  Error. */ 
	if (*Node == NULL) { 
		/* we had a bracket underflow, from what I can see. JAS */ 
		CPARSE_ERROR_CURID("ERROR:ROUTE: no DEF name found - check scoping and \"}\"s"); 
		PARSER_FINALLY;  
		return FALSE; 
	} 


	/* The next character has to be a '.' - skip over it */ 
	if(!lexer_point(me->lexer)) {
		CPARSE_ERROR_CURID("ERROR:ROUTE: Expected \".\" after the NODE name") 
		PARSER_FINALLY;  
		return FALSE;  
	} 
	//SAVEUP  //save the lexer spot so if it's not an IS we can back up
	/* get nodeFieldName */
	if(!lexer_setCurID(me->lexer)) return FALSE;
	ASSERT(me->lexer->curID);
	nodeFieldName = STRDUP(me->lexer->curID);
	//BACKUP;
	FREE_IF_NZ(me->lexer->curID);

	fieldPtr = NULL;
	foundField = find_anyfield_by_nameAndRouteDir(me->lexer, *Node, &fieldPtr, &mode, &type, 
		nodeFieldName, &source, &fdecl, &ifield, PKW_eventType);
	if(foundField)
	{
		if(source == 0)
			*Ofs = NODE_OFFSETS[(*Node)->_nodeType][ifield*5 + 1];
		else
			*Ofs = ifield;
		*ScriptField = fdecl;
		*fieldType = type;
		return TRUE;
	}
	if((*Node)->_nodeType==NODE_Script && !fdecl) {
 			PARSE_ERROR("Event-field invalid for this PROTO/Script!") 
	} else { 
			PARSE_ERROR(cerror1) 
	} 
	FREE_IF_NZ(nodeFieldName);
	PARSER_FINALLY;  
	return FALSE;  
}


// modified by dug9 feb6, 2013
// I had trouble following the old_ version, but 
// the final output - a route to register - seemed simple.
// so it seemed like a lot of the old one was quality checking
static BOOL parser_routeStatement_B(struct VRMLParser* me)
{
	struct X3D_Node* fromNode;
	int fromOfs;
	int fromType;
	int fromNodeIndex;
	struct ScriptFieldDecl* fromScriptField;

	struct X3D_Node* toNode;
	int toOfs;
	int toType;
	int toNodeIndex;
	struct ScriptFieldDecl* toScriptField;
	ppCParseParser p = (ppCParseParser)gglobal()->CParseParser.prv;

	fromOfs = 0;
	fromType = 0;
	toOfs = 0;
	toType = 0;
	toNode = NULL; fromNode = NULL; 
	fromScriptField=NULL; toScriptField=NULL;

	ASSERT(me->lexer);
	lexer_skip(me->lexer);

	/* Is this a routeStatement? */
	if(!lexer_keyword(me->lexer, KW_ROUTE))
        return FALSE;

    /* Parse the elements. */

    /* Parse the first part of a routing statement: DEFEDNODE.event by locating the node DEFEDNODE in either the builtin or user-defined name arrays
       and locating the event in the builtin or user-defined event name arrays */
    //ROUTE_PARSE_NODEFIELD(from, outputOnly);
	if(!route_parse_nodefield(me,&fromNodeIndex,&fromNode,KW_outputOnly,&fromOfs,
		&fromType,&fromScriptField)) return FALSE;

        /* Next token has to be "TO" */
        if(!lexer_keyword(me->lexer, KW_TO)) {
            /* try to make a better error message. */
			char *buf = p->fw_outline;
            strcpy (buf,"ERROR:ROUTE: Expected \"TO\" found \"");
            if (me->lexer->curID != NULL) strcat (buf, me->lexer->curID); else strcat (buf, "(EOF)");
            strcat (buf,"\" ");
            if (fromNode != NULL) { strcat (buf, " from type:"); strcat (buf, stringNodeType(fromNode->_nodeType)); strcat (buf, " "); }
            //if (fromFieldE != ID_UNDEFINED) { strcat (buf, ":"); strcat (buf, EXPOSED_FIELD[fromFieldE]); strcat (buf, " "); }
            //if (fromFieldO != ID_UNDEFINED) { strcat (buf, ":"); strcat (buf, EVENT_OUT[fromFieldO]); strcat (buf, " "); }

            /* PARSE_ERROR("Expected TO in ROUTE-statement!") */
            CPARSE_ERROR_CURID(buf); 
            PARSER_FINALLY; 
	    return FALSE; 
        }
/* Parse the second part of a routing statement: DEFEDNODE.event by locating the node DEFEDNODE in either the builtin or user-defined name arrays 
   and locating the event in the builtin or user-defined event name arrays */
	//ROUTE_PARSE_NODEFIELD(to, inputOnly);
	if(!route_parse_nodefield(me,&toNodeIndex,&toNode,KW_inputOnly,&toOfs,
		&toType,&toScriptField)) return FALSE;

    /* We can only ROUTE between two equivalent fields.  If the size of one field value is different from the size of the other, we have problems (i.e. can't route SFInt to MFNode) */
    if(fromType!=toType) {
        /* try to make a better error message. */
		char *buf = p->fw_outline;
        strcpy (buf,"ERROR:Types mismatch in ROUTE: ");
        if (fromNode != NULL) { strcat (buf, " from type:"); strcat(buf, stringNodeType(fromNode->_nodeType)); strcat (p->fw_outline, " "); }
        //if (fromFieldE != ID_UNDEFINED) { strcat (buf, ":"); strcat(buf, EXPOSED_FIELD[fromFieldE]); strcat (p->fw_outline, " "); }
        //if (fromFieldO != ID_UNDEFINED) { strcat (buf, ":"); strcat(buf, EVENT_OUT[fromFieldO]); strcat (p->fw_outline, " "); }

        if (toNode != NULL) { strcat (buf, " to type:"); strcat(buf, stringNodeType(toNode->_nodeType)); strcat (buf, " "); }
        //if (toFieldE != ID_UNDEFINED) { strcat(buf, ":"); strcat(buf, EXPOSED_FIELD[toFieldE]); strcat (buf, " "); }
        //if (toFieldO != ID_UNDEFINED) { strcat(buf, ":"); strcat(buf, EVENT_IN[toFieldO]); strcat (buf, " "); }

        /* PARSE_ERROR(fw_outline) */
        CPARSE_ERROR_CURID(buf); 
        PARSER_FINALLY; 
		return FALSE; 
    }

    /* Finally, register the route. */
    /* **************************** */

    /* Built-in to built-in */
	int pflags = ((struct X3D_Proto*)(me->ptr))->__protoFlags;
	char oldwayflag = ciflag_get(pflags,1); //((char *)&pflags)[1];
	char instancingflag = ciflag_get(pflags,0);//((char *)&pflags)[0];
	if(oldwayflag || instancingflag)
		parser_registerRoute(me, fromNode, fromOfs, toNode, toOfs, toType); //old way direct registration
	//else
	broto_store_route((struct X3D_Proto*)me->ptr,fromNode,fromOfs,toNode,toOfs,toType); //new way delay until sceneInstance()

    return TRUE;
}

/*
	Binary Protos aka Brotos - allows deeply nested protos, by using scene parsing code to parse protobodies 
		recursively, as Flux2008 likely does
	History: 
		Jan 2013, Broto1- did first version for .wrl parsing, but it unrolled the results into the main scene tables
		 (Routes, scripts, etc) via function sceneInstance() - and didn't do .x3d parsing, nor externProtos
		Sept 2014 Broto2 - attempt to render directly from Broto-format scene, and fix externProtos
	Concepts:
		deep vs shallow: when instancing a prototype, you go deep if you copy the protoDeclare body 
			to the body of the protoInstance, and recursively deepen any contained protoInstances. 
			You go shallow if you just copy the interface (so you can route to it) leaving the body empty. Related to proto expansion.
		Scene as Proto - to allow the parser to use the same code to parse the scene and protobodies -recursing-
			the scene and protos need to have a common format. Once parsed, Broto1 converted the SceneProto to old tables and structs. 
			Broto2 is rendering the SceneProto directly, saving awkward conversion, and requiring rendering
			algorithms that recurse
		ExternProtoDeclare/instance - design goal: have externProtoDeclare wrap/contain a protoDeclare,
			and instance the protodeclare as its first node once loaded, and have an algorithm that ISes 
			between the externProtoDeclare interface and the contained protoInstance fields. This 'wrapper' design 
			allows flexibility in ordering of fields, and can do minor PKW mode conversions, and allow the parser
			to continue parsing the scene while the externproto definition downloads and parses asynchronously
		p2p - pointer2pointer node* lookup table when copying a binary protoDeclare to a protoInstance: 
			tables in the declare -such as routes and DEFs- are in terms of the Declare node*, and after new nodoes
			are created for the protoInstance, the protoDeclare-to-protoInstance node* lookup can be done during
			copying of the tables
	Broto2: 5 things share an X3D_Proto node structure:
		1. ProtoInstance - the only one of these 5 that's shown as a node type in web3d.org specs
		2. ProtoDeclare - we don't register the struct as a node when mallocing, and we don't expand (deepen) its contained protoInstances
		3. ExternProtoInstance - in the specs, this would be just another ProtoInstance.
		4. ExternProtoDeclare - will be like ProtoDeclare, with a URL and a way to watch for it's protodefinition being loaded, like Inline
		5. Scene - so that parsing can parse protoDeclares and Scene using the same code, we use the same struct. 
			- Broto1 parsed the scene shallow, then in a second step sceneInstance() deepened the brotos while converting to old scene format
			- Broto2 parses deep for scenes and inlines, shallow for protodeclares and externProtodeclares
	To keep these 5 distinguished,
		char *pflags = (char *)(int* &__protoFlags)
		pflag[0]: deep parsing instruction
			1= parse deep: deepen any protoInstances recursively, and register nodes when mallocing (for scene, inline)
			0= parse shallow: instance protos with no body, don't register nodes during mallocing (for protoDeclare, externProtoDeclare)
		pflag[1]: oldway parsing instruction
			1= oldway, for where to send routes etc
			0= broto1, broto2 way
		pflag[2]: declare, instance, or scene object
			0 = protoDeclare or externProtoDeclare  //shouldn't be rendered
			1 = ProtoInstance or externProtoInstance //first child node is in the render transform stack
			2 = scene //all child nodes are rendered
		pflag[3]: extern or intern object
			0 = scene, protodeclare, protoInstance
			1 = externProtoInstance, externprotodeclare
*/

struct brotoDefpair{
	struct X3D_Node* node;
	char* name;
};
void broto_store_DEF(struct X3D_Proto* proto,struct X3D_Node* node, char *name)
{
	Stack *defs;
	struct brotoDefpair *def = MALLOC(struct brotoDefpair*,sizeof(struct brotoDefpair));
	def->node = node;
	def->name = STRDUP(name); //I don't know if the lexer clears its arrays after parsing, so DUP here.
	defs = proto->__DEFnames;
	if( defs == NULL)
	{
		defs = newStack(struct brotoDefpair *);
		proto->__DEFnames = defs;
	}
	stack_push(struct brotoDefpair*, defs, def);
}

BOOL isAvailableBroto(char *pname, struct X3D_Proto* currentContext, struct X3D_Proto **proto)
{
	/*	search list of already-defined binary protos in current context, 
		and in ancestor proto contexts*/
	int i;
    struct ProtoDefinition* obj;
	struct X3D_Proto *p;
	struct X3D_Proto* context;
	//struct Multi_Node *plist;
	struct Vector *plist;

	*proto = NULL;
	/* besides current context list also search parent context list if there is one */
	context = currentContext;
	do {
		int j;
		//flux,vivaty search top-down, cortona,blaxxun,white_dune search bottom-up within context,
		// this only makes a difference if you have more than one protodefinition with the same protoName
		// in the same context
		BOOL bottomUp = TRUE; 
		//plist = &context->__protoDeclares;
		plist = (struct Vector*) context->__protoDeclares;
		if(plist){
			int n = vectorSize(plist);
			for(i=0;i<n;i++)
			{
				j = i;
				if(bottomUp) j = n - 1 - i;
				//p = (struct X3D_Proto*)plist->p[j];
				p = vector_get(struct X3D_Proto*,plist,j);
				obj = p->__protoDef;
				if(!strcmp(obj->protoName,pname))
				{
					*proto = p;
					return TRUE;
				}
			}
		}
		plist = (struct Vector*) context->__externProtoDeclares;
		if(plist){
			int n = vectorSize(plist);
			for(i=0;i<n;i++)
			{
				j = i;
				if(bottomUp) j = n - 1 - i;
				//p = (struct X3D_Proto*)plist->p[j];
				p = vector_get(struct X3D_Proto*,plist,j);
				obj = p->__protoDef;
				if(!strcmp(obj->protoName,pname))
				{
					*proto = p;
					return TRUE;
				}
			}
		}
		context = (struct X3D_Proto*)context->__parentProto;
	}while(context);
	printf("ouch no broto definition found\n");
	return FALSE;
}
struct pointer2pointer{
	struct X3D_Node* pp;  //old or protoDeclare pointer
	struct X3D_Node* pn;  //new or protoInstance pointer
};

struct X3D_Node* inPointerTable(struct X3D_Node* source,struct Vector *p2p)
{
	int i;
	struct X3D_Node *dest = NULL;
	struct pointer2pointer *pair;
	for(i=0;i<p2p->n;i++)
	{
		pair = vector_get(struct pointer2pointer*, p2p, i);
		if(pair->pp == source){
			dest = pair->pn;
			break;
		}
	}
	return dest;
}
struct X3D_Node *p2p_lookup(struct X3D_Node *pnode, struct Vector *p2p);
void copy_routes2(Stack *routes, struct X3D_Proto* target, struct Vector *p2p)
{
	//for 2014 broto2, deep copying of brotodeclare to brotoinstance
	int i;
	struct brotoRoute *route;
	struct X3D_Node *fromNode, *toNode;
	if(routes == NULL) return;
	for(i=0;i<routes->n;i++)
	{
		route = vector_get(struct brotoRoute*, routes, i);
		//parser_registerRoute(me, fromNode, fromOfs, toNode, toOfs, toType); //old way direct registration
		//broto_store_route(me,fromNode,fromOfs,toNode,toOfs,toType); //new way delay until sceneInstance()
		fromNode = p2p_lookup(route->fromNode,p2p);
		toNode = p2p_lookup(route->toNode,p2p);
       	CRoutes_RegisterSimple(fromNode, route->fromOfs, toNode, route->toOfs, route->ft);
		//we'll also store in the deep broto instance, although they aren't used there (yet), and
		//if target is the main scene, they are abandoned. Maybe someday they'll be used.
		//if( target )
		broto_store_route(target,fromNode,route->fromOfs, toNode, route->toOfs, route->ft); 
	}
}
//copy broto defnames to single global scene defnames, for node* to defname lookup in parser_getNameFromNode
//but can't go the other way (name to node) because there can be duplicate nodes with the same name in
//different contexts
//this version for 2014 broto2
void copy_defnames2(Stack *defnames, struct X3D_Proto* target, struct Vector *p2p)
{
	Stack* defs;
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;

	defs = globalParser->brotoDEFedNodes;
	if( defs == NULL)
	{
		defs = newStack(struct brotoDefpair *);
		globalParser->brotoDEFedNodes = defs;
	}
	if(target->__DEFnames == NULL)
		target->__DEFnames = newStack(struct brotoDefpair *);
	if(defnames)
	{
		int i,n;
		struct brotoDefpair* def, *def2;
		n = vectorSize(defnames);
		for(i=0;i<n;i++){
			def = vector_get(struct brotoDefpair*,defnames,i);
			def2 = MALLOC(struct brotoDefpair*,sizeof(struct brotoDefpair));
			def2->name = def->name; //I wonder who owns this name
			def2->node = p2p_lookup(def->node, p2p);
			stack_push(struct brotoDefpair*, defs, def2);
			stack_push(struct brotoDefpair*, target->__DEFnames, def2); //added for broto2
		}
	}
}
void copy_IS(Stack *istable, struct X3D_Proto* target, struct Vector *p2p);
void copy_IStable(Stack **sourceIS, Stack** destIS);
void copy_field(int typeIndex, union anyVrml* source, union anyVrml* dest, struct Vector *p2p, 
				Stack *instancedScripts, struct X3D_Proto *ctx, struct X3D_Node *parent);
void initialize_scripts(Stack *instancedScripts);
void deep_copy_broto_body2(struct X3D_Proto** proto, struct X3D_Proto** dest)
{
	//for use with 2014 broto2 when parsing scene/inline and we want to deep-instance brotos as we parse
	//converts from binary proto/broto format to old scene format:
	//  - ROUTES are registered in global ROUTE registry
	//  - nodes are instanced and registered in memoryTable for startOfLoopNodesUpdate and killNode access
	//  - sensors, viewpoints etc are registered
	//proto - broto instance with 
	//  - a pointer __prototype to its generic prototype
	//dest - protoinstance with user fields filled out for this instance already 
	//     - children/body not filled out yet - it's done here
	//     - any field/exposedField values in interface IS copied to body node fields
	//     - broto ROUTES registered in global route registry, with this instances' node* addresses
	//     ? ( sensors, viewpoints will be directly registered in global/main scene structs)
	//what will appear different in the scene:
	//  old: PROTO instances appear as Group nodes with metaSF nodes for interface routing, mangled DEFnames
	//  new: PROTO instances will appear as X3DProto nodes with userfields for interface routing, local DEFnames

	//DEEP COPYING start here
	//we're instancing for the final scene, so go deep
	//0. setup pointer lookup table proto to instance
	//1. copy nodes, recursing on MFNode,SFnode fields
	//   to copy, instance a new node of the same type and save (new pointer, old pointer) in lookup table
	//   iterate over proto's node's fields, copying, and recursing on MFNode, SFNode
	//   if node is a ProtoInstance, deepcopy it
	//2. copy ROUTE table, looking up new_pointers in pointer lookup table
	//3. copy IS table, looking up new_pointers in pointer lookup table
	//4. copy DEFname table, looking up new pointers in pointer lookup table
	//

	struct X3D_Proto *prototype, *p;
	struct X3D_Node *parent;
	Stack *instancedScripts;
	struct Vector *p2p = newVector(struct pointer2pointer*,10);
	
	//2. copy body from source's _prototype.children to dest.children, ISing initialvalues as we go
	p=(*dest);
	p->__children.n = 0;
	p->__children.p = NULL;
	parent = (struct X3D_Node*) (*dest); //NULL;
	prototype = (struct X3D_Proto*)(*proto)->__prototype;

	p->__prototype = X3D_NODE(prototype);
	p->__protoFlags = prototype->__protoFlags;
	p->__protoFlags = ciflag_set(p->__protoFlags,1,2); //deep instancing of protoInstances inside a protoDeclare 

	//prototype = (struct X3D_Proto*)p->__prototype;
	//2.c) copy IS
	//p->__IS = copy IStable from prototype, and the targetNode* pointer will be wrong until p2p
	//copy_IStable(&((Stack*)prototype->__IS), &((Stack*)p->__IS));

	copy_IStable((Stack **) &(prototype->__IS), (Stack **) &(p->__IS));

	instancedScripts = (*dest)->__scripts;
	if( instancedScripts == NULL)
	{
		instancedScripts = newStack(struct X3D_Node *);
		(*dest)->__scripts = instancedScripts;
	}

	//2.a) copy rootnodes
	copy_field(FIELDTYPE_MFNode,(union anyVrml*)&(prototype->__children),(union anyVrml*)&(p->__children),
		p2p,instancedScripts,p,parent);
	//2.b) copy routes
	copy_routes2(prototype->__ROUTES, p, p2p);
	//2.d) copy defnames
	copy_defnames2(prototype->__DEFnames, p, p2p);

	////3. convert IS events to backward routes - maybe not for broto3, which might use the IS table in the (yet to be developed) routing algo
	copy_IS(p->__IS, p, p2p);

	initialize_scripts(instancedScripts);

	//*dest = p;
	//free p2p
	return;
}
struct X3D_Proto *brotoInstance(struct X3D_Proto* proto, BOOL ideep)
{
	//shallow copy - just the user-fields, and point back to the *prototype for later 
	//   deep copy of body and IS-table (2014 broto2 when parsing a protoDeclare or externProtoDeclare)
	//deep copy - copy body and tables, and if an item in the body is a protoInstance, deep copy it (recursively) 
	//	(2014 broto2 when parsing a scene or inline)

	int i;
    //int iProtoDeclarationLevel;
    struct ProtoDefinition *pobj,*nobj;
	struct ProtoFieldDecl *pdecl,*ndecl;
	struct X3D_Proto *p;
	if(ideep){
		p = createNewX3DNode(NODE_Proto);
		//memcpy(p,proto,sizeof(struct X3D_Proto)); //dangerous, make sure you re-instance all pointer variables
		p->__children.n = 0; //don't copy children in here - see below
		p->__children.p = NULL;
		int pflags = 0;
		//char pflags[4];
		pflags = ciflag_set(pflags,1,0); //pflags[0] = 1; //deep
		//pflags[1] = 0; //new way/brotos
		pflags = ciflag_set(pflags,1,2); //pflags[2] = 1; //this is a protoInstance
		pflags = ciflag_set(pflags,0,3); //pflags[3] = 0; //not an extern
		if(ciflag_get(proto->__protoFlags,3)==1) 
			pflags = ciflag_set(pflags,1,3); //its an externProtoInstance
		//memcpy(&p->__protoFlags,pflags,sizeof(int));
		p->__protoFlags = pflags;
	}else{
		//shallow
		p = createNewX3DNode0(NODE_Proto);
		//memcpy(p,proto,sizeof(struct X3D_Proto)); //dangerous, make sure you re-instance all pointer variables
		p->__children.n = 0; //don't copy children in here.
		p->__children.p = NULL;
		//char pflags[4];
		//pflags[0] = 0; //shallow
		//pflags[1] = 0; //new way/brotos
		//pflags[2] = 0; //this is a protoDeclare if shallow
		//pflags[3] = 0; //not an extern
		//memcpy(&p->__protoFlags,pflags,sizeof(int));
		p->__protoFlags = 0;
	}
	//memcpy(p,proto,sizeof(struct X3D_Proto)); //dangerous, make sure you re-instance all pointer variables
	p->__prototype = proto->__prototype;
	p->_nodeType = proto->_nodeType;
	pobj = proto->__protoDef;
	if(pobj){ //Prodcon doesn't bother mallocing this for scene nRn
		nobj = MALLOC(struct ProtoDefinition*,sizeof(struct ProtoDefinition));
		memcpy(nobj,pobj,sizeof(struct ProtoDefinition));
		nobj->iface = newVector(struct ProtoFieldDecl *, pobj->iface->n);
		for(i=0;i<pobj->iface->n;i++)
		{
			pdecl = protoDefinition_getFieldByNum(pobj, i);
 			ndecl=newProtoFieldDecl(pdecl->mode, pdecl->type, pdecl->name);
			//memcpy(ndecl,pdecl,sizeof(struct ProtoFieldDecl *)); //not just the pointer
			memcpy(ndecl,pdecl,sizeof(struct ProtoFieldDecl));  //.. the whole struct
			protoDefinition_addIfaceField(nobj, ndecl);
		}
		p->__protoDef = nobj;
	}
	//if(0) if(ideep)  moved to after field parsing, so ISing of initial values on the ProtoInstance get into the body
	//	deep_copy_broto_body2(&proto,&p);
	return p;
}
struct X3D_Node *p2p_lookup(struct X3D_Node *pnode, struct Vector *p2p)
{
	int i;
	struct pointer2pointer* pair;
	for(i=0;i<p2p->n;i++)
	{
		pair = vector_get(struct pointer2pointer*, p2p, i);
		if(pnode == pair->pp) return pair->pn;
	}
	return NULL;
}
//copy broto routes to old-style global scene routes
void copy_routes(Stack *routes, struct X3D_Proto* target, struct Vector *p2p)
{
	int i;
	struct brotoRoute *route;
	struct X3D_Node *fromNode, *toNode;
	if(routes == NULL) return;
	for(i=0;i<routes->n;i++)
	{
		route = vector_get(struct brotoRoute*, routes, i);
		//parser_registerRoute(me, fromNode, fromOfs, toNode, toOfs, toType); //old way direct registration
		//broto_store_route(me,fromNode,fromOfs,toNode,toOfs,toType); //new way delay until sceneInstance()
		fromNode = p2p_lookup(route->fromNode,p2p);
		toNode = p2p_lookup(route->toNode,p2p);
       	CRoutes_RegisterSimple(fromNode, route->fromOfs, toNode, route->toOfs, route->ft);
		//we'll also store in the deep broto instance, although they aren't used there (yet), and
		//if target is the main scene, they are abandoned. Maybe someday they'll be used.
		//if( target )
		//	broto_store_route(target,fromNode,route->fromOfs, toNode, route->toOfs, route->ft); 
	}
}
//struct ISrecord {
//	int protoFieldIndex;
//	int nodeFieldSource; //target node field source: builtin=0, script user field=1, broto user field =2
//	int nodeFieldIndexOrOffset; //int OFFSET for builtin fields, int field index for user fields in script, broto
//	struct X3D_Node* node; //target node
//};
struct brotoIS
{
	struct X3D_Proto *proto;
	char *protofieldname;
	int pmode;
	int iprotofield;
	int type;
	struct X3D_Node *node;
	char* nodefieldname;
	int mode;
	int ifield;
	int source; //0= builtin field, 1=script, 2={ComposedShader,ShaderProgram,PackagedShader} 3=Proto
};

//copy broto IS to old-style global scene routes
void copy_IS(Stack *istable, struct X3D_Proto* target, struct Vector *p2p)
{
	int i;
	struct brotoIS *is;
	struct X3D_Node *node, *pnode;
	if(istable == NULL) return;
	for(i=0;i<istable->n;i++)
	{
		int ifield, iprotofield;
		is = vector_get(struct brotoIS*, istable, i);
		//parser_registerRoute(me, fromNode, fromOfs, toNode, toOfs, toType); //old way direct registration
		//broto_store_route(me,fromNode,fromOfs,toNode,toOfs,toType); //new way delay until sceneInstance()
		node = p2p_lookup(is->node,p2p);
		pnode = X3D_NODE(target);
		ifield = is->ifield;
		if(node->_nodeType != NODE_Script && node->_nodeType != NODE_Proto)
			ifield = NODE_OFFSETS[node->_nodeType][ifield*5 +1];
		iprotofield = is->iprotofield;
		//if(pnode->_nodeType != NODE_Script && pnode->_nodeType != NODE_Proto)
		//	iprotofield = NODE_OFFSETS[node->_nodeType][offset*5 +1];
		if(is->pmode == PKW_outputOnly){ //we should use pmode instead of mode, because pmode is more restrictive, so we don't route from pmode initializeOnly (which causes cycles in 10.wrl)
			//idir = 0;
			//if(node->_nodeType == NODE_Script) idir = FROM_SCRIPT;
			 CRoutes_RegisterSimple(node, ifield, pnode, iprotofield, 0);

		}else if(is->pmode == PKW_inputOnly){
			CRoutes_RegisterSimple(pnode, iprotofield, node, ifield, 0);
		}else if(is->pmode == PKW_inputOutput){
			CRoutes_RegisterSimple(node, ifield, pnode, iprotofield, 0);
			CRoutes_RegisterSimple(pnode, iprotofield, node, ifield, 0);
		}else{
			//initialize Only - nothing to do routing wise
		}
	}
}
void copy_IStable(Stack **sourceIS, Stack** destIS)
{
	int i;
	if(*sourceIS){
		struct brotoIS *iss, *isd;
		*destIS = newStack(struct brotoIS); 

		for(i=0;i<(*sourceIS)->n;i++)
		{
			isd = MALLOC(struct brotoIS*,sizeof(struct brotoIS));
			iss = vector_get(struct brotoIS*,*sourceIS,i);
			(*isd) = (*iss); //deep copy struct brotoIS?
			stack_push(struct brotoIS*, *destIS, isd);
		}
	}
}
struct brotoIS * in_IStable(struct X3D_Node *target, int ifield, Stack *IS, int source)
{
	int i;
	struct Vector* IStable = (struct Vector*)IS;
	if(IStable){
		for(i=0;i<IStable->n;i++)
		{
			struct brotoIS * record = vector_get(struct brotoIS*,IStable,i);
			if(target == record->node){
				if(ifield == record->ifield && source == record->source){
					//field isource: 0=builtin 1=script user field 2=shader_program user field 3=Proto/Broto user field 4=group __protoDef
					return record;
				}
			}
		}
	}
	return NULL;
}
//copy broto defnames to single global scene defnames, for node* to defname lookup in parser_getNameFromNode
//but can't go the other way (name to node) because there can be duplicate nodes with the same name in
//different contexts
void copy_defnames(Stack *defnames, struct X3D_Proto* target, struct Vector *p2p)
{
	Stack* defs;
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;

	defs = globalParser->brotoDEFedNodes;
	if( defs == NULL)
	{
		defs = newStack(struct brotoDefpair *);
		globalParser->brotoDEFedNodes = defs;
	}
	if(defnames)
	{
		int i,n;
		struct brotoDefpair* def, *def2;
		n = vectorSize(defnames);
		for(i=0;i<n;i++){
			def = vector_get(struct brotoDefpair*,defnames,i);
			def2 = MALLOC(struct brotoDefpair*,sizeof(struct brotoDefpair));
			def2->name = def->name; //I wonder who owns this name
			def2->node = p2p_lookup(def->node, p2p);
			stack_push(struct brotoDefpair*, defs, def2);
		}
	}
}
char *broto_getNameFromNode(struct X3D_Node* node)
{
	char *ret;
	Stack* defs;
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;
	ret = NULL;
	defs = globalParser->brotoDEFedNodes;
	if(defs){
		int i,n;
		struct brotoDefpair* def;
		n = vectorSize(defs);
		for(i=0;i<n;i++){
			def = vector_get(struct brotoDefpair*,defs,i);
			if(def->node == node){
				ret = def->name; 
				break;
			}
		}
	}
	return ret;
}
void deep_copy_node(struct X3D_Node** source, struct X3D_Node** dest, struct Vector *p2p, 
					Stack *instancedScripts, struct X3D_Proto *ctx);
//void deep_copy_broto(struct X3D_Proto** proto, struct X3D_Proto** dest, Stack *instancedScripts);

void copy_field(int typeIndex, union anyVrml* source, union anyVrml* dest, struct Vector *p2p, 
				Stack *instancedScripts, struct X3D_Proto *ctx, struct X3D_Node *parent)
{
	int i, isize;
	int sftype, isMF;
	struct Multi_Node *mfs,*mfd;

	isMF = typeIndex % 2;  //this is wrong - you need a functional lookup or re-arrange the #defines
	sftype = typeIndex - isMF;
	//from EAI_C_CommonFunctions.c
	isize = returnElementLength(sftype) * returnElementRowSize(sftype);
	if(isMF)
	{
		int nele;
		char *ps, *pd;
		mfs = (struct Multi_Node*)source;
		mfd = (struct Multi_Node*)dest;
		//we need to malloc and do more copying
		nele = mfs->n;
		if( sftype == FIELDTYPE_SFNode ) nele = (int) upper_power_of_two(nele);
		mfd->p = MALLOC (struct X3D_Node **, isize*nele);
		mfd->n = mfs->n;
		ps = (char *)mfs->p;
		pd = (char *)mfd->p;
		for(i=0;i<mfs->n;i++)
		{
			copy_field(sftype,(union anyVrml*)ps,(union anyVrml*)pd,p2p,instancedScripts,ctx,parent);
			ps += isize;
			pd += isize;
		}

	}else{ 
		//isSF
		switch(typeIndex)
		{
			case FIELDTYPE_SFNode:
				{
					if(source->sfnode){ 
						deep_copy_node(&source->sfnode,&dest->sfnode,p2p,instancedScripts,ctx);
						add_parent(dest->sfnode,parent,__FILE__,__LINE__);
					}else{
						dest->sfnode = NULL;
					}
				}
				break;
			case FIELDTYPE_SFString:
				{
					struct Uni_String **ss, *sd;
					ss = (struct Uni_String **)source;
					sd = (struct Uni_String *)MALLOC (struct Uni_String*, sizeof(struct Uni_String));
					memcpy(sd,*ss,sizeof(struct Uni_String));
					sd->strptr = STRDUP((*ss)->strptr);
					dest->sfstring = sd;
				}
				break;
			default:
				//memcpy(dest,source,sizeof(union anyVrml));
				memcpy(dest,source,isize);
				break;
		}
	}
} //return copy_field

//deep_copy_broto_body((struct X3D_Proto**)source,(struct X3D_Proto**)dest,p2p,instancedScripts);
void deep_copy_broto_body(struct X3D_Proto** proto, struct X3D_Proto** dest, Stack *instancedScripts)
{
	//converts from binary proto/broto format to old scene format:
	//  - ROUTES are registered in global ROUTE registry
	//  - nodes are instanced and registered in memoryTable for startOfLoopNodesUpdate and killNode access
	//  - sensors, viewpoints etc are registered
	//proto - broto instance with 
	//  - a pointer __prototype to its generic prototype
	//dest - protoinstance with user fields filled out for this instance already 
	//     - children/body not filled out yet - it's done here
	//     - any field/exposedField values in interface IS copied to body node fields
	//     - broto ROUTES registered in global route registry, with this instances' node* addresses
	//     ? ( sensors, viewpoints will be directly registered in global/main scene structs)
	//what will appear different in the scene:
	//  old: PROTO instances appear as Group nodes with metaSF nodes for interface routing, mangled DEFnames
	//  new: PROTO instances will appear as X3DProto nodes with userfields for interface routing, local DEFnames

	//DEEP COPYING start here
	//we're instancing for the final scene, so go deep
	//0. setup pointer lookup table proto to instance
	//1. copy nodes, recursing on MFNode,SFnode fields
	//   to copy, instance a new node of the same type and save (new pointer, old pointer) in lookup table
	//   iterate over proto's node's fields, copying, and recursing on MFNode, SFNode
	//   if node is a ProtoInstance, deepcopy it
	//2. copy ROUTE table, looking up new_pointers in pointer lookup table
	//3. copy IS table, looking up new_pointers in pointer lookup table
	//4. copy DEFname table, looking up new pointers in pointer lookup table
	//

	struct X3D_Proto *prototype, *p;
	struct X3D_Node *parent;
	struct Vector *p2p = newVector(struct pointer2pointer*,10);
	
	//2. copy body from source's _prototype.children to dest.children, ISing initialvalues as we go
	p=(*dest);
	p->__children.n = 0;
	p->__children.p = NULL;
	parent = (struct X3D_Node*) (*dest); //NULL;
	prototype = (struct X3D_Proto*)(*proto)->__prototype;
	//prototype = (struct X3D_Proto*)p->__prototype;
	//2.c) copy IS
	//p->__IS = copy IStable from prototype, and the targetNode* pointer will be wrong until p2p
	//copy_IStable(&((Stack*)prototype->__IS), &((Stack*)p->__IS));

	copy_IStable((Stack **) &(prototype->__IS), (Stack **) &(p->__IS));

	//2.a) copy rootnodes
	copy_field(FIELDTYPE_MFNode,(union anyVrml*)&(prototype->__children),(union anyVrml*)&(p->__children),
		p2p,instancedScripts,p,parent);
	//2.b) copy routes
	copy_routes(prototype->__ROUTES, p, p2p);
	//2.d) copy defnames
	copy_defnames(prototype->__DEFnames, p, p2p);

	//3. convert IS events to backward routes
	copy_IS(p->__IS, p, p2p);

	//*dest = p;
	//free p2p
	return;
}

/* shallow_copy_field - a step beyond memcpy(anyvrml,anyvrml,len) by getting the MF elements
   malloced and copied to, except shallow in that SFNodes aren't deep copied - just the 
   pointers are copied
*/
void shallow_copy_field(int typeIndex, union anyVrml* source, union anyVrml* dest)
{
	int i, isize;
	int sftype, isMF;
	struct Multi_Node *mfs,*mfd;

	isMF = typeIndex % 2;
	sftype = typeIndex - isMF;
	//from EAI_C_CommonFunctions.c
	isize = returnElementLength(sftype) * returnElementRowSize(sftype);
	if(isMF)
	{
		int nele;
		char *ps, *pd;
		mfs = (struct Multi_Node*)source;
		mfd = (struct Multi_Node*)dest;
		//we need to malloc and do more copying
		nele = mfs->n;
		if( sftype == FIELDTYPE_SFNode ) nele = (int) upper_power_of_two(nele);
		mfd->p = MALLOC (struct X3D_Node **, isize*nele);
		mfd->n = mfs->n;
		ps = (char *)mfs->p;
		pd = (char *)mfd->p;
		for(i=0;i<mfs->n;i++)
		{
			shallow_copy_field(sftype,(union anyVrml*)ps,(union anyVrml*)pd);
			ps += isize;
			pd += isize;
		}
	}else{ 
		//isSF
		switch(typeIndex)
		{
			case FIELDTYPE_SFString:
				{
					//go deep, same as copy_field
					struct Uni_String **ss, *sd;
					ss = (struct Uni_String **)source;
					sd = (struct Uni_String *)MALLOC (struct Uni_String*, sizeof(struct Uni_String));
					memcpy(sd,*ss,sizeof(struct Uni_String));
					sd->strptr = STRDUP((*ss)->strptr);
					dest->sfstring = sd;
				}
				break;
			default:
				//memcpy(dest,source,sizeof(union anyVrml));
				memcpy(dest,source,isize);

				break;
		}
	}
} //return copy_field
int PKW_from_KW(int KW_index)
{
	/* translates the KEYWORDS[KW_index] field mode found in the 4th column of the OFFSETS_ 
	   into an index that works in the PROTOKEYWORDS[] array  */
	int pkw = -1;
	switch(KW_index)
	{
		case KW_initializeOnly:
			pkw = PKW_initializeOnly; break;
		case KW_inputOnly:
			pkw = PKW_inputOnly; break;
		case KW_outputOnly:
			pkw = PKW_outputOnly; break;
		case KW_inputOutput:
			pkw = PKW_inputOutput; break;
		case KW_field:
			pkw = PKW_field; break;
		case KW_eventIn:
			pkw = PKW_eventIn; break;
		case KW_eventOut:
			pkw = PKW_eventOut; break;
		case KW_exposedField:
			pkw = PKW_exposedField; break;
		default:
			pkw = -1;
	}
	return pkw;
}
int isManagedField(int mode, int type, int isPublic);
void registerParentIfManagedField(int type, int mode, int isPublic, union anyVrml* any, struct X3D_Node* parent)
{
	//puts what you say is the parent of the sfnode/mfnodes into the parentVector of each sfnode/mfnodes
	// if its a managed field.
	//isPublic - no leading _ on field name, or script/proto user field - you tell us, its easier that way
	//managed field: public && node field && value holding
	//int isManagedField;
	//isManagedField = isPublic && (type == FIELDTYPE_SFNode || type == FIELDTYPE_MFNode);
	//isManagedField = isManagedField && (mode == PKW_initializeOnly || mode == PKW_inputOutput);
	if(isManagedField(mode,type,isPublic))
	{
		int n,k,haveSomething;
		struct X3D_Node **plist, *sfn;
		haveSomething = (type==FIELDTYPE_SFNode && any->sfnode) || (type==FIELDTYPE_MFNode && any->mfnode.n);
		haveSomething = haveSomething && parent;
		if(haveSomething){
			if(type==FIELDTYPE_SFNode){
				plist = &any->sfnode;
				n = 1;
			}else{
				plist = any->mfnode.p;
				n = any->mfnode.n;
			}
			for(k=0;k<n;k++)
			{
				sfn = plist[k];
				if(sfn){ 
					if( !sfn->_parentVector)
						sfn->_parentVector = newVector(struct X3D_Node*,2);
					vector_pushBack(struct X3D_Node*, sfn->_parentVector, parent);
				}
			}
		}
	}
}
void deep_copy_node(struct X3D_Node** source, struct X3D_Node** dest, struct Vector *p2p, Stack *instancedScripts, 
					struct X3D_Proto* ctx)
{
	struct pointer2pointer *pair;
	struct X3D_Node* parent;
	if(*source == NULL){
		*dest = NULL;
		return;
	}
	*dest = inPointerTable(*source,p2p);
	if(*dest) 
		return; //already created and we're likely at what would be a USE in the original ProtoDeclare body
	//create new Node
	//if((*source)->_nodeType == NODE_PlaneSensor)
	//	printf("got a planesensor - going to allocate and register it\n");
	*dest=X3D_NODE(createNewX3DNode( (*source)->_nodeType)); //will register sensors and viewpionts
	parent = *dest;
	if((*source)->_nodeType == NODE_Script)
		stack_push(struct X3D_Node*,instancedScripts,*dest);
	//register in pointer lookup table
	pair = MALLOC(struct pointer2pointer*,sizeof(struct pointer2pointer));
	pair->pp = *source;
	pair->pn = *dest;
	vector_pushBack(struct pointer2pointer*, p2p, pair);
	//copy fields
	{
		typedef struct field_info{
			int nameIndex;
			int offset;
			int typeIndex;
			int ioType;
			int version;
		} *finfo;
		finfo offsets;
		finfo field;
		int ifield;

		offsets = (finfo)NODE_OFFSETS[(*source)->_nodeType];
		ifield = 0;
		field = &offsets[ifield];
		while( field->nameIndex > -1) 
		{
			int is_source;
			struct brotoIS * isrecord;
			//printf(" %s",FIELDNAMES[field->nameIndex]); //[0]]);
			//printf(" (%s)",FIELDTYPES[field->typeIndex]); //field[2]]);
			isrecord = NULL;
			is_source = 0; 	//field isource: 0=builtin 1=script user field 2=shader_program user field 3=Proto/Broto user field 4=group __protoDef
            isrecord = in_IStable(*source,ifield,(Stack *)ctx->__IS, is_source);
            if (isrecord != NULL)
			{
				//do something to change from:
				// copy *source to *dest, to 
				// copy ctx->interface[ctx->__IS[is_addr].interfacefieldIndex]].value to *dest.[ifield] 
				union anyVrml *source_field, *dest_field;
				struct ProtoDefinition *sp; 
				struct ProtoFieldDecl *sdecl;
				sp = ctx->__protoDef;
				sdecl = protoDefinition_getFieldByNum(sp, isrecord->iprotofield);

				//source_field = (union anyVrml*)&((char*)*source)[field->offset];
				source_field = (union anyVrml*)&(sdecl->defaultVal);
				dest_field   = (union anyVrml*)&((char*)*dest  )[field->offset];
				//copy_field(field->typeIndex,source_field,dest_field,p2p,instancedScripts,ctx);
				shallow_copy_field(field->typeIndex, source_field, dest_field);
				registerParentIfManagedField(field->typeIndex,PKW_from_KW(field->ioType),1, dest_field, *dest);

				// similarly below when processing ISs on user fields in script and broto fields.
				// istable could include a hint on the type of field to be looking for:
				//   ie builtin offset vs scriptfield/brotofield/userfield index
				//   while remembering some node types could have either ie Script{ url IS protofield_URL SFString hemoglobin IS protofield_blood...
			}
			else if((*source)->_nodeType == NODE_Proto && !strcmp(FIELDNAMES[field->nameIndex],"__protoDef") )
			{
				int k;
				//struct X3D_Proto *prototype, *p;
				struct ProtoDefinition *sp, *dp; 
				struct ProtoFieldDecl *sdecl,*ddecl;
				struct X3D_Proto *s, *d;

				s = (struct X3D_Proto*)*source;
				d = (struct X3D_Proto*)*dest;
				sp = s->__protoDef;
				dp = NULL;

				if(sp){ //are there any Proto fields? Not for the Scene - this may not be malloced for the scene
					//dp = MALLOC(struct ProtoDefinition*,sizeof(struct ProtoDefinition));
					dp = newProtoDefinition();
					//memcpy(dp,sp,sizeof(struct ProtoDefinition));
					dp->iface = newVector(struct ProtoFieldDecl *, sp->iface->n);
					dp->protoName = strdup(sp->protoName);
					dp->isCopy = TRUE;
					for(k=0;k<sp->iface->n;k++)
					{
						sdecl = protoDefinition_getFieldByNum(sp, k);
 						ddecl=newProtoFieldDecl(sdecl->mode, sdecl->type, sdecl->name);
						//memcpy(ndecl,pdecl,sizeof(struct ProtoFieldDecl *)); //not just the pointer

						is_source = 3; 	//field isource: 0=builtin 1=script user field 2=shader_program user field 3=Proto/Broto user field 4=group __protoDef

						
						isrecord = in_IStable(*source,k,(Stack *)ctx->__IS, is_source); 
                        if (isrecord != NULL)
						{
							//do something to change from:
							// copy *source to *dest, to 
							// copy ctx->interface[ctx->__IS[is_addr].interfacefieldIndex]].value to *dest.[ifield] 
							union anyVrml *source_field, *dest_field;
							struct ProtoDefinition *sp; 
							struct ProtoFieldDecl *sdecl;
							sp = ctx->__protoDef;
							sdecl = protoDefinition_getFieldByNum(sp, isrecord->iprotofield);

							//source_field = (union anyVrml*)&((char*)*source)[field->offset];
							source_field = (union anyVrml*)&(sdecl->defaultVal);
							dest_field   = (union anyVrml*)&(ddecl->defaultVal);
							//copy_field(sdecl->type,source_field,dest_field,p2p,instancedScripts,ctx);
							//shallow copy means if its an SFNode or MFNode field, don't 
							//deep copy - just copy the pointers. That's because the SFNodes involved
							//are already instanced/memoryTable malloced for the live scene: they were
							//done for the current proto interface
							shallow_copy_field(sdecl->type, source_field, dest_field);
							registerParentIfManagedField(sdecl->type,sdecl->mode,1, dest_field, *dest);
						}else{
							if(0) //shallow copy for testing
								memcpy(ddecl,sdecl,sizeof(struct ProtoFieldDecl));  //.. the whole struct
							/* proper deep copy we must do to get value-holding SFNode fields 
							   (event fields will have uninitialized junk)*/
							if(sdecl->mode == PKW_initializeOnly || sdecl->mode == PKW_inputOutput){
							//if(1){
								union anyVrml *source_field, *dest_field;
								source_field = (union anyVrml*)&(sdecl->defaultVal);
								dest_field   = (union anyVrml*)&(ddecl->defaultVal);
								copy_field(sdecl->type,source_field,dest_field,p2p,instancedScripts,ctx,parent);
							}
						}
						protoDefinition_addIfaceField(dp, ddecl);
					}
					d->__protoDef = dp;
				}
			}
			else if((*source)->_nodeType == NODE_Script && !strcmp(FIELDNAMES[field->nameIndex],"__scriptObj") )
			{
				/*deep copy script user fields */
				int k;
				struct Vector *sfields;
				struct ScriptFieldDecl *sfield, *dfield;
				struct Shader_Script *sp, *dp;
				struct X3D_Script *s, *d;

				s = (struct X3D_Script*)*source;
				d = (struct X3D_Script*)*dest;
				sp = s->__scriptObj;
				dp = d->__scriptObj = new_Shader_ScriptB(*dest); 
				dp->loaded = sp->loaded; //s.b. FALSE
				dp->num = sp->num; //s.b. -1
				sfields = sp->fields;
				for(k=0;k<sfields->n;k++)
				{
					BOOL isInitialize;
					dfield = MALLOC(struct ScriptFieldDecl*,sizeof(struct ScriptFieldDecl));
					dfield->fieldDecl = MALLOC(struct FieldDecl *,sizeof(struct FieldDecl));
					is_source = 1; 	//field isource: 0=builtin 1=script user field 2=shader_program user field 3=Proto/Broto user field 4=group __protoDef
					sfield = vector_get(struct ScriptFieldDecl *,sfields,k);

					isrecord = in_IStable(*source,k,(Stack *)ctx->__IS, is_source);
					isInitialize = isrecord && (isrecord->mode == PKW_initializeOnly || isrecord->mode == PKW_inputOutput);
					if( isInitialize )
					{
						//do something to change from:
						// copy *source to *dest, to 
						// copy ctx->interface[ctx->__IS[is_addr].interfacefieldIndex]].value to *dest.[ifield] 
						union anyVrml *source_field, *dest_field;
						struct ProtoDefinition *sp; 
						struct ProtoFieldDecl *sdecl;

						sp = ctx->__protoDef;
						sdecl = protoDefinition_getFieldByNum(sp, isrecord->iprotofield);
						dfield->ASCIIvalue = STRDUP(sdecl->fieldString);
						memcpy(dfield->fieldDecl,sfield->fieldDecl,sizeof(struct FieldDecl));
						//ddecl = dfield->fieldDecl;
						//ddecl->fieldType = sdecl->type;
						//ddecl->JSparamNameIndex = sfield->fieldDecl->;
						//ddecl->lexerNameIndex = sdecl->name;
						//ddecl->PKWmode = sdecl->mode;
						//ddecl->shaderVariableID = 0;
						//source_field = (union anyVrml*)&((char*)*source)[field->offset];
						source_field = (union anyVrml*)&(sdecl->defaultVal);
						dest_field   = (union anyVrml*)&(dfield->value);
						//copy_field(field->typeIndex,source_field,dest_field,p2p,instancedScripts,ctx);
						shallow_copy_field(sdecl->type, source_field, dest_field);
						registerParentIfManagedField(sdecl->type, sdecl->mode, 1, dest_field, *dest);

					}else{
						dfield->ASCIIvalue = STRDUP(sfield->ASCIIvalue);
						//*(output->fieldDecl) = *(sfield->fieldDecl);
						memcpy(dfield->fieldDecl,sfield->fieldDecl,sizeof(struct FieldDecl));
						/* shallow copy for testing some scenarios*/
						if(0){
							dfield->value = sfield->value;
						}
						/* proper deep copy we must do to get value-holding SFNode fields 
						   (event fields will have uninitialized junk)*/
						if(sfield->fieldDecl->PKWmode == PKW_initializeOnly || sfield->fieldDecl->PKWmode == PKW_inputOutput){
							union anyVrml *source_field, *dest_field;
							source_field = (union anyVrml*)&(sfield->value);
							dest_field   = (union anyVrml*)&(dfield->value);
							copy_field(dfield->fieldDecl->fieldType,source_field,dest_field,p2p,instancedScripts,ctx,parent);
						}
						dfield->valueSet = sfield->valueSet;
					}
					vector_pushBack(struct ScriptFieldDecl *, dp->fields, dfield);
				}
			}
			else
			{
				if( FIELDNAMES[field->nameIndex][0] != '_'){ //Q. should we ignor private fields?
					union anyVrml *source_field, *dest_field;
					source_field = (union anyVrml*)&((char*)*source)[field->offset];
					dest_field   = (union anyVrml*)&((char*)*dest  )[field->offset];
					//if(!strcmp(FIELDNAMES[field->nameIndex],"appearance"))
					//{
					//	struct X3D_Shape* shp1, *shp2;
					//	shp1 = (struct X3D_Shape*)source;
					//	shp2 = (struct X3D_Shape*)(*source);
					//	printf("appearance shp1= %d shp2= %d\n",(int)shp1,(int)shp2);
					//}
					copy_field(field->typeIndex,source_field,dest_field,p2p,instancedScripts,ctx,parent);
				}
			}
			ifield++;
			field = &offsets[ifield];
		}
	}
	if((*source)->_nodeType == NODE_Proto)
	{
		/* deep copy the body/context/_prototype from the Proto:
		   - defnames, ISes, Routes, body nodes from _prototype upgraded by ISes
		*/
		//if(usingBrotos() )
			deep_copy_broto_body2((struct X3D_Proto**)source,(struct X3D_Proto**)dest);
		//else
		//	deep_copy_broto_body((struct X3D_Proto**)source,(struct X3D_Proto**)dest,instancedScripts);
	}
}
int nextScriptHandle (void);
#ifdef HAVE_JAVASCRIPT
void initialize_one_script(struct Shader_Script* ss, const struct Multi_String *url){
	struct ScriptFieldDecl* field;
	int j;

	//printf("script node %p \n",sn);
	ss->num = nextScriptHandle(); 
	//printf(" num=%d \n",ss->num);
	JSInit(ss); //ss->num);
	// 2)init each field
	for(j=0;j<ss->fields->n;j++)
	{
		//printf("initializing field %d of %d \n",j,ss->fields->n);
		field = vector_get(struct ScriptFieldDecl*,ss->fields,j);
		//script_addField(ss,field);

		scriptFieldDecl_jsFieldInit(field, ss->num); //saves it for initializeOnly work
		//printf("\t field index %d JSparamnameIndex %d name %s\n",
		//	j,field->fieldDecl->JSparamNameIndex,JSparamnames[field->fieldDecl->JSparamNameIndex].name);
		//Q. do I need this: - it mallocs something. Or is this just for initializeOnly?
		//void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value) {

 	}
	// 3)init from URI
	script_initCodeFromMFUri(ss, url);
}
#endif /* HAVE_JAVASCRIPT */
void initialize_scripts(Stack *instancedScripts)
{
	/* 
	Old (pre-2013) Script special handling during Parsing:
	1. new script node? get a num for it 
		ret->num=nextScriptHandle(); JSInit(ret->num);
	2. new script field? add it and initailize its value: 
		script_addField > scriptFieldDecl_jsFieldInit(field, me->num);
	3. after all fields of a script node have been added, try initializing the URL field
		script_initCodeFromUri  > script_initCode

	New (2013 BROTO era) Script Special handling:
	Parsing: don't do any of the above 3
	Instancing: in sceneInstance():
	- instance all nodes generically, including script nodes,
		except for each script node instanced put it in a special node* list
	- after all nodes have been instanced, call this initializeScripts() function:
	-- go through the special node* list of instanced scripts and for each (script) node*:
		1)get a script num  2)init each field  3)init from URI  (like the above 3)

	*/
	//int n, i,j;
	//struct X3D_Node* p;
	//struct X3D_Script* sn;
	//struct Shader_Script* ss; //)X3D_SCRIPT(node)->__scriptObj)->num
	//struct ScriptFieldDecl* field;
	//JSObject *eventInFunction;

	#ifdef HAVE_JAVASCRIPT

	if(instancedScripts)
	{
        int n, i,j;
        struct X3D_Node* p;
        struct X3D_Script* sn;
        struct Shader_Script* ss; //)X3D_SCRIPT(node)->__scriptObj)->num
        struct ScriptFieldDecl* field;


		n = instancedScripts->n;
		for(i=0;i<n;i++)
		{
			p = vector_get(struct X3D_Node*,instancedScripts,i);
			sn = (struct X3D_Script*)p;
			// 1)get a script num
			ss = sn->__scriptObj;
			if(1){
				initialize_one_script(ss,&sn->url);
			}else{
			
				//printf("script node %p \n",sn);
				//printf("in initialize_scripts i=%d __scriptObj =%p ",i,ss);
				ss->num = nextScriptHandle(); 
				//printf(" num=%d \n",ss->num);
				JSInit(ss); //ss->num);
				// 2)init each field
				for(j=0;j<ss->fields->n;j++)
				{
					//printf("initializing field %d of %d \n",j,ss->fields->n);
					field = vector_get(struct ScriptFieldDecl*,ss->fields,j);
					//script_addField(ss,field);

					scriptFieldDecl_jsFieldInit(field, ss->num); //saves it for initializeOnly work
					//printf("\t field index %d JSparamnameIndex %d name %s\n",
					//	j,field->fieldDecl->JSparamNameIndex,JSparamnames[field->fieldDecl->JSparamNameIndex].name);
					//Q. do I need this: - it mallocs something. Or is this just for initializeOnly?
					//void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value) {

 				}
				// 3)init from URI
				script_initCodeFromMFUri(ss, &sn->url);
			}
		}
	}
	#endif /* HAVE_JAVASCRIPT */

}
void sceneInstance(struct X3D_Proto* sceneProto, struct X3D_Node *sceneInstance)
{
	//deprecated Sept 2014 by dug9: we now instance as we parse a scene (or if we did parse a scene as a sceneDeclare
	//   we could call brotoInstance() and deep_copy_broto_body2() to instance the scene)
	
	//sceneProto - cParse results in new X3D_Proto format
	//sceneInstance - pass in a Group node to accept scene rootNodes 
	//				- (ROUTES, sensors, viewpoints will be directly registered in global/main scene structs)
	//converts from binary proto/broto format to old scene format:
	//  - ROUTES are registered in global ROUTE registry
	//  - nodes are instanced and registered in memoryTable for startOfLoopNodesUpdate and killNode access
	//  - sensors, viewpoints etc are registered
	//what will appear different in the scene:
	//  old: PROTO instances appear as Group nodes with metaSF nodes for interface routing, mangled DEFnames
	//  new: PROTO instances will appear as X3DProto nodes with userfields for interface routing, local DEFnames

	//DEEP COPYING start here
	//we're instancing for the final scene, so go deep
	//0. setup pointer lookup table proto to instance
	//1. copy nodes, recursing on MFNode,SFnode fields
	//   to copy, instance a new node of the same type and save (new pointer, old pointer) in lookup table
	//   iterate over proto's node's fields, copying, and recursing on MFNode, SFNode
	//   if node is a ProtoInstance, deepcopy it
	//2. copy ROUTE table, looking up new_pointers in pointer lookup table
	//3. copy IS table, looking up new_pointers in pointer lookup table
	//4. copy DEFname table, looking up new pointers in pointer lookup table
	//

	
	struct X3D_Proto *scenePlaceholderProto;
	struct X3D_Node *parent;
	struct Multi_Node *children;
	struct Vector *p2p = newVector(struct pointer2pointer*,10);
	Stack *instancedScripts = newStack(struct X3D_Node*);
	children = childrenField(sceneInstance);
	children->n = 0;
	children->p = NULL;
	parent = (struct X3D_Node*)sceneInstance;
	scenePlaceholderProto = createNewX3DNode0(NODE_Proto);
	//if(0){
	//	prototype = (struct X3D_Proto*)sceneProto->__prototype;
	//	//copy rootnodes
	//	copy_field(FIELDTYPE_MFNode,(union anyVrml*)&(prototype->children),(union anyVrml*)&(sceneInstance->children),p2p);
	//}else{
	//I think the sceneProto being passed in is already the prototype -with body- and not an interface/instance
	//copy rootnodes
	copy_field(FIELDTYPE_MFNode,(union anyVrml*)&(sceneProto->__children),(union anyVrml*)children,
		p2p,instancedScripts,scenePlaceholderProto,parent);
	//}
	//copy sceneProto routes (child protoInstance routes copied elsewhere)
	copy_routes(sceneProto->__ROUTES, NULL, p2p);
	//shouldn't be any IS-table in main scene (child protoInstance IS-tables copied elsewhere)
	copy_IS(sceneProto->__IS, NULL, p2p);
	//copy sceneProto defnames (child protoInstance defnames copied elsewhere - will duplicate)
	copy_defnames(sceneProto->__DEFnames, NULL, p2p);

	initialize_scripts(instancedScripts);

	return;
}


/*  added Jan 15, 2013:
	parse an IS statement:
	- test if it's an IS, if not backup and return lexer's original position
	- register IS in ProtoDeclare's IS-table. 
    - swallow the the nodeFieldName, IS and protoFieldName tokens. 
	- set the value of the field in the node, if appropriate.

	Background:
	Goal: allow IS in the main scene, or more precisely, 
	   to parse ProtoDeclare Bodies with main scene parsing code for new style protos. 
	That means we need IS handling in the main scene parsing code. 
	This seems like a nice place to put the handler.
	I'm expecting everything else already handled except IS:
	----------------------------
	WRL Parsing Fields on Nodes
	when parsing a node we expect to see
	A. For built-in fields and user fields on protoInstances where mode and fieldtype are known in advance:
	(optional DEF name) nodeType ({ list of fields } or USE name)
	And for each field mentioned, one of:
	fieldname <fieldvalue>			where <fieldvalue> is one of: 
*		IS protoFieldName	#on any type & mode that jives^ with protoField type & mode
		1 2 3				#just for SF modes: field/initializeOnly, exposedField/inputOutput
		[1 2 3, 4 5 6]		#just for MF modes: field/initializeOnly, exposedField/inputOutput
		USE nodename		#just for SFNode modes: field/initializOnly, exposedField/inputOutput
		DEF nodename type{}	#just for SFNode modes: field/initializOnly, exposedField/inputOutput
		type{}				#just for SFNode modes: field/initializOnly, exposedField/inputOutput
	where type{} is any builtin or user node type	
	B. for user fields on scripts and protoDeclares:
	mode fieldtype fieldname <fieldvalue>
	where:
  		mode is one of field, eventIn, eventOut, exposedField
  		fieldtype is one of SF*,MF* where * is one of Color,Vec3f,Node,...
  		<fieldvalue> can be one of the above combinations as for builtin fields

	^See the IS mode-jive table here:
	http://www.web3d.org/files/specifications/19775-1/V3.2/Part01/concepts.html#PROTOdefinitionsemantics
	where:
	'Prototype Declaration' means the Interface of a ProtoDeclare/PROTO
	'Prototype Definition'  means the Body      of a ProtoDeclare/PROTO
	---------------------------------
 
*/
BOOL nodeTypeSupportsUserFields(struct X3D_Node *node)
{
	BOOL user = FALSE;
	user = node->_nodeType == NODE_Proto || node->_nodeType == NODE_Script || 
		   node->_nodeType == NODE_ShaderProgram ||  node->_nodeType == NODE_ComposedShader ||
		   node->_nodeType == NODE_PackagedShader ? TRUE : FALSE;
	if(!user &&  node->_nodeType == NODE_Group){
		struct X3D_Group* grp = (struct X3D_Group*)node;
		user = grp->FreeWRL__protoDef !=INT_ID_UNDEFINED ? TRUE : FALSE;
	}
	return user;
}


const char *rootFieldName(const char* fieldname, int* len, int *has_changed, int *has_set)
{
	/* 
	given a fieldname wholename "set_amazing_changed"
	it returns:  s="amazing_changed", len = 7, has_changed 1, has_set 1 
	you can use s and len in strncmp to test against the rootname
	*/
	static char* str_changed = "_changed";

	static int len_changed = 8;
	static int len_set = 4;

	int ln;
	const char* s;

	ln = (int) strlen(fieldname);

	*has_changed = ln > len_changed ? !strncmp(&fieldname[ln-len_changed],str_changed,len_changed) : FALSE;
	*has_set     = ln > len_set ? !strncmp(fieldname,"set_",len_set) : FALSE;
	s = *has_set ? &fieldname[len_set] : fieldname;
	*len = *has_changed? (int)(&fieldname[ln - len_changed] - s) : (int)(&fieldname[ln] - s);
	return s;
}
BOOL fieldSynonymCompare(const char *routeFieldName, const char* nodeFieldName) //, int nodeFieldMode)
{
	// like strcmp, it returns false if there's no difference / a match
	// except it compares rootnames if there is a set_ or _changed on either
	// ie (set_amazing, amazing_changed) returns FALSE (a match of rootname).
	int lr,hsr,hcr,ln,hsn,hcn;
	const char *rf, *nf;

	if(!strcmp(routeFieldName,nodeFieldName) )return FALSE; //easy match of wholenames

	//get rootnames with set_ and _changed stripped off
	rf = rootFieldName(routeFieldName,&lr,&hsr,&hcr);
	nf = rootFieldName(nodeFieldName, &ln,&hsn,&hcn);

	if(lr != ln) return TRUE; //rootname lengths are different so no hope of match
	if(strncmp(rf,nf,lr)) return TRUE; //rootnames are different so no hope of match
	//the rootnames are equal
	//Q. are there some special conditions for matching, with nodeFieldMode or route/IS field mode
	//a) for routes you know a-priori/beforehand 'ROUTE from TO to' and on the 
	//   from node you want either  eventOut/outputOnly  (_changed) or exposedField/inputOutput (rootname)
	//   to   node you want either  eventIn/inputOnly or (set_)   or exposedField/inputOutput (rootname)
	//b) for IS there is a "mode-jive table" that says 
	//		- if your node is inputOutput the protofield can be any mode.
	//		- otherwise the nodefield and protofield modes must match.
	// in both cases you would want to try to match wholenames first ie trust the scene author.
	return FALSE; //a match
}


//#define WRLMODE(val) (((val) % 4)+4) //jan 2013 codegen PROTOKEYWORDS[] was ordered with x3d synonyms first, wrl last

//#define X3DMODE(val)  ((val) % 4)
//#define X3DMODE(##val)   PKW_from_KW((##val) 
int X3DMODE(int val)
{
	int iret = 0;
	switch(val){
		case PKW_field: 
		case PKW_initializeOnly:
			iret = PKW_initializeOnly; break;
		case PKW_exposedField:
		case PKW_inputOutput:
			iret = PKW_inputOutput; break;
		case PKW_eventIn: 
		case PKW_inputOnly:
			iret = PKW_inputOnly; break;
		case PKW_eventOut: 
		case PKW_outputOnly:
			iret = PKW_outputOnly; break;
		default:
			printf("ouch from X3DMODE\n");
	}
	return iret;
}


BOOL walk_fields(struct X3D_Node* node, int (*callbackFunc)(), void* callbackData);
typedef struct cbDataExactName {
	char *fname;
	union anyVrml* fieldValue;
	int mode;
	int type;
	int jfield;
	int source;
	int publicfield;
} s_cbDataExactName;
BOOL cbExactName(void *callbackData,struct X3D_Node* node,int jfield,union anyVrml *fieldPtr,char *fieldName, int mode,int type,int source,int publicfield)
{
	BOOL found;
	s_cbDataExactName *cbd = (s_cbDataExactName*)callbackData;
	found = !strcmp(fieldName,cbd->fname);
	if(found){
		cbd->fieldValue = fieldPtr;
		cbd->fname = fieldName;
		cbd->jfield = jfield;
		cbd->mode = mode;
		cbd->type = type;
		cbd->publicfield = publicfield;
		cbd->source = source;
	}
	return found; //returning true continues field walk, returning false breaks out.
}
BOOL find_anyfield_by_name(struct VRMLLexer* lexer, struct X3D_Node* node, union anyVrml **anyptr, 
			int *imode, int *itype, char* nodeFieldName, int *isource, void** fdecl, int *ifield)
{
	int found;
	s_cbDataExactName cbd;
	cbd.fname = nodeFieldName;
	found = walk_fields(node,cbExactName,&cbd);
	if(found){
		*anyptr = cbd.fieldValue;
		*imode = cbd.mode;
		*itype = cbd.type;
		*isource = cbd.source;
		*ifield = cbd.jfield;
	}
	return found;
}
typedef struct cbDataRootNameAndRouteDir {
	char *fname;
	int PKW_eventType;
	union anyVrml* fieldValue;
	int mode;
	int type;
	int jfield;
	int source;
	int publicfield;
} s_cbDataRootNameAndRouteDir;

BOOL cbRootNameAndRouteDir(void *callbackData,struct X3D_Node* node,int jfield,union anyVrml *fieldPtr,char *fieldName, int mode,int type,int source,int publicfield)
{

	int found;
	s_cbDataRootNameAndRouteDir *cbd = (s_cbDataRootNameAndRouteDir*)callbackData;
	found = !fieldSynonymCompare(fieldName,cbd->fname) ? TRUE : FALSE;
	found = found && (mode == cbd->PKW_eventType || mode == PKW_inputOutput);
	if(found){
		cbd->fname = fieldName;
		cbd->jfield = jfield;
		cbd->mode = mode;
		cbd->type = type;
		cbd->publicfield = publicfield;
		cbd->source = source;
	}
	return found;
}
BOOL find_anyfield_by_nameAndRouteDir(struct VRMLLexer* lexer, struct X3D_Node* node, union anyVrml **anyptr, 
			int *imode, int *itype, char* nodeFieldName, int *isource, void** fdecl, int *ifield, int PKW_eventType)
{
	int found;
	s_cbDataRootNameAndRouteDir cbd;
	cbd.fname = nodeFieldName;
	cbd.PKW_eventType = PKW_eventType;
	found = walk_fields(node,cbRootNameAndRouteDir,&cbd);
	if(found){
		*anyptr = cbd.fieldValue;
		*imode = cbd.mode;
		*itype = cbd.type;
		*isource = cbd.source;
		*ifield = cbd.jfield;
	}
	return found;
}

void broto_store_IS(struct X3D_Proto *proto,char *protofieldname,int pmode, int iprotofield, int type,
					struct X3D_Node *node, char* nodefieldname, int mode, int ifield, int source)
{
	Stack* ISs;
	struct brotoIS* is;

	is = MALLOC(struct brotoIS*,sizeof(struct brotoIS));
	is->proto = proto;
	is->protofieldname = protofieldname;
	is->pmode = pmode;
	is->iprotofield = iprotofield;
	is->type = type;
	is->node = node;
	is->nodefieldname = nodefieldname;
	is->mode = mode;
	is->ifield = ifield;
	is->source = source;

	ISs = proto->__IS;
	if( ISs == NULL){
		ISs = newStack(struct brotoIS *);
		proto->__IS = ISs;
	}
	stack_push(struct brotoIS *, ISs, is);
	return;
}

BOOL found_IS_field(struct VRMLParser* me, struct X3D_Node *node)
{
	/* if IS found, then
		if field/exposedfield/initializeOnly/inputOutput
			copy value
		register IS in IS-table
	*/
	int i;
    int mode;
    int type;
	int source;
	int ifield, iprotofield;
    	//OLDCODE union anyVrml *defaultVal;
	struct ProtoDefinition* pdef=NULL;
	struct X3D_Proto* proto;
	char *protoFieldName;
	char *nodeFieldName;
	DECLAREUP
	int foundField;
	int foundProtoField;
	struct ProtoFieldDecl* f;
	union anyVrml *fieldPtr;
	void *fdecl;

	//OLDCODE defaultVal = NULL;



	SAVEUP  //save the lexer spot so if it's not an IS we can back up
	/* get nodeFieldName */
	if(!lexer_setCurID(me->lexer)) return FALSE;
	ASSERT(me->lexer->curID);
	nodeFieldName = STRDUP(me->lexer->curID);
	//BACKUP;
	FREE_IF_NZ(me->lexer->curID);

	/*
	We want to know 5 things about the node:
	0. is it a user or builtin node
	1. does the field/event exist on the node
	if so then
	3. what's it's type - SF/MF,type - needed for compatibility check with proto field
	4. what's it's mode - [in][out][in/out] - ditto, IS-table routing direction and initialization
	5. what's its route field address: how does the is-table or route get to it: 
		- integer Offset for builtin, integer field index for usernode
		- when deepcopying a protoDeclare during sceneInstance, the node address would be remapped,
		  but these integers would stay the same
	*/
	fieldPtr = NULL;
	foundField = find_anyfield_by_name(me->lexer, node, &fieldPtr, &mode, &type, nodeFieldName, &source, &fdecl, &ifield);

	if(!(foundField))
	{
		BACKUP
		return FALSE;  /* not a field or event */
	}

	/* got a node field or event */
	/* check if IS-statement? */
	if(!lexer_keyword(me->lexer, KW_IS)) {
		BACKUP
		FREE_IF_NZ(nodeFieldName);
		return FALSE; /* not an IS, maybe regular field or USE/DEF */
	}

	/* got an IS, so unconditionally swallow 3 tokens: nodeFieldName IS protoFieldName */

	/* get protoFieldName */
	if(!lexer_setCurID(me->lexer)) return FALSE;
	ASSERT(me->lexer->curID);
	protoFieldName = STRDUP(me->lexer->curID);
	FREE_IF_NZ(me->lexer->curID);


	/*Now we need to know the same things about the proto node and field:
	0. we know it's a user node - all X3D_Proto/Protos are, and they (should) have no public builtin fields
	   (children should be removed)
	1. does the field/event exist on the proto
	if so then
	3. what's it's type - SF/MF,type - needed for compatibility check with node field
	4. what's it's mode - [in][out][in/out] - ditto, IS-table routing direction and initialization
	5. what's its route field address: how does the is-table or route get to it: 
		- integer field index for usernode
	*/
	proto = (struct X3D_Proto*)me->ptr;
	pdef = (struct ProtoDefinition*)proto->__protoDef; //__brotoObj;
	foundProtoField = FALSE;
	f = NULL;
	{
		const char* pname = NULL;
		struct Vector* usernames[4];
		const char **userArr;
		usernames[0] = me->lexer->user_initializeOnly;
		usernames[1] = me->lexer->user_inputOnly;
		usernames[2] = me->lexer->user_outputOnly;
		usernames[3] = me->lexer->user_inputOutput;
		iprotofield = -1;
		for(i=0; i!=vectorSize(pdef->iface); ++i)
		{
			f=vector_get(struct ProtoFieldDecl*, pdef->iface, i);
			userArr =&vector_get(const char*, usernames[X3DMODE(f->mode)], 0);
			pname = userArr[f->name];

			foundProtoField = !strcmp(pname,protoFieldName) ? TRUE : FALSE;
			if(foundProtoField ) {
				/* printf ("protoDefinition_getField, comparing %d %d and %d %d\n", f->name, ind, f->mode, mode); */
				//return f;
				iprotofield = i;
				break;
			}
		}
		if( !foundProtoField ){
			ConsoleMessage("Parser error: no matching protoField for %s IS %s\n",
				nodeFieldName,protoFieldName);
			FREE_IF_NZ(me->lexer->curID);
			FREEUP
			FREE_IF_NZ(nodeFieldName);
			FREE_IF_NZ(protoFieldName);
			return TRUE;
		}
		//check its type
		if(f->type != type){
			if(!pname) pname = "";
			ConsoleMessage("Parser error: IS - we have a name match: %s IS %s found protofield %s\n",
				nodeFieldName,protoFieldName,pname);
			ConsoleMessage("...But the types don't match: nodefield %s protofield %s\n",
				FIELDTYPES[type],FIELDTYPES[f->type]);
			FREE_IF_NZ(me->lexer->curID);
			FREEUP
			FREE_IF_NZ(nodeFieldName);
			FREE_IF_NZ(protoFieldName);
			return TRUE;
		}
		//check its mode
		// http://www.web3d.org/files/specifications/19775-1/V3.2/Part01/concepts.html#t-RulesmappingPROTOTYPEdecl
		// there's what I call a mode-jive table
		//							proto interface
		//							inputOutput	initializeOnly	inputOnly	outputOnly
		//	node	inputOutput		jives		jives			jives		jives
		//			initializeOnly				jives
		//			inputOnly									jives
		//			outputOnly												jives
		//
		// so if our nodefield's mode is inputOutput/exposedField then we are covered for all protoField modes
		// otherwise, the nodefield's mode must be the same as the protofield's mode
		if(X3DMODE(mode) != PKW_inputOutput && X3DMODE(mode) != X3DMODE(f->mode)){
			ConsoleMessage("Parser Error: IS - we have a name match: %s IS %s found protofield %s\n",
				nodeFieldName,protoFieldName,f->fieldString);
			ConsoleMessage("...But the modes don't jive: nodefield %s protofield %s\n",
				PROTOKEYWORDS[mode],PROTOKEYWORDS[f->mode]);
			FREE_IF_NZ(me->lexer->curID);
			FREEUP
			FREE_IF_NZ(nodeFieldName);
			FREE_IF_NZ(protoFieldName);
			return TRUE;
		}
	}

	//we have an IS that's compatible/jives
	//a) copy the value if it's an initializeOnly or inputOutput
	if(X3DMODE(f->mode) == PKW_initializeOnly || X3DMODE(f->mode) == PKW_inputOutput)
	{

		//Q. how do I do this? Just a memcpy on anyVrml or ???
		//printf("size of anyVrml=%d\n",sizeof(union anyVrml));
		//printf("f->mode=%d  f->type=%d fieldptr mode=%d type=%d\n",f->mode,f->type,mode,type);
		//heapdump();
		//isMF = type % 2;
		//sftype = type - isMF;
		//from EAI_C_CommonFunctions.c
		//isize = returnElementLength(sftype) * returnElementRowSize(sftype);
		shallow_copy_field(type, &(f->defaultVal) , fieldPtr);
		//memcpy(fieldPtr,&(f->defaultVal),isize); //sizeof(union anyVrml));
		//heapdump();
		//if(0)//should probably only do this for the final deep_copy sceneInstance, not here during parsing
		//if(source==1) 
		//{
		//	scriptFieldDecl_jsFieldInit(
		//		(struct ScriptFieldDecl*) fdecl, 
		//		((struct Shader_Script*)X3D_SCRIPT(node)->__scriptObj)->num);
		//}

	}
	//b) register it in the IS-table for our context
	broto_store_IS(proto,protoFieldName,X3DMODE(f->mode),iprotofield,type,
					node,nodeFieldName,X3DMODE(mode),ifield,source);
	/* Add this scriptfielddecl to the list of script fields mapped to this proto field */
	//sfield = newScriptFieldInstanceInfo(sdecl, script);
	//vector_pushBack(struct ScriptFieldInstanceInfo*, pField->scriptDests, sfield);
	//we need an IS-table for builtin node targets - what did CProto do?
	//defaultVal = &(f->defaultVal);
	
	//
	FREEUP
	return TRUE;

}

/* note that we get the resources in a couple of steps; this tries to keep the scenegraph running 
	copied from load_Inline

*/

resource_item_t * resLibraryAlreadyRequested(resource_item_t *res){
	/* for externProto libraries, check if there's already a resitem launched for the library
		and if so, return the associated resitem, else return null.
		- compare the absolutized (resource-identified) before-# 
		- exampe for hddp://something.com/mylibrary.wrl#myProto1 we'll compare hddp://something.com/mylibrary.wrl
	*/
	void3 *ulr;
	ulr = librarySearch(res->URLrequest);
	if(ulr) return ulr->three; //resource
	return NULL;
}


/* EXTERNPROTO library status */
#define LOAD_INITIAL_STATE 0
#define LOAD_REQUEST_RESOURCE 1
#define LOAD_FETCHING_RESOURCE 2
#define LOAD_PARSING 3
#define LOAD_STABLE 10
/* dug9 Sept 2014 wrapper technique for externProto for useBrotos:
	PD - proto declare - a type declaration (shallow copies of contained protos; allocated nodes not registered)
	PI - proto insstance - a node for the scenegraph to render (deep copies of contained protos; nodes registered)
	EPD - extern PD
	EPI - extern PI
	Wrapper technique:
		EPD { url, resource, loadstatus, PD once loaded  }
		EPI { *EPD, PI once loaded }
	1. during parsing, when an ExternProtoDeclare is encountered, an empty X3D_Proto is created
		to represent the ExternProtoDeclare EPD as a type, and fields as declared are added, along
		with the url. No body. A flag is set saying it's not loaded. 
	2. parsing continues. When an instance of that type is encountered, a copy of the EPD is 
		created as an externProtoInstance EPI -also empty, and with a pointer back to EPD. Parsing
		continues, including routing to the EPI
	3. during rendering when the EPI is visited (currently in startofloopnodeupdates()) it 
		checks itself to see if its been instanced yet, and 
		a)	if not loaded:- checks to see if the EPD has been loaded yet, 
			i) if not, gives a time slice to the EPD when visiting it, see #4
			ii) if yes, then instances itself, and generates routes between the wrapper EPI and contained PI
		b) if loaded, renders or whatever its supposed to be doing as a normal node
	4. EPD when visited 
		a) checks to see if the resource has been created, if not creates it. 
			It checks to see if another EPD has already requested the URL (in the case of a proto library)
			and if so, uses that EPD's resource, else submits a new resource for fetching
		b) if resource is loaded, then i) if its in a proto library, fetches the #name else ii) takes the first proto
			and sets it in its __protoDeclare list, and marks itself loaded
*/

void load_externProtoDeclare (struct X3D_Proto *node) {
	resource_item_t *res, *res2;
	// printf ("load_externProto %u, loadStatus %d loadResource %u\n",node, node->__loadstatus, node->__loadResource);

    //printf ("load_externProto, node %p loadStatus %d\n",node,node->load);
	char flagInstance, flagExtern;
	flagInstance = ciflag_get(node->__protoFlags,2);
	flagExtern = ciflag_get(node->__protoFlags,3);
	if(flagInstance == 0 && flagExtern == 1) { 
		/* printf ("loading externProtoDeclare\n");  */

		switch (node->__loadstatus) {
			case LOAD_INITIAL_STATE: /* nothing happened yet */

			if (node->url.n == 0) {
				node->__loadstatus = LOAD_STABLE; /* a "do-nothing" approach */
				break;
			} else {
				res = resource_create_multi(&(node->url));
				res->media_type = resm_unknown;
				node->__loadstatus = LOAD_REQUEST_RESOURCE;
				node->__loadResource = res;
			}
			break;

			case LOAD_REQUEST_RESOURCE:
			res = node->__loadResource;
			resource_identify(node->_parentResource, res);
			node->__afterPound = (void*)res->afterPoundCharacters;
			//check if this is a library request, and if so has it already been requested
			res2 = NULL;
			if(node->__afterPound)
				res2 = resLibraryAlreadyRequested(res);
			if(res2){
				node->__loadResource = res2;
			}else{
				/* printf ("load_Inline, we have type  %s  status %s\n",
					resourceTypeToString(res->type), resourceStatusToString(res->status)); */
				res->actions = resa_download | resa_load; //not resa_parse which we do below
				struct X3D_Proto *libraryScene = createNewX3DNode0(NODE_Proto);
				res->whereToPlaceData = X3D_NODE(libraryScene);
				res->offsetFromWhereToPlaceData = offsetof (struct X3D_Proto, __children);
				addLibrary(res->URLrequest,libraryScene,res);
				resitem_enqueue(ml_new(res));
			}
			node->__loadstatus = LOAD_FETCHING_RESOURCE;
			break;

			case LOAD_FETCHING_RESOURCE:
			res = node->__loadResource;
			/* printf ("load_Inline, we have type  %s  status %s\n",
				resourceTypeToString(res->type), resourceStatusToString(res->status)); */
			if(res->complete){
				if (res->status == ress_loaded) {
					res->actions = resa_process;
					res->complete = FALSE;
					resitem_enqueue(ml_new(res));
				} else if ((res->status == ress_failed) || (res->status == ress_invalid)) {
					//no hope left
					printf ("resource failed to load\n");
					node->__loadstatus = LOAD_STABLE; // a "do-nothing" approach 
				} else	if (res->status == ress_parsed) {
					//fetch the particular desired PROTO from the libraryScene, and place in _protoDeclares
					//or in _prototype
					struct X3D_Proto *libraryScene = res->whereToPlaceData; //from above
					if(node->__externProtoDeclares == NULL)
						node->__externProtoDeclares = newVector(struct X3D_Proto*,1);
					vector_pushBack(struct X3D_Proto*,node->__externProtoDeclares,libraryScene);

					if(node->__externProtoDeclares){
						int n = vectorSize(node->__externProtoDeclares);
						if(n){
							struct X3D_Proto *libraryScene = vector_get(struct X3D_Proto*,node->__externProtoDeclares,0);
							if(libraryScene->__protoDeclares){
								int m = vectorSize(libraryScene->__protoDeclares);
								if(m){
									int k;
									struct X3D_Proto* pDefinition;
									/* the specs say if there's no # in the url, take the first PROTO definition in the library file
										else match #name (afterpound in resitem) 
									*/
									if(node->__afterPound){
										for(k=0;k<m;k++){
											char *typename, *matchstring;
											//matchstring = node->__typename; //specs don't say this
											matchstring = node->__afterPound;
											pDefinition = vector_get(struct X3D_Proto*,libraryScene->__protoDeclares,k);
											typename = (char *)pDefinition->__typename;
											if(typename)
												if(!strcmp(matchstring,typename)){
													//found it - the library proto's user type name matches this extern proto's #name (afterPound)
													//now add this protoDeclare to our externProto's protoDeclare list in 1st position
													node->__protoDeclares = newVector(struct X3D_Proto*,1);
													vector_pushBack(struct X3D_Proto*,node->__protoDeclares,pDefinition);
													break;
												}
										} //for k
									}else{
										//no afterPound, so according to specs: take the first proto in the file
										pDefinition = vector_get(struct X3D_Proto*,libraryScene->__protoDeclares,0);
										node->__protoDeclares = newVector(struct X3D_Proto*,1);
										vector_pushBack(struct X3D_Proto*,node->__protoDeclares,pDefinition);
									} //else no afterPound
								} //if(m)
							} //if(libraryScene->__protoDeclares)
						} //if(n)
					} //if(node->__externProtoDeclares)
					node->__loadstatus = LOAD_STABLE; 
				} //if (res->status == ress_parsed)
			} //if(res->complete)
			//end case LOAD_FETCHING_RESOURCE
			break;

			case LOAD_STABLE:
			break;
		}

	} 
}
void load_externProtoInstance (struct X3D_Proto *node) {
	resource_item_t *res;
	// printf ("load_externProto %u, loadStatus %d loadResource %u\n",node, node->__loadstatus, node->__loadResource);

    //printf ("load_externProto, node %p loadStatus %d\n",node,node->load);
	char flagInstance, flagExtern;
	flagInstance = ciflag_get(node->__protoFlags,2);
	flagExtern = ciflag_get(node->__protoFlags,3);
	if(flagInstance == 1 && flagExtern == 1) { //if protoInstance and extern
		struct X3D_Proto *pnode = NULL;
		if(node->__children.n) return; //externProtoInstance body node already instanced
		pnode = (struct X3D_Proto*)node->__prototype;
		if(pnode) {
			if(pnode->__loadstatus != LOAD_STABLE){
				// extern proto declare not loaded yet, give it a time slice to check its resource
				load_externProtoDeclare(pnode);
			}
			if(pnode->__loadstatus == LOAD_STABLE){
				// externProtoDeclare may already be loaded, if so, we just need to instance it
				if(pnode->__protoDeclares){
					int n = vectorSize(pnode->__protoDeclares);
					if(n){
						struct X3D_Proto *pdeclare, *pinstance;
						pdeclare = vector_get(struct X3D_Proto*,pnode->__protoDeclares,0);
						pinstance = brotoInstance(pdeclare,1);
						if (pinstance != NULL) {
							struct ProtoDefinition *ed, *pd;
							struct Vector *ei, *pi;
							struct ProtoFieldDecl *ef, *pf;
							ed = node->__protoDef;
							ei = ed->iface;
							pd = pinstance->__protoDef;
							pi = pd->iface;

							//inject IS routes
							{
								int i,j;
								char *ename, *pname;

								//match fields to create IStable
								for(i=0;i<ei->n;i++){
									ef = protoDefinition_getFieldByNum(ed, i);
									ename = ef->cname;
									for(j=0;j<pi->n;j++){
										pf = protoDefinition_getFieldByNum(pd, j);
										pname = pf->cname;
										if(!strcmp(ename,pname)){
											//name match
											printf("ename = %s pname = %s\n",ename,pname);
											if(ef->type == pf->type){
												//type match
												printf("etype = %d ptype = %d\n",ef->type, pf->type);
												//add IS
												broto_store_IS(node,ef->cname,ef->mode, i, ef->type,	X3D_NODE(pinstance), pf->cname, pf->mode, j, 3);
											}
										}
									}
								}
								//convert IStable to browser routes
								struct Vector *p2p = newVector(struct pointer2pointer*,1);
								struct pointer2pointer *p2pentry = malloc(sizeof(struct pointer2pointer));
								//nothing to look up, nuisance to re-use copy_IS
								p2pentry->pp = X3D_NODE(pinstance);
								p2pentry->pn = X3D_NODE(pinstance);
								vector_pushBack(struct pointer2pointer*,p2p,p2pentry);
								copy_IS(node->__IS, node, p2p);
							}
							//copy EPI field initial values to contained PI
							{
								int i;

								Stack *istable = (Stack*) node->__IS;
								if(istable != NULL)
								for(i=0;i<istable->n;i++)
								{
									struct brotoIS* is;
									int ifield, iprotofield;
									is = vector_get(struct brotoIS*, istable, i);
									if(is->pmode == PKW_inputOutput || is->pmode == PKW_initializeOnly){ 
										if(is->mode == PKW_inputOutput || is->mode == PKW_initializeOnly){
											ef = protoDefinition_getFieldByNum(ed, is->iprotofield);
											pf = protoDefinition_getFieldByNum(pd, is->ifield);
											if(ef->alreadySet)
											//if(ef->defaultVal.sffloat != 0.0f)
												memcpy(&pf->defaultVal,&ef->defaultVal, sizeof(union anyVrml));
										}
									}
								}
							}
							//now copy broto body, which should cascade inital values down
							deep_copy_broto_body2(&pdeclare,&pinstance);
                			AddRemoveChildren(X3D_NODE(node), &node->__children, &X3D_NODE(pinstance), 1, 1,__FILE__,__LINE__);
							add_parent(X3D_NODE(pinstance),X3D_NODE(node),__FILE__,__LINE__);
						} //if (pinstance != NULL) 
					}
				}
			}
		}
	}
}
