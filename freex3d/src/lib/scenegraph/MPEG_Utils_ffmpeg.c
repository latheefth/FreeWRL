
#ifdef MOVIETEXTURE_FFMPEG
// http://dranger.com/ffmpeg/tutorial01.html

#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

#define inline   //someone in ffmpeg put a bit of cpp in their headers, this seemed to fix it
//#include "libavutil/avstring.h"
//#include "libavutil/colorspace.h"
//#include "libavutil/mathematics.h"
//#include "libavutil/pixdesc.h"
//#include "libavutil/imgutils.h"
//#include "libavutil/pixfmt.h"
//#include "libavutil/dict.h"
//#include "libavutil/parseutils.h"
//#include "libavutil/samplefmt.h"
//#include "libavutil/avassert.h"
//#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
//#include "libavutil/opt.h"
//#include "libavcodec/avfft.h"
//#include "libswresample/swresample.h"

#include "internal.h"
#include "Vector.h"
#include "../opengl/textures.h"
void saveImage_web3dit(struct textureTableIndexStruct *tti, char *fname);
// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif


//from ffmpeg tutorial01.c
//save to .ppm imge format for debugging, which gimp will read but only if RGB24 / nchan==3
void SaveFrame(AVFrame *pFrame, int width, int height, int nchan, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*nchan, pFile);
  
  // Close file
  fclose(pFile);
}

