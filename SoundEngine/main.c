/*********************************************************************
 *
 * FreeWRL SoundServer engine 
 *
 *  Copyright (C) 2002 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 *
 *
 * 
 * Wav decoding data came from data sheets at:
 * http://www.borg.com/~jglatt/tech/wave.htm
 *
 * Some programming info from:
 * http://vengeance.et.tudelft.nl/ecfh/Articles/devdsp-0.1.txt
 *
 *
 *********************************************************************/

#include "soundheader.h"


#define USE_THREAD_TO_MONITOR
#ifdef USE_THREAD_TO_MONITOR
#include <pthread.h>

static pid_t freewrlPID;
static pthread_t freewrlMonitor;
void startFreeWRLMonitor(char *);
#endif


key_t IPCKey;
#ifndef __APPLE__
int msq_fromclnt;
int msq_toclnt;
#else
char* serverpipe = "/tmp/soundserver";
char* clientpipe = "/tmp/soundclient";
int client_pipe_fd, server_pipe_fd;
#endif

int current_max = -1;		// the maximum source number recieved so far.
int current_loudest = -1;	// current loudest sound to play

int registered[MAXSOURCES];	// is source registered? (boolean)
int active[MAXSOURCES];		// is source active? (boolean)
int loop[MAXSOURCES];		// is this sound looped? (boolean)
float ampl[MAXSOURCES];		// Gain of this sound
char *names[MAXSOURCES];	// name of url for this file
SNDFILE *sndfile[MAXSOURCES];	// structure containing sound file info

FWSNDMSG msg;	// incoming message
char theFile[310];
char theFiles[MAXSOURCES][310];

int currentSource = -1;


int S_Server_IPC = -1;
int xx;



//testing...
#include <sys/types.h>
#include <sys/stat.h>
#ifndef __APPLE__
#include <sys/soundcard.h>
#endif
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

unsigned char* data;

//##########################################################################
//
// Sound file handling routines
//
//

void reversit(char* buffer, int numbytes)
{
	int i;
	char temp[4];
	numbytes -= 1;

	for (i = 0; i <= numbytes; i++)
	{
		temp[i] = buffer[numbytes-i];
	}

	for (i=0; i <= numbytes; i++)
	{
		buffer[i] = temp[i];
	}
}
// if the sndfile is open, rewind and set bytes_remaining
void rewind_to_beginning (SNDFILE *wavfile) {
	if (wavfile != NULL) {
		if (wavfile->fd != NULL) {
			wavfile->bytes_remaining = wavfile->DataChunk.chunkSize;
			fseek (wavfile->fd, wavfile->wavdataoffset, SEEK_SET);
		//} else {
		//	printf ("Wavfile not opened yet, just returning\n");
		}
	//} else {
	//	printf ("SNDfile not initialized yet\n");
	}
}



// find a chunk start.
int chunk (char *buf, char *find, int len) {
	int mycnt;

	mycnt=0;
	while (mycnt < (len) - strlen(find)) {
		if (strncmp (&buf[mycnt], find, strlen(find)) == 0) {
			// found it!
			//printf ("found %s at %d\n",find, mycnt);
			return (mycnt);
		} else {
			//printf ("not found, mycnt = %d\n",mycnt);
			mycnt ++;
		}
	}
	return -1;  // did not find it
}


// Play a sound file...
void playSound (int indexno) {

	//printf("In playSound, indexno is %d, currentSource is %d\n", indexno, currentSource);
	// New sound?
	if (currentSource != indexno) {
		// Sound already opened for another clip?
		if (currentSource >= 0) {
			closeDSP();
		}

		currentSource = indexno;
		initiateDSP();
		selectWavParameters(sndfile[currentSource]);
	}

	playWavFragment(sndfile[currentSource],currentSource);
}

// Decode what kind of file this is - skip through the headers,
// save the type in the SNDFILE structure.

