/* 
 * Copyright(C) 1998 Tuomas J. Lukka, 2001, 2002 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#define XRES 96
#define YRES 96
#define PPI 72
#define MAXVECS 200
#define POINTSIZE 20


#define TOPTOBOTTOM (fsparam & 0x04)
#define LEFTTORIGHT (!(fsparam & 0x02))

#define OUT2GL(a) (x_size * (0.0 +a) / ((1.0*(font_face[myff]->height)) / PPI*XRES))

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "../OpenGL/OpenGL.m"

D_OPENGL;

#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


/* spline calculations */
#define CONIC_STEPS	4
#define	DELTA  (float) 1.0/CONIC_STEPS
#define DELTA2 (float) DELTA * DELTA
#define DELTA3 (float) DELTA * DELTA * DELTA


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


/* decompose interface func pointer */
FT_Outline_Funcs FW_outline_interface;


/* lets store the font paths here */
#define fp_len 128
#define fp_name_len 140
char sys_fp[fp_len];
char fw_fp[fp_len];
char thisfontname[fp_name_len];

/* where are we? */
float pen_x, pen_y;


float x_size;		// size of chars from file
float y_size;		// size of chars from file
int   myff;		// which index into font_face are we using 

/* are we initialized yet */
int initialized = FALSE;


GLUtriangulatorObj *triang;



/* Outline callbacks and global vars */
int contour_started = FALSE;
FT_Vector last_point;
int FW_Vertex;
static GLdouble vecs[3*MAXVECS];

int FW_moveto ( FT_Vector* to, void* user) {


	/* Have we started a new line */
	if (contour_started) {
		gluNextContour(triang,GLU_UNKNOWN);
	} 

	/* well if not, tell us that we have started one */
	contour_started = TRUE;

	last_point.x = to->x; last_point.y = to->y;

	if (verbose) 
		printf ("moveto tox %ld toy %ld\n",to->x, to->y);

    return 0;
}

int FW_lineto ( FT_Vector* to, void* user) {

	GLdouble *v2;

	if ((last_point.x == to->x) && (last_point.y == to->y)) {
		// printf ("FW_lineto, early return\n");
		return 0;
	}

	last_point.x = to->x; last_point.y = to->y;

	v2 = vecs+3*(FW_Vertex++);
	v2[0] = OUT2GL(last_point.x + pen_x);
	v2[1] = OUT2GL(last_point.y) + pen_y;
	v2[2] = 0.0;

	
	gluTessVertex(triang,v2,v2);

	if (verbose) {
		printf ("lineto, going to %d %d\n",to->x, to->y);
		printf ("gluTessVertex %f %f %f \n", v2[0],v2[1],v2[2]);
	}

    return 0;
  }

int FW_conicto ( FT_Vector* control, FT_Vector* to, void* user) {

	/* Ok, we could make some really pretty characters, but
	   that would take up triangles, something that is bad for
	   speed. This shortcut seems to work quite well */

	if (verbose)
		printf ("FW_conicto\n");

	FW_lineto (control,user);
	FW_lineto (to,user);

	return 0;
}

int FW_cubicto ( FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* user) {
	GLdouble *v2;

	/* really ignore control points */
	if (verbose)
		printf ("FW_cubicto\n");

	FW_lineto (control1, user);
	FW_lineto (control2, user);
	FW_lineto (to, user);
	return 0;
}


/* make up the font name */
FW_make_fontname (int num) {
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
			case 0x09: strcat (thisfontname,"/Baubodn.ttf"); break;
			case 0x0a: strcat (thisfontname,"/Baubodi.ttf"); break;
			case 0x0b: strcat (thisfontname,"/Baubodbi.ttf"); break;

			/* Typewriter, norm, bold, italic, bold italic */
			case 0x10: strcat (thisfontname,"/Futuran.ttf"); break;
			case 0x11: strcat (thisfontname,"/Futurab.ttf"); break;
			case 0x12: strcat (thisfontname,"/Futurabi.ttf"); break;
			case 0x13: strcat (thisfontname,"/Futurabi.ttf"); break;

			default: printf ("dont know how to handle font id %x\n",num);
		}
	}
	// printf("made %s\n",thisfontname);
}



/* initialize the freetype library */
int FW_init_face() {
	int err;

	/* load a font face */
	err = FT_New_Face(library, thisfontname, 0, &font_face[myff]);
	if (err) {
		printf ("FreeType - can not use font %s\n",thisfontname);
		return FALSE;
	} else {
		/* access face content */
		err = FT_Set_Char_Size (font_face[myff],/* handle to face object 	*/
				POINTSIZE*64,	/* char width in 1/64th of points */
				POINTSIZE*64,	/* char height in 1/64th of points */
				XRES,	/* horiz device resolution	*/
				YRES	/* vert device resolution	*/
				);

		if (err) { 
			printf ("FreeWRL - FreeType, can not set char size for font %s\n",thisfontname);
			return FALSE;
		} else {
			font_opened[myff] = TRUE;
		}
	}
	return TRUE;
}

/* calculate extent of a range of characters */
float FW_extent (int start, int length) {
	int count;
	float ret = 0;

	for (count = start; count <length; count++) {
		ret += glyphs[count]->advance.x >> 10;
	}
	return ret;
}



