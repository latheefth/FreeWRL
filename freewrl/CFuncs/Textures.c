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
#include "Structs.h"
#include "headers.h"

struct loadTexParams {
	/* data sent in to texture parsing thread */
	unsigned *texture_num;
	unsigned repeatS;
	unsigned repeatT;
	SV *parenturl;
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


/* for isloaded structure */
#define NOTLOADED	0
#define LOADING		1
#define NEEDSBINDING	2
#define LOADED		3	
#define INVALID		4
#define UNSQUASHED	5

/* supported image types */
#define IMAGETEXTURE	0
#define PIXELTEXTURE	1
#define MOVIETEXTURE	2

/* we keep track of which textures have been loaded, and which have not */
static int max_texture = 0;
static unsigned char  *isloaded; 
static struct loadTexParams *loadparams;

/* threading variables for loading textures in threads */
static pthread_t loadThread;
static pthread_mutex_t texmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t texcond   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t genmutex = PTHREAD_MUTEX_INITIALIZER;
#define TLOCK 		pthread_mutex_lock(&texmutex);
#define TUNLOCK 	pthread_mutex_unlock(&texmutex);
#define T_LOCK_SIGNAL 	pthread_cond_signal(&texcond);
#define T_LOCK_WAIT	pthread_cond_wait(&texcond,&texmutex);
/* lock the reallocs of data structures */
#define REGENLOCK 	pthread_mutex_lock(&genmutex);
#define REGENUNLOCK 	pthread_mutex_unlock(&genmutex);

int TexVerbose=0;

/* is the texture thread up and running yet? */
int TextureThreadInitialized = FALSE;

/* are we currently active? */
int TextureParsing = FALSE;

/* current index into loadparams that texture thread is working on */
int currentlyWorkingOn = -1;

/* function Prototypes */
int findTextureFile (int *texnum, int type, int *remove);
void _textureThread(void);
void store_tex_info(
		int texno,
		int depth, 
		int x, 
		int y, 
		unsigned char *ptr, 
		GLint Sgl_rep_or_clamp, 
		GLint Tgl_rep_or_clamp, 
		GLint Image);
	
void __reallyloadPixelTexture();
void __reallyloadImageTexture();
void __reallyloadMovieTexture();
void do_possible_multitexture(int texno);



/************************************************************************/
/* start up the texture thread */
void initializeTextureThread() {
	pthread_create (&loadThread, NULL, (void *)&_textureThread, NULL);
}

/* is the texture thread initialized yet? */
int isTextureinitialized() { 
	return TextureThreadInitialized;
}

/* statusbar uses this to tell user that we are still loading */
int isTextureParsing() {return currentlyWorkingOn>=0;}

/* load in a texture, if possible */
void loadImageTexture (struct VRML_ImageTexture *node) {
	bind_image(IMAGETEXTURE, node->__parenturl, 
		node->url, 
		&node->__texture,node->repeatS,node->repeatT);
}

/* load in a texture, if possible */
void loadPixelTexture (struct VRML_PixelTexture *node) {
	struct Multi_String mynull;
	bind_image(PIXELTEXTURE, node->image, 
		mynull, 
		&node->__texture,node->repeatS,node->repeatT);
}

/* load in a texture, if possible */
void loadMovieTexture (struct VRML_MovieTexture *node) {
	int isl;
	int origtex, firsttex,lasttex;
	/* when the data is "unsquished", this texture becomes invalid,
		and the new texture ranges are placed */

	firsttex = node->__texture0_;
	bind_image(MOVIETEXTURE, node->__parenturl, 
		node->url, 
		&node->__texture0_,node->repeatS,node->repeatT);

	/* is this texture now unsquished? (was NEEDSBINDING, now is INVALID) */
	if (isloaded[firsttex] == UNSQUASHED) {
		if (TexVerbose) 
			printf ("movie texture now unsquished, first and last textures %d %d ctex %d\n",
			loadparams[firsttex].x, loadparams[firsttex].y,
			node->__ctex);

		/* copy over the first and last texture numbers */
		node->__texture0_ = loadparams[firsttex].x;
		node->__texture1_ = loadparams[firsttex].y;

		/* which frame to start with? */
		if (node->speed>=0) node->__ctex = node->__texture0_;
		else node->__ctex = node->__texture1_;

		/* set this inactive... SensInterps will set active */
		node->isActive = 0;

		/* make an event for the inittime */
		node->__inittime = TickTime;
	}
			
		
}


/************************************************************************/
void store_tex_info(
		int texno,
		int depth, 
		int x, 
		int y, 
		unsigned char *ptr, 
		GLint Sgl_rep_or_clamp, 
		GLint Tgl_rep_or_clamp, 
		GLint Image) {

		loadparams[texno].frames=1;
		loadparams[texno].depth = depth;
		loadparams[texno].x = x;
		loadparams[texno].y = y;
		loadparams[texno].texdata = ptr;
		loadparams[texno].Src = Sgl_rep_or_clamp;
		loadparams[texno].Trc = Tgl_rep_or_clamp;
		loadparams[texno].Image = Image;
}

/* do we do 1 texture, or is this a series of textures, requiring final binding
   by this thread? */

void do_possible_multitexture(int texno) {
	int st,ed;
	int *texnums;
	int imageDatasize;
	int framecount;
	GLubyte *imageptr;

	if (loadparams[texno].frames > 1) {
		/* save the NUMBER of frames to copy. (numbered 0 to xxx[].frames-1) */
		framecount = loadparams[texno].frames;

		/* ok, a series of textures - eg, an mpeg file - needs unsquishing */
		texnums = malloc (sizeof(GLuint) * framecount);
		glGenTextures(framecount,texnums);

		/* this is the size of each image frame */
		imageDatasize = sizeof (GLbyte) * loadparams[texno].x *
                                loadparams[texno].y * loadparams[texno].depth;
printf ("ids is %d %d %d\n",loadparams[texno].x,loadparams[texno].y,loadparams[texno].depth);

		/* and, get a pointer to the whole, unsquished, image data */
		imageptr = loadparams[texno].texdata;

		for (st = 0; st < framecount; st++){
			/* make new table entries for these new textures */
			checkAndAllocTexMemTables(&(texnums[st]),16);

			/* copy most of the elements over from the base, verbatim */
			memcpy (&loadparams[texnums[st]], &loadparams[texno], sizeof (struct loadTexParams));

			/* elements that are different from the "standard" */
			loadparams[texnums[st]].texdata = malloc (imageDatasize);
			loadparams[texnums[st]].texture_num = &texnums[st];
			loadparams[texnums[st]].frames=1;

			/* copy the segment out of the squished data to this pure frame */
			memcpy(loadparams[texnums[st]].texdata, imageptr, imageDatasize);
			new_do_texture(texnums[st]);

			/* and, lets look at the next frame in the squished data */
			imageptr += imageDatasize;
			
		}
		/* we have unsquished this whole image; lets tell the caller this */
		isloaded[texno] = UNSQUASHED;
		loadparams[texno].x = texnums[0];
		loadparams[texno].y = texnums[framecount-1];
		free (loadparams[texno].texdata);
	} else {
		new_do_texture(texno);
		isloaded[texno] = LOADED;
	}
}


/* make this data into a OpenGL texture */

void new_do_texture(int texno) {
	int rx,ry,sx,sy;
	int depth,x,y;

	glBindTexture (GL_TEXTURE_2D, *loadparams[texno].texture_num);

	if (global_texSize==0) {
		/* called in OSX from the command line, so this is not set yet */
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &global_texSize);
	}