int querySoundType(SNDFILE *me) {
	int br;

	br = fread(me->data,1,BUFSIZE,me->fd);
	me->dataptr = chunk (me->data,"RIFF",BUFSIZE);

	//Not a RIFF file
	if (me->dataptr < 0) {
		printf ("not a RIFF file\n");
		return -1;
	}
	
	br = chunk (&me->data[me->dataptr],"WAVE",BUFSIZE);
	// a WAVE file
	if (br < 0) {
		printf ("not a WAVE file\n");
		return -1;
	}

	me->dataptr +=br;
	br = chunk (&me->data[me->dataptr],"fmt ",BUFSIZE);
	// have format
	if (br < 0) {
		printf ("no fmt found in WAVE file\n");
		return -1;
	}

	me->dataptr += br;

	// copy over format header information
	memcpy (&me->FormatChunk, &me->data[me->dataptr], sizeof (fmtChnk));
#ifdef AQUAPPC
 	reversit((char*)&(me->FormatChunk.wChannels), 2);	
	reversit((char*)&(me->FormatChunk.wFormatTag), 2);
	reversit((char*)&(me->FormatChunk.dwSamplesPerSec), 4);
	reversit((char*)&(me->FormatChunk.chunkSize), 4);
	reversit((char*)&(me->FormatChunk.dwAvgBytesPerSec), 4);
	reversit((char*)&(me->FormatChunk.wBlockAlign), 2);
	reversit((char*)&(me->FormatChunk.wBitsPerSample), 2);
#endif
	
	

	//printf("size of format %ld size of channels %ld size of samplespersec %ld\n", sizeof(me->FormatChunk.wChannels), sizeof(me->FormatChunk.wFormatTag), sizeof(me->FormatChunk.dwSamplesPerSec));
	//printf ("fmt chunkid %c%c%c%c\n",me->FormatChunk.chunkID[0], me->FormatChunk.chunkID[1],me->FormatChunk.chunkID[2],me->FormatChunk.chunkID[3]);
	//printf ("fmt chunkSize %ld\n", me->FormatChunk.chunkSize);
	//printf ("fmt wChannels %d\n", me->FormatChunk. wChannels);
	//printf ("fmt wFormatTag %d\n", me->FormatChunk. wFormatTag);
	//printf ("fmt dwSamplesPerSec %ld\n", me->FormatChunk. dwSamplesPerSec);
	//printf ("fmt dwAvgBytesPerSec %ld\n", me->FormatChunk. dwAvgBytesPerSec);
	//printf ("fmt wBlockAlign %d\n", me->FormatChunk. wBlockAlign);
	//printf ("fmt wBitsPerSample %d\n", me->FormatChunk. wBitsPerSample);


	// is this file compressed?
	if (me->FormatChunk. wFormatTag != 1) {
		printf ("WAV file is compressed - dont handle this yet\n");
		return -1;
	}

	// pass over the fmt chunk - note the chunkSize does not include all. see spec.
	me->dataptr += 8 + me->FormatChunk.chunkSize;
	

	br = chunk (&me->data[me->dataptr],"data",BUFSIZE);
	// have data
	if (br < 0) {
		printf ("no data found in WAVE file\n");
		return -1;
	}

	me->dataptr += br;
	memcpy (&me->DataChunk, &me->data[me->dataptr], sizeof (datChnk));

#ifdef __APPLE__
	reversit((char*)&(me->DataChunk.chunkSize), 4);
#endif

 	//printf ("data chunkid %c%c%c%c\n",me->DataChunk.chunkID[0], me->DataChunk.chunkID[1],me->DataChunk.chunkID[2],me->DataChunk.chunkID[3]);
	//printf ("data chunkSize %ld\n", me->DataChunk.chunkSize);
	//printf ("actual number of sample frames %ld\n",me->DataChunk.chunkSize/me->FormatChunk.wBlockAlign);
	//printf ("dataptr is %d\n",me->dataptr);


	me->wavdataoffset = me->dataptr+8; // wavdataoffset is the actual position of start of data
	return WAVFILE;
}

// Open and initiate sound file
	
SNDFILE *openSound (char *path) {

	SNDFILE *mysound;
       
	mysound	= (SNDFILE *) malloc (sizeof(SNDFILE)); // This is the return value
	
	if (!mysound) return NULL;	// memory allocation error

	mysound->fd = fopen(path,"r");

	if (mysound->fd == NULL) {
		free (mysound);
		return NULL;
	}

	// ok - we have the file opened. Assume WAV file, because that's
	// what we handle for now.

	switch (querySoundType(mysound)) {
		case WAVFILE: {
					return initiateWAVSound(mysound);
					break;
				}
		case MP3FILE: {
				     	mysound->type = MP3FILE;
					break;
				}
		case MPGFILE: {
				     	mysound->type = MPGFILE;
					break;
				}
		default: {
				 printf ("unknown file type: %s\n",path);
				 free (mysound);
				 return NULL;
			}
		}
	
	// we should never reach here...
	return NULL;
}


