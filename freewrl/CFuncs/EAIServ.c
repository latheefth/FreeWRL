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


int EAIVerbose = 1;

int EAIsendcount = 0;			// how many commands have been sent back?



// prototypes
void EAI_parse_commands (char *stptr);
unsigned int EAI_SendEvent(char *bufptr);
void EAI_send_string (char *str);
void connect_EAI(void);
void create_EAI(char *eailine);
void handle_EAI(void);


void EAI_send_string(char *str){
	int n;

	/* add a trailing newline */
	strcat (str,"\n");

	if (EAIVerbose) printf ("EAISERVER:EAI_send_string, %s\n",str);
	n = write (listenfd, str, (unsigned int) strlen(str));
	if (n<strlen(str)) {
		printf ("write, expected to write %d, actually wrote %d\n",n,strlen(str));
	}
	if (EAIVerbose) printf ("EAISERVER:EAI_send_string, sent \n");

}

/* open the socket connection -  we open as a TCP server, and will find a free socket */
void connect_EAI() {
	int socketincrement;
	int len;
	const int on=1;
	int flags;

        struct sockaddr_in      servaddr;

	if (EAIfailed) return;

	//if (EAIVerbose) printf ("EAISERVER:connect\n");

	if (sockfd < 0) {
		// step 1  - create socket
	        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf ("EAIServer: socket error\n");
			EAIfailed=TRUE;
			return;
		}
	
	
		setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
		if (flags=fcntl(sockfd,F_GETFL,0) < 0) {
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
	
		printf ("connect_EAI - socket made\n");
	
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
		//printf ("past step 3\n");
	}

	if ((sockfd >=0) && (listenfd<0)) {
		// step 4 - accept
		len = sizeof(cliaddr);
	        if ( (listenfd = accept(sockfd, (struct sockaddr *) &cliaddr, &len)) < 0) {
			//printf ("EAIServer: no client yet\n");
		}
		//printf ("past step 4 - listenfd %d\n",listenfd);
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

/* possibly we have an incoming EAI request from the client */
void handle_EAI () {
	int retval;
	int len;

	/* do nothing unless we are wanted */
	if (!EAIwanted) return;
	if (!EAIinitialized) {
		connect_EAI();
		return;
	}

	len = sizeof (servaddr);
	retval = FALSE;
	bufcount = 0;

	do {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(listenfd, &rfds);
	
		retval = select(listenfd+1, &rfds, NULL, NULL, &tv);
	
	        //if (EAIVerbose) printf ("EAISERVER:Select retval:%d\n",retval);
	
		if (retval) {
			retval = read (listenfd, &buffer[bufcount],EAIREADSIZE);

			if (retval == 0) {
				// client disappeared
				close (listenfd);
				listenfd = -1;
				EAIinitialized=FALSE;
			}

			printf ("read in %d , max %d",retval,EAIREADSIZE);
			printf ("space left %d\n",bufsize - bufcount);
			bufcount += retval;

			if ((bufsize - bufcount) < 10) {
				printf ("HAVE TO REALLOC INPUT MEMORY\n");
				bufsize += EAIREADSIZE;
				buffer = realloc (buffer, bufsize);
			}
		}
	} while (retval);

	/* make this into a C string */
	bufcount ++;
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
	int count;
	char command;
	unsigned int uretval;		// unsigned return value
	unsigned int ra,rb,rc,rd;	// temps


	while (strlen(bufptr)> 0) {
		//for (tmp = 0; tmp <= strlen(bufptr); tmp ++) {
		//	printf ("char %d is %x %d\n",tmp,bufptr[tmp],bufptr[tmp]);
		//}
	
		printf ("EAIServer string is:%s: strlen %d\n",bufptr,strlen (bufptr));


		/* step 1, get the command sequence number */
		if (sscanf (bufptr,"%d",&count) != 1) {
			printf ("EAI_parse_commands, expected a sequence number on command :%s:\n",bufptr);
			count = 0;
		}

		/* step 2, skip past the sequence number */
		while (isdigit(*bufptr)) bufptr++;
		while (*bufptr == ' ') bufptr++;

		/* step 3, get the command */
		printf ("command %c seq %d\n",*bufptr,count);
		command = *bufptr;
		bufptr++;

		// return is something like: $hand->print("RE\n$reqid\n1\n$id\n");

		/* step 4, get the return string prepared */
		sprintf (buf,"\nRE\n%d\n",count);

		switch (command) {
			case GETNAME: { 
				if (EAIVerbose) printf ("GETNAME command recieved\n");
				strcat (buf,BrowserName);
				break; 
				}
			case GETVERSION: { 
				if (EAIVerbose) printf ("GETVERSION command recieved\n");
				strcat (buf,BrowserVersion);
				break; 
				}
			case GETCURSPEED: { 
				if (EAIVerbose) printf ("GETCURRENTSPEED command recieved\n");
				strcat (buf,"0.0"); // valid to return this
				break; 
				}
			case GETFRAMERATE: { 
				if (EAIVerbose) printf ("GETFRAMERATE command recieved\n");
				sprintf (buf,"%d %f",count,BrowserFPS);
				break; 
				}
			case GETURL: { 
				if (EAIVerbose) printf ("GETURL command recieved\n");
				strcat (buf,BrowserURL); // valid to return this
				break; 
				}
			case GETNODE:  {
				//format int seq# COMMAND    string nodename

				if (EAIVerbose) printf ("GETNODE command recieved\n");

				sscanf (bufptr," %s",ctmp);
				uretval = EAI_GetNode(ctmp);

				sprintf (buf,"RE\n%d\n1000\n%d",count,uretval);
				break; 
				}
			case GETTYPE:  {
				//format int seq# COMMAND  int node#   string fieldname   string direction

				if (EAIVerbose) printf ("GETNODE command recieved\n");

				sscanf (bufptr,"%d %s %s",&uretval,ctmp,dtmp);
				EAI_GetType (uretval,ctmp,dtmp,&ra,&rb,&rc,&rd);

				sprintf (buf,"RE\n%d\n01\n%d %d %d %c",count,ra,rb,rc,rd);
				break;
				}
			case SENDEVENT:   {
				//format int seq# COMMAND NODETYPE pointer offset data
				if (EAIVerbose) printf ("SENDEVENT command recieved\n");
				printf ("line is %s\n",bufptr);

				sprintf (buf,"%d %d\n",count,EAI_SendEvent(bufptr));

				break;
				}
			case REPLACEWORLD:  
			case UPDATEROUTING : 
			case SENDCHILD :  
			case GETVALUE: 
			case REGLISTENER: 
			case ADDROUTE:  
			case DELETEROUTE:  
			case LOADURL: 
			case SETDESCRIPT:  
			case CREATEVS: 
			case CREATEVU:  
			case STOPFREEWRL:  
			default: {
				printf ("unknown command :%c: %d\n",command,command);
				strcat (buf, "unknown_EAI_command");
				break;
				}
					
		}


		/* send the response */
		EAI_send_string (buf);
	
		/* skip to the next command */
		while (*bufptr >= ' ') bufptr++;
		//printf ("at next command len %d str:%s:\n",strlen(bufptr),bufptr);

		/* skip any new lines that may be there */
		while ((*bufptr == 10) || (*bufptr == 13)) bufptr++;
		//printf ("skipped newlines, len %d str:%s:\n",strlen(bufptr),bufptr);
	}
}

unsigned int EAI_SendEvent (char *ptr) {
	unsigned char nodetype;
	unsigned int nodeptr;
	unsigned int offset;

	int ival;
	float fl[4];
	double dval;

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
	printf ("EAI_SendEvent, nodeptr %x offset %x\n",nodeptr,offset);

	// now, we are at start of data.
	printf ("EAI_SendEvent, event string now is %s\n",ptr);

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
			memcpy ((void *)nodeptr+offset, (void *)&ival,sizeof(int));
			break;
		}

//xxx		case SFTIME: {
//xxx			if (!JS_ValueToNumber((JSContext *)JSglobs[actualscript].cx, 
//xxx								  global_return_val,&tval)) tval=0.0;
//xxx
//xxx			//printf ("SFTime conversion numbers %f from string %s\n",tval,ptr);
//xxx			//printf ("copying to %#x offset %#x len %d\n",tn, tptr,len);
//xxx			memcpy ((void *)nodeptr+offset, (void *)&tval,sizeof(double));
//xxx			break;
//xxx		}
		case SFNODE:
		case SFINT32: {
			sscanf (ptr,"%d",&ival);
			memcpy ((void *)nodeptr+offset, (void *)&ival,sizeof(int));
			break;
		}
		case SFFLOAT: {
			sscanf (ptr,"%f",&fl);
			memcpy ((void *)nodeptr+offset, (void *)&fl,sizeof(float));
			break;
		}

		case SFVEC2F: {	/* SFVec2f */
			sscanf (ptr,"%f %f",&fl[0],&fl[1]);
			memcpy ((void *)nodeptr+offset, (void *)fl,sizeof(float)*2);
			break;
		}
		case SFVEC3F:
		case SFCOLOR: {	/* SFColor */
			sscanf (ptr,"%f %f %f",&fl[0],&fl[1],&fl[2]);
			memcpy ((void *)nodeptr+offset, (void *)fl,sizeof(float)*3);
			break;
		}

		case SFROTATION: {
			sscanf (ptr,"%f %f %f %f",&fl[0],&fl[1],&fl[2],&fl[3]);
			memcpy ((void *)nodeptr+offset, (void *)fl,sizeof(float)*4);
			break;
		}


		/* a series of Floats... */
//xxx		case MFVEC3F:
//xxx		case MFCOLOR: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, nodeptr+offset,3); break;}
//xxx		case MFFLOAT: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, nodeptr+offset,1); break;}
//xxx		case MFROTATION: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, nodeptr+offset,4); break;}
//xxx		case MFVEC2F: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, nodeptr+offset,2); break;}
//xxx		case MFNODE: {getMFNodetype (ptr,nodeptr+offset,CRoutes[route].extra); break;}
//xxx		case MFSTRING: {
//xxx			getMFStringtype ((JSContext *) JSglobs[actualscript].cx,
//xxx							 global_return_val,nodeptr+offset); 
//xxx			break;
//xxx		}
//xxx
//xxx		case MFINT32: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, nodeptr+offset,0); break;}
//xxx		case MFTIME: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, nodeptr+offset,5); break;}

		default: {
			printf ("unhandled Event :%c: - get code in here\n",nodetype);
			return FALSE;
		}
	}
	update_node ((void *)nodeptr);
	return TRUE;
}


