/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
 * Portions of this software Copyright (c) 1995 Brown University.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement
 * is hereby granted, provided that the above copyright notice and the
 * following two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF BROWN
 * UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * BROWN UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND BROWN UNIVERSITY HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef MPEG_LIB_VIDEO_HEADER
#define MPEG_LIB_VIDEO_HEADER
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef SH_MEM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif


/* X11/xmd.h correctly defines INT32, etc */
#ifndef XMD_H
typedef int INT32;
typedef short INT16;
typedef char INT8;
#endif
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
 
/* Define Parsing error codes. */

#define SKIP_PICTURE (-10)
#define SKIP_TO_START_CODE (-1)
#define PARSE_OK 1

/* Define BOOLEAN, TRUE, and FALSE. */

#define BOOLEAN int
#define TRUE 1
#define FALSE 0

/* Set ring buffer size. */

#define RING_BUF_SIZE 5

/* Macros for picture code type. */

#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

/* Start codes. */

#define SEQ_END_CODE 0x000001b7
#define SEQ_START_CODE 0x000001b3
#define GOP_START_CODE 0x000001b8
#define PICTURE_START_CODE 0x00000100
#define SLICE_MIN_START_CODE 0x00000101
#define SLICE_MAX_START_CODE 0x000001af
#define EXT_START_CODE 0x000001b5
#define USER_START_CODE 0x000001b2
#define SEQUENCE_ERROR_CODE 0x000001b4

/* Number of macroblocks to process in one call to mpeg_VidRsrc. */

#define MB_QUANTUM 100

/* Macros used with macroblock address decoding. */

#define MB_STUFFING 34
#define MB_ESCAPE 35

/* Lock flags for pict images. */

#define DISPLAY_LOCK 0x01
#define PAST_LOCK 0x02
#define FUTURE_LOCK 0x04

#define HYBRID_DITHER 0
#define HYBRID2_DITHER 1
#define FS4_DITHER 2
#define FS2_DITHER 3
#define FS2FAST_DITHER 4
#define Twox2_DITHER 5
#define GRAY_DITHER 6
#define FULL_COLOR_DITHER 7
#define NO_DITHER 8
#define ORDERED_DITHER 9
#define MONO_DITHER 10
#define MONO_THRESHOLD 11
#define ORDERED2_DITHER 12
#define MBORDERED_DITHER 13
#define GRAY256_DITHER 14
#define PPM_DITHER     15
#define FULL_COLOR2_DITHER 16
#define GRAY2_DITHER 17
#define GRAY2562_DITHER 18

#ifdef DISABLE_DITHER
#define IS_2x2_DITHER(a) (0)
#else
#define IS_2x2_DITHER(a) ((a) == Twox2_DITHER || (a) == FULL_COLOR2_DITHER || (a) == GRAY2_DITHER || (a) == (GRAY2562_DITHER))
#endif

/* External declaration of row,col to zig zag conversion matrix. */

/* Brown - changed to const int because it is a help variable */
extern const int scan[][8];

/* Temporary definition of time stamp structure. */

typedef int TimeStamp;

/* Structure with reconstructed pixel values. */

typedef struct pict_image {
  unsigned char *luminance;              /* Luminance plane.   */
  unsigned char *Cr;                     /* Cr plane.          */
  unsigned char *Cb;                     /* Cb plane.          */
  unsigned char *display;                /* Display plane.     */
  int locked;                            /* Lock flag.         */
  TimeStamp show_time;                   /* Presentation time. */

#ifdef SH_MEM
  XShmSegmentInfo shminfo;               /* Segment info.  */
  XImage *ximage;                        /* Ximage struct. */
#endif

} PictImage;

/* Group of pictures structure. */

typedef struct GoP {
  BOOLEAN drop_flag;                     /* Flag indicating dropped frame. */
  unsigned int tc_hours;                 /* Hour component of time code.   */
  unsigned int tc_minutes;               /* Minute component of time code. */
  unsigned int tc_seconds;               /* Second component of time code. */
  unsigned int tc_pictures;              /* Picture counter of time code.  */
  BOOLEAN closed_gop;                    /* Indicates no pred. vectors to
					    previous group of pictures.    */
  BOOLEAN broken_link;                   /* B frame unable to be decoded.  */
  char *ext_data;                        /* Extension data.                */
  char *user_data;                       /* User data.                     */
} GoP;

/* Picture structure. */

typedef struct pict {
  unsigned int temp_ref;                 /* Temporal reference.             */
  unsigned int code_type;                /* Frame type: P, B, I             */
  unsigned int vbv_delay;                /* Buffer delay.                   */
  BOOLEAN full_pel_forw_vector;          /* Forw. vectors specified in full
					    pixel values flag.              */
  unsigned int forw_r_size;              /* Used for vector decoding.       */
  unsigned int forw_f;                   /* Used for vector decoding.       */
  BOOLEAN full_pel_back_vector;          /* Back vectors specified in full 
					    pixel values flag.              */
  unsigned int back_r_size;              /* Used in decoding.               */
  unsigned int back_f;                   /* Used in decoding.               */
  char *extra_info;                      /* Extra bit picture info.         */
  char *ext_data;                        /* Extension data.                 */
  char *user_data;                       /* User data.                      */
} Pict;

/* Slice structure. */

typedef struct slice {
  unsigned int vert_pos;                 /* Vertical position of slice. */
  unsigned int quant_scale;              /* Quantization scale.         */
  char *extra_info;                      /* Extra bit slice info.       */
} Slice;

/* Macroblock structure. */

