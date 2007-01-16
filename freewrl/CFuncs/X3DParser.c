#include "jsapi.h"
#include "jsUtils.h"
#include "headers.h"
/*
#include <stdio.h>
#define TRUE 1==1
#define FALSE 1==0
#define MYBUFSIZ 100000
*/
#include "expat.h"

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

#define LINE XML_GetCurrentLineNumber(x3dparser)

static int X3DParser_initialized = FALSE;
static XML_Parser x3dparser;
static int parentIndex = 0;
#define PARENTSTACKSIZE 256
struct X3D_Node *parentStack[PARENTSTACKSIZE];

#define PARSING_NODES 1
#define PARSING_SCRIPT 2
static int parserMode = PARSING_NODES;


/* DEF/USE table  for X3D parser */
/* Script name/type table */
/*struct Uni_String {
        char * strptr;
        int len;
};
*/

struct DEFnameStruct {
        struct X3D_Node *node;
        struct Uni_String *name;
};



struct DEFnameStruct *DEFnames = 0;
int DEFtableSize = -1;
int MAXDEFNames = 0;


struct X3D_Node *DEFNameIndex (char *name, struct X3D_Node* node) {
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
		if ((tmp->len == len) &&
			(strncmp(name,tmp->strptr,len)==0)) {
			return DEFnames[ctr].node;
		}
	}

	/* nope, not duplicate */

	DEFtableSize ++;

	/* ok, we got a name and a type */
	if (DEFtableSize >= MAXDEFNames) {
		/* oooh! not enough room at the table */
		MAXDEFNames += 100; /* arbitrary number */
		DEFnames = (struct DEFnameStruct*)realloc (DEFnames, sizeof(*DEFnames) * MAXDEFNames);
	}

	DEFnames[DEFtableSize].name = newASCIIString(name);
	DEFnames[DEFtableSize].node = node;
	return node;
}



/* parse a ROUTE statement. Should be like:
	<ROUTE fromField="fraction_changed"  fromNode="TIME0" toField="set_fraction" toNode="COL_INTERP"/>
*/

static void parseX3DRoutes (char **atts) {
	struct X3D_Node *fromNode = NULL;
	struct X3D_Node *toNode = NULL;	
	int fromOffset = -1;
	int toOffset = -1;
	int i;
	int error = FALSE;

	int fromType;
	int toType;
	int ctmp;
	int fieldInt;


	#ifdef X3DPARSERVERBOSE
	printf ("\nstart ofrouting\n");	
	#endif

	/* 2 passes - first, find the nodes */
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		printf("ROUTING pass 1 field:%s=%s\n", atts[i], atts[i + 1]);
		#endif

		if (strncmp("fromNode",atts[i],8) == 0) {
			fromNode = DEFNameIndex (atts[i+1], NULL);
			if (fromNode == NULL) {
				ConsoleMessage ("ROUTE statement, line %d fromNode (%s) does not exist",LINE,atts[i+1]);
				error = TRUE;
			}
		} else if (strncmp("toNode",atts[i],6) == 0) {
			toNode = DEFNameIndex (atts[i+1],NULL);
			if (toNode == NULL) {
				ConsoleMessage ("ROUTE statement, line %d toNode (%s) does not exist",LINE,atts[i+1]);
				error = TRUE;
			}
		} else if ((strncmp("fromField",atts[i],9)!=0) &&
				(strncmp("toField",atts[i],7) !=0)) {
			ConsoleMessage ("Field in line %d ROUTE statement not understood: %s",LINE,atts[i]);
			error = TRUE;
		}
	}
	#ifdef X3DPARSERVERBOSE
	printf ("end of pass1, fromNode %d, toNode %d\n",fromNode,toNode);
	#endif
	
	/* second pass - get the fields of the nodes */
	if (!error) {
		for (i = 0; atts[i]; i += 2) {
			if (strncmp("fromField",atts[i],9)==0) {
				/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
				fieldInt = findRoutedFieldInFIELDNAMES (atts[i+1], 0);
				if (fieldInt >=0) findFieldInOFFSETS(NODE_OFFSETS[fromNode->_nodeType], 
						fieldInt, &fromOffset, &fromType, &ctmp);
				if (fromOffset <=0) {
					ConsoleMessage ("ROUTE: line %d fromField %s not found in node type %s",LINE,
						atts[i+1],stringNodeType(fromNode->_nodeType));
					error = TRUE;
				}
			} else if (strncmp("toField",atts[i],7) ==0) {
				fieldInt = findRoutedFieldInFIELDNAMES (atts[i+1], 1);
				if (fieldInt > 0) findFieldInOFFSETS(NODE_OFFSETS[toNode->_nodeType], 
						fieldInt, &toOffset, &toType, &ctmp);
				if (toOffset <=0) {
					ConsoleMessage ("ROUTE: line %d toField  %s not found in node type %s", LINE,
						atts[i+1],stringNodeType(toNode->_nodeType));
					error = TRUE;
				}
			}
		}
	}

	/* are the types the same? */

	#ifdef X3DPARSERVERBOSE
	printf ("routing from a %s to a %s %d %d\n",FIELD_TYPE_STRING(fromType), FIELD_TYPE_STRING(toType),fromType,toType);
	#endif

	if (!error) {
		if (fromType != toType) {
			ConsoleMessage ("Routing type mismatch line %d %s != %s",LINE,stringFieldtypeType(fromType), stringFieldtypeType(toType));
			error = TRUE;
		}
	}


	/* can we register the route? */
	if (!error) {
		CRoutes_RegisterSimple(fromNode, fromOffset, toNode, toOffset, returnRoutingElementLength(fromType),0);
	}
}

