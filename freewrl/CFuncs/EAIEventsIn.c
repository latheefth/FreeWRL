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
#include "Viewer.h"
#include <sys/time.h>

/* include socket.h for irix and apple */
#ifndef LINUX
#include <sys/socket.h>
#endif

#include "EAIheaders.h"

/* used for loadURL */
struct X3D_Anchor EAI_AnchorNode;
int waiting_for_anchor = FALSE;

/* used for reading in SVs */
SV *sv_global_tmp;

void createLoadURL(char *bufptr);
void makeFIELDDEFret(uintptr_t,char *buf,int c);

/* get how many bytes in the type */
int returnElementLength(int type) {
	  switch (type) {
		case SFTIME :
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
		case SFCOLORRGBA:
		case MFCOLORRGBA:
			return 4;
	}
	return 1;

}

/* copy new scanned in data over to the memory area in the scene graph. */

uintptr_t Multi_Struct_memptr (int type, void *memptr) {
	struct Multi_Vec3f *mp;
	uintptr_t retval;

	/* is this a straight copy, or do we have a struct to send to? */
	/* now, some internal reps use a structure defined as:
	   struct Multi_Vec3f { int n; struct SFColor  *p; };
	   so, we have to put the data in the p pointer, so as to
	   not overwrite the data. */

	retval = (uintptr_t) memptr;

	switch (type) {
		case MFVEC2F: 
		case MFCOLOR: 
		case MFCOLORRGBA: 
		case MFVEC3F: 
		case MFROTATION: 
		case MFFLOAT:
		case MFINT32:
			mp = (struct Multi_Vec3f*) memptr;
			retval = (uintptr_t) (mp->p);

		default: {}
		}
	return retval;
}


/* change a memory loction - do we need to do a simple copy, or
do we need to change a Structure type? */