typedef struct macroblock {
  int mb_address;                        /* Macroblock address.              */
  int past_mb_addr;                      /* Previous mblock address.         */
  int motion_h_forw_code;                /* Forw. horiz. motion vector code. */
  unsigned int motion_h_forw_r;          /* Used in decoding vectors.        */
  int motion_v_forw_code;                /* Forw. vert. motion vector code.  */
  unsigned int motion_v_forw_r;          /* Used in decdoinge vectors.       */
  int motion_h_back_code;                /* Back horiz. motion vector code.  */
  unsigned int motion_h_back_r;          /* Used in decoding vectors.        */
  int motion_v_back_code;                /* Back vert. motion vector code.   */
  unsigned int motion_v_back_r;          /* Used in decoding vectors.        */
  unsigned int cbp;                      /* Coded block pattern.             */
  BOOLEAN mb_intra;                      /* Intracoded mblock flag.          */
  BOOLEAN bpict_past_forw;               /* Past B frame forw. vector flag.  */
  BOOLEAN bpict_past_back;               /* Past B frame back vector flag.   */
  int past_intra_addr;                   /* Addr of last intracoded mblock.  */
  int recon_right_for_prev;              /* Past right forw. vector.         */
  int recon_down_for_prev;               /* Past down forw. vector.          */
  int recon_right_back_prev;             /* Past right back vector.          */
  int recon_down_back_prev;              /* Past down back vector.           */
} Macroblock;

/* Block structure. */

typedef struct block {
  short int dct_recon[8][8];             /* Reconstructed dct coeff matrix. */
  short int dct_dc_y_past;               /* Past lum. dc dct coefficient.   */
  short int dct_dc_cr_past;              /* Past cr dc dct coefficient.     */
  short int dct_dc_cb_past;              /* Past cb dc dct coefficient.     */
} Block;

/* Video stream structure. */

typedef struct vid_stream {
  unsigned int h_size;                         /* Horiz. size in pixels.     */
  unsigned int v_size;                         /* Vert. size in pixels.      */
  unsigned int mb_height;                      /* Vert. size in mblocks.     */
  unsigned int mb_width;                       /* Horiz. size in mblocks.    */
  unsigned char aspect_ratio;                  /* Code for aspect ratio.     */
  unsigned char picture_rate;                  /* Code for picture rate.     */
  unsigned int bit_rate;                       /* Bit rate.                  */
  unsigned int vbv_buffer_size;                /* Minimum buffer size.       */
  BOOLEAN const_param_flag;                    /* Contrained parameter flag. */
  unsigned char intra_quant_matrix[8][8];      /* Quantization matrix for
						  intracoded frames.         */
  unsigned char non_intra_quant_matrix[8][8];  /* Quanitization matrix for 
						  non intracoded frames.     */
  char *ext_data;                              /* Extension data.            */
  char *user_data;                             /* User data.                 */
  GoP group;                                   /* Current group of pict.     */
  Pict picture;                                /* Current picture.           */
  Slice slice;                                 /* Current slice.             */
  Macroblock mblock;                           /* Current macroblock.        */
  Block block;                                 /* Current block.             */
  int state;                                   /* State of decoding.         */
  int bit_offset;                              /* Bit offset in stream.      */
  unsigned int *buffer;                        /* Pointer to next byte in
						  buffer.                    */
  int buf_length;                              /* Length of remaining buffer.*/
  unsigned int *buf_start;                     /* Pointer to buffer start.   */
/* Brown - beginning of added variables that used to be static or global */
  int max_buf_length;                          /* Max length of buffer.      */
  int film_has_ended;                          /* Boolean - film has ended   */
  int sys_layer;                               /* -1 uninitialized,
	                                           0 video layer,
						   1 syslayer                */
  unsigned int num_left;                       /* from ReadPacket - leftover */
  unsigned int leftover_bytes;                 /* from ReadPacket - leftover */
  int EOF_flag;                                /* stream is EOF              */
  FILE *input;                                 /* stream comes from here     */
  long seekValue;                              /* 0 no seeking
						  >0 do a seek,
						  <0 already has done seek   */
  BOOLEAN swap;                                /* from ReadFile              */
  BOOLEAN Parse_done;                          /* from read_sys              */
  int gAudioStreamID;
  int gVideoStreamID;
  int gReservedStreamID;
  int right_for,down_for;                      /* From ReconPMBlock, video.c */
  int right_half_for, down_half_for;
  unsigned int curBits;                        /* current bits               */
  int matched_depth;                           /* depth of displayed movie   */
  char *filename;                              /* Name of stream filename    */
  int ditherType;                              /* What type of dithering     */
  char *ditherFlags;                           /* flags for MB Ordered dither*/
  int totNumFrames;                            /* Total Number of Frames     */
  double realTimeStart;                        /* When did the movie start?  */
/* Brown - end of added variables */
  PictImage *past;                             /* Past predictive frame.     */
  PictImage *future;                           /* Future predictive frame.   */
  PictImage *current;                          /* Current frame.             */
  PictImage *ring[RING_BUF_SIZE];              /* Ring buffer of frames.     */
  /* x,y size of PPM output file */
  int ppm_width, ppm_height, ppm_modulus;
} mpeg_VidStream;   

typedef struct {
  XImage   *ximage;
  Colormap cmap;
  Window   window;
  GC       gc;
  Display  *display;
  int      owncmFlag;
  XSizeHints hints;
  Visual   *visual;
  int      depth;
  char     *name;
  int      ditherType;
  Window   ExistingWindow;
} XInfo;

/* Declaration of global display pointer. */

#if SH_MEM
/* Shared memory flag. */
extern int shmemFlag;
#endif

/* Quiet mode flag. */
extern int quietFlag;

/* Flag controlling the "Press return" prompt */
extern int requireKeypressFlag;

/* Flag controlling speed vs. quality */
extern int qualityFlag;

/* Gamma correction stuff */
extern int gammaCorrectFlag;
extern double gammaCorrect;

/* Chroma correction stuff */
extern int chromaCorrectFlag;
extern double chromaCorrect;

/* Definition of Contant integer scale factor. */

#define CONST_BITS 13

/* Misc DCT definitions */
#define DCTSIZE		8	/* The basic DCT block is 8x8 samples */
#define DCTSIZE2	64	/* DCTSIZE squared; # of elements in a block */

#define GLOBAL			/* a function referenced thru EXTERNs */
  
typedef short DCTELEM;
typedef DCTELEM DCTBLOCK[DCTSIZE2];
 

#ifdef SH_MEM
extern int gXErrorFlag;
#endif

extern int loopFlag;
extern int noDisplayFlag;
extern int partialFlag, startFrame, endFrame;

#ifdef ANALYSIS
extern unsigned int bitCount;
extern int showEachFlag;
extern unsigned int cacheHit[8][8];
extern unsigned int cacheMiss[8][8];
#endif

