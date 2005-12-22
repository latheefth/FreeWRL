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


void prep_Transform (struct X3D_Transform *node) {
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
		fwXformPush(node);

		/* might we have had a change to a previously ignored value? */
		if (node->_change != node->_dlchange) {
			/* printf ("re-rendering for %d\n",node);*/
			node->__do_center = verify_translate ((GLfloat *)node->center.c);
			node->__do_trans = verify_translate ((GLfloat *)node->translation.c);
			node->__do_scale = verify_scale ((GLfloat *)node->scale.c);
			node->__do_rotation = verify_rotate ((GLfloat *)node->rotation.r);
			node->__do_scaleO = verify_rotate ((GLfloat *)node->scaleOrientation.r);
			node->_dlchange = node->_change;
		}



		/* TRANSLATION */
		if (node->__do_trans)
			glTranslatef(node->translation.c[0],node->translation.c[1],node->translation.c[2]);

		/* CENTER */
		if (node->__do_center)
			glTranslatef(node->center.c[0],node->center.c[1],node->center.c[2]);

		/* ROTATION */
		if (node->__do_rotation) {
			my_rotation = node->rotation.r[3]/3.1415926536*180;
			glRotatef(my_rotation,
				node->rotation.r[0],node->rotation.r[1],node->rotation.r[2]);
		}

		/* SCALEORIENTATION */
		if (node->__do_scaleO) {
			my_scaleO = node->scaleOrientation.r[3]/3.1415926536*180;
			glRotatef(my_scaleO, node->scaleOrientation.r[0],
				node->scaleOrientation.r[1],node->scaleOrientation.r[2]);
		}


		/* SCALE */
		if (node->__do_scale)
			glScalef(node->scale.c[0],node->scale.c[1],node->scale.c[2]);

		/* REVERSE SCALE ORIENTATION */
		if (node->__do_scaleO)
			glRotatef(-my_scaleO, node->scaleOrientation.r[0],
				node->scaleOrientation.r[1],node->scaleOrientation.r[2]);

		/* REVERSE CENTER */
		if (node->__do_center)
			glTranslatef(-node->center.c[0],-node->center.c[1],-node->center.c[2]);

		/* did either we or the Viewpoint move since last time? */
		if (recalculate_dist) {
			/* printf ("calling PointInView for %d\n",node);*/
			node->PIV = PointInView(node);
			/* printf ("ppv %d\n",node->PIV);*/

	       }
        }
}

void fin_Transform (struct X3D_Transform *node) {
        if(!render_vp) {
            /* glPopMatrix();*/
            fwXformPop(node);
        } else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if(found_vp) {
                glTranslatef(((node->center).c[0]),((node->center).c[1]),((node->center).c[2])
                );
                glRotatef(((node->scaleOrientation).r[3])/3.1415926536*180,((node->scaleOrientation).r[0]),((node->scaleOrientation).r[1]),((node->scaleOrientation).r[2])
                );
                glScalef(1.0/(((node->scale).c[0])),1.0/(((node->scale).c[1])),1.0/(((node->scale).c[2]))
                );
                glRotatef(-(((node->scaleOrientation).r[3])/3.1415926536*180),((node->scaleOrientation).r[0]),((node->scaleOrientation).r[1]),((node->scaleOrientation).r[2])
                );
                glRotatef(-(((node->rotation).r[3]))/3.1415926536*180,((node->rotation).r[0]),((node->rotation).r[1]),((node->rotation).r[2])
                );
                glTranslatef(-(((node->center).c[0])),-(((node->center).c[1])),-(((node->center).c[2]))
                );
                glTranslatef(-(((node->translation).c[0])),-(((node->translation).c[1])),-(((node->translation).c[2]))
                );
            }
        }
} 

void child_Switch (struct X3D_Switch *node) {
                /* exceedingly simple - render only one child */
                int wc = (node->whichChoice) /*cget*/;
                if(wc >= 0 && wc < ((node->choice).n)) {
                        void *p = ((node->choice).p[wc]);
                        render_node(p);
                }
}


