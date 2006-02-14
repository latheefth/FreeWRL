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
#include <stdio.h>
#include "readpng.h"

/* we keep track of which textures have been loaded, and which have not */
static int max_texture = 0;
struct loadTexParams *loadparams;
unsigned char  *texIsloaded;

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

/* is the texture thread up and running yet? */
int TextureThreadInitialized = FALSE;

static int next_texture  = 1;

/* are we currently active? */
int TextureParsing = FALSE;

/* current index into loadparams that texture thread is working on */
int currentlyWorkingOn = -1;

int textureInProcess = -1;

/* how many texel units; if -1, we have not tried to find out yet */
GLint maxTexelUnits = -1;


/* for texture remapping in TextureCoordinate nodes */
int	*global_tcin;
int	global_tcin_count;

/* for AQUA OS X sharing of OpenGL Contexts */
#ifdef AQUA
//#include "/System/./Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/CoreGraphics.framework/Versions/A/Headers/CGDirectDisplay.h"
#include "CGDirectDisplay.h"
extern CGLContextObj aqglobalContext;
CGLPixelFormatAttribute attribs[] = { kCGLPFADisplayMask, 0,
                                      kCGLPFAFullScreen,
                                      kCGLPFADoubleBuffer,
                                      0 };
CGLPixelFormatObj pixelFormat = NULL;
long numPixelFormats = 0;
CGLContextObj aqtextureContext = NULL;
#endif


/* function Prototypes */
int findTextureFile (int cwo, int *remove);
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

void __reallyloadPixelTexture(void);
void __reallyloadImageTexture(void);
void __reallyloadMovieTexture(void);
void do_possible_textureSequence(int texno);

int readpng_init(FILE *infile, ulg *pWidth, ulg *pHeight);
void readpng_cleanup(int free_image_data);



/************************************************************************/
/* start up the texture thread */
void initializeTextureThread() {
	pthread_create (&loadThread, NULL, (void*(*)(void *))&_textureThread, NULL);
}

/* is the texture thread initialized yet? */
int isTextureinitialized() {
	return TextureThreadInitialized;
}

/* is this texture loaded? used in LoadSensor */
int isTextureLoaded(int texno) {
	/* no, have not even started looking at this */

	if (texno == 0) return FALSE;
	return (texIsloaded[texno]==LOADED);
}


/* statusbar uses this to tell user that we are still loading */
int isTextureParsing() {
	/* return currentlyWorkingOn>=0; */
	#ifdef TEXVERBOSE 
	printf ("call to isTextureParsing, returning %d\n",textureInProcess > 0);
	#endif

	return textureInProcess >0;
}

/* can our OpenGL implementation handle multitexturing?? */
void init_multitexture_handling() {
	char *glExtensions;

	glExtensions = (char *)glGetString(GL_EXTENSIONS);

	if ((strstr (glExtensions, "GL_ARB_texture_env_combine")!=0) &&
		(strstr (glExtensions,"GL_ARB_multitexture")!=0)) {

		glGetIntegerv(GL_MAX_TEXTURE_UNITS,&maxTexelUnits);

		if (maxTexelUnits > MAX_MULTITEXTURE) {
			printf ("init_multitexture_handling - reducing number of multitexs from %d to %d\n",
				maxTexelUnits,MAX_MULTITEXTURE);
			maxTexelUnits = MAX_MULTITEXTURE;
		}
		#ifdef TEXVERBOSE
		printf ("can do multitexture we have %d units\n",maxTexelUnits);
		#endif


		/* we assume that GL_TEXTURE*_ARB are sequential. Lets to a little check */
		if ((GL_TEXTURE0 +1) != GL_TEXTURE1) {
			printf ("Warning, code expects GL_TEXTURE0 to be 1 less than GL_TEXTURE1\n");
		} 

	} else {
		printf ("can not do multitexture\n");
		maxTexelUnits = 0;
	}
}



/* lets remove this texture from the process... */
void freeTexture (GLuint *texno) {
	#ifdef TEXVERBOSE 
	printf ("freeTexture, texno %d cwo %d inprocess %d\n",*texno,currentlyWorkingOn, textureInProcess );
	#endif

	if (*texno > 0) texIsloaded[*texno] = INVALID;

	/* is this the texture that we are currently working on?? */
	if ((*texno) == textureInProcess) {
		#ifdef TEXVERBOSE 
		printf ("freeTexture - zeroing textureInProcess, too\n");
		#endif

		textureInProcess = -1;
	}
	/*
	if (currentlyWorkingOn != -1) {
		printf ("freeTexture, cwo ne neg1, what should we do with it?\n");
	}
	*/

	/* this crashes some Nvidia drivers
	glDeleteTextures(1,texo);
	*/

	*texno = 0;
}


/* do Background textures, if possible */
void loadBackgroundTextures (struct X3D_Background *node) {
	int *thistex = 0;
	struct Multi_String thisurl;
	int count;

	for (count=0; count<6; count++) {
		/* go through these, back, front, top, bottom, right left */
		switch (count) {
			case 0: {thistex = &node->__texturefront;  thisurl = node->frontUrl; break;}
			case 1: {thistex = &node->__textureback;   thisurl = node->backUrl; break;}
			case 2: {thistex = &node->__texturetop;    thisurl = node->topUrl; break;}
			case 3: {thistex = &node->__texturebottom; thisurl = node->bottomUrl; break;}
			case 4: {thistex = &node->__textureright;  thisurl = node->rightUrl; break;}
			case 5: {thistex = &node->__textureleft;   thisurl = node->leftUrl; break;}
		}
		if (thisurl.n != 0) {
			/* we have an image specified for this face */

			bind_image (IMAGETEXTURE, node->__parenturl, thisurl, (GLuint *)thistex, 0, 0,
				NULL);

			/* if we do not have an image for this Background face yet, dont draw
			 * the quads */

			if (texIsloaded[*thistex] == LOADED)
				glDrawArrays (GL_QUADS, count*4,4);
		};
	}
}


