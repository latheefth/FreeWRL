/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*
 * General Texture objects
 */


#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "Structs.h"
#include "headers.h"

//#include "jinclude.h"
#include "jpeglib.h"

void do_texture(depth,x,y,ptr,Sgl_rep_or_clamp, Tgl_rep_or_clamp,Image)
	int x,y,depth;
	GLint Sgl_rep_or_clamp;
	GLint Tgl_rep_or_clamp;
	GLint Image;
	unsigned char *ptr;


{

	int rx,ry,sx,sy;
	int ct;

	/* save this to determine whether we need to do material node
	  within appearance or not */
	last_texture_depth = depth;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	/* Image should be GL_LINEAR for pictures, GL_NEAREST for pixelTs */
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Image );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Image );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Sgl_rep_or_clamp);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Tgl_rep_or_clamp);

	if((depth) && x && y) {
		unsigned char *dest = ptr;
		rx = 1; sx = x;
		while(sx) {sx /= 2; rx *= 2;}
		if(rx/2 == x) {rx /= 2;}
		ry = 1; sy = y;
		while(sy) {sy /= 2; ry *= 2;}
		if(ry/2 == y) {ry /= 2;}
		if(rx != x || ry != y || rx > global_texSize || ry > global_texSize) {
			/* do we have texture limits??? */
			if (rx > global_texSize) rx = global_texSize;
			if (ry > global_texSize) ry = global_texSize;

			/* We have to scale */
			/* printf ("Do_Texture scaling from rx %d ry %d to x %d y %d\n",x,y,rx,ry);  */
			dest = malloc((depth) * rx * ry);
			gluScaleImage(
			     ((depth)==1 ? GL_LUMINANCE : 
			     ((depth)==2 ? GL_LUMINANCE_ALPHA : 
			     ((depth)==3 ? GL_RGB : 
			     GL_RGBA))),
			     x, y, GL_UNSIGNED_BYTE, ptr, rx, ry, 
			     GL_UNSIGNED_BYTE, dest);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, depth,  rx, ry, 0,
			     ((depth)==1 ? GL_LUMINANCE : 
			     ((depth)==2 ? GL_LUMINANCE_ALPHA : 
			     ((depth)==3 ? GL_RGB : 
			     GL_RGBA))),
			     GL_UNSIGNED_BYTE, dest);
		if(ptr != dest) free(dest);
	}
}


void bind_image (filename, texture_num, repeatS, repeatT, remove, isloaded) 
	char *filename;
	GLuint texture_num;
	int repeatS;
	int repeatT;
	int remove;
	int isloaded;
{
	FILE *infile;
	unsigned char *image_data = 0;

	/* png reading variables */
	int rc;
	int len;
	unsigned long image_width = 0;
	unsigned long image_height = 0;
	unsigned long image_rowbytes = 0;
	int image_channels = 0;
	double display_exponent = 0.0;

	/* jpeg variables */
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JDIMENSION nrows;
	JSAMPARRAY buffer;
	JDIMENSION buffer_height;
	int read_scanlines = 0;
	JSAMPROW row = 0;
	JSAMPROW rowptr[1];
	int rowcount, columncount, dp;

	/* assume a jpeg file, unless shown otherwise */
	int jpeg_file = 1;

	int fnamelen;


	/* have we already processed this one before? */
	if (isloaded == 1) {
		//printf ("bind_image, simply reusing %d\n",texture_num);
		glBindTexture (GL_TEXTURE_2D, texture_num);
		return;
	}


	/* determine whether it is a jpeg file, or a png file */
	fnamelen = strlen(filename);
	if (fnamelen >= 4) {
		char last4[4];
		last4[0] = tolower (filename[fnamelen-3]);
		last4[1] = tolower (filename[fnamelen-2]);
		last4[2] = tolower (filename[fnamelen-1]);
		last4[3] = 0;
		jpeg_file = (strcmp(last4,"png"));
	}
	



	/* read in the file from the local file system */	
	if (!(infile = fopen(filename, "rb"))) {
		printf("can not open ImageTexture file [%s]\n", filename);
	} else {
		
		if (jpeg_file) {
			/* see http://www.the-labs.com/JPEG/libjpeg.html for details */
			
			/* Select recommended processing options for quick-and-dirty output. */
			cinfo.two_pass_quantize = FALSE;
			cinfo.dither_mode = JDITHER_ORDERED;
			cinfo.desired_number_of_colors = 216;
			cinfo.dct_method = JDCT_FASTEST;
			cinfo.do_fancy_upsampling = FALSE;

			cinfo.err = jpeg_std_error(&jerr);
			jpeg_create_decompress(&cinfo);

			/* Specify data source for decompression */
			jpeg_stdio_src(&cinfo, infile);

			/* Read file header, set default decompression parameters */
			(void) jpeg_read_header(&cinfo, TRUE);

			/* Start decompressor */
			(void) jpeg_start_decompress(&cinfo);



			row = malloc(cinfo.output_width * sizeof(JSAMPLE)*cinfo.output_components);
			rowptr[0] = row;
			image_data = malloc(cinfo.output_width * sizeof (JSAMPLE) * cinfo.output_height * cinfo.output_components);
			/* Process data */
			for (rowcount = 0; rowcount < cinfo.output_height; rowcount++) {
				nrows = jpeg_read_scanlines(&cinfo, rowptr, 1);

				for (columncount = 0; columncount < cinfo.output_width; columncount++) {
					for(dp=0; dp<cinfo.output_components; dp++) {
						image_data[(cinfo.output_height-rowcount-1)
								*cinfo.output_width*cinfo.output_components
						       		+ columncount* cinfo.output_components	+dp]
							= row[columncount*cinfo.output_components + dp];
					}
				}
			}
				

			if (jpeg_finish_decompress(&cinfo) != TRUE)
				printf("warning: jpeg_finish_decompress error\n");
			jpeg_destroy_decompress(&cinfo);
			free(row);

			glBindTexture (GL_TEXTURE_2D, texture_num);
			do_texture (cinfo.output_components, cinfo.output_width,
				cinfo.output_height,image_data,
				((repeatS)) ? GL_REPEAT : GL_CLAMP, 
				((repeatT)) ? GL_REPEAT : GL_CLAMP,
				GL_LINEAR);

			free(image_data);
		} else {
			if ((rc = readpng_init(infile, &image_width, &image_height)) != 0) {
			switch (rc) {
				case 1:
					printf("[%s] is not a PNG file: incorrect signature\n", filename);
					break;
				case 2:
					printf("[%s] has bad IHDR (libpng longjmp)\n", filename);
					break;
				case 4:
					printf("insufficient memory\n");
					break;
				default:
					printf("unknown readpng_init() error\n");
					break;
				}
			} else {
				image_data = readpng_get_image(display_exponent, &image_channels,
						&image_rowbytes);
	
				glBindTexture (GL_TEXTURE_2D, texture_num);
				do_texture (image_channels, image_width, image_height,
					image_data,
					((repeatS)) ? GL_REPEAT : GL_CLAMP, 
					((repeatT)) ? GL_REPEAT : GL_CLAMP,
					GL_LINEAR);
			}
			readpng_cleanup (TRUE);
		}
		fclose (infile);
	}

	/* is this a temporary file? */
	if (remove == 1) {
		unlink (filename);
	}
}



