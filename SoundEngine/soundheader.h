#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "/usr/include/unistd.h"
#include <errno.h>
#include <fcntl.h>
#ifndef __APPLE__
#include <malloc.h>
#else
#include <sys/malloc.h>
#endif
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#ifndef __APPLE__
#include <sys/msg.h>
#include <linux/soundcard.h>
#else
#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include "AudioFilePlay.h"
#include <sys/uio.h>
#endif
#include <sys/sem.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <assert.h>
#include <math.h>

#define  UNKNOWNFILE 0
#define  WAVFILE 3 
#define	 MPGFILE 1
#define  MP3FILE 2


#define DSP "/dev/dsp"

#define MAXSOURCES 50
#define BUFSIZE  512
#define NUMBUFS 2 


/* number of fragments */
#define N_FRAGMENTS 	48 
/* a buffersize of 2^8 = 256 bytes */
#define FRAG_SIZE 	8 


#define FormatID 'fmt '   /* chunkID for Format Chunk. NOTE: There is a space at the end of this ID. */

typedef struct {
	char           chunkID[4];
	long           chunkSize;
	short          wFormatTag;
	unsigned short wChannels;
	unsigned long  dwSamplesPerSec;
	unsigned long  dwAvgBytesPerSec;
	unsigned short wBlockAlign;
	unsigned short wBitsPerSample;
} fmtChnk;


#define DataID 'data'  /* chunk ID for data Chunk */

typedef struct {
	char	chunkID[4];
	long           chunkSize;
//JAS	unsigned char  waveformData[];
} datChnk;


typedef struct {
	        long    mtype;  // message type
	        char    msg[256]; // message data
} FWSNDMSG;

typedef struct {
	int	type;		// 1 = wav, etc, etc
	FILE	*fd;		// file descriptor of sound file
	char	data[BUFSIZE];	// data read in from file
	int	dataptr;	// where in the data field we are
	int	wavdataoffset;	// this is the offset from start of file to wave data
	float	pitch;		// pitch multiplier
	long int bytes_remaining;	// how many bytes until EOF is reached - used in 
					// playing the file
	fmtChnk	FormatChunk;	// the "fmt " chunk is copied here
	datChnk DataChunk;	// the one and only one "data" chunk is copied here
} SNDFILE;

#ifndef ALREADYHERE
#define ALREADYHERE
extern int loop[MAXSOURCES];
extern char theFile[310];
#ifdef __APPLE__
extern AudioUnit theUnit;
#endif
#endif
extern int dspFile;
void closeDSP();
SNDFILE *initiateWAVSound (SNDFILE *wavfile); 
void selectWavParameters (SNDFILE *wavfile); 
void playWavFragment (SNDFILE *wavfile, int currentSource);
void initiateDSP ();
void rewind_to_beginning (SNDFILE *wavfile);

// Mixer proto defs
void closeMixer();
void setMixerGain(float mygain);
