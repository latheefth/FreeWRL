/*


X3D Sound Component

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



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../vrml_parser/CRoutes.h"
#include "../main/headers.h"
#include "../opengl/OpenGL_Utils.h"

#include "LinearAlgebra.h"
#include "sounds.h"

#ifdef HAVE_OPENAL
//#include <AL/alhelpers.c>
/* InitAL opens the default device and sets up a context using default
 * attributes, making the program ready to call OpenAL functions. */
void* fwInitAL(void)
{
    ALCdevice *device;
    ALCcontext *ctx;

    /* Open and initialize a device with default settings */
    device = alcOpenDevice(NULL);
    if(!device)
    {
        fprintf(stderr, "Could not open a device!\n");
        return NULL;
    }

    ctx = alcCreateContext(device, NULL);
    if(ctx == NULL || alcMakeContextCurrent(ctx) == ALC_FALSE)
    {
        if(ctx != NULL)
            alcDestroyContext(ctx);
        alcCloseDevice(device);
        fprintf(stderr, "Could not set a context!\n");
        return NULL;
    }

    printf("Opened \"%s\"\n", alcGetString(device, ALC_DEVICE_SPECIFIER));
    return ctx;
}

/* CloseAL closes the device belonging to the current context, and destroys the
 * context. */
void fwCloseAL(void *alctx)
{
    ALCdevice *device;
    ALCcontext *ctx;

    //ctx = alcGetCurrentContext();
	ctx = alctx;
    if(ctx == NULL)
        return;

    device = alcGetContextsDevice(ctx);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
}

#endif


typedef struct pComponent_Sound{
	/* for printing warnings about Sound node problems - only print once per invocation */
	int soundWarned;// = FALSE;
	int SoundSourceNumber;
	void *alContext;
/* this is used to return the duration of an audioclip to the perl
   side of things. works, but need to figure out all
   references, etc. to bypass this fudge JAS */
	float AC_LastDuration[50];
}* ppComponent_Sound;
void *Component_Sound_constructor(){
	void *v = malloc(sizeof(struct pComponent_Sound));
	memset(v,0,sizeof(struct pComponent_Sound));
	return v;
}
void Component_Sound_init(struct tComponent_Sound *t){
	//public
	/* Sounds can come from AudioClip nodes, or from MovieTexture nodes. Different
   structures on these */
	t->sound_from_audioclip= 0;

	/* is the sound engine started yet? */
	t->SoundEngineStarted = FALSE;
	//private
	t->prv = Component_Sound_constructor();
	{
		ppComponent_Sound p = (ppComponent_Sound)t->prv;
		/* for printing warnings about Sound node problems - only print once per invocation */
		p->soundWarned = FALSE;
		p->SoundSourceNumber = 0;
		p->alContext = NULL;
		/* this is used to return the duration of an audioclip to the perl
		   side of things. works, but need to figure out all
		   references, etc. to bypass this fudge JAS */
		{
			int i;
			for(i=0;i<50;i++)
				p->AC_LastDuration[i]  = -1.0f;
		}
	}
}
//ppComponent_Sound p = (ppComponent_Sound)gglobal()->Component_Sound.prv;
void Sound_toserver(char *message)
{}

// Position of the listener.
float ListenerPos[] = { 0.0, 0.0, 0.0 };
// Velocity of the listener.
float ListenerVel[] = { 0.0, 0.0, 0.0 };
// Orientation of the listener. (first 3 elements are "at", second 3 are "up")
float ListenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };

