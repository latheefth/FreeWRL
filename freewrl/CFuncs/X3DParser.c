/*******************************************************************
 Copyright (C) 2006 John Stewart, CRC Canada
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*
 * $Id$
 *
 */


#include "jsapi.h"
#include "jsUtils.h"
#include "headers.h"
#include "X3DParser.h"

/* If XMLCALL isn't defined, use empty one */
#ifndef XMLCALL
 #define XMLCALL
#endif /* XMLCALL */

static int inCDATA = FALSE;
char *scriptText = NULL;
int scriptTextMallocSize = 0;


/* this ifdef sequence is kept around, for a possible Microsoft Vista port */
#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

int parentIndex = -1;
struct X3D_Node *parentStack[PARENTSTACKSIZE];

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
		"unused high"};
		
int parserMode = PARSING_NODES;

/* DEF/USE table  for X3D parser */
struct DEFnameStruct {
        struct X3D_Node *node;
        struct Uni_String *name;
};

struct DEFnameStruct *DEFnames = 0;
int DEFtableSize = ID_UNDEFINED;
int MAXDEFNames = 0;

/* XML parser variables */
static int X3DParserRecurseLevel = ID_UNDEFINED;
static XML_Parser x3dparser[PROTOINSTANCE_MAX_LEVELS];
static XML_Parser currentX3DParser = NULL;

/* get the line number of the current parser for error purposes */
int freewrl_XML_GetCurrentLineNumber(void) {
	if (currentX3DParser != NULL) 
		return XML_GetCurrentLineNumber(currentX3DParser);
	return ID_UNDEFINED;
}


/* "forget" the DEFs. Keep the table around, though, as the entries will simply be used again. */
void kill_X3DDefs(void) {
	DEFtableSize = ID_UNDEFINED;
}

/* return a node assoicated with this name. If the name exists, return the previous node. If not, return
the new node */
static struct X3D_Node *DEFNameIndex (const char *name, struct X3D_Node* node, int force) {
	unsigned len;
	int ctr;
	struct Uni_String *tmp;

	
	/* printf ("start of DEFNameIndex, name %s, type %s\n",name,stringNodeType(node->_nodeType)); */


	len = strlen(name) + 1; /* length includes null termination, in newASCIIString() */

	/* is this a duplicate name and type? types have to be same,
	   name lengths have to be the same, and the strings have to be the same.
	*/
	for (ctr=0; ctr<=DEFtableSize; ctr++) {
		tmp = DEFnames[ctr].name;
		if (strcmp(name,tmp->strptr)==0) {
			/* do we really want to change this one if it is already found? */
			if (force) {
				/* printf ("DEFNameIndex, rewriting node :%s: at index %d\n",tmp->strptr,ctr); */
				DEFnames[ctr].node = node;
			}
			return DEFnames[ctr].node;
		}
	}

	/* nope, not duplicate */

	DEFtableSize ++;

	/* ok, we got a name and a type */
	if (DEFtableSize >= MAXDEFNames) {
		/* oooh! not enough room at the table */
		MAXDEFNames += 100; /* arbitrary number */
		DEFnames = (struct DEFnameStruct*)REALLOC (DEFnames, sizeof(*DEFnames) * MAXDEFNames);
	}

	DEFnames[DEFtableSize].name = newASCIIString((char *)name);
	DEFnames[DEFtableSize].node = node;
	return node;
}


static int getRouteField (struct X3D_Node *node, int *offs, int* type, char *name, int dir) {
	int error = FALSE;
	int fieldInt;
	int ctmp;
 
	if (node->_nodeType == NODE_Script) {
		error = !(getFieldFromScript (name,X3D_SCRIPT(node)->_X3DScript,offs,type));
	} else {

		/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
		fieldInt = findRoutedFieldInFIELDNAMES (node, name, dir);
		if (fieldInt >=0) findFieldInOFFSETS(NODE_OFFSETS[node->_nodeType], 
				fieldInt, offs, type, &ctmp);
	}
	if (*offs <0) {
		ConsoleMessage ("ROUTE: line %d Field %s not found in node type %s",LINE,
			name,stringNodeType(node->_nodeType));
		error = TRUE;
	}
	return error;
}


/******************************************************************************************/
/* parse a ROUTE statement. Should be like:
	<ROUTE fromField="fraction_changed"  fromNode="TIME0" toField="set_fraction" toNode="COL_INTERP"/>
*/

