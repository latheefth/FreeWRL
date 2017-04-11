/*


Small routines to help with interfacing EAI to Daniel Kraft's parser.

*/


/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"

#include "../input/EAIHeaders.h"
#include "../world_script/fieldGet.h"


#if !defined(EXCLUDE_EAI)

/*****************************************************************
*
*	EAIListener is called when a requested value changes.
*
*	(see the CRoutes_Register call above for this routing setup)
*
*	This routine decodes the data type and acts on it. The data type
*	has (currently) an id number, that the client uses, and the data
*	type.
*
********************************************************************/

#ifdef FOR_DEBUGGING
static void goThroughFields(struct X3D_Node *boxptr) {

char utilBuf[10000];
char *tmpptr;
char ctmp;
char dtmp;

int *np;
int myc;
int errcount;


np = (int *) NODE_OFFSETS[boxptr->_nodeType];
        myc = 0;
        while (*np != -1) {
                /* is this a hidden field? */
                if (0 != strncmp(stringFieldType(np[0]), "_", 1) ) {
                                ctmp = (char) mapFieldTypeToEAItype(np[2]) ;
                                dtmp = mapEAItypeToFieldType(ctmp) ;

                                tmpptr = offsetPointer_deref (char *, boxptr,np[1]);
                                printf("%s,%d ",__FILE__,__LINE__) ;
                                printf("Field %d %s , ", myc, stringFieldType(np[0])) ;
                                printf("offset=%d bytes , ", np[1]) ;

                                printf("field_type= %c (%d) , ", ctmp , dtmp) ;
                                printf("Routing=%s , ", stringKeywordType(np[3])) ;
                                printf("Spec=%d , ", np[4]) ;

                                errcount = UtilEAI_Convert_mem_to_ASCII (dtmp,tmpptr, utilBuf);
                                if (0 == errcount) {
                                        printf ("\t\tValue = %s\n",utilBuf);
                                } else {
                                        printf ("\t\tValue = indeterminate....\n");
                                }
                        myc ++;
                }
                np +=5;
        }
}
#endif // FOR_DEBUGGING

/*
struct EAI_Extra_Data {
	int field_id;
	int node_id;
	int field_type;
	int listener_id;
};
*/




void EAIListener () {
	int node_id, field_id;
	int field_type, listener_id;
	char buf[EAIREADSIZE];
	int eaiverbose;
	//ppEAICore p;
	ttglobal tg = gglobal();
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;
	//p = (ppEAICore)tg->EAICore.prv;

	/* get the type and the id.*/
	struct  EAI_Extra_Data *pp = (struct EAI_Extra_Data*) tg->CRoutes.CRoutesExtra;
	field_id = pp->field_id;
	node_id = pp->node_id;
	field_type = pp->field_type;
	listener_id = pp->listener_id;

//	field_id = tg->CRoutes.CRoutesExtra&0xff;
//	node_id = (tg->CRoutes.CRoutesExtra & 0x00ffff00) >>8;
//	field_type = ((tg->CRoutes.CRoutesExtra & 0xff000000) >>24);

	struct X3D_Node* tableNode =  getEAINodeFromTable(node_id,field_id);
	int actual_offs = getEAIActualOffset(node_id, field_id);

	#ifdef FOR_DEBUGGING

	printf ("EAIListener, from table, actual offset is %d\n",actual_offs);
	printf ("EAIListener, frim table, field_type is :%d: %s\n",field_type,FIELDTYPES[field_type]);
	printf ("EAIListener, from table, table node is %p\n",tableNode);
	printf ("EAIListener, from a node of %s\n",stringNodeType(tableNode->_nodeType));
	printf ("EAIListener, type %d, node id %d\n",field_id,node_id);
	printf ("EAIListener, node type from table as a string is %s\n",stringNodeType(getEAINodeTypeFromTable(node_id)));
	goThroughFields(tableNode);
	#endif //FOR_DEBUGGING


	char *field_pointer = offsetPointer_deref (char *, tableNode, actual_offs);
	EAI_Convert_mem_to_ASCII (listener_id,"EV", field_type, field_pointer, buf);

	/* append the EV_EOT marker to the end of the string */
	strcat (buf,"\nEV_EOT");

	if (eaiverbose) {
		printf ("Handle Listener, returning %s\n",buf);
	}	

	/* send the EV data (not really a reply, but a system event) */
	fwlio_RxTx_sendbuffer(__FILE__,__LINE__,CHANNEL_EAI,buf) ;
}

#endif //EXCLUDE_EAI