/* Load a character, a maximum of MAX_GLYPHS are here. Note that no 
   line formatting is done here; this is just for pre-calculating
   extents, etc.

   NOTE: we store the handles to each glyph object for each
   character in the glyphs array
*/
FT_Error  FW_Load_Char(int idx) {
	FT_Glyph  glyph;
	FT_UInt glyph_index;
	int error;

	if (cur_glyph >= MAX_GLYPHS) {
		return 1;
	}

	/* retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index(font_face[myff],idx);

	/* loads the glyph in the glyph slot */

	error = FT_Load_Glyph( font_face[myff], glyph_index, FT_LOAD_DEFAULT ) ||
		FT_Get_Glyph ( font_face[myff]->glyph, &glyph );
	if (!error) { glyphs[cur_glyph++] = glyph; }
	return error;
}


void FW_draw_outline (FT_OutlineGlyph oglyph) {
	int thisptr;
	int retval;

	gluBeginPolygon(triang);
	FW_Vertex = 0;

	/* thisptr may possibly be null; I dont think it is use in freetype */
	retval = FT_Outline_Decompose (&oglyph->outline, &FW_outline_interface, &thisptr);

	if (contour_started) {
		// glEnd();
	}

	gluEndPolygon(triang);

	if (retval != FT_Err_Ok) printf ("FT_Outline_Decompose, error %d\n");	
}


/* draw a glyph object */
FW_draw_character (FT_Glyph glyph) {

	if (glyph->format == ft_glyph_format_outline) {
		FW_draw_outline ((FT_OutlineGlyph) glyph);
		pen_x +=  (glyph->advance.x >> 10);
	} else {
		printf ("FW_draw_character; glyphformat  -- need outline for %s %s\n",
			font_face[myff]->family_name,font_face[myff]->style_name); 
	}
}





static void FW_rendertext(int n,SV **p,int nl, float *length, 
		float maxext, float spacing, float mysize, unsigned int fsparam) {
	char *str;
	int i,gindex,row;
	int contour; int point;
	int err;
	float shrink = 0;
	float rshrink = 0;
	int flag;
	int counter=0;

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

	pen_x = 0.0; pen_y = 0.0;
	cur_glyph = 0;
	x_size = mysize;		// global variable for size
	y_size = mysize;		// global variable for size

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
		FW_make_fontname(myff);
		if (!FW_init_face()) {
			/* tell this to render as fw internal font */
			FW_make_fontname (0);
			FW_init_face();
		}
	}

	/* type 1 fonts different than truetype fonts */
        if (font_face[myff]->units_per_EM != 1000)
              x_size = x_size * font_face[myff]->units_per_EM/1000.0;


	glNormal3f(0,0,-1);
	glEnable(GL_LIGHTING);

	/* load all of the characters first... */
	for (row=0; row<n; row++) {
		str = SvPV(p[row],PL_na);
		for(i=0; i<strlen(str); i++) {
			FW_Load_Char(str[i]);
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

	   if(maxlen > maxext) {shrink = maxext / OUT2GL(maxlen);}
	}

	/* topToBottom */
	if (TOPTOBOTTOM) {
		spacing =  -spacing;  /* row increment */
		pen_y = 0.0;
	} else {
		pen_y -= n-1;
	}


	/* leftToRight */
	if (LEFTTORIGHT) {
		glRotatef (180,0,1,0);
	}


	for(row = 0; row < n; row++) {
	   	float rowlen;

	   	str = SvPV(p[row],PL_na);
		if (verbose) printf ("text2 row %d :%s:\n",row, str);
	        pen_x = 0.0;
		rshrink = 0;
		rowlen = FW_extent(counter,strlen(str));
		if(row < nl && length[row]) {
			rshrink = length[row] / OUT2GL(rowlen);
		}
		if(shrink) { glScalef(shrink,1,1); }
		if(rshrink) { glScalef(rshrink,1,1); }


		/* Justify, FIRST, BEGIN, MIDDLE and END */

		/* MIDDLE */
		if (fsparam & 0x800) { pen_x = -rowlen/2.0; }

		/* END */
		if ((fsparam & 0x1000) & (fsparam & 0x01)) {
			printf ("rowlen is %f\n",rowlen);
			pen_x = -rowlen;
		}


		for(i=0; i<strlen(str); i++) {
			FT_UInt glyph_index;
			int error;
	
			FW_draw_character (glyphs[counter+i]);
			FT_Done_Glyph (glyphs[counter+i]);
		}
		counter += strlen(str);

		pen_y += spacing * y_size;
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
open_font(sys_path, fw_path, OpenGLtriangulator)
char *sys_path
char *fw_path
unsigned int OpenGLtriangulator;
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
	triang = (GLUtriangulatorObj *)OpenGLtriangulator;

	FW_outline_interface.move_to = (FT_Outline_MoveTo_Func)FW_moveto;
	FW_outline_interface.line_to = (FT_Outline_LineTo_Func)FW_lineto;
	FW_outline_interface.conic_to = (FT_Outline_ConicTo_Func)FW_conicto;
	FW_outline_interface.cubic_to = (FT_Outline_CubicTo_Func)FW_cubicto;
	FW_outline_interface.shift = 0;
	FW_outline_interface.delta = 0;
	


	/* lets initialize some things */
	for (len = 0; len < num_fonts; len++) {
		font_opened[len] = FALSE;
	}

	}


BOOT:
	{
	I_OPENGL;
	}