int SoundEngineInit(void)
{
	void *alctx;
	int retval = FALSE;
	ppComponent_Sound p = (ppComponent_Sound)gglobal()->Component_Sound.prv;
#ifdef HAVE_OPENAL
	retval = TRUE;
	/* Initialize OpenAL with the default device, and check for EFX support. */
	alctx = fwInitAL();
	if(!alctx ){
		ConsoleMessage("initAL failed\n");
		retval = FALSE;
	}
	p->alContext = alctx;
#ifdef HAVE_ALUT
	if(!alutInitWithoutContext(NULL,NULL)) //this does not create an AL context (simple)
	{
		ALenum error = alutGetError ();
		ConsoleMessage("%s\n", alutGetErrorString (error));
		retval = FALSE;
	}
#endif //HAVE_ALUT

		//listener is avatar,
		//we could move both listener and sources in world coordinates
		//instead we'll work in avatar/local coordinates and
		//freeze listener at 0,0,0 and update the position of the sound sources 
		//relative to listener, on each frame
		alListenerfv(AL_POSITION,    ListenerPos);
		alListenerfv(AL_VELOCITY,    ListenerVel);
		alListenerfv(AL_ORIENTATION, ListenerOri);
		if(1){
			//ALenum error;
			if(FALSE) //meters)
				alSpeedOfSound(345.0f); //alDopplerVelocity(34.0f); //m/s
			else //feet
				alSpeedOfSound(1132.0f); //alDopplerVelocity(1132.0f); // using feet/second – change propagation velocity 
			alDopplerFactor(1.0f); // exaggerate pitch shift by 20% 
			//if ((error = alGetError()) != AL_NO_ERROR) DisplayALError("alDopplerX : ", error);
		}
		alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED); //here's what I think web3d wants
		//alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED); //seems a bit faint

#endif //HAVE_OPENAL
	gglobal()->Component_Sound.SoundEngineStarted = retval;
	return retval;
}

void waitformessage(void)
{}

void SoundEngineDestroy(void)
{}

int SoundSourceRegistered(int num)
{ 
	if(num > -1) return TRUE;
	return FALSE;
}

float SoundSourceInit(int num, int loop, double pitch, double start_time, double stop_time, char *url)
{return 0.0f;}

void SetAudioActive(int num, int stat)
{}


int haveSoundEngine(){
	ttglobal tg = gglobal();

	if (!tg->Component_Sound.SoundEngineStarted) {
		#ifdef SEVERBOSE
		printf ("SetAudioActive: initializing SoundEngine\n");
		#endif
		tg->Component_Sound.SoundEngineStarted = SoundEngineInit();
	}
	return tg->Component_Sound.SoundEngineStarted;
}

