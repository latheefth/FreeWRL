/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Grouping Component

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


void prep_Transform (struct X3D_Transform *this_) {
	GLfloat my_rotation;
	GLfloat my_scaleO=0;
	int	recalculate_dist;

        /* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
         * so we do nothing here in that case -ncoder */

	 /* we recalculate distance on last pass, or close to it, and only
	 once per event-loop tick. we can do it on the last pass - the
	 render_sensitive pass, but when mouse is clicked (eg, moving in
	 examine mode, sensitive node code is not rendered. So, we choose
	 the second-last pass. ;-) */
	recalculate_dist = render_light;

	/* printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

	if(!render_vp) {
                /* glPushMatrix();*/
		fwXformPush(this_);

		/* might we have had a change to a previously ignored value? */
		if (this_->_change != this_->_dlchange) {
			/* printf ("re-rendering for %d\n",this_);*/
			this_->__do_center = verify_translate ((GLfloat *)this_->center.c);
			this_->__do_trans = verify_translate ((GLfloat *)this_->translation.c);
			this_->__do_scale = verify_scale ((GLfloat *)this_->scale.c);
			this_->__do_rotation = verify_rotate ((GLfloat *)this_->rotation.r);
			this_->__do_scaleO = verify_rotate ((GLfloat *)this_->scaleOrientation.r);
			this_->_dlchange = this_->_change;
		}



		/* TRANSLATION */
		if (this_->__do_trans)
			glTranslatef(this_->translation.c[0],this_->translation.c[1],this_->translation.c[2]);

		/* CENTER */
		if (this_->__do_center)
			glTranslatef(this_->center.c[0],this_->center.c[1],this_->center.c[2]);

		/* ROTATION */
		if (this_->__do_rotation) {
			my_rotation = this_->rotation.r[3]/3.1415926536*180;
			glRotatef(my_rotation,
				this_->rotation.r[0],this_->rotation.r[1],this_->rotation.r[2]);
		}

		/* SCALEORIENTATION */
		if (this_->__do_scaleO) {
			my_scaleO = this_->scaleOrientation.r[3]/3.1415926536*180;
			glRotatef(my_scaleO, this_->scaleOrientation.r[0],
				this_->scaleOrientation.r[1],this_->scaleOrientation.r[2]);
		}


		/* SCALE */
		if (this_->__do_scale)
			glScalef(this_->scale.c[0],this_->scale.c[1],this_->scale.c[2]);

		/* REVERSE SCALE ORIENTATION */
		if (this_->__do_scaleO)
			glRotatef(-my_scaleO, this_->scaleOrientation.r[0],
				this_->scaleOrientation.r[1],this_->scaleOrientation.r[2]);

		/* REVERSE CENTER */
		if (this_->__do_center)
			glTranslatef(-this_->center.c[0],-this_->center.c[1],-this_->center.c[2]);

		/* did either we or the Viewpoint move since last time? */
		if (recalculate_dist) {
			/* printf ("calling PointInView for %d\n",this_);*/
			this_->PIV = PointInView(this_);
			/* printf ("ppv %d\n",this_->PIV);*/

	       }
        }
}

void fin_Transform (struct X3D_Transform *this_) {
        if(!render_vp) {
            /* glPopMatrix();*/
            fwXformPop(this_);
        } else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if(found_vp) {
                glTranslatef(((this_->center).c[0]),((this_->center).c[1]),((this_->center).c[2])
                );
                glRotatef(((this_->scaleOrientation).r[3])/3.1415926536*180,((this_->scaleOrientation).r[0]),((this_->scaleOrientation).r[1]),((this_->scaleOrientation).r[2])
                );
                glScalef(1.0/(((this_->scale).c[0])),1.0/(((this_->scale).c[1])),1.0/(((this_->scale).c[2]))
                );
                glRotatef(-(((this_->scaleOrientation).r[3])/3.1415926536*180),((this_->scaleOrientation).r[0]),((this_->scaleOrientation).r[1]),((this_->scaleOrientation).r[2])
                );
                glRotatef(-(((this_->rotation).r[3]))/3.1415926536*180,((this_->rotation).r[0]),((this_->rotation).r[1]),((this_->rotation).r[2])
                );
                glTranslatef(-(((this_->center).c[0])),-(((this_->center).c[1])),-(((this_->center).c[2]))
                );
                glTranslatef(-(((this_->translation).c[0])),-(((this_->translation).c[1])),-(((this_->translation).c[2]))
                );
            }
        }
} 