static int canWeIgnoreThisNode(char *name) {

	if (strncmp ("Header",name,7) == 0) {return TRUE;}
	if (strncmp ("Metadata",name,8) == 0) {return TRUE;}
	if (strncmp ("Scene",name,5) == 0) {return TRUE;}
	if (strncmp ("meta",name,4) == 0) {return TRUE;}
	if (strncmp ("head",name,4) == 0) {return TRUE;}
	if (strncmp ("X3D",name,3) == 0) {return TRUE;}
return FALSE;
}

/* parse normal X3D nodes/fields */
void parseNormalX3D(char *name, char** atts) {
	int i;

	struct X3D_Node *thisNode;
	struct X3D_Node *fromDEFtable;
	int coffset;
	int ctype;
	int ctmp;
	int foffset;

	int myNodeType;
	/* create this to be a new node */	
	myNodeType = findNodeInNODES(name);

	if (myNodeType != -1) {
		thisNode = createNewX3DNode(myNodeType);
		parentStack[parentIndex] = thisNode; 

		if (thisNode->_nodeType == NODE_Script) {
			#ifdef X3DPARSERVERBOSE
			printf ("working through script parentIndex %d\n",parentIndex);
			#endif
			parserMode = PARSING_SCRIPT;

			((struct X3D_Script *)thisNode)->__scriptObj = nextScriptHandle();
printf ("calling JSInit\n");
			JSInit(((struct X3D_Script *)thisNode)->__scriptObj);
printf ("called JSInit\n");
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


			if (strncmp ("DEF",atts[i],3) == 0) {
				#ifdef X3DPARSERVERBOSE
				printf ("this is a DEF, name %s\n",atts[i+1]);
				#endif

				fromDEFtable = DEFNameIndex (atts[i+1],thisNode);
				if (fromDEFtable != thisNode) {
					ConsoleMessage ("Warning - line %d duplicate DEF name: \'%s\'",LINE,atts[i+1]);
				}

			} else if (strncmp ("USE",atts[i],3) == 0) {
				#ifdef X3DPARSERVERBOSE
				printf ("this is a USE, name %s\n",atts[i+1]);
				#endif

				fromDEFtable = DEFNameIndex (atts[i+1],thisNode);
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
			} else {
				setField_fromJavascript (thisNode, atts[i],atts[i+1]);
			}
		}
	} else if (strncmp(name,"ROUTE",5) == 0) {
		parseX3DRoutes(atts);
	} else {
		ConsoleMessage ("X3D Parser, line %d node type %s not supported by FreeWRL",LINE,name);
		return;
	}


	if (parentIndex < (PARENTSTACKSIZE-2)) 
		parentIndex += 1;
	else ConsoleMessage ("X3DParser, line %d stack overflow",LINE);
	
}

