/*******************************************************************
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.


 This is the SoundEngine code for FreeWRL. 

 Some of this stuff came from files from "wavplay"  - see information below


*********************************************************************/


#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "Structs.h"
#include "headers.h"

#include "sounds.h"
#include <sys/msg.h>


void SoundEngineDestroy ();


int SReg[MAXSOUNDS];

int my_ipc_key;

FWSNDMSG msg;		// message buffer

char sspath[] = "/usr/local/bin/FreeWRL_SoundServer";

int initialized = SOUND_NEEDS_STARTING; // are we able to run?

// IPC stuff
int msq_toserver = -1;
int msq_fromserver = -1;
pid_t S_Server_PID;

void Sound_toserver (char *message) {
	int xx;

	if (initialized != SOUND_STARTED)  return;

	strcpy (msg.msg,message);
	printf ("Client:Sending to server %s\n",msg.msg);
        while(xx = msgsnd(msq_toserver, &msg,strlen(msg.msg)+1,IPC_NOWAIT));
        if (xx) {   /* Send to server */
                printf ("SoundEngineServer - error sending ready msg\n");
                initialized = !SOUND_STARTED;
        }
}




void SoundEngineInit () { 
	int x;
	char buf[200];
	pid_t PID;
	int proc_status;
	time_t t0, t;

	struct stat enginestat;

	// have we done this before (can happen if more than 1 sound source)
	if (initialized != SOUND_NEEDS_STARTING) return;

	// is the sound engine installed on this system?
	if (stat(sspath,&enginestat)) {
		printf ("FreeWRL: SoundEngine not installed on system\n");
		printf ("errno is %d\n",errno);
		initialized = SOUND_FAILED;
		return;
	}

	
	//my_ipc_key = getpid();
	my_ipc_key = 1234;
	msg.mtype=1;

	// initialize SoundRegistered "database"
	for (x=0; x<MAXSOUNDS; x++) SReg[x]=FALSE;

	//printf ("Client, thus queue key is %d\n",my_ipc_key);
	
	// message queue for client/server comms
	if ( (msq_toserver = msgget(my_ipc_key,IPC_CREAT|0666)) < 0 ) {
		printf ("FreeWRL:SoundServer error creating toserver message queue\n");
		initialized = SOUND_FAILED;
		return;
	}
	if ( (msq_fromserver = msgget(my_ipc_key+1,IPC_CREAT|0666)) < 0 ) {
		printf ("FreeWRL:SoundServer error creating fromserver message queue\n");
		initialized = SOUND_FAILED;
		return;
	}

	// printf ("Client - msq_toserver %d msq_fromserver %d\n",
	// 	msq_toserver,msq_fromserver);

	sprintf(buf,"INIT %d",my_ipc_key);

	if ( (S_Server_PID = fork()) == (pid_t)0L ) {
		// is this path ok?
		execl(sspath,buf,"",NULL);

		// if we got here, we have an error...
		printf("FreeWRL:SoundServer:%s: exec of %s\n",
			sys_errlist[errno],sspath);
		msgctl(msq_toserver,IPC_RMID,NULL);
		msgctl(msq_fromserver,IPC_RMID,NULL);
		initialized = SOUND_FAILED;
		return;

	} else if ( S_Server_PID < 0 ) {
		printf ("FreeWRL:SoundServer %s: error starting server process",
			sys_errlist[errno]);
		msgctl(msq_toserver,IPC_RMID,NULL);
		msgctl(msq_fromserver,IPC_RMID,NULL);
		initialized = SOUND_FAILED;
		return;
	}

	// printf ("Client: - server pid %d\n",S_Server_PID);

	// if FreeWRL actually gets to the exit stage... :-)
	atexit(SoundEngineDestroy);	

	// wait for the message queue to initialize.
	time(&t0);

	while ( 1 ) {
		int xx;

		// wait for a response - is the server telling us it is ok?
		printf ("Client: waiting for response on %d\n",msq_toserver);

		do {
			xx = msgrcv(msq_fromserver,&msg,128,1,0);
			//printf ("Client waiting... xx is %d\n",xx);
		} while (!xx);

		// printf ("message received was %s\n", msg.msg);
		if (xx) {
			 // We have a message from the server
			if ( msg.mtype == 1 ) {
				initialized = SOUND_STARTED;
				return;	// connect OK
			}
		} else	{
			while ((PID=waitpid(-1,&proc_status,WNOHANG)) == -1 
				&& errno==EINTR );
			if ( PID > 0 ) {
				printf("FreeWRL:SoundServer process ID %ld terminated: %d",
					PID,proc_status);
				initialized = SOUND_FAILED;
				return;

			} else	sleep(1);
		}

		time(&t);
		if ( t - t0 > 5 )
			break;
	}

	printf("FreeWRL:SoundServer: Timeout: starting server.");
	SoundEngineDestroy();
	return;
}

// close socket, destroy the server
void SoundEngineDestroy() {
	if (initialized == SOUND_STARTED) {
		msgctl(msq_toserver,IPC_RMID,NULL);
		msgctl(msq_fromserver,IPC_RMID,NULL);
		printf ("SoundEngineDestroy, sound was started successfully\n");
		kill(S_Server_PID,SIGTERM);
	}
	initialized = !SOUND_STARTED;
}

int SoundSourceRegistered  (int num) {
	if (num >= MAXSOUNDS) {
		printf ("Too many sounds in VRML file - max %d",num);
		return FALSE;
	}
	return SReg[num];
}

void SoundSourceInit (int num, int loop, float pitch, float start_time, float stop_time,
		char *url) {

	char mystring[512];

	if (strlen(url) > 192) {
		printf ("SoundSourceInit - url %s is too long\n",url);
		return;
	}

	sprintf (mystring,"REGS %2d %2d %4.3f %4.3f %4.3f %s",num,loop,pitch,start_time,
			stop_time,url);
	SReg[num] = TRUE;
	Sound_toserver(mystring);
}

