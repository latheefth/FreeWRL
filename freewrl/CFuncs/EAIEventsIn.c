/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003,2005 John Stewart, Ayla Khan CRC Canada
 Portions Copyright (C) 1998 John Breen
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/************************************************************************

EAIEventsIn.c - handle incoming EAI (and java class) events with panache.

************************************************************************/

#include "headers.h"
#include "Structs.h"
#include "Viewer.h"
#include <sys/time.h>

/* include socket.h for irix and apple */
#ifndef LINUX
#include <sys/socket.h>
#endif

#include "EAIheaders.h"
#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}

/* get how many bytes in the type */
int returnElementLength(int type) {
	  switch (type) {
    		case MFTIME : return sizeof(double); break;
    		case SFNODE :
    		case MFNODE :
    		case MFINT32: return sizeof(int)   ; break;
	  	default     : {}
	}
	return sizeof(float) ; /* turn into byte count */
}

/* how many numbers/etc in an array entry? eg, SFVec3f = 3 - 3 floats */
/*		"" ""			eg, MFVec3f = 3 - 3 floats, too! */
int returnElementRowSize (int type) {
	switch (type) {
		case SFVEC2F: 
		case MFVEC2F: 
			return 2;
		case SFCOLOR: 
		case MFCOLOR: 
		case SFVEC3F: 
		case MFVEC3F: 
			return 3;
		case SFROTATION: 
		case MFROTATION: 
			return 4;
	}
	return 1;

}

/* copy new scanned in data over to the memory area in the scene graph. */

void *Multi_Struct_memptr (int type, void *memptr) {
	struct Multi_Vec3f *mp;

	/* is this a straight copy, or do we have a struct to send to? */
	/* now, some internal reps use a structure defined as:
	   struct Multi_Vec3f { int n; struct SFColor  *p; };
	   so, we have to put the data in the p pointer, so as to
	   not overwrite the data. */

	switch (type) {
		case MFVEC2F: 
		case MFCOLOR: 
		case MFVEC3F: 
		case MFROTATION: 
		case MFFLOAT:
		case MFINT32:
			mp = (struct Multi_Vec3f*) memptr;
			memptr = mp->p;

		default: {}
		}
	return memptr;
}


/* change a memory loction - do we need to do a simple copy, or
do we need to change a Structure type? */

void SetMemory (int type, void *destptr, void *srcptr, int len) {
	void *newptr;
	struct Multi_Vec3f *mp;

	/* is this a structure? If Multi_Struct_memptr returns a different
	   pointer, than it IS a struct {int n void *p} structure type. */

	newptr = Multi_Struct_memptr(type, destptr);
	if (newptr != destptr) {
		mp = (struct Multi_Vec3f*) destptr;
		/* printf ("SetMemory was %d ",mp->n); */
		mp->n=0;
		FREE_IF_NZ(mp->p);
		mp->p = malloc(len);
		memcpy (mp->p,srcptr,len);
		mp->n = len /(returnElementLength(type)*returnElementRowSize(type));
		/* printf (" is %d\n ",mp->n); */
	} else {
		/* this is a straight copy */
		memcpy (destptr, srcptr, len);
	}
}





/* take an ASCII string from the EAI or CLASS, and convert it into
   a memory block 

   This is not the whole command, just a convert this string to a 
   bunch of floats, or equiv. */

