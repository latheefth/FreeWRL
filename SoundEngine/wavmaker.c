/*******************************************************************
 *  Copyright (C) 2002 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 *
 *******************************************************************/

#include "soundheader.h"

int 	dspFile = -1;		// Sound output device
char 	*dspBlock = NULL;	// a block to send

// Fragment parameters
int readSize;			// how much to read from wav file - either BUFSIZE or less

// are we playing a sound?
int DSPplaying = -1;

// How many bytes have been used since the last cycle?
int bytesPerCycle = 0;

// how many bytes can we write to the sound card assuming no sound playing?
int soundcardBufferEmptySize = 0;
int soundcardBufferCurrentSize = 0;

// if FreeWRL's fps is too low, we may have to read and write more than one
// block from the wav file per update. The following variable tells us how
// many to do.
int loopsperloop = 1;


void playWavFragment(SNDFILE *wavfile, int source) {
	audio_buf_info leftover;
	int mydata;			// DSP buffer size... used to keep data flowing
	int tmp;
	
	// Only write if there is the need to write data. - dont want
	// to buffer too much; want sound to be responsive but smooth
	// -- this call tells us how much room there is.


	// Find out how much data was processed by the sound card since the
	// last write. First time through we assume that the sound card
	// buffer is flushed, and that we have to write data.	
	
	//printf ("start of playWavFragment source %d\n",source);

	if (DSPplaying != 0) {
		// first time through
		//printf ("first time through\n");
		DSPplaying =  0;
		mydata = 0; 
		readSize = 0;
		bytesPerCycle = BUFSIZE; // make an assumption.
		loopsperloop = 1;
		rewind_to_beginning (wavfile);
	} else {
		// we have done this before since the file open...
		mydata = soundcardBufferEmptySize - soundcardBufferCurrentSize;
		//printf ("SCES %d SCCS %d mydata %d bytes_remaining %ld\n",
		//		soundcardBufferEmptySize,
		//		soundcardBufferCurrentSize,mydata,wavfile->bytes_remaining);


		// lets try some scaling here.
		// did we (or are we close to) running out of data?
		if ((mydata <= 0x4ff) && 
				(bytesPerCycle < BUFSIZE*16) &&
				(wavfile->bytes_remaining > bytesPerCycle)) {
			//printf ("increasing bps\n");
			bytesPerCycle += 0x100;
			loopsperloop += 1;
		}
	}
	//printf ("bps %d\n",bytesPerCycle);

	// Should we read and write? 
	if (mydata <= (bytesPerCycle*2)) {

		// ok - we should write. Get the next bit of data

		// Calculate if we are going to go past the EOF marker,
		// and if so, go back to the beginning. (assume loop=true)
		//
		if (wavfile->bytes_remaining <= 0) {
			//printf ("EOF input, lets reset and re-read\n");
		       if (loop[source] == 1) {
			       rewind_to_beginning(wavfile);
			} else {
				// dont loop - just return
				return;
			}
		}

		// Are we reaching the end of the file? Lets calculate the 
		// size of the WAV file read we are going to do, and adjust
		// where we are in the read of the WAV file. Note that for
		// really short files, this works, too!
		//
		//

		for (tmp = 0; tmp < loopsperloop; tmp++) {
			if (wavfile->bytes_remaining < BUFSIZE) {
				readSize = (int) wavfile->bytes_remaining;
				wavfile->bytes_remaining = 0;
			} else {
				readSize = BUFSIZE;
				wavfile->bytes_remaining -= BUFSIZE;
			}

			// read and write here.
			if (wavfile->bytes_remaining > 0) {
				fread(wavfile->data,readSize,1,wavfile->fd);	
				write (dspFile, wavfile->data, readSize);
			}
		}
	}

	if (ioctl(dspFile, SNDCTL_DSP_GETOSPACE,&leftover) <0) {
		printf ("error, SNDCTL_DSP_GETOSPACE\n");
		dspFile = -1;
	}
	soundcardBufferCurrentSize = leftover.bytes;
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
	DSPplaying = -1;
}

void initiateDSP() {
	int i;
	audio_buf_info leftover;

	if ( (dspFile = open("/dev/dsp",O_WRONLY)) 
                                   == -1 ) {
		printf ("open /dev/dsp problem\n");
		dspFile=-1;
		return;
	}

	i = (N_FRAGMENTS<<16) | FRAG_SIZE;
	if ( ioctl(dspFile, SNDCTL_DSP_SETFRAGMENT,
                             &i) == -1 ) {
		printf("ioctl set fragment problem\n");
		dspFile=-1;
		return ;
	}

	// quick calculation to find out how much space the DRIVER thinks
	// we have when the sound card buffer is empty.
	if (ioctl(dspFile, SNDCTL_DSP_GETOSPACE,&leftover) <0) {
		printf ("error, SNDCTL_DSP_GETOSPACE\n");
		dspFile = -1;
	}
	soundcardBufferEmptySize = leftover.bytes;
	//printf ("can write a possible amount of %d bytes\n",leftover.bytes);

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
	//second and a half - make sure we are at the beginning of data
	rewind_to_beginning (wavfile);

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
	ltmp = (long int) ((float) wavfile->FormatChunk.dwSamplesPerSec * wavfile->pitch);
	//printf ("SNDCTL_DSP_SPEED %ld from %ld pitch %f \n",ltmp,
	//		wavfile->FormatChunk.dwSamplesPerSec,wavfile->pitch);
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

	
