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
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldSet.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CProto.h"
#include "../vrml_parser/CParse.h"
#include "../vrml_parser/CRoutes.h"
#include "../input/EAIHeaders.h"	/* resolving implicit declarations */
#include "../input/EAIHelpers.h"	/* resolving implicit declarations */

#include "X3DParser.h"
#include "X3DProtoScript.h"

#include <libxml/parser.h>
typedef xmlSAXHandler* XML_Parser;

/* for now - fill this in later */
#define XML_GetCurrentLineNumber(aaa) (int)999


#define XML_ParserFree(aaa) FREE_IF_NZ(aaa)
#define XML_SetUserData(aaa,bbb)
#define XML_STATUS_ERROR -1
//OLDCODE #define XML_GetErrorCode(aaa)
//OLDCODE #define XML_ErrorString(aaa) "errors not currently being reported by libxml port"

struct xml_user_data{
	Stack *context;
	Stack *nodes;
	Stack *atts;
	Stack *modes;
	Stack *fields;
};
struct xml_user_data *new_xml_user_data(){
	struct xml_user_data *ud = MALLOCV(sizeof(struct xml_user_data));
	ud->context = ud->nodes = ud->atts  = ud->modes = ud->fields = NULL;
	ud->context = newVector(struct X3D_Node*,256);
	ud->context->n = 0;
	ud->nodes = newVector(struct X3D_Node*,256);
	ud->nodes->n = 0;
	ud->atts = newVector(void*,256);
	ud->atts->n = 0;
	ud->modes = newVector(int,256);
	ud->modes->n = 0;
	ud->fields = newVector(char *,256);
	ud->fields->n = 0;
	return ud;
}
void free_xml_user_data(struct xml_user_data *ud){
	if(ud){
		deleteVector(struct X3D_Node*,ud->context);
		deleteVector(struct X3D_Node*,ud->nodes);
		deleteVector(void*,ud->atts);
		deleteVector(void*,ud->modes);
		deleteVector(void*,ud->fields);
		FREE_IF_NZ(ud);
	}
}
//for push,pop,get the index is the vector index range 0, n-1. 
// Or going from the top top= -1, parent to top = -2.
#define TOP -1
//#define BOTTOM 0

//currently context isn't a separate struct, its part of X3D_Proto and X3D_Inline, which have
//the same structure, and can be cross-cast, and represent a web3d executionContext or context for short
//and that includes DEFnames, ROUTES, protoDeclares, externProtoDeclares, IMPORTS,EXPORTS,scripts
void pushContext(void *userData, struct X3D_Node* context){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	if(context->_nodeType != NODE_Proto && context->_nodeType != NODE_Inline)
		printf("attempt to cast a node of type %d to Proto\n",context->_nodeType);
	stack_push(struct X3D_Proto*,ud->context,X3D_PROTO(context));
}
struct X3D_Proto* getContext(void *userData, int index){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	//return stack_top(struct X3D_Node*,ud->context);
	if(index < 0)
		return vector_get(struct X3D_Proto*,ud->context, vectorSize(ud->context)+index);
	else
		return vector_get(struct X3D_Proto*,ud->context, index);
}
void popContext(void *userData){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	stack_pop(struct X3D_Proto*,ud->context);
}

void pushNode(void *userData,struct X3D_Node* node){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	stack_push(struct X3D_Node*,ud->nodes,node);
	stack_push(void* ,ud->atts,NULL);
}
struct X3D_Node* getNode(void *userData, int index){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	if(index < 0)
		return vector_get(struct X3D_Node*,ud->nodes, vectorSize(ud->nodes)+index);
	else
		return vector_get(struct X3D_Node*,ud->nodes, index);
}
void* getAtt(void *userData, int index){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	//return stack_top(void*,ud->atts);
	if(index < 0)
		return vector_get(void* ,ud->atts, vectorSize(ud->atts)+index);
	else
		return vector_get(void* ,ud->atts, index);
}
void popNode(void *userData){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	stack_pop(struct X3D_Node*,ud->nodes);
	stack_pop(void* ,ud->atts);
	//stack_pop(void* ,ud->childs);
}
int getNodeTop(void *userData){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	return vectorSize(ud->nodes)-1;
}
void setAtt(void *userData, int index, void *att){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	//return stack_top(void*,ud->atts);
	if(index < 0)
		vector_set(void* ,ud->atts, vectorSize(ud->atts)+index, att);
	else
		vector_set(void* ,ud->atts, index, att);
}
void pushMode(void *userData, int parsingmode){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	stack_push(int,ud->modes,parsingmode);
}
int getMode(void *userData, int index){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	//return stack_top(int,ud->modes);
	if(index < 0)
		return vector_get(int,ud->modes, vectorSize(ud->modes)+index);
	else
		return vector_get(int,ud->modes, index);
}
void popMode(void *userData){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	stack_pop(int,ud->modes);
}

void pushField(void *userData, char *fname){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	stack_push(char *,ud->fields,fname);
	if(0) printf("push n=%d\n",ud->fields->n);
}
char * getField(void *userData, int index){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	if(0) printf("get n=%d\n",ud->fields->n);
	if(index < 0)
		return vector_get(char *,ud->fields, vectorSize(ud->fields)+index);
	else
		return vector_get(char *,ud->fields, index);
}

void popField(void *userData){
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	stack_pop(char *,ud->fields);
	if(0) printf("pop n=%d\n",ud->fields->n);
}

static int XML_ParseFile(xmlSAXHandler *me, void *user_data, const char *myinput, int myinputlen, int recovery) {

	if (xmlSAXUserParseMemory(me, user_data, myinput,myinputlen) == 0) return 0;
	return XML_STATUS_ERROR;
}


/* basic parser stuff */
#define XML_CreateParserLevel(aaa) \
	aaa = MALLOC(xmlSAXHandler *, sizeof (xmlSAXHandler)); \
	bzero (aaa,sizeof(xmlSAXHandler));

#define XML_SetElementHandler(aaa,bbb,ccc) \
	aaa->startElement = bbb; \
	aaa->endElement = ccc; 

/* CDATA handling */
#define XML_SetDefaultHandler(aaa,bbb) /* this is CDATA related too */
#define XML_SetCdataSectionHandler(aaa,bbb,ccc) \
	aaa->cdataBlock = endCDATA;



//#define X3DPARSERVERBOSE 1

#define PROTO_MARKER 567000

/* If XMLCALL isn't defined, use empty one */
#ifndef XMLCALL
 #define XMLCALL
#endif /* XMLCALL */

//#define MAX_CHILD_ATTRIBUTE_DEPTH 32

typedef struct pX3DParser{
	struct VRMLLexer *myLexer;// = NULL;
	Stack* DEFedNodes;// = NULL;
	int CDATA_TextMallocSize;// = 0;
	/* for testing Johannes Behrs fieldValue hack for getting data in */
	int in3_3_fieldValue;// = FALSE;
	int in3_3_fieldIndex;// = INT_ID_UNDEFINED;
	/* XML parser variables */
	int X3DParserRecurseLevel;// = INT_ID_UNDEFINED;
	XML_Parser x3dparser[PROTOINSTANCE_MAX_LEVELS];
	XML_Parser currentX3DParser;// = NULL;

	int currentParserMode[PROTOINSTANCE_MAX_LEVELS];
	int currentParserModeIndex;// = 0; //INT_ID_UNDEFINED;
	struct xml_user_data *user_data;

}* ppX3DParser;
void *X3DParser_constructor(){
	void *v = MALLOCV(sizeof(struct pX3DParser));
	memset(v,0,sizeof(struct pX3DParser));
	return v;
}
void X3DParser_init(struct tX3DParser *t){
	//public
	t->parentIndex = -1;
	t->CDATA_Text = NULL;
	t->CDATA_Text_curlen = 0;
	//private
	t->prv = X3DParser_constructor();
	{
		ppX3DParser p = (ppX3DParser)t->prv;
		p->myLexer = NULL;
		p->DEFedNodes = NULL;
		//p->childAttributes= NULL;
		p->CDATA_TextMallocSize = 0;
		/* for testing Johannes Behrs fieldValue hack for getting data in */
		p->in3_3_fieldValue = FALSE;
		p->in3_3_fieldIndex = INT_ID_UNDEFINED;
		/* XML parser variables */
		p->X3DParserRecurseLevel = INT_ID_UNDEFINED;
		p->currentX3DParser = NULL;

		p->currentParserModeIndex = 0; //INT_ID_UNDEFINED;
		p->user_data = NULL;

	}
}
void X3DParser_clear(struct tX3DParser *t){
	if(t){
		ppX3DParser p = (ppX3DParser)t->prv;
		free_xml_user_data(p->user_data);
		if(p->myLexer){
			lexer_destroyData(p->myLexer);
			FREE_IF_NZ(p->myLexer);
		}
	}
}

	//ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;




#ifdef X3DPARSERVERBOSE
static const char *parserModeStrings[] = {
		"unused",
		"PARSING_NODES",
		"PARSING_SCRIPT",
		"PARSING_PROTODECLARE ",
		"PARSING_PROTOINTERFACE ",
		"PARSING_PROTOBODY",
		"PARSING_PROTOINSTANCE",
		"PARSING_IS",
		"PARSING_CONNECT",
		"PARSING_EXTERNPROTODECLARE",
		"unused high"};
#endif
#undef X3DPARSERVERBOSE
		

/* get the line number of the current parser for error purposes */
int freewrl_XML_GetCurrentLineNumber(void) {
	ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;
	if (p->X3DParserRecurseLevel > INT_ID_UNDEFINED)
	{
		p->currentX3DParser = p->x3dparser[p->X3DParserRecurseLevel]; /*dont trust current*/
		return (int) XML_GetCurrentLineNumber(p->currentX3DParser); 
	}
	return INT_ID_UNDEFINED;
}


/*
in3_3:

2b) Allow <fieldValue> + extension for all node-types
-------------------------------------------------------------
There is already a fieldValue element in current X3D-XML
spec to specify the value of a ProtoInstance-field.
To specify a value of a ProtoInstance field looks like this:

<ProtoInstance name='bar' >
  <fieldValue name='foo' value='TRUE' />
</ProtoInstance>

We could change the wording in the spec to allow
<fieldValue>- elements not just for ProtoInstance-nodes
but for all instances of nodes. In addition we would
allow element data in fieldValues which will
be read to the single specified field. This solution
is not limited to a single field per node but could
easily handle any number of fields

The 'count' attribute is optional and an idea
borrowed form the COLLADA specification. The count-value
could be used to further improve the parser-speed because
the parser could already reserve the amount of data needed.

<Coordinate3d>
  <fieldValue name='point' count='4' >
   0.5 1.0 1.0
   2.0 2.0 2.0
   3.0 0.5 1.5
   4.0 1.4 2.0
  </fieldValue>
</Coordinate3d>

pro:
+ Dynamic solution; we do not have to tag fields
+ Long term solution, no one-field-per-node limitation
+ Uses an existing element/concept. No additional complexity
+ Works with protos

con:
- Introduces one additional element in the code; data looks not as compact as 2a
- Allowing attribute and element-data for a single field is redundant; Need to specify how to handle ambiguities


*/

/* add this data to the end of the current CData array for later use */
static void appendDataToFieldValue(char *data, int len) {
	ttglobal tg = gglobal();
	ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;
	if ((tg->X3DParser.CDATA_Text_curlen+len) > p->CDATA_TextMallocSize-100) {
		while ((tg->X3DParser.CDATA_Text_curlen+len) > p->CDATA_TextMallocSize-100) {
			if (p->CDATA_TextMallocSize == 0) p->CDATA_TextMallocSize = 2048;
			else p->CDATA_TextMallocSize *= 2;
		}
		tg->X3DParser.CDATA_Text = REALLOC (tg->X3DParser.CDATA_Text,p->CDATA_TextMallocSize);
	}

	memcpy(&tg->X3DParser.CDATA_Text[tg->X3DParser.CDATA_Text_curlen],data,len);
	tg->X3DParser.CDATA_Text_curlen+=len;
	tg->X3DParser.CDATA_Text[tg->X3DParser.CDATA_Text_curlen]='\0';
}

void endProtoInstanceFieldTypeNode(void *ud, const char *name);
static void endProtoInstanceField(void *ud, const char *name);
/* we are finished with a 3.3 fieldValue, tie it in */
static void setFieldValueDataActive(void *ud,const char* name) {
	ttglobal tg = gglobal();
	ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;

	if (!p->in3_3_fieldValue) 
		endProtoInstanceField(ud,name);
	else
	{
		//printf ("expected this to be in a fieldValue\n");

		/* if we had a valid field for this node... */
		if (p->in3_3_fieldIndex != INT_ID_UNDEFINED) {

#ifdef X3DPARSERVERBOSE
			printf ("setFieldValueDataActive field %s, parent is a %s\n",
				stringFieldType(in3_3_fieldIndex),stringNodeType(parentStack[parentIndex]->_nodeType)); 
#endif

			setField_fromJavascript (getNode(ud,TOP), (char *) stringFieldType(p->in3_3_fieldIndex),
				tg->X3DParser.CDATA_Text, TRUE);
		} else {

			printf ("in a end field tag, what should we do here?? \n");
		}

		/* free data */
		p->in3_3_fieldValue = FALSE;
		tg->X3DParser.CDATA_Text_curlen = 0;
		p->in3_3_fieldIndex = INT_ID_UNDEFINED;
	}
}


/**************************************************************************************/

/* for EAI/SAI - if we have a Node, look up the name in the DEF names */
char *X3DParser_getNameFromNode(struct X3D_Node* myNode) {
	indexT ind;
	struct X3D_Node* node;
	ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;

	/* printf ("X3DParser_getNameFromNode called on %u, DEFedNodes %u\n",myNode,DEFedNodes); */
	if (!p->DEFedNodes) return NULL;
	/* printf ("X3DParser_getNameFromNode, DEFedNodes not null\n"); */

	/* go through the DEFed nodes and match the node pointers */
	for (ind=0; ind<vectorSize(stack_top(struct Vector*, p->DEFedNodes)); ind++) {
		node=vector_get(struct X3D_Node*, stack_top(struct Vector*, p->DEFedNodes),ind);
		
		/* did we have a match? */
		/* printf ("X3DParser_getNameFromNode, comparing %u and %u at %d\n",myNode,node,ind); */
		if (myNode == node) {
			/* we have the index into the lexers name table; return the name */
			struct Vector *ns;
			ns = stack_top(struct Vector*, p->myLexer->userNodeNames);
			return ((char *)vector_get (const char*, ns,ind));
		}
	}

	/* not found, return NULL */
	return NULL;
}

