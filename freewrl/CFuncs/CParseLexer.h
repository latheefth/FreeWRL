/* Lexer (input of terminal symbols) for CParse */

#ifndef CPARSELEXER_H
#define CPARSELEXER_H

#include "headers.h"
#include "Vector.h"

#include "CParseGeneral.h"

/* Tables of user-defined IDs */
extern struct Vector* userNodeNames;

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

/* Set input */
#define lexer_fromString(me, str) \
 ((me)->isEof=FALSE, (me)->nextIn=str)

/* Is EOF? */
#define lexer_eof(me) \
 ((me)->isEof && !(me)->curID)

/* Skip whitespace and comments. */
void lexer_skip(struct VRMLLexer*);

/* Ensures that curID is set. */
BOOL lexer_setCurID(struct VRMLLexer*);

/* Some operations with IDs */
BOOL lexer_keyword(struct VRMLLexer*, indexT);
BOOL lexer_specialID(struct VRMLLexer*, indexT* retB, indexT* retU,
 const char**, const indexT, struct Vector**);
BOOL lexer_specialID_string(struct VRMLLexer*, indexT* retB, indexT* retU,
 const char**, const indexT, struct Vector**,
 const char*);
BOOL lexer_defineID(struct VRMLLexer*, indexT*, struct Vector**);
#define lexer_defineNodeName(me, ret) \
 lexer_defineID(me, ret, &userNodeNames)
BOOL lexer_field(struct VRMLLexer*, indexT*, indexT*, indexT*, indexT*);
BOOL lexer_eventIn(struct VRMLLexer*, indexT*, indexT*, indexT*, indexT*);
BOOL lexer_eventOut(struct VRMLLexer*, indexT*, indexT*, indexT*, indexT*);
#define lexer_node(me, r1, r2) \
 lexer_specialID(me, r1, r2, NODES, NODES_COUNT, NULL)
#define lexer_nodeName(me, ret) \
 lexer_specialID(me, NULL, ret, NULL, 0, &userNodeNames)

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