#if !defined(__MIPSEL__) && (defined(MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || defined(__mipsel) || defined(__mipsel__))
#define __MIPSEL__ 1
#endif

#if !defined(__MIPSEB__) && (defined(MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__) || defined(__mipseb) || defined(__mipseb__))
#define __MIPSEB__ 1
#endif

#if !defined(__SPARC__) && (defined(SPARC) || defined(__SPARC) || defined(__SPARC__) || defined(__sparc) || defined(__sparc__))
#define __SPARC__ 1
#endif

#if !defined(__alpha__) && (defined(ALPHA) || defined(__ALPHA) || defined(__ALPHA__) || defined(__alpha))
#define __alpha__ 1
#endif

#if !defined(__680x0__) && (defined(__680x0) || defined(__680x0__))
#define __680x0__ 1
#endif

#if !defined(__AIX__) && (defined(AIX) || defined(_AIX) || defined(__AIX) || defined(__AIX__))
#define __AIX__ 1
#endif

#if !defined(__RS6000__) && (defined(__AIX__) || defined(RS6000) || defined(_RS6000) || defined(__RS6000) || defined(__RS6000__))
#define __RS6000__ 1
#endif

#if !defined(__HPUX__) && (defined(HPUX) || defined(_HPUX) || defined(__HPUX) || defined(__HPUX__))
#define __HPUX__ 1
#endif
#if !defined(__HPUX__) && (defined(hpux) || defined(_hpux) || defined(__hpux) || defined(__hpux__))
#define __HPUX__ 1
#endif

#if !defined(__VAX__) && (defined(VAX) || defined (__VAX))
#define __VAX__ 1
#endif

#if !defined(__SCO__) && (defined(SCO) || defined(__SCO) || defined(sco) || defined(__sco__))
#define __SCO__ 1
#endif

#if defined(__i386__) || defined(__VAX__) || defined(__MIPSEL__) || defined(__alpha__) || defined(__SCO__)
#undef  BIG_ENDIAN_ARCHITECTURE
#define LITTLE_ENDIAN_ARCHITECTURE 1
#endif

#if defined(__RS6000__) || defined(__SPARC__) || defined(__680x0__) || defined(__HPUX__) || defined(__MIPSEB__) || defined(convex) || defined(__convex__)
#undef  LITTLE_ENDIAN_ARCHITECTURE
#define BIG_ENDIAN_ARCHITECTURE 1
#endif

#if !defined(LITTLE_ENDIAN_ARCHITECTURE) && !defined(BIG_ENDIAN_ARCHITECTURE)
Error: Unknown endianism of architecture
#endif

#ifdef __alpha__
#define SIXTYFOUR_BIT
#endif
#endif /* video.h already included */


#ifdef __STDC__
# define	P(s) s
#include <stdlib.h>	/* used by almost all modules */
#else
# define P(s) ()
#endif

/* util.c */
void correct_underflow P((mpeg_VidStream *vid_stream ));
int next_bits P((int num , unsigned int mask , mpeg_VidStream *vid_stream ));
char *get_ext_data P((mpeg_VidStream *vid_stream ));
int next_start_code P((mpeg_VidStream *vid_stream));
char *get_extra_bit_info P((mpeg_VidStream *vid_stream ));

/* video.c */
void init_stats P((void ));
void PrintAllStats P((mpeg_VidStream *vid_stream ));
double ReadSysClock P((void ));
void PrintTimeInfo P(( mpeg_VidStream *vid_stream ));
void InitCrop P((void ));
mpeg_VidStream *Newmpeg_VidStream P((unsigned int buffer_len ));
#ifndef NOCONTROLS
void Resetmpeg_VidStream P((mpeg_VidStream *vid ));
#endif
void Destroympeg_VidStream P((mpeg_VidStream *astream));
PictImage *NewPictImage P(( mpeg_VidStream *vid_stream ));
void DestroyPictImage P((PictImage *apictimage));
mpeg_VidStream *mpeg_VidRsrc P((TimeStamp time_stamp,mpeg_VidStream *vid_stream, int first ));
void SetBFlag P((BOOLEAN val ));
void SetPFlag P((BOOLEAN val ));

/* parseblock.c */
void ParseReconBlock P((int n, mpeg_VidStream *vid_stream ));
void ParseAwayBlock P((int n , mpeg_VidStream *vid_stream ));

/* motionvector.c */
void ComputeForwVector P((int *recon_right_for_ptr , int *recon_down_for_ptr , mpeg_VidStream *the_stream ));
void ComputeBackVector P((int *recon_right_back_ptr , int *recon_down_back_ptr, mpeg_VidStream *the_stream ));

/* decoders.c */
void mpeg_init_tables P((void ));
void decodeDCTDCSizeLum P((unsigned int *value ));
void decodeDCTDCSizeChrom P((unsigned int *value ));
void decodeDCTCoeffFirst P((unsigned int *run , int *level ));
void decodeDCTCoeffNext P((unsigned int *run , int *level ));

/* from main.c or equivalent */
void DoDitherImage P(( mpeg_VidStream *vid_stream ));

/* readfile.c */
void SeekStream P((mpeg_VidStream *vid_stream ));
void clear_data_stream P(( mpeg_VidStream *vid_stream));
int get_more_data P(( mpeg_VidStream *vid_stream ));
int pure_get_more_data P((unsigned int *buf_start , int max_length , int *length_ptr , unsigned int **buf_ptr, mpeg_VidStream *vid_stream ));
int read_sys P(( mpeg_VidStream *vid_stream, unsigned int start ));
int ReadStartCode P(( unsigned int *startCode, mpeg_VidStream *vid_stream ));

int ReadPackHeader P((
   double *systemClockTime,
   unsigned long *muxRate,
   mpeg_VidStream *vid_stream ));

int ReadSystemHeader P(( mpeg_VidStream *vid_stream ));

int find_start_code P(( FILE *input ));

int ReadPacket P(( unsigned char packetID, mpeg_VidStream *vid_stream ));

void ReadTimeStamp P((
   unsigned char *inputBuffer,
   unsigned char *hiBit,
   unsigned long *low4Bytes));

