/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/


#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>


#ifdef AQUA 
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "headers.h"
#include "jpeglib.h"

void do_texture(int depth,
				int x,
				int y,
				unsigned char *ptr,
				GLint Sgl_rep_or_clamp,
				GLint Tgl_rep_or_clamp,
				GLint Image);

void bind_image(char *filename,
				GLuint texture_num,
				int repeatS,
				int repeatT,
				int remove);