void SetMemory (int type, void *destptr, void *srcptr, int len) {
	void *newptr;
	struct Multi_Vec3f *mp;
	SV *svptr;


	/* is this a structure? If Multi_Struct_memptr returns a different
	   pointer, than it IS a struct {int n void *p} structure type. */

	/* printf ("start of SetMemory, len %d type %d\n",len,type);  */

	newptr = (void *)Multi_Struct_memptr(type, destptr);
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

		/* is this a straight copy, or a setting of an SV * ? */
		if ((type == SFSTRING)  || (type == SFIMAGE)) {
			/* printf ("SetMemory - this is an SV *\n"); */
                        svptr = (SV *)destptr;
			/* printf ("dest was a SV of type %x at %x\n",SvTYPE((SV *)svptr->sv_any),svptr->sv_any); */
			svptr->sv_any = sv_global_tmp;
			/* printf ("dest now is a SV of type %xat %x\n",SvTYPE((SV *)svptr->sv_any),svptr->sv_any); */
			
		} else {
			/* this is a straight copy */
			memcpy (destptr, srcptr, len);
		}
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
	int retint;	/* used for getting sscanf return val */
	
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

	    case SFNODE:
	    case SFINT32: {
	    	retint=sscanf (buf,"%d",(int *)memptr);
		len = sizeof (int);
	    	break;
	    }
	    case SFFLOAT: {
	    	retint=sscanf (buf,"%f",(float *)memptr);
		len = sizeof (float);
	    	break;
	    }

	    case SFVEC2F: {	/* SFVec2f */
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f",&flmem[0], &flmem[1]);
		len = sizeof(float) * 2;
	    	break;
	    }

	    case SFVEC3F:
	    case SFCOLOR: {	/* SFColor */
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f %f",&flmem[0],&flmem[1],&flmem[2]);
		len = sizeof(float) * 3;
	    	break;
	    }

	    case SFCOLORRGBA:
	    case SFROTATION: {
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f %f %f",&flmem[0],&flmem[1],&flmem[2],&flmem[3]);
		len = sizeof(float) * 4;
	    	break;
	    }

	    case SFTIME: {
		retint=sscanf (buf, "%lf", (double *)memptr);
		len = sizeof(double);
		break;
	    }

	    case MFNODE:
	    case MFINT32:
	    case MFTIME:
	    case MFCOLOR:
	    case MFCOLORRGBA:
	    case MFVEC3F:
	    case MFFLOAT:
	    case MFROTATION:
	    case MFVEC2F: {
		/* first number is the "number of numbers". */

		/* scan to start of element count, and read it in.*/
		while (*buf==' ') buf++; 
		retint=sscanf (buf,"%d",quant); while (*buf>' ') buf++;

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
			if ((type==MFINT32) || (type==MFNODE)) {
				retint=sscanf (buf,"%d",ip);
			} else { 
				retint=sscanf (buf,"%f",fp);
			}

			/* go to next number */
			while (*buf >' ') buf++;
			fp ++; ip++;
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


		/* return a Multi_String.						*/
		/*  struct Multi_String { int n; SV * *p; };				*/
		/*  buf will look like:							*/
		/*  	2  0;9:wordg.png  1;12:My"wordg.png				*/
		/*  	where 2 = max elements; 0;9 is element 0, 9 chars long...	*/
		/* OR, if a set1Value:							*/
		/* 	  -1 2:10:xxxxxxxxx						*/
		/* 	  where the 2 is the index.					*/

		strptr = (struct Multi_String *)memptr;

		/* scan to start of element count, and read it in.*/
		while (*buf==' ') buf++;
		retint=sscanf (buf,"%d",&maxele);
		while (*buf!=' ') buf++;

		/* is this a set1Value, or a setValue */
		if (maxele == -1) {
			/* is the range ok? for set1Value, we only replace, do not expand. */
			while (*buf==' ') buf++;
			retint=sscanf (buf,"%d;%d",&thisele,&thissize);
			/* printf ("this element %d has len %d MFStr size %d \n",thisele,thissize, strptr->n); */

			if (maxele < strptr->n) {
				/* scan to start of string*/
				while (*buf!=':') buf++; buf++;

				/* replace the space at stringln with a 0 */
				buf += thissize; *buf = 0; buf-=thissize;
				strptr->p[thisele] = EAI_newSVpv(buf);

				/* go to end of string */
				buf += thissize+1;
			} else {
				printf ("EAI - warning, MFString set1Value, set %d out of range for array (0-%d)\n",
					maxele,strptr->n);
			}
		} else {	
			/* make (and initialize) this MFString internal representation.*/
			strptr->n = maxele;
			FREE_IF_NZ (strptr->p);

			strptr->p = (SV**)malloc (maxele * sizeof(strptr->p));
			newp = strptr->p;
	
			/* scan through EAI string, extract strings, etc, etc.*/
			do {
				/* scan to start of element number*/
				/* make the new SV */
	
				while (*buf==' ') buf++;
				retint=sscanf (buf,"%d;%d",&thisele,&thissize);
				/*printf ("this element %d has size %d\n",thisele,thissize);*/
	
				/* scan to start of string*/
				while (*buf!=':') buf++; buf++;
	
				/* replace the space at stringln with a 0 */
				buf += thissize; *buf = 0; buf-=thissize;
				strptr->p[thisele] = EAI_newSVpv(buf);

				/* go to end of string */
				buf += thissize+1;
	
				/* scan to next start of string, or end of line*/
				while (*buf==' ') buf++;
	
				/* point to next SV to fill*/
				newp++;
			} while (((int)*buf)>=32);
		}
		/*len = maxele*sizeof(struct Multi_String);*/
		/* return -1 to indicate that this is "wierd".*/
		len = -1;

		break;
	   }

	case SFIMAGE:
	case SFSTRING: {
		int thissize;

		/* save this stuff to a global SV, rather than worrying about memory pointers */
		#ifdef EAIVERBOSE
		printf ("ScanValtoBuffer: SFSTRING, string is %s, ptr %x %d\n",buf,memptr,memptr);
		#endif

		/* strings in the format "25:2 2 1 0xff 0x80 0x80 0xff" where 25 is the length */
		while (*buf == ' ') buf++;

		retint=sscanf (buf,"%d",&thissize);
		/* printf ("this SFStr size %d \n",thissize);  */

		/* scan to start of string*/
		while (*buf!=':') buf++; buf++;

		/* replace the space at stringln with a 0 */
		buf += thissize; 
		*buf=0;
		buf -= thissize;
                sv_global_tmp = EAI_newSVpv(buf);

		/* return char to a space */
		buf += thissize;
		*buf=' ';

		/* printf ("ScanValtoBuffer, svptr is now of type %x\n",SvTYPE(sv_global_tmp)); */
		/*len = maxele*sizeof(struct Multi_String);*/
		len = sizeof (void *);
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
	uintptr_t ra,rb,rc,rd;	/* temps*/

	unsigned int scripttype;
	char *EOT;		/* ptr to End of Text marker*/
	int retint;		/* used for getting retval for sscanf */
	int flag;



	while (strlen(bufptr)> 0) {
		#ifdef EAIVERBOSE
		printf ("EAI_parse_commands:start of while loop, strlen %d str :%s:\n",strlen(bufptr),bufptr);
		#endif

		/* step 1, get the command sequence number */
		if (sscanf (bufptr,"%d",&count) != 1) {
			printf ("EAI_parse_commands, expected a sequence number on command :%s:\n",bufptr);
			count = 0;
		}
		#ifdef EAIVERBOSE
		printf ("EAI - seq number %d\n",count);
		#endif

		/* step 2, skip past the sequence number */
		while (isdigit(*bufptr)) bufptr++;
		#ifdef EAIVERBOSE
		printf("past sequence number, string:%s\n",bufptr);
		#endif

		while (*bufptr == ' ') bufptr++;
		#ifdef EAIVERBOSE
		printf ("past the space, string:%s\n",bufptr);
		#endif

		/* step 3, get the command */

		command = *bufptr;
		#ifdef EAIVERBOSE
		printf ("command %c\n",command);
		#endif
		bufptr++;

		/* return is something like: $hand->print("RE\n$reqid\n1\n$id\n");*/

		#ifdef EAIVERBOSE 
		printf ("\n... %d ",count);
		#endif

		switch (command) {
			case GETRENDPROP: {
				#ifdef EAIVERBOSE 
				printf ("GETRENDPROP\n");
				#endif

				/* is MultiTexture initialized yet? */
				if (maxTexelUnits < 0) init_multitexture_handling();

				sprintf (buf,"RE\n%f\n%d\n%s %dx%d %d %s %d %f",TickTime,count,
					"SMOOTH",				/* Shading */
					global_texSize, global_texSize, 	/* Texture size */	
					maxTexelUnits,				/* texture units */
					"FALSE",				/* antialiased? */
					displayDepth,				/* bit depth of display */
					256.0					/* amount of memory left on card -
										   can not find this in OpenGL, so
										   just make it large... */
					);
				break;
				}

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
			case GETENCODING: {
				#ifdef EAIVERBOSE 
				printf ("GETENCODING\n");
				#endif
				sprintf (buf,"RE\n%f\n%d\n%d",TickTime,count,currentFileVersion);
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

				retint=sscanf (bufptr," %s",ctmp);
				#ifdef EAIVERBOSE 
				printf ("GETNODE %s\n",ctmp);
				#endif

				/* is this the SAI asking for the root node? */
				if (strncmp(ctmp,SYSTEMROOTNODE,strlen(SYSTEMROOTNODE))) {
					uretval = EAI_GetNode(ctmp);
				} else {
					uretval = rootNode;
				}

				sprintf (buf,"RE\n%f\n%d\n%d",TickTime,count,uretval);
				break;
			}
			case GETTYPE:  {
				/*format int seq# COMMAND  int node#   string fieldname   string direction*/

				retint=sscanf (bufptr,"%d %s %s",&uretval,ctmp,dtmp);
				#ifdef EAIVERBOSE 
				printf ("GETTYPE NODE%d %s %s\n",uretval, ctmp, dtmp);
				#endif

				/* special case for now - handle only rootNodes here */
				if (uretval == rootNode) {

					/* printf ("GETTYPE =  have root node to compare against\n"); */
					ra = rootNode;
					rb = 0;
					if (strncmp (ctmp,"addChildren",strlen("addChildren")) == 0) rb = offsetof (struct X3D_Group, children);
					if (strncmp (ctmp,"removeChildren",strlen("removeChildren")) == 0) rb = offsetof (struct X3D_Group, children);
					if (strncmp (ctmp,"children",strlen("children")) == 0) rb = offsetof (struct X3D_Group, children);
					if (strncmp (ctmp,"bboxSize",strlen("bboxSize")) == 0) rb = offsetof (struct X3D_Group, bboxSize);
					if (strncmp (ctmp,"bboxCenter",strlen("bboxCenter")) == 0) rb = offsetof (struct X3D_Group, bboxCenter);

					if (rb == 0) printf ("GETTYPE for rootNode = unknown field %s\n",ctmp);
			
					rc = 0;
					rd = 113;
					scripttype = 0;
				} else {
					EAI_GetType (uretval,ctmp,dtmp,(int *)&ra,(int *)&rb,(int *)&rc,(int *)&rd,(int *)&scripttype);
				}

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
					/* sanitize this string - remove leading and trailing garbage */
					ra = 0; rb = 0;
					while ((ra < strlen(bufptr)) && (bufptr[ra] <= ' ')) ra++;
					while (bufptr[ra] > ' ') { ctmp[rb] = bufptr[ra]; rb ++; ra++; }

					/* ok, lets make a real name from this; maybe it is local to us? */
					ctmp[rb] = 0;
					dtmp[0] = 0;
					makeAbsoluteFileName (dtmp, "./",ctmp);

					#ifdef EAIVERBOSE 
					printf ("CREATEVU %s\n",dtmp);
					#endif
					ra = EAI_CreateVrml("URL",dtmp,nodarr,200);
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

				retint=sscanf (bufptr,"%d %d %s %s",&ra,&rb,ctmp,dtmp);
				rc = ra+rb; /* final pointer- should point to a Multi_Node*/

				#ifdef EAIVERBOSE 
				printf ("SENDCHILD Parent: %d ParentField: %d %s Child: %s\n",ra, rb, ctmp, dtmp);
				#endif

				/* add (1), remove (2) or replace (0) for the add/remove/set flag. But,
				   we only have addChildren or removeChildren, so flag can be 1 or 2 only */
				if (strcmp(ctmp,"removeChildren")==0) { flag = 2;} else {flag = 1;}

				getMFNodetype (dtmp,(struct Multi_Node *)rc,
						(struct X3D_Box *)ra, flag);

				/* tell the routing table that this node is updated - used for RegisterListeners */
				mark_event((void *)ra,rb);

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}
			case UPDATEROUTING :  {
				/*format int seq# COMMAND  int node#   ParentNode field ChildNode*/

				retint=sscanf (bufptr,"%d %d %s %d",&ra,&rb,ctmp,&rc);
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
				retint=sscanf (bufptr,"%d %d %c %d",&ra,&rb,ctmp,&rc);
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

			case UNREGLISTENER: {
				#ifdef EAIVERBOSE 
				printf ("UNREGISTERLISTENER %s \n",bufptr);
				#endif

				/*143024848 88 8 e 6*/
				retint=sscanf (bufptr,"%d %d %c %d",&ra,&rb,ctmp,&rc);
				/* so, count = query id, ra pointer, rb, offset, ctmp[0] type, rc, length*/
				ctmp[1]=0;

				/* printf ("UNREGISTERLISTENER from %d foffset %d fieldlen %d type %s \n",
						ra, rb,rc,ctmp); */


				/* put the address of the listener area in a string format for registering
				   the route - the route propagation will copy data to here */
				sprintf (EAIListenerArea,"%d:0",(int)&EAIListenerData);

				/* set up the route from this variable to the handle_Listener routine */
				CRoutes_Register  (0,(void *)ra,(int)rb, 1, EAIListenerArea, (int) rc,(void *) &handle_Listener, 0, (count<<8)+ctmp[0]); /* encode id and type here*/

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}

			case GETVALUE: {
				#ifdef EAIVERBOSE 
				printf ("GETVALUE %s \n",bufptr);
				#endif


				/* format: ptr, offset, type, length (bytes)*/
				retint=sscanf (bufptr, "%d %d %c %d", &ra,&rb,ctmp,&rc);

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

			case GETPROTODECL:  {
				#ifdef EAIVERBOSE 
				/* printf ("SAI SV ret command .%s\n",bufptr); */
				#endif
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,SAI_StrRetCommand ((char) command,bufptr));
				break;
				}
			case REMPROTODECL: 
			case UPDPROTODECL: 
			case UPDNAMEDNODE: 
			case REMNAMEDNODE: 
			case ADDROUTE:
			case DELETEROUTE:  {
				#ifdef EAIVERBOSE 
				printf ("SV int ret command ..%s\n",bufptr);
				#endif
				sprintf (buf,"RE\n%f\n%d\n%d",TickTime,count,
					SAI_IntRetCommand ((char) command,bufptr));
				break;
				}

		  	case STOPFREEWRL: {
				#ifdef EAIVERBOSE 
				printf ("Shutting down Freewrl\n");
				#endif
				if (!RUNNINGASPLUGIN) {
					doQuit();
				    break;
				}
			    }
			  case VIEWPOINT: {
				#ifdef EAIVERBOSE 
				printf ("Viewpoint :%s:\n",bufptr);
				#endif
				/* do the viewpoints. Note the spaces in the strings */
				if (!strncmp(bufptr, " NEXT", strlen (" NEXT"))) Next_ViewPoint();
				if (!strncmp(bufptr, " FIRST", strlen (" FIRST"))) First_ViewPoint();
				if (!strncmp(bufptr, " LAST", strlen (" LAST"))) Last_ViewPoint();
				if (!strncmp(bufptr, " PREV", strlen (" PREV"))) Prev_ViewPoint();

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
			    }

			case LOADURL: {
				#ifdef EAIVERBOSE
				printf ("loadURL %s\n",bufptr);
				#endif

				/* signal that we want to send the Anchor pass/fail to the EAI code */
				waiting_for_anchor = TRUE;

				/* make up the URL from what we currently know */
				createLoadURL(bufptr);


				/* prep the reply... */
				sprintf (buf,"RE\n%f\n%d\n",TickTime,count);

				/* now tell the EventLoop that BrowserAction is requested... */
				AnchorsAnchor = &EAI_AnchorNode;
				BrowserAction = TRUE;
				break;
				}

			case CREATEPROTO: 
			case CREATENODE: {
				/* sanitize this string - remove leading and trailing garbage */
				ra = 0; rb = 0;
				while ((ra < strlen(bufptr)) && (bufptr[ra] <= ' ')) ra++;
				while (bufptr[ra] > ' ') { ctmp[rb] = bufptr[ra]; rb ++; ra++; }

				ctmp[rb] = 0;
				#ifdef EAIVERBOSE 
				printf ("CREATENODE/PROTO %s\n",ctmp);
				#endif
				if (command == CREATENODE) 
					ra = EAI_CreateVrml("CREATENODE",ctmp,nodarr,200); 
				else if (command == CREATEPROTO)
					ra = EAI_CreateVrml("CREATEPROTO",ctmp,nodarr,200); 
				else 
					printf ("eai - huh????\n");

				sprintf (buf,"RE\n%f\n%d\n",TickTime,count);

				for (rb = 0; rb < ra; rb++) {
					sprintf (ctmp,"%d ", nodarr[rb]);
					strcat (buf,ctmp);
				}
				break;
				}

			case GETFIELDDEFS: {
				/* get a list of fields of this node */
				sscanf (bufptr,"%d",&ra);

				makeFIELDDEFret(ra,buf,count);

				

				break;
				}

			default: {
				printf ("unhandled command :%c: %d\n",command,command);
				strcat (buf, "unknown_EAI_command");
				break;
				}

			}


		/* send the response - events don't send a reply */
		/* and, Anchors send a different reply (loadURLS) */
		if (command != SENDEVENT) {
			if (command != LOADURL) strcat (buf,"\nRE_EOT");
			EAI_send_string (buf,EAIlistenfd);
		}

		/* skip to the next command */
		while (*bufptr >= ' ') bufptr++;

		/* skip any new lines that may be there */
		while ((*bufptr == 10) || (*bufptr == 13)) bufptr++;
	}
}


/* for a GetFieldTypes command for a node, we return a string giving the field types */

void makeFIELDDEFret(uintptr_t myptr, char *buf, int repno) {
	struct X3D_Box *boxptr;
	int myc;
	int a,b,c;
	int *np;
	char myline[200];

	boxptr = (struct X3D_Box *) myptr;
	printf ("GETFIELDDEFS, node %d\n",boxptr);
	printf ("node type is %s\n",stringNodeType(boxptr->_nodeType));


	/* how many fields in this node? */
	np = NODE_OFFSETS[boxptr->_nodeType];
	myc = 0;
	while (*np != -1) {
		/* is this a hidden field? */

		if (strncmp (FIELDNAMES[*np],"_",1) != 0) {
			myc ++; 
		}

		np +=4;

	}

	sprintf (buf,"RE\n%f\n%d\n",TickTime,repno);

	sprintf (myline, "%d ",myc);
	strcat (buf, myline);

	/* now go through and get the name, type, keyword */
	np = NODE_OFFSETS[boxptr->_nodeType];
	for (a = 0; a < myc; a++) {
		if (strncmp (FIELDNAMES[*np],"_",1) != 0) {
			sprintf (myline,"%s %c %s ",FIELDNAMES[np[0]], EAIFIELD_TYPE_STRING(np[2]), 
				KEYWORDS[np[3]]);
			strcat (buf, myline);
		}
		np += 4;
	}
	strcat (buf, myline);
}

/* an incoming EAI/CLASS event has come in, convert the ASCII characters
 * to an internal representation, and act upon it */

unsigned int EAI_SendEvent (char *ptr) {
	unsigned char nodetype;
	uintptr_t nodeptr;
	uintptr_t offset;
	unsigned int scripttype;

	uintptr_t memptr;

	int valIndex;
	struct Multi_Color *tcol;

	int len, elemCount;
	int MultiElement;
	int retint; 			/* used to get return value of sscanf */
	char myBuffer[6000];

	/* we have an event, get the data properly scanned in from the ASCII string, and then
		friggin do it! ;-) */

	/* node type */
	nodetype = *ptr; ptr++;

	/* blank space */
	ptr++;

	/* nodeptr, offset */
	retint=sscanf (ptr, "%d %d %d",&nodeptr, &offset, &scripttype);
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
	printf ("EAI_SendEvent, event string now is :%s:\n",ptr);
	#endif

	/* is this a MF node, that has floats or ints, and the set1Value method is called? 	*/
	/* check out the java external/field/MF*java files for the string "ONEVAL "		*/
	if (strncmp("ONEVAL ",ptr,7) == 0) {
		ptr += 7;

		/* find out which element the user wants to set - that should be the next number */
		while (*ptr==' ')ptr++;
		retint=sscanf (ptr,"%d",&valIndex);
		while (*ptr>' ')ptr++; /* past the number */
		while (*ptr==' ')ptr++;

		/* lets do some bounds checking here. */
		tcol = (struct Multi_Color *) memptr;
		if (tcol->n <= valIndex) {
			printf ("Error, setting 1Value of %d, max in scenegraph is %d\n",valIndex,tcol->n);
			return FALSE;
		}


		/* if this is a struct Multi* node type, move the actual memory pointer to the data */
		memptr = Multi_Struct_memptr(nodetype-EAI_SFUNKNOWN, (void *) memptr);

		/* and index into that array; we have the index, and sizes to worry about 	*/
		memptr += valIndex * returnElementLength(nodetype-EAI_SFUNKNOWN) *  returnElementRowSize(nodetype-EAI_SFUNKNOWN);

		/* and change the nodetype to reflect this change */
		switch (nodetype) {
			case EAI_MFNODE: nodetype = EAI_SFNODE; break;
			case EAI_MFINT32: nodetype = EAI_SFINT32; break;
			case EAI_MFTIME: nodetype = EAI_SFTIME; break;
			case EAI_MFCOLOR: nodetype = EAI_SFCOLOR; break;
			case EAI_MFCOLORRGBA: nodetype = EAI_SFCOLORRGBA; break;
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
	  	case EAI_SFCOLORRGBA:
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
	        case EAI_MFCOLORRGBA   :
	        case EAI_MFFLOAT   : {
		    MultiElement=TRUE;
		   break;
		}
		case EAI_MFSTRING: {
			/* myBuffer will have a full SV structure now, and len will*/
			/* be -1.*/
			break;
		}
		
		case EAI_SFIMAGE:
		case EAI_SFSTRING: {
			break;
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
		  case EAI_MFROTATION:
		  case EAI_MFCOLOR:
		  case EAI_MFCOLORRGBA:
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
		  case EAI_SFCOLORRGBA   :
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
#undef EAIVERBOSE

}


/* EAI, replaceWorld. */
void EAI_RW(char *str) {
	char *newNode;
	int i;
	char *tmp;

	/* clean the slate! keep EAI running, though */
	kill_oldWorld(FALSE,TRUE,TRUE);

	tmp = (char *) rootNode;

	/* go through the string, and send the nodes into the rootnode */
	/* first, remove the command, and get to the beginning of node */
	while ((*str != ' ') && (strlen(str) > 0)) str++;
	while (isspace(*str)) str++;
	while (strlen(str) > 0) {
		i = sscanf (str, "%u",&newNode);
		if (i>0) addToNode (tmp,offsetof (struct X3D_Group, children),newNode);

		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
}


void createLoadURL(char *bufptr) {
	#define strbrk " :loadURLStringBreak:"
	int count;
	char newstring[2000];
	int np;
	char *spbrk;
	int retint;		/* used to get retval from sscanf */


	/* fill in Anchor parameters */
	EAI_AnchorNode.description = EAI_newSVpv("From EAI");

	/* fill in length fields from string */
	while (*bufptr==' ') bufptr++;
	retint=sscanf (bufptr,"%d",&EAI_AnchorNode.url.n);
	while (*bufptr>' ') bufptr++;
	while (*bufptr==' ') bufptr++;
	retint=sscanf (bufptr,"%d",&EAI_AnchorNode.parameter.n);
	while (*bufptr>' ') bufptr++;
	while (*bufptr==' ') bufptr++;

	/* now, we should be at the strings. */
	bufptr--;

	/* malloc the sizes required */
	if (EAI_AnchorNode.url.n > 0) EAI_AnchorNode.url.p = malloc(EAI_AnchorNode.url.n * sizeof (struct sv));
	if (EAI_AnchorNode.parameter.n > 0) EAI_AnchorNode.parameter.p = malloc(EAI_AnchorNode.parameter.n * sizeof (struct sv));

	for (count=0; count<EAI_AnchorNode.url.n; count++) {
		bufptr += strlen(strbrk);
		/* printf ("scanning, at :%s:\n",bufptr); */
		
		/* nullify the next "strbrk" */
		spbrk = strstr(bufptr,strbrk);
		if (spbrk!=NULL) *spbrk='\0';

		EAI_AnchorNode.url.p[count] = EAI_newSVpv(bufptr);

		if (spbrk!=NULL) bufptr = spbrk;
	}
	for (count=0; count<EAI_AnchorNode.parameter.n; count++) {
		bufptr += strlen(strbrk);
		/* printf ("scanning, at :%s:\n",bufptr); */
		
		/* nullify the next "strbrk" */
		spbrk = strstr(bufptr,strbrk);
		if (spbrk!=NULL) *spbrk='\0';

		EAI_AnchorNode.parameter.p[count] = EAI_newSVpv(bufptr);

		if (spbrk!=NULL) bufptr = spbrk;
	}
	EAI_AnchorNode.__parenturl = EAI_newSVpv("./");
}



/* mimic making newSVpv, but *not* using Perl, as this is a different thread */
/* see Extending and Embedding Perl, Jenness, Cozens pg 75-77 */
SV *EAI_newSVpv(char *str) {
	SV *retval;
	struct xpv *newpv;

	#ifdef EAIVERBOSE
	printf ("EAI_newSVpv for :%s:\n",str);
	#endif

	/* the returning SV is here. Make blank struct */
	retval = malloc (sizeof (struct sv));
	newpv = malloc (sizeof (struct xpv));

	retval->sv_any = newpv;
	retval->sv_refcnt = 1;
	retval->sv_flags = SVt_PV | SVf_POK;

	newpv->xpv_pv = malloc (strlen (str)+1);
	strncpy(newpv->xpv_pv,str,strlen(str)+1);
	newpv->xpv_cur = strlen(str);
	newpv->xpv_len = strlen(str)+1;

	return retval;
}

/* if we have a LOADURL command (loadURL in java-speak) we call Anchor code to do this.
   here we tell the EAI code of the success/fail of an anchor call, IF the EAI is 
   expecting such a call */

void EAI_Anchor_Response (int resp) {
	char myline[1000];
	if (waiting_for_anchor) {
		if (resp) strcpy (myline,"OK\nRE_EOT");
		else strcpy (myline,"FAIL\nRE_EOT");
		EAI_send_string (myline,EAIlistenfd);
	}
	waiting_for_anchor = FALSE;
}