void ReadSTD P((
   unsigned char *inputBuffer,
   unsigned char *stdBufferScale,
   unsigned long *stdBufferSize));

void ReadRate P((
   unsigned char *inputBuffer,
   unsigned long *rate));

int MakeFloatClockTime P((
   unsigned char hiBit,
   unsigned long low4Bytes,
   double *floatClockTime));


#undef P

/* Status codes for bit stream i/o operations. */

#define NO_VID_STREAM (-1)
#define STREAM_UNDERFLOW (-2)
#define OK 1

/* Size increment of extension data buffers. */

#define EXT_BUF_SIZE 1024

/* External declarations for bitstream i/o operations. */
extern unsigned int bitMask[];
extern unsigned int nBitMask[];
extern unsigned int rBitMask[];
extern unsigned int bitTest[];

/* Macro for updating bit counter if analysis tool is on. */
#ifdef ANALYSIS
#define UPDATE_COUNT(numbits) bitCount += numbits
#else
#define UPDATE_COUNT(numbits)
#endif

#ifdef NO_SANITY_CHECKS
#define get_bits1(result)                                                 \
{                                                                         \
  UPDATE_COUNT(1);                                                        \
  result = ((vid_stream->curBits & 0x80000000) != 0);                     \
  vid_stream->curBits <<= 1;                                              \
  vid_stream->bit_offset++;                                               \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset = 0;                                           \
    vid_stream->buffer++;                                                 \
    vid_stream->curBits = *vid_stream->buffer;                            \
    vid_stream->buf_length--;                                             \
  }                                                                       \
}

#define get_bits2(result)                                                 \
{                                                                         \
  UPDATE_COUNT(2);                                                        \
  vid_stream->bit_offset += 2;                                            \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset -= 32;                                         \
    vid_stream->buffer++;                                                 \
    vid_stream->buf_length--;                                             \
    if (vid_stream->bit_offset) {                                         \
      vid_stream->curBits |=                                              \
	 (*vid_stream->buffer >> (2 - vid_stream->bit_offset));           \
    }                                                                     \
    result = ((vid_stream->curBits & 0xc0000000) >> 30);                  \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;  \
  }                                                                       \
                                                                          \
  result = ((vid_stream->curBits & 0xc0000000) >> 30);                    \
  vid_stream->curBits <<= 2;                                              \
}

#define get_bitsX(num, mask, shift,  result)                              \
{                                                                         \
  UPDATE_COUNT(num);                                                      \
  vid_stream->bit_offset += num;                                          \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset -= 32;                                         \
    vid_stream->buffer++;                                                 \
    vid_stream->buf_length--;                                             \
    if (vid_stream->bit_offset) {                                         \
      vid_stream->curBits |= (*vid_stream->buffer >>                      \
      (num - vid_stream->bit_offset));                                    \
    }                                                                     \
    result = ((vid_stream->curBits & mask) >> shift);                     \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;  \
  }                                                                       \
  else {                                                                  \
    result = ((vid_stream->curBits & mask) >> shift);                     \
    vid_stream->curBits <<= num;                                          \
  }                                                                       \
}
#else

#define get_bits1(result)                                                 \
{                                                                         \
  /* Check for underflow. */                                              \
                                                                          \
  if (vid_stream->buf_length < 2) {                                       \
    correct_underflow(vid_stream);                                        \
  }                                                                       \
  UPDATE_COUNT(1);                                                        \
  result = ((vid_stream->curBits & 0x80000000) != 0);                     \
  vid_stream->curBits <<= 1;                                              \
  vid_stream->bit_offset++;                                               \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset = 0;                                           \
    vid_stream->buffer++;                                                 \
    vid_stream->curBits = *vid_stream->buffer;                            \
    vid_stream->buf_length--;                                             \
  }                                                                       \
}

#define get_bits2(result)                                                 \
{                                                                         \
  /* Check for underflow. */                                              \
                                                                          \
  if (vid_stream->buf_length < 2) {                                       \
    correct_underflow(vid_stream);                                        \
  }                                                                       \
  UPDATE_COUNT(2);                                                        \
  vid_stream->bit_offset += 2;                                            \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset -= 32;                                         \
    vid_stream->buffer++;                                                 \
    vid_stream->buf_length--;                                             \
    if (vid_stream->bit_offset) {                                         \
      vid_stream->curBits |= (*vid_stream->buffer >>                      \
      (2 - vid_stream->bit_offset));                                      \
    }                                                                     \
    result = ((vid_stream->curBits & 0xc0000000) >> 30);                  \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;  \
  }                                                                       \
                                                                          \
  result = ((vid_stream->curBits & 0xc0000000) >> 30);                    \
  vid_stream->curBits <<= 2;                                              \
}

#define get_bitsX(num, mask, shift,  result)                              \
{                                                                         \
  /* Check for underflow. */                                              \
                                                                          \
  if (vid_stream->buf_length < 2) {                                       \
    correct_underflow(vid_stream);                                        \
  }                                                                       \
  UPDATE_COUNT(num);                                                      \
  vid_stream->bit_offset += num;                                          \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset -= 32;                                         \
    vid_stream->buffer++;                                                 \
    vid_stream->buf_length--;                                             \
    if (vid_stream->bit_offset) {                                         \
      vid_stream->curBits |= (*vid_stream->buffer >>                      \
      (num - vid_stream->bit_offset));                                    \
    }                                                                     \
    result = ((vid_stream->curBits & mask) >> shift);                     \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;  \
  }                                                                       \
  else {                                                                  \
   result = ((vid_stream->curBits & mask) >> shift);                      \
   vid_stream->curBits <<= num;                                           \
  }                                                                       \
}
#endif

