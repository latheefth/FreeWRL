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

static int X3DParser_initialized = FALSE;
static XML_Parser x3dparser;
static int parentIndex = 0;
#define PARENTSTACKSIZE 256
struct X3D_Node *parentStack[PARENTSTACKSIZE];

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

	/*
	printf ("start of DEFNameIndex, name %s, type %s\n",name,type);
	printf ("start of DEFNameIndex, lengths name %d, type %d\n",
			strlen(name),strlen(type)); 
	*/


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


static int canWeIgnoreThisNode(char *name) {

	if (strncmp ("Header",name,7) == 0) {return TRUE;}
	if (strncmp ("Scene",name,5) == 0) {return TRUE;}
	if (strncmp ("meta",name,4) == 0) {return TRUE;}
	if (strncmp ("head",name,4) == 0) {return TRUE;}
	if (strncmp ("X3D",name,3) == 0) {return TRUE;}
return FALSE;
}
static void XMLCALL startElement(void *unused, const char *name, const char **atts) {
	int i;
	struct X3D_Node *thisNode;
	struct X3D_Node *fromDEFtable;
	int coffset;
	int ctype;
	int ctmp;
	int foffset;

	int myNodeType;

	/* is this a node that we can ignore? */
	if (canWeIgnoreThisNode(name)) return;

	#ifdef X3DPARSERVERBOSE
	for (i = 0; i < parentIndex; i++) putchar('\t');
	printf ("Node Name %s ",name);
	for (i = 0; atts[i]; i += 2) printf(" field:%s=%s", atts[i], atts[i + 1]);
	printf ("\n");
	#endif

	/* create this to be a new node */	
	myNodeType = findNodeInNODES(name);
	if (myNodeType != -1) {
		thisNode = createNewX3DNode(myNodeType);
		parentStack[parentIndex] = thisNode; 

		/* go through the fields, and link them in. SFNode and MFNodes will be handled 
		 differently - these are usually the result of a different level of parsing,
		 and the "containerField" value */
		for (i = 0; atts[i]; i += 2) {
			if (strncmp ("DEF",atts[i],3) == 0) {
				#ifdef X3DPARSERVERBOSE
				printf ("this is a DEF, name %s\n",atts[i+1]);
				#endif

				fromDEFtable = DEFNameIndex (atts[i+1],thisNode);
				if (fromDEFtable != thisNode) {
					ConsoleMessage ("Warning - duplicate DEF name: \'%s\'",name);
				}

			} else if (strncmp ("USE",atts[i],3) == 0) {
				#ifdef X3DPARSERVERBOSE
				printf ("this is a USE, name %s\n",atts[i+1]);
				#endif

				fromDEFtable = DEFNameIndex (atts[i+1],thisNode);
				if (fromDEFtable == thisNode) {
					ConsoleMessage ("Warning - DEF name: \'%s\' not found",atts[i+1]);
				} else {
					#ifdef X3DPARSERVERBOSE
					printf ("copying for field %s defName %s\n",atts[i], atts[i+1]);
					#endif

					if (fromDEFtable->_nodeType != fromDEFtable->_nodeType) {
						ConsoleMessage ("Warning, DEF/USE mismatch, '%s', %s != %s",
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
	} else {
		ConsoleMessage ("X3D Parser, node type %s not supported by FreeWRL",name);
		return;
	}

	if (parentIndex < (PARENTSTACKSIZE-2)) 
		parentIndex += 1;
	else ConsoleMessage ("X3DParser, stack overflow");
	
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
	if (parentIndex > 1) parentIndex -= 1; else ConsoleMessage ("X3DParser, stack underflow\n");


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
		ConsoleMessage ("X3DParser, trouble linking to field %s, node type %s (this nodeType %s)",
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
		ConsoleMessage ("X3DParser - stack issues.... parentIndex %d",parentIndex);
		return FALSE;
	}
	return TRUE;
}
