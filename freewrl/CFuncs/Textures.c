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


#include "Textures.h"
#include <pthread.h>

struct loadTexParams {
	/* data sent in to texture parsing thread */
	char *filename;
	unsigned texture_num;
	unsigned repeatS;
	unsigned repeatT;
	unsigned remove;

	/* data returned from texture parsing thread */
	int depth; 
	int x; 
	int y; 
	unsigned char *texdata; 
	GLint Src; 
	GLint Trc; 
	GLint Image;

	/* thread number to wait for */
	pthread_t	toWaitFor;
};


/* for isloaded structure */
#define NOTLOADED	0
#define LOADING		1
#define NEEDSBINDING	2
#define LOADED		3	
#define INVALID		4


/* we keep track of which textures have been loaded, and which have not */
static unsigned max_texture = 0;
static unsigned char  *isloaded; 
static struct loadTexParams *loadparams;

/* threading variables for loading textures in threads */
static pthread_t loadThread;
//static pthread_t lastTexThread = NULL;
static pthread_mutex_t texmutex = PTHREAD_MUTEX_INITIALIZER;
static activeThreadCount = 0;
#define TLOCK pthread_mutex_lock(&texmutex)
#define TUNLOCK pthread_mutex_unlock(&texmutex)

int TexVerbose=0;

/* function Prototype */
void load_in_tex(void *myparams);
char *message1 = "Thread 1";
pthread_t thread1;

void store_tex_info(
		struct loadTexParams *mydata,
		int depth, 
		int x, 
		int y, 
		unsigned char *ptr, 
		GLint Sgl_rep_or_clamp, 
		GLint Tgl_rep_or_clamp, 
		GLint Image) {

		mydata->depth = depth;
		mydata->x = x;
		mydata->y = y;
		mydata->texdata = ptr;
		mydata->Src = Sgl_rep_or_clamp;
		mydata->Trc = Tgl_rep_or_clamp;
		mydata->Image = Image;
}

void new_do_texture(struct loadTexParams *mydata) {
	int rx,ry,sx,sy;
	int depth,x,y;

	glBindTexture (GL_TEXTURE_2D, mydata->texture_num);

	if (global_texSize==0) {
		/* called in OSX from the command line, so this is not set yet */
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &global_texSize);

		/* set a maximum here; 1024 is probably more than fine. */
		if (global_texSize > 1024) global_texSize=1024; 
	}

	/* save this to determine whether we need to do material node
	  within appearance or not */
	
	last_texture_depth = depth;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	/* Image should be GL_LINEAR for pictures, GL_NEAREST for pixelTs */
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mydata->Image);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mydata->Image);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mydata->Src);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mydata->Trc);
	depth = mydata->depth; 
	x = mydata->x;
	y = mydata->y;

	if((depth) && x && y) {
		unsigned char *dest = mydata->texdata;
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
			dest = malloc((unsigned) (depth) * rx * ry);
			gluScaleImage(
			     ((depth)==1 ? GL_LUMINANCE : 
			     ((depth)==2 ? GL_LUMINANCE_ALPHA : 
			     ((depth)==3 ? GL_RGB : 
			     GL_RGBA))),
			     x, y, GL_UNSIGNED_BYTE, mydata->texdata, rx, ry, 
			     GL_UNSIGNED_BYTE, dest);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, depth,  rx, ry, 0,
			     ((depth)==1 ? GL_LUMINANCE : 
			     ((depth)==2 ? GL_LUMINANCE_ALPHA : 
			     ((depth)==3 ? GL_RGB : 
			     GL_RGBA))),
			     GL_UNSIGNED_BYTE, dest);
		if((mydata->texdata) != dest) free(dest);
		free (mydata->texdata);
	}
}

