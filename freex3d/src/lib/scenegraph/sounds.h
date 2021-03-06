/*


Sound engine client code

*/

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


#ifndef __FREEWRL_SOUND_CLIENT_H__
#define __FREEWRL_SOUND_CLIENT_H__

#ifdef HAVE_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#ifdef HAVE_ALUT
#include <AL/alut.h>
#endif
#endif

#define SNDMAXMSGSIZE 256

/* states of the sound engine */
#define SOUND_FAILED  2
#define SOUND_STARTED 1
#define SOUND_NEEDS_STARTING 3

#define MAXSOUNDS 50

typedef struct {
	long mtype;	/* message type */
	char	msg[SNDMAXMSGSIZE]; /* message data */
} FWSNDMSG;


void Sound_toserver(char *message);

int SoundEngineInit(void);

void waitformessage(void);

void SoundEngineDestroy(void);

int SoundSourceRegistered(int num);

float SoundSourceInit(int num,
				int loop,
				double pitch,
				double start_time,
				double stop_time,
				char *url);

void SetAudioActive(int num, int stat);
int haveSoundEngine();
/* if a Sound {} can not be found... */
#define BADAUDIOSOURCE -9999

#endif /* __FREEWRL_SOUND_CLIENT_H__ */