#ifdef OLDCODE
OLDCODEvoid render_AudioControl (struct X3D_AudioControl *node) {
OLDCODE	GLDOUBLE mod[16];
OLDCODE	GLDOUBLE proj[16];
OLDCODE	struct point_XYZ vec, direction, location;
OLDCODE	double len;
OLDCODE	double angle;
OLDCODE	float midmin, midmax;
OLDCODE
OLDCODE	/*  do the sound registering first, and tell us if this is an audioclip*/
OLDCODE	/*  or movietexture.*/
OLDCODE
OLDCODE
OLDCODE	/* if not enabled, do nothing */
OLDCODE	if (!node) return;
OLDCODE	if (node->__oldEnabled != node->enabled) {
OLDCODE		node->__oldEnabled = node->enabled;
OLDCODE		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_AudioControl, enabled));
OLDCODE	}
OLDCODE	if (!node->enabled) return;
OLDCODE
OLDCODE	direction.x = node->direction.c[0];
OLDCODE	direction.y = node->direction.c[1];
OLDCODE	direction.z = node->direction.c[2];
OLDCODE
OLDCODE	location.x = node->location.c[0];
OLDCODE	location.y = node->location.c[1];
OLDCODE	location.z = node->location.c[2];
OLDCODE
OLDCODE	midmin = (node->minFront - node->minBack) / (float) 2.0;
OLDCODE	midmax = (node->maxFront - node->maxBack) / (float) 2.0;
OLDCODE
OLDCODE
OLDCODE	FW_GL_PUSH_MATRIX();
OLDCODE
OLDCODE	/*
OLDCODE	first, find whether or not we are within the maximum circle.
OLDCODE
OLDCODE	translate to the location, and move the centre point, depending
OLDCODE	on whether we have a direction and differential maxFront and MaxBack
OLDCODE	directions.
OLDCODE	*/
OLDCODE
OLDCODE	FW_GL_TRANSLATE_D (location.x + midmax*direction.x,
OLDCODE			location.y + midmax*direction.y,
OLDCODE			location.z + midmax * direction.z);
OLDCODE
OLDCODE	/* make the ellipse a circle by scaling...
OLDCODE	FW_GL_SCALE_F (direction.x*2.0 + 0.5, direction.y*2.0 + 0.5, direction.z*2.0 + 0.5);
OLDCODE	- scaling needs work - we need direction information, and parameter work. */
OLDCODE
OLDCODE	if ((fabs(node->minFront - node->minBack) > 0.5) ||
OLDCODE		(fabs(node->maxFront - node->maxBack) > 0.5)) {
OLDCODE		if (!soundWarned) {
OLDCODE			printf ("FreeWRL:Sound: Warning - minBack and maxBack ignored in this version\n");
OLDCODE			soundWarned = TRUE;
OLDCODE		}
OLDCODE	}
OLDCODE
OLDCODE
OLDCODE
OLDCODE	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, mod);
OLDCODE	FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, proj);
OLDCODE	FW_GLU_UNPROJECT(viewport[2]/2,viewport[3]/2,0.0,
OLDCODE		mod,proj,viewport, &vec.x,&vec.y,&vec.z);
OLDCODE	/* printf ("mod %lf %lf %lf proj %lf %lf %lf\n",*/
OLDCODE	/* mod[12],mod[13],mod[14],proj[12],proj[13],proj[14]);*/
OLDCODE
OLDCODE	len = sqrt(VECSQ(vec));
OLDCODE	/* printf ("len %f\n",len);  */
OLDCODE	/* printf("Sound: len %f mB %f mF %f angles (%f %f %f)\n",len,*/
OLDCODE	/* 	-node->maxBack, node->maxFront,vec.x,vec.y,vec.z);*/
OLDCODE
OLDCODE
OLDCODE	/*  pan left/right. full left = 0; full right = 1.*/
OLDCODE	if (len < 0.001) angle = 0;
OLDCODE	else {
OLDCODE		if (APPROX (mod[12],0)) {
OLDCODE			/* printf ("mod12 approaches zero\n");*/
OLDCODE			mod[12] = 0.001;
OLDCODE		}
OLDCODE		angle = fabs(atan2(mod[14],mod[12])) - (PI/2.0);
OLDCODE		angle = angle/(PI/2.0);
OLDCODE
OLDCODE		/*  Now, scale this angle to make it between -0.5*/
OLDCODE		/*  and +0.5; if we divide it by 2.0, we will get*/
OLDCODE		/*  this range, but if we divide it by less, then*/
OLDCODE		/*  the sound goes "hard over" to left or right for*/
OLDCODE		/*  a bit.*/
OLDCODE		angle = angle / 1.5;
OLDCODE
OLDCODE		/*  now scale to 0 to 1*/
OLDCODE		angle = angle + 0.5;
OLDCODE
OLDCODE		/* and, "reverse" the value, so that left is left, and right is right */
OLDCODE		angle = 1.0 - angle;
OLDCODE
OLDCODE		/*  bounds check...*/
OLDCODE		if (angle > 1.0) angle = 1.0;
OLDCODE		if (angle < 0.0) angle = 0.0;
OLDCODE
OLDCODE		#ifdef SOUNDVERBOSE
OLDCODE		printf ("angle: %f\n",angle); 
OLDCODE		#endif
OLDCODE	}
OLDCODE
OLDCODE	/* convert to a MIDI control value */
OLDCODE	node->panFloatVal = (float) angle;
OLDCODE	node->panInt32Val = (int) (angle * 128);
OLDCODE	if (node->panInt32Val < 0) node->panInt32Val = 0; if (node->panInt32Val > 127) node->panInt32Val = 127;
OLDCODE
OLDCODE
OLDCODE	node->volumeFloatVal = (float) 0.0;
OLDCODE	/* is this within the maxFront maxBack? */
OLDCODE
OLDCODE	/* this code needs rework JAS */
OLDCODE	if (len < node->maxFront) {
OLDCODE		/* did this node become active? */
OLDCODE		if (!node->isActive) {
OLDCODE			node->isActive = TRUE;
OLDCODE			MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_AudioControl, isActive));
OLDCODE			#ifdef SOUNDVERBOSE
OLDCODE			printf ("AudioControl node is now ACTIVE\n");
OLDCODE			#endif
OLDCODE
OLDCODE
OLDCODE			/* record the length for doppler shift comparisons */
OLDCODE			node->__oldLen = len;
OLDCODE		}
OLDCODE
OLDCODE		/* note: using vecs, length is always positive - need to work in direction
OLDCODE		vector */
OLDCODE		if (len < 0.0) {
OLDCODE			if (len < node->minBack) {node->volumeFloatVal = (float) 1.0;}
OLDCODE			else { node->volumeFloatVal = ((float) len - node->maxBack) / (node->maxBack - node->minBack); }
OLDCODE		} else {
OLDCODE			if (len < node->minFront) {node->volumeFloatVal = (float) 1.0;}
OLDCODE			else { node->volumeFloatVal = (node->maxFront - (float) len) / (node->maxFront - node->minFront); }
OLDCODE		}
OLDCODE
OLDCODE		/* work out the delta for len */
OLDCODE		if (APPROX(node->maxDelta, 0.0)) {
OLDCODE			printf ("AudioControl: maxDelta approaches zero!\n");
OLDCODE			node->deltaFloatVal = (float) 0.0;
OLDCODE		} else {
OLDCODE			#ifdef SOUNDVERBOSE
OLDCODE			printf ("maxM/S %f \n",(node->__oldLen - len)/ (TickTime()- lastTime));
OLDCODE			#endif
OLDCODE
OLDCODE			/* calculate change as Metres/second */
OLDCODE
OLDCODE			/* compute node->deltaFloatVal, and clamp to range of -1.0 to 1.0 */
OLDCODE			node->deltaFloatVal = (float) ((node->__oldLen - len)/(TickTime()-lastTime()))/node->maxDelta;
OLDCODE			if (node->deltaFloatVal < (float) -1.0) node->deltaFloatVal = (float) -1.0; if (node->deltaFloatVal > (float) 1.0) node->deltaFloatVal = (float) 1.0;
OLDCODE			node->__oldLen = len;
OLDCODE		}
OLDCODE
OLDCODE		/* Now, fit in the intensity. Send along command, with
OLDCODE		source number, amplitude, balance, and the current Framerate */
OLDCODE		node->volumeFloatVal = node->volumeFloatVal*node->intensity;
OLDCODE		node->volumeInt32Val = (int) (node->volumeFloatVal * 128.0); 
OLDCODE		if (node->volumeInt32Val < 0) node->volumeInt32Val = 0; if (node->volumeInt32Val > 127) node->volumeInt32Val = 127;
OLDCODE
OLDCODE		node->deltaInt32Val = (int) (node->deltaFloatVal * 64.0) + 64; 
OLDCODE		if (node->deltaInt32Val < 0) node->deltaInt32Val = 0; if (node->deltaInt32Val > 127) node->deltaInt32Val = 127;
OLDCODE
OLDCODE		#ifdef SOUNDVERBOSE
OLDCODE		printf ("AudioControl: amp: %f (%d)  angle: %f (%d)  delta: %f (%d)\n",node->volumeFloatVal,node->volumeInt32Val,
OLDCODE			node->panFloatVal, node->panInt32Val ,node->deltaFloatVal,node->deltaInt32Val);
OLDCODE		#endif
OLDCODE
OLDCODE		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_AudioControl, volumeInt32Val));
OLDCODE		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_AudioControl, volumeFloatVal));
OLDCODE		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_AudioControl, panInt32Val));
OLDCODE		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_AudioControl, panFloatVal));
OLDCODE		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_AudioControl, deltaInt32Val));
OLDCODE		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_AudioControl, deltaFloatVal));
OLDCODE
OLDCODE	} else {
OLDCODE		/* node just became inActive */
OLDCODE		if (node->isActive) {
OLDCODE			node->isActive = FALSE;
OLDCODE			MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_AudioControl, isActive));
OLDCODE			#ifdef SOUNDVERBOSE
OLDCODE			printf ("AudioControl node is now INACTIVE\n");
OLDCODE			#endif
OLDCODE		}
OLDCODE	}
OLDCODE
OLDCODE	FW_GL_POP_MATRIX();
OLDCODE}
#endif // OLDCODE

