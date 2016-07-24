/*

  FreeWRL support library.
  New implementation of texture loading.

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
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include "vrml_parser/Structs.h"
#include "main/ProdCon.h"
#include "OpenGL_Utils.h"
#include "Textures.h"
#include "LoadTextures.h"
#include "../scenegraph/Component_CubeMapTexturing.h"

#include <list.h>
#include <io_files.h>
#include <io_http.h>

#include <threads.h>

#include <libFreeWRL.h>

/* We do not want to include Struct.h: enormous file :) */
typedef struct _Multi_String Multi_String;
void Multi_String_print(struct Multi_String *url);

#ifdef _MSC_VER
#include "ImageLoader.h"
#else
#if !(defined(TARGET_AQUA) || defined(IPHONE) || defined(_ANDROID) || defined(ANDROIDNDK))
		#include <Imlib2.h>
	#endif
#endif


#if defined (TARGET_AQUA)

#ifdef IPHONE
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#else
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#endif /* IPHONE */
#endif /* TARGET_AQUA */

///* is the texture thread up and running yet? */
//int TextureThreadInitialized = FALSE;



//GLuint defaultBlankTexture;

typedef struct pLoadTextures{
	s_list_t* texture_request_list;// = NULL;
	bool loader_waiting;// = false;
	/* list of texture table entries to load */
	s_list_t *texture_list;// = NULL;
	/* are we currently active? */
	int TextureParsing; // = FALSE;
}* ppLoadTextures;
void *LoadTextures_constructor(){
	void *v = MALLOCV(sizeof(struct pLoadTextures));
	memset(v,0,sizeof(struct pLoadTextures));
	return v;
}
void LoadTextures_init(struct tLoadTextures *t)
{
	//public
	/* is the texture thread up and running yet? */
	//t->TextureThreadInitialized = FALSE;

	//private
	t->prv = LoadTextures_constructor();
	{
		ppLoadTextures p = (ppLoadTextures)t->prv;
		p->texture_request_list = NULL;
		p->loader_waiting = false;
		/* list of texture table entries to load */
		p->texture_list = NULL;
		/* are we currently active? */
		p->TextureParsing = FALSE;
	}
}
//s_list_t* texture_request_list = NULL;
//bool loader_waiting = false;


/* All functions here works with the array of 'textureTableIndexStruct'.
 * In the future we may want to refactor this struct.
 * In the meantime lets make it work :).
 */

#ifdef TEXVERBOSE
static void texture_dump_entry(textureTableIndexStruct_s *entry)
{
	DEBUG_MSG("%s\t%p\t%s\n", texst(entry->status), entry, entry->filename);
}
#endif

void texture_dump_list()
{
#ifdef TEXVERBOSE
	DEBUG_MSG("TEXTURE: wait queue\n");
	ppLoadTextures p = (ppLoadTextures)gglobal()->LoadTextures.prv;
	ml_foreach(p->texture_list, texture_dump_entry(ml_elem(__l)));
	DEBUG_MSG("TEXTURE: end wait queue\n");
#endif
}


/**
 *   texture_load_from_pixelTexture: have a PixelTexture node,
 *                           load it now.
 */
static void texture_load_from_pixelTexture (textureTableIndexStruct_s* this_tex, struct X3D_PixelTexture *node)
{

/* load a PixelTexture that is stored in the PixelTexture as an MFInt32 */
	int hei,wid,depth;
	unsigned char *texture;
	int count;
	int ok;
	int *iptr;
	int tctr;

	iptr = node->image.p;

	ok = TRUE;

	DEBUG_TEX ("start of texture_load_from_pixelTexture...\n");

	/* are there enough numbers for the texture? */
	if (node->image.n < 3) {
		printf ("PixelTexture, need at least 3 elements, have %d\n",node->image.n);
		ok = FALSE;
	} else {
		wid = *iptr; iptr++;
		hei = *iptr; iptr++;
		depth = *iptr; iptr++;

		DEBUG_TEX ("wid %d hei %d depth %d\n",wid,hei,depth);

		if ((depth < 0) || (depth >4)) {
			printf ("PixelTexture, depth %d out of range, assuming 1\n",(int) depth);
			depth = 1;
		}
	
		if ((wid*hei-3) > node->image.n) {
			printf ("PixelTexture, not enough data for wid %d hei %d, have %d\n",
					wid, hei, (wid*hei)-2);
			ok = FALSE;
		}
	}

	/* did we have any errors? if so, create a grey pixeltexture and get out of here */
	if (!ok) {
		return;
	}

	/* ok, we are good to go here */
	this_tex->x = wid;
	this_tex->y = hei;
	this_tex->hasAlpha = ((depth == 2) || (depth == 4));

	texture = MALLOC (unsigned char *, wid*hei*4);
	this_tex->texdata = texture; /* this will be freed when texture opengl-ized */
	this_tex->status = TEX_NEEDSBINDING;

	tctr = 0;
	if(texture != NULL){
		for (count = 0; count < (wid*hei); count++) {
			switch (depth) {
				case 1: {
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = 0xff; /*alpha, but force it to be ff */
					   break;
				   }
				case 2: {
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (*iptr>>0) & 0xff; /*A*/
					   break;
				   }
				case 3: {
					   texture[tctr++] = (*iptr>>0) & 0xff; /*B*/
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (*iptr>>16) & 0xff; /*R*/
					   texture[tctr++] = 0xff; /*alpha, but force it to be ff */
					   break;
				   }
				case 4: {
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*B*/
					   texture[tctr++] = (*iptr>>16) & 0xff; /*G*/
					   texture[tctr++] = (*iptr>>24) & 0xff; /*R*/
					   texture[tctr++] = (*iptr>>0) & 0xff; /*A*/
					   break;
				   }
			}
			iptr++;
		}
	}
}



/* rewrite MovieTexture loading - for now, just do a blank texture. See:
	HAVE_TO_REIMPLEMENT_MOVIETEXTURES
define */
//static void texture_load_from_MovieTexture (textureTableIndexStruct_s* this_tex)
//{
//	printf("in texture_load_from_MovieTexture\n");
//}

