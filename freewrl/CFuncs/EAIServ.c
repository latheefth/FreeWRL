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

/* Subtypes - types of data to get from EAI  - we don't use the ones defined in
   headers.h, because we want ASCII characters */

#define	EAI_SFUNKNOWN	'a'
#define	EAI_SFBOOL		'b'
#define	EAI_SFCOLOR		'c'
#define	EAI_SFFLOAT		'd'
#define	EAI_SFTIME		'e'
#define	EAI_SFINT32		'f'
#define	EAI_SFSTRING	'g'
#define	EAI_SFNODE		'h'
#define	EAI_SFROTATION	'i'
#define	EAI_SFVEC2F		'j'
#define	EAI_SFIMAGE		'k'
#define	EAI_MFCOLOR		'l'
#define	EAI_MFFLOAT		'm'
#define	EAI_MFTIME		'n'
#define	EAI_MFINT32		'o'
#define	EAI_MFSTRING	'p'
#define	EAI_MFNODE		'q'
#define	EAI_MFROTATION	'r'
#define	EAI_MFVEC2F		's'
#define EAI_MFVEC3F		't'
#define EAI_SFVEC3F		'u'



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

struct hostent *hostptr;
struct sockaddr_in	servaddr, cliaddr;
fd_set rfds;
struct timeval tv;

/* EAI input buffer */
char *buffer;
int bufcount;				// pointer into buffer
int bufsize;				// current size in bytes of input buffer 


int EAIVerbose = 0;

int EAIsendcount = 0;			// how many commands have been sent back?

char EAIListenerData[EAIREADSIZE];	// this is the location for getting Listenered data back again.
char EAIListenerArea[40];		// put the address of the EAIListenerData here.

// prototypes
void EAI_parse_commands (char *stptr);
unsigned int EAI_SendEvent(char *bufptr);
void EAI_send_string (char *str);
void connect_EAI(void);
void handle_EAI(void);
int EAI_GetNode(char *str);		// in VRMLC.pm
void EAI_GetType (unsigned int uretval,
	char *ctmp, char *dtmp,
	int *ra, int *rb,
	int *rc, int *rd, int *re);		// in VRMLC.pm
void read_EAI_socket(void);
void handle_Listener (void);
void CRoutes_Register(int adrem, unsigned int from, unsigned int fromoffset,
	int to_count, char *tonode_str, unsigned int length,
	void *intptr, int scrdir, int extra);				// CFuncs/CRoutes.c
void EAI_Convert_mem_to_ASCII (int id, char *reptype, int type, char *memptr, char *buf);
void shutdown_EAI(void);


void EAI_send_string(char *str){
	unsigned int n;

	/* add a trailing newline */
	strcat (str,"\n");

	if (EAIVerbose) printf ("EAI Command returns\n%s(end of command)\n",str);

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
	
		/* zero out the EAIListenerData here, and after every use */	
		bzero(&EAIListenerData, sizeof(EAIListenerData));

		/* seems like we are up and running now, and waiting for a command */
		EAIinitialized = TRUE;	
	}
}

