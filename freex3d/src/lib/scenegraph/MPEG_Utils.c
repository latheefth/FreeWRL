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
		substituting freewrl's audio lib's API and openGL plumbing for SDL

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
	II. Support 
		a) Continuous Streaming video from url or 
		b) just finite local file
		SIMPLIFYING DECISION: b) finite local file


	Q1. which threads and structs should be working on MovieTexture?
	input media: MPEG1 contains Audio and/or Video
	Nodes: which we want to use to support SoundSource and/or Texture2D
	Texture2D: shows one frame at a time in opengl
	SoundSource: used as / like an AudioClip for Sound node. AudioClip has its own private thread
	If using a platform library to extract frames, it would likely have its own thread
	if using a berkley derivitive, we could add a thread
	It doesn't make sense to load a movietexture 2x if using for both texture and sound. 
	- you would DEF for one use, and USE for the other
	- then when you play, you play both USEs at the same time so audio and video are synced
	Sound is handled per-node.
	Textures have an intermediary texturetableindexstruct
		- why separate texturetable:
			H0: original arbitrary perl arrays design
			H1: easier garbage collection of opengl structs 
			H2: easier to maintain separate mipmapping thread away from Nodes rendering thread
			H3: .jpg/.png/.gif loaded only once even if requested multiple times in scenefile 
				- DISCONFIRMED: in freewrl Texture 1:1 TextureTableIndexStruct 1:1 resitem 1:1 url
	Q. if MovieTexture is loaded generically, then how does it sync up with TextureTableIndexStruct?
		we need :
			TextureTableEntry 1:1 Texture2D 0,1:1 MovieTexture
			SoundSource 0,1: MovieTexture
		so if MovieTexture is DEFd as SoundSource first, it still gets a textureTable so its ready for a USE as Texture2D
			option: give it a texturetableindexstruct even if there's nothing to fill in
			problem: workflow is currently request_texture -> tti -> request load -> report back to texture thread to finish tti
			problem: once a frame is extracted, all the normal texture2D stuff applies, should be like imageTexture
				- so it still needs mipmapping, opengl texture etc except once per rendered frame
	Q. what about a separate thread for interpolating frames at frame rate? Options:
		a) in rendering thread (any throttling problems?)
		b) new separate thread (but what about mipmapping and opengizing interpolated frame?)
		c) (somehow) use texture thread which is currently parked/unused once images are mipmapped 
			- but currently triggered during image file loading process
			- could set a flag to an earlier state and re-submit?
	Proposed freewrl plumbing:

	1. for texture rendering, MovieTexture works like ImageTexture on each render_hier frame, with a single opengl texture number
	2. a separate thread throttles frame interpolation, pauses, stops, rewinds, and plays. 
		As it plays or changes position, it interpolates a frame, and upadates the texture 
		as seen by the rendering thread: mipmaps and replaces opengl texture
	3. the pause, stop, play, rewind interface is usable as SoundSource like Audioclip, analogous to AudioClip
	4. perl > make AudioClip and MovieTexture fields in same order, so MovieTexture can be up-caste to AudioClip DONE

	top level interface:
	movie_load()
	parse_movie()
	do_MovieTextureTick()
	loadstatus_AudioClip(struct X3D_AudioClip *node) - loadsensor can check if file loaded
	locateAudioSource (struct X3D_AudioClip *node) - will work for MovieTexture
*/
#include <config.h>
#include <system.h>
#include <system_threads.h>
#include <display.h>
#include <internal.h>

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

#define MOVIETEXTURE_STUB 1
//Option A.
//	movie_load - load as BLOB using standard FILE2BLOB in io_files.c retval = resource_load(res);  //FILE2BLOB
//	parse_movie - converts BLOB to sound and video parts, returns parts
//Option B.
//  movie_load - parse movie as loading
//  parse_movie - return movie parts 


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
#elif MOVIETEXTURE_FFMPEG
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
	char *bbuffer;
	struct X3D_AudioClip *anode;
	struct X3D_MovieTexture *mnode;
	mnode = (struct X3D_MovieTexture *)node;
	anode = (struct X3D_AudioClip *)node;
	audio_sourcenumber = -1; //BADAUDIOSOURCE
	//parse audio and video
	//extract audio buffer if exists
	//MPEG1 level1,2 are compressed audio
	//decoders generally deliver so called PCM pulse code modulated buffers
	//and that's what audio drivers on computers normally take
	//and same with the APIs that wrap the hardware drivers ie openAL API
	//iret = parse_audioclip(anode,bbuffer, len);
#ifdef MOVIETEXTURE_STUB
	audio_sourcenumber = -1;
#elif MOVIETEXTURE_BERKLEYBROWN
#elif MOVIETEXTURE_FFMPEG
#endif
	return audio_sourcenumber;
}

bool  process_res_movie(resource_item_t *res){
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