void bind_image(char *filename, GLuint *texture_num, int repeatS, int repeatT, int remove) 
{
	FILE *infile;

	/* temp variable */
	int count, flen;


	/* yield for a bit */
	sched_yield();


	/* is this the first call for this texture? */
	if (*texture_num==0) {
		glGenTextures(1,texture_num);
		if (TexVerbose) printf ("just genned texture %d\n",*texture_num);
	}

	/* do we have enough room to save the isloaded flag for this texture? */
	if (*texture_num>=max_texture) {
		/* printf ("bind_image, must allocate a bunch more space for flags\n"); */
		max_texture += 1024;
		isloaded = realloc(isloaded, sizeof(*isloaded) * max_texture);
		loadparams = realloc(loadparams, sizeof(*loadparams) * max_texture);

		/* printf ("zeroing from %d to %d\n",max_texture-1024,max_texture); */
		for (count = (int)max_texture-1024; count < (int)max_texture; count++) {
			isloaded[count] = NOTLOADED;
			loadparams[count].filename=0;
		}
	}
	
	/* have we already processed this one before? */
	if (isloaded[*texture_num] == LOADED) {
		glBindTexture (GL_TEXTURE_2D, *texture_num);
		return;
	}

	/* is this one bad? */
	if (isloaded[*texture_num] == INVALID) {
		return;
	}

	/* is this one read in, but requiring final manipulation
	 * by THIS thread? */
	if (isloaded[*texture_num] == NEEDSBINDING) {
		new_do_texture(&loadparams[*texture_num]);
		isloaded[*texture_num] = LOADED;
		return;
	}
	
	/* are we loading this one? */
	if (isloaded[*texture_num] == LOADING) {
		return;
	}

	/* so, we really need to do this one... */
	loadparams[*texture_num].filename = filename;
	loadparams[*texture_num].texture_num = *texture_num;
	loadparams[*texture_num].repeatS = repeatS;
	loadparams[*texture_num].repeatT = repeatT;
	loadparams[*texture_num].remove = remove;

	/* hmmm - is this one a duplicate? */
	/* if so, just make the texture pointer the same as
	 * the one before, and delete this texture */
	flen = strlen(filename);
	for (count=1; count < *texture_num; count++) {
		if (strncmp(filename,loadparams[count].filename,flen)==0) {
			if (TexVerbose) printf ("duplicate name %s at %d %d\n",
					filename,count,*texture_num);

			isloaded[*texture_num]==INVALID;
			glDeleteTextures(1,texture_num);
			*texture_num=count; 
			return;
		}	
	}

	/* do we have enough threads already ? */
	TLOCK;
	if (activeThreadCount !=0) {
		TUNLOCK;
		return;
	}
	activeThreadCount=1;
	TUNLOCK;

	isloaded[*texture_num] = LOADING;

	pthread_create (&loadThread, 
			NULL, (void *)&load_in_tex, 
			&loadparams[*texture_num]);

	if (TexVerbose) printf ("just created thread for %s, id %d tex %d\n",
			filename,loadThread, *texture_num);

	/* and save this thread id for later calls */
	//lastTexThread = loadthread[*texture_num];
}


