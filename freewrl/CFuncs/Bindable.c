/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*****************************************

Bindable nodes - Background, Fog, NavigationInfo, Viewpoint.

******************************************/

#include "Bindable.h"
#include "Viewer.h"

extern VRML_Viewer Viewer; //in VRMLC.pm


int background_tos = -1;
int fog_tos = -1;
int navi_tos = -1;
int viewpoint_tos = -1;
unsigned int background_stack[MAX_STACK];
unsigned int fog_stack[MAX_STACK];
unsigned int viewpoint_stack[MAX_STACK];
unsigned int navi_stack[MAX_STACK];

/* this is called after a Viewpoint bind */
void reset_upvector() {
    ViewerUpvector.x = 0;
    ViewerUpvector.y = 0;
    ViewerUpvector.z = 0;
}

/* called when binding NavigationInfo nodes */
void set_naviinfo(struct VRML_NavigationInfo *node) {
	struct Multi_String *to;
	SV **svptr;
	int i;
	char *typeptr;

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
		typeptr = SvPV(svptr[i],PL_na);

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
void send_bind_to(char *nodetype, void *node, int value) {
	struct VRML_Background *bg;
	struct VRML_Fog *fg;
	struct VRML_NavigationInfo *nv;
	struct VRML_Viewpoint *vp;

	// printf ("\nsend_bind_to, nodetype %s node %d value %d\n",nodetype,node,value);

	if (strncmp("Background",nodetype,strlen("Background"))==0) {
		bg = (struct VRML_Background *) node;
		bg->set_bind = value;

		bind_node (node,offsetof (struct VRML_Background,set_bind),
			offsetof (struct VRML_Background,isBound),
			&background_tos,&background_stack[0]);

	} else if (strncmp("Viewpoint",nodetype,strlen("Viewpoint"))==0) {
		vp = (struct VRML_Viewpoint *) node;
		vp->set_bind = value;

		bind_node (node,offsetof (struct VRML_Viewpoint,set_bind),
			offsetof (struct VRML_Viewpoint,isBound),
			&viewpoint_tos,&viewpoint_stack[0]);

		/* up_vector is reset after a bind */
		if (value==1) {
			reset_upvector();
			bind_viewpoint (vp);
		}

	} else if (strncmp("Fog",nodetype,strlen("Fog"))==0) {
		fg = (struct VRML_Fog *) node;
		fg->set_bind = value;
		bind_node (node,offsetof (struct VRML_Fog,set_bind),
			offsetof (struct VRML_Fog,isBound),
			&fog_tos,&fog_stack[0]);

	} else if (strncmp("NavigationInfo",nodetype,strlen("NavigationInfo"))==0) {
		nv = (struct VRML_NavigationInfo *) node;
		nv->set_bind = value;
		bind_node (node,offsetof (struct VRML_NavigationInfo,set_bind),
			offsetof (struct VRML_NavigationInfo,isBound),
			&navi_tos,&navi_stack[0]);

		if (value==1) set_naviinfo(nv);

	} else {
		printf ("send_bind_to, cant send a set_bind to %s !!\n",nodetype);
	}
}



/* Do binding for node and stack - works for all bindable nodes */

void bind_node (void *node, unsigned int setBindofst,
			int isboundofst, int *tos, int *stack) {

	unsigned int *oldstacktop;
	unsigned int *newstacktop;
	unsigned int *setBindptr;	/* this nodes setBind */
	unsigned int *isBoundptr;	/* this nodes isBound */
	unsigned int *oldboundptr;	/* previous nodes isBound */

	/* setup some variables */
	setBindptr = (unsigned int) node + setBindofst;
	isBoundptr = (unsigned int) node + isboundofst;
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
		update_node(*newstacktop);

		/* was there another node at the top of the stack? */
		if (*tos >= 1) {
			/* yep... unbind it, and send an event in case anyone cares */
			oldboundptr = *oldstacktop + isboundofst;
			*oldboundptr = 0;
			 //printf ("....bind_node, in set_bind true, unbinding node %d\n",*oldstacktop);
	
			/* tell the possible parents of this change */
			update_node(*oldstacktop);
		}
	} else {
		/* POP FROM TOP OF STACK  - if we ARE the top of stack */

		/* isBound mimics setBind */
		*isBoundptr = 0;

		/* unset the set_bind flag  - setBind can be 0 or 1; lets make it garbage */
		*setBindptr = 100;

		mark_event ((unsigned int) node, (unsigned int) isboundofst);

		//printf ("old TOS is %d, we are %d\n",*oldstacktop, node);
		if (node != *oldstacktop) return;

		//printf ("ok, we were TOS, setting %d to 0\n",node);


		*tos = *tos - 1;

		if (*tos >= 0) {
			/* stack is not empty */
			newstacktop = stack + *tos;
			//printf ("   .... and we had a stack value; binding node %d\n",*newstacktop);
		
			/* set the popped value of isBound to true */
			isBoundptr = *newstacktop + isboundofst;
			*isBoundptr = 1;

			/* tell the possible parents of this change */
			update_node(*newstacktop);
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
	int foglen;
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
	glFogf(GL_FOG_END,node->visibilityRange);
	if (strncmp("LINEAR",fogptr,foglen)) {
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

void render_Viewpoint (struct VRML_Viewpoint *node) {
	GLint vp[10];
	double a1;
       /* GLdouble modelMatrix[16]; */

	//printf ("rvp, node %d ib %d sb %d\n",node,node->isBound,node->set_bind);
	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		/* up_vector is reset after a bind */
		if (node->set_bind==1) reset_upvector();

		bind_node (node,offsetof (struct VRML_Viewpoint,set_bind),
			offsetof (struct VRML_Viewpoint,isBound),
			&viewpoint_tos,&viewpoint_stack[0]);
	}

	if(!node->isBound) return;

	/*
        glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
       printf ("Cur Matrix: \n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n",
                modelMatrix[0],  modelMatrix[4],  modelMatrix[ 8],  modelMatrix[12],
                modelMatrix[1],  modelMatrix[5],  modelMatrix[ 9],  modelMatrix[13],
                modelMatrix[2],  modelMatrix[6],  modelMatrix[10],  modelMatrix[14],
                modelMatrix[3],  modelMatrix[7],  modelMatrix[11],  modelMatrix[15]);
	*/


	/* stop rendering when we hit A viewpoint or THE viewpoint???
	   shouldnt we check for what_vp???? 
           maybe only one viewpoint is in the tree at a time? -  ncoder*/

	found_vp = 1; /* We found the viewpoint */

	/* perform Viewpoint translations */
	glRotatef(-node->orientation.r[3]/PI*180.0,node->orientation.r[0],node->orientation.r[1],
		node->orientation.r[2]);
	glTranslatef(-node->position.c[0],-node->position.c[1],-node->position.c[2]);

	/*
        glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
       printf ("Cur Matrix: \n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n",
                modelMatrix[0],  modelMatrix[4],  modelMatrix[ 8],  modelMatrix[12],
                modelMatrix[1],  modelMatrix[5],  modelMatrix[ 9],  modelMatrix[13],
                modelMatrix[2],  modelMatrix[6],  modelMatrix[10],  modelMatrix[14],
                modelMatrix[3],  modelMatrix[7],  modelMatrix[11],  modelMatrix[15]);
	*/




	/* now, lets work on the Viewpoint fieldOfView */
	glGetIntegerv(GL_VIEWPORT, vp);
	if(vp[2] > vp[3]) {
		a1=0;
		fieldofview = node->fieldOfView/3.1415926536*180;
	} else {
		a1 = node->fieldOfView;
		a1 = atan2(sin(a1),vp[2]/((float)vp[3]) * cos(a1));
		fieldofview = a1/3.1415926536*180;
	}
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
	int hdiv = horiz_div;
	int h,v;
	double va1, va2, ha1, ha2;	/* JS - vert and horiz angles 	*/
	/* double vatemp;	 */	
	/* GLuint mask; */
	GLfloat bk_emis[4];		/* background emissive colour	*/
	float	sc;


	/* Background Texture Objects.... */
	static int bcklen,frtlen,rtlen,lftlen,toplen,botlen;
	unsigned char *bckptr,*frtptr,*rtptr,*lftptr,*topptr,*botptr;

	static GLfloat light_ambient[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_diffuse[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_specular[] = {0.0, 0.0, 0.0, 1.0};
	static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};

	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		bind_node (node,offsetof (struct VRML_Background,set_bind),
			offsetof (struct VRML_Background,isBound),
			&background_tos,&background_stack[0]);
	}

	/* don't even bother going further if this node is not bound on the top */
	if(!node->isBound) return;

	bk_emis[3]=0.0; /* always zero for backgrounds */

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


	/* now, is this the same background as before??? */
	if (node->_dlist) {
		if (node->_dlchange == node->_change) {
			glCallList(node->_dlist);
			glPopMatrix();
			glPopAttrib();
			return;
		} else {
			glDeleteLists(node->_dlist,1);
		}
	}

	/* we are here; compile and display a new background! */
	node->_dlist = glGenLists(1);
	node->_dlchange = node->_change;
	glNewList(node->_dlist,GL_COMPILE_AND_EXECUTE);

	/* do we have any background textures?  */
	frtptr = SvPV((node->__locfilefront),frtlen); 
	bckptr = SvPV((node->__locfileback),bcklen);
	topptr = SvPV((node->__locfiletop),toplen);
	botptr = SvPV((node->__locfilebottom),botlen);
	lftptr = SvPV((node->__locfileleft),lftlen);
	rtptr = SvPV((node->__locfileright),rtlen);

	/*printf ("background textures; lengths %d %d %d %d %d %d\n",
		frtlen,bcklen,toplen,botlen,lftlen,rtlen);
	printf ("backgrouns textures; names %s %s %s %s %s %s\n",
		frtptr,bckptr,topptr,botptr,lftptr,rtptr); */

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);


	sc = 20000.0; /* where to put the sky quads */
	glBegin(GL_QUADS);
	if(node->skyColor.n == 1) {
		c1 = &node->skyColor.p[0];
		va1 = 0;
		va2 = PI/2; 
		bk_emis[0]=c1->c[0]; bk_emis[1]=c1->c[1]; bk_emis[2]=c1->c[2];
		glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
		glColor3d(c1->c[0], c1->c[1], c1->c[2]);

		for(v=0; v<2; v++) {
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;

				glVertex3d(sin(va2)*sc * cos(ha1), cos(va2)*sc, sin(va2) * sin(ha1)*sc);
				glVertex3d(sin(va2)*sc * cos(ha2), cos(va2)*sc, sin(va2) * sin(ha2)*sc);
				glVertex3d(sin(va1)*sc * cos(ha2), cos(va1)*sc, sin(va1) * sin(ha2)*sc);
				glVertex3d(sin(va1)*sc * cos(ha1), cos(va1)*sc, sin(va1) * sin(ha1)*sc);
			}
			va1 = va2;
			va2 = PI;
		}
	} else {
		va1 = 0;
		for(v=0; v<(node->skyColor.n-1); v++) {
			c1 = &node->skyColor.p[v];
			c2 = &node->skyColor.p[v+1];
			va2 = node->skyAngle.p[v];
			
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;

				bk_emis[0]=c2->c[0]; bk_emis[1]=c2->c[1]; bk_emis[2]=c2->c[2];
				glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
				glColor3d(c2->c[0], c2->c[1], c2->c[2]);
				glVertex3d(sin(va2) * cos(ha1)*sc, cos(va2)*sc, sin(va2) * sin(ha1)*sc);
				glVertex3d(sin(va2) * cos(ha2)*sc, cos(va2)*sc, sin(va2) * sin(ha2)*sc);
				bk_emis[0]=c1->c[0]; bk_emis[1]=c1->c[1]; bk_emis[2]=c1->c[2];
				glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
				glColor3d(c1->c[0], c1->c[1], c1->c[2]);
				glVertex3d(sin(va1) * cos(ha2)*sc, cos(va1)*sc, sin(va1) * sin(ha2)*sc);
				glVertex3d(sin(va1) * cos(ha1)*sc, cos(va1)*sc, sin(va1) * sin(ha1)*sc);
			}
			va1 = va2;
		}

		/* now, the spec states: "If the last skyAngle is less than pi, then the
		  colour band between the last skyAngle and the nadir is clamped to the last skyColor." */
		if (va2 < (PI-0.01)) {
			bk_emis[0]=c2->c[0]; bk_emis[1]=c2->c[1]; bk_emis[2]=c2->c[2];
			glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
			glColor3d(c2->c[0], c2->c[1], c2->c[2]);
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;
	
				glVertex3d(sin(PI) * cos(ha1)*sc, cos(PI)*sc, sin(PI) * sin(ha1)*sc);
				glVertex3d(sin(PI) * cos(ha2)*sc, cos(PI)*sc, sin(PI) * sin(ha2)*sc);
				glVertex3d(sin(va2) * cos(ha2)*sc, cos(va2)*sc, sin(va2) * sin(ha2)*sc);
				glVertex3d(sin(va2) * cos(ha1)*sc, cos(va2)*sc, sin(va2) * sin(ha1)*sc);
			}
		}
	}
	glEnd();


	/* Do the ground, if there is anything  to do. */
	if (node->groundColor.n>0) {
		// JAS sc = 1250.0; /* where to put the ground quads */
		sc = 12500.0; /* where to put the ground quads */
		glBegin(GL_QUADS);
		if(node->groundColor.n == 1) {
			c1 = &node->groundColor.p[0];
			bk_emis[0]=c1->c[0]; bk_emis[1]=c1->c[1]; bk_emis[2]=c1->c[2];
			glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
			glColor3d(c1->c[0], c1->c[1], c1->c[2]);
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;

				glVertex3d(sin(PI) * cos(ha1)*sc, cos(PI)*sc, sin(PI) * sin(ha1)*sc);
				glVertex3d(sin(PI) * cos(ha2)*sc, cos(PI)*sc, sin(PI) * sin(ha2)*sc);
				glVertex3d(sin(PI/2) * cos(ha2)*sc, cos(PI/2)*sc, sin(PI/2) * sin(ha2)*sc);
				glVertex3d(sin(PI/2) * cos(ha1)*sc, cos(PI/2)*sc, sin(PI/2) * sin(ha1)*sc);
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

					bk_emis[0]=c1->c[0]; bk_emis[1]=c1->c[1]; bk_emis[2]=c1->c[2];
					glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
					glColor3d(c1->c[0], c1->c[1], c1->c[2]);
					glVertex3d(sin(va1) * cos(ha1)*sc, cos(va1)*sc, sin(va1) * sin(ha1)*sc);
					glVertex3d(sin(va1) * cos(ha2)*sc, cos(va1)*sc, sin(va1) * sin(ha2)*sc);

					bk_emis[0]=c2->c[0]; bk_emis[1]=c2->c[1]; bk_emis[2]=c2->c[2];
					glMaterialfv(GL_FRONT,GL_EMISSION, bk_emis);
					glColor3d(c2->c[0], c2->c[1], c2->c[2]);
					glVertex3d(sin(va2) * cos(ha2)*sc, cos(va2)*sc, sin(va2) * sin(ha2)*sc);
					glVertex3d(sin(va2) * cos(ha1)*sc, cos(va2)*sc, sin(va2) * sin(ha1)*sc);
				}
				va1 = va2;
			}
		}
		glEnd();
	}




	/* now, for the textures, if they exist */

	if ((node->__textureback>0) || 
			(node->__texturefront>0) || 
			(node->__textureleft>0) || 
			(node->__textureright>0) || 
			(node->__texturetop>0) || 
			(node->__texturebottom>0)) {
        	GLfloat mat_emission[] = {1.0,1.0,1.0,1.0};
       	 	GLfloat col_amb[] = {1.0, 1.0, 1.0, 1.0};
       	 	GLfloat col_dif[] = {1.0, 1.0, 1.0, 1.0};

        	glEnable (GL_LIGHTING);
        	glEnable(GL_TEXTURE_2D);
        	glColor3d(1.0,1.0,1.0);

		// JAS sc = 500.0; /* where to put the tex vertexes */
		sc = 5000.0; /* where to put the tex vertexes */

        	glMaterialfv(GL_FRONT,GL_EMISSION, mat_emission);
        	glLightfv (GL_LIGHT0, GL_AMBIENT, col_amb);
        	glLightfv (GL_LIGHT0, GL_DIFFUSE, col_dif);

		/* go through each of the 6 possible sides */

		if(node->__textureback>0) {
			bind_image (bckptr,node->__textureback, 0,0,node->__istemporaryback);
			glBegin(GL_QUADS);
			glNormal3d(0.0,0.0,1.0); 
			glTexCoord2d(1.0, 0.0); glVertex3d(-sc, -sc, sc);
			glTexCoord2d(1.0, 1.0); glVertex3d(-sc, sc, sc);
			glTexCoord2d(0.0, 1.0); glVertex3d(sc, sc, sc);
			glTexCoord2d(0.0, 0.0); glVertex3d(sc, -sc, sc);
			glEnd();
		};

		if(node->__texturefront>0) {
			bind_image (frtptr,node->__texturefront, 0,0,node->__istemporaryfront);
			glBegin(GL_QUADS);
			glNormal3d(0.0,0.0,-1.0);
			glTexCoord2d(1.0,1.0); glVertex3d(sc,sc,-sc);
			glTexCoord2d(0.0,1.0); glVertex3d(-sc,sc,-sc);
			glTexCoord2d(0.0,0.0); glVertex3d(-sc,-sc,-sc);
			glTexCoord2d(1.0,0.0); glVertex3d(sc,-sc,-sc); 
			glEnd();
		};

		if(node->__texturetop>0) {
			bind_image (topptr,node->__texturetop, 0,0,node->__istemporarytop);
			glBegin(GL_QUADS);
			glNormal3d(0.0,1.0,0.0);
			glTexCoord2d(1.0,1.0); glVertex3d(sc,sc,sc);
			glTexCoord2d(0.0,1.0); glVertex3d(-sc,sc,sc);
			glTexCoord2d(0.0,0.0); glVertex3d(-sc,sc,-sc);
			glTexCoord2d(1.0,0.0); glVertex3d(sc,sc,-sc);
			glEnd();
		};

		if(node->__texturebottom>0) {
			bind_image (botptr,node->__texturebottom, 0,0,node->__istemporarybottom);
			glBegin(GL_QUADS);
			glNormal3d(0.0,-(1.0),0.0);
			glTexCoord2d(1.0,1.0); glVertex3d(sc,-sc,-sc);
			glTexCoord2d(0.0,1.0); glVertex3d(-sc,-sc,-sc);
			glTexCoord2d(0.0,0.0); glVertex3d(-sc,-sc,sc);
			glTexCoord2d(1.0,0.0); glVertex3d(sc,-sc,sc);
			glEnd();
		};

		if(node->__textureright>0) {
			bind_image (rtptr,node->__textureright, 0,0,node->__istemporaryright);
			glBegin(GL_QUADS);
			glNormal3d(1.0,0.0,0.0);
			glTexCoord2d(1.0,1.0); glVertex3d(sc,sc,sc);
			glTexCoord2d(0.0,1.0); glVertex3d(sc,sc,-sc);
			glTexCoord2d(0.0,0.0); glVertex3d(sc,-sc,-sc);
			glTexCoord2d(1.0,0.0); glVertex3d(sc,-sc,sc);
			glEnd();
		};

		if(node->__textureleft>0) {
			bind_image (lftptr,node->__textureleft, 0,0,node->__istemporaryleft);
			glBegin(GL_QUADS);
			glNormal3d(-1.0,0.0,0.0);
			glTexCoord2d(1.0,1.0); glVertex3d(-sc,sc, -sc);
			glTexCoord2d(0.0,1.0); glVertex3d(-sc,sc,  sc); 
			glTexCoord2d(0.0,0.0); glVertex3d(-sc,-sc, sc);
			glTexCoord2d(1.0,0.0); glVertex3d(-sc,-sc,-sc);
			glEnd();
		 };
	}

	/* end of textures... */
	glEndList();
	glPopMatrix();
	glPopAttrib();
}
