/* 
 * Copyright(C) 1998 Tuomas J. Lukka, 2001 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#define XRES 96
#define YRES 96
#define PPI 72
#define PIXELSIZE 1
#define POINTSIZE 50

#define OUT2GL(a,i) (size * (0.0 +a) / ((1.0*(font_face[i]->height)) / PPI*XRES))

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "../OpenGL/OpenGL.m"

D_OPENGL;

#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


/* initialize the library with this variable */
FT_Library library; /* handle to library */

#define num_fonts 32
FT_Face font_face[num_fonts];		/* handle to face object */
int	font_opened[num_fonts];		/* is this font opened   */


/* we load so many gliphs into an array for processing */
#define 	MAX_GLYPHS	2048
int		num_glyphs;
FT_Glyph	glyphs[MAX_GLYPHS];
int		cur_glyph;
int		verbose = 0;



/* lets store the font paths here */
#define fp_len 128
#define fp_name_len 140
char sys_fp[fp_len];
char fw_fp[fp_len];
char thisfontname[fp_name_len];

/* where are we? */
FT_Vector pen;

/* are we initialized yet */
int initialized = FALSE;


GLUtriangulatorObj *triang;

static GLdouble vecs[3*10000];


/* routines for the tesselation callbacks */
static void FW_beg(GLenum e) {
	if(verbose) printf("BEGIN %d\n",e);
	glBegin(e);
}

static void FW_end() {
	if(verbose) printf("END\n");
	glEnd();
}

static void FW_ver(void *p) {
	GLdouble *dp = p;
	if(verbose) printf("V: %f %f %f\n",dp[0],dp[1],dp[2]);
	glVertex3f(dp[0],dp[1],dp[2]);
}

static void FW_err(GLenum e) {
	/* if(verbose) */ printf("FreeWRL Text error %d: '%s'\n",e,gluErrorString(e));
}

/* make up the font name */
FW_make_fontname (int num, char *name) {
	int i;

/*
                        bit:    0       BOLD        (boolean)
                        bit:    1       ITALIC      (boolean)
                        bit:    2       SERIF
                        bit:    3       SANS
                        bit:    4       TYPEWRITER
*/

	if (num == 0) {
		strcpy (thisfontname,fw_fp);
		strcat (thisfontname,"/baklava.ttf");
	} else {
		strcpy (thisfontname,sys_fp);

		switch (num) {
			/* Serif, norm, bold, italic, bold italic */
			case 0x04: strcat (thisfontname,"/Amrigon.ttf"); break;
			case 0x05: strcat (thisfontname,"/Amrigob.ttf"); break;
			case 0x06: strcat (thisfontname,"/Amrigoi.ttf"); break;
			case 0x07: strcat (thisfontname,"/Amrigobi.ttf"); break;

			/* Sans, norm, bold, italic, bold italic */
			case 0x08: strcat (thisfontname,"/Baubodn.ttf"); break;
			case 0x09: strcat (thisfontname,"/Baubodi.ttf"); break;
			case 0x0a: strcat (thisfontname,"/Baubodn.ttf"); break;
			case 0x0b: strcat (thisfontname,"/Baubodbi.ttf"); break;

			/* Typewriter, norm, bold, italic, bold italic */
			case 0x10: strcat (thisfontname,"/Futuran.ttf"); break;
			case 0x11: strcat (thisfontname,"/Futurabi.ttf"); break;
			case 0x12: strcat (thisfontname,"/Futurab.ttf"); break;
			case 0x13: strcat (thisfontname,"/Futurabi.ttf"); break;

			default: printf ("dont know how to handle font id %x\n",num);
		}
	}
}



/* initialize the freetype library */
int FW_init_face(int num, char *name) {
	int err;

	/* load a font face */
	err = FT_New_Face(library, name, 0, &font_face[num]);
	if (err) {
		printf ("FreeType - can not use font %s\n",name);
		return FALSE;
	} else {
		/* access face content */
		err = FT_Set_Char_Size (font_face[num],	/* handle to face object 	*/
				0,	/* char width in 1/64th of points */
				16*64,	/* char height in 1/64th of points */
				XRES,	/* horiz device resolution	*/
				YRES	/* vert device resolution	*/
				);

		if (err) { 
			printf ("FreeWRL - FreeType, can not set char size for font %s\n",name);
			return FALSE;
		} else {
			font_opened[num] = TRUE;
		}
	}
	return TRUE;
}

/* calculate extent of a range of characters */
double FW_extent (int start, int length) {
	int count;
	double ret = 0;

	for (count = start; count <length; count++) {
		ret += glyphs[count]->advance.x;
	}
	printf ("FW_Extent returning %lf\n",ret);
	return ret;
}



