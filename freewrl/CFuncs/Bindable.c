/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*****************************************

Bindable nodes - Background, Fog, NavigationInfo, Viewpoint, GeoViewpoint.

******************************************/

#include "Bindable.h"
#include "Viewer.h"
#include "GeoVRML.h"

/* Viewport data */
GLint viewPort[10];

int background_tos = -1;
int fog_tos = -1;
int navi_tos = -1;
int viewpoint_tos = -1;
unsigned int background_stack[MAX_STACK];
unsigned int fog_stack[MAX_STACK];
unsigned int viewpoint_stack[MAX_STACK];
unsigned int navi_stack[MAX_STACK];

void saveBGVert (struct VRML_Background *node, 
		int *vertexno, float *col, double dist,
		double x, double y, double z) ;

/* this is called after a Viewpoint or GeoViewpoint bind */
void reset_upvector() {
    ViewerUpvector.x = 0;
    ViewerUpvector.y = 0;
    ViewerUpvector.z = 0;
}

/* called when binding NavigationInfo nodes */
void set_naviinfo(struct VRML_NavigationInfo *node) {
	SV **svptr;
	int i;
	char *typeptr;
	unsigned int xx;

	if (node->avatarSize.n<2) {
		printf ("set_naviinfo, avatarSize smaller than expected\n");	
	} else {
        	naviinfo.width = node->avatarSize.p[0];
        	naviinfo.height = node->avatarSize.p[1];
        	naviinfo.step = node->avatarSize.p[2];
	}
        Viewer.headlight = node->headlight;
        Viewer.speed = (double) node->speed;

	/* keep track of valid Navigation types. */
	svptr = node->type.p;

	/* assume "NONE" is set */
	for (i=0; i<6; i++) Viewer.oktypes[i] = FALSE; 


	/* now, find the ones that are ok */
	for (i = 0; i < node->type.n; i++) {
		// get the string pointer
		typeptr = SvPV(svptr[i],xx);

		if (strncmp(typeptr,"WALK",strlen("WALK")) == 0) {
			Viewer.oktypes[WALK] = TRUE;
			if (i==0) set_viewer_type(WALK);
		}
		if (strncmp(typeptr,"FLY",strlen("FLY")) == 0) {
			Viewer.oktypes[FLY] = TRUE;
			if (i==0) set_viewer_type(FLY);
		}
		if (strncmp(typeptr,"EXAMINE",strlen("EXAMINE")) == 0) {
			Viewer.oktypes[EXAMINE] = TRUE;
			if (i==0) set_viewer_type(EXAMINE);
		}
		if (strncmp(typeptr,"NONE",strlen("NONE")) == 0) {
			Viewer.oktypes[NONE] = TRUE;
			if (i==0) set_viewer_type(NONE);
		}
		if (strncmp(typeptr,"EXFLY",strlen("EXFLY")) == 0) {
			Viewer.oktypes[EXFLY] = TRUE;
			if (i==0) set_viewer_type(EXFLY);
		}
		if (strncmp(typeptr,"ANY",strlen("ANY")) == 0) {
			Viewer.oktypes[EXAMINE] = TRUE;
			Viewer.oktypes[WALK] = TRUE;
			Viewer.oktypes[EXFLY] = TRUE;
			Viewer.oktypes[FLY] = TRUE;
			if (i==0) set_viewer_type (WALK); // just choose one
		}
	}
}