int ScanValtoBuffer(int *quant, int type, char *buf, void *memptr, int bufsz) {
	float *fp;
	int *ip;
	void *tmpbuf;
	int count;
	int len;

	/* pointers to cast memptr to*/
	float *flmem;

	/* pass in string in buf; memory block is memptr, size in bytes, bufsz */

	#ifdef EAIVERBOSE
	printf("ScanValtoBuffer - buffer %s\n",buf);
	#endif

	if (bufsz < 10) {
		printf ("cant perform conversion with small buffer\n");
		return (0);
	}

	switch (type) {
	    case SFBOOL:	{	/* SFBool */
	    	if (strncasecmp(buf,"true",4)==0) {
		    *(int *)memptr = 1;
	    	} else {
		    *(int *)memptr = 0;
	    	}
		len = sizeof(int);
	    	break;
	    }

	    case SFINT32: {
	    	sscanf (buf,"%d",(int *)memptr);
		len = sizeof (int);
	    	break;
	    }
	    case SFFLOAT: {
	    	sscanf (buf,"%f",(float *)memptr);
		len = sizeof (float);
	    	break;
	    }

	    case SFVEC2F: {	/* SFVec2f */
		flmem = (float *)memptr;
	    	sscanf (buf,"%f %f",&flmem[0], &flmem[1]);
		len = sizeof(float) * 2;
	    	break;
	    }

	    case SFVEC3F:
	    case SFCOLOR: {	/* SFColor */
		flmem = (float *)memptr;
	    	sscanf (buf,"%f %f %f",&flmem[0],&flmem[1],&flmem[2]);
		len = sizeof(float) * 3;
	    	break;
	    }

	    case SFROTATION: {
		flmem = (float *)memptr;
	    	sscanf (buf,"%f %f %f %f",&flmem[0],&flmem[1],&flmem[2],&flmem[3]);
		len = sizeof(float) * 4;
	    	break;
	    }

	    case SFTIME: {
		sscanf (buf, "%lf", (double *)memptr);
		len = sizeof(double);
		break;
	    }

	    case MFNODE:
	    case MFINT32:
	    case MFTIME:
	    case SFNODE:
	    case MFCOLOR:
	    case MFVEC3F:
	    case MFFLOAT:
	    case MFROTATION:
	    case MFVEC2F: {
		/* first number is the "number of numbers". */

		/* scan to start of element count, and read it in.*/
		while (*buf==' ') buf++; 
		sscanf (buf,"%d",quant); while (*buf>' ') buf++;

		/* how many elements per row of this type? */
		*quant = *quant * returnElementRowSize(type);

		/* so, right now, quant tells us how many numbers we will read in. */



		/* how long is each element in bytes of this type? */
		len = *quant * returnElementLength(type);

		  #ifdef EAIVERBOSE
			printf ("bufsz is %d, len = %d quant = %d str: %s\n",bufsz, len, *quant,buf);
		  #endif
		  if (len > bufsz) {
			  printf ("Warning, MultiFloat too large, truncating to %d \n",bufsz);
			  len = bufsz;
		  }

		tmpbuf = (float *) malloc (len);
		fp = (float *) tmpbuf;
		ip = (int *) tmpbuf;
		for (count = 0; count < *quant; count++) {
			/* scan to start of number */
			while (*buf<=' ') buf++;

			/* scan in number */
			if ((type==MFINT32) || (type=MFNODE)) sscanf (buf,"%d",fp);
			else sscanf (buf,"%f",fp);

			/* go to next number */
			while (*buf >' ') buf++;
			fp ++;
		}
			


		  /* now, copy over the data to the memory pointer passed in */
		  if(NULL != tmpbuf) memcpy (memptr,tmpbuf,len);
		  else perror("ScanValtoBuffer: tmpbuf NULL!");

		  /* free the memory malloc'd in Alberto's code */
		  free (tmpbuf);
		  break;
	    }

	    case MFSTRING: {
		int count;
		SV ** newp;
		struct xpv *mypv;
		struct Multi_String *strptr;
		int thisele, thissize, maxele;	/* used for reading in MFStrings*/


		/* return a Multi_String.*/
		/*  struct Multi_String { int n; SV * *p; };*/
		/*  buf will look like:*/
		/*  2  0;9:wordg.png  1;12:My"wordg.png*/
		/*  where 2 = max elements; 0;9 is element 0, 9 chars long...*/


		strptr = (struct Multi_String *)memptr;

		/* scan to start of element count, and read it in.*/
		while (*buf==' ') buf++;
		sscanf (buf,"%d",&maxele);
		while (*buf!=' ') buf++;

		/* make (and initialize) this MFString internal representation.*/
		strptr->n = maxele;
		/*printf ("mallocing strptr->p, size %d\n",sizeof(strptr->p));*/
		strptr->p = (SV**)malloc (maxele * sizeof(strptr->p));
		newp = strptr->p;

		/* scan through EAI string, extract strings, etc, etc.*/
		do {
			/* scan to start of element number*/

			/* make the new SV */
			*newp = (SV*)malloc (sizeof (struct STRUCT_SV));
			(*newp)->sv_flags = SVt_PV | SVf_POK;
			(*newp)->sv_refcnt=1;
			mypv = (struct xpv *)malloc(sizeof (struct xpv));
			/*printf ("just mallocd for mypv, it is %d and size %d\n",*/
			/*		mypv, sizeof (struct xpv));*/
			(*newp)->sv_any = mypv;

			while (*buf==' ') buf++;
			sscanf (buf,"%d;%d",&thisele,&thissize);
			/*printf ("this element %d has size %d\n",thisele,thissize);*/

			/*mypv = (struct xpv *) newp + (thisele*sizeof(newp));*/

			/* scan to start of string*/
			while (*buf!=':') buf++; buf++;

			/* fill in the SV values...copy the string over...*/
			(*mypv).xpv_pv = (char *)malloc (thissize+2);
			strncpy((*mypv).xpv_pv ,buf,thissize);
			(*mypv).xpv_pv[thissize] = '\0'; /* null terminate*/
			(*mypv).xpv_cur = thissize-1;    /* size without term*/
			(*mypv).xpv_len = thissize;      /* size with termination*/

			/* increment buf by string size.*/
			buf += thissize;

			/* scan to next start of string, or end of line*/
			while (*buf==' ') buf++;

			/* point to next SV to fill*/
			newp++;
		} while (((int)*buf)>=32);
		/*len = maxele*sizeof(struct Multi_String);*/
		/* return -1 to indicate that this is "wierd".*/
		len = -1;

		break;
	   }
	  default: {
		printf("WARNING: unhandled CLASS from type %s\n", FIELD_TYPE_STRING(type));
		printf ("complain to the FreeWRL team.\n");
		printf ("(string is :%s:)\n",buf);
		return (0);
	    }
	}
	return (len);
}