/* do TextureBackground textures, if possible */
void loadTextureBackgroundTextures (struct X3D_TextureBackground *node) {
	struct X3D_Box *thistex = 0;
	int count;

	for (count=0; count<6; count++) {
		/* go through these, back, front, top, bottom, right left */
		switch (count) {
			case 0: {thistex = (struct X3D_Box *)node->frontTexture;  break;}
			case 1: {thistex = (struct X3D_Box *)node->backTexture;   break;}
			case 2: {thistex = (struct X3D_Box *)node->topTexture;    break;}
			case 3: {thistex = (struct X3D_Box *)node->bottomTexture; break;}
			case 4: {thistex = (struct X3D_Box *)node->rightTexture;  break;}
			case 5: {thistex = (struct X3D_Box *)node->leftTexture;   break;}
		}
		if (thistex != 0) {
			/* we have an image specified for this face */
			/* the X3D spec says that a X3DTextureNode has to be one of... */
			if ((thistex->_nodeType == NODE_ImageTexture) ||
			    (thistex->_nodeType == NODE_PixelTexture) ||
			    (thistex->_nodeType == NODE_MovieTexture) ||
			    (thistex->_nodeType == NODE_MultiTexture)) {

				texture_count = 0;
				/* render the proper texture */
				render_node((void *)thistex);
		                glColor3d(1.0,1.0,1.0);

        			textureDraw_start(NULL,Backtex);
        			glVertexPointer (3,GL_FLOAT,0,BackgroundVert);
        			glNormalPointer (GL_FLOAT,0,Backnorms);

        			glDrawArrays (GL_QUADS, count*4, 4);
        			textureDraw_end();



			} 
		}
	}
}

/* load in a texture, if possible */
void loadImageTexture (struct X3D_ImageTexture *node, void *param) {
	if (node->_ichange != node->_change) {
		/* force a node reload - make it a new texture. Don't change
		 the parameters for the original number, because if this
		 texture is shared, then ALL references will change! so,
		 we just accept that the current texture parameters have to
		 be left behind. */
		node->_ichange = node->_change;

		/* this will cause bind_image to create a new "slot" for this texture */
		/* cast to GLuint because __texture defined in VRMLNodes.pm as SFInt */
		freeTexture((GLuint*) &(node->__texture)); 
	}

	bind_image(IMAGETEXTURE, node->__parenturl,
		node->url,
		(GLuint*)&node->__texture,node->repeatS,node->repeatT,param);

        bound_textures[texture_count] = node->__texture;
}