/* send a set_bind event from Perl to this Bindable node */
void send_bind_to(int nodetype, void *node, int value) {
	struct VRML_Background *bg;
	struct VRML_Fog *fg;
	struct VRML_NavigationInfo *nv;
	struct VRML_Viewpoint *vp;
	struct VRML_GeoViewpoint *gvp;
	char * nameptr;
	unsigned int len;

	//printf ("\nsend_bind_to, nodetype %d node %d value %d\n",nodetype,node,value); 

	if (nodetype == BACKGROUND) {
		bg = (struct VRML_Background *) node;
		bg->set_bind = value;

		bind_node (node,offsetof (struct VRML_Background,set_bind),
			offsetof (struct VRML_Background,isBound),
			&background_tos,&background_stack[0]);

	} else if (nodetype == VIEWPOINT) {
		vp = (struct VRML_Viewpoint *) node;
		vp->set_bind = value;
		nameptr = SvPV(vp->description,len);
		viewpoint_name_status (nameptr);

		bind_node (node,offsetof (struct VRML_Viewpoint,set_bind),
			offsetof (struct VRML_Viewpoint,isBound),
			&viewpoint_tos,&viewpoint_stack[0]);

		/* up_vector is reset after a bind */
		if (value==1) {
			reset_upvector();
			bind_viewpoint (vp);
		}

	} else if (nodetype == GEOVIEWPOINT) {
		gvp = (struct VRML_GeoViewpoint *) node;
		gvp->set_bind = value;
		nameptr = SvPV(gvp->description,len);
		viewpoint_name_status (nameptr);

		bind_node (node,offsetof (struct VRML_GeoViewpoint,set_bind),
			offsetof (struct VRML_GeoViewpoint,isBound),
			&viewpoint_tos,&viewpoint_stack[0]);

		/* up_vector is reset after a bind */
		if (value==1) {
			reset_upvector();
			bind_geoviewpoint (gvp);
		}

	} else if (nodetype == FOG) {
		fg = (struct VRML_Fog *) node;
		fg->set_bind = value;
		bind_node (node,offsetof (struct VRML_Fog,set_bind),
			offsetof (struct VRML_Fog,isBound),
			&fog_tos,&fog_stack[0]);

	} else if (nodetype == NAVIGATIONINFO) {
		nv = (struct VRML_NavigationInfo *) node;
		nv->set_bind = value;
		bind_node (node,offsetof (struct VRML_NavigationInfo,set_bind),
			offsetof (struct VRML_NavigationInfo,isBound),
			&navi_tos,&navi_stack[0]);

		if (value==1) set_naviinfo(nv);

	} else {
		printf ("send_bind_to, cant send a set_bind to %d !!\n",nodetype);
	}
}



/* Do binding for node and stack - works for all bindable nodes */

void bind_node (void *node, unsigned int setBindofst,
			int isboundofst, int *tos, unsigned int *stack) {

	unsigned int *oldstacktop;
	unsigned int *newstacktop;
	unsigned int *setBindptr;	/* this nodes setBind */
	unsigned int *isBoundptr;	/* this nodes isBound */
	unsigned int *oldboundptr;	/* previous nodes isBound */

	/* setup some variables */
	setBindptr = (unsigned int *) ((unsigned int) node + setBindofst);
	isBoundptr = (unsigned int *) ((unsigned int) node + isboundofst);
	oldstacktop = stack + *tos;  

	//printf ("bind_node, node %d, set_bind %d\n",node,*setBindptr);
	/* we either have a setBind of 1, which is a push, or 0, which
	   is a pop. the value of 100 (arbitrary) indicates that this
	   is not a new push or pop */

	if (*setBindptr == 1) {
		/* PUSH THIS TO THE TOP OF THE STACK */

		/* are we off the top of the stack? */
		if (*tos >= (MAX_STACK-2)) return;

		/* isBound mimics setBind */
		*isBoundptr = 1;

		/* unset the set_bind flag  - setBind can be 0 or 1; lets make it garbage */
		*setBindptr = 100;

		mark_event ((unsigned int) node, (unsigned int) isboundofst);

		/* set up pointers, increment stack */
		*tos = *tos+1;
		newstacktop = stack + *tos;


		/* save pointer to new top of stack */
		*newstacktop = (unsigned int) node;
		update_node((void *) *newstacktop);

		/* was there another DIFFERENT node at the top of the stack? 
		   have to check for a different one, as if we are binding to the current 
		   Viewpoint, then we do NOT want to unbind it, if we do then the current
		   top of stack Viewpoint is unbound! */

		if ((*tos >= 1) && (*oldstacktop!=*newstacktop)) {
			/* yep... unbind it, and send an event in case anyone cares */
			oldboundptr = (unsigned int *) (*oldstacktop + isboundofst);
			*oldboundptr = 0;
			 //printf ("....bind_node, in set_bind true, unbinding node %d\n",*oldstacktop);
	
			/* tell the possible parents of this change */
			update_node((void *) *oldstacktop);
		}
	} else {
		/* POP FROM TOP OF STACK  - if we ARE the top of stack */

		/* anything on stack? */
		if (*tos <= -1) return;   /* too many pops */

		/* isBound mimics setBind */
		*isBoundptr = 0;

		/* unset the set_bind flag  - setBind can be 0 or 1; lets make it garbage */
		*setBindptr = 100;

		mark_event ((unsigned int) node, (unsigned int) isboundofst);

		//printf ("old TOS is %d, we are %d\n",*oldstacktop, node);
		if ((unsigned int) node != *oldstacktop) return;

		//printf ("ok, we were TOS, setting %d to 0\n",node);


		*tos = *tos - 1;

		if (*tos >= 0) {
			/* stack is not empty */
			newstacktop = stack + *tos;
			//printf ("   .... and we had a stack value; binding node %d\n",*newstacktop);
		
			/* set the popped value of isBound to true */
			isBoundptr = (unsigned int *) (*newstacktop + isboundofst);
			*isBoundptr = 1;

			/* tell the possible parents of this change */
			update_node((void *) *newstacktop);
			mark_event ((unsigned int) *newstacktop, (unsigned int) isboundofst);
		}
	}
}