static void parseRoutes (const char **atts) {
	struct X3D_Node *fromNode = NULL;
	struct X3D_Node *toNode = NULL;	
	int fromOffset = ID_UNDEFINED;
	int toOffset = ID_UNDEFINED;
	int i;
	int error = FALSE;

	int fromType;
	int toType;
	int scriptDiri = 0;

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

	/* get the direction correct */
	if (fromNode->_nodeType == NODE_Script) scriptDiri += FROM_SCRIPT;
	if (toNode->_nodeType == NODE_Script) scriptDiri += TO_SCRIPT;
	
	/* second pass - get the fields of the nodes */
	for (i = 0; atts[i]; i += 2) {
		if (strcmp("fromField",atts[i])==0) {
			error = getRouteField(fromNode, &fromOffset, &fromType, (char *)atts[i+1],0);
		} else if (strcmp("toField",atts[i]) ==0) {
			error = getRouteField(toNode, &toOffset, &toType, (char *)atts[i+1],1);
		}
	}	

	/* get out of here if an error is found */
	if (error) return;


	/* is there a script here? if so, now change the script NODE pointer to a Script index */
	if (fromNode->_nodeType == NODE_Script) fromNode = ((struct X3D_Node*) ((struct X3D_Script*)fromNode)->_X3DScript);
	if (toNode->_nodeType == NODE_Script) toNode = ((struct X3D_Node*) ((struct X3D_Script*)toNode)->_X3DScript);

	#ifdef X3DPARSERVERBOSE
	printf ("now routing from a %s to a %s %d %d\n",FIELDTYPES[fromType], FIELDTYPES[toType],fromType,toType);
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
	CRoutes_RegisterSimple(fromNode, fromOffset, toNode, toOffset, returnRoutingElementLength(fromType),scriptDiri);
}

/* parse normal X3D nodes/fields */
static void parseNormalX3D(int myNodeType, const char *name, const char** atts) {
	int i;

	struct X3D_Node *thisNode;
	struct X3D_Node *fromDEFtable;
	int coffset;
	int ctype;
	int ctmp;
	int foffset;

	/* semantic check */
	if ((parserMode != PARSING_NODES) && (parserMode != PARSING_PROTOBODY)) {
		ConsoleMessage("parseNormalX3D: expected parserMode to be PARSING_NODES, got %s",
					parserModeStrings[parserMode]);
	}

	switch (myNodeType) {
        	case NODE_Script: parserMode = PARSING_SCRIPT; break;
        	default: {}
        }


	/* create this to be a new node */	
	thisNode = createNewX3DNode(myNodeType);
	parentStack[parentIndex] = thisNode; 

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("parseNormalX3D: for name %s, myNodeType = %d is %u parentInded %d\n",name,myNodeType,thisNode,parentIndex);
	#endif

	if (thisNode->_nodeType == NODE_Script) {
		#ifdef X3DPARSERVERBOSE
		printf ("working through script parentIndex %d\n",parentIndex);
		#endif

		((struct X3D_Script *)thisNode)->_X3DScript = (int) nextScriptHandle();
		JSInit(((struct X3D_Script *)thisNode)->_X3DScript);
	}

	/* go through the fields, and link them in. SFNode and MFNodes will be handled 
	 differently - these are usually the result of a different level of parsing,
	 and the "containerField" value */
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		if (parserMode == PARSING_SCRIPT) {
			printf ("parsing script decl; have %s %s\n",atts[i], atts[i+1]);
		}
		#endif


		if (strcmp ("DEF",atts[i]) == 0) {
			#ifdef X3DPARSERVERBOSE
			printf ("this is a DEF, name %s\n",atts[i+1]);
			#endif

			fromDEFtable = DEFNameIndex ((char *)atts[i+1],thisNode, TRUE);
			if (fromDEFtable != thisNode) {
				#ifdef X3DPARSERVERBOSE
				printf ("Warning - line %d duplicate DEF name: \'%s\'\n",LINE,atts[i+1]);
				#endif
				printf ("Warning - line %d duplicate DEF name: \'%s\'\n",LINE,atts[i+1]);
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

				if (fromDEFtable->_nodeType != fromDEFtable->_nodeType) {
					ConsoleMessage ("Warning, line %d DEF/USE mismatch, '%s', %s != %s", LINE,
						atts[i+1],stringNodeType(fromDEFtable->_nodeType), stringNodeType (thisNode->_nodeType));
				} else {
					thisNode = fromDEFtable;
					parentStack[parentIndex] = thisNode; 
					#ifdef X3DPARSERVERBOSE
					printf ("successful copying for field %s defName %s\n",atts[i], atts[i+1]);
					#endif

				}
			}
		} else if (strcmp("containerField",atts[i])==0) {
			indexT tmp;
			/* printf ("SETTING CONTAINER FIELD TO %s for node of type %s\n",(char *)atts[i+1], stringNodeType(thisNode->_nodeType )); */
			tmp = findFieldInFIELDNAMES((char *)atts[i+1]);
			if (tmp == ID_UNDEFINED) {
				ConsoleMessage ("Error setting containerField to :%s: for node of type :%s:\n",
					(char *)atts[i+1], stringNodeType(thisNode->_nodeType ));
			} else {
				thisNode->_defaultContainer = tmp;
			}
		} else {
			setField_fromJavascript (thisNode, (char *)atts[i],(char *)atts[i+1]);
		}
	}
}