//##########################################################################
//
// Receive information from FreeWRL

void
toclnt(char *message_to_send) {
	msg.mtype= 1;
	(void) strcpy(msg.msg, message_to_send);
	//printf ("SoundEngine - sending back %s\n",msg.msg);

#ifndef __APPLE__
	while((xx=msgsnd(msq_toclnt, &msg,strlen(msg.msg)+1,IPC_NOWAIT)) != 0);	
#else
	xx = write(client_pipe_fd, &msg, sizeof(msg));
	if (xx > 0)
		xx = 0;
#endif
	if (xx) {   /* Send to client */
		perror("client error");
		printf ("SoundEngineServer - error sending ready msg\n");
		exit(1);
	}
	//printf ("SoundEngine - sent back %s\n",msg.msg);
}

int fromclnt () {
#ifndef __APPLE__
		return msgrcv(msq_fromclnt,&msg,256,1,0);
#else
	return read(server_pipe_fd, &msg, sizeof(msg));
#endif
}


// Go through, and act on the message -it is stored in the global "msg" struct
void process_command () {
	float x,y,z; // temporary variables 
	int a,b,c;   // temporary variables
	char cp[310];    // temporary variable
	double duration;

	//printf ("processing %s\n",msg.msg);

	if (strncmp ("REGS",msg.msg,4) == 0) {
		// a REGISTER message
		a=5; b=0; 
		//printf ("REGS matched len %d, first start %c %c %c %c\n",strlen(msg.msg), msg.msg[a],msg.msg[a+1], msg.msg[a+2], msg.msg[a+3]);
		
		
		// copy over the url name; skip past the REGS: at beginning.
		while ((a<strlen(msg.msg)-1) && (b<300) && (msg.msg[a]>' ')) {
			cp[b]=msg.msg[a];
			theFile[b] = msg.msg[a];
			b++; a++;
		}
		cp[b]='\0';
		theFile[b] = '\0';
		//printf ("copy name finished - it is %s len %d\n",cp,b);
		
		sscanf (&msg.msg[a], " %d %d %f %f %f",&a,&b,&x,&y,&z); 
		strcpy(theFiles[a], theFile);
	
		//printf ("registering source %d loop %d x %f y %f z %f name %s \n",a,b,x,y,z,cp);	
		//printf ("set source for %d to %s\n", a, theFiles[a]);

		if (a > current_max) current_max = a;

		// Can we open this sound file?

		//printf ("REGS opening sound\n");
		sndfile[a] = openSound(cp);

		if (sndfile[a] == NULL) {
			printf ("SoundServer - can not open %s\n",cp);
		} else {
			// Copy over all of the temporary data to the correct place.
			registered[a] = 1;     // is source registered? (boolean)
			loop[a] = b;           // is this sound looped? (boolean)
			sndfile[a]->pitch = x;          // pitch of 1 = standard playback
			ampl[a] = 0.0;         // Gain of this sound
			names[a] = (char*) malloc(strlen(cp)+2);
			for (c=0; c<=strlen(cp); c++) {
				names[a][c] = cp[c];
			}
		}

		duration = (double) sndfile[a]->DataChunk.chunkSize / (double) sndfile[a]->FormatChunk.dwAvgBytesPerSec;

		sprintf (cp, "REGS %d %f",a,(float)duration);
		toclnt(cp);			/* Tell client we're ready */
		
	} else if (strncmp ("AMPL",msg.msg,4) == 0) {
		// set amplitude for this sound source
		
		sscanf (msg.msg,"AMPL %d %f %f",&a,&x,&y);
		if ((registered[a] == 1) && (a>=0) && (a<MAXSOURCES)) {
			ampl[a] = x;
		}

		// Go through sounds, and find one with maximum gain to play.
		x = 0.0;
		current_loudest = -1;
		for (b=0; b<=current_max; b++) {
			if ((registered[b] == 1) && (active[b] == 1)) {
				if (ampl[b] > x) {
					current_loudest = b; // the one with highest gain
					x = ampl[b];
				}
			}
		}

		//printf ("AMPL recieved, current_loudest %d\n",current_loudest);
		// Now that (if) we have found the loudest, play it
		if (current_loudest >= 0) {
			if (ampl[current_loudest] > 0.01) {
				setMixerGain(ampl[current_loudest]);
				playSound (current_loudest);
				strcpy(theFile, theFiles[current_loudest]);
			}
		}
	} else if (strncmp ("ACTV",msg.msg,4) == 0) {
		// set this source to be active
		sscanf (msg.msg,"ACTV %d %d",&a,&b);
		if ((a>=0) && (a<MAXSOURCES)) {
			active[a]=b;
			if (b==1) {
				// sound is becoming active
				rewind_to_beginning (sndfile[a]);
			}
		}
		//printf ("ACTV parsing, active%d now is %d from message %s\n",a,b,msg.msg);

	} else {
		printf ("SoundEngine - unknown message recieved\n");
	}
}

