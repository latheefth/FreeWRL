/* 
 * Copyright(C) 1998 Tuomas J. Lukka, 2001, 2002 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

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

#include "Structs.h"
#include "headers.h"


#define XRES 96
#define YRES 96
#define PPI 72
#define POINTSIZE 20


#define TOPTOBOTTOM (fsparam & 0x04)
#define LEFTTORIGHT (!(fsparam & 0x02))

#define OUT2GL(a) (x_size * (0.0 +a) / ((1.0*(font_face[myff]->height)) / PPI*XRES))

#ifdef AQUA
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

/* #include "../OpenGL/OpenGL.m"

D_OPENGL;
*/ 

#include <stdio.h>

#include <ft2build.h>
#include <ftoutln.h>

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
int		TextVerbose = 0;


/* decompose interface func pointer */
FT_Outline_Funcs FW_outline_interface;


/* lets store the font paths here */
char sys_fp[fp_name_len];
char thisfontname[fp_name_len];

/* where are we? */
float pen_x, pen_y;


float x_size;		/* size of chars from file */
float y_size;		/* size of chars from file */
int   myff;		/* which index into font_face are we using  */


/* for keeping track of tesselated points */
int FW_RIA[500];	/* pointer to which point is returned by tesselator  */
int FW_RIA_indx;			/* index into FW_RIA			     */
struct VRML_PolyRep *FW_rep_;	/* this is the internal rep of the polyrep	     */
int FW_pointctr;		/* how many points used so far? maps into rep-_coord */
int indx_count;			/* maps intp FW_rep_->cindex			     */
int coordmaxsize;		/* maximum coords before needing to realloc	     */
int cindexmaxsize;		/* maximum cindexes before needing to realloc        */


/* Outline callbacks and global vars */
int contour_started;
FT_Vector last_point;
int FW_Vertex;

/* flag to determine if we need to call the open_font call */ 
int started = FALSE;

/* function prototypes */
void FW_NewVertexPoint(float Vertex_x, float Vertex_y);
int FW_moveto (FT_Vector* to, void* user);
int FW_lineto(FT_Vector* to, void* user);
int FW_conicto(FT_Vector* control, FT_Vector* to, void* user);
int FW_cubicto(FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* user);
void FW_make_fontname (int num);
int FW_init_face(void);
float FW_extent (int start, int length);
FT_Error FW_Load_Char(int idx);
void FW_draw_outline(FT_OutlineGlyph oglyph);
void FW_draw_character(FT_Glyph glyph);
void FW_rendertext(int numrows,SV **ptr,char *directstring, int nl, float *length,
                float maxext, float spacing, float mysize, unsigned int fsparam,
                struct VRML_PolyRep *rp) ;
int open_font(void);


void FW_NewVertexPoint (float Vertex_x, float Vertex_y) {
	GLdouble v2[3];

	UNUSED(Vertex_x);
	UNUSED(Vertex_y);

	/* printf ("FW_NewVertexPoint setting coord index %d %d %d\n", */
	/* 	FW_pointctr, FW_pointctr*3+2,FW_rep_->coord[FW_pointctr*3+2]); */
	FW_rep_->coord[FW_pointctr*3+0] = OUT2GL(last_point.x + pen_x);
	FW_rep_->coord[FW_pointctr*3+1] = OUT2GL(last_point.y) + pen_y;
	FW_rep_->coord[FW_pointctr*3+2] = 0.0;

	/* the following should NEVER happen.... */
	if (FW_RIA_indx >500) { printf ("Text, relative index too small\n");exit(1);}

	FW_RIA[FW_RIA_indx]=FW_pointctr;
	v2[0]=FW_rep_->coord[FW_pointctr*3+0];
	v2[1]=FW_rep_->coord[FW_pointctr*3+1];
	v2[2]=FW_rep_->coord[FW_pointctr*3+2];

	gluTessVertex(global_tessobj,v2,&FW_RIA[FW_RIA_indx]);

	if (TextVerbose) {
		printf ("FW_NewVertexPoint %f %f %f index %d\n", 
				FW_rep_->coord[FW_pointctr*3+0],
				FW_rep_->coord[FW_pointctr*3+1],
				FW_rep_->coord[FW_pointctr*3+2],
				FW_RIA_indx);
	}
	FW_pointctr++;
	FW_RIA_indx++;

	if (FW_pointctr >= coordmaxsize) {
		coordmaxsize+=800;
		FW_rep_->coord = realloc(FW_rep_->coord, sizeof(*(FW_rep_->coord))*coordmaxsize*3);

		if (!(FW_rep_->coord)) { 
			printf ("realloc failed - out of memory \n");
			exit(1);
		}
	}

  }

