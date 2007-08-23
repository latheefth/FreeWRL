/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Sound Component

*********************************************************************/

#include <math.h>
#include "headers.h"
#include "installdir.h"
#include "LinearAlgebra.h"


void render_AudioControl (struct X3D_AudioControl *node) {
	GLdouble mod[16];
	GLdouble proj[16];
	struct pt vec, direction, location;
	double len;
	double angle;
	float midmin, midmax;

	char mystring[256];

	/*  do the sound registering first, and tell us if this is an audioclip*/
	/*  or movietexture.*/

	/*  if the attached node is not active, just return*/
	if (node->enabled == 0) return;

	direction.x = node->direction.c[0];
	direction.y = node->direction.c[1];
	direction.z = node->direction.c[2];

	location.x = node->location.c[0];
	location.y = node->location.c[1];
	location.z = node->location.c[2];

	midmin = (node->minFront - node->minBack) / 2.0;
	midmax = (node->maxFront - node->maxBack) / 2.0;


	glPushMatrix();

	/*
	first, find whether or not we are within the maximum circle.

	translate to the location, and move the centre point, depending
	on whether we have a direction and differential maxFront and MaxBack
	directions.
	*/

	glTranslatef (location.x + midmax*direction.x,
			location.y + midmax*direction.y,
			location.z + midmax * direction.z);

	/* make the ellipse a circle by scaling...
	glScalef (direction.x*2.0 + 0.5, direction.y*2.0 + 0.5, direction.z*2.0 + 0.5);
	- scaling needs work - we need direction information, and parameter work. */

	if ((fabs(node->minFront - node->minBack) > 0.5) ||
		(fabs(node->maxFront - node->maxBack) > 0.5)) {
		if (!soundWarned) {
			printf ("FreeWRL:Sound: Warning - minBack and maxBack ignored in this version\n");
			soundWarned = TRUE;
		}
	}



	fwGetDoublev(GL_MODELVIEW_MATRIX, mod);
	fwGetDoublev(GL_PROJECTION_MATRIX, proj);
	gluUnProject (viewport[2]/2,viewport[3]/2,0.0,
		mod,proj,viewport, &vec.x,&vec.y,&vec.z);
	/* printf ("mod %lf %lf %lf proj %lf %lf %lf\n",*/
	/* mod[12],mod[13],mod[14],proj[12],proj[13],proj[14]);*/

	len = sqrt(VECSQ(vec));
	/* printf ("len %f\n",len);  */
	/* printf("Sound: len %f mB %f mF %f angles (%f %f %f)\n",len,*/
	/* 	-node->maxBack, node->maxFront,vec.x,vec.y,vec.z);*/


	/*  pan left/right. full left = 0; full right = 1.*/
	if (len < 0.001) angle = 0;
	else {
		if (APPROX (mod[12],0)) {
			/* printf ("mod12 approaches zero\n");*/
			mod[12] = 0.001;
		}
		angle = fabs(atan2(mod[14],mod[12])) - (PI/2.0);
		angle = angle/(PI/2.0);

		/*  Now, scale this angle to make it between -0.5*/
		/*  and +0.5; if we divide it by 2.0, we will get*/
		/*  this range, but if we divide it by less, then*/
		/*  the sound goes "hard over" to left or right for*/
		/*  a bit.*/
		angle = angle / 1.5;

		/*  now scale to 0 to 1*/
		angle = angle + 0.5;

		/*  bounds check...*/
		if (angle > 1.0) angle = 1.0;
		if (angle < 0.0) angle = 0.0;
		/* printf ("angle: %f\n",angle); */
	}

	/* convert to a MIDI control value */
	node->panInt32Val = (int) (angle * 128);
	if (node->panInt32Val < 0) node->panInt32Val = 0; if (node->panInt32Val > 127) node->panInt32Val = 127;
	node->panFloatVal = (float) angle;


	node->volumeFloatVal = 0.0;
	/* is this within the maxFront maxBack? */

	/* this code needs rework JAS */
	if (len < node->maxFront) {
		/* did this node become active? */
		if (!node->isActive) {
			node->isActive = TRUE;
			mark_event (node, offsetof (struct X3D_AudioControl, isActive));
			printf ("AudioControl node is now ACTIVE\n");

			/* record the length for doppler shift comparisons */
			node->__oldLen = len;
		}

		/* note: using vecs, length is always positive - need to work in direction
		vector */
		if (len < 0.0) {
			if (len < node->minBack) {node->volumeFloatVal = 1.0;}
			else { node->volumeFloatVal = (len - node->maxBack) / (node->maxBack - node->minBack); }
		} else {
			if (len < node->minFront) {node->volumeFloatVal = 1.0;}
			else { node->volumeFloatVal = (node->maxFront - len) / (node->maxFront - node->minFront); }
		}

		/* work out the delta for len */
		if (APPROX(node->maxDelta, 0.0)) {
			printf ("AudioControl: maxDelta approaches zero!\n");
			node->deltaFloatVal = 0.0;
		} else {
			printf ("maxM/S %f \n",(node->__oldLen - len)/ (TickTime- lastTime));
			/* calculate change as Metres/second */

			/* compute node->deltaFloatVal, and clamp to range of -1.0 to 1.0 */
			node->deltaFloatVal = ((node->__oldLen - len)/(TickTime-lastTime))/node->maxDelta;
			if (node->deltaFloatVal < -1.0) node->deltaFloatVal = -1.0; if (node->deltaFloatVal > 1.0) node->deltaFloatVal = 1.0;
			node->__oldLen = len;
		}

		/* Now, fit in the intensity. Send along command, with
		source number, amplitude, balance, and the current Framerate */
		node->volumeFloatVal = node->volumeFloatVal*node->intensity;
		node->volumeInt32Val = (int) (node->volumeFloatVal * 128.0); 
		if (node->volumeInt32Val < 0) node->volumeInt32Val = 0; if (node->volumeInt32Val > 127) node->volumeInt32Val = 127;

		node->deltaInt32Val = (int) (node->deltaFloatVal * 64.0) + 64; 
		if (node->deltaInt32Val < 0) node->deltaInt32Val = 0; if (node->deltaInt32Val > 127) node->deltaInt32Val = 127;

		printf ("AudioControl: amp: %f (%d)  angle: %f (%d)  delta: %f (%d)\n",node->volumeFloatVal,node->volumeInt32Val,
			node->panFloatVal, node->panInt32Val ,node->deltaFloatVal,node->deltaInt32Val);

/*
                                                # need distance, pan position as ints and floats
                                                done volumeInt32Val => [SFInt32, 0, eventOut],
                                                done volumeFloatVal => [SFFloat, 0.0, eventOut],
                                                done panInt32Val => [SFInt32, 0, eventOut],
                                                done panFloatVal => [SFInt32, 0.0, eventOut],
                                                done deltaInt32Val => [SFInt32, 0, eventOut],
                                                done deltaFloatVal => [SFInt32, 0.0, eventOut],
*/

		mark_event (node, offsetof (struct X3D_AudioControl, volumeInt32Val));
		mark_event (node, offsetof (struct X3D_AudioControl, volumeFloatVal));
		mark_event (node, offsetof (struct X3D_AudioControl, panInt32Val));
		mark_event (node, offsetof (struct X3D_AudioControl, panFloatVal));
		mark_event (node, offsetof (struct X3D_AudioControl, deltaInt32Val));
		mark_event (node, offsetof (struct X3D_AudioControl, deltaFloatVal));

	} else {
		/* node just became inActive */
		if (node->isActive) {
			node->isActive = FALSE;
			mark_event (node, offsetof (struct X3D_AudioControl, isActive));
			printf ("AudioControl node is now INACTIVE\n");
		}
	}

	glPopMatrix();
}

