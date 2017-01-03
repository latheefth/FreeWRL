/*


???

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


/* NOTE: we have to re-implement the loading of movie textures; the code in here was a decade old and did not
keep up with "the times". Check for ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES in the code */

/* July 2016 note:
	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/texturing.html#MovieTexture
	- specs say to support MPEG1
	- online says all patents have expired for MPEG1 and its Level II audio
	http://www.web3d.org/x3d/content/examples/ConformanceNist/
	- see Movie texture example with VTS.mpg

	A. mpeg1 capable library plumbing
	There are still patents and licensing issues with recent mp4 and even mpeg-2 I've heard.
	MPEG1 - any patents have expired
	Goal: a generic interface we can use to wrap 3 implementations:
		1. stub
		2. option: old-fashioned cross-platform mpeg1 c code, as fallback/default
		3. platform-supplied/platform-specific video API/libraries 
	and to use both audio and video streams, with audio compatible with SoundSource node api needs.

	Links > 
	2. old fashioned mpeg1
	Mpeg-1 libs / code
	2.1 reference implementation
		https://en.wikipedia.org/wiki/MPEG-1
		See source code link at bottom, reference implementation
		which has an ISO license which dug9 reads to mean: 
		- use for simulation of what electronic devices will do, presumably by electronics companies
		- (but not intendended / foreseen as a software library product, uncertain if allowed)
		it includes audio and video
	2.2 Berkley mpeg_play derivitives
		a) berkley_brown
			original freewrl implementation (taken out in 2009), has both berkley and brown U listed in license section
		b) berkley_gerg
			http://www.gerg.ca/software/mpeglib/
			Mpeg1 - hack of berkley mpeg1 code, no separate license on hacks show
		c) berkley_nabg
			http://www.uow.edu.au/~nabg/MPEG/mpeg1.html
			Mpeg1 explanation with hacked Berkley code and license: berkley + do-what-you-want-with-hacked-code 
	2.3 ffmpeg.org
		LGPL by default (can add in GPL parts, we don't need)
		but may include patented algorithms for post-MPEG1-Audio_Level_II
		but has a way to limit codecs available at runtime: 
			instead of load_all() you would somehow say which ones to load?? see dranger tutorial about tut5
		and if so and you limit codecs to MPEG1 with Level II audio, then
		ffplay.c and tutorial http://dranger.com/ffmpeg/ can help, 
			substituting freewrl's audio lib's API, pthreads, and our openGL plumbing for SDL
			might be able to #define some pthread for SDL thread functions in ffplay.c
			- except don't assume freewrl's pthreads is complete: no cancel, its an emulated lib on some platforms
		https://github.com/FFmpeg/FFmpeg
		https://github.com/FFmpeg/FFmpeg/blob/master/ffplay.c
			line interesting function
			3352 event_loop() - play, pause, rewind user functions
			3000 infinite_buffer real_time -in theory could pull from url instead of file
			2660 forcing codec by name - if blocking patented MPEG2+ / sticking to MPEG1+LevelII audio
			2400 audio_decode_frame - somwhere we need to get the audio PCM decompressed buffer, so we can pass to our audio API
			1808 get_video_frame - somewhere we need to get the current frame, perhaps from do_MovieTextureTick() we would get closest-to-current frame and freeze it for opengl
			1506 video_refresh called to display each frame


	3. Platform supplied
	3.1 windows > directX audio and video (I have the dx audio working in one app)
	3.2 android > 
	3.3 linux desktop > 

	B. freewrl texture and sound plumbing
	MAJOR DESIGN OPTIONS:
	I. Process video frame into mipmapped opengl texture:
	a) on loading:, pre-process entire video into mipmapped opengl textures, one per video frame (2009 implementation) 
		Disadvantage: 30fps x 10 seconds is 300 textures - a lot to store, and prepare if unneeded
		x a lot of opengl textures needed - exhaustion/limits?
		Benefit: faster per frame when playing
	b) on-the-fly during play: in a separate thread, process video frames to replace single opengl texture 
		Benefit: vs c: mpeg decompression: successive frames require previous frame decoded, so state is managed
		- vs a,c: thread can do its own throttling 
		- vs a: storage is just the decompression sequence
		Disadvantage: single core will be doing a lot of un-needed mipmapping
		x thread collisions - texture being replaced in one thread and drawn in another?
		x stereo vision > left and right eye might see different texture
	c) on-the-fly in do_tick (once per render frame), interpolate closest frame based on time, replace single opengl texture
		Benefit: vs a,b: no unnecessary frames interpolated, no unnecessary mipmapping in any thread, just the frame needed
		vs. b: same frame appears in left/right stereo, no timing weirdness
		vs. a: storage is just the decompression sequence
	d) combo of b) and c): separate thread prepares small set of raw frames, 
		do_MovieTextureTick() picks one or asks for one and opengl-izes it
	II. Support streaming?
		a) Continuous Streaming video from url or 
		b) just finite local file
		SIMPLIFYING DECISION: b) finite local file
	III. Separate thread for decoding / interpolating frames?
		a) or in rendering thread (any throttling problems?)
		b) new separate thread (but what about mipmapping and opengizing interpolated frame?)
		c) (somehow) use texture thread which is currently parked/unused once images are mipmapped 
			- but currently triggered during image file loading process
			- could set a flag to an earlier state and re-submit?
		SIMPLIFYING DECISION: depends on implementation ie what libs you get and how easy it is

	A few facts / details:
	input media: MPEG1 contains Audio and/or Video
	Nodes: which we want to use to support SoundSource and/or Texture2D
	Texture2D: shows one frame at a time in opengl
	SoundSource: used as / like an AudioClip for Sound node. AudioClip has its own private thread
	It doesn't make sense to load a movietexture 2x if using for both texture and sound. 
	- you would DEF for one use, and USE for the other
	- then when you play, you play both USEs at the same time so audio and video are synced
	Sound is handled per-node.
	Textures have an intermediary texturetableindexstruct
	
	2003 - 2009 freewrl movietexture:
		- on load: decoded and created an opengl texture for each movie frame
		- used berkley mpeg aka berkley-brown


	Proposed freewrl plumbing:

	1. for texture rendering, MovieTexture works like ImageTexture on each render_hier frame, 
		with a single opengl texture number
	2. leave it to the library-specifics to decide if
		a) decode-on-load
		b) decode in a separate thread, anticipatory/queue
		c) decode-on-demand
	3. the pause, stop, play, rewind interface is usable as SoundSource like Audioclip, analogous to AudioClip
	4. perl > make AudioClip and MovieTexture fields in same order, so MovieTexture can be up-caste to AudioClip DONE

	top level interface:
	X3DMovieTexture._movie; an opaque pointer that will hold a malloced struct representing 
		the movie container and streams; different implementations will be different
	movie_load() - like loadTextures.c image loaders - takes local path and gets things started
		puts intial load into ._movie of the requesting node ie res->(wheretoplacedata,offset) = (MovieTexture,_movie)
	do_MovieTextureTick() 
		in senseInterp.c, once per frame (so stereo uses same movie frame for left/right)
		could ask for closest frame based on movie time
		node->tti->texdata = getClosestMovieFrame(movietime)
			a) decode-on-load would have the frame ready in a list 
			b) multi-thread anticipatory decode would have a private queue/list of decoded frames,
				and get the closest one, and discard stale frames, and restart decode queue to fill up
				queue again
			c) decode-on-demand would decode on demand
		Texture2D(,,,node->tti->opeglTexture,,,node->tti->texdata) //reset texture data

	loadstatus_MovieTexture(struct X3D_MovieTexture *node) - loadsensor can check if file loaded
	loadstatus_AudioClip(struct X3D_AudioClip *node) - loadsensor can check if file loaded
	locateAudioSource (struct X3D_AudioClip *node) - will work for MovieTexture
	
	search code for MovieTexture, resm_movie to see all hits


	MPEG_Utils_berkley.c Nov 15, 2016: compiled but bombs on loading even simple vts.mpg
	MPEG_Utils_libmpeg2.c Nove 15, 2016:  not attempted to implement
		x undocumented code
		x GPL
		x uses callback for frames
		x no audio channel
		- but small test program mpeg2dec does run on windows
		- code seems lite / compact
*/
#include <config.h>
#include <system.h>
#include <system_threads.h>
#include <display.h>
#include <internal.h>
#include "vrml_parser/CRoutes.h"
#include "vrml_parser/Structs.h"
#include "main/ProdCon.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"
#include "../opengl/LoadTextures.h"
#include "../scenegraph/Component_CubeMapTexturing.h"