void child_StaticGroup (struct X3D_StaticGroup *node) {
	int nc = ((node->children).n);
	DIRECTIONAL_LIGHT_SAVE
	int createlist = FALSE;

	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	VerboseStart ("STATICGROUP", (struct X3D_Box *)node, nc);
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
			if (node->__transparency < 0) {
				/* printf ("creating transparency display list %d\n",node->__transparency); */
				node->__transparency  = glGenLists(1);
				createlist = TRUE;
				glNewList(node->__transparency,GL_COMPILE_AND_EXECUTE);
			} else {
				/* printf ("calling transparency list\n"); */
				glCallList (node->__transparency);
				return;
			}

		} else {
			if  (node->__solid <0 ) {
				/* printf ("creating solid display list\n"); */
				node->__solid  = glGenLists(1);
				createlist = TRUE;
				glNewList(node->__solid,GL_COMPILE_AND_EXECUTE);
			} else {
				/* printf ("calling solid list\n"); */
				glCallList (node->__solid);
				/* do we have to tell the MainLoop that we have transparency down here? */
				/* is the transparency display list created? */
				if (node->__transparency > -1) have_transparency++;
				return;
			}
		}
	}


	if (render_blend == VF_Blend)
		if ((node->_renderFlags & VF_Blend) != VF_Blend) {
			#ifdef CHILDVERBOSE
			VerboseEnd ("STATICGROUP");
			#endif
			if (createlist) glEndList();
			return;
		}



	/* do we have to sort this node? Only if not a proto - only first node has visible children. */
	if ((nc > 1)  && !render_blend) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		#ifdef CHILDVERBOSE
			printf ("staticGroupingChild, this is %d, extent %f %f %f\n",
			node, node->_extent[0], node->_extent[1],
			node->_extent[2]);
		#endif

		node->bboxSize.c[0] = node->_extent[0];
		node->bboxSize.c[1] = node->_extent[1];
		node->bboxSize.c[2] = node->_extent[2];
		

		/* pass the bounding box calculations on up the chain */
		propagateExtent((float)0.0,(float)0.0,(float)0.0,(struct X3D_Box *)node);
		BoundingBox(node->bboxCenter,node->bboxSize,node->PIV);
	}


	/* did we have that directionalLight? */
	if((node->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	VerboseEnd ("STATICGROUP");
	#endif


			if (createlist) glEndList();

	DIRECTIONAL_LIGHT_OFF
}


void child_Group (struct X3D_Group *node) {
	int nc = ((node->children).n);
	DIRECTIONAL_LIGHT_SAVE

	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	VerboseStart ("GROUP", (struct X3D_Box *)node, nc);
	#endif
/*
{
int x;
struct X3D_Box *xx;

printf ("child_Group, this %d\n",node);
for (x=0; x<nc; x++) {
xx = (struct X3D_Box *)node->children.p[x];

printf ("	ch %d type %s\n",node->children.p[x],stringNodeType(xx->_nodeType));
}
}
*/

		

	/* should we go down here? */
	/* printf ("Group, rb %x VF_B %x, rg  %x VF_G %x\n",render_blend, VF_Blend, render_geom, VF_Geom); */
	if (render_blend == VF_Blend)
		if ((node->_renderFlags & VF_Blend) != VF_Blend) {
			#ifdef CHILDVERBOSE
			VerboseEnd ("GROUP");
			#endif
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((node->_renderFlags & VF_Proximity) != VF_Proximity)  {
			#ifdef CHILDVERBOSE
			VerboseEnd ("GROUP");
			#endif
			return;
		}

	/* do we have to sort this node? Only if not a proto - only first node has visible children. */
	if ((node->__isProto == 0) && (nc > 1)  && !render_blend) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->children);

	/* now, just render the non-directionalLight children */
	if ((node->__isProto == 1) && render_geom) {
		(node->children).n = 1;
		normalChildren(node->children);
		(node->children).n = nc;
	} else {
		normalChildren(node->children);
	}


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		#ifdef CHILDVERBOSE
			printf ("GroupingChild, this is %d, extent %f %f %f\n",
			node, node->_extent[0], node->_extent[1],
			node->_extent[2]);
		#endif
		node->bboxSize.c[0] = node->_extent[0];
		node->bboxSize.c[1] = node->_extent[1];
		node->bboxSize.c[2] = node->_extent[2];

		/* pass the bounding box calculations on up the chain */
		propagateExtent((float)0.0,(float)0.0,(float)0.0,(struct X3D_Box *)node);
		BoundingBox(node->bboxCenter,node->bboxSize,node->PIV);
	}

	/* did we have that directionalLight? */
	if((node->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	VerboseEnd ("GROUP");
	#endif


	DIRECTIONAL_LIGHT_OFF
}


void child_Transform (struct X3D_Transform *node) {
	int nc = (node->children).n;
	DIRECTIONAL_LIGHT_SAVE

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
	VerboseStart ("TRANSFORM",(struct X3D_Box *)node, nc);
	#endif

	/* Check to see if we have to check for collisions for this transform. */
	if (render_collision) {
		iv.x = node->_extent[0]/2.0;
		jv.y = node->_extent[1]/2.0;
		kv.z = node->_extent[2]/2.0;
		ov.x = -(node->_extent[0]); 
		ov.y = -(node->_extent[1]); 
		ov.z = -(node->_extent[2]);

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
			node,node->_extent[0],node->_extent[1],node->_extent[2],
			t_orig.x,t_orig.y,t_orig.z); */
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_box_intersect(abottom,atop,awidth,t_orig,
			scale*node->_extent[0]*2,
			scale*node->_extent[1]*2,
			scale*node->_extent[2]*2)) {
			/* printf ("TB this %d returning fast\n",node); */
			return;
		/* } else {
			printf ("TB really look at %d\n",node); */
		}
	}

	/* should we go down here? */
	/* printf("transformChild %d render_blend %x renderFlags %x\n",
			node, render_blend, node->_renderFlags); */
	if (render_blend == VF_Blend)
		if ((node->_renderFlags & VF_Blend) != VF_Blend) {
			#ifdef CHILDVERBOSE
			VerboseEnd ("TRANSFORM");
			#endif
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((node->_renderFlags & VF_Proximity) != VF_Proximity) {
			#ifdef CHILDVERBOSE
			VerboseEnd ("TRANSFORM");
			#endif
			return;
		}



#ifdef BOUNDINGBOX
	if (node->PIV > 0) {
#endif
	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->children);

	/* now, just render the non-directionalLight children */

	/* printf ("Transform %d, flags %d, render_sensitive %d\n",
			node,node->_renderFlags,render_sensitive); */

	normalChildren(node->children);
#ifdef BOUNDINGBOX
	}
#endif

	if (render_geom && (!render_blend)) {
		#ifdef CHILDVERBOSE
			printf ("TransformChild, this is %d, extent %f %f %f\n",
			node, node->_extent[0], node->_extent[1],
			node->_extent[2]);
		#endif
		node->bboxSize.c[0] = node->_extent[0];
		node->bboxSize.c[1] = node->_extent[1];
		node->bboxSize.c[2] = node->_extent[2];
		node->bboxCenter.c[0] = node->translation.c[0];
		node->bboxCenter.c[1] = node->translation.c[1];
		node->bboxCenter.c[2] = node->translation.c[2];

		/* pass the bounding box calculations on up the chain */
		propagateExtent(node->bboxCenter.c[0],
				node->bboxCenter.c[1],
				node->bboxCenter.c[2],
				(struct X3D_Box*)node);
		BoundingBox(node->bboxCenter,node->bboxSize,node->PIV);
	}


	/* did we have that directionalLight? */
	if((node->has_light)) glPopAttrib();

	#ifdef CHILDVERBOSE
	VerboseEnd ("TRANSFORM");
	#endif

	DIRECTIONAL_LIGHT_OFF
}


void changed_StaticGroup (struct X3D_StaticGroup *node) {
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
void changed_Transform (struct X3D_Transform *node) {
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


void changed_Group (struct X3D_Group *node) {
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


