/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003,2005 John Stewart, Ayla Khan CRC Canada
 Portions Copyright (C) 1998 John Breen
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/************************************************************************/
/************************************************************************/

#include "headers.h"
#include "Viewer.h"
#include <sys/time.h>

/* include socket.h for irix and apple */
#ifndef LINUX
#include <sys/socket.h>
#endif

#include "EAIheaders.h"

/*****************************************************************
*
*	handle_Listener is called when a requested value changes.
*
*	What happens is that the "normal" freewrl routing code finds
*	an EventOut changed, copies the data to the buffer EAIListenerData,
*	and copies an extra bit to the global CRoutesExtra.
*
*	(see the CRoutes_Register call above for this routing setup)
*
*	This routine decodes the data type and acts on it. The data type
*	has (currently) an id number, that the client uses, and the data
*	type.
*
********************************************************************/

void handle_Listener () {
	int id, tp;
	char buf[EAIREADSIZE];
	struct Multi_Node *mfptr;	/* used for freeing memory*/

	/* get the type and the id.*/
	tp = CRoutesExtra&0xff;
	id = (CRoutesExtra & 0xffffff00) >>8;

	#ifdef EAIVERBOSE
		printf ("Handle listener, id %x type %x extradata %x\n",id,tp,CRoutesExtra);
	#endif

	/* convert the data to string form, for sending to the EAI java client */
	EAI_Convert_mem_to_ASCII (id,"EV", tp, EAIListenerData, buf);

	/* if this is a MF type, there most likely will be malloc'd memory to free... */
	switch (tp) {
		case EAI_MFCOLOR:
		case EAI_MFCOLORRGBA:
		case EAI_MFFLOAT:
		case EAI_MFTIME:
		case EAI_MFINT32:
		case EAI_MFSTRING:
		case EAI_MFNODE:
		case EAI_MFROTATION:
		case EAI_MFVEC2F:
		case EAI_MFVEC3F: {
			mfptr = (struct Multi_Node *) EAIListenerData;
			if (((*mfptr).p) != NULL) free ((*mfptr).p);
		}
		default: {}
	}




	/* zero the memory for the next time - MultiMemcpy needs this to be zero, otherwise
	   it might think that the "oldlen" will be non-zero */
	bzero(&EAIListenerData, sizeof(EAIListenerData));

	/* append the EV_EOT marker to the end of the string */
	strcat (buf,"\nEV_EOT");

	#ifdef EAIVERBOSE
	printf ("Handle Listener, returning %s\n",buf);
	#endif

	/* send the EV reply */
	EAI_send_string(buf,EAIlistenfd);
}