void load_in_tex(void *ptr) {

	char *filename; 
	FILE *infile;  
	GLuint texture_num; 
	int repeatS; 
	int repeatT; 
	int remove;

	struct loadTexParams *mydata;
	unsigned char *image_data = 0;

	/* PixelTexture variables */
	unsigned hei,wid,depth;
	long inval;
	unsigned char *texture;
	int tctr;
	
	/* png reading variables */
	int rc;
	unsigned long image_width = 0;
	unsigned long image_height = 0;
	unsigned long image_rowbytes = 0;
	int image_channels = 0;
	double display_exponent = 0.0;

	/* jpeg variables */
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JDIMENSION nrows;
	JSAMPROW row = 0;
	JSAMPROW rowptr[1];
	unsigned rowcount, columncount;
	int dp;

	/* temps */
	int count;
//	struct sched_param myparam;


	mydata = (struct load_in_data *)ptr;
	filename = mydata->filename;

	/* drop my priority down */
	//dp = sched_getparam(0, &myparam);
	//printf ("thread getparam returns %d\n",dp);
//printf ("thread priority is %d \n",myparam.sched_priority);
//	dp = pthread_setschedparam (0, SCHED_FIFO, &myparam);
//	printf ("thread setschedparam returns %d\n",dp);

	/* do we have to wait for another thread? */
//	if ((mydata->toWaitFor) != NULL) {
//		printf ("load_texture, waiting for %d\n",mydata->toWaitFor); 
//		pthread_join(mydata->toWaitFor, NULL);
//		printf ("load_texture, %d finished, continuing with %d \n",mydata->toWaitFor,
//			pthread_self()); 
//	}

	/* load in the texture, if we can... */
        if (!(infile = fopen(filename, "rb"))) {
                printf("can not open ImageTexture file [%s]\n", filename);
		isloaded[texture_num] = INVALID;
		TLOCK;
		activeThreadCount=0;
		TUNLOCK;
		return;
	}

	texture_num = mydata->texture_num;
	repeatS = mydata->repeatS;
	repeatT = mydata->repeatT;
	remove = mydata->remove;

	/* is this a pixeltexture? */
	if (strstr(filename,".pixtex")) {
		if (fscanf (infile, "%i%i%i",&wid,&hei,&depth)) {
			if ((depth < 1) || (depth >4)) {
				printf ("PixelTexture, depth %d out of range, assuming 1\n",depth);
				depth = 1;
			}

			/* have header ok, now read in all values */
			count = 0; tctr = 0;

			texture = malloc (wid*hei*4);


			while (count < (int)(wid*hei)) {
				inval = -9999;
				if (fscanf (infile,"%lx",&inval) != 1) {
					printf("PixelTexture: expected %d pixels, got %d\n",wid*hei,count);
					isloaded[texture_num] = INVALID;
					break;
				}
				switch (depth) {
					case 1: {
						   texture[tctr++] = inval & 0xff;
						   break;
					   }
					case 2: {
						   texture[tctr++] = inval & 0x00ff;
						   texture[tctr++] = (inval>>8) & 0xff;
						   break;
					   }
					case 3: {
						   texture[tctr++] = (inval>>16) & 0xff; //R
						   texture[tctr++] = (inval>>8) & 0xff;	 //G
						   texture[tctr++] = (inval>>0) & 0xff; //B
						   break;
					   }
					case 4: {
						   texture[tctr++] = (inval>>24) & 0xff; //A
						   texture[tctr++] = (inval>>16) & 0xff; //R
						   texture[tctr++] = (inval>>8) & 0xff;	 //G
						   texture[tctr++] = (inval>>0) & 0xff; //B
						   break;
					   }
				}

				count ++;
			}

			if (count == (int)(wid*hei)) {
				store_tex_info(mydata,
					(int)depth,(int)wid,(int)hei,texture,
					((repeatS)) ? GL_REPEAT : GL_CLAMP, 
					((repeatT)) ? GL_REPEAT : GL_CLAMP,
					GL_NEAREST);
			}
		} else {
			printf ("PixelTexture, invalid height, width, or depth\n");
			isloaded[texture_num] = INVALID;
		}


	} else{ 
	if ((rc = readpng_init(infile, &image_width, &image_height)) != 0) {

		/* it is not a png file - assume a jpeg file */
		/* start from the beginning again */

		rewind (infile);
		
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
			/* yield for a bit */
			sched_yield();


			for (columncount = 0; columncount < cinfo.output_width; columncount++) {
				for(dp=0; dp<cinfo.output_components; dp++) {
					image_data[(cinfo.output_height-rowcount-1)
							*cinfo.output_width*cinfo.output_components
					       		+ columncount* cinfo.output_components	+dp]
						= row[columncount*cinfo.output_components + dp];
				}
			}
		}
			

		if (jpeg_finish_decompress(&cinfo) != TRUE) {
			printf("warning: jpeg_finish_decompress error\n");
			isloaded[texture_num] = INVALID;
		}
		jpeg_destroy_decompress(&cinfo);
		free(row);

		store_tex_info( mydata, 
			cinfo.output_components, (int)cinfo.output_width,
			(int)cinfo.output_height,image_data,
			((repeatS)) ? GL_REPEAT : GL_CLAMP, 
			((repeatT)) ? GL_REPEAT : GL_CLAMP,
			GL_LINEAR);
	} else { 
		if (rc != 0) {
		isloaded[texture_num] = INVALID;
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

			store_tex_info (mydata, image_channels, 
				(int)image_width, (int)image_height,
				image_data,
				((repeatS)) ? GL_REPEAT : GL_CLAMP, 
				((repeatT)) ? GL_REPEAT : GL_CLAMP,
				GL_LINEAR);
		}
		readpng_cleanup (FALSE);
	}
	}
	fclose (infile);


	/* check to see if there was an error */
	if (isloaded[texture_num]!=INVALID) isloaded[texture_num] = NEEDSBINDING;

	/* is this a temporary file? */
	if (remove == 1) {
		unlink (filename);
	}
	
	TLOCK;
	activeThreadCount=0;
	TUNLOCK;
}