#if defined(_ANDROID) || defined(ANDROIDNDK)
// sometimes (usually?) we have to flip an image vertically. 
static unsigned char *flipImageVerticallyB(unsigned char *input, int height, int width, int bpp) {
	int i,ii,rowcount;
	unsigned char *sourcerow, *destrow;
	unsigned char * blob;
    
	rowcount = width * bpp; //4; bytes per pixel
	blob = MALLOC(unsigned char*, height * rowcount);
	for(i=0;i<height;i++) {
		ii = height - 1 - i;
		sourcerow = &input[i*rowcount];
		destrow = &blob[ii*rowcount];
		memcpy(destrow,sourcerow,rowcount);
	}
	//FREE_IF_NZ(input);
	return blob;
}
static unsigned char *flipImageVertically(unsigned char *input, int height, int width) {
	return flipImageVerticallyB(input,height,width,4);
}
static unsigned char *expandto4bpp(unsigned char *input, int height, int width, int bpp) {
	int i, j, rowcountin, rowcountout;
	unsigned char *sourcerow, *destrow;
	unsigned char * blob;

	rowcountin = width * bpp; //bytes per pixel
	rowcountout = width * 4;
	blob = MALLOCV(height * rowcountout);
	for (i = 0; i<height; i++) {
		sourcerow = &input[i*rowcountin];
		destrow = &blob[i*rowcountout];
		for(j=0;j<width;j++){
			memcpy(&destrow[j*4], &sourcerow[j*bpp], bpp);
			destrow[j*4 + 3] = 255;
		}
	}
	//FREE_IF_NZ(input);
	return blob;
}
#endif //ANDROID - for flipImageVertically



#if defined (TARGET_AQUA)
/* render from aCGImageRef into a buffer, to get EXACT bits, as a CGImageRef contains only
estimates. */
/* from http://developer.apple.com/qa/qa2007/qa1509.html */

static inline double radians (double degrees) {return degrees * M_PI/180;} 

int XXX;

CGContextRef CreateARGBBitmapContext (CGImageRef inImage) {

	CGContextRef    context = NULL;
	CGColorSpaceRef colorSpace;
	int             bitmapByteCount;
	int             bitmapBytesPerRow;
	CGBitmapInfo	bitmapInfo;
	size_t		bitsPerComponent;

	 // Get image width, height. Well use the entire image.
	int pixelsWide = (int) CGImageGetWidth(inImage);
	int pixelsHigh = (int) CGImageGetHeight(inImage);

	// Declare the number of bytes per row. Each pixel in the bitmap in this
	// example is represented by 4 bytes; 8 bits each of red, green, blue, and
	// alpha.
	bitmapBytesPerRow   = (pixelsWide * 4);
	bitmapByteCount     = (bitmapBytesPerRow * pixelsHigh);

	// Use the generic RGB color space.
    colorSpace = CGColorSpaceCreateDeviceRGB();
	if (colorSpace == NULL)
	{
	    fprintf(stderr, "Error allocating color space\n");
	    return NULL;
	}

	
	/* figure out the bitmap mapping */
	bitsPerComponent = CGImageGetBitsPerComponent(inImage);

	if (bitsPerComponent >= 8) {
		CGRect rect = CGRectMake(0., 0., pixelsWide, pixelsHigh);
		bitmapInfo = kCGImageAlphaNoneSkipLast;

		bitmapInfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
	
		/* Create the bitmap context. We want pre-multiplied ARGB, 8-bits
		  per component. Regardless of what the source image format is
		  (CMYK, Grayscale, and so on) it will be converted over to the format
		  specified here by CGBitmapContextCreate. */
		context = CGBitmapContextCreate (NULL, pixelsWide, pixelsHigh,
			bitsPerComponent, bitmapBytesPerRow, colorSpace, bitmapInfo); 
	
		if (context == NULL) {
		    fprintf (stderr, "Context not created!");
		} else {
	
			/* try scaling and rotating this image to fit our ideas on life in general */
			CGContextTranslateCTM (context, 0, pixelsHigh);
			CGContextScaleCTM (context,1.0, -1.0);
		}
		CGContextDrawImage(context, rect,inImage);
	}
    else
    {
        CGColorSpaceRelease(colorSpace);
		printf ("bits per component of %d not handled\n",(int) bitsPerComponent);
		return NULL;
	}

	/* Make sure and release colorspace before returning */
	CGColorSpaceRelease( colorSpace );

	return context;
}
#endif

#ifdef QNX
#include <img/img.h>
static img_lib_t  ilib = NULL;
int loadImage(textureTableIndexStruct_s* tti, char* fname)
{
	int ierr, iret;
	img_t img;
	if(!ilib) ierr = img_lib_attach( &ilib );
	img.format = IMG_FMT_PKLE_ARGB8888;  //GLES2 little endian 32bit - saw in sample code, no idea
	img.flags |= IMG_FORMAT;
	ierr= img_load_file(ilib, fname, NULL, &img);
	iret = 0;
	if(ierr == NULL)
	{

		//deep copy data so browser owns it (and does its FREE_IF_NZ) and we can delete our copy here and forget about it
		tti->x = img.w;
		tti->y = img.h;
		tti->frames = 1;
		tti->texdata = img.access.direct.data;
		if(!tti->texdata)
		   printf("ouch in gdiplus image loader L140 - no image data\n");
		else
		{
			int flipvertically = 1;
			if(flipvertically){
				int i,j,ii,rowcount;
				unsigned char *sourcerow, *destrow;
				unsigned char * blob;
				rowcount = tti->x * 4;
				blob = MALLOCV(img.h * rowcount);
				for(i=0;i<img.h;i++) {
					ii = tti->y - 1 - i;
					sourcerow = &tti->texdata[i*rowcount];
					destrow = &blob[ii*rowcount];
					memcpy(destrow,sourcerow,rowcount);
				}
				tti->texdata = blob;
				//try johns next time: tti->texdata = flipImageVertically(myFile->fileData, myFile->imageHeight, myFile->imageWidth); 

			}
		}
		tti->hasAlpha = 1; //img.transparency; //Gdiplus::IsAlphaPixelFormat(bitmap->GetPixelFormat())?1:0;
		//printf("fname=%s alpha=%ld\n",fname,tti->hasAlpha);
		iret = 1;
	}
	return iret;
}

#endif

char* download_file(char* filename);
void close_openned_file(openned_file_t *file);
int load_file_blob(const char *filename, char **blob, int *len);

