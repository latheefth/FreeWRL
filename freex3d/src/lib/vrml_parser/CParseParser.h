/* 
=INSERT_TEMPLATE_HERE=

$Id$

Parser (input of non-terminal symbols) for CParse

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


#ifndef __FREEWRL_CPARSE_PARSER_H__
#define __FREEWRL_CPARSE_PARSER_H__

void resetParseSuccessfullyFlag(void);
int parsedSuccessfully(void);

// JAS - OLDCODE #ifdef REWIRE
// JAS - OLDCODE #define BOOL int
// JAS - OLDCODE #include "../../libeai/EAI_C.h"
// JAS - OLDCODE #endif

struct ProtoDefinition;
struct ProtoFieldDecl;
struct Shader_Script;
struct OffsetPointer;


#define BLOCK_STATEMENT(LOCATION) \
   if(parser_routeStatement(me))  { \
	continue; \
   } \
 \
  if (parser_componentStatement(me)) { \
	continue; \
  } \
 \
  if (parser_exportStatement(me)) { \
	continue; \
  } \
 \
  if (parser_importStatement(me)) { \
	continue; \
  } \
 \
  if (parser_metaStatement(me)) { \
	continue; \
  } \
 \
  if (parser_profileStatement(me)) { \
	continue; \
  } 

/* This is our parser-object. */
struct VRMLParser
{
 struct VRMLLexer* lexer;	/* The lexer used. */
 /* Where to put the parsed nodes? */
 void* ptr;
 unsigned ofs;
 /* Currently parsing a PROTO? */
 struct ProtoDefinition* curPROTO;

 /* This is the DEF/USE memory. */
 Stack* DEFedNodes;

 /* This is for PROTOs -- not stacked, as explained in CParseLexer.h */
 struct Vector* PROTOs;

	/* which format some field strings will be in - XML and "classic" VRML are different */
	int parsingX3DfromXML;
 Stack* brotoDEFedNodes;
};

/* Functions parsing a type by its index */
extern BOOL (*PARSE_TYPE[])(struct VRMLParser*, void*);

/* Constructor and destructor */
struct VRMLParser* newParser(void*, unsigned, int isX3DFormat);
struct VRMLParser* reuseParser(void*, unsigned);
void deleteParser(struct VRMLParser*);

/* Other clean up */
void parser_destroyData(struct VRMLParser*);

/* Scoping */
void parser_scopeIn(struct VRMLParser*);
void parser_scopeOut(struct VRMLParser*);

/* Sets parser's input */
#define parser_fromString(me, str) \
 lexer_fromString(me->lexer, str)

/* Parses SF* field values */
#define parser_sffloatValue(me, ret) \
 lexer_float(me->lexer, ret)
#define parser_sfint32Value(me, ret) \
 lexer_int32(me->lexer, ret)
#define parser_sfstringValue(me, ret) \
 lexer_string(me->lexer, ret)
#define lexer_sfstringValue(me, ret) \
 lexer_string(me, ret)

/* Initializes node-specific fields */
void parser_specificInitNode(struct X3D_Node*, struct VRMLParser*);

/* Registers a ROUTE, in current PROTO or scene */
void parser_registerRoute(struct VRMLParser*,
 struct X3D_Node*, int, struct X3D_Node*, int, int);

BOOL parseType(struct VRMLParser* me, int type,   union anyVrml *defaultVal);


void replaceProtoField(struct VRMLLexer *me, struct ProtoDefinition *thisProto, char *thisID, char **outTextPtr, size_t *outSize);

void cParseErrorCurID(struct VRMLParser *me, char *str);
void cParseErrorFieldString(struct VRMLParser *me, char *str1, const char *str2);

#define CPARSE_ERROR_CURID(str) cParseErrorCurID(me, str);
#define CPARSE_ERROR_FIELDSTRING(str1,str2) cParseErrorFieldString(me, str1, str2);

/* Main parsing routine, parses the start symbol (vrmlScene) */
BOOL parser_vrmlScene(struct VRMLParser*);

BOOL nodeTypeSupportsUserFields(struct X3D_Node *node);
int PKW_from_KW(int KW_index);
BOOL find_anyfield_by_nameAndRouteDir(struct VRMLLexer* lexer, struct X3D_Node* node, union anyVrml **anyptr, 
	int *imode, int *itype, char* nodeFieldName, int *isource, void** fdecl, int *ifield, int PKW_eventType);
BOOL found_IS_field(struct VRMLParser* me, struct X3D_Node *node);
BOOL isAvailableBroto(char *pname, struct X3D_Proto* currentContext, struct X3D_Proto **proto);
void registerParentIfManagedField(int type, int mode, int isPublic, union anyVrml* any, struct X3D_Node* parent);
void shallow_copy_field(int typeIndex, union anyVrml* source, union anyVrml* dest);
BOOL usingBrotos();

#endif /* __FREEWRL_CPARSE_PARSER_H__ */
