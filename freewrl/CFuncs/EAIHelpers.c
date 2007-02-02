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

Interface functions, for PROTO field access. From an email from Daniel:

Access fields:
size_t protoDefiniton_getFieldCount(protoDef)
struct ProtoFieldDecl* protoDefinition_getFieldByNum(protoDef, index)

Properties of struct ProtoFieldDecl:
indexT protoFieldDecl_getType(fdecl)
indexT protoFieldDEcl_getAccessType(fdecl)

Get name:
indexT protoFieldDecl_getIndexName(fdecl)
const char* protoFieldDecl_getStringName(fdecl)

Default value:
union anyVrml protoFieldDecl_getDefaultValue(fdecl)
this union contains fields for every specific type, see CParseGeneral.h for exact structure.

Desination pointers:
size_t protoFieldDecl_getDestinationCount(fdecl)
struct OffsetPointer* protoFieldDecl_getDestination(fdecl, index)

This struct contains both a node and an ofs field.

************************************************************************/

#include "headers.h"
#include "Viewer.h"
#include <sys/time.h>

/* include socket.h for irix and apple */
#ifndef LINUX
#include <sys/socket.h>
#endif

#include "EAIheaders.h"
#include "CProto.h"
#include "CParseLexer.h"


/* get a node pointer in memory for a node. Return the node pointer, or NULL if it fails */
uintptr_t EAI_GetNode(const char *str) {
	struct X3D_Node *myn;

	#ifdef EAIVERBOSE
	printf ("EAI_GetNode - getting %s\n",str);
	#endif

	myn = parser_getNodeFromName(str);
	#ifdef EAIVERBOSE
	printf ("EAI_GetNode for %s returns %x - it is a %s\n",str,myn,stringNodeType(myn->_nodeType));
	#endif
	return (uintptr_t) myn;
}


int mapToKEYWORDindex (indexT pkwIndex) {
	if (pkwIndex == PKW_exposedField) return KW_exposedField;
	if (pkwIndex == PKW_eventIn) return KW_eventIn;
	if (pkwIndex == PKW_eventOut) return KW_eventOut;
	if (pkwIndex == PKW_field) return KW_field;
	return 0;
}
/***************
Access fields:
size_t protoDefiniton_getFieldCount(protoDef)
struct ProtoFieldDecl* protoDefinition_getFieldByNum(protoDef, index)

Properties of struct ProtoFieldDecl:
indexT protoFieldDecl_getType(fdecl)
indexT protoFieldDEcl_getAccessType(fdecl)

Get name:
indexT protoFieldDecl_getIndexName(fdecl)
const char* protoFieldDecl_getStringName(fdecl)

Default value:
union anyVrml protoFieldDecl_getDefaultValue(fdecl)
this union contains fields for every specific type, see CParseGeneral.h for exact structure.

Desination pointers:
size_t protoFieldDecl_getDestinationCount(fdecl)
struct OffsetPointer* protoFieldDecl_getDestination(fdecl, index)
*/

/*********************************************************************************

findFieldInPROTOOFFSETS General purpose PROTO field lookup function.

parameters:

	strct X3D_Node *myNode	a node pointer to an X3D_Node. This should be to an X3D_Group with the
				__protoDef field pointing to a valid data structure.

	char *myField		a string pointer to a field name to look up.

	uintptr_t *myNodeP	pointer to the X3D_Node* in memory of the last ISd field in this PROTO.
				returns 0 if not found.

	int *myOfs		offset to the field in the last ISd field memory structure. returns
				0 if not found.

	int *myType		returns a value, eg "EAI_SFFLOAT", or 0 if this field is not found.

	int *accessType 	returns one of KW_exposedField, KW_eventIn, KW_eventOut, KW_field, or 0 
				if field not found.

	int *myProtoIndex	index into the proto field definition tables for this field. returns
				-1 if this field is not found.

	char *retstr		char buffer for "EAI" style ascii value. If not NULL, assumes
				the buffer will hold enough for call to EAI_Convert_mem_to_ASCII.

*********************************************************************************/



