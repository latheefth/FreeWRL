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

 parser=newParser();
 parser_fromString(parser, data);
 assert(parser->lexer);

 if(!parser_node(parser, &node))
  fprintf(stderr, "Parser failed!\n");

 deleteParser(parser);

 addToNode(ptr, ofs, node);

 return TRUE;
}
