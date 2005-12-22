/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Navigation Component

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

#include "Bindable.h"
#include "Viewer.h"
#include "Collision.h"

extern struct sCollisionInfo OldCollisionInfo;

void prep_Viewpoint (struct X3D_Viewpoint *node) {
	double a1;

	if (!render_vp) return;

	 /* printf ("RVP, node %d ib %d sb %d gepvp\n",node,node->isBound,node->set_bind);
	 printf ("VP stack %d tos %d\n",viewpoint_tos, viewpoint_stack[viewpoint_tos]);
	 */

	/* check the set_bind eventin to see if it is TRUE or FALSE */
	/* code to perform binding is now in set_viewpoint. */

	if(!node->isBound) return;

	found_vp = 1; /* We found the viewpoint */

	/* perform Viewpoint translations */
	glRotated(-node->orientation.r[3]/PI*180.0,node->orientation.r[0],node->orientation.r[1],
		node->orientation.r[2]);
	glTranslated(-node->position.c[0],-node->position.c[1],-node->position.c[2]);

	/* now, lets work on the Viewpoint fieldOfView */
	glGetIntegerv(GL_VIEWPORT, viewPort);
	if(viewPort[2] > viewPort[3]) {
		a1=0;
		fieldofview = node->fieldOfView/3.1415926536*180;
	} else {
		a1 = node->fieldOfView;
		a1 = atan2(sin(a1),viewPort[2]/((float)viewPort[3]) * cos(a1));
		fieldofview = a1/3.1415926536*180;
	}
	calculateFrustumCone();
	/* printf ("render_Viewpoint, bound to %d, fieldOfView %f \n",node,node->fieldOfView); */
}

void prep_Billboard (struct X3D_Billboard *this_) {
	struct pt vpos, ax, cp, cp2, arcp;
	static const struct pt orig = {0.0, 0.0, 0.0};
	static const struct pt zvec = {0.0, 0.0, 1.0};
	struct orient viewer_orient;
	GLdouble mod[16];
	GLdouble proj[16];

	int align;
	double len, len2, angle;
	int sign;
	ax.x = this_->axisOfRotation.c[0];
	ax.y = this_->axisOfRotation.c[1];
	ax.z = this_->axisOfRotation.c[2];
	align = (APPROX(VECSQ(ax),0));

	quaternion_to_vrmlrot(&(Viewer.Quat),
		&(viewer_orient.x), &(viewer_orient.y),
		&(viewer_orient.z), &(viewer_orient.a));

	glPushMatrix();

	fwGetDoublev(GL_MODELVIEW_MATRIX, mod);
	fwGetDoublev(GL_PROJECTION_MATRIX, proj);
	gluUnProject(orig.x, orig.y, orig.z, mod, proj,
		viewport, &vpos.x, &vpos.y, &vpos.z);

	len = VECSQ(vpos);
	if (APPROX(len, 0)) { return; }
	VECSCALE(vpos, 1/sqrt(len));

	if (align) {
		ax.x = viewer_orient.x;
		ax.y = viewer_orient.y;
		ax.z = viewer_orient.z;
	}

	VECCP(ax, zvec, arcp);
	len = VECSQ(arcp);
	if (APPROX(len, 0)) { return; }

	len = VECSQ(ax);
	if (APPROX(len, 0)) { return; }
	VECSCALE(ax, 1/sqrt(len));

	VECCP(vpos, ax, cp); /* cp is now 90deg to both vector and axis */
	len = sqrt(VECSQ(cp));
	if (APPROX(len, 0)) {
		glRotatef(-viewer_orient.a/3.1415926536*180, ax.x, ax.y, ax.z);
		return;
	}
	VECSCALE(cp, 1/len);

	/* Now, find out angle between this and z axis */
	VECCP(cp, zvec, cp2);

	len2 = VECPT(cp, zvec); /* cos(angle) */
	len = sqrt(VECSQ(cp2)); /* this is abs(sin(angle)) */

	/* Now we need to find the sign first */
	if (VECPT(cp, arcp) > 0) { sign = -1; } else { sign = 1; }
	angle = atan2(len2, sign*len);

	glRotatef(angle/3.1415926536*180, ax.x, ax.y, ax.z);
}



void render_NavigationInfo (struct X3D_NavigationInfo *node) {
	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		if (node->set_bind == 1) set_naviinfo(node);

		bind_node (node, &navi_tos,&navi_stack[0]);
	}
	if(!node->isBound) return;
}

void fin_Billboard (struct X3D_Billboard *this_) {
	UNUSED(this_);
	glPopMatrix();
}


