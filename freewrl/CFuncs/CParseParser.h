/* Parser (input of non-terminal symbols) for CParse */

#ifndef CPARSEPARSER_H
#define CPARSEPARSER_H

#include <stddef.h>

#include "headers.h"
#include "Vector.h"

#include "CParseGeneral.h"
#include "CParseLexer.h"

/* This is the DEF/USE memory. */
extern struct Vector* DEFedNodes;

/* This is our parser-object. */
struct VRMLParser
{
 struct VRMLLexer* lexer;	/* The lexer used. */
 /* Where to put the parsed nodes? */
 void* ptr;
 unsigned ofs;
};

/* Constructor and destructor */
struct VRMLParser* newParser(void*, unsigned);
void deleteParser(struct VRMLParser*);

/* Sets parser's input */
#define parser_fromString(me, str) \
 lexer_fromString(me->lexer, str)

/* Parses MF* field values */
BOOL parser_mfboolValue(struct VRMLParser*, struct Multi_Bool*);
BOOL parser_mfcolorValue(struct VRMLParser*, struct Multi_Color*);
BOOL parser_mfcolorrgbaValue(struct VRMLParser*, struct Multi_ColorRGBA*);
BOOL parser_mffloatValue(struct VRMLParser*, struct Multi_Float*);
BOOL parser_mfint32Value(struct VRMLParser*, struct Multi_Int32*);
BOOL parser_mfnodeValue(struct VRMLParser*, struct Multi_Node*);
BOOL parser_mfrotationValue(struct VRMLParser*, struct Multi_Rotation*);
BOOL parser_mfstringValue(struct VRMLParser*, struct Multi_String*);
BOOL parser_mftimeValue(struct VRMLParser*, struct Multi_Time*);
BOOL parser_mfvec2fValue(struct VRMLParser*, struct Multi_Vec2f*);
BOOL parser_mfvec3fValue(struct VRMLParser*, struct Multi_Vec3f*);

/* Parses SF* field values */
BOOL parser_sfboolValue(struct VRMLParser*, vrmlBoolT*);
BOOL parser_sfcolorValue(struct VRMLParser*, vrmlColorT*);
BOOL parser_sfcolorrgbaValue(struct VRMLParser*, vrmlColorRGBAT*);
#define parser_sffloatValue(me, ret) \
 lexer_float(me->lexer, ret)
#define parser_sfimageValue(me, ret) \
 lexer_image(me->lexer, ret)
#define parser_sfint32Value(me, ret) \
 lexer_int32(me->lexer, ret)
BOOL parser_sfnodeValue(struct VRMLParser*, vrmlNodeT*);
BOOL parser_sfrotationValue(struct VRMLParser*, vrmlRotationT*);
#define parser_sfstringValue(me, ret) \
 lexer_string(me->lexer, ret)
BOOL parser_sftimeValue(struct VRMLParser*, vrmlTimeT*);
BOOL parser_sfvec2fValue(struct VRMLParser*, vrmlVec2fT*);
#define parser_sfvec3fValue(me, ret) \
 parser_sfcolorValue(me, ret)

/* Parses nodes, fields and other statements. */
BOOL parser_routeStatement(struct VRMLParser*);
BOOL parser_nodeStatement(struct VRMLParser*, vrmlNodeT*);
BOOL parser_node(struct VRMLParser*, vrmlNodeT*);
BOOL parser_field(struct VRMLParser*, struct X3D_Node*);

/* Main parsing routine, parses the start symbol (vrmlScene) */
BOOL parser_vrmlScene(struct VRMLParser*);

#endif /* Once-check */
