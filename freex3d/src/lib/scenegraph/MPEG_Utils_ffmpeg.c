
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

struct fw_movietexture {
	AVFormatContext *pFormatCtx;
	AVCodecContext *pVideoCodecCtx;
	int width,height,nchan,nframes,fps;
	double duration;
	unsigned char **frames;
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
	fw_movie.pFormatCtx = pFormatCtx;

	int i, videoStream;
	AVCodecContext *pCodecCtxOrig = NULL;
	AVCodecContext *pCodecCtx = NULL;

	// Find the first video stream
	videoStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			videoStream=i;
			break;
		}
	if(videoStream==-1)
		return -1; // Didn't find a video stream

	// Get a pointer to the codec context for the video stream
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;

	AVCodec *pCodec = NULL;

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
	fw_movie.pVideoCodecCtx = pCodecCtx;

	AVFrame *pFrame = NULL;

	// Allocate video frame
	pFrame=av_frame_alloc();

	// Allocate an AVFrame structure
	AVFrame *pFrameRGB = NULL;
	pFrameRGB=av_frame_alloc();
	if(pFrameRGB==NULL)
		return -1;

	uint8_t *buffer = NULL;
	int numBytes;
	// Determine required buffer size and allocate buffer
	int nchan, av_pix_fmt;
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


	struct SwsContext *sws_ctx = NULL;
	int frameFinished;
	AVPacket packet;
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

	Stack *fw_framequeue = newStack(unsigned char *); //I like stack because stack_push will realloc
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
					kk = pCodecCtx->height - k - 1;
					ks = k*pFrame->linesize[0]*nchan;
					kd = kk * fw_movie.width * nchan;
					src = ((unsigned char *)pFrameRGB->data[0]) + ks;
					memcpy(&fw_frame[kd],src,fw_movie.width * nchan);
				}
				stack_push(unsigned char *,fw_framequeue,fw_frame);
			}
		}
		
		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}
	fw_movie.frames = fw_framequeue->data;
	fw_movie.nframes = fw_framequeue->n;
	fw_movie.duration = (double)(fw_movie.nframes) / 30.0; //s = frames / fps 

	*opaque = malloc(sizeof(struct fw_movietexture));
	memcpy(*opaque,&fw_movie,sizeof(struct fw_movietexture));
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
	//IF(DECODE-ON-LOAD)
	//   GARBAGE COLLECT FFMPEG STUFF
	// Free the RGB image
	av_free(buffer);
	av_frame_free(&pFrameRGB);
  
	// Free the YUV frame
	av_frame_free(&pFrame);
  
	// Close the codecs
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	return 1;
}
double movie_get_duration(void *opaque){
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	return fw_movie->duration;
}
//int movie_get_nearest_frame_by_movie_time(void *opaque, int *x, int *y, int *nchan, unsigned char *frame){
//	return 0;
//}
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
int movie_get_audio_channel(void *opaque, unsigned char *audiobuf){
	return 0;
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