void loadMultiTexture (struct X3D_MultiTexture *node) {
	int count;
	int max;
	struct multiTexParams *paramPtr;

	char *param;
	STRLEN xx;

	struct X3D_ImageTexture *nt;

	#ifdef TEXVERBOSE
	 printf ("loadMultiTexture, this %d have %d textures %d %d\n",node->_nodeType,
			node->texture.n,
			node->texture.p[0], node->texture.p[1]);
	printf ("	change %d ichange %d\n",node->_change, node->_ichange);
	#endif
	
	/* new node, or node paramaters changed */
        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;

		/* have we initiated multitexture support yet? */
		if (maxTexelUnits < 0)  {
			init_multitexture_handling();
		}

		/* alloc fields, if required - only do this once, even if node changes */
		if (node->__params == 0) {
			/* printf ("loadMulti, mallocing for params\n"); */
			node->__params = (int) malloc (sizeof (struct multiTexParams) * maxTexelUnits);
			paramPtr = (struct multiTexParams*) node->__params;

			/* set defaults for these fields */
			for (count = 0; count < maxTexelUnits; count++) {
				paramPtr->texture_env_mode  = GL_MODULATE; 
				paramPtr->combine_rgb = GL_MODULATE;
				paramPtr->source0_rgb = GL_TEXTURE;
				paramPtr->operand0_rgb = GL_SRC_COLOR;
				paramPtr->source1_rgb = GL_PREVIOUS;
				paramPtr->operand1_rgb = GL_SRC_COLOR;
				paramPtr->combine_alpha = GL_REPLACE;
				paramPtr->source0_alpha = GL_TEXTURE;
				paramPtr->operand0_alpha = GL_SRC_ALPHA;
				paramPtr->source1_alpha = 0;
				paramPtr->operand1_alpha = 0;
				/*
				paramPtr->source1_alpha = GL_PREVIOUS;
				paramPtr->operand1_alpha = GL_SRC_ALPHA;
				*/
				paramPtr->rgb_scale = 1;
				paramPtr->alpha_scale = 1;
				paramPtr++;
			}
		}

		/* how many textures can we use? no sense scanning those we cant use */
		max = node->mode.n; 
		if (max > maxTexelUnits) max = maxTexelUnits;

		/* go through the params, and change string name into a GLint */
		paramPtr = (struct multiTexParams*) node->__params;
		for (count = 0; count < max; count++) {
			param = SvPV(node->mode.p[count],xx);
			/* printf ("param %d is %s len %d\n",count, param, xx); */

		        if (strncmp("MODULATE2X",param,strlen("MODULATE2X"))==0) { 
				paramPtr->texture_env_mode  = GL_COMBINE; 
                                paramPtr->rgb_scale = 2;
                                paramPtr->alpha_scale = 2; } 

		        else if (strncmp("MODULATE4X",param,strlen("MODULATE4X"))==0) {
				paramPtr->texture_env_mode  = GL_COMBINE; 
                                paramPtr->rgb_scale = 4;
                                paramPtr->alpha_scale = 4; } 
		        else if (strncmp("ADDSMOOTH",param,strlen("ADDSMOOTH"))==0) {  
				paramPtr->texture_env_mode = GL_COMBINE;
				paramPtr->combine_rgb = GL_ADD;}
/* 
#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305

*/
		        else if (strncmp("BLENDDIFFUSEALPHA",param,strlen("BLENDDIFFUSEALPHA"))==0) {  
				paramPtr->texture_env_mode = GL_COMBINE;
				paramPtr->combine_rgb = GL_SUBTRACT;}
		        else if (strncmp("BLENDCURRENTALPHA",param,strlen("BLENDCURRENTALPHA"))==0) {  
				paramPtr->texture_env_mode = GL_COMBINE;
				paramPtr->combine_rgb = GL_SUBTRACT;}
		        else if (strncmp("MODULATEALPHA_ADDCOLOR",param,strlen("MODULATEALPHA_ADDCOLOR"))==0) { 
				paramPtr->texture_env_mode = GL_COMBINE;
				paramPtr->combine_rgb = GL_SUBTRACT;}
		        else if (strncmp("MODULATEINVALPHA_ADDCOLOR",param,strlen("MODULATEINVALPHA_ADDCOLOR"))==0) { 
				paramPtr->texture_env_mode = GL_COMBINE;
				paramPtr->combine_rgb = GL_SUBTRACT;}
		        else if (strncmp("MODULATEINVCOLOR_ADDALPHA",param,strlen("MODULATEINVCOLOR_ADDALPHA"))==0) { 
				paramPtr->texture_env_mode = GL_COMBINE;
				paramPtr->combine_rgb = GL_SUBTRACT;}
		        else if (strncmp("SELECTARG1",param,strlen("SELECTARG1"))==0) {  
				paramPtr->texture_env_mode = GL_REPLACE;
				paramPtr->combine_rgb = GL_TEXTURE0;}
		        else if (strncmp("SELECTARG2",param,strlen("SELECTARG2"))==0) {  
				paramPtr->texture_env_mode = GL_REPLACE;
				paramPtr->combine_rgb = GL_TEXTURE1;}
		        else if (strncmp("DOTPRODUCT3",param,strlen("DOTPRODUCT3"))==0) {  
				paramPtr->texture_env_mode = GL_COMBINE;
				paramPtr->combine_rgb = GL_DOT3_RGB;}
/* */
		        else if (strncmp("MODULATE",param,strlen("MODULATE"))==0) {
				/* defaults */}

		        else if (strncmp("REPLACE",param,strlen("REPLACE"))==0) {
				paramPtr->texture_env_mode = GL_REPLACE;}

		        else if (strncmp("SUBTRACT",param,strlen("SUBTRACT"))==0) {
				paramPtr->texture_env_mode = GL_COMBINE;
				paramPtr->combine_rgb = GL_SUBTRACT;}

		        else if (strncmp("ADDSIGNED2X",param,strlen("ADDSIGNED2X"))==0) {
				paramPtr->rgb_scale = 2;
				paramPtr->alpha_scale = 2;
				paramPtr->texture_env_mode = GL_COMBINE; 
				paramPtr->combine_rgb = GL_ADD_SIGNED;}

		        else if (strncmp("ADDSIGNED",param,strlen("ADDSIGNED"))==0) {
				paramPtr->texture_env_mode = GL_COMBINE; 
				paramPtr->combine_rgb = GL_ADD_SIGNED;}


		        else if (strncmp("ADD",param,strlen("ADD"))==0) {
					paramPtr->texture_env_mode = GL_COMBINE;
					paramPtr->combine_rgb = GL_ADD; }


		        else if (strncmp("OFF",param,strlen("OFF"))==0) { 
					paramPtr->texture_env_mode = 0; } 




			else {
				ConsoleMessage ("MultiTexture - invalid param or not supported yet- \"%s\"\n",param);
			}

			/* printf ("paramPtr for %d is %d\n",count,*paramPtr);  */
			paramPtr++;
		}


	/* coparamPtrile the sources */
/*
""
"DIFFUSE"
"SPECULAR"
"FACTOR"
*/
	/* coparamPtrile the functions */
/*""
"COMPLEMENT"
"ALPHAREPLICATE"
*/


	}

	/* ok, normally the scene graph contains function pointers. What we have
	   here is a set of pointers to datastructures of (hopefully!)
	   types like X3D_ImageTexture, X3D_PixelTexture, and X3D_MovieTexture.

	*/

	/* how many textures can we use? */
	max = node->texture.n; 
	if (max > maxTexelUnits) max = maxTexelUnits;

	/* go through and get all of the textures */
	paramPtr = (struct multiTexParams *) node->__params;

	for (count=0; count < max; count++) {
		#ifdef TEXVERBOSE
		printf ("loadMultiTexture, working on texture %d\n",count);
		#endif

		/* get the texture */
		nt = node->texture.p[count];

		switch (nt->_nodeType) {
			case NODE_ImageTexture : 
				/* printf ("MultiTexture %d is a ImageTexture param %d\n",count,*paramPtr);  */
				loadImageTexture ((struct X3D_ImageTexture*) nt,
					(void *)paramPtr);
				break;
			case NODE_MultiTexture:
				printf ("MultiTexture texture %d is a MULTITEXTURE!!\n",count);
				break;
			case NODE_PixelTexture:
				/* printf ("MultiTexture %d is a PixelTexture\n",count); */
				loadPixelTexture ((struct X3D_PixelTexture*) nt,
					(void *)paramPtr);
				break;
			case NODE_MovieTexture:
				/* printf ("MultiTexture %d is a MovieTexture\n"); */
				loadMovieTexture ((struct X3D_MovieTexture*) nt,
					(void *)paramPtr);
				break;
			default:
				printf ("MultiTexture - unknown sub texture type %d\n",
						nt->_nodeType);
		}

		/* now, lets increment texture_count. The current texture will be
		   stored in bound_textures[texture_count]; texture_count will be 1
		   for "normal" textures; at least 1 for MultiTextures. */

        	texture_count++;
		paramPtr++;

		#ifdef TEXVERBOSE
		printf ("loadMultiTexture, finished with texture %d\n",count);
		#endif
	}
}