void child_Collision (struct X3D_Collision *this_) {
	int nc = (this_->children).n;
	int i;

	if(render_collision) {
		if((this_->collide) && !(this_->proxy)) {
			struct sCollisionInfo OldCollisionInfo = CollisionInfo;
			for(i=0; i<nc; i++) {
				void *p = ((this_->children).p[i]);
				#ifdef CHILDVERBOSE
				printf("RENDER COLLISION %d CHILD %d\n",this_, p);
				#endif
				render_node(p);
			}
			if((!APPROX(CollisionInfo.Offset.x,
					OldCollisionInfo.Offset.x)) ||
			   (!APPROX(CollisionInfo.Offset.y,
				   OldCollisionInfo.Offset.y)) ||
			   (!APPROX(CollisionInfo.Offset.z,
				    OldCollisionInfo.Offset.z))) {
			/* old code was:
			if(CollisionInfo.Offset.x != OldCollisionInfo.Offset.x ||
			   CollisionInfo.Offset.y != OldCollisionInfo.Offset.y ||
			   CollisionInfo.Offset.z != OldCollisionInfo.Offset.z) { */
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

		#ifdef CHILDVERBOSE
		printf("RENDER COLLISIONCHILD START %d (%d)\n",this_, nc);
		#endif
		/* do we have to sort this node? */
		if ((nc > 1 && !render_blend)) sortChildren(this_->children);

		/* do we have a DirectionalLight for a child? */
		if(this_->has_light) dirlightChildren(this_->children);

		/* now, just render the non-directionalLight children */
		normalChildren(this_->children);

		if (render_geom && (!render_blend)) {
			/* printf ("collisionChild, this is %d, extent %f %f %f\n",
			this_, this_->_extent[0], this_->_extent[1],
			this_->_extent[2]); */
			this_->bboxSize.c[0] = this_->_extent[0];
			this_->bboxSize.c[1] = this_->_extent[1];
			this_->bboxSize.c[2] = this_->_extent[2];
			BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
		}

		/* did we have that directionalLight? */
		if((this_->has_light)) glPopAttrib();

		#ifdef CHILDVERBOSE
		printf("RENDER COLLISIONCHILD END %d\n",this_);
		#endif
		curlight = savedlight;
	}
}


void child_LOD (struct X3D_LOD *this_) {
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
                /* printf ("LOD, mat %f %f %f\n",mod[12],mod[13],mod[14]); */
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

void child_InlineLoadControl (struct X3D_InlineLoadControl *this_) {
	int nc = (this_->children).n;
	int savedlight = curlight;
	struct X3D_Inline *inl;


	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	printf("RENDER INLINELOADCHILD START %d (%d)\n",this_, nc);
	#endif

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
	if ((nc > 1 && !render_blend)) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->children);

	if (render_geom && (!render_blend)) {
		/* printf ("inlineLODChild, this is %d, extent %f %f %f\n",
		this_, this_->_extent[0], this_->_extent[1],
		this_->_extent[2]); */
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	printf("RENDER INLINELOADCHILD END %d\n",this_);
	#endif

	curlight = savedlight;
}


void  child_Billboard (struct X3D_Billboard *this_) {
	int nc = (this_->children).n;
	int savedlight = curlight;


	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	printf("RENDER BILLBOARD START %d (%d)\n",this_, nc);
	#endif

	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->children);

	if (render_geom && (!render_blend)) {
		#ifdef CHILDVERBOSE
			printf ("BillboardChild, this is %d, extent %f %f %f\n",
			this_, this_->_extent[0], this_->_extent[1],
			this_->_extent[2]);
		#endif
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	printf("RENDER BILLBOARD END %d\n",this_);
	#endif

	curlight = savedlight;
}


void changed_Billboard (struct X3D_Billboard *this_) {
                int i;
                int nc = ((this_->children).n);
                struct X3D_Box *p;
                struct X3D_Virt *v;

                (this_->has_light) = 0;
                for(i=0; i<nc; i++) {
                        p = (struct X3D_Box *)((this_->children).p[i]);
                        if (p->_nodeType == NODE_DirectionalLight) {
                                /*  printf ("group found a light\n");*/
                                (this_->has_light) ++;
                        }
                }
}


void changed_Inline (struct X3D_Inline *this_) {
                int i;
                int nc = ((this_->__children).n);
                struct X3D_Box *p;
                struct X3D_Virt *v;

                (this_->has_light) = 0;
                for(i=0; i<nc; i++) {
                        p = (struct X3D_Box *)((this_->__children).p[i]);
                        if (p->_nodeType == NODE_DirectionalLight) {
                                /*  printf ("group found a light\n");*/
                                (this_->has_light) ++;
                        }
                }
}


void changed_Collision (struct X3D_Collision *this_) {
                int i;
                int nc = ((this_->children).n);
                struct X3D_Box *p;
                struct X3D_Virt *v;

                (this_->has_light) = 0;
                for(i=0; i<nc; i++) {
                        p = (struct X3D_Box *)((this_->children).p[i]);
                        if (p->_nodeType == NODE_DirectionalLight) {
                                /*  printf ("group found a light\n");*/
                                (this_->has_light) ++;
                        }
                }
}


void changed_InlineLoadControl (struct X3D_InlineLoadControl *this_) {
                int i;
                int nc = ((this_->children).n);
                struct X3D_Box *p;
                struct X3D_Virt *v;

                (this_->has_light) = 0;
                for(i=0; i<nc; i++) {
                        p = (struct X3D_Box *)((this_->children).p[i]);
                        if (p->_nodeType == NODE_DirectionalLight) {
                                /*  printf ("group found a light\n");*/
                                (this_->has_light) ++;
                        }
                }
}