#define LOAD_INITIAL_STATE 0
#define LOAD_REQUEST_RESOURCE 1
#define LOAD_FETCHING_RESOURCE 2
#define LOAD_PARSING 3
#define LOAD_STABLE 10

void locateAudioSource (struct X3D_AudioClip *node) {
	resource_item_t *res;
	resource_item_t *parentPath;
	ppComponent_Sound p = (ppComponent_Sound)gglobal()->Component_Sound.prv;

	switch (node->__loadstatus) {
		case LOAD_INITIAL_STATE: /* nothing happened yet */

		if (node->url.n == 0) {
			node->__loadstatus = LOAD_STABLE; /* a "do-nothing" approach */
			break;
		} else {
			res = resource_create_multi(&(node->url));
			res->media_type = resm_audio;
			node->__loadstatus = LOAD_REQUEST_RESOURCE;
			node->__loadResource = res;
		}
		//printf("1");
		break;

		case LOAD_REQUEST_RESOURCE:
		res = node->__loadResource;
		resource_identify(node->_parentResource, res);
		res->actions = resa_download | resa_load; //not resa_parse which we do below
		res->whereToPlaceData = X3D_NODE(node);
		//res->offsetFromWhereToPlaceData = offsetof (struct X3D_AudioClip, __FILEBLOB);
		resitem_enqueue(ml_new(res));
		node->__loadstatus = LOAD_FETCHING_RESOURCE;
		//printf("2");
		break;

		case LOAD_FETCHING_RESOURCE:
		res = node->__loadResource;
		/* printf ("load_Inline, we have type  %s  status %s\n",
			resourceTypeToString(res->type), resourceStatusToString(res->status)); */
		if(res->complete){
			if (res->status == ress_loaded) {
				res->actions = resa_process;
				res->complete = FALSE;
				resitem_enqueue(ml_new(res));
			} else if ((res->status == ress_failed) || (res->status == ress_invalid)) {
				//no hope left
				printf ("resource failed to load\n");
				node->__loadstatus = LOAD_STABLE; // a "do-nothing" approach 
				node->__sourceNumber = BADAUDIOSOURCE;
			} else	if (res->status == ress_parsed) {
				node->__loadstatus = LOAD_STABLE; 
			} //if (res->status == ress_parsed)
		} //if(res->complete)
		//end case LOAD_FETCHING_RESOURCE
		//printf("3");
		break;

		case LOAD_STABLE:
		//printf("4");
		break;
	}
}

