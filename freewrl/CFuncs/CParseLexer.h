/* Lexer (input of terminal symbols) for CParse */

#ifndef CPARSELEXER_H
#define CPARSELEXER_H

#include "headers.h"
#include "Vector.h"

#include "CParseGeneral.h"

/* Tables of user-defined IDs:
 * userNodeNames (DEFs) is scoped with a simple stack, as every PROTO has its
   scope completely *different* from the rest of the world.
 * userNodeTypes (PROTO definitions) needs to be available up through the whole
   stack, values are stored in a vector, and the indices where each stack level
   ends are stored in a stack.
 * fields are not scoped and therefore stored in a simple vector.
 */
extern Stack* userNodeNames;
extern struct Vector* userNodeTypesVec;
extern Stack* userNodeTypesStack;
extern struct Vector* user_field;
extern struct Vector* user_exposedField;
extern struct Vector* user_eventIn;
extern struct Vector* user_eventOut;

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

/* Count of elements to pop off the PROTO vector for scope-out */
#define lexer_getProtoPopCnt() \
 (vector_size(userNodeTypesVec)-stack_top(size_t, userNodeTypesStack))

/* Set input */
#define lexer_fromString(me, str) \
 ((me)->isEof=FALSE, (me)->nextIn=str)

/* Is EOF? */
#define lexer_eof(me) \
 ((me)->isEof && !(me)->curID)

/* indexT -> char* conversion */
#define lexer_stringUFieldName(index, type) \
 vector_get(char*, user_##type, index)
#define lexer_stringUser_field(index) \
 lexer_stringUFieldName(index, field)
#define lexer_stringUser_exposedField(index) \
 lexer_stringUFieldName(index, exposedField)
#define lexer_stringUser_eventIn(index) \
 lexer_stringUFieldName(index, eventIn)
#define lexer_stringUser_eventOut(index) \
 lexer_stringUFieldName(index, eventOut)
/* User field name -> char*, takes care of access mode */
const char* lexer_stringUser_fieldName(indexT name, indexT mode);

/* Skip whitespace and comments. */
void lexer_skip(struct VRMLLexer*);

/* Ensures that curID is set. */
BOOL lexer_setCurID(struct VRMLLexer*);

/* Some operations with IDs */
void lexer_scopeIn();
void lexer_scopeOut();
void lexer_scopeOut_PROTO();
BOOL lexer_keyword(struct VRMLLexer*, indexT);
BOOL lexer_specialID(struct VRMLLexer*, indexT* retB, indexT* retU,
 const char**, const indexT, struct Vector*);
BOOL lexer_specialID_string(struct VRMLLexer*, indexT* retB, indexT* retU,
 const char**, const indexT, struct Vector*, const char*);
BOOL lexer_defineID(struct VRMLLexer*, indexT*, struct Vector*, BOOL);
#define lexer_defineNodeName(me, ret) \
 lexer_defineID(me, ret, stack_top(struct Vector*, userNodeNames), TRUE)
#define lexer_defineNodeType(me, ret) \
 lexer_defineID(me, ret, userNodeTypesVec, FALSE)
#define lexer_define_field(me, ret) \
 lexer_defineID(me, ret, user_field, TRUE)
#define lexer_define_exposedField(me, ret) \
 lexer_defineID(me, ret, user_exposedField, TRUE)
#define lexer_define_eventIn(me, ret) \
 lexer_defineID(me, ret, user_eventIn, TRUE)
#define lexer_define_eventOut(me, ret) \
 lexer_defineID(me, ret, user_eventOut, TRUE)
BOOL lexer_field(struct VRMLLexer*, indexT*, indexT*, indexT*, indexT*);
BOOL lexer_event(struct VRMLLexer*, struct X3D_Node*,
 indexT*, indexT*, indexT*, indexT*,
 const char** arr, size_t arrCnt, int routeToFrom);
#define lexer_eventIn(me, node, a, b, c, d) \
 lexer_event(me, node, a, b, c, d, EVENT_IN, EVENT_IN_COUNT, 1)
#define lexer_eventOut(me, node, a, b, c, d) \
 lexer_event(me, node, a, b, c, d, EVENT_OUT, EVENT_OUT_COUNT, 0)
#define lexer_node(me, r1, r2) \
 lexer_specialID(me, r1, r2, NODES, NODES_COUNT, userNodeTypesVec)
#define lexer_nodeName(me, ret) \
 lexer_specialID(me, NULL, ret, NULL, 0, \
  stack_top(struct Vector*, userNodeNames))
#define lexer_protoFieldMode(me, r) \
 lexer_specialID(me, r, NULL, PROTOKEYWORDS, PROTOKEYWORDS_COUNT, NULL)
#define lexer_fieldType(me, r) \
 lexer_specialID(me, r, NULL, FIELDTYPES, FIELDTYPES_COUNT, NULL)
indexT lexer_string2id(const char*, const struct Vector*);
#define lexer_nodeName2id(str) \
 lexer_string2id(str, stack_top(struct Vector*, userNodeNames))

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
