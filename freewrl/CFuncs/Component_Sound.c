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
		/* printf ("in Sound, checking AudioClip isactive %d\n", acp->isActive);*/
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
			/* printf ("angle: %f\n",angle);*/
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
		/* printf ("AudioClip: initializing SoundEngine\n"); */
		SoundEngineStarted = TRUE;
		SoundEngineInit();
	}
#ifndef JOHNSOUND
	if (node->isActive == 0) return;  /*  not active, so just bow out*/
#endif

	if (!SoundSourceRegistered(node->__sourceNumber)) {

		 /* printf ("AudioClip: registering clip %d loop %d p %f s %f st %f url %s\n",
			node->__sourceNumber,  node->loop, node->pitch,node->startTime, node->stopTime,
			filename);
		*/



		pitch = node->pitch;
		stime = node->startTime;
		sttime = node->stopTime;
		loop = node->loop;

		AC_LastDuration[node->__sourceNumber] =
			SoundSourceInit (node->__sourceNumber, node->loop,
			(float) pitch,(float) stime, (float) sttime, (char *)filename);
		/* printf ("globalDuration source %d %f\n",
				node->__sourceNumber,AC_LastDuration[node->__sourceNumber]); */

		if (filename) free (filename);
	}
}