void render_Fog (struct VRML_Fog *node) {
	GLdouble mod[16];
	GLdouble proj[16];
	GLdouble unit[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	GLdouble x,y,z;
	GLdouble x1,y1,z1;
	GLdouble sx, sy, sz;
	/* int frtlen; */
	GLfloat fog_colour [4];
	unsigned int foglen;
	char *fogptr;


	//printf ("render_Fog, node %d isBound %d color %f %f %f set_bind %d\n",
		//node, node->isBound, node->color.c[0],node->color.c[1],node->color.c[2],node->set_bind);

	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {

		bind_node (node,offsetof (struct VRML_Fog,set_bind),
			offsetof (struct VRML_Fog,isBound),
			&fog_tos,&fog_stack[0]);

		/* if we do not have any more nodes on top of stack, disable fog */
		glDisable (GL_FOG);
	}

	if(!node->isBound) return;
	if (node->visibilityRange <= 0.00001) return;

	fog_colour[0] = node->color.c[0];
	fog_colour[1] = node->color.c[1];
	fog_colour[2] = node->color.c[2];
	fog_colour[3] = 1.0;

	fogptr = SvPV((node->fogType),foglen); 
	glPushMatrix();
	glGetDoublev(GL_MODELVIEW_MATRIX, mod);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	/* Get origin */
	gluUnProject(0.0f,0.0f,0.0f,mod,proj,viewport,&x,&y,&z);
	glTranslated(x,y,z);

	gluUnProject(0.0f,0.0f,0.0f,mod,unit,viewport,&x,&y,&z);
	/* Get scale */
	gluProject(x+1,y,z,mod,unit,viewport,&x1,&y1,&z1);
	sx = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	gluProject(x,y+1,z,mod,unit,viewport,&x1,&y1,&z1);
	sy = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	gluProject(x,y,z+1,mod,unit,viewport,&x1,&y1,&z1);
	sz = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	/* Undo the translation and scale effects */
	glScaled(sx,sy,sz);


	/* now do the foggy stuff */
	glFogfv(GL_FOG_COLOR,fog_colour);
	glFogf(GL_FOG_END, (float) (node->visibilityRange));
	if (strncmp("LINEAR",fogptr,(unsigned) foglen)) {
		glFogi(GL_FOG_MODE, GL_EXP);
	} else {
		glFogi(GL_FOG_MODE, GL_LINEAR);
	}
	glEnable (GL_FOG);

	glPopMatrix();
}

void render_NavigationInfo (struct VRML_NavigationInfo *node) {
	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		if (node->set_bind == 1) set_naviinfo(node);

		bind_node (node,offsetof (struct VRML_NavigationInfo,set_bind),
			offsetof (struct VRML_NavigationInfo,isBound),
			&navi_tos,&navi_stack[0]);
	}
	if(!node->isBound) return;
}