void render_AudioClip (struct X3D_AudioClip *node) {
/*  audio clip is a flat sound -no 3D- and a sound node (3D) refers to it
	specs: if an audioclip can't be reached in the scenegraph, then it doesn't play
*/

	/* is this audio wavelet initialized yet? */
	if (node->__loadstatus != LOAD_STABLE) {
		locateAudioSource (node);
	}
	if(node->__loadstatus != LOAD_STABLE) return;
	/* is this audio ok? if so, the sourceNumber will range
	 * between 0 and infinity; if it is BADAUDIOSOURCE, bad source.
	 * check out locateAudioSource to find out reasons */
	if (node->__sourceNumber == BADAUDIOSOURCE) return;

#ifdef HAVE_OPENAL

#elif HAVE_OLDSOUND //MUST_RE_IMPLEMENT_SOUND_WITH_OPENAL
	/*  register an audioclip*/
	float pitch,stime, sttime;
	int loop;
	int sound_from_audioclip;
	unsigned char *filename = (unsigned char *)node->__localFileName;
	ppComponent_Sound p = (ppComponent_Sound)gglobal()->Component_Sound.prv;

	/* tell Sound that this is an audioclip */
	sound_from_audioclip = TRUE;

	/* printf ("_change %d _ichange %d\n",node->_change, node->_ichange);  */

	if(!haveSoundEngine()) return;

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

		p->AC_LastDuration[node->__sourceNumber] =
			SoundSourceInit (node->__sourceNumber, node->loop,
			(double) pitch,(double) stime, (double) sttime, filename);
		/* printf ("globalDuration source %d %f\n",
				node->__sourceNumber,AC_LastDuration[node->__sourceNumber]);  */
	}
#endif /* MUST_RE_IMPLEMENT_SOUND_WITH_OPENAL */
}



