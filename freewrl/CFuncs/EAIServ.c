/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003 John Stewart, Ayla Khan CRC Canada
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

#ifdef __APPLE__
#include <sys/socket.h>
#endif

extern char *BrowserName, *BrowserVersion, *BrowserURL; // defined in VRMLC.pm


#define MAXEAIHOSTNAME	255		// length of hostname on command line
#define EAIREADSIZE	2048		// maximum we are allowed to read in from socket
#define EAIBASESOCKET   9877		// socket number to start at


/* these are commands accepted from the EAI client */
#define GETNODE		'A'
#define UPDATEROUTING 	'B'
#define SENDCHILD 	'C'
#define SENDEVENT	'D'
#define GETVALUE	'E'
#define GETTYPE		'F'
#define	REGLISTENER	'G'
#define	ADDROUTE	'H'
#define	DELETEROUTE	'J'
#define GETNAME		'K'
#define	GETVERSION	'L'
#define GETCURSPEED	'M'
#define GETFRAMERATE	'N'
#define	GETURL		'O'
#define	REPLACEWORLD	'P'
#define	LOADURL		'Q'
#define	SETDESCRIPT	'R'
#define CREATEVS	'S'
#define	CREATEVU	'T'
#define	STOPFREEWRL	'U'

/* Subtypes - types of data to get from EAI */
#define	SFUNKNOWN	'a'
#define	SFBOOL		'b'
#define	SFCOLOR		'c'
#define	SFFLOAT		'd'
#define	SFTIME		'e'
#define	SFINT32		'f'
#define	SFSTRING	'g'
#define	SFNODE		'h'
#define	SFROTATION	'i'
#define	SFVEC2F		'j'
#define	SFIMAGE		'k'
#define	MFCOLOR		'l'
#define	MFFLOAT		'm'
#define	MFTIME		'n'
#define	MFINT32		'o'
#define	MFSTRING	'p'
#define	MFNODE		'q'
#define	MFROTATION	'r'
#define	MFVEC2F		's'
#define MFVEC3F		't'
#define SFVEC3F		'u'



int EAIwanted = FALSE;			// do we want EAI?
char EAIhost[MAXEAIHOSTNAME];		// host we are connecting to
int EAIport;				// port we are connecting to
int EAIinitialized = FALSE;		// are we running?
int EAIrecount = 0;			// retry counter for opening socket interface
int EAIfailed = FALSE;			// did we not succeed in opening interface?
int EAIconnectstep = 0;			// where we are in the connect sequence

/* socket stuff */
int 	sockfd = -1;			// main TCP socket fd
int	listenfd = -1;			// listen to this one for an incoming connection

struct sockaddr_in	servaddr, cliaddr;
fd_set rfds;
struct timeval tv;

/* eai connect line */
char *inpline;

/* EAI input buffer */
char *buffer;
int bufcount;				// pointer into buffer
int bufsize;				// current size in bytes of input buffer 


int EAIVerbose = 0;

int EAIsendcount = 0;			// how many commands have been sent back?



// prototypes
void EAI_parse_commands (char *stptr);
unsigned int EAI_SendEvent(char *bufptr);
void EAI_send_string (char *str);
void connect_EAI(void);
void create_EAI(char *eailine);
void handle_EAI(void);
int EAI_GetNode(char *str);		// in VRMLC.pm
void EAI_GetType (unsigned int uretval,
	char *ctmp, char *dtmp,
	int *ra, int *rb,
	int *rc, int *rd);		// in VRMLC.pm
void read_EAI_socket(void);
int EAICreateVrmlFromString(char *str);	// in VRMLC.pm



void EAI_send_string(char *str){
	unsigned int n;

	/* add a trailing newline */
	strcat (str,"\n");

	if (EAIVerbose) printf ("EAI Command returns\n%s",str);

	n = write (listenfd, str, (unsigned int) strlen(str));
	if (n<strlen(str)) {
		printf ("write, expected to write %d, actually wrote %d\n",n,strlen(str));
	}

}