static void XMLCALL startCDATA (void *userData) {
	#ifdef X3DPARSERVERBOSE
	printf ("startCDATA -parentIndex %d parserMode %s\n",parentIndex,parserModeStrings[parserMode]);
	#endif
	inCDATA = TRUE;
}

static void XMLCALL endCDATA (void *userData) {
	#ifdef X3DPARSERVERBOSE
	printf ("endCDATA -parentIndex %d parserMode %s\n",parentIndex,parserModeStrings[parserMode]);
	#endif
	inCDATA = FALSE;


	if (parserMode == PARSING_PROTOBODY) {
		dumpCDATAtoProtoBody (scriptText);
	} else {
		/* check sanity for top of stack This should be a Script node */
		if (parentStack[parentIndex]->_nodeType != NODE_Script) {
			ConsoleMessage ("endCDATA, line %d, expected the parent to be a Script node",LINE);
			return;
		}
	}
	
	#ifdef X3DPARSERVERBOSE
        printf ("returning from EndCData\n");
        #endif  

}

static void XMLCALL handleCDATA (void *userData, const char *string, int len) {
	char mydata[4096];
	char firstTime;
	if (inCDATA) {
		/* do we need to set this string larger? */
		if (len > scriptTextMallocSize-10) {
			firstTime = (scriptTextMallocSize == 0);
			scriptTextMallocSize +=4096;
			if (firstTime) {
				scriptText = MALLOC (scriptTextMallocSize);
				scriptText[0] = '\0';
			} else {
				scriptText = REALLOC (scriptText,scriptTextMallocSize);
			}
		}
		memcpy (mydata, string,len);
		mydata[len] = '\0';
		strcat (scriptText,mydata);
/*
		printf ("CDATA full text %s\n",scriptText);
*/
	
	}
}

/* parse a export statement, and send the results along */
static void parseImport(const char **atts) {
	int i;
        char *nodeToImport = NULL;
        char *alias = NULL;

        for (i = 0; atts[i]; i += 2) {
		printf("import field:%s=%s\n", atts[i], atts[i + 1]);
	}
/* do nothing right now */
return;
}