#include <list.h>
#include <io_files.h>
#include <io_http.h>

#include <threads.h>

#include <libFreeWRL.h>

//put your choice in your config.h (or windows preprocessor directives):
//#define MOVIETEXTURE_STUB 1   //default
//#define MOVIETEXTURE_BERKLEYBROWN 1
//#define MOVIETEXTURE_FFMPEG 1
//#define MOVIETEXTURE_LIBMPEG2 1

//Option A.
//	movie_load - load as BLOB using standard FILE2BLOB in io_files.c retval = resource_load(res);  //FILE2BLOB
//	parse_movie - converts BLOB to sound and video parts, returns parts
//Option B.
//  movie_load - parse movie as loading
//  parse_movie - return movie parts 

#ifdef MOVIETEXTURE_BERKLEYBROWN
#include "MPEG_Utils_berkley.c"
#elif MOVIETEXTURE_FFMPEG
//#include "MPEG_Utils_ffmpeg.c"
int movie_load_from_file(char *fname, void **opaque);
double movie_get_duration(void *opaque);
unsigned char *movie_get_frame_by_fraction(void *opaque, float fraction, int *width, int *height, int *nchan);
unsigned char * movie_get_audio_PCM_buffer(void *opaque,int *freq, int *channels, int *size, int *bits);
#include "sounds.h"
//BufferData * alutBufferDataConstruct (ALvoid *data, size_t length, ALint numChannels,
//                          ALint bitsPerSample, ALfloat sampleFrequency);

