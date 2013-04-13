
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

// JAS - OLDCODE #ifndef REWIRE
#include "EAI_C.h"
#include "config.h"
#include "system.h"
// JAS - OLDCODE #endif


#ifdef OLDCODE
OLDCODE #define WAIT_FOR_RETVAL ((command!=SENDEVENT) && (command!=MIDICONTROL))
#else
#define WAIT_FOR_RETVAL (command!=SENDEVENT)
#endif

static pthread_mutex_t eailock = PTHREAD_MUTEX_INITIALIZER;
#define EAILOCK pthread_mutex_lock(&eailock);
#define EAIUNLOCK pthread_mutex_unlock(&eailock);

int _X3D_FreeWRL_FD;
int _X3D_FreeWRL_Swig_FD = 0;
int _X3D_FreeWRL_listen_FD = 0;
int _X3D_queryno = 1;

int receivedData= FALSE;
/* for waiting on a socket */
fd_set rfds;
struct timeval tv;
struct timeval tv2;

void X3D_error(char *msg) {
    perror(msg);
    exit(0);
}

double mytime;

//char readbuffer[2048];
char *sendBuffer = NULL;
int sendBufferSize = 0;

/* handle a callback - this should get a line like:
 EV
1170697988.125835
31
0.877656
EV_EOT
*/

/* make a buffer large enough to hold our data */
void verifySendBufferSize (int len) {
	if (len < (sendBufferSize-50)) return;
	
	/* make it large enough to contain string, plus some more, as we usually throw some stuff on the beginning. */
	while (len>(sendBufferSize-200)) sendBufferSize+=1024;
	sendBuffer = realloc(sendBuffer,sendBufferSize);
}

