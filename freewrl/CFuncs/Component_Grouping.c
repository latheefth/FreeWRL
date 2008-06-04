/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Grouping Component

*********************************************************************/

#include <math.h>
#include "headers.h"
#include "installdir.h"
#include "OpenGL_Utils.h"

#ifdef CHILDVERBOSE
static int VerboseIndent = 0;

void VerboseStart (char *whoami, struct X3D_Node *me, int nc) {
	int c;

	for (c=0; c<VerboseIndent; c++) printf ("  ");
	printf ("RENDER %s START %d nc %d \n",
			whoami,me,nc);
	VerboseIndent++;
}

void VerboseEnd (char *whoami) {
	int c;

	VerboseIndent--;
	for (c=0; c<VerboseIndent; c++) printf ("  ");
	printf ("RENDER %s END\n",whoami);
}
#endif


/* prep_Group - we need this so that distance (and, thus, distance sorting) works for Groups */
void prep_Group (struct X3D_Group *node) {
        GLdouble modelMatrix[16];

	 /* we recalculate distance on last pass, or close to it, and only
	 once per event-loop tick. we can do it on the last pass - the
	 render_sensitive pass, but when mouse is clicked (eg, moving in
	 examine mode, sensitive node code is not rendered. So, we choose
	 the second-last pass. ;-) */


        if (render_light) {
		fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
		node->_dist = modelMatrix[14];
	}
}

/* do transforms, calculate the distance */
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

	/* printf ("prep_Transform, render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	 render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

	/* do we have any geometry visible, and are we doing anything with geometry? */
	OCCLUSIONTEST

	if(!render_vp) {
                /* glPushMatrix();*/
		fwXformPush(node);

		/* might we have had a change to a previously ignored value? */
		if (node->_change != node->__verify_transforms) {
			/* printf ("re-rendering for %d\n",node);*/
			node->__do_center = verify_translate ((GLfloat *)node->center.c);
			node->__do_trans = verify_translate ((GLfloat *)node->translation.c);
			node->__do_scale = verify_scale ((GLfloat *)node->scale.c);
			node->__do_rotation = verify_rotate ((GLfloat *)node->rotation.r);
			node->__do_scaleO = verify_rotate ((GLfloat *)node->scaleOrientation.r);
			node->__verify_transforms = node->_change;
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
			/* printf ("calling recordDistance for %d\n",node);*/
			recordDistance(node);
			/* printf ("ppv %d\n"g);*/

	       }
        }
}


void fin_Transform (struct X3D_Transform *node) {
	OCCLUSIONTEST

        if(!render_vp) {
            fwXformPop(node);
        } else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if((node->_renderFlags & VF_Viewpoint) == VF_Viewpoint) {
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
	VerboseStart ("STATICGROUP", X3D_NODE(node), nc);
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
	DIRLIGHTCHILDREN(node->children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX	

		/* pass the bounding box calculations on up the chain */
		propagateExtent(X3D_NODE(node));
		BOUNDINGBOX
	}


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
	VerboseStart ("GROUP", X3D_NODE(node), nc);
	#endif

	/* {
		int x;
		struct X3D_Node *xx;

		printf ("child_Group, this %d isProto %p\n",node,node->FreeWRL__protoDef);
		for (x=0; x<nc; x++) {
			xx = X3D_NODE(node->children.p[x]);
			printf ("	ch %d type %s dist %f\n",node->children.p[x],stringNodeType(xx->_nodeType),xx->_dist);
		}
	} */


		

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
	if ((!node->FreeWRL__protoDef) && (nc > 1)  && !render_blend) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	DIRLIGHTCHILDREN(node->children);

	/* now, just render the non-directionalLight children */
	if (node->FreeWRL__protoDef && render_geom) {
		(node->children).n = 1;
		normalChildren(node->children);
		(node->children).n = nc;
	} else {
		normalChildren(node->children);
	}


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX

		/* pass the bounding box calculations on up the chain */
		propagateExtent(X3D_NODE(node));
		BOUNDINGBOX
	}

	#ifdef CHILDVERBOSE
	VerboseEnd ("GROUP");
	#endif

	DIRECTIONAL_LIGHT_OFF
}


void child_Transform (struct X3D_Transform *node) {
	int nc = (node->children).n;
	OCCLUSIONTEST

	DIRECTIONAL_LIGHT_SAVE

	/* any children at all? */
	if (nc==0) return;

	/* {
		int x;
		struct X3D_Node *xx;

		printf ("child_Transform, this %d \n",node);
		for (x=0; x<nc; x++) {
			xx = X3D_NODE(node->children.p[x]);
			printf ("	ch %d type %s dist %f\n",node->children.p[x],stringNodeType(xx->_nodeType),xx->_dist);
		}
	} */

	#ifdef CHILDVERBOSE
	VerboseStart ("TRANSFORM",X3D_NODE(node), nc);
	#endif

	/* Check to see if we have to check for collisions for this transform. */
#ifdef COLLISIONTRANSFORM
	if (render_collision) {
		iv.x = node->EXTENT_MAX_X/2.0;
		jv.y = node->EXTENT_MAX_Y/2.0;
		kv.z = node->EXTENT_MAX_Z/2.0;
		ov.x = -(node->EXTENT_MAX_X); 
		ov.y = -(node->EXTENT_MAX_Y); 
		ov.z = -(node->EXTENT_MAX_Z);

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
			node,node->EXTENT_MAX_X,node->EXTENT_MAX_Y,EXTENT_MAX_Z,
			t_orig.x,t_orig.y,t_orig.z); */
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_box_intersect(abottom,atop,awidth,t_orig,
			scale*node->EXTENT_MAX_X*2,
			scale*node->EXTENT_MAX_Y*2,
			scale*node->EXTENT_MAX_Z*2)) {
			/* printf ("TB this %d returning fast\n",node); */
			return;
		/* } else {
			printf ("TB really look at %d\n",node); */
		}
	}
#endif


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



#ifdef XXBOUNDINGBOX
	if (node->PIV > 0) {
#endif
	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	DIRLIGHTCHILDREN(node->children);

	/* now, just render the non-directionalLight children */

	/* printf ("Transform %d, flags %d, render_sensitive %d\n",
			node,node->_renderFlags,render_sensitive); */

	#ifdef CHILDVERBOSE
		printf ("transform - doing normalChildren\n");
	#endif

	normalChildren(node->children);

	#ifdef CHILDVERBOSE
		printf ("transform - done normalChildren\n");
	#endif
#ifdef XXBOUNDINGBOX
	}
#endif

	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
		node->bboxCenter.c[0] = node->translation.c[0];
		node->bboxCenter.c[1] = node->translation.c[1];
		node->bboxCenter.c[2] = node->translation.c[2];

		/* pass the bounding box calculations on up the chain */
		propagateExtent(X3D_NODE(node));
		BOUNDINGBOX

	}


	#ifdef CHILDVERBOSE
	VerboseEnd ("TRANSFORM");
	#endif

	DIRECTIONAL_LIGHT_OFF
}

void changed_StaticGroup (struct X3D_StaticGroup *node) {
                int i;
                int nc = ((node->children).n);
                struct X3D_Node *p;

		INITIALIZE_EXTENT

}
void changed_Transform (struct X3D_Transform *node) {
                int i;
                int nc = ((node->children).n);
                struct X3D_Node *p;

		INITIALIZE_EXTENT

}


void changed_Group (struct X3D_Group *node) {
                int i;
                int nc = ((node->children).n);
                struct X3D_Node *p;

		INITIALIZE_EXTENT
}


