/*******************************************************************
  Copyright (C) 2007 Daniel Kraft,  John Stewart, CRC Canada.
  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
  See the GNU Library General Public License (file COPYING in the distribution)
  for conditions of use and redistribution.
 *********************************************************************/


/*
Sourcecode for CParse.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "CParse.h"
#include "CParseParser.h"
 
/* Keep a pointer to the parser for the main URL */
struct VRMLParser* globalParser = NULL;

BOOL cParse(void* ptr, unsigned ofs, const char* data)
{
 struct VRMLParser* parser;

 if (!globalParser) {
	/* printf ("cParse, new parser\n"); */
 	parser=newParser(ptr, ofs);
	globalParser = parser;
 } else {
	/* printf ("cParse, using old parser\n"); */
	parser=reuseParser(ptr,ofs);
 }

 parser_fromString(parser, data);
 ASSERT(parser->lexer);

 if(!parser_vrmlScene(parser))
  fprintf(stderr, "Parser failed!\n");

 /* printf ("after parsing in cParse, VRMLParser->DEFinedNodes %u\n",parser->DEFedNodes); */
 /* deleteParser(parser); */

  /* this data is a copy, so we can delete it here */
  FREE_IF_NZ (parser->lexer->startOfStringPtr);

 return TRUE;
}

/* ************************************************************************** */
/* Accessor methods */

/* Return DEFed node from its name */
struct X3D_Node* parser_getNodeFromName(const char* name)
{
 indexT ind=lexer_nodeName2id(globalParser->lexer, name);
 if(ind==ID_UNDEFINED)
  return NULL;

 ASSERT(!stack_empty(globalParser->DEFedNodes));
 ASSERT(ind<vector_size(stack_top(struct Vector*, globalParser->DEFedNodes)));
 return vector_get(struct X3D_Node*,
  stack_top(struct Vector*, globalParser->DEFedNodes), ind);
}


void fw_assert (char *file, int line) {
	int looper;
	printf ("FreeWRL Internal Error at line %d in %s\n",line,file);
	ConsoleMessage ("FreeWRL Internal Error at line %d in %s",line,file);
	
	for (looper=1; looper<60; looper++) {
		sleep(1);
		sched_yield();
	}
	printf ("FreeWRL exiting...\n");
	exit(1);
}

