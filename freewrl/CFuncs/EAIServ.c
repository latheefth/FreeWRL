/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003, 2005 John Stewart, Ayla Khan CRC Canada
 Portions Copyright (C) 1998 John Breen
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/************************************************************************/
/*									*/
/* implement EAI server functionality for FreeWRL.			*/
/*									*/
/* Design notes:							*/
/*	FreeWRL is a server, the Java (or whatever) program is a client	*/
/*									*/
/*	Commands come in, and get answered to, except for sendEvents;	*/
/*	for these there is no response (makes system faster)		*/
/*									*/
/*	Nodes that are registered for listening to, send async		*/
/*	messages.							*/
/*									*/
/*	very simple example:						*/
/*		move a transform; Java code:				*/
/*									*/
/*		EventInMFNode addChildren;				*/
/*		EventInSFVec3f newpos;					*/
/*		try { root = browser.getNode("ROOT"); }			*/
/*		catch (InvalidNodeException e) { ... }			*/
/*									*/
/*		newpos=(EventInSFVec3f)root.getEventIn("translation");	*/
/*		val[0] = 1.0; val[1] = 1.0; val[2] = 1.0;		*/
/*		newpos.setValue(val);					*/
/*									*/
/*		Three EAI commands sent:				*/
/*			1) GetNode ROOT					*/
/*				returns a node identifier		*/
/*			2) GetType (nodeID) translation			*/
/*				returns posn in memory, length,		*/
/*				and data type				*/
/*									*/
/*			3) SendEvent posn-in-memory, len, data		*/
/*				returns nothing - assumed to work.	*/
/*									*/
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

int EAIport = 9877;				/* port we are connecting to*/
int EAIinitialized = FALSE;		/* are we running?*/
int EAIfailed = FALSE;			/* did we not succeed in opening interface?*/
char EAIListenerData[EAIREADSIZE]; /* this is the location for getting Listenered data back again.*/
char EAIListenerArea[40];

/* socket stuff */
int 	EAIsockfd = -1;			/* main TCP socket fd*/
int	EAIlistenfd = -1;			/* listen to this one for an incoming connection*/
fd_set rfds2;
struct timeval tv2;


struct sockaddr_in	servaddr, cliaddr;
unsigned char loopFlags = 0;

enum theLoopFlags {
		NO_CLIENT_CONNECTED = 0x1,
		NO_EAI_CLASS	    = 0x2,
		NO_RETVAL_CHANGE    = 0x4
};

/* EAI input buffer */
char *EAIbuffer;
int EAIbufcount;				/* pointer into buffer*/
int EAIbufsize;				/* current size in bytes of input buffer*/

int EAIwanted = FALSE;                       /* do we want EAI?*/

/* prototypes*/
/*
void EAI_parse_commands (char *stptr);
void EAI_Convert_mem_to_ASCII (int id, char *reptype, int type, char *memptr, char *buf);
void EAI_RW(char *str);
void EAI_RNewW(char *);
extern void Next_ViewPoint();
extern void set_EAI_MFElementtype (int num, int offset, unsigned char *pptr, int len);
*/