/* parse a export statement, and send the results along */
static void parseExport(const char **atts) {
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
static void parseComponent(const char **atts) {
	int i;
	int myComponent = ID_UNDEFINED;
	int myLevel = ID_UNDEFINED;

	/* go through the fields and make sense of them */
        for (i = 0; atts[i]; i += 2) {
		/* printf("components field:%s=%s\n", atts[i], atts[i + 1]);  */
		if (strcmp("level",atts[i]) == 0) {
			if (sscanf(atts[i+1],"%d",&myLevel) != 1) {
				ConsoleMessage ("expected Component level for component %s, got %s",atts[i], atts[i+1]);
				return;
			}
		} else if (strcmp("name",atts[i]) == 0) {
			myComponent = findFieldInCOMPONENTS(atts[i+1]);
			if (myComponent == ID_UNDEFINED) {
				ConsoleMessage("Component statement, but component name not valid :%s:",atts[i+1]);
				return;
			}

		} else {
			ConsoleMessage ("Unknown fields in Component statement :%s: :%s:",atts[i], atts[i+1]);
		}
	}

	if (myComponent == ID_UNDEFINED) {
		ConsoleMessage("Component statement, but component name not stated");
	} else if (myLevel == ID_UNDEFINED) {
		ConsoleMessage("Component statement, but component level not stated");
	} else {
		handleComponent(myComponent,myLevel);
	}
}

static void parseMeta(const char **atts) {}
static void parseHeader(const char **atts) {}
static void parseScene(const char **atts) {
}
static void parseX3Dhead(const char **atts) {
}
static void parseIS() {
	if (parserMode != PARSING_PROTOBODY) {
		ConsoleMessage ("endProtoInterfaceTag: got a <IS> but not a ProtoBody at %d",LINE);
	}
}

static void endProtoInterfaceTag() {
	if (parserMode != PARSING_PROTOINTERFACE) {
		ConsoleMessage ("endProtoInterfaceTag: got a </ProtoInterface> but not parsing one at line %d",LINE);
	}
	/* now, a ProtoInterface should be within a ProtoDeclare, so, make the expected mode PARSING_PROTODECLARE */
	parserMode = PARSING_PROTODECLARE;
}

static void endProtoBodyTag(char *name) {
	/* ending <ProtoBody> */
	if (parserMode != PARSING_PROTOBODY) {
		ConsoleMessage ("endProtoBodyTag: got a </ProtoBody> but not parsing one at line %d",LINE);
	}

	endDumpProtoBody(name);

	/* now, a ProtoBody should be within a ProtoDeclare, so, make the expected mode PARSING_PROTODECLARE */
	parserMode = PARSING_PROTODECLARE;
}

static void endProtoDeclareTag() {
	/* ending <ProtoDeclare> */

	if (parserMode != PARSING_PROTODECLARE) {
		ConsoleMessage ("endProtoDeclareTag: got a </ProtoDeclare> but not parsing one at line %d",LINE);
		parserMode = PARSING_PROTODECLARE;
	}

	endProtoDeclare();
}

static void endProtoInstanceTag() {
	struct X3D_Group *protoExpGroup = NULL;

	/* ending <ProtoInstance> */
	#ifdef X3DPARSERVERBOSE
	printf ("endProtoInstanceTag, goot ProtoInstance got to find it, and expand it.\n");
	#endif

	if (parserMode != PARSING_PROTOINSTANCE) {
		ConsoleMessage ("endProtoInstanceTag: got a </ProtoInstance> but not parsing one at line %d",LINE);
	}

	/* we should just be assuming that we are parsing regular nodes for the scene graph now */
	parserMode = PARSING_NODES;

	protoExpGroup = (struct X3D_Group *) createNewX3DNode(NODE_Transform);
		#ifdef X3DPARSERVERBOSE
		if (protoExpGroup != NULL) {
			printf ("\nOK, linking in this proto. I'm %d, ps-1 is %d, and p %d\n",protoExpGroup,parentStack[parentIndex-1], parentStack[parentIndex]);
		}
		#endif

	expandProtoInstance(protoExpGroup);
}

static void endFieldTag() {
	/* is this possibly a field name, that we do not expect? */
	if (parserMode == PARSING_NODES) {
		ConsoleMessage ("X3DParser: line %d Got a <field> but not parsing Scripts nor PROTOS",LINE);
		printf ("Got a <field> but not parsing Scripts nor PROTOS\n");
	}
}

static void endFieldValueTag() {
	if (parserMode != PARSING_PROTOINSTANCE) {
		ConsoleMessage ("X3DParser: line %d Got a <fieldValue> but not in a <ProtoInstance>",LINE);
		printf ("Got a <fieldValue> but not in a <ProtoInstance>\n");
	}
} 

void linkNodeIn() {
	int coffset;
	int ctype;
	int ctmp;
	uintptr_t *destnode;
	char *memptr;

	/* did we have a valid node here? Things like ProtoDeclares are NOT valid nodes, and we can ignore them,
	   because there will be no code associated with them */
	
	/* bounds check */
	if (parentIndex < 1) {
		ConsoleMessage ("linkNodeIn: stack underflow");
		return;
	}

	if ((parentStack[parentIndex] == NULL) || (parentStack[parentIndex-1] == NULL)) {
		ConsoleMessage ("linkNodeIn: NULL found in stack");
		return;
	}

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("linkNodeIn: parserMode %s parentIndex %d, ",
			parserModeStrings[parserMode],parentIndex);
	printf ("linking in %s to %s, field %s (%d)\n",
		stringNodeType(parentStack[parentIndex]->_nodeType),
		stringNodeType(parentStack[parentIndex-1]->_nodeType),
		stringFieldType(parentStack[parentIndex]->_defaultContainer),
		parentStack[parentIndex]->_defaultContainer);
	#endif

	/* Link it in; the parent containerField should exist, and should be an SF or MFNode  */
	findFieldInOFFSETS(NODE_OFFSETS[parentStack[parentIndex-1]->_nodeType], 
		parentStack[parentIndex]->_defaultContainer, &coffset, &ctype, &ctmp);

	if (coffset <= 0) {
		printf ("X3DParser - trouble finding field %s in node %s\n",
			stringFieldType(parentStack[parentIndex]->_defaultContainer),
			stringNodeType(parentStack[parentIndex-1]->_nodeType));
	}

	if ((ctype != FIELDTYPE_MFNode) && (ctype != FIELDTYPE_SFNode)) {
		ConsoleMessage ("X3DParser: warning, line %d: trouble linking to containerField %s of parent node type %s (in a %s node)", LINE,
			stringFieldType(parentStack[parentIndex]->_defaultContainer),
			stringNodeType(parentStack[parentIndex-1]->_nodeType),
			stringNodeType(parentStack[parentIndex]->_nodeType));
		return;
	}
	memptr = (char *)parentStack[parentIndex-1] + coffset;
	if (ctype == FIELDTYPE_SFNode) {
		/* copy over a single memory pointer */
		destnode = (uintptr_t *) memptr;
		*destnode = parentStack[parentIndex];
		ADD_PARENT(X3D_NODE(parentStack[parentIndex]), X3D_NODE(parentStack[parentIndex-1]));
	} else {
		AddRemoveChildren (
			parentStack[parentIndex-1], /* parent */
			(struct Multi_Node *) memptr,			/* where the children field is */
			((uintptr_t *) &(parentStack[parentIndex])),	/* this child, 1 node */
                1, 1);

	}
}

static void XMLCALL startElement(void *unused, const char *name, const char **atts) {
	int i;
	int myNodeIndex;

	#ifdef X3DPARSERVERBOSE
	printf ("startElement: %s : level %d\n",name,parentIndex);
        for (i = 0; atts[i]; i += 2) {
		printf("	field:%s=%s\n", atts[i], atts[i + 1]);
	}
	#endif

	/* are we storing a PROTO body?? */
	if (parserMode == PARSING_PROTOBODY) {
		dumpProtoBody(name,atts);
		return;
	}

	myNodeIndex = findFieldInNODES(name);

	/* is this a "normal" node that can be found in x3d, x3dv and wrl files? */
	if (myNodeIndex != ID_UNDEFINED) {
		INCREMENT_PARENTINDEX 
		parseNormalX3D(myNodeIndex,name, atts); 
		return;
	}

	/* no, it is not. Lets see what else it could be... */
	myNodeIndex = findFieldInX3DSPECIAL(name);
	if (myNodeIndex != ID_UNDEFINED) {
		switch (myNodeIndex) {
			case X3DSP_ProtoDeclare: parseProtoDeclare(atts); break;
			case X3DSP_ProtoBody: parseProtoBody(atts); break;
			case X3DSP_ProtoInterface: parseProtoInterface(atts); break;
			case X3DSP_ProtoInstance: parseProtoInstance(atts); break;
			case X3DSP_ROUTE: parseRoutes(atts); break;
			case X3DSP_meta: parseMeta(atts); break;
			case X3DSP_Scene: parseScene(atts); break;
			case X3DSP_head:
			case X3DSP_Header: parseHeader(atts); break;
			case X3DSP_X3D: parseX3Dhead(atts); break;
			case X3DSP_fieldValue: parseProtoInstanceFields (name, atts); break;
			case X3DSP_field: parseScriptProtoField (atts); break;
			case X3DSP_IS: parseIS(); break;
			case X3DSP_component: parseComponent(atts); break;
			case X3DSP_export: parseExport(atts); break;
			case X3DSP_import: parseImport(atts); break;
			
			/* should never do this: */
			default: printf ("huh? startElement, X3DSPECIAL, but not handled?? %d, :%s:\n",myNodeIndex,X3DSPECIAL[myNodeIndex]);
		}
		return;
	}

	printf ("startElement name  do not currently handle this one :%s: index %d\n",name,myNodeIndex); 
}

static void XMLCALL endElement(void *unused, const char *name) {
	int i;

	int coffset;
	int ctype;
	int ctmp;
	uintptr_t *destnode;
	char *memptr;
	int myNodeIndex;

	#ifdef X3DPARSERVERBOSE
	printf ("endElement: %s : parentIndex %d\n",name,parentIndex);
	#endif

	/* are we storing a PROTO body?? */
	if (parserMode == PARSING_PROTOBODY) {
		/* are we finished with this ProtoBody? */
		if (strcmp("ProtoBody",name)==0) {
			parserMode == PARSING_NODES;
		} else {
			addToProtoCode(name);
			return;
		}
	}
	myNodeIndex = findFieldInNODES(name);
	if (myNodeIndex != ID_UNDEFINED) {
		switch (myNodeIndex) {
			case NODE_Script: initScriptWithScript(); break;
			default: linkNodeIn();
		}

		DECREMENT_PARENTINDEX
		return;
	}

	/* no, it is not. Lets see what else it could be... */
	myNodeIndex = findFieldInX3DSPECIAL(name);
	if (myNodeIndex != ID_UNDEFINED) {
		switch (myNodeIndex) {
			case X3DSP_ProtoInterface: endProtoInterfaceTag(); break;
			case X3DSP_ProtoBody: endProtoBodyTag(name); break;
			case X3DSP_ProtoDeclare: endProtoDeclareTag(); break;
			case X3DSP_ProtoInstance: endProtoInstanceTag(); break;
			case X3DSP_ROUTE: 
			case X3DSP_IS:
			case X3DSP_meta:
			case X3DSP_Scene:
			case X3DSP_head:
			case X3DSP_Header:
			case X3DSP_field:
			case X3DSP_fieldValue:
			case X3DSP_component:
			case X3DSP_X3D: break;
			
			/* should never do this: */
			default: 
			printf ("endElement: huh? X3DSPECIAL, but not handled?? %s\n",X3DSPECIAL[myNodeIndex]);
		}
		/* DECREMENT_PARENTINDEX */
		return;
	}

	printf ("unhandled endElement name %s index %d\n",name,myNodeIndex); 
	/* DECREMENT_PARENTINDEX */
	#ifdef X3DPARSERVERBOSE
	printf ("endElement %s\n",name);
	#endif

	
}

static XML_Parser initializeX3DParser () {
	X3DParserRecurseLevel++;

	if (X3DParserRecurseLevel >= PROTOINSTANCE_MAX_LEVELS) {
		ConsoleMessage ("XML_PARSER init: XML file PROTO nested too deep\n");
		X3DParserRecurseLevel--;
	} else {
		x3dparser[X3DParserRecurseLevel] = XML_ParserCreate(NULL);
		XML_SetElementHandler(x3dparser[X3DParserRecurseLevel], startElement, endElement);
		XML_SetCdataSectionHandler (x3dparser[X3DParserRecurseLevel], startCDATA, endCDATA);
		XML_SetDefaultHandler (x3dparser[X3DParserRecurseLevel],handleCDATA);
		XML_SetUserData(x3dparser[X3DParserRecurseLevel], &parentIndex);
	}

	return x3dparser[X3DParserRecurseLevel];
}

static void shutdownX3DParser () {
	XML_ParserFree(x3dparser[X3DParserRecurseLevel]);
	X3DParserRecurseLevel--;
	
	/* lets free up memory here for possible PROTO variables */
	if (X3DParserRecurseLevel == ID_UNDEFINED) {
		/* if we are at the bottom of the parser call nesting, lets reset parentIndex */
		parentIndex = 0;
		freeProtoMemory ();
	}

	if (X3DParserRecurseLevel < ID_UNDEFINED) {
		ConsoleMessage ("XML_PARSER close underflow");
		X3DParserRecurseLevel = ID_UNDEFINED;
	}
}

int X3DParse (struct X3D_Group* myParent, char *inputstring) {
	currentX3DParser = initializeX3DParser();

	/* printf ("x3dparse ,parent %u\n",myParent); */
	INCREMENT_PARENTINDEX
	parentStack[parentIndex] = X3D_NODE(myParent);
	
	if (XML_Parse(currentX3DParser, inputstring, strlen(inputstring), TRUE) == XML_STATUS_ERROR) {
		fprintf(stderr,
			"%s at line %" XML_FMT_INT_MOD "u\n",
			XML_ErrorString(XML_GetErrorCode(currentX3DParser)),
			XML_GetCurrentLineNumber(currentX3DParser));
		shutdownX3DParser();
		return FALSE;
	}
	shutdownX3DParser();
	return TRUE;
}