	/* save this to determine whether we need to do material node
	  within appearance or not */
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	/* Image should be GL_LINEAR for pictures, GL_NEAREST for pixelTs */
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, loadparams[texno].Image);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, loadparams[texno].Image);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, loadparams[texno].Src);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, loadparams[texno].Trc);
	depth = loadparams[texno].depth; 
	last_texture_depth = loadparams[texno].depth;
	x = loadparams[texno].x;
	y = loadparams[texno].y;

	if((depth) && x && y) {
		unsigned char *dest = loadparams[texno].texdata;
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
			gluScaleImage((unsigned)
			     ((depth)==1 ? GL_LUMINANCE : 
			     ((depth)==2 ? GL_LUMINANCE_ALPHA : 
			     ((depth)==3 ? GL_RGB : 
			     GL_RGBA))),
			     x, y, GL_UNSIGNED_BYTE, loadparams[texno].texdata, rx, ry, 
			     GL_UNSIGNED_BYTE, dest);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, depth,  rx, ry, 0, (unsigned)
			     ((depth)==1 ? GL_LUMINANCE : 
			     ((depth)==2 ? GL_LUMINANCE_ALPHA : 
			     ((depth)==3 ? GL_RGB : 
			     GL_RGBA))),
			     GL_UNSIGNED_BYTE, dest);
		if((loadparams[texno].texdata) != dest) free(dest);
		free (loadparams[texno].texdata);
	}
}

