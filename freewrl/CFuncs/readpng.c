/*---------------------------------------------------------------------------

   rpng - simple PNG display program                              readpng.c

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2000 Greg Roelofs.  All rights reserved.

      This software is provided "as is," without warranty of any kind,
      express or implied.  In no event shall the author or contributors
      be held liable for any damages arising in any way from the use of
      this software.

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute
      it freely, subject to the following restrictions:

      1. Redistributions of source code must retain the above copyright
         notice, disclaimer, and this list of conditions.
      2. Redistributions in binary form must reproduce the above copyright
         notice, disclaimer, and this list of conditions in the documenta-
         tion and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this
         software must display the following acknowledgment:

            This product includes software developed by Greg Roelofs
            and contributors for the book, "PNG: The Definitive Guide,"
            published by O'Reilly and Associates.


	JAS - changed to flip rows around - images were being used upside
	down.

  ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "png.h"        /* libpng header; includes zlib.h */
#include "readpng.h"    /* typedefs, common macros, public prototypes */

/* future versions of libpng will provide this macro: */
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr)   ((png_ptr)->jmpbuf)
#endif


static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;

png_uint_32  width, height;
int  bit_depth, color_type;
uch  *image_data = NULL;

//JAS - not required.
//JAS void readpng_version_info(void)
//JAS {
//JAS     fprintf(stderr, "   Compiled with libpng %s; using libpng %s.\n",
//JAS       PNG_LIBPNG_VER_STRING, png_libpng_ver);
//JAS     fprintf(stderr, "   Compiled with zlib %s; using zlib %s.\n",
//JAS       ZLIB_VERSION, zlib_version);
//JAS }


/* return value = 0 for success, 1 for bad sig, 2 for bad IHDR, 4 for no mem */

int readpng_init(FILE *infile, ulg *pWidth, ulg *pHeight)
{
    uch sig[8];


    /* first do a quick check that the file really is a PNG image; could
     * have used slightly more general png_sig_cmp() function instead */

    fread(sig, 1, 8, infile);
    if (!png_check_sig(sig, 8))
        return 1;   /* bad signature */


    /* could pass pointers to user-defined error handlers instead of NULLs: */

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        return 4;   /* out of memory */

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return 4;   /* out of memory */
    }


    /* we could create a second info struct here (end_info), but it's only
     * useful if we want to keep pre- and post-IDAT chunk info separated
     * (mainly for PNG-aware image editors and converters) */


    /* setjmp() must be called in every function that calls a PNG-reading
     * libpng function */

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 2;
    }


    png_init_io(png_ptr, infile);
    png_set_sig_bytes(png_ptr, 8);  /* we already read the 8 signature bytes */

    png_read_info(png_ptr, info_ptr);  /* read all PNG info up to image data */


    /* alternatively, could make separate calls to png_get_image_width(),
     * etc., but want bit_depth and color_type for later [don't care about
     * compression_type and filter_type => NULLs] */

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
      NULL, NULL, NULL);
    *pWidth = width;
    *pHeight = height;


    /* OK, that's all we need for now; return happy */

    return 0;
}


//JAS  -- not required for FreeWRL
//JAS 
//JAS /* returns 0 if succeeds, 1 if fails due to no bKGD chunk, 2 if libpng error;
//JAS  * scales values to 8-bit if necessary */
//JAS 
//JAS int readpng_get_bgcolor(uch *red, uch *green, uch *blue)
//JAS {
//JAS     png_color_16p pBackground;
//JAS 
//JAS 
//JAS     /* setjmp() must be called in every function that calls a PNG-reading
//JAS      * libpng function */
//JAS 
//JAS     if (setjmp(png_jmpbuf(png_ptr))) {
//JAS         png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
//JAS         return 2;
//JAS     }
//JAS 
//JAS 
//JAS     if (!png_get_valid(png_ptr, info_ptr, PNG_INFO_bKGD))
//JAS         return 1;
//JAS 
//JAS     /* it is not obvious from the libpng documentation, but this function
//JAS      * takes a pointer to a pointer, and it always returns valid red, green
//JAS      * and blue values, regardless of color_type: */
//JAS 
//JAS     png_get_bKGD(png_ptr, info_ptr, &pBackground);
//JAS 
//JAS 
//JAS     /* however, it always returns the raw bKGD data, regardless of any
//JAS      * bit-depth transformations, so check depth and adjust if necessary */
//JAS 
//JAS     if (bit_depth == 16) {
//JAS         *red   = pBackground->red   >> 8;
//JAS         *green = pBackground->green >> 8;
//JAS         *blue  = pBackground->blue  >> 8;
//JAS     } else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
//JAS         if (bit_depth == 1)
//JAS             *red = *green = *blue = pBackground->gray? 255 : 0;
//JAS         else if (bit_depth == 2)
//JAS             *red = *green = *blue = (255/3) * pBackground->gray;
//JAS         else /* bit_depth == 4 */
//JAS             *red = *green = *blue = (255/15) * pBackground->gray;
//JAS     } else {
//JAS         *red   = (uch)pBackground->red;
//JAS         *green = (uch)pBackground->green;
//JAS         *blue  = (uch)pBackground->blue;
//JAS     }
//JAS 
//JAS     return 0;
//JAS }
//JAS 
//JAS 


/* display_exponent == LUT_exponent * CRT_exponent */

uch *readpng_get_image(double display_exponent, int *pChannels, ulg *pRowbytes)
{
    double  gamma;
    png_uint_32  i, rowbytes;
    png_bytepp  row_pointers = NULL;


    /* setjmp() must be called in every function that calls a PNG-reading
     * libpng function */

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }


    /* expand palette images to RGB, low-bit-depth grayscale images to 8 bits,
     * transparency chunks to full alpha channel; strip 16-bit-per-sample
     * images to 8 bits per sample; and convert grayscale to RGB[A] */

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);


    /* unlike the example in the libpng documentation, we have *no* idea where
     * this file may have come from--so if it doesn't have a file gamma, don't
     * do any correction ("do no harm") */

    if (png_get_gAMA(png_ptr, info_ptr, &gamma))
        png_set_gamma(png_ptr, display_exponent, gamma);


    /* all transformations have been registered; now update info_ptr data,
     * get rowbytes and channels, and allocate image memory */

    png_read_update_info(png_ptr, info_ptr);

    *pRowbytes = rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    *pChannels = (int)png_get_channels(png_ptr, info_ptr);

    if ((image_data = (uch *)malloc(rowbytes*height)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }
    if ((row_pointers = (png_bytepp)malloc(height*sizeof(png_bytep))) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(image_data);
        image_data = NULL;
        return NULL;
    }

    Trace((stderr, "readpng_get_image:  channels = %d, rowbytes = %ld, height = %ld\n", *pChannels, rowbytes, height));


    /* set the individual row_pointers to point at the correct offsets */

    for (i = 0;  i < height;  ++i) {
        //JAS - flip rows around row_pointers[i] = image_data + i*rowbytes;
        row_pointers[i] = image_data + (height-i-1)*rowbytes;
    }
    

    /* now we can go ahead and just read the whole image */

    png_read_image(png_ptr, row_pointers);


    /* and we're done!  (png_read_end() can be omitted if no processing of
     * post-IDAT text/time/etc. is desired) */

    free(row_pointers);
    row_pointers = NULL;

    //JAS png_read_end(png_ptr, NULL);

    return image_data;
}


void readpng_cleanup(int free_image_data)
{
    if (free_image_data && image_data) {
        free(image_data);
        image_data = NULL;
    }

    if (png_ptr && info_ptr) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        png_ptr = NULL;
        info_ptr = NULL;
    }
}