/* open the socket connection -  we open as a TCP server, and will find a free socket */
void connect_EAI() {
	int socketincrement;
	int len;
	const int on=1;
	int flags;

        struct sockaddr_in      servaddr;

	if (EAIfailed) return;

	if (sockfd < 0) {
		// step 1  - create socket
	        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf ("EAIServer: socket error\n");
			EAIfailed=TRUE;
			return;
		}
	
	
		setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
		if ((flags=fcntl(sockfd,F_GETFL,0)) < 0) {
			printf ("EAIServer: trouble gettingsocket flags\n");
			EAIfailed=TRUE;
			return;
		} else {
			flags |= O_NONBLOCK;
	
			if (fcntl(sockfd, F_SETFL, flags) < 0) {
				printf ("EAIServer: trouble setting non-blocking socket\n");
				EAIfailed=TRUE;
				return;
			}
		}
	
		if (EAIVerbose) printf ("connect_EAI - socket made\n");
	
		// step 2 - bind to socket
		socketincrement = 0;
	        bzero(&servaddr, sizeof(servaddr));
	        servaddr.sin_family      = AF_INET;
	        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	        servaddr.sin_port        = htons(EAIBASESOCKET+socketincrement);
	
	        while (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
			//socketincrement++;
			//printf ("error binding to %d, trying %d\n",EAIBASESOCKET+socketincrement-1,
			//	EAIBASESOCKET+socketincrement);
			//servaddr.sin_port        = htons(EAIBASESOCKET+socketincrement);

			// do we really want to ramp up and find a "free" socket? Not in
			// this version, anyway.
			EAIfailed=TRUE;
			return;
		}
	
		if (EAIVerbose) printf ("EAISERVER: bound to socket %d\n",EAIBASESOCKET+socketincrement);
	
		// step 3 - listen
	
	        if (listen(sockfd, 1024) < 0) {
	                printf ("EAIServer: listen error\n");
			EAIfailed=TRUE;
			return;
		}
	}

	if ((sockfd >=0) && (listenfd<0)) {
		// step 4 - accept
		len = sizeof(cliaddr);
	        if ( (listenfd = accept(sockfd, (struct sockaddr *) &cliaddr, &len)) < 0) {
			//printf ("EAIServer: no client yet\n");
		}
	}


	if (listenfd >=0) {
		/* allocate memory for input buffer */
		bufcount = 0;
		bufsize = 2 * EAIREADSIZE; // initial size
		buffer = malloc(bufsize * sizeof (char));
		if (buffer == 0) {
			printf ("can not malloc memory for input buffer in create_EAI\n");
			EAIfailed = TRUE;
			return;
		}
		
		/* seems like we are up and running now, and waiting for a command */
		EAIinitialized = TRUE;	
	}
}

void create_EAI(char *eailine) {
        if (EAIVerbose) printf ("EAISERVER:create_EAI called :%s:\n",eailine);

	/* already wanted? if so, just return */
	if (EAIwanted) return;

	/* so we know we want EAI */
	EAIwanted = TRUE;

	/* copy over the eailine to a local variable */

	// JAS - right now we use localhost, and a base of EAIBASESOCKET, so ignore this line
	//inpline = malloc((strlen (eailine)+1) * sizeof (char));

	//if (inpline == 0) {
	//	printf ("can not malloc memory in create_EAI\n");
	//	EAIwanted = FALSE;
	//	return;
	//}

	//strcpy (inpline,eailine);
	
	/* have we already started? */
	if (!EAIinitialized) {
		connect_EAI();
	}
}