void render_Sound (struct X3D_Sound *node) {
/* node fields...
	direction => [SFVec3f, [0, 0, 1]],
	intensity => [SFFloat, 1.0],
	location => [SFVec3f, [0,0,0]],
	maxBack => [SFFloat, 10],
	maxFront => [SFFloat, 10],
	minBack => [SFFloat, 1],
	minFront => [SFFloat, 1],
	priority => [SFFloat, 0],
	source => [SFNode, NULL],
	spatialize => [SFBool,1, ""]	# not exposedfield
*/

	GLdouble mod[16];
	GLdouble proj[16];
	struct pt vec, direction, location;
	double len;
	double angle;
	float midmin, midmax;
	float amp;

	struct X3D_AudioClip *acp;
	struct X3D_MovieTexture *mcp;
	char mystring[256];

	acp = (struct X3D_AudioClip *) node->source;
	mcp = (struct X3D_MovieTexture *) node->source;

	/*  MovieTextures NOT handled yet*/
	/*  first - is there a node (any node!) attached here?*/
	if (acp) {
		/*  do the sound registering first, and tell us if this is an audioclip*/
		/*  or movietexture.*/

		render_node(acp);

		/*  if the attached node is not active, just return*/
		/* printf ("in Sound, checking AudioClip isactive %d\n", acp->isActive); */
		if (acp->isActive == 0) return;

		direction.x = node->direction.c[0];
		direction.y = node->direction.c[1];
		direction.z = node->direction.c[2];

		location.x = node->location.c[0];
		location.y = node->location.c[1];
		location.z = node->location.c[2];

		midmin = (node->minFront - node->minBack) / 2.0;
		midmax = (node->maxFront - node->maxBack) / 2.0;


		glPushMatrix();

		/*
		first, find whether or not we are within the maximum circle.

		translate to the location, and move the centre point, depending
		on whether we have a direction and differential maxFront and MaxBack
		directions.
		*/

		glTranslatef (location.x + midmax*direction.x,
				location.y + midmax*direction.y,
				location.z + midmax * direction.z);

		/* make the ellipse a circle by scaling...
		glScalef (direction.x*2.0 + 0.5, direction.y*2.0 + 0.5, direction.z*2.0 + 0.5);
		- scaling needs work - we need direction information, and parameter work. */

		if ((fabs(node->minFront - node->minBack) > 0.5) ||
			(fabs(node->maxFront - node->maxBack) > 0.5)) {
			if (!soundWarned) {
				printf ("FreeWRL:Sound: Warning - minBack and maxBack ignored in this version\n");
				soundWarned = TRUE;
			}
		}



		fwGetDoublev(GL_MODELVIEW_MATRIX, mod);
		fwGetDoublev(GL_PROJECTION_MATRIX, proj);
		gluUnProject (viewport[2]/2,viewport[3]/2,0.0,
			mod,proj,viewport, &vec.x,&vec.y,&vec.z);
		/* printf ("mod %lf %lf %lf proj %lf %lf %lf\n",*/
		/* mod[12],mod[13],mod[14],proj[12],proj[13],proj[14]);*/

		len = sqrt(VECSQ(vec));
		/* printf ("len %f\n",len); */
		/* printf("Sound: len %f mB %f mF %f angles (%f %f %f)\n",len,*/
		/* 	-node->maxBack, node->maxFront,vec.x,vec.y,vec.z);*/


		/*  pan left/right. full left = 0; full right = 1.*/
		if (len < 0.001) angle = 0;
		else {
			if (APPROX (mod[12],0)) {
				/* printf ("mod12 approaches zero\n");*/
				mod[12] = 0.001;
			}
			angle = fabs(atan2(mod[14],mod[12])) - (PI/2.0);
			angle = angle/(PI/2.0);

			/*  Now, scale this angle to make it between -0.5*/
			/*  and +0.5; if we divide it by 2.0, we will get*/
			/*  this range, but if we divide it by less, then*/
			/*  the sound goes "hard over" to left or right for*/
			/*  a bit.*/
			angle = angle / 1.5;

			/*  now scale to 0 to 1*/
			angle = angle + 0.5;

			/*  bounds check...*/
			if (angle > 1.0) angle = 1.0;
			if (angle < 0.0) angle = 0.0;
			/* printf ("angle: %f\n",angle); */
		}


		amp = 0.0;
		/* is this within the maxFront maxBack? */

		/* this code needs rework JAS */
		if (len < node->maxFront) {

			/* note: using vecs, length is always positive - need to work in direction
			vector */
			if (len < 0.0) {
				if (len < node->minBack) {amp = 1.0;}
				else {
					amp = (len - node->maxBack) / (node->maxBack - node->minBack);
				}
			} else {
				if (len < node->minFront) {amp = 1.0;}
				else {
					amp = (node->maxFront - len) / (node->maxFront - node->minFront);
				}
			}

			/* Now, fit in the intensity. Send along command, with
			source number, amplitude, balance, and the current Framerate */
			amp = amp*node->intensity;
			if (sound_from_audioclip) {
				sprintf (mystring,"AMPL %d %f %f",acp->__sourceNumber,amp,angle);
			} else {
				sprintf (mystring,"MMPL %d %f %f",mcp->__sourceNumber,amp,angle);
			}
			Sound_toserver(mystring);
		}
		glPopMatrix();
	}
}