int main(int argc,char **argv) {
	if (argc <1) {
		printf ("Server: too few args\n");
		exit(1);
	}


#ifdef USE_THREAD_TO_MONITOR
         startFreeWRLMonitor(argv[0]);
#endif


	// initiate tables
	for (xx=0; xx<MAXSOURCES; xx++) {
		registered[xx] = 0;
		active[xx] = 0;
		sndfile[xx] = NULL;
	}


	//printf ("Server - getting the client IPC from argv %s\n", argv[0]);
	S_Server_IPC=getppid();

	//printf ("a='%s', msg='%s', d='%d'.\n", argv[0],msg.msg,S_Server_IPC);
#ifndef __APPLE__
	if (!strncmp("INIT",argv[0],4)) {
		sscanf (argv[0],"%s%d",msg.msg,&S_Server_IPC);
	} else {
		printf ("SoundServer: no Client_IPC on command line\n");
		//printf ("a='%s', msg='%s', dud='%d'.\n", argv[0],msg.msg,dud);
		exit(1);
	}	

	// get message queues
	if ((msq_fromclnt = msgget(S_Server_IPC,0666)) < 0) {
		printf ("SoundServer: no IPC queue available\n");
		exit(1);
	}
	if ((msq_toclnt = msgget(S_Server_IPC+1,0666)) < 0) {
		printf ("SoundServer: no IPC queue available\n");
		exit(1);
	}
	//printf ("Server, ok, msq_fromclnt=%x msq_toclnt=%x key %d\n",
	//		msq_fromclnt,msq_toclnt,S_Server_IPC);
#endif
#ifdef __APPLE__
	if ((server_pipe_fd = open (serverpipe, O_RDONLY | O_NONBLOCK)) < 0) {
		if ((mkfifo(serverpipe, S_IRUSR | S_IWUSR | S_IXUSR)) < 0) {
			printf("FreeWRL:SoundClient error creating server pipe\n");
			exit(1);
		}
		//printf("made server fifo\n");
		if ((server_pipe_fd = open (serverpipe, O_RDONLY | O_NONBLOCK)) < 0) {
			printf("FreeWRL:SoundClient error opening server pipe\n");
			exit(1);
		}
		//printf("opened server fifo for reading\n");
	}
	//printf("Client created server pipe\n");
	if ((client_pipe_fd = open (clientpipe, O_WRONLY | O_NONBLOCK)) < 0) {
		printf("FreeWRL:SoundClient error opening client pipe\n");
		exit(1);
	}
	//printf("Client created client pipe\n");
#endif

	toclnt("OK");			/* Tell client we're ready */

	do {
		while (xx <= 0) {
			xx = fromclnt();
			usleep(1);
		}
		if (xx < 0) {
			// gets here if the client exited
			printf("exiting\n");
			exit (0);
		}
		xx = 0;
		
		//printf ("server, from FreeWRL=%x message='%s'\n",xx,msg.msg);
		process_command ();
	} while (strncmp ("QUIT",msg.msg,4));
	closeMixer();

	//printf ("Server exiting normally\n");
	exit(0);
}