/* the user has pressed the "q" key */
void shutdown_EAI() {
	
	if (EAIVerbose) printf ("shutting down EAI\n");
	strcpy (EAIListenerData,"QUIT\n\n\n");
	if (EAIinitialized) {
		EAI_send_string(EAIListenerData);
	}

}
void create_EAI() {
        if (EAIVerbose) printf ("EAISERVER:create_EAI called\n");

	/* already wanted? if so, just return */
	if (EAIwanted) return;

	/* so we know we want EAI */
	EAIwanted = TRUE;

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
* (eg, EAI_SFFLOAT); these sub types are indicated with a lower case letter; again,
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
	unsigned int scripttype;
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
				sprintf (buf,"RE\n%d\n%s",count,BrowserName);
				break; 
				}
			case GETVERSION: { 
				if (EAIVerbose) printf ("GETVERSION\n");
				sprintf (buf,"RE\n%d\n%s",count,BrowserVersion);
				break; 
				}
			case GETCURSPEED: { 
				if (EAIVerbose) printf ("GETCURRENTSPEED\n");
				sprintf (buf,"RE\n%d\n%f",count,(float) 1.0/BrowserFPS);
				break; 
				}
			case GETFRAMERATE: { 
				if (EAIVerbose) printf ("GETFRAMERATE\n");
				sprintf (buf,"RE\n%d\n%f",count,BrowserFPS);
				break; 
				}
			case GETURL: { 
				if (EAIVerbose) printf ("GETURL\n");
				sprintf (buf,"RE\n%d\n%s",count,BrowserURL);
				break; 
				}
			case GETNODE:  {
				//format int seq# COMMAND    string nodename

				sscanf (bufptr," %s",ctmp);
				if (EAIVerbose) printf ("GETNODE %s\n",ctmp);

				uretval = EAI_GetNode(ctmp);

				sprintf (buf,"RE\n%d\n%d",count,uretval);
				break; 
			}
			case GETTYPE:  {
				//format int seq# COMMAND  int node#   string fieldname   string direction
	
				sscanf (bufptr,"%d %s %s",&uretval,ctmp,dtmp);
				if (EAIVerbose) printf ("GETTYPE NODE%d %s %s\n",uretval, ctmp, dtmp);
	
				EAI_GetType (uretval,ctmp,dtmp,&ra,&rb,&rc,&rd,&scripttype);
	
				sprintf (buf,"RE\n%d\n%d %d %d %c %d",count,ra,rb,rc,rd,scripttype);
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
	
					ra = EAI_CreateVrml("String",bufptr,nodarr,200);
				} else {
					if (EAIVerbose) printf ("CREATEVU %s\n",bufptr);
					ra = EAI_CreateVrml("URL",bufptr,nodarr,200);
				}
	
				sprintf (buf,"RE\n%d\n",count);
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
	
				if (EAIVerbose) printf ("SENDCHILD Parent: %d ParentField: %d %s Child: %s\n",ra, rb, ctmp, dtmp);

	
				getMFNodetype (dtmp,(struct Multi_Node *)rc, 
						strcmp(ctmp,"removeChildren"));
	
				/* tell the routing table that this node is updated - used for RegisterListeners */
				mark_event(ra,rb);

				sprintf (buf,"RE\n%d\n0",count);
				break;
				}
			case UPDATEROUTING :  {
				//format int seq# COMMAND  int node#   ParentNode field ChildNode
	
				sscanf (bufptr,"%d %d %s %d",&ra,&rb,ctmp,&rc);
				if (EAIVerbose) printf ("SENDCHILD %d %d %s %d\n",ra, rb, ctmp, rc);
	
				sprintf (buf,"RE\n%d\n0",count);
				break;
				}
			case REGLISTENER: {
				if (EAIVerbose) printf ("REGISTERLISTENER %s \n",bufptr);
	
				//143024848 88 8 e 6
				sscanf (bufptr,"%d %d %c %d",&ra,&rb,ctmp,&rc);
				// so, count = query id, ra pointer, rb, offset, ctmp[0] type, rc, length
				ctmp[1]=0;
	
				//printf ("REGISTERLISTENER from %d foffset %d fieldlen %d type %s \n",
				//		ra, rb,rc,ctmp);
				

				/* put the address of the listener area in a string format for registering
				   the route - the route propagation will copy data to here */
				sprintf (EAIListenerArea,"%d:0",(int)&EAIListenerData);

				/* set up the route from this variable to the handle_Listener routine */
				CRoutes_Register  (1,ra,rb, 1, EAIListenerArea, rc, &handle_Listener, 0, 
					(count<<8)+ctmp[0]); // encode id and type here
	
				sprintf (buf,"RE\n%d\n0",count);
				break;
				}

			case GETVALUE: {
				if (EAIVerbose) printf ("GETVALUE %s \n",bufptr);

				// format: ptr, offset, type, length (bytes)
				sscanf (bufptr, "%d %d %c %d", &ra,&rb,ctmp,&rc);

				ra = ra + rb;   // get absolute pointer offset
				EAI_Convert_mem_to_ASCII (count,"RE",(int)ctmp[0],(char *)ra, buf);
				break;
				}
			case REPLACEWORLD:  {
				if (EAIVerbose) printf ("REPLACEWORLD %s \n",bufptr);
				EAI_replaceWorld(bufptr);
				break;
				}
			case ADDROUTE:  
			case DELETEROUTE:  {
				if (EAIVerbose) printf ("Add/Delete route %s\n",bufptr);
				EAI_Route (command,bufptr);
				sprintf (buf,"RE\n%d\n0",count);
				break;
				}
				