/* load in a texture, if possible */
void loadPixelTexture (struct X3D_PixelTexture *node, void *param) {
	struct Multi_String mynull;

	if (node->_ichange != node->_change) {
		/* force a node reload - make it a new texture. Don't change
		 the parameters for the original number, because if this
		 texture is shared, then ALL references will change! so,
		 we just accept that the current texture parameters have to
		 be left behind. */
		node->_ichange = node->_change;
		/* this will cause bind_image to create a new "slot" for this texture */
		/* cast to GLuint because __texture defined in VRMLNodes.pm as SFInt */
		freeTexture((GLuint*) &(node->__texture)); 
	}

	bind_image(PIXELTEXTURE, node->image,
		mynull,
		(GLuint*)&node->__texture,node->repeatS,node->repeatT, param);

        bound_textures[texture_count] = node->__texture;
}

/* load in a texture, if possible */
void loadMovieTexture (struct X3D_MovieTexture *node, void *param) {
	int firsttex;

	/* possible bug? If two nodes use the same MovieTexture URL, and,
	 * one changes (eg, EAI), then possibly both will change. This
	 * happened with ImageTextures, doubt if it will happen here */

	/* when the data is "unsquished", this texture becomes invalid,
		and the new texture ranges are placed */
	firsttex = node->__texture0_;

	if (node->_ichange != node->_change) {
		node->_change = node->_ichange;
		/*  did the URL's change? we can't test for _change here, because
		   movie running will change it, so we look at the urls. */
		if ((node->url.p) != (node->__oldurl.p)) {
			/*  we have a node change - is this an initial load, or
			 a reload? */
			if (firsttex > 0) {
				/*  we have changed the url - reload it all. */
				texIsloaded[firsttex] = NOTLOADED;
				loadparams[firsttex].filename="uninitialized file";
				loadparams[firsttex].depth = 0;
				freeTexture((GLuint *)&(node->__texture0_)); /* this will cause bind_image to create a */
				freeTexture((GLuint *)&(node->__texture1_)); /* this will cause bind_image to create a */
				node->__ctex = 0;
				node->__inittime = 0;
				node->__sourceNumber = -1;
			}
			node->__oldurl.p = node->url.p;
		}
	}

	bind_image(MOVIETEXTURE, node->__parenturl,
		node->url,
		(GLuint*)&node->__texture0_,node->repeatS,node->repeatT, param);

	/* is this texture now unsquished? (was NEEDSBINDING, now is INVALID) */

	if (texIsloaded[firsttex] == UNSQUASHED) {
		#ifdef TEXVERBOSE
			printf ("movie texture now unsquished, first and last textures %d %d ctex %d\n",
			loadparams[firsttex].x, loadparams[firsttex].y,
			node->__ctex);

		#endif

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
        bound_textures[texture_count] = node->__ctex;
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

void do_possible_textureSequence(int texno) {
	int st;
	GLuint *texnums;
	int imageDatasize;
	int framecount;
	GLubyte *imageptr;
	int c;

	if (loadparams[texno].frames > 1) {
		/* save the NUMBER of frames to copy. (numbered 0 to xxx[].frames-1) */
		framecount = loadparams[texno].frames;

		/* ok, a series of textures - eg, an mpeg file - needs unsquishing */
		texnums = (GLuint*)malloc (sizeof(GLuint) * framecount);
		glGenTextures(framecount,texnums);

		/* Irix returns "wierd" numbers; this appears to work
		   across all platforms */
		for (c=0; c<framecount; c++) {
			texnums[c] = c+next_texture;
		}

		/* this is the size of each image frame */
		imageDatasize = sizeof (GLbyte) * loadparams[texno].x *
                                loadparams[texno].y * loadparams[texno].depth;

		/* and, get a pointer to the whole, unsquished, image data */
		imageptr = loadparams[texno].texdata;

		for (st = 0; st < framecount; st++){
			/* make new table entries for these new textures */
			checkAndAllocTexMemTables(&(texnums[st]),16);

			/* copy most of the elements over from the base, verbatim */

			memcpy (&loadparams[texnums[st]], &loadparams[texno], sizeof (struct loadTexParams));

			/* elements that are different from the "standard" */
			loadparams[texnums[st]].texdata =(unsigned char *) malloc (imageDatasize);
			loadparams[texnums[st]].texture_num = (GLuint *)&texnums[st];
			loadparams[texnums[st]].frames=1;

			/* copy the segment out of the squished data to this pure frame */
			memcpy(loadparams[texnums[st]].texdata, imageptr, imageDatasize);

			new_do_texture(texnums[st]);

			/* and, lets look at the next frame in the squished data */
			imageptr += imageDatasize;

		}
		/* we have unsquished this whole image; lets tell the caller this */
		texIsloaded[texno] = UNSQUASHED;
		loadparams[texno].x = texnums[0];
		loadparams[texno].y = texnums[framecount-1];
		free (loadparams[texno].texdata);
	} else {
		new_do_texture(texno);
		texIsloaded[texno] = LOADED;
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
	
	#ifdef TEXVERBOSE
	printf ("new_do_texture, tex %d, s %d t %d\n",texno, loadparams[texno].Src, loadparams[texno].Trc);
	#endif

	/* Image should be GL_LINEAR for pictures, GL_NEAREST for pixelTs */
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, loadparams[texno].Image);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, loadparams[texno].Image);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, loadparams[texno].Src);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, loadparams[texno].Trc);
	depth = loadparams[texno].depth;
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
			dest = (unsigned char *)malloc((unsigned) (depth) * rx * ry);
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

	param - vrml fields, but translated into GL_TEXTURE_ENV_MODE, GL_MODULATE, etc.