void render_Sound (struct X3D_Sound *node) {
/*  updates the position and velocity vector of the sound source relative to the listener/avatar
	so 3D sound effects can be rendered: distance attenuation, stereo left/right volume balance, 
	and doppler (pitch) effect
	- refers to sound source ie audioclip or movie
	- an audioclip may be DEFed and USEd in multiple Sounds, but will be playing the same tune at the same time
*/
	int sound_from_audioclip;

	struct X3D_AudioClip *acp = NULL;
	struct X3D_MovieTexture *mcp = NULL;
	struct X3D_Node *tmpN = NULL;
	ppComponent_Sound p = (ppComponent_Sound)gglobal()->Component_Sound.prv;

	/* why bother doing this if there is no source? */
	if (node->source == NULL) return;

	/* ok, is the source a valid node?? */

	/* might be a PROTO expansion, as in what Adam Nash does... */
	POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->source,tmpN)

	/* did not find a valid source node, even after really looking at a PROTO def */
	if (tmpN == NULL) return;

	sound_from_audioclip = FALSE;
	if (tmpN->_nodeType == NODE_AudioClip) {
		acp = (struct X3D_AudioClip *) tmpN;
		sound_from_audioclip = TRUE;
	}else if (tmpN->_nodeType == NODE_MovieTexture){
		mcp = (struct X3D_MovieTexture *) tmpN;
	} else {
		ConsoleMessage ("Sound node- source type of %s invalid",stringNodeType(tmpN->_nodeType));
		node->source = NULL; /* stop messages from scrolling forever */
		return;
	}

#ifdef HAVE_OPENAL
	/*  4 sources of openAL explanations and examples:
		- http://open-activewrl.sourceforge.net/data/OpenAL_PGuide.pdf  
		- http://forum.devmaster.net/t  and type 'openal' in the search box to get several lessons on openal
		- http://kcat.strangesoft.net/openal.html  example code (win32 desktop is using this openal-soft implementation of openal)
		- http://en.wikipedia.org/wiki/OpenAL links
		- <al.h> comments
	*/
	if(acp){
		if(haveSoundEngine()){
			if( acp->__sourceNumber < 0){
				render_AudioClip(acp);
			}
			if( acp->__sourceNumber > -1 ){
				//have a buffer loaded
				int i;
				GLDOUBLE modelMatrix[16];
				GLDOUBLE SourcePosd[3] = { 0.0f, 0.0f, 0.0f };
				ALfloat SourcePos[3];

				//transform source local coordinate 0,0,0 location into avatar/listener space
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);
				transformAFFINEd(SourcePosd,SourcePosd,modelMatrix);
				for(i=0;i<3;i++) SourcePos[i] = (ALfloat)SourcePosd[i];

				if( node->__sourceNumber < 0){
					//convert buffer to openAL sound source
					ALint source;
					source = 0;
					alGenSources(1, &source);
					alSourcei(source, AL_BUFFER, acp->__sourceNumber);
					alSourcef (source, AL_PITCH,    acp->pitch);
					alSourcef (source, AL_GAIN,     node->intensity );
					alSourcei (source, AL_LOOPING,  acp->loop);
					alSourcei (source, AL_SOURCE_RELATIVE, AL_TRUE);  //we'll treat the avatar/listener as fixed, and the sources moving relative
					//openAL will automatically mix multiple sources for one listener, there's no need for .priority hint
					alSourcef (source, AL_MAX_DISTANCE, node->maxFront);
					//no attempt is made to implement minBack, maxBack ellipsoidal as in web3d specs
					//- just a spherical sound, and with spatialize attempt at a cone
					node->__lasttime = TickTime();
					veccopy3f(node->__lastlocation.c,SourcePos);

					node->__sourceNumber = source;
					assert(alGetError()==AL_NO_ERROR && "Failed to setup sound source");
				}
				if( node->__sourceNumber > -1){
					int istate;
					ALfloat SourceVel[3] = { 0.0f, 0.0f, 0.0f };
					float travelled[3];
					double traveltime;

					//update position
					alSourcefv(node->__sourceNumber, AL_POSITION, SourcePos);

					//update velocity for doppler effect
					vecdif3f(travelled,node->__lastlocation.c,SourcePos);
					traveltime = TickTime() - node->__lasttime;
					if(traveltime > 0.0)
						vecscale3f(SourceVel,travelled,1.0/traveltime);
					alSourcefv(node->__sourceNumber, AL_VELOCITY, SourceVel);

					node->__lasttime = TickTime();
					veccopy3f(node->__lastlocation.c,SourcePos);

					//directional sound - I don't hear directional effects with openAL-Soft
					//AL_CONE_OUTER_GAIN f the gain when outside the oriented cone 
					//AL_CONE_INNER_ANGLE f, i the gain when inside the oriented cone 
					//AL_CONE_OUTER_ANGLE f, i outer angle of the sound cone, in degrees default is 360 
					if(node->spatialize){
						double dird[3];
						ALfloat dirf[3];
						//transform source direction into avatar/listener space
						for(i=0;i<3;i++) dird[i] = node->direction.c[i];
						transformAFFINEd(dird,dird,modelMatrix);
						for(i=0;i<3;i++) dirf[i] = dird[i];
						if (1)
							alSourcefv(node->__sourceNumber, AL_DIRECTION, dirf);
						else
							alSource3f(node->__sourceNumber, AL_DIRECTION, dirf[0], dirf[1], dirf[2]);
						alSourcef(node->__sourceNumber, AL_CONE_OUTER_GAIN, .5f);
						alSourcef(node->__sourceNumber,AL_CONE_INNER_ANGLE,90.0f);
						alSourcef(node->__sourceNumber,AL_CONE_OUTER_ANGLE,135.0f);
					}

					// for routed values going to audioclip, update values
					alSourcef (node->__sourceNumber, AL_PITCH,    acp->pitch);
					alSourcef (node->__sourceNumber, AL_GAIN,     node->intensity );
					alSourcei (node->__sourceNumber, AL_LOOPING,  acp->loop);
					// update to audioclip start,stop,pause,resume is done in do_AudioTick()
					if(acp->isPaused) alSourcePause(node->__sourceNumber);
					//execute audioclip state
					alGetSourcei(node->__sourceNumber, AL_SOURCE_STATE,&istate);
					if(acp->isActive ){
						if(istate != AL_PLAYING && !acp->isPaused){
							alSourcePlay(node->__sourceNumber);
							printf(".play.");
						}
					}else{
						if(istate != AL_STOPPED)
							alSourceStop(node->__sourceNumber);
					}
				}
			}
		}
	}