int FW_moveto (FT_Vector* to, void* user) {
	UNUSED(user);

	/* Have we started a new line */
	if (contour_started) {
		gluNextContour(global_tessobj,GLU_UNKNOWN);
	}

	/* well if not, tell us that we have started one */
	contour_started = TRUE;

	last_point.x = to->x; last_point.y = to->y;

	if (TextVerbose) 
		printf ("FW_moveto tox %ld toy %ld\n",to->x, to->y);

    return 0;
}

int FW_lineto (FT_Vector* to, void* user) {
	UNUSED(user);

	if ((last_point.x == to->x) && (last_point.y == to->y)) {
		/* printf ("FW_lineto, early return\n"); */
		return 0;
	}

	last_point.x = to->x; last_point.y = to->y;
	if (TextVerbose) {
		printf ("FW_lineto, going to %ld %ld\n",to->x, to->y);
	}

	FW_NewVertexPoint(OUT2GL(last_point.x+pen_x), OUT2GL(last_point.y + pen_y));

	return 0;
}


int FW_conicto (FT_Vector* control, FT_Vector* to, void* user) {
	FT_Vector ncontrol;

	/* Bezier curve calcs; fairly rough, but makes ok characters */

	if (TextVerbose)
		printf ("FW_conicto\n");
	ncontrol.x =(int) ((float) 0.25*last_point.x + 0.5*control->x + 0.25*to->x),
	ncontrol.y =(int) ((float) 0.25*last_point.y + 0.5*control->y + 0.25*to->y),

	/* printf ("Cubic points (%d %d) (%d %d) (%d %d)\n", last_point.x,last_point.y, */
	/* 	ncontrol.x, ncontrol.y, to->x,to->y); */

	FW_lineto (&ncontrol,user);
	FW_lineto (to,user);

	return 0;
}

int FW_cubicto (FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* user) {
	/* GLdouble *v2; */

	/* really ignore control points */
	if (TextVerbose)
		printf ("FW_cubicto\n");

	FW_lineto (control1, user);
	FW_lineto (control2, user);
	FW_lineto (to, user);
	return 0;
}


/* make up the font name */
void FW_make_fontname (int num) {
	/* int i; */

/*
                        bit:    0       BOLD        (boolean)
                        bit:    1       ITALIC      (boolean)
                        bit:    2       SERIF
                        bit:    3       SANS
                        bit:    4       TYPEWRITER
*/

	strcpy (thisfontname, sys_fp);
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

	/* JAS gluTessBeginPolygon(global_tessobj,NULL); */
	gluBeginPolygon(global_tessobj);
	FW_Vertex = 0;

	/* thisptr may possibly be null; I dont think it is use in freetype */
	retval = FT_Outline_Decompose (&oglyph->outline, &FW_outline_interface, &thisptr);

	if (contour_started) {
		/* glEnd(); */
	}

	/* gluTessEndPolygon(global_tessobj); */
	gluEndPolygon(global_tessobj);

	if (retval != FT_Err_Ok) printf ("FT_Outline_Decompose, error %d\n");	
}


/* draw a glyph object */
void FW_draw_character (FT_Glyph glyph) {

	if (glyph->format == ft_glyph_format_outline) {
		FW_draw_outline ((FT_OutlineGlyph) glyph);
		pen_x +=  (glyph->advance.x >> 10);
	} else {
		printf ("FW_draw_character; glyphformat  -- need outline for %s %s\n",
			font_face[myff]->family_name,font_face[myff]->style_name); 
	}
	if (TextVerbose) printf ("done character\n");
}




