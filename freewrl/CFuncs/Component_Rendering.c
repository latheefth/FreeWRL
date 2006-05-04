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


extern GLfloat last_emission[];

void render_IndexedTriangleFanSet (struct X3D_IndexedTriangleFanSet *node) {
                if (!node->_intern || node->_change != ((struct X3D_PolyRep *)node->_intern)->_change) 
                        regen_polyrep(node, node->coord, node->color, node->normal, node->texCoord);

		CULL_FACE(node->solid)
		render_polyrep(node);
}

void render_IndexedTriangleSet (struct X3D_IndexedTriangleSet *node) {
                if (!node->_intern || node->_change != ((struct X3D_PolyRep *)node->_intern)->_change) 
                        regen_polyrep(node, node->coord, node->color, node->normal, node->texCoord);

		CULL_FACE(node->solid)
		render_polyrep(node);
}

void render_IndexedTriangleStripSet (struct X3D_IndexedTriangleStripSet *node) {
                if (!node->_intern || node->_change != ((struct X3D_PolyRep *)node->_intern)->_change) 
                        regen_polyrep(node, node->coord, node->color, node->normal, NULL);

		CULL_FACE(node->solid)
		render_polyrep(node);
}

void render_TriangleFanSet (struct X3D_TriangleFanSet *node) {
                if (!node->_intern || node->_change != ((struct X3D_PolyRep *)node->_intern)->_change) 
                        regen_polyrep(node, node->coord, node->color, node->normal, node->texCoord);

		CULL_FACE(node->solid)
		render_polyrep(node);
}

void render_TriangleStripSet (struct X3D_TriangleStripSet *node) {
                if (!node->_intern || node->_change != ((struct X3D_PolyRep *)node->_intern)->_change) 
                        regen_polyrep(node, node->coord, node->color, node->normal, node->texCoord);

		CULL_FACE(node->solid)
		render_polyrep(node);
}

void render_TriangleSet (struct X3D_TriangleSet *node) {
                if (!node->_intern || node->_change != ((struct X3D_PolyRep *)node->_intern)->_change) 
                        regen_polyrep(node, node->coord, node->color, node->normal, node->texCoord);

		CULL_FACE(node->solid)
		render_polyrep(node);
}


void render_LineSet (struct X3D_LineSet *node) {
	int vtc;		/* which vertexCount[] we should be using for this line segment */
	int c;			/* temp variable */
	struct SFColor *coord=0; int ncoord;
	struct SFColor *color=0; int ncolor=0;
	int *vertexC; int nvertexc;
	int totVertexRequired;

	struct X3D_Coordinate *xc;
	struct X3D_Color *cc;
	GLuint *pt;
	uintptr_t *vpt;

	/* believe it or not - material emissiveColor can affect us... */
	GLfloat defColor[] = {1.0, 1.0, 1.0};
	GLfloat *thisColor;

	/* is there an emissiveColor here??? */
	if (lightingOn) {
		/* printf ("ILS - have lightingOn!\n"); */
		thisColor = last_emission;
	} else {
		thisColor = defColor;
	}

	LIGHTING_OFF
	DISABLE_CULL_FACE
	

	/* do we have to re-verify LineSet? */
	if (node->_ichange != node->_change) {
		node->_ichange = node->_change;
		node->__segCount = 0; /* assume this for now */


		nvertexc = (node->vertexCount).n; vertexC = (node->vertexCount).p;
		if (nvertexc==0) return;
		totVertexRequired = 0;


		/* sanity check vertex counts */
		for  (c=0; c<nvertexc; c++) {
			totVertexRequired += vertexC[c];
			if (vertexC[c]<2) {
				ConsoleMessage ("make_LineSet, we have a vertexCount of %d, must be >=2,",vertexC[c]);
				return;
			}
		}

        	if(node->coord) {
                	xc = (struct X3D_Coordinate *) node->coord;
                	if (xc->_nodeType != NODE_Coordinate) {
                	        ConsoleMessage ("make_LineSet, coord node expected but not found");
                	} else {
                        	coord = xc->point.p;
                        	ncoord = xc->point.n;
                	}
        	}

		/* check that we have enough vertexes */
		if (totVertexRequired > ncoord) {
			ConsoleMessage ("make_LineSet, not enough points for vertexCount (vertices:%d points:%d)",
				totVertexRequired, ncoord);
			return;
		}
 
        	if (node->color) {
                	cc = (struct X3D_Color *) node->color;
                	if ((cc->_nodeType != NODE_Color) && (cc->_nodeType != NODE_ColorRGBA)) {
                	        ConsoleMessage ("make_LineSet, expected %d got %d\n", NODE_Color, cc->_nodeType);
                	} else {
                	        ncolor = cc->color.n;
				color = cc->color.p;
                	}
			/* check that we have enough verticies for the Colors */
			if (totVertexRequired > ncolor) {
				ConsoleMessage ("make_LineSet, not enough colors for vertexCount (vertices:%d colors:%d)",
					totVertexRequired, ncolor);
				return;
			}
        	}

		/* create the index for the arrays. Really simple... */
		if (node->__vertArr) free ((void *)node->__vertArr);
		node->__vertArr = malloc (sizeof(GLuint)*(ncoord));
		if (!node->__vertArr) {
			printf ("can not malloc memory for LineSet vertArr\n");
			return;
		}
		pt = (GLint *)node->__vertArr;
		for (vtc = 0; vtc < ncoord; vtc++) {
			*pt=vtc; pt++;
		}

		/* create the index for each line segment */
		if (node->__vertIndx) free ((void *)node->__vertIndx);
		node->__vertIndx = malloc (sizeof(uintptr_t)*(nvertexc));
		if (!node->__vertIndx) {
			printf ("can not malloc memory for LineSet vertIndx\n");
			return;
		}
		c = 0;
		pt = (GLint *)node->__vertArr;
		vpt = (uintptr_t *) node->__vertIndx;
		for (vtc=0; vtc<nvertexc; vtc++) {
			*vpt =  (uintptr_t) pt;
			vpt++;
			pt += vertexC[vtc];
		}

		/* if we made it this far, we are ok tell the rendering engine that we are ok */
		node->__segCount = nvertexc;
	}

	/* now, actually draw array */
	if (node->__segCount > 0) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);

		if (node->color) {
                	cc = (struct X3D_Color *) node->color;
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer (3,GL_FLOAT,0,cc->color.p);
		} else {
			glColor3fv (thisColor);
		}
        	xc = (struct X3D_Coordinate *) node->coord;
		glVertexPointer (3,GL_FLOAT,0,xc->point.p);

		glMultiDrawElements (
			GL_LINE_STRIP,
			node->vertexCount.p,
			GL_UNSIGNED_INT,
			node->__vertIndx,
			node->__segCount
		);

		glEnableClientState (GL_NORMAL_ARRAY);
		if (node->color) {
			glDisableClientState(GL_COLOR_ARRAY);
		}
	}
}