/* for EAI/SAI - if we have a DEF name, look up the node pointer */
struct X3D_Node *X3DParser_getNodeFromName(const char *name) {
	return DEFNameIndex(name,NULL,FALSE);
}

/**************************************************************************************/



/* "forget" the DEFs. Keep the table around, though, as the entries will simply be used again. */
void kill_X3DDefs(void) {
	int i;
	ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;

	//FREE_IF_NZ(p->childAttributes);
	//p->childAttributes = NULL;

	if (p->DEFedNodes != NULL) {
		for (i=0; i<vectorSize(p->DEFedNodes); i++) {
			struct Vector * myele = vector_get (struct Vector*, p->DEFedNodes, i);

			/* we DO NOT delete individual elements of this vector, as they are pointers
			   to struct X3D_Node*; these get deleted in the general "Destroy all nodes
			   and fields" routine; kill_X3DNodes(void). */
			deleteVector (struct Vector *,myele);
		}
		deleteVector(struct Vector*, p->DEFedNodes);
		p->DEFedNodes = NULL;
	}

	/* now, for the lexer... */
	if (p->myLexer != NULL) {
		lexer_destroyData(p->myLexer);
		deleteLexer(p->myLexer);
		p->myLexer=NULL;
	}

	/* do we have a parser for scanning string values to memory? */
	Parser_deleteParserForScanStringValueToMem();
}



/* return a node assoicated with this name. If the name exists, return the previous node. If not, return
the new node */
struct X3D_Node *DEFNameIndex (const char *name, struct X3D_Node* node, int force) {
	indexT ind = ID_UNDEFINED;
	ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;

#ifdef X3DPARSERVERBOSE
	printf ("DEFNameIndex, looking for :%s:, force %d nodePointer %u\n",name,force,node);
#endif
	/* lexer_defineNodeName is #defined as lexer_defineID(me, ret, stack_top(struct Vector*, userNodeNames), TRUE) */
	/* Checks if this node already exists in the userNodeNames vector.  If it doesn't, adds it. */

	if (p->myLexer == NULL) return NULL;
	lexer_forceStringCleanup(p->myLexer); 
	lexer_fromString(p->myLexer,STRDUP(name));

	if(!lexer_defineNodeName(p->myLexer, &ind))
		printf ("Expected nodeNameId after DEF!\n");

#ifdef X3DPARSERVERBOSE
	printf ("DEF returns id of %d for %s\n",ind,name);
#endif

	ASSERT(ind<=vectorSize(stack_top(struct Vector*, p->DEFedNodes)));

#ifdef X3DPARSERVERBOSE
	printf ("so, in DEFNameIndex, we have ind %d, vectorSize %d\n",ind,vectorSize(stack_top(struct Vector*, DEFedNodes)));
#endif

	if(ind==vectorSize(stack_top(struct Vector*, p->DEFedNodes))) {
		vector_pushBack(struct X3D_Node*, stack_top(struct Vector*, p->DEFedNodes), node);
	}
	ASSERT(ind<vectorSize(stack_top(struct Vector*, p->DEFedNodes)));

	/* if we did not find this node, just return */
	if (ind == ID_UNDEFINED) {return NULL; }

	node=vector_get(struct X3D_Node*, stack_top(struct Vector*, p->DEFedNodes),ind);

#ifdef X3DPARSERVERBOSE
	if (node != NULL) printf ("DEFNameIndex for %s, returning %u, nt %s\n",name, node,stringNodeType(node->_nodeType));
	else printf ("DEFNameIndex, node is NULL\n");
#endif

	return node;
}


/* look through the script fields for this field, and return the values. */
static int getFieldFromScript (struct VRMLLexer *myLexer, char *fieldName, struct Shader_Script *me, int *offs, int *type, int *accessType) {

	struct ScriptFieldDecl* myField;

	/* initialize */
	myField = NULL;

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldFromScript, looking for %s\n",fieldName);
	#endif

	/* go through the user arrays in this lexer, and see if we have a match */

	myField = script_getField_viaCharName (me, fieldName);
	//printf ("try2: getFieldFromScript, field %s is %p\n",fieldName,myField); 

	if (myField != NULL) {
		int myFieldNumber;

		/* is this a script? if so, lets do the conversion from our internal lexer name index to
		   the scripting name index. */
		if (me->ShaderScriptNode->_nodeType == NODE_Script) {
			/* wow - have to get the Javascript text string index from this one */
			myFieldNumber = JSparamIndex(fieldName,stringFieldtypeType(
				fieldDecl_getType(myField->fieldDecl))); 

			*offs=myFieldNumber;


		} else {
			*offs = fieldDecl_getIndexName(myField->fieldDecl);
		}
		*type = fieldDecl_getType(myField->fieldDecl);
		/* go from PKW_xxx to KW_xxx  .... sigh ... */
		*accessType = mapToKEYWORDindex(fieldDecl_getAccessType(myField->fieldDecl));
		return TRUE;
	}

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldFromScript, did not find field %s in script\n",fieldName);
	#endif
	
	/* did not find it */
	*offs = INT_ID_UNDEFINED;  *type = 0;
	return FALSE;

}
#undef X3DPARSERVERBOSE


int getRoutingInfo (struct VRMLLexer *myLexer, struct X3D_Node *node, int *offs, int* type, int *accessType, struct Shader_Script **myObj, char *name, int routeTo) {
	int error;
	int fieldInt;


#ifdef X3DPARSERVERBOSE
	printf ("getRoutingInfo, node %p name %s\n",node,name);
	printf ("getRoutingInfo, nt %s\n",stringNodeType(node->_nodeType));
#endif
	error = FALSE;
	switch (node->_nodeType) {

	case NODE_Script: {
		*myObj = (struct Shader_Script *) X3D_SCRIPT(node)->__scriptObj;
		error = !(getFieldFromScript (myLexer, name,*myObj,offs,type,accessType));
		break; }
	case NODE_ComposedShader: {
		*myObj = (struct Shader_Script *) X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields;
		error = !(getFieldFromScript (myLexer, name,*myObj,offs,type,accessType));
		break; }
	case NODE_ShaderProgram: {
		*myObj = (struct Shader_Script *) X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields;
		error = !(getFieldFromScript (myLexer, name,*myObj,offs,type,accessType));        
		break; }
	case NODE_PackagedShader: {
		*myObj = (struct Shader_Script *) X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields;
		error = !(getFieldFromScript (myLexer, name,*myObj,offs,type,accessType));
		break; }
    case NODE_ProgramShader: {
			int i;

			// assume we have an error, unless we find this in the PackagedShader field
			error = true;

			// a ProgramShader has potentially lots of ShaderPrograms in the "programs" field...
			//ConsoleMessage ("have a PRogramShder here, it has %d programs ",X3D_PROGRAMSHADER(node)->programs.n);
			for (i=0; i<X3D_PROGRAMSHADER(node)->programs.n; i++) {
				struct X3D_ShaderProgram *ps = X3D_SHADERPROGRAM(X3D_PROGRAMSHADER(node)->programs.p[i]);
				//ConsoleMessage ("ProgramShader program %d is %p",i,ps);
				if (ps != NULL) {
					int tmpOfs, tmpType, terror;

					//ConsoleMessage ("ProgramShader, child %d is a %s",i,stringNodeType(ps->_nodeType));
					*myObj = (struct Shader_Script *) ps->_shaderUserDefinedFields;
					//ConsoleMessage (".... and the userDefinedFields for this one is %p",*myObj);
					terror = !(getFieldFromScript (myLexer, name,*myObj,&tmpOfs,&tmpType,accessType));
					//ConsoleMessage ("have error %s",terror?" ok ":" not ok" );

					if (!terror) {
						//ConsoleMessage ("found it");
						*offs = tmpOfs; *type = tmpType;
						error = false; // ie, no error
					}

				}
			}
			break;}
	default:
		*myObj=NULL;

		/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
		fieldInt = findRoutedFieldInFIELDNAMES (node, name, routeTo);

		if (fieldInt >=0) { findFieldInOFFSETS(node->_nodeType,fieldInt, offs, type, accessType);
		} else {
			/* do NOT set error here; this might be a PROTO expansion and more work is needed */
			*offs=INT_ID_UNDEFINED;
			*type=INT_ID_UNDEFINED;
		}
	}
	//if (error) ConsoleMessage ("getRoutingInfo an error"); else ConsoleMessage ("getRoutingInfo ok");

	return error;
}


static int getRouteField (struct VRMLLexer *myLexer, struct X3D_Node **innode, int *offs, int* type, char *name, int routeTo) {
	int error;
	int fieldInt;
	int accessType;
	struct X3D_Node *node;
	struct Shader_Script *holder;

	node = *innode; /* ease of use - contents of pointer in param line */
 
	//printf ("start of getRouteField...\n");

	error = getRoutingInfo(myLexer,node,offs,type,&accessType, &holder, name,routeTo);

	//if (error) ConsoleMessage ("getRouteField,  after getRoutingInfo an error"); else ConsoleMessage ("getRouteField, getRoutingInfo ok");
	//printf ("getRouteField, offs %d type %d\n",*offs, *type); 

	if ((*offs <0) && isProto(node)) 
	{
		/* is this a PROTO expansion? */
		if (node->_nodeType==NODE_Group) 
		{
			struct X3D_Group *myg;
			int myp;

			/* lets go finding; if this is a PROTO expansion, we will have FreeWRL__protoDef != INT_ID_UNDEFINED */
			myg = X3D_GROUP(node);

			/* printf ("routing, looking to see if this is a proto expansion... myg %u\n",myg); */
			myp = myg->FreeWRL__protoDef;

			if (myp != INT_ID_UNDEFINED) 
			{
				char newname[1000];
				struct X3D_Node *newn;
				/* printf ("we are routing to an X3D PROTO Expansion\n");
				printf ("looking for name %s\n",name); */
				sprintf (newname,"%s_%s_%d",name,FREEWRL_SPECIFIC,myp);
				/* printf ("and, defined name is %s\n",newname); */
				/* look up this node; if it exists, look for field within it */
				newn = DEFNameIndex ((const char *)newname, NULL, FALSE);

				/* printf ("newn is %u\n",newn); */
				if (newn!=NULL)
				{
					/* printf ("newn node type %s\n",stringNodeType(newn->_nodeType)); */
					if (routeTo == 0) 
					{
						/* printf ("and we are routing FROM this proto expansion\n"); */
						fieldInt = findRoutedFieldInFIELDNAMES(newn,"valueChanged",routeTo);
					} else {
						/* printf ("and, routing TO this proto expansion\n"); */
						fieldInt = findRoutedFieldInFIELDNAMES(newn,"setValue",routeTo);
					}
					if (fieldInt >=0) 
					{
						findFieldInOFFSETS(newn->_nodeType, 
						fieldInt, offs, type, &accessType);
						*innode = newn; /* pass back this new node for routing */
					}
				}
			}
		}
		//if (node->_nodeType==NODE_Proto)
		//{
		//	struct X3D_Proto *myg;
		//	int myp;

		//	/* lets go finding; if this is a PROTO expansion, we will have FreeWRL__protoDef != INT_ID_UNDEFINED */
		//	myg = X3D_PROTO(node);

		//	/* printf ("routing, looking to see if this is a proto expansion... myg %u\n",myg); */
		//	myp = myg->FreeWRL__protoDef;

		//	if (myp != INT_ID_UNDEFINED) 
		//	{
		//		char newname[1000];
		//		struct X3D_Node *newn;
		//		/* printf ("we are routing to an X3D PROTO Expansion\n");
		//		printf ("looking for name %s\n",name); */
		//		sprintf (newname,"%s_%s_%d",name,FREEWRL_SPECIFIC,myp);
		//		/* printf ("and, defined name is %s\n",newname); */
		//		/* look up this node; if it exists, look for field within it */
		//		newn = DEFNameIndex ((const char *)newname, NULL, FALSE);

		//		/* printf ("newn is %u\n",newn); */
		//		if (newn!=NULL)
		//		{
		//			/* printf ("newn node type %s\n",stringNodeType(newn->_nodeType)); */
		//			if (routeTo == 0) 
		//			{
		//				/* printf ("and we are routing FROM this proto expansion\n"); */
		//				fieldInt = findRoutedFieldInFIELDNAMES(newn,"valueChanged",routeTo);
		//			} else {
		//				/* printf ("and, routing TO this proto expansion\n"); */
		//				fieldInt = findRoutedFieldInFIELDNAMES(newn,"setValue",routeTo);
		//			}
		//			if (fieldInt >=0) {
		//				findFieldInOFFSETS(newn->_nodeType, 
		//				fieldInt, offs, type, &accessType);
		//				*innode = newn; /* pass back this new node for routing */
		//			}
		//		}
		//	}
		//}
	}

	//if (error) ConsoleMessage ("getRouteField,  an error"); else ConsoleMessage ("getRouteField,  ok");


	if (*offs <0) {
		ConsoleMessage ("ROUTE: line %d Field %s not found in node type %s",LINE,
			name,stringNodeType(node->_nodeType));
		error = TRUE;
	}

	/* can we route with this direction with this field? This might be already checked, but lets
		make sure once and for all... */
	if (routeTo) {
		if ((accessType != KW_inputOnly) && (accessType != KW_inputOutput)) {
			ConsoleMessage ("ROUTE: line %d: can not route TO a type of %s\n",LINE,stringKeywordType(accessType));
			error = TRUE;
		}
	} else {
		if ((accessType != KW_outputOnly) && (accessType != KW_inputOutput)) {
			ConsoleMessage ("ROUTE: line %d: can not route FROM a type of %s\n",LINE,stringKeywordType(accessType));
			error = TRUE;
		}
	}
	//if (error) ConsoleMessage ("getRouteField, returning an error"); else ConsoleMessage ("getRouteField, returning ok");
	return error;
}