#if defined(ANDROIDNDK)
#define HAVE_LIBJPEG_H 1
#ifdef HAVE_LIBJPEG_H
#include <jpeglib.h>
#include <setjmp.h>
struct my_error_mgr {
	struct jpeg_error_mgr pub;    /* "public" fields */
	jmp_buf setjmp_buffer; /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
* Here's the routine that will replace the standard error_exit method:
*/

METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	/* JAS (*cinfo->err->output_message) (cinfo); */

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}
#define ERROR -1
#define NOT_JPEGT -2
#define JPEG_SUCCESS 0

static int loadImageTexture_jpeg(textureTableIndexStruct_s* this_tex, char *filename) {
	FILE *infile;
	//char *filename;
	GLuint texture_num;
	unsigned char *image_data = 0;


	/* jpeg variables */
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JDIMENSION nrows;
	JSAMPROW row = 0;
	JSAMPROW rowptr[1];
	unsigned rowcount, columncount;
	int dp;

	int tempInt;


	if ((infile = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return ERROR;
	}

	/* is it a jpeg file */

	/* Select recommended processing options for quick-and-dirty output. */
	//cinfo.two_pass_quantize = FALSE;
	//cinfo.dither_mode = JDITHER_ORDERED;
	//cinfo.desired_number_of_colors = 216;
	//cinfo.dct_method = JDCT_FASTEST;
	//cinfo.do_fancy_upsampling = FALSE;

	/* call my error handler if there is an error */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		/* if we are here, we have a JPEG error */
		ConsoleMessage("FreeWRL Image problem - could not read %s\n", filename);
		jpeg_destroy_compress((j_compress_ptr)&cinfo);
		fclose(infile);
		return ERROR;
	}


	jpeg_create_decompress(&cinfo);

	/* Specify data source for decompression */
	jpeg_stdio_src(&cinfo, infile);

	/* Read file header, set default decompression parameters */
	/* (void) jpeg_read_header(&cinfo, TRUE); */
	// https://www4.cs.fau.de/Services/Doc/graphics/doc/jpeg/libjpeg.html
	tempInt = jpeg_read_header(&cinfo, TRUE);


	/* Start decompressor */
	(void)jpeg_start_decompress(&cinfo);



	row = (JSAMPLE*)MALLOCV(cinfo.output_width * sizeof(JSAMPLE)*cinfo.output_components);
	rowptr[0] = row;
	image_data = (unsigned char *)MALLOCV(cinfo.output_width * sizeof(JSAMPLE) * cinfo.output_height * cinfo.output_components);
	/* Process data */
	for (rowcount = 0; rowcount < cinfo.output_height; rowcount++) {
		nrows = jpeg_read_scanlines(&cinfo, rowptr, 1);
		/* yield for a bit */
		sched_yield();


		for (columncount = 0; columncount < cinfo.output_width; columncount++) {
			for (dp = 0; dp<cinfo.output_components; dp++) {
				image_data[(cinfo.output_height - rowcount - 1)
					*cinfo.output_width*cinfo.output_components
					+ columncount* cinfo.output_components + dp]
					= row[columncount*cinfo.output_components + dp];
			}
		}
	}

	int iret = JPEG_SUCCESS;
	if (jpeg_finish_decompress(&cinfo) != TRUE) {
		printf("warning: jpeg_finish_decompress error\n");
		//releaseTexture(loadThisTexture->scenegraphNode);
		iret = ERROR;
	}


	//store_tex_info(loadThisTexture,
	//	cinfo.output_components,
	//	(int)cinfo.output_width,
	//	(int)cinfo.output_height, image_data, cinfo.output_components == 4);
	fclose(infile);

	this_tex->x = (int)cinfo.output_width;
	this_tex->y = (int)cinfo.output_height;
	this_tex->hasAlpha = 0; //jpeg doesn't have alpha?
	
	//int bpp = this_tex->hasAlpha ? 4 :  3; //bytes per pixel
	int bpp = cinfo.output_components; //4
	//char *dataflipped = flipImageVerticallyB(image_data, this_tex->y, this_tex->x, bpp);
	char *data4bpp = expandto4bpp(image_data,this_tex->y,this_tex->x,bpp);
	//free(image_data);
	this_tex->frames = 1;
	this_tex->texdata = data4bpp;
	FREE_IF_NZ(image_data);
	this_tex->filename = filename;

	jpeg_destroy_decompress(&cinfo);
	FREE_IF_NZ(row);

	return JPEG_SUCCESS;
}



#endif //HAVE_LIBJPEG_H

#define HAVE_LIBPNG_H 1
#ifdef HAVE_LIBPNG_H
#include <png.h>

#define ERROR -1
#define NOT_PNG -2
#define PNG_SUCCESS 0
// http://www.learnopengles.com/loading-a-png-into-memory-and-displaying-it-as-a-texture-with-opengl-es-2-using-almost-the-same-code-on-ios-android-and-emscripten/
typedef struct {
	const png_byte* data;
	const png_size_t size;
} DataHandle;

typedef struct {
	const DataHandle data;
	png_size_t offset;
} ReadDataHandle;

typedef struct {
	const png_uint_32 width;
	const png_uint_32 height;
	const int color_type;
} PngInfo;
static GLenum get_gl_color_format(const int png_color_format) {
	assert(png_color_format == PNG_COLOR_TYPE_GRAY
		|| png_color_format == PNG_COLOR_TYPE_RGB_ALPHA
		|| png_color_format == PNG_COLOR_TYPE_GRAY_ALPHA);

	switch (png_color_format) {
	case PNG_COLOR_TYPE_GRAY:
		return GL_LUMINANCE;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		return GL_RGBA;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		return GL_LUMINANCE_ALPHA;
	}

	return 0;
}

