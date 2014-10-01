/*


X3D parser functions.

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


#ifndef __FREEWRL_X3D_PARSER_H__
#define __FREEWRL_X3D_PARSER_H__

/* attributes get put into this structure */
struct nameValuePairs {
        char *fieldName;
        char *fieldValue;
		int fieldType; //0 string 1 itoa(DEF index) 10 FIELDTYPE_SFNode sprintf %p anyVrml* 11 FIELDTYPE_MFNode sprintf %p anyVrml*
};


/* header file for the X3D parser, only items common between the X3DParser files should be here. */

/*#define X3DPARSERVERBOSE 1*/
#define PARSING_NODES 1
#define PARSING_SCRIPT 2
#define PARSING_PROTODECLARE  3
#define PARSING_PROTOINTERFACE  4
#define PARSING_PROTOBODY       5
#define PARSING_PROTOINSTANCE   6
#define PARSING_IS		7
#define PARSING_CONNECT		8
#define PARSING_EXTERNPROTODECLARE 9
#define PARSING_FIELD 10

/* for our internal PROTO tables, and, for initializing the XML parser */
#define PROTOINSTANCE_MAX_LEVELS 50
#define PROTOINSTANCE_MAX_PARAMS 20

#define FREEWRL_SPECIFIC "FrEEWrL_pRotto"

#ifndef VERBOSE
#define DECREMENT_PARENTINDEX \
        if (gglobal()->X3DParser.parentIndex > 0) { gglobal()->X3DParser.parentIndex--; } else { ConsoleMessage ("X3DParser, line %d stack underflow (source code %s:%d)",LINE,__FILE__,__LINE__); }

#define INCREMENT_PARENTINDEX_hide \
        if (gglobal()->X3DParser.parentIndex < (PARENTSTACKSIZE-2))  { \
                gglobal()->X3DParser.parentIndex++; \
                gglobal()->X3DParser.parentStack[gglobal()->X3DParser.parentIndex] = NULL; /* make sure we know the state of the new Top of Stack */ \
        } else ConsoleMessage ("X3DParser, line %d stack overflow",LINE);
#else
#define DECREMENT_PARENTINDEX \
        if (gglobal()->X3DParser.parentIndex > 0) { \
		gglobal()->X3DParser.parentIndex--; \
		printf("Decrementing parentIndex to %d %s %d\n",gglobal()->X3DParser.parentIndex,__FILE__,__LINE__); \
	} else { \
		ConsoleMessage ("X3DParser, line %d stack underflow (source code %s:%d)",LINE,__FILE__,__LINE__); \
	}

#define INCREMENT_PARENTINDEX \
        if (gglobal()->X3DParser.parentIndex < (PARENTSTACKSIZE-2))  { \
                gglobal()->X3DParser.parentIndex++; \
				printf("Incrementing parentIndex to %d %s %d\n",gglobal()->X3DParser.parentIndex,__FILE__,__LINE__); \
                gglobal()->X3DParser.parentStack[gglobal()->X3DParser.parentIndex] = NULL; /* make sure we know the state of the new Top of Stack */ \
        } else ConsoleMessage ("X3DParser, line %d stack overflow",LINE);
#endif

int freewrl_XML_GetCurrentLineNumber();
#define LINE freewrl_XML_GetCurrentLineNumber()
#define TTY_SPACE {int tty; printf ("%3d ",gglobal()->X3DParser.parentIndex); for (tty = 0; tty < gglobal()->X3DParser.parentIndex; tty++) printf ("  ");}
//extern int getParserMode(void);
//extern void debugsetParserMode(int,char*, int);
int getParserMode(void);
void debugpushParserMode(int newmode, char *fle, int line);
void debugpopParserMode(char *fle, int line);
#define pushParserMode(xxx) debugpushParserMode(xxx,__FILE__,__LINE__)
#define popParserMode() debugpopParserMode(__FILE__,__LINE__)

#define PARENTSTACKSIZE 256
//extern int parentIndex;
//int gglobal()->X3DParser.parentIndex;
//int setParentIndex(int newParentIndex);

//extern struct X3D_Node *parentStack[PARENTSTACKSIZE];
//extern char *CDATA_Text;
//extern int CDATA_Text_curlen;


/* function protos */
struct X3D_Node *DEFNameIndex (const char *name, struct X3D_Node* node, int force);

void parseProtoDeclare (void *ud, char **atts);
void parseProtoDeclare_B (void *ud, char **atts);
void parseExternProtoDeclare (void *ud, char **atts);
void parseExternProtoDeclare_B (void *ud, char **atts);
void parseProtoInterface (void *ud, char **atts);
void parseProtoBody (void *ud, char **atts);
void parseProtoBody_B (void *ud, char **atts);
void parseProtoInstance (void *ud, char **atts);
void parseProtoInstance_B (void *ud, char **atts);
void parseProtoInstanceFields(void *ud, const char *name, char **atts);
void dumpProtoBody (const char *name, char **atts);
void dumpCDATAtoProtoBody (char *str);
void parseScriptProtoField(void *ud, struct VRMLLexer *, char **atts);
void parseScriptProtoField_B(void *ud, char **atts);
void expandProtoInstance(void *ud, struct VRMLLexer *, struct X3D_Group * myGroup);
void freeProtoMemory (void);
void kill_X3DProtoScripts(void);
void linkNodeIn(void *ud, char *, int);
void parseConnect(void *ud, struct VRMLLexer * myLexer, char **atts, struct Vector *tos);
void endConnect(void *ud);
void endProtoDeclare(void *ud);
void endExternProtoDeclare(void *ud);
struct X3D_Node *X3DParser_getNodeFromName(const char *name);
int getRoutingInfo (struct VRMLLexer *myLexer, struct X3D_Node *node, int *offs, int* type, int *accessType, struct Shader_Script **myObj, char *name, int routeTo);
 
char *X3DParser_getNameFromNode(struct X3D_Node* myNode);


struct xml_user_data;
//for push,pop,get the index is the vector index range 0, n-1. 
// Or going from the top top= -1, parent to top = -2.
#define TOP -1
#define BOTTOM 0
void pushContext(void *userData, struct X3D_Node* context);
void pushNode(void *userData,struct X3D_Node* node);
void pushMode(void *userData, int parsingmode);
struct X3D_Proto* getContext(void *userData, int index);
struct X3D_Node* getNode(void *userData, int index);
void* getAtt(void *userData, int index);
void setAtt(void *userData, int index, void* att);
int getNodeTop(ud);
int getMode(void *userData, int index);
void popContext(void *userData);
void popNode(void *userData);
void popMode(void *userData);
void pushField(void *userData, char* fname);
void popField(void *userData);
char* getField(void *userData, int index);
#endif /*  __FREEWRL_X3D_PARSER_H__ */