/* take a text string, font spec, etc, and make it into an OpenGL Polyrep.
   Note that the text comes EITHER from a SV (ie, from perl) or from a directstring,
   eg, for placing text on the screen from within FreeWRL itself */

void FW_rendertext(int numrows,SV **ptr,char *directstring, int nl, float *length, 
		float maxext, float spacing, float mysize, unsigned int fsparam,
		struct VRML_PolyRep *rp) {
	char *str;
	int i/* ,gindex */,row;
	/* int contour; */
	/* int point; */
	/* int err; */
	float shrink = 0;
	float rshrink = 0;
	/* int flag; */
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

	/* do we need to call open font? */
	if (!started) {
		if (strlen(sys_fp) ==0) {
			printf ("Could not find System Fonts for Text nodes\n");
			return;
		}
		open_font();
		started = TRUE;
	}

	if (TextVerbose) printf ("entering FW_Render_text \n");

	FW_rep_ = rp;


	FW_RIA_indx = 0;                /* index into FW_RIA                         */
	FW_pointctr=0;              /* how many points used so far? maps into rep-_coord */
	indx_count=0;               /* maps intp FW_rep_->cindex                            */
	contour_started = FALSE;


	pen_x = 0.0; pen_y = 0.0;
	cur_glyph = 0;
	x_size = mysize;		/* global variable for size */
	y_size = mysize;		/* global variable for size */


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

	/* if we have a direct string, then we only have ONE, so initialize it here */
	if (directstring != 0) str = directstring;

	/* load all of the characters first... */
	for (row=0; row<numrows; row++) {
		if (directstring == 0) str = SvPV(ptr[row],PL_na);

		for(i=0; i<strlen(str); i++) {
			FW_Load_Char(str[i]);
			char_count++;
		}
	}

	if (TextVerbose) {
		printf ("Text: rows %d char_count %d\n",numrows,char_count);
	}

	/* what is the estimated number of triangles? assume a certain number of tris per char */
	est_tri = char_count*TESS_MAX_COORDS;
	coordmaxsize=est_tri;
	cindexmaxsize=est_tri;
	FW_rep_->cindex=malloc(sizeof(*(FW_rep_->cindex))*est_tri);
	FW_rep_->coord = malloc(sizeof(*(FW_rep_->coord))*est_tri*3);
	if (!(FW_rep_->coord && FW_rep_->cindex)) {
		printf ("can not malloc memory for text triangles\n");
		exit(1);
	}


	
	if(maxext > 0) {
	   double maxlen = 0;
	   double l;
	   int counter = 0;
	   for(row = 0; row < numrows; row++) {
		if (directstring == 0) str = SvPV(ptr[row],PL_na);
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
		pen_y -= numrows-1;
	}


	/* leftToRight */
	if (LEFTTORIGHT) {
		glRotated(180.0,0.0,1.0,0.0);
	}


	for(row = 0; row < numrows; row++) {
	   	float rowlen;

		if (directstring == 0) str = SvPV(ptr[row],PL_na);
		if (TextVerbose) 
				printf ("text2 row %d :%s:\n",row, str);
	        pen_x = 0.0;
		rshrink = 0;
		rowlen = FW_extent(counter,strlen(str));
		if(row < nl && length[row]) {
			rshrink = length[row] / OUT2GL(rowlen);
		}
		if(shrink) { glScaled(shrink,1.0,1.0); }
		if(rshrink) { glScaled(rshrink,1.0,1.0); }


		/* Justify, FIRST, BEGIN, MIDDLE and END */

		/* MIDDLE */
		if (fsparam & 0x800) { pen_x = -rowlen/2.0; }

		/* END */
		if ((fsparam & 0x1000) & (fsparam & 0x01)) {
			/* printf ("rowlen is %f\n",rowlen); */
			pen_x = -rowlen;
		}


		for(i=0; i<strlen(str); i++) {
			/* FT_UInt glyph_index; */
			/* int error; */
			int x;

			global_IFS_Coord_count = 0;
			FW_RIA_indx = 0;

			FW_draw_character (glyphs[counter+i]);
			FT_Done_Glyph (glyphs[counter+i]);

			/* copy over the tesselated coords for the character to
			 * the rep structure */

			for (x=0; x<global_IFS_Coord_count; x++) {
				/* printf ("copying %d\n",global_IFS_Coords[x]); */

				/* did the tesselator give us back garbage? */

				if ((global_IFS_Coords[x] >= cindexmaxsize) ||
				   (indx_count >= cindexmaxsize) ||
				   (global_IFS_Coords[x] < 0)) {
					/* if (TextVerbose)  */
					/* printf ("Tesselated index %d out of range; skipping indx_count, %d cindexmaxsize %d global_IFS_Coord_count %d\n", */
					/* 	     global_IFS_Coords[x],indx_count,cindexmaxsize,global_IFS_Coord_count); */
					/* just use last point - this sometimes happens when */
					/* we have intersecting lines. Lets hope first point is */
					/* not invalid... JAS */
					FW_rep_->cindex[indx_count] = FW_rep_->cindex[indx_count-1];
					if (indx_count < (cindexmaxsize-1)) indx_count ++;
				} else {	
					/* printf ("global_ifs_coords is %d indx_count is %d \n",global_IFS_Coords[x],indx_count); */
					/* printf ("filling up cindex; index %d now points to %d\n",indx_count,global_IFS_Coords[x]); */
					FW_rep_->cindex[indx_count++] = global_IFS_Coords[x];
				}
			}

			if (indx_count > (cindexmaxsize-400)) {
				cindexmaxsize +=TESS_MAX_COORDS;
				FW_rep_->cindex=realloc(FW_rep_->cindex,sizeof(*(FW_rep_->cindex))*cindexmaxsize);
				if (!(FW_rep_->cindex)) {
					printf ("out of memory at realloc for cindex\n");
					exit(1);
				}
			}
		}
		counter += strlen(str);

		pen_y += spacing * y_size;
   	}
	/* save the triangle count (note, we have a "vertex count", not a "triangle count" */
	FW_rep_->ntri=indx_count/3;

	/* if indx count is zero, DO NOT get rid of mallocd memory - creates a bug as pointers cant be null */
	if (indx_count !=0) {
		/* realloc bug in linux - this causes the pointers to be eventually lost... */
		/* realloc (FW_rep_->cindex,sizeof(*(FW_rep_->cindex))*indx_count); */
		/* realloc (FW_rep_->coord,sizeof(*(FW_rep_->coord))*FW_pointctr*3); */
	}

	/* now, generate normals */
	FW_rep_->normal = malloc(sizeof(*(FW_rep_->normal))*indx_count*3);
	for (i = 0; i<indx_count; i++) {
		FW_rep_->normal[i*3+0] = 0.0;
		FW_rep_->normal[i*3+1] = 0.0;
		FW_rep_->normal[i*3+2] = -1.0;
	}


	/* do we have texture mapping to do? */
	if (HAVETODOTEXTURES) {
		FW_rep_->tcoord = malloc(sizeof(*(FW_rep_->tcoord))*(FW_pointctr+1)*3);
		if (!(FW_rep_->tcoord)) {
			printf ("can not malloc memory for text textures\n");
		} else {
			/* an attempt to try to make this look like the NIST example */
			/* I can't find a standard as to how to map textures to text JAS */
			for (i=0; i<FW_pointctr; i++) {
				FW_rep_->tcoord[i*3+0] = FW_rep_->coord[i*3+0]*1.66;
				FW_rep_->tcoord[i*3+1] = 0.0; 
				FW_rep_->tcoord[i*3+2] = FW_rep_->coord[i*3+1]*1.66;
			}

		}

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

	if ((err = FT_Init_FreeType(&library)))
		  fprintf(stderr, "FreeWRL FreeType Initialize error %d\n",err);

	return err;
}