#define get_bits3(result) get_bitsX(3,   0xe0000000, 29, result)
#define get_bits4(result) get_bitsX(4,   0xf0000000, 28, result)
#define get_bits5(result) get_bitsX(5,   0xf8000000, 27, result)
#define get_bits6(result) get_bitsX(6,   0xfc000000, 26, result)
#define get_bits7(result) get_bitsX(7,   0xfe000000, 25, result)
#define get_bits8(result) get_bitsX(8,   0xff000000, 24, result)
#define get_bits9(result) get_bitsX(9,   0xff800000, 23, result)
#define get_bits10(result) get_bitsX(10, 0xffc00000, 22, result)
#define get_bits11(result) get_bitsX(11, 0xffe00000, 21, result)
#define get_bits12(result) get_bitsX(12, 0xfff00000, 20, result)
#define get_bits14(result) get_bitsX(14, 0xfffc0000, 18, result)
#define get_bits16(result) get_bitsX(16, 0xffff0000, 16, result)
#define get_bits18(result) get_bitsX(18, 0xffffc000, 14, result)
#define get_bits32(result) get_bitsX(32, 0xffffffff,  0, result)

#define get_bitsn(num, result) get_bitsX((num), nBitMask[num], (32-(num)), result)

#ifdef NO_SANITY_CHECKS
#define show_bits32(result)                              		\
{                                                                       \
  if (vid_stream->bit_offset) {					        \
    result = vid_stream->curBits | (*(vid_stream->buffer+1) >>          \
	 (32 - vid_stream->bit_offset));                                \
  }                                                                     \
  else {                                                                \
    result = vid_stream->curBits;					\
  }                                                                     \
}

#define show_bitsX(num, mask, shift,  result)                           \
{                                                                       \
  int bO;                                                               \
  bO = vid_stream->bit_offset + num;                                    \
  if (bO > 32) {                                                        \
    bO -= 32;                                                           \
    result = ((vid_stream->curBits & mask) >> shift) |                  \
                (*(vid_stream->buffer+1) >> (shift + (num - bO)));      \
  }                                                                     \
  else {                                                                \
    result = ((vid_stream->curBits & mask) >> shift);                   \
  }                                                                     \
}

#else
#define show_bits32(result)                               		\
{                                                                       \
  /* Check for underflow. */                                            \
  if (vid_stream->buf_length < 2) {                                     \
    correct_underflow(vid_stream);                                      \
  }                                                                     \
  if (vid_stream->bit_offset) {						\
    result = vid_stream->curBits | (*(vid_stream->buffer+1) >>          \
    (32 - vid_stream->bit_offset));		                        \
  }                                                                     \
  else {                                                                \
    result = vid_stream->curBits;					\
  }                                                                     \
}

#define show_bitsX(num, mask, shift, result)                            \
{                                                                       \
  int bO;                                                               \
                                                                        \
  /* Check for underflow. */                                            \
  if (vid_stream->buf_length < 2) {                                     \
    correct_underflow(vid_stream);                                      \
  }                                                                     \
  bO = vid_stream->bit_offset + num;                                    \
  if (bO > 32) {                                                        \
    bO -= 32;                                                           \
    result = ((vid_stream->curBits & mask) >> shift) |                  \
                (*(vid_stream->buffer+1) >> (shift + (num - bO)));      \
  }                                                                     \
  else {                                                                \
    result = ((vid_stream->curBits & mask) >> shift);                   \
  }                                                                     \
}
#endif

#define show_bits1(result)  show_bitsX(1,  0x80000000, 31, result)
#define show_bits2(result)  show_bitsX(2,  0xc0000000, 30, result)
#define show_bits3(result)  show_bitsX(3,  0xe0000000, 29, result)
#define show_bits4(result)  show_bitsX(4,  0xf0000000, 28, result)
#define show_bits5(result)  show_bitsX(5,  0xf8000000, 27, result)
#define show_bits6(result)  show_bitsX(6,  0xfc000000, 26, result)
#define show_bits7(result)  show_bitsX(7,  0xfe000000, 25, result)
#define show_bits8(result)  show_bitsX(8,  0xff000000, 24, result)
#define show_bits9(result)  show_bitsX(9,  0xff800000, 23, result)
#define show_bits10(result) show_bitsX(10, 0xffc00000, 22, result)
#define show_bits11(result) show_bitsX(11, 0xffe00000, 21, result)
#define show_bits12(result) show_bitsX(12, 0xfff00000, 20, result)
#define show_bits13(result) show_bitsX(13, 0xfff80000, 19, result)
#define show_bits14(result) show_bitsX(14, 0xfffc0000, 18, result)
#define show_bits15(result) show_bitsX(15, 0xfffe0000, 17, result)
#define show_bits16(result) show_bitsX(16, 0xffff0000, 16, result)
#define show_bits17(result) show_bitsX(17, 0xffff8000, 15, result)
#define show_bits18(result) show_bitsX(18, 0xffffc000, 14, result)
#define show_bits19(result) show_bitsX(19, 0xffffe000, 13, result)
#define show_bits20(result) show_bitsX(20, 0xfffff000, 12, result)
#define show_bits21(result) show_bitsX(21, 0xfffff800, 11, result)
#define show_bits22(result) show_bitsX(22, 0xfffffc00, 10, result)
#define show_bits23(result) show_bitsX(23, 0xfffffe00,  9, result)
#define show_bits24(result) show_bitsX(24, 0xffffff00,  8, result)
#define show_bits25(result) show_bitsX(25, 0xffffff80,  7, result)
#define show_bits26(result) show_bitsX(26, 0xffffffc0,  6, result)
#define show_bits27(result) show_bitsX(27, 0xffffffe0,  5, result)
#define show_bits28(result) show_bitsX(28, 0xfffffff0,  4, result)
#define show_bits29(result) show_bitsX(29, 0xfffffff8,  3, result)
#define show_bits30(result) show_bitsX(30, 0xfffffffc,  2, result)
#define show_bits31(result) show_bitsX(31, 0xfffffffe,  1, result)

#define show_bitsn(num,result) show_bitsX((num), (0xffffffff << (32-(num))), (32-(num)), result)

#ifdef NO_SANITY_CHECKS
#define flush_bits32                                                  \
{                                                                     \
  UPDATE_COUNT(32);                                                   \
                                                                      \
  vid_stream->buffer++;                                               \
  vid_stream->buf_length--;                                           \
  vid_stream->curBits = *vid_stream->buffer  << vid_stream->bit_offset;\
}