/* read in from the socket.  */
void read_EAI_socket() {
	int retval;

	retval = FALSE;
	do {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(listenfd, &rfds);
	
		retval = select(listenfd+1, &rfds, NULL, NULL, &tv);
	
		if (retval) {
			retval = read (listenfd, &buffer[bufcount],EAIREADSIZE);

			if (retval == 0) {
				// client disappeared
				close (listenfd);
				listenfd = -1;
				EAIinitialized=FALSE;
			}

			if (EAIVerbose) printf ("read in from socket %d , max %d",retval,EAIREADSIZE);

			bufcount += retval;

			if ((bufsize - bufcount) < 10) {
				//printf ("HAVE TO REALLOC INPUT MEMORY\n");
				bufsize += EAIREADSIZE;
				buffer = realloc (buffer, (unsigned int) bufsize);
			}
		}
	} while (retval);
}


/* possibly we have an incoming EAI request from the client */
void handle_EAI () {
	/* do nothing unless we are wanted */
	if (!EAIwanted) return;
	if (!EAIinitialized) {
		connect_EAI();
		return;
	}

	bufcount = 0;
	read_EAI_socket();

	/* make this into a C string */
	buffer[bufcount] = 0;

	/* any command read in? */
	if (bufcount > 1) 
		EAI_parse_commands (buffer);
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
* (eg, SFFLOAT); these sub types are indicated with a lower case letter; again,
* look to the top of this file for the #defines
*
*********************************************************************************/

void EAI_parse_commands (char *bufptr) {
	char buf[EAIREADSIZE];	// return value place
	char ctmp[EAIREADSIZE];	// temporary character buffer
	char dtmp[EAIREADSIZE];	// temporary character buffer
	unsigned int nodarr[200]; // returning node/backnode combos from CreateVRML fns.

	int count;
	char command;
	unsigned int uretval;		// unsigned return value
	unsigned int ra,rb,rc,rd;	// temps
	char *EOT;		// ptr to End of Text marker

	while (strlen(bufptr)> 0) {
		//printf ("start of while loop, strlen %d str :%s:\n",strlen(bufptr),bufptr);

		/* step 1, get the command sequence number */
		if (sscanf (bufptr,"%d",&count) != 1) {
			printf ("EAI_parse_commands, expected a sequence number on command :%s:\n",bufptr);
			count = 0;
		}

		/* step 2, skip past the sequence number */
		while (isdigit(*bufptr)) bufptr++;
		while (*bufptr == ' ') bufptr++;

		/* step 3, get the command */
		//printf ("command %c seq %d\n",*bufptr,count);
		command = *bufptr;
		bufptr++;

		// return is something like: $hand->print("RE\n$reqid\n1\n$id\n");

		if (EAIVerbose) printf ("\n... %d ",count);

		switch (command) {
			case GETNAME: { 
				if (EAIVerbose) printf ("GETNAME\n");
				sprintf (buf,"RE\n%d\n1\n%s",count,BrowserName);
				break; 
				}
			case GETVERSION: { 
				if (EAIVerbose) printf ("GETVERSION\n");
				sprintf (buf,"RE\n%d\n1\n%s",count,BrowserVersion);
				break; 
				}
			case GETCURSPEED: { 
				if (EAIVerbose) printf ("GETCURRENTSPEED\n");
				sprintf (buf,"RE\n%d\n1\n%f",count,(float) 1.0/BrowserFPS);
				break; 
				}
			case GETFRAMERATE: { 
				if (EAIVerbose) printf ("GETFRAMERATE\n");
				sprintf (buf,"RE\n%d\n1\n%f",count,BrowserFPS);
				break; 
				}
			case GETURL: { 
				if (EAIVerbose) printf ("GETURL\n");
				sprintf (buf,"RE\n%d\n1\n%s",count,BrowserURL);
				break; 
				}
			case GETNODE:  {
				//format int seq# COMMAND    string nodename

				sscanf (bufptr," %s",ctmp);
				if (EAIVerbose) printf ("GETNODE %s\n",ctmp);

				uretval = EAI_GetNode(ctmp);

				sprintf (buf,"RE\n%d\n1\n%d",count,uretval);
				break; 
			}
		case GETTYPE:  {
			//format int seq# COMMAND  int node#   string fieldname   string direction

			sscanf (bufptr,"%d %s %s",&uretval,ctmp,dtmp);
			if (EAIVerbose) printf ("GETTYPE NODE%d %s %s\n",uretval, ctmp, dtmp);

			EAI_GetType (uretval,ctmp,dtmp,&ra,&rb,&rc,&rd);

			sprintf (buf,"RE\n%d\n1\n%d %d %d %c",count,ra,rb,rc,rd);
			break;
			}
		case SENDEVENT:   {
			//format int seq# COMMAND NODETYPE pointer offset data
			if (EAIVerbose) printf ("SENDEVENT %s\n",bufptr);
			EAI_SendEvent(bufptr);
			break;
			}
		case CREATEVU: 
		case CREATEVS: {
			//format int seq# COMMAND vrml text     string EOT
			if (command == CREATEVS) {
				if (EAIVerbose) printf ("CREATEVS %s\n",bufptr);

				EOT = strstr(buffer,"\nEOT\n");
				// if we do not have a string yet, we have to do this...
				while (EOT == NULL) {
					read_EAI_socket();
					EOT = strstr(buffer,"\nEOT\n");
				}

				*EOT = 0; // take off the EOT marker

				ra = EAI_CreateVrml("String",bufptr,nodarr);
			} else {
				if (EAIVerbose) printf ("CREATEVU %s\n",bufptr);
				ra = EAI_CreateVrml("URL",bufptr,nodarr);
			}

			sprintf (buf,"RE\n%d\n%d\n",count,ra);
			for (rb = 0; rb < ra; rb++) {
				sprintf (ctmp,"%d ", nodarr[rb]);
				strcat (buf,ctmp);
			}

			// finish this for now
			bufptr[0] = 0;
			break;
			}
		case SENDCHILD :  {
			//format int seq# COMMAND  int node#   ParentNode field ChildNode

			sscanf (bufptr,"%d %d %s %s",&ra,&rb,ctmp,dtmp);
			rc = ra+rb; // final pointer- should point to a Multi_Node

			if (EAIVerbose) printf ("SENDCHILD %d %d %s %s\n",ra, rb, ctmp, dtmp);

			getMFNodetype (dtmp,(struct Multi_Node *)rc, 
					strcmp(ctmp,"removeChildren"));

			sprintf (buf,"RE\n%d\n1\n0",count);
			break;
			}
		case UPDATEROUTING :  {
			//format int seq# COMMAND  int node#   ParentNode field ChildNode

			sscanf (bufptr,"%d %d %s %d",&ra,&rb,ctmp,&rc);
			if (EAIVerbose) printf ("SENDCHILD %d %d %s %d\n",ra, rb, ctmp, rc);

			sprintf (buf,"RE\n%d\n1\n0",count);
			break;
			}
		case REPLACEWORLD:  
		case GETVALUE: 
		case REGLISTENER: 
		case ADDROUTE:  
		case DELETEROUTE:  
		case LOADURL: 
		case SETDESCRIPT:  
		case STOPFREEWRL:  
		default: {
			printf ("unhandled command :%c: %d\n",command,command);
			strcat (buf, "unknown_EAI_command");
			break;
			}
					
		}


		/* send the response - events don't send a reply */
		if (command != SENDEVENT) EAI_send_string (buf);
	
		/* skip to the next command */
		while (*bufptr >= ' ') bufptr++;

		/* skip any new lines that may be there */
		while ((*bufptr == 10) || (*bufptr == 13)) bufptr++;
	}
}

unsigned int EAI_SendEvent (char *ptr) {
	unsigned char nodetype;
	unsigned int nodeptr;
	unsigned int offset;

	int ival;
	float fl[4];
	double dval;
	unsigned int memptr;

	/* we have an event, get the data properly scanned in from the ASCII string, and then
		friggin do it! ;-) */

	// node type
	nodetype = *ptr; ptr++;

	//blank space
	ptr++;
	
	//nodeptr, offset
	sscanf (ptr, "%d %d",&nodeptr, &offset);
	while ((*ptr) > ' ') ptr++; 	// node ptr
	while ((*ptr) == ' ') ptr++;	// inter number space(s)
	while ((*ptr) > ' ') ptr++;	// node offset

	if (EAIVerbose) printf ("EAI_SendEvent, nodeptr %x offset %x\n",nodeptr,offset);

	memptr = nodeptr+offset;	// actual pointer to start of destination data in memory

	// now, we are at start of data.
	if (EAIVerbose) printf ("EAI_SendEvent, event string now is %s\n",ptr);

	/* This switch statement is almost identical to the one in the Javascript
	   code (check out CFuncs/CRoutes.c), except that explicit Javascript calls
	   are impossible here (this is not javascript!) */

	switch (nodetype) {
		case SFBOOL:	{	/* SFBool */
			/* printf ("we have a boolean, copy value over string is %s\n",strp); */
			if (strncmp(ptr,"true",4)== (unsigned int) 0) {
				ival = 1;
			} else {
				/* printf ("ASSUMED TO BE FALSE\n"); */
				ival = 0;
			}	
			memcpy ((void *)memptr, (void *)&ival,sizeof(int));
			break;
		}

//xxx		case SFTIME: {
//xxx			if (!JS_ValueToNumber((JSContext *)JSglobs[actualscript].cx, 
//xxx								  global_return_val,&tval)) tval=0.0;
//xxx
//xxx			//printf ("SFTime conversion numbers %f from string %s\n",tval,ptr);
//xxx			//printf ("copying to %#x offset %#x len %d\n",tn, tptr,len);
//xxx			memcpy ((void *)memptr, (void *)&tval,sizeof(double));
//xxx			break;
//xxx		}
		case SFNODE:
		case SFINT32: {
			sscanf (ptr,"%d",&ival);
			memcpy ((void *)memptr, (void *)&ival,sizeof(int));
			break;
		}
		case SFFLOAT: {
			sscanf (ptr,"%f",fl);
			memcpy ((void *)memptr, (void *)fl,sizeof(float));
			break;
		}

		case SFVEC2F: {	/* SFVec2f */
			sscanf (ptr,"%f %f",&fl[0],&fl[1]);
			memcpy ((void *)memptr, (void *)fl,sizeof(float)*2);
			break;
		}
		case SFVEC3F:
		case SFCOLOR: {	/* SFColor */
			sscanf (ptr,"%f %f %f",&fl[0],&fl[1],&fl[2]);
			memcpy ((void *)memptr, (void *)fl,sizeof(float)*3);
			break;
		}

		case SFROTATION: {
			sscanf (ptr,"%f %f %f %f",&fl[0],&fl[1],&fl[2],&fl[3]);
			memcpy ((void *)memptr, (void *)fl,sizeof(float)*4);
			break;
		}


		/* a series of Floats... */
//xxx		case MFVEC3F:
//xxx		case MFCOLOR: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,3); break;}
//xxx		case MFFLOAT: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,1); break;}
//xxx		case MFROTATION: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,4); break;}
//xxx		case MFVEC2F: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,2); break;}
//xxx		case MFNODE: {getMFNodetype (ptr,memptr,CRoutes[route].extra); break;}
//xxx		case MFSTRING: {
//xxx			getMFStringtype ((JSContext *) JSglobs[actualscript].cx,
//xxx							 global_return_val,memptr); 
//xxx			break;
//xxx		}
//xxx
//xxx		case MFINT32: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,0); break;}
//xxx		case MFTIME: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,5); break;}

		default: {
			printf ("unhandled Event :%c: - get code in here\n",nodetype);
			return FALSE;
		}
	}
	update_node ((void *)nodeptr);
	return TRUE;
}