static PngInfo read_and_update_info(const png_structp png_ptr, const png_infop info_ptr)
{
	png_uint_32 width, height;
	int bit_depth, color_type;

	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(
		png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	// Convert transparency to full alpha
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	// Convert grayscale, if needed.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	// Convert paletted images, if needed.
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	// Add alpha channel, if there is none.
	// Rationale: GL_RGBA is faster than GL_RGB on many GPUs)
	if (color_type == PNG_COLOR_TYPE_PALETTE || color_type == PNG_COLOR_TYPE_RGB)
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

	// Ensure 8-bit packing
	if (bit_depth < 8)
		png_set_packing(png_ptr);
	else if (bit_depth == 16)
		png_set_scale_16(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	// Read the new color type after updates have been made.
	color_type = png_get_color_type(png_ptr, info_ptr);

	return (PngInfo) { width, height, color_type };
}
static DataHandle read_entire_png_image(
	const png_structp png_ptr,
	const png_infop info_ptr,
	const png_uint_32 height)
{
	const png_size_t row_size = png_get_rowbytes(png_ptr, info_ptr);
	const int data_length = row_size * height;
	assert(row_size > 0);

	png_byte* raw_image = malloc(data_length);
	assert(raw_image != NULL);

	png_byte* row_ptrs[height];

	png_uint_32 i;
	for (i = 0; i < height; i++) {
		row_ptrs[i] = raw_image + i * row_size;
	}

	png_read_image(png_ptr, &row_ptrs[0]);

	return (DataHandle) { raw_image, data_length };
}
static void read_png_data_callback(
	png_structp png_ptr, png_byte* raw_data, png_size_t read_length) {
	ReadDataHandle* handle = png_get_io_ptr(png_ptr);
	const png_byte* png_src = handle->data.data + handle->offset;

	memcpy(raw_data, png_src, read_length);
	handle->offset += read_length;
}
enum {
TACTIC_FROM_FILE = 1,
TACTIC_FROM_BLOB = 2,
};
static int loadImageTexture_png(textureTableIndexStruct_s* this_tex, char *filename) {
	FILE *fp;
	//char *filename;
	GLuint texture_num;
	unsigned char *image_data = 0;

	/* png reading variables */
	int rc;
	unsigned long image_width = 0;
	unsigned long image_height = 0;
	unsigned long image_rowbytes = 0;
	int image_channels = 0;
	double display_exponent = 0.0;
	char * png_data;
	int png_data_size;
	int is_png;
	int tactic;
	int tempInt;
	tactic= TACTIC_FROM_BLOB;


	png_structp png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	
	//from memory (if from file there s png_set_io
	if(tactic == TACTIC_FROM_FILE){
		char header[8];
		fp = fopen(filename,"rb");
		fread(header, 1, 8, fp);
		is_png = !png_sig_cmp(png_data, 0, 8);
		fclose(fp);
		if (!is_png)
		{
			return (NOT_PNG);
		}
		fp = fopen(filename,"rb");
		png_init_io(png_ptr, fp);
	} else if(tactic == TACTIC_FROM_BLOB){
		if (!load_file_blob(filename, &png_data, &png_data_size)) {
			return ERROR;
		}
		is_png = !png_sig_cmp(png_data, 0, 8);
		if (!is_png)
		{
			return (NOT_PNG);
		}
		ReadDataHandle png_data_handle = (ReadDataHandle) { {png_data, png_data_size}, 0 };
		png_set_read_fn(png_ptr, &png_data_handle, read_png_data_callback);
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,
			(png_infopp)NULL);
		if (tactic == TACTIC_FROM_FILE) fclose(fp);
		return (ERROR);
	}

	//png_read_png(png_ptr, info_ptr, 0, NULL);

	//image_data = readpng_get_image(display_exponent, &image_channels,
	//	&image_rowbytes);
	const PngInfo png_info = read_and_update_info(png_ptr, info_ptr);
	const DataHandle raw_image = read_entire_png_image(
		png_ptr, info_ptr, png_info.height);

	png_read_end(png_ptr, info_ptr);
	this_tex->x = png_info.width;
	this_tex->y = png_info.height;
	this_tex->hasAlpha = png_info.color_type == GL_RGBA || png_info.color_type == GL_LUMINANCE_ALPHA;
	//int bpp = this_tex->hasAlpha ? 4 :  3; //bytes per pixel
	int bpp = 4;
	char *dataflipped = flipImageVerticallyB(raw_image.data, this_tex->y, this_tex->x, bpp);
	free(raw_image.data);
	this_tex->frames = 1;
	this_tex->texdata = dataflipped;
	this_tex->filename = filename;
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);


	//readpng_cleanup(FALSE);

	if (tactic == TACTIC_FROM_FILE) fclose(fp);
	return PNG_SUCCESS;
}

#endif //HAVE_LIBPNG_H

#define HAVE_LIBGIF_H 1
#ifdef HAVE_LIBGIF_H
#include <gif_lib.h>  //loads stdbool.h
// http://cd.textfiles.com/amigaplus/lesercd16/Tools/Development/ming0_2/util/gif2dbl.c

int getTransparentColor(GifFileType * file)
{
	//get the color index of transparent 
	int i;
	ExtensionBlock * ext = file->SavedImages[0].ExtensionBlocks;

	for (i = 0; i < file->SavedImages[0].ExtensionBlockCount; i++, ext++) {

		if (ext->Function == GRAPHICS_EXT_FUNC_CODE) {
			if (ext->Bytes[0] & 1)	// there is a transparent color 
				return ext->Bytes[3];	// here it is
		}
	}

	return -1;
}


//#define GIF_ERROR -1
#define NOT_GIF -2
#define GIF_SUCCESS 0

