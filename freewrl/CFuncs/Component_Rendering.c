/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Rendering Component

*********************************************************************/

#include "headers.h"


void render_IndexedTriangleFanSet (struct X3D_IndexedTriangleFanSet *this_) {
                if (!this_->_intern || this_->_change != ((struct X3D_PolyRep *)this_->_intern)->_change) 
                        regen_polyrep(this_, this_->coord, this_->color, this_->normal, this_->texCoord);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}

void render_IndexedTriangleSet (struct X3D_IndexedTriangleSet *this_) {
                if (!this_->_intern || this_->_change != ((struct X3D_PolyRep *)this_->_intern)->_change) 
                        regen_polyrep(this_, this_->coord, this_->color, this_->normal, this_->texCoord);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}

void render_IndexedTriangleStripSet (struct X3D_IndexedTriangleStripSet *this_) {
                if (!this_->_intern || this_->_change != ((struct X3D_PolyRep *)this_->_intern)->_change) 
                        regen_polyrep(this_, this_->coord, this_->color, this_->normal, NULL);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}

void render_TriangleFanSet (struct X3D_TriangleFanSet *this_) {
                if (!this_->_intern || this_->_change != ((struct X3D_PolyRep *)this_->_intern)->_change) 
                        regen_polyrep(this_, this_->coord, this_->color, this_->normal, this_->texCoord);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}

void render_TriangleStripSet (struct X3D_TriangleStripSet *this_) {
                if (!this_->_intern || this_->_change != ((struct X3D_PolyRep *)this_->_intern)->_change) 
                        regen_polyrep(this_, this_->coord, this_->color, this_->normal, this_->texCoord);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}

void render_TriangleSet (struct X3D_TriangleSet *this_) {
                if (!this_->_intern || this_->_change != ((struct X3D_PolyRep *)this_->_intern)->_change) 
                        regen_polyrep(this_, this_->coord, this_->color, this_->normal, this_->texCoord);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);
		if(!this_->solid) glPopAttrib();
}


