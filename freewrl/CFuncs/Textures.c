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


#include "CORE/EXTERN.h"
#include "CORE/perl.h"
#include "CORE/XSUB.h"
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "Structs.h"
#include "headers.h"


void do_texture(depth,x,y,ptr,Sgl_rep_or_clamp, Tgl_rep_or_clamp,Image)
	int x,y,depth;
	GLint Sgl_rep_or_clamp;
	GLint Tgl_rep_or_clamp;
	GLint Image;
	unsigned char *ptr;


{

	GLint texSize;
	int rx,ry,sx,sy;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	/* Image should be GL_LINEAR for pictures, GL_NEAREST for pixelTs */
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Image );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Image );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Sgl_rep_or_clamp);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Tgl_rep_or_clamp);

	/* find out the largest size that a texture can have. */
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);

	if((depth) && x && y) {
		unsigned char *dest = ptr;
		rx = 1; sx = x;
		while(sx) {sx /= 2; rx *= 2;}
		if(rx/2 == x) {rx /= 2;}
		ry = 1; sy = y;
		while(sy) {sy /= 2; ry *= 2;}
		if(ry/2 == y) {ry /= 2;}
		if(rx != x || ry != y || rx > texSize || ry > texSize) {
			/* do we have texture limits??? */
			if (rx > texSize) rx = texSize;
			if (ry > texSize) ry = texSize;

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

