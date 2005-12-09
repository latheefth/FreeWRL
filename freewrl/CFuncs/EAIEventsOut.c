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
#include "Structs.h"
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

	/* send the EV reply */
	EAI_send_string(buf,EAIlistenfd);
}



/* convert a number in memory to a printable type. Used to send back EVents, or replies to
   the Java client program. */

void EAI_Convert_mem_to_ASCII (int id, char *reptype, int type, char *memptr, char *buf) {

	double dval;
	float fl[4];
	float *fp;
	int *ip;
	int ival;
	int row;			/* MF* counter */
	struct Multi_String *MSptr;	/* MFString pointer */
	struct Multi_Node *MNptr;	/* MFNode pointer */
	struct Multi_Color *MCptr;	/* MFColor pointer */
	char *ptr;			/* used for building up return string */
	STRLEN xx;

	int numPerRow;			/* 1, 2, 3 or 4 floats per row of this MF? */
	int i;

	/* used because of endian problems... */
	int *intptr;
	intptr = (int *) memptr;

	switch (type) {
		case EAI_SFBOOL: 	{
			#ifdef EAIVERBOSE 
			printf ("EAI_SFBOOL\n");
			#endif

			if (*intptr == 1) sprintf (buf,"%s\n%f\n%d\nTRUE",reptype,TickTime,id);
			else sprintf (buf,"%s\n%f\n%d\nFALSE",reptype,TickTime,id);
			break;
		}

		case EAI_SFTIME:	{
			#ifdef EAIVERBOSE 
			printf ("EAI_SFTIME\n");
			#endif
			memcpy(&dval,memptr,sizeof(double));
			sprintf (buf, "%s\n%f\n%d\n%lf",reptype,TickTime,id,dval);
			break;
		}

		case EAI_SFNODE:
		case EAI_SFINT32:	{
			#ifdef EAIVERBOSE 
			printf ("EAI_SFINT32 or EAI_SFNODE\n");
			#endif
			memcpy(&ival,memptr,sizeof(int));
			sprintf (buf, "%s\n%f\n%d\n%d",reptype,TickTime,id,ival);
			break;
		}

		case EAI_SFFLOAT:	{
			#ifdef EAIVERBOSE 
			printf ("EAI_SFFLOAT\n");
			#endif

			memcpy(fl,memptr,sizeof(float));
			sprintf (buf, "%s\n%f\n%d\n%f",reptype,TickTime,id,fl[0]);
			break;
		}

		case EAI_SFVEC3F:
		case EAI_SFCOLOR:	{
			#ifdef EAIVERBOSE 
			printf ("EAI_SFCOLOR or EAI_SFVEC3F\n");
			#endif
			memcpy(fl,memptr,sizeof(float)*3);
			sprintf (buf, "%s\n%f\n%d\n%f %f %f",reptype,TickTime,id,fl[0],fl[1],fl[2]);
			break;
		}

		case EAI_SFVEC2F:	{
			#ifdef EAIVERBOSE 
			printf ("EAI_SFVEC2F\n");
			#endif
			memcpy(fl,memptr,sizeof(float)*2);
			sprintf (buf, "%s\n%f\n%d\n%f %f",reptype,TickTime,id,fl[0],fl[1]);
			break;
		}

		case EAI_SFROTATION:	{
			#ifdef EAIVERBOSE 
			printf ("EAI_SFROTATION\n");
			#endif

			memcpy(fl,memptr,sizeof(float)*4);
			sprintf (buf, "%s\n%f\n%d\n%f %f %f %f",reptype,TickTime,id,fl[0],fl[1],fl[2],fl[3]);
			break;
		}

		case EAI_SFSTRING:	{
			#ifdef EAIVERBOSE 
			printf ("EAI_SFSTRING\n");
			#endif
			sprintf (buf, "%s\n%f\n%d\n\"%s\"",reptype,TickTime,id,memptr);
			break;
		}

		case EAI_MFSTRING:	{
			#ifdef EAIVERBOSE 
			printf ("EAI_MFSTRING\n");
			#endif

			/* make the Multi_String pointer */
			MSptr = (struct Multi_String *) memptr;

			/* printf ("EAI_MFString, there are %d strings\n",(*MSptr).n);*/
			sprintf (buf, "%s\n%f\n%d\n",reptype,TickTime,id);
			ptr = buf + strlen(buf);

			for (row=0; row<(*MSptr).n; row++) {
        	        	/* printf ("String %d is %s\n",row,SvPV((*MSptr).p[row],xx));*/
				if (strlen (SvPV((*MSptr).p[row],xx)) == 0) {
					sprintf (ptr, "\"XyZZtitndi\" "); /* encode junk for Java side.*/
				} else {
					sprintf (ptr, "\"%s\" ",SvPV((*MSptr).p[row],xx));
				}
				/* printf ("buf now is %s\n",buf);*/
				ptr = buf + strlen (buf);
			}

			break;
		}

		case EAI_MFNODE: 	{
			MNptr = (struct Multi_Node *) memptr;

			#ifdef EAIVERBOSE 
			printf ("EAI_MFNode, there are %d nodes at %d\n",(*MNptr).n,(int) memptr);
			#endif

			sprintf (buf, "%s\n%f\n%d\n",reptype,TickTime,id);
			ptr = buf + strlen(buf);

			for (row=0; row<(*MNptr).n; row++) {
				sprintf (ptr, "%d ",(int) (*MNptr).p[row]);
				ptr = buf + strlen (buf);
			}
			break;
		}

		case EAI_MFINT32: {
			MCptr = (struct Multi_Color *) memptr;
			#ifdef EAIVERBOSE 
				printf ("EAI_MFColor, there are %d nodes at %d\n",(*MCptr).n,(int) memptr);
			#endif

			sprintf (buf, "%s\n%f\n%d\n%d \n",reptype,TickTime,id,(*MCptr).n);
			ptr = buf + strlen(buf);

			ip = (int *) (*MCptr).p;
			for (row=0; row<(*MCptr).n; row++) {
				sprintf (ptr, "%d \n",*ip); 
				ip++;
				/* printf ("line %d is %s\n",row,ptr);  */
				ptr = buf + strlen (buf);
			}

			break;
		}

		case EAI_MFFLOAT:
		case EAI_MFVEC2F:
		case EAI_MFVEC3F:
		case EAI_MFROTATION:
		case EAI_MFCOLOR: {
			numPerRow=3;
			if (type==EAI_MFFLOAT) {numPerRow=1;}
			else if (type==EAI_MFVEC2F) {numPerRow=2;}
			else if (type==EAI_MFROTATION) {numPerRow=4;}

			MCptr = (struct Multi_Color *) memptr;
			#ifdef EAIVERBOSE 
				printf ("EAI_MFColor, there are %d nodes at %d\n",(*MCptr).n,(int) memptr);
			#endif

			sprintf (buf, "%s\n%f\n%d\n%d \n",reptype,TickTime,id,(*MCptr).n);
			ptr = buf + strlen(buf);


			fp = (float *) (*MCptr).p;
			for (row=0; row<(*MCptr).n; row++) {
				for (i=0; i<numPerRow; i++) {
					fl[i] = *fp; fp++;
				}
				switch (numPerRow) {
					case 1:
						sprintf (ptr, "%f \n",fl[0]); break;
					case 2:
						sprintf (ptr, "%f %f \n",fl[0],fl[1]); break;
					case 3:
						sprintf (ptr, "%f %f %f \n",fl[0],fl[1],fl[2]); break;
					case 4:
						sprintf (ptr, "%f %f %f %f \n",fl[0],fl[1],fl[2],fl[3]); break;
				}
				/* printf ("line %d is %s\n",row,ptr); */
				ptr = buf + strlen (buf);
			}

			break;
		}
		default: {
			printf ("EAI, type %c not handled yet\n",type);
		}


/*XXX	case EAI_SFIMAGE:	{handleptr = &handleEAI_SFIMAGE_Listener;break;}*/
/*XXX	case EAI_MFTIME:	{handleptr = &handleEAI_MFTIME_Listener;break;}*/
	}
}