static void parseScriptField(char *name, char **atts) {
	int i;
	uintptr_t myScriptNumber;

	/* check sanity for top of stack This should be a Script node */
	if (parentStack[parentIndex-1]->_nodeType != NODE_Script) {
		ConsoleMessage ("X3DParser, line %d, expected the parent to be a Script node",LINE);
		return;
	}
	
	myScriptNumber = ((struct X3D_Script *)parentStack[parentIndex-1])->__scriptObj;

	printf ("parsing script level %d name %s \n",parentIndex,name);
	for (i = 0; atts[i]; i += 2) {
		printf("	field:%s=%s\n", atts[i], atts[i + 1]);
		if (strncmp(atts[i],"name",4) == 0) {
			printf ("have name\n");
		} else if (strncmp(atts[i],"accessType",10) == 0) {
			printf ("have access type\n");
		} else if (strncmp(atts[i],"type",4) == 0) {
			printf ("have type\n");
		} else {
			ConsoleMessage ("X3D Script parsing line %d- unknown field type %s",LINE,atts[i]);
			return;
		}
	}
}


static int inCDATA = FALSE;
static char *scriptText = NULL;
static int scriptTextMallocSize = 0;

static void XMLCALL startCDATA (void *userData) {
	#ifdef X3DPARSERVERBOSE
	printf ("start CDATA\n");
	#endif
	inCDATA = TRUE;
}