//XXXX			case LOADURL: 
//XXXX			case SETDESCRIPT:  
//XXXX			case STOPFREEWRL:  
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
	unsigned int scripttype;

	int ival;
	float fl[4];
	double tval;
	unsigned int memptr;
	int convertnodetype;

	/* we have an event, get the data properly scanned in from the ASCII string, and then
		friggin do it! ;-) */

	// node type
	nodetype = *ptr; ptr++;

	//blank space
	ptr++;
	
	//nodeptr, offset
	sscanf (ptr, "%d %d %d",&nodeptr, &offset, &scripttype);
	while ((*ptr) > ' ') ptr++; 	// node ptr
	while ((*ptr) == ' ') ptr++;	// inter number space(s)
	while ((*ptr) > ' ') ptr++;	// node offset
	while ((*ptr) == ' ') ptr++;	// inter number space(s)
	while ((*ptr) > ' ') ptr++;	// script type

	if (EAIVerbose) 
		printf ("EAI_SendEvent, nodeptr %x offset %x script type %d \n",nodeptr,offset, scripttype);

	/* We have either a event to a memory location, or to a script. */
	/* the field scripttype tells us whether this is true or not.   */

	memptr = nodeptr+offset;	// actual pointer to start of destination data in memory

	// now, we are at start of data.
	if (EAIVerbose) printf ("EAI_SendEvent, event string now is %s\n",ptr);

	/* This switch statement is almost identical to the one in the Javascript
	   code (check out CFuncs/CRoutes.c), except that explicit Javascript calls
	   are impossible here (this is not javascript!) */

	switch (nodetype) {
		case EAI_SFBOOL:	{	/* EAI_SFBool */
			/* printf ("we have a boolean, copy value over string is %s\n",strp); */
			/* yes, it is <space>TRUE... */
			if (strncmp(ptr," TRUE",5)== (unsigned int) 0) { ival = 1;
			} else { ival = 0; }

			if (scripttype) set_one_ECMAtype(nodeptr,offset,SFBOOL,&ival,sizeof(int));
			else memcpy ((void *)memptr, (void *)&ival,sizeof(int));
			break;
		}

		case EAI_SFTIME: {
			sscanf (ptr,"%lf",&tval);
			//printf ("EAI_SFTime conversion numbers %f from string %s\n",tval,ptr);
			if (scripttype) set_one_ECMAtype(nodeptr,offset,SFTIME,&tval,sizeof(double));
			else memcpy ((void *)memptr, (void *)&tval,sizeof(double));
			break;
		}
		case EAI_SFNODE:
		case EAI_SFINT32: {
			sscanf (ptr,"%d",&ival);
			if (scripttype) set_one_ECMAtype(nodeptr,offset,SFINT32,&ival,sizeof(int));
			else memcpy ((void *)memptr, (void *)&ival,sizeof(int));
			break;
		}
		case EAI_SFFLOAT: {
			sscanf (ptr,"%f",fl);
			if (scripttype) set_one_ECMAtype(nodeptr,offset,SFFLOAT,&fl,sizeof(float));
			else memcpy ((void *)memptr, (void *)fl,sizeof(float));
			break;
		}

		case EAI_SFVEC2F: {	/* EAI_SFVec2f */
			sscanf (ptr,"%f %f",&fl[0],&fl[1]);
			if (scripttype) Set_one_MultiElementtype (nodeptr, offset, ptr, sizeof(float)*2);
			else memcpy ((void *)memptr, (void *)fl,sizeof(float)*2);
			break;
		}
		case EAI_SFVEC3F:
		case EAI_SFCOLOR: {	/* EAI_SFColor */
			sscanf (ptr,"%f %f %f",&fl[0],&fl[1],&fl[2]);
			if (scripttype) Set_one_MultiElementtype (nodeptr, offset, ptr, sizeof(float)*3);
			else memcpy ((void *)memptr, (void *)fl,sizeof(float)*3);
			break;
		}

		case EAI_SFROTATION: {
			sscanf (ptr,"%f %f %f %f",&fl[0],&fl[1],&fl[2],&fl[3]);
			if (scripttype) Set_one_MultiElementtype (nodeptr, offset, ptr, sizeof(float)*4);
			else memcpy ((void *)memptr, (void *)fl,sizeof(float)*4);
			break;
		}

		case EAI_MFSTRING: {
			printf ("EAI_MFSTRING, string is %s\nxxx\n",ptr);
			printf ("EAI_MFSTRING, have to fix this code - sorry Sarah. JohnS\n");
//xxx			getEAI_MFStringtype ((JSContext *) JSglobs[actualscript].cx,
//xxx							 global_return_val,memptr); 
			break;
		}

		case EAI_SFSTRING: {

			// this can be handled exactly like a set_one_ECMAtype if it is a script
		}


		/* a series of Floats... */
//xxx		case EAI_MFVEC3F:
//xxx		case EAI_MFCOLOR: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,3); break;}
//xxx		case EAI_MFFLOAT: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,1); break;}
//xxx		case EAI_MFROTATION: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,4); break;}
//xxx		case EAI_MFVEC2F: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,2); break;}
//xxx		case EAI_MFNODE: {getEAI_MFNodetype (ptr,memptr,CRoutes[route].extra); break;}
//xxx		case EAI_MFSTRING: {
//xxx			break;
//xxx		}
//xxx
//xxx		case EAI_MFINT32: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,0); break;}
//xxx		case EAI_MFTIME: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, memptr,5); break;}

		default: {
			printf ("unhandled Event :%c: - get code in here\n",nodetype);
			return FALSE;
		}
	}

	if (scripttype) {
		mark_script(nodeptr);
	} else {
		/* if this is a geometry, make it re-render. Some nodes (PROTO interface params w/o IS's) 
	   	   will have an offset of zero, and are thus not "real" nodes, only memory locations */

		if (offset > 0) update_node ((void *)nodeptr);

		/* if anything uses this for routing, tell it that it has changed */
		mark_event (nodeptr,offset);
	}

	return TRUE;
}

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
	struct Multi_Node *mfptr;	// used for freeing memory

	// get the type and the id.
	tp = CRoutesExtra&0xff;
	id = (CRoutesExtra & 0xffffff00) >>8;

	if (EAIVerbose) 
		printf ("Handle listener, id %x type %x extradata %x\n",id,tp,CRoutesExtra);

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
	EAI_send_string(buf);
}