struct X3D_Node *broto_search_DEFname(struct X3D_Proto *context, char *name);
int getFieldFromNodeAndName(struct X3D_Node* node,const char *fieldname, int *type, int *kind, int *iifield, union anyVrml **value);
void broto_store_route(struct X3D_Proto* proto, struct X3D_Node* fromNode, int fromOfs, struct X3D_Node* toNode, int toOfs, int ft);
struct IMEXPORT *broto_search_IMPORTname(struct X3D_Proto *context, char *name);
void broto_store_ImportRoute(struct X3D_Proto* proto, char *fromNode, char *fromField, char *toNode, char* toField);
struct brotoRoute *createNewBrotoRoute();
void broto_store_broute(struct X3D_Proto* context,struct brotoRoute *route);
int QA_routeEnd(struct X3D_Proto *context, char* cnode, char* cfield, struct brouteEnd* brend, int isFrom){
	//checks one end of a route during parsing
	struct X3D_Node* node;
	int found = 0;

	brend->weak = 1;
	brend->cfield = STRDUP(cfield);
	brend->cnode = STRDUP(cnode);

	node = broto_search_DEFname(context,cnode);
	if(!node){
		struct IMEXPORT *imp;
		imp = broto_search_IMPORTname(context, cnode);
		if(imp){
			found = 1;
			brend->weak = 2;
		}
	}else{
		int idir;
		int type,kind,ifield,source;
		void *decl;
		union anyVrml *value;
		if(isFrom) idir = PKW_outputOnly;
		else idir = PKW_inputOnly;
		found = find_anyfield_by_nameAndRouteDir(node,&value,&kind,&type,cfield,&source,&decl,&ifield,idir);  //fieldSynonymCompare for set_ _changed
		if(found){
			brend->node = node;
			brend->weak = 0;
			brend->ftype = type;
			brend->ifield = ifield;
		}
	}
	return found;
}
void QAandRegister_parsedRoute_B(struct X3D_Proto *context, char* fnode, char* ffield, char* tnode, char* tfield){
	// used by both x3d and vrml parsers if usingBrotos(), to quality check each end of a route for validity,
	//  store in context->__ROUTES, and -if instancing scenery- register the route
	//  this version accomodates regular routes and routes starting and/or ending on an IMPORTed node, which 
	//  may not show up until the inline is loaded, and which may disappear when the inline is unloaded.
	int haveFrom, haveTo, ok;
	struct brotoRoute* route;
	int ftf,ftt;
	int allowingVeryWeakRoutes = 1;  //this will store char* node, char* field on an end (or 2) for later import updating via js or late IMPORT statement

	ok = FALSE;
	route = createNewBrotoRoute();
	haveFrom = QA_routeEnd(context, fnode, ffield, &route->from, 1);
	haveTo = QA_routeEnd(context, tnode, tfield, &route->to, 0);
	if((haveFrom && haveTo) || allowingVeryWeakRoutes){
		ftf = -1;
		ftt = -1;
		if( !route->from.weak) ftf = route->from.ftype;
		if( !route->to.weak) ftt = route->to.ftype;
		route->ft = ftf > -1 ? ftf : ftt > -1? ftt : -1;
		route->lastCommand = 0; //not registered
		if(ftf == ftt && ftf > -1){
			//regular route, register while we are hear
			int pflags = context->__protoFlags;
			char oldwayflag = ciflag_get(pflags,1); 
			char instancingflag = ciflag_get(pflags,0);
			if(oldwayflag || instancingflag){
				CRoutes_RegisterSimpleB(route->from.node, route->from.ifield, route->to.node, route->to.ifield, route->ft);
				route->lastCommand = 1; //registered
			}
			//broto_store_route(context,fromNode,fifield,toNode,tifield,ftype); //new way delay until sceneInstance()
			//broto_store_broute(context,route);
			ok = TRUE;
		}else if(route->to.weak || route->from.weak){
			//broto_store_broute(context,route);
			ok = TRUE;
		}
	}
	if(ok || allowingVeryWeakRoutes)
		broto_store_broute(context,route);
	if(!ok || !(haveFrom && haveTo)){
		ConsoleMessage("Routing problem: ");
		/* are the types the same? */
		if (haveFrom && haveTo && route->from.ftype != route->to.ftype) {
			ConsoleMessage ("type mismatch %s != %s, ",stringFieldtypeType(route->from.ftype), stringFieldtypeType(route->to.ftype));
		}
		if(!haveFrom) ConsoleMessage(" _From_ ");
		if(!haveTo) ConsoleMessage(" _To_ ");
		ConsoleMessage ("from  %s %s, ",fnode,ffield);
		ConsoleMessage ("to %s %s\n",tnode,tfield);
	}
}
/******************************************************************************************/
/* parse a ROUTE statement. Should be like:
	<ROUTE fromField="fraction_changed"  fromNode="TIME0" toField="set_fraction" toNode="COL_INTERP"/>
*/
static void parseRoutes_B (void *ud, char **atts) {
	struct X3D_Proto *context;
	//struct X3D_Node *fromNode = NULL;
	//struct X3D_Node *toNode = NULL;	
	int i; //, okf,okt, ftype,fkind,fifield,fsource,ttype,tkind,tifield,tsource;
	//union anyVrml *fvalue, *tvalue;
	//void *fdecl,*tdecl;
	//int error = FALSE;
	//int isImportRoute;
	//int fromType;
	//int toType;
	char *ffield, *tfield, *fnode, *tnode;
	
	context = getContext(ud,TOP);

	ffield = tfield = fnode = tnode = NULL;
	for (i = 0; atts[i]; i += 2) {
		if (strcmp("fromNode",atts[i]) == 0) {
			fnode = atts[i+1];
		} else if (strcmp("toNode",atts[i]) == 0) {
			tnode = atts[i+1];
		} else if (strcmp("fromField",atts[i])==0) {
			ffield = atts[i+1];
		} else if (strcmp("toField",atts[i]) ==0) {
			tfield = atts[i+1];
		}
	}
	QAandRegister_parsedRoute_B(context, fnode, ffield, tnode, tfield);
}
static void parseRoutes (char **atts) {
	struct X3D_Node *fromNode = NULL;
	struct X3D_Node *toNode = NULL;	
	int fromOffset = INT_ID_UNDEFINED;
	int toOffset = INT_ID_UNDEFINED;
	int i;
	int error = FALSE;

	int fromType;
	int toType;
	ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;

	#ifdef X3DPARSERVERBOSE
	printf ("\nstart ofrouting\n");	
	#endif

	/* 2 passes - first, find the nodes */
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		printf("ROUTING pass 1 field:%s=%s\n", atts[i], atts[i + 1]);
		#endif

		if (strcmp("fromNode",atts[i]) == 0) {
			fromNode = DEFNameIndex (atts[i+1], NULL, FALSE);
			if (fromNode == NULL) {
				ConsoleMessage ("ROUTE statement, line %d fromNode (%s) does not exist",LINE,atts[i+1]);
				error = TRUE;
			}
		} else if (strcmp("toNode",atts[i]) == 0) {
			toNode = DEFNameIndex (atts[i+1],NULL, FALSE);
			if (toNode == NULL) {
				ConsoleMessage ("ROUTE statement, line %d toNode (%s) does not exist",LINE,atts[i+1]);
				error = TRUE;
			}
		} else if ((strcmp("fromField",atts[i])!=0) &&
				(strcmp("toField",atts[i]) !=0)) {
			ConsoleMessage ("Field in line %d ROUTE statement not understood: %s",LINE,atts[i]);
			error = TRUE;
		}
	}

	/* get out of here if an error is found */
	if (error) return;

	#ifdef X3DPARSERVERBOSE
	printf ("end of pass1, fromNode %d, toNode %d\n",fromNode,toNode);
	printf ("looking for a route from a %s to a %s\n",stringNodeType(fromNode->_nodeType),
			stringNodeType(toNode->_nodeType));
	#endif

	/* second pass - get the fields of the nodes */
	for (i = 0; atts[i]; i += 2) {
		if (strcmp("fromField",atts[i])==0) {
			error = getRouteField(p->myLexer, &fromNode, &fromOffset, &fromType, (char *)atts[i+1],0);
		} else if (strcmp("toField",atts[i]) ==0) {
			error = getRouteField(p->myLexer, &toNode, &toOffset, &toType, (char *)atts[i+1],1);
		}
	}

	/* get out of here if an error is found */
	if (error) return;

	#ifdef X3DPARSERVERBOSE
	printf ("now routing from a %s to a %s \n",stringFieldtypeType(fromType), stringFieldtypeType(toType));
	printf ("	pointers %d %d to %d %d\n",fromNode, fromOffset, toNode, toOffset);
	#endif


	/* are the types the same? */
	if (fromType != toType) {
		ConsoleMessage ("Routing type mismatch line %d %s != %s",LINE,stringFieldtypeType(fromType), stringFieldtypeType(toType));
		error = TRUE;
	}

	/* get out of here if an error is found */
	if (error) return;


	/* can we register the route? */
	CRoutes_RegisterSimple(fromNode, fromOffset, toNode, toOffset, fromType);
}


/* linkNodeIn - put nodes into parents.


WARNING - PROTOS have a two extra groups put in; one to hold while parsing, and one Group that is always there. 

<Scene>
	<ProtoDeclare name='txt001'>
	<ProtoBody>
		<Material ambientIntensity='0' diffuseColor='1 0.85 0.7' specularColor='1 0.85 0.7' shininess='0.3'/>
	</ProtoBody>
	</ProtoDeclare>
	<Shape>
		<Cone/>
		<Appearance>
			<ProtoInstance  name='txt001'/>
		</Appearance>
	</Shape>
</Scene>

is a good test to see what happens for these nodes. 

Note the "PROTO_MARKER" which is assigned to the bottom-most group. This is the group that is created to
hold the proto expansion, and is passed in to the expandProtoInstance function. So, we KNOW that this
Group node is the base for the PROTO.  If you look at the code to expand the PROTO, the first node is a
Group node, too. So, we will have a Group, children Group, children actual nodes in file. eg:

	Material
	Group
	Group   marker = PROTO_MARKER.

Now, in the example above, the "Material" tries to put itself inside an "appearance" field of an Appearance
node.  So, if the parentIndex-1 and parentIndex-2 entry are Groups, and parentIndex-2 has PROTO_MARKER
set, we know this is a PROTO expansion, so just assign the Material node to the children field, and worry
about it later.

Ok, later, we have the opposite problem, so we look UP the stack to see what the defaultContainer was
actually expected to be. This is the "second case" below.

Note that if we are dealing with Groups as Proto Expansions, none of the above is a problem; just if we
need to put a node into something other than the "children" field.

*/
int getFieldFromNodeAndName(struct X3D_Node* node,const char *fieldname, int *type, int *kind, int *iifield, union anyVrml **value);
int indexChildrenName(struct X3D_Node *node);
struct Multi_Node *childrenField(struct X3D_Node *node);
void linkNodeIn_B(void *ud) {
/*	Assumes you have parsed a node, and have it pushed onto the node stack, and 
	now you want to put it in a field in it's parent
	'children' is a weak field recommendation from either the parent or current node
	more specific recommendations take priority
1. when parsing a node, ProtoInstance, field, or fieldvalue, 
	pushField the char* name or fieldoffset of the most likely default field, or NULL if no recommendation for a default
	Shape would have no default. Transform has children. Inline has __children. GeoLOD has rootnodes.
2. PopField when coming out of node, ProtoInstance, fieldValue, or field end
3. In CDATA, startNode, startProtoInstance 
a) in node or protoinstance, look if there's a _defaultContainer for currentNode, 
	and if not null, and not children, look it up in the parent node 
	(may be null, ie geometry might be trying to find a field in a Proto)
b) get the parent's suggested fieldname off stack, and if not null, 
	lookup in topnode to get type, offsetof and over-ride (example, proto would steer geometry into its __children)
	- and field or fieldValue would be very specific
c) look at atts containerField, and if not null and not children, use it.
	- scene author is trying to over-ride defaults.
*/
	int defaultContainer; //, instanceContainer, i;
	struct X3D_Node *node, *parent;
	char *parentsSuggestion; //*ic,  
	int type, kind, iifield, ok;
	union anyVrml *value;
	const char *fname;

	node = getNode(ud,TOP);
	parent = getNode(ud,TOP-1);
	if(!node || !parent)return;
	parentsSuggestion = getField(ud,TOP-1);
	//for(i=0;i<10;i++)
	//	printf("getField %d = %s",i,getField(ud,i));

	//3.a)
	defaultContainer = node->_defaultContainer;
	if(defaultContainer == FIELDNAMES_children) defaultContainer = 0;
	value = NULL;
	fname = NULL;
	ok = 0;
	if(defaultContainer){
		fname = FIELDNAMES[defaultContainer];
		ok = getFieldFromNodeAndName(parent,fname,&type,&kind,&iifield,&value);
		ok = ok && (kind == PKW_initializeOnly || kind == PKW_inputOutput); //not inputOnly or outputOnly - we can't park nodes there
	}
	if(!value && node->_defaultContainer == FIELDNAMES_children){
		//if you try and put a transform into a proto, or LOD, or Inline (or switch?) you'll come in
		//here to get the equivalent-to-children field
		ok = getFieldFromNodeAndName(parent,"children",&type,&kind,&iifield,&value);
		ok = ok && (kind == PKW_initializeOnly || kind == PKW_inputOutput); //not inputOnly or outputOnly - we can't park nodes there
		if(!ok){
			int kids = indexChildrenName(parent);
			if(kids > 0){
				 value = (union anyVrml*)childrenField(parent);
				 type = FIELDTYPE_MFNode;
			}
		}
	}
	//3.b)
	//if(parentsSuggestion) {
	if(!ok && parentsSuggestion) {
		//if you're parsing a fieldValue, and your value is an SF or MFnode in a child xml element,
		//<fieldValue name='myTransform'>
		//	<Transform USE='tommysTransform'/>
		//</fieldValue>
		//you'll come in here
		//don't want to come in here for metadata
		ok =getFieldFromNodeAndName(parent,parentsSuggestion,&type,&kind,&iifield,&value);
	}
			
	if(!value && parent){
		ok = getFieldFromNodeAndName(parent,"children",&type,&kind,&iifield,&value);
		if(!ok){
			int kids = indexChildrenName(parent);
			if(kids > 0){
				 value = (union anyVrml*)childrenField(parent);
				 type = FIELDTYPE_MFNode;
			}
		}
	}

	if(value){
		if(type == FIELDTYPE_SFNode){
			value->sfnode = node;
			ADD_PARENT(node,parent);
		}else if(type == FIELDTYPE_MFNode){
			union anyVrml *valueadd = NULL;
			ok = 0;
			if(parent->_nodeType == NODE_Proto){
				struct X3D_Proto *pparent = X3D_PROTO(parent);
				char cflag = ciflag_get(pparent->__protoFlags,2);
				if(cflag == 2)  //scene
					ok = getFieldFromNodeAndName(parent,"addChildren",&type,&kind,&iifield,&valueadd);
			}
			if(ok)
				AddRemoveChildren(parent,&valueadd->mfnode,&node,1,1,__FILE__,__LINE__);
			else
				AddRemoveChildren(parent,&value->mfnode,&node,1,1,__FILE__,__LINE__);
		}
	}else{
		printf("no where to put node in parent\n");
	}

}

