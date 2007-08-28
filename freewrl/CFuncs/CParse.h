/* VRML-parsing routines in C. */

#ifndef CPARSE_H
#define CPARSE_H

#include "headers.h"

BOOL cParse(void*, unsigned, const char*);

/* Destroy all data associated with the currently parsed world kept. */
#define destroyCParserData(me) \
 parser_destroyData(me)

/* Some accessor-methods */
struct X3D_Node* parser_getNodeFromName(const char*);
extern struct VRMLParser* globalParser;

#endif /* Once-check */
