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
	int ncoc;		/* which coord we are using for this vertex */
	int punt;		/* how many vetex points are in this line segment */
	int c;			/* temp variable */
	/* int *p;*/
	struct SFColor *coord=0; int ncoord;
	struct SFColor *color=0; int ncolor=0;
	int *vertexC; int nvertexc;
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

	LIGHTING_OFF
	DISABLE_CULL_FACE
	

	/* do we have to re-verify LineSet? */
	if (node->_ichange != node->_change) {
		/*  re-draw every time. node->_ichange = node->_change;*/

		nvertexc = (node->vertexCount).n; vertexC = (node->vertexCount).p;

        	if(node->coord) {
                	xc = (struct X3D_Coordinate *) node->coord;
                	if (xc->_nodeType != NODE_Coordinate) {
                	        freewrlDie ("LineSet, coord node expected");
                	} else {
                        	coord = xc->point.p;
                        	ncoord = xc->point.n;
                	}
        	}
 

        	if (node->color) {
                	cc = (struct X3D_Color *) node->color;
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
			node->_ichange = node->_change; /* make this error show only once */

			return;
		}


		/* if we are re-genning; remalloc */
		/* if (node->__points) free ((void *)node->__points);*/

		/* if (!node->__points) node->__points = malloc (sizeof(unsigned int)*(nvertexc));*/
		/* if (!node->__points) {*/
		/* 	printf ("can not malloc memory for LineSet points\n");*/
		/* 	node->_ichange = node->_change; make this error show only once */
		/* 	return;*/
		/* }*/
		/* p = (int *)node->__points;*/


		/* go through the vertex count array and verify that all is good. */
		for (vtc = 0; vtc < nvertexc; vtc++) {
			/* save the pointer to the vertex array for the GL call */
			/* *p = vertexC;*/
			/* p++;*/

			punt = *vertexC;

			/* there HAVE to be 2 or more vertexen in a segment */
			if (punt < 2) {
				printf ("LineSet, vertexCount[%d] has %d vertices...\n",vtc,punt);
				node->_ichange = node->_change; /* make this error show only once */
				/* free ((void *)node->__points);*/
				return;
			}

			/* do we have enough Vertex Coords? */
			if ((punt + ncoc) > ncoord) {
				printf ("LineSet, ran out of vertices at vertexCount[%d] has %d vertices...\n",vtc,punt);
				node->_ichange = node->_change; /* make this error show only once */
				/* free ((void *)node->__points);*/
				return;
			}
			if (ncolor != 0) {
			if ((punt + ncoc) > ncolor) {
				printf ("LineSet, ran out of vertices at vertexCount[%d] has %d vertices...\n",vtc,punt);
				node->_ichange = node->_change; /* make this error show only once */
				/* free ((void *)node->__points);*/
				return;
			}
			}

			/*  do this for glMultiDrawElements ncoc += punt;*/
			glBegin(GL_LINE_STRIP);

			/* draw the line */
			if (ncolor!= 0) 
				glColor3f( color[ncoc].c[0],color[ncoc].c[1],color[ncoc].c[2]);
			else glColor3fv(thisColor);

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
	/* if (node->__points) {*/
	/* 	printf ("calling glMultiDrawElements, promcount %d\n",(node->vertexCount).n);*/
	/* 	glMultiDrawElements(GL_LINE_STRIP,(node->vertexCount).p,GL_FLOAT,*/
	/* 			node->__points,(node->vertexCount).n);*/
	/* }*/
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