/* open the socket connection -  we open as a TCP server, and will find a free socket */
/* EAI will have a socket increment of 0; Java Class invocations will have 1 +	      */
int conEAIorCLASS(int socketincrement, int *EAIsockfd, int *EAIlistenfd) {
	int len;
	const int on=1;
	int flags;

        struct sockaddr_in      servaddr;

	if ((EAIfailed) &&(socketincrement==0)) return FALSE;

	if ((*EAIsockfd) < 0) {
		/* step 1  - create socket*/
	        if (((*EAIsockfd) = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf ("EAIServer: socket error\n");
			loopFlags &= ~NO_EAI_CLASS;
			return FALSE;
		}

		setsockopt ((*EAIsockfd), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		if ((flags=fcntl((*EAIsockfd),F_GETFL,0)) < 0) {
			printf ("EAIServer: trouble gettingsocket flags\n");
			loopFlags &= ~NO_EAI_CLASS;
			return FALSE;
		} else {
			flags |= O_NONBLOCK;

			if (fcntl((*EAIsockfd), F_SETFL, flags) < 0) {
				printf ("EAIServer: trouble setting non-blocking socket\n");
				loopFlags &= ~NO_EAI_CLASS;
				return FALSE;
			}
		}

		#ifdef EAIVERBOSE 
		printf ("conEAIorCLASS - socket made\n");
		#endif


		/* step 2 - bind to socket*/
	        bzero(&servaddr, sizeof(servaddr));
	        servaddr.sin_family      = AF_INET;
	        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	        servaddr.sin_port        = htons(EAIport+socketincrement);
		/*printf ("binding to socket %d\n",EAIport+socketincrement);*/

	        while (bind((*EAIsockfd), (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
			loopFlags &= ~NO_EAI_CLASS;
			return FALSE;
		}

		#ifdef EAIVERBOSE 
		printf ("EAISERVER: bound to socket %d\n",EAIBASESOCKET+socketincrement);
		#endif


		/* step 3 - listen*/

	        if (listen((*EAIsockfd), 1024) < 0) {
	                printf ("EAIServer: listen error\n");
			loopFlags &= ~NO_EAI_CLASS;
			return FALSE;
		}
	}

	if (((*EAIsockfd) >=0) && ((*EAIlistenfd)<0)) {
		/* step 4 - accept*/
		len = sizeof(cliaddr);
	        if ( ((*EAIlistenfd) = accept((*EAIsockfd), (struct sockaddr *) &cliaddr, (socklen_t *)&len)) < 0) {
			#ifdef EAIVERBOSE
			if (!(loopFlags&NO_CLIENT_CONNECTED)) {
				printf ("EAISERVER: no client yet\n");
				loopFlags |= NO_CLIENT_CONNECTED;
			}
			#endif

		} else {
			loopFlags &= ~NO_CLIENT_CONNECTED;
			#ifdef EAIVERBOSE
				printf ("EAISERVER: no client yet\n");
			#endif
		}
	}


	/* are we ok, ? */
	if ((*EAIlistenfd) >=0)  {
		/* allocate memory for input buffer */
		EAIbufcount = 0;
		EAIbufsize = 2 * EAIREADSIZE; /* initial size*/
		EAIbuffer = (char *)malloc(EAIbufsize * sizeof (char));
		if (EAIbuffer == 0) {
			printf ("can not malloc memory for input buffer in create_EAI\n");
			loopFlags &= ~NO_EAI_CLASS;
			return FALSE;
		}


		/* zero out the EAIListenerData here, and after every use */
		bzero(&EAIListenerData, sizeof(EAIListenerData));

		/* seems like we are up and running now, and waiting for a command */
		/* and are we using this with EAI? */
		if (socketincrement==0) EAIinitialized = TRUE;
	}
	/* printf ("EAISERVER: conEAIorCLASS returning TRUE\n");*/

	#ifdef EAIVERBOSE
	if ( !(loopFlags&NO_EAI_CLASS)) {
		printf ("EAISERVER: conEAIorCLASS returning TRUE\n");
		loopFlags |= NO_EAI_CLASS;
	}
	#endif

	return TRUE;
}


/* the user has pressed the "q" key */
void shutdown_EAI() {

	#ifdef EAIVERBOSE 
	printf ("shutting down EAI\n");
	#endif

	strcpy (EAIListenerData,"QUIT\n\n\n");
	if (EAIinitialized) {
		EAI_send_string(EAIListenerData,EAIlistenfd);
	}

}
void create_EAI() {
        #ifdef EAIVERBOSE 
	printf ("EAISERVER:create_EAI called\n");
	#endif


	/* already wanted? if so, just return */
	if (EAIwanted) return;

	/* so we know we want EAI */
	EAIwanted = TRUE;

	/* have we already started? */
	if (!EAIinitialized) {
		EAIfailed = !(conEAIorCLASS(0,&EAIsockfd,&EAIlistenfd));
	}
}


/* possibly we have an incoming EAI request from the client */
void handle_EAI () {
	/* do nothing unless we are wanted */
	if (!EAIwanted) return;
	if (!EAIinitialized) {
		EAIfailed = !(conEAIorCLASS(0,&EAIsockfd,&EAIlistenfd));
		return;
	}

	/* have we closed connection? */
	if (EAIlistenfd < 0) return;

	EAIbufcount = 0;

	EAIbuffer = read_EAI_socket(EAIbuffer,&EAIbufcount, &EAIbufsize, &EAIlistenfd);

	/* make this into a C string */
	EAIbuffer[EAIbufcount] = 0;
	#ifdef EAIVERBOSE
		if (EAIbufcount) printf ("handle_EAI-- Data is :%s:\n",EAIbuffer);
	#endif

	/* any command read in? */
	if (EAIbufcount > 1)
		EAI_parse_commands (EAIbuffer);
}


void EAI_send_string(char *str, int lfd){
	unsigned int n;

	/* add a trailing newline */
	strcat (str,"\n");

	#ifdef EAIVERBOSE
		printf ("EAI/CLASS Command returns\n%s(end of command)\n",str);
	#endif

	/*printf ("EAI_send_string, sending :%s:\n",str);*/
	n = write (lfd, str, (unsigned int) strlen(str));
	if (n<strlen(str)) {
		#ifdef EAIVERBOSE
		printf ("write, expected to write %d, actually wrote %d\n",n,strlen(str));
		#endif
	}
	/*printf ("EAI_send_string, wrote %d\n",n);*/
}


/* read in from the socket.   pass in -
	pointer to buffer,
	pointer to buffer index
	pointer to max size,
	pointer to socket to listen to

 	return the char pointer - it may have been realloc'd */


char *read_EAI_socket(char *bf, int *bfct, int *bfsz, int *EAIlistenfd) {
	int retval, oldRetval;

	/*printf ("read_EAI_socket, EAIlistenfd %d buffer addr %d\n",*EAIlistenfd,bf);*/
	retval = FALSE;
	do {
		tv2.tv_sec = 0;
		tv2.tv_usec = 0;
		FD_ZERO(&rfds2);
		FD_SET((*EAIlistenfd), &rfds2);

		oldRetval = retval;
		retval = select((*EAIlistenfd)+1, &rfds2, NULL, NULL, &tv2);
		/*printf ("select retval %d\n",retval);*/

		if (retval != oldRetval) {
			loopFlags &= NO_RETVAL_CHANGE;
		}

		#ifdef EAIVERBOSE
		if (!(loopFlags&NO_RETVAL_CHANGE)) {
			printf ("readEAIsocket--, retval %d\n",retval);
			loopFlags |= NO_RETVAL_CHANGE;
		}
		#endif


		if (retval) {
			retval = read ((*EAIlistenfd), &bf[(*bfct)],EAIREADSIZE);

			if (retval <= 0) {
				#ifdef EAIVERBOSE
					printf ("read_EAI_socket, client is gone! errno %d\n",errno);
				#endif

				/*perror("READ_EAISOCKET");*/
				/* client disappeared*/
				close ((*EAIlistenfd));
				(*EAIlistenfd) = -1;

				/* And, lets just exit FreeWRL*/
				printf ("FreeWRL:EAI socket closed, exiting...\n");
				doQuit();
			}

			#ifdef EAIVERBOSE
			{
			    char tmpBuff1[EAIREADSIZE];
			    strncpy(tmpBuff1,&bf[(*bfct)],retval);
			    tmpBuff1[retval] = '\0';
			    printf ("read in from socket %d bytes, max %d bfct %d cmd <%s>\n",
				    retval,EAIREADSIZE, *bfct,tmpBuff1);/*, &bf[(*bfct)]);*/
			}
			#endif


			(*bfct) += retval;

			if (((*bfsz) - (*bfct)) < 128) {
				printf ("HAVE TO REALLOC INPUT MEMORY\n");
				(*bfsz) += EAIREADSIZE;
				bf = (char *)realloc (bf, (unsigned int) (*bfsz));
			}
		}
	} while (retval);
	return (bf);
}




/********************************************************************
*
* Extra Memory routine - for PROTO interface declarations that are used,
* but are not IS'd. (EAI uses these things!)
*
**********************************************************************/

unsigned EAI_do_ExtraMemory (int size,SV *data,char *type) {
	int val;
	double lval;
	void *memptr;
	int ty;
	float fl[4];
	STRLEN len;

	/* variables for MFStrings */
	struct Multi_String *MSptr;
	struct Multi_Color *MCptr;
	float *SFFloats;
	int *SFints;
	AV *aM;
	AV *subaM;
	SV **bM;
	int iM;
	int lM; 
	int tmpint;
	int numPerRow;
	STRLEN xx;


	memptr = 0;  /* get around a compiler warning */

	/* convert the type string to an internal type */
	ty = convert_typetoInt (type);

 	#ifdef EAIVERBOSE  
		printf ("EAI - extra memory for size %d type %s\n",size,type);
	#endif

	if (size > 0) {
		memptr =malloc ((unsigned)size);
		if (memptr == NULL) {
			printf ("can not allocate memory for PROTO Interface decls\n");
			return 0;
		}
	}

	switch (ty) {
		case SFNODE :
		case SFBOOL:
		case SFINT32: {
				val = SvIV(data);
				memcpy(memptr,&val,(unsigned) size);
				break;
			}
		case SFTIME: {
				lval = SvIV(data); /* is this ok for doubles?*/
				memcpy(memptr,&lval,(unsigned) size);
				break;
			}
		case SFFLOAT: {
				fl[0] = SvNV(data);
				memcpy(memptr,&fl[0],(unsigned) size);
				break;
			}


		case SFSTRING: {
				memptr = malloc(strlen(SvPV(data,len))+1);
				if (memptr == NULL) {
					printf ("can not allocate memory for PROTO Interface decls\n");
					return 0;
				}
				strcpy (memptr, SvPV(data,xx));
				break;
			}
		case SFROTATION:
		case SFCOLOR:
		case SFVEC2F: {
				/* these are the same, different lengths for different types, though. */
				SFFloats = (float *) memptr;
				len = size / (sizeof(float));	/* should be 2 for SFVec2f, 3 for SFVec3F...*/
				#ifdef EAIVERBOSE
				printf ("EAI Extra - size %d len %d\n",size,len);
				#endif

				if(!SvROK(data)) {
					for (iM=0; iM<len; iM++) {
						*SFFloats = 0;
						SFFloats++;
					}
					printf ("EAI_Extra_Memory: Help! SFFloattype without being ref\n");
					return 0;
				} else {
					if(SvTYPE(SvRV(data)) != SVt_PVAV) {
						printf ("EAI_Extra_Memory: Help! SFfloattype without being arrayref\n");
						return 0;
					}
					aM = (AV *) SvRV(data);
					for(iM=0; iM<len; iM++) {
						bM = av_fetch(aM, iM, 1); /* LVal for easiness */
						if(!bM) {
							printf ("EAI_Extra_Memory: Help: SFfloattype b == 0\n");
							return 0;
						}
						*SFFloats = SvNV(*bM);
						SFFloats ++;
					}
				}
				break;
			}

		case MFSTRING: {
			/* malloc the main pointer */
			memptr = malloc (sizeof (struct Multi_String));

			if (memptr == NULL) {
				printf ("can not allocate memory for PROTO Interface decls\n");
				return 0;
			}

			/* set the contents pointer to zero  - mimics alloc_offs_MFString */
			MSptr = (struct Multi_String *)memptr;
        		(*MSptr).n = 0; (*MSptr).p = 0;

			/* now we mimic set_offs_MFString to set these values in C */
			if(!SvROK(data)) {
				(*MSptr).n = 0;
				(*MSptr).p = 0;
				printf ("EAI_Extra_Memory: Help! Multi without being ref\n");
				return 0;
			} else {
				if(SvTYPE(SvRV(data)) != SVt_PVAV) {
					printf ("EAI_Extra_Memory: Help! Multi without being ref\n");
				}
				aM = (AV *) SvRV(data);
				lM = av_len(aM)+1;
				(*MSptr).n = lM;
				(*MSptr).p = (SV**)malloc(lM * sizeof(*((*MSptr).p)));
				for(iM=0; iM<lM; iM++) {
					bM = av_fetch(aM, iM, 1); /* LVal for easiness */
					if(!bM) {
						printf ("EAI_Extra_Memory: Help: Multi VRML::Field::SFString bM == 0\n");
					}
					(*MSptr).p[iM] = newSVpv("",0);
					sv_setsv(((*MSptr).p[iM]),(*bM));
				}
			}
			break;
		}

		case MFINT32: {
			numPerRow=1;
			/* malloc the main pointer */
			memptr = malloc (sizeof (struct Multi_Int32));
			if (memptr == NULL) {
				printf ("can not allocate memory for PROTO Interface decls\n");
				return 0;
			}
			MCptr = (struct Multi_Int32 *) memptr;
			(*MCptr).n = 0;
			(*MCptr).p = 0;
			if(!SvROK(data)) {
				printf ("EAI_Extra_Memory: Help! Multi without being ref\n");
				return 0;
			} 
			if(SvTYPE(SvRV(data)) != SVt_PVAV) {
				printf ("EAI_Extra_Memory: Help! Multi without being ref\n");
			}

			/* printf ("sv_dump on data %x is:\n",data); sv_dump(data);  */

                        aM = (AV *) SvRV(data);
                        lM = av_len(aM)+1;

			/* printf ("sv_dump on aM data %x is:\n",aM); sv_dump(aM);  */

			/* printf ("This MFInt32 has (lM is) %d\n",lM);  */
                        /* XXX Free previous p */
                        (*MCptr).n = lM;
                        (*MCptr).p = (struct SFInt32 *)malloc(lM * numPerRow * sizeof(*((*MCptr).p)));
			SFints = (int *) (*MCptr).p;

			/* printf ("EAI_DO_EXTRA, memptr for floats is %x\n",SFints); */

			/* bM = av_fetch(aM, 0, 1);*/  
			for (iM = 0; iM < lM; iM++) {
				bM = av_fetch(aM, iM, 1); /* LVal for easiness */
				/* printf ("bm is %x iM %d flags %x\n",*bM, iM, SvFLAGS (*bM)); */
				if(!bM) {
					freewrlDie("Help: Multi VRML::Field::SFColor bM == 0");
				}

				/* printf ("type of node is %x\n",SvTYPE(*bM)); sv_dump(*bM); 
				printf ("type of aM %x is %x\n",aM,SvTYPE(aM)); sv_dump(aM);  */


				/* is this a single or double array? */
				/* printf ("rows 1; type of node is %x\n",SvTYPE(*bM)); sv_dump(*bM);  */
				*SFints = SvIV(*bM); SFints++;
				/* printf ("saved int %d as %d\n",iM,SvIV(*bM)); */
			}

			break;
			}

		case MFFLOAT:
		case MFVEC2F:
		case MFROTATION:
		case MFVEC3F:
		case MFCOLOR: {
			/* struct Multi_Color { int n; struct SFColor  *p; }; */
			numPerRow=3;
			if (ty==MFFLOAT) {numPerRow=1;}
			else if (ty==MFVEC2F) {numPerRow=2;}
			else if (ty==MFROTATION) {numPerRow=4;};


			/* malloc the main pointer */
			memptr = malloc (sizeof (struct Multi_Color));
			if (memptr == NULL) {
				printf ("can not allocate memory for PROTO Interface decls\n");
				return 0;
			}
			MCptr = (struct Multi_Color *) memptr;
			(*MCptr).n = 0;
			(*MCptr).p = 0;
			if(!SvROK(data)) {
				printf ("EAI_Extra_Memory: Help! Multi without being ref\n");
				return 0;
			} 
			if(SvTYPE(SvRV(data)) != SVt_PVAV) {
				printf ("EAI_Extra_Memory: Help! Multi without being ref\n");
			}


			/* printf ("sv_dump on data %x is:\n",data); sv_dump(data);  */

                        aM = (AV *) SvRV(data);
                        lM = av_len(aM)+1;

			/* printf ("sv_dump on aM data %x is:\n",aM); sv_dump(aM);  */

			/* printf ("This MFColor has (lM is) %d\n",lM); */
                        /* XXX Free previous p */
                        (*MCptr).n = lM;
                        (*MCptr).p = (struct SFColor *)malloc(lM * numPerRow * sizeof(*((*MCptr).p)));
			SFFloats = (float *) (*MCptr).p;

			/* printf ("EAI_DO_EXTRA, memptr for floats is %x\n",SFFloats); */

			/* bM = av_fetch(aM, 0, 1);*/  
			for (iM = 0; iM < lM; iM++) {
				bM = av_fetch(aM, iM, 1); /* LVal for easiness */
				/* printf ("bm is %x iM %d flags %x\n",*bM, iM, SvFLAGS (*bM)); */
				if(!bM) {
					freewrlDie("Help: Multi VRML::Field::SFColor bM == 0");
				}

				/* printf ("type of node is %x\n",SvTYPE(*bM)); sv_dump(*bM); 
				printf ("type of aM %x is %x\n",aM,SvTYPE(aM)); sv_dump(aM);  */


				/* is this a single or double array? */
				if (numPerRow==1) {
					/* printf ("rows 1; type of node is %x\n",SvTYPE(*bM)); sv_dump(*bM);  */
					*SFFloats = SvNV(*bM); SFFloats++;
				} else {
					if(!SvROK((*bM))) {
						for(tmpint=0; tmpint<numPerRow; tmpint++) {
							*SFFloats = 0; SFFloats++;
						}
					} else {
						if(SvTYPE(SvRV((*bM))) != SVt_PVAV) {
							freewrlDie("Help! SFColor without being arrayref");
						}
						subaM = (AV *) SvRV((*bM));
						for(tmpint=0; tmpint<numPerRow; tmpint++) {
							bM = av_fetch(subaM, tmpint, 1); /* LVal for easiness */
							if(!bM) {
								freewrlDie("Help: SFColor b == 0");
							}
							/* printf ("saving %d %d %f\n",iM,tmpint,SvNV(*bM)); */
							*SFFloats = SvNV(*bM); SFFloats++;
	
						}
					}
				}
			}
 


			 break; 
		}

/*XXX		case MFNODE: { break; }*/
/*XXX		case MFROTATION: { break; }*/
/*XXX		case SFTIME : { break; }*/
/*XXX		case SFIMAGE: { break; }*/
/*XXX		case MFTIME: { break; }*/
		default: {
			printf ("EAI_do_ExtraMemory, unhandled type %s\n",type);
		}
	}
	/* printf ("EAI_Extra memory, returning %d\n",memptr);*/
	return (unsigned) memptr;

}