void render_GeoViewpoint (struct VRML_GeoViewpoint *node) {
	double a1;
	char *posnstring;
	unsigned int xx, yy;


	//printf ("rgvp, node %d ib %d sb %d gepvp\n",node,node->isBound,node->set_bind);
	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		/* up_vector is reset after a bind */
		if (node->set_bind==1) reset_upvector();

		bind_node (node,offsetof (struct VRML_GeoViewpoint,set_bind),
			offsetof (struct VRML_GeoViewpoint,isBound),
			&viewpoint_tos,&viewpoint_stack[0]);
	}

	if(!node->isBound) return;

	/* stop rendering when we hit A viewpoint or THE viewpoint???
	   shouldnt we check for what_vp???? 
           maybe only one viewpoint is in the tree at a time? -  ncoder*/

	found_vp = 1; /* We found the viewpoint */

	/* is the position "compiled" yet? */
	if (node->_change != node->_dlchange) {
		//printf ("have to recompile position...\n");
		posnstring = SvPV(node->position,xx);
		if (sscanf (SvPV(node->position,xx),"%f %f %f",&node->__position.c[0],
			&node->__position.c[1], &node->__position.c[2]) != 3) {
			printf ("GeoViewpoint - vp:%s: invalid position string: :%s:\n",
					SvPV(node->description,xx),
					SvPV(node->position,yy));
		}

		geoSystemCompile (&node->geoSystem, &node->__geoSystem, 
			SvPV(node->description,xx));

		node->_dlchange = node->_change;
	}

	if (node->geoOrigin) {
		render_node (node->geoOrigin);
	}

	/* perform GeoViewpoint translations */
	glRotated(-node->orientation.r[3]/PI*180.0,node->orientation.r[0],node->orientation.r[1],
		node->orientation.r[2]);
        glTranslated (GeoOrig[0] - node->__position.c[0],
                        GeoOrig[1] - node->__position.c[1],
                        GeoOrig[2] - node->__position.c[2]);

        /* printf ("GeoViewing at %f %f %f\n", 
        		GeoOrig[0] - node->__position.c[0],
                        GeoOrig[1] - node->__position.c[1],
                        GeoOrig[2] - node->__position.c[2]); */

	/* now, lets work on the GeoViewpoint fieldOfView */
	glGetIntegerv(GL_VIEWPORT, viewPort);
	if(viewPort[2] > viewPort[3]) {
		a1=0;
		fieldofview = node->fieldOfView/3.1415926536*180;
	} else {
		a1 = node->fieldOfView;
		a1 = atan2(sin(a1),viewPort[2]/((float)viewPort[3]) * cos(a1));
		fieldofview = a1/3.1415926536*180;
	}
	calculateFrustum();
	//printf ("render_GeoViewpoint, bound to %d, fieldOfView %f \n",node,node->fieldOfView);
}

void render_Viewpoint (struct VRML_Viewpoint *node) {
	double a1;

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
	calculateFrustum();
	//printf ("render_Viewpoint, bound to %d, fieldOfView %f \n",node,node->fieldOfView);
}

