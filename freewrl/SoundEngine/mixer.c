/*******************************************************************
 *  Copyright (C) 2002 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 *
 *******************************************************************/

#include "soundheader.h"
int mixerFile = -10; // -10 means "try me"


void openMixer() {
	mixerFile = open("/dev/mixer",O_WRONLY);
}


void closeMixer() {
	close(mixerFile);
	mixerFile = -1; // -10 means, "try me".
}


void setMixerGain(float mygain) {
	int vol;

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
}