#define flush_bits(num)                                               \
{                                                                     \
  vid_stream->bit_offset += num;                                      \
                                                                      \
  UPDATE_COUNT(num);                                                  \
                                                                      \
  if (vid_stream->bit_offset & 0x20) {                                \
    vid_stream->bit_offset -= 32;                                     \
    vid_stream->buffer++;                                             \
    vid_stream->buf_length--;                                         \
    vid_stream->curBits = *vid_stream->buffer  << vid_stream->bit_offset;\
  }                                                                   \
  else {                                                              \
    vid_stream->curBits <<= num;                                      \
  }                                                                   \
}
#else
#define flush_bits32                                                  \
{                                                                     \
  if (vid_stream  == NULL) {                                          \
    /* Deal with no vid stream here. */                               \
  }                                                                   \
                                                                      \
  if (vid_stream->buf_length < 2) {                                   \
    correct_underflow(vid_stream);                                    \
  }                                                                   \
                                                                      \
  UPDATE_COUNT(32);                                                   \
                                                                      \
  vid_stream->buffer++;                                               \
  vid_stream->buf_length--;                                           \
  vid_stream->curBits = *vid_stream->buffer  << vid_stream->bit_offset;\
}

#define flush_bits(num)                                               \
{                                                                     \
  if (vid_stream== NULL) {                                            \
    /* Deal with no vid stream here. */                               \
  }                                                                   \
                                                                      \
  if (vid_stream->buf_length < 2) {                                   \
    correct_underflow(vid_stream);                                    \
  }                                                                   \
                                                                      \
  UPDATE_COUNT(num);                                                  \
                                                                      \
  vid_stream->bit_offset += num;                                      \
                                                                      \
  if (vid_stream->bit_offset & 0x20) {                                \
    vid_stream->buf_length--;                                         \
    vid_stream->bit_offset -= 32;                                     \
    vid_stream->buffer++;                                             \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;\
  }                                                                   \
  else {                                                              \
    vid_stream->curBits <<= num;                                      \
  }                                                                   \
}
#endif

#define UTIL2

extern int LUM_RANGE;
extern int CR_RANGE;
extern int CB_RANGE;


#define CB_BASE 1
#define CR_BASE (CB_BASE*CB_RANGE)
#define LUM_BASE (CR_BASE*CR_RANGE)

extern unsigned char pixel[256];
extern unsigned long wpixel[256];
extern int *lum_values;
extern int *cr_values;
extern int *cb_values;

#define Min(x,y) (((x) < (y)) ? (x) : (y))
#define Max(x,y) (((x) > (y)) ? (x) : (y))

#define GAMMA_CORRECTION(x) ((int)(pow((x) / 255.0, 1.0 / gammaCorrect) * 255.0))
#define CHROMA_CORRECTION256(x) ((x) >= 128 \
                        ? 128 + Min(127, (int)(((x) - 128.0) * chromaCorrect)) \
                        : 128 - Min(128, (int)((128.0 - (x)) * chromaCorrect)))
#define CHROMA_CORRECTION128(x) ((x) >= 0 \
                        ? Min(127,  (int)(((x) * chromaCorrect))) \
                        : Max(-128, (int)(((x) * chromaCorrect))))
#define CHROMA_CORRECTION256D(x) ((x) >= 128 \
                        ? 128.0 + Min(127.0, (((x) - 128.0) * chromaCorrect)) \
                        : 128.0 - Min(128.0, (((128.0 - (x)) * chromaCorrect))))
#define CHROMA_CORRECTION128D(x) ((x) >= 0 \
                        ? Min(127.0,  ((x) * chromaCorrect)) \
                        : Max(-128.0, ((x) * chromaCorrect)))



/* Code for unbound values in decoding tables */
#define ERROR (-1)
#define DCT_ERROR 63

#define MACRO_BLOCK_STUFFING 34
#define MACRO_BLOCK_ESCAPE 35

/* Two types of DCT Coefficients */
#define DCT_COEFF_FIRST 0
#define DCT_COEFF_NEXT 1

/* Special values for DCT Coefficients */
#define END_OF_BLOCK 62
#define ESCAPE 61

/* Structure for an entry in the decoding table of 
 * macroblock_address_increment */
typedef struct {
  int value;       /* value for macroblock_address_increment */
  int num_bits;             /* length of the Huffman code */
} mb_addr_inc_entry;

/* Structure for an entry in the decoding table of macroblock_type */
typedef struct {
  unsigned int mb_quant;              /* macroblock_quant */
  unsigned int mb_motion_forward;     /* macroblock_motion_forward */
  unsigned int mb_motion_backward;    /* macroblock_motion_backward */
  unsigned int mb_pattern;            /* macroblock_pattern */
  unsigned int mb_intra;              /* macroblock_intra */
  int num_bits;                       /* length of the Huffman code */
} mb_type_entry;

/* Decoding table for macroblock_type in predictive-coded pictures */
//JASextern mb_type_entry mb_type_P[64];

/* Decoding table for macroblock_type in bidirectionally-coded pictures */
//JASextern mb_type_entry mb_type_B[64];


/* Structures for an entry in the decoding table of coded_block_pattern */
typedef struct {
  unsigned int cbp;            /* coded_block_pattern */
  int num_bits;                /* length of the Huffman code */
} coded_block_pattern_entry;

/* External declaration of coded block pattern table. */

extern coded_block_pattern_entry coded_block_pattern[512];



/* Structure for an entry in the decoding table of motion vectors */
typedef struct {
  int code;              /* value for motion_horizontal_forward_code,
			  * motion_vertical_forward_code, 
			  * motion_horizontal_backward_code, or
			  * motion_vertical_backward_code.
			  */
  int num_bits;          /* length of the Huffman code */
} motion_vectors_entry;


/* Decoding table for motion vectors */
extern motion_vectors_entry motion_vectors[2048];


/* Structure for an entry in the decoding table of dct_dc_size */
typedef struct {
  unsigned int value;    /* value of dct_dc_size (luminance or chrominance) */
  int num_bits;          /* length of the Huffman code */
} dct_dc_size_entry;

/* External declaration of dct dc size lumiance table. */

//JASextern dct_dc_size_entry dct_dc_size_luminance[32];
//JASextern dct_dc_size_entry dct_dc_size_luminance1[16];