/* count the number of numbers on a line - useful for MFNode return value mallocs */
int _X3D_countWords(char *ptr) {
	int ct;
	
	ct = 0;
	
	while (*ptr >= ' ') {
		SKIP_CONTROLCHARS
		SKIP_IF_GT_SPACE
		ct ++;
	}
	return ct;
}
#ifdef WIN32
void *freewrlSwigThread(void* nada) {
#else
void freewrlSwigThread(void) {
#endif
	const int on=1;
	/* unused int flags; */
	int len;

	struct sockaddr_in servaddr, cliaddr;
#ifdef WIN32
    int iResult;
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        X3D_error("WSAStartup failed to load winsock2 ws2_32.dll\n");
        /* return NULL; */
        return ;
    }
#endif

	if ((_X3D_FreeWRL_listen_FD= socket(AF_INET, SOCK_STREAM, 0)) < 0) {
              X3D_error("ERROR opening swig socket");
              /* return NULL; */
              return ;
		
        }
	
	setsockopt(_X3D_FreeWRL_listen_FD, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(EAIBASESOCKET+ 500);
	
	if (bind((_X3D_FreeWRL_listen_FD), (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		X3D_error("ERROR in bind");
	}

	if (listen(_X3D_FreeWRL_listen_FD, 1024) < 0) {
		X3D_error("ERROR in listen");
		/* return NULL; */
		return ;
	}

	len = sizeof(cliaddr);

#ifdef WIN32
	_X3D_FreeWRL_Swig_FD = accept((_X3D_FreeWRL_listen_FD), (struct sockaddr*) &cliaddr, &len);
#else
	_X3D_FreeWRL_Swig_FD = accept((_X3D_FreeWRL_listen_FD), (struct sockaddr*) &cliaddr, (socklen_t *) &len);
#endif
	/* return NULL; */
	return ;
}
#include <list.h>
static s_list_t *evlist = NULL;
static s_list_t *relist = NULL;

pthread_mutex_t mut_relist = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_evlist = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_re = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t condition_relist_nonempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t condition_evlist_nonempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t condition_re_done = PTHREAD_COND_INITIALIZER;


s_list_t* _enqueue_readbuffer(s_list_t *list, char *readbuffer){
	s_list_t* item;
	char *cbline = strdup(readbuffer);
	item = ml_new(cbline);
	list = ml_append(list, item);
	return list;
}
void _enqueue_readbuffer_re(char *readbuffer)
{
	pthread_mutex_lock (&mut_relist);
	relist = _enqueue_readbuffer(relist,readbuffer);
	pthread_cond_signal(&condition_relist_nonempty);
	pthread_mutex_unlock (&mut_relist);
}
void _enqueue_readbuffer_ev(char *readbuffer)
{
	pthread_mutex_lock (&mut_evlist);
	evlist = _enqueue_readbuffer(evlist,readbuffer);
	pthread_cond_signal(&condition_evlist_nonempty);
	pthread_mutex_unlock (&mut_evlist);
}
char* dequeue_readbuffer(s_list_t **plist)
{
	s_list_t *list = *plist;
	if(list){
		char *readbuffer = ml_elem(list);
		*plist = ml_delete_self(list, list);
		return readbuffer;
	}else
		return NULL;
}
int waiting_for_RE = 0;
char *dequeue_readbuffer_wait_re()
{
	char *readbuffer;
	pthread_mutex_lock (&mut_re); //hold up ev work
	waiting_for_RE = 1; //set flag for holding up ev

	pthread_mutex_lock (&mut_relist);
	if(relist == NULL)
		pthread_cond_wait(&condition_relist_nonempty, &mut_relist);
	readbuffer = dequeue_readbuffer(&relist);
	pthread_mutex_unlock (&mut_relist);
	
	waiting_for_RE = 0;
	pthread_cond_signal(&condition_re_done); //signal ev OK
	pthread_mutex_unlock (&mut_re); //release ev work
	return readbuffer;
}
char* dequeue_readbuffer_ev(int wait)
{
	char *readbuffer;
	pthread_mutex_lock (&mut_evlist);
	if(evlist == NULL){
		if(!wait){
			pthread_mutex_unlock (&mut_evlist);
			return NULL;
		}
		pthread_cond_wait(&condition_evlist_nonempty, &mut_evlist);
	}
	readbuffer = dequeue_readbuffer(&evlist);
	pthread_mutex_unlock (&mut_evlist);
	return readbuffer;
}
/* Dave says: 
	"If you get an EV while still waiting for any REs you should not
	handle the EV until the RE queue is empty and you are not expecting
	any REs."
	In sendToFreewrl we send, then immediately set waiting_for_RE if/while
	we are waiting, and here we skip/return if that's the case.
*/
void dequeue_callback_ev(int wait)
{
	char* cbline;
	if(!wait)
	{
		if(waiting_for_RE) return;
		cbline = dequeue_readbuffer_ev(wait);
		if(cbline){
			_handleFreeWRLcallback (cbline) ;
			free(cbline);
		}
	}else if(wait) {
		cbline = dequeue_readbuffer_ev(wait);
		if(cbline){
			pthread_mutex_lock (&mut_re);
			if(waiting_for_RE)
				pthread_cond_wait(&condition_re_done, &mut_re);
			pthread_mutex_unlock (&mut_re);
			_handleFreeWRLcallback (cbline) ;
			free(cbline);
		}
	}
}
/* you'd start this thread if you have no main.c to call dequeue_callback_ev() from,
   such as SAI in javascript.
*/
void freewrlEVcallbackThread(void) {
	while(1){
		dequeue_callback_ev(1);
	}
}

/* read in the reply - if it is an RE; it is the reply to an event; if it is an
   EV it is an async event */
void freewrlReadThread(void)  {
	int retval;
	char readbuffer[2048];
	//initialize_queue_mutexes();
	while (1==1) {


        tv.tv_sec = 0;
        tv.tv_usec = 100;
        FD_ZERO(&rfds);
        FD_SET(_X3D_FreeWRL_FD, &rfds);

        /* wait for the socket. We HAVE to select on "sock+1" - RTFM */
		/* WIN32 ignors the first select parameter, just there for berkley compatibility */
        // retval = select(_X3D_FreeWRL_FD+1, &rfds, NULL, NULL, &tv); //times out
		retval = select(_X3D_FreeWRL_FD+1, &rfds, NULL, NULL, NULL); //blocking
		if(retval)printf("+");
		else printf("-");
        if (retval) {
#ifdef WIN32
			retval = recv(_X3D_FreeWRL_FD, readbuffer, 2048, 0);
			if (retval  == SOCKET_ERROR) {
#else
			retval = read(_X3D_FreeWRL_FD,readbuffer,2048);
			if (retval  <= 0) {
#endif
				printf("ERROR reading fromsocket\n");
				exit(1);
			}
			readbuffer[retval] = '\0';

			/* if this is normal data - signal that it is received */
			if (strncmp ("RE",readbuffer,2) == 0) {
				if(0)
					receivedData = TRUE;
				if(1)
					_enqueue_readbuffer_re(readbuffer);
			} else if (strncmp ("EV",readbuffer,2) == 0) {
				if(0)
					_handleFreeWRLcallback(readbuffer);
				if(1)
					_enqueue_readbuffer_ev(readbuffer);
			} else if (strncmp ("QUIT",readbuffer,4) == 0) {
				exit(0);
			} else {
				printf ("readThread - unknown prefix - %s\n",readbuffer);
			}

        }

	}
}

/* threading - we thread only to read from a different thread. This
allows events and so on to go quickly - no return value required. */
char readbuffer[2048];

static char *sendToFreeWRL(char *callerbuffer, int size, int waitForResponse) {
	int retval;
	int readquery;
	char *ptr;

	#ifdef VERBOSE
	printf ("sendToFreeWRL - sending :%s:\n",callerbuffer);
	#endif

#ifdef WIN32
	ptr = NULL;
	receivedData = FALSE;
	retval = send(_X3D_FreeWRL_FD, callerbuffer, size, 0);
	if (retval == SOCKET_ERROR ) 
#else	
	ptr = NULL;
	retval = write(_X3D_FreeWRL_FD, callerbuffer, size);
	if (retval < 0) 
#endif
		 X3D_error("ERROR writing to socket");

	if (waitForResponse) {

		//receivedData = FALSE;
		if(0)
		while (!receivedData) {
			sched_yield();
		}
		if(1){
			char *rb;
			//sched_yield();
			rb = dequeue_readbuffer_wait_re();
			strcpy(readbuffer,rb);
			free(rb);
		}


		/* have the response here now. */

		#ifdef VERBOSE
		printf("Client got: %s\n",readbuffer);
		#endif
		
		/* should return something like: RE
1165347857.925786
1
0.000000
RE_EOT
*/
		/* see if it is a reply, or an event return */


		ptr = readbuffer;
		while ((*ptr != '\0') && (*ptr <= ' ')) ptr++;

		#ifdef VERBOSE
		printf ("found a reply\n");
		#endif

		SKIP_IF_GT_SPACE
		SKIP_CONTROLCHARS
		if (sscanf(ptr,"%lf",&mytime) != 1) {
			printf ("huh, expected the time, got %s\n",ptr);
			exit(1);
		}
		#ifdef VERBOSE
		printf ("time of command is %lf\n",mytime);
		#endif

		SKIP_IF_GT_SPACE
		SKIP_CONTROLCHARS

		#ifdef VERBOSE
		printf ("this should be the query number: %s\n",ptr);
		#endif

		if (sscanf(ptr,"%d",&readquery) != 1) {
			printf ("huh, expected the time, got %s\n",ptr);
			exit(1);
		}
		#ifdef VERBOSE
		printf ("query returned is %d\n",readquery);
		#endif

		if (_X3D_queryno != readquery) {
			printf ("server: warning, _X3D_queryno %d != received %d\n",_X3D_queryno,readquery);
			usleep(5000);
			sched_yield();
		}

		SKIP_IF_GT_SPACE
		SKIP_CONTROLCHARS


		strncpy(callerbuffer,readbuffer,retval);

	}
	_X3D_queryno ++;
	return ptr;
}


void _X3D_sendEvent (char command, char *string) {
        char *myptr;
	EAILOCK
	verifySendBufferSize (strlen(string));
        sprintf (sendBuffer, "%d %c %s\n",_X3D_queryno,command,string);
        myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
	EAIUNLOCK
}

char *_X3D_makeShortCommand (char command) {
	char *myptr;

	EAILOCK
	verifySendBufferSize (100);
	sprintf (sendBuffer, "%d %c\n",_X3D_queryno,command);
	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
	EAIUNLOCK
	#ifdef VERBOSE
	printf ("makeShortCommand, buffer now %s\n",myptr);
	#endif
	return myptr;
}

char *_X3D_make1VoidCommand (char command, int adr) {
	char *myptr;

	EAILOCK
	verifySendBufferSize (100);
	sprintf (sendBuffer, "%d %c %d\n",_X3D_queryno,command,adr);
	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
	EAIUNLOCK
	#ifdef VERBOSE
	printf ("make1VoidCommand, buffer now %s\n",myptr);
	#endif
	return myptr;
}

char *_X3D_make1StringCommand (char command, char *name) {
	char *myptr;
	
	EAILOCK
	verifySendBufferSize (strlen(name));
	sprintf (sendBuffer, "%d %c %s\n",_X3D_queryno,command,name);
	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
	EAIUNLOCK
	#ifdef VERBOSE
	printf ("make1StringCommand, buffer now %s\n",myptr);
	#endif
	return myptr;
}

char *_X3D_make2StringCommand (char command, char *str1, char *str2) {
	char *myptr;
	char sendBuffer[2048];
	
	EAILOCK
	verifySendBufferSize ( strlen(str1) + strlen(str2));
	sprintf (sendBuffer, "%d %c %s%s\n",_X3D_queryno,command,str1,str2);
	printf("before sendToFreeWRL\n");
	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
	printf("after sendToFreeWRL myptr=%s\n",myptr);
	EAIUNLOCK

	#ifdef VERBOSE
	printf ("make2StringCommand, buffer now %s\n",myptr);
	#endif
	return myptr;
}


char *_X3D_Browser_SendEventType(int adr,char *name, char *evtype) {
	char *myptr;

	EAILOCK
	verifySendBufferSize (100);
	sprintf (sendBuffer, "%d %c %d %s %s\n",_X3D_queryno, GETFIELDTYPE, adr, name, evtype);

	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),TRUE);
	EAIUNLOCK
	#ifdef VERBOSE
	printf ("_X3D_Browser_SendEventType, buffer now %s\n",myptr);
	#endif
	return myptr;
}

char * _RegisterListener (X3DEventOut *node, int adin) {
	char *myptr;
	

	verifySendBufferSize (100);
	#ifdef VERBOSE
	printf ("in RegisterListener, we have query %d advise index %d nodeptr %d offset %d datatype %d datasize %d field %s\n",
		_X3D_queryno,
                adin, node->nodeptr, node->offset, node->datatype, node->datasize, node->field);
	#endif

/*
 EAIoutSender.send ("" + queryno + "G " + nodeptr + " " + offset + " " + datatype +
                " " + datasize + "\n");
*/
	EAILOCK
	sprintf (sendBuffer, "%u %c %d %d %c %d\n",
		_X3D_queryno, 
		REGLISTENER, 
		node->nodeptr,
		node->offset,
		mapFieldTypeToEAItype(node->datatype),
		node->datasize);

	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),TRUE);
	EAIUNLOCK
	#ifdef VERBOSE
	printf ("_X3D_Browser_SendEventType, buffer now %s\n",myptr);
	#endif
	return myptr;
}

