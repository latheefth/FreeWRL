/*******************************************************************
 Copyright (C) 2006 John Stewart, CRC Canada
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/************************************************************************

EAIHelpers.c  - small routines to help with interfacing EAI to Daniel Kraft's
parser.

NOTE - originally, when perl parsing was used, there used to be a perl
NodeNumber, and a memory location. Now, there is only a memory location...

************************************************************************/

#include "headers.h"
#include "Viewer.h"
#include <sys/time.h>

/* include socket.h for irix and apple */
#ifndef LINUX
#include <sys/socket.h>
#endif

#include "EAIheaders.h"


/* get a node pointer in memory for a node. Return the node pointer, or NULL if it fails */
uintptr_t EAI_GetNode(const char *str) {
	struct X3D_Node *myn;

	myn = parser_getNodeFromName(str);
	#ifdef EAIVERBOSE
	printf ("EAI_GetNode for %s returns %x - it is a %s\n",str,myn,stringNodeType(myn->_nodeType));
	#endif
	return (uintptr_t) myn;
}

	
char *EAI_GetTypeName (unsigned int uretval) {
	printf ("HELP::EAI_GetTypeName %d\n",uretval);
	return "unknownType";
}


int SAI_IntRetCommand (char cmnd, const char *fn) {
	printf ("HELP::SAI_IntRetCommand, %c, %s\n",cmnd,fn);
	return 0;
}

char * SAI_StrRetCommand (char cmnd, const char *fn) {
	printf ("HELP::SAI_StrRetCommand, %c, %s\n",cmnd,fn);
	return "iunknownreturn";
}

char* EAI_GetValue(unsigned int nodenum, const char *fieldname, const char *nodename) {
	printf ("HELP::EAI_GetValue, %d, %s %s\n",nodenum, fieldname, nodename);
}

unsigned int EAI_GetViewpoint(const char *str) {
	printf ("HELP::EAI_GetViewpoint %s\n",str);
	return 0;
}