void render_LineSet (struct X3D_LineSet *this_) {
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */

	int vtc;		/* which vertexCount[] we should be using for this line segment */
	int ncoc;		/* which coord we are using for this vertex */
	int punt;		/* how many vetex points are in this line segment */
	int c;			/* temp variable */
	/* int *p;*/
	struct SFColor *coord=0; int ncoord;
	struct SFColor *color=0; int ncolor=0;
	int *vertexC; int nvertexc;
	struct X3D_Coordinate *xc;
	struct X3D_Color *cc;

	glPushAttrib(GL_ENABLE_BIT);
	glDisable (GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);

	/* do we have to re-verify LineSet? */
	if (this_->_ichange != this_->_change) {
		/*  re-draw every time. this_->_ichange = this_->_change;*/

		nvertexc = (this_->vertexCount).n; vertexC = (this_->vertexCount).p;

        	if(this_->coord) {
                	xc = (struct X3D_Coordinate *) this_->coord;
                	if (xc->_nodeType != NODE_Coordinate) {
                	        freewrlDie ("LineSet, coord node expected");
                	} else {
                        	coord = xc->point.p;
                        	ncoord = xc->point.n;
                	}
        	}
 

        	if (this_->color) {
                	cc = (struct X3D_Color *) this_->color;
                	if ((cc->_nodeType != NODE_Color) && (cc->_nodeType != NODE_ColorRGBA)) {
                	        ConsoleMessage ("make_IFS, expected %d got %d\n", NODE_Color, cc->_nodeType);
                	} else {
                	        ncolor = cc->color.n;
				color = cc->color.p;
                	}
        	}



		/* printf ("we have %d coords, %d colors\n",ncoord,ncolor);*/
		ncoc = 0;

		if ((nvertexc == 0) || (ncoord == 0)) {
			printf ("LineSet, no vertexCounts or no coords\n");
			this_->_ichange = this_->_change; /* make this error show only once */

			return;
		}


		/* if we are re-genning; remalloc */
		/* if (this_->__points) free ((void *)this_->__points);*/

		/* if (!this_->__points) this_->__points = malloc (sizeof(unsigned int)*(nvertexc));*/
		/* if (!this_->__points) {*/
		/* 	printf ("can not malloc memory for LineSet points\n");*/
		/* 	this_->_ichange = this_->_change; make this error show only once */
		/* 	return;*/
		/* }*/
		/* p = (int *)this_->__points;*/


		/* go through the vertex count array and verify that all is good. */
		for (vtc = 0; vtc < nvertexc; vtc++) {
			/* save the pointer to the vertex array for the GL call */
			/* *p = vertexC;*/
			/* p++;*/

			punt = *vertexC;

			/* there HAVE to be 2 or more vertexen in a segment */
			if (punt < 2) {
				printf ("LineSet, vertexCount[%d] has %d vertices...\n",vtc,punt);
				this_->_ichange = this_->_change; /* make this error show only once */
				/* free ((void *)this_->__points);*/
glPopAttrib();
				return;
			}

			/* do we have enough Vertex Coords? */
			if ((punt + ncoc) > ncoord) {
				printf ("LineSet, ran out of vertices at vertexCount[%d] has %d vertices...\n",vtc,punt);
				this_->_ichange = this_->_change; /* make this error show only once */
				/* free ((void *)this_->__points);*/
glPopAttrib();
				return;
			}
			if (ncolor != 0) {
			if ((punt + ncoc) > ncolor) {
				printf ("LineSet, ran out of vertices at vertexCount[%d] has %d vertices...\n",vtc,punt);
				this_->_ichange = this_->_change; /* make this error show only once */
				/* free ((void *)this_->__points);*/
glPopAttrib();
				return;
			}
			}

			/*  do this for glMultiDrawElements ncoc += punt;*/
			glBegin(GL_LINE_STRIP);

			/* draw the line */
			if (ncolor!= 0) 
				glColor3f( color[ncoc].c[0],color[ncoc].c[1],color[ncoc].c[2]);
			else glColor3f (1.0, 1.0, 1.0);

			glVertex3f( coord[ncoc].c[0],coord[ncoc].c[1],coord[ncoc].c[2]);
			ncoc++;


			for (c=1; c<punt; c++) {
				if (ncolor!= 0) 
					glColor3f( color[ncoc].c[0],color[ncoc].c[1],color[ncoc].c[2]);
			
				
				glVertex3f( coord[ncoc].c[0],coord[ncoc].c[1],coord[ncoc].c[2]);
				ncoc++;
			}

			/* now, lets go and get ready for the next vertexCount */
			vertexC++;
			glEnd();


		}
	}

	/* now, actually draw array */
	/* if (this_->__points) {*/
	/* 	printf ("calling glMultiDrawElements, promcount %d\n",(this_->vertexCount).n);*/
	/* 	glMultiDrawElements(GL_LINE_STRIP,(this_->vertexCount).p,GL_FLOAT,*/
	/* 			this_->__points,(this_->vertexCount).n);*/
	/* }*/
	glPopAttrib();
}