/******************************************************************************
*
* EAI_parse_commands
*
* there can be many commands waiting, so we loop through commands, and return
* a status of EACH command
*
* a Command starts off with a sequential number, a space, then a letter indicating
* the command, then the parameters of the command.
*
* the command names are #defined at the start of this file.
*
* some commands have sub commands (eg, get a value) to indicate data types,
* (eg, EAI_SFFLOAT); these sub types are indicated with a lower case letter; again,
* look to the top of this file for the #defines
*
*********************************************************************************/

void EAI_parse_commands (char *bufptr) {
	char buf[EAIREADSIZE];	/* return value place*/
	char ctmp[EAIREADSIZE];	/* temporary character buffer*/
	char dtmp[EAIREADSIZE];	/* temporary character buffer*/
	unsigned int nodarr[200]; /* returning node/backnode combos from CreateVRML fns.*/

	int count;
	char command;
	unsigned int uretval;		/* unsigned return value*/
	unsigned int ra,rb,rc,rd;	/* temps*/
	unsigned int scripttype;
	char *EOT;		/* ptr to End of Text marker*/

	while (strlen(bufptr)> 0) {
	/*	printf ("start of while loop, strlen %d str :%s:\n",strlen(bufptr),bufptr);*/

		/* step 1, get the command sequence number */
		if (sscanf (bufptr,"%d",&count) != 1) {
			printf ("EAI_parse_commands, expected a sequence number on command :%s:\n",bufptr);
			count = 0;
		}
	/*	printf ("EAI - seq number %d\n",count);*/

		/* step 2, skip past the sequence number */
		while (isdigit(*bufptr)) bufptr++;
	/*	printf("past sequence number, string:%s\n",bufptr);*/
		while (*bufptr == ' ') bufptr++;
	/*	printf ("past the space, string:%s\n",bufptr);*/

		/* step 3, get the command */

		/* EAIVERBOSE*/
	/*	{*/
	/*	    printf ("EAI_parse_commands cmd %s\n",*bufptr);*/
	/*	    printf ("command %c seq %d\n",*bufptr,count);*/
	/*	}*/

		command = *bufptr;
	/*	printf ("command %c\n",command);*/
		bufptr++;

		/* return is something like: $hand->print("RE\n$reqid\n1\n$id\n");*/

		#ifdef EAIVERBOSE 
		printf ("\n... %d ",count);
		#endif

		switch (command) {
			case GETNAME: {
				#ifdef EAIVERBOSE 
				printf ("GETNAME\n");
				#endif
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,BrowserName);
				break;
				}
			case GETVERSION: {
				#ifdef EAIVERBOSE 
				printf ("GETVERSION\n");
				#endif
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,BrowserVersion);
				break;
				}
			case GETCURSPEED: {
				#ifdef EAIVERBOSE 
				printf ("GETCURRENTSPEED\n");
				#endif
				sprintf (buf,"RE\n%f\n%d\n%f",TickTime,count,getCurrentSpeed());
				break;
				}
			case GETFRAMERATE: {
				#ifdef EAIVERBOSE 
				printf ("GETFRAMERATE\n");
				#endif
				sprintf (buf,"RE\n%f\n%d\n%f",TickTime,count,BrowserFPS);
				break;
				}
			case GETURL: {
				#ifdef EAIVERBOSE 
				printf ("GETURL\n");
				#endif
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,BrowserURL);
				break;
				}
			case GETNODE:  {
				/*format int seq# COMMAND    string nodename*/

				sscanf (bufptr," %s",ctmp);
				#ifdef EAIVERBOSE 
				printf ("GETNODE %s\n",ctmp);
				#endif

				uretval = EAI_GetNode(ctmp);

				sprintf (buf,"RE\n%f\n%d\n%d",TickTime,count,uretval);
				break;
			}
			case GETTYPE:  {
				/*format int seq# COMMAND  int node#   string fieldname   string direction*/

				sscanf (bufptr,"%d %s %s",&uretval,ctmp,dtmp);
				#ifdef EAIVERBOSE 
				printf ("GETTYPE NODE%d %s %s\n",uretval, ctmp, dtmp);
				#endif

				EAI_GetType (uretval,ctmp,dtmp,(int *)&ra,(int *)&rb,(int *)&rc,(int *)&rd,(int *)&scripttype);

				sprintf (buf,"RE\n%f\n%d\n%d %d %d %c %d",TickTime,count,ra,rb,rc,rd,scripttype);
				break;
				}
			case SENDEVENT:   {
				/*format int seq# COMMAND NODETYPE pointer offset data*/
				#ifdef EAIVERBOSE 
				printf ("SENDEVENT %s\n",bufptr);
				#endif
				EAI_SendEvent(bufptr);
				break;
				}
			case CREATEVU:
			case CREATEVS: {
				/*format int seq# COMMAND vrml text     string EOT*/
				if (command == CREATEVS) {
					#ifdef EAIVERBOSE 
					printf ("CREATEVS %s\n",bufptr);
					#endif

					EOT = strstr(EAIbuffer,"\nEOT\n");
					/* if we do not have a string yet, we have to do this...*/
					while (EOT == NULL) {
						EAIbuffer = read_EAI_socket(EAIbuffer,&EAIbufcount, &EAIbufsize, &EAIlistenfd);
						EOT = strstr(EAIbuffer,"\nEOT\n");
					}

					*EOT = 0; /* take off the EOT marker*/

					ra = EAI_CreateVrml("String",bufptr,nodarr,200);
				} else {
					/* sanitize this string - remove leading
					 * and trailing garbage */
					ra = 0; rb = 0;
					while ((ra < strlen(bufptr)) &&
							(bufptr[ra] <= ' '))
						ra++;
					while (bufptr[ra] > ' ') {
						ctmp[rb] = bufptr[ra];
						rb ++; ra++;
					}
					ctmp[rb] = 0;

					#ifdef EAIVERBOSE 
					printf ("CREATEVU %s\n",ctmp);
					#endif
					ra = EAI_CreateVrml("URL",ctmp,nodarr,200);
				}

				sprintf (buf,"RE\n%f\n%d\n",TickTime,count);
				for (rb = 0; rb < ra; rb++) {
					sprintf (ctmp,"%d ", nodarr[rb]);
					strcat (buf,ctmp);
				}

				/* finish this for now*/
				bufptr[0] = 0;
				break;
				}
			case SENDCHILD :  {
				/*format int seq# COMMAND  int node#   ParentNode field ChildNode*/

				sscanf (bufptr,"%d %d %s %s",&ra,&rb,ctmp,dtmp);
				rc = ra+rb; /* final pointer- should point to a Multi_Node*/

				#ifdef EAIVERBOSE 
				printf ("SENDCHILD Parent: %d ParentField: %d %s Child: %s\n",ra, rb, ctmp, dtmp);
				#endif


				getMFNodetype (dtmp,(struct Multi_Node *)rc,
						(struct VRML_Box *)ra,
						strcmp(ctmp,"removeChildren"));

				/* tell the routing table that this node is updated - used for RegisterListeners */
				mark_event((void *)ra,rb);

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}
			case UPDATEROUTING :  {
				/*format int seq# COMMAND  int node#   ParentNode field ChildNode*/

				sscanf (bufptr,"%d %d %s %d",&ra,&rb,ctmp,&rc);
				#ifdef EAIVERBOSE 
				printf ("SENDCHILD %d %d %s %d\n",ra, rb, ctmp, rc);
				#endif

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}
			case REGLISTENER: {
				#ifdef EAIVERBOSE 
				printf ("REGISTERLISTENER %s \n",bufptr);
				#endif

				/*143024848 88 8 e 6*/
				sscanf (bufptr,"%d %d %c %d",&ra,&rb,ctmp,&rc);
				/* so, count = query id, ra pointer, rb, offset, ctmp[0] type, rc, length*/
				ctmp[1]=0;

				/*printf ("REGISTERLISTENER from %d foffset %d fieldlen %d type %s \n",*/
				/*		ra, rb,rc,ctmp);*/


				/* put the address of the listener area in a string format for registering
				   the route - the route propagation will copy data to here */
				sprintf (EAIListenerArea,"%d:0",(int)&EAIListenerData);

				/* set up the route from this variable to the handle_Listener routine */
				CRoutes_Register  (1,(void *)ra,(int)rb, 1, EAIListenerArea, (int) rc,(void *) &handle_Listener, 0, (count<<8)+ctmp[0]); /* encode id and type here*/

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}

			case GETVALUE: {
				#ifdef EAIVERBOSE 
				printf ("GETVALUE %s \n",bufptr);
				#endif


				/* format: ptr, offset, type, length (bytes)*/
				sscanf (bufptr, "%d %d %c %d", &ra,&rb,ctmp,&rc);

				ra = ra + rb;   /* get absolute pointer offset*/
				EAI_Convert_mem_to_ASCII (count,"RE",(int)ctmp[0],(char *)ra, buf);
				break;
				}
			case REPLACEWORLD:  {
				#ifdef EAIVERBOSE 
				printf ("REPLACEWORLD %s \n",bufptr);
				#endif

				EAI_RW(bufptr);
				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}
			case ADDROUTE:
			case DELETEROUTE:  {
				#ifdef EAIVERBOSE 
				printf ("Add/Delete route %s\n",bufptr);
				#endif

				EAI_Route ((char) command,bufptr);
				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}

			case REREADWRL: {

				#ifdef EAIVERBOSE 
				printf ("REREADWRL <%s> \n",bufptr);
				#endif

				EAI_RNewW(bufptr);

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
			}