/* External declaration of dct dc size chrom table. */

//JASextern dct_dc_size_entry dct_dc_size_chrominance[32];
//JASextern dct_dc_size_entry dct_dc_size_chrominance1[32];


/* DCT coeff tables. */

#define RUN_MASK 0xfc00
#define LEVEL_MASK 0x03f0
#define NUM_MASK 0x000f
#define RUN_SHIFT 10
#define LEVEL_SHIFT 4

/* External declaration of dct coeff tables. */

//JASextern unsigned short int dct_coeff_tbl_0[256];
//JASextern unsigned short int dct_coeff_tbl_1[16];
//JASextern unsigned short int dct_coeff_tbl_2[4];
//JASextern unsigned short int dct_coeff_tbl_3[4];
//JASextern unsigned short int dct_coeff_next[256];
//JASextern unsigned short int dct_coeff_first[256];

#define DecodeDCTDCSizeLum(macro_val)                    \
{                                                    \
  unsigned int index;	\
	\
  show_bits5(index);	\
  	\
  if (index < 31) {	\
  	macro_val = dct_dc_size_luminance[index].value;	\
  	flush_bits(dct_dc_size_luminance[index].num_bits);	\
  }	\
  else {	\
	show_bits9(index);	\
	index -= 0x1f0;	\
	macro_val = dct_dc_size_luminance1[index].value;	\
	flush_bits(dct_dc_size_luminance1[index].num_bits);	\
  }	\
}

#define DecodeDCTDCSizeChrom(macro_val)                      \
{                                                        \
  unsigned int index;	\
	\
  show_bits5(index);	\
  	\
  if (index < 31) {	\
  	macro_val = dct_dc_size_chrominance[index].value;	\
  	flush_bits(dct_dc_size_chrominance[index].num_bits);	\
  }	\
  else {	\
	show_bits10(index);	\
	index -= 0x3e0;	\
	macro_val = dct_dc_size_chrominance1[index].value;	\
	flush_bits(dct_dc_size_chrominance1[index].num_bits);	\
  }	\
}

#define DecodeDCTCoeff(dct_coeff_tbl, run, level)			\
{									\
  unsigned int temp, index;						\
  unsigned int value, next32bits, flushed;				\
									\
  /*									\
   * Grab the next 32 bits and use it to improve performance of		\
   * getting the bits to parse. Thus, calls are translated as:		\
   *									\
   *	show_bitsX  <-->   next32bits >> (32-X)				\
   *	get_bitsX   <-->   val = next32bits >> (32-flushed-X);		\
   *			   flushed += X;				\
   *			   next32bits &= bitMask[flushed];		\
   *	flush_bitsX <-->   flushed += X;				\
   *			   next32bits &= bitMask[flushed];		\
   *									\
   * I've streamlined the code a lot, so that we don't have to mask	\
   * out the low order bits and a few of the extra adds are removed.	\
   */									\
  show_bits32(next32bits);						\
									\
  /* show_bits8(index); */						\
  index = next32bits >> 24;						\
									\
  if (index > 3) {							\
    value = dct_coeff_tbl[index];					\
    run = value >> RUN_SHIFT;						\
    if (run != END_OF_BLOCK) {						\
      /* num_bits = (value & NUM_MASK) + 1; */				\
      /* flush_bits(num_bits); */					\
      if (run != ESCAPE) {						\
	 /* get_bits1(value); */					\
	 /* if (value) level = -level; */				\
	 flushed = (value & NUM_MASK) + 2;				\
         level = (value & LEVEL_MASK) >> LEVEL_SHIFT;			\
	 value = next32bits >> (32-flushed);				\
	 value &= 0x1;							\
	 if (value) level = -level;					\
	 /* next32bits &= ((~0) >> flushed);  last op before update */	\
       }								\
       else {    /* run == ESCAPE */					\
	 /* Get the next six into run, and next 8 into temp */		\
         /* get_bits14(temp); */					\
	 flushed = (value & NUM_MASK) + 1;				\
	 temp = next32bits >> (18-flushed);				\
	 /* Normally, we'd ad 14 to flushed, but I've saved a few	\
	  * instr by moving the add below */				\
	 temp &= 0x3fff;						\
	 run = temp >> 8;						\
	 temp &= 0xff;							\
	 if (temp == 0) {						\
            /* get_bits8(level); */					\
	    level = next32bits >> (10-flushed);				\
	    level &= 0xff;						\
	    flushed += 22;						\
 	    assert(level >= 128);					\
	 } else if (temp != 128) {					\
	    /* Grab sign bit */						\
	    flushed += 14;						\
	    level = ((int) (temp << 24)) >> 24;				\
	 } else {							\
            /* get_bits8(level); */					\
	    level = next32bits >> (10-flushed);				\
	    level &= 0xff;						\
	    flushed += 22;						\
	    level = level - 256;					\
	    assert(level <= -128 && level >= -255);			\
	 }								\
       }								\
       /* Update bitstream... */					\
       flush_bits(flushed);						\
       assert (flushed <= 32);						\
    }									\
  }									\
  else {								\
    switch (index) {                                                    \
    case 2: {   							\
      /* show_bits10(index); */						\
      index = next32bits >> 22;						\
      value = dct_coeff_tbl_2[index & 3];				\
      break;                                                            \
    }									\
    case 3: { 						                \
      /* show_bits10(index); */						\
      index = next32bits >> 22;						\
      value = dct_coeff_tbl_3[index & 3];				\
      break;                                                            \
    }									\
    case 1: {                                             		\
      /* show_bits12(index); */						\
      index = next32bits >> 20;						\
      value = dct_coeff_tbl_1[index & 15];				\
      break;                                                            \
    }									\
    default: { /* index == 0 */						\
      /* show_bits16(index); */						\
      index = next32bits >> 16;						\
      value = dct_coeff_tbl_0[index & 255];				\
    }}									\
    run = value >> RUN_SHIFT;						\
    level = (value & LEVEL_MASK) >> LEVEL_SHIFT;			\
									\
    /*									\
     * Fold these operations together to make it fast...		\
     */									\
    /* num_bits = (value & NUM_MASK) + 1; */				\
    /* flush_bits(num_bits); */						\
    /* get_bits1(value); */						\
    /* if (value) level = -level; */					\
									\
    flushed = (value & NUM_MASK) + 2;					\
    value = next32bits >> (32-flushed);					\
    value &= 0x1;							\
    if (value) level = -level;						\
									\
    /* Update bitstream ... */						\
    flush_bits(flushed);						\
    assert (flushed <= 32);						\
  }									\
}