#elif HAVE_OLDSOUND //MUST_RE_IMPLEMENT_SOUND_WITH_OPENAL


	/* printf ("sound, node %d, acp %d source %d\n",node, acp, acp->__sourceNumber); */
	/*  MovieTextures NOT handled yet*/
	/*  first - is there a node (any node!) attached here?*/
	if (acp) {
		GLDOUBLE mod[16];
		GLDOUBLE proj[16];
		struct point_XYZ vec, direction, location;
		double len;
		double angle;
		float midmin, midmax;
		float amp;
		char mystring[256];

		/*  do the sound registering first, and tell us if this is an audioclip*/
		/*  or movietexture.*/

		render_node(X3D_NODE(acp));

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


		FW_GL_PUSH_MATRIX();

		/*
		first, find whether or not we are within the maximum circle.

		translate to the location, and move the centre point, depending
		on whether we have a direction and differential maxFront and MaxBack
		directions.
		*/

		FW_GL_TRANSLATE_F (location.x + midmax*direction.x,
				location.y + midmax*direction.y,
				location.z + midmax * direction.z);

		/* make the ellipse a circle by scaling...
		FW_GL_SCALE_F (direction.x*2.0 + 0.5, direction.y*2.0 + 0.5, direction.z*2.0 + 0.5);
		- scaling needs work - we need direction information, and parameter work. */

		if ((fabs(node->minFront - node->minBack) > 0.5) ||
			(fabs(node->maxFront - node->maxBack) > 0.5)) {
			if (!p->soundWarned) {
				printf ("FreeWRL:Sound: Warning - minBack and maxBack ignored in this version\n");
				p->soundWarned = TRUE;
			}
		}



		FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, mod);
		FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, proj);
		FW_GLU_UNPROJECT(viewport[2]/2,viewport[3]/2,0.0,
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

		/* printf ("sound %d len %f maxFront %f\n",acp->__sourceNumber, len, node->maxFront); */
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
				sprintf (mystring,"MMPL %d %f %f",mcp->__textureTableIndex, amp, angle); //__sourceNumber,amp,angle);
			}
			Sound_toserver(mystring);
		}
		FW_GL_POP_MATRIX();
	}
