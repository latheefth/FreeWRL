/* Lexer (input of terminal symbols) for CParse */

#ifndef CPARSELEXER_H
#define CPARSELEXER_H

#include "headers.h"
#include "Vector.h"

#include "CParseGeneral.h"

/* Tables of user-defined IDs */
extern struct Vector* userNodeNames;

/* Flag to define a given index is a user-id */
#define USER_ID	0x10000000

/* This is our lexer-object. */
struct VRMLLexer
{
 const char* nextIn;	/* Next input character. */
 char* curID;	/* Currently input but not lexed id. */
};

/* Constructor and destructor */
struct VRMLLexer* newLexer();
void deleteLexer(struct VRMLLexer*);

/* Set input */
#define lexer_fromString(me, str) \
 (me->nextIn=str)

/* Skip whitespace and comments. */
void lexer_skip(struct VRMLLexer*);

/* Ensures that curID is set. */
BOOL lexer_setCurID(struct VRMLLexer*);

/* Some operations with IDs */
BOOL lexer_keyword(struct VRMLLexer*, indexT);
BOOL lexer_specialID(struct VRMLLexer*, indexT*,
 const char**, const indexT, struct Vector**);
#define lexer_node(me, ret) \
 lexer_specialID(me, ret, NODES, NODES_COUNT, NULL)
#define lexer_nodeName(me, ret) \
 lexer_specialID(me, ret, NULL, 0, &userNodeNames)
#define lexer_fieldName(me, ret) \
 lexer_specialID(me, ret, FIELDNAMES, FIELDNAMES_COUNT, NULL)

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