/**********************************************************************************

 bind the image, 

	itype 	tells us whether it is a PixelTexture, ImageTexture or MovieTexture.

	parenturl  is a pointer to the url of the parent (for relative loads) OR
		a pointer to the image data (PixelTextures only)

	url	the list of urls from the VRML file, or NULL (for PixelTextures)

	texture_num	the OpenGL texture identifier

	repeatS, repeatT VRML fields
************************************************************************************/

void bind_image(int itype, SV *parenturl, struct Multi_String url, 
		GLuint *texture_num, int repeatS, int repeatT) {

	/* temp variable */
	int count;

	/* yield for a bit */
	sched_yield();

	/* is this the first call for this texture? */
	if (*texture_num==0) {
		glGenTextures(1,texture_num);
		if (TexVerbose) printf ("just genned texture %d\n",*texture_num);
	}

	/* check to see if "isloaded" and "loadparams" is ok size-wise. if not,
	   make them larger, by 16 */
	checkAndAllocTexMemTables(texture_num, 16);
	
	/* have we already processed this one before? */
	if (isloaded[*texture_num] == LOADED) {
		glBindTexture (GL_TEXTURE_2D, *texture_num);
		return;
	}

	/* is this one bad? */
	if (isloaded[*texture_num] == INVALID) { return; }

	/* is this one an unsquished movie texture? */
	if (isloaded[*texture_num] == UNSQUASHED) { return; }

	/* is this one read in, but requiring final manipulation
	 * by THIS thread? */
	if (isloaded[*texture_num] == NEEDSBINDING) {
		if (TexVerbose) printf ("tex %d needs binding, name %s\n",*texture_num,
				loadparams[*texture_num].filename);
		do_possible_multitexture(*texture_num);
		if (TexVerbose) printf ("tex %d now loaded\n",*texture_num);
		return;
	}
	
	/* are we loading this one? */
	if (isloaded[*texture_num] == LOADING) {
		return;
	}

	/* so, we really need to do this one... */

	/* is the thread currently doing something? */
	if (TextureParsing) return;

        TLOCK
	loadparams[*texture_num].type = itype;
	loadparams[*texture_num].parenturl = parenturl;
	loadparams[*texture_num].url = url;
	loadparams[*texture_num].texture_num = texture_num;
	loadparams[*texture_num].repeatS = repeatS;
	loadparams[*texture_num].repeatT = repeatT;
	if (currentlyWorkingOn <0) {
		if (TexVerbose) 
			printf ("currentlyWorkingOn WAS %d ",currentlyWorkingOn);
		currentlyWorkingOn = *texture_num;
		if (TexVerbose)
			printf ("just set currentlyWorkingOn to %d\n",currentlyWorkingOn);
	}
        T_LOCK_SIGNAL
        TUNLOCK

}

