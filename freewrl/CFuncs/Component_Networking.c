/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Networking Component

*********************************************************************/

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
#include "installdir.h"

void render_LoadSensor (struct X3D_LoadSensor *node) {
	int total;
	int count;
	int nowLoading;
	int nowFinished;
	struct X3D_ImageTexture *tnode;
	struct X3D_MovieTexture *mnode;
	struct X3D_AudioClip *anode;
	struct X3D_Inline *inode;
	
	/* if not enabled, do nothing */
	if (!node) return;
	if (!node->enabled) return;

	/* we only need to look at this during the rendering pass - once per event loop */
	if (!render_geom) return;

	/* do we need to re-generate our internal variables? */
	if (node->_change != node->_ichange) {
		node->_ichange = node->_change;
		node->__loading = 0;
		node->__finishedloading = 0;
		node->progress = 0.0;
		node->__StartLoadTime = 0.0;
	}

	/* do we actually have any nodes to watch? */
	if (node->watchList.n<=0) return;

	/* are all nodes loaded? */
	if (node->__finishedloading == node->watchList.n) return;

	/* our current status... */
	nowLoading = 0;
	nowFinished = 0;

	/* go through node list, and check to see what the status is */
	/* printf ("have %d nodes to watch\n",node->watchList.n); */
	for (count = 0; count < node->watchList.n; count ++) {

		tnode = (struct X3D_ImageTexture *) node->watchList.p[count];

		/* printf ("node type of node %d is %d\n",count,tnode->_nodeType); */
		switch (tnode->_nodeType) {
		case NODE_ImageTexture:
			/* printf ("opengl tex is %d\n",tnode->__texture); */
			/* is this texture thought of yet? */
			if (tnode->__texture > 0) {
				nowLoading++;
				/* is it finished loading? */
				if (isTextureLoaded(tnode->__texture)) nowFinished ++;
			}
				
			break;

		case NODE_MovieTexture:
			mnode = (struct X3D_MovieTexture *) tnode; /* change type to MovieTexture */
			/* printf ("opengl tex is %d\n",mnode->__texture0_); */
			/* is this texture thought of yet? */
			if (mnode->__texture0_ > 0) {
				nowLoading++;
				/* is it finished loading? */
				if (isTextureLoaded(mnode->__texture0_)) nowFinished ++;
			}
				
			break;

		case NODE_Inline:
			inode = (struct X3D_Inline *) tnode; /* change type to Inline */
			printf ("LoadSensor, Inline %d, type %d loadstatus %d at %d\n",inode,inode->_nodeType,inode->__loadstatus, &inode->__loadstatus);
			break;

		case NODE_Script:
			nowLoading ++; /* broken - assume that the url is ok for now */
			break;

		case NODE_AudioClip:
			anode = (struct X3D_AudioClip *) tnode; /* change type to AudioClip */
			/* AudioClip sourceNumber will be gt -1 if the clip is ok. see code for details */
			if (anode->__sourceNumber > -1) nowLoading ++;

			break;

		default :{} /* there should never be anything here, but... */
		}
	}
		

	/* ok, are we NOW finished loading? */
	if (nowFinished == node->watchList.n) {
		node->isActive = 0;
		mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

		node->isLoaded = 1;
		mark_event (node, offsetof (struct X3D_LoadSensor, isLoaded));

		node->progress = 1.0;
		mark_event (node, offsetof (struct X3D_LoadSensor, progress));

		node->loadTime = TickTime;
		mark_event (node, offsetof (struct X3D_LoadSensor, loadTime));
	}	

	/* have we NOW started loading? */
	if ((nowLoading > 0) && (node->__loading == 0)) {
		/* mark event isActive TRUE */
		node->isActive = 1;
		mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

	
		node->__StartLoadTime = TickTime;
	}
	
	/* what is our progress? */
	if (node->isActive == 1) {
		node->progress = (float)(nowFinished)/(float)(node->watchList.n);
		mark_event (node, offsetof (struct X3D_LoadSensor, progress));
	}

	/* remember our status for next time. */
	node->__loading = nowLoading;
	node->__finishedloading = nowFinished;

	/* did we run out of time? */
	if (node->timeOut > 0.0001) {			/* we have a timeOut specified */
		if (node->__StartLoadTime > 0.001) {	/* we have a start Time recorded from the isActive = TRUE */
		
			/* ok, we should look at time outs */
			if ((TickTime - node->__StartLoadTime) > node->timeOut) {
				node->isLoaded = 0;
				mark_event (node, offsetof (struct X3D_LoadSensor, isLoaded));

				node->isActive = 0;
				mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

				/* and, we will just assume that we have loaded everything next iteration */
				node->__finishedloading = node->watchList.n;
			}
		}
	}
}


void child_Anchor (struct X3D_Anchor *node) {
	int nc = (node->children).n;
	DIRECTIONAL_LIGHT_SAVE


	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	printf("RENDER ANCHOR START %d (%d)\n",node, nc);
	#endif

	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);

	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
		BoundingBox(node->bboxCenter,node->bboxSize);
	}

	/* did we have that directionalLight? */
	if((node->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	printf("RENDER ANCHOR END %d\n",node);
	#endif

	DIRECTIONAL_LIGHT_OFF
}



void child_Inline (struct X3D_Inline *node) {
	int nc = (node->__children).n;
	DIRECTIONAL_LIGHT_SAVE

	#ifdef CHILDVERBOSE
	printf("RENDER INLINE START %d (%d)\n",node, nc);
	#endif

	/* lets see if we still have to load this one... */
	if ((node->__loadstatus)==0) loadInline(node);


	/* any children at all? */
	if (nc==0) return; 

	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->__children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->__children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->__children);

	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
		BoundingBox(node->bboxCenter,node->bboxSize);
	}

	/* did we have that directionalLight? */
	if((node->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	printf("RENDER INLINE END %d\n",node);
	#endif

	DIRECTIONAL_LIGHT_OFF
}

void changed_Anchor (struct X3D_Anchor *node) {
                int i;
                int nc = ((node->children).n);
                struct X3D_Box *p;
                struct X3D_Virt *v;

                (node->has_light) = 0;
                for(i=0; i<nc; i++) {
                        p = (struct X3D_Box *)((node->children).p[i]);
                        if (p->_nodeType == NODE_DirectionalLight) {
                                /*  printf ("group found a light\n");*/
                                (node->has_light) ++;
                        }
                }
}