static int loadImageTexture_gif(textureTableIndexStruct_s* this_tex, char *filename) {
// http://giflib.sourceforge.net/gif_lib.html#idm46571690371280

	int ErrorCode, alpha, iret;
	int
	InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
	InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
	ColorMapObject *ColorMap;
	GifRowType *ScreenBuffer;
	int Error;

	GifFileType *GifFile = DGifOpenFileName(filename, &ErrorCode);
	if(!GifFile){
		return GIF_ERROR;
	}
	if (GifFile->SHeight == 0 || GifFile->SWidth == 0) {
		return GIF_ERROR;
	}

	ErrorCode = DGifSlurp(GifFile);
	if(ErrorCode != GIF_OK)
		return GIF_ERROR;
	alpha = getTransparentColor(GifFile);
	iret = GIF_ERROR;
	ColorMap = (GifFile->Image.ColorMap
		? GifFile->Image.ColorMap
		: GifFile->SColorMap);
	if (ColorMap == NULL) {
		return GIF_ERROR;
	}

	if(GifFile->ImageCount){
		unsigned char *pixel;
		int i,j,ipix,icolor;
		
		unsigned char * raw = GifFile->SavedImages[0].RasterBits;
		int width = GifFile->SavedImages[0].ImageDesc.Width;
		int height = GifFile->SavedImages[0].ImageDesc.Height;
		GifColorType *Colors = ColorMap->Colors;
		unsigned char *rgba = MALLOCV(width * height * 4);
		GifColorType *color;

		for(i=0;i<height;i++){
			for(j=0;j<width;j++){
				ipix = i*width + j;
				pixel = &rgba[ipix*4];
				icolor = raw[ipix];
				color = &Colors[icolor];
				pixel[0] = color->Red;
				pixel[1] = color->Green;
				pixel[2] = color->Blue;
				pixel[3] = icolor == alpha ? 0 : 255;
			}
		}
		this_tex->x = width;
		this_tex->y = height;
		this_tex->hasAlpha = 1; //jpeg doesn't have alpha?
		this_tex->frames = 1;
		int bpp = 4;
		char *dataflipped = flipImageVerticallyB(rgba, this_tex->y, this_tex->x, bpp);
		FREE_IF_NZ(rgba);
		this_tex->texdata = dataflipped;
		this_tex->filename = filename;
		iret = GIF_SUCCESS;
	}
	return iret;
	
}
#define bool int //set back to freewrl convention
#endif //HAVE_LIBGIF_H


static void __reallyloadImageTexture(textureTableIndexStruct_s* this_tex, char *filename) {
//filenames coming in can be temp file names - scrambled
//there are 3 ways to tell in the backend what type of image file:
//a) .xxx original filename suffix
//b) MIME type 
//c) file signature https://en.wikipedia.org/wiki/List_of_file_signatures
// right now we aren't passing in the .xxx or mime or signature bytes
// except through the file conents we can get the signature
	char header[20];
	FILE* fp = fopen(filename,"rb");
	fread(header,20,1,fp);
	fclose(fp);

#ifdef HAVE_LIBPNG_H
	if(!strncmp(&header[1],"PNG",3))
		loadImageTexture_png(this_tex, filename);
#endif
#ifdef HAVE_LIBJPEG_H
	if(!strncmp(header,"ÿØÿ",3))
		loadImageTexture_jpeg(this_tex, filename);
#endif
#ifdef HAVE_LIBGIF_H
	if(!strncmp(header,"GIF",3))
		loadImageTexture_gif(this_tex, filename);
#endif 
	return;
}


#endif // ANDROIDNDK




/**
 *   texture_load_from_file: a local filename has been found / downloaded,
 *                           load it now.
 */
 
bool texture_load_from_file(textureTableIndexStruct_s* this_tex, char *filename)
{

/* Android, put it here... */

#if defined(ANDROIDNDK)

	__reallyloadImageTexture(this_tex, filename);

	/*
	// if we got null for data, lets assume that there was not a file there
	if (myFile->fileData == NULL) {
		result = FALSE;
	}
	else {
		this_tex->texdata = flipImageVertically(myFile->fileData, myFile->imageHeight, myFile->imageWidth);

		this_tex->filename = filename;
		this_tex->hasAlpha = myFile->imageAlpha;
		this_tex->frames = 1;
		this_tex->x = myFile->imageWidth;
		this_tex->y = myFile->imageHeight;
		result = TRUE;
	}
	//close_openned_file(myFile);
	FREE_IF_NZ(myFile);
	*/
	return this_tex->frames;

#endif //ANDROIDNDK



#if defined(_ANDROID)
	unsigned char *image = NULL;
	unsigned char *imagePtr;
	int i;

	openned_file_t *myFile = load_file (filename);
    bool result = FALSE;
	/* if we got null for data, lets assume that there was not a file there */
	if (myFile->fileData == NULL) {
		result = FALSE;
	} else {
		//this_tex->texdata = MALLOC(unsigned char*,myFile->fileDataSize);
		//memcpy(this_tex->texdata,myFile->fileData,myFile->fileDataSize);
/*
{char me[200]; sprintf(me,"texture_load, %d * %d * 4 = %d, is it %d??",myFile->imageHeight, myFile->imageWidth,
			myFile->imageHeight*myFile->imageWidth*4, myFile->fileDataSize);
ConsoleMessage(me);}
*/

		this_tex->texdata = flipImageVertically(myFile->fileData, myFile->imageHeight, myFile->imageWidth); 

		this_tex->filename = filename;
		this_tex->hasAlpha = myFile->imageAlpha;
		this_tex->frames = 1;
		this_tex->x = myFile->imageWidth;
		this_tex->y = myFile->imageHeight;
		result = TRUE;
	}
#ifdef FRONTEND_GETS_FILES
	close_openned_file(myFile);
	FREE_IF_NZ(myFile);
#endif
    return result;

#endif //ANDROID



/* WINDOWS */
#if defined (_MSC_VER) 
	char *fname;
	int ret;

	fname = STRDUP(filename);
	ret = loadImage(this_tex, fname);
    if (!ret) {
		ERROR_MSG("load_texture_from_file: failed to load image: %s\n", fname);
	}else{
#ifdef GL_ES_VERSION_2_0
			//swap red and blue
			//search for GL_RGBA in textures.c
			int x,y,i,j,k,m;
			unsigned char R,B,*data;
			x = this_tex->x;
			y = this_tex->y;
			data = this_tex->texdata;
			for(i=0,k=0;i<y;i++)
			{
				for(j=0;j<x;j++,k++)
				{
					m=k*4;
					R = data[m];
					B = data[m+2];
					data[m] = B;
					data[m+2] = R;
				}
			}
#endif
	}
	FREE(fname);
	return (ret != 0);

#endif


/* LINUX */
#if !defined (_MSC_VER) && !defined (TARGET_AQUA) && !defined(_ANDROID) && !defined(ANDROIDNDK)
	Imlib_Image image;
	Imlib_Load_Error error_return;

	//image = imlib_load_image_immediately(filename);
	//image = imlib_load_image(filename);
	image = imlib_load_image_with_error_return(filename,&error_return);

	if (!image) {
		char *es = NULL;
		switch(error_return){
			case IMLIB_LOAD_ERROR_NONE: es = "IMLIB_LOAD_ERROR_NONE";break;
			case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST: es = "IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST";break;
			case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY: es = "IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY";break;
			case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ: es = "IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ";break;
			case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT: es = "IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT";break;
			case IMLIB_LOAD_ERROR_PATH_TOO_LONG: es = "IMLIB_LOAD_ERROR_PATH_TOO_LONG";break;
			case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT: es = "IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT";break;
			case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY: es = "IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY";break;
			case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE: es = "IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE";break;
			case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS: es = "IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS";break;
			case IMLIB_LOAD_ERROR_OUT_OF_MEMORY: es = "IMLIB_LOAD_ERROR_OUT_OF_MEMORY";break;
			case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS: es = "IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS";break;
			case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE: es = "IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE";break;
			case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE: es = "IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE";break;
			case IMLIB_LOAD_ERROR_UNKNOWN:
			default:
			es = "IMLIB_LOAD_ERROR_UNKNOWN";break;
		}
		ERROR_MSG("imlib load error = %d %s\n",error_return,es);
		ERROR_MSG("load_texture_from_file: failed to load image: %s\n", filename);
		return FALSE;
	}
	DEBUG_TEX("load_texture_from_file: Imlib2 succeeded to load image: %s\n", filename);

	imlib_context_set_image(image);
	imlib_image_flip_vertical(); /* FIXME: do we really need this ? */

	/* store actual filename, status, ... */
	this_tex->filename = filename;
	this_tex->hasAlpha = (imlib_image_has_alpha() == 1);
	this_tex->frames = 1;
	this_tex->x = imlib_image_get_width();
	this_tex->y = imlib_image_get_height();

	this_tex->texdata = (unsigned char *) imlib_image_get_data_for_reading_only(); 
	return TRUE;
#endif

/* OSX */
#if defined (TARGET_AQUA)

	CGImageRef 	image;


	int 		image_width;
	int 		image_height;

#ifndef FRONTEND_GETS_FILES
	CFStringRef	path;
    CFURLRef 	url;
#endif
    
	CGContextRef 	cgctx;

	unsigned char *	data;
	int		hasAlpha;

	CGImageSourceRef 	sourceRef;

	/* initialization */
	image = NULL;
	hasAlpha = FALSE;

#ifdef FRONTEND_GETS_FILES
	openned_file_t *myFile = load_file (filename);
	/* printf ("got file from load_file, openned_file_t is %p %d\n", myFile->fileData, myFile->fileDataSize); */


	/* if we got null for data, lets assume that there was not a file there */
	if (myFile->fileData == NULL) {
		sourceRef = NULL;
		image = NULL;
	} else {
		//CFDataRef localData = CFDataCreate(NULL,(const UInt8 *)myFile->fileData,myFile->fileDataSize);
		CFDataRef localData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)myFile->fileData,myFile->fileDataSize, kCFAllocatorNull);
		sourceRef = CGImageSourceCreateWithData(localData,NULL);
		CFRelease(localData);
	}

	/* step 2, if the data exists, was it a file for us? */
	if (sourceRef != NULL) {
		image = CGImageSourceCreateImageAtIndex(sourceRef, 0, NULL);
		CFRelease (sourceRef);
	}