************************************************************************************/

void bind_image(int itype, SV *parenturl, struct Multi_String url,
		GLuint *texture_num, int repeatS, int repeatT, void *param) {

	struct multiTexParams *paramPtr;

	#ifdef TEXVERBOSE 
	printf ("bind_image, textureInProcess %d texture_num %d\n",textureInProcess,*texture_num);
	#endif

	if (textureInProcess > 0) {
		sched_yield();
		/* we are already working on a texture. Is it THIS one? */
		if (textureInProcess != (*texture_num)) {
			#ifdef TEXVERBOSE 
			printf ("bind_image, textureInProcess = %d, texture_num %d returning \n",
				textureInProcess,*texture_num);
			#endif

			return;
		}
		#ifdef TEXVERBOSE 
		printf ("bind_image, textureInProcess == texture_num\n");
		#endif
	}

	/* signal that his is the one we want to work on */
	textureInProcess = *texture_num;



	/* is this the first call for this texture? */
	if (*texture_num==0) {
		*texture_num = next_texture;
		/* signal that his is the one we want to work on */
		textureInProcess = *texture_num;

		next_texture ++;

		/* check to see if "texIsloaded" and "loadparams" is ok
			size-wise. if not,
			make them larger, by 16 */
		checkAndAllocTexMemTables(texture_num, 16);

		glGenTextures(1,&loadparams[*texture_num].genned_texture);
		#ifdef TEXVERBOSE
			printf ("just genned texture %d\n",*texture_num);
		#endif
	}
	#ifdef TEXVERBOSE
		printf ("bind_image, textureInProcess %d status %d\n",textureInProcess,texIsloaded[*texture_num]);
	#endif


	/* check to see if "texIsloaded" and "loadparams" is ok size-wise. if not,
	   make them larger, by 16 */
	checkAndAllocTexMemTables(texture_num, 16);

	/* set the texture depth - required for Material diffuseColor selection */
	last_texture_depth = loadparams[*texture_num].depth;

	/* have we already processed this one before? */
	if (texIsloaded[*texture_num] == LOADED) {
		#ifdef TEXVERBOSE 
		printf ("now binding to pre-bound %d, num %d\n",*texture_num, *texture_num);
		#endif

		/* save the texture params for when we go through the MultiTexture stack */
		texParams[texture_count] = param;

		textureInProcess = -1; /* we have finished the whole process */

		return;
	}

	/* is this one bad? */
	if (texIsloaded[*texture_num] == INVALID) { 
				/* freeTexture(texture_num); */
				textureInProcess = -1;
				return; }

	/* is this one an unsquished movie texture? */
	if (texIsloaded[*texture_num] == UNSQUASHED) { return; }

	#ifndef AQUA
	/* is this one read in, but requiring final manipulation
	 * by THIS thread? */
	if (texIsloaded[*texture_num] == NEEDSBINDING) {
		#ifdef TEXVERBOSE 
		printf ("tex %d needs binding, name %s\n",*texture_num,
				loadparams[*texture_num].filename);
		#endif

		do_possible_textureSequence(*texture_num);
		#ifdef TEXVERBOSE 
		printf ("tex %d now loaded\n",*texture_num);
		#endif

		return;
	}
	#endif

	/* are we loading this one? */
	if (texIsloaded[*texture_num] == LOADING) {
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
		#ifdef TEXVERBOSE
			printf ("currentlyWorkingOn WAS %d ",currentlyWorkingOn);
		#endif

		currentlyWorkingOn = *texture_num;
		#ifdef TEXVERBOSE
			printf ("just set currentlyWorkingOn to %d\n",currentlyWorkingOn);
		#endif
	}
        T_LOCK_SIGNAL
        TUNLOCK

}