#ifdef USE_THREAD_TO_MONITOR
void monThread(void) {
	char inl[8000];
	char *ptr;
	int i;
	FILE *rwc;
	pid_t tpid;
	int freewrlFound;
	int Version;

	#define UNKNOWN 1
	#define OSX10_4	2
	#define OSX10_5 3

	#define fwf "/tmp/freewrlReWireChecker"
	#define versFile "/System/Library/CoreServices/SystemVersion.plist"
	#define prodKeyline "<key>ProductVersion</key>"
	#define String "<string>"

	/* what version are we running under? */
	Version = UNKNOWN;

	rwc = fopen (versFile,"r");
	if (rwc != NULL) {
		while ((i=fread(inl,1,8000,rwc)) >0) {
			//printf ("version checking: just read in %d bytes\n",i); 
			inl[i] = '\0';
			//printf ("monp, %s\n",inl); 

			/* look for the "<key>ProductVersion</key>" line */
			ptr = strstr (inl,prodKeyline);
			if (ptr != NULL) {
				ptr += strlen (prodKeyline);
				
				/* look for the "<string>" */
				ptr = strstr (ptr,String);
				if (ptr != NULL) {
					ptr += strlen(String);
					/* in case there is a space in there */
					while (isspace(*ptr)) ptr++;
					//printf ("ptr now is %s\n",ptr);

					if (strncmp("10.4",ptr,4) == 0) Version = OSX10_4;
					else if (strncmp("10.5",ptr,4) == 0) Version = OSX10_5;
					
				}	
			}
		}
		fclose (rwc);
	}

	//printf ("OSX Version %d\n",Version);


	while (1==1) {
		//printf ("monThread running...looking for PID %u\n",freewrlPID); 

		/* first, the old way, OSX 10.4 and before... */
		if (Version != OSX10_5) {
			sprintf (inl, "ps -aux | grep %u > %s",freewrlPID,fwf);
		} else {
			/* ...now, the OSX 10.5 way*/
			sprintf (inl, "ps -a | grep %u > %s",freewrlPID,fwf);
		}

		//printf ("doing system on :%s:\n",inl);
		system (inl);

		rwc = fopen (fwf, "r");
		if (rwc != NULL) {
			while ((i=fread(inl,1,8000,rwc)) >0) {
				/* printf ("just read in %d bytes\n",i); */
				inl[i] = '\0';
				//printf ("monp, %s\n",inl); 

				/* go through, looking for the FreeWRL pid, to ensure that it is still running */
				ptr = inl;
				freewrlFound = FALSE;

				while ((*ptr != '\0') &&(!freewrlFound)) {
					/* Version 10.4 will have a user name as the first token... */
					if (Version != OSX10_5) {
						while (!isspace(*ptr)) ptr++;
					}
					/* printf ("scanning for uid on :%s:\n",ptr); */

					sscanf (ptr,"%u",&tpid);
					/* printf ("scanned pid %u, expected %u line %s\n",tpid,freewrlPID,ptr); */
					while (*ptr >= ' ') ptr++; /* go until a cret, or end of string */
					while ((*ptr > '\0') && (*ptr < ' ')) ptr++;

					/* did we find it? */
					freewrlFound = (tpid == freewrlPID);
				}
			}
		}

		unlink(fwf);


		/* did we find it??? */
		if (!freewrlFound) {
			printf ("freewrlReWireServer, exiting, did not find the FreeWRL process, did it crash?\n");
			exit(0);

		}
		sleep (6);
	}
}

/* FreeWRL has given us its PID, lets monitor this, and if FreeWRL crashes for any
   reason, AND, IT IS NOT CAUGHT IN OTHER WAYS, we can stop ourselves */

void startFreeWRLMonitor (char * pidstr) {
	freewrlPID = 0;

	/* look for the FreeWRL pid on argc[0]; string should be something like "INIT 334332" */
	sscanf (pidstr,"INIT %u",&freewrlPID);
	// printf ("startFreeWRLMonitor - found pid to be %u\n",freewrlPID); 

	if (freewrlPID != 0) {
		pthread_create(&freewrlMonitor, NULL, (void*(*)(void *))&monThread, NULL);
	}
}
#endif
