/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka
 Copyright (C) 2002 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/


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


/* for texIsloaded structure */
#define NOTLOADED       0
#define LOADING         1
#define NEEDSBINDING	2
#define LOADED          3
#define INVALID         4
#define UNSQUASHED      5

/* supported image types */
#define IMAGETEXTURE    0
#define PIXELTEXTURE    1
#define MOVIETEXTURE    2

/* bind_texture stores the param table pointer for the texture here */
extern void *texParams[];

void new_do_texture(int texno);
void checkAndAllocTexMemTables(GLuint *texture_num, int increment);

struct loadTexParams {
	/* data sent in to texture parsing thread */
	GLuint *texture_num;
	GLuint genned_texture;
	unsigned repeatS;
	unsigned repeatT;
	struct Uni_String *parenturl;
	unsigned type;
	struct Multi_String url;

	/* data returned from texture parsing thread */
	char *filename;
	int depth;
	int x;
	int y;
	int frames;		/* 1 unless video stream */
	unsigned char *texdata;
	GLint Src;
	GLint Trc;
	GLint Image;
};

struct multiTexParams {
	GLint texture_env_mode;
	GLint combine_rgb;
	GLint source0_rgb;
	GLint operand0_rgb;
	GLint source1_rgb;
	GLint operand1_rgb;
	GLint combine_alpha;
	GLint source0_alpha;
	GLint operand0_alpha;
	GLint source1_alpha;
	GLint operand1_alpha;
	GLfloat rgb_scale;
	GLfloat alpha_scale;
};


/* we keep track of which textures have been loaded, and which have not */
extern unsigned char  *texIsloaded;
extern struct loadTexParams *loadparams;

void bind_image(int type, struct Uni_String *parenturl, struct Multi_String url,
				GLuint *texture_num,
				int repeatS,
				int repeatT,
				void  *param);