void render_IndexedLineSet (struct X3D_IndexedLineSet *this_) {
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
		int i;
                int cin = (this_->coordIndex.n);
                int colin = (this_->colorIndex.n);
                int cpv = (this_->colorPerVertex);

		int plno = 0;
		int ind;
		int c;
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct X3D_Coordinate *xc;
		struct X3D_Color *cc;

		#ifdef RENDERVERBOSE
		printf("Line: cin %d colin %d cpv %d\n",cin,colin,cpv);
		#endif

        	if(this_->coord) {
                	xc = (struct X3D_Coordinate *) this_->coord;
                	if (xc->_nodeType != NODE_Coordinate) {
                	        freewrlDie ("IndexedLineSet, coord node expected");
                	} else {
                        	points = xc->point.p;
                        	npoints = xc->point.n;
                	}
        	}
 
        	if (this_->color) {
                	cc = (struct X3D_Color *) this_->color;
                	if ((cc->_nodeType != NODE_Color) && (cc->_nodeType != NODE_ColorRGBA)) {
                	        ConsoleMessage ("make_IFS, expected %d got %d\n", NODE_Color, cc->_nodeType);
                	} else {
                	        ncolors = cc->color.n;
				colors = cc->color.p;
                	}
        	}

                glPushAttrib(GL_ENABLE_BIT);
		glDisable (GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
                glDisable(GL_CULL_FACE);

		glBegin(GL_LINE_STRIP);
		for(i=0; i<cin; i++) {
			ind = this_->coordIndex.p[i];
			#ifdef RENDERVERBOSE
			printf("Line: %d %d\n",i,ind);
			#endif

			/* a new line strip? */
			if(ind==-1) {
				glEnd();
				glBegin(GL_LINE_STRIP);
				plno++;
			} else {
				if(ncolors) {
					if (cpv) {
						c = i;
						if(colin)  c = this_->colorIndex.p[i];
						else c=i; 
					} else {
						c = plno;
					}

					if (c<ncolors) {
					      glColor3f(colors[c].c[0],
					        colors[c].c[1],
					   	colors[c].c[2]);
					} else {
					      glColor3f(colors[0].c[0],
					        colors[0].c[1],
					   	colors[0].c[2]);
					}

				} else {
					glColor3f(1.0,1.0,1.0);
				}
				glVertex3f(
					points[ind].c[0],
					points[ind].c[1],
					points[ind].c[2]
				);
			}
		}
		glEnd();
                glPopAttrib();
}

void render_PointSet (struct X3D_PointSet *this_) {
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	/* we should be able to speed this up greatly using glMultiDrawElements, but, for now use immediate mode */
	int i;
	struct SFColor *points=0; int npoints=0;
	struct SFColor *colors=0; int ncolors=0;
	struct X3D_Coordinate *xc;
	struct X3D_Color *cc;

        	if(this_->coord) {
                	xc = (struct X3D_Coordinate *) this_->coord;
                	if (xc->_nodeType != NODE_Coordinate) {
                	        freewrlDie ("IndexedLineSet, coord node expected");
                	} else {
                        	points = xc->point.p;
                        	npoints = xc->point.n;
                	}
        	}
 

        	if (this_->color) {
                	cc = (struct X3D_Color *) this_->color;
                	if ((cc->_nodeType != NODE_Color) && (cc->_nodeType != NODE_ColorRGBA)) {
                	        ConsoleMessage ("make_IFS, expected %d got %d\n", NODE_Color, cc->_nodeType);
                	} else {
                	        ncolors = cc->color.n;
				colors = cc->color.p;
                	}
        	}

	if(ncolors && ncolors < npoints) {
		printf ("PointSet has less colors than points - removing color\n");
		ncolors = 0;
	}

	glPushAttrib(GL_ENABLE_BIT);
	glDisable (GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);

	glBegin(GL_POINTS);

	#ifdef RENDERVERBOSE
	printf("PointSet: %d %d\n", npoints, ncolors);
	#endif

	if (ncolors==0) glColor3f (1.0, 1.0, 1.0);

	for(i=0; i<npoints; i++) {
		if(ncolors) {
			#ifdef RENDERVERBOSE
			printf("Color: %f %f %f\n",
				  colors[i].c[0],
				  colors[i].c[1],
				  colors[i].c[2]);
			#endif

			glColor3f(colors[i].c[0],
				  colors[i].c[1],
				  colors[i].c[2]);
		}
		glVertex3f(
			points[i].c[0],
			points[i].c[1],
			points[i].c[2]
		);
	}
	glEnd();
                glPopAttrib();
}
