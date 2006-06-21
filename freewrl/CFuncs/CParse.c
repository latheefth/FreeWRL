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

void destroyCParserData()
{
 parser_destroyData();
}