void linkNodeIn(void *ud, char *where, int lineno) {
	int coffset;
	int ctype;
	int ctmp;
	char *memptr;
	int myContainer;
	int defaultContainer;
	ttglobal tg = gglobal();

	/* did we have a valid node here? Things like ProtoDeclares are NOT valid nodes, and we can ignore them,
		because there will be no code associated with them */
	
	/* bounds check */
	//if (tg->X3DParser.parentIndex < 1) {
	//	ConsoleMessage ("linkNodeIn: stack underflow");
	//	return;
	//}

	if ((getNode(ud,TOP) == NULL) || (getNode(ud,TOP-1) == NULL)) {
		ConsoleMessage ("linkNodeIn: NULL found in stack");
		return;
	}
	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
/*
	printf ("linkNodeIn at %s:%d: parserMode %s parentIndex %d, ",
			where,lineno,
			parserModeStrings[getMode(ud,TOP)],parentIndex);
*/
	printf ("linkNodeIn parserMode %s parentIndex %d, ",
			parserModeStrings[getMode(ud,TOP)],parentIndex);
	printf ("linking in %s (%u) to %s (%u), field %s (%d)\n",
		stringNodeType(parentStack[parentIndex]->_nodeType),
		parentStack[parentIndex],
		stringNodeType(parentStack[parentIndex-1]->_nodeType),
		parentStack[parentIndex-1],
		stringFieldType(parentStack[parentIndex]->_defaultContainer),
		parentStack[parentIndex]->_defaultContainer);

	if (parentStack[parentIndex]->_nodeType == NODE_Group) {
	TTY_SPACE
		printf ("stack %d is a Group; FreeWRL__protoDef is %d\n",
			parentIndex,
			X3D_GROUP(parentStack[parentIndex])->FreeWRL__protoDef);

	}

	if (parentStack[parentIndex-1]->_nodeType == NODE_Group) {
	TTY_SPACE
		printf ("stack %d is a Group; FreeWRL__protoDef is %d\n",
			parentIndex-1,
			X3D_GROUP(parentStack[parentIndex-1])->FreeWRL__protoDef);

	}
	#endif

	/* where to put this node... */
	myContainer = getNode(ud,TOP)->_defaultContainer;
	
	/* kid swap - any parent nodes -like GeoLOD- that have a children field, but intend 
		to put _defaultContainer=FIELDNAMES_children xml child nodes 
		into a different field can do it here
	*/
	defaultContainer = myContainer;
	/* GeoLOD - put into rootNode field */
	if(myContainer == FIELDNAMES_children) //&& getNode(ud,TOP-1)->_nodeType == NODE_GeoLOD)
	{
		switch(getNode(ud,TOP-1)->_nodeType){
		case NODE_GeoLOD:
			defaultContainer = FIELDNAMES_rootNode; break;
		case NODE_Proto:
			defaultContainer = FIELDNAMES___children; break; 
		}
	}

	/* Link it in; the parent containerField should exist, and should be an SF or MFNode  */
	findFieldInOFFSETS(getNode(ud,TOP-1)->_nodeType, 
		defaultContainer, &coffset, &ctype, &ctmp);
		//parentStack[parentIndex]->_defaultContainer, &coffset, &ctype, &ctmp);

	/* PROTOS - we will have a Group node here */
	/* first case, assigning a node to a PROTO Group expansion - eg, in the above example, 
		Material should go to appearance, but FORCE it go to children here. */

	if ((ctype == INT_ID_UNDEFINED) && (getNode(ud,TOP-1)->_nodeType == NODE_Group)) {
		/* printf ("problem finding field %d in a Group %u, so we are pretending this is a PROTO for now\n",
			stringFieldType(parentStack[parentIndex-1],
			stringFieldType(myContainer)); */
		/* printf ("and, FreeWRL__protodEf for the group is %d\n",X3D_GROUP(parentStack[parentIndex-1])->FreeWRL__protoDef); */

		/* lets see if we have a base node with a PROTO Group flag. */
		if (tg->X3DParser.parentIndex>=2) {
			/* printf ("we have enough space...\n"); */
			if(getNode(ud,TOP-2)->_nodeType == NODE_Group) {
				/* printf ("and we have a group->group\n"); */
				if (X3D_GROUP(getNode(ud,TOP-2))->FreeWRL__protoDef == PROTO_MARKER) {
					/* printf ("proto, step1, were going to go to a %s, not to children\n",stringFieldType(myContainer)); */
					findFieldInOFFSETS(NODE_Group, 
						FIELDNAMES_children, &coffset, &ctype, &ctmp);
					/* printf ("changed it to ctype %d\n",ctype); */
				}
			}else if(getNode(ud,TOP-2)->_nodeType == NODE_Script) {
				/* Script SFNode fields are set up with Group nodes as containers - so allow a default container to be group's children */
					findFieldInOFFSETS(NODE_Group, 
						FIELDNAMES_children, &coffset, &ctype, &ctmp);
			}
		}
	}

	/* PROTOS, second case: we have the PROTO group, and it is going to an invalid container field.... */
	if ((ctype == INT_ID_UNDEFINED) && (getNode(ud,TOP)->_nodeType == NODE_Group)) {
		/* is this linking in the PROTO? */
		if (X3D_GROUP(getNode(ud,TOP))->FreeWRL__protoDef == PROTO_MARKER) {
			/* printf ("WE HAVE PROTODEF %d\n",X3D_GROUP(parentStack[parentIndex])->FreeWRL__protoDef);
			printf ("GROUP has %d children\n",X3D_GROUP(parentStack[parentIndex])->children.n); */
			if (X3D_GROUP(getNode(ud,TOP))->children.n>0) {
				struct X3D_Group *firstCh = X3D_GROUP(X3D_GROUP(getNode(ud,TOP))->children.p[0]);

				/* printf ("first child is of type %s\n", stringNodeType(firstCh->_nodeType)); */

				if (firstCh->_nodeType == NODE_Group) {
					/* printf ("we have the Group->Group symbology\n"); */
					firstCh = X3D_GROUP(firstCh->children.p[0]);

					/*
					printf ("now, firstCh is of type %s\n",stringNodeType(firstCh->_nodeType));
					printf ("defaultContainers are %s %s %s\n",
					stringFieldType(parentStack[parentIndex]->_defaultContainer),
					stringFieldType(parentStack[parentIndex-1]->_defaultContainer),
					stringFieldType(parentStack[parentIndex-2]->_defaultContainer));
					printf ("upstack defaultContainers are %s %s %s\n",
					stringFieldType(parentStack[parentIndex]->_defaultContainer),
					stringFieldType(parentStack[parentIndex+1]->_defaultContainer),
					stringFieldType(parentStack[parentIndex+2]->_defaultContainer));
					*/
					
					myContainer = getNode(ud,TOP+2)->_defaultContainer;
					
					/*
					printf ("and, we are going to look for container %s\n",stringFieldType(myContainer));
					printf ("in a node type of %s\n",stringNodeType(X3D_NODE(parentStack[parentIndex-1])->_nodeType));
					*/

					findFieldInOFFSETS(X3D_NODE(getNode(ud,TOP-1))->_nodeType, myContainer,
						&coffset, &ctype, &ctmp);
				}
			}
		}
	}

	/* strict parsing on - lets see if this is a Metadatafield not following guidelines */

	if ((coffset <= 0) && (!tg->internalc.global_strictParsing)) {
		if ((getNode(ud,TOP)->_nodeType == NODE_MetadataFloat) ||
			(getNode(ud,TOP)->_nodeType == NODE_MetadataString) ||
			(getNode(ud,TOP)->_nodeType == NODE_MetadataDouble) ||
			(getNode(ud,TOP)->_nodeType == NODE_MetadataInteger)) {
			findFieldInOFFSETS(getNode(ud,TOP-1)->_nodeType, 
				FIELDNAMES_metadata, &coffset, &ctype, &ctmp);

			/*
			printf ("X3DParser - COFFSET problem, metada node: %s parent %s coffset now %d...\n", 
			stringNodeType(parentStack[parentIndex]->_nodeType),
			stringNodeType(parentStack[parentIndex-1]->_nodeType),
			coffset);
			*/

		}
		if (coffset <= 0) {
			/* this is stated better below 
			ConsoleMessage ("X3DParser - trouble finding field %s in node %s\n",
			stringFieldType(parentStack[parentIndex]->_defaultContainer),
			stringNodeType(parentStack[parentIndex-1]->_nodeType));
		    */
		} else {
			printf ("X3DParser - warning line %d, incorrect Metadata; \"%s\" defaultContainer changed to \"metadata\"\n",
				LINE,
				stringNodeType(getNode(ud,TOP)->_nodeType));
		}
	}


	/* this will be a MFNode or an SFNode if the marking is ok. */
	if ((ctype != FIELDTYPE_MFNode) && (ctype != FIELDTYPE_SFNode)) {
		ConsoleMessage ("X3DParser: warning, line %d: trouble linking to containerField :%s: of parent node type :%s: (specified in a :%s: node)", LINE,
			stringFieldType(myContainer),
			stringNodeType(getNode(ud,TOP-1)->_nodeType),
			stringNodeType(getNode(ud,TOP)->_nodeType));
		return;
	}
	memptr = offsetPointer_deref (char *, getNode(ud,TOP-1),coffset);
	if (ctype == FIELDTYPE_SFNode) {
		void *temp = getNode(ud,TOP);
		/* copy over a single memory pointer */
		//memcpy (memptr, &getNode(ud,TOP),sizeof(struct X3D_Node *));
		memcpy (memptr, &temp,sizeof(struct X3D_Node *));
		ADD_PARENT(getNode(ud,TOP), getNode(ud,TOP-1));
	} else {
		struct X3D_Node *temp = getNode(ud,TOP);
		AddRemoveChildren (
			getNode(ud,TOP-1), /* parent */
			(struct Multi_Node *) memptr,			/* where the children field is */
			&temp, //&(getNode(ud,TOP)),	/* this child, 1 node */
				1, 1,__FILE__,__LINE__);
	}
}
void Parser_scanStringValueToMem_B(union anyVrml* any, indexT ctype, char *value, int isXML);
void endCDATA_B (void *ud, const xmlChar *string, int len) {
	char *fieldname = getField(ud,TOP);
	struct X3D_Node *node = getNode(ud,TOP);
	int type, kind, iifield, ok, handled;
	union anyVrml *value;
	ok = getFieldFromNodeAndName(node,fieldname,&type,&kind,&iifield,&value);
	if(ok){
		handled = FALSE;
		if(!strcmp(fieldname,"url")){
			//Script javascript doesn't parse well as an MFString
			if(strstr((char*)string,"script")){
				handled = TRUE;
				value->mfstring.n = 1;
				value->mfstring.p = MALLOCV(sizeof(void *));
				value->mfstring.p[0] = newASCIIString((char *)string);
				//if(0) printf("copied cdata string= [%s]\n",(struct Uni_String*)(value->mfstring.p[0])->strptr);
			}
		}
		if(!handled)
			Parser_scanStringValueToMem_B(value, type,(char *)string, TRUE);
	}
}

void endCDATA (void *ud, const xmlChar *string, int len) {
	ttglobal tg = gglobal();
	ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;
	if(usingBrotos()){
		endCDATA_B(ud,string,len);
		return;
	}
	/* JAS printf ("cdata_element, :%s:\n",string); */
	if (getMode(ud,TOP) == PARSING_PROTOBODY) {
		dumpCDATAtoProtoBody ((char *)string);
	} else if (p->in3_3_fieldValue) {
		appendDataToFieldValue((char *)string,len);
	} else {
		/* most likely we have a script here */
		#ifdef X3DPARSERVERBOSE
		printf ("X3DParser, have the following CDATA :%s:\n",string);
		#endif

		/* copy the CDATA text over to the CDATA_Text string, so that the script can get to it */
		FREE_IF_NZ(tg->X3DParser.CDATA_Text);
		tg->X3DParser.CDATA_Text = MALLOC(char *, len+1);
		p->CDATA_TextMallocSize = len+1;
		tg->X3DParser.CDATA_Text_curlen = len;

		memcpy(tg->X3DParser.CDATA_Text,string,p->CDATA_TextMallocSize*sizeof(char));
	}
}



/* parse a export statement, and send the results along */
static void parseImport(char **atts) {
	int i;

	for (i = 0; atts[i]; i += 2) {
		printf("import field:%s=%s\n", atts[i], atts[i + 1]);
	}
/* do nothing right now */
return;
}
void handleImport_B (struct X3D_Node *nodeptr, char *nodeName,char *nodeImport, char *as);
static void parseImport_B(void *ud, char **atts) {
	int i;
	char *inlinedef, *exporteddef, *as;
	struct X3D_Proto *context;
	context = getContext(ud,TOP);

	inlinedef = exporteddef = as = NULL;
	for (i = 0; atts[i]; i += 2) {
		printf("import field:%s=%s\n", atts[i], atts[i + 1]);
		if(!strcmp(atts[i],"inlineDEF")) inlinedef = atts[i+1];
		if(!strcmp(atts[i],"exportedDEF")) exporteddef = atts[i+1];
		if(!strcmp(atts[i],"AS")) as = atts[i+1];

	}
	handleImport_B (X3D_NODE(context), inlinedef, exporteddef, as);
}


void handleExport_B (void *nodeptr, char *node, char *as);
static void parseExport_B(void *ud, char **atts) {
	// http://www.web3d.org/documents/specifications/19776-1/V3.3/Part01/concepts.html#IMPORT_EXPORTStatementSyntax
	int i;
	char *localdef, *as;
	struct X3D_Proto *context;
	context = getContext(ud,TOP);

	localdef = as = NULL;
	for (i = 0; atts[i]; i += 2) {
		printf("export field:%s=%s\n", atts[i], atts[i + 1]);
		if(!strcmp(atts[i],"localDEF")) localdef = atts[i+1];
		if(!strcmp(atts[i],"AS")) as = atts[i+1];
	}
	handleExport_B(context,localdef, as);
}
/* parse a export statement, and send the results along */
static void parseExport(char **atts) {
	int i;
	char *nodeToExport = NULL;
	char *alias = NULL;

	for (i = 0; atts[i]; i += 2) {
		printf("export field:%s=%s\n", atts[i], atts[i + 1]);
	}
/* do nothing right now */
return;

	handleExport(nodeToExport, alias);
}