void render_IndexedLineSet (struct X3D_IndexedLineSet *node) {
		int i;
                int cin = (node->coordIndex.n);
                int colin = (node->colorIndex.n);
                int cpv = (node->colorPerVertex);

		int plno = 0;
		int ind;
		int c;
		struct SFColor *points=0; int npoints;
		struct SFColor *colors=0; int ncolors=0;
		struct X3D_Coordinate *xc;
		struct X3D_Color *cc;

		/* believe it or not - material emissiveColor can affect us... */
		GLfloat defColor[] = {1.0, 1.0, 1.0};
		GLfloat *thisColor;

		#ifdef RENDERVERBOSE
		printf("Line: cin %d colin %d cpv %d\n",cin,colin,cpv);
		#endif

		/* is there an emissiveColor here??? */
		if (lightingOn) {
			/* printf ("ILS - have lightingOn!\n"); */
			thisColor = last_emission;
		} else {
			thisColor = defColor;
		}


        	if(node->coord) {
                	xc = (struct X3D_Coordinate *) node->coord;
                	if (xc->_nodeType != NODE_Coordinate) {
                	        freewrlDie ("IndexedLineSet, coord node expected");
                	} else {
                        	points = xc->point.p;
                        	npoints = xc->point.n;
                	}
        	}
 
        	if (node->color) {
                	cc = (struct X3D_Color *) node->color;
                	if ((cc->_nodeType != NODE_Color) && (cc->_nodeType != NODE_ColorRGBA)) {
                	        ConsoleMessage ("make_IFS, expected %d got %d\n", NODE_Color, cc->_nodeType);
                	} else {
                	        ncolors = cc->color.n;
				colors = cc->color.p;
                	}
        	}

		LIGHTING_OFF
                DISABLE_CULL_FACE

		glBegin(GL_LINE_STRIP);
		for(i=0; i<cin; i++) {
			ind = node->coordIndex.p[i];
			#ifdef RENDERVERBOSE
			printf("Line: %d %d\n",i,ind);
			#endif

			/* a new line strip? */
			if(ind==-1) {
				glEnd();
				#ifdef RENDERVERBOSE
				printf ("new strip\n");
				#endif
				glBegin(GL_LINE_STRIP);
				plno++;
			} else {
				if(ncolors) {
					if (cpv) c = i;
					 else c = plno;
					

					if(colin) {
					   if ((i>=colin) || (i<0)) {
						c = 0;
					   } else {
					        c = node->colorIndex.p[i]; 
					   }
					}

					#ifdef RENDERVERBOSE
					printf ("using Color %d\n",c);
					#endif
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
					glColor3fv(thisColor);
				}
				glVertex3f(
					points[ind].c[0],
					points[ind].c[1],
					points[ind].c[2]
				);
			}
		}
		glEnd();
}

void render_PointSet (struct X3D_PointSet *node) {
	int i;
	struct SFColor *points=0; int npoints=0;
	struct SFColor *colors=0; int ncolors=0;
	struct X3D_Coordinate *xc;
	struct X3D_Color *cc;

	/* believe it or not - material emissiveColor can affect us... */
	GLfloat defColor[] = {1.0, 1.0, 1.0};
	GLfloat *thisColor;

	/* is there an emissiveColor here??? */
	if (lightingOn) {
		/* printf ("ILS - have lightingOn!\n"); */
		thisColor = last_emission;
	} else {
		thisColor = defColor;
	}


        	if(node->coord) {
                	xc = (struct X3D_Coordinate *) node->coord;
                	if (xc->_nodeType != NODE_Coordinate) {
                	        freewrlDie ("IndexedLineSet, coord node expected");
                	} else {
                        	points = xc->point.p;
                        	npoints = xc->point.n;
                	}
        	}
 

        	if (node->color) {
                	cc = (struct X3D_Color *) node->color;
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

	LIGHTING_OFF
	DISABLE_CULL_FACE

	glBegin(GL_POINTS);

	#ifdef RENDERVERBOSE
	printf("PointSet: %d %d\n", npoints, ncolors);
	#endif

	if (ncolors==0) glColor3fv (thisColor);

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
}