void render_AudioClip (struct X3D_AudioClip *node) {
	/*  register an audioclip*/
	float pitch,stime, sttime;
	int loop;
	unsigned char *filename = (unsigned char *)node->__localFileName;

	/* tell Sound that this is an audioclip */
	sound_from_audioclip = TRUE;

	/* printf ("_change %d _ichange %d\n",node->_change, node->_ichange);  */

	if (!SoundEngineStarted) {
		printf ("AudioClip: initializing SoundEngine\n");
		SoundEngineStarted = TRUE;
		SoundEngineInit();
	}
#ifndef JOHNSOUND
	if (node->isActive == 0) return;  /*  not active, so just bow out*/
#endif

	if (!SoundSourceRegistered(node->__sourceNumber)) {

		/*  printf ("AudioClip: registering clip %d loop %d p %f s %f st %f url %s\n",
			node->__sourceNumber,  node->loop, node->pitch,node->startTime, node->stopTime,
			filename); */

		pitch = node->pitch;
		stime = node->startTime;
		sttime = node->stopTime;
		loop = node->loop;

		AC_LastDuration[node->__sourceNumber] =
			SoundSourceInit (node->__sourceNumber, node->loop,
			(double) pitch,(double) stime, (double) sttime, filename);
		/* printf ("globalDuration source %d %f\n",
				node->__sourceNumber,AC_LastDuration[node->__sourceNumber]);  */
	}
}