/* parse a component statement, and send the results along */
static void parseComponent(char **atts) {
	int i;
	int myComponent = INT_ID_UNDEFINED;
	int myLevel = INT_ID_UNDEFINED;

	/* go through the fields and make sense of them */
        for (i = 0; atts[i]; i += 2) {
		/* printf("components field:%s=%s\n", atts[i], atts[i + 1]);  */
		if (strcmp("level",atts[i]) == 0) {
			if (sscanf(atts[i+1],"%d",&myLevel) != 1) {
				ConsoleMessage ("Line %d: Expected Component level for component %s, got %s",LINE, atts[i], atts[i+1]);
				return;
			}
		} else if (strcmp("name",atts[i]) == 0) {
			myComponent = findFieldInCOMPONENTS(atts[i+1]);
			if (myComponent == INT_ID_UNDEFINED) {
				ConsoleMessage("Line %d: Component statement, but component name not valid :%s:",LINE,atts[i+1]);
				return;
			}

		} else {
			ConsoleMessage ("Line %d: Unknown fields in Component statement :%s: :%s:",LINE,atts[i], atts[i+1]);
		}
	}

	if (myComponent == INT_ID_UNDEFINED) {
		ConsoleMessage("Line %d: Component statement, but component name not stated",LINE);
	} else if (myLevel == INT_ID_UNDEFINED) {
		ConsoleMessage("Line %d: Component statement, but component level not stated",LINE);
	} else {
		handleComponent(myComponent,myLevel);
	}
}

/* parse the <X3D profile='Immersive' version='3.0' xm... line */
static void parseX3Dhead(char **atts) {
	int i;
	int myProfile = -10000; /* something negative, not INT_ID_UNDEFINED... */
	int versionIndex = INT_ID_UNDEFINED;

	for (i = 0; atts[i]; i += 2) {
		/* printf("parseX3Dhead: field:%s=%s\n", atts[i], atts[i + 1]); */
		if (strcmp("profile",atts[i]) == 0) {
			myProfile = findFieldInPROFILES(atts[i+1]);
		} else if (strcmp("version",atts[i]) == 0) {
			versionIndex = i+1;
		} else {
			/* printf ("just skipping this data\n"); */
		}
	}

	/* now, handle all the found variables */
	if (myProfile == INT_ID_UNDEFINED) {
		ConsoleMessage ("expected valid profile in X3D header");
	} else {
		/* printf ("X3DParsehead, myProfile %d\n",myProfile); */
		if (myProfile >= 0) handleProfile (myProfile);
	}

	if (versionIndex != INT_ID_UNDEFINED) {
		handleVersion (atts[versionIndex]);
	}
}

static void parseHeader(char **atts) {
	int i;
	for (i = 0; atts[i]; i += 2) {
		/* printf("parseHeader: field:%s=%s\n", atts[i], atts[i + 1]); */
	}
}
static void parseScene(char **atts) {
	int i;
	for (i = 0; atts[i]; i += 2) {
		/* printf("parseScene: field:%s=%s\n", atts[i], atts[i + 1]); */
	}
}
static void parseMeta(char **atts) {
	int i;
	for (i = 0; atts[i]; i += 2) {
		/* printf("parseMeta field:%s=%s\n", atts[i], atts[i + 1]); */
	}
}
void deleteMallocedFieldValue(int type,union anyVrml *fieldPtr);
static void parseFieldValue_B(void *ud, char **atts) {
	int i, type, kind, iifield, ok;
	char *fname, *svalue, *cname;
	union anyVrml *value;
	struct X3D_Node *node = getNode(ud,TOP);

	if(0) printf("parseFieldValue\n");
	fname = svalue = NULL;
	for(i=0;atts[i];i+=2){
		if(!strcmp(atts[i],"name")) fname = atts[i+1];
		if(!strcmp(atts[i],"value")) svalue = atts[i+1];
	}
	ok = 0;
	cname = NULL;
	value = NULL;
	if(fname){
		ok = getFieldFromNodeAndName(node,fname,&type,&kind,&iifield,&value);
		if(ok){
			//get a pointer to a heap version of the field name (because atts vanishes on return)
			ok = getFieldFromNodeAndIndex(node, iifield, &cname, &type, &kind, &value);
		}
	}
	if(cname && value && svalue){
		deleteMallocedFieldValue(type,value);
		Parser_scanStringValueToMem_B(value,type,svalue,TRUE);
	}
	if(cname && (node->_nodeType == NODE_Proto)){
		//for protoInstances, whether or not you have a value, 
		//if you declare a field then you are saying you declare the value null or 0 or default at least.
		//so for SFNode fields where <fieldValue><a node></fieldValue> and we get the node later
		//whether or not there's a node/value parsed, we are declaring its set even at null.
		//therefore alreadyset
		//the way to acheive not alreadySet is to not mention the field in your protoInstance.
		struct X3D_Proto *pnode;
		struct ProtoFieldDecl* pfield;
		struct ProtoDefinition* pstruct;
		pnode = X3D_PROTO(node);
		pstruct = (struct ProtoDefinition*) pnode->__protoDef;
		pfield = vector_get(struct ProtoFieldDecl*,pstruct->iface,iifield);
		pfield->alreadySet = TRUE;
	}

	pushField(ud,cname); //in case there's no value, because its SF or MFNodes in child xml, or in CDATA
}
static void endFieldValue_B(void *ud){
	if(0) printf("endFieldValue\n");
	//in x3d, <fieldvalue type=SFNode><a node></fieldValue>

	popField(ud);
}
/* we have a fieldValue, should be in a PROTO expansion */
static void parseFieldValue(void *ud, const char *name, char **atts) {
	int i;
	int nameIndex = INT_ID_UNDEFINED;
	ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;


	#ifdef X3DPARSERVERBOSE
	printf ("parseFieldValue, mode %s\n",parserModeStrings[getMode(ud,TOP)]);  
	#endif

	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		printf("parseFieldValue field:%s=%s\n", atts[i], atts[i + 1]);
		#endif

		if (strcmp(atts[i],"name") == 0) nameIndex= i+1;
	}

	if ((getMode(ud,TOP) == PARSING_EXTERNPROTODECLARE) || (getMode(ud,TOP) == PARSING_PROTOINSTANCE)) {
		parseProtoInstanceFields(ud,name,atts);
	} else {
		if (p->in3_3_fieldValue) printf ("parseFieldValue - did not expect in3_3_fieldValue to be set\n");
		p->in3_3_fieldValue = TRUE;

		if (nameIndex == INT_ID_UNDEFINED) {
			printf ("did not find name field for this 3.3 fieldType test\n");
			p->in3_3_fieldIndex = INT_ID_UNDEFINED;
		} else {
		/* printf ("parseFieldValue field %s, parent is a %s\n",atts[nameIndex],stringNodeType(parentStack[parentIndex]->_nodeType)); */

			p->in3_3_fieldIndex = findFieldInFIELDNAMES(atts[nameIndex]);
		}
	}
}


static void parseIS(void *ud) {
	#ifdef X3DPARSERVERBOSE
	printf ("parseIS mode is %s\n",parserModeStrings[getMode(ud,TOP)]); 
	#endif
	pushMode(ud,PARSING_IS);

}



static void endIS(void *ud) {
	#ifdef X3DPARSERVERBOSE
	printf ("endIS mode is %s\n",parserModeStrings[getMode(ud,TOP)]); 
	#endif
	popMode(ud);
}



static void endProtoInterfaceTag(void *ud) {
	if (getMode(ud,TOP) != PARSING_PROTOINTERFACE) {
		ConsoleMessage ("endProtoInterfaceTag: got a </ProtoInterface> but not parsing one at line %d",LINE);
	}
	/* now, a ProtoInterface should be within a ProtoDeclare, so, make the expected mode PARSING_PROTODECLARE */
	//setParserMode(PARSING_PROTODECLARE);
	popMode(ud);
}
static void endProtoBodyTag_B(void *ud, const char *name) {
	//pop context
	if (getMode(ud,TOP) != PARSING_PROTOBODY) {
		ConsoleMessage ("endProtoBodyTag: got a </ProtoBody> but not parsing one at line %d",LINE);
	}
	popMode(ud);
	popContext(ud);
}
static void endProtoBodyTag(void *ud, const char *name) {
	/* ending <ProtoBody> */
	
	#ifdef X3DPARSERVERBOSE
	printf ("endProtoBody, mode is %s\n",parserModeStrings[getMode(ud,TOP)]);
	#endif

	if (getMode(ud,TOP) != PARSING_PROTOBODY) {
		ConsoleMessage ("endProtoBodyTag: got a </ProtoBody> but not parsing one at line %d",LINE);
	}

	endDumpProtoBody(name);

	/* now, a ProtoBody should be within a ProtoDeclare, so, make the expected mode PARSING_PROTODECLARE */
	//setParserMode(PARSING_PROTODECLARE);
	popMode(ud);
}

static void endExternProtoDeclareTag_B(void *ud) {
	popMode(ud);
	popNode(ud);
	popField(ud);
}
static void endExternProtoDeclareTag(void *ud) {
	/* ending <ExternProtoDeclare> */

	if (getMode(ud,TOP) != PARSING_EXTERNPROTODECLARE) {
		ConsoleMessage ("endExternProtoDeclareTag: got a </ExternProtoDeclare> but not parsing one at line %d",LINE);
		//setParserMode(PARSING_EXTERNPROTODECLARE);
		pushMode(ud,PARSING_EXTERNPROTODECLARE);
	}
	
	/* we do the DECREMENT_PARENTINDEX here because successful parsing of the included ProtoDeclare leaves it too high */
	endExternProtoDeclare(ud);
	popMode(ud); //+
}

static void endProtoDeclareTag_B(void *ud) {
	/* ending <ProtoDeclare> */
	struct X3D_Proto * proto;
	struct Multi_Node *cptr;

	if (getMode(ud,TOP) != PARSING_PROTODECLARE) {
		ConsoleMessage ("endProtoDeclareTag: got a </ProtoDeclare> but not parsing one at line %d",LINE);
		pushMode(ud,PARSING_PROTODECLARE);
	}
	if(0) printf("end protoDeclare\n");
	// set defaultContainer based on 1st child
	proto = X3D_PROTO(getNode(ud,TOP));
	cptr = NULL;
	if(proto->__children.n)
		cptr = &proto->__children;
	else if(proto->addChildren.n)
		cptr = &proto->addChildren;
	if(cptr){
		struct X3D_Node *c1 = cptr->p[0];
		if(c1->_defaultContainer > INT_ID_UNDEFINED) 
			proto->_defaultContainer = c1->_defaultContainer;
	}
	popField(ud);
	popNode(ud); //I think I should pop the X3DProto off the stack
	popMode(ud);
}

static void endProtoDeclareTag(void *ud) {
	/* ending <ProtoDeclare> */

	if (getMode(ud,TOP) != PARSING_PROTODECLARE) {
		ConsoleMessage ("endProtoDeclareTag: got a </ProtoDeclare> but not parsing one at line %d",LINE);
		pushMode(ud,PARSING_PROTODECLARE);
	}

	endProtoDeclare(ud);
	popMode(ud);
}
void deep_copy_broto_body2(struct X3D_Proto** proto, struct X3D_Proto** dest);
static void endProtoInstance_B(void *ud, const char *name) {
	//now that initial field values are set, deep copy the broto body
	struct X3D_Node *node;
	if(0) printf("endProtoInstance_B\n");

	node = getNode(ud,TOP);
	if(node){
		if(node->_nodeType == NODE_Proto || node->_nodeType == NODE_Inline ){
			char pflagdepth;
			struct X3D_Proto *pnode = X3D_PROTO(node);
			pflagdepth = ciflag_get(pnode->__protoFlags,0); //0 - we're in a protodeclare, 1 - we are instancing live scenery
			if( pflagdepth){
				//copying the body _after_ the protoInstance field values have been parsed 
				//allows ISd fields in body nodes to get the pkw_initializeOnly/inputOutput value
				//from the protoInstance interface
				struct X3D_Proto *pdeclare;
				pdeclare = X3D_PROTO(pnode->__prototype);
				deep_copy_broto_body2(&pdeclare,&pnode);
			}
		}
		linkNodeIn_B(ud);
	}
	popField(ud);
	popNode(ud);
	popMode(ud);
}
static void endProtoInstanceField(void *ud, const char *name) {
	struct X3D_Group *protoExpGroup = NULL;
	ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;

	/* ending </field> */
	#ifdef X3DPARSERVERBOSE
	printf ("endProtoInstanceField, got %s got to find it, and expand it.\n",name);
	printf ("endProtoInstanceField, parentIndex %d\n",parentIndex);
	#endif

	if (strcmp(name,"ProtoInstance")==0) {
		/* we should just be assuming that we are parsing regular nodes for the scene graph now */
		//setParserMode(PARSING_NODES);
		pushMode(ud,PARSING_NODES);
	
		protoExpGroup = (struct X3D_Group *) createNewX3DNode(NODE_Group);
		protoExpGroup->FreeWRL__protoDef = PROTO_MARKER;

			#ifdef X3DPARSERVERBOSE
			if (protoExpGroup != NULL) {
				printf ("\nOK, linking in this proto. I'm %d, ps-1 is %d, and p %d\n",protoExpGroup,parentStack[parentIndex-1], parentStack[parentIndex]);
				printf ("types %s %s and %s respectively. \n",
					stringNodeType(X3D_NODE(protoExpGroup)->_nodeType),
					stringNodeType(X3D_NODE(parentStack[parentIndex-1])->_nodeType),
					stringNodeType(X3D_NODE(parentStack[parentIndex])->_nodeType));
			}
			{int i;
			for (i=parentIndex; i>=0; i--) {
				printf ("parentStack %d node %u \n",i,parentStack[i]);
				if (parentStack[i] != NULL) {
					printf ("  type %s\n",stringNodeType(parentStack[i]->_nodeType));
				}
			}
			}
			#endif
	
		expandProtoInstance(ud, p->myLexer, protoExpGroup);
		popMode(ud);
	
#ifdef X3DPARSERVERBOSE
		printf ("after expandProtoInstance, my group (%u)has %d children, %d Meta nodes, and is protodef %d\n",
		protoExpGroup,
		protoExpGroup->children.n, protoExpGroup->FreeWRL_PROTOInterfaceNodes.n, protoExpGroup->FreeWRL__protoDef);
		{int i;
			for (i=0; i<protoExpGroup->children.n; i++) {
				printf ("child %d is %u, type %s\n",i,protoExpGroup->children.p[i], 
					stringNodeType(X3D_GROUP(protoExpGroup->children.p[i])->_nodeType));
				if (X3D_GROUP(protoExpGroup->children.p[i])->_nodeType == NODE_Group) {
					struct X3D_Group * pxx = X3D_GROUP(protoExpGroup->children.p[i]);
					printf (" and it has %d children, %d Meta nodes, and is protodef %d\n",
					pxx->children.n, pxx->FreeWRL_PROTOInterfaceNodes.n, pxx->FreeWRL__protoDef);
				}
		
			}
		}
#endif
		popMode(ud); //+ but was something pushed?
	} else if (strcmp(name,"fieldValue")==0) {
		/* printf ("endProtoInstanceField, got %s, ignoring it\n",name);*/
		///BIGPOP
		endProtoInstanceFieldTypeNode(ud,name);
	} else {
		/* this could be something like the ending of shape in the following:
		   <fieldValue...> <Shape> <Box/> </Shape> </fieldValue> */
		printf ("endProtoInstanceField, got %s, ignoring it.\n",name);
	}
}