#else /* FRONTEND_GETS_FILES */

	path = CFStringCreateWithCString(NULL, filename, kCFStringEncodingUTF8);
	url = CFURLCreateWithFileSystemPath (NULL, path, kCFURLPOSIXPathStyle, 0);

	/* ok, we can define USE_CG_DATA_PROVIDERS or TRY_QUICKTIME...*/

	/* I dont know whether to use quicktime or not... Probably not... as the other ways using core 
		graphics seems to be ok. Anyway, I left this code in here, as maybe it might be of use for mpegs
	*/

	sourceRef = CGImageSourceCreateWithURL(url,NULL);
	if (sourceRef != NULL) {
		image = CGImageSourceCreateImageAtIndex(sourceRef, 0, NULL);
		CFRelease (sourceRef);
	}

	CFRelease(url);
	CFRelease(path);

#endif /* FRONTEND_GETS_FILES */

	/* We were able to load in the image here... */
	if (image != NULL) {
		image_width = (int) CGImageGetWidth(image);
		image_height = (int) CGImageGetHeight(image);
	
		/* go through every possible return value and check alpha. 
			note, in testing, kCGImageAlphaLast and kCGImageAlphaNoneSkipLast
			are what got returned - which makes sense for BGRA textures */
		switch (CGImageGetAlphaInfo(image)) {
			case kCGImageAlphaNone: hasAlpha = FALSE; break;
			case kCGImageAlphaPremultipliedLast: hasAlpha = TRUE; break;
			case kCGImageAlphaPremultipliedFirst: hasAlpha = TRUE; break;
			case kCGImageAlphaLast: hasAlpha = TRUE; break;
			case kCGImageAlphaFirst: hasAlpha = TRUE; break;
			case kCGImageAlphaNoneSkipLast: hasAlpha = FALSE; break;
			case kCGImageAlphaNoneSkipFirst: hasAlpha = FALSE; break;
			default: hasAlpha = FALSE; /* should never get here */
		}
	
		#ifdef TEXVERBOSE
		printf ("\nLoadTexture %s\n",filename);
		printf ("CGImageGetAlphaInfo(image) returns %x\n",CGImageGetAlphaInfo(image));
		printf ("   kCGImageAlphaNone %x\n",   kCGImageAlphaNone);
		printf ("   kCGImageAlphaPremultipliedLast %x\n",   kCGImageAlphaPremultipliedLast);
		printf ("   kCGImageAlphaPremultipliedFirst %x\n",   kCGImageAlphaPremultipliedFirst);
		printf ("   kCGImageAlphaLast %x\n",   kCGImageAlphaLast);
		printf ("   kCGImageAlphaFirst %x\n",   kCGImageAlphaFirst);
		printf ("   kCGImageAlphaNoneSkipLast %x\n",   kCGImageAlphaNoneSkipLast);
		printf ("   kCGImageAlphaNoneSkipFirst %x\n",   kCGImageAlphaNoneSkipFirst);
	
		if (hasAlpha) printf ("Image has Alpha channel\n"); else printf ("image - no alpha channel \n");
	
		printf ("raw image, AlphaInfo %x\n",(int) CGImageGetAlphaInfo(image));
		printf ("raw image, BitmapInfo %x\n",(int) CGImageGetBitmapInfo(image));
		printf ("raw image, BitsPerComponent %d\n",(int) CGImageGetBitsPerComponent(image));
		printf ("raw image, BitsPerPixel %d\n",(int) CGImageGetBitsPerPixel(image));
		printf ("raw image, BytesPerRow %d\n",(int) CGImageGetBytesPerRow(image));
		printf ("raw image, ImageHeight %d\n",(int) CGImageGetHeight(image));
		printf ("raw image, ImageWidth %d\n",(int) CGImageGetWidth(image));
		#endif
		
	
	
		/* now, lets "draw" this so that we get the exact bit values */
		cgctx = CreateARGBBitmapContext(image);
	
		 
		#ifdef TEXVERBOSE
		printf ("GetAlphaInfo %x\n",(int) CGBitmapContextGetAlphaInfo(cgctx));
		printf ("GetBitmapInfo %x\n",(int) CGBitmapContextGetBitmapInfo(cgctx));
		printf ("GetBitsPerComponent %d\n",(int) CGBitmapContextGetBitsPerComponent(cgctx));
		printf ("GetBitsPerPixel %d\n",(int) CGBitmapContextGetBitsPerPixel(cgctx));
		printf ("GetBytesPerRow %d\n",(int) CGBitmapContextGetBytesPerRow(cgctx));
		printf ("GetHeight %d\n",(int) CGBitmapContextGetHeight(cgctx));
		printf ("GetWidth %d\n",(int) CGBitmapContextGetWidth(cgctx));
		#endif
		
		data = (unsigned char *)CGBitmapContextGetData(cgctx);
	
/*
		#ifdef TEXVERBOSE
		if (CGBitmapContextGetWidth(cgctx) < 301) {
			int i;
	
			printf ("dumping image\n");
			for (i=0; i<CGBitmapContextGetBytesPerRow(cgctx)*CGBitmapContextGetHeight(cgctx); i++) {
				printf ("index:%d data:%2x\n ",i,data[i]);
			}
			printf ("\n");
		}
		#endif
*/
	
		/* is there possibly an error here, like a file that is not a texture? */
		if (CGImageGetBitsPerPixel(image) == 0) {
			ConsoleMessage ("texture file invalid: %s",filename);
		}
	
		if (data != NULL) {
			this_tex->filename = filename;
			this_tex->hasAlpha = hasAlpha;
			this_tex->frames = 1;
			this_tex->x = image_width;
			this_tex->y = image_height;
            
            int bitmapBytesPerRow = (image_width * 4);
            size_t bitmapByteCount = (bitmapBytesPerRow * image_height);
            
            unsigned char *	texdata = MALLOC(unsigned char*, bitmapByteCount);
            memcpy(texdata, data, bitmapByteCount);
            
			this_tex->texdata = texdata;
		}
	
		CGContextRelease(cgctx);
		CGImageRelease(image);
#ifdef FRONTEND_GETS_FILES
        close_openned_file(myFile);
#endif
		return TRUE;
	} else {
#ifdef FRONTEND_GETS_FILES
        close_openned_file(myFile);
        FREE_IF_NZ(myFile);
#endif
		/* is this, possibly, a dds file for an ImageCubeMap? */
		return textureIsDDS(this_tex, filename);
	}
