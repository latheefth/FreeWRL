/* 
 * Copyright(C) 1998 Tuomas J. Lukka, 2001, 2002 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
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


#define XRES 96
#define YRES 96
#define PPI 72
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
int		TextVerbose = 0;


/* decompose interface func pointer */
FT_Outline_Funcs FW_outline_interface;


/* lets store the font paths here */
char sys_fp[fp_name_len];
char thisfontname[fp_name_len];

/* where are we? */
float pen_x, pen_y;


float x_size;		// size of chars from file
float y_size;		// size of chars from file
int   myff;		// which index into font_face are we using 


/* for keeping track of tesselated points */
int relative_index[500];	/* pointer to which point is returned by tesselator  */
int relindx = 0;		/* index into relative_index			     */
struct VRML_PolyRep *rep_;	/* this is the internal rep of the polyrep	     */
int point_count=0;		/* how many points used so far? maps into rep-_coord */
int indx_count=0;		/* maps intp rep_->cindex			     */
int coordmaxsize;		/* maximum coords before needing to realloc	     */
int cindexmaxsize;		/* maximum cindexes before needing to realloc        */


/* Outline callbacks and global vars */
int contour_started = FALSE;
FT_Vector last_point;
int FW_Vertex;

int FW_moveto ( FT_Vector* to, void* user) {


	/* Have we started a new line */
	if (contour_started) {
		gluNextContour(global_tessobj,GLU_UNKNOWN);
	} 

	/* well if not, tell us that we have started one */
	contour_started = TRUE;

	last_point.x = to->x; last_point.y = to->y;

	if (TextVerbose) 
		printf ("moveto tox %ld toy %ld\n",to->x, to->y);

    return 0;
}

int FW_lineto ( FT_Vector* to, void* user) {

	GLdouble v2[3];

	if ((last_point.x == to->x) && (last_point.y == to->y)) {
		// printf ("FW_lineto, early return\n");
		return 0;
	}

	last_point.x = to->x; last_point.y = to->y;

	rep_->coord[point_count*3+0] = OUT2GL(last_point.x + pen_x);
	rep_->coord[point_count*3+1] = OUT2GL(last_point.y) + pen_y;
	rep_->coord[point_count*3+2] = 0.0;

	/* the following should NEVER happen.... */
	if (relindx >500) { printf ("Text, relative index too small\n");exit(1);}

	relative_index[relindx]=point_count;
	v2[0]=rep_->coord[point_count*3+0];
	v2[1]=rep_->coord[point_count*3+1];
	v2[2]=rep_->coord[point_count*3+2];

	gluTessVertex(global_tessobj,v2,&relative_index[relindx]);

	if (TextVerbose) {
		printf ("lineto, going to %d %d\n",to->x, to->y);
		printf ("gluTessVertex %f %f %f index %d\n", 
				rep_->coord[point_count*3+0],
				rep_->coord[point_count*3+1],
				rep_->coord[point_count*3+1],
				relindx);
	}
	point_count++;
	relindx++;

	if (point_count >= coordmaxsize) {
		coordmaxsize+=800;
		rep_->coord = realloc(rep_->coord, sizeof(*(rep_->coord))*coordmaxsize*3);

		if (!(rep_->coord)) { 
			printf ("realloc failed - out of memory \n");
			exit(1);
		}
	}

    return 0;
  }

int FW_conicto ( FT_Vector* control, FT_Vector* to, void* user) {

	/* Ok, we could make some really pretty characters, but
	   that would take up global_tessobjles, something that is bad for
	   speed. This shortcut seems to work quite well */

	if (TextVerbose)
		printf ("FW_conicto\n");

	FW_lineto (control,user);
	FW_lineto (to,user);

	return 0;
}

