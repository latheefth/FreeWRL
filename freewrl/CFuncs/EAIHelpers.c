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


void EAI_GetType (uintptr_t cNode,  char *ctmp, char *dtmp, uintptr_t *cNodePtr, uintptr_t *fieldOffset,
			uintptr_t *dataLen, uintptr_t *typeString,  unsigned int *scripttype, int *accessType) {

	struct X3D_Node *nodePtr;
	int myField;
	int *myofs;
	int ctype;
/* 	
	cNode = node pointer into memory - assumed to be valid
	ctmp = field as a string - eg, "addChildren"
	dtmp = access method = "eventIn", "eventOut", "field" or...???
	cNodePtr = C node pointer;
	fieldOffset = offset;
	dataLen = data len;
	typeString = EAIFIELD_TYPE_STRING (ctype);
	scripttype = 0 - meaning, not to/from a javascript. (see CRoutes.c for more info)
	
*/

	nodePtr = (struct X3D_Node*) cNode;
	/*
	printf ("this is a valid C node %d (%x)\n",nodePtr,nodePtr);
	printf ("	of type %d\n",nodePtr->_nodeType);
	printf ("	of string type %s\n",stringNodeType(nodePtr->_nodeType)); 
	*/
					

	if ((strncmp (ctmp,"addChildren",strlen("addChildren")) == 0) || 
	(strncmp (ctmp,"removeChildren",strlen("removeChildren")) == 0)) {
		myField = findFieldInALLFIELDNAMES("children");
	} else {
		/* try finding it, maybe with a "set_" or "changed" removed */
		myField = findRoutedFieldInFIELDNAMES(ctmp,0);
		if (myField == -1) 
			myField = findRoutedFieldInFIELDNAMES(ctmp,1);
	}
	myofs = NODE_OFFSETS[nodePtr->_nodeType];

	/* find offsets, etc */
       	findFieldInOFFSETS(myofs, myField, fieldOffset, &ctype, accessType);

	/* return values. */
	*cNodePtr = cNode; 	/* node pointer */
	/* fieldOffset - assigned above - offset */
	*dataLen = 0;	/* data len */
	*typeString = EAIFIELD_TYPE_STRING(ctype);	
	*scripttype =0;

	/* re-map the access type back for children fields */
	if (strncmp (ctmp,"addChildren",strlen("addChildren")) == 0) *accessType = KW_eventIn; 
	if (strncmp (ctmp,"removeChildren",strlen("removeChildren")) == 0) *accessType = KW_eventOut;
					
	/* printf ("so we have coffset %d, ctype %c, ctmp %s\n",fieldOffset,dataLen, KEYWORDS[xxx]);  */
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