/*XXXX			case SETDESCRIPT:*/
		  	case STOPFREEWRL: {
				#ifdef EAIVERBOSE 
				printf ("Shutting down Freewrl\n");
				#endif
				if (!RUNNINGASPLUGIN) {
					doQuit();
				    break;
				}
			    }
			  case NEXTVIEWPOINT: {
				#ifdef EAIVERBOSE 
				printf ("Next Viewpoint\n");
				#endif

				Next_ViewPoint();
				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
			    }
			default: {
				printf ("unhandled command :%c: %d\n",command,command);
				strcat (buf, "unknown_EAI_command");
				break;
				}

			}


		/* send the response - events don't send a reply */
		if (command != SENDEVENT) {
			strcat (buf,"\nRE_EOT");
			EAI_send_string (buf,EAIlistenfd);
		}

		/* skip to the next command */
		while (*bufptr >= ' ') bufptr++;

		/* skip any new lines that may be there */
		while ((*bufptr == 10) || (*bufptr == 13)) bufptr++;
	}
}

/* an incoming EAI/CLASS event has come in, convert the ASCII characters
 * to an internal representation, and act upon it */

unsigned int EAI_SendEvent (char *ptr) {
	unsigned char nodetype;
	unsigned int nodeptr;
	unsigned int offset;
	unsigned int scripttype;

	unsigned int memptr;

	int valIndex;
	struct Multi_Color *tcol;

	int len, elemCount;
	int MultiElement;
	char myBuffer[6000];

	/* we have an event, get the data properly scanned in from the ASCII string, and then
		friggin do it! ;-) */

	/* node type */
	nodetype = *ptr; ptr++;

	/* blank space */
	ptr++;

	/* nodeptr, offset */
	sscanf (ptr, "%d %d %d",&nodeptr, &offset, &scripttype);
	while ((*ptr) > ' ') ptr++; 	/* node ptr */
	while ((*ptr) == ' ') ptr++;	/* inter number space(s) */
	while ((*ptr) > ' ') ptr++;	/* node offset */
	while ((*ptr) == ' ') ptr++;	/* inter number space(s) */
	while ((*ptr) > ' ') ptr++;	/* script type */

	#ifdef EAIVERBOSE
		 printf ("EAI_SendEvent, type %c, nodeptr %x offset %x script type %d \n",
				 nodetype,nodeptr,offset, scripttype);
	#endif

	/* We have either a event to a memory location, or to a script. */
	/* the field scripttype tells us whether this is true or not.   */

	memptr = nodeptr+offset;	/* actual pointer to start of destination data in memory */

	/* now, we are at start of data. */


	/* lets go to the first non-blank character in the string */
	while (*ptr == ' ') ptr++;
	#ifdef EAIVERBOSE 
	printf ("EAI_SendEvent, event string now is :%s\n",ptr);
	#endif

	/* is this a MF node, that has floats or ints, and the set1Value method is called? 	*/
	/* check out the java external/field/MF*java files for the string "ONEVAL "		*/
	if (strncmp("ONEVAL ",ptr,7) == 0) {
		ptr += 7;

		/* find out which element the user wants to set - that should be the next number */
		while (*ptr==' ')ptr++;
		sscanf (ptr,"%d",&valIndex);
		while (*ptr>' ')ptr++; /* past the number */
		while (*ptr==' ')ptr++;

		/* lets do some bounds checking here. */
		tcol = (struct Multi_Color *) memptr;
		if (tcol->n <= valIndex) {
			printf ("Error, setting 1Value of %d, max in scenegraph is %d\n",valIndex,tcol->n);
			return FALSE;
		}


		/* if this is a struct Multi* node type, move the actual memory pointer to the data */
		memptr = (unsigned int) Multi_Struct_memptr(nodetype-EAI_SFUNKNOWN, (void *) memptr);

		/* and index into that array; we have the index, and sizes to worry about 	*/
		memptr += valIndex * returnElementLength(nodetype-EAI_SFUNKNOWN) *  returnElementRowSize(nodetype-EAI_SFUNKNOWN);

		/* and change the nodetype to reflect this change */
		switch (nodetype) {
			case EAI_MFNODE: nodetype = EAI_SFNODE; break;
			case EAI_MFINT32: nodetype = EAI_SFINT32; break;
			case EAI_MFTIME: nodetype = EAI_SFTIME; break;
			case EAI_MFCOLOR: nodetype = EAI_SFCOLOR; break;
			case EAI_MFVEC3F: nodetype = EAI_SFVEC3F; break;
			case EAI_MFFLOAT: nodetype = EAI_SFFLOAT; break;
			case EAI_MFROTATION: nodetype = EAI_SFROTATION; break;
			case EAI_MFVEC2F: nodetype = EAI_SFVEC2F; break;
			default: {printf ("EAI input, ONEVAL set but type unknown %d\n",nodetype);
				  return FALSE;
				}
		}
	}

	/* This switch statement is almost identical to the one in the Javascript
	   code (check out CFuncs/CRoutes.c), except that explicit Javascript calls
	   are impossible here (this is not javascript!) */


	/* convert the ascii string into an internal representation */
	/* this will return '0' on failure */
	len = ScanValtoBuffer(&elemCount, nodetype - EAI_SFUNKNOWN, ptr , myBuffer, sizeof(myBuffer));


	/* an error in ascii to memory conversion happened */
	if (len == 0) {
		printf ("EAI_SendEvent, conversion failure\n");
		return( -1 );
	}

	MultiElement=FALSE;
	switch (nodetype) {
		case EAI_SFBOOL:
		case EAI_SFTIME:
		case EAI_SFNODE:
		case EAI_SFINT32:
		case EAI_SFFLOAT: {
			  MultiElement = FALSE;  /*Redundant, I hope the compiler will optimize */
			  break;
		} /* these are all ok, just continue on */

		case EAI_SFVEC2F:
	  	case EAI_SFVEC3F:
	  	case EAI_SFCOLOR:
		case EAI_SFROTATION: {
			MultiElement=TRUE;
			break;
		}
	        case EAI_MFROTATION:
	        case EAI_MFTIME    :
	        case EAI_MFINT32   :
	        case EAI_MFNODE    :
	        case EAI_MFVEC2F   :
	        case EAI_MFVEC3F   :
	        case EAI_MFCOLOR   :
	        case EAI_MFFLOAT   : {
		    MultiElement=TRUE;
		   break;
		}
		case EAI_MFSTRING: {
			/* myBuffer will have a full SV structure now, and len will*/
			/* be -1.*/
			break;
		}

		case EAI_SFSTRING: {
			/* AD What happens here???????? */
			/* this can be handled exactly like a set_one_ECMAtype if it is a script */
		}
		default: {
                        printf ("unhandled Event :%c: - get code in here\n",nodetype);
			return FALSE;
		}
	}

	if (scripttype) {
	    /* this is a Javascript route, so... */
	    if (MultiElement) {
		switch (nodetype)
		{
		  case EAI_MFVEC3F:
		  case EAI_MFCOLOR:
		  case EAI_MFFLOAT: {
		      #ifdef EAIVERBOSE
			printf("EAI_SendEvent, elem %i, count %i, nodeptr %i, off %i, ptr \"%s\".\n",len, elemCount, (int)nodeptr,(int)offset,ptr);
			#endif

		      set_EAI_MFElementtype ((int)nodeptr, (int)offset, (unsigned char *)myBuffer, len);
		      break;
		  }
		  case EAI_SFVEC2F   :
		  case EAI_SFVEC3F   :
		  case EAI_SFCOLOR   :
		  case EAI_SFROTATION: {
		      Set_one_MultiElementtype ((int)nodeptr, (int)offset,
						myBuffer,len);
		      break;
		  }
		}
	    }else {
		set_one_ECMAtype((int)nodeptr,(int)offset,
				 nodetype-EAI_SFUNKNOWN, myBuffer,len);
	    }
	    mark_script((int)nodeptr);
	} else {
		/* now, do the memory copy */
		/* if we have a positive len, then, do a straight copy */

		if (len > 0) {
			SetMemory(nodetype-EAI_SFUNKNOWN,(void *)memptr,(void *)myBuffer,len);
		} else {
			/* if len < 0, it is "wierd". See ScanValtoBuffer
			 * for accurate return values. */
			if (len == -1) {
				/*printf ("EAI_MFSTRING copy over \n");*/
				getEAI_MFStringtype ((struct Multi_String *)myBuffer,
							(struct Multi_String *)memptr);
			}
		}


		/* if this is a geometry, make it re-render.
		   Some nodes (PROTO interface params w/o IS's)
		   will have an offset of zero, and are thus not
		   "real" nodes, only memory locations
		*/

		if (offset > 0) update_node ((void *)nodeptr);

		/* if anything uses this for routing, tell it that it has changed */
		mark_event ((void *)nodeptr,offset);
	}
	return TRUE;
}



