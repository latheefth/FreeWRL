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

static int ChildVerbose = 0;
static int VerboseIndent = 0;

static void VerboseStart (char *whoami, struct VRML_Box *me, int nc) {
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

/* sort children - use bubble sort with early exit flag */
void sortChildren (struct Multi_Node ch) { 
	int i,j;
	int nc;
	int noswitch;
	struct VRML_Box *a, *b, *c;

	//printf ("have to sort %d, nc %d\n",this_, nc);
	/* simple, inefficient bubble sort */
	/* this is a fast sort when nodes are already sorted;
	   may wish to go and "QuickSort" or so on, when nodes
	   move around a lot. (Bubblesort is bad when nodes
	   have to be totally reversed) */

	nc = ch.n;
	//printf ("sortChildren, have %d children\n",nc);

	for(i=0; i<nc; i++) {
		noswitch = TRUE;
		for (j=(nc-1); j>i; j--) {
			//printf ("comparing %d %d\n",i,j);
			a = ch.p[j-1];
			b = ch.p[j];

			if (a->_dist > b->_dist) {
				//printf ("have to switch %d %d\n",i,j);
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
	//for(i=0; i<nc; i++) {
	//	b = ch.p[i]);
	//	printf ("child %d %d %f\n",i,b,b->_dist);
	//}
}

/* this grouping node has a DirectionalLight for a child, render this first */
void dirlightChildren(struct Multi_Node ch) {
	int i;

	glPushAttrib(GL_LIGHTING_BIT|GL_ENABLE_BIT);
	for(i=0; i<ch.n; i++) {
		struct VRML_Box *p = ch.p[i];
		struct VRML_Virt *v = *(struct VRML_Virt **)p;

		if(v->rend == DirectionalLight_Rend) 
			render_node(p);
	}
}

/* render all children, except the directionalight ones */
void normalChildren(struct Multi_Node ch) {
	int i;

	for(i=0; i<ch.n; i++) {
		struct VRML_Box *p = ch.p[i];
		struct VRML_Virt *v = *(struct VRML_Virt **)p;
		/* Hmm - how much time does this consume? */
		/* Not that much. */
		if(v->rend != DirectionalLight_Rend) {
//		printf ("normalchildren, child %d, piv %d\n",p,p->PIV);
//			if ((p->PIV) > 0)
				render_node(p);
		}
	}
}
  
/********************************************************************
 * now, nodes called from VRMLFunc.[c,xs] during rendering process
 * *****************************************************************/
void groupingChild (struct VRML_Group *this_) {
	int nc = ((this_->children).n); 
	int savedlight = curlight;

	if(ChildVerbose) VerboseStart ("GROUP", (struct VRML_Box *)this_, nc);

	/* should we go down here? */
	//printf ("Group, rb %x VF_B %x, flags %x\n",render_blend, VF_Blend, this_->_renderFlags);
	if (render_blend == VF_Blend) 
		if ((this_->_renderFlags & VF_Blend) != VF_Blend) {
			if(ChildVerbose) VerboseEnd ("GROUP");
			return;
		}
	if (render_proximity == VF_Proximity) 
		if ((this_->_renderFlags & VF_Proximity) != VF_Proximity)  {
			if(ChildVerbose) VerboseEnd ("GROUP");
			return;
		}

	/* do we have to sort this node? */
	if ((nc > 2 && render_blend)) sortChildren(this_->children);
	
	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->children);
	
	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		//if (ChildVerbose) 
		//	printf ("GroupingChild, this is %d, extent %f %f %f\n",
		//	this_, this_->_extent[0], this_->_extent[1],
		//	this_->_extent[2]);
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];

		/* pass the bounding box calculations on up the chain */
		propagateExtent((float)0.0,(float)0.0,(float)0.0,this_);
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();
	
	if(ChildVerbose) VerboseEnd ("GROUP");

	curlight = savedlight;
}


void billboardChild (struct VRML_Billboard *this_) {
	int nc = (this_->children).n; 
	int savedlight = curlight;

	if(ChildVerbose) printf("RENDER BILLBOARD START %d (%d)\n",this_, nc);

	/* do we have to sort this node? */
	if ((nc > 2 && render_blend)) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->children);

	if (render_geom && (!render_blend)) {
		if (ChildVerbose) 
			printf ("BillboardChild, this is %d, extent %f %f %f\n",
			this_, this_->_extent[0], this_->_extent[1],
			this_->_extent[2]);
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();
	
	if(ChildVerbose) printf("RENDER BILLBOARD END %d\n",this_);

	curlight = savedlight;
}
void transformChild (struct VRML_Transform *this_) {
	int nc = (this_->children).n; 
	int savedlight = curlight;

	if(ChildVerbose) VerboseStart ("TRANSFORM",(struct VRML_Box *)this_, nc);

	/* should we go down here? */
	//printf("transformChild %d render_blend %x renderFlags %x\n",
	//		this_, render_blend, this_->_renderFlags);
	if (render_blend == VF_Blend) 
		if ((this_->_renderFlags & VF_Blend) != VF_Blend) {
			if(ChildVerbose) VerboseEnd ("TRANSFORM");
			return;
		}
	if (render_proximity == VF_Proximity) 
		if ((this_->_renderFlags & VF_Proximity) != VF_Proximity) {
			if(ChildVerbose) VerboseEnd ("TRANSFORM");
			return;
		}



#ifdef BOUNDINGBOX
	if (this_->PIV > 0) {
#endif
	/* do we have to sort this node? */
	if ((nc > 2 && render_blend)) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */

	//printf ("Transform %d, flags %d, render_sensitive %d\n",
	//		this_,this_->_renderFlags,render_sensitive);

	normalChildren(this_->children);
#ifdef BOUNDINGBOX
	}
#endif

	if (render_geom && (!render_blend)) {
		//if (ChildVerbose)
		//	printf ("TransformChild, this is %d, extent %f %f %f\n",
		//	this_, this_->_extent[0], this_->_extent[1],
		//	this_->_extent[2]);
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];
		this_->bboxCenter.c[0] = this_->translation.c[0];
		this_->bboxCenter.c[1] = this_->translation.c[1];
		this_->bboxCenter.c[2] = this_->translation.c[2];

		/* pass the bounding box calculations on up the chain */
		propagateExtent(this_->bboxCenter.c[0],
				this_->bboxCenter.c[1],
				this_->bboxCenter.c[2],
				this_);
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}


	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();
	
	if(ChildVerbose) VerboseEnd ("TRANSFORM");

	curlight = savedlight;
}
void anchorChild (struct VRML_Anchor *this_) {
	int nc = (this_->children).n; 
	int savedlight = curlight;

	if(ChildVerbose) printf("RENDER ANCHOR START %d (%d)\n",this_, nc);

	/* do we have to sort this node? */
	if ((nc > 2 && render_blend)) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->children);

	if (render_geom && (!render_blend)) {
		//printf ("anchorChild, this is %d, extent %f %f %f\n",
		//this_, this_->_extent[0], this_->_extent[1],
		//this_->_extent[2]);
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();
	
	if(ChildVerbose) printf("RENDER ANCHOR END %d\n",this_);

	curlight = savedlight;
}
void geolocationChild (struct VRML_GeoLocation *this_) {
	int nc = (this_->children).n; 
	int savedlight = curlight;

	if(ChildVerbose) printf("RENDER GEOLOCATION START %d (%d)\n",this_, nc);

	/* do we have to sort this node? */
	if ((nc > 2 && render_blend)) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->children);

	if (render_geom && (!render_blend)) {
		//printf ("geoLocationChild, this is %d, extent %f %f %f\n",
		//this_, this_->_extent[0], this_->_extent[1],
		//this_->_extent[2]);
		//this_->bboxSize.c[0] = this_->_extent[0];
		//this_->bboxSize.c[1] = this_->_extent[1];
		//this_->bboxSize.c[2] = this_->_extent[2];
		//BoundingBox(this_->bboxCenter,this_->bboxSize);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();
	
	if(ChildVerbose) printf("RENDER GEOLOCATION END %d\n",this_);

	curlight = savedlight;
}


void inlineChild (struct VRML_Inline *this_) {
	int nc = (this_->__children).n; 
	int savedlight = curlight;

	if(ChildVerbose) {printf("RENDER INLINE START %d (%d)\n",this_, nc);}

	/* lets see if we still have to load this one... */
	if ((this_->__loadstatus)==0) loadInline(this_);

	/* do we have to sort this node? */
	if ((nc > 2 && render_blend)) sortChildren(this_->__children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->__children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->__children);

	if (render_geom && (!render_blend)) {
		//printf ("inlineChild, this is %d, extent %f %f %f\n",
		//this_, this_->_extent[0], this_->_extent[1],
		//this_->_extent[2]);
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();

	if(ChildVerbose) {printf("RENDER INLINE END %d\n",this_);}

	curlight = savedlight;
}


void inlinelodChild (struct VRML_InlineLoadControl *this_) {
	int nc = (this_->children).n; 
	int savedlight = curlight;
	struct VRML_Inline *inl;

	if(ChildVerbose) {printf("RENDER INLINELOADCHILD START %d (%d)\n",this_, nc);}

	/* lets see if we still have to load this one... */
	if (((this_->__loadstatus)==0) && (this_->load)) {
		/* treat this as an inline; copy params over */
		inl->url = this_->url;
		inl->__children = this_->children;
		inl->__parenturl = this_->__parenturl;
		inl->__loadstatus = this_->__loadstatus;
		
		loadInline(inl);
		
		this_->url = inl->url;
		this_->children = inl->__children;
		this_->__parenturl = inl->__parenturl;
		this_->__loadstatus = inl->__loadstatus;
	} else if (!(this_->load) && ((this_->__loadstatus) != 0)) {
		printf ("InlineLoadControl, removing children\n");
		this_->children.n = 0;
		free (this_->children.p);
		this_->__loadstatus = 0;
	}

	/* do we have to sort this node? */
	if ((nc > 2 && render_blend)) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->children);

	if (render_geom && (!render_blend)) {
		//printf ("inlineLODChild, this is %d, extent %f %f %f\n",
		//this_, this_->_extent[0], this_->_extent[1],
		//this_->_extent[2]);
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();

	if(ChildVerbose) printf("RENDER INLINELOADCHILD END %d\n",this_);

	curlight = savedlight;
}

void lodChild (struct VRML_LOD *this_) {
	GLdouble mod[16];
	GLdouble proj[16];
	struct pt vec;
	double dist;
	int nran = (this_->range).n;
	int nnod = (this_->level).n;
	int i;
	void *p;

	if(!nran) {
		void *p = (this_->level).p[0];
		render_node(p);
		return;
	}

	/* calculate which one to display - only do this once per eventloop. */
	if (render_geom && (!render_blend)) {
		fwGetDoublev(GL_MODELVIEW_MATRIX, mod);
		//printf ("LOD, mat %f %f %f\n",mod[12],mod[13],mod[14]);
		fwGetDoublev(GL_PROJECTION_MATRIX, proj);
		gluUnProject(0,0,0,mod,proj,viewport,
			&vec.x,&vec.y,&vec.z);
		vec.x -= (this_->center).c[0];
		vec.y -= (this_->center).c[1];
		vec.z -= (this_->center).c[2];

		dist = sqrt(VECSQ(vec));
		i = 0;

		while (i<nran) {
			if(dist < ((this_->range).p[i])) { break; }
			i++;
		}
		if(i >= nnod) i = nnod-1;
		this_->_selected = i;
	}
	p = (this_->level).p[this_->_selected];
	render_node(p);
}


void collisionChild(struct VRML_Collision *this_) {
	int nc = (this_->children).n; 
	int i;
	
	if(render_collision) {
		if((this_->collide) && !(this_->proxy)) {
			struct sCollisionInfo OldCollisionInfo = CollisionInfo;
			for(i=0; i<nc; i++) {
				void *p = ((this_->children).p[i]);
				if(ChildVerbose) {printf("RENDER COLLISION %d CHILD %d\n",this_, p);}
				render_node(p);
			}
			if(CollisionInfo.Offset.x != OldCollisionInfo.Offset.x ||
			   CollisionInfo.Offset.y != OldCollisionInfo.Offset.y ||
			   CollisionInfo.Offset.z != OldCollisionInfo.Offset.z) {
				/*collision occured
				 * bit 0 gives collision, bit 1 gives change */
				this_->__hit = (this_->__hit & 1) ? 1 : 3;
			} else
				this_->__hit = (this_->__hit & 1) ? 2 : 0;

		}
        	        if(this_->proxy) 
                        render_node(this_->proxy);

	} else { /*standard group behaviour*/
		int savedlight = curlight;

		if(ChildVerbose) {printf("RENDER COLLISIONCHILD START %d (%d)\n",this_, nc);}

		/* do we have to sort this node? */
		if ((nc > 2 && render_blend)) sortChildren(this_->children);

		/* do we have a DirectionalLight for a child? */
		if(this_->has_light) dirlightChildren(this_->children);

		/* now, just render the non-directionalLight children */
		normalChildren(this_->children);

		if (render_geom && (!render_blend)) {
			//printf ("collisionChild, this is %d, extent %f %f %f\n",
			//this_, this_->_extent[0], this_->_extent[1],
			//this_->_extent[2]);
			this_->bboxSize.c[0] = this_->_extent[0];
			this_->bboxSize.c[1] = this_->_extent[1];
			this_->bboxSize.c[2] = this_->_extent[2];
			BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
		}

		/* did we have that directionalLight? */
		if((this_->has_light)) glPopAttrib();
	
		if(ChildVerbose) {printf("RENDER COLLISIONCHILD END %d\n",this_);}
		curlight = savedlight;
	}
}




/* propagate flags up the scene graph */
/* used to tell the rendering pass that, there is/used to be nodes
 * of interest down the branch. Eg, Transparent nodes - no sense going
 * through it all when rendering only for nodes. */

void update_renderFlag(void *ptr, int flag) {
	struct VRML_Box *p = ptr;
	int i;

	/* send notification up the chain */
	//printf ("start of update_RenderFlag for %d parents %d\n",p,
	//		p->_nparents);
	p->_renderFlags = p->_renderFlags | flag;

	for (i = 0; i < p->_nparents; i++) {
		//printf ("node %d has %d for a parent\n",p,p->_parents[i]);
		update_renderFlag(p->_parents[i],flag);
	}
	//printf ("finished update_RenderFlag for %d\n",p);
}
