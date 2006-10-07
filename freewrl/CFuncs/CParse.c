/*
Sourcecode for CParse.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "CParse.h"
#include "CParseParser.h"

BOOL cParse(void* ptr, unsigned ofs, const char* data)
{
 struct VRMLParser* parser;
 struct X3D_Node* node;

 parser=newParser(ptr, ofs);
 parser_fromString(parser, data);
 assert(parser->lexer);

 if(!parser_vrmlScene(parser))
  fprintf(stderr, "Parser failed!\n");

 deleteParser(parser);

 return TRUE;
}

/* ************************************************************************** */
/* Accessor methods */

/* Return DEFed node from its name */
struct X3D_Node* parser_getNodeFromName(const char* name)
{
 indexT ind=lexer_nodeName2id(name);
 if(ind==ID_UNDEFINED)
  return NULL;
  
 assert(!stack_empty(DEFedNodes));
 assert(ind<vector_size(stack_top(struct Vector*, DEFedNodes)));
 return vector_get(struct X3D_Node*,
  stack_top(struct Vector*, DEFedNodes), ind);
}