//sub gulp {
//	my($this, $handle) = @_;
//	my ($s,$b);
//	my($rin,$rout);
//	do {
//		# print "GULPING\n";
//		my $n = $handle->sysread($b,1000);
//		# print "GULPED $n ('$b')\n";
//		goto OUT if !$n;
//		$s .= $b;
//		vec($rin,$handle->fileno,1) = 1;
//		select($rout=$rin,"","",0);
//		# print "ROUT : $rout\n";
//	} while(vec($rout,$handle->fileno,1));
//	# print "ENDGULP\n";
//  OUT:
//	return $s;
//}
//
//# However funny this is, we do connect as the client ;)
//sub connect {
//	my($this, $addr) = @_;
//	$addr =~ /^(.*):(.*)$/ or 
//		die  "Invalid EAI adress '$addr'";
//	
//	($EAIhost, $EAIport) = ($1,$2);
//
//	#print ("FreeWRL: connect: remote $EAIhost  port $EAIport\n");
//	my $sock;
//	$sock = IO::Socket::INET->new(
//		Proto => "tcp",
//		PeerAddr => $EAIhost,
//		PeerPort => $EAIport
//	);
//
//	# is socket open? If not, wait.....
//	if (!$sock) { 
//		# print "FreeWRL: Connect: socket not opened yet...\n";
//		return;
//	}
//
//	$this->doconnect($sock);
//}
//
//sub doconnect {
//	my($this,$sock) = @_;
//
//	# set up a socket, when it is connected, then send EAI an initial message. 
//	$sock->autoflush(1);
//
//	$sock->setvbuf("",&_IONBF,0);
//	push @{$this->{Conn}}, $sock;
//	$sock->print("$VRML::Config{VERSION}\n");
//}
//
//sub poll {
//	my($this) = @_;
//	my ($nfound, $timeleft,$rout);
//
//
//	# if the socket is not open yet, try it, once again...
//	if (!defined $this->{Conn}) {
//		# print "FreeWRL: Poll: socket not opened yet for host $EAIhost port $EAIport\n";
//		# lets just try again in a while...
//		if ($EAIrecount < 100) {
//			$EAIrecount +=1;
//			return;
//		}
//
//		# woops! While is up! lets try connecting again.
//        	my $sock;
//       		 $sock = IO::Socket::INET->new(
//       		         Proto => "tcp",
//       		         PeerAddr => $EAIhost,
//       		         PeerPort => $EAIport
//       		 );
//
//       		 # is socket open? If not, wait.....
//		if (!$sock && $EAIfailure < $VRML::ENV{EAI_CONN_RETRY}) {
//			 $EAIfailure += 1;
//			 $EAIrecount = 0;
//       		         #print "FreeWRL: Poll: socket not opened yet...\n";
//       		 } elsif (!$sock
//			     && $EAIfailure >= $VRML::ENV{EAI_CONN_RETRY}
//			     && $VRML::ENV{AS_PLUGIN}) {
//			 print "FreeWRL: Poll: connect to EAI socket timed-out.\n"
//			     if $VRML::verbose::EAI;
//			 ## remove the sub poll from array reference
//			 shift(@{$this->{B}->{Periodic}});
//       		 } else {
//			#print "FreeWRL: Poll: Socket finally opened!!! \n";
//        		$this->doconnect($sock);
//		}
//
//	}
//
//	if (defined $this->{Conn}) {
//		my $rin = '';
//
//	#print "poll opened, eof is ", ref $this->{Conn},"\n";
//	
//		for(@{$this->{Conn}}) {
//			vec($rin, $_->fileno, 1) = 1;
//		}
//		($nfound, $timeleft) = select($rout = $rin, '', '', 0);
//		if($nfound) {
//			for(@{$this->{Conn}}) {
//				if(vec($rout, $_->fileno, 1)) {
//					$this->handle_input($_);
//				}
//			}
//		}
//	}
//}
//
//
//
//sub find_actual_node_and_field {
//	my ($id, $field, $eventin) = @_;
//	my $actualfield;
//
//	# print "find_actual_node_and_field, looking for ",
//	# 	VRML::NodeIntern::dump_name($id)," field $field\n";
//	my $node = VRML::Handles::get($id);
//	if ($node eq ""){
//		# node was not registered
//		$node = $id;
//	}
//
//	# remove the set_ and _changed, if it exists.
//	# actual fields (NOT EventIN/OUTs) are what we are usually after
//	$actualfield = $field;
//	if ($eventin == 1) {
//  		$actualfield =~ s/^set_//; # trailing new line....
//	} else {
//  		$actualfield =~ s/_changed$//; # trailing new line....
//	}
//
//	 # print "find_actual_node, looking at node ",
//	 # 	VRML::NodeIntern::dump_name($node),"  ref ", ref $node, 
//	 # 	" field :$field:, actualfield $actualfield eventin flag $eventin\n";
//
//	if (exists $node->{Fields}{$field}) {
//		# print "find_actual_node this is a direct pointer to the field, just returning\n";
//		return ($node, $field);
//	}
//
//	if (exists $node->{Fields}{$actualfield}) {
//		# print "find_actual_node this is a direct pointer to the actualfield, just returning\n";
//		return ($node, $actualfield);
//	}
//
//	# print "find_actual_node, step 0.3\n";
//
//	#AK - try to let FieldHash deal with locating IS references...
//	#AK - #my $direction;
//	#AK - #if ($eventin == 1) { $direction = "IS_ALIAS_IN"}
//	#AK - #else {$direction = "IS_ALIAS_OUT"};
//	#AK - #
//	#AK - #if (defined $node->{Scene}{$direction}{$field}) {
//		# aha! is this an "IS"?
//		# print "find_actual_node, this is a IsProto\n";
//
//		#AK - #my $n; my $f;
//
//		#AK - #$n = $node->{Scene}{$direction}{$field}[0][0];
//		#AK - #$f = $node->{Scene}{$direction}{$field}[0][1];
//		# print "find_actual_node, n is ",VRML::NodeIntern::dump_name($n)," f is $f\n";
//		#AK - #if ($n != "") {
//			# it is an is...
//			# print "find_actual_node, returning n,f\n";
//			#AK - #return ($n, $f);
//		#AK - #}
//		# it is a protoexp, and it it not an IS, so, lets just return
//		# the original, and cross our fingers that it is right.
//		# print "find_actual_node, actually, did not find node, returning defaults\n";
//		#AK - #return ($node, $field);
//	#AK - #}
//
//	#JAS . dont actually know if any of the rest is of importance, but
//	#JAS keeping it here for now.
//
//	# Hmmm - it was not a PROTO, lets just see if the field
//	# exists here.
//
//	if (defined $node->{$field}) {
//		return ($node, $field);
//	}
//	# Well, next test. Is this an EventIn/EventOut, static parameter to
//	# a PROTO?
//	# print "find_actual_node, step3\n";
//
//	#AK - #if (defined $node->{Scene}) {
//		#AK - #$node = VRML::Handles::front_end_child_get($node);
//		#JAS ($node,$field) = find_actual_node_and_field ($node,$field,$eventin);
//		#AK - #return ($node,$field);
//	#AK - #}
//
//	return ($node,$field);
//}
//
//
//sub handle_input {
//	my($this, $hand) = @_;
//
//	my @lines = split "\n",$this->gulp($hand);
//
//	# is the socket closed???
//	if ($#lines == -1) {
//		# send a "quitpressed" - note that this will only be
//		# intercepted when not running in netscape. It is 
//		# very useful, however, when running eai standalone.
//		$this->{B}->{BE}->{QuitPressed} = 1;
//	}
//
//	while (@lines) {
//		if($VRML::verbose::EAI) {
//		  print "Handle input $#lines\nEAI input:\n";
//		  my $myline; 
//		  foreach $myline (@lines) {
//			print "....",$myline,".... \n";
//		  }
//		  print ".. finished\n\n";
//		}
//
//		my $reqid = shift @lines; # reqid + newline
//
//                if ($reqid eq '') {
//		  $reqid = shift @lines; # reqid + newline
//                }
//
//		my $str = shift @lines; 
//
//		if($str =~ /^GN (.*)$/) { # Get node
//        		# Kevin Pulo <kev@pulo.com.au>
//			my $node = $this->{B}->api_getNode($1);
//			if (!defined $node) {
//				warn("Node $1 is not defined");
//				next;
//			}
//
//			if ("VRML::DEF" eq ref $node) {
//				$node = $this->{B}->api_getNode(VRML::Handles::return_def_name($1));
//				if (!defined $node) {
//					warn("DEF node $1 is not defined");
//					next;
//				}
//			}
//
//			#AK - #if (defined $node->{IsProto}) {
//				# print "GN of $1 is a proto, getting the real node\n";
//				#AK - #$node = $node->real_node();
//			#AK - #}
//
//			my $id = VRML::Handles::reserve($node);
//
//			# remember this - this node is displayed already
//			VRML::Handles::displayed($node);
//
//			if ($VRML::verbose::EAI) {
//				  print "GN returns $id\n";
//			}
//			$hand->print("RE\n$reqid\n1\n$id\n");
//
//		} elsif($str =~ /^GT ([^ ]+) ([^ ]+)$/) { # get eventOut type
//			my($id, $field) = ($1, $2);
//
//			my $node;
//
//			($node,$field) = find_actual_node_and_field($id,$field,0);
//
//                        if ($VRML::verbose::EAI) {
//				print "GT, looking for type for node $node\n";
//			}
//
//			my ($kind, $type) = 
//			 $this->{B}->api__getFieldInfo($node, $field);
//
//			 if ($VRML::verbose::EAI) { print " type  $type\n";}
//		        $hand->print("RE\n$reqid\n0\n$type\n");
//
//		} elsif($str =~ /^GV ([^ ]+) ([^ ]+)$/) { # get eventOut Value
//			my($id, $field) = ($1, $2);
//			my $node;
//
//
//			($node,$field) = find_actual_node_and_field($id,$field,0);
//
//                        if ($VRML::verbose::EAI) {
//				print "GV, looking for type for node $node\n";
//			}
//
//			my ($kind, $type) = 
//			 $this->{B}->api__getFieldInfo($node, $field);
//			# print "GV - kind is $kind, type is $type\n";
//
//			#print "GV, trying first get\n";
//			my $val = $node->{RFields}{$field};
//			if ($val eq '') {
//				#print "GV, woops,, have to try normal fields\n";
//				$val = $node->{Fields}{$field};
//			}
//			#print "GV, got the value now\n";
//
//			my $strval;
//
//			if ($type eq "MFNode") {
//				if ($VRML::verbose::EAI) {
//					print "VRMLServ.pm: GV found a MFNode for $node\n";
//				}
//
//				# if this is a MFnode, we don't want the VRML CODE
//				# if ("ARRAY" eq ref $node->{Fields}{$field}) { print "and, it is an ARRAY\n";}
//
//				# $strval = "@{$node->{Fields}{$field}}";
//
//				foreach (@{$node->{Fields}{$field}}) {
//					if (VRML::Handles::check($_) == 0) {
//						$strval = $strval ." ". VRML::Handles::reserve($_);
//					} else {
//					 	$strval = $strval ." ". VRML::Handles::get($_);
//					}
//					# print " so, Im setting strval to $strval\n";
//				}
//
//			} else {
//				$strval = "VRML::Field::$type"->as_string($val);
//				# print "GV value is $val, strval is $strval\n";
//			}
//
//			if ($VRML::verbose::EAI) {
//				print "GV returns $strval\n";
//			}
//			$hand->print("RE\n$reqid\n2\n$strval\n");
//
//		} elsif($str =~ /^UR ([^ ]+) ([^ ]+)$/) { # update routing - for 0.27's adding
//							# MFNodes - to get touchsensors, etc, in there
//
//			# we should do things with this, rather than go through the
//			# whole scene graph again... JAS.
//			my($id, $field) = ($1, $2);
//			my $v = (shift @lines)."\n";
//		        # JS - sure hope we can remove the trailing whitespace ALL the time...
//  			$v =~ s/\s+$//; # trailing new line....
//
//			my $cnode = VRML::Handles::get($v);
//			$this->{B}->api__updateRouting($cnode, $field);
//
//			# are there any routes?
//         		#AK if (defined $cnode->{SceneRoutes}) {
//				# print "VRMLServ.pm - UR ROUTE ",
//        		        #  $cnode->{SceneRoutes}[0][0] , " ",
//        		        #  $cnode->{SceneRoutes}[0][1] , " ",
//        		        #  $cnode->{SceneRoutes}[0][2] , " ",
//        		        #  $cnode->{SceneRoutes}[0][3] , " from $this to node: $cnode\n";
//
//				#AK my $scene = $this->{B}{Scene};
//	
//	             #AK if($field eq "removeChildren") {
//					#AK my $item;		
//					#AK foreach $item (@{$cnode->{SceneRoutes}}) {
//						# print "deleting route ",$_[0], $_[1], $_[2], $_[3],"\n";
//						#AK $scene->delete_route($item);
//					#AK }
//				#AK } else {
//					#AK my $item;		
//					#AK foreach $item (@{$cnode->{SceneRoutes}}) {
//						#AK my ($fn,$ff,$tn,$tf) = @{$item};
//						# print "VRMLServ.pm - expanded: $fn $ff $tn $tf\n";
//						#my $fh = VRML::Handles::return_def_name($fn);
//        					#my $th = VRML::Handles::return_def_name($tn);
//
//						#my $fh = VRML::Handles::get($fn);
//						#my $th = VRML::Handles::get($tn);
//						
//						#AK my @ar=[$fn,$ff,$tn,$tf];
//						#AK $scene->new_route(@ar);
//					#AK }
//				#AK }
//			# } else {
//			# 	print "VRMLServ.pm - no routes defined\n";
//			#AK }
//
//			#AK $this->{B}->prepare2();
//			# make sure it gets rendered
//			#AK VRML::OpenGL::set_render_frame();
//
//			$hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^SC ([^ ]+) ([^ ]+)$/) { # send SFNode eventIn to node
//			my($id, $field) = ($1,$2);
//			my $v = (shift @lines)."\n";
//
//		        # JS - sure hope we can remove the trailing whitespace ALL the time...
//  			$v =~ s/\s+$//; # trailing new line....
//
//			my $node;
//			($node,$field) = find_actual_node_and_field($id,$field,1);
//
//			my $child = VRML::Handles::get($v);
//
//			if ($VRML::verbose::EAI) {
//				print "VRMLServ.pm - node $node child $child field $field\n";
//			}
//
//			#AK - #if (defined $child->{IsProto}) {
//				#AK - #my $temp = $child;
//				#AK - #$child = $child->real_node();
//				
//				#print "VRMLServ.pm - child proto got $child\n";
//			#AK - #}
//
//			# the events are as follows:
//			# VRMSServ.pm - api__sendEvent(
//			#	handle VRML::Handles::get($id); 
//			#	"children"
//			#	 array VRML::Handles::get($v) + previous children
//			#
//			# Browser.pm:api__sendEvent(
//			#	->{EV}->send_event_to (same parameters)
//			#	   ie, node, field, value
//			#
//			# Events.pm:send_event_to(
//			#	push on {ToQueue}, [parameters] 
//			#
//			# then, some time later....
//			# Browser.pm:tick calls
//			# Events.pm:propagate_events sends this eventually to
//			#
//			# Scene.pm:receive_event (this, field, value...)
//			# Tuomas' comments follow:	
//			# The FieldHash
//			#
//			# This is the object behind the "RFields" hash member of
//			# the object VRML::NodeIntern. It allows you to send an event by
//			# simply saying "$node->{RFields}{xyz} = [3,4,5]" for which
//			# calls the STORE method here which then queues the event.
//			#
//			# so, 
//			# Scene.pm:STORE (node, "children" value)
//			#	$$v = $value;
//			# 	$node->set_backend_fields ("children");
//			#
//			# Scene.pm:set_backend_fields (field)
//			#	calls make_backend for $v
//			#	takes the global $v, creates a global $f{"children"}=$v, 
//			#	and calls
//			#	$be->set_fields($this->{BackNode},\%f);
//			#	
//			# and the backend sets the fields, and everyone lives happily
//			# ever after...
//			#	
//
//			# AK - #if ($field eq "removeChildren") {
//				# AK - #if ($this->{B}->checkChildPresent($node,$child)) {
//		  			# AK - #my @av = $this->{B}->removeChild($node, $child);
//		  			# AK - #$this->{B}->api__sendEvent($node, "children",\@av);
//				# AK - #}
//			# AK - #} else {
//				# is the child already there???
//				# AK - #if ($field eq "addChildren") {
//					# AK - #$field = "children";
//				# AK - #}
//				# AK - #if (!($this->{B}->checkChildPresent($node,$child))) {
//					#JAS my @av = @{$node->{RFields}{$field}};
//					# AK - #my @av = @{$node->{Fields}{$field}};
//					# AK - #push @av, $child;
//		  			# AK - #$this->{B}->api__sendEvent($node, $field,\@av);
//				# AK - #}
//			# AK - #}
//
//			$this->{B}->api__sendEvent($node, $field, [$child]);
//
//			# make sure it gets rendered
//			VRML::OpenGL::set_render_frame();
//		    $hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^SE ([^ ]+) ([^ ]+)$/) { # send eventIn to node
//			my($id, $field) = ($1,$2);
//			my $v = (shift @lines)."\n";
//
//			# JS - sure hope we can remove the trailing whitespace ALL the time...
//  			$v =~ s/\s+$//; # trailing new line....
//
//			my $node = VRML::Handles::get($id);
//	
//			#AK # $node = VRML::Handles::front_end_child_get($node);
//
//			my ($x,$ft) = $this->{B}->api__getFieldInfo($node,$field);
//
//			# make sure it gets rendered
//			VRML::OpenGL::set_render_frame();
//	
//			($node,$field) = find_actual_node_and_field($id,$field,1);
//
//
//			if ($ft eq "SFNode"){
//				#print "VRMLServ.pm - doing a SFNode\n";
//				my $child = VRML::Handles::get($v);
//				#AK - #if (defined $child->{IsProto}) {
//					#print "VRMLServ.pm - SE child proto got\n";
//					#AK - #$child =  $child->real_node();
//				#AK - #}
//				#print "VRMLServ.pm, ft $ft child $child\n";
//				$this->{B}->api__sendEvent($node, $field, $child);
//			} else {
//			    	my $value = "VRML::Field::$ft"->parse("FOO",$v);
//		    		#print "VRMLServ.pm, at 3, sending to $node ",
//				# 	" field $field value $v\n";
//		    		$this->{B}->api__sendEvent($node, $field, $value);
//			}
//
//		} elsif($str =~ /^DN (.*)$/) { # Dispose node
//			VRML::Handles::release($1);
//		        $hand->print("RE\n$reqid\n0\n");
//
//		} elsif($str =~ /^RL ([^ ]+) ([^ ]+) ([^ ]+)$/) {
//			my($id, $field, $lid) = ($1,$2,$3);
//			my $node;
//		
//			# Register Listener - send an event if changed.
//
//			($node,$field) = find_actual_node_and_field($id,$field,0);
//
//			# print "RL, field $field, node $node\n";
//			$this->{B}->api__registerListener(
//				$node,
//				$field,
//				sub {
//					$this->send_listened($hand,
//						$node,$id,$field,$lid,
//						$_[0]);
//				}
//			);
//		        $hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^AR ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {  # addRoute
//			my($fn, $ff, $tn, $tf) = ($1,$2,$3,$4);
//
//			my $nfn;
//			my $ntn;
//			my $field;
//			($nfn, $field) = find_actual_node_and_field($fn, $ff, 0);
//			($ntn, $field) = find_actual_node_and_field($tn, $tf, 1);
//
//			#AK - #my @ar = [$nfn,$ff,$ntn,$tf];
//			my $scene = $this->{B}{Scene};
//
//			$scene->new_route($nfn, $ff, $ntn, $tf);
//
//			$this->{B}->prepare2();
//			# make sure it gets rendered
//			VRML::OpenGL::set_render_frame();
//
//
//			$hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^DR ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {  # deleteRoute
//			my($fn, $ff, $tn, $tf) = ($1,$2,$3,$4);
//			print "deleteroute, $fn $ff, $tn $tf\n";
//
//			my $scene = $this->{B}{Scene};
//			#AK - #my $fromNode = VRML::Handles::get($fn)->real_node();
//			#AK - #my $toNode = VRML::Handles::get($tn)->real_node();
//			#AK - #my @ar = [$fromNode, $ff, $toNode, $tf];
//			
//			my $nfn;
//			my $ntn;
//			my $field;
//			($nfn, $field) = find_actual_node_and_field($fn, $ff, 0);
//			($ntn, $field) = find_actual_node_and_field($tn, $tf, 1);
//
//			$scene->delete_route($nfn, $ff, $ntn, $tf);
//
//			$this->{B}->prepare2();
//
//			$hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^GNAM$/) { # Get name
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getName(), "\n");
//
//		} elsif($str =~ /^GVER$/) { # Get Version
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getVersion(), "\n");
//
//		} elsif($str =~ /^GCS$/) { # Get Current Speed
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getCurrentSpeed(), "\n");
//
//		} elsif($str =~ /^GCFR$/) { # Get Current Frame Rate
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getCurrentFrameRate(), "\n");
//
//		} elsif($str =~ /^GWU$/) { # Get WorldURL
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getWorldURL(), "\n");
//
//		} elsif($str =~ /^RW (.*)$/) { # replaceWorld
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->replaceWorld($1), "\n");
//
//		} elsif($str =~ /^LU$/) { # LoadURL
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->loadURL(), "\n");
//
//		} elsif($str =~ /^SD$/) { # set Description
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->setDescription(), "\n");
//
//		} elsif($str =~ /^CVS (.*)$/) { # Create VRML From String
//			my $vrmlcode = $1;
//			my $ll = "";
//			while ($ll ne "EOT") {
//			  $vrmlcode = "$vrmlcode $ll\n";
//                          $ll = shift @lines;
//			}
//
//			my $retval = $this->{B}->createVrmlFromString($vrmlcode);
//
//			if ($VRML::verbose::EAI) { print "CVS returns $retval\n";}
//		        $hand->print("RE\n$reqid\n0\n", $retval, "\n");
//
//		} elsif($str =~ /^CVU (.*)$/) { # Create VRML From URL
//			my $retval = $this->{B}->createVrmlFromURL($1,$2);
//			if ($VRML::verbose::EAI) { print "CVU returns $retval\n";}
//		        $hand->print("RE\n$reqid\n0\n", $retval, "\n");
//
//
//		} elsif($str =~ /^STOP$/) { # set Description
//			print "FreeWRL got a stop!!\n";
//		} else {
//			if ($str ne  "") {
//				die("Invalid EAI input: '$str'");
//			}
//		}
//	}
//	# print "ENDLINES\n";
//}
//
//sub send_listened {
//	my($this, $hand, $node, $id, $field, $lid, $value) = @_;
//
//	my $ft = $node->{Type}{FieldTypes}{$field};
//
//	if ($VRML::verbose::EAI) {
//		print "send_listened, hand $hand node $node id $id  field $field  lid $lid  value $value\n";
//		print "field type is $ft\n";
//	 	if ("ARRAY" eq ref $value) { 
//			my $item;
//		 	foreach $item (@{$value}) {
//				print "	value element $item \n";
//			}
//		}
//	}
//
//
//	
//	my $str = "VRML::Field::$ft"->as_string($value);
//
//	# position and orientation sends an event per loop, we do not need
//	# to send duplicate positions and orientations, but this may give us a bug...
//
//
//	if ($VRML::EAIServer::evvals{$lid} eq $str) {
//		return;
//	}
//
//	$VRML::EAIServer::evvals{$lid} = $str; # save it for next time.
//	$hand->print("EV\n"); # Event incoming
//	$hand->print("$lid\n");
//	$hand->print("$str\n");
//}
//
//
//1;