//our opaque pointer is a struct:
struct fw_movietexture {
	//AVFormatContext *pFormatCtx; //don't need to save for decode-on-load
	//AVCodecContext *pVideoCodecCtx; //don't need to save for decode-on-load
	//video and audio section:
	double duration;
	//video section:
	int width,height,nchan,nframes,fps;
	unsigned char **frames;
	//audio section:
	unsigned char *audio_buf;
	int audio_buf_size;
	int channels;
	int freq;
	int bits_per_channel;
};
int movie_load_from_file(char *fname, void **opaque){
	static int once = 0;
	struct fw_movietexture fw_movie;
	*opaque = NULL;

	//initialize ffmpeg libs once per process
	if(once == 0){
		av_register_all(); //register all codecs - will filter in the future for patent non-expiry
		once = 1;
	}
	AVFormatContext *pFormatCtx = NULL;

	// Open video file
	if(avformat_open_input(&pFormatCtx, fname, NULL, NULL)!=0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, fname, 0);
	//fw_movie.pFormatCtx = pFormatCtx;

	int i, videoStream, audioStream;
	AVCodecContext *pCodecCtxOrig = NULL;
	AVCodecContext *pCodecCtx = NULL;

	// Find the first video stream
	videoStream=-1;
	audioStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++){
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO && videoStream < 0) {
			videoStream=i;
		}
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO && audioStream < 0) {
			audioStream=i;
		}
	}
	if(videoStream==-1 && audioStream == -1)
		return -1; // Didn't find either video or audio stream



	//audio and video prep
	memset(&fw_movie,0,sizeof(struct fw_movietexture));
	fw_movie.frames = NULL;
	fw_movie.nframes = 0;
	fw_movie.audio_buf = NULL;
	fw_movie.audio_buf_size = 0;

	//audio function-scope variables
	AVCodecContext  *aCodecCtxOrig = NULL;
	AVCodecContext  *aCodecCtx = NULL;
	AVCodec         *aCodec = NULL;
	AVFrame			*aFrame = NULL;
	uint8_t *audio_pkt_data = NULL;
	int audio_pkt_size = 0;
	unsigned int audio_buf_size = 1000000;
	unsigned int audio_buf_index = 0;
	uint8_t * audio_buf = NULL;

	//audio prep
	if(audioStream > -1){
		aCodecCtxOrig=pFormatCtx->streams[audioStream]->codec;
		aCodec = avcodec_find_decoder(aCodecCtxOrig->codec_id);
		if(!aCodec) {
			fprintf(stderr, "Unsupported codec!\n");
			return -1;
		}

		// Copy context
		aCodecCtx = avcodec_alloc_context3(aCodec);
		if(avcodec_copy_context(aCodecCtx, aCodecCtxOrig) != 0) {
			fprintf(stderr, "Couldn't copy codec context");
			return -1; // Error copying codec context
		}

		//// Set audio settings from codec info
		//wanted_spec.freq = aCodecCtx->sample_rate;
		//wanted_spec.format = AUDIO_S16SYS;
		//wanted_spec.channels = aCodecCtx->channels;
		fw_movie.channels = aCodecCtx->channels;
		fw_movie.freq = aCodecCtx->sample_rate;
		aCodecCtx->bits_per_coded_sample = 16;
		fw_movie.bits_per_channel = 16; //aCodecCtx->bits_per_raw_sample; //or should it be per_coded_sample?
		//wanted_spec.silence = 0;
		//wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
		//wanted_spec.callback = audio_callback;
		//wanted_spec.userdata = aCodecCtx;
  //
		//if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
		//fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		//return -1;
		//}

		avcodec_open2(aCodecCtx, aCodec, NULL);

		//// audio_st = pFormatCtx->streams[index]
		//packet_queue_init(&audioq);
		//SDL_PauseAudio(0);

		audio_buf = malloc(audio_buf_size);
		aFrame=av_frame_alloc();

	}

	//video function-scope variables
	struct SwsContext *sws_ctx = NULL;
	int frameFinished;
	AVPacket packet;
	AVFrame *pFrame = NULL;
	AVCodec *pCodec = NULL;
	Stack *fw_framequeue = NULL;
	AVFrame *pFrameRGB = NULL;
	int nchan;
	uint8_t *buffer = NULL;
	//video prep
	if(videoStream > -1){
		// Get a pointer to the codec context for the video stream
		pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;


		// Find the decoder for the video stream
		pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
		if(pCodec==NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return -1; // Codec not found
		}
		// Copy context
		pCodecCtx = avcodec_alloc_context3(pCodec);
		if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
			fprintf(stderr, "Couldn't copy codec context");
			return -1; // Error copying codec context
		}
		// Open codec
		if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
			return -1; // Could not open codec
		//fw_movie.pVideoCodecCtx = pCodecCtx;


		// Allocate video frame
		pFrame=av_frame_alloc();

		// Allocate an AVFrame structure
		pFrameRGB=av_frame_alloc();
		if(pFrameRGB==NULL)
			return -1;

		int numBytes;
		// Determine required buffer size and allocate buffer
		int av_pix_fmt;
		if(0){
			nchan = 3;
			av_pix_fmt = AV_PIX_FMT_RGB24;
		}else{
			nchan = 4;
			av_pix_fmt = AV_PIX_FMT_RGBA;
		}

		fw_movie.nchan = nchan; //RGB24 == 3, RGBA == 4
		fw_movie.width = pCodecCtx->width;
		fw_movie.height = pCodecCtx->height;

		numBytes=avpicture_get_size(av_pix_fmt, pCodecCtx->width,  //AV_PIX_FMT_RGB24, AV_PIX_FMT_RGBA
									pCodecCtx->height);
		buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));


		// Assign appropriate parts of buffer to image planes in pFrameRGB
		// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
		// of AVPicture
		avpicture_fill((AVPicture *)pFrameRGB, buffer,av_pix_fmt, //AV_PIX_FMT_RGBA, //AV_PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);


		// initialize SWS context for software scaling
		sws_ctx = sws_getContext(pCodecCtx->width,
			pCodecCtx->height,
			pCodecCtx->pix_fmt,
			pCodecCtx->width,
			pCodecCtx->height,
			av_pix_fmt, //AV_PIX_FMT_RGBA, //AV_PIX_FMT_RGB24,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL
			);

		//if( METHOD_DECODE_ON_LOAD ) - decodes all frames in resource thread when loading the file
		fw_framequeue = newStack(unsigned char *); //I like stack because stack_push will realloc
	}

	//video and audo decoded in combined loop (could split for decode-on-load)
	i=0;
	while(av_read_frame(pFormatCtx, &packet)>=0) {
		// Is this a packet from the video stream?
		if(packet.stream_index==videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
    
			// Did we get a video frame?
			if(frameFinished) {
				// Convert the image from its native format to RGB
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);
	
				// Save the frame to disk
				++i;
				//if(++i<=5)
				if(0) if(i<=5){
					SaveFrame(pFrameRGB, pCodecCtx->width, 
							pCodecCtx->height, nchan, i);
				}
				//printf("saving frame %d %d %d\n",pCodecCtx->width,pCodecCtx->height, i);
				//printf("linesize = %d \n",pFrameRGB->linesize[0]);

				unsigned char * fw_frame = malloc(fw_movie.height * fw_movie.width *  nchan); //assumes width == linesize[0]
				
				for(int k=0;k<pCodecCtx->height;k++){
					int kd,ks,kk;
					unsigned char *src;
					kk = pCodecCtx->height - k - 1; //flip y-down to y-up for opengl
					ks = k*pFrame->linesize[0]*nchan;
					kd = kk * fw_movie.width * nchan;
					src = ((unsigned char *)pFrameRGB->data[0]) + ks;
					memcpy(&fw_frame[kd],src,fw_movie.width * nchan);
				}
				stack_push(unsigned char *,fw_framequeue,fw_frame);
			}
			av_free_packet(&packet);
		} else if(packet.stream_index==audioStream) {
			// http://open-activewrl.sourceforge.net/data/OpenAL_PGuide.pdf
			// page 5:
			// "Fill the buffers with PCM data using alBufferData."
			// alBufferData(g_Buffers[0],format,data,size,freq); 
			// Goal: PCM data
			// taking code from decode_audio_frame in ffmpeg tutorial03.c
			int got_frame = 0;
			int len1;
			len1 = avcodec_decode_audio4(aCodecCtx, aFrame, &got_frame, &packet);
			if(len1 < 0) {
				/* if error, skip frame */
				audio_pkt_size = 0;
				continue;
			}
			audio_pkt_data += len1;
			audio_pkt_size -= len1;
			int data_size = 0;
			int buf_size = audio_buf_size - audio_buf_index;
			if(got_frame) {
				data_size = av_samples_get_buffer_size(NULL, 
										aCodecCtx->channels,
										aFrame->nb_samples,
										aCodecCtx->sample_fmt,
										1);
				if(data_size > buf_size){
					audio_buf = realloc(audio_buf,audio_buf_size *2);
					audio_buf_size *= 2;
				}
				memcpy(&audio_buf[audio_buf_index], aFrame->data[0], data_size);
				audio_buf_index += data_size;
			}

		} else {
			// Free the packet that was allocated by av_read_frame
			av_free_packet(&packet);
		}
	}

	//video fin
	if(videoStream > -1){
		fw_movie.frames = fw_framequeue->data;
		fw_movie.nframes = fw_framequeue->n;
		fw_movie.duration = (double)(fw_movie.nframes) / 30.0; //s = frames / fps 

		if(0){
			//write out frames in .web3dit image format for testing
			textureTableIndexStruct_s ttipp, *ttip;
			ttip = &ttipp;
			ttip->x = fw_movie.width;
			ttip->y = fw_movie.height;
			ttip->z = 1;
			ttip->hasAlpha = 1;
			ttip->channels = nchan;

			for(int k=0;k<fw_movie.nframes;k++){
				ttip->texdata = fw_movie.frames[k];
				char namebuf[100];
				sprintf(namebuf,"%s%d.web3dit","ffmpeg_frame_",k);
				saveImage_web3dit(ttip, namebuf);
			}
		}
		//IF(METHOD_DECODE_ON_LOAD)
		//   GARBAGE COLLECT FFMPEG STUFF
		// Free the RGB image
		av_free(buffer);
		av_frame_free(&pFrameRGB);
  
		// Free the YUV frame
		av_frame_free(&pFrame);
  
		// Close the codecs
		avcodec_close(pCodecCtx);
		avcodec_close(pCodecCtxOrig);
	}
	//audio fin
	if(audioStream > -1){
		fw_movie.audio_buf = audio_buf;
		fw_movie.audio_buf_size = audio_buf_index;
		fw_movie.duration = (double)(fw_movie.nframes) / 30.0; //s = frames / fps 

		avcodec_close(aCodecCtxOrig);
		avcodec_close(aCodecCtx);
	}

	//audio and video fin
		// Close the video file
		avformat_close_input(&pFormatCtx);
		*opaque = malloc(sizeof(struct fw_movietexture));
		memcpy(*opaque,&fw_movie,sizeof(struct fw_movietexture));
	

	return 1;
}
double movie_get_duration(void *opaque){
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	return fw_movie->duration;
}

unsigned char *movie_get_frame_by_fraction(void *opaque, float fraction, int *width, int *height, int *nchan){
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	if(!fw_movie) return NULL;

	int iframe = (int)(fraction * ((float)(fw_movie->nframes -1) + .5f));
	iframe = max(0,iframe);
	iframe = min(fw_movie->nframes -1,iframe);
	*width = fw_movie->width;
	*height = fw_movie->height;
	*nchan = fw_movie->nchan;
	return fw_movie->frames[iframe];
}
unsigned char * movie_get_audio_PCM_buffer(void *opaque,int *freq, int *channels, int *size, int *bits){
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	if(!fw_movie) return NULL;
	if(!fw_movie->audio_buf) return NULL;
	*freq = fw_movie->freq;
	*channels = fw_movie->channels;
	*size = fw_movie->audio_buf_size;
	*bits = fw_movie->bits_per_channel;
	return fw_movie->audio_buf;
}
void movie_free(void *opaque){
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	if(fw_movie) {
		for(int k=0;k<fw_movie->nframes;k++){
			FREE_IF_NZ(fw_movie->frames[k]);
		}
		free(opaque);
	}
}

#endif //MOVIETEXTURE_FFMPEG