/* check to see if we have enough memory for internal Texture tables */
/* we should lock, unless we are already locked- MPG decoding happens when locked */
void checkAndAllocTexMemTables(int *texture_num, int increment) {
	int count;
	int prev_max_texture;

	/* do we have enough room to save the isloaded flag for this texture? */
	if ((int)(*texture_num)>=(max_texture-2)) {
		REGENLOCK
		/* printf ("bind_image, must allocate a bunch more space for flags\n"); */
		prev_max_texture = max_texture;
		max_texture+=increment;
		isloaded = realloc(isloaded, sizeof(*isloaded) * max_texture);
		loadparams = realloc(loadparams, sizeof(*loadparams) * max_texture);

		/* printf ("zeroing from %d to %d\n",prev_max_texture,max_texture); */
		for (count = prev_max_texture; count < (int)max_texture; count++) {
			isloaded[count] = NOTLOADED;
			loadparams[count].filename="uninitialized file";
		}
		REGENUNLOCK
	}


}

/****************************************************************/
/*								*/
/*	Texture loading thread and associated functions		*/
/*								*/
/*	only do 1 texture at a time				*/
/*								*/
/*								*/
/*								*/
/****************************************************************/

/* find the file, either locally or within the Browser. Note that
   this is almost identical to the one for Inlines, but running
   in different threads */

int findTextureFile (int *texnum, int type, int *istemp) {
	char *filename;
	char *mypath;
	char *thisurl;
	char *slashindex;
	int count,flen;
	char firstBytes[4];
	char *sysline;

	/* pattern matching, for finding internally handled types */
	char firstPNG[] = {0x89,0x50,0x4e,0x47};
	char firstJPG[] = {0xff,0xd8,0xff,0xe0};
	char firstMPGa[] = {0x00, 0x00, 0x01, 0xba};
	char firstMPGb[] = {0x00, 0x00, 0x01, 0xb3};


	int xx;
	*istemp=FALSE;	/* don't remove this file */

	/* is this a PixelTexture? if so, we have the "file" in memory */
	if (type == PIXELTEXTURE) {
		return TRUE;
	}	

	/* nope, try to find this file. */
	filename = malloc(1000);
	
	/* lets make up the path and save it, and make it the global path */
	count = strlen(SvPV(loadparams[*texnum].parenturl,xx));
	mypath = malloc ((sizeof(char)* count)+1);
	
	if ((!filename) || (!mypath)) {
		printf ("texture thread can not malloc for filename\n");
		exit(1);
	}
	
	/* copy the parent path over */
	strcpy (mypath,SvPV(loadparams[*texnum].parenturl,xx));
	
	/* and strip off the file name, leaving any path */
	slashindex = rindex(mypath,'/');
	if (slashindex != NULL) { 
		slashindex ++; /* leave the slash on */
		*slashindex = 0; 
	 } else {mypath[0] = 0;}

	/* try the first url, up to the last */
	count = 0;
	while (count < loadparams[*texnum].url.n) {
		thisurl = SvPV(loadparams[*texnum].url.p[count],xx);
	
		/* check to make sure we don't overflow */
		if ((strlen(thisurl)+strlen(mypath)) > 900) break;
	
		/* does this name start off with a ftp, http, or a "/"? */
		if ((strncmp(thisurl,"ftp://", strlen("ftp://"))) &&
		   (strncmp(thisurl,"FTP://", strlen("FTP://"))) &&
		   (strncmp(thisurl,"http://", strlen("http://"))) &&
		   (strncmp(thisurl,"HTTP://", strlen("HTTP://"))) &&
		   (strncmp(thisurl,"/",strlen("/")))) {
			strcpy (filename,mypath);
		} else {
			filename[0]=0;
		}
		strcat(filename,thisurl);
		if (fileExists(filename,firstBytes)) { break; }
		count ++;
	}
	
	if (count != loadparams[*texnum].url.n) {
		/* printf ("we were successful at locating %s\n",filename);  */
	} else {
		printf ("Could not locate url (last choice was %s)\n",filename);
		free (filename);
		isloaded[*texnum]=INVALID;
		/* this crashes some Nvidia drivers 
		   glDeleteTextures(1,texnum);
		*/
		loadparams[*texnum].filename="file not found";
		return FALSE;
	}

	/* ok, have we seen this one before? */
	flen = strlen(filename);
	for (count=1; count < max_texture; count++) {
		/*
		if (TexVerbose)
			printf ("comparing :%s: :%s: (%d %d) count %d\n",
					filename,
					loadparams[count].filename,
					strlen(filename),
					strlen(loadparams[count].filename),
					count);
		*/

		/* are the names different lengths? */
		if (strlen(loadparams[count].filename) == flen) {
		    if(strncmp(loadparams[count].filename,filename,flen)==0) {
			if (TexVerbose) printf ("duplicate name %s at %d %d\n",
					filename,count,*texnum);

			/* duplicate, make this entry INVALID, and make the 
			   filename the same (avoids segfaults in strncmp, above)
			*/
			isloaded[*texnum]=INVALID;
			loadparams[*texnum].filename="Duplicate Filename";

			/* and, delete memory associated with this duplicate */
			/* this crashes some nvidia drivers 
			   glDeleteTextures(1,texnum);
			*/
			free (filename);

			/* and tell OpenGL to use the previous texture number */
			*texnum=count; 
			return FALSE;
		    }
		}	
	}

	/* is this a texture type that is *not* handled internally? */
	if ((strncmp(firstBytes,firstPNG,4) != 0) && 
	    (strncmp(firstBytes,firstJPG,4) != 0) && 
	    (strncmp(firstBytes,firstMPGa,4) != 0) && 
	    (strncmp(firstBytes,firstMPGb,4) != 0)) {
		sysline = malloc(sizeof(char)*(strlen(filename)+100));
		if (!sysline) {printf ("malloc failure in convert, exiting\n"); exit(1);}
		sprintf(sysline,"convert %s /tmp/freewrl%d.png",filename,getpid());
		if (system (sysline) != 0) {
			printf ("Freewrl: error running convert line %s\n",sysline);
		} else {
			sprintf (filename,"/tmp/freewrl%d.png",getpid());
			*istemp=TRUE;
		}	
		free (sysline);
	}

	/* save filename in data structure for later comparisons */
	loadparams[*texnum].filename = malloc(sizeof(char) * strlen(filename)+1);
	strcpy (loadparams[*texnum].filename,filename);
	free (filename);
	if (TexVerbose) 
		printf ("new name, save it %d, name %s\n",*texnum,loadparams[*texnum].filename);
	return TRUE;
}