void child_Switch (struct X3D_Switch *this_) {
                /* exceedingly simple - render only one child */
                int wc = (this_->whichChoice) /*cget*/;
                if(wc >= 0 && wc < ((this_->choice).n)) {
                        void *p = ((this_->choice).p[wc]);
                        render_node(p);
                }
}


void child_StaticGroup (struct X3D_StaticGroup *this_) {
	int nc = ((this_->children).n);
	int savedlight = curlight;
	int createlist = FALSE;

	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	VerboseStart ("STATICGROUP", (struct X3D_Box *)this_, nc);
	#endif

	/* should we go down here? */
/*	printf ("staticGroup, rb %x VF_B %x, rg  %x VF_G %x\n",render_blend, VF_Blend, render_geom, VF_Geom); 
       printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
        render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);
*/

	if ((!render_geom) && (!render_light)) {
		/* printf ("staticGroup, returning short, no light or geom\n"); */
		return;
	}

	if (render_geom) {
		if (render_blend==VF_Blend) {
			if (this_->__transparency < 0) {
				/* printf ("creating transparency display list %d\n",this_->__transparency); */
				this_->__transparency  = glGenLists(1);
				createlist = TRUE;
				glNewList(this_->__transparency,GL_COMPILE_AND_EXECUTE);
			} else {
				/* printf ("calling transparency list\n"); */
				glCallList (this_->__transparency);
				return;
			}

		} else {
			if  (this_->__solid <0 ) {
				/* printf ("creating solid display list\n"); */
				this_->__solid  = glGenLists(1);
				createlist = TRUE;
				glNewList(this_->__solid,GL_COMPILE_AND_EXECUTE);
			} else {
				/* printf ("calling solid list\n"); */
				glCallList (this_->__solid);
				/* do we have to tell the MainLoop that we have transparency down here? */
				/* is the transparency display list created? */
				if (this_->__transparency > -1) have_transparency++;
				return;
			}
		}
	}


	if (render_blend == VF_Blend)
		if ((this_->_renderFlags & VF_Blend) != VF_Blend) {
			#ifdef CHILDVERBOSE
			VerboseEnd ("STATICGROUP");
			#endif
			if (createlist) glEndList();
			return;
		}



	/* do we have to sort this node? Only if not a proto - only first node has visible children. */
	if ((nc > 1)  && !render_blend) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	normalChildren(this_->children);


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		#ifdef CHILDVERBOSE
			printf ("staticGroupingChild, this is %d, extent %f %f %f\n",
			this_, this_->_extent[0], this_->_extent[1],
			this_->_extent[2]);
		#endif

		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];
		

		/* pass the bounding box calculations on up the chain */
		propagateExtent((float)0.0,(float)0.0,(float)0.0,(struct X3D_Box *)this_);
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}


	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	VerboseEnd ("STATICGROUP");
	#endif


			if (createlist) glEndList();

	curlight = savedlight;
}


void child_Group (struct X3D_Group *this_) {
	int nc = ((this_->children).n);
	int savedlight = curlight;

	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	VerboseStart ("GROUP", (struct X3D_Box *)this_, nc);
	#endif
/*
{
int x;
struct X3D_Box *xx;

printf ("child_Group, this %d\n",this_);
for (x=0; x<nc; x++) {
xx = (struct X3D_Box *)this_->children.p[x];

printf ("	ch %d type %s\n",this_->children.p[x],stringNodeType(xx->_nodeType));
}
}
*/

		

	/* should we go down here? */
	/* printf ("Group, rb %x VF_B %x, rg  %x VF_G %x\n",render_blend, VF_Blend, render_geom, VF_Geom); */
	if (render_blend == VF_Blend)
		if ((this_->_renderFlags & VF_Blend) != VF_Blend) {
			#ifdef CHILDVERBOSE
			VerboseEnd ("GROUP");
			#endif
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((this_->_renderFlags & VF_Proximity) != VF_Proximity)  {
			#ifdef CHILDVERBOSE
			VerboseEnd ("GROUP");
			#endif
			return;
		}

	/* do we have to sort this node? Only if not a proto - only first node has visible children. */
	if ((this_->__isProto == 0) && (nc > 1)  && !render_blend) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */
	if ((this_->__isProto == 1) && render_geom) {
		(this_->children).n = 1;
		normalChildren(this_->children);
		(this_->children).n = nc;
	} else {
		normalChildren(this_->children);
	}


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		#ifdef CHILDVERBOSE
			printf ("GroupingChild, this is %d, extent %f %f %f\n",
			this_, this_->_extent[0], this_->_extent[1],
			this_->_extent[2]);
		#endif
		this_->bboxSize.c[0] = this_->_extent[0];
		this_->bboxSize.c[1] = this_->_extent[1];
		this_->bboxSize.c[2] = this_->_extent[2];

		/* pass the bounding box calculations on up the chain */
		propagateExtent((float)0.0,(float)0.0,(float)0.0,(struct X3D_Box *)this_);
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}

	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	VerboseEnd ("GROUP");
	#endif


	curlight = savedlight;
}