/* check to see if we have enough memory for internal Texture tables */
/* we should lock, unless we are already locked- MPG decoding happens when locked */
void checkAndAllocTexMemTables(GLuint *texture_num, int increment) {
	int count;
	int prev_max_texture;

	/* do we have enough room to save the texIsloaded flag for this texture? */
	if ((int)(*texture_num)>=(max_texture-2)) {
		REGENLOCK
		/* printf ("bind_image, must allocate a bunch more space for flags\n"); */
		prev_max_texture = max_texture;
		max_texture+=increment;
		texIsloaded = (unsigned char *)realloc(texIsloaded, sizeof(*texIsloaded) * max_texture);
		loadparams = (struct loadTexParams *)realloc(loadparams, sizeof(*loadparams) * max_texture);

		/* printf ("zeroing from %d to %d\n",prev_max_texture,max_texture); */
		for (count = prev_max_texture; count < (int)max_texture; count++) {
			texIsloaded[count] = NOTLOADED;
			loadparams[count].filename="uninitialized file";
			loadparams[count].depth = 0;
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

int findTextureFile (int cwo, int *istemp) {
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


	STRLEN xx;
	*istemp=FALSE;	/* don't remove this file */

	#ifdef TEXVERBOSE 
	printf ("textureThread:start of findTextureFile for cwo %d \n",cwo);
	#endif
	/* try to find this file. */

	if (loadparams[cwo].type !=PIXELTEXTURE) {
		/* lets make up the path and save it, and make it the global path */
		count = strlen(SvPV(loadparams[cwo].parenturl,xx));
		mypath = (char *)malloc ((sizeof(char)* count)+1);
		filename = (char *)malloc(1000);

		if ((!filename) || (!mypath)) {
			outOfMemory ("texture thread can not malloc for filename\n");
		}

		/* copy the parent path over */
		strcpy (mypath,SvPV(loadparams[cwo].parenturl,xx));

		/* and strip off the file name, leaving any path */
		slashindex = (char *)rindex(mypath,'/');
		if (slashindex != NULL) {
			slashindex ++; /* leave the slash on */
			*slashindex = 0;
		 } else {mypath[0] = 0;}

		/* try the first url, up to the last */
		count = 0;
		while (count < loadparams[cwo].url.n) {
			thisurl = SvPV(loadparams[cwo].url.p[count],xx);

			/* check to make sure we don't overflow */
			if ((strlen(thisurl)+strlen(mypath)) > 900) break;

			/* put the path and the file name together */
			makeAbsoluteFileName(filename,mypath,thisurl);

			#ifdef TEXVERBOSE 
			printf ("textureThread: checking for %s\n", filename);
			#endif

			if (fileExists(filename,firstBytes,TRUE)) { break; }
			count ++;
		}

		if (count != loadparams[cwo].url.n) {
			#ifdef TEXVERBOSE 
				printf ("textureThread: we were successful at locating %s\n",filename); 
			#endif
		} else {
			if (count > 0) {
				printf ("Could not locate URL for texture %d (last choice was %s)\n",cwo,filename);
			}
			/* So, we could not find the correct file. Make this into a blank PixelTexture, so that
			   at least this looks ok on the screen */
			loadparams[cwo].type = PIXELTEXTURE;
			/* really bright PixelTexture! 
			loadparams[cwo].parenturl=EAI_newSVpv("2 4 3 0xff0000 0x00ff00 0xff0000 0x00ff00 0xffff00 0x0000ff 0xffff00 0x0000ff");
			*/
			loadparams[cwo].parenturl=EAI_newSVpv("1 1 3 0x707070");
		}
	}

	/* pixelTextures - lets just make a specific string for this one */
	if (loadparams[cwo].type ==PIXELTEXTURE) {
		int a,b,c;
		char *name;

		#ifdef TEXVERBOSE 
		printf ("textureThread, going to get name \n");
		#endif

		name = SvPV(loadparams[cwo].parenturl,xx);
		filename = (char *)malloc(100);

		#ifdef TEXVERBOSE
		printf ("in find, name %s strlen %d\n",name,strlen(name));
		#endif

		/* make up a checksum name that is unique for PixelTextures - for checking on duplicates */
		b = 0;
		c = strlen(name);
		if (c > 3000) c = 3000; /* lets hope this is unique in 3000 characters */
		for (a=0; a<c; a++) {
			b =  b + (int) (name[a] & 0xff);
		}

		sprintf (filename,"PixelTexture_%d_%d",c,b);
		#ifdef TEXVERBOSE 
		printf ("textureThread, temp name is %s\n",filename);
		#endif
	}

	/* ok, have we seen this one before? */
	#ifdef SEARCHFORDUPLICATETEXTURES
	/* this is commented out, as if the image has different repeatT, repeatS flags, it will
	   not be rendered correctly for the "second" invocation */
	/* maybe this is fixed with texture changes for 1.16.x??? */

	flen = strlen(filename);
	for (count=1; count < max_texture; count++) {
		
		#ifdef TEXVERBOSE
			printf ("textureThread: comparing :%s: :%s: (%d %d) count %d\n",
					filename,
					loadparams[count].filename,
					strlen(filename),
					strlen(loadparams[count].filename),
					count);
		#endif
	

		/* are the names different lengths? */
		if (strlen(loadparams[count].filename) == flen) {
		    if(strncmp(loadparams[count].filename,filename,flen)==0) {
			#ifdef TEXVERBOSE
				printf ("duplicate name %s at %d %d\n",
					ilename,count,cwo);
			#endif

			/* duplicate, make this entry INVALID, and make the
			   filename the same (avoids segfaults in strncmp, above)
			*/

			freeTexture(texnum);
			/* texIsloaded[*texnum]=INVALID; */
			loadparams[*texnum].filename="Duplicate Filename";

			free (filename);

			/* and tell OpenGL to use the previous texture number */
			*texnum=count;
			textureInProcess = count;
			#ifdef TEXVERBOSE 
			printf ("textureThread: duplicate, so setting textureInProcess back to %d\n",count);
			#endif

			return FALSE;
		    }
		}
	}
	#endif

	if (loadparams[cwo].type !=PIXELTEXTURE) {
		/* is this a texture type that is *not* handled internally? */
		if ((strncmp(firstBytes,firstPNG,4) != 0) &&
		    (strncmp(firstBytes,firstJPG,4) != 0) &&
		    (strncmp(firstBytes,firstMPGa,4) != 0) &&
		    (strncmp(firstBytes,firstMPGb,4) != 0)) {
			sysline = (char *)malloc(sizeof(char)*(strlen(filename)+100));
			if (!sysline) {printf ("malloc failure in convert, exiting\n"); exit(1);}
			sprintf(sysline,"%s %s /tmp/freewrl%d.png",
					CONVERT,filename,getpid());
			#ifdef TEXVERBOSE 
				printf ("textureThread: running convert on %s\n",sysline);
			#endif

			if (freewrlSystem (sysline) != 0) {
				printf ("Freewrl: error running convert line %s\n",sysline);
			} else {
				sprintf (filename,"/tmp/freewrl%d.png",getpid());
				*istemp=TRUE;
			}
			free (sysline);
		}
	}

	/* save filename in data structure for later comparisons */
	loadparams[cwo].filename = (char *)malloc(sizeof(char) * strlen(filename)+1);
	strcpy (loadparams[cwo].filename,filename);
	free (filename);
	#ifdef TEXVERBOSE
		printf ("textureThread: new name, save it %d, name %s\n",cwo,loadparams[cwo].filename);
	#endif
	return TRUE;
}

/*************************************************************/
/* _textureThread - work on textures, until the end of time. */


void _textureThread(void) {

	int remove;

	/* printf ("textureThread is %d\n",pthread_self()); */

	#ifdef AQUA
	/* To get this thread to be able to manipulate textures, first, get the 
	   Display attributes */
	CGDirectDisplayID display = CGMainDisplayID ();
	attribs[1] = CGDisplayIDToOpenGLDisplayMask (display);

	/* now, for this thread, create and join OpenGL Contexts */
	CGLChoosePixelFormat (attribs, &pixelFormat, &numPixelFormats);
	CGLCreateContext(pixelFormat, aqglobalContext, &aqtextureContext);

	/* set the context for this thread so that we can share textures with
	   the main context (aqglobalContext) */

	CGLSetCurrentContext(aqtextureContext);
	/* printf ("textureThread, have to try to remember to destroy this context\n"); */
	#endif

	/* we wait forever for the data signal to be sent */
	for (;;) {
		TLOCK
		TextureThreadInitialized = TRUE;
		T_LOCK_WAIT
		REGENLOCK
		texIsloaded[currentlyWorkingOn] = LOADING;
		TextureParsing = TRUE;

		/* look for the file. If one does not exist, or it
		   is a duplicate, just unlock and return */
		#ifdef TEXVERBOSE
			printf ("textureThread, currentlyworking on %d\n",currentlyWorkingOn);
		#endif

		if (findTextureFile(currentlyWorkingOn, &remove)) {
			#ifdef TEXVERBOSE
			printf ("textureThread, findTextureFile ok for %d\n",currentlyWorkingOn);
			#endif


			/* is this a pixeltexture? */
			if (loadparams[currentlyWorkingOn].type==PIXELTEXTURE) {
				__reallyloadPixelTexture();
			} else if (loadparams[currentlyWorkingOn].type==MOVIETEXTURE) {
				__reallyloadMovieTexture();
			} else {
				__reallyloadImageTexture();
			}

			#ifdef TEXVERBOSE
			printf ("textureThread, after reallyLoad for  %d\n",currentlyWorkingOn);
			#endif

			/* check to see if there was an error */
			if (texIsloaded[*loadparams[currentlyWorkingOn].texture_num]!=INVALID) {
				#ifndef AQUA
				texIsloaded[*loadparams[currentlyWorkingOn].texture_num] = NEEDSBINDING;
				#else

				#ifdef TEXVERBOSE 
				printf ("tex %d needs binding, name %s\n",*loadparams[currentlyWorkingOn].texture_num,
					loadparams[*loadparams[currentlyWorkingOn].texture_num].filename);
				#endif

				do_possible_textureSequence(
					*loadparams[currentlyWorkingOn].texture_num);
				#ifdef TEXVERBOSE 
				printf ("tex %d now loaded\n",*loadparams[currentlyWorkingOn].texture_num);
				#endif
				#endif
			}

			/* is this a temporary file? */
			if (remove == 1) {
				unlink (loadparams[currentlyWorkingOn].filename);
			}
		} else {
			texIsloaded[*loadparams[currentlyWorkingOn].texture_num]=INVALID;
		}

		/* signal that we are finished */
		#ifdef TEXVERBOSE
			printf ("textureThread: finished parsing texture for currentlyWorkingOn %d\n",currentlyWorkingOn);
		#endif

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
	long hei,wid,depth;
	long long inval;
	unsigned char *texture;
	char *tptr;
	char *endptr;
	int tctr;
	STRLEN xx;
	int count;
	int ok;


	/* check to see if there really is a PixelTexture there; if there is not,
	   then mark texture INVALID and return. eg, PixelTexture {} will get
	   caught here */

	#ifdef TEXVERBOSE 
	printf ("start of reallyLoadPixelTexture\n");
	#endif

	if ((SvFLAGS(loadparams[currentlyWorkingOn].parenturl) & SVf_POK) == 0) {
		printf ("this one is going to fail\n");
		freeTexture(loadparams[currentlyWorkingOn].texture_num);
		/* texIsloaded[*loadparams[currentlyWorkingOn].texture_num] = INVALID; */
		return;
	}

	/* ok - we have a valid Perl pointer, go for it. */
	tptr = (char *)SvPV(loadparams[currentlyWorkingOn].parenturl,xx);
	/* printf ("PixelTextures, string now is %s\n",tptr);  */

	while (isspace(*tptr))tptr++;
	ok = TRUE;

	/* scan in the width, height, and depth */
	wid = strtol(tptr,&endptr,0);
	if (tptr == endptr) { ok = FALSE; } else { tptr = endptr; }
	hei = strtol(tptr,&endptr,0);
	if (tptr == endptr) { ok = FALSE; } else { tptr = endptr; }
	depth = strtol(tptr,&endptr,0);
	if (tptr == endptr) { ok = FALSE; } else { tptr = endptr; }

	if (ok) {
		if ((depth < 1) || (depth >4)) {
			printf ("PixelTexture, depth %d out of range, assuming 1\n",(int) depth);
			depth = 1;
		}
		/* have header ok, now read in all values */
		count = 0; tctr = 0;

		texture = (unsigned char *)malloc (wid*hei*4);

		while (count < (int)(wid*hei)) {
			inval = strtoll(tptr,&endptr,0);
			if (tptr == endptr) {
				printf("PixelTexture: expected %d pixels, got %d\n",(int)(wid*hei),count);
				freeTexture(loadparams[currentlyWorkingOn].texture_num);
				/* texIsloaded[*loadparams[currentlyWorkingOn].texture_num] = INVALID; */
				break;
			} else { tptr = endptr; }

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
					   texture[tctr++] = (inval>>16) & 0xff; /*R*/
					   texture[tctr++] = (inval>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (inval>>0) & 0xff; /*B*/
					   break;
				   }
				case 4: {
					   texture[tctr++] = (inval>>24) & 0xff; /*R*/
					   texture[tctr++] = (inval>>16) & 0xff; /*G*/
					   texture[tctr++] = (inval>>8) & 0xff;	 /*B*/
					   texture[tctr++] = (inval>>0) & 0xff; /*A*/
					   /* printf ("verify, %x %x %x %x\n",texture[tctr-4],texture[tctr-3],
						texture[tctr-2],texture[tctr-1]); */
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
		freeTexture(loadparams[currentlyWorkingOn].texture_num);
		/*texIsloaded[*loadparams[currentlyWorkingOn].texture_num] = INVALID; */
	}

	#ifdef TEXVERBOSE 
	printf ("end of reallyloadPixelTextures\n");
	#endif
}

/*********************************************************************************************/

/*
 * JPEG ERROR HANDLING: code from
 * http://courses.cs.deu.edu.tr/cse566/newpage2.htm
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr {
	struct jpeg_error_mgr pub;    /* "public" fields */
	jmp_buf setjmp_buffer; /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  	my_error_ptr myerr = (my_error_ptr) cinfo->err;

 	/* Always display the message. */
  	/* We could postpone this until after returning, if we chose. */
  	/* JAS (*cinfo->err->output_message) (cinfo); */

 	/* Return control to the setjmp point */
  	longjmp(myerr->setjmp_buffer, 1);
}

/*********************************************************************************************/

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
	struct my_error_mgr jerr;
	JDIMENSION nrows;
	JSAMPROW row = 0;
	JSAMPROW rowptr[1];
	unsigned rowcount, columncount;
	int dp;

	int tempInt;


	filename = loadparams[currentlyWorkingOn].filename;

	infile = fopen(filename,"r");

	if ((rc = readpng_init(infile, &image_width, &image_height)) != 0) {
		/* it is not a png file - assume a jpeg file */
		/* start from the beginning again */
		rewind (infile);

		/* Select recommended processing options for quick-and-dirty output. */
		cinfo.two_pass_quantize = FALSE;
		cinfo.dither_mode = JDITHER_ORDERED;
		cinfo.desired_number_of_colors = 216;
		cinfo.dct_method = JDCT_FASTEST;
		cinfo.do_fancy_upsampling = FALSE;

		/* call my error handler if there is an error */
		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = my_error_exit;
		if (setjmp(jerr.setjmp_buffer)) {
			/* if we are here, we have a JPEG error */
			printf ("FreeWRL Image problem - could not read %s\n", filename);
			jpeg_destroy_compress((j_compress_ptr)&cinfo);
			fclose (infile);
			freeTexture(&texture_num);
			/* texIsloaded[texture_num] = INVALID; */
			return;
		}


		jpeg_create_decompress(&cinfo);

		/* Specify data source for decompression */
		jpeg_stdio_src(&cinfo, infile);

		/* Read file header, set default decompression parameters */
		/* (void) jpeg_read_header(&cinfo, TRUE); */
		tempInt = jpeg_read_header(&cinfo, TRUE);


		/* Start decompressor */
		(void) jpeg_start_decompress(&cinfo);



		row = (JSAMPLE*)malloc(cinfo.output_width * sizeof(JSAMPLE)*cinfo.output_components);
		rowptr[0] = row;
		image_data = (unsigned char *)malloc(cinfo.output_width * sizeof (JSAMPLE) * cinfo.output_height * cinfo.output_components);
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
			/* texIsloaded[texture_num] = INVALID; */
			freeTexture(&texture_num);
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
		freeTexture(&texture_num);
		/* texIsloaded[texture_num] = INVALID; */
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
	void *ptr;
	int firstTex;

	firstTex = *loadparams[currentlyWorkingOn].texture_num;
	ptr=NULL;

	/* now, generate a new first texture */

	mpg_main(loadparams[currentlyWorkingOn].filename,
		&x,&y,&depth,&frameCount,&ptr);


	/* store the "generic" data */
	store_tex_info(currentlyWorkingOn,
		(int)depth,(int)x,(int)y,(unsigned char *)ptr,
		((loadparams[currentlyWorkingOn].repeatS)) ? GL_REPEAT : GL_CLAMP,
		((loadparams[currentlyWorkingOn].repeatT)) ? GL_REPEAT : GL_CLAMP,
		GL_NEAREST);

	/* now, for the mpeg specific data */
	loadparams[currentlyWorkingOn].frames = frameCount;
}

