/*******************************************************************
 *  Copyright (C) 2002 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 *
 *******************************************************************/

#include "soundheader.h"

int 	dspFile = -1;		// Sound output device
int 	dspBlockSize = 0;	// blocking size for output
char 	*dspBlock = NULL;	// a block to send

// Fragment parameters
int n_fragments = 12; /* number of fragments */
int fragment_size = 8; /* a buffersize of 2^8 = 256 bytes */
long int bytes_remaining;	// how many bytes are remaining in the file?
int readSize;			// how much to read from wav file - either BUFSIZE or less

// how much data was used since last write.
int DSPbufferSize = -1;

void playWavFragment(SNDFILE *wavfile) {
	audio_buf_info leftover;
	int mydata;			// DSP buffer size... used to keep data flowing
	int bytes_read; 		// amount read in from the file - temp variable
	//int tmp;

	// Only write if there is the need to write data. - dont want
	// to buffer too much; want sound to be responsive but smooth

	if (ioctl(dspFile, SNDCTL_DSP_GETOSPACE,&leftover) <0) {
		printf ("error, SNDCTL_DSP_GETOSPACE\n");
		dspFile = -1;
	}
	//printf ("leftover %d fragsize %d canwrite %d ",leftover.fragments,leftover.fragsize,
	//		leftover.bytes);

	if (DSPbufferSize < 0) {
		DSPbufferSize =  leftover.bytes;
		mydata = 0; // lets guess - first time through.

		fseek (wavfile->fd, wavfile->wavdataoffset, SEEK_SET);
	} else {
		mydata = DSPbufferSize - leftover.bytes;
	}

	//printf ("amt writ %d \n",mydata);
	
	// Should we write? multiply bytes written by three, so there is a bit
	// of a buffer in the soundcard.
	if (mydata < (BUFSIZE*NUMBUFS)) {

		// ok - we should write. Get the next bit of data
		//printf ("bufsize %x, data chunk size %lx remaining %lx\n",
		//		BUFSIZE,wavfile->DataChunk.chunkSize,bytes_remaining);	

		//printf ("bytes_remaining %ld readSize %d\n",bytes_remaining,readSize);
		if (bytes_remaining <= 0) {
			//printf ("EOF input, lets reset and re-read\n");
			fseek (wavfile->fd, wavfile->wavdataoffset, SEEK_SET);
			bytes_remaining = wavfile->DataChunk.chunkSize;
		}

		// Are we reaching the end of the file?
		if (bytes_remaining < BUFSIZE) {
			readSize = (int) bytes_remaining;
			bytes_remaining = 0;
		} else {
			readSize = BUFSIZE;
			bytes_remaining -= BUFSIZE;
		}

			
		bytes_read = BUFSIZE*fread(wavfile->data,readSize,1,wavfile->fd);	

		//printf ("bytes read from index %x\n",wavfile->wavdataoffset);
		//for (tmp=0; tmp<readSize; tmp++) {
		//	printf ("\n %x: ",tmp);
		//	printf ("%x",(wavfile->data[tmp] & 0xff));
		//}
		//printf ("\n");

		write (dspFile, wavfile->data, readSize);
	}
}



// WAV file header read in, lets get the rest of the data ready.
SNDFILE *initiateWAVSound (SNDFILE *wavfile) {
	wavfile->type=WAVFILE;
	return wavfile;
}

// Close the DSP, release memory.
void closeDSP () {
	setMixerGain(0.0);
	if (dspBlock!=NULL) free(dspBlock);
	if (dspFile>=0) close(dspFile);
	dspFile = -1;
	DSPbufferSize = -1;
}

void initiateDSP() {
    int i;

    if ( (dspFile = open("/dev/dsp",O_WRONLY)) 
                                   == -1 ) {
        perror("open /dev/dsp");
        return;
    }

    i = (n_fragments<<16) | fragment_size;
    if ( ioctl(dspFile, SNDCTL_DSP_SETFRAGMENT,
                             &i) == -1 ) {
        perror("ioctl set fragment");
        return ;
    }
    return ;
}


// Different WAV files may have different representations; set the
// DSP for this file.
void selectWavParameters (SNDFILE *wavfile) {
	unsigned long ltmp;
	int tmp;

	// first - is the DSP open?
	if (dspFile<0) return;

	//second - find out how many bytes in the data chunk
	//JAS bytes_remaining = wavfile->DataChunk.chunkSize;
	bytes_remaining = -1; 

	// third - set the bit size
	tmp = wavfile->FormatChunk.wBitsPerSample;
	//printf ("SNDCTL_DSP_SAMPLESIZE %d\n",tmp);
	if (ioctl(dspFile,SNDCTL_DSP_SETFMT,&tmp)<0) {
		printf ("unable to set DSP bit size to %d\n",tmp);
		dspFile = -1; // flag an error
		//JAS return;
	}	


	// fourth - set mono or stereo
	tmp = wavfile->FormatChunk.wChannels-1;
	//printf ("SNDCTL_DSP_STEREO %d channels %d\n",tmp, wavfile->FormatChunk.wChannels);
	if (ioctl(dspFile,SNDCTL_DSP_STEREO,&tmp)<0) {
		printf ("unable to set mono/stereo mode to %d\n",tmp);
		dspFile = -1; // flag an error
		//JAS return;
	}	

	// second - set the sampling rate
	ltmp = wavfile->FormatChunk.dwSamplesPerSec;
	//printf ("SNDCTL_DSP_SPEED %ld\n",ltmp);
	if (ioctl(dspFile,SNDCTL_DSP_SPEED,&ltmp)<0) {
		printf ("unable to set DSP sampling rate to %ld\n",ltmp);
		dspFile = -1; // flag an error
		return;
	}	

	//printf ("SNDCTL_DSP_SYNC,0\n");
	if (ioctl(dspFile,SNDCTL_DSP_SYNC,0)!= 0) {
		printf("unable to set sync on dsp\n");
		dspFile = -1; // flag an error
		//JAS return;
	}	

	//printf ("selectWavParameters - returning...\n");
	return;
}

	