void _textureThread(void) {

	int remove;


	/* temps */
	int count;

	/* we wait forever for the data signal to be sent */
	for (;;) {
		TLOCK
		TextureThreadInitialized = TRUE;
		T_LOCK_WAIT
		REGENLOCK
		isloaded[currentlyWorkingOn] = LOADING;
		TextureParsing = TRUE;

		/* look for the file. If one does not exist, or it
		   is a duplicate, just unlock and return */
		if (TexVerbose) 
			printf ("tex thread, currentlyworking on %d\n",currentlyWorkingOn);

		if (findTextureFile(loadparams[currentlyWorkingOn].texture_num,
			loadparams[currentlyWorkingOn].type,&remove)) {
	
			/* is this a pixeltexture? */
			if (loadparams[currentlyWorkingOn].type==PIXELTEXTURE) {
				__reallyloadPixelTexture();
			} else if (loadparams[currentlyWorkingOn].type==MOVIETEXTURE) {
				__reallyloadMovieTexture();
			} else { 
				__reallyloadImageTexture();
			}

			/* check to see if there was an error */
			if (isloaded[*loadparams[currentlyWorkingOn].texture_num]!=INVALID) 
				isloaded[*loadparams[currentlyWorkingOn].texture_num] = NEEDSBINDING;

			/* is this a temporary file? */
			if (remove == 1) {
				unlink (loadparams[currentlyWorkingOn].filename);
			}
		} else {
			if (TexVerbose) printf ("duplicate file, currentlyWorkingOn %d texnum %s\n",
				currentlyWorkingOn, loadparams[currentlyWorkingOn].texture_num);
		}

		/* signal that we are finished */
		if (TexVerbose)
			printf ("finished parsing texture for currentlyWorkingOn %d\n",currentlyWorkingOn);
		TextureParsing=FALSE;
		currentlyWorkingOn = -1;
		REGENUNLOCK
		TUNLOCK
	}
}