static void XMLCALL endCDATA (void *userData) {
	uintptr_t myScriptNumber;
	char *startingIndex;
	jsval rval;
	#ifdef X3DPARSERVERBOSE
	printf ("EndCData is %s\n",scriptText);
	#endif
	inCDATA = FALSE;

	/* check sanity for top of stack This should be a Script node */
	if (parentStack[parentIndex-1]->_nodeType != NODE_Script) {
		ConsoleMessage ("X3DParser, line %d, expected the parent to be a Script node",LINE);
		return;
	}
	
	myScriptNumber = ((struct X3D_Script *)parentStack[parentIndex-1])->__scriptObj;

	/* peel off the ecmascript etc */
	startingIndex = strstr(scriptText,"ecmascript:");
	if (startingIndex != NULL) {
		startingIndex += strlen ("ecmascript:");
	} else if (startingIndex == NULL) {
		startingIndex = strstr(scriptText,"vrmlscript:");
		if (startingIndex != NULL)
			startingIndex += strlen ("vrmlscript:");
	} else if (startingIndex == NULL) {
		startingIndex = strstr(scriptText,"vrmlscript:");
		if (startingIndex != NULL)
			startingIndex += strlen ("vrmlscript:");
	}

	if (startingIndex == NULL) {
		ConsoleMessage ("X3DParser, line %d have Script node, but no valid script",LINE);
		return;
	}

	if (!ActualrunScript (myScriptNumber, scriptText, &rval)) {
		ConsoleMessage ("X3DParser, script initialization error at line %d",LINE);
		return;
	}
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
				scriptText = malloc (scriptTextMallocSize);
				scriptText[0] = '\0';
			} else {
				scriptText = realloc (scriptText,scriptTextMallocSize);
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

static void XMLCALL startElement(void *unused, const char *name, const char **atts) {
	int i;

	/* is this a node that we can ignore? */
	if (canWeIgnoreThisNode(name)) return;

	#ifdef X3DPARSERVERBOSE
	for (i = 0; i < parentIndex; i++) putchar('\t');
	printf ("Node Name %s ",name);
	for (i = 0; atts[i]; i += 2) printf(" field:%s=%s", atts[i], atts[i + 1]);
	printf ("\n");
	#endif

	/* what is the mode? normal X3D? script parsing?? */
	if (parserMode == PARSING_SCRIPT) parseScriptField (name, atts);
	else parseNormalX3D(name, atts);
}
	
static void XMLCALL endElement(void *unused, const char *name) {
	int i;

	int coffset;
	int ctype;
	int ctmp;
	uintptr_t *destnode;
	char *memptr;

	/* is this a node that we can ignore? */
	if (canWeIgnoreThisNode(name)) return;
	
	/* is this a Script? */
	if (strncmp (name,"field",5) == 0) {
		if (parserMode != PARSING_SCRIPT) {
			ConsoleMessage ("X3DParser: line %d Got a <field> but not parsing Scripts",LINE);
			printf ("Got a <fieldt> but not parsing Scripts\n");
		}
		return;
	}


	/* sanity check here */
	if (parserMode == PARSING_SCRIPT) {
		if (strncmp (name,"Script",6) != 0) 
		ConsoleMessage ("X3DParser line %d endElement, name %s, still PARSING_SCRIPTS",LINE,name);
	} 


	/* is this the end of a Script? */
	if (strncmp(name,"Script",6) == 0) {
		printf ("got END of script - script should be registered\n");
		parserMode = PARSING_NODES;
	}

	
	if (parentIndex > 1) parentIndex -= 1; else ConsoleMessage ("X3DParser, line %d stack underflow",LINE);


	#ifdef X3DPARSERVERBOSE
	for (i = 0; i < parentIndex; i++) putchar('\t');

	printf ("parentIndex %d, stack top %d, stack- %d\n",parentIndex,parentStack[parentIndex],parentStack[parentIndex-1]);
	printf ("ok, linking in %s to %s, field %s (%d)\n",
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
#ifdef xxx
		printf ("is this maybe a PROTO?? if so, it will be a Group node with __protoDef set to the pointer\n");
		if (node->_nodeType == NODE_Group) {
			group = (struct X3D_Group *)node;
			printf ("it IS a group...\n");
			if (group->__protoDef) {
				printf ("and, this is a PROTO...have to go through PROTO defs to get to it\n");
			}
		}
#endif
	}

	if ((ctype != MFNODE) && (ctype != SFNODE)) {
		ConsoleMessage ("X3DParser, line %d trouble linking to field %s, node type %s (this nodeType %s)", LINE,
			stringFieldType(parentStack[parentIndex]->_defaultContainer),
			stringNodeType(parentStack[parentIndex-1]->_nodeType),
			stringNodeType(parentStack[parentIndex]->_nodeType));
		return;
	}
	memptr = (char *)parentStack[parentIndex-1] + coffset;
	if (ctype == SFNODE) {
		/* copy over a single memory pointer */
		destnode = (uintptr_t *) memptr;
		*destnode = parentStack[parentIndex];
	} else {
		AddRemoveChildren (
			parentStack[parentIndex-1], /* parent */
			memptr,			/* where the children field is */
			&(parentStack[parentIndex]),	/* this child, 1 node */
                1, 1);

	}
}

int initializeX3DParser () {
	x3dparser = XML_ParserCreate(NULL);
	XML_SetElementHandler(x3dparser, startElement, endElement);
	XML_SetCdataSectionHandler (x3dparser, startCDATA, endCDATA);
	XML_SetDefaultHandler (x3dparser,handleCDATA);
	XML_SetUserData(x3dparser, &parentIndex);
}

void shutdownX3DParser () {
	if (X3DParser_initialized) {
		X3DParser_initialized = FALSE;
		XML_ParserFree(x3dparser);
	}
}

int X3DParse (struct X3D_Group* myParent, char *inputstring) {
	parentIndex = 0;
	parentStack[parentIndex] = myParent;
	parentIndex ++;
	
	if (XML_Parse(x3dparser, inputstring, strlen(inputstring), TRUE) == XML_STATUS_ERROR) {
		fprintf(stderr,
			"%s at line %" XML_FMT_INT_MOD "u\n",
			XML_ErrorString(XML_GetErrorCode(x3dparser)),
			XML_GetCurrentLineNumber(x3dparser));
		return FALSE;
	}
	if (parentIndex != 1) {
		ConsoleMessage ("X3DParser line %d  - stack issues.... parentIndex %d",LINE,parentIndex);
		return FALSE;
	}
	return TRUE;
}