int FW_cubicto ( FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* user) {
	GLdouble *v2;

	/* really ignore control points */
	if (TextVerbose)
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

	gluBeginPolygon(global_tessobj);
	FW_Vertex = 0;

	/* thisptr may possibly be null; I dont think it is use in freetype */
	retval = FT_Outline_Decompose (&oglyph->outline, &FW_outline_interface, &thisptr);

	if (contour_started) {
		// glEnd();
	}

	gluEndPolygon(global_tessobj);

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





void FW_rendertext(int n,SV **p,int nl, float *length, 
		float maxext, float spacing, float mysize, unsigned int fsparam,
		struct VRML_PolyRep *rp) {
	char *str;
	int i,gindex,row;
	int contour; int point;
	int err;
	float shrink = 0;
	float rshrink = 0;
	int flag;
	int counter=0;
	int char_count=0;
	int est_tri=0;
	

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

	if (TextVerbose) printf ("entering FW_Render_text \n");

	rep_ = rp;

	pen_x = 0.0; pen_y = 0.0;
	cur_glyph = 0;
	x_size = mysize;		// global variable for size
	y_size = mysize;		// global variable for size


	/* is this font opened */
	myff = (fsparam >> 3) & 0x1F;
	if (myff <4) {
		/* we dont yet allow externally specified fonts, so one of
		   the font style bits HAS to be set. If there was no FontStyle
		   node, this will be blank, so... */
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


	/* load all of the characters first... */
	for (row=0; row<n; row++) {
		str = SvPV(p[row],PL_na);
		for(i=0; i<strlen(str); i++) {
			FW_Load_Char(str[i]);
			char_count++;
		}
	}

	/* what is the estimated number of triangles? assume a certain number of tris per char */
	est_tri = char_count*150;
	coordmaxsize=est_tri;
	cindexmaxsize=est_tri;
	rep_->cindex=malloc(sizeof(*(rep_->cindex))*est_tri);
	rep_->coord = malloc(sizeof(*(rep_->coord))*est_tri*3);
	if (!(rep_->coord && rep_->cindex)) {
		printf ("can not malloc memory for text triangles\n");
		exit(1);
	}

	
	if(maxext > 0) {
	   double maxlen = 0;
	   double l;
	   int counter = 0;
	   for(row = 0; row < n; row++) {
		str = SvPV(p[row],PL_na);
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
		if (TextVerbose) 
				printf ("text2 row %d :%s:\n",row, str);
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
			//printf ("rowlen is %f\n",rowlen);
			pen_x = -rowlen;
		}


		for(i=0; i<strlen(str); i++) {
			FT_UInt glyph_index;
			int error;
			int x;

			global_IFS_Coord_count = 0;
			relindx = 0;

			FW_draw_character (glyphs[counter+i]);
			FT_Done_Glyph (glyphs[counter+i]);

			/* copy over the tesselated coords for the character to
			 * the rep structure */

			for (x=0; x<global_IFS_Coord_count; x++) {
				rep_->cindex[indx_count++] = global_IFS_Coords[x];
			}

			if (indx_count > (cindexmaxsize-400)) {
				cindexmaxsize +=500;
				rep_->cindex=realloc(rep_->cindex,sizeof(*(rep_->cindex))*cindexmaxsize);
				if (!(rep_->cindex)) {
					printf ("out of memory at realloc for cindex\n");
					exit(1);
				}
			}
		}
		counter += strlen(str);

		pen_y += spacing * y_size;
   	}
	/* save the triangle count (note, we have a "vertex count", not a "triangle count" */
	rep_->ntri=indx_count/3;

	//printf ("cindex indexes, mallocd %d, used %d\n",cindexmaxsize,indx_count);
	//printf ("coord indexes, mallocd %d   used %d\n",coordmaxsize,point_count);
	realloc (rep_->cindex,sizeof(*(rep_->cindex))*indx_count);
	realloc (rep_->coord,sizeof(*(rep_->coord))*point_count*3);

	/* now, generate normals */
	rep_->normal = malloc(sizeof(*(rep_->normal))*indx_count*3);
	for (i = 0; i<indx_count; i++) {
		rep_->normal[i*3+0] = 0.0;
		rep_->normal[i*3+1] = 0.0;
		rep_->normal[i*3+2] = -1.0;
	}



	if (TextVerbose) printf ("exiting FW_Render_text\n");
}



int
open_font() {
	int len;
	int err;

	if (TextVerbose) 
		printf ("open_font called\n");


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

	if(err = FT_Init_FreeType(&library))
		  die("FreeWRL FreeType Initialize error %d\n",err);
}