/* ========================================================================== */
/* AD */

void EAI_RNewW(char *bufptr) {

	unsigned oldlen;
	struct   VRML_Group *rn;
	struct   Multi_Node *par;
	char     *pstr;

	rn = (struct VRML_Group *) rootNode;
	par = &(rn->children);

	#ifdef EAIVERBOSE 
	printf ("EAI_RNewW, rootNode is %d\n",rootNode);
	#endif


	/* oldlen = what was there in the first place */
	oldlen = par->n;

	#ifdef EAIVERBOSE 
	printf ("oldRoot has %d nodes\n",oldlen);
	#endif


	/* make the old root have ZERO nodes  -well, leave the initial Group {}*/
	par->n = 1;

	/* trick to put only the path name only */
	pstr = bufptr;
	while('/' != *pstr) {pstr ++; bufptr++;}
	while(!isspace(*pstr)) pstr ++;
	*pstr = 0;
	#ifdef EAIVERBOSE 
	printf ("New bufptr <%s>\n",bufptr);
	#endif


	EAI_readNewWorld(bufptr);

	#ifdef EAIVERBOSE 
	printf ("EAI_RNewW, rootNode now is %d\n",rootNode);
	#endif

}

/* ========================================================================== */

/* EAI, replaceWorld. */
void EAI_RW(char *str) {

	int oldlen;
	char *newNode;
	struct VRML_Group *rn;
	struct Multi_Node *par;
	char *tmp;

	int i;

	rn = (struct VRML_Group *) rootNode;
	par = &(rn->children);
	tmp = (char *) rootNode;
	tmp += offsetof (struct VRML_Group, children);

	#ifdef EAIVERBOSE 
	printf ("EAIRW, rootNode is %d\n",rootNode);
	#endif


	/* oldlen = what was there in the first place */
	oldlen = par->n;

	#ifdef EAIVERBOSE 
	printf ("oldRoot has %d nodes\n",oldlen);
	#endif


	/* make the old root have ZERO nodes  -well, leave the initial Group {}*/
	par->n = 1;

	/* go through the string, and send the nodes into the rootnode */
	/* first, remove the command, and get to the beginning of node */
	while ((*str != ' ') && (strlen(str) > 0)) str++;
	while (isspace(*str)) str++;
	while (strlen(str) > 0) {
		i = sscanf (str, "%u",&newNode);
		if (i>0) addToNode (tmp,newNode);

		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
}