/* did we get a USE in a proto instance, like:
<ProtoInstance name='CamLoader' DEF='Camera1_Bgpic'>
	<fieldValue name='imageName' value='Default.jpg'/>
	<fieldValue name='relay'>
		<Script USE='CameraRelay'/>
	</fieldValue>
	<fieldValue name='yscale' value='3.0'/>
</ProtoInstance>

if so, we will be here for the USE fields.


*/


static void saveProtoInstanceFields (void *ud, const char *name, char **atts) {
	#ifdef X3DPARSERVERBOSE
		printf ("saveProtoInstanceFields, have node :%s:\n",name);
	#endif

	if (strcmp(name,"fieldValue") == 0) {
		parseFieldValue(ud,name,atts);
	} else {
		/* printf ("warning - saveProtoInstanceFields - dont know what to do with %s\n",name); */
		parseFieldValue(ud,name,atts);
	}
	#ifdef X3DPARSERVERBOSE
		printf ("saveProtoInstanceFields END\n");
	#endif
}


/********************************************************/


void **shaderFields(struct X3D_Node* node){
	void **shaderfield;
	switch(node->_nodeType){
	case NODE_Script:
		shaderfield = &X3D_SCRIPT(node)->__scriptObj; break;
	case NODE_ComposedShader:
		shaderfield = (void**)&X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields; break;
	case NODE_ShaderProgram:
		shaderfield = (void**)&X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields; break;
	case NODE_PackagedShader:
		shaderfield = (void**)&X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields; break;
	default:
		shaderfield = NULL;
	}
	return shaderfield;
}
void broto_store_DEF(struct X3D_Proto* proto,struct X3D_Node* node, char *name);
static void parseAttributes_B(void *ud, char **atts);
void add_node_to_broto_context(struct X3D_Proto *context,struct X3D_Node *node);
void push_binding_stack_set();
void push_next_layerId_from_binding_stack_set();
void pop_binding_stack_set();

static void startBuiltin_B(void *ud, int myNodeType, const xmlChar *name, char** atts) {
	struct X3D_Node *node, *fromDEFtable;
	struct X3D_Proto *context;
	void **shaderfield;
	char pflagdepth;
	int kids, i, isUSE;
	char *defname, *suggestedChildField, *containerfield;

	suggestedChildField = containerfield = NULL;
	context = getContext(ud,TOP);
	pflagdepth = ciflag_get(context->__protoFlags,0); //0 - we're in a protodeclare, 1 - we are instancing live scenery
	if(0) printf("start builtin %s\n",name);
	node = NULL;
	defname = NULL;
	isUSE = FALSE;
	/* go through the attributes; do some here, do others later (in case of PROTO IS fields found) */
	for (i = 0; atts[i]; i += 2) {
		/* is this a DEF name? if so, record the name and then ignore the field */
		if (strcmp ("DEF",atts[i]) == 0) {
			/* printf ("saveAttributes, this is a DEF, name %s\n",atts[i+1]); */
			defname = atts[i+1];
			//fromDEFtable = DEFNameIndex ((char *)atts[i+1],node, TRUE);
			fromDEFtable = broto_search_DEFname(context,defname);
			if (fromDEFtable) {
				#ifdef X3DPARSERVERBOSE
				printf ("Warning - line %d duplicate DEF name: \'%s\'\n",LINE,atts[i+1]);
				#endif
			}
		} else if (strcmp ("USE",atts[i]) == 0) {
			#ifdef X3DPARSERVERBOSE
			printf ("this is a USE, name %s\n",atts[i+1]);
			#endif

			//fromDEFtable = DEFNameIndex ((char *)atts[i+1],node, FALSE);
			fromDEFtable = broto_search_DEFname(context,atts[i+1]);
			if (!fromDEFtable) {
				ConsoleMessage ("Warning - line %d DEF name: \'%s\' not found",LINE,atts[i+1]);
			} else {
				#ifdef X3DPARSERVERBOSE
				printf ("copying for field %s defName %s\n",atts[i], atts[i+1]);
				#endif

				/* if (fromDEFtable->_nodeType != fromDEFtable->_nodeType) { */
				if (myNodeType != fromDEFtable->_nodeType) {
					ConsoleMessage ("Warning, line %d DEF/USE mismatch, '%s', %s != %s", LINE,
						atts[i+1],stringNodeType(fromDEFtable->_nodeType), stringNodeType (myNodeType));
				} else {
					/* Q. should thisNode.referenceCount be decremented or ??? */
					node = fromDEFtable;
					node->referenceCount++; //dug9 added but should???
					//getNode(ud,TOP) = thisNode; 
					#ifdef X3DPARSERVERBOSE
					printf ("successful copying for field %s defName %s\n",atts[i], atts[i+1]);
					#endif
					isUSE = TRUE;
				}
			}
		} else if(!strcmp(atts[i],"containerField")) containerfield = atts[i+1];
	}

	if(!isUSE){
		if(pflagdepth)
			node = createNewX3DNode(myNodeType);
		else
			node = createNewX3DNode0(myNodeType);
		if(defname)
			broto_store_DEF(context,node,defname);
	}
	pushNode(ud,node);
	
	if(containerfield) {
		//int builtinField = findFieldInARR(containerfield,FIELDNAMES,FIELDNAMES_COUNT); 
		int builtinField = findFieldInFIELDNAMES(containerfield);
		if(builtinField > INT_ID_UNDEFINED){
			node->_defaultContainer = builtinField;
		}
	}

	//linkNodeIn_B(ud);

	if(!isUSE){
		shaderfield = shaderFields(node);
		if(shaderfield)
			(*shaderfield) = (void *)new_Shader_ScriptB(node);
		//if(node->_nodeType == NODE_Script && pflagdepth)
			//initialize script - wait till end element
		if(node->_nodeType == NODE_LayerSet)
			push_binding_stack_set();
		if(node->_nodeType == NODE_Layer || node->_nodeType == NODE_LayoutLayer)
			push_next_layerId_from_binding_stack_set();
		if(node->_nodeType == NODE_Inline)
			X3D_INLINE(node)->__parentProto = X3D_NODE(context); //when searching for user proto declarations, apparently inlines can search the scene 
		node->_executionContext = X3D_NODE(context);
		add_node_to_broto_context(context,node);
		
		kids = indexChildrenName(node);
		if(kids > -1)
			suggestedChildField = FIELDNAMES[kids];
		if(node->_nodeType == NODE_Script)
			suggestedChildField = FIELDNAMES[FIELDNAMES_url]; //for CDATA 
		pushField(ud,suggestedChildField);

		parseAttributes_B(ud,atts);
	}else{
		pushField(ud,NULL); //we pop in endBuiltin, so we have to push something
	}

}
void initialize_one_script(struct Shader_Script* ss, const struct Multi_String *url);
void endBuiltin_B(void *ud, const xmlChar *name){
	struct X3D_Node *node;
	struct X3D_Proto *context;
	char pflagdepth;
	node = getNode(ud,TOP);
	context = getContext(ud,TOP);
	if(0)printf("end builtin %s\n",name);
	pflagdepth = ciflag_get(context->__protoFlags,0); //0 - we're in a protodeclare, 1 - we are instancing live scenery
	if(node->_nodeType == NODE_Script && pflagdepth){
		struct X3D_Script *sn = X3D_SCRIPT(node);
		//overkill -duplicates new_Shader_Script 
		initialize_one_script(sn->__scriptObj,&sn->url);
		//script_initCodeFromMFUri(sn->__scriptObj, &sn->url);
	}
	if(node->_nodeType == NODE_LayerSet)
		pop_binding_stack_set();

	linkNodeIn_B(ud);

	popNode(ud);
	popField(ud);

}
static void saveAttributes(void *ud, int myNodeType, const xmlChar *name, char** atts) {
	struct nameValuePairs* nvp;
	int i;
	struct X3D_Node *thisNode;
	struct X3D_Node *fromDEFtable;
	struct Vector *childAttributes;
	//ttglobal tg = gglobal();
	//ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;

	DEBUG_X3DPARSER ("	saveAttributes, parentIndex %d parentIndex %d\n",tg->X3DParser.parentIndex,tg->X3DParser.parentIndex);
	
	/* create the scenegraph node for this one */
	thisNode = createNewX3DNode(myNodeType);
	childAttributes = newVector(struct nameValuePairs*,8);
	//getNode(ud,TOP) = thisNode;

	//printf ("saveAttributes, node type %s\n",stringNodeType(myNodeType));

	if (myNodeType == NODE_Script) {
		#ifdef HAVE_JAVASCRIPT
		struct Shader_Script *myObj;

		/* create the Shader_Script for this one */
		X3D_SCRIPT(thisNode)->__scriptObj=new_Shader_Script(thisNode);


		#ifdef X3DPARSERVERBOSE
		printf ("working through script parentIndex %d\n",parentIndex);
		#endif

		myObj = X3D_SCRIPT(thisNode)->__scriptObj;
		JSInit(myObj); /* ->num); */
		#else

			ConsoleMessage ("Javascript not supported\n");
		#endif
	} else if (myNodeType == NODE_ComposedShader) {
		X3D_COMPOSEDSHADER(thisNode)->_shaderUserDefinedFields=X3D_NODE(new_Shader_Script(thisNode));
	} else if (myNodeType == NODE_ShaderProgram) {
		X3D_SHADERPROGRAM(thisNode)->_shaderUserDefinedFields=X3D_NODE(new_Shader_Script(thisNode));
	} else if (myNodeType == NODE_PackagedShader) {
		X3D_PACKAGEDSHADER(thisNode)->_shaderUserDefinedFields=X3D_NODE(new_Shader_Script(thisNode));
	}


 
	/* go through the attributes; do some here, do others later (in case of PROTO IS fields found) */
	for (i = 0; atts[i]; i += 2) {
		/* is this a DEF name? if so, record the name and then ignore the field */
		if (strcmp ("DEF",atts[i]) == 0) {
			/* printf ("saveAttributes, this is a DEF, name %s\n",atts[i+1]); */

			fromDEFtable = DEFNameIndex ((char *)atts[i+1],thisNode, TRUE);
			if (fromDEFtable != thisNode) {
				#ifdef X3DPARSERVERBOSE
				printf ("Warning - line %d duplicate DEF name: \'%s\'\n",LINE,atts[i+1]);
				#endif
			}
		} else if (strcmp ("USE",atts[i]) == 0) {
			#ifdef X3DPARSERVERBOSE
			printf ("this is a USE, name %s\n",atts[i+1]);
			#endif

			fromDEFtable = DEFNameIndex ((char *)atts[i+1],thisNode, FALSE);
			if (fromDEFtable == thisNode) {
				ConsoleMessage ("Warning - line %d DEF name: \'%s\' not found",LINE,atts[i+1]);
			} else {
				#ifdef X3DPARSERVERBOSE
				printf ("copying for field %s defName %s\n",atts[i], atts[i+1]);
				#endif

				/* if (fromDEFtable->_nodeType != fromDEFtable->_nodeType) { */
				if (thisNode->_nodeType != fromDEFtable->_nodeType) {
					ConsoleMessage ("Warning, line %d DEF/USE mismatch, '%s', %s != %s", LINE,
						atts[i+1],stringNodeType(fromDEFtable->_nodeType), stringNodeType (thisNode->_nodeType));
				} else {
					/* Q. should thisNode.referenceCount be decremented or ??? */
					thisNode->referenceCount--; //dug9 added but should???
					thisNode = fromDEFtable;
					thisNode->referenceCount++; //dug9 added but should???
					//getNode(ud,TOP) = thisNode; 
					#ifdef X3DPARSERVERBOSE
					printf ("successful copying for field %s defName %s\n",atts[i], atts[i+1]);
					#endif

				}
			}

		/* do all the normal fields when we are ending the node */
		} else {
			nvp = MALLOC(struct nameValuePairs* , sizeof (struct nameValuePairs));
			nvp->fieldName = STRDUP(atts[i]);
			nvp->fieldValue=STRDUP(atts[i+1]);
			//ConsoleMessage("name[%s] value[%s]\n",atts[i], atts[i+1]);
			nvp->fieldType = 0;
			//vector_pushBack(struct nameValuePairs*, p->childAttributes[tg->X3DParser.parentIndex], nvp);
			vector_pushBack(struct nameValuePairs*,childAttributes, nvp);
		}
	}
	pushNode(ud,thisNode);
	setAtt(ud,TOP,childAttributes);
}