#endif /* MUST_RE_IMPLEMENT_SOUND_WITH_OPENAL */
}


int	parse_audioclip(struct X3D_AudioClip *node,char *bbuffer, int len){
#ifdef HAVE_OPENAL
	ALint buffer = AL_NONE;
#ifdef HAVE_ALUT
	buffer = alutCreateBufferFromFileImage (bbuffer, len);
//#elif HAVE_SDL
#endif
	if (buffer == AL_NONE)
		buffer = BADAUDIOSOURCE;
#else
	int buffer = BADAUDIOSOURCE;
#endif
	printf("parse_audioclip buffer=%d\n",buffer);
	return buffer;
}

double compute_duration(int ibuffer){

	double retval = 1.0;
#ifdef HAVE_OPENAL
	int ibytes;
	int ibits;
	int ichannels;
	int ifreq;
	double framesizebytes, bytespersecond;
	alGetBufferi(ibuffer,AL_FREQUENCY,&ifreq);
	alGetBufferi(ibuffer,AL_BITS,&ibits);
	alGetBufferi(ibuffer,AL_CHANNELS,&ichannels);
	alGetBufferi(ibuffer,AL_SIZE,&ibytes);
	framesizebytes = (double)(ibits * ichannels)/8.0;
	bytespersecond = framesizebytes * (double)ifreq;
	if(bytespersecond > 0.0)
		retval = (double)(ibytes) / bytespersecond;
	else
		retval = 1.0;
#elif HAVE_OLDSOUND
	//not sure how this is supposed to work, havent compiled it, good luck
	float pitch;
	double stime, sttime;
	int loop;
	pitch = node->pitch;
	stime = node->startTime;
	sttime = node->stopTime;
	loop = node->loop;

	retval = SoundSourceInit (ibuffer, node->loop,
		(double) pitch,(double) stime, (double) sttime, filename);
	
#endif
	return retval;
}
bool  process_res_audio(resource_item_t *res){
	s_list_t *l;
	openned_file_t *of;
	struct Shader_Script* ss;
	const char *buffer;
	int len;
	struct X3D_AudioClip *node;

	buffer = NULL;

	switch (res->type) {
	case rest_invalid:
		return FALSE;
		break;

	case rest_string:
		buffer = res->URLrequest;
		break;
	case rest_url:
	case rest_file:
	case rest_multi:
		//l = (s_list_t *) res->openned_files;
		//if (!l) {
		//	/* error */
		//	return FALSE;
		//}

		//of = ml_elem(l);
		of = res->openned_files;
		if (!of) {
			/* error */
			return FALSE;
		}

		buffer = of->fileData;
		len = of->fileDataSize;
		break;
	}

	node = (struct X3D_AudioClip *) res->whereToPlaceData;
	//node->__FILEBLOB = buffer;
	node->__sourceNumber = parse_audioclip(node,buffer,len); //__sourceNumber will be openAL buffer number
	if(node->__sourceNumber > -1) {
		node->duration_changed = compute_duration(node->__sourceNumber);
		MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_AudioClip, duration_changed));
		return TRUE;
	} 
	return FALSE;
}


/* returns the audio duration, unscaled by pitch */
double return_Duration (struct X3D_AudioClip *node) {
	double retval;
	int indx;
	indx = node->__sourceNumber;
	if (indx < 0)  retval = 1.0;
	else if (indx > 50) retval = 1.0;
	else 
	{
#ifdef HAVE_OPENAL
		retval = node->duration_changed;
#else
		ppComponent_Sound p = (ppComponent_Sound)gglobal()->Component_Sound.prv;
		retval = p->AC_LastDuration[indx];
#endif
	}
	return retval;
}