void findFieldInPROTOOFFSETS (struct X3D_Node *myNode, char *myField, uintptr_t *myNodeP,
					int *myOfs, int *myType, int *accessType, 
					int *myProtoIndex, char *retstr) {

	struct X3D_Group *group;
	struct ProtoDefinition *myProtoDecl;
	struct ProtoFieldDecl *thisIndex;
	int fc, fc2;
	union anyVrml myDefaultValue;
	struct OffsetPointer *myP;
	char *cp;


	/* set these values, so that we KNOW if we have found the correct field */
	*myType = 0;
	*myNodeP = 0;
	*myOfs = 0;
	*myProtoIndex = -1;

	#ifdef FF_PROTO_PRINT
	printf ("setField_method1, trouble finding field %s in node %s\n",myField,stringNodeType(myNode->_nodeType));
	printf ("is this maybe a PROTO?? if so, it will be a Group node with __protoDef set to the pointer\n");
	#endif

	if (myNode->_nodeType == NODE_Group) {
		group = (struct X3D_Group *)myNode;
		#ifdef FF_PROTO_PRINT
		printf ("it IS a group...\n"); 
		#endif

		if (group->__protoDef) {
			#ifdef FF_PROTO_PRINT
			printf ("and, this is a PROTO...have to go through PROTO defs to get to it\n");
			#endif

			myProtoDecl = (struct ProtoDefinition *) group->__protoDef;

			#ifdef FF_PROTO_PRINT
			printf ("proto has %d fields...\n",protoDefinition_getFieldCount(myProtoDecl)); 
			#endif

			for (fc = 0; fc <protoDefinition_getFieldCount(myProtoDecl); fc++) {
				thisIndex = protoDefinition_getFieldByNum(myProtoDecl, fc);

				#ifdef FF_PROTO_PRINT
				printf ("working through index %d\n",fc); 
				printf ("	field name is %s\n",protoFieldDecl_getStringName(thisIndex));
				printf ("	type is %d which is %s\n",protoFieldDecl_getType(thisIndex),
					FIELDTYPES[protoFieldDecl_getType(thisIndex)]);
				printf ("	Accesstype is %d as string %s\n",protoFieldDecl_getAccessType(thisIndex),
					PROTOKEYWORDS[protoFieldDecl_getAccessType(thisIndex)]);
				printf ("	indexnameString %s\n",protoFieldDecl_getStringName(thisIndex));



				#endif

				/* do we want the output value?? */
				if (retstr != NULL) {
					myDefaultValue = protoFieldDecl_getDefaultValue(thisIndex);		
					EAI_Convert_mem_to_ASCII (0, "", mapFieldTypeToEAItype(protoFieldDecl_getType(thisIndex)), 
						&myDefaultValue, retstr);
				
					/* EAI_Convert_mem_to_ASCII gives us timestamps, etc, lets just get the last part */
				
					cp = rindex(retstr,'\n');
					if (cp != NULL) {
						cp ++; /* skip past the \n */
						/* copy the memory over. */
						memmove (retstr,cp,strlen(cp));
						#ifdef FF_PROTO_PRINT
						printf ("	value now %s\n",cp);
						#endif
					}
				}

				#ifdef FF_PROTO_PRINT
				printf ("		dest pointer is %d\n",protoFieldDecl_getDestinationCount(thisIndex));
				#endif

				/* is this the node we are looking for? */
				if (strlen(protoFieldDecl_getStringName(thisIndex)) == strlen(myField)) {
					if (strcmp(protoFieldDecl_getStringName(thisIndex),myField) == 0) {
			
						#ifdef FF_PROTO_PRINT
						printf ("		found the field in the PROTO\n");
						#endif

						*myProtoIndex = fc;
						*myType = mapFieldTypeToEAItype(protoFieldDecl_getType(thisIndex));
						*accessType = mapToKEYWORDindex(protoFieldDecl_getAccessType(thisIndex));

						for (fc2 = 0; fc2 < protoFieldDecl_getDestinationCount(thisIndex); fc2 ++) {
							myP = protoFieldDecl_getDestination(thisIndex,fc2); 
							#ifdef FF_PROTO_PRINT
							printf ("		points to %x, type %s ofs %d\n",
								myP->node, stringNodeType(myP->node->_nodeType),myP->ofs);
							#endif

							/* these values most likely changed - if more than 1, just use the last for now 
							   as EAI/SAI can only handle 1 */
							*myNodeP = (uintptr_t *)myP->node;	
							*myOfs = myP->ofs;
						}
					}
				}
			}

		}
	}
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
	typeString = mapFieldTypeToEAItype (ctype);
	scripttype = 0 - meaning, not to/from a javascript. (see CRoutes.c for more info)
	
*/

	int myProtoIndex;
	char myCharBuffer[300];
	int myFieldOffs;

	nodePtr = (struct X3D_Node*) cNode;
	
	#ifdef EAIVERBOSE
	printf ("start of EAI_GetType, this is a valid C node %d (%x)\n",nodePtr,nodePtr);
	printf ("	of type %d\n",nodePtr->_nodeType);
	printf ("	of string type %s\n",stringNodeType(nodePtr->_nodeType)); 
	#endif

					

	if ((strncmp (ctmp,"addChildren",strlen("addChildren")) == 0) || 
	(strncmp (ctmp,"removeChildren",strlen("removeChildren")) == 0)) {
		myField = findFieldInALLFIELDNAMES("children");
	} else {
		/* try finding it, maybe with a "set_" or "changed" removed */
		myField = findRoutedFieldInFIELDNAMES(nodePtr,ctmp,0);
		if (myField == -1) 
			myField = findRoutedFieldInFIELDNAMES(nodePtr,ctmp,1);
	}
	myofs = NODE_OFFSETS[nodePtr->_nodeType];

	/* find offsets, etc */
       	findFieldInOFFSETS(myofs, myField, &myFieldOffs, &ctype, accessType);

	/* is this a PROTO, or just an invalid field?? */ 
	if (myFieldOffs <= 0) {
		#ifdef EAIVERBOSE
			printf ("EAI_GetType, myFieldOffs %d, try findFieldInPROTOOFFSETS\n",myFieldOffs);
		#endif
		findFieldInPROTOOFFSETS (nodePtr, ctmp, cNodePtr, &myFieldOffs, &ctype, accessType ,&myProtoIndex,
			myCharBuffer);
	} else {
		*cNodePtr = cNode; 	/* node pointer */
		ctype = mapFieldTypeToEAItype(ctype); /* change to EAI type */
	}

	/* return values. */
	/* fieldOffset - assigned above - offset */
	*fieldOffset = (uintptr_t) myFieldOffs;
	*dataLen = returnRoutingElementLength();	/* data len */
	*typeString = (uintptr_t) ctype;	
	*scripttype =0;

	/* re-map the access type back for children fields */
	if (strncmp (ctmp,"addChildren",strlen("addChildren")) == 0) *accessType = KW_eventIn; 
	if (strncmp (ctmp,"removeChildren",strlen("removeChildren")) == 0) *accessType = KW_eventOut;
					
	#ifdef EAIVERBOSE
	printf ("EAI_GetType, so we have coffset %d, ctype %x, ctmp %s\n",*fieldOffset,ctype, KEYWORDS[*accessType]);
	#endif
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