#ifdef FRONTEND_GETS_FILES
        close_openned_file(myFile);
        FREE_IF_NZ(myFile);
#endif
    
#endif
	return FALSE;
}

/**
 *   texture_process_entry: process a texture table entry
 *
 * find the file, either locally or within the Browser. Note that
 * this is almost identical to the one for Inlines, but running
 * in different threads 
 */
static bool texture_process_entry(textureTableIndexStruct_s *entry)
{
	resource_item_t *res;
	resource_type_t restype;
	struct Multi_String *url;
	resource_item_t *parentPath = NULL;

	DEBUG_TEX("textureThread - working on %p (%s)\n"
		  "which is node %p, nodeType %d status %s, opengltex %u, and frames %d\n",
		  entry, entry->filename, entry->scenegraphNode, entry->nodeType, 
		  texst(entry->status), entry->OpenGLTexture, 
		  entry->frames);
	
	entry->status = TEX_LOADING;
	url = NULL;
	res = NULL;

    /* did this node just disappear? */
    if (!checkNode(entry->scenegraphNode,__FILE__,__LINE__)) {
        ConsoleMessage ("node for texture just deleted...\n");
        return FALSE;
    }
    
    
	switch (entry->nodeType) {

	case NODE_PixelTexture:
		texture_load_from_pixelTexture(entry,(struct X3D_PixelTexture *)entry->scenegraphNode);
		//sets TEX_NEEDSBINDING internally
		return TRUE;
		break;

	case NODE_ImageTexture:
		url = & (((struct X3D_ImageTexture *)entry->scenegraphNode)->url);
		parentPath = (resource_item_t *)(((struct X3D_ImageTexture *)entry->scenegraphNode)->_parentResource);
		restype = resm_image;
		break;

	case NODE_MovieTexture:
#ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES
		url = & (((struct X3D_MovieTexture *)entry->scenegraphNode)->url);
		parentPath = (resource_item_t *)(((struct X3D_MovieTexture *)entry->scenegraphNode)->_parentResource);
		entry->status = TEX_NEEDSBINDING; //as with pixeltexture, just do the move_to_opengl part, we load from file elsewhere
		restype = resm_movie;
		return TRUE;  //like pixeltexture - assume the pixels are delivered magically, not from file, so just return
		break;
#else  // HAVE_TO_REIMPLEMENT_MOVIETEXTURES
		//texture_load_from_MovieTexture(entry);
		entry->status = TEX_NOTFOUND; //NOT_IMPLEMENTED
		return TRUE;
#endif /* HAVE_TO_REIMPLEMENT_MOVIETEXTURES */

	case NODE_ImageCubeMapTexture:
		url = & (((struct X3D_ImageCubeMapTexture *)entry->scenegraphNode)->url);
		parentPath = (resource_item_t *)(((struct X3D_ImageCubeMapTexture *)entry->scenegraphNode)->_parentResource);
		restype = resm_image;
		break;

	default: 
		printf ("invalid nodetype given to loadTexture, %s is not valid\n",stringNodeType(entry->nodeType));
	}
	if(!url){
		entry->status = TEX_NOTFOUND;
		return FALSE;
	}

	//TEX_LOADING
	res = resource_create_multi(url);
	res->type=rest_multi;
	res->media_type = restype; //resm_image; /* quick hack */
	resource_identify(parentPath, res);
	res->whereToPlaceData = entry;
	res->textureNumber = entry->textureNumber;
	resitem_enqueue(ml_new(res));
	return TRUE;

}
/*
parsing thread --> texture_loading_thread hand-off
GOAL: texture thread blocks when no textures requested. (rather than sleep(500) and for(;;) )
IT IS AN ERROR TO CALL (condition signal) before calling (condition wait). 
So you might have a global variable bool waiting = false.
1. The threads start, list=null, waiting=false
2. The texture thread loops to lock_mutex line, checks if list=null, 
   if so it sets waiting = true, and sets condition wait, and blocks, 
   waiting for the main thread to give it some texure names
3. The parsing/main thread goes to schedule a texture. It mutex locks, 
   list= add new item. it checks if textureloader is waiting, 
   if so signals condition (which locks momentarily blocks while 
   other thread does something to the list) then unlock mutex.
4. The texture thread gets a signal its waiting on. it copies the list and sets it null, 
   sets waiting =false, and unlocks and does its loading work 
   (on its copy of the list), and goes back around to 2.

*/