void render_Background (struct VRML_Background *node) {
	GLdouble mod[16];
	GLdouble proj[16];
	GLdouble unit[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	/* struct pt vec[4]; struct pt vec2[4]; struct pt vec3[4]; */
	/* int i,j; int ind=0; */
	GLdouble x,y,z;
	GLdouble x1,y1,z1;
	GLdouble sx, sy, sz;
	struct SFColor *c1,*c2;
	int hdiv = 20;			/* number of horizontal strips allowed */
	int h,v;
	double va1, va2, ha1, ha2;	/* JS - vert and horiz angles 	*/
	int estq;
	int actq;

	 //printf ("RBG, node %d ib %d sb %d gepvp\n",node,node->isBound,node->set_bind);
	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		bind_node (node,offsetof (struct VRML_Background,set_bind),
			offsetof (struct VRML_Background,isBound),
			&background_tos,&background_stack[0]);
	}

	/* don't even bother going further if this node is not bound on the top */
	if(!node->isBound) return;

	/* Cannot start_list() because of moving center, so we do our own list later */

	glPushAttrib(GL_LIGHTING_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT);
	glShadeModel(GL_SMOOTH);
	glPushMatrix();
	glGetDoublev(GL_MODELVIEW_MATRIX, mod);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	/* Get origin */
	gluUnProject(0.0f,0.0f,0.0f,mod,proj,viewport,&x,&y,&z);
	glTranslated(x,y,z);

	glDisable (GL_LIGHTING);

	gluUnProject(0.0f,0.0f,0.0f,mod,unit,viewport,&x,&y,&z);
	/* Get scale */
	gluProject(x+1,y,z,mod,unit,viewport,&x1,&y1,&z1);
	sx = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	gluProject(x,y+1,z,mod,unit,viewport,&x1,&y1,&z1);
	sy = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	gluProject(x,y,z+1,mod,unit,viewport,&x1,&y1,&z1);
	sz = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );

	/* Undo the translation and scale effects */
	glScaled(sx,sy,sz);

	if (node->_change != node->_ichange) {
		/* we have to re-calculate display arrays */
		node->_ichange = node->_change;
		
		/* do we have an old background to destroy? */
		if (node->__points != 0) free ((void *)node->__points);
		if (node->__colours != 0) free ((void *)node->__colours);
	
		/* calculate how many quads are required */
		estq=0; actq=0;
		if(node->skyColor.n == 1) {
			estq += 40;
		} else {
			estq += (node->skyColor.n-1) * 20 + 20;
			// attempt to find exact estimate, fails if no skyAngle, so
			// simply changed above line to add 20 automatically.
			//if ((node->skyColor.n >2) &&
			//	(node->skyAngle.n > node->skyColor.n-2)) {
			//	if (node->skyAngle.p[node->skyColor.n-2] < (PI-0.01)) 
			//		estq += 20;
			//}
		}
	
		if(node->groundColor.n == 1) estq += 40;
		else if (node->groundColor.n>0) estq += (node->groundColor.n-1) * 20;

		/* remember how many quads are in this background */
		node->__quadcount = estq * 4;

		/* now, malloc space for new arrays  - 3 points per vertex, 4 per quad. */
		node->__points = (int)malloc (sizeof (GLfloat) * estq * 3 * 4);
		node->__colours = (int)malloc (sizeof (GLfloat) * estq * 3 * 4);
		if ((node->__points == 0) || (node->__colours == 0)) {
			printf ("malloc failure in background\n");
			exit(1);
		}
	
		if(node->skyColor.n == 1) {
			c1 = &node->skyColor.p[0];
			va1 = 0;
			va2 = PI/2; 
	
			for(v=0; v<2; v++) {
				for(h=0; h<hdiv; h++) {
					ha1 = h * PI*2 / hdiv;
					ha2 = (h+1) * PI*2 / hdiv;
					saveBGVert (node,&actq,&c1->c[0],20000.0,
					  sin(va2)*cos(ha1), cos(va2), sin(va2)*sin(ha1));
					saveBGVert (node,&actq,&c1->c[0],20000.0,
					  sin(va2)*cos(ha2), cos(va2), sin(va2)*sin(ha2));
					saveBGVert (node,&actq,&c1->c[0],20000.0,
					  sin(va1)*cos(ha2), cos(va1), sin(va1)*sin(ha2));
					saveBGVert (node,&actq,&c1->c[0],20000.0,
					  sin(va1)*cos(ha1), cos(va1), sin(va1) * sin(ha1));
				}
				va1 = va2;
				va2 = PI;
			}
		} else {
			va1 = 0;
			/* this gets around a compiler warning - we really DO want last values of this from following
			   for loop */
			c1 = &node->skyColor.p[0]; 
			if (node->skyAngle.n>0) {
				va2= node->skyAngle.p[0]; 
			} else {
				va2 = PI/2;
			}
			c2=c1;


			for(v=0; v<(node->skyColor.n-1); v++) {
				c1 = &node->skyColor.p[v];
				c2 = &node->skyColor.p[v+1];
				if (node->skyAngle.n>0) { va2 = node->skyAngle.p[v];}
				else { va2 = PI/2; }
				
				for(h=0; h<hdiv; h++) {
					ha1 = h * PI*2 / hdiv;
					ha2 = (h+1) * PI*2 / hdiv;
					saveBGVert(node,&actq,&c2->c[0],20000.0,	
					  sin(va2)*cos(ha1), cos(va2), sin(va2) * sin(ha1));
					saveBGVert(node,&actq,&c2->c[0],20000.0,	
					  sin(va2)*cos(ha2), cos(va2), sin(va2) * sin(ha2));
					saveBGVert(node,&actq,&c1->c[0],20000.0,	
					  sin(va1)*cos(ha2), cos(va1), sin(va1) * sin(ha2));
					saveBGVert(node,&actq,&c1->c[0],20000.0,	
					  sin(va1) * cos(ha1), cos(va1), sin(va1) * sin(ha1));
				}
				va1 = va2;
			}
	
			/* now, the spec states: "If the last skyAngle is less than pi, then the
			  colour band between the last skyAngle and the nadir is clamped to the last skyColor." */
			if (va2 < (PI-0.01)) {
				for(h=0; h<hdiv; h++) {
					ha1 = h * PI*2 / hdiv;
					ha2 = (h+1) * PI*2 / hdiv;
					saveBGVert(node,&actq,&c2->c[0],20000.0,	
					  sin(PI) * cos(ha1), cos(PI), sin(PI) * sin(ha1));
					saveBGVert(node,&actq,&c2->c[0],20000.0,	
					  sin(PI) * cos(ha2), cos(PI), sin(PI) * sin(ha2));
					saveBGVert(node,&actq,&c2->c[0],20000.0,	
					  sin(va2) * cos(ha2), cos(va2), sin(va2) * sin(ha2));
					saveBGVert(node,&actq,&c2->c[0],20000.0,	
					  sin(va2) * cos(ha1), cos(va2), sin(va2) * sin(ha1));
				}
			}
		}
	
		/* Do the ground, if there is anything  to do. */
		if (node->groundColor.n>0) {
			if(node->groundColor.n == 1) {
				c1 = &node->groundColor.p[0];
				for(h=0; h<hdiv; h++) {
					ha1 = h * PI*2 / hdiv;
					ha2 = (h+1) * PI*2 / hdiv;
	
					saveBGVert(node,&actq,&c1->c[0],12500.0,	
					  sin(PI) * cos(ha1), cos(PI), sin(PI) * sin(ha1));
					saveBGVert(node,&actq,&c1->c[0],12500.0,	
					  sin(PI) * cos(ha2), cos(PI), sin(PI) * sin(ha2));
					saveBGVert(node,&actq,&c1->c[0],12500.0,	
					  sin(PI/2) * cos(ha2), cos(PI/2), sin(PI/2) * sin(ha2));
					saveBGVert(node,&actq,&c1->c[0],12500.0,	
					  sin(PI/2) * cos(ha1), cos(PI/2), sin(PI/2) * sin(ha1));
				}
			} else {
				va1 = PI;
				for(v=0; v<node->groundColor.n-1; v++) {
					c1 = &node->groundColor.p[v];
					c2 = &node->groundColor.p[v+1];
					va2 = PI - node->groundAngle.p[v];
			
					for(h=0; h<hdiv; h++) {
						ha1 = h * PI*2 / hdiv;
						ha2 = (h+1) * PI*2 / hdiv;
	
						saveBGVert(node,&actq,&c1->c[0],12500.0,	
						  sin(va1)*cos(ha1), cos(va1), sin(va1)*sin(ha1));
						saveBGVert(node,&actq,&c1->c[0],12500.0,	
						  sin(va1)*cos(ha2), cos(va1), sin(va1)*sin(ha2));
						saveBGVert(node,&actq,&c2->c[0],12500.0,	
						  sin(va2)*cos(ha2), cos(va2), sin(va2)*sin(ha2));
						saveBGVert(node,&actq,&c2->c[0],12500.0,	
						  sin(va2) * cos(ha1), cos(va2), sin(va2)*sin(ha1));
					}
					va1 = va2;
				}
			}
		}
	}

	/* now, display the lists */
	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)node->__points);
	glColorPointer(3, GL_FLOAT, 0, (GLfloat *)node->__colours);
	glEnableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glDrawArrays (GL_QUADS, 0, node->__quadcount);

	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);





	/* now, for the textures, if they exist */
	if (((node->backUrl).n>0) || 
			((node->frontUrl).n>0) || 
			((node->leftUrl).n>0) || 
			((node->rightUrl).n>0) || 
			((node->topUrl).n>0) || 
			((node->bottomUrl).n>0)) {

        	glEnable(GL_TEXTURE_2D);
        	glColor3d(1.0,1.0,1.0);
        	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
        	glVertexPointer (3,GL_FLOAT,0,BackgroundVert);
        	glNormalPointer (GL_FLOAT,0,Backnorms);
        	glTexCoordPointer (2,GL_FLOAT,0,Backtex);

		loadBackgroundTextures(node);

        	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}
	glPopMatrix();
	glPopAttrib();
}

/* save a Background vertex into the __points and __colours arrays */
void saveBGVert (struct VRML_Background *node, 
		int *vertexno, float *col, double dist,
		double x, double y, double z) {
		
		float *pt;
		
		/* first, save the colour */
		pt = (float *) node->__colours;

		memcpy (&pt[*vertexno*3], col, sizeof(float)*3);

		/* and, save the vertex info */
		pt = (float *) node->__points;
		pt[*vertexno*3+0] = (float)x*dist;
		pt[*vertexno*3+1] = (float)y*dist;
		pt[*vertexno*3+2] = (float)z*dist;
		
		(*vertexno)++;
}