/* Load a character, a maximum of MAX_GLYPHS are here. Note that no 
   line formatting is done here; this is just for pre-calculating
   extents, etc.

   NOTE: we store the handles to each glyph object for each
   character in the glyphs array
*/
FT_Error  FW_Load_Char(int num, int idx) {
	FT_Glyph  glyph;
	FT_UInt glyph_index;
	int error;

	if (cur_glyph >= MAX_GLYPHS) {
		return 1;
	}

	/* retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index(font_face[num],idx);

	/* loads the glyph in the glyph slot */

	error = FT_Load_Glyph( font_face[num], glyph_index, FT_LOAD_DEFAULT ) ||
		FT_Get_Glyph ( font_face[num]->glyph, &glyph );
	if (!error) { glyphs[cur_glyph++] = glyph; }
	return error;
}



void FW_draw_outline (int myff, FT_OutlineGlyph oglyph, float size) {
	int nthvec = 0;
	int contour; int point;
	int flag,x,y = 0;
        GLdouble *vlast;
	int flaglast;
	GLdouble v[3];
	GLdouble *v2;
	GLdouble *vnew;

	/* lets do the stuff to equate truetype and type1 fonts */
	if (font_face[myff]->units_per_EM != 1000) 
		size = size * font_face[myff]->units_per_EM/1000.0;

	gluBeginPolygon(triang);
	if (verbose) printf("Contours: %d\n",oglyph->outline.contours);

	for(contour = 0; contour < oglyph->outline.n_contours; contour++) {
		if (verbose) 
			printf ("FW_draw_character, contour %d of %d\n",contour, oglyph->outline.n_contours);

		vlast = 0;
		flaglast = 0;
		if(contour) { gluNextContour(triang,GLU_UNKNOWN); }

		if (verbose) printf("End %d: %d\n", contour, oglyph->outline.contours[contour]);

		for(point = (contour ? oglyph->outline.contours[contour-1]+1 : 0); 
		   		point <= oglyph->outline.contours[contour];
		   		point ++) {



			float x = OUT2GL(oglyph->outline.points[point].x+pen.x,myff);
			float y = (0.0 + OUT2GL(oglyph->outline.points[point].y,myff) + pen.y);

			flag = oglyph->outline.tags[point];
			v[0] = x; v[1] = y; v[2] = 0;
			v2 = vecs+3*(nthvec++);
			if(nthvec >= 10000) {
				die("Too large letters");
			}
			v2[0] = v[0]; v2[1] = v[1]; v2[2] = v[2];
			if(vlast &&
				   v2[0] == vlast[0] &&
				   v2[1] == vlast[1] &&
				   v2[2] == vlast[2]) {
					continue;
			}
			if (verbose)
				 printf("OX, OY: %f, %f, X,Y: %f,%f FLAG %d\n",
						oglyph->outline.points[point].x+0.0,
						oglyph->outline.points[point].y+0.0,x,y,flag);
			if(flag) {
				gluTessVertex(triang,v2,v2);
			} else {
				if(!vlast) {
					die("Can't be first off");
					if(flaglast) {
						/* Interp */
						vnew = vecs+3*(nthvec++);
						if(nthvec >= 10000) {
							die("Too large letters2");
						}
						vnew[0] = 0.5*(v2[0]+vlast[0]);
						vnew[1] = 0.5*(v2[1]+vlast[1]);
						vnew[2] = 0.5*(v2[2]+vlast[2]);
						gluTessVertex(triang,vnew,vnew);
					}
				}
			}
			vlast = v2;
			flaglast = flag;
		}
	}
	gluEndPolygon(triang);
}

/* draw a glyph object */
FW_draw_character (int myff, FT_Glyph glyph, float size) {

	if (glyph->format == ft_glyph_format_outline) {
		FW_draw_outline (myff, (FT_OutlineGlyph) glyph,size);

		/* old type1 fonts, lets scale the pen movement appropriately */
		if (font_face[myff]->units_per_EM == 1000) 
			size = size * 1.7;
		pen.x +=  size * (glyph->advance.x >> 10);
	} else {
		printf ("FW_draw_character; glyphformat  -- need outline for %s %s\n",
			font_face[myff]->family_name,font_face[myff]->style_name); 
	}
}