/**
 *   texture_process_list_item: process a texture_list item
 */
static void texture_process_list_item(s_list_t *item)
{
	bool remove_it = FALSE;
	textureTableIndexStruct_s *entry;
	ppLoadTextures p = (ppLoadTextures)gglobal()->LoadTextures.prv;

	if (!item || !item->elem)
		return;
	
	entry = ml_elem(item);
	
	DEBUG_TEX("texture_process_list: %s\n", entry->filename);
	
	/* FIXME: it seems there is no case in which we not want to remote it ... */

	switch (entry->status) {
	
	/* JAS - put in the TEX_LOADING flag here - it helps on OSX */
	case TEX_LOADING:
		if (texture_process_entry(entry)) {
			remove_it = TRUE;
		}else{
			remove_it = TRUE; //still remove it 
			// url doesn't exist (or none of multi-url exist)
			// no point in trying again, 
			// you'll just get the same result in a vicious cycle
		}
		break;
	case TEX_READ:
		entry->status = TEX_NEEDSBINDING;
		remove_it = TRUE;
		break;		
	default:
		//DEBUG_MSG("Could not process texture entry: %s\n", entry->filename);
		remove_it = TRUE;
		break;
	}
		
	if (remove_it) {
		/* free the parsed resource and list item */
		//p->texture_list = ml_delete_self(p->texture_list, item);
		ml_free(item);
	}
}

void threadsafe_enqueue_item_signal(s_list_t *item, s_list_t** queue, pthread_mutex_t* queue_lock, pthread_cond_t *queue_nonzero);
s_list_t* threadsafe_dequeue_item_wait(s_list_t** queue, pthread_mutex_t *queue_lock, pthread_cond_t *queue_nonzero, int* wait);

void texitem_enqueue(s_list_t *item){
	ppLoadTextures p;
	ttglobal tg = gglobal();
	p = (ppLoadTextures)gglobal()->LoadTextures.prv;

	threadsafe_enqueue_item_signal(item, &p->texture_request_list, &tg->threads.mutex_texture_list, &tg->threads.texture_list_condition);
}
s_list_t *texitem_dequeue(){
	ppLoadTextures p;
	ttglobal tg = gglobal();
	p = (ppLoadTextures)gglobal()->LoadTextures.prv;

	return threadsafe_dequeue_item_wait(&p->texture_request_list, &tg->threads.mutex_texture_list, &tg->threads.texture_list_condition, &tg->threads.TextureThreadWaiting);
}
//we want the void* addresses of the following, so the int value doesn't matter
static const int tex_command_exit;

void texitem_queue_exit(){
	texitem_enqueue(ml_new(&tex_command_exit));
}

void send_texture_to_loader(textureTableIndexStruct_s *entry)
{
	texitem_enqueue(ml_new(entry));
}
textureTableIndexStruct_s *getTableIndex(int i);
void process_res_texitem(resource_item_t *res){
	//resitem after download+load -> texture thread
	textureTableIndexStruct_s *entry;
	int textureNumber;
	textureNumber = res->textureNumber;
	//check in case texture has been deleted due to inline unloading during image download
	//entry = res->whereToPlaceData;
	entry = getTableIndex(textureNumber);
	if(entry)
		texitem_enqueue(ml_new(entry));
}

/**
 *   _textureThread: work on textures, until the end of time.
 */


#if !defined(HAVE_PTHREAD_CANCEL)
void Texture_thread_exit_handler(int sig)
{ 
    ConsoleMessage("Texture_thread_exit_handler: No pTheadCancel - textureThread exiting - maybe should cleanup? Should be done but need to check some rainy day");
    pthread_exit(0);
}
#endif //HAVE_PTHREAD_CANCEL



void _textureThread(void *globalcontext)
{
	ttglobal tg = (ttglobal)globalcontext;
	tg->threads.loadThread = pthread_self();
	fwl_setCurrentHandle(tg, __FILE__, __LINE__);
	//ENTER_THREAD("texture loading");
	{
		ppLoadTextures p;
		//ttglobal tg = gglobal();
		p = (ppLoadTextures)tg->LoadTextures.prv;

		//tg->LoadTextures.TextureThreadInitialized = TRUE;
		tg->threads.TextureThreadRunning = TRUE;

		/* we wait forever for the data signal to be sent */
		for (;;) {
			void* elem;
			s_list_t *item = texitem_dequeue();
			elem = ml_elem(item);
			if (elem == &tex_command_exit){
				FREE_IF_NZ(item);
				break;
			}
			if (tg->threads.flushing){
				FREE_IF_NZ(item);
				continue;
			}
			p->TextureParsing = TRUE;
			texture_process_list_item(item);
			p->TextureParsing = FALSE;
		}
	}
	printf("Ending texture load thread gracefully\n");
	tg->threads.TextureThreadRunning = FALSE;

}