#define LOAD_STABLE 10 //from Component_Sound.c
#elif MOVIETEXTURE_LIBMPEG2
#endif

bool movie_load(resource_item_t *res){
	bool retval;
	// see io_files.c for call place
	//Option A: just load blob for later
	// retval = resource_load(res);  //FILE2BLOB
	//Option B:
	// parse during load
	// copied from imagery_load - but TEX_READ flag will be wrong for movie
	//int textureNumber;
	//struct textureTableIndexStruct *entry; // = res->whereToPlaceData;
	//textureNumber = res->textureNumber;
	//if(res->status == ress_downloaded){
	//	entry = getTableIndex(textureNumber);
	//	if(entry)
	//	if (movie_load_from_file(entry, res->actual_file)) {
	//		entry->status = TEX_READ; // tell the texture thread to convert data to OpenGL-format 
	//		res->status = ress_loaded;
	//		retval = TRUE;
	//		return retval;
	//	}
	//}
	//res->status = ress_not_loaded;
	retval = FALSE;

#ifdef MOVIETEXTURE_STUB
	res->status = ress_loaded;
	retval = TRUE;
#elif MOVIETEXTURE_BERKLEYBROWN
	 {
        int x,y,depth,frameCount;
        char *ptr;
        ptr=NULL;
		//H: this returns something like a volume image, with slices packed into ptr, and z=frameCount, nchannels = depth.
		//Q: what's the 'normal' frame rate? should that be returned too, or is there a standard/default?
		//Nov 15, 2016: bombs on small test file vts.mpg
        mpg_main(res->actual_file, &x,&y,&depth,&frameCount,&ptr);
		#ifdef TEXVERBOSE
		printf ("have x %d y %d depth %d frameCount %d ptr %d\n",x,y,depth,frameCount,ptr);
		#endif
		// store_tex_info(loadThisTexture, depth, x, y, ptr,depth==4); 

		// and, manually put the frameCount in. 
		//res->frames = frameCount;
	}

#elif MOVIETEXTURE_FFMPEG
	{
		void *opaque;
		int loaded;
		loaded = movie_load_from_file(res->actual_file,&opaque);
		retval = loaded > -1 ? TRUE : FALSE;
		if(loaded){
			int freq,channels,size,bits;
			unsigned char * pcmbuf;
			struct X3D_MovieTexture *node;

			res->status = ress_loaded;
			res->complete = TRUE;
			res->status = ress_parsed; //we'll skip the parse_movie/load_from_blob handler 

			node = (struct X3D_MovieTexture *) res->whereToPlaceData;
			//AUDIO AND/OR VIDEO CHANNELS?
			node->duration_changed = movie_get_duration(opaque);
			node->__fw_movie = opaque;
			node->__loadstatus = LOAD_STABLE;
			//VIDEO CHANNEL?
			//double totalframes = node->duration_changed * 30.0; 
			node->speed = 1.0; //1 means normal speed 30.0 / totalframes; //in fractions per second = speed in frames/second / totalframes
			MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_MovieTexture, duration_changed));
			//AUDIO CHANNEL?
			//node->__sourceNumber = parse_movie(node,buffer,len); //__sourceNumber will be openAL buffer number
			pcmbuf = movie_get_audio_PCM_buffer(opaque,&freq,&channels,&size,&bits);
			if(pcmbuf){
				//MPEG1 level1,2 are compressed audio
				//decoders generally deliver so called PCM pulse code modulated buffers
				//and that's what audio drivers on computers normally take
				//and same with the APIs that wrap the hardware drivers ie openAL API
				printf("audio freq %d channels %d size %d bits per channel %d\n",freq,channels,size,bits);
				#ifdef HAVE_OPENAL
				// http://open-activewrl.sourceforge.net/data/OpenAL_PGuide.pdf
				// page 6
				{
					int format;
					ALuint albuffer; 
					static int once = 0;
					if(!once){
						#ifdef HAVE_ALUT
						//alutInit(0, NULL); // Initialize OpenAL 
						if (!alutInitWithoutContext(NULL, NULL))
							ConsoleMessage("ALUT init failed\n");
						#endif //HAVE_ALUT
						alGetError(); // Clear Error Code
						//SoundEngineInit();
						once = 1;
					}

					alGenBuffers(1, &albuffer); 
					//al.h
					//#define AL_FORMAT_MONO8                          0x1100
					//#define AL_FORMAT_MONO16                         0x1101
					//#define AL_FORMAT_STEREO8                        0x1102
					//#define AL_FORMAT_STEREO16                       0x1103
					//if(bits == 8)
					//	format = AL_FORMAT_MONO8;
					//else
					//	format = AL_FORMAT_MONO16;
					//if(channels == 2) 
					//	if(bits == 8)
					//		format = AL_FORMAT_STEREO8;
					//	else
					//		format = AL_FORMAT_STEREO16;
					format = 0;
					switch(bits){
						case 8:
							format = AL_FORMAT_MONO8;
							if(channels == 2) 
								format = AL_FORMAT_STEREO8;
							break;
						case 16:
							format = AL_FORMAT_MONO16;
							if (channels == 2)
								format = AL_FORMAT_STEREO16;
							break;
						case 32:
							#ifdef AL_EXT_float32
							format = AL_FORMAT_MONO_FLOAT32;
							if (channels == 2) 
								format = AL_FORMAT_STEREO_FLOAT32;
							break;
							#endif
						default:
							break;
					}
					if(format > 0){
						//this is a complex function that tries to figure out if its float, int PCM etc
						alBufferData(albuffer,format,pcmbuf,size,freq); 
						//BufferData * bdata = _alutBufferDataConstruct( pcmbuf,size,channels,bits, freq);

						node->__sourceNumber = albuffer;
					}
				}
				#endif //HAVE_OPENAL
			}
		} 

		printf("opqaue = %p, loaded=%d \n",opaque,res->status);
	}