/********************************************************************
*
* Extra Memory routine - for PROTO interface declarations that are used,
* but are not IS'd. (EAI uses these things!)
*
**********************************************************************/

unsigned EAI_do_ExtraMemory (int size,SV *data,char *type) {
	int val;
	char *memptr;
	int ty;
	float fl[4];
	int len;

	/* variables for MFStrings */
	struct Multi_String *MSptr;
	struct SFRotation *SFFloats;
	AV *aM;
	SV **bM;
	int iM;
	int lM;
	int xx;


	memptr = 0;  /* get around a compiler warning */

	/* convert the type string to an internal type */
	ty = convert_typetoInt (type);

	printf ("EAI - extra memory for size %d type %s\n",size,type);

	if (size > 0) {
		memptr = malloc ((unsigned)size);
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
		case SFFLOAT: {
				fl[0] = SvNV(data);
				memcpy(memptr,&fl[0],(unsigned) size);
				break; 
			}	


		case SFSTRING: { 
				memptr = malloc (strlen(SvPV(data,len))+1);
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
				SFFloats = (struct SFRotation *) memptr;
				len = size / (sizeof(float));	// should be 2 for SFVec2f, 3 for SFVec3F...

				if(!SvROK(data)) {
					for (iM=0; iM<len; iM++) (*SFFloats).r[iM] = 0;
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
						(*SFFloats).r[iM] = SvNV(*bM);
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
				(*MSptr).p = malloc(lM * sizeof(*((*MSptr).p)));
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

//XXX		case MFNODE: { break; }
//XXX		case MFROTATION: { break; }
//XXX		case MFVEC2F: { break; }
//XXX		case SFTIME : { break; }
//XXX		case SFIMAGE: { break; }
//XXX		case MFCOLOR: { break; }
//XXX		case MFFLOAT: { break; }
//XXX		case MFTIME: { break; }
//XXX		case MFINT32: { break; }
		default: {
			printf ("EAI_do_ExtraMemory, unhandled type %s\n",type);
		}
	}
	// printf ("EAI_Extra memory, returning %d\n",memptr);
	return (unsigned) memptr;
	
}

/* convert a number in memory to a printable type. Used to send back EVents, or replies to
   the Java client program. */

void EAI_Convert_mem_to_ASCII (int id, char *reptype, int type, char *memptr, char *buf) {

	double dval;
	float fl[4];
	int ival;
	int row;			/* MF* counter */
	struct Multi_String *MSptr;	/* MFString pointer */
	struct Multi_Node *MNptr;	/* MFNode pointer */
	char *ptr;			/* used for building up return string */
	int xx;

	switch (type) {
		case EAI_SFBOOL: 	{
			if (EAIVerbose) printf ("EAI_SFBOOL\n");				
			if (memptr[0] == 1) sprintf (buf,"%s\n%d\nTRUE",reptype,id);
			else sprintf (buf,"%s\n%d\nFALSE",reptype,id);
			break;
		}

		case EAI_SFTIME:	{
			if (EAIVerbose) printf ("EAI_SFTIME\n");
			memcpy(&dval,memptr,sizeof(double));
			sprintf (buf, "%s\n%d\n%lf",reptype,id,dval);
			break;
		}

		case EAI_SFNODE:
		case EAI_SFINT32:	{
			if (EAIVerbose) printf ("EAI_SFINT32 or EAI_SFNODE\n");
			memcpy(&ival,memptr,sizeof(int));
			sprintf (buf, "%s\n%d\n%d",reptype,id,ival);
			break;
		}

		case EAI_SFFLOAT:	{
			if (EAIVerbose) printf ("EAI_SFTIME\n");
			memcpy(fl,memptr,sizeof(float));
			sprintf (buf, "%s\n%d\n%f",reptype,id,fl[0]);
			break;
		}

		case EAI_SFVEC3F:
		case EAI_SFCOLOR:	{
			if (EAIVerbose) printf ("EAI_SFCOLOR or EAI_SFVEC3F\n");
			memcpy(fl,memptr,sizeof(float)*3);
			sprintf (buf, "%s\n%d\n%f %f %f",reptype,id,fl[0],fl[1],fl[2]);
			break;
		}

		case EAI_SFVEC2F:	{
			if (EAIVerbose) printf ("EAI_SFVEC2F\n");
			memcpy(fl,memptr,sizeof(float)*2);
			sprintf (buf, "%s\n%d\n%f %f",reptype,id,fl[0],fl[1]);
			break;
		}

		case EAI_SFROTATION:	{
			if (EAIVerbose) printf ("EAI_SFROTATION\n");
			memcpy(fl,memptr,sizeof(float)*4);
			sprintf (buf, "%s\n%d\n%f %f %f %f",reptype,id,fl[0],fl[1],fl[2],fl[3]);
			break;
		}

		case EAI_SFSTRING:	{
			if (EAIVerbose) printf ("EAI_SFSTRING\n");
			sprintf (buf, "%s\n%d\n\"%s\"",reptype,id,memptr);
			break;
		}

		case EAI_MFSTRING:	{
			if (EAIVerbose) printf ("EAI_MFSTRING\n");
		
			/* make the Multi_String pointer */
			MSptr = (struct Multi_String *) memptr;

			// printf ("EAI_MFString, there are %d strings\n",(*MSptr).n);
			sprintf (buf, "%s\n%d\n",reptype,id);
			ptr = buf + strlen(buf);

			for (row=0; row<(*MSptr).n; row++) {
        	        	// printf ("String %d is %s\n",row,SvPV((*MSptr).p[row],xx));
				if (strlen (SvPV((*MSptr).p[row],xx)) == 0) {
					sprintf (ptr, "\"XyZZtitndi\" "); // encode junk for Java side.
				} else {
					sprintf (ptr, "\"%s\" ",SvPV((*MSptr).p[row],xx));
				}
				// printf ("buf now is %s\n",buf);
				ptr = buf + strlen (buf);
			}
	
			break;
		}

		case EAI_MFNODE: 	{
			MNptr = (struct Multi_Node *) memptr;

			if (EAIVerbose) printf ("EAI_MFNode, there are %d nodes at %d\n",(*MNptr).n,(int) memptr);
			sprintf (buf, "%s\n%d\n",reptype,id);
			ptr = buf + strlen(buf);

			for (row=0; row<(*MNptr).n; row++) {
				sprintf (ptr, "%d ",(int) (*MNptr).p[row]);
				ptr = buf + strlen (buf);
			}
			break;
		}
		default: {
			printf ("EAI, type %c not handled yet\n",type);
		}


//XXX	case EAI_SFIMAGE:	{handleptr = &handleEAI_SFIMAGE_Listener;break;}
//XXX	case EAI_MFCOLOR:	{handleptr = &handleEAI_MFCOLOR_Listener;break;}
//XXX	case EAI_MFFLOAT:	{handleptr = &handleEAI_MFFLOAT_Listener;break;}
//XXX	case EAI_MFTIME:	{handleptr = &handleEAI_MFTIME_Listener;break;}
//XXX	case EAI_MFINT32:	{handleptr = &handleEAI_MFINT32_Listener;break;}
//XXX	case EAI_MFROTATION:{handleptr = &handleEAI_MFROTATION_Listener;break;}
//XXX	case EAI_MFVEC2F:	{handleptr = &handleEAI_MFVEC2F_Listener;break;}
//XXX	case EAI_MFVEC3F:	{handleptr = &handleEAI_MFVEC3F_Listener;break;}
	}
}
