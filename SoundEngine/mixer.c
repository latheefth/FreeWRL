/*******************************************************************
 *  Copyright (C) 2002 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 *
 *******************************************************************/

#include "soundheader.h"
int mixerFile = -10; // -10 means "try me"
#ifdef __APPLE__
OSStatus Mresult;
#endif


void openMixer() {
#ifndef __APPLE__
	mixerFile = open("/dev/mixer",O_WRONLY);
#else
	mixerFile = 1;
#endif
}


void closeMixer() {
#ifndef __APPLE__
	close(mixerFile);
	mixerFile = -1; // -10 means, "try me".
#else
	
#endif
}


void setMixerGain(float mygain) {
	int vol;

#ifndef __APPLE__
	if (mixerFile == -10) {
		openMixer();
	}
	if (mixerFile>=0) {
		// set gain
		vol = (int) (mygain*100.0);
		if (vol < 0) vol = 0;
		if (vol > 100) vol = 100;

		if (ioctl(mixerFile,SOUND_MIXER_WRITE_PCM,&vol) == -1) {
			printf ("mixer error\n");
			closeMixer();
		}
	}
#else
	if (mixerFile == -10) {
		openMixer();
	}
	if (mixerFile >= 0) {
		//printf("setting volume to %f\n", mygain);
		Mresult = AudioUnitSetParameter(theUnit, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, mygain, 0);
		if (Mresult)
			printf("setting volume failed\n");
		Mresult = AudioUnitGetParameter(theUnit, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, &mygain);
	}
#endif
}