#elif MOVIETEXTURE_LIBMPEG2
#endif
	return retval;
}
int	parse_audioclip(struct X3D_AudioClip *node,char *bbuffer, int len);
int parse_movie(node,buffer,len){
	//Option B - parse blob
	//if your movie api will take a blob, you can call it from here to parse
	//convert BLOB (binary large object) into video and audio structures
	//Option A and B - return audio and video parts
	int audio_sourcenumber;
	audio_sourcenumber = -1; //BADAUDIOSOURCE
	//MPEG1 level1,2 are compressed audio
	//decoders generally deliver so called PCM pulse code modulated buffers
	//and that's what audio drivers on computers normally take
	//and same with the APIs that wrap the hardware drivers ie openAL API
#ifdef MOVIETEXTURE_STUB
#elif MOVIETEXTURE_BERKLEYBROWN
#elif MOVIETEXTURE_FFMPEG
#elif MOVIETEXTURE_LIBMPEG2
#endif
	return audio_sourcenumber;
}
double compute_duration(int ibuffer);

bool  process_res_movie(resource_item_t *res){
	// METHOD_LOAD_ON_DEMAND
	//you'll get in here if you didn't (completely) handle movie_load from file
	//
	//s_list_t *l;
	openned_file_t *of;
	const char *buffer;
	int len;
	struct X3D_MovieTexture *node;

	buffer = NULL;
	len = 0;
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
		of = res->openned_files;
		if (!of) {
			/* error */
			return FALSE;
		}

		buffer = of->fileData;
		len = of->fileDataSize;
		break;
	}

	node = (struct X3D_MovieTexture *) res->whereToPlaceData;
	//node->__FILEBLOB = buffer;
	node->__sourceNumber = parse_movie(node,buffer,len); //__sourceNumber will be openAL buffer number
	if(node->__sourceNumber > -1) {
		node->duration_changed = compute_duration(node->__sourceNumber);
		MARK_EVENT (X3D_NODE(node), offsetof(struct X3D_MovieTexture, duration_changed));
		return TRUE;
	} 
	return FALSE;
}