void child_Transform (struct X3D_Transform *this_) {
	int nc = (this_->children).n;
	int savedlight = curlight;

	/* Fast collision with BoundingBox during render_collision phase */
	GLdouble awidth = naviinfo.width; /*avatar width*/
	GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/

	GLdouble modelMatrix[16];
	GLdouble upvecmat[16]; 
	struct pt iv = {0,0,0};
	struct pt jv = {0,0,0};
	struct pt kv = {0,0,0};
	struct pt ov = {0,0,0};

	struct pt t_orig = {0,0,0};
	GLdouble scale; /* FIXME: wont work for non-uniform scales. */

	struct pt delta;
	struct pt tupv = {0,1,0};


	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	VerboseStart ("TRANSFORM",(struct X3D_Box *)this_, nc);
	#endif

	/* Check to see if we have to check for collisions for this transform. */
	if (render_collision) {
		iv.x = this_->_extent[0]/2.0;
		jv.y = this_->_extent[1]/2.0;
		kv.z = this_->_extent[2]/2.0;
		ov.x = -(this_->_extent[0]); 
		ov.y = -(this_->_extent[1]); 
		ov.z = -(this_->_extent[2]);

	       /* get the transformed position of the Box, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       /* matinverse(upvecmat,upvecmat); */

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
		/* printf ("TB this %d, extent %4.3f %4.3f %4.3f pos %4.3f %4.3f %4.3f\n", 
			this_,this_->_extent[0],this_->_extent[1],this_->_extent[2],
			t_orig.x,t_orig.y,t_orig.z); */
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_box_intersect(abottom,atop,awidth,t_orig,
			scale*this_->_extent[0]*2,
			scale*this_->_extent[1]*2,
			scale*this_->_extent[2]*2)) {
			/* printf ("TB this %d returning fast\n",this_); */
			return;
		/* } else {
			printf ("TB really look at %d\n",this_); */
		}
	}

	/* should we go down here? */
	/* printf("transformChild %d render_blend %x renderFlags %x\n",
			this_, render_blend, this_->_renderFlags); */
	if (render_blend == VF_Blend)
		if ((this_->_renderFlags & VF_Blend) != VF_Blend) {
			#ifdef CHILDVERBOSE
			VerboseEnd ("TRANSFORM");
			#endif
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((this_->_renderFlags & VF_Proximity) != VF_Proximity) {
			#ifdef CHILDVERBOSE
			VerboseEnd ("TRANSFORM");
			#endif
			return;
		}



#ifdef BOUNDINGBOX
	if (this_->PIV > 0) {
#endif
	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(this_->children);

	/* do we have a DirectionalLight for a child? */
	if(this_->has_light) dirlightChildren(this_->children);

	/* now, just render the non-directionalLight children */

	/* printf ("Transform %d, flags %d, render_sensitive %d\n",
			this_,this_->_renderFlags,render_sensitive); */

	normalChildren(this_->children);
#ifdef BOUNDINGBOX
	}
#endif

	if (render_geom && (!render_blend)) {
		#ifdef CHILDVERBOSE
			printf ("TransformChild, this is %d, extent %f %f %f\n",
			this_, this_->_extent[0], this_->_extent[1],
			this_->_extent[2]);
		#endif
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
				(struct X3D_Box*)this_);
		BoundingBox(this_->bboxCenter,this_->bboxSize,this_->PIV);
	}


	/* did we have that directionalLight? */
	if((this_->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	VerboseEnd ("TRANSFORM");
	#endif

	curlight = savedlight;
}


void changed_StaticGroup (struct X3D_StaticGroup *this_) {
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
void changed_Transform (struct X3D_Transform *this_) {
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


void changed_Group (struct X3D_Group *this_) {
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