static xmlChar* fixAmp(const unsigned char *InFieldValue)
{
	char *fieldValue = (char *)InFieldValue;

	//for x3d string '"&amp;"' libxml2 gives us &#38; 
	//we want & like other browsers get
	//we do it by left-shifting over the #38;
	//fieldValue - the MFString before splitting into SFs ie ["you & me" "John & Ian"\0]
	//except that libxml2 will wrongly give you ["you &#38; me" "John &#38; Ian"\0]
	if(fieldValue)
	{
		char *pp = strstr((char *)fieldValue,"&#38;");
		while(pp){
			memmove(pp+1,pp+5,strlen(fieldValue) - (pp+1 - fieldValue));
			pp = strstr(pp,"&#38;");
			/* or the following works if you don't have memmove:
			int len, nmove, ii;
			len = strlen(fieldValue);
			pp++;
			nmove = (len+1) - (pp - fieldValue);
			for(ii=0;ii<nmove;ii++){
				*pp = *(pp+4);
				pp++;
			}
			*/
		}
	}
	return (xmlChar *)fieldValue;
}
static void parseAttributes_B(void *ud, char **atts) {
	int i, type, kind, iifield;
	struct X3D_Node *node;
	char *name, *svalue, *ignore [] = {"containerField","USE", "DEF"};
	union anyVrml *value;

	node = getNode(ud,TOP);
	for (i=0; atts[i]; i+=2) {
		name = atts[i];
		svalue = atts[i+1];
		/* see if we have a containerField here */
		if(findFieldInARR(name,ignore,3) == INT_ID_UNDEFINED){
			if(getFieldFromNodeAndName(node,name,&type,&kind,&iifield,&value)){
				deleteMallocedFieldValue(type,value);
				Parser_scanStringValueToMem_B(value, type,svalue, TRUE);
			}
		}
		if(!strcmp(name,"side")){
			//stereoscopic experiments
			if(!strcmp(svalue,"left"))
				node->_renderFlags |= VF_HideRight;
			else if(!strcmp(svalue,"right"))
				node->_renderFlags |= VF_HideLeft;
			//printf("node renderflags=%d\n",node->_renderFlags);
		}
	}
}