static void FW_rendertext(int n,SV **p,int nl, float *length, 
		float maxext, float spacing, float size, unsigned int fsparam) {
	char *str;
	int i,gindex,row;
	int contour; int point;
	int err;
	float shrink = 0;
	float rshrink = 0;
	int flag;
	int counter=0;
	int myff;

	/* fsparam has the following bitmaps:
	
			bit:	0	horizontal  (boolean)
			bit:	1	leftToRight (boolean)
			bit:	2	topToBottom (boolean)
			  (style)
			bit:	3	BOLD	    (boolean)
			bit:	4	ITALIC	    (boolean)
			  (family)
			bit:	5	SERIF
			bit:	6	SANS	
			bit:	7	TYPEWRITER
			bit:	8 	indicates exact font pointer (future use)
			  (Justify - major)
			bit:	9	FIRST
			bit:	10	BEGIN
			bit:	11	MIDDLE
			bit:	12	END
			  (Justify - minor)
			bit:	13	FIRST
			bit:	14	BEGIN
			bit:	15	MIDDLE
			bit:	16	END

			bit: 17-31	spare
	*/

	/* have we done any rendering yet */

	if (verbose) printf ("entering FW_Render_text initialized %d\n",initialized);

	pen.x = 0; pen.y = 0;
	cur_glyph = 0;

	if (!initialized) {
		if(err = FT_Init_FreeType(&library))
		  die("FreeWRL FreeType Initialize error %d\n",err);
		initialized = TRUE;
	}

	/* is this font opened */
	myff = (fsparam >> 3) & 0x1F;
	if (myff <4) {
		/* we dont yet allow externally specified fonts, so one of
		   the font style bits HAS to be set */	
		printf ("FreeWRL - Warning - FontStyle funny - setting to SERIF\n");
		myff = 4;
	}

	if (!font_opened[myff]) {
		FW_make_fontname(myff,thisfontname);
		if (!FW_init_face(myff,thisfontname)) {
			/* tell this to render as fw internal font */
			FW_make_fontname (0,thisfontname);
			FW_init_face(myff,thisfontname);
		}
	}

	glNormal3f(0,0,-1);
	glEnable(GL_LIGHTING);

	/* load all of the characters first... */
	for (row=0; row<n; row++) {
		str = SvPV(p[row],PL_na);
		for(i=0; i<strlen(str); i++) {
			FW_Load_Char(myff,str[i]);
		}
	}

	if(maxext > 0) {
	   double maxlen = 0;
	   double l;
	   int counter = 0;
	   for(row = 0; row < n; row++) {
		str = SvPV(p[row],PL_na);
		printf ("text: %s\n",str);
		l = FW_extent(counter,strlen(str));
		counter += strlen(str);
		if(l > maxlen) {maxlen = l;}
	   }

	   if(maxlen > maxext) {shrink = maxext / OUT2GL(maxlen,myff);}
	}

	pen.y = 0;
	for(row = 0; row < n; row++) {
	   	double l;

	   	str = SvPV(p[row],PL_na);
		if (verbose) printf ("text2 row %d :%s:\n",row, str);
	        pen.x = 0;
		rshrink = 0;
		if(row < nl && length[row]) {
			l = FW_extent(counter,strlen(str));
			rshrink = length[row] / OUT2GL(l,myff);
		}

		if(shrink) { glScalef(shrink,1,1); }
		if(rshrink) { glScalef(rshrink,1,1); }

		for(i=0; i<strlen(str); i++) {
			FT_UInt glyph_index;
			int error;
	
			FW_draw_character (myff, glyphs[counter+i],size);
			FT_Done_Glyph (glyphs[counter+i]);
		}
		counter += strlen(str);
		pen.y -= 1.0;  /* row increment */
   	}
	if (verbose) printf ("exiting FW_Render_text\n");
}



MODULE=VRML::Text 	PACKAGE=VRML::Text

PROTOTYPES: ENABLE


void *
get_rendptr()
CODE:
	RETVAL = (void *)FW_rendertext;
OUTPUT:
	RETVAL




int
open_font(sys_path, fw_path)
char *sys_path
char *fw_path
CODE:
	{
	int len;

	if (verbose) printf ("open_font called\n");
	/* copy over font paths */
	if (strlen(sys_path) < fp_len) {
		strcpy (sys_fp,sys_path);
	} else { 
		printf ("FreeWRL - System font path in vrml.conf too long:\n\t%s\n",sys_path);
		sys_fp[0] = 0;
	}

	if (strlen(fw_path) < fp_len) {
		strcpy (fw_fp,fw_path);
	} else { 
		printf ("FreeWRL - internal font path in vrml.conf too long:\n\t%s\n",fw_path);
		fw_fp[0] = 0;
	}

	/* register tesselation callbacks for OpenGL calls */
	triang = gluNewTess();
	gluTessCallback(triang, GLU_BEGIN, FW_beg);
	gluTessCallback(triang, GLU_VERTEX, FW_ver);
	gluTessCallback(triang, GLU_END, FW_end);
	gluTessCallback(triang, GLU_ERROR, FW_err);


	/* lets initialize some things */
	for (len = 0; len < num_fonts; len++) {
		font_opened[len] = FALSE;
	}

	}


BOOT:
	{
	I_OPENGL;
	}



