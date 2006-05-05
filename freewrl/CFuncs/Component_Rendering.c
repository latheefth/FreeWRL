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


void test_render_IndexedLineSet (struct X3D_IndexedLineSet *node) {
	int i;		/* temporary */
	struct SFColor *points;
	int npoints;
	struct X3D_Coordinate *xc;
	int maxCoordFound;		/* for bounds checking				*/
	int maxColorFound;		/* for bounds checking				*/
	struct X3D_Color *cc;
	int nSegments;			/* how many individual lines in this shape 	*/
	int thisSeg;			/* temporary					*/
	int thisSegStart;		/* temporary					*/
	int *vertCountPtr;		/* temporary, for vertexCount filling		*/
	uintptr_t *indxStartPtr;	/* temporary, for creating pointer to index arr */

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
	

	/* do we have to re-verify IndexedLineSet? */
	if (node->_ichange != node->_change) {
		node->_ichange = node->_change;
		nSegments = 0;
		node->__segCount = 0;

		if (node->coord) {
			xc = (struct X3D_Coordinate *) node->coord;
			if (xc->_nodeType != NODE_Coordinate) {
				ConsoleMessage ("IndexedLineSet - Coordinate node expected for coord field");
				return;
			} else {
				npoints = xc->point.n;
				points = xc->point.p;
			}
		} else {
			return; /* no coordinates - nothing to do */
		}

		if (node->coordIndex.n == 0) return; /* no coord indexes - nothing to do */

		/* sanity check that we have enough coordinates */
		maxCoordFound = -1000;
		nSegments = 1;
		for (i=0; i<node->coordIndex.n; i++) {
			/* make sure that the coordIndex is greater than -1 */
			if (node->coordIndex.p[i] < -1) {
				ConsoleMessage ("IndexedLineSet - coordIndex less than 0 at %d\n",i);
				return;
			}

			/* count segments; dont bother if the very last number is -1 */
			if (node->coordIndex.p[i] == -1) 
				if (i!=((node->coordIndex.n)-1)) nSegments++;

			/* try to find the highest coordinate index for bounds checking */
			if (node->coordIndex.p[i] > maxCoordFound) maxCoordFound = node->coordIndex.p[i];
		}
		if (maxCoordFound > npoints) {
			ConsoleMessage ("IndexedLineSet - not enough coordinates - coordindex contains higher index");
			return;
		}

		/* now, generate the array that points to the start of each line segment,
		   and, the array of line segment lengths */
		if (node->__vertIndx) free ((void *)node->__vertIndx);
		node->__vertIndx = malloc (sizeof(uintptr_t)*(nSegments));
		if (!node->__vertIndx) {
			printf ("can not malloc memory for LineSet vertIndx\n");
			return;
		}
		if (node->__vertexCount) free ((void *)node->__vertexCount);
		node->__vertexCount = malloc (sizeof(int)*(nSegments));
		if (!node->__vertexCount) {
			printf ("can not malloc memory for LineSet vertexCount\n");
			return;
		}

		/* ... we have the arrays... lets do the start */
		thisSeg = 0;
		thisSegStart = 0;
		i = 0;
		vertCountPtr = (int *) node->__vertexCount;
		indxStartPtr = (uintptr_t *)node->__vertIndx;

		/* record the start of the first segment - this one is easy! */
		*indxStartPtr = ((uintptr_t) &(node->coordIndex.p[thisSegStart])); 
		indxStartPtr++;

		while (thisSeg < nSegments) {
			/* did we find the end of a segment? */
			/* printf ("looking at coord %d for index %d n is %d\n",node->coordIndex.p[i], i, node->coordIndex.n); */
			if ((node->coordIndex.p[i] <0) || (i>=(node->coordIndex.n)-1)) {
				/* record segment length (number of vertices) ... */
				vertCountPtr[thisSeg] = i-thisSegStart;

				/* but, if we are at the end, and the last number is NOT -1... */
				if (node->coordIndex.p[i] != -1) { vertCountPtr[thisSeg]++;}

				/* lines must have at least 2 vertices... */
				if (vertCountPtr[thisSeg] < 2) {
					ConsoleMessage ("IndexedLineSet, segment %d has less than 2 vertices",thisSeg);
					return;
				}

				/* printf ("segment length %d\n",vertCountPtr[thisSeg]); */
				thisSegStart = i+1; /* skip past the -1... */
				
				thisSeg++;
				if (thisSeg < nSegments) {
					/* record the start pointer for this segment */
					*indxStartPtr = ((uintptr_t) &(node->coordIndex.p[thisSegStart])); 
					indxStartPtr++;
					/*printf ("new segment found at %d - segment %d \n",i,thisSeg);  */
				}
			}
			
			i++;
		}


		/* sanity check the colors, if they exist */
		if (node->color) {
			/* we resort the color nodes so that we have an RGBA color node per vertex */
			if (node->__colours) free ((void *)node->__colours);
			node->__colours = malloc (4 * sizeof(float)*(maxCoordFound+1));
			if (!node->__colours) {
				printf ("can not malloc memory for LineSet colIndx\n");
				return;
			}
               		cc = (struct X3D_Color *) node->color;
			/* do we have a colorIndex? if so, make sure it is big enough */
			printf ("node colorindex count is %d\n",node->colorIndex.n);

			/* we can have a colorPerVertex flag, and a colorIndex array... */
			if (node->colorPerVertex) {
				/* we expect a color for each vertex. Now, do we have a colorIndex, or
				   do we use the coordIndex?? */

				if ((node->colorIndex.n)>0) {
					printf ("CPV FILL ME IN\n");
				} else {
					/* we should have the same, or more colors as required vertices */
					printf ("checking we have enough colors... colorn %d maxvertexfound %d\n",
						cc->color.n, maxCoordFound);
					if ((cc->color.n) > maxCoordFound) {
						ConsoleMessage ("IndexedLineSet, have %d colors, but %d vertex",
							cc->color.n,maxCoordFound);
						return; /* oh well... */
					}

					/* colorPerVertex, no colorIndex, so copy vertex index to color index */
					/* memcpy (node->__colours, node-> */
				}

			} else {
				/* so, we have a color per line segment. Lets check this stuff... */
				if ((node->colorIndex.n)>0) {
					printf ("NOT CPV FILL ME IN\n");
				} else {
					/* we should have the same, or more colors as required segments */
					printf ("checking we have enough colors... colorn %d maxsegfound %d\n",
						cc->color.n, nSegments);
					if ((cc->color.n) > nSegments) {
						ConsoleMessage ("IndexedLineSet, have %d colors, but %d segments",
							cc->color.n,nSegments);
						return; /* oh well... */
					}
				}
			}
		}

		node->__segCount = nSegments; /* we passed, so we can render */
	}

	/* now, actually draw array */
	if (node->__segCount > 0) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);

		if (node->__colours) {
			glEnableClientState(GL_COLOR_ARRAY);
{
GLfloat cols[] = {
		1, 1, 1, 1,
		1, 1, 1, 1,
		1, 1, 1, 1,
		1, 1, 1, 1,

		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,

		0, 1, 0, 1,
		0, 1, 0, 1,
		0, 1, 0, 1,
		0, 1, 0, 1,
		0, 1, 0, 1,
		0, 1, 0, 1,
		0, 1, 0, 1,

		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1};
		glColorPointer (4,GL_FLOAT,0,cols);
}

#ifdef hjy
                	cc = (struct X3D_Color *) node->color;
			/* is this a Color or ColorRGBA color node? */
                	if (cc->_nodeType == NODE_Color) {
				glColorPointer (3,GL_FLOAT,0,cc->color.p);
			} else {
				glColorPointer (4,GL_FLOAT,0,cc->color.p);
			}
#endif
		} else {
			glColor3fv (thisColor);
		}
        	xc = (struct X3D_Coordinate *) node->coord;
		glVertexPointer (3,GL_FLOAT,0,xc->point.p);

		glMultiDrawElements (
			GL_LINE_STRIP,
			node->__vertexCount,
			GL_UNSIGNED_INT,
			node->__vertIndx,
			node->__segCount
		);

		glEnableClientState (GL_NORMAL_ARRAY);
		if (node->__colours) {
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
                	ConsoleMessage ("make_PointSet, coord node expected but not found");
			return;
              	} else {
                       	points = xc->point.p;
                       	npoints = xc->point.n;
               	}
       	}
	if (npoints <=0 ) return; /* nothing to do */
 

       	if (node->color) {
               	cc = (struct X3D_Color *) node->color;
               	if ((cc->_nodeType != NODE_Color) && (cc->_nodeType != NODE_ColorRGBA)) {
               	        ConsoleMessage ("make_PointSet, expected %d got %d\n", NODE_Color, cc->_nodeType);
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

	#ifdef RENDERVERBOSE
	printf("PointSet: %d %d\n", npoints, ncolors);
	#endif

	if (ncolors>=0) {
		glEnableClientState(GL_COLOR_ARRAY);
                cc = (struct X3D_Color *) node->color;
		/* is this a Color or ColorRGBA color node? */
               	if (cc->_nodeType == NODE_Color) {
			glColorPointer (3,GL_FLOAT,0,colors);
		} else {
			glColorPointer (4,GL_FLOAT,0,colors);
		}
	} else {
		glColor3fv (thisColor);
	}

	/* draw the shape */
	glDisableClientState (GL_NORMAL_ARRAY);

        xc = (struct X3D_Coordinate *) node->coord;
	glVertexPointer (3,GL_FLOAT,0,xc->point.p);
	glDrawArrays(GL_POINTS,0,npoints);

	/* put things back to normal */
	glEnableClientState(GL_NORMAL_ARRAY);
	if (ncolors>=0) {
		glDisableClientState(GL_COLOR_ARRAY);
	}


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

		/* create the index for the arrays. Really simple... Used to index
		   into the coords, so, eg, __vertArr is [0,1,2], which means use
		   coordinates 0, 1, and 2 */
		if (node->__vertArr) free ((void *)node->__vertArr);
		node->__vertArr = malloc (sizeof(GLuint)*(ncoord));
		if (!node->__vertArr) {
			printf ("can not malloc memory for LineSet vertArr\n");
			return;
		}
		pt = (GLint *)node->__vertArr;
		for (vtc = 0; vtc < ncoord; vtc++) {
			*pt=vtc; pt++; /* ie, index n contains the number n */
		}

		/* create the index for each line segment. What happens here is
		   that we create an array of pointers; each pointer points into
		   the __vertArr array - this gives a starting index for each line
		   segment The LENGTH of each segment (good question) comes from the
		   vertexCount parameter of the LineSet node */
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
			glEnableClientState(GL_COLOR_ARRAY);
                	cc = (struct X3D_Color *) node->color;
			/* is this a Color or ColorRGBA color node? */
                	if (cc->_nodeType == NODE_Color) {
				glColorPointer (3,GL_FLOAT,0,cc->color.p);
			} else {
				glColorPointer (4,GL_FLOAT,0,cc->color.p);
			}
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
