# Copyright (C) 2000, 2002 John Stewart, CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# This is the  Text  -> polyrep code, used by VRMLC.pm to generate
# VRMLFunc.xs &c.

	


'
	struct VRML_PolyRep *rep_ = this_->_intern;
        float spacing = 1.0;
        float size = 1.0;
	unsigned int fsparams = 0;

	/* for shape display list redrawing */
	this_->_myshape = last_visited_shape; 

	/* We need both sides */
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);

	if($f(fontStyle)) {
		/* We have a FontStyle. Parse params (except size and spacing) and
		   make up an unsigned int with bits indicating params, to be
		   passed to the Text Renderer 
	
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

		struct VRML_FontStyle *fsp = $f(fontStyle);
		unsigned char *lang = SvPV((fsp->language),PL_na);
		unsigned char *style = SvPV((fsp->style),PL_na);
		struct Multi_String family = fsp->family;	 
		struct Multi_String justify = fsp->justify;
		int tmp; int tx;
		SV **svptr;
		unsigned char *stmp;


		/* Step 1 - record the spacing and size, for direct use */
		spacing = fsp->spacing;
		size = fsp->size;

		/* Step 2 - do the SFBools */
        	fsparams = (fsp->horizontal)|(fsp->leftToRight<<1)|(fsp->topToBottom<<2); 


		/* Step 3 - the SFStrings - style and language */
		/* actually, language is not parsed yet */
	
		if (strlen(style)) {
			if (!strcmp(style,"ITALIC")) {fsparams |= 0x10;}
			else if(!strcmp(style,"BOLD")) {fsparams |= 0x08;}
			else if (!strcmp(style,"BOLDITALIC")) {fsparams |= 0x18;}
			else if (strcmp(style,"PLAIN")) {
				printf ("Warning - FontStyle style %s  assuming PLAIN\n",style);}
		}
		if (strlen(lang)) {printf ("Warning - FontStyle - language param unparsed\n");}


		/* Step 4 - the MFStrings now. Family, Justify. */
		/* family can be blank, or one of the pre-defined ones. Any number of elements */

		svptr = family.p;
		for (tmp = 0; tmp < family.n; tmp++) {
			stmp = SvPV(svptr[tmp],PL_na);
			if (strlen(stmp) == 0) {fsparams |=0x20; } 
			else if (!strcmp(stmp,"SERIF")) { fsparams |= 0x20;}
			else if(!strcmp(stmp,"SANS")) { fsparams |= 0x40;}
			else if (!strcmp(stmp,"TYPEWRITER")) { fsparams |= 0x80;}
			//else { printf ("Warning - FontStyle family %s unknown\n",stmp);}
		}

		svptr = justify.p;
		tx = justify.n;
		/* default is "BEGIN" "FIRST" */
		if (tx == 0) { fsparams |= 0x2400; }
		else if (tx == 1) { fsparams |= 0x2000; }
		else if (tx > 2) {
			printf ("Warning - FontStyle, max 2 elements in Justify\n");
			tx = 2;
		}
		
		for (tmp = 0; tmp < tx; tmp++) {
			stmp = SvPV(svptr[tmp],PL_na);
			if (strlen(stmp) == 0) {
				if (tmp == 0) {fsparams |= 0x400;
				} else {fsparams |= 0x2000;
				}
			} 
			else if (!strcmp(stmp,"FIRST")) { fsparams |= (0x200<<(tmp*4));}
			else if(!strcmp(stmp,"BEGIN")) { fsparams |= (0x400<<(tmp*4));}
			else if (!strcmp(stmp,"MIDDLE")) { fsparams |= (0x800<<(tmp*4));}
			else if (!strcmp(stmp,"END")) { fsparams |= (0x1000<<(tmp*4));}
			//else { printf ("Warning - FontStyle family %s unknown\n",stmp);}
		}
	} else {
		/* send in defaults */
		fsparams = 0x2427;
	}


	// do the Text parameters, guess at the number of triangles required
	rep_->ntri = 0;
	//printf ("Text, calling FW_rendertext\n");

	/* call render text - NULL means get the text from the string */
	FW_rendertext ($f_n(string),$f(string),NULL, $f_n(length),$f(length),
			$f(maxExtent),spacing,size,fsparams,rep_);

	//printf ("Text, tris = %d\n",rep_->ntri);

	glPopAttrib();

	'