// - still needed ? don't know depends on implementation
//void getMovieTextureOpenGLFrames(int *highest, int *lowest,int myIndex) {
//        textureTableIndexStruct_s *ti;
//
///*        if (myIndex  == 0) {
//		printf ("getMovieTextureOpenGLFrames, myIndex is ZERL\n");
//		*highest=0; *lowest=0;
//	} else {
//*/
//	*highest=0; *lowest=0;
//	
//	#ifdef TEXVERBOSE
//	printf ("in getMovieTextureOpenGLFrames, calling getTableIndex\n");
//	#endif
//
//       	ti = getTableIndex(myIndex);
//
///* 	if (ti->frames>0) { */
//		if (ti->OpenGLTexture != TEXTURE_INVALID) {
//			*lowest = ti->OpenGLTexture;
//			*highest = 0;
///* 			*highest = ti->OpenGLTexture[(ti->frames) -1]; */
//		}
///* 	} */
//}

unsigned char *movietexture_get_frame_by_fraction(struct X3D_Node* node, float fraction, int *width, int *height, int *nchan){
	unsigned char* retval = NULL;
	if(node && node->_nodeType == NODE_MovieTexture){
		struct X3D_MovieTexture *movietexture = (struct X3D_MovieTexture *)node;
#ifdef MOVIETEXTURE_STUB
#elif MOVIETEXTURE_BERKLEYBROWN
#elif MOVIETEXTURE_FFMPEG
		retval = movie_get_frame_by_fraction(movietexture->__fw_movie,fraction,width,height,nchan);
#elif MOVIETEXTURE_LIBMPEG2
#endif
	}
	return retval;
}