#define DecodeDCTCoeffFirst(runval, levelval)         \
{                                                     \
  DecodeDCTCoeff(dct_coeff_first, runval, levelval);  \
}          

#define DecodeDCTCoeffNext(runval, levelval)          \
{                                                     \
  DecodeDCTCoeff(dct_coeff_next, runval, levelval);   \
}

/*
 *--------------------------------------------------------------
 *
 * DecodeMBAddrInc --
 *
 *      Huffman Decoder for macro_block_address_increment; the location
 *      in which the result will be placed is being passed as argument.
 *      The decoded value is obtained by doing a table lookup on
 *      mb_addr_inc.
 *
 * Results:
 *      The decoded value for macro_block_address_increment or ERROR
 *      for unbound values will be placed in the location specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeMBAddrInc(val)				\
{							\
    unsigned int index;					\
    show_bits11(index);					\
    val = mb_addr_inc[index].value;			\
    flush_bits(mb_addr_inc[index].num_bits);		\
}

/*
 *--------------------------------------------------------------
 *
 * DecodeMotionVectors --
 *
 *      Huffman Decoder for the various motion vectors, including
 *      motion_horizontal_forward_code, motion_vertical_forward_code,
 *      motion_horizontal_backward_code, motion_vertical_backward_code.
 *      Location where the decoded result will be placed is being passed
 *      as argument. The decoded values are obtained by doing a table
 *      lookup on motion_vectors.
 *
 * Results:
 *      The decoded value for the motion vector or ERROR for unbound
 *      values will be placed in the location specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */

#define DecodeMotionVectors(value)			\
{							\
  unsigned int index;					\
  show_bits11(index);					\
  value = motion_vectors[index].code;			\
  flush_bits(motion_vectors[index].num_bits);		\
}
/*
 *--------------------------------------------------------------
 *
 * DecodeMBTypeB --
 *
 *      Huffman Decoder for macro_block_type in bidirectionally-coded
 *      pictures;locations in which the decoded results: macroblock_quant,
 *      macroblock_motion_forward, macro_block_motion_backward,
 *      macroblock_pattern, macro_block_intra, will be placed are
 *      being passed as argument. The decoded values are obtained by
 *      doing a table lookup on mb_type_B.
 *
 * Results:
 *      The various decoded values for macro_block_type in
 *      bidirectionally-coded pictures or ERROR for unbound values will
 *      be placed in the locations specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeMBTypeB(quant, motion_fwd, motion_bwd, pat, intra)	\
{									\
  unsigned int index;							\
									\
  show_bits6(index);							\
									\
  quant = mb_type_B[index].mb_quant;					\
  motion_fwd = mb_type_B[index].mb_motion_forward;			\
  motion_bwd = mb_type_B[index].mb_motion_backward;			\
  pat = mb_type_B[index].mb_pattern;					\
  intra = mb_type_B[index].mb_intra;					\
  flush_bits(mb_type_B[index].num_bits);				\
}
/*
 *--------------------------------------------------------------
 *
 * DecodeMBTypeI --
 *
 *      Huffman Decoder for macro_block_type in intra-coded pictures;
 *      locations in which the decoded results: macroblock_quant,
 *      macroblock_motion_forward, macro_block_motion_backward,
 *      macroblock_pattern, macro_block_intra, will be placed are
 *      being passed as argument.
 *
 * Results:
 *      The various decoded values for macro_block_type in intra-coded
 *      pictures or ERROR for unbound values will be placed in the
 *      locations specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeMBTypeI(quant, motion_fwd, motion_bwd, pat, intra)	\
{									\
  unsigned int index;							\
  static int quantTbl[4] = {ERROR, 1, 0, 0};				\
									\
  show_bits2(index);							\
									\
  motion_fwd = 0;							\
  motion_bwd = 0;							\
  pat = 0;								\
  intra = 1;								\
  quant = quantTbl[index];						\
  if (index) {								\
    flush_bits (1 + quant);						\
  }									\
}
/*
 *--------------------------------------------------------------
 *
 * DecodeMBTypeP --
 *
 *      Huffman Decoder for macro_block_type in predictive-coded pictures;
 *      locations in which the decoded results: macroblock_quant,
 *      macroblock_motion_forward, macro_block_motion_backward,
 *      macroblock_pattern, macro_block_intra, will be placed are
 *      being passed as argument. The decoded values are obtained by
 *      doing a table lookup on mb_type_P.
 *
 * Results:
 *      The various decoded values for macro_block_type in
 *      predictive-coded pictures or ERROR for unbound values will be
 *      placed in the locations specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeMBTypeP(quant, motion_fwd, motion_bwd, pat, intra)	\
{									\
  unsigned int index;							\
									\
  show_bits6(index);							\
									\
  quant = mb_type_P[index].mb_quant;					\
  motion_fwd = mb_type_P[index].mb_motion_forward;			\
  motion_bwd = mb_type_P[index].mb_motion_backward;			\
  pat = mb_type_P[index].mb_pattern;					\
  intra = mb_type_P[index].mb_intra;					\
									\
  flush_bits(mb_type_P[index].num_bits);				\
}
/*
 *--------------------------------------------------------------
 *
 * DecodeCBP --
 *
 *      Huffman Decoder for coded_block_pattern; location in which the
 *      decoded result will be placed is being passed as argument. The
 *      decoded values are obtained by doing a table lookup on
 *      coded_block_pattern.
 *
 * Results:
 *      The decoded value for coded_block_pattern or ERROR for unbound
 *      values will be placed in the location specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeCBP(coded_bp)						\
{									\
  unsigned int index;							\
									\
  show_bits9(index);							\
  coded_bp = coded_block_pattern[index].cbp;				\
  flush_bits(coded_block_pattern[index].num_bits);			\
}
