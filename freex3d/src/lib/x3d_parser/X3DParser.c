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
#include "../vrml_parser/CParse.h"
#include "../vrml_parser/CRoutes.h"
#include "../input/EAIHeaders.h"	/* resolving implicit declarations */
#include "../input/EAIHelpers.h"	/* resolving implicit declarations */

#include "X3DParser.h"


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
struct mode_name {
int mode;
const char *name;
} mode_names [] = {
PARSING_NODES,"PARSING_NODES",
PARSING_SCRIPT,"PARSING_SCRIPT",
PARSING_PROTODECLARE,"PARSING_PROTODECLARE",
PARSING_PROTOINTERFACE,"PARSING_PROTOINTERFACE",
PARSING_PROTOBODY,"PARSING_PROTOBODY",
PARSING_PROTOINSTANCE,"PARSING_PROTOINSTANCE",
PARSING_IS,"PARSING_IS",
PARSING_CONNECT,"PARSING_CONNECT",
PARSING_EXTERNPROTODECLARE,"PARSING_EXTERNPROTODECLARE",
PARSING_FIELD,"PARSING_FIELD",
PARSING_PROTOINSTANCE_USE,"PARSING_PROTOINSTANCE_USE",
0,NULL,
};
const char* getModeName(int mode){
	//return printable modename
	struct mode_name *mname;
	int i;
	i = 0;
	do{
		mname = &mode_names[i];
		if(mode == mname->mode) 
			return mname->name;
		i++;
	}while(mname->name);
	return NULL;
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
void printModeStack(void *userData){
	int i,j,mode,isize;
	struct xml_user_data *ud = (struct xml_user_data *)userData;
	isize =  vectorSize(ud->modes);
	printf("mode_stack size=%d top to bottom:\n",isize);
	for(i=0;i<isize;i++){
		j = isize - i - 1;
		mode =  vector_get(int,ud->modes, j);
		printf("%s\n",getModeName(mode));
	}
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
	// used by both x3d and vrml parsers, to quality check each end of a route for validity,
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
	int type, kind, iifield, ok, isRootNode, mode;
	union anyVrml *value;
	const char *fname;

	mode = getMode(ud,TOP);
	node = getNode(ud,TOP);
	parent = getNode(ud,TOP-1);
	if(!node || !parent)return;
	isRootNode = FALSE;
	if(parent->_nodeType == NODE_Proto){
		if(mode == PARSING_PROTOBODY) isRootNode = TRUE;
	}
	//if(parent->_nodeType == NODE_TransformSensor)
	//	printf("adding a node to transformsensor\n");
	if(isRootNode){
		//if we are adding a rootnode to scene or protobody, it should be added to
		// the scene/protobody's private __children field
		// (not to any of the proto's public fields, for example if the proto author called a public field 'children')
		union anyVrml *valueadd = NULL;
		ok = getFieldFromNodeAndName(parent,"__children",&type,&kind,&iifield,&valueadd);
		AddRemoveChildren(parent,&valueadd->mfnode,&node,1,1,__FILE__,__LINE__);
	}else{
		parentsSuggestion = getField(ud,TOP-1);

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
	endCDATA_B(ud,string,len);
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
		//is there a function for zeroing a fieldValue of anytype? Need it here.
		//in xml the MFNode in particular will get 'added to' ie mf.n++ later, so need to clear that
		// see tests/protos/questionforexperts_mod.x3d
		if(pfield->type == FIELDTYPE_MFNode){
			struct Multi_Node* mfn = &pfield->defaultVal.mfnode;
			if(mfn->n)
				AddRemoveChildren(node,mfn,mfn->p,mfn->n,2,__FILE__,__LINE__);
			pfield->defaultVal.mfnode.n = 0;
			pfield->defaultVal.mfnode.p = NULL;
		}
		if(pfield->type == FIELDTYPE_SFNode){
			struct X3D_Node **sfn = &pfield->defaultVal.sfnode;
			if(*sfn)
				AddRemoveSFNodeFieldChild(node,sfn,*sfn,2,__FILE__,__LINE__);
			pfield->defaultVal.sfnode = NULL;
		}
		pfield->alreadySet = TRUE;
	}

	pushField(ud,cname); //in case there's no value, because its SF or MFNodes in child xml, or in CDATA
}
static void endFieldValue_B(void *ud){
	if(0) printf("endFieldValue\n");
	//in x3d, <fieldvalue type=SFNode><a node></fieldValue>

	popField(ud);
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

static void endExternProtoDeclareTag_B(void *ud) {
	popMode(ud);
	popNode(ud);
	popField(ud);
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


void deep_copy_broto_body2(struct X3D_Proto** proto, struct X3D_Proto** dest);
static void endProtoInstance_B(void *ud, const char *name) {
	//now that initial field values are set, deep copy the broto body
	int mode;
	struct X3D_Node *node;
	if(0) printf("endProtoInstance_B\n");

	node = getNode(ud,TOP);
	mode = getMode(ud,TOP);
	if(node){
		if(node->_nodeType == NODE_Proto || node->_nodeType == NODE_Inline ){
			if(mode != PARSING_PROTOINSTANCE_USE){
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
		}
		linkNodeIn_B(ud);
	}
	popField(ud);
	popNode(ud);
	popMode(ud);
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
void push_binding_stack_set(struct X3D_Node* layersetnode);
void push_next_layerId_from_binding_stack_set(struct X3D_Node* layer);
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
				ConsoleMessage("\n");
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
			push_binding_stack_set(node);
		if(node->_nodeType == NODE_Layer || node->_nodeType == NODE_LayoutLayer)
			push_next_layerId_from_binding_stack_set(node);
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
		struct Shader_Script *myObj;

		/* create the Shader_Script for this one */
		X3D_SCRIPT(thisNode)->__scriptObj=new_Shader_Script(thisNode);


		#ifdef X3DPARSERVERBOSE
		printf ("working through script parentIndex %d\n",parentIndex);
		#endif

		myObj = X3D_SCRIPT(thisNode)->__scriptObj;
		JSInit(myObj); /* ->num); */
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
int findFieldInARR(const char* field, const char** arr, size_t cnt);
void parseScriptProtoField_B(void *ud, char **atts) {
	/* new user field definitions -name,type,mode- possibly with fieldvalue anyVrml
		- we will be parsing either:
			a ProtoDeclare or ExternProtoDeclare (extern will lack fieldValue)
			a Script Node or Shader node
		- Script field may have fieldValue as child element, or IS/connect as peer
		- ProtoDeclare may have fieldValue as child element
	*/ 
	//struct X3D_Node *node;
	int mp_name, mp_accesstype, mp_type, mp_value, i;
	int pkwmode, type;
	union anyVrml defaultValue; //, *value;
	char *fname, *cname;
	//value = NULL;
	cname = NULL;
	mp_name = mp_accesstype = mp_type = mp_value = ID_UNDEFINED;
	if(0) printf("start scriptProtoField\n");
	/* have a "key" "value" pairing here. They can be in any order; put them into our order */
	for (i = 0; atts[i]; i += 2) {
		/* skip any "appinfo" or "documentation" fields here */
		if ((strcmp("appinfo", atts[i]) != 0)  &&
			(strcmp("documentation",atts[i]) != 0)) {
			if (strcmp(atts[i],"name") == 0) { mp_name = i+1;
			} else if (strcmp(atts[i],"accessType") == 0) { mp_accesstype = i+1;
			} else if (strcmp(atts[i],"type") == 0) { mp_type = i+1;
			} else if (strcmp(atts[i],"value") == 0) { mp_value = i+1;
			} else {
				ConsoleMessage ("X3D Proto/Script parsing line %d: unknown field type %s",LINE,atts[i]);
				return;
			}
		}
	}
	if(mp_accesstype > -1 && mp_type > -1 && mp_name > -1){
		int valueSet;
		pkwmode = findFieldInARR(atts[mp_accesstype], PROTOKEYWORDS, PROTOKEYWORDS_COUNT);
		pkwmode = pkwmode > -1? X3DMODE(pkwmode) : pkwmode;
		type = findFieldInARR(atts[mp_type],FIELDTYPES,FIELDTYPES_COUNT);
		fname = atts[mp_name];
		cname = NULL;
		//memset(&defaultValue,0,sizeof(union anyVrml));
		bzero(&defaultValue, sizeof (union anyVrml));
		valueSet = FALSE;
		if(mp_value > -1){
			Parser_scanStringValueToMem_B(&defaultValue, type, atts[mp_value], TRUE);
			valueSet = TRUE;
		}
		if(pkwmode > -1 && type > -1){
			struct X3D_Node * node = getNode(ud,TOP);
			if(node->_nodeType == NODE_Proto){
				struct X3D_Proto *pnode;
				struct ProtoFieldDecl* pfield;
				struct ProtoDefinition* pstruct;
				pnode = X3D_PROTO(node);
				pstruct = (struct ProtoDefinition*) pnode->__protoDef;
				pfield = newProtoFieldDecl(pkwmode,type,0);
				pfield->cname = STRDUP(fname);
				cname = pfield->cname;
				memcpy(&pfield->defaultVal,&defaultValue,sizeof(union anyVrml));
				vector_pushBack(struct ProtoFieldDecl*, pstruct->iface, pfield);
				//value = &pfield->defaultVal;
			}else{
				struct Shader_Script* shader = NULL;
				struct ScriptFieldDecl* sfield;
				int jsname;
				switch(node->_nodeType) 
				{ 
					case NODE_Script:         shader =(struct Shader_Script *)(X3D_SCRIPT(node)->__scriptObj); break;
					case NODE_ComposedShader: shader =(struct Shader_Script *)(X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields); break;
					case NODE_ShaderProgram:  shader =(struct Shader_Script *)(X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields); break;
					case NODE_PackagedShader: shader =(struct Shader_Script *)(X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields); break;
				}
				jsname = JSparamIndex (fname, atts[mp_type]);
				cname = getJSparamnames()[jsname].name;
				//sfield = newScriptFieldDecl() // too hard to fathom, I'll break it out:
				sfield = MALLOC(struct ScriptFieldDecl *, sizeof(struct ScriptFieldDecl));
				bzero(sfield,sizeof(struct ScriptFieldDecl));
				sfield->fieldDecl = newFieldDecl(pkwmode,type,0,jsname,0); //not using a lexer
				memcpy(&sfield->value,&defaultValue,sizeof(union anyVrml));
				sfield->valueSet = valueSet; //=(mod!=PKW_initializeOnly);
				sfield->eventInSet = FALSE; //flag used for directOutput
				vector_pushBack(struct ScriptFieldDecl*, shader->fields, sfield);
				//value = &sfield->value;
			}
		}
	}
	pushField(ud,cname); //strdup(fname)); //strong recommendation
	pushMode(ud,PARSING_FIELD);
}
/* simple sanity check, and change mode */
void parseProtoInterface (void *ud, char **atts) {
	if (getMode(ud,TOP) != PARSING_PROTODECLARE && getMode(ud,TOP) != PARSING_EXTERNPROTODECLARE) {
		ConsoleMessage ("got a <ProtoInterface>, but not within a <ProtoDeclare>\n");
	}
	//setParserMode(PARSING_PROTOINTERFACE);
	pushMode(ud,PARSING_PROTOINTERFACE);
}
void Parser_scanStringValueToMem_B(union anyVrml* any, indexT ctype, char *value, int isXML);

void parseExternProtoDeclare_B (void *ud, char **atts) {
	/*	1.create a new proto but not registered node
		2.get user type name from atts
		3.set flag for shallow/declare
		4.add to current context's externProtoDeclare array
		5.push on node stack awaiting interface (with no initial values)
	*/
	int i;
	char *type_name, *appinfo, *documentation, *containerfield, *url;
	struct ProtoDefinition* obj;
	struct X3D_Proto* proto;
	struct X3D_Proto* parent;
	type_name = appinfo = documentation = containerfield = url = NULL;
	if(0) printf("in parseExternProtoDeclare_B\n");

	proto = createNewX3DNode0(NODE_Proto);
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseProtoDeclare: field:%s=%s\n", atts[i], atts[i+1]);
		#endif

		if (!strcmp("name",atts[i]) ) type_name = atts[i+1];
		else if(!strcmp("containerField",atts[i])) containerfield = atts[i+1];
		else if(!strcmp("appInfo",atts[i])) appinfo = atts[i+1];
		else if(!strcmp("documentation",atts[i])) documentation = atts[i+1];
		else if(!strcmp("url",atts[i])) url = atts[i+1];
	}

	parent = (struct X3D_Proto*)getContext(ud,TOP);
	obj=newProtoDefinition();

	/* did we find the name? */
	if (type_name) {
		obj->protoName = STRDUP(type_name);
	} else {
		printf ("warning - have proto but no name, so just copying a default string in\n");
		obj->protoName = STRDUP("noProtoNameDefined");
	}
	type_name = obj->protoName;

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
	proto->__typename = STRDUP(obj->protoName);
	if(containerfield){
		int builtinField = findFieldInFIELDNAMES(containerfield);
		if(builtinField > -1){
			proto->_defaultContainer = builtinField;
		}
	}
	if(url){
		Parser_scanStringValueToMem_B((union anyVrml*)&proto->url, FIELDTYPE_MFString,url, TRUE);
	}
	proto->__loadstatus = 0; //= LOAD_INITIAL_STATE
	pushMode(ud,PARSING_EXTERNPROTODECLARE);
	pushNode(ud,X3D_NODE(proto));
	pushField(ud,"__children");

}
void parseProtoDeclare_B (void *ud, char **atts) {
	/*	1.create a new proto but not registered node
		2.get user type name from atts
		3.set flag for shallow/declare
		4.add to current context's protoDeclare array
		5.push on node stack awaiting interface and body
	*/
	int i;
	struct X3D_Proto* proto;
	char *type_name, *appinfo, *documentation, *containerfield;
	struct ProtoDefinition* obj;
	struct X3D_Proto* parent;

	type_name = appinfo = documentation = containerfield = NULL;
	if(0) printf("in start protoDeclare\n");

	proto = createNewX3DNode0(NODE_Proto);
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseProtoDeclare: field:%s=%s\n", atts[i], atts[i+1]);
		#endif

		if (!strcmp("name",atts[i]) ) type_name = atts[i+1];
		else if(!strcmp("containerField",atts[i])) containerfield = atts[i+1];
		else if(!strcmp("appInfo",atts[i])) appinfo = atts[i+1];
		else if(!strcmp("documentation",atts[i])) documentation = atts[i+1];
	}

	parent = (struct X3D_Proto*)getContext(ud,TOP);
	obj=newProtoDefinition();

	/* did we find the name? */
	if (type_name) {
		obj->protoName = STRDUP(type_name);
	} else {
		printf ("warning - have proto but no name, so just copying a default string in\n");
		obj->protoName = STRDUP("noProtoNameDefined");
	}
	type_name = obj->protoName;

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
	proto->__typename = STRDUP(obj->protoName);
	if(containerfield){
		int builtinField = findFieldInFIELDNAMES(containerfield);
		if(builtinField > -1){
			proto->_defaultContainer = builtinField;
		}
	}

	pushMode(ud,PARSING_PROTODECLARE);
	pushNode(ud,X3D_NODE(proto));
	pushField(ud,"__children");
}
void parseProtoBody_B (void *ud, char **atts) {
	//push proto node on context stack
	pushContext(ud,getNode(ud,TOP));
	pushMode(ud,PARSING_PROTOBODY);
}
void broto_store_DEF(struct X3D_Proto* proto,struct X3D_Node* node, char *name);
struct X3D_Proto *brotoInstance(struct X3D_Proto* proto, BOOL ideep);
void add_node_to_broto_context(struct X3D_Proto *context,struct X3D_Node *node);
void linkNodeIn_B(void *ud);
struct X3D_Node *broto_search_DEFname(struct X3D_Proto *context, char *name);
void parseProtoInstance_B(void *ud, char **atts) {
	/*broto version
		1. lookup the user (proto) type in current and parent context protoDeclare and externProtoDeclare tables
		2. brotoInstance()
		3. parse att and any <fieldValue> and IS
		4. on end, deep_copy_broto_body2 applying the initial field values parsed.
	*/
	int i, isUSE;
	int nameIndex;
	//int containerIndex;
	//int containerField;
	int defNameIndex;
	//int protoTableIndex;
	char *protoname;
	struct X3D_Proto *currentContext;
	struct X3D_Node *node = NULL;
	char pflagdepth;
	struct X3D_Node *fromDEFtable;


	/* initialization */
	nameIndex = INT_ID_UNDEFINED;
	//containerIndex = INT_ID_UNDEFINED;
	//containerField = INT_ID_UNDEFINED;
	defNameIndex = INT_ID_UNDEFINED;
	//protoTableIndex = 0;
	if(0) printf("parseProtoInstance\n");
	isUSE = FALSE;
	for (i = 0; atts[i]; i += 2) {
		if (strcmp("name",atts[i]) == 0) {
			nameIndex=i+1;
		} else if (strcmp("containerField",atts[i]) == 0) {
			//containerIndex = i+1;
		} else if (strcmp("DEF",atts[i]) == 0) {
			defNameIndex = i+1;
		} else if (strcmp("class",atts[i]) == 0) {
			ConsoleMessage ("field \"class\" not currently used in a ProtoInstance parse... sorry");
		} else if (strcmp("USE",atts[i]) == 0) {
			//ConsoleMessage ("field \"USE\" not currently used in a ProtoInstance parse.. sorry");
			isUSE = TRUE;
			defNameIndex = i+1;
		}
	}

	currentContext = getContext(ud,TOP);

	pflagdepth = ciflag_get(currentContext->__protoFlags,0); //depth 0 we are deep inside protodeclare, depth 1 we are instancing live scenery

	/* did we find the name? */
	protoname = NULL;
	if (nameIndex != INT_ID_UNDEFINED) {
		protoname = atts[nameIndex];
	} else {
		ConsoleMessage ("\"ProtoInstance\" found, but field \"name\" not found!\n");
	}


	if(protoname){
		if(isUSE){
			//ConsoleMessage ("field \"USE\" not currently used in a ProtoInstance parse.. sorry");
			char * defname = atts[defNameIndex]; //gets STRDUP();'d inside broto_store_DEF

			fromDEFtable = broto_search_DEFname(currentContext,defname);
			if (!fromDEFtable) {
				ConsoleMessage ("Warning - line %d DEF name: \'%s\' not found",LINE,atts[i+1]);
				ConsoleMessage("\n");
			} else {
				#ifdef X3DPARSERVERBOSE
				printf ("copying for field %s defName %s\n",atts[i], atts[i+1]);
				#endif

				/* if (fromDEFtable->_nodeType != fromDEFtable->_nodeType) { */
				if (NODE_Proto != fromDEFtable->_nodeType) {
					ConsoleMessage ("Warning, line %d DEF/USE mismatch, '%s', %s != %s", LINE,
						atts[i+1],stringNodeType(fromDEFtable->_nodeType), stringNodeType (NODE_Proto));
				} else {
					/* Q. should thisNode.referenceCount be decremented or ??? */
					char* containerfield;
					node = fromDEFtable;
					node->referenceCount++; //dug9 added but should???
					//getNode(ud,TOP) = thisNode; 
					#ifdef X3DPARSERVERBOSE
					printf ("successful copying for field %s defName %s\n",atts[i], atts[i+1]);
					#endif
					pushNode(ud,node);
					containerfield = NULL;
					for (i = 0; atts[i]; i += 2) {
						if(!strcmp(atts[i],"containerField")) containerfield = atts[i+1];
					}
					if(containerfield) {
						int builtinField = findFieldInFIELDNAMES(containerfield);
						if(builtinField > INT_ID_UNDEFINED){
							node->_defaultContainer = builtinField;
						}
					}
					pushField(ud,NULL); //no particular default field
					pushMode(ud,PARSING_PROTOINSTANCE_USE);
					return;
				}
			}
		}else{
			struct X3D_Proto *proto;
			if( isAvailableBroto(protoname, currentContext , &proto))
			{
				//struct X3D_Node *parent;
				char* containerfield;
				/* its a binary proto, new in 2013 */
				int idepth = 0; //if its old brotos (2013) don't do depth until sceneInstance. If 2014 broto2, don't do depth here if we're in a protoDeclare or externProtoDeclare
				idepth = pflagdepth == 1; //2014 broto2: if we're parsing a scene (or Inline) then deepcopy proto to instance it, else shallow
				node=X3D_NODE(brotoInstance(proto,idepth));
				node->_executionContext = X3D_NODE(proto);
				if (defNameIndex != INT_ID_UNDEFINED){
					char * defname = atts[defNameIndex]; //gets STRDUP();'d inside broto_store_DEF
					broto_store_DEF(currentContext,node, defname);
				}
				add_node_to_broto_context(currentContext,node);

				pushNode(ud,node);
				containerfield = NULL;
				for (i = 0; atts[i]; i += 2) {
					if(!strcmp(atts[i],"containerField")) containerfield = atts[i+1];
				}
				if(containerfield) {
					int builtinField = findFieldInFIELDNAMES(containerfield);
					if(builtinField > INT_ID_UNDEFINED){
						node->_defaultContainer = builtinField;
					}
				}
				//linkNodeIn_B(ud);
				//parseAttributes_B(ud,atts); //PI uses FieldValue
			}else{
				pushNode(ud,NULL);
				ConsoleMessage ("Attempt to instance undefined prototype typename %s\n",protoname);
			}
		}
	}
	pushField(ud,NULL); //no particular default field
	pushMode(ud,PARSING_PROTOINSTANCE);

}

BOOL nodeTypeSupportsUserFields(struct X3D_Node *node);
int getFieldFromNodeAndName(struct X3D_Node* node,const char *fieldname, int *type, int *kind, int *iifield, union anyVrml **value);
void broto_store_IS(struct X3D_Proto *proto,char *protofieldname,int pmode, int iprotofield, int type,
					struct X3D_Node *node, char* nodefieldname, int mode, int ifield, int source);
void parseConnect_B(void *ud, char **atts) {
	int i,okp, okn;
	struct X3D_Node *node;
	struct X3D_Proto *context, *proto;
	char *nodefield, *protofield;
	node = getNode(ud,TOP);
	proto = context = getContext(ud,TOP);

	nodefield = protofield = NULL;
	for(i=0;atts[i];i+=2){
		if(!strcmp(atts[i],"nodeField")) nodefield = atts[i+1];
		if(!strcmp(atts[i],"protoField")) protofield = atts[i+1];
	}
	okp = okn = 0;
	if(nodefield && protofield){
		int ptype, pkind, pifield, ntype, nkind, nifield;
		union anyVrml *pvalue, *nvalue;
		okp = getFieldFromNodeAndName(X3D_NODE(proto),protofield,&ptype, &pkind, &pifield, &pvalue);
		okn = getFieldFromNodeAndName(node, nodefield,&ntype, &nkind, &nifield, &nvalue);
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
		if(okp && okn)
		if(ntype != ptype){
			ConsoleMessage("Parser error: IS - we have a name match: %s IS %s found protofield %s\n",
				nodefield,protofield,protofield);
			ConsoleMessage("...But the types don't match: nodefield %s protofield %s\n",
				FIELDTYPES[ntype],FIELDTYPES[ptype]);
			okp = 0;
		}
		if(okp && okn)
		if(nkind != PKW_inputOutput && nkind != pkind){
			if(pkind != PKW_inputOutput){
				ConsoleMessage("Parser Error: IS - we have a name match: %s IS %s found protofield %s\n",
					nodefield,protofield,protofield);
				ConsoleMessage("...But the modes don't jive: nodefield %s protofield %s\n",
					PROTOKEYWORDS[nkind],PROTOKEYWORDS[pkind]);
				okp = 0;
			}else{
				ConsoleMessage("Parser Warning: IS - we have a name match: %s IS %s found protofield %s\n",
					nodefield,protofield,protofield);
				ConsoleMessage("...But the modes don't jive: nodefield %s protofield %s\n",
					PROTOKEYWORDS[nkind],PROTOKEYWORDS[pkind]);
				ConsoleMessage("...will thunk\n");
			}
		}
		if(okp && okn){
			int source;
			//we have an IS that's compatible/jives
			//a) copy the value if it's an initializeOnly or inputOutput
			if(pkind == PKW_initializeOnly || pkind == PKW_inputOutput)
			{
				shallow_copy_field(ntype, pvalue , nvalue);
			}
			//b) register it in the IS-table for our context
			source = node->_nodeType == NODE_Proto ? 3 : node->_nodeType == NODE_Script ? 1 : nodeTypeSupportsUserFields(node) ? 2 : 0;
			//Q. do I need to convert builtin from field index to offset? if( source == 0) nifield *=5;
			broto_store_IS(context,protofield,pkind,pifield,ptype,
							node,nodefield,nkind,nifield,source);
		}
	}
}

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
	

	myNodeIndex = findFieldInNODES(name);

	/* is this a "normal" node that can be found in x3d, x3dv and wrl files? */
	if (myNodeIndex != INT_ID_UNDEFINED) {
		startBuiltin_B(ud,myNodeIndex,(const xmlChar *)name,myAtts);
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
				parseProtoDeclare_B(ud,myAtts);
				break;
			case X3DSP_ExternProtoDeclare: 
				parseExternProtoDeclare_B(ud,myAtts);
				break;
			case X3DSP_ProtoBody: 
				parseProtoBody_B(ud,myAtts); 
				break;
			case X3DSP_ProtoInterface: 
				parseProtoInterface(ud,myAtts); 
				break;
			case X3DSP_ProtoInstance: 
				parseProtoInstance_B(ud,myAtts); 
				break;
			case X3DSP_ROUTE: 
				parseRoutes_B(ud,myAtts);
				break;
			case X3DSP_meta: parseMeta(myAtts); break;
			case X3DSP_Scene: parseScene(myAtts); break;
			case X3DSP_head:
			case X3DSP_Header: parseHeader(myAtts); break;
			case X3DSP_X3D: parseX3Dhead(myAtts); break;
			case X3DSP_fieldValue:  
				parseFieldValue_B(ud,myAtts);
				break;
			case X3DSP_field: 
				parseScriptProtoField_B (ud, myAtts);
				break;
			case X3DSP_IS: parseIS(ud); break;
			case X3DSP_component: parseComponent(myAtts); break;
			case X3DSP_EXPORT: 
				parseExport_B(ud,myAtts);
				break;
			case X3DSP_IMPORT: 
				parseImport_B(ud,myAtts);
				break;
			case X3DSP_connect: 
				parseConnect_B(ud,myAtts);
				break;

			default: printf ("	huh? startElement, X3DSPECIAL, but not handled?? %d, :%s:\n",myNodeIndex,X3DSPECIAL[myNodeIndex]);
		}
		return;
	}

	printf ("startElement name  do not currently handle this one :%s: index %d\n",name,myNodeIndex); 
}

void scriptFieldDecl_jsFieldInit(struct ScriptFieldDecl* me, int num);
void endScriptProtoField_B(void *ud) {
	if(0) printf("end scriptprotofield\n");
	popField(ud);
	popMode(ud); //PARSING_FIELD);
}


static void XMLCALL X3DendElement(void *ud, const xmlChar *iname) {
	int myNodeIndex;
    const char*name = (const char*) iname;
	//ttglobal tg = gglobal();
	//ppX3DParser p = (ppX3DParser)tg->X3DParser.prv;

	/* printf ("X3DEndElement for %s\n",name); */

	#ifdef X3DPARSERVERBOSE
	printf ("endElement: %s : parentIndex %d mode %s\n",name,parentIndex,parserModeStrings[getMode(ud,TOP)]); 
	#endif

		

	myNodeIndex = findFieldInNODES(name);
	if (myNodeIndex != INT_ID_UNDEFINED) {
		endBuiltin_B(ud,iname);
		return;

	}

		

	/* no, it is not. Lets see what else it could be... */
	myNodeIndex = findFieldInX3DSPECIAL(name);
	if (myNodeIndex != INT_ID_UNDEFINED) {
		switch (myNodeIndex) {
			case X3DSP_ProtoInstance: 
				endProtoInstance_B(ud,name);
				break;
			case X3DSP_ProtoInterface: 
				endProtoInterfaceTag(ud); 
				break;
			case X3DSP_ProtoBody: 
				endProtoBodyTag_B(ud,name);
				break;
			case X3DSP_ProtoDeclare: 
				endProtoDeclareTag_B(ud);
				break;
			case X3DSP_ExternProtoDeclare: 
				endExternProtoDeclareTag_B(ud);
				break;
			case X3DSP_IS: 
				endIS(ud); 
				break;
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
				endScriptProtoField_B(ud);
				break;
			case X3DSP_fieldValue:
				endFieldValue_B(ud);
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
		//freeProtoMemory ();
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


	DEBUG_X3DPARSER ("X3DPARSE on :\n%s:\n",inputstring);
	if(p->user_data == NULL){
		//just once. We are re-entrant when parsing text protos (trotos)
		// and want to keep the stack for the parent scene
		p->user_data = new_xml_user_data();
	}
	pushContext(p->user_data,ectx);
	if(myParent->_nodeType == NODE_Proto )
		pushField(p->user_data,"__children");
	else
		pushField(p->user_data,"children");
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
		popField(p->user_data);
		shutdownX3DParser(p->user_data);
		Parser_deleteParserForScanStringValueToMem();
		return FALSE;
	}
	popField(p->user_data);
	shutdownX3DParser(p->user_data);
	Parser_deleteParserForScanStringValueToMem();
	return TRUE;
}
