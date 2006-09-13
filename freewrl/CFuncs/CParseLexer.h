/* Lexer (input of terminal symbols) for CParse */

#ifndef CPARSELEXER_H
#define CPARSELEXER_H

#include "headers.h"
#include "Vector.h"

#include "CParseGeneral.h"

/* Tables of user-defined IDs */
extern Stack* userNodeNames;
extern Stack* userNodeTypes;
extern Stack* user_field;
extern Stack* user_exposedField;
extern Stack* user_eventIn;
extern Stack* user_eventOut;

/* Undefined ID (for special "class", like builtIn and exposed) */
#define ID_UNDEFINED	((indexT)-1)

/* This is our lexer-object. */
struct VRMLLexer
{
 const char* nextIn;	/* Next input character. */
 char* curID;	/* Currently input but not lexed id. */
 BOOL isEof;	/* Error because of EOF? */
};

/* Constructor and destructor */
struct VRMLLexer* newLexer();
void deleteLexer(struct VRMLLexer*);

/* Other clean up. */
void lexer_destroyData();
void lexer_destroyIdStack(Stack*);

/* Set input */
#define lexer_fromString(me, str) \
 ((me)->isEof=FALSE, (me)->nextIn=str)

/* Is EOF? */
#define lexer_eof(me) \
 ((me)->isEof && !(me)->curID)

/* indexT -> char* conversion */
#define lexer_stringUFieldName(index, type) \
 vector_get(char*, stack_top(user_##type), index)
#define lexer_stringUser_field(index) \
 lexer_stringUFieldName(index, field)
#define lexer_stringUser_exposedField(index) \
 lexer_stringUFieldName(index, exposedField)
#define lexer_stringUser_eventIn(index) \
 lexer_stringUFieldName(index, eventIn)
#define lexer_stringUser_eventOut(index) \
 lexer_stringUFieldName(index, eventOut)

/* Skip whitespace and comments. */
void lexer_skip(struct VRMLLexer*);

/* Ensures that curID is set. */
BOOL lexer_setCurID(struct VRMLLexer*);

/* Some operations with IDs */
void lexer_scopeIn();
void lexer_scopeOut();
BOOL lexer_keyword(struct VRMLLexer*, indexT);
BOOL lexer_specialID(struct VRMLLexer*, indexT* retB, indexT* retU,
 const char**, const indexT, Stack*);
BOOL lexer_specialID_string(struct VRMLLexer*, indexT* retB, indexT* retU,
 const char**, const indexT, Stack*,
 const char*);
BOOL lexer_defineID(struct VRMLLexer*, indexT*, Stack**);
#define lexer_defineNodeName(me, ret) \
 lexer_defineID(me, ret, &userNodeNames)
#define lexer_defineNodeType(me, ret) \
 lexer_defineID(me, ret, &userNodeTypes)
#define lexer_define_field(me, ret) \
 lexer_defineID(me, ret, &user_field)
#define lexer_define_exposedField(me, ret) \
 lexer_defineID(me, ret, &user_exposedField)
#define lexer_define_eventIn(me, ret) \
 lexer_defineID(me, ret, &user_eventIn)
#define lexer_define_eventOut(me, ret) \
 lexer_defineID(me, ret, &user_eventOut)
BOOL lexer_field(struct VRMLLexer*, indexT*, indexT*, indexT*, indexT*);
BOOL lexer_eventIn(struct VRMLLexer*, indexT*, indexT*, indexT*, indexT*);
BOOL lexer_eventOut(struct VRMLLexer*, indexT*, indexT*, indexT*, indexT*);
#define lexer_node(me, r1, r2) \
 lexer_specialID(me, r1, r2, NODES, NODES_COUNT, userNodeTypes)
#define lexer_nodeName(me, ret) \
 lexer_specialID(me, NULL, ret, NULL, 0, userNodeNames)
#define lexer_protoFieldMode(me, r) \
 lexer_specialID(me, r, NULL, PROTOKEYWORDS, PROTOKEYWORDS_COUNT, NULL)
#define lexer_fieldType(me, r) \
 lexer_specialID(me, r, NULL, FIELDTYPES, FIELDTYPES_COUNT, NULL)
indexT lexer_string2id(const char*, const Stack*);
#define lexer_nodeName2id(str) \
 lexer_string2id(str, userNodeNames)

/* Input the basic literals */
BOOL lexer_int32(struct VRMLLexer*, vrmlInt32T*);
BOOL lexer_float(struct VRMLLexer*, vrmlFloatT*);
BOOL lexer_string(struct VRMLLexer*, vrmlStringT*);

/* Checks for the five operators of VRML */
BOOL lexer_operator(struct VRMLLexer*, char);
#define lexer_point(me) \
 lexer_operator(me, '.')
#define lexer_openCurly(me) \
 lexer_operator(me, '{')
#define lexer_closeCurly(me) \
 lexer_operator(me, '}')
#define lexer_openSquare(me) \
 lexer_operator(me, '[')
#define lexer_closeSquare(me) \
 lexer_operator(me, ']')

#endif /* Once-check */
