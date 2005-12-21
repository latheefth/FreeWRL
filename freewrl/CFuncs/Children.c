/*******************************************************************
 Copyright (C) 2004 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*********************************************************************
 * Render the children of nodes.
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
#include "Collision.h"

#ifdef CHILDVERBOSE
static int VerboseIndent = 0;

static void VerboseStart (char *whoami, struct X3D_Box *me, int nc) {
	int c;

	for (c=0; c<VerboseIndent; c++) printf ("  ");
	printf ("RENDER %s START %d nc %d PIV %d ext %4.2f %4.2f %4.2f\n",
			whoami,me,nc,me->PIV,me->_extent[0],me->_extent[1],
			me->_extent[2]);
	VerboseIndent++;
}

static void VerboseEnd (char *whoami) {
	int c;

	VerboseIndent--;
	for (c=0; c<VerboseIndent; c++) printf ("  ");
	printf ("RENDER %s END\n",whoami);
}
#endif


/* sort children - use bubble sort with early exit flag */
void sortChildren (struct Multi_Node ch) {
	int i,j;
	int nc;
	int noswitch;
	struct X3D_Box *a, *b, *c;

	/* simple, inefficient bubble sort */
	/* this is a fast sort when nodes are already sorted;
	   may wish to go and "QuickSort" or so on, when nodes
	   move around a lot. (Bubblesort is bad when nodes
	   have to be totally reversed) */

	nc = ch.n;

	for(i=0; i<nc; i++) {
		noswitch = TRUE;
		for (j=(nc-1); j>i; j--) {
			/* printf ("comparing %d %d\n",i,j); */
			a = (struct X3D_Box *)ch.p[j-1];
			b = (struct X3D_Box *)ch.p[j];

			if (a->_dist > b->_dist) {
				/* printf ("have to switch %d %d\n",i,j); */
				c = a;
				ch.p[j-1] = b;
				ch.p[j] = c;
				noswitch = FALSE;
			}
		}
		/* did we have a clean run? */
		if (noswitch) {
			break;
		}
	}
	/* for(i=0; i<nc; i++) {
		b = ch.p[i]);
		printf ("child %d %d %f\n",i,b,b->_dist);
	} */
}

/* this grouping node has a DirectionalLight for a child, render this first */
void dirlightChildren(struct Multi_Node ch) {
	int i;

	glPushAttrib(GL_LIGHTING_BIT|GL_ENABLE_BIT);
	for(i=0; i<ch.n; i++) {
		struct X3D_Box *p = (struct X3D_Box *)ch.p[i];
		if (p->_nodeType == NODE_DirectionalLight)
			render_node(p);
	}
}

/* render all children, except the directionalight ones */
void normalChildren(struct Multi_Node ch) {
	int i;

	for(i=0; i<ch.n; i++) {
		struct X3D_Box *p = (struct X3D_Box *)ch.p[i];
		if(p->_nodeType != NODE_DirectionalLight) {
		/* printf ("normalchildren, child %d, piv %d\n",p,p->PIV); */
/*			if ((p->PIV) > 0) */
				render_node(p);
		}
	}
}

/* propagate flags up the scene graph */
/* used to tell the rendering pass that, there is/used to be nodes
 * of interest down the branch. Eg, Transparent nodes - no sense going
 * through it all when rendering only for nodes. */

void update_renderFlag(void *ptr, int flag) {
	struct X3D_Box *p = (struct X3D_Box *)ptr;
	int i;

	/* send notification up the chain */
	/* printf ("start of update_RenderFlag for %d parents %d\n",p, p->_nparents); */
	p->_renderFlags = p->_renderFlags | flag;

	for (i = 0; i < p->_nparents; i++) {
		/* printf ("node %d has %d for a parent\n",p,p->_parents[i]); */
		update_renderFlag(p->_parents[i],flag);
	}
	/* printf ("finished update_RenderFlag for %d\n",p); */
}