static void parseAttributes(void *ud) {
	int ind;
	struct nameValuePairs *nvp;
	struct X3D_Node *thisNode;
	struct Vector *childAttributes;
	ttglobal tg = gglobal();
	ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;

	thisNode = getNode(ud,TOP);
	childAttributes = getAtt(ud,TOP);
	 /* printf  ("parseAttributes..level %d for node type %s\n",parentIndex,stringNodeType(thisNode->_nodeType));  */
	//if(p->childAttributes[tg->X3DParser.parentIndex])
	if(childAttributes)
	//for (ind=0; ind<vectorSize(p->childAttributes[tg->X3DParser.parentIndex]); ind++) {
	for (ind=0; ind<vectorSize(childAttributes); ind++) {
		//nvp = vector_get(struct nameValuePairs*, p->childAttributes[tg->X3DParser.parentIndex],ind);
		nvp = vector_get(struct nameValuePairs*, childAttributes,ind);
		 /* printf ("	nvp %ld, fieldName:%s fieldValue:%s\n",ind,nvp->fieldName,nvp->fieldValue); */

		/* see if we have a containerField here */
		if (strcmp("containerField",nvp->fieldName)==0) {
			indexT tmp;
			//printf ("SETTING CONTAINER FIELD TO %s for node of type %s\n",nvp->fieldValue, stringNodeType(thisNode->_nodeType ));
			tmp = findFieldInFIELDNAMES(nvp->fieldValue);
			if (tmp == INT_ID_UNDEFINED) {
				ConsoleMessage ("Error line %d: setting containerField to :%s: for node of type :%s:\n", LINE,
					nvp->fieldValue, stringNodeType(thisNode->_nodeType ));
			} else {
				thisNode->_defaultContainer = tmp;
			}
		} else {
			/* Scripts/Shaders are different from normal nodes - here we go through the initialization tables for the 
			   Script/Shader, and possibly change the initialization value - this HAS to be run before the script is
			   initialized in JSInitializeScriptAndFields of course! */
			switch (thisNode->_nodeType) {
				case NODE_Script:
				case NODE_ComposedShader: 
				case NODE_ShaderProgram:
				case NODE_PackagedShader: {
					int rv, offs, type, accessType;
					struct Shader_Script *myObj;

					UNUSED(rv); // compiler warning mitigation

					/* this is a Shader/Script, look through the parameters and see if there is a replacement for value */
					rv = getRoutingInfo (p->myLexer, thisNode, &offs, &type, &accessType, &myObj, nvp->fieldName,0);
					/* printf ("parseAttributes, for fieldName %s value %s have offs %d type %d accessType %d rv %d\n",
						nvp->fieldName, nvp->fieldValue,offs,type,accessType, rv); */


					/* found the name, if the offset is not INT_ID_UNDEFINED */
					if (offs != INT_ID_UNDEFINED) {
						int kk;
//#define OLDWAY33 1
#ifdef OLDWAY33
						struct ScriptParamList *thisEntry;
						struct CRscriptStruct *ScriptControl = getScriptControl();

						thisEntry = ScriptControl[myObj->num].paramList;
						kk= -1;
						while (thisEntry != NULL) {
							kk++;
#else
						int nfield = vectorSize(myObj->fields);
						//printf("number of fields in script=%d\n",nfield);
						for(kk=0;kk<nfield;kk++){

#endif
							/* printf ("script field is %s\n",thisEntry->field); */
							int itype, kind;
							union anyVrml *value;
							char *fname;
#ifdef OLDWAY33
							fname = thisEntry->field;
							kind = thisEntry->kind;
							value = &thisEntry->value;
							itype = thisEntry->type;
#else
							//struct Vector *sfields;
							struct ScriptFieldDecl *sfield;
							struct FieldDecl *fdecl;
							struct CRjsnameStruct *JSparamnames = getJSparamnames();

							sfield = vector_get(struct ScriptFieldDecl *,myObj->fields,kk); //offs);
							//if(sfield->ASCIIvalue) printf("Ascii value=%s\n",sfield->ASCIIvalue);
							fdecl = sfield->fieldDecl;
							fname = fieldDecl_getShaderScriptName(fdecl);
							//if(!strcmp(fieldName,fieldname)){
							itype = fdecl->fieldType;
							kind = fdecl->PKWmode;
							value = &(sfield->value);
#endif
							if (strcmp (nvp->fieldName, fname) == 0) {
								//printf("nvp->fname = %s fname=%s itype=%d kind=%d jindx=%d\n",nvp->fieldName, fname,itype,kind,kk);

								/* printf ("name MATCH\n");
								printf ("thisEntry->kind %d type %d value %f\n",
									thisEntry->kind,thisEntry->type,thisEntry->value); */
								if ((kind==PKW_initializeOnly) || (kind==PKW_inputOutput)) 
								{
									if (nvp->fieldValue== NULL) {
										ConsoleMessage ("PROTO connect field, an initializeOnly or inputOut needs an initialValue for name %s",nvp->fieldName);
									} else {
										if(nvp->fieldType == 0)
										{
											/* printf ("have to parse fieldValue :%s: and place it into my value\n",nvp->fieldValue);  */
											Parser_scanStringValueToMem(X3D_NODE(value), 0, 
												itype, nvp->fieldValue, TRUE);
										}
										else if(nvp->fieldType == 1)
										{
											/* not currently implemented, but reserved for DEF index style
											?? itoa(DEF index)
											?? np = getEAINodeFromTable(atoi(value), -1);
											*/
										}
										else if(nvp->fieldType == FIELDTYPE_MFNode || nvp->fieldType == FIELDTYPE_SFNode )  
										{
											/*dug9 added July 18, 2010 (search for BIGPUSH / BIGPOP to find where data set)
												we have an MFNode field already in binary form 
												<fieldValue name="Buildings">
													<Transform USE="House1"/>
													<Transform USE="House2">
												</fieldValue>
												the House transforms have been parsed as regular nodes and 
												the top level nodes listed in an MFNode
											*/
											union anyVrml* av;
											if(sscanf(nvp->fieldValue,"%p",&av) != 1)
											{
												printf ("parseAttributes - can not get handle from %s\n",nvp->fieldValue);
											}
											else
											{
												/* printf("parseAttributes - got %d mf fields back from pointer %s \n",((struct Multi_Node *)av)->n,nvp->fieldValue);*/
												if( type == FIELDTYPE_MFNode )
												{
													memcpy(value,av,sizeof(union anyVrml)); 
												}
												else if( type == FIELDTYPE_SFNode)
												{
													if(nvp->fieldType == FIELDTYPE_SFNode)
													{
														/* this came from the ProtoInterface field where we knew the type */
														memcpy(value,av,sizeof(union anyVrml)); 
													}
													else if(nvp->fieldType == FIELDTYPE_MFNode)
													{
														/* this came from the ProtoInstance fieldValue where did not know the type
															and we guessed at MFNode to be most general. But we were wrong in this
															case - so down-convert first node in MFNode to SFNode and forget the rest */
														//struct X3D_Transform *tt = (struct X3D_Transform*)((struct Multi_Node *)av)->p[0];
														struct X3D_Node *tt = (struct X3D_Node*)((struct Multi_Node *)av)->p[0];
														memcpy(value,&tt,sizeof(struct X3D_Node*));
														/* could free the rest of the unused MFnodes here if there are some and we were ambitious */
													}
												}
												else
												{
													//we got an MFNode or SFNode in the ProtoInstance fieldValue
													//but the expanded proto and Script node don't want it
													printf("ProtoInstance fieldValue type MFNode or SFNode Proto type %d mismatch\n",type);
													/* could free all the unused MFnodes here if we were ambitious */
												}
												FREE_IF_NZ(av); //size of union anyVrml, malloced elsewhere
											}
										}
										/* printf ("done this parsing\n"); */
									}
								}
							}
#ifdef OLDWAY33
							thisEntry=thisEntry->next;
#endif
						}
					} else {
						/* some fields, eg "PackagedShader language field" or any other field that is not a field,
							is just a normal field as defined in the spec, so make it so */
						setField_fromJavascript (thisNode, nvp->fieldName,nvp->fieldValue, TRUE);
					}
				}
				break;
				default: 
					//setField_fromJavascript (thisNode, nvp->fieldName,nvp->fieldValue, TRUE);
					//break;
					/* experiment for ISing to non-script children fields */
					//if(nvp->fieldType == 0)
					//{
					//	setField_fromJavascript (thisNode, nvp->fieldName,nvp->fieldValue, TRUE);
					//}
					if(nvp->fieldType == 1)
					{
						/* not currently implemented, but reserved for DEF index style
						?? itoa(DEF index)
						?? np = getEAINodeFromTable(atoi(value), -1);
						*/
					}
					else if(nvp->fieldType == FIELDTYPE_MFNode)  
					{
						int foffset;
						int coffset;
						int ctype;
						int ctmp;
						//union anyVrml* av;
						struct Multi_Node * mv;

						if(sscanf(nvp->fieldValue,"%p",&mv) != 1)
						{
							printf ("parseAttributes - can not get handle from %s\n",nvp->fieldValue);
						}
						else
						{
							union anyVrml *nst;
							struct Multi_Node * tn;

							/* is this a valid field? */
							foffset = findRoutedFieldInFIELDNAMES(thisNode,nvp->fieldName,1);	

							if (foffset < 0) {
								ConsoleMessage ("field %s is not a valid field of a node %s",nvp->fieldName,stringNodeType(thisNode->_nodeType));
								printf ("field %s is not a valid field of a node %s\n",nvp->fieldName,stringNodeType(thisNode->_nodeType));
								return;
							}

							/* get offsets for this field in this nodeType */
							#ifdef SETFIELDVERBOSE
/* ..does not compile right now...
							printf ("getting nodeOffsets for type %s field %s value %s\n",stringNodeType(node->_nodeType),field,value); 
*/
							#endif

							findFieldInOFFSETS(thisNode->_nodeType, foffset, &coffset, &ctype, &ctmp);

							nst = offsetPointer_deref(union anyVrml *,thisNode,coffset);
							tn = offsetPointer_deref(struct Multi_Node*,thisNode,coffset);
							/*printf("parseAttributes - got %d mf fields back from pointer %s \n",mv->n,nvp->fieldValue);*/
							if( ctype == FIELDTYPE_MFNode )
							{
								/* (search for BIGPUSH / BIGPOP to find where data set) */
								AddRemoveChildren(thisNode,tn,(struct X3D_Node **)mv->p,mv->n,0,__FILE__,__LINE__);
							}
							else if( ctype == FIELDTYPE_SFNode)
							{
								/* down-convert first node in MFNode to SFNode and forget the rest */
								memcpy(nst,mv->p[0],sizeof(&nst));
								/* could free the rest of the unused MFnodes here if there are some and we were ambitious */
							}
							else
							{
								//we got an MFNode or SFNode in the ProtoInstance fieldValue
								//but the expanded proto and Script node don't want it
								printf("ProtoInstance fieldValue type MFNode or SFNode Proto type %d mismatch\n",ctype);
								/* could free all the unused MFnodes here if we were ambitious */
							}
							FREE_IF_NZ(mv->p); //size of n * (struct X3DNode *)
							FREE_IF_NZ(mv); //size of union anyVrml
						}

					}
					else //if(nvp->fieldType == 0)
					{
						//I get -85529292 so some branches arent setting .value or .fieldValue to 0.
						//printf("Unknown fieldType %d\n",nvp->fieldType);
						setField_fromJavascript (thisNode, nvp->fieldName,nvp->fieldValue, TRUE);

					}
			}
		}

		/* do not need these anymore */
		FREE_IF_NZ(nvp->fieldName);
		FREE_IF_NZ(nvp->fieldValue);
		FREE_IF_NZ(nvp);
	}
}
void parseConnect_B(void *ud, char **atts);
static void XMLCALL X3DstartElement(void *ud, const xmlChar *iname, const xmlChar **atts) {
	int myNodeIndex;
	char **myAtts;
	int i;
	char *blankAtts[] = {NULL,NULL};
	const char *name = (const char*) iname;  // get around compiler warnings on iPhone, etc...
	ttglobal tg = gglobal();
	ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;

	/* libxml passes NULL, while expat passes {0,0}. Make them the same */
	if (atts == NULL) myAtts = blankAtts;
	else myAtts = (char **) atts;
	
	#ifdef X3DPARSERVERBOSE
	//printf ("startElement: %s : level %d parserMode: %s \n",name,parentIndex,parserModeStrings[getMode(ud,TOP)]);
	printf ("X3DstartElement: %s: atts %p\n",name,atts);
	//printf ("startElement, myAtts :%p contents %p\n",myAtts,myAtts[0]);
	{ int i;
			for (i = 0; myAtts[i]; i += 2) {
					printf("	      X3DStartElement field:%s=%s\n", myAtts[i], atts[i + 1]);
	}}

	//printf ("X3DstartElement - finished looking at myAtts\n\n");
	#endif

	if(atts)
		for (i = 0; atts[i]; i += 2) {
			atts[i+1] = fixAmp(atts[i+1]);
		}
	
	/* are we storing a PROTO body?? */
	if(!usingBrotos())
	if (getMode(ud,TOP) == PARSING_PROTOBODY) {
		dumpProtoBody(name,myAtts);
		return;
	}

	/* maybe we are doing a Proto Instance?? */
	if(!usingBrotos())
	if (getMode(ud,TOP) == PARSING_PROTOINSTANCE) {
		saveProtoInstanceFields(ud,name,myAtts);
		return;
	}



	myNodeIndex = findFieldInNODES(name);

	/* is this a "normal" node that can be found in x3d, x3dv and wrl files? */
	if (myNodeIndex != INT_ID_UNDEFINED) {
		if(usingBrotos()){
			startBuiltin_B(ud,myNodeIndex,(const xmlChar *)name,myAtts);
		}else{
			DEBUG_X3DPARSER ("	creating new vector for parentIndex %d\n",tg->X3DParser.parentIndex); 
			saveAttributes(ud,myNodeIndex,(const xmlChar *)name,myAtts);
		}
		return;
	}
	/*in theory, you could search broto prototype typenames here, and if found
		assume it's instanced with builtin syntax (instead of ProtoInstance syntax)
		if( isAvailableBroto(name, getContext(ud,TOP) , &proto))
	*/

	/* no, it is not. Lets see what else it could be... */
	myNodeIndex = findFieldInX3DSPECIAL(name);
	if (myNodeIndex != INT_ID_UNDEFINED) {
		switch (myNodeIndex) {
			case X3DSP_ProtoDeclare: 
				if(usingBrotos()) parseProtoDeclare_B(ud,myAtts);
				else parseProtoDeclare(ud,myAtts); 
				break;
			case X3DSP_ExternProtoDeclare: 
				if(usingBrotos()) parseExternProtoDeclare_B(ud,myAtts);
				else parseExternProtoDeclare(ud,myAtts); 
				break;
			case X3DSP_ProtoBody: 
				if(usingBrotos()) parseProtoBody_B(ud,myAtts); 
				else parseProtoBody(ud,myAtts); 
				break;
			case X3DSP_ProtoInterface: 
				parseProtoInterface(ud,myAtts); break;
			case X3DSP_ProtoInstance: 
				if(usingBrotos()) parseProtoInstance_B(ud,myAtts); 
				else parseProtoInstance(ud,myAtts); 
				break;
			case X3DSP_ROUTE: 
				if(usingBrotos()) parseRoutes_B(ud,myAtts);
				else parseRoutes(myAtts); 
				break;
			case X3DSP_meta: parseMeta(myAtts); break;
			case X3DSP_Scene: parseScene(myAtts); break;
			case X3DSP_head:
			case X3DSP_Header: parseHeader(myAtts); break;
			case X3DSP_X3D: parseX3Dhead(myAtts); break;
			case X3DSP_fieldValue:  
				if(usingBrotos()) parseFieldValue_B(ud,myAtts);
				else parseFieldValue(ud,name,myAtts); 
				break;
			case X3DSP_field: 
				if(usingBrotos()) parseScriptProtoField_B (ud, myAtts);
				else parseScriptProtoField (ud, p->myLexer, myAtts); break;
			case X3DSP_IS: parseIS(ud); break;
			case X3DSP_component: parseComponent(myAtts); break;
			case X3DSP_EXPORT: 
				if(usingBrotos()) parseExport_B(ud,myAtts);
				else parseExport(myAtts); 
				break;
			case X3DSP_IMPORT: 
				if(usingBrotos()) parseImport_B(ud,myAtts);
				else parseImport(myAtts); break;
			case X3DSP_connect: 
				if(usingBrotos()) parseConnect_B(ud,myAtts);
				else parseConnect(ud,p->myLexer, myAtts,getAtt(ud,TOP)); 
				break;

			default: printf ("	huh? startElement, X3DSPECIAL, but not handled?? %d, :%s:\n",myNodeIndex,X3DSPECIAL[myNodeIndex]);
		}
		return;
	}

	printf ("startElement name  do not currently handle this one :%s: index %d\n",name,myNodeIndex); 
}
void endScriptProtoField(void *ud); //struct VRMLLexer* myLexer);
void endScriptProtoField_B(void *ud); //struct VRMLLexer* myLexer);

static void XMLCALL X3DendElement(void *ud, const xmlChar *iname) {
	int myNodeIndex;
    const char*name = (const char*) iname;
	//ttglobal tg = gglobal();
	//ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;

	/* printf ("X3DEndElement for %s\n",name); */

	#ifdef X3DPARSERVERBOSE
	printf ("endElement: %s : parentIndex %d mode %s\n",name,parentIndex,parserModeStrings[getMode(ud,TOP)]); 
	#endif

	/* are we storing a PROTO body?? */
	if(!usingBrotos())
	if (getMode(ud,TOP) == PARSING_PROTOBODY) {
		/* are we finished with this ProtoBody? */
		if (strcmp("ProtoBody",name)==0) {
			/* do nothing... setParserMode(PARSING_PROTODECLARE); */
		} else {
			addToProtoCode(name);
			return;
		}
	}

	/* are we parsing a PROTO Instance still? */
	if(!usingBrotos())
	if (getMode(ud,TOP) == PARSING_PROTOINSTANCE) {
		endProtoInstanceField(ud,name);
		return;
	}

	/* is this an SFNode for a Script field? */
	if(!usingBrotos())
	if (getMode(ud,TOP) == PARSING_SCRIPT) {
		switch (getNode(ud,TOP-1)->_nodeType) {
			case NODE_Script:

/* I wonder if there is a better way of handling this case */
			#ifdef X3DPARSERVERBOSE
			printf ("linkNodeIn, got parsing script, have to link node into script body\n");
			printf ("linking in %s to %s, field %s (%d)\n",
				stringNodeType(getNode(ud,TOP)->_nodeType),
				stringNodeType(getNode(ud,TOP-1)->_nodeType),
				stringFieldType(getNode(ud,TOP)->_defaultContainer),
				getNode(ud,TOP)->_defaultContainer);
			printf ("but skipping this\n");
			#endif

			popNode(ud);
			return;
			break;

			default: {};
		}
	}
		

	myNodeIndex = findFieldInNODES(name);
	if (myNodeIndex != INT_ID_UNDEFINED) {
		if(usingBrotos()){
			endBuiltin_B(ud,iname);
		}else{
			struct Vector *childAttributes;
			/* printf ("endElement - normalNode :%s:\n",name); */
			if (myNodeIndex == NODE_Script) {
				#ifdef HAVE_JAVASCRIPT
				initScriptWithScript(ud);
				#endif
			}
			parseAttributes(ud);
			linkNodeIn(ud,__FILE__,__LINE__);


			childAttributes = getAtt(ud,TOP);
			if (childAttributes!=NULL) deleteVector (struct nameValuePairs*, childAttributes);
			setAtt(ud,TOP,NULL);

			popNode(ud);
		}
		return;

	}

		

	/* no, it is not. Lets see what else it could be... */
	myNodeIndex = findFieldInX3DSPECIAL(name);
	if (myNodeIndex != INT_ID_UNDEFINED) {
		switch (myNodeIndex) {
			case X3DSP_ProtoInstance: 
				if(usingBrotos()) endProtoInstance_B(ud,name);
				break;
			case X3DSP_ProtoInterface: endProtoInterfaceTag(ud); break;
			case X3DSP_ProtoBody: 
				if(usingBrotos()) endProtoBodyTag_B(ud,name);
				else endProtoBodyTag(ud,name); break;
			case X3DSP_ProtoDeclare: 
				if(usingBrotos()) endProtoDeclareTag_B(ud);
				else endProtoDeclareTag(ud); 
				break;
			case X3DSP_ExternProtoDeclare: 
				if(usingBrotos()) endExternProtoDeclareTag_B(ud);
				else endExternProtoDeclareTag(ud); 
				break;
			case X3DSP_IS: endIS(ud); break;
			case X3DSP_connect:
			case X3DSP_ROUTE: 
			case X3DSP_meta:
			case X3DSP_Scene:
			case X3DSP_head:
			case X3DSP_Header:
			case X3DSP_component:
			case X3DSP_EXPORT:
			case X3DSP_IMPORT:
			case X3DSP_X3D: break;
			case X3DSP_field:
				if(usingBrotos()) endScriptProtoField_B(ud);
				else endScriptProtoField(ud);
				break;
			case X3DSP_fieldValue:
				if(usingBrotos()) endFieldValue_B(ud);
				else setFieldValueDataActive(ud,name);
				break;
			
			/* should never do this: */
			default: 
			printf ("endElement: huh? X3DSPECIAL, but not handled?? %s\n",X3DSPECIAL[myNodeIndex]);
		}
		return;
	}

	printf ("unhandled endElement name %s index %d\n",name,myNodeIndex); 
	#ifdef X3DPARSERVERBOSE
	printf ("endElement %s\n",name);
	#endif
}

static XML_Parser initializeX3DParser () {
	ppX3DParser p = (ppX3DParser)gglobal()->X3DParser.prv;
	p->X3DParserRecurseLevel++;

	if (p->X3DParserRecurseLevel >= PROTOINSTANCE_MAX_LEVELS) {
		ConsoleMessage ("XML_PARSER init: XML file PROTO nested too deep\n");
		p->X3DParserRecurseLevel--;
	} else {
		XML_CreateParserLevel(p->x3dparser[p->X3DParserRecurseLevel]);
		XML_SetElementHandler(p->x3dparser[p->X3DParserRecurseLevel], X3DstartElement, X3DendElement);
		XML_SetCdataSectionHandler (p->x3dparser[p->X3DParserRecurseLevel], startCDATA, endCDATA);
		XML_SetDefaultHandler (p->x3dparser[p->X3DParserRecurseLevel],handleCDATA);
		XML_SetUserData(p->x3dparser[p->X3DParserRecurseLevel], &parentIndex);
	}
	/* printf ("initializeX3DParser, level %d, parser %u\n",x3dparser[X3DParserRecurseLevel]); */
	return p->x3dparser[p->X3DParserRecurseLevel];
}

static void shutdownX3DParser (void *ud) {
	ttglobal tg = gglobal();
	ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;

	/* printf ("shutdownX3DParser, recurseLevel %d\n",X3DParserRecurseLevel); */
	XML_ParserFree(p->x3dparser[p->X3DParserRecurseLevel]);
	p->X3DParserRecurseLevel--;
	
	/* lets free up memory here for possible PROTO variables */
	if (p->X3DParserRecurseLevel == INT_ID_UNDEFINED) {
		/* if we are at the bottom of the parser call nesting, lets reset parentIndex */
		gglobal()->X3DParser.parentIndex = 0; //setParentIndex( 0 );
		freeProtoMemory ();
	}

	if (p->X3DParserRecurseLevel < INT_ID_UNDEFINED) {
		ConsoleMessage ("XML_PARSER close underflow");
		p->X3DParserRecurseLevel = INT_ID_UNDEFINED;
	}

	/* CDATA text space, free it up */
	FREE_IF_NZ(tg->X3DParser.CDATA_Text);
	p->CDATA_TextMallocSize = 0; 
	if (p->X3DParserRecurseLevel > INT_ID_UNDEFINED)
		p->currentX3DParser = p->x3dparser[p->X3DParserRecurseLevel];
	/* printf ("shutdownX3DParser, current X3DParser %u\n",currentX3DParser); */
	popMode(ud);

	if(p->DEFedNodes){
		int i;
		for(i=0;i<vectorSize(p->DEFedNodes);i++){
			struct Vector* vd = vector_get(struct Vector*,p->DEFedNodes,i);
			deleteVector(struct X3D_Node*,vd);
		}
		deleteVector(struct Vector*, p->DEFedNodes);
	}

	/*
    * Cleanup function for the XML library.
    */
   xmlCleanupParser();
   /*
    * this is to debug memory for regression tests
    */
   xmlMemoryDump();
}

int X3DParse (struct X3D_Node* ectx, struct X3D_Node* myParent, const char *inputstring) {
	ttglobal tg = gglobal();
	ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;
	p->currentX3DParser = initializeX3DParser();

	/* printf ("X3DParse, current X3DParser is %u\n",currentX3DParser); */

	if(!usingBrotos()) {
		/* Use classic parser Lexer for storing DEF name info */
		if (p->myLexer == NULL) p->myLexer = newLexer();
		if (p->DEFedNodes == NULL) {
			p->DEFedNodes = newStack(struct Vector*);
			ASSERT(p->DEFedNodes);
			#define DEFMEM_INIT_SIZE 16
			stack_push(struct Vector*, p->DEFedNodes,
        			   newVector(struct X3D_Node*, DEFMEM_INIT_SIZE));
			ASSERT(!stack_empty(p->DEFedNodes));
		}
	}


	DEBUG_X3DPARSER ("X3DPARSE on :\n%s:\n",inputstring);
	if(p->user_data == NULL){
		//just once. We are re-entrant when parsing text protos (trotos)
		// and want to keep the stack for the parent scene
		p->user_data = new_xml_user_data();
	}
	if(usingBrotos()) {
		pushContext(p->user_data,ectx);
		if(myParent->_nodeType == NODE_Proto )
			pushField(p->user_data,"__children");
		else
			pushField(p->user_data,"children");
	}
	pushNode(p->user_data,myParent);
	pushMode(p->user_data,PARSING_NODES);

	if (XML_ParseFile(p->currentX3DParser, p->user_data, inputstring, (int) strlen(inputstring), TRUE) == XML_STATUS_ERROR) {
		// http://xmlsoft.org/html/libxml-xmlerror.html
		xmlErrorPtr xe = xmlGetLastError();
		ConsoleMessage("Xml Error %s \n",xe->message);
		ConsoleMessage("Line %d\n",xe->line);
		/*
		fprintf(stderr,
			"%s at line %d\n",
			XML_ErrorString(XML_GetErrorCode(currentX3DParser)),
			XML_GetCurrentLineNumber(currentX3DParser));
		*/
		if(usingBrotos()) popField(p->user_data);
		shutdownX3DParser(p->user_data);
		Parser_deleteParserForScanStringValueToMem();
		return FALSE;
	}
	if(usingBrotos()) popField(p->user_data);
	shutdownX3DParser(p->user_data);
	Parser_deleteParserForScanStringValueToMem();
	return TRUE;
}
