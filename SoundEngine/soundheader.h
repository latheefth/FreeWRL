#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <assert.h>
#include <linux/soundcard.h>
#include <math.h>

#define  UNKNOWNFILE 0
#define  WAVFILE 3 
#define	 MPGFILE 1
#define  MP3FILE 2


#define DSP "/dev/dsp"

#define MAXSOURCES 50
#define BUFSIZE 1024 
#define NUMBUFS 2 

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
	fmtChnk	FormatChunk;	// the "fmt " chunk is copied here
	datChnk DataChunk;	// the one and only one "data" chunk is copied here
} SNDFILE;

extern int dspFile;
void closeDSP();
SNDFILE *initiateWAVSound (SNDFILE *wavfile); 
void selectWavParameters (SNDFILE *wavfile); 
void playWavFragment (SNDFILE *wavfile);
void initiateDSP ();


// Mixer proto defs
int openMixer();
void closeMixer();
void setMixerGain(float mygain);