/********************************************************************************/
/* load specific types of textures						*/
/********************************************************************************/
void __reallyloadPixelTexture() {
	/* PixelTexture variables */
	unsigned hei,wid,depth;
	long inval;
	unsigned char *texture;
	unsigned char *tptr;
	int tctr;
	int xx;
	int count;

	tptr = SvPV(loadparams[currentlyWorkingOn].parenturl,xx);
	while (isspace(*tptr))tptr++;
	if (sscanf (tptr, "%i%i%i",&wid,&hei,&depth)==3) {
		if ((depth < 1) || (depth >4)) {
			printf ("PixelTexture, depth %d out of range, assuming 1\n",depth);
			depth = 1;
		}
		/* skip past the depth, width */
		while (!isspace(*tptr))tptr++;
		while (isspace(*tptr))tptr++;
		while (!isspace(*tptr))tptr++;
		while (isspace(*tptr))tptr++;
	
		/* have header ok, now read in all values */
		count = 0; tctr = 0;
		
		texture = malloc (wid*hei*4);
		
		
		while (count < (int)(wid*hei)) {
			inval = -9999;
			/* skip TO the number */
			while (!isspace(*tptr))tptr++;
			while (isspace(*tptr))tptr++;
			if (sscanf (tptr,"%lx",&inval) != 1) {
				printf("PixelTexture: expected %d pixels, got %d\n",wid*hei,count);
				isloaded[*loadparams[currentlyWorkingOn].texture_num] = INVALID;
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
			store_tex_info(currentlyWorkingOn,
				(int)depth,(int)wid,(int)hei,texture,
				((loadparams[currentlyWorkingOn].repeatS)) ? GL_REPEAT : GL_CLAMP, 
				((loadparams[currentlyWorkingOn].repeatT)) ? GL_REPEAT : GL_CLAMP,
				GL_NEAREST);
		}
	} else {
		printf ("PixelTexture, invalid height, width, or depth\n");
		isloaded[*loadparams[currentlyWorkingOn].texture_num] = INVALID;
	}
}
		
	
void __reallyloadImageTexture() {	
	FILE *infile;  
	char *filename; 
	GLuint texture_num; 
	unsigned char *image_data = 0;

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


	filename = loadparams[currentlyWorkingOn].filename;
	infile = fopen(filename,"r");
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

		store_tex_info( currentlyWorkingOn, 
			cinfo.output_components, (int)cinfo.output_width,
			(int)cinfo.output_height,image_data,
			((loadparams[currentlyWorkingOn].repeatS)) ? GL_REPEAT : GL_CLAMP, 
			((loadparams[currentlyWorkingOn].repeatT)) ? GL_REPEAT : GL_CLAMP,
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

			store_tex_info (currentlyWorkingOn, image_channels, 
				(int)image_width, (int)image_height,
				image_data,
				((loadparams[currentlyWorkingOn].repeatS)) ? GL_REPEAT : GL_CLAMP, 
				((loadparams[currentlyWorkingOn].repeatT)) ? GL_REPEAT : GL_CLAMP,
				GL_LINEAR);
		}
		readpng_cleanup (FALSE);
	}
	fclose (infile);
}


void __reallyloadMovieTexture () {
	int x,y,depth,frameCount;
	int ptr;
int j;
	int firstTex;
	
	firstTex = *loadparams[currentlyWorkingOn].texture_num;
	ptr=NULL;

	/* now, generate a new first texture */
	mpg_main(loadparams[currentlyWorkingOn].filename,
		&x,&y,&depth,&frameCount,&ptr);

	if (TexVerbose) printf ("ireallyloadmv frame count is %d depth %d ptr %d\n",frameCount,depth,ptr);

	/* store the "generic" data */
	store_tex_info(currentlyWorkingOn,
		(int)depth,(int)x,(int)y,ptr,
		((loadparams[currentlyWorkingOn].repeatS)) ? GL_REPEAT : GL_CLAMP,
		((loadparams[currentlyWorkingOn].repeatT)) ? GL_REPEAT : GL_CLAMP,
		GL_NEAREST);

	/* now, for the mpeg specific data */
	loadparams[currentlyWorkingOn].frames = frameCount;